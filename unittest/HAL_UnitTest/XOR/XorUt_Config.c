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
#include "XorUt_Config.h"
#include "XorUt_Common.h"
#include "HAL_ParamTable.h"

GLOBAL U16 g_usXorProtectRatio = 4;
GLOBAL U32 g_ulXorPageSize = 16 * KB_SIZE;
// Single page is Single-Plane page for MLC, is a lower page, middle page or upper page for TLC.
GLOBAL U32 g_ulSinglePageSize = 16 * KB_SIZE;
GLOBAL U16 g_usRedunSizePerSinglePage = RED_SZ;

GLOBAL U8 g_ucCheckDwPerSector = 2;      // (1 ~ 128)
GLOBAL BOOL g_bIsSinglePlaneOperate = FALSE;
GLOBAL U8 g_ucPuCount = 2;
GLOBAL U8 g_ucLunPerPu = 1;
GLOBAL U8 g_ucTestXoreId = 0;
GLOBAL BOOL g_bIsMultiXoreTest = TRUE;
GLOBAL U8 g_ucTestTaskCnt = XORUT_TEST_TASK_CNT;
GLOBAL BOOL g_IsFixedMixPattern = FALSE;

GLOBAL XORUT_BASIC_TEST_PATTERN_CTRL g_tBasicTestPatternCtrl =
{
    XORUT_PATTERN_CNT,
    {
        XORUT_TLC_NFC_PROTECT, XORUT_NFC_PROTECT, XORUT_NFC_RECOVER, XORUT_BPS_RECOVER_WITH_NFC_READ,
        XORUT_BPS_PROTECT, XORUT_BPS_RECOVER, XORUT_STORE_TO_DRAM, XORUT_LOAD_FROM_DRAM, XORUT_LOAD_THEN_STORE
    }
};

GLOBAL XORUT_BASIC_TEST_PATTERN_CTRL g_aBasicTestPatternCtrl[XORUT_TEST_TASK_CNT_MAX] =
{
    {
        1,
        {
            XORUT_NFC_PROTECT
        }
    },

    {
        2,
        {
            XORUT_NFC_PROTECT, XORUT_NFC_RECOVER
        }
    },

    {
        1,
        {
            XORUT_BPS_PROTECT
        }
    },

    {
        1,
        {
            XORUT_BPS_RECOVER
        }
    },

    {
        1,
        {
            XORUT_STORE_TO_DRAM
        }
    },

{
        2,
    { 
            XORUT_LOAD_FROM_DRAM, XORUT_LOAD_THEN_STORE
        }
    }
};

GLOBAL XORUT_BLOCK_INFO_OF_PU g_aXorPageLocationTable[XORUT_TEST_TASK_CNT_MAX][NFC_PU_MAX] =
{
    {
        { 0, { 0, 0, 0, 0 } }, { 1, { 0, 0, 0, 0 } }, { 2, { 0, 0, 0, 0 } }, { 3, { 0, 0, 0, 0 } },
        { 4, { 0, 0, 0, 0 } }, { 5, { 0, 0, 0, 0 } }, { 6, { 0, 0, 0, 0 } }, { 7, { 0, 0, 0, 0 } },
    },

    {
        { 1, { 1, 1, 1, 1 } }, { 2, { 1, 1, 1, 1 } }, { 3, { 1, 1, 1, 1 } }, { 4, { 1, 1, 1, 1 } },
        { 5, { 1, 1, 1, 1 } }, { 6, { 1, 1, 1, 1 } }, { 7, { 1, 1, 1, 1 } }, { 0, { 1, 1, 1, 1 } }, 
    },

    {
        { 2, { 2, 2, 2, 2 } }, { 3, { 2, 2, 2, 2 } }, { 4, { 2, 2, 2, 2 } }, { 5, { 2, 2, 2, 2 } },
        { 6, { 2, 2, 2, 2 } }, { 7, { 2, 2, 2, 2 } }, { 0, { 2, 2, 2, 2 } }, { 1, { 2, 2, 2, 2 } },
    },

    {
        { 3, { 3, 3, 3, 3 } }, { 4, { 3, 3, 3, 3 } }, { 5, { 3, 3, 3, 3 } }, { 6, { 3, 3, 3, 3 } },
        { 7, { 3, 3, 3, 3 } }, { 0, { 3, 3, 3, 3 } }, { 1, { 3, 3, 3, 3 } }, { 2, { 3, 3, 3, 3 } },
    },

    {
        { 4, { 4, 4, 4, 4 } }, { 5, { 4, 4, 4, 4 } }, { 6, { 4, 4, 4, 4 } }, { 7, { 4, 4, 4, 4 } },
        { 0, { 4, 4, 4, 4 } }, { 1, { 4, 4, 4, 4 } }, { 2, { 4, 4, 4, 4 } }, { 3, { 4, 4, 4, 4 } },
    },

    {
        { 5, { 5, 5, 5, 5 } }, { 6, { 5, 5, 5, 5 } }, { 7, { 5, 5, 5, 5 } }, { 0, { 5, 5, 5, 5 } },
        { 1, { 5, 5, 5, 5 } }, { 2, { 5, 5, 5, 5 } }, { 3, { 5, 5, 5, 5 } }, { 4, { 5, 5, 5, 5 } },
    },
};

