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
Filename    : TEST_NfcCheckList.h
Version     : Ver 1.0
Author      : abby
Date        : 2016.09.03
Description : compile by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_CHECK_LIST_H_
#define _HAL_NFC_TEST_CHECK_LIST_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPattQ.h"
#include "TEST_NfcFCMDQ.h"
#include "FW_BufAddr.h"
/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
#ifdef SIM
#define TEST_NFC_CHK_LIST_BASE      (DRAM_DATA_BUFF_MCU1_BASE)
#else
#define TEST_NFC_CHK_LIST_BASE      (0x43000000)//(OTFB_XOR_PARITY_BASE)
#endif

#define CHECKLIST_LOAD_CNTR_REG     (OTFB_XOR_REDUNDANT_BASE)//(0xfff90000), multi-use XOR memory, released after UT init

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
typedef enum _CHECK_LIST_FILE_TYPE_
{
    BASIC_CHK_LIST = 0,
    EXT_CHK_LIST,
    CHK_LIST_TYPE_CNT
}CHECK_LIST_FILE_TYPE;

typedef enum _CHECK_LIST_LOAD_METHOD_
{
    LOAD_DISABLE = 0,
    LOAD_AUTO_FROM_FLASH,       //load checklist file from flash into dram
    LOAD_MANU_FROM_TXT          //save checklist file from ext files into flash
}CHECK_LIST_LOAD_METHOD;

typedef struct _CHECK_LIST_LOAD_CNTR_
{
    U32 bsLoadMethod:8;
    U32 bsFileType:8;
    U32 bsLoadTrigger:1;
    U32 bsRsv:15;
}CHECK_LIST_LOAD_CNTR_REG;

/* check list file attribution:1DW */
typedef struct _CHECK_LIST_FILE_ATTR_
{
    U32 bsFileType:4;           // 0:basic 1:extend(FCMD)
    U32 bsLastFile:1;           // 1: End Pattern
    U32 bsRsv0:27;
}CHECK_LIST_FILE_ATTR;

typedef struct _CHECK_LIST_BASIC_FILE_
{
    /* DW0: File Attribution */
    CHECK_LIST_FILE_ATTR tFlieAttr;
    BASIC_PATT_ENTRY tBasicPattEntry;
}CHECK_LIST_BASIC_FILE;

typedef struct _CHECK_LIST_EXT_FILE_
{
    /* DW0: File Attribution */
    CHECK_LIST_FILE_ATTR tFlieAttr;
    
    /* DW1-12 */
    FCMD_REQ_ENTRY tFCmdReqEntry;
}CHECK_LIST_EXT_FILE;

typedef struct _MANU_UPT_STATUS_
{
    U8 aStatusAddr[SUBSYSTEM_LUN_MAX][FCMDQ_DEPTH];
}MANU_UPT_STATUS;

#define CHECK_LIST_DRAM_SZ          (100*1024*1024)//100M
#ifdef SIM
#define CHECK_LIST_FILE_NUM_MAX     (CHECK_LIST_DRAM_SZ/sizeof(CHECK_LIST_EXT_FILE))//(65536/8)
#else
#define CHECK_LIST_FILE_NUM_MAX     (CHECK_LIST_DRAM_SZ/sizeof(CHECK_LIST_EXT_FILE))//100M//(65536)//((OTFB_ADDR_MAX - TEST_NFC_CHK_LIST_BASE)/sizeof(CHECK_LIST_EXT_FILE))//(24576)
#endif

#define CHECK_LIST_FILE_SZ_MAX      (CHECK_LIST_FILE_NUM_MAX * sizeof(CHECK_LIST_EXT_FILE))//0x340000
#define CHECK_LIST_BLK_NUM          (CHECK_LIST_DRAM_SZ/(PHYPG_SZ*PG_PER_SLC_BLK))        
extern GLOBAL MCU12_VAR_ATTR U32 g_ulSsuInOtfbBase;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulSsu1OtfbBase;
extern GLOBAL MCU12_VAR_ATTR volatile U32 *g_pCheckListPtr;

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
U32* TEST_NfcGetChklistFile(void);
U8 TEST_NfcGetCheckListFileType(void);
void TEST_NfcGetNextChklistFile(U8 ucCurFileType);
void MCU1_DRAM_TEXT TEST_NfcCheckListAllocDram(U32 *pFreeDRAMBase);

#endif

/*  end of this file  */
