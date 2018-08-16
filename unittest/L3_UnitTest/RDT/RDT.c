/*******************************************************************************
*               Copyright (C), 2013, VIA Tech. Co., Ltd.                       *
* Information in this file is the intellectual property of VIA Tech, Inc.,     *
* It may contains trade secrets and must be stored and viewed confidentially.  *
********************************************************************************
* File Name    : RDT.c
* Discription  :
* CreateAuthor : Jason
* CreateDate   : 2016.4.27
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Xtensa.h"
#include "L2_FTL.h"
#include "L2_TableBBT.h"
#include "L2_Interface.h"
#include "RDT.h"
#include "FW_BufAddr.h"
#include "HAL_GPIO.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
//#define RDT_ERR_STATISTIC                 // Statistic the error number of E/W/R. Note: can not statixtical the inherit info
//#define RDT_CHECK_RESULT                  //
//#define RDT_ERRH_DISENABLE                // if defined, when encount an error, all the plane will add bbt
//#define RDT_FCMDQ_STATISTIC               // FCMDQ full number statistic
//#define RDT_EWR_TIME_STATISTIC            // FCMD E/W/R test one block time statistic
//#define RDT_FULL_BLOCK_TIME_STATISTIC     // RDT test time statistic, to calculate the time of full LUN

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern BOOL bTBRebuildPending;
extern BOOL g_BootUpOk;
extern U32  g_L2TempBufferAddr;

LOCAL void RDT_Timer_Isr();
LOCAL void RDT_StartTimer();

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
//RDT control
LOCAL RDT_CTRL           *l_ptRdtCtrl;
LOCAL RDT_SORT_MANAGER   *l_ptRdtSortMg;
LOCAL RDT_TEST_MANAGER   *l_ptRdtTestMg;

//RDT write buffer for 2D-TLC
LOCAL U32 (*l_aBuffAddr)  [DSG_BUFF_SIZE * PG_PER_WL];
LOCAL U32 (*l_aRedAddr)   [DSG_BUFF_SIZE * PG_PER_WL];
LOCAL U32 (*l_aStatusAddr)[DSG_BUFF_SIZE] [PG_PER_WL];

//RDT write buffer for 3D-TLC
LOCAL U32 (*l_ulTotalPrgOrder);

//RDT Dram/OTFB/Sram resource
GLOBAL U32 g_ulRdtDramAddr;
GLOBAL U32 g_ulRdtDramAddrSize;
GLOBAL U32 g_ulRdtOtfbAddr;
GLOBAL U32 g_ulRdtOtfbAddrSize;

//RDT Dram/OTFB resource pointer
LOCAL U32 l_ulRdtDummyDataAddr;
LOCAL U32 l_ulRdtDummyDataAddrFor2DTlcWrite;
LOCAL U32 l_ulRdtDummySpareAddr;
LOCAL U32 l_ulRdtDummySpareAddrFor2DTlcWrite;
LOCAL U32 l_ulRdtDummyStatusAddr;
LOCAL U32 l_ulRdtDummyStatusAddrOTFB;

//RDT LED control
LOCAL U8  l_ucLedCtrl;
LOCAL U32 l_ulLedRunCount_Sec;

//RDT Test finish signal
LOCAL BOOL l_bFinishRDTTest = FALSE;

LOCAL U32 l_ulCountInOneUs;
LOCAL U32 l_ulTimeTag1, l_ulTimeTag2, l_ulTimeTag3, l_ulTimeTag4;
LOCAL U32 l_ulTimeTag5, l_ulTimeTag6, l_ulTimeTag7, l_ulTimeTag8;

#ifdef RDT_FCMDQ_STATISTIC
GLOBAL U32 g_aFCMDStatus[16][8] = { 0 };  //[16 TLun][E/W/R/Tatal full---E/W/R/Tatal Tatal]
#endif

#ifdef RDT_ERR_STATISTIC
LOCAL RDT_ERR_NUM l_tErrNum[16] = { 0 };
#endif

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
/*RDT LDE*/
LOCAL MCU1_DRAM_TEXT void RDT_Led_On(void)
{
    rGPIOLED = 0x3131;
    //DBG_Printf("Led On\n");

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_Led_Off(void)
{
    rGPIOLED = 0x2121;
    //DBG_Printf("Led Off\n");

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_Timer_Isr(void)
{
    //DBG_Printf("RDT_Timer_Isr\n");
    l_ulLedRunCount_Sec++;
    if (0 == l_ulLedRunCount_Sec % 60)   //Printf per 1 min
    {
        DBG_Printf("RDT testing Block %d\n", l_ptRdtTestMg[0].Blk);
    }
    if (l_ucLedCtrl == 0)
    {
        RDT_Led_On();
        l_ucLedCtrl = 1;
    }
    else
    {
        RDT_Led_Off();
        l_ucLedCtrl = 0;
    }
    RDT_StartTimer();

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_StartTimer(void)
{
#ifndef SIM
    HAL_StartMcuTimer(1000000, RDT_Timer_Isr); //Call RDT_Timer_Isr() per 1000,000 us
#endif

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_InitTimer(void)
{
    l_ucLedCtrl = 0;
    *(volatile U32 *)(REG_BASE_GLB + 0x50) &= ~(0x1 << 2);
#ifndef SIM
    //#disable IIC function
    *(volatile U32 *)0x1ff80068 = 0x3c00;
#endif
#ifndef SIM
    //HAL_InitInterrupt(BIT_ORINT_TIMER0);
    DBG_Printf("RDT_InitTimer\n");
    HAL_InitInterrupt(TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0,
        BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0 | BIT_ORINT_TIMER0);
#endif

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_CancelTimer(void)
{
#ifndef SIM
    HAL_StopMcuTimer();
#endif
    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_TestEnd(void)
{
    U8 ucTLun;
    BOOL blRes = TRUE;

    RDT_CancelTimer();
    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        if (l_ptRdtCtrl->RdtTlunStat[ucTLun] == RDT_FAIL)
        {
            blRes = FALSE;
            break;
        }
    }
    if (blRes == FALSE)
    {
        RDT_Led_Off();
    }
    else
    {
        RDT_Led_On();
    }
    DBG_Printf("RDT Test end!\n");

    return;
}

/*RDT BBT Bit Map*/
LOCAL MCU1_DRAM_TEXT BOOL RDT_GetBadBlkBit(U8 ucTLun, U8 ucPln, U16 usBlock)
{
    U32 ulBitPos, ulBytePos;
    BOOL bBadBlk;

    ulBitPos = ucPln * (BLK_PER_PLN + RSV_BLK_PER_PLN) + usBlock;
    ulBytePos = l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + ulBitPos / 8;
    bBadBlk = (0 != (*(volatile U8 *)ulBytePos & (1 << (ulBitPos % 8)))) ? TRUE : FALSE;

    return bBadBlk;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_BbtIsBadBlock(U8 ucTLun, U8 ucPln, U16 usBlk)
{
    BOOL bBadBlk = FALSE;

    if (TRUE == RDT_GetBadBlkBit(ucTLun, ucPln, usBlk))
    {
        bBadBlk = TRUE;
    }

    return bBadBlk;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_AllPlnBlkIsBad(U8 ucTLun, U16 ulBlk)
{
    BOOL bAnd = TRUE;
    U8 ucIndex;

    for (ucIndex = 0; ucIndex < PLN_PER_LUN; ucIndex++)
    {
        bAnd &= RDT_BbtIsBadBlock(ucTLun, ucIndex, ulBlk);
    }

    return bAnd;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_AllPlnBlkIsGood(U8 ucTLun, U32 ulBlk)
{
    BOOL bOr = FALSE;
    U8   ucIndex;

    for (ucIndex = 0; ucIndex < PLN_PER_LUN; ucIndex++)
    {
        bOr |= RDT_BbtIsBadBlock(ucTLun, ucIndex, ulBlk);
    }

    return !bOr;
}

LOCAL MCU1_DRAM_TEXT void RDT_PrintBadBlkCnt(void)
{
    U8  ucTLun, ucPln;
    U16 usBlk;
    U32 ulBadBlkCnt;

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            ulBadBlkCnt = 0;
            for (usBlk = 0; usBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlk++)
            {
                if (TRUE == RDT_GetBadBlkBit(ucTLun, ucPln, usBlk))
                {
                    ulBadBlkCnt++;
                }
            }
            DBG_Printf("Tlun:%d Pln:%d BadBlkCnt:%d\n", ucTLun, ucPln, ulBadBlkCnt);
        }
    }

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_PrintAllBadBlk(void)
{
    U8 ucTLun, ucPln;
    U16 usBlk;
    U32 ulBadBlkCnt;
    U32 ucIndex;
    U32 ulRdtPayLoadAddr;
    RDT_PAYLOAD_ENTRY *pRdtPayloadEntry;

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        ulBadBlkCnt = 0;
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            for (usBlk = 0; usBlk < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlk++)
            {
                if (TRUE == RDT_GetBadBlkBit(ucTLun, ucPln, usBlk))
                {
                    ulBadBlkCnt++;
                }
            }
        }
        ulRdtPayLoadAddr = l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + l_ptRdtCtrl->usLocalRdtOffset + sizeof(RDT_HEAD);
        for (ucIndex = 0; ucIndex < ulBadBlkCnt; ucIndex++)
        {
            pRdtPayloadEntry = (RDT_PAYLOAD_ENTRY *)(ulRdtPayLoadAddr + ucIndex * sizeof(RDT_PAYLOAD_ENTRY));
            switch (pRdtPayloadEntry->ucErrType)
            {
            case RDT_ERASE_ERR:
                DBG_Printf("Tlun=%d Pln=%d Blk=%d Stage=%d RDT_ERASE_ERR.\n", ucTLun, pRdtPayloadEntry->ucPln, pRdtPayloadEntry->usBlk, pRdtPayloadEntry->ucStage, pRdtPayloadEntry->ucErrType);
                break;
            case RDT_WRITE_ERR:
                DBG_Printf("Tlun=%d Pln=%d Blk=%d Stage=%d RDT_WRITE_ERR.\n", ucTLun, pRdtPayloadEntry->ucPln, pRdtPayloadEntry->usBlk, pRdtPayloadEntry->ucStage, pRdtPayloadEntry->ucErrType);
                break;
            case RDT_READ_ERR:
                DBG_Printf("Tlun=%d Pln=%d Blk=%d Stage=%d RDT_READ_ERR.\n", ucTLun, pRdtPayloadEntry->ucPln, pRdtPayloadEntry->usBlk, pRdtPayloadEntry->ucStage, pRdtPayloadEntry->ucErrType);
                break;
            case RDT_IDB_ERR:
                DBG_Printf("Tlun=%d Pln=%d Blk=%d Stage=%d RDT_IDB_ERR.\n", ucTLun, pRdtPayloadEntry->ucPln, pRdtPayloadEntry->usBlk, pRdtPayloadEntry->ucStage, pRdtPayloadEntry->ucErrType);
                break;
            default:
                //DBG_Printf("Tlun=%d Pln=%d Blk=%d Stage=%d ErrTypeInvalid=%d.\n", ucTLun, pRdtPayloadEntry->ucPln, pRdtPayloadEntry->usBlk, pRdtPayloadEntry->ucStage, pRdtPayloadEntry->ucErrType);
                DBG_Getch();
                break;
            }
        }
    }

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_ClearWriteStatusAddrBuf(U8 ucTLun)
{
    U8 ucPrgWl, ucPrgCycle;

    for (ucPrgWl = 0; ucPrgWl < PG_PER_WL; ucPrgWl++)
    {
        for (ucPrgCycle = 0; ucPrgCycle < 3; ucPrgCycle++)
        {
            l_aStatusAddr[ucTLun][ucPrgWl % PG_PER_WL][ucPrgCycle] = 0;
        }
    }

    return;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_CalculateBitMapOffSet(U16 usBlkCnt, U8 ucPlnCnt)
{
    U16 usExtend, usLocalBbtSz;

    /*byte align*/
    usExtend = (((usBlkCnt * ucPlnCnt) % 8) ? 1 : 0);
    usLocalBbtSz = ((usBlkCnt * ucPlnCnt) / 8) + usExtend;

    /*dword align*/
    usExtend = ((usLocalBbtSz % 4) ? 1 : 0);
    usLocalBbtSz = (usLocalBbtSz / 4 + usExtend) * 4;

    return usLocalBbtSz;
}

LOCAL MCU1_DRAM_TEXT void RDT_TestMgInit(void)
{
    U8 ucTLun;
    RDT_TEST_MANAGER *ptTLunMg;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptTLunMg = &l_ptRdtTestMg[ucTLun];

        ptTLunMg->CurStage      = RDT_TEST_INIT;
        ptTLunMg->FlashSLCMode  = FALSE; // default tlc operation
        ptTLunMg->FlashSingePln = FALSE; // default multi-pln operation
        ptTLunMg->Blk           = INVALID_4F;
        ptTLunMg->Pln           = INVALID_2F;
        ptTLunMg->StartBlk      = l_ptRdtCtrl->usStartBlk;
        ptTLunMg->LastBlk       = l_ptRdtCtrl->usLastBlk;
        ptTLunMg->LastPage      = LOGIC_PG_PER_BLK - 1; // control the write page number
        ptTLunMg->PattenValue   = l_ptRdtCtrl->tRdtHead.ucPattenVal[l_ptRdtCtrl->ulRdtPatten];
    }

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_DramVarInit(U32 *FreeDramBase, U32 *ulDramSize)
{
    ASSERT(FreeDramBase != NULL);
    ASSERT(ulDramSize   != NULL);

    U32 ulFreeDramBase = *FreeDramBase;
    U32 ucIndex;

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulRdtDummyDataAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ * SUBSYSTEM_LUN_NUM * NFCQ_DEPTH);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulRdtDummyDataAddrFor2DTlcWrite = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ * SUBSYSTEM_LUN_NUM * DSG_BUFF_SIZE * (PG_PER_WL + 1));

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulRdtDummySpareAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RED) * PG_PER_WL * SUBSYSTEM_LUN_NUM * NFCQ_DEPTH);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulRdtDummySpareAddrFor2DTlcWrite = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RED) * SUBSYSTEM_LUN_NUM * DSG_BUFF_SIZE * (PG_PER_WL + 1));

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulRdtDummyStatusAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(U8) * SUBSYSTEM_LUN_NUM * NFCQ_DEPTH);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ptRdtCtrl = (RDT_CTRL *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RDT_CTRL));

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ptRdtSortMg = (RDT_SORT_MANAGER *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RDT_SORT_MANAGER) * SUBSYSTEM_LUN_NUM);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ptRdtTestMg = (RDT_TEST_MANAGER *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, sizeof(RDT_TEST_MANAGER) * SUBSYSTEM_LUN_NUM);

    for (ucIndex = 0; ucIndex < SUBSYSTEM_LUN_NUM; ucIndex++)
    {
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
        l_ptRdtCtrl->ulLocalBbtAddr[ucIndex] = ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, LOGIC_PIPE_PG_SZ * RDT_PG_NUM * (PLN_PER_LUN - 1));
    }

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_aBuffAddr = (U32(*)[DSG_BUFF_SIZE * PG_PER_WL])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_LUN_NUM * sizeof(l_aBuffAddr[0])));

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_aRedAddr = (U32(*)[DSG_BUFF_SIZE * PG_PER_WL])ulFreeDramBase;

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_aStatusAddr = (U32(*)[DSG_BUFF_SIZE][PG_PER_WL])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_LUN_NUM * sizeof(l_aStatusAddr[0])));

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    l_ulTotalPrgOrder = (U32(*))ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_LUN_NUM * sizeof(l_aStatusAddr[0])));

    if ((ulFreeDramBase - *FreeDramBase) > *ulDramSize)
    {
        DBG_Printf("RDT Dram size overflow!\n");
        DBG_Getch();
    }

    DBG_Printf("MCU#%d RDT_Test Alloc Dram [0x%x,0x%x] Size: 0x%x\n",
        HAL_GetMcuId(), *FreeDramBase, ulFreeDramBase, ulFreeDramBase - *FreeDramBase);

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_OtfbVarInit(U32 *pCacheStatusBase, U32 *ulOtfbSize)
{
    ASSERT(pCacheStatusBase != NULL);
    ASSERT(ulOtfbSize != NULL);

    U32 ulCacheStatusOTFBBase = *pCacheStatusBase;

    COM_MemAddr16DWAlign(&ulCacheStatusOTFBBase);
    l_ulRdtDummyStatusAddrOTFB = ulCacheStatusOTFBBase;
    COM_MemIncBaseAddr(&ulCacheStatusOTFBBase, sizeof(U8)*SUBSYSTEM_LUN_NUM * NFCQ_DEPTH);

    if ((ulCacheStatusOTFBBase - *pCacheStatusBase) > *ulOtfbSize)
    {
        DBG_Printf("RDT OTFB size overflow!\n");
        DBG_Getch();
    }

    DBG_Printf("MCU#%d RDT_Test Alloc OTFB [0x%x,0x%x] Size: 0x%x\n",
        HAL_GetMcuId(), *pCacheStatusBase, ulCacheStatusOTFBBase, ulCacheStatusOTFBBase - *pCacheStatusBase);

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_MemInit(void)
{
    RDT_DramVarInit(&g_ulRdtDramAddr, &g_ulRdtDramAddrSize);
    RDT_OtfbVarInit(&g_ulRdtOtfbAddr, &g_ulRdtOtfbAddrSize);

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_FormatDataSpareBuf(void)
{
    COM_MemZero((U32*)l_ulRdtDummyDataAddr, LOGIC_PIPE_PG_SZ * SUBSYSTEM_LUN_NUM * NFCQ_DEPTH / 4);
    COM_MemZero((U32*)l_ulRdtDummySpareAddr, sizeof(RED) * PG_PER_WL * SUBSYSTEM_LUN_NUM * NFCQ_DEPTH / 4);
    COM_MemZero((U32*)l_ulRdtDummyDataAddrFor2DTlcWrite, LOGIC_PIPE_PG_SZ * SUBSYSTEM_LUN_NUM * DSG_BUFF_SIZE * (PG_PER_WL + 1) / 4);
    COM_MemZero((U32*)l_ulRdtDummySpareAddrFor2DTlcWrite, sizeof(RED) * SUBSYSTEM_LUN_NUM * DSG_BUFF_SIZE * (PG_PER_WL + 1) / 4);

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_FormatProOrder(void)
{
    COM_MemZero((U32*)l_ulTotalPrgOrder, SUBSYSTEM_LUN_NUM / 4);

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_SortMgInit(void)
{
    COM_MemZero((U32*)l_ptRdtSortMg, sizeof(RDT_SORT_MANAGER) * SUBSYSTEM_LUN_NUM / sizeof(U32));

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_ErrHManagerInit(U8 ucTLun)
{
    RDT_TEST_MANAGER *ptDptrMg;

    ptDptrMg = &l_ptRdtTestMg[ucTLun];

    ptDptrMg->ErrhPage = 0;
    ptDptrMg->ErrhStage = RDT_TEST_ERASE;

    return;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_GetDataBuffAddr(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulRdtDummyDataAddr + (NFCQ_DEPTH*ucTLun + ulWPtr)*LOGIC_PIPE_PG_SZ;

    return ulBuffAddr;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_GetDataBuffAddrFor2DTlcWrite(U8 ucTLun, U16 usPrgWL, U8 ucPageIndex)
{
    U32 ulBuffAddr;

    ulBuffAddr = l_ulRdtDummyDataAddrFor2DTlcWrite + (2 * NFCQ_DEPTH * ucTLun + 2 * (usPrgWL % NFCQ_DEPTH) + ucPageIndex)*LOGIC_PIPE_PG_SZ;

    return ulBuffAddr;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_GetSpareBuffAddr(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulRdtDummySpareAddr + (NFCQ_DEPTH*ucTLun + ulWPtr)*sizeof(RED)*PG_PER_WL;

    return ulBuffAddr;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_GetSpareBuffAddrFor2DTlcWrite(U8 ucTLun, U16 usPrgWL, U8 ucPageIndex)
{
    U32 ulBuffAddr;

    ulBuffAddr = l_ulRdtDummySpareAddrFor2DTlcWrite + (2 * NFCQ_DEPTH * ucTLun + 2 * (usPrgWL%NFCQ_DEPTH) + ucPageIndex)*sizeof(RED);

    return ulBuffAddr;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_GetStatusBuffAddr(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulRdtDummyStatusAddr + (NFCQ_DEPTH*ucTLun + ulWPtr)*sizeof(U8);

    return ulBuffAddr;
}

LOCAL MCU1_DRAM_TEXT U32 RDT_GetStatusBuffAddrOTFB(U8 ucTLun)
{
    U32 ulWPtr, ulBuffAddr;

    ulWPtr = L2_FCMDQGetReqWptr(ucTLun, 0);
    ulBuffAddr = l_ulRdtDummyStatusAddrOTFB + (NFCQ_DEPTH*ucTLun + ulWPtr)*sizeof(U8);

    return ulBuffAddr;
}

LOCAL MCU1_DRAM_TEXT void RDT_Add_Bbt(U8 ucTLun, U8 ucPln, U16 usBlock, ERR_TYPE Err_Type)
{
    U32 ulBitPos, ulBytePos, ulTgtAddr;
    RDT_PAYLOAD_ENTRY *ptRdtPayLoadEntry;

    if (TRUE == RDT_BbtIsBadBlock(ucTLun, ucPln, usBlock))
    {
        return;
    }

    //Add Bbt
    ulBitPos = ucPln * (BLK_PER_PLN + RSV_BLK_PER_PLN) + usBlock;
    ulBytePos = l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + ulBitPos / 8;
    *(volatile U8 *)ulBytePos |= (1 << (ulBitPos % 8));

    //Add Rdt Payload
    ulTgtAddr = l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + l_ptRdtCtrl->usLocalRdtOffset + sizeof(RDT_HEAD) + l_ptRdtCtrl->usBadBlkCnt[ucTLun] * sizeof(RDT_PAYLOAD_ENTRY);
    ptRdtPayLoadEntry = (RDT_PAYLOAD_ENTRY *)(ulTgtAddr);
    ptRdtPayLoadEntry->ucPln = ucPln;
    ptRdtPayLoadEntry->ucStage = l_ptRdtCtrl->tRdtHead.ucStage;
    ptRdtPayLoadEntry->usBlk = usBlock;
    ptRdtPayLoadEntry->ucErrType = Err_Type;

    //Add bad block count
    l_ptRdtCtrl->usBadBlkCnt[ucTLun]++;
    DBG_Printf("RDT_Add_Bbt TLun:%d pln:%d blk:%d ErrType:%d\n", ucTLun, ucPln, usBlock, Err_Type);

    //Check bad block count
    if (l_ptRdtCtrl->usBadBlkCnt[ucTLun] >= l_ptRdtCtrl->tRdtHead.usBadBlkThres)
    {
        l_ptRdtCtrl->RdtTlunStat[ucTLun] = RDT_FAIL;
        /*if one TLun RDT_FAIL(BadBlkCnt > Thres), stop RDT_TEST and Save Result.*/
        if (l_ptRdtCtrl->tRdtHead.ucFailStop == TRUE)
        {
            DBG_Printf("TLun%d Bad Blk Cnt%d > Thres%d, loop stop ,save result!\n", ucTLun, l_ptRdtCtrl->usBadBlkCnt[ucTLun], l_ptRdtCtrl->tRdtHead.usBadBlkThres);
            l_bFinishRDTTest = TRUE;
        }
    }
    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_Add_ErrH(U8 ucTLun, U16 usBlk, U16 usPage, U16 usReqType)
{
    RDT_TEST_MANAGER *ptDptrMg;

    ptDptrMg = &l_ptRdtTestMg[ucTLun];

    if (usBlk != ptDptrMg->Blk)
    {
        return;
    }

    switch (usReqType)
    {
    case FCMD_REQ_TYPE_ERASE:
        ptDptrMg->ErrhPage = 0;
        ptDptrMg->ErrhStage = RDT_TEST_WRITE;
        ptDptrMg->FlashSingePln = TRUE;
        break;

    case FCMD_REQ_TYPE_WRITE:
        if (usPage >= l_ptRdtTestMg[ucTLun].LastPage + 1)
        {
            ptDptrMg->ErrhPage = 0;
            ptDptrMg->ErrhStage = RDT_TEST_READ;
        }
        else
        {
            if (LOW_PAGE_WITHOUT_HIGH == HAL_GetFlashPairPageType(usPage))
            {
                ptDptrMg->ErrhPage = usPage + 1;
                ptDptrMg->ErrhStage = RDT_TEST_WRITE;
            }
            else
            {
                ptDptrMg->ErrhPage = usPage + 2;
                ptDptrMg->ErrhStage = RDT_TEST_WRITE;
            }
        }
        ptDptrMg->FlashSingePln = TRUE;
        break;

    case FCMD_REQ_TYPE_READ:
        if (usPage >= l_ptRdtTestMg[ucTLun].LastPage + 1)
        {
            ptDptrMg->ErrhPage = INVALID_4F;
            ptDptrMg->ErrhStage = RDT_TEST_READ;
        }
        else
        {
            ptDptrMg->ErrhPage = (usPage >= 3) ? (usPage - 3) : 0;
            ptDptrMg->ErrhStage = RDT_TEST_READ;
        }
        ptDptrMg->FlashSingePln = TRUE;
        break;

    default:
        DBG_Getch();
        break;
    }

    return;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_IsIdbBadBlk(U32 ulBufAddr) //Single plane read all sector
{
    U32 ulByteOffsetOfMarkCw;
    U32 ulByteOffsetInCw;
    U32 ulByteOffsetInBuf;
    BOOL bBadBlk;
    U8 ucCw;

    /*1. Calculation which CW the bad blk mark is in */
    ucCw = BAD_BLK_MARK_COLUMN_POS / (CW_INFO_SZ + LDPC_MAT_PRT_LEN_FST_15K);

    /*2. Calculation the offset of marked Cw in page */
    if (0 == ucCw)
    {
        ulByteOffsetOfMarkCw = 0;
    }
    else
    {
        volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;

        ulByteOffsetOfMarkCw = (CW_INFO_SZ + LDPC_MAT_PRT_LEN_FST_15K) * ucCw;
        if (pDsReg->atDSEntry[DS_ENTRY_SEL].bsNCRCEn)
        {
            ulByteOffsetOfMarkCw = ulByteOffsetOfMarkCw + 2 * ucCw;
        }
        if (pDsReg->atDSEntry[DS_ENTRY_SEL].bsLbaEn)
        {
            ulByteOffsetOfMarkCw = ulByteOffsetOfMarkCw + 4 * (ucCw * CW_INFO_SZ / LPN_SIZE);
        }
    }

    /*3. Calculation the offset of bad block mark positon in last marked Cw*/
    ulByteOffsetInCw = BAD_BLK_MARK_COLUMN_POS - ulByteOffsetOfMarkCw;

    /*4. Calculation the location of bad block mark in buffer*/
    ulByteOffsetInBuf = ulByteOffsetInCw + CW_INFO_SZ * 2 * ucCw + ulBufAddr - 1;

    DBG_Printf("*(IDb-1)=0x%x\n", *(U8 *)(ulByteOffsetInBuf - 1));
    DBG_Printf("*(IDb)=0x%x\n", *(U8 *)(ulByteOffsetInBuf));
    DBG_Printf("*(IDb+1)=0x%x\n", *(U8 *)(ulByteOffsetInBuf + 1));

    if (BAD_BLK_IDB_MARK == (*(U8 *)ulByteOffsetInBuf & 0xFF))
    {
        bBadBlk = TRUE;
    }
    else
    {
        bBadBlk = FALSE;
    }

    return bBadBlk;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_WaitAllOpStageDone(U32 ulFinishStage)
{
    U8 ucTLun1, ucTLun2, ucFinishCnt;
    BOOL bDone = FALSE;
    RDT_SORT_MANAGER *ptDptrMg1, *ptDptrMg2;

    for (ucTLun1 = 0, ucFinishCnt = 0; ucTLun1 < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun1++)
    {
        ptDptrMg1 = &l_ptRdtSortMg[ucTLun1];
        if (ulFinishStage == ptDptrMg1->bsOpStage)
        {
            ucFinishCnt++;
            if (l_ptRdtCtrl->ulRdtTestLunNumCnt == ucFinishCnt)
            {   // wait all lbbt-manager-op-stage to finish, then reset them to init.
                bDone = TRUE;
                for (ucTLun2 = 0; ucTLun2 < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun2++)
                {
                    ptDptrMg2 = &l_ptRdtSortMg[ucTLun2];

                    ptDptrMg2->bsOpStage = 0;
                }
                break;
            }
        }
    }

    return bDone;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_Idb(void)
{
    U8 ucTLun;
    RDT_SORT_MANAGER *ptIdbMg;
    FCMD_REQ_ENTRY *ptReqEntry;
    BOOL bRdtIdbDone = FALSE;

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        ptIdbMg = &l_ptRdtSortMg[ucTLun];
        switch (ptIdbMg->bsOpStage)
        {
        case RDT_SORT_INIT:
        {
            if (FALSE == l_ptRdtCtrl->tRdtHead.ucIdbEnable)
            {
                return TRUE;
            }
            ASSERT(TRUE == l_ptRdtCtrl->tRdtHead.ucIdbEnable);
            ASSERT(FALSE == l_ptRdtCtrl->tRdtHead.ucInherit);
            ptIdbMg->bsBlk = 1;
            ptIdbMg->bsPage = 0;  //Bad block mark position
            ptIdbMg->bsPln = 0;

            ptIdbMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddrOTFB(ucTLun);

            ptIdbMg->DataBuffAddr = RDT_GetDataBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptIdbMg->DataBuffAddr, LOGIC_PIPE_PG_SZ / sizeof(U32));

            ptIdbMg->SpareBuffAddr = RDT_GetSpareBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptIdbMg->SpareBuffAddr, sizeof(RED) / sizeof(U32));

            ptIdbMg->bsOpStage = RDT_SORT_READ;
            break;
        }
        case RDT_SORT_READ:
        {
            if (TRUE != L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->tLocalDesc.bsRawRdCmd = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptIdbMg->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptIdbMg->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptIdbMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

            ptReqEntry->atBufDesc[0].bsBufID = COM_GetBufferIDByMemAddr(ptIdbMg->DataBuffAddr, TRUE, LOGIC_PIPE_PG_SZ_BITS);
            ptReqEntry->atBufDesc[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart;
            ptReqEntry->atBufDesc[0].bsSecLen = ptReqEntry->tFlashDesc.bsSecLen;
            ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

            ptReqEntry->ulSpareAddr = (U32)ptIdbMg->SpareBuffAddr;

            *(U8 *)ptIdbMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptIdbMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            //DBG_Printf("IDB Tlun%d Blk%d Pln%d Page%d read\n", ucTLun, ptIdbMg->bsBlk, ptIdbMg->bsPln, ptIdbMg->bsPage);
            ptIdbMg->bsOpStage = RDT_SORT_READ_WAIT;
            break;
        }
        case RDT_SORT_READ_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *(U8*)ptIdbMg->FlashStatusBuffAddr)
            {
                if (SUBSYSTEM_STATUS_SUCCESS == *(U8*)ptIdbMg->FlashStatusBuffAddr)
                {
                    if (TRUE == RDT_IsIdbBadBlk(ptIdbMg->DataBuffAddr))
                    {
                        RDT_Add_Bbt(ucTLun, ptIdbMg->bsPln, ptIdbMg->bsBlk, RDT_IDB_ERR);
                    }
                }
                else
                {
                    RDT_Add_Bbt(ucTLun, ptIdbMg->bsPln, ptIdbMg->bsBlk, RDT_IDB_ERR);
                }

                ptIdbMg->bsBlk++;
                ptIdbMg->bsOpStage = RDT_SORT_READ;

                if (ptIdbMg->bsBlk >= BLK_PER_PLN + RSV_BLK_PER_PLN)
                {
                    ptIdbMg->bsPln++;
                    ptIdbMg->bsBlk = 1;

                    if (ptIdbMg->bsPln >= PLN_PER_LUN)
                    {
                        ptIdbMg->bsPln = 0;
                        ptIdbMg->bsBlk = 1;
                        if (0 == ptIdbMg->bsPage)
                        {
                            ptIdbMg->bsPage = PG_PER_BLK - 1;
                            ptIdbMg->bsOpStage = RDT_SORT_READ;
                        }
                        else
                        {
                            ptIdbMg->bsPage = 0;
                            ptIdbMg->bsOpStage = RDT_SORT_FINISH;
                        }
                    }
                }
            }
            break;
        }
        case RDT_SORT_FINISH:
        {
            if (TRUE == RDT_WaitAllOpStageDone(RDT_SORT_FINISH))
            {
                return TRUE; // return immediately.
            }
            break;
        }
        default:
        {
            //DBG_Printf("MCU#%d TLun#%d IDB Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptIdbMg->bsOpStage);
            RDT_CancelTimer();
            RDT_Led_Off();
            DBG_Getch();
        }
        }
    }

    return bRdtIdbDone;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_RdtPageInit(void)
{
    U8 ucTLun;
    U32 ulRdtHeaderAddr;
    RDT_HEAD *ptRdtHead;
    RDT_SORT_MANAGER *ptInitMg;
    BOOL bRdtInitRdtPage = FALSE;
    FCMD_REQ_ENTRY *ptReqEntry;

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        ptInitMg = &l_ptRdtSortMg[ucTLun];
        switch (ptInitMg->bsOpStage)
        {
        case RDT_SORT_INIT:
        {
            ptInitMg->bsBlk = 0;
            ptInitMg->bsPage = 0;
            ptInitMg->bsPln = 1;

            ptInitMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddrOTFB(ucTLun);

            ptInitMg->DataBuffAddr = RDT_GetDataBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptInitMg->DataBuffAddr, LOGIC_PG_SZ / sizeof(U32));

            ptInitMg->SpareBuffAddr = RDT_GetSpareBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptInitMg->SpareBuffAddr, sizeof(RED) / sizeof(U32));

            if (0 == l_ptRdtCtrl->tRdtHead.ucStage)
            {
                ptInitMg->bsOpStage = RDT_SORT_ERASE;
            }
            else
            {
                ptInitMg->bsOpStage = RDT_SORT_READ;
            }
            break;
        }
        case RDT_SORT_READ:
        {
            if (TRUE != L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptInitMg->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptInitMg->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptInitMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

            ptReqEntry->atBufDesc[0].bsBufID = COM_GetBufferIDByMemAddr(ptInitMg->DataBuffAddr, TRUE, LOGIC_PG_SZ_BITS);
            ptReqEntry->atBufDesc[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart + LOGIC_PG_SZ * (ptInitMg->bsPage % RDT_PG_NUM);
            ptReqEntry->atBufDesc[0].bsSecLen = ptReqEntry->tFlashDesc.bsSecLen;
            ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

            ptReqEntry->ulSpareAddr = (U32)ptInitMg->SpareBuffAddr;

            *(U8 *)ptInitMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptInitMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            ptInitMg->bsOpStage = RDT_SORT_READ_WAIT;
            break;
        }
        case RDT_SORT_READ_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *(U8 *)ptInitMg->FlashStatusBuffAddr)
            {
                if (SUBSYSTEM_STATUS_EMPTY_PG == *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    if (0 == ptInitMg->bsPage && 1 == ptInitMg->bsPln)
                    {
                        ptInitMg->bsOpStage = RDT_SORT_ERASE;
                        break;
                    }
                    l_ptRdtCtrl->usRdtPln[ucTLun] = ptInitMg->bsPln;
                    l_ptRdtCtrl->ucRdtPage[ucTLun] = ptInitMg->bsPage;
                    //DBG_Printf("RDT Page location  Tlun:%d Pln:%d Blk:%d Page:%d\n", ucTLun, ptInitMg->bsPln, ptInitMg->bsBlk, ptInitMg->bsPage);
                    /*Set the Bad block count and Pu status, according to the bbt inherited*/
                    ulRdtHeaderAddr = l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + l_ptRdtCtrl->usLocalRdtOffset;
                    ptRdtHead = (RDT_HEAD*)ulRdtHeaderAddr;
                    l_ptRdtCtrl->usBadBlkCnt[ucTLun] = ptRdtHead->usBadBlkCnt;
                    if (l_ptRdtCtrl->usBadBlkCnt[ucTLun] >= l_ptRdtCtrl->tRdtHead.usBadBlkThres)
                    {
                        l_ptRdtCtrl->RdtTlunStat[ucTLun] = RDT_FAIL;
                        if (l_ptRdtCtrl->tRdtHead.ucFailStop == TRUE)
                        {
                            DBG_Printf("TLun%d Inherit Bad Blk Cnt%d > Thres%d, jump RDT test ,save result!\n", ucTLun, l_ptRdtCtrl->usBadBlkCnt[ucTLun], l_ptRdtCtrl->tRdtHead.usBadBlkThres);
                            l_bFinishRDTTest = TRUE;
                        }
                    }
                    ptInitMg->bsOpStage = RDT_SORT_FINISH;
                }

                if (SUBSYSTEM_STATUS_SUCCESS == *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    if (TRUE == l_ptRdtCtrl->tRdtHead.ucInherit)
                    {
                        COM_MemCpy((U32 *)(l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + LOGIC_PG_SZ * (ptInitMg->bsPage % RDT_PG_NUM)),
                            (U32 *)(ptInitMg->DataBuffAddr + LOGIC_PG_SZ * (ptInitMg->bsPage % RDT_PG_NUM)), LOGIC_PG_SZ / sizeof(U32));
                    }
                    ptInitMg->bsPage++;
                    ptInitMg->bsOpStage = RDT_SORT_READ;

                    if (ptInitMg->bsPage == (PG_PER_BLK - (PG_PER_BLK % RDT_PG_NUM)))
                    {
                        ptInitMg->bsPln++;
                        ptInitMg->bsPage = 0;
                        if (PLN_PER_LUN == ptInitMg->bsPln)
                        {
                            ptInitMg->bsPln = 0;
                            l_ptRdtCtrl->usRdtPln[ucTLun] = 1;
                            l_ptRdtCtrl->ucRdtPage[ucTLun] = 0;

                            ptInitMg->bsOpStage = RDT_SORT_ERASE;
                        }
                    }
                }

                if (SUBSYSTEM_STATUS_FAIL == *(U8 *)ptInitMg->FlashStatusBuffAddr || SUBSYSTEM_STATUS_RECC == *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    RDT_Add_Bbt(ucTLun, ptInitMg->bsPln, ptInitMg->bsBlk, RDT_READ_ERR);
                    ptInitMg->bsPln++;
                    ptInitMg->bsPage = 0;
                    ptInitMg->bsOpStage = RDT_SORT_READ;
                    if (PLN_PER_LUN == ptInitMg->bsPln)
                    {
                        ptInitMg->bsPln = 1;

                        l_ptRdtCtrl->usRdtPln[ucTLun] = 1;
                        l_ptRdtCtrl->ucRdtPage[ucTLun] = 0;

                        ptInitMg->bsOpStage = RDT_SORT_ERASE;
                    }
                }

                if (SUBSYSTEM_STATUS_EMPTY_PG != *(U8 *)ptInitMg->FlashStatusBuffAddr && SUBSYSTEM_STATUS_FAIL != *(U8 *)ptInitMg->FlashStatusBuffAddr && SUBSYSTEM_STATUS_SUCCESS != *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    //DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d RDT-Search RDT Page Read-BBT InvalidStatus:%d.\n", HAL_GetMcuId(), ucTLun, ptInitMg->bsPln, ptInitMg->bsBlk, *(U8 *)ptInitMg->FlashStatusBuffAddr);
                    RDT_CancelTimer();
                    RDT_Led_Off();
                    DBG_Getch();
                }
            }
            break;
        }

        case RDT_SORT_ERASE:
        {
            if (TRUE != L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
            ptReqEntry->bsTableReq = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptInitMg->bsBlk;
            ptReqEntry->tFlashDesc.bsPlnNum = ptInitMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;

            *(U8 *)ptInitMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = ptInitMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            ptInitMg->bsOpStage = RDT_SORT_ERASE_WAIT;
            break;
        }

        case RDT_SORT_ERASE_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *(U8 *)ptInitMg->FlashStatusBuffAddr)
            {
                if (SUBSYSTEM_STATUS_FAIL != *(U8 *)ptInitMg->FlashStatusBuffAddr && SUBSYSTEM_STATUS_SUCCESS != *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    RDT_CancelTimer();
                    RDT_Led_Off();
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_FAIL == *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    RDT_Add_Bbt(ucTLun, ptInitMg->bsPln, ptInitMg->bsBlk, RDT_ERASE_ERR);

                    ptInitMg->bsPln++;
                    ptInitMg->bsOpStage = RDT_SORT_ERASE;
                    if (PLN_PER_LUN == ptInitMg->bsPln)
                    {
                        ptInitMg->bsPln = 1;
                    }
                }

                if (SUBSYSTEM_STATUS_SUCCESS == *(U8 *)ptInitMg->FlashStatusBuffAddr)
                {
                    ptInitMg->bsPln = (ptInitMg->bsPln + 1) % PLN_PER_LUN;
                    ptInitMg->bsOpStage = RDT_SORT_ERASE;
                    if (0 == ptInitMg->bsPln)
                    {
                        l_ptRdtCtrl->usRdtPln[ucTLun] = 1;
                        l_ptRdtCtrl->ucRdtPage[ucTLun] = 0;
                        //DBG_Printf("RDT Page location  Tlun:%d Pln:%d Blk:%d Page:%d\n", ucTLun, ptInitMg->bsPln, ptInitMg->bsBlk, ptInitMg->bsPage);
                        ptInitMg->bsOpStage = RDT_SORT_FINISH;
                    }
                }
            }
            break;
        }

        case RDT_SORT_FINISH:
        {
            if (TRUE == RDT_WaitAllOpStageDone(RDT_SORT_FINISH))
            {
                return TRUE; // return immediately.
            }
            break;
        }
        default:
        {
            //DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d RDT-Search RDT Page Read-BBT Invalid Stage:%d.\n", HAL_GetMcuId(), ucTLun, ptInitMg->bsPln, ptInitMg->bsBlk, ptInitMg->bsOpStage);
            RDT_CancelTimer();
            RDT_Led_Off();
            DBG_Getch();
            break;
        }
        }
    }
    return bRdtInitRdtPage;
}

LOCAL MCU1_DRAM_TEXT void RDT_Init(void)
{
    U8      ucTLun;
    PTABLE  *pPtable;

    /*LED start*/
    RDT_InitTimer();
    RDT_StartTimer();

    /*get the Rdt header from ptable*/
    pPtable = HAL_GetPTableAddr();
    COM_MemCpy((U32 *)&l_ptRdtCtrl->tRdtHead, (U32 *)pPtable->tRdtTable, sizeof(RDT_HEAD) / sizeof(U32));
    l_ulLedRunCount_Sec = 0;

#ifdef SIM
    //dword0
    l_ptRdtCtrl->tRdtHead.ucStage = 0;
    l_ptRdtCtrl->tRdtHead.ucIdbEnable = 0;
    l_ptRdtCtrl->tRdtHead.ucInherit = 0;
    l_ptRdtCtrl->tRdtHead.ucFailStop = FALSE;

    //dword1
    l_ptRdtCtrl->tRdtHead.ulReccValue = 60;

    //dword2
    l_ptRdtCtrl->tRdtHead.usBeginBlk = 1;
    l_ptRdtCtrl->tRdtHead.usTestBlkCnt = 2; // BLK_PER_PLN + RSV_BLK_PER_PLN - 1

    //dword3
    l_ptRdtCtrl->tRdtHead.ucBeginPln = 0;
    l_ptRdtCtrl->tRdtHead.ucTestPlnCnt = PLN_PER_LUN;;

    //dword4
    l_ptRdtCtrl->tRdtHead.usLoopCnt = 1;
    l_ptRdtCtrl->tRdtHead.ucPattenCnt = 1;
    l_ptRdtCtrl->tRdtHead.ucSlcMode = FALSE;

    //dword5
    l_ptRdtCtrl->tRdtHead.usBadBlkThres = 1;

    //dword6~dword9
    l_ptRdtCtrl->tRdtHead.ucPattenVal[0] = 0X5A;
    l_ptRdtCtrl->tRdtHead.ucPattenVal[1] = 0XA5;

    //dword10
    l_ptRdtCtrl->tRdtHead.ucRdtLogSizeInPage = 1;
    l_ptRdtCtrl->tRdtHead.ucByPassRDTEn = 0;
#endif

    DBG_Printf(" stage=%d\n Idb=%d\n Inherit=%d\n FailStop=%d\n Recc=%d\n BeginBlk=%d\n TestBlkCnt=%d\n BeginPln=%d\n TestPln=%d\n Loop=%d\n ucPattenCnt=%d\n SlcMode=%d\n BadBlkCnt=%d\n BadBlkThres=%d\n ucPattenVal=%d\n",
        l_ptRdtCtrl->tRdtHead.ucStage, l_ptRdtCtrl->tRdtHead.ucIdbEnable, l_ptRdtCtrl->tRdtHead.ucInherit, l_ptRdtCtrl->tRdtHead.ucFailStop, l_ptRdtCtrl->tRdtHead.ulReccValue, l_ptRdtCtrl->tRdtHead.usBeginBlk,
        l_ptRdtCtrl->tRdtHead.usTestBlkCnt, l_ptRdtCtrl->tRdtHead.ucBeginPln, l_ptRdtCtrl->tRdtHead.ucTestPlnCnt, l_ptRdtCtrl->tRdtHead.usLoopCnt, l_ptRdtCtrl->tRdtHead.ucPattenCnt, l_ptRdtCtrl->tRdtHead.ucSlcMode,
        l_ptRdtCtrl->tRdtHead.usBadBlkCnt, l_ptRdtCtrl->tRdtHead.usBadBlkThres, l_ptRdtCtrl->tRdtHead.ucPattenVal[0]);

    /*Check Rdt header info */
    if ((l_ptRdtCtrl->tRdtHead.usBeginBlk < 1) || ((l_ptRdtCtrl->tRdtHead.usBeginBlk + l_ptRdtCtrl->tRdtHead.usTestBlkCnt) >(BLK_PER_PLN + RSV_BLK_PER_PLN)))
    {
        DBG_Printf("Blk number is wrony!\n");
        RDT_CancelTimer();
        RDT_Led_Off();
        DBG_Getch();
    }
    if ((l_ptRdtCtrl->tRdtHead.usLoopCnt < 1) || (l_ptRdtCtrl->tRdtHead.ucPattenCnt < 1))
    {
        DBG_Printf("loop/pattern can't less than one!\n");
        RDT_CancelTimer();
        RDT_Led_Off();
        DBG_Getch();
    }
    if (l_ptRdtCtrl->tRdtHead.ucStage == 0 && l_ptRdtCtrl->tRdtHead.ucInherit == TRUE)
    {
        DBG_Printf("T1 stage can not inherit!\n");
        RDT_CancelTimer();
        RDT_Led_Off();
        DBG_Getch();
    }
    if (l_ptRdtCtrl->tRdtHead.ucStage == 1 && l_ptRdtCtrl->tRdtHead.ucIdbEnable == TRUE)
    {
        DBG_Printf("T2 stage can not do IDB test!\n");
        RDT_CancelTimer();
        RDT_Led_Off();
        DBG_Getch();
    }

    /*Init some setting*/
    g_BootUpOk = FALSE;
    bTBRebuildPending = TRUE;

    /*Set RDT controller*/
    l_ptRdtCtrl->ulRdtRunCycleCnt = l_ptRdtCtrl->tRdtHead.usLoopCnt;
    l_ptRdtCtrl->ulRdtRunCycle = 0;
    l_ptRdtCtrl->ulFinishLunNum = 0;
    l_ptRdtCtrl->ulRdtPattenCnt = l_ptRdtCtrl->tRdtHead.ucPattenCnt;
    l_ptRdtCtrl->ulRdtPatten = 0;
    l_ptRdtCtrl->ulRdtTestLunNumCnt = SUBSYSTEM_LUN_NUM;//SUBSYSTEM_LUN_NUM
#ifdef RDT_EWR_TIME_STATISTIC
    l_ptRdtCtrl->ulRdtTestLunNumCnt = 1;
#endif

    l_ptRdtCtrl->usStartBlk = l_ptRdtCtrl->tRdtHead.usBeginBlk;
    l_ptRdtCtrl->usLastBlk = l_ptRdtCtrl->tRdtHead.usBeginBlk + l_ptRdtCtrl->tRdtHead.usTestBlkCnt - 1;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        l_ptRdtCtrl->RdtTlunStat[ucTLun] = RDT_PASS;
        l_ptRdtCtrl->usBadBlkCnt[ucTLun] = 0;
    }

    /*Calculate the local bbt and single bad block table offset, init the bit map area*/
    l_ptRdtCtrl->usLocalRdtOffset = RDT_CalculateBitMapOffSet(BLK_PER_PLN + RSV_BLK_PER_PLN, PLN_PER_LUN);

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        COM_MemSet((U32 *)l_ptRdtCtrl->ulLocalBbtAddr[ucTLun], LOGIC_PG_SZ * RDT_PG_NUM * (PLN_PER_LUN - 1) / sizeof(U32), 0);
    }

    return;
}

//Select next good plane, and set bSinglePlane signal.
//    if found, return trun;
//    if not found, return FALSE, indicate all planes are bad after plane++.
LOCAL MCU1_DRAM_TEXT BOOL RDT_SelectNextGoodPlane(U8 ucTLun, U8 ucCurPln, U16 usBlk)
{
    ASSERT(ucCurPln < PLN_PER_LUN || ucCurPln == INVALID_2F);
    ASSERT(usBlk <= l_ptRdtTestMg[ucTLun].LastBlk);

    BOOL bSelectNewPln = FALSE;
    U8 ucPln = (ucCurPln == INVALID_2F) ? 0 : (ucCurPln + 1);

    for (ucPln; ucPln < PLN_PER_LUN; ucPln++)
    {
        if (TRUE != RDT_BbtIsBadBlock(ucTLun, ucPln, usBlk))
        {
            l_ptRdtTestMg[ucTLun].Pln = ucPln;
            l_ptRdtTestMg[ucTLun].FlashSingePln = TRUE;
            bSelectNewPln = TRUE;
            break;
        }
    }

    return bSelectNewPln;
}

// Select next good block, and set bSinglePlane signal
// if blk > lastblk, return fasle, indicat didn't find new block.
//   note:
//      1) if found, return TRUE.
//      2) if not found, return FALSE, indicate that all blocks are bad after block++ or block number beyone the range.
LOCAL MCU1_DRAM_TEXT BOOL RDT_SelectNextGoodBlock(U8 ucTLun)
{
    U8 ucPln;
    U16 usBlk, usLastBlk;
    BOOL bSelectNewBlock = TRUE;
    RDT_TEST_MANAGER *ptTLunMg;

    ptTLunMg = &l_ptRdtTestMg[ucTLun];

    //usBlk++; if usBlk==INVALID_4F, usBlk=StartBlk
    ucPln = INVALID_2F;
    usLastBlk = ptTLunMg->LastBlk;
    usBlk = (ptTLunMg->Blk == INVALID_4F) ? ptTLunMg->StartBlk : (ptTLunMg->Blk + 1);
    ptTLunMg->FlashSingePln = FALSE;

    //find good block or plane
    if (usBlk > usLastBlk)
    {
        bSelectNewBlock = FALSE;
    }
    else
    {
        for (usBlk; usBlk <= usLastBlk; usBlk++)
        {
            if (TRUE == RDT_AllPlnBlkIsBad(ucTLun, usBlk))
            {
                continue;
            }
            else if (TRUE == RDT_AllPlnBlkIsGood(ucTLun, usBlk))
            {
                break;
            }
            else
            {
                if (TRUE == RDT_SelectNextGoodPlane(ucTLun, INVALID_2F, usBlk))
                {
                    ptTLunMg->FlashSingePln = TRUE;
                    ucPln = ptTLunMg->Pln;
                    break;
                }
                else
                {
                    DBG_Printf("RDT select blk fail\n");
                    DBG_Getch();
                }
            }
        }

        if (usBlk > usLastBlk)
        {
            bSelectNewBlock = FALSE;
        }
    }

    //Set current block and plane
    ptTLunMg->Blk = usBlk;
    ptTLunMg->Pln = ucPln;

    return bSelectNewBlock;
}

//check current block, select next good block(may be is single plane block), and set next stage.
LOCAL MCU1_DRAM_TEXT void RDT_SelectNextErrHStage(U8 ucTLun)
{
    RDT_TEST_MANAGER *ptTLunMg;

    ptTLunMg = &l_ptRdtTestMg[ucTLun];

    //Check all the plane, select nearest block, and set next stage to continue test.
    if ((TRUE == RDT_SelectNextGoodPlane(ucTLun, ptTLunMg->Pln, ptTLunMg->Blk)))
    {
        ptTLunMg->Page = ptTLunMg->ErrhPage;
        ptTLunMg->CurStage = ptTLunMg->ErrhStage;
        ptTLunMg->FlashSingePln = TRUE;
    }
    else    //if do not find, select a new good block, and set the next stage to erase.
    {
        ptTLunMg->Page = 0;
        ptTLunMg->FlashSingePln = FALSE;
        ptTLunMg->CurStage = RDT_TEST_ERASE;

        if (TRUE != RDT_SelectNextGoodBlock(ucTLun))
        {
            ptTLunMg->CurStage = RDT_TEST_FINISH;
            l_ptRdtCtrl->ulFinishLunNum++;
        }

        RDT_ErrHManagerInit(ucTLun);
    }

    return;
}

//When check error, return FALSE.
LOCAL MCU1_DRAM_TEXT BOOL RDT_Check(U8 ucTLun)
{
    FCMD_REQ_ENTRY *ptReqEntry;
    ERR_TYPE Err_Type;
    U32 ulStatus;
    U8 ucPlnErrBitMap;

    ptReqEntry = L2_FCMDQGetReqEntry(ucTLun, 0, L2_FCMDQGetReqWptr(ucTLun, 0));

    //Check current FCMD if null
    if (0 == ptReqEntry->ulReqStsAddr)
    {
        return TRUE;
    }

    ulStatus = *(U8 *)ptReqEntry->ulReqStsAddr & 0x0f;
    if (SUBSYSTEM_STATUS_PENDING != ulStatus)
    {
        if (SUBSYSTEM_STATUS_SUCCESS == ulStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ulStatus)
        {
            if (TRUE == l_ptRdtTestMg[ucTLun].FlashSingePln)
            {
                if (FCMD_REQ_SUBTYPE_ONEPG == ptReqEntry->bsReqSubType || FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType)
                {
                    RDT_Add_ErrH(ucTLun, ptReqEntry->tFlashDesc.bsVirBlk, ptReqEntry->tFlashDesc.bsVirPage, ptReqEntry->bsReqType);
                }
            }
            return TRUE;
        }
        else if (SUBSYSTEM_STATUS_FAIL == ulStatus) //Erase/Program fail, Read Uecc
        {
            if (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType)
            {
                Err_Type = RDT_READ_ERR;
            }
            else if (FCMD_REQ_TYPE_ERASE == ptReqEntry->bsReqType)
            {
                Err_Type = RDT_ERASE_ERR;
            }
            else if (FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType)
            {
                Err_Type = RDT_WRITE_ERR;
            }
            else
            {
                DBG_Printf("RDT ReqType err!\n");
                RDT_CancelTimer();
                RDT_Led_Off();
                DBG_Getch();
            }

            /*when cmd is normal, set single plane bad signal, to do single plane E/P/R*/
            if (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType || FCMD_REQ_SUBTYPE_ONEPG == ptReqEntry->bsReqSubType)
            {
                ucPlnErrBitMap = *(U8 *)ptReqEntry->ulReqStsAddr >> 4;

#if defined (RDT_ERRH_DISENABLE)
                ucPlnErrBitMap = 0x0f;
#endif
                U8 ucPln;
                for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
                {
                    if (0 != (ucPlnErrBitMap & (1 << ucPln)))
                    {
                        RDT_Add_Bbt(ucTLun, ucPln, ptReqEntry->tFlashDesc.bsVirBlk, Err_Type);//add bbt and payload
                    }
                    else
                    {
                        RDT_Add_ErrH(ucTLun, ptReqEntry->tFlashDesc.bsVirBlk, ptReqEntry->tFlashDesc.bsVirPage, ptReqEntry->bsReqType);
                    }
                }
                return FALSE;
            }/*when cmd is single, add bbt*/
            else if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType || FCMD_REQ_SUBTYPE_SINGLE_ONEPG == ptReqEntry->bsReqSubType || FCMD_REQ_SUBTYPE_SINGLE_TWOPG == ptReqEntry->bsReqSubType)
            {
                RDT_Add_Bbt(ucTLun, ptReqEntry->tFlashDesc.bsPlnNum, ptReqEntry->tFlashDesc.bsVirBlk, Err_Type);//add bbt and payload
                return FALSE;
            }
            else
            {
                DBG_Printf("bsReqSubType error!\n");
                DBG_Getch();
            }

            *(U8 *)ptReqEntry->ulReqStsAddr = SUBSYSTEM_STATUS_SUCCESS;
            ////Clear 2D_TLC Status
            //if (ptTLunMg->Blk == ptReqEntry->tFlashDesc.bsVirBlk)
            //{
            //    RDT_ClearWriteStatusAddrBuf(ucTLun);
            //}
        }
        else if (SUBSYSTEM_STATUS_RECC == ulStatus) //Recc
        {
            Err_Type = RDT_READ_ERR;

            if (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType) //when Read is normal, set single plane bad signal, to do single plane Read
            {
                ucPlnErrBitMap = *(U8 *)ptReqEntry->ulReqStsAddr >> 4;
#if defined (RDT_ERRH_DISENABLE)
                ucPlnErrBitMap = 0x0f;
#endif
                U8 ucPln;
                for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
                {
                    if (0 != (ucPlnErrBitMap & (1 << ucPln)))
                    {
                        RDT_Add_Bbt(ucTLun, ucPln, ptReqEntry->tFlashDesc.bsVirBlk, Err_Type);//add bbt and payload
                    }
                    else
                    {
                        RDT_Add_ErrH(ucTLun, ptReqEntry->tFlashDesc.bsVirBlk, ptReqEntry->tFlashDesc.bsVirPage, ptReqEntry->bsReqType);
                    }
                }
                return FALSE;
            }
            else //when cmd is single, add bbt
            {
                RDT_Add_Bbt(ucTLun, ptReqEntry->tFlashDesc.bsPlnNum, ptReqEntry->tFlashDesc.bsVirBlk, Err_Type);//add bbt and payload
                return FALSE;
            }
            *(U8 *)ptReqEntry->ulReqStsAddr = SUBSYSTEM_STATUS_SUCCESS;
        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == ulStatus)
        {
            DBG_Printf("RDT Chk EMPTY_PG!\n");
            DBG_Getch();
        }
        else
        {
            DBG_Printf("RDT Chk Status%d eror!\n", ulStatus);
            DBG_Getch();
        }
    }
    return TRUE;
}

LOCAL MCU1_DRAM_TEXT void RDT_UpdateRECCThrd(void)
{
    U8 ucThrd = l_ptRdtCtrl->tRdtHead.ulReccValue;
    volatile LDPC_RECC_THRE_REG *pReccReg = (volatile LDPC_RECC_THRE_REG *)LDPC_RECC_THRE_REG_BASE;

    pReccReg->bsReccThr0 = ucThrd;
    pReccReg->bsReccThr1 = ucThrd;
    pReccReg->bsReccThr2 = ucThrd;
    pReccReg->bsReccThr3 = ucThrd;

    return;
}

LOCAL MCU1_DRAM_TEXT void RDT_TurnOffHardDecoder(void)
{
    volatile HARD_DEC_CONF_REG *pHardConfReg = (volatile HARD_DEC_CONF_REG *)LDPC_HARD_CFG_REG_BASE;

    pHardConfReg->bsHardTsbEn = FALSE;

    return;
}


LOCAL MCU1_DRAM_TEXT void RDT_ClearLastFourFCmd(U8 ucCheckTLun)
{
    U8 ucLoop = 0;
    FCMD_REQ_ENTRY *ptReqEntry;

    while (ucLoop < NFCQ_DEPTH)
    {
        while (TRUE != L2_FCMDQNotFull(ucCheckTLun))
        {
            ;
        }
        ptReqEntry = L2_FCMDQGetReqEntry(ucCheckTLun, 0, g_ptFCmdReqDptr->atReqQDptr[ucCheckTLun][0].ucWptr);

        //Clear FCmd status address
        if (NULL != ptReqEntry->ulReqStsAddr)
        {
            *(U8 *)ptReqEntry->ulReqStsAddr = 0;
            ptReqEntry->ulReqStsAddr = NULL;
        }

        g_ptFCmdReqDptr->atReqQDptr[ucCheckTLun][0].ucWptr = (g_ptFCmdReqDptr->atReqQDptr[ucCheckTLun][0].ucWptr + 1) % NFCQ_DEPTH;
        ucLoop++;
    }
    return;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_CheckLastFourFCmd(U8 ucCheckTLun)
{
    U8 ucLoop = 0;
    BOOL bCheck = TRUE;

    while (ucLoop < NFCQ_DEPTH)
    {
        while (TRUE != L2_FCMDQNotFull(ucCheckTLun))
        {
            ;
        }
        if (TRUE != RDT_Check(ucCheckTLun))
        {
            bCheck = FALSE;
        }

        g_ptFCmdReqDptr->atReqQDptr[ucCheckTLun][0].ucWptr = (g_ptFCmdReqDptr->atReqQDptr[ucCheckTLun][0].ucWptr + 1) % NFCQ_DEPTH;
        ucLoop++;
    }
    return bCheck;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_FormatGB(void)
{
    U8 ucTLun;
    RDT_SORT_MANAGER *ptFormatGBMg;
    BOOL bRdtFormatGB = FALSE;
    FCMD_REQ_ENTRY *ptReqEntry;
    LOCAL U8 ucFormatLunNum = 0;

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        ptFormatGBMg = &l_ptRdtSortMg[ucTLun];
        switch (ptFormatGBMg->bsOpStage)
        {
        case RDT_SORT_INIT:
        {
            ptFormatGBMg->bsBlk = 0;
            ptFormatGBMg->bsPln = 0;

            ptFormatGBMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddrOTFB(ucTLun);
            ptFormatGBMg->bsOpStage = RDT_SORT_ERASE;
            break;
        }
        case RDT_SORT_ERASE:
        {
            if (TRUE != L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
            ptReqEntry->bsTableReq = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptFormatGBMg->bsBlk;
            ptReqEntry->tFlashDesc.bsPlnNum = ptFormatGBMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;

            *(U8 *)ptFormatGBMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = ptFormatGBMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            ptFormatGBMg->bsOpStage = RDT_SORT_ERASE_WAIT;
            break;
        }

        case RDT_SORT_ERASE_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *(U8 *)ptFormatGBMg->FlashStatusBuffAddr)
            {
                if (SUBSYSTEM_STATUS_FAIL != *(U8 *)ptFormatGBMg->FlashStatusBuffAddr && SUBSYSTEM_STATUS_SUCCESS != *(U8 *)ptFormatGBMg->FlashStatusBuffAddr)
                {
                    RDT_CancelTimer();
                    RDT_Led_Off();
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_FAIL == *(U8 *)ptFormatGBMg->FlashStatusBuffAddr)
                {
                    DBG_Printf("Tlun:%d RDT Erase GB Fail!\n", ucTLun);
                    RDT_CancelTimer();
                    RDT_Led_Off();
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_SUCCESS == *(U8 *)ptFormatGBMg->FlashStatusBuffAddr)
                {
                    ucFormatLunNum++;
                    ptFormatGBMg->bsOpStage = RDT_SORT_FINISH;
                }
            }
            break;
        }

        case RDT_SORT_FINISH:
        {
            if (l_ptRdtCtrl->ulRdtTestLunNumCnt == ucFormatLunNum)
            {
                bRdtFormatGB = TRUE;
                ptFormatGBMg->bsOpStage = RDT_SORT_INIT;
            }
            break;
        }
        default:
        {
            //DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d RDT-Search RDT Page Read-BBT Invalid Stage:%d.\n", HAL_GetMcuId(), ucTLun, ptFormatGBMg->bsPln, ptFormatGBMg->bsBlk, ptFormatGBMg->bsOpStage);
            RDT_CancelTimer();
            RDT_Led_Off();
            DBG_Getch();
            break;
        }
        }
    }
    return bRdtFormatGB;
}

LOCAL MCU1_DRAM_TEXT U8 RDT_SelectNextStage(U8 ucTLun)
{
    U8 ucNextStage;
    RDT_TEST_MANAGER *ptTLunMg;

    ptTLunMg = &l_ptRdtTestMg[ucTLun];

    switch (ptTLunMg->CurStage)
    {
    case RDT_TEST_INIT:
        RDT_ClearLastFourFCmd(ucTLun);
        RDT_ErrHManagerInit(ucTLun);

        ptTLunMg->Blk = INVALID_4F;
        ptTLunMg->Pln = INVALID_2F;
        ptTLunMg->Page = 0;
        ptTLunMg->CurStage = RDT_TEST_ERASE;
        ptTLunMg->FlashSingePln = FALSE;

        //check first block, if first block is not a good block, select a new good block to test
        if (TRUE != RDT_SelectNextGoodBlock(ucTLun))
        {
            ptTLunMg->CurStage = RDT_TEST_FINISH;
            l_ptRdtCtrl->ulFinishLunNum++;
        }

#ifdef RDT_EWR_TIME_STATISTIC
        ptTLunMg->FlashSingePln = TRUE;
        l_ulTimeTag5 = HAL_GetMCUCycleCount();
#endif

        ucNextStage = ptTLunMg->CurStage;
        break;

    case RDT_TEST_ERASE:
        ptTLunMg->Page = 0;
        ucNextStage = RDT_TEST_WRITE;
#ifdef RDT_EWR_TIME_STATISTIC
        RDT_CheckLastFourFCmd(ucTLun);
        l_ulTimeTag6 = HAL_GetMCUCycleCount();
#endif
        break;

    case RDT_TEST_WRITE:
        ptTLunMg->Page++;
        ucNextStage = RDT_TEST_WRITE;
        if (ptTLunMg->Page > ptTLunMg->LastPage)
        {
            ptTLunMg->Page = 0;
            ucNextStage = RDT_TEST_READ;
#ifdef RDT_EWR_TIME_STATISTIC
            RDT_CheckLastFourFCmd(ucTLun);
            l_ulTimeTag7 = HAL_GetMCUCycleCount();
#endif
        }
        break;

    case RDT_TEST_READ:
        ptTLunMg->Page++;
        ucNextStage = RDT_TEST_READ;
        if (ptTLunMg->Page > ptTLunMg->LastPage)
        {
#ifdef RDT_EWR_TIME_STATISTIC
            RDT_CheckLastFourFCmd(ucTLun);
            l_ulTimeTag8 = HAL_GetMCUCycleCount();
            l_ptRdtCtrl->ulFinishLunNum++;
            ucNextStage = RDT_TEST_FINISH;
            break;
#endif
            //Check Last Four FCMD, if find error, set next stage to do error handle. Clear last four FCMD;
            if (TRUE != RDT_CheckLastFourFCmd(ucTLun))
            {
                ptTLunMg->ErrhPage = ptTLunMg->LastPage - NFCQ_DEPTH + 1;
                ptTLunMg->ErrhStage = RDT_TEST_READ;
                ptTLunMg->FlashSingePln = TRUE;
            }
            RDT_ClearLastFourFCmd(ucTLun);

            ptTLunMg->Page = 0;
            if (FALSE == ptTLunMg->FlashSingePln)
            {
                ucNextStage = RDT_TEST_ERASE;
                if (TRUE != RDT_SelectNextGoodBlock(ucTLun)) //block ++
                {
                    ucNextStage = RDT_TEST_FINISH;
                    l_ptRdtCtrl->ulFinishLunNum++;
                }
            }
            else
            {
                if (TRUE == RDT_SelectNextGoodPlane(ucTLun, ptTLunMg->Pln, ptTLunMg->Blk)) //plane++
                {
                    ptTLunMg->Page = ptTLunMg->ErrhPage;
                    ucNextStage = ptTLunMg->ErrhStage;
                }
                else
                {
                    ucNextStage = RDT_TEST_ERASE;
                    ptTLunMg->FlashSingePln = FALSE;
                    RDT_ErrHManagerInit(ucTLun);
                    if (TRUE != RDT_SelectNextGoodBlock(ucTLun)) //if not found, block++, clear error handle signal.
                    {
                        ucNextStage = RDT_TEST_FINISH;
                        l_ptRdtCtrl->ulFinishLunNum++;
                        ptTLunMg->FlashSingePln = FALSE;
                    }
                }
            }
        }
        break;

    case RDT_TEST_FINISH:
        ucNextStage = RDT_TEST_FINISH;
        break;

    default:
        //DBG_Printf("MCU#%d RDT_SelectNextStage Stage Error %d\n", HAL_GetMcuId(), ptTLunMg->CurStage);
        RDT_CancelTimer();
        RDT_Led_Off();
        DBG_Getch();
        break;
    }

    return ucNextStage;
}

/*RDT Erase Command*/
LOCAL MCU1_DRAM_TEXT void RDT_FlashEraseBlock(U8 ucTLun, RDT_TEST_MANAGER *ptTLunMg)
{
    FCMD_REQ_ENTRY *ptReqEntry;
    U8 *pStatus = (U8*)ptTLunMg->FlashStatusBuffAddr;

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
    ptReqEntry->bsReqType = FCMD_REQ_TYPE_ERASE;
    ptReqEntry->bsReqSubType = ptTLunMg->FlashSingePln ? FCMD_REQ_SUBTYPE_SINGLE : FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->tLocalDesc.bsRdtCmd = TRUE;
    ptReqEntry->bsTableReq = FALSE;

    ptReqEntry->tFlashDesc.bsVirBlk = ptTLunMg->Blk;
    ptReqEntry->tFlashDesc.bsPlnNum = ptTLunMg->Pln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;

    *(U8*)pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

#ifdef RDT_FCMDQ_STATISTIC
    g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_ERASE + 3]++;
    g_aFCMDStatus[ucTLun][7]++;
    if (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_ERASE]++;
        g_aFCMDStatus[ucTLun][6]++;
    }
#endif
    return;
}

/*RDT Write Command*/
LOCAL MCU1_DRAM_TEXT BOOL RDT_FlashPrepareFor2DTlcWrite(U32 ulTLun, RDT_TEST_MANAGER * ptTLunMg)
{
    U32 ulSecIndex;
    U32 ulLPNIndex;
    U32 ulDummyData;
    U32 ulPhyPgNum;
    U32 ulSecNumPerPageForTest;
    RED *pRed = NULL;
    U32 ucPattenVal;
    U8 ucPartIndex;
    U16 usPrgWL;
    U8 ucPrgCycle;
    U8 ucPageIndex;

    ptTLunMg->PrePage = ptTLunMg->Page;

    /*calculate the WL and PrgCycle*/
    usPrgWL = HAL_FlashGetTlcPrgWL(ptTLunMg->Page);
    ucPrgCycle = HAL_FlashGetTlcPrgCycle(ptTLunMg->Page);

    /*allocate flash status and check it before send the next program command.*/
    ptTLunMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ulTLun);
    if (l_aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][ucPrgCycle] == 0)
    {
        l_aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][ucPrgCycle] = ptTLunMg->FlashStatusBuffAddr;
    }
    else
    {
        if (*(U8*)l_aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][2] == SUBSYSTEM_STATUS_PENDING)
        {
            return FALSE;
        }
        else
        {
            l_aStatusAddr[ulTLun][usPrgWL % PG_PER_WL][ucPrgCycle] = ptTLunMg->FlashStatusBuffAddr;
        }
    }

    /*generate the red address and data buffer address for the program command*/
    if (0 == ucPrgCycle)
    {
        for (ucPageIndex = 0; ucPageIndex < DSG_BUFF_SIZE; ucPageIndex++)
        {
            l_aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex] = RDT_GetSpareBuffAddrFor2DTlcWrite(ulTLun, usPrgWL, ucPageIndex);
            COM_MemZero((U32 *)l_aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex], sizeof(RED) / sizeof(U32));

            l_aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, usPrgWL, ucPageIndex);
            COM_MemZero((U32 *)l_aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPageIndex], BUF_SIZE / sizeof(U32));
        }
    }
    ptTLunMg->SpareBuffAddr = (U32)l_aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE];
    ptTLunMg->DataBuffAddr = (U32)&l_aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE];

    /*if it is not the first program cycle, no need to init the red&data buffer.*/
    if (0 != ucPrgCycle)
    {
        return TRUE;
    }

    ulPhyPgNum = usPrgWL*PG_PER_WL;
    ulSecNumPerPageForTest = (TRUE != ptTLunMg->FlashSingePln) ? SEC_PER_BUF : SEC_PER_LOGIC_PG;

    ucPattenVal = ptTLunMg->PattenValue;
    for (ucPartIndex = 0; ucPartIndex < DSG_BUFF_SIZE; ucPartIndex++)
    {
        U32 ulDataBaseAddr, ulRedBaseAddr;

        ulDataBaseAddr = l_aBuffAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPartIndex];
        ulRedBaseAddr = l_aRedAddr[ulTLun][(usPrgWL % PG_PER_WL)*DSG_BUFF_SIZE + ucPartIndex];

        // init red area & data area.
        ulDummyData = (ucPattenVal | (ucPattenVal << 8) | (ucPattenVal << 16) | (ucPattenVal << 24));
        pRed = (RED *)ulRedBaseAddr;
        pRed->m_RedComm.bcPageType = PAGE_TYPE_DATA;
        pRed->m_RedComm.ulTimeStamp = ulDummyData;
        pRed->m_RedComm.ulTargetOffsetTS = ulDummyData;
        ulLPNIndex = 0;

        for (ulSecIndex = 0; ulSecIndex < ulSecNumPerPageForTest; ulSecIndex++)
        {
            // Note: Inorder to meet mode data check solution, we should
            // init data in the second dw each sec, and set Red Lpn data
            // according ulDummyData. -- with nina's advice.
            *((volatile U32 *)(ulDataBaseAddr + ulSecIndex*SEC_SIZE) + 1) = ulDummyData;

            if (0 == ulSecIndex%SEC_PER_LPN)
            {
                pRed->m_DataRed.aCurrLPN[ulLPNIndex++] = L0M_GET_SUBSYSLBA_FROM_LBA(ulDummyData, 0) / SEC_PER_LPN;
            }
        }
    }

    return TRUE;
}

