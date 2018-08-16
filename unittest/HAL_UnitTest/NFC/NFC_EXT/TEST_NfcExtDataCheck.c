/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
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
Filename    : TEST_NfcExtDataCheck.c
Version     : Ver 1.0
Author      : abby
Description : compile by MCU1 and MCU2
Others      :
Modify      :
    20160903    abby    create
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcExtDataCheck.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR WRITE_DATA_BUFF g_tWrBuf;
GLOBAL MCU12_VAR_ATTR U16 g_aWrBufId[DATA_PATT_NUM][FCMDQ_DEPTH][PHYPG_PER_PRG];
GLOBAL MCU12_VAR_ATTR READ_DATA_BUFF g_tRdBuf;
GLOBAL MCU12_VAR_ATTR WRITE_RED_BUFF g_tWrRed;
GLOBAL MCU12_VAR_ATTR READ_RED_BUFF g_tRdRed;

GLOBAL MCU12_VAR_ATTR volatile U32 *g_pDataCheckBMP;//each TLUN occupy 8 bit(2 PRI * FCMD_DEPTH)

/*------------------------------------------------------------------------------
    EXTERN FUNC DECLARATION
------------------------------------------------------------------------------*/
extern FCMD_REQ_ENTRY *TEST_NfcFCmdQGetEntryAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);

void MCU1_DRAM_TEXT TEST_NfcExtDataDRAMMemMap(U32 *pFreeSharedDRAMBase)
{
    U8 ucIdx, i, j;
    U32 ulFreeBase = *pFreeSharedDRAMBase;
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    /* write RED base  */
    for(ucIdx = 0; ucIdx < PHYPG_PER_PRG; ucIdx++)
    {  
        g_tWrRed.pWrRed[ucIdx] = (U32*)ulFreeBase;
        COM_MemZero(g_tWrRed.pWrRed[ucIdx], LOGIC_RED_SZ >> 2);
        COM_MemIncBaseAddr(&ulFreeBase, LOGIC_RED_SZ);
        COM_MemAddr16DWAlign(&ulFreeBase);
    }
        
    /* read RED base  */
    for(i = 0; i < RD_LUN_MAX; i++)
    {  
        for(j = 0; j < FCMDQ_DEPTH; j++)
        {  
            g_tRdRed.pRdRed[i][j] = (U32 *)ulFreeBase;
            COM_MemZero((U32*)g_tRdRed.pRdRed[i][j], LOGIC_RED_SZ >> 2);
            COM_MemIncBaseAddr(&ulFreeBase, LOGIC_RED_SZ);
            COM_MemAddrPageBoundaryAlign(&ulFreeBase);
        }
    }

    /* write data buffer  */
    for(i = 0; i < DATA_PATT_NUM; i++)
    { 
        for(j = 0; j < FCMDQ_DEPTH; j++)
        {  
            for(ucIdx = 0; ucIdx < PHYPG_PER_PRG; ucIdx++)
            {  
                g_tWrBuf.pWrBuff[i][j][ucIdx] = (U32*)ulFreeBase;
                COM_MemZero(g_tWrBuf.pWrBuff[i][j][ucIdx], BUF_SIZE>>2);
                COM_MemIncBaseAddr(&ulFreeBase, BUF_SIZE);
                COM_MemAddrPageBoundaryAlign(&ulFreeBase);

                g_aWrBufId[i][j][ucIdx] = COM_GetBufferIDByMemAddr((U32)g_tWrBuf.pWrBuff[i][j][ucIdx], TRUE, BUF_SIZE_BITS);
            }
        }
    }

    /* read data buffer  */
    for(i = 0; i < RD_LUN_MAX; i++)
    {  
        for(j = 0; j < FCMDQ_DEPTH; j++)
        {  
            g_tRdBuf.pRdBuff[i][j] = (U32 *)ulFreeBase;
            COM_MemZero((U32*)g_tRdBuf.pRdBuff[i][j], BUF_SIZE>>2);
            COM_MemIncBaseAddr(&ulFreeBase, BUF_SIZE);
            COM_MemAddrPageBoundaryAlign(&ulFreeBase);
        }
    }

    //ASSERT((ulFreeBase - DRAM_DATA_BUFF_MCU1_BASE)<= DATA_BUFF_MCU1_SIZE);

    DBG_Printf("MCU#%d NFC EXT UT Alloc DRAM 0x%x - 0x%x total %dKB, Rsvd %dKB.\n"
        ,HAL_GetMcuId(),*pFreeSharedDRAMBase,ulFreeBase,(ulFreeBase - *pFreeSharedDRAMBase)/1024, (DATA_BUFF_MCU1_SIZE - (ulFreeBase - *pFreeSharedDRAMBase))/1024);

    *pFreeSharedDRAMBase = ulFreeBase;
    
    return;
}

