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
Filename    :L2_Evaluater.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.6
Description :functions about Read and Write process
Others      :
Modify      :CloudsZhang: Update for multi thread code
****************************************************************************/

#include "L2_Defines.h"
#include "L2_FTL.h"
#include "L2_PMTPage.h"
#include "L2_Init.h"
#include "L2_StripeInfo.h"
#include "L2_Thread.h"
#include "L2_Evaluater.h"
#include "L2_PMTI.h"
#include "L2_PMTManager.h"
#include "L2_PBIT.h"
#ifdef SIM
#include "xt_library.h"
#include "L3_FlashMonitor.h"
#endif


#ifdef L2MEASURE
GLOBAL MCU12_VAR_ATTR U32 g_TraceWrite[SUBSYSTEM_PU_MAX][L2MEASURE_TYPE_ALL];
GLOBAL MCU12_VAR_ATTR MeasureBlkEC* pMeasureErase[SUBSYSTEM_SUPERPU_MAX];
#endif

#ifdef SIM
#include "stdio.h"
#include "math.h"
#include "direct.h"

extern U32 g_ulHostInfoAddr;

LOCAL MCU12_VAR_ATTR U32 l_ulDBGInit;
#if defined(SWL_EVALUATOR)

extern GLOBAL MCU12_VAR_ATTR SWLRecord *g_SWLRecord[SUBSYSTEM_SUPERPU_MAX];

void SWLRecordInit(U8 ucPu)
{
    _mkdir("WLstatistic");
    g_SWLRecord[ucPu]->ulSwlCancelTimes = 0;
    g_SWLRecord[ucPu]->ulSwlDoneTimes = 0;
    g_SWLRecord[ucPu]->ulSWLTriggerByNoraml = 0;
    g_SWLRecord[ucPu]->ulSWLTriggerByTooColdCnt = 0;
    g_SWLRecord[ucPu]->ulTLCBlkLevelWriteCnt = 0;
    g_SWLRecord[ucPu]->ulTLCGCCnt = 0;
    g_SWLRecord[ucPu]->ulSLC2SLCGCCnt = 0;
    g_SWLRecord[ucPu]->ulSWLSwapCnt = 0;
    g_SWLRecord[ucPu]->ulSWLGCCnt = 0;
    g_SWLRecord[ucPu]->ulhostWCnt_L2 = 0;
    g_SWLRecord[ucPu]->ulhostWCnt_L1 = 0;

    g_SWLRecord[ucPu]->ulSLCExtraWCnt = 0;
    g_SWLRecord[ucPu]->ulSLCExtraTableWCnt = 0;
    g_SWLRecord[ucPu]->ulSLCExtraGCWCnt = 0;

    g_SWLRecord[ucPu]->ulTLCExtraWCnt = 0;
    g_SWLRecord[ucPu]->ulSLC2TLCMergeWCnt = 0;
    g_SWLRecord[ucPu]->ulTLC2TLCGCWCnt = 0;
    g_SWLRecord[ucPu]->ulTLC2TLCSWLWCnt = 0;
    g_SWLRecord[ucPu]->ulTLCTotalSWLSrcBlkDirtyLpnCnt = 0;

    g_SWLRecord[ucPu]->ulSLC2TLCGCWCnt_L3 = 0;
    g_SWLRecord[ucPu]->ulTLC2TLCGCWCnt_L3 = 0;
    g_SWLRecord[ucPu]->ulTLC2TLCSWLWCnt_L3 = 0;

    g_SWLRecord[ucPu]->ulTotalTLCWPageCnt = 0;
}

void SWL_TotalTLCWPageCntInc(U8 ucPu, U16 ucPrgCnt)
{
    g_SWLRecord[ucPu]->ulTotalTLCWPageCnt += ucPrgCnt;
}

void SWLRecordIncSLCExtraWCnt(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSLCExtraWCnt++;
    return;
}

void SWLRecordIncSLCTableWCnt(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSLCExtraTableWCnt++;
    return;
}

void SWLRecordIncTLCExtraWCnt(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulTLCExtraWCnt++;
    return;
}

void SWLRecordIncSLC2TLCMergeWCnt(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSLC2TLCMergeWCnt++;
    return;
}

void SWLRecordIncTLCSrcBlkDCnt(U8 ucPu, U16 Val)
{
    g_SWLRecord[ucPu]->ulTLCTotalSWLSrcBlkDirtyLpnCnt += Val;
    return;
}

void SWLRecordIncTLCWCnt_L3()
{
    g_SWLRecord[0]->ulSLC2TLCGCWCnt_L3++;
    return;
}

void SWLRecordIncTLCGCWCnt_L3()
{
    g_SWLRecord[0]->ulTLC2TLCGCWCnt_L3++;
    return;
}

void SWLRecordIncTLCSWLWCnt_L3()
{
    g_SWLRecord[0]->ulTLC2TLCSWLWCnt_L3++;
    return;
}

void SWLRecordIncDoneTime(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSwlDoneTimes++;
    return;
}

void SWLRecordIncCancelTime(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSwlCancelTimes++;
    return;
}

void SWLRecordIncTooCold(U8 ucSuperPU)
{
    g_SWLRecord[ucSuperPU]->ulSWLTriggerByTooColdCnt++;
    return;
}

void SWLRecordIncNormal(U8 ucSuperPU)
{
    g_SWLRecord[ucSuperPU]->ulSWLTriggerByNoraml++;
    return;
}

void SWLRecordTLCW(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulTLCBlkLevelWriteCnt++;
    return;
}

void SWLRecordGCSLC2SLC(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSLC2SLCGCCnt++;
    return;
}

void SWLRecordTLCGCCnt(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulTLCGCCnt++;
    return;
}

void SWLRecordSWLSwap(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSWLSwapCnt++;
    return;
}

void SWLRecordSWLGCCnt(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSWLGCCnt++;
    return;
}

void SWLRecordIncHostWCnt_L2(U8 ucSuperPu, U16 Val)
{
    g_SWLRecord[ucSuperPu]->ulhostWCnt_L2 += Val;
    return;
}

void SWLRecordIncHostWCnt_L1(U16 Val)
{
    g_SWLRecord[0]->ulhostWCnt_L1 += Val;
    return;
}

void SWLRecordClearHostWCnt_L2(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulhostWCnt_L2 = 0;
    return;
}

U32 SWLRecordRetHostWCnt_L2(U8 ucSuperPu)
{
    return g_SWLRecord[ucSuperPu]->ulhostWCnt_L2;
}

