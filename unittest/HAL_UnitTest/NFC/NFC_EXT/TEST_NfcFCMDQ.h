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
Filename    : TEST_NfcFCMDQ.h
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : compile both by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_FCMDQ_H_
#define _HAL_NFC_TEST_FCMDQ_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "L2_Interface.h"
#include "L2_FCMDQ.h"
#include "HAL_Xtensa.h"
#include "TEST_NfcConfig.h"
#include "COM_BitMask.h"
#include "TEST_NfcFlashTypeAdapter.h"

/*------------------------------------------------------------------------------
    MACROS DECLARATION
------------------------------------------------------------------------------*/
typedef struct _NFC_PENDING_FCMD_INFO_
{
    U32 bsPendPtr:8;
    U32 bsIsPending:1;
    U32 bsRsv:23;
    FCMD_REQ_ENTRY tPendFCMD;
}NFC_PENDING_FCMD_INFO;

typedef struct _NFC_PENDING_FCMDQ_
{
    FCMD_REQ_ENTRY aPendFCmd[SUBSYSTEM_LUN_MAX][FCMD_REQ_PRI_NUM];
}NFC_PENDING_FCMDQ;

typedef struct _NFC_FCMD_MONITOR_ITEM_
{
    U32 bsPhyBlk:16;
    U32 bsPhyPage:16;
    
    U32 bsReqType:8;
    U32 bsReqSubType:8;
    U32 bsVthShiftRd:8;
    U32 bsBlkMod:8;

    U32 bsSlcVthRetry:8;
    U32 bsMlcVthRetry:8;
    U32 bsSlcVthRetryDft:8;
    //U32 bsMlcVthRetryDft:8;
    U32 bsMlcVthRetryDft:7;
    U32 bsReadUppEn:1;
}NFC_FCMD_MONITOR_ITEM;

typedef struct _NFC_FCMD_MONITOR_
{
    NFC_FCMD_MONITOR_ITEM aFMItem[SUBSYSTEM_LUN_MAX][FCMD_REQ_PRI_NUM];
}NFC_FCMD_MONITOR;

typedef enum _LOCAL_FCMD_REQ_STS_
{
    LOCAL_FCMD_REQ_STS_INIT = 0,
    LOCAL_FCMD_REQ_STS_ALLOC,
    LOCAL_FCMD_REQ_STS_PUSH,
    LOCAL_FCMD_REQ_STS_POP,
    LOCAL_FCMD_REQ_STS_RDY
}LOCAL_FCMD_REQ_STS;

/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR FCMD_REQ *g_pLocalFCmdReq;    // Allocated from shared Dsram1
extern GLOBAL MCU12_VAR_ATTR volatile FCMD_REQSTS   *g_pLocalFCmdReqSts; // Allocated from OTFB
extern GLOBAL MCU12_VAR_ATTR volatile FCMD_REQ_DPTR *g_pLocalFCmdReqDptr;// Allocated from shared Dsram1

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
U8 TEST_NfcFCmdQGetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri);
U32 TEST_NfcFCmdQGetReqStsAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
void TEST_NfcFCmdQSetReqEntryRdy(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
BOOL TEST_NfcFCmdQIsWptrFree(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
FCMD_REQ_ENTRY *TEST_NfcFCmdQAllocReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
void TEST_NfcFCmdQPushReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
void TEST_NfcFCmdQAdaptPhyBlk(FCMD_REQ_ENTRY *ptReqEntry);
U16 TEST_NfcFCmdQGetPhyBlk(FCMD_REQ_ENTRY *ptReqEntry);
BOOL TEST_NfcIsSLCMode(FCMD_FLASH_DESC *pFlashDesc);

#endif

