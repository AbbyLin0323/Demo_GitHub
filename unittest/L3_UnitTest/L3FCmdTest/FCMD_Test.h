/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : FCMD_Test.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2014.12.9
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _FCMD_TEST_H
#define _FCMD_TEST_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_MemoryMap.h"
#include "HAL_FlashChipDefine.h"
#include "Disk_Config.h"
#include "L2_PMTPage.h"
#include "L2_FCMDQ.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define UT_DUMMY_PAGE_OFFSET    (8)
#define UT_DUMMY_BLK_OFFSET     (UT_DUMMY_PAGE_OFFSET + 8)
#define UT_DUMMY_TLUN_OFFSET    (UT_DUMMY_BLK_OFFSET + 8)
#define UT_GET_DUMMY_DATA(tlun,blk,page,sec) (0x80000000 + (((tlun)&0xFF)<<UT_DUMMY_TLUN_OFFSET)+(((blk)&0xFF)<<UT_DUMMY_BLK_OFFSET)+(((page)&0xFF)<<UT_DUMMY_PAGE_OFFSET)+(((sec)&0xFF)))

#define L3_UT_DUMMY_RED(Pu, Page, Dw)   ((0x05aa0000 + (Dw))<<3)

// Burn Cnt = (Blk number * average PE Cnt per Blk / 10000)
#define FCMD_TEST_BURN_CNT   (BLK_PER_PLN*3000/15000) 

// Test Ctrol Macros
#define UTC_DATA_CHECK_ENABLE
//#define UTC_DATA_GEN_ENABLE
#ifndef L3_RDT_TEST
#define UTC_RANDOM_READ_MODE
//#define UTC_UN_CLOSED_BLK_WRITE_ENABLE
#endif

// L3 Unit Test Local Request Status Machine.
typedef enum _L3_UT_LOCAL_STAGE
{
    L3_UT_LOCAL_STAGE_INIT = 0,
    L3_UT_LOCAL_STAGE_ERS,
    L3_UT_LOCAL_STAGE_WT,
    L3_UT_LOCAL_STAGE_RD,
    L3_UT_LOCAL_STAGE_RD_RED,
    L3_UT_LOCAL_STAGE_RD_DATA,
    L3_UT_LOCAL_STAGE_RD_LPN,
    L3_UT_LOCAL_STAGE_RD_MERGE,
    L3_UT_LOCAL_STAGE_RD_4KSEQ,
    L3_UT_LOCAL_STAGE_END,
    L3_UT_LOCAL_READ_NUM = L3_UT_LOCAL_STAGE_RD_4KSEQ - L3_UT_LOCAL_STAGE_WT
}L3_UT_LOCAL_STAGE;

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef struct _L3_UT_PU_MG_DPTR{
    U32 BurnCnt;
    U32 CurStage;
    U16 FlashSLCMode:8;
    U16 FlashSingePln:8;
    U16 PhyBlk;
    U16 Blk;
    U16 Page;
    U32 LastPage : 16;
    U32 SLCBlkStart : 16;
    U32 DataBuffAddr;
    U32 SpareBuffAddr;
    U32 FlashStatusBuffAddr;
}L3_UT_PU_MG_DPTR;

typedef struct _L3_UT_LPN_READ_CASE{
    U8 StartInPage;
    U8 Cnt;
    U8 StartInBuf;
    U8 Rvd;
}L3_UT_LPN_READ_CASE;

typedef struct _L3_UT_NORMAL_READ_CASE{
    U32 LPNOffset:8;
    U32 LPNCount :8;
    U32 ReqOffset:8;
    U32 ReqLength:8;
    U32 BitMap;   // for merge read
    PhysicalAddr AddrArray[LPN_PER_BUF];
}L3_UT_NORMAL_READ_CASE;

typedef struct _FCMD_REQ_ITEM_
{
    U32 bsTLun : 8;
    U32 bsRsvd : 8;

    U32 bsPln:2;
    U32 bsFCmdType:2;
    U32 bsPage : 12;

    U32 bsFCmdSubType:3;
    U32 bsTableReq:1;
    U32 bsBlk : 12;

    U32 bsSLCMode:1;
    U32 bsErrH:1;
    U32 bsShiftReadEn : 1; // only data block need to check the safe page issue.
    U32 bsResvd2 : 1;
    U32 bsNewPhyBlk:12;

    U32 DebugLBA;
}FCMD_REQ_ITEM;
typedef struct _FCMD_READ_RANGE_
{
    U32 bsLPNInPage:8;
    U32 bsLPNInBuff:8;
    U32 bsLPNCnt:8;
    U32 bsRsvd:8;
}FCMD_READ_RANGE;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
extern GLOBAL L3_UT_PU_MG_DPTR *g_ptPuMgDptr;

BOOL L3_FCmdTest(void);

S32 L3_UTRand(void);
void L3_UTBasicInit(void);
void L3_UTDataCheck(U8 ucTLun);
void L3_UTPrepareFor4KSEQR(U8 ucSuperPu, U8 ucLunInSuperPu, L3_UT_PU_MG_DPTR * ptPuDptr, FCMD_REQ_ITEM *ptFCmdReqItem, U16 *pBuffID, FCMD_READ_RANGE *ptRange);
void L3_NormalReadPage(FCMD_REQ_ITEM *ptFCmdReqItem, U16 *pBuffID, FCMD_READ_RANGE *ptRange, U8 *pStatus);
U8   L3_UTSelectLunForPush(U8 ucSuperPu);
U8   L3_UTSelectNextStage(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucCurStage);
void L3_UTPrepareForWrite(U8 ucSuperPu, U8 ucLunInSuperPu, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr);
BOOL L3_UTPrepareForWriteFor2DTlc(U8 ucSuperPu, U8 ucLunInSuperPu, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr);
void L3_UTPrepareForRD(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucCurStage, PhysicalAddr *phyAddr, L3_UT_PU_MG_DPTR * ptPuDptr, BUF_REQ_READ *pBufReq);

void UT_Wait(U32 ulBase);
U32 L3_UTGetDataBuffAddr(U8 ucTLun);
U32 L3_UTGetDataBuffAddrFor2DTlcWrite(U8 ucTLun, U16 usPrgWL, U8 ucPageIndex);
U32 L3_UTGetSpareBuffAddr(U8 ucTLun);
U32 L3_UTGetSpareBuffAddrFor2DTlcWrite(U8 ucTLun, U16 usPrgWL, U8 ucPageIndex);
U32 L3_UTGetStatusBuffAddrOTFB(U8 ucTLun);

#endif
/*====================End of this head file===================================*/