void DBG_DumpWholeChipWAF()
{
    U32 ulmid_val;
    U16 i;
    FLOAT Waf1, Waf2;
    FILE* pFile;
    U32 ulL3PrgCnt;
    unsigned long long ulL2PrgPgCnt_WithTable, ulL2PrgPgCnt;
    //unsigned long long int ulL0HostWriteSec;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    char filename[256];
    sprintf(filename, ".\\WLstatistic\\WAF.txt");
    pFile = fopen(filename, "a");
    if (pFile == NULL)
        return;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (g_SWLRecord[i]->ulhostWCnt_L1 % SEC_PER_BUF == 0)
        {
            ulmid_val = (U32)(g_SWLRecord[i]->ulhostWCnt_L1 / SEC_PER_BUF);
        }
        else
        {
            ulmid_val = (U32)(g_SWLRecord[i]->ulhostWCnt_L1 / SEC_PER_BUF + 1);
        }       

        ulL2PrgPgCnt = g_SWLRecord[i]->ulhostWCnt_L2 + g_SWLRecord[i]->ulSLCExtraGCWCnt + g_SWLRecord[i]->ulTotalTLCWPageCnt;
        ulL2PrgPgCnt_WithTable = g_SWLRecord[i]->ulhostWCnt_L2 + g_SWLRecord[i]->ulSLCExtraWCnt + g_SWLRecord[i]->ulTotalTLCWPageCnt;
        Waf1 = (FLOAT)((FLOAT)(ulL2PrgPgCnt) / ulmid_val);

        ulL3PrgCnt = L3_FMGetTotPrgCnt();
        Waf2 = (FLOAT)((FLOAT)(ulL3PrgCnt) / ulmid_val);

        //ulL0HostWriteSec = (unsigned long long int)((pHostInfoPage->TotalLBAWrittenHigh) << 32) + (unsigned long long int)pHostInfoPage->TotalLBAWrittenLow;

        fprintf(pFile, "\nSPU %d SWLRecord(L2_W/L1_W) WAF = %f, SMART(L3_W/L1_W) WAF = %f -- L0WrSec LBAHigh %d LBALow %d L1WrSec %d L2PrgPgCnt_NoTableW %llu L2PrgPgCnt_WithTable %llu L3PrgPgCnt %d \n", 
            i, Waf1, Waf2,pHostInfoPage->TotalLBAWrittenHigh,pHostInfoPage->TotalLBAWrittenLow, g_SWLRecord[i]->ulhostWCnt_L1, ulL2PrgPgCnt, ulL2PrgPgCnt_WithTable, ulL3PrgCnt);
    }
    fclose(pFile);
    return;
}
void DBG_DumpSWLRecordInfo()
{
    U16 i;
    FILE* pFile;
    U32 mid_val;
    U32 TLCMergeWCnt;
    char filename[256];
    sprintf(filename, ".\\WLstatistic\\SWLRecordInfo.txt");
    pFile = fopen(filename, "w");
    if (pFile == NULL)
        return;

    fprintf(pFile, "SUBSYSTEM_SUPERPU_NUM = %d\n", SUBSYSTEM_SUPERPU_NUM);
    fprintf(pFile, "SUBSYSTEM_PU_NUM = %d\n", SUBSYSTEM_PU_NUM);
    fprintf(pFile, "LUN_NUM_PER_SUPERPU = %d\n", LUN_NUM_PER_SUPERPU);
    fprintf(pFile, "PG_PER_BLK = %d\n", PG_PER_WL);
    fprintf(pFile, "PG_PER_BLK = %d\n", PG_PER_BLK);
    fprintf(pFile, "LPN_PER_BUF = %d\n", LPN_PER_BUF);
    fprintf(pFile, "--------------------------\n");

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (g_SWLRecord[i]->ulhostWCnt_L1 % SEC_PER_BUF == 0)
        {
            mid_val = (U32)(g_SWLRecord[i]->ulhostWCnt_L1 / SEC_PER_BUF);
        }
        else
        {
            mid_val = (U32)(g_SWLRecord[i]->ulhostWCnt_L1 / SEC_PER_BUF + 1);
        }

        fprintf(pFile, "SuperPu%u - hostWCnt_L1 = %llu\n", i, mid_val);
        fprintf(pFile, "SuperPu%u - hostWCnt_L2 = %u\n", i, g_SWLRecord[i]->ulhostWCnt_L2);
        //fprintf(pFile, "SuperPu%u - SwlCancelTimes = %u\n", i, g_SWLRecord[i]->ulSwlCancelTimes);
        fprintf(pFile, "SuperPu%u - SwlDoneTimes = %u\n", i, g_SWLRecord[i]->ulSwlDoneTimes);
        //fprintf(pFile, "SuperPu%u - SWLTriggerByNoraml = %u\n", i, g_SWLRecord[i]->ulSWLTriggerByNoraml);
        //fprintf(pFile, "SuperPu%u - SWLTriggerByTooColdCnt = %u\n", i, g_SWLRecord[i]->ulSWLTriggerByTooColdCnt);
        fprintf(pFile, "SuperPu%u - TLCWriteCnt = %u\n", i, g_SWLRecord[i]->ulTLCBlkLevelWriteCnt);
        fprintf(pFile, "SuperPu%u - TLCGCCnt = %u\n", i, g_SWLRecord[i]->ulTLCGCCnt);
        fprintf(pFile, "SuperPu%u - SLC2SLCGCCnt = %u\n", i, g_SWLRecord[i]->ulSLC2SLCGCCnt);
        //fprintf(pFile, "SuperPu%u - SWLSwapCnt = %u\n", i, g_SWLRecord[i]->ulSWLSwapCnt);
        //fprintf(pFile, "SuperPu%u - SWLGCCnt = %u\n", i, g_SWLRecord[i]->ulSWLGCCnt);

        fprintf(pFile, "SuperPu%u - SLCExtraWCnt = %u\n", i, g_SWLRecord[i]->ulSLCExtraWCnt);
        fprintf(pFile, "SuperPu%u - SLCExtraTableWCnt = %u\n", i, g_SWLRecord[i]->ulSLCExtraTableWCnt);
        g_SWLRecord[i]->ulSLCExtraGCWCnt = g_SWLRecord[i]->ulSLCExtraWCnt - g_SWLRecord[i]->ulSLCExtraTableWCnt;
        fprintf(pFile, "SuperPu%u - SLCExtraGCWCnt = %u\n", i, g_SWLRecord[i]->ulSLCExtraGCWCnt);

        fprintf(pFile, "SuperPu%u - TLCExtraWCnt = %u\n", i, g_SWLRecord[i]->ulTLCExtraWCnt);

        TLCMergeWCnt = g_SWLRecord[i]->ulSLC2TLCMergeWCnt + g_SWLRecord[i]->ulTLCBlkLevelWriteCnt * PG_PER_WL;

        fprintf(pFile, "SuperPu%u - SLC2TLCMergeWCnt = %u\n", i, TLCMergeWCnt);

        g_SWLRecord[i]->ulTLC2TLCSWLWCnt = g_SWLRecord[i]->ulSwlDoneTimes *PG_PER_BLK * LUN_NUM_PER_SUPERPU;
        fprintf(pFile, "SuperPu%u - TLC2TLCSWLWCnt = %u\n", i, g_SWLRecord[i]->ulTLC2TLCSWLWCnt);
        g_SWLRecord[i]->ulTLC2TLCGCWCnt = g_SWLRecord[i]->ulTLCExtraWCnt - TLCMergeWCnt - g_SWLRecord[i]->ulTLC2TLCSWLWCnt;
        fprintf(pFile, "SuperPu%u - TLC2TLCGCWCnt = %u\n", i, g_SWLRecord[i]->ulTLC2TLCGCWCnt);
        fprintf(pFile, "SuperPu%u - TLCTotalSWLSrcBlkDirtyLpnCnt = %u\n", i, g_SWLRecord[i]->ulTLCTotalSWLSrcBlkDirtyLpnCnt);

        fprintf(pFile, "\nSuperPu%u - SLC2TLCGCWCnt_L3 = %u\n", i, g_SWLRecord[i]->ulSLC2TLCGCWCnt_L3);
        fprintf(pFile, "SuperPu%u - TLC2TLCGCWCnt_L3 = %u\n", i, g_SWLRecord[i]->ulTLC2TLCGCWCnt_L3);
        fprintf(pFile, "SuperPu%u - TLC2TLCSWLWCnt_L3 = %u\n", i, g_SWLRecord[i]->ulTLC2TLCSWLWCnt_L3);

        fprintf(pFile, "--------------------------\n");
    }

    fclose(pFile);
    return;
}
#endif


