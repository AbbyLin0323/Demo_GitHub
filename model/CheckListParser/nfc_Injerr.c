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

#include "sim_nfccmd.h"
#include "HAL_MemoryMap.h"
#include "checklist_parse.h"
#include "memory_access.h"
#include "Sim_DEC_Status.h"
#include "flash_opinfo.h"

#ifdef DBG_TABLE_REBUILD
#include "L2_SimTablecheck.h"
#endif

extern U16 HAL_GetLowPageIndex(U16 usHighPage);
extern PAIR_PAGE_TYPE HAL_GetFlashPairPageType(U16 usPage);

extern ERR_INJ_PARRM ErrParamSlot[ERR_INJ_SLOT_MAX];
extern BOOL g_bInjectError;
extern BOOL load_one_fcondlist(COND_LIST **pFCond);

void RecInjErrFile(HANDLE hFile,char* fmt,...)
{
    unsigned int haswrite;
    char buffer[512];

    va_list ap;
    va_start(ap,fmt);

    vsprintf_s(buffer, 512, fmt,ap);

    WriteFile(hFile,(LPVOID)(&buffer), (DWORD)strlen(buffer), &haswrite, NULL);

    va_end(ap);
}

void Dump_InjectErrorInfo(FLASH_PHY* pflash_phy1,U8 PageType,U8 ErrType,U8 RetryTime)
{
    RecInjErrFile(hFileInjErr,"PhyPuInTotal %2d Pln %2d PhyBlk %4d Page %3d PageType:%d,ErrCode:%d,RetryTime:%d\n",
        pflash_phy1->bsPhyPuInTotal, pflash_phy1->nPln, pflash_phy1->nBlock,pflash_phy1->nPage,PageType,ErrType,RetryTime);
    return;
}

extern BOOL IsDuringTableRebuild();
void set_fcond(FLASH_PHY* pflash_phy1,U32* pRedData,U32 CmdCode)
{
    U32 pg_type = 0;
    U8 op_type = 0;
    COND_LIST *pFCond = NULL;

    if (TRUE == IsDuringTableRebuild())
    {
        return;
    }

    do
    {
        load_one_fcondlist(&pFCond);
        if (NULL != pFCond && pFCond->CurCond.FCond.bNoCond != TRUE)
        {

            if(pRedData != NULL)
            {
                pg_type = NFC_GetSparePGType((SIM_NFC_RED*)pRedData);
                op_type = NFC_GetSpareOpType(pRedData);
            }
            if(CmdCode != pFCond->CurCond.FCond.CmdCode)
                return;

            if(pg_type != pFCond->CurCond.FCond.PageType)
                return;

            if(op_type != pFCond->CurCond.FCond.OpType)
                return;

            if(pFCond->CurCond.FCond.CE != INVALID_2F && pFCond->CurCond.FCond.CE != pflash_phy1->ucLunInTotal)
                return;
            if(pFCond->CurCond.FCond.PhyBlk != INVALID_4F && pFCond->CurCond.FCond.PhyBlk != pflash_phy1->nBlock)
                return;
            if(pFCond->CurCond.FCond.PhyPg != INVALID_4F && pFCond->CurCond.FCond.PhyPg != pflash_phy1->nPage)
                return;

            pFCond->CurCond.FCond.FCmdConDoCnt++;
        }
    }while (NULL != pFCond);

    return;
}

// updated by jasonguo & ninayang 20140815
extern void Comm_ReadOtfb(U32 addr, U32 nWords, U32 *buf);
extern U16 GetPPOFromPhyPage(U16 PhyPage);

