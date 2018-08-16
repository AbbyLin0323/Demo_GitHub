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

*Filename     : sim_flash_shedule.c
*Version      :   Ver 1.0
*Date         :
*Author       :  Catherine Li

*Description:
*shedule for flash module
*include falsh module sheduling and timing delay

*Depend file:
*sim_flash_shedule.h
*sim_flash_interface.h & c
*sim_flash_cmd.h & c

*Modification History:
*20120208      Catherine Li 001 first create

*************************************************/
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"
#include "sim_flash_shedule.h"
#include "sim_flash_interface.h"
#include "sim_nfccmd.h"
#include "system_statistic.h"
#include "sim_flash_config.h"
#include "sim_SGE.h"
#include "checklist_parse.h"
#include "win_bootloader.h"
#include "sim_flash_common.h"
#include "sim_flash_status.h"
#include "Sim_DEC_Status.h"
#include "flash_opinfo.h"
#ifndef L1_FAKE
#include "L2_TableBlock.h"
#endif

/************************************************
GLOBAL value define
*************************************************/
//flash module log
char FM_LogString[256]={0};
int FM_LogHasWrite;
HANDLE hFM_LogFile;
CRITICAL_SECTION FMLog_critical_section;

//ST_CE_ERROR_ENTRY pu_err[CE_MAX] = {0};
ST_FLASH_ERR_INJ_ENTRY  g_stFlashErrInjEntry[ERR_INJ_TABLE_MAX];
ST_PU_SHEDULE_PARAM pu_param[NFC_CH_TOTAL][NFC_PU_PER_CH];
ST_FLASH_CMD_PARAM flash_cmd_param[NFC_CH_TOTAL][NFC_PU_PER_CH][NFC_LUN_MAX_PER_PU];
ST_FLASH_READ_RETRY_PARAM flash_readretry_param[NFC_CH_TOTAL][NFC_PU_PER_CH][NFC_LUN_MAX_PER_PU];

CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
CRITICAL_SECTION g_PuAccCriticalSection;
CRITICAL_SECTION g_aCSUptTLunCmdSts[NFC_MODEL_LUN_SUM];
CRITICAL_SECTION g_aCSUptTLunSwBitmap;

SIM_NFC_RED g_pRedBuffer[NFC_CH_TOTAL][NFC_PU_PER_CH][NFC_LUN_PER_PU];
U8   bus_status[NFC_CH_TOTAL];

U32 g_secAddGroup;
U32 g_splitedSecInGroup;
U32 g_totalRemainSec;
U32 g_finishSecInDmaEntry;
U32 g_remainSecToXfer;
U32 g_bmBitPos;

U32 g_RetryTimes[SUBSYSTEM_LUN_MAX] = {0}; // updated by jsaonguo 20140815
U8 L_PRCQ_CurrentCnt = 0;

U32 g_SwitchSwMod[SUBSYSTEM_LUN_MAX][3] = { 0 };

extern BOOL bSoftDecorerTrigger;
extern GLOBAL MCU12_VAR_ATTR volatile SOFT_MCU2_CFG_REG *pSoftMcu2CfgReg;
extern GLOBAL PRE_CMD_INFO g_aPreCmdInfo[NFC_MODEL_LUN_SUM];
extern GLOBAL U32 g_ulWorkLun;
//squashed_mode tage
GLOBAL BOOL g_bSquashMode = FALSE;

/************************************************
Function define
*************************************************/
//tobey add start
extern NFCQ_ENTRY* COM_GetNFCQEntry(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 rp);
extern void NFC_InterfaceUpdateRedData(ST_FLASH_CMD_PARAM *pFlashCmdParam);
extern void RecInjErrFile(HANDLE hFile,char* fmt,...);
extern HANDLE hFileInjErr;
extern void IsHitErrInjParam(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* p_flash_param,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param);
extern void set_fcond(FLASH_PHY* pflash_phy1,U32* pRedData,U32 CmdCode);
SOFT_DEC_DESCRIPTOR * MCU12_DRAM_TEXT HAL_SoftDecGetDescAddr(U32 DescID);
extern NF_PPUST_REG *p_flash_ppust_reg;

#ifndef IGNORE_PERFORMANCE
extern U8 l_ucPcieBusDevToHostSts;
extern CRITICAL_SECTION g_PcieDevToHostCriticalSection;

LOCAL U8 l_aOtfbBuffSts[NFC_CH_TOTAL][NFC_PU_PER_CH];
LOCAL U8 l_aSgeOtfbSts[NFC_CH_TOTAL][NFC_PU_PER_CH];
LOCAL U8 ucPreOtfbSecCnt[NFC_CH_TOTAL][NFC_PU_PER_CH];
#endif

// FCmd Check Cnt, added by jasonguo 20150409
extern BOOL L3_DbgFCmdAutoChk(U8 ucPhyPuInTotal);
extern BOOL L3_DbgFCmdNullChk(U8 ucPhyPuInTotal);
extern BOOL L3_DbgFCmdHostRdChk(U8 ucPhyPuInTotal);
void NFC_LogCmdCnt(ST_FLASH_CMD_PARAM *p_flash_param, const NFCM_LUN_LOCATION *tNfcOrgStruct)
{
    U8 ucPhyPuInTotal = tNfcOrgStruct->ucLunInPhyPu * NFC_CH_TOTAL * NFC_PU_PER_CH + tNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + tNfcOrgStruct->ucCh;

    if (TRUE == p_flash_param->ssu0_en)
    {
        if (TRUE == p_flash_param->ssu1_en)
        {
            if (TRUE == L3_DbgFCmdAutoChk(ucPhyPuInTotal))
            {
                //FIRMWARE_LogInfo("TLun%d Cmd=%d RP=%d SSU1 Finish.\n", ucPhyPuInTotal, p_flash_param->bsCmdCode, NFC_GetRP(tNfcOrgStruct));
            }
        }
        else if ((TRUE == p_flash_param->bsTrigOmEn) || (TRUE == p_flash_param->p_nf_cq_entry->bsBmEn))
        {
            if (TRUE == L3_DbgFCmdHostRdChk(ucPhyPuInTotal))
            {
                //FIRMWARE_LogInfo("TLun%d Cmd=%d RP=%d HostRd Finish.\n", ucPhyPuInTotal, p_flash_param->bsCmdCode, NFC_GetRP(tNfcOrgStruct));
            }
        }
        else
        {
            if (TRUE == L3_DbgFCmdNullChk(ucPhyPuInTotal))
            {
                //FIRMWARE_LogInfo("TLun%d Cmd=%d RP=%d SSU0 Finish.\n", ucPhyPuInTotal, p_flash_param->bsCmdCode, NFC_GetRP(tNfcOrgStruct));
            }
        }
    }

    return;
}

/*
The callback function for NFC cmd trigger, updated by jasonGuo.
*/

void NFC_SendCMD(U8 ucPhyPuInTotal, U8 ucLunInPhyPu)
{
    NFCM_LUN_LOCATION tNfcOrgStruct;

    tNfcOrgStruct.ucCh = ucPhyPuInTotal % NFC_CH_TOTAL;
    tNfcOrgStruct.ucPhyPuInCh = ucPhyPuInTotal / NFC_CH_TOTAL;
    tNfcOrgStruct.ucLunInPhyPu = ucLunInPhyPu;

    NFC_InterfaceJumpCQWP(&tNfcOrgStruct);
}

