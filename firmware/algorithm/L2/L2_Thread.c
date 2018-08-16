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
Filename    :L2_Thread.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.28
Description :functions about Thread Dispatcher
Others      :
Modify      :CloudsZhang: Update for multi thread code
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "FW_Event.h"
#include "L1_Cache.h"
#include "L1_Interface.h"
#include "L2_Thread.h"
#include "L2_Defines.h"
#include "L2_Thread.h"
#include "L2_FTL.h"
#include "L2_StaticWL.h"
#include "L2_ErrorHandling.h"
#include "L2_GCManager.h"
#include "L2_PMTManager.h"
#include "L2_Shutdown.h"
#include "L2_Schedule.h"
#include "L2_Debug.h"
#include "L2_Boot.h"
#include "L2_Erase.h"
#include "L2_TableBBT.h"
#include "L2_FCMDQ.h"

#ifndef SIM
#include <xtensa/tie/xt_timer.h>
#endif

extern void L2_FTLWrite(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
extern L2_FTL_STATUS L2_FTLRead(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);


LOCAL  SCHEDULE l_L2Scheduler[SUBSYSTEM_SUPERPU_MAX];
LOCAL  SYSTEM_STATE l_L2SystemState[SUBSYSTEM_SUPERPU_MAX];

#define TL_FILE_NUM L2_Thread_c


void MCU1_DRAM_TEXT L2_InitScheduler(U8 ucSuperPu)
{
    l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_ERROR_HANDLING] = 0;
    l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_BOOT] = 0;
    l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_TABLE_PMT] = 0;
    l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_FTL] = 0;
    l_L2Scheduler[ucSuperPu].eCurrThread = SYS_THREAD_FTL;
    l_L2Scheduler[ucSuperPu].usThreadQuotaUsed = 0;
    return;
}

/*****************************************************************************
 Prototype      : L2_ClearSysScheduler
 Description    :
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/25
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
void L2_ClearSysScheduler(U8 ucSuperPu)
{
    U8 i;

    for (i = 0; i < SYS_THREAD_TYPE_ALL; i++)
    {
        l_L2Scheduler[ucSuperPu].aThreadQuota[i] = 0;
    }

    l_L2Scheduler[ucSuperPu].usThreadQuotaUsed = 0;
    return;
}

void L2_ForceFlushPMTSetting(U8 ucSuperPu)
{
    SYS_THREAD_TYPE eCurrThread;

    eCurrThread = l_L2Scheduler[ucSuperPu].eCurrThread;
    l_L2Scheduler[ucSuperPu].usThreadQuotaUsed = l_L2Scheduler[ucSuperPu].aThreadQuota[eCurrThread];
#ifdef DEBUG_NOTLCFREEBLK
    DBG_Printf("[%s] CurThread %d, maxQ %d, curQ %d\n", __FUNCTION__, eCurrThread, l_L2Scheduler[ucSuperPu].aThreadQuota[eCurrThread],
        l_L2Scheduler[ucSuperPu].usThreadQuotaUsed);
#endif
}

/*****************************************************************************
 Prototype      : L2_SetSystemState
 Description    : set system state and set default quota
 Input          : U8 ucSuperPu
 SYSTEM_STATE SystemState
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/25
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
void L2_SetSystemState(U8 ucSuperPu, SYSTEM_STATE eSystemState)
{
    l_L2SystemState[ucSuperPu] = eSystemState;

    /* clear system scheduler */
    L2_ClearSysScheduler(ucSuperPu);

    /* assign default quota for thread */
    switch (eSystemState)
    {
    case SYSTEM_STATE_BOOT_BLOCKING:
        l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_BOOT] = 0xFFFF;
        l_L2Scheduler[ucSuperPu].eCurrThread = SYS_THREAD_BOOT;
        break;

    case SYSTEM_STATE_BOOT_PENDING:
        if (BOOT_NORMAL == g_BootManagement->m_BootType)
        {
            l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_BOOT] = 1;
        }
        else
        {
            l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_BOOT] = 0xFFFF;
        }
        l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_FTL] = 1;
        l_L2Scheduler[ucSuperPu].eCurrThread = SYS_THREAD_FTL;
        break;

    case SYSTEM_STATE_ERROR_HANDLING:
        l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_ERROR_HANDLING] = 0xFFFF;
        l_L2Scheduler[ucSuperPu].eCurrThread = SYS_THREAD_ERROR_HANDLING;
        break;

    case SYSTEM_STATE_NORMAL:
        l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_FTL] = g_ulPMTFlushRatio;
        l_L2Scheduler[ucSuperPu].aThreadQuota[SYS_THREAD_TABLE_PMT] = 1;
        l_L2Scheduler[ucSuperPu].eCurrThread = SYS_THREAD_FTL;
        break;


    default:
        break;
    }

    return;
}

