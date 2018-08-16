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
* File Name    : L0_NVMeAdminCmd.c
* Discription  : This file is create to implement the Admin Cmd set defined in
*                NVMe spec, including Delete SQ/CQ, create SQ/CQ, Get log page,
*                Identify, Abort, Set feature, Get Feature, Async Event...
* CreateAuthor : Haven Yang
* CreateDate   : 2014.12.18
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_HCmdTimer.h"
#include "HAL_NVME.h"
#ifdef AF_ENABLE
#include "HAL_NVMECFGEX.h"
#endif
#include "NvmeSpec.h"
#include "L0_Config.h"
#include "L0_NVMe.h"
#include "L0_NVMeAdminCmd.h"
#include "L0_NVMeHCT.h"
#include "L0_NVMeDataIO.h"
#include "L0_NVMeErrHandle.h"
#include "L0_Interface.h"
#include "L0_ViaCmd.h"
#include "HAL_ParamTable.h"
#include "HAL_HCT.h"
#include "HAL_TraceLog.h"
#ifndef SIM
#include "HAL_PM.h"
#endif

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define NVME_NUMBER_OF_SQS      (MAX_SQ_NUM - 1)
#define NVME_NUMBER_OF_CQS      (MAX_CQ_NUM - 1)
#define NVME_NUMBER_OF_VECTOR   8

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern FW_UPDATE g_tFwUpdate;
extern U32 g_ulInfoIdfyPage;
extern U32 g_ulSubsysNum;
extern U32 g_ulSubSysBootOk;
extern U32 g_ulHostInfoAddr;
extern U32 g_ulATAGPLBuffStart;
extern U32 g_ulATARawBuffStart;
extern U32 g_ulFWUpdateImageAddr;
extern U32 g_ulDevParamAddr;
extern PTABLE *g_pBootParamTable;
#ifdef AF_ENABLE
extern volatile NVME_CFG_EX *g_pNVMeCfgExReg;
#endif

extern void L0_FillFwRuntimeInfo(FW_RUNTIME_INFO *pFwInfo);
extern void L0_UpgradeBootloader(U32 ulFwSlot);
extern void L0_UpgradeSpiRom(U32 ulFwSlot);
extern BOOL L0_FwUpgradeCheckImgValid(void);
extern void L0_FwUpgradeCalcImgLen(void);
extern NVME_TEMPERATURE_THRESHOLD_TABLE g_tNVMeTempThrldTable;

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/


#define FEATURE_ATTR_SAVEABLE    0x01
#define FEATURE_ATTR_NAMESPEC    0x02
#define FEATURE_ATTR_CHANGEABLE  0x04

typedef union _NVME_FEATURE_ENTRY_ATTR{
    U32 ulDW0;
    struct {
        U32 ulSaveable : 1;
        U32 ulNameSpec : 1;
        U32 ulChangeable : 1;
        U32 ulRsv;
    };
}NVME_FEATURE_ENTRY_ATTR, *PNVME_FEATURE_ENTRY_ATTR;

typedef struct _NVME_FEATURE_ENTRY
{
    NVME_FEATURE_ENTRY_ATTR uAttribute;
    U32 ulDefVal;
    U32 ulSavedVal;
    U32 ulCurVal;
    U32 ulSavedMarker;  //Indicated the saveVal is set
}NVME_FEATURE_ENTRY, *PNVME_FEATURE_ENTRY;


typedef struct _NVME_FEATURE_TABLE
{
    NVME_FEATURE_ENTRY tFea[FEATURE_NUM];
}NVME_FEATURE_TABLE, *PNVME_FEATURE_TABLE;



/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL NVME_FEATURE_TABLE gNvmeFeaTbl;

#define LL(a,b) b,
LOCAL const U32 gNvmeFIDMap[] = {
    FEATURE_LIST
    FEATURE_NUM
};
#undef LL

//GLOBAL FW_UPGRADE_PAYLOAD_INFO g_aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_IMG_NUM];

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/


MCU0_DRAM_TEXT BOOL L0_NVMeProcessAdminCmd(PCB_MGR pSlot)
{
    U32 ulSlotNum;
    U32 ulSlotHwState;
    BOOL bFinish;

    ulSlotNum = pSlot->SlotNum;
    ulSlotHwState = HAL_HCTGetCST(ulSlotNum);

    if ((SLOT_SQENTRY_RDY == ulSlotHwState)
        || (SLOT_PRPLIST_RDY == ulSlotHwState))
    {
        switch(pSlot->Ch->OPC)
        {
        case ACS_DELETE_SQ:
            bFinish = L0_NVMeDeleteSQ(pSlot);        
            break;
        case ACS_CREATE_SQ:
            bFinish = L0_NVMeCreateSQ(pSlot);
            break;
        case ACS_GET_LOG_PAGE:
            bFinish = L0_NVMeGetLogPage(pSlot);
            break;
        case ACS_DELETE_CQ:
            bFinish = L0_NVMeDeleteCQ(pSlot);
            break;
        case ACS_CREATE_CQ:
            bFinish = L0_NVMeCreateCQ(pSlot);
            break;
        case ACS_IDENTIFY:
            bFinish = L0_NVMeIdentify(pSlot);
            break;
        case ACS_ABORT:
            bFinish = L0_NVMeAbort(pSlot);
            break;
        case ACS_SET_FEATURES:
            bFinish = L0_NVMeSetFeatures(pSlot);
            break;
        case ACS_GET_FEATURES:
            bFinish = L0_NVMeGetFeatures(pSlot);
            break;
        case ACS_ASYN_EVENT_REQ:
            L0_NVMeAsyncEvent(pSlot);
            return TRUE; /* Async event must return at here */
        case ACS_FW_COMMIT:
            bFinish = L0_NVMeFwCommit(pSlot);
            break;
        case ACS_IMG_DOWNLOAD:
            bFinish = L0_NVMeImgDownload(pSlot);
            break;
        case ACS_FORMAT_NVM:
            bFinish = L0_NVMeFormatNvm(pSlot);
            break;
        case ACS_VIA_VENDOR_CMD:
            bFinish = L0_NVMeVIACmd(pSlot);
            return bFinish; /* VIA vendor command must return at here */
        default:
            DBG_Printf("unknown admin command, slot[0x%x], opcode[0x%x]\n", pSlot->SlotNum, pSlot->Ch->OPC);
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(0, 0, NVME_SF_IVLD_OPCODE));
            bFinish = TRUE;
        }
        
        if(TRUE == bFinish)
        {
            L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
            HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
            pSlot->CmdState = CMD_COMPLETED;
        }
    }

    else
    {
        bFinish = FALSE;
    }

    return bFinish;
}


MCU0_DRAM_TEXT BOOL L0_NVMeCreateCQ(PCB_MGR pSlot)
{
    U32 ulCQID;
    U32 ulCQSize;
    U32 ulIntrVec;
    U32 ulIntrEn;
    U32 ulMemPhyCont;
    U32 ulSupported = FALSE;
    U32 ulStatusField;
   
    ulCQID = (pSlot->Ch->DW10 & MSK_4F);
    ulCQSize = (pSlot->Ch->DW10 >> 16) + 1;
    ulIntrVec = (pSlot->Ch->DW11 >> 16);
    ulIntrEn = (pSlot->Ch->DW11 >> 1) & 1;
    ulMemPhyCont = (pSlot->Ch->DW11) & 1;

    /* 1. parameter check */
    if ((FALSE == ulMemPhyCont) && (TRUE == NVME_CAP_CQR(g_pNVMeCfgReg->bar.capl)))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(11 * 4, 0, NVME_SF_IVLD_FIELD);
    }
#ifndef SIM
    else if (0 != (pSlot->Ch->PRP1L & HPAGE_SIZE_MSK))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(24, 0, NVME_SF_IVLD_PRP_OFS);
    }
#endif
    else if ((0 == ulCQID) || (ulCQID > NVME_NUMBER_OF_CQS) || CQ_VLD(ulCQID))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_QID);
    }
    else if ((ulCQSize < 2u) || (ulCQSize > (NVME_CAP_MQES(g_pNVMeCfgReg->bar.capl) + 1)))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4 + 2, 0, NVME_SF_IVLD_QSIZE);
    }
    else if ((ulIntrVec > NVME_NUMBER_OF_VECTOR) ||
        ((0 != ulIntrVec) && (0 == (rPCIeNCfg(0xB0) & BIT(31)))))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(11 * 4 + 2, 0, NVME_SF_IVLD_INT_VECTOR);
    }
    else
    {
        ulSupported = TRUE;
    }

    if(FALSE == ulSupported)
    {
#ifndef SPEEDUP_UNH_IOL
        DBG_Printf("Create CQ(%d) Error(%d)!\n", ulCQID, ulStatusField);
#endif
        L0_NVMeCmdError(pSlot->SlotNum, ulStatusField);
        SIM_Getch();
    }
    else
    {
        /* 2. update firmware manager information */
        gNvmeMgr.CQ[ulCQID].AdrL   = pSlot->Ch->PRP1L;
        gNvmeMgr.CQ[ulCQID].AdrH   = pSlot->Ch->PRP1H;
        gNvmeMgr.CQ[ulCQID].QID    = (U16)ulCQID;
        gNvmeMgr.CQ[ulCQID].QSIZE  = (U16)ulCQSize;
        gNvmeMgr.CQ[ulCQID].IV     = (U16)ulIntrVec;
        gNvmeMgr.CQ[ulCQID].IEN    = (U16)ulIntrEn;
        gNvmeMgr.CQ[ulCQID].PC     = ulMemPhyCont;
        gNvmeMgr.CQ[ulCQID].Valid  = TRUE;
        gNvmeMgr.CQ[ulCQID].CqTail = 0;

        /* 3. update hardware information */
        g_pNVMeCfgReg->cq_size[ulCQID - 1] = ulCQSize - 1; /*lint -e676 */
        g_pNVMeCfgReg->cq_info[ulCQID].base_addr_l = pSlot->Ch->PRP1L;
        g_pNVMeCfgReg->cq_info[ulCQID].base_addr_h = pSlot->Ch->PRP1H;
        g_pNVMeCfgReg->msix_vector |= (ulIntrVec & MSK_1F) << ((ulCQID - 1) << 2);
        
        DBG_Printf("Create CQ: %d, Size:%d\n", ulCQID, ulCQSize);
    }

    return TRUE;
}

MCU0_DRAM_TEXT BOOL L0_NVMeCreateSQ(PCB_MGR pSlot)
{
    U32 ulCQID;
    U32 ulSQID;
    U32 ulSQSize;
    U32 ulSQPrio;
    U32 ulMemPhyCont;
    U32 ulSupported = FALSE;
    U32 ulStatusField;
    
    ulCQID = (pSlot->Ch->DW11 >> 16);
    ulSQID = (pSlot->Ch->DW10 & MSK_4F);
    ulSQSize = (pSlot->Ch->DW10 >> 16) + 1;
    ulMemPhyCont = (pSlot->Ch->DW11) & 1;

    /* 1. parameter check */
    if ((FALSE == ulMemPhyCont) && (TRUE == NVME_CAP_CQR(g_pNVMeCfgReg->bar.capl)))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(11 * 4, 0, NVME_SF_IVLD_FIELD);
    }
#ifndef SIM
    else if (0 != (pSlot->Ch->PRP1L & HPAGE_SIZE_MSK))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(24, 0, NVME_SF_IVLD_PRP_OFS);
    }
