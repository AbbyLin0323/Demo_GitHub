/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
* and may contain trade secrets and/or other confidential information of VIA   *
* Technologies,Inc.                                                            *
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : L0_NVMe.c
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2014.11.18
*===============================================================================
* Modify Record:
*=============================================================================*/

#include "BaseDef.h"
#ifdef SIM
#include "windows.h"
#else
#include "HAL_PM.h"
#endif
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "COM_BitMask.h"
#include "HAL_Inc.h"
#include "HAL_HwDebug.h"
#include "HAL_HCmdTimer.h"
#include "HAL_HCT.h"
#include "HAL_NVME.h"
#include "HAL_NVMECFGEX.h"
#include "HAL_SGE.h"
#include "NvmeSpec.h"
#include "L0_Config.h"
#include "L0_NVMe.h"
#include "L0_NVMeHCT.h"
#include "L0_NVMeSchedule.h"
#include "L0_NVMeAdminCmd.h"
#include "L0_NVMeDataIO.h"
#include "L0_NVMeNVMCmd.h"
#include "L0_NVMeErrHandle.h"
#include "L0_TrimProcess.h"
#include "L0_Interface.h"
#include "L0_TaskManager.h"
#include "L0_Event.h"
#include "FW_Event.h"

volatile U32 g_PmCtrMpMode;
volatile struct hal_nvmecfg *g_pNVMeCfgReg;
volatile NVME_CFG_EX        *g_pNVMeCfgExReg;

extern U32 g_ulHostInfoAddr;
extern U32 g_ulDevParamAddr;
extern U32 g_ulGlobalInfoAddr;
extern U32 g_ulATAGPLBuffStart;
extern U32 g_ulInfoIdfyPage;
extern U32 g_ulATARawBuffStart;
extern U32 g_ulSubsysNum, g_ulSubsysNumBits;
extern U32 g_ulL0IdleTaskFinished;
extern U32 g_ulSubSysBootOk;
extern U32 g_ulMaxHostLBA;
extern U32 g_ulMaxBlockNum;
extern PTABLE *g_pBootParamTable;
extern L0_TASK_MANAGER g_tL0TaskMgr;
extern FW_VERSION_INFO *g_pFwVersionInfo;
extern FW_UPDATE g_tFwUpdate;
extern BOOL g_ulUARTCmdPending;

extern U32 L0_RAMAlloc(U32 ulRAMBaseAddr, U32 ulAllocLen, U32 *pStartPtr, U32 ulAlignToBits);
extern BOOL L0_TaskWaitBoot(void *pParam);
extern BOOL L0_TaskBootDone(void *pParam);
extern BOOL L0_TaskShutDown(void *pParam);

LOCAL PCOMMAND_TABLE g_pLocCmdTable;
LOCAL PCOMMAND_HEADER g_pLocCmdHeader;
LOCAL L0MSGQ l_L0MsgQueue;
LOCAL volatile PL0MSGQ l_pL0MsgQueue;
LOCAL volatile PL0MSGNODE l_pMsgNode = NULL;

LOCAL U32 l_ulHWCQInfo[MAX_CQ_NUM];
LOCAL U16 l_ulHWSQInfo[MAX_SQ_NUM];
LOCAL U32 l_ulPCIeTxRxNSetting;
LOCAL U32 l_ulPCIeBusDeviceNumber;

#if 0
LOCAL EPHY_RF_TABLE_ENTRY l_aEPHYRFSettingTable[] = 
{
    {0, 0},
    {4, 0},
    {16, 0},
    {20, 0},
    {28, 0},
    {32, 0},
    {36, 0},
    {48, 0},
    {52, 0},
    {56, 0},
    {68, 0},
    {72, 0},
    {76, 0},
    {84, 0},
    {INVALID_8F, 0}
};
#endif

GLOBAL U32 g_ulSysLBAMax;
GLOBAL U32 g_ulATAGPLBuffStart;
GLOBAL U32 g_ulInfoIdfyPage;
GLOBAL NVME_MGR gNvmeMgr;
GLOBAL NVME_TEMPERATURE_THRESHOLD_TABLE g_tNVMeTempThrldTable;

GLOBAL U32 g_ulSecPerDataBlk;
GLOBAL U32 g_ulSecPerDataBlkBits;

#ifndef SIM
extern GLOBAL BOOL bDPLLGating;
#endif

BOOL L0_NVMeTaskBootDone(void *pParam)
{
    L0_InitIdentifyData();
    L0_NVMeFeatureTblInit();
    L0_InitLogData();
    (void)L0_IssueAccessDevParamSCmd(0, g_ulDevParamAddr, GLBINFO_LOAD);
    L0_WaitForAllSCmdCpl(0);
    L0_ViaCmdInit();
    L0_FwUpdateInit();
    
    return TRUE;
}

/* Programs EP_Init register: 
        0: Allows host to access configuration space or memory mapped register space;
        1: Does not allow host to access configuration space or memory mapped register space. */
INLINE void L0_NVMeCfgLock(void)
{
    *(volatile U32*)(REG_BASE_HOSTC + 0x50) |= (1 << 31);
    return;
}

INLINE void L0_NVMeCfgUnlock(void)
{
    *(volatile U32*)(REG_BASE_HOSTC + 0x50) &= ~(1 << 31);
    return;
}

#ifndef SIM
void L0_NVMePrepareL12Entry(void)
{
    U32 ulQId;

    /* 1. Saves NVME active SQ/CQ information. */
    for (ulQId = 0; ulQId < MAX_Q_NUM; ulQId++)
    {
        l_ulHWCQInfo[ulQId] = g_pNVMeCfgReg->cq_info[ulQId].dword1;
        l_ulHWSQInfo[ulQId] = g_pNVMeCfgExReg->SQCfgAttr[ulQId].HWRP;
    }

    /* 2. Saves the TXN/RXN setting of EPHY. */
    l_ulPCIeTxRxNSetting = rPCIe(0x670);

    /* 3. Save PCIe Bus/Device number*/
    l_ulPCIeBusDeviceNumber = rPCIe(0x404) & 0x1FFFF;
    
#if 0
    /* 3. Saves EPHY RF registers. */
    for (ulQId = 0; INVALID_8F != l_aEPHYRFSettingTable[ulQId].ulIndex; ulQId++)
    {
        l_aEPHYRFSettingTable[ulQId].ulValue = rPCIe(0x600 + (l_aEPHYRFSettingTable[ulQId].ulIndex << 2));
    }
#endif

    return;
}

void L0_NVMePrepareL12Exit(void)
{
    U32 ulCurrAsyncEvtNum;
    U32 ulRTNRXOut, ulRTNTXOut;
    U32 ulRTNManualSetting;
    U32 i;
    
    /* 1. Restores the TXN/RXN setting of EPHY. */
    ulRTNRXOut = (l_ulPCIeTxRxNSetting & MSK_1F);
    ulRTNTXOut = ((l_ulPCIeTxRxNSetting >> 8) & MSK_1F);

    ulRTNManualSetting = rPCIe(0x608);
    HAL_MCUInsertbits(ulRTNManualSetting, ulRTNRXOut, 16, 19);
    HAL_MCUInsertbits(ulRTNManualSetting, ulRTNTXOut, 20, 23);
    HAL_MCUInsertbits(ulRTNManualSetting, 3, 14, 15);
    rPCIe(0x608) = ulRTNManualSetting;

    /* 2. Restore PCIe Bus/Device numer */
    rPCIeByte(0x1BA) = (U8)(l_ulPCIeBusDeviceNumber & MSK_2F); //Bus Number
    HAL_MCUInsertbits(rPCIeByte(0x1BD), (l_ulPCIeBusDeviceNumber >> 8), 0, 4); // Device Number
    
#if 0
    /* 3. Loads EPHY RF registers. */
    for (ulRTNRXOut = 0; INVALID_8F != l_aEPHYRFSettingTable[ulRTNRXOut].ulIndex; ulRTNRXOut++)
    {
        rPCIe(0x600 + (l_aEPHYRFSettingTable[ulRTNRXOut].ulIndex << 2)) = l_aEPHYRFSettingTable[ulRTNRXOut].ulValue;
    }
#endif

    /* 4. Restores CST of HIDs reserved for Async Event command. */
    ulCurrAsyncEvtNum = gNvmeMgr.AsyncEvtCtrl.ulOutstdReqCount;
    if ((0 != ulCurrAsyncEvtNum) &&
        (ulCurrAsyncEvtNum <= MAX_ASYN_EVENT_NUM))
    {
        for (i = 0; i < ulCurrAsyncEvtNum; i++)
        {
            HAL_HCTSetCST(i, SLOT_ASYNC_EVT_RESV);
        }
    }

    /* 5. Restores NVME active SQ/CQ information.*/
    for (ulRTNRXOut = 0; ulRTNRXOut < MAX_Q_NUM; ulRTNRXOut++)
    {
        g_pNVMeCfgReg->cq_info[ulRTNRXOut].dword1 = l_ulHWCQInfo[ulRTNRXOut];
        g_pNVMeCfgExReg->SQCfgAttr[ulRTNRXOut].HWRP = l_ulHWSQInfo[ulRTNRXOut];
    }

    /* 6. Enables retention mode ECO fix. */
    rPCIe(0x220) |= (1 << 30);
    
    /* 7. Recovers PCIe from L1.2 Entry ready state. */
    HAL_PMDisablePCIeL12Ready();
    
    /* 8. Clears interrupt status. */
    rHOSTC_INTPENDING = INVALID_8F;

    /* 9. Enables PCIe module to return to normal work status. */
    rGlbClkCtrl &= ~PMU_L1CKG_PCIE;
    HAL_DelayUs(1);
    rGlbSoftRst &= ~(R_RST_PCIE);
    HAL_DelayUs(1);
    rGlbClkCtrl |= PMU_L1CKG_PCIE;

    /* 10. Triggers hardware to load bus/device number in Step 2. */
    rPCIeByte(0x1BD) |= (1 << 7);
    HAL_DelayCycle(8);
    rPCIeByte(0x1BD) &= ~(1 << 7);

    return;
}
#endif

MCU0_DRAM_TEXT U32 L0_NVMeBarSpaceInit(void)
{
    PTABLE * pPTable =  HAL_GetPTableAddr();

    /* 1. Inits NVMe controller register for device capability in bar space before allowing PCIE function to be available. */
    g_pNVMeCfgReg = (volatile struct hal_nvmecfg *)REG_BASE_NVME;
    g_pNVMeCfgExReg = (volatile NVME_CFG_EX *)REG_BASE_EXT_NVME_AF;
#ifdef AF_ENABLE
    g_pNVMeCfgReg->bar.capl    = 0x3C030000 | (0xFFFF & (MAX_IOSQ_DEPTH - 1)); //Support WRR arb
#else
    g_pNVMeCfgReg->bar.capl    = 0x3C010000 | (0xFFFF & (MAX_IOSQ_DEPTH - 1));     
#endif
    g_pNVMeCfgReg->bar.caph    = 0x00000020;  
    g_pNVMeCfgReg->bar.vs      = 0x00010201;
    g_pNVMeCfgReg->bar.intms   = 0;
    g_pNVMeCfgReg->bar.intmc   = 0xFFFFFFFF;
    g_pNVMeCfgReg->bar.csts    = 0;
    g_pNVMeCfgReg->bar.cc      = 0;
    g_pNVMeCfgReg->msix_vector = 0;

    /* 2. Changes DID to 3533 instead of 3514. */
    *((volatile U16*)(REG_BASE_PCIE_NVME + 2)) = 0x3533;

    /* 3. Selects interrupt mode supported that would be reported to host. */
    if(1 == pPTable->tNVMeL0Feature.tNVMEL0Feat.bsNVMeL0MSIXEnable) /* Enable MSI-X*/
    {
        *((volatile U8*)(REG_BASE_PCIE_NVME + 0x91)) = 0xB0;
    }
    else if(2 == pPTable->tNVMeL0Feature.tNVMEL0Feat.bsNVMeL0MSIXEnable) /* Enable MSI*/
    {
        *((volatile U8*)(REG_BASE_PCIE_NVME + 0x91)) = 0xC4;
        //*((volatile U32*)(REG_BASE_HOSTC + 0x44)) = 0x0D;
    }
    else        
    {
        *((volatile U8*)(REG_BASE_PCIE_NVME + 0x81)) = 0xC4;/* INTA Only,disable MSI/MSI-X */
    }

    /* 4. Patches IO BAR to allow new Win10 driver to enumerate us correctly. */
    rPCIeNCfg(0x18) |= 1; // Due to a logic design bug, we writes rNVMe[0x18] but actually modifies reported behavior of IO BAR 3 (rNVME[0x1c]).
    *((volatile U32*)(REG_BASE_HOSTC+ 0x2C)) = 0xFFFFFFFF; // Sets BAR3 host access attribute to all RW.

    // Patch for HOSTC performance
#ifdef VT3533_A2ECO_HOSTCPFMC
    rEXT(0x378) = 0x40;
    rEXT(0x37C) = 0x40;
#else
    rEXT(0x378) = 0x30040;
    rEXT(0x37C) = 0x30040;
#endif

    rEXT(0x304) = 0x05000A;

    /* 5. Unlocks configuration space to allow host accessing. */
    L0_NVMeCfgUnlock();

    return 0;
}

