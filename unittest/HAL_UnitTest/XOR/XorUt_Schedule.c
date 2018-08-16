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
#include "XorUt_Nfc_Interface.h"
#include "HAL_XOR.h"
#include "TEST_NfcFuncBasic.h"


// XorUt represents XOR unit test.

typedef struct _XORUT_TEST_TASK
{
    XORUT_PATTERN eCurTestPatternId;
    XORUT_BASIC_TEST_PATTERN_CTRL *pBasicTestPatternCtrl;
    U8  ucPassedTestPatternCnt;
    BOOL bIsFinished;
    XOR_NFC_PROTECT_CTRL tNfcProtectCtrl;
    XOR_TLC_NFC_PROTECT_CTRL tTlcNfcProtectCtrl;
    XOR_NFC_PROTECT_CTRL t3dMlcNfcProtectCtrl;
    XOR_NFC_RECOVER_CTRL tNfcRecoverCtrl;
    XOR_BYPASS_NFC_CTRL tBypassProtectCtrl;
    XOR_BYPASS_NFC_CTRL tBypassRecoverCtrl;
    XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL tBypassRecoverWithNfcReadCtrl;
    XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL tBypassRecoverWithTlcNfcReadCtrl;
    XOR_LOAD_STORE_CTRL tLoadFromDramCtrl;
    XOR_LOAD_STORE_CTRL tStoreToDramCtrl;
    XOR_LOAD_STORE_CTRL tLoadThenStoreCtrl;
}XORUT_TEST_TASK;


LOCAL U8 l_ucFinishedTestTaskCnt = 0;