LOCAL MCU1_DRAM_TEXT void RDT_FlashWrite2DTlcPage(U8 ucTLun, RDT_TEST_MANAGER * ptTLunMg, XOR_PARAM *pXorParam)
{
    U8 ucIndex;
    U16 aBuffID[DSG_BUFF_SIZE] = { 0 };
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;
    BOOL bSingle = (BOOL)ptTLunMg->FlashSingePln;
    U32 *pPBuffer = (U32 *)ptTLunMg->DataBuffAddr;
    U32 *pSpare = (U32 *)ptTLunMg->SpareBuffAddr;
    U8 *pStatus = (U8 *)ptTLunMg->FlashStatusBuffAddr;
    U8 ucBuffCnt = DSG_BUFF_SIZE;

    for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
    {
        aBuffID[ucIndex] = ((U32)(pPBuffer[ucIndex]) - DRAM_START_ADDRESS) >> BUF_SIZE_BITS;
    }

#ifndef SIM
    if ((U32)pPBuffer[0] > DRAM_HIGH_START_ADDRESS)
    {
        for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
        {
            aBuffID[ucIndex] = ((U32)pPBuffer[ucIndex] - DRAM_HIGH_START_ADDRESS) >> BUF_SIZE_BITS;
        }
    }
#endif

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = bSingle ? FCMD_REQ_SUBTYPE_SINGLE : FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->tLocalDesc.bsRdtCmd = TRUE;

    ptReqEntry->tFlashDesc.bsVirBlk = ptTLunMg->Blk;
    ptReqEntry->tFlashDesc.bsVirPage = ptTLunMg->PrePage;
    ptReqEntry->tFlashDesc.bsPlnNum = ptTLunMg->Pln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;

    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = (bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF) * ucBuffCnt;

    for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
    {
        ptReqEntry->atBufDesc[ucIndex].bsBufID = aBuffID[ucIndex];
        ptReqEntry->atBufDesc[ucIndex].bsSecStart = 0;
        ptReqEntry->atBufDesc[ucIndex].bsSecLen = bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF;
    }

    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
    if (TRUE == bSingle)
    {
        COM_MemCpy(pTargetRed, pSpare, RED_SZ_DW);
        COM_MemCpy(pTargetRed + RED_SZ_DW, pSpare + sizeof(NFC_RED) / sizeof(U32), RED_SZ_DW);
        COM_MemCpy(pTargetRed + 2 * RED_SZ_DW, pSpare + 2 * sizeof(NFC_RED) / sizeof(U32), RED_SZ_DW);
    }
    else
    {
        COM_MemCpy(pTargetRed, pSpare, sizeof(NFC_RED)*PG_PER_WL / sizeof(U32));
    }
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    if (pXorParam != NULL)
    {
        ptReqEntry->bsXorEn = pXorParam->bsXorEn;
        ptReqEntry->bsXorStripeId = pXorParam->bsXorStripeId;
        ptReqEntry->bsContainXorData = pXorParam->bsContainXorData;
    }

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return;
}

