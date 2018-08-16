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
* File Name    : L0_NVMeErrHandle.c
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2015.1.28
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_NVME.h"
#include "HAL_TraceLog.h"
#include "L0_NVMe.h"
#include "L0_NVMeErrHandle.h"
#include "L0_NVMeHCT.h"
#include "L0_NVMeDataIO.h"
#include "L0_Interface.h"
#include "L0_Event.h"
#include "HAL_NVMECFGEX.h"
#include "NvmeSpec.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
//specify file name for Trace Log
#define TL_FILE_NUM     L0_NVMeErrHandle_c

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL U32 g_ulSubsysSCQClrMap;
extern GLOBAL U32 g_ulSubsysNum;
extern U32 g_ulHostInfoAddr;
extern U32 g_ulATAGPLBuffStart;

#ifdef AF_ENABLE
extern volatile HCT_CONTROL_REG *g_pHCTControlReg;
extern volatile NVME_CFG_EX *g_pNVMeCfgExReg;
#endif

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL NVME_RESET l_tNVMeReset;
LOCAL U32 l_ulCurrErrLogEntryIndex;

LOCAL void L0_ErrHndlPrep(U32 ulErrorType, U32 ulCmdIdle, PSUBSYS_ERRHNDLCTRL pSubsysCtrl);
LOCAL PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY L0_NVMeGetErrorLogEntry(void);

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

MCU0_DRAM_TEXT void L0_NVMeErrorHandleInit(void)
{
    L0_NVMeClearErrorLog();
    l_tNVMeReset.ulResetMap = 0;

    return;
}

MCU0_DRAM_TEXT void L0_NVMeClearErrorLog(void)
{
    U32 ulErrorLogBaseAddr = g_ulATAGPLBuffStart + MCU0_LOG_ERROR_OFFSET;

    COM_MemZero((U32 *)ulErrorLogBaseAddr, (LOG_ERROR_LEN >> DWORD_SIZE_BITS));
    l_ulCurrErrLogEntryIndex = 0;

    return;
}

LOCAL PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY L0_NVMeGetErrorLogEntry(void)
{
    PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY pErrorLogBase, pCurrErrorLog;

    pErrorLogBase = (PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY)(g_ulATAGPLBuffStart + MCU0_LOG_ERROR_OFFSET);

    pCurrErrorLog = pErrorLogBase + (l_ulCurrErrLogEntryIndex++);

    if (LOG_ERROR_MAX_ENTRY_NUM == l_ulCurrErrLogEntryIndex)
    {
        l_ulCurrErrLogEntryIndex = 0;
    }

    return pCurrErrorLog;
}

INLINE U32 L0_NVMeGetLastErrorLogIndex(void)
{
    U32 ulLastErrLogIndex;

    if (0 == l_ulCurrErrLogEntryIndex)
    {
        ulLastErrLogIndex = (LOG_ERROR_MAX_ENTRY_NUM - 1);
    }

    else
    {
        ulLastErrLogIndex = (l_ulCurrErrLogEntryIndex - 1);
    }

    return ulLastErrLogIndex;
}

