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
Filename    :L2_StripeInfo.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.28
Description :defines for Pu Data Structure.
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_STRIPEINFO_H__
#define __L2_STRIPEINFO_H__
#include "L2_Defines.h"
#include "L2_RPMT.h"
#include "L2_Erase.h"

#define RPMT_BUFFER_DEPTH   2

#define TARGET_BLOCK_CNT            3
#define ERROR_HANDLE_BLOCK_CNT      1
#define DELAY_ERASE_BLOCK_CNT       (Rsv_EraseQueueSize)
/* threshold block-level */

#ifdef SWL_CPV_OFF
#define WL_SLC_BLOCK_CNT    0
#else
#define WL_SLC_BLOCK_CNT    6
#endif

#define TLCSWL_BLOCK_CNT        2
#define TLCMERGE_NEED_CUSHION   3
#define TLCMERGE_QUOTA_ENSURE   4
#define TLCGC_SELSRC_CUSHION    1
#define TLCAREA_TARGET          1
#define RESERVED_BLOCK_CNT        (ERROR_HANDLE_BLOCK_CNT + TLCMERGE_NEED_CUSHION)//(ERROR_HANDLE:1; TLCMERGE_NEED:3;)
#define GC_THRESHOLD_BLOCK_CNT    (RESERVED_BLOCK_CNT + TLCMERGE_QUOTA_ENSURE + TLCGC_SELSRC_CUSHION)  // SLCArea_Target:2/ make sure TLCMERGER:HostWrite = 1:1;
#define GC_THRESHOLD_TLCNORMAL    (ERROR_HANDLE_BLOCK_CNT + TLCSWL_BLOCK_CNT + TLCAREA_TARGET)//4//(ERROR_HANDLE:1; TLCMERGE_NEED/TLCGC_NEED/TLCSWL_Need:2(TLCSWL may buffer one); TLCArea_Target:1) (2 + ERROR_HANDLE_BLOCK_CNT + WL_TLC_BLOCK_CNT)

#define IDLEGC_THRESHOLD_BLOCK_CNT   (5)
#define REBUILD_MOVE_BLKCNT          (TARGET_HOST_ALL)

/* threshold Page-level */
#define RESERVED_PAGE_CNT    (RESERVED_BLOCK_CNT * PG_PER_SLC_BLK)
#define GC_THRESHOLD_PAGE_CNT    (GC_THRESHOLD_BLOCK_CNT * PG_PER_SLC_BLK)

typedef struct PuInfo_Tag
{
    U32 m_TimeStamp;
    //the time stamp is seperate for each Pu.

    U8 m_TargetOffsetTS[TARGET_ALL][LUN_NUM_PER_SUPERPU];
    //LUN TS of one SuperPu

    U16 m_DataBlockCnt[BLKTYPE_ALL];
    U16 m_AllocateBlockCnt[BLKTYPE_ALL];
    //means the count that has been allocated for the Pu in each PU

    U32 m_SLCMLCFreePageCnt;

    // Pu info target block number: SEQ/Rand/SLC
    U16 m_TargetBlk[TARGET_ALL];

    // Pu info target block number: SEQ/Rand/SLC
    U16 m_TargetPPO[TARGET_ALL];

    U32 m_TargetOffsetBitMap[TARGET_ALL];
    //Lun write bitmap of one SuperPu,0:not write,1:has written

    RPMT* m_pRPMT[TARGET_ALL][RPMT_BUFFER_DEPTH];
    //record the RPMT of current writing block

    U8 m_RPMTBufferStatus[TARGET_ALL][RPMT_BUFFER_DEPTH][LUN_NUM_PER_SUPERPU];
    //when Flush RPMT into Flash, firmware will set the Status to 1,
    //if the RAM is ok, L3 will reset the status to 0.

    U32 m_RPMTFlushBitMap[TARGET_ALL];
    //add for super page,0:not flush,1:has been flushed

    U32 m_RPMTBufferPointer[TARGET_ALL];


    U16 m_SLCEndPhyBlk[LUN_NUM_PER_SUPERPU];
    U16 m_TLCStartPhyBlk[LUN_NUM_PER_SUPERPU];

}PuInfo;

extern GLOBAL MCU12_VAR_ATTR PuInfo* g_PuInfo[SUBSYSTEM_SUPERPU_MAX];

U32 L2_GetLPNInCEFromLPN(U32 LPN);
BOOL L2_FlushCurrRPMT(U8 ucSuperPu, TargetType FlushType);
U32 L2_AllocateOnePage(U8 ucSuperPu, U32* LPN, TargetType Type, BLOCK_TYPE* pActualType);
void L2_InitPuInfo(U8 ucSuperPu);
void L2_LLFPuInfo(U8 ucSuperPu);
BOOL L2_IsNeedSLCGC(U8 ucSuperPu);
BOOL L2_IsNeedTLCGC(U8 ucSuperPu);
BOOL L2_IsTLCFreeBlkEnough(U8 ucSuperPu);
U32 L2_GetFreePageCntInPu(U8 ucSuperPu);
U32 L2_GetDirtyCnt(U16 PUSer, U16 VBN);
U16 L2_SelectGCBlock(U8 ucSuperPu, U8 ucSrcVBTType);

void L2_RecycleBlock(U8 PUSer, U16 BlockSN);
void L2_IncreaseDirty(U16 CESerInDisk, U16 VirtualBlockSN, U32 DirtyCnt);
U32 L2_GetTimeStampInPu(U8 ucSuperPu);
void L2_IncTimeStampInPu(U8 ucSuperPu);
void L2_UpdateTSInPuByRebuild(U8 ucSuperPu, U32 NewTS);
void L3_SetPuIDInVBT(U32 PUSer, U32 VBN, U32 PuID);
void L3_SetDirtyCntInVBT(U32 PUSer, U32 VBN, U32 DirtyLPNCnt);
BOOL L2_IsNeedTrimLastDefenseMechanism(U8 ucSuperPu);

extern U8 L2_GetSuperPuFromLPN(U32 LPN);
extern U16 L2_GetPuTargetVBN(U8 ucPU, TargetType eTarget);
extern U16 L2_GetPuTargetPPO(U8 ucPU, TargetType eTarget);
extern void L2_SetUninitalPuInRebuild();

extern void L2_UpdateResCntByRebuild(U8 ucSuperPu);
extern void L2_RebuildPuInfo(void);
extern U8 L2_GetTargetOffsetTS(U32* pBitMap);
extern U8 L2_GetRPMTOffsetTS(U8 ucSuperPu);
extern void L2_CheckDirtyCnt(U8 ucPU, U16 usVBN);
void L2_RebuildLoadRPMTInPu(RPMT* pRPMT, U16 CESerInDisk, U16 PBN, U8* pStatus);
extern U16 L2_SelectGCSrcBlkMinSN(U8 ucSuperPu, U8 ucTLCBlk, U8 ucVBTType);
extern BOOL L2_IsTLCNeedGC(U8 ucSUperPu, U8 ucTLCGCTH);
extern BOOL L2_IsNeedIdleSLCGC(U8 ucSuperPu);
#endif