GLOBAL MCU12_VAR_ATTR L2DbgCnt DBGCnter;
void DBG_ResetDBGCnt()
{
    U16 i;
    for (i = 0; i < DbgCntTypeAll; i++)
    {
        DBGCnter.Cnt[i] = 0;
        DBGCnter.StartTimer[i].QuadPart = 0;
        DBGCnter.TotalTimer[i].QuadPart = 0;
    }
}

void DBG_IncDBGCnt(DbgCntType SN)
{
    DBGCnter.Cnt[SN]++;
}

void DBG_RecordStartTime(DbgCntType SN)
{
    QueryPerformanceCounter(&DBGCnter.StartTimer[SN]);
}

void DBG_RecordRunTime(DbgCntType SN)
{
    LARGE_INTEGER StopTime;
    LARGE_INTEGER DiffTime;
    LARGE_INTEGER Freq;

    QueryPerformanceCounter(&StopTime);
    QueryPerformanceFrequency(&Freq);
    DiffTime.QuadPart = StopTime.QuadPart - DBGCnter.StartTimer[SN].QuadPart;
    DBGCnter.TotalTimer[SN].QuadPart += DiffTime.QuadPart;
}

GLOBAL MCU12_VAR_ATTR L2DbgSNInfo DbgSNInfo;
void DBG_RecordSNInfo(DbgSNType SNType, U32 Value)
{
    DbgSNInfo.SN[SNType] = Value;
}

void DBG_DumpPMTI()
{
    FILE* pPMTIFile;

    return;
    pPMTIFile = fopen("PMTI.txt", "w");

    if (pPMTIFile == NULL)
        return;

    fprintf(pPMTIFile, "PMTIIndex\t");
    fprintf(pPMTIFile, "PUSer\t");
    fprintf(pPMTIFile, "PBN\t");
    fprintf(pPMTIFile, "PPO\t");
    fprintf(pPMTIFile, "LPN\t");
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "RamAddr\t");
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "RebuildOn\t");
    fprintf(pPMTIFile, "InRam\t");
    fprintf(pPMTIFile, "AfterRebuild\t");
    fprintf(pPMTIFile, "Valid\t");
    fprintf(pPMTIFile, "Dirty\t");
    fprintf(pPMTIFile, "Exist\t");
    fprintf(pPMTIFile, "\n");


    /*for(PMTIIndex = 0; PMTIIndex < PMTPAGE_CNT_TOTAL; PMTIIndex++)
    {
    extern PMTI* g_PMTI;
    PMTIItem* pItem;

    pItem = &g_PMTI->m_PMTIItems[PMTIIndex];

    fprintf(pPMTIFile, "%d\t", PMTIIndex);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_PUSer);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_BlockInPU);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_PageInBlock);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_LPNInPage);
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "0x%x\t", pItem->m_bRebuildOn);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_bAfterRebuild);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_bDirty);
    fprintf(pPMTIFile, "\n");


    }*/

    fclose(pPMTIFile);
}