MCU0_DRAM_TEXT void L0_NVMeCmdError(U32 ulSlot, U32 ulStatusField)
{
    /* Input parameter ulStatusField:
        Lower half - Status Field to be reported to host;
        Higher half - Parameter Error Location to be recorded into Error log.
        */
    CB_MGR *pCmdSlot;
    PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY         pErr;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    U32 ulSQID, ulCQID;

    if (ulSlot >= MAX_SLOT_NUM)
    {
        DBG_Getch();
    }
    
    pCmdSlot = GET_SLOT_MGR(ulSlot);
    pCmdSlot->CmdSts = NVME_ERRLOG_EXTRACT_STATUS(ulStatusField);
    ulSQID = pCmdSlot->SQID;
    ulCQID = pCmdSlot->CQID;
    pErr = L0_NVMeGetErrorLogEntry();

    pHostInfoPage->NvmeErrorCntDW0++;
    if (0 == pHostInfoPage->NvmeErrorCntDW0)
    {
        pHostInfoPage->NvmeErrorCntDW1++;
    }

    COM_MemZero((U32 *)pErr, (sizeof(PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY) >> DWORD_SIZE_BITS));

    pErr->ErrorCount.DW0 = pHostInfoPage->NvmeErrorCntDW0;
    pErr->ErrorCount.DW1 = pHostInfoPage->NvmeErrorCntDW1;
    
    pErr->SubmissionQueueID = pCmdSlot->SQID;
    pErr->CommandID = pCmdSlot->Ch->CID;
    pErr->StatusField.Status = NVME_ERRLOG_EXTRACT_STATUS(ulStatusField);
    pErr->StatusField.PhaseTag = g_pNVMeCfgReg->cq_info[ulCQID].pbit;
    pErr->ParameterErrorLocation.usValue = NVME_ERRLOG_EXTRACT_LOCATION(ulStatusField);
    pErr->Namespace = 1;

    if ((NCS_READ == pCmdSlot->Ch->OPC) || (NCS_WRITE == pCmdSlot->Ch->OPC))
    {
        pErr->LBA.DW0 = pCmdSlot->CurrentLBAL;
        pErr->LBA.DW1 = pCmdSlot->CurrentLBAH;
    }

    //L0_NVMeGenerateAsyncEvent(ASYNC_EVENT_TYPE_ERROR, ASYNC_EVENT_ERRORINFO_TRS_INTERR);

#ifndef SPEEDUP_UNH_IOL
    DBG_Printf("L0_NVMeCmdError: Slot = %d, StatusField = 0x%x\n", ulSlot, ulStatusField);
#endif
    TRACE_LOG((void*)&ulSlot, sizeof(U8), U8, 0, "NVMe L0 Command Error, Slot: ");
    TRACE_LOG((void*)&ulStatusField, sizeof(U16), U16, 0, "NVMe L0 Command Error, StatusField: ");

    TRACE_LOG((void*)&ulSQID, sizeof(U32), U32, 0, "SQID:");
    TRACE_LOG((void *)pCmdSlot->Ch,sizeof(COMMAND_HEADER),COMMAND_HEADER,0,"host send cmd");//read pointer

    //todo: specific behavior for specific error type.

    return;
}

MCU0_DRAM_TEXT void L0_NVMeNonCmdError(U32 ulErrorType)
{
    PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY pErrLog = L0_NVMeGetErrorLogEntry();
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    pHostInfoPage->NvmeErrorCntDW0++;
    if (0 == pHostInfoPage->NvmeErrorCntDW0)
    {
        pHostInfoPage->NvmeErrorCntDW1++;
    }

    COM_MemZero((U32 *)pErrLog, (sizeof(PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY) >> DWORD_SIZE_BITS));

    pErrLog->ErrorCount.DW0 = pHostInfoPage->NvmeErrorCntDW0;
    pErrLog->ErrorCount.DW1 = pHostInfoPage->NvmeErrorCntDW1;

    pErrLog->SubmissionQueueID = INVALID_4F;
    pErrLog->CommandID = INVALID_4F;
    pErrLog->StatusField.Status = NVME_SF_IVLD_QID;
    pErrLog->ParameterErrorLocation.usValue = INVALID_4F;

    L0_NVMeGenerateAsyncEvent(ASYNC_EVENT_TYPE_ERROR, ulErrorType);

#ifndef SPEEDUP_UNH_IOL
    DBG_Printf("L0_NVMeNonCmdError: Type = 0x%x\n", ulErrorType);
#endif
    TRACE_LOG((void*)&ulErrorType, sizeof(U32), U32, 0, "NVMe Non Command Error, Type: ");

    return;
}

void L0_NVMeUecc(U32 ulSlot)
{
    volatile PNVME_WBQ pTargetWBQEntry;
    L0_NVMeCmdError(ulSlot, ((INVALID_4F << 16) | NVME_SF_UECC));
    
    //Update CQ Status.
    pTargetWBQEntry = (PNVME_WBQ)HAL_HCTGetWBQEntry(ulSlot, 0);
    pTargetWBQEntry->StatusF = NVME_SF_UECC;

    return;
}

