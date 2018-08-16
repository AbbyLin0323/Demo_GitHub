/****************************************************************************
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
*****************************************************************************
* File Name    : FW_ViaCmd.c
* Discription  :
* CreateAuthor : Haven Yang
* CreateDate   : 2015.1.15
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_TraceLog.h"
#include "HAL_ParamTable.h"
#include "Disk_Config.h"
#include "L3_HostAdapter.h"
#include "L1_SCmdInterface.h"
#include "L1_GlobalInfo.h"
#include "FW_ViaCmd.h"
#include "FW_BufAddr.h"
#include "FW_Event.h"
#ifndef L1_FAKE
#include "L2_RT.h"
#include "L2_PBIT.h"
#include "L2_VBT.h"
#include "L2_FCMDQ.h"
#endif
#include "L2_Interface.h"
#include "L1_Ramdisk.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define VD_FLASHADDR(Pu)    (l_pVarSubTable->ulDevFlashDataAddr[(Pu)])
#define VD_REDADDR(Pu)      (l_pVarSubTable->ulDevFlashRedAddr + (Pu) * sizeof(NFC_RED))

#define VD_FLASH_BUF_SIZE   ( SUBSYSTEM_PU_NUM * BUF_SIZE)
#define VD_RED_BUF_SIZE     (BUF_SIZE)
typedef BOOL (*ViaFlashBasicFun)(U8 ucPU, U8 ucBlun ,U32 ulParam);
/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern void FW_FlushSubsystemTrace(SCMD* pSCMD);
extern void FW_FlushMcu0Trace(SCMD* pSCMD);
extern void Ven_BbtSave(void);
extern BOOL Ven_BbtLoad(void);
extern U32 L2_BbtGetGBBTAddr(void);
extern void L2_Scheduler(void);
extern void L1_SetDefaultDeviceParam(void);
extern void L1_SetDefaultHostInfo(void);
#ifndef LCT_TRIM_REMOVED
extern void L1_CacheResetLCTTag(void);
#endif
#ifndef LCT_VALID_REMOVED
extern void L1_SetLCTValidLoadStatus(U8 ucStatus);
#endif
extern void FW_DbgShowAll(void);

GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
GLOBAL BOOL gbGlobalInfoSaveFlag;

BOOL MCU12_DRAM_TEXT HAL_FlashBasicReset(U8 ucPU, U8 ucBlun, U32 ulParam)
{
    FLASH_ADDR FlashAddr = {0};
    FlashAddr.ucPU = ucPU;
    return HAL_NfcResetFlash(&FlashAddr);

}
BOOL MCU12_DRAM_TEXT HAL_FlashBasicSendRetryTerminate(U8 ucPU, U8 ucBlun, U32 ulParam)
{
    BOOL bTlcMode = ulParam;
    return HAL_FlashRetryTerminate(ucPU,ucBlun, bTlcMode);
}
BOOL MCU12_DRAM_TEXT HAL_FlashBasicSendRetryVal(U8 ucPU, U8 ucBlun, U32 ulParam)
{
    U8 ucParaNum = ulParam;
    FLASH_ADDR FlashAddr = {0};

    FlashAddr.ucPU = ucPU;
    FlashAddr.ucLun = ucBlun;
    //ucParaNum = (FALSE == bTlcMode) ? HAL_FLASH_RETRY_PARA_MAX : HAL_TLC_FLASH_RETRY_PARA_MAX;

    HAL_FlashRetryEn(&FlashAddr, FALSE);

    return  HAL_FlashRetrySendParam(&FlashAddr, NULL, 0, ucParaNum);

}
BOOL MCU12_DRAM_TEXT HAL_FlashBasicSendRetryPreConditon(U8 ucPU, U8 ucBlun, U32 ulParam)
{
    FLASH_ADDR FlashAddr = {0};

    FlashAddr.ucPU = ucPU;
    FlashAddr.ucLun = ucBlun;

    return HAL_FlashRetryPreConditon(&FlashAddr);
}

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL MCU12_VAR_ATTR VAR_SUBSYS_TABLE *l_pVarSubTable;
LOCAL MCU12_VAR_ATTR U32 l_ulPuBuffAddr;
LOCAL MCU12_VAR_ATTR U32 l_ulRedDataAddr;

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlVarTable(SCMD* pSCMD)
{
    VAR_TABLE *pVarTable;
    VAR_SUBSYS_TABLE *pVST; /* var subsystem table */
    U32 ulSubSysId;
    U8  ucMcuID;
    U8  ucPu;
    TL_INFO* pTLInfo;

    ucMcuID = HAL_GetMcuId();
    pTLInfo = TL_GetTLInfo(ucMcuID);

    ulSubSysId = ucMcuID - MCU1_ID;
    pVarTable  = (VAR_TABLE *)pSCMD->tViaDevCtrl.ulCmdDefine;
    pVST       = &pVarTable->aSubSysTable[ulSubSysId];

    for (ucPu = 0; ucPu < SUBSYSTEM_PU_NUM; ucPu++)
    {
        pVST->ulDevFlashDataAddr[ucPu] = l_ulPuBuffAddr + ucPu * BUF_SIZE;
    }

    pVST->ulDevFlashRedAddr = l_ulRedDataAddr;

    pVST->ulPhyPuMskLow  = HAL_GetSubSystemCEMap(ucMcuID);
    pVST->ulPhyPuMskHigh = 0;

    pVST->ulPuNum   = SUBSYSTEM_PU_NUM;
    pVST->ulBlkNum  = BLK_PER_PLN;
    pVST->ulRsvBlkNum = RSV_BLK_PER_PLN;
    pVST->ulPageNum = LOGIC_PG_PER_BLK;
    pVST->ulPlnNum  = PLN_PER_LUN;
    pVST->ulPhyPageSize = LOGIC_PG_SZ;
    pVST->ulFlashId[0] = HAL_GetFlashChipId(0);
    pVST->ulFlashId[1] = HAL_GetFlashChipId(1);

    pVST->ulSuperPuNum   = SUBSYSTEM_SUPERPU_NUM;
    pVST->ulLunInSuperPu  = LUN_NUM_PER_SUPERPU;

