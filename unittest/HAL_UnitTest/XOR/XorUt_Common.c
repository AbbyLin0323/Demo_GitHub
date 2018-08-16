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
#include "XorUt_Common.h"
#include "XorUt_Config.h"
#include "XorUt_Nfc_Interface.h"
#include "HAL_XOR_Common.h"
#include "HAL_FlashDriverExt.h"

#define SEC_PER_SINGLE_PAGE     (g_ulSinglePageSize / SEC_SIZE)
#define XORUT_DRAM_PER_XOR_PAGE (48 * KB_SIZE)
#define XORUT_DRAM_BASE       DRAM_DATA_BUFF_MCU2_BASE
#define XORUT_INVALID_XORE_ID 0x0F

U8 ucLoopCnt = 0;

U32 XorUt_GetDataPattern(U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));

    static U32 l_aUserDataPattern[8] = { 0xA5A5A5A5, 0x12345678, 0xD7D6D9E2, 0xABCDEF01, 0x7EC029B5,
        0xDCBA9876, 0xA9B6C3D7, 0xF0E1D2C3 };

    return (l_aUserDataPattern[ulXorPageNumInTotal % 8] + (ulXorPageNumInTotal << 16) + ulXorPageNumInTotal);
}

// Prepare data of one XOR page. 
void XorUt_PageDataPrepare(U32 *pPageData, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(pPageData != NULL);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0;
    U32 ulDataPattern = XorUt_GetDataPattern(ulXorPageNumInTotal);
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        COM_MemSet(pPageData, g_ucCheckDwPerSector, ulDataPattern);
        pPageData += SEC_SIZE_DW;
    }

    return;
}

// Prepare data of one XOR page. Support Multi-Plane TLC. 
void XorUt_PageDataPrepareTlc(U32 *pPageData, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(pPageData != NULL);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulXorPageSize == (32 * KB_SIZE)) || (ulXorPageSize == (48 * KB_SIZE)));

    U32 i = 0, j = 0;
    U32 ulDataPattern = XorUt_GetDataPattern(ulXorPageNumInTotal);

    for (i = 0; i < (ulXorPageSize / g_ulSinglePageSize); ++i)
    {
        for (j = 0; j < SEC_PER_SINGLE_PAGE; ++j)
        {
            COM_MemSet(pPageData, g_ucCheckDwPerSector, ulDataPattern);
            pPageData += SEC_SIZE_DW;
        }

        if (g_bIsSinglePlaneOperate == FALSE)
        {
            pPageData += ((XORUT_PLN_PER_LUN - 1) * g_ulSinglePageSize) / sizeof(U32);
        }
    }


    return;
}

// Check data of one XOR page. For Multi-Plane TLC or Multi-Plane 3D MLC, we read the page to DRAM
// sequential, so this function is suitable for use. 
BOOL XorUt_PageDataCheck(U32 *pPageData, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(pPageData != NULL);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0, j = 0;
    U32 ulDataPattern = XorUt_GetDataPattern(ulXorPageNumInTotal);
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        for (j = 0; j < g_ucCheckDwPerSector; ++j)
        {
            if (pPageData[i * SEC_SIZE_DW + j] != ulDataPattern)
            {
                DBG_Printf("secnum: %d, dwnum: %d, read data: 0x%x, write data: 0x%x!\n\n", i, j, pPageData[i * SEC_SIZE_DW + j], ulDataPattern);
                return FALSE;
            }
        }
    }

    return TRUE;
}

// Clean data of one XOR page. 
void XorUt_PageDataClean(U32 *pPageData, U32 ulXorPageSize)
{
    XORUT_ASSERT(pPageData != NULL);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0;
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        COM_MemZero(pPageData, g_ucCheckDwPerSector);
        pPageData += SEC_SIZE_DW;
    }

    return;
}

void XorUt_PageRedunPrepare(U32 *pPageRedun, U32 ulXorPageSize, U32 ulXorPageCnt)
{
    XORUT_ASSERT(pPageRedun != NULL);
    XORUT_ASSERT((ulXorPageCnt > 0) && (ulXorPageCnt < g_usXorProtectRatio));
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0;
    U32 ulRedunCnt = (ulXorPageCnt * ulXorPageSize) / g_ulSinglePageSize;
    
    for (i = 0; i < ulRedunCnt; ++i)
    {
        COM_MemSet(pPageRedun, (g_usRedunSizePerSinglePage / sizeof(U32)), 0x0b55aa12);
        pPageRedun += (g_usRedunSizePerSinglePage / sizeof(U32));
    }
    
    return;
}

// Check redundant of one XOR page, maybe multi redundant for TLC.
BOOL XorUt_RedunCheck(U32 *pPageRedun, U32 ulXorPageSize)
{
    XORUT_ASSERT(pPageRedun != NULL);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0, j = 0;
    
    for (i = 0; i < (ulXorPageSize / g_ulSinglePageSize); ++i)
    {
        for (j = 0; j < (g_usRedunSizePerSinglePage / sizeof(U32)); ++j)
        {
            if (pPageRedun[i * (g_usRedunSizePerSinglePage / sizeof(U32)) + j] != 0x0b55aa12)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

void XorUt_RedunClean(U32 *pPageRedun, U32 ulXorPageSize, U32 ulXorPageCnt)
{
    XORUT_ASSERT(pPageRedun != NULL);
    XORUT_ASSERT((ulXorPageCnt > 0) && (ulXorPageCnt <= g_usXorProtectRatio));
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0;
    U32 ulRedunCnt = (ulXorPageCnt * ulXorPageSize) / g_ulSinglePageSize;
    
    for (i = 0; i < ulRedunCnt; ++i)
    {
        COM_MemZero(pPageRedun, (g_usRedunSizePerSinglePage / sizeof(U32)));
        pPageRedun += (g_usRedunSizePerSinglePage / sizeof(U32));
    }
    
    return;
}

// Prepare parity data to one XOR page. 
void XorUt_ParityDataPrepare(U32 *pParityData, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(pParityData != NULL);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));
    
    U32 i = 0, j = 0;
    U32 ulXorPageNumInTotal = ulStartXorPageNumInTotal;
    U32 ulParityPattern = 0;

    for (i = 0; i < (U32)(g_usXorProtectRatio - 1); ++i)
    {
        U32 ulDataPattern = XorUt_GetDataPattern(ulXorPageNumInTotal);
        ulParityPattern ^= ulDataPattern;
        ++ulXorPageNumInTotal;
    }
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        COM_MemSet(pParityData, g_ucCheckDwPerSector, ulParityPattern);
        pParityData += SEC_SIZE_DW;
    }

    return;
}

void XorUt_Log(XOR_UT_LOG_TYPE eXorUtLogType, U32 ulRecordValue)
{
    XORUT_ASSERT((eXorUtLogType >= 0) && (eXorUtLogType < XOR_UT_LOG_TYPE_ASSERT));

    switch (eXorUtLogType)
    {
#if defined(COSIM)
        case XOR_UT_CONFIG_CHECK_OK:
        {
            g_pCosimTracerErrorReporter->bsConfigCheckOk = TRUE;
            break;
        }
        case XOR_UT_DATA_CHECK_ERROR:
        {
            g_pCosimTracerErrorReporter->bsDataCheckError = TRUE;
            break;
        }
        case XOR_UT_ALL_TEST_TASK_DONE:
        {
            g_pCosimTracerErrorReporter->bsAllTestTaskDone = TRUE;
            break;
        }
        case XOR_UT_NFC_CMD_ERROR:
        {
            g_pCosimTracerErrorReporter->bsNfcCmdError = TRUE;
            break;
        }
        case XOR_UT_SWITCH_CASE_ERROR:
        {
            g_pCosimTracerErrorReporter->bsSwitchCaseError = TRUE;
            break;
        }
        case XOR_UT_GENERAL_ERROR:
        {
            g_pCosimTracerErrorReporter->bsGeneralError = TRUE;
            break;
        }
        case XOR_UT_RECOVER_READ_DATA_OK:
        {
            g_pCosimTracerErrorReporter->bsRecoverReadDataOk = TRUE;
            break;
        }
        case XOR_UT_RECORD_CUR_TEST_PATTERN_ID:
        {
            g_pCosimTracerErrorReporter->bsCurTestPatternId = ulRecordValue;
            break;
        }
        case XOR_UT_RECORD_PASSED_TEST_PATTERN_ID:
        {
            g_pCosimTracerErrorReporter->bsPassedTestPatternId = ulRecordValue;
            break;
        }
        case XOR_UT_RECORD_XORE_ID:
        {
            g_pCosimTracerErrorReporter->bsXoreIdOfCurTestPattern = ulRecordValue;
            break;
        }
        default:
        {
            g_pCosimTracerErrorReporter->bsSwitchCaseError = TRUE;
            DBG_Getch();
        }
#elif (defined(SIM) || defined(FPGA) || defined(ASIC))
        case XOR_UT_CONFIG_CHECK_OK:
        {
            DBG_Printf("XorUt: Config Check OK!\n");
            break;
        }
        case XOR_UT_DATA_CHECK_ERROR:
        {
            DBG_Printf("XorUt: Data Check Error!\n");
            break;
        }
        case XOR_UT_ALL_TEST_TASK_DONE:
        {
            DBG_Printf("\nXorUt: HAL XOR unit test End!\n\n");
            break;
        }
        case XOR_UT_NFC_CMD_ERROR:
        case XOR_UT_GENERAL_ERROR:
        {
            // These situation needn't to do anything.
            break;
        }
        case XOR_UT_SWITCH_CASE_ERROR:
        {
            DBG_Printf("XorUt: Switch Case Error!\n");
            break;
        }
        case XOR_UT_RECOVER_READ_DATA_OK:
        {
            DBG_Printf("XorUt: NFC Recover Read Data Ok!\n");
            break;
        }
        case XOR_UT_RECORD_CUR_TEST_PATTERN_ID:
        {
            //DBG_Printf("XorUt: Current Test Pattern ID: %d!\n", ulRecordValue);
            break;
        }
        case XOR_UT_RECORD_PASSED_TEST_PATTERN_ID:
        {
            DBG_Printf("XorUt: Passed Test Pattern ID: %d!\n", ulRecordValue);
            break;
        }
        case XOR_UT_RECORD_XORE_ID:
        {
            //DBG_Printf("XorUt: XOR Engine ID of Current Test Pattern: %d!\n", ulRecordValue);
            break;
        }
        default:
        {
            DBG_Printf("XorUt: Log Type Error!\n");
            DBG_Getch();
        }
#else 
#error "Running environment type must be defined!"
#endif
    }

    return;
}

// Check parity(size in XOR page). For Multi-Plane TLC, maybe need to change. 
BOOL XorUt_ParityDataCheck(U32 *pParityData, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(pParityData != NULL);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0, j = 0;
    U32 ulXorPageNumInTotal = ulStartXorPageNumInTotal;
    U32 ulParityPattern = 0;

    for (i = 0; i < (U32)(g_usXorProtectRatio - 1); ++i)
    {
        U32 ulDataPattern = XorUt_GetDataPattern(ulXorPageNumInTotal);
        ulParityPattern ^= ulDataPattern;
        ++ulXorPageNumInTotal;
    }
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        for (j = 0; j < g_ucCheckDwPerSector; ++j)
        {
            if (pParityData[i * SEC_SIZE_DW + j] != ulParityPattern)
            {
                DBG_Printf("Got Data = 0x%x, Right Data = 0x%x\n",pParityData[i * SEC_SIZE_DW + j], ulParityPattern);  //For Debug
                return FALSE;
            }
        }
    }

    return TRUE;
}

// Check parity(size in XOR page). Support Multi-Plane TLC. 
BOOL XorUt_ParityDataCheckTlc(U32 *pParityData, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(pParityData != NULL);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (32 * KB_SIZE)) || (ulXorPageSize == (48 * KB_SIZE)));

    U32 i = 0, j = 0, k = 0;
    U32 ulXorPageNumInTotal = ulStartXorPageNumInTotal;
    U32 ulParityPattern = 0;

    for (i = 0; i < (U32)(g_usXorProtectRatio - 1); ++i)
    {
        U32 ulDataPattern = XorUt_GetDataPattern(ulXorPageNumInTotal);
        ulParityPattern ^= ulDataPattern;
        ++ulXorPageNumInTotal;
    }

    for (i = 0; i < (ulXorPageSize / g_ulSinglePageSize); ++i)
    {
        for (j = 0; j < SEC_PER_SINGLE_PAGE; ++j)
        {
            for (k = 0; k < g_ucCheckDwPerSector; ++k)
            {
                if (pParityData[j * SEC_SIZE_DW + k] != ulParityPattern)
                {
                    return FALSE;
                }
            }
        }

        if (g_bIsSinglePlaneOperate == FALSE)
        {
            pParityData += (XORUT_PLN_PER_LUN * g_ulSinglePageSize) / sizeof(U32);
        }
    }

    return TRUE;
}

