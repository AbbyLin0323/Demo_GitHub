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
Filename    : TEST_NfcFlashTypeAdpter.c
Version     : Ver 1.0
Author      : abby
Date        : 201600928
Description : compile by MCU1 and MCU2
              related with flash type
Others      :
Modify      :
*******************************************************************************/
#ifndef BOOTLOADER

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcFlashTypeAdapter.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR TLC_PRG_TABLE *g_pTLCPrgTable; // Allocated from shared Dsram1, for TLC write cntl
GLOBAL MCU12_VAR_ATTR TLC_P2L_PG_MAP *g_pTLCP2LPageMap; // Allocated from shared Dsram1, phy to logic mapping

extern MCU12_VAR_ATTR GLOBAL WRITE_DATA_BUFF g_tWrBuf;
extern MCU12_VAR_ATTR GLOBAL WRITE_RED_BUFF g_tWrRed;
extern MCU12_VAR_ATTR U16 g_aWrBufId[DATA_PATT_NUM][FCMDQ_DEPTH][PHYPG_PER_PRG];

extern void TEST_NfcExtPrepareDummyData(U8 ucTLun, U16 usPhyPage, U8 ucWptr);
//extern void DMAE_TEXT_ATTR HAL_DMAECopyOneBlock(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte);

void MCU1_DRAM_TEXT TEST_NfcTLCPrgTableInit(void)
{
    U16 usPhyPage;
    U16 usLogicPage = 0;
    PAIR_PAGE_TYPE ePageType;
    
    /* page 0-111 */
    for (usPhyPage = 0; usPhyPage < 112; usPhyPage++)
    {
        if (usPhyPage < 16)
        {
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usPhyPage;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPageType = LOW_PAGE_WITHOUT_HIGH;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPrgBufNum = 1; 
        }
        else
        {
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usPhyPage++;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPageType = LOW_PAGE;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPrgBufNum = 2; 
        }
        usLogicPage++;
    }
    /* page 112-1504 */
    for (; usPhyPage < 1505; usPhyPage++)
    {
        ePageType = HAL_GetFlashPairPageType(usPhyPage);
        if(EXTRA_PAGE == ePageType)
        {
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usPhyPage;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPairPhyPage = HAL_GetHighPageIndexfromExtra(usPhyPage);
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPageType = EXTRA_PAGE;
        }
        else//LOW_PAGE
        {
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usPhyPage++;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPairPhyPage = usPhyPage;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPageType = LOW_PAGE;
        }
        g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPrgBufNum = 2;
        usLogicPage++;
    }
    
    /* page 1505-1535 */
    for (; usPhyPage < PG_PER_BLK; usPhyPage++)
    {
        ePageType = HAL_GetFlashPairPageType(usPhyPage);
        if(LOW_PAGE_WITHOUT_HIGH == ePageType)
        {
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usPhyPage;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPairPhyPage = INVALID_4F;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPageType = LOW_PAGE_WITHOUT_HIGH;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPrgBufNum = 1;
        }
        else//EXTRA_PAGE
        {
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usPhyPage;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPairPhyPage = HAL_GetHighPageIndexfromExtra(usPhyPage);
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPageType = EXTRA_PAGE;
            g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPrgBufNum = 2;
        }
        usLogicPage++;
    }
}

void MCU1_DRAM_TEXT TEST_NfcTLCP2LMapInit(void)
{
    U16 usPhyPage;
    U16 usLogicPage = 0;
    PAIR_PAGE_TYPE ePageType;

    for(usPhyPage = 0; usPhyPage < 112; usPhyPage++)
    {
        g_pTLCP2LPageMap->aLogicPage[usPhyPage++] = usLogicPage;
        g_pTLCP2LPageMap->aLogicPage[usPhyPage] = usLogicPage++;
    }
    for(; usPhyPage < 1505; usPhyPage++)
    {
        ePageType = HAL_GetFlashPairPageType(usPhyPage);
        if(EXTRA_PAGE == ePageType)
        {
            g_pTLCP2LPageMap->aLogicPage[usPhyPage] = usLogicPage;
        }
        else
        {
            g_pTLCP2LPageMap->aLogicPage[usPhyPage++] = usLogicPage;
            g_pTLCP2LPageMap->aLogicPage[usPhyPage] = usLogicPage++;
        }
    }
    for (; usPhyPage < PG_PER_BLK; usPhyPage++)
    {
        ePageType = HAL_GetFlashPairPageType(usPhyPage);
        if (EXTRA_PAGE == ePageType)
        {
            g_pTLCP2LPageMap->aLogicPage[usPhyPage] = usLogicPage;
            //upp page mapping to logic page which is the same with it's extra page
            //g_pTLCP2LPageMap->aLogicPage[usPairHighPhyPage] = usLogicPage++;
        }
        else
        {
            g_pTLCP2LPageMap->aLogicPage[usPhyPage] = usLogicPage++;
        }
    }
}

U16 TEST_NfcGetTLCPhyPage(U16 usPrgOrder)
{
    return g_pTLCPrgTable->aTlcPrgTab[usPrgOrder].bsPhyPage;
}

U8 TEST_NfcGetTLCPrgBufNum(U16 usLogicPage, BOOL bSLCMode)
{   
    U8 ucBufNum = 1;
    
    if ((!bSLCMode)&&(LOW_PAGE_WITHOUT_HIGH != HAL_GetFlashPairPageType(usLogicPage)))//usPhyPage = usLogicPage
    {
        ucBufNum = 2;
    }
    
    return ucBufNum;
}

