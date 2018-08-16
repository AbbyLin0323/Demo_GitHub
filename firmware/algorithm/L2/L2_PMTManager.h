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
Filename    :L2_PMTManager.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.29
Description :defines for PMTManager Structure.
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_PMT_MANAGER_H__
#define __L2_PMT_MANAGER_H__

#include "L2_Defines.h"
#include "L2_PMTPage.h"
#include "L2_PMTI.h"

#define TableWLThresholdValue       40
#define RationOfTableFlush2TableWL  8

typedef struct _PMTBlkInfo_Tag  
{
    BOOL   m_bFree;
    U16    m_DirtyCnt;   
}PMTBlkInfo;

typedef struct PMTBlkManager
{
     U16      m_CurPPO;
     U16      m_CurBlkSN;
     U32      m_FreePagesCnt;
     
     PMTBlkInfo m_PMTBlkInfo[AT1_BLOCK_COUNT];     

}PMTBlkManager;

typedef enum _L2_PMT_SAVE_STAGE
{
    L2_PMT_SAVE_PREPARE,
    L2_PMT_SAVE_WRITE_PAGE,
    L2_PMT_SAVE_WAIT_WRITE_PAGE,
    L2_PMT_SAVE_FAIL_MOVE_BLOCK,
    L2_PMT_SAVE_SUCCESS,
    L2_PMT_SAVE_DONE,
}L2_PMT_SAVE_STAGE;

typedef enum _L2_SAVE_STATUS
{
    L2_SAVE_SUCCESS,
    L2_SAVE_PENDING,
    L2_SAVE_FAIL,
}L2_SAVE_STATUS;

typedef struct PMTManager_Tag 
{
    PMTBlkManager* m_PMTBlkManager;
 
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* (*m_pPMTPage)[PMTPAGE_CNT_PER_SUPERPU_MAX];
    RED* (*m_PMTSpareBuffer)[PMTPAGE_CNT_PER_SUPERPU_MAX];

    //U32 m_ValidLPNCountSave[PMTPAGE_CNT_TOTAL];
    //The Address of the PMT Pages which in RAM.

    U8(*m_FlushStatus)[LUN_NUM_PER_SUPERPU][PMTPAGE_CNT_PER_SUPERPU_MAX];
#else
    PMTPage* (*m_pPMTPage)[PMTPAGE_CNT_PER_SUPERPU];
    RED* (*m_PMTSpareBuffer)[PMTPAGE_CNT_PER_SUPERPU];

    //U32 m_ValidLPNCountSave[PMTPAGE_CNT_TOTAL];
    //The Address of the PMT Pages which in RAM.

    U8(*m_FlushStatus)[LUN_NUM_PER_SUPERPU][PMTPAGE_CNT_PER_SUPERPU];
#endif
    //Flush Status, before flush into the Flash, set to 1
    //when the program is accomplished, L3 will set it to 0.

    U32 m_PMTBitMap[SUBSYSTEM_SUPERPU_MAX];
    //add Load/Flush PMT BitMap for super page,0:not load/flush,1:has been loaded/flushed
    U32 m_PMTBitMapSPOR[SUBSYSTEM_SUPERPU_MAX];// avoid being overlapped when load PMT during rebuild.

    U32 m_CurFlushPMTIndex[SUBSYSTEM_SUPERPU_MAX];
    L2_PMT_SAVE_STAGE m_PMTSaveStage[SUBSYSTEM_SUPERPU_MAX];

}PMTManager;


#define TABLE_RW_PAGE_CNT   2

typedef struct TableGCInfo_tag
{
    U16 m_BlockSN;
    U16 m_OriginPBN[LUN_NUM_PER_SUPERPU];

    U32 m_PMTPageInPu;
} TableGCInfo;

typedef struct TableRWManager_Tag
{
    TableGCInfo m_TableWLInfo;
    TableGCInfo m_TableGCInfo;
    TableRWType m_TableRWType;
    TableRWStatus m_TableStatus;
    U16 m_PMTIIndex[TABLE_RW_PAGE_CNT];
    U16 m_PMTIStatus[TABLE_RW_PAGE_CNT];
    U16 m_TableRWCnt;
    U32 m_TableFlushCntStatistic;    
    U32 m_TableWLCntStatistic;    
} TableRWManager;

