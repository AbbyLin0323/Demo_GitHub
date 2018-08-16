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
Filename    :L2_Boot.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.04.24
Description :defines for Boot Enrty functions.
Others      :
Modify      :
****************************************************************************/
#ifndef __L2_BOOT_H__
#define __L2_BOOT_H__

#include "BaseDef.h"
#include "L2_Defines.h"
#include "L2_PMTPage.h"
#include "L2_PMTI.h"
#include "L2_FTL.h"
#include "L2_TableBlock.h"

#define SLC_UECCCNT_ERASE_THRESHOLD 16
#define RebuildBufferCntInPU (TARGET_ALL)
#define MaxLPNBitMapSize  (MAX_LPN_IN_DISK/32 + 32)
#define MAX_ERR_NUM_OF_REBUILD (PG_PER_BLK + 100)//256

#define BOOT_STRIPE_CNT 8
#define PMT_PAGE_CNT_PER_BOOT_PU ((PMTPAGE_CNT_PER_PU / BOOT_STRIPE_CNT) + 1)

#define REBUILD_BLK_CNT_MAX_PER_PU (30)

typedef enum _BOOT_STATUS_
{
    BOOT_STATUS_BEGIN = 0,
    BOOT_PU_SELECT,
    BOOT_PU_LOAD_PMT_PAGE,
    BOOT_PU_WAIT_STATUS,
    BOOT_STATUS_DONE
}BOOT_STATUS;

typedef enum _BOOT_TYPE_
{
    BOOT_NORMAL = 0,
    BOOT_ABNORMAL
}BOOT_TYPE;

typedef enum _CALC_DIRTY_COUNT_STATUS_
{
    CALC_ONE_BLOCK_UNDONE = 0,
    CALC_ONE_BLOCK_DONE,
    CALC_ONE_ROUND_DONE,
    CALC_ALL_BLOCK_DONE
}CALC_DIRTY_COUNT_STATUS;

typedef enum _ADJUST_BLOCK_STATUS_
{
    SEQ_NOT_TARGET = 0,
    SEQ_AND_TARGET_SAME,
    SEQ_AND_TARGET_DIFF,
    RAND_NOT_TARGET,
    RAND_AND_TARGET_SAME,
    RAND_AND_TARGET_DIFF,
    ADJUST_BLOCK_STATUS_ALL
}ADJUST_BLOCK_STATUS;

typedef struct
{
    BOOT_TYPE m_BootType;
    U32 m_BootStripeSN[SUBSYSTEM_LUN_MAX];
    U32 m_BootStripeStartPMTIndex[SUBSYSTEM_LUN_MAX];
    U32 m_BootLoadPMTPagePos[SUBSYSTEM_LUN_MAX];
    U32 m_MissStripeForCmd[SUBSYSTEM_LUN_MAX][LPN_PER_BUF];
    U32 m_MissStripeCnt[SUBSYSTEM_LUN_MAX];

    U32 m_BootStripeStatus[SUBSYSTEM_LUN_MAX][BOOT_STRIPE_CNT];

    U32 m_BootUpOkOfPu[SUBSYSTEM_LUN_MAX];
    BOOT_STATUS m_BootStatus[SUBSYSTEM_LUN_MAX];
    BOOL m_DoingBootStipeFlag[SUBSYSTEM_LUN_MAX];
}BOOT_MANAGEMENT;



//Err Info Of Rebuild
typedef enum _ERR_TAG_OF_REBUILD_
{
    UNDEF_ERR = 0,
    FIRST_PAGE_ERR,
    PMT_PAGE_ERR,
    RPMT_ERR,
    TLC_RPMT_ERR,
    DATA_SPARE_ERR,
    TLC_DATA_SPARE_ERR,
    MOVE_BLK_SPARE_ERR
}ERR_TAG_OF_REBUILD;

typedef enum _RPMT_LOAD_STAGE_
{
    RPMT_LOAD_BEGIN = 0,
    RPMT_LOAD_CMD_SEND,
    RPMT_LOAD_WAIT,
    RPMT_LOAD_DONE,
    RPMT_LOAD_ALL
}RPMT_LOAD_STAGE;

