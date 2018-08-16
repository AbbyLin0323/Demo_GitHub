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
Filename    :L2_GCManager.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.29
Description :defines for GCManager Structure.
Others      :
Modify      :
****************************************************************************/
#ifndef __L2_GC_MANAGER_H__
#define __L2_GC_MANAGER_H__
#include "L2_Defines.h"
#include "L2_StripeInfo.h"
#include "L2_PMTPage.h"
#include "L2_RPMT.h"
#include "L2_ReadDisturb.h"

#define B_VALID     1
#define B_DIRTY     !(B_VALID)

#define GC_BUFFER_DEPTH            2
#define TLCGC_RPMT_RECORD_DEPTH    3
#define GC_RPMT_RECORD_DEPTH (max(GC_BUFFER_DEPTH, TLCGC_RPMT_RECORD_DEPTH))

typedef enum _GC_TYPE_MODE
{
    SLCGC_MODE = 0,
    TLCGC_MODE = 1,
    GC_SRC_MODE //GC_MODE_NUM
}GC_TYPE_MODE;

typedef enum PopStage_Tag
{
    POP_BLOCK_DONE = 0,
    POP_TABLE_MISS,
    POP_ZERO_LPN,
    POP_ONE_LPN,
    POP_MULTI_LPN,
    POP_NOT_ALLOWED,
    POP_STAGE_DONE
}PopStage;

#if (LPN_PER_BUF == 8)
/*Dirty Page Bitmap table*/
typedef struct _DPBM_ENTRY
{
    U8 m_LpnMapPerPage[LUN_NUM_PER_SUPERPU][(PG_PER_SLC_BLK * PG_PER_WL)];
} DPBM_ENTRY;
#elif (LPN_PER_BUF == 16)
typedef struct _DPBM_ENTRY
{
    U16 m_LpnMapPerPage[LUN_NUM_PER_SUPERPU][PG_PER_SLC_BLK * PG_PER_WL];
} DPBM_ENTRY;
#else
typedef struct _DPBM_ENTRY
{
    U8 m_LpnMapPerPage[LUN_NUM_PER_SUPERPU][(PG_PER_SLC_BLK * PG_PER_WL)];
} DPBM_ENTRY;

#endif

typedef struct _DPBM
{
    DPBM_ENTRY(*m_LpnMap)[VIR_BLK_CNT];
} DPBM;

extern GLOBAL DPBM *pDPBM;

#define DPBM_SUPERPAGE_SIZE_PER_SUPERPU     ((sizeof(DPBM_ENTRY))*VIR_BLK_CNT)
#define DPBM_SUPERPAGE_COUNT_PER_SUPERPU    ((DPBM_SUPERPAGE_SIZE_PER_SUPERPU % SUPER_PAGE_SIZE) ? (DPBM_SUPERPAGE_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE + 1) : (DPBM_SUPERPAGE_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE))

#define GC_ERASE_BLK_BUF_CNT_MAX  ((PAIR_PAGE_INTERVAL_MAX + 1) * LUN_NUM_PER_SUPERPU)
#define TLCGC_SRCBLK_MAX (TLC_BLK_CNT)

