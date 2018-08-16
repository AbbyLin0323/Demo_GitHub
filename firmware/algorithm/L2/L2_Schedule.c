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
File Name     : L2_RT.c
Version       : Initial version
Author        : henryluo
Created       : 2015/02/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2012/01/18
Author      : peterxiu
Modification: Created file

*******************************************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L3_HostAdapter.h"
#include "L2_FullRecovery.h"
#include "L1_PlatformDefine.h"
#include "L1_SubCmdProc.h"
#include "L1_Cache.h"
#include "L1_Buffer.h"
#include "L1_Interface.h"
#include "FW_Event.h"
#include "L2_Schedule.h"
#include "L2_LLF.h"
#include "L2_Thread.h"
#include "L2_PMTManager.h"
#include "L2_StaticWL.h"
#include "L2_Boot.h"
#include "L2_Thread.h"
#include "L2_Debug.h"
#include "L2_RT.h"
#include "L2_Interface.h"
#include "L2_TableBlock.h"
#include "L1_SpecialSCMD.h"
#include "L2_DWA.h"
#include "L2_TableBBT.h"
#include "L2_ReadDisturb.h"
#include "L2_FCMDQ.h"
#include "L2_TLCMerge.h"
#include "COM_BitMask.h"

#include "L2_PMTPage.h"

#include "HAL_Xtensa.h"
#ifdef SIM
#include "HAL_MultiCore.h"
#include "L2_SimTablecheck.h"
extern BOOL g_bTableRebuildFlag[FW_MCU_COUNT];
#else
#include <xtensa/tie/xt_timer.h>
#endif
/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/
extern void L2_LLF_PMT_Init(void);
extern BOOL L2_IdleFlushPMTPage(U32 ulFlushCNT);
extern U32 L2_ShutdownEntry(void);
#ifndef LCT_VALID_REMOVED
extern void L1_SetLCTValidLoadStatus(U8 ucStatus);
#endif
extern L2_FTL_STATUS L2_FTLRead(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
extern BOOL COM_BitMaskGet(U32 ulBitMask, U8 ucIndex);
extern void L2_FTLSetTableRebuildFlag(U8 ucSuperPu, BOOL bFlagVal);
extern GLOBAL void MCU1_DRAM_TEXT L2_BbtDisablePbnBindingTable(void);
extern U32 L2_GetSuperPuBitMap(U32 ulLunAllowToSendFcmdBitmap);

#ifdef SIM
extern void L2_BackupRPMT();
#endif
/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
GLOBAL  BOOL bShutdownPending = FALSE;
GLOBAL  U32 g_ulShutdownFlushPagePos[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  U32 g_ulShutdownFlushPageCnt[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  U32 g_L2EventInit = FALSE;
GLOBAL  BOOL g_bL2PbnLoadDone = FALSE;
GLOBAL  BOOL bBbtBootLoadStatus = FALSE;
GLOBAL  L2_EventStatus g_L2EventStatus = { FALSE };

extern GLOBAL  U32 Rebuild_Start;
extern GLOBAL  U32 Rebuild_End;
extern GLOBAL  LC lc;

extern GLOBAL FTL_IDLE_GC_MGR g_tFTLIdleGCMgr;
/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/

BOOL L2_IsEventPending()
{
    COMMON_EVENT L2_Event;

    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);

    if (0 == L2_Event.Event)
    {
        return FALSE;
    }

    return TRUE;
}

/* Return SUCCESS:
    1. normal running, no event => return SUCCESS;
    2. L2 received an shutdown event, standby immediately=> return SUCCESS;
    3. L2 received an shutdown event, system shutdown, waiting ldle => return SUCCESS;
Return FAIL:
    L2 received an shutdown event, system shutdown => return FAIL; 
    only return FAIL when ShutDown task is under processing*/
U32 L2_CheckTaskEventStatus(BOOL bCheckWaitlde)
{
    if ((g_L2EventStatus.m_Shutdown == FALSE) || (g_L2EventStatus.m_WriteAfterShutdown == TRUE))
    {
        return SUCCESS;
    }
    else
    {
        if ((TRUE == bCheckWaitlde) && (g_L2EventStatus.m_WaitIdle == TRUE))
        {
            return SUCCESS;
        }

        return FAIL;
    }
}

BOOL L2_HasWaitIdleEvent()
{
    return g_L2EventStatus.m_WaitIdle;
}

/****************************************************************************
Name        :L2_TaskEventBootInit
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.11.12    17:18:00
Description :init L2 boot event static parameters.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT L2_TaskEventBootInit(void)
{
    U32 i;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_ulShutdownFlushPagePos[i] = 0;
        g_ulShutdownFlushPageCnt[i] = 0;
    }
    bShutdownPending = FALSE;
}

void MCU12_DRAM_TEXT L2_TaskInit(void)
{
    U8 i;

    L2_SharedMemMap(&g_FreeMemBase);
    L2_FCMDQReqInit();
    L2_InitRebuildDptr();
#ifdef PMT_ITEM_SIZE_REDUCE
	//Adjustable PMTItem Strategy needed
	L2_PMTBitInit((U8)SUBSYSTEM_SUPERPU_NUM, (U8)LUN_NUM_PER_SUPERPU, (U8)BLOCK_PER_PU_BITS, (U8)PAGE_PER_BLOCK_BITS, (U8)LPN_PER_PAGE_BITS);
#endif

    L2_Sram0Map(&g_FreeMemBase.ulFreeSRAM0Base);
    L2_Sram1Map(&g_FreeMemBase.ulFreeSRAM1Base);
    L2_DramMap(&g_FreeMemBase.ulDRAMBase);
    L2_OtfbMap(&g_FreeMemBase.ulFreeOTFBBase);

    L2_BbtDisablePbnBindingTable();
    L2_InitDataStructures(FALSE);
    L2_TB_Reset_All();
    L2_InitTLCInverseProgOrderTable();
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        L2_InitScheduler(i);
        L2_InitFTLScheduler(i);
    }
    L2_TaskEventBootInit();
#ifdef SIM
    L2_BackupRPMT();
#endif

#ifdef SUBSYSTEM_BYPASS_LLF_BOOT
    L2_LLFInit();
    L2_TableBlock_LLF(FALSE);
    L2_LLF_PMT_Init();
    g_BootUpOk = TRUE;
    g_RebuildDirtyFlag = FALSE;

    /* Set L2 system state to normal */
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        L2_SetSystemState(i, SYSTEM_STATE_BOOT_PENDING);
    }
#endif

    return;
}