FLASH_ADDR XorUt_XorPageLocate(const XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable, U32 ulXorPageNumInXore)
{
    XORUT_ASSERT(pXorPageLocationTable != NULL);
    XORUT_ASSERT(ulXorPageNumInXore < g_usXorProtectRatio);

    FLASH_ADDR tFlashAddr = { 0 };
    U32 ulBlockNumShiftWithTlc = 500;

#if (defined(FLASH_TLC) || defined(FLASH_3D_MLC))
    ulBlockNumShiftWithTlc += XORUT_TEST_TASK_CNT;
#endif
    
    U32 ulVerticalLocation = ulXorPageNumInXore / (g_ucPuCount * g_ucLunPerPu * XORUT_PLN_PER_LUN);
    U32 ulHorizontalLocation = ulXorPageNumInXore % (g_ucPuCount * g_ucLunPerPu * XORUT_PLN_PER_LUN);

    U32 ulPuNumInSelfLocation = ulHorizontalLocation / (g_ucLunPerPu * XORUT_PLN_PER_LUN);
    tFlashAddr.ucPU = (pXorPageLocationTable[ulPuNumInSelfLocation].ucPuNum) % g_ucPuCount;
    U32 ulHorizontalLocationInPu = ulHorizontalLocation % (g_ucLunPerPu * XORUT_PLN_PER_LUN);
    tFlashAddr.ucLun = ulHorizontalLocationInPu / (XORUT_PLN_PER_LUN);
    tFlashAddr.bsPln = ulHorizontalLocationInPu % (XORUT_PLN_PER_LUN);
    tFlashAddr.usPage = ulVerticalLocation % PG_PER_BLK;

    tFlashAddr.usBlock = pXorPageLocationTable[ulPuNumInSelfLocation].aBlockNum[tFlashAddr.ucLun];

    tFlashAddr.usBlock += ulBlockNumShiftWithTlc + ucLoopCnt;

    return tFlashAddr;
}

FLASH_ADDR XorUt_Get3dMlcXorPageLocate(const XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable, U32 ulXorPageNumInXore)
{
    XORUT_ASSERT(pXorPageLocationTable != NULL);
    XORUT_ASSERT(ulXorPageNumInXore < g_usXorProtectRatio);

    FLASH_ADDR tFlashAddr = { 0 };
    U32 ulBlockNumShiftWithTlc = 500;

    U32 ulVerticalLocation = ulXorPageNumInXore / (g_ucPuCount * g_ucLunPerPu * XORUT_PLN_PER_LUN);
    U32 ulHorizontalLocation = ulXorPageNumInXore % (g_ucPuCount * g_ucLunPerPu * XORUT_PLN_PER_LUN);

    U32 ulPuNumInSelfLocation = ulHorizontalLocation / (g_ucLunPerPu * XORUT_PLN_PER_LUN);
    tFlashAddr.ucPU = (pXorPageLocationTable[ulPuNumInSelfLocation].ucPuNum) % g_ucPuCount;
    U32 ulHorizontalLocationInPu = ulHorizontalLocation % (g_ucLunPerPu * XORUT_PLN_PER_LUN);
    tFlashAddr.ucLun = ulHorizontalLocationInPu / (XORUT_PLN_PER_LUN);
    tFlashAddr.bsPln = ulHorizontalLocationInPu % (XORUT_PLN_PER_LUN);
    tFlashAddr.usPage = ulVerticalLocation % PG_PER_BLK;

    tFlashAddr.usBlock = pXorPageLocationTable[ulPuNumInSelfLocation].aBlockNum[tFlashAddr.ucLun];

    tFlashAddr.usBlock += ulBlockNumShiftWithTlc;

    return tFlashAddr;
}

FLASH_ADDR XorUt_GetTlcXorPageLocation(const XORUT_BLOCK_INFO_OF_PU * pXorPageLocationTable, U32 ulWordLineIndex,
    U32 ulPlaneNumInRow)
{
    XORUT_ASSERT(pXorPageLocationTable != NULL);
    XORUT_ASSERT(ulWordLineIndex < PG_PER_BLK);
    U32 ulEndPlnOfRow = (g_usXorProtectRatio < PLN_CNT_PER_ROW) ? g_usXorProtectRatio : PLN_CNT_PER_ROW;
    XORUT_ASSERT(ulPlaneNumInRow < ulEndPlnOfRow);

    FLASH_ADDR tFlashAddr = { 0 };

    U32 ulVerticalLocation = ulWordLineIndex;
    U32 ulHorizontalLocation = ulPlaneNumInRow;

    U32 ulPuNumInSelfLocation = ulHorizontalLocation / (g_ucLunPerPu * XORUT_PLN_PER_LUN);
    tFlashAddr.ucPU = (pXorPageLocationTable[ulPuNumInSelfLocation].ucPuNum) % g_ucPuCount;;
    U32 ulHorizontalLocationInPu = ulHorizontalLocation % (g_ucLunPerPu * XORUT_PLN_PER_LUN);
    tFlashAddr.ucLun = ulHorizontalLocationInPu / (XORUT_PLN_PER_LUN);
    tFlashAddr.bsPln = ulHorizontalLocationInPu % (XORUT_PLN_PER_LUN);
    tFlashAddr.usPage = ulVerticalLocation % PG_PER_BLK;

    tFlashAddr.usBlock = pXorPageLocationTable[ulPuNumInSelfLocation].aBlockNum[tFlashAddr.ucLun];
    tFlashAddr.usBlock += 400 + ucLoopCnt;

    return tFlashAddr;
}

U32 XorUt_GetPageDataSizePerXore()
{
    static U32 ulPageDataSizePerXore = 0;

    if (ulPageDataSizePerXore == 0)
    {
        U32 ulUserPagesSize = (g_usXorProtectRatio - 1) * XORUT_DRAM_PER_XOR_PAGE;
        U32 ulDestPageSize = XORUT_DRAM_PER_XOR_PAGE;
        U32 ulLoadStorePageSize = XORUT_DRAM_PER_XOR_PAGE;
        
        ulPageDataSizePerXore = ulUserPagesSize + ulDestPageSize + ulLoadStorePageSize;
    }
   
    return ulPageDataSizePerXore;
}

U32 XorUt_GetPageRedunSizePerXore()
{
    static U32 ulPageRedunSizePerXore = 0;

    if (ulPageRedunSizePerXore == 0)
    {
        U32 ulUserPagesSize = (g_usXorProtectRatio - 1) * XORUT_DRAM_PER_XOR_PAGE;
        U32 ulDestPageSize = XORUT_DRAM_PER_XOR_PAGE;
        U32 ulUserPagesRedunSize = (ulUserPagesSize / g_ulSinglePageSize) * g_usRedunSizePerSinglePage;
        U32 ulDestPageRedunSize = (ulDestPageSize / g_ulSinglePageSize) * g_usRedunSizePerSinglePage;
        U32 ulLoadStorePageSize = XORUT_DRAM_PER_XOR_PAGE;
        U32 ulLoadStoreRedunSize = (ulLoadStorePageSize / g_ulSinglePageSize) * g_usRedunSizePerSinglePage;
        
        ulPageRedunSizePerXore = ulUserPagesRedunSize + ulDestPageRedunSize + ulLoadStoreRedunSize;
    }
   
    return ulPageRedunSizePerXore;
}

U32 XorUt_GetPageDataAddr(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInXore)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
        
    U32 ulCurXorePageDataBaseAddr = XORUT_DRAM_BASE + ulXoreId * XorUt_GetPageDataSizePerXore();
    
    return (ulCurXorePageDataBaseAddr + ulXorPageNumInXore * ulXorPageSize);
}

U32 XorUt_GetPageDataAddrTlc(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInXore)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT((ulXorPageSize == (32 * KB_SIZE)) || (ulXorPageSize == (48 * KB_SIZE)));

    if (g_bIsSinglePlaneOperate == TRUE)
    {
        return XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, ulXorPageNumInXore);
    }
    else
    {
        U32 ulEvenXorPageNumAddr = XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize,
            (ulXorPageNumInXore - (ulXorPageNumInXore % XORUT_PLN_PER_LUN)));

        return (ulEvenXorPageNumAddr + (ulXorPageNumInXore % XORUT_PLN_PER_LUN) * g_ulSinglePageSize);
    }
}

U32 XorUt_GetDestPageDataAddr(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    return XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, (g_usXorProtectRatio - 1));
}

U32 XorUt_GetDestPageDataAddrTlc(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (32 * KB_SIZE)) || (ulXorPageSize == (48 * KB_SIZE)));

    return XorUt_GetPageDataAddrTlc(ulXoreId, ulXorPageSize, (g_usXorProtectRatio - 1));
}

U32 XorUt_GetLoadStoreDataAddr(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    return XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, g_usXorProtectRatio);
}

U32 XorUt_GetRedunBaseAddr()
{
    return (XORUT_DRAM_BASE + XORE_CNT * XorUt_GetPageDataSizePerXore());
}

U32 XorUt_GetPageRedunAddr(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInXore)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 ulCurXorePageRedunBaseAddr = XorUt_GetRedunBaseAddr() + ulXoreId * XorUt_GetPageRedunSizePerXore();
    U32 ulRedunSizePerXorPage = (ulXorPageSize / g_ulSinglePageSize) * g_usRedunSizePerSinglePage;
    
    return (ulCurXorePageRedunBaseAddr  + ulXorPageNumInXore * ulRedunSizePerXorPage);
}

U32 XorUt_GetParityPageRedunAddr(U32 ulXoreId)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);

    return HAL_XoreGetParityRedunOffset(ulXoreId) + OTFB_XOR_REDUNDANT_BASE;
}

U32 XorUt_GetDestPageRedunAddr(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    return XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, (g_usXorProtectRatio - 1));
}

U32 XorUt_GetLoadStoreRedunAddr(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    return XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, g_usXorProtectRatio);
}

U32 XorUt_GetStartXorPageNum(U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));

    return ((ulBrokenXorPageNumInTotal / g_usXorProtectRatio) * g_usXorProtectRatio);
}

BOOL XorUt_IsInSamePlane(U32 ulBrokenXorPageNumInTotal, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal / g_usXorProtectRatio) == 
        (ulXorPageNumInTotal / g_usXorProtectRatio));

    if ((ulBrokenXorPageNumInTotal / XORUT_PLN_PER_LUN) == (ulXorPageNumInTotal / XORUT_PLN_PER_LUN))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void XorUt_DestPageClean(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    // Clean data of destination page.
    U32 *pDestPageData = (U32 *)XorUt_GetDestPageDataAddr(ulXoreId, ulXorPageSize);
    XorUt_PageDataClean(pDestPageData, ulXorPageSize);

    // Clean redundant of destination page.
    U32 *pDestPageRedun = (U32 *)XorUt_GetDestPageRedunAddr(ulXoreId, ulXorPageSize);
    XorUt_RedunClean(pDestPageRedun, ulXorPageSize, 1);

    return;
}

void XorUt_LoadStorePageClean(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    // Clean data of Load-Store page.
    U32 *pLoadStorePageData = (U32 *)XorUt_GetLoadStoreDataAddr(ulXoreId, ulXorPageSize);
    XorUt_PageDataClean(pLoadStorePageData, ulXorPageSize);

    // Clean redundant of Load-Store page.
    U32 *pLoadStorePageRedun = (U32 *)XorUt_GetLoadStoreRedunAddr(ulXoreId, ulXorPageSize);
    XorUt_RedunClean(pLoadStorePageRedun, ulXorPageSize, 1);

    return;
}

void XorUt_ParityDataInOtfbCheck(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 *pParityData = (U32 *)HAL_XoreGetParityPageAddr(ulXoreId);
    U32 *pParityRedun = (U32 *)XorUt_GetParityPageRedunAddr(ulXoreId);
    
    if (FALSE == XorUt_ParityDataCheck(pParityData, ulXorPageSize, ulStartXorPageNumInTotal))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        
        DBG_Printf("XOR_ID = %d\n",ulXoreId);  //For Debug
        DBG_Printf("Parity Addr = 0x%p\n", pParityData);  //For Debug
        
        DBG_Getch();
    }

    // Check XOR result redundant data which be moved to DRAM
    if (FALSE == XorUt_RedunCheck(pParityRedun, ulXorPageSize))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    return;
}

void XorUt_RecoverDataInOtfbCheck(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 *pXorRecoverResultData = (U32 *)HAL_XoreGetParityPageAddr(ulXoreId);
    U32 *pParityRedun = (U32 *)XorUt_GetParityPageRedunAddr(ulXoreId);
    
    if (FALSE == XorUt_PageDataCheck(pXorRecoverResultData, ulXorPageSize, ulBrokenXorPageNumInTotal))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    // Check XOR result redundant data which be moved to DRAM
    if (FALSE == XorUt_RedunCheck(pParityRedun, ulXorPageSize))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    return;
}

