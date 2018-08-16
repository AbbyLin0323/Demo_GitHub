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
Filename    : FT_BasicFunc.c
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : Define the basic function interfaces. 
              These functions can't be run independently but can be grouped in multi-ways.
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "HAL_Xtensa.h"
#include "COM_Memory.h"
#include "HAL_FlashDriverBasic.h"
#include "FT_Config.h"
#include "HAL_FlashDriverExt.h"
#include "FT_BasicFunc.h"
#include "FT_FlashTypeAdapter.h"
#include "HAL_DecStsReport.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
/* Test Mode Related */
GLOBAL BOOL g_bFTMlcMode;       //MLC mode
GLOBAL BOOL g_bFTRawDataRead;
GLOBAL BOOL g_bFTSnapRead;      //snap read in micron or partial read in YMTC
GLOBAL BOOL g_bFTDecFifoEn;
GLOBAL volatile U8 g_ucFTDecFifoRp;
GLOBAL BOOL g_bFTSinglePgProg;  //YMTC MLC single page programming mode

/* Data Buffer Related */
GLOBAL FT_DATA_BUFF g_tFTWrBuf, g_tFTRdBuf; 
GLOBAL NFC_RED *g_pFTWrRed; 
GLOBAL NFC_RED **g_ppFTRdRed;
#ifdef OTP_TEST
GLOBAL U16 g_aFTWrBufId[PG_PER_WL], g_aFTRdBufId[SUBSYSTEM_LUN_MAX*2]; //used for accelation
GLOBAL U16 g_aFT1PlnWrBufId[PG_PER_WL], g_aFT1PlnRdBufId[SUBSYSTEM_LUN_MAX*2]; 
#else
GLOBAL U16 g_aFTWrBufId[PG_PER_WL], g_aFTRdBufId[PG_PER_WL]; //used for accelation
GLOBAL U16 g_aFT1PlnWrBufId[PG_PER_WL], g_aFT1PlnRdBufId[PG_PER_WL]; 
#endif

GLOBAL FT_PARAM g_tFTParam;
GLOBAL BOOL g_bFTGetParamExternal;  //1-input param by external scripts
                                    //0-set param in test code

/* Timer */
GLOBAL U32 g_ulFTStartTimer, g_ulFTEndTimer, g_ulFTDiffCycle;

/* N0 Cnt */
GLOBAL U32 g_ulFTN0Cnt = 0;
/*------------------------------------------------------------------------------
    EXTERN DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL volatile BOOL g_ErrInjEn;
extern GLOBAL NFC_ERR_INJ g_tErrInj;

/*------------------------------------------------------------------------------
    EXTERN FUNCTIONS
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    FUNCTION DEFINITION
------------------------------------------------------------------------------*/
LOCAL void FT_StartTimer(void)
{
    g_ulFTStartTimer = HAL_GetMCUCycleCount();
}

LOCAL U32 FT_EndTimer(void)
{
    g_ulFTEndTimer = HAL_GetMCUCycleCount();
    g_ulFTDiffCycle = COM_DiffU32(g_ulFTStartTimer, g_ulFTEndTimer);
    U32 ulDiffUs = g_ulFTDiffCycle / (HAL_GetMcuClock() / 1000000);

    return ulDiffUs;
}

/* offset = PU * level + level */
LOCAL U32 FT_GetOffset(FLASH_ADDR *pFlashAddr)
{
    return ((pFlashAddr->ucPU << NFCQ_DEPTH_BIT) | HAL_NfcGetRP(pFlashAddr->ucPU, pFlashAddr->ucLun));
}

LOCAL U8 FT_RdGetDecFifoCmdInx(void)
{
    U8 ucCmdIndex = 0;
    if (g_bFTDecFifoEn)
    {
        ucCmdIndex = g_ucFTDecFifoRp;
    }

    return ucCmdIndex;
}

void FT_DataDRAMMemMap(void)
{
    U8  i;
    U32 ulFreeBase = g_FreeMemBase.ulDRAMBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_pFTWrRed = (NFC_RED*)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, RED_SW_SZ_DW<<2);
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_ppFTRdRed = (NFC_RED**)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, RED_SW_SZ_DW<<2);
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    *g_ppFTRdRed = (NFC_RED*)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, RED_SW_SZ_DW<<2);
    COM_MemAddrPageBoundaryAlign(&ulFreeBase);

    //DBG_Printf("g_pFTWrRed 0x%x g_ppFTRdRed 0x%x *g_ppFTRdRed 0x%x\n",(U32)g_pFTWrRed, (U32)g_ppFTRdRed,(U32)(*g_ppFTRdRed));    

    for(i = 0; i < PG_PER_WL; i++)
    {
        /* write data buffer  */
        g_tFTWrBuf.pBuffAddr[i] = (U32*)ulFreeBase;
        COM_MemZero((U32*)g_tFTWrBuf.pBuffAddr[i], LOGIC_PIPE_PG_SZ>>2);
        COM_MemIncBaseAddr(&ulFreeBase, LOGIC_PIPE_PG_SZ);
        COM_MemAddrPageBoundaryAlign(&ulFreeBase);
        //DBG_Printf("g_tFTWrBuf.pBuffAddr[%d] 0x%x\n",i,(U32)g_tFTWrBuf.pBuffAddr[i]);

        /* write data buffer ID  */
        g_aFTWrBufId[i] = ((U32)g_tFTWrBuf.pBuffAddr[i] - DRAM_START_ADDRESS) >> LOGIC_PIPE_PG_SZ_BITS;
        //DBG_Printf("g_aFTWrBufId[%d] 0x%x\n",i,(U32)g_aFTWrBufId[i]);
    }

