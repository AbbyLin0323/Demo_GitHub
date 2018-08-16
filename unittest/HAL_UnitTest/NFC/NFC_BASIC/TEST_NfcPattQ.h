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
Filename    : TEST_NfcPattQ.h
Version     : Ver 1.0
Author      : abby
Date        : 2016.09.03
Description : compile by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_PATTQ_H_
#define _HAL_NFC_TEST_PATTQ_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_Xtensa.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
#define PATTQ_DEPTH         (2)//2 is enough

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
//6 DWs
typedef struct _BASIC_PATT_FEATURE_
{
    /* DW1 */
    U32 bsPuStart:8;         
    U32 bsPuEnd:8;           
    U32 bsLunStart:8;       
    U32 bsLunEnd:8;

    /* DW2 */
    U32 bsBlkStart:16;
    U32 bsBlkEnd:16;

    /* DW3 */
    U32 bsPageStart:16;
    U32 bsPageEnd:16;

    /* DW4 Byte0 */
    U32 bsPatternId:8;          // 0~255

    /* DW4 Byte1 */
    U32 bsDataCheckEn:1;        // TRUE : enable data init and data check
    U32 bsSsuEn:1;              // TRUE : enable ssu update
    U32 bsCacheStsEn:1;         // TRUE : enable cache status update
    U32 bsSsu0DramEn:1;         // TRUE : SSU0 update to DRAM; FALSE : OTFB
    U32 bsSsu1DramEn:1;         // TRUE : SSU1 update to DRAM; FALSE : OTFB
    U32 bsRedDramEn:1;          // TRUE : Red update to DRAM; FALSE : OTFB
    U32 bsLbaChk:1;             // TRUE : Check LBA
    U32 bsDecFifoEn:1;

    /* DW4 Byte2 */
    U32 bsTlcMode:1;
    U32 bsSinglePln:1;
    U32 bsRawDataRead:1;
    U32 bsLowPageMode:1;
    U32 bsErrInjEn:1;
    U32 bsDsIndex:3;            // Choose DS syntax entry
    U32 bsEMEnable:1;
    U32 bsSnapRead:1;           // enable snap read
    U32 bsRsv1:6;

    NFC_ERR_INJ tErrInj;        //2DW
}BASIC_PATT_FEATURE;

typedef enum _TEST_NFC_PATTERN_TYPE_
{
    P_RESET = 0,
    P_READ_ID,
    P_NORMAL_SET_FEAT,
    P_NORMAL_GET_FEAT,
    P_PIO_SET_FEAT,
    P_PIO_GET_FEAT,
    P_SINGLE_PLN,       //ID 6
    P_MULTI_PLN,
    P_PART_READ,        //ID 8
    P_SING_PLN_CCL_READ,
    P_CHANGE_COL_READ,
    P_RED_ONLY_READ,
    P_SSU_CS,           //ID 12
    P_RED_UPDATE,
    P_SSU_UPDATE,
    P_ERR_INJ_DEC_REP,
    P_RETRY,
    P_PU_BITMAP,
    P_LUN_BITMAP,       //ID 18
    P_TLC_COPY,
    P_READ_STS,
    P_LDPC_MAT_DL,
    P_LBA_CHK,
    P_HOST_WR,
    P_TIMING_MODE,
    //for extend pattern
    P_EXT_ERASE,
    P_EXT_WRITE,
    P_EXT_READ,
    P_TYPE_CNT
}TEST_NFC_PATTERN_TYPE;

typedef struct _BASIC_PATT_ENTRY_
{
    BASIC_PATT_FEATURE tPattFeature;
}BASIC_PATT_ENTRY;

typedef struct _BASIC_PATTQ_
{
    BASIC_PATT_ENTRY aPattQ[PATTQ_DEPTH];
}BASIC_PATTQ;

typedef enum _NFC_BASIC_PATTQ_STS_
{
    BASIC_PATTQ_STS_INIT = 0,
    BASIC_PATTQ_STS_ALLOC,
    BASIC_PATTQ_STS_PUSH,
    BASIC_PATTQ_STS_POP
}BASIC_PATTQ_STS;

typedef struct _BASIC_PATT_STSQ_
{
    U8 aPattStsQ[PATTQ_DEPTH];
}BASIC_PATT_STSQ;

typedef struct _BASIC_PATTQ_DPTR_
{
    U8 ucWptr;
    U8 ucRptr;
}BASIC_PATTQ_DPTR;

extern GLOBAL MCU12_VAR_ATTR BASIC_PATTQ *g_pPattQ; 
extern GLOBAL MCU12_VAR_ATTR volatile BASIC_PATT_STSQ *g_pPattStsQ;
extern GLOBAL MCU12_VAR_ATTR volatile BASIC_PATTQ_DPTR *g_pPattQDptr;

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void TEST_NfcPattQInit(void);
U8 TEST_NfcPattQGetWptr(void);
BOOL TEST_NfcPattQIsWptrFree(U8 ucWptr);
BASIC_PATT_ENTRY *TEST_NfcPattQAllocEntry(U8 ucWptr);
void TEST_NfcPattQPushEntry(U8 ucWptr);
BOOL TEST_NfcPattQIsPushed(void);
BASIC_PATT_ENTRY *TEST_NfcPattQPopEntry(void);
void TEST_NfcPattQRecycleEntry(void);


#endif