void IsHitAbnormalPowerDownPairPageUECC(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* p_flash_param, ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param, SIM_NFC_RED* pRedData)
{
    FLASH_PHY flash_phy;
    U32 RetryTimes = 8;
    U16 usLastPage;
    U16 usLowPage = INVALID_4F;

    NFC_GetFlashAddr(tNfcOrgStruct, p_flash_param->phy_page_req[0].row_addr, p_flash_param->phy_page_req[0].part_in_wl, p_flash_param->bsIsTlcMode, &flash_phy);

    usLastPage = g_LastHandleAddr[flash_phy.ucLunInTotal].nPage;
    if (HIGH_PAGE == HAL_GetFlashPairPageType(usLastPage))
    {
        usLowPage = HAL_GetLowPageIndex(usLastPage);
    }
    else
    {
        return;
    }

    if (flash_phy.ucLunInTotal== g_LastHandleAddr[flash_phy.ucLunInTotal].ucLunInTotal &&
        flash_phy.nBlock == g_LastHandleAddr[flash_phy.ucLunInTotal].nBlock &&
        flash_phy.nPage == g_LastHandleAddr[flash_phy.ucLunInTotal].nPage &&
        CMD_CODE_READ == p_flash_param->bsCmdType &&
        PAGE_TYPE_DATA == pRedData->m_RedComm.bcPageType)
    {
        if (NFC_SetErrInjEntry(tNfcOrgStruct, flash_phy.nBlock, flash_phy.nPage, CMD_CODE_READ, NF_ERR_TYPE_UECC, 0, RetryTimes,0))
        {
            g_bInjectError = TRUE;

            Mid_FlashInjError(&flash_phy, NF_ERR_TYPE_UECC, RetryTimes);
            //Dump_InjectErrorInfo(&flash_phy, pRedData->m_RedComm.bcPageType, NF_ERR_TYPE_UECC, RetryTimes);
        }

    }

    //inject low page UECC
    if (flash_phy.ucLunInTotal == g_LastHandleAddr[flash_phy.ucLunInTotal].ucLunInTotal &&
        flash_phy.nBlock == g_LastHandleAddr[flash_phy.ucLunInTotal].nBlock &&
        flash_phy.nPage == usLowPage &&
        CMD_CODE_READ == p_flash_param->bsCmdType &&
        PAGE_TYPE_DATA == pRedData->m_RedComm.bcPageType)
    {
        if (NFC_SetErrInjEntry(tNfcOrgStruct, flash_phy.nBlock, flash_phy.nPage, CMD_CODE_READ, NF_ERR_TYPE_UECC, 0, RetryTimes,0))
        {
            g_bInjectError = TRUE;

            Mid_FlashInjError(&flash_phy, NF_ERR_TYPE_UECC, RetryTimes);
            //Dump_InjectErrorInfo(&flash_phy, pRedData->m_RedComm.bcPageType, NF_ERR_TYPE_UECC, RetryTimes);
        }
    }

    return;
}

void IsHitAbnormalPowerDownError(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* p_flash_param, ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param, SIM_NFC_RED* pRedData)
{
    FLASH_PHY flash_phy;
    U32 RetryTimes = 8;

    NFC_GetFlashAddr(tNfcOrgStruct, p_flash_param->phy_page_req[0].row_addr, p_flash_param->phy_page_req[0].part_in_wl, p_flash_param->bsIsTlcMode, &flash_phy);

    if (flash_phy.ucLunInTotal== g_LastHandleAddr[flash_phy.ucLunInTotal].ucLunInTotal &&
        flash_phy.nBlock == g_LastHandleAddr[flash_phy.ucLunInTotal].nBlock &&
        flash_phy.nPage == g_LastHandleAddr[flash_phy.ucLunInTotal].nPage &&
        CMD_CODE_READ == p_flash_param->bsCmdType &&
        PAGE_TYPE_DATA == pRedData->m_RedComm.bcPageType)
    {
        if (NFC_SetErrInjEntry(tNfcOrgStruct, flash_phy.nBlock, flash_phy.nPage, CMD_CODE_READ, NF_ERR_TYPE_UECC, 0, RetryTimes, 0))
        {
            g_bInjectError = TRUE;

            Mid_FlashInjError(&flash_phy, NF_ERR_TYPE_UECC, RetryTimes);
            //Dump_InjectErrorInfo(&flash_phy, pRedData->m_RedComm.bcPageType, NF_ERR_TYPE_UECC, RetryTimes);

#if 0//def DBG_TABLE_REBUILD
            {
                U16 VirBlk = INVALID_4F;
                U16 usPPO = INVALID_4F;
                VirBlk = pRedData->m_RedComm.bsVirBlockAddr;

                if (VirBlk != INVALID_4F && (U8)pRedData->m_RedComm.bcPageType == PAGE_TYPE_DATA)
                {
                    if (BLOCK_TYPE_SLC == pRedData->m_RedComm.bcBlockType)
                    {
                        usPPO = GetPPOFromPhyPage(flash_phy.nPage);
                        if (INVALID_4F == usPPO)
                        {
                            DBG_Getch();
                        }
                    }
                    else
                    {
                        usPPO = flash_phy.nPage;
                    }
                    L2_AddErrAddrOfCheckPMT((U8)flash_phy.nPU, VirBlk, usPPO);
                }
            }
#endif

        }

    }
}

