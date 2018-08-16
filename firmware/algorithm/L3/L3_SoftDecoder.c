/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
* File Name    : L3_SoftDecoder.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "FW_BufAddr.h"
#include "HAL_Xtensa.h"
#include "HAL_Dmae.h"
#include "HAL_LdpcSoftDec.h"
#include "HAL_DecStsReport.h"
#include "L2_Interface.h"
#include "L3_ErrHBasic.h"
#include "L3_BufMgr.h"
#include "L3_SoftDecoder.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL MCU2_DRAM_TEXT SOFT_DECO_CW_SEGMENT g_aCwNumPerDesc[DESCRIPTOR_MAX_NUM];
extern GLOBAL MCU2_DRAM_TEXT SOFT_DECO_CW_SEGMENT g_aCodeWordSegment[DESCRIPTOR_MAX_NUM];
extern GLOBAL U16 L2_BbtGetPbnBindingTable(U8 ucTlun, U16 usPbn, U8 ucPlane);
extern void MCU2_DRAM_TEXT L3_ErrHPreCondition(U8 ucTLun);
extern void MCU2_DRAM_TEXT L3_ErrHSetParameter(U8 ucTLun, U8 ucVthShift, BOOL bSLCMode);
extern void MCU2_DRAM_TEXT L3_ErrHTerminate(U8 ucTLun, BOOL bTLC);
extern U8 MCU2_DRAM_TEXT L3_ErrHGetRetryVtIndex(U8 ucTLun, U8 ucIndex, BOOL bTlcMode, U16 usWLType, U8 ucPageType);
extern BOOL MCU12_DRAM_TEXT L3_ErrHIsHomemadeReadRetryEntry(U8 ucTLun, U8 ucPriorityIndex, BOOL bTlcMode, U16 usWLType, U8 ucPageType);

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*==============================================================================
Func Name  : L3_LdpcSRdBitMap
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
LOCAL U32 MCU2_DRAM_TEXT L3_LdpcSRdBitMap(U32 ulPu, U32 ulLun, U32 ulPlane, U8 ucNFCQPtr)
{
    U32 ulCwBitMap;
    volatile DEC_STATUS_SRAM *pNfcDecSts;

    pNfcDecSts = (DEC_STATUS_SRAM *) DEC_STATUS_BASE;
    ulCwBitMap = pNfcDecSts->aDecStsSram[ulPu][ulLun][ucNFCQPtr][ulPlane].ulDecFailBitMap;
    //DBG_Printf("LDPC soft dec Pu#%d Lun#%d Rp#%d Pln#%d, Read DEC SRAM BitMap: 0x%x.\n",ulPu,ulLun,ucNFCQPtr,ulPlane,ulCwBitMap);

    if((ulCwBitMap & 0xFFFF0000) != 0)
    {
        DBG_Printf("LDPC Soft DEC DEC_SRAM Bitmap Read Error, Addr:0x%x!\n", (U32)&(pNfcDecSts->aDecStsSram[ulPu][ulLun][ucNFCQPtr][ulPlane].ulDecFailBitMap));
        DBG_Getch();
    }

    return ulCwBitMap;
}

/*==============================================================================
Func Name  : L3_LdpcSFindSeq1
Input      : NONE
Output     : ulCwBitMap
Return Val : LOCAL
Discription: Find sequent bit 1 in the bitmap
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
    2. 2016.01.27 sync to abby's design in HAL_FlashChipFeature.c
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_LdpcSFindSeq1(U32 ulBitMap, U32 *ptStartBit, U8 *ptBit1Num, U8 *ptBitPos)
{
    U32 ucCurBitPos = *ptBitPos;
    U32 ucBit1Num;

    while (((ulBitMap & (1<<ucCurBitPos))==0) && (32 >ucCurBitPos))
    {
        ucCurBitPos++;
    }

    if(32 <= ucCurBitPos)
    {
        ucCurBitPos = 0;
        *ptBitPos = ucCurBitPos;
        return FALSE;
    }

    *ptStartBit = ucCurBitPos++;
    ucBit1Num = 1;
    while ((ulBitMap & (1<<ucCurBitPos)) && (32>ucCurBitPos) && (DESC_SHARED_CW_NUM > ucBit1Num) && ((ucCurBitPos % CW_PER_LBA) != 0))
    {
        ucBit1Num++;
        ucCurBitPos++;
    }
    *ptBit1Num = ucBit1Num;
    *ptBitPos = ucCurBitPos;

    return TRUE;
}


/*==============================================================================
Func Name  : L3_LdpcSDescCwAssign
Input      : NONE
Output     : CwBitMap
Return Val : status
Discription:
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
    2. 2016.02.01 Take Jason's advice and modify BitMap reading function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_LdpcSDescCwAssign(U32 ulDecSramBitMap, U32 ulStartDescriptorId)
{
    ASSERT(ulStartDescriptorId < DESCRIPTOR_MAX_NUM);

    U32 ulStartBit=0;
    U8 ucCwNum=0;
    U8 ucBitPos=0;
    BOOL bFind;
    U32 ulDescriptorId = ulStartDescriptorId;

    do
    {
        bFind = L3_LdpcSFindSeq1(ulDecSramBitMap, &ulStartBit, &ucCwNum, &ucBitPos);

        if(bFind == SUCCESS)
        {
            g_aCodeWordSegment[ulDescriptorId].bsStartCodeWord = ulStartBit;
            g_aCodeWordSegment[ulDescriptorId].bsCodeWordCount = ucCwNum;

            ulDescriptorId = (ulDescriptorId + 1) % DESCRIPTOR_MAX_NUM;
        }

    }while(bFind == SUCCESS);

    return SUCCESS;
}


/*==============================================================================
Func Name  : L3_LdpcSDescTotalCnt
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Calculate the descriptors number
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
LOCAL U8 MCU2_DRAM_TEXT L3_LdpcSDescTotalCnt(U32 ulDecSramBitMap, U8 ucPln)
{
    U32 ulStartBit = 0;
    U8 ucCwNum = 0;
    U8 ucBitPos = 0;
    U8 ucDescCnt;
    BOOL bFind;

    for (ucDescCnt = 0; ucDescCnt < DESCRIPTOR_MAX_NUM; ucDescCnt++)
    {
        g_aCwNumPerDesc[ucDescCnt].bsCodeWordCount= 0;
        g_aCwNumPerDesc[ucDescCnt].bsStartCodeWord= 0;
    }

    ucDescCnt = 0;
    do
    {
        bFind = L3_LdpcSFindSeq1(ulDecSramBitMap, &ulStartBit, &ucCwNum, &ucBitPos);

        if (bFind == SUCCESS)
        {
            g_aCwNumPerDesc[ucDescCnt].bsCodeWordCount= ucCwNum;
            g_aCwNumPerDesc[ucDescCnt].bsStartCodeWord= ulStartBit;
            ucDescCnt++;
        }
    } while (bFind == SUCCESS);

    return ucDescCnt;
}

/*==============================================================================
Func Name  : L3_LdpcSGetStartCw
Input      : U32 ulDescriptorId
Output     : bsStartCodeWord
Return Val : LOCAL
Discription: Get Start CodeWord
Usage      :
History    :
1. 2016.01.21 MapleXu create function
==============================================================================*/
LOCAL U32 MCU2_DRAM_TEXT L3_LdpcSGetStartCw(U32 ulDescriptorId)
{
    ASSERT(ulDescriptorId < DESCRIPTOR_MAX_NUM);

    return g_aCodeWordSegment[ulDescriptorId].bsStartCodeWord;
}

