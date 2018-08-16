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
#include "BaseDef.h"
#include "HAL_HGE.h"
#include "HAL_MemoryMap.h"
#include "COM_Memory.h"
//#include "TEST_NfcMain.h"
#include "HAL_FlashCmd.h"
#include "TEST_HGE.h"
#include "TEST_NfcFuncBasic.h"

//error bit count MORE to LESS (80 , 78 is good)
GLOBAL U32 g_HgeErrorInjBitCnt[RETRY_TIME_MAX] = {
    185, 145, 115, 95, 85, 80, 78, 70, 50, 20 
};

extern GLOBAL HGE_RESULTS *g_HgeResults;
extern GLOBAL BOOL g_bLbaChk;
extern GLOBAL BOOL g_bRedOntf;

GLOBAL U32 g_ulHgeAddr;

/*------------------------------------------------------------------------------
Name: TEST_HgePrepareData
Description:
    Prepare fake dram data for HGE (MLC)
Input Param:
    U32* TargetAddr
    U32 LengthDW
    U8 ucLowHigh : MLC(0=low;1=high) TLC(0=low;1=mid;2=high)
Output Param:
    none
Return Value:
    none
Usage:
History:
20160316   Henry Chen    create
------------------------------------------------------------------------------*/
void TEST_HgePrepareData(U32* TargetAddr, U32 LengthDW, U8 ucLowHigh)
{
    switch(ucLowHigh) {
        case 0:
            COM_MemSet(TargetAddr, LengthDW, 0x55aa0000);
            break;
        case 1:
            COM_MemSet(TargetAddr, LengthDW, 0xffff0000);
            break;
        case 2:
            COM_MemSet(TargetAddr, LengthDW, 0xffffffff);
            break;
        default:
            DBG_Printf("[TEST_HgePrepareData] Invalid low/mid/high page index.\n");
            break;
    }

    return;
}


/*------------------------------------------------------------------------------
Name: TEST_HgeErrorInject
Description:
    Inject error on prepared dram data for HGE
    Inject continuous error bit from TargetAddr within ulErrRangeLengthDW
Input Param:
    U32* TargetAddr : Start address
    U32 ulErrRangeLengthDW : Error inject length(in DW) from TargetAddr
    U8 ulInjErrBitCnt : Error inject bit count
Output Param:
    none
Return Value:
    none
Usage:
History:
20160323    Henry Chen  create
------------------------------------------------------------------------------*/
void TEST_HgeErrorInject(U32* TargetAddr, U32 ulErrRangeLengthDW, U32 ulInjErrBitCnt)
{
    U8 ucBit = 0;
    U32 ulBitCnt = 0;

    if (ulInjErrBitCnt > 0) {
        if (ulErrRangeLengthDW > ((ulInjErrBitCnt - 1)/32)) {
            while(ulBitCnt < ulInjErrBitCnt) {
                if (ulBitCnt%32 == 0)
                    TargetAddr += (ulBitCnt/32);
                (*TargetAddr) ^= (1<<(ulBitCnt%32));
                ulBitCnt++;
            }
        } else
            DBG_Printf("[TEST_HgeErrorInject] Invalid Inject Error Bit Count.\n");
    }

    return;
}