void initSplitSectorAddr(NFCQ_ENTRY *p_nf_cq_entry)
{
    g_secAddGroup = 0;
    g_splitedSecInGroup = 0;
    g_totalRemainSec = p_nf_cq_entry->bsDmaTotalLength;
}

BOOL doSplitSectorAddr(NFCQ_ENTRY *p_nf_cq_entry, U8 *pStartInPage, U8 *pLengthInPage, U8 *pPlaneIndex)
{
    U8 curStart, curLength, startInPage, lengthInPage, planeIndex;
    BOOL bFinishSplitOnePhyPage = FALSE;

    curStart = p_nf_cq_entry->aSecAddr[g_secAddGroup].bsSecStart + g_splitedSecInGroup;
    curLength = p_nf_cq_entry->aSecAddr[g_secAddGroup].bsSecLength - g_splitedSecInGroup;

    startInPage = curStart&SEC_PER_PHYPG_MSK;
    planeIndex = curStart>>SEC_PER_LOGIC_PG_BITS;
    if ((startInPage + curLength)>SEC_PER_PHYPG)
    {
        lengthInPage = SEC_PER_PHYPG - startInPage;
        g_splitedSecInGroup  += lengthInPage;
        bFinishSplitOnePhyPage = TRUE;
    }
    else
    {
        lengthInPage = curLength;
        g_secAddGroup++;
        g_splitedSecInGroup = 0;

        if(planeIndex !=((p_nf_cq_entry->aSecAddr[g_secAddGroup].bsSecStart)>>SEC_PER_LOGIC_PG_BITS) ||
            0 == g_totalRemainSec - lengthInPage)
        {
            bFinishSplitOnePhyPage = TRUE;
        }
    }

    *pStartInPage = startInPage;
    *pLengthInPage = lengthInPage;
    *pPlaneIndex = planeIndex;
    g_totalRemainSec -= lengthInPage;

    return bFinishSplitOnePhyPage;
}

BOOL getInfoInRawCmd(NFCQ_ENTRY *pNfcqEntry,
                U8 *pRowAddrSel,
                U8 *pPlaneSel)
{
    NFC_PRCQ_ELEM * pPRCQ_Entry;
    //U8 temp;

    if (L_PRCQ_CurrentCnt >= MAX_PRCQ_ELEM_PER_CMD)
    {
        DBG_Printf("getInfoInRawCmd: L_PRCQ_CurrentCnt = %d, greater than or equal to PRCQ_DEPTH = %d\n", L_PRCQ_CurrentCnt, MAX_PRCQ_ELEM_PER_CMD);
        DBG_Break();
    }

    pPRCQ_Entry = (NFC_PRCQ_ELEM *)(PRCQ_ENTRY_BASE + (pNfcqEntry->bsPrcqStartDw << DWORD_SIZE_BITS) + L_PRCQ_CurrentCnt);
    while((PRCQ_CMD_DQE !=  pPRCQ_Entry->bsGroup) && ((PRCQ_CMD_END) !=  *((U8 *)pPRCQ_Entry)))
    {
        pPRCQ_Entry++;
        L_PRCQ_CurrentCnt++;
    }

    //if meet QE end get plansel and rowsel false
    if(PRCQ_CMD_END ==  *(U8 *)pPRCQ_Entry)
    {
        return FALSE;
    }

    //if meet DQE search backward for AQE
    if(PRCQ_CMD_DQE ==  pPRCQ_Entry->bsGroup)
    {
        //*pPlaneSel = pPRCQ_Entry->bsPlnSel;
        //when DQE Index stand for PlnSel
        *pPlaneSel = pPRCQ_Entry->bsIndex;
        do
        {
            --pPRCQ_Entry;
            if((U32)pPRCQ_Entry == (U32)(PRCQ_ENTRY_BASE + pNfcqEntry->bsPrcqStartDw))
            {
                DBG_Getch();
            }
        }while(PRCQ_CMD_AQE != pPRCQ_Entry->bsGroup);
        *pRowAddrSel =   pPRCQ_Entry->bsIndex;
        L_PRCQ_CurrentCnt++;
    }
    return TRUE;
}