void XorUt_CheckTestConfig()
{
    U32 i = 0;

    XORUT_ASSERT((g_usXorProtectRatio > 1) && (g_usXorProtectRatio <= 256));
#ifdef FLASH_TLC
    XORUT_ASSERT((g_usXorProtectRatio <= PLN_CNT_PER_ROW) ||
        ((g_usXorProtectRatio > PLN_CNT_PER_ROW) && (g_usXorProtectRatio % PLN_CNT_PER_ROW == 0)));
#endif
    XORUT_ASSERT((g_ulXorPageSize == (16 * KB_SIZE)) || (g_ulXorPageSize == (48 * KB_SIZE)));
    XORUT_ASSERT((g_ucCheckDwPerSector > 0) && (g_ucCheckDwPerSector <= SEC_SIZE_DW));

    XORUT_ASSERT((g_bIsSinglePlaneOperate == TRUE) || (g_bIsSinglePlaneOperate == FALSE));
    XORUT_ASSERT(g_ucTestXoreId < XORE_CNT);
    XORUT_ASSERT((g_ucLunPerPu > 0) && (g_ucLunPerPu <= NFC_LUN_PER_PU));
    XORUT_ASSERT((g_ucPuCount > 0) && (g_ucPuCount <= SUBSYSTEM_PU_NUM));
    XORUT_ASSERT((g_tBasicTestPatternCtrl.ucPatternCount > 0) && (g_tBasicTestPatternCtrl.ucPatternCount <= XORUT_PATTERN_CNT));

    for (i = 0; i < g_tBasicTestPatternCtrl.ucPatternCount; ++i)
    {
        XORUT_ASSERT(g_tBasicTestPatternCtrl.ucPatternId[i] < XORUT_PATTERN_CNT);
    }

    XORUT_ASSERT((g_bIsMultiXoreTest == TRUE) || (g_bIsMultiXoreTest == FALSE));
    XORUT_ASSERT((g_ucTestTaskCnt > 0) && (g_ucTestTaskCnt <= XORUT_TEST_TASK_CNT));
    XORUT_ASSERT((g_IsFixedMixPattern == TRUE) || (g_IsFixedMixPattern == FALSE));

    XorUt_Log(XOR_UT_CONFIG_CHECK_OK, NULL);

    return;
}

#if defined(COSIM)
typedef volatile struct _COSIM_TEST_CONFIG_DW0
{
    U16 usXorProtectRatio;
    U8  ucXorPageSizeKB;
    U8  ucCheckDwPerSector;
}COSIM_TEST_CONFIG_DW0;

typedef volatile struct _COSIM_TEST_CONFIG_DW1
{
    U8  bsIsSinglePlaneOperate : 1;
    U8  bsOnlyTestXoreId : 4;
    U8  bsLunPerPu : 3;

    U8  ucPuCount;

    U8  bsBasicTestPatternCnt : 4;
    U8  bsBasicTestPattern1Id : 4;

    U8  bsBasicTestPattern2Id : 4;
    U8  bsBasicTestPattern3Id : 4;
}COSIM_TEST_CONFIG_DW1;

typedef volatile struct _COSIM_TEST_CONFIG_DW2
{
    U8  bsBasicTestPattern4Id : 4;
    U8  bsBasicTestPattern5Id : 4;

    U8  bsBasicTestPattern6Id : 4;
    U8  bsBasicTestPattern7Id : 4;

    U8  bsBasicTestPattern8Id : 4;
    U8  bsBasicTestPattern9Id : 4;

    U8  bsBasicTestPattern10Id : 4;
    U8  bsBasicTestPattern11Id : 4;
}COSIM_TEST_CONFIG_DW2;

typedef volatile struct _COSIM_TEST_CONFIG_DW3
{
    U8  bsIsMultiXoreTest : 1;
    U8  bsTestTaskCnt : 5;
    U8  bsIsFixedMixPattern : 1;
    U8  bsResv0 : 1;

    U8  ucResv1;
    U16 usResv2;
}COSIM_TEST_CONFIG_DW3;

