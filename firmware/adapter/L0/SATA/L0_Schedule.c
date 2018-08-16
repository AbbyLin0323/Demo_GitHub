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
Filename    :L0_Schedule.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_SataIO.h"
#include "HAL_TraceLog.h"
#include "Disk_Config.h"
#include "L0_Interface.h"
#include "L0_Event.h"
#include "L0_HcmdChain.h"
#include "L0_Pio.h"
#include "L0_TaskManager.h"
#include "L0_Schedule.h"
#include "L0_SataErrorHandling.h"
#include "HAL_Xtensa.h"

#define TL_FILE_NUM L0_Schedule_c

extern PIOINFO gPIOInfoBlock;
extern U32 g_ulSubsysNum;
extern U32 g_ulPowerStatus;
extern BOOL g_bStartTimer;
extern U32 g_ulStandbyTimer;
extern GLOBAL BOOL g_ulUARTCmdPending;

GLOBAL BOOL g_bFwUpdateOngoing;
GLOBAL BOOL g_bMultiCoreSCMDFlag;
GLOBAL U8 g_ucCurHcmdStage;
extern void L0_FwUpdateInit(void);

U8 L0_SataTask(void)
{
    // this is the main function of the L0 in SATA mode,
    // the major purpose of this function is to process
    // received host commands

    if (TRUE == L0_SataSelectErrPro())
    {
        return 2;
    }
    
    // try to select a host command from the host command queue
    // if the current host command is null
    if(NULL == g_pCurHCMD)
    {
        // try selecting a host command from the chain
        g_pCurHCMD = L0_HostCmdSelect();

        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L0 Sata select host command");

        // we've successfully selected a host command from the
        // chain, now we have to parse it
        if (NULL != g_pCurHCMD)
        {
            if(TRUE == g_bFwUpdateOngoing && ATA_CMD_DOWNLOAD_MICROCODE_DMA != g_pCurHCMD->tCFis.Command)
            {
                g_bFwUpdateOngoing = FALSE;
                L0_FwUpdateInit();
            }

            #ifdef HOST_CMD_REC
            TRACE_LOG((void *)&(g_pCurHCMD->tCFis),sizeof(CFIS),CFIS,0,"host send cmd");
            #endif
            g_pCurHCMD->tCbMgr.ATAState = ATACMD_NEW;
            g_pCurHCMD->tCbMgr.StageRemainingBytes = g_pCurHCMD->tCbMgr.TotalRemainingBytes;
            g_pCurHCMD->tCbMgr.CurrentSubCmdIndex = 0;

            // set the stage for the current host command
            g_ucCurHcmdStage = L0_HCMD_PROCESSING;

#ifndef SIM
            // after receive cmd, stop the timer
            if (HCMD_TYPE_DIRECT_MEDIA_ACCESS == g_pCurHCMD->tCbMgr.CmdType)
            {
                g_bStartTimer = TRUE;
                L0_ATAPowerCancelStandbyTimer();
            }
#endif
        }

#ifndef SIM
        else if (0 != g_ulStandbyTimer)
        {
            // if no cmd and the power status is idle, start timer to tranfer status
            if ((TRUE == g_bStartTimer) &&
                ((SATA_POWER_IDLE == g_ulPowerStatus) ||
                    (SATA_POWER_ACTIVEORIDLE == g_ulPowerStatus) ||
                    (SATA_POWER_ACTIVE == g_ulPowerStatus)))
            {
                L0_ATAPowerStartStandbyTimer(g_ulStandbyTimer);
                g_bStartTimer = FALSE;
            }
        }
#endif
    }

    // call ATA Lib for command processing
    if(NULL != g_pCurHCMD)
    {
        // process the current host command using the ATA lib
        // if it hasn't been processed
        if(g_ucCurHcmdStage == L0_HCMD_PROCESSING)
        {
            if(TRUE == L0_ATAProcessHostCmd(&g_pCurHCMD->tCbMgr))
            {
                // advance the state if the ATA lib is done
                // processing the current host command
                g_ucCurHcmdStage = L0_HCMD_FINISH;
            }
            TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L0_ATAProcessHostCmd");
        }

        // if the host command has been processed successfully
        // by the ATA lib, Invoke L0_ProcessSataProtocol to
        // handle sata protocol related operations
        if(g_ucCurHcmdStage == L0_HCMD_FINISH)
        {
            if(TRUE == L0_SataCompleteCmd(&g_pCurHCMD->tCbMgr))
            {
                // all protocol-related operations have been
                // completed, set the current host command
                // pointer to NULL
                g_pCurHCMD = NULL;
            }
            TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L0_SataCompleteCmd");
        }
    }

    // L0 todo: this should be moved to L0_ProcessSataProtocol
    // process the pio host command if we need to
    if(SATA_PIO_NOCMD != gPIOInfoBlock.ucCurrPIOState)
    {
        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L0_HandlePioProtocol start ");
        L0_HandlePioProtocol();
        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L0_HandlePioProtocol end");
    }

    return 1;
}