SYSTEM_STATE L2_GetCurrSystemState(U8 ucSuperPu)
{
    return l_L2SystemState[ucSuperPu];
}

/*****************************************************************************
 Prototype      : L2_GetCurrThreadType
 Description    :
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/25
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
SYS_THREAD_TYPE L2_GetCurrThreadType(U8 ucSuperPu)
{
    return l_L2Scheduler[ucSuperPu].eCurrThread;
}

void L2_SetCurrThreadType(U8 ucSuperPu, SYS_THREAD_TYPE eThreadSet)
{
    l_L2Scheduler[ucSuperPu].eCurrThread = eThreadSet;
    l_L2Scheduler[ucSuperPu].usThreadQuotaUsed = 0;

    return;
}

void L2_SetThreadQuota(U8 ucSuperPu, SYS_THREAD_TYPE eThreadSet, U32 ulQuota)
{
    l_L2Scheduler[ucSuperPu].aThreadQuota[eThreadSet] = ulQuota;

    return;
}

void L2_SysIncUsedThreadQuota(U8 ucSuperPu)
{
    l_L2Scheduler[ucSuperPu].usThreadQuotaUsed++;
    return;
}

/*****************************************************************************
 Prototype      : L2_ThreadQuotaAssign
 Description    : assign quota for thread when all quota comsume out.
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/25
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
void L2_ThreadQuotaAssign(U8 ucSuperPu)
{
    /* ask thread controller one by one to assign the quota */
    //TODO:
    U8 ucSycState = SYS_THREAD_FTL;

    /* prcess error handling */
    if (SYSTEM_STATE_ERROR_HANDLING == l_L2SystemState[ucSuperPu])
    {
        //L2_ClearSysScheduler(ucSuperPu);
        L2_SetCurrThreadType(ucSuperPu, SYS_THREAD_ERROR_HANDLING);
        L2_SetThreadQuota(ucSuperPu, SYS_THREAD_ERROR_HANDLING, 0xFFFF);
    }
    /* process boot */
    else if (SYSTEM_STATE_BOOT_PENDING == l_L2SystemState[ucSuperPu])
    {
        if (TRUE == L2_IsAllTablePuLoadOk(ucSuperPu))
        {
            L2_SetSystemState(ucSuperPu, SYSTEM_STATE_NORMAL);
        }
        else
        {
            if (BOOT_NORMAL == g_BootManagement->m_BootType)
            {
                if (BOOT_STATUS_BEGIN == g_BootManagement->m_BootStatus[ucSuperPu])
                {
                    if (FALSE == g_BootManagement->m_DoingBootStipeFlag[ucSuperPu])
                    {
                        L2_SetThreadQuota(ucSuperPu, SYS_THREAD_TABLE_PMT, 0);
                        //FTL: TablePMT: Boot = 1:0:1
                    }
                    else
                    {
                        L2_SetThreadQuota(ucSuperPu, SYS_THREAD_FTL, 0);
                        L2_SetThreadQuota(ucSuperPu, SYS_THREAD_TABLE_PMT, 0);
                        //FTL: TablePMT: Boot = 0:0:1
                    }
                }
                else if (BOOT_PU_LOAD_PMT_PAGE == g_BootManagement->m_BootStatus[ucSuperPu])
                {
                    ucSycState = SYS_THREAD_TABLE_PMT;
                    L2_SetThreadQuota(ucSuperPu, SYS_THREAD_TABLE_PMT, 1);
                    //FTL: TablePMT: Boot = 0:1:1 or FTL: TablePMT: Boot = 1:1:1
                }
            }//normal boot
            
            L2_SetCurrThreadType(ucSuperPu, ucSycState);
        }
    }
    else if (SYSTEM_STATE_BOOT_BLOCKING == l_L2SystemState[ucSuperPu])
    {
        //L2_ClearSysScheduler(ucSuperPu);
        L2_SetCurrThreadType(ucSuperPu, SYS_THREAD_BOOT);
        L2_SetThreadQuota(ucSuperPu, SYS_THREAD_BOOT, 0xFFFF);
    }
    else if (SYSTEM_STATE_NORMAL == l_L2SystemState[ucSuperPu])
    {
        //L2_ClearSysScheduler(ucSuperPu);
        /* ask and set FTL thread quota */
        //L2_SetThreadQuota(ucSuperPu, SYS_THREAD_FTL, 64);

        /* ask and set table thread quota */

        if (TRUE == L2_IsPMTTableGC(ucSuperPu))   //table need GC 
        {
            L2_SetThreadQuota(ucSuperPu, SYS_THREAD_TABLE_PMT, 2);
        }
        else
        {
            L2_SetThreadQuota(ucSuperPu, SYS_THREAD_TABLE_PMT, 1);
        }

        /* set current thread to FTL */
        L2_SetCurrThreadType(ucSuperPu, SYS_THREAD_FTL);
    }
    else
    {
        /* set current thread to FTL */
        L2_SetCurrThreadType(ucSuperPu, SYS_THREAD_FTL);
    }

    return;
}