#ifdef FLASH_INTEL_3DTLC
LOCAL MCU1_DRAM_TEXT BOOL RDT_FlashPrepareForINTEL3DTlcWrite(U32 ulTLun, RDT_TEST_MANAGER * ptTLunMg)
{
    U8 ucPattenVal;
    U32 ulSecIndex;
    U32 ulLPNIndex;
    U32 ulDummyData;
    RED *pRed = NULL;
    U8 ucTLCPairPageType;
    U8 ucPartIndex, ucBuffCnt;
    static U32 ulaBuffAddr[DSG_BUFF_SIZE] = { 0 };
    static U32 ulaRedAddr[DSG_BUFF_SIZE] = { 0 };
    U16 aPageNum[DSG_BUFF_SIZE] = { 0 };

    ptTLunMg->PrePage = ptTLunMg->Page;

    // allocate flash status and check it before send the next program command.
    ptTLunMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ulTLun);

    // generate the red address and data buffer address for the program command
    ucTLCPairPageType = HAL_GetFlashPairPageType(ptTLunMg->Page);

    if (LOW_PAGE_WITHOUT_HIGH == ucTLCPairPageType)
    {
        ucBuffCnt = 1;

        aPageNum[0] = ptTLunMg->Page;
        ulaBuffAddr[0] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);
    }
    else if (LOW_PAGE == ucTLCPairPageType)
    {
        ucBuffCnt = 2;

        aPageNum[0] = ptTLunMg->Page;
        ulaBuffAddr[0] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);

        aPageNum[1] = ptTLunMg->Page + 1;
        ulaBuffAddr[1] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 1);

        ptTLunMg->Page++; // skip the upper page
    }
    else if (EXTRA_PAGE == ucTLCPairPageType)
    {
        ucBuffCnt = 2;

        aPageNum[0] = ptTLunMg->Page;
        ulaBuffAddr[0] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);

        aPageNum[1] = HAL_GetHighPageIndexfromExtra(ptTLunMg->Page);
        ulaBuffAddr[1] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 1);
    }
    else
    {
        DBG_Getch();
    }


    ulaRedAddr[0] = RDT_GetSpareBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);

    ptTLunMg->SpareBuffAddr = (U32)ulaRedAddr[0];
    ptTLunMg->DataBuffAddr = (U32)ulaBuffAddr;