void L0_NVMeNVMReset(void)
{
    l_tNVMeReset.bsNVMSubSystemReset = TRUE;
    L0_EventSet(L0_EVENT_TYPE_XFER_RESET, NULL);

    return;
}

void L0_NVMePCIeReset(void)
{
    L0_NVMeCfgLock();
    l_tNVMeReset.bsPCIeReset = TRUE;
    L0_EventSet(L0_EVENT_TYPE_XFER_RESET, NULL);

    return;
}

void L0_NVMeControllerReset(void)
{
    l_tNVMeReset.bsControllerReset = TRUE;
    L0_EventSet(L0_EVENT_TYPE_XFER_RESET, NULL);

    return;
}

BOOL L0_NVMeLinkFailure(void *pEventParam)
{
    /* Finds the type of link failure and clears corresponding failure status. */
    if (0 != (rPCIe(0xCC) & (1 << 18)))
    {
        /* Currently we only process PCIe Fatal Error status.
                  Warning: this patch method is not considered to be
                  a correct behavior on a PCIe controller, because it
                  is host's responsibility to clear error status in PCIe
                  capability registers. */
        rPCIeByte(0xCE) = (1 << 2);
    }

    /* Terminates processing of outstanding commands immediately. */
    STOP_NVME();

    /* Uses CSTS.CFS flag to notify host a fatal error occurred. */
    SET_FATAL_ERR();

    return TRUE;
}

BOOL L0_NVMeLinkFatalError(void *pEventParam)
{
    /* Simulates host clearing Fatal Error Status and Flow Control Protocol Error Status. */
    rPCIe(0x104) = (1 << 13);
    rPCIeByte(0xCE) = (1 << 2);

    return TRUE;
}

U32 L0_NVMeGetResetMap(void)
{
    return l_tNVMeReset.ulResetMap;
}

LOCAL void L0_ErrHndlPrep(U32 ulErrorType, U32 ulCmdIdle, PSUBSYS_ERRHNDLCTRL pSubsysCtrl)
{
    /* Re-programs PCIE registers that are cleared in a PCIE reset. */
    if (RSTTP_PCIE_RESET == ulErrorType)
    {
        L0_NVMePhySetting();
    }

    /* The error or event occurs in abnormal condition. */
    if (FALSE == ulCmdIdle)
    {
        /* 1. Holds reset signal of HOSTC/HCT in a controller reset. */
        if (RSTTP_CONTROLLER_RESET == ulErrorType)
        {
            HAL_HCTAssertReset();
        }

        /* 2. Helps hardware OTFB path to recover:
                       enables SGQ to finish on-the-fly data transfer. */
        rGlbOTFBMCtrl0 |= (1 << 18);   /* ROTFB_SELFCLR_EN */

        /* 3. Marks a SCMD queue force clear is required. */
        pSubsysCtrl->ulProcStage = ERRHNDL_FORCECLEARSCQ;
    }

    /* The error or event occurs in normal condition. */
    else
    {
        /* Force Idle is not required if no data transfer is pending. */
        pSubsysCtrl->ulProcStage = ERRHNDL_SELFTEST;
    }

    /* Initializes subsystem related control information. */
    pSubsysCtrl->ulSubsysMap = 0;

    /* Disables Async Event reporting as early as possible. */
    L0_NVMeDisableAsyncEventReporting();

    return;
}

