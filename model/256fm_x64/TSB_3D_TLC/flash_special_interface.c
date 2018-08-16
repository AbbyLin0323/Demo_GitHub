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

#ifdef FLASH_TSB_3D
extern FLASH_RUNTIME_STATUS g_aFlashStatus;
extern BOOL Flash_TlcPrgDataCmp(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nType);
FLASH_PRG_ORDER l_aTsb3dTlcPrgPPO[NFC_MODEL_LUN_SUM] = { 0 };

U8 Tsb3dTlc_GetCmdType(U8 ucCmdCode)
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
    else if ((NF_PRCQ_TLC_READ_LP_MULTIPLN <= ucCmdCode && NF_PRCQ_TLC_READ_UP_MULTIPLN >= ucCmdCode)
            ||(NF_PRCQ_TLC_READ_LP <= ucCmdCode && NF_PRCQ_TLC_READ_UP >= ucCmdCode))
    {
        ucCmdType = CMD_CODE_READ;
    }
    else if ((NF_PRCQ_TLC_PRG_1ST_MULTIPLN == ucCmdCode) || (NF_PRCQ_TLC_PRG_1ST == ucCmdCode))
    {
        ucCmdType = CMD_CODE_PROGRAM;
    }
    else if ((NF_PRCQ_TLC_ERS_MULTIPLN == ucCmdCode)||(NF_PRCQ_TLC_ERS == ucCmdCode))
    {
        ucCmdType = CMD_CODE_ERASE;
    }
    else
    {
        DBG_Getch();
    }

    return ucCmdType;
}

U8 Tsb3dTlc_GetPgType(U16 usPage, U8 ucBlkType)
{
    return (usPage % 3);
}

U8 Tsb3dTlc_GetPairPgType(U16 usPairPage, U8 ucBlkType)
{
    return LOW_MID_HIGH_PAGE;
}

U8 Tsb3dTlc_GetPlnNum (U8 ucCmdCode)
{
    U8 ucPlnNum;

    if (NF_PRCQ_ERS <= ucCmdCode && NF_PRCQ_PRG >= ucCmdCode)
    {
        ucPlnNum = 1;
    }
    else if (NF_PRCQ_TLC_ERS <= ucCmdCode && NF_PRCQ_TLC_PRG_1ST >= ucCmdCode)
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

U16 Tsb3dTlc_GetPairPage(U16 usPage, U8 ucBlkType)
{
    return (usPage / 3);
}

void Tsb3dTlc_SetBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode)
{
    if (CMD_CODE_PROGRAM == ucCmdType &&  FALSE == bsTLCMode)
    {
        // set BlkType is SLC blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_SLC;
    }
    else if ((CMD_CODE_PROGRAM == ucCmdType &&  TRUE == bsTLCMode))
    {
        // set BlkType is TLC blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_TLC;
    }
    else
    {
        // set BlkType is Invalid Blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_INVALID;
    }

    return;
}

void Tsb3dTlc_IncPairPgPrgStep(PFLASH_PHY pFlash_phy)
{
    U8 ucLun, ucPln, ucPairPageType ;
    U16 usBlk, usPairPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPairPage = g_tFlashSpecInterface.GetPairPage(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));
    ucPairPageType = g_tFlashSpecInterface.GetPairPgType(usPairPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));

    if (BLOCK_SLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk))
    {
        g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = PAIR_PAGE_PRG_MAX;
    }

    else if(LOW_MID_HIGH_PAGE == ucPairPageType && FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        /* for TLC block */
        g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep++;
    }

    if (g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep > PAIR_PAGE_PRG_MAX)
    {
        DBG_Printf("Flash status: program step error\n");
        DBG_Getch();
    }

    return;
}

void Tsb3dTlc_ResetPrgOrder(U8 ucLun, U16 usBlk, U8 ucPln)
{
    l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = 0;
    l_aTsb3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = 0;

    return;
}

