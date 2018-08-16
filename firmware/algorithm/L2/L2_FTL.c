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
Filename    :L2_FTL.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.6
Description :functions about Read and Write process
Others      :
Modify      :CloudsZhang: Update for multi thread code
****************************************************************************/
#include "HAL_Inc.h"
#include "HAL_FlashDriverBasic.h"
#include "COM_BitMask.h"
#include "L1_Interface.h"
#include "L2_Defines.h"
#include "L2_FTL.h"
#include "L2_StripeInfo.h"
#include "L2_Thread.h"
#include "L2_PMTManager.h"
#include "L2_GCManager.h"
#include "L2_RT.h"
#include "L2_TableBlock.h"
#include "L2_Trim.h"
#include "L2_StaticWL.h"
#include "L2_Boot.h"
#include "COM_Memory.h"
#include "HAL_TraceLog.h"
#include "L2_DWA.h"
#include "L2_Erase.h"
#include "L2_Schedule.h"
#include "L2_FCMDQ.h"
#include "L2_TLCMerge.h"
#include "L2_Interface.h"
#include "FW_Event.h"
#include "L2_ReadDisturb.h"
#ifdef SIM
#include "L2_ErrorHandling.h"
#endif

#if (defined(L2MEASURE) || (defined(SWL_EVALUATOR)))
#include "L2_Evaluater.h"
extern GLOBAL MCU12_VAR_ATTR SWLRecord *g_SWLRecord[SUBSYSTEM_SUPERPU_MAX];
#endif

#ifndef SIM
#include <xtensa/tie/xt_timer.h>
#endif

#ifdef DATA_MONITOR_ENABLE
#include "FW_BufAddr.h"
#include "FW_DataMonitor.h"
#endif

#if defined(HOST_AHCI)
#include "L2_Ahci.h"
#elif defined(HOST_SATA)
#include "L2_Sata.h"
#elif defined(HOST_NVME)
#include "L2_NVMe.h"
#endif

//specify file name for Trace Log
#define TL_FILE_NUM  L2_FTL_c

//add for TLC Read ErrorHandling
extern GLOBAL MCU12_VAR_ATTR U32 g_ulDramStartAddr;

#define FlushPMTThs  64
GLOBAL U32 g_ulPMTFlushRatio = FlushPMTThs;
GLOBAL U32 g_ulPMTFlushing = FALSE;
extern void L2_DbgShowAll();

U32 g_ulPopLunBitMap[SUBSYSTEM_SUPERPU_MAX] = { 0 };
GLOBAL  L2_FTLDptr *g_FTLDptr;
GLOBAL  L2_FTLReadDptr (*g_FTLReadDptr)[LUN_NUM_PER_SUPERPU];
GLOBAL  PMTFlushStatistic *g_PMTFlushManager;


extern GLOBAL  U32 g_L2TempBufferAddr;
extern GLOBAL  U32 g_DWASustainWriteThs;
#ifndef LCT_TRIM_REMOVED
extern GLOBAL  U32 g_ulPendingTrimLctCount;
#endif

extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;

LOCAL  U32 g_IdleFlushPos[SUBSYSTEM_SUPERPU_MAX] = { 0 };
LOCAL  FTL_SCHEDULER l_FTLScheduler[SUBSYSTEM_SUPERPU_MAX];

GLOBAL FTL_IDLE_GC_MGR g_tFTLIdleGCMgr;
LOCAL U8 l_ucIdleGCStatus = IDLE_GC_PREPARE;

extern GLOBAL U32 g_ulL2DummyDataAddr;
extern GLOBAL U32 g_WLAccDegree;

GLOBAL FTL_ERASE_STATUS_MGR * l_ptFTLEraseStatusManager;

extern GLOBAL BOOL L2_IsEraseQueueEmpty(U8 ucSuperPuNum);
extern GLOBAL BOOL L2_EraseQueueLastVBNIsTLC(U8 ucSuperPuNum);
extern TableQuitStatus L2_TableFlush(U8 ucSuperPu);
extern void L2_ForceFlushPMTSetting(U8 ucSuperPu);

U8 L2_GetPuFromAddr(PhysicalAddr* pAddr)
{
    return L2_GET_TLUN(pAddr->m_PUSer, pAddr->m_OffsetInSuperPage);
}

/*****************************************************************************
 Prototype      : L2_InitFTLScheduler
 Description    : init FTL scheduler
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/26
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
void MCU1_DRAM_TEXT L2_InitFTLScheduler(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_WRITE] = 0;
    l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_SLCGC] = 0;

    l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_TLCGC] = 0;
    l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_WL] = 0;
    l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_GC_2TLC] = 0;

    l_FTLScheduler[ucSuperPu].eCurrThread = FTL_THREAD_SLCGC;
    l_FTLScheduler[ucSuperPu].usThreadQuotaUsed = 0;
    l_FTLScheduler[ucSuperPu].bPassOneW = FALSE;
    l_FTLScheduler[ucSuperPu].bPassQuota = FALSE;
    l_FTLScheduler[ucSuperPu].bTableRebuildFlag = FALSE;
    l_FTLScheduler[ucSuperPu].bChkSPORSLCAllDFlag = FALSE;
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag = 0;

    return;
}

void L2_ClearAllPUFTLScheduler(void)
{
    U8 ucTaskBitOffset;
    U8 ucSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_WRITE] = 0;
        l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_SLCGC] = 0;
        //FIRMWARE_LogInfo("SuperPu %d SLCGZero 11\n", ucSuperPu);

        l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_TLCGC] = 0;
        l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_WL] = 0;
        l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_GC_2TLC] = 0;

        l_FTLScheduler[ucSuperPu].eCurrThread = FTL_THREAD_SLCGC;

        /*clean current TLCArea remain task*/
        for (ucTaskBitOffset = TASK_TLCGC_BIT; ucTaskBitOffset <= TASK_TLCSWL_BIT; ucTaskBitOffset++)
        {
            l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag &= ~(1 << ucTaskBitOffset);
        }
    }
    return;
}

BOOL L2_FTLSchedulerIsGC(U8 ucSuperPu)
{
    if (l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_SLCGC] != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void L2_FTLSetTableRebuildFlag(U8 ucSuperPu, BOOL bFlagVal)
{
    l_FTLScheduler[ucSuperPu].bTableRebuildFlag = bFlagVal;
    return;
}

BOOL L2_FTLGetTableRebuildFlag(U8 ucSuperPu)
{
    return l_FTLScheduler[ucSuperPu].bTableRebuildFlag;
}


void L2_FTLSetSPORSLCAllDFlag(U8 ucSuperPu, BOOL bFlagVal)
{
    l_FTLScheduler[ucSuperPu].bChkSPORSLCAllDFlag = bFlagVal;
    return;
}

BOOL L2_FTLGetSPORSLCAllDFlag(U8 ucSuperPu)
{
    return l_FTLScheduler[ucSuperPu].bChkSPORSLCAllDFlag;
}

BOOL L2_FTLIsAllTaskDone(U8 ucSuperPu)
{
    return (0 == l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag) ? TRUE : FALSE;
}

U8 L2_FTLGetTaskBitInfo(U8 ucSuperPu)
{
    return l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag;
}

void L2_FTLTaskSLCGCSet(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag |= (1<<TASK_SLCGC_BIT);
    return;
}

void L2_FTLTaskTLCMergeSet(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag |= (1<<TASK_TLCMERGE_BIT);
    return;
}

void L2_FTLTaskTLCGCSet(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag |= (1<<TASK_TLCGC_BIT);
    return;
}

BOOL L2_FTLIsTaskSet(U8 ucSuperPu, U8 TaskBitOffset)
{
    return (l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag & (1<<TaskBitOffset)) ? TRUE:FALSE;
}

BOOL L2_FTLIsHaveTLCAreaTask(U8 ucSuperPu)
{
    U8 ucCurSchTLCTaskBitFlag;

    ucCurSchTLCTaskBitFlag = l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag;
    ucCurSchTLCTaskBitFlag &= ~ (1<< TASK_SLCGC_BIT);

    return (0 == ucCurSchTLCTaskBitFlag)? FALSE:TRUE;
}

BOOL L2_FTLIsHaveSLCAreaTask(U8 ucSuperPu)
{
    return L2_FTLIsTaskSet(ucSuperPu, TASK_SLCGC_BIT);
}

void L2_FTLTaskTLCSWLSet(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag |= (1<<TASK_TLCSWL_BIT);
    return;
}


void L2_FTLTaskSLCGCClear(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag &= ~(1<<TASK_SLCGC_BIT);
    return;
}

void L2_FTLTaskTLCMergeClear(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag &= ~(1<<TASK_TLCMERGE_BIT);
    return;
}

void L2_FTLTaskTLCGCClear(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag &= ~(1<<TASK_TLCGC_BIT);
    return;
}

void L2_FTLTaskTLCSWLClear(U8 ucSuperPu)
{
    l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag &= ~(1<<TASK_TLCSWL_BIT);
    return;
}

BOOL L2_CanServiceHost(U8 ucSuperPu)
{
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPu];

    if ((L2_GetSLCAreaFreePageCnt(ucSuperPu)+ (L2_GetEraseQueueSize(ucSuperPu, TRUE) * PG_PER_SLC_BLK)) <= (RESERVED_PAGE_CNT + BOOT_STRIPE_CNT))
    {
        return FALSE;
    }
    return TRUE;
}

BOOL L2_IsMultiPlnRdProcess(U8 ucSecStart, U8 ucSecLen)
{
    if ((ucSecStart % SEC_PER_LOGIC_PG + ucSecLen ) > SEC_PER_LOGIC_PG)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


void L2_FTLThreadProcessTaskConfirm(U8 ucSuperPu)
{
    U32 ulTLCFreeBlockNum;
    FTL_THREAD_TYPE eFTLThread;

    /*TLCGC task confirm:
     case1: avoid TLC GC task enable if TLC merge/SWL not finish after force idle 
     case2: ForceIdle clear all task bits, but normal shutdown will wait TLCGC done, so set TLCGC Task bit again*/
    if (((TRUE == L2_IsNeedTLCGC(ucSuperPu) && (TLC_WRITE_HOSTWRITE != g_TLCManager->aeTLCWriteType[ucSuperPu]) && (TLC_WRITE_SWL != g_TLCManager->aeTLCWriteType[ucSuperPu])))
        || (TLC_WRITE_TLCGC == g_TLCManager->aeTLCWriteType[ucSuperPu]))
    {
        L2_FTLTaskTLCGCSet(ucSuperPu);
    }
    
    /*ForceIdle clear all task bits, but normal shutdown will wait TLCMerge done, so set TLCMerge Task bit again*/
    if (TLC_WRITE_HOSTWRITE == g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
        L2_FTLTaskTLCMergeSet(ucSuperPu);
    }
    else if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
        L2_FTLTaskTLCSWLSet(ucSuperPu);
    }
    /*SLCGC or SLCMerge task confirm*/
    else if ((TRUE == L2_IsNeedSLCGC(ucSuperPu)) || (TRUE == L2_IsNeedIdleSLCGC(ucSuperPu) ))
    {
        /*select min SN SLCBLK*/
        eFTLThread = SelectSLCGCBlock(ucSuperPu, VBT_TYPE_HOST);
        if (FTL_THREAD_GC_2TLC == eFTLThread) // write to TLC Area, TLC Merge
        {
            /*only do TLCMerge when TLC FREE BLOCK enough*/
            ulTLCFreeBlockNum = g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_TLC] - g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC];
            ulTLCFreeBlockNum = ulTLCFreeBlockNum + L2_GetEraseQueueSize(ucSuperPu, FALSE);
            if (ulTLCFreeBlockNum >= GC_THRESHOLD_TLCNORMAL)
            {
                L2_FTLTaskTLCMergeSet(ucSuperPu);
            }
        }
        else if (FTL_THREAD_SLCGC == eFTLThread)
        {
            L2_FTLTaskSLCGCSet(ucSuperPu);
        }
    }

    /*SWL task confirm*/
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    if ((INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu]) && (TLC_WRITE_HOSTWRITE != g_TLCManager->aeTLCWriteType[ucSuperPu]) && (TLC_WRITE_TLCGC != g_TLCManager->aeTLCWriteType[ucSuperPu]))
#else
    if (INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu])
#endif
    {
        L2_FTLTaskTLCSWLSet(ucSuperPu);
    }

    /*if (0 != l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag)
    {
        FIRMWARE_LogInfo("SuperPU %d  FTLTaskInfo 0x%x\n", ucSuperPu, l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag);
    }*/

    return;
}

void L2_FTLThreadSetAllQuotaZero(U8 ucSuperPu)
{
    U16 * pSuperPuQuota;
    U32 ulFTLType;
    
    pSuperPuQuota = l_FTLScheduler[ucSuperPu].aThreadQuota;
    for (ulFTLType = 0; ulFTLType < FTL_THREAD_TYPE_ALL; ulFTLType++)
    {
        pSuperPuQuota[ulFTLType] = 0;
    }

    return;
}

BOOL L2_FTLThreadIsAllQuotaZero(U8 ucSuperPu)
{
    U16 * pSuperPuQuota;
    U8 ucFTLThreadIdx;
    
    pSuperPuQuota = l_FTLScheduler[ucSuperPu].aThreadQuota;
    for (ucFTLThreadIdx = 0; ucFTLThreadIdx < FTL_THREAD_TYPE_ALL; ucFTLThreadIdx++)
    {
        if ((0 != pSuperPuQuota[ucFTLThreadIdx]) && (FTL_THREAD_WRITE != ucFTLThreadIdx))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

void L2_FTLThreadCalTaskQuota(U8 ucSuperPu)
{
    U16 * pSuperPuQuota;
    U32 ulTLCWriteRealPage = 0;
    U32 ulHostWriteRealPage;
    U32 ulSLCFreePageForHostWrite;
    U8 ucTaskIndex;
#ifdef SIM
    if ((TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_SLCGC_BIT))
        && (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT)))
    {
        DBG_Printf("SLCGC and TLCMerge can't be all done in one process\n");
        DBG_Getch();
    }
#endif

    pSuperPuQuota = l_FTLScheduler[ucSuperPu].aThreadQuota;

    /*step1: cal TLCWriteRealPage*/
    if (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCGC_BIT))
    {
        ulTLCWriteRealPage += PG_PER_WL * PG_PER_SLC_BLK;
    }
    if (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT))
    {
        ulTLCWriteRealPage += PG_PER_WL * PG_PER_SLC_BLK;
    }

    /*step2:Cal HostWrite real page number*/
    ulSLCFreePageForHostWrite = L2_GetSLCAreaFreePageCnt(ucSuperPu) + (L2_GetEraseQueueSize(ucSuperPu, TRUE) * PG_PER_SLC_BLK)
        - RESERVED_PAGE_CNT - BOOT_STRIPE_CNT;
    if (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_SLCGC_BIT))
    {
        pSuperPuQuota[FTL_THREAD_SLCGC] = g_GCManager[SLCGC_MODE]->m_NeedCopyForOneWrite[ucSuperPu];
        ulHostWriteRealPage = (U32)g_GCManager[SLCGC_MODE]->m_HostWriteRealPage[ucSuperPu];
    }
    else if (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT))
    {
        ulTLCWriteRealPage += PG_PER_WL * PG_PER_SLC_BLK;
        ulHostWriteRealPage = min(ulSLCFreePageForHostWrite, PG_PER_WL * PG_PER_SLC_BLK);
    }
    else  //no SLCGC and TLCMERGER
    {
        ulHostWriteRealPage = (ulSLCFreePageForHostWrite >= PG_PER_SLC_BLK) ? PG_PER_SLC_BLK : ulSLCFreePageForHostWrite;

        /* DannierChen : 1) tlcgc or 2) (tlcgc + swl), 3) only hostw
         * 1) tlc blk < THS (no =) and slc blk = (THS+1), to current quota is 1:1 or 2:1, to avoid slc area out of free pages, it should set quota from 1:3+1 to 1:(3+1)*(v/d)
         * 2) also
         */
        if (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCGC_BIT) && TRUE == L2_IsNeedSLCGC(ucSuperPu))
        {
            U16 BlockSN;
            U32 usDirtyCnt;
            U32 ulValidPageInPU;
            U32 ulValidDirty;

            GCManager* pGCManager;
            pGCManager = g_GCManager[TLCGC_MODE];

            BlockSN = L2_SelectGCBlock(ucSuperPu, VBT_TYPE_TLCW);
            usDirtyCnt = L2_GetDirtyCnt(ucSuperPu, BlockSN);
            ulValidPageInPU = ((LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT - usDirtyCnt) / LPN_PER_SUPERBUF) + 1;
            ulValidDirty = (ulValidPageInPU / (PG_PER_WL * PG_PER_SLC_BLK - ulValidPageInPU)) + 2;

            ulHostWriteRealPage = min((ulHostWriteRealPage / ulValidDirty) + 1, ulHostWriteRealPage);
        }
    }

    /*step3: cal TLCAreaWrite quota*/
    for (ucTaskIndex = TASK_TLCGC_BIT; ucTaskIndex <= TASK_TLCSWL_BIT; ucTaskIndex++)
    {
        if (TRUE == L2_FTLIsTaskSet(ucSuperPu, ucTaskIndex))
        {
            if (0 < ulHostWriteRealPage)
            {
                pSuperPuQuota[FTL_THREAD_SLCGC + ucTaskIndex] = (ulTLCWriteRealPage / ulHostWriteRealPage);
                if (0 != (ulTLCWriteRealPage % ulHostWriteRealPage))
                {
                    pSuperPuQuota[FTL_THREAD_SLCGC + ucTaskIndex] += 1;
                }
                if (pSuperPuQuota[FTL_THREAD_SLCGC + ucTaskIndex] > (PG_PER_WL * PG_PER_SLC_BLK))
                {
                    pSuperPuQuota[FTL_THREAD_SLCGC + ucTaskIndex] = PG_PER_WL * PG_PER_SLC_BLK;
                }
            }
            else
            {
                pSuperPuQuota[FTL_THREAD_SLCGC + ucTaskIndex] = PG_PER_WL * PG_PER_SLC_BLK;
            }
            break;
        }
    }

    return;
}

/*
pass quota at below 4 case happen(Precondition:always TLCGC priority to TLCMerge; TLCMerge priority to TLCSWL):
case1:TLCGCQuota == TLCMERGEQuota
case2:TLCMERGEQuota == TLCSWLQuota
case3:TLCGCQuota == TLCSWLQuota
case4:TLCGCQuota == TLCMERGEQuota == TLCSWLQuota
other case:continue matian old quota until current process end
*/
void L2_FTLThreadInheritQuota(U8 ucSuperPu)
{
    U16 * pSuperPuQuota;

    pSuperPuQuota = l_FTLScheduler[ucSuperPu].aThreadQuota;

    /*one CornerCase: when Idle, and SLCGC is processing,
    L2 will disturb SLCGC by set it's Quota zero*/
    if ((0 == pSuperPuQuota[FTL_THREAD_SLCGC]) && (FALSE != L2_FTLIsTaskSet(ucSuperPu, TASK_SLCGC_BIT))
        && (FALSE == g_L2EventStatus.m_ForceIdle))
    {
        pSuperPuQuota[FTL_THREAD_SLCGC] = g_GCManager[SLCGC_MODE]->m_NeedCopyForOneWrite[ucSuperPu];
    }

    /*if SLGC done, Clear Quota*/
    if ((0 != pSuperPuQuota[FTL_THREAD_SLCGC]) && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_SLCGC_BIT)))
    {
        pSuperPuQuota[FTL_THREAD_SLCGC] = 0;
    }

    /*TLCArea only TLCGC, set it's quota to zero when process done*/
    if ((0 != pSuperPuQuota[FTL_THREAD_TLCGC])
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCGC_BIT))
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT))
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)))
    {
        pSuperPuQuota[FTL_THREAD_TLCGC] = 0;
        return;
    }

    /*TLCArea only TLCMerge, set it's quota to zero when process done*/
    if ((0 != pSuperPuQuota[FTL_THREAD_GC_2TLC])
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT))
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)))
    {
        pSuperPuQuota[FTL_THREAD_GC_2TLC] = 0;
        return;
    }

    /*TLCArea only TLCSWL, set it's quota to zero when process done*/
    if ((0 != pSuperPuQuota[FTL_THREAD_WL])
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)))
    {
        pSuperPuQuota[FTL_THREAD_WL] = 0;
        return;
    }

    /*TLCGCQuota == TLCMERGEQuota, when TLCGC Done pass quota to TLCMERGE*/
    if ((0 != pSuperPuQuota[FTL_THREAD_TLCGC])
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCGC_BIT))
        && (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT))
        && (0 == pSuperPuQuota[FTL_THREAD_GC_2TLC]))
    {
        pSuperPuQuota[FTL_THREAD_GC_2TLC] = pSuperPuQuota[FTL_THREAD_TLCGC];
        pSuperPuQuota[FTL_THREAD_TLCGC] = 0;

        return;
    }
    
    /*TLCMERGEQuota == TLCSWLQuota or TLCGCQuota == TLCSWLQuota,
    when TLCMERGEQuota or TLCGCQuota done pass quota to TLCSWL*/
    if ((FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCGC_BIT))
        && (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCMERGE_BIT))
        && (TRUE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT))
        && (0 == pSuperPuQuota[FTL_THREAD_WL]))
    {
#ifdef SIM
        if (((0 != pSuperPuQuota[FTL_THREAD_TLCGC]) && (0 != pSuperPuQuota[FTL_THREAD_GC_2TLC]))
            || ((0 == pSuperPuQuota[FTL_THREAD_TLCGC]) && (0 == pSuperPuQuota[FTL_THREAD_GC_2TLC])))
        {
            DBG_Getch();
        }
#endif
        if (0 != pSuperPuQuota[FTL_THREAD_TLCGC])
        {
            pSuperPuQuota[FTL_THREAD_WL] = pSuperPuQuota[FTL_THREAD_TLCGC];
            pSuperPuQuota[FTL_THREAD_TLCGC] = 0;
        }

        if (0 != pSuperPuQuota[FTL_THREAD_GC_2TLC])
        {
            pSuperPuQuota[FTL_THREAD_WL] = pSuperPuQuota[FTL_THREAD_GC_2TLC];
            pSuperPuQuota[FTL_THREAD_GC_2TLC] = 0;
        }
    }

    return;
}

/*****************************************************************************
 Prototype      : L2_FTLThreadQuotaAssign
 Description    : FTL thread quota assign
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/26
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
BOOL L2_FTLThreadQuotaAssign(U8 ucSuperPu)
{
    U16 * pSuperPuQuota;
    U32 ulCurFreePageForHost;
    U16 usVBN;
    U32 CurrDevWriteCntOfCE;
    U32 LastDevWriteCntOfCE;
    
    /*new a TLCWrite target*/
    if ((INVALID_4F == g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE])
        && (FALSE != L2_IsBootupOK()))
    {
#ifdef SIM
        GET_ALLOC_FREE_BLK_LOCK;        
#else
        HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif        
        usVBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, TRUE);
#ifdef SIM
        RELEASE_ALLOC_FREE_BLK_LOCK;
