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
#include "Xor_Test.h"
#include "L2_FCMDQ.h"
#include "L2_FTL.h"
#include "FCMD_Test.h"
#include "FW_Debug.h"

// If you change the LUN count of super PU, the XOR protect ratio will be changed, so you must
// modify the value of "l_kXorProtectRatio" in L3_DataRecover.c.
#define L3UT_XOR_LUN_PER_SUPERPU   2
#define L3UT_XOR_SUPER_PAGE_CNT    5

typedef enum _L3_UNIT_TEST_XOR_PATTERN
{
    L3UT_XOR_TLC_PROTECT = 0,
    L3UT_XOR_SLC_PROTECT,
    L3UT_XOR_RECOVER,
    L3UT_XOR_PATTERN_CNT, 
    L3UT_XOR_PATTERN_ASSERT = L3UT_XOR_PATTERN_CNT,
}L3UT_XOR_PATTERN;

typedef struct _L3_UNIT_TEST_XOR_PATTERN_INFORMATION
{
    L3UT_XOR_PATTERN ePatternId;
    U16 usStartBlock;
    U16 usEndBlock;
    U8 ucSuperPu;
}L3UT_XOR_PATTERN_INFO;

LOCAL const L3UT_XOR_PATTERN_INFO l_aXorTestTaskList[] =
{
    // {ePatternId, usStartBlock, usEndBlock, ucSuperPu}
    {L3UT_XOR_TLC_PROTECT, 19, 22, 0},
    {L3UT_XOR_SLC_PROTECT, 1, 5, 0},
    {L3UT_XOR_SLC_PROTECT, 6, 9, 0},
    {L3UT_XOR_SLC_PROTECT, 10, 12, 0},
    {L3UT_XOR_SLC_PROTECT, 15, 17, 0},
    //{L3UT_XOR_RECOVER},
};

LOCAL BOOL l_bIsInfiniteLoop = TRUE;
LOCAL U8 l_ucCheckDwPerSector = 2;      // (1 ~ SEC_SIZE_DW(128))


#ifdef SIM
#define DEBUG_GET_CHAR() __debugbreak()
#else
#define DEBUG_GET_CHAR()\
    do\
    {\
        PRINT_ERR("Fatal Error, MCU %d DBG_Getch!!!\n", HAL_GetMcuId());\
        while(1);\
    } while (0)
#endif

#define STR(x) #x

#define L3UT_XOR_PROTECT_RATIO     (L3UT_XOR_LUN_PER_SUPERPU * PLN_PER_LUN)
#define L3UT_XOR_TEST_TASK_CNT     (sizeof(l_aXorTestTaskList) / sizeof(l_aXorTestTaskList[0]))

#define SEC_SIZE_DW          (SEC_SIZE / sizeof(U32))

typedef enum _L3_UNIT_TEST_XOR_STATE
{
    L3UT_XOR_INIT,
    L3UT_XOR_IS_NEED_ERASE_BLOCK,
    L3UT_XOR_ERASE_BLOCK,
    L3UT_XOR_PROGRAM_PAGE,
    L3UT_XOR_READ_PAGE,
    L3UT_XOR_READ_PARITY,
    L3UT_XOR_WAIT_DONE,
    L3UT_XOR_STATE_ASSERT
}L3UT_XOR_STATE;

// Control information of SLC/MLC/TLC protect flow.
typedef struct _L3_UNIT_TEST_XOR_PROTECT_CONTROL
{
    L3UT_XOR_STATE eStatus;
    U32 *pXorPageParity[PG_PER_WL];
    U32 *pXorRedunParity[PG_PER_WL];
    volatile U8 *pReadCmdStatus[PG_PER_WL];
    BOOL bIsHighestPriority;
    U16 usCurBlock;
    U16 usCurSuperPage;       // For SLC/MLC it is index of current super page, for TLC it is
                              // the current program order.
    U8 ucSuperPu;
    U8 ucCurLunInSuperPu;
    U8 bsXorStripeId   : 4;
    U8 bsCurReadIndex  : 4;   // Only used in TLC protect.
}L3UT_XOR_PROTECT_CTRL;

// Control information of recover flow.
typedef struct _L3_UNIT_TEST_XOR_RECOVER_CONTROL
{
    L3UT_XOR_STATE eStatus;
    BOOL bIsHighestPriority;
}L3UT_XOR_RECOVER_CTRL;

typedef struct _L3_UNIT_TEST_XOR_TASK
{
    const L3UT_XOR_PATTERN_INFO *pPatternInfo;
    BOOL bIsFinished;
    U32 ulLoopCnt;
    L3UT_XOR_PROTECT_CTRL tProtectCtrl;
    L3UT_XOR_RECOVER_CTRL tRecoverCtrl;
    U8 ucTaskId;
}L3UT_XOR_TEST_TASK;

typedef struct _L3_UNIT_TEST_XOR_PARAMETER
{
    PhysicalAddr tPhyAddr;
    XOR_PARAM tXorParam;
    U32 aPageAddr[PG_PER_WL];
    U32 ulSpareAddr;
    U32 ulStatusAddr;
}L3UT_XOR_PARAM;

LOCAL U8 l_ucXorStripeId = 0;

void L3Ut_XorTestTaskStateUpdate(L3UT_XOR_TEST_TASK *pTestTask)
{
    ASSERT(pTestTask != NULL);
    ASSERT(pTestTask->pPatternInfo != NULL);

    if (pTestTask->tProtectCtrl.usCurBlock == pTestTask->pPatternInfo->usEndBlock)
    {
        if (TRUE == l_bIsInfiniteLoop)
        {
            pTestTask->tProtectCtrl.usCurBlock = pTestTask->pPatternInfo->usStartBlock;
            ++(pTestTask->ulLoopCnt);
            PRINT_INFO("Infinite Loop Mode: Task %d has finished %d loops!\n", pTestTask->ucTaskId,
                       pTestTask->ulLoopCnt);
        }
        else
        {
            pTestTask->bIsFinished = TRUE;
        }
    }
    else
    {
        ++(pTestTask->tProtectCtrl.usCurBlock);
    }
    return;
}