#ifdef OTP_TEST
    for(i = 0; i < SUBSYSTEM_LUN_MAX*2; i++)
#else
    for(i = 0; i < PG_PER_WL; i++)
#endif
    {
        /* read data buffer  */
        g_tFTRdBuf.pBuffAddr[i] = (U32 *)ulFreeBase;
        COM_MemZero((U32*)g_tFTRdBuf.pBuffAddr[i], LOGIC_PIPE_PG_SZ >> 2);
        COM_MemIncBaseAddr(&ulFreeBase, LOGIC_PIPE_PG_SZ);
        COM_MemAddrPageBoundaryAlign(&ulFreeBase);
        DBG_Printf("g_tFTRdBuf.pBuffAddr[%d] 0x%x\n",i,(U32)g_tFTRdBuf.pBuffAddr[i]);

        /* read data buffer ID  */
        g_aFTRdBufId[i] = ((U32)g_tFTRdBuf.pBuffAddr[i] - DRAM_START_ADDRESS) >> LOGIC_PIPE_PG_SZ_BITS;
        g_aFT1PlnRdBufId[i] = ((U32)g_tFTRdBuf.pBuffAddr[i] - DRAM_START_ADDRESS) >> LOGIC_PG_SZ_BITS;
        //DBG_Printf("g_aFTRdBufId[%d] 0x%x\n",i,(U32)g_aFTRdBufId[i]);
    }

    g_FreeMemBase.ulDRAMBase = ulFreeBase;

    return;
}

LOCAL U32 FT_GetDummyData(U16 usPage, U16 usBlk, U8 ucPln, U32 ulDwIndex)
{
    U32 ulData;
    
    switch (FT_DATA_PATT_SEL)
    {
        case INC_DATA:
        {
            ulData = ulDwIndex;
        }break;

        case ALL_55AA55AA:
        {
            ulData = 0x55AA55AA;
        }break;

        case ALL_ZERO:
        {
            ulData = 0x0;
        }break;

        case ALL_ONE:
        {
            ulData = 0xFFFFFFFF;
        }break;

        case CELL_TYPE:
        {
            //pending
            ulData = ulDwIndex | (ucPln << 8) | (usBlk << 16)|(usPage << 24);//usPage % PG_PER_WL + ulDwIndex;
        }break;

        default:
        {
            DBG_Printf("Not support this data pattern!\n");
            DBG_Getch();
        } 
    }
    return ulData;
}

LOCAL void FT_Prepare1PlnMsg(FLASH_ADDR *pFlashAddr, U8 ucPlnBufIdx, U8 ucPageCnt, U8 ucInterPgCnt)
{
    U32 ulWrBaseAddr, ulData, ulDwIndex, ulSecInBuf, ulSecPerPg;
    U8  ucSec, ucPageType = 0, ucPln = pFlashAddr->bsPln;
    U16 usBlk = pFlashAddr->usBlock, usPhyPage = pFlashAddr->usPage;

    ulSecPerPg = SEC_PER_PHYPG;
    ulSecInBuf = (SEC_PER_PHYPG * ucPlnBufIdx) + ((ucPageCnt + ucInterPgCnt)* ulSecPerPg);
    
    ulWrBaseAddr = (g_aFTWrBufId[0] << LOGIC_PG_SZ_BITS) + (ulSecInBuf << SEC_SZ_BITS) + DRAM_START_ADDRESS;

    if (0 == usPhyPage)
        ;//DBG_Printf("Prepare data PLn%d ucPageType %d ulWrBaseAddr 0x%x\n", ucPln, ucPageType, ulWrBaseAddr);

    for (ucSec = 0; ucSec < SEC_PER_PHYPG; ucSec++)
    {        
        for (ulDwIndex = 0; ulDwIndex < DW_SZ_PER_SEC; ulDwIndex++)
        {
            ulData = FT_GetDummyData(usPhyPage+ucInterPgCnt, usBlk, ucPln, ulDwIndex);
            if (1 == usPhyPage && 0 == ulDwIndex && 0 == ucSec)
            {
                DBG_Printf("Prepare data 0x%x ulWrBaseAddr 0x%x\n", ulData, ulWrBaseAddr);
            }
        #ifdef OTP_TEST
            if (1 == usPhyPage && 0 == ulDwIndex && 0 == ucSec)
            {
                ulData = 0;
                DBG_Printf("Prepare data 0 usPhyPage %d ulDwIndex %d ulWrBaseAddr 0x%x\n", usPhyPage, ulDwIndex, ulWrBaseAddr);
            }
        #endif

            *(volatile U32*)(ulWrBaseAddr + (ucSec << SEC_SZ_BITS) + ulDwIndex * 4) = ulData;
        }
    }
}