void DBG_DumpPMTManager()
{
    U32 i;
    U32 PUSer;
    U32 BlockSN;
    FILE* pFile;

    return;
    pFile = fopen("PMTManager.txt", "w");

    if (pFile == NULL)
        return;

    fprintf(pFile, "TableTS:\t");
    // fprintf(pFile, "%d\t", g_PMTManager->m_TableTimeStamp);
    //fprintf(pFile, "0x%x\t", g_PMTManager->m_TableTimeStamp);
    fprintf(pFile, "\n");

    fprintf(pFile, "BMT_CESer\t");
    for (PUSer = 0; PUSer < SUBSYSTEM_PU_NUM; PUSer++)
    {
        fprintf(pFile, "%x\t", PUSer);
    }
    fprintf(pFile, "\n");
    for (BlockSN = 0; BlockSN < AT1_BLOCK_COUNT; BlockSN++)
    {
        fprintf(pFile, "PMTBlkSN %02d\t", BlockSN);
        for (PUSer = 0; PUSer < SUBSYSTEM_PU_NUM; PUSer++)
        {
            fprintf(pFile, "0x%x\t", g_PMTManager->m_PMTBlkManager[PUSer].m_PMTBlkInfo[BlockSN]);
        }
        fprintf(pFile, "\n");
    }
    fprintf(pFile, "\n");

    fprintf(pFile, "PPO_CESer\t");
    for (PUSer = 0; PUSer < SUBSYSTEM_PU_NUM; PUSer++)
    {
        fprintf(pFile, "%x\t", PUSer);
    }
    fprintf(pFile, "\n");
#if 0
    for (BlockSN = 0; BlockSN < AT1_BLOCK_COUNT; BlockSN++)
    {
        fprintf(pFile, "PPO_%02d\t", BlockSN);
        for (PUSer = 0; PUSer < SUBSYSTEM_PU_NUM; PUSer++)
        {
            fprintf(pFile, "0x%x\t", g_PMTManager->m_PPO[PUSer][BlockSN]);
        }
        fprintf(pFile, "\n");
    }
#endif
    for (PUSer = 0; PUSer < SUBSYSTEM_PU_NUM; PUSer++)
    {
        fprintf(pFile, "0x%x\t", L2_GetCurPPOOfPMT(PUSer));
    }

    fprintf(pFile, "\n");
    fprintf(pFile, "FreePagePointer\t");
    fprintf(pFile, "FlushPointer\t");
    fprintf(pFile, "\n");

    fprintf(pFile, "\n");

    fprintf(pFile, "PMTIIndex in RAM\n");
    fprintf(pFile, "SN\t");
    fprintf(pFile, "PMTIIndex\t");
    fprintf(pFile, "Status\t");
    for (PUSer = 0; PUSer < SUBSYSTEM_PU_NUM; PUSer++)
    {
        for (i = 0; i < PMTPAGE_CNT_PER_PU; i++)
        {
            fprintf(pFile, "%d\t", i);
            // fprintf(pFile, "0x%x\t", g_PMTManager->m_PMTIIndex[i]);
            fprintf(pFile, "0x%x\t", g_PMTManager->m_FlushStatus[PUSer][0][i]);
            fprintf(pFile, "\n");
        }
    }

    fclose(pFile);

    /*

    U32 m_FlushStatus[PMTPAGE_CNT_TOTAL];
    //Flush Status, before flush into the Flash, set to 1
    //when the program is accomplished, L3 will set it to 0.

    U32 m_DirtyCnt;
    */

}

void DBG_DumpTLCSrcBlkDCnt(U16 VBN, U32 usSrcBlkDCnt, U8 ucIsSWL)
{
    FILE* pFile0;
    FILE* pFile1;
    char filename0[256];
    char filename1[256];
    sprintf(filename0, ".\\WLstatistic\\TLCGCSrcBlkDCnt.txt");
    sprintf(filename1, ".\\WLstatistic\\TLCSWLSrcBlkDCnt.txt");

    if (TRUE != ucIsSWL)
    {
        pFile0 = fopen(filename0, "a");
        if (pFile0 == NULL)
            return;
        fprintf(pFile0, "%u\n", usSrcBlkDCnt);
        fclose(pFile0);
    }
    else
    {
        pFile1 = fopen(filename1, "a");
        if (pFile1 == NULL)
            return;
        fprintf(pFile1, "%u\n", usSrcBlkDCnt);
        fclose(pFile1);
    }
    
    return;
}

extern GLOBAL MCU12_VAR_ATTR PBIT *pPBIT[SUBSYSTEM_SUPERPU_MAX];

void DBG_DumpSTD()
{
    DOUBLE sum_SLC = 0, sum_TLC = 0;
    U32 aver_SLC = 0, totle_blk_num_SLC = 0;
    U32 aver_TLC = 0, totle_blk_num_TLC = 0;
    U32 i, j;
    FILE* pFile_SLC;
    FILE* pFile_TLC;
    char filename_SLC[256];
    char filename_TLC[256];
    FLOAT STD_SLC, STD_TLC;
    for (j = 0; j < (BLK_PER_PLN + RSVD_BLK_PER_LUN); j++)
    {
        for (i = 0; i <SUBSYSTEM_SUPERPU_NUM; i++)
        {

            if ((pPBIT[i]->m_PBIT_Entry[0][j].bError != TRUE))
            {
                if (pPBIT[i]->m_PBIT_Entry[0][j].bTLC == TRUE)
                {
                    sum_TLC += pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                    totle_blk_num_TLC++;
                }
                else if ((pPBIT[i]->m_PBIT_Entry[0][j].bTable != TRUE) && (pPBIT[i]->m_PBIT_Entry[0][j].bReserved != TRUE) && (pPBIT[i]->m_PBIT_Entry[0][j].bBackup != TRUE))
                {
                    sum_SLC += pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                    totle_blk_num_SLC++;
                }
            }
        }
    }
    aver_TLC = (int)sum_TLC / totle_blk_num_TLC;
    aver_SLC = (int)sum_SLC / totle_blk_num_SLC;
    sum_TLC = 0;
    sum_SLC = 0;
    for (j = 0; j < (BLK_PER_PLN + RSVD_BLK_PER_LUN); j++)
    {
        for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
        {
            if (pPBIT[i]->m_PBIT_Entry[0][j].bError != TRUE)
            {
                if (pPBIT[i]->m_PBIT_Entry[0][j].bTLC == TRUE)
                {
                    sum_TLC += pow((abs(pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt - aver_TLC)), 2);
                }
                else if ((pPBIT[i]->m_PBIT_Entry[0][j].bTable != TRUE) && (pPBIT[i]->m_PBIT_Entry[0][j].bReserved != TRUE) && (pPBIT[i]->m_PBIT_Entry[0][j].bBackup != TRUE))
                {
                    sum_SLC += pow((abs(pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt - aver_SLC)), 2);
                }
            }
        }
    }
    STD_TLC = (FLOAT)(sqrt(sum_TLC / totle_blk_num_TLC));
    STD_SLC = (FLOAT)(sqrt(sum_SLC / totle_blk_num_SLC));

    sprintf(filename_TLC, ".\\WLstatistic\\STD_TLC.txt");
    sprintf(filename_SLC, ".\\WLstatistic\\STD_SLC.txt");
    pFile_TLC = fopen(filename_TLC, "a");
    pFile_SLC = fopen(filename_SLC, "a");
    if ((pFile_TLC == NULL) || (pFile_SLC == NULL))
        return;
    fprintf(pFile_TLC, "%f\n", STD_TLC);
    fprintf(pFile_SLC, "%f\n", STD_SLC);
    fclose(pFile_TLC);
    fclose(pFile_SLC);
    return;
}



