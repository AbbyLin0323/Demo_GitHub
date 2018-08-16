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
Filename    :L2_Evaluate.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.05.24
Description :to evaluate the performance of the algorithm

Others      :
Modify      :
****************************************************************************/

#ifndef __L2_EVALUATER_H__
#define __L2_EVALUATER_H__
#include "L2_Defines.h"
#ifdef SIM
#include "windows.h"
#endif

#define TLC_TOTAL_RPMT_PAGE_NUM 3
typedef enum
{
    DbgCnt_GCEntryStart = 0,
    DbgCnt_CopyValidOneLineEntry,
    DbgCnt_CopyValidMultiLpnEntry,
    DbgCnt_TryToPopGC,
    DbgCnt_BeforePopGCPage,
    DbgCnt_BeforeLoadRPMT,
    DbgCnt_BeforeLookupPMT,
    DbgCnt_BeforeFillBuffer,
    DbgCnt_WriteOnePhysicalPage,
    DBGCnt_GC_STAGE_DONE,

    DbgCntTypeAll
}DbgCntType;
#ifdef SIM
typedef struct
{
    U32 Cnt[DbgCntTypeAll];
    LARGE_INTEGER StartTimer[DbgCntTypeAll];
    LARGE_INTEGER TotalTimer[DbgCntTypeAll];
}L2DbgCnt;
#endif

typedef enum
{
    DbgSN_LPN151B25_GlobalSN = 0,
    DbgSN_LPN151B25_PhysicalAddr,
    DBGSN_GCEntrySN,
    DBGSN_StartGCSN,
    DBGSN_LPNOther,
    DBGSN_All,
}DbgSNType;

typedef struct
{
    U32 SN[DBGSN_All];
}L2DbgSNInfo;

#if defined(SIM) && (defined(SWL_EVALUATOR))
typedef struct SWLRecord_tag
{
    U32 ulSwlDoneTimes;        // record the times of TLC SWL process
    U32 ulSwlCancelTimes;
    U32 ulSWLTriggerByTooColdCnt;
    U32 ulSWLTriggerByNoraml;
    U32 ulTLCBlkLevelWriteCnt; // record the times of TLC Merge process in blk level
    U32 ulTLCGCCnt;            // record the times of TLC GC process
    U32 ulSLC2SLCGCCnt;        // record the times of SLC GC process
    U32 ulSWLSwapCnt;
    U32 ulSWLGCCnt;
    U32 ulhostWCnt_L2;         // page-level write of L2 acception
    unsigned long long int ulhostWCnt_L1;  // lba-level cmd of L1 acception

    U32 ulSLCExtraWCnt;
    U32 ulSLCExtraTableWCnt;
    U32 ulSLCExtraGCWCnt;
    U32 ulTLCExtraWCnt;
    U32 ulSLC2TLCMergeWCnt;
    U32 ulTLC2TLCGCWCnt;
    U32 ulTLC2TLCSWLWCnt;
    U32 ulTLCTotalSWLSrcBlkDirtyLpnCnt;

    U32 ulSLC2TLCGCWCnt_L3;
    U32 ulTLC2TLCGCWCnt_L3;
    U32 ulTLC2TLCSWLWCnt_L3;
    U32 ulTotalTLCWPageCnt; //Total TLC Write page number, which means the data amount written into NAND
}SWLRecord;
#else
typedef struct SWLRecord_tag
{
    U32 ulSwlDoneTimes;
    U32 ulSwlCancelTimes;
    U32 ulSWLTriggerByTooColdCnt;
    U32 ulSWLTriggerByNoraml;
    U32 ulTLCWriteCnt;
    U32 ulSLC2TLCGCCnt;
    U32 ulSLC2SLCGCGnt;
    U32 ulSWLSwapCnt;
    U32 ulSWLGCCnt;
    U32 ulhostWCnt;
}SWLRecord;
#endif

typedef struct ECDetail_tag
{
    U32 ulHostErase;
    U32 ulSLC2TLCErase;
    U32 ulSLCGCErase;
    U32 ulTLCGCErase;
    U32 ulTLCSWLErase;
    U32 ulTLCErrErase;
}ECDetail;