typedef struct GCComStruct_Tag
{
    GCState m_GCStage[SUBSYSTEM_SUPERPU_MAX];

    U8 m_ucSrcBlkType[SUBSYSTEM_SUPERPU_MAX];

    //Up page of extra page state read offset, up page should read again from table after copy to extra page.
    U16 m_GCUpOfExtraReadOffset[SUBSYSTEM_SUPERPU_MAX];

    U16 m_GCReadOffset[SUBSYSTEM_SUPERPU_MAX];
    //tobey modify LPNInBuffer and AddrForBuffer number
    //for SLCGC: 2 buffer pingpong use, 1 more buffer is used to temp save extra valid data's LPN and Addr.ADDR;
    //for TLCGC: 3 group of WL_buffer circle use, 
    U32 m_LPNInBuffer[SUBSYSTEM_SUPERPU_MAX][GC_RPMT_RECORD_DEPTH][LPN_PER_SUPERBUF];
    PhysicalAddr m_AddrForBuffer[SUBSYSTEM_SUPERPU_MAX][GC_BUFFER_DEPTH][LPN_PER_SUPERBUF];

    U16 m_SrcPBN[SUBSYSTEM_SUPERPU_MAX];
    U16 m_SrcWLO[SUBSYSTEM_SUPERPU_MAX];
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    U16 m_SrcWLO_Extra[SUBSYSTEM_SUPERPU_MAX];
#endif

    // m_SrcPrefix is the page offset within a word line
    U16 m_SrcPrefix[SUBSYSTEM_SUPERPU_MAX];

    U16 m_SrcOffset[SUBSYSTEM_SUPERPU_MAX];    
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    U16 m_SrcOffset_Extra[SUBSYSTEM_SUPERPU_MAX];
#endif    
    
    //record the LPN offset in the physical page
    //from 0 to 3 in the configuration of 4K LPN in 16K Physical Page

    U8 (*m_FlushStatus)[PG_PER_WL + 1][LPN_PER_SUPERBUF];
    U8 (*m_GCWriteStatus)[LUN_NUM_PER_SUPERPU];
    U8 (*m_RPMTStatus)[LUN_NUM_PER_SUPERPU][PG_PER_WL];

    RPMT* m_pRPMT[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL];

    // m_ucRPMTNum specifies the next RPMT we should load, we need this variable
    // since for a TLC block there are 3 RPMTs, we need to know which one to load next
    U8 m_ucRPMTNum[SUBSYSTEM_SUPERPU_MAX];

    U8 m_GCBufferPointer[SUBSYSTEM_SUPERPU_MAX];
    //the GC Buffer ID of the data page to be written into flash

    U32 m_LoadRPMTBitMap[SUBSYSTEM_SUPERPU_MAX];
    //add load RPMT bit map for support super page

    SuperPage* m_GCBuffer[SUBSYSTEM_SUPERPU_MAX][GC_BUFFER_DEPTH];
    //the GC Buffer to store the data page


    //add for super page,0:not read/write,1:has been read/written
    U32 m_FinishBitMap[SUBSYSTEM_SUPERPU_MAX];

    //TLC UECC error handling
    TLCErrH m_ErrStage[SUBSYSTEM_SUPERPU_MAX];
    U8 m_ErrLun[SUBSYSTEM_SUPERPU_MAX];
    BOOL m_GoodLunFinish[SUBSYSTEM_SUPERPU_MAX];

    // add for record which LUN fail, 0: not fail, 1: fail
    U32 m_FailLunBitMap[SUBSYSTEM_SUPERPU_MAX];

    // TLCGC ReadFail Record
    U16 m_ReadStatusLocation[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL + 1][LUN_NUM_PER_SUPERPU];
    U8 m_LastReadLen[SUBSYSTEM_SUPERPU_MAX];

	/* if TLCGC exhaust too many source block, over 4/5 of TLC blocks, then dummy write is used to close TLCGC target block */
    BOOL m_TLCGCDummyWrite[SUBSYSTEM_SUPERPU_MAX];

    //add for super page, SLC GC Lun bit 0:not written,1:has been written
    U32 m_SLCGCWrittenBitMap[SUBSYSTEM_SUPERPU_MAX];

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    BOOL m_TLCGCNeedtoWaitforRPMT[SUBSYSTEM_SUPERPU_MAX];
#endif	
	
}GCComStruct;

typedef struct GCManager_Tag
{
    GCComStruct tGCCommon;

    U32 m_DirtyLpnCnt[SUBSYSTEM_SUPERPU_MAX];
    S32 m_FreePageCnt[SUBSYSTEM_SUPERPU_MAX];
    S32 m_FreePageForHost[SUBSYSTEM_SUPERPU_MAX];

    U16 m_NeedCopyForOneWrite[SUBSYSTEM_SUPERPU_MAX];
    U16 m_HostWriteRealPage[SUBSYSTEM_SUPERPU_MAX];

    //TargetBlkGCState m_TargetBlkGCState[SUBSYSTEM_SUPERPU_MAX];
    U16 m_DstPBN[SUBSYSTEM_SUPERPU_MAX];
    RED m_Spare[SUBSYSTEM_SUPERPU_MAX];
    U32 m_TargetBlkStatusBitmap[SUBSYSTEM_SUPERPU_MAX];
}GCManager;

