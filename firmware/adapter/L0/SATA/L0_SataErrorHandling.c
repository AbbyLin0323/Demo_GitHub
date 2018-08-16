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
Filename    :L0_SataErrorHandling.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_SataIO.h"
#include "HAL_BufMap.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#include "HAL_SataDSG.h"
#include "HAL_TraceLog.h"
#include "L0_Interface.h"
#include "L0_Event.h"
#include "L0_HcmdChain.h"
#include "L0_Pio.h"
#include "L0_Schedule.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"
#include "L0_SATAErrorHandling.h"

//specify file name for Trace Log
#define TL_FILE_NUM     L0_SataErrorHandling_c

extern U32 g_ulATAInfoIdfyPage;
extern U32 g_ulMaxGlobalLba;
extern U32 g_ulSubsysNum;
extern U32 g_ulATAGPLBuffStart;
extern U32 g_ulSubsysSCQClrMap;
extern U32 g_ulHostInfoAddr;
extern U32 g_ulPowerStatus;

LOCAL SATA_ERR_MARK l_tErrorMark;
LOCAL SATA_ERR_STATUS l_tErrorStatus;
LOCAL BOOL l_bDiskLocked;
LOCAL BOOL l_bCommandBlocked;
LOCAL BOOL l_bUECCMsgRcv;
LOCAL U8 l_ucUECCCmdTag;
LOCAL U32 l_ulEhSubsysMap;

LOCAL SATA_REJECT_MARK l_tRejectMark;

void L0_LockDisk(void)
{
    l_bDiskLocked = TRUE;
    //DBG_Printf("Lock disk\n");
    return;
}

void L0_UnlockDisk(void)
{
    l_bDiskLocked = FALSE;
    //DBG_Printf("Unlcok disk\n");
    return;
}

BOOL L0_IsDiskLocked(void)
{
    return (l_bDiskLocked);
}

BOOL L0_SataIsErrorPending(void)
{
    return (l_tErrorStatus.bsPending);
}

void L0_SataSetUECCMsgRcv(U8 ucTag)
{
    l_bUECCMsgRcv = TRUE;
    l_ucUECCCmdTag = ucTag;
    //DBG_Printf("Receive UECC SMsg\n");
    return;
}

void L0_SataClearUECCMsgRcv(void)
{
    l_bUECCMsgRcv = FALSE;
    return;
}

U8 L0_SataIsUECCTag(void)
{
    if (FALSE == l_bUECCMsgRcv)
    {
        return INVALID_2F;
    }

    return (l_ucUECCCmdTag);
}

BOOL L0_SataIsEncounterUECC(void)
{
    return (l_bUECCMsgRcv);
}

void L0_SataEhClearSCQCleanupMsgRcv(void)
{
    g_ulSubsysSCQClrMap = 0;

    return;
}