#else
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif        

        if (INVALID_4F != usVBN)
        {
            if (pVBT[ucSuperPu]->m_VBT[usVBN].Target == VBT_NOT_TARGET)
            {
                DBG_Printf("Alloc TLCWriteTarget SPU %d Blk %d Error\n", ucSuperPu, usVBN);
                DBG_Getch();
            }
            g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE] = usVBN;
            g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_TLC_WRITE] = 0;
            pVBT[ucSuperPu]->m_VBT[usVBN].StripID = ucSuperPu;
            pVBT[ucSuperPu]->m_VBT[usVBN].Target = VBT_TARGET_TLC_W;
            pVBT[ucSuperPu]->m_VBT[usVBN].VBTType = VBT_TYPE_TLCW;
            pVBT[ucSuperPu]->m_VBT[usVBN].bFree = FALSE;
            g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC]++;
        }
        else
        {
            /* no TLC free block, check whether rase TLC flag is enable. try to recovery. */
            if (g_EraseOpt[ucSuperPu].bEraseTLC == TRUE)
            {
                /* There has pending erased TLC block, retrun and wait erase done */
#ifdef DEBUG_NOTLCFREEBLK
                DBG_Printf("no free TLC blk: Current TLC blk erase not done\n");                
#endif
                return FALSE;
            }
            else
            {
#ifdef DEBUG_NOTLCFREEBLK
                DBG_Printf("no TLC free blk: TEQ %d, SEQ %d\n", L2_GetEraseQueueSize(ucSuperPu, FALSE), L2_GetEraseQueueSize(ucSuperPu, TRUE));
#endif

                if (L2_GetEraseQueueSize(ucSuperPu, FALSE) == 0)
                {
                    DBG_Printf("[%s]No TLC free blk and erase queue empty\n", __FUNCTION__);
                    DBG_Getch();  
                }

				//set quota and force fo to flush PMT
                CurrDevWriteCntOfCE = g_PMTFlushManager[ucSuperPu].m_DevWriteCntOfCE;
                LastDevWriteCntOfCE = g_PMTFlushManager[ucSuperPu].m_LastDevWriteCntOfCE;
#ifdef DEBUG_NOTLCFREEBLK
                DBG_Printf("before CurrDevWriteCntOfCE %d, LastDevWriteCntOfCE %d\n", CurrDevWriteCntOfCE, LastDevWriteCntOfCE);
#endif
                if ((CurrDevWriteCntOfCE - LastDevWriteCntOfCE) >= g_ulPMTFlushRatio)
                {
                    /* do nothing, not need to increase CurrDevWriteCntOfCE */
#ifdef DEBUG_NOTLCFREEBLK
                    DBG_Printf("achieve threshold\n");
                    DBG_Printf("[%s]CurrDevWriteCntOfCE - LastDevWriteCntOfCE = %d >= g_ulPMTFlushRatio %d\n", __FUNCTION__, (CurrDevWriteCntOfCE - LastDevWriteCntOfCE), g_ulPMTFlushRatio);
#endif
                }
                else
                {
#ifdef DEBUG_NOTLCFREEBLK
                    DBG_Printf("not achieve threshold, force set\n");
                    DBG_Printf("[%s]CurrDevWriteCntOfCE - LastDevWriteCntOfCE = %d >= g_ulPMTFlushRatio %d\n", __FUNCTION__, (CurrDevWriteCntOfCE - LastDevWriteCntOfCE), g_ulPMTFlushRatio);
#endif
                    g_PMTFlushManager[ucSuperPu].m_DevWriteCntOfCE += g_ulPMTFlushRatio - (CurrDevWriteCntOfCE - LastDevWriteCntOfCE);
				}

#ifdef DEBUG_NOTLCFREEBLK
                DBG_Printf("after CurrDevWriteCntOfCE %d, LastDevWriteCntOfCE %d\n", CurrDevWriteCntOfCE, LastDevWriteCntOfCE);
#endif

                return FALSE;
            }
        }

        //FIRMWARE_LogInfo("SuperPU %d allocated TLCBLK_VBN %d Current TLCFreeBlkCnt %d\n", ucSuperPu, usVBN, g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_TLC] - g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC]);
    }

    pSuperPuQuota = l_FTLScheduler[ucSuperPu].aThreadQuota;

    /* ask thread controller one by one to assign the quota */
    if ((FALSE == L2_IsBootupOK()) || (TRUE == g_L2EventStatus.m_ForceIdle))
    {
        // PMT hasn't been loaded completely or there's a force idle event
        pSuperPuQuota[FTL_THREAD_SLCGC] = 0;
        pSuperPuQuota[FTL_THREAD_TLCGC] = 0;
        pSuperPuQuota[FTL_THREAD_WL] = 0;
        pSuperPuQuota[FTL_THREAD_WRITE] = 1;
        pSuperPuQuota[FTL_THREAD_GC_2TLC] = 0;
    }
    else
    {
        if (SUCCESS == L2_CheckTaskEventStatus(TRUE))
        {            
            if (TRUE == L2_FTLIsAllTaskDone(ucSuperPu))
            {
                /*clear all quota*/
                L2_FTLThreadSetAllQuotaZero(ucSuperPu);

                /*step1: one process task confirm*/
                L2_FTLThreadProcessTaskConfirm(ucSuperPu);

                /*step2: different task quota calculate*/
                L2_FTLThreadCalTaskQuota(ucSuperPu);
            }
            else
            {
                /*inherit quota in current process*/
                L2_FTLThreadInheritQuota(ucSuperPu);
            }
        }
        else
        {
            /*clear all quota*/
            L2_FTLThreadSetAllQuotaZero(ucSuperPu);

            //FIRMWARE_LogInfo("L2_CheckTaskEventStatus(TRUE) SuperPU %d check fail\n", ucSuperPu);
        }

        /*HostWrite Quota assign*/
        if (TRUE == L2_FTLThreadIsAllQuotaZero(ucSuperPu) && (SUCCESS == L2_CheckTaskEventStatus(TRUE)))
        {
            ulCurFreePageForHost = L2_GetSLCAreaFreePageCnt(ucSuperPu) + (L2_GetEraseQueueSize(ucSuperPu, TRUE) * PG_PER_SLC_BLK)
                - (RESERVED_PAGE_CNT + BOOT_STRIPE_CNT);
            pSuperPuQuota[FTL_THREAD_WRITE] = min(PG_PER_SLC_BLK, ulCurFreePageForHost);

            g_ulPMTFlushRatio = FlushPMTThs;
            L2_SetThreadQuota(ucSuperPu, SYS_THREAD_FTL, g_ulPMTFlushRatio);
        }
        else
        {
            if (TRUE == L2_CanServiceHost(ucSuperPu))
            {
                U32 ulTLCQuotaSum = 0, ucTaskIndex;

                /* quota set to 1 for least write latency during GC or WL */
                pSuperPuQuota[FTL_THREAD_WRITE] = 1;

                for (ucTaskIndex = FTL_THREAD_TLCGC; ucTaskIndex < FTL_THREAD_WRITE; ucTaskIndex++)
                {
                    ulTLCQuotaSum += pSuperPuQuota[ucTaskIndex];
                }

                g_ulPMTFlushRatio = (FlushPMTThs/2) / (ulTLCQuotaSum + 1);
                if (g_ulPMTFlushRatio < 1)
                {
                    g_ulPMTFlushRatio = 1;
                }
                //FIRMWARE_LogInfo("g_ulPMTFlushRatio %d ulTLCQuotaSum %d SLCGC %d TLCMerge %d TLCGC %d WL %d \n", g_ulPMTFlushRatio, ulTLCQuotaSum, pSuperPuQuota[FTL_THREAD_SLCGC],
                //    pSuperPuQuota[FTL_THREAD_GC_2TLC], pSuperPuQuota[FTL_THREAD_TLCGC], pSuperPuQuota[FTL_THREAD_WL]);
            }
            else
            {
                if (TRUE == g_L2EventStatus.m_WaitIdle && FALSE == L1_LowPrioFifoEmpty(ucSuperPu))                    
                {
                    /* Idle wait LowPrioFIFO empty must assign FTL quota */
                    pSuperPuQuota[FTL_THREAD_WRITE] = 1;
                }
                else
                {
                    pSuperPuQuota[FTL_THREAD_WRITE] = 0;
                    DBG_Printf("FTLW Quota = 0\n");
                    TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, "FTLWrite Thread Quota == 0,SuperPu");
                }
                g_ulPMTFlushRatio = 1;
            }
            
            /* Shutdown improvement : avoid L2_FTLEntry() idle loop running and keep quotas for L2_TableEntry() */
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            if(g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt >= SUBSYSTEM_SUPERPU_NUM)
#else//SHUTDOWN_IMPROVEMENT_STAGE1
            if(g_L2EventStatus.m_ShutdownEraseTLCBlkDoneCnt >= SUBSYSTEM_SUPERPU_NUM)
#endif
               g_ulPMTFlushRatio = 1;
               
            L2_SetThreadQuota(ucSuperPu, SYS_THREAD_FTL, g_ulPMTFlushRatio);
        }
    }
    
    return TRUE;
}


/*****************************************************************************
 Prototype      : L2_FTLThreadDecision
 Description    : FTL thread decision
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   : Current thread type
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/26
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
FTL_THREAD_TYPE L2_FTLThreadDecision(U8 ucSuperPu)
{
    U16 usMaxQuota;
    FTL_THREAD_TYPE CurrThread, OriginThread;

    CurrThread = l_FTLScheduler[ucSuperPu].eCurrThread;
    OriginThread = CurrThread;

    while (TRUE)
    {
        usMaxQuota = l_FTLScheduler[ucSuperPu].aThreadQuota[CurrThread];
        if (l_FTLScheduler[ucSuperPu].usThreadQuotaUsed >= usMaxQuota)
        {
            l_FTLScheduler[ucSuperPu].usThreadQuotaUsed = 0;
            CurrThread++;
            if (CurrThread >= FTL_THREAD_TYPE_ALL)
            {
                CurrThread = FTL_THREAD_SLCGC;

                /* assign the quota again for all thread */
                if (L2_FTLThreadQuotaAssign(ucSuperPu) == FALSE)
                    return FTL_THREAD_NO_TLC_FREEBLK;
            }

            /* add braek deadloop when FTL_THREAD_WRITE Quota is zero */
            if (OriginThread == CurrThread && 0 == l_FTLScheduler[ucSuperPu].aThreadQuota[FTL_THREAD_WRITE])
            {
                return FTL_THREAD_TYPE_ALL;
            }

            l_FTLScheduler[ucSuperPu].eCurrThread = CurrThread;
        }
        else
        {
            return CurrThread;
        }
    }
}
/*****************************************************************************
 Prototype      : L2_FTLIncUsedThreadQuota
 Description    : increase FTL used thread quota
 Input          : U8 ucSuperPu
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2015/03/26
 Author       : zoewen
 Modification : Created function

 *****************************************************************************/
void L2_FTLIncUsedThreadQuota(U8 ucSuperPu, U16 usVal)
{
    l_FTLScheduler[ucSuperPu].usThreadQuotaUsed += usVal;
    return;
}

void L2_FTLUsedThreadQuotaMax(U8 ucSuperPu, FTL_THREAD_TYPE CurrThread)
{
    l_FTLScheduler[ucSuperPu].usThreadQuotaUsed = l_FTLScheduler[ucSuperPu].aThreadQuota[CurrThread];
    return;
}

U16 L2_FTLGetUsedThreadQuota(U8 ucSuperPu)
{
    return l_FTLScheduler[ucSuperPu].usThreadQuotaUsed;
}

U16 L2_FTLGetThreadQuotaMax(U8 ucSuperPu, FTL_THREAD_TYPE CurrThread)
{
    return l_FTLScheduler[ucSuperPu].aThreadQuota[CurrThread];;
}

void L2_FTLSetThreadQuota(U8 ucSuperPu, FTL_THREAD_TYPE CurrThread, U32 val)
{
    l_FTLScheduler[ucSuperPu].aThreadQuota[CurrThread] = val;
    return;
}


void L2_SetPassQuota(U8 ucSuperPu, BOOL bVa) //SWL
{
    l_FTLScheduler[ucSuperPu].bPassQuota = bVa;
}

BOOL L2_IsPassQuota(U8 ucSuperPu)
{
    return l_FTLScheduler[ucSuperPu].bPassQuota;
}

void MCU1_DRAM_TEXT L2_InitStatistic(void)
{
    U8 PUSer;

    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        g_PMTFlushManager[PUSer].m_DevWriteCntOfCE = 0;
        g_PMTFlushManager[PUSer].m_LastDevWriteCntOfCE = 0;

        g_PMTFlushManager[PUSer].m_CurPMTFlushPosOfCE = 0;

        //Idle flush
        g_IdleFlushPos[PUSer] = 0;
    }


#if defined(HOST_AHCI)
    L2_AhciInitRdWithoutWtDptr();
#elif defined (HOST_NVME)
    L2_NVMeInitRdWithoutWtDptr();
#endif

}

U32 L2_GetNextDirtyPMTIndex(U32 ucSuperPu, U32 StartPos)
{
    U32 PMTIndexInPu;
    U32 i;

    for (i = 0; i < PMTPAGE_CNT_PER_SUPERPU; i++)
    {
        PMTIndexInPu = (StartPos + i) % PMTPAGE_CNT_PER_SUPERPU;
        if (L2_IsPMTPageDirty(ucSuperPu, PMTIndexInPu))
        {
            return PMTIndexInPu;
        }
    }
    return INVALID_8F;
}

BOOL L2_FlushPMTPageForRebuild(U32 ulSuperPu, U32* pPMTIndexInPu)
{
    U32 CurFlushPos;
    U16 PMTIndexFlushArray[TABLE_RW_PAGE_CNT];
    BOOL bRet = TRUE;

    CurFlushPos = g_PMTFlushManager[ulSuperPu].m_CurPMTFlushPosOfCE;
    if (CurFlushPos >= PMTPAGE_CNT_PER_SUPERPU)
    {
        CurFlushPos = 0;
    }
  
    *pPMTIndexInPu = L2_GetNextDirtyPMTIndex(ulSuperPu, CurFlushPos);

    if (INVALID_8F == *pPMTIndexInPu)
    {
        return FALSE;
    }

    PMTIndexFlushArray[0] = *pPMTIndexInPu;
    bRet = L2_SetTableRW(ulSuperPu, PMTIndexFlushArray, 1, TABLE_WRITE);
    if(TRUE == bRet)
    {
        CurFlushPos = *pPMTIndexInPu + 1;
        g_PMTFlushManager[ulSuperPu].m_CurPMTFlushPosOfCE = CurFlushPos;     
    }

    return bRet;
}


void L2_IncDeviceWrite(U16 PUSer, U32 PageCnt)
{
    g_PMTFlushManager[PUSer].m_DevWriteCntOfCE += PageCnt;
}

BOOL L2_IsNeedFlushPMT(U16 PUSer)
{
    U32 CurrDevWriteCntOfCE;
    U32 LastDevWriteCntOfCE;

    CurrDevWriteCntOfCE = g_PMTFlushManager[PUSer].m_DevWriteCntOfCE;
    LastDevWriteCntOfCE = g_PMTFlushManager[PUSer].m_LastDevWriteCntOfCE;

    if ((CurrDevWriteCntOfCE - LastDevWriteCntOfCE) >= g_ulPMTFlushRatio)    
    {
        g_PMTFlushManager[PUSer].m_LastDevWriteCntOfCE = CurrDevWriteCntOfCE;
        return TRUE;
    }

    return FALSE;
}

BOOL L2_IsPassFTLWQuota(U8 ucSuperPu)
{
#if 1
    // check if we have to manually pass the write quota
    if ((FALSE == L2_IsBootupOK()) || (TRUE == g_L2EventStatus.m_Shutdown) || (TRUE == g_L2EventStatus.m_WaitIdle))// event
    {
        return TRUE;
    }
    else if ((TRUE == g_tFTLIdleGCMgr.bIdleGC) || (TRUE == L2_IsWL(ucSuperPu)) || (FALSE == L2_IsTLCFreeBlkEnough(ucSuperPu))) //system running
    {
        if (L2_IsAllEraseQueueEmpty(ucSuperPu) == FALSE)
        {
            L2_IncDeviceWrite(ucSuperPu, 1);
        }
        else
        {
            L2_SetPassQuota(ucSuperPu, TRUE);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    if ((FALSE == L2_IsBootupOK()) || (TRUE == g_L2EventStatus.m_Shutdown) || (TRUE == g_L2EventStatus.m_WaitIdle))// event
    {
        return TRUE;
    }
    else// if ((TRUE == g_tFTLIdleGCMgr.bIdleGC) || (TRUE == L2_IsWL(ucSuperPu)) || (FALSE == L2_IsTLCFreeBlkEnough(ucSuperPu))) //system running
    {
        if (L2_IsAllEraseQueueEmpty(ucSuperPu) == FALSE)
        {
            L2_IncDeviceWrite(ucSuperPu, 1);
        }
        else
        {
            L2_SetPassQuota(ucSuperPu, TRUE);
        }
        return TRUE;
    }
#endif
}


/*PatrolRead relative start*/
LOCAL MCU12_VAR_ATTR PATROL_READ_MANAGER * l_pPatrolReadMgr;
LOCAL MCU12_VAR_ATTR U32 l_ulWaitTimeInterval;

#ifdef SCAN_BLOCK_N1
LOCAL MCU12_VAR_ATTR SCAN_N1_MANAGER *l_pScanN1Mgr;
/*----------------------------------------------------------------------------
Name: L2_ScanBlockN1MgrDramAllocate
Description:
allocate dram space for block N1 scan
Input Param:
U32* pFreeDramBase:
Output Param:
none
Return Value:
void
Usage:
in bootup stage, call this function to allocate dram space for block N1 Scan
History:
20171228    DannierChen   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_ScanBlockN1MgrDramAllocate(U32* pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_pScanN1Mgr = (SCAN_N1_MANAGER *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(SCAN_N1_MANAGER));

    COM_MemAddr16DWAlign(&ulFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

    return;
}
#endif

/*----------------------------------------------------------------------------
Name: L2_PatrolReadMgrDramAllocate
Description:
allocate dram space for PatrolReadMgr
Input Param:
U32* pFreeDramBase:
Output Param:
none
Return Value:
void
Usage:
in bootup stage, call this function to allocate dram space for PatrolReadMgr
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_PatrolReadMgrDramAllocate(U32* pFreeDramBase)
{
    U8 ucSPU;
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    l_pPatrolReadMgr = (PATROL_READ_MANAGER *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(PATROL_READ_MANAGER));

    for (ucSPU = 0; ucSPU < SUBSYSTEM_SUPERPU_NUM; ucSPU++)
    {
        l_pPatrolReadMgr->pReadEntry[ucSPU] = (PATROL_READ_ENTRY *)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(PATROL_READ_ENTRY));
    }

    COM_MemAddr16DWAlign(&ulFreeDramBase);

    *pFreeDramBase = ulFreeDramBase;

    return;
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadMgrInit
Description:
Init PatrolReadMgr
Input Param:
none
Output Param:
none
Return Value:
void
Usage:
in bootup stage, call this function to Init PatrolReadMgr.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_PatrolReadMgrInit(void)
{
    U8 ucSuperPU;

    l_pPatrolReadMgr->ucReadStage = PATROL_READ_FIRST_AFTER_BOOT;
    l_pPatrolReadMgr->ulPatrolWaitStartTime = 0;
    l_pPatrolReadMgr->ucPatrolReadPageDoneBitMap = FALSE;
    l_pPatrolReadMgr->ucPatrolReadBlkDoneBitMap = FALSE;
    l_pPatrolReadMgr->ucPatrolReadCmdSendPUBitMap = FALSE;
    l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap = FALSE;
    l_pPatrolReadMgr->bGetFirstBlockAfterInterval = FALSE;
    l_pPatrolReadMgr->bContinueScanAfterBoot = FALSE;
    l_pPatrolReadMgr->ulPatrolReadCycleCnt = INVALID_8F;

    COM_MemZero((U32*)l_pPatrolReadMgr->pReadEntry[0], (sizeof(PATROL_READ_ENTRY)*(SUBSYSTEM_SUPERPU_NUM)) >> 2);

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucReadPStage = PATROL_READ_PROCESS_SENDCMD;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt = 0;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulReadLUNBitMap = 0;
    }

    return;
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadRecordWaitStartTime
Description:
record time of PatrolReadWait start.
Input Param:
none
Output Param:
none
Return Value:
void
Usage:
when one disk PatrolRead done, invoke it to record time of PatrolReadWait start.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_PatrolReadRecordWaitStartTime(void)
{
#ifndef SIM
    l_pPatrolReadMgr->ulPatrolWaitStartTime = g_pSubSystemHostInfoPage->PowerOnHours;
#else
    l_pPatrolReadMgr->ulPatrolWaitStartTime = g_pSubSystemHostInfoPage->PowerOnMins;
#endif
    return;
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadCalcWaitTime
Description:
calculate time pass from the PatorlReadWait start point. unit by hour
Input Param:
none
Output Param:
none
Return Value:
U32 : time pass by hour
Usage:
when PatrolRead time up check, invoke it to calculate time pass from the PatorlReadWait start point.
in windows, use mintes intead of hours.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
U32 MCU1_DRAM_TEXT L2_PatrolReadCalcWaitTime(void)
{
#ifndef SIM
    return (g_pSubSystemHostInfoPage->PowerOnHours - l_pPatrolReadMgr->ulPatrolWaitStartTime);
#else
    if (g_pSubSystemHostInfoPage->PowerOnMins < l_pPatrolReadMgr->ulPatrolWaitStartTime)
    {
        return ((g_pSubSystemHostInfoPage->PowerOnMins + 60) - l_pPatrolReadMgr->ulPatrolWaitStartTime);
    }
    else
    {
        return (g_pSubSystemHostInfoPage->PowerOnMins - l_pPatrolReadMgr->ulPatrolWaitStartTime);
    }
#endif
}
/*----------------------------------------------------------------------------
Name: L2_PatrolReadGetCurVBN
Description:
get one PatorlRead BLK for special SuperPU. if don't get one, record VBN& PBN with INVALID_4F
Input Param:
U8 ucSuperPU: SuperPU NUM
Output Param:
none
Return Value:
BOOL : Get one PatrolRead BLK success
Usage:
when PatrolRead wait time up,invoke it to get one PatorlRead BLK for special SuperPU.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_PatrolReadGetCurVBN(U8 ucSuperPU)
{
    U8 ucLunIndex;
    U16 uBlkVBN = INVALID_4F;
    U16 uVirBlk = 0;
    U16 uBlkPhy = 0;
    U32 MinEraseCnt = INVALID_8F;
    PBIT_ENTRY* pPBIT_Entry;
    U16 EraseCntSrc = INVALID_4F;
    U8 ucLunInSuperPu = 0;
    U32 MinTSNum = INVALID_8F;
    U32 CurTSNum = 0;
    U32 DirtyCnt;
    U32 usTotalLPNCnt;

    pPBIT_Entry = pPBIT[ucSuperPU]->m_PBIT_Entry[ucLunInSuperPu];
    for (uBlkPhy = g_ulDataBlockStart[ucSuperPU][ucLunInSuperPu]; uBlkPhy < BLK_PER_LUN + RSVD_BLK_PER_LUN; uBlkPhy++)
    {
        uVirBlk = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        if (uVirBlk == INVALID_4F)
            continue;

        if (VBT_NOT_TARGET != pVBT[ucSuperPU]->m_VBT[uVirBlk].Target)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPU]->m_VBT[uVirBlk].bsInEraseQueue)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPU]->m_VBT[uVirBlk].bWaitEraseSts)
        {
            continue;
        }

        if (VBT_TYPE_HOST != pVBT[ucSuperPU]->m_VBT[uVirBlk].VBTType && VBT_TYPE_TLCW != pVBT[ucSuperPU]->m_VBT[uVirBlk].VBTType)
        {
            continue;
        }

        if (TRUE == pPBIT_Entry[uBlkPhy].bError)
            continue;

        if (TRUE == pPBIT_Entry[uBlkPhy].bLock)
            continue;

        if (TRUE == pPBIT_Entry[uBlkPhy].bTable)
            continue;

        if (TRUE == pPBIT_Entry[uBlkPhy].bBackup)
            continue;

        if (TRUE == pPBIT_Entry[uBlkPhy].bReserved)
            continue;

        if (TRUE == pPBIT_Entry[uBlkPhy].bFree)
            continue;

        if (TRUE == pPBIT_Entry[uBlkPhy].bPatrolRead)
            continue;

        DirtyCnt = L2_GetDirtyCnt(ucSuperPU, uVirBlk);

        if (TRUE == L2_VBT_Get_TLC(ucSuperPU, uVirBlk))
        {
            usTotalLPNCnt = LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT;
        }
        else
        {
            usTotalLPNCnt = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;
        }

        if (usTotalLPNCnt == DirtyCnt)
            continue;

        /*select blok marked with bL3RECC in priority*/
        if (TRUE == pPBIT_Entry[uBlkPhy].bWeak)
        {
            uBlkVBN = uVirBlk;
            CurTSNum = 0; //fake set for later check
            break;
        }
        else
        {
            /* Get last page TS of block */
            CurTSNum = L2_Get_PBIT_PTR_Blk(ucSuperPU, ucLunInSuperPu, uBlkPhy);
            if ((CurTSNum < MinTSNum)
                && (CurTSNum < l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulLastMaxTS))
            {
                MinTSNum = CurTSNum;
                uBlkVBN = uVirBlk;
            }
        }
    }

    if (INVALID_8F == MinTSNum)
    {
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurVBN = INVALID_4F;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurPPO = 0;
        for (ucLunIndex = 0; ucLunIndex < LUN_NUM_PER_SUPERPU; ucLunIndex++)
        {
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->aCurPBN[ucLunIndex] = INVALID_4F;
        }
        return FALSE;
    }
    else
    {
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurVBN = uBlkVBN;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurPPO = 0;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usUECCCnt = 0;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usRECCCnt = 0;
        l_pPatrolReadMgr->pReadEntry[ucSuperPU]->bSLCBLK = L2_IsSLCBlock(ucSuperPU, uBlkVBN);

#ifdef SIM
        if (TRUE == l_pPatrolReadMgr->pReadEntry[ucSuperPU]->bSLCBLK)
        {
            //FIRMWARE_LogInfo("SuperPU#%d, VBN#0x%x slc block.\n", ucSuperPU, uBlkVBN);
        }
        else
        {
            //FIRMWARE_LogInfo("SuperPU#%d, VBN#0x%x mlc block.\n", ucSuperPU, uBlkVBN);
        }
#endif

        for (ucLunIndex = 0; ucLunIndex < LUN_NUM_PER_SUPERPU; ucLunIndex++)
        {
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->aCurPBN[ucLunIndex] = pVBT[ucSuperPU]->m_VBT[uBlkVBN].PhysicalBlockAddr[ucLunIndex];
        }

        //l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt++;
        return TRUE;
    }

}