COSIM_TRACER_ERROR_REPORTER *g_pCosimTracerErrorReporter = NULL;

U32 XorUt_GetXoreLastReservedDwAddr(U32 ulXoreId)
{
    XORUT_ASSERT(ulXoreId < (XORE_CNT - 1));      // The last reserved DW of last XOR engine is used for logic debug.

    return ((REG_BASE_XOR + (ulXoreId + 1) * sizeof(XORE_CFG_REG)) - sizeof(U32));
}

void XorUt_FetchCosimTestConfig(void)
{
    U32 ulLastReservedDwAddrOf7thXore = (REG_BASE_XOR + (6 + 1) * sizeof(XORE_CFG_REG)) - sizeof(U32);
    g_pCosimTracerErrorReporter = (COSIM_TRACER_ERROR_REPORTER *)ulLastReservedDwAddrOf7thXore;
    COM_MemZero((U32 *)g_pCosimTracerErrorReporter, (sizeof(COSIM_TRACER_ERROR_REPORTER) >> 2));

    COSIM_TEST_CONFIG_DW0 *pCosimTestConfigDw0 = (COSIM_TEST_CONFIG_DW0 *)XorUt_GetXoreLastReservedDwAddr(0);

    XORUT_ASSERT(pCosimTestConfigDw0 != NULL);

    g_usXorProtectRatio = pCosimTestConfigDw0->usXorProtectRatio;
    g_ulXorPageSize = (pCosimTestConfigDw0->ucXorPageSizeKB) * KB_SIZE;
    g_ucCheckDwPerSector = pCosimTestConfigDw0->ucCheckDwPerSector;

    COSIM_TEST_CONFIG_DW1 *pCosimTestConfigDw1 = (COSIM_TEST_CONFIG_DW1 *)XorUt_GetXoreLastReservedDwAddr(1);

    XORUT_ASSERT(pCosimTestConfigDw1 != NULL);

    g_bIsSinglePlaneOperate = pCosimTestConfigDw1->bsIsSinglePlaneOperate;
    g_ucTestXoreId = pCosimTestConfigDw1->bsOnlyTestXoreId;
    g_ucLunPerPu = pCosimTestConfigDw1->bsLunPerPu;
    g_ucPuCount = pCosimTestConfigDw1->ucPuCount;
    g_tBasicTestPatternCtrl.ucPatternCount = pCosimTestConfigDw1->bsBasicTestPatternCnt;

    g_tBasicTestPatternCtrl.ucPatternId[0] = pCosimTestConfigDw1->bsBasicTestPattern1Id;
    g_tBasicTestPatternCtrl.ucPatternId[1] = pCosimTestConfigDw1->bsBasicTestPattern2Id;
    g_tBasicTestPatternCtrl.ucPatternId[2] = pCosimTestConfigDw1->bsBasicTestPattern3Id;

    COSIM_TEST_CONFIG_DW2 *pCosimTestConfigDw2 = (COSIM_TEST_CONFIG_DW2 *)XorUt_GetXoreLastReservedDwAddr(2);

    XORUT_ASSERT(pCosimTestConfigDw2 != NULL);

    g_tBasicTestPatternCtrl.ucPatternId[3] = pCosimTestConfigDw2->bsBasicTestPattern4Id;
    g_tBasicTestPatternCtrl.ucPatternId[4] = pCosimTestConfigDw2->bsBasicTestPattern5Id;
    g_tBasicTestPatternCtrl.ucPatternId[5] = pCosimTestConfigDw2->bsBasicTestPattern6Id;
    g_tBasicTestPatternCtrl.ucPatternId[6] = pCosimTestConfigDw2->bsBasicTestPattern7Id;

    COSIM_TEST_CONFIG_DW3 *pCosimTestConfigDw3 = (COSIM_TEST_CONFIG_DW3 *)XorUt_GetXoreLastReservedDwAddr(3);

    XORUT_ASSERT(pCosimTestConfigDw3 != NULL);

    g_bIsMultiXoreTest = pCosimTestConfigDw3->bsIsMultiXoreTest;
    g_ucTestTaskCnt = pCosimTestConfigDw3->bsTestTaskCnt;
    g_IsFixedMixPattern = pCosimTestConfigDw3->bsIsFixedMixPattern;

    return;
}
#elif (defined(SIM) || defined(FPGA) || defined(ASIC))
// These situation needn't to do anything.
#else 
#error "Running environment type must be defined!"
#endif
