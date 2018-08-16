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
File Name     : L2_TLCMerge.h
Version       : Initial Draft
Author        : ZoeWen
Created       : 2015/05/22
Description   : TLC manager define
Function List :
History       :
1.Date        : 2015/05/22
Author      : ZoeWen
Modification: Created file
******************************************************************************/

#ifndef _L2_TLC_MERGE_H
#define _L2_TLC_MERGE_H
#include "BaseDef.h"
#include "L2_Defines.h"
#include "L2_GCManager.h"

#define L2_HANDLE_UECCBLK_PERLUN_MAX 20
#define TLC_PROGPAGE_CNT 2
#define TLC_FIRST_RPMT_PAGE_NUM (PG_PER_TLC_BLK - 3)

#ifdef FLASH_IM_3DTLC_GEN2
#define TLC_FORCE_TO_CLOSE_PAGE_NUM 2219
#else
#define TLC_FORCE_TO_CLOSE_PAGE_NUM 1502
#endif

typedef enum _IM_3D_TLC_PAGE_TYPE
{
    L2_LOWER_PAGE = 0,
    L2_UPPER_PAGE,
    L2_EXTRA_PAGE,
    L2_LOW_PAGE_WITHOUT_HIGH
}IM_3D_TLC_PAGE_TYPE;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef FLASH_IM_3DTLC_GEN2 // for B16A and B17A
typedef union _INVERSE_TLC_SHARE
{   U16 Value;
    struct
    {
      U16 m_PageType : 2;//Lower/Upper/Extra/LOW_PAGE_WITHOUT_HIGH
      U16 m_ProgPageCnt : 2;//0 ~ 2
      U16 m_2ndPageNum : 12;//B16A(0 ~ 2303)
   };
} INVERSE_TLC_SHARED_PAGE;
#else
typedef union _INVERSE_TLC_SHARE
{   U16 Value;
    struct
    {
      U16 m_PageType : 2;//Lower/Upper/Extra/LOW_PAGE_WITHOUT_HIGH
      U16 m_ProgPageCnt : 2;//0 ~ 2
      U16 m_WLClosed : 1;
      U16 m_2ndPageNum : 11;//B0KB(0 ~ 1535)
   };
} INVERSE_TLC_SHARED_PAGE;
#endif
#else
typedef union _INVERSE_TLC_SHARE
{   U16 Value;
    struct
    {
      U16 m_PageType : 2;//Lower/Upper/Extra/LOW_PAGE_WITHOUT_HIGH
      U16 m_ProgPageCnt : 2;//0 ~ 2
      U16 m_2ndPageNum : 12;//B0KB(0 ~ 1535)/B16A(0 ~ 2303)
   };
} INVERSE_TLC_SHARED_PAGE;
#endif

typedef enum _TLCRESET_STAGE
{
    TLCRESET_UPDATE_RPMT = 0,
    TLCRESET_BLKERASE_CHECK,
    TLCRESET_RESET_MANAGER
}TLCRESET_STAGE;