LOCAL U32 TEST_NfcExtGetDummyData(U8 ucTLun, U16 usPhyPage, U8 usSec, U8 ucInterPgCnt, U32 ulDwIndex)
{
    U32 ulData;
    
    switch (DATA_PATTERN_SEL)
    {
        case RELAVANT_FLASH_ADDR_DATA://special prepare every time before push write FCMD
        {
            ulData = DUMMY_DATA_WITH_ADDR(ucTLun, usPhyPage, (usSec + ucInterPgCnt * SEC_PER_LOGIC_PIPE_PG));
            ulData += ulDwIndex;
        }break;

        case RANDOM_DATA:
        {
            taus_state_t state;
            U32 seed = 0;

            COM_SetCTGSeeds(seed,&state);
            COM_TausGet(&state);
            COM_TausGet(&state);
            COM_TausGet(&state);
            COM_TausGet(&state);
            
            ulData = COM_TausGet(&state);
        }break;

        case FIX_AA55:
        {
            ulData = 0x55AA55AA;
        }break;

        case FIX_ALL_ZERO:
        {
            ulData = 0x0;
        }break;

        case FIX_ALL_ONE:
        {
            ulData = 0xFFFFFFFF;
        }break;

        default:
        {
            DBG_Printf("Not support this data pattern!\n");
            DBG_Getch();
        } 
    }
    return ulData;
}

LOCAL void TEST_NfcExtPrepare1PlnData(U8 ucTLun, U16 usPhyPage, U8 ucPln, U8 ucPageCnt, U8 ucInterPgCnt, U8 ucWptr)
{
    U32 ulWrBaseAddr, ulData, ulDwIndex, ulSecInBuf;
    U8  ucSec;

    ulSecInBuf = SEC_PER_PHYPG * ucPln;
    
    ulWrBaseAddr = (U32)(g_tWrBuf.pWrBuff[DATA_PATTERN_SEL][ucWptr][(ucPageCnt + ucInterPgCnt)]) + (ulSecInBuf << SEC_SZ_BITS);
    //DBG_Printf("TLUN %d Page %d PLN%d ucInterPgCnt%d ulSecInBuf%d Prepara data WrAddr:0x%x\n"
    //        ,ucTLun,usPhyPage,ucPln,ucInterPgCnt,ulSecInBuf,ulWrBaseAddr);
    
    for (ucSec = 0; ucSec < SEC_PER_PHYPG; ucSec++)
    {        
        for (ulDwIndex = 0; ulDwIndex < SEC_SZ_DW; ulDwIndex++)
        {
            /*  "ucInterPgCnt" to distinc data between low/mid/upp page in one WL for TSB 2D TLC  */
            ulData = TEST_NfcExtGetDummyData(ucTLun, usPhyPage, ucSec, ucInterPgCnt, ulDwIndex);

            *(volatile U32*)(ulWrBaseAddr + (ucSec << SEC_SZ_BITS) + ulDwIndex * 4) = ulData;
        }
    }
}

