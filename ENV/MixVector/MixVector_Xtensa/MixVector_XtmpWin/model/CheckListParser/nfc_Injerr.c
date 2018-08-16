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

#ifdef DBG_TABLE_REBUILD   
#include "L2_SimTablecheck.h"
#endif

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
    RecInjErrFile(hFileInjErr,"CE %2d PhyBlk %4d Page %3d PageType:%d,ErrCode:%d,RetryTime:%d\n",
        pflash_phy1->nPU,pflash_phy1->nBlock,pflash_phy1->nPage,PageType,ErrType,RetryTime); 
    return;
}

void set_fcond(FLASH_PHY* pflash_phy1,U32* pRedData,U32 CmdCode)
{
    U32 pg_type = 0;    
    U8 op_type = 0;
    COND_LIST *pFCond = NULL;

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

            if(pFCond->CurCond.FCond.CE != INVALID_2F && pFCond->CurCond.FCond.CE != pflash_phy1->nPU)
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
extern U32* g_pDataBufferOut;
extern void Comm_ReadOtfb(U32 addr, U32 nWords, U32 *buf);
extern U16 GetPPOFromPhyPage(U16 PhyPage);
void IsHitErrInjParam(U8 Pu,ST_FLASH_CMD_PARAM* p_flash_param,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    U8 pg_type = 0;
    ERR_INJ_PARRM* pErrParam;
    U32 i;
    SIM_NFC_RED* pRedData;
    U32 CmdCode;
    FLASH_PHY flash_phy;
    U8 phyPageIndex = 0;
    U32* p_addr_red = NULL;
    U32* p_buf;

    if(g_InjErrHead == g_InjErrTail)
    {
        return;
    }

    pRedData = (SIM_NFC_RED*)p_flash_param->p_local_red; 
    if(CMD_CODE_READ == p_flash_param->cur_cmd_code)
    {    
        p_buf = g_pDataBufferOut;
        phyPageIndex = 0;

        NFC_GetFlashAddr(p_flash_param->phy_page_req[0].row_addr,
            p_flash_param->phy_page_req[0].part_in_wl, Pu,&flash_phy);

        p_addr_red = &pRedData->m_Content[g_RED_SZ_DW*(p_flash_param->phy_page_req[phyPageIndex].part_in_wl*PLN_PER_PU+phyPageIndex)];
        Mid_Read(&flash_phy, (char*) p_buf+phyPageIndex*g_PG_SZ, (char*) p_addr_red, g_PG_SZ);       
    }
    else if(CMD_CODE_PROGRAM == p_flash_param->cur_cmd_code)
    {
        Comm_ReadOtfb(p_flash_param->p_red_addr, sizeof(NFC_RED)/sizeof(U32), (U32 *)pRedData);
    }

    if(NULL != pRedData)
    {
        pg_type = (U8)pRedData->m_RedComm.bcPageType;
        if(PG_TYPE_FREE == pg_type)
        {
            return;
        }
    }

    for(i = 0; i < g_InjErrTail; i++)
    {       
        pErrParam = &ErrParamSlot[i];
        if(TRUE == pErrParam->bHit)
        {
            continue;
        }

        CmdCode = p_flash_param->cur_cmd_code;        
        if(CmdCode != pErrParam->CmdCode)  
        {
            continue;
        }
        if(CmdCode == CMD_CODE_READ || CmdCode == CMD_CODE_PROGRAM)
        {
            if(pg_type != pErrParam->PageType)       
            {
                continue;
            }
        }
        
        NFC_GetFlashAddr(p_flash_param->phy_page_req[0].row_addr,
            p_flash_param->phy_page_req[0].part_in_wl,Pu,&flash_phy);

        if(pErrParam->CE != INVALID_2F && pErrParam->CE != flash_phy.nPU)         
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

        if(NFC_SetErrInjEntry((U8)flash_phy.nPU,flash_phy.nBlock,flash_phy.nPage,
            CmdCode,pErrParam->ErrType,0, pErrParam->RetryTime))
        {
            g_bInjectError = TRUE;
            Dump_InjectErrorInfo(&flash_phy,pErrParam->PageType,pErrParam->ErrType,pErrParam->RetryTime);
            pErrParam->bHit = TRUE;
            g_InjErrHead++;
            p_flash_param->module_inj_type = pErrParam->ErrType;
            p_flash_param->module_inj_en = TRUE;

            if(NF_ERR_TYPE_UECC == pErrParam->ErrType)
            {
                p_flash_readrety_param->read_retry_success_time = pErrParam->RetryTime;

            #ifdef DBG_TABLE_REBUILD    
                {
                    U16 VirBlk = INVALID_4F;
                    U16 usPPO = INVALID_4F;
                    SIM_NFC_RED red;
                    Mid_Read_RedData(&flash_phy,(char*)&red);
                    VirBlk = red.m_RedComm.bsVirBlockAddr;
                    if(VirBlk != INVALID_4F && (U8)red.m_RedComm.bcPageType == PAGE_TYPE_DATA)
                    {
                        if(BLOCK_TYPE_SLC == red.m_RedComm.bcBlockType)
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
                        L2_AddErrAddrOfCheckPMT((U8)flash_phy.nPU,VirBlk,usPPO);
                    }
                }
            #endif
            }
            break;
        }           
    }
    return;
}

BOOL parse_injecterror_param(ACTION* pAct,char* param)
{
    char pagetype[256];
    char cmdcode[256];
    char CE[4];
    char Blk[4];
    char Page[4];
    char errtype[64];
    U32 retrytime = 0;

    if(pAct->ActType == ACT_TYPE_INJECT_ERROR)
    {
        pAct->m_ErrInjParam.CE = 0xFF;
        pAct->m_ErrInjParam.PhyBlk = INVALID_4F;
        pAct->m_ErrInjParam.PhyPg = INVALID_4F;   
        pAct->m_ErrInjParam.bHit = FALSE;

        if(sscanf(param,"{%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%d}",CE,Blk,Page,cmdcode,pagetype,errtype,&retrytime))
        {
            if(strncmp(CE,"?",1))
            {
                pAct->m_ErrInjParam.CE = atoi(CE);
            }
            if(strncmp(Blk,"?",1))
            {
                pAct->m_ErrInjParam.PhyBlk = atoi(Blk);
            }

            if(!strncmp(Page,"?",1))
            {
                ;
            }
            else if(!strncmp(Page,"R?",2))
            {
                pAct->m_ErrInjParam.PhyPg = rand()%(PG_PER_BLK-1);
            }
            else
            {
                pAct->m_ErrInjParam.PhyPg = atoi(Page);
            }
            if(!strncmp(cmdcode,"Read",4))
            {
                pAct->m_ErrInjParam.CmdCode = CMD_CODE_READ;
            }
            else if(!strncmp(cmdcode,"Write",5))
            {
                pAct->m_ErrInjParam.CmdCode = CMD_CODE_PROGRAM;
            }
            else if(!strncmp(cmdcode,"Erase",5))
            {
                pAct->m_ErrInjParam.CmdCode = CMD_CODE_ERASE;
            }

            if(FALSE == get_pagetype(pagetype,&pAct->m_ErrInjParam.PageType))
            {
                printf("Not find pagetype %s\n",pagetype);
                DBG_Break();
            }
            if(FALSE == get_errtype(errtype,&pAct->m_ErrInjParam.ErrType))
            {
                printf("Not find pagetype %s\n",errtype);
                DBG_Break();
            }

            pAct->m_ErrInjParam.RetryTime  = retrytime;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
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
