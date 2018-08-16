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
Description :
*******************************************************************************/
#ifndef __XORUT_CONFIG_H__
#define __XORUT_CONFIG_H__

#include "BaseDef.h"
#include "HAL_XOR_Common.h"

#define XORUT_TEST_TASK_CNT (1 * XORE_CNT)
#define XORUT_TEST_TASK_CNT_MAX 6

/*  replace by abby
#ifdef FLASH_3D_MLC
#define XORUT_PLN_PER_LUN PLN_PER_LUN
#define XORUT_PLN_PER_LUN_BITS PLN_PER_LUN_BITS
#else
#define XORUT_PLN_PER_LUN PLN_PER_LUN
#define XORUT_PLN_PER_LUN_BITS PLN_PER_LUN_BITS
#endif
*/
#define XORUT_PLN_PER_LUN PLN_PER_LUN
#define XORUT_PLN_PER_LUN_BITS PLN_PER_LUN_BITS

typedef enum _XORUT_PATTERN
{
    XORUT_NFC_PROTECT = 0,
    XORUT_TLC_NFC_PROTECT,
    XORUT_3DMLC_NFC_PROTECT,
    XORUT_NFC_RECOVER,
    XORUT_BPS_PROTECT,
    XORUT_BPS_RECOVER,
    XORUT_BPS_RECOVER_WITH_NFC_READ,
    XORUT_BPS_RECOVER_WITH_TLC_NFC_READ,
    XORUT_STORE_TO_DRAM,
    XORUT_LOAD_FROM_DRAM,
    XORUT_LOAD_THEN_STORE,
    XORUT_PATTERN_CNT,
    XORUT_PATTERN_ASSERT = XORUT_PATTERN_CNT
}XORUT_PATTERN;

typedef struct _XORUT_BASIC_TEST_PATTERN_CONTROL
{
    U8 ucPatternCount;
    U8 ucPatternId[XORUT_PATTERN_CNT];
}XORUT_BASIC_TEST_PATTERN_CTRL;

typedef struct _XORUT_BLOCK_INFO_OF_PU
{
    U8 ucPuNum;
    U8 aBlockNum[NFC_LUN_MAX_PER_PU];
}XORUT_BLOCK_INFO_OF_PU;

extern U16 g_usXorProtectRatio;
extern U32 g_ulXorPageSize;
// Single page is Single-Plane page for MLC, is a lower page, middle page or upper page for TLC.
extern U32 g_ulSinglePageSize;
extern U16 g_usRedunSizePerSinglePage;

extern U8 g_ucCheckDwPerSector;      // (1 ~ 128)
extern BOOL g_bIsSinglePlaneOperate;
extern U8 g_ucPuCount;
extern U8 g_ucLunPerPu;
extern U8 g_ucTestXoreId;
extern BOOL g_bIsMultiXoreTest;
extern U8 g_ucTestTaskCnt;
extern BOOL g_IsFixedMixPattern;

extern XORUT_BASIC_TEST_PATTERN_CTRL g_tBasicTestPatternCtrl;
extern XORUT_BASIC_TEST_PATTERN_CTRL g_aBasicTestPatternCtrl[XORUT_TEST_TASK_CNT_MAX];
extern XORUT_BLOCK_INFO_OF_PU g_aXorPageLocationTable[XORUT_TEST_TASK_CNT_MAX][NFC_PU_MAX];

void XorUt_CheckTestConfig(void);

#if defined(COSIM)
typedef volatile struct _COSIM_TRACER_ERROR_REPORTER
{
    U8 bsXorUtAssertError : 1;
    U8 bsSwitchCaseError : 1;
    U8 bsDataCheckError : 1;
    U8 bsNfcCmdError : 1;
    U8 bsGeneralError : 1;
    U8 bsConfigCheckOk : 1;
    U8 bsRecoverReadDataOk : 1;
    U8 bsAllTestTaskDone : 1;

    U8  bsCurTestPatternId : 4;
    U8  bsPassedTestPatternId : 4;
    U16 bsXoreIdOfCurTestPattern : 4;
    U16 bsReserved : 12;
}COSIM_TRACER_ERROR_REPORTER;

extern COSIM_TRACER_ERROR_REPORTER *g_pCosimTracerErrorReporter;

void XorUt_FetchCosimTestConfig(void);
#elif (defined(SIM) || defined(FPGA) || defined(ASIC))
// These situation needn't to do anything.
#else 
#error "Running environment type must be defined!"
#endif

#endif/* __XORUT_CONFIG_H__ */
