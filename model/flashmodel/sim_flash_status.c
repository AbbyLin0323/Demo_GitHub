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

*Filename     : sim_flash_status.c
*Version      :   Ver 1.0
*Date         :
*Author       :  LeoYang

*Description:
*interface for set flash status

*Depend file:
*sim_flash_status.h

*Modification History:
*20160704      LeoYang 001 first create

*************************************************/
#include "sim_flash_status.h"

GLOBAL FLASH_RUNTIME_STATUS g_aFlashStatus;
GLOBAL PRE_CMD_INFO g_aPreCmdInfo[NFC_MODEL_LUN_SUM];
GLOBAL U32 g_ulWorkLun;

extern void Flash_ResetErrInfo(ERR_INFO *pErrInfo);
extern U8 NFC_GetPartInWL(U8 ucCmdCode);

/* LUN related runtime status interface */
void FlashStsM_SetLunMode(U8 ucLun, U8 bTrue)
{
    g_aFlashStatus.aFlashLunInfo[ucLun].bsSLCMode = bTrue;
    return;
}
void FlashStsM_SetReadRetryEn(U8 ucLun, U8 bTrue)
{
    g_aFlashStatus.aFlashLunInfo[ucLun].bsRetryEn = bTrue;
    return;
}
void FlashstsM_SetLunCacheWr(U8 ucLun, U8 bTrue)
{
     g_aFlashStatus.aFlashLunInfo[ucLun].bsCacheWr = bTrue;
     return;
}
void FlashstsM_SetLunCacheRd(U8 ucLun, U8 bTrue)
{
     g_aFlashStatus.aFlashLunInfo[ucLun].bsCacheRd = bTrue;
     return;
}
void FlashstsM_AddLunBBCnt(U8 ucLun)
{
     g_aFlashStatus.aFlashLunInfo[ucLun].bsBadBlockCnt++;
    return;
}
BOOL FlashStsM_GetReadRetryEn(U8 ucLun)
{
    return g_aFlashStatus.aFlashLunInfo[ucLun].bsRetryEn;
}
BOOL FlashstsM_GetLunCacheWr(U8 ucLun)
{
    return g_aFlashStatus.aFlashLunInfo[ucLun].bsCacheWr;
}
BOOL FlashstsM_GetLunCacheRd(U8 ucLun)
{
    return g_aFlashStatus.aFlashLunInfo[ucLun].bsCacheRd;
}
U16 FlashstsM_GetLunBBCnt(U8 ucLun)
{
    return g_aFlashStatus.aFlashLunInfo[ucLun].bsBadBlockCnt;
}
U8 FlashstsM_GetLunMode(U8 ucLun)
{
    return g_aFlashStatus.aFlashLunInfo[ucLun].bsSLCMode;
}


/*Block related runtime status interface */

void FlashStsM_SetBlkSts(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucStatus)
{
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockSts= ucStatus;
    return;
}
void FlashStsM_AddBlkPeCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsPeCnt++;
    return;
}
void FlashStsM_AddBlkReadCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsReadCnt++;
    return;
}
void FlashStsM_AddBlkEraseFailCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsEraseFaiCnt++;
    return;
}
U8 FlashStsM_GetBlkType(U8 ucLun, U8 ucPln, U16 usBlk)
{
    return  g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType;
}
U8 FlashStsM_GetBlkSts(U8 ucLun, U8 ucPln, U16 usBlk)
{
     return g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockSts;
}
U16 FlashStsM_GetBlkPeCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
     return g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsPeCnt;
}
U32 FlashStsM_GetBlkReadCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
     return g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsReadCnt;
}
U32 FlashStsM_GetBlkEraseFailCnt(U8 ucLun, U8 ucPln, U16 usBlk)
{
    return  g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsEraseFaiCnt;
}