void L3Ut_XorInit1BlockProtectCtrl(L3UT_XOR_TEST_TASK *pTestTask)
{
    ASSERT(pTestTask != NULL);
    ASSERT(pTestTask->pPatternInfo != NULL);

    L3UT_XOR_PROTECT_CTRL *pProtectCtrl = &(pTestTask->tProtectCtrl);
    pProtectCtrl->eStatus = L3UT_XOR_INIT;
    pProtectCtrl->bsXorStripeId = 0;
    pProtectCtrl->usCurSuperPage = 0;
    pProtectCtrl->ucSuperPu = pTestTask->pPatternInfo->ucSuperPu;
    pProtectCtrl->ucCurLunInSuperPu = 0;
    U32 i = 0;
    for (i = 0; i < PG_PER_WL; ++i)
    {
        pProtectCtrl->pReadCmdStatus[i] = NULL;
        pProtectCtrl->pXorPageParity[i] = NULL;
        pProtectCtrl->pXorRedunParity[i] = NULL;
    }
    pProtectCtrl->bIsHighestPriority = FALSE;
    pProtectCtrl->bsCurReadIndex = 0;
    return;
}

void L3Ut_XorClearRecoverCtrl(L3UT_XOR_RECOVER_CTRL *pRecoverCtrl)
{
    ASSERT(pRecoverCtrl != NULL);

    pRecoverCtrl->eStatus = L3UT_XOR_INIT;
    pRecoverCtrl->bIsHighestPriority = FALSE;
    return;
}

void L3Ut_XorTestTaskInit(L3UT_XOR_TEST_TASK aTestTask[])
{
    U32 i = 0;
    for (i = 0; i < L3UT_XOR_TEST_TASK_CNT; ++i)
    {
        L3UT_XOR_TEST_TASK *pTestTask = &(aTestTask[i]);
        pTestTask->ucTaskId = i;
        pTestTask->pPatternInfo = &l_aXorTestTaskList[i];   
        pTestTask->bIsFinished = FALSE;
        pTestTask->ulLoopCnt = 0;
        pTestTask->tProtectCtrl.usCurBlock = pTestTask->pPatternInfo->usStartBlock;

        L3Ut_XorInit1BlockProtectCtrl(pTestTask);
        L3Ut_XorClearRecoverCtrl(&(pTestTask->tRecoverCtrl));
    }
    return;
}

U32 L3Ut_XorGetDataPattern(U32 ulXorPageNumInTotal)
{
    ASSERT(ulXorPageNumInTotal < (U32)((L3UT_XOR_SUPER_PAGE_CNT + 3) * L3UT_XOR_PROTECT_RATIO));

    static U32 l_aUserDataPattern[8] = { 0xA5A5A5A5, 0x12345678, 0xD7D6D9E2, 0xABCDEF01, 0x7EC029B5,
        0xDCBA9876, 0xA9B6C3D7, 0xF0E1D2C3 };

    return (l_aUserDataPattern[ulXorPageNumInTotal % 8] + (ulXorPageNumInTotal << 16) +
            ulXorPageNumInTotal);
}

// Prepare data of one XOR page. 
void L3Ut_XorDataPrepare(U32 *pPageData, U32 ulXorPageSize, U32 ulXorPageNumInTotal)
{
    ASSERT(pPageData != NULL);
    ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    ASSERT(ulXorPageNumInTotal < (U32)((L3UT_XOR_SUPER_PAGE_CNT + 3) * L3UT_XOR_PROTECT_RATIO));
    
    U32 i = 0;
    U32 ulDataPattern = L3Ut_XorGetDataPattern(ulXorPageNumInTotal);
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        COM_MemSet(pPageData, l_ucCheckDwPerSector, ulDataPattern);
        pPageData += SEC_SIZE_DW;
    }

    return;
}

void L3Ut_XorRedunPrepare(U32 *pPageRedun, U32 ulXorPageSize, U32 ulXorPageCnt)
{
    ASSERT(pPageRedun != NULL);
    ASSERT((ulXorPageCnt > 0) && (ulXorPageCnt <= PLN_PER_LUN * PG_PER_WL));
    ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0;
    U32 ulRedunCnt = (ulXorPageCnt * ulXorPageSize) / LOGIC_PG_SZ;
    
    for (i = 0; i < ulRedunCnt; ++i)
    {
        COM_MemSet(pPageRedun, RED_SZ_DW, 0x0b55aa12);
        pPageRedun += RED_SZ_DW;
    }
    
    return;
}

void L3Ut_XorPrepareCommon(const L3UT_XOR_PROTECT_CTRL *pProtectCtrl,
                           L3UT_XOR_PARAM *pL3UtXorParam)
{
    ASSERT(pProtectCtrl != NULL);
    ASSERT(pL3UtXorParam != NULL);

    PhysicalAddr *pPhyAddr = &(pL3UtXorParam->tPhyAddr);
    pPhyAddr->m_PUSer = pProtectCtrl->ucSuperPu;
    pPhyAddr->m_OffsetInSuperPage = pProtectCtrl->ucCurLunInSuperPu;
    pPhyAddr->m_BlockInPU = pProtectCtrl->usCurBlock;
    pPhyAddr->m_LPNInPage = 0;
    
    U32 ulTLun = L2_GET_TLUN(pProtectCtrl->ucSuperPu, pProtectCtrl->ucCurLunInSuperPu);
    pL3UtXorParam->ulStatusAddr = L3_UTGetStatusBuffAddrOTFB(ulTLun);
    return;
}

void L3Ut_XorPrepareSlcRead(const L3UT_XOR_PROTECT_CTRL *pSlcProtectCtrl,
                           L3UT_XOR_PARAM *pL3UtXorParam)
{
    ASSERT(pSlcProtectCtrl != NULL);
    ASSERT(pL3UtXorParam != NULL);

    L3Ut_XorPrepareCommon(pSlcProtectCtrl, pL3UtXorParam);
    pL3UtXorParam->tPhyAddr.m_PageInBlock = pSlcProtectCtrl->usCurSuperPage;;
    U32 ulTLun = L2_GET_TLUN(pSlcProtectCtrl->ucSuperPu, pSlcProtectCtrl->ucCurLunInSuperPu);
    pL3UtXorParam->aPageAddr[0] = L3_UTGetDataBuffAddr(ulTLun);
    pL3UtXorParam->ulSpareAddr = L3_UTGetSpareBuffAddr(ulTLun);
    COM_MemZero((U32 *)pL3UtXorParam->aPageAddr[0], LOGIC_PIPE_PG_SZ / sizeof(U32));
    COM_MemZero((U32 *)pL3UtXorParam->ulSpareAddr, RED_SW_SZ_DW);
    return;
}

