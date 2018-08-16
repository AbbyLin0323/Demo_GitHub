/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
* File Name    : L2_FCMDQ.h
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.7.14
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L2_FCMDQ_H
#define _L2_FCMDQ_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "Disk_Config.h"
#include "HAL_FlashDriverBasic.h"
#include "L3_HostAdapter.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#ifdef UT_QD1
#define FCMDQ_DEPTH    (1)
#else
#define FCMDQ_DEPTH    (NFCQ_DEPTH)
#endif
#define SLC_OR_BUF_NUM (3)
#define DSG_BUFF_SIZE  (SLC_OR_BUF_NUM)
#define SLC_PG_PER_TLC (SLC_OR_BUF_NUM)
/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef enum _FCMD_REQ_PRI_
{
    FCMD_REQ_PRI_LOW = 0, // Other Requests
    FCMD_REQ_PRI_HIGH = 0,    // Host Read, Merge Read
    FCMD_REQ_PRI_NUM
}FCMD_REQ_PRI;

typedef enum _FCMD_REQ_TYPE_
{
    FCMD_REQ_TYPE_WRITE = 0,
    FCMD_REQ_TYPE_READ,
    FCMD_REQ_TYPE_ERASE,
    FCMD_REQ_TYPE_CHK_IDLE,
    FCMD_REQ_TYPE_SELF_TEST,
    FCMD_REQ_TYPE_RDSTS,
    FCMD_REQ_TYPE_RSVD2,
    FCMD_REQ_TYPE_SETFEATURE,
    FCMD_REQ_TYPE_NUM
}FCMD_REQ_TYPE;

typedef enum _FCMD_REQ_SUBTYPE_
{
    FCMD_REQ_SUBTYPE_SINGLE = 0,
    FCMD_REQ_SUBTYPE_NORMAL,
    FCMD_REQ_SUBTYPE_INTRNAL,
    FCMD_REQ_SUBTYPE_ONEPG,
    FCMD_REQ_SUBTYPE_IDLE_0,
    FCMD_REQ_SUBTYPE_SINGLE_ONEPG = FCMD_REQ_SUBTYPE_IDLE_0,
    FCMD_REQ_SUBTYPE_IDLE_1,
    FCMD_REQ_SUBTYPE_SINGLE_TWOPG = FCMD_REQ_SUBTYPE_IDLE_1,
    FCMD_REQ_SUBTYPE_IDLE_2,
    FCMD_REQ_SUBTYPE_SETFEATURE,
    FCMD_REQ_SUBTYPE_NUM
}FCMD_REQ_SUBTYPE;


typedef enum _FCMD_REQ_BLK_MOD_
{
    FCMD_REQ_SLC_BLK = 0,
    FCMD_REQ_MLC_BLK,
    FCMD_REQ_TLC_BLK,
    FCMD_REQ_RSV_MOD
}FCMD_REQ_BLK_MOD;

typedef enum _FCMD_REQ_STS_
{
    FCMD_REQ_STS_INIT = 0,
    FCMD_REQ_STS_ALLOC,
    FCMD_REQ_STS_PUSH,
    FCMD_REQ_STS_POP
}FCMD_REQ_STS;

typedef enum _REQ_STS_UPT_MODE_
{
    REQ_STS_UPT_NULL = 0,
    REQ_STS_UPT_MANUL,
    REQ_STS_UPT_AUTO
}REQ_STS_UPT_MODE;

typedef struct _FCMD_BUF_DESC_
{
    U32 bsBufID     : 16;
    U32 bsSecStart  : 8;
    U32 bsSecLen    : 8; // 0:256, 1~255:1~255
}FCMD_BUF_DESC;

typedef struct _FCMD_FLASH_ADDR_
{
    U32 bsVirBlk    : 11;
    U32 bsPhyBlk    : 11;
    U32 bsVirPage   : 10;
}FCMD_FLASH_ADDR;

typedef struct _FCMD_FLASH_DESC_
{
    // DW#0
#ifdef FLASH_INTEL_3DTLC
#ifdef FLASH_IM_3DTLC_GEN2
    U32 bsVirBlk    : 9;
    U32 bsPhyBlk    : 9;
    U32 bsVirPage   : 12;
    U32 bsRsvd      : 2;
#else
    U32 bsVirBlk    : 10;
    U32 bsPhyBlk    : 10;
    U32 bsVirPage   : 11;
    U32 bsRsvd      : 1;
#endif
#elif defined(FLASH_YMTC_3D_MLC)
    U32 bsVirBlk    : 12;
    U32 bsPhyBlk    : 12;
    U32 bsVirPage   : 8;
#else
    FCMD_FLASH_ADDR atFlashAddr;
#endif

    // DW#1
    U32 bsSecStart    : 8;
    U32 bsSecLen      : 8; // 0:256, 1~255:1~255
    U32 bsBlkMod      : 2;
    U32 bsPlnNum      : 2;
    U32 bsShiftRdEn   : 1;
    U32 bsHostRdEn    : 1;
    U32 bsRdRedOnly   : 1;
    U32 bsMergeRdEn   : 1;
    U32 bsLpnBitmap   : 8;
}FCMD_FLASH_DESC;

typedef struct _FCMD_LOCAL_DESC_
{
    U32 bsRdtCmd         : 1;   // 1 - RDT command
    U32 bsPatrolRdCmd    : 1;   // 1 - Patrol Read Commamd
    U32 bsRawRdCmd       : 1;   // 1 - Raw Read Cmd
    U32 bsPairPageType   : 3;   // 3 - TLC page type : lower/upper/extra
    U32 bsPairPageCnt    : 2;   // 2 - TLC page pair page count : 1 or 2
    U32 bsRsvd           : 12;
    U32 bsPairPageNum    : 12;  // 12 - TLC page's pair page number
    U32 ulDWRsvd;
}FCMD_LOCAL_DESC;