void DBG_DumpPBITEraseCnt()
{
    U32 i, j;
    FILE* pFile;
    FILE *fptr;
    SYSTEMTIME sys_time;
    char filename[256], filename2[256];
    U32 MCU;
    U32 MaxEC = 0, MinEC = INVALID_8F;
    U32 AvgECTLC = 0;
    U32 BlkCnt = 0;
    U32 ulmid_val = 0;

    //return;
    MCU = XT_RSR_PRID();

    GetLocalTime(&sys_time);
    if (MCU == 2)
    {
        sprintf(filename, ".\\WLstatistic\\PBITEraseCnt_MCU1_%d_%d_%d_%d_%d_%d.xls", sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
        sprintf(filename2, ".\\WLstatistic\\MCU1_EraseCnt.txt");
    }
    else
    {
        sprintf(filename, ".\\WLstatistic\\PBITEraseCnt_MCU2_%d_%d_%d_%d_%d_%d.xls", sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
        sprintf(filename2, ".\\WLstatistic\\MCU2_EraseCnt.txt");
    }
    pFile = fopen(filename, "w");
    fptr = fopen(filename2, "a");

    if (pFile == NULL)
        return;

    for (j = 0; j < (BLK_PER_PLN + RSVD_BLK_PER_LUN); j++)
    {
        for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
        {
            fprintf(pFile, "%d\t", pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt);
            if (pPBIT[i]->m_PBIT_Entry[0][j].bTLC == TRUE)
            {
                if (MaxEC < pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt)
                {
                    MaxEC = pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                }

                if (MinEC > pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt)
                {
                    MinEC = pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                }
                AvgECTLC += pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                BlkCnt++;
            }
        }
        fprintf(pFile, "\n");
    }

#if defined(SWL_EVALUATOR)
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        if (g_SWLRecord[i]->ulhostWCnt_L1 % SEC_PER_BUF == 0)
        {
            ulmid_val = (U32)(g_SWLRecord[i]->ulhostWCnt_L1 / SEC_PER_BUF);
        }
        else
        {
           ulmid_val = (U32)(g_SWLRecord[i]->ulhostWCnt_L1 / SEC_PER_BUF + 1);
        }

    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        fprintf(pFile, "PU:%d SWL done\t%d\t SWL cancel\t%d\t", i, g_SWLRecord[i]->ulSwlDoneTimes, g_SWLRecord[i]->ulSwlCancelTimes);
        fprintf(pFile, "TooCold\t%d\tNormal\t%d\n", g_SWLRecord[i]->ulSWLTriggerByTooColdCnt, g_SWLRecord[i]->ulSWLTriggerByNoraml);
        fprintf(pFile, "TLCW\t%d\tTLCGC\t%d\tSLCGC\t%d\n", g_SWLRecord[i]->ulTLCBlkLevelWriteCnt, g_SWLRecord[i]->ulTLCGCCnt, g_SWLRecord[i]->ulSLC2SLCGCCnt);

        fprintf(fptr, "%d\t%d\t%d\nSwlDoneTimes = %u\nTLCWriteCnt = %u\nTLCGCCnt = %u\n", MaxEC, MinEC, (AvgECTLC / BlkCnt), g_SWLRecord[i]->ulSwlDoneTimes, g_SWLRecord[i]->ulTLCBlkLevelWriteCnt, g_SWLRecord[i]->ulTLCGCCnt);
        fprintf(fptr, "SuperPu%u - hostWCnt_L1 = %llu\n", i, ulmid_val);
        fprintf(fptr, "SuperPu%u - hostWCnt_L2 = %u\n", i, g_SWLRecord[i]->ulhostWCnt_L2);
        fprintf(fptr, "SuperPu%u - SLCExtraWCnt = %u\n", i, g_SWLRecord[i]->ulSLCExtraWCnt);
    }
#endif

    fclose(pFile);
    fclose(fptr);

}

/*void SWLRecordInit(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSwlCancelTimes = 0;
    g_SWLRecord[ucPu]->ulSwlDoneTimes = 0;
    g_SWLRecord[ucPu]->ulSWLTriggerByNoraml = 0;
    g_SWLRecord[ucPu]->ulSWLTriggerByTooColdCnt = 0;
    g_SWLRecord[ucPu]->ulTLCWriteCnt = 0;
    g_SWLRecord[ucPu]->ulSLC2TLCGCCnt = 0;
    g_SWLRecord[ucPu]->ulSLC2SLCGCGnt = 0;
    g_SWLRecord[ucPu]->ulSWLSwapCnt = 0;
    g_SWLRecord[ucPu]->ulSWLGCCnt = 0;
    g_SWLRecord[ucPu]->ulhostWCnt = 0;
}

void SWLRecordIncDoneTime(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSwlDoneTimes++;
    return;
}

void SWLRecordIncCancelTime(U8 ucPu)
{
    g_SWLRecord[ucPu]->ulSwlCancelTimes++;
    return;
}

void SWLRecordIncTooCold(U8 ucSuperPU)
{
    g_SWLRecord[ucSuperPU]->ulSWLTriggerByTooColdCnt++;
    return;
}

void SWLRecordIncNormal(U8 ucSuperPU)
{
    g_SWLRecord[ucSuperPU]->ulSWLTriggerByNoraml++;
    return;
}

void SWLRecordTLCW(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulTLCWriteCnt++;
    return;
}

void SWLRecordGCSLC2SLC(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSLC2SLCGCGnt++;
    return;
}

void SWLRecordGCTLC2SLC(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSLC2TLCGCCnt++;
    return;
}

void SWLRecordSWLSwap(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSWLSwapCnt++;
    return;
}

void SWLRecordSWLGCCnt(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulSWLGCCnt++;
    return;
}

void SWLRecordIncHostWCnt(U8 ucSuperPu, U16 Val)
{
    g_SWLRecord[ucSuperPu]->ulhostWCnt += Val;
    return;
}

void SWLRecordClearHostWCnt(U8 ucSuperPu)
{
    g_SWLRecord[ucSuperPu]->ulhostWCnt = 0;
    return;
}

U32 SWLRecordRetHostWCnt(U8 ucSuperPu)
{
    return g_SWLRecord[ucSuperPu]->ulhostWCnt;
}
#endif


GLOBAL MCU12_VAR_ATTR L2DbgCnt DBGCnter;------------------------
void DBG_ResetDBGCnt()
{
    U16 i;
    for (i = 0; i < DbgCntTypeAll; i++)
    {
        DBGCnter.Cnt[i] = 0;
        DBGCnter.StartTimer[i].QuadPart = 0;
        DBGCnter.TotalTimer[i].QuadPart = 0;
    }
}

void DBG_IncDBGCnt(DbgCntType SN)
{
    DBGCnter.Cnt[SN]++;
}

void DBG_RecordStartTime(DbgCntType SN)
{
    QueryPerformanceCounter(&DBGCnter.StartTimer[SN]);
}

void DBG_RecordRunTime(DbgCntType SN)
{
    LARGE_INTEGER StopTime;
    LARGE_INTEGER DiffTime;
    LARGE_INTEGER Freq;

    QueryPerformanceCounter(&StopTime);
    QueryPerformanceFrequency(&Freq);
    DiffTime.QuadPart = StopTime.QuadPart - DBGCnter.StartTimer[SN].QuadPart;
    DBGCnter.TotalTimer[SN].QuadPart += DiffTime.QuadPart;
}

GLOBAL MCU12_VAR_ATTR L2DbgSNInfo DbgSNInfo;
void DBG_RecordSNInfo(DbgSNType SNType, U32 Value)
{
    DbgSNInfo.SN[SNType] = Value;
}

void DBG_DumpPMTI()
{
    FILE* pPMTIFile;

    return;
    pPMTIFile = fopen("PMTI.txt", "w");

    if (pPMTIFile == NULL)
        return;

    fprintf(pPMTIFile, "PMTIIndex\t");
    fprintf(pPMTIFile, "PUSer\t");
    fprintf(pPMTIFile, "PBN\t");
    fprintf(pPMTIFile, "PPO\t");
    fprintf(pPMTIFile, "LPN\t");
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "RamAddr\t");
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "RebuildOn\t");
    fprintf(pPMTIFile, "InRam\t");
    fprintf(pPMTIFile, "AfterRebuild\t");
    fprintf(pPMTIFile, "Valid\t");
    fprintf(pPMTIFile, "Dirty\t");
    fprintf(pPMTIFile, "Exist\t");
    fprintf(pPMTIFile, "\n");


    /*for(PMTIIndex = 0; PMTIIndex < PMTPAGE_CNT_TOTAL; PMTIIndex++)
    {
    extern PMTI* g_PMTI;
    PMTIItem* pItem;

    pItem = &g_PMTI->m_PMTIItems[PMTIIndex];

    fprintf(pPMTIFile, "%d\t", PMTIIndex);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_PUSer);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_BlockInPU);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_PageInBlock);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_PhyAddr.m_LPNInPage);
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "\t");
    fprintf(pPMTIFile, "0x%x\t", pItem->m_bRebuildOn);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_bAfterRebuild);
    fprintf(pPMTIFile, "0x%x\t", pItem->m_bDirty);
    fprintf(pPMTIFile, "\n");


    }*/

/*    fclose(pPMTIFile);----------------------------
}

void DBG_DumpPMTManager()
{
    U32 i;
    U32 PUSer;
    U32 BlockSN;
    FILE* pFile;

    return;
    pFile = fopen("PMTManager.txt", "w");

    if (pFile == NULL)
        return;

    fprintf(pFile, "TableTS:\t");
    // fprintf(pFile, "%d\t", g_PMTManager->m_TableTimeStamp);
    //fprintf(pFile, "0x%x\t", g_PMTManager->m_TableTimeStamp);
    fprintf(pFile, "\n");

    fprintf(pFile, "BMT_CESer\t");
    for (PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
    {
        fprintf(pFile, "%x\t", PUSer);
    }
    fprintf(pFile, "\n");
    for (BlockSN = 0; BlockSN < AT1_BLOCK_COUNT; BlockSN++)
    {
        fprintf(pFile, "PMTBlkSN %02d\t", BlockSN);
        for (PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
        {
            fprintf(pFile, "0x%x\t", g_PMTManager->m_PMTBlkManager[PUSer].m_PMTBlkInfo[BlockSN]);
        }
        fprintf(pFile, "\n");
    }
    fprintf(pFile, "\n");

    fprintf(pFile, "PPO_CESer\t");
    for (PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
    {
        fprintf(pFile, "%x\t", PUSer);
    }
    fprintf(pFile, "\n");
#if 0
    for(BlockSN = 0; BlockSN < AT1_BLOCK_COUNT; BlockSN++)
    {
        fprintf(pFile, "PPO_%02d\t", BlockSN);
        for(PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
        {
            fprintf(pFile, "0x%x\t", g_PMTManager->m_PPO[PUSer][BlockSN]);
        }
        fprintf(pFile, "\n");
    }
#endif
    for (PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
    {
        fprintf(pFile, "0x%x\t", L2_GetCurPPOOfPMT(PUSer));
    }

    fprintf(pFile, "\n");
    fprintf(pFile, "FreePagePointer\t");
    fprintf(pFile, "FlushPointer\t");
    fprintf(pFile, "\n");

    fprintf(pFile, "\n");

    fprintf(pFile, "PMTIIndex in RAM\n");
    fprintf(pFile, "SN\t");
    fprintf(pFile, "PMTIIndex\t");
    fprintf(pFile, "Status\t");
    for (PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
    {
        for (i = 0; i < PMTPAGE_CNT_PER_PU; i++)
        {
            fprintf(pFile, "%d\t", i);
            // fprintf(pFile, "0x%x\t", g_PMTManager->m_PMTIIndex[i]);
            fprintf(pFile, "0x%x\t", g_PMTManager->m_FlushStatus[PUSer][0][i]);
            fprintf(pFile, "\n");
        }
    }

    fclose(pFile);

    /*

        U32 m_FlushStatus[PMTPAGE_CNT_TOTAL];
        //Flush Status, before flush into the Flash, set to 1
        //when the program is accomplished, L3 will set it to 0.

        U32 m_DirtyCnt;
        */

/*}---------------------------------------


void DBG_DumpPBITEraseCnt()
{
    U32 i, j;
    FILE* pFile;
    FILE *fptr;
    SYSTEMTIME sys_time;
    char filename[256], filename2[256];
    U32 MCU;
    U32 MaxEC = 0, MinEC = INVALID_8F;
    U32 AvgECTLC = 0;
    U32 BlkCnt = 0;

    //return;
    MCU = XT_RSR_PRID();

    GetLocalTime(&sys_time);
    if (MCU == 2)
    {
        sprintf(filename, ".\\WLstatistic\\PBITEraseCnt_MCU1_%d_%d_%d_%d_%d_%d.xls", sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
        sprintf(filename2, ".\\WLstatistic\\MCU1_EraseCnt.txt");
    }
    else
    {
        sprintf(filename, ".\\WLstatistic\\PBITEraseCnt_MCU2_%d_%d_%d_%d_%d_%d.xls", sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
        sprintf(filename2, ".\\WLstatistic\\MCU2_EraseCnt.txt");
    }
    pFile = fopen(filename, "w");
    fptr = fopen(filename2, "a");

    if (pFile == NULL)
        return;

    for (j = 0; j < BLK_PER_PLN; j++)
    {
        for (i = 0; i < SUBSYSTEM_LUN_NUM; i++)
        {
            fprintf(pFile, "%d\t", pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt);
            if (pPBIT[i]->m_PBIT_Entry[0][j].bTLC == TRUE)
            {
                if (MaxEC < pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt)
                {
                    MaxEC = pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                }

                if (MinEC > pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt)
                {
                    MinEC = pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                }
                AvgECTLC += pPBIT[i]->m_PBIT_Entry[0][j].EraseCnt;
                BlkCnt++;
            }
        }
        fprintf(pFile, "\n");
    }

#if (defined(SIM) && (!defined(SWL_OFF)))
    for (i = 0; i < SUBSYSTEM_LUN_NUM; i++)
    {
        fprintf(pFile, "PU:%d SWL done\t%d\t SWL cancel\t%d\t", i, g_SWLRecord[i]->ulSwlDoneTimes, g_SWLRecord[i]->ulSwlCancelTimes);
        fprintf(pFile, "TooCold\t%d\tNormal\t%d\n", g_SWLRecord[i]->ulSWLTriggerByTooColdCnt, g_SWLRecord[i]->ulSWLTriggerByNoraml);
        fprintf(pFile, "TLCW\t%d\tTLCGC\t%d\tSLCGC\t%d\n", g_SWLRecord[i]->ulTLCWriteCnt, g_SWLRecord[i]->ulSLC2TLCGCCnt, g_SWLRecord[i]->ulSLC2SLCGCGnt);
    }
#endif
    fprintf(fptr, "%d\t%d\t%d\n", MaxEC, MinEC, (AvgECTLC / BlkCnt));
    fclose(pFile);
    fclose(fptr);

}*/

#ifdef L2MEASURE
void DBG_DumpWCMDCnt()
{
    U32 MCU;
    U8 ucWriteType;
    U8 ucSuperPu;
    U32 TableWritePg = 0;
    U32 DataWritePg = 0;
    U32 DeviceWritePg = 0;
    U32 OtherWritePg = 0;
    FILE *fptr;

    fptr = fopen(".\\WLstatistic\\L2TraceWCM.txt", "a");

    MCU = XT_RSR_PRID();

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_LUN_NUM; ucSuperPu++)
    {
        for (ucWriteType = 0; ucWriteType < L2MEASURE_TYPE_ALL; ucWriteType++)
        {
            switch(ucWriteType)
            {
            case L2MEASURE_TYPE_HOST_SLC:
            case L2MEASURE_TYPE_TLCMERGE_TLC:
            case L2MEASURE_TYPE_TLCGC_SLC:
            case L2MEASURE_TYPE_SLCGC_SLC:
            case L2MEASURE_TYPE_SWLALL_TLC:
                DataWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 2);
                DeviceWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 2);
                break;
            case L2MEASURE_TYPE_RPMT_SLC:
            case L2MEASURE_TYPE_RT_SLC:
            case L2MEASURE_TYPE_AT0_SLC:
            case L2MEASURE_TYPE_AT1_SLC:
                TableWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 2);
                DeviceWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 2);
                break;            
            case L2MEASURE_TYPE_BBT_SLC:
                TableWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 1);
                DeviceWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 1);
                break;
            case L2MEASURE_TYPE_OTHER:
                OtherWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 2);
                DeviceWritePg += (g_TraceWrite[ucSuperPu][ucWriteType] * 2);
                break;
            }
        }
    }
    DBG_Printf("MCU :%d, TableWritePg:%d, DataWritePg:%d, OtherWritePg:%d, DeviceWritePg:%d\n",
        MCU, TableWritePg, DataWritePg, OtherWritePg, DeviceWritePg);
    fprintf(fptr, "MCU :%d\tTableWritePg:%d\tDataWritePg:%d\tOtherWritePg:%d\tDeviceWritePg:%d\n",
        MCU, TableWritePg, DataWritePg, OtherWritePg, DeviceWritePg);
    DBG_Printf("  \n");
    fclose(fptr);
}
#endif