void L3Ut_XorPrepareTlcRead(const L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl,
                           L3UT_XOR_PARAM *pL3UtXorParam)
{
    ASSERT(pTlcProtectCtrl != NULL);
    ASSERT(pL3UtXorParam != NULL);

    U32 ulCurWlIndex = HAL_FlashGetTlcPrgWL(pTlcProtectCtrl->usCurSuperPage);
    L3Ut_XorPrepareCommon(pTlcProtectCtrl, pL3UtXorParam);
    pL3UtXorParam->tPhyAddr.m_PageInBlock =
        (ulCurWlIndex - 1) * PG_PER_WL + pTlcProtectCtrl->bsCurReadIndex;
    U32 ulTLun = L2_GET_TLUN(pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu);
    pL3UtXorParam->aPageAddr[0] = L3_UTGetDataBuffAddr(ulTLun);
    pL3UtXorParam->ulSpareAddr = L3_UTGetSpareBuffAddr(ulTLun);
    COM_MemZero((U32 *)pL3UtXorParam->aPageAddr[0], LOGIC_PIPE_PG_SZ / sizeof(U32));
    COM_MemZero((U32 *)pL3UtXorParam->ulSpareAddr, RED_SW_SZ_DW);
    return;
}

void L3Ut_XorPrepareForSlcWrite(const L3UT_XOR_PROTECT_CTRL *pSlcProtectCtrl,
                                L3UT_XOR_PARAM *pL3UtXorParam)
{
    ASSERT(pSlcProtectCtrl != NULL);
    ASSERT(pL3UtXorParam != NULL);

    L3Ut_XorPrepareCommon(pSlcProtectCtrl, pL3UtXorParam);
    pL3UtXorParam->tPhyAddr.m_PageInBlock = pSlcProtectCtrl->usCurSuperPage;
    U32 ulTLun = L2_GET_TLUN(pSlcProtectCtrl->ucSuperPu, pSlcProtectCtrl->ucCurLunInSuperPu);
    pL3UtXorParam->aPageAddr[0] = L3_UTGetDataBuffAddr(ulTLun);
    pL3UtXorParam->ulSpareAddr = L3_UTGetSpareBuffAddr(ulTLun);
    COM_MemZero((U32 *)pL3UtXorParam->aPageAddr[0], LOGIC_PIPE_PG_SZ / sizeof(U32));
    COM_MemZero((U32 *)pL3UtXorParam->ulSpareAddr, RED_SW_SZ_DW);

    pL3UtXorParam->tXorParam.bsXorEn = TRUE;
    pL3UtXorParam->tXorParam.bsXorStripeId = pSlcProtectCtrl->bsXorStripeId;
    pL3UtXorParam->tXorParam.bsContainXorData = 
        (pSlcProtectCtrl->ucCurLunInSuperPu == (L3UT_XOR_LUN_PER_SUPERPU - 1)) ? TRUE : FALSE;

    // Prepare the data and redundant.
    int i = 0;
    U32 *pPageData = (U32 *)pL3UtXorParam->aPageAddr[0];
    U32 ulXorPageNumInTotal = pSlcProtectCtrl->usCurSuperPage * L3UT_XOR_PROTECT_RATIO +
                              pSlcProtectCtrl->ucCurLunInSuperPu * PLN_PER_LUN;
    for (i = 0; i < PLN_PER_LUN; ++i)
    {
        L3Ut_XorDataPrepare(pPageData, LOGIC_PG_SZ, ulXorPageNumInTotal + i);
        pPageData += (LOGIC_PG_SZ / sizeof(U32));
    }
    
    L3Ut_XorRedunPrepare((U32 *)pL3UtXorParam->ulSpareAddr, LOGIC_PG_SZ, PLN_PER_LUN);
    
    return;
}

void L3Ut_XorPrepareForTlcWrite(const L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl,
                                L3UT_XOR_PARAM *pL3UtXorParam)
{
    ASSERT(pTlcProtectCtrl != NULL);
    ASSERT(pL3UtXorParam != NULL);

    L3Ut_XorPrepareCommon(pTlcProtectCtrl, pL3UtXorParam);
    pL3UtXorParam->tPhyAddr.m_PageInBlock = pTlcProtectCtrl->usCurSuperPage;

    U32 ulTLun = L2_GET_TLUN(pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu);

    U32 ulCurWlIndex = HAL_FlashGetTlcPrgWL(pTlcProtectCtrl->usCurSuperPage);
    U32 ulCurPrgCycle = HAL_FlashGetTlcPrgCycle(pTlcProtectCtrl->usCurSuperPage);
    U32 ulPageInWl = 0;
    for (ulPageInWl = 0; ulPageInWl < DSG_BUFF_SIZE; ++ulPageInWl)
    {
        pL3UtXorParam->aPageAddr[ulPageInWl] =
            L3_UTGetDataBuffAddrFor2DTlcWrite(ulTLun, ulCurWlIndex, ulPageInWl);
    }
    pL3UtXorParam->ulSpareAddr = L3_UTGetSpareBuffAddrFor2DTlcWrite(ulTLun, ulCurWlIndex, 0);

    PRINT_DEBUG("Buffer Addresses of Super PU:%d, LUN in Super PU:%d, Block:%d, Program Index:%d is"
                ":\n",pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu,
                pTlcProtectCtrl->usCurBlock, pTlcProtectCtrl->usCurSuperPage);
    PRINT_DEBUG("Data 0:0x%x, Data 1:0x%x, Data 2:0x%x, Spare:0x%x, Status:0x%x.\n",
                pL3UtXorParam->aPageAddr[0], pL3UtXorParam->aPageAddr[1],
                pL3UtXorParam->aPageAddr[2], pL3UtXorParam->ulSpareAddr,
                pL3UtXorParam->ulStatusAddr);
    // Only need to prepare the data and redundant when it is 1st program cycle.
    if (ulCurPrgCycle == 0)
    {
        for (ulPageInWl = 0; ulPageInWl < DSG_BUFF_SIZE; ++ulPageInWl)
        {
            COM_MemZero((U32 *)pL3UtXorParam->aPageAddr[ulPageInWl], LOGIC_PIPE_PG_SZ / sizeof(U32));
        }
        COM_MemZero((U32 *)pL3UtXorParam->ulSpareAddr, RED_SW_SZ_DW * PG_PER_WL);

        pL3UtXorParam->tXorParam.bsXorEn = TRUE;
        pL3UtXorParam->tXorParam.bsXorStripeId = pTlcProtectCtrl->bsXorStripeId;
        pL3UtXorParam->tXorParam.bsContainXorData =
            (pTlcProtectCtrl->ucCurLunInSuperPu == (L3UT_XOR_LUN_PER_SUPERPU - 1)) ? TRUE : FALSE;

        int i = 0;
        int j = 0;
        U32 ulXorPageNumInTotal = ulCurWlIndex * L3UT_XOR_PROTECT_RATIO +
                                  pTlcProtectCtrl->ucCurLunInSuperPu * PLN_PER_LUN;
        for (i = 0; i < DSG_BUFF_SIZE; ++i)
        {
            U32 *pPageData = (U32 *)pL3UtXorParam->aPageAddr[i];
            for (j = 0; j < PLN_PER_LUN; ++j)
            {
                L3Ut_XorDataPrepare(pPageData, LOGIC_PG_SZ, ulXorPageNumInTotal + j);
                pPageData += (LOGIC_PG_SZ / sizeof(U32));
            }
        }

        L3Ut_XorRedunPrepare((U32 *)pL3UtXorParam->ulSpareAddr, LOGIC_PG_SZ,
                             PLN_PER_LUN * DSG_BUFF_SIZE);
    }
    
    return;
}

