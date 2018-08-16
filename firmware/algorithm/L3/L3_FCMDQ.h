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
* File Name    : L3_FCMDQ.h
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.2.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_FCMDQ_H
#define _L3_FCMDQ_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L2_FCMDQ.h"
#include "HAL_LdpcSoftDec.h"
#include "FW_SDL.h"
#include "HAL_DecStsReport.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define FCMD_INTR_BAK_NUM (SUBSYSTEM_LUN_NUM)

/*============================================================================*/
/* #FCmd Internal Error Handling Entry define                                 */
/*============================================================================*/
typedef struct _UECCH_RETRY_
{
    U32 bsEnable : 1;
    U32 bsTime   : 7;
    U32 bsUnSend : 1;
    U32 bsRsvd   : 23;
}UECCH_RETRY;

#ifdef N1_SCAN
#if 0
typedef struct _SAR_PARAM_
{
    /* FA=0x89 */
    union{
        U32 ulParamFor89h;
        
    struct {
        U8 rLA; //A Read, LSB
        U8 rLC; //C Read, MSB
        U8 rLE; //E Read, LSB
        U8 rLG; //G Read, MSB
    
        };
    };
    
    /* FA=0x8A */
    union{
        U32 ulParamFor8Ah;

    struct {
        U8 rLB; //B Read, CSB
        U8 rLD; //D Read, CSB
        U8 rLF; //F Read, CSB
        U8 bsResolution:4;
        U8 bsRsv:1;
        U8 bsSearchCnt:3;
        };
    };
}SAR_PARAM;

typedef union _DISTANCE_PARAM_
{
    /* FA=00h */
    union{
        U32 ulParamFor00h;
        struct{
            U32 rLA_rLB_rLC:3;
            U32 bsRsv0:1;
            U32 rLE_rLD_rLG:3;
            U32 bsRsv1:25;
        };
    };
    /* FA=01h */
    union{
        U32 ulParamFor01h;
        struct{
            U32 rLF:3;
            U32 bsRsv2:29;
        };
    };
}DISTANCE_PARAM;
#endif

typedef struct _N1_SCAN_PARA_
{
    U32 bsRdLevel:  4;   //7 levels: AR-GR
    U32 bsShiftVt:  8;
    //U32 bsVirPage:  12;
    U32 bsPageType: 2;
    U32 bsVtIdx       :10;
    U32 bsStep:     4;       //the interval between adjusted Vt, only support max 15
}N1_SCAN_PARA;
#endif

typedef struct _UECCH_SOFTDECO_
{
    U32 bsSinglePlaneStatus     : 4;
    U32 bsWholeStatus           : 4;
    U32 bsHaveNotUeccPlane      : 4;        // If a plane didn't occur UECC, then the bit of it will be set to 1.
    U32 bsStartDescriptorId     : 4;
    U32 bsDescriptorCount       : 5;        // UECC bitmap have 32 bits, so max descriptor count of one page is
    // 16, so we need 5 bits.
    U32 bsShiftReadTimesIndex   : 4;
    U32 bsCurPlaneIndex         : 2;        // Index of the plane that doing LDPC Soft-Decode.
    U32 bsCurShiftReadTime      : 3;
    U32 bsRsvd                  : 2;
    SDL_NODE * aRawPageReadBufferId[LDPC_SOFT_MAX_DEC_CNT];
    SDL_NODE * aDecodeResultBufferId[PLN_PER_LUN];
    U32 aDecSramBitMap[PLN_PER_LUN];
    U32 ulLba;
}UECCH_SOFTDECO;

typedef struct _UECCH_XORCOVER_
{
    U32 bsXoreId           : 3;
    U32 bsReadPlnCnt       : 3;
    U32 bsHaveFirstXorPage : 1;
    U32 bsBrokenPln        : 2;
    U32 bsCurPln           : 2;
    U32 bsReadSinglePlnCnt : 2;
    U32 bsIsStarted        : 1;
    U32 bsRsvd             : 18;
    U8  ucReadStatus;
    U8  ucSuperPu;
    U8  ucCurLunInSuperPu;
    U8  ucRsvd;
}UECCH_XORCOVER;

typedef struct _UECC_ERRH_CTRL_
{
    U32 bsStage    : 8;
    U32 bsSubStage : 8;
    U32 bsRsvd     : 16;
    U32 aFailedCwBitMap[DEC_PLN_MAX_PER_LUN];
    SDL_NODE      *ptBufNodePtr;
    SDL_NODE      *ptRedunNodePtr;
    UECCH_RETRY    tRetry;
#ifdef N1_SCAN
    N1_SCAN_PARA   tN1ScanPara;
#endif
    UECCH_SOFTDECO tSoftDeco;
    UECCH_XORCOVER tXoRcover;
}UECC_ERRH_CTRL;

