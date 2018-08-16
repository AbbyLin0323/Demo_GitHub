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
* File Name    : TEST_LdpcSoftDecDriver.c
* Discription  : 
* CreateAuthor : Maple Xu
* CreateDate   : 2016.02.01
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "Disk_Config.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashCmd.h"
#include "HAL_NormalDSG.h"

#include "TEST_NfcFuncBasic.h"
#include "HAL_LdpcEngine.h"
#include "HAL_LdpcSoftDec.h"
#include "TEST_LdpcSoftDec.h"

#ifdef SIM
    #include "sim_flash_common.h"
#endif

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

#define Write_BUFID_START       1
#define READ_BUFID_START        300

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL MCU12_VAR_ATTR volatile SOFT_MCU1_CFG_REG *pSoftMcu1CfgReg;
extern GLOBAL MCU12_VAR_ATTR volatile SOFT_MCU2_CFG_REG *pSoftMcu2CfgReg;

extern GLOBAL MCU2_DRAM_TEXT SOFT_DECO_CW_SEGMENT g_aCodeWordSegment[DESCRIPTOR_MAX_NUM];
extern GLOBAL MCU12_VAR_ATTR volatile NFC_CMD_STS *g_pNfcCmdSts;

extern U32 TEST_NfcCalcuData(U32 ulPU, U32 ulPage, U32 ulSec);
extern void TEST_NfcRdCfgReqComm(NFC_READ_REQ_DES *pRdReq, U8 ucSecStart, U16 usSecLen);
extern void TEST_NfcBasicPattRunInit(void);

/*============================================================================*/
/* global region: declare global variable                                        */
/*============================================================================*/

GLOBAL U8 g_aShiftRdNTable[6] = {1, 2, 3, 4, 5, INVALID_2F};

/*Record DEC_STS of each Retry*/
GLOBAL MCU12_VAR_ATTR U8 g_SuccDescNum;
GLOBAL MCU12_VAR_ATTR BOOL g_aSuccDescId[DESCRIPTOR_MAX_NUM];
GLOBAL MCU12_VAR_ATTR U8 g_aDecStsOfTime[LDPC_SOFT_MAX_RDN_CNT];
GLOBAL MCU12_VAR_ATTR SOFT_DEC_STATUS_REPORT g_tSoftDecStsReport;
extern GLOBAL volatile BOOL g_bSinglePln;

#ifdef RANDOM_DATA_PATTERN

#endif

/*==============================================================================
Func Name  : TEST_LdpcSRdBitMap
Input      : NONE
Output     : ulCwBitMap
Return Val : LOCAL
Discription: Read OTF/Hard+ failed CWs' BitMap
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
    2. 2016.01.27 sync to abby's design in HAL_FlashChipFeature.c 
    3. 2016.02.16 Maple change logicPU to PU
==============================================================================*/
U32 MCU2_DRAM_TEXT TEST_LdpcSRdBitMap(FLASH_ADDR * pFlashAddr)
{
    U32 ulCwBitMap;
     
#ifdef SOFT_DRIVER_TEST
    U8 ucPln, ucLun, ucWp, ucPU;

    ucPln =  pFlashAddr->bsPln;
    ucLun = pFlashAddr->ucLun;
    ucWp = HAL_NfcGetWP(pFlashAddr->ucPU, ucLun);
    ucPU = pFlashAddr->ucPU;

    pNfcDecSts  = (DEC_SRAM_STATUS_ENTRY*)DEC_STATUS_BASE;
    ulCwBitMap = pNfcDecSts->aLdpcSts[ucPU][ucLun][ucWp][ucPln].ulHdrOrOtfFailBmp;
#else
    ulCwBitMap = FAKE_CW_BITMAP;
#endif

    return ulCwBitMap;
}

/*==============================================================================
Func Name  : TEST_LdpcSFindSeq1
Input      : NONE
Output     : ulCwBitMap
Return Val : LOCAL
Discription: Find sequent bit 1 in the bitmap 
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
    2. 2016.01.27 sync to abby's design in HAL_FlashChipFeature.c 
==============================================================================*/
BOOL MCU2_DRAM_TEXT TEST_LdpcSFindSeq1(U32 ulBitMap, U32 *ptStartBit, U8 *ptBit1Num, U8 *ptBitPos)
{
    U32 ucCurBitPos = *ptBitPos;
    U32 ucBit1Num;

    while (((ulBitMap & (1<<ucCurBitPos))==0) && (CW_NUM_MAX_PER_DS_ENTRY >ucCurBitPos))
    {
        ucCurBitPos++;
    }

    if(CW_NUM_MAX_PER_DS_ENTRY <= ucCurBitPos)
    {
        ucCurBitPos = 0;
        *ptBitPos = ucCurBitPos;
        return FALSE;   
    }

    *ptStartBit = ucCurBitPos++;
    ucBit1Num = 1;
    
    while ((ulBitMap&(1<<ucCurBitPos))&&(CW_NUM_MAX_PER_DS_ENTRY>ucCurBitPos)&&(DESC_SHARED_CW_NUM>ucBit1Num)&&((ucCurBitPos%CW_PER_LBA)!=0))
    {
        ucBit1Num++;
        ucCurBitPos++;
    }
    *ptBit1Num = ucBit1Num;
    *ptBitPos = ucCurBitPos;
    
    return TRUE;
}

