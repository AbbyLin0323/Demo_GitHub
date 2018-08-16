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

#if (defined(FLASH_TSB) && !defined(FLASH_3D_MLC))

extern FLASH_RUNTIME_STATUS g_aFlashStatus;
FLASH_PRG_ORDER l_aTsb4PlnPrgPPO[NFC_MODEL_LUN_SUM] = { 0 };

U8 Tsb4Pln_GetCmdType(U8 ucCmdCode)
{
    U8 ucCmdType = CMD_CODE_OTHER;

    if(NF_PRCQ_RETRY_EN == ucCmdCode)
    {
        ucCmdType = CMD_CODE_READRETRY_EN;
    }
    else if(((ucCmdCode >= NF_CMD_READRETRY_ADJ_START) && (ucCmdCode <= NF_CMD_READRETRY_ADJ_END))
    || ((ucCmdCode >= NF_CMD_SHIFTREAD_ADJ_START) && (ucCmdCode <= NF_CMD_SHIFTREAD_ADJ_END)))
    {
        ucCmdType = CMD_CODE_READRETRY_ADJ;
    }

    return ucCmdType;
}

U8 Tsb4Pln_GetPgType(U16 usPage, U8 ucBlkType)
{
    U8 ucPageType;

    if (0 == usPage)
    {
        ucPageType = FLASH_LOW_PAGE;
    }
    else if ((PG_PER_BLK-1) == usPage)
    {
        ucPageType = FLASH_MID_PAGE;
    }
    else
    {
        ucPageType = (0 == (usPage % 2)) ? FLASH_MID_PAGE : FLASH_LOW_PAGE;
    }

    return ucPageType;
}

U8 Tsb4Pln_GetPairPgType(U16 usPairPage, U8 ucBlkType)
{
    return LOW_MID_PAGE;
}

U8 Tsb4Pln_GetPlnNum (U8 ucCmdCode)
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

U16 Tsb4Pln_GetPairPage(U16 usPage, U8 ucBlkType)
{
    U16 usPairPgNum;

    if(0 == usPage || (PG_PER_BLK-1) == usPage)
    {
        usPairPgNum = usPage / 2;
    }
    else
    {
        if (0 == (usPage % 2))
        {
            usPairPgNum = usPage / 2 - 1;
        }
        else
        {
            usPairPgNum = usPage / 2 + 1;
        }
    }
    return usPairPgNum;
}

void Tsb4Pln_SetBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode)
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

void Tsb4Pln_IncPairPgPrgStep(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln, ucPairPageType ;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, BLOCK_MLC);
    ucPairPageType = g_tFlashSpecInterface.GetPairPgType(usPairPage, BLOCK_MLC);

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

void Tsb4Pln_ResetPrgOrder(U8 ucLun, U16 usBlk, U8 ucPln)
{
    l_aTsb4PlnPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = 0;
    l_aTsb4PlnPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = 0;
    return;
}

void Tsb4Pln_GetWriteFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
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

void Tsb4Pln_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
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

BOOL Tsb4Pln_CheckBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode)
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

BOOL Tsb4Pln_CheckErsBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode)
{
   if(BLOCK_MLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk)  &&
        (NF_PRCQ_ERS_MULTIPLN != ucCmdCode && NF_PRCQ_ERS != ucCmdCode))
   {
       return FALSE;
   }

   return TRUE;
}

BOOL Tsb4Pln_CheckPrgOrder(PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPage = (U16)pFlash_phy->nPage;

    if (usPage >= PG_PER_BLK)
    {
        DBG_Printf("Flash Tsb4Pln page %d program order overflow!\n", usPage);
        DBG_Getch();
    }

    if (usPage != l_aTsb4PlnPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
    {
        DBG_Printf("Flash Tsb4Pln check Program order error\n");
        DBG_Printf("Page %d is not equal to l_aTsb4PlnPrgPPO[%d].bsNextPrgPPO[%d][%d]: %d", usPage, ucLun, ucPln, usBlk, l_aTsb4PlnPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
        DBG_Getch();
    }
    else
    {
        l_aTsb4PlnPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
        l_aTsb4PlnPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;
    }

    return TRUE;
}

BOOL Tsb4Pln_ReadCheck(PFLASH_PHY pFlash_phy)
{
    return NF_SUCCESS;
}

void Tsb4Pln_InterfaceInit(FlashSpecInterface * ptFlashSpecInterface)
{
  ptFlashSpecInterface->GetCmdType       = Tsb4Pln_GetCmdType;
  ptFlashSpecInterface->GetPgType        = Tsb4Pln_GetPgType;
  ptFlashSpecInterface->GetPairPgType    = Tsb4Pln_GetPairPgType;
  ptFlashSpecInterface->GetPairPage      = Tsb4Pln_GetPairPage;
  ptFlashSpecInterface->SetBlkType       = Tsb4Pln_SetBlkType;
  ptFlashSpecInterface->IncPairPgPrgStep = Tsb4Pln_IncPairPgPrgStep;
  ptFlashSpecInterface->CheckBlkType     = Tsb4Pln_CheckBlkType;
  ptFlashSpecInterface->CheckErsType     = Tsb4Pln_CheckErsBlkType;
  ptFlashSpecInterface->CheckPrgOrder    = Tsb4Pln_CheckPrgOrder;
  ptFlashSpecInterface->ReadCheck        = Tsb4Pln_ReadCheck;
  ptFlashSpecInterface->ResetPrgOrder    = Tsb4Pln_ResetPrgOrder;
  ptFlashSpecInterface->GetWriteFlashAddr = Tsb4Pln_GetWriteFlashAddr;
  ptFlashSpecInterface->GetPlnNum = Tsb4Pln_GetPlnNum;
  ptFlashSpecInterface->CheckTransferLen = Tsb4Pln_CheckTransferLen;
  return;
}
#endif