void XorUt_TestTaskInit(XORUT_TEST_TASK aTestTask[])
{
    U32 i = 0;

    for (i = 0; i < XORUT_TEST_TASK_CNT; ++i)
    {
        if (g_IsFixedMixPattern == TRUE)
        {
            aTestTask[i].pBasicTestPatternCtrl = &g_aBasicTestPatternCtrl[i];
        } 
        else
        {
            aTestTask[i].pBasicTestPatternCtrl = &g_tBasicTestPatternCtrl;
        }
        aTestTask[i].eCurTestPatternId = aTestTask[i].pBasicTestPatternCtrl->ucPatternId[0];
        aTestTask[i].ucPassedTestPatternCnt = 0;
        aTestTask[i].bIsFinished = FALSE;

        XorUt_ClearNfcProtectCtrl(&(aTestTask[i].tNfcProtectCtrl));
        // XOR pages of any two test task can't have overlapping, if not, will occur rewrite issue.
        // Because test task is running simultaneously.
        aTestTask[i].tNfcProtectCtrl.usStartXorPageNumInTotal = i * g_usXorProtectRatio;
        aTestTask[i].tNfcProtectCtrl.pXorPageLocationTable = g_aXorPageLocationTable[i];

        XorUt_ClearTlcNfcProtectCtrl(&(aTestTask[i].tTlcNfcProtectCtrl));
        aTestTask[i].tTlcNfcProtectCtrl.usStartXorPageNumInTotal = i * g_usXorProtectRatio;
        aTestTask[i].tTlcNfcProtectCtrl.pXorPageLocationTable = g_aXorPageLocationTable[i];

        XorUt_ClearBypassRecoverWithNfcReadCtrl(&(aTestTask[i].tBypassRecoverWithTlcNfcReadCtrl));
        aTestTask[i].tBypassRecoverWithTlcNfcReadCtrl.bIsTlcRecover = TRUE;
        aTestTask[i].tBypassRecoverWithTlcNfcReadCtrl.usBrokenXorPageNumInTotal =
            aTestTask[i].tTlcNfcProtectCtrl.usStartXorPageNumInTotal + (1 % g_usXorProtectRatio);
        aTestTask[i].tBypassRecoverWithTlcNfcReadCtrl.pXorPageLocationTable = g_aXorPageLocationTable[i];
        aTestTask[i].tBypassRecoverWithTlcNfcReadCtrl.ucPageType = 2;

        XorUt_ClearNfcProtectCtrl(&(aTestTask[i].t3dMlcNfcProtectCtrl));
        aTestTask[i].t3dMlcNfcProtectCtrl.usStartXorPageNumInTotal = i * g_usXorProtectRatio;
        aTestTask[i].t3dMlcNfcProtectCtrl.ulXorPageSize = 32 * KB_SIZE;
        aTestTask[i].t3dMlcNfcProtectCtrl.pXorPageLocationTable = g_aXorPageLocationTable[i];

        XorUt_ClearNfcRecoverCtrl(&(aTestTask[i].tNfcRecoverCtrl));
        aTestTask[i].tNfcRecoverCtrl.usBrokenXorPageNumInTotal = 
            aTestTask[i].tNfcProtectCtrl.usStartXorPageNumInTotal + (1 % g_usXorProtectRatio);
        aTestTask[i].tNfcRecoverCtrl.pXorPageLocationTable = g_aXorPageLocationTable[i];

        XorUt_ClearBypassNfcCtrl(&(aTestTask[i].tBypassProtectCtrl));
        aTestTask[i].tBypassProtectCtrl.eTarget = XOR_PROTECT;
        aTestTask[i].tBypassProtectCtrl.usStartXorPageNumInTotal = i * g_usXorProtectRatio;

        XorUt_ClearBypassNfcCtrl(&(aTestTask[i].tBypassRecoverCtrl));
        aTestTask[i].tBypassRecoverCtrl.eTarget = XOR_RECOVER;
        aTestTask[i].tBypassRecoverCtrl.usBrokenXorPageNumInTotal = 
            aTestTask[i].tBypassProtectCtrl.usStartXorPageNumInTotal + (1 % g_usXorProtectRatio);

        XorUt_ClearBypassRecoverWithNfcReadCtrl(&(aTestTask[i].tBypassRecoverWithNfcReadCtrl));
        aTestTask[i].tBypassRecoverWithNfcReadCtrl.usBrokenXorPageNumInTotal =
            aTestTask[i].tNfcProtectCtrl.usStartXorPageNumInTotal + (1 % g_usXorProtectRatio);
        aTestTask[i].tBypassRecoverWithNfcReadCtrl.pXorPageLocationTable = g_aXorPageLocationTable[i];

        XorUt_ClearLoadStoreCtrl(&(aTestTask[i].tLoadFromDramCtrl));
        aTestTask[i].tLoadFromDramCtrl.eTarget = XOR_LOAD_FROM_DRAM;
        aTestTask[i].tLoadFromDramCtrl.usXorPageNumInTotal = i * g_usXorProtectRatio;

        XorUt_ClearLoadStoreCtrl(&(aTestTask[i].tStoreToDramCtrl));
        aTestTask[i].tStoreToDramCtrl.eTarget = XOR_STORE_TO_DRAM;
        aTestTask[i].tStoreToDramCtrl.usXorPageNumInTotal =
            aTestTask[i].tLoadFromDramCtrl.usXorPageNumInTotal + (g_usXorProtectRatio - 1);
        XorUt_ClearLoadStoreCtrl(&(aTestTask[i].tLoadThenStoreCtrl));
        aTestTask[i].tLoadThenStoreCtrl.eTarget = XOR_LOAD_FROM_DRAM;
        aTestTask[i].tLoadThenStoreCtrl.usXorPageNumInTotal =
            aTestTask[i].tBypassProtectCtrl.usStartXorPageNumInTotal + (1 % g_usXorProtectRatio); 
    }

    return;
}

void XorUt_TestTaskStateUpdate(XORUT_TEST_TASK *pTestTask)
{
    XORUT_ASSERT(pTestTask != NULL);
    XORUT_ASSERT((pTestTask->eCurTestPatternId >= 0) && (pTestTask->eCurTestPatternId < XORUT_PATTERN_ASSERT));
    XORUT_ASSERT(pTestTask->pBasicTestPatternCtrl != NULL);
    XORUT_ASSERT(pTestTask->ucPassedTestPatternCnt < pTestTask->pBasicTestPatternCtrl->ucPatternCount);

    XorUt_Log(XOR_UT_RECORD_PASSED_TEST_PATTERN_ID, pTestTask->eCurTestPatternId);

    ++(pTestTask->ucPassedTestPatternCnt);
    if (pTestTask->ucPassedTestPatternCnt == pTestTask->pBasicTestPatternCtrl->ucPatternCount)
    {
        pTestTask->bIsFinished = TRUE;
        ++l_ucFinishedTestTaskCnt;
    }
    else
    {
        pTestTask->eCurTestPatternId = pTestTask->pBasicTestPatternCtrl->ucPatternId[pTestTask->ucPassedTestPatternCnt];
    }

    return;
}