#endif
    else if ((ulCQID > NVME_NUMBER_OF_CQS) || CQ_IVLD(ulCQID))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(11 * 4 + 2, 0, NVME_SF_IVLD_CQ);
    }
    else if ((0 == ulCQID) || (0 == ulSQID) ||
        (ulSQID > NVME_NUMBER_OF_SQS) || SQ_VLD(ulSQID))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_QID);
    }
    else if ((ulSQSize < 2u)||(ulSQSize > (NVME_CAP_MQES(g_pNVMeCfgReg->bar.capl) + 1)))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4 + 2, 0, NVME_SF_IVLD_QSIZE);
    }
    else
    {
        ulSupported = TRUE;
    }
    
    if(FALSE == ulSupported)
    {
#ifndef SPEEDUP_UNH_IOL
        DBG_Printf("Create SQ(%d) Error(%d) in Slot(%d)!\n", ulSQID, ulStatusField, pSlot->SlotNum);
        DBG_Printf("CQID (%d), SQ Size (%d), Cont. (%d)!\n", ulCQID, ulSQSize, ulMemPhyCont);
#endif
        L0_NVMeCmdError(pSlot->SlotNum, ulStatusField);
        SIM_Getch();
    }
    else
    {
        /* 2. update firmware manager information */
        ulSQPrio = (pSlot->Ch->DW11 >> 1) & 3;
        gNvmeMgr.SQ[ulSQID].AdrL     =  pSlot->Ch->PRP1L;
        gNvmeMgr.SQ[ulSQID].AdrH     =  pSlot->Ch->PRP1H;
        gNvmeMgr.SQ[ulSQID].QID      = (U16)ulSQID;
        gNvmeMgr.SQ[ulSQID].BindCQID = (U8)ulCQID;
        gNvmeMgr.SQ[ulSQID].QSIZE    = (U16)ulSQSize;
        gNvmeMgr.SQ[ulSQID].QPRIO    = (U16)ulSQPrio;
        gNvmeMgr.SQ[ulSQID].PC       = ulMemPhyCont;
        gNvmeMgr.SQ[ulSQID].Valid    = TRUE;
        gNvmeMgr.SQ[ulSQID].SqHead   = 0;

        gNvmeMgr.IOSQCnt++;
        gNvmeMgr.ValidIOSQMap |= BIT(ulSQID);

        DBG_Printf("Create SQ:%d, bind CQ:%d, Size:%d\n", ulSQID, ulCQID, ulSQSize);

#ifdef AF_ENABLE
        /* 3. update hardware information */
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].BaseAddrL = pSlot->Ch->PRP1L;
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].BaseAddrH = pSlot->Ch->PRP1H;
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].Size      = ulSQSize - 1;
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].StepAddr  = sizeof(COMMAND_HEADER);
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].CQMaped   = (U8)ulCQID;
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].P         = PRIORITY_DEFAULT;
        g_pNVMeCfgExReg->SQCfgAttr[ulSQID].BM        = MAX_BM_IOSQ;
#endif
    }

    return TRUE;
}

    
MCU0_DRAM_TEXT BOOL L0_NVMeDeleteSQ(PCB_MGR pSlot)
{
    U32 ulSQID;
    U32 ulStatusField;
    U32 ulSlotNum;
    PCB_MGR pCurrSlot;
    SLOT_STATE ulSlotState; 
    BOOL bFinish;

    ulSQID = (pSlot->Ch->DW10 & MSK_4F);

    if (CMD_NEW == pSlot->CmdState)
    {
        /* 1. parameter check */
        if ((0 == ulSQID) || (ulSQID > NVME_NUMBER_OF_SQS) || SQ_IVLD(ulSQID))
        {
            ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_QID);
#ifndef SPEEDUP_UNH_IOL
            DBG_Printf("Delete SQ(%d) Error(%d) in Slot(%d)!\n", ulSQID, ulStatusField, pSlot->SlotNum);
#endif
            L0_NVMeCmdError(pSlot->SlotNum, ulStatusField);
            SIM_Getch();
            bFinish = TRUE;
        }
        else
        {
            pSlot->CmdState = CMD_HANDLING;
            bFinish = FALSE;
        }
    }

    else
    {
        bFinish = TRUE;
        for (ulSlotNum = 0; ulSlotNum < MAX_SLOT_NUM; ulSlotNum++)
        {
            pCurrSlot = GET_SLOT_MGR(ulSlotNum);
            if (pCurrSlot->SQID == ulSQID)
            {
                ulSlotState = HAL_HCTGetCST(ulSlotNum);
    
#ifdef AF_ENABLE
                if (ulSlotState == SLOT_WAITING_SQENTRY )
                {
                    DBG_Printf("SlotState(%d) should not be READ_CH in AF_ENABLE Branch\n", ulSlotState);
                    DBG_Getch();
                }
#else
                if (ulSlotState == SLOT_AFSQE_RDY)
                {
                    DBG_Printf("SlotState(%d) should not be AF_RECEIVED in NON_AF_ENABLE branch\n", ulSlotState);
                    DBG_Getch();
                }
#endif
                switch (ulSlotState)
                {
                    case SLOT_IDLE:
                        break;
                        
                    case SLOT_WAITING_SQENTRY:
                    case SLOT_WAITING_PRPLIST:
                    case SLOT_TRIGGER_WBQ:
                    case SLOT_AFSQE_RDY:
                        DBG_Printf("Slot(%d) SLOT_AFSQE_RDY | SLOT_WAITING_SQENTRY | PRP | TRIGGER_BWQ(Status:%d)\n", ulSlotNum, ulSlotState);
                        bFinish = FALSE; //Should be waiting!
                        break;
                                 
                    case SLOT_SQENTRY_RDY:
                    case SLOT_PRPLIST_RDY:
                        DBG_Printf("Slot(%d)  SLOT_SQENTRY_RDY | NEW_CMD_READY(Status:%d)\n", ulSlotNum, ulSlotState);
                        bFinish = FALSE; //Should be waiting!
                        L0_NVMeForceCompleteCmd(pCurrSlot, ((INVALID_4F << 16) | NVME_SF_ABORT_BY_SQ_DEL));
                        break;

                    default:
                        DBG_Printf("Find slot %d state %d error!\n", pCurrSlot->SlotNum, ulSlotState);
                        SIM_Getch();
                        break;
                }
            }
        }

#ifdef AF_ENABLE
        if (g_pNVMeCfgExReg->SQCfgAttr[ulSQID].HWRP != g_pNVMeCfgReg->doorbell[ulSQID].sq_tail)
        {
            bFinish = FALSE;
        }
#endif
        if (TRUE == bFinish)
        {
            // 2. update firmware manager information 
            gNvmeMgr.IOSQCnt--;    
            gNvmeMgr.SQ[ulSQID].Valid = FALSE;
            gNvmeMgr.SQ[ulSQID].SqHead = 0;
            gNvmeMgr.ValidIOSQMap &= ~(BIT(ulSQID));
    
            // 3. update hardware status 
#ifdef AF_ENABLE
            L0_NVMeDisableSQAF(ulSQID);
#endif
            CLR_HW_SQ_PTR(ulSQID);
            
            DBG_Printf("Deleted SQ: %d\n", ulSQID);
        }
    }

    return bFinish;
}

MCU0_DRAM_TEXT BOOL L0_NVMeDeleteCQ(PCB_MGR pSlot)
{
    U32 ulSQID;
    U32 ulCQID;
    U32 ulStatusField;
    BOOL bSupported = FALSE;

    ulCQID = (pSlot->Ch->DW10 & MSK_4F);

    /* 1. parameter check */
    if ((0 == ulCQID) || (ulCQID > NVME_NUMBER_OF_CQS) || CQ_IVLD(ulCQID))
    {
        ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_QID);
    }
    else
    {
        for(ulSQID = 1; ulSQID <= NVME_NUMBER_OF_SQS; ulSQID++)
        {
            if(SQ_VLD(ulSQID) && (BIND_CQID(ulSQID) == ulCQID))
            {
                ulStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_Q_DELETION);
                break;
            }
        }

        if (ulSQID > NVME_NUMBER_OF_SQS)
        {
            bSupported = TRUE;
        }
    }

    if(FALSE == bSupported)
    {
#ifndef SPEEDUP_UNH_IOL
        DBG_Printf("Delete CQ(%d) Error(%d)!\n", ulCQID, ulStatusField);
#endif
        L0_NVMeCmdError(pSlot->SlotNum, ulStatusField);
        SIM_Getch();
    }
    else
    {
        /* 2. update firmware manager information */
        gNvmeMgr.CQ[ulCQID].Valid = FALSE;
        gNvmeMgr.CQ[ulCQID].CqTail = 0;

        /* 3. update hardware status */
        g_pNVMeCfgReg->cq_clr = BIT(ulCQID);
        g_pNVMeCfgReg->msix_vector &= ~(MSK_1F << ((ulCQID - 1) << 2));

        /* 4. clear interrupt status */
        g_pNVMeCfgReg->int_status = BIT(ulCQID);
        g_pNVMeCfgReg->intvec_int_status = BIT(gNvmeMgr.CQ[ulCQID].IV);

        DBG_Printf("Deleted CQ: %d\n", ulCQID);
    }

    return TRUE;
}

MCU0_DRAM_TEXT BOOL L0_NVMeIdentify(PCB_MGR pSlot)
{
    BOOL bFinish;
    U32 ulCNSValue;
    U32 ulErrorStatusField;
    BOOL bCNSSupported = FALSE;
    BOOL bSupported = FALSE;
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;

    if (CMD_NEW == pSlot->CmdState)
    {
        ulCNSValue = (pSlot->Ch->DW10 & MSK_2F);
        switch (ulCNSValue)
        {
            case IDENTIFY_NAMESPACE_STRUCTURE:
                pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;
                if (1 != pSlot->Ch->NSID)
                {
                    /* Invalid Namespace ID (Broadcast value is also invalid here). */
                    DBG_Printf("Identify NameSpace ID Error(%d)!\n",pSlot->Ch->NSID);
                    ulErrorStatusField = NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_NS_FMT);
                }
                else
                {
                    pSlot->DataAddr = g_ulInfoIdfyPage + MCU0_NAMESPACE_IDENTIFY_OFFSET;
                    bCNSSupported = TRUE;
                }
                break;
                
            case IDENTIFY_CONTROLLER_STRUCTURE:
                pSlot->DataAddr = g_ulInfoIdfyPage;
                bCNSSupported = TRUE;
                break;
                
            case IDENTIFY_ACTIVE_NAMESPACE_LIST:
                pSlot->DataAddr = g_ulInfoIdfyPage + MCU0_NAMESPACE_LIST_OFFSET;
                if (0 == pSlot->Ch->NSID)
                {
                    *(volatile U32 *)pSlot->DataAddr = 1;
                    bCNSSupported = TRUE;
                }
                else if (pSlot->Ch->NSID >= 0xFFFFFFFE)
                {
                    /* Broadcast value is also invalid here. */
                    DBG_Printf("Identify NameSpace ID Error(%d)!\n",pSlot->Ch->NSID);
                    ulErrorStatusField = NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_NS_FMT);
                }
                else
                {
                    *(volatile U32 *)pSlot->DataAddr = 0;
                    bCNSSupported = TRUE;
                }
                break;

            default:
                ulErrorStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FIELD);
                DBG_Printf("Invalid CNS field setting: %d\n", ulCNSValue);
                break;
        }

        if (TRUE == bCNSSupported)
        {
            if (PRPLISTSTS_INVALIDADDR == L0_CheckNeedToFetchPRPList(pSlot))
            {
                DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pSlot->PRPTable[1].ulDW0, pSlot->PRPTable[1].ulDW1);
                ulErrorStatusField = NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS);
            }

            else
            {
                bSupported = TRUE;
                pSlot->HasDataXfer = TRUE;
                pSlot->DataXferState = NVME_DATAXFER_START;
                pSlot->IsWriteDir = TRUE;
                pSlot->TotalRemainingBytes = HPAGE_SIZE;
                pSlot->CmdState = CMD_HANDLING;
            }
        }

        if (FALSE == bSupported)
        {
            L0_NVMeCmdError(pSlot->SlotNum, ulErrorStatusField);
            pSlot->CmdState = CMD_COMPLETED;
            bFinish = TRUE; 
        }

        else
        {
            bFinish = L0_NVMeXferData(pSlot);
        }
    }

    else
    {
        bFinish = L0_NVMeXferData(pSlot);
    }

    return bFinish;
}

