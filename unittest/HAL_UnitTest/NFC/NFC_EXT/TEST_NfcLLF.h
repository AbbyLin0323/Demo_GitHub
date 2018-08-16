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
Filename    : TEST_NfcLLF.h
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : compile by MCU1
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_LLF_H_
#define _HAL_NFC_TEST_LLF_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "L2_TableBBT.h"
#include "L2_TableBlock.h"
#include "HAL_Dmae.h"
#include "TEST_NfcFCMDQ.h"
#include "TEST_NfcChecklist.h"
#include "TEST_NfcPerformance.h"

#define LOCAL_BLK_PER_PLN     (BLK_PER_PLN + RSVD_BLK_PER_LUN)//(BLK_PER_PLN - CHECK_LIST_BLK_NUM - 1)//checklist blk will be not erased

/****************************************************************************
    VARIABLE DEFINITION
****************************************************************************/
typedef enum _NFC_LOCAL_LLF_RESULT_
{
    NFC_LLF_SUCCESS,
    NFC_LLF_PENDING
}NFC_LOCAL_LLF_RESULT;

typedef enum _NFC_LOCAL_LLF_STATUS
{
    NFC_LLF_BBTBUILD= 0,
    NFC_LLF_RT_AT0_TABLE,
    NFC_LLF_BIT,
    NFC_LLF_FINISH
}NFC_LOCAL_LLF_STATUS;

typedef struct _BLOCK_INFO_
{
    /* Byte 0 */
    U32 bsFree:1;       //erase done, not write
    U32 bsValid:1;      //have valid data
    U32 bsOpen:1;       //not program full
    U32 bsClose:1;      //program all page
    //U32 bsBad:1;      //erase/program/read fail
    U32 bsRsv0:4;
    
    /* Byte 1-2 */
    U32 bsPPO:16;       //program page pointer. physical page

    U32 bsRsv1:8;
}BLOCK_INFO;

typedef struct _BLOCK_INFO_TABLE_
{
    BLOCK_INFO aInfoPerBlk[SUBSYSTEM_LUN_MAX][BLK_PER_LUN];
}BLOCK_INFO_TABLE;

#endif