/*****************************************************************************
 Prototype      : L2_SysThreadDecision
 Description    : system thread decision
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/25
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
SYS_THREAD_TYPE L2_SysThreadDecision(U8 ucSuperPu)
{
    U16 usMaxQuota;
    SYS_THREAD_TYPE eCurrThread;
    U32 ulWatchDog = 0;

    eCurrThread = l_L2Scheduler[ucSuperPu].eCurrThread;

    while (TRUE)
    {
        usMaxQuota = l_L2Scheduler[ucSuperPu].aThreadQuota[eCurrThread];
        if (l_L2Scheduler[ucSuperPu].usThreadQuotaUsed >= usMaxQuota)
        {
            l_L2Scheduler[ucSuperPu].usThreadQuotaUsed = 0;
            eCurrThread++;
            if (eCurrThread >= SYS_THREAD_TYPE_ALL)
            {
                /* assign the quota again for all thread */
                L2_ThreadQuotaAssign(ucSuperPu);
                eCurrThread = l_L2Scheduler[ucSuperPu].eCurrThread;
            }
            else
            {
                l_L2Scheduler[ucSuperPu].eCurrThread = eCurrThread;
            }

            /* avoid dead loop in thread decision */
            ulWatchDog++;
            if(ulWatchDog >= 2 * (SYS_THREAD_TYPE_ALL))
            {
                return SYS_THREAD_TYPE_ALL;
            }
        }
        else
        {
            return eCurrThread;
        }
    }
}

U32 GetLpnInSystem(U32 ulPUID, U32 ulLpnInPU)
{
    U32 ulLpnInPuRow;
    U32 ulLpnInPuRowOffset;
    U32 ulLpnInSystem;

    ulLpnInPuRow = ulLpnInPU / LPN_PER_BUF;
    ulLpnInPuRowOffset = ulLpnInPU% LPN_PER_BUF;
    ulLpnInSystem = ulLpnInPuRow * (SUBSYSTEM_SUPERPU_NUM * LPN_PER_BUF)
        + ulPUID * LPN_PER_BUF + ulLpnInPuRowOffset;

    return ulLpnInSystem;
}

U32 L2_GetSuperPuBitMap(U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucSuperPu = 0;
    U32 i;
    U32 ulSuperPuBitMap = 0;

    if (1 == SUBSYSTEM_SUPERPU_NUM && 0 != ulLunAllowToSendFcmdBitmap)
    {
        return 0x1;
    }
    else if (1 == LUN_NUM_PER_SUPERPU)
    {
        return ulLunAllowToSendFcmdBitmap;
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (0 != (ulLunAllowToSendFcmdBitmap & SUPERPU_LUN_NUM_BITMSK))
        {
            ulSuperPuBitMap |= (1 << i);
        }
        ulLunAllowToSendFcmdBitmap = (ulLunAllowToSendFcmdBitmap >> LUN_NUM_PER_SUPERPU);
    }

    return ulSuperPuBitMap;
}
extern GLOBAL U32 g_ulPMTFlushing;