void IsHitErrInjParam(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* p_flash_param,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    U8 pg_type = 0;
    U8 ucRowIndex = 0;
    ERR_INJ_PARRM* pErrParam;
    U32 i;
    SIM_NFC_RED* pRedData;
    U32 CmdCode;
    FLASH_PHY flash_phy;
    U32* p_addr_red = NULL;
    U8 ucPhyPuInTotal = NfcM_GetPhyPuInTotal(tNfcOrgStruct);
    U8 usErrCode = NF_SUCCESS;
    U8 ucStartRow, ucEndRow;

    if(g_InjErrHead == g_InjErrTail)
    {
        return;
    }

    if (CMD_CODE_READ == p_flash_param->bsCmdType)
    {
        ucStartRow = p_flash_param->p_nf_cq_entry->aSecAddr[0].bsSecStart / SEC_PER_LOGIC_PG;
        ucEndRow = (p_flash_param->p_nf_cq_entry->aSecAddr[0].bsSecStart + p_flash_param->p_nf_cq_entry->aSecAddr[0].bsSecLength + SEC_PER_LOGIC_PG - 1) / SEC_PER_LOGIC_PG;
    }
    else if (CMD_CODE_PROGRAM == p_flash_param->bsCmdType)
    {
        ucStartRow = 0;
        ucEndRow = (p_flash_param->p_nf_cq_entry->bsDmaTotalLength * CW_INFO_SZ) / LOGIC_PG_SZ;
    }
    else if (CMD_CODE_ERASE == p_flash_param->bsCmdType)
    {
        ucStartRow= 0;
        ucEndRow = PLN_PER_LUN;
    }

    pRedData = (SIM_NFC_RED*)p_flash_param->p_local_red;
    if (CMD_CODE_READ == p_flash_param->bsCmdType)
    {
        ucRowIndex = 0;

        NFC_GetFlashAddr(tNfcOrgStruct, p_flash_param->phy_page_req[ucRowIndex].row_addr,
            p_flash_param->phy_page_req[ucRowIndex].part_in_wl, p_flash_param->bsIsTlcMode, &flash_phy);

        p_addr_red = &pRedData->m_Content[0];
        Mid_Read_RedData(&flash_phy, (char*)p_addr_red);
    }
    else if (CMD_CODE_PROGRAM == p_flash_param->bsCmdType)
    {
        if (TRUE == p_flash_param->p_nf_cq_entry->bsRedEn)
        {
            ucRowIndex = 0;

            if (p_flash_param->p_nf_cq_entry->bsRedOntf == TRUE)
            {
                Comm_ReadOtfb(p_flash_param->p_red_addr, sizeof(SIM_NFC_RED) / sizeof(U32), (U32 *)pRedData);
            }
            else
            {
                Comm_ReadDram(g_ulModelRedOffSetInDram + p_flash_param->p_red_addr,
                    sizeof(SIM_NFC_RED) / sizeof(U32), (U32 *)pRedData);
            }
        }
        else
        {
            ucRowIndex = 6; // for tlc internal one time copy
            pRedData = NULL;
        }
    }

    if(NULL != pRedData)
    {
        pg_type = (U8)pRedData->m_RedComm.bcPageType;
        if (PG_TYPE_FREE == pg_type && CMD_CODE_ERASE != p_flash_param->bsCmdType)
        {
            return;
        }
    }

    for(i = 0; i < g_InjErrTail; i++)
    {
        pErrParam = &ErrParamSlot[i];

        if (pErrParam->ParamType == SPOR_LAST_PAGE_UECC_FLASH_PARAM)
        {
            IsHitAbnormalPowerDownError(tNfcOrgStruct, p_flash_param, p_flash_readrety_param, pRedData);
            g_InjErrParamType = INTERNAL_FLASH_PARAM;
            return;
        }
        else if (pErrParam->ParamType == SPOR_PAIR_PAGE_UECC_FLASH_PARAM)
        {
            IsHitAbnormalPowerDownPairPageUECC(tNfcOrgStruct, p_flash_param, p_flash_readrety_param, pRedData);
            g_InjErrParamType = INTERNAL_FLASH_PARAM;
            return;
        }

        g_InjErrParamType = EXTERNAL_FLASH_PARAM;

        if(TRUE == pErrParam->bHit)
        {
            continue;
        }

        CmdCode = p_flash_param->bsCmdType;
        if(CmdCode != pErrParam->CmdCode)
        {
            continue;
        }
        if(CmdCode == CMD_CODE_READ || CmdCode == CMD_CODE_PROGRAM)
        {
            if(pg_type != pErrParam->PageType && pg_type != 0)
            {
                continue;
            }
        }

        if (TRUE == p_flash_param->bsInterCopyEn)
        {
            NFC_GetFlashAddr(tNfcOrgStruct, p_flash_param->phy_page_req[PG_PER_WL*PLN_PER_LUN].row_addr, p_flash_param->bsPageInWL, p_flash_param->bsIsTlcMode, &flash_phy);
        }
        else
        {
            NFC_GetFlashAddr(tNfcOrgStruct, p_flash_param->phy_page_req[ucRowIndex].row_addr,
                p_flash_param->phy_page_req[ucRowIndex].part_in_wl, p_flash_param->bsIsTlcMode, &flash_phy);
        }

        if(pErrParam->CE != INVALID_2F && pErrParam->CE != flash_phy.ucLunInTotal)
        {
            continue;
        }
        if(pErrParam->PhyBlk != INVALID_4F && pErrParam->PhyBlk != flash_phy.nBlock )
        {
            continue;
        }
        if(pErrParam->PhyPg != INVALID_4F && pErrParam->PhyPg != flash_phy.nPage)
        {
            continue;
        }

        if (pErrParam->RedOnly == 1 && p_flash_param->red_only != 1)
        {
            continue;
        }

        if(NFC_SetErrInjEntry(tNfcOrgStruct,flash_phy.nBlock,flash_phy.nPage,CmdCode,pErrParam->ErrType,0, pErrParam->RetryTime,pErrParam->RedOnly))
        {
            g_bInjectError = TRUE;
            pErrParam->bHit = TRUE;
            g_InjErrHead++;

            //for (ucRowIndex = 0; ucRowIndex < (p_flash_param->xfer_sec_cnt / SEC_PER_LOGIC_PG); ucRowIndex++)
            for (ucRowIndex = ucStartRow; ucRowIndex < ucEndRow; ucRowIndex++)
            {
                NFC_GetFlashAddr(tNfcOrgStruct, p_flash_param->phy_page_req[ucRowIndex].row_addr,
                    p_flash_param->phy_page_req[ucRowIndex].part_in_wl, p_flash_param->bsIsTlcMode, &flash_phy);

                Dump_InjectErrorInfo(&flash_phy,pErrParam->PageType,pErrParam->ErrType,pErrParam->RetryTime);

                /* checklist hit NFC error, mark a error */
                Mid_FlashInjError(&flash_phy, pErrParam->ErrType, pErrParam->RetryTime);

                if(NF_ERR_TYPE_UECC == pErrParam->ErrType)
                {
                    //p_flash_readrety_param->read_retry_success_time = pErrParam->RetryTime;

                #ifdef DBG_TABLE_REBUILD
                    {
                        U16 VirBlk = INVALID_4F;
                        U16 usPPO = INVALID_4F;
                        SIM_NFC_RED red;
                        Mid_Read_RedData(&flash_phy,(char*)&red);
                        VirBlk = red.m_RedComm.bsVirBlockAddr;
                        if(VirBlk != INVALID_4F && (U8)red.m_RedComm.bcPageType == PAGE_TYPE_DATA)
                        {
                            if (BLOCK_TYPE_SEQ == red.m_RedComm.bcBlockType || BLOCK_TYPE_RAN == red.m_RedComm.bcBlockType)
                            {
                                usPPO = GetPPOFromPhyPage(flash_phy.nPage);
                                if(INVALID_4F == usPPO)
                                {
                                    DBG_Getch();
                                }
                            }
                            else
                            {
                                usPPO = flash_phy.nPage;
                            }
                            L2_AddErrAddrOfCheckPMT(ucPhyPuInTotal, VirBlk, usPPO);
                        }
                    }
                #endif
                }
            }

            if (CmdCode == CMD_CODE_ERASE && NF_ERR_TYPE_ERS == pErrParam->ErrType)
            {
                Dump_InjectErrorInfo(&flash_phy,pErrParam->PageType,pErrParam->ErrType,pErrParam->RetryTime);

                /* checklist hit NFC Erase error, mark a error */
                Mid_FlashInjError(&flash_phy, pErrParam->ErrType, pErrParam->RetryTime);


            }
            break;
        }
    }
    return;
}