void Tsb3dTlc_GetWriteFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
{
    U8  pln, plun, rp = 0, ceSel = 0;
    U16 pbn, ppo;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U32 row_addr = pFlashCmdParam->phy_page_req[ulPlnIndex].row_addr;

    ppo = (row_addr & ((1 << PG_PER_BLK_BITS) - 1))  * PAGE_PER_WL + ulPageInWl;

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

void Tsb3dTlc_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
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
    else if (NF_PRCQ_TLC_PRG_1ST == ucCmdCode || NF_PRCQ_TLC_PRG_1ST_MULTIPLN == ucCmdCode)
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * CW_INFO_SZ;
    }
    else if ((NF_PRCQ_TLC_READ_LP <= ucCmdCode && NF_PRCQ_TLC_READ_UP >= ucCmdCode) ||
        (NF_PRCQ_TLC_READ_LP_MULTIPLN <= ucCmdCode && NF_PRCQ_TLC_READ_UP_MULTIPLN >= ucCmdCode))
    {
        ulTransferLen = pNfcqEntry->bsDmaTotalLength * SEC_SIZE;
    }

    /*Check Transfer Length*/
    if ((NF_PRCQ_PRG == ucCmdCode) && (ulTransferLen != g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_READ == ucCmdCode) && (ulTransferLen > g_PG_SZ))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_READ_MULTIPLN == ucCmdCode) && (ulTransferLen > (g_PG_SZ * PLN_PER_LUN)))
    {
        DBG_Getch();
    }

    //TLC write cmd check
    if ((NF_PRCQ_TLC_PRG_1ST == ucCmdCode) && (ulTransferLen != g_PG_SZ * INTRPG_PER_PGADDR))
    {
        DBG_Getch();
    }
    else if ((NF_PRCQ_TLC_PRG_1ST_MULTIPLN == ucCmdCode) && (ulTransferLen != g_PG_SZ * INTRPG_PER_PGADDR * PLN_PER_LUN))
    {
        DBG_Getch();
    }

    //TLC Read cmd check
    if (((NF_PRCQ_TLC_READ_LP <= ucCmdCode) && (NF_PRCQ_TLC_READ_UP >= ucCmdCode)) && (ulTransferLen > g_PG_SZ))
    {
        DBG_Getch();
    }
    else if (((NF_PRCQ_TLC_READ_LP_MULTIPLN <= ucCmdCode) && (NF_PRCQ_TLC_READ_UP_MULTIPLN >= ucCmdCode)) && (ulTransferLen > g_PG_SZ * PLN_PER_LUN))
    {
        DBG_Getch();
    }

    return;
}