#else
void DBG_DumpPMTManager()
{
    ;
}

void DBG_DumpPMTI()
{
    ;
}

void DBG_ResetCounter()
{
    return;
}

void Dbg_InitStatistic()
{
    return;
}

void Dbg_IncGlobalSN(int Cnt)
{

    return;
}

void Dbg_IncHostWriteCnt(int PageCnt)
{
    return;
}

void Dbg_IncDevWriteCnt(int PageCnt)
{
    return;
}

void Dbg_ReportStatistic(char* pMsg)
{
    return;
}


void DBG_ResetDBGCnt()
{
    return;
}

void DBG_IncDBGCnt(DbgCntType SN)
{
    return;
}

void DBG_RecordStartTime(DbgCntType SN)
{
    return;
}

void DBG_RecordRunTime(DbgCntType SN)
{
    return;
}


void DBG_RecordSNInfo(DbgSNType SNType, U32 Value)
{
    return;
}

void DBG_DumpPBITEraseCnt()
{
    return;
}
#endif


#ifdef L2MEASURE
void MCU1_DRAM_TEXT L2MeasureLogInit(U8 ucSuperPu)
{
    U8 ucWriteType;
    U16 usBlk;

    for (ucWriteType = 0; ucWriteType < L2MEASURE_TYPE_ALL; ucWriteType++)
    {
        g_TraceWrite[ucSuperPu][ucWriteType] = 0;
    }

    for (usBlk = 0; usBlk < BLK_PER_LUN + RSVD_BLK_PER_LUN; usBlk++)
    {
        pMeasureErase[ucSuperPu]->PBN[0][usBlk].ulHostErase = 0;
        pMeasureErase[ucSuperPu]->PBN[0][usBlk].ulSLC2TLCErase = 0;
        pMeasureErase[ucSuperPu]->PBN[0][usBlk].ulSLCGCErase = 0;
        pMeasureErase[ucSuperPu]->PBN[0][usBlk].ulTLCErrErase = 0;
        pMeasureErase[ucSuperPu]->PBN[0][usBlk].ulTLCGCErase = 0;
        pMeasureErase[ucSuperPu]->PBN[0][usBlk].ulTLCSWLErase = 0;
    }

    return;
}