/*****************************************************************************
 Prototype      : L2_GlobalProcessRead
 Description    : patch for response read command during calc dirty count for detect disk ASAP
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/4/29
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_GlobalProcessRead(void)
{
    U8 ucSuperPu, i;
    U8 ucAllowScheduleSuperPuCnt;
    U32 ulLunAllowToSendFcmdBitmap, ulSuperPuBitMap;
    U32 ulReadBufReqCnt = 0;

    while (0 != g_ulReadBufReqPuBitMap)
    {
        /* get total super PU need L2 to scheduler*/
        ulLunAllowToSendFcmdBitmap = L2_FCMDQGetNotFullBitmap(0);
        ulSuperPuBitMap = L2_GetSuperPuBitMap(ulLunAllowToSendFcmdBitmap);

        /* set sclz regesiter */
        ulSuperPuBitMap &= g_ulReadBufReqPuBitMap;
        ucAllowScheduleSuperPuCnt = HAL_POPCOUNT(ulSuperPuBitMap);

        if (0 == ucAllowScheduleSuperPuCnt)
        {
            return;
        }

        /* set sclz regesiter */
        HAL_Wclzstate(ulSuperPuBitMap);

        /* single LUN shceduler */
        for (i = 0; i < ucAllowScheduleSuperPuCnt; i++)
        {
            /* get super PU */
            ucSuperPu = (31 - HAL_SCLZ());

            /* priority to process READ cmd */
            if (FTL_STATUS_OK == L2_FTLRead(ucSuperPu, ulLunAllowToSendFcmdBitmap))
            {
                ulReadBufReqCnt++;
            }
            else
            {
                return;
            }
        }
    }

    return;
}

/****************************************************************************
Name        :L2_TaskEventHandle
Input       :
Output      :
Author      :Peter Xiu
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#define IDLE_NEED_GC            0x00
#define IDLE_FLUSH_PMTPAGE      0x01
#define IDLE_PATROL_READ        0x02
#define IDLE_FTL_BUILD_REQ      0x03

LOCAL MCU12_VAR_ATTR U8 l_ucIdleStatus = IDLE_NEED_GC;

U32 L2_TaskEventHandle(void)
{
    U32 Ret;
    COMMON_EVENT L2_Event;
    COMM_EVENT_PARAMETER * pParameter;
    Ret = COMM_EVENT_STATUS_BLOCKING;

    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event))
    {
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    if (L2_Event.EventSaveBBT)
    {
        if (FALSE == L2_BbtIsSavedDone())
        {
            L2_BbtSchedule();
        }
        else
        {
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_BBT);
            Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
    }
    else if (L2_Event.EventLLF)
    {
        Ret = L2_HandleEventLLF();
    }
    else if (L2_Event.EventSaveRT)
    {
        if (L2_RT_WAIT != L2_RT_SaveRT(FALSE))
        {
            /* clear the RT status */
            L2_RT_ResetSaveStatus();

            /* Clear Evnet */
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVERT);
            Ret = COMM_EVENT_STATUS_BLOCKING;
        }
    }
    else if (L2_Event.EventSavePbit)
    {
        CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
        if (TRUE == L2_TB_SavePbitBlock(&(pParameter->EventParameterNormal[3])))
        {
            /* Clear Evnet */
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_PBIT);
            Ret = COMM_EVENT_STATUS_BLOCKING;
        }
    }
    else if (L2_Event.EventShutDown)
    {
        Ret = L2_HandleEventShutDown();
    }
    else if (L2_Event.EventBoot)
    {
        Ret = L2_HandleEventBoot();
    }
    else if (L2_Event.EventBootAfterLoadRT)
    {
        Ret = L2_HandleEventBootAfterLoadRT();
    }
    else if (L2_Event.EventRebuild)
    {
        Ret = L2_HandleEventRebuild();
    }
    else if (L2_Event.EventRebuildDirtyCnt)
    {
        Ret = L2_HandleEventRebuildDirtyCnt();
    }
    else if (L2_Event.EventIdle)
    {
        Ret = L2_HandleEventIdle();
    }
    else if (L2_Event.EventSelfTest)
    {
        L2_DbgShowAll();

        /* Clear L2 Evnet */
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SELFTEST);

        /*Set L3 Self-Test Event*/
        CommSetEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_SELFTEST);

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else if (L2_Event.EventErrorHandling)
    {
        //L2_ErrorHandlingEntry();
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_ERR);
    }
    else if (L2_Event.EventDbg) /* add by henryluo 2010-09-10 for L2 Debug */
    {
        if (SUCCESS == L2_DbgEventHandler())
        {
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_DBG);
            Ret = COMM_EVENT_STATUS_BLOCKING;
        }
    }
    else if (L2_Event.EventSaveTable)
    {
        Ret = L2_HandleEventSaveTable();
    }

    return Ret;
}