MCU0_DRAM_TEXT BOOL L0_IsNeedAbort(U16 usAbortSQID,U16 usAbortCID)
{
    U32 ulSlotNum;
    PCB_MGR pCurrSlot;
    BOOL bAbort = FALSE;

    for(ulSlotNum = 0; ulSlotNum < MAX_SLOT_NUM; ulSlotNum++)
    {
        pCurrSlot = GET_SLOT_MGR(ulSlotNum);
        if(pCurrSlot->SQID == usAbortSQID && pCurrSlot->Ch->CID == usAbortCID)
        {
            if(CMD_NEW == pCurrSlot->CmdState)
            {
                DBG_Printf("Need abort Cmd SQ %d SQIndex 0x%x CID 0x%x,Slot 0x%x\n",
                    usAbortSQID,pCurrSlot->SQEntryIndex,usAbortCID,ulSlotNum);

                pCurrSlot->bAbort = TRUE;
                bAbort = TRUE;
                break;
            }
        }
    }

    return bAbort;
}

MCU0_DRAM_TEXT BOOL L0_NVMeAbort(PCB_MGR pSlot)
{
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;
    U16 usAbortSQID;
    U32 usAbortCID;
    static U32 s_ulAbortCmdLimitTimerEnMarker;
    static U32 s_ulAbortCnt = 0;

    pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;
    
    usAbortSQID = pSlot->Ch->DW10 & 0xFFFF;
    usAbortCID = (pSlot->Ch->DW10 >> 16) & 0xFFFF;

    if(TRUE == L0_IsNeedAbort(usAbortSQID,usAbortCID))
    {
        pSlot->CSPC = 0;//aborted
    }
    else
    {
        pSlot->CSPC = 1;//not aborted
    }

    //Abort Cmd limit exceeded Check
    if (FALSE == s_ulAbortCmdLimitTimerEnMarker)
    {
#ifndef SIM
        HAL_PMClearTimeOutFlag(PMU_TIMER0);
#ifdef CALC_RINGOSC_CLK
        HAL_PMStartTimer(PMU_TIMER0,  10u * HAL_PMGetTimerTickPerMS());
#else
        HAL_PMStartTimer(PMU_TIMER0, PMU_TIMER_TICKCOUNT_1MS * 10u);
#endif
#endif
        s_ulAbortCmdLimitTimerEnMarker = TRUE;
        s_ulAbortCnt = 0;
    }

#ifndef SIM
    else if (TRUE == HAL_PMIsTimerTimedOut(PMU_TIMER0))
    {
        s_ulAbortCmdLimitTimerEnMarker = FALSE;
    }
#endif

    else
    {
        s_ulAbortCnt++;
    }

    if (s_ulAbortCnt > (U32)pIDCTR->ACL)
    {
        DBG_Printf("L0_NVMeAbort, Abort Cmd limit exceeded. AbortCnt:%d\n",s_ulAbortCnt );
        pSlot->CSPC = 1;
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4 + 2, 0, NVME_SF_ABORT_CMD_LE));
    }
    
    return TRUE;
}

MCU0_DRAM_TEXT void L0_NVMeFeatureTblInit(void)
{
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;
    CCC_CONTROL* pCccControl;
    U32 ulRegValue;
    U32 i;

    /* Initializes temperature threshold table with default WCTEMP value in Identify Controller data. */
    COM_MemZero((U32 *)(&g_tNVMeTempThrldTable), (sizeof(NVME_TEMPERATURE_THRESHOLD_TABLE) >> DWORD_SIZE_BITS));
    g_tNVMeTempThrldTable.aSensorEntryArray[0].ulOverTempThrshld = pIDCTR->WCTEMP;
    g_tNVMeTempThrldTable.aSensorEntryArray[0].ulUnderTempThrshld = pIDCTR->CCTEMP - 100;
    g_tNVMeTempThrldTable.tDefaultCompositeEntry.ulOverTempThrshld = pIDCTR->WCTEMP;
    g_tNVMeTempThrldTable.tDefaultCompositeEntry.ulUnderTempThrshld = pIDCTR->CCTEMP - 100;

    /* Initializes feature table */
    for ( i = 0; i < FEATURE_NUM; i++)
    {
        gNvmeFeaTbl.tFea[i].uAttribute.ulDW0 = FEATURE_ATTR_CHANGEABLE;
        gNvmeFeaTbl.tFea[i].ulSavedVal = 0;
        gNvmeFeaTbl.tFea[i].ulCurVal = 0;
        gNvmeFeaTbl.tFea[i].ulDefVal= 0;
        gNvmeFeaTbl.tFea[i].ulSavedMarker = FALSE;
        
        //Init Feature Vendor Specific Default Value
        switch(gNvmeFIDMap[i])
        {
            case LBA_RANGE_TYPE:    
                gNvmeFeaTbl.tFea[i].uAttribute.ulChangeable = FALSE;   //UnSaveable & UnChangeable
                break;

            case NUMBER_OF_QUEUES:  
                gNvmeFeaTbl.tFea[i].ulDefVal = (((NVME_NUMBER_OF_CQS - 1) << 16) | (NVME_NUMBER_OF_SQS - 1));
                //FID:7 Can not use saveVal.
                gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulDefVal;        
                break;

            case INTERRUPT_COALESCING : 
                ulRegValue = *((volatile U32*)(REG_BASE_HOSTC + 0x0c));
                pCccControl = (CCC_CONTROL *)&ulRegValue;
                gNvmeFeaTbl.tFea[i].ulDefVal |= (pCccControl->TIME << 8);
                gNvmeFeaTbl.tFea[i].ulDefVal |= pCccControl->THR;
                if (gNvmeFeaTbl.tFea[i].ulSavedMarker == TRUE)
                {
                    gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulSavedVal;
                }
                else
                {
                    gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulDefVal;
                }

                break;

            case INTERRUPT_VECTOR_CONFIGURATION:
                gNvmeFeaTbl.tFea[i].ulDefVal = (*((volatile U32*)(REG_BASE_HOSTC + 0x0c)) & 0x01FF);
                if (gNvmeFeaTbl.tFea[i].ulSavedMarker == TRUE)
                {
                    gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulSavedVal;        
                }
                else
                {
                    gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulDefVal;
                }
                break;

            case ERROR_RECOVERY:
                gNvmeFeaTbl.tFea[i].uAttribute.ulNameSpec = TRUE;
                break;

            default:
                if ( gNvmeFeaTbl.tFea[i].ulSavedMarker == TRUE)
                {
                    gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulSavedVal;        
                }
                else
                {
                    gNvmeFeaTbl.tFea[i].ulCurVal = gNvmeFeaTbl.tFea[i].ulDefVal;
                }
                break;
        }
        
        //DBG_Printf("FID(%d)(Index:%d).Attr(%d)\n", gNvmeFIDMap[i], i, gNvmeFeaTbl.tFea[i].uAttribute.ulDW0);
    }
    
    return;
}

MCU0_DRAM_TEXT U32 L0_NVMeGetFIDMapIndex(U32 ulFID)
{
    U32 i;
    for (i = 0; i < FEATURE_NUM; i++)
    {
        if ( gNvmeFIDMap[i] == ulFID )
        {
            return i;
        }
    }
    DBG_Printf("Cannot find FID:%d in gNVMeFIDMap\n", ulFID);
    DBG_Getch();
    return FEATURE_NUM;
}

MCU0_DRAM_TEXT BOOL L0_NVMeGetFeatureItemCheck(PCB_MGR pSlot, U32 ulFIDIndex, U32* ulSEL)
{
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;
    
    pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;

    if(ulFIDIndex >= FEATURE_NUM)
    {
        DBG_Printf("ulFID(%d) >= FEATURE_NUM(%d) is invalid\n", ulFIDIndex, FEATURE_NUM);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FIELD));
        return FAIL;
    }

    // Select field Value Check. Abort Command and return Error Invalid Field.
    if (*ulSEL > 0x3)
    {
        DBG_Printf("ulSEL (%d) is reserved.\n", *ulSEL);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4 + 1, 0, NVME_SF_IVLD_FIELD));
        return FAIL;
    }

    //Namespace ID Check. Abort Command and return Error Invalid Namespace or Format.
    if ((FALSE != gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulNameSpec) && (1 != pSlot->Ch->NSID) && (INVALID_8F != pSlot->Ch->NSID))
    {
        DBG_Printf("ulFID(%d) is namespace specific and namespace ID is invalid. NSID:%d,\n",
            gNvmeFIDMap[ulFIDIndex], pSlot->Ch->NSID);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_NS_FMT));
        return FAIL;
    }

    //Namespace Specific Check. Abort Command and return Error Feature Not Namespace Specific.
    if ((0 != pSlot->Ch->NSID) && (FALSE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulNameSpec))
    {
        DBG_Printf("ulFID(%d) is not namespace specific. info:FID.Attr:%d, DW10:0x%x,\n",
            gNvmeFIDMap[ulFIDIndex], gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0, pSlot->Ch->DW10);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_FNNS));
        return FAIL;
    }

    if ( 0 == pIDCTR->ONCS.SupportSetFeatureSave)
    {
        //SEL field is not supported. Return CurValue
        *ulSEL = 0x00;
    }

    if ((0x02 == *ulSEL)
        &&((gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulSaveable == FALSE)
        || (gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedMarker == FALSE)))
    {
        //UnSaveable or No saved value, return DefVal
        *ulSEL = 0x01;
    }

    return SUCCESS;
}

MCU0_DRAM_TEXT U32 L0_NVMeGetFeatureItemCom(PCB_MGR pSlot, U32 ulFID, U32 ulSEL)
{
    U32 ulRst = 0;
    U32 ulFIDIndex;
    
    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    
    if (FAIL != L0_NVMeGetFeatureItemCheck(pSlot, ulFIDIndex, &ulSEL))
    {
        switch (ulSEL)
        {
            case 0x00:
                ulRst = gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal;
                //DBG_Printf("L0_NVMeGetFeatureItem Get CurVal.0x%x\n", ulRst);
                break;
            case 0x01:
                ulRst = gNvmeFeaTbl.tFea[ulFIDIndex].ulDefVal;
                //DBG_Printf("L0_NVMeGetFeatureItem Get DefVal.0x%x\n", ulRst);
                break;
            case 0x02:
                DBG_Printf("L0_NVMeGetFeatureItem ulSEL(0x%x) is not supported\n", ulSEL);
                break;
            case 0x03:
                ulRst = gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0;
                //DBG_Printf("L0_NVMeGetFeatureItem Get Attr.0x%x\n", ulRst);
                break;
            default:
                DBG_Printf("L0_NVMeGetFeatureItem ulSEL(0x%x) is invalid\n", ulSEL);
                break;
        }
    }

    return ulRst;
}

MCU0_DRAM_TEXT U32 L0_NVMeGetFeatureItemTempThrshld(PCB_MGR pSlot, U32 ulFID, U32 ulSEL)
{
    U32 ulFIDIndex;
    U32 ulParam;
    U32 ulTHSEL, ulTMPSEL;
    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    
    if (0x3 == ulSEL)
    {
        if (FALSE == L0_NVMeGetFeatureItemCheck(pSlot, ulFIDIndex, &ulSEL))
        {
            return 0;
        }
        else
        {
            return gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0;
        }
    }

    else
    {
        ulParam = pSlot->Ch->DW11;
        ulTHSEL = ((ulParam >> 20) & 0x3);
        
        if (ulTHSEL > 0x1)
        {
            DBG_Printf("Get Features: Temperature Threshold => Invalid Threshold Type Select %d\n", ulTHSEL);
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(11 * 4 + 2, 4, NVME_SF_IVLD_FIELD));
            return 0;
        }
        
        ulTMPSEL = ((ulParam >> 16) & MSK_1F);
        
        if (ulTMPSEL > 0x8)
        {
            DBG_Printf("Get Features: Temperature Threshold => Invalid Threshold Temperature Select %d\n", ulTMPSEL);
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(11 * 4 + 2, 0, NVME_SF_IVLD_FIELD));
            return 0;
        }
        
        if (FALSE == L0_NVMeGetFeatureItemCheck(pSlot, ulFIDIndex, &ulSEL))
        {
            return 0;
        }

        if (0 == ulSEL)
        {
            if (0 == ulTHSEL)
            {
                return g_tNVMeTempThrldTable.aSensorEntryArray[ulTMPSEL].ulOverTempThrshld;
            }
            else
            {
                return g_tNVMeTempThrldTable.aSensorEntryArray[ulTMPSEL].ulUnderTempThrshld;
            }
        }

        else
        {
            if (0 == ulTMPSEL)
            {
                if(0 == ulTHSEL)
                {
                    return g_tNVMeTempThrldTable.tDefaultCompositeEntry.ulOverTempThrshld;
                }
                else
                {
                    return g_tNVMeTempThrldTable.tDefaultCompositeEntry.ulUnderTempThrshld;
                }
            }

            else
            {
                return 0;
            }
        }
    }
}