void TEST_NfcExtPrepareDummyData(U8 ucTLun, U16 usPhyPage, U8 ucWptr)
{
    U8 ucPageCnt;       //page number of different page address in once program
    U8 ucInterPgCnt;    //internal page number of the same page addr
    U8 ucPln;

    for (ucPageCnt = 0; ucPageCnt < PGADDR_PER_PRG; ucPageCnt++)
    {
        for (ucInterPgCnt = 0; ucInterPgCnt < INTRPG_PER_PGADDR; ucInterPgCnt++)
        {
            for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
            {
                TEST_NfcExtPrepare1PlnData(ucTLun, usPhyPage, ucPln, ucPageCnt, ucInterPgCnt, ucWptr);
            }
        }
    #ifdef FLASH_INTEL_3DTLC
        if (LOW_PAGE_WITHOUT_HIGH == HAL_GetFlashPairPageType(usPhyPage))
        {
            break;
        }
        else if (EXTRA_PAGE == HAL_GetFlashPairPageType(usPhyPage))
        {
            usPhyPage = HAL_GetHighPageIndexfromExtra(usPhyPage);
        }
        else
        {
            usPhyPage++;
        }
    #else
        usPhyPage++;
    #endif
    }
}

LOCAL void TEST_NfcExtPrepare1PlnRed(U32 *pRed, U8 ucPln)
{
    U32 ulDwIndex;
    U8  ucLPNIndex;
    SPARE_AREA *pSpare;
    
    pSpare = (SPARE_AREA *)pRed;
    
    for (ulDwIndex = 0; ulDwIndex < RED_SZ_DW; ulDwIndex++)
    {
        *(pRed + ulDwIndex) =  TEST_RED_DATA;
    }
    for(ucLPNIndex = 0; ucLPNIndex < (LPN_PER_BUF>>PLN_PER_LUN_BITS); ucLPNIndex++)
    {
        if (g_bEmEnable)
        {
            pSpare->aCurrLPN[ucLPNIndex] = (ucLPNIndex + (ucPln << 2)) << 3;
        }
        else
        {
    
            pSpare->aCurrLPN[ucLPNIndex] = DUMMY_DATA_WITH_ADDR(0, 0, ucLPNIndex) << 3;
        }
    }
    pSpare->bcPageType = PAGE_TYPE_DATA;
    
#ifdef SIM
    pSpare->ulMCUId = MCU1_ID;
#endif
}

LOCAL void TEST_NfcExtPrepareDummyRED(void)
{
    U8 ucPln, ucPage;
    U32 *pRed = g_tWrRed.pWrRed[0];

    for (ucPage = 0; ucPage < PHYPG_PER_PRG; ucPage++)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            TEST_NfcExtPrepare1PlnRed(pRed, ucPln);
            pRed += RED_SZ_DW;
        }
    }
}

void MCU1_DRAM_TEXT TEST_NfcDummyDataInit(void)
{
    if (RELAVANT_FLASH_ADDR_DATA != DATA_PATTERN_SEL)
    {
        U8 ucWptr, ucDataPatt;
        
        for (ucDataPatt = 0; ucDataPatt < DATA_PATT_NUM; ucDataPatt++)
        {
            for(ucWptr = 0; ucWptr < FCMDQ_DEPTH; ucWptr++)
            {
                TEST_NfcExtPrepareDummyData(0, 0, ucWptr);
            }
        }
    }
    
    TEST_NfcExtPrepareDummyRED();
}


BOOL TEST_NfcIsEnableDataCheck(U8 ucFCmdReqType)
{
    BOOL bChkData = FALSE;
    
#ifdef DATA_CHK
    if (FCMD_REQ_TYPE_READ == ucFCmdReqType)
    {
        bChkData = TRUE;
    }
#endif

    return bChkData;
}

BOOL TEST_NfcFCmdQIsNeedDataCheck(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
#ifndef DATA_CHK
    return FALSE;
#endif

    U8 ucDWPos, ucBitPos;
    BOOL bIsNeedChk = FALSE;
    
    ucDWPos  = ucTLun / 8;
    ucBitPos = (ucTLun * FCMDQ_DEPTH) + (eFCmdPri * FCMD_REQ_PRI_NUM)+ ucWptr;
    if (0 != ((*(g_pDataCheckBMP + ucDWPos))&(1 << ucBitPos)))
    {
        bIsNeedChk = TRUE;
    }
    bIsNeedChk = COM_BitMaskGet(*(U32*)(g_pDataCheckBMP + ucDWPos), ucBitPos);
    
    return bIsNeedChk;
}

