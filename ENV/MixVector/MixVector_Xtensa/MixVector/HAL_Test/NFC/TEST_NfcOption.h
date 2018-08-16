/****************************************************************************
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
*****************************************************************************
 * File Name    : TEST_NfcOption.h
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#ifndef _TEST_NFC_OPTION_H_
#define _TEST_NFC_OPTION_H_
#include "BaseDef.h"


/* MACRO FOR FPGA/ASIC/XTMP TEST */
#define TEST_PU_NUM         4
#define TEST_PAGE_NUM       512
#define TEST_BLOCK_START    0
#define TEST_BLOCK_END      1
#define TEST_DRAM_EN        FALSE
#define TEST_DATA_CHECK_EN  TRUE
#define TEST_DRQDWQ_EN      FALSE
#define TEST_SSU_CS_EN      FALSE
#define TEST_PATTERN_ID     0

#define TEST_SEC_START      0
#define TEST_SEC_LEN        64


/* REGISTER (8DW) 0x1ff812a0 ~ 0x1ff812bc  */
typedef struct NFC_REG_OPTION_
{
    // DWORD 0
    U32 bsPuNum:6;                 // 1~32
    U32 bsLunNum:4;                // 1~4
    U32 bsBlockNum:12;             // 1~1024
    U32 bsPageNum:10;              // 1~512

    // DWORD 1
    U32 usSecStart:16;          // 0~62
    U32 usSecLen:16;            // 2~64

    // DWORD 2
    U32 bsPatternId:8;          // 0~255
    U32 bsDramInitDisable:1;    // TRUE : diable DRAM init
    U32 bsDxqModeEn:1;          // TRUE : process DRQ/DWQ operation
    U32 bsDataCheckEn:1;        // TRUE : enable data init and data check
    U32 bsSsuCsEn:1;            // TRUE : enable ssu and cache status check
    U32 bsRes:20;
}NFC_REG_OPTION;

U32 TEST_NfcGetPuNum(void);
U32 TEST_NfcGetLunNum(void);
U32 TEST_NfcGetPageNum(void);
U32 TEST_NfcGetBlockStart(void);
U32 TEST_NfcGetBlockEnd(void);
BOOL TEST_NfcDataCheckEn(void);
BOOL TEST_NfcDxqEn(void);
U16 TEST_NfcGetSecStart(void);
U16 TEST_NfcGetSecLen(void);
U8 TEST_NfcGetPatternId(void);
BOOL TEST_NfcDramInitEn(void);
BOOL TEST_NfcSsuCsEn(void);

#endif /*  */