void L2_Scheduler(void)
{
    SYS_THREAD_TYPE Thread;
    U8 ucSuperPu, i;
    U8 ucAllowScheduleSuperPuCnt;
    U16 usEraseVBN;
    U32 ulLunAllowToSendFcmdBitmap;
    U32 ulSuperPuBitMap;
    BOOL ucIncUsedQuota = FALSE;
    L2_FTL_STATUS tReadSts;
    BOOL bTLCEraseFinish;

    /* process not single CE schedule event */
    if (COMM_EVENT_STATUS_BLOCKING == L2_TaskEventHandle())
    {
        return;
    }

    /* get total PU need L2 to scheduler*/
    ulLunAllowToSendFcmdBitmap = L2_FCMDQGetNotFullBitmap(0);
    ulSuperPuBitMap = L2_GetSuperPuBitMap(ulLunAllowToSendFcmdBitmap);

    /* add lookup L1 Read/Write BufReqPuBitMap to decide which superpu need to schedule except Shutdown/BootUp stage */
    if (FALSE == L2_IsEventPending() && TRUE == L2_IsBootupOK())
    {
        if(FALSE == g_ulPMTFlushing)
        {
        	ulSuperPuBitMap &= L1_GetRWBufReqPuBitMap();
    	}
    }

    ucAllowScheduleSuperPuCnt = HAL_POPCOUNT(ulSuperPuBitMap);

    if (0 == ucAllowScheduleSuperPuCnt)
    {
        return;
    }

    /* set sclz regesiter */
    //HAL_Wclzstate(ulSuperPuBitMap);

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L2 select PU ");

    /* single CE shceduler */
    for (i = 0; i < ucAllowScheduleSuperPuCnt; i++)
    {
        /* set sclz regesiter,Move Here By Nina 2016-05-05, because TLCMerge will also use HAL_Wclzstate*/
        HAL_Wclzstate(ulSuperPuBitMap);

        /* get PU */
        ucSuperPu = (31 - HAL_SCLZ());

        ulSuperPuBitMap &= ~(1 << ucSuperPu);

        /* priority to process READ cmd */
        tReadSts = L2_FTLRead(ucSuperPu, ulLunAllowToSendFcmdBitmap);
#ifdef READ_DISTURB_OPEN
        if (FTL_STATUS_CMD_PENDING == tReadSts)
        {
            continue;
        }
        else if (FTL_STATUS_OK == tReadSts)
        {
            U32 ul2ndCalSuperPuBitMap;

            //decrease no need cylce consume for Read performance, when disksize is large the bottle neck is MCU.
            if ((FALSE == L2_BlkQueueIsEmpty(g_pForceGCSrcBlkQueue[ucSuperPu], LINK_TYPE_HOST))
                || (FALSE == L2_BlkQueueIsEmpty(g_pForceGCSrcBlkQueue[ucSuperPu], LINK_TYPE_TLCW))
                || (FALSE == L2_FTLIsAllTaskDone(ucSuperPu)))
            {
                ulLunAllowToSendFcmdBitmap = L2_FCMDQGetNotFullBitmap(0);
                ul2ndCalSuperPuBitMap = L2_GetSuperPuBitMap(ulLunAllowToSendFcmdBitmap);

                if (0 == ((1 << ucSuperPu)&ul2ndCalSuperPuBitMap))
                {
                    continue;
                }
                //FIRMWARE_LogInfo("##\n");
            }
            else
            {
                continue;
            }
        }
#else
        if (FTL_STATUS_NEED_SCHEDULER != tReadSts)
        {
            continue;
        }
#endif

        /* check to be erase pool, erase all dirty block */
        /* if the block is all dirty */
        bTLCEraseFinish = L2_FTLCheckEraseSts(ucSuperPu);
#ifdef SCAN_BLOCK_N1
        if (bTLCEraseFinish)
        {
            continue;
        }
#endif
        if (g_EraseOpt[ucSuperPu].L2NeedEraseBlk == TRUE)
        {
            //usEraseVBN = L2_PopBlkFromEraseQueue(ucSuperPu);
            usEraseVBN = L2_SelectNeedEraseBlkFromQueue(ucSuperPu);
            if (usEraseVBN != INVALID_4F)
            {
                L2_FTLErase(ucSuperPu, ulLunAllowToSendFcmdBitmap, usEraseVBN);
                continue;
            }

            g_EraseOpt[ucSuperPu].L2NeedEraseBlk = FALSE;
        }

        /*
           FTL entry, PMT Table entry, Boot entry
           */
        Thread = L2_SysThreadDecision(ucSuperPu);
        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L2 select Thread ");
        switch (Thread)
        {
        case SYS_THREAD_FTL:
            ucIncUsedQuota = L2_FTLEntry(ucSuperPu, ulLunAllowToSendFcmdBitmap);
            break;

        case SYS_THREAD_TABLE_PMT:
            ucIncUsedQuota = L2_TableEntry(ucSuperPu);
            break;

        case SYS_THREAD_BOOT:
            ucIncUsedQuota = L2_BootEntry(ucSuperPu);
            break;

        case SYS_THREAD_ERROR_HANDLING:
            break;

        default:
            break;
        }

        if (TRUE == ucIncUsedQuota)
        {
            L2_SysIncUsedThreadQuota(ucSuperPu);
        }
    }

    return;
}