typedef struct
{
    PhysicalAddr m_ErrAddr;
    ERR_TAG_OF_REBUILD m_ErrTag;
}Err_Info_OfRebuild;

typedef struct
{
    Err_Info_OfRebuild m_Err_Info[MAX_ERR_NUM_OF_REBUILD];
    U32 m_Cnt;
}Err_Info_Manager;

typedef struct
{
    RebuildPMTIState m_PMTIState;

    RED(*m_PMTSpare)[LUN_NUM_PER_SUPERPU][PG_PER_SLC_BLK*AT1_BLOCK_COUNT];

    //U32 TableBlockCnt[SUBSYSTEM_LUN_MAX];  //L3 find all table block
    U32 m_RebuildTableBlockSN[SUBSYSTEM_SUPERPU_MAX];//the BlockSN to load in next time 
    U32 m_RebuildTableBlkPPO[SUBSYSTEM_SUPERPU_MAX];//the PPO to load in next time

    BOOL m_RebuildPMTIAccomplish[SUBSYSTEM_SUPERPU_MAX];//when the Flash Read Command has been sent, set this flag
#ifdef PMT_ITEM_SIZE_REDUCE
	U32(*m_PMTPageTS)[PMTPAGE_CNT_PER_SUPERPU_MAX];   //TimeStamp of PMTPage.
#else
	U32(*m_PMTPageTS)[PMTPAGE_CNT_PER_SUPERPU];   //TimeStamp of PMTPage.
#endif

    U32 m_MaxTableTs[SUBSYSTEM_SUPERPU_MAX];
    U32 m_PMTIndexofMaxTableTS[SUBSYSTEM_SUPERPU_MAX];
    U32 m_MinTableTs[SUBSYSTEM_SUPERPU_MAX];
    U32 m_PMTIndexofMinTableTS[SUBSYSTEM_SUPERPU_MAX];
    PMTDirtyBitMap m_RebuildPMTDirtyBitMapInCE[SUBSYSTEM_SUPERPU_MAX];
    U32 m_bExistDirtyPMTPageInPU[SUBSYSTEM_SUPERPU_MAX];

    U32 m_CheckStatusPos[SUBSYSTEM_SUPERPU_MAX];
    U32 m_LastReadPos[SUBSYSTEM_SUPERPU_MAX];    
    U32 m_CurrBufferPos;
    U16 m_TableBlkSNOfBuffer[SUBSYSTEM_SUPERPU_MAX][RebuildBufferCntInPU];
    PhysicalAddr m_TableAddrOfBuffer[SUBSYSTEM_SUPERPU_MAX][RebuildBufferCntInPU][LUN_NUM_PER_SUPERPU];

#ifdef PMT_ITEM_SIZE_REDUCE
    U32(*m_PMTPageNoDirtyMaxTS)[PMTPAGE_CNT_PER_SUPERPU_MAX];
#else
	U32(*m_PMTPageNoDirtyMaxTS)[PMTPAGE_CNT_PER_SUPERPU];
#endif
    U32 m_RebuildClipTSInPu[SUBSYSTEM_SUPERPU_MAX];
    U16 m_UECCCnt[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT];
    BOOL m_NeedErase[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT];
}
RebuildPMTI;

typedef struct
{
    U32 m_TableBlk[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT][LUN_NUM_PER_SUPERPU];
    U32 m_TableBlkPPO[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT];
    U32 m_TableBlkTS[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT];
    U32 m_TableBlkFirstEmptyPage[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT][LUN_NUM_PER_SUPERPU]; //Table Blk First Empty page number 
    U32 m_TableBlklastPagePMTI[SUBSYSTEM_SUPERPU_MAX][AT1_BLOCK_COUNT][LUN_NUM_PER_SUPERPU]; //Table Blk First Empty page number 
}
RebuildPMTManager;