void L3Ut_XorParityDataCheck(U32 *pParityData, U32 ulXorPageSize, U32 ulStartXorPageNumInTotal)
{
    ASSERT(pParityData != NULL);
    ASSERT(ulStartXorPageNumInTotal < (U32)(L3UT_XOR_SUPER_PAGE_CNT * L3UT_XOR_PROTECT_RATIO));
    ASSERT((ulStartXorPageNumInTotal % L3UT_XOR_PROTECT_RATIO) == 0);
    ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0, j = 0;
    U32 ulXorPageNumInTotal = ulStartXorPageNumInTotal;
    U32 ulParityPattern = 0;

    for (i = 0; i < (U32)(L3UT_XOR_PROTECT_RATIO - 1); ++i)
    {
        U32 ulDataPattern = L3Ut_XorGetDataPattern(ulXorPageNumInTotal);
        ulParityPattern ^= ulDataPattern;
        ++ulXorPageNumInTotal;
    }
    
    for (i = 0; i < (ulXorPageSize / SEC_SIZE); ++i)
    {
        for (j = 0; j < l_ucCheckDwPerSector; ++j)
        {
            if (pParityData[i * SEC_SIZE_DW + j] != ulParityPattern)
            {
                PRINT_ERR("Got Data = 0x%x, Right Data = 0x%x\n", pParityData[i * SEC_SIZE_DW + j],
                           ulParityPattern);
                DEBUG_GET_CHAR();
            }
        }
    }

    return;
}

void L3Ut_XorRedunCheck(U32 *pPageRedun, U32 ulXorPageSize)
{
    ASSERT(pPageRedun != NULL);
    ASSERT((ulXorPageSize == (16 * KB_SIZE)) || (ulXorPageSize == (32 * KB_SIZE)) ||
        (ulXorPageSize == (48 * KB_SIZE)));
    
    U32 i = 0, j = 0;
    for (i = 0; i < (ulXorPageSize / LOGIC_PG_SZ); ++i)
    {
        for (j = 0; j < RED_SZ_DW; ++j)
        {
            if (pPageRedun[i * RED_SZ_DW + j] != 0x0b55aa12)
            {
                PRINT_ERR("Got Data = 0x%x, Right Data = 0x%x\n", pPageRedun[i * RED_SZ_DW + j],
                           0x0b55aa12);
                DEBUG_GET_CHAR();
            }
        }
    }

    return;
}

void L3Ut_XorSlcProtectCheck(const L3UT_XOR_PROTECT_CTRL *pSlcProtectCtrl)
{
    ASSERT(pSlcProtectCtrl != NULL);

    U32 ulStartXorPageNumInTotal = pSlcProtectCtrl->usCurSuperPage * L3UT_XOR_PROTECT_RATIO;
    // Check XOR result of page data.
    L3Ut_XorParityDataCheck(pSlcProtectCtrl->pXorPageParity[0], LOGIC_PG_SZ,
                            ulStartXorPageNumInTotal);
    // Check XOR result of redundant data.
    L3Ut_XorRedunCheck(pSlcProtectCtrl->pXorRedunParity[0], LOGIC_PG_SZ);
    return;
}

void L3Ut_XorTlcProtectCheck(const L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl)
{
    ASSERT(pTlcProtectCtrl != NULL);

    U32 ulCurWlIndex = HAL_FlashGetTlcPrgWL(pTlcProtectCtrl->usCurSuperPage);
    U32 ulStartXorPageNumInTotal = (ulCurWlIndex - 1) * L3UT_XOR_PROTECT_RATIO;

    U32 i = 0;
    for (i = 0; i < PG_PER_WL; ++i)
    {
        // Check XOR result of page data.
        L3Ut_XorParityDataCheck(pTlcProtectCtrl->pXorPageParity[i], LOGIC_PG_SZ,
                                ulStartXorPageNumInTotal);
        // Check XOR result of redundant data.
        L3Ut_XorRedunCheck(pTlcProtectCtrl->pXorRedunParity[i], LOGIC_PG_SZ);
    }
    
    PRINT_INFO("TLC XOR Protect: Word Line %d Data Check Success!\n", (ulCurWlIndex - 1));
    return;
}