/****************************************************************************
Function      : FT_PrepareDummyMsg
Input         :
Output        :
Description   : prepare msg data for 1 page;
History       :
    20170628    abby    create
****************************************************************************/
void FT_PrepareDummyMsg(FLASH_ADDR *pFlashAddr)
{
    U8 ucInterPgCnt, ucPageCnt, ucPlnBufIdx;
    FLASH_ADDR tFlashAddr = *pFlashAddr;

    for (ucPageCnt = 0; ucPageCnt < PGADDR_PER_PRG; ucPageCnt++)
    {
        for (ucInterPgCnt = 0; ucInterPgCnt < INTRPG_PER_PGADDR; ucInterPgCnt++)
        {
            for (tFlashAddr.bsPln = 0; tFlashAddr.bsPln < PLN_PER_LUN; tFlashAddr.bsPln++)
            {
                FT_Prepare1PlnMsg(&tFlashAddr, tFlashAddr.bsPln, ucPageCnt, ucInterPgCnt);
            }
        }
        tFlashAddr.usPage++;
    }
}

LOCAL void FT_Prepare1PlnRed(U32 *pRed)
{
    U32 ulDwIndex;
    U8  ucLPNIndex;
    FT_SPARE_AREA *pSpare;
    
    pSpare = (FT_SPARE_AREA *)pRed;
    //DBG_Printf("Prepare RED addr0x%x\n",(U32)pSpare);
    
    for (ulDwIndex = 0; ulDwIndex < RED_SZ_DW; ulDwIndex++)
    {
        *(pRed + ulDwIndex) =  FT_DUMMY_RED_DATA;
    }
    for(ucLPNIndex = 0; ucLPNIndex < (LPN_PER_BUF>>PLN_PER_LUN_BITS); ucLPNIndex++)
    {
        pSpare->aCurrLPN[ucLPNIndex] = ucLPNIndex << 3;
    }
    pSpare->bcPageType = PAGE_TYPE_DATA;
    
#ifdef SIM
    pSpare->ulMCUId = MCU1_ID;
#endif
}

/****************************************************************************
Function      : FT_PrepareDummyRED
Input         :
Output        :
Description   : prepare RED dummy data for 1 page;
History       :
    20170628    abby    create
****************************************************************************/
LOCAL void FT_PrepareDummyRED(void)
{
    U8  ucPln, ucPgIdx, ucPgNum;
    U32 *pRed = (U32*)g_pFTWrRed;;
    
    ucPgNum = INTRPG_PER_PGADDR * PGADDR_PER_PRG;//whatever LP only or 2 shared pages, prepare all red for simplicity

    for (ucPgIdx = 0; ucPgIdx < ucPgNum; ucPgIdx++)
    {    
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            FT_Prepare1PlnRed(pRed);
            pRed += RED_SZ_DW;
        }
    }
}

/****************************************************************************
Function      : FT_PrepareOnePageDummyData
Input         :
Output        :
Description   : prepare dummy msg and red data for 1 page;
                called before each programming
History       :
    20170628    abby    create
****************************************************************************/
void FT_PrepareOnePageDummyData(FLASH_ADDR *pFlashAddr)
{
#ifndef FT_DATA_CHK
    return;
#endif
    
    FT_PrepareDummyMsg(pFlashAddr);
    FT_PrepareDummyRED();
}

LOCAL void FT_WtCfgReqComm(NFC_PRG_REQ_DES *pWrReq, FLASH_ADDR *pFlashAddr)
{
    pWrReq->bsWrBuffId      = g_aFTWrBufId[0];
    pWrReq->pNfcRed         = NULL;//(NFC_RED *)g_pFTWrRed;
    
    /* Default setting is for simplicity, but you can change it if you need */
    pWrReq->bsRedOntf       = TRUE;
    pWrReq->bsCSEn          = FALSE;
    pWrReq->bsSsu0En        = FALSE;
    pWrReq->bsSsu0Ontf      = TRUE;
    pWrReq->bsSsu1En        = FALSE;
    pWrReq->bsSsu1Ontf      = TRUE;
    pWrReq->bsLbaChkEn      = FALSE;
    pWrReq->bsTlcMode       = g_bFTMlcMode;
    pWrReq->bsEmEn          = FALSE;
    pWrReq->bsSinglePgProg  = g_bFTSinglePgProg;

    if (pWrReq->bsTlcMode)
    {
        pWrReq->bsTlcPgCycle = HAL_FlashGetTlcPrgCycle(pFlashAddr->usPage);
    }

    if (g_ErrInjEn)
    {
        pWrReq->bsInjErrEn = TRUE;
        pWrReq->pErrInj    = &g_tErrInj;
    }
    else
    {
        pWrReq->pErrInj = NULL;
    }

    /*  scramble disable when raw data read   */
    if (g_bFTRawDataRead)
    {
        volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
        pNfcPgCfg->bsScrBps = TRUE;
    }

    if (pWrReq->bsSsu0En)
    {
        pWrReq->bsSsu0Addr = FT_GetOffset(pFlashAddr);
    }
    if (pWrReq->bsSsu1En)
    {
        pWrReq->bsSsu1Addr = pWrReq->bsSsu0Addr + 0x400;
    }
    if (pWrReq->bsCSEn)
    {
        pWrReq->bsCsAddrOff = pWrReq->bsSsu0Addr;
    }
}