void MCU1_DRAM_TEXT L2MeasureLogIncWCnt(U8 ucSuperPu, U8 ucWType)
{
    g_TraceWrite[ucSuperPu][ucWType]++;
    return;
}

void MCU1_DRAM_TEXT L2MeasureLogIncECTyepCnt(U8 ucSuperPu, U16 usPBN, U8 ucECType)
{
    switch(ucECType)
    {
    case L2MEASURE_ERASE_HOST:
        pMeasureErase[ucSuperPu]->PBN[0][usPBN].ulHostErase++;
        break;
    case L2MEASURE_ERASE_SLC2TLC:
        pMeasureErase[ucSuperPu]->PBN[0][usPBN].ulSLC2TLCErase++;
        break;
    case L2MEASURE_ERASE_SLCGC:
        pMeasureErase[ucSuperPu]->PBN[0][usPBN].ulSLCGCErase++;
        break;
    case L2MEASURE_ERASE_TLCGC:
        pMeasureErase[ucSuperPu]->PBN[0][usPBN].ulTLCGCErase++;
        break;
    case L2MEASURE_ERASE_TLCSWL:
        pMeasureErase[ucSuperPu]->PBN[0][usPBN].ulTLCSWLErase++;
        break;
    case L2MEASURE_ERASE_TLCERR:
        pMeasureErase[ucSuperPu]->PBN[0][usPBN].ulTLCErrErase++;
        break;
    }
    return;
}