#ifdef AF_ENABLE
/* AutoFetch initialization */
MCU0_DRAM_TEXT void L0_NVMeCfgExInit(void)
{
    if ( NULL == g_pNVMeCfgExReg )
    {
        DBG_Printf("Error:g_pNVMeCfgExReg == NULL \n");
        DBG_Getch();
    }

    g_pNVMeCfgExReg->CmdBaseAddr   = 0;    //Base (U16)HCT_S0_BASE; 
    g_pNVMeCfgExReg->CmdLen        = sizeof(COMMAND_HEADER);
    g_pNVMeCfgExReg->CmdOffset     = sizeof(COMMAND_HEADER);  
    g_pNVMeCfgExReg->CstAutoTrig   = SLOT_IDLE;
    g_pHCTControlReg->bsAUTOFCHCST = SLOT_AFSQE_RDY;

    /* S0 is initialized in L0_NVMeMgrInit() */
    return;
}


MCU0_DRAM_TEXT void L0_NVMeDisableSQAF(U32 ulSQId)
{
    if (MAX_SQ_NUM > ulSQId)
    {
        g_pNVMeCfgExReg->SQCfgAttr[ulSQId].BM = 0;
    }

    return;
}

void L0_UpdateSQHead(PCB_MGR pSlot)
{
    Q_INFO * q ;
    U8       uSqid;

    uSqid = pSlot->SQID;

    q = &gNvmeMgr.SQ[uSqid];

    //Set IOSQState to indicate cmd fetch to local device done
    gNvmeMgr.IOSQState[pSlot->SQEntryIndex] |= (1 << uSqid);
    while(gNvmeMgr.IOSQState[q->SqHead]&(1 << uSqid))
    {
        gNvmeMgr.IOSQState[q->SqHead++] &= ~(1 << uSqid);
        if(q->SqHead == q->QSIZE)
        {
            q->SqHead = 0;
        }
    }
    
    //update SQ head register
    g_pNVMeCfgReg->sq_ptr[uSqid].head = gNvmeMgr.SQ[uSqid].SqHead;

    return;
}
#endif //AF_ENABLE


MCU0_DRAM_TEXT void L0_InitIdentifyData(void)
{
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;
    volatile ADMIN_IDENTIFY_NAMESPACE *pIDNS;
    volatile PHOST_INFO_PAGE pHostInfoPage;
    PTABLE * pPTable =  HAL_GetPTableAddr();
    U8 ucMNByteIndex;
    U8 ucSNByteIndex;
    UCHAR ucFwRv[8];
    U8 ucFwRvIndex;
    
    COM_MemZero((U32 *)g_ulInfoIdfyPage, (4096u * 3) >> DWORD_SIZE_BITS);
     
    pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;

    pIDCTR->VID = 0x1106;
    pIDCTR->SSVID = 0x1106;

    /* Firmware Reversion */
    COM_DwordToString(g_pFwVersionInfo->ulFWRleaseVerion, (U8*)&ucFwRv[0]);
    for( ucFwRvIndex=0; ucFwRvIndex < sizeof(pIDCTR->FR); )
    {
        pIDCTR->FR[ucFwRvIndex] = ucFwRv[ucFwRvIndex + 1];
        pIDCTR->FR[ucFwRvIndex+1] = ucFwRv[ucFwRvIndex];
        ucFwRvIndex +=2;
    }
    
    /* Disk Name*/
    for(ucMNByteIndex=0; ucMNByteIndex < sizeof(pIDCTR->MN); ucMNByteIndex++)
    {
        pIDCTR->MN[ucMNByteIndex++] = (g_pBootParamTable->aFWDiskName[ucMNByteIndex/sizeof(U32)] >> 8) & 0xff;
        pIDCTR->MN[ucMNByteIndex++] = g_pBootParamTable->aFWDiskName[ucMNByteIndex/sizeof(U32)] & 0xff;
        pIDCTR->MN[ucMNByteIndex++] = (g_pBootParamTable->aFWDiskName[ucMNByteIndex/sizeof(U32)] >> 24) & 0xff;
        pIDCTR->MN[ucMNByteIndex] = (g_pBootParamTable->aFWDiskName[ucMNByteIndex/sizeof(U32)] >> 16) & 0xff;
    }
    /* Serial Number */
    for (ucSNByteIndex = 0; ucSNByteIndex < sizeof(pIDCTR->SN); ucSNByteIndex++)
    {
        pIDCTR->SN[ucSNByteIndex++] = (g_pBootParamTable->aFWSerialNum[ucSNByteIndex / sizeof(U32)] >> 8) & 0xff;
        pIDCTR->SN[ucSNByteIndex++] = g_pBootParamTable->aFWSerialNum[ucSNByteIndex / sizeof(U32)] & 0xff;
        pIDCTR->SN[ucSNByteIndex++] = (g_pBootParamTable->aFWSerialNum[ucSNByteIndex / sizeof(U32)] >> 24) & 0xff;
        pIDCTR->SN[ucSNByteIndex] = (g_pBootParamTable->aFWSerialNum[ucSNByteIndex / sizeof(U32)] >> 16) & 0xff;
    }
    
#ifdef AF_ENABLE
    pIDCTR->RAB = 0x08;
#else
    //pIDCTR->RAB = 0x00;//INTEL  0x00
#endif
    pIDCTR->IEEE[0] = 0xf2;
    pIDCTR->IEEE[1] = 0x1f;
    pIDCTR->IEEE[2] = 0x00;

    //OACS INTEL 0x0006
    //pIDCTR->OACS.SupportsSecuritySendSecurityReceive = FALSE;
    pIDCTR->OACS.SupportsFormatNVM = TRUE;
    pIDCTR->OACS.SupportsFirmwareActivateFirmwareDownload = TRUE;
    //pIDCTR->OACS.Reserved = 0;

    /*Format NVM Support */
    //pIDCTR->FNA.SupportsCryptographicErase = FALSE;
    pIDCTR->FNA.FormatAppliesToAllNamespaces = TRUE;
    //pIDCTR->FNA.SecureEraseAppliesToAllNamespaces = FALSE;

    /* Does not report volatile cache present */
    //pIDCTR->VWC.Present = TRUE;

    //pIDCTR->LPA.SupportsSMART_HealthInformationLogPage = FALSE;
    pIDCTR->LPA.SupportsExtendedDataforGetLogPage = TRUE;

    pIDCTR->SQES.RequiredSubmissionQueueEntrySize = 0x06;
    pIDCTR->SQES.MaximumSubmissionQueueEntrySize  = 0x06;

    pIDCTR->CQES.RequiredCompletionQueueEntrySize = 0x04;
    pIDCTR->CQES.MaximumCompletionQueueEntrySize  = 0x04;

    pIDCTR->NN = 1;

    pIDCTR->AVSCC = TRUE;

    pIDCTR->PSD[0].MP = 2500; // 25W
    pIDCTR->PSD[0].ENLAT = 15; // 15us
    pIDCTR->PSD[0].EXLAT = 5; // 5us
    pIDCTR->PSD[0].IDLP = 10; // 100mW
    pIDCTR->PSD[0].IPS = 2; // Unit is 10mW
    pIDCTR->PSD[0].ACTP = 2500; // 25W
    pIDCTR->PSD[0].APW = 2; // Heavy Sequential Write
    pIDCTR->PSD[0].APS = 2; // Unit is 10mW

    pIDCTR->MDTS = 0x05;
    g_ulMaxBlockNum = (1u << (0x05 + HPAGE_SIZE_BITS - DATABLOCK_SIZE_BITS));
    pIDCTR->ACL  = 0x03;
    pIDCTR->UAERL = 0x03;
    pIDCTR->ELPE = 0x3F;
    
    //pIDCTR->ONCS.SupportSetFeatureSave = TRUE;
    /* Support DataSet Management (Trim) and Write Zeroes */
    pIDCTR->ONCS.SupportsWriteZero = TRUE;
    pIDCTR->ONCS.SupportsDataSetManagement = TRUE; 

    pIDCTR->RTD3R = (200u * 1000u); // 200ms for boot
    pIDCTR->RTD3E = (160u * 1000u); // 160ms for normal shutdown

    /* Warning and Critical Temperature Report */
    pIDCTR->WCTEMP = 0x157;
    pIDCTR->CCTEMP = 0x175;

    //FW Update Supported
    pIDCTR->OACS.SupportsFirmwareActivateFirmwareDownload = TRUE;
    pIDCTR->FRMW.FirstFirmwareSlotReadOnly = TRUE;         //Slot 1 is read only
    pIDCTR->FRMW.SupportedNumberOfFirmwareSlots = 2;    //Slot 1&2 is valid.

    /*Set IEEE OUI Identify*/
    pIDCTR->IEEE[0] = (pPTable->ulWorldWideName[0] >> 4) & 0xff;
    pIDCTR->IEEE[1] = (pPTable->ulWorldWideName[0] >> 12) & 0xff;
    pIDCTR->IEEE[2] = (pPTable->ulWorldWideName[0] >> 20) & 0xff;

    pIDNS = (volatile ADMIN_IDENTIFY_NAMESPACE *)(g_ulInfoIdfyPage + MCU0_NAMESPACE_IDENTIFY_OFFSET);

    if (1 == pPTable->tNVMeL0Feature.tNVMEL0Feat.bsNVMeL0LBA4KBEnable) // 4K data Block
    {
        g_ulSecPerDataBlk = 8;
        g_ulSecPerDataBlkBits = 3;
    }
    else //512 byte data block
    {
        g_ulSecPerDataBlk = 1;
        g_ulSecPerDataBlkBits = 0;
    }

    pIDNS->LBAFx[0].LBADS = 9 + SEC_PER_DATABLOCK_BITS;
    pIDNS->NLBAF = 0;// INTEL 6
    // INTEL : BYTE 27 = 1
    // INTEL : BYTE 28 = 0x11

    pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    pIDNS->NCAP = g_ulMaxHostLBA =
        pHostInfoPage->HPAMaxLBA >> SEC_PER_DATABLOCK_BITS;
    pIDNS->NUSE = pIDNS->NCAP;
    pIDNS->NSZE = pIDNS->NCAP;
    g_ulSysLBAMax = pHostInfoPage->HPAMaxLBA;

    //pIDNS->NSFEAT.SupportsThinProvisioning = TRUE;

    return;
}