void XorUt_ParityDataInDestCheck(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 *pDestData = (U32 *)XorUt_GetDestPageDataAddr(ulXoreId, ulXorPageSize);
    U32 *pDestRedun = (U32 *)XorUt_GetDestPageRedunAddr(ulXoreId, ulXorPageSize);
    
    // Check XOR protect result data which be moved to DRAM
    if (FALSE == XorUt_ParityDataCheck(pDestData, ulXorPageSize, ulStartXorPageNumInTotal))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    // Check XOR result redundant data which be moved to DRAM
    if (FALSE == XorUt_RedunCheck(pDestRedun, ulXorPageSize))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    return;
}

void XorUt_ParityDataInLoadStoreCheck(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    U32 *pLoadStoreData = (U32 *)XorUt_GetLoadStoreDataAddr(ulXoreId, ulXorPageSize);
    U32 *pDestRedun = (U32 *)XorUt_GetDestPageRedunAddr(ulXoreId, ulXorPageSize);

    // Check XOR protect result data which be moved to DRAM
    if (FALSE == XorUt_ParityDataCheck(pLoadStoreData, ulXorPageSize, ulStartXorPageNumInTotal))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    // Check XOR result redundant data which be moved to DRAM
    if (FALSE == XorUt_RedunCheck(pDestRedun, ulXorPageSize))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    return;
}

void XorUt_ParityDataInDestCheckTlc(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    U32 *pDestData = (U32 *)XorUt_GetDestPageDataAddrTlc(ulXoreId, ulXorPageSize);

    // Check XOR protect result data which be moved to DRAM
    if (FALSE == XorUt_ParityDataCheckTlc(pDestData, ulXorPageSize, ulStartXorPageNumInTotal))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    return;
}

void XorUt_RecoverDataInDestCheck(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 *pDestData = (U32 *)XorUt_GetDestPageDataAddr(ulXoreId, ulXorPageSize);
    U32 *pDestRedun = (U32 *)XorUt_GetDestPageRedunAddr(ulXoreId, ulXorPageSize);
    
    // Check XOR result data which be moved to DRAM
    if (FALSE == XorUt_PageDataCheck(pDestData, ulXorPageSize, ulBrokenXorPageNumInTotal))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }

    // Check XOR result redundant data which be moved to DRAM
    if (FALSE == XorUt_RedunCheck(pDestRedun, ulXorPageSize))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }
}

void XorUt_ProtectPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0;

    // Prepare data of user page.
    for (i = 0; i < (U32)(g_usXorProtectRatio - 1); ++i)
    {
        U32 *pUserPageData = (U32 *)XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, i);
        XorUt_PageDataPrepare(pUserPageData, ulXorPageSize, (ulStartXorPageNumInTotal + i));
    }

    // Prepare redundant of user page.
    U32 *pUserPageRedun = (U32 *)XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, 0);
    XorUt_PageRedunPrepare(pUserPageRedun, ulXorPageSize, (g_usXorProtectRatio - 1));

    return;
}

void XorUt_ProtectPrepareDataTlc(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    U32 i = 0;

    // Prepare data of user page.
    for (i = 0; i < (U32)(g_usXorProtectRatio - 1); ++i)
    {
        U32 *pUserPageData = (U32 *)XorUt_GetPageDataAddrTlc(ulXoreId, ulXorPageSize, i);
        XorUt_PageDataPrepareTlc(pUserPageData, ulXorPageSize, (ulStartXorPageNumInTotal + i));
    }

    // Prepare redundant of user page.
    U32 *pUserPageRedun = (U32 *)XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, 0);
    XorUt_PageRedunPrepare(pUserPageRedun, ulXorPageSize, (g_usXorProtectRatio - 1));

    return;
}

void XorUt_NfcProtectCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    XorUt_ParityDataInOtfbCheck(ulXoreId, ulXorPageSize, ulStartXorPageNumInTotal);

    return;
}

void XorUt_BypassProtectCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));

    // Check XOR protect result data in OTFB SRAM
    XorUt_ParityDataInOtfbCheck(ulXoreId, ulXorPageSize, ulStartXorPageNumInTotal);

    // Check XOR protect result data and redundant which be moved to DRAM
    XorUt_ParityDataInDestCheck(ulXoreId, ulXorPageSize, ulStartXorPageNumInTotal);

    return;
}

void XorUt_NfcRecoverCleanData(U32 ulXoreId, U32 ulXorPageSize)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));

    U32 i = 0;

    // Clean data of user page and destination page.
    for (i = 0; i < g_usXorProtectRatio; ++i)
    {
        U32 *pUserPageData = (U32 *)XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, i);
        XorUt_PageDataClean(pUserPageData, ulXorPageSize);
    }

    // Clean redundant of user page and destination page.
    U32 *pUserPageRedun = (U32 *)XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, 0);
    XorUt_RedunClean(pUserPageRedun, ulXorPageSize, g_usXorProtectRatio);

    return;
}

void XorUt_NfcReadCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));

    U32 i = 0, j = 0;
    U32 ulStartXorPageNumInTotal = XorUt_GetStartXorPageNum(ulBrokenXorPageNumInTotal);

    while (i < g_usXorProtectRatio)
    {
        if ((ulStartXorPageNumInTotal + i) == ulBrokenXorPageNumInTotal)
        {
            ++i;
            continue;
        }

        U32 *pPageData = (U32 *)XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, j);
        U32 *pPageRedun = (U32 *)XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, j);

        if (i == (g_usXorProtectRatio - 1))
        {
            // Check XOR parity that be read in DRAM.
            if (FALSE == XorUt_ParityDataCheck(pPageData, ulXorPageSize, ulStartXorPageNumInTotal))
            {
                // XOR result is wrong.
                XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
                DBG_Getch();
            }
        }
        else
        {
            // Check user data that be read in DRAM.
            if (FALSE == XorUt_PageDataCheck(pPageData, ulXorPageSize, (ulStartXorPageNumInTotal + i)))
            {
                // XOR result is wrong.
                XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
                DBG_Getch();
            }
        }


        // Check redundant that be read in DRAM.
        if (FALSE == XorUt_RedunCheck(pPageRedun, ulXorPageSize))
        {
            // XOR result is wrong.
            XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
            DBG_Getch();
        }

        ++i; ++j;
    }

    XorUt_Log(XOR_UT_RECOVER_READ_DATA_OK, NULL);

    return;
}

void XorUt_NfcRecoverCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));
    
    // Check user data and redundant that be used to recover the broken XOR page.
    XorUt_NfcReadCheckData(ulXoreId, ulXorPageSize, ulBrokenXorPageNumInTotal);

    // Check XOR recover result data in OTFB SRAM
    XorUt_RecoverDataInOtfbCheck(ulXoreId, ulXorPageSize, ulBrokenXorPageNumInTotal);

    // Check XOR result data and redundant which be moved to DRAM
    //XorUt_RecoverDataInDestCheck(ulXoreId, ulXorPageSize, ulBrokenXorPageNumInTotal);

    return;
}

void XorUt_BypassRecoverPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));
    
    U32 i = 0, j = 0;
    U32 ulStartXorPageNumInTotal = XorUt_GetStartXorPageNum(ulBrokenXorPageNumInTotal);
    
    while (i < g_usXorProtectRatio)
    {
        if ((ulStartXorPageNumInTotal + i) == ulBrokenXorPageNumInTotal)
        {
            ++i;
            continue;
        }

        U32 *pPageData = (U32 *)XorUt_GetPageDataAddr(ulXoreId, ulXorPageSize, j);
        U32 *pPageRedun = (U32 *)XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, j);

        if (i == (g_usXorProtectRatio - 1))
        {
            XorUt_ParityDataPrepare(pPageData, ulXorPageSize, ulStartXorPageNumInTotal);
        }
        else
        {
            XorUt_PageDataPrepare(pPageData, ulXorPageSize, (ulStartXorPageNumInTotal + i));
        }

        ++i;++j;    
    }
    

    // Prepare redundant of user page and parity page.
    U32 *pUserPageRedun = (U32 *)XorUt_GetPageRedunAddr(ulXoreId, ulXorPageSize, 0);
    XorUt_PageRedunPrepare(pUserPageRedun, ulXorPageSize, (g_usXorProtectRatio - 1));

    return;
}

void XorUt_BypassRecoverCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulBrokenXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));

    // Check XOR recover result data in OTFB SRAM
    XorUt_RecoverDataInOtfbCheck(ulXoreId, ulXorPageSize, ulBrokenXorPageNumInTotal);

    // Check XOR recover result data and redundant which be moved to DRAM
    XorUt_RecoverDataInDestCheck(ulXoreId, ulXorPageSize, ulBrokenXorPageNumInTotal);

    return;
}

void XorUt_LoadFromDramPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));
    
    U32 *pLoadData = (U32 *)XorUt_GetLoadStoreDataAddr(ulXoreId, ulXorPageSize);
    U32 *pLoadRedun = (U32 *)XorUt_GetLoadStoreRedunAddr(ulXoreId, ulXorPageSize);

    if ((ulXorPageNumInTotal % g_usXorProtectRatio) == (g_usXorProtectRatio - 1))
    {
        XorUt_ParityDataPrepare(pLoadData, ulXorPageSize, XorUt_GetStartXorPageNum(ulXorPageNumInTotal));
    }
    else
    {
        XorUt_PageDataPrepare(pLoadData, ulXorPageSize, ulXorPageNumInTotal);
    }
    
    XorUt_PageRedunPrepare(pLoadRedun, ulXorPageSize, 1);

    return;
}

void XorUt_LoadFromDramCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));

    if ((ulXorPageNumInTotal % g_usXorProtectRatio) == (g_usXorProtectRatio - 1))
    {
        XorUt_ParityDataInOtfbCheck(ulXoreId, ulXorPageSize, XorUt_GetStartXorPageNum(ulXorPageNumInTotal));
    }
    else
    {
        XorUt_RecoverDataInOtfbCheck(ulXoreId, ulXorPageSize, ulXorPageNumInTotal);
    }

    return;
}

void XorUt_StoreToDramPrepareData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));

    U32 *pParityData = (U32 *)HAL_XoreGetParityPageAddr(ulXoreId);
    U32 *pStoreRedun = (U32 *)XorUt_GetParityPageRedunAddr(ulXoreId);
    
    if ((ulXorPageNumInTotal % g_usXorProtectRatio) == (g_usXorProtectRatio - 1))
    {
        U32 ulStartXorPageNumInTotal = ulXorPageNumInTotal - (g_usXorProtectRatio - 1);
        XorUt_ParityDataPrepare(pParityData, ulXorPageSize, ulStartXorPageNumInTotal);
    }
    else
    {
        XorUt_PageDataPrepare(pParityData, ulXorPageSize, ulXorPageNumInTotal);
    }

    XorUt_PageRedunPrepare(pStoreRedun, ulXorPageSize, 1);

    return;
}