typedef struct MeasureBlkEC_tag
{
    ECDetail PBN[LUN_NUM_PER_SUPERPU][BLK_PER_LUN + RSVD_BLK_PER_LUN];
}MeasureBlkEC;

typedef enum L2TraceType_Tag
{
    L2MEASURE_TYPE_HOST_SLC = 0,
    L2MEASURE_TYPE_TLCMERGE_TLC,
    L2MEASURE_TYPE_TLCGC_SLC,
    L2MEASURE_TYPE_SLCGC_SLC,
    L2MEASURE_TYPE_SWLALL_TLC,
    L2MEASURE_TYPE_RPMT_SLC,
    L2MEASURE_TYPE_RT_SLC,
    L2MEASURE_TYPE_AT0_SLC,
    L2MEASURE_TYPE_AT1_SLC,
    L2MEASURE_TYPE_BBT_SLC,
    L2MEASURE_TYPE_OTHER,         //saveTraceBlk, rebuild, SLCAcc...
    L2MEASURE_TYPE_ALL,
}L2TraceType;

typedef enum L2Measure_Tag
{
    L2MEASURE_ERASE_HOST,
    L2MEASURE_ERASE_SLC2TLC,
    L2MEASURE_ERASE_SLCGC,
    L2MEASURE_ERASE_TLCGC,
    L2MEASURE_ERASE_TLCSWL,
    L2MEASURE_ERASE_TLCERR,
    L2MEASURE_ERASE_ALL,
}L2MeasureEC;

#ifdef SIM

extern GLOBAL MCU12_VAR_ATTR L2DbgCnt DBGCnter;
extern GLOBAL MCU12_VAR_ATTR L2DbgSNInfo DbgSNInfo;
#endif

extern GLOBAL MCU12_VAR_ATTR U32 g_TraceWrite[SUBSYSTEM_PU_MAX][L2MEASURE_TYPE_ALL];
extern GLOBAL MCU12_VAR_ATTR MeasureBlkEC* pMeasureErase[SUBSYSTEM_SUPERPU_MAX];


void Dbg_InitStatistic();
void Dbg_IncGlobalSN(int Cnt);
void Dbg_IncHostWriteCnt(int PageCnt);
extern void Dbg_IncDevWriteCnt(int PageCnt);
void Dbg_ReportStatistic();
void DBG_ResetDBGCnt();
void DBG_IncDBGCnt(DbgCntType SN);
void DBG_RecordStartTime(DbgCntType SN);
void DBG_RecordRunTime(DbgCntType SN);

extern void DBG_RecordSNInfo(DbgSNType SNType, U32 Value);

extern void DBG_DumpPBITEraseCnt();

extern void DBG_DumpWCMDCnt();
extern void L2_DumpEraseCnt(U8 ucMCUId);
extern void L2_DumpBlkEraseType(U8 ucMCUId);

extern void SWLRecordInit(U8 ucPu);
extern void SWLRecordGCSLC2SLC(U8 ucSuperPu);
extern void SWLRecordTLCGCCnt(U8 ucSuperPu);
extern void SWLRecordSWLSwap(U8 ucSuperPu);
extern void SWLRecordIncHostWCnt_L1(U16 Val);
extern void SWLRecordIncHostWCnt_L2(U8 ucSuperPu, U16 Val);
extern void SWLRecordClearHostWCnt_L2(U8 ucSuperPu);
extern U32 SWLRecordRetHostWCnt_L2(U8 ucSuperPu);

extern void SWLRecordIncCancelTime(U8 ucPu);
extern void SWLRecordIncDoneTime(U8 ucPu);
extern void SWLRecordIncTooCold(U8 ucSuperPU);
extern void SWLRecordIncNormal(U8 ucSuperPU);
extern void SWLRecordSWLGCCnt(U8 ucSuperPu);