#if 0
    DBG_Printf("TLUN%d Blk%d PrgPage%d FlashStatus=0x%x, RedAddr=0x%x, DataBuff=0x%x,0x%x,0x%x, BufID=0x%x,0x%x,0x%x\n",
        ulTLun, ptTLunMg->Blk, ptTLunMg->Page,
        ptTLunMg->FlashStatusBuffAddr,
        ptTLunMg->SpareBuffAddr,
        *((U32*)ptTLunMg->DataBuffAddr),
        *((U32*)ptTLunMg->DataBuffAddr + 1),
        *((U32*)ptTLunMg->DataBuffAddr + 2),
        COM_GetBufferIDByMemAddr(*((U32*)ptTLunMg->DataBuffAddr), TRUE, LOGIC_PIPE_PG_SZ_BITS),
        COM_GetBufferIDByMemAddr(*((U32*)ptTLunMg->DataBuffAddr + 1), TRUE, LOGIC_PIPE_PG_SZ_BITS),
        COM_GetBufferIDByMemAddr(*((U32*)ptTLunMg->DataBuffAddr + 2), TRUE, LOGIC_PIPE_PG_SZ_BITS));
#endif

    // prepare data and red.
    ucPattenVal = ptTLunMg->PattenValue;
    for (ucPartIndex = 0; ucPartIndex < ucBuffCnt; ucPartIndex++)
    {
        U32 ulDataBaseAddr, ulRedBaseAddr;
        U16 usPageNum;

        ulDataBaseAddr = ulaBuffAddr[ucPartIndex];
        ulRedBaseAddr = ulaRedAddr[0] + sizeof(RED)*ucPartIndex;
        usPageNum = aPageNum[ucPartIndex];

        // init red area & data area.
        ulDummyData = (ucPattenVal | (ucPattenVal << 8) | (ucPattenVal << 16) | (ucPattenVal << 24));
        pRed = (RED *)ulRedBaseAddr;
        pRed->m_RedComm.bcPageType = PAGE_TYPE_DATA;
        pRed->m_RedComm.ulTimeStamp = ulDummyData;
        pRed->m_RedComm.ulTargetOffsetTS = ulDummyData;

        ulLPNIndex = 0;
#ifdef SIM
        for (ulSecIndex = 0; ulSecIndex < SEC_PER_LOGIC_PG; ulSecIndex++)
#else
        for (ulSecIndex = 0; ulSecIndex < 2; ulSecIndex++)
#endif
        {
            // Note: Inorder to meet mode data check solution, we should
            // init data in the second dw each sec, and set Red Lpn data
            // according ulDummyData. -- with nina's advice.
            *((volatile U32 *)(ulDataBaseAddr + ulSecIndex*SEC_SIZE) + 1) = ulDummyData;

            if (0 == ulSecIndex%SEC_PER_LPN)
            {
                pRed->m_DataRed.aCurrLPN[ulLPNIndex++] = L0M_GET_SUBSYSLBA_FROM_LBA(ulDummyData, 0) / SEC_PER_LPN;
            }
        }
    }

    l_ulTotalPrgOrder[ulTLun]++;

    return TRUE;
}