#ifdef TEST_HGE_NORMAL
BOOL TEST_HgeRawReadWL(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U16 usSecLen, BOOL bsSinglePln)
{
    NFC_READ_REQ_DES tRdReq = {0};
    U8  ucPageType;
    tRdReq.pErrInj = NULL;
    
    if (FALSE != g_bDecFifoEn)
    {
        tRdReq.bsDecFifoEn = TRUE;
    }
    tRdReq.bsSecStart = ucSecStart;
    tRdReq.bsSecLen   = usSecLen;
    tRdReq.bsRedOntf  = g_bRedOntf;
    tRdReq.bsLbaChkEn = g_bLbaChk;
    
    tRdReq.bsTlcMode  = TRUE;
    tRdReq.bsRawData  = TRUE;
    
    for(ucPageType = 0; ucPageType < 3; ucPageType++)
    {
        tRdReq.bsTlcPgType  = ucPageType;
        tRdReq.bsRdBuffId = TEST_NfcGetRdBufId(pFlashAddr, ucPageType);// + ucPageType;
        tRdReq.bsLba        = TEST_NfcGetReadLba(pFlashAddr, tRdReq.bsSecStart);
        tRdReq.ppNfcRed     = (NFC_RED **)&pRRed[TEST_NfcGetRedOffset(pFlashAddr)];
        while (TRUE == HAL_NfcGetFull(pFlashAddr->ucPU, pFlashAddr->ucLun))
        {
            ;
        }
        
        if (TRUE == bsSinglePln)
        {
            HAL_NfcSinglePlnRead(pFlashAddr, &tRdReq, FALSE);
        }
        else
        {
            DBG_Printf("Not a single plane operation.\n");
            DBG_Getch();
        }
        
        if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
        {
            HAL_NfcResetCmdQue(pFlashAddr->ucPU, pFlashAddr->ucLun);
            HAL_NfcClearINTSts(pFlashAddr->ucPU, pFlashAddr->ucLun);
        }
        //DBG_Printf("Pu %d LUN %d BLK%d PG%d PageType%d Read OK!\n",pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usBlock
        //   ,pFlashAddr->usPage,ucPageType);
    }

    return TRUE;
}

void TEST_HgeShiftRawRead(FLASH_ADDR *pFlashAddr, U8 ucRetryTime, BOOL bsSinglePln, BOOL bsTlcMode)
{
    BOOL ucRawReadSts;
    NFC_READ_REQ_DES tRdReq = {0};
    RETRY_TABLE tRetryPara;
    U8 ucParaNum;
        
    /*RetryStep1. Retry PreConditon*/
    HAL_FlashRetryPreConditon(pFlashAddr);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("ReadRetry-PreConditon PU#%d Fail! \n", pFlashAddr->ucPU);
        DBG_Getch();
    }
 
    /*RetryStep2. Soft DEC ShiftRead Table Parameter Setting*/
    U32 ulIndex = HAL_FlashSelRetryPara(bsTlcMode);
    tRetryPara = HAL_FlashGetRetryParaTab(ulIndex);
    ucParaNum = (FALSE != tRdReq.bsTlcMode) ? HAL_TLC_FLASH_RETRY_PARA_MAX : HAL_FLASH_RETRY_PARA_MAX;
    HAL_FlashRetrySendParam(pFlashAddr, &tRetryPara, tRdReq.bsTlcMode, ucParaNum);

    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("ReadRetry-SendParam PU#%d Fail! \n", pFlashAddr->ucPU);
        DBG_Getch();
    }

    /*RetryStep3. Retry Enable*/
    HAL_FlashRetryEn(pFlashAddr, TRUE);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("ReadRetry-SendRetryCmd PU#%d Fail! \n", pFlashAddr->ucPU);
        DBG_Getch();
    }
    
    /*RetryStep4. redo read:*/
    #ifdef TLC_MODE_TEST
    ucRawReadSts = TEST_HgeRawReadWL(pFlashAddr, 0, SEC_PER_LOGIC_PG, TRUE);
    #else
    tRdReq.bsSecStart = 0;
    tRdReq.bsSecLen   = SEC_PER_LOGIC_PG;
    tRdReq.bsRdBuffId = TEST_NfcGetRdBufId(pFlashAddr, 0);
    tRdReq.bsRawData = TRUE;
    
    ucRawReadSts = HAL_NfcSinglePlnRead(pFlashAddr, &tRdReq, FALSE);
    #endif
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("ReadRetry-Partial read PU#%d Fail! \n", pFlashAddr->ucPU);
        DBG_Getch();
    }

    // RetryStep5. terminate
    HAL_FlashRetryTerminate(pFlashAddr->ucPU, pFlashAddr->ucLun, FALSE);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("ReadRetry-Partial read PU#%d Fail! \n", pFlashAddr->ucPU);
        DBG_Getch();
    }
    
    return;
}

void TEST_HgeMlcReadAll(U8 ucSecStart, U16 usSecLen, READ_REQ_TYPE ReadType, BOOL bsSinglePln)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_READ_REQ_DES tRdReq = {0};
    DEC_SRAM_STATUS_ENTRY tDecSramSts = {0};
    BOOL bTlcMode  = FALSE;
    U16 usBlkStart = TEST_BLOCK_START;
    U16 usBlkEnd   = TEST_BLOCK_END; 

    U8 i, ucVtIndex, ucRetryTime;
    HGE_DESC *HgeDescriptor = { 0 };
    U32 ulRdDramAddr[2];
    U32 ulTestLength = 0x200;//8QW Aligned
    HGE_READ_ADDRESS_INFO g_HgeReadAddrInfo = {0};
    U32 ulReadBuffId;
    