BOOL MCU1_DRAM_TEXT L2_PatrolReadGetSRCSuperPU(U8 ucSuperPU)
{
    return COM_8BitMaskGet(l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap, ucSuperPU);
}
void MCU1_DRAM_TEXT L2_PatrolReadSetSRCSuperPU(U8 ucSuperPU)
{
    COM_8BitMaskSet(&l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap, ucSuperPU);
    return;
}

BOOL MCU1_DRAM_TEXT L2_PatrolReadGetPageSend(U8 ucSuperPU)
{
    return COM_8BitMaskGet(l_pPatrolReadMgr->ucPatrolReadCmdSendPUBitMap, ucSuperPU);
}
void MCU1_DRAM_TEXT L2_PatrolReadSetPageSend(U8 ucSuperPU)
{
    COM_8BitMaskSet(&l_pPatrolReadMgr->ucPatrolReadCmdSendPUBitMap, ucSuperPU);
    return;
}
/*----------------------------------------------------------------------------
Name: L2_PatrolReadGetPageDone
Description:
get one specific SuperPU PageDone status from ulPatrolReadPageDoneBitMap.
Input Param:
U8 ucSuperPU: SuperPU NUM
Output Param:
none
Return Value:
BOOL : Get one PatrolRead BLK success
Usage:
invoke it get one specific SuperPU PageDone status from ulPatrolReadPageDoneBitMap.
used in syc all SuperPu PatrolRead PageRead work.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_PatrolReadGetPageDone(U8 ucSuperPU)
{
    return COM_8BitMaskGet(l_pPatrolReadMgr->ucPatrolReadPageDoneBitMap, ucSuperPU);
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadSetPageDone
Description:
set one specific SuperPU PageDone status.
Input Param:
U8 ucSuperPU: SuperPU NUM
Output Param:
none
Return Value:
none
Usage:
invoke it set one specific SuperPU PageDone status.
used in syc all SuperPu PatrolRead PageRead work.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_PatrolReadSetPageDone(U8 ucSuperPU)
{
    COM_8BitMaskSet(&l_pPatrolReadMgr->ucPatrolReadPageDoneBitMap, ucSuperPU);
    return;
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadSetBLKDone
Description:
set one specific SuperPU BlkDone status.
Input Param:
U8 ucSuperPU: SuperPU NUM
Output Param:
none
Return Value:
none
Usage:
invoke it set one specific SuperPU BlkDone status.
used in syc all SuperPu PatrolRead BlkRead work.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_PatrolReadSetBLKDone(U8 ucSuperPU)
{
    COM_8BitMaskSet(&l_pPatrolReadMgr->ucPatrolReadBlkDoneBitMap, ucSuperPU);
    return;
}

BOOL MCU1_DRAM_TEXT L2_PatrolReadGetBLKDone(U8 ucSuperPU)
{
    return COM_8BitMaskGet(l_pPatrolReadMgr->ucPatrolReadBlkDoneBitMap, ucSuperPU);
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadSetCurLunBitMap
Description:
set one specific SuperPU's LUNInSuperPU BitMap.
Input Param:
U8 ucSuperPU: SuperPU NUM
Output Param:
none
Return Value:
none
Usage:
invoke it set one specific SuperPU's LUNInSuperPU BitMap.
used in syc one specific SuperPu's all Lun work.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_PatrolReadSetCurLunBitMap(U8 ucSuperPU, U8 ucLunInSuperPU)
{
    COM_BitMaskSet(&l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulReadLUNBitMap, ucLunInSuperPU);
    return;
}

/*----------------------------------------------------------------------------
Name: L2_PatrolReadGetTimeInteral
Description:
get time interval need wait by PatrolRead.
Input Param:
none
Output Param:
none
Return Value:
none
Usage:
invoke it get time interval need wait by PatrolRead..
different P/E cycle has different TimeInterval.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
U32 MCU1_DRAM_TEXT L2_PatrolReadGetTimeInteral(void)
{
#ifdef PATROL_READ_WAIT_TIME_FIXED
    return PATROL_READ_TIME_INTERVAL_THS;
#else
    U8 ucSuperPU;
    U32 ulAverageEraseCnt;
    U32 ulSumEraseCnt = 0;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        ulSumEraseCnt += pPBIT[ucSuperPU]->m_EraseCnt[BLKTYPE_TLC] / (BLK_PER_LUN * LUN_NUM_PER_SUPERPU);
    }

    ulAverageEraseCnt = ulSumEraseCnt / SUBSYSTEM_SUPERPU_NUM;

    if (ulAverageEraseCnt < PATROL_READ_PE_CYCLE_THS_0)
    {
        return PATROL_READ_TIME_INTERVAL_THS_0;
    }
    else if (ulAverageEraseCnt < PATROL_READ_PE_CYCLE_THS_1)
    {
        return PATROL_READ_TIME_INTERVAL_THS_1;
    }
    else if (ulAverageEraseCnt < PATROL_READ_PE_CYCLE_THS_2)
    {
        return PATROL_READ_TIME_INTERVAL_THS_2;
    }
    else if (ulAverageEraseCnt < PATROL_READ_PE_CYCLE_THS_3)
    {
        return PATROL_READ_TIME_INTERVAL_THS_3;
    }
    else if (ulAverageEraseCnt < PATROL_READ_PE_CYCLE_THS_4)
    {
        return PATROL_READ_TIME_INTERVAL_THS_4;
    }
    else
    {
        return PATROL_READ_TIME_INTERVAL_THS_5;
    }

#endif
}

U32 MCU1_DRAM_TEXT L2_PatrolReadGetAverageEraseCnt(void)
{
#ifdef PATROL_READ_WAIT_TIME_FIXED
    return PATROL_READ_TIME_INTERVAL_THS;
#else
    U8 ucSuperPU;
    U32 ulAverageEraseCnt;
    U32 ulSumEraseCnt = 0;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        ulSumEraseCnt += pPBIT[ucSuperPU]->m_EraseCnt[BLKTYPE_TLC] / (BLK_PER_LUN * LUN_NUM_PER_SUPERPU);
    }

    ulAverageEraseCnt = ulSumEraseCnt / SUBSYSTEM_SUPERPU_NUM;

    return ulAverageEraseCnt;
#endif
}

/*----------------------------------------------------------------------------
Name: L2_IsCurBlkPBNChange
Description:
check if CurBlk PBN change.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:changed; FALSE:not changed;
Usage:
invoke it check if CurBlk PBN change.
this case may be caused by WL.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_IsCurBlkPBNChange(U8 ucSuperPU)
{
    U8 ucLunIndex;
    U16 usOldPBN;
    U16 usCurPBN;
    U16 usVBN;

    usVBN = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurVBN;
    for (ucLunIndex = 0; ucLunIndex < LUN_NUM_PER_SUPERPU; ucLunIndex++)
    {
        usCurPBN = pVBT[ucSuperPU]->m_VBT[usVBN].PhysicalBlockAddr[ucLunIndex];
        usOldPBN = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->aCurPBN[ucLunIndex];
        if (usCurPBN != usOldPBN)
        {
            //FIRMWARE_LogInfo("MCU#%d SuperPU#%d CurVBN0x%x PBN change happen!!!\n", HAL_GetMcuId(), ucSuperPU, usVBN);
            return TRUE;
        }
    }

    return FALSE;
}

/*----------------------------------------------------------------------------
Name: L2_IsCurBlkTypeTarget
Description:
check if CurBlk BLK changed to be a target BLK.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:changed; FALSE:not change;
Usage:
invoke it to check if CurBlk BLK changed to be a target BLK.
this case may be caused by GC + be allocated again.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_IsCurBlkTypeTarget(U8 ucSuperPU)
{
    U16 usCurVBN;
    usCurVBN = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurVBN;

    if (VBT_NOT_TARGET != pVBT[ucSuperPU]->m_VBT[usCurVBN].Target)
    {
        //FIRMWARE_LogInfo("MCU#%d ucSuperPU:%d CurVBN:0x%x changed to open happen!!!\n", HAL_GetMcuId(), ucSuperPU, usCurVBN);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: L2_IsCurBlkInEraseQueOrWaitEraseSts
Description:
check if CurBlk BLK is in erase queue, or wait erase status
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:In erase queue or in wait erase status
Usage:
invoke it to check if if CurBlk BLK is in erase queue, or wait erase status.
in this case current block patrol read should stop.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_IsCurBlkInEraseQueOrWaitEraseSts(U8 ucSuperPU)
{
    PATROL_READ_ENTRY * pReadEntry;

    pReadEntry = l_pPatrolReadMgr->pReadEntry[ucSuperPU];

    if ((pVBT[ucSuperPU]->m_VBT[pReadEntry->usCurVBN].bsInEraseQueue)
        || (pVBT[ucSuperPU]->m_VBT[pReadEntry->usCurVBN].bWaitEraseSts))
    {
        //FIRMWARE_LogInfo("MCU#%d ucSuperPU:%d CurVBN:0x%x BLK mode changed!\n", HAL_GetMcuId(), ucSuperPU, pReadEntry->usCurVBN);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: L2_IsCurBlkLastPage
Description:
check if CurBlk PPO scan has meet the last page.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:meet last page; FALSE:not meet last page;
Usage:
invoke it to check if CurBlk PPO scan has meet the last page.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_IsCurBlkLastPage(U8 ucSuperPU)
{
    BOOL bRet;
    BOOL bSLCMode;
    U32 ulPageNumMax;

    bSLCMode = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->bSLCBLK;

    ulPageNumMax = (TRUE == bSLCMode) ? PG_PER_SLC_BLK : PG_PER_TLC_BLK;

    if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurPPO < (ulPageNumMax - 1))
    {
        bRet = FALSE;
    }
    else if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurPPO == (ulPageNumMax - 1))
    {
        bRet = TRUE;
    }
    else if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurPPO > (ulPageNumMax - 1))
    {
        DBG_Printf("ucSuperPU%d PatrolRead CurPPO error\n", ucSuperPU);
        DBG_Getch();
    }

    return bRet;
}

/*----------------------------------------------------------------------------
Name: L2_SetPBITFlagPatrolRead
Description:
set flag PatrolRead in PBIT, which means PBN has scaned by PatrolRead or not.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
none
Usage:
invoke it to set flag PatrolRead in PBIT. one VBN's all PBN should be set.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_SetPBITFlagPatrolRead(U8 ucSuperPU)
{
    U8 ucLunInSuperPU;
    U16 usBlock;

    for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
    {
        usBlock = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->aCurPBN[ucLunInSuperPU];
        L2_PBIT_Set_PatrolRead(ucSuperPU, ucLunInSuperPU, usBlock, TRUE);
    }
    return;
}

/*----------------------------------------------------------------------------
Name: L2_ClearPBITFlagPatrolRead
Description:
clear all PBN PatrolRead flag in PBIT for to start one brand new start PatrolRead cycle.
Input Param:
none
Output Param:
none
Return Value:
none
Usage:
invoke it to clear all PBN PatrolRead flag in PBIT for to start one brand new start PatrolRead cycle.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_ClearPBITFlagPatrolRead(void)
{
    U16 usBlkPhy;
    U8  ucLunInSuperPU;
    U8 ucSuperPU;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
        {
            for (usBlkPhy = g_ulDataBlockStart[ucSuperPU][ucLunInSuperPU]; usBlkPhy < BLK_PER_LUN + RSVD_BLK_PER_LUN; usBlkPhy++)
            {
                L2_PBIT_Set_PatrolRead(ucSuperPU, ucLunInSuperPU, usBlkPhy, FALSE);
            }
        }
    }

    return;
}

/*----------------------------------------------------------------------------
Name: L2_ClearPBITFlagWeak
Description:
set flag Weak in PBIT, which means BLK is health or not.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
none
Usage:
invoke it to set flag Weak in PBIT.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
void MCU1_DRAM_TEXT L2_ClearPBITFlagWeak(U8 ucSuperPU)
{
    U16 usVBN = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurVBN;
    L2_PBIT_Set_Weak(ucSuperPU, usVBN, FALSE);

    return;
}

/*----------------------------------------------------------------------------
Name: L2_PatorlReadSuccessCheck
Description:
check whether one SuperPU's all LUN cmd success or not.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:all success; FALSE:at least one LUN cmd isn't success;
Usage:
invoke it to check whether one SuperPU's all LUN cmd success or not.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_PatorlReadSuccessCheck(U8 ucSuperPU)
{
    U8 ucLunInSuperPU;
    BOOL bRet = TRUE;
#ifdef PATROL_PASS_FBC_TO_L2
    U8 FBC_THS;

    if (l_pPatrolReadMgr->ulPatrolReadCycleCnt == 0)
    {
        if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->bSLCBLK == TRUE)
        {
            FBC_THS = PATROL_READ_SLC_FIRST_BOOT_FBC_THS;
        }
        else
        {
            FBC_THS = PATROL_READ_TLC_FIRST_BOOT_FBC_THS;
        }
    }
    else
    {
        if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->bSLCBLK == TRUE)
        {
            FBC_THS = PATROL_READ_SLC_NORMAL_FBC_THS;
        }
        else
        {
            FBC_THS = PATROL_READ_TLC_NORMAL_FBC_THS;
        }
    }
#endif

    for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
    {
#ifdef DEBUG_PATROL_READ_SIM_TEST
        if (TRUE == L2_BlkQueueIsEmpty(g_pForceGCSrcBlkQueue[ucSuperPU], LINK_TYPE_HOST) && TRUE == L2_BlkQueueIsEmpty(g_pForceGCSrcBlkQueue[ucSuperPU], LINK_TYPE_TLCW)
            && (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurVBN % 3) == 0)
        if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU] == 0)
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU] = 
            (((HAL_GetMCUCycleCount() + (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usCurPPO % 100)) % 50) ? 0 : 1);
#endif

        if (SUBSYSTEM_STATUS_SUCCESS != l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU]
            && SUBSYSTEM_STATUS_RETRY_SUCCESS != l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU]
            /*&& SUBSYSTEM_STATUS_RECC != l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU]*/)
        {
            bRet = FALSE;
            break;
        }
#ifdef PATROL_PASS_FBC_TO_L2
        else if (l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucMaxCWFBC[ucLunInSuperPU] > FBC_THS)
        {
            bRet = FALSE;
            break;
        }
#endif
    }

    if (bRet == FALSE && l_pPatrolReadMgr->ulPatrolReadCycleCnt == 0)
    {
        l_pPatrolReadMgr->bContinueScanAfterBoot = TRUE;
    }

    return bRet;
}

/*----------------------------------------------------------------------------
Name: L2_PatorlReadEmptyCheck
Description:
check whether one SuperPU's Lun CMD is send to an empty page.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:empty page happen; FALSE:empty page not happen;
Usage:
invoke it to check whether one SuperPU's Lun CMD is send to an empty page.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_PatorlReadEmptyCheck(U8 ucSuperPU)
{
    U8 ucLunInSuperPU;
    BOOL bRet = FALSE;

    for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
    {
        if (SUBSYSTEM_STATUS_EMPTY_PG == l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU])
        {
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}

/*----------------------------------------------------------------------------
Name: L2_PatorlReadFailCheck
Description:
check whether one SuperPU's Lun CMD is fail.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:fail happen; FALSE:fail not happen;
Usage:
invoke it to check whether one SuperPU's Lun CMD is send fail.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_PatorlReadFailCheck(U8 ucSuperPU)
{
    U8 ucLunInSuperPU;
    BOOL bRet = FALSE;

    for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
    {
        if (SUBSYSTEM_STATUS_FAIL == l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU])
        {
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}

/*----------------------------------------------------------------------------
Name: L2_PatorlReadUECCRECCCheck
Description:
check whether one SuperPU's Lun CMD hold RECC or retry success.
Input Param:
U8 ucSuperPU:SuperPU num
Output Param:
none
Return Value:
BOOL:TRUE:REDD or retry success happen; FALSE:not happen;
Usage:
invoke it to check whether one SuperPU's Lun CMD hold RECC or retry success.
History:
20150812    Tobey   create
------------------------------------------------------------------------------*/
BOOL MCU1_DRAM_TEXT L2_PatorlReadUECCRECCCheck(U8 ucSuperPU)
{
    U8 ucLunInSuperPU;
    BOOL bRet = FALSE;

    for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
    {
        if (SUBSYSTEM_STATUS_RETRY_SUCCESS == l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU])
        {
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usUECCCnt++;
            bRet = TRUE;
        }
        else if (SUBSYSTEM_STATUS_RECC == l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucFlashSts[ucLunInSuperPU])
        {
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usRECCCnt++;
            bRet = TRUE;
        }
    }

    return bRet;
}

BOOL MCU1_DRAM_TEXT L2_PatrolReadSendCmd(void)
{
    U8 ucSuperPU;
    U8 ucTLun;
    U8 ucLunInSuperPU;
    PhysicalAddr tAddr;
    U32 pBuffer[2] = { 0 };
    U8 * pStatus;
#ifdef PATROL_PASS_FBC_TO_L2
    U8 * pFBC;
#endif
    BOOL bSLCMode;
    PATROL_READ_ENTRY *pReadEntry;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        if (L2_PatrolReadGetSRCSuperPU(ucSuperPU) == FALSE)
        {
            continue;
        }
        pReadEntry = l_pPatrolReadMgr->pReadEntry[ucSuperPU];

        if (L2_PatrolReadGetPageSend(ucSuperPU) == TRUE)
        {
            continue;
        }

        if (INVALID_4F == pReadEntry->usCurVBN)
        {
            L2_PatrolReadSetPageDone(ucSuperPU);
            L2_PatrolReadSetPageSend(ucSuperPU);
            continue;
        }

        if (TRUE == L2_PatrolReadGetPageDone(ucSuperPU))
        {
            L2_PatrolReadSetPageSend(ucSuperPU);
            continue;
        }

        for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
        {
            /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
            ucTLun = L2_GET_TLUN(ucSuperPU, ucLunInSuperPU);

            /*check cmd can send or not for current ucPu*/
            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                continue;
            }

            if (TRUE == COM_BitMaskGet(pReadEntry->ulReadLUNBitMap, ucLunInSuperPU))
            {
                continue;
            }

            /* Check PBN Change or CurBlk type change or Blk Type change*/
            if ((TRUE == L2_IsCurBlkPBNChange(ucSuperPU)) || (TRUE == L2_IsCurBlkTypeTarget(ucSuperPU)) || (TRUE == L2_IsCurBlkInEraseQueOrWaitEraseSts(ucSuperPU))
                || L2_PBIT_Get_Lock(ucSuperPU, pReadEntry->usCurVBN))
            {
#ifdef SIM
                DBG_Printf("Spu%d VBN=%d page=%d PBN Change=%d CurTargetTypt=%d IsErasing=%d IsLocked=%d\n",
                ucSuperPU, pReadEntry->usCurVBN, pReadEntry->usCurPPO, L2_IsCurBlkPBNChange(ucSuperPU),
                L2_IsCurBlkTypeTarget(ucSuperPU), L2_IsCurBlkInEraseQueOrWaitEraseSts(ucSuperPU),
                L2_PBIT_Get_Lock(ucSuperPU, pReadEntry->usCurVBN));
#endif
                pReadEntry->usCurVBN = INVALID_4F;
                L2_PatrolReadSetPageSend(ucSuperPU);
                L2_PatrolReadSetPageDone(ucSuperPU);

                break;
            }

            /*FCMD prpare prepare*/
            tAddr.m_PPN = 0;
            tAddr.m_PUSer = ucSuperPU;
            tAddr.m_LPNInPage = 0;
            tAddr.m_BlockInPU = pReadEntry->usCurVBN;
            tAddr.m_PageInBlock = pReadEntry->usCurPPO;
            tAddr.m_OffsetInSuperPage = ucLunInSuperPU;

            //pBuffer = (U32 *)g_L2TempBufferAddr;
            pStatus = &pReadEntry->ucFlashSts[ucLunInSuperPU];
            bSLCMode = pReadEntry->bSLCBLK;
#ifdef PATROL_PASS_FBC_TO_L2
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucMaxCWFBC[ucLunInSuperPU] = 0;
            pFBC = &l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucMaxCWFBC[ucLunInSuperPU];
#endif

            /*send FCMD*/
#ifdef PATROL_PASS_FBC_TO_L2
            __L2_FtlReadLocal(pBuffer, &tAddr, pStatus, NULL, LPN_PER_BUF, 0, FALSE, bSLCMode, pFBC);
#else
            __L2_FtlReadLocal(pBuffer, &tAddr, pStatus, NULL, LPN_PER_BUF, 0, FALSE, bSLCMode, TRUE);
#endif

            L2_PatrolReadSetCurLunBitMap(ucSuperPU, ucLunInSuperPU);

            /*Check one SuperPU all LUN done or not*/
            if (SUPERPU_LUN_NUM_BITMSK == pReadEntry->ulReadLUNBitMap)
            {
                L2_PatrolReadSetPageSend(ucSuperPU);
                pReadEntry->ulReadLUNBitMap = 0;
            }
        }
    }

    if (l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap == l_pPatrolReadMgr->ucPatrolReadCmdSendPUBitMap)
    {
        l_pPatrolReadMgr->ucPatrolReadCmdSendPUBitMap = 0;

        for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
        {
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulReadLUNBitMap = 0;
        }

        return TRUE;
    }

    return FALSE;
}