LOCAL MCU1_DRAM_TEXT void RDT_FlashWriteINTEL3DTlcPage(U8 ucTLun, RDT_TEST_MANAGER * ptTLunMg, XOR_PARAM *pXorParam)
{
    U8 ucIndex;
    U16 aBuffID[DSG_BUFF_SIZE];
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;
    BOOL bSingle = (BOOL)ptTLunMg->FlashSingePln;
    U32 *pPBuffer = (U32 *)ptTLunMg->DataBuffAddr;
    U32 *pSpare = (U32 *)ptTLunMg->SpareBuffAddr;
    U8 *pStatus = (U8 *)ptTLunMg->FlashStatusBuffAddr;
    U8 ucPairPageType = HAL_GetFlashPairPageType(ptTLunMg->PrePage);
    U32 ucPairPageNum;
    U8 ucBuffCnt;

    if (LOW_PAGE_WITHOUT_HIGH == ucPairPageType)
    {
        ucBuffCnt = 1;
        ucPairPageNum = ptTLunMg->PrePage;
    }
    else if (LOW_PAGE == ucPairPageType)
    {
        ucBuffCnt = 2;
        ucPairPageNum = ptTLunMg->PrePage + 1;
    }
    else if (EXTRA_PAGE == ucPairPageType)
    {
        ucBuffCnt = 2;
        ucPairPageNum = HAL_GetHighPageIndexfromExtra(ptTLunMg->PrePage);
    }
    else
    {
        DBG_Getch();
    }

    for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
    {
        aBuffID[ucIndex] = ((U32)(pPBuffer[ucIndex]) - DRAM_START_ADDRESS) >> BUF_SIZE_BITS;
    }
#ifndef SIM
    if ((U32)pPBuffer[0] > DRAM_HIGH_START_ADDRESS)
    {
        for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
        {
            aBuffID[ucIndex] = ((U32)pPBuffer[ucIndex] - DRAM_HIGH_START_ADDRESS) >> BUF_SIZE_BITS;
        }
    }
#endif
    for (; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        aBuffID[ucIndex] = INVALID_4F;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = (1 == ucBuffCnt) ? (bSingle ? FCMD_REQ_SUBTYPE_SINGLE_ONEPG : FCMD_REQ_SUBTYPE_ONEPG) : (bSingle ? FCMD_REQ_SUBTYPE_SINGLE_TWOPG : FCMD_REQ_SUBTYPE_NORMAL);
    ptReqEntry->tLocalDesc.bsRdtCmd = TRUE;

    ptReqEntry->tFlashDesc.bsVirBlk = ptTLunMg->Blk;
    ptReqEntry->tFlashDesc.bsVirPage = ptTLunMg->PrePage;
    ptReqEntry->tFlashDesc.bsPlnNum = ptTLunMg->Pln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = (bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF) * ucBuffCnt;

    for (ucIndex = 0; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        ptReqEntry->atBufDesc[ucIndex].bsBufID = aBuffID[ucIndex];
        ptReqEntry->atBufDesc[ucIndex].bsSecStart = 0;
        ptReqEntry->atBufDesc[ucIndex].bsSecLen = bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF;
    }

    ptReqEntry->tLocalDesc.bsPairPageType = ucPairPageType;
    ptReqEntry->tLocalDesc.bsPairPageCnt = ucBuffCnt;
    ptReqEntry->tLocalDesc.bsPairPageNum = ucPairPageNum;

    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
    COM_MemCpy(pTargetRed, pSpare, sizeof(RED)*ucBuffCnt / sizeof(U32));
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    if (pXorParam != NULL)
    {
        ptReqEntry->bsXorEn = pXorParam->bsXorEn;
        ptReqEntry->bsXorStripeId = pXorParam->bsXorStripeId;
        ptReqEntry->bsContainXorData = pXorParam->bsContainXorData;
    }

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
#ifdef RDT_FCMDQ_STATISTIC
    g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_WRITE + 3]++;
    g_aFCMDStatus[ucTLun][7]++;
    if (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_WRITE]++;
        g_aFCMDStatus[ucTLun][6]++;
    }
