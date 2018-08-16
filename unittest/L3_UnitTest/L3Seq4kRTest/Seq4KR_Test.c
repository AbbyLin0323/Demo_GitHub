/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : Seq4KR_Test.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.3.19
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Xtensa.h"
#include "L2_FTL.h"
#include "FCMD_Test.h"
#include "Seq4KR_Test.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
typedef enum _SEQ4KR_TEST_STAGE_
{
    SEQ4KR_TEST_STAGE_INIT = 0,
    SEQ4KR_TEST_STAGE_RUN,
    SEQ4KR_TEST_STAGE_FINISH
}SEQ4KR_TEST_STAGE;

#define TOTAL_TEST_CYCLE        10
#define TOTAL_CMD_NUM_PER_CYCLE 1000
/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
GLOBAL U32 g_aFwStartEnd[32][2];
GLOBAL U32 g_aFwCycleMax[32];
GLOBAL U32 g_aFwCycleMin[32];
GLOBAL U32 g_aFwCycleAvr[32];
GLOBAL U32 g_aFwCycleAvrCnt[32];
GLOBAL U32 g_aFwCycleTotal[32];
GLOBAL U32 g_aFwRunCnt[32];
    
GLOBAL U32 g_startTs;
GLOBAL U32 g_startFlag[32];
GLOBAL U32 g_startFlagPuCnt;
GLOBAL U32 g_endTs;
GLOBAL U32 g_totalCmdTime;
GLOBAL U32 g_printCount;

LOCAL void L3_UTPerformanceResetVar(void)
{
    g_startTs = 0;
    COM_MemZero((U32*)g_startFlag, 32);
    g_startFlagPuCnt = 0;
    g_endTs = 0;
    g_totalCmdTime = 0;
    g_printCount = 0;

    COM_MemZero((U32*)&g_aFwStartEnd[0][0], 32 * 2);
    COM_MemZero((U32*)g_aFwCycleMax, 32);
    COM_MemSet((U32*)g_aFwCycleMin, 32, 0xfffffff0ul);
    COM_MemZero((U32*)g_aFwCycleAvr, 32);
    COM_MemZero((U32*)g_aFwCycleAvrCnt, 32);
    COM_MemZero((U32*)g_aFwCycleTotal, 32);
    COM_MemZero((U32*)g_aFwRunCnt, 32);
    
    return;
}

LOCAL void L3_UTPerformanceStart(U8 ucPu)
{
    if (g_startTs != 0)
    {
        g_aFwStartEnd[ucPu][0] = HAL_GetMCUCycleCount();
    }

    return;
}
LOCAL BOOL L3_UTPerformanceWholeCycle(U8 ucPu)
{
    if (g_startTs != 0)
    {
         g_totalCmdTime++;
         if (TOTAL_CMD_NUM_PER_CYCLE == g_totalCmdTime)
         {
             g_endTs = HAL_GetMCUCycleCount();
             DBG_Printf("MCU %d start = 0x%x , end = 0x%x, totalcycle = 0x%x\n",HAL_GetMcuId(),g_startTs, g_endTs, COM_DiffU32(g_startTs, g_endTs));
             DBG_Printf("MCU %d FwMax = 0x%x , FwMin = 0x%x , FwAvr = 0x%x\n",HAL_GetMcuId(),
                g_aFwCycleMax[ucPu],g_aFwCycleMin[ucPu],g_aFwCycleAvr[ucPu]);
             g_printCount++;
             if (TOTAL_TEST_CYCLE == g_printCount)
             {
                return TRUE;
             }
             
             {
                 U32 i;
                 for (i=0; i<SUBSYSTEM_LUN_NUM; i++)
                 {
                     g_startFlag[i]=0;
                 }
                 g_startTs = 0;
                 g_endTs   = 0;
                 g_startFlagPuCnt = 0;
                 g_totalCmdTime = 0;
             } 
         }
     }

    return FALSE;
}
LOCAL void L3_UTPerformanceFwCycle(U8 ucPu)
{
    U32 ulFwCycle;
    U32 ulFwCycleAvr;
    
    if (g_startTs != 0)
    {
        g_aFwStartEnd[ucPu][1] = HAL_GetMCUCycleCount();
        ulFwCycle = COM_DiffU32(g_aFwStartEnd[ucPu][0],g_aFwStartEnd[ucPu][1]);
        g_aFwCycleMax[ucPu] = max(g_aFwCycleMax[ucPu],ulFwCycle);
        g_aFwCycleMin[ucPu] = min(g_aFwCycleMin[ucPu],ulFwCycle);
        g_aFwCycleTotal[ucPu] += ulFwCycle;
        g_aFwRunCnt[ucPu]++;
        if (0 == g_aFwRunCnt[ucPu] % 1000)
        {
            g_aFwCycleAvrCnt[ucPu]++;
            ulFwCycleAvr = g_aFwCycleTotal[ucPu]/g_aFwRunCnt[ucPu];
            g_aFwCycleAvr[ucPu] = (g_aFwCycleAvr[ucPu]*(g_aFwCycleAvrCnt[ucPu]-1) + ulFwCycleAvr)/g_aFwCycleAvrCnt[ucPu];
            g_aFwCycleMax[ucPu] = 0;
            g_aFwCycleMin[ucPu] = 0xfffffff0ul;
            g_aFwCycleTotal[ucPu] = 0;
            g_aFwRunCnt[ucPu] = 0;
        }
    }
    if (g_startFlag[ucPu] == 0)
    {
        g_startFlag[ucPu] = 1;
        
        g_startFlagPuCnt++;
        if (g_startFlagPuCnt == SUBSYSTEM_LUN_NUM)
        {
            g_startTs = HAL_GetMCUCycleCount();
        }
    }

    return;
}