/*==============================================================================
Func Name  : L3_LdpcSSinglePlnRawRd
Input      : FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq
Output     : Status
Return Val : LOCAL
Discription: Read raw data by single plane
Usage      :
History    :
1. 2016.01.21 MapleXu create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_LdpcSSinglePlnRawRd(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucPageType = 0;
    U16 usCurDsgId;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY *pDSG;
    NFC_SCR_CONF eScrType = NORMAL_SCR;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPln = pFlashAddr->bsPln;
    pFlashAddr->bsSLCMode = (TRUE == pRdReq->bsTlcMode) ? FALSE : TRUE;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (pRdReq->bsTlcMode)
    {
        ucReqType = NF_PRCQ_TLC_READ;
        eScrType = NORMAL_SCR;
        ucPln = pFlashAddr->bsPln;
    }
    else
    {
        ucReqType = NF_PRCQ_READ;
        eScrType = NORMAL_SCR;
        ucPln = pFlashAddr->bsPln;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY) >> DWORD_SIZE_BITS);

    /*  NFCQ DW0: mode configure   */
    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;  //select flash page layout by DS entry in IRS
    /*  DSG configure   */
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsDsgEn = TRUE;
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed) // Read raw data cmd will read data with redundant, so don't set RedEn to true in NFCQ.
    {
        pNFCQEntry->bsRedOntf = pRdReq->bsRedOntf;
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, pRdReq->bsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, pRdReq->bsRedOntf);
    }

    /* Raw data read = true, each CW 2K align */
    pNFCQEntry->bsRawReadEn = pRdReq->bsRawData;

    /*  NFCQ DW8-15: flash address  */
    pNFCQEntry->atRowAddr[0].bsRowAddr = HAL_NfcGetFlashRowAddr(pFlashAddr);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY) >> DWORD_SIZE_BITS);

    /*  DSG DW0: transfer length and chain config  */
    pDSG->bsXferByteLen = (pRdReq->bsSecLen) << 9 ;
    pDSG->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    /******************************* IMPORTANT for SOFT DEC *********************************/
    pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, 0, BUF_SIZE_BITS); // Need 2KB to store one CW

    /******************************* IMPORTANT for SOFT DEC *********************************/
    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr,  ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*==============================================================================
Func Name  : L3_LdpcSShiftRead
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry,
             U32 ulCurrentTime,
             U32 ulTotalTimes
Output     : bFinishSoftDecodeShiftRead
Return Val : LOCAL
Discription: 1. Base on current time to get shift read paramter table.
             2. Send set feature to adjust Vth.
             3. Send read raw data cmd.
Usage      :
History    :
1. 2016.01.21 MapleXu create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_LdpcSShiftRead(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, U32 ulCurrentTime, U32 ulTotalTimes)
{
    ASSERT(ulTotalTimes <= LDPC_SOFT_MAX_DEC_CNT);
    ASSERT(ulCurrentTime < ulTotalTimes);

    BOOL bFinishSoftDecodeShiftRead = FALSE;
    NFC_READ_REQ_DES tRdReq = { 0 };
    FLASH_ADDR tFlashAddr = { 0 };
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    UECCH_SOFTDECO *ptLdpcSoftDecoCtrl = &(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tSoftDeco);
    U32 ulRawPageBufferId;
    BOOL bTlcMode = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;

    RETRY_TABLE tRetryParaSoftDec = { 0 };  // Get Soft Dec Vth Offset Table
    RETRY_TABLE tRetryPara = { 0 };         // Get Vth center by Retry Priority
    U8 ucPageType;
    U8 ucWLType;
    U8 i, ucReadOffsetTime;
    U8 ucTLun = ptReqEntry->bsTLun;
    U8 ucPu = L3_GET_PU(ucTLun);
    U8 ucLun = L3_GET_LUN_IN_PU(ucTLun);

    tFlashAddr.ucPU = ucPu;
    tFlashAddr.ucLun = ucLun;
    tFlashAddr.usPage = ptCtrlEntry->bsPhyPage;

    tFlashAddr.bsPln = ptLdpcSoftDecoCtrl->bsCurPlaneIndex;
    tFlashAddr.usBlock = L2_BbtGetPbnBindingTable(ucTLun, ptReqEntry->tFlashDesc.bsPhyBlk, tFlashAddr.bsPln);

    ulRawPageBufferId = L3_BufMgrGetBufIDByNode(ptLdpcSoftDecoCtrl->aRawPageReadBufferId[ulCurrentTime]);

    if (ulCurrentTime == 0)
    {
        // Enter into Shift-Read mode.
        L3_ErrHPreCondition(ucTLun);
    }

    // Set parameter
    ucPageType = HAL_GetFlashPairPageType(tFlashAddr.usPage);
    ucWLType = HAL_GetFlashWlType(tFlashAddr.usPage, bTlcMode);
    ucReadOffsetTime = HAL_GetVthAdjustTime(ucPageType, bTlcMode);
    #ifdef LDPC_CENTER_RECORD
    DBG_Printf("PU%d BLK%d PLN%d Page%d PageType%d bTlcMode%d WLType%d Start to do LDPC shift read: ShiftReadTimesIndex=%d ulTotalTimes=%d CurTime=%d\n",tFlashAddr.ucPU, tFlashAddr.usBlock, tFlashAddr.bsPln, tFlashAddr.usPage
    , ucPageType, bTlcMode, ucWLType, ptLdpcSoftDecoCtrl->bsShiftReadTimesIndex, ulTotalTimes, ulCurrentTime);
    #endif
    for (i = 0; i < ucReadOffsetTime; i++)
    {
        tRetryParaSoftDec = HAL_LdpcSoftDecGetShiftRdParaTab(ulCurrentTime, ulTotalTimes, bTlcMode, ucWLType, ucPageType, i);

#ifdef READ_RETRY_REFACTOR
        U8 ucPriorityIndex;
        U32 ulIndex = 0, ulEntryCnt = 0, j;

        do
        {
            if (TRUE == L3_ErrHIsHomemadeReadRetryEntry(ucTLun, ulEntryCnt, bTlcMode, ucWLType, ucPageType))
            {
                ucPriorityIndex = L3_ErrHGetRetryVtIndex(ucTLun, ulEntryCnt, bTlcMode, ucWLType, ucPageType);
                ulIndex++;
            }
            ulEntryCnt++;
        } while (ulIndex != ((ptLdpcSoftDecoCtrl->bsShiftReadTimesIndex / 2) + 1));

        tRetryPara = HAL_FlashGetNewRetryParaTab((ucPriorityIndex - RETRY_CNT), bTlcMode, ucWLType, ucPageType, i);
    #ifdef LDPC_CENTER_RECORD
        DBG_Printf("ReadOffsetIndex%d Get Retry Table PriIndex=%d ", i, (ucPriorityIndex - RETRY_CNT));
    #endif
        for (j = 0; j < HAL_TLC_FLASH_RETRY_PARA_MAX; j++)
        {
            tRetryParaSoftDec.aRetryPara[j].bsData = HAL_LdpcSoftDecVthCal(tRetryParaSoftDec.aRetryPara[j].bsData, tRetryPara.aRetryPara[j].bsData);
    #ifdef LDPC_CENTER_RECORD
            DBG_Printf("tRetryPara.aRetryPara[%d]:FA=0x%x PA=0x%x -> tRetryParaSoftDec.aRetryPara[%d].bsData=0x%x\n", j,tRetryPara.aRetryPara[j].bsAddr, tRetryPara.aRetryPara[j].bsData, j, tRetryParaSoftDec.aRetryPara[j].bsData);
    #endif
        }
#endif

        HAL_FlashRetrySendParam(&tFlashAddr, &tRetryParaSoftDec, bTlcMode, HAL_FLASH_RETRY_PARA_MAX);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPu, ucLun))
        {
            //DBG_Printf("MCU#%d PU#%d L3_ErrHSetParameter Fail.\n", HAL_GetMcuId(), ucPu);
            DBG_Getch();
        }
    }

    HAL_FlashRetryEn(&tFlashAddr, TRUE);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPu, ucLun))
    {
        //DBG_Printf("MCU#%d PU#%d L3_ErrHSetParameter RetryEn Fail.\n", HAL_GetMcuId(), ucPu);
        DBG_Getch();
    }

    // Raw Data Read
    tRdReq.bsRawData = TRUE;
    tRdReq.bsRdBuffId = ulRawPageBufferId;
    tRdReq.bsSecStart = 0;
    tRdReq.bsSecLen = SEC_PER_PHYPG;
    tRdReq.bsTlcMode = bTlcMode;
    tRdReq.bsTlcPgType = ucPageType;

    L3_LdpcSSinglePlnRawRd(&tFlashAddr, &tRdReq);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPu, ucLun))
    {
        //DBG_Printf("MCU#%d PU#%d Read raw data Fail.\n", HAL_GetMcuId(), ucPu);
        DBG_Getch();
    }

    // Terminate Shift-Read mode, return back to normal mode.