#endif
    return;
}

#endif

#ifdef FLASH_IM_3DTLC_GEN2
LOCAL MCU1_DRAM_TEXT BOOL RDT_FlashPrepareForIM3DTlcGen2Write(U32 ulTLun, RDT_TEST_MANAGER * ptTLunMg)
{
    U8 ucPattenVal;
    U32 ulSecIndex;
    U32 ulLPNIndex;
    U32 ulDummyData;
    RED *pRed = NULL;
    U8 ucTLCPairPageType;
    U8 ucPartIndex, ucBuffCnt;
    static U32 ulaBuffAddr[DSG_BUFF_SIZE] = { 0 };
    static U32 ulaRedAddr[DSG_BUFF_SIZE] = { 0 };
    U16 aPageNum[DSG_BUFF_SIZE] = { 0 };

    ptTLunMg->PrePage = ptTLunMg->Page;

    // allocate flash status and check it before send the next program command.
    ptTLunMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ulTLun);

    // generate the red address and data buffer address for the program command
    ucTLCPairPageType = HAL_GetFlashPairPageType(ptTLunMg->Page);

    if (LOW_PAGE_WITHOUT_HIGH == ucTLCPairPageType) //only have low page
    {
        ucBuffCnt = 1;

        aPageNum[0] = ptTLunMg->Page;
        ulaBuffAddr[0] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);
    }
    else if (LOW_PAGE == ucTLCPairPageType || EXTRA_PAGE == ucTLCPairPageType)        // only have low and mid page
    {
        ucBuffCnt = 2;

        aPageNum[0] = ptTLunMg->Page;
        ulaBuffAddr[0] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);

        aPageNum[1] = ptTLunMg->Page + 1;
        ulaBuffAddr[1] = RDT_GetDataBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 1);

        ptTLunMg->Page++; // skip the upper page
    }
    else
    {
        DBG_Getch();
    }

    ulaRedAddr[0] = RDT_GetSpareBuffAddrFor2DTlcWrite(ulTLun, l_ulTotalPrgOrder[ulTLun], 0);

    ptTLunMg->SpareBuffAddr = (U32)ulaRedAddr[0];
    ptTLunMg->DataBuffAddr = (U32)ulaBuffAddr;

#if 0
    DBG_Printf("TLUN%d Blk%d PrgPage%d FlashStatus=0x%x, RedAddr=0x%x, DataBuff=0x%x,0x%x,0x%x, BufID=0x%x,0x%x,0x%x\n",
        ulTLun, ptTLunMg->Blk, ptTLunMg->Page,
        ptTLunMg->FlashStatusBuffAddr,
        ptTLunMg->SpareBuffAddr,
        *((U32*)ptTLunMg->DataBuffAddr),
        *((U32*)ptTLunMg->DataBuffAddr + 1),
        *((U32*)ptTLunMg->DataBuffAddr + 2),
        COM_GetBufferIDByMemAddr(*((U32*)ptTLunMg->DataBuffAddr), TRUE, LOGIC_PIPE_PG_SZ_BITS),
        COM_GetBufferIDByMemAddr(*((U32*)ptTLunMg->DataBuffAddr + 1), TRUE, LOGIC_PIPE_PG_SZ_BITS),
        COM_GetBufferIDByMemAddr(*((U32*)ptTLunMg->DataBuffAddr + 2), TRUE, LOGIC_PIPE_PG_SZ_BITS));
#endif

    // prepare data and red.
    ucPattenVal = ptTLunMg->PattenValue;
    for (ucPartIndex = 0; ucPartIndex < ucBuffCnt; ucPartIndex++)
    {
        U32 ulDataBaseAddr, ulRedBaseAddr;
        U16 usPageNum;

        ulDataBaseAddr = ulaBuffAddr[ucPartIndex];
        ulRedBaseAddr = ulaRedAddr[0] + sizeof(RED)*ucPartIndex;
        usPageNum = aPageNum[ucPartIndex];

        // init red area & data area.
        ulDummyData = (ucPattenVal | (ucPattenVal << 8) | (ucPattenVal << 16) | (ucPattenVal << 24));
        pRed = (RED *)ulRedBaseAddr;
        pRed->m_RedComm.bcPageType = PAGE_TYPE_DATA;
        pRed->m_RedComm.ulTimeStamp = ulDummyData;
        pRed->m_RedComm.ulTargetOffsetTS = ulDummyData;

        ulLPNIndex = 0;

        for (ulSecIndex = 0; ulSecIndex < SEC_PER_LOGIC_PG; ulSecIndex++)
        {
            // Note: Inorder to meet mode data check solution, we should
            // init data in the second dw each sec, and set Red Lpn data
            // according ulDummyData. -- with nina's advice.
            *((volatile U32 *)(ulDataBaseAddr + ulSecIndex*SEC_SIZE) + 1) = ulDummyData;

            if (0 == ulSecIndex%SEC_PER_LPN)
            {
                pRed->m_DataRed.aCurrLPN[ulLPNIndex++] = L0M_GET_SUBSYSLBA_FROM_LBA(ulDummyData, 0) / SEC_PER_LPN;
            }
        }
    }

    l_ulTotalPrgOrder[ulTLun]++;

    return TRUE;
}

LOCAL MCU1_DRAM_TEXT void RDT_FlashWriteIM3DTlcGen2Page(U8 ucTLun, RDT_TEST_MANAGER * ptTLunMg, XOR_PARAM *pXorParam)
{
    U8 ucIndex;
    U16 aBuffID[DSG_BUFF_SIZE];
    U32 *pTargetRed;
    FCMD_REQ_ENTRY *ptReqEntry;
    BOOL bSingle = ptTLunMg->FlashSingePln;
    U32 *pPBuffer = (U32 *)ptTLunMg->DataBuffAddr;
    U32 *pSpare = (U32 *)ptTLunMg->SpareBuffAddr;
    U8 *pStatus = (U8 *)ptTLunMg->FlashStatusBuffAddr;
    U8 ucPairPageType = HAL_GetFlashPairPageType(ptTLunMg->PrePage);
    U32 ucPairPageNum;
    U8 ucBuffCnt;

    if (LOW_PAGE_WITHOUT_HIGH == ucPairPageType)
    {
        ucBuffCnt = 1;
        ucPairPageNum = ptTLunMg->PrePage;
    }
    else if (LOW_PAGE == ucPairPageType || EXTRA_PAGE == ucPairPageType)
    {
        ucBuffCnt = 2;
        ucPairPageNum = ptTLunMg->PrePage + 1;
    }
    else
    {
        DBG_Getch();
    }

    for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
    {
        aBuffID[ucIndex] = ((U32)(pPBuffer[ucIndex]) - DRAM_START_ADDRESS) >> BUF_SIZE_BITS;
    }
#ifndef SIM
    if ((U32)pPBuffer[0] > DRAM_HIGH_START_ADDRESS)
    {
        for (ucIndex = 0; ucIndex < ucBuffCnt; ucIndex++)
        {
            aBuffID[ucIndex] = ((U32)pPBuffer[ucIndex] - DRAM_HIGH_START_ADDRESS) >> BUF_SIZE_BITS;
        }
    }
#endif
    for (; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        aBuffID[ucIndex] = INVALID_4F;
    }

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
    ptReqEntry->bsReqSubType = (1 == ucBuffCnt) ? (bSingle ? FCMD_REQ_SUBTYPE_SINGLE_ONEPG : FCMD_REQ_SUBTYPE_ONEPG) : (bSingle ? FCMD_REQ_SUBTYPE_SINGLE_TWOPG : FCMD_REQ_SUBTYPE_NORMAL);
    ptReqEntry->tLocalDesc.bsRdtCmd = TRUE;

    ptReqEntry->tFlashDesc.bsVirBlk = ptTLunMg->Blk;
    ptReqEntry->tFlashDesc.bsVirPage = ptTLunMg->PrePage;
    ptReqEntry->tFlashDesc.bsPlnNum = ptTLunMg->Pln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = (bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF) * ucBuffCnt;

    for (ucIndex = 0; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        ptReqEntry->atBufDesc[ucIndex].bsBufID = aBuffID[ucIndex];
        ptReqEntry->atBufDesc[ucIndex].bsSecStart = 0;
        ptReqEntry->atBufDesc[ucIndex].bsSecLen = bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF;
    }

    ptReqEntry->tLocalDesc.bsPairPageType = ucPairPageType;
    ptReqEntry->tLocalDesc.bsPairPageCnt = ucBuffCnt;
    ptReqEntry->tLocalDesc.bsPairPageNum = ucPairPageNum;

    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
    COM_MemCpy(pTargetRed, pSpare, sizeof(RED)*ucBuffCnt / sizeof(U32));
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    if (pXorParam != NULL)
    {
        ptReqEntry->bsXorEn = pXorParam->bsXorEn;
        ptReqEntry->bsXorStripeId = pXorParam->bsXorStripeId;
        ptReqEntry->bsContainXorData = pXorParam->bsContainXorData;
    }

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
#ifdef RDT_FCMDQ_STATISTIC
    g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_WRITE + 3]++;
    g_aFCMDStatus[ucTLun][7]++;
    if (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_WRITE]++;
        g_aFCMDStatus[ucTLun][6]++;
    }
#endif
    return;
}

#endif