MCU0_DRAM_TEXT void L0_NVMeMgrInit(PNVME_MGR Mgr)
{
    U8 i;
    COM_MemZero((U32*)Mgr,sizeof(NVME_MGR)/sizeof(U32));

    //Init Admin Queue
    Mgr->SQ[0].Valid = TRUE;
    Mgr->SQ[0].AdrL = g_pNVMeCfgReg->bar.asql;
    Mgr->SQ[0].AdrH = g_pNVMeCfgReg->bar.asqh;
    Mgr->SQ[0].QSIZE = (g_pNVMeCfgReg->bar.aqa&0xfff)+1;
    Mgr->SQ[0].SqHead  = 0;
    Mgr->CQ[0].QSIZE = ((g_pNVMeCfgReg->bar.aqa>>16)&0xfff)+1;
    Mgr->CQ[0].Valid = TRUE;
    Mgr->CQ[0].IEN = TRUE;

    g_pNVMeCfgReg->cq_info[0].base_addr_l = g_pNVMeCfgReg->bar.acql;
    g_pNVMeCfgReg->cq_info[0].base_addr_h = g_pNVMeCfgReg->bar.acqh;
    g_pNVMeCfgReg->cq0_size = (g_pNVMeCfgReg->bar.aqa>>16)&0xfff;
    g_pNVMeCfgReg->cq_addr_inc_l = 16;
    g_pNVMeCfgReg->cq_addr_inc_h = 0;

#ifdef AF_ENABLE
    g_pNVMeCfgExReg->SQCfgAttr[0].BaseAddrL = g_pNVMeCfgReg->bar.asql;
    g_pNVMeCfgExReg->SQCfgAttr[0].BaseAddrH = g_pNVMeCfgReg->bar.asqh;
    g_pNVMeCfgExReg->SQCfgAttr[0].Size      = (g_pNVMeCfgReg->bar.aqa&0xfff);
    g_pNVMeCfgExReg->SQCfgAttr[0].StepAddr  = sizeof(COMMAND_HEADER);
    g_pNVMeCfgExReg->SQCfgAttr[0].CQMaped   = 0;
    g_pNVMeCfgExReg->SQCfgAttr[0].BM        = MAX_BM_ADMINSQ;   //Fetch Admin One by One;
    g_pNVMeCfgExReg->SQCfgAttr[0].P         = PRIORITY_ADMIN;   //Admin Priority
#endif //AF_ENABLE

    // all Q pointer clear to zero
    g_pNVMeCfgReg->cq_clr |= 0x01ff;
    g_pNVMeCfgReg->sq_clr |= 0x01ff;
    //clear interrupt status
    g_pNVMeCfgReg->int_status |= 0x01ff;
    g_pNVMeCfgReg->intvec_int_status |= 0x01ff;

    //Init Slot
    for( i = 0; i < MAX_SLOT_NUM; i++)
    {
        Mgr->CbMgr[ i ].SlotNum             = i;
        Mgr->CbMgr[ i ].Ch                  = &g_pLocCmdHeader[i];
        Mgr->CbMgr[ i ].PRPTable            = g_pLocCmdTable[i].PRPTable;
    }

    //Init CST
    for( i = 0; i < MAX_SLOT_NUM; i++)
    {
        HAL_HCTSetCST(i, SLOT_IDLE);
    }

    //init IO SQ/CQ status
    for( i = 1; i < MAX_SQ_NUM; i++)
    {
        Mgr->CQ[i].Valid = FALSE;
        Mgr->SQ[i].Valid = FALSE;
        Mgr->SQ[i].SqHead = 0;
        Mgr->CQ[i].CqTail = 0;
    }

    Mgr->ActiveSlot = INVALID_CMD_ID;
    Mgr->AdminProcessingSlot = INVALID_CMD_ID;
    Mgr->AdminProcessing = FALSE;

    //Init Async Event control info
    Mgr->AsyncEvtCtrl.tHostConfig.ulCmdDW11 = 0x71F;

    //clear fw update data
    g_tFwUpdate.ulDldSize    = 0;
    //g_tFwUpdate.ulCRC        = 0;

    return ;
}

void L0_NVMeWarmBootInit(void)
{
    //all the registers related restored by HW
    //all the variables related restored by other modules
    //do nothing
    return;
}

void L0_NVMeStoreforPowerLoss(void)
{
    //all the registers related stored by HW
    //all the variables related stored by other modules
    //do nothing
    return;
}

U32 L0_CalcPRPCount(U32 ulPrp1DW0,U32 ulCmdDataLen)
{
    U32 ulPRP1MaxDTBytes;
    U32 ulPRPLeftXferBytes;
    U32 ulTotalPRPNeeded;

    ulPRP1MaxDTBytes = PRP_ABILITY(ulPrp1DW0);

    if (0 == ulCmdDataLen)
    {
        ulTotalPRPNeeded = 1;
    }

    else if (ulCmdDataLen <= ulPRP1MaxDTBytes)
    {
        ulTotalPRPNeeded = 1;
    }

    else
    {
        ulPRPLeftXferBytes = ulCmdDataLen - ulPRP1MaxDTBytes;
        ulTotalPRPNeeded = 1 + (ulPRPLeftXferBytes >> HPAGE_SIZE_BITS);

        if (0 != (ulPRPLeftXferBytes & HPAGE_SIZE_MSK))
        {
            ulTotalPRPNeeded++;
        }

    }

    return ulTotalPRPNeeded;
}

#if 0
void L0_CheckAndReadPRPList(PCB_MGR pCurrSlot)
{
    U32 ulViaCmdCode;
    U32 ulDataBytes;
    U32 ulPRPCount;

    /* 1. Acquires data transfer length in byte */
    /* for Admin command */
    if(0 == pCurrSlot->SQID)
    {
        if (ACS_VIA_VENDOR_CMD == pCurrSlot->Ch->OPC)
        {
            ulViaCmdCode = pCurrSlot->Ch->DW12 & MSK_2F;
            if ((VIA_CMD_MEM_READ == ulViaCmdCode) || (VIA_CMD_MEM_WRITE == ulViaCmdCode))
            {
                ulDataBytes = pCurrSlot->Ch->DW14;
            }
            else
            {
                ulDataBytes = 0;
            }
        }     

        else if (ACS_IMG_DOWNLOAD == pCurrSlot->Ch->OPC)
        {
            ulDataBytes = (pCurrSlot->Ch->DW10 + 1) << 2;
        }

        else
        {
            ulDataBytes = 0;
        }
    }

    else
    {
        /* for NVM command */
        switch(pCurrSlot->Ch->OPC)
        {
            case NCS_READ:
            case NCS_WRITE:
                ulDataBytes = ((pCurrSlot->Ch->DW12 & MSK_4F) + 1) << DATABLOCK_SIZE_BITS;
                break;

            case NCS_DATASET_MANAGEMENT:
                ulDataBytes = ((pCurrSlot->Ch->DW10 & MSK_2F) + 1) << 4;
                break;
                
            default:
                DBG_Printf("unknown NVM command: slot[%d], OpCode[%d]\n", pCurrSlot->SlotNum,pCurrSlot->Ch->OPC);
                L0_NVMeCmdError(pCurrSlot->SlotNum, NVME_SF_IVLD_OPCODE);

            case NCS_FLUSH:
            case NCS_WRITE_ZERO:
                ulDataBytes = 0;
                break;
                
        }
    }

    /* 2. Calculates total PRP count needed */
    ulPRPCount = L0_CalcPRPCount(pCurrSlot->Ch->PRP1L, ulDataBytes);
    pCurrSlot->TotalPRPCnt = ulPRPCount;

    /* 3. Fetches PRP list if needed */
    if (2 < ulPRPCount)
    {
        /* PRP List shall be QWORD aligned */
        if (0 != (pCurrSlot->PRPTable[1].Offset & (QWORD_SIZE - 1)))
        {
            DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pCurrSlot->PRPTable[1].ulDW0, pCurrSlot->PRPTable[1].ulDW1);
            L0_NVMeForceCompleteCmd(pCurrSlot, NVME_SF_IVLD_PRP_OFS);
        }

        else if ((0 == pCurrSlot->PRPTable[1].ulDW0) &&
            (0 == pCurrSlot->PRPTable[1].ulDW1))
        {
            pCurrSlot->TotalPRPCnt = 1;
            HAL_HCTSetCST(pCurrSlot->SlotNum, SLOT_PRPLIST_RDY);
            pCurrSlot->CmdState = CMD_NEW;
        }

        else
        {
            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp start:");
            L0_NVMeHCTReadPRP(pCurrSlot);
            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp end  :");
        }
    }

    else
    {
        HAL_HCTSetCST(pCurrSlot->SlotNum, SLOT_PRPLIST_RDY);
        pCurrSlot->CmdState = CMD_NEW;
    }

    return;
}
#else
void L0_MovePRPtoPRPList(PCB_MGR pSlot)
{
    U32 *pSrcInSQE;
    U32 *pDestInPRPT;

    pSrcInSQE = (U32 *)&(pSlot->Ch->PRP1L);
    pDestInPRPT = (U32 *)(pSlot->PRPTable);

    /* 2 PRP entry DW count */
    COM_MemCpy(pDestInPRPT, pSrcInSQE, (sizeof(PRP) >> (DWORD_SIZE_BITS - 1)));

    return;
}

U32 L0_CheckNeedToFetchPRPList(PCB_MGR pCurrSlot)
{
    U32 ulDataBytes;
    U32 ulPRPCount;
    U32 ulStatus;

    /* 1. Copies PRP entries in SQ entry to PRP table area. */
    L0_MovePRPtoPRPList(pCurrSlot);

    /* 2. Calculates total PRP count needed by transfer length of current command. */
    ulDataBytes = pCurrSlot->TotalRemainingBytes;
    ulPRPCount = L0_CalcPRPCount(pCurrSlot->Ch->PRP1L, ulDataBytes);
    pCurrSlot->TotalPRPCnt = ulPRPCount;

    /* 3. Checks if PRP list is valid and whether we need to fetch more PRPs from host DRAM. */
    if (2 < ulPRPCount)
    {
        /* PRP List shall be QWORD aligned */
        if (0 != (pCurrSlot->PRPTable[1].ulDW0 & (QWORD_SIZE - 1)))
        {
            ulStatus = PRPLISTSTS_INVALIDADDR;
        }

        else
        {
            pCurrSlot->IsNeedPRPList = TRUE;
            ulStatus = PRPLISTSTS_NEEDFETCH;
        }
    }

    else
    {
        ulStatus = PRPLISTSTS_READY;
    }

    return ulStatus;
}

U32 L0_IsPRPListOffsetValid(PCB_MGR pSlot)
{
    U32 ulCheckPRPCount;
    PPRP pCurrPRP;

    //checks prp
    ulCheckPRPCount = pSlot->TotalPRPCnt - 1;
    pCurrPRP = &(pSlot->PRPTable[1]);

    while (0 != ulCheckPRPCount--)
    {
        if (0 != pCurrPRP->Offset)
        {
            DBG_Printf("R/W PRP Offset Invalid (%d)!\n", pCurrPRP->ulDW0);
            return FALSE;
        }

        pCurrPRP++;
    }

    return TRUE;
}

#endif

void L0_UpdateIOSQHead(PCB_MGR pSlot)
{
    Q_INFO * q ;
    U8       uSqid;

    uSqid = pSlot->SQID;

    if ( !((0 < uSqid) && (uSqid < 9)) )
    {
        DBG_Printf("Error:L0_UpdateIOSQHead() got Invalid SQID:%d\n", uSqid);
        DBG_Getch();
    }
    q = &gNvmeMgr.SQ[uSqid];

    //Set IOSQState to indicate cmd fetch to local device done
    gNvmeMgr.IOSQState[pSlot->SQEntryIndex] |= (1 << (uSqid-1));
    while(gNvmeMgr.IOSQState[q->SqHead]&(1 << (uSqid-1)))
    {
        gNvmeMgr.IOSQState[q->SqHead++] &= ~(1 << (uSqid-1));
        if(q->SqHead == q->QSIZE)
        {
            q->SqHead = 0;
        }
    }
    
    //update SQ head register
    g_pNVMeCfgReg->sq_ptr[uSqid].head = gNvmeMgr.SQ[uSqid].SqHead;
}

void L0_UpdateASQHead(PCB_MGR pSlot)
{
    Q_INFO * q;
    
    if (0 != pSlot->SQID)
    {
        DBG_Printf("Error:L0_UpdateASQHead() got IOSQID\n");
        DBG_Getch();
    }

    q = &gNvmeMgr.SQ[0];
    q->SqHead++;
    if (q->SqHead == q->QSIZE)
    {
        q->SqHead = 0;
    }

    //update SQ head register
    g_pNVMeCfgReg->sq_ptr[0].head = gNvmeMgr.SQ[0].SqHead;
    return;    
}