// This function will write several SLC super pages, the maximum is page count of a block, if the
// value of "L3UT_XOR_SUPER_PAGE_CNT" is less than page count of a block, then this function only
// write "L3UT_XOR_SUPER_PAGE_CNT" SLC super pages.
BOOL L3Ut_Xor1BlockSlcProtect(L3UT_XOR_PROTECT_CTRL *pSlcProtectCtrl)
{
    ASSERT(pSlcProtectCtrl != NULL);
    ASSERT(pSlcProtectCtrl->eStatus < L3UT_XOR_STATE_ASSERT);
    
    BOOL bFinish = FALSE;
    L3UT_XOR_STATE eNextState = pSlcProtectCtrl->eStatus;

    switch (pSlcProtectCtrl->eStatus)
    {
        case L3UT_XOR_INIT:
        {
            pSlcProtectCtrl->bsXorStripeId = l_ucXorStripeId;
            ++l_ucXorStripeId;
            eNextState = L3UT_XOR_IS_NEED_ERASE_BLOCK;
            PRINT_DEBUG("TLC XOR Protect: " STR(L3UT_XOR_INIT) " ");
            PRINT_DEBUG("New Xor Stripe ID:%d.\n", pSlcProtectCtrl->bsXorStripeId);
            break;
        }
        case L3UT_XOR_IS_NEED_ERASE_BLOCK:
        {
            eNextState = (pSlcProtectCtrl->usCurSuperPage == 0) ?
                         L3UT_XOR_ERASE_BLOCK : L3UT_XOR_PROGRAM_PAGE;
            break;
        }
        case L3UT_XOR_ERASE_BLOCK:
        {
            U32 ulTLun = L2_GET_TLUN(pSlcProtectCtrl->ucSuperPu, pSlcProtectCtrl->ucCurLunInSuperPu);
            if (TRUE == L2_FCMDQNotFull(ulTLun))
            {
                L2_FtlEraseBlock(pSlcProtectCtrl->ucSuperPu, pSlcProtectCtrl->ucCurLunInSuperPu,
                                 pSlcProtectCtrl->usCurBlock, NULL, FALSE, TRUE, TRUE);//last parameter need owner double check
                eNextState = L3UT_XOR_PROGRAM_PAGE;
            }
            break;
        }
        case L3UT_XOR_PROGRAM_PAGE:
        {
            U32 ulTLun = L2_GET_TLUN(pSlcProtectCtrl->ucSuperPu, pSlcProtectCtrl->ucCurLunInSuperPu);
            if (TRUE == L2_FCMDQNotFull(ulTLun))
            {
                L3UT_XOR_PARAM tL3UtXorParam = {0};
                L3Ut_XorPrepareForSlcWrite(pSlcProtectCtrl, &tL3UtXorParam);
                L2_FtlWriteLocal(&(tL3UtXorParam.tPhyAddr), (U32 *)(tL3UtXorParam.aPageAddr[0]),
                                 (U32 *)tL3UtXorParam.ulSpareAddr, 
                                 (U8 *)tL3UtXorParam.ulStatusAddr, FALSE, TRUE,
                                 &(tL3UtXorParam.tXorParam));

                eNextState = L3UT_XOR_IS_NEED_ERASE_BLOCK;

                ++(pSlcProtectCtrl->ucCurLunInSuperPu);
                if (pSlcProtectCtrl->ucCurLunInSuperPu == L3UT_XOR_LUN_PER_SUPERPU)
                {
                    --(pSlcProtectCtrl->ucCurLunInSuperPu);
                    eNextState = L3UT_XOR_READ_PARITY;
                }
            }
            break;
        }
        // This processing belongs to data checking of XOR protection. 
        case L3UT_XOR_READ_PARITY:
        {
            U32 ulTLun = L2_GET_TLUN(pSlcProtectCtrl->ucSuperPu, pSlcProtectCtrl->ucCurLunInSuperPu);
            if (TRUE == L2_FCMDQNotFull(ulTLun))
            {
                L3UT_XOR_PARAM tL3UtXorParam = {0};
                L3Ut_XorPrepareSlcRead(pSlcProtectCtrl, &tL3UtXorParam);
                L2_FtlReadLocal((U32 *)(tL3UtXorParam.aPageAddr[0]), &(tL3UtXorParam.tPhyAddr),
                                (U8 *)tL3UtXorParam.ulStatusAddr, (U32 *)tL3UtXorParam.ulSpareAddr,
                                LPN_PER_BUF, 0, FALSE, TRUE);
                pSlcProtectCtrl->pReadCmdStatus[0] = (U8 *)tL3UtXorParam.ulStatusAddr;
                // The parity always the last plane of the LUN.
                pSlcProtectCtrl->pXorPageParity[0] = (U32 *)(tL3UtXorParam.aPageAddr[0]) +
                                                     (LOGIC_PIPE_PG_SZ - LOGIC_PG_SZ) / sizeof(U32);
                pSlcProtectCtrl->pXorRedunParity[0] = (U32 *)tL3UtXorParam.ulSpareAddr + 
                                                      (RED_SW_SZ_DW - RED_SZ_DW);
                eNextState = L3UT_XOR_WAIT_DONE;
                // Because of read command buffer request mechanism, we must do data checking
                // by blocking mode.
                pSlcProtectCtrl->bIsHighestPriority = TRUE;
            }
            break;
        }
        // This processing belongs to data checking of XOR protection.
        case L3UT_XOR_WAIT_DONE:
        {
            if (*(pSlcProtectCtrl->pReadCmdStatus[0]) != SUBSYSTEM_STATUS_PENDING)
            {
                U8 ucReadCmdStatus = *(pSlcProtectCtrl->pReadCmdStatus[0]);
                if (ucReadCmdStatus == SUBSYSTEM_STATUS_SUCCESS)
                {
                    L3Ut_XorSlcProtectCheck(pSlcProtectCtrl);

                    if (++(pSlcProtectCtrl->usCurSuperPage) == L3UT_XOR_SUPER_PAGE_CNT)
                    {
                        bFinish = TRUE;
                    }
                    else
                    {
                        pSlcProtectCtrl->ucCurLunInSuperPu = 0;
                        pSlcProtectCtrl->bIsHighestPriority = FALSE;
                    }

                    eNextState = L3UT_XOR_INIT;
                }
                else
                {
                    PRINT_ERR("L3UtXor: Parity Read Error During XOR Protection!\n");
                    DEBUG_GET_CHAR();
                }
            }
            break;
        }
        default:
        {
            PRINT_ERR("L3UtXor: Switch Case Error!\n");
            DEBUG_GET_CHAR();
        }
    }

    pSlcProtectCtrl->eStatus = eNextState;
    return bFinish;
}