/*==============================================================================
Func Name  : Seq4kR_Test
Input      : void  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.3.19 JasonGuo create function
==============================================================================*/
LOCAL BOOL Seq4kR_Test(void)
{
    U8 ucSuperPu;
    U8 ucLunInSuperPu;
    U8 ucTLun;
    U8 ucNextStage;
    L3_UT_PU_MG_DPTR *ptPuDptr;
    PhysicalAddr   phyAddr = { 0 };
    BOOL bFinishSeq4kRTest = FALSE;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        /*select one LunInSuperPu to test*/
        ucLunInSuperPu = L3_UTSelectLunForPush(ucSuperPu);
        if (INVALID_2F == ucLunInSuperPu)
        {
            continue;
        }

        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        ptPuDptr = &g_ptPuMgDptr[ucTLun];
        
        switch (ptPuDptr->CurStage)
        {
            case L3_UT_LOCAL_STAGE_INIT:
            {
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_INIT);
                break;
            }
            case L3_UT_LOCAL_STAGE_ERS:
            {
                L2_FtlEraseBlock(ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, NULL, FALSE, ptPuDptr->FlashSLCMode, TRUE);//last parameter need owner double check
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_ERS);
                break;
            }
            case L3_UT_LOCAL_STAGE_WT:
            {
                L3_UTPrepareForWrite(ucSuperPu, ucLunInSuperPu, &phyAddr, ptPuDptr);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_WT SLC=%d\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page,ptPuDptr->FlashSLCMode);
                L2_FtlWriteLocal(&phyAddr, (U32 *)ptPuDptr->DataBuffAddr, (U32 *)ptPuDptr->SpareBuffAddr, (U8 *)ptPuDptr->FlashStatusBuffAddr, FALSE, ptPuDptr->FlashSLCMode, NULL);
                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_WT);
                
                if (L3_UT_LOCAL_STAGE_WT != ucNextStage)
                {
                    ucNextStage = L3_UT_LOCAL_STAGE_RD_4KSEQ;
                }
                break;
            }
            case L3_UT_LOCAL_STAGE_RD_4KSEQ:
            {
                U16 aBuffId[DSG_BUFF_SIZE] = { 0 };
                FCMD_REQ_ITEM tFCmdReqItem = { 0 };
                FCMD_READ_RANGE tRange;

                L3_UTPerformanceStart(ucTLun);

                L3_UTPrepareFor4KSEQR(ucSuperPu, ucLunInSuperPu, ptPuDptr, &tFCmdReqItem, aBuffId, &tRange);
                //DBG_Printf("MCU#%d [%d-%d,%d,%d] L3_UT_LOCAL_STAGE_RD_4KSEQ LPNCnt=%d.\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, tRange.bsLPNCnt);
                L3_NormalReadPage(&tFCmdReqItem, aBuffId, &tRange, (U8*)ptPuDptr->FlashStatusBuffAddr);

                ucNextStage = L3_UTSelectNextStage(ucSuperPu, ucLunInSuperPu, L3_UT_LOCAL_STAGE_RD_4KSEQ);
                if (L3_UT_LOCAL_STAGE_RD_4KSEQ != ucNextStage)
                {
                    ucNextStage = L3_UT_LOCAL_STAGE_RD_4KSEQ;
                }

                ucNextStage = (TRUE == L3_UTPerformanceWholeCycle(ucTLun)) ? L3_UT_LOCAL_STAGE_END : ucNextStage;
                L3_UTPerformanceFwCycle(ucTLun);
                
                break;
            }
            case L3_UT_LOCAL_STAGE_END:
            {
                bFinishSeq4kRTest = TRUE;
                ucNextStage = L3_UT_LOCAL_STAGE_INIT;
                break;
            }
            default:
            {
                DBG_Printf("MCU#%d [%d-%d,%d,%d] Seq4kR_Test Error Local Stage=0x%x!\n", HAL_GetMcuId(), ucSuperPu, ucLunInSuperPu, ptPuDptr->Blk, ptPuDptr->Page, ptPuDptr->CurStage);
                DBG_Getch();
            }
        }

        ptPuDptr->CurStage = ucNextStage;
    }

    return bFinishSeq4kRTest;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_Seq4kRTest
Input      : void  
Output     : NONE
Return Val : 
Discription: only for test the performance of the 4k seq read
Usage      : 
History    : 
    1. 2016.3.19 JasonGuo create function
==============================================================================*/
BOOL L3_Seq4kRTest(void)
{
    BOOL bSeq4kRTestDone = FALSE;
    LOCAL BOOL l_bSeq4kRTestStage = SEQ4KR_TEST_STAGE_INIT;

    switch (l_bSeq4kRTestStage)
    {
        case SEQ4KR_TEST_STAGE_INIT:
        {
            L3_UTBasicInit();
            L3_UTPerformanceResetVar();
            l_bSeq4kRTestStage = SEQ4KR_TEST_STAGE_RUN;
            break;
        }
        case SEQ4KR_TEST_STAGE_RUN:
        {
            if (TRUE == Seq4kR_Test())
            {                
                l_bSeq4kRTestStage = SEQ4KR_TEST_STAGE_FINISH;
            }
            break;
        }
        case SEQ4KR_TEST_STAGE_FINISH:
        {            
            DBG_Printf("\nMCU#%d L3_Seq4kRTest Finish.\n\n", HAL_GetMcuId());
            l_bSeq4kRTestStage = SEQ4KR_TEST_STAGE_INIT;
            bSeq4kRTestDone = TRUE;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d L3_Seq4kRTest Status=%d Err.\n", HAL_GetMcuId(), l_bSeq4kRTestStage);
            DBG_Getch();
        }
    }
    
    return bSeq4kRTestDone;
}


/*====================End of this file========================================*/