#ifdef READ_RETRY_REFACTOR
    HAL_FlashHomemadeVtTerminate(ucPu, ucLun, bTlcMode, ucWLType, ucPageType);
#else
    L3_ErrHTerminate(ucTLun, bTlcMode);
#endif

    if (ulCurrentTime == (ulTotalTimes -1))
    {
       bFinishSoftDecodeShiftRead = TRUE;
    }

    return bFinishSoftDecodeShiftRead;
}

/*==============================================================================
Func Name  : L3_LdpcSRdAllBitMap
Input      : UECCH_SOFTDECO *pLdpcSoftDecoCtrl, U32 ulPu, U32 ulLun, U8 ucNFCQPtr, BOOL bSinglePln
Output     : NONE
Return Val : NONE
Discription: Set CodeWord bitmap by DEC status.
Usage      :
History    :
1. 2016.01.21 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_LdpcSRdAllBitMap(UECCH_SOFTDECO *pLdpcSoftDecoCtrl, U32 ulPu, U32 ulLun, U8 ucNFCQPtr, BOOL bSinglePln)
{
    U8 ucPlnIdx;
    U8 ucCurrentPln;

    ucCurrentPln = pLdpcSoftDecoCtrl->bsCurPlaneIndex;

    if (TRUE == bSinglePln)
    {
        /* For single plane, bitmap is always in the lowest pln, need copy to CurPln */
        pLdpcSoftDecoCtrl->aDecSramBitMap[ucCurrentPln] = INVALID_4F;
    }
    else
    {
        for (ucPlnIdx = 0; ucPlnIdx < PLN_PER_LUN; ucPlnIdx++)
        {
            pLdpcSoftDecoCtrl->aDecSramBitMap[ucPlnIdx] = INVALID_4F; //L3_LdpcSRdBitMap(ulPu,ulLun,ucPlnIdx,ucNFCQPtr);
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_LdpcSCopyBackCw
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry,
             U32 ulDecoResultBufId,
             U32 ulDescId,
             U8 ucPlnIdx
Output     : NONE
Return Val : NONE
Discription: Copy data to destination address by code word.
Usage      :
History    :
1. 2016.01.21 MapleXu create function
==============================================================================*/
void L3_LdpcSCopyBackCw(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, U32 ulDecoResultBufId, U32 ulDescId, U8 ucPlnIdx)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U16 usSecIndex;
    U16 usCurSecInPage;
    S16 sDiffSecCnt;

    U16 usSrcCwStartInPln, usSrcCwLenInPln;
    U16 usSrcSecStartInPage, usSrcSecLenInPage;
    U32 ulSrcBaseAddr, ulSrcAddr;

    U16 usDstSecStartInPage, usDstSecLenInPage;
    U32 ulDstBaseAddr, ulDstAddr;
    FCMD_BUF_DESC atBuffEntry[DSG_BUFF_SIZE];

    // Src Data Address
    usSrcCwStartInPln = g_aCwNumPerDesc[ulDescId].bsStartCodeWord;
    usSrcCwLenInPln = g_aCwNumPerDesc[ulDescId].bsCodeWordCount;

    #ifdef ASIC
    ulSrcBaseAddr = COM_GetMemAddrByBufferID(ulDecoResultBufId, TRUE, BUF_SIZE_BITS) + usSrcCwStartInPln * (CW_INFO_SZ << 1);
    #else
    ulSrcBaseAddr = COM_GetMemAddrByBufferID(ulDecoResultBufId, TRUE, BUF_SIZE_BITS) + usSrcCwStartInPln * CW_INFO_SZ;
    #endif

    usSrcSecStartInPage = ucPlnIdx * SEC_PER_PHYPG + usSrcCwStartInPln * 2;
    usSrcSecLenInPage = usSrcCwLenInPln * 2;

    // Dst Data Address - Single/Multi Plane Local Read and Normal Read
    if ((FALSE == ptReqEntry->tFlashDesc.bsMergeRdEn)
     && (INVALID_4F == ptReqEntry->atBufDesc[1].bsBufID)
     && (FALSE == ptReqEntry->tFlashDesc.bsRdRedOnly))
    {
        #ifdef HOST_NVME
        if(TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
        {
            atBuffEntry[0].bsBufID = L3_BufMgrGetBufIDByNode(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.ptBufNodePtr);
        }
        else
        {
            atBuffEntry[0].bsBufID = ptReqEntry->atBufDesc[0].bsBufID;
        }
        #else
        atBuffEntry[0].bsBufID = ptReqEntry->atBufDesc[0].bsBufID;
        #endif
        atBuffEntry[0].bsSecStart = ptReqEntry->atBufDesc[0].bsSecStart;
        atBuffEntry[0].bsSecLen = ptReqEntry->atBufDesc[0].bsSecLen;

        // if single plane cmd type + table req => BBT operation, other is normal read.
        if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType)
        {
            if (TRUE == ptReqEntry->bsTableReq)
            {
                ulDstBaseAddr = COM_GetMemAddrByBufferID(atBuffEntry[0].bsBufID, TRUE, PHYPG_SZ_BITS) + atBuffEntry[0].bsSecStart * SEC_SIZE;
            }
            else
            {
                ulDstBaseAddr = COM_GetMemAddrByBufferID(atBuffEntry[0].bsBufID, TRUE, BUF_SIZE_BITS) + atBuffEntry[0].bsSecStart * SEC_SIZE;
            }
            usDstSecStartInPage = ptReqEntry->tFlashDesc.bsSecStart + ucPlnIdx * SEC_PER_PHYPG;
            usDstSecLenInPage = ptReqEntry->tFlashDesc.bsSecLen;
        }
        else
        {
            ulDstBaseAddr = COM_GetMemAddrByBufferID(atBuffEntry[0].bsBufID, TRUE, BUF_SIZE_BITS) + atBuffEntry[0].bsSecStart * SEC_SIZE;
            usDstSecStartInPage = ptReqEntry->tFlashDesc.bsSecStart;
            usDstSecLenInPage = ptReqEntry->tFlashDesc.bsSecLen;
        }

        for (usSecIndex = 0; usSecIndex < usSrcSecLenInPage; usSecIndex++)
        {
            usCurSecInPage = usSrcSecStartInPage + usSecIndex;
            sDiffSecCnt = usCurSecInPage - usDstSecStartInPage;

            // skip the head
            if (sDiffSecCnt < 0)
            {
                continue;
            }
            // break the tail
            if (sDiffSecCnt >= usDstSecLenInPage)
            {
                break;
            }

            #ifdef ASIC
            ulSrcAddr = ulSrcBaseAddr + (usSecIndex / 2) * (CW_INFO_SZ << 1) + (usSecIndex % 2) * SEC_SIZE;
            #else
            ulSrcAddr = ulSrcBaseAddr + (usSecIndex / 2) * CW_INFO_SZ + (usSecIndex % 2) * SEC_SIZE;
            #endif

            ulDstAddr = ulDstBaseAddr + (sDiffSecCnt * SEC_SIZE);

            #if defined(SIM) && defined(HOST_SATA)
            if (*(U32 *)(ulSrcAddr + 4) != *(U32 *)(ulDstAddr + 4))
            {
                DBG_Printf("ulSrcAddr:0x%x, ulDstAddr:0x%x\n", *(U32 *)(ulSrcAddr + 4), *(U32 *)(ulDstAddr + 4));
                DBG_Printf("Data misscompare\n");
                DBG_Getch();
            }
            #endif
            HAL_DMAECopyOneBlock(ulDstAddr, ulSrcAddr, SEC_SIZE);
        }
    }

    // Dst Data Address - Multi Plane GC Local Read for Performance
    if ((FALSE == ptReqEntry->tFlashDesc.bsMergeRdEn)
        && (INVALID_4F != ptReqEntry->atBufDesc[1].bsBufID)
        && (FALSE == ptReqEntry->tFlashDesc.bsRdRedOnly))
    {
        atBuffEntry[0].bsBufID = ptReqEntry->atBufDesc[0].bsBufID;
        atBuffEntry[0].bsSecStart = ptReqEntry->atBufDesc[0].bsSecStart;
        atBuffEntry[0].bsSecLen = ptReqEntry->atBufDesc[0].bsSecLen;
        atBuffEntry[1].bsBufID = ptReqEntry->atBufDesc[1].bsBufID;
        atBuffEntry[1].bsSecStart = ptReqEntry->atBufDesc[1].bsSecStart;
        atBuffEntry[1].bsSecLen = ptReqEntry->atBufDesc[1].bsSecLen;

        usDstSecStartInPage = ptReqEntry->tFlashDesc.bsSecStart;
        usDstSecLenInPage = ptReqEntry->tFlashDesc.bsSecLen;

        for (usSecIndex = 0; usSecIndex < usSrcSecLenInPage; usSecIndex++)
        {
            usCurSecInPage = usSrcSecStartInPage + usSecIndex;
            sDiffSecCnt = usCurSecInPage - usDstSecStartInPage;

            // skip the head
            if (sDiffSecCnt < 0)
            {
                continue;
            }
            // break the tail
            if (sDiffSecCnt >= usDstSecLenInPage)
            {
                break;
            }

            #ifdef ASIC
            ulSrcAddr = ulSrcBaseAddr + (usSecIndex / 2) * (CW_INFO_SZ << 1) + (usSecIndex % 2) * SEC_SIZE;
            #else
            ulSrcAddr = ulSrcBaseAddr + (usSecIndex / 2) * CW_INFO_SZ + (usSecIndex % 2) * SEC_SIZE;
            #endif

            if ((U16)sDiffSecCnt < atBuffEntry[0].bsSecLen)
            {
                ulDstBaseAddr = COM_GetMemAddrByBufferID(atBuffEntry[0].bsBufID, TRUE, BUF_SIZE_BITS) + atBuffEntry[0].bsSecStart * SEC_SIZE;
                ulDstAddr = ulDstBaseAddr + (usSrcSecStartInPage + usSecIndex - usDstSecStartInPage) * SEC_SIZE;
            }
            else
            {
                ulDstBaseAddr = COM_GetMemAddrByBufferID(atBuffEntry[1].bsBufID, TRUE, BUF_SIZE_BITS) + atBuffEntry[1].bsSecStart * SEC_SIZE;
                ulDstAddr = ulDstBaseAddr + (sDiffSecCnt - atBuffEntry[0].bsSecLen) * SEC_SIZE;
            }

            #if defined(SIM) && defined(HOST_SATA)
            if (*(U32 *)(ulSrcAddr + 4) != *(U32 *)(ulDstAddr + 4))
            {
                DBG_Printf("ulSrcAddr:0x%x, ulDstAddr:0x%x\n", *(U32 *)(ulSrcAddr + 4), *(U32 *)(ulDstAddr + 4));
                DBG_Printf("Data misscompare\n");
                DBG_Getch();
            }
            #endif

            HAL_DMAECopyOneBlock(ulDstAddr, ulSrcAddr, SEC_SIZE);
        }
    }

    // Dst Data Address - Merge Read
    if ((FALSE == ptReqEntry->tFlashDesc.bsHostRdEn)
        && (TRUE == ptReqEntry->tFlashDesc.bsMergeRdEn)
        && (INVALID_4F == ptReqEntry->atBufDesc[1].bsBufID)
        && (FALSE == ptReqEntry->tFlashDesc.bsRdRedOnly))
    {
        U8 ucLpnBitMap, ucPos = 0;

        ucLpnBitMap = ptReqEntry->tFlashDesc.bsLpnBitmap;
        while (0 == (ucLpnBitMap & 1))
        {
            ucLpnBitMap >>= 1;
        }
        usDstSecStartInPage = ptCtrlEntry->tDTxCtrl.aSecAddr[0].bsSecStart + ucPlnIdx * SEC_PER_PHYPG;

        atBuffEntry[0].bsBufID = ptReqEntry->atBufDesc[0].bsBufID;
        atBuffEntry[0].bsSecStart = ptReqEntry->atBufDesc[0].bsSecStart;
        ulDstBaseAddr = COM_GetMemAddrByBufferID(atBuffEntry[0].bsBufID, TRUE, BUF_SIZE_BITS) + atBuffEntry[0].bsSecStart * SEC_SIZE;

        for (usSecIndex = 0; usSecIndex < usSrcSecLenInPage; usSecIndex++)
        {
            usCurSecInPage = usSrcSecStartInPage + usSecIndex;
            sDiffSecCnt = usCurSecInPage - usDstSecStartInPage;

            // skip the head
            if (sDiffSecCnt < 0)
            {
                continue;
            }

            if (0 != (ucLpnBitMap & 1))
            {
                #ifdef ASIC
                ulSrcAddr = ulSrcBaseAddr + (usSecIndex / 2) * (CW_INFO_SZ << 1) + (usSecIndex % 2) * SEC_SIZE;
                #else
                ulSrcAddr = ulSrcBaseAddr + (usSecIndex / 2) * CW_INFO_SZ + (usSecIndex % 2) * SEC_SIZE;
                #endif
                ulDstAddr = ulDstBaseAddr + sDiffSecCnt * SEC_SIZE;

                #if defined(SIM) && defined(HOST_SATA)
                if (*(U32 *)(ulSrcAddr + 4) != *(U32 *)(ulDstAddr + 4))
                {
                    DBG_Printf("ulSrcAddr:0x%x, ulDstAddr:0x%x\n", *(U32 *)(ulSrcAddr + 4), *(U32 *)(ulDstAddr + 4));
                    DBG_Printf("Data misscompare\n");
                    DBG_Getch();
                }
                #endif

                HAL_DMAECopyOneBlock(ulDstAddr, ulSrcAddr, SEC_SIZE);
            }

            ucLpnBitMap >>= 1;
        }
    }

    if ((NULL != ptReqEntry->ulSpareAddr) && ((usSrcCwStartInPln + usSrcCwLenInPln) == CW_PER_PLN))
    {
        U8 ucFCmdPtr;

        if (TRUE == ptCtrlEntry->bsIntrReq)
        {
            ucFCmdPtr = ptCtrlEntry->bsCtrlPtr;
            ulDstAddr = RED_ABSOLUTE_ADDR(MCU2_ID, ptReqEntry->bsTLun, ucFCmdPtr) + ucPlnIdx * RED_SZ;
        }
        else
        {
            ucFCmdPtr = ptCtrlEntry->ptReqEntry->bsReqPtr;
            ulDstAddr = RED_ABSOLUTE_ADDR(MCU1_ID, ptReqEntry->bsTLun, ucFCmdPtr) + ucPlnIdx * RED_SZ;
        }

        #ifdef ASIC
        ulSrcAddr = ulSrcBaseAddr + (usSrcCwLenInPln * (CW_INFO_SZ << 1)) - CW_INFO_SZ;
        #else
        ulSrcAddr = ulSrcBaseAddr + (usSrcCwLenInPln * CW_INFO_SZ);
        #endif

        #ifdef SIM
        if ((*(U32 *)ulSrcAddr != *(U32 *)ulDstAddr) && (0 == ucPlnIdx))
        {
            DBG_Printf("ulSrcAddr:0x%x, ulDstAddr:0x%x, ulSrcData:0x%x, ulDstData:0x%x\n", ulSrcAddr, ulDstAddr, *(U32 *)ulSrcAddr, *(U32 *)ulDstAddr);
            DBG_Printf("Red misscompare\n");
        }
        else
        {
            //DBG_Printf("Correct RedData:0x%x\n", *(U32 *)ulDstAddr);
        }
        #endif

        HAL_DMAECopyOneBlock(ulDstAddr, ulSrcAddr, sizeof(NFC_RED));
    }

    return;
}

void L3_LdpcSCopyBackSinglePln(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry,U8 ucPln, U32 ulDecoResultBufId)
{
    U32 ulDescIdx = 0;
    UECCH_SOFTDECO *pLdpcSoftDecoCtrl = &ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tSoftDeco;

    L3_LdpcSDescTotalCnt(pLdpcSoftDecoCtrl->aDecSramBitMap[ucPln], ucPln);

    while(g_aCwNumPerDesc[ulDescIdx].bsCodeWordCount != 0)// CwNum != 0
    {
        L3_LdpcSCopyBackCw(ptCtrlEntry, ulDecoResultBufId,ulDescIdx,ucPln);
        ulDescIdx++;
    }

    return;
}

/*==============================================================================
Func Name  : L3_LdpcSDescCfg
Input      : U32 ulDescriptorCnt
             U32 ulShiftReadTimes
             FLASH_ADDR *pFlashAddr
             UECCH_SOFTDECO *pLdpcSoftDecoCtrl
             U8 ucScrMod
             U8 ucPageType
Output     : NONE
Return Val : NONE
Discription: Setting soft decoder descriptor.
Usage      :
History    :
1. 2016.01.21 MapleXu create function
==============================================================================*/
void L3_LdpcSDescCfg(U32 ulDescriptorCnt, U32 ulShiftReadTimes, FLASH_ADDR *pFlashAddr, UECCH_SOFTDECO *pLdpcSoftDecoCtrl, U8 ucScrMod, U8 ucPageType)
{
    ASSERT((ulDescriptorCnt > 0) && (ulDescriptorCnt <= VALID_DESCRIPTOR_CNT));
    ASSERT((ulShiftReadTimes > 0) && (ulShiftReadTimes <= LDPC_SOFT_MAX_DEC_CNT));
    // @todo Add assert here for ulPu later. ASSERT(ulPu < );
    // @todo Add assert here for ulLun later. ASSERT(ulLun < );
    // @todo Add assert here for ulPlane later. ASSERT(ulPlane < );
    ASSERT(pLdpcSoftDecoCtrl != NULL);

    /* Get valid Desc ID from FW WPTR */
    U32 ulDescriptorId = pLdpcSoftDecoCtrl->bsStartDescriptorId = HAL_LdpcSGetDescId();
    U32 ulPu = pFlashAddr->ucPU;
    U32 ulPlane = pFlashAddr->bsPln;
    DESC_EN_CTL tDescEnCtl = { 0 };
    U32 i = 0;
    U8 ucLpnIndex, ucSecInLpn;
    U32 ulDescriptorStartId = ulDescriptorId;

    L3_LdpcSDescCwAssign(pLdpcSoftDecoCtrl->aDecSramBitMap[ulPlane], ulDescriptorId);

    tDescEnCtl.bsDramCrcEn  = FALSE;
    tDescEnCtl.bsScrEn      = (SCRAMBLE_MSK_EN ? FALSE : TRUE);
    tDescEnCtl.bsCrcEn      = TRUE;
    tDescEnCtl.bsLbaEn      = TRUE;
    tDescEnCtl.bsRedunEn    = TRUE;

    #ifdef LBA_CHK_EN
    tDescEnCtl.bsLbaChkEn   = TRUE;
    #endif

    #ifdef DATA_EM_ENABLE
    tDescEnCtl.bsEmEn       = TRUE;
    #endif

    tDescEnCtl.bsScrShfNum = HAL_FlashGetScrSeed(pFlashAddr->usPage, ucScrMod, ucPageType);

    // Config all descriptors one by one.
    for (i = 0; i < ulDescriptorCnt; ++i)
    {
        ucLpnIndex = (g_aCodeWordSegment[ulDescriptorId].bsStartCodeWord << SEC_PER_CW_BITS) / SEC_PER_LPN;
        ucSecInLpn = (g_aCodeWordSegment[ulDescriptorId].bsStartCodeWord << SEC_PER_CW_BITS) % SEC_PER_LPN;

        #if defined(LBA_CHK_EN) || defined(DATA_EM_ENABLE)
        tDescEnCtl.ulLba = (((ulPlane << (SEC_PER_PHYPG_BITS - SEC_PER_LPN_BITS)) + ucLpnIndex) << SEC_PER_LPN_BITS) + ucSecInLpn;
        #endif
        tDescEnCtl.bsShiftReadTimes = ulShiftReadTimes;

        /*configuration of this FW descriptor*/
        HAL_LdpcSDescCfg(ulPu, ulDescriptorId, &tDescEnCtl);
        HAL_LdpcSDescCfgLlr(ulDescriptorId, ulShiftReadTimes);

        U32 j = 0;
        volatile SOFT_DEC_DESCRIPTOR *pSoftDecDesc = HAL_LdpcSGetDescAddr(ulDescriptorId);
        U32 ulRawPageReadBufferId, ulDecodeResultBufferId;
        U32 ulStartCodeWord = L3_LdpcSGetStartCw(ulDescriptorId);

        for (j = 0; j < ulShiftReadTimes; ++j)
        {
            ulRawPageReadBufferId = L3_BufMgrGetBufIDByNode(pLdpcSoftDecoCtrl->aRawPageReadBufferId[j]);
            #ifdef ASIC
            pSoftDecDesc->aReadAddr[j] = HAL_NfcGetDmaAddr(ulRawPageReadBufferId, (ulStartCodeWord << 2), BUF_SIZE_BITS) << 1;
            #else
            pSoftDecDesc->aReadAddr[j] = HAL_NfcGetDmaAddr(ulRawPageReadBufferId, (ulStartCodeWord << 1), BUF_SIZE_BITS) << 1;
            #endif
        }

        ulDecodeResultBufferId = L3_BufMgrGetBufIDByNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[ulPlane]);

        #ifdef ASIC
        pSoftDecDesc->ulWADDR = HAL_NfcGetDmaAddr(ulDecodeResultBufferId, (ulStartCodeWord << 2), BUF_SIZE_BITS) << 1;
        #else
        pSoftDecDesc->ulWADDR = HAL_NfcGetDmaAddr(ulDecodeResultBufferId, (ulStartCodeWord << 1), BUF_SIZE_BITS) << 1;
        #endif

        ulDescriptorId = (ulDescriptorId + 1) % DESCRIPTOR_MAX_NUM;
    }

#if 0 // Check the soft dec infolen worng
    for (i = ulDescriptorStartId; i < (ulDescriptorStartId + ulDescriptorCnt); i++)
    {
        volatile SOFT_DEC_DESCRIPTOR *pSoftDecDescCheck = HAL_LdpcSGetDescAddr(i);

        if ((pSoftDecDescCheck->bsInfoLen1 < CW_INFO_SZ) || (pSoftDecDescCheck->bsInfoLen2 < CW_INFO_SZ) || (pSoftDecDescCheck->bsInfoLen3 < CW_INFO_SZ) || (pSoftDecDescCheck->bsInfoLen4 < CW_INFO_SZ))
        {
            DBG_Printf("The info length is wrong\n");
            DBG_Getch();
        }
    }
#endif

    /* Push all the descriptors and Trigger LDPC wrapper*/
    HAL_LdpcSTrigger(ulDescriptorCnt);

    return;
}