void L0_NVMeSetupSimpleCompletion(PCB_MGR pSlot, U32 ulWaitSGE, U32 ulUpdateSQHead)
{
    volatile NVME_WBQ *pTargetWBQEntry;
    Q_INFO *pCQ;
    U32 ulCQId;

    pTargetWBQEntry = (PNVME_WBQ)HAL_HCTGetWBQEntry(pSlot->SlotNum, 0);
    ulCQId = pSlot->CQID;
    pCQ = &gNvmeMgr.CQ[ulCQId];

    /* Zero-filling the temp WBQ entry. */
    COM_MemZero( (U32*)pTargetWBQEntry, sizeof( NVME_WBQ ) / sizeof( U32 ) );

    /* Updating command state is required. */    
    pTargetWBQEntry->Id = pSlot->SlotNum;
    pTargetWBQEntry->Cqid = ulCQId;
    pTargetWBQEntry->Sqid = pSlot->SQID;
    pTargetWBQEntry->StatusF = pSlot->CmdSts;
    pTargetWBQEntry->CmdID = pSlot->Ch->CID;
    pTargetWBQEntry->CmdSpec = pSlot->CSPC;


    pTargetWBQEntry->NST = SLOT_IDLE;
    pTargetWBQEntry->Update = TRUE;

    pTargetWBQEntry->R3 = pCQ->IEN;
    pTargetWBQEntry->Rf = 0;

    /* Completing the WBQ. */
    pTargetWBQEntry->Last = TRUE;
    pTargetWBQEntry->WSGE = ulWaitSGE;

    /* Updating corresponding SQ head. */
    if (FALSE != ulUpdateSQHead)
    {
#ifdef AF_ENABLE
        L0_UpdateSQHead(pSlot);
#else
        if (0 == pSlot->SQID)
        {
            L0_UpdateASQHead(pSlot);
        }
        else
        {
            L0_UpdateIOSQHead(pSlot);
        }
#endif
    }

    return;
}

void L0_GlobalInfoUpdateLBACnt(PCB_MGR pSlot)
{
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    if (NCS_READ == pSlot->Ch->OPC)
    {
        if ((INVALID_8F - pHostInfoPage->TotalLBAReadLow) <= pSlot->TotalSecCnt)
        {
            pHostInfoPage->TotalLBAReadHigh++;
            pHostInfoPage->TotalLBAReadLow = pSlot->TotalSecCnt - (INVALID_8F - pHostInfoPage->TotalLBAReadLow);
        }
        else
        {
            pHostInfoPage->TotalLBAReadLow += pSlot->TotalSecCnt;
        }
    }
    else
    {
        if ((INVALID_8F - pHostInfoPage->TotalLBAWrittenLow) <= pSlot->TotalSecCnt)
        {
            pHostInfoPage->TotalLBAWrittenHigh++;
            pHostInfoPage->TotalLBAWrittenLow = pSlot->TotalSecCnt - (INVALID_8F - pHostInfoPage->TotalLBAWrittenLow);
        }
        else
        {
            pHostInfoPage->TotalLBAWrittenLow += pSlot->TotalSecCnt;
        }
    }

    return;
}

void L0_NVMeSetCmdParam(PCB_MGR pSlot)
{
    U16 usRangeNum;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    switch(pSlot->Ch->OPC)
    {
        case NCS_READ:
            pSlot->IsWriteDir = 1;
            pHostInfoPage->NvmeHostReadCntDW0++;
            if (0 == pHostInfoPage->NvmeHostReadCntDW0)
            {
                pHostInfoPage->NvmeHostReadCntDW1++;
                if (0 == pHostInfoPage->NvmeHostReadCntDW1)
                {
                    pHostInfoPage->NvmeHostReadCntDW2++;
                    if (0 == pHostInfoPage->NvmeHostReadCntDW2)
                    {
                        pHostInfoPage->NvmeHostReadCntDW3++;
                    }
                }
            }

            break;

        case NCS_WRITE:
            pSlot->IsWriteDir = 0;
            pHostInfoPage->NvmeHostWriteCntDW0++;
            if (0 == pHostInfoPage->NvmeHostWriteCntDW0)
            {
                pHostInfoPage->NvmeHostWriteCntDW1++;
                if (0 == pHostInfoPage->NvmeHostWriteCntDW1)
                {
                    pHostInfoPage->NvmeHostWriteCntDW2++;
                    if (0 == pHostInfoPage->NvmeHostWriteCntDW2)
                    {
                        pHostInfoPage->NvmeHostWriteCntDW3++;
                    }
                }
            }

            break;

        case NCS_WRITE_ZERO:
            pHostInfoPage->NvmeHostWriteCntDW0++;
            if (0 == pHostInfoPage->NvmeHostWriteCntDW0)
            {
                pHostInfoPage->NvmeHostWriteCntDW1++;
                if (0 == pHostInfoPage->NvmeHostWriteCntDW1)
                {
                    pHostInfoPage->NvmeHostWriteCntDW2++;
                    if (0 == pHostInfoPage->NvmeHostWriteCntDW2)
                    {
                        pHostInfoPage->NvmeHostWriteCntDW3++;
                    }
                }
            }
            
            pSlot->CurrentLBAL = (pSlot->Ch->DW10) << SEC_PER_DATABLOCK_BITS;
            pSlot->CurrentLBAH = pSlot->Ch->DW11;
            pSlot->TotalSecCnt = ((pSlot->Ch->DW12 & MSK_4F) + 1) << SEC_PER_DATABLOCK_BITS;
            pSlot->RemSecCnt = pSlot->TotalSecCnt;
            pSlot->TotalRemainingBytes = pSlot->TotalSecCnt << SEC_SIZE_BITS;
            
            L0_GlobalInfoUpdateLBACnt(pSlot);
            break;

        case NCS_DATASET_MANAGEMENT:
            usRangeNum = (pSlot->Ch->DW10 & 0xFF) + 1;
            pSlot->IsWriteDir = 0;
            pSlot->HasDataXfer = TRUE;
            pSlot->IsWriteDir = FALSE;
            pSlot->DataAddr = g_ulATARawBuffStart;
            pSlot->TotalRemainingBytes = usRangeNum * sizeof(LBA_ENTRY);
            break;

        case NCS_FLUSH:
            break;

        default:
            break;
            //ASSERT(FAIL);
    }

    if(NCS_READ == pSlot->Ch->OPC || NCS_WRITE == pSlot->Ch->OPC)
    {
        pSlot->PRPIndex = 0;
        pSlot->PRPOffset = ((PPRP)(&pSlot->Ch->PRP1L))->Offset << 2;
        pSlot->CurrentLBAL = (pSlot->Ch->DW10) << SEC_PER_DATABLOCK_BITS;
        pSlot->CurrentLBAH = pSlot->Ch->DW11;
        pSlot->TotalSecCnt = ((pSlot->Ch->DW12 & MSK_4F) + 1) << SEC_PER_DATABLOCK_BITS;
        pSlot->RemSecCnt = pSlot->TotalSecCnt;
        pSlot->TotalRemainingBytes = pSlot->TotalSecCnt << SEC_SIZE_BITS;

        L0_GlobalInfoUpdateLBACnt(pSlot);
    }

    return;
}

void L0_NVMeCheckSQDoorBell(void)
{
    U32 ulInvalidIOSQBitmap;
    U32 ulInvalidDoorbellBitmap;
    U32 ulInvalidSQId;

    ulInvalidIOSQBitmap = ~(gNvmeMgr.ValidIOSQMap | 1);
    ulInvalidDoorbellBitmap = g_pNVMeCfgReg->cmd_fetch_helper & ulInvalidIOSQBitmap;

    while (0 != ulInvalidDoorbellBitmap)
    {
        /* Host has written to the tail doorbell of a non-exist SQ. */
        /* 1. Acquires an invalid SQ whose tail doorbell has been updated. */
        ulInvalidSQId = 31 - HAL_CLZ(ulInvalidDoorbellBitmap);

        /* 2. Updates Error Log and generates an asynchronous event to report the error. */
        L0_NVMeNonCmdError(ASYNC_EVENT_ERRORINFO_INVLDDBL_WRT);

        /* 3. Clears the doorbell value which was incorrectly written. */
        g_pNVMeCfgReg->doorbell[ulInvalidSQId].sq_tail = 0;

        /* 4. Clears the corresponding bit in invalid SQ bitmap. */
        ulInvalidDoorbellBitmap &= ~(BIT(ulInvalidSQId));
    }

    return;
}

void L0_NVMeUpdateCrtclWrng(U32 ulRepAsyncEvent)
{
#ifndef L1_FAKE
    U32 ulPECycleCount;
    U32 ulRsvdBlockCount;
#endif
    U32 ulLifePercentUsed;
    U32 ulRemainSparePercent;
    U32 ulSpareThreshold;
    U32 ulCompositeTemperature;

    PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY  pSMARTLog =
        (PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY)(g_ulATAGPLBuffStart + MCU0_LOG_SMART_OFFSET);
    PDEVICE_PARAM_PAGE pDeviceInfoPage = (PDEVICE_PARAM_PAGE)g_ulDevParamAddr;

    /* 1. Acquires parameters for calculating from Device Param page. */
    ulCompositeTemperature = pDeviceInfoPage->SYSTemperature + 273u;
    ulSpareThreshold = pSMARTLog->AvailableSpareThreshold;
#ifdef L1_FAKE
    ulRemainSparePercent = 100u;
    ulLifePercentUsed = 0u;
#else
    ulPECycleCount = g_pBootParamTable->ulFlashPECycleVal;
    ulRsvdBlockCount = pDeviceInfoPage->AvailRsvdSpace + pDeviceInfoPage->UsedRsvdBlockCnt;

    if (0 != ulPECycleCount)
    {
        ulLifePercentUsed = ((pDeviceInfoPage->AvgEraseCount / TLC_BLK_CNT) * 100u) / ulPECycleCount;
    }
    else
    {
       ulLifePercentUsed = 0u;
    }

    if (0 != ulRsvdBlockCount)
    {
        ulRemainSparePercent = ((pDeviceInfoPage->AvailRsvdSpace) * 100u) / ulRsvdBlockCount;
    }

    else
    {
        ulRemainSparePercent = 100u;
    }
#endif

    /* 2. Calculates critical warning conditions. */
    if ((ulCompositeTemperature > g_tNVMeTempThrldTable.aSensorEntryArray[0].ulOverTempThrshld) ||
        (ulCompositeTemperature < g_tNVMeTempThrldTable.aSensorEntryArray[0].ulUnderTempThrshld))
    {
        pSMARTLog->CriticalWarning.TemperatureExceededCriticalThreshold = TRUE;

        if (FALSE != ulRepAsyncEvent)
        {
            L0_NVMeGenerateAsyncEvent(ASYNC_EVENT_TYPE_SMART, ASYNC_EVENT_SMARTINFO_TEMP_THRSHD);
        }
    }

    else
    {
        pSMARTLog->CriticalWarning.TemperatureExceededCriticalThreshold = FALSE;
    }

    if (ulRemainSparePercent < ulSpareThreshold)
    {
        pSMARTLog->CriticalWarning.AvailableSpareSpaceBelowThreshold = TRUE;

        if (FALSE != ulRepAsyncEvent)
        {
            L0_NVMeGenerateAsyncEvent(ASYNC_EVENT_TYPE_SMART, ASYNC_EVENT_SMARTINFO_SPARE_THRSHD);
        }
    }

    else
    {
        pSMARTLog->CriticalWarning.AvailableSpareSpaceBelowThreshold = FALSE;
    }

    if (ulLifePercentUsed > NVME_LIFEPERCENTUSED_THRESHOLD)
    {
        pSMARTLog->CriticalWarning.DeviceReliablityDegraded = TRUE;

        if (FALSE != ulRepAsyncEvent)
        {
            L0_NVMeGenerateAsyncEvent(ASYNC_EVENT_TYPE_SMART, ASYNC_EVENT_SMARTINFO_NVM_RLBLTY);
        }
    }

    return;
}