/*==============================================================================
Func Name  : TEST_LdpcSCwAssign
Input      : NONE
Output     : CwBitMap
Return Val : status
Discription: 
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
    2. 2016.02.01 Take Jason's advice and modify BitMap reading function
==============================================================================*/
BOOL MCU2_DRAM_TEXT TEST_LdpcSCwAssign(FLASH_ADDR * pFlashAddr)
{
    U32 ulDecSramBitMap;
    U32 ulStartBit=0;
    U8 ucCwNum=0;
    U8 ucBitPos=0;
    U8 ucDescCnt;
    BOOL bFind;

    U8 ucDescStartId = HAL_LdpcSGetDescId();
    
    /*Clear aCwNumPerDesc array*/
    for (ucDescCnt = 0; ucDescCnt < DESCRIPTOR_MAX_NUM; ucDescCnt++)
    {
        g_aCodeWordSegment[(ucDescCnt + ucDescStartId) % DESCRIPTOR_MAX_NUM].bsCodeWordCount = 0;
        g_aCodeWordSegment[(ucDescCnt + ucDescStartId) % DESCRIPTOR_MAX_NUM].bsStartCodeWord = 0;
    }
    
    /*Read BitMap from DEC SRAM*/
    ulDecSramBitMap = TEST_LdpcSRdBitMap(pFlashAddr);
    DBG_Printf("LDPC Soft DEC read bitmap:0x%x\n",ulDecSramBitMap);

    /*assign Failed CWs to each descriptor*/
    ucDescCnt = ucDescStartId;
    do
    {
        bFind = TEST_LdpcSFindSeq1(ulDecSramBitMap, &ulStartBit, &ucCwNum, &ucBitPos);

        if(bFind == SUCCESS)
        {       
            g_aCodeWordSegment[ucDescCnt].bsCodeWordCount = ucCwNum;
            g_aCodeWordSegment[ucDescCnt].bsStartCodeWord = ulStartBit;
            ucDescCnt = (ucDescCnt + 1) % DESCRIPTOR_MAX_NUM;
        }
    }while(bFind == SUCCESS);
    
    return SUCCESS;
}

/*==============================================================================
Func Name  : TEST_LdpcSTotalDescCnt
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Calculate the descriptors number
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U8 MCU2_DRAM_TEXT TEST_LdpcSTotalDescCnt(void)
{
    U8 ucDescId, ucDescCnt;
    ucDescCnt = 0;
    
    for (ucDescId = 0; ucDescId < DESCRIPTOR_MAX_NUM; ucDescId++)
    {
        if(g_aCodeWordSegment[ucDescId].bsCodeWordCount != 0)
        {
            ucDescCnt++;
        }
    }

    return ucDescCnt;
}

/*==============================================================================
Func Name  : TEST_LdpcSGetDmaAddr
Input      : NONE
Output     : (ulBaseAddr + ulOffSet)
Return Val : LOCAL
Discription: Get DRAM address for the descriptor
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U32 MCU2_DRAM_TEXT TEST_LdpcSGetDmaAddr(U32 BuffID, U32 ulSecInBuf)
{
    return HAL_NfcGetDmaAddr(BuffID, ulSecInBuf, LOGIC_PG_SZ_BITS + 1);
}

/*==============================================================================
Func Name  : TEST_LdpcSGetDescRWAddrs
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Get five RADDRs & one WADDR for the descriptor
Usage      :
History    :
1. 2016.02.25 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT TEST_LdpcSGetDescRWAddrs(U32 DescID, U32 BuffID, U8 ucSecStart)
{
    volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc;

    pSoftDecDesc = HAL_LdpcSGetDescAddr(DescID);

    pSoftDecDesc->aReadAddr[0] = (TEST_LdpcSGetDmaAddr(BuffID,     ucSecStart)<<1);//
    pSoftDecDesc->aReadAddr[1] = (TEST_LdpcSGetDmaAddr(BuffID + 2, ucSecStart)<<1);
    pSoftDecDesc->aReadAddr[2] = (TEST_LdpcSGetDmaAddr(BuffID + 4, ucSecStart)<<1);
    pSoftDecDesc->aReadAddr[3] = (TEST_LdpcSGetDmaAddr(BuffID + 6, ucSecStart)<<1);
    pSoftDecDesc->aReadAddr[4] = (TEST_LdpcSGetDmaAddr(BuffID + 8, ucSecStart)<<1);
    if(FALSE == pSoftDecDesc->bsDramOtfbSel)
    {
        pSoftDecDesc->ulWADDR  = (TEST_LdpcSGetDmaAddr(BuffID + 10, ucSecStart)<<1);
        COM_MemZero((U32*)((pSoftDecDesc->ulWADDR) + DRAM_START_ADDRESS), (LOGIC_PG_SZ << 1) / sizeof(U32));
    }
    else
    {
        pSoftDecDesc->ulWADDR  = 0x90000;
    }
    //DBG_Printf("LDPC Soft DEC RWADDR configure done, BuffId=%d, RAddr0=0x%x, WAddr=0x%x.\n",
    //    BuffID,pSoftDecDesc->aReadAddr[0],pSoftDecDesc->ulWADDR);

    return;
}


/*==============================================================================
Func Name  : TEST_LdpcSGetDecSts
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Read LDPC soft DEC status until it is done
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
BOOL MCU2_DRAM_TEXT TEST_LdpcSGetDecSts(volatile U8 ucCurDescId)
{
    U8 ucCwNum;
    U8 ucIdleBitMap;
    volatile SOFT_DEC_DESCRIPTOR * pSoftDecDesc;

    while((pSoftMcu1CfgReg->bsRptr1 != pSoftMcu1CfgReg->bsWptr1)||
          (pSoftMcu2CfgReg->bsRptr2 != pSoftMcu2CfgReg->bsWptr2))
    {
        ;
    }

    pSoftDecDesc = HAL_LdpcSGetDescAddr(ucCurDescId);
    ucCwNum = pSoftDecDesc->bsCwNum;
    ucIdleBitMap = (0xF >> (4 - ucCwNum));

    if(ucIdleBitMap != (pSoftDecDesc->bsBitMap))
    {
        //DBG_Printf("LDPC Soft DEC done, Bitmap != IdleBitmap!\n");
        DBG_Getch();
    }
    
    return TRUE;
}

/*==============================================================================
Func Name  : TEST_LdpcSChkAllSts
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Check all the decsriptors' status if it is decoded done, then remove succeed ones
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U8 MCU2_DRAM_TEXT TEST_LdpcSChkAllSts(U8 DescNum, U8 DecTime)
{
    U8 ucDescId;
    U32 ucCurWptr;
    U32 ucPreDescId;
    U8 ucResults;

    volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc ;
    
    if((pSoftMcu1CfgReg->bsWptr1 != pSoftMcu1CfgReg->bsRptr1)||(pSoftMcu2CfgReg->bsWptr2 != pSoftMcu2CfgReg->bsRptr2))
    {
        DBG_Printf("LDPC SoftDEC Check status fail, WPTR!=RPTR.\n");
        DBG_Printf("WPTR1=%d, RPTR1=%d, WPTR2=%d, RPTR2= %d.\n",pSoftMcu1CfgReg->bsWptr1,pSoftMcu1CfgReg->bsRptr1,pSoftMcu2CfgReg->bsWptr2,pSoftMcu2CfgReg->bsRptr2);
        DBG_Getch();
    }
    
    ucCurWptr = HAL_LdpcSGetDescId();//(pSoftMcu2CfgReg->bsWptr2);
    
    for (ucDescId = 0; ucDescId < DescNum; ucDescId++)
    {
        //if(SUCCESS == g_aSuccDescId[ucDescId]) continue;
        
        ucPreDescId = (ucCurWptr + DESCRIPTOR_MAX_NUM - DescNum + ucDescId) % DESCRIPTOR_MAX_NUM;
        pSoftDecDesc = HAL_LdpcSGetDescAddr(ucPreDescId);

        /*Check fail sts after tasks finished*/
        if (TRUE == pSoftDecDesc->bsFailed) // 1 indicates DEC fail
        {
            //HAL_LdpcSoftDecCombinedChk(ucPreDescId, DecTime, pSoftDecDesc->bsCwNum);
            //ucResults = FAIL;
            return FAIL;//This Page dec fail
            //g_aSuccDescId[ucDescId] = FAIL;
        }
        else
        {
            ucResults = SUCCESS;
            //g_aSuccDescId[ucDescId] = SUCCESS;
            g_SuccDescNum++;
        }
    }
    
    return ucResults;
}