static U32 MCU1_DRAM_TEXT L2_HandleEventLLF(void)
{
    BOOL bSecurityErase;
    BOOL bKeepEraseCnt;
    U32 Ret;
    U32 Sta;
    COMM_EVENT_PARAMETER * pParameter;

    Ret = COMM_EVENT_STATUS_BLOCKING;
    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
    bKeepEraseCnt = pParameter->EventParameterNormal[0];
    bSecurityErase = pParameter->EventParameterNormal[1];

    if (FALSE == g_L2EventInit)
    {
        L2_LLFInit();

#ifdef DBG_PMT
        L2_InitDebugPMT();
#endif
        if (TRUE == bSecurityErase)
        {
            U8 i;

            L2_InitDataStructures(bSecurityErase);
            L2_InitTLCInverseProgOrderTable();
            for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
            {
                L2_InitScheduler(i);
                L2_InitFTLScheduler(i);
            }
        }
        g_L2EventInit = TRUE;
    }

    Sta = L2_LLF(bKeepEraseCnt, bSecurityErase);

    if (L2_LLFSuccess == Sta)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_LLF);
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;

        g_L2EventInit = FALSE;
    }
    else if (L2_LLFPending == Sta)
    {
        Ret = COMM_EVENT_STATUS_BLOCKING;
    }
    else if (L2_LLFFail == Sta)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_LLF);
        Ret = COMM_EVENT_STATUS_FAIL;

        g_L2EventInit = FALSE;
    }

    pParameter->EventStatus = Ret;
    return Ret;

}
static U32 L2_HandleEventShutDown(void)
{
    U32 Ret;
    //U32 Sta;
    U8 ucPuNum;
    COMM_EVENT_PARAMETER * pParameter;

    Ret = COMM_EVENT_STATUS_BLOCKING;
    g_L2EventStatus.m_Shutdown = TRUE;
    if (FALSE == g_L2EventInit)
    {
        if (L2_IsFTLIdle() == FALSE)
        {
            g_L2EventStatus.m_WaitIdle = TRUE;
            g_L2EventStatus.m_ForceIdle = FALSE;
            Ret = COMM_EVENT_STATUS_GET_EVENTPEND;
        }
        else
        {
            /*wait erase task done*/
            for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
            {
                if (FALSE == L2_FTLWaitAllEraseDone(ucPuNum))
                {
                    return Ret;
                }
                //L2_EraseQueueShutdown(ucPuNum);
            }           

            for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
            {
                L2_SetThreadQuota(ucPuNum, SYS_THREAD_FTL, 0);
                L2_InitFTLScheduler(ucPuNum);
                g_PMTFlushManager[ucPuNum].m_LastDevWriteCntOfCE = g_PMTFlushManager[ucPuNum].m_DevWriteCntOfCE;

                /*unlock SLCGC SRCBLK*/
                if (INVALID_4F != g_GCManager[SLCGC_MODE]->tGCCommon.m_SrcPBN[ucPuNum])
                {
                    L2_PBIT_Set_Lock(ucPuNum, g_GCManager[SLCGC_MODE]->tGCCommon.m_SrcPBN[ucPuNum], FALSE);
#ifdef DBG_LC
                    if (0 != lc.uLockCounter[ucPuNum][DATA_GC])
                    {
                        lc.uLockCounter[ucPuNum][DATA_GC]--;
                        //FIRMWARE_LogInfo("SuperPu %d L2_HandleEventShutDown --\n", ucPuNum);
                    }
                    else
                    {
                        DBG_Getch();
                    }
#endif
                }

                /*Init SLCGCManager in case of WriteAfterShutDown accompany with SLCGC*/
                L2_InitGCManager(ucPuNum, SLCGC_MODE);

                /* unlock BLKs which are in ForceGCQue */
                L2_BlkQueueRemoveAll(ucPuNum, g_pForceGCSrcBlkQueue[ucPuNum]);
            }
            bShutdownPending = TRUE;
            g_L2EventInit = TRUE;
            g_L2EventStatus.m_WriteAfterShutdown = FALSE;
            g_L2EventStatus.m_WaitIdle = FALSE;
        }
    }
    else
    {
        //if(TRUE == L1_BufReqFifoEmpty())
        {
            if (TRUE == bShutdownPending)
            {
                if (TRUE == g_RebuildDirtyFlag)
                {
                    DBG_Printf("Error: Rebuild not done in Shutdown event! \n");
                    DBG_Getch();
                }
                else
                {
                    /* ShutDown Start */
                    Ret = L2_ShutdownEntry();
                }
            }
            else
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
#if 0
                /* Shutdown for AT0 table and RT */
                Sta = L2_TableBlock_Shutdown();

                if (L2_TB_WAIT == Sta)
                {
                    Ret = COMM_EVENT_STATUS_BLOCKING;
                }
                else if (L2_TB_SUCCESS == Sta)
                {
                    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
                    {
                        L2_SetSystemState(ucPuNum, SYSTEM_STATE_NORMAL);
                    }
                    CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SHUTDOWN);
                    Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
                    g_L2EventInit = FALSE;
                    //g_L2EventStatus.m_Shutdown = FALSE;
                }
                else
                {
                    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
                    {
                        L2_SetSystemState(ucPuNum, SYSTEM_STATE_NORMAL);
                    }
                    CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SHUTDOWN);
                    Ret = COMM_EVENT_STATUS_FAIL;
                    g_L2EventInit = FALSE;
                }
#endif
                CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SHUTDOWN);
                Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
                g_L2EventInit = FALSE;

                pParameter->EventStatus = Ret;
            }
        }
    }

    return Ret;

}