MCU0_DRAM_TEXT U32 L0_NVMeGetFeatureItemIVC(PCB_MGR pSlot, U32 ulFID, U32 ulSEL, U32 DW11)
{
    U32 ulRst = 0;
    U32 ulFIDIndex;
    
    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    
    if (FAIL != L0_NVMeGetFeatureItemCheck(pSlot, ulFIDIndex, &ulSEL))
    {
        switch (ulSEL)
        {
            case 0x00:
                ulRst = DW11 & 0xffff;
                if (0 ==(gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal & (1<< (DW11&0xffff))))
                {
                    ulRst |= BIT(16);
                }
                //DBG_Printf("L0_NVMeGetFeatureItemIVC Get CurVal.0x%x\n", ulRst);
                break;
            case 0x01:
                ulRst = DW11 & 0xffff;
                if (0 ==(gNvmeFeaTbl.tFea[ulFIDIndex].ulDefVal & (1<< (DW11&0xffff))))
                {
                    ulRst |= BIT(16);
                }
                //DBG_Printf("L0_NVMeGetFeatureItemIVC Get DefVal.0x%x\n", ulRst);
                break;
            case 0x02:
                DBG_Printf("L0_NVMeGetFeatureItemIVC ulSEL(0x%x) is not supported\n", ulSEL);
                break;
            case 0x03:
                ulRst = gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0;
                //DBG_Printf("L0_NVMeGetFeatureItemIVC Get Attr.0x%x\n", ulRst);
                break;
            default:
                DBG_Printf("L0_NVMeGetFeatureItemIVC ulSEL(0x%x) is invalid\n", ulSEL);
                break;
        }
    }

    return ulRst;
}


MCU0_DRAM_TEXT BOOL L0_NVMeGetFeatures(PCB_MGR pSlot)
{
    BOOL bFinish = TRUE;
    U32 ulFID;
    U32 ulSEL;

    pSlot->CSPC = 0;
    ulFID = pSlot->Ch->DW10 & 0xFF;
    ulSEL = (pSlot->Ch->DW10 >> 8) & 0x07;

#ifndef SPEEDUP_UNH_IOL
    DBG_Printf("L0_NVMeGetFeatures, Slot:%d, DW10.FID:%d, DW10.SEL:%d\n", pSlot->SlotNum, ulFID, ulSEL);
#endif
    switch(ulFID)
    {
        case ARBITRATION:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;

        case POWER_MANAGEMENT:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;
            
        case TEMPERATURE_THRESHOLD:
            pSlot->CSPC = L0_NVMeGetFeatureItemTempThrshld(pSlot, ulFID, ulSEL);
            break;

        case ERROR_RECOVERY:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;

        case NUMBER_OF_QUEUES:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;

        case INTERRUPT_COALESCING:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;

        case INTERRUPT_VECTOR_CONFIGURATION:
            pSlot->CSPC = L0_NVMeGetFeatureItemIVC(pSlot, ulFID, ulSEL, pSlot->Ch->DW11);
            break;

        case WRITE_ATOMICITY:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;

        case ASYNCHRONOUS_EVENT_CONFIGURATION:
            pSlot->CSPC = L0_NVMeGetFeatureItemCom(pSlot, ulFID, ulSEL);
            break;

        case SOFTWARE_PROGRESS_MARKER:
        case VOLATILE_WRITE_CACHE:
        case LBA_RANGE_TYPE:
        default:
            DBG_Printf("Unknown or UnSupport Get Feature(0x%x)\n", ulFID);
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FIELD));
            break;
    }

    return bFinish;
}


//SetFeature Param Check
MCU0_DRAM_TEXT BOOL L0_NVMeSetFeatureItemCheck(PCB_MGR pSlot, U32 ulFIDIndex, U32 ulSave, U32 DW11)
{
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;

    pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;


    //DBG_Printf("L0_NVMeSetFeatureItemCheck.Info:ulFID:%d,ulFIDIndex:%d,ulSV:%d, FID.Attr:0x%x.\n", 
    //    gNvmeFIDMap[ulFIDIndex], ulFIDIndex, ulSV,gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0);
    if (ulFIDIndex >= FEATURE_NUM)
    {
        DBG_Printf("Fatal Error:ulFIDIndex(%d) >= FEATURE_NUM(%d) is invalid\n", ulFIDIndex, FEATURE_NUM);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FIELD));
        return FAIL;
    }

    
    //Namespace ID Check. Abort Command and return Error Invalid Namespace or Format.
    if ((FALSE != gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulNameSpec) && (1 != pSlot->Ch->NSID) && (INVALID_8F != pSlot->Ch->NSID))
    {
        DBG_Printf("ulFID(%d) is namespace specific and namespace ID is invalid. NSID:%d,\n",
            gNvmeFIDMap[ulFIDIndex], pSlot->Ch->NSID);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_NS_FMT));
        return FAIL;
    }

    //SV field is not supported. Abort Command and return Error SV Feature Identifer Not Saveable.
    if ((FALSE == pIDCTR->ONCS.SupportSetFeatureSave) && (TRUE == ulSave))
    {
        
        DBG_Printf("Set Features Save field is Unsupported. Info: DW10:0x%x, DW11:0x%x, ulFID:%d, ulSV:%d\n",
            pSlot->Ch->DW10, DW11, gNvmeFIDMap[ulFIDIndex], ulSave);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4 + 3, 7, NVME_SF_FINS));
        return FAIL;
    }


    //UnSaveable Check. Abort Command and return Error SV Feature Identifer Not Saveable.
    if ((TRUE == ulSave) && (FALSE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulSaveable))
    {    
        DBG_Printf("ulFID(%d) is UnSaveable. Info:FID.Attr:0x%x, DW10:0x%x\n",
            gNvmeFIDMap[ulFIDIndex], gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0, pSlot->Ch->DW10);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4 + 3, 7, NVME_SF_FINS));
        return FAIL;
    }

    //Unchangedable Check. Abort Command and return Error Feature UnChangeable.
    if (FALSE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulChangeable)
    {
        DBG_Printf("ulFID(%d) is unChangeabled. info:FID.Attr:%d, DW10:0x%x,\n",
            gNvmeFIDMap[ulFIDIndex], gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0, pSlot->Ch->DW10);    
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_FNC));
        return FAIL;
    }

    //Namespace Specific Check. Abort Command and return Error Feature Not Namespace Specific.
    if ((0 != pSlot->Ch->NSID) && (FALSE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulNameSpec))
    {
        DBG_Printf("ulFID(%d) is not namespace specific. info:FID.Attr:%d, DW10:0x%x,\n",
            gNvmeFIDMap[ulFIDIndex], gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulDW0, pSlot->Ch->DW10);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_FNNS));
        return FAIL;
    }

    return SUCCESS;
}

MCU0_DRAM_TEXT void L0_NVMeSetFeatureItemCom(PCB_MGR pSlot, U32 ulFID, U32 ulSave, U32 DW11)
{
    U32 ulFIDIndex;

    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    
    if (FALSE == L0_NVMeSetFeatureItemCheck(pSlot, ulFIDIndex, ulSave, DW11))
    {
        return;
    }
    
    //SaveVal Setting
    if ((TRUE == ulSave) && (TRUE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulSaveable))
    {
        DBG_Printf("ulFID(%d) ulFIDIndex(%d) Set SaveVal.Info:DW11:0x%x\n", gNvmeFIDMap[ulFIDIndex], ulFIDIndex, DW11);    
        gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedVal = DW11;
        gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedMarker = TRUE;
        return;   
    }

    //CurVal Setting
    gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal = DW11;
    return ;
}

//Handling Temperature Threshold
MCU0_DRAM_TEXT void L0_NVMeSetFeatureItemTempThrshld(PCB_MGR pSlot, U32 ulFID, U32 ulSave, U32 ulParam)
{
    U32 ulFIDIndex;
    U32 ulTHSEL, ulTMPSEL, ulTMPTH;

    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    ulTHSEL = ((ulParam >> 20) & 0x3);

    if (ulTHSEL > 0x01)
    {
        DBG_Printf("Set Features: Temperature Threshold => Invalid Threshold Type Select %d\n", ulTHSEL);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(11 * 4 + 2, 4, NVME_SF_IVLD_FIELD));
        return;
    }

    ulTMPSEL = ((ulParam >> 16) & MSK_1F);

    if ((0xF != ulTMPSEL) && (ulTMPSEL > 0x8))
    {
        DBG_Printf("Set Features: Temperature Threshold => Invalid Threshold Temperature Select %d\n", ulTMPSEL);
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(11 * 4 + 2, 0, NVME_SF_IVLD_FIELD));
        return;
    }

    if (FALSE == L0_NVMeSetFeatureItemCheck(pSlot, ulFIDIndex, ulSave, ulParam))
    {
        return;
    }

    ulTMPTH = (ulParam & MSK_4F);

    if (0xF == ulTMPSEL)
    {
        /* Modifies temperature threshold for all implemented temperature sensors. */
        if (0 == ulTHSEL)
        {
            for (ulTHSEL = 0; ulTHSEL < 9; ulTHSEL++)
            {
                g_tNVMeTempThrldTable.aSensorEntryArray[ulTHSEL].ulOverTempThrshld = ulTMPTH;
            }
        }
        else
        {
            for (ulTHSEL = 0; ulTHSEL < 9; ulTHSEL++)
            {
                g_tNVMeTempThrldTable.aSensorEntryArray[ulTHSEL].ulUnderTempThrshld = ulTMPTH;
            }
        }

        L0_NVMeUpdateCrtclWrng(TRUE);
    }

    else
    {
        /* Modifies temperature threshold for a dedicate temperature sensor. */
        if (0 == ulTHSEL)
        {
            g_tNVMeTempThrldTable.aSensorEntryArray[ulTMPSEL].ulOverTempThrshld = ulTMPTH;
        }
        else
        {
            g_tNVMeTempThrldTable.aSensorEntryArray[ulTMPSEL].ulUnderTempThrshld = ulTMPTH;
        }

        if (0 == ulTMPSEL)
        {
            L0_NVMeUpdateCrtclWrng(TRUE);
        }
    }

    return ;
}

// Handling Number of Queues
MCU0_DRAM_TEXT void L0_NVMeSetFeatureItemNOQ(PCB_MGR pSlot, U32 ulFID, U32 ulSV, U32 ulDW11)
{
    U32 ulFIDIndex;
    U32 ulNCQR, ulNSQR;
    U32 ulNCQA, ulNSQA;

    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    ulNCQR = (ulDW11 >> 16);
    ulNSQR = (ulDW11 & MSK_4F);

    if (INVALID_4F == ulNCQR)
    {
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(11 * 4 + 2, 0, NVME_SF_IVLD_FIELD));
        return;
    }

    else if (INVALID_4F == ulNSQR)
    {
        L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(11 * 4, 0, NVME_SF_IVLD_FIELD));
        return;
    }

    if( FAIL == L0_NVMeSetFeatureItemCheck(pSlot, ulFIDIndex, ulSV, ulDW11))
    {
        return;
    }

    ulNCQA = ((NVME_NUMBER_OF_CQS - 1) < ulNCQR) ? (NVME_NUMBER_OF_CQS - 1) : ulNCQR;
    ulNSQA = ((NVME_NUMBER_OF_SQS - 1) < ulNSQR) ? (NVME_NUMBER_OF_SQS - 1) : ulNSQR;
    ulDW11 = ((ulNCQA << 16) | ulNSQA);

    gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal = ulDW11;
    pSlot->CSPC = ulDW11;

    return;
}

