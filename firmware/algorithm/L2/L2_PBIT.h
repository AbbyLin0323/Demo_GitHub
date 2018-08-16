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
File Name     : L2_PBIT.c
Version       : Initial version
Author        : henryluo
Created       : 2015/2/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2012/01/07
Author      : peterxiu
Modification: Created file

*******************************************************************************/
#ifndef _L2_PBIT_H
#define _L2_PBIT_H

#include "L2_Defines.h"
#include "L2_VBT.h"
#include "L2_GCManager.h"

#define THRES_ERASE_CNT     (TLC_BLK_CNT * LUN_NUM_PER_SUPERPU)

/*Physics block information table*/
typedef union _PBIT_ENTRY
{
    U32 Value[2];
    struct
    {
        U32 VirtualBlockAddr : 16;
        U32 EraseCnt : 16;

        U32 bFree : 1;
        U32 bError : 1;
        U32 bAllocated : 1;
        U32 bBackup : 1;        //should be recovery when full revovery
        U32 bTable : 1;
        U32 BlockType : 4;
        U32 bPatrolRead:1;
        U32 bWL : 1;
        U32 bLock : 1;
        U32 bTLC : 1;           // 1: TLC, 0:SLC
        U32 bBroken : 1;
        U32 bWeak : 1;  // Weak block, marked by l3 when normal read recc or retry-success, checked by l2 when petrol-read or gc-src-block-selection.- added by jasonguo 20150817
        U32 bRetryFail : 1;
        U32 bReserved: 1;
        
        #ifndef L2MEASURE
        U32 bRsv : 15;
        #else
        U32 ECPerPOR: 15;
        #endif
    };
}PBIT_ENTRY;


typedef union _PBIT_FIRST_PAGE_INFO
{
    U32 Value[5];
    struct
    {
        /* page type */
        U32 PageType : 4;

        /* op type */
        U32 OPType : 4;

        U32 CurSerialNum:24;

        /* first page TS */
        U32 TimeStamp : 32;

        /* first page TS */
        U32 LastPageTimeStamp : 32;

        U32 FirstPageRealTS : 32;

        /* ReadCnt */
        S32 ReadCnt;
    };
}PBIT_INFO;

typedef struct _PBIT
{
    PBIT_ENTRY m_PBIT_Entry[LUN_NUM_PER_SUPERPU][BLK_PER_LUN + RSVD_BLK_PER_LUN];
    PBIT_INFO  m_PBIT_Info[LUN_NUM_PER_SUPERPU][BLK_PER_LUN + RSVD_BLK_PER_LUN];
    U32 m_TotalDataBlockCnt[BLKTYPE_ALL];
    U32 m_AllocatedDataBlockCnt[BLKTYPE_ALL];
    U32 m_FreePageCnt;
    //U8 m_TLCGCBlkCnt;
    //U8 m_TLCSWLBlkCnt;
    U32 m_EraseCnt[BLKTYPE_ALL];
    U32 m_SaveEraseCnt[BLKTYPE_ALL];

    // for recording Blk-Level TS
    U32 m_CurBlkSN;  //max blk serial num (include slc & tlc)
    U32 m_MinBlkSN;  //min blk serial num

    U16 ulECMin;
    U32 ulECMinCnt;

    U16 m_TargetBlock[TARGET_ALL];
    U16 m_TargetPPO[TARGET_ALL];
    U32 m_TargetOffsetBitMap[TARGET_ALL];
    U32 m_RPMTFlushBitMap[TARGET_ALL];

    //U16 m_GCSrcBlk;//only save SLCGC info
    U16 m_WLSrcBlk;
    U16 m_WLDstBlk;
    S32 m_CurPage;
    
} PBIT;

#define PBIT_PAGE_SIZE_PER_PU     ((U32)sizeof(PBIT))
#define PBIT_PAGE_COUNT_PER_PU    ((PBIT_PAGE_SIZE_PER_PU % BUF_SIZE) ? (PBIT_PAGE_SIZE_PER_PU/BUF_SIZE + 1) : (PBIT_PAGE_SIZE_PER_PU/BUF_SIZE))
#define PBIT_PAGE_SIZE_PER_SUPERPU          ((U32)sizeof(PBIT))
#define PBIT_SUPERPAGE_COUNT_PER_SUPERPU    ((PBIT_PAGE_SIZE_PER_SUPERPU % SUPER_PAGE_SIZE) ? (PBIT_PAGE_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE + 1) : (PBIT_PAGE_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE))