#if 0
MCU0_DRAM_TEXT U32 L0_NVMeProcessAsycEvent(void)
{
    U8  Type;
    U8  Info;
    U8  Id;
    U16  RptrBak;
    U32  ulSlotNum;
    volatile  PNVME_WBQ pWBQ;
    volatile PNVME_WBQ pTargetWBQEntry;
    ASYN_EVENT_MGR* pMgr;
    PCB_MGR pSlot;

    Id = 0;
    Info = 0;
    Type = 0;
    pMgr = &gNvmeMgr.AsynEvtMgr;
    RptrBak = gNvmeMgr.AsynEvtMgr.Rptr;
    pWBQ = &pMgr->Queue[gNvmeMgr.AsynEvtMgr.Rptr++];

    if(gNvmeMgr.AsynEvtMgr.Rptr == MAX_ASYN_EVEVNT_NUM)
    {
        gNvmeMgr.AsynEvtMgr.Rptr = 0;
    }

    if(gNvmeMgr.AsynEvtMgr.Rptr == gNvmeMgr.AsynEvtMgr.Wptr)
    {
        gNvmeMgr.AsynEvtMgr.Rptr = RptrBak;
        return FALSE;
    }

    ulSlotNum = HAL_HCTSearchCST(SLOT_IDLE);
    if(ulSlotNum == INVALID_CMD_ID)
    {
        gNvmeMgr.AsynEvtMgr.Rptr = RptrBak;
        return FALSE;
    }

    pSlot = &gNvmeMgr.CbMgr[ulSlotNum];
    pSlot->SQID = 0;
    if(pMgr->SpareSpaceSend)
    {
        Type = 1;
        Info = 2;
        Id = LID_SMART;
        pMgr->SpareSpaceSend = 0;
        DBG_Printf("asyn event send:spare space send\n");
    }
    else if(pMgr->TemperatureSend)
    {
        Type = 1;
        Info = 1;
        Id = LID_SMART;
        pMgr->TemperatureSend = 0;
        DBG_Printf("asyn event send:temperature send\n");
    }

    else if(pMgr->NvmReliaSend || pMgr->ReadOnlySend ||pMgr->BackupDevSend)
    {
        Type = 1;
        Info = 0;
        Id = LID_SMART;
    }

    else if(pMgr->NANSend)
    {
        Type = 2;
        Info = 0;
        Id = LID_NAMESPACE_CHANGE;
    }
    else if(pMgr->FANSend)
    {
        Type = 2;
        Info = 1;
        Id = LID_FW_SLOT_INFO;
    }
    else if(pMgr->ErrorStsSend)
    {
        Type = 0;
        Info = 0;
        Id = LID_ERROR;
    }
    else
    {
        //return 0;
        DBG_Printf("Construct asyc event error\n");
        DBG_Getch();
    }

    pWBQ->CmdSpec= (Id<<16)|(Info<<8)|Type;

    pTargetWBQEntry = (PNVME_WBQ)HAL_HCTGetWBQEntry(ulSlotNum, 0);
    *pTargetWBQEntry = *pWBQ;
    pTargetWBQEntry->Id = ulSlotNum;

    HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
    pSlot->CmdState = CMD_COMPLETED;

    gNvmeMgr.AsynEvtMgr.OutStandingCnt--;
    
    return TRUE;
}
#else
MCU0_DRAM_TEXT void L0_NVMeDisableAsyncEventReporting(void)
{
    gNvmeMgr.AsyncEvtCtrl.ulEventPendingStatus = FALSE;
    gNvmeMgr.AsyncEvtCtrl.ulOutstdReqCount = 0;

    return;
}

MCU0_DRAM_TEXT void L0_NVMeClearAsyncEventResvSlots(void)
{
    U32 ulSlotNum;

    while (TRUE)
    {
        ulSlotNum = HAL_HCTSearchCST(SLOT_ASYNC_EVT_RESV);

        if (INVALID_CMD_ID == ulSlotNum)
        {
            break;
        }

        else
        {
            HAL_HCTSetCST(ulSlotNum, SLOT_IDLE);
        }
    }

    return;
}

MCU0_DRAM_TEXT U32 L0_NVMeGetAsyncEventLogPageId(U32 ulType, U32 ulSubtype)
{
    U32 ulLogPageId;

    switch (ulType)
    {
        case ASYNC_EVENT_TYPE_ERROR:
            ulLogPageId = LID_ERROR;
            break;

        case ASYNC_EVENT_TYPE_SMART:
            ulLogPageId = LID_SMART;
            break;

        case ASYNC_EVENT_TYPE_NOTICE:
            switch (ulSubtype)
            {
                case ASYNC_EVENT_NOTICEINFO_NMSP_ATTR:
                    ulLogPageId = LID_NAMESPACE_CHANGE;
                    break;

                case ASYNC_EVENT_NOTICEINFO_FW_ACTVTN:
                    ulLogPageId = LID_FW_SLOT_INFO;
                    break;

                case ASYNC_EVENT_NOTICEINFO_TLMTRY_LOG:
                    ulLogPageId = LID_TELEMETRY_CONTROLLER;
                    break;

                default:
                    ulLogPageId = LID_RESERVED;
                    break;
            }

            break;

        case ASYNC_EVENT_TYPE_IOCMD_SPECIFIC:
            switch (ulSubtype)
            {
                case ASYNC_EVENT_IOCMDINFO_RSVTN_LOG:
                    ulLogPageId = LID_RESVTN_NOTIFICATION;
                    break;

                case ASYNC_EVENT_IOCMDINFO_SANTZ_CMPL:
                    ulLogPageId = LID_SANITIZE_STATUS;
                    break;

                default:
                    ulLogPageId = LID_RESERVED;
                    break;
            }

            break;

        default:
            ulLogPageId = LID_RESERVED;
            break;
    }

    return ulLogPageId;
}

MCU0_DRAM_TEXT void L0_NVMeClearAsyncEvent(U32 ulLogPageID)
{
    /* 1. Clears the mask of corresponding Async Event type. */
    switch (ulLogPageID)
    {
        case LID_ERROR:
            gNvmeMgr.AsyncEvtCtrl.ulEventTypeMask &= ~(1 << ASYNC_EVENT_TYPE_ERROR);
            break;

        case LID_SMART:
            gNvmeMgr.AsyncEvtCtrl.ulEventTypeMask &= ~(1 << ASYNC_EVENT_TYPE_SMART);
            break;

        default:
            if (ulLogPageID == gNvmeMgr.AsyncEvtCtrl.ulNoticeTypeUnmaskLogId)
            {
                gNvmeMgr.AsyncEvtCtrl.ulEventTypeMask &= ~(1 << ASYNC_EVENT_TYPE_NOTICE);
            }

            else if (ulLogPageID == gNvmeMgr.AsyncEvtCtrl.ulIOSpecificTypeUnmaskLogId)
            {
                gNvmeMgr.AsyncEvtCtrl.ulEventTypeMask &= ~(1 << ASYNC_EVENT_TYPE_IOCMD_SPECIFIC);
            }
            break;
    }

    /* 2. Clears the pending Async Event waiting to be reported. */
    if ((TRUE == gNvmeMgr.AsyncEvtCtrl.ulEventPendingStatus) &&
        (ulLogPageID == gNvmeMgr.AsyncEvtCtrl.tPendingEvent.bsLogPageId))
    {
        gNvmeMgr.AsyncEvtCtrl.ulEventPendingStatus = FALSE;
    }

    return;
}

MCU0_DRAM_TEXT U32 L0_NVMeIsAsyncEventEnabled(U32 ulType, U32 ulSubtype)
{
    U32 ulEnabled;

    /* 1. Checks if corresponding event type has been masked. */
    if (0 != (gNvmeMgr.AsyncEvtCtrl.ulEventTypeMask & (1 << ulType)))
    {
        ulEnabled = FALSE;
    }

    /* 2. Checks if host has enabled some types of Async Events in a Set Features command. */
    else
    {
        if (ASYNC_EVENT_TYPE_ERROR == ulType)
        {
            ulEnabled = TRUE;
        }

        else if (ASYNC_EVENT_TYPE_NOTICE == ulType)
        {
            switch (ulSubtype)
            {
                case ASYNC_EVENT_NOTICEINFO_NMSP_ATTR:
                    ulEnabled = gNvmeMgr.AsyncEvtCtrl.tHostConfig.bsNmspAttrNtcs;
                    break;

                case ASYNC_EVENT_NOTICEINFO_FW_ACTVTN:
                    ulEnabled = gNvmeMgr.AsyncEvtCtrl.tHostConfig.bsFwActvnNtcs;
                    break;

                case ASYNC_EVENT_NOTICEINFO_TLMTRY_LOG:
                    ulEnabled = gNvmeMgr.AsyncEvtCtrl.tHostConfig.bsTlmLogNtcs;
                    break;

                default:
                    ulEnabled = FALSE;
            }
        }

        else if (ASYNC_EVENT_TYPE_SMART == ulType)
        {
            switch (ulSubtype)
            {
                case ASYNC_EVENT_SMARTINFO_NVM_RLBLTY:
                    ulEnabled = gNvmeMgr.AsyncEvtCtrl.tHostConfig.bsSMARTRlblWrng;
                    break;

                case ASYNC_EVENT_SMARTINFO_TEMP_THRSHD:
                    ulEnabled = gNvmeMgr.AsyncEvtCtrl.tHostConfig.bsSMARTTmptWrng;
                    break;

                case ASYNC_EVENT_SMARTINFO_SPARE_THRSHD:
                    ulEnabled = gNvmeMgr.AsyncEvtCtrl.tHostConfig.bsSMARTSpareSpcWrng;
                    break;

                default:
                    ulEnabled = FALSE;
            }
        }

        else
        {
            /* Unsupported Async Event type. */
            ulEnabled = FALSE;
        }
    }

    return ulEnabled;
}

MCU0_DRAM_TEXT void L0_NVMeReportAsyncEvent(PASYNC_EVENT_RESPONSE pResp)
{
    U32 ulSlotNum;
    PCB_MGR pAsyncEventSlot;

    /* 1. Finds a command slot reserved for Async Event. */
    ulSlotNum = HAL_HCTSearchCST(SLOT_ASYNC_EVT_RESV);
    if (INVALID_CMD_ID == ulSlotNum)
    {
        DBG_Printf("No reserved Async Event slot found!\n");
        DBG_Getch();
    }

    /* 2. Sets the command specific response. */
    pAsyncEventSlot = GET_SLOT_MGR(ulSlotNum);
    pAsyncEventSlot->CSPC = pResp->ulCQDW0;

    /* 3. Posts the CQ entry. */
    L0_NVMeSetupSimpleCompletion(pAsyncEventSlot, FALSE, FALSE);
    HAL_HCTSetCST(ulSlotNum, SLOT_TRIGGER_WBQ);
    pAsyncEventSlot->CmdState = CMD_COMPLETED;

    /* 4. Records corresponding Log Page identifier of last Notice or IO Specific type for unmask. */
    if (ASYNC_EVENT_TYPE_NOTICE == pResp->bsAsyncEventType)
    {
        gNvmeMgr.AsyncEvtCtrl.ulNoticeTypeUnmaskLogId = pResp->bsLogPageId;
    }

    else if (ASYNC_EVENT_TYPE_IOCMD_SPECIFIC == pResp->bsAsyncEventType)
    {
        gNvmeMgr.AsyncEvtCtrl.ulIOSpecificTypeUnmaskLogId = pResp->bsLogPageId;
    }

    DBG_Printf("NVME ASYN EVENT Request slot %d reported: Type %d, Subtype %d.\n", ulSlotNum, pResp->bsAsyncEventType, pResp->bsAsyncEventInfo);
    return;
}

MCU0_DRAM_TEXT void L0_NVMeStashAsyncEvent(PASYNC_EVENT_RESPONSE pResp)
{
    gNvmeMgr.AsyncEvtCtrl.tPendingEvent.ulCQDW0 = pResp->ulCQDW0;
    gNvmeMgr.AsyncEvtCtrl.ulEventPendingStatus = TRUE;

    DBG_Printf("NVME ASYN EVENT stashed: Type %d, Subtype %d.\n", pResp->bsAsyncEventType, pResp->bsAsyncEventInfo);
    return;
}