#ifdef TLC_MODE_TEST
    DBG_Printf("Should not be TLC Mode!!\n");
    DBG_Getch();
#endif
    tRdReq.bsSecStart = ucSecStart;
    tRdReq.bsSecLen   = usSecLen;
    tRdReq.bsRedOntf  = g_bRedOntf;
    tRdReq.bsLbaChkEn = g_bLbaChk;
    tRdReq.bsTlcMode  = bTlcMode;
    
    tFlashAddr.usBlock = TEST_BLOCK_START;
    while (tFlashAddr.usBlock < TEST_BLOCK_END)
    {
        tFlashAddr.ucPU = TEST_PU_START;
        while (tFlashAddr.ucPU < TEST_PU_END)
        {
            tFlashAddr.ucLun = TEST_LUN_START;
            while (tFlashAddr.ucLun < TEST_LUN_END)
            {
                for (tFlashAddr.usPage = TEST_PAGE_START; tFlashAddr.usPage < TEST_PAGE_END; tFlashAddr.usPage++)
                {
                    while (TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
                    {
                        ;
                    }
                    
                    ucRetryTime = 0;
                    COM_MemZero((U32*)g_HgeResults, (RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS))>>2);
                    
                    while (ucRetryTime < RETRY_CNT)
                    {
                        //ulPairPage = HAL_GetPairPage(tFlashAddr.usPage);
                        for (i = 0; i < 2; i++) 
                        {
                            //Pending, currently just read the same page twice
                            TEST_HgeShiftRawRead(&tFlashAddr, ucRetryTime, TRUE, bTlcMode);
                            ulReadBuffId = TEST_NfcGetRdBufId(&tFlashAddr, 0) + i;
                            ulRdDramAddr[i] = (HAL_NfcGetDmaAddr(ulReadBuffId,0,LOGIC_PG_SZ_BITS) << 1) + DRAM_START_ADDRESS;
                            HAL_HgeSetReadAddrInfo((HGE_READ_ADDRESS_INFO *)&g_HgeReadAddrInfo, ulRdDramAddr[i], i);
                        }

                        for (ucVtIndex = 0; ucVtIndex < HGE_MLC_TYPE_LOOPTIME; ucVtIndex++) 
                        {
                            HgeDescriptor = (HGE_DESC *)HAL_HgeGetValidDescriptor(HAL_GetMcuId());

                            HAL_HgeSetDescriptor(HgeDescriptor, PATTERN_IDX_MLC_BR+ ucVtIndex, 
                                    g_HgeReadAddrInfo, ucVtIndex, ulTestLength);

                            HAL_HgeRegTrigger(HAL_GetMcuId());

                            HAL_HgeWaitDescriptorDone(HgeDescriptor);
                            
                            HAL_HgeGetDescriptorResultsMlc(HgeDescriptor, ucRetryTime);

                            if (i == PATTERN_IDX_MLC_BR)
                                HAL_HgeCheckPatternMatchSLC((U32*)ulRdDramAddr[0], ulTestLength, ucRetryTime,HgeDescriptor);
                            else if (i == PATTERN_IDX_MLC_AR_CR) 
                                HAL_HgeCheckPatternMatchMLC((U32*)ulRdDramAddr[0], (U32*)ulRdDramAddr[1], ulTestLength, ucRetryTime, HgeDescriptor);
                            
                            COM_MemZero((U32*)HgeDescriptor, sizeof(HGE_DESC)>>2);
                        }
                        
                        ucRetryTime++;
                    }
                    
                    //HAL_PrintHgeResults();
                    for(ucVtIndex = 0; ucVtIndex < HGE_MLC_TYPE_VT_CNT; ucVtIndex++) 
                    {
                        U8 ucResult = HAL_HgeDataAnalysis(AR+ucVtIndex);
                        DBG_Printf("%d. Result = %d\n", ucVtIndex, ucResult);
                    }
                    DBG_Printf("MCU%d Pu#%d Lun#%d Blk#%d Pg#%d HGE Test Done.\n",HAL_GetMcuId(),
                            tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock, tFlashAddr.usPage);
                }
                tFlashAddr.ucLun++;
            }
            tFlashAddr.ucPU++;
        }
        tFlashAddr.usBlock++;
    }
    
    return;
}

