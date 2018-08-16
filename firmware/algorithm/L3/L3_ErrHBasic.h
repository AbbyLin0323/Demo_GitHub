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
* File Name    : L3_ErrHBasic.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_ERRHBASIC_H
#define _L3_ERRHBASIC_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L3_FCMDQ.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
typedef enum _UECC_ERRH_STAGE_
{
    UECC_ERRH_INIT = 0,
    UECC_ERRH_EMPTY_PAGE,
#ifdef N1_SCAN
    UECC_ERRH_N1_SCAN,
#endif
    UECC_ERRH_READ_RETRY,
    UECC_ERRH_SOFT_DECODER,
    UECC_ERRH_DATA_RECOVER,
    UECC_ERRH_REPORT_FAIL,
    UECC_ERRH_DONE
}UECC_ERRH_STAGE;

typedef enum _EMPTY_PG_HANDLE_STAGE_
{
    EMPTY_PG_HANDLE_INIT = 0,
    EMPTY_PG_HANDLE_REDO,
    EMPTY_PG_HANDLE_REDO_CHK,
    EMPTY_PG_HANDLE_DONE
}EMPTY_PG_HANDLE_STAGE;

typedef enum _READ_RETRY_STAGE_
{
    READ_RETRY_INIT = 0,
    READ_RETRY_SHIFTRD,
    READ_RETRY_SHIFTRD_CHK,
    READ_RETRY_READ,
    READ_RETRY_READ_CHK,
    READ_RETRY_SUCCESS,
    READ_RETRY_FAIL
}READ_RETRY_STAGE;

typedef enum _RPT_UECCSTS_STAGE_
{
    UECCH_FAILED_INIT = 0,
    UECCH_FAILED_PEND,
    UECCH_FAILED_DONE
}RPT_UECCSTS_STAGE;

#ifdef N1_SCAN
/* Vt SCAN by N1 Related  */
typedef enum _N1_SCAN_STATG_
{
    N1_SCAN_INIT = 0x0,
    N1_SCAN_SHIFT_VT,
    N1_SCAN_READ,
    N1_SCAN_CHECK_N1,
    N1_SCAN_TERMINATE,
    N1_SCAN_DONE
}N1_SCAN_STAGE;

typedef struct _N1_SHIFT_RANGE_
{
    U8 ucStartVt;
    U8 ucEndVt;
    U8 ucAddr;       //set feature FA
    U8 ucBestVt;
}N1_SHIFT_RANGE;

/* TSB Flash Read Level Definition */
#define READ_LEVEL_CNT      (8)//TLC

#endif
/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHAllocDram(U32 *pFreeDramBase);
void MCU2_DRAM_TEXT L3_ErrHAdaptiveRetryTableInit(void);
void MCU2_DRAM_TEXT L3_ErrHBuildBrthLunMap(void);
void MCU2_DRAM_TEXT L3_ErrHSetBrthLunLockBmp(U8 ucTLun);
void MCU2_DRAM_TEXT L3_ErrHClrBrthLunLockBmp(U8 ucTLun);
BOOL MCU2_DRAM_TEXT L3_ErrHRead(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
#ifdef READ_RETRY_REFACTOR
U8 MCU2_DRAM_TEXT L3_ErrHGetRetryVtIndex(U8 ucTLun, U8 ucIndex, BOOL bTlcMode, U16 usWLType, U8 ucPageType);
#endif

void L3_PCmdHandling(U8 ucTLun);
void L3_ErrHHandling(U8 ucTLun);

#endif
/*====================End of this head file===================================*/

