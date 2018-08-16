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

#ifdef FLASH_MICRON_3DTLC_B16
extern FLASH_RUNTIME_STATUS g_aFlashStatus;
extern BOOL Flash_TlcPrgDataCmp(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nType);
FLASH_PRG_ORDER l_a3dTlcPrgPPO[NFC_MODEL_LUN_SUM] = { 0 };

extern BOOL g_bSquashMode;

#define MICRON_3DTLC_B16_MAX_PG_NUM 2303


U16 Micron3dTlcB16_GetUpperPG(U16 usExtraPg)
{
    return (++usExtraPg);
}

U8 Micron3dTlcB16_GetCmdType(U8 ucCmdCode) 
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

    else if ((NF_PRCQ_TLC_READ == ucCmdCode) || (NF_PRCQ_TLC_READ_MULTIPLN == ucCmdCode))
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

U8 Micron3dTlcB16_GetPgType(U16 usPage, U8 ucBlkType)
{
    U8 ucPageType;

    if (BLOCK_SLC == ucBlkType)
    {
        return FLASH_LOW_PAGE;
    }

    if ((usPage < 12)||(usPage >= 2292))//page 0~12,2292~2303
    {
        ucPageType = FLASH_LOW_PAGE;
    }
    else if (usPage < 36)                    //page 12~35
    {
        if (0 == usPage%2)
            ucPageType = FLASH_LOW_PAGE;
        else
            ucPageType = FLASH_MID_PAGE;
    }
    else if (usPage < 59)                    //page 36~58
    {
        ucPageType = FLASH_LOW_PAGE;
    }
    else if (usPage < 2222)                  //page 59~2221
    {
        if (0 == (usPage - 59) % 3)     
            ucPageType = FLASH_LOW_PAGE;
        else if (1 == (usPage - 59) % 3)
            ucPageType = FLASH_HIGH_PAGE;
        else
            ucPageType = FLASH_MID_PAGE;
    }
    else if (usPage < 2268)                  //page 2222~2267
    {
        if (0 == (usPage - 2222) % 4)     
            ucPageType = FLASH_LOW_PAGE;
        else if (2 == (usPage - 2222) % 4)
            ucPageType = FLASH_HIGH_PAGE;
        else //1 or 3 = (usPage - 2222) % 4
            ucPageType = FLASH_MID_PAGE;
    }
    else                   //page 2268~2291
    {
        if (0 == usPage % 2)     
            ucPageType = FLASH_HIGH_PAGE;
        else
            ucPageType = FLASH_MID_PAGE;
    }
#if 0
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
#endif
    return ucPageType;
}

U8 Micron3dTlcB16_GetPairPgType(U16 usPairPage, U8 ucBlkType)
{
    U8 ucPairPgType;

    if (BLOCK_SLC == ucBlkType)
    {
        ucPairPgType = LOW_PAGE_ONLY;
        return ucPairPgType;
    }

    if ((usPairPage < 12)||(usPairPage >= 2292))
    {
        ucPairPgType = LOW_PAGE_ONLY;
    }    
    else if (usPairPage < 36)
    {
        ucPairPgType = LOW_MID_PAGE;
    }
    else if ((usPairPage < 59) || (usPairPage < 2222))       
    {
        ucPairPgType = LOW_MID_HIGH_PAGE;
    }
    else if (usPairPage < 2268)                  
    {
        if ((2 == (usPairPage - 2222) % 4) || (3 ==(usPairPage - 2222) % 4))    
            ucPairPgType = LOW_MID_HIGH_PAGE;
        else 
            ucPairPgType = LOW_MID_PAGE;
    }
    else //page 2268~2291
    {
        ucPairPgType = LOW_MID_PAGE;
    }
    return ucPairPgType;
}

U8 Micron3dTlcB16_GetPlnNum (U8 ucCmdCode)
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


U16 Micron3dTlcB16_GetPairPage(U16 usPage, U8 ucBlkType)
{
    U16 usPairPgNum = usPage;
    U8  ucPageType = 0;
    
    if (BLOCK_SLC == ucBlkType)
    {
        return usPage;
    }

    /*Get pair page for TLC block*/
    ucPageType = Micron3dTlcB16_GetPgType(usPage, ucBlkType);
    if (FLASH_LOW_PAGE == ucPageType)
    {
        usPairPgNum = usPage;
    }
    else if (FLASH_MID_PAGE == ucPageType)
    {
        usPairPgNum--;
    }
    else
    {
        usPairPgNum++;
    }

    return usPairPgNum;
}

void Micron3dTlcB16_SetBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode)
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

void Micron3dTlcB16_IncPairPgPrgStep(PFLASH_PHY pFlash_phy)
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
        //g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = PAIR_PAGE_PRG_MAX;
        g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep = 1;
    }
    else 
    {
        if (FLASH_MID_PAGE == g_tFlashSpecInterface.GetPgType(pFlash_phy->nPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
        {
            //g_aFlashStatus.aFlashPgInfo[ucLun][ucPln][usBlk][usPairPage].bsProgStep++;   

            //if (LOW_MID_PAGE == ucPairPageType)
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

void Micron3dTlcB16_ResetPrgOrder(U8 ucLun, U16 usBlk, U8 ucPln)
{
    l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk] = 0;
    l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = 0;
    l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk] = 0;

    return;
}

void Micron3dTlcB16_GetWriteFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr)
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

void Micron3dTlcB16_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
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
    else if (NF_PRCQ_TLC_READ == ucCmdCode || NF_PRCQ_TLC_READ_MULTIPLN == ucCmdCode)    
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

    /*TLC read cmd check*/
    else if ((NF_PRCQ_TLC_READ == ucCmdCode) && (ulTransferLen > g_PG_SZ))    
    {        
        DBG_Getch();    
    }    
    else if ((NF_PRCQ_TLC_READ_MULTIPLN == ucCmdCode) && (ulTransferLen > (g_PG_SZ * PLN_PER_LUN)))    
    {        
        DBG_Getch();    
    }

    return;
}