/****************************************************************************
Function      : FT_Check1PlnRed
Input         :
Output        :

Description   : prepare RED dummy data and LPN for 1 plane
History       :
    20170628    abby    create
****************************************************************************/
LOCAL void FT_Check1PlnRed(U32 *pRed, U16 usSecStart, U16 usSecLen)
{
    U32 ulDwIndex, ulLpn;
    U8  ucLPNIndex;
    volatile FT_SPARE_AREA *pSpare = (volatile FT_SPARE_AREA *)pRed;
   
    //DBG_Printf("*pRed Addr 0x%x usSecStart%d usSecLen%d\n", (U32)pRed, usSecStart, usSecLen);
#ifdef SIM
    if (MCU1_ID != pSpare->ulMCUId)
    {
        DBG_Printf("MCUId %d is wrong, should be %d\n",pSpare->ulMCUId, MCU1_ID);
        DBG_Getch();
    }
    else
    {
        pSpare->ulMCUId = FT_DUMMY_RED_DATA;
    }
#endif

    if (PAGE_TYPE_DATA != pSpare->bcPageType)
    {
        DBG_Printf("RED PageType %d is wrong, should be %d\n",pSpare->bcPageType, PAGE_TYPE_DATA);
    }
    else
    {
        pSpare->bcPageType = 0x55;
    }
    
    /*    check LPN, for single pln write, just init 4 LPN for 16KB page;    */
    for(ucLPNIndex = 0; ucLPNIndex < (LPN_PER_BUF>>PLN_PER_LUN_BITS); ucLPNIndex++)
    {
        ulLpn = ucLPNIndex << 3;
        if( ulLpn != pSpare->aCurrLPN[ucLPNIndex] )
        {
            DBG_Printf("aCurrLPN[%d] = 0x%x is wrong, should be 0x%x\n"
            ,ucLPNIndex,pSpare->aCurrLPN[ucLPNIndex],ulLpn);
            DBG_Getch();
        }
        else
        {
            pSpare->aCurrLPN[ucLPNIndex] = FT_DUMMY_RED_DATA;
        }
    }
    
    /*    check RED dummy data    */
    for (ulDwIndex = 0; ulDwIndex < RED_SZ_DW; ulDwIndex++)
    {
        if (FT_DUMMY_RED_DATA != *(volatile U32*)(pRed + ulDwIndex))
        {
            DBG_Printf("pRed->aContent[%d] = 0x%x is wrong, should be 0x%x, *pRed Addr 0x%x\n", ulDwIndex, *(pRed + ulDwIndex), FT_DUMMY_RED_DATA, (U32)pRed);
            DBG_Getch();
        }
        else
        {
            *(U32*)(pRed + ulDwIndex) = 0;
        }
    }
   
    return;
}

/****************************************************************************
Function      : FT_CheckRed
Input         :
Output        :

Description   : check 1 page red data, if it is right, then clr it.
History       :
    20170628    abby    create
****************************************************************************/
LOCAL void FT_CheckRed(FLASH_ADDR *pFlashAddr,NFC_RED **pRed, U16 usSecStart, U16 usSecLen)
{
    U32 ucPlnStart, ucPlnEnd, ucPlnLen, ucPlnIndex;
    U32 *pRdRed = (U32*)(*pRed);

    ucPlnStart  = usSecStart / SEC_PER_PHYPG;
    ucPlnEnd    = (usSecStart + usSecLen - 1) / SEC_PER_PHYPG;
    ucPlnLen    = usSecLen / SEC_PER_PHYPG;
    
    if (0 == ucPlnEnd)//no red will be read
        return;
        
    pRdRed += ucPlnStart * RED_SZ_DW;
    
    for (ucPlnIndex = ucPlnStart; ucPlnIndex <= ucPlnEnd; ucPlnIndex++)
    {
        FT_Check1PlnRed(pRdRed, usSecStart, usSecLen);
        pRdRed += RED_SZ_DW;
    }
    
    return;
}