BOOL MCU1_DRAM_TEXT L2_PatrolReadWaitCmd(void)
{
    U8 ucSuperPU;
    U8 ucLunInSuperPU;
    PATROL_READ_ENTRY *pReadEntry;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        if (L2_PatrolReadGetSRCSuperPU(ucSuperPU) == FALSE || L2_PatrolReadGetPageDone(ucSuperPU) == TRUE
            || L2_PatrolReadGetBLKDone(ucSuperPU) == TRUE)
        {
            continue;
        }
        pReadEntry = l_pPatrolReadMgr->pReadEntry[ucSuperPU];

        for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
        {
            if (SUBSYSTEM_STATUS_PENDING != pReadEntry->ucFlashSts[ucLunInSuperPU])
            {
                L2_PatrolReadSetCurLunBitMap(ucSuperPU, ucLunInSuperPU);
#ifdef SIM
#ifdef PATROL_PASS_FBC_TO_L2
                if (l_pPatrolReadMgr->ulPatrolReadCycleCnt != 0)
                    l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ucMaxCWFBC[ucLunInSuperPU] = ((HAL_GetMCUCycleCount() + (pReadEntry->usCurPPO % 43)) % 42);
#endif
#endif
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == pReadEntry->ulReadLUNBitMap)
        {
            L2_PatrolReadSetPageDone(ucSuperPU);
            pReadEntry->ulReadLUNBitMap = 0;
        }
    }

    if (l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap == l_pPatrolReadMgr->ucPatrolReadPageDoneBitMap)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL MCU1_DRAM_TEXT L2_PatrolReadCheckStatus()
{
    U8 ucSuperPU;
    PATROL_READ_ENTRY *pReadEntry;
    BOOL ret;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        if (L2_PatrolReadGetSRCSuperPU(ucSuperPU) == FALSE || L2_PatrolReadGetBLKDone(ucSuperPU) == TRUE)
        {
            continue;
        }
#ifdef SIM
        if (L2_PatrolReadGetPageDone(ucSuperPU) == FALSE)
        {
            DBG_Getch();
        }
#endif
        pReadEntry = l_pPatrolReadMgr->pReadEntry[ucSuperPU];

        if (pReadEntry->usCurVBN == INVALID_4F)
        {
            L2_PatrolReadSetBLKDone(ucSuperPU);
#ifdef SIM
            DBG_Printf("Spu%d scan VBN=%d change at page=%d\n", ucSuperPU, pReadEntry->usCurVBN, pReadEntry->usCurPPO);
#endif
        }
        else
        {
            /*Success Check*/
            if (TRUE == L2_PatorlReadSuccessCheck(ucSuperPU))
            {
                /* to be defined */
            }
            /*EmptyPage Check*/
            else if (TRUE == L2_PatorlReadEmptyCheck(ucSuperPU))
            {
                DBG_Printf("PatrolRead empty page error SuperPU#%d VBN:0x%x!!\n", ucSuperPU, pReadEntry->usCurVBN);
                L2_PatrolReadSetBLKDone(ucSuperPU);
                //DBG_Getch();
            }
            /*Fail Check*/
            else //Best retry over THS //if (TRUE == L2_PatorlReadFailCheck(ucSuperPU))
            {
                ret = L2_BlkQueuePushBlock(ucSuperPU, g_pForceGCSrcBlkQueue[ucSuperPU], pVBT[ucSuperPU]->m_VBT[pReadEntry->usCurVBN].VBTType, pReadEntry->usCurVBN);
                /*add CurBlk to GC que*/
                DBG_Printf("L2_PatrolRead Push SPU%d CurVBN 0x%x to FGEQ=%d\n", ucSuperPU, pReadEntry->usCurVBN, ret);
                if (ret == FALSE && TRUE == L2_BlkQueueIsFull(g_pForceGCSrcBlkQueue[ucSuperPU]))
                {
                    DBG_Printf("PatrolRead FGEQ is full\n");
                }
                else
                {
                    L2_PatrolReadSetBLKDone(ucSuperPU);
                }
            }
#if 0
            /*UECC(RetrySuccess)&RECC Check*/
            else if (TRUE == L2_PatorlReadUECCRECCCheck(ucSuperPU))
            {
                U16 usUECCRECCCnt = pReadEntry->usUECCCnt + pReadEntry->usRECCCnt;
                DBG_Printf("ucSuperPU#%d, usUECCCnt:%d, usRECCCnt:%d\n", ucSuperPU, pReadEntry->usUECCCnt, pReadEntry->usRECCCnt);
                if (l_pPatrolReadMgr->ulPatrolReadCycleCnt == 0 && usUECCRECCCnt >= PATROL_READ_UECC_RECC_THS)
                {
                    /*add CurBlk to GC que*/
                    DBG_Printf("L2_PatrolRead Push Pu %d CurVBN 0x%x to forceGCSrcBlkQueue for RUECC.\n", ucSuperPU, pReadEntry->usCurVBN);

                    //L2_PBIT_Set_Lock(ucSuperPU, pReadEntry->usCurVBN, TRUE);
                    //L2_BlkQueuePushBlock(ucSuperPU, g_pForceGCSrcBlkQueue[ucSuperPU], pReadEntry->bSLCBLK, pReadEntry->usCurVBN);

                    L2_PatrolReadSetBLKDone(ucSuperPU);
                }
            }
#endif
        }

        if (L2_PatrolReadGetBLKDone(ucSuperPU) == FALSE)
        {
            if (TRUE == L2_IsCurBlkLastPage(ucSuperPU))
            {
                L2_PatrolReadSetBLKDone(ucSuperPU);
                //L2_SetPBITFlagPatrolRead(ucSuperPU);//need remove, move to after getting source block
                l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt++;
#ifdef SIM
                DBG_Printf("Spu%d scan blk=%d blkCnt=%d to last page=%d\n", ucSuperPU, pReadEntry->usCurVBN, l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt, pReadEntry->usCurPPO);
#endif
                pReadEntry->usCurVBN = INVALID_4F;
            }
            else
            {
                pReadEntry->usCurPPO++;
            }
        }
        else /* Fail case set block done*/
        {
            //L2_SetPBITFlagPatrolRead(ucSuperPU);//need remove!
            l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt++;
#ifdef SIM
            DBG_Printf("Spu%d scan blk=%d blkCnt=%d break at page=%d\n", ucSuperPU, pReadEntry->usCurVBN, l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt, pReadEntry->usCurPPO);
#endif
            pReadEntry->usCurVBN = INVALID_4F;
        }
    }

    if (l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap == l_pPatrolReadMgr->ucPatrolReadBlkDoneBitMap)
    {
        return TRUE;
    }

    return FALSE;
}

U8 MCU1_DRAM_TEXT L2_PatrolRead(void)
{
    U8 ucSuperPU;
    U32 ulPatrolReadWaitTime;
    U8 ucRetSts = PATROL_READ_WAIT;
    BOOL bGetFirstBlockAfterInterval = FALSE;

    switch (l_pPatrolReadMgr->ucReadStage)
    {
    case PATROL_READ_FIRST_AFTER_BOOT:
    {
#if 0
        if (L2_PatrolReadGetAverageEraseCnt() < PATROL_READ_PE_CYCLE_THS_0)
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_TIME_INTERVAL;
            /* directly jump to next stage without break */
        }
        else
#endif
        {
            l_pPatrolReadMgr->bGetFirstBlockAfterInterval = TRUE;
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_GET_BLOCK;
            break;
        }
    }
    case PATROL_READ_TIME_INTERVAL:
    {
        ulPatrolReadWaitTime = L2_PatrolReadCalcWaitTime();
        l_ulWaitTimeInterval = L2_PatrolReadGetTimeInteral();
        if (ulPatrolReadWaitTime < l_ulWaitTimeInterval)
        {
            /* still need wait */
            ucRetSts = PATROL_READ_WAIT_TIME_INTERVAL;
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_TIME_INTERVAL;
            break;
        }
        else
        {
            l_pPatrolReadMgr->bGetFirstBlockAfterInterval = TRUE;
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_GET_BLOCK;
            /* directly jump to next stage without break */
        }
    }
    case PATROL_READ_GET_BLOCK:
    {
        bGetFirstBlockAfterInterval = l_pPatrolReadMgr->bGetFirstBlockAfterInterval;
        if (bGetFirstBlockAfterInterval == TRUE)
        {
            /* clear all blk's bPatrolRead flag */
            L2_ClearPBITFlagPatrolRead();
        }

        for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
        {
            /* Update max TS */
            if (l_pPatrolReadMgr->ulPatrolReadCycleCnt == INVALID_8F)
            {
                l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulCurMaxTS = L2_GetTimeStampInPu(ucSuperPU); //L2_Get_PBIT_WL_CE(ucSuperPU);
                l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulLastMaxTS = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulCurMaxTS;
            }
            else
            {
                l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulLastMaxTS = l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulCurMaxTS;
                l_pPatrolReadMgr->pReadEntry[ucSuperPU]->ulCurMaxTS = L2_GetTimeStampInPu(ucSuperPU);
            }

            if (l_pPatrolReadMgr->bGetFirstBlockAfterInterval == TRUE)
            {
                l_pPatrolReadMgr->pReadEntry[ucSuperPU]->usScanBlkCnt = 0;
            }

            /* obtain BLK for PatrolRead */
            if (TRUE == L2_PatrolReadGetCurVBN(ucSuperPU))
            {
                L2_PatrolReadSetSRCSuperPU(ucSuperPU);
                L2_SetPBITFlagPatrolRead(ucSuperPU);//why not set here? originally set at read last page.
            }
        }

        /* recored patrol read start start time */
        if (bGetFirstBlockAfterInterval == TRUE)
        {
            L2_PatrolReadRecordWaitStartTime();
            l_pPatrolReadMgr->bGetFirstBlockAfterInterval = FALSE;
        }

        if (l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap == 0)
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_DISK_DONE;
            break;
        }
        else
        {
            if (bGetFirstBlockAfterInterval == TRUE)
            {
                l_pPatrolReadMgr->ulPatrolReadCycleCnt++;
            }
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_PAGE_READ;
            /* directly jump to next stage without break */
        }
    }
    case PATROL_READ_PAGE_READ:
    {
        /* wait all selected super PUs cmd send => set page done bit map */
        if (L2_PatrolReadSendCmd() == TRUE)
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_WAIT_PAGE;
        }

        /* always break */
        break;
    }
    case PATROL_READ_WAIT_PAGE:
    {
        if (l_pPatrolReadMgr->ucPatrolReadCmdSendPUBitMap != 0)
        {
            DBG_Getch();
        }
        /* wait all selected super PUs cmd finish => set page done bit map */
        if (L2_PatrolReadWaitCmd() == TRUE)
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_CHECK_STATUS;
            /* directly jump to next stage without break */
        }
        else
        {
            break;
        }
    }
    case PATROL_READ_CHECK_STATUS:
    {
        /* if page to the end of block => set block done bit map */
        if (L2_PatrolReadCheckStatus() == TRUE)
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_BLOCK_DONE;
            /* directly jump to next stage without break */
        }
        else
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_PAGE_READ;
            l_pPatrolReadMgr->ucPatrolReadPageDoneBitMap = 0;
            ucRetSts = PATROL_READ_ONE_PAGE_DONE;
            break;
        }
    }
    case PATROL_READ_BLOCK_DONE:
    {
        l_pPatrolReadMgr->ucPatrolReadPageDoneBitMap = 0;
        l_pPatrolReadMgr->ucPatrolReadBlkDoneBitMap = 0;
        l_pPatrolReadMgr->ucPatrolReadScanSuperPUBitMap = 0;
        if (l_pPatrolReadMgr->ulPatrolReadCycleCnt == 0 && l_pPatrolReadMgr->bContinueScanAfterBoot == FALSE)
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_DISK_DONE;
        }
        else
        {
            l_pPatrolReadMgr->ucReadStage = PATROL_READ_GET_BLOCK;
            ucRetSts = PATROL_READ_ONE_BLK_DONE;
            break;
        }
    }
    case PATROL_READ_DISK_DONE:
    {
        l_pPatrolReadMgr->ucReadStage = PATROL_READ_TIME_INTERVAL;
        ucRetSts = PATROL_READ_ONE_SCAN_DONE;
#ifdef SIM
        DBG_Printf("PatrolRead scan disk finish count=%d\n", l_pPatrolReadMgr->ulPatrolReadCycleCnt);
#endif
        break;
    }

    default:
        DBG_Getch();
    }

    return ucRetSts;
}

/*PatrolRead relative end*/



BOOL MCU1_DRAM_TEXT L2_IdleFlushPMTPage(U32 ulFlushCNT)
{

    U32 ulSuperPu;
    U16 i;
    U16 PMTIIndexInPu;
    U16 FlushPMTIIndex[SUBSYSTEM_SUPERPU_MAX][TABLE_RW_PAGE_CNT] = { INVALID_4F };
    U32 ulActualFlushCNT[SUBSYSTEM_SUPERPU_MAX] = { 0 };     // Check if the stack is overflow(>2KB).
    U32 ulAllFlushCNT = 0;

    return TRUE;//invalidate idle flush for rebuild table flush, add by henryluo

    for (ulSuperPu = 0; ulSuperPu < SUBSYSTEM_SUPERPU_NUM; ulSuperPu++)
    {
        if ((ulFlushCNT > TABLE_RW_PAGE_CNT) || (g_IdleFlushPos[ulSuperPu] > PMTPAGE_CNT_PER_SUPERPU))
        {
            DBG_Getch();
        }

        /* get dirty PMT page */
        for (i = 0; i < ulFlushCNT; i++)
        {
            for (PMTIIndexInPu = g_IdleFlushPos[ulSuperPu]; PMTIIndexInPu < PMTPAGE_CNT_PER_SUPERPU; PMTIIndexInPu++)
            {
                if (TRUE != L2_IsPMTPageDirty(ulSuperPu, PMTIIndexInPu))
                {
                    continue;
                }
                else
                {
                    FlushPMTIIndex[ulSuperPu][i] = PMTIIndexInPu;
                    g_IdleFlushPos[ulSuperPu] = PMTIIndexInPu;
                    ulActualFlushCNT[ulSuperPu] ++;
                    break;
                }
            }
        }
    }

    for (ulSuperPu = 0; ulSuperPu < SUBSYSTEM_SUPERPU_NUM; ulSuperPu++)
    {
        //statistic cnt
        ulAllFlushCNT += ulActualFlushCNT[ulSuperPu];

        if (ulActualFlushCNT[ulSuperPu] > 0)
        {
            L2_SetTableRW(ulSuperPu, FlushPMTIIndex[ulSuperPu], ulActualFlushCNT[ulSuperPu], TABLE_WRITE);
            L2_SetThreadQuota((U8)ulSuperPu, SYS_THREAD_TABLE_PMT, 1);
        }
    }

    if (0 == ulAllFlushCNT)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

void L2_UpdateFlushPos(U16 PUSer, U16 Pos)
{
    g_PMTFlushManager[PUSer].m_CurPMTFlushPosOfCE = Pos;
}


void L2_PrintFlushPos(void)
{
    U16 PUSer;
    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        DBG_Printf("PU %d PMTFlushPos %d\n", PUSer, g_PMTFlushManager[PUSer].m_CurPMTFlushPosOfCE);
    }
}

void L2_InitFTLDptr(void)
{
    U32 i, j, k;

    /* init FTL write Dptr */
    for (j = 0; j < SUBSYSTEM_SUPERPU_NUM; j++)
    {
        g_FTLDptr[j].pCurBufREQ = NULL;
    }

    /* init FTL read Dptr */
    for (j = 0; j < SUBSYSTEM_SUPERPU_NUM; j++)
    {
        g_ulPopLunBitMap[j] = 0;

        for (k = 0; k < LUN_NUM_PER_SUPERPU; k++)
        {
            g_FTLReadDptr[j][k].m_BufReqID = INVALID_2F;
            g_FTLReadDptr[j][k].m_BufReqLpnBitMap = 0;
            g_FTLReadDptr[j][k].m_LPNRemain = 0;
            g_FTLReadDptr[j][k].m_LPNOffset = 0;
            g_FTLReadDptr[j][k].m_bPMTLookuped = FALSE;

            for (i = 0; i < LPN_PER_BUF; i++)
            {
                g_FTLReadDptr[j][k].m_Addr[i].m_PPN = INVALID_8F;
                g_FTLReadDptr[j][k].m_ReqLength[i] = 0;
                COM_MemZero((U32*)&g_FTLReadDptr[j][k].BufReqHostInfo[i], sizeof(BUFREQ_HOST_INFO) / sizeof(U32));
            }
        }
    }

    return;
}

BOOL MCU1_DRAM_TEXT L2_IsFTLIdle(void)
{
    U32 i, j;
    COMMON_EVENT L2_Event;

    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);

    /* shutdown before bootup OK may trigger table GC to flush PMTpage not load, add by henryluo 2015-02-09 */
    if (FALSE == L2_IsBootupOK())
    {
        return FALSE;
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (SYSTEM_STATE_NORMAL != L2_GetCurrSystemState(i))
        {
            return FALSE;
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (NULL != g_FTLDptr[i].pCurBufREQ)
        {
            return FALSE;
        }

        for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
        {
            if (INVALID_2F != g_FTLReadDptr[i][j].m_BufReqID)
            {
                return FALSE;
            }
        }
    }

    if (L2_Event.EventShutDown)
    {
        /* finish TLC open block before shutdown */
        for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
        {
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            if ((g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt == 0) && (TLC_WRITE_ALL != g_TLCManager->aeTLCWriteType[i]))
#else
            if (TLC_WRITE_ALL != g_TLCManager->aeTLCWriteType[i])
#endif
            {
                //DBG_Printf("-2- TLCWriteType %d GCStg %d TLCStg %d\n", g_TLCManager->aeTLCWriteType[i], g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[0], g_TLCManager->aeTLCStage[i]);
                return FALSE;
            }
        }

        /* finish TLCGC before shutdown*/
        for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
        {
            if (VBT_TYPE_INVALID != g_GCManager[TLCGC_MODE]->tGCCommon.m_ucSrcBlkType[i])
            {
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                if (g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt == 0)
#endif                
                {
                    //DBG_Printf("-3- PU %d Stg %d\n",i, g_GCManager[TLCGC_MODE]->tGCCommon.m_GCStage[i]);
                    return FALSE;
                }
            }
        }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        if(g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt == 0)
        {
           /* Shutdown improvement : avoid L2_FTLEntry() idle loop running and keep quotas for L2_TableEntry() */
           for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
           {
               L2_SetThreadQuota(i, SYS_THREAD_FTL, 1);
               g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt++;
           }
        }
#else
        if(g_L2EventStatus.m_ShutdownEraseTLCBlkDoneCnt == 0)
        {
           /* Shutdown improvement : avoid L2_FTLEntry() idle loop running and keep quotas for L2_TableEntry() */
           for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
           {
               L2_SetThreadQuota(i, SYS_THREAD_FTL, 1);
               g_L2EventStatus.m_ShutdownEraseTLCBlkDoneCnt++;
           }
        }
#endif
    }

    /* RW and LOADTABLE state may be exist in shundown event, modified by henryluo for shutdown */
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (g_TableRW[i].m_TableStatus != TABLE_STATUS_OVER)
        {
            return FALSE;
        }
    }


    if (TRUE == L1_BufReqEmpty())
    {
        return TRUE;
    }
    else
    {
        if (FALSE == L1_AllHighPrioLinkEmpty())
        {
            return FALSE;
        }
        else if (0 != L1_GetLowPrioFifoPendingCnt())
        {
            return FALSE;
        }

        return TRUE;
    }
}

U8 L2_SelectReadBufReq(U8 ucSuperPu, U32* pPopLunBitmap, U8* pLunInSPU, BOOL* bNeedPop, U8* pPreBufReqId)
{
    BUF_REQ_READ* pReq;
    U32 LPNOffset, bFind = FALSE;
    U8 ucCurBufReqID, ucPreBufReqID = INVALID_2F;
    U8 ucTLun, ucLunInSuperPu, ucSearchCnt = 0;

    ucCurBufReqID = L1_GetHighPrioLinkHead(ucSuperPu);
    if (INVALID_2F == ucCurBufReqID)
    {
        return ucCurBufReqID;
    }

    while (FALSE == bFind)
    {
        pReq = L1_GetReadBufReq(ucSuperPu, ucCurBufReqID);

        if (pReq->bPMTLookuped)
        {
            LPNOffset = pReq->ucLPNOffset;

#ifdef READ_DISTURB_OPEN
            /* Bug fix */
            if (TRUE == pReq->bNeedReLookupPMT)
            {
                L2_LookupPMT(&pReq->aFlashAddr[LPNOffset], pReq->aLPN[LPNOffset], FALSE);
            }
#endif

            if (INVALID_8F == pReq->aFlashAddr[LPNOffset].m_PPN)
            {
                bFind = TRUE;
                *bNeedPop = TRUE;
                break;
            }

            ucLunInSuperPu = pReq->aFlashAddr[LPNOffset].m_OffsetInSuperPage;
            ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
            if (pReq->aFlashAddr[LPNOffset].m_PUSer == ucSuperPu && (*pPopLunBitmap & (1 << ucTLun)))
            {
                bFind = TRUE;
                *pLunInSPU = ucLunInSuperPu;
                *pPopLunBitmap &= ~(1 << ucTLun);
                *bNeedPop = TRUE;
                break;
            }
            else
            {
                ucPreBufReqID = ucCurBufReqID;
                ucCurBufReqID = L1_GetHighPrioLinkNextNode(ucSuperPu, ucCurBufReqID);
                ucSearchCnt++;
                if (INVALID_2F == ucCurBufReqID)
                {
                    *bNeedPop = FALSE;
                    break;
                }
            }
        }
        else
        {
            *bNeedPop = FALSE;
            bFind = TRUE;
            break;
        }
    }

    *pPreBufReqId = ucPreBufReqID;

    //L1_RemoveBufReqFromHighPrioLink(ucSuperPu, ucPreBufReqID);
    return ucCurBufReqID;
}

