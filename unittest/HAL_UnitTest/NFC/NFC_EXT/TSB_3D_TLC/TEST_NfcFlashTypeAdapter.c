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

//TSB 3D TLC
void MCU1_DRAM_TEXT TEST_NfcTLCPrgTableInit(void)
{
    U16 usLogicPage;
    
    for (usLogicPage = 0; usLogicPage < LOGIC_PG_PER_BLK; usLogicPage++)
    {
        g_pTLCPrgTable->aTlcPrgTab[usLogicPage].bsPhyPage = usLogicPage / PG_PER_WL;
    }
    
    return;
}

/* g_pTLCP2LPageMap only for program page mapping */
void MCU1_DRAM_TEXT TEST_NfcTLCP2LMapInit(void)
{
    U16 usPhyPage;
  
    for (usPhyPage = 0; usPhyPage < PG_PER_BLK; usPhyPage++)
    {
        g_pTLCP2LPageMap->aLogicPage[usPhyPage] = usPhyPage * PG_PER_WL;
    }
    
    return;
}


/****************************************************************************
Function  : TEST_NfcGetPrgVirPageFromOrder
Input     :
Output    :

Purpose   : program page mapping
Reference : 
            SLC mode: Program order = PhyPage = VirPage   (0~191)
            TLC mode: Program order = PhyPage = VirPage/3 
****************************************************************************/
U16 MCU1_DRAM_TEXT TEST_NfcGetPrgVirPageFromOrder(U16 usPrgOrder, BOOL bSLCMode)
{
    if (bSLCMode)
    {
        return usPrgOrder;
    }
    else
    {
        return g_pTLCP2LPageMap->aLogicPage[usPrgOrder];
    }
}

U16 TEST_NfcGetPrgPhyPageFromVirPage(U16 usVirPage, BOOL bSLCMode)
{
    if (bSLCMode)
    {
        return usVirPage;
    }
    else
    {
        return g_pTLCPrgTable->aTlcPrgTab[usVirPage].bsPhyPage;
    }
}

/****************************************************************************
Function  : TEST_NfcGetReadVirPageFromOrder
Input     :
Output    :

Purpose   : read page mapping
Reference : 
            SLC mode: Program order = PhyPage = VirPage   (0~191)
            TLC mode: Program order = VirPage = PhyPage*3 + PageType
****************************************************************************/
U16 TEST_NfcGetReadVirPageFromOrder(U16 usPrgOrder, BOOL bSLCMode)
{
    return usPrgOrder;
}

U16 TEST_NfcGetReadPhyPageFromVirPage(U16 usVirPage, BOOL bSLCMode)
{
    return TEST_NfcGetPrgPhyPageFromVirPage(usVirPage, bSLCMode);
}

U16 TEST_NfcGetPrgVirPageFromPhyPage(U16 usPhyPage, U8 ucPrgCycle, BOOL bSLCMode)
{
    if (bSLCMode)
    {
        return usPhyPage;
    }
    else
    {
        return g_pTLCP2LPageMap->aLogicPage[usPhyPage];//usPhyPage*PG_PER_WL;
    }
}

void TEST_NfcAdaptWriteBuffAddr(U8 ucTLun, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode)
{
    U32 *pTargetRed;
    U16 aBuffID[DSG_BUFF_SIZE];
    U8  ucPrgPageNum, ucBufIdx;
    
    ucPrgPageNum = bSLCMode ? 1 : PG_PER_WL;
    
    //data buff addr
    for (ucBufIdx = 0; ucBufIdx < ucPrgPageNum; ucBufIdx++)
    {
        aBuffID[ucBufIdx] = g_aWrBufId[DATA_PATTERN_SEL][ucWptr][ucBufIdx];//COM_GetBufferIDByMemAddr((U32)(g_tWrBuf.pWrBuff[DATA_PATTERN_SEL][ucWptr][ucBufIdx]),TRUE,BUF_SIZE_BITS);//(ulBufferAddr - ulDramStartAddr) / BUF_SIZE;
        ptReqEntry->atBufDesc[ucBufIdx].bsBufID = aBuffID[ucBufIdx];
    }
    if (ucBufIdx < 3)//not all DSG buff be used, then invalid last buff
    {
        ptReqEntry->atBufDesc[ucBufIdx].bsBufID = INVALID_4F;
    }    
    
    //RED addr
    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ucWptr);
    HAL_DMAECopyOneBlock((const U32)pTargetRed, (const U32)g_tWrRed.pWrRed[0], (const U32)(LOGIC_RED_SZ *ucPrgPageNum));
#ifdef WITHOUT_RED
    ptReqEntry->ulSpareAddr = NULL;
#else
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;
#endif
    
#ifdef DATA_CHK
    if (RELAVANT_FLASH_ADDR_DATA == DATA_PATTERN_SEL)
    {
        /* prepare data by flash address for every time of program */
        U32 usPhyPage = TEST_NfcGetPrgPhyPageFromVirPage(ptReqEntry->tFlashDesc.bsVirPage, bSLCMode);
        TEST_NfcExtPrepareDummyData(ucTLun, usPhyPage, ucWptr);
    }
#endif
}

void TEST_NfcFCmdQAdaptWriteSecRange(FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode)
{
    FCMD_FLASH_DESC *ptFlashDesc = &ptReqEntry->tFlashDesc;
    U8 ucPrgPageNum, ucBufIdx;

    /* slc mode or low page without high : program 32K */
    ucPrgPageNum = bSLCMode ? 1 : PG_PER_WL;

    ptFlashDesc->bsSecStart = 0;
    ptFlashDesc->bsSecLen = SEC_PER_BUF * ucPrgPageNum;

    for (ucBufIdx = 0; ucBufIdx < ucPrgPageNum; ucBufIdx++)
    {
        ptReqEntry->atBufDesc[ucBufIdx].bsSecStart = 0;
        ptReqEntry->atBufDesc[ucBufIdx].bsSecLen = SEC_PER_BUF;
    }
    
    return;
}

U16 TEST_NfcGetNextPPO(U16 usCurrPPO, U8 ucPrgCycle, BOOL bSLCMode)
{
    return (usCurrPPO + 1);
}

U16 TEST_NfcGetPrgOrderMax(BOOL bSLCMode)
{
    return PG_PER_BLK;
}

U16 TEST_NfcGetReadOrderMax(BOOL bSLCMode)
{
    return (bSLCMode? PG_PER_BLK : LOGIC_PG_PER_BLK);
}

U16 TEST_NfcGetPhyPageMax(BOOL bSLCMode)
{
    return PG_PER_BLK;
}

#endif//BOOTLOADER

/*  end of this file  */