extern void L2MeasureLogInit(U8 ucSuperPu);
extern U8  L2MeasureLogTargetTypeMapping(U8 ucTargetType);
extern void L2MeasureLogIncWCnt(U8 ucSuperPu, U8 ucWType);
extern void L2MeasureLogIncECTyepCnt(U8 ucSuperPu, U16 usPBN, U8 ucECType);
extern void  L2MeasureLogDumpWriteCnt(U8 ucMCUId);

extern void DBG_DumpSTD();
extern void DBG_DumpTLCSrcBlkDCnt(U16 VBN, U32 usSrcBlkDCnt, U8 ucIsSWL);
extern void DBG_DumpSWLRecordInfo();
extern void SWLRecordIncSLCExtraWCnt(U8 ucPu);
extern void SWLRecordIncSLCTableWCnt(U8 ucPu);
extern void SWLRecordIncTLCExtraWCnt(U8 ucPu);
extern void SWLRecordIncSLC2TLCMergeWCnt(U8 ucPu);
extern void SWLRecordIncTLCSrcBlkDCnt(U8 ucPu, U16 Val);

extern void SWLRecordIncTLCWCnt_L3();

extern void SWLRecordIncTLCGCWCnt_L3();

extern void SWLRecordIncTLCSWLWCnt_L3();

extern void SWL_TotalTLCWPageCntInc(U8 ucPu, U16 ucPrgCnt);
extern void DBG_DumpWholeChipWAF();
/*extern GLOBAL MCU12_VAR_ATTR L2DbgCnt DBGCnter;
extern GLOBAL MCU12_VAR_ATTR L2DbgSNInfo DbgSNInfo;
#endif

extern GLOBAL MCU12_VAR_ATTR U32 g_TraceWrite[SUBSYSTEM_PU_MAX][L2MEASURE_TYPE_ALL];
extern GLOBAL MCU12_VAR_ATTR MeasureBlkEC* pMeasureErase[SUBSYSTEM_SUPERPU_MAX];


void Dbg_InitStatistic();
void Dbg_IncGlobalSN(int Cnt);
void Dbg_IncHostWriteCnt(int PageCnt);
extern void Dbg_IncDevWriteCnt(int PageCnt);
void Dbg_ReportStatistic();
void DBG_ResetDBGCnt();
void DBG_IncDBGCnt(DbgCntType SN);
void DBG_RecordStartTime(DbgCntType SN);
void DBG_RecordRunTime(DbgCntType SN);

extern void DBG_RecordSNInfo(DbgSNType SNType, U32 Value);

extern void DBG_DumpPBITEraseCnt();

extern void DBG_DumpWCMDCnt();
extern void L2_DumpEraseCnt(U8 ucMCUId);
extern void L2_DumpBlkEraseType(U8 ucMCUId);

extern void SWLRecordInit(U8 ucPu);
extern void SWLRecordGCSLC2SLC(U8 ucSuperPu);
extern void SWLRecordGCTLC2SLC(U8 ucSuperPu);
extern void SWLRecordSWLSwap(U8 ucSuperPu);
extern void SWLRecordIncHostWCnt(U8 ucSuperPu, U16 Val);
extern void SWLRecordClearHostWCnt(U8 ucSuperPu);
extern U32 SWLRecordRetHostWCnt(U8 ucSuperPu);

extern void SWLRecordIncCancelTime(U8 ucPu);
extern void SWLRecordIncDoneTime(U8 ucPu);
extern void SWLRecordIncTooCold(U8 ucSuperPU);
extern void SWLRecordIncNormal(U8 ucSuperPU);
extern void SWLRecordSWLGCCnt(U8 ucSuperPu);

extern void L2MeasureLogInit(U8 ucSuperPu);
extern U8  L2MeasureLogTargetTypeMapping(U8 ucTargetType);
extern void L2MeasureLogIncWCnt(U8 ucSuperPu, U8 ucWType);
extern void L2MeasureLogIncECTyepCnt(U8 ucSuperPu, U16 usPBN, U8 ucECType);
extern void  L2MeasureLogDumpWriteCnt(U8 ucMCUId);*/


#endif