static U32 L2_HandleEventSaveTable(void)
{
    U32 Ret;
    U32 Sta;
    U8 ucPuNum;
    COMM_EVENT_PARAMETER * pParameter;

    Ret = COMM_EVENT_STATUS_BLOCKING;

    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);

    /* Shutdown for AT0 table and RT */
    Sta = L2_TableBlock_Shutdown();

    if (L2_TB_WAIT == Sta)
    {
        Ret = COMM_EVENT_STATUS_BLOCKING;
    }
    else if (L2_TB_SUCCESS == Sta)
    {
        for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
        {
            L2_SetSystemState(ucPuNum, SYSTEM_STATE_NORMAL);
        }
        g_L2EventStatus.m_ShutdownDone = TRUE;
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVETABLE);
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else
    {
        for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
        {
            L2_SetSystemState(ucPuNum, SYSTEM_STATE_NORMAL);
        }
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVETABLE);
        Ret = COMM_EVENT_STATUS_FAIL;
    }

    pParameter->EventStatus = Ret;
    return Ret;
}

static U32 L2_HandleEventBoot(void)
{
    U8 PUSer;
    U32 Sta;
    U32 Ret;
    COMM_EVENT_PARAMETER * pParameter;

    Ret = COMM_EVENT_STATUS_BLOCKING;

    if (FALSE == g_L2EventInit)
    {
#ifdef DCACHE
        HAL_InvalidateDCache();
#endif

        /* Set system state to boot */
        for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
        {
            L2_SetSystemState(PUSer, SYSTEM_STATE_BOOT_PENDING);
        }

        bBbtBootLoadStatus = FALSE;
        g_L2EventInit = TRUE;
    }

    /*boot up load  BBT*/
    if (FALSE == bBbtBootLoadStatus)
    {
        if (FALSE == g_bL2PbnLoadDone)
        {
            if (TRUE == L2_BbtLoadPbnBindingTable())
            {
                g_bL2PbnLoadDone = TRUE;

                // enable the PBN binding table
                L2_BbtEnablePbnBindingTable();
            }
        }
        else
        {
            if (TRUE == L2_BbtLoad(NULL))
            {
                // check the validity of the PBN binding table
                //L2_BbtPbnBindingTableCheck();

                L2_BbtPrintAllBbt();
                //DBG_Printf("MCU#%d BBT Load Finished in BootEvent.\n", HAL_GetMcuId());
                bBbtBootLoadStatus = TRUE;
            }
        }
        Sta = L2_TB_WAIT;
    }
    else
    {
        Sta = L2_LoadRTBeforBoot();
    }

    if (L2_TB_WAIT == Sta)
    {
        Ret = COMM_EVENT_STATUS_BLOCKING;
    }
    else if (L2_TB_SUCCESS == Sta)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_BOOT);
        g_L2EventInit = FALSE;
        Ret = COMM_EVENT_STATUS_BLOCKING;
    }
    else  //next need table rebuild
    {
        Ret = COMM_EVENT_STATUS_BLOCKING;

#ifndef LCT_VALID_REMOVED
        //TB status
        L1_SetLCTValidLoadStatus(2);
#endif

        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_BOOT);
        g_L2EventInit = FALSE;
        Ret = COMM_EVENT_STATUS_FAIL;
        g_BootManagement->m_BootType = BOOT_ABNORMAL;
    }

    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
    pParameter->EventStatus = Ret;

    return Ret;

}

static U32 L2_HandleEventBootAfterLoadRT(void)
{
    U32 Sta;
    U32 Ret;
    COMM_EVENT_PARAMETER * pParameter;

    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);

    Ret = COMM_EVENT_STATUS_BLOCKING;

    if (FALSE == g_L2EventInit)
    {
        g_L2EventInit = TRUE;
    }

    Sta = L2_TableBlock_Boot();

    if (L2_TB_WAIT == Sta)
    {
        Ret = COMM_EVENT_STATUS_BLOCKING;
    }
    else if (L2_TB_SUCCESS == Sta)
    {
        /*resume important info */
        if (BOOT_METHOD_NORMAL == DiskConfig_GetBootMethod())
        {
            L2_NormalBootResumeInfo();
        }
#ifndef LCT_VALID_REMOVED
        //TB status
        L1_SetLCTValidLoadStatus(1);
#endif

        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_BOOT_AFTER_RT);
        //g_L2EventInit = FALSE;
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        g_L2EventInit = FALSE;
        pParameter->EventStatus = Ret;

        DBG_Printf("MCU%d BOOT done!\n", HAL_GetMcuId());
    }
    else  //next need table rebuild
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_BOOT_AFTER_RT);
        //g_L2EventInit = FALSE;
        //Ret = COMM_EVENT_STATUS_FAIL;