LOCAL U32 L0_ErrHndlSubsysProc(PSUBSYS_ERRHNDLCTRL pSubsysCtrl)
{
    U32 ulSubsysId;
    U32 ulProcStage;
    U32 ulFinished;

    U32 ulSCMDSent;

    ulProcStage = pSubsysCtrl->ulProcStage;
    ulFinished = FALSE;

    TRACE_LOG((void*)&ulProcStage, sizeof(U32), U32, 0, "L0_ErrHndlSubsysProc ulProcStage:");
    switch (ulProcStage)
    {
        case ERRHNDL_FORCECLEARSCQ:
            /* 1. In handling of an abnormal PCIe reset, we need to issue an Clear SCMD Queue Request
                          to each subsystem through inter-MCU interrupt . */
            g_ulSubsysSCQClrMap = 0;

            //post notify message to all sub-systems
            L0_PostNtfnMsg(((1 << g_ulSubsysNum) - 1), SMSG_FORCE_CLEAR_SCQ);

            ulProcStage = ERRHNDL_WAITSCQCLEAR;
            break;

        case ERRHNDL_WAITSCQCLEAR:
            /* 2. Waits for the response SMSGs from both subsystems. */
            if (((1 << g_ulSubsysNum) - 1) == g_ulSubsysSCQClrMap)
            {
                ulProcStage = ERRHNDL_FORCEIDLE;
            }

            break;

        case ERRHNDL_SELFTEST:
        case ERRHNDL_FORCEIDLE:
            /* 3. Issues proper SCMDs to subsystems according to error occuring condition. */
            if (((1 << g_ulSubsysNum) - 1) == pSubsysCtrl->ulSubsysMap)
            {
                /* Appropriate SCMD has already been issued to all subsystems. */
                ulProcStage = ERRHNDL_WAITIDLE;
            }
            
            else
            {
                for (ulSubsysId = 0; ulSubsysId < g_ulSubsysNum; ulSubsysId++)
                {
                    if (0 == (pSubsysCtrl->ulSubsysMap & (1 << ulSubsysId)))
                    {
                        if (ERRHNDL_SELFTEST == ulProcStage)
                        {
                            ulSCMDSent = L0_IssueSelfTestingSCmd(ulSubsysId);
                        }
            
                        else
                        {
                            /* Each subsystem shall reset its chain maintainence data
                                                     when receiving an IDLE SCMD with Error Recovery request. */
                            /* Subsystem: HAL_ChainMaintainInit(); */
                            ulSCMDSent = L0_IssueIdleSCmd(ulSubsysId, (U32)IDLE_ERRORRECOVERY);
                        }
            
                        if (SUCCESS == ulSCMDSent)
                        {
                            pSubsysCtrl->ulSubsysMap |= (1 << ulSubsysId);
                        }
                    }
            
                }
            }

            break;

        case ERRHNDL_WAITIDLE:
            /* 4. Waits for the responses of Force Idle or Self Test SCMDs from subsystems. */
            if (TRUE == L0_CheckSCQAllEmpty() && L0_IsMsgQueueEmpty())
            {
                ulFinished = TRUE;
            }

            break;

        default:
            break;
    }

    pSubsysCtrl->ulProcStage = ulProcStage;

    return ulFinished;
}

LOCAL U32 L0_ErrHndlExecLocalRcvry(U32 ulResetType, U32 ulCmdIdle)
{

    /* Resets L0 local task executing status when an error or event
            occurs in abnormal condition. */
    if (FALSE == ulCmdIdle)
    {
        /* 1) Resets SGE hardware completely to clear any possible invalid internal status. */
        /* Moved SGE resetting to L3 error recovering force idle process. */
        //HAL_SGEReset();

        /* L0 local interfaces for managing SGE shall be reset as well. */
        L0_HostDataXferInit();

        /* 2) Resets all command slot managers. */
        //L0_NVMeBarSpaceInit();  /* to be confirm */
        //L0_NVMeMgrInit(&gNvmeMgr);

        /* 3) Resets HCT hardware to avoid unexpected status report to host. */
        /* Resets OTFB manager. */
        HAL_SGEOTFBMapReset();
        
        /* And we must restore the normal operating state of SGE hardware by
                     clearing OTFB self-clear signal. */
        rGlbOTFBMCtrl0 &= ~(1 << 18);   /* ROTFB_SELFCLR_EN */

        if (RSTTP_CONTROLLER_RESET == ulResetType)
        {
            /* De-asserts the holding reset signal for a controller reset. */
            HAL_HCTReleaseReset();
        }

        else
        {
            /* De-asserts the automatical holding reset signal for a PCIe reset. */
            HAL_HCTClearAutoReset();
        }

        /* Re-initializes HCT and HOSTC registers after resetting HCT. */
        L0_NVMEInitHCT();
    }

    else if (RSTTP_PCIE_RESET == ulResetType)
    {
        /* For a PCIe reset, hardware would always reset HCT and HOSTC despite of the reset occurs
           under what condition. So we always need to de-assert the reset signal and re-initialize
           configuration registers. */
        HAL_HCTClearAutoReset();

        /* Re-initializes HCT and HOSTC registers. */
        L0_NVMEInitHCT();
    }

    else
    {
        /* For a Controller reset without resetting HCT, we must clear all command slots reserved for Async Event manually. */
        L0_NVMeClearAsyncEventResvSlots();
    }

    return TRUE;
}