BOOL L2_PopReadBufREQ(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucBufReqID, ucPreBufReqID, ucLunInSPU = INVALID_2F;
    BUF_REQ_READ* pReq;
    U32 i;
    BOOL bNeedPop = TRUE, bPoped = FALSE;
    L2_FTLReadDptr* pFTLReadDptr;
    U32 ulPopLunBitmap = ulLunAllowToSendFcmdBitmap;
    BOOL bRet = FALSE;

    if (INVALID_2F == g_FTLReadDptr[ucSuperPu][0].m_BufReqID)
    {
        while (0 != ulPopLunBitmap && TRUE == bNeedPop)
        {
            ucLunInSPU = INVALID_2F;
            ucBufReqID = L2_SelectReadBufReq(ucSuperPu, &ulPopLunBitmap, &ucLunInSPU, &bNeedPop, &ucPreBufReqID);

            if (INVALID_2F == ucBufReqID)
            {
                return bRet;
            }

            pReq = L1_GetReadBufReq(ucSuperPu, ucBufReqID);

            if (INVALID_2F != ucLunInSPU)
            {
                L1_RemoveBufReqFromHighPrioLink(ucSuperPu, ucPreBufReqID);

                pFTLReadDptr = &g_FTLReadDptr[ucSuperPu][ucLunInSPU];
                g_ulPopLunBitMap[ucSuperPu] |= (1 << ucLunInSPU);
                bPoped = TRUE;
            }
            else
            {
                pFTLReadDptr = &g_FTLReadDptr[ucSuperPu][0];
                if (bPoped)
                {
                    return bRet;
                }
                else
                {
                    L1_RemoveBufReqFromHighPrioLink(ucSuperPu, ucPreBufReqID);
                }
            }

            pFTLReadDptr->m_BufReqID = ucBufReqID;
            //pFTLReadDptr->m_ReqRemain = pReq->ucReqLength;
            pFTLReadDptr->m_LPNRemain = pReq->ucLPNCount;
            pFTLReadDptr->m_LPNOffset = pReq->ucLPNOffset;
            pFTLReadDptr->m_bPMTLookuped = FALSE;
            pFTLReadDptr->m_BufReqLpnBitMap = 0;            

            for (i = pReq->ucLPNOffset; i < (pReq->ucLPNOffset + pReq->ucLPNCount); i++)
            {
                pFTLReadDptr->m_Addr[i].m_PPN = INVALID_8F;
                pFTLReadDptr->m_ReqLength[i] = 0;

                COM_MemCpy((U32*)&pFTLReadDptr->BufReqHostInfo[i], (U32*)&pReq->tBufReqHostInfo, sizeof(BUFREQ_HOST_INFO) / sizeof(U32));

#ifdef SIM
                /* lba range check */
                if ((pReq->aLPN[i] != INVALID_8F) && (pReq->aLPN[i] >= MAX_LPN_IN_DISK))
                {
                    DBG_Printf("FetchBufferReq LPN 0x%x overflow!!\n", pReq->aLPN[i]);
                    DBG_Getch();
                }
#endif
            }

            bRet = TRUE;
            if (FALSE == bPoped)
            {
                break;
            }
        }
    }
    else
    {
        bRet = TRUE;
    }

    return bRet;
}


void L2_PopWriteBufREQ(U8 ucSuperPu)
{
    BUF_REQ_WRITE* pReq;
    U32 i;

    if (g_FTLDptr[ucSuperPu].pCurBufREQ == NULL)
    {
        pReq = L1_WriteBufReqPop(ucSuperPu);
        g_FTLDptr[ucSuperPu].pCurBufREQ = pReq;

        for (i = 0; i < LPN_PER_BUF; i++)
        {
            if ((pReq->LPN[i] != INVALID_8F) && (pReq->LPN[i] >= MAX_LPN_IN_DISK))
            {
                DBG_Printf("FetchBufferReq LPN 0x%x overflow!!\n", pReq->LPN[i]);
                DBG_Getch();
            }
        }
    }

    g_IdleFlushPos[ucSuperPu] = 0;

    return;
}

void UpdateTablesBeforeWrite(BUF_REQ_WRITE* pBufReq, PhysicalAddr* pAddr)
{
    U16 i;
    PhysicalAddr OriginalAddr = { 0 };
    U32 LPN;
    U32* pLPN = &pBufReq->LPN[0];
    U32 ulTotalLpnPerBlk;

    for (i = 0; i < LPN_PER_BUF; i++)
    {
        LPN = pLPN[i];

        /* if LPN have no been writen, increase the target block dirty count */
        if (INVALID_8F == LPN)
        {
            /* increase the dirty count of the block */
            L2_IncreaseDirty(pAddr->m_PUSer, pAddr->m_BlockInPU, 1);
            L2_IncreaseOffsetInAddr(pAddr);
            continue;
        }

        /* update LPN point to the new physical addr */
        OriginalAddr.m_PPN = INVALID_8F;
        L2_UpdatePMT(pAddr, &OriginalAddr, LPN);
        if (INVALID_8F != OriginalAddr.m_PPN)
        {
            /* If LPN is trimmed,dirtycnt has been already increased*/
            //if(OriginalAddr.m_bInvalid == 0)
            {
                /* increase the dirty count of the original block */
                L2_IncreaseDirty(OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU, 1);

                L2_UpdateDirtyLpnMap(&OriginalAddr, B_DIRTY);

                /* if original block all dirty during write, trigger to erase it */
                if (TRUE == L2_VBT_Get_TLC(OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU))
                {
                    ulTotalLpnPerBlk = LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT;
                }
                else
                {
                    ulTotalLpnPerBlk = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;
                }

                // check if the dirty count of the original block has reached the maximum

                if (ulTotalLpnPerBlk == L2_GetDirtyCnt(OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU))
                {
                    // the drity count has reached the maximum

                    /* this block must not be the GC/WL source block and write target block */
                    if ((VBT_NOT_TARGET == pVBT[OriginalAddr.m_PUSer]->m_VBT[OriginalAddr.m_BlockInPU].Target) &&
                        (TRUE != L2_PBIT_Get_Lock(OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU)))
                    {
                        //FIRMWARE_LogInfo("UpdataTableBeforeWrite SuperPU %d BLK 0x%x into EreaseQue\n", OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU);
                        if (ulTotalLpnPerBlk == LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT)                        
                            L2_InsertBlkIntoEraseQueue(OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU, FALSE);
                        else
                            L2_InsertBlkIntoEraseQueue(OriginalAddr.m_PUSer, OriginalAddr.m_BlockInPU, TRUE);

#ifdef L2MEASURE
                        L2MeasureLogIncECTyepCnt(OriginalAddr.m_PUSer, L2_VBT_GetPhysicalBlockAddr(OriginalAddr.m_PUSer,0, OriginalAddr.m_BlockInPU), L2MEASURE_ERASE_HOST);
#endif
                    }
                }
            }
        }
        //FIRMWARE_LogInfo("FTL LPN 0x%x -> Addr 0x%x TS %d\n", LPN, pAddr->m_PPN, L2_GetTimeStampInPu(pAddr->m_PUSer));

        /* Move update DebugPMT here,because L1 write buffer management changed,Nina 2016-02-29 */
#ifdef DBG_PMT
        L2_UpdateDebugPMT(pAddr, LPN);
#endif

        L2_UpdateDirtyLpnMap(pAddr, B_VALID);
        L2_IncreaseOffsetInAddr(pAddr);

    }
}

void FillSpareAreaInWrite(RED* pSpare, U32* pLPN, U8 ucSuperPu, U8 ucLUNInSuperPU, BLOCK_TYPE ActualBlockType, TargetType eTargetType)
{
    U16 i = 0;
    U32 TimeStamp;

    TimeStamp = L2_GetTimeStampInPu(ucSuperPu);
    pSpare->m_RedComm.ulTimeStamp = TimeStamp;

    pSpare->m_RedComm.ulTargetOffsetTS = g_PuInfo[ucSuperPu]->m_TargetOffsetTS[eTargetType][ucLUNInSuperPU];

    for (i = 0; i < LPN_PER_BUF; i++)
    {
        pSpare->m_DataRed.aCurrLPN[i] = pLPN[i];
    }

    pSpare->m_RedComm.bcPageType = PAGE_TYPE_DATA;   //data type
    pSpare->m_RedComm.bcBlockType = ActualBlockType;
}

void FillSpareAreaInWriteTable(RED* pSpare, U8 ucSuperPu, U8 ucLUNInSuperPU, PAGE_TYPE PageType, TargetType FlushType)
{
    U32 TimeStamp;
    U16 i = 0;

    TimeStamp = L2_GetTimeStampInPu(ucSuperPu);

    pSpare->m_RedComm.ulTimeStamp = TimeStamp;
    pSpare->m_RedComm.ulTargetOffsetTS = g_PuInfo[ucSuperPu]->m_TargetOffsetTS[FlushType][ucLUNInSuperPU];

    for (i = 0; i < LPN_PER_BUF; i++)
    {
        pSpare->m_DataRed.aCurrLPN[i] = INVALID_8F;
    }

    pSpare->m_RedComm.bcPageType = PageType;   //data type

}

/*****************************************************************************
Prototype      : L2_IsFlashPageSafed
Description    : to check if the flash page is safed for L95 issue.
Input          : U32 ulSuperPu
Output         : None
Return Value   : U32
Calls          :
Called By      :

History        :
1.Date         : 2014/4/2
Author         : henryluo
Modification   : Created function
2.Date         : 2015/8/21
Author         : JasonGuo
Modification   : delete the WriteType parameter
*****************************************************************************/
U32 L2_IsFlashPageSafed(PhysicalAddr* pAddr)
{
    U8 ucSuperPu;
    U16 usBlock;
    U16 usPage;
    U16 usPPO;
    PuInfo* pInfo;
    //TargetType* Type = NULL;
    U8 ucLunInSuperPu, ucTLun;

    if (INVALID_8F == pAddr->m_PPN)
    {
        return TRUE;
    }

    ucSuperPu = pAddr->m_PUSer;
    ucLunInSuperPu = pAddr->m_OffsetInSuperPage;
    usBlock = pAddr->m_BlockInPU;
    usPage = pAddr->m_PageInBlock;
    ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

    pInfo = g_PuInfo[ucSuperPu];
    if (usBlock == pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
    {
        usPPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1;
        //*Type = TARGET_HOST_WRITE_NORAML;
    }
    else if (usBlock == pInfo->m_TargetBlk[TARGET_HOST_GC])
    {
        usPPO = pInfo->m_TargetPPO[TARGET_HOST_GC] - 1;
        //*Type = TARGET_HOST_GC;
    }
    else
    {
        return TRUE;
    }


    if (FALSE == HAL_IsFlashPageSafed(ucTLun, usPPO, usPage))
    {
        //FIRMWARE_LogInfo("L2_IsFlashPageSafed Pu %d  usWritePPO %d usReadPPO %d\n", ucSuperPu, usPPO, usPage);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*****************************************************************************
Prototype      : L2_DummyWrite
Description    : write dummy page to protect read page safed
Input          : None
Output         : None
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2014/4/2
Author       : henryluo
Modification : Created function

*****************************************************************************/
L2_FTL_STATUS  L2_DummyWrite(U8 ucSuperPu, TargetType Type)
{
    BLOCK_TYPE BlockType;
    PhysicalAddr Addr = { 0 };
    RED Spare;
    U8 FlushStatus;
    PuInfo* pInfo;
    U32 LPN[LPN_PER_BUF];
    U8 i;
    U8 ucLunInSuperPu, ucTLun;

    /* init dummy write LPN to INVALID_8F */
    for (i = 0; i < LPN_PER_BUF; i++)
    {
        LPN[i] = INVALID_8F;
    }

    /*Initial BlockType for partial DummyWrie*/
    BlockType = BLOCK_TYPE_SEQ + Type;

    pInfo = g_PuInfo[ucSuperPu];

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (FALSE == L2_FCMDQNotFull(ucTLun))
        {
            continue;
        }

        /* whether write this page or not according to the state of allocate page in this Pu */
        if (0 == g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[Type])
        {
            Addr.m_PPN = L2_AllocateOnePage(ucSuperPu, LPN, Type, &BlockType);
            if (Addr.m_PPN == INVALID_8F)
            {
                /* if target block is closed, dummy write done! */
                if (TARGET_HOST_WRITE_NORAML == Type)
                {
                    if (PG_PER_SLC_BLK == pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
                    {
                        return FTL_STATUS_OK;
                    }
                }
                else if (TARGET_HOST_GC == Type)
                {
                    if (PG_PER_SLC_BLK == pInfo->m_TargetPPO[TARGET_HOST_GC])
                    {
                        return FTL_STATUS_OK;
                    }
                }
                else if (TARGET_TLC_WRITE == Type)
                {
                    DBG_Printf("TARGET_TLC_WRITE shouldn't have dummy write\n");
                    DBG_Getch();
                }
                DBG_Printf("[L2_DummyWrite]: No free page!!!\n");
                DBG_Getch();
                return FTL_STATUS_NO_FREE_PAGE;
            }
        }
        else
        {
            if (TRUE == COM_BitMaskGet(g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[Type], ucLunInSuperPu))
            {
                continue;
            }
            Addr.m_PPN = 0;
            Addr.m_PUSer = ucSuperPu;
            Addr.m_BlockInPU = g_PuInfo[ucSuperPu]->m_TargetBlk[Type];
            Addr.m_PageInBlock = g_PuInfo[ucSuperPu]->m_TargetPPO[Type] - 1;
        }

        Addr.m_OffsetInSuperPage = ucLunInSuperPu;

        /* dummy write dirty count on seq target need to wait increase during adjust dirty count */
        if ((TRUE == L2_IsAdjustDirtyCntDone()) || (TARGET_HOST_WRITE_NORAML != Type))
        {
            /* increase the dirty count of the block */
            L2_IncreaseDirty(Addr.m_PUSer, Addr.m_BlockInPU, LPN_PER_BUF);
        }

        FillSpareAreaInWrite(&Spare, LPN, ucSuperPu, ucLunInSuperPu, BlockType, Type);
        Spare.m_RedComm.bsVirBlockAddr = Addr.m_BlockInPU;
        Spare.m_RedComm.eOPType = OP_TYPE_DUMMY_WRITE;

        /* remove the check code for SuperPage, because it may exist dummy write page0,Nina 2016-02-16*/
#if 0 //def SIM
        if (0 == Addr.m_PageInBlock)
        {
            DBG_Printf("L2_DummyWrite shouldn't write page 0!!\n");
            DBG_Getch();
        }
#endif

        L2_FtlWriteLocal(&Addr, (U32 *)g_L2TempBufferAddr, (U32*)&Spare, &FlushStatus, FALSE, TRUE,NULL);

#ifdef L2MEASURE
        L2MeasureLogIncWCnt(ucSuperPu, L2MeasureLogTargetTypeMapping(Type));
#endif

        COM_BitMaskSet((U32*)&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[Type], ucLunInSuperPu);
        g_PuInfo[ucSuperPu]->m_TargetOffsetTS[Type][ucLunInSuperPu] = L2_GetTargetOffsetTS(&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[Type]);
        if (SUPERPU_LUN_NUM_BITMSK == g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[Type])
        {
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
            L2_IncTimeStampInPu(ucSuperPu);
#endif
            g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[Type] = 0;
        }
    }
    return FTL_STATUS_OK;
}

/****************************************************************************
Name        :L2_FtlReadProcess
Input       :BUF_REQ* pReq
Output      :
Author      :HenryLuo
Date        :2013.08.19    15:15:35
Description :
Others      :
Modify      :
****************************************************************************/
void L2_SetBufReqLpnBitMap(U8 ucSuperPu, U8 ucLunInSPU, U16 Pos)
{
    g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_BufReqLpnBitMap |= (1 << Pos);
}


void L2_ClearBufReqLpnBitMap(U8 ucSuperPu, U8 ucLunInSPU, U16 Pos)
{
    g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_BufReqLpnBitMap &= ~(1 << Pos);
}

U16 L2_GetBufReqLpnBitMap(U8 ucSuperPu, U8 ucLunInSPU)
{
    return g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_BufReqLpnBitMap;
}

U8 L2_GetBufReqLunInSuperPu(U8 ucSuperPu, U8 ucLunInSPU, U8 Pos)
{
    U8 ucLunInSuperPu = g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_Addr[Pos].m_OffsetInSuperPage;
    return ucLunInSuperPu;
}

void L2_SetBufReqLength(BUF_REQ_READ* pReq, U8 ucSuperPu, U8 ucLunInSPU)
{
    U8 Pos;
    U8 ucReqLenth = 0;
    U8 LPNOffset = pReq->ucLPNOffset;
    U8 ReqRemain = pReq->ucReqLength;
    U8 MoveLength = 0;

    for (Pos = LPNOffset; Pos < (pReq->ucLPNOffset + pReq->ucLPNCount); Pos++)
    {
#if defined (HOST_AHCI) || defined(HOST_NVME)
        L2_MoveHostPointer(&g_FTLReadDptr[ucSuperPu][ucLunInSPU].BufReqHostInfo[Pos].HmemDptr, MoveLength);
#endif
        if (Pos == pReq->ucLPNOffset)
        {
            ucReqLenth = SEC_PER_LPN - (pReq->ucReqOffset % SEC_PER_LPN);
            if (ReqRemain < ucReqLenth)
            {
                ucReqLenth = ReqRemain;
            }
        }
        else
        {
            ucReqLenth = (ReqRemain >= SEC_PER_LPN) ? SEC_PER_LPN : ReqRemain;
        }
        g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[Pos] = ucReqLenth;
        ReqRemain -= ucReqLenth;
        MoveLength += ucReqLenth;
    }
}

BOOL L2_FtlReadProcess(U8 ucSuperPu, U8 ucLunInSPU, U32 ulLunAllowToSendFcmdBitmap, BUF_REQ_READ *pReq)
{
    U32 ulRet;

    if (FALSE == g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_bPMTLookuped)
    {
        //get physical address for all LPN
        while (g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_LPNRemain > 0)
        {
            U16 Pos;
            PhysicalAddr* pAddr;
            U32 LPN;

            Pos = g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_LPNOffset;
            pAddr = &g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_Addr[Pos];
            if (FALSE == pReq->bPMTLookuped)
            {
                LPN = pReq->aLPN[Pos];

                /* lookup in RPMT buffer before abnormal boot up ok, add by henryluo 2014-01-08 in TaiPei */
                if (TRUE != L2_IsAdjustDirtyCntDone())
                {
                    if (FALSE == L2_RebuildLookupRPMTBuffer(pAddr, LPN))
                    {
                        L2_LookupPMT(pAddr, LPN, FALSE);
                    }
                }
                else
                {
                    L2_LookupPMT(pAddr, LPN, FALSE);
                }
            }
            else
            {
                pAddr->m_PPN = pReq->aFlashAddr[Pos].m_PPN;
            }

            L2_SetBufReqLpnBitMap(ucSuperPu, ucLunInSPU, Pos);

            g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_LPNOffset++;
            g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_LPNRemain--;
        }

        g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_LPNOffset = pReq->ucLPNOffset;

        g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_bPMTLookuped = TRUE;

        if (TRUE != pReq->bReqLocalREQFlag)
        {
            L2_SetBufReqLength(pReq, ucSuperPu, ucLunInSPU);
        }
    }

    /* merge read */
    if (TRUE == pReq->bReqLocalREQFlag)
    {
        ulRet = L2_FtlReadMerge(ucSuperPu, ulLunAllowToSendFcmdBitmap, pReq, g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_Addr);
    }
    /* host read and prefetch read */
    else
    {
        ulRet = L2_FtlReadNormal(ucSuperPu, ucLunInSPU, ulLunAllowToSendFcmdBitmap, pReq, g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_Addr);
    }

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE, "L2 FTL read");

    return ulRet;
}

#ifndef SIM
void MCU1_DRAM_TEXT L2_DbgFTL(U16 FlagID,U8 PUSer)
{
    U8 ucSuperPu = PUSer;
    U32 ulCycleCnt = XT_RSR_CCOUNT();

    switch(FlagID)
    {
    case 5:
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[FTLEntry]: Flush PMT during GC ");
        TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, " ");
        break;

    case 6:
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[FTLEntry]: Need GC ");
        TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, " ");
        break;

    case 7:
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[FTLEntry]: FTL Write during GC ");
        TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, " ");
        break;

    case 12:
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[FTLEntry]: FTL Read during GC ");
        TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, " ");
        break;   

    case 13:
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[FTLEntry]: FTL no cmd during GC ");
        TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, " ");
        break;     

    default:
        break;                  
    }

}
#else
void MCU1_DRAM_TEXT L2_DbgFTL(U16 FlagID, U8 PUSer)
{
}
#endif