BOOL parse_injecterror_param(ACTION* pAct, char* param)
{
    char pagetype[256];
    char cmdcode[256];
    char realParam[256] = { '0' };
    char* pdest;
    char CE[4];
    char Blk[6];
    char Page[128];
    char errtype[64];
    char retrytime[64];
    char redOnly[64] = { '0' };
    char cTocken = ',';
    U32 ulParaCnt;
    U8 index;

    char *pParamArray[] = { CE, Blk, Page, cmdcode, pagetype, errtype, retrytime, redOnly };

    if (pAct->ActType == ACT_TYPE_INJECT_ERROR)
    {
        pAct->m_ErrInjParam.CE = 0xFF;
        pAct->m_ErrInjParam.PhyBlk = INVALID_4F;
        pAct->m_ErrInjParam.PhyPg = INVALID_4F;
        pAct->m_ErrInjParam.bHit = FALSE;

        pdest = strchr(param, '}');
        index = (U8)(pdest - param);
        memcpy(realParam, param + 1, index - 1);

        ulParaCnt = GetParameters(realParam, &cTocken, pParamArray, 8);
        if (strncmp(CE, "?", 1))
        {
            pAct->m_ErrInjParam.CE = atoi(CE);
        }
        if (strncmp(Blk, "?", 1))
        {
            pAct->m_ErrInjParam.PhyBlk = atoi(Blk);
        }

        if (!strncmp(Page, "?", 1))
        {
            ;
        }
        else if (!strncmp(Page, "SPO_LastPage_UECC", 8))
        {
            pAct->m_ErrInjParam.ParamType = SPOR_LAST_PAGE_UECC_FLASH_PARAM;// INTERNAL_FLASH_PARAM;
        }
        else if (!strncmp(Page, "SPOR_PairPage_UECC", 16))
        {
            pAct->m_ErrInjParam.ParamType = SPOR_PAIR_PAGE_UECC_FLASH_PARAM;// INTERNAL_FLASH_PARAM;
        }
        else if (!strncmp(Page, "R?", 2))
        {
            pAct->m_ErrInjParam.ParamType = EXTERNAL_FLASH_PARAM;
            pAct->m_ErrInjParam.PhyPg = rand() % (PG_PER_BLK - 1);
        }
        else
        {
            pAct->m_ErrInjParam.ParamType = EXTERNAL_FLASH_PARAM;
            pAct->m_ErrInjParam.PhyPg = atoi(Page);
        }
        if (!strncmp(cmdcode, "Read", 4))
        {
            pAct->m_ErrInjParam.CmdCode = CMD_CODE_READ;
        }
        else if (!strncmp(cmdcode, "Write", 5))
        {
            pAct->m_ErrInjParam.CmdCode = CMD_CODE_PROGRAM;
        }
        else if (!strncmp(cmdcode, "Erase", 5))
        {
            pAct->m_ErrInjParam.CmdCode = CMD_CODE_ERASE;
        }

        if (FALSE == get_pagetype(pagetype, &pAct->m_ErrInjParam.PageType))
        {
            printf("Not find pagetype %s\n", pagetype);
            DBG_Break();
        }
        if (FALSE == get_errtype(errtype, &pAct->m_ErrInjParam.ErrType))
        {
            printf("Not find pagetype %s\n", errtype);
            DBG_Break();
        }

        pAct->m_ErrInjParam.RetryTime = atoi(retrytime);
        if (!strncmp(redOnly, "RED", 3))
        {
            pAct->m_ErrInjParam.RedOnly = 1;
        }

        return TRUE;

    }
    else
    {
        return FALSE;
    }
}

