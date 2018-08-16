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
*******************************************************************************/

#include <COM/BaseDef.h>
#include <flashmodel/sim_flash_shedule.h>
#include <256fm_x64/flash_meminterface.h>
#include <flashmodel/sim_flash_status.h>

#ifdef FLASH_INTEL_3DTLC
extern FLASH_RUNTIME_STATUS g_aFlashStatus;
extern BOOL Flash_TlcPrgDataCmp(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nType);
FLASH_PRG_ORDER l_a3dTlcPrgPPO[NFC_MODEL_LUN_SUM] = { 0 };
LOCAL BOOL l_bLowPGMode[NFC_MODEL_LUN_SUM][PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN]  = {0};

extern BOOL g_bSquashMode;

#define INTEL_3DTLC_MAX_PG_NUM 1535


U16 Intel3dTlc_GetUpperPG(U16 usExtraPg)
{
    U16 usUpPgNum;
    U8 ucRow, ucCol;
    U16 usFirstExtraPg = 112;

    ucRow = ((usExtraPg - usFirstExtraPg) / 3) / 4;
    ucCol = ((usExtraPg - usFirstExtraPg) / 3) % 4;

    if(208 > usExtraPg)
    {
        usUpPgNum = 49 + (ucRow * 8) + (ucCol * 2);
    }
    else if(1504 > usExtraPg && 208 <= usExtraPg)
    {
        usUpPgNum = 114 + ((ucRow-8) * 12) + (ucCol * 3);
    }
    else
    {
        ucRow = ((usExtraPg - 1504) /2 ) / 4;
        ucCol = ((usExtraPg - 1504) /2 ) % 4;
        usUpPgNum = 1410 + (ucRow * 12) + (ucCol * 3);
    }

    return usUpPgNum;
}

U16 Intel3dTlc_LowPgModeGetNextPG(U16 usPageNum, U8 ucPageInWL)
{
    U16 ulNextPgNum;

    if (usPageNum < 16)
    {
        ulNextPgNum = usPageNum + 1;
    }
    else if (usPageNum >= 16 && usPageNum < 110)
    {
        ulNextPgNum = usPageNum + 2;
    }
    else if (usPageNum >= 110 && usPageNum < 1504)
    {
        ulNextPgNum = usPageNum + 3;
    }
    else if (usPageNum > 1504)
    {
        ulNextPgNum = usPageNum + 2;
    }

    return ulNextPgNum;
}

