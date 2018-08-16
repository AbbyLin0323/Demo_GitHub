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
LOCAL MCU12_DRAM_TEXT L3_UT_STRESS_READ_MGR l_tRdStressMgr[SUBSYSTEM_SUPERPU_MAX] = { 0 };
LOCAL MCU12_DRAM_TEXT U8 aBlkInfo[SUBSYSTEM_SUPERPU_MAX][BLK_PER_LUN] = { 0 };
LOCAL MCU12_DRAM_TEXT L3_UT_SUPERPU_MG_DPTR l_ptSuperPuMgDptr[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL U8 MCU12_DRAM_TEXT L3_GetLPNCaseOffset();
extern GLOBAL L3_UT_LPN_READ_CASE * MCU12_DRAM_TEXT L3_GetLPNCase();

/*==============================================================================
Func Name  : L3_UTRdStressInit
Input      : void  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.3.19 VIA create function
==============================================================================*/
GLOBAL void L3_UTRdStressInit(void)
{
    U8 ucSurperPU;

    COM_MemZero((U32*)&l_tRdStressMgr[0], sizeof(L3_UT_STRESS_READ_MGR)*SUBSYSTEM_SUPERPU_MAX/sizeof(U32));
    COM_MemZero((U32*)l_ptSuperPuMgDptr, sizeof(L3_UT_SUPERPU_MG_DPTR)*SUBSYSTEM_SUPERPU_MAX/sizeof(U32));
    for (ucSurperPU = 0; ucSurperPU < SUBSYSTEM_SUPERPU_NUM; ucSurperPU++)
    {
        l_ptSuperPuMgDptr[ucSurperPU].ulLunBitMap = SUPERPU_LUN_NUM_BITMSK;
    }

    return;
}

GLOBAL L3_UT_SUPERPU_MG_DPTR* L3_GetSuperMgDptr(void)
{
    return l_ptSuperPuMgDptr;
}

GLOBAL BOOL L3_IsSuperPUDone(U8 ucSuperPu)
{
    L3_UT_SUPERPU_MG_DPTR* ptSuperPuDptr;

    ptSuperPuDptr = L3_GetSuperMgDptr();

    if (SUPERPU_LUN_NUM_BITMSK == ptSuperPuDptr[ucSuperPu].ulLunBitMap)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

GLOBAL U16 L3_UTRandBlk()
{
    U16 usBlk;
    
    usBlk = (rand() % BLK_PER_LUN);
    return usBlk;
}

GLOBAL U16 L3_UTRandPg()
{
    U16 ucPg;
    
    ucPg = (rand() % PG_PER_BLK);
    return ucPg;
}

LOCAL BOOL L3_UTSetAddrInfo(L3_UT_PU_MG_DPTR *ptPuDptr, U8 ucStage, U8 ucSuperPu)
{
    BOOL bRet = TRUE;

        if(L3_UT_READSTRESS_STATE_ERASE == ucStage)
        {
            ptPuDptr->Blk = L3_UTRandBlk();
            if(ptPuDptr->Blk == l_tRdStressMgr[ucSuperPu].usOpenBlk)
            {
                bRet = FALSE;
            }
        }
        
        else if(L3_UT_READSTRESS_STATE_WRITE == ucStage)
        {
            if(0 == l_tRdStressMgr[ucSuperPu].usPgCnt)
            {
                ptPuDptr->Blk = L3_UTRandBlk();
                ptPuDptr->Page = l_tRdStressMgr[ucSuperPu].usPgCnt;
                l_tRdStressMgr[ucSuperPu].usOpenBlk = ptPuDptr->Blk;
                if(TRUE == aBlkInfo[ucSuperPu][ptPuDptr->Blk])
                {
                    bRet = FALSE;
                }
            }
            else
            {
                ptPuDptr->Blk = l_tRdStressMgr[ucSuperPu].usOpenBlk;
                ptPuDptr->Page = l_tRdStressMgr[ucSuperPu].usPgCnt;
            }
        }
        else
        {
            ptPuDptr->Blk = L3_UTRandBlk();
            ptPuDptr->Page = L3_UTRandPg();
            if(FALSE == aBlkInfo[ucSuperPu][ptPuDptr->Blk])
            {
                bRet = FALSE;
            }
            if(ptPuDptr->Blk == l_tRdStressMgr[ucSuperPu].usOpenBlk)
            {
                if(l_tRdStressMgr[ucSuperPu].usPgCnt <= ptPuDptr->Page)
                {
                    bRet = FALSE;
                }
            }
        }
    
    return bRet;
}

LOCAL U8 L3_UTRandStage(U8 ucSuperPu)
{
    U8 ucStage;
    
    ucStage = (rand() % L3_UT_READSTRESS_STATE_DONE);
    if(L3_UT_READSTRESS_STATE_INIT == ucStage)
    {
        ucStage = L3_UT_READSTRESS_STATE_READ;
    }
    
    return ucStage;
}

LOCAL void L3_UTSetStageInfo(L3_UT_PU_MG_DPTR *ptPuDptr, U8 ucStage, U8 ucSuperPu)
{
    if(TRUE == L3_IsSuperPUDone(ucSuperPu))
    {
        if((0 == l_tRdStressMgr[ucSuperPu].usWtBlkCnt)
            && (0 == l_tRdStressMgr[ucSuperPu].usPgCnt))
        {
            l_tRdStressMgr[ucSuperPu].ucStage = L3_UT_READSTRESS_STATE_WRITE;
            L3_UTSetAddrInfo(ptPuDptr, l_tRdStressMgr[ucSuperPu].ucStage, ucSuperPu);

        }
        else if(l_tRdStressMgr[ucSuperPu].usWtBlkCnt >= BLK_PER_LUN)
        {
            l_tRdStressMgr[ucSuperPu].ucStage = L3_UT_READSTRESS_STATE_ERASE;
        }
        else
        {
            l_tRdStressMgr[ucSuperPu].ucStage = L3_UTRandStage(ucSuperPu);
            while(FALSE == L3_UTSetAddrInfo(ptPuDptr, l_tRdStressMgr[ucSuperPu].ucStage, ucSuperPu));
        }
    }
    return;
}


U8 L3_UTSelectLunForSend(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;
    L3_UT_SUPERPU_MG_DPTR *ptSuperPuDptr;

    ptSuperPuDptr = &l_ptSuperPuMgDptr[ucSuperPu];
    ucLunInSuperPu = HAL_CLZ(ptSuperPuDptr->ulLunBitMap);

    if (32 != ucLunInSuperPu)
    {
        return (31 - ucLunInSuperPu);
    }

    return INVALID_2F;
}

GLOBAL BOOL L3_UTUpdateLUNBitMap(U8 ucSuperPu)
{
    L3_UT_SUPERPU_MG_DPTR* ptSuperPuDptr;
    U8 ucLunInSuperPu;

    ptSuperPuDptr = L3_GetSuperMgDptr();
    ucLunInSuperPu = L3_UTSelectLunForSend(ucSuperPu);

    ptSuperPuDptr[ucSuperPu].ulLunBitMap &= ~(1 << ucLunInSuperPu);
    if (0 != ptSuperPuDptr[ucSuperPu].ulLunBitMap)
    {
        return FALSE;
    }
    else
    {
        ptSuperPuDptr[ucSuperPu].ulLunBitMap = SUPERPU_LUN_NUM_BITMSK;
        return TRUE;
    }
}

LOCAL U8 L3_UTUpdateBlkInfo(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucCurStage, U16 usBlk)
{
        if(ucCurStage == L3_UT_READSTRESS_STATE_ERASE)
        {
            if(TRUE == aBlkInfo[ucSuperPu][usBlk])
            {
                aBlkInfo[ucSuperPu][usBlk] = FALSE;
                l_tRdStressMgr[ucSuperPu].usWtBlkCnt--;
            }
        }
        else if(ucCurStage == L3_UT_READSTRESS_STATE_WRITE)
        {
            aBlkInfo[ucSuperPu][usBlk] = TRUE;
            l_tRdStressMgr[ucSuperPu].usPgCnt++;
            if(l_tRdStressMgr[ucSuperPu].usPgCnt > PG_PER_BLK -1)
            {
                l_tRdStressMgr[ucSuperPu].usWtBlkCnt++;
                l_tRdStressMgr[ucSuperPu].usPgCnt = 0;
                 l_tRdStressMgr[ucSuperPu].usOpenBlk = INVALID_4F;
            }
        }
        
    return TRUE;
}

BOOL L3_BufferTest(void)
{
    U8 ucBufferCnt = 0, ucNumber = 0, ucWp = 0, ucRp = 0;
    U16 aBufferID[10] = {0};
    U16 ucBuffID;

    return 0;

    DBG_Printf("###start allcate buffer###\n");
    while(1)
    {
        ucNumber = rand() % 10;

        if(ucNumber > 2)
        {
            //ucBuffID = L3_AllocateBuffer();
            if(INVALID_4F != ucBuffID)
            {
                aBufferID[ucWp] = ucBuffID;
                ucBufferCnt++;
                DBG_Printf("Alocate Buffer ID#%d , Allocate buffer count = %d", aBufferID[ucWp], ucBufferCnt);
                ucWp = (ucWp + 1) % 10;
            }
            else
            {
                DBG_Printf("No free Buffer, Allocate buffer count = %d", ucBufferCnt);
            }
        }
        else
        {
            if(ucBufferCnt != 0)
            {
                //L3_ReleaseBuffer(aBufferID[ucRp]);
                ucBufferCnt--;
                DBG_Printf("###Release Buffer ID#%d, Allocate buffer count = %d", aBufferID[ucRp], ucBufferCnt);
                ucRp = (ucRp + 1) % 10;
            }
            else
            {
                DBG_Printf("Allocate buffer count = %d, not busy buffer can release", ucBufferCnt);
            }
        }
        DBG_Printf("\n");
    }
}

/*==============================================================================
Func Name  : L3_UTMixStressTest
Input      : None
Output     : None
Return Val : 
Discription: L3 Unit Test read stress test.
Usage      : 
History    : 
    1. 2016.03.11 stevenChang create function
==============================================================================*/
LOCAL BOOL L3_UTMixStressTest(void)
{
    U8 ucSuperPu, ucLunInSuperPu, ucTLun;
    L3_UT_PU_MG_DPTR *ptPuDptr;
    BUF_REQ_READ tReadBufReq;
    PhysicalAddr   phyAddr = { 0 };
    PhysicalAddr atAddr[LPN_PER_BUF] = { 0 };
    L3_UT_LPN_READ_CASE *l_aLpnRdCase;
    U8 ucLPNOffset;
    LOCAL U32 l_ulLoop = 0;
    
    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        /*convert [ucSuperPu:ucLunInSuperPu] to [ucPu:ucBLun]*/
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
        L3_UTSetStageInfo(ptPuDptr, l_tRdStressMgr[ucSuperPu].ucStage, ucSuperPu); 
        
        switch(l_tRdStressMgr[ucSuperPu].ucStage)
        {
            case L3_UT_READSTRESS_STATE_INIT:
            {
                //tmp do nothing
                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {                    
                }            
                break;
            }
            
            case L3_UT_READSTRESS_STATE_ERASE:
            {
                L3_UTDataCheck(ucTLun);
                
                L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, NULL, FALSE, FALSE, TRUE);//last parameter need owner double check
                //DBG_Printf("MCU#%d [SuperPu#%d, LunInSuperPu#%d Blk#%d] Erase Stage\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk);

                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    // add the return condition for read stress test.
                    l_ulLoop++;
                    if (FCMD_TEST_BURN_CNT == l_ulLoop)
                    {
                        l_ulLoop = 0;
                        l_tRdStressMgr[ucSuperPu].ucStage = L3_UT_READSTRESS_STATE_DONE;
                    }
                    else
                    {
                        L3_UTUpdateBlkInfo(ucSuperPu, ucLunInSuperPu, L3_UT_READSTRESS_STATE_ERASE, ptPuDptr->Blk);
                    }
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_WRITE:
            {
                L3_UTDataCheck(ucTLun);
                
                L3_UTPrepareForWrite(ucSuperPu, ucLunInSuperPu, &phyAddr, ptPuDptr);              
                L2_FtlWriteLocal(&phyAddr, (U32 *)ptPuDptr->DataBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, FALSE, ptPuDptr->FlashSLCMode, NULL);

                //DBG_Printf("MCU#%d [SuperPu#%d, LunInSuperPu#%d Blk#%d, Page#%d] Write Stage.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
               
                if(TRUE == L3_UTUpdateLUNBitMap(ucSuperPu))
                {
                    L3_UTUpdateBlkInfo(ucSuperPu, ucLunInSuperPu, L3_UT_READSTRESS_STATE_WRITE, ptPuDptr->Blk);
                }
                break;
            }
            
            case L3_UT_READSTRESS_STATE_READ:
            {
                L3_UTDataCheck(ucTLun);
                
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD, &phyAddr, ptPuDptr, NULL);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, LPN_PER_BUF, 0, FALSE, ptPuDptr->FlashSLCMode);
                //DBG_Printf("MCU#%d [SuperPu#%d, LunInSuperPu#%d Blk#%d, Page#%d] Read Stage.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);

                L3_UTUpdateLUNBitMap(ucSuperPu);
                break;
            }

            case L3_UT_READSTRESS_STATE_RD_RED:
            {   
                L3_UTDataCheck(ucTLun);
                
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_RED, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_RED.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, 0, 0, FALSE, ptPuDptr->FlashSLCMode);

                L3_UTUpdateLUNBitMap(ucSuperPu);
                break;
            }
            
            case L3_UT_READSTRESS_STATE_RD_DATA:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_DATA, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_DATA.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL, LPN_PER_BUF, 0, FALSE, ptPuDptr->FlashSLCMode);

                L3_UTUpdateLUNBitMap(ucSuperPu);
                break;
            }

            case L3_UT_READSTRESS_STATE_RD_LPN:
            {
                L3_UTDataCheck(ucTLun);

                ucLPNOffset = L3_GetLPNCaseOffset();
                l_aLpnRdCase = L3_GetLPNCase();
                
                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_LPN, &phyAddr, ptPuDptr, NULL);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_LPN [%d, %d, %d, %d].\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ucLPNOffset, phyAddr.m_LPNInPage, l_aLpnRdCase[ucLPNOffset].StartInBuf, l_aLpnRdCase[ucLPNOffset].Cnt);
                L2_FtlReadLocal((U32 *)ptPuDptr->DataBuffAddr, &phyAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, NULL, l_aLpnRdCase[ucLPNOffset].Cnt, l_aLpnRdCase[ucLPNOffset].StartInBuf, FALSE, ptPuDptr->FlashSLCMode);

                L3_UTUpdateLUNBitMap(ucSuperPu);
                break;
            }
            
            case L3_UT_READSTRESS_STATE_RD_MERGE:
            {
                L3_UTDataCheck(ucTLun);

                L3_UTPrepareForRD(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_MERGE, atAddr, ptPuDptr, &tReadBufReq);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_NORMAL_STAGE_MERGE_RD.BufID=0x%x LpnOffSet=0x%x, LpnBitMap=0x%x\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page,tBufReq.PhyBufferID, tBufReq.LPNOffset,tBufReq.LPNBitMap);
                if (TRUE != L2_FtlReadMerge(ucSuperPu, INVALID_8F, &tReadBufReq, atAddr))
                {
                    DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_MERGE getch.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page);
                    DBG_Getch();
                }

                L3_UTUpdateLUNBitMap(ucSuperPu);
                break;
            }
            
            case L3_UT_READSTRESS_STATE_RD_4KSEQ:
            {
                U16 aBuffId[DSG_BUFF_SIZE] = { 0 };
                FCMD_REQ_ITEM tFCmdReqItem = { 0 };
                FCMD_READ_RANGE tRange = { 0 };

                L3_UTDataCheck(ucTLun);

                L3_UTPrepareFor4KSEQR(ucSuperPu, ucLunInSuperPu, ptPuDptr, &tFCmdReqItem, aBuffId, &tRange);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_4KSEQ LPNCnt=%d.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, tRange.bsLPNCnt);
                L3_NormalReadPage(&tFCmdReqItem, aBuffId, &tRange, (U8*)ptPuDptr->FlashStatusBuffAddr);

                L3_UTUpdateLUNBitMap(ucSuperPu);
                break;
            }
            
            case L3_UT_READSTRESS_STATE_DONE:
            {
                return TRUE;
            }

            default:
            {
                DBG_Printf("MixStress Test Stage Error.\n");
                DBG_Getch();
            }
        }
    }

    return FALSE;
}