void TEST_NfcSetDataCheckBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    U8 ucDWPos, ucBitPos;
    
    ucDWPos  = ucTLun / 8;
    ucBitPos = (ucTLun * FCMDQ_DEPTH) + ucWptr;

    COM_BitMaskSet((U32*)(g_pDataCheckBMP + ucDWPos), ucBitPos);
    
    return;
}

void TEST_NfcFCmdQClrDataCheckBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    U8 ucDWPos, ucBitPos;
    
    ucDWPos  = ucTLun / 8;
    ucBitPos = (ucTLun * FCMDQ_DEPTH) + ucWptr;
    
    COM_BitMaskClear((U32*)(g_pDataCheckBMP + ucDWPos), ucBitPos);
    
    return;
}

LOCAL U32 TEST_NfcExtCheck1SecData(U8 ucTLun, U8 ucSec, U8 ucInterPgCnt, U16 usPhyPage, U32 ulBaseAddr)
{
    U32 ulWrData, ulRdData, ulDwIndex, ulDwNum;
    U32 ulRDramAddr;
    U32 ulDwErrCount = 0;

    #ifdef DATA_CHECK_2DW 
        ulDwNum = 2;
    #else
        ulDwNum = SEC_SZ_DW;
    #endif
    
    ulBaseAddr += ucSec << SEC_SZ_BITS;
    ucSec = ucSec % SEC_PER_PHYPG;//for merge read cross pln
    
    for (ulDwIndex = 0; ulDwIndex < ulDwNum; ulDwIndex++)
    {
        ulWrData = TEST_NfcExtGetDummyData(ucTLun, usPhyPage, ucSec, ucInterPgCnt, ulDwIndex);
        ulRDramAddr = ulBaseAddr + ulDwIndex * 4;
        ulRdData = *(volatile U32*)ulRDramAddr;

        if( ulWrData != ulRdData )
        {
        #if 1
            DBG_Printf("TLUN%d Page %d Data Miss-compare!\n",ucTLun, usPhyPage);
            DBG_Printf("Rdaddr:0x%x ulsec:0x%x Rdata:0x%x Wdata:0x%x\n",ulRDramAddr,ucSec,ulRdData,ulWrData);
        #endif
            ulDwErrCount++;
            DBG_Getch();
        }
        else
        {
            *(U32*)ulRDramAddr = 0x0;
        }
    }
        
    return ulDwErrCount;
}

LOCAL void TEST_NfcExtCheck1PlnData(U8 ucPln, U8 ucTLun, U8 ucWptr, FCMD_FLASH_DESC *pFlashDes, FCMD_BUF_DESC *pBufDes, U8 ucInterPgCnt)
{
    U32 ulRDramAddr, ulRBaseAddr, ulSecInBuf;
    U8  ucSec;
    U32 ulDwErrCount = 0;
    BOOL bsErr = FALSE;
    BOOL bSLCMode = TEST_NfcIsSLCMode(pFlashDes);
    U16 usPhyPage = TEST_NfcGetReadPhyPageFromVirPage(pFlashDes->bsVirPage, bSLCMode);
    
    ulSecInBuf = SEC_PER_PHYPG * ucPln;
    ulRBaseAddr = (U32)g_tRdBuf.pRdBuff[ucTLun][ucWptr];
    ulRDramAddr = ulRBaseAddr + (ulSecInBuf << SEC_SZ_BITS);//(HAL_NfcGetDmaAddr(pBufDes->bsBufID, ulSecInBuf, BUF_SIZE_BITS)<< 1) + TEST_START_ADDRESS;
    //DBG_Printf("Check data:RDramBaseAddr0x%x ulSecInBuf%d ucInterPgCnt%d\n",ulRDramAddr,ulSecInBuf,ucInterPgCnt);

    if (FALSE == pFlashDes->bsMergeRdEn)
    {
        for (ucSec = pBufDes->bsSecStart; ucSec <= pBufDes->bsSecLen; ucSec++)
        {
            ulDwErrCount = TEST_NfcExtCheck1SecData(ucTLun, ucSec, ucInterPgCnt, usPhyPage, ulRDramAddr);
            if (0 != ulDwErrCount)
                bsErr = TRUE;
        }
    }
    else
    {
        U8 ucLpnBitMap = pFlashDes->bsLpnBitmap;
        U8 ucBitPos = 0;
        
        while (ucBitPos < 8)
        {
            if (ucLpnBitMap & (1<<ucBitPos))
            {
                ucSec = pBufDes->bsSecStart + ucBitPos;
                ulDwErrCount = TEST_NfcExtCheck1SecData(ucTLun, ucSec, ucInterPgCnt, usPhyPage, ulRDramAddr);
                if (0 != ulDwErrCount)
                    bsErr = TRUE;
            }
            ucBitPos++;
        }
    }
    
    if (bsErr)
    {
        DBG_Printf("TLUN %d Block %d Page %d Error DW Count %d\n",ucTLun,pFlashDes->bsPhyBlk,usPhyPage,ulDwErrCount);
        DBG_Getch();
    }
    return;
}