void TEST_HgeTlcReadAll(U8 ucSecStart, U16 usSecLen, READ_REQ_TYPE ReadType, BOOL bsSinglePln)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_READ_REQ_DES tRdReq = {0};
    DEC_SRAM_STATUS_ENTRY tDecSramSts = {0};
    BOOL bTlcMode  = FALSE;
    U16 usBlkStart = TEST_BLOCK_START;
    U16 usBlkEnd   = TEST_BLOCK_END; 

    U8 ucPageType, ucVtIndex, ucRetryTime;
    HGE_DESC *HgeDescriptor = { 0 };
    U32 ulRdDramAddr[3];
    U32 ulTestLength = 0x200;//8QW Aligned
    HGE_READ_ADDRESS_INFO g_HgeReadAddrInfo = {0};
    U32 ulReadBuffId;

    tRdReq.bsSecStart = ucSecStart;
    tRdReq.bsSecLen   = usSecLen;
    tRdReq.bsRedOntf  = g_bRedOntf;
    tRdReq.bsLbaChkEn = g_bLbaChk;
    tRdReq.bsTlcMode  = bTlcMode;

    tFlashAddr.usBlock = TEST_BLOCK_START;
    while (tFlashAddr.usBlock < TEST_BLOCK_END)
    {
        tFlashAddr.ucPU = TEST_PU_START;
        while (tFlashAddr.ucPU < TEST_PU_END)
        {
            tFlashAddr.ucLun = TEST_LUN_START;
            while (tFlashAddr.ucLun < TEST_LUN_END)
            {
                for (tFlashAddr.usPage = TEST_PAGE_START; tFlashAddr.usPage < TEST_PAGE_END; tFlashAddr.usPage++)
                {
                    TEST_NfcSetTraceFlashAddr(&tFlashAddr);

                    while (TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
                    {
                        ;
                    }

                    ucRetryTime = 0;
                    COM_MemZero((U32*)g_HgeResults, (RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS))>>2);
                    
                    while (ucRetryTime < TLC_RETRY_CNT)
                    {
                        TEST_HgeShiftRawRead(&tFlashAddr, ucRetryTime, TRUE, bTlcMode);
                        
                        for (ucPageType = 0; ucPageType < TEST_PG_PER_WL; ucPageType++) 
                        {
                            ulReadBuffId = TEST_NfcGetRdBufId(&tFlashAddr, ucPageType);// + ucPageType;
                            ulRdDramAddr[ucPageType] = (HAL_NfcGetDmaAddr(ulReadBuffId,0,LOGIC_PG_SZ_BITS) << 1) + DRAM_START_ADDRESS;
                            HAL_HgeSetReadAddrInfo((HGE_READ_ADDRESS_INFO *)&g_HgeReadAddrInfo, ulRdDramAddr[ucPageType], ucPageType);
                        }

                        //AR & BR & CR & DR, ER & FR & GR
                        for (ucVtIndex = 0; ucVtIndex < HGE_TLC_TYPE_LOOPTIME; ucVtIndex++) 
                        {
                            HgeDescriptor = (HGE_DESC *)HAL_HgeGetValidDescriptor(HAL_GetMcuId());
                                
                            HAL_HgeSetDescriptor(HgeDescriptor, PATTERN_IDX_TLC_AR_BR_CR_DR + ucVtIndex, 
                                    g_HgeReadAddrInfo, 2, ulTestLength);

                            HAL_HgeRegTrigger(HAL_GetMcuId());

                            HAL_HgeWaitDescriptorDone(HgeDescriptor);

                            HAL_HgeGetDescriptorResultsTlc(HgeDescriptor, ucRetryTime , ucVtIndex);

                            HAL_HgeCheckPatternMatchTLC((U32*)ulRdDramAddr[0], (U32*)ulRdDramAddr[1], (U32*)ulRdDramAddr[2], 
                                    ulTestLength, ucRetryTime ,ucVtIndex ,HgeDescriptor);
                            
                            COM_MemZero((U32*)HgeDescriptor, sizeof(HGE_DESC)>>2);
                        }
                        
                        ucRetryTime++;
                    }
                    
                    //HAL_PrintHgeResults();
                    for(ucVtIndex = 0; ucVtIndex < HGE_TLC_TYPE_VT_CNT; ucVtIndex++) 
                    {
                        U8 ucResult = HAL_HgeDataAnalysis(AR+ucVtIndex);
                        DBG_Printf("%d. Result = %d\n", ucVtIndex, ucResult);
                    }
                    DBG_Printf("MCU%d Pu#%d Lun#%d Blk#%d Pg#%d HGE Test Done.\n",HAL_GetMcuId(),
                        tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock, tFlashAddr.usPage);
                }
            }
        }
    }

    return;
}
/*------------------------------------------------------------------------------
Name: Test_HgeEngineMlc
Description:
    test histogram for MLC
Input Param:
Output Param:
    none
Return Value:
    none
Usage:
History:
20160111   Henry Chen    create
------------------------------------------------------------------------------*/
void Test_HgeEngineMlc(void)
{
    //Erase
    TEST_NfcEraseAll(); //Test FW works
    DBG_Printf("NFC Erase All done.\n\n");
    
    //Program
    TEST_NfcWriteAll(SING_PLN_WRITE);
    DBG_Printf("NFC Write All done.\n\n");

    TEST_HgeMlcReadAll(0, SEC_PER_LOGIC_PG, SING_PLN_READ, TRUE);
    DBG_Printf("HGE Test All done.\n\n");

    return;
}