/*==============================================================================
Func Name  : L3_MixStressTest
Input      : void  
Output     : NONE
Return Val : 
Discription: Only for read stress test case
Usage      : 
History    : 
    1. 2016.3.19 VIA create function
==============================================================================*/
BOOL L3_MixStressTest(void)
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

            if (LUN_NUM_PER_SUPERPU > 1)
            {
                DBG_Printf("MixStressTest does not support multi-lun now. Please Steven to update the stage machine.\n");
                l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_INIT;
                bRdStressTestDone = TRUE;
            }
            break;
        }
        case RD_STRESS_TEST_STAGE_RUN:
        {
#if 0
            //test buffer
            while(1)
            {                
                L3_BufferTest();
            }
            //test buffer
#endif
            if (TRUE == L3_UTMixStressTest())
            {
                l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_INIT;
            }
            break;
        }
        case RD_STRESS_TEST_STAGE_FINISH:
        {
            DBG_Printf("\nMCU#%d L3_MixStressTest Finish.\n\n", HAL_GetMcuId());
            l_ulRdStressTestStage = RD_STRESS_TEST_STAGE_INIT;
            bRdStressTestDone = TRUE;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d L3_MixStressTest Status=%d Err.\n", HAL_GetMcuId(), l_ulRdStressTestStage);
            DBG_Getch();
        }
    }

    return bRdStressTestDone;
}

/*====================End of this file========================================*/