BOOL parse_injectCRCerror_param(ACTION* pAct, char* param)
{
    char cmdcode[256];
    char realParam[256] = { '0' };
    char* pdest;
    char LBA[128];
    char errtype[64];
    char cTocken = ',';
    U32 ulParaCnt;
    U8 index;

    //FC1_A1_P1 = {0}:{InjectCRCError}: {1,24,333,50,Read,CRC}

    char *pParamArray[] = { LBA, cmdcode, errtype };

    if (pAct->ActType == ACT_TYPE_INJECT_CRCERR)
    {
        pAct->m_ErrInjParam.CE = 0xFF;
        pAct->m_ErrInjParam.PhyBlk = INVALID_4F;
        pAct->m_ErrInjParam.PhyPg = INVALID_4F;
        pAct->m_ErrInjParam.bHit = FALSE;

        pdest = strchr(param, '}');
        index = (U8)(pdest - param);
        memcpy(realParam, param + 1, index - 1);

        ulParaCnt = GetParameters(realParam, &cTocken, pParamArray, 3);

        if (strncmp(LBA, "?", 1))
        {
            pAct->m_ErrInjParam.LBA = atoi(LBA);
        }

        if (!strncmp(cmdcode, "Read", 4))
        {
            pAct->m_ErrInjParam.CmdCode = CMD_CODE_READ;
        }
        else if (!strncmp(cmdcode, "Write", 5))
        {
            pAct->m_ErrInjParam.CmdCode = CMD_CODE_PROGRAM;
        }

        if (FALSE == get_errtype(errtype, &pAct->m_ErrInjParam.ErrType))
        {
            printf("Not find pagetype %s\n", errtype);
            DBG_Break();
        }

        return TRUE;

    }
    else
    {
        return FALSE;
    }
}