void TEST_NfcExtCheck1BuffData(FCMD_REQ_ENTRY *pFCMDReq, FCMD_BUF_DESC *pBufDes)
{
    FCMD_FLASH_DESC tFlashDes = {0};
    FCMD_BUF_DESC tChkBufDes = {0};
    U8 ucSecStart, ucSecLen, ucWptr;
    U8 ucPlnStart, ucPlnEnd, ucTLun, ucInterPgCnt, ucPlnIdx;
    BOOL bSLCMode = FALSE;

    tFlashDes = pFCMDReq->tFlashDesc;
    tChkBufDes.bsBufID = pBufDes->bsBufID;
    bSLCMode = TEST_NfcIsSLCMode(&tFlashDes);

    ucWptr = pFCMDReq->bsReqPtr;
    ucTLun = pFCMDReq->bsTLun;
    ucSecStart = pBufDes->bsSecStart;
    ucSecLen   = pBufDes->bsSecLen;
    ucPlnStart = ucSecStart / SEC_PER_PHYPG;
    if (tFlashDes.bsMergeRdEn)
    {
        ucPlnEnd = ucPlnStart;
    }
    else
    {
        ucPlnEnd = (ucSecStart + ucSecLen - 1) / SEC_PER_PHYPG;
    }

#if (defined(FLASH_TLC) &&!defined(FLASH_TSB_3D))//TSB 2d TLC
    ucInterPgCnt = bSLCMode ? (0) : (tFlashDes.bsVirPage%PG_PER_WL);
#else
    ucInterPgCnt = 0;   
#endif
    
    for (ucPlnIdx = ucPlnStart; ucPlnIdx <= ucPlnEnd; ucPlnIdx++)
    {
        tChkBufDes.bsSecStart = 0;
        tChkBufDes.bsSecLen = SEC_PER_PHYPG - 1;
        
        if (ucPlnStart == ucPlnIdx)
        {
            tChkBufDes.bsSecStart = ucSecStart % SEC_PER_PHYPG;  // SecStartInPlnStart
        }
        if (ucPlnEnd == ucPlnIdx)
        {
            tChkBufDes.bsSecLen = (ucSecStart + ucSecLen - 1) % SEC_PER_PHYPG;
        }
        //DBG_Printf("Page%d ucInterPgCnt%d ucPlnStart%d ucPlnEnd%d\n",pFlashAddr->usPage,ucInterPgCnt,ucPlnStart,ucPlnEnd);
        TEST_NfcExtCheck1PlnData(ucPlnIdx, ucTLun, ucWptr, &tFlashDes, &tChkBufDes, ucInterPgCnt);
    }
}