typedef struct
{
    //RebuildPu
    RebuildPuState m_RebuildPuState;
    RebuildPMTState m_PMTState;
    REBUILD_PMT_STAGE m_RebuildPMTStage[SUBSYSTEM_SUPERPU_MAX];

    U32(*m_DataBlock)[VIR_BLK_CNT];     // DataBlock to be rebuild.
    U32(*m_DataBlockType)[VIR_BLK_CNT];
    U32(*m_DataBlockTS)[VIR_BLK_CNT];

    //U32 m_MinPMTPageTSInPu[SUBSYSTEM_LUN_MAX];   
    U32 m_MaxTSOfPU[SUBSYSTEM_SUPERPU_MAX];// record the Max Time Stamp for strip time rebuild 

    U32 m_LoadedPMTIIndex[SUBSYSTEM_SUPERPU_MAX]; //which PMTIIndex has been loaded.

    BOOL m_UseRPMTFlag[SUBSYSTEM_SUPERPU_MAX][RebuildBufferCntInPU][LUN_NUM_PER_SUPERPU]; //Use RPMT or Spare Area    

    U8(*m_LoadSpareStatus)[RebuildBufferCntInPU][PG_PER_SLC_BLK][LUN_NUM_PER_SUPERPU];//Flash read Spare Status

    U32(*m_RebuildBlk)[TARGET_ALL][VIR_BLK_CNT];
    U32(*m_RebuildBlkTS)[TARGET_ALL][VIR_BLK_CNT];
    U32 m_RebuildBlkCnt[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];

    U16 m_CurrRebuildBlkSN[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];
    U16 m_CurrRebuildPPO[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];

    U16 m_LoadSparePPO[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL][LUN_NUM_PER_SUPERPU];//when the RPMT cannot be loaded,  load the spare area.
    BOOL m_TargetFinished[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];
    BOOL m_NeedLoadRPMT[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL][LUN_NUM_PER_SUPERPU];
    NeedRebuildFlag m_TLCBlkByPassPMTRbuild[SUBSYSTEM_SUPERPU_MAX][RebuildBufferCntInPU][LUN_NUM_PER_SUPERPU]; //Bypass PMT rebuild some loop 

    RebuildFullfillPartialPMTPPOState m_RebuildFullfillPartialPMTPPOState[SUBSYSTEM_SUPERPU_MAX];
}RebuildPMT;