MCU0_DRAM_TEXT void L0_NVMeApplyStashedAsyncEvent(PCB_MGR pSlot)
{
    ASYNC_EVENT_RESPONSE tPendingResp;

    tPendingResp.ulCQDW0 = gNvmeMgr.AsyncEvtCtrl.tPendingEvent.ulCQDW0;

    /* 1. Acquires stashed response to complete the Async Event request. */
    pSlot->CSPC = tPendingResp.ulCQDW0;

    /* 2. Posts the CQ entry. */
    L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
    HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
    pSlot->CmdState = CMD_COMPLETED;

    /* 3. Records corresponding Log Page identifier of last Notice or IO Specific type for unmask. */
    if (ASYNC_EVENT_TYPE_NOTICE == tPendingResp.bsAsyncEventType)
    {
        gNvmeMgr.AsyncEvtCtrl.ulNoticeTypeUnmaskLogId = tPendingResp.bsLogPageId;
    }

    else if (ASYNC_EVENT_TYPE_IOCMD_SPECIFIC == tPendingResp.bsAsyncEventType)
    {
        gNvmeMgr.AsyncEvtCtrl.ulIOSpecificTypeUnmaskLogId = tPendingResp.bsLogPageId;
    }

    /* 4. Clears pending status. */
    gNvmeMgr.AsyncEvtCtrl.ulEventPendingStatus = FALSE;

    return;
}

MCU0_DRAM_TEXT void L0_NVMeGenerateAsyncEvent(U32 ulType, U32 ulSubtype)
{
    U32 ulLogPageId;
    ASYNC_EVENT_RESPONSE tResp;

    /* 1. Checks whether we can report the corresponding type and subtype of Async Event. */
    if (FALSE != L0_NVMeIsAsyncEventEnabled(ulType, ulSubtype))
    {
        /* 2. Finds corresponding Log Page identifier for current event type. */
        ulLogPageId = L0_NVMeGetAsyncEventLogPageId(ulType, ulSubtype);

        if (LID_RESERVED != ulLogPageId)
        {
            /* 3. Calculates the response DWord 0. */
            tResp.bsAsyncEventType = ulType;
            tResp.bsAsyncEventInfo = ulSubtype;
            tResp.bsLogPageId = ulLogPageId;

            /* 4. Checks whether there is an outstanding request existing. */
            if (0 == gNvmeMgr.AsyncEvtCtrl.ulOutstdReqCount)
            {
                /* Since there is no outstanding request left, we should stash current async event. */
                L0_NVMeStashAsyncEvent(&tResp);
            }

            else
            {
                /* Posts CQ entry for the async event immediately. */
                L0_NVMeReportAsyncEvent(&tResp);

                /* Updates the consumation of outstanding Async Event requests. */
                gNvmeMgr.AsyncEvtCtrl.ulOutstdReqCount--;
            }

            /* 5. Masks corresponding event type from being generated again. */
            gNvmeMgr.AsyncEvtCtrl.ulEventTypeMask |= (1 << ulType);
        }
    }

    return;
}
#endif

MCU0_DRAM_TEXT void L0_InitLogData(void)
{
    U32 Addr =
        g_ulATAGPLBuffStart + MCU0_LOG_ERROR_OFFSET;

    COM_MemZero((void *)Addr, (LOG_ERROR_LEN + LOG_SMART_LEN + LOG_FW_SLOT_INFO_LEN) >> DWORD_SIZE_BITS);

    return;
}

U32 L0_NVMeMemAlloc(U32 ulStartAddr)
{
    U32 ulCurrAllocPos = ulStartAddr;

    /* 1. The buffer space allocated for General Purpose Log pages. One whole buffer block in DDR SDRAM. */
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos, 
        BUF_SIZE,
        &g_ulATAGPLBuffStart,
        BUF_SIZE_BITS);

    /* 2. Calculating the starting address of IDENTIFY DEVICE data, which belongs to General Purpose Log. */
    g_ulInfoIdfyPage = g_ulATAGPLBuffStart;

    /* 3. Allocating receiving buffer space for NVMe command data structures. */
    (void)L0_RAMAlloc(HCT_S0_BASE,
        (sizeof(COMMAND_HEADER) * MAX_SLOT_NUM),
        (U32 *)(&g_pLocCmdHeader),
        0);

    (void)L0_RAMAlloc(HCT_S1_BASE,
        (sizeof(COMMAND_TABLE) * MAX_SLOT_NUM),
        (U32 *)(&g_pLocCmdTable),
        0);

    return ulCurrAllocPos;
}

void L0_RegScanHelper(L0_TASK_MANAGER *pL0TaskMgr)
{
    U32 ulCurrCmdIdle;
    extern U32 L0_EventGetMap(void);

    /* Only checking command idle status for NVMe. */
    if ((0 != L0_EventGetMap()) || (FALSE == CC_EN()) || (FALSE == L0_IsMsgQueueEmpty()))
    {
        ulCurrCmdIdle = FALSE;
    }
    /* when uart interface used, FW can't entry PM_PRED_SLEEP state
       once MCU1 & MCU2 are halted, UART command will fail. */
    else if((FALSE != g_PmCtrMpMode) || (FALSE != g_ulUARTCmdPending))
    {
        ulCurrCmdIdle = FALSE;
    }
    else
    {
        ulCurrCmdIdle = L0_NVMeCheckHostIdle();
    }

    if (FALSE != pL0TaskMgr->PrevPortIdle)
    {
        if (FALSE != ulCurrCmdIdle)
        {
            pL0TaskMgr->IdleLoopCount++;
        }

        else
        {
            pL0TaskMgr->IdleLoopCount = 0;
        }
    }

    pL0TaskMgr->PrevPortIdle = ulCurrCmdIdle;

    return;
}


void L0_NVMeUeccPuBitMapSet(PCB_MGR pCbMgr, U32 ulPuId, U32 ulValue)
{
    U32 ulPuBitIndex;
    U32 ulPuBitOffset;

    ulPuBitIndex = ulPuId / 32;
    ulPuBitOffset = ulPuId % 32;
    if((ulPuId >= SUBSYSTEM_LUN_MAX) || (ulPuBitIndex >= MAX_UECC_PUBMAP_DWSIZE))
    {
        DBG_Getch();
    }

    if (TRUE == ulValue)
    {
        pCbMgr->UeccPUBitMap[ulPuBitIndex] |= (1 << ulPuBitOffset);
    }else if (FALSE == ulValue)
    {
        pCbMgr->UeccPUBitMap[ulPuBitIndex] &= ~(1 << ulPuBitOffset);
    }else
    {
        DBG_Getch();
    }
        
    return;
}

BOOL L0_NVMeUeccPuBitMapGet(PCB_MGR pCbMgr, U32 ulPuId)
{
    U32 ulPuBitIndex;
    U32 ulPuBitOffset;

    ulPuBitIndex = ulPuId / 32;
    ulPuBitOffset = ulPuId % 32;
    if((ulPuId >= SUBSYSTEM_LUN_MAX) || (ulPuBitIndex >= MAX_UECC_PUBMAP_DWSIZE))
    {
        DBG_Getch();
    }
    
    return ((pCbMgr->UeccPUBitMap[ulPuBitIndex] & (1 << ulPuBitOffset)) ? TRUE:FALSE);
}


BOOL L0_NVMeUeccPuBitMapIsDirty(PCB_MGR pCbMgr)
{
    U32 i;
    BOOL bResult = FALSE;

    for ( i = 0; i< MAX_UECC_PUBMAP_DWSIZE; i++)
    {
        if (0 != pCbMgr->UeccPUBitMap[i])
        {
            bResult = TRUE;
            return bResult;
        }
    }
    return bResult;
}


void L0_NVMeUeccPuBitMapClear(PCB_MGR pCbMgr)
{
    U32 i;

    for( i = 0; i < MAX_UECC_PUBMAP_DWSIZE; i++)
    {
        pCbMgr->UeccPUBitMap[i] = 0;
    }
    return;
}

void L0_NVMeRecycleCbMgr(PCB_MGR CbMgr)
{
    U32 ulSlotNum;
    PCOMMAND_HEADER pBindSQEntry;
    PPRP pPRPTable;

    ulSlotNum = CbMgr->SlotNum;
    pBindSQEntry = CbMgr->Ch;
    pPRPTable = CbMgr->PRPTable;
    COM_MemZero((U32 *)CbMgr, (sizeof(CB_MGR) >> DWORD_SIZE_BITS));
    CbMgr->SlotNum = ulSlotNum;
    CbMgr->Ch = pBindSQEntry;
    CbMgr->PRPTable = pPRPTable;

#ifdef SIM
#ifndef AF_ENABLE //In 3533, HW should init S0(entry)
    COM_MemZero((U32 *)CbMgr->Ch, (sizeof(COMMAND_HEADER) >> DWORD_SIZE_BITS));
#endif //AF
#endif //SIM

#ifndef AF_ENABLE //In 3533, HW should init S0(entry)
    CbMgr->Ch->DW15 = 0xFFFFFFFF;
#endif
}


/*==============================================================================
Func Name  : L0_NVMeHCmdTOCallback
Input      : U8 ucHID: slot number.
Output     : NONE
Return Val : 
Discription: if host command process time out, hw will send a interrupt to MCU0,
             this function is the call back function, when TO occur, fw's behavior
Usage      : 
History    : 
    1. 2014.12.23 Haven Yang create function
==============================================================================*/
void L0_NVMeHCmdTOCallback(U8 ucHID)
{

#ifndef SIM
    if(bDPLLGating == FALSE)
#endif
    {
       DBG_Printf("Slot(%d) command time out(%d us)\n", ucHID, HAL_HCmdTimerGetCurTime(ucHID));
    }
       
    HAL_HCmdTimerStart(ucHID); //patch hw bug, start again
    HAL_HCmdTimerStop(ucHID);
    //DBG_Getch();
}

#define HIGH_QD_TH 1
#define LOW_QD_LOOP 1000
BOOL L0_NVMeCheckHostIdle(void)
{
    U32 ulSlot;
    U32 ulSlotCST;
    BOOL bHighQD = TRUE;
    static BOOL bPreHighQD = TRUE;
    static U32 ulLowQDCnt = 0;
    static BOOL bPrePrefetchEvent = FALSE;
    BOOL bCurHighQD = FALSE;
    BOOL bHostIdle = TRUE;
    U32 ulQDCnt = 0;
    COMM_EVENT_PARAMETER * pParameter;


    /* 0. Checks whether host enabled expansion ROM. */
    if (0 != (*(volatile U32*)(REG_BASE_PCIE_NVME + 0x30) & 0x1))
    {
        return FALSE;
    }

    /* 1. check SQ entry */
    if (HAVE_NVME_CMD())
    {
        return FALSE;
    }

    /* 2. check CST */
    for (ulSlot = 0; ulSlot < MAX_SLOT_NUM; ulSlot++)
    {
        ulSlotCST = rHCT_CS_REG[ulSlot];

        if((SLOT_IDLE != ulSlotCST)
            && (SLOT_ASYNC_EVT_RESV != ulSlotCST))
        {
            bHostIdle = FALSE;

            if (ulQDCnt >= HIGH_QD_TH)
            {
                bCurHighQD = TRUE;
                break;
            }
            else
                ulQDCnt++;
        }
    }

    if (bPreHighQD == TRUE && bCurHighQD == TRUE)
    {
        ulLowQDCnt = 0;
    }
    else if (bPreHighQD == FALSE && bCurHighQD == FALSE)
    {
        if (ulLowQDCnt > LOW_QD_LOOP)
        {
            if (bPrePrefetchEvent == FALSE)
            {
                //Reach Low DQ condition, enable read prefetch
                CommGetEventParameter(COMM_EVENT_OWNER_L1, &pParameter);
                pParameter->EventStatus = 1;
                CommSetEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_LOWQD);
                bPrePrefetchEvent = TRUE;
                //DBG_Printf("Enable\n");
            }
        }
        else
            ulLowQDCnt++;
    }
    else if (bPreHighQD == TRUE && bCurHighQD == FALSE)
    {
        ulLowQDCnt = 0;
        bPreHighQD = FALSE;
    }
    else if (bPreHighQD == FALSE && bCurHighQD == TRUE)
    {
        if (bPrePrefetchEvent == TRUE)
        {
            ulLowQDCnt = 0;
            bPreHighQD = TRUE;
            //disable read prefetch
            CommGetEventParameter(COMM_EVENT_OWNER_L1, &pParameter);
            pParameter->EventStatus = 0;
            CommSetEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_LOWQD);
            bPrePrefetchEvent = FALSE;
            //DBG_Printf("Disable\n");
        }
    }

    return bHostIdle;
}