BOOL L3Ut_XorTlcIsReadParityDone(const L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl)
{
    U32 i = 0;
    for (i = 0; i < PG_PER_WL; ++i)
    {
        if (*(pTlcProtectCtrl->pReadCmdStatus[i]) == SUBSYSTEM_STATUS_PENDING)
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL L3Ut_XorTlcIsAllReadParitySuccess(const L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl)
{
    U32 i = 0;
    for (i = 0; i < PG_PER_WL; ++i)
    {
        if (*(pTlcProtectCtrl->pReadCmdStatus[i]) != SUBSYSTEM_STATUS_SUCCESS)
        {
            return FALSE;
        }
    }

    return TRUE;
}

// This function will write several TLC super Word-Lines, the maximum is Word-Line count of a block,
// if the value of "L3UT_XOR_SUPER_PAGE_CNT" is less than Word-Line count of a block, then this
// function only write "L3UT_XOR_SUPER_PAGE_CNT" TLC Word-Lines.
BOOL L3Ut_Xor1BlockTlcProtect(L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl)
{
    ASSERT(pTlcProtectCtrl != NULL);
    ASSERT(pTlcProtectCtrl->eStatus < L3UT_XOR_STATE_ASSERT);

    BOOL bFinish = FALSE;
    L3UT_XOR_STATE eNextState = pTlcProtectCtrl->eStatus;

    switch (pTlcProtectCtrl->eStatus)
    {
        case L3UT_XOR_INIT:
        {
            pTlcProtectCtrl->bsXorStripeId = l_ucXorStripeId;
            ++l_ucXorStripeId;
            eNextState = L3UT_XOR_IS_NEED_ERASE_BLOCK;
            PRINT_DEBUG("TLC XOR Protect: " STR(L3UT_XOR_INIT) " ");
            PRINT_DEBUG("New Xor Stripe ID:%d.\n", pTlcProtectCtrl->bsXorStripeId);
            break;
        }
        case L3UT_XOR_IS_NEED_ERASE_BLOCK:
        {
            eNextState = (pTlcProtectCtrl->usCurSuperPage == 0) ?
                         L3UT_XOR_ERASE_BLOCK : L3UT_XOR_PROGRAM_PAGE;
            break;
        }
        case L3UT_XOR_ERASE_BLOCK:
        {
            U32 ulTLun = L2_GET_TLUN(pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu);
            if (TRUE == L2_FCMDQNotFull(ulTLun))
            {
                PRINT_DEBUG("TLC XOR Protect: " STR(L3UT_XOR_ERASE_BLOCK) " ");
                PRINT_DEBUG("Super PU:%d, LUN in Super PU:%d, Block:%d.\n",
                            pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu,
                            pTlcProtectCtrl->usCurBlock);
                L2_FtlEraseBlock(pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu,
                                 pTlcProtectCtrl->usCurBlock, NULL, FALSE, FALSE, TRUE);//last parameter need owner double check
                eNextState = L3UT_XOR_PROGRAM_PAGE;
            }
            break;
        }
        case L3UT_XOR_PROGRAM_PAGE:
        {
            U32 ulTLun = L2_GET_TLUN(pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu);
            if (TRUE == L2_FCMDQNotFull(ulTLun))
            {
                PRINT_DEBUG("TLC XOR Protect: " STR(L3UT_XOR_PROGRAM_PAGE) " ");
                PRINT_DEBUG("Super PU:%d, LUN in Super PU:%d, Block:%d, Program Index:%d.\n",
                            pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu,
                            pTlcProtectCtrl->usCurBlock, pTlcProtectCtrl->usCurSuperPage);

                L3UT_XOR_PARAM tL3UtXorParam = {0};
                L3Ut_XorPrepareForTlcWrite(pTlcProtectCtrl, &tL3UtXorParam);
                L2_FtlTLCExternalWriteLocal(
                    &(tL3UtXorParam.tPhyAddr), (U32 *)tL3UtXorParam.aPageAddr,
                    (U32 *)tL3UtXorParam.ulSpareAddr, (U8 *)tL3UtXorParam.ulStatusAddr,
                    &(tL3UtXorParam.tXorParam));

                eNextState = L3UT_XOR_IS_NEED_ERASE_BLOCK;

                ++(pTlcProtectCtrl->ucCurLunInSuperPu);
                if (pTlcProtectCtrl->ucCurLunInSuperPu == L3UT_XOR_LUN_PER_SUPERPU)
                {
                    U32 ulCurWlIndex = HAL_FlashGetTlcPrgWL(pTlcProtectCtrl->usCurSuperPage);
                    U32 ulCurPrgCycle = HAL_FlashGetTlcPrgCycle(pTlcProtectCtrl->usCurSuperPage);
                    if ((ulCurPrgCycle == 2) && (ulCurWlIndex > 0))
                    {
                        --(pTlcProtectCtrl->ucCurLunInSuperPu);
                        eNextState = L3UT_XOR_READ_PARITY;
                        // Because of read command buffer request mechanism, we must do data checking
                        // by blocking mode.
                        pTlcProtectCtrl->bIsHighestPriority = TRUE;
                    }
                    else
                    {
                        ++(pTlcProtectCtrl->usCurSuperPage);
                        pTlcProtectCtrl->ucCurLunInSuperPu = 0;
                        eNextState = 
                            (0 == HAL_FlashGetTlcPrgCycle(pTlcProtectCtrl->usCurSuperPage)) ?
                            L3UT_XOR_INIT : eNextState;
                    }
                }
            }
            break;
        }
        // This processing belongs to data checking of XOR protection.
        case L3UT_XOR_READ_PARITY:
        {
            U32 ulTLun = L2_GET_TLUN(pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu);
            if (TRUE == L2_FCMDQNotFull(ulTLun))
            {
                L3UT_XOR_PARAM tL3UtXorParam = {0};
                L3Ut_XorPrepareTlcRead(pTlcProtectCtrl, &tL3UtXorParam);
                L2_FtlReadLocal((U32 *)(tL3UtXorParam.aPageAddr[0]), &(tL3UtXorParam.tPhyAddr),
                                (U8 *)tL3UtXorParam.ulStatusAddr, (U32 *)tL3UtXorParam.ulSpareAddr,
                                LPN_PER_BUF, 0, FALSE, FALSE);
                U32 ulCurReadIndex = pTlcProtectCtrl->bsCurReadIndex;
                pTlcProtectCtrl->pReadCmdStatus[ulCurReadIndex] = (U8 *)tL3UtXorParam.ulStatusAddr;
                // The parity always the last plane of the LUN.
                pTlcProtectCtrl->pXorPageParity[ulCurReadIndex] = 
                    (U32 *)tL3UtXorParam.aPageAddr[0] +
                    (LOGIC_PIPE_PG_SZ - LOGIC_PG_SZ) / sizeof(U32);
                pTlcProtectCtrl->pXorRedunParity[ulCurReadIndex] = 
                    (U32 *)tL3UtXorParam.ulSpareAddr + (RED_SW_SZ_DW - RED_SZ_DW);

                PRINT_DEBUG("TLC XOR Protect: " STR(L3UT_XOR_READ_PARITY) " ");
                PRINT_DEBUG("Super PU:%d, LUN in Super PU:%d, Block:%d, Page:%d, Read Index:%d.\n",
                            pTlcProtectCtrl->ucSuperPu, pTlcProtectCtrl->ucCurLunInSuperPu,
                            pTlcProtectCtrl->usCurBlock, tL3UtXorParam.tPhyAddr.m_PageInBlock,
                            ulCurReadIndex);

                if (++ulCurReadIndex == PG_PER_WL)
                {
                    pTlcProtectCtrl->bsCurReadIndex = 0;
                    eNextState = L3UT_XOR_WAIT_DONE;
                }
                else
                {
                    pTlcProtectCtrl->bsCurReadIndex = ulCurReadIndex;
                }
            }
            break;
        }
        // This processing belongs to data checking of XOR protection.
        case L3UT_XOR_WAIT_DONE:
        {
            if (TRUE == L3Ut_XorTlcIsReadParityDone(pTlcProtectCtrl))
            {
                if (TRUE == L3Ut_XorTlcIsAllReadParitySuccess(pTlcProtectCtrl))
                {
                    L3Ut_XorTlcProtectCheck(pTlcProtectCtrl);

                    U16 usCurSuperPage = pTlcProtectCtrl->usCurSuperPage;

                    if (HAL_FlashGetTlcPrgWL(usCurSuperPage) == L3UT_XOR_SUPER_PAGE_CNT)
                    {
                        bFinish = TRUE;
                    }
                    else
                    {
                        pTlcProtectCtrl->usCurSuperPage = ++usCurSuperPage;
                        pTlcProtectCtrl->ucCurLunInSuperPu = 0;
                        pTlcProtectCtrl->bsCurReadIndex = 0;
                        pTlcProtectCtrl->bIsHighestPriority = FALSE;

                        eNextState = 
                            (0 == HAL_FlashGetTlcPrgCycle(pTlcProtectCtrl->usCurSuperPage)) ?
                            L3UT_XOR_INIT : L3UT_XOR_IS_NEED_ERASE_BLOCK;
                    }
                }
                else
                {
                    PRINT_ERR("L3UtXor: Parity Read Error During XOR TLC Protection!\n");
                    DEBUG_GET_CHAR();
                }
            }
            break;
        }
        default:
        {
            PRINT_ERR("L3UtXor: Switch Case Error!\n");
            DEBUG_GET_CHAR();
        }
    }

    pTlcProtectCtrl->eStatus = eNextState;
    return bFinish;
}

BOOL L3Ut_XorRecover(L3UT_XOR_RECOVER_CTRL *pRecoverCtrl)
{
    ASSERT(pRecoverCtrl != NULL);
    ASSERT(pRecoverCtrl->eStatus < L3UT_XOR_STATE_ASSERT);

    BOOL bRecoverFinish = FALSE;

    switch (pRecoverCtrl->eStatus)
    {
        case L3UT_XOR_INIT:
        {
            bRecoverFinish = TRUE;
            break;
        }
        case L3UT_XOR_READ_PAGE:
        {
            break;
        }
        case L3UT_XOR_WAIT_DONE:
        {
            break;
        }
        default:
        {
            PRINT_ERR("L3UtXor: Switch Case Error!\n");
            DEBUG_GET_CHAR();
        }
    }

    return bRecoverFinish;
}

void L3Ut_XorSchedule(L3UT_XOR_TEST_TASK *pTestTask)
{
    ASSERT(pTestTask != NULL);
    ASSERT(pTestTask->pPatternInfo != NULL);
    L3UT_XOR_PATTERN ePatternId = pTestTask->pPatternInfo->ePatternId;
    ASSERT((ePatternId >= 0) && (ePatternId < L3UT_XOR_PATTERN_ASSERT));

    switch (ePatternId)
    {
        case L3UT_XOR_SLC_PROTECT:
        {
            L3UT_XOR_PROTECT_CTRL *pSlcProtectCtrl = &(pTestTask->tProtectCtrl);

            BOOL bFinishCurBlock = L3Ut_Xor1BlockSlcProtect(pSlcProtectCtrl);
            if (bFinishCurBlock == TRUE)
            {
                PRINT_INFO("SLC XOR Protect Passed!\n");
                L3Ut_XorInit1BlockProtectCtrl(pTestTask);
                L3Ut_XorTestTaskStateUpdate(pTestTask);
            }
            break;
        }
        case L3UT_XOR_TLC_PROTECT:
        {
            L3UT_XOR_PROTECT_CTRL *pTlcProtectCtrl = &(pTestTask->tProtectCtrl);

            BOOL bFinishCurBlock = L3Ut_Xor1BlockTlcProtect(pTlcProtectCtrl);
            if (bFinishCurBlock == TRUE)
            {
                PRINT_INFO("TLC XOR Protect Passed!\n");
                L3Ut_XorInit1BlockProtectCtrl(pTestTask);
                L3Ut_XorTestTaskStateUpdate(pTestTask);
            }
            break;
        }
        case L3UT_XOR_RECOVER:
        {
            L3UT_XOR_RECOVER_CTRL *pRecoverCtrl = &(pTestTask->tRecoverCtrl);

            BOOL bRecoverFinish = L3Ut_XorRecover(pRecoverCtrl);
            if (bRecoverFinish == TRUE)
            {
                PRINT_INFO("XOR Recover Passed!\n");
                L3Ut_XorClearRecoverCtrl(pRecoverCtrl);
                L3Ut_XorTestTaskStateUpdate(pTestTask);
            }
            break;
        }
        default:
        {
            PRINT_ERR("L3UtXor: Switch Case Error!\n");
            DEBUG_GET_CHAR();
        }
    }

    return;
}

BOOL L3Ut_XorIsHighestPriority(const L3UT_XOR_TEST_TASK *pCurTestTask)
{
    ASSERT(pCurTestTask != NULL);
    if (pCurTestTask->pPatternInfo->ePatternId == L3UT_XOR_RECOVER)
    {
        return pCurTestTask->tRecoverCtrl.bIsHighestPriority;
    }
    else
    {
        return pCurTestTask->tProtectCtrl.bIsHighestPriority;
    }
}

BOOL L3Ut_XorSelect1Task(const L3UT_XOR_TEST_TASK *pTestTask, U32 *pTestTaskIndex)
{
    ASSERT(pTestTask != NULL);
    ASSERT(pTestTaskIndex != NULL);

    static S32 slCurTestTask = -1;

    if ((slCurTestTask != -1) && (TRUE == L3Ut_XorIsHighestPriority(&(pTestTask[slCurTestTask]))))
    {
        *pTestTaskIndex = slCurTestTask;
        return TRUE;
    }

    U32 i = 0;
    for (i = 0; i < L3UT_XOR_TEST_TASK_CNT; ++i)
    {
        slCurTestTask = (slCurTestTask + 1) % L3UT_XOR_TEST_TASK_CNT;
        if (FALSE == pTestTask[slCurTestTask].bIsFinished)
        {
            *pTestTaskIndex = slCurTestTask;
            return TRUE;
        }
    }

    return FALSE;
}

void L3Ut_XorCheckConfig(void)
{
    if ((L3UT_XOR_LUN_PER_SUPERPU <= 1) || (L3UT_XOR_LUN_PER_SUPERPU > SUBSYSTEM_LUN_NUM))
    {
        PRINT_ERR("L3Ut_XorCheckConfig: LUN Per Super PU Error!\n");
        DEBUG_GET_CHAR();
    }
    
    if ((L3UT_XOR_SUPER_PAGE_CNT == 0) || (L3UT_XOR_SUPER_PAGE_CNT > PG_PER_BLK))
    {
        PRINT_ERR("L3Ut_XorCheckConfig: The Test Super Page Count of Each Block Error!\n");
        DEBUG_GET_CHAR();
    }

    if ((l_bIsInfiniteLoop != TRUE) && (l_bIsInfiniteLoop != FALSE))
    {
        PRINT_ERR("L3Ut_XorCheckConfig: Infinite Loop Enable Can Only Be TRUE or FALSE!\n");
        DEBUG_GET_CHAR();
    }

#ifdef SIM
    if (l_ucCheckDwPerSector != 2)
    {
        PRINT_ERR("L3Ut_XorCheckConfig: Win-Sim Can Only Check 2 DW!\n");
        DEBUG_GET_CHAR();
    }
#else
    if ((l_ucCheckDwPerSector == 0) || (l_ucCheckDwPerSector > SEC_SIZE_DW))
    {
        PRINT_ERR("L3Ut_XorCheckConfig: The Check DW Count of Each Sector Error!\n");
        DEBUG_GET_CHAR();
    }
#endif

    U32 i = 0;
    for (i = 0; i < L3UT_XOR_TEST_TASK_CNT; ++i)
    {
        const L3UT_XOR_PATTERN_INFO * pPatternInfo = &(l_aXorTestTaskList[i]);
        if (pPatternInfo->ePatternId >= L3UT_XOR_PATTERN_ASSERT)
        {
            PRINT_ERR("L3Ut_XorCheckConfig: Task Pattern ID Error!\n");
            DEBUG_GET_CHAR();
        }
        
        if ((pPatternInfo->usStartBlock > pPatternInfo->usEndBlock) ||
            (pPatternInfo->usEndBlock >= BLK_PER_PLN))
        {
            PRINT_ERR("L3Ut_XorCheckConfig: Task Block Range Error!\n");
            DEBUG_GET_CHAR();
        }

        if (pPatternInfo->ucSuperPu >= (SUBSYSTEM_LUN_NUM / L3UT_XOR_LUN_PER_SUPERPU))
        {
            PRINT_ERR("L3Ut_XorCheckConfig: Task Super PU Error!\n");
            DEBUG_GET_CHAR();
        }
    }

    for (i = 0; i < L3UT_XOR_TEST_TASK_CNT; ++i)
    {
        U32 j = 0;
        for (j = i + 1; j < L3UT_XOR_TEST_TASK_CNT; ++j)
        {
            if (((l_aXorTestTaskList[j].usStartBlock < l_aXorTestTaskList[i].usStartBlock) &&
                (l_aXorTestTaskList[i].usStartBlock < l_aXorTestTaskList[j].usEndBlock)) ||
                ((l_aXorTestTaskList[i].usStartBlock < l_aXorTestTaskList[j].usStartBlock) &&
                (l_aXorTestTaskList[j].usStartBlock < l_aXorTestTaskList[i].usEndBlock)))
            {
                PRINT_ERR("L3Ut_XorCheckConfig: Tasks Block Range Overlap Error!\n");
                DEBUG_GET_CHAR();
            }
        }
    }
    return;
}

BOOL L3_XorTest(void)
{
    L3Ut_XorCheckConfig();
    L3UT_XOR_TEST_TASK aTestTask[L3UT_XOR_TEST_TASK_CNT];

    L3_UTBasicInit();
    L3Ut_XorTestTaskInit(aTestTask);

    U32 ulTestTaskIndex = 0;
        
    while (TRUE == L3Ut_XorSelect1Task(aTestTask, &ulTestTaskIndex))
    {  
        L3Ut_XorSchedule(&(aTestTask[ulTestTaskIndex]));
    }

    PRINT_INFO("\nL3UtXor: L3 XOR unit test End!\n\n");
    while (TRUE){}
    return TRUE;
}