BOOL L0_HcmdSplitCheck(PCB_MGR pSlot)
{
    if(1 == g_ulSubsysNum)
    {
        // always return TRUE if there's only one subsystem,
        // in this case, we don't have to consider first data
        // ready at all
        return TRUE;
    }
    else if(pSlot->CurrentSubCmdIndex == 0)
    {
        // if we have 2 subsystems and we're about to split the
        // first scmd of a host command, we have to check if
        // this host command spreads across 2 MCUs
        if((L0M_GET_OFFSET_IN_LCT_FROM_LBA(pSlot->CurrentLBA) + (pSlot->TotalRemainingBytes >> SEC_SIZE_BITS)) > SEC_PER_BUF)
        {
            // this rule ensures that before we start processing
            // a multi-MCU host commands, we must wait until
            // all subcommand queues are empty
            return (L0_CheckSCQAllEmpty());
        }
        else if (TRUE == g_bMultiCoreSCMDFlag)
        {
            // g_bMultiCoreSCMDFlag means if there are subcommands
            // originated from a multi-MCU host command present
            // in at least one of the subcommand queues, if that's
            // the case, we must wait until all subcommand queues
            // become empty before processing a new host command
            return (L0_CheckSCQAllEmpty());
        }
        else
        {
            // the current host command doesn't spread across
            // 2 MCUs and there no multi-core commands in the
            // queue
            return TRUE;
        }
    }
    else
    {
        // not the first scmd, always return TRUE
        return TRUE;
    }
}

BOOL L0_SataIsDeviceIdle(void)
{
    return HAL_SataIsSdcIdle();
}

void L0_RegScanHelper(L0_TASK_MANAGER *pL0TaskMgr)
{
    U32 ulCurrIdleStat;
    U32 ulL0EventBitMap = L0_EventGetMap();

    /* Checking command idle status and updating global idle record. */
    if ((0 != (ulL0EventBitMap & ~(1 << L0_EVENT_TYPE_ATASTANDBYTIMEOUT)))
        || (TRUE == L0_SATAErrorPending()))
    {
        ulCurrIdleStat = FALSE;
    }

#ifndef SIM
    /* when uart interface used, FW can't entry PM_PRED_SLEEP state
           once MCU1 & MCU2 are halted, UART command will fail. */
    else if (TRUE == g_ulUARTCmdPending)
    {
        ulCurrIdleStat = FALSE;
    }
    else
    {
        ulCurrIdleStat = L0_SataIsDeviceIdle();
    }
#else
    else
    {
        extern U32 Host_IsCMDEmpty(void);
        ulCurrIdleStat = Host_IsCMDEmpty();
    }
#endif

    if (TRUE == pL0TaskMgr->PrevPortIdle)
    {
        if (TRUE == ulCurrIdleStat)
        {
            pL0TaskMgr->IdleLoopCount++;
        }

        else
        {
            pL0TaskMgr->IdleLoopCount = 0;
        }
    }

    pL0TaskMgr->PrevPortIdle = ulCurrIdleStat;

    return;
}

