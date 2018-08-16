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

#ifdef FLASH_IM_3DTLC_GEN2
extern FLASH_RUNTIME_STATUS g_aFlashStatus;
extern BOOL Flash_TlcPrgDataCmp(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nType);
FLASH_PRG_ORDER l_aIM3dTlcGen2_[NFC_MODEL_LUN_SUM] = { 0 };
LOCAL BOOL l_bIMLowPGMode[NFC_MODEL_LUN_SUM][PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN]  = {0};

extern BOOL g_bSquashMode;

//#define INTEL_3DTLC_MAX_PG_NUM 1535


U16 IM3dTlcGen2_GetUpperPG(U16 usExtraPg)
{
	ASSERT(FLASH_MID_PAGE == g_tFlashSpecInterface.GetPgType(usExtraPg, FALSE));

	return usExtraPg + 1;
}

U16 IM3dTlcGen2_LowPgModeGetNextPG(U16 usPageNum, U8 ucPageInWL)
{
	ASSERT(usPageNum >= 0 && usPageNum < 2304);

    U16 ulNextPgNum;
    
    if (usPageNum < 12)
    {
        ulNextPgNum = usPageNum + 1;
    }
    else if (usPageNum >= 12 && usPageNum < 36)
    {
        ulNextPgNum = usPageNum + 2;
    }
    else if (usPageNum >= 36 && usPageNum < 59)
	{
		ulNextPgNum = usPageNum + 1;
	}
	else if (usPageNum >= 59 && usPageNum < 2222)
    {
        ulNextPgNum = usPageNum + 3;
    } 
	else if (usPageNum >= 2222 && usPageNum < 2266)
	{
		ulNextPgNum = usPageNum + 4;
	}
	else if (usPageNum == 2266)
	{
		ulNextPgNum = 2292;
	}
    else if (usPageNum >= 2292)
    {
        ulNextPgNum = usPageNum + 1;
    }

    return ulNextPgNum;
}

U8 IM3dTlcGen2_GetCmdType(U8 ucCmdCode)
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

    /*TLC wirte cmd*/
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

U8 IM3dTlcGen2_GetPgType(U16 usPage, U8 ucBlkType)
{
	ASSERT(usPage >= 0 && usPage < 2304);

    U8 ucPageType;

    if (BLOCK_SLC == ucBlkType)
    {
        return FLASH_LOW_PAGE;
    }

    if (usPage < 12)
    {
        ucPageType = FLASH_LOW_PAGE;
    }

    if ((usPage >= 12) && (usPage < 36))
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

	if ((usPage >= 36) && (usPage < 60))
	{
		ucPageType = FLASH_LOW_PAGE;
	}

    if ((usPage >= 60) && (usPage < 2220))
    {
        if (2 == usPage % 3)
        {
			ucPageType = FLASH_LOW_PAGE;
        }
		else if (1 == usPage % 3)
        {
			ucPageType = FLASH_MID_PAGE;
        }
        else
        {
			ucPageType = FLASH_HIGH_PAGE;
        }
    }

	if ((usPage >= 2220) && (usPage < 2269))
    {
		if (3 == usPage % 4)
		{
			ucPageType = FLASH_MID_PAGE;
		}
		else if (2 == usPage % 4)
		{
			ucPageType = FLASH_LOW_PAGE;
		}
		else if (1 == usPage % 4)
		{
			ucPageType = FLASH_MID_PAGE;
		}
		else
		{
			ucPageType = FLASH_HIGH_PAGE;
		}
    }

	if ((usPage >= 2269) && (usPage < 2292))
	{
		if (0 == usPage % 2)
		{
			ucPageType = FLASH_HIGH_PAGE;
		}
		else
		{
			ucPageType = FLASH_MID_PAGE;
		}

	}

	if (usPage >= 2292)
	{
		ucPageType = FLASH_LOW_PAGE;
	}

    return ucPageType;
}

U8 IM3dTlcGen2_GetPairPgType(U16 usPairPage, U8 ucBlkType)
{
	ASSERT(usPairPage >= 0 && usPairPage < 792);

    U8 ucPairPgType;

    if (BLOCK_SLC == ucBlkType)
    {
        return LOW_PAGE_ONLY;
    }
    
    if (usPairPage < 12 || usPairPage > 779)
    {
        ucPairPgType = LOW_PAGE_ONLY;
    }
	else if ((usPairPage >= 12 && usPairPage < 24) || (usPairPage >= 768 && usPairPage < 780))
    {
        ucPairPgType = LOW_MID_PAGE; 
    }
    else
    {
        ucPairPgType = LOW_MID_HIGH_PAGE;
    }
    
    return ucPairPgType;
}