BOOL Micron3dTlcB16_CheckBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode)
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
    DBG_Printf("Micron3dTlcB16_CheckBlkType: check Block type error, Lun %d, Pln %d, Block %d BlockType %d CmdType %d TLCMode %d \n",
        ucLun, ucPln, usBlk, g_aFlashStatus.aFlashBlkInfo[ucLun][ucPln][usBlk].bsBlockType, ucCmdType, bsTLCMode);
    return FALSE;
}

BOOL Micron3dTlcB16_CheckErsBlkType(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode)
{
    if(BLOCK_TLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk)  && 
        (NF_PRCQ_ERS_MULTIPLN == ucCmdCode || NF_PRCQ_ERS == ucCmdCode))
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL Micron3dTlcB16_CheckPrgOrder(PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *p_flash_cmd_para)
{
    U8 ucCmdCode;
    U8 ucLun, ucPln;
    U16 usBlk, usPage;

    ucLun = (U8)pFlash_phy->ucLunInTotal;
    ucPln = (U8)pFlash_phy->nPln;
    usBlk = (U16)pFlash_phy->nBlock;
    usPage = (U16)pFlash_phy->nPage;
    ucCmdCode = p_flash_cmd_para->bsCmdCode;

    if (12 == usPage)
    {
        usPage = 12;
    }
    if (BLOCK_SLC == FlashStsM_GetBlkType(ucLun, ucPln, usBlk))
    {
        if (usPage > 767)
        {
            DBG_Printf("MICRON_3DTLC_B16 SLC block page %d page number overflow!\n", usPage);
            DBG_Getch();
        }

        if (usPage != l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
        {
            DBG_Printf("MICRON_3DTLC_B16 SLC block check program order error\n");
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

    /* check page number overflow */
    if (usPage > MICRON_3DTLC_B16_MAX_PG_NUM)
    {       
        DBG_Printf("MICRON_3DTLC_B16 page %d program order overflow!\n", usPage);
        DBG_Getch();
    }

    /* check 3D-TLC program order */
    if (usPage != l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk])
    {
        DBG_Printf("PageNum %d is not equal to l_a3DTlcPrgPPO[%d].bsNextPrgPPO[%d][%d]: %d!\n", usPage, ucLun, ucPln, usBlk, l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]);
        DBG_Printf("Program order %d!\n", l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk]);
        //DBG_Getch();
    }

    /* update next program page */
    if (FLASH_LOW_PAGE == g_tFlashSpecInterface.GetPgType(usPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        U16 usPairPage = g_tFlashSpecInterface.GetPairPage(usPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk));
        if (LOW_MID_PAGE == g_tFlashSpecInterface.GetPairPgType(usPairPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
        {
            l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
            l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk] += 2;
        }

        else
        {
            l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
            l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk]++;
        }
    }
    else if (FLASH_HIGH_PAGE == g_tFlashSpecInterface.GetPgType(usPage, FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {
        l_a3dTlcPrgPPO[ucLun].bsNextPrgPPO[ucPln][usBlk]++;
    }

    /* update previous page and program order */
    if(FLASH_HIGH_PAGE != g_tFlashSpecInterface.GetPgType(l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk], FlashStsM_GetBlkType(ucLun, ucPln, usBlk)))
    {        
        //l_a3dTlcPrgPPO[ucLun].bsPrgOrder[ucPln][usBlk]++;
    }   
    l_a3dTlcPrgPPO[ucLun].bsPrePrgPPO[ucPln][usBlk] = usPage;

    return TRUE;
    
}

BOOL Micron3dTlcB16_ReadCheck(PFLASH_PHY pFlash_phy)
{
    return NF_SUCCESS;
}

BOOL Micron3dTlcB16_GetBlockMode(U8 ucCmdCode)
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

void Micron3dTlcB16_InterfaceInit(FlashSpecInterface * ptFlashSpecInterface)
{
  ptFlashSpecInterface->GetCmdType = Micron3dTlcB16_GetCmdType;
  ptFlashSpecInterface->GetPgType = Micron3dTlcB16_GetPgType;
  ptFlashSpecInterface->GetPairPgType = Micron3dTlcB16_GetPairPgType;
  ptFlashSpecInterface->GetPairPage = Micron3dTlcB16_GetPairPage;
  ptFlashSpecInterface->SetBlkType = Micron3dTlcB16_SetBlkType;
  ptFlashSpecInterface->IncPairPgPrgStep = Micron3dTlcB16_IncPairPgPrgStep;
  ptFlashSpecInterface->CheckBlkType = Micron3dTlcB16_CheckBlkType;
  ptFlashSpecInterface->CheckErsType = Micron3dTlcB16_CheckErsBlkType;
  ptFlashSpecInterface->CheckPrgOrder = Micron3dTlcB16_CheckPrgOrder;
  ptFlashSpecInterface->ReadCheck = Micron3dTlcB16_ReadCheck;
  ptFlashSpecInterface->ResetPrgOrder = Micron3dTlcB16_ResetPrgOrder;
  ptFlashSpecInterface->GetWriteFlashAddr = Micron3dTlcB16_GetWriteFlashAddr;
  ptFlashSpecInterface->GetBlkMode = Micron3dTlcB16_GetBlockMode;
  ptFlashSpecInterface->GetPlnNum = Micron3dTlcB16_GetPlnNum;
  ptFlashSpecInterface->CheckTransferLen = Micron3dTlcB16_CheckTransferLen;
  return;
}
#endif