/****************************************************************************
Function      : FT_Check1PlnData
Input         :
Output        :

Description   : check 1 plane data = 16KB, calculate error cnt by DW unit 
History       :
    20170628    abby    create
****************************************************************************/
LOCAL void FT_Check1PlnData(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U16 usSecEnd, U8 ucPageType)
{
    U8 ucShiftIdx = 0, ucPln = pFlashAddr->bsPln;
    U16 usSec, usBlk = pFlashAddr->usBlock, usPage = pFlashAddr->usPage;
    U32 ulRDramAddr, ulWDramAddr, ulWrData, ulRdData;
    U32 ulSecInBuf, ulDwIndex, ulDwErrCount = 0, ulN0 = 0;
    BOOL bsErr = FALSE;

    ulSecInBuf = SEC_PER_PHYPG * pFlashAddr->bsPln;
    ulSecInBuf = g_bFTRawDataRead ? (ulSecInBuf * 2) : ulSecInBuf;
    
    ulRDramAddr = (g_aFT1PlnRdBufId[ucPageType] << LOGIC_PG_SZ_BITS) + DRAM_START_ADDRESS;

    //DBG_Printf("Check data:RDramBaseAddr0x%x ulSecInBuf%d\n",ulRDramAddr,ulSecInBuf);
    
    for (usSec = ucSecStart; usSec <= usSecEnd; usSec++)
    {
        if ((0 == usSec % 2)&&(usSec != 0) && g_bFTRawDataRead)
        {
            ulRDramAddr = ulRDramAddr + CW_INFO_SZ;
        }
        for (ulDwIndex = 0; ulDwIndex < DW_SZ_PER_SEC; ulDwIndex++)
        {
            ulWrData = FT_GetDummyData(usPage+ucPageType, usBlk, ucPln, ulDwIndex);

            ulRdData = *(volatile U32*)(ulRDramAddr + (usSec << SEC_SZ_BITS) + ulDwIndex * 4);
            if( ulWrData != ulRdData )
            {
            #if 1
                if((0 == usSec)&&(0 == ulDwIndex))
                {
                DBG_Printf("PU %d LUN%d Block %d Page %d Data Miss-compare!\n",pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usBlock,usPage);
                DBG_Printf("Page %d Rdaddr:0x%x ucPln%d ulsec:%d Rdata:0x%x Wdata:0x%x\n",usPage
                ,ulRDramAddr + (usSec<<SEC_SZ_BITS)+ ulDwIndex*4, ucPln, usSec,ulRdData,ulWrData);
                //DBG_Printf("Page %d Rdata:0x%x \n",usPage, ulRdData);
                }
            #endif
                ulDwErrCount++;
                bsErr = TRUE;
                //DBG_Getch();
            }
            else
            {
                if((0 == usSec)&&(0 == ulDwIndex))
                {
                    //DBG_Printf("Page%d ulRdData 0x%x\n", usPage, ulRdData);
                }
                //*(U32*)(ulRDramAddr + (usSec<<SEC_SZ_BITS) + ulDwIndex * 4) = 0x0;
            }
        #ifdef N0_CHECK
            U32 i;
            for (i = 0; i < 32; i++)
            {
                if (0 == (BIT(i) & ulRdData))
                {
                    ulN0++;
                }
            }
            *(U32*)(ulRDramAddr + (usSec<<SEC_SZ_BITS) + ulDwIndex * 4) = 0x0;
        #endif
        }
    }
    g_ulFTN0Cnt += ulN0;

    if (g_bFTSnapRead)//check next sector which is over the read range, should be 0 or wrong data
    {
        if ((0 == usSec % 2)&&(usSec != 0) && g_bFTRawDataRead)
        {
            ulRDramAddr = ulRDramAddr + CW_INFO_SZ;
        }
        for (ulDwIndex = 0; ulDwIndex < DW_SZ_PER_SEC; ulDwIndex++)
        {
            ulWrData = FT_GetDummyData(usPage+ucPageType, usBlk, ucPln, ulDwIndex);

            ulRdData = *(volatile U32*)(ulRDramAddr + (usSec << SEC_SZ_BITS) + ulDwIndex * 4);
            if( ulWrData == ulRdData )
            {
                DBG_Printf("Snap Read: PU %d LUN%d Block %d Page %d Data Miss-compare!\n",pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usBlock,usPage);
                DBG_Printf("Page %d Rdaddr:0x%x ucPln%d ulsec:%d Rdata:0x%x Wdata:0x%x\n",usPage
                ,ulRDramAddr + (usSec<<SEC_SZ_BITS)+ ulDwIndex*4, ucPln, usSec,ulRdData,ulWrData);
                //DBG_Printf("Page %d Rdata:0x%x \n",usPage, ulRdData);
                bsErr = TRUE;
                //DBG_Getch();
            }
        }
    }
    
    if (bsErr)
    {
        DBG_Printf("PU %d Block %d Page %d Error DW Count %d PLN %d N0=%d BLK N0=%d\n",pFlashAddr->ucPU,pFlashAddr->usBlock,usPage,ulDwErrCount, pFlashAddr->bsPln, ulN0, g_ulFTN0Cnt);
        //DBG_Getch();
    }


    return;
}

/****************************************************************************
Function      : FT_CheckData
Input         : 
Output        :

Description   : check one page msg data, also usable for raw data read 
History       :
    20170628    abby    create
****************************************************************************/
LOCAL U32 FT_CheckData(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U16 usSecLen, U8 ucPageType)
{
#ifndef FT_DATA_CHK
    return SUCCESS;
#endif

    FLASH_ADDR tFlashAddr = *pFlashAddr;

    U8 ucPlnStart, ucPlnEnd, ucPlnldx;
    U16 usSecStartInPln, usSecEndInPln; 

    ucPlnStart  = ucSecStart / SEC_PER_PHYPG;
    ucPlnEnd    = (ucSecStart + usSecLen - 1) / SEC_PER_PHYPG;

    for (tFlashAddr.bsPln = ucPlnStart; tFlashAddr.bsPln <= ucPlnEnd; tFlashAddr.bsPln++)
    {
        usSecStartInPln = 0;
        usSecEndInPln   = SEC_PER_PHYPG - 1;
        
        if (ucPlnStart == tFlashAddr.bsPln)
        {
            usSecStartInPln = ucSecStart % SEC_PER_PHYPG;  // SecStartInPlnStart
        }
        if (ucPlnEnd == tFlashAddr.bsPln)
        {
            usSecEndInPln = (ucSecStart + usSecLen - 1) % SEC_PER_PHYPG;
        }
        FT_Check1PlnData(&tFlashAddr,(U8)usSecStartInPln,usSecEndInPln, ucPageType);
    }

    return SUCCESS;
}