/*------------------------------------------------------------------------------
Name: Test_HgeEngineTlc
Description:
    test histogram for MLC
Input Param:
Output Param:
    none
Return Value:
    none
Usage:
History:
20160111   Henry Chen    create
------------------------------------------------------------------------------*/
void Test_HgeEngineTlc(void)
{    
    //Erase
    TEST_NfcEraseAll(); //Test FW works
    DBG_Printf("NFC Erase All done.\n\n");
    
    //Program
    TEST_NfcWriteAll(SING_PLN_WRITE);
    DBG_Printf("NFC Write All done.\n\n");

    TEST_HgeTlcReadAll(0, SEC_PER_LOGIC_PG, SING_PLN_READ, TRUE);
    DBG_Printf("HGE Test All done.\n\n");

    return;
}

#endif
/*------------------------------------------------------------------------------
Name: TEST_HgeEngineMlcSkipEwr
Description:
    test histogram for MLC without access nand flash
Input Param:
    U8 ucEnableErrInj : enable error injection or not
Output Param:
    none
Return Value:
    none
Usage:
History:
20160316   Henry Chen    create
------------------------------------------------------------------------------*/
void TEST_HgeEngineMlcSkipEwr(U8 ucEnableErrInj)
{
    FLASH_ADDR tFlashAddr = {0};
    U16 usRdBufId = START_RBUF_ID;
    U8 ucRetryTime = 0;
    U8 i, ucResult;
    //U16 ulPairPage;
    HGE_DESC *HgeDescriptor;
    U32 ulRdAddr[2];
    U32 ulTestLength = 0x200;//length tmp 8QW(512B)
    HGE_READ_ADDRESS_INFO g_HgeReadAddrInfo = {0};

    COM_MemZero((U32*)g_HgeResults, (RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS))>>2);
    
    while (ucRetryTime < HGE_TEST_LOOP) 
    {
        for (i = 0; i < 2; i++) 
        {
            #ifdef RANDOM_DATA_PATTERN
            ulRdAddr[i] = g_ulHgeAddr + PHYPG_SZ*(i + 1);
            g_ulHgeAddr = ulRdAddr[i];
            if ((*(U32*)g_ulHgeAddr) == 0 )
            {
                g_ulHgeAddr = 0x54a00000;
            }
            #else
            ulRdAddr[i] = DRAM_START_ADDRESS + 0x4000*(i + 1);
            TEST_HgePrepareData((U32*)ulRdAddr[i], ulTestLength/4, i);
            #endif
            
            //Error inject
            if (ucEnableErrInj)
            {
                TEST_HgeErrorInject((U32*)ulRdAddr[i], ulTestLength/4, g_HgeErrorInjBitCnt[ucRetryTime]);
            }
            
            //prepare data for BR , AR&CR in DRAM
            HAL_HgeSetReadAddrInfo((HGE_READ_ADDRESS_INFO *)&g_HgeReadAddrInfo, ulRdAddr[i], i);
        }

        //Getting & Setting Descriptor & Trigger & Get Results
        for(i = 0; i < HGE_MLC_TYPE_LOOPTIME; i++) {//BR , AR&CR
            
            HgeDescriptor = (HGE_DESC *)HAL_HgeGetValidDescriptor(HAL_GetMcuId());

            HAL_HgeSetDescriptor(HgeDescriptor, PATTERN_IDX_MLC_BR + i, g_HgeReadAddrInfo, i, ulTestLength);

            HAL_HgeRegTrigger(HAL_GetMcuId());

            HAL_HgeWaitDescriptorDone(HgeDescriptor);
            
            HAL_HgeGetDescriptorResultsMlc(HgeDescriptor, ucRetryTime);

            //Check HGE count result with FW count result
            if (i == PATTERN_IDX_MLC_BR)
                HAL_HgeCheckPatternMatchSLC((U32*)ulRdAddr[0], ulTestLength, ucRetryTime,HgeDescriptor);
            else if (i == PATTERN_IDX_MLC_AR_CR) 
                HAL_HgeCheckPatternMatchMLC((U32*)ulRdAddr[0], (U32*)ulRdAddr[1], ulTestLength, ucRetryTime, HgeDescriptor);

            //Maple move Desc clr here
            COM_MemZero((U32*)HgeDescriptor, sizeof(HGE_DESC)>>2);    
        }
        ucRetryTime++;

    }
    
    //HAL_PrintHgeResults();

    //Analysis data & print result
    for(i = 0; i < HGE_MLC_TYPE_VT_CNT; i++) 
    {
        ucResult = HAL_HgeDataAnalysis(AR+i);
        DBG_Printf("%d. Result = %d\n", i, ucResult);
    }

    return;
}