#ifndef LCT_VALID_REMOVED
        //TB status
        L1_SetLCTValidLoadStatus(2);
#endif

        //set table rebuld event
        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD);
        g_L2EventInit = FALSE;
        g_BootManagement->m_BootType = BOOT_ABNORMAL;
        pParameter->EventStatus = COMM_EVENT_STATUS_FAIL;
    }

    //CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
    //pParameter->EventStatus = Ret;

    return Ret;
}

BOOL L2_RebuildFullfillPartialPMTPPO()
{
    U32 uSuperPU;
    U32 uLUNInSuperPU;
    U32 uRet = FALSE;
    PhysicalAddr pAddr;
    PhysicalPage *pPage;  
    U32 uCurBlkSN;
    U32 uPMTIToWrite;
    RED *pSpare;
    U8 *pStatus;
    RebuildFullfillPartialPMTPPOState eState;
  
    for(uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        /* ppo not partial write bypass */
        if(0 == g_PMTManager->m_PMTBitMapSPOR[uSuperPU] )
        {
            uRet = TRUE;
            break;
        }
        
        eState = l_RebuildPMTDptr->m_RebuildFullfillPartialPMTPPOState[uSuperPU];
        switch(eState)
        {
        case Rebuild_FullfillPartialPMTPPO_Write:
        
            uCurBlkSN = g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurBlkSN;
                                 
            /*find PPO corresponding PMTI*/
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if(COM_BitMaskGet(g_PMTManager->m_PMTBitMapSPOR[uSuperPU], uLUNInSuperPU))
                {
                    uPMTIToWrite = l_RebuildPMTManagerDptr->m_TableBlklastPagePMTI[uSuperPU][uCurBlkSN][uLUNInSuperPU];
                    break;
                }
            }

            /* already written LUN set status to success */
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if (COM_BitMaskGet(g_PMTManager->m_PMTBitMapSPOR[uSuperPU], uLUNInSuperPU))
                {
                    pStatus = &l_BufferManagerDptr->m_BufferStatus[uSuperPU][0][uLUNInSuperPU];
                    *pStatus = SUBSYSTEM_STATUS_SUCCESS;
                }
            }
        
            /*fullfill unwritten LUN */
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {                                
                if(!COM_BitMaskGet(g_PMTManager->m_PMTBitMapSPOR[uSuperPU], uLUNInSuperPU))
                {                    
                    if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(uSuperPU, uLUNInSuperPU)))
                    {
                        continue;
                    }
                    pAddr.m_PUSer = uSuperPU;
                    pAddr.m_OffsetInSuperPage = uLUNInSuperPU;
                    pAddr.m_BlockInPU = l_RebuildPMTManagerDptr->m_TableBlk[uSuperPU][uCurBlkSN][uLUNInSuperPU];
                    pAddr.m_PageInBlock = g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurPPO-1;
#ifdef PMT_ITEM_SIZE_REDUCE
					pPage= &((SuperPage*)g_PMTManager->m_pPMTPage[uSuperPU][uPMTIToWrite])->m_Content[uLUNInSuperPU];
#else
					pPage= &g_PMTManager->m_pPMTPage[uSuperPU][uPMTIToWrite]->m_Page.m_Content[uLUNInSuperPU];
#endif
                    pSpare = l_BufferManagerDptr->m_SpareBuffer[uSuperPU][0][0][uLUNInSuperPU];
                    pSpare->m_RedComm.eOPType = OP_TYPE_DUMMY_WRITE;
                    pSpare->m_RedComm.bcPageType = PAGE_TYPE_DUMMY;
                    pSpare->m_RedComm.bcBlockType = BLOCK_TYPE_PMT;

                    pStatus = &l_BufferManagerDptr->m_BufferStatus[uSuperPU][0][uLUNInSuperPU];
                    DBG_Printf("[%s]dummy wr Lun%d blk%d pg%d\n", __FUNCTION__, uLUNInSuperPU, pAddr.m_BlockInPU, pAddr.m_PageInBlock);
                    L2_FtlWriteLocal(&pAddr, (U32 *)pPage, (U32 *)pSpare, pStatus, TRUE, TRUE, NULL);
                    COM_BitMaskSet(&g_PMTManager->m_PMTBitMapSPOR[uSuperPU], uLUNInSuperPU);
                }
            }
            if (SUPERPU_LUN_NUM_BITMSK == g_PMTManager->m_PMTBitMapSPOR[uSuperPU])
            {
                l_RebuildPMTDptr->m_RebuildFullfillPartialPMTPPOState[uSuperPU] = Rebuild_FullfillPartialPMTPPO_WaitStatus;
            }
            break;
        case Rebuild_FullfillPartialPMTPPO_WaitStatus:
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {                            
                pStatus = &l_BufferManagerDptr->m_BufferStatus[uSuperPU][0][uLUNInSuperPU];
                if(SUBSYSTEM_STATUS_PENDING == *pStatus)
                {
                    break;
                }                                     
            }

            if(SUBSYSTEM_STATUS_PENDING != *pStatus)
            {
                g_PMTManager->m_PMTBitMap[uSuperPU] = 0;
                uRet = TRUE;
            }
            break;
        }
    }
    return uRet;    
}