BOOL L0_SataEhIsSCQCleanupDone(void)
{
    U32 ulSubsysId;
    BOOL bRet = TRUE;

    for (ulSubsysId = 0; ulSubsysId < g_ulSubsysNum; ulSubsysId++)
    {
        if (0 == (g_ulSubsysSCQClrMap & (1 << ulSubsysId)))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

void L0_SataEhInit(void)
{
    l_tErrorStatus.bsPending = FALSE;
    l_bDiskLocked = FALSE;
    l_bCommandBlocked = FALSE;
    l_bUECCMsgRcv = FALSE;
    g_ulSubsysSCQClrMap = 0;
    l_ulEhSubsysMap = 0;

    return;
}

void L0_SataEhWarmInit(void)
{
    l_bDiskLocked = FALSE;
    l_bCommandBlocked = FALSE;
    l_bUECCMsgRcv = FALSE;
    g_ulSubsysSCQClrMap = 0;
    l_ulEhSubsysMap = 0;

    return;
} 

void L0_SataMarkCmdError(U8 ucType, U8 ucTag, U32 ulLBA, U32 ulSecCnt)
{
    l_tErrorMark.ulErrBitmap |= (1 << ucType);
    l_tErrorMark.aErrInfo[ucType].ucTag = ucTag;
    l_tErrorMark.aErrInfo[ucType].ulLBA = ulLBA;
    l_tErrorMark.aErrInfo[ucType].ulSecCnt = ulSecCnt;
    l_tErrorMark.aErrInfo[ucType].ulNcqOutstd = rSDC_NCQOutstd;

    return;
}

void L0_SataMarkReset(U8 ucRstType)
{
    l_tErrorMark.ulErrBitmap |= (1 << ucRstType);
    
    return;
}

void L0_SataMarkSError(U16 usSErrType)
{
    l_tErrorMark.ulErrBitmap |= (1 << SATAERR_TYPE_SERROR);
    l_tErrorMark.aErrInfo[SATAERR_TYPE_SERROR].usSErrType = usSErrType;

    return;
}

BOOL L0_SATAErrorPending(void)
{
    return (0 != l_tErrorMark.ulErrBitmap);
}

void L0_SataSErrTrace(void)
{
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    U16 usSErrType = l_tErrorMark.aErrInfo[SATAERR_TYPE_SERROR].usSErrType;
    U16 usSErrorCnt = ++(pHostInfoPage->SErrorCnt);

    DBG_Printf("SERROR type 0x%x, count 0x%x\n", usSErrType, usSErrorCnt);
    TRACE_LOG((void*)&usSErrType, sizeof(U16), U16, 0, "SError Type: ");
    TRACE_LOG((void*)&usSErrorCnt, sizeof(U16), U16, 0, "SError Count: ");

    return;
}

void L0_SataSetErrEvent(U8 ucErrType)
{
    DBG_Printf("Set SATA EH event, ErrType=%d\n", ucErrType);
    
    L0_EventSet(L0_EVENT_TYPE_ERR_HANDLING, NULL);

    l_tErrorStatus.bsPending = TRUE;
    l_tErrorStatus.ucState = SATAERR_STATE_NEW;
    l_tErrorStatus.ucType = ucErrType;
    COM_MemCpy((U32 *)&l_tErrorStatus.tErrInfo, (U32 *)&(l_tErrorMark.aErrInfo[ucErrType]), (sizeof(SATA_ERR_INFO) >> DWORD_SIZE_BITS));

    return;
}

BOOL L0_SataSelectErrPro(void)
{
    U8 ucErrType;
    BOOL bRet = FALSE;

    if (TRUE == l_tErrorStatus.bsPending)
    {
        return TRUE;
    }

    HAL_DisableMCUIntAck();
    
    if (0 != (l_tErrorMark.ulErrBitmap & (1 << SATAERR_TYPE_SERROR)))
    {
        
        L0_SataSErrTrace();
        l_tErrorMark.ulErrBitmap &= ~(1 << SATAERR_TYPE_SERROR);       
    }

    if (0 != l_tErrorMark.ulErrBitmap)
    {
        for (ucErrType = SATAERR_TYPE_COMRESET; ucErrType <= SATAERR_TYPE_NCQ_UECC; ucErrType++)
        {
            if (0 != (l_tErrorMark.ulErrBitmap & (1 << ucErrType)))
            {
                L0_SataSetErrEvent(ucErrType);
                /* COMRESET and OOB done event need to be cleared at the same time for fixing OOB racing issue */
                if (ucErrType == SATAERR_TYPE_COMRESET)
                    l_tErrorMark.ulErrBitmap &= ~((1 << ucErrType) | (1 << SATAERR_TYPE_COMRESET_FIRST));
                else
                    l_tErrorMark.ulErrBitmap &= ~(1 << ucErrType);
                bRet = TRUE;
                break;
            }
        }
    }

    HAL_EnableMCUIntAck();

    return bRet;
}

void L0_SataEhUpdateNcqCmdErrLog(void)
{
    NCQ_CMD_ERR *pNcqErrLog;
    U8 *pByte;
    U32 ulLBA = l_tErrorStatus.tErrInfo.ulLBA;
    U32 ulSecCnt = l_tErrorStatus.tErrInfo.ulSecCnt;
    U32 ulByteIndex;
    U8 ucCheckSum = 0;

    pNcqErrLog = (NCQ_CMD_ERR *)L0_GPLGetLogPageAddr(g_ulATAGPLBuffStart, GPL_LOGADDR_NCQERR, 0);

    COM_MemZero((U32 *)pNcqErrLog, sizeof(NCQ_CMD_ERR)/sizeof(U32));

    if (SATAERR_TYPE_INTERMIX == l_tErrorStatus.ucType)
    {
        pNcqErrLog->bsNQ = TRUE;
    }

    pNcqErrLog->bsTAG = l_tErrorStatus.tErrInfo.ucTag;
    pNcqErrLog->Status = 0x41;
    if (SATAERR_TYPE_NCQ_UECC == l_tErrorStatus.ucType)
    {
        pNcqErrLog->Error = 0x40;
    }
    else
    {
        pNcqErrLog->Error = 0x4;
    }
    pNcqErrLog->LBAByte0 = (U8)ulLBA;
    pNcqErrLog->LBAByte1 = (U8)(ulLBA >> 8);
    pNcqErrLog->LBAByte2 = (U8)(ulLBA >> 16);
    pNcqErrLog->Device = 0xE0;
    pNcqErrLog->LBAByte3 = (U8)(ulLBA >> 24);
    pNcqErrLog->LBAByte4 = 0;
    pNcqErrLog->LBAByte5 = 0;
    pNcqErrLog->CountByte0 = (U8)ulSecCnt;
    pNcqErrLog->CountByte1 = (U8)(ulSecCnt >> 8);

    pByte = (U8 *)pNcqErrLog;
    for (ulByteIndex = 0; ulByteIndex < (sizeof(NCQ_CMD_ERR) - 1); ulByteIndex++)
    {
        ucCheckSum += pByte[ulByteIndex];
    }
    pNcqErrLog->Checksum = (~ucCheckSum) + 1;

    return;
}

BOOL L0_SataErrorHandling(void *pParam)
{
    U8 ucProState = l_tErrorStatus.ucState;
    U8 ucErrType = l_tErrorStatus.ucType;
    U32 ulSubsysId; 
    U32 ulSCmdSentFlag;
    BOOL bRet = FALSE;
    U8 ucIsIdle;
    U32 ulSlotID;
    PIDENTIFY_DEVICE_DATA pIdentifyData;

    switch (ucProState)
    {
    case SATAERR_STATE_NEW:
        ucIsIdle = L0_SataIsDeviceIdle();
        TRACE_LOG((void*)&ucErrType, sizeof(U8), U8, 0, "Sata L0 EH start, Error Type: ");
        DBG_Printf("L0 EH %d new, Idle = %d\n", ucErrType, ucIsIdle);
        TRACE_LOG((void*)&ucIsIdle, sizeof(U8), U8, 0, "Sata L0 EH start, Device Idle: ");        
        TRACE_LOG((void*)&rSDC_IOControl, sizeof(U8), U8, 0, "rSDC_IOControl: ");
        TRACE_LOG((void*)&rSDC_PwrAndCmdStatus, sizeof(U32), U32, 0, "rSDC_PwrAndCmdStatus: ");

        if (FALSE == ucIsIdle)
        {
            for (ulSlotID = 0; ulSlotID < MAX_SLOT_NUM; ulSlotID++)
            {
                TRACE_LOG((void*)&ulSlotID, sizeof(U32), U32, 0, "SlotID: ");
                TRACE_LOG((void*)&HostCmdSlot[ulSlotID], sizeof(HCMD), HCMD, 0, "HCMD: ");
            }
        }

        switch (ucErrType)
        {
        case SATAERR_TYPE_INTERMIX:
        case SATAERR_TYPE_TAG:        
            L0_LockDisk();
            ucProState = SATAERR_STATE_CLEAN_START;
            break;

        case SATAERR_TYPE_NCQ_LBA:
            L0_LockDisk();

            if (FALSE == ucIsIdle)
            {
                ucProState = SATAERR_STATE_CLEAN_START;
            }
            else
            {
                ucProState = SATAERR_STATE_SELF_TEST;
            }
            break;

        case SATAERR_TYPE_NCQ_UECC:
            HAL_DelayCycle(0x800000);

            HAL_SDCHoldDmaAndCmd();
            HAL_HoldBuffMap();
            ucProState = SATAERR_STATE_CLEAN_START;
            break;

        case SATAERR_TYPE_SERROR:
            HAL_DelayCycle(0x100000);

        case SATAERR_TYPE_COMRESET:
        case SATAERR_TYPE_SOFTWARE_RESET:
            rSDC_ControlRegister &= ~HW_PWR_EN;
            HAL_SDCHoldDmaAndCmd();
            HAL_HoldBuffMap();

            if (FALSE == ucIsIdle)
            {
                ucProState = SATAERR_STATE_CLEAN_START;
            }
            else
            {
                //ucProState = SATAERR_STATE_SELF_TEST;
                ucProState = SATAERR_STATE_SUBSYS_OK;
            }
            break;

        default:
            break;
        }//switch (ucErrType)
        HAL_ResetSataDSG();
        break;

    case SATAERR_STATE_CLEAN_START:
        //DBG_Printf("L0 EH %d clean start\n", ucErrType);
        if (SATAERR_TYPE_NCQ_UECC != ucErrType)
        {
            L0_HostCommandInit();
            L0_PioInit();
        }
        
        /* Interrupt MCU12 */
        L0_PostNtfnMsg((1 << g_ulSubsysNum) - 1, SMSG_FORCE_CLEAR_SCQ);
        ucProState = SATAERR_STATE_WAIT_SCQ_CLEANUP;
        break;

    case SATAERR_STATE_WAIT_SCQ_CLEANUP:
        if (TRUE == L0_SataEhIsSCQCleanupDone())
        {
            L0_SataEhClearSCQCleanupMsgRcv();
            l_ulEhSubsysMap = 0;
            ucProState = SATAERR_STATE_FORCE_IDLE;
        }
        break;

    case SATAERR_STATE_FORCE_IDLE:
    case SATAERR_STATE_SELF_TEST:   
        if (((1 << g_ulSubsysNum) - 1) != l_ulEhSubsysMap)
        {           
            for (ulSubsysId = 0; ulSubsysId < g_ulSubsysNum; ulSubsysId++)
            {
                if (0 == (l_ulEhSubsysMap & (1 << ulSubsysId)))
                {
                    if (SATAERR_STATE_SELF_TEST == ucProState)
                    {
                        ulSCmdSentFlag = L0_IssueSelfTestingSCmd(ulSubsysId);
                    }
                    else
                    {
                        ulSCmdSentFlag = L0_IssueIdleSCmd(ulSubsysId, (U32)IDLE_ERRORRECOVERY);
                    }

                    if (SUCCESS == ulSCmdSentFlag)
                    {
                        l_ulEhSubsysMap |= (1 << ulSubsysId);
                        //DBG_Printf("Send IdleTask SCmd to subsys %d success, l_ulEhSubsysMap = 0x%x\n",ulSubsysId, l_ulEhSubsysMap);
                    }
                }
            }//for (ulSubsysId = 0; ulSubsysId < g_ulSubsysNum; ulSubsysId++)
        }
        else
        {
            l_ulEhSubsysMap = 0;
            ucProState = SATAERR_STATE_WAIT_SUBSYS;
            //DBG_Printf("Eh state Force Idle finish\n");
        }
        break;

    case SATAERR_STATE_WAIT_SUBSYS:
        if (TRUE == L0_CheckSCQAllEmpty())
        {
            HAL_ResetSataDSG();
            //DBG_Printf("Eh state Wait Subsys,SCQ all empty\n");
            ucProState = SATAERR_STATE_SUBSYS_OK;
            TRACE_LOG((void*)&ucErrType, sizeof(U8), U8, 0, "Sata L0 EH subsys OK, Error Type: ");
        }
        break;

    case SATAERR_STATE_SUBSYS_OK:
        //DBG_Printf("Eh state Subsys OK\n");
        switch (ucErrType)
        {
        case SATAERR_TYPE_INTERMIX:
        case SATAERR_TYPE_TAG:
        case SATAERR_TYPE_NCQ_LBA:
            L0_SataEhUpdateNcqCmdErrLog();
                
            HAL_SDCReleaseDmaAndCmd();
            HAL_ReleaseBuffMap();
            HAL_SataClearAllNcqOutstd();
            HAL_SataClearBigBusy();

            // mask command interrupt to ensure hardware DIPM correctness
            HAL_SataEnableCmdRcvInt(FALSE);

            HAL_SataSendAbortStatus();
            HAL_SataClearPIOCmdPending();

            // Unmask command interrupt
            HAL_SataEnableCmdRcvInt(TRUE);

            //DBG_Printf("Eh state Subsys OK finish\n");
            ucProState = SATAERR_STATE_DONE;
            break;

        case SATAERR_TYPE_NCQ_UECC:
            L0_SataEhUpdateNcqCmdErrLog();

            HAL_SDCReleaseDmaAndCmd();
            HAL_ReleaseBuffMap();
            HAL_SataClearAllNcqOutstd();

            L0_HostCommandInit();
            L0_PioInit();

            HAL_SataSendSDBUncorrectableError();

            L0_LockDisk();

            //HAL_SataClearSmallBusy();
            HAL_SataSetAllSendSDBFISReady();

            ucProState = SATAERR_STATE_DONE;
            break;

        case SATAERR_TYPE_COMRESET:
        case SATAERR_TYPE_SOFTWARE_RESET:
            pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
            pIdentifyData->SataFeatureEnable.DipmEn  = FALSE;  //IPM-08 initial identify value

            // if receive comreset with SSP disable, the security status will be transfer
            if(FALSE == pIdentifyData->SataFeatureEnable.SwSettingPreserveEn)
            {
                L0_ATAInitSecurity();
            }

            if((TRUE == pIdentifyData->SataFeatureSupport.SwSettingPreserve) 
                && (SATAERR_TYPE_COMRESET == ucErrType))
            {
                pIdentifyData->SataFeatureEnable.SwSettingPreserveEn = TRUE;
            }

            // if receive reset, device need to change power status
            if((SATA_POWER_SLEEP == g_ulPowerStatus) || (SATA_POWER_STANDBY == g_ulPowerStatus))
            {
                g_ulPowerStatus = SATA_POWER_STANDBY;
            }
            else
            {
                g_ulPowerStatus = SATA_POWER_ACTIVEORIDLE;
            }

            HAL_SDCReleaseDmaAndCmd();

            HAL_SataClearAllNcqOutstd();
            HAL_SataClearPIOCmdPending();
            HAL_SataClearHWHoldErr();
            HAL_SataClearComResetBlock();

            L0_SataEhWarmInit();
            HAL_SataSetAllSendSDBFISReady();
            HAL_SataDisableNcqFinishInt();
            rSDC_ControlRegister |= FW_DMAEXE_STE_EN;//temporary fixing: Gavin, 20150320

            /* OOB status doesn't return to host if new COMRESET is asserted again for fixing OOB racing issue */
            if ((l_tErrorMark.ulErrBitmap & (1 << SATAERR_TYPE_COMRESET_FIRST)) == 0)
                HAL_SataSendGoodStatus();

            HAL_ReleaseBuffMap();//change sequence bruce
            //stop SDC from sending COM_INIT automatically when next COM_RESET arrive
            //*(volatile U32 *)(REG_BASE_PMU + 0x10) &= ~(1 << 6);

            ucProState = SATAERR_STATE_DONE;
            break;

        case SATAERR_TYPE_SERROR:
            HAL_SDCReleaseDmaAndCmd();
            HAL_ReleaseBuffMap();
            HAL_SataClearAllNcqOutstd();
            HAL_SataClearHWHoldErr();
            HAL_SataClearComResetBlock();

            HAL_SataSendAbortStatus();
            L0_UnlockDisk();

            ucProState = SATAERR_STATE_DONE;
            break;

        default:
            break;
        }//switch (ucErrType)
        break;

    case SATAERR_STATE_DONE:
        DBG_Printf("L0 EH %d done\n", ucErrType);
        TRACE_LOG((void*)&ucErrType, sizeof(U8), U8, 0, "Sata L0 EH completed, Error Type: ");
        l_tErrorStatus.bsPending = FALSE;
        
        //Giga 2015.8.20 : Set SDC Offset 0xB8 bit 11 CONTROL_INT_MSK to 0.
        if((rSDC_IntMask & BIT_SDC_INTSRC_FIS_CONTROL) != 0)
            rSDC_IntMask &= ~(BIT_SDC_INTSRC_FIS_CONTROL);
           
        bRet = TRUE;
        break;

    default:
        DBG_Printf("L0_SataErrorHandling: state error\n");
        break;
    }//switch (ucProState)

    l_tErrorStatus.ucState = ucProState;

    return bRet;
}

void L0_SataReceiveUECCMsg(U8 ucTag)
{
    if ((FALSE == L0_SataIsEncounterUECC()) && (FALSE == L0_SataIsErrorPending()))
    {
        L0_SataSetUECCMsgRcv(ucTag);
        HAL_SataEnableNcqFinishInt();
    }
    else
    {
    }
    return;
}

void L0_SataMarkCmdReject(U8 ucReason, U8 ucTag, U8 ucCmdCode)
{
    L0_EventSet(L0_EVENT_TYPE_CMD_REJECT, NULL);
    
    l_tRejectMark.uctReason = ucReason;
    l_tRejectMark.ucTag = ucTag;
    l_tRejectMark.ucCmdCode = ucCmdCode;

    return;
}

BOOL L0_SataCmdReject(void *p)
{
    DBG_Printf("Reject cmd 0x%x, tag 0x%x, reason %d\n", l_tRejectMark.ucCmdCode, l_tRejectMark.ucTag, l_tRejectMark.uctReason);
    TRACE_LOG((void*)&l_tRejectMark.ucCmdCode, sizeof(U8), U8, 0, "Reject Cmd code: ");
    TRACE_LOG((void*)&l_tRejectMark.ucTag, sizeof(U8), U8, 0, "Reject cmd tag: ");
    TRACE_LOG((void*)&l_tRejectMark.uctReason, sizeof(U8), U8, 0, "Reject cmd reason: ");

    // mask command interrupt to ensure hardware DIPM correctness
    HAL_SataEnableCmdRcvInt(FALSE);

    HAL_SataSendAbortStatus();
    HAL_SataClearPIOCmdPending();    

    // Unmask command interrupt
    HAL_SataEnableCmdRcvInt(TRUE);

    return TRUE;
}

/* end of file L0_SATAErrorHandling.c */