/*------------------------------------------------------------------------------
Name: TEST_HgeEngineTlcSkipEwr
Description:
    test histogram for TLC without access nand flash
Input Param:
    U8 ucEnableErrInj : enable error injection or not
Output Param:
    none
Return Value:
    none
Usage:
History:
20160323   Henry Chen    create
------------------------------------------------------------------------------*/
void TEST_HgeEngineTlcSkipEwr(U8 ucEnableErrInj)
{
    FLASH_ADDR tFlashAddr = {0};
    U16 usRdBufId = START_RBUF_ID;//TEST_NfcGetRdBufId(&tFlashAddr);
    U8 ucRetryTime = 0;
    U8 i, ucResult;
    HGE_DESC *HgeDescriptor;
    U32 ulRdAddr[3];
    U32 ulTestLength = 0x200;//length tmp 8QW(512B)
    HGE_READ_ADDRESS_INFO g_HgeReadAddrInfo = {0};
    
    COM_MemZero((U32*)g_HgeResults, (RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS))>>2);
    
    while (ucRetryTime < HGE_TEST_LOOP) {

        //fake low/mid/high page data
        for (i = 0; i < 3; i++) 
        {
            #ifdef RANDOM_DATA_PATTERN
            ulRdAddr[i] = g_ulHgeAddr + PHYPG_SZ*(i + 1);
            g_ulHgeAddr = ulRdAddr[i];
            if ((*(U32*)g_ulHgeAddr) == 0 )
            {
                g_ulHgeAddr = 0x54a00000;
            }
            #else
            ulRdAddr[i] = DRAM_START_ADDRESS + 0x4000*(i + 1);
            TEST_HgePrepareData((U32*)ulRdAddr[i], ulTestLength, i);
            #endif

            //Error inject
            if (ucEnableErrInj)
                TEST_HgeErrorInject((U32*)ulRdAddr[i], ulTestLength/4, g_HgeErrorInjBitCnt[ucRetryTime]);

            //prepare data in DRAM
            HAL_HgeSetReadAddrInfo((HGE_READ_ADDRESS_INFO *)&g_HgeReadAddrInfo, ulRdAddr[i], i);
        }

        //Getting & Setting Descriptor & Trigger & Get Results
        for (i = 0; i < HGE_TLC_TYPE_LOOPTIME; i++) 
        {//AR & BR & CR & DR, ER & FR & GR
            HgeDescriptor = (HGE_DESC *)HAL_HgeGetValidDescriptor(HAL_GetMcuId());

            HAL_HgeSetDescriptor(HgeDescriptor, PATTERN_IDX_TLC_AR_BR_CR_DR + i, g_HgeReadAddrInfo, 2, ulTestLength);

            HAL_HgeRegTrigger(HAL_GetMcuId());

            HAL_HgeWaitDescriptorDone(HgeDescriptor);
            
            HAL_HgeGetDescriptorResultsTlc(HgeDescriptor, ucRetryTime , i);

            //Check HGE count result with FW count result
            HAL_HgeCheckPatternMatchTLC((U32*)ulRdAddr[0], (U32*)ulRdAddr[1], (U32*)ulRdAddr[2], ulTestLength, ucRetryTime ,i,HgeDescriptor);
            
            //Maple move Desc clr here
            COM_MemZero((U32*)HgeDescriptor, sizeof(HGE_DESC)>>2);
        }

        ucRetryTime++;

    }   
    
    //Analysis data & print result
    for(i = 0; i < HGE_TLC_TYPE_VT_CNT; i++) {
        ucResult = HAL_HgeDataAnalysis(AR+i);
        DBG_Printf("%d. Result = %d\n", i, ucResult);
    }

	return;
}