/*****************************************************************************
 Prototype      : L2_FtlWriteNormal
 Description    : host write
 Input          : BUF_REQ *pReq
 PhysicalAddr *pAddr
 U32 *pSpare
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/8/28
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_FtlWriteNormal(BUF_REQ_WRITE *pReq, PhysicalAddr *pAddr, U32 *pSpare, BOOL bSLCMode)
{
    U8 ucTLun;
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;

#ifdef SIM
    if (pAddr->m_PUSer > SUBSYSTEM_SUPERPU_NUM
     || pAddr->m_BlockInPU > (BLK_PER_PLN + RSV_BLK_PER_PLN)
     || pAddr->m_PageInBlock > PG_PER_SLC_BLK)
    {
        DBG_Printf("L2_FtlWriteLocal Invalid PhysicalAddr SuperPu %d VirBlk 0x%x Pg 0x%x\n",
        pAddr->m_PUSer, pAddr->m_BlockInPU, pAddr->m_PageInBlock);
        DBG_Getch();
    }
#endif

    ucTLun = L2_GetPuFromAddr(pAddr);
    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsNeedL3ErrHandle = (TRUE == bSLCMode) ? TRUE : FALSE;

    ptReqEntry->tFlashDesc.bsVirBlk = pAddr->m_BlockInPU;
    ptReqEntry->tFlashDesc.bsVirPage = pAddr->m_PageInBlock;
    ptReqEntry->tFlashDesc.bsBlkMod = (TRUE == bSLCMode) ? FCMD_REQ_SLC_BLK : FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF;

    ptReqEntry->atBufDesc[0].bsBufID = pReq->PhyBufferID;
    ptReqEntry->atBufDesc[0].bsSecStart = 0;
    ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
    ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
#ifdef DATA_EM_ENABLE
    L2_FillRedForEM(pTargetRed, pSpare, RED_SW_SZ_DW);
#else
    COM_MemCpy(pTargetRed, pSpare, RED_SW_SZ_DW);
#endif
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;

    ptReqEntry->ulReqStsAddr = pReq->ReqStatusAddr;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

    //ptReqEntry->tHostDesc.tHostDptr.bsCmdTag = pReq->Tag;
    ptReqEntry->tHostDesc.ulFtlLba = pReq->LPN[0] << SEC_PER_LPN_BITS;

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

#if (!defined(HAL_UNIT_TEST) && !defined(L3_UNIT_TEST))
    L1_WriteBufReqMoveHeadPtr(pAddr->m_PUSer);
#endif
    
    return;
}

/*****************************************************************************
 Prototype      : L2_FtlWriteLocal
 Description    : FTL local write
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/9/2
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
#ifdef DATA_EM_ENABLE
void L2_FillRedForEM(U32 *pTargetRed, U32* pSpare, U32 ulDW)
{
    U32 Pln;
    U32 RedIndex;
    U32* pDst = pTargetRed;
    U32* pSrc = pSpare;

    for (RedIndex = 0; RedIndex < ulDW / RED_SW_SZ_DW; RedIndex++)
    {
        pDst = pTargetRed + RedIndex * RED_SW_SZ_DW;
        pSrc = pSpare + RedIndex * RED_SW_SZ_DW;
        for (Pln = 0; Pln < PLN_PER_LUN; Pln++)
        {
            *((U32*)pDst + RED_SZ_DW*Pln) = 0 + Pln * 32;
            *((U32*)pDst + RED_SZ_DW*Pln + 1) = (1 << 3) + Pln * 32;
            *((U32*)pDst + RED_SZ_DW*Pln + 2) = (2 << 3) + Pln * 32;
            *((U32*)pDst + RED_SZ_DW*Pln + 3) = (3 << 3) + Pln * 32;

            COM_MemCpy(pDst + RED_SZ_DW*Pln + 4, pSrc, (RED_SZ_DW - 4));
            pSrc += (RED_SZ_DW - 4);
        }
    }
}
#endif

void L2_FtlWriteLocal(PhysicalAddr* pAddr, U32* pPage, U32* pSpare, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, XOR_PARAM *pXorParam)
{
    U8 ucTLun;    
    U16 usBuffID;
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;
    RED *pRed = (RED *)pSpare;

#ifdef SIM
    if (pAddr->m_PUSer > SUBSYSTEM_SUPERPU_NUM 
     || pAddr->m_BlockInPU > (BLK_PER_PLN + RSV_BLK_PER_PLN) 
     || pAddr->m_PageInBlock > (PG_PER_SLC_BLK * PG_PER_WL))
    {
        DBG_Printf("L2_FtlWriteLocal Invalid PhysicalAddr SuperPu %d VirBlk 0x%x Pg 0x%x\n", pAddr->m_PUSer, pAddr->m_BlockInPU, pAddr->m_PageInBlock);
        DBG_Getch();
    }
#endif

#if defined(SWL_EVALUATOR)
    SWLRecordIncSLCExtraWCnt(pAddr->m_PUSer);
    if (pPBIT[pAddr->m_PUSer]->m_PBIT_Entry[0][pAddr->m_BlockInPU].bTable == TRUE)
    {
        SWLRecordIncSLCTableWCnt(pAddr->m_PUSer);
    }
#endif

    usBuffID = ((U32)pPage - DRAM_START_ADDRESS) / BUF_SIZE;
#ifndef SIM
    if ((U32)pPage > DRAM_HIGH_START_ADDRESS)
    {
        usBuffID = ((U32)pPage - DRAM_HIGH_START_ADDRESS) / BUF_SIZE;
    }
#endif

    ucTLun = L2_GetPuFromAddr(pAddr);
    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);    
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsTableReq = bTableReq;

    ptReqEntry->tFlashDesc.bsVirBlk = pAddr->m_BlockInPU;
    ptReqEntry->tFlashDesc.bsVirPage = pAddr->m_PageInBlock;
    ASSERT(TRUE==bSLCMode);
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;

    if (pRed->m_RedComm.bcBlockType == BLOCK_TYPE_SEQ || pRed->m_RedComm.bcBlockType == BLOCK_TYPE_RAN)
    {
        /* SLC HW RPMT write and SLC GC write will use this interface, when program fail, L3 needs to do error handling.
                 Thus, need to set bsNeedL3ErrHandle flag. */
        ptReqEntry->bsNeedL3ErrHandle = TRUE;
    }

    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF;
        
    ptReqEntry->atBufDesc[0].bsBufID = usBuffID;
    ptReqEntry->atBufDesc[0].bsSecStart = 0;
    ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_BUF;
    ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
#ifdef DATA_EM_ENABLE
    if (bTableReq)
    {
        COM_MemCpy(pTargetRed, pSpare, RED_SW_SZ_DW);
    }
    else
    {
        L2_FillRedForEM(pTargetRed, pSpare, RED_SW_SZ_DW);
    }
#else
    COM_MemCpy(pTargetRed, pSpare, RED_SW_SZ_DW);
#endif
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    if (pXorParam != NULL)
    {
        ptReqEntry->bsXorEn = pXorParam->bsXorEn;
        ptReqEntry->bsXorStripeId = pXorParam->bsXorStripeId;
        ptReqEntry->bsContainXorData = pXorParam->bsContainXorData;
    }

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
    
    return;
}

void L2_FtlTLCExternalWriteLocal(PhysicalAddr* pAddr, U32* pPBuffer, U32* pSpare, U8* pStatus, XOR_PARAM *pXorParam)
{
    U8 ucIndex, ucTLun;
    U16 aBuffID[DSG_BUFF_SIZE];
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;
    U8 ucPageType;
    U16 uc2ndPageNum = 0;
    U8 ucBuffCnt;

    ucPageType = L3_GetPageTypeByProgOrder(pAddr->m_PageInBlock);
    ucBuffCnt = L3_GetProgPageCntByProgOrder(pAddr->m_PageInBlock);
    if (ucBuffCnt == 2)
        uc2ndPageNum = L3_Get2ndPageNumByProgOrder(pAddr->m_PageInBlock);
#if 0
    U8 ucPairPageType = HAL_GetFlashPairPageType(pAddr->m_PageInBlock);

    if (LOW_PAGE_WITHOUT_HIGH == ucPairPageType)
    {
        ucBuffCnt = 1;
    }
    else if (LOW_PAGE == ucPairPageType || EXTRA_PAGE == ucPairPageType)
    {
        ucBuffCnt = 2;
    }
    else
    {
        DBG_Getch();
    }
#endif

    for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
    {
        aBuffID[ucIndex] = ((U32)(pPBuffer[ucIndex]) - DRAM_START_ADDRESS) >> BUF_SIZE_BITS;
    }
#ifndef SIM
    if ((U32)pPBuffer[0] > DRAM_HIGH_START_ADDRESS)
    {
        for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
        {
            aBuffID[ucIndex] = ((U32)pPBuffer[ucIndex] - DRAM_HIGH_START_ADDRESS) >> BUF_SIZE_BITS;
        }
    }
#endif
    for (; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        aBuffID[ucIndex] = INVALID_4F;
    }
       
#if defined(SWL_EVALUATOR)
    SWLRecordIncTLCExtraWCnt(pAddr->m_PUSer);
#endif

    ucTLun = L2_GetPuFromAddr(pAddr);
    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = (1 == ucBuffCnt) ? FCMD_REQ_SUBTYPE_ONEPG : FCMD_REQ_SUBTYPE_NORMAL;

    ptReqEntry->tFlashDesc.bsVirBlk = pAddr->m_BlockInPU;
    ptReqEntry->tFlashDesc.bsVirPage = pAddr->m_PageInBlock;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_BUF * ucBuffCnt;
    
    for (ucIndex = 0; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        ptReqEntry->atBufDesc[ucIndex].bsBufID = aBuffID[ucIndex];
        ptReqEntry->atBufDesc[ucIndex].bsSecStart = 0;
        ptReqEntry->atBufDesc[ucIndex].bsSecLen = SEC_PER_BUF;
    }

    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);

#ifdef DATA_EM_ENABLE
    L2_FillRedForEM(pTargetRed, pSpare, RED_SW_SZ_DW*ucBuffCnt);
#else
    COM_MemCpy(pTargetRed, pSpare, RED_SW_SZ_DW*ucBuffCnt);
#endif
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;
    
    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    if (pXorParam != NULL)
    {
        ptReqEntry->bsXorEn = pXorParam->bsXorEn;
        ptReqEntry->bsXorStripeId = pXorParam->bsXorStripeId;
        ptReqEntry->bsContainXorData = pXorParam->bsContainXorData;
    }

    ptReqEntry->tLocalDesc.bsPairPageType = ucPageType + 1;
    ptReqEntry->tLocalDesc.bsPairPageCnt = ucBuffCnt;
    ptReqEntry->tLocalDesc.bsPairPageNum= uc2ndPageNum;
    
    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
    
    return;
}

void L2_FtlTLCInternalWriteLocal(PhysicalAddr *pSrcAddr, PhysicalAddr *pDesAddr, U8* pStatus)
{
    U8 ucTLun, ucIndex;
    FCMD_REQ_ENTRY *ptReqEntry;

#if defined(SWL_EVALUATOR)
    //TLCMerge/GC/SWL all adopt external Write in current 3533 edition 20170605
    SWLRecordIncTLCExtraWCnt(pSrcAddr->m_PUSer);
    SWLRecordIncSLC2TLCMergeWCnt(pSrcAddr->m_PUSer);
#endif

    ucTLun = L2_GetPuFromAddr(pDesAddr);
    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_INTRNAL;

    ptReqEntry->tFlashDesc.bsVirBlk = pDesAddr->m_BlockInPU;
    ptReqEntry->tFlashDesc.bsVirPage = pDesAddr->m_PageInBlock;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;

    for (ucIndex = 0; ucIndex < SLC_PG_PER_TLC; ucIndex++)
    {
        ptReqEntry->atFlashAddr[ucIndex].bsVirBlk = pSrcAddr[ucIndex].m_BlockInPU;
        ptReqEntry->atFlashAddr[ucIndex].bsVirPage = pSrcAddr[ucIndex].m_PageInBlock;
    }
    
    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
    
    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

/*****************************************************************************
 Prototype      : L2_FtlReadMerge
 Description    : L1 buffer merge read
 Input          : BUF_REQ* pReq
 PhysicalAddr* pAddr
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/9/2
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_FtlReadMerge(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, BUF_REQ_READ* pReq, PhysicalAddr* pAddr)
{
    U8 ucTLun;
    FCMD_REQ_ENTRY *ptReqEntry;

    if (INVALID_8F == pAddr[pReq->ucLPNOffset].m_PPN)
    {
        L2_FtlHandleReadWithoutWrite(ucSuperPu, pReq, NULL, pReq->ucLPNOffset, (U8)pReq->ulLPNBitMap, 1, pReq->bFirstRCMDEn);
        //L2_ClearBufReqLpnBitMap(ucSuperPu, 0, pReq->ucLPNOffset);
    }
    else
    {
        ucTLun = L2_GetPuFromAddr(&pAddr[pReq->ucLPNOffset]);

        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            //DBG_Printf("[L2_FtlReadMerge]PU:%d L2_FCMDQNotFull==FALSE.\n",ucPu);
            //DBG_Getch();        
            return FALSE;
        }

        //FIRMWARE_LogInfo("MergeRead SuperPu %d LPN %d  PhyAddr 0x%x (BlkAddr %d PageAddr %d)\n", ucSuperPu, pReq->aLPN[pReq->ucLPNOffset], pAddr[pReq->ucLPNOffset].m_PPN, pAddr[pReq->ucLPNOffset].m_BlockInPU, pAddr[pReq->ucLPNOffset].m_PageInBlock);

        // allocate a fcmd entry and construct it.
        ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
        ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
        ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;

        ptReqEntry->tFlashDesc.bsVirBlk = pAddr[pReq->ucLPNOffset].m_BlockInPU;
        ptReqEntry->tFlashDesc.bsVirPage = pAddr[pReq->ucLPNOffset].m_PageInBlock;
        if (TRUE == L2_IsSLCBlock(pAddr[pReq->ucLPNOffset].m_PUSer, pAddr[pReq->ucLPNOffset].m_BlockInPU))
        {
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
        }
        else
        {
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
        }
        ptReqEntry->tFlashDesc.bsSecStart = (pAddr[pReq->ucLPNOffset].m_LPNInPage << SEC_PER_LPN_BITS) % SEC_PER_LOGIC_PG;
        ptReqEntry->tFlashDesc.bsPlnNum = (pAddr[pReq->ucLPNOffset].m_LPNInPage << SEC_PER_LPN_BITS) / SEC_PER_LOGIC_PG;
        ptReqEntry->tFlashDesc.bsMergeRdEn = TRUE;
        ptReqEntry->tFlashDesc.bsLpnBitmap = (U8)pReq->ulLPNBitMap;
        //ptReqEntry->tFlashDesc.bsShiftRdEn = (TRUE == L2_IsFlashPageSafed(&pAddr[pReq->ucLPNOffset])) ? FALSE : TRUE;

        ptReqEntry->atBufDesc[0].bsBufID = pReq->usPhyBufferID;
        ptReqEntry->atBufDesc[0].bsSecStart = pReq->ucLPNOffset << SEC_PER_LPN_BITS;
        ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

        ptReqEntry->tHostDesc.ulFtlLba = pReq->aLPN[pReq->ucLPNOffset] << SEC_PER_LPN_BITS;

        ptReqEntry->ulReqStsAddr = pReq->ulReqStatusAddr;
        ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

        L2_FCMDQAdaptPhyBlk(ptReqEntry);
        L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

        //L2_ClearBufReqLpnBitMap(ucSuperPu, 0, pReq->ucLPNOffset);
#ifdef READ_DISTURB_OPEN
        /* update ReadCnt */
        if (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
        {
            L2_UpdateBlkReadCnt(pAddr[pReq->ucLPNOffset].m_PUSer, pAddr[pReq->ucLPNOffset].m_OffsetInSuperPage, pAddr[pReq->ucLPNOffset].m_BlockInPU);
        }
#endif
    }

    return TRUE;
}

/*****************************************************************************
 Prototype      : L2_FtlHostRead
 Description    : host read and prefetch read
 Input          : BUF_REQ* pReq
 PhysicalAddr* pAddr
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/9/2
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_FtlReadNormal(U8 ucSuperPu, U8 ucLunInSPU, U32 ulLunAllowToSendFcmdBitmap, BUF_REQ_READ* pReq, PhysicalAddr* pAddr)
{
    U8 LPNOffset = 0;
    //U8 ReqRemain = 0;
    U8 OrigPos = 0,Pos = 0, ucStartPos;
    U8 i = 0;
    U8 ucReqOffset, ucReqLenth, ucReqLPNCnt;
    FCMD_REQ_ENTRY *ptReqEntry;
    U8 ucLunInSuperPu;
    U8 ucTLun;
    BUFREQ_HOST_INFO* pBufReqHostInfo;
    BOOL bFirstRCMDEn = FALSE;
    U32 ulDebugLBA;

    //check whether LPN exists in one physical page
    LPNOffset = g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_LPNOffset;
    //ReqRemain = g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqRemain;    

    // Configure active LPN, that is LPNoffeset +(0, LPNCount)
    for (Pos = LPNOffset; Pos < (pReq->ucLPNOffset + pReq->ucLPNCount); Pos++)
    {
        pBufReqHostInfo = &g_FTLReadDptr[ucSuperPu][ucLunInSPU].BufReqHostInfo[Pos];

#ifdef HOST_NVME
        /* i. handle host read without write - Nvme */
        if (TRUE == l_RdWithoutWtDptr[ucSuperPu].Initialized || INVALID_8F == pAddr[Pos].m_PPN)
        {        
            //U8 ucCmdTag;
            ucReqLenth = 0;
            OrigPos = 0;
            ucReqLPNCnt = 0;            
            
            if(FALSE == l_RdWithoutWtDptr[ucSuperPu].Initialized)
            {
                if (0 == (L2_GetBufReqLpnBitMap(ucSuperPu, ucLunInSPU) & (1 << Pos)))
                {
                    continue;
                }                
            
                OrigPos = Pos;
                
                if (Pos == pReq->ucLPNOffset)
                {
                    bFirstRCMDEn = pReq->bFirstRCMDEn;
                }
                else
                {
                    bFirstRCMDEn = FALSE;
                }

                ucReqLenth += g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[Pos];
                ucReqLPNCnt++;
                L2_ClearBufReqLpnBitMap(ucSuperPu, ucLunInSPU, Pos);                
                //ucReqLPNCnt++;
                //ucCmdTag = g_FTLReadDptr[ucSuperPu][ucLunInSPU].BufReqHostInfo.HmemDptr.bsCmdTag;


                /* + merge the sequential LPN read without write to one request + */
                for (i = Pos + 1; i < (pReq->ucLPNOffset + pReq->ucLPNCount); i++)
                {                
                    if ((L2_GetBufReqLpnBitMap(ucSuperPu, ucLunInSPU) & (1 << i)) != 0 && INVALID_8F == pAddr[i].m_PPN)
                    {
                        //if(ucCmdTag == g_FTLReadDptr[ucSuperPu][ucLunInSPU].BufReqHostInfo.HmemDptr.bsCmdTag)
                        {
                            ucReqLenth += g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[i];

                            L2_ClearBufReqLpnBitMap(ucSuperPu, ucLunInSPU, i);

                            Pos++;
                            ucReqLPNCnt++;
                        }
                    }
                    else
                    {
                        break;
                    }
                }                
            }
            else
            {
                //DBG_Printf("RdWithoutWt already initialize case\n");
            }            

            //DBG_Printf("cmd 0x%x LUN 0x%x rdwithoutwr request 0x%x sectors startLBA 0x%x\n", g_FTLReadDptr[ucSuperPu][ucLunInSPU].BufReqHostInfo.HmemDptr.bsCmdTag,
            //    g_ulRdWithoutWrLUN, ucReqLenth, g_FTLReadDptr[ucSuperPu][ucLunInSPU].BufReqHostInfo.HmemDptr.ulLBA);
            //FIRMWARE_LogInfo("LUN 0x%x rdwithoutwr request 0x%x sectors\n", g_ulRdWithoutWrLUN, ucReqLenth);
            /* process read without write */
            if (TRUE == L2_FtlHandleReadWithoutWrite(ucSuperPu, pReq, pBufReqHostInfo, OrigPos, ucReqLenth, ucReqLPNCnt, bFirstRCMDEn))
            {
                //ReqRemain -= ucReqLenth;
                //L2_ClearBufReqLpnBitMap(ucSuperPu, ucLunInSPU, Pos);
                //g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqRemain = ReqRemain;

                //FIRMWARE_LogInfo("ReadWithoutWrite Pu %d LPN 0x%x  (Pos %d) ReqLength %d ReqRemain %d HmemDptr Tag 0x%x Lba 0x%x PrdIndex 0x%x bsOffset 0x%x\n",
                //    ucSuperPu, pReq->LPN[Pos], Pos, ucReqLenth, ReqRemain, pBufReqHostInfo->HmemDptr.bsCmdTag,
                //   pBufReqHostInfo->HmemDptr.ulLBA, pBufReqHostInfo->HmemDptr.bsPrdOrPrpIndex, pBufReqHostInfo->HmemDptr.bsOffset);
                Pos = l_RdWithoutWtDptr[ucSuperPu].LPNNextPos;
                
                continue;
            }
            else
            {
                //g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqRemain = ReqRemain;                
                return FALSE;
            }
        }        
#endif

        if (0 == (L2_GetBufReqLpnBitMap(ucSuperPu, ucLunInSPU) & (1 << Pos)))
        {
            continue;
        }        

#ifdef HOST_SATA
        /* i. handle host read without write - SATA */
        if (INVALID_8F == pAddr[Pos].m_PPN)
        {
            ucReqLenth = g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[Pos];
            ucReqLPNCnt = 1;

            if (Pos == pReq->ucLPNOffset)
            {
                bFirstRCMDEn = pReq->bFirstRCMDEn;
            }
            else
            {
                bFirstRCMDEn = FALSE;
            }

            /* + merge the sequential LPN read without write to one request + */
            for (i = Pos + 1; i < (pReq->ucLPNOffset + pReq->ucLPNCount); i++)
            {
                if ((L2_GetBufReqLpnBitMap(ucSuperPu, ucLunInSPU) & (1 << i)) != 0 && INVALID_8F == pAddr[i].m_PPN)
                {
                    ucReqLenth += g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[i];                      
                    ucReqLPNCnt++;
                }
                else
                {
                    break;
                }
            }

            //FIRMWARE_LogInfo("cmd 0x%x rdwithoutwr request lpnCnt 0x%x\n", pReq->tBufReqHostInfo.ucTag, ucReqLPNCnt);
            //DBG_Printf("pos 0x%x reqLen 0x%x reqLPNCnt 0x%x firstCmd 0x%x\n",Pos, ucReqLenth, ucReqLPNCnt, bFirstRCMDEn);
            /* process read without write */
            if (TRUE == L2_FtlHandleReadWithoutWrite(ucSuperPu, pReq, pBufReqHostInfo, Pos, ucReqLenth, ucReqLPNCnt, bFirstRCMDEn))
            {
                //ReqRemain -= ucReqLenth;
                for (i = 0; i < ucReqLPNCnt; i++)
                {
                    L2_ClearBufReqLpnBitMap(ucSuperPu, ucLunInSPU, Pos + i);
                }
                Pos += ucReqLPNCnt - 1;
                //g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqRemain = ReqRemain;

                //FIRMWARE_LogInfo("ReadWithoutWrite Pu %d LPN 0x%x  (Pos %d) ReqLength %d ReqRemain %d HmemDptr Tag 0x%x Lba 0x%x PrdIndex 0x%x bsOffset 0x%x\n",
                //    ucSuperPu, pReq->LPN[Pos], Pos, ucReqLenth, ReqRemain, pBufReqHostInfo->HmemDptr.bsCmdTag,
                //   pBufReqHostInfo->HmemDptr.ulLBA, pBufReqHostInfo->HmemDptr.bsPrdOrPrpIndex, pBufReqHostInfo->HmemDptr.bsOffset);

                continue;
            }
            else
            {
                //g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqRemain = ReqRemain;
                return FALSE;
            }
        }