static U32 L2_HandleRebuildState_RebuildPMT(void)
{
    U32 Ret = COMM_EVENT_STATUS_BLOCKING;    
    U32 uSuperPU;
    RebuildPMTState PMTState;

    PMTState = L2_RebuildGetPMTState();
    Ret = COMM_EVENT_STATUS_BLOCKING;
    switch (PMTState)
    {
    case Rebuild_PMT_LoadPMT:
        if (L2_RebuildLoadPMT() == TRUE)
        {
            if (TRUE == g_bPMTErrFlag)
            {
                /* Executes rebuilding process for L2 */
                L2_Init_before_rebuild();
                L2_TBManagement_Init();
                L2_TBRebManagement_Init();
                g_bPMTErrFlag = TRUE;
                g_L2EventInit = TRUE;
                g_BootManagement->m_BootType = BOOT_ABNORMAL;

                Ret = COMM_EVENT_STATUS_BLOCKING;
                return Ret;
            }

            //Finish Load all PMT
            L2_RebuildSetPMTState(Rebuild_PMT_Prepare);
        }

        Ret = COMM_EVENT_STATUS_BLOCKING;
        break;

    case Rebuild_PMT_Prepare:
        for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
        {
            //whether normal boot. If not ,must rebuild PMT
            if (L2_IsPMTNeedRebuild(uSuperPU) || L2_IsHavePMTErrOfRebuild(uSuperPU))
            {
                L2_PrepareToRebuildPMT(uSuperPU);
            }

            ResetRebuildPMTStates(uSuperPU);
        }

        L2_RebuildSetPMTState(Rebuild_PMT_RebuildPMT);
        break;

    case Rebuild_PMT_RebuildPMT:
        L2_RebuildPMT();
        break;

    case Rebuild_PMT_Done:
        //L2_SetDirtyCntInRebuild();
        //L2_SetRebuildState(Rebuild_State_Done);

        L2_RebuildPuTimeStamp();

#ifdef DBG_TABLE_REBUILD
        L3_CheckPuInfo();
        L3_CheckPBITwithPuInfo();
#endif

        L2_RebuildSetPMTState(Rebuild_PMT_RebuildErrTLC);

        /* set here before response any host cmd for dirty cnt issue, add by henryluo 2015-05-11. */
        g_RebuildDirtyFlag = TRUE; 
        break;
    case Rebuild_PMT_RebuildErrTLC:
        if (TRUE == L2_RebuildErrTLCOfAllPu())
        {
            L2_RebuildSetPMTState(Rebuild_State_FullfillPartialPMTPPO);
        }
        break;
    case Rebuild_State_FullfillPartialPMTPPO:
        if(TRUE == L2_RebuildFullfillPartialPMTPPO())
        {
            g_RebuildPMTDone = TRUE;
            L2_SetRebuildState(Rebuild_State_ErrHandling);
        }
        break;
    }
    return Ret;
}

static U32 MCU1_DRAM_TEXT L2_HandleEventRebuild(void)
{
    U32 Ret;
    U8 PUSer;
    U32 Sta;
    COMM_EVENT_PARAMETER * pParameter;
    RebuildState CurrState;
    Ret = COMM_EVENT_STATUS_BLOCKING;
    U32 ulL2FreePageCnt;

    if (FALSE == g_L2EventInit)
    {
        U8 ucSuperPu;

#ifndef SIM
        /* MCU data cache must be invalidated since tables are loaded from flash into DRAM through DMA. */
#ifdef DCACHE
        HAL_InvalidateDCache();
#endif

        Rebuild_Start = XT_RSR_CCOUNT();
#else
        {
            U32 ulMcuId = HAL_GetMcuId();
            g_bTableRebuildFlag[ulMcuId - 2] = TRUE;
        }
#endif

        /* Executes rebuilding process for L2 */
        DBG_Printf("######Event rebuild begin!######\n");
        L2_Init_before_rebuild();

        for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
        {
            L2_FTLSetTableRebuildFlag(ucSuperPu, TRUE);
        }
        g_BootManagement->m_BootType = BOOT_ABNORMAL;

        /* move set g_L1PopCmdEn here */
        g_L1PopCmdEn = FALSE;
        g_L2EventInit = TRUE;
    }

    CurrState = L2_GetRebuildState();
    switch (CurrState)
    {
    case Rebuild_State_Init:
        L2_SetRebuildState(Rebuild_State_RebuildTB);
        Ret = COMM_EVENT_STATUS_BLOCKING;
        break;

    case Rebuild_State_RebuildTB:
        Sta = L2_TB_FullRecovery();
        if (L2_TB_WAIT == Sta)
        {
            Ret = COMM_EVENT_STATUS_BLOCKING;
        }
        else if (L2_TB_SUCCESS == Sta)
        {
#ifdef ASIC
            Rebuild_End = XT_RSR_CCOUNT();
            DBG_Printf("######L2_TB_FullRecovery end (totally cost %d mS) ######\n", CalcDiffTime(Rebuild_Start,Rebuild_End)/300000);
            Rebuild_Start = Rebuild_End;
#endif

            L2_RebuildPrepare();

#ifdef DBG_TABLE_REBUILD
            L3_CheckPBIT();
#endif
            Ret = COMM_EVENT_STATUS_BLOCKING;
        }
        else
        {
            CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);

            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD);
            g_L2EventInit = FALSE;

            Ret = COMM_EVENT_STATUS_FAIL;
            pParameter->EventStatus = Ret;

            /* MCU data cache must be invalidated since tables are loaded from flash into DRAM through DMA. */
#ifdef DCACHE
            HAL_InvalidateDCache();
#endif
        }
        break;

    case Rebuild_State_RebuildPMTI:
        L2_RebuildPMTI();
        Ret = COMM_EVENT_STATUS_BLOCKING;
        break;

    case Rebuild_State_RebuildPMTManager:
        L2_RebuildPMTManager();
        L2_SetRebuildState(Rebuild_State_RebuildPu);//Rebuild_State_RebuildPMT;
        Ret = COMM_EVENT_STATUS_BLOCKING;
        break;

    case Rebuild_State_RebuildPu:
        L2_RebuildPuInfo();

        ulL2FreePageCnt = L2_GetL2FreePage();
        g_ulAllowHostWriteSec = (ulL2FreePageCnt + L1_WRITE_CACHE_BUFFER) * SEC_PER_BUF;

        //L2_SetUninitalPuInRebuild();