typedef struct _EXTEND_ERRH_CTRL_
{
    U32 bsStage      : 4;
    U32 bsSubStage   : 4;
    U32 bsBackUpBlk  : 1;
    U32 bsBadBlkType : 3;
    U32 bsClosedBlk  : 1;
    U32 bsRsvd       : 19;
    U32 bsCpyPageCnt : 16;
    U32 bsCpyPageNum : 16;
    U32 bsNewPhyBlk  : 16;
    U32 bsNewVirBlk  : 16;
    U32 ulStatusAddr;
    U32 ulTmpBufAddr;
    U32 ulXorParityAddr;
}EXTD_ERRH_CTRL;

typedef struct _FCMD_INTR_ERRH_ENTRY_
{
    U32 bsErrRpt        : 1;
    U32 bsErrCode       : 7;
    U32 bsEmptyPg       : 1;
    U32 bsErrHold       : 1;
    U32 bsPlnErrInfo    : 4; //Plane error Bit map, support 4 plane E/W/R
    U32 bsRsvd          : 18;
    UECC_ERRH_CTRL tUeccHCtrl;
    EXTD_ERRH_CTRL tExtHCtrl;
}FCMD_INTR_ERRH_ENTRY;

/*============================================================================*/
/* #FCmd Error Handling Queue define                                          */
/*============================================================================*/
typedef struct _FCMD_INTR_ERRH_
{
    FCMD_INTR_ERRH_ENTRY atErrHQ[SUBSYSTEM_LUN_MAX];
}FCMD_INTR_ERRH;
#define FCMD_INTR_ERRH_SZ (SUBSYSTEM_LUN_NUM*sizeof(FCMD_INTR_ERRH_ENTRY))
#define FCMD_INTR_ERRH_SZ_BAK (FCMD_INTR_BAK_NUM*sizeof(FCMD_INTR_ERRH_ENTRY))

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* #Status Bitmap count define(For reducing L3 scheduling cycle count purpose)*/
/*============================================================================*/
typedef struct _STS_BM_CNT_
{
    U32 ucPendCnt       : 8;
    U32 ucRecycleCnt    : 8;
    U32 ucErrHCnt       : 8;
    U32 ucExtHCnt       : 8;
} STS_BM_CNT;

/*============================================================================*/
/* #FCmd Internal Ctrl Entry define                                           */
/*============================================================================*/
typedef struct _FCMD_DATA_TX_CTRL_
{
    // DW0-1
    U32 bsDSGEn       : 1;
    U32 bsLpnBitMapPos: 3;
    U32 bsAddChainEn  : 1;
    U32 bsRemSecLen   : 20;
    U32 bsRsvd1       : 1;
    U32 bsPrdOrPrpIdx : 6;
    U32 bsLstSecRemain: 10;
    U32 bsOffSet      : 22;
    // DW2
    U32 bsFstEngineID : 16;
    U32 bsCurEngineID : 16;
    // DW3
    U32 bsBdEngineDone: 1;
    U32 bsSecAddrIndex: 3;
    U32 bsDmaTotalLen : 9;
    U32 bsRsvd2       : 19;
    // DW4-5
    NFC_SEC_ADDR aSecAddr[NFCQ_SEC_ADDR_COUNT];
}FCMD_DATA_TX_CTRL;

typedef struct _FCMD_INTR_CTRL_ENTRY_
{
    // DW#0
    FCMD_REQ_ENTRY *ptReqEntry;

    // DW#1
    U32 bsPhyPage   : 16;
    U32 bsCmdType   : 8;
    U32 bsUpdStsMod : 2;
    U32 bsMultiStep : 2;
    U32 bsCtrlPtr   : 2;
    U32 bsNfcqPtr   : 2;

    // DW#2
    U32 bsIntrPage  : 2; // TLC-Read: Low_Page/Mid_Page/Uper_Page; TLC-Write: 1st_PrgCycle/2nd_PrgCycle/3rd_PrgCycle.
    U32 bsScrMod    : 3;
    U32 bsXoreId    : 3;
    U32 bsRdRawData : 1;
    U32 bsBypassEcc : 1;
    U32 bsEMEn      : 1;
    U32 bsIntrReq   : 1;
    U32 bsLBAChkEn  : 1;
    U32 bsDW2Rsvd   : 19;

    // DW#3~7
    FCMD_DATA_TX_CTRL tDTxCtrl;

    // DW#8
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry;

}FCMD_INTR_CTRL_ENTRY;


typedef struct _ARB_STS_CNT_
{
    U32 ucPendCnt    : 8;
    U32 ucRecycleCnt : 8;
    U32 ucErrHCnt    : 8;
    U32 ucExtHCnt    : 8;
}ARB_STS_CNT;
/*============================================================================*/
/* #FCmd Internal Ctrl Queue define                                           */
/*============================================================================*/
typedef struct _FCMD_INTR_CTRL_Q_
{
    FCMD_INTR_CTRL_ENTRY atCtrl[NFCQ_DEPTH];
}FCMD_INTR_CTRL_Q;