//Handling INTERRUPT_COALESCING
MCU0_DRAM_TEXT void L0_NVMeSetFeatureItemIC(PCB_MGR pSlot, U32 ulFID, U32 ulSV, U32 DW11)
{
    CCC_CONTROL* pCccControl;
    U32 ulRegValue;
    U32 ulFIDIndex;

    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    
    if( FALSE == L0_NVMeSetFeatureItemCheck(pSlot, ulFIDIndex, ulSV, DW11))
    {
        return;
    }
    
    //SaveVal Setting
    if ((ulSV == TRUE) && (TRUE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulSaveable))
    {
        DBG_Printf("ulFID(%d) Set SaveVal.Info:DW11:0x%x\n", gNvmeFIDMap[ulFIDIndex], DW11);    
        gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedVal = DW11;
        gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedMarker = TRUE;
        return;   
    }

    //CurVal Setting
    gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal = DW11;
    ulRegValue = *((volatile U32*)(REG_BASE_HOSTC + 0x0c));
    pCccControl = (CCC_CONTROL *)&ulRegValue;
    pCccControl->TMSCL = 0x2;
    pCccControl->TIME = (DW11 & 0xff00)>>8;
    pCccControl->THR = DW11 & 0xff;
    *((volatile U32*)(REG_BASE_HOSTC + 0x0c)) = ulRegValue;

    return;
}


//Handling INTERRUPT_VECTOR_CONFIGURATION
MCU0_DRAM_TEXT void L0_NVMeSetFeatureItemIVC(PCB_MGR pSlot, U32 ulFID, U32 ulSV, U32 DW11)
{
    U32 ulFIDIndex;

    ulFIDIndex = L0_NVMeGetFIDMapIndex(ulFID);
    
    if( FALSE == L0_NVMeSetFeatureItemCheck(pSlot, ulFIDIndex, ulSV, DW11))
    {
        return;
    }

    //SaveVal Setting
    if ((ulSV == TRUE) && (TRUE == gNvmeFeaTbl.tFea[ulFIDIndex].uAttribute.ulSaveable))
    {
        DBG_Printf("ulFID(%d) Set SaveVal.Info:DW11:0x%x\n", gNvmeFIDMap[ulFIDIndex], DW11);
        if ( DW11 & BIT(16))
        {
            gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedVal &= ~(1 << (DW11&0xFFFF));
        }
        else
        {
            gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedVal |= (1 << (DW11&0xFFFF));
        }
        gNvmeFeaTbl.tFea[ulFIDIndex].ulSavedMarker = TRUE;
        return;   
    }

    //CurVal Setting
    if ( DW11 & BIT(16))
    {
        gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal &= ~(1 << (DW11&0xFFFF));
    }
    else
    {
        gNvmeFeaTbl.tFea[ulFIDIndex].ulCurVal |= (1 << (DW11&0xFFFF));
    }
    return ;
}

#ifdef AF_ENABLE
MCU0_DRAM_TEXT BOOL L0_NVMeSetFeatureARB(U32 DW11)
{
    typedef struct _ARB_DW11
    {
        U32 ulAB : 3;
        U32 ulRsv : 5;
        U32 ulLPW : 8;
        U32 ulMPW : 8;
        U32 ulHPW : 8;
    }ARB_DW11, *PARB_DW11;

    volatile NVMe_CC *pNVMeCC;
    U32 ulSQId;
    PARB_DW11 pArbDw11;
    U32 ulSumWeight;
    U32 ulWeight = 0;

    pArbDw11 = (PARB_DW11)&DW11;
    pNVMeCC = (volatile NVMe_CC *)&(g_pNVMeCfgReg->bar.cc);
    
    DBG_Printf("SetFeature Arbitration:DW11:0x%x\n", DW11);
        
    //Check CC.AMS 
    if(pNVMeCC->AMS == 0)
    {
        DBG_Printf("Warnning: CC.AMS(%d) claim that device do not enalbe WRR ARB, ignore ARB setting",
            pNVMeCC->AMS);
        return TRUE;
    }

    ulSumWeight = pArbDw11->ulLPW + pArbDw11->ulMPW + pArbDw11->ulHPW;
    //Since HW don't support a interface for weighted setting but the Max number command(BM).
    //FW will try to mix weighted and arbitration burst to set BM.
    for(ulSQId = 1; ulSQId < MAX_SQ_NUM; ulSQId++)
    {
        if(0 == (gNvmeMgr.ValidIOSQMap & BIT(ulSQId)))
        {
            continue;
        }

        switch (g_pNVMeCfgExReg->SQCfgAttr[ulSQId].P)
        {
            case PRIORITY_URGENT:
                g_pNVMeCfgExReg->SQCfgAttr[ulSQId].BM = pArbDw11->ulAB;
                break;
            case PRIORITY_HIGH:
                g_pNVMeCfgExReg->SQCfgAttr[ulSQId].BM = (pArbDw11->ulAB *  pArbDw11->ulHPW / ulSumWeight) ? (pArbDw11->ulAB * pArbDw11->ulHPW / ulSumWeight) : 1;
                break;
            case PRIORITY_MEDIUM:
                g_pNVMeCfgExReg->SQCfgAttr[ulSQId].BM = (pArbDw11->ulAB *  pArbDw11->ulMPW / ulSumWeight) ? (pArbDw11->ulAB * pArbDw11->ulMPW / ulSumWeight) : 1;
                break;
            case PRIORITY_LOW:
                g_pNVMeCfgExReg->SQCfgAttr[ulSQId].BM = (pArbDw11->ulAB *  pArbDw11->ulLPW / ulSumWeight) ? (pArbDw11->ulAB * pArbDw11->ulLPW / ulSumWeight) : 1;
                break;
            default:
                DBG_Printf("g_pNVMeCfgExReg->SQCfgAttr[%d].P[%d] is invalid\n", ulSQId, g_pNVMeCfgExReg->SQCfgAttr[ulSQId].P);
                DBG_Getch();
        }
    }
    
    return TRUE;
}
#endif

MCU0_DRAM_TEXT BOOL L0_NVMeSetFeatures(PCB_MGR pSlot)
{
    PTABLE *pPTable =  HAL_GetPTableAddr();

    U32 ulFID;
    U32 ulSave;
    U32 ulParam;

    ulFID   = (pSlot->Ch->DW10 & MSK_2F);
    ulSave  = (pSlot->Ch->DW10 >> 31);
    ulParam =  pSlot->Ch->DW11;

#ifndef SPEEDUP_UNH_IOL
    DBG_Printf("L0_NVMeSetFeatures, Slot:%d, DW10.FID:%d, DW10.SV:0x%x, DW10:0x%x, DW11:0x%x\n",
        pSlot->SlotNum, ulFID, ulSave, pSlot->Ch->DW10, ulParam);
#endif
    switch(ulFID)
    {
        case ARBITRATION:
            L0_NVMeSetFeatureItemCom(pSlot, ulFID, ulSave, ulParam);
#ifdef AF_ENABLE
            L0_NVMeSetFeatureARB(ulParam);
#endif
            break;
        case POWER_MANAGEMENT:
            L0_NVMeSetFeatureItemCom(pSlot, ulFID, ulSave, ulParam);
            break;
        
        case TEMPERATURE_THRESHOLD:
            L0_NVMeSetFeatureItemTempThrshld(pSlot, ulFID, ulSave, ulParam);
            break;

        case ERROR_RECOVERY:
            L0_NVMeSetFeatureItemCom(pSlot, ulFID, ulSave, ulParam);
            break;

        case NUMBER_OF_QUEUES:
            L0_NVMeSetFeatureItemNOQ(pSlot, ulFID, ulSave, ulParam);
            break;

        case INTERRUPT_COALESCING:
            L0_NVMeSetFeatureItemIC(pSlot, ulFID, ulSave, ulParam);
            break;

        case INTERRUPT_VECTOR_CONFIGURATION:
            L0_NVMeSetFeatureItemIVC(pSlot, ulFID, ulSave, ulParam);
            break;

        case WRITE_ATOMICITY:
            L0_NVMeSetFeatureItemCom(pSlot, ulFID, ulSave, ulParam);
            break;

        case ASYNCHRONOUS_EVENT_CONFIGURATION:
            L0_NVMeSetFeatureItemCom(pSlot, ulFID, ulSave, ulParam);
            gNvmeMgr.AsyncEvtCtrl.tHostConfig.ulCmdDW11 = ulParam;
            break;

        case LBA_RANGE_TYPE:
        case SOFTWARE_PROGRESS_MARKER:
        case VOLATILE_WRITE_CACHE:
        default:
            DBG_Printf("Unsupported Set Feature (0x%x)\n", ulFID );
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FIELD));
            break;
    }

    return TRUE;
}

MCU0_DRAM_TEXT void L0_NVMeFormatErrorLog(void)
{
    U32 ulCurrErrLogEntryNum, ulCurrCopyIndex;
    PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY pSourceErrLogEntry, pDestErrLogEntry, pBaseErrLogEntry;

    ulCurrErrLogEntryNum = L0_NVMeGetLastErrorLogIndex();

    pBaseErrLogEntry = (PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY)(g_ulATAGPLBuffStart + MCU0_LOG_ERROR_OFFSET);
    pDestErrLogEntry = (PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY)g_ulATARawBuffStart;

    /* Copies error log entries to RAW buffer with correct sequence. */
    for (ulCurrCopyIndex = 0; ulCurrCopyIndex < LOG_ERROR_MAX_ENTRY_NUM; ulCurrCopyIndex++)
    {
        pSourceErrLogEntry = pBaseErrLogEntry + ulCurrErrLogEntryNum;
        COM_MemCpy((U32 *)pDestErrLogEntry, (U32 *)pSourceErrLogEntry,
            (sizeof(ADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY) >> DWORD_SIZE_BITS));

        pDestErrLogEntry++;

        if (0 == ulCurrErrLogEntryNum)
        {
            ulCurrErrLogEntryNum = LOG_ERROR_MAX_ENTRY_NUM - 1;
        }
        else
        {
            ulCurrErrLogEntryNum--;
        }
    }

    return;
}