#ifdef DBG_TABLE_REBUILD
        L3_RecordBlakePuInfo();
#endif
        L2_SetRebuildState(Rebuild_State_RebuildPMT);//Rebuild_State_RebuildPMT;

        Ret = COMM_EVENT_STATUS_BLOCKING;
        break;

    case Rebuild_State_RebuildPMT:
    
        Ret = L2_HandleRebuildState_RebuildPMT();    
        break;

    case Rebuild_State_ErrHandling:
    {
        U32 All_Finish_Flag;
        RebuildErrHandleState ErrHandleState;
        ErrHandleState = L2_RebuildGetErrHandleState();
        Ret = COMM_EVENT_STATUS_BLOCKING;

        L2_GlobalProcessRead();
        switch (ErrHandleState)
        {
        case Rebuild_ErrHandle_Prepare:
            /* Process ErrInfo, decide wich ce and blk need move */
            L2_PrepareErrHanle();
            //L2_DumpErrHandleMoveBlkInfo();
            L2_RebuildSetErrHandleStateState(Rebuild_ErrHandle_MoveBlkProcess);
            break;
        case Rebuild_ErrHandle_MoveBlkProcess:
            All_Finish_Flag = L2_RebuildErrHandleAllCE();
            if (All_Finish_Flag)
            {
                L2_RebuildSetErrHandleStateState(Rebuild_ErrHandle_Done);
                L2_SetRebuildState(Rebuild_State_Done);
            }
            break;

        default:
            break;
        }
    }
        break;

    case Rebuild_State_Done:

        L2_GlobalProcessRead();

#ifdef SIM
        Dump_RebuildErrInfoOfRebuild();
#endif

        L2_RebuildFlushManager();

        Ret = COMM_EVENT_STATUS_BLOCKING;

        /* MCU data cache must be invalidated since tables are loaded from flash into DRAM through DMA. */
#ifdef DCACHE
        HAL_InvalidateDCache();
#endif

#ifdef DBG_PMT
        L2_CheckPMT();
#endif

#ifdef DBG_TABLE_REBUILD
        //Ingore some blk of old lpn to check VBT
#ifdef DBG_PMT
        L3_UpdateIgnoredBlkOfCheckVBT(g_SimRstLPNOfPMTCheck.IgnoredLPN, g_SimRstLPNOfPMTCheck.LPNCnt);
#endif
        L3_CheckVBT();
#endif

        /* entry boot thread to calc dirty count */
        DBG_Printf("rebuild PMT done! \n");
        L2_SetupRebuildDrityCntFlag();
        L2_SetCanDoFTLWriteFlag();

        for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
        {
            L2_SetSystemState(PUSer, SYSTEM_STATE_BOOT_PENDING);
        }

#ifdef SIM
            {
                U32 ulMcuId = HAL_GetMcuId();
                g_bTableRebuildFlag[ulMcuId - 2] = FALSE;
            }
#else
        Rebuild_End = XT_RSR_CCOUNT();
        DBG_Printf("###### rebuild PMT (totally cost %d mS) ######\n", CalcDiffTime(Rebuild_Start,Rebuild_End)/300000);
        Rebuild_Start = Rebuild_End;
#endif

        //L2_SetRebuildState(Rebuild_State_RebuildDirtyCnt);
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD);
        g_L2EventInit = FALSE;

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
        pParameter->EventStatus = Ret;

        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD_DIRTYCNT);


        //g_L1PopCmdEn = FALSE;
        g_L2RebuildDC = TRUE;
        //HAL_StartMcuTimer(800000, NULL);
        break;

    default:
        break;
    }
    return Ret;

}