/*RDT Read Command*/
LOCAL MCU1_DRAM_TEXT void RDT_FlashReadPage(U8 ucTLun, RDT_TEST_MANAGER * ptTLunMg)
{
    FCMD_REQ_ENTRY *ptReqEntry;
    U32 ulDataBuffAddr, ulSpareBuffAddr;
    U8 *pStatus = (U8*)ptTLunMg->FlashStatusBuffAddr;
    BOOL bSingle = ptTLunMg->FlashSingePln;

    ulDataBuffAddr = RDT_GetDataBuffAddr(ucTLun);
    ulSpareBuffAddr = RDT_GetSpareBuffAddr(ucTLun);

    ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
    ptReqEntry->bsReqSubType = bSingle ? FCMD_REQ_SUBTYPE_SINGLE : FCMD_REQ_SUBTYPE_NORMAL;
    ptReqEntry->tLocalDesc.bsRdtCmd = TRUE;
    ptReqEntry->bsTableReq = FALSE;

    ptReqEntry->tFlashDesc.bsVirBlk = ptTLunMg->Blk;
    ptReqEntry->tFlashDesc.bsVirPage = ptTLunMg->Page;
    ptReqEntry->tFlashDesc.bsPlnNum = ptTLunMg->Pln;
    ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_TLC_BLK;
    ptReqEntry->tFlashDesc.bsSecStart = 0;
    ptReqEntry->tFlashDesc.bsSecLen = bSingle ? SEC_PER_LOGIC_PG : SEC_PER_BUF;

    ptReqEntry->atBufDesc[0].bsBufID = COM_GetBufferIDByMemAddr(ulDataBuffAddr, TRUE, bSingle ? LOGIC_PG_SZ_BITS : LOGIC_PIPE_PG_SZ_BITS);
    ptReqEntry->atBufDesc[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ptReqEntry->atBufDesc[0].bsSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

    ptReqEntry->ulSpareAddr = (U32)ulSpareBuffAddr;

    *pStatus = SUBSYSTEM_STATUS_PENDING;
    ptReqEntry->ulReqStsAddr = (U32)pStatus;
    ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

    L2_FCMDQAdaptPhyBlk(ptReqEntry);
    L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
#ifdef RDT_FCMDQ_STATISTIC
    g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_READ + 3]++;
    g_aFCMDStatus[ucTLun][7]++;
    if (TRUE != L2_FCMDQNotFull(ucTLun))
    {
        g_aFCMDStatus[ucTLun][FCMD_REQ_TYPE_READ]++;
        g_aFCMDStatus[ucTLun][6]++;
    }
#endif
    return;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_Test(void)
{
    U8 ucTLun, ucIndex;
    U8 ucNextStage;
    RDT_TEST_MANAGER *ptTLunMg;

    U32 ulTestLunNumCnt = l_ptRdtCtrl->ulRdtTestLunNumCnt;

    for (ucTLun = 0; ucTLun < ulTestLunNumCnt; ucTLun++)
    {
        if (TRUE != L2_FCMDQIsNotFull(ucTLun, 0))
        {
            continue;
        }

        if (TRUE != RDT_Check(ucTLun))
        {
            RDT_CheckLastFourFCmd(ucTLun);
            RDT_ClearLastFourFCmd(ucTLun);
            RDT_SelectNextErrHStage(ucTLun);
        }

        ptTLunMg = &l_ptRdtTestMg[ucTLun];

        switch (ptTLunMg->CurStage)
        {
        case RDT_TEST_INIT:
            ucNextStage = RDT_SelectNextStage(ucTLun);
            break;

        case RDT_TEST_ERASE:
            ptTLunMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ucTLun);
            RDT_FlashEraseBlock(ucTLun, ptTLunMg);
            //DBG_Printf("MCU#%d [%d,%d,%d,%d] PATTEN=%d LOOP=%d ERASE\n", HAL_GetMcuId(), ucTLun, ptTLunMg->Blk, ptTLunMg->Pln, ptTLunMg->Page, l_ptRdtCtrl->ulRdtPatten, l_ptRdtCtrl->ulRdtRunCycle);
            ucNextStage = RDT_SelectNextStage(ucTLun);
            break;

        case RDT_TEST_WRITE:
#ifdef FLASH_INTEL_3DTLC
            #ifdef FLASH_IM_3DTLC_GEN2
            RDT_FlashPrepareForIM3DTlcGen2Write(ucTLun, ptTLunMg);
            RDT_FlashWriteIM3DTlcGen2Page(ucTLun, ptTLunMg, NULL);
            #else
            RDT_FlashPrepareForINTEL3DTlcWrite(ucTLun, ptTLunMg);
            RDT_FlashWriteINTEL3DTlcPage(ucTLun, ptTLunMg, NULL);
            #endif
#else
            RDT_FlashPrepareFor2DTlcWrite(ucTLun, ptTLunMg);
            RDT_FlashWrite2DTlcPage(ucTLun, ptTLunMg, NULL);
#endif
            //DBG_Printf("MCU#%d [%d,%d,%d,%d] PATTEN=%d LOOP=%d WRITE\n", HAL_GetMcuId(), ucTLun, ptTLunMg->Blk, ptTLunMg->Pln, ptTLunMg->Page, l_ptRdtCtrl->ulRdtPatten, l_ptRdtCtrl->ulRdtRunCycle);
            ucNextStage = RDT_SelectNextStage(ucTLun);
            break;

        case RDT_TEST_READ:
            ptTLunMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ucTLun);
            RDT_FlashReadPage(ucTLun, ptTLunMg);
            //DBG_Printf("MCU#%d [%d,%d,%d,%d] PATTEN=%d LOOP=%d READ\n", HAL_GetMcuId(), ucTLun, ptTLunMg->Blk, ptTLunMg->Pln, ptTLunMg->Page, l_ptRdtCtrl->ulRdtPatten, l_ptRdtCtrl->ulRdtRunCycle);
            ucNextStage = RDT_SelectNextStage(ucTLun);
            break;

        case RDT_TEST_FINISH:
            ucNextStage = RDT_TEST_FINISH;
            if (l_ptRdtCtrl->ulRdtTestLunNumCnt <= l_ptRdtCtrl->ulFinishLunNum)
            {
                /*Check last four cmd*/
                for (ucIndex = 0; ucIndex < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucIndex++)
                {
                    RDT_CheckLastFourFCmd(ucIndex);
                }
                ucNextStage = RDT_TEST_INIT;
                DBG_Printf("Loop#%d Patten#%d Pass!\n\n", l_ptRdtCtrl->ulRdtRunCycle, l_ptRdtCtrl->ulRdtPatten);
                l_ptRdtCtrl->ulRdtPatten++;
                l_ptRdtCtrl->ulFinishLunNum = 0;
                RDT_TestMgInit();
                if (l_ptRdtCtrl->ulRdtPatten == l_ptRdtCtrl->ulRdtPattenCnt)
                {
                    l_ptRdtCtrl->ulRdtRunCycle++;
                    l_ptRdtCtrl->ulRdtPatten = 0;

                    if (l_ptRdtCtrl->ulRdtRunCycleCnt == l_ptRdtCtrl->ulRdtRunCycle)
                    {
                        l_bFinishRDTTest = TRUE;
                    }
                }
            }
            break;

        default:
            DBG_Printf("MCU#%d [%d,%d,%d] RDT_Test Error Stage=0x%x!\n", HAL_GetMcuId(), ucTLun, ptTLunMg->Blk, ptTLunMg->Page, ptTLunMg->CurStage);
            RDT_CancelTimer();
            RDT_Led_Off();
            DBG_Getch();
        }

        ptTLunMg->CurStage = ucNextStage;
    }

    return l_bFinishRDTTest;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_Save_Result(void)
{
    U8 ucTLun = 0;
    BOOL bRdtSaveDone = FALSE;
    RDT_SORT_MANAGER *ptSaveMg;
    LOCAL U8 ucSaveLunNum = 0;
    LOCAL RED l_tBbtRed;
    U32 ulRdtHeaderAddr;
    RDT_HEAD RdtHead = { 0 };

    for (ucTLun = 0; ucTLun < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucTLun++)
    {
        ptSaveMg = &l_ptRdtSortMg[ucTLun];

        switch (ptSaveMg->bsOpStage)
        {
        case RDT_SORT_INIT:
        {
            ptSaveMg->bsBlk = 0;
            ptSaveMg->bsPage = l_ptRdtCtrl->ucRdtPage[ucTLun];
            ptSaveMg->bsPln = l_ptRdtCtrl->usRdtPln[ucTLun];
            //DBG_Printf("RDT_SAVE TLun=%d RdtPln=%d RdtPage=%d\n", ucTLun, ptSaveMg->bsPln, ptSaveMg->bsPage);

            ulRdtHeaderAddr = l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + l_ptRdtCtrl->usLocalRdtOffset;
            COM_MemCpy((U32 *)&RdtHead, (U32 *)&l_ptRdtCtrl->tRdtHead, sizeof(RDT_HEAD) / sizeof(U32));
            RdtHead.usBadBlkCnt = l_ptRdtCtrl->usBadBlkCnt[ucTLun];
            RdtHead.ucRdtLogSizeInPage = RDT_PG_NUM;
            RdtHead.usRDTTime = (U16)(l_ulLedRunCount_Sec);
            COM_MemCpy((U32 *)ulRdtHeaderAddr, (U32 *)&RdtHead, sizeof(RDT_HEAD) / sizeof(U32));

            ptSaveMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddrOTFB(ucTLun);

            ptSaveMg->SpareBuffAddr = RDT_GetSpareBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptSaveMg->SpareBuffAddr, sizeof(RED) / sizeof(U32));

            ptSaveMg->bsOpStage = RDT_SORT_WRITE;

            break;
        }

        case RDT_SORT_WRITE:
        {
            U32 *pTargetRed;
            FCMD_REQ_ENTRY *ptReqEntry;
            RED *pRed = NULL;

            if (TRUE != L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            pRed = (RED *)ptSaveMg->SpareBuffAddr;
            pRed->m_RedComm.bcPageType = PAGE_TYPE_BBT;
            pRed->m_tGBbtInfo.ulBbtMark = BBT_RDT_MARK;

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_WRITE;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptSaveMg->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptSaveMg->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptSaveMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

            ptReqEntry->atBufDesc[0].bsBufID = COM_GetBufferIDByMemAddr(l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + (ptSaveMg->bsPln - 1)*LOGIC_PG_SZ, TRUE, LOGIC_PG_SZ_BITS);
            ptReqEntry->atBufDesc[0].bsSecStart = 0;
            ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
            ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

            pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
            COM_MemCpy(pTargetRed, (U32*)ptSaveMg->SpareBuffAddr, sizeof(NFC_RED) / sizeof(U32));
            ptReqEntry->ulSpareAddr = (U32)pTargetRed;

            *(U8 *)ptSaveMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptSaveMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_AUTO;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
            DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Save.\n", HAL_GetMcuId(), ucTLun, ptSaveMg->bsPln, ptSaveMg->bsBlk, ptSaveMg->bsPage);
            ptSaveMg->bsOpStage = RDT_SORT_WRITE_WAIT;
            break;
        }

        case RDT_SORT_WRITE_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *(U8 *)ptSaveMg->FlashStatusBuffAddr)
            {
                if (SUBSYSTEM_STATUS_FAIL != *(U8 *)ptSaveMg->FlashStatusBuffAddr && SUBSYSTEM_STATUS_SUCCESS != *(U8 *)ptSaveMg->FlashStatusBuffAddr)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d BBT Save-Local-BBT InvalidStatus.\n", HAL_GetMcuId(), ucTLun, ptSaveMg->bsPln, ptSaveMg->bsBlk);
                    RDT_CancelTimer();
                    RDT_Led_Off();
                    DBG_Getch();
                }

                if (SUBSYSTEM_STATUS_FAIL == *(U8 *)ptSaveMg->FlashStatusBuffAddr)
                {
                    DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d BBT Save-Local-BBT Fail.\n", HAL_GetMcuId(), ucTLun, ptSaveMg->bsPln, ptSaveMg->bsBlk, ptSaveMg->bsPage);
                    RDT_CancelTimer();
                    RDT_Led_Off();
                    DBG_Getch();
                }

                ptSaveMg->bsPln = (ptSaveMg->bsPln + 1) % PLN_PER_LUN;
                if (0 == ptSaveMg->bsPln)
                {
                    ptSaveMg->bsOpStage = RDT_SORT_FINISH;
                    ucSaveLunNum++;
                }
                else
                {
                    ptSaveMg->bsOpStage = RDT_SORT_WRITE;
                }
            }
            break;
        }
        case RDT_SORT_FINISH:
        {
            if (l_ptRdtCtrl->ulRdtTestLunNumCnt == ucSaveLunNum)
            {
                bRdtSaveDone = TRUE;
                return TRUE;
            }
            ptSaveMg->bsOpStage = RDT_SORT_FINISH;
            break;
        }
        default:
        {
            DBG_Printf("MCU#%d TLun#%d BBT Save Local BBT Stage %d Error.\n", HAL_GetMcuId(), ucTLun, ptSaveMg->bsOpStage);
            RDT_CancelTimer();
            RDT_Led_Off();
            DBG_Getch();
        }
        }
    }

    return bRdtSaveDone;

}