typedef struct _TLC_Merge
{
    U16 ausSrcBlk[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL];
    U16 ausDesTLCBlk[SUBSYSTEM_SUPERPU_MAX];

    U16 ausTLCProgOrder[SUBSYSTEM_SUPERPU_MAX];
    
    TLCStage aeTLCStage[SUBSYSTEM_SUPERPU_MAX];

    TLCRESET_STAGE aeTLCResetStage[SUBSYSTEM_SUPERPU_MAX];

    U8 aucSLCReadNum[SUBSYSTEM_SUPERPU_MAX];

    TLCWriteType aeTLCWriteType[SUBSYSTEM_SUPERPU_MAX];

    /* status [WLx_PGx, WLx_PGx, WLx_PGx] */
    /* the last status is write status */
    //U8 aucTLCBufStatus[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU][(TLC_BUF_CNT + 1)];
    U8 (*aucTLCBufStatus)[LUN_NUM_PER_SUPERPU][(TLC_BUF_CNT + 1)];
    
    /* the TLC GC write Buffer to store the data page, one more buffer for TLCGC use as buffer0's substitute*/
    //PhysicalPage *ptWriteTLCBuffer[SUBSYSTEM_SUPERPU_MAX][TLC_BUF_CNT + PG_PER_WL][LUN_NUM_PER_SUPERPU];
    PhysicalPage *ptWriteTLCBuffer[SUBSYSTEM_SUPERPU_MAX][TLC_BUF_CNT + 1][LUN_NUM_PER_SUPERPU];//3D_TLC RingBuffers[3 + 1 (Extra's Upper)]
    
    /* SLC spare buffer */
    //RED atSpare[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU][TLC_BUF_CNT];
    RED* (*atSpare)[LUN_NUM_PER_SUPERPU][TLC_BUF_CNT];

    U32 aucTLCReadBitMap[SUBSYSTEM_SUPERPU_MAX];

    /* RPMT */
    RPMT* pRPMT[SUBSYSTEM_SUPERPU_MAX][PG_PER_WL];
    
    U8 aucRPMTNum[SUBSYSTEM_SUPERPU_MAX];  //for mlc, slc: 1; for TLC:3;

    //U8 aucRPMTStatus[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU][PG_PER_WL];
    U8 (*aucRPMTStatus)[LUN_NUM_PER_SUPERPU][PG_PER_WL];

    /* Error handling */
    TLCErrH aeErrHandleStage[SUBSYSTEM_SUPERPU_MAX];
    U32 aulErrProgBitMap[SUBSYSTEM_SUPERPU_MAX];

#ifdef L2_HANDLE_UECC
    TLCErrH aeErrHandleUECCStage[SUBSYSTEM_SUPERPU_MAX];
    U16 ausUECCBlk[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU][L2_HANDLE_UECCBLK_PERLUN_MAX];
    U16 ausUECCBlkCnt[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
#endif

    /* TLC internal write */
    BOOL bTLCInternalW[SUBSYSTEM_SUPERPU_MAX];
    U32 aFirstPageRealTS[SUBSYSTEM_SUPERPU_MAX];

    /*support for TLCGC use*/
    U8 m_TLCGCBufferCopyCnt[SUBSYSTEM_SUPERPU_MAX];
    
    /* TLCGC LPN's SrcAddrRecord*/
    PhysicalAddr (*m_TLCGCSrcAddr)[PG_PER_WL][LPN_PER_SUPER_BLOCK/PG_PER_WL];
}TLCMerge;

BOOL L2_TLCReset(U8 ucSuperPu);
void L2_ResetTLCManager(U8 ucSuperPu);
TLCStage L2_TLCReadSrcBlk(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
BOOL L2_TLCWriteTLC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
void MCU1_DRAM_TEXT L2_TLCSharedPageClosedReset(U8 ucSuperPu);
void MCU1_DRAM_TEXT L2_TLCSharedPageClosedSave(U8 ucSuperPuNum);
void MCU1_DRAM_TEXT L2_TLCSharedPageClosedLoad(U8 ucSuperPuNum);
#else//SHUTDOWN_IMPROVEMENT_STAGE1
void MCU1_DRAM_TEXT L2_EraseTLCWriteBlk(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
void MCU1_DRAM_TEXT L2_TLCShutdownReset(U8 ucSuperPu);
#endif

BOOL L2_SetTLCMergeInfo(U8 ucSuperPu, U8 ucSrcVBTType, TLCWriteType eWriteType);
BOOL L2_TLCWrite(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
void MCU1_DRAM_TEXT L2_InitTLCInverseProgOrderTable(void);
void L2_SetTLCPPO(U32 uSuperPu, U32 PPO);
U32 L2_GetTLCPPO(U32 uSuperPu);
BOOL L2_IsTLCProLastWordLine(U32 uSuperPu);
BOOL L2_TLCWaitReadStatus(U8 ucSuperPu);
BOOL L2_TLCCheckWriteStatus(U8 ucSuperPu);
BOOL MCU1_DRAM_TEXT L2_TLCCheckEraseStatus(U8 ucSuperPu);
void L2_TLCHandleProgFail(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
void L2_Set_WL_TLCPPO(U32 uSuperPu, U32 PPO);


U8 L2_GetInterCopySrcBlkIndex(U16 usPageNum);
U16 L2_GetInterCopyPgIndex(U16 usPageNum);

U8 L2_GetProgCycleByProgOrder(U8 ucSuperPu);
U8 L2_GetCurTLCWLByProgOrder(U8 ucSuperPu);

U8 L2_GetPageTypeByProgOrder(U8 ucSuperPu);
U8 L2_GetProgPageCntByProgOrder(U8 ucSuperPu);
U16 L2_Get1stPageNumByProgOrder(U8 ucSuperPu);
U16 L2_Get2ndPageNumByProgOrder(U8 ucSuperPu);

U8 L3_GetPageTypeByProgOrder(U16 usPage);
U8 L3_GetProgPageCntByProgOrder(U16 usPage);
U16 L3_Get2ndPageNumByProgOrder(U16 usPage);

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED
BOOL L2_GetWordLineClose(U16 usCurPageNum);
#else
BOOL L2_GetSharedPageClose(U16 usCurPageNum);
#endif
#endif

extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;
#endif

