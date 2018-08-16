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

#include "BaseDef.h"
#include "sim_flash_shedule.h"
#include "flash_meminterface.h"
#include <flashmodel/sim_flash_status.h>

#ifdef FLASH_L95
extern FLASH_RUNTIME_STATUS g_aFlashStatus;
FLASH_PRG_ORDER l_aL95PrgPPO[NFC_MODEL_LUN_SUM] = { 0 };

U8 L95_GetCmdType(U8 ucCmdCode)
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

    return ucCmdType;
}

U8 L95_GetPgType(U16 usPage, U8 ucBlkType)
{
    U8 ucPageType;

    if (usPage < 10)
    {
        if (6 == usPage || 9 == usPage)
        {
            ucPageType = FLASH_MID_PAGE;
        }
        else
        {
            ucPageType = FLASH_LOW_PAGE;
        }
    }

    else if (usPage >= 10 && usPage < 509)
    {
        if (0 != (usPage % 4) / 2)
        {
            ucPageType = FLASH_LOW_PAGE;
        }
        else
        {
            ucPageType = FLASH_MID_PAGE;
        }
    }
    else
    {
        if ((PG_PER_BLK - 1) == usPage)
        {
            ucPageType = FLASH_MID_PAGE;
        }
        else
        {
            ucPageType = FLASH_LOW_PAGE;
        }
    }
    return ucPageType;
}

U8 L95_GetPairPgType(U16 usPairPage, U8 ucBlkType)
{
    U8 ucPairPgType;

    if (usPairPage < 5 || usPairPage > 253)
    {
        if (1 == usPairPage || 255 == usPairPage || 257 == usPairPage)
        {
            ucPairPgType = LOW_MID_PAGE;
        }
        else
        {
            ucPairPgType = LOW_PAGE_ONLY;
        }
    }
    else
    {
        ucPairPgType = LOW_MID_PAGE;
    }

    return ucPairPgType;
}

U8 L95_GetPlnNum (U8 ucCmdCode)
{
    U8 ucPlnNum;

    if (NF_PRCQ_ERS <= ucCmdCode && NF_PRCQ_PRG >= ucCmdCode)
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


U16 L95_GetPairPage(U16 usPage, U8 ucBlkType)
{
    U16 usPairPgNum;
    U16 usLowPgNum = INVALID_4F;

    /* Get low page number to obtain pair page number */
    if (FLASH_MID_PAGE == g_tFlashSpecInterface.GetPgType (usPage, BLOCK_MLC))
    {
        if (9 == usPage || 511 == usPage)
        {
            usLowPgNum = usPage - 4;
        }
        else if (6 == usPage || 12 == usPage || 13 == usPage || 508 == usPage)
        {
            usLowPgNum = usPage - 5;
        }
        else
        {
            usLowPgNum = usPage - PAIR_PAGE_INTERVAL_MAX;
        }
    }
    else
    {
        usLowPgNum = usPage;
    }

    if (FLASH_LOW_PAGE != g_tFlashSpecInterface.GetPgType(usLowPgNum, BLOCK_MLC))
    {
        DBG_Printf("L95 lowPgNum get type error\n");
        DBG_Getch();
    }

    /* Get corresponding pair page number */
    if (usLowPgNum < 6)
    {
        usPairPgNum = usLowPgNum;
    }
    else if (usLowPgNum >= 6 && usLowPgNum < 10)
    {
        usPairPgNum = usLowPgNum -1;
    }
    else
    {
        usPairPgNum = 8 + ((usLowPgNum - 10) / 2) + ((usLowPgNum - 10) % 2);

        if (509 == usLowPgNum)
        {
            usPairPgNum = PAIR_PAGE_PER_BLK - 1;
        }
    }

    return usPairPgNum;
}

void L95_SetBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode)
{
    if (CMD_CODE_PROGRAM == ucCmdType &&  FALSE == bsTLCMode)
    {
        // set BlkType is MLC blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_MLC;
    }
    else
    {
        // set BlkType is invalid blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_INVALID;
    }

    return;
}

void L95_IncPairPgPrgStep(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln, ucPairPageType;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, BLOCK_MLC);
    ucPairPageType = g_tFlashSpecInterface.GetPairPgType(usPairPage, BLOCK_MLC);

    if (LOW_PAGE_ONLY == ucPairPageType)
    {
        if (FLASH_LOW_PAGE != g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, BLOCK_MLC))
        {
            DBG_Printf("Page number %d error,  on pair page %d with low page only\n", pFlash_phy->nPage, usPairPage);
            DBG_Getch();
        }
        g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = PAIR_PAGE_PRG_MAX;
    }

    if (LOW_MID_PAGE == ucPairPageType && FLASH_MID_PAGE == g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, BLOCK_MLC))
    {
        g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep++;
    }

    if (g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep > PAIR_PAGE_PRG_MAX)
    {
        DBG_Printf("Flash status: program step error\n");
        DBG_Getch();
    }

    return;
}

