/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : ReadStress_Test.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2014.12.9
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Xtensa.h"
#include "L2_FTL.h"
#include "FCMD_Test.h"
#include "ReadStress_Test.h"
#include "L3_BufMgr.h"
#include "MixStress_Test.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
extern GLOBAL L3_UT_SUPERPU_MG_DPTR* L3_GetSuperMgDptr(void);
extern GLOBAL U8 MCU12_DRAM_TEXT L3_GetLPNCaseOffset();
extern GLOBAL L3_UT_LPN_READ_CASE * MCU12_DRAM_TEXT L3_GetLPNCase();
extern GLOBAL BOOL L3_UTUpdateLUNBitMap(U8 ucSuperPu);
extern GLOBAL U16 L3_UTRandBlk();
extern GLOBAL U16 L3_UTRandPg();
extern GLOBAL BOOL L3_IsSuperPUDone(U8 ucSuperPu);
extern GLOBAL void L3_UTRdStressInit(void);
/*==============================================================================
Func Name  : L3_UTReadStressTest
Input      : None
Output     : None
Return Val : 
Discription: L3 Unit Test read stress test.
Usage      : 
History    : 
    1. 2016.03.11 stevenChang create function
==============================================================================*/
LOCAL BOOL L3_UTReadStressTest(void)
{
    U8 ucSuperPu, ucLunInSuperPu, ucTLun;
    L3_UT_PU_MG_DPTR *ptPuDptr;
    BUF_REQ_READ tReadBufReq;
    PhysicalAddr   phyAddr = { 0 };
    PhysicalAddr atAddr[LPN_PER_BUF] = { 0 };
    L3_UT_LPN_READ_CASE *l_aLpnRdCase;
    U8 ucLPNOffset;
    LOCAL U8 aStage[SUBSYSTEM_SUPERPU_MAX] = {L3_UT_READSTRESS_STATE_INIT};
    LOCAL U16 aBlkCnt[SUBSYSTEM_SUPERPU_MAX] = {0};
    LOCAL U16 aPgCnt[SUBSYSTEM_SUPERPU_MAX] = {0};
    LOCAL U32 l_ulLoop = 0;
    
    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        /*select one LunInSuperPu to test*/
        ucLunInSuperPu = L3_UTSelectLunForSend(ucSuperPu);
        if (INVALID_2F == ucLunInSuperPu)
        {
            DBG_Printf("MCU#%d SuperPu=%d, LunInSuperPu=%d SelectLunForSend Fail.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu);
            DBG_Getch();
        }
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (FALSE == L2_FCMDQNotFull(ucTLun))
        {
            continue; 
        }

        /*obtain ucPu's manaer and total test page num*/
        ptPuDptr = &g_ptPuMgDptr[ucTLun];
    
        switch(aStage[ucSuperPu])
        {
            case L3_UT_READSTRESS_STATE_INIT:
            {
                //tmp do nothing
                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_ERASE;
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_ERASE:
            {
                ptPuDptr->Blk = aBlkCnt[ucSuperPu];
                    
                L3_UTDataCheck(ucTLun);

                L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, NULL, FALSE, FALSE, TRUE);//last parameter need owner double check
                //DBG_Printf("MCU#%d [SuperPu#%d, LunInSuperPu#%d, Blk#%d] Erase Stage\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aBlkCnt[ucSuperPu]++;
                    if(aBlkCnt[ucSuperPu] > BLK_PER_LUN - 1)
                    {
                        // add the return condition for read stress test.
                        l_ulLoop++;
                        if (FCMD_TEST_BURN_CNT == l_ulLoop)
                        {
                            l_ulLoop = 0;
                            aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_DONE;
                        }
                        else
                        {
                            aBlkCnt[ucSuperPu] = 0;
                            aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_WRITE;
                        }
                    }
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_WRITE:
            {
                ptPuDptr->Blk = aBlkCnt[ucSuperPu];
                ptPuDptr->Page = aPgCnt[ucSuperPu];
                
                L3_UTDataCheck(ucTLun);
                L3_UTPrepareForWrite(ucSuperPu, ucLunInSuperPu, &phyAddr, ptPuDptr);              
                L2_FtlWriteLocal(&phyAddr, (U32 *)ptPuDptr->DataBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, FALSE, ptPuDptr->FlashSLCMode, NULL);
                if(0 == ptPuDptr->Page)
                {
                    DBG_Printf("MCU#%d [SuperPu#%d, LunInSuperPu#%d, Blk#%d, Page#%d] Write Stage.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                }
                
                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aPgCnt[ucSuperPu]++;
                    if(aPgCnt[ucSuperPu] > PG_PER_BLK - 1)
                    {
                        aBlkCnt[ucSuperPu]++;
                        aPgCnt[ucSuperPu] = 0;
                        if(aBlkCnt[ucSuperPu] > BLK_PER_LUN - 1)
                        {
                            aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ;
                            aBlkCnt[ucSuperPu] = 0;
                        }
                    }
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_READ:
            {
                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = L3_UTRandPg();
                
                L3_UTDataCheck(ucTLun);
                
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD, &phyAddr, ptPuDptr, NULL);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, LPN_PER_BUF, 0, FALSE, ptPuDptr->FlashSLCMode);
                //DBG_Printf("MCU#%d [SuperPu#%d, LunInSuperPu#%d, Blk#%d, Page#%d] Read Stage.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ + (L3_UTRand() % 6);
                }
                break;
            }

            case L3_UT_READSTRESS_STATE_RD_RED:
            {   
                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = L3_UTRandPg();
                
                L3_UTDataCheck(ucTLun);
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_RED, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_RED.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, 0, 0, FALSE, ptPuDptr->FlashSLCMode);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ + (L3_UTRand() % 6);
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_RD_DATA:
            {
                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = L3_UTRandPg();
                
                L3_UTDataCheck(ucTLun);
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_DATA, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_DATA.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL, LPN_PER_BUF, 0, FALSE, ptPuDptr->FlashSLCMode);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ + (L3_UTRand() % 6);
                }
                break;
            }

            case L3_UT_READSTRESS_STATE_RD_LPN:
            {
                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = L3_UTRandPg();
                
                L3_UTDataCheck(ucTLun);
                ucLPNOffset = L3_GetLPNCaseOffset();
                l_aLpnRdCase = L3_GetLPNCase();
                
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_LPN, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_LPN [%d, %d, %d, %d].\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ucLPNOffset, phyAddr.m_LPNInPage, l_aLpnRdCase[ucLPNOffset].StartInBuf, l_aLpnRdCase[ucLPNOffset].Cnt);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL, l_aLpnRdCase[ucLPNOffset].Cnt, l_aLpnRdCase[ucLPNOffset].StartInBuf, FALSE, ptPuDptr->FlashSLCMode);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ + (L3_UTRand() % 6);
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_RD_MERGE:
            {
                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = L3_UTRandPg();
                
                L3_UTDataCheck(ucTLun);
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_MERGE, atAddr, ptPuDptr, &tReadBufReq);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_NORMAL_STAGE_MERGE_RD.BufID=0x%x LpnOffSet=0x%x, LpnBitMap=0x%x\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, tBufReq.PhyBufferID, tBufReq.LPNOffset, tBufReq.LPNBitMap);
                if (TRUE != L2_FtlReadMerge(ucSuperPu, INVALID_8F, &tReadBufReq, atAddr))
                {
                    DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_MERGE getch.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                    DBG_Getch();
                }

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ + (L3_UTRand() % 6);
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_RD_4KSEQ:
            {
                U16 aBuffId[DSG_BUFF_SIZE] = { 0 };
                FCMD_REQ_ITEM tFCmdReqItem = { 0 };
                FCMD_READ_RANGE tRange = { 0 };

                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = L3_UTRandPg();

                L3_UTDataCheck(ucTLun);
                L3_UTPrepareFor4KSEQR(ucSuperPu, ucLunInSuperPu, ptPuDptr, &tFCmdReqItem, aBuffId, &tRange);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_4KSEQ LPNCnt=%d.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, tRange.bsLPNCnt);
                L3_NormalReadPage(&tFCmdReqItem, aBuffId, &tRange, (U8*)ptPuDptr->FlashStatusBuffAddr);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    aStage[ucSuperPu] = L3_UT_READSTRESS_STATE_READ + (L3_UTRand() % 6);
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_DONE:
            {
                return TRUE;
            }

            default:
            {
                DBG_Printf("ReadStress Test Stage Error.\n");
                DBG_Getch();
            }
        }
    }

    return FALSE;
}