MCU0_DRAM_TEXT void L0_NVMeFillSmartData(PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY  pSmart)
{
    FW_RUNTIME_INFO *pFirmwareInfo;
    ULONGLONG ullTimeVal;
    ULONGLONG ullTotalRSecCnt, ullTotalWSecCnt;
    U32 ulLifePercentUsed = 0;
    U32 ulRemainSparePercent = 0;
    
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    PDEVICE_PARAM_PAGE pDeviceParamPage = (PDEVICE_PARAM_PAGE)g_ulDevParamAddr;

    //Update DevParamPage
    //1.Get HostInfo and DeviceParam from L1 
    L0_IssueAccessDevParamSCmd(0,(U32)g_ulDevParamAddr,(U32)GLBINFO_LOAD);
    L0_WaitForAllSCmdCpl(0);

    //2.Correct the device param
    if (0 == g_pBootParamTable->ulFlashPECycleVal)
    {
        /* Patch for dividing by zero problem when in LLF or MPT boot mode. */
        ulLifePercentUsed = 0;
    }

    else
    {
        ulLifePercentUsed = pDeviceParamPage->AvgEraseCount / TLC_BLK_CNT * 100
            / g_pBootParamTable->ulFlashPECycleVal;
    }
    
    ullTimeVal = pHostInfoPage->PowerOnMins + pHostInfoPage->PowerOnHours * 60ULL;

    if ((0 == pDeviceParamPage->AvailRsvdSpace) &&
        (0 == pDeviceParamPage->UsedRsvdBlockCnt))
    {
        /* Patch for dividing by zero problem when in LLF or MPT boot mode. */
        ulRemainSparePercent = 100;
    }

    else
    {
        ulRemainSparePercent = pDeviceParamPage->AvailRsvdSpace * 100 /
            (pDeviceParamPage->AvailRsvdSpace + pDeviceParamPage->UsedRsvdBlockCnt);
    }

    ullTotalRSecCnt = (((ULONGLONG)pHostInfoPage->TotalLBAReadHigh) << 32) + pHostInfoPage->TotalLBAReadLow;
    ullTotalRSecCnt /= 1000;
    ullTotalWSecCnt = (((ULONGLONG)pHostInfoPage->TotalLBAWrittenHigh) << 32) + pHostInfoPage->TotalLBAWrittenLow;
    ullTotalWSecCnt /= 1000;
    
    pSmart->Temperature = pDeviceParamPage->SYSTemperature + 273;
    pSmart->AvailableSpare = ulRemainSparePercent;
    pSmart->AvailableSpareThreshold = 10;
    pSmart->PercentageUsed = (ulLifePercentUsed > 255 )? 255 : ulLifePercentUsed;
    pSmart->DataUnitsRead.Lower.DW0 = (U32)ullTotalRSecCnt;
    pSmart->DataUnitsRead.Lower.DW1 =  ullTotalRSecCnt >> 32;
    pSmart->DataUnitsRead.Upper.DW0 = 0;
    pSmart->DataUnitsRead.Upper.DW1 = 0;
    pSmart->DataUnitsWritten.Lower.DW0 = (U32)ullTotalWSecCnt;
    pSmart->DataUnitsWritten.Lower.DW1 = ullTotalWSecCnt >> 32;
    pSmart->DataUnitsWritten.Upper.DW0 = 0;
    pSmart->DataUnitsWritten.Upper.DW1 = 0;
    pSmart->HostReadCommands.Lower.DW0 = pHostInfoPage->NvmeHostReadCntDW0;
    pSmart->HostReadCommands.Lower.DW1 = pHostInfoPage->NvmeHostReadCntDW1;
    pSmart->HostReadCommands.Upper.DW0 = pHostInfoPage->NvmeHostReadCntDW2;
    pSmart->HostReadCommands.Upper.DW1 = pHostInfoPage->NvmeHostReadCntDW3;
    pSmart->HostWriteCommands.Lower.DW0 = pHostInfoPage->NvmeHostWriteCntDW0;
    pSmart->HostWriteCommands.Lower.DW1 = pHostInfoPage->NvmeHostWriteCntDW1;
    pSmart->HostWriteCommands.Upper.DW0 = pHostInfoPage->NvmeHostWriteCntDW2;
    pSmart->HostWriteCommands.Upper.DW1 = pHostInfoPage->NvmeHostWriteCntDW3;
    pSmart->ControllerBusyTime.Lower.DW0 = (U32)ullTimeVal;
    pSmart->ControllerBusyTime.Lower.DW1 = ullTimeVal >> 32;
    pSmart->ControllerBusyTime.Upper.DW0 = 0;
    pSmart->ControllerBusyTime.Upper.DW1 = 0;
    pSmart->PowerCycles.Lower.DW0 = pDeviceParamPage->PowerCycleCnt;
    pSmart->PowerCycles.Lower.DW1 = 0;
    pSmart->PowerCycles.Upper.DW0 = 0;
    pSmart->PowerCycles.Upper.DW1 = 0;
    pSmart->PowerOnHours.Lower.DW0 = pHostInfoPage->PowerOnHours;
    pSmart->PowerOnHours.Lower.DW1 = 0;
    pSmart->PowerOnHours.Upper.DW0 = 0;
    pSmart->PowerOnHours.Upper.DW1 = 0;
    pSmart->UnsafeShutdowns.Lower.DW0 = pDeviceParamPage->SYSUnSafeShutdownCnt;
    pSmart->UnsafeShutdowns.Lower.DW1 = 0;
    pSmart->UnsafeShutdowns.Upper.DW0 = 0;
    pSmart->UnsafeShutdowns.Upper.DW1 = 0;
    pSmart->MediaErrors.Lower.DW0 = pDeviceParamPage->SYSUECCCnt;
    pSmart->MediaErrors.Lower.DW1 = 0;
    pSmart->MediaErrors.Upper.DW0 = 0;
    pSmart->MediaErrors.Upper.DW1 = 0;
    pSmart->NumberofErrorInformationLogEntries.Lower.DW0 = pHostInfoPage->NvmeErrorCntDW0;
    pSmart->NumberofErrorInformationLogEntries.Lower.DW1 = pHostInfoPage->NvmeErrorCntDW1;
    pSmart->NumberofErrorInformationLogEntries.Upper.DW0 = 0;
    pSmart->NumberofErrorInformationLogEntries.Upper.DW1 = 0;

    L0_NVMeUpdateCrtclWrng(FALSE);

    pFirmwareInfo = (FW_RUNTIME_INFO *)(((U32)pSmart) + 388);
    L0_FillFwRuntimeInfo(pFirmwareInfo);
    
    return;
}

MCU0_DRAM_TEXT BOOL L0_NVMeGetLogPage(PCB_MGR pSlot)
{
    BOOL bFinish;
    U32 ulLogID;
    U32 ulNumDword;
    U32 ulNSID;
    U32 ulErrorStatusField;
    U32 ulReqDataLenInByte;
    U32 ulLogPageOffsetLower, ulLogPageOffsetUpper;
    U32 ulLogLenInByte;
    BOOL bRetainAsyncEvent;
    BOOL bLogIDSupported;
    BOOL bSupported = FALSE;

    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;
    PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY  pSmart;

    if (CMD_NEW == pSlot->CmdState)
    {
        ulLogID = (pSlot->Ch->DW10 & MSK_2F);
        bRetainAsyncEvent = (pSlot->Ch->DW10 >> 15) & 1;
        ulNumDword = (pSlot->Ch->DW11 << 16) | (pSlot->Ch->DW10 >> 16);
        ulReqDataLenInByte = ((ulNumDword + 1) << DWORD_SIZE_BITS);
        ulLogPageOffsetLower = pSlot->Ch->DW12 & ~DWORD_SIZE_MSK;
        ulLogPageOffsetUpper = pSlot->Ch->DW13;
        ulNSID = pSlot->Ch->NSID;
        pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;
        DBG_Printf("GetLogPage: Namespace = %d, LogID = %d, NumDW = %d, LogPageOffset = 0x%x:0x%x\n",
            ulNSID, ulLogID, ulNumDword, ulLogPageOffsetUpper, ulLogPageOffsetLower);

        /* 1. Checks data transfer length. */
        if ((1u << (pIDCTR->MDTS + HPAGE_SIZE_BITS - DWORD_SIZE_BITS)) <= ulNumDword)
        {
            /* Invalid data transfer length. */
            DBG_Printf("GetLogPage Data Length Error (%d)!\n", ulReqDataLenInByte);
            ulErrorStatusField = NVME_ERRLOG_COMPRESS(10 * 4 + 2, 0, NVME_SF_IVLD_FIELD);
        }

        /* 2. Checks Namespace ID. */
        else if ((0 != pSlot->Ch->NSID) &&
            (1 != pSlot->Ch->NSID) &&
            (INVALID_8F != pSlot->Ch->NSID))
        {
            /* Invalid Namespace ID. */
            DBG_Printf("GetLogPage NameSpace ID not 0 or broadcast (%d)!\n",pSlot->Ch->NSID);
            ulErrorStatusField = NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_FIELD);
        }

        /* 3. Checks Log Id and Log page offset, PRP offset as well. */
        else
        {
            bLogIDSupported = TRUE;

            switch (ulLogID)
            {
                case LID_ERROR:
                    DBG_Printf("Get log page: ERROR log\n");
                    pSlot->DataAddr = g_ulATARawBuffStart;
                    ulLogLenInByte = LOG_ERROR_LEN;
                    L0_NVMeFormatErrorLog();
                    break;

                case LID_SMART:
                    DBG_Printf("Get log page: SMART log\n");
                    pSlot->DataAddr = g_ulATAGPLBuffStart + MCU0_LOG_SMART_OFFSET;
                    pSmart = (PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY)pSlot->DataAddr;
                    L0_NVMeFillSmartData(pSmart);
                    ulLogLenInByte = sizeof(ADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY);
                    break;

                case LID_FW_SLOT_INFO:
                    DBG_Printf("Get log page: FW_SLOT_INFO log\n");
                    pSlot->DataAddr = g_ulATAGPLBuffStart + MCU0_LOG_FW_SLOT_INFO_OFFSET;
                    ulLogLenInByte = sizeof(ADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY);
                    break;

                case LID_RESVTN_NOTIFICATION:
                    DBG_Printf("Get log page: RSV_NOTIFY log\n");
                    pSlot->DataAddr = g_ulATAGPLBuffStart + MCU0_LOG_RSV_NOTIFY_OFFSET;
                    ulLogLenInByte = SEC_SIZE;
                    break;

                default:
                    DBG_Printf("Get Log Page Error: LogID = %d\n", ulLogID);
                    ulErrorStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_LOG_PAGE);
                    bLogIDSupported = FALSE;
                    break;
            }

            if (TRUE == bLogIDSupported)
            {
                if ((0 != ulLogPageOffsetUpper) || (ulLogPageOffsetLower >= ulLogLenInByte))
                {
                    DBG_Printf("Log page offset overflow!\n");
                    ulErrorStatusField = NVME_ERRLOG_COMPRESS(12 * 4, 2, NVME_SF_IVLD_FIELD);
                }
                else if (PRPLISTSTS_INVALIDADDR == L0_CheckNeedToFetchPRPList(pSlot))
                {
                    /* PRP list address is invalid. */
                    DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pSlot->PRPTable[1].ulDW0, pSlot->PRPTable[1].ulDW1);
                    ulErrorStatusField = NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS);
                }
                else
                {
                    pSlot->DataAddr += ulLogPageOffsetLower;
                    ulLogLenInByte -= ulLogPageOffsetLower;
                    bSupported = TRUE;
                }
            }
        }

        if (TRUE == bSupported)
        {
            pSlot->HasDataXfer = TRUE;
            pSlot->IsWriteDir = TRUE;
            pSlot->CmdState = CMD_HANDLING;

            if (ulLogLenInByte < ulReqDataLenInByte)
            {
                pSlot->TotalRemainingBytes = ulLogLenInByte;
            }
            else
            {
                pSlot->TotalRemainingBytes = ulReqDataLenInByte;
            }

            /* Clears corresponding Async Event. */
            if (0 == bRetainAsyncEvent)
            {
                L0_NVMeClearAsyncEvent(ulLogID);
            }

            bFinish = L0_NVMeXferData(pSlot);
        }
        else
        {
            L0_NVMeCmdError(pSlot->SlotNum, ulErrorStatusField);
            //SIM_Getch();
            bFinish = TRUE; 
        }
    }
        
    else
    {
        bFinish = L0_NVMeXferData(pSlot);
    }
    
    return bFinish;
}