void parse_flashcond_action(COND_LIST* tmp_condlist,char* cond,char* action,char* param)
{
    U32 retrytime = 0;
    char optype[128];
    U32 FcondHitCnt = 0;
    char pagetype[256];
    char cmdcode[256];
    char CE[4];
    char Blk[4];
    char Page[4];

    tmp_condlist->CurCond.FCond.CE = INVALID_2F;
    tmp_condlist->CurCond.FCond.PhyBlk = INVALID_4F;
    tmp_condlist->CurCond.FCond.PhyPg = INVALID_4F;
    tmp_condlist->CurCond.FCond.PPOType = PPO_TYPE_ALL;

    if(!strncmp(cond,"{0}",3))
    {
        tmp_condlist->CurCond.FCond.bNoCond = TRUE;
    }
    else if(sscanf(cond,"{%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%d}",CE,Blk,Page,cmdcode,pagetype,optype,&FcondHitCnt))
    {
        if(strncmp(CE,"?",1))
        {
            tmp_condlist->CurCond.FCond.CE = atoi(CE);
        }
        if(strncmp(Blk,"?",1))
        {
            tmp_condlist->CurCond.FCond.PhyBlk = atoi(Blk);
        }

        if(!strncmp(Page,"?",1))
        {
            ;
        }
        else if(!strncmp(Page,"R?",2))
        {
            tmp_condlist->CurCond.FCond.PhyPg = rand()%(PG_PER_BLK-1);
            tmp_condlist->CurCond.FCond.PPOType = RAND_PPO;
        }
        else if(!strncmp(Page,"M2",2))
        {
            tmp_condlist->CurCond.FCond.PPOType = MOD2_PPO;
        }
        else
        {
            tmp_condlist->CurCond.FCond.PhyPg = atoi(Page);
        }
        if(!strncmp(cmdcode,"Read",4))
        {
            tmp_condlist->CurCond.FCond.CmdCode = CMD_CODE_READ;
        }
        else if(!strncmp(cmdcode,"Write",5))
        {
            tmp_condlist->CurCond.FCond.CmdCode = CMD_CODE_PROGRAM;
        }
        tmp_condlist->CurCond.FCond.FCmdConHitCnt = FcondHitCnt;
        if(FALSE == get_pagetype(pagetype,&tmp_condlist->CurCond.FCond.PageType))
        {
            printf("Not find pagetype %s\n",pagetype);
            DBG_Break();
        }
        if(FALSE == get_optype(optype,&tmp_condlist->CurCond.FCond.OpType))
        {
            printf("Not find optype %s\n",optype);
            DBG_Break();
        }

    }
    parse_action(tmp_condlist,action,param);

    return;
}