U8 NF_GetCmdType(U8 ucCmdCode)
{
    U8 ucCmdType = CMD_CODE_NONE;
    if ((NF_PRCQ_READ == ucCmdCode) || (NF_PRCQ_READ_MULTIPLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_READ;
    }
    else if ((NF_PRCQ_CCLR == ucCmdCode) || (NF_PRCQ_CCLR_MULTIPLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_READ;
    }
#ifdef FLASH_INTEL_3DTLC
    else if (NF_PRCQ_SLC_SNAP_READ == ucCmdCode)
    {
        ucCmdType = CMD_CODE_READ;
    }
#endif
    else if ((NF_PRCQ_PRG == ucCmdCode) || (NF_PRCQ_PRG_MULTIPLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_PROGRAM;
    }
    else if ((NF_PRCQ_ERS_MULTIPLN == ucCmdCode) || (NF_PRCQ_ERS == ucCmdCode))
    {
        ucCmdType = CMD_CODE_ERASE;
    }
    else if (NF_PRCQ_RESET == ucCmdCode)
    {
        ucCmdType = CMD_CODE_RST;
    }
    else if (NF_PRCQ_SETFEATURE == ucCmdCode)
    {
        ucCmdType = CMD_CODE_SET_FEATURE;
    }
    else if (NF_PRCQ_GETFEATURE == ucCmdCode)
    {
        ucCmdType = CMD_CODE_GET_FEATURE;
    }
    else if (NF_PRCQ_READID == ucCmdCode)
    {
        ucCmdType = CMD_CODE_READID;
    }
    else if (ucCmdCode > NF_PRCQ_PIO_SETFEATURE_EX)
    {
        ucCmdType = CMD_CODE_UNKNOWN;
    }
    else
    {
        if (g_tFlashSpecInterface.GetCmdType != NULL)
        {
            ucCmdType = g_tFlashSpecInterface.GetCmdType(ucCmdCode);
        }
        else
        {
            ucCmdType = CMD_CODE_OTHER;
        }
    }

    return ucCmdType;
}

U8 NFC_GetPartInWL(U8 ucCmdCode)
{
    U8 ucPartInWL;

#ifdef FLASH_TLC
    if (ucCmdCode >= NF_PRCQ_TLC_READ_LP_MULTIPLN && ucCmdCode <= NF_PRCQ_TLC_READ_UP_MULTIPLN)
    {
        ucPartInWL = (ucCmdCode - NF_PRCQ_TLC_READ_LP_MULTIPLN) % PAGE_PER_WL; // uc_raw_cmd_code - READ_LOW;
    }
    else if (ucCmdCode >= NF_PRCQ_TLC_READ_LP && ucCmdCode <= NF_PRCQ_TLC_READ_UP)
    {
        ucPartInWL = (ucCmdCode - NF_PRCQ_TLC_READ_LP) % PAGE_PER_WL; // uc_raw_cmd_code - READ_LOW;
    }
    else if (ucCmdCode == NF_PRCQ_CCLR || ucCmdCode == NF_PRCQ_CCLR_MULTIPLN)
    {
        ucPartInWL = g_aPreCmdInfo[g_ulWorkLun].bsPage % PAGE_PER_WL;
    }
    else
#endif
    {
        ucPartInWL = 0;
    }

    return ucPartInWL;
}

/************************************************
Function Name: NFC_UpdateCmdParam
Input:     pointer to HW CQ entry and raw cmd
Output:     param handled by model
Description:
convert HW CQ entry & raw cmd to param handled by model
*************************************************/
void NFC_UpdateCmdParam(ST_FLASH_CMD_PARAM *p_flash_param,
                NFCQ_ENTRY *p_nf_cq_entry, SIM_NFC_RED * pLocalRedundant, U8 ucCmdCode)
{
    U8 bSecHighDW,bitPosInDW;
    BOOL bFinishSplitOnePhyPage, bSuccess;
    U8 startInPage,lengthInPage,planeIndex;
    U8 rowAddrSel;
    U8 planeSel = PLN_PER_LUN - 1;
    U32 ulWritePhyPageCount = 0;
    U8 cmd_type = 0;

    initSplitSectorAddr(p_nf_cq_entry);

    cmd_type = NF_GetCmdType(ucCmdCode);

    p_flash_param->p_nf_cq_entry = p_nf_cq_entry;
    p_flash_param->p_red_addr = p_nf_cq_entry->bsRedAddr << 3;
    p_flash_param->ontf_en= p_nf_cq_entry->bsOntfEn;
    p_flash_param->red_en = p_nf_cq_entry->bsRedEn;
    p_flash_param->red_only = p_nf_cq_entry->bsRedOnly;
    p_flash_param->p_local_red = (NFC_RED *)pLocalRedundant;
    p_flash_param->int_en = p_nf_cq_entry->bsIntEn;
    p_flash_param->ssu0_en = p_nf_cq_entry->bsSsu0En;
    p_flash_param->ssu1_en = p_nf_cq_entry->bsSsu1En;
    p_flash_param->ncq_1st_en = p_nf_cq_entry->bsFstDataRdy;//hv
    p_flash_param->ncq_mode = p_nf_cq_entry->bsNcqMd;
    p_flash_param->ncq_num = p_nf_cq_entry->bsNcqNum;
    p_flash_param->inj_en = p_nf_cq_entry->bsInjEn;
    p_flash_param->err_type = p_nf_cq_entry->bsErrTypeS;
    p_flash_param->bsBypassRdErr = p_nf_cq_entry->bsBypassRdErr;
    p_flash_param->bsRawReadEn = p_nf_cq_entry->bsRawReadEn;
    p_flash_param->bsTrigOmEn = p_nf_cq_entry->bsTrigOmEn;
    p_flash_param->bsRomRd   = p_nf_cq_entry->bsRomRd;
    p_flash_param->bsDmaByteEn = p_nf_cq_entry->bsDmaByteEn;
    p_flash_param->bsCmdCode = ucCmdCode;
    p_flash_param->bsCmdType = cmd_type;

    // JasonGuo: the following cmdcode detection needs to be moved to flash-interfaces

    /*Get Internal copy enable bit*/
    if (g_tFlashSpecInterface.GetInterCopy != NULL)
    {
        p_flash_param->bsInterCopyEn = g_tFlashSpecInterface.GetInterCopy(ucCmdCode);
    }
    else
    {
        p_flash_param->bsInterCopyEn = FALSE;
    }

    /*Get Plane number*/
    if (g_tFlashSpecInterface.GetPlnNum != NULL)
    {
        p_flash_param->bsPlnNum = g_tFlashSpecInterface.GetPlnNum(ucCmdCode);
    }
    else
    {
        DBG_Getch();
    }

    /*Get TLC mode enable*/
    if (g_tFlashSpecInterface.GetBlkMode != NULL)
    {
        p_flash_param->bsIsTlcMode = g_tFlashSpecInterface.GetBlkMode(ucCmdCode);
    }
    else
    {
        p_flash_param->bsIsTlcMode = FALSE;
    }

    // Slc Internal copy to TLC - DmaByteEn
    if (TRUE == p_flash_param->bsDmaByteEn)
    {
        U8 pln;
        for (pln = 0; pln < 8; pln++)
        {
            p_flash_param->phy_page_req[pln].row_addr = p_nf_cq_entry->atRowAddr[pln].bsRowAddr;
        }
        return;
    }

    if( (cmd_type == CMD_CODE_READ || cmd_type == CMD_CODE_PROGRAM)
        && p_flash_param->red_only != 1 )
    {
        if(0 == p_nf_cq_entry->bsDmaTotalLength)
        {
            p_flash_param->xfer_sec_cnt = 256;
        }
        else
        {
            p_flash_param->xfer_sec_cnt = p_nf_cq_entry->bsDmaTotalLength;
        }
    }


    //special handling for command without data transfer
    if (cmd_type != CMD_CODE_READ && cmd_type != CMD_CODE_PROGRAM )
    {
        if( cmd_type == CMD_CODE_ERASE )
        {
            U8 pln;
            for (pln = 0; pln < p_flash_param->bsPlnNum; pln++)
            {
                p_flash_param->phy_page_req[pln].row_addr = p_nf_cq_entry->atRowAddr[pln].bsRowAddr;
            }
        }
        return;
    }

    //clear for getInfoInRawCmd each time
    L_PRCQ_CurrentCnt = 0;

    if (cmd_type == CMD_CODE_READ)
    {
        if (p_flash_param->red_only !=1)
        {
            //split data transfer info for read/write command
            do
            {
                bFinishSplitOnePhyPage = doSplitSectorAddr(p_flash_param->p_nf_cq_entry,
                                            &startInPage,
                                            &lengthInPage,
                                            &planeIndex);
                while(lengthInPage)
                {
                    bSecHighDW = (startInPage>31)?1:0;
                    bitPosInDW = (bSecHighDW==1)?(startInPage-32):startInPage;
                    p_flash_param->phy_page_req[planeIndex].sec_en[bSecHighDW] |= 1<<bitPosInDW;
                    lengthInPage--;
                    startInPage++;
                }

                if(TRUE == bFinishSplitOnePhyPage)
                {
                    do
                    {
                        bSuccess = getInfoInRawCmd(p_flash_param->p_nf_cq_entry, &rowAddrSel, &planeSel);

                        if(FALSE == bSuccess &&(cmd_type == CMD_CODE_PROGRAM || cmd_type == CMD_CODE_READ))
                        {
                            DBG_Getch();//err in raw command, check RAW_DATA_IO_CMD
                        }
                    } while (planeSel != (planeIndex % p_flash_param->bsPlnNum)); // while ((planeSel%PLN_PER_LUN) != planeIndex); // while(planeSel != planeIndex);

                    p_flash_param->phy_page_req[planeIndex].row_addr = (U32)p_nf_cq_entry->atRowAddr[rowAddrSel].bsRowAddr;
                    p_flash_param->phy_page_req[planeIndex].part_in_wl = NFC_GetPartInWL(p_flash_param->bsCmdCode);
                    p_flash_param->bsPageInWL = NFC_GetPartInWL(p_flash_param->bsCmdCode);
                    p_flash_param->phy_page_req[planeIndex].red_offset_id = planeSel;
                }
            }while(0 != g_totalRemainSec);
        }
        else
        {
            /// VT3533 Read redentant only does not set the secAddr of NFCQ,so we
            /// need get the plane number from bsDmaTotalLength of NFCQ.
            for (planeIndex = 0; planeIndex <= p_flash_param->p_nf_cq_entry->bsDmaTotalLength; planeIndex++)
            {
                p_flash_param->phy_page_req[planeIndex].row_addr = (U32)p_nf_cq_entry->atRowAddr[planeIndex].bsRowAddr;
                p_flash_param->phy_page_req[planeIndex].part_in_wl = NFC_GetPartInWL(p_flash_param->bsCmdCode);
                p_flash_param->bsPageInWL = NFC_GetPartInWL(p_flash_param->bsCmdCode);
            }
        }

    }
    else
    {
        ulWritePhyPageCount = (p_flash_param->p_nf_cq_entry->bsDmaTotalLength * CW_INFO_SZ) / LOGIC_PG_SZ;

        for(planeIndex = 0; planeIndex < ulWritePhyPageCount; ++planeIndex)
        {
            do
            {
                bSuccess = getInfoInRawCmd(p_flash_param->p_nf_cq_entry, &rowAddrSel, &planeSel);

                if(FALSE == bSuccess &&(cmd_type == CMD_CODE_PROGRAM || cmd_type == CMD_CODE_READ))
                {
                    DBG_Getch();//err in raw command, check RAW_DATA_IO_CMD
                }
                //} while (planeSel != (planeIndex % p_flash_param->bsPlnNum)); // while ((planeSel%PLN_PER_LUN) != planeIndex); // while(planeSel != planeIndex);
            } while ((planeSel% p_flash_param->bsPlnNum) != (planeIndex % p_flash_param->bsPlnNum));
            p_flash_param->phy_page_req[planeIndex].row_addr = (U32)p_nf_cq_entry->atRowAddr[planeIndex].bsRowAddr;
            p_flash_param->phy_page_req[planeIndex].part_in_wl = 0; //NFC_GetPartInWL(p_flash_param->bsCmdCode);
            p_flash_param->bsPageInWL = 0;
            p_flash_param->phy_page_req[planeIndex].red_offset_id = planeSel;
        }
    }

    return;
}

void NfcM_UpdateCmdParamSpecFields(const NFCM_LUN_LOCATION *ptNfcOrgStruct, U32 ulCmdLevel, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    if ((NfcM_IsFullPlaneXorParityPageInWriteCmd(ptNfcOrgStruct, ulCmdLevel) == TRUE) &&
        (NfcM_Is6DsgIssueCmd(pFlashCmdParam->bsCmdCode) == TRUE))
    {
        NFCQ_ENTRY *pNfcqEntry = COM_GetNFCQEntry(ptNfcOrgStruct, ulCmdLevel);

        pFlashCmdParam->bsIsTlcMode = TRUE;
        pFlashCmdParam->bsIs6DsgIssue = TRUE;
        pFlashCmdParam->bsPageInWL = pNfcqEntry->bsParPgPos;
    }
}

void NfcM_CmdParamFetch(ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    ASSERT(pFlashCmdParam != NULL);
    ASSERT(pFlashCmdParam->pLunLocation != NULL);

    NFCQ_ENTRY * pNfcqEntry = NULL;
    U8 ucRp = 0;
    U8 ucCmdCode = 0;
    SIM_NFC_RED * pLocalRedundant = NULL;
    const NFCM_LUN_LOCATION * pLunLocation = pFlashCmdParam->pLunLocation;
    ST_FLASH_READ_RETRY_PARAM * pFlashReadretryParam = pFlashCmdParam->pFlashReadretryParam;

    // 1st, memory zero the pFlashCmdParam;
    memset(pFlashCmdParam, 0, sizeof(ST_FLASH_CMD_PARAM));
    pFlashCmdParam->pLunLocation = pLunLocation;
    pFlashCmdParam->pFlashReadretryParam = pFlashReadretryParam;

    // 2nd, get the NFCQ Entry & command code by tNfcOrgStruct + ucRp.
    ucRp = NFC_GetRP(pLunLocation);
    pNfcqEntry = COM_GetNFCQEntry(pLunLocation, ucRp);
    ucCmdCode = NFC_GetCmdCode(pLunLocation, ucRp);
    pLocalRedundant = &g_pRedBuffer[pLunLocation->ucCh][pLunLocation->ucPhyPuInCh][pLunLocation->ucLunInPhyPu];

    // 3rd, update the pFlashCmdParam according the NFCQ Entry.
    NFC_UpdateCmdParam(pFlashCmdParam, pNfcqEntry, pLocalRedundant, ucCmdCode);

    // 4th, update pFlashCmdParam according other special situations.
    NfcM_UpdateCmdParamSpecFields(pLunLocation, ucRp, pFlashCmdParam);
    return;
}

void NfcM_CmdParamCheck(const ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    ASSERT(pFlashCmdParam != NULL);

    const ST_FLASH_READ_RETRY_PARAM *pFlashReadretryParam = pFlashCmdParam->pFlashReadretryParam;
    U8 ucCmdType = pFlashCmdParam->bsCmdType;

    /*Check Transfer Length*/
    if (g_tFlashSpecInterface.CheckTransferLen!= NULL)
    {
        g_tFlashSpecInterface.CheckTransferLen(pFlashCmdParam->bsCmdCode, pFlashCmdParam->p_nf_cq_entry);
    }
    else
    {
        DBG_Getch();
    }

    //check for read retry sequence
    if (TRUE == pFlashReadretryParam->read_retry_en)
    {
        if((ucCmdType != CMD_CODE_RST) && (ucCmdType != CMD_CODE_READ) && (ucCmdType != CMD_CODE_READRETRY_ADJ)
            && (ucCmdType != CMD_CODE_READRETRY_EN) && (ucCmdType != CMD_CODE_SET_FEATURE))
        {
            DBG_Printf("NfcM_CmdParamCheck  cmd_code is %d\n", ucCmdType);
            DBG_Getch();
        }
    }

    // Check XOR related information.
    NFCQ_ENTRY *pNfcqEntry = pFlashCmdParam->p_nf_cq_entry;
    if (pNfcqEntry->bsXorEn == TRUE)
    {
        if (pNfcqEntry->bsXorBufId != 0)
        {
            DBG_Printf("NfcM_CmdParamCheck  XOR bufferid in NFCQ entry should always be 0 nowadays!\n");
            DBG_Getch();
        }
    }

    // Check plane count of this command
    if (pNfcqEntry->bsTlcPlnNum > PLN_PER_LUN_BITS)
    {
        DBG_Printf("NfcM_CmdParamCheck  bsTlcPlnNum should be bits of plane count!\n");
        DBG_Getch();
    }

    return;
}

void NfcM_CmdErrInjection(ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    if (FALSE == pFlashCmdParam->inj_en)
    {
        NFC_ErrorInjCmd(pFlashCmdParam);
    }
    else
    {
        NFC_ErrInj(pFlashCmdParam->err_type, pFlashCmdParam);
    }

    return;
}

//#ifdef FLASH_TLC
void NFC_InternalCopyCmdExecute(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
#ifdef FLASH_TSB_3D
    return;
#else
    BOOL bResult = TRUE;
    U8 ucTriggerStage, ucPartInWL;

    switch (pFlashCmdParam->bsCmdCode)
    {
        case NF_PRCQ_SLC_COPY_SLC_MULTIPLN:
        {
            bResult = NFC_SlcCopyToSlcCMD(tNfcOrgStruct, pFlashCmdParam);
            break;
        }

        case NF_PRCQ_SLC_COPY_TLC_1ST_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_2ND_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_3RD_MULTIPLN:
        {
            ucTriggerStage = PAGE_PER_WL;
            ucPartInWL = pFlashCmdParam->bsCmdCode - NF_PRCQ_SLC_COPY_TLC_1ST_MULTIPLN;
            bResult = NFC_SlcCopyToTlcCMD(tNfcOrgStruct, pFlashCmdParam, ucTriggerStage, ucPartInWL);
            break;
        }
//#ifdef TRI_STAGE_COPY
        case NF_PRCQ_SLC_COPY_TLC_1ST_LP_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_1ST_MP_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_1ST_UP_MULTIPLN:
        {
            ucTriggerStage = 1;
            ucPartInWL = 0;
            bResult = NFC_SlcCopyToTlcCMD(tNfcOrgStruct, pFlashCmdParam, ucTriggerStage, ucPartInWL);
            break;
        }

        case NF_PRCQ_SLC_COPY_TLC_2ND_LP_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_2ND_MP_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_2ND_UP_MULTIPLN:
        {
            ucTriggerStage = 1;
            ucPartInWL = 1;
            bResult = NFC_SlcCopyToTlcCMD(tNfcOrgStruct, pFlashCmdParam, ucTriggerStage, ucPartInWL);
            break;
        }

        case NF_PRCQ_SLC_COPY_TLC_3RD_LP_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_3RD_MP_MULTIPLN:
        case NF_PRCQ_SLC_COPY_TLC_3RD_UP_MULTIPLN:
        {
            ucTriggerStage = 1;
            ucPartInWL = 2;
            bResult = NFC_SlcCopyToTlcCMD(tNfcOrgStruct, pFlashCmdParam, ucTriggerStage, ucPartInWL);
            break;
        }
//#endif
        default:
        {
            DBG_Printf("CmdType invalid.\n");
            DBG_Getch();
        }
    }
#endif
}
//#endif

// ReWrite Check
void NfcM_ReWriteCheck(const NFCM_LUN_LOCATION *pLunLocation, const ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U32 ulPhyPuInTotal = NfcM_GetPhyPuInTotal(pLunLocation);
    U32 ulAtomPageCount = (pFlashCmdParam->p_nf_cq_entry->bsDmaTotalLength * CW_INFO_SZ) / LOGIC_PG_SZ;
    FLASH_PHY tFlashAddr = { 0 };
    SIM_NFC_RED tCurrentRedun = { 0 }, tRedTemp = { 0 };

    for (U32 ulIndex = 0; ulIndex < ulAtomPageCount; ++ulIndex)
    {
        if (0 != ulIndex % pFlashCmdParam->bsPlnNum)
        {
            continue;
        }

        NFC_GetFlashAddr(pLunLocation,
                         pFlashCmdParam->phy_page_req[ulIndex % pFlashCmdParam->bsPlnNum].row_addr,
                         pFlashCmdParam->bsPageInWL + (ulIndex / pFlashCmdParam->bsPlnNum), pFlashCmdParam->bsIsTlcMode, &tFlashAddr);

        Mid_Read_RedData(&tFlashAddr, (char*)&tCurrentRedun);

        if(TRUE == pFlashCmdParam->p_nf_cq_entry->bsEMEn)
        {
#ifdef DATA_EM_ENABLE
            if (tFlashAddr.nBlock >= g_ulDataBlockStart[0][tFlashAddr.ucLunInTotal])
            {
                COM_MemCpy((U32*)&tRedTemp, (U32*)&tCurrentRedun, RED_SW_SZ_DW);
                NFC_TransEmRed(&tCurrentRedun, &tRedTemp);
            }
#endif
        }

        U32 ulPgType = NFC_GetSparePGType(&tCurrentRedun);
        if (TRUE != pFlashCmdParam->bsIsTlcMode)
        {
            if (PG_TYPE_FREE != ulPgType)
            {
                DBG_Printf("PhyPuInTotal %d Blk 0x%x Page 0x%x rewrite0 pg_type=%d\n", ulPhyPuInTotal, tFlashAddr.nBlock, tFlashAddr.nPage, ulPgType);
                DBG_Getch();
            }
        }
        else if ((NF_PRCQ_TLC_PRG_1ST_MULTIPLN == pFlashCmdParam->bsCmdCode) ||
            (NF_PRCQ_TLC_PRG_1ST == pFlashCmdParam->bsCmdCode) ||
            ((NF_PRCQ_TLC_PRG_1ST_LP_MULTIPLN == pFlashCmdParam->bsCmdCode) ||
                (NF_PRCQ_TLC_PRG_1ST_MP_MULTIPLN == pFlashCmdParam->bsCmdCode) ||
                (NF_PRCQ_TLC_PRG_1ST_UP_MULTIPLN == pFlashCmdParam->bsCmdCode)))
        {
            //DBG_Printf("PhyPuInTotal %d Blk 0x%x Page 0x%x rewrite pg_type=%d\n", ucPhyPuInTotal, flash_phy.nBlock, flash_phy.nPage, pg_type);
            if (PG_TYPE_FREE != ulPgType)
            {
                DBG_Printf("PhyPuInTotal %d Blk 0x%x Page 0x%x rewrite1 pg_type=%d\n", ulPhyPuInTotal, tFlashAddr.nBlock, tFlashAddr.nPage, ulPgType);
                DBG_Getch();
            }
        }
        else
        {
            //DBG_Printf("PhyPuInTotal %d Blk 0x%x Page 0x%x rewrite pg_type=%d\n", ucPhyPuInTotal, flash_phy.nBlock, flash_phy.nPage, pg_type);
            if (PG_TYPE_FREE != ulPgType)
            {
                DBG_Printf("PhyPuInTotal %d Blk 0x%x Page 0x%x rewrite2 pg_type=%d\n", ulPhyPuInTotal, tFlashAddr.nBlock, tFlashAddr.nPage, ulPgType);
                DBG_Getch();
            }
        }
    }
    return;
}

void NfcM_CmdExecute(ST_FLASH_CMD_PARAM *pFlashCmdParam)
{

    ASSERT(pFlashCmdParam != NULL);
    ASSERT(pFlashCmdParam->pLunLocation != NULL);
    ASSERT(pFlashCmdParam->pFlashReadretryParam != NULL);

    BOOL bResult = TRUE;
    U8 ucCmdType = pFlashCmdParam->bsCmdType;
    const NFCM_LUN_LOCATION *pLunLocation = pFlashCmdParam->pLunLocation;
    ST_FLASH_READ_RETRY_PARAM *pFlashReadretryParam = pFlashCmdParam->pFlashReadretryParam;
    FLASH_PHY flash_phy;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(pLunLocation);

    /*Process change column read related parameter*/
    if (NF_PRCQ_CCLR == pFlashCmdParam->bsCmdCode || NF_PRCQ_CCLR_MULTIPLN == pFlashCmdParam->bsCmdCode)
    {
        FlashStsM_CheckCCRParam(pLunLocation, pFlashCmdParam);
    }

    /*Get Lun tatal in Pu param by get row */
    NFC_GetFlashAddr(pLunLocation, pFlashCmdParam->p_nf_cq_entry->atRowAddr[0].bsRowAddr, 0, pFlashCmdParam->bsIsTlcMode, &flash_phy);


#if  defined(FLASH_INTEL_3DTLC) || defined (FLASH_IM_3DTLC_GEN2)
    #ifndef SWITCH_MODE_DADF
        pFlashCmdParam->bsIsTlcMode = g_SwitchSwMod[ucPhyPuInTotal][2];
    #else
        FlashStsM_SetLunMode(flash_phy.ucLunInTotal, !pFlashCmdParam->bsIsTlcMode);
    #endif
#endif

#if (defined(FLASH_TLC))
        FlashStsM_SetLunMode(flash_phy.ucLunInTotal, !pFlashCmdParam->bsIsTlcMode);
#endif

    switch (ucCmdType)
    {
        case CMD_CODE_READ:
        {
            if (TRUE == pFlashCmdParam->bsTrigOmEn) // part1: Otfb Read
            {
                bResult = NFC_OtfbPGReadCMD(pLunLocation, pFlashCmdParam, pFlashReadretryParam);
            }
            else                                     // part2: Dram Read
            {
                if (TRUE == pFlashCmdParam->bsDmaByteEn)
                {
                    bResult = NFC_ReadByteCMD(pLunLocation, pFlashCmdParam);
                }
                else
                {
                    bResult = NFC_PGReadCMD(pLunLocation, pFlashCmdParam, pFlashReadretryParam);
                }
            }

            break;
        }
        case CMD_CODE_PROGRAM:
        {
            NfcM_ReWriteCheck(pLunLocation, pFlashCmdParam);

            if (TRUE == pFlashCmdParam->bsInterCopyEn)
            {
                NFC_InternalCopyCmdExecute(pLunLocation, pFlashCmdParam);
            }
            else if (TRUE == pFlashCmdParam->bsTrigOmEn) // part1: Otfb Write
            {
                bResult = NFC_OtfbPGWriteCMD(pLunLocation, pFlashCmdParam);
            }
            else                                     // part2: Dram Write
            {
                bResult = NFC_PGWriteCMD(pLunLocation, pFlashCmdParam);
            }

            if (FALSE != bResult)
            {
                FLASH_PHY flash_phy;
                if (TRUE == pFlashCmdParam->bsInterCopyEn)
                {
                    NFC_GetFlashAddr(pLunLocation, pFlashCmdParam->phy_page_req[PG_PER_WL*PLN_PER_LUN].row_addr, pFlashCmdParam->bsPageInWL, pFlashCmdParam->bsIsTlcMode, &flash_phy);
                }
                else
                {
                    NFC_GetFlashAddr(pLunLocation, pFlashCmdParam->phy_page_req[0].row_addr, pFlashCmdParam->bsPageInWL, pFlashCmdParam->bsIsTlcMode, &flash_phy);
                }
                set_fcond(&flash_phy, (U32*)pFlashCmdParam->p_local_red, pFlashCmdParam->bsCmdType);
            }

            /*Clear previous command info*/
            FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_ERASE:
        {
            // Block Erase
            bResult = NFC_BlkEreaseCMD(pLunLocation, pFlashCmdParam);

            /*Clear previous command info*/
            FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_RST:
        {
            pFlashReadretryParam->read_retry_en = 0;
            pFlashReadretryParam->read_retry_current_time = 0;

            bResult = TRUE;

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case  CMD_CODE_READRETRY_EN:
        {
            pFlashReadretryParam->read_retry_en = TRUE;
            bResult = TRUE;

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_READRETRY_ADJ:
        {
            pFlashReadretryParam->read_retry_current_time++;
            bResult = TRUE;

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_SET_FEATURE:
        {
            g_SwitchSwMod[ucPhyPuInTotal][0] = pFlashCmdParam->p_nf_cq_entry->aByteAddr.usByteAddr;
            g_SwitchSwMod[ucPhyPuInTotal][1] = pFlashCmdParam->p_nf_cq_entry->ulSetFeatVal;

            if (0x91 == g_SwitchSwMod[ucPhyPuInTotal][0])
            {
                if (0 == g_SwitchSwMod[ucPhyPuInTotal][1] || 0x100 == g_SwitchSwMod[ucPhyPuInTotal][1])
                {
                    g_SwitchSwMod[ucPhyPuInTotal][2] = 0; // SLC Mode

                    #ifndef SWITCH_MODE_DADF
                    FlashStsM_SetLunMode(ucPhyPuInTotal, TRUE);
                    #endif
                }
                else
                {
                    g_SwitchSwMod[ucPhyPuInTotal][2] = 1; // TLC Mode

                    #ifndef SWITCH_MODE_DADF
                    FlashStsM_SetLunMode(ucPhyPuInTotal, FALSE);
                    #endif
                }

                if (0 == g_SwitchSwMod[ucPhyPuInTotal][1] || 4 == g_SwitchSwMod[ucPhyPuInTotal][1])
                {
                    g_bSquashMode = TRUE;
                }
            }

            // SetFeature (Normal/PIO)
            pFlashReadretryParam->read_retry_current_time = g_RetryTimes[ucPhyPuInTotal];
            if (g_RetryTimes[ucPhyPuInTotal] > 0)
            {
                pFlashReadretryParam->read_retry_en = TRUE;
            }
            else
            {
                pFlashReadretryParam->read_retry_en = FALSE;//Retry terminate
            }

            bResult = TRUE;

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_GET_FEATURE:
        {
            // Now GetFeature is not supported, just return true. (Normal/PIO)
            bResult = TRUE;

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_READID:
        {
            // Read Flash ID
            bResult = NFC_ReadFlashIDProcess(pLunLocation);

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        case CMD_CODE_OTHER:
        {
            // Other nosupported command, just return true.
            bResult = TRUE;

            /*Clear previous command info*/
            //FlashStsM_ClrPreCmdInfo(flash_phy.ucLunInTotal);
            break;
        }
        default:
        {
            DBG_Printf("CmdType invalid.\n");
            DBG_Getch();
        }
    }

    if (TRUE != bResult)
    {
        DBG_Printf("Cmd Execute Fail.\n");
        DBG_Getch();
    }

    return;
}

/************************************************
Function Name: NfcM_CmdFinish
Input:     physical pu number, flash_param
Output:     None
Description:
flash command finish, do operations according to flash mod config
*************************************************/
void NfcM_CmdFinish(ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    ASSERT(pFlashCmdParam != NULL);

    const NFCM_LUN_LOCATION *pLunLocation = pFlashCmdParam->pLunLocation;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(pLunLocation);

    //Copy redundant data no matter error occur.
    if (CMD_CODE_READ == pFlashCmdParam->bsCmdType)
    {
        if (TRUE == pFlashCmdParam->red_en || TRUE == pFlashCmdParam->bsRawReadEn)
        {
            NFC_InterfaceUpdateRedData(pFlashCmdParam);
        }
    }

    // trigger types : nfcq or checklist or flash-mode-auto-generate
    if (TRUE == pFlashCmdParam->module_inj_en)
    {
        //cq rp do not jump when error injection
        NFC_InterfaceSetErrParameter(pLunLocation, ((U8)pFlashCmdParam->module_inj_type));
        return;
    }

    // double-check the fcmd cnt.
    NFC_LogCmdCnt(pFlashCmdParam, pLunLocation);

    //Special Status Update: NOTE: FW check SSU to judge command finish or not, so SSU must updated after others
    if(TRUE == pFlashCmdParam->ssu1_en)
    {
        NFC_InterfaceUpdateSsu(pFlashCmdParam->p_nf_cq_entry->bsSsu1Addr,
            pFlashCmdParam->p_nf_cq_entry->bsSsu1Data,1, pFlashCmdParam->p_nf_cq_entry->bsSsu1Ontf);
    }
    if(TRUE == pFlashCmdParam->ssu0_en)
    {
        NFC_InterfaceUpdateSsu(pFlashCmdParam->p_nf_cq_entry->bsSsu0Addr,
            pFlashCmdParam->p_nf_cq_entry->bsSsu0Data,0, pFlashCmdParam->p_nf_cq_entry->bsSsu0Ontf);
    }

    NFC_InterfaceJumpCQRP(pLunLocation);

    //interrupt
    if(pFlashCmdParam->int_en)
    {
        EnterCriticalSection(&g_CHCriticalSection[pLunLocation->ucCh]);
        g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][pLunLocation->ucLunInPhyPu].bsIntSts = 1;
        LeaveCriticalSection(&g_CHCriticalSection[pLunLocation->ucCh]);

        NFC_InterfaceSetInt(pLunLocation);//need adjust to 3 core mode
    }

    return;
}

/************************************************
Function Name: NFC_ChannelSchedule
Input:     channel number
Output:     None
Description:
shedule function for one channel
*************************************************/
void NfcM_CmdProcess(const NFCM_LUN_LOCATION *pNfcOrgStruct)
{
    ASSERT(pNfcOrgStruct != NULL);

    ST_FLASH_CMD_PARAM *pFlashCmdParam = &flash_cmd_param[pNfcOrgStruct->ucCh][pNfcOrgStruct->ucPhyPuInCh][pNfcOrgStruct->ucLunInPhyPu];
    pFlashCmdParam->pFlashReadretryParam = &flash_readretry_param[pNfcOrgStruct->ucCh][pNfcOrgStruct->ucPhyPuInCh][pNfcOrgStruct->ucLunInPhyPu];
    pFlashCmdParam->pLunLocation = pNfcOrgStruct;

    NfcM_CmdParamFetch(pFlashCmdParam);

    NfcM_CmdParamCheck(pFlashCmdParam);

    NfcM_CmdErrInjection(pFlashCmdParam);

    NfcM_CmdExecute(pFlashCmdParam);

    NfcM_CmdFinish(pFlashCmdParam);

    return;
}
/************************************************
Function Name: sim_flash_cmd_send
Input:     None
Output:     None
Description:
The callback function for NFC cmd trigger
*************************************************/
//BOOL NFC_ModelParamInit(BOOL b_create_new, BOOL b_erase_new, BOOL b_check_data)
BOOL NFC_ModelParamInit()
{
    //U32 config_dw;
    //U8 dma_split_size;
    U8 ch;
    U16 i;
    U8 ucCh, ucPhyPuInCh, ucLunInPhyPu;
    U8 ucPhyPuInTotal;
    //U32 config_tmp;
    //int j;

    //init nfc dptr

    p_flash_ppust_reg->ulIdleBitMap  = 0xFFFFFFFF;
    p_flash_ppust_reg->ulErrorBitMap = 0;
    p_flash_ppust_reg->ulNotFullBitMap  = 0xFFFFFFFF;
    p_flash_ppust_reg->ulEmptyBitMap = 0xFFFFFFFF;

    //default value of logic pu status
    p_flash_lpust_reg->ulIdleBitMap  = 0xFFFFFFFF;
    p_flash_lpust_reg->ulErrorBitMap = 0;
    p_flash_lpust_reg->ulNotFullBitMap  = 0xFFFFFFFF;
    p_flash_lpust_reg->ulEmptyBitMap = 0xFFFFFFFF;

    for (i = 0; i < 4; ++i)
    {
        g_pModelLogicLunStsReg->aIdleReg[i].ulBitMap  = 0xFFFFFFFF;
        g_pModelLogicLunStsReg->aErrorReg[i].ulBitMap = 0;
        g_pModelLogicLunStsReg->aNotFullReg[i].ulBitMap = 0xFFFFFFFF;
        g_pModelLogicLunStsReg->aEmptyReg[i].ulBitMap = 0xFFFFFFFF;
    }


    for (ucCh = 0; ucCh < NFC_CH_TOTAL; ++ucCh)
    {
        for (ucPhyPuInCh = 0; ucPhyPuInCh < NFC_PU_PER_CH; ++ucPhyPuInCh)
        {
            for (ucLunInPhyPu = 0; ucLunInPhyPu < NFC_LUN_MAX_PER_PU; ++ucLunInPhyPu)
            {
                ucPhyPuInTotal     = ucPhyPuInCh * NFC_CH_TOTAL + ucCh;
                *(U32*)&(g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu]) = 0;
                g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsEmpty = 1;
                g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsIdle = 1;
                g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsFsLv0 = 1;
                g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsFsLv1 = 1;
                g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsFsLv2 = 1;
                g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsFsLv3 = 1;

                if (ucLunInPhyPu >= NFC_LUN_PER_PU)
                {
                    g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu].bsFull = 1;
                }

                NFC_UpdateLogicLunSwSts(&g_pModelNfcCmdSts->aNfcqCmdStsReg[ucPhyPuInTotal][ucLunInPhyPu], ucPhyPuInTotal, ucLunInPhyPu, 0xF);
            }
        }
    }

    for(ch = 0;ch < NFC_CH_TOTAL;ch++)
    {
        for (i=0; i<NFC_PU_PER_CH; i++)
        {
            for (ucLunInPhyPu = 0; ucLunInPhyPu < NFC_LUN_MAX_PER_PU; ++ucLunInPhyPu)
            {
                pu_param[ch][i].pu_status = PU_STATUS_WAIT_CMD;
                pu_param[ch][i].timer_start = 0;
                pu_param[ch][i].timer_busy = 0;
                pu_param[ch][i].timer_status = TIMER_STATUS_END;

                flash_readretry_param[ch][i][ucLunInPhyPu].read_retry_current_time = 0;
                flash_readretry_param[ch][i][ucLunInPhyPu].read_retry_en = 0;
                flash_readretry_param[ch][i][ucLunInPhyPu].read_retry_rsvd = 0;
                flash_readretry_param[ch][i][ucLunInPhyPu].read_retry_success_time = 0;
            }
        }
#ifndef IGNORE_PERFORMANCE
        bus_status[ch] = BUS_STATUS_FREE;
        memset((void *)l_aSgeOtfbSts, SGE_OTFB_WAIT_PCIE_BUS, NFC_CH_TOTAL * NFC_PU_PER_CH * sizeof(U8));
        memset((void *)l_aOtfbBuffSts, OTFB_BUFF_FREE, NFC_CH_TOTAL * NFC_PU_PER_CH * sizeof(U8));
        memset((void *)ucPreOtfbSecCnt, 0, NFC_CH_TOTAL * NFC_PU_PER_CH * sizeof(U8));
#endif
    }

    NFC_InitLastFlashAddr();
    //NFC_CalcBlkBits();

    return 0;
}

BOOL NFC_SetErrInjEntry(const NFCM_LUN_LOCATION *tNfcOrgStruct, U16 block, U16 page, U8 cmd_code,U8 err_type,U8 index,U8 retry_times,U8 RedOnly)
{
    U8 nLoop;
    U8 bRtn = FALSE;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);

    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        if (g_stFlashErrInjEntry[nLoop].valid)
        {
            if (g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal == ucPhyPuInTotal &&
                g_stFlashErrInjEntry[nLoop].block == block &&
                g_stFlashErrInjEntry[nLoop].page == page &&
                g_stFlashErrInjEntry[nLoop].cmd_code == cmd_code &&
                g_stFlashErrInjEntry[nLoop].err_type == err_type &&
                g_stFlashErrInjEntry[nLoop].retry_times == retry_times &&
                g_stFlashErrInjEntry[nLoop].red_only == RedOnly)
            {
                break;
            }
            continue;
        }
        else
        {
            g_stFlashErrInjEntry[nLoop].valid = TRUE;
            g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal = ucPhyPuInTotal;
            g_stFlashErrInjEntry[nLoop].block = block;
            g_stFlashErrInjEntry[nLoop].page = page;
            g_stFlashErrInjEntry[nLoop].cmd_code = cmd_code;
            g_stFlashErrInjEntry[nLoop].err_type = err_type;
            g_stFlashErrInjEntry[nLoop].retry_times = retry_times;
            g_stFlashErrInjEntry[nLoop].red_only = RedOnly;
            bRtn = TRUE;
            break;
        }
    }
    return bRtn;

}

void NFC_ErrorInjCmd(ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U32 nLoop;
    U8 phyPageIndex;
    FLASH_PHY flash_phy;
    U32 ulMaxLoopCnt = 0;
    const NFCM_LUN_LOCATION *pLunLocation = pFlashCmdParam->pLunLocation;
    ST_FLASH_READ_RETRY_PARAM *pFlashReadretryParam = pFlashCmdParam->pFlashReadretryParam;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(pLunLocation);
    U8 ucErrCode = NF_SUCCESS;
    U8 ucStartRow = 0;
    U8 ucEndRow = 0;
    U8 ucCmdCode = pFlashCmdParam->bsCmdType;

    if (CMD_CODE_READ == ucCmdCode)
    {
        ucStartRow = pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecStart  / SEC_PER_LOGIC_PG;
        ucEndRow = (pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecStart + pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecLength + SEC_PER_LOGIC_PG - 1) / SEC_PER_LOGIC_PG;
    }
    else if (CMD_CODE_PROGRAM == ucCmdCode)
    {
        ucStartRow = 0;
        ucEndRow = (pFlashCmdParam->p_nf_cq_entry->bsDmaTotalLength * CW_INFO_SZ) / LOGIC_PG_SZ;
    }
    else if (CMD_CODE_ERASE == ucCmdCode)
    {
        ucEndRow = PLN_PER_LUN;
    }

    if (g_InjErrParamType == INTERNAL_FLASH_PARAM)
    {
        ulMaxLoopCnt = ERR_INJ_TABLE_MAX;
    }
    else
    {
        ulMaxLoopCnt = g_InjErrHead;
    }

    // 1.lookup FlashErrInjEntry
    for (nLoop = 0; nLoop < ulMaxLoopCnt; nLoop++)
    {
        if (!g_stFlashErrInjEntry[nLoop].valid)
        {
            continue;
        }
        else
        {
            //for (phyPageIndex = 0; phyPageIndex < 8; phyPageIndex++)
            for (phyPageIndex = ucStartRow; phyPageIndex < ucEndRow; phyPageIndex++)
            {
                //if (phyPageIndex >= (pFlashCmdParam->xfer_sec_cnt / SEC_PER_LOGIC_PG))
                //{
                //    break;
                //}

                NFC_GetFlashAddr(pLunLocation, pFlashCmdParam->phy_page_req[phyPageIndex].row_addr,
                    pFlashCmdParam->phy_page_req[phyPageIndex].part_in_wl, pFlashCmdParam->bsIsTlcMode, &flash_phy);

                if ((pFlashCmdParam->bsCmdType == g_stFlashErrInjEntry[nLoop].cmd_code)
                    && (ucPhyPuInTotal == g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal)
                    && (flash_phy.nBlock == g_stFlashErrInjEntry[nLoop].block)
                    && (flash_phy.nPage == g_stFlashErrInjEntry[nLoop].page)
                    && (pFlashCmdParam->red_only == g_stFlashErrInjEntry[nLoop].red_only))
                {
                    Mid_FlashInjError(&flash_phy, g_stFlashErrInjEntry[nLoop].err_type, g_stFlashErrInjEntry[nLoop].retry_times);
                    //break;
                }
            }
        }
    }

    // 2.is hit error injection param
    IsHitErrInjParam(pLunLocation, pFlashCmdParam, pFlashReadretryParam);
}


void NFC_ResetErrInjEntry()
{
    U32 nLoop = 0;
    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        g_stFlashErrInjEntry[nLoop].valid = FALSE;
        g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal = 0;
        g_stFlashErrInjEntry[nLoop].block = 0;
        g_stFlashErrInjEntry[nLoop].page = 0;
        g_stFlashErrInjEntry[nLoop].cmd_code = 0;
        g_stFlashErrInjEntry[nLoop].err_type = 0;
        g_stFlashErrInjEntry[nLoop].retry_times = 0;
    }
}
void NFC_ClearInjErrEntry(U8 Pu,U16 PhyBlk,U16 Page)
{
    U16 Pg;
    U32 nLoop;

    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        if (!g_stFlashErrInjEntry[nLoop].valid)
            continue;
        else
        {
            if(Pu != g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal)
                continue;
            if(PhyBlk != g_stFlashErrInjEntry[nLoop].block)
                continue;
            if((Page != INVALID_4F) && (Page != g_stFlashErrInjEntry[nLoop].page))
                continue;

            if(CMD_CODE_ERASE == g_stFlashErrInjEntry[nLoop].cmd_code)
                continue;

            Pg = g_stFlashErrInjEntry[nLoop].page;
            g_stFlashErrInjEntry[nLoop].valid = FALSE;
            g_stFlashErrInjEntry[nLoop].ucPhyPuInTotal = 0;
            g_stFlashErrInjEntry[nLoop].block = 0;
            g_stFlashErrInjEntry[nLoop].page = 0;
            g_stFlashErrInjEntry[nLoop].cmd_code = 0;
            g_stFlashErrInjEntry[nLoop].err_type = 0;
            g_stFlashErrInjEntry[nLoop].retry_times = 0;
            DBG_Printf("Pu %d PhyBlk 0x%x Pg 0x%x Index %d Inject Error is Cleared!\n", Pu, PhyBlk, Pg, nLoop);
            RecInjErrFile(hFileInjErr,"\tPu %2d PhyBlk %4d Pg %2d Inject Error is Cleared!\n",Pu,PhyBlk,Pg);
        }
    }
}