U8 IM3dTlcGen2_GetPlnNum (U8 ucCmdCode)
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


U16 IM3dTlcGen2_GetPairPage(U16 usPage, U8 ucBlkType)
{
	ASSERT(usPage >= 0 && usPage < 2304);

    U16 usUpPgNum = INVALID_4F;
    U16 usPairPgNum;
    
    if (BLOCK_SLC == ucBlkType)
    {
        return usPage;
    }

    /*Get pair page for TLC block*/
    if (usPage < 12)
    {
        usPairPgNum = usPage;
    }
    else if (usPage >= 12 && usPage < 36)
    {
		usPairPgNum = (usPage + 12) / 2;
    }
	else if (usPage >= 36 && usPage < 60)
	{
		usPairPgNum = usPage - 12;
	}
	else if ((usPage >= 60) && (usPage < 2220))
	{
		if (FLASH_LOW_PAGE == g_tFlashSpecInterface.GetPgType(usPage, ucBlkType))
		{
			usPairPgNum = (usPage - 62) / 3 + 48;
		}
		else
		{
			usPairPgNum = (usPage - 60) / 3 + 24;
		}
	}
	else if ((usPage >= 2220) && (usPage < 2268))
	{
		if ((usPage % 4) < 2)
		{
			usPairPgNum = (usPage - 2220) / 4 + 744;
		}
		else if ((usPage % 4) >= 2)
		{
			usPairPgNum = (usPage - 2222) / 4 + 768;
		}
	}
	else if ((usPage >= 2268) && (usPage < 2292))
	{
		usPairPgNum = (usPage - 2268) / 2 + 756;
	}
	else
	{
		usPairPgNum = usPage - 1512;
	}

    return usPairPgNum;
}

void IM3dTlcGen2_SetBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode)
{
    if (CMD_CODE_PROGRAM == ucCmdType && TRUE == bsTLCMode)
    {
        // set BlkType is TLC_LOWPG blk
        g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_TLC_LOWPG;
        
        // if write page 13 (upper page), set BlkType to TLC blk
        if (13 == usPage)
        {
            g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType = BLOCK_TLC;
            l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk] = 13;
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

void IM3dTlcGen2_IncPairPgPrgStep(PFLASH_PHY pFlash_phy)
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
	else if (LOW_MID_PAGE == ucPairPageType)
	{
		if (FLASH_MID_PAGE == g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
		{
			g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = PAIR_PAGE_PRG_MAX;
		}
	}
	else
    {
		if (FLASH_HIGH_PAGE != g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
		{
			g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep++;
		}
    }
    
    if (g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep > PAIR_PAGE_PRG_MAX)
    {
        DBG_Printf("Flash status: program step error\n");
        DBG_Getch();
    }
    
    return;
}

void IM3dTlcGen2_ResetPrgOrder(U8 ucLun, U16 usBlk, U8 ucPln)
{
    l_aIM3dTlcGen2_[ucLun].bsPrgOrder[ucPln][usBlk] = 0;
    l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk] = 0;
    l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk] = 0;

    l_bIMLowPGMode[ucLun][ucPln][usBlk]  = FALSE;
    return;
}

void IM3dTlcGen2_GetWriteFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
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

void IM3dTlcGen2_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
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

BOOL IM3dTlcGen2_CheckBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode)
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
    DBG_Printf("IM3dTlcGen2_CheckBlkType: check Block type error, Lun %d, Pln %d, Block %d BlockType %d CmdType %d TLCMode %d \n",
        ucLun, ucPln, usBlk, g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType, ucCmdType, bsTLCMode);
    return FALSE;
}