U8 Intel3dTlc_GetCmdType(U8 ucCmdCode)
{
    U8 ucCmdType = CMD_CODE_OTHER;

    if (NF_PRCQ_RETRY_EN == ucCmdCode)
    {
        ucCmdType = CMD_CODE_READRETRY_EN;
    }
    else if (((ucCmdCode >= NF_CMD_READRETRY_ADJ_START) && (ucCmdCode <= NF_CMD_READRETRY_ADJ_END))
        || ((ucCmdCode >= NF_CMD_SHIFTREAD_ADJ_START) && (ucCmdCode <= NF_CMD_SHIFTREAD_ADJ_END)))
    {
        ucCmdType = CMD_CODE_READRETRY_ADJ;
    }

    else if ((NF_PRCQ_TLC_READ == ucCmdCode) || (NF_PRCQ_TLC_READ_MULTIPLN == ucCmdCode) || (NF_PRCQ_TLC_SNAP_READ == ucCmdCode))
    {
        ucCmdType = CMD_CODE_READ;
    }

    else if ((NF_PRCQ_TLC_ERS == ucCmdCode) || (NF_PRCQ_TLC_ERS_MULTIPLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_ERASE;
    }

    /*Single pln TLC wirte cmd*/
    else if ((NF_PRCQ_TLC_PRG == ucCmdCode) || (NF_PRCQ_TLC_PRG_MULTIPLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_PROGRAM;
    }

    /*Single pln low page cmd*/
    else if ((NF_PRCQ_PRG_LOW_PG == ucCmdCode) || (NF_PRCQ_TLC_PRG_LOW_PG == ucCmdCode))
    {
        ucCmdType = CMD_CODE_PROGRAM;
    }

    /*Multi-Pln low page cmd*/
    else if ((NF_PRCQ_PRG_MULTIPLN_LOW_PG_NO_HIGH == ucCmdCode) || (NF_PRCQ_PRG_MULTIPLN_LOW_PG == ucCmdCode) ||
        (NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG_NO_HIGH == ucCmdCode) || (NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG == ucCmdCode))
    {
        ucCmdType = CMD_CODE_PROGRAM;
    }
    return ucCmdType;
}

U8 Intel3dTlc_GetPgType(U16 usPage, U8 ucBlkType)
{
    U8 ucPageType;

    if (BLOCK_SLC == ucBlkType)
    {
        return FLASH_LOW_PAGE;
    }

    if (usPage < 16)
    {
        ucPageType = FLASH_LOW_PAGE;
    }

    if ((usPage >= 16) && (usPage < 112))
    {
        if (0 == usPage % 2)
        {
            ucPageType = FLASH_LOW_PAGE;
        }
        else
        {
            ucPageType = FLASH_MID_PAGE;
        }
    }

    if ((usPage >= 112) && (usPage < 1505))
    {
        if (0 == (usPage - 112) % 3)
        {
            ucPageType = FLASH_HIGH_PAGE;
        }
        else if (1 == (usPage - 112) % 3)
        {
            ucPageType = FLASH_LOW_PAGE;
        }
        else
        {
            ucPageType = FLASH_MID_PAGE;
        }
    }

    if ((usPage >= 1505) && (usPage < 1536))
    {
        if (0 == usPage % 2)
        {
            ucPageType = FLASH_HIGH_PAGE;
        }
        else
        {
            ucPageType = FLASH_LOW_PAGE;
        }
    }

    return ucPageType;
}

U8 Intel3dTlc_GetPairPgType(U16 usPairPage, U8 ucBlkType)
{
    U8 ucPairPgType;

    if (BLOCK_SLC == ucBlkType)
    {
        return LOW_PAGE_ONLY;
    }

    if (usPairPage < 16 || usPairPage > 527)
    {
        ucPairPgType = LOW_PAGE_ONLY;
    }
    else if ((usPairPage >= 16 && usPairPage < 32) ||(usPairPage >= 512 && usPairPage < 528))

    {
        ucPairPgType = LOW_MID_PAGE;
    }

    else
    {
        ucPairPgType = LOW_MID_HIGH_PAGE;
    }

    return ucPairPgType;
}

U8 Intel3dTlc_GetPlnNum (U8 ucCmdCode)
{
    U8 ucPlnNum;

    if (NF_PRCQ_ERS <= ucCmdCode && NF_PRCQ_PRG >= ucCmdCode)
    {
        ucPlnNum = 1;
    }
    else if ((NF_PRCQ_TLC_ERS <= ucCmdCode && NF_PRCQ_TLC_PRG_LOW_PG >= ucCmdCode) ||
        NF_PRCQ_PRG_LOW_PG == ucCmdCode)
    {
        ucPlnNum = 1;
    }
    else if (NF_PRCQ_CCLR == ucCmdCode)
    {
        ucPlnNum = 1;
    }
    else
    {
        ucPlnNum = PLN_PER_LUN;
    }

    return ucPlnNum;
}


U16 Intel3dTlc_GetPairPage(U16 usPage, U8 ucBlkType)
{
    U16 usUpPgNum = INVALID_4F;
    U16 usPairPgNum;

    if (BLOCK_SLC == ucBlkType)
    {
        return usPage;
    }

    /*Get pair page for TLC block*/
    if (usPage < 16)
    {
        usPairPgNum = usPage;
    }
    else if (usPage >= 16 && usPage < 112)
    {
        usPairPgNum = (usPage + 16) / 2;
    }
    else if ((usPage >= 112) && (usPage < 1505))
    {
        if (FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(usPage, ucBlkType))
        {
            usUpPgNum = Intel3dTlc_GetUpperPG(usPage);
            usPairPgNum = g_tFlashSpecInterface.GetPairPage(usUpPgNum, ucBlkType);
        }
        else
        {
            usPairPgNum = 64 + ((usPage - 112) /3);
        }
    }
    else if ((usPage >= 1505) && (usPage < 1536))
    {
        if (FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(usPage, ucBlkType))
        {
            usUpPgNum = Intel3dTlc_GetUpperPG(usPage);
            usPairPgNum = g_tFlashSpecInterface.GetPairPage(usUpPgNum, ucBlkType);
        }
        else
        {
            usPairPgNum = 528 + ((usPage - 1505) / 2);
        }
    }

    return usPairPgNum;
}

void Intel3dTlc_SetBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode)
{
    if (CMD_CODE_PROGRAM == ucCmdType && TRUE == bsTLCMode)
    {
        // set BlkType is TLC_LOWPG blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_TLC_LOWPG;

        // if write page 17 (upper page), set BlkType to TLC blk
        if (17 == usPage)
        {
            g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_TLC;
            l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = 17;
        }
    }
    else if (CMD_CODE_PROGRAM == ucCmdType && FALSE == bsTLCMode)
    {
        // set BlkType is SLC blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_SLC;
    }
    else
    {
        // set BlkType is invalid blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_INVALID;
    }

    return;
}

void Intel3dTlc_IncPairPgPrgStep(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln, ucPairPageType ;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));
    ucPairPageType = g_tFlashSpecInterface.GetPairPgType(usPairPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    if (LOW_PAGE_ONLY == ucPairPageType)
    {
        if (FLASH_LOW_PAGE != g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
        {
            DBG_Printf("Page number %d error,  on pair page %d with low page only\n", pFlash_phy->nPage, usPairPage);
            DBG_Getch();
        }
        g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = PAIR_PAGE_PRG_MAX;
    }
    else
    {
        if (FLASH_MID_PAGE == g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
        {
            g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep++;

            if (LOW_MID_PAGE == ucPairPageType)
            {
                g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = PAIR_PAGE_PRG_MAX;
            }
        }
    }

    if (g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep > PAIR_PAGE_PRG_MAX)
    {
        DBG_Printf("Flash status: program step error\n");
        DBG_Getch();
    }

    return;
}

void Intel3dTlc_ResetPrgOrder(U8 ucLun, U16 usBlk, U8 ucPln)
{
    l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk] = 0;
    l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = 0;
    l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = 0;

    l_bLowPGMode[ucLun][ucPln][usBlk]  = FALSE;
    return;
}

void Intel3dTlc_GetWriteFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
{
    U8  pln, plun, rp = 0, ceSel = 0;
    U16 pbn, ppo;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U32 row_addr = pFlashCmdParam->phy_page_req[ulAtomPageIndex].row_addr;


    if (FALSE == IsTLCMode && TRUE == g_bSquashMode)
    {
        ppo = (row_addr & ((1 << SLC_PG_PER_BLK_BITS) - 1)) * PAGE_PER_WL;
        pbn = ((row_addr >> SLC_BLK_POS_IN_ROW_ADDR) & BLK_PER_PLN_MSK);
        pln = ((row_addr >> SLC_PLN_POS_IN_ROW_ADDR) & PLN_PER_LUN_MSK);
    }
    else
    {
        ppo = (row_addr & ((1 << PG_PER_BLK_BITS) - 1)) * PAGE_PER_WL;

        pbn = ((row_addr >> BLK_POS_IN_ROW_ADDR) & BLK_PER_PLN_MSK);
        pln = ((row_addr >> PLN_POS_IN_ROW_ADDR) & PLN_PER_LUN_MSK);
    }

    plun = (row_addr >> LUN_POS_IN_ROW_ADDR);

    pFlashAddr->nBlock = pbn;
    pFlashAddr->nPln = pln;
    pFlashAddr->nPage = ppo;
    pFlashAddr->bsPhyPuInTotal = ucPhyPuInTotal;
    pFlashAddr->bsLunInCe = plun;

    rp = NFC_GetRP(tNfcOrgStruct);
    ceSel = (U8)(NfcM_GetTriggerReg(tNfcOrgStruct, rp)->bsCESel);

    pFlashAddr->ucLunInTotal = (pFlashAddr->bsPhyPuInTotal * LUN_PER_CE * NFC_CE_PER_PU + ceSel * LUN_PER_CE + pFlashAddr->bsLunInCe);

    return;
}

void Intel3dTlc_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
{
    U32 ulTransferLen = 0;

    /*Set Transfer Length*/
    if ((NF_PRCQ_PRG == ucCmdCode) || (NF_PRCQ_PRG_MULTIPLN == ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * CW_INFO_SZ;
    }
    else if ((NF_PRCQ_READ == ucCmdCode) || (NF_PRCQ_READ_MULTIPLN == ucCmdCode) || (NF_PRCQ_SLC_SNAP_READ == ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * SEC_SIZE;
    }

    /*SLC write cmd*/
    else if (NF_PRCQ_PRG_LOW_PG == ucCmdCode || (NF_PRCQ_PRG_MULTIPLN_LOW_PG_NO_HIGH <= ucCmdCode && NF_PRCQ_PRG_MULTIPLN_LOW_PG >= ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * CW_INFO_SZ;
    }

    /*TLC write cmd*/
    else if ((NF_PRCQ_TLC_PRG <= ucCmdCode && NF_PRCQ_TLC_PRG_LOW_PG >= ucCmdCode) ||
        (NF_PRCQ_TLC_PRG_MULTIPLN <= ucCmdCode && NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG >= ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * CW_INFO_SZ;
    }

    /*TLC Read cmd*/
    else if (NF_PRCQ_TLC_READ == ucCmdCode || NF_PRCQ_TLC_READ_MULTIPLN == ucCmdCode || NF_PRCQ_TLC_SNAP_READ == ucCmdCode)
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * SEC_SIZE;
    }

    /*Check Transfer Length*/
    if ((NF_PRCQ_PRG == ucCmdCode) && (ulTransferLen != g_PG_SZ))
    {
        DBG_Getch();
    }

    /*SLC write cmd check*/
    else if ((NF_PRCQ_PRG_LOW_PG == ucCmdCode) && (ulTransferLen != g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_PRG_MULTIPLN <= ucCmdCode && NF_PRCQ_PRG_MULTIPLN_LOW_PG >= ucCmdCode) &&
        (ulTransferLen != (g_PG_SZ * PLN_PER_LUN)))
    {
        DBG_Getch();
    }

    /*TLC write cmd check*/
    else if ((NF_PRCQ_TLC_PRG == ucCmdCode) && (ulTransferLen != g_PG_SZ * PGADDR_PER_PRG))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_TLC_PRG_LOW_PG == ucCmdCode) && (ulTransferLen != g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_TLC_PRG_MULTIPLN == ucCmdCode) && (ulTransferLen != g_PG_SZ * PLN_PER_LUN * PGADDR_PER_PRG))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG_NO_HIGH == ucCmdCode || NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG == ucCmdCode)
        && (ulTransferLen != g_PG_SZ * PLN_PER_LUN))
    {
        DBG_Getch();
    }

    /*SLC read cmd check*/
    if ((NF_PRCQ_READ == ucCmdCode) && (ulTransferLen > g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_READ_MULTIPLN == ucCmdCode) && (ulTransferLen > (g_PG_SZ * PLN_PER_LUN)))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_SLC_SNAP_READ == ucCmdCode) && (ulTransferLen > g_PG_SZ / 2))
    {
        DBG_Getch();
    }

    /*TLC read cmd check*/
    else if ((NF_PRCQ_TLC_READ == ucCmdCode) && (ulTransferLen > g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_TLC_READ_MULTIPLN == ucCmdCode) && (ulTransferLen > (g_PG_SZ * PLN_PER_LUN)))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_TLC_SNAP_READ == ucCmdCode) && ((ulTransferLen > g_PG_SZ / 2)))
    {
        DBG_Getch();
    }

    /*Current hardware only supports snap read first 8K+. After the hardware optimization, need to delete this check*/
    if ((NF_PRCQ_SLC_SNAP_READ == ucCmdCode || NF_PRCQ_TLC_SNAP_READ == ucCmdCode) && (pNfcqEntry->aSecAddr[0].bsSecStart + pNfcqEntry->aSecAddr[0].bsSecLength > 16))
    {
        DBG_Getch();
    }

    return;
}