LOCAL void TEST_NfcExtCheck1PlnRed(U32 *pRed, U16 usSecStart, U16 usSecLen, U8 ucPageType, U8 ucPln)
{
    U32 ulDwIndex, ulLpn;
    U8  ucLPNIndex;
    volatile SPARE_AREA *pSpare;

    pSpare = (volatile SPARE_AREA *)pRed;
    
    //DBG_Printf("*pRed Addr 0x%x usSecStart%d usSecLen%d\n", (U32)pRed, usSecStart, usSecLen);
#ifdef SIM
    if (MCU1_ID != pSpare->ulMCUId)
    {
        DBG_Printf("MCUId %d is wrong, should be %d\n",pSpare->ulMCUId, MCU1_ID);
        DBG_Getch();
    }
    else
    {
        pSpare->ulMCUId = TEST_RED_DATA;
    }
#endif

    if (PAGE_TYPE_DATA != pSpare->bcPageType)
    {
        DBG_Printf("RED PageType %d is wrong, should be %d\n",pSpare->bcPageType, PAGE_TYPE_DATA);
        //DBG_Getch();
    }
    else
    {
        pSpare->bcPageType = 0x55;
    }

    for(ucLPNIndex = 0; ucLPNIndex < (LPN_PER_BUF>>PLN_PER_LUN_BITS); ucLPNIndex++)
    {
        if (TRUE == g_bEmEnable)
        {
            ulLpn = (ucLPNIndex + (ucPln << 2)) << 3;
        }
        else
        {
            ulLpn = DUMMY_DATA_WITH_ADDR(0, 0, ucLPNIndex) << 3;
        }
        if( ulLpn != pSpare->aCurrLPN[ucLPNIndex] )
        {
            DBG_Printf("aCurrLPN[%d] = 0x%x is wrong, should be 0x%x\n"
            ,ucLPNIndex,pSpare->aCurrLPN[ucLPNIndex],ulLpn);
            DBG_Getch();
        }
        else
        {
            pSpare->aCurrLPN[ucLPNIndex] = TEST_RED_DATA;
        }
    }
    
    /*    check RED dummy data    */
    for (ulDwIndex = 0; ulDwIndex < RED_SZ_DW; ulDwIndex++)
    {
        if (TEST_RED_DATA != *(volatile U32*)(pRed + ulDwIndex))
        {
            DBG_Printf("pRed->aContent[%d] = 0x%x is wrong, should be 0x%x, *pRed Addr 0x%x\n",ulDwIndex,*(pRed + ulDwIndex),TEST_RED_DATA,(U32)pRed);
            DBG_Getch();
        }
        else
        {
            *(U32*)(pRed + ulDwIndex) = 0;
        }
    }
   
    return;
}

void TEST_NfcExtCheck1BuffRED(FCMD_REQ_ENTRY *pFCMDReq, FCMD_BUF_DESC *pBufDes)
{
    U8 ucSecStart, ucSecLen;
    U8 ucPlnStart, ucPlnEnd, ucPlnLen, ucPlnIndex, ucInterPgCnt;
    U32 *pRdRed;
    
    ucSecStart = pBufDes->bsSecStart;
    ucSecLen   = pBufDes->bsSecLen;

    ucPlnStart  = ucSecStart / SEC_PER_PHYPG;
    ucPlnEnd    = ((ucSecStart + ucSecLen)/SEC_PER_PHYPG) - 1;//(ucSecStart + ucSecLen - 1) / SEC_PER_PHYPG;
    ucPlnLen    = ucSecLen / SEC_PER_PHYPG;

    pRdRed = (U32*)pFCMDReq->ulSpareAddr;
    
#if (defined(FLASH_TLC) &&!defined(FLASH_TSB_3D))//TSB 2d TLC
    BOOL bSLCMode = FALSE;
    FCMD_FLASH_DESC tFlashDes = {0};

    tFlashDes = pFCMDReq->tFlashDesc;
    bSLCMode = TEST_NfcIsSLCMode(&tFlashDes);
    ucInterPgCnt = bSLCMode ? (0) : (pFCMDReq->tFlashDesc.bsVirPage%PG_PER_WL);
#else
    ucInterPgCnt = 0;   
#endif
    
    if (0 == ucPlnLen) //no RED be read
    {
        return;
    }
    pRdRed += ucPlnStart * RED_SZ_DW;
    
    for (ucPlnIndex = ucPlnStart; ucPlnIndex <= ucPlnEnd; ucPlnIndex++)
    {
        TEST_NfcExtCheck1PlnRed(pRdRed, ucSecStart, ucSecLen, ucInterPgCnt, ucPlnIndex);
        pRdRed += RED_SZ_DW;
        //DBG_Printf("ucPlnIndex%d\n",ucPlnIndex);
    }
}

