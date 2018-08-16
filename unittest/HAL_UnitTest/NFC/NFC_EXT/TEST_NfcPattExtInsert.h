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
Filename    : TEST_NfcPattExtInsert.h
Version     : Ver 1.0
Author      : abby
Date        : 20160907
Description : compile by MCU1
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_FLASH_TEST_EXT_PATTERN_INSERT_H_
#define _HAL_FLASH_TEST_EXT_PATTERN_INSERT_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "Proj_Config.h"
#include "TEST_NfcCheckList.h"
#include "TEST_NfcLLF.h"
#include "TEST_NfcExtDataCheck.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
typedef enum _INSERT_FCMD_TYPE_
{
    NO_INSERT = 0,
    INSERT_ERASE,
    INSERT_DUMMY_WRITE,
    INSERT_READ_BEFORE_CCL,
}INSERT_FCMD_TYPE;

void MCU1_DRAM_TEXT TEST_NfcInsertFCmdInit(void);
U32 TEST_NfcIsNeedInsertFCmd(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY *pFCmdReq);
void TEST_NfcHandleInsertFCmd(U8 ucTLun, FCMD_REQ_ENTRY *pFCmdReq);
void TEST_NfcFakeExtChecklist(CHECK_LIST_EXT_FILE *pCheckList);
U32 TEST_NfcFakeRandR(CHECK_LIST_EXT_FILE *pCheckList);
U32 TEST_NfcFakeSeqW(CHECK_LIST_EXT_FILE *pCheckList);
U32 TEST_NfcFakeSeqR(CHECK_LIST_EXT_FILE *pCheckList);
U32 TEST_NfcFakeExtCopyPrepare(CHECK_LIST_EXT_FILE *pCheckList);
U32 TEST_NfcFakeExtCopy(CHECK_LIST_EXT_FILE *pCheckList);

#endif