/*==============================================================================
Func Name  : L3_LdpcSSinglePlane
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : NONE
Discription: Send raw read command by single plane
Usage      :
History    :
1. 2016.8.2 JasonGuo create function
==============================================================================*/
BOOL L3_LdpcSSinglePlane(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    UECCH_SOFTDECO *pLdpcSoftDecoCtrl = &(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tSoftDeco);
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FLASH_ADDR tFlashAddr = { 0 };
    U32 i;

    tFlashAddr.ucPU = L3_GET_PU(ptReqEntry->bsTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ptReqEntry->bsTLun);
    tFlashAddr.usPage = ptCtrlEntry->bsPhyPage;

    tFlashAddr.bsPln = pLdpcSoftDecoCtrl->bsCurPlaneIndex;
    tFlashAddr.usBlock = L2_BbtGetPbnBindingTable(ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsPhyBlk, tFlashAddr.bsPln);

    BOOL bCurrentPlaneFinishDecode = FALSE;

    switch (pLdpcSoftDecoCtrl->bsSinglePlaneStatus)
    {
        case SOFT_DECO_INIT:
        {
            // Read CW BitMap, and get the new descriptors' number*/
            pLdpcSoftDecoCtrl->bsDescriptorCount = L3_LdpcSDescTotalCnt(pLdpcSoftDecoCtrl->aDecSramBitMap[tFlashAddr.bsPln], tFlashAddr.bsPln);
            if (pLdpcSoftDecoCtrl->bsDescriptorCount == 0)
            {
                // Handle the situation that count of descriptor equal to zero.
                pLdpcSoftDecoCtrl->bsHaveNotUeccPlane |= (1 << tFlashAddr.bsPln);
                pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_SUCCESS;
                bCurrentPlaneFinishDecode = TRUE;
            }
            else
            {
                pLdpcSoftDecoCtrl->bsCurShiftReadTime = 0;
                pLdpcSoftDecoCtrl->bsShiftReadTimesIndex = 0;

                if (NULL == pLdpcSoftDecoCtrl->aRawPageReadBufferId[0])
                {
                    pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_ALLOCATE_BUF;
                }
                else
                {
                    pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_READ_RAW_DATA;
                }
            }
            break;
        }
        case SOFT_DECO_ALLOCATE_BUF:
        {
            U32 ulShiftReadTimes = HAL_LdpcSGetReadTime(pLdpcSoftDecoCtrl->bsShiftReadTimesIndex);

            if ((PLN_PER_LUN + LDPC_SOFT_MAX_DEC_CNT) <= L3_BufMgrGetFreeCnt())
            {
                SDL_NODE *ptRDBufNode;

                for (i = 0; i < LDPC_SOFT_MAX_DEC_CNT; i++)
                {
                    ptRDBufNode = L3_BufMgrAllocateNode();
                    pLdpcSoftDecoCtrl->aRawPageReadBufferId[i] = ptRDBufNode;
                }

                if (FCMD_REQ_SUBTYPE_SINGLE == ptCtrlEntry->ptReqEntry->bsReqSubType)
                {
                    ptRDBufNode = L3_BufMgrAllocateNode();
                    pLdpcSoftDecoCtrl->aDecodeResultBufferId[tFlashAddr.bsPln] = ptRDBufNode;

                }
                else
                {
                    for (i = 0; i < PLN_PER_LUN; i++)
                    {
                        ptRDBufNode = L3_BufMgrAllocateNode();
                        pLdpcSoftDecoCtrl->aDecodeResultBufferId[i] = ptRDBufNode;

                    }
                }

                pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_READ_RAW_DATA;
            }
            break;
        }
        case SOFT_DECO_READ_RAW_DATA:
        {
            if (TRUE == HAL_NfcGetIdle(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                U32 ulCurShiftReadTime = pLdpcSoftDecoCtrl->bsCurShiftReadTime;
                U32 ulShiftReadTimes = HAL_LdpcSGetReadTime(pLdpcSoftDecoCtrl->bsShiftReadTimesIndex);
                BOOL bFinishSoftDecodeShiftRead;

                ++(pLdpcSoftDecoCtrl->bsCurShiftReadTime);

                bFinishSoftDecodeShiftRead = L3_LdpcSShiftRead(ptCtrlEntry, ulCurShiftReadTime, ulShiftReadTimes);
                if (bFinishSoftDecodeShiftRead == TRUE)
                {
                    pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_TRIG;
                }
            }

            break;
        }
        case SOFT_DECO_TRIG:
        {
            if (pLdpcSoftDecoCtrl->bsDescriptorCount <= HAL_LdpcSFifoGetFreeNum())
            {
                U32 ulShiftReadTimes = HAL_LdpcSGetReadTime(pLdpcSoftDecoCtrl->bsShiftReadTimesIndex);
                U32 ulDescriptorCnt = pLdpcSoftDecoCtrl->bsDescriptorCount;

                L3_LdpcSDescCfg(ulDescriptorCnt, ulShiftReadTimes, &tFlashAddr, pLdpcSoftDecoCtrl, ptCtrlEntry->bsScrMod, ptCtrlEntry->bsIntrPage);

                pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_TRIG_CHK;
            }

            break;
        }
        case SOFT_DECO_TRIG_CHK:
        {
            U32 ulStartDescriptorId = pLdpcSoftDecoCtrl->bsStartDescriptorId;
            U32 ulDescriptorCnt = pLdpcSoftDecoCtrl->bsDescriptorCount;

            if (TRUE == HAL_LdpcSAllDone(ulStartDescriptorId, ulDescriptorCnt))
            {
                // Release raw page buffer that store shift read data
                U32 ulShiftReadTimes = HAL_LdpcSGetReadTime(pLdpcSoftDecoCtrl->bsShiftReadTimesIndex);

                if (TRUE == HAL_LdpcSAllSuccess(ulStartDescriptorId, ulDescriptorCnt))
                {
                    if (TRUE == HAL_LdpcSCrcStsCheck(ulStartDescriptorId, ulDescriptorCnt))
                    {
                        pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_SUCCESS;
                    }
                    else
                    {
                        if (FCMD_REQ_SUBTYPE_SINGLE == ptCtrlEntry->ptReqEntry->bsReqSubType)
                        {
                            // Release raw page buffer of current plane that store the result of LDPC Soft-Decode.
                            L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[tFlashAddr.bsPln]);
                            pLdpcSoftDecoCtrl->aDecodeResultBufferId[tFlashAddr.bsPln] = 0;
                        }
                        else
                        {
                            for (i = 0; i < PLN_PER_LUN; i++)
                            {
                                L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[i]);
                                pLdpcSoftDecoCtrl->aDecodeResultBufferId[i] = 0;
                            }
                        }

                        #ifdef XOR_ENABLE
                        if ((TRUE == ptReqEntry->bsTableReq) || (TRUE == ptCtrlEntry->bsIsXorRead))
                        {
                            pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_FAIL2FAIL;
                        }
                        else
                        {
                            pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_FAIL2XOR;
                        }
                        #else
                        pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_FAIL2FAIL;
                        #endif
                    }

                    bCurrentPlaneFinishDecode = TRUE;
                }
                else
                {
                    U32 ulShiftReadTimesTotalCnt = HAL_LdpcSGetReadCnt();
                    ++(pLdpcSoftDecoCtrl->bsShiftReadTimesIndex);

                    if (pLdpcSoftDecoCtrl->bsShiftReadTimesIndex == ulShiftReadTimesTotalCnt)
                    {
                        if (FCMD_REQ_SUBTYPE_SINGLE == ptCtrlEntry->ptReqEntry->bsReqSubType)
                        {
                            // Release raw page buffer of current plane that store the result of LDPC Soft-Decode.
                            L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[tFlashAddr.bsPln]);
                            pLdpcSoftDecoCtrl->aDecodeResultBufferId[tFlashAddr.bsPln] = 0;
                        }
                        else
                        {
                            for (i = 0; i < PLN_PER_LUN; i++)
                            {
                                L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[i]);
                                pLdpcSoftDecoCtrl->aDecodeResultBufferId[i] = 0;
                            }
                        }

                        #ifdef XOR_ENABLE
                        if ((TRUE == ptReqEntry->bsTableReq) || (TRUE == ptCtrlEntry->bsIsXorRead))
                        {
                            pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_FAIL2FAIL;
                        }
                        else
                        {
                            pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_FAIL2XOR;
                        }
                        #else
                        pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_FAIL2FAIL;
                        #endif

                        bCurrentPlaneFinishDecode = TRUE;
                    }
                    else if (pLdpcSoftDecoCtrl->bsShiftReadTimesIndex < ulShiftReadTimesTotalCnt)
                    {
                        pLdpcSoftDecoCtrl->bsCurShiftReadTime = 0;
                        pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_READ_RAW_DATA;
                    }
                    else
                    {
                        //DBG_Printf("Impossible situation!");
                        DBG_Getch();
                    }
                }

                HAL_LdpcSDescRelease(ulStartDescriptorId, ulDescriptorCnt);
            }

            break;
        }
        default:
        {
            DBG_Getch();
            break;
        }
    }

    return bCurrentPlaneFinishDecode;
}