/****************************************************************************
Function      : FT_CheckN0Data
Input         : 
Output        :

Description   : check N0 number in one page msg data, only support single plane
History       :
    20170628    abby    create
****************************************************************************/
LOCAL U32 FT_CheckN0Data(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U16 usSecLen, BOOL bErr)
{
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    DEC_SRAM_STATUS_ENTRY tDecSramSts = {0};

    U8 ucPlnStart, ucPlnEnd, ucErrCWNum = 0, ucTotalErrCW = 0;
    U16 usSecStartInPln, usSecEndInPln;
    U32 ulN1 = 0, ulN0 = 0, ulCwFailBitmap = 0, ulTh;
        
    if (!bErr)
        HAL_DecSramGetDecStsEntry(&tFlashAddr, &tDecSramSts);
    else
        HAL_DecSramGetDecStsEntryInErr(&tFlashAddr, &tDecSramSts);
    
    ulCwFailBitmap = tDecSramSts.ulDecFailBitMap;
    while (0 != ulCwFailBitmap)
    {
        if (0 != (ulCwFailBitmap & 1))
        {
            ucTotalErrCW++;
            if(ucErrCWNum <= 7)
                ucErrCWNum++;;
        }
        ulCwFailBitmap >>= 1;
    }
    ulN1 = tDecSramSts.bsN1Accu;
    ulTh = 8 * ((CW_INFO_SZ + DS_CRC_LENTH + 128) * 8 + DS_LBA_LENTH * (8 / 4));
    ulN0 = ulTh - ulN1;

    if(ulN0 > 1)
    {
        DBG_Printf("PU%d BLK%d PLN%d Page%d Th%d ucTotalErrCW%d In 8CWs N0=%d N1=%d\n"
        , tFlashAddr.ucPU, tFlashAddr.usBlock, pFlashAddr->bsPln, tFlashAddr.usPage,ulTh, ucTotalErrCW, ulN0, ulN1);
    }
    ASSERT(ulN1 <= ulTh);
        
    return ulN0;
}

LOCAL void FT_RdCfgReqComm(NFC_READ_REQ_DES *pRdReq, U8 ucSecStart, U16 usSecLen)
{
    pRdReq->bsSecStart = ucSecStart;
    pRdReq->bsSecLen   = usSecLen;

    /* Default setting is for simplicity, but you can change it if you need */    
    pRdReq->bsRedOntf       = FALSE;
    pRdReq->bsLbaChkEn      = FALSE;
    pRdReq->bsSsu0En        = FALSE;
    pRdReq->bsSsu0Ontf      = TRUE;
    pRdReq->bsSsu1En        = FALSE;
    pRdReq->bsSsu1Ontf      = TRUE;
    pRdReq->bsCSEn          = FALSE;
    pRdReq->bsRawData       = g_bFTRawDataRead;
    pRdReq->bsTlcMode       = g_bFTMlcMode;
    pRdReq->bsSnapReadEn    = g_bFTSnapRead;
    pRdReq->bsEmEn          = FALSE;
    pRdReq->bsDsIndex       = DS_ENTRY_SEL;
    pRdReq->ppNfcRed        = NULL;//g_bFTSnapRead ? NULL : (NFC_RED **)g_ppFTRdRed;
    
    if (g_ErrInjEn)
    {
        pRdReq->bsInjErrEn = TRUE;
        pRdReq->pErrInj = &g_tErrInj;
    }
    else
    {
        pRdReq->pErrInj = NULL;
    }
    
    if (g_bFTDecFifoEn)
    {
        pRdReq->bsDecFifoEn = TRUE;
        pRdReq->bsDecFifoIndex = FT_RdGetDecFifoCmdInx();
    }
    
    /*  scramble disable    */
    if (g_bFTRawDataRead)
    {
        volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
        pNfcPgCfg->bsScrBps = TRUE;
    }

    return;
}

LOCAL U32 FT_DecStsChk(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    DEC_SRAM_STATUS_ENTRY tDecSramSts = {0};
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    NFCQ_ENTRY *pNfcqEntry = HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU, pFlashAddr->ucLun);

    /* Read status from DEC SRAM, and check DEC SRAM decode fail bitmap */
    HAL_DecSramGetDecStsEntry(&tFlashAddr, &tDecSramSts);

#if 0
    DBG_Printf("PU%d LUN%d RP%d DEC SRAM STS: \n", pFlashAddr->ucPU, pFlashAddr->ucLun, HAL_NfcGetRP(pFlashAddr->ucPU,pFlashAddr->ucLun));
    DBG_Printf("bsFstItrSyndAccu=0x%x ulRCrcBitMap=0x%x bsN1Accu=%d\n", tDecSramSts.bsFstItrSyndAccu,tDecSramSts.ulRCrcBitMap,tDecSramSts.bsN1Accu);
    DBG_Printf("ulDecFailBitMap=0x%x bsErrCntAcc0T1=%d bsErrCntAcc=%d\n\n", tDecSramSts.ulDecFailBitMap,tDecSramSts.bsErrCntAcc0T1,tDecSramSts.bsErrCntAcc);
    DBG_Printf("bsErrCntAcc=%d\n\n", tDecSramSts.bsErrCntAcc);