BOOL FlashStsM_CheckStsFree(U8 ucLun, U8 ucPln, U16 usBlk)
{
    U16 usPairPage;

    if(BLOCK_ERASED != FlashStsM_GetBlkSts(ucLun, ucPln, usBlk) ||
        BLOCK_INVALID != FlashStsM_GetBlkType(ucLun, ucPln, usBlk) ||
        0 != FlashStsM_GetBlkReadCnt(ucLun, ucPln, usBlk))
    {
        return FALSE;
    }

    for(usPairPage = 0; usPairPage < PAIR_PAGE_PER_BLK; usPairPage++)
    {
        if(PAIR_PAGE_FREE != g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsPageSts ||
            FALSE != g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsLpIsWritten ||
            FALSE != g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsMpIsWritten ||
            FALSE != g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsUpIsWritten)
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL FlashStsM_CheckEraseFail(U8 ucLun, U8 ucPln,U16 usBlk)
{
    if (BLOCK_BAD == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockSts)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL FlashStsM_NextWLOpenCheck(PFLASH_PHY pFlash_phy)
{
    if((PAIR_PAGE_PER_BLK - 1) == g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock) ))
    {
        return FALSE;
    }

    if(BLOCK_INVALID != FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock) &&
        BLOCK_SLC != FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock) &&
        PAIR_PAGE_OPEN == FlashStsM_GetNextPairPgSts(pFlash_phy))
    {
        return TRUE;
    }
    return FALSE;
}

/* Pair-Page related runtime status interface */
void FlashStsM_SetLpMpUpWrite(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    switch(g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        case FLASH_LOW_PAGE:
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsLpIsWritten = TRUE;
            break;
        case FLASH_MID_PAGE:
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsMpIsWritten = TRUE;
            break;
        case FLASH_HIGH_PAGE:
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsUpIsWritten = TRUE;
            break;
        default:
            DBG_Printf("Flash status: Set L/M/U page wriitten error\n");
            DBG_Getch();
            break;
    }

    return;
}
void FlashStsM_SetPairPgSts(PFLASH_PHY pFlash_phy, U8 ucPgSts)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsPageSts = ucPgSts;

    return;
}
void FlashStsM_SetPairPgSrcSeed(PFLASH_PHY pFlash_phy, NFCQ_ENTRY *pNFCQEntry, U32 ulSeedIndex)
{
    U8 ucLun, ucPln;
    U8 ucSeed;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    /* Get scramble seed according seed index */
    switch(ulSeedIndex)
    {
        case 0:
            ucSeed = pNFCQEntry->bsScrSeed0;
            break;
        case 1:
            ucSeed = (pNFCQEntry->bsScrSeed1Msb << 1) | pNFCQEntry->bsScrSeed1Lsb;
            break;
        case 2:
           ucSeed = (pNFCQEntry->bsScrSeed2Msb << 1) | pNFCQEntry->bsScrSeed2Lsb;
           break;
        default:
            DBG_Printf("Flash status: Get NfcqSeed error\n");
            DBG_Getch();
            break;
    }

    /* Set scramble seed according page type */
    switch(g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        case FLASH_LOW_PAGE:
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsSrcSeed0 = ucSeed;
            break;
        case FLASH_MID_PAGE:
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsSrcSeed1 = ucSeed;
            break;
        case FLASH_HIGH_PAGE:
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsSrcSeed2 = ucSeed;
            break;
        default:
            DBG_Printf("Flash status: set SrcSeed error\n");
            DBG_Getch();
            break;
    }
    return;
}

U8 FlashStsM_GetLpMpUpWrite(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    switch(g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        case FLASH_LOW_PAGE:
            return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsLpIsWritten;
        case FLASH_MID_PAGE:
            return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsMpIsWritten;
        case FLASH_HIGH_PAGE:
            return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsUpIsWritten;
        default:
            DBG_Printf("Flash status: Get L/M/U page wriitten error\n");
            DBG_Getch();
            break;
    }
    return 0;
}

U8 FlashStsM_GetPairPgSts(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsPageSts;
}

U8 FlashStsM_GetNextPairPgSts(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage, usNextPairPg;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));
    usNextPairPg = usPairPage++;

    return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usNextPairPg].bsPageSts;
}

U8 FlashStsM_GetPairPgPrgStep(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep;
}