#ifdef RDT_CHECK_RESULT
LOCAL MCU1_DRAM_TEXT BOOL RDT_FindRdtTarget(void)
{
    U8 ucTLun;
    BOOL bFindLBbtDone = FALSE;
    RDT_SORT_MANAGER *ptFindTargetMg;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptFindTargetMg = &l_ptRdtSortMg[ucTLun];

        switch (ptFindTargetMg->bsOpStage)
        {
        case RDT_SORT_INIT:
        {
            ptFindTargetMg->bsPage = 0;
            ptFindTargetMg->bsBlk = 0;
            ptFindTargetMg->bsPln = 1;

            ptFindTargetMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ucTLun);

            ptFindTargetMg->SpareBuffAddr = RDT_GetSpareBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptFindTargetMg->SpareBuffAddr, sizeof(RED) / sizeof(U32));

            ptFindTargetMg->bsOpStage = RDT_SORT_READ_RED;
            break;
        }
        case RDT_SORT_READ_RED:
        {
            FCMD_REQ_ENTRY *ptReqEntry;

            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);

            ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptFindTargetMg->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptFindTargetMg->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptFindTargetMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;

            ptReqEntry->tFlashDesc.bsRdRedOnly = TRUE;
            ptReqEntry->ulSpareAddr = (U32)ptFindTargetMg->SpareBuffAddr;

            *(U8 *)ptFindTargetMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptFindTargetMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
            DBG_Printf("MCU#%d TLun#%d Pln#%d Blk#%d Page#%d Find Rdt Page.\n", HAL_GetMcuId(), ucTLun, ptFindTargetMg->bsPln, ptFindTargetMg->bsBlk, ptFindTargetMg->bsPage);
            ptFindTargetMg->bsOpStage = RDT_SORT_READ_WAIT;
            break;
        }
        case RDT_SORT_READ_WAIT:
        {
            if (SUBSYSTEM_STATUS_PENDING != *(U8 *)ptFindTargetMg->FlashStatusBuffAddr)
            {
                if (SUBSYSTEM_STATUS_SUCCESS == *(U8 *)ptFindTargetMg->FlashStatusBuffAddr || SUBSYSTEM_STATUS_RECC == *(U8 *)ptFindTargetMg->FlashStatusBuffAddr || SUBSYSTEM_STATUS_RETRY_SUCCESS == *(U8 *)ptFindTargetMg->FlashStatusBuffAddr)
                {
                    if (BBT_RDT_MARK != ((RED *)ptFindTargetMg->SpareBuffAddr)->m_tGBbtInfo.ulBbtMark)
                    {
                        DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Find Rdt Mark-Miss-Match.\n", ucTLun, ptFindTargetMg->bsPln, ptFindTargetMg->bsBlk, ptFindTargetMg->bsPage);
                        return TRUE;
                    }

                    ptFindTargetMg->bsPage++;
                    if ((PG_PER_BLK - PG_PER_BLK % RDT_PG_NUM) <= ptFindTargetMg->bsPage) // CheckFull
                    {
                        DBG_Printf("MCU#%d TLun#%d BBT Find Rdt Page Fail.\n", HAL_GetMcuId(), ucTLun);
                        DBG_Getch();
                    }

                    ptFindTargetMg->bsOpStage = RDT_SORT_READ_RED;
                }
                else if (SUBSYSTEM_STATUS_EMPTY_PG == *(U8 *)ptFindTargetMg->FlashStatusBuffAddr)
                {
                    if (0 == ptFindTargetMg->bsPage)
                    {
                        DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Find Rdt First-Page-Empty.\n", ucTLun, ptFindTargetMg->bsPln, ptFindTargetMg->bsBlk, ptFindTargetMg->bsPage);
                        ptFindTargetMg->bsPage = 0;
                    }
                    else
                    {
                        if (0 == (ptFindTargetMg->bsPage % RDT_PG_NUM))
                        {
                            ptFindTargetMg->bsPage = ptFindTargetMg->bsPage - RDT_PG_NUM;
                        }
                        else
                        {
                            DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Find Rdt Page-Number-Error.\n", ucTLun, ptFindTargetMg->bsPln, ptFindTargetMg->bsBlk, ptFindTargetMg->bsPage);
                            DBG_Getch();
                        }
                    }
                    ptFindTargetMg->bsOpStage = RDT_SORT_FINISH;
                }
                else if (SUBSYSTEM_STATUS_FAIL == *(U8 *)ptFindTargetMg->FlashStatusBuffAddr)
                {
                    DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Find Rdt Read-Red Fail.\n", ucTLun, ptFindTargetMg->bsPln, ptFindTargetMg->bsBlk, ptFindTargetMg->bsPage);
                    DBG_Getch();
                    ptFindTargetMg->bsOpStage = RDT_SORT_FINISH;
                }
                else
                {
                    ; // wait flash status...
                }
            }
            break;
        }
        case RDT_SORT_FINISH:
        {
            if (TRUE == RDT_WaitAllOpStageDone(RDT_SORT_FINISH))
            {
                return TRUE; // return immediately.
            }

            break;
        }
        default:
        {
            DBG_Printf("TLun#%d Find Rdt Stage %d Error.\n", ucTLun, ptFindTargetMg->bsOpStage);
            DBG_Getch();
        }
        }
    }

    return bFindLBbtDone;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_ReadRdtPage(void)
{
    U8 ucTLun;
    BOOL bLoadBbtDone = FALSE;
    RDT_SORT_MANAGER *ptRdRdtPgMg;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptRdRdtPgMg = &l_ptRdtSortMg[ucTLun];

        switch (ptRdRdtPgMg->bsOpStage)
        {
        case RDT_SORT_INIT:
        {
            ptRdRdtPgMg->FlashStatusBuffAddr = RDT_GetStatusBuffAddr(ucTLun);

            ptRdRdtPgMg->DataBuffAddr = RDT_GetDataBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptRdRdtPgMg->DataBuffAddr, BUF_SIZE / sizeof(U32));

            ptRdRdtPgMg->SpareBuffAddr = RDT_GetSpareBuffAddr(ucTLun);
            COM_MemZero((U32 *)ptRdRdtPgMg->SpareBuffAddr, sizeof(RED) / sizeof(U32));

            ptRdRdtPgMg->bsOpStage = RDT_SORT_READ;
            break;
        }
        case RDT_SORT_READ:
        {
            FCMD_REQ_ENTRY *ptReqEntry;

            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                break;
            }

            ptReqEntry = L2_FCMDQAllocReqEntry(ucTLun, 0);
            ptReqEntry->bsReqType = FCMD_REQ_TYPE_READ;
            ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SINGLE;
            ptReqEntry->bsTableReq = TRUE;

            ptReqEntry->tFlashDesc.bsVirBlk = ptRdRdtPgMg->bsBlk;
            ptReqEntry->tFlashDesc.bsVirPage = ptRdRdtPgMg->bsPage;
            ptReqEntry->tFlashDesc.bsPlnNum = ptRdRdtPgMg->bsPln;
            ptReqEntry->tFlashDesc.bsBlkMod = FCMD_REQ_SLC_BLK;
            ptReqEntry->tFlashDesc.bsSecStart = 0;
            ptReqEntry->tFlashDesc.bsSecLen = SEC_PER_LOGIC_PG;

            ptReqEntry->atBufDesc[0].bsBufID = COM_GetBufferIDByMemAddr(ptRdRdtPgMg->DataBuffAddr, TRUE, LOGIC_PG_SZ_BITS);
            ptReqEntry->atBufDesc[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart + SEC_PER_LOGIC_PG * (ptRdRdtPgMg->bsPage % RDT_PG_NUM);;
            ptReqEntry->atBufDesc[0].bsSecLen = SEC_PER_LOGIC_PG;
            ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

            ptReqEntry->ulSpareAddr = (U32)ptRdRdtPgMg->SpareBuffAddr;

            *(U8 *)ptRdRdtPgMg->FlashStatusBuffAddr = SUBSYSTEM_STATUS_PENDING;
            ptReqEntry->ulReqStsAddr = (U32)ptRdRdtPgMg->FlashStatusBuffAddr;
            ptReqEntry->bsReqUptMod = REQ_STS_UPT_MANUL;

            L2_FCMDQAdaptPhyBlk(ptReqEntry);
            L2_FCMDQPushReqEntry(ucTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

            ptRdRdtPgMg->bsOpStage = RDT_SORT_READ_WAIT;
            break;
        }
        case RDT_SORT_READ_WAIT:
        {
            if (SUBSYSTEM_STATUS_SUCCESS == *(U8 *)ptRdRdtPgMg->FlashStatusBuffAddr || SUBSYSTEM_STATUS_RECC == *(U8 *)ptRdRdtPgMg->FlashStatusBuffAddr || SUBSYSTEM_STATUS_RETRY_SUCCESS == *(U8 *)ptRdRdtPgMg->FlashStatusBuffAddr)
            {
                if (BBT_RDT_MARK != ((RED *)ptRdRdtPgMg->SpareBuffAddr)->m_tGBbtInfo.ulBbtMark)
                {
                    DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Load Rdt Red-Mis-Match.\n", ucTLun, ptRdRdtPgMg->bsPln, ptRdRdtPgMg->bsBlk, ptRdRdtPgMg->bsPage);
                    DBG_Getch();
                }
                COM_MemCpy((U32 *)(l_ptRdtCtrl->ulLocalBbtAddr[ucTLun] + LOGIC_PG_SZ * (ptRdRdtPgMg->bsPage % RDT_PG_NUM)),
                    (U32 *)(ptRdRdtPgMg->DataBuffAddr + LOGIC_PG_SZ * (ptRdRdtPgMg->bsPage % RDT_PG_NUM)), LOGIC_PG_SZ / sizeof(U32));

                ptRdRdtPgMg->bsPage++;
                if (0 == ptRdRdtPgMg->bsPage % RDT_PG_NUM)
                {
                    ptRdRdtPgMg->bsOpStage = RDT_SORT_FINISH;
                }
                else
                {
                    ptRdRdtPgMg->bsOpStage = RDT_SORT_READ;
                }
            }
            else if (SUBSYSTEM_STATUS_EMPTY_PG == *(U8 *)ptRdRdtPgMg->FlashStatusBuffAddr)
            {
                if (0 == ptRdRdtPgMg->bsPage)
                {
                    ptRdRdtPgMg->bsPage = INVALID_4F;
                    ptRdRdtPgMg->bsPln = 1;
                    ptRdRdtPgMg->bsOpStage = RDT_SORT_FINISH;
                }
                else
                {
                    DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Load Rdt Empty-Page.\n", ucTLun, ptRdRdtPgMg->bsPln, ptRdRdtPgMg->bsBlk, ptRdRdtPgMg->bsPage);
                    DBG_Getch();
                }
            }
            else if (SUBSYSTEM_STATUS_FAIL == *(U8 *)ptRdRdtPgMg->FlashStatusBuffAddr)
            {
                DBG_Printf("Lun#%d Pln#%d Blk#%d Pg#%d Bbt Load Rdt Retry-Fail.\n", ucTLun, ptRdRdtPgMg->bsPln, ptRdRdtPgMg->bsBlk, ptRdRdtPgMg->bsPage);
                DBG_Getch();
            }
            else
            {
                ;// wait flash status...
            }
            break;
        }
        case RDT_SORT_FINISH:
        {
            if (TRUE == RDT_WaitAllOpStageDone(RDT_SORT_FINISH))
            {
                return TRUE; // return immediately.
            }
            break;
        }
        default:
        {
            DBG_Printf("Lun#%d Load Stage %d Error.\n", ucTLun, ptRdRdtPgMg->bsOpStage);
            DBG_Getch();
        }
        }
    }

    return bLoadBbtDone;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_CompareResult(void)
{
    U8 ucTLun;
    U16 usBlk, usLocalBbtSz;
    U32 usBadBlkCnt;
    RDT_HEAD *ptHeader;
    RDT_PAYLOAD_ENTRY *ptPayload;
    U32 RdtPayloadAddr;

    usLocalBbtSz = RDT_CalculateBitMapOffSet(BLK_PER_PLN + RSV_BLK_PER_PLN, PLN_PER_LUN);

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        ptHeader = (RDT_HEAD*)(l_ptRdtSortMg[ucTLun].DataBuffAddr + usLocalBbtSz);
        usBadBlkCnt = ptHeader->usBadBlkCnt;
        for (usBlk = 0; usBlk < usBadBlkCnt; usBlk++)
        {
            RdtPayloadAddr = l_ptRdtSortMg[ucTLun].DataBuffAddr + usLocalBbtSz + sizeof(RDT_HEAD) + sizeof(RDT_PAYLOAD_ENTRY) * usBlk;
            ptPayload = (RDT_PAYLOAD_ENTRY*)(RdtPayloadAddr);
            if (ptPayload->ucErrType != RDT_ERASE_ERR &&
                ptPayload->ucErrType != RDT_WRITE_ERR &&
                ptPayload->ucErrType != RDT_READ_ERR  &&
                ptPayload->ucErrType != RDT_IDB_ERR)
            {
                DBG_Printf("Compare fail. TlunL%d Stage:%d Blk:%d Pln:%d ErrType:%d\n", ucTLun, ptPayload->ucStage, ptPayload->usBlk, ptPayload->ucPln, ptPayload->ucErrType);
                DBG_Getch();
            }
        }
    }

    return TRUE;
}

LOCAL MCU1_DRAM_TEXT BOOL RDT_CheckResult(void)
{
    BOOL bBbtLoadRdtDone = FALSE;
    LOCAL U32 s_ulBbtLoadStage = RDT_CHECK_RESULT_INIT;

    switch (s_ulBbtLoadStage)
    {
    case RDT_CHECK_RESULT_INIT:
    {
        RDT_SortMgInit();
        s_ulBbtLoadStage = RDT_CHECK_RESULT_FIND_TARGET;
        break;
    }
    case RDT_CHECK_RESULT_FIND_TARGET:
    {
        if (TRUE == RDT_FindRdtTarget())
        {
            DBG_Printf("Find Rdt target pass!\n");
            s_ulBbtLoadStage = RDT_CHECK_RESULT_READ_TARGET;
        }
        break;
    }
    case RDT_CHECK_RESULT_READ_TARGET:
    {
        if (TRUE == RDT_ReadRdtPage())
        {
            DBG_Printf("Read Rdt target pass!\n");
            s_ulBbtLoadStage = RDT_CHECK_RESULT_CHECK_TARGET;
        }
        break;
    }
    case RDT_CHECK_RESULT_CHECK_TARGET:
    {
        if (TRUE == RDT_CompareResult())
        {
            DBG_Printf("Compare Rdt result pass!\n");
        }
        s_ulBbtLoadStage = RDT_CHECK_RESULT_FINISH;
        break;
    }
    case RDT_CHECK_RESULT_FINISH:
    {
        bBbtLoadRdtDone = TRUE;
        s_ulBbtLoadStage = RDT_CHECK_RESULT_INIT;
        break;
    }
    default:
    {
        DBG_Printf("Load Stage %d Error.\n", s_ulBbtLoadStage);
        DBG_Getch();
    }
    }

    return bBbtLoadRdtDone;
}
#endif

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_RDTTest
Return Val :
Discription: only for sortting the bad block before burning test
Usage      :
History    :
1. 2016.4.27 JasonGuo create function
==============================================================================*/
BOOL MCU1_DRAM_TEXT L3_RDTTest(void)
{
    BOOL bRDTTestDone = FALSE;
    LOCAL BOOL l_bRDTTestStage = RDT_INIT;

    switch (l_bRDTTestStage)
    {
    case RDT_INIT:
    {
#ifdef RDT_FULL_BLOCK_TIME_STATISTIC
        l_ulTimeTag1 = HAL_GetMCUCycleCount();
#endif
        RDT_MemInit();
        RDT_Init();
        RDT_SortMgInit();
        RDT_UpdateRECCThrd();
        RDT_TurnOffHardDecoder();
        l_bRDTTestStage = RDT_FORMATGB;
        DBG_Printf("RDT Init PASS!\n");
        break;
    }
    case RDT_FORMATGB:
    {
        if (TRUE == RDT_FormatGB())
        {
            RDT_SortMgInit();
            l_bRDTTestStage = RDT_SEARCH;
            DBG_Printf("RDT Format GB PASS!\n");
        }
        break;
    }
    case RDT_SEARCH:
    {
        if (TRUE == RDT_RdtPageInit())
        {
            RDT_SortMgInit();
            l_bRDTTestStage = RDT_IDB;
            if (TRUE == l_ptRdtCtrl->tRdtHead.ucInherit)
            {
                RDT_PrintAllBadBlk();
                RDT_PrintBadBlkCnt();
            }
            DBG_Printf("RDT Page Init PASS!\n\n");
        }
        break;
    }
    case RDT_IDB:
    {
        if (TRUE == RDT_Idb())
        {
            RDT_FormatDataSpareBuf();
            RDT_FormatProOrder();
            if (FALSE == l_ptRdtCtrl->tRdtHead.ucByPassRDTEn)
            {
                RDT_TestMgInit();
                l_bRDTTestStage = RDT_TEST;
            }
            else
            {
                RDT_SortMgInit();
                l_bRDTTestStage = RDT_SAVE;
            }
            DBG_Printf("RDT IDB PASS!\n");
#ifdef RDT_FULL_BLOCK_TIME_STATISTIC
            l_ulTimeTag2 = HAL_GetMCUCycleCount();
#endif
        }
        break;
    }
    case RDT_TEST:
    {
        if (TRUE == RDT_Test())
        {
#ifdef RDT_FULL_BLOCK_TIME_STATISTIC
        l_ulTimeTag3 = HAL_GetMCUCycleCount();
#endif
            RDT_SortMgInit();
            l_bRDTTestStage = RDT_SAVE;
            DBG_Printf("RDT Test PASS!\n");
        }
        break;
    }
    case RDT_SAVE:
    {
        if (TRUE == RDT_Save_Result())
        {
            DBG_Printf("\nRDT Save PASS!\n");
            DBG_Printf("\nThe Newest Test Result:\n");
            RDT_PrintBadBlkCnt();
            RDT_PrintAllBadBlk();

#ifdef RDT_FCMDQ_STATISTIC
            U8 ucIndex1;
            for (ucIndex1 = 0; ucIndex1 < SUBSYSTEM_LUN_NUM; ucIndex1++)
            {
                DBG_Printf("FCMDQ Tlun%d Erase-full[%d-%d- %d%%], Write-full[%d-%d- %d%%], Read-full[%d-%d- %d%%], All-full[%d-%d- %d%%]\n", ucIndex1,
                    g_aFCMDStatus[ucIndex1][2], g_aFCMDStatus[ucIndex1][5], g_aFCMDStatus[ucIndex1][2] * 100 / g_aFCMDStatus[ucIndex1][5],
                    g_aFCMDStatus[ucIndex1][0], g_aFCMDStatus[ucIndex1][3], g_aFCMDStatus[ucIndex1][0] * 100 / g_aFCMDStatus[ucIndex1][3],
                    g_aFCMDStatus[ucIndex1][1], g_aFCMDStatus[ucIndex1][4], g_aFCMDStatus[ucIndex1][1] * 100 / g_aFCMDStatus[ucIndex1][4],
                    g_aFCMDStatus[ucIndex1][6], g_aFCMDStatus[ucIndex1][7], g_aFCMDStatus[ucIndex1][6] * 100 / g_aFCMDStatus[ucIndex1][7]);
            }
#endif

            RDT_SortMgInit();
            l_bRDTTestStage = RDT_FINISH;
#ifdef RDT_CHECK_RESULT
            l_bRDTTestStage = RDT_CHK_RESULT;
#endif
        }
        break;
    }
    case RDT_CHK_RESULT:
    {
#ifdef RDT_CHECK_RESULT
        if (TRUE == RDT_CheckResult())
        {
            DBG_Printf("\nThe RDT_Check Result:\n");
            RDT_PrintBadBlkCnt();
            RDT_PrintAllBadBlk();
            RDT_SortMgInit();
            l_bRDTTestStage = RDT_FINISH;

        }
#endif
        break;
    }
    case RDT_FINISH:
    {

#ifdef RDT_ERR_STATISTIC
        U8 ucIndex;
        for (ucIndex = 0; ucIndex < l_ptRdtCtrl->ulRdtTestLunNumCnt; ucIndex++)
        {
            DBG_Printf("TLun:%d EraErr:%d WtErr:%d Uecc:%d Recc:%d AllErr:%d\n", ucIndex, l_tErrNum[ucIndex].EraseErrNum, l_tErrNum[ucIndex].WriteErrNum, l_tErrNum[ucIndex].UeccNum, l_tErrNum[ucIndex].ReccNum, l_tErrNum[ucIndex].AllErrNum);
        }
#endif

        l_bRDTTestStage = RDT_INIT;
        RDT_TestEnd();
        bRDTTestDone = TRUE;

        DBG_Printf("\nRdtRunTime = %d s\n", l_ulLedRunCount_Sec);
        DBG_Printf("\nL3_RDTTest Finish.\n\n");
#ifdef RDT_FULL_BLOCK_TIME_STATISTIC
        l_ulTimeTag4 = HAL_GetMCUCycleCount();
        l_ulCountInOneUs = HAL_GetMcuClock() / 1000000;

        U32 ulOpTime = (U32)COM_DiffU32(l_ulTimeTag2, l_ulTimeTag3) / l_ulCountInOneUs / 1000;
        U32 ulRdtTime = (U32)COM_DiffU32(l_ulTimeTag1, l_ulTimeTag4) / l_ulCountInOneUs / 1000;
        DBG_Printf("\nOpTime = %d ms\n", ulOpTime);
        DBG_Printf("\nRdtTime =  %d ms\n", ulRdtTime);

        U32 ulCalcFullTime = ulOpTime * 547 / 2 + ulRdtTime - ulOpTime;
        DBG_Printf("\nCalcFullTime = %d ms = %d s \n", ulCalcFullTime, ulCalcFullTime / 1000);
#endif

#ifdef RDT_EWR_TIME_STATISTIC
        l_ulCountInOneUs = HAL_GetMcuClock() / 1000000;
        U32 ulEraseTime = (U32)COM_DiffU32(l_ulTimeTag5, l_ulTimeTag6) / l_ulCountInOneUs;
        U32 ulWriteTime = (U32)COM_DiffU32(l_ulTimeTag6, l_ulTimeTag7) / l_ulCountInOneUs;
        U32 ulReadTime = (U32)COM_DiffU32(l_ulTimeTag7, l_ulTimeTag8) / l_ulCountInOneUs;
        DBG_Printf("EraaseTime = %d us\n", ulEraseTime);
        DBG_Printf("WriteTime  = %d us\n", ulWriteTime);
        DBG_Printf("ReadTime   = %d us\n", ulReadTime);
#endif

        while (1);
        break;
    }
    default:
    {
        //DBG_Printf("L3_RDTTest Status=%d Err.\n", l_bRDTTestStage);
        RDT_CancelTimer();
        RDT_Led_Off();
        DBG_Getch();
    }
    }

    return bRDTTestDone;
}

/*====================End of this file========================================*/