void XorUt_StoreToDramCheckData(U32 ulXoreId, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    XORUT_ASSERT(ulXoreId < XORE_CNT);
    XORUT_ASSERT(ulXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT(ulXorPageSize == (16 * KB_SIZE));

    U32 *pStoreData = (U32 *)XorUt_GetLoadStoreDataAddr(ulXoreId, ulXorPageSize);
    U32 *pStoreRedun = (U32 *)XorUt_GetLoadStoreRedunAddr(ulXoreId, ulXorPageSize);
    
    if ((ulXorPageNumInTotal % g_usXorProtectRatio) == (g_usXorProtectRatio - 1))
    {
        U32 ulStartXorPageNumInTotal = ulXorPageNumInTotal - (g_usXorProtectRatio - 1);

        if (FALSE == XorUt_ParityDataCheck(pStoreData, ulXorPageSize, ulStartXorPageNumInTotal))
        {
            // XOR result is wrong.
            XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
            DBG_Getch();
        }
    }
    else
    {
        if (FALSE == XorUt_PageDataCheck(pStoreData, ulXorPageSize, ulXorPageNumInTotal))
        {
            // XOR result is wrong.
            XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
            DBG_Getch();
        }
    }
/*
    if (FALSE == XorUt_RedunCheck(pStoreRedun, ulXorPageSize))
    {
        // XOR result is wrong.
        XorUt_Log(XOR_UT_DATA_CHECK_ERROR, NULL);
        DBG_Getch();
    }
*/
    return;
}

BOOL XorUt_IsNeedEraseBlock(const XOR_NFC_PROTECT_CTRL *ptNfcProtectCtrl)
{
    if (ptNfcProtectCtrl->tCurrentFlashAddr.usPage == 0)
    {
        if ((ptNfcProtectCtrl->tCurrentFlashAddr.ucPU != ptNfcProtectCtrl->tPreEraseBlkFlashAddr.ucPU) ||
            (ptNfcProtectCtrl->tCurrentFlashAddr.ucLun != ptNfcProtectCtrl->tPreEraseBlkFlashAddr.ucLun) ||
            (ptNfcProtectCtrl->tCurrentFlashAddr.usBlock != ptNfcProtectCtrl->tPreEraseBlkFlashAddr.usBlock))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    } 
    else
    {
        return FALSE;
    }
}

BOOL XorUt_IsNeedEraseBlockTlc(const XOR_TLC_NFC_PROTECT_CTRL *pTlcNfcProtectCtrl)
{
    if (pTlcNfcProtectCtrl->ucCurTlcProgramIndex == 0)
    {
        if ((pTlcNfcProtectCtrl->tCurrentFlashAddr.ucPU != pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr.ucPU) ||
            (pTlcNfcProtectCtrl->tCurrentFlashAddr.ucLun != pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr.ucLun) ||
            (pTlcNfcProtectCtrl->tCurrentFlashAddr.usBlock != pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr.usBlock))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

BOOL XorUt_IsParityNfcCmd1stStageDone(U32 ulStartXorPageNumInTotal)
{
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);

    volatile U8 *pXorPagesSsu = NULL;

    if (g_bIsSinglePlaneOperate == TRUE)
    {
        pXorPagesSsu = (volatile U8 *)(XorUt_GetSsuDramBaseAddr() + ulStartXorPageNumInTotal + g_usXorProtectRatio - 1);

        if (*pXorPagesSsu == 1)
        {
            //COM_MemZero((U32 *)pXorPagesSsu, 1);
            *pXorPagesSsu = NULL;
            return TRUE;
        }
        else if (*pXorPagesSsu == 0)
        {
            return FALSE;
        }
        else
        {
            DBG_Printf("SSU value is not right!\n");
            XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
            DBG_Getch();
            return FALSE;
        }
    }
    else
    {
        pXorPagesSsu = (U8 *)(XorUt_GetSsuDramBaseAddr() + ulStartXorPageNumInTotal + g_usXorProtectRatio - XORUT_PLN_PER_LUN);

        if (*pXorPagesSsu == XORUT_PLN_PER_LUN)
        {
            //COM_MemZero((U32 *)pXorPagesSsu, 1);
            *pXorPagesSsu = NULL;
            return TRUE;
        }
        else if (*pXorPagesSsu == 0)
        {
            return FALSE;
    } 
    else
    {
            DBG_Printf("SSU value is not right!\n");
            XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
            DBG_Getch();
        return FALSE;
    }
}
}

BOOL XorUt_IsAllXorPageNfcCmdDone(U32 ulStartXorPageNumInTotal, XOR_TARGET eTarget)
{
    XORUT_ASSERT(ulStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((ulStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT((eTarget >= 0) && (eTarget < XOR_TARGET_ASSERT));

    U8 *pXorPagesSsu = (U8 *)(XorUt_GetSsuDramBaseAddr() + ulStartXorPageNumInTotal);
    U32 ulXorPageNfcCmdDoneCnt = 0;
    U32 i = 0;

    for (i = 0; i < g_usXorProtectRatio; ++i)
    {
        ulXorPageNfcCmdDoneCnt += pXorPagesSsu[i];
    }

    U32 ulXorPageNfcCmdCnt = (eTarget == XOR_PROTECT) ? g_usXorProtectRatio : (g_usXorProtectRatio - 1);

    if (ulXorPageNfcCmdDoneCnt == ulXorPageNfcCmdCnt)
    {
        COM_MemZero((U32 *)pXorPagesSsu, (g_usXorProtectRatio >> 2));
        return TRUE;
    }
    else if (ulXorPageNfcCmdDoneCnt < ulXorPageNfcCmdCnt)
    {
        return FALSE;
    }
    else
    {
        DBG_Printf("SSU value is not right!\n");
        XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
        DBG_Getch();
        return FALSE;
    }
}

void XorUt_ClearNfcProtectCtrl(XOR_NFC_PROTECT_CTRL *pNfcProtectCtrl)
{
    XORUT_ASSERT(pNfcProtectCtrl != NULL);

    pNfcProtectCtrl->ucStatus = XOR_UT_INIT;
    pNfcProtectCtrl->ucXoreId = XORUT_INVALID_XORE_ID;
    pNfcProtectCtrl->ucCurXorPageNumInXore = 0;
    pNfcProtectCtrl->ulXorPageSize = g_ulXorPageSize;
    //pNfcProtectCtrl->pXorPageLocationTable = NULL;  //Regina modify
    
    return;
}

void XorUt_ClearTlcNfcProtectCtrl(XOR_TLC_NFC_PROTECT_CTRL *pTlcNfcProtectCtrl)
{
    XORUT_ASSERT(pTlcNfcProtectCtrl != NULL);

    pTlcNfcProtectCtrl->ucStatus = XOR_UT_INIT;
    pTlcNfcProtectCtrl->ucXoreId = XORUT_INVALID_XORE_ID;
    pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow = 0;
    pTlcNfcProtectCtrl->ucCurTlcProgramIndex = 0;
    pTlcNfcProtectCtrl->ucXorParityPartNum = 0;
    pTlcNfcProtectCtrl->ulXorPageSize = 48 * KB_SIZE;
    pTlcNfcProtectCtrl->pXorPageLocationTable = NULL;

    return;
}

void XorUt_ClearNfcRecoverCtrl(XOR_NFC_RECOVER_CTRL *pNfcRecoverCtrl)
{
    XORUT_ASSERT(pNfcRecoverCtrl != NULL);

    pNfcRecoverCtrl->ucStatus = XOR_UT_INIT;
    pNfcRecoverCtrl->ucXoreId = XORUT_INVALID_XORE_ID;
    pNfcRecoverCtrl->ucCurXorPageNumInXore = 0;
    pNfcRecoverCtrl->ucStoreXorPageNumInXore = 0;
    pNfcRecoverCtrl->usStartXorPageNumInTotal = 0;
    pNfcRecoverCtrl->ulXorPageSize = g_ulXorPageSize;
    pNfcRecoverCtrl->pXorPageLocationTable = NULL;

    return;
}

void XorUt_ClearBypassNfcCtrl(XOR_BYPASS_NFC_CTRL *pBypassNfcCtrl)
{
    XORUT_ASSERT(pBypassNfcCtrl != NULL);

    pBypassNfcCtrl->ucStatus = XOR_UT_INIT;
    pBypassNfcCtrl->ucXoreId = XORUT_INVALID_XORE_ID;
    pBypassNfcCtrl->ucCurXorPageNumInXore = 0;
    pBypassNfcCtrl->ucStartXorPageNumOfLastTrigger = 0;
    pBypassNfcCtrl->ucXorPageCntOfLastTrigger = 0;
    pBypassNfcCtrl->ulXorPageSize = g_ulXorPageSize;

    return;
}

void XorUt_ClearBypassRecoverWithNfcReadCtrl(XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL *pBypassRecoverWithNfcReadCtrl)
{
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl != NULL);

    pBypassRecoverWithNfcReadCtrl->ucStatus = XOR_UT_INIT;
    pBypassRecoverWithNfcReadCtrl->ucXoreId = XORUT_INVALID_XORE_ID;
    pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore = 0;
    pBypassRecoverWithNfcReadCtrl->usStartXorPageNumInTotal = 0;
    pBypassRecoverWithNfcReadCtrl->ucStoreXorPageNumInXore = 0;
    pBypassRecoverWithNfcReadCtrl->ucStartXorPageNumOfLastTrigger = 0;
    pBypassRecoverWithNfcReadCtrl->ucXorPageCntOfLastTrigger = 0;
    pBypassRecoverWithNfcReadCtrl->ulXorPageSize = g_ulXorPageSize;
    pBypassRecoverWithNfcReadCtrl->bIsTlcRecover = FALSE;
    pBypassRecoverWithNfcReadCtrl->ucPageType = 0;

    return;
}

void XorUt_ClearLoadStoreCtrl(XOR_LOAD_STORE_CTRL *pLoadStoreCtrl)
{
    XORUT_ASSERT(pLoadStoreCtrl != NULL);

    pLoadStoreCtrl->ucStatus = XOR_UT_INIT;
    pLoadStoreCtrl->ucXoreId = XORUT_INVALID_XORE_ID;
    pLoadStoreCtrl->ulXorPageSize = g_ulXorPageSize;

    return;
}

BOOL XorUt_NfcProtect(XOR_NFC_PROTECT_CTRL *pNfcProtectCtrl)
{
    XORUT_ASSERT(pNfcProtectCtrl != NULL);
    XORUT_ASSERT(pNfcProtectCtrl->usStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((pNfcProtectCtrl->usStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT(pNfcProtectCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    XORUT_ASSERT(pNfcProtectCtrl->ucCurXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT((g_bIsSinglePlaneOperate == TRUE) || (g_bIsSinglePlaneOperate == FALSE));
    
    BOOL bXorNfcProtectFinish = FALSE;

    switch (pNfcProtectCtrl->ucStatus)
    {
        case XOR_UT_INIT:
        {
            XORUT_ASSERT(pNfcProtectCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

            U32 ulXoreId = 0;

            if (TRUE == HAL_GetXore(pNfcProtectCtrl->ulXorPageSize, &ulXoreId))
            {
                pNfcProtectCtrl->ucXoreId = ulXoreId;

                XorUt_ProtectPrepareData(ulXoreId, pNfcProtectCtrl->ulXorPageSize, 
                    pNfcProtectCtrl->usStartXorPageNumInTotal);

                U32 ulPreRdyPageCnt = (g_bIsSinglePlaneOperate == TRUE) ? 0 : (XORUT_PLN_PER_LUN - 1);

                HAL_XoreNfcModeConfig(ulXoreId, XOR_PROTECT, g_usXorProtectRatio, 
                    ulPreRdyPageCnt, FALSE);
                HAL_XoreTrigger(ulXoreId);

                if (XorUt_IsXoreValid(ulXoreId) == TRUE)
                {
                    DBG_Printf("This XOR engine should non-valid at here!\n");
                    XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
                    DBG_Getch();
                }

                pNfcProtectCtrl->tPreEraseBlkFlashAddr.ucPU = 0xFF;
                pNfcProtectCtrl->tPreEraseBlkFlashAddr.ucLun = 0xFF;
                pNfcProtectCtrl->tPreEraseBlkFlashAddr.usBlock = 0xFFFF;

                pNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
            }

            break;
        }
        case XOR_UT_IS_NEED_ERASE_BLOCK:
        {
            XORUT_ASSERT(pNfcProtectCtrl->ucXoreId < XORE_CNT);

            pNfcProtectCtrl->tCurrentFlashAddr = XorUt_XorPageLocate(pNfcProtectCtrl->pXorPageLocationTable,
                pNfcProtectCtrl->ucCurXorPageNumInXore);

            if (TRUE == XorUt_IsNeedEraseBlock(pNfcProtectCtrl))
            {
                pNfcProtectCtrl->ucStatus = XOR_UT_ERASE_BLOCK;
            }
            else
            {
                pNfcProtectCtrl->ucStatus = XOR_UT_PROGRAM_PAGE;
            }

            break;
        }
        case XOR_UT_ERASE_BLOCK:
        {
            XORUT_ASSERT(pNfcProtectCtrl->ucXoreId < XORE_CNT);

            FLASH_ADDR *pCurFlashAddr = &(pNfcProtectCtrl->tCurrentFlashAddr);

            if (TRUE == HAL_NfcGetErrHold(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                DBG_Printf("PU %d XOR NFC protect fail, Full Block Erase Error!\n", pCurFlashAddr->ucPU);
                XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
                DBG_Getch();
            }

            if (FALSE == HAL_NfcGetFull(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                DBG_Printf("Erase blk %d page %d ucPu %d\n", pCurFlashAddr->usBlock, pCurFlashAddr->usPage, pCurFlashAddr->ucPU);
                HAL_NfcFullBlockErase(pCurFlashAddr, FALSE);

                //Regina added
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
                {
                    DBG_Printf("PU%d LUN %d BLK%d Erase Fail!\n", pCurFlashAddr->ucPU, pCurFlashAddr->ucLun, pCurFlashAddr->usBlock);
                }
                
                COM_MemCpy((U32 *)&(pNfcProtectCtrl->tPreEraseBlkFlashAddr), (U32 *)pCurFlashAddr,
                    (sizeof(FLASH_ADDR) / sizeof(U32)));

                pNfcProtectCtrl->ucStatus = XOR_UT_PROGRAM_PAGE;
            }

            break;
        }
        case XOR_UT_PROGRAM_PAGE:
        {
            XORUT_ASSERT(pNfcProtectCtrl->ucXoreId < XORE_CNT);

            FLASH_ADDR *pCurFlashAddr = &(pNfcProtectCtrl->tCurrentFlashAddr);

            if (TRUE == HAL_NfcGetErrHold(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                DBG_Printf("PU %d XOR NFC protect fail, program page error!\n", pCurFlashAddr->ucPU);
                XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
                DBG_Getch();
            }

            if (FALSE == HAL_NfcGetFull(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                U32 ulXoreId = pNfcProtectCtrl->ucXoreId;
                U8 *pCurXorPageNumInXore = &(pNfcProtectCtrl->ucCurXorPageNumInXore);
                U32 ulXorPageNumInTotal = pNfcProtectCtrl->usStartXorPageNumInTotal + *pCurXorPageNumInXore;

                U32 ulPageDataAddr = XorUt_GetPageDataAddr(ulXoreId, pNfcProtectCtrl->ulXorPageSize,
                    *pCurXorPageNumInXore);
                U32 ulPageRedunAddr = XorUt_GetPageRedunAddr(ulXoreId, pNfcProtectCtrl->ulXorPageSize,
                    *pCurXorPageNumInXore);
                BOOL bIsLastNfcInXor = FALSE;

                if (g_bIsSinglePlaneOperate == TRUE)
                {
                    bIsLastNfcInXor = (*pCurXorPageNumInXore == (g_usXorProtectRatio - 1)) ? TRUE : FALSE;
                    XorUt_NfcSinglePlnWrite(pCurFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                        ulXorPageNumInTotal, ulXoreId, bIsLastNfcInXor);
                    ++(*pCurXorPageNumInXore);
                }
                else
                {
                    bIsLastNfcInXor = (*pCurXorPageNumInXore == (g_usXorProtectRatio - XORUT_PLN_PER_LUN)) ? TRUE : FALSE;
                    XorUt_NfcFullPlnWrite(pCurFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                        ulXorPageNumInTotal, ulXoreId, bIsLastNfcInXor);
                    (*pCurXorPageNumInXore) += XORUT_PLN_PER_LUN;
                }

                if ((*pCurXorPageNumInXore) == g_usXorProtectRatio)
                {
                    pNfcProtectCtrl->ucStatus = XOR_UT_WAIT_DONE;
                } 
                else
                {
                    pNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
                }
            }

            break;
        }
        case XOR_UT_WAIT_DONE:
        {
            XORUT_ASSERT(pNfcProtectCtrl->ucXoreId < XORE_CNT);

            if (TRUE == XorUt_IsAllXorPageNfcCmdDone(pNfcProtectCtrl->usStartXorPageNumInTotal, XOR_PROTECT))
            {
                // NFC should release this XOR engine.
                if (XorUt_IsXoreValid(pNfcProtectCtrl->ucXoreId) == FALSE)
                {
                    DBG_Printf("This XOR engine should valid at here!\n");
                    DBG_Printf("XORE ID = %d\n",pNfcProtectCtrl->ucXoreId);
                    XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
                    DBG_Getch();
                }
                bXorNfcProtectFinish = TRUE;
            }

            break;
        }
        default:
        {
            XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
            DBG_Getch();
        }
    }

    return bXorNfcProtectFinish;
}

#ifdef FLASH_TLC
BOOL XorUt_NfcProtectTlc(XOR_TLC_NFC_PROTECT_CTRL *pTlcNfcProtectCtrl)
{
    XORUT_ASSERT(pTlcNfcProtectCtrl != NULL);
    XORUT_ASSERT(pTlcNfcProtectCtrl->usStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((pTlcNfcProtectCtrl->usStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT(pTlcNfcProtectCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    U32 ulEndPlnOfRow = (g_usXorProtectRatio < PLN_CNT_PER_ROW) ? g_usXorProtectRatio : PLN_CNT_PER_ROW;
    XORUT_ASSERT(pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow < ulEndPlnOfRow);
    XORUT_ASSERT((g_bIsSinglePlaneOperate == TRUE) || (g_bIsSinglePlaneOperate == FALSE));

    BOOL bXorTlcNfcProtectFinish = FALSE;

    switch (pTlcNfcProtectCtrl->ucStatus)
    {
        case XOR_UT_INIT:
        {
            XORUT_ASSERT(pTlcNfcProtectCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

            U32 ulXoreId = 0;

            if (TRUE == HAL_GetXore(pTlcNfcProtectCtrl->ulXorPageSize, &ulXoreId))
            {
                pTlcNfcProtectCtrl->ucXoreId = ulXoreId;

                XorUt_ProtectPrepareDataTlc(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                    pTlcNfcProtectCtrl->usStartXorPageNumInTotal);

                U32 ulPreRdyPageCnt = (g_bIsSinglePlaneOperate == TRUE) ? 0 : (XORUT_PLN_PER_LUN - 1);

                HAL_XoreNfcModeConfig(ulXoreId, XOR_PROTECT, g_usXorProtectRatio,
                    ulPreRdyPageCnt, FALSE);
                HAL_XoreTrigger(ulXoreId);

                if (XorUt_IsXoreValid(ulXoreId) == TRUE)
                {
                    DBG_Printf("This XOR engine should non-valid at here!\n");
                    XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
                    DBG_Getch();
                }

                pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr.ucPU = 0xFF;
                pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr.ucLun = 0xFF;
                pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr.usBlock = 0xFFFF;

                pTlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
            }

            break;
        }
        case XOR_UT_IS_NEED_ERASE_BLOCK:
        {
            XORUT_ASSERT(pTlcNfcProtectCtrl->ucXoreId < XORE_CNT);

            U32 ulWordLineIndex = HAL_FlashGetTlcPrgWL(pTlcNfcProtectCtrl->ucCurTlcProgramIndex);
            pTlcNfcProtectCtrl->tCurrentFlashAddr = 
                XorUt_GetTlcXorPageLocation(pTlcNfcProtectCtrl->pXorPageLocationTable, ulWordLineIndex,
                pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow);

            if (TRUE == XorUt_IsNeedEraseBlockTlc(pTlcNfcProtectCtrl))
            {
                pTlcNfcProtectCtrl->ucStatus = XOR_UT_ERASE_BLOCK;
            }
            else
            {
                pTlcNfcProtectCtrl->ucStatus = XOR_UT_PROGRAM_PAGE;
            }

            break;
        }
        case XOR_UT_ERASE_BLOCK:
        {
            XORUT_ASSERT(pTlcNfcProtectCtrl->ucXoreId < XORE_CNT);

            FLASH_ADDR *pCurFlashAddr = &(pTlcNfcProtectCtrl->tCurrentFlashAddr);

            if (TRUE == HAL_NfcGetErrHold(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                DBG_Printf("PU %d XOR TLC NFC protect fail, Full Block Erase Error!\n", pCurFlashAddr->ucPU);
                XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
                DBG_Getch();
            }

            if (FALSE == HAL_NfcGetFull(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                DBG_Printf("Erase blk %d ucPu %d\n", pCurFlashAddr->usBlock, pCurFlashAddr->ucPU);
                HAL_NfcFullBlockErase(pCurFlashAddr, TRUE);

                //Regina added
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
                {
                    DBG_Printf("PU%d LUN %d BLK%d Erase Fail!\n", pCurFlashAddr->ucPU, pCurFlashAddr->ucLun, pCurFlashAddr->usBlock);
                }
                
                COM_MemCpy((U32 *)&(pTlcNfcProtectCtrl->tPreEraseBlkFlashAddr), (U32 *)pCurFlashAddr,
                    (sizeof(FLASH_ADDR) / sizeof(U32)));

                pTlcNfcProtectCtrl->ucStatus = XOR_UT_PROGRAM_PAGE;
            }

            break;
        }
        case XOR_UT_PROGRAM_PAGE:
        {
            XORUT_ASSERT(pTlcNfcProtectCtrl->ucXoreId < XORE_CNT);

            FLASH_ADDR *pCurFlashAddr = &(pTlcNfcProtectCtrl->tCurrentFlashAddr);

            if (TRUE == HAL_NfcGetErrHold(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                DBG_Printf("PU %d XOR NFC protect fail, program page error!\n", pCurFlashAddr->ucPU);
                XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
                DBG_Getch();
            }

            if (FALSE == HAL_NfcGetFull(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
            {
                U32 ulXoreId = pTlcNfcProtectCtrl->ucXoreId;
                U32 ulPageDataAddr = 0;
                U32 ulPageRedunAddr = 0;

                U32 ulTlcProgramCycle = HAL_FlashGetTlcPrgCycle(pTlcNfcProtectCtrl->ucCurTlcProgramIndex);

                U32 ulEndPlnOfRow = (g_usXorProtectRatio < PLN_CNT_PER_ROW) ? g_usXorProtectRatio : PLN_CNT_PER_ROW;
                U32 ulCurXorPageNumInXore = ulEndPlnOfRow * pCurFlashAddr->usPage + pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow;
                U32 ulXorPageNumInTotal = pTlcNfcProtectCtrl->usStartXorPageNumInTotal + ulCurXorPageNumInXore;

                if (ulCurXorPageNumInXore < g_usXorProtectRatio)
                {
                    ulPageDataAddr = XorUt_GetPageDataAddrTlc(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                        ulCurXorPageNumInXore);
                    ulPageRedunAddr = XorUt_GetPageRedunAddr(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                        ulCurXorPageNumInXore);
                } 
                else
                {
                    ulPageDataAddr = XorUt_GetPageDataAddrTlc(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                        (g_usXorProtectRatio - 1));
                    ulPageRedunAddr = XorUt_GetPageRedunAddr(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                        (g_usXorProtectRatio - 1));
                }


                BOOL bIsLastNfcInXor = FALSE;

                if (g_bIsSinglePlaneOperate == TRUE)
                {
                    bIsLastNfcInXor = (ulCurXorPageNumInXore == (g_usXorProtectRatio - 1)) ? TRUE : FALSE;

                    //DBG_Printf("Write blk %d page %d ucPU %d, Program Cyle %d\n", pCurFlashAddr->usBlock, pCurFlashAddr->usPage, pCurFlashAddr->ucPU, ulTlcProgramCycle);
                    //DBG_Printf("CurXorPageNum = %d, CurTlcProgramIndex = %d\n", ulCurXorPageNumInXore, pTlcNfcProtectCtrl->ucCurTlcProgramIndex);
                    //DBG_Printf("Dram Addr = 0x%x\n", ulPageDataAddr);

                    XorUt_NfcSinglePlnTlcWrite(pCurFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                        ulTlcProgramCycle, ulCurXorPageNumInXore, ulXorPageNumInTotal, ulXoreId, bIsLastNfcInXor);
                    ++(pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow);

                    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
                    {
                        DBG_Printf("Pu %d Block %d Page %d Write Fail!\n",pCurFlashAddr->ucPU,pCurFlashAddr->usBlock,pCurFlashAddr->usPage);
                        //NFC_GETCH;
                    }

                    if ((bIsLastNfcInXor == TRUE) && (ulTlcProgramCycle == 0))
                    {
                        pTlcNfcProtectCtrl->ucStatus = XOR_UT_TLC_STORE;
                    }
                    else
                    {
                        pTlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
                    }
                }
                else
                {
                    bIsLastNfcInXor = (ulCurXorPageNumInXore == (g_usXorProtectRatio - XORUT_PLN_PER_LUN)) ? TRUE : FALSE;
                    
                    if ((bIsLastNfcInXor == TRUE) && (ulTlcProgramCycle == 0))
                    {
                        U32 ulXorParityPartNum = pTlcNfcProtectCtrl->ucXorParityPartNum;

                        ulPageDataAddr += ulXorParityPartNum * LOGIC_PIPE_PG_SZ;
                        ulPageRedunAddr += ulXorParityPartNum * (XORUT_PLN_PER_LUN - 1) * g_usRedunSizePerSinglePage;

                        //DBG_Printf("Write blk %d page %d ucPU %d, Program Cyle %d\n", pCurFlashAddr->usBlock, pCurFlashAddr->usPage, pCurFlashAddr->ucPU, ulTlcProgramCycle);
                                    //DBG_Printf("CurXorPageNum = %d, CurTlcProgramIndex = %d\n", ulCurXorPageNumInXore, pTlcNfcProtectCtrl->ucCurTlcProgramIndex);
                                    //DBG_Printf("Dram Addr = 0x%x\n", ulPageDataAddr);


                        XorUt_NfcFullPlnTlcWrite(pCurFlashAddr, ulPageDataAddr, ulPageRedunAddr, ulTlcProgramCycle,
                             ulCurXorPageNumInXore, ulXorPageNumInTotal, ulXoreId, bIsLastNfcInXor, ulXorParityPartNum);

                        ++(pTlcNfcProtectCtrl->ucXorParityPartNum);
                        if (pTlcNfcProtectCtrl->ucXorParityPartNum == 3)
                        {
                            pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow += XORUT_PLN_PER_LUN;
                            pTlcNfcProtectCtrl->ucStatus = XOR_UT_TLC_STORE;
                        }
                        else
                        {
                            pTlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
                        }
                    }
                    else
                    {
                        XorUt_NfcFullPlnTlcWrite(pCurFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                            ulTlcProgramCycle, ulCurXorPageNumInXore, ulXorPageNumInTotal, ulXoreId, bIsLastNfcInXor, 0);

                        if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
                        {
                          DBG_Printf("Pu %d Block %d Page %d Write Fail!\n",pCurFlashAddr->ucPU,pCurFlashAddr->usBlock,pCurFlashAddr->usPage);
                          //NFC_GETCH;
                        }

                        pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow += XORUT_PLN_PER_LUN;
                        pTlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
                    }
                }

                if (pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow == ulEndPlnOfRow)
                {
                    ++(pTlcNfcProtectCtrl->ucCurTlcProgramIndex);

                    U32 ulEndWordLineIndex = (g_usXorProtectRatio / ulEndPlnOfRow);
                    
                    if ((pCurFlashAddr->usPage == ulEndWordLineIndex) && (ulTlcProgramCycle == 2))
                    {
                        pTlcNfcProtectCtrl->ucStatus = XOR_UT_WAIT_DONE;
                    }
                    
                    pTlcNfcProtectCtrl->ucCurProgramPlaneNumInRow = 0;
                }
            }

            break;
        }
        case XOR_UT_TLC_STORE:
        {
            U32 ulXoreId = pTlcNfcProtectCtrl->ucXoreId;

            XORUT_ASSERT(ulXoreId < XORE_CNT);

            if (TRUE == XorUt_IsParityNfcCmd1stStageDone(pTlcNfcProtectCtrl->usStartXorPageNumInTotal))
            {
                XorUt_ParityDataInOtfbCheck(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                    pTlcNfcProtectCtrl->usStartXorPageNumInTotal);

                U32 i = 0;
                XOR_SATUS eResult = XOR_IDLE;
                U32 ulParityPageDataAddr = XorUt_GetDestPageDataAddrTlc(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize);
                U32 ulParityPageRedunAddr = XorUt_GetDestPageRedunAddr(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize);
                U32 ulParityPageDataOffset = HAL_XoreGetParityPageOffset(ulXoreId);
                U32 ulParityPageRedunOffset = HAL_XoreGetParityRedunOffset(ulXoreId);

                U32 ulOldParityPageOffset = ulParityPageDataOffset;
                U32 ulOldParityRedunOffset = ulParityPageRedunOffset;

                HAL_XoreSetXorPageSize(ulXoreId, g_ulSinglePageSize);
                HAL_XorePartialOperate(ulXoreId, 0, g_ulSinglePageSize / CW_INFO_SZ);

                for (i = 0; i < 3; ++i)
                {
                    HAL_XoreSetParityPageOffset(ulXoreId, ulParityPageDataOffset);
                    HAL_XoreSetParityRedunOffset(ulXoreId, ulParityPageRedunOffset);

                    HAL_XoreLoadStoreModeConfig(ulXoreId, XOR_STORE_TO_DRAM, ulParityPageDataAddr,
                        ulParityPageRedunAddr, g_usRedunSizePerSinglePage, FALSE);

                    HAL_XoreTrigger(ulXoreId);

                    do
                    {
                        eResult = HAL_XoreGetStatus(ulXoreId);
                    } while (eResult != XOR_FINISH);

                    ulParityPageDataOffset += g_ulSinglePageSize;
                    ulParityPageRedunOffset += g_usRedunSizePerSinglePage;

                    if (g_bIsSinglePlaneOperate == TRUE)
                    {
                        ulParityPageDataAddr += g_ulSinglePageSize;
                        ulParityPageRedunAddr += g_usRedunSizePerSinglePage;
                    }
                    else
                    {
                        ulParityPageDataAddr += XORUT_PLN_PER_LUN * g_ulSinglePageSize;
                        ulParityPageRedunAddr += g_usRedunSizePerSinglePage;
                    }
                }

                HAL_XoreSetXorPageSize(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize);
                HAL_XorePartialOperate(ulXoreId, 0, (pTlcNfcProtectCtrl->ulXorPageSize / CW_INFO_SZ));
                HAL_XoreSetParityPageOffset(ulXoreId, ulOldParityPageOffset);
                HAL_XoreSetParityRedunOffset(ulXoreId, ulOldParityRedunOffset);

                XorUt_ParityDataInDestCheckTlc(ulXoreId, pTlcNfcProtectCtrl->ulXorPageSize,
                    pTlcNfcProtectCtrl->usStartXorPageNumInTotal);
                pTlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
            }

            break;
        }
        case XOR_UT_WAIT_DONE:
        {
            XORUT_ASSERT(pTlcNfcProtectCtrl->ucXoreId < XORE_CNT);

            if (TRUE == XorUt_IsAllXorPageNfcCmdDone(pTlcNfcProtectCtrl->usStartXorPageNumInTotal, XOR_PROTECT))
            {
                // NFC should release this XOR engine.
                if (XorUt_IsXoreValid(pTlcNfcProtectCtrl->ucXoreId) == FALSE)
                {
                    DBG_Printf("This XOR engine should valid at here!\n");
                    XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
                    DBG_Getch();
                }

                bXorTlcNfcProtectFinish = TRUE;
            }

            break;
        }
        default:
        {
            XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
            DBG_Getch();
        }
    }

    return bXorTlcNfcProtectFinish;
}
#endif

#ifdef FLASH_3D_MLC
BOOL XorUt_NfcProtect3dMlc(XOR_NFC_PROTECT_CTRL *p3dMlcNfcProtectCtrl)
{
    XORUT_ASSERT(p3dMlcNfcProtectCtrl != NULL);
    XORUT_ASSERT(p3dMlcNfcProtectCtrl->usStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((p3dMlcNfcProtectCtrl->usStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucCurXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT(g_bIsSinglePlaneOperate == FALSE);

    BOOL b3dMlcXorNfcProtectFinish = FALSE;

    switch (p3dMlcNfcProtectCtrl->ucStatus)
    {
    case XOR_UT_INIT:
    {
        XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

        U32 ulXoreId = 0;

        if (TRUE == HAL_GetXore(p3dMlcNfcProtectCtrl->ulXorPageSize, &ulXoreId))
        {
            p3dMlcNfcProtectCtrl->ucXoreId = ulXoreId;

            XorUt_ProtectPrepareDataTlc(ulXoreId, p3dMlcNfcProtectCtrl->ulXorPageSize,
                p3dMlcNfcProtectCtrl->usStartXorPageNumInTotal);

            U32 ulPreRdyPageCnt = (XORUT_PLN_PER_LUN - 1);

            HAL_XoreNfcModeConfig(ulXoreId, XOR_PROTECT, g_usXorProtectRatio, ulPreRdyPageCnt, FALSE);
            HAL_XoreTrigger(ulXoreId);

            if (XorUt_IsXoreValid(ulXoreId) == TRUE)
            {
                DBG_Printf("This XOR engine should non-valid at here!\n");
                XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
                DBG_Getch();
            }

            p3dMlcNfcProtectCtrl->tPreEraseBlkFlashAddr.ucPU = 0xFF;
            p3dMlcNfcProtectCtrl->tPreEraseBlkFlashAddr.ucLun = 0xFF;
            p3dMlcNfcProtectCtrl->tPreEraseBlkFlashAddr.usBlock = 0xFFFF;

            p3dMlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
        }

        break;
    }
    case XOR_UT_IS_NEED_ERASE_BLOCK:
    {
        XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucXoreId < XORE_CNT);

        p3dMlcNfcProtectCtrl->tCurrentFlashAddr = XorUt_Get3dMlcXorPageLocate(p3dMlcNfcProtectCtrl->pXorPageLocationTable,
            p3dMlcNfcProtectCtrl->ucCurXorPageNumInXore);

        if (TRUE == XorUt_IsNeedEraseBlock(p3dMlcNfcProtectCtrl))
        {
            p3dMlcNfcProtectCtrl->ucStatus = XOR_UT_ERASE_BLOCK;
        }
        else
        {
            p3dMlcNfcProtectCtrl->ucStatus = XOR_UT_PROGRAM_PAGE;
        }

        break;
    }
    case XOR_UT_ERASE_BLOCK:
    {
        XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucXoreId < XORE_CNT);

        FLASH_ADDR *pCurFlashAddr = &(p3dMlcNfcProtectCtrl->tCurrentFlashAddr);

        if (TRUE == HAL_NfcGetErrHold(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
        {
            DBG_Printf("PU %d XOR NFC protect fail, Full Block Erase Error!\n", pCurFlashAddr->ucPU);
            XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
            DBG_Getch();
        }

        if (FALSE == HAL_NfcGetFull(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
        {
            HAL_NfcFullBlockErase(pCurFlashAddr, FALSE);

            COM_MemCpy((U32 *)&(p3dMlcNfcProtectCtrl->tPreEraseBlkFlashAddr), (U32 *)pCurFlashAddr,
                (sizeof(FLASH_ADDR) / sizeof(U32)));

            p3dMlcNfcProtectCtrl->ucStatus = XOR_UT_PROGRAM_PAGE;
        }

        break;
    }
    case XOR_UT_PROGRAM_PAGE:
    {
        XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucXoreId < XORE_CNT);

        FLASH_ADDR *pCurFlashAddr = &(p3dMlcNfcProtectCtrl->tCurrentFlashAddr);

        if (TRUE == HAL_NfcGetErrHold(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
        {
            DBG_Printf("PU %d XOR NFC protect fail, program page error!\n", pCurFlashAddr->ucPU);
            XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
            DBG_Getch();
        }

        if (FALSE == HAL_NfcGetFull(pCurFlashAddr->ucPU, pCurFlashAddr->ucLun))
        {
            U32 ulXoreId = p3dMlcNfcProtectCtrl->ucXoreId;
            U8 *pCurXorPageNumInXore = &(p3dMlcNfcProtectCtrl->ucCurXorPageNumInXore);
            U32 ulXorPageNumInTotal = p3dMlcNfcProtectCtrl->usStartXorPageNumInTotal + *pCurXorPageNumInXore;

            U32 ulPageDataAddr = XorUt_GetPageDataAddrTlc(ulXoreId, p3dMlcNfcProtectCtrl->ulXorPageSize,
                *pCurXorPageNumInXore);
            U32 ulPageRedunAddr = XorUt_GetPageRedunAddr(ulXoreId, p3dMlcNfcProtectCtrl->ulXorPageSize,
                *pCurXorPageNumInXore);
            BOOL bIsLastNfcInXor = FALSE;


            bIsLastNfcInXor = (*pCurXorPageNumInXore == (g_usXorProtectRatio - XORUT_PLN_PER_LUN)) ? TRUE : FALSE;
            XorUt_Nfc3dMlcFullPlnWrite(pCurFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                ulXorPageNumInTotal, ulXoreId, bIsLastNfcInXor);
            (*pCurXorPageNumInXore) += XORUT_PLN_PER_LUN;


            if ((*pCurXorPageNumInXore) == g_usXorProtectRatio)
            {
                p3dMlcNfcProtectCtrl->ucStatus = XOR_UT_WAIT_DONE;
            }
            else
            {
                p3dMlcNfcProtectCtrl->ucStatus = XOR_UT_IS_NEED_ERASE_BLOCK;
            }
        }

        break;
    }
    case XOR_UT_WAIT_DONE:
    {
        XORUT_ASSERT(p3dMlcNfcProtectCtrl->ucXoreId < XORE_CNT);

        if (TRUE == XorUt_IsAllXorPageNfcCmdDone(p3dMlcNfcProtectCtrl->usStartXorPageNumInTotal, XOR_PROTECT))
        {
            // NFC should release this XOR engine.
            /*if (XorUt_IsXoreValid(pNfcProtectCtrl->ucXoreId) == FALSE)
            {
            DBG_Printf("This XOR engine should valid at here!\n");
            XorUt_Log(XOR_UT_GENERAL_ERROR, NULL);
            DBG_Getch();
            }*/

            b3dMlcXorNfcProtectFinish = TRUE;
        }

        break;
    }
    default:
    {
        XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
        DBG_Getch();
    }
    }

    return b3dMlcXorNfcProtectFinish;
}
#endif

BOOL XorUt_NfcRecover(XOR_NFC_RECOVER_CTRL *pNfcRecoverCtrl)
{
    XORUT_ASSERT(pNfcRecoverCtrl != NULL);
    XORUT_ASSERT(pNfcRecoverCtrl->usStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((pNfcRecoverCtrl->usStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT(pNfcRecoverCtrl->usBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((pNfcRecoverCtrl->usBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(pNfcRecoverCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    XORUT_ASSERT(pNfcRecoverCtrl->ucCurXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT(pNfcRecoverCtrl->ucStoreXorPageNumInXore < g_usXorProtectRatio);
    XORUT_ASSERT((g_bIsSinglePlaneOperate == TRUE) || (g_bIsSinglePlaneOperate == FALSE));

    BOOL bXorNfcRecoverFinish = FALSE;

    switch (pNfcRecoverCtrl->ucStatus)
    {
        case XOR_UT_INIT:
        {
            XORUT_ASSERT(pNfcRecoverCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

            U32 ulXoreId = 0;

            if (TRUE == HAL_GetXore(pNfcRecoverCtrl->ulXorPageSize, &ulXoreId))
            {
                pNfcRecoverCtrl->ucXoreId = ulXoreId;

                XorUt_NfcRecoverCleanData(ulXoreId, pNfcRecoverCtrl->ulXorPageSize);

                U32 ulDestDataAddr = XorUt_GetDestPageDataAddr(ulXoreId, pNfcRecoverCtrl->ulXorPageSize);
                U32 ulDestRedunAddr = XorUt_GetDestPageRedunAddr(ulXoreId, pNfcRecoverCtrl->ulXorPageSize);

                HAL_XoreNfcModeConfig(ulXoreId, XOR_RECOVER, g_usXorProtectRatio, 0, FALSE);
                //HAL_XoreAutoLoad(ulXoreId, ulDestDataAddr, ulDestRedunAddr);

                HAL_XoreTrigger(ulXoreId);

                pNfcRecoverCtrl->usStartXorPageNumInTotal = 
                    XorUt_GetStartXorPageNum(pNfcRecoverCtrl->usBrokenXorPageNumInTotal);

                pNfcRecoverCtrl->ucStatus = XOR_UT_READ_PAGE;
            }

            break;
        }
        case XOR_UT_READ_PAGE:
        {
            XORUT_ASSERT(pNfcRecoverCtrl->ucXoreId < XORE_CNT);

            U8 *pCurXorPageNumInXore = &(pNfcRecoverCtrl->ucCurXorPageNumInXore);
            U32 ulStartXorPageNumInTotal = pNfcRecoverCtrl->usStartXorPageNumInTotal;
            U32 ulBrokenXorPageNumInTotal = pNfcRecoverCtrl->usBrokenXorPageNumInTotal;

            if ((ulStartXorPageNumInTotal + *pCurXorPageNumInXore) == ulBrokenXorPageNumInTotal)
            {
                ++(*pCurXorPageNumInXore);
                break;
            }

            U32 ulXorPageNumInTotal = ulStartXorPageNumInTotal + *pCurXorPageNumInXore;
            FLASH_ADDR tFlashAddr = XorUt_XorPageLocate(pNfcRecoverCtrl->pXorPageLocationTable, *pCurXorPageNumInXore);

            if (TRUE == HAL_NfcGetErrHold(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                DBG_Printf("PU %d XOR NFC recover fail, read page error!\n", tFlashAddr.ucPU);
                XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
                DBG_Getch();
            }

            if (FALSE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                U32 ulXoreId = pNfcRecoverCtrl->ucXoreId;
                U8 *pStoreXorPageNumInXore = &(pNfcRecoverCtrl->ucStoreXorPageNumInXore);

                U32 ulPageDataAddr = XorUt_GetPageDataAddr(ulXoreId, pNfcRecoverCtrl->ulXorPageSize,
                    *pStoreXorPageNumInXore);
                U32 ulPageRedunAddr = XorUt_GetPageRedunAddr(ulXoreId, pNfcRecoverCtrl->ulXorPageSize,
                    *pStoreXorPageNumInXore);

                if (g_bIsSinglePlaneOperate == TRUE)
                {
                    XorUt_NfcSinglePlnRead(&tFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                        ulXorPageNumInTotal, TRUE, ulXoreId, FALSE, 0);
                    ++(*pCurXorPageNumInXore); ++(*pStoreXorPageNumInXore);
                }
                else
                {
                    if (TRUE == XorUt_IsInSamePlane(ulBrokenXorPageNumInTotal, ulStartXorPageNumInTotal +
                        *pCurXorPageNumInXore))
                    {
                        XorUt_NfcSinglePlnRead(&tFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                            ulXorPageNumInTotal, TRUE, ulXoreId, FALSE, 0);
                        ++(*pCurXorPageNumInXore); ++(*pStoreXorPageNumInXore);
                    }
                    else
                    {
                        XorUt_NfcFullPlnRead(&tFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                            ulXorPageNumInTotal, TRUE, ulXoreId);
                        (*pCurXorPageNumInXore) += XORUT_PLN_PER_LUN;
                        (*pStoreXorPageNumInXore) += XORUT_PLN_PER_LUN;
                    }
                }

                if ((*pCurXorPageNumInXore) == g_usXorProtectRatio)
                {
                    pNfcRecoverCtrl->ucStatus = XOR_UT_WAIT_DONE;
                }
            }

            break;
        }
        case XOR_UT_WAIT_DONE:
        {
            XORUT_ASSERT(pNfcRecoverCtrl->ucXoreId < XORE_CNT);

            if (XOR_FINISH == HAL_XoreGetStatus(pNfcRecoverCtrl->ucXoreId))
            {
                if (TRUE == XorUt_IsAllXorPageNfcCmdDone(pNfcRecoverCtrl->usStartXorPageNumInTotal, XOR_RECOVER))
                {
                    bXorNfcRecoverFinish = TRUE;
                }
            }

            break;
        }
        default:
        {
            XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
            DBG_Getch();
        }
    }

    return bXorNfcRecoverFinish;
}

BOOL XorUt_BypassNfc(XOR_BYPASS_NFC_CTRL *pBypassNfcCtrl)
{
    XORUT_ASSERT(pBypassNfcCtrl != NULL);
    XORUT_ASSERT((pBypassNfcCtrl->eTarget == XOR_PROTECT) || (pBypassNfcCtrl->eTarget == XOR_RECOVER));
    XORUT_ASSERT(pBypassNfcCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    XORUT_ASSERT(pBypassNfcCtrl->ucCurXorPageNumInXore < (g_usXorProtectRatio - 1));

    BOOL bBypassNfcFinish = FALSE;

    switch (pBypassNfcCtrl->ucStatus)
    {
        case XOR_UT_INIT:
        {
            XORUT_ASSERT(pBypassNfcCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

            U32 ulXoreId = 0;

            if (TRUE == HAL_GetXore(pBypassNfcCtrl->ulXorPageSize, &ulXoreId))
            {
                pBypassNfcCtrl->ucXoreId = ulXoreId;

                if (pBypassNfcCtrl->eTarget == XOR_PROTECT)
                {
                    XorUt_ProtectPrepareData(ulXoreId, pBypassNfcCtrl->ulXorPageSize,
                        pBypassNfcCtrl->usStartXorPageNumInTotal);
                } 
                else
                {
                    XorUt_BypassRecoverPrepareData(ulXoreId, pBypassNfcCtrl->ulXorPageSize,
                        pBypassNfcCtrl->usBrokenXorPageNumInTotal);
                }

                XorUt_DestPageClean(ulXoreId, pBypassNfcCtrl->ulXorPageSize);

                U32 ulDestDataAddr = XorUt_GetDestPageDataAddr(ulXoreId, pBypassNfcCtrl->ulXorPageSize);
                U32 ulDestRedunAddr = XorUt_GetDestPageRedunAddr(ulXoreId, pBypassNfcCtrl->ulXorPageSize);

                HAL_XoreBpsModeConfig(ulXoreId, pBypassNfcCtrl->eTarget, g_usXorProtectRatio,
                    g_usRedunSizePerSinglePage, FALSE);
                HAL_XoreAutoLoad(ulXoreId, ulDestDataAddr, ulDestRedunAddr);

                if (pBypassNfcCtrl->eTarget == XOR_RECOVER)
                {
                    // HAL_XorePartialOperate(ulXoreId, 4, 2);
                }

                pBypassNfcCtrl->ucXorPageCntOfLastTrigger = (g_usXorProtectRatio - 1) % XOR_SRC_DADDR_REG_CNT;
                pBypassNfcCtrl->ucStartXorPageNumOfLastTrigger = 
                    (g_usXorProtectRatio - 1) - pBypassNfcCtrl->ucXorPageCntOfLastTrigger;

                pBypassNfcCtrl->ucStatus = XOR_UT_XOR_CALCULATE;
            }

            break;
        }
        case XOR_UT_XOR_CALCULATE:
        {
            XORUT_ASSERT(pBypassNfcCtrl->ucXoreId < XORE_CNT);

            U32 i = 0;
            XOR_SRC_DATA tBypassSrcData;
            U8 *pCurXorPageNumInXore = &(pBypassNfcCtrl->ucCurXorPageNumInXore);
            U32 ulLoopTimes = (*pCurXorPageNumInXore == pBypassNfcCtrl->ucStartXorPageNumOfLastTrigger) ?
                pBypassNfcCtrl->ucXorPageCntOfLastTrigger : XOR_SRC_DADDR_REG_CNT;

            for (i = 0; i < ulLoopTimes; ++i)
            {
                tBypassSrcData.aPageDramAddr[i] = XorUt_GetPageDataAddr(pBypassNfcCtrl->ucXoreId,
                    pBypassNfcCtrl->ulXorPageSize, (i + *pCurXorPageNumInXore));
                tBypassSrcData.aRedunDramAddr[i] = XorUt_GetPageRedunAddr(pBypassNfcCtrl->ucXoreId,
                    pBypassNfcCtrl->ulXorPageSize, (i + *pCurXorPageNumInXore));
            }

            tBypassSrcData.ulValidAddrCnt = ulLoopTimes;

            HAL_XoreSetSrcDataAddr(pBypassNfcCtrl->ucXoreId, &tBypassSrcData, !(*pCurXorPageNumInXore));
            HAL_XoreTrigger(pBypassNfcCtrl->ucXoreId);

            pBypassNfcCtrl->ucStatus = XOR_UT_WAIT_DONE;

            break;
        }
        case XOR_UT_WAIT_DONE:
        {
            XORUT_ASSERT(pBypassNfcCtrl->ucXoreId < XORE_CNT);

            XOR_SATUS eResult = HAL_XoreGetStatus(pBypassNfcCtrl->ucXoreId);
            if ((eResult == XOR_PARTIAL_FINISH) || (eResult == XOR_FINISH))
            {
                U32 ulLoopTimes = 
                    (pBypassNfcCtrl->ucCurXorPageNumInXore == pBypassNfcCtrl->ucStartXorPageNumOfLastTrigger) ?
                    pBypassNfcCtrl->ucXorPageCntOfLastTrigger : XOR_SRC_DADDR_REG_CNT;

                pBypassNfcCtrl->ucCurXorPageNumInXore += ulLoopTimes;

                if (pBypassNfcCtrl->ucCurXorPageNumInXore == (U32)(g_usXorProtectRatio - 1))
                {
                    bBypassNfcFinish = TRUE;
                } 
                else
                {
                    pBypassNfcCtrl->ucStatus = XOR_UT_XOR_CALCULATE;
                }
            }
            
            break;
        }
        default:
        {
            XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
            DBG_Getch();
        }
    }

    return bBypassNfcFinish;
}

BOOL XorUt_BypassRecoverWithNfcRead(XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL *pBypassRecoverWithNfcReadCtrl)
{
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl != NULL);
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->usStartXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((pBypassRecoverWithNfcReadCtrl->usStartXorPageNumInTotal % g_usXorProtectRatio) == 0);
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->usBrokenXorPageNumInTotal < (U32)(g_usXorProtectRatio * XORE_CNT));
    XORUT_ASSERT((pBypassRecoverWithNfcReadCtrl->usBrokenXorPageNumInTotal % g_usXorProtectRatio) != (g_usXorProtectRatio - 1));
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore <= g_usXorProtectRatio);
    XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucStoreXorPageNumInXore < g_usXorProtectRatio);
    XORUT_ASSERT((pBypassRecoverWithNfcReadCtrl->bIsTlcRecover == TRUE) || (pBypassRecoverWithNfcReadCtrl->bIsTlcRecover == FALSE));
    XORUT_ASSERT((g_bIsSinglePlaneOperate == TRUE) || (g_bIsSinglePlaneOperate == FALSE));

    BOOL bBypassRecoverWithNfcReadFinish = FALSE;

    switch (pBypassRecoverWithNfcReadCtrl->ucStatus)
    {
        case XOR_UT_INIT:
        {
            XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

            U32 ulXoreId = 0;

            if (TRUE == HAL_GetXore(pBypassRecoverWithNfcReadCtrl->ulXorPageSize, &ulXoreId))
            {
                pBypassRecoverWithNfcReadCtrl->ucXoreId = ulXoreId;

                XorUt_NfcRecoverCleanData(ulXoreId, pBypassRecoverWithNfcReadCtrl->ulXorPageSize);

                pBypassRecoverWithNfcReadCtrl->usStartXorPageNumInTotal =
                    XorUt_GetStartXorPageNum(pBypassRecoverWithNfcReadCtrl->usBrokenXorPageNumInTotal);

                pBypassRecoverWithNfcReadCtrl->ucStatus = XOR_UT_READ_PAGE;
            }

            break;
        }
        case XOR_UT_READ_PAGE:
        {
            XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucXoreId < XORE_CNT);

            U8 *pCurXorPageNumInXore = &(pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore);
            U32 ulStartXorPageNumInTotal = pBypassRecoverWithNfcReadCtrl->usStartXorPageNumInTotal;
            U32 ulBrokenXorPageNumInTotal = pBypassRecoverWithNfcReadCtrl->usBrokenXorPageNumInTotal;

            if ((ulStartXorPageNumInTotal + *pCurXorPageNumInXore) == ulBrokenXorPageNumInTotal)
            {
                ++(*pCurXorPageNumInXore);
                break;
            }

            U32 ulXorPageNumInTotal = ulStartXorPageNumInTotal + *pCurXorPageNumInXore;
            FLASH_ADDR tFlashAddr = { 0 };

            if (pBypassRecoverWithNfcReadCtrl->bIsTlcRecover == TRUE)
            {
                tFlashAddr = XorUt_GetTlcXorPageLocation(pBypassRecoverWithNfcReadCtrl->pXorPageLocationTable,
                    0, *pCurXorPageNumInXore);
            }
            else
            {
                tFlashAddr = XorUt_XorPageLocate(pBypassRecoverWithNfcReadCtrl->pXorPageLocationTable,
                    *pCurXorPageNumInXore);
            }

            if (TRUE == HAL_NfcGetErrHold(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                DBG_Printf("PU %d XOR bypass recover with NFC read fail, read page error!\n", tFlashAddr.ucPU);
                XorUt_Log(XOR_UT_NFC_CMD_ERROR, NULL);
                DBG_Getch();
            }

            if (FALSE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                U32 ulXoreId = pBypassRecoverWithNfcReadCtrl->ucXoreId;
                U8 *pStoreXorPageNumInXore = &(pBypassRecoverWithNfcReadCtrl->ucStoreXorPageNumInXore);

                U32 ulPageDataAddr = XorUt_GetPageDataAddr(ulXoreId, pBypassRecoverWithNfcReadCtrl->ulXorPageSize,
                    *pStoreXorPageNumInXore);
                U32 ulPageRedunAddr = XorUt_GetPageRedunAddr(ulXoreId, pBypassRecoverWithNfcReadCtrl->ulXorPageSize,
                    *pStoreXorPageNumInXore);

                if ((g_bIsSinglePlaneOperate == TRUE) || (pBypassRecoverWithNfcReadCtrl->bIsTlcRecover == TRUE))
                {
                    XorUt_NfcSinglePlnRead(&tFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                        ulXorPageNumInTotal, FALSE, ulXoreId, pBypassRecoverWithNfcReadCtrl->bIsTlcRecover,
                        pBypassRecoverWithNfcReadCtrl->ucPageType);
                    ++(*pCurXorPageNumInXore); ++(*pStoreXorPageNumInXore);
                }
                else
                {
                    if (TRUE == XorUt_IsInSamePlane(ulBrokenXorPageNumInTotal, ulStartXorPageNumInTotal +
                        *pCurXorPageNumInXore))
                    {
                        XorUt_NfcSinglePlnRead(&tFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                            ulXorPageNumInTotal, FALSE, ulXoreId, FALSE, 0);
                        ++(*pCurXorPageNumInXore); ++(*pStoreXorPageNumInXore);
                    }
                    else
                    {
                        XorUt_NfcFullPlnRead(&tFlashAddr, ulPageDataAddr, ulPageRedunAddr,
                            ulXorPageNumInTotal, FALSE, ulXoreId);
                        (*pCurXorPageNumInXore) += XORUT_PLN_PER_LUN;
                        (*pStoreXorPageNumInXore) += XORUT_PLN_PER_LUN;
                    }
                }

                if ((*pCurXorPageNumInXore) == g_usXorProtectRatio)
                {
                    pBypassRecoverWithNfcReadCtrl->ucStatus = XOR_UT_WAIT_NFC_READ_DONE;
                }
            }

            break;
        }
        case XOR_UT_WAIT_NFC_READ_DONE:
        {
            XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucXoreId < XORE_CNT);

            if (TRUE == XorUt_IsAllXorPageNfcCmdDone(pBypassRecoverWithNfcReadCtrl->usStartXorPageNumInTotal, XOR_RECOVER))
            {
                U32 ulXoreId = pBypassRecoverWithNfcReadCtrl->ucXoreId;
                U32 ulXorPageSize = pBypassRecoverWithNfcReadCtrl->ulXorPageSize;

                // Check user data and redundant that be used to recover the broken XOR page.
                XorUt_NfcReadCheckData(ulXoreId, ulXorPageSize, 
                    pBypassRecoverWithNfcReadCtrl->usBrokenXorPageNumInTotal);

                XorUt_DestPageClean(ulXoreId, ulXorPageSize);

                U32 ulDestDataAddr = XorUt_GetDestPageDataAddr(ulXoreId, ulXorPageSize);
                U32 ulDestRedunAddr = XorUt_GetDestPageRedunAddr(ulXoreId, ulXorPageSize);

                HAL_XoreBpsModeConfig(ulXoreId, XOR_RECOVER, g_usXorProtectRatio, g_usRedunSizePerSinglePage, FALSE);
                HAL_XoreAutoLoad(ulXoreId, ulDestDataAddr, ulDestRedunAddr);

                pBypassRecoverWithNfcReadCtrl->ucXorPageCntOfLastTrigger = (g_usXorProtectRatio - 1) % XOR_SRC_DADDR_REG_CNT;
                pBypassRecoverWithNfcReadCtrl->ucStartXorPageNumOfLastTrigger =
                    (g_usXorProtectRatio - 1) - pBypassRecoverWithNfcReadCtrl->ucXorPageCntOfLastTrigger;

                pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore = 0;
                pBypassRecoverWithNfcReadCtrl->ucStatus = XOR_UT_XOR_CALCULATE;
            }

            break;
        }
        case XOR_UT_XOR_CALCULATE:
        {
            XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucXoreId < XORE_CNT);

            U32 i = 0;
            XOR_SRC_DATA tBypassSrcData;
            U8 *pCurXorPageNumInXore = &(pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore);
            U32 ulLoopTimes = (*pCurXorPageNumInXore == pBypassRecoverWithNfcReadCtrl->ucStartXorPageNumOfLastTrigger) ?
                pBypassRecoverWithNfcReadCtrl->ucXorPageCntOfLastTrigger : XOR_SRC_DADDR_REG_CNT;

            for (i = 0; i < ulLoopTimes; ++i)
            {
                tBypassSrcData.aPageDramAddr[i] = XorUt_GetPageDataAddr(pBypassRecoverWithNfcReadCtrl->ucXoreId,
                    pBypassRecoverWithNfcReadCtrl->ulXorPageSize, (i + *pCurXorPageNumInXore));
                tBypassSrcData.aRedunDramAddr[i] = XorUt_GetPageRedunAddr(pBypassRecoverWithNfcReadCtrl->ucXoreId,
                    pBypassRecoverWithNfcReadCtrl->ulXorPageSize, (i + *pCurXorPageNumInXore));
            }

            tBypassSrcData.ulValidAddrCnt = ulLoopTimes;

            HAL_XoreSetSrcDataAddr(pBypassRecoverWithNfcReadCtrl->ucXoreId, &tBypassSrcData, !(*pCurXorPageNumInXore));
            HAL_XoreTrigger(pBypassRecoverWithNfcReadCtrl->ucXoreId);

            pBypassRecoverWithNfcReadCtrl->ucStatus = XOR_UT_WAIT_DONE;

            break;
        }
        case XOR_UT_WAIT_DONE:
        {
            XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->ucXoreId < XORE_CNT);

            XOR_SATUS eResult = HAL_XoreGetStatus(pBypassRecoverWithNfcReadCtrl->ucXoreId);
            if ((eResult == XOR_PARTIAL_FINISH) || (eResult == XOR_FINISH))
            {
                U32 ulLoopTimes =
                    (pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore == 
                    pBypassRecoverWithNfcReadCtrl->ucStartXorPageNumOfLastTrigger) ?
                    pBypassRecoverWithNfcReadCtrl->ucXorPageCntOfLastTrigger : XOR_SRC_DADDR_REG_CNT;

                pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore += ulLoopTimes;

                if (pBypassRecoverWithNfcReadCtrl->ucCurXorPageNumInXore == (U32)(g_usXorProtectRatio - 1))
                {
                    bBypassRecoverWithNfcReadFinish = TRUE;
                }
                else
                {
                    pBypassRecoverWithNfcReadCtrl->ucStatus = XOR_UT_XOR_CALCULATE;
                }
            }

            break;
        }
        default:
        {
            XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
            DBG_Getch();
        }
    }

    return bBypassRecoverWithNfcReadFinish;
}

BOOL XorUt_LoadStore(XOR_LOAD_STORE_CTRL *pLoadStoreCtrl)
{
    XORUT_ASSERT(pLoadStoreCtrl != NULL);
    XORUT_ASSERT((pLoadStoreCtrl->eTarget == XOR_LOAD_FROM_DRAM) || (pLoadStoreCtrl->eTarget == XOR_STORE_TO_DRAM));
    XORUT_ASSERT(pLoadStoreCtrl->ucStatus < XOR_UT_STATE_ASSERT);
    
    BOOL bLoadStoreFinish = FALSE;

    switch (pLoadStoreCtrl->ucStatus)
    {
        case XOR_UT_INIT:
        {
            XORUT_ASSERT(pLoadStoreCtrl->ucXoreId == XORUT_INVALID_XORE_ID);

            U32 ulXoreId = 0;

            if (TRUE == HAL_GetXore(pLoadStoreCtrl->ulXorPageSize, &ulXoreId))
            {
                pLoadStoreCtrl->ucXoreId = ulXoreId;

                if (pLoadStoreCtrl->eTarget == XOR_LOAD_FROM_DRAM)
                {
                    XorUt_LoadFromDramPrepareData(ulXoreId, pLoadStoreCtrl->ulXorPageSize,
                        pLoadStoreCtrl->usXorPageNumInTotal);
                }
                else
                {
                    XorUt_StoreToDramPrepareData(ulXoreId, pLoadStoreCtrl->ulXorPageSize,
                        pLoadStoreCtrl->usXorPageNumInTotal);
                    XorUt_LoadStorePageClean(ulXoreId, pLoadStoreCtrl->ulXorPageSize);
                }

                pLoadStoreCtrl->ucStatus = XOR_UT_LOAD_STORE;
            }

            break;
        }
        case XOR_UT_LOAD_STORE:
        {
            XORUT_ASSERT(pLoadStoreCtrl->ucXoreId < XORE_CNT);

            U32 ulLoadStoreDataAddr = XorUt_GetLoadStoreDataAddr(pLoadStoreCtrl->ucXoreId,
                pLoadStoreCtrl->ulXorPageSize);
            U32 ulLoadStoreRedunAddr = XorUt_GetLoadStoreRedunAddr(pLoadStoreCtrl->ucXoreId,
                pLoadStoreCtrl->ulXorPageSize);

            HAL_XoreLoadStoreModeConfig(pLoadStoreCtrl->ucXoreId, pLoadStoreCtrl->eTarget,
                ulLoadStoreDataAddr, ulLoadStoreRedunAddr, g_usRedunSizePerSinglePage, FALSE);

            HAL_XoreTrigger(pLoadStoreCtrl->ucXoreId);

            pLoadStoreCtrl->ucStatus = XOR_UT_WAIT_DONE;

            break;
        }
        case XOR_UT_WAIT_DONE:
        {
            XORUT_ASSERT(pLoadStoreCtrl->ucXoreId < XORE_CNT);

            XOR_SATUS eResult = HAL_XoreGetStatus(pLoadStoreCtrl->ucXoreId);
            if (eResult == XOR_FINISH)
            {
                bLoadStoreFinish = TRUE;
            }

            break;
        }
        default:
        {
            XorUt_Log(XOR_UT_SWITCH_CASE_ERROR, NULL);
            DBG_Getch();
        }
    }

    return bLoadStoreFinish;
}