#endif
    
    return tDecSramSts.bsErrCntAcc;
}

LOCAL BOOL FT_RdStsAndDataChk(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    BOOL bRet = SUCCESS;
    U8 ucPU, ucLun;
    U16 usBlk, usPage;
    LOCAL U32 l_MaxErrCntInPage[BLK_PER_PLN+RSV_BLK_PER_PLN] = {0};

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    usBlk = pFlashAddr->usBlock;
    usPage = pFlashAddr->usPage;    

    /* NFC CMD REG status check */
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
    {
    #ifdef N0_CHECK
        //g_ulFTN0Cnt += FT_CheckN0Data(pFlashAddr, pRdReq->bsSecStart, pRdReq->bsSecLen);
        FT_CheckN0Data(pFlashAddr, pRdReq->bsSecStart, pRdReq->bsSecLen, TRUE);
    #else
        DBG_Printf("Pu %d LUN %d BLK%d PG%d PageType%d Read Fail, ErrType:%d\n",ucPU
        ,ucLun,usBlk,usPage,pRdReq->bsTlcPgType,HAL_NfcGetErrCode(ucPU, ucLun));
        //DBG_Getch();
    #endif
        HAL_NfcResetCmdQue(ucPU, ucLun);
        HAL_NfcClearINTSts(ucPU, ucLun);
        bRet = FAIL;
    }
    #ifdef ERR_BIT_CNT_CHK
    else//if success, check error bit cnt from DEC SRAM
    {
        U32 ulErrCnt = FT_DecStsChk(pFlashAddr, pRdReq);
        DBG_Printf("PU%d Blk%d Page%d Error Cnt = %d\n", ucPU, usBlk, usPage, ulErrCnt);
        
        if (l_MaxErrCntInPage[usBlk] < ulErrCnt)
        {
            l_MaxErrCntInPage[usBlk] = ulErrCnt;
            DBG_Printf("BLK%d MAX Err Cnt is updated to %d\n",usBlk, l_MaxErrCntInPage[usBlk]);
        }
    }
    #endif
    
#ifndef FT_DATA_CHK
    return bRet;      //high pressure read, do not check data
#endif

    /* RED check */
    if ((!g_bFTRawDataRead) && (NULL != pRdReq->ppNfcRed))//RAW DATA READ: NFC won't update RED
    {
        //FT_CheckRed(pFlashAddr, pRdReq->ppNfcRed, pRdReq->bsSecStart, pRdReq->bsSecLen);
    }
    
    /* Data check */
    bRet = FT_CheckData(pFlashAddr, pRdReq->bsSecStart, pRdReq->bsSecLen, pRdReq->bsTlcPgType);
        
    return bRet;
}

/****************************************************************************
Function      : FT_WriteOnePage
Input         : BOOL bIsSlcMode -- TRUR = SLC operation;FALSE = TLC
                BOOL bIsSinglePln -- TRUE = 1 PLN operation;FALSE = Multi-Pln operation

Output        :
Description   : For flash test.Support single or multi plane program.
                program one page and check status.
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_WriteOnePage(FLASH_ADDR *pFlashAddr)
{
    U8 ucPU  = pFlashAddr->ucPU;
    U8 ucLun = pFlashAddr->ucLun;
    
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    NFC_PRG_REQ_DES tWrReq = {0};

    /* config req descriptor and prepara dummy data */
    FT_WtCfgReqComm(&tWrReq, &tFlashAddr);
    FT_PrepareOnePageDummyData(&tFlashAddr);
    
    while(TRUE == HAL_NfcGetFull(ucPU, ucLun));

    FT_StartTimer();
    
    HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrReq);
    
    /* check status  */
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
    {
        DBG_Printf("Pu %d Block %d Page %d PLN %d Write Fail!\n", ucPU, tFlashAddr.usBlock, tFlashAddr.usPage, tFlashAddr.bsPln);
        HAL_NfcResetCmdQue(pFlashAddr->ucPU, pFlashAddr->ucLun);
        HAL_NfcClearINTSts(pFlashAddr->ucPU, pFlashAddr->ucLun);
        //DBG_Getch();
    }
    else
    {
        //DBG_Printf("Pu %d Block %d Page %d Write OK!\n", ucPU, tFlashAddr.usBlock, tFlashAddr.usPage);
    }
    U32 ulDiffUs = FT_EndTimer();
    DBG_Printf("Write page %d cost cycle 0x%x latency is %dus\n", pFlashAddr->usPage, g_ulFTDiffCycle, ulDiffUs);
}