/*============================================================================*/
/* #FCmd Req Entry define                                                     */
/*============================================================================*/
// 12*DW Per FCMD_REQ_ENTRY
typedef struct _FCMD_REQ_ENTRY_
{
    // DW#0
    U32 bsTLun           : 8;

    U32 bsTBRebuilding   : 1;
    U32 bsBootUpOk       : 1;
    U32 bsPrePgEn        : 1;
    U32 bsTableReq       : 1;
    U32 bsNeedL3ErrHandle: 1;
    U32 bsContainXorData : 1;
    U32 bsDecFifoEn      : 1;
    U32 bsFCmdPri        : 1;

    U32 bsReqPtr         : 2;
    U32 bsReqType        : 3;
    U32 bsReqSubType     : 3;

    U32 bsXorStripeId    : 4;
    U32 bsReqUptMod      : 2;
    U32 bsXorEn          : 1;

#if 1// (defined(HAL_NFC_TEST) || defined(HAL_UNIT_TEST)) || defined(FLASH_TEST)
    U32 bsForceCCL       : 1;
#else
    U32 bsDW0Rsvd1       : 1;
#endif

    // DW#1
    U32 ulReqStsAddr;

    // DW#2
    U32 ulSpareAddr;

    // DW#3~4
    FCMD_FLASH_DESC tFlashDesc;

    // DW#5~7
    union {
        FCMD_BUF_DESC atBufDesc[DSG_BUFF_SIZE];
        FCMD_FLASH_ADDR atFlashAddr[SLC_PG_PER_TLC];
    };

    // DW#8~11
    union {
        FCMD_HOST_DESC tHostDesc;  //Host Memory description
        FCMD_LOCAL_DESC tLocalDesc;
    };
}FCMD_REQ_ENTRY;

/*============================================================================*/
/* #FCmd REQ Queue define                                                     */
/*============================================================================*/
typedef struct _FCMD_REQ_Q_
{
    FCMD_REQ_ENTRY atReq[FCMDQ_DEPTH];
}FCMD_REQ_Q;

typedef struct _FCMD_REQ_
{
    FCMD_REQ_Q atReqQ[SUBSYSTEM_LUN_MAX][FCMD_REQ_PRI_NUM];
}FCMD_REQ;
#define FCMD_REQ_SZ (SUBSYSTEM_LUN_NUM*FCMD_REQ_PRI_NUM*sizeof(FCMD_REQ_Q))

/*============================================================================*/
/* #FCmd REQ Queue Status define                                              */
/*============================================================================*/
typedef struct _FCMD_REQSTS_Q_
{
    U8 aReqStatus[FCMDQ_DEPTH];
}FCMD_REQSTS_Q;

typedef struct _FCMD_REQSTS_
{
    FCMD_REQSTS_Q atReqStsQ[SUBSYSTEM_LUN_MAX][FCMD_REQ_PRI_NUM];
}FCMD_REQSTS;
#define FCMD_REQSTS_SZ (SUBSYSTEM_LUN_NUM*FCMD_REQ_PRI_NUM*sizeof(FCMD_REQSTS_Q))

/*============================================================================*/
/* #FCmd REQ Queue Dptr define                                                */
/*============================================================================*/
typedef struct _FCMD_REQ_Q_DPTR_
{
    U8 ucWptr;
    U8 ucRptr;
}FCMD_REQ_Q_DPTR;

typedef struct _FCMD_REQ_DPTR_
{
    FCMD_REQ_Q_DPTR atReqQDptr[SUBSYSTEM_LUN_MAX][FCMD_REQ_PRI_NUM];
}FCMD_REQ_DPTR;
#define FCMD_REQ_DPTR_SZ (SUBSYSTEM_LUN_MAX*FCMD_REQ_PRI_NUM*sizeof(FCMD_REQ_Q_DPTR))

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
extern MCU12_VAR_ATTR FCMD_REQ      *g_ptFCmdReq;
extern MCU12_VAR_ATTR FCMD_REQSTS   *g_ptFCmdReqSts;
extern MCU12_VAR_ATTR volatile FCMD_REQ_DPTR *g_ptFCmdReqDptr;

void L2_FCMDQReqInit(void);
void L2_FCmdReqStsInit(U32 ucMcuId);

U8 L2_FCMDQGetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
void L2_FCMDQSetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);

U32  L2_FCMDQGetReqStsAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
FCMD_REQ_STS L2_FCMDQGetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
void L2_FCMDQSetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel, FCMD_REQ_STS eReqSts);
BOOL L2_FCMDQIsWptrFree(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
BOOL L2_FCMDQIsNotFull(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
U32  L2_FCMDQGetNotFullBitmap(FCMD_REQ_PRI eFCmdPri);
BOOL L2_FCMDQIsEmpty(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
U32  L2_FCMDQGetEmptyBitmap(FCMD_REQ_PRI eFCmdPri);
BOOL L2_FCMDQIsAllEmpty(void);

FCMD_REQ_ENTRY *L2_FCMDQGetReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
FCMD_REQ_ENTRY *L2_FCMDQAllocReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
void L2_FCMDQPushReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);

void L2_FCMDQAdaptPhyBlk(FCMD_REQ_ENTRY *ptReqEntry);

// Interface Adapter
BOOL L2_FCMDQNotFull(U8 ucTLun);

#endif
/*====================End of this head file===================================*/