BOOL Intel3dTlc_CheckBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode)
{
    if (BLOCK_SLC == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType && FALSE == bsTLCMode)
    {
        return TRUE;
    }
    else if ((BLOCK_TLC == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType ||
        BLOCK_TLC_LOWPG == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType) && TRUE == bsTLCMode)
    {
        return TRUE;
    }

    if (BLOCK_INVALID == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType)
    {
        // read empty page or erase empty blk for SuperPu
        if (CMD_CODE_READ == ucCmdType || CMD_CODE_ERASE == ucCmdType)
        {
            return TRUE;
        }
    }
    DBG_Printf("Intel3dTlc_CheckBlkType: check Block type error, Lun %d, Pln %d, Block %d BlockType %d CmdType %d TLCMode %d \n",
        ucLun, ucPln, usBlk, g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType, ucCmdType, bsTLCMode);
    return FALSE;
}

BOOL Intel3dTlc_CheckErsBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode)
{
    if(BLOCK_TLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk)  &&
        (NF_PRCQ_ERS_MULTIPLN == ucCmdCode || NF_PRCQ_ERS == ucCmdCode))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL Intel3dTlc_CheckPrgOrder(PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucCmdCode;
    U8 ucLun, ucPln;
    U16 usBlk, usPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPage = (U16)pFlash_phy->nPage;
    ucCmdCode = pFlashCmdParam->bsCmdCode;

    if (BLOCK_SLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk))
    {
        if (usPage > (SLC_PG_PER_BLK - 1))
        {
            DBG_Printf("Intel3DTLC SLC block page %d page number overflow!\n", usPage);
            DBG_Getch();
        }

        if (usPage != l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
        {
            DBG_Printf("Intel3DTLC SLC block check program order error\n");
            DBG_Printf("Page %d is not equal to l_a3dTlcPrgPPO[%d].bsNextPrgPPO[%d][%d]: %d", usPage, ucLun, ucPln, usBlk, l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
            DBG_Getch();
        }
        else
        {
            l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] ++;
            l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;
        }
        return TRUE;
    }

    if (BLOCK_TLC == FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))
    {
        l_bLowPGMode[ucLun][ucPln][usBlk] = FALSE;
    }
    else if (BLOCK_TLC_LOWPG == FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))
    {
        l_bLowPGMode[ucLun][ucPln][usBlk] = TRUE;
    }
    /* check page number overflow */
    if (usPage > (PG_PER_BLK - 1))
    {
        DBG_Printf("Intel 3D TLC page %d program order overflow!\n", usPage);
        DBG_Getch();
    }

    /* check 3D-TLC program order */
    if (usPage != l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
    {
        DBG_Printf("PageNum %d is not equal to l_a3DTlcPrgPPO[%d].bsNextPrgPPO[%d][%d]: %d!\n", usPage, ucLun, ucPln, usBlk, l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
        DBG_Printf("Program order %d!\n", l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk]);
        DBG_Getch();
    }

    /* if second program upper page, check upper page data */
    if(FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        if(FALSE == Flash_TlcPrgDataCmp(pFlash_phy, pDataBuf, pRedBuf, nType))
        {
            DBG_Printf("Lun%d Pln%d Blk%d Upper page %d data compare error !!\n", pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, usPage);
            DBG_Getch();
        }
    }

    /* update next program page */
    if(FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] =  l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk];
    }
    else
    {
        if(FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(usPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
        {
            l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = Intel3dTlc_GetUpperPG(usPage);
        }
        else
        {
            if(TRUE == l_bLowPGMode[ucLun][ucPln][usBlk] )
            {
                /* For low page mode, get next checked page */
                 l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = Intel3dTlc_LowPgModeGetNextPG(usPage, pFlashCmdParam->bsPageInWL);
            }
            else
            {
                l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
            }
        }
    }

    /* update previous page and program order */
    if(FLASH_HIGH_PAGE != g_tFlashSpecInterface.GetPgType(l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk]++;

        if((TRUE == l_bLowPGMode[ucLun][ucPln][usBlk] ) &&
            (FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk))))
        {
            l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk]++;
        }
    }
    l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;

    return TRUE;

}