/*==============================================================================
Func Name  : L3_LdpcSCore
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : NONE
Discription: The main function of the soft decoding process
Usage      :
History    :
1. 2016.8.2 JasonGuo create function
==============================================================================*/
void L3_LdpcSCore(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    ASSERT(ptCtrlEntry->ptReqEntry->bsReqSubType < FCMD_REQ_SUBTYPE_IDLE_0);

    FLASH_ADDR tFlashAddr = { 0 };
    UECCH_SOFTDECO *pLdpcSoftDecoCtrl = &(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tSoftDeco);
    BOOL bCurrentPlaneFinishDecode;
    BOOL bSinglePln;

    tFlashAddr.ucPU = L3_GET_PU(ptCtrlEntry->ptReqEntry->bsTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ptCtrlEntry->ptReqEntry->bsTLun);
    tFlashAddr.usPage = ptCtrlEntry->bsPhyPage;

    if (TRUE == ptCtrlEntry->bsEMEn || TRUE == ptCtrlEntry->bsLBAChkEn)
    {
        pLdpcSoftDecoCtrl->ulLba = ptCtrlEntry->ptReqEntry->tHostDesc.ulFtlLba >> 3;
    }

    switch (pLdpcSoftDecoCtrl->bsWholeStatus)
    {
        case SOFT_DECO_INIT:
        {
            if (FCMD_REQ_SUBTYPE_SINGLE == ptCtrlEntry->ptReqEntry->bsReqSubType)
            {
                pLdpcSoftDecoCtrl->bsCurPlaneIndex = ptCtrlEntry->ptReqEntry->tFlashDesc.bsPlnNum;
                bSinglePln = TRUE;
            }
            else
            {
                pLdpcSoftDecoCtrl->bsCurPlaneIndex = 0;
                bSinglePln = FALSE;
            }

            L3_LdpcSRdAllBitMap(pLdpcSoftDecoCtrl,tFlashAddr.ucPU, tFlashAddr.ucLun, ptCtrlEntry->bsNfcqPtr, bSinglePln);
            pLdpcSoftDecoCtrl->bsHaveNotUeccPlane = 0;
            pLdpcSoftDecoCtrl->bsWholeStatus = SOFT_DECO_TRIG;

            break;
        }
        case SOFT_DECO_TRIG:
        {
            U32 i;
            bCurrentPlaneFinishDecode = L3_LdpcSSinglePlane(ptCtrlEntry);
            if (bCurrentPlaneFinishDecode == TRUE)
            {
                if (SOFT_DECO_SUCCESS == pLdpcSoftDecoCtrl->bsSinglePlaneStatus)
                {
                    if (FCMD_REQ_SUBTYPE_SINGLE == ptCtrlEntry->ptReqEntry->bsReqSubType)
                    {
                        for (i = 0; i < LDPC_SOFT_MAX_DEC_CNT; ++i)
                        {
                            L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aRawPageReadBufferId[i]);
                            pLdpcSoftDecoCtrl->aRawPageReadBufferId[i] = 0;
                        }
                        pLdpcSoftDecoCtrl->bsWholeStatus = SOFT_DECO_SUCCESS;
                    }
                    else
                    {
                        if (pLdpcSoftDecoCtrl->bsCurPlaneIndex == (PLN_PER_LUN - 1))
                        {
                            for (i = 0; i < LDPC_SOFT_MAX_DEC_CNT; ++i)
                            {
                                L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aRawPageReadBufferId[i]);
                                pLdpcSoftDecoCtrl->aRawPageReadBufferId[i] = 0;
                            }
                            pLdpcSoftDecoCtrl->bsWholeStatus = SOFT_DECO_SUCCESS;
                        }
                        else if (pLdpcSoftDecoCtrl->bsCurPlaneIndex < (PLN_PER_LUN - 1))
                        {
                            ++(pLdpcSoftDecoCtrl->bsCurPlaneIndex);
                        }
                        else
                        {
                            //DBG_Printf("Error Plane Index!");
                            DBG_Getch();  // Impossible situation.
                        }
                    }
                }
                else if (SOFT_DECO_FAIL2XOR == pLdpcSoftDecoCtrl->bsSinglePlaneStatus || SOFT_DECO_FAIL2FAIL == pLdpcSoftDecoCtrl->bsSinglePlaneStatus)
                {

                    for (i = 0; i < LDPC_SOFT_MAX_DEC_CNT; ++i)
                    {
                        L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aRawPageReadBufferId[i]);
                        pLdpcSoftDecoCtrl->aRawPageReadBufferId[i] = 0;
                    }
                    pLdpcSoftDecoCtrl->bsWholeStatus = pLdpcSoftDecoCtrl->bsSinglePlaneStatus;
                }
                else
                {
                    //DBG_Printf("Error LDPC Soft-Decode Status!");
                    DBG_Getch();
                }

                pLdpcSoftDecoCtrl->bsSinglePlaneStatus = SOFT_DECO_INIT;
            }

            break;
        }
        default:
        {
            DBG_Printf("Error LDPC Soft-Decode Whole Status!");
            DBG_Getch();
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_LdpcSCopyBack
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : NONE
Discription: Copy data by single plane or multi-plane.
Usage      :
History    :
1. 2016.8.2 JasonGuo create function
==============================================================================*/
void L3_LdpcSCopyBack(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U32 i = 0;
    U32 ulNeedReleaseBufferCnt = 0;
    UECCH_SOFTDECO *pLdpcSoftDecoCtrl;
    U32 ulDecodeResultBufferId = 0;

    pLdpcSoftDecoCtrl = &ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tSoftDeco;

    if (FCMD_REQ_SUBTYPE_SINGLE == ptCtrlEntry->ptReqEntry->bsReqSubType)
    {
        if (0 != pLdpcSoftDecoCtrl->aDecSramBitMap[pLdpcSoftDecoCtrl->bsCurPlaneIndex])
        {
            // copy the result to the corrosponding cw position.
            ulDecodeResultBufferId = L3_BufMgrGetBufIDByNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[pLdpcSoftDecoCtrl->bsCurPlaneIndex]);
            L3_LdpcSCopyBackSinglePln(ptCtrlEntry, pLdpcSoftDecoCtrl->bsCurPlaneIndex, ulDecodeResultBufferId);

            // relase the buffer
            L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[pLdpcSoftDecoCtrl->bsCurPlaneIndex]);
            pLdpcSoftDecoCtrl->aDecodeResultBufferId[pLdpcSoftDecoCtrl->bsCurPlaneIndex] = 0;
        }
    }
    else
    {
        ulNeedReleaseBufferCnt = pLdpcSoftDecoCtrl->bsCurPlaneIndex + 1;

        // Copy decoded CW to the right position, then release Decode Result Buffer
        for (i = 0; i < ulNeedReleaseBufferCnt; ++i)
        {
            if (0 != pLdpcSoftDecoCtrl->aDecSramBitMap[i])
            {
                // copy the result to the corrosponding cw position.
                ulDecodeResultBufferId = L3_BufMgrGetBufIDByNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[i]);
                L3_LdpcSCopyBackSinglePln(ptCtrlEntry, i, ulDecodeResultBufferId);

                // relase the buffer
                L3_BufMgrReleaseNode(pLdpcSoftDecoCtrl->aDecodeResultBufferId[i]);
                pLdpcSoftDecoCtrl->aDecodeResultBufferId[i] = 0;
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_SoftDecoder
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : U32
Discription: Decoder success -> recovery the data and red (option).
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_SoftDecoder(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;

    UECCH_SOFTDECO *pLdpcSoftDecoCtrl;
    SDL_NODE *ptResultBufNode;
    U8 ucTLun;
    U16 usBlk, usPhyBlk, usPage, usPhyPage, usSecInPage, usSecLen, usLpnBitMap;

    ucTLun = ptReqEntry->bsTLun;
    usBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    usSecInPage = ptReqEntry->tFlashDesc.bsSecStart;
    usSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    usLpnBitMap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    pLdpcSoftDecoCtrl = &(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tSoftDeco);

    /* Note:
        If the WholeStatus is success or fail, indicate the LDPC is done. => Needn't schedule to enter the function.
        But no allocate buffer resource, firmware will schedule the L3 again.
        So adding the judgment of the purpose is reduced cycle.
    */
    if (SOFT_DECO_SUCCESS != pLdpcSoftDecoCtrl->bsWholeStatus &&
        SOFT_DECO_FAIL2XOR != pLdpcSoftDecoCtrl->bsWholeStatus &&
        SOFT_DECO_FAIL2FAIL != pLdpcSoftDecoCtrl->bsWholeStatus)
    {
        L3_LdpcSCore(ptCtrlEntry);
    }

    if (SOFT_DECO_SUCCESS == pLdpcSoftDecoCtrl->bsWholeStatus)
    {
        // The ResultBuf is for DRQ by NVMe, SATA need't to use he buffer.
        // For code consistency, so to do the same behavior.
        if (0 >= L3_BufMgrGetFreeCnt())
        {
            return;
        }
        ptResultBufNode = L3_BufMgrAllocateNode();
        ptErrHEntry->tUeccHCtrl.ptBufNodePtr = ptResultBufNode;

        L3_LdpcSCopyBack(ptCtrlEntry);
        L2_PBIT_Set_Weak(L2_GET_SPU(ucTLun), usBlk, TRUE);

        L3_BufMgrReleaseNode(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.ptBufNodePtr);

        // Response host read cmd.
        // SATA : Set buffer bit map after soft dec success; NVMe : Trigger DRQ to move data from dram to host memory.
        if (TRUE == ptCtrlEntry->ptReqEntry->tFlashDesc.bsHostRdEn)
        {
            L3_HostReadRecover(ptCtrlEntry, FALSE);
        }

        // For L2
        if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
        {
            // Note:
            //  If the cmd isn't intrReq, after call L3_IFUpdtReqStatus function.
            //  L3 can't use the ptCtrlEntry, because L2 maybe update the ptCtrlEntry.
            L3_IFUpdtReqStatus(ptCtrlEntry);
        }

        L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);

        DBG_Printf("TLun=%d Blk=%d_%d Page=%d_%d SecInPage=%d_%d_%d LDPC Soft DEC SUCCESS!!!\n",
            ucTLun, usBlk, usPhyBlk, usPage, usPhyPage, usSecInPage, usSecLen, usLpnBitMap);

        ptErrHEntry->tUeccHCtrl.bsSubStage = pLdpcSoftDecoCtrl->bsWholeStatus;
    }
    else if (SOFT_DECO_FAIL2XOR == pLdpcSoftDecoCtrl->bsWholeStatus || SOFT_DECO_FAIL2FAIL == pLdpcSoftDecoCtrl->bsWholeStatus)
    {
        DBG_Printf("TLun=%d Blk=%d_%d Page=%d_%d SecInPage=%d_%d_%d LDPC Soft DEC FAIL!!!\n",
            ucTLun, usBlk, usPhyBlk, usPage, usPhyPage, usSecInPage, usSecLen, usLpnBitMap);

        ptErrHEntry->tUeccHCtrl.bsSubStage = pLdpcSoftDecoCtrl->bsWholeStatus;
    }

    return;
}

/*====================End of this file========================================*/