/*------------------------------------------------------------------------------
Name: TEST_HgeEngineMlcSkipEwrFifo
Description:
    test histogram for MLC without access nand flash (FIFO mode)
Input Param:
    U8 ucEnableErrInj : enable error injection or not
Output Param:
    none
Return Value:
    none
Usage:
History:
20160324   Henry Chen    create
------------------------------------------------------------------------------*/
void TEST_HgeEngineMlcSkipEwrFifo(U8 ucEnableErrInj)
{
    FLASH_ADDR tFlashAddr = {0};
    FIFO_Queue FIFO_Q[HGE_ENTRY_MAX*4] = {0};//for saving descriptor id
    U8 ucRetryTime = 0, ucFifoSlot = 0;
    U8 i, ucResult;
    //U16 ulPairPage;
    U16 usRdBufId = START_RBUF_ID;//TEST_NfcGetRdBufId(&tFlashAddr);
    U32 ulRdAddr[2];
    U32 ulTestLength = 0x200;//length tmp 8QW(512B)
    HGE_DESC *HgeDescriptor;
    HGE_DES_ID DesId;
    HGE_READ_ADDRESS_INFO g_HgeReadAddrInfo = {0};

    COM_MemZero((U32*)g_HgeResults, (RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS))>>2);
    
    while (ucRetryTime < HGE_TEST_LOOP) 
    {
        //fake low/high page data
        for (i = 0; i < 2; i++) 
        {
            //DRAM data need to be assigned for each descriptor. Can't use fixed DRAM address.
            ulRdAddr[i] = DRAM_START_ADDRESS + 0x4000*(ucFifoSlot + i);//+1?

            TEST_HgePrepareData((U32*)ulRdAddr[i], ulTestLength, i);

            //Error inject
            if (ucEnableErrInj)
                TEST_HgeErrorInject((U32*)ulRdAddr[i], ulTestLength/4, g_HgeErrorInjBitCnt[ucRetryTime]);

            //prepare data for BR , AR&CR in DRAM
            HAL_HgeSetReadAddrInfo((HGE_READ_ADDRESS_INFO *)&g_HgeReadAddrInfo, ulRdAddr[i], i);
        }

        //Getting & Setting Descriptor & Trigger & Get Results
        for(i = 0; i < HGE_MLC_TYPE_LOOPTIME; i++) 
        {//BR , AR&CR
            FIFO_Q[ucFifoSlot].ucDesId = HAL_HgeGetCurrentWp(HAL_GetMcuId());//save current wp
            FIFO_Q[ucFifoSlot].ucRetryTime = ucRetryTime;

            HgeDescriptor = (HGE_DESC *)HAL_HgeGetValidDescriptor(HAL_GetMcuId());

            HAL_HgeSetDescriptor(HgeDescriptor, PATTERN_IDX_MLC_BR + i, g_HgeReadAddrInfo, i, ulTestLength);

            HAL_HgeRegTrigger(HAL_GetMcuId());

            HAL_HgeWaitDescriptorDone(HgeDescriptor);
            
            ucFifoSlot++;

            //When meet last request or fifo full, we need to get our results back.
            if ((ucRetryTime == (HGE_TEST_LOOP - 1) && i == (HGE_MLC_TYPE_LOOPTIME - 1)) || (ucFifoSlot%HGE_ENTRY_MAX) == 0 ) 
            {
                for (i = 0; i < ucFifoSlot; i++) 
                {
                    DesId.ulDescMcu = HAL_GetMcuId();
                    DesId.ulDescID = FIFO_Q[i].ucDesId;
                    
                    HgeDescriptor = (HGE_DESC *)HAL_HgeGetDescriptorAddress(DesId);
                    
                    HAL_HgeGetDescriptorResultsMlc(HgeDescriptor, FIFO_Q[i].ucRetryTime);
                    
                    COM_MemZero((U32*)HgeDescriptor, sizeof(HGE_DESC)>>2);
                }
                ucFifoSlot = 0;
            }
        }

        ucRetryTime++;

    }
    
    //Analysis data & print result
    for(i = 0; i < HGE_MLC_TYPE_VT_CNT; i++) {
        ucResult = HAL_HgeDataAnalysis(AR+i);
        DBG_Printf("%d. Result = %d\n", i, ucResult);
    }

	return;
}