/*  program page mapping  */
U16 TEST_NfcGetPrgVirPageFromOrder(U16 usPrgOrder, BOOL bSLCMode)
{
    if (bSLCMode)
    {
        return usPrgOrder;
    }
    else
    {
        return (TEST_NfcGetTLCPhyPage(usPrgOrder));//VirPage = PhyPage
    }
}

U16 TEST_NfcGetPrgPhyPageFromOrder(U16 usPrgOrder, BOOL bSLCMode)
{
    return (TEST_NfcGetPrgVirPageFromOrder(usPrgOrder, bSLCMode));
}

U16 TEST_NfcGetPrgPhyPageFromVirPage(U16 usVirPage, BOOL bSLCMode)
{
    return usVirPage;
}

/*  read page mapping  */
U16 TEST_NfcGetReadVirPageFromOrder(U16 usPrgOrder, BOOL bSLCMode)
{
    return usPrgOrder;
}

U16 TEST_NfcGetReadPhyPageFromVirPage(U16 usVirPage, BOOL bSLCMode)
{
    return usVirPage;
}

U16 TEST_NfcGetPrgVirPageFromPhyPage(U16 usPhyPage, U8 ucPrgCycle, BOOL bSLCMode)
{
    return usPhyPage;
}

void TEST_NfcAdaptWriteBuffAddr(U8 ucTLun, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode)
{
    U32 *pTargetRed;
    U8  ucPrgPageNum, ucBufIdx;
    U16 usVirPage;
    //U16 aBuffID[DSG_BUFF_SIZE];

    /* slc mode or low page without high : program 32K */
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    ucPrgPageNum = TEST_NfcGetTLCPrgBufNum(usVirPage, bSLCMode);
    
    //RED addr
    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ucWptr);
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;
    HAL_DMAECopyOneBlock((const U32)pTargetRed, (const U32)g_tWrRed.pWrRed[0], (const U32)(LOGIC_RED_SZ *ucPrgPageNum));

    //data buff addr
    for (ucBufIdx = 0; ucBufIdx < ucPrgPageNum; ucBufIdx++)
    {
        ptReqEntry->atBufDesc[ucBufIdx].bsBufID = g_aWrBufId[DATA_PATTERN_SEL][ucWptr][ucBufIdx];
    }
    ptReqEntry->atBufDesc[ucBufIdx].bsBufID = INVALID_4F;
    
#ifdef DATA_CHK
    if (RELAVANT_FLASH_ADDR_DATA == DATA_PATTERN_SEL)
    {
        /* prepare data by flash address for every time of program */
        U16 usPhyPage = TEST_NfcGetPrgPhyPageFromVirPage(usVirPage, bSLCMode);
        TEST_NfcExtPrepareDummyData(ucTLun, usPhyPage, ucWptr);
    }
#endif
}

void TEST_NfcFCmdQAdaptWriteSecRange(FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode)
{
    FCMD_FLASH_DESC *ptFlashDesc;
    U8  ucPrgPageNum, ucBufIdx;

    ptFlashDesc = &ptReqEntry->tFlashDesc;

    /* slc mode or low page without high : program 32K */
    ucPrgPageNum = TEST_NfcGetTLCPrgBufNum(ptFlashDesc->bsVirPage, bSLCMode);
    ptReqEntry->bsReqSubType = (1 == ucPrgPageNum) ? FCMD_REQ_SUBTYPE_ONEPG : FCMD_REQ_SUBTYPE_NORMAL;
    
    ptFlashDesc->bsSecStart = 0;
    ptFlashDesc->bsSecLen = SEC_PER_BUF * ucPrgPageNum;

    for (ucBufIdx = 0; ucBufIdx < ucPrgPageNum; ucBufIdx++)
    {
        ptReqEntry->atBufDesc[ucBufIdx].bsSecStart = 0;
        ptReqEntry->atBufDesc[ucBufIdx].bsSecLen = SEC_PER_BUF;
    }
    
    return;
}
U16 TEST_NfcGetTlcPairPage(U16 usPhyPage)
{
    U16 usPairPage;
    
    if (EXTRA_PAGE == HAL_GetFlashPairPageType(usPhyPage))
    {
        usPairPage = HAL_GetHighPageIndexfromExtra(usPhyPage);
    }
    else
    {
        usPairPage = usPhyPage + 1;
    }
    return usPairPage;
}

U16 TEST_NfcGetNextPPO(U16 usCurrPPO, U8 ucPrgCycle, BOOL bSLCMode)
{
    U8  ucPrgPageNum; 
    U16 usNextPPO = usCurrPPO;

    ucPrgPageNum = TEST_NfcGetTLCPrgBufNum(usCurrPPO, bSLCMode);
    
    if (EXTRA_PAGE == HAL_GetFlashPairPageType(usCurrPPO))
    {
        usNextPPO++;
    }
    else
    {
        usNextPPO += ucPrgPageNum;   
    }
    return usNextPPO;
}

U16 TEST_NfcGetPrgOrderMax(BOOL bSLCMode)
{
    U16 usOrderMax = (TRUE == bSLCMode)? PG_PER_SLC_BLK : TLC_PRG_ORDER_MAX;
    return usOrderMax;
}

U16 TEST_NfcGetReadOrderMax(BOOL bSLCMode)
{
    U16 usOrderMax = (TRUE == bSLCMode)? PG_PER_SLC_BLK : PG_PER_BLK;
    return usOrderMax;
}


U16 TEST_NfcGetPhyPageMax(BOOL bSLCMode)
{
    return TEST_NfcGetPrgOrderMax(bSLCMode);
}

#endif//BOOTLOADER

/*  end of this file  */