/*==============================================================================
Func Name  : TEST_LdpcSReportSts
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Report successful status
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT TEST_LdpcSReportSts(U8 ucChkSts, U8 DescId, U8 DecTime)
{
    U8 ucRdCnt;
    volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc;

    pSoftDecDesc = HAL_LdpcSGetDescAddr(DescId);
        
    g_tSoftDecStsReport.bsCwNum = pSoftDecDesc->bsCwNum;
    g_tSoftDecStsReport.bsRdNum = pSoftDecDesc->bsRdNum;
    
    if (SUCCESS == ucChkSts)
    {
        g_tSoftDecStsReport.bsErrCnt1 = pSoftDecDesc->bsErrCnt1;
        g_tSoftDecStsReport.bsErrCnt2 = pSoftDecDesc->bsErrCnt2;
        g_tSoftDecStsReport.bsErrCnt3 = pSoftDecDesc->bsErrCnt3;
        g_tSoftDecStsReport.bsErrCnt4 = pSoftDecDesc->bsErrCnt4;

        g_tSoftDecStsReport.bsIterNum1 = pSoftDecDesc->bsIterNum1;
        g_tSoftDecStsReport.bsIterNum2 = pSoftDecDesc->bsIterNum2;
        g_tSoftDecStsReport.bsIterNum3 = pSoftDecDesc->bsIterNum3;
        g_tSoftDecStsReport.bsIterNum4 = pSoftDecDesc->bsIterNum4;

        g_tSoftDecStsReport.ulCwWADDR = pSoftDecDesc->ulWADDR;

        return;
    }
    
    if (COMB_SUCCESS == ucChkSts)
    {
        for (ucRdCnt = 0; ucRdCnt <= DecTime; ucRdCnt++)
        {
            pSoftDecDesc = HAL_LdpcSGetDescAddr(DescId - ucRdCnt);
            if (0 != ((1 << 0) & pSoftDecDesc->bsDecSts))
            {
                g_tSoftDecStsReport.bsErrCnt1 = pSoftDecDesc->bsErrCnt1;
                g_tSoftDecStsReport.bsIterNum1 = pSoftDecDesc->bsIterNum1;
            }
            if (0 != ((1 << 1) & pSoftDecDesc->bsDecSts))
            {
                g_tSoftDecStsReport.bsErrCnt2 = pSoftDecDesc->bsErrCnt2;
                g_tSoftDecStsReport.bsIterNum2 = pSoftDecDesc->bsIterNum2;
            }
            if (0 != ((1 << 2) & pSoftDecDesc->bsDecSts))
            {
                g_tSoftDecStsReport.bsErrCnt3 = pSoftDecDesc->bsErrCnt3;
                g_tSoftDecStsReport.bsIterNum3 = pSoftDecDesc->bsIterNum3;
            }
            if (0 != ((1 << 3) & pSoftDecDesc->bsDecSts))
            {
                g_tSoftDecStsReport.bsErrCnt4 = pSoftDecDesc->bsErrCnt4;
                g_tSoftDecStsReport.bsIterNum4 = pSoftDecDesc->bsIterNum4;
            }
        }
        g_tSoftDecStsReport.ulCwWADDR = INVALID_8F; //ulCwWADDR pending !!

        return;
    }
}


/*==============================================================================
Func Name  : TEST_LdpcSCwRawRd
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.02.25 MapleXu copy from HAL_CfcPageRead(), and disable ECC.
    2. 2016.03.02 Maple change DSG's DRAM to 2KB aligned.
==============================================================================*/
U32 MCU2_DRAM_TEXT TEST_LdpcSCwRawRd(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, BOOL bOTFBBuff)
{
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucPageType = 0;
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY *pDSG;
    NFC_SCR_CONF eScrType = NORMAL_SCR;
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    U8  ucDsIndex = pRdReq->bsDsIndex;
    U8  ucBufUnit = ((pDsReg->atDSEntry[ucDsIndex].bsCwNum % 16) ? CW_INFO_SZ_BITS : LOGIC_PG_SZ_BITS);

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }
    
    if (FALSE != pRdReq->bsTlcMode)//TLC
    {
        ucPageType = pRdReq->bsTlcPgType;
        ucReqType = NF_PRCQ_TLC_READ_LP + ucPageType;
        ucPln      = pFlashAddr->bsPln + ucPageType;
        eScrType   = TLC_RD_ONE_PG;
    }
    else
    {
        ucPageType = 0;
        ucReqType = NF_PRCQ_READ;
        ucPln = pFlashAddr->bsPln;
        eScrType = NORMAL_SCR;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry,sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = ucDsIndex;  //select flash page layout by DS entry in IRS

    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;
    
    /*  DSG configure   */
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsDsgEn = TRUE;
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;
    
    /*  OTFB configure  */
    if(FALSE != bOTFBBuff)
    {
        pNFCQEntry->bsOntfEn   = TRUE;
        pNFCQEntry->bsTrigOmEn = TRUE;
    }
    
    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;
    
    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);
    
    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed)
    {
        pNFCQEntry->bsRedOntf = pRdReq->bsRedOntf;
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,pRdReq->bsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, pRdReq->bsRedOntf);
    }
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_SINGLE_PLN, ucPageType, eScrType);
    
    /* Raw data read = true, each CW 2K align */
    pNFCQEntry->bsRawReadEn = pRdReq->bsRawData;

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }
    
    /*  NFCQ DW8-15: flash address  */ 
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);

    /*  DSG DW0: transfer length and chain config  */ 
    pDSG->bsXferByteLen = (pRdReq->bsSecLen) << SEC_SZ_BITS;
    pDSG->bsLast = TRUE;
    /*  DSG DW1: dram address  */ 
    /******************************* IMPORTANT for SOFT DEC *********************************/
    //pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, ((pRdReq->bsSecStart) << 1), (LOGIC_PG_SZ_BITS + 1)); // Need 2KB to store one CW
    pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, 0, (LOGIC_PG_SZ_BITS + 1)); // Need 2KB to store one CW
    COM_MemZero((U32*)((pDSG->bsDramAddr << 1) + DRAM_START_ADDRESS), (LOGIC_PG_SZ << 1) / sizeof(U32));
    //DBG_Printf("LDPC Soft DEC Pu#%d Lun#%d Pln#%d Page#%d raw data Write Addr = 0x%x, BuffId=%d.\n",
    //    pFlashAddr->ucPU,pFlashAddr->ucLun,ucPln,pFlashAddr->usPage,((pDSG->bsDramAddr)<<1)+DRAM_START_ADDRESS,pRdReq->bsRdBuffId);
    /******************************* IMPORTANT for SOFT DEC *********************************/
    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr,  ucReqType, FALSE, INVALID_2F);
    
    return NFC_STATUS_SUCCESS;
}