void L95_ResetPrgOrder(U8 ucLun, U16 usBlk, U8 ucPln)
{
    l_aL95PrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = 0;
    l_aL95PrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = 0;
    return;
}

void L95_GetWriteFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
{
    U8  pln, plun, rp = 0, ceSel = 0;
    U16 pbn, ppo;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U32 row_addr = pFlashCmdParam->phy_page_req[ulPlnIndex].row_addr;

    ppo = (row_addr & ((1 << PG_PER_BLK_BITS) - 1)) * PAGE_PER_WL + ulPageInWl;

    pbn = ((row_addr >> BLK_POS_IN_ROW_ADDR) & BLK_PER_PLN_MSK);
    pln = ((row_addr >> PLN_POS_IN_ROW_ADDR) & PLN_PER_LUN_MSK);

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

void L95_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
{
    U32 ulTransferLen = 0;

    /*Set Transfer Length*/
    if ((NF_PRCQ_PRG == ucCmdCode) || (NF_PRCQ_PRG_MULTIPLN == ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * CW_INFO_SZ;
    }
    else if ((NF_PRCQ_READ == ucCmdCode) || (NF_PRCQ_READ_MULTIPLN == ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * SEC_SIZE;
    }

    /*Check Transfer Length*/
    if ((NF_PRCQ_PRG == ucCmdCode) && (ulTransferLen != g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_PRG_MULTIPLN == ucCmdCode) && (ulTransferLen != (g_PG_SZ * PLN_PER_LUN)))
    {
        DBG_Getch();
    }

    if ((NF_PRCQ_READ == ucCmdCode) && (ulTransferLen > g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_READ_MULTIPLN == ucCmdCode) && (ulTransferLen > (g_PG_SZ * PLN_PER_LUN)))
    {
        DBG_Getch();
    }

    return;
}


BOOL L95_CheckBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode)
{
    if (BLOCK_MLC == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType)
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

    return FALSE;
}

BOOL L95_CheckErsBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode)
{
    if (BLOCK_MLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk) &&
        (NF_PRCQ_ERS_MULTIPLN != ucCmdCode && NF_PRCQ_ERS != ucCmdCode))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL L95_CheckPrgOrder(PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPage = (U16)pFlash_phy->nPage;

    if (usPage >= PG_PER_BLK)
    {
        DBG_Printf("Flash L95 page %d program order overflow!\n", usPage);
        DBG_Getch();
    }

    if (usPage != l_aL95PrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
    {
        DBG_Printf("Flash L95 check Program order error\n");
        DBG_Printf("Page %d is not equal to l_aL95PrgPPO[%d].bsNextPrgPPO[%d][%d]: %d", usPage, ucLun, ucPln, usBlk, l_aL95PrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
        DBG_Getch();
    }
    else
    {
        l_aL95PrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
        l_aL95PrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;
    }

    return TRUE;
}


BOOL L95_ReadCheck(PFLASH_PHY pFlash_phy)
{
    return NF_SUCCESS;
}

void L95_InterfaceInit(FlashSpecInterface * ptFlashSpecInterface)
{
  ptFlashSpecInterface->GetCmdType       = L95_GetCmdType;
  ptFlashSpecInterface->GetPgType        = L95_GetPgType;
  ptFlashSpecInterface->GetPairPgType    = L95_GetPairPgType;
  ptFlashSpecInterface->GetPairPage      = L95_GetPairPage;
  ptFlashSpecInterface->SetBlkType       = L95_SetBlkType;
  ptFlashSpecInterface->IncPairPgPrgStep = L95_IncPairPgPrgStep;
  ptFlashSpecInterface->CheckBlkType     = L95_CheckBlkType;
  ptFlashSpecInterface->CheckErsType     = L95_CheckErsBlkType;
  ptFlashSpecInterface->CheckPrgOrder    = L95_CheckPrgOrder;
  ptFlashSpecInterface->ReadCheck        = L95_ReadCheck;
  ptFlashSpecInterface->ResetPrgOrder    = L95_ResetPrgOrder;
  ptFlashSpecInterface->GetWriteFlashAddr = L95_GetWriteFlashAddr;
  ptFlashSpecInterface->GetPlnNum = L95_GetPlnNum;
  ptFlashSpecInterface->CheckTransferLen = L95_CheckTransferLen;

  return;
}
#endif