typedef struct _FCMD_INTR_CTRL_
{
    FCMD_INTR_CTRL_Q atCtrlQ[SUBSYSTEM_LUN_MAX];
}FCMD_INTR_CTRL;
#define FCMD_INTR_CTRL_SZ (SUBSYSTEM_LUN_NUM*sizeof(FCMD_INTR_CTRL_Q))
#define FCMD_INTR_CTRL_SZ_BAK (FCMD_INTR_BAK_NUM*sizeof(FCMD_INTR_CTRL_Q))

/*============================================================================*/
/* #FCmd Internal Queue Dptr define                                           */
/*============================================================================*/
#define INVALID_DPTR  INVALID_2F
typedef struct _FCMD_INTR_Q_DPTR_
{
    U8 ucWptr; // Free Pointer
    U8 ucRptr; // Recycle FCmd Pointer
    U8 ucPptr; // Pending FCmd Pointer
    U8 ucEptr; // Error FCmd Pointer
}FCMD_INTR_Q_DPTR;

typedef struct _FCMD_INTR_DPTR_
{
    FCMD_INTR_Q_DPTR atIntrQDptr[SUBSYSTEM_LUN_MAX];
}FCMD_INTR_DPTR;
#define FCMD_INTR_DPTR_SZ (SUBSYSTEM_LUN_NUM*sizeof(FCMD_INTR_Q_DPTR))
#define FCMD_INTR_DPTR_SZ_BAK (FCMD_INTR_BAK_NUM*sizeof(FCMD_INTR_Q_DPTR))

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
extern FCMD_INTR_CTRL *g_ptFCmdIntrCtrl; // Allocated from Dsram0-mcu2
extern FCMD_INTR_ERRH *g_ptFCmdIntrErrH; // Allocated from Dram/Dsram0-mcu2
extern FCMD_INTR_DPTR *g_ptFCmdIntrDptr; // Allocated from Dsram0-mcu2

extern FCMD_REQ_ENTRY *g_ptFCmdReqBak2;  // for set-feature
extern FCMD_REQ_ENTRY *g_ptFCmdReqBak;
extern FCMD_INTR_CTRL *g_ptFCmdIntrCtrlBak; // Allocated from Dram/Dsram0-mcu2
extern FCMD_INTR_ERRH *g_ptFCmdIntrErrHBak; // Allocated from Dram/Dsram0-mcu2
extern FCMD_INTR_DPTR *g_ptFCmdIntrDptrBak; // Allocated from Dram/Dsram0-mcu2

void MCU2_DRAM_TEXT L3_FCMDQAllocSRAM0(U32 *pFreeSram0Base);
void MCU2_DRAM_TEXT L3_FCMDQIntrInit(BOOL bIsBak);

FCMD_REQ_PRI L3_FCMDQArbReqPri(U8 ucTLun);
U8 L3_FCMDQGetReqRptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
void L3_FCMDQAddReqRptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);

FCMD_REQ_ENTRY *L3_FCMDQPopReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);

U8 L3_FCMDQGetIntrWptr(U8 ucTLun, BOOL bBak);
U8 L3_FCMDQGetIntrRptr(U8 ucTLun, BOOL bBak);
U8 L3_FCMDQGetIntrPptr(U8 ucTLun, BOOL bBak);
U8 L3_FCMDQGetIntrEptr(U8 ucTLun, BOOL bBak);
void L3_FCMDQSetIntrWptr(U8 ucTLun, U8 ucWptr, BOOL bBak);
void L3_FCMDQSetIntrRptr(U8 ucTLun, U8 ucRptr, BOOL bBak);
void L3_FCMDQSetIntrPptr(U8 ucTLun, U8 ucPptr, BOOL bBak);
void L3_FCMDQSetIntrEptr(U8 ucTLun, U8 ucEptr, BOOL bBak);

FCMD_INTR_CTRL_ENTRY *L3_FCMDQAllocIntrEntry(U8 ucTLun);
FCMD_INTR_CTRL_ENTRY *L3_FCMDQGetIntrCtrlEntry(U8 ucTLun, U8 ucLevel, BOOL bBak);
FCMD_INTR_ERRH_ENTRY *L3_FCMDQAllocIntrErrHEntry(U8 ucTLun, BOOL bBak);
FCMD_INTR_ERRH_ENTRY *L3_FCMDQGetIntrErrHEntry(U8 ucTLun, BOOL bBak);
FCMD_REQ_ENTRY *L3_FCMDQAllocTmpReqEntry(U8 ucTLun);
FCMD_REQ_ENTRY *L3_FCMDQGetTmpReqEntry(U8 ucTLun);
FCMD_REQ_ENTRY *L3_FCMDQAllocTmp2ReqEntry(U8 ucTLun);
FCMD_REQ_ENTRY *L3_FCMDQGetTmp2ReqEntry(U8 ucTLun);

#endif
/*====================End of this head file===================================*/