U8 FlashStsM_GetPairPgSrcSeed(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    switch(g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        case FLASH_LOW_PAGE:
            return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsSrcSeed0;
        case FLASH_MID_PAGE:
            return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsSrcSeed1;
        case FLASH_HIGH_PAGE:
            return g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsSrcSeed2;
        default:
            DBG_Printf("Flash status: Get SrcSeed error\n");
            DBG_Getch();
            break;
    }
    return 0;
}
BOOL FlashStsM_CheckPairPgType(PFLASH_PHY pFlash_phy)  //check page's prog type is correct (LP-only, LP+MP, LP+MP+UP)
{
    /* check after write */
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;
    U8 ucPairPageType;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));
    ucPairPageType = g_tFlashSpecInterface.GetPairPgType(usPairPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    if(LOW_PAGE_ONLY == ucPairPageType &&
        (TRUE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsMpIsWritten  ||
         TRUE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsUpIsWritten))
    {
        DBG_Printf("Low-page-only program error, Mid page or Upper page is written\n");
        return FALSE;
        //DBG_Getch();
    }
    else if (LOW_MID_PAGE == ucPairPageType &&
        TRUE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsUpIsWritten)
    {
        DBG_Printf("Low-Mid-page program error, Upper page is written\n");
        return FALSE;
        //DBG_Getch();
    }

    return TRUE;
}
BOOL FlashStsM_CheckPairPgPrg(PFLASH_PHY pFlash_phy)
{
    /* check before write */
    U8 ucLun, ucPln;
    U16 usBlk, usPairPage;
    U8 ucPageType;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));
    ucPageType = g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    /* check pair page data program is correct */
#if defined (FLASH_IM_3DTLC_GEN2)
    if(FLASH_HIGH_PAGE == ucPageType && FALSE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsLpIsWritten)
#else
    if(FLASH_HIGH_PAGE == ucPageType &&
       (FALSE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsMpIsWritten ||
        FALSE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsLpIsWritten))
#endif
    {
        DBG_Printf("Lun %d Blk %d Page %d Write High page error, Mid or Low page not written\n", ucLun, usBlk, pFlash_phy->nPage);
        return FALSE;
    }
    else if(FLASH_MID_PAGE == ucPageType &&
        FALSE == g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsLpIsWritten)
    {
        DBG_Printf("Lun %d Blk %d Page %d Write Mid page error, Low page not written\n", ucLun, usBlk, pFlash_phy->nPage);
        return FALSE;
    }

    return TRUE;
}
BOOL FlashStsM_CheckPairPgSrcSeed(U8 ucSeed, NFCQ_ENTRY *pNFCQEntry, U32 ulSeedIndex)
{
    U8 ucNfcqSeed;

    switch(ulSeedIndex)
    {
        case 0:
            ucNfcqSeed = pNFCQEntry->bsScrSeed0;
            break;
        case 1:
            ucNfcqSeed = (pNFCQEntry->bsScrSeed1Msb << 1) | pNFCQEntry->bsScrSeed1Lsb;
            break;
        case 2:
            ucNfcqSeed = (pNFCQEntry->bsScrSeed2Msb << 1) | pNFCQEntry->bsScrSeed2Lsb;
            break;
        default:
            DBG_Printf("Flash status: Get NfcqSeed error\n");
            DBG_Getch();
            break;
    }

    if(ucNfcqSeed != ucSeed)
    {
        DBG_Printf("scramble seed compare error, Seed %d, ucNfcqSeed %d\n", ucSeed, ucNfcqSeed);
        return FALSE;
    }
    return TRUE;
}

BOOL FlashStsM_CheckPrgFail(PFLASH_PHY pFlash_phy)
{
    if(PAIR_PAGE_PROGRAM_FAIL == FlashStsM_GetPairPgSts(pFlash_phy))
    {
        return TRUE;
    }
    return FALSE;
}

void FlashStsM_InitFlashStatus(void)
{
    memset(&g_aFlashStatus, 0, sizeof(FLASH_RUNTIME_STATUS));
    memset(&g_aPreCmdInfo[0], 0xFF, sizeof(PRE_CMD_INFO) * NFC_MODEL_LUN_SUM);
    return;
}

/* Reset status */
void FlashStsM_ResetSts(U8 ucLun, U8 ucPln, U16 usBlk)
{
    /* clear block status */
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_INVALID;
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockSts = BLOCK_ERASED;
    g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsReadCnt = 0;

    /* clear all pair page status */
    COM_MemZero((U32*)&g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][0], sizeof(FLASH_PAIRPG_INFO)*PAIR_PAGE_PER_BLK /sizeof(U32));

    return;
}