void XorUt_BasicTest(XORUT_TEST_TASK *pTestTask)
{
    XORUT_ASSERT(pTestTask != NULL);
    XORUT_ASSERT((pTestTask->eCurTestPatternId >= 0) && (pTestTask->eCurTestPatternId < XORUT_PATTERN_ASSERT));
    XORUT_ASSERT(pTestTask->pBasicTestPatternCtrl != NULL);
    XORUT_ASSERT(pTestTask->ucPassedTestPatternCnt < pTestTask->pBasicTestPatternCtrl->ucPatternCount);
    
    XorUt_Log(XOR_UT_RECORD_CUR_TEST_PATTERN_ID, pTestTask->eCurTestPatternId);

    switch (pTestTask->eCurTestPatternId)
    {
        case XORUT_NFC_PROTECT:
        {
            XOR_NFC_PROTECT_CTRL *pNfcProtectCtrl = &(pTestTask->tNfcProtectCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pNfcProtectCtrl->ucXoreId);

            BOOL bXorNfcProtectFinish = XorUt_NfcProtect(pNfcProtectCtrl);
            if (bXorNfcProtectFinish == TRUE)
            {
                XorUt_NfcProtectCheckData(pNfcProtectCtrl->ucXoreId, pNfcProtectCtrl->ulXorPageSize,
                    pNfcProtectCtrl->usStartXorPageNumInTotal);
                DBG_Printf("XOR engine ID: %d XorUt_NfcProtect Pass!\n", pNfcProtectCtrl->ucXoreId);

                XorUt_ClearNfcProtectCtrl(pNfcProtectCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }

            break;
        }
#ifdef FLASH_TLC
        case XORUT_TLC_NFC_PROTECT:
        {
            XOR_TLC_NFC_PROTECT_CTRL *pTlcNfcProtectCtrl = &(pTestTask->tTlcNfcProtectCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pTlcNfcProtectCtrl->ucXoreId);

            BOOL bXorTlcNfcProtectFinish = XorUt_NfcProtectTlc(pTlcNfcProtectCtrl);
            if (bXorTlcNfcProtectFinish == TRUE)
            {
                //XorUt_NfcProtectCheckData(pNfcProtectCtrl->ucXoreId, pNfcProtectCtrl->usStartXorPageNumInTotal);
                HAL_ReleaseXore(pTlcNfcProtectCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_NfcProtectTlc Pass!\n", pTlcNfcProtectCtrl->ucXoreId);

                XorUt_ClearTlcNfcProtectCtrl(pTlcNfcProtectCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }

            break;
            }
#endif
#ifdef FLASH_3D_MLC
        case XORUT_3DMLC_NFC_PROTECT:
        {
            XOR_NFC_PROTECT_CTRL *p3dMlcNfcProtectCtrl = &(pTestTask->t3dMlcNfcProtectCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, p3dMlcNfcProtectCtrl->ucXoreId);

            BOOL b3dMlcXorNfcProtectFinish = XorUt_NfcProtect3dMlc(p3dMlcNfcProtectCtrl);
            if (b3dMlcXorNfcProtectFinish == TRUE)
            {
                XorUt_NfcProtectCheckData(p3dMlcNfcProtectCtrl->ucXoreId, p3dMlcNfcProtectCtrl->ulXorPageSize,
                    p3dMlcNfcProtectCtrl->usStartXorPageNumInTotal);
                DBG_Printf("XOR engine ID: %d XorUt_NfcProtect3dMlc Pass!\n", p3dMlcNfcProtectCtrl->ucXoreId);

                XorUt_ClearNfcProtectCtrl(p3dMlcNfcProtectCtrl);
                p3dMlcNfcProtectCtrl->ulXorPageSize = 32 * KB_SIZE;
                XorUt_TestTaskStateUpdate(pTestTask);
            }

            break;
        }
#endif
        case XORUT_NFC_RECOVER:
        {
            XOR_NFC_RECOVER_CTRL *pNfcRecoverCtrl = &(pTestTask->tNfcRecoverCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pNfcRecoverCtrl->ucXoreId);

            BOOL bXorNfcRecoverFinish = XorUt_NfcRecover(pNfcRecoverCtrl);
            if (bXorNfcRecoverFinish == TRUE)
            {
                XorUt_NfcRecoverCheckData(pNfcRecoverCtrl->ucXoreId, pNfcRecoverCtrl->ulXorPageSize,
                    pNfcRecoverCtrl->usBrokenXorPageNumInTotal);
                HAL_ReleaseXore(pNfcRecoverCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_NfcRecover Pass!\n", pNfcRecoverCtrl->ucXoreId);

                XorUt_ClearNfcRecoverCtrl(pNfcRecoverCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }

            break;
        }
        case XORUT_BPS_PROTECT:
        {
            XOR_BYPASS_NFC_CTRL *pBypassProtectCtrl = &(pTestTask->tBypassProtectCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pBypassProtectCtrl->ucXoreId);

            BOOL bBypassProtectFinish = XorUt_BypassNfc(pBypassProtectCtrl);
            if (bBypassProtectFinish == TRUE)
            {
                XorUt_BypassProtectCheckData(pBypassProtectCtrl->ucXoreId, pBypassProtectCtrl->ulXorPageSize,
                    pBypassProtectCtrl->usStartXorPageNumInTotal);
                HAL_ReleaseXore(pBypassProtectCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_BypassNfc Protect Pass!\n", pBypassProtectCtrl->ucXoreId);

                XorUt_ClearBypassNfcCtrl(pBypassProtectCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }

            break;
        }
        case XORUT_BPS_RECOVER:
        {
            XOR_BYPASS_NFC_CTRL *pBypassRecoverCtrl = &(pTestTask->tBypassRecoverCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pBypassRecoverCtrl->ucXoreId);

            BOOL bBypassRecoverFinish = XorUt_BypassNfc(pBypassRecoverCtrl);
            if (bBypassRecoverFinish == TRUE)
            {
                XorUt_BypassRecoverCheckData(pBypassRecoverCtrl->ucXoreId, pBypassRecoverCtrl->ulXorPageSize,
                    pBypassRecoverCtrl->usBrokenXorPageNumInTotal);
                HAL_ReleaseXore(pBypassRecoverCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_BypassNfc Recover Pass!\n", pBypassRecoverCtrl->ucXoreId);

                XorUt_ClearBypassNfcCtrl(pBypassRecoverCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }
          
            break;
        }
        case XORUT_BPS_RECOVER_WITH_NFC_READ:
        {
            XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL *pBypassRecoverWithNfcReadCtrl = &(pTestTask->tBypassRecoverWithNfcReadCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pBypassRecoverWithNfcReadCtrl->ucXoreId);
            XORUT_ASSERT(pBypassRecoverWithNfcReadCtrl->bIsTlcRecover == FALSE);

            BOOL bBypassRecoverWithNfcReadFinish = XorUt_BypassRecoverWithNfcRead(pBypassRecoverWithNfcReadCtrl);
            if (bBypassRecoverWithNfcReadFinish == TRUE)
            {
                XorUt_BypassRecoverCheckData(pBypassRecoverWithNfcReadCtrl->ucXoreId, 
                    pBypassRecoverWithNfcReadCtrl->ulXorPageSize,
                    pBypassRecoverWithNfcReadCtrl->usBrokenXorPageNumInTotal);
                HAL_ReleaseXore(pBypassRecoverWithNfcReadCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_BypassRecoverWithNfcRead Recover Pass!\n",
                    pBypassRecoverWithNfcReadCtrl->ucXoreId);

                XorUt_ClearBypassRecoverWithNfcReadCtrl(pBypassRecoverWithNfcReadCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }

            break;
        }
        case XORUT_BPS_RECOVER_WITH_TLC_NFC_READ:
        {
            XOR_BYPASS_RECOVER_WITH_NFC_READ_CTRL *pBypassRecoverWithTlcNfcReadCtrl = &(pTestTask->tBypassRecoverWithTlcNfcReadCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pBypassRecoverWithTlcNfcReadCtrl->ucXoreId);
            XORUT_ASSERT(pBypassRecoverWithTlcNfcReadCtrl->bIsTlcRecover == TRUE);

            BOOL bBypassRecoverWithTlcNfcReadFinish = XorUt_BypassRecoverWithNfcRead(pBypassRecoverWithTlcNfcReadCtrl);
            if (bBypassRecoverWithTlcNfcReadFinish == TRUE)
            {
                XorUt_BypassRecoverCheckData(pBypassRecoverWithTlcNfcReadCtrl->ucXoreId,
                    pBypassRecoverWithTlcNfcReadCtrl->ulXorPageSize,
                    pBypassRecoverWithTlcNfcReadCtrl->usBrokenXorPageNumInTotal);
                HAL_ReleaseXore(pBypassRecoverWithTlcNfcReadCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_BypassRecoverWithTlcNfcRead Recover Pass!\n",
                    pBypassRecoverWithTlcNfcReadCtrl->ucXoreId);

                XorUt_ClearBypassRecoverWithNfcReadCtrl(pBypassRecoverWithTlcNfcReadCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }
            break;
        }
        case XORUT_LOAD_FROM_DRAM:
        {
            XOR_LOAD_STORE_CTRL *pLoadFromDramCtrl = &(pTestTask->tLoadFromDramCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pLoadFromDramCtrl->ucXoreId);

            BOOL bLoadFromDramFinish = XorUt_LoadStore(pLoadFromDramCtrl);
            if (bLoadFromDramFinish == TRUE)
            {
                XorUt_LoadFromDramCheckData(pLoadFromDramCtrl->ucXoreId, pLoadFromDramCtrl->ulXorPageSize,
                    pLoadFromDramCtrl->usXorPageNumInTotal);
                HAL_ReleaseXore(pLoadFromDramCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_LoadStore Load From Dram Pass!\n", pLoadFromDramCtrl->ucXoreId);

                XorUt_ClearLoadStoreCtrl(pLoadFromDramCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }
   
            break;
        }
        case XORUT_STORE_TO_DRAM:
        {
            XOR_LOAD_STORE_CTRL *pStoreToDramCtrl = &(pTestTask->tStoreToDramCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pStoreToDramCtrl->ucXoreId);

            BOOL bStoreToDramFinish = XorUt_LoadStore(pStoreToDramCtrl);
            if (bStoreToDramFinish == TRUE)
            {
                XorUt_StoreToDramCheckData(pStoreToDramCtrl->ucXoreId, pStoreToDramCtrl->ulXorPageSize,
                    pStoreToDramCtrl->usXorPageNumInTotal);
                HAL_ReleaseXore(pStoreToDramCtrl->ucXoreId);
                DBG_Printf("XOR engine ID: %d XorUt_LoadStore Store To Dram Pass!\n", pStoreToDramCtrl->ucXoreId);

                XorUt_ClearLoadStoreCtrl(pStoreToDramCtrl);
                XorUt_TestTaskStateUpdate(pTestTask);
            }
   
            break;
        }
        case XORUT_LOAD_THEN_STORE:
        {
            XOR_LOAD_STORE_CTRL *pLoadThenStoreCtrl = &(pTestTask->tLoadThenStoreCtrl);
            XorUt_Log(XOR_UT_RECORD_XORE_ID, pLoadThenStoreCtrl->ucXoreId);

            BOOL bLoadThenStoreFinish = XorUt_LoadStore(pLoadThenStoreCtrl);
            if (bLoadThenStoreFinish == TRUE)
            {
                if (pLoadThenStoreCtrl->eTarget == XOR_LOAD_FROM_DRAM)
                {
                    XorUt_LoadFromDramCheckData(pLoadThenStoreCtrl->ucXoreId, pLoadThenStoreCtrl->ulXorPageSize,
                        pLoadThenStoreCtrl->usXorPageNumInTotal);

                    XorUt_LoadStorePageClean(pLoadThenStoreCtrl->ucXoreId, pLoadThenStoreCtrl->ulXorPageSize);
                    pLoadThenStoreCtrl->eTarget = XOR_STORE_TO_DRAM;
                    pLoadThenStoreCtrl->ucStatus = XOR_UT_LOAD_STORE;
                } 
                else
                {
                    XorUt_StoreToDramCheckData(pLoadThenStoreCtrl->ucXoreId, pLoadThenStoreCtrl->ulXorPageSize,
                        pLoadThenStoreCtrl->usXorPageNumInTotal);
                    HAL_ReleaseXore(pLoadThenStoreCtrl->ucXoreId);
                    DBG_Printf("XOR engine ID: %d XorUt_LoadStore Load Then Store Pass!\n", pLoadThenStoreCtrl->ucXoreId);

                    XorUt_ClearLoadStoreCtrl(pLoadThenStoreCtrl);
                    XorUt_TestTaskStateUpdate(pTestTask);
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

    return;
}

BOOL UT_XorMain(void)
{
    //BL_NfcInit_Substitute();

    // Enable XOR then we must use the ONTF bit in DSG.
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *)&rNfcPgCfg;
    pNfcPgCfg->bsOntfMode = TRUE;

    pNfcPgCfg->bsScrBps = FALSE;

    // Change redundant base address to meet XOR unit test requirement.
    volatile NF_RED_DRAM_BASE_REG *pRedBaseAddr = (volatile NF_RED_DRAM_BASE_REG *)&rNfcRedDramBase;
    pRedBaseAddr->bsRedDramBaseAddr = (XorUt_GetRedunBaseAddr() - DRAM_START_ADDRESS) >> 3;
    rNfcXorDecFifoCfg = 0x00012000;
    rNfcNfdmaCfg |= (1 << 6);
#if defined(FLASH_TLC)   
    if (g_bIsSinglePlaneOperate == FALSE)
    {
        pNfcPgCfg->bsTlcParityPogMode = TRUE;
    }
#endif

    HAL_XorInit(FALSE, TRUE);

#if defined(COSIM)
    XorUt_FetchCosimTestConfig();
#elif (defined(SIM) || defined(FPGA) || defined(ASIC))
// These situation needn't to do anything.
#else 
#error "Running environment type must be defined!"
#endif
    XorUt_CheckTestConfig();

    XORUT_TEST_TASK aTestTask[XORUT_TEST_TASK_CNT];

    while(TRUE)
    {
        XorUt_TestTaskInit(aTestTask);

        // Clear SSU in DRAM
        COM_MemZero((U32 *)XorUt_GetSsuDramBaseAddr(), ((g_usXorProtectRatio * g_ucTestTaskCnt) >> 2));

        if (g_bIsMultiXoreTest == FALSE)
        {
            XorUt_SetXoreValidOnly(g_ucTestXoreId);
            g_ucTestTaskCnt = 1;
        }
        
        l_ucFinishedTestTaskCnt = 0;
        U32 ulTestTaskIndex = 0;
        
        while (l_ucFinishedTestTaskCnt < g_ucTestTaskCnt)
        {
            while (aTestTask[ulTestTaskIndex].bIsFinished == TRUE)
            {
                ulTestTaskIndex = (ulTestTaskIndex + 1) % g_ucTestTaskCnt;
            }
            
            XorUt_BasicTest(&aTestTask[ulTestTaskIndex]);

            ulTestTaskIndex = (ulTestTaskIndex + 1) % g_ucTestTaskCnt;
        }

        ++ucLoopCnt;
        ucLoopCnt = ucLoopCnt % 50;
     }
    

    XorUt_Log(XOR_UT_ALL_TEST_TASK_DONE, NULL);
    while (TRUE){ ; }

    return TRUE;
}