extern GLOBAL MCU12_VAR_ATTR PBIT *pPBIT[SUBSYSTEM_SUPERPU_MAX];

void L2_PBIT_Init(BOOL bLLF, BOOL bKeepEraseCnt);
void L2_PBIT_Init_Clear_All(void);
void L2_PBIT_Save_UpdateData(U8 ucSuperPu);
void L2_PBIT_Load_ResumeData(U8 ucSuperPu);
void L2_Set_DataBlock_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usBlock, RED* pRed);
void L2_Set_DataBlock_PBIT_InfoLastTimeStamp(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usVirBlock, RED* pRed);
void L2_Set_TableBlock_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usBlock, RED* pRed);
void L2_PBIT_Set_Reserve(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Set_Broken(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
void L2_PBIT_Set_PatrolRead(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue);
U32  L2_PBIT_Get_EraseCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN);
void L2_PBIT_VBT_Swap_Block(U8 ucSuperPu, U16 VBN1, U16 VBN2);
void L2_PBIT_Increase_AllocBlockCnt(U8 ucSuperPu, U8 ucBlkType);
void L2_PBIT_Increase_TotalBlockCnt(U8 ucSuperPu, U8 ucBlkType);
void L2_PBIT_Decrease_TotalBlockCnt(U8 ucSuperPu, U8 ucBlkType);
void L2_PBIT_Set_RetryFail(U8 ucSuperPu, U16 PhyBlockAddr, U8 ucLunInSuperPu, U8 bTrue);
BOOL L2_PBIT_Get_RetryFail(U8 ucSuperPu, U16 PhyBlockAddr, U8 ucLunInSuperPu);
void  L2_Set_TableBlock_PBIT_Info_TLC(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPhyBlock, RED* pRed);

void L2_PBIT_SetEvent_SavePbit(U8 ucSuperPu, U8 ucBlkType);
void L2_PBIT_GetFirstPageRedInfo(U8 ucSuperPu, U16 usVirBlkAddr, U16 Page, RED *pRed);
void L2_PBIT_SetLastPageRedInfo(U8 ucSuperPu, U32 ucLunInSuperPu, U16 usVirBlkAddr, RED *pRed);

void L2_BM_CollectFreeBlock(U8 ucSuperPu, U16 VBN);

void L2_BM_CollectFreeBlock_WL(U8 ucSuperPu, U16 PBN);
BOOL L2_IsPBNEmtpy(U8 ucSuperPu, U32 VBN);
BOOL L2_IsPBNError(U8 ucSuperPu, U32 VBN);
void L2_BM_CollectFreeBlock_Rebuild(U8 ucSuperPu, U16 PBN);
void L2_BM_CollectFreeBlock_WL_ErrH(U8 ucSuperPu, U16 VBN, U32 ulFailLun);
void L2_BM_CollectFreeBlock_GC_ErrH(U8 ucSuperPu, U16 VBN, U32 ulFailLun);

//the related interfaces of BlkSN Info
void L2_Set_PBIT_BlkSN_CE(U8 ucSuperPu, U32 Value);
extern void L2_Set_PBIT_BlkSN_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U32 Value);
U32  L2_Get_PBIT_BlkSN_CE(U8 ucSuperPu);
U32  L2_Get_PBIT_BlkSN_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN);
U32  L2_Get_PBIT_PTR_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN);
void L2_Set_PBIT_MinBlkSN_CE(U8 ucSuperPu, U32 Value);
U32  L2_Get_PBIT_MinBlkSN_CE(U8 ucSuperPu);
void L3_Update_WL_Dst_Diff(U8 ucSuperPu, U16 PhyDstBlk);
BOOL L3_Dst_Diff(U8 ucSuperPu, U16 PhyDstBlk);
U16  L2_Get_WLSrcBlk_E(U8 ucSuperPu, U8 ucLunInSuperPu, BOOL bTLC);
U16  L2_Get_WLSrcBlk_S(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PhyBlkDst, BOOL bTLC);
void L2_BM_LLF_Init_BlkSN(void);
void L2_Set_PBIT_BlkSN(U8 ucSuperPu, U16 VBN);



#endif
/********************** FILE END ***************/