U8 MCU1_DRAM_TEXT L2MeasureLogTargetTypeMapping(U8 ucTargetType)
{
    U8 ucTraceTypeact;

    switch(ucTargetType)
    {
    case TARGET_HOST_WRITE_NORAML:
        ucTraceTypeact = L2MEASURE_TYPE_HOST_SLC;
        break;
    case TARGET_HOST_GC:
        ucTraceTypeact = L2MEASURE_TYPE_SLCGC_SLC;
        break;
    case TARGET_HOST_WRITE_ACC:
        ucTraceTypeact = L2MEASURE_TYPE_OTHER;
        break;
    case TARGET_TLC_WRITE:
        ucTraceTypeact = L2MEASURE_TYPE_TLCMERGE_TLC;
        break;
    default:
        DBG_Printf("TargetTypeError\n");
        DBG_Getch();
        break;
    }
    return ucTraceTypeact;
}

void MCU1_DRAM_TEXT L2MeasureLogDumpWriteCnt(U8 ucMCUId)
{
    U8 ucSuperPu, ucWriteType;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_LUN_NUM; ucSuperPu++)
    {
        for (ucWriteType = 0; ucWriteType < L2MEASURE_TYPE_ALL; ucWriteType++)
        {
            /*
            Data write
            L2MEASURE_TYPE_HOST_SLC,              0
            L2MEASURE_TYPE_TLCMERGE_TLC,       1
            L2MEASURE_TYPE_TLCGC_SLC,             2
            L2MEASURE_TYPE_SLCGC_SLC,             3
            L2MEASURE_TYPE_SWLALL_TLC,           4
            Table write
            L2MEASURE_TYPE_RPMT_SLC,              7
            L2MEASURE_TYPE_RT_SLC,                  8
            L2MEASURE_TYPE_AT0_SLC,                9
            L2MEASURE_TYPE_AT1_SLC                 10
            BBT write
            L2MEASURE_TYPE_BBT_SLC                 11
            Other write
            L2MEASURE_TYPE_OTHER                    12

            Erase
            */
            if (ucWriteType != L2MEASURE_TYPE_BBT_SLC)
            {
                DBG_Printf("MCU:%d\tPU:%d\tWType:%d\tCnt:%d\n",ucMCUId,  ucSuperPu, ucWriteType, (g_TraceWrite[ucSuperPu][ucWriteType] * 2));
            }
            else
            {
                DBG_Printf("MCU:%d\tPU:%d\tWType:%d\tCnt:%d\n",ucMCUId,  ucSuperPu, ucWriteType, g_TraceWrite[ucSuperPu][ucWriteType]);
            }
        }
    }
    return;
}

void MCU1_DRAM_TEXT L2_DumpEraseCnt(U8 ucMCUId)
{
    U8  ucLunInSuperPu = 0;
    U16 usBlock;
    U8 ucSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (usBlock = 0; usBlock < (BLK_PER_LUN + RSVD_BLK_PER_LUN); usBlock++)
            {
                if (1 == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bTLC)
                {
                    DBG_Printf("MCU:%d\tPU:%d\tPBN:%d\tEC:%d\tECPerPOR:%d\tTLC\tBad:%d\tBackUp:%d\tFree:%d\tTable:%d\tRetryFail:%d\n", ucMCUId, ucSuperPu,usBlock, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt, 
                        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].ECPerPOR,
                        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bBackup,
                        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bFree, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bTable,
                        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bRetryFail);
                }
                else
                {
                    if (1 == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bBackup)
                    {
                        DBG_Printf("MCU:%d\tPU:%d\tPBN:%d\tEC:%d\tECPerPOR:%d\tRSV\tBad:%d\tBackUp:%d\tFree:%d\tTable:%d\tRetryFail:%d\n", ucMCUId, ucSuperPu,usBlock, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].ECPerPOR,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bBackup,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bFree, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bTable,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bRetryFail);
                    }
                    else
                    {
                        DBG_Printf("MCU:%d\tPU:%d\tPBN:%d\tEC:%d\tECPerPOR:%d\tSLC\tBad:%d\tBackUp:%d\tFree:%d\tTable:%d\tRetryFail:%d\n", ucMCUId, ucSuperPu,usBlock, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].ECPerPOR,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bBackup,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bFree, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bTable,
                            pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bRetryFail);
                    }
                }
            }
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_DumpBlkEraseType(U8 ucMCUId)
{
    U8  ucLunInSuperPu = 0;
    U16 usBlock;
    U8 ucSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (usBlock = 0; usBlock < (BLK_PER_LUN + RSVD_BLK_PER_LUN); usBlock++)
            {
                DBG_Printf("MCU:%d\tPU:%d\tPBN:%d\tECPerPOR:%d\tHOSTEC:%d\tSLC2SLCEC:%d\tSLC2TLCEC:%d\tTLCGCEC:%d\tTLCSWLEC:%d\tTLCErrEC:%d\n", ucMCUId, ucSuperPu,usBlock,
                    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].ECPerPOR,
                    pMeasureErase[ucSuperPu]->PBN[ucLunInSuperPu][usBlock].ulHostErase,
                    pMeasureErase[ucSuperPu]->PBN[ucLunInSuperPu][usBlock].ulSLCGCErase,
                    pMeasureErase[ucSuperPu]->PBN[ucLunInSuperPu][usBlock].ulSLC2TLCErase,
                    pMeasureErase[ucSuperPu]->PBN[ucLunInSuperPu][usBlock].ulTLCGCErase,
                    pMeasureErase[ucSuperPu]->PBN[ucLunInSuperPu][usBlock].ulTLCSWLErase,
                    pMeasureErase[ucSuperPu]->PBN[ucLunInSuperPu][usBlock].ulTLCErrErase
                    );
            }
        }
    }

    return;
}

#endif