void TEST_LdpcSShiftReadAll(FLASH_ADDR * pFlashAddr, U32 BuffID, U8 ucRetryTime, U8 ucPageType, BOOL bTlcMode)
{
    U8 ucRetryCnt;
    BOOL ucRawReadSts;
    NFC_READ_REQ_DES tRdReq = {0};
    RETRY_TABLE tRetryPara;
    U8 ucParaNum, ucWLType, ucReadOffsetTime;

    tRdReq.bsTlcMode = bTlcMode;
    tRdReq.bsTlcPgType = ucPageType;
    tRdReq.bsRawData = TRUE;
    ucWLType = HAL_GetFlashWlType(pFlashAddr->usPage, bTlcMode);
    ucReadOffsetTime = HAL_GetVthAdjustTime(ucPageType, bTlcMode);
#ifdef NFC_ERR_INJ_EN
    NFC_ERR_INJ tNfcErrInj = { 0 };
    tRdReq.bsInjErrEn = TRUE;
    tRdReq.pErrInj = &tNfcErrInj;
    tRdReq.pErrInj->bsInjErrBitPerCw = 40;
    tRdReq.pErrInj->bsInjCwStart = 0;
    tRdReq.pErrInj->bsInjCwLen = 15;
    tRdReq.pErrInj->bsInjErrType = 6;
    
    //DBG_Printf("LDPC SoftDEC ErrorInj ErrBitCnt = %d, CwStart = %d, CwLen = %d.\n",tRdReq.pErrInj->bsInjErrBitPerCw,
    //    tRdReq.pErrInj->bsInjCwStart,tRdReq.pErrInj->bsInjCwLen);
#endif

    /* RetryStep1~4*/
    for (ucRetryCnt = 0;ucRetryCnt < ucRetryTime; ucRetryCnt++)
    {
        tRdReq.bsRdBuffId = ((U16)(BuffID + ucRetryCnt*2)); //need 2-PG DRAM space to store one page raw data
        tRdReq.bsSecStart = 0;
        tRdReq.bsSecLen = 16 << SEC_PER_CW_BITS;
        
        /*RetryStep1. Retry PreConditon*/
#ifndef LDPC_SOFT_NORMAL
        HAL_FlashRetryPreConditon(pFlashAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
        {
            DBG_Printf("ReadRetry-PreConditon PU#%d Fail! \n", pFlashAddr->ucPU);
            DBG_Getch();
        }
     
        /*RetryStep2. Soft DEC ShiftRead Table Parameter Setting*/
        tRetryPara = HAL_LdpcSoftDecGetShiftRdParaTab(ucRetryCnt,ucRetryTime,bTlcMode,ucWLType,ucPageType, 0);//pending
        
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
#endif           
        /*RetryStep4. redo read:*/
        ucRawReadSts = TEST_LdpcSCwRawRd(pFlashAddr, &tRdReq, FALSE);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
        {
            DBG_Printf("ReadRetry-Partial read PU#%d Fail! \n", pFlashAddr->ucPU);
            DBG_Getch();
        }
    }

#ifndef LDPC_SOFT_NORMAL
    // RetryStep5. terminate retry: enter to normal mode
    HAL_FlashRetryTerminate(pFlashAddr->ucPU, pFlashAddr->ucLun,bTlcMode);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("ReadRetry-Partial read PU#%d Fail! \n", pFlashAddr->ucPU);
        DBG_Getch();
    }
#endif

    return;
}
/*==============================================================================
Func Name  : TEST_LdpcSCwErrInj
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT TEST_LdpcSCwErrInj(U32 BuffID, U8 ucSecStart, U8 ucRetryTime, U8 ucFlipDwCnt)
{
    U32 ulRAddrCnt;
    U32 ulDramOffAddr, ulDramAbsAddr;
    U8 ucDwCnt;
    volatile U32 ulRData, ulWData;

    for(ulRAddrCnt=0; ulRAddrCnt < ucRetryTime;ulRAddrCnt++)
    {
        ulDramOffAddr = TEST_LdpcSGetDmaAddr(BuffID + ulRAddrCnt*2, ucSecStart)<<1;

#ifdef DRAMLESS_MODULE_TEST
        ulDramAbsAddr = ulDramOffAddr + OTFB_START_ADDRESS;
#else
        ulDramAbsAddr = ulDramOffAddr + DRAM_START_ADDRESS;
#endif
        for (ucDwCnt=0; ucDwCnt < ucFlipDwCnt; ucDwCnt++)
        {
            ulDramAbsAddr += (ucDwCnt*4);
            ulRData = *(volatile U32*)(ulDramAbsAddr);
            ulWData = ulRData ^ INVALID_8F; //flip all bits in this DW

            *(volatile U32*)(ulDramAbsAddr) = ulWData;
            //DBG_Printf("LDPC Soft DEC DRAM Data Flip, BuffId =%d DRAM Addr 0x%x data:0x%x fliped to 0x%x.\n",(BuffID + ulRAddrCnt*2),ulDramAbsAddr,ulRData,ulWData);
        }
    }

    return;
}

/*==============================================================================
Func Name  : TEST_LdpcSGetLba
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U32 MCU2_DRAM_TEXT TEST_LdpcSGetLba(FLASH_ADDR *pFlashAddr, U8 ucSecStart)
{
    U32 ulLba = 0;
    
    if (TRUE != g_bEmEnable)
    {
        ulLba = (ucSecStart << 8);
    }
    else
    {
        ulLba = ucSecStart;
    }
    
    return (ulLba << 3);
}


BOOL MCU2_DRAM_TEXT TEST_LdpcSCore(FLASH_ADDR * pFlashAddr, U32 BuffID, U8 ucPageType, BOOL bTlcMode)
{
    U8 ucDecTime = 0;
    U32 ulCurBuffID;
    U8 ucDescStartId, ucCurDescId, ucDescNum, ucDescCnt;
    U8 ucChkSts;
    U8 ucSecStart;
    //U8 ucDwErrInj;
    U8 ucLpnIndex, ucSecInLpn;
    DESC_EN_CTL tDescEnCtl = {0};

    /*Read CW BitMap, and get the new descriptors' number*/
    TEST_LdpcSCwAssign(pFlashAddr);
    ucDescNum = TEST_LdpcSTotalDescCnt();
    //DBG_Printf("LDPC Soft Dec descriptor cnt = %d.\n", ucDescNum);

    /* Read raw data and push descriptors for each decoding cycle */
    for (ucDecTime = 0; ucDecTime < LDPC_SOFT_MAX_DEC_CNT; ucDecTime++) // 1/3/5
    {
        /* Clear SuccDescNum */
        g_SuccDescNum = 0;

        /* Update FW RPTR */
        HAL_LdpcSFifoPop();//RPTR
        
        if(TRUE == HAL_LdpcSFifoIsEmpty())
        {
            ;
        }
        
        /* Use different BUF sets for the each decode loop */
        ulCurBuffID = BuffID + (ucDecTime * LDPC_SOFT_DEC_BUF_NUM);

        /* Get valid Desc ID from FW WPTR */
        ucDescStartId = HAL_LdpcSGetDescId();

        /* Shift Read full page raw data */
        TEST_LdpcSShiftReadAll(pFlashAddr, ulCurBuffID, g_aShiftRdNTable[ucDecTime],ucPageType,bTlcMode);
        
        tDescEnCtl.bsIntStsEn       = FALSE;
        tDescEnCtl.bsDecTime        = ucDecTime;
        tDescEnCtl.bsDramCrcEn      = TRUE;
        tDescEnCtl.bsScrEn          = (SCRAMBLE_MSK_EN ? FALSE:TRUE);
        tDescEnCtl.bsRedunEn        = TRUE;
        tDescEnCtl.bsCrcEn          = TRUE;
        tDescEnCtl.bsLbaEn          = TRUE;
        tDescEnCtl.bsLbaChkEn       = TRUE;

        tDescEnCtl.bsShiftReadTimes = g_aShiftRdNTable[ucDecTime];
        
        /* Cfg all the Fw descriptors */
        for (ucDescCnt = 0; ucDescCnt < ucDescNum; ucDescCnt++)
        {
            ucSecStart           = (g_aCodeWordSegment[ucDescStartId + ucDescCnt].bsStartCodeWord << SEC_PER_CW_BITS);
            ucLpnIndex           = ucSecStart / SEC_PER_LPN + (pFlashAddr->bsPln << LPN_PER_PLN);
            ucSecInLpn           = ucSecStart % SEC_PER_LPN;
            tDescEnCtl.bsDescCnt = ucDescCnt;
            #ifdef NFC_EM_EN
            tDescEnCtl.bsEmEn    = TRUE;
            #endif
            
            if (0 == ucPageType)
                tDescEnCtl.ulLba = TEST_LdpcSGetLba(pFlashAddr, ucLpnIndex) + ucSecInLpn;
            else
                tDescEnCtl.ulLba = TEST_RED_DATA + ucSecInLpn;
        
            ucCurDescId = (ucDescStartId + ucDescCnt) % DESCRIPTOR_MAX_NUM;

            // Cfg scramble seed for Soft DEC
            if (TRUE == tDescEnCtl.bsScrEn)
            {   
                #if (defined TLC_MODE_TEST) && (defined SOFT_TLC_INTERNAL)
                tDescEnCtl.bsScrShfNum = (U8)(pFlashAddr->usPage);
                #elif (defined TLC_MODE_TEST)
                tDescEnCtl.bsScrShfNum = (U8)(pFlashAddr->usPage + ucPageType);
                #else
                tDescEnCtl.bsScrShfNum = (U8)(pFlashAddr->usPage);
                #endif
            }
            
            /*configuration of this FW descriptor*/
            HAL_LdpcSDescCfg(pFlashAddr->ucPU, ucCurDescId, &tDescEnCtl);
            HAL_LdpcSDescCfgLlr(ucCurDescId,ucDecTime + 1);
            TEST_LdpcSGetDescRWAddrs(ucCurDescId, ulCurBuffID, ucSecStart<<1);
            
#ifdef LDPC_SOFT_ERRINJ
            if (ucDescNum > 3)
            {
                ucDwErrInj = 0;
            }
            else               
            {
                ucDwErrInj = ucDescNum;
            }
            //ucDwErrInj = 3;
            if(ucDecTime == (LDPC_SOFT_MAX_DEC_CNT-1))
            {
                ucDwErrInj = 0;//make sure the last decoding success
            }
            
            TEST_LdpcSCwErrInj(ulCurBuffID, ucSecStart<<1, g_aShiftRdNTable[ucDecTime], ucDwErrInj);
#endif
        }
        
        /* Push all the descriptors and Trigger LDPC wrapper*/
        HAL_LdpcSFifoPush(ucDescNum);// Update FW WPTR
        
        /*Wait and check if Dec is done*/
        for (ucDescCnt = 0; ucDescCnt < ucDescNum; ucDescCnt++)
        {
            ucCurDescId = (ucDescStartId + ucDescCnt) % (DESCRIPTOR_MAX_NUM);
            TEST_LdpcSGetDecSts(ucCurDescId);
            //DBG_Printf("LDPC Soft Dec descriptor#%d decode done.\n",ucCurDescId);
        }

        /*Check status, only all 16-CW of this page succeed will SUCCESS be returned*/
        ucChkSts = TEST_LdpcSChkAllSts(ucDescNum, ucDecTime);
        
#ifdef LDPC_SOFT_CWCHK
        for(ucDescCnt = 0; ucDescCnt < ucDescNum; ucDescCnt++)
        {
            U32 ulCurWptr, ulPreDescId;
            ulCurWptr = HAL_LdpcSGetDescId(HAL_GetMcuId());
            ulPreDescId = (ulCurWptr + DESCRIPTOR_MAX_NUM - ucDescNum + ucDescCnt) % DESCRIPTOR_MAX_NUM;
            #ifdef SOFT_TLC_INTERNAL
            ucPageType = 0;
            #endif
            if (0 == ucPageType)
            {
                TEST_LdpcSDataChk(pFlashAddr, ulPreDescId, ucPageType);
            }
        }
#endif

        if (SUCCESS == ucChkSts)
        {
            if (ucDescNum == g_SuccDescNum)
            {
                return SUCCESS;
            }
            else
            {
                DBG_Printf("LDPC Soft Dec error SUCCESS status!!!\n",ucDecTime);
                DBG_Getch();
            }
        }
        else
        {
            DBG_Printf("LDPC Soft Dec DecTime_%d FAIL!.\n",ucDecTime);
        }
        
        if (g_aShiftRdNTable[ucDecTime] == 5)
        {
            return FAIL;
        }
    }

    return SUCCESS;
}