typedef struct _TLCGCSrcBlkRecord_tag
{
    U16 m_TLCGCAllDirtySrcBlkCnt[SUBSYSTEM_SUPERPU_MAX];
    U16 l_aTLCGCSrcBlk[SUBSYSTEM_SUPERPU_MAX][TLCGC_SRCBLK_MAX];
    U32 m_TLCGC1stSrcBlkTS[SUBSYSTEM_SUPERPU_MAX];
    BOOL bIsSrcBlkAllValid[SUBSYSTEM_SUPERPU_MAX];
    U32 l_aTLC1stGCSrcBlk[SUBSYSTEM_SUPERPU_MAX]; // used to judge whether src blk is full of valid data or not
    U32 m_ulSrcBlkMaxSN[SUBSYSTEM_SUPERPU_MAX]; // used to record the maximum SN of all TLCGC src blks
#ifdef SIM
    U32 m_CopyValidLPNCnt[SUBSYSTEM_SUPERPU_MAX][TLCGC_SRCBLK_MAX];
    U32 m_DirtyCnt[SUBSYSTEM_SUPERPU_MAX][TLCGC_SRCBLK_MAX];
    U32 m_NewAddDirtyCnt[SUBSYSTEM_SUPERPU_MAX][TLCGC_SRCBLK_MAX];
    U32 m_CopyDummyLPNCnt[SUBSYSTEM_SUPERPU_MAX];
#endif
}TLCGCSrcBlkRecord;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
typedef struct _TLCGCSrcBlkRecordStage2_tag
{
    U16 m_TLCGCAllDirtySrcBlkCnt[SUBSYSTEM_SUPERPU_MAX];
    U16 l_aTLCGCSrcBlk[SUBSYSTEM_SUPERPU_MAX][TLCGC_SRCBLK_MAX];
    U32 m_TLCGC1stSrcBlkTS[SUBSYSTEM_SUPERPU_MAX];
    BOOL bIsSrcBlkAllValid[SUBSYSTEM_SUPERPU_MAX];
    U32 l_aTLC1stGCSrcBlk[SUBSYSTEM_SUPERPU_MAX]; // used to judge whether src blk is full of valid data or not
    U32 m_ulSrcBlkMaxSN[SUBSYSTEM_SUPERPU_MAX]; // used to record the maximum SN of all TLCGC src blks
}TLCGCSrcBlkRecordStage2;
#endif

extern GLOBAL  GCManager* g_GCManager[GC_SRC_MODE];
extern GLOBAL  BOOL gbGCTraceRec;
void L2_SetRefGCBlock(U16 PuID);
BOOL L2_GCEntry(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucThreadType);
void L2_InitGCManager(U8 ucSuperPu, U8 ucGCMode);
void L2_LLFGCManager(void);
void L2_UpdateDirtyLpnMap(PhysicalAddr* pAddr, BOOL bValid);
BOOL L2_LookupDirtyLpnMap(PhysicalAddr* pAddr);
void L2_LoadRPMTInLun(RPMT* pRPMT, U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8* pStatus, BOOL bSLCMode, U8 ucRPMTO);
void L2_LoadRPMT(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucRPMTnum, U16 usVBN, U8 ucTarget, BOOL bTLCGC);
void L2_InitCommonInfo(U8 ucSuperPu, GCComStruct *ptCom);

extern BOOL L2_IsGCEraseBlkBufQFull(U8 ucSuperPu);
extern void L2_InitGCEraseBufQ(U8 ucSuperPu);
extern BOOL L2_PushGCEraseBlkBufQ(U8 ucSuperPu, U16 usVirBlk, U16 usRndBlk, U16 usRndPPO);

BOOL L2_ProcessGC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucThreadType);

void SetSrcBlock(U8 ucSuperPu, U16 BlockSN);
FTL_THREAD_TYPE SelectSLCGCBlock(U8 ucSuperPu, U8 ucSrcVBTType);
U32 L2_GetSLCAreaFreePageCnt(U8 ucSuperPu);
#endif