/*==============================================================================
Func Name  : L3_ReadStressTest
Input      : void  
Output     : NONE
Return Val : 
Discription: Only for read stress test case
Usage      : 
History    : 
    1. 2016.3.19 VIA create function
==============================================================================*/
BOOL L3_ReadStressTest(void)
{
    BOOL bRdStressTestDone = FALSE;
    LOCAL U32 l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_INIT;

    switch (l_ulRdStressTestStage)
    {
        case RD_STRESS_TEST_STAGE_INIT:
        {
            L3_UTBasicInit();
            L3_UTRdStressInit();
            l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_RUN;
            break;
        }
        case RD_STRESS_TEST_STAGE_RUN:
        {
            if (TRUE == L3_UTReadStressTest())
            {
                l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_INIT;
            }
            break;
        }
        case RD_STRESS_TEST_STAGE_FINISH:
        {
            DBG_Printf("\nMCU#%d L3_ReadStressTest Finish.\n\n", HAL_GetMcuId());
            l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_INIT;
            bRdStressTestDone = TRUE;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d L3_ReadStressTest Status=%d Err.\n", HAL_GetMcuId(), l_ulRdStressTestStage);
            DBG_Getch();
        }
    }

    return bRdStressTestDone;
}
/*====================End of this file========================================*/