U32 TEST_NfcExtCheckData(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    U32 ulBuffID;
    U8 ucIndex;
    volatile U8 *pStatus;
    FCMD_REQ_ENTRY *pFCMDReq;
    FCMD_BUF_DESC tBufDesc = {0};

    /* if no error, then check data */
    pFCMDReq = TEST_NfcFCmdQGetEntryAddr(ucTLun, eFCmdPri, ucLevel);
    
    /*  check error first  */
    pStatus = (volatile U8*)pFCMDReq->ulReqStsAddr;
    if (SUBSYSTEM_STATUS_SUCCESS != *pStatus)
    {
        DBG_Printf("TLUN%d Wptr%d Read Fail!\n", ucTLun, ucLevel);
        //DBG_Getch();
    }
    
    for (ucIndex = 0; ucIndex < DSG_BUFF_SIZE; ucIndex++)
    {
        tBufDesc = pFCMDReq->atBufDesc[ucIndex];
        ulBuffID = tBufDesc.bsBufID;
        if (INVALID_4F == ulBuffID)//not use this buffer
        {
            break;
        }
        TEST_NfcExtCheck1BuffData(pFCMDReq, &tBufDesc);
        TEST_NfcExtCheck1BuffRED(pFCMDReq, &tBufDesc);
    } 
    return SUCCESS;
}

//detect if any FCMDQ need data check, handle data check and clr bitmap if it is
BOOL TEST_NfcIsRemainDataCheckFCMDQ(void)
{
    FCMD_REQ_PRI eFCmdPri;
    U8 ucTLun, ucWptr;
    BOOL bNeedChk = FALSE;
    
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (eFCmdPri = 0; eFCmdPri < FCMD_REQ_PRI_NUM; eFCmdPri++)
        {
            for (ucWptr = 0; ucWptr < FCMDQ_DEPTH; ucWptr++)
            {
                bNeedChk = TEST_NfcFCmdQIsNeedDataCheck(ucTLun, eFCmdPri, ucWptr);
                if (bNeedChk)
                {
                    /* wait FCMD excute done, then check data */
                    while(FALSE == TEST_NfcFCmdQIsWptrFree(ucTLun, eFCmdPri, ucWptr))
                    {
                        ;
                    }
                    TEST_NfcExtCheckData(ucTLun, eFCmdPri, ucWptr);
                    TEST_NfcFCmdQClrDataCheckBitmap(ucTLun, eFCmdPri, ucWptr);
                }
            }
        }
    }
    
    return FALSE;
}

void TEST_NfcAdaptReadBuffAddr(U8 ucTLun, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode)
{
    U32 *pTargetRed;
    U16 aBuffID[DSG_BUFF_SIZE];
    FCMD_FLASH_DESC tFlashDes = ptReqEntry->tFlashDesc;

    //data buff addr
    aBuffID[0] = COM_GetBufferIDByMemAddr((U32)(g_tRdBuf.pRdBuff[ucTLun][ucWptr]),TRUE,BUF_SIZE_BITS);//(ulBufferAddr - ulDramStartAddr) / BUF_SIZE;
    ptReqEntry->atBufDesc[0].bsBufID = aBuffID[0];
    ptReqEntry->atBufDesc[1].bsBufID = INVALID_4F;

    ptReqEntry->atBufDesc[0].bsSecStart = tFlashDes.bsSecStart;
    ptReqEntry->atBufDesc[0].bsSecLen = tFlashDes.bsSecLen;
    
    //RED addr
    pTargetRed = (U32*)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ucWptr);
#ifdef WITHOUT_RED
    ptReqEntry->ulSpareAddr = NULL;
#else
    ptReqEntry->ulSpareAddr = (U32)pTargetRed;
#endif
    g_tRdRed.pRdRed[ucTLun][ucWptr] = pTargetRed;
}


/*  end of this file  */