void FlashStsM_SetPreCmdInfo(PFLASH_PHY pFlash_phy, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucLun = (U8)pFlash_phy->ucLunInTotal;

    /*Set previous cmd param*/
    g_aPreCmdInfo[ucLun].bsLun = pFlash_phy->ucLunInTotal;
    g_aPreCmdInfo[ucLun].bsBlk[pFlash_phy->nPln] = pFlash_phy->nBlock;
    g_aPreCmdInfo[ucLun].bsPage = pFlash_phy->nPage;
    g_aPreCmdInfo[ucLun].bsCmdType = pFlashCmdParam->bsCmdType;
    g_aPreCmdInfo[ucLun].bsTLCMode = pFlashCmdParam->bsIsTlcMode;

    return;
}

void FlashStsM_ClrPreCmdInfo(U8 ucLun)
{
    U8 ucPln;
    g_ulWorkLun = INVALID_2F;
    g_aPreCmdInfo[ucLun].bsLun = INVALID_2F;
    g_aPreCmdInfo[ucLun].bsPage = INVALID_4F;
    g_aPreCmdInfo[ucLun].bsCmdType = INVALID_2F;
    g_aPreCmdInfo[ucLun].bsTLCMode = INVALID_2F;

    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        g_aPreCmdInfo[ucLun].bsBlk[ucPln] = INVALID_4F;
    } 

    return;
}

void FlashStsM_CheckCCRParam(const NFCM_LUN_LOCATION *pLunLocation, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    FLASH_PHY flash_phy;
    U8 ucPlnIndex;

    /*Get LunInTotal param*/
    NFC_GetFlashAddr(pLunLocation, pFlashCmdParam->p_nf_cq_entry->atRowAddr[0].bsRowAddr, 0, 0, &flash_phy);
    g_ulWorkLun = flash_phy.ucLunInTotal;

    /*Get correct TLC mode bit by previous cmd info*/
    pFlashCmdParam->bsIsTlcMode = g_aPreCmdInfo[g_ulWorkLun].bsTLCMode;

    /*Check Change column read param is match previous cmd*/
    for (ucPlnIndex = 0; ucPlnIndex < pFlashCmdParam->bsPlnNum; ucPlnIndex++)
    {
        NFC_GetFlashAddr(pLunLocation, pFlashCmdParam->p_nf_cq_entry->atRowAddr[ucPlnIndex].bsRowAddr, NFC_GetPartInWL(pFlashCmdParam->bsCmdCode), pFlashCmdParam->bsIsTlcMode, &flash_phy);

        if (g_aPreCmdInfo[flash_phy.ucLunInTotal].bsCmdType != pFlashCmdParam->bsCmdType ||
            g_aPreCmdInfo[flash_phy.ucLunInTotal].bsLun != flash_phy.ucLunInTotal ||
            g_aPreCmdInfo[flash_phy.ucLunInTotal].bsBlk[flash_phy.nPln] != flash_phy.nBlock ||
            g_aPreCmdInfo[flash_phy.ucLunInTotal].bsPage != flash_phy.nPage)
        {
            DBG_Printf("Change column param error!!\n");
            DBG_Printf("PreCmdType: 0x%x, CurCmdType: 0x%x\n", g_aPreCmdInfo[flash_phy.ucLunInTotal].bsCmdType, pFlashCmdParam->bsCmdType);
            DBG_Printf("PreCmdLun: %d, CurCmdLun: %d\n",g_aPreCmdInfo[flash_phy.ucLunInTotal].bsLun, flash_phy.ucLunInTotal);
            DBG_Printf("PreCmdBlk_Pln: %d_%d, CurCmdBlk: %d\n", g_aPreCmdInfo[flash_phy.ucLunInTotal].bsBlk[flash_phy.nPln], ucPlnIndex, flash_phy.nBlock);
            DBG_Printf("PreCmdPage: %d, CurCmdPage: %d\n", g_aPreCmdInfo[flash_phy.ucLunInTotal].bsPage,  flash_phy.nPage);
            DBG_Getch();
        }
    }
    return;
}