#endif

        /* Need send Fcmd to L3*/
        ucLunInSuperPu = L2_GetBufReqLunInSuperPu(ucSuperPu, ucLunInSPU, Pos);

        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
        ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
        ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;

        #ifdef HOST_SATA
        ptReqEntry->tHostDesc.tHostDptr.bsPrdID = pReq->tBufReqHostInfo.ucDSGId;
        ptReqEntry->tHostDesc.tHostDptr.bsCmdTag = pReq->tBufReqHostInfo.ucTag;
        #endif

        if (Pos == pReq->ucLPNOffset)
        {
            #ifdef HOST_SATA
            ptReqEntry->tHostDesc.tHostDptr.bsFstCmdEn = pReq->bFirstRCMDEn;
            #endif

            ucReqOffset = pReq->ucReqOffset;
        }
        else
        {
            ucReqOffset = Pos * SEC_PER_LPN;
        }
        
        ucReqLenth = g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[Pos];
        //ReqRemain -= ucReqLenth;
        ucStartPos = Pos;        

        /* merge the sequential LPN to one flash request */
        for (i = (Pos + 1); i < (pReq->ucLPNOffset + pReq->ucLPNCount); i++)
        {
            if ((pAddr[i].m_PUSer == pAddr[i - 1].m_PUSer) &&
                (pAddr[i].m_BlockInPU == pAddr[i - 1].m_BlockInPU) &&
                (pAddr[i].m_PageInBlock == pAddr[i - 1].m_PageInBlock) &&
                (pAddr[i].m_OffsetInSuperPage == pAddr[i - 1].m_OffsetInSuperPage) &&
                (pAddr[i].m_LPNInPage == pAddr[i - 1].m_LPNInPage + 1))
            {
                ucReqLenth += g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[i];
                //ReqRemain -= g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqLength[i];

                L2_ClearBufReqLpnBitMap(ucSuperPu, ucLunInSPU, i);

                Pos++;

                if (TRUE == pReq->bReadPreFetch)
                {
                    L1_L2SubPrefetchCacheStatus(pReq->usPhyBufferID);
                }
            }
            else
            {
                break;
            }
        }   

        ptReqEntry->tFlashDesc.bsVirBlk = pAddr[ucStartPos].m_BlockInPU;
        ptReqEntry->tFlashDesc.bsVirPage = pAddr[ucStartPos].m_PageInBlock;        
        if (TRUE == L2_VBT_Get_TLC(pAddr[ucStartPos].m_PUSer, pAddr[ucStartPos].m_BlockInPU))
        {
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
        }
        else
        {
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
        }
        ptReqEntry->tFlashDesc.bsSecStart = (pAddr[ucStartPos].m_LPNInPage << SEC_PER_LPN_BITS) + (ucReqOffset & SEC_PER_LPN_MSK);
        ptReqEntry->tFlashDesc.bsSecLen = ucReqLenth;

        if (FALSE == pReq->bSeq && FALSE == L2_IsMultiPlnRdProcess(ptReqEntry->tFlashDesc.bsSecStart, ptReqEntry->tFlashDesc.bsSecLen) && 0 != ptReqEntry->tFlashDesc.bsSecLen)
        {
            ptReqEntry->tFlashDesc.bsPlnNum = ptReqEntry->tFlashDesc.bsSecStart / SEC_PER_LOGIC_PG;
            ptReqEntry->tFlashDesc.bsSecStart = ptReqEntry->tFlashDesc.bsSecStart % SEC_PER_LOGIC_PG;          
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
        }
        
        //ptReqEntry->tFlashDesc.bsShiftRdEn = (TRUE == L2_IsFlashPageSafed(&pAddr[pReq->ucLPNOffset])) ? FALSE : TRUE;

        ptReqEntry->atBufDesc[0].bsBufID = pReq->usPhyBufferID;
        ptReqEntry->atBufDesc[0].bsSecStart = ucReqOffset;
        ptReqEntry->atBufDesc[0].bsSecLen = ucReqLenth;
        ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

        ulDebugLBA = (pReq->aLPN[ucStartPos] << SEC_PER_LPN_BITS) + (ucReqOffset & SEC_PER_LPN_MSK);

        //read prefetch need cache status, and no buffmap needed
        if (TRUE == pReq->bReadPreFetch)
        {
            ptReqEntry->ulReqStsAddr = L1_L2GetPrefetchCacheStatusAddr(pReq->usPhyBufferID, ucReqOffset, ucReqLenth);
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;
        }
        else
        {
            ptReqEntry->tFlashDesc.bsHostRdEn = TRUE;

            #ifdef HOST_NVME       
            COM_MemCpy((U32*)&ptReqEntry->tHostDesc.tHostDptr, (U32*)&pBufReqHostInfo->HmemDptr, sizeof(HMEM_DPTR) / sizeof(U32)); 
            #endif
        }

        ptReqEntry->tHostDesc.ulFtlLba = ulDebugLBA;

        L2_FCMDQAdaptPhyBlk(ptReqEntry);
        L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

#ifdef READ_DISTURB_OPEN
        /* update ReadCnt */
        if (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
        {
            L2_UpdateBlkReadCnt(pAddr[ucStartPos].m_PUSer, pAddr[ucStartPos].m_OffsetInSuperPage, pAddr[ucStartPos].m_BlockInPU);
        }
#endif

        //g_FTLReadDptr[ucSuperPu][ucLunInSPU].m_ReqRemain = ReqRemain;

        L2_ClearBufReqLpnBitMap(ucSuperPu, ucLunInSPU, ucStartPos);
        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
    }//end for LPNCount 

    /* Whether BufReq LPN is all send to L3 */
#ifdef HOST_NVME
    if (0 != L2_GetBufReqLpnBitMap(ucSuperPu, ucLunInSPU) || TRUE == l_RdWithoutWtDptr[ucSuperPu].Initialized)
#else //SATA
    if (0 != L2_GetBufReqLpnBitMap(ucSuperPu, ucLunInSPU))
#endif
    {
        return FALSE;
    }

    return TRUE;
}
/*****************************************************************************
 Prototype      :   __L2_FtlReadLocal
 Description    :   Read "ReadLPNCnt" logical pages to the "LPNOffset" position of
 pPBuffer[BufferPointer] address. If pPBuffer[BufferPointer] is
 full, then read them into the next circular buffer , and the
 pointer of next circular buffer is ( 0x01 - BufferPointer ),
 because there are just two physical pages in the circular buffer.
 Input          :   U32* pBBuffer
 U8 BufferPointer
 PhysicalAddr* pAddr
 U32* pStatus
 U32* pSpare
 U32 ReadLPNCnt
 U32 LPNOffset
 Output         :   None
 Return Value   :   void
 Calls          :
 Called By      :

 History        :
 1.2014/09/03 henryluo  Created L2_FtlReadLocal() function
 2.2015/02/02 javenliu  Move it here from L2_FtlReadLocal() function
 Add BufferPointer parameter to support double circular buffer.
 If you just use one buffer, then set BufferPointer to zero.
 Modify the first parameter to U32** pPBuffer, which means
 the pointer to the address of target buffer.

 *****************************************************************************/
void __L2_FtlReadLocal(U32* pBuffAddr, PhysicalAddr* pAddr, U8* pStatus,
    U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode, BOOL bPatrol)
{
    U32 ulBufferAddr, ulDramStartAddr;
    U16 aBuffID[DSG_BUFF_SIZE];
    U8 ucTLun;
    FCMD_REQ_ENTRY *ptReqEntry;

#ifdef SIM
    if (pAddr->m_PUSer > SUBSYSTEM_SUPERPU_NUM
     || pAddr->m_BlockInPU > (BLK_PER_PLN + RSV_BLK_PER_PLN) 
     || pAddr->m_PageInBlock > (PG_PER_SLC_BLK * PG_PER_WL))
    {
        DBG_Printf("L2_FtlReadLocal Invalid PhysicalAddr SuperPu %d VirBlk 0x%x Pg 0x%x\n", pAddr->m_PUSer, pAddr->m_BlockInPU, pAddr->m_PageInBlock);
        DBG_Getch();
    }
#endif

    /* config buffer id */
    ulBufferAddr = (0 == pBuffAddr[0]) ? g_L2TempBufferAddr : (U32)pBuffAddr[0];
    ulBufferAddr += (LPNOffset / LPN_PER_BUF) * BUF_SIZE;
    ulDramStartAddr = DRAM_START_ADDRESS;
#ifndef SIM
    if(ulBufferAddr > DRAM_HIGH_START_ADDRESS)
    {
        ulDramStartAddr = DRAM_HIGH_START_ADDRESS;
    }
#endif
    // Whether it needs to be split, just valid for GC
    aBuffID[0] = (ulBufferAddr - ulDramStartAddr) / BUF_SIZE;
    if (LPNOffset + ReadLPNCnt > LPN_PER_SUPERBUF)
    {
        // Set the next buffer to be used, just valid for the circular buffer of two physical pages.
        ulBufferAddr = (U32)pBuffAddr[1];
        aBuffID[1] = (ulBufferAddr - ulDramStartAddr) / BUF_SIZE;
    }

    ucTLun = L2_GetPuFromAddr(pAddr);
    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsTableReq = bTableReq;
    
    ptReqEntry->tFlashDesc.bsVirBlk = pAddr->m_BlockInPU;
    ptReqEntry->tFlashDesc.bsVirPage = pAddr->m_PageInBlock;
    ptReqEntry->tFlashDesc.bsBlkMod = (TRUE == bSLCMode) ? FCMD_REQ_SLC_BLK : FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = pAddr->m_LPNInPage * SEC_PER_LPN;
    ptReqEntry->tFlashDesc.bsSecLen = ReadLPNCnt * SEC_PER_LPN;
    //ptReqEntry->tFlashDesc.bsShiftRdEn = (TRUE == bSLCMode || TRUE == L2_IsFlashPageSafed(pAddr)) ? FALSE : TRUE;

    ptReqEntry->tLocalDesc.bsPatrolRdCmd = bPatrol;

    if (FALSE == L2_IsMultiPlnRdProcess(ptReqEntry->tFlashDesc.bsSecStart, ptReqEntry->tFlashDesc.bsSecLen) && 0 != ptReqEntry->tFlashDesc.bsSecLen)
    {
        ptReqEntry->tFlashDesc.bsPlnNum = ptReqEntry->tFlashDesc.bsSecStart / SEC_PER_LOGIC_PG;
        ptReqEntry->tFlashDesc.bsSecStart = ptReqEntry->tFlashDesc.bsSecStart % SEC_PER_LOGIC_PG;        
        ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
    }  

    // Whether it needs to be split, just valid for GC
    if ((LPNOffset + ReadLPNCnt) > LPN_PER_SUPERBUF)
    {
        ptReqEntry->atBufDesc[0].bsBufID = aBuffID[0];
        ptReqEntry->atBufDesc[0].bsSecStart = (LPNOffset % LPN_PER_BUF) * SEC_PER_LPN;
        ptReqEntry->atBufDesc[0].bsSecLen = (LPN_PER_BUF - (LPNOffset % LPN_PER_BUF)) * SEC_PER_LPN;
        ptReqEntry->atBufDesc[1].bsBufID = aBuffID[1];
        ptReqEntry->atBufDesc[1].bsSecStart = 0;
        ptReqEntry->atBufDesc[1].bsSecLen = ((LPNOffset % LPN_PER_BUF) + ReadLPNCnt - LPN_PER_BUF) * SEC_PER_LPN;
        ptReqEntry->atBufDesc[2].bsBufID = INVALID_4F;
    }
    else
    {
        ptReqEntry->atBufDesc[0].bsBufID = aBuffID[0];
        ptReqEntry->atBufDesc[0].bsSecStart = (LPNOffset % LPN_PER_BUF) * SEC_PER_LPN;
        ptReqEntry->atBufDesc[0].bsSecLen = ReadLPNCnt * SEC_PER_LPN;
        ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;
    }

    if (NULL != pSpare)
    {
        ptReqEntry->ulSpareAddr = (U32)pSpare;

        if (0 == ReadLPNCnt)
        {
            ptReqEntry->tFlashDesc.bsRdRedOnly = TRUE;
        }
    }

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
    
#ifdef READ_DISTURB_OPEN
    /* update ReadCnt: TLCArea has no table */
    if (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
    {
        L2_UpdateBlkReadCnt(pAddr->m_PUSer, pAddr->m_OffsetInSuperPage, pAddr->m_BlockInPU);
    }
#endif
    
    return;
}

/*****************************************************************************
 Prototype      : L2_FtlReadLocal
 Description    :
 Input          : U32* pBuffer
 PhysicalAddr* pAddr
 U32* pStatus
 U32* pSpare
 U32 ReadLPNCnt
 U32 LPNOffset
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.2014/09/03 henryluo  Created function

 *****************************************************************************/
void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus,
    U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode)
{
    U32 aBuffAddr[2] = { 0 };
    if (NULL != pBuffer)
    {
        aBuffAddr[0] = (U32)pBuffer;
    }

    __L2_FtlReadLocal(aBuffAddr, pAddr, pStatus, pSpare, ReadLPNCnt, LPNOffset, bTableReq, bSLCMode, FALSE);

    return;
}

/*****************************************************************************
 Prototype      : L2_FtlEraseBlock
 Description    :
 Input          : U8 PUSer
 U16 BlockInCE
 U32* pStatus
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/9/3
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_FtlEraseBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, BOOL bNeedL3ErrorHandle)
{
    U8 ucTLun;
    FCMD_REQ_ENTRY *ptReqEntry;

    ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
    
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->bsTableReq = bTableReq;

    ptReqEntry->tFlashDesc.bsVirBlk = BlockInCE;
    ptReqEntry->tFlashDesc.bsBlkMod = (TRUE == bSLCMode) ? FCMD_REQ_SLC_BLK : FCMD_REQ_TLC_BLK;
    ptReqEntry->bsNeedL3ErrHandle = bNeedL3ErrorHandle;

    if (NULL != pStatus)
    {
        *pStatus = SUBSYSTEM_STATUS_PENDING;
        ptReqEntry->ulReqStsAddr = (U32)pStatus;
        ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;
    }

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

void L2_EraseTLCBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE, U8* pStatus, BOOL bNeedL3ErrorHandle)
{
    L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, BlockInCE, pStatus, FALSE, FALSE, bNeedL3ErrorHandle);

    return;
}

/*****************************************************************************
Prototype      : L2_FTLGetCurEraseStsAddr
Description    :
Input          : U8 ucSuperPu;  U8 ucLUNInSuperPU
Output         : U8 *
Return Value   : status addr pointer
Calls          :
Called By      :

History        :
1.Date         : 2017.0508
Author       : Tobey TAN
Modification : Created function
Other: invoke it to initialize erase status Manager.
*****************************************************************************/
void L2_FTLEraseStsManagerInit(void)
{
    U8 ucSuperPUIndex;
    U8 ucLocIndex;
    U8 ucLUNInSuperPU;
    FTL_ERASE_STATUS_INFO *pFTLEraseStsInfo;

    l_ptFTLEraseStatusManager = (FTL_ERASE_STATUS_MGR *)DSRAM1_MCU12_ERASESTS_BASE;
    
    if (DSRAM1_MCU12_ERASESTS_SIZE < (sizeof(FTL_ERASE_STATUS_MGR)))
    {
        DBG_Printf("EraseStatusManager oversize\n");
        DBG_Getch();
    }

    pFTLEraseStsInfo = l_ptFTLEraseStatusManager->tEraseStsInfo;

    for (ucSuperPUIndex = 0; ucSuperPUIndex < SUBSYSTEM_SUPERPU_NUM; ucSuperPUIndex++)
    {
        pFTLEraseStsInfo[ucSuperPUIndex].ucInsertPtr = 0;
        pFTLEraseStsInfo[ucSuperPUIndex].ucRecyclePtr = 0;
        for (ucLocIndex = 0; ucLocIndex < WAIT_ERASE_STS_ARRAY_SIZE; ucLocIndex++)
        {
            pFTLEraseStsInfo[ucSuperPUIndex].ausEraseVBN[ucLocIndex] = INVALID_4F;
            for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
            {
                pFTLEraseStsInfo[ucSuperPUIndex].aucEraseStatus[ucLocIndex][ucLUNInSuperPU] = SUBSYSTEM_STATUS_INIT;
            }
        }
    }

    return;
}

/*****************************************************************************
Prototype      : L2_FTLGetCurEraseStsAddr
Description    :
Input          : U8 ucSuperPu;  U8 ucLUNInSuperPU
Output         : U8 *
Return Value   : status addr pointer
Calls          :
Called By      :

History        :
1.Date         : 2017.0508
Author       : Tobey TAN
Modification : Created function
Other: invoke it to get status addr pointer for current LUN's erase command.
*****************************************************************************/
U8 * L2_FTLGetCurEraseStsAddr(U8 ucSuperPu, U8 ucLUNInSuperPU)
{
    FTL_ERASE_STATUS_INFO * pFTLEraseStsInfo;

    pFTLEraseStsInfo = &l_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPu];
    return &pFTLEraseStsInfo->aucEraseStatus[pFTLEraseStsInfo->ucInsertPtr][ucLUNInSuperPU];
}

/*****************************************************************************
Prototype      : L2_FTLRecordEraseStsInfo
Description    :
Input          : U8 ucSuperPu;  U16 usVBN
Output         : NONE
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2017.0508
Author       : Tobey TAN
Modification : Created function
Other: invoke it to record current L2 send out command VBN in erase status queue.
*****************************************************************************/
void L2_FTLRecordEraseStsInfo(U8 ucSuperPu, U16 usVBN)
{
    FTL_ERASE_STATUS_INFO * pFTLEraseStsInfo;

    pFTLEraseStsInfo = &l_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPu];

    if ((INVALID_4F != pFTLEraseStsInfo->ausEraseVBN[pFTLEraseStsInfo->ucInsertPtr])
        || (((pFTLEraseStsInfo->ucInsertPtr + 1) % WAIT_ERASE_STS_ARRAY_SIZE) == pFTLEraseStsInfo->ucRecyclePtr))
    {
        DBG_Printf("EraseSts fifo full or inset location didn't recycle\n");
        DBG_Getch();
    }

    /*record in WaitEraseSts Que*/
    pFTLEraseStsInfo->ausEraseVBN[pFTLEraseStsInfo->ucInsertPtr] = usVBN;
    pFTLEraseStsInfo->ucInsertPtr = (pFTLEraseStsInfo->ucInsertPtr + 1) % WAIT_ERASE_STS_ARRAY_SIZE;

    /*Mark flag in VBT*/
    L2_VBTSetWaitEraseSts(ucSuperPu, usVBN, TRUE);
    
    return;
}

#ifdef SCAN_BLOCK_N1
void L2_ReadErasedBlockN0Count(U8 ucSuperPu, U16 usRecycleVBN)
{

    U8 ucTLun;
    PhysicalAddr tAddr;
    U32 pBuffer[2] = { 0 };
    U8 * pStatus;
    U16 usPage = 0, usPageCnt;
    BOOL bSLCMode;
    U8 ucLUNInSuperPU;

    DBG_Printf("N1V%d\n", usRecycleVBN);
    while (FALSE == L2_FCMDQIsAllEmpty())
        ;
    /*FCMD prpare prepare*/
    tAddr.m_PPN = 0;
    tAddr.m_PUSer = ucSuperPu;
    tAddr.m_LPNInPage = 0;
    tAddr.m_BlockInPU = usRecycleVBN;
    bSLCMode = L2_IsSLCBlock(ucSuperPu, usRecycleVBN);
    usPageCnt = ((bSLCMode == TRUE) ? PG_PER_SLC_BLK : PG_PER_TLC_BLK);
    for (usPage = 0; usPage < usPageCnt; usPage++)
    {
        if (usPage < 2200 && usPage > 110 && L2_EXTRA_PAGE != L3_GetPageTypeByProgOrder(usPage))
        {
            continue;
        }
        tAddr.m_PageInBlock = usPage;
        l_pScanN1Mgr->ucCmdSendLunBitMap = 0;
        l_pScanN1Mgr->ucPageDoneBitMap = 0;

        while (l_pScanN1Mgr->ucCmdSendLunBitMap != SUPERPU_LUN_NUM_BITMSK)
        {
            for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
            {
                ucTLun = L2_GET_TLUN(ucSuperPu, ucLUNInSuperPU);
                if (FALSE == L2_FCMDQNotFull(ucTLun) || (l_pScanN1Mgr->ucCmdSendLunBitMap & (1 << ucLUNInSuperPU)))
                {
                    continue;
                }

                tAddr.m_OffsetInSuperPage = ucLUNInSuperPU;
                pStatus = &l_pScanN1Mgr->ucFlashSts[ucLUNInSuperPU];
                __L2_FtlReadLocal(pBuffer, &tAddr, pStatus, NULL, LPN_PER_BUF, 0, FALSE, bSLCMode, TRUE);
                l_pScanN1Mgr->ucCmdSendLunBitMap |= (1 << ucLUNInSuperPU);
            }
        }

        while (l_pScanN1Mgr->ucPageDoneBitMap != SUPERPU_LUN_NUM_BITMSK)
        {
            for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
            {
                if (SUBSYSTEM_STATUS_PENDING != l_pScanN1Mgr->ucFlashSts[ucLUNInSuperPU])
                {
                    l_pScanN1Mgr->ucPageDoneBitMap |= (1 << ucLUNInSuperPU);
                }
            }
        }
    }
    DBG_Printf("N1E\n");
}
#endif

/*****************************************************************************
Prototype      : L2_FTLCheckEraseSts
Description    :
Input          : U8 ucSuperPu
Output         : NONE
Return Value   : 
Calls          :
Called By      :

History        :
1.Date         : 2017.0508
Author       : Tobey TAN
Modification : Created function
Other: invoke it to check erase command's relative status and recycle erase status queue
              at every L2 schedule cycle.
*****************************************************************************/
BOOL L2_FTLCheckEraseSts(U8 ucSuperPu)
{
    U8 ucCheckLoc;
    U8 ucLUNInSuperPU;
    U16 usRecycleVBN;
    FTL_ERASE_STATUS_INFO * pFTLEraseStsInfo;
    BOOL N1Read = FALSE;

    pFTLEraseStsInfo = &l_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPu];

    /* No EraseTask need check sts */
    if (pFTLEraseStsInfo->ucRecyclePtr == pFTLEraseStsInfo->ucInsertPtr)
    {
        return FALSE;
    }

    ucCheckLoc = pFTLEraseStsInfo->ucRecyclePtr;
    while (ucCheckLoc != pFTLEraseStsInfo->ucInsertPtr)
    {
#ifdef SIM
        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            if (SUBSYSTEM_STATUS_INIT == pFTLEraseStsInfo->aucEraseStatus[ucCheckLoc][ucLUNInSuperPU])
            {
                DBG_Getch();
            }
        }
#endif

        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            if (SUBSYSTEM_STATUS_PENDING == pFTLEraseStsInfo->aucEraseStatus[ucCheckLoc][ucLUNInSuperPU])
            {                
                return N1Read;
            }
        }

        /*all lun's current blk erase done, RecyleBlock*/
        usRecycleVBN = pFTLEraseStsInfo->ausEraseVBN[ucCheckLoc];
        if (g_EraseOpt[ucSuperPu].bEraseTLC== TRUE)
        {
            if (usRecycleVBN == g_EraseOpt[ucSuperPu].uErasingTLCVBN)
            {
                g_EraseOpt[ucSuperPu].uErasingTLCVBN = INVALID_4F;
                g_EraseOpt[ucSuperPu].bEraseTLC = FALSE;
                //DBG_Printf("TLC%d erase done\n", usRecycleVBN);
            }
        }
        L2_RecycleBlock(ucSuperPu, usRecycleVBN);
        L2_BM_CollectFreeBlock(ucSuperPu, usRecycleVBN);
        L2_PBIT_SetEvent_SavePbit(ucSuperPu, BLKTYPE_TLC);

        //FIRMWARE_LogInfo("FTLErase Pu %d VirBlk 0x%x TLCBlk %d_Used %d SLCBlk %d_Used %d\n", ucSuperPu, usRecycleVBN,
        //    g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_TLC], g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC],
        //    g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_SLC], g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_SLC]);

        /*recycle one Location+*/
        pFTLEraseStsInfo->ausEraseVBN[ucCheckLoc] = INVALID_4F;
        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            pFTLEraseStsInfo->aucEraseStatus[ucCheckLoc][ucLUNInSuperPU] = SUBSYSTEM_STATUS_INIT;
        }
        pFTLEraseStsInfo->ucRecyclePtr = (pFTLEraseStsInfo->ucRecyclePtr + 1) % WAIT_ERASE_STS_ARRAY_SIZE;
        ucCheckLoc = pFTLEraseStsInfo->ucRecyclePtr;