LOCAL void L0_ErrHndlNtfyHost(U32 ulResetType)
{
    if (RSTTP_PCIE_RESET == ulResetType)
    {
        rGlbMcuSgeRst |= R_RST_HOSTC_MMCFG;
        HAL_DelayCycle(20);  //wait 20 clock
        rGlbMcuSgeRst &= (~R_RST_HOSTC_MMCFG);

#ifdef AF_ENABLE
        /* Restores auto-fetch settings because they would be reset synchronously with MMIO reset. */
        L0_NVMeCfgExInit();
#endif
    }

    return;
}


BOOL L0_NVMeProcResetEvent(void *p)
{
    static SUBSYS_ERRHNDLCTRL tSubsysCtrlInfo;
    static U32 ulProcState;
    static U32 ulHostIdle;
    static U32 ulResetType;
    U32 ulSlotIndex;
    BOOL bFinish = FALSE;
    U32 ulSQID;

    /* Patch for Normal shutdown on windows simulation environment */
#ifdef SIM    
    return TRUE;
#endif    
    
    if (L0_EVENT_STAGE_START == L0_EventGetStage(L0_EVENT_TYPE_XFER_RESET))
    {
    
        STOP_NVME();
        ulProcState = ERRORSTATE_NEW;
        //Disabe auto fetch before we check host idle or not.
#ifdef AF_ENABLE
        STOP_AF();
        for (ulSQID = 1; ulSQID < MAX_SQ_NUM; ulSQID++)
        {
            L0_NVMeDisableSQAF(ulSQID);
        }
#endif
        if (TRUE == l_tNVMeReset.bsPCIeReset)
        {
            ulHostIdle = L0_NVMeCheckPCIEResetIdle();
        }
        else
        {
            ulHostIdle  = L0_NVMeCheckHostIdle();
        }

        L0_EventSetStage(L0_EVENT_TYPE_XFER_RESET, L0_EVENT_STAGE_PROCESSING);

        TRACE_LOG((void*)&l_tNVMeReset.ulResetMap, sizeof(U32), U32, 0, "NVMe L0 Proc Reset start, l_tNVMeReset.ulResetMap: ");
        TRACE_LOG((void*)&ulHostIdle, sizeof(U32), U32, 0, "NVMe L0 Proc Reset start, ulHostIdle: ");

        if (l_tNVMeReset.bsNVMSubSystemReset)
        {
            ulResetType = RSTTP_NVM_SUBSYS_RESET;
            DBG_Printf("NVM subsystem reset, ");
        }
        else if (l_tNVMeReset.bsPCIeReset)
        {
            ulResetType = RSTTP_PCIE_RESET;
            DBG_Printf("PCIe reset, ");
        }
        else if (l_tNVMeReset.bsControllerReset)
        {
            ulResetType = RSTTP_CONTROLLER_RESET;
            DBG_Printf("Controller reset, ");
        }
        else
        {
            DBG_Printf("Unknown reset, something wrong. \n");
            L0_NVMeErrorHandleInit();
            return TRUE;
        }

        if (TRUE == ulHostIdle)
        {
            DBG_Printf("Host idle.\n");
        }
        else
        {
            DBG_Printf("Host command pending.\n");
            TRACE_LOG((void*)&gNvmeMgr, sizeof(NVME_MGR), NVME_MGR, 0, "NVMe L0 Proc Reset start, gNvmeMgr: ");
            TRACE_LOG((void*)&(g_pNVMeCfgReg->cmd_fetch_helper), sizeof(U32), U32, 0, "NVMe L0 Proc Reset start, g_pNVMeCfgReg->cmd_fetch_helper: ");
            for (ulSlotIndex = 0; ulSlotIndex < MAX_SLOT_NUM; ulSlotIndex++)
            {
                TRACE_LOG((void*)&ulSlotIndex, sizeof(U32), U32, 0, "NVMe L0 Proc Reset start, SlotID: ");
                TRACE_LOG((void*)(HCT_CS_REG + sizeof(U8) * ulSlotIndex), sizeof(U8), U8, 0, "NVMe L0 Proc Reset start, CST: ");
                ulSQID = SQID_IN_SLOT(ulSlotIndex);
                TRACE_LOG((void*)&ulSQID, sizeof(U32), U32, 0, "SQID:");
                TRACE_LOG((void*)(HCT_S0_BASE + sizeof(COMMAND_HEADER) * ulSlotIndex), sizeof(COMMAND_HEADER), COMMAND_HEADER, 0, "NVMe L0 Proc Reset start, COMMAND_HEADER: ");
                TRACE_LOG((void*)(HCT_S1_BASE + sizeof(COMMAND_TABLE) * ulSlotIndex), sizeof(COMMAND_TABLE), COMMAND_TABLE, 0, "NVMe L0 Proc Reset start, COMMAND_TABLE: ");
            }
        }
    }

    TRACE_LOG((void*)&ulProcState, sizeof(U32), U32, 0, "ulProcState:");
    
    switch(ulProcState)
    {
        case ERRORSTATE_NEW:
            /* 1. Checks error type and reports the preparing status. */
            L0_ErrHndlPrep(ulResetType, ulHostIdle, &tSubsysCtrlInfo);
            ulProcState = ERRORSTATE_PROCSUBSYS;
            break;

        case ERRORSTATE_PROCSUBSYS:
            /* 2. Starts interactions with subsystems that are required due to error type. */
            if (TRUE == L0_ErrHndlSubsysProc(&tSubsysCtrlInfo))
            {
                ulProcState = ERRORSTATE_LOCALRECOVERY;
            }
            break;

        case ERRORSTATE_LOCALRECOVERY:
            /* 3. Performs L0 local error recovery work. */
            if (TRUE == L0_ErrHndlExecLocalRcvry(ulResetType, ulHostIdle))
            {
                ulProcState = ERRORSTATE_NOTIFYHOST;
            }
            break;

        case (U32)ERRORSTATE_NOTIFYHOST:
            /* 4. Updates status registers or generates interrupt to notify host
                         after clearing the error pending status. */
            L0_ErrHndlNtfyHost(ulResetType);
            ulProcState = (U32)ERRORSTATE_COMPLETED;
            break;

        default:
            DBG_Getch();
    }
    
    if ((U32)ERRORSTATE_COMPLETED == ulProcState)
    {
        switch(ulResetType)
        {
            case RSTTP_NVM_SUBSYS_RESET:
                l_tNVMeReset.bsNVMSubSystemReset = FALSE;
                DBG_Printf("NVM Subsystem reset process completed.\n ");
                break;

            case RSTTP_PCIE_RESET:
                l_tNVMeReset.bsPCIeReset = FALSE;
                DBG_Printf("PCIe reset process completed.\n ");
                break;

            case RSTTP_CONTROLLER_RESET:
                l_tNVMeReset.bsControllerReset = FALSE;
                DBG_Printf("Controller reset process completed.\n ");
                break;

            default:
                DBG_Getch();
        }

        if (0 == l_tNVMeReset.ulResetMap)
        {
            DBG_Printf("Bar space init\n");
            L0_NVMeBarSpaceInit();
            L0_NVMeClearErrorLog();
            bFinish = TRUE;
        }
        else
        {
            L0_EventSetStage(L0_EVENT_TYPE_XFER_RESET, L0_EVENT_STAGE_START);
        }

        TRACE_LOG((void*)&ulResetType, sizeof(U32), U32, 0, "NVMe L0 Proc Reset completed, ulResetType: ");
    }

    return bFinish;   
}

void L0_WaitHostReset(void)
{
    DBG_Printf("Stops NVME and waits for host reset...");
    STOP_NVME();

    return;
}


/*====================End of this file========================================*/