BOOL Tsb3dTlc_CheckBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode)
{
    if (BLOCK_SLC == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType && FALSE == bsTLCMode)
    {
        return TRUE;
    }
    else if (BLOCK_TLC == g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType && TRUE == bsTLCMode)
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

BOOL Tsb3dTlc_CheckErsBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode)
{
    if (BLOCK_TLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk) &&
        (NF_PRCQ_ERS_MULTIPLN == ucCmdCode || NF_PRCQ_ERS == ucCmdCode))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL Tsb3dTlc_CheckPrgOrder(PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucLun, ucPln;
    U16 usBlk, usPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPage = (U16)pFlash_phy->nPage;

    /* check page number over range */
    if (BLOCK_SLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk))
    {
        if (usPage >= PG_PER_BLK * INTRPG_PER_PGADDR)
        {
            DBG_Printf("Tsb3DTLC SLC block page %d page number overflow!\n", usPage);
            DBG_Getch();
        }

        if (usPage != l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
        {
            DBG_Printf("Tsb3DTLC SLC block check program order error\n");
            DBG_Printf("Page %d is not equal to l_aTsb3dTlcPrgPPO[%d].bsNextPrgPPO[%d][%d]: %d", usPage, ucLun, ucPln, usBlk, l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
            DBG_Getch();
        }
        else
        {
            l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] += 3;
            l_aTsb3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;
        }
    }

    else if (BLOCK_TLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk))
    {
        if (usPage >= PG_PER_BLK * INTRPG_PER_PGADDR)
        {
            DBG_Printf("Tsb3DTLC TLC block page %d page number overflow!\n", usPage);
            DBG_Getch();
        }

        if (usPage != l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
        {
            DBG_Printf("Tsb3dTlc TLC block check Program order error\n");
            DBG_Printf("Page %d is not equal to l_aTsb3dTlcPrgPPO[%d].bsNextPrgPPO[%d][%d]: %d", usPage, ucLun, ucPln, usBlk, l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
            DBG_Getch();
        }
        else
        {
            l_aTsb3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
            l_aTsb3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;
        }
    }

    return TRUE;
}

BOOL Tsb3dTlc_ReadCheck(PFLASH_PHY pFlash_phy)
{
    return NF_SUCCESS;
}

BOOL Tsb3dTlc_GetBlockMode(U8 ucCmdCode)
{
#if 1
    if (NF_PRCQ_TLC_ERS_MULTIPLN == ucCmdCode
     || NF_PRCQ_TLC_READ_LP_MULTIPLN == ucCmdCode
     || NF_PRCQ_TLC_READ_MP_MULTIPLN == ucCmdCode
     || NF_PRCQ_TLC_READ_UP_MULTIPLN == ucCmdCode
     || NF_PRCQ_TLC_PRG_1ST_MULTIPLN == ucCmdCode
     || NF_PRCQ_TLC_ERS == ucCmdCode
     || NF_PRCQ_TLC_READ_LP == ucCmdCode
     || NF_PRCQ_TLC_READ_MP == ucCmdCode
     || NF_PRCQ_TLC_READ_UP == ucCmdCode
     || NF_PRCQ_TLC_PRG_1ST == ucCmdCode)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    if (NF_PRCQ_TLC_ERS_MULTIPLN <= ucCmdCode && NF_PRCQ_TLC_PRG_1ST_MULTIPLN >= ucCmdCode)
    {
        return TRUE;
    }
    else if (NF_PRCQ_TLC_ERS <= ucCmdCode && NF_PRCQ_TLC_PRG_1ST >= ucCmdCode)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif
}

void Tsb3dTlc_InterfaceInit(FlashSpecInterface * ptFlashSpecInterface)
{
    ptFlashSpecInterface->GetCmdType = Tsb3dTlc_GetCmdType;
    ptFlashSpecInterface->GetPgType = Tsb3dTlc_GetPgType;
    ptFlashSpecInterface->GetPairPgType = Tsb3dTlc_GetPairPgType;
    ptFlashSpecInterface->GetPairPage = Tsb3dTlc_GetPairPage;
    ptFlashSpecInterface->SetBlkType = Tsb3dTlc_SetBlkType;
    ptFlashSpecInterface->IncPairPgPrgStep = Tsb3dTlc_IncPairPgPrgStep;
    ptFlashSpecInterface->CheckBlkType = Tsb3dTlc_CheckBlkType;
    ptFlashSpecInterface->CheckErsType = Tsb3dTlc_CheckErsBlkType;
    ptFlashSpecInterface->CheckPrgOrder = Tsb3dTlc_CheckPrgOrder;
    ptFlashSpecInterface->ReadCheck = Tsb3dTlc_ReadCheck;
    ptFlashSpecInterface->ResetPrgOrder = Tsb3dTlc_ResetPrgOrder;
    ptFlashSpecInterface->GetWriteFlashAddr = Tsb3dTlc_GetWriteFlashAddr;
    ptFlashSpecInterface->GetBlkMode = Tsb3dTlc_GetBlockMode;
    ptFlashSpecInterface->GetPlnNum = Tsb3dTlc_GetPlnNum;
    ptFlashSpecInterface->CheckTransferLen = Tsb3dTlc_CheckTransferLen;
    return;
}
#endif