void UT_HgeMain(void)
{   
    DBG_Printf("[HGE_UnitTest] HGE_UnitTest Start\n");
    HAL_HgeDsAlloc((U32 *)DRAM_DATA_BUFF_MCU2_BASE);
    HAL_HgeRegInit();
    
    #ifdef RANDOM_DATA_PATTERN
    COM_WriteDataBuffInit();
    g_ulHgeAddr = 0x54a00000;
    DBG_Printf("Prepare random data done.\n\n");
    #endif

    #ifdef TEST_HGE_NORMAL
    U8 ucTestCase = HGE_MLC_NORMAL;
    TEST_NfcBasicInit();
    DBG_Printf("NFC Init Finish!\n\n");
    #else
    U8 ucTestCase = HGE_MLC_NO_READ;
    #endif
    
    switch (ucTestCase) 
    {
    #ifdef TEST_HGE_NORMAL
        case HGE_MLC_NORMAL:
            Test_HgeEngineMlc();
            break;
        case HGE_TLC_NORMAL:
            Test_HgeEngineTlc();
            break;
    #endif
        case HGE_MLC_NO_READ:
            TEST_HgeEngineMlcSkipEwr(FALSE);
            DBG_Printf("Test Case: HGE_MLC_NO_READ Done!\n");
            //break;
        case HGE_MLC_NO_READ_ERROR_INJ:
            TEST_HgeEngineMlcSkipEwr(TRUE);
            DBG_Printf("Test Case: HGE_MLC_NO_READ_ERROR_INJ Done!\n");
            //break;
        case HGE_TLC_NO_READ:
            TEST_HgeEngineTlcSkipEwr(FALSE);
            DBG_Printf("Test Case: HGE_TLC_NO_READ Done!\n");
            //break;
        case HGE_TLC_NO_READ_ERROR_INJ:
            TEST_HgeEngineTlcSkipEwr(TRUE);
            DBG_Printf("Test Case: HGE_TLC_NO_READ_ERROR_INJ Done!\n");
            //break;
        case HGE_MLC_NO_READ_FIFO:
            TEST_HgeEngineMlcSkipEwrFifo(FALSE);
            DBG_Printf("Test Case: HGE_MLC_NO_READ_FIFO Done!\n");
            //break;
        case HGE_MLC_NO_READ_ERROR_INJ_FIFO:
            TEST_HgeEngineMlcSkipEwrFifo(TRUE);
            DBG_Printf("Test Case: HGE_MLC_NO_READ_ERROR_INJ_FIFO Done!\n");
            break;
        default:
            DBG_Printf("[HGE_UnitTest] Invalid Test Case Number.\n");
            break;
    }
    DBG_Printf("[HGE_UnitTest] HGE_UnitTest End\n");

    return;
}