static U32 MCU1_DRAM_TEXT L2_HandleEventRebuildDirtyCnt(void)
{
    U32 Ret;
    L2_GlobalProcessRead();

    /* calc dirty count */
    Ret = L2_AbnormalBootEntry();

    //all finish
    if (TRUE == g_BootUpOk)
    {
        CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD_DIRTYCNT);
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;

#ifdef ASIC
        Rebuild_End = XT_RSR_CCOUNT();
        DBG_Printf("###### rebuild dirty count (totally cost %d mS) ######\n", CalcDiffTime(Rebuild_Start, Rebuild_End)/300000);
#endif
        g_L1PopCmdEn = TRUE;
        g_L2RebuildDC = FALSE;
        //HAL_StopMcuTimer();
    }

    return Ret;
}
void L2_SetL3IdleEvent(U32 ulIdleParam)
{
    COMM_EVENT_PARAMETER *pParameter;

    CommGetEventParameter(COMM_EVENT_OWNER_L3, &pParameter);
    pParameter->EventParameterNormal[0] = ulIdleParam;

    CommSetEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_IDLE);    
    return;
}

void L2_WaitL3IdleEventDone()
{
    COMMON_EVENT Event;

    do
    {
        CommCheckEvent(COMM_EVENT_OWNER_L3, &Event);
    } while (TRUE == Event.EventIdle);

    return;
}

static U32 L2_HandleEventIdle(void)
{
    U32 Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    U8 ucPatrolReadSts;
    COMM_EVENT_PARAMETER *pParameter;

    U8 ucNeedSchedule = FALSE;

    /* 0: Idle Normal; 1: Idle Force; 2: Idle Error Recovery */
    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
    switch (pParameter->EventParameterNormal[0])
    {
    case 0:
        /* For support L0 SelfWake */
        g_L2EventStatus.m_ForceIdle = FALSE;

        if ((TRUE == L2_IsFTLIdle()) && (FALSE == g_L2EventStatus.m_ShutdownDone))
        {
            switch (l_ucIdleStatus)
            {
            case IDLE_NEED_GC:
                
                if (FALSE != L2_IdleGCEntry())
                {
                    l_ucIdleStatus = IDLE_FLUSH_PMTPAGE;
                }
                else
                {
                    Ret = COMM_EVENT_STATUS_GET_EVENTPEND;
                    break;
                }
            case IDLE_FLUSH_PMTPAGE:
                
                /* Idle one time, flush one table pages */
                if (TRUE == L2_IdleFlushPMTPage(1))
                {
                    l_ucIdleStatus = IDLE_PATROL_READ;
                }
                else
                {
                    Ret = COMM_EVENT_STATUS_GET_EVENTPEND;
                    break;
                }
            case IDLE_PATROL_READ:
                ucPatrolReadSts = L2_PatrolRead();
                if (PATROL_READ_WAIT == ucPatrolReadSts)
                {
                    Ret = COMM_EVENT_STATUS_BLOCKING;
                    break;
                }
                else if (PATROL_READ_ONE_PAGE_DONE == ucPatrolReadSts)
                {
                    Ret = COMM_EVENT_STATUS_BLOCKING;
                    break;
                }
                else if ((PATROL_READ_ONE_BLK_DONE == ucPatrolReadSts)
                    || (PATROL_READ_ONE_SCAN_DONE == ucPatrolReadSts)
                    || (PATROL_READ_WAIT_TIME_INTERVAL == ucPatrolReadSts))
                {
                    l_ucIdleStatus = IDLE_FTL_BUILD_REQ;
                }
                else
                {
                    DBG_Getch();
                }

            case IDLE_FTL_BUILD_REQ:

                /* set L3 Idle Event*/
                L2_SetL3IdleEvent(FCMD_REQ_SUBTYPE_IDLE_0);
                L2_WaitL3IdleEventDone();

                l_ucIdleStatus = IDLE_NEED_GC;

                break;
            default:
                DBG_Getch();
            }
        }

        if (Ret == COMM_EVENT_STATUS_SUCCESS_NOEVENT)
        {
            L2_IdleGCResetManager();
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_IDLE);
        }
        break;

    case 1:
        /* wait L2 idle */
        if (TRUE == L2_IsFTLIdle())
        {
            g_L2EventStatus.m_ForceIdle = TRUE;
            g_L2EventStatus.m_WaitIdle = FALSE;
            L2_ClearAllPUFTLScheduler();

            /* set L3 Idle Event*/
            L2_SetL3IdleEvent(FCMD_REQ_SUBTYPE_IDLE_1);
            L2_WaitL3IdleEventDone();
        }
        else
        {
            g_L2EventStatus.m_WaitIdle = TRUE;
            Ret = COMM_EVENT_STATUS_GET_EVENTPEND;
        }

        if (Ret == COMM_EVENT_STATUS_SUCCESS_NOEVENT)
        {
            L2_IdleGCResetManager();
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_IDLE);
        }
        break;

    case 2:
        /* wait L2 idle */
        if (TRUE == L2_IsFTLIdle())
        {
            g_L2EventStatus.m_ForceIdle = TRUE;
            g_L2EventStatus.m_WaitIdle = FALSE;
            L2_ClearAllPUFTLScheduler();

            /* set L3 Idle Event*/
            L2_SetL3IdleEvent(FCMD_REQ_SUBTYPE_IDLE_2);
            L2_WaitL3IdleEventDone();
        }
        else
        {
            g_L2EventStatus.m_WaitIdle = TRUE;
            Ret = COMM_EVENT_STATUS_GET_EVENTPEND; //return FTL_STATUS_OK;
        }

        if (Ret == COMM_EVENT_STATUS_SUCCESS_NOEVENT)
        {
            L2_IdleGCResetManager();
            CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_IDLE);
        }
        break;

    default:
        break;
    }

    return Ret;

}

/********************** FILE END ***************/