#ifndef L1_FAKE
    pVST->ulL3BbtTableAddr = L2_BbtGetGBBTAddr();//BBT_GetBBTAddr();

    /* init FW debug infos */
    pVST->ulDParamBaseAddr   = (U32)g_pSubSystemDevParamPage;
    pVST->ulDParamBaseSize   = sizeof(DEVICE_PARAM_PAGE);

    for (ucPu = 0; ucPu < SUBSYSTEM_PU_NUM; ucPu++)
    {
        pVST->ulFlashMonitorAddr[ucPu] = (U32)L3_FMGetUsrItem(L2_GET_TLUN(ucPu, 0));
        pVST->ulFlashMonitorSize[ucPu] = sizeof(FM_USER_ITEM);
    }

    for (ucPu = 0; ucPu < SUBSYSTEM_SUPERPU_NUM; ucPu++)
    {
        pVST->ulPBITBaseAddr[ucPu] = (U32)(pPBIT[ucPu]);
        pVST->ulPBITBaseSize[ucPu] = sizeof(PBIT);
        pVST->ulVBTBaseAddr[ucPu]  = (U32)(pVBT[ucPu]);
        pVST->ulVBTBaseSize[ucPu]  = sizeof(VBT);
    }

    pVST->ulRTBaseAddr = (U32)pRT;
    pVST->ulRTBaseSize = sizeof(RT);
#endif

    l_pVarSubTable = pVST;
    return TRUE;
}