BOOL IM3dTlcGen2_CheckErsBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode)
{
    if(BLOCK_TLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk)  && 
        (NF_PRCQ_ERS_MULTIPLN == ucCmdCode || NF_PRCQ_ERS == ucCmdCode))
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL IM3dTlcGen2_CheckPrgOrder(PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *pFlashCmdParam)
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

        if (usPage != l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk])
        {
            DBG_Printf("Intel3DTLC SLC block check program order error\n");
            DBG_Printf("Page %d is not equal to l_aIM3dTlcGen2_[%d].bsNextPrgPPO[%d][%d]: %d", usPage, ucLun, ucPln, usBlk, l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk]);
            DBG_Getch();
        }
        else
        {
            l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk] ++;
            l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;
        }
        return TRUE;
    }

    if (BLOCK_TLC == FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))
    {
        l_bIMLowPGMode[ucLun][ucPln][usBlk] = FALSE;
    }
    else if (BLOCK_TLC_LOWPG == FlashStsM_GetBlkType(pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock))
    {
        l_bIMLowPGMode[ucLun][ucPln][usBlk] = TRUE;   
    }
    /* check page number overflow */
    if (usPage > (PG_PER_BLK - 1))
    {       
        DBG_Printf("Intel 3D TLC page %d program order overflow!\n", usPage);
        DBG_Getch();
    }

    /* check 3D-TLC program order */
    if (usPage != l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk])
    {
        DBG_Printf("PageNum %d is not equal to l_aIM3dTlcGen2_[%d].bsNextPrgPPO[%d][%d]: %d!\n", usPage, ucLun, ucPln, usBlk, l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk]);
        DBG_Printf("Program order %d!\n", l_aIM3dTlcGen2_[ucLun].bsPrgOrder[ucPln][usBlk]);
        DBG_Getch();
    }

    /* if second program upper page, check upper page data */
    //if(FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    //{
    //    if(FALSE == Flash_TlcPrgDataCmp(pFlash_phy, pDataBuf, pRedBuf, nType))
    //    {
    //        DBG_Printf("Lun%d Pln%d Blk%d Upper page %d data compare error !!\n", pFlash_phy->ucLunInTotal, pFlash_phy->nPln, pFlash_phy->nBlock, usPage);
    //        DBG_Getch();
    //    }
    //}

    /* update next program page */
	if (FLASH_LOW_PAGE == g_tFlashSpecInterface.GetPgType(l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
	{
		if (TRUE == l_bIMLowPGMode[ucLun][ucPln][usBlk])
		{
			/* For low page mode, get next checked page */
			l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk] = IM3dTlcGen2_LowPgModeGetNextPG(usPage, pFlashCmdParam->bsPageInWL);
		}
		else
		{
			l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
		}
	}
	else if (FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
	{
		l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk] = IM3dTlcGen2_GetUpperPG(usPage);
	}
	else
	{
		l_aIM3dTlcGen2_[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
	}

    /* update previous page and program order */
    if(FLASH_HIGH_PAGE != g_tFlashSpecInterface.GetPgType(l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {        
        l_aIM3dTlcGen2_[ucLun].bsPrgOrder[ucPln][usBlk]++;

        if((TRUE == l_bIMLowPGMode[ucLun][ucPln][usBlk] ) && 
            (FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(l_aIM3dTlcGen2_[ucLun].bsPrgOrder[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk))))
        {
            l_aIM3dTlcGen2_[ucLun].bsPrgOrder[ucPln][usBlk]++;
        }
    }   
    l_aIM3dTlcGen2_[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;

    return TRUE;
    
}

BOOL IM3dTlcGen2_ReadCheck(PFLASH_PHY pFlash_phy)
{
    return NF_SUCCESS;
}

BOOL IM3dTlcGen2_GetBlockMode(U8 ucCmdCode)
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

void IM3dTlcGen2_InterfaceInit(FlashSpecInterface * ptFlashSpecInterface)
{
    ptFlashSpecInterface->GetCmdType = IM3dTlcGen2_GetCmdType;
    ptFlashSpecInterface->GetPgType = IM3dTlcGen2_GetPgType;
    ptFlashSpecInterface->GetPairPgType = IM3dTlcGen2_GetPairPgType;
    ptFlashSpecInterface->GetPairPage = IM3dTlcGen2_GetPairPage;
    ptFlashSpecInterface->SetBlkType = IM3dTlcGen2_SetBlkType;
    ptFlashSpecInterface->IncPairPgPrgStep = IM3dTlcGen2_IncPairPgPrgStep;
    ptFlashSpecInterface->CheckBlkType = IM3dTlcGen2_CheckBlkType;
    ptFlashSpecInterface->CheckErsType = IM3dTlcGen2_CheckErsBlkType;
    ptFlashSpecInterface->CheckPrgOrder = IM3dTlcGen2_CheckPrgOrder;
    ptFlashSpecInterface->ReadCheck = IM3dTlcGen2_ReadCheck;
    ptFlashSpecInterface->ResetPrgOrder = IM3dTlcGen2_ResetPrgOrder;
    ptFlashSpecInterface->GetWriteFlashAddr = IM3dTlcGen2_GetWriteFlashAddr;
    ptFlashSpecInterface->GetBlkMode = IM3dTlcGen2_GetBlockMode;
    ptFlashSpecInterface->GetPlnNum = IM3dTlcGen2_GetPlnNum;
    ptFlashSpecInterface->CheckTransferLen = IM3dTlcGen2_CheckTransferLen;
    return;
}
#endif