#ifdef SCAN_BLOCK_N1
        if (TRUE == L2_IsBootupOK() && !L2_IsSLCBlock(ucSuperPu, usRecycleVBN))
        {
            N1Read = TRUE;
            L2_ReadErasedBlockN0Count(ucSuperPu, usRecycleVBN);
        }
#endif

        /*unmark flag in VBT*/
        L2_VBTSetWaitEraseSts(ucSuperPu, usRecycleVBN, FALSE);
    }

    return N1Read;
}

/*****************************************************************************
Prototype      : L2_FTLWaitAllEraseDone
Description    :
Input          : U8 ucSuperPu
Output         : BOOL
Return Value   : TRUE: all L2 send out erase command complete
                 FALSE: not all L2 send out erase command complete
Calls          :
Called By      :

History        :
1.Date         : 2017.0508
Author       : Tobey TAN
Modification : Created function
Other: invoke it to wait erase command's relative status before normal shutdown.

*****************************************************************************/
BOOL L2_FTLWaitAllEraseDone(U8 ucSuperPu)
{
    FTL_ERASE_STATUS_INFO * pFTLEraseStsInfo;

    pFTLEraseStsInfo = &l_ptFTLEraseStatusManager->tEraseStsInfo[ucSuperPu];
    if (pFTLEraseStsInfo->ucRecyclePtr != pFTLEraseStsInfo->ucInsertPtr)
    {
        L2_FTLCheckEraseSts(ucSuperPu);
        return FALSE;
    }
    else
    {
        return TRUE;
    }   
}

/*****************************************************************************
Prototype      : L2_FTLErase
Description    :
Input          : U8 ucSuperPu, U16 usBlock
Output         : None
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2015.01.29
Author       : zoewen
Modification : Created function
Other: erase this dirty block and release it as free block and send FCMD-Erase

*****************************************************************************/
void L2_FTLErase(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U16 usVBN)
{
    U8 ucLunInSuperPu;
    U8 ucTLun;
    U8* pStatus;
    U8 bSLCMode = L2_IsSLCBlock(ucSuperPu, usVBN);

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if ((TRUE == COM_BitMaskGet(g_EraseQueue[ucSuperPu]->ulEraseBitMap, ucLunInSuperPu)) && (bSLCMode == TRUE))
        {
            continue;
        }
        else if ((TRUE == COM_BitMaskGet(g_EraseQueue[ucSuperPu]->ulTLCEraseBitMap, ucLunInSuperPu)) && (bSLCMode == FALSE))
        {
            continue;
        }

        pStatus = L2_FTLGetCurEraseStsAddr(ucSuperPu, ucLunInSuperPu);
        L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, usVBN, pStatus, FALSE, bSLCMode, TRUE);

        if (bSLCMode == TRUE)
            g_EraseQueue[ucSuperPu]->ulEraseBitMap |= (1 << ucLunInSuperPu);
        else
            g_EraseQueue[ucSuperPu]->ulTLCEraseBitMap |= (1 << ucLunInSuperPu);

        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
    }

    if ((SUPERPU_LUN_NUM_BITMSK == g_EraseQueue[ucSuperPu]->ulEraseBitMap) 
        || (SUPERPU_LUN_NUM_BITMSK == g_EraseQueue[ucSuperPu]->ulTLCEraseBitMap))
    {
        L2_FTLRecordEraseStsInfo(ucSuperPu, usVBN);
    }
    return;
}
/*****************************************************************************
 Prototype      : L2_FtlBuildIdleReq
 Description    :
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/9/3
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
#if 0
BOOL L2_FtlBuildIdleReq(U16 usIdleType)
{
    U8 ucTLun = 0; // build an Idle check cmd into PU 0
    FCMD_REQ_ENTRY *ptReqEntry;

    if (FALSE == L2_FCMDQNotFull(ucTLun))
    {
        return FALSE;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_CHK_IDLE;
    ptReqEntry->bsReqSubType = usIdleType;
    ptReqEntry->ulReqStsAddr = (U32)&g_pMCU12MiscInfo->bSubSystemIdle;

    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return TRUE;
}

/*****************************************************************************
 Prototype      : L2_FtlBuildSelfTestReq
 Description    :
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/9/3
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
BOOL L2_FtlBuildSelfTestReq(void)
{
    U8 ucTLun = 0; // build an self test cmd into PU 0
    FCMD_REQ_ENTRY *ptReqEntry;

    if (FALSE == L2_FCMDQNotFull(ucTLun))
    {
        return FALSE;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_SELF_TEST;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->ulReqStsAddr = (U32)&g_pMCU12MiscInfo->bSubSystemSelfTestDone;

    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return TRUE;
}
#endif

/****************************************************************************
Name        :L2_FTLWrite
Input       :U8 ucSuperPu
Output      :
Author      :zoewen
Date        :2015/01/22
Description : process one write command
Other       :
20150324 NinaYang modify for Superpage support
20150511 ZoeWen
for MLC mode, host write to MLC block
for TLC mode, host write to SLC block
****************************************************************************/
BOOL L2_FTLWrite(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    BUF_REQ_WRITE* pReq;
    static MCU12_VAR_ATTR BLOCK_TYPE BlockType;//actual block type for the free page.
    PhysicalAddr Addr = { 0 };
    RED Spare;
    TargetType eTargetType;
    BOOL bSLCMode;
    U8 ucLunInSuperPu = 0;
    U8 ucLun;
    U8 ucTLun;
    COMMON_EVENT L2_Event;

    for (ucLun = 0; ucLun < LUN_NUM_PER_SUPERPU; ucLun++)
    {
        if (L1_IsWriteBufReqPending(ucSuperPu) == FALSE)
        {
            return L2_IsPassFTLWQuota(ucSuperPu);
        }
        else
        {
            L2_PopWriteBufREQ(ucSuperPu);

            if (TRUE == g_L2EventStatus.m_Shutdown)  //standby immediately 
            {
                g_L2EventStatus.m_WriteAfterShutdown = TRUE;
                g_L2EventStatus.m_ShutdownDone = FALSE; //Enable L2 idle task after write
                CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);
                if (L2_Event.EventShutDown == FALSE)
                {
                    g_L2EventStatus.m_Shutdown = FALSE;
                    g_L2EventStatus.m_WriteAfterShutdown = FALSE;
                }
            }
            g_L2EventStatus.m_ForceIdle = FALSE;
        }

        pReq = g_FTLDptr[ucSuperPu].pCurBufREQ;

        if (TRUE != L2_IsBootupOK())
        {
            if (BOOT_NORMAL == g_BootManagement->m_BootType)
            {
                //If system state alread in LOADTABLE, first wait it done 
                if (TRUE == g_BootManagement->m_DoingBootStipeFlag[ucSuperPu])
                {
                    return TRUE;
                }

                /* check whether system boot up ok, that is whether all pmt table load done */
                if (TRUE == L2_IsBootStripeNeedLoad(pReq->LPN, pReq->LPNOffset, pReq->LPNCount))
                {
                    return TRUE; //FTL_STATUS_TABLE_MANAGEMENT;
                }
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
            if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
            {
                continue;
            }

            eTargetType = TARGET_HOST_WRITE_NORAML;
            bSLCMode = TRUE;

            /* check whethre super page write done*/
            if (0 == g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[eTargetType])
            {
                /* whether write this page or not according to the state of allocate page in this Pu */
                Addr.m_PPN = L2_AllocateOnePage(ucSuperPu, pReq->LPN, eTargetType, &BlockType);
                if (Addr.m_PPN == INVALID_8F)
                {
                    TRACE_LOG((void*)&ucSuperPu, sizeof(U8), U8, 0, "[L2_FTLEntry]: No free page!!! ");
                    return TRUE;
                }
            }
            else
            {
                if (TRUE == COM_BitMaskGet(g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[eTargetType], ucLunInSuperPu))
                {
                    continue;
                }
                Addr.m_PPN = 0;
                Addr.m_PUSer = ucSuperPu;
                Addr.m_BlockInPU = g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                Addr.m_PageInBlock = g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1;

                /* Normalshutdown BlockType will be init again */
                BlockType = BLOCK_TYPE_SEQ;
            }

            Addr.m_OffsetInSuperPage = ucLunInSuperPu;

            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "L2 FTL UpdateTablesBeforeWrite start");

            //LUN offset TS
            g_PuInfo[ucSuperPu]->m_TargetOffsetTS[eTargetType][ucLunInSuperPu] = L2_GetTargetOffsetTS(&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[eTargetType]);

            FillSpareAreaInWrite(&Spare, pReq->LPN, ucSuperPu, ucLunInSuperPu, BlockType, eTargetType);
            Spare.m_RedComm.bsVirBlockAddr = Addr.m_BlockInPU;
            Spare.m_RedComm.eOPType = OP_TYPE_HOST_WRITE;

           //FIRMWARE_LogInfo("FTLWrite SuperPu %d LUN %d Blk %d Pg %d BlockType %d PhyBufId 0x%x SuperPageTs %d LunOrder %d\n", 
           //    ucSuperPu, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, Addr.m_PageInBlock, BlockType, pReq->PhyBufferID, Spare.m_RedComm.ulTimeStamp, Spare.m_RedComm.ulTargetOffsetTS);
        
            /* not to update table when response host write in abnormal boot up */
            if (TRUE == L2_IsAdjustDirtyCntDone())
            {
                UpdateTablesBeforeWrite(pReq, &Addr);
            }
#ifdef DBG_PMT
            else
            {
                U32 i;

                /* add by henryluo 2012-09-10 for L2 PMT debug */
                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    Addr.m_LPNInPage = i;
                    if (INVALID_8F != pReq->LPN[i])
                    {
                        L2_UpdateDebugPMT(&Addr, pReq->LPN[i]);

                        //FIRMWARE_LogInfo("MCU#%d LPN 0x%x -> Addr 0x%x (%d_0x%x_0x%x_%d_%d) m_TargetOffsetTS 0x%x\n", HAL_GetMcuId(), pReq->LPN[i], Addr.m_PPN,Addr.m_PUSer, Addr.m_BlockInPU, 
                        //    Addr.m_PageInBlock, Addr.m_LPNInPage, Addr.m_OffsetInSuperPage, g_PuInfo[ucSuperPu]->m_TargetOffsetTS[eTargetType][ucLunInSuperPu]);
                    }
                }
            }
#endif

            TL_PERFORMANCE(PTL_LEVEL_DETAIL, "L2 FTL UpdateTablesBeforeWrite end  ");

            L2_UpdateRPMT(ucSuperPu, ucLunInSuperPu, pReq->LPN, eTargetType, &Addr);

            /* update PBIT 1st page Info */
            if (0 == Addr.m_PageInBlock)
            {
                L2_Set_DataBlock_PBIT_Info(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, &Spare);
            }

#ifdef DATA_MONITOR_ENABLE
            FW_DataMonitorCheckWriteData(&pReq->ucWriteData[0],
            COM_GetMemAddrByBufferID(pReq->PhyBufferID, TRUE, BUF_SIZE_BITS));
#endif

            L2_FtlWriteNormal(pReq, &Addr, (U32*)&Spare, bSLCMode);

            COM_BitMaskSet((U32*)&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[eTargetType], ucLunInSuperPu);

#if defined(SWL_EVALUATOR)
            SWLRecordIncHostWCnt_L2(ucSuperPu, 1);

            if (((g_SWLRecord[0]->ulhostWCnt_L2 % 5000000) == 0) && (ucSuperPu == 0))
            {
                DBG_DumpSTD();
                DBG_DumpPBITEraseCnt();
            }

            if ((g_SWLRecord[0]->ulhostWCnt_L2 % 163840) == 0)
            {                
                DBG_DumpSWLRecordInfo();
                DBG_DumpWholeChipWAF();
            }
#endif

#ifdef L2MEASURE
            L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_HOST_SLC);
#endif

#ifndef LCT_TRIM_REMOVED
            /* the last defense mechanism of trim */
            if (g_ulPendingTrimLctCount != 0 && L2_IsNeedTrimLastDefenseMechanism(ucSuperPu) == TRUE)
            {
                L1_ProcessPendingTrimBulk(L1_TRIM_WRITE_MAX_LCT_CNT, L1_TRIM_FLUSH_MAX_LOOP_CNT);
            }
#endif

            g_FTLDptr[ucSuperPu].pCurBufREQ = NULL;

            if (SUPERPU_LUN_NUM_BITMSK == g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[eTargetType])
            {
            #ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
                L2_IncTimeStampInPu(ucSuperPu);
            #endif
                g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[eTargetType] = 0;
                return TRUE;
            }
            ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
            break;
        }
    }

    return FALSE;
}
/****************************************************************************
Name        :L2_FTLREAD
Input       :U8 ucSuperPu
Output      :
Author      :zoewen
Date        :2015/01/22
Description : process one read command
Other       :
20150324 NinaYang modify for Superpage support
****************************************************************************/
L2_FTL_STATUS L2_FTLRead(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U8 ucBufReqID, ucLunInSuperPU;
    BUF_REQ_READ* pReq;

#ifdef SIM
    if (ucSuperPu > SUBSYSTEM_SUPERPU_NUM)
    {
        DBG_Printf("SuperPu %d ulLunAllowToSendFcmdBitmap 0x%x Error!\n", ucSuperPu, ulLunAllowToSendFcmdBitmap);
        DBG_Getch();
    }
#endif

    if (L1_IsReadBufReqPending(ucSuperPu) == FALSE)
    {
        return FTL_STATUS_NEED_SCHEDULER;
    }
    else
    {
        if (FALSE == L2_PopReadBufREQ(ucSuperPu, ulLunAllowToSendFcmdBitmap))
        {
            return FTL_STATUS_NEED_SCHEDULER;
        }
    }

    if (0 == g_ulPopLunBitMap[ucSuperPu])
    {
        ucBufReqID = g_FTLReadDptr[ucSuperPu][0].m_BufReqID;

        pReq = L1_GetReadBufReq(ucSuperPu, ucBufReqID);
        if (TRUE != L2_IsBootupOK())
        {
            if (BOOT_NORMAL == g_BootManagement->m_BootType)
            {
                //If system state alread in LOADTABLE, first wait it done 
                if (TRUE == g_BootManagement->m_DoingBootStipeFlag[ucSuperPu])
                {
                    return FTL_STATUS_NEED_SCHEDULER;
                }

                /* check whether system boot up ok, that is whether all pmt table load done */
                if (TRUE == L2_IsBootStripeNeedLoad(pReq->aLPN, pReq->ucLPNOffset, pReq->ucLPNCount))
                {
                    return FTL_STATUS_NEED_SCHEDULER;
                }
            }
        }

        if (FALSE == L2_FtlReadProcess(ucSuperPu, 0, ulLunAllowToSendFcmdBitmap, pReq))
        {
            return FTL_STATUS_CMD_PENDING;
        }
        else
        {                    
            g_FTLReadDptr[ucSuperPu][0].m_BufReqID = INVALID_2F;            

#if (!defined(HAL_UNIT_TEST) && !defined(L3_UNIT_TEST))
            L1_RecycleReadBufReq(ucSuperPu, ucBufReqID);
#endif
        }
    }
    else
    {
        for (ucLunInSuperPU = 0; ucLunInSuperPU < LUN_NUM_PER_SUPERPU; ucLunInSuperPU++)
        {
            if (g_ulPopLunBitMap[ucSuperPu] & (1 << ucLunInSuperPU))
            {
                ucBufReqID = g_FTLReadDptr[ucSuperPu][ucLunInSuperPU].m_BufReqID;
                pReq = L1_GetReadBufReq(ucSuperPu, ucBufReqID);

                if (FALSE == L2_FtlReadProcess(ucSuperPu, ucLunInSuperPU, ulLunAllowToSendFcmdBitmap, pReq))
                {
                    return FTL_STATUS_CMD_PENDING;
                }
                else
                {
                    g_FTLReadDptr[ucSuperPu][ucLunInSuperPU].m_BufReqID = INVALID_2F;                    

#if (!defined(HAL_UNIT_TEST) && !defined(L3_UNIT_TEST))
                    L1_RecycleReadBufReq(ucSuperPu, ucBufReqID);
#endif
                    g_ulPopLunBitMap[ucSuperPu] &= ~(1 << ucLunInSuperPU);
                }
            }
        }
    }

    return FTL_STATUS_OK;
}


/****************************************************************************
Name        :L2_FTLEntry
Input       :U8 ucSuperPu
Output      :
Author      :zoewen
Date        :2015/03/26
Description : L2 FTL service routine
Other       :
****************************************************************************/
BOOL L2_FTLEntry(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    FTL_THREAD_TYPE eThread;
    BOOL ucIncUsedQuota;

    /* FTL thread decision */
    eThread = L2_FTLThreadDecision(ucSuperPu);

    switch (eThread)
    {
    case FTL_THREAD_WRITE:
        ucIncUsedQuota = L2_FTLWrite(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        break;

    case FTL_THREAD_SLCGC:
        ucIncUsedQuota = L2_ProcessGC(ucSuperPu, ulLunAllowToSendFcmdBitmap, FTL_THREAD_SLCGC);
        break;

    case FTL_THREAD_GC_2TLC:
        ucIncUsedQuota = L2_TLCWrite(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        break;
    case FTL_THREAD_TLCGC:
        ucIncUsedQuota = L2_ProcessTLCGC(ucSuperPu, ulLunAllowToSendFcmdBitmap, FTL_THREAD_TLCGC, TLC_WRITE_TLCGC);
        break;
    case FTL_THREAD_WL:
        ucIncUsedQuota = L2_StaticWLEntry(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        break;

    default:
        ucIncUsedQuota = FALSE;
        break;
    }

    if (TRUE == ucIncUsedQuota)
    {
        L2_FTLIncUsedThreadQuota(ucSuperPu, 1);
        if (TRUE == L2_IsPassQuota(ucSuperPu))
        {
            ucIncUsedQuota = FALSE;
            L2_SetPassQuota(ucSuperPu, FALSE);
        }

        /* Increase IDLE GC Quota cnt if need */
        if ((FALSE != g_tFTLIdleGCMgr.bIdleGC)
            && (FTL_THREAD_WRITE != eThread))
        {
            L2_FTLIncIdleGCQuotaCnt(ucSuperPu, eThread, 1);
        }
    }

    /* only SLC Write call L2_IncDeviceWrite, tune flushPMTratio for table rebuild */
    if (FTL_THREAD_WRITE == eThread || FTL_THREAD_SLCGC == eThread)
    {
        return ucIncUsedQuota;
    }
    else if (FTL_THREAD_NO_TLC_FREEBLK == eThread)
    {
        if (g_EraseOpt[ucSuperPu].bEraseTLC == TRUE)
        {
            //there has TLC block erased, wait erase done
            return FALSE;
        }
        else
        {
            // there is no TLC free block and no TLC block is erasing, force to flush PMT
            L2_ForceFlushPMTSetting(ucSuperPu);
            return FALSE;
        }
    }
    else
    {
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        if(g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt > 0)
           return ucIncUsedQuota;
        else
#endif
        return FALSE;
    }
}

void L2_FTLIncIdleGCQuotaCnt(U8 ucSuperPu, FTL_THREAD_TYPE tThread, U16 usVal)
{
    //FIRMWARE_LogInfo("L2_FTLIncIdleGCQuotaCnt SLC_TLC %d_%d \n", g_tFTLIdleGCMgr.usTLCAreaQuotaCnt[ucSuperPu], 
    //    g_tFTLIdleGCMgr.usSLCAreaQuotaCnt[ucSuperPu]);

    if (tThread >= FTL_THREAD_TLCGC)
    {
        g_tFTLIdleGCMgr.usTLCAreaQuotaCnt[ucSuperPu] += usVal;
    }
    else
    {
        g_tFTLIdleGCMgr.usSLCAreaQuotaCnt[ucSuperPu] += usVal;
    }    

#ifdef IDLE_HANDSHAKE_OLD
    g_tFTLIdleGCMgr.bClearEvent = TRUE;
#endif
    return;
}

BOOL L2_FTLIsIdleTaskDone(void)
{
    U8 ucSuperPu;
    U16 usSLCAreaQuotaCnt;
    U16 usTLCAreaQuotaCnt;
    BOOL bRet = TRUE;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        /* all task done */
        if (0 == l_FTLScheduler[ucSuperPu].ucCurSchTaskBitFlag)
        {
            continue;
        }

        usSLCAreaQuotaCnt = g_tFTLIdleGCMgr.usSLCAreaQuotaCnt[ucSuperPu];
        usTLCAreaQuotaCnt = g_tFTLIdleGCMgr.usTLCAreaQuotaCnt[ucSuperPu];

        /* Quota threshold meet */
        if (((usSLCAreaQuotaCnt < IDLE_GC_SLC_QUOTA_MAX) && (L2_FTLIsHaveSLCAreaTask(ucSuperPu)))
            || ((usTLCAreaQuotaCnt < IDLE_GC_TLC_QUOTA_MAX) && (L2_FTLIsHaveTLCAreaTask(ucSuperPu))))
        {
            bRet = FALSE;
            break;
        }
    }
    
    return bRet;
}

void L2_IdleGCResetManager(void)
{
    U8 ucSuperPU;

    g_tFTLIdleGCMgr.bIdleGC = FALSE;
#ifdef IDLE_HANDSHAKE_OLD
    g_tFTLIdleGCMgr.bClearEvent = FALSE;
#endif
    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        //FIRMWARE_LogInfo("L2 Idle usSLCAreaQuotaCnt %d usTLCAreaQuotaCnt %d \n",
        //    g_tFTLIdleGCMgr.usSLCAreaQuotaCnt[ucSuperPU], g_tFTLIdleGCMgr.usTLCAreaQuotaCnt[ucSuperPU]);
        g_tFTLIdleGCMgr.usSLCAreaQuotaCnt[ucSuperPU] = 0;
        g_tFTLIdleGCMgr.usTLCAreaQuotaCnt[ucSuperPU] = 0;
    }

    l_ucIdleGCStatus = IDLE_GC_PREPARE;

    return;
}

BOOL L2_IdleGCEntry(void)
{
    BOOL bRet = FALSE;

    switch (l_ucIdleGCStatus)
    {
        case IDLE_GC_PREPARE :
        {
            ASSERT(FALSE == g_tFTLIdleGCMgr.bIdleGC);

            g_tFTLIdleGCMgr.bIdleGC = TRUE;
            l_ucIdleGCStatus = IDLE_GC_CHECK;

            //FIRMWARE_LogInfo("000_0 ForceIdle %d WaitIdle %d \n",
            //    g_L2EventStatus.m_ForceIdle, g_L2EventStatus.m_WaitIdle);
        }
        break;

        case IDLE_GC_CHECK :
        {
            ASSERT(FALSE != g_tFTLIdleGCMgr.bIdleGC);

            if (TRUE == L2_FTLIsIdleTaskDone())
            {
                l_ucIdleGCStatus = IDLE_GC_PREPARE;
          
                bRet = TRUE;
            }
        }
        break;
    }

    return bRet;
}