#if (!defined(L1_FAKE)) && (!defined(L2_FAKE))
extern void L1_HostIOWarmInit(void);
extern void L1_RamdiskClearSCQ(void);

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlL2Format(SCMD* pSCMD)
{
    COMMON_EVENT Event;
    BOOTLOADER_FILE *pBootLoader;
    COMM_EVENT_PARAMETER *pParameter;

    /* L1 set default infos */
    L1_SetDefaultDeviceParam();
    L1_SetDefaultHostInfo();
#ifndef LCT_TRIM_REMOVED
    L1_CacheResetLCTTag();
#endif

     /* get parameter from comm event */ 
     CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);

     /* set keep erase cnt flag*/
     pParameter->EventParameterNormal[0] = TRUE;

     /* set security erase flag*/
     pParameter->EventParameterNormal[1] = FALSE;

    /* set L2 LLF event */
    CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_LLF);

    /* call L2/L3 Scheduler and wait L2 LLF done */
    CommCheckEvent(COMM_EVENT_OWNER_L2, &Event);

    // Set as RamDisk when LLF in Normal mode
    pBootLoader = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
    if (BOOT_METHOD_NORMAL == pBootLoader->tSysParameterTable.sBootStaticFlag.bsBootMethodSel)
    {
        /* clear all SCQ pending SCMD */
        //L1_RamdiskClearSCQ();

        /* reset HostIO */
        L1_HostIOWarmInit();

        L1_RamdiskSetMode(L1_RAMDISK_ENABLE_NORMAL);
    }

    while (TRUE == Event.EventLLF)
    {
        L2_Scheduler();

        //L3_Scheduler();

        CommCheckEvent(COMM_EVENT_OWNER_L2, &Event);
    }

    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == pParameter->EventStatus)
    {
        DBG_Printf("FW_DevCtrlL2Format: COMM_EVENT_OFFSET_LLF success! \n");
        return TRUE;
    }
    else
    {
        DBG_Printf("FW_DevCtrlL2Format: COMM_EVENT_OFFSET_LLF Fail! \n");
        return FALSE;
    }
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlL3Format(SCMD* pSCMD)
{
    //Vend_L3_LLF();
    return TRUE;
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlBBTSave(SCMD* pSCMD)
{
    Ven_BbtSave();
    return TRUE;
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlBBTLoad(SCMD* pSCMD)
{
    return Ven_BbtLoad();
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlDebugShowAll(SCMD* pSCMD)
{
    FW_DbgShowAll();
    return TRUE;
}

MCU12_DRAM_TEXT LOCAL void FW_DevCtrlFlashCommon(SCMD* pSCMD,U32 ulInput,ViaFlashBasicFun pFun)
{
    U32 ulLunMsk;
    U32 * pOutPut0;
    BOOL ulCmdStatus;
    U8 ucPu,ucLun,ucLoop;

    ulLunMsk = pSCMD->tViaDevCtrl.tViaParam.tFlashSetParam.ulLunMsk;
    pOutPut0 = &(pSCMD->tViaDevCtrl.aOutputValue[0]);
    for(ucLoop = 0;ucLoop<32;ucLoop++)
    {
        if(ulLunMsk&(1<<ucLoop))
        {
            ucPu = (ucLoop/LUN_NUM_PER_PU);
            ucLun = (ucLoop%LUN_NUM_PER_PU);
            (ViaFlashBasicFun)pFun(ucPu,ucLun,ulInput);
            ulCmdStatus = HAL_NfcWaitStatus(ucPu, ucLun);
            if (NFC_STATUS_SUCCESS != ulCmdStatus)
            {
                if(pOutPut0!=NULL)
                {
                    *pOutPut0|=(1<<(ucLoop));
                }
            }
        }
    }
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlFlashPreCondition(SCMD* pSCMD)
{
    ViaFlashBasicFun pFun;
    pFun = HAL_FlashBasicSendRetryPreConditon;
    FW_DevCtrlFlashCommon(pSCMD,NULL,pFun);
    return TRUE;
}
MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlFlashTerminate(SCMD* pSCMD)
{
    ViaFlashBasicFun pFun;
    U32 ulParam = 0;//only meanful in TSB 3D TLC flash, 0-SLC, 1-TLC
    pFun = HAL_FlashBasicSendRetryTerminate;
    FW_DevCtrlFlashCommon(pSCMD,NULL,pFun);

    return TRUE;
}
MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlFlashReset(SCMD* pSCMD)
{
    ViaFlashBasicFun pFun;
    pFun = HAL_FlashBasicReset;
    FW_DevCtrlFlashCommon(pSCMD,NULL,pFun);

    return TRUE;

}
MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlFlashSetParam(SCMD* pSCMD)
{
    ViaFlashBasicFun pFun;
    U32 ulParam;
    ulParam = pSCMD->tViaDevCtrl.tViaParam.tFlashSetParam.ulFlashParam;
    pFun = HAL_FlashBasicSendRetryVal;
    FW_DevCtrlFlashCommon(pSCMD,ulParam,pFun);

    return TRUE;
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlTraceLog(SCMD* pSCMD)
{
    U32             ulSecCnt;
    VIA_TLCT        eCtrlType;
    VIA_CMD_PARAM  *pCmdParam;

    pCmdParam = &pSCMD->tViaDevCtrl.tViaParam;

    eCtrlType = (VIA_TLCT)pCmdParam->aByte[0];

    switch(eCtrlType)
    {
        case TLCT_DISABLE_TL:
            TL_Disable();
            break;

        case TLCT_ENABLE_TL:
            TL_Enable();
            break;

        case TLCT_FLUSH_DATA:
            if (MCU1_ID == HAL_GetMcuId())
            {
                FW_FlushMcu0Trace(pSCMD);
            }
            FW_FlushSubsystemTrace(pSCMD);
            break;

        case TLCT_INVALID_DATA:
            ulSecCnt = (pCmdParam->aDW[0] >> 16);
            TL_InvalidateTraceMemory(ulSecCnt);
            break;

        default:

            break;
    }

    return TRUE;
}
#endif

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlRegRead(SCMD* pSCMD)
{
    U32 ulLocalAddr;

    ulLocalAddr = pSCMD->tViaDevCtrl.tViaParam.tRegAccess.ulAddr;

    pSCMD->tViaDevCtrl.aOutputValue[0] = *(volatile U32*)ulLocalAddr;
    return TRUE;
}

MCU12_DRAM_TEXT LOCAL BOOL FW_DevCtrlRegWrite(SCMD* pSCMD)
{
    U32 ulLocalAddr;

    ulLocalAddr = pSCMD->tViaDevCtrl.tViaParam.tRegAccess.ulAddr;

    *(volatile U32*)ulLocalAddr = pSCMD->tViaDevCtrl.tViaParam.tRegAccess.ulData;
    return TRUE;
}

/*==============================================================================
Func Name  : FW_ViaCmdDevCtrl
Input      : SCMD* pSCMD
Output     : NONE
Return Val : void
Discription: VIA vendor define command (Device Control) subsystem side implement.
Usage      : when a Device Control SCMD received, call this interface to
             complete it.
History    :
    1. 2015.1.20 Haven Yang create function
==============================================================================*/
MCU12_DRAM_TEXT BOOL FW_ViaCmdDevCtrl(SCMD* pSCMD)
{
    U8 ucDevCtrlCode = pSCMD->tViaDevCtrl.ucViaCmdCode;
    BOOL ret = TRUE;

    switch(ucDevCtrlCode)
    {
        case VIA_CMD_VAR_TABLE:
            ret = FW_DevCtrlVarTable(pSCMD);
            break;
        case VIA_CMD_REG_READ:
            ret = FW_DevCtrlRegRead(pSCMD);
            break;
        case VIA_CMD_REG_WRITE:
            ret = FW_DevCtrlRegWrite(pSCMD);
            break;
#if (!defined(L1_FAKE)) && (!defined(L2_FAKE))
        case VIA_CMD_TRACELOG_CONTROL:
            ret = FW_DevCtrlTraceLog(pSCMD);
            break;
        case VIA_CMD_L2_FORMAT:
            ret = FW_DevCtrlL2Format(pSCMD);
            break;
        case VIA_CMD_L3_FORMAT:
            ret = FW_DevCtrlL3Format(pSCMD);
            break;
        case VIA_CMD_BBT_SAVE:
            ret = FW_DevCtrlBBTSave(pSCMD);
            break;
        case VIA_CMD_BBT_LOAD:
            ret = FW_DevCtrlBBTLoad(pSCMD);
            break;
        case VIA_CMD_DBG_SHOWALL:
            ret = FW_DevCtrlDebugShowAll(pSCMD);
            break;
        case VIA_CMD_FLASH_PRECONDITION:
            ret = FW_DevCtrlFlashPreCondition(pSCMD);
            break;
        case VIA_CMD_FLASH_SETPARAM:
            ret = FW_DevCtrlFlashSetParam(pSCMD);
            break;
        case VIA_CMD_FLASH_TERMINATE:
            ret = FW_DevCtrlFlashTerminate(pSCMD);
            break;
        case VIA_CMD_FLASH_RESET:
            ret = FW_DevCtrlFlashReset(pSCMD);
            break;

#endif
        default:
            DBG_Printf("unkown VIA Device control command(%d) in MCU(%d)\n", ucDevCtrlCode, HAL_GetMcuId());
            DBG_Getch();
    }

    return ret;
}

MCU12_DRAM_TEXT LOCAL void FW_ViaCmdMemRead(SCMD* pSCMD)
{
    U32 ulBuffer;
    U32 ulLocalAddr;
    U32 ulByteLen;
    U8  ucMcuID = pSCMD->tRawData.tViaParam.tMemAccess.bsMcuID;

    if (HAL_GetMcuId() != ucMcuID)
    {
        DBG_Printf("VIA MemRead McuID(%d) error in MCU(%d)\n", ucMcuID, HAL_GetMcuId());
        DBG_Getch();
    }

    ulBuffer    = pSCMD->tRawData.ulBuffAddr;
    ulLocalAddr = pSCMD->tRawData.tViaParam.tMemAccess.ulDevAddr;
    ulByteLen   = pSCMD->tRawData.tViaParam.tMemAccess.bsByteLen;

    if (ulBuffer != ulLocalAddr)
    {
        COM_MemByteCopy((U8 *)ulBuffer, (U8 *)ulLocalAddr, ulByteLen);
    }
#ifdef DCACHE
    HAL_InvalidateDCache();
#endif
}

MCU12_DRAM_TEXT LOCAL void FW_ViaCmdMemWrite(SCMD* pSCMD)
{
    U32 ulBuffer;
    U32 ulLocalAddr;
    U32 ulByteLen;
    U8  ucMcuID = pSCMD->tRawData.tViaParam.tMemAccess.bsMcuID;

    if (HAL_GetMcuId() != ucMcuID)
    {
        DBG_Printf("VIA MemWrite McuID(%d) error in MCU(%d)\n", ucMcuID, HAL_GetMcuId());
        DBG_Getch();
    }

    ulBuffer    = pSCMD->tRawData.ulBuffAddr;
    ulLocalAddr = pSCMD->tRawData.tViaParam.tMemAccess.ulDevAddr;
    ulByteLen   = pSCMD->tRawData.tViaParam.tMemAccess.bsByteLen;
    if (ulLocalAddr != ulBuffer)
    {
        COM_MemByteCopy((U8 *)ulLocalAddr, (U8 *)ulBuffer, ulByteLen);
    }
}

MCU12_DRAM_TEXT LOCAL void L2_ViaEraseBlock(FLASH_ADDR *pFlashAddr, BOOL bSinglePln, U8* pStatus)
{
    U8 ucTLun;
    FCMD_REQ_ENTRY *ptReqEntry;

    ucTLun = L3_GET_TLUN(pFlashAddr->ucPU, pFlashAddr->ucLun);

    while (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        ;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
    ptReqEntry->bsReqSubType = bSinglePln ? FCMD_REQ_SUBTYPE_SINGLE : FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsTableReq = TRUE;

    ptReqEntry->tFlashDesc.bsVirBlk = pFlashAddr->usBlock;
    ptReqEntry->tFlashDesc.bsPhyBlk = pFlashAddr->usBlock;
    ptReqEntry->tFlashDesc.bsPlnNum = pFlashAddr->bsPln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;

    if (NULL != pStatus)
    {
        *pStatus = SUBSYSTEM_STATUS_PENDING;
        ptReqEntry->ulReqStsAddr = (U32)pStatus;
        ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
    }

    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

MCU12_DRAM_TEXT LOCAL void L2_ViaWritePage(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq, BOOL bSinglePln, U8 *pStatus)
{
    U8 ucTLun;
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;

    ucTLun = L3_GET_TLUN(pFlashAddr->ucPU, pFlashAddr->ucLun);

    while (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        ;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = bSinglePln ? FCMD_REQ_SUBTYPE_SINGLE : FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsTableReq = TRUE;

    ptReqEntry->tFlashDesc.bsVirBlk = pFlashAddr->usBlock;
    ptReqEntry->tFlashDesc.bsPhyBlk = pFlashAddr->usBlock;
    ptReqEntry->tFlashDesc.bsVirPage = pFlashAddr->usPage;
    ptReqEntry->tFlashDesc.bsPlnNum = pFlashAddr->bsPln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = bSinglePln ? SEC_PER_LOGIC_PG : SEC_PER_BUF;

    ptReqEntry->atBufDesc[0].bsBufID = pWrReq->bsWrBuffId;
    ptReqEntry->atBufDesc[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ptReqEntry->atBufDesc[0].bsSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

    if (TRUE == pWrReq->bsRedOntf)
    {
        pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
        COM_MemCpy(pTargetRed, (U32*)pWrReq->pNfcRed, sizeof(RED) / sizeof(U32));
        ptReqEntry->ulSpareAddr = (U32)pTargetRed;
    }

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

MCU12_DRAM_TEXT LOCAL void L2_ViaReadPage(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, BOOL bSinglePln, U8 *pStatus)
{
    U8 ucTLun;
    FCMD_REQ_ENTRY *ptReqEntry;

    ucTLun = L3_GET_TLUN(pFlashAddr->ucPU, pFlashAddr->ucLun);

    while (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        ;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
    ptReqEntry->bsReqSubType = bSinglePln ? FCMD_REQ_SUBTYPE_SINGLE : FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsTableReq = TRUE;

    ptReqEntry->tFlashDesc.bsVirBlk = pFlashAddr->usBlock;
    ptReqEntry->tFlashDesc.bsPhyBlk = pFlashAddr->usBlock;
    ptReqEntry->tFlashDesc.bsVirPage = pFlashAddr->usPage;
    ptReqEntry->tFlashDesc.bsPlnNum = pFlashAddr->bsPln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = pRdReq->bsSecStart;
    ptReqEntry->tFlashDesc.bsSecLen = pRdReq->bsSecLen;

    ptReqEntry->atBufDesc[0].bsBufID = pRdReq->bsRdBuffId;
    ptReqEntry->atBufDesc[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ptReqEntry->atBufDesc[0].bsSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

    if (TRUE == pRdReq->bsRedOntf)
    {
        ptReqEntry->ulSpareAddr = (U32)(*(pRdReq->ppNfcRed));
        if (0 == ptReqEntry->tFlashDesc.bsSecLen)
        {
            ptReqEntry->tFlashDesc.bsRdRedOnly = TRUE;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
            ptReqEntry->atBufDesc[0].bsBufID = INVALID_4F;
            ptReqEntry->atBufDesc[0].bsSecStart = INVALID_2F;
            ptReqEntry->tFlashDesc.bsSecLen = INVALID_2F;
        }
    }

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

MCU12_DRAM_TEXT LOCAL void FW_ViaCmdFlashErase(SCMD* pSCMD)
{
    FLASH_ADDR  tFlashAddr = { 0 };
    U32         ulPuMask;
    U32         ulStatusAddr;
    U8          ucPuNum;
    U8          ucPlaneMode;
    U8          *pStatus;

    ucPlaneMode = pSCMD->tRawData.tViaParam.tFlashAccess.bsPlaneMode;
    ulPuMask = pSCMD->tRawData.tViaParam.tFlashAccess.ulPuMask;
    ulStatusAddr = pSCMD->tRawData.ulBuffAddr;

    tFlashAddr.usBlock = pSCMD->tRawData.tViaParam.tFlashAccess.bsBlock;
    tFlashAddr.bsPln = pSCMD->tRawData.tViaParam.tFlashAccess.bsPlane;
    tFlashAddr.ucLun = VIA_TBD;
    tFlashAddr.usPage = 0;

    /* 1. send flash read command */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
    {
        if (ulPuMask & BIT(ucPuNum))
        {
            tFlashAddr.ucPU = ucPuNum;
            pStatus = (U8 *)(ulStatusAddr + (ucPuNum * 4));
            *(U32 *)pStatus = 0;

            if (FULL_PLANE_MODE == ucPlaneMode)
            {
                L2_ViaEraseBlock(&tFlashAddr, FALSE, pStatus);
            }
            else
            {
                L2_ViaEraseBlock(&tFlashAddr, TRUE, pStatus);
            }
        }
    }

    /* 2. wait flash IDLE, check flash erase result and save to L0 temp buffer */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
    {
        if (ulPuMask & BIT(ucPuNum))
        {
            pStatus = (U8 *)(ulStatusAddr + (ucPuNum * 4));

            while (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                DBG_Printf("PU:%d Via Erase Cmd is Pending\n", ucPuNum);
                ; //wait FCMD finish
            }

            if (SUBSYSTEM_STATUS_SUCCESS == *pStatus)
            {
                DBG_Printf("PU:%d Via Erase Cmd is Success\n", ucPuNum);
                *(U32 *)pStatus = NF_SUCCESS;    //adapt to Hal
            }
            else
            {
                DBG_Printf("PU:%d Via Erase Cmd is Fail\n", ucPuNum);
            }
        }
    }

    return;
}

MCU12_DRAM_TEXT LOCAL void FW_ViaCmdFlashWrite(SCMD* pSCMD)
{
    NFC_PRG_REQ_DES tDes = { 0 };
    FLASH_ADDR  tFlashAddr = { 0 };
    U32         ulPuMask;
    U32         ulStatusAddr;
    U8          ucPuNum;
    U8          ucPlaneMode;
    U8          *pStatus;

    ucPlaneMode = pSCMD->tRawData.tViaParam.tFlashAccess.bsPlaneMode;
    ulPuMask = pSCMD->tRawData.tViaParam.tFlashAccess.ulPuMask;
    ulStatusAddr = pSCMD->tRawData.ulBuffAddr;

    tFlashAddr.usBlock = pSCMD->tRawData.tViaParam.tFlashAccess.bsBlock;
    tFlashAddr.bsPln = pSCMD->tRawData.tViaParam.tFlashAccess.bsPlane;
    tFlashAddr.usPage = pSCMD->tRawData.tViaParam.tFlashAccess.bsPage;
    tFlashAddr.ucLun = VIA_TBD;

    /* 1. send flash write command */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
    {
        if (ulPuMask & BIT(ucPuNum))
        {
            tFlashAddr.ucPU = ucPuNum;
            pStatus = (U8 *)(ulStatusAddr + (ucPuNum * 4));
            *(U32 *)pStatus = 0;

            if (FULL_PLANE_MODE == ucPlaneMode)
            {
                tDes.bsWrBuffId = (U16)COM_GetBufferIDByMemAddr(VD_FLASHADDR(ucPuNum), TRUE, BUF_SIZE_BITS);
                tDes.bsRedOntf = TRUE;
                tDes.pNfcRed = (NFC_RED *)VD_REDADDR(ucPuNum);
                tDes.pErrInj = NULL;

                L2_ViaWritePage(&tFlashAddr, &tDes, FALSE, pStatus);
            }
            else
            {
                tDes.bsWrBuffId = (U16)COM_GetBufferIDByMemAddr((VD_FLASHADDR(ucPuNum) + tFlashAddr.bsPln*(1 << LOGIC_PG_SZ_BITS)), TRUE, LOGIC_PG_SZ_BITS);
                tDes.bsRedOntf = TRUE;
                tDes.pNfcRed = (NFC_RED *)(VD_REDADDR(ucPuNum) + RED_SZ * tFlashAddr.bsPln);
                tDes.pErrInj = NULL;

                L2_ViaWritePage(&tFlashAddr, &tDes, TRUE, pStatus);
            }
        }
    }


    /* 2. wait flash IDLE, check flash write result and save to L0 temp buffer */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
    {
        if (ulPuMask & BIT(ucPuNum))
        {
            pStatus = (U8 *)(ulStatusAddr + (ucPuNum * 4));

            while (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                DBG_Printf("PU:%d Via Write Cmd is Pending\n", ucPuNum);
                ; //wait FCMD finish
            }

            if (SUBSYSTEM_STATUS_SUCCESS == *pStatus)
            {
                DBG_Printf("PU:%d Via Write Cmd is Success\n", ucPuNum);
                *(U32 *)pStatus = NF_SUCCESS;    //adapt to Hal
            }
            else
            {
                DBG_Printf("PU:%d Via Write Cmd is Fail\n", ucPuNum);
            }
        }
    }

    return;
}

MCU12_DRAM_TEXT LOCAL void FW_ViaCmdFlashRead(SCMD* pSCMD)
{
    NFC_READ_REQ_DES tDes={0};
    FLASH_ADDR  tFlashAddr = { 0 };
    NFC_RED     *ptReadRedSW;
    U32         ulPuMask;
    U32         ulStatusAddr;
    U32         ulRedAddr;
    U16         usBuffID;
    U8          ucPuNum;
    U8          ucPlaneMode;
    U8          ucSecLen;
    U8          *pStatus;

    ulPuMask     = pSCMD->tRawData.tViaParam.tFlashAccess.ulPuMask;
    ulStatusAddr = pSCMD->tRawData.ulBuffAddr;
    ucPlaneMode  = pSCMD->tRawData.tViaParam.tFlashAccess.bsPlaneMode;

    tFlashAddr.usBlock = pSCMD->tRawData.tViaParam.tFlashAccess.bsBlock;
    tFlashAddr.bsPln   = pSCMD->tRawData.tViaParam.tFlashAccess.bsPlane;
    tFlashAddr.usPage  = pSCMD->tRawData.tViaParam.tFlashAccess.bsPage;
    tFlashAddr.ucLun   = VIA_TBD;


    /* 1. send flash read command */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
    {
        if (ulPuMask & BIT(ucPuNum))
        {
            tFlashAddr.ucPU    = ucPuNum;
            pStatus = (U8 *)(ulStatusAddr + (ucPuNum * 4));
            *(U32 *)pStatus = 0;

            if (FULL_PLANE_MODE == ucPlaneMode)
            {
                usBuffID = COM_GetBufferIDByMemAddr(VD_FLASHADDR(ucPuNum), TRUE, BUF_SIZE_BITS);
                ucSecLen = (PLN_PER_LUN * LOGIC_PG_SZ) / SEC_SIZE;

                tDes.bsSecStart = 0;
                tDes.bsSecLen = ucSecLen;
                tDes.bsRdBuffId = usBuffID;
                tDes.bsRedOntf = TRUE;
                ulRedAddr = (U32)VD_REDADDR(ucPuNum);
                COM_MemZero((U32*)ulRedAddr, RED_SW_SZ_DW);
                ptReadRedSW = (NFC_RED *)ulRedAddr;
                tDes.ppNfcRed = (NFC_RED **)&ptReadRedSW;
                tDes.pErrInj = NULL;

                L2_ViaReadPage(&tFlashAddr, &tDes, FALSE, pStatus);
            }
            else
            {
                usBuffID = COM_GetBufferIDByMemAddr(VD_FLASHADDR(ucPuNum), TRUE, LOGIC_PG_SZ_BITS);
                usBuffID += tFlashAddr.bsPln;
                ucSecLen = (LOGIC_PG_SZ) / SEC_SIZE;

                tDes.bsSecStart = 0;
                tDes.bsSecLen = ucSecLen;
                tDes.bsRdBuffId = usBuffID;
                tDes.bsRedOntf = TRUE;
                ulRedAddr = VD_REDADDR(ucPuNum) + RED_SZ * tFlashAddr.bsPln;
                COM_MemZero((U32*)ulRedAddr, RED_SZ_DW);
                ptReadRedSW = (NFC_RED *)ulRedAddr;
                tDes.ppNfcRed = (NFC_RED **)&ptReadRedSW;
                tDes.pErrInj = NULL;

                L2_ViaReadPage(&tFlashAddr, &tDes, TRUE, pStatus);
            }
        }
    }

    /* 2. wait flash IDLE, check flash read result and save to L0 temp buffer */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_PU_NUM; ucPuNum++)
    {
        if (ulPuMask & BIT(ucPuNum))
        {
            pStatus = (U8 *)(ulStatusAddr + (ucPuNum * 4));

            while (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                DBG_Printf("PU:%d Via Read Cmd is Pending\n", ucPuNum);
                ; //wait FCMD finish
            }

            if (SUBSYSTEM_STATUS_SUCCESS == *pStatus)
            {
                DBG_Printf("PU:%d Via Read Cmd is Success\n", ucPuNum);
                *(U32 *)pStatus = NF_SUCCESS;    //adapt to Hal
            }
            else
            {
                DBG_Printf("PU:%d Via Read Cmd is Fail\n", ucPuNum);
                if (SUBSYSTEM_STATUS_EMPTY_PG == *pStatus)
                {
                    *(U32 *)pStatus = 7;   //adapt to Hal
                }
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : FW_ViaCmdPrepareData
Input      : SCMD* pSCMD
Output     : NONE
Return Val : void
Discription: subsystem prepare data to the buffer which defined in L0. and xfer
             this data to the host in the following operation.
Usage      : when subsystem receive a RAW_DATA_REQ SCMD and required subsystem
             preparing data, call this interface to prepare data.
             this interface is only used by via vendor define command.
History    :
    1. 2015.1.20 Haven Yang create function
==============================================================================*/
MCU12_DRAM_TEXT void FW_ViaCmdPrepareData(SCMD* pSCMD)
{
    U8 ucCmdCode = pSCMD->ucSCmdSpecific;

    switch(ucCmdCode)
    {
        case VIA_CMD_MEM_READ:
            FW_ViaCmdMemRead(pSCMD);
            break;
        case VIA_CMD_MEM_WRITE:
#ifdef DCACHE
            HAL_InvalidateDCache();
#endif
            return;
            break;
        case VIA_CMD_FLASH_READ:
            FW_ViaCmdFlashRead(pSCMD);
            break;
        case VIA_CMD_FLASH_WRITE:
            FW_ViaCmdFlashWrite(pSCMD);
            break;
        case VIA_CMD_FLASH_ERASE:
            FW_ViaCmdFlashErase(pSCMD);
            break;
        default:
            DBG_Printf("unkown VIA vendor command(%d) in MCU(%d)\n", ucCmdCode, HAL_GetMcuId());
            DBG_Getch();
    }

    pSCMD->ucSCmdSpecific = VIA_CMD_NULL;
}

/*==============================================================================
Func Name  : FW_ViaCmdXferDataDone
Input      : SCMD* pSCMD
Output     : NONE
Return Val : void
Discription: local operation after data xfer down for VIA vendor define command
Usage      :
History    :
    1. 2015.1.20 Haven Yang create function
==============================================================================*/
MCU12_DRAM_TEXT void FW_ViaCmdXferDataDone(SCMD* pSCMD)
{
    if (VIA_CMD_MEM_WRITE == pSCMD->ucSCmdSpecific)
    {
        FW_ViaCmdMemWrite(pSCMD);
    }
    else
    {
        // do nothing
    }
}

MCU12_DRAM_TEXT void FW_ViaCmdInit(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;

    COM_MemAddrPageBoundaryAlign(pFreeDramBase);
    ulFreeDramBase = *pFreeDramBase;

    l_ulPuBuffAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, VD_FLASH_BUF_SIZE);

    l_ulRedDataAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, VD_RED_BUF_SIZE);

    *pFreeDramBase = ulFreeDramBase;
    return;
}

/*====================End of this file========================================*/