#ifndef AF_ENABLE
#if 0
BOOL L0_NVMeCheckSQIdle(usSQID)
{
    U32 ulSlot;
    
    /* 1. check SQ entry */
    if (HAVE_NVME_IO_CMD(usSQID))
    {
        return FALSE;
    }

    /* 2. check CST */
    for (ulSlot = 0; ulSlot < MAX_SLOT_NUM; ulSlot++)
    {
        if(SLOT_IDLE != *(volatile U8*)HCT_CS_REG_ADDRESS(ulSlot))
        {
            if (SQID_IN_SLOT(ulSlot) == usSQID)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}
#endif
#endif

BOOL L0_NVMeCheckPCIEResetIdle(void)
{
    /* 1. check SQ entry */
    if (HAVE_NVME_CMD())
    {
        return FALSE;
    }

    /* 2. check SGE idle */
    if (0x3 != ((rModeClkEn >> 16) & 0x3))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL L0_NVMeShutdownCompletion(void *pParam)
{
    SET_CSTS_SHST(SHN_PROCESS_COMPLETE);
    STOP_NVME();
    DBG_Printf("firmware shutdown successful !\n");
    return TRUE;
}

void L0_NVMeEventRegister(void)
{
    /* Accelerates boot flow only on ASIC. */
    L0_EventRegHander(L0_EVENT_TYPE_SUBSYS_BOOT, L0_TaskWaitBoot, L0_NVMeTaskBootDone);
#ifndef SIM
    L0_EventRegHander(L0_EVENT_TYPE_PCIE_LINKFAILURE, L0_NVMeLinkFailure, NULL);
    L0_EventRegHander(L0_EVENT_TYPE_PCIE_FATALERROR, L0_NVMeLinkFatalError, NULL);
#endif
    L0_EventRegHander(L0_EVENT_TYPE_CCEN, L0_NVMeCcEn, NULL);
    L0_EventRegHander(L0_EVENT_TYPE_SHUTDOWN, L0_TaskShutDown, L0_NVMeShutdownCompletion);
    L0_EventRegHander(L0_EVENT_TYPE_XFER_RESET, L0_NVMeProcResetEvent, NULL);
}


BOOL FwCrcChk(U32 ulFwBaseAddr)
{
    U32 ulCrcCalc = 0;
    U32 *pFwDW;
    U32 ulFwDWCnt;

    pFwDW = (U32*)ulFwBaseAddr;
    ulFwDWCnt = (U32)FW_IMAGE_MEM_SIZE >> 2;    //FW_IMAGE_MEM_SIZE is in byte, Calc in DW

    do 
    {
        ulCrcCalc ^= *pFwDW;
        pFwDW++;
    } while (--ulFwDWCnt);
    
    return (0 == ulCrcCalc) ? TRUE : FALSE;
}


void L0_NVMeForceCompleteCmd(PCB_MGR pSlot, U32 ulStatus)
{
    L0_NVMeCmdError(pSlot->SlotNum, ulStatus);
    L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
    HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
    pSlot->CmdState = CMD_COMPLETED;
}

MCU0_DRAM_TEXT void L0_NVMePhySetting(void)
{
    PTABLE *pPTable = HAL_GetPTableAddr();
    U32 ulEQreject;

    /*TempFix:Set SoftReset ISR before access in 3533 */
    /* rGBG_18[29]: PCIE software reset */
    rGlbSoftRst &= ~R_RST_PCIE;

    //PCIe IP setting by curry 0827 start
    //1-ACK timer
    HAL_MCUInsertbits(rPCIe(0x1B8), 0x0E, 24, 31);

    HAL_MCUInsertbits(rPCIe(0x1BC), 0x15, 16, 23);

    //2-IP priority
    HAL_MCUInsertbits(rPCIe(0x1B4), 0x7, 0, 2);

    //3-NACK flag
    //533 default is 1, no need to set
    //HAL_MCUInsertbits(rPCIe(0x1B4), 0x1, 23, 23);

    //HAL_MCUClearbits(rPCIe(0x1A0), 6, 6); //DL_DOWN reset PCIE traffic

    //HAL_MCUClearbits(rPCIe(0x208), 10, 11); //DL_DOWN reset PCIE IRS

    //rPCIe(0x448) = 0x001F0000;

    HAL_MCUInsertbits(rPCIe(0x1A4), 0x1, 28, 28);

    //P1EXT
    HAL_MCUInsertbits(rPCIe(0xD0), 0x7, 15, 17);

    //P0sP2EXT
    HAL_MCUInsertbits(rPCIe(0xD0), 0x7, 12, 14);

    /* Patch for EPHY EC and L1 exit latency register. */
    HAL_MCUInsertbits(rPCIe(0x284), 0x1, 0, 0);

    // P1EXT for C2
    //533  no need to set
    //HAL_MCUInsertbits(rPCIe(0x28C), 0x7, 0, 2);

    // Sets the Endpoint L1 Acceptable Latency to "no limit"
    //533 default 7, no need to set
    //HAL_MCUInsertbits(rPCIe(0xC8), 0x7, 9, 11);

    //Set Max Payload Size Supported to 000b : 128 bytes
    //HAL_MCUClearbits(rPCIe(0xC8), 0, 2);

    // Patch for C2 INTA mode
    HAL_MCUClearbits(rPCIe(0xE8), 18, 19);
    HAL_MCUClearbits(rPCIe(0xEC), 13, 14);

#if 0
    //FORCE GEN1 for L1.2 test platform
    HAL_MCUInsertbits(rPCIe(0xD0), 0x1, 0, 3);
    HAL_MCUInsertbits(rPCIe(0xF0), 0x1, 1, 3);
#endif

    if(TRUE == pPTable->tNVMeL0Feature.tNVMEL0Feat.bsNVMeL0ForceGen2)
    {
        //4(Optional)-Force PCIe GEN2 
        HAL_MCUInsertbits(rPCIe(0xD0), 0x2, 0, 3);

        HAL_MCUInsertbits(rPCIe(0xF0), 0x3, 1, 3);
    }

    HAL_MCUClearbits(rPCIe(0x1A4), 28, 28);

    // patch for GEN3 EQ
    HAL_MCUInsertbits(rPCIe(0x288), 0x1, 1, 1);
    for(ulEQreject = 0; ulEQreject < 84; ulEQreject++)
    {
        HAL_MCUClearbits(rPCIe(0x2A0 + (ulEQreject << 2)), 15, 15);
    }

    // enabling hardware patch for submission queue shift bug
    HAL_MCUInsertbits(rPCIe(0x1B8), 0x1, 1, 1);

    // enabling hardware ASPM timer if firmware ratio control is disabled
#ifndef PCIE_ASPM_MANAGEMENT
    rPCIeByte(0x1D3) = 0x50;
#endif

    //re-setting PCIE clk gating
    /* Patch from Yao: */
    /* Disables all clock gating in PCIE for ASPM L1 recovery issue. */
#if 0
    rPCIe(0x1A4) &= ~(0x3FF<<0);
    rPCIe(0x1A4) |= (0x3FF<<0);

    rPCIe(0x1B0) &= ~(0x1<<16);
    rPCIe(0x1B0) |= (0x1<<16);

    rPCIe(0x1C0) &= ~(0x1<<15);
    rPCIe(0x1C0) |= (0x1<<15);

    rPCIe(0x1E8) &= ~(0x1<<0);
    rPCIe(0x1E8) |= (0x1<<0);

    rPCIe(0x208) &= ~(0x1f<<25);
    rPCIe(0x208) |= (0x1f<<25);

    rPCIe(0x208) &= ~(0x1f<<18);
    rPCIe(0x208) |= (0x1f<<18);

    rPCIe(0x208) &= ~(0x3<<8);
    rPCIe(0x208) |= (0x3<<8);
#endif

    return;
}

/*
L0 Msg Queue is introduced for stroing non-immediated task or message 
such as a message from MCU1/2. Currently, this MsgQueue is only used for handling
UECC message from L3.
*/
/*===========================================================================
Func Name   :L0_InitMsgQueue
Input       :NULL
Output      :NULL
Return Value:TRUE - Indicated Init L0 MsgQueue successfully
             FAIL - fail to init L0 MsgQueue.This can be occured when get L0MSGLock 
             fail or the Pointer of MsgQueue is NULL
Description :Init L0 MsgQueue
History     :
    1. 2015.11.5 JankaLun create function
==============================================================================*/
MCU0_DRAM_TEXT BOOL L0_InitMsgQueue(void)
{
    l_pL0MsgQueue = &l_L0MsgQueue;

    if ((l_pL0MsgQueue == NULL) || (l_pMsgNode != NULL))
    {
        DBG_Printf("Init MsgQueue fail, l_L0MsgQueue is NULL or l_pMsgNode != NULL");
        DBG_Getch();
        //return FAIL;
    }

    COM_MemZero((U32*)l_pL0MsgQueue, sizeof(L0MSGNODE)/sizeof(U32));

    l_pL0MsgQueue->ulMaxSize = MAX_L0MSG_QDEPTH;

    return TRUE;
}

#define QUEUE_EMPTY(HEAD, TAIL)             ((HEAD) == (TAIL))
#define QUEUE_NOT_EMPTY(HEAD, TAIL)         (!QUEUE_EMPTY((HEAD), (TAIL)))
#define QUEUE_FULL(HEAD, TAIL, SIZE)        ((HEAD) == ((TAIL + 1) % (SIZE)))
#define QUEUE_NOT_FULL(HEAD, TAIL, SIZE)    (!(QUEUE_FULL(HEAD, TAIL, SIZE)))
#define QUEUE_PUSH_POINTER(POINTER, SIZE)   ((POINTER) = ((POINTER) + 1) % (SIZE))

/*===========================================================================
Func Name   :L0_IsMsgQueueEmpty
Input       :NULL
Output      :NULL
Return Value:TRUE - Queue is empty.
             FAIL - Queue is not empty
Description :
History     :
==============================================================================*/
BOOL L0_IsMsgQueueEmpty(void)
{
    return QUEUE_EMPTY(l_pL0MsgQueue->ulHead, l_pL0MsgQueue->ulTail) ? TRUE : FAIL;
}


/*===========================================================================
Func Name   :L0_IsMsgQueueFull
Input       :NULL
Output      :NULL
Description :
History     :
==============================================================================*/
MCU0_DRAM_TEXT BOOL L0_IsMsgQueueFull(void)
{
    return QUEUE_FULL(l_pL0MsgQueue->ulHead, l_pL0MsgQueue->ulTail,
        l_pL0MsgQueue->ulMaxSize) ? TRUE : FAIL;
}


MCU0_DRAM_TEXT U32 L0_GetMsgQueueNodeNum(void)
{
    return l_pL0MsgQueue->ulNodeNum;
}



/*===========================================================================
Func Name   :L0_GetMsgQueueNode
Input       :NULL
Output      :NULL
Return Value:NULL - Queue is empty
            :Pointer of the Msg Node from header
Description :This function only return the pointer of the Msg Node from header
             without moving head pointer.
History     :
==============================================================================*/
MCU0_DRAM_TEXT PL0MSGNODE L0_GetMsgQueueNode(void)
{
    if (TRUE == L0_IsMsgQueueEmpty())
    {
        return NULL;
    }

    return &(l_pL0MsgQueue->MsgNode[l_pL0MsgQueue->ulHead]);
}


MCU0_DRAM_TEXT BOOL L0_MoveMsgQueueHead(void)
{
    l_pL0MsgQueue->ulNodeNum--;
    if (l_pL0MsgQueue->ulNodeNum >= l_pL0MsgQueue->ulMaxSize)
    {
        DBG_Printf("MsgNodeNum(%d) >= MsgMaxSize(%d)\n", l_pL0MsgQueue->ulNodeNum, l_pL0MsgQueue->ulMaxSize);
        DBG_Getch();
    }
    
    if (TRUE == L0_IsMsgQueueEmpty())
    {
        return FAIL;
    }

    QUEUE_PUSH_POINTER(l_pL0MsgQueue->ulHead, l_pL0MsgQueue->ulMaxSize);

    return TRUE;
}

MCU0_DRAM_TEXT BOOL L0_MoveMsgQueueTail()
{
    
    l_pL0MsgQueue->ulNodeNum++;

    if (l_pL0MsgQueue->ulNodeNum >= l_pL0MsgQueue->ulMaxSize)  //One node is reserved as Full Queue Marker
    {
        DBG_Printf("MsgNodeNum(%d) >= MsgMaxSize(%d)\n", l_pL0MsgQueue->ulNodeNum, l_pL0MsgQueue->ulMaxSize);
        DBG_Getch();
    }
    
    if (TRUE == L0_IsMsgQueueFull())
    {
        return FAIL;
    }

    QUEUE_PUSH_POINTER(l_pL0MsgQueue->ulTail, l_pL0MsgQueue->ulMaxSize);

    return TRUE;
}


/*===========================================================================
Func Name   :L0_PopMsgQueueNode
Input       :pMsgNode - A pointer of MsgNode which will store the MsgNode
Output      :NULL
Return Value:
Description :
History     :
==============================================================================*/
MCU0_DRAM_TEXT BOOL L0_PopMsgQueueNode(PL0MSGNODE pMsgNode)
{
    if (TRUE == L0_IsMsgQueueEmpty())
    {
        return FAIL;
    }

    if (NULL == pMsgNode)
    {
        DBG_Printf("Warning pMsgNode == NULL\n");
        return FAIL;
    }

    COM_MemCpy((U32*)pMsgNode, (U32*)&(l_pL0MsgQueue->MsgNode[l_pL0MsgQueue->ulHead]), sizeof(L0MSGNODE) / sizeof(U32));

    return L0_MoveMsgQueueHead();
}



/*===========================================================================
Func Name   :L0_PushMsgQueueNode
Input       :pNewMsgNode - The pointer of MsgNode
Output      :NULL
Return Value:TRUE - Indicated  Push L0 Msg Node to Queue successfully
             FAIL - Fail to push msg node
Description :
History     :
==============================================================================*/
MCU0_DRAM_TEXT BOOL L0_PushMsgQueueNode(PL0MSGNODE pNewMsgNode)
{
    PL0MSGNODE pMsgNode;
    U32 ulParamLen;
    U32 ulParamIndex;

    if (TRUE == L0_IsMsgQueueFull())
    {
        return FAIL;
    }

    if (NULL == pNewMsgNode)
    {
        DBG_Printf("L0NewMsgNode == NULL\n");
        return FAIL;
        //DBG_Getch();
    }

    ulParamLen = pNewMsgNode->ucParamLen;

    if (ulParamLen > MAX_L0MSG_PARAM_NUM)
    {
        DBG_Printf("Error:L0Msg Node Param Num(%d) > MAX_L0MSG_PARAM_NUM(%d)\n",
            ulParamLen, MAX_L0MSG_PARAM_NUM);
        return FAIL;
    }

    pMsgNode = &(l_pL0MsgQueue->MsgNode[l_pL0MsgQueue->ulTail]);
    pMsgNode->ulHeader = pNewMsgNode->ulHeader;

    for (ulParamIndex = 0; ulParamIndex < ulParamLen; ulParamIndex++)
    {
        pMsgNode->Param[ulParamIndex] = pNewMsgNode->Param[ulParamIndex];
    }

    return L0_MoveMsgQueueTail();

}



//UECC Error MsgNode Pushing 
MCU0_DRAM_TEXT BOOL L0_PushMsgQueueUeccNode(U32 ulSubSytemID, U32 ulSlotID, U32 ulPUID)
{
    L0MSGNODE MsgNode;

    COM_MemSet((U32*)&MsgNode, sizeof(L0MSGNODE) / sizeof(U32), 0);

    MsgNode.ulHeader = (
        L0MSG_TYPE_ERROR << L0MSG_TYPE_SHIF |
        L0MSG_SUBTYPE_UECCERROR << L0MSG_SUBTYPE_SHIF |
        L0MSG_SUBTYPE_UECCERROR_PARAMSIZE << L0MSG_PARAMLEN_SHIF);

    MsgNode.Param[0] = (
        ulPUID << L0MSG_SUBTYPE_UECCERROR_PUID_SHIF |
        ulSlotID << L0MSG_SUBTYPE_UECCERROR_SLOTID_SHIF |
        ulSubSytemID << L0MSG_SUBTYPE_UECCERROR_SYSID_SHIF);

    return (L0_PushMsgQueueNode(&MsgNode));
    
}

//UECC Error MsgNode Handling
MCU0_DRAM_TEXT BOOL L0_ProcessErrorUeccMsgNode(PL0MSGNODE pMsgNode)
{
    PCB_MGR pSlotCbMgr;

    U32 ulSubSystemID = (pMsgNode->Param[0] & L0MSG_SUBTYPE_UECCERROR_SYSID_MASK) >> L0MSG_SUBTYPE_UECCERROR_SYSID_SHIF;
    U32 ulSlotID = (pMsgNode->Param[0] & L0MSG_SUBTYPE_UECCERROR_SLOTID_MASK) >> L0MSG_SUBTYPE_UECCERROR_SLOTID_SHIF;
    U32 ulPuID = (pMsgNode->Param[0] & L0MSG_SUBTYPE_UECCERROR_PUID_MASK) >> L0MSG_SUBTYPE_UECCERROR_PUID_SHIF;

    if ((ulSubSystemID > SUB_SYSTEM_NUM_MAX - 1 ) || 
        (ulSlotID > MAX_SLOT_NUM - 1) || (ulPuID > SUBSYSTEM_LUN_MAX - 1))
    {
        DBG_Printf("Uecc Msg Node Param Error. SID:%d, SlotID:%d, PUID:%d\n",
            ulSubSystemID, ulSlotID, ulPuID);
        DBG_Getch();
    }
    
    PSMSG_UECC_RSP_MARKER pUeccMarker = &(g_apMcShareData[ulSubSystemID]->tRspMarker.SmsgUeccRspMarker);

    //Check the last MsgNode is finished or Not
    if (pUeccMarker->ulMarkerType == SMS_UECC_CLEAR)      //Should wait
    {
        return  FALSE;
    }

    if (pUeccMarker->ulMarkerType != SMS_UECC_MARKER_VALID)
    {
        DBG_Printf("ulMarkerType(0x%x) != SMS_UECC_MARKER_VALID", pUeccMarker->ulMarkerType);
        DBG_Getch();
    }
    
    if (0 != pUeccMarker->ulPUID)
    {
        DBG_Printf("UeccPUID(%x) is uncleared.\n", pUeccMarker->ulPUID);
        DBG_Getch();
    }
    
    pSlotCbMgr = GET_SLOT_MGR(ulSlotID);
    if ( FALSE == L0_NVMeUeccPuBitMapIsDirty(pSlotCbMgr))    //Fisrt PU Uecc Error for this slot
    {
        //Store into NVMeInfo & Update WBQ(CQE). Fix Me:Should store more error information
        L0_NVMeUecc(ulSlotID);
    }
    
    if (TRUE == L0_NVMeUeccPuBitMapGet(pSlotCbMgr, ulPuID))
    {
        DBG_Printf("Warnning: Get the same PU number Uecc error in the same slot\n");
        //DBG_Getch();
    }
    L0_NVMeUeccPuBitMapSet(pSlotCbMgr, ulPuID, TRUE); //Pu ID is 0 based

    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_UECC_RSP_MARKER);
    pUeccMarker->ulPUID = ulPuID;
    pUeccMarker->ulMarkerType = SMS_UECC_CLEAR;
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_UECC_RSP_MARKER);

    DBG_Printf("L0 Processed Uecc MsgNode. SubSys:%d, PuID:%d\n", ulSubSystemID, ulPuID);

    return TRUE;
}


//Error MsgNode Handling
MCU0_DRAM_TEXT BOOL L0_ProcessErrorMsgNode(PL0MSGNODE pMsgNode)
{
    BOOL bFinish = FALSE;
    ASSERT(L0MSG_TYPE_ERROR == pMsgNode->ucMsgType);

    switch (pMsgNode->ucSubType)
    {
    case L0MSG_SUBTYPE_UECCERROR:
        bFinish = L0_ProcessErrorUeccMsgNode(pMsgNode);
        break;
    default:
        DBG_Printf("MsgType(%d) is unrecognized.\n", pMsgNode->ucMsgType);
        //DBG_Getch();
        //Ignore this msg
        bFinish = TRUE;
    }

    return bFinish;
}


/*===========================================================================
Func Name   :
Input       :NULL
Output      :NULL
Return Value:
Description :
History     :
==============================================================================*/
MCU0_DRAM_TEXT BOOL L0_ProceesMsgQueueNode(PL0MSGNODE pMsgNode)
{
    BOOL bFinish = FALSE;
    if (NULL == pMsgNode)
    {
        DBG_Printf("pMsgNode is NULL");
        DBG_Getch();
    }

    switch (pMsgNode->ucMsgType)
    {
    case L0MSG_TYPE_ERROR :
        bFinish = L0_ProcessErrorMsgNode(pMsgNode);
        break;
    default:
        DBG_Printf("MsgType(%d) is unrecognized.\n", pMsgNode->ucMsgType);
        //DBG_Getch();
        //Ignore this msg
        bFinish = TRUE;
    }

    return bFinish;
}

/*===========================================================================
Func Name   :L0_ProcessMsgQueue
Input       :NULL
Output      :NULL
Return Value:
Description :This function will not pop next MsgNode until the current MsgNode
             is handled.
History     :
==============================================================================*/
void L0_ProcessMsgQueue(void)
{
    U32 ulNodeIndex = 0;


    /*
    Queue Empty, l_pMsgNode NULL        : Nothing to do.
    Queue Empty, l_pMsgNode non-Null    : Logical error.
    Queue non-empty, l_pMsgNode NULL    : Get Msg Node and handle it
    Queue non-empty, l_pMsgNode non-NULL: Check node or wait MsgNode finished.
    */
    if (TRUE == L0_IsMsgQueueEmpty() && NULL == l_pMsgNode )
    {
        return;
    }
    else if (TRUE == L0_IsMsgQueueEmpty() && NULL != l_pMsgNode)
    {
        DBG_Printf("L0MsgQueue is empty, but the l_pMsgNode is non-NULL\n");
        DBG_Getch();
    }

    //Get Last or newest MsgNode
    if (NULL == l_pMsgNode)
    {
        l_pMsgNode = L0_GetMsgQueueNode();
        if (NULL == l_pMsgNode)
        {
            DBG_Printf("Error: L0MsgQueue is non-empty, But get New MsgNode Fail\n");
            DBG_Getch();
            //return;
        }
    }
        
    
    if (TRUE == L0_ProceesMsgQueueNode(l_pMsgNode))
    {
        l_pMsgNode = NULL;

        if (FAIL == L0_MoveMsgQueueHead())
        {
            DBG_Printf("Process MsgNode Successfully. Moving Head Pointer FAIL\n");
            DBG_Getch();
        }
    }

    /*
    do
    {
        //Get Last or New MsgNode
        if (NULL == l_pMsgNode)
        {
            l_pMsgNode = L0_GetMsgQueueNode();
            if (NULL == l_pMsgNode)
            {
                return;
            }
        }

        if (TRUE == L0_ProceesMsgQueueNode(l_pMsgNode))
        {
            l_pMsgNode = NULL;

            ASSERT(L0_MoveMsgQueueHead());
        }
    } while (NULL == l_pMsgNode); //If l_pMsgNode == NULL indicating MsgNode is processed.
                                  //Get and process the Next MsgNode.
    
    */
    
    return ;
}