/*==============================================================================
Func Name  : TEST_LdpcSDataChk
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.01.21 MapleXu create function
==============================================================================*/
BOOL MCU2_DRAM_TEXT TEST_LdpcSDataChk(FLASH_ADDR *pFlashAddr, U32 ulCurDescId, U8 ucPageType)
{
    U8  ucSec;
    U32 ulCwNum, ulCwCnt;
    U8 ucSecStart, ucSecLen, ucChkSec;
    U32 ulDecodedAddr, ulRawReadAddr;
    U32 ulPrepareData, ulDecodedData, ulRawReadData;
    U32 ulInfoRBerCnt, ulCwDecBerCnt;

    U32 ulDataBuffAddr;
    BOOL bCrcStatus, bRCrcStatus;

    ulDataBuffAddr = 0x54a00000 + (NFCQ_DEPTH*(pFlashAddr->ucPU) + (pFlashAddr->usPage) % NFCQ_DEPTH)*LOGIC_PIPE_PG_SZ;
    #ifdef FLASH_TLC
    ulDataBuffAddr += (ucPageType * LOGIC_PG_SZ);
    #endif
    
    volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc;
    pSoftDecDesc = HAL_LdpcSGetDescAddr(ulCurDescId);

    ulCwNum = pSoftDecDesc->bsCwNum;
    ulDecodedAddr = (U32) pSoftDecDesc->ulWADDR + DRAM_START_ADDRESS;
    ulRawReadAddr = (U32) pSoftDecDesc->aReadAddr[(pSoftDecDesc->bsRdNum)-1] + DRAM_START_ADDRESS;//use the newest shift read data
    
    for (ulCwCnt=0; ulCwCnt < ulCwNum; ulCwCnt++)
    {
        /* Decoded BER of the succeed CWs */
        ulCwDecBerCnt = (ulCwCnt == 0) ? (pSoftDecDesc->bsErrCnt1)     :
                        (ulCwCnt == 1) ? (pSoftDecDesc->bsErrCnt2)     :
                        (ulCwCnt == 2) ? (pSoftDecDesc->bsErrCnt3)     : (pSoftDecDesc->bsErrCnt4);
                        
        bCrcStatus    = (ulCwCnt == 0) ? (pSoftDecDesc->bsCrcStatus1)  : 
                        (ulCwCnt == 1) ? (pSoftDecDesc->bsCrcStatus2)  :
                        (ulCwCnt == 2) ? (pSoftDecDesc->bsCrcStatus3)  : (pSoftDecDesc->bsCrcStatus4);
                        
        bRCrcStatus   = (ulCwCnt == 0) ? (pSoftDecDesc->bsRcrcStatus1) : 
                        (ulCwCnt == 1) ? (pSoftDecDesc->bsRcrcStatus2) :
                        (ulCwCnt == 2) ? (pSoftDecDesc->bsRcrcStatus3) : (pSoftDecDesc->bsRcrcStatus4);
        
        ulInfoRBerCnt = 0;
        
        /* check info part of all CWs one-by-one */
        U8 bStatus = (pSoftDecDesc->bsDecSts & (1<<ulCwCnt));

        ucSecStart = ulCwCnt << 2;
        ucChkSec = ((pSoftDecDesc->bsCwId) + ulCwCnt) << 1;
        ucSecLen = 2; 
        for (ucSec = ucSecStart; ucSec < (ucSecStart + ucSecLen); ucSec++)
        {
            U32 ulDwIndex, i;   
            for (ulDwIndex = 0; ulDwIndex < 128; ulDwIndex++)
            {    
                #ifdef LDPC_RANDOM_PATTERN
                    ulPrepareData = *(volatile U32*)(ulDataBuffAddr + (ucChkSec << SEC_SIZE_BITS) + ulDwIndex * 4); 
                #else
                    ulPrepareData = TEST_NfcCalcuData(((pFlashAddr->ucLun << 6) + pFlashAddr->ucPU), pFlashAddr->usPage, ucChkSec) + ulDwIndex;
                #endif                
                ulRawReadData = *(volatile U32*)(ulRawReadAddr + (ucSec << SEC_SZ_BITS) + ulDwIndex * 4);
                ulDecodedData = *(volatile U32*)(ulDecodedAddr + (ucSec << SEC_SZ_BITS) + ulDwIndex * 4);

                /* Count INFO_RBER if dec failed*/
                if(TRUE == pSoftDecDesc->bsEmEn || TRUE == pSoftDecDesc->bsScrEn)
                {
                    ulInfoRBerCnt = INVALID_8F;
                }
                else
                {
                    if(ulPrepareData != ulRawReadData)
                    {
                        for (i=0; i<32; i++)
                        {
                            if((ulPrepareData & (1<<i)) != (ulRawReadData & (1<<i)))
                                ulInfoRBerCnt++;
                        }
                    }
                }
                
                /* check decoding error */
                if((0 != bStatus) && (ulPrepareData != ulDecodedData))
                {
                    DBG_Printf("LDPC Soft DEC Desc#%d CW#%d Data Miss-compare!\n",ulCurDescId, ulCwCnt);
                    DBG_Printf("DecodedAddr:0x%x, Sec:%d, DW:%d, DecodedData:0x%x PrepareData:0x%x, Addr:0x%x.\n"
                                ,(ulDecodedAddr + (ucSec<<SEC_SZ_BITS) + ulDwIndex * 4),ucSec,ulDwIndex,ulDecodedData,
                                ulPrepareData,(ulDataBuffAddr + (ucChkSec << SEC_SZ_BITS) + ulDwIndex * 4));
                    DBG_Getch();
                }
            }
            ucChkSec++;
        }
        //DBG_Printf("LDPC Soft DEC Desc#%d CW#%d FBC:Prepare Addr=0x%x INFO_RBER=%d, DBER=%d, CrcSts = %d, bRCrcStatus = %d, DecSts=%d!\n",
        //    ulCurDescId, ulCwCnt,ulDataBuffAddr,ulInfoRBerCnt,ulCwDecBerCnt,bCrcStatus,bRCrcStatus,((bStatus==0)?0:1));
    }
    
    // Check LPN of Reduntant
    if (0 == ucPageType && 0 != (pSoftDecDesc->bsDecSts & (1<<(ulCwCnt-1))) && 0x10 == (pSoftDecDesc->bsCwId + ulCwCnt))
    {
        U32 ulWAddrRedAddr;
        U32 ulRdLpn, ulWrLpn;
        U8 ucLPNIndex;
        ulWAddrRedAddr = ulDecodedAddr + ((ulCwCnt*2-1) << (SEC_SZ_BITS + 1));
        for (ucLPNIndex = 0; ucLPNIndex < (LPN_PER_PLN * PGADDR_PER_PRG * INTRPG_PER_PGADDR); ucLPNIndex++)
        {
            if (TRUE == g_bEmEnable)
            {
                ulWrLpn = (ucLPNIndex << 3);
            }
            else
            {
                ulWrLpn = ((ucLPNIndex<<8) << 3);
            }
            ulRdLpn = ((SPARE_AREA*)ulWAddrRedAddr)->aCurrLPN[ucLPNIndex];

            if (ulRdLpn != ulWrLpn && ulRdLpn != TEST_RED_DATA)
            {
                DBG_Printf("LDPC SoftDEC Red check error, LPN[%d]= 0x%x Error! Should be 0x%x, RedAddr=0x%x.\n", 
                    ucLPNIndex, ulRdLpn, ulWrLpn,ulWAddrRedAddr);
                DBG_Getch();
            }
        }
    }

    // Check LBA Status
    #ifndef SOFT_TLC_INTERNAL
    if(TRUE != pSoftDecDesc->bsFailed && TRUE == pSoftDecDesc->bsLbaChkEn && TRUE == pSoftDecDesc->bsLbaErrStatus)
    {
        DBG_Printf("LDPC Soft DEC LBA Check Error!!!\n");
        DBG_Getch();
    }
    #endif
    
    return SUCCESS;
}