extern GLOBAL  BOOL g_bPMTErrFlag;
extern GLOBAL  PMTManager* g_PMTManager;
extern GLOBAL  TableRWManager *g_TableRW;
extern GLOBAL  U32 g_FindTableGCPos[SUBSYSTEM_SUPERPU_MAX];

#ifdef ValidLPNCountSave_IN_DSRAM1
extern GLOBAL  U16* g_pValidLPNCountSaveL;
extern GLOBAL  U8*  g_pValidLPNCountSaveH;

extern INLINE U32 U24getValue(U8 x, U32 y);
extern INLINE void U24setValue(U8 x, U32 y, U32 value);
extern INLINE void U24incOne(U8 x, U32 y);
extern INLINE void U24decOne(U8 x, U32 y);
#endif

BOOL L2_TableEntry(U8 ucSuperPu);
void L2_InitPMTManager();
void L2_LLFPMT();
#ifdef PMT_ITEM_SIZE_REDUCE
U32* GetPMTPage(U32 LPN);
#else
PMTPage* GetPMTPage(U32 LPN);
#endif 
LPNLookUPQuitCode L2_LookupPMT(PhysicalAddr* pAddr, U32 LPN, BOOL bRebuild);
extern LPNLookUPQuitCode L2_UpdatePMT(PhysicalAddr* pNewAddr, PhysicalAddr* pOriginalAddr, U32 ulLpn);
BOOL L2_LoadPMTPage(U8 ucSuperPu, U16 PMTIIndexInPu);
BOOL L2_FlushPMTPage(U32 ucSuperPu,U16 PMTIIndexInPu);
U16 L2_GetBlockSNOfPMTPage(U32 ucSuperPu,U16 PMTIIndexInPu);
extern BOOL L2_SetTableRW(U32 ucSuperPu,U16* PMTIIndex, U16 TableRWCnt, TableRWType TableRWType);
TableQuitStatus L2_TablePreProcess(U8 ucSuperPu);
TableQuitStatus L2_TableGC(); 
TableQuitStatus L2_TableWaitStatus();
U32 L2_CheckFlushPMTStatus(U8 ucSuperPu, U16 PMTIndexInSuperPu);
void L2_ResetSavePMTStage();

/////////////////////////////////////////////////////////
//                 New Table GC delcation start  
/////////////////////////////////////////////////////////
void L2_InitPMTBlkManager();
U16 L2_GetCurBlockSNOfPMT(U8 ucSuperPu);
U16 L2_GetCurPPOOfPMT(U8 ucSuperPu);
U16 L2_GetPBNOfBlockSN(U8 ucSuperPu,U16 BlockSN);
U32 L2_GetFreePagesCntofPMT(U8 ucSuperPu);
BOOL L2_IncCurPOOfPMT(U8 ucSuperPu);
U16 L2_FindFreeBlockofPMT(U8 ucSuperPu);
U16 L2_GetPBNOfPMTBlockSN(U8 ucSuperPu, U16 BlockSN);
U16 L2_GetBlockSNOfPMTPBN(U8 ucSuperPu, U8 ucLunInSuperPu,U16 BlockInCE);
U16 L2_GetDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN);
void L2_IncDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN);
void L2_ClearDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN);
void L2_UpdatePhysicalAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 uPMTIIndexInPu, PhysicalAddr* pAddr);
U16 L2_FindGCBlockSNOfPMT(U8 ucSuperPu);
U16 L2_FindValidPPOOfPMT(U16* pPMTIndexOfPgs);
void L2_ClearPMTIndexOfPgs(U16* pPMTIndexOfPgs);
void L2_ResetTableManagerByInfo();
void L2_Init_before_rebuild();
BOOL L2_IsNeedTableWL(U32 ucSuperPu);
TableQuitStatus L2_TableWL(U8 ucSuperPu, BOOL *SendCmd);
U16 L2_GetTableWLSourceBlk(U32 ucSuperPu);
void L2_SetTableWLInfo(U32 ucSuperPu);
extern BOOL L2_IsPMTTableGC(U8 ucPuNum);
#endif