#if 0
MCU0_DRAM_TEXT BOOL L0_NVMeAsyncEvent(PCB_MGR pSlot)
{
    PNVME_WBQ pWBQ;
    
    gNvmeMgr.AsynEvtMgr.OutStandingCnt++;

    if (gNvmeMgr.AsynEvtMgr.OutStandingCnt > MAX_ASYN_EVEVNT_NUM)
    {
        gNvmeMgr.AsynEvtMgr.OutStandingCnt--;
        DBG_Printf("Async count exceeded!\n");
        L0_NVMeForceCompleteCmd(pSlot, NVME_SF_AE_REQ_LE);
        return TRUE;
    }
    
    pWBQ = &gNvmeMgr.AsynEvtMgr.Queue[gNvmeMgr.AsynEvtMgr.Wptr++];

    if(gNvmeMgr.AsynEvtMgr.Wptr == MAX_ASYN_EVEVNT_NUM)
    {
        gNvmeMgr.AsynEvtMgr.Wptr = 0;
    }

    COM_MemZero( (U32*)pWBQ, sizeof( NVME_WBQ ) / sizeof( U32 ) );
    pWBQ->Id = pSlot->SlotNum;
    pWBQ->Cqid = pSlot->CQID;
    pWBQ->Sqid = pSlot->SQID;
    pWBQ->StatusF = 0;
    pWBQ->CmdID = pSlot->Ch->CID;
    pWBQ->CmdSpec = pSlot->CSPC;
    pWBQ->NST = SLOT_IDLE;
    pWBQ->Update = TRUE;
    pWBQ->R3 = gNvmeMgr.CQ[pSlot->CQID].IEN;
    pWBQ->Rf = 0;
    pWBQ->Last = TRUE;
    pWBQ->WSGE = 0;

#ifdef AF_ENABLE
    L0_UpdateSQHead(pSlot);
#else
    if ( 0 == pSlot->SQID )
    {
        L0_UpdateASQHead(pSlot);
    }
    else
    {
        L0_UpdateIOSQHead(pSlot);
    }
#endif

    
    pSlot->CmdState = CMD_COMPLETED;
    HAL_HCTSetCST(pSlot->SlotNum, SLOT_IDLE);

    HAL_HCmdTimerStop(pSlot->SlotNum);

    DBG_Printf("NVME ASYN EVENT REQ PROCESSED\n");
    return TRUE;
}
#else
MCU0_DRAM_TEXT void L0_NVMeAsyncEvent(PCB_MGR pSlot)
{
    /* Checks outstanding Async Event request count. */
    if (MAX_ASYN_EVENT_NUM == gNvmeMgr.AsyncEvtCtrl.ulOutstdReqCount)
    {
        /* 1. Maximum outstanding request number exceeded. */
        DBG_Printf("Async Event request slot %d count exceeded!\n", pSlot->SlotNum);
        L0_NVMeForceCompleteCmd(pSlot, ((INVALID_4F << 16) | NVME_SF_AE_REQ_LE));
    }

    else if (TRUE == gNvmeMgr.AsyncEvtCtrl.ulEventPendingStatus)
    {
        /* 2. We have a pending event due to lacking of outstanding request.
                    So we report it immediately. */
        DBG_Printf("NVME ASYN EVENT Request slot %d reported immediately.\n", pSlot->SlotNum);
        L0_NVMeApplyStashedAsyncEvent(pSlot);
    }

    else
    {
        /* 3. Records the new outstanding request and preserves the command slot/CST. */
        HAL_HCmdTimerStop(pSlot->SlotNum);
        pSlot->CmdState = CMD_WAITING;
        HAL_HCTSetCST(pSlot->SlotNum, SLOT_ASYNC_EVT_RESV);
#ifdef AF_ENABLE
        L0_UpdateSQHead(pSlot);
#else
        L0_UpdateASQHead(pSlot);
#endif
        gNvmeMgr.AsyncEvtCtrl.ulOutstdReqCount++;
        DBG_Printf("NVME ASYN EVENT Request slot %d buffered.\n", pSlot->SlotNum);
    }
    return;
}
#endif

MCU0_DRAM_TEXT LOCAL BOOL L0_NVMeFwCommitSaveFw(U32 ulFwSlot)
{
    BOOL ulRst = FAIL;
    pSaveFW pSaveFWFunc;
    PADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY pFW;

    //Fw has already downloaded in g_tFwUpdate.ulFwBassAddr via Download Cmd
    //Call the Bootloader to save Fw into flash and update the FW Log Page
    pSaveFWFunc = (pSaveFW)HAL_GetFTableFuncAddr(SAVE_FIRWARE);

#ifndef SIM
    COM_MemCpy( (U32*)DRAM_DATA_BUFF_MCU1_BASE,
                (U32*)g_tFwUpdate.ulFwBaseAddr,
                g_tFwUpdate.ulFwSize/sizeof(U32));
    ulRst = pSaveFWFunc(ulFwSlot-2,DRAM_DATA_BUFF_MCU1_BASE);

#else
    ulRst = TRUE;
#endif // SIM

    if (TRUE == ulRst)
    {
        g_tFwUpdate.ulDldSize = 0; //Set to 0 to enable a new download.
        //Update FW Log Page
        pFW = (PADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY)(g_ulATAGPLBuffStart + MCU0_LOG_FW_SLOT_INFO_OFFSET);
        //FIX ME:Revision definition
        pFW->Frs[ulFwSlot][0] = '1';

        DBG_Printf("SaveFW Done.\r\n");
        
        return TRUE;
    }
    else
    {
        return FAIL;
    }

    return FAIL;    //Return FAIL in default
}

MCU0_DRAM_TEXT LOCAL BOOL L0_NVMeFwCommitActFw(U32 ulFwSlot)
{
    BOOL ulRst = FAIL;
    //pActiveFW pActiveFWFunc;
    PADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY pFW;

    //pActiveFWFunc = (pActiveFW)HAL_GetFTableFuncAddr(ACTIVE_FIRWARE);
#ifndef SIM
    ulRst = TRUE;
#else
    ulRst = TRUE;
#endif // !SIM

    if (TRUE == ulRst)
    {
        //Update FW Log Page
        pFW = (PADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY)(g_ulATAGPLBuffStart + MCU0_LOG_FW_SLOT_INFO_OFFSET);
        pFW->AFI.NextTimetoActive = ulFwSlot;
        //TODO: Initial AFI.FirmwareSlot in initialization schedule

        DBG_Printf("ActiveFw Done\r\n");
        return TRUE;
    }
    else
    {
        return FAIL;
    }

    return FAIL;    //Return FAIL in default
}

MCU0_DRAM_TEXT LOCAL BOOL L0_NVMeSaveFwImage(U32 ulFwSlot)
{
    BOOL ulRst = FAIL;

    ulRst = L0_NVMeFwCommitSaveFw(ulFwSlot);

    return ulRst;
}

MCU0_DRAM_TEXT BOOL L0_NVMeFwCommit(PCB_MGR pSlot)
{
    U32 ulFwSlot;
    U32 ulCommitAction;
    U32 ulStatus = NVME_SF_SUCCESS;
    pRunFW pRunFWFunc;
    BOOL bStsFlag = FALSE;

    #define CA_FLUSH_NOT_ACTIVE     0
    #define CA_FLUSH_ACTIVE_NEXTRST 1
    #define CA_ACTIVE_NEXTRST       2
    #define CA_ACTIVE_IMMEDIATELY   3

    ulFwSlot = pSlot->Ch->DW10 & 0x7;
    ulCommitAction = (pSlot->Ch->DW10 >> 3) & 0x7;

    if ((ulFwSlot > 2)||(0 == ulFwSlot))
    {
        ulStatus = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FW_SLOT);
    }

    if (NVME_SF_SUCCESS == ulStatus)
    {
        switch(ulCommitAction)
        {
            case CA_FLUSH_NOT_ACTIVE:
            {
                if (1 == ulFwSlot)
                {
                    ulStatus = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FW_SLOT);
                }
                else if(FALSE == L0_FwUpgradeCheckImgValid()) /* firmware CRC check error */
                {
                    ulStatus = ((INVALID_4F << 16) | NVME_SF_IVLD_FW_IMAGE);
                    DBG_Printf("FwCrcChk Fail.\r\n");
                }
                else
                {
                    L0_ForceAllSubSysIdle();
                    L0_SubSystemOnlineShutdown(FALSE);
                    if (FAIL == L0_NVMeSaveFwImage(ulFwSlot) )
                    {
                        ulStatus = ((INVALID_4F << 16) | NVME_SF_INTERNAL_ERROR);
                        //TODO: Send AsyncEvent
                    }
                    L0_SubSystemOnlineReboot();
                }
            }
            break;

            case CA_FLUSH_ACTIVE_NEXTRST:
            {
                if (1 == ulFwSlot)
                {
                    ulStatus = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FW_SLOT);
                }
                else if(FALSE == L0_FwUpgradeCheckImgValid()) /* firmware CRC check error */                
                {
                    ulStatus = ((INVALID_4F << 16) | NVME_SF_IVLD_FW_IMAGE);
                    DBG_Printf("FwCrcChk Fail.\r\n");
                }
                else
                {
                    L0_ForceAllSubSysIdle();
                    L0_SubSystemOnlineShutdown(FALSE);
                    bStsFlag = L0_NVMeSaveFwImage(ulFwSlot);
                    
                    if (TRUE == bStsFlag)
                    {
                        bStsFlag &= L0_NVMeFwCommitActFw(ulFwSlot);
                    }
                    if (FAIL == bStsFlag)
                    {
                        ulStatus = ((INVALID_4F << 16) | NVME_SF_INTERNAL_ERROR);
                        //TODO: Send AsyncEvent
                    }
                    L0_SubSystemOnlineReboot();
                }
            }
            break;
            
            case CA_ACTIVE_NEXTRST:
            {
                L0_ForceAllSubSysIdle();
                L0_SubSystemOnlineShutdown(FALSE);
                if (FAIL == L0_NVMeFwCommitActFw(ulFwSlot))
                {
                    ulStatus = ((INVALID_4F << 16) | NVME_SF_INTERNAL_ERROR);
                    //TODO: Send AsyncEvent
                }
                L0_SubSystemOnlineReboot();
            }
            break;
            
            case CA_ACTIVE_IMMEDIATELY:
            {
                //bFinish = L0_RunNewFirmware(ulFwSlot);
                //Unspported now
                //pRunFWFunc = (pRunFW)HAL_GetFTableFuncAddr(RUN_FIRWARE);
                (void)pRunFWFunc;
                DBG_Printf("Do Not Support Active FW Immediately\r\n");
                ulStatus = ((INVALID_4F << 16) | NVME_SF_FW_ACT_R_RST); /* do no support active immediately now, need a reset */
            }
            break;

            default:
            {
                ulStatus = NVME_ERRLOG_COMPRESS(10 * 4, 3, NVME_SF_IVLD_FIELD);
            }
            break;
        }
    }

    if (NVME_SF_SUCCESS != ulStatus)
    {
        L0_NVMeCmdError(pSlot->SlotNum, ulStatus);
    }
    
    return TRUE;
}