/*==============================================================================
Func Name  : TEST_LdpcSRead
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
void TEST_LdpcSRead(U8 ucSecStart, U16 usSecLen, READ_REQ_TYPE ReadType)
{
    U8 ucLdpcDecSts;
    U16 ucLdpcBuffID = START_RBUF_ID;

    FLASH_ADDR tFlashAddr = {0};
    NFC_READ_REQ_DES tRdReq = {0};
    U8  ucPageType;
        
    TEST_NfcRdCfgReqComm(&tRdReq, ucSecStart, usSecLen);

    tFlashAddr.usBlock = TEST_BLOCK_START;
    while (tFlashAddr.usBlock < (U16)TEST_BLOCK_END)
    {
        tFlashAddr.ucPU = TEST_PU_START;
        while (tFlashAddr.ucPU < TEST_PU_END)
        {
            tFlashAddr.ucLun = TEST_LUN_START;
            while (tFlashAddr.ucLun < TEST_LUN_END)
            {
                tFlashAddr.usPage = TEST_PAGE_START;
                while (tFlashAddr.usPage < (U16)TEST_PAGE_END)
                {                   
                    //DBG_Printf("\n****** LDPC SoftDEC Pu%d Lun%d Blk%d Page%d begin. ******\n",tFlashAddr.ucPU,tFlashAddr.ucLun,tFlashAddr.usBlock,tFlashAddr.usPage);
                    for(ucPageType = 0; ucPageType < TEST_PG_PER_WL; ucPageType++)
                    {
                        while (TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
                        {
                                ;
                        }
                                                
                        ucLdpcDecSts = TEST_LdpcSCore(&tFlashAddr, ucLdpcBuffID, ucPageType, g_bTlcMode);
                        
                        if (SUCCESS == ucLdpcDecSts)
                        {
                            DBG_Printf("LDPC SoftDEC Pu%d Lun%d Blk%d Page%d_%d SUCCESS\n",tFlashAddr.ucPU,tFlashAddr.ucLun,tFlashAddr.usBlock,tFlashAddr.usPage,ucPageType);
                        }
                        else
                        {
                            DBG_Printf("LDPC SoftDEC Pu%d Lun%d Blk%d Page%d_%d FAIL\n",tFlashAddr.ucPU,tFlashAddr.ucLun,tFlashAddr.usBlock,tFlashAddr.usPage,ucPageType);
                        }
                        //DBG_Getch();
                    }
                    tFlashAddr.usPage++;
                }
                tFlashAddr.ucLun++;
            }
            tFlashAddr.ucPU++;
        }
        tFlashAddr.usBlock++;
    }

    return;
}

void TEST_LdpcSMain (void)
{
    //Erase
    TEST_NfcEraseAll();
    DBG_Printf("NFC Erase All done.\n\n");
    
    //Program
    TEST_NfcWriteAll(SING_PLN_WRITE);
    DBG_Printf("NFC Write All done.\n\n");
  
    //Soft DEC with fake Bitmap
    TEST_LdpcSRead(0, SEC_PER_LOGIC_PG, SING_PLN_READ);

    return;
}
extern void TEST_NfcBasicPattRunInit(void);

BOOL UT_LdpcMain(void)
{
    TEST_NfcBasicPattRunInit();

    TEST_NfcPatternSel();
    
    g_bSinglePln   = TRUE;
    
    #ifdef NFC_EM_EN
    /* Please make sure 'HAL_EMInit()' has gone through */
    g_bEmEnable = TRUE;
    #endif

    #ifdef RANDOM_DATA_PATTERN
    COM_WriteDataBuffInit();
    DBG_Printf("Prepare random data done.\n\n");
    #endif

    TEST_LdpcSMain();
   
    return TRUE;
}