/****************************************************************************
Function      : FT_ReadOnePage
Input         : U8 ucPageType -- For TSB TLC, LP+MP+UP
Output        :
Description   : For flash test.Support single or multi plane read.
                read one page and check status and data
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
U32 FT_ReadOnePage(U8 ucPageType, U8 ucSecStart, U16 usSecLen, FLASH_ADDR *pFlashAddr)
{
    U8 ucPU = pFlashAddr->ucPU, ucLun = pFlashAddr->ucLun;
    U32 ulStatus;
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    NFC_READ_REQ_DES tRdReq = {0};

    FT_RdCfgReqComm(&tRdReq, ucSecStart, usSecLen);
    
    tRdReq.bsTlcPgType  = ucPageType;

#ifdef OTP_TEST
    tRdReq.bsRdBuffId   = g_aFT1PlnRdBufId[tFlashAddr.ucPU*2];
#else
    tRdReq.bsRdBuffId   = g_aFT1PlnRdBufId[ucPageType];
#endif

    while (TRUE == HAL_NfcGetFull(ucPU, ucLun));
    
    FT_StartTimer();

    HAL_NfcSinglePlnRead(&tFlashAddr, &tRdReq, FALSE);

    /* check status and data */
    ulStatus = FT_RdStsAndDataChk(&tFlashAddr, &tRdReq);

    U32 ulDiffUs = FT_EndTimer();
    //if (0 == pFlashAddr->usPage%16)
    {
        DBG_Printf("Read PU%d blk%d pln%d page %d PageType%d cost cycle 0x%x latency is %dus\n"
        , pFlashAddr->ucPU, pFlashAddr->usBlock, pFlashAddr->bsPln, pFlashAddr->usPage, ucPageType, g_ulFTDiffCycle, ulDiffUs);
    }
    return ulStatus;
}

/****************************************************************************
Function      : FT_EraseOneBlk
Input         : 
Output        :
Description   : For flash test.Support single or multi plane erase.
                Erase one block.
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
BOOL FT_EraseOneBlk(FLASH_ADDR *pFlashAddr)
{
    U8 ucPU  = pFlashAddr->ucPU;
    U8 ucLun = pFlashAddr->ucLun;
    BOOL bRet = FAIL;
   
    while(TRUE == HAL_NfcGetFull(ucPU, ucLun));

    FT_StartTimer();

    HAL_NfcSingleBlockErase(pFlashAddr, g_bFTMlcMode);

    /*    check nfc status    */
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
    {
        HAL_NfcResetCmdQue(pFlashAddr->ucPU, pFlashAddr->ucLun);
        HAL_NfcClearINTSts(pFlashAddr->ucPU, pFlashAddr->ucLun);

        DBG_Printf("PU%d LUN%d BLK%d PLN%d Erase Fail!\n", ucPU, ucLun, pFlashAddr->usBlock, pFlashAddr->bsPln);
        HAL_NfcResetLun(pFlashAddr);//error handle, otherwise program other blk also will be fail
        bRet = FAIL;
    }
    else
    {
        U32 ulDiffUs = FT_EndTimer();
        DBG_Printf("Erase PU%d LUN%d BLK%d cost cycle 0x%x latency is %dus\n", ucPU, ucLun, pFlashAddr->usBlock, g_ulFTDiffCycle, ulDiffUs);
        bRet = SUCCESS;
        //DBG_Printf("PU%d LUN %d BLK%d PLN%d Erase OK!\n", ucPU, ucLun, pFlashAddr->usBlock, pFlashAddr->bsPln);
    }
    return bRet;
}

/****************************************************************************
Function      : FT_WriteOneBlk
Input         : 
Output        :
Description   : For flash test. Program one block from usStartPage to usEndPage
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_WriteOneBlk(U16 usStartPage, U16 usEndPage, FLASH_ADDR *pFlashAddr)
{   
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    U16 usPrgCnt = usEndPage - usStartPage;
    usPrgCnt = g_bFTMlcMode ? (g_bFTSinglePgProg ? usPrgCnt : (usPrgCnt/PGADDR_PER_PRG)) : usPrgCnt;//dual page mode, program 2 pages once
    
    /* program */
    tFlashAddr.usPage = usStartPage;
    while (0 != usPrgCnt)
    {
        FT_WriteOnePage(&tFlashAddr); //write 1 page and check status
        tFlashAddr.usPage += g_bFTMlcMode ? (g_bFTSinglePgProg ? 1: PGADDR_PER_PRG) : 1;
        usPrgCnt--;
    }  
}

/****************************************************************************
Function      : FT_ReadOneBlk
Input         : 
Output        :
Description   : For flash test. Read each page from one block from usStartPage to usEndPage
Reference     :
History       :
    20170626    abby    create
****************************************************************************/
void FT_ReadOneBlk(U16 usStartPage, U16 usEndPage, FLASH_ADDR *pFlashAddr)
{   
    U16 usSecLen;
    U32 ulStatus;
    
    pFlashAddr->usPage = usStartPage;
    usSecLen = SEC_PER_PHYPG;
    
    while (pFlashAddr->usPage < usEndPage)
    {
        ulStatus = FT_ReadOnePage(0, 0, usSecLen, pFlashAddr); // read, check status and check data
    #ifdef ERR_BIT_CNT_CHK//check raw data FBC
        if (FAIL == ulStatus)
        {
            g_bFTRawDataRead = TRUE;
            FT_ReadOnePage(0, 0, usSecLen, pFlashAddr);
            g_bFTRawDataRead = FALSE;
        }
    #endif
        pFlashAddr->usPage++;
    }  
}

/* end of this file */