typedef struct
{
    RebuildErrTLCState m_RebuildErrTLCState[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    //Need err handling tlc block
    U32 m_ErrTLCBlk[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU][MAX_ERR_TLC_NUM];
    U32 m_ErrTLCBlkCnt[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];

    //
    U32 m_CurErrTLCBlkSN[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    U32 m_CurErrTLCPageSN[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    U32 m_CurLoadDestBlkSN[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    PhysicalAddr m_LoadDestBlkAddr[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU][LPN_PER_BUF];
}RebuildErrTLC;

typedef struct
{
    BOOL m_BufferUsed[RebuildBufferCntInPU];

    RED* (*m_SpareBuffer)[TARGET_ALL-1][PG_PER_SLC_BLK][LUN_NUM_PER_SUPERPU];
#ifdef PMT_ITEM_SIZE_REDUCE
	U32* m_PageBuffer[SUBSYSTEM_SUPERPU_MAX][RebuildBufferCntInPU]; //Memory buffer  
#else
	PMTPage* m_PageBuffer[SUBSYSTEM_SUPERPU_MAX][RebuildBufferCntInPU]; //Memory buffer  
#endif

	//TLC Buffer
#ifdef PMT_ITEM_SIZE_REDUCE
	U32* m_TLCPageBuffer[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL];
#else
	PMTPage* m_TLCPageBuffer[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL];
#endif
    RED* (*m_TLCSpareBuffer)[PG_PER_SLC_BLK][PG_PER_WL][LUN_NUM_PER_SUPERPU];
    U32 m_TLCCurWordLineOffset[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    U8 (*m_BufferStatus)[PG_PER_SLC_BLK][LUN_NUM_PER_SUPERPU];//Status of Flash I/O
    U8 (*m_TLCPageBufferStatus)[PG_PER_WL][LUN_NUM_PER_SUPERPU];
    U8 (*m_TLCSpareBufferStatus)[PG_PER_SLC_BLK][PG_PER_WL][LUN_NUM_PER_SUPERPU];
    U32 m_LoadRPMTBitMap[SUBSYSTEM_SUPERPU_MAX];
}BufferManager;

#define MAXERRBLK 256
typedef struct
{
    RebuildErrHandleState m_ErrHandleState;
    U32(*m_NeedMoveBlk)[MAXERRBLK];
    U32(*m_NeedMoveBlkTargetType)[MAXERRBLK];
    U16(*m_MoveTargetBlk)[MAXERRBLK];
    U32 m_NeedMoveBlkCnt[SUBSYSTEM_SUPERPU_MAX];

    //the current move blk sn
    U32 m_ErrHandleBlockSN[SUBSYSTEM_SUPERPU_MAX];
    //the current ppo of moving blk
    U32 m_SrcBlk[SUBSYSTEM_SUPERPU_MAX];
    U32 m_DstBlk[SUBSYSTEM_SUPERPU_MAX];
    U32 m_ErrHandlePage[SUBSYSTEM_SUPERPU_MAX];
    RebuildMoveBlkStage m_MoveBlkStage[SUBSYSTEM_SUPERPU_MAX];

    BOOL m_FinishErrHandleFlag[SUBSYSTEM_SUPERPU_MAX];
    BOOL m_FinishErrHandleOneBlock[SUBSYSTEM_SUPERPU_MAX];
    U8 (* m_EraseStatus)[TARGET_HOST_ALL][LUN_NUM_PER_SUPERPU]; //one for HostWrite target erase, another is for SLCGC 

    //just for statistic Err Pages Cnt of Err Blk
    U32(*m_ErrPageCntOfNeedMoveBlk)[MAXERRBLK];

    //RED m_NextPageSpare[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    RebuildHandleTargetBlkStage m_HandleTargetBlkStage[SUBSYSTEM_SUPERPU_MAX];

    U32 m_TargetOffsetBitMap[SUBSYSTEM_SUPERPU_MAX];
}RebuildErrHandleDptr;

typedef struct
{
    RebuildCalcDirtyCntState m_CalcDirtyCntState[SUBSYSTEM_SUPERPU_MAX];
    U32(*m_NeedCalcDirtyBlk)[VIR_BLK_CNT];
    U32 m_NeedCalcDirtyBlkCnt[SUBSYSTEM_SUPERPU_MAX];
    U32 m_CurrentCalcDirtyBlkSN[SUBSYSTEM_SUPERPU_MAX];
    U16 m_CalcDirtyTLCBlkCnt[SUBSYSTEM_SUPERPU_MAX];

    /* Begin: add by henryluo for calc dirty count round by round */
    U32 m_RoundSN;
    U32 m_AccRoundSN;
    U32 m_NeedCalcDirtyBlkCntInRound;
    RebuildCalcDirtyCntStage m_RebuildDirtyCntStage;

    U16 m_CurrentCalcDirtyBlkSNInRound[SUBSYSTEM_SUPERPU_MAX];
    U16 m_CurrentCalcDirtyPage[SUBSYSTEM_SUPERPU_MAX];
    BOOL m_CalcDirtyRoundStatus[SUBSYSTEM_SUPERPU_MAX];

    ADJUST_BLOCK_STATUS m_CurrentAdjustDirtyBlkStatus[SUBSYSTEM_SUPERPU_MAX];

    U16 m_TargetBlk[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];
    U16 m_TargetPPO[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];
    U32 m_TargetOffsetBitMap[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];
    U32 m_RPMTFlushBitMap[SUBSYSTEM_SUPERPU_MAX][TARGET_ALL];
    U32 m_TimeStamp[SUBSYSTEM_SUPERPU_MAX];
    /* End: add by henryluo for calc dirty count round by round */

    //Read RPMT 
    RebuildReadRPMTState m_ReadRPMTState[SUBSYSTEM_SUPERPU_MAX];
    RebuildLoadSpareState m_LoadSpareState[SUBSYSTEM_SUPERPU_MAX];
    BOOL m_UseRPMTFlag[SUBSYSTEM_SUPERPU_MAX];
    BOOL m_TLCUseRPMTFlag[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL]; //Use RPMT or Spare Area of TLC Write Target

    //Read All Page Spare
    U32 m_LoadSparePPO[SUBSYSTEM_SUPERPU_MAX];

    //For rebuild trim
#ifdef PMT_ITEM_SIZE_REDUCE
    U32(*m_bValidBitMap[SUBSYSTEM_SUPERPU_MAX])[PMTPAGE_VALID_LPN_MAP_SIZE_MAX];
    U32(*m_ValidLPNCountCalc)[PMTPAGE_CNT_PER_PU_MAX];
#else
	U32(*m_bValidBitMap)[PMTPAGE_CNT_PER_PU][PMTPAGE_VALID_LPN_MAP_SIZE];

	U32(*m_ValidLPNCountCalc)[PMTPAGE_CNT_PER_PU];
#endif

    BOOL m_NeedRebuitDirtyCntFlag[SUBSYSTEM_SUPERPU_MAX];
    BOOL m_CanDoFTLWrite;

}RebuildCalcDirtyCntDptr;


//maybe some PMT block is in GC, so there maybe more block than PMTBlocCntPerCE
typedef struct
{
    //curr state
    RebuildState m_State;
    RebuildPMTI  m_RebuildPMTI;

    RebuildPMTManager  m_RebuildPMTManager;

    RebuildPMT  m_RebuildPMT;

    RebuildErrTLC m_RebuildErrTLC;

    BufferManager m_BufManager;

    Err_Info_Manager m_ErrManager;
    RebuildErrHandleDptr m_ErrHanler;
    RebuildCalcDirtyCntDptr m_CalcDirtyCntDptr;
}RebuildDptr;

extern GLOBAL  RebuildDptr* l_RebuildDptr;
extern GLOBAL  RebuildPMTI* l_RebuildPMTIDptr;
extern GLOBAL  RebuildPMTManager* l_RebuildPMTManagerDptr;
extern GLOBAL  RebuildPMT* l_RebuildPMTDptr;
extern GLOBAL  BufferManager* l_BufferManagerDptr;
extern GLOBAL  RebuildErrHandleDptr* l_ErrHandleDptr;
extern GLOBAL  BOOL g_RebuildDirtyFlag;
extern GLOBAL  BOOT_MANAGEMENT* g_BootManagement;
extern GLOBAL  BOOL g_BootUpOk;
extern GLOBAL  U32  g_RebuildPMTDone;
extern GLOBAL  U32  g_ulAllowHostWriteSec;

void L2_InitRebuildDptr();
RebuildState L2_GetRebuildState();
void L2_InitRebuild();
void L2_RebuildPrepare();
void L2_RebuildPMTI();
void L2_RebuildEntry();
//Err Handling API of Table Rebuild
void  L2_InitErrManagerOfRebuild();
void  L2_CollectErrInfoOfRebuild(U8 ucSuperPU, U8 ucLUNInSuperPU, U16 Blk, U16 Page, ERR_TAG_OF_REBUILD ErrTag);
void  L2_RebuildFlushManager();
void  L2_SetDirtyCntInRebuildOfCE(U32 PUSer);
void  L2_RebuildDirtyCntByPMTPage(U32 PUSer, U32 PMTIIndexInPu);
void  L2_RebuildPuTimeStamp();
void L2_InitErrHandleOfRebuild();
RebuildErrHandleState  L2_RebuildGetErrHandleState();
void  L2_RebuildSetErrHandleStateState(RebuildErrHandleState State);
void L2_AddErrHandleBlk(U32 PUSer, U32 Blk, U32 uTarget);
void  L2_DumpErrHandleMoveBlkInfo();
void  L2_PrepareErrHanle();
BOOL  L2_IsErrHandleFinish(U32 PUSer);
U32 L2_RebuildGetErrHandleSrcBlk(U32 ulSuperPU, U32* pTargetType);
U32 L2_RebuildGetErrHandleSrcPage(U32 PUSer);
U32 L2_RebuildIncErrHandleSrcPage(U32 PUSer);
U16 L2_RebuildGetErrHandleDstBlk(U32 PUSer);
BOOL  L2_RebuildMovePage(U32 PUSer, U32 SrcBlk, U32 DstBlk, U16 Page);
BOOL  L2_RebuildErrHandleAllCE();
BOOL  L2_RebuildHandleRndTarget();
void  L2_InitCalcDirtyCntOfRebuild();
U32 L2_GetL2FreePage(void);

BOOL L2_IsAllTablePuLoadOk();
BOOL L2_IsRebuildPMTPageDirty(U32 PUSer, U32 PMTIndexInCE);
BOOL LoadRPMTToBuffer(U32 LoadBlockSN);
BOOL L2_RebuildIsSuperPMTPageBeTarget(U32 uSuperPU,U16 *Blk, U16 page, U32 * uPMTBitMap);
BOOL L2_FindErrPMTPageOfRebuild(U32 PU, U16 BlkSN, U16 Page, BOOL *bMayBeTarget);
BOOL L2_FindErrPMTCEOfRebuild(U8 ucSuperPU, U8 ucLUNInSuperPU);
BOOL L2_IsRebuildDirtyCntFinish();
BOOL L2_AdjustDirtyCnt(U32 PUSer);
BOOL L2_RebuildIsTargetBlk(U32 PUSer, U32 Blk, U8* pucTargetType);
BOOL L2_IsTargetBlk(U32 PUSer, U32 Blk, U8* pucTargetType);

void L2_LoadSpare(PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, BOOL bSLCMode);
void L2_TableLoadSpare(PhysicalAddr* pAddr, U8* pStatus, U32* pSpare);
void L2_ClearRebuildDirtyCntFlagOfCE(U32 PUSer);
void L2_SetCanDoFTLWriteFlag();

extern BOOL L2_IsBootStripeNeedLoad(U32* pLPN, U32 uLPNOffset, U32 uLPNCnt);
extern BOOL L2_IsNeedRebuildDirtyCnt(U32 PUSer);
extern void L2_RebuildSetDRAMForPMTBuffer(U16 PUSer, U16 RebuildBufferSN, U32* pBufferAddr);
extern void L2_RebuildSetDRAMforSpare(U16 PUSer, U16 BlockSN, U32* pBufferAddr);
extern void L2_RebuildSetDRAMForSpareBuffer(U16 PUSer, U16 RebuildBufferSN, U32* pBufferAddr);
extern void L2_ClearRebuildDirtyCntFlag();
extern void L2_BootManagementInit();
extern void L2_RebuildPMTManager();
extern RebuildPMTState L2_RebuildGetPMTState();
extern void L2_PrepareToRebuildPMT(U32 PUSer);
extern BOOL L2_RebuildLoadPMT();
extern void L2_LoadFirstRPMT();
extern void L2_RebuildPMT();
extern void L2_SetRebuildState(RebuildState State);
extern void L2_SetupRebuildDrityCntFlag();
extern BOOL L2_BootEntry(U8 ucPuNum);
extern BOOL L2_FindErrPMTPageOfRebuild(U32 PU, U16 BlkSN, U16 Page, BOOL *bHadWrite);
extern CALC_DIRTY_COUNT_STATUS L2_RebuildDirtyCnt(U32 PUSer);
extern BOOL L2_RebuildLookupRPMTBuffer(PhysicalAddr* pAddr, U32 ulLPN);

extern void L2_NormalBootResumeInfo();
extern BOOL L2_IsAdjustDirtyCntDone();
extern BOOL L2_IsBootupOK();
extern U32  L2_Rebuild_GetFirstUsedLUNOfSuperPage(U32 uSuperPU, U32 uVirtualBlock, U32 uPageNum);
extern U32  L2_TB_Rebuild_TableBlockErase(U8 ucSuperPuNum);
extern void L2_TB_Rebuild_TableEraseResetParam(U8 ucSuperPuNum);

#ifdef SIM
extern void Dump_RebuildErrInfoOfRebuild();
#endif
BOOL L2_IsPMTNeedRebuild(U32 PUSer);
void ResetRebuildPMTStates(U32  PUSer);
void L2_RebuildSetPMTState(RebuildPMTState State);

BOOL L2_IsHavePMTErrOfRebuild(U32 PUSer);
BOOL L2_IsHavePMTErr(PhysicalAddr* pAddr);
BOOL L2_IsAllTablePuLoadOkOfAllPu();
U32 L2_AbnormalBootEntry();
BOOL L2_RebuildErrTLCOfAllPu();
BOOL L2_FindBlkOfErrTLCRPMTC(U32 ulSuperPU, U32 ulLUNInSuperPU, U16 Blk );

#endif