MCU0_DRAM_TEXT BOOL L0_NVMeImgDownload(PCB_MGR pSlot)
{
    U32 ulDataByteLen;
    static U32 ulOffset;
    U32 ulNeedPRPList;
    BOOL bFinish;

    if (CMD_NEW == pSlot->CmdState)
    {
        ulDataByteLen = (pSlot->Ch->DW10 + 1) << 2;
        ulOffset      = (pSlot->Ch->DW11) << 2;
        
        pSlot->HasDataXfer = TRUE;
        pSlot->IsWriteDir = FALSE;
        
        pSlot->DataAddr = g_tFwUpdate.ulFwBaseAddr + ulOffset;
        pSlot->TotalRemainingBytes = ulDataByteLen;

        DBG_Printf("admin cmd: download image (0x%x ~ 0x%x)\n", ulOffset, ulOffset + ulDataByteLen);

        if (ulOffset == 0)
        {
            COM_MemSet((U32*)g_tFwUpdate.ulFwBaseAddr, g_tFwUpdate.ulFwSize >> 2, 0x00);
            g_tFwUpdate.ulDldSize = 0;
            g_tFwUpdate.bUpdateFwSize = FALSE;
        }

        /*
        The host software shall ensure that firmware pieces do not have Dword 
        ranges that overlap. Firmware portions may be submitted out of order to 
        the controller. firmware only check total data xfered, and do a CRC data
        check at firmware commit to ensure the integrity of the firmware Image.
        */
        g_tFwUpdate.ulDldSize += ulDataByteLen;
        if ((g_tFwUpdate.ulDldSize > g_tFwUpdate.ulFwSize)||((ulOffset + ulDataByteLen) > g_tFwUpdate.ulFwSize))
        {
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_OVERLAPPING));
            bFinish = TRUE;
        }
        
        else if (((ulOffset + ulDataByteLen) > FW_IMAGE_MEM_SIZE) || (g_tFwUpdate.ulFwSize > FW_IMAGE_MEM_SIZE))
        {
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_INTERNAL_ERROR));
            bFinish = TRUE;
        }

        else
        {
            ulNeedPRPList = L0_CheckNeedToFetchPRPList(pSlot);

            if (PRPLISTSTS_INVALIDADDR == ulNeedPRPList)
            {
                /* PRP list address is invalid. */
                DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pSlot->PRPTable[1].ulDW0, pSlot->PRPTable[1].ulDW1);
                L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
                bFinish = TRUE;
            }

            else if (PRPLISTSTS_NEEDFETCH == ulNeedPRPList)
            {
                /* We need to program FCQ to fetch PRP list. */
                pSlot->CmdState = CMD_FETCHPRPLIST;
                bFinish = FALSE;
            }

            else
            {
                /* It's not required to fetch PRP list. */
                pSlot->CmdState = CMD_HANDLING;
                bFinish = FALSE;
            }
        }
    }

    else if (CMD_FETCHPRPLIST == pSlot->CmdState)
    {
        TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp start:");
        if (SUCCESS == L0_NVMeHCTReadPRP(pSlot))
        {
            pSlot->CmdState = CMD_CHECKPRPLIST;
            bFinish = FALSE;
        }
        TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp end  :");
    }

    else if (CMD_CHECKPRPLIST == pSlot->CmdState)
    {
        if (FALSE == L0_IsPRPListOffsetValid(pSlot))
        {
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
            bFinish = TRUE;
        }
        
        else
        {
            pSlot->CmdState = CMD_HANDLING;
            bFinish = FALSE;
        }
    }

    else
    {
        bFinish = L0_NVMeXferData(pSlot);
    
        if ((FALSE == g_tFwUpdate.bUpdateFwSize ) && (TRUE == bFinish) && (g_tFwUpdate.ulDldSize >= 16384))
        {
            L0_FwUpgradeCalcImgLen();
            g_tFwUpdate.bUpdateFwSize = TRUE;
        }
    }

    return bFinish;
}


MCU0_DRAM_TEXT BOOL L0_NVMeFormatNvm(PCB_MGR pSlot)
{
    U32 ulSlotID;
    U32 ulSlotState;
    ADMIN_CMD_FORMAT_NVM tFormatParam;
    volatile ADMIN_IDENTIFY_CONTROLLER *pIDCTR;
    volatile ADMIN_IDENTIFY_NAMESPACE *pIDNS;
    BOOL bSupported = FALSE;
    BOOL bFormatFailed;
    U32 ulErrorStatusField;

    pIDCTR = (volatile ADMIN_IDENTIFY_CONTROLLER *)g_ulInfoIdfyPage;
    pIDNS = (volatile ADMIN_IDENTIFY_NAMESPACE *)(g_ulInfoIdfyPage + MCU0_NAMESPACE_IDENTIFY_OFFSET);

    tFormatParam.DW = pSlot->Ch->DW10;
    DBG_Printf("Admin Format NVM in Slot(%d), DW10:%d\n", pSlot->SlotNum, pSlot->Ch->DW10);

    //Paramet check
    //1.We only check whether NSID is equal  1 or 0xFFFFFFFF 
    //  since the format opertaion is apply to all NS as defined in FNA of Identify Controller Data.
    //2.PI should not been enabled since end to end protection is unsupport.
    //  and therefore, we ignore the value of PIL.
    //3.Check the LBA Foramt type we can support or not as defined in NS Identify Data structure(Common)
    //4.Check MS field in the LBA Format[x].
    if (tFormatParam.SES > 2u)
    {
        DBG_Printf("Secure Erase Setting field (SES %d)is reserved.\n", tFormatParam.SES);
        ulErrorStatusField = NVME_ERRLOG_COMPRESS(10 * 4 + 1, 1, NVME_SF_IVLD_FIELD);
    }

    else if (0 != tFormatParam.PI)
    {
        DBG_Printf("Protection Informtion is requested. But we do not support. PI:%d\n", tFormatParam.PI);
        ulErrorStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 5, NVME_SF_IVLD_FIELD);
    }

    else if ((1 != pSlot->Ch->NSID) && (INVALID_8F != pSlot->Ch->NSID))
    {
        DBG_Printf("Invalid NS ID %d in FormatNVM\n", pSlot->Ch->NSID);
        ulErrorStatusField = NVME_ERRLOG_COMPRESS(4, 0, NVME_SF_IVLD_NS_FMT);
    }

    else if ((tFormatParam.LBAF > pIDNS->NLBAF) || (pIDNS->LBAFx[tFormatParam.LBAF].LBADS < 9u))
    {
        DBG_Printf("Unsupported LBA format (%d)\n", tFormatParam.LBAF);
        ulErrorStatusField = NVME_ERRLOG_COMPRESS(10 * 4, 0, NVME_SF_IVLD_FORMAT);
    }

    else
    {
        bSupported = TRUE;
    }

    if (FALSE == bSupported)
    {
        L0_NVMeCmdError(pSlot->SlotNum, ulErrorStatusField);
    }

    else
    {
        //Since we only support One LBAF, there is no LBA size changed in NS.
        //And we only support One NS, there is no NSID earse check.

        //Make sure there is no IO CMD in Slot(Including Split SCMD,Read RPR and etc..)
        L0_WaitForAllSCmdCpl(0);

        bFormatFailed = FALSE;

        for (ulSlotID = 0; ulSlotID < MAX_SLOT_NUM; ulSlotID++)
        {
            if (ulSlotID != pSlot->SlotNum)
            {
                ulSlotState = HAL_HCTGetCST(ulSlotID);
                switch (ulSlotState)
                {
                    case SLOT_IDLE:
                    case SLOT_ASYNC_EVT_RESV:
                        break;

                    case SLOT_WAITING_SQENTRY:
                    case SLOT_WAITING_PRPLIST:
                    case SLOT_AFSQE_RDY:
                    case SLOT_SQENTRY_RDY:
                    case SLOT_PRPLIST_RDY:
                        DBG_Printf("Slot(%d) is %d state in Format Operation.\n", ulSlotID, ulSlotState);
                        bFormatFailed = TRUE;
                        break;

                    case SLOT_TRIGGER_WBQ:
                        DBG_Printf("WaitForAllSCmdCpl Fail in Format Operation.Slot(%d)\n", ulSlotID);
                        bFormatFailed = TRUE;
                        break;

                    default:
                        DBG_Printf("Slot(%d) is running out of state(%d)\n", ulSlotID, ulSlotState);
                        bFormatFailed = TRUE;
                        break;
                }
            }

            if (FALSE != bFormatFailed)
            {
                break;
            }
        }

        if (FALSE == bFormatFailed)
        {
            //logically,  there is no Slot is running  IO SCMD
            if ((pSlot->SQID != 0) && (INVALID_2F != gNvmeMgr.ActiveSlot))
            {
                DBG_Printf("Slot(%d) is running IO CMD in Format Operation\n", pSlot->SlotNum);
                bFormatFailed = TRUE;
            }
            else
            {
                //Stop timer before Format since Format will consume several seconds.
                HAL_HCmdTimerStop(pSlot->SlotNum);
                
                //Format
                if (SUCCESS != L0_IssueSecurityEraseSCmd(0, g_ulHostInfoAddr, FALSE))
                {
                    DBG_Printf("Security erase failed\n");
                    bFormatFailed = TRUE;
                }
                else
                {
                    //L0_SubSystemOnlineShutdown(TRUE);
                    //L0_SubSystemOnlineReboot();
                    L0_WaitForAllSCmdCpl(0);

                    g_ulSubSysBootOk = FALSE;
                    L0_IssueBootSCmd(0, g_ulHostInfoAddr);

                    L0_WaitForAllSCmdCpl(0);

                    //Start Timer again
                    HAL_HCmdTimerStart(pSlot->SlotNum);

                    DBG_Printf("Admin Format NVM. Done\n");
                }
            }
        }

        if (FALSE != bFormatFailed)
        {
            L0_NVMeCmdError(pSlot->SlotNum, NVME_ERRLOG_COMPRESS(0, 0, NVME_SF_IVLD_FORMAT));
        }
    }

    return TRUE;
}


MCU0_DRAM_TEXT BOOL L0_NVMeVIACmd(PCB_MGR pSlot)
{
    static VIA_CMD_CODE    eViaCmd;
    BOOL bFinish = FALSE;
    VIA_CMD_STATUS  eStatus = VCS_SUCCESS;
    VIA_CMD_PARAM   tCmdParam;
    U32 aFeedback[2] = {0};
    U32 ulNeedPRPList;

    switch(pSlot->CmdState)
    {
        case CMD_NEW:
            /* new command prepare */
            eViaCmd = BYTE_0(pSlot->Ch->DW12);
            tCmdParam.aDW[0] = pSlot->Ch->DW13;
            tCmdParam.aDW[1] = pSlot->Ch->DW14;
            tCmdParam.aDW[2] = BYTE_1(pSlot->Ch->DW12);

            if ((VIA_CMD_MEM_READ == eViaCmd) || (VIA_CMD_MEM_WRITE == eViaCmd))
            {
                pSlot->TotalRemainingBytes = tCmdParam.tMemAccess.bsByteLen;
                ulNeedPRPList = L0_CheckNeedToFetchPRPList(pSlot);
                
                if (PRPLISTSTS_INVALIDADDR == ulNeedPRPList)
                {
                    /* PRP list address is invalid. */
                    DBG_Printf("PRP List offset not aligned to QWORD: DW0 0x%x, DW1 0x%x\n", pSlot->PRPTable[1].ulDW0, pSlot->PRPTable[1].ulDW1);
                    L0_NVMeForceCompleteCmd(pSlot, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
                    bFinish = TRUE;
                }
                
                else if (PRPLISTSTS_NEEDFETCH == ulNeedPRPList)
                {
                    /* We need to program FCQ to fetch PRP list. */
                    pSlot->CmdState = CMD_FETCHPRPLIST;
                }
                
                else
                {
                    /* It's not required to fetch PRP list. */
                    pSlot->CmdState = CMD_HANDLING;
                }
            }

            else
            {
                pSlot->CmdState = CMD_HANDLING;
            }

            break;

        case CMD_HANDLING:
            eStatus = L0_ViaHostCmd(pSlot->SlotNum, eViaCmd, &tCmdParam, &aFeedback[0]);
            break;

        case CMD_FETCHPRPLIST:
            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp start:");
            if (SUCCESS == L0_NVMeHCTReadPRP(pSlot))
            {
                pSlot->CmdState = CMD_CHECKPRPLIST;
            }
            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "HCT build fcq for prp end  :");
            break;

        case CMD_CHECKPRPLIST:
            if (FALSE == L0_IsPRPListOffsetValid(pSlot))
            {
                L0_NVMeForceCompleteCmd(pSlot, NVME_ERRLOG_COMPRESS(32, 0, NVME_SF_IVLD_PRP_OFS));
                bFinish = TRUE;
            }

            else
            {
                pSlot->CmdState = CMD_HANDLING;
            }

            break;

        default:
            DBG_Getch();
    }

    if ((TRUE == bFinish) && (VCS_WAITING_RESOURCE != eStatus)) /* command finished */
    {
        pSlot->CSPC = aFeedback[0];
        SET_CQ_DW1(pSlot->SlotNum, aFeedback[1]);

        if (VCS_SUCCESS != eStatus)
        {
            L0_NVMeCmdError(pSlot->SlotNum, ((INVALID_4F << 16) | NVME_SF_VENDOR(eStatus)));
            DBG_Printf("VIA command(%d) execute error: (%d)!\n", eViaCmd, eStatus);
        }

        L0_NVMeSetupSimpleCompletion(pSlot, FALSE, TRUE);
        HAL_HCTSetCST(pSlot->SlotNum, SLOT_TRIGGER_WBQ);
        pSlot->CmdState = CMD_COMPLETED;
    }
    
    return bFinish;
}


/*====================End of this file========================================*/