BOOL Intel3dTlc_ReadCheck(PFLASH_PHY pFlash_phy)
{
    return NF_SUCCESS;
}

BOOL Intel3dTlc_GetBlockMode(U8 ucCmdCode)
{
    if (NF_PRCQ_TLC_ERS_MULTIPLN <= ucCmdCode && NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG >= ucCmdCode)
    {
        return TRUE;
    }
    else if (NF_PRCQ_TLC_ERS <= ucCmdCode && NF_PRCQ_TLC_PRG_LOW_PG >= ucCmdCode)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void Intel3dTlc_InterfaceInit(FlashSpecInterface * ptFlashSpecInterface)
{
  ptFlashSpecInterface->GetCmdType = Intel3dTlc_GetCmdType;
  ptFlashSpecInterface->GetPgType = Intel3dTlc_GetPgType;
  ptFlashSpecInterface->GetPairPgType = Intel3dTlc_GetPairPgType;
  ptFlashSpecInterface->GetPairPage = Intel3dTlc_GetPairPage;
  ptFlashSpecInterface->SetBlkType = Intel3dTlc_SetBlkType;
  ptFlashSpecInterface->IncPairPgPrgStep = Intel3dTlc_IncPairPgPrgStep;
  ptFlashSpecInterface->CheckBlkType = Intel3dTlc_CheckBlkType;
  ptFlashSpecInterface->CheckErsType = Intel3dTlc_CheckErsBlkType;
  ptFlashSpecInterface->CheckPrgOrder = Intel3dTlc_CheckPrgOrder;
  ptFlashSpecInterface->ReadCheck = Intel3dTlc_ReadCheck;
  ptFlashSpecInterface->ResetPrgOrder = Intel3dTlc_ResetPrgOrder;
  ptFlashSpecInterface->GetWriteFlashAddr = Intel3dTlc_GetWriteFlashAddr;
  ptFlashSpecInterface->GetBlkMode = Intel3dTlc_GetBlockMode;
  ptFlashSpecInterface->GetPlnNum = Intel3dTlc_GetPlnNum;
  ptFlashSpecInterface->CheckTransferLen = Intel3dTlc_CheckTransferLen;
  return;
}
#endif
