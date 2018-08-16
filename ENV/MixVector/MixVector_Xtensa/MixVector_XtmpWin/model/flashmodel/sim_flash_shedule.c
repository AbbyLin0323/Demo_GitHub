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


/************************************************
GLOBAL value define
*************************************************/
//flash module log
char FM_LogString[256]={0};
int FM_LogHasWrite;
HANDLE hFM_LogFile;
CRITICAL_SECTION FMLog_critical_section;

ST_CE_ERROR_ENTRY pu_err[CE_MAX] = {0};
ST_FLASH_ERR_INJ_ENTRY  g_stFlashErrInjEntry[ERR_INJ_TABLE_MAX];
ST_PU_SHEDULE_PARAM pu_param[NFC_CH_TOTAL][NFC_PU_PER_CH];
ST_FLASH_CMD_PARAM flash_cmd_param[NFC_CH_TOTAL][NFC_PU_PER_CH];
ST_FLASH_READ_RETRY_PARAM flash_readretry_param[NFC_CH_TOTAL][NFC_PU_PER_CH];

CRITICAL_SECTION g_CHCriticalSection[NFC_CH_TOTAL];
CRITICAL_SECTION g_PuAccCriticalSection;

U32 *g_pDataBufferIn;
U32 *g_pDataBufferOut;
NFC_RED g_pRedBuffer[CE_MAX];
U8   bus_status[NFC_CH_TOTAL];

U32 g_secAddGroup;
U32 g_splitedSecInGroup;
U32 g_totalRemainSec;
U32 g_finishSecInDmaEntry;
U32 g_remainSecToXfer;
U32 g_bmBitPos;

U32 g_RetryTimes[SUBSYSTEM_PU_MAX*2] = {0}; // updated by jsaonguo 20140815
U8 L_PRCQ_CurrentCnt = 0;

/************************************************
Function define
*************************************************/
//tobey add start
extern U8 NFC_InterfaceGetErrHold(U8 pu);
extern NFCQ_ENTRY* Comm_GetCQEntry(U8 pu, U8 rp);
extern U8 NFC_InterfaceGetCmdType(U8 pu, U8 level);
extern NFC_PRCQ_ENTRY * Comm_GetPRCQEntry(U8 Pu, U8 Rp);
#ifdef VT3514_C0
extern FIRST_BYTE_ENTRY * Comm_GetFirstByteEntry(U8 ucCE, U8 ucRp);
#endif
extern void NFC_InterfaceUpdateRedData(ST_FLASH_CMD_PARAM *p_flash_cmd_para);
extern void RecInjErrFile(HANDLE hFile,char* fmt,...);
extern HANDLE hFileInjErr;
extern void IsHitErrInjParam(U8 Pu,ST_FLASH_CMD_PARAM* p_flash_param,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param);
extern void set_fcond(FLASH_PHY* pflash_phy1,U32* pRedData,U32 CmdCode);
extern NF_PPUST_REG *p_flash_ppust_reg;

#ifndef IGNORE_PERFORMANCE
extern U8 l_ucPcieBusDevToHostSts;
extern CRITICAL_SECTION g_PcieDevToHostCriticalSection;

LOCAL U8 l_aOtfbBuffSts[NFC_CH_TOTAL][NFC_PU_PER_CH];
LOCAL U8 l_aSgeOtfbSts[NFC_CH_TOTAL][NFC_PU_PER_CH];
LOCAL U8 ucPreOtfbSecCnt[NFC_CH_TOTAL][NFC_PU_PER_CH];
#endif


/*
The callback function for NFC cmd trigger, updated by jasonGuo.
*/
void NFC_SendCMD(U8 pu)
{
    U8 wp = NFC_InterfaceCQWP(pu);

    EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
    NFC_InterfaceJumpCQWP(pu);//jump write pointer
    LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);

    NFC_InterfaceUpdateHwPuSts();
    NFC_UpdateLogicPuSts(pu);
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

    startInPage = curStart&SEC_PER_PG_MSK;
    planeIndex = curStart>>SEC_PER_PG_BITS;
    if((startInPage+curLength)>(1<<SEC_PER_PG_BITS))
    {
        lengthInPage = (1<<SEC_PER_PG_BITS)-startInPage;
        g_splitedSecInGroup  += lengthInPage;
        bFinishSplitOnePhyPage = TRUE;
    }
    else
    {
        lengthInPage = curLength;
        g_secAddGroup++;
        g_splitedSecInGroup = 0;

        if(planeIndex !=((p_nf_cq_entry->aSecAddr[g_secAddGroup].bsSecStart)>>SEC_PER_PG_BITS) ||
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

BOOL getInfoInRawCmd(NFC_PRCQ_ENTRY *p_nf_prcq_entry,
                U8 *pRowAddrSel,
                U8 *pPlaneSel)
{
    NFC_PRCQ_ELEM * pPRCQ_Entry;
    //U8 temp;

    if (L_PRCQ_CurrentCnt >= PRCQ_DEPTH)
    {
        DBG_Printf("getInfoInRawCmd: L_PRCQ_CurrentCnt = %d, greater than or equal to PRCQ_DEPTH = %d\n", L_PRCQ_CurrentCnt, PRCQ_DEPTH);
        DBG_Break();
    }

    pPRCQ_Entry = (NFC_PRCQ_ELEM *)p_nf_prcq_entry+L_PRCQ_CurrentCnt;
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
            if((U32)pPRCQ_Entry == (U32)p_nf_prcq_entry)
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
    if((NF_CMD_RD_1PLN == ucCmdCode) || (NF_CMD_RD == ucCmdCode)
        || (NF_CMD_CHANGECOLUM_1PLN == ucCmdCode) 
        || (NF_CMD_CHANGECOLUM_2PLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_READ;
    }
    else if((NF_CMD_WT_1PLN == ucCmdCode) || (NF_CMD_WT == ucCmdCode))
    {
        ucCmdType = CMD_CODE_PROGRAM;
    }
    else if((NF_CMD_ERS == ucCmdCode) || (NF_CMD_ERS_1PLN == ucCmdCode))
    {
        ucCmdType = CMD_CODE_ERASE;
    }
    else if(NF_CMD_RST == ucCmdCode)
    {
        ucCmdType = CMD_CODE_RST;
    }
#ifdef FLASH_TSB
    else if(NF_CMD_READRETRY_EN == ucCmdCode)
    {
        ucCmdType = CMD_CODE_READRETRY_EN;
    }
    else if((ucCmdCode >= NF_CMD_READRETRY_ADJ_START) && (ucCmdCode <= NF_CMD_READRETRY_ADJ_END))
    {
        ucCmdType = CMD_CODE_READRETRY_ADJ;
    }
#endif
    else if(NF_CMD_SETFEATURE == ucCmdCode)
    {
        ucCmdType = CMD_CODE_SET_FEATURE;
    }
    else if(ucCmdCode > NF_CMD_PIO_SETFEATURE_EX)
    {
       ucCmdType = CMD_CODE_UNKNOWN;
    }
    else
    {
        ucCmdType = CMD_CODE_OTHER;
    }
    return ucCmdType;
}

void NFC_CheckTransferLen(U8 ucCmdCode, NFCQ_ENTRY * pNfcqEntry)
{
    U32 ulTransferLen = pNfcqEntry->bsDmaTotalLength << SEC_SIZE_BITS;
    if ((NF_CMD_WT_1PLN == ucCmdCode) && (ulTransferLen != g_PG_SZ))
    {
        DBG_Getch();
    }

    if ((NF_CMD_WT == ucCmdCode) && (ulTransferLen != (g_PG_SZ * PLN_PER_PU)))
    {
        DBG_Getch();
    }
    
    if ((NF_CMD_RD_1PLN == ucCmdCode) && (ulTransferLen > g_PG_SZ))
    {
        DBG_Getch();
    }
    
    if ((NF_CMD_RD_1PLN == ucCmdCode) && (ulTransferLen > (g_PG_SZ * PLN_PER_PU)))
    {
        DBG_Getch();
    }

    return;
}

/************************************************
Function Name: NFC_GetCmdParam
Input:     pointer to HW CQ entry and raw cmd
Output:     param handled by model
Description:
convert HW CQ entry & raw cmd to param handled by model
*************************************************/
void NFC_GetCmdParam(U8 pu,
                ST_FLASH_CMD_PARAM *p_flash_param,
                NFCQ_ENTRY *p_nf_cq_entry,
                NFC_PRCQ_ENTRY *p_nf_prcq_entry,
                U8 cmd_type)
{
    U8 bSecHighDW,bitPosInDW;
    BOOL bFinishSplitOnePhyPage, bSuccess;
    U8 startInPage,lengthInPage,planeIndex;
    U8 rowAddrSel;
    U8 planeSel = PLN_PER_PU-1;

    initSplitSectorAddr(p_nf_cq_entry);
    memset(p_flash_param, 0, sizeof(ST_FLASH_CMD_PARAM));
    
    p_flash_param->p_nf_cq_entry = p_nf_cq_entry;
    p_flash_param->p_prcq_entry = p_nf_prcq_entry;
    p_flash_param->p_red_addr = p_nf_cq_entry->bsRedAddr<<4;
    p_flash_param->ontf_en=p_nf_cq_entry->bsOntfEn;
    p_flash_param->red_en = p_nf_cq_entry->bsRedEn;
    p_flash_param->red_only = p_nf_cq_entry->bsRedOnly;
    p_flash_param->p_local_red = &g_pRedBuffer[pu];
    p_flash_param->int_en = p_nf_cq_entry->bsIntEn;
    p_flash_param->ssu0_en = p_nf_cq_entry->bsSsu0En;
    p_flash_param->ssu1_en = p_nf_cq_entry->bsSsu1En;
    p_flash_param->ncq_1st_en = p_nf_cq_entry->bsFstDataRdy;//hv
    p_flash_param->ncq_mode = p_nf_cq_entry->bsNcqMode;
    p_flash_param->ncq_num = p_nf_cq_entry->bsNcqNum;
    p_flash_param->inj_en = p_nf_cq_entry->bsInjEn;
    p_flash_param->err_type = p_nf_cq_entry->bsErrTypeS;
    p_flash_param->bsTrigOmEn = p_nf_cq_entry->bsTrigOmEn;
    p_flash_param->bsPuEccMsk = p_nf_cq_entry->bsPuEccMsk;
#ifdef VT3514_C0
    p_flash_param->bsFirstByteCheckEn = p_nf_cq_entry->bsFstByteCheck;
#endif

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
    else
    {
        p_flash_param->xfer_sec_cnt = 0;
    }

    //special handling for command without data transfer
    if( cmd_type != CMD_CODE_READ && cmd_type != CMD_CODE_PROGRAM )
    {
        if( cmd_type == CMD_CODE_ERASE )
        {
            U8 pln;
            for(pln=0; pln<PLN_PER_PU; pln++)
            {
                p_flash_param->phy_page_req[pln].row_addr = p_nf_cq_entry->aRowAddr[pln];
            }
        }
        return;
    }

    //clear for getInfoInRawCmd each time
    L_PRCQ_CurrentCnt = 0;

    //split data transfer info for read/write command
    do{
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
            do{
                bSuccess = getInfoInRawCmd(p_flash_param->p_prcq_entry, &rowAddrSel, &planeSel);

                if(FALSE == bSuccess &&(cmd_type == CMD_CODE_PROGRAM || cmd_type == CMD_CODE_READ))
                {  
                    DBG_Getch();//err in raw command, check RAW_DATA_IO_CMD
                }
            }while((planeSel%PLN_PER_PU) !=  planeIndex); // while(planeSel != planeIndex);
            
            p_flash_param->phy_page_req[planeIndex].row_addr = (U32)p_nf_cq_entry->aRowAddr[rowAddrSel];
            p_flash_param->phy_page_req[planeIndex].part_in_wl = planeSel/PLN_PER_PU;
            p_flash_param->phy_page_req[planeIndex].red_offset_id = planeSel;
        }
    }while(0 != g_totalRemainSec);

    return;
}

/************************************************
Function Name: NFC_HandleCMD
Input:     physical pu number
Output:     None 
Description:
flash handling, call flash file interfaces
*************************************************/
BOOL NFC_HandleCMD(U8 pu, ST_FLASH_CMD_PARAM* p_flash_param, ST_FLASH_READ_RETRY_PARAM * p_flash_readretry_param)
{
    U8 rp;
    U8 i;
    U8 cmd_code;
    U8 ucNFCmdType;
    NFCQ_ENTRY * p_cq_entry;
    NFC_PRCQ_ENTRY * p_prcq_entry;
    U32 base_addr = 0;
    BOOL retStatus = FALSE;
    BOOL bHandelCmd = FALSE;

    if(NULL == p_flash_param)
    {
        DBG_Getch();
    }

    if(FALSE == NFC_InterfaceIsCQEmpty(pu))
    {
        //initial flash cmd parameter
        pu_err[pu].err_flag = FALSE;
        pu_err[pu].err_type = 0;

        //initialise p_flash_param
        for(i=0; i<sizeof(ST_FLASH_CMD_PARAM)/sizeof(U32); i++)
        {
            *((U32*)p_flash_param + i) = 0;
        }

        rp = NFC_InterfaceCQRP(pu);

        p_cq_entry = Comm_GetCQEntry(pu, rp);

        p_prcq_entry = Comm_GetPRCQEntry(pu, rp);

        cmd_code = NFC_InterfaceGetCmdType(pu, rp);

        NFC_CheckTransferLen(cmd_code, p_cq_entry);

        ucNFCmdType = NF_GetCmdType(cmd_code);

        //get NFC command, and saved as local parameter
        NFC_GetCmdParam(pu, p_flash_param, p_cq_entry, p_prcq_entry, ucNFCmdType);
#ifdef VT3514_C0
        p_flash_param->pFirstByteEntry = Comm_GetFirstByteEntry(pu, rp);
#endif
        //check for read retry sequence
        if (TRUE == p_flash_readretry_param->read_retry_en)
        {
            if((ucNFCmdType != CMD_CODE_RST) && (ucNFCmdType != CMD_CODE_READ) && (ucNFCmdType != CMD_CODE_READRETRY_ADJ) 
                && (ucNFCmdType != CMD_CODE_READRETRY_EN) && (ucNFCmdType != CMD_CODE_SET_FEATURE))
            {
                DBG_Printf("NFC_HandleCMD  cmd_code is %d\n",ucNFCmdType);
                DBG_Getch();
            }
        }

        p_flash_param->cur_cmd_code = ucNFCmdType;        

        NFC_ErrorInjCmd(pu,p_flash_param, p_flash_readretry_param); // added by jasonguo 20140815
        
        //handle flash command
        switch(ucNFCmdType)
        {
        case CMD_CODE_READ:
            {
                if(TRUE == p_cq_entry->bsTrigOmEn)
                {
                    bHandelCmd = NFC_OtfbPGReadCMD(pu, p_flash_param, p_flash_readretry_param);
                }
                else
                {
                    bHandelCmd = NFC_PGReadCMD(pu, p_flash_param,p_flash_readretry_param);
                }
                if( FALSE == bHandelCmd){
                    retStatus = FALSE;
                }
                else{
                    p_flash_param->cur_cmd_code = CMD_CODE_READ;
                    retStatus = TRUE;
                }
            }
            break;
        case CMD_CODE_PROGRAM:
            {
                if(TRUE == p_cq_entry->bsTrigOmEn)
                {
                    bHandelCmd = NFC_OtfbPGWriteCMD(pu, p_flash_param);
                }
                else
                {    
                    bHandelCmd = NFC_PGWriteCMD(pu, p_flash_param);
                }

                if(FALSE == bHandelCmd )
                {
                    retStatus = FALSE;
                }
                else
                {
                    FLASH_PHY flash_phy;
                    NFC_GetFlashAddr(p_flash_param->phy_page_req[0].row_addr,
                        p_flash_param->phy_page_req[0].part_in_wl,pu,&flash_phy);
                    set_fcond(&flash_phy,(U32*)p_flash_param->p_local_red,p_flash_param->cur_cmd_code);

                    p_flash_param->cur_cmd_code = CMD_CODE_PROGRAM;
                    retStatus = TRUE;
                }
            }
            break;

        case CMD_CODE_ERASE:
            {
                bHandelCmd = NFC_BlkEreaseCMD(pu, p_flash_param);

                if(FALSE == bHandelCmd){
                    retStatus = FALSE;
                }
                else{
                    p_flash_param->cur_cmd_code = CMD_CODE_ERASE;
                    retStatus = TRUE;
                }

            }
            break;

        case CMD_CODE_RST:
            {
                p_flash_param->cur_cmd_code = CMD_CODE_RST;
                retStatus = TRUE;
            }
            break;

        case  CMD_CODE_READRETRY_EN:
        {
            p_flash_readretry_param->read_retry_en = TRUE;

            p_flash_param->cur_cmd_code = CMD_CODE_READRETRY_EN;
            retStatus = TRUE;
        }
        break;

        case CMD_CODE_READRETRY_ADJ:
            { 
                p_flash_readretry_param->read_retry_current_time++;

                p_flash_param->cur_cmd_code = CMD_CODE_READRETRY_ADJ;
                retStatus = TRUE;
            }
            break;
        case CMD_CODE_SET_FEATURE://For L85 flashchip                              
            p_flash_readretry_param->read_retry_current_time = g_RetryTimes[pu];
            if(g_RetryTimes[pu] > 0)
            {
                p_flash_readretry_param->read_retry_en = TRUE;
            }
            else
            {
                p_flash_readretry_param->read_retry_en = FALSE;//Retry terminate
            }
            retStatus = TRUE;
            break;
        case CMD_CODE_OTHER:
            {
                p_flash_param->cur_cmd_code = CMD_CODE_OTHER;
                retStatus = TRUE;
            }
            break;

        default:
            p_flash_param->cur_cmd_code = CMD_CODE_UNKNOWN;
            break;
        }
    }
    else
    {
        p_flash_param->cur_cmd_code = CMD_CODE_NONE;
        retStatus = TRUE;
    }

    //NFC_ErrorInjCmd(pu,p_flash_param, p_flash_readretry_param); // deleated by jasonguo 20140815

    return retStatus;
}


/************************************************
Function Name: NFC_FinishCMD
Input:     physical pu number, flash_param
Output:     None 
Description:
flash command finish, do operations according to flash mod config
*************************************************/
void NFC_FinishCMD(U8 pu, ST_FLASH_CMD_PARAM* p_flash_param, ST_FLASH_READ_RETRY_PARAM *p_flash_readretry_param )
{
    U8 i = 0;   

    if(CMD_CODE_RST == p_flash_param->cur_cmd_code)
    {
        p_flash_readretry_param->read_retry_en = 0;
        p_flash_readretry_param->read_retry_current_time = 0;
    }

    //Copy redundant data no matter error occur.
    if(CMD_CODE_READ == p_flash_param->cur_cmd_code &&
        TRUE == p_flash_param->red_en)
    {
        NFC_InterfaceUpdateRedData(p_flash_param);
    }

    //Read operation error report
    if(CMD_CODE_READ == p_flash_param->cur_cmd_code)  
    {
        if (TRUE == NFC_GetErrFlag(pu))
        {
            NFC_InterfaceSetErrParameter(pu, pu_err[pu].err_type);
            return;
        }
    }
    else if (TRUE == p_flash_param->module_inj_en)
    {      
        NFC_InterfaceSetErrParameter(pu, ((U8)p_flash_param->module_inj_type));
        return;
    }

    //Special Status Update: NOTE: FW check SSU to judge command finish or not, so SSU must updated after others
    if(TRUE == p_flash_param->ssu1_en)
    {
        NFC_InterfaceUpdateSsu((U32)p_flash_param->p_nf_cq_entry->bsSsuAddr1,
            (U32)p_flash_param->p_nf_cq_entry->bsSsuData1,1);
    }
    if(TRUE == p_flash_param->ssu0_en)
    {
        NFC_InterfaceUpdateSsu((U32)p_flash_param->p_nf_cq_entry->bsSsuAddr0,
            (U32)p_flash_param->p_nf_cq_entry->bsSsuData0,0);
    }

    //r/w pointer  /Avoid coflict in inner channel
    EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
    NFC_InterfaceJumpCQRP(pu);
    LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);

    //interrupt
    if(p_flash_param->int_en)
    {
        EnterCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);
        p_flash_cq_dptr[pu].bsIntSts = 1;
        LeaveCriticalSection(&g_CHCriticalSection[pu&NFC_CH_MSK]);

        NFC_InterfaceSetInt(pu);//need adjust to 3 core mode
    }
}

/************************************************
Function Name: NFC_ChannelSchedule
Input:     channel number 
Output:     None 
Description:
shedule function for one channel
*************************************************/
void NFC_ChannelSchedule(U32 ch)
{

    U8 i;
    U8 pu = 0;
#ifndef IGNORE_PERFORMANCE
    U8 sec_count = 0;
    U32 cur_time=0;
#endif
    //query the status of each pu
    for(i=0; i<NFC_PU_PER_CH; i++)
    {
        //pu: 0, 8, 16, 24...... for channel 0
        pu = i*NFC_CH_TOTAL + ch;

        if (0 == NFC_InterfaceGetErrHold(pu))
        {

#ifdef IGNORE_PERFORMANCE
            if(FALSE == NFC_HandleCMD(pu,
                &flash_cmd_param[ch][i],&flash_readretry_param[ch][i]))
            {
                //NFC_LogErr(LOG_FILE, 0, "pu = %d, handle cmd ERR\n", pu);
                DBG_Getch();
            }

            if(CMD_CODE_NONE != flash_cmd_param[ch][i].cur_cmd_code)
            {
                NFC_FinishCMD(pu, &flash_cmd_param[ch][i],&flash_readretry_param[ch][i]);
            }

            flash_cmd_param[ch][i].cur_cmd_code = CMD_CODE_NONE;
#else
            switch (pu_param[ch][i].pu_status)
            {
            case PU_STATUS_WAIT_CMD:
                {
                    //handle the command
                    if(FALSE == NFC_HandleCMD(pu, &flash_cmd_param[ch][i],&flash_readretry_param[ch][i])){
                        NFC_LogErr(LOG_FILE, 0, "pu = %d, handle cmd ERR\n", pu);
                        DBG_Getch();
                    }
                    //set pu status
                    if(CMD_CODE_READ == flash_cmd_param[ch][i].cur_cmd_code)
                    {
                        U8 page_type;
                        //set pu status
                        pu_param[ch][i].pu_status = PU_STATUS_CMD_BUSY;
                        pu_param[ch][i].timer_start = (U32)GET_TIME();
                        pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                        
                        if(1 == (flash_cmd_param[ch][i].phy_page_req[0].row_addr&0x1))
                            page_type = NFC_CONFIG_FAST_PAGE;
                        else
                            page_type = NFC_CONFIG_SLOW_PAGE;
                        
                        switch(page_type)
                        {
                        case NFC_CONFIG_SLOW_PAGE:
                            pu_param[ch][i].timer_busy = BUSY_TIME_SLOW_PAGE_READ;
                            break;
                        case NFC_CONFIG_FAST_PAGE:
                            pu_param[ch][i].timer_busy = BUSY_TIME_FAST_PAGE_READ;
                            break;
                        case NFC_CONFIG_NORMAL_PAGE:
                            pu_param[ch][i].timer_busy = BUSY_TIME_FLASH_READ;
                            break;
                        }
                    }
                    else if(CMD_CODE_PROGRAM == flash_cmd_param[ch][i].cur_cmd_code)
                    {
                        //check bus status
                        if(BUS_STATUS_FREE == bus_status[ch])
                        {
                            //data transfer
                            pu_param[ch][i].pu_status = PU_STATUS_DATA_TRANSFER;

                            pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                            pu_param[ch][i].timer_start = (U32)GET_TIME();
                            pu_param[ch][i].timer_busy = (U32)BUSY_TIME_BUS_TRANSFER;

                            bus_status[ch] = BUS_STATUS_BUSY;
                        }
                        else
                        {
                            pu_param[ch][i].pu_status = PU_STATUS_WAIT_BUS;
                        }
                    }
                    else if(CMD_CODE_ERASE == flash_cmd_param[ch][i].cur_cmd_code)
                    {
                        //set pu busy
                        pu_param[ch][i].pu_status = PU_STATUS_CMD_BUSY;

                        //set timer
                        pu_param[ch][i].timer_start = (U32)GET_TIME();
                        pu_param[ch][i].timer_busy = BUSY_TIME_FLASH_ERASE;
                        pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                    }
                    else if(CMD_CODE_NONE == flash_cmd_param[ch][i].cur_cmd_code)
                    {
                        //if no command, go to next pu
                        continue;
                    }
                    else if(CMD_CODE_RST == flash_cmd_param[ch][i].cur_cmd_code)
                    {
                        NFC_FinishCMD(pu, &flash_cmd_param[ch][i],&flash_readretry_param[ch][i]);
                    }
                    else if(CMD_CODE_OTHER == flash_cmd_param[ch][i].cur_cmd_code)//other command, just finish
                    {
                        NFC_FinishCMD(pu, &flash_cmd_param[ch][i],&flash_readretry_param[ch][i]);
                    }
                }
                break;//end case PU_STATUS_WAIT_CMD
            case PU_STATUS_CMD_BUSY:
                {
                    cur_time = (U32)GET_TIME();
                    //busy timeout
                    if((cur_time - pu_param[ch][i].timer_start) >= pu_param[ch][i].timer_busy)
                    {
                        pu_param[ch][i].timer_status = TIMER_STATUS_END;
                        if(CMD_CODE_READ == flash_cmd_param[ch][i].cur_cmd_code)
                        {
                            //if OTFBA read change to wait OTFBA buffer AND BUS
                            if(TRUE == flash_cmd_param[ch][i].bsTrigOmEn)
                            {
                                pu_param[ch][i].pu_status= PU_STATUS_WAIT_OTFB_BUFF;
                            }

                            //if dram read
                            if(BUS_STATUS_FREE == bus_status[ch])
                            {
                                sec_count = flash_cmd_param[ch][i].xfer_sec_cnt;                      
                                pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                                pu_param[ch][i].timer_start = (U32)GET_TIME();
                                pu_param[ch][i].timer_busy = (U32)((sec_count * BUSY_TIME_BUS_TRANSFER) / SEC_PER_BUF);            

                                pu_param[ch][i].pu_status= PU_STATUS_DATA_TRANSFER;
                                bus_status[ch] = BUS_STATUS_BUSY;
                            }
                            else
                            {
                                pu_param[ch][i].pu_status = PU_STATUS_WAIT_BUS;
                            }
                        }
                        else//program busy timeout
                        {
                            pu_param[ch][i].pu_status = PU_STATUS_WAIT_CMD;

                            //flash command finish for erase and write
                            NFC_FinishCMD(pu, &flash_cmd_param[ch][i],&flash_readretry_param[ch][i]);
                            flash_cmd_param[ch][i].cur_cmd_code = CMD_CODE_NONE;
                        }
                    }
                }
                break;//end case PU_STATUS_CMD_BUSY
            case PU_STATUS_WAIT_BUS:
                {
                    //check command code, not be erase
                    //check bus status
                    if(BUS_STATUS_FREE == bus_status[ch])
                    {                 
                        sec_count = flash_cmd_param[ch][i].xfer_sec_cnt;                      
                        pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                        pu_param[ch][i].timer_start = (U32)GET_TIME();
                        pu_param[ch][i].timer_busy = (U32)((sec_count * BUSY_TIME_BUS_TRANSFER) / SEC_PER_BUF);            

                        pu_param[ch][i].pu_status = PU_STATUS_DATA_TRANSFER;
                        bus_status[ch] = BUS_STATUS_BUSY;
                    }
                }
                break;//end case PU_STATUS_WAIT_BUS
            case PU_STATUS_DATA_TRANSFER:
                {
                    cur_time = (U32)GET_TIME();
                    //check timer
                    if((cur_time - pu_param[ch][i].timer_start) >= pu_param[ch][i].timer_busy)
                    {
                        pu_param[ch][i].timer_status = TIMER_STATUS_END;

                        bus_status[ch] = BUS_STATUS_FREE;

                        if(CMD_CODE_READ == flash_cmd_param[ch][i].cur_cmd_code)
                        {
                            if(TRUE == flash_cmd_param[ch][i].bsTrigOmEn)
                            {
                                l_aOtfbBuffSts[ch][i] = OTFB_BUFF_BUSY;
                                ucPreOtfbSecCnt[ch][i] = flash_cmd_param[ch][i].xfer_sec_cnt;
                            }

                            pu_param[ch][i].pu_status = PU_STATUS_WAIT_CMD;                                 
                            //flash command finish for read
                            NFC_FinishCMD(pu, &flash_cmd_param[ch][i],&flash_readretry_param[ch][i]);
                            flash_cmd_param[ch][i].cur_cmd_code = CMD_CODE_NONE;   
                        }
                        else if(CMD_CODE_PROGRAM == flash_cmd_param[ch][i].cur_cmd_code)
                        {
                            U8 page_type;
                            pu_param[ch][i].pu_status = PU_STATUS_CMD_BUSY;

                            pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                            pu_param[ch][i].timer_start = (U32)GET_TIME();

                            if(1 == (flash_cmd_param[ch][i].phy_page_req[0].row_addr&0x1))
                                page_type = NFC_CONFIG_FAST_PAGE;
                            else
                                page_type = NFC_CONFIG_SLOW_PAGE;
                            
                            switch(page_type)
                            {
                            case NFC_CONFIG_SLOW_PAGE:
                                pu_param[ch][i].timer_busy = BUSY_TIME_SLOW_PAGE_WRITE;
                                break;
                            case NFC_CONFIG_FAST_PAGE:
                                pu_param[ch][i].timer_busy = BUSY_TIME_FAST_PAGE_WRITE;
                                break;
                            case NFC_CONFIG_NORMAL_PAGE:
                                pu_param[ch][i].timer_busy = BUSY_TIME_FLASH_WRITE;
                                break;
                            }
                        }
                        else
                        {
                            //error handle
                        }
                    }//next loop, time will ++
                }
                break;

            case PU_STATUS_WAIT_OTFB_BUFF:
                {
                    if((OTFB_BUFF_FREE == l_aOtfbBuffSts[ch][i]) && (BUS_STATUS_FREE == bus_status[ch]))
                    {
                        sec_count = flash_cmd_param[ch][i].xfer_sec_cnt;
                        pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                        pu_param[ch][i].timer_start = (U32)GET_TIME();
                        pu_param[ch][i].timer_busy = (U32)((sec_count * BUSY_TIME_BUS_TRANSFER) / SEC_PER_BUF);

                        pu_param[ch][i].pu_status = PU_STATUS_DATA_TRANSFER;
                        bus_status[ch] = BUS_STATUS_BUSY;
                    }
                }
                break;

            default:
                break;

            }//end switch

            if(OTFB_BUFF_BUSY == l_aOtfbBuffSts[ch][i])
            {
                switch(l_aSgeOtfbSts[ch][i])
                {
                case SGE_OTFB_WAIT_PCIE_BUS:
                    {
                        EnterCriticalSection(&g_PcieDevToHostCriticalSection);
                        if(PCIE_STS_FREE == l_ucPcieBusDevToHostSts)
                        {
                            l_ucPcieBusDevToHostSts = PCIE_STS_BUSY;
                            pu_param[ch][i].timer_status = TIMER_STATUS_SET_START;
                            pu_param[ch][i].timer_start = (U32)GET_TIME();
                            pu_param[ch][i].timer_busy = ((ucPreOtfbSecCnt[ch][i] * BUSY_TIME_OTFB_TO_HOST) / SEC_PER_BUF);

                            l_aSgeOtfbSts[ch][i] = SGE_OTFB_DATA_OTFB_TO_HOST;
                        }
                        LeaveCriticalSection(&g_PcieDevToHostCriticalSection);
                    }
                    break;

                case SGE_OTFB_DATA_OTFB_TO_HOST:
                    {
                        cur_time = (U32)GET_TIME();
                        //check timer
                        if((cur_time - pu_param[ch][i].timer_start) >= pu_param[ch][i].timer_busy)
                        {
                            pu_param[ch][i].timer_status = TIMER_STATUS_END;
                            EnterCriticalSection(&g_PcieDevToHostCriticalSection);
                            l_ucPcieBusDevToHostSts = PCIE_STS_FREE;
                            LeaveCriticalSection(&g_PcieDevToHostCriticalSection);

                            l_aSgeOtfbSts[ch][i] = SGE_OTFB_WAIT_PCIE_BUS;
                            l_aOtfbBuffSts[ch][i] = OTFB_BUFF_FREE;
                        }
                    }
                    break;

                default:
                    break;
                }
            }
#endif//IGNORE_PERFORMANCE
        }
    }
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
    U8 ucppu;
    //U32 config_dw;
    //U8 dma_split_size;
    U8 ch;
    U16 i;
    //U32 config_tmp;
    //int j;

    //init nfc dptr

    p_flash_ppust_reg->ulIdleBitMap = 0xFFFFFFFF;
    p_flash_ppust_reg->ulErrorBitMap = 0;
    p_flash_ppust_reg->ulFullBitMap = 0xFFFFFFFF;
    p_flash_ppust_reg->ulEmptyBitMap = 0;

    //default value of logic pu status
    p_flash_lpust_reg->ulIdleBitMap = 0xFFFFFFFF;
    p_flash_lpust_reg->ulErrorBitMap = 0;
    p_flash_lpust_reg->ulFullBitMap = 0xFFFFFFFF;
    p_flash_lpust_reg->ulEmptyBitMap = 0;
    //default value of pu finish bitmap
    p_flash_pufsb_reg->ulLogPufsbReg= 0xffffffff;

    for(ucppu = 0; ucppu < CE_MAX; ucppu ++)
    {
        *(U32*)(&p_flash_cq_dptr[ucppu]) = 0;
        *((U32*)&p_flash_cq_dptr[ucppu] + 1) = 0;
        p_flash_cq_dptr[ucppu].bsEmpty  = 1;//set 'empty' filed
        p_flash_cq_dptr[ucppu].bsIdle  = 1;//set 'idle' filed
        //clear pu finish status to 0
        p_flash_cq_dptr[ucppu].bsFsLv0 = 1;
        p_flash_cq_dptr[ucppu].bsFsLv1 = 1;
        #ifdef VT3514_C0
        p_flash_cq_dptr[ucppu].bsFsLv2 = 1;
        p_flash_cq_dptr[ucppu].bsFsLv3 = 1;
        #endif
        //disable hw logic pu mapping by default
        //p_flash_pucr_reg->aNfcLogicPUReg[ucppu%4][ucppu/4].bsPuEnable = 0;
    }

    for(ch = 0;ch < NFC_CH_TOTAL;ch++)
    {
        for (i=0; i<NFC_PU_PER_CH; i++)
        {
            pu_param[ch][i].pu_status = PU_STATUS_WAIT_CMD;
            pu_param[ch][i].timer_start = 0;
            pu_param[ch][i].timer_busy = 0;
            pu_param[ch][i].timer_status = TIMER_STATUS_END;

            flash_readretry_param[ch][i].read_retry_current_time = 0;
            flash_readretry_param[ch][i].read_retry_en = 0;
            flash_readretry_param[ch][i].read_retry_rsvd = 0;
            flash_readretry_param[ch][i].read_retry_success_time = 0;
        }
#ifndef IGNORE_PERFORMANCE
        bus_status[ch] = BUS_STATUS_FREE;
        memset((void *)l_aSgeOtfbSts, SGE_OTFB_WAIT_PCIE_BUS, NFC_CH_TOTAL * NFC_PU_PER_CH * sizeof(U8));
        memset((void *)l_aOtfbBuffSts, OTFB_BUFF_FREE, NFC_CH_TOTAL * NFC_PU_PER_CH * sizeof(U8));
        memset((void *)ucPreOtfbSecCnt, 0, NFC_CH_TOTAL * NFC_PU_PER_CH * sizeof(U8));
#endif
        //InitializeCriticalSection(&g_CHCriticalSection[ch]);
    }

    return 0;
}

BOOL NFC_SetErrInjEntry(U8 pu,U16 block, U16 page, U8 cmd_code,U8 err_type,U8 index,U8 retry_times)
{

    U8 nLoop;
    U8 bRtn = FALSE;
    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        if (g_stFlashErrInjEntry[nLoop].valid)
            continue;
        else
        {            
            g_stFlashErrInjEntry[nLoop].valid = TRUE;
            g_stFlashErrInjEntry[nLoop].pu = pu;
            g_stFlashErrInjEntry[nLoop].block = block;
            g_stFlashErrInjEntry[nLoop].page = page;
            g_stFlashErrInjEntry[nLoop].cmd_code = cmd_code;
            g_stFlashErrInjEntry[nLoop].err_type = err_type;
            g_stFlashErrInjEntry[nLoop].retry_times = retry_times;
            bRtn = TRUE;
            break;
        }
    }
    return bRtn;

}

void NFC_ErrorInjCmd(U8 pu,ST_FLASH_CMD_PARAM* p_flash_param,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param)
{
    U32 nLoop;
    U8 phyPageIndex;
    FLASH_PHY flash_phy;

    // 1.lookup FlashErrInjEntry
    for (nLoop = 0; nLoop < g_InjErrHead; nLoop++)
    {
        if (!g_stFlashErrInjEntry[nLoop].valid)
        {
            continue;
        }
        else
        {
            for(phyPageIndex = 0; phyPageIndex < PHY_PAGE_PER_LOGIC_PAGE; phyPageIndex++)
            {

                NFC_GetFlashAddr(p_flash_param->phy_page_req[phyPageIndex].row_addr,
                    p_flash_param->phy_page_req[phyPageIndex].part_in_wl,
                    pu,
                    &flash_phy);


                if (   (p_flash_param->cur_cmd_code == g_stFlashErrInjEntry[nLoop].cmd_code)  
                    && (pu == g_stFlashErrInjEntry[nLoop].pu)              
                    && (flash_phy.nBlock== g_stFlashErrInjEntry[nLoop].block)        
                    && (flash_phy.nPage == g_stFlashErrInjEntry[nLoop].page))
                {
                    p_flash_param->module_inj_type = g_stFlashErrInjEntry[nLoop].err_type;
                    p_flash_param->module_inj_en = TRUE;
                    p_flash_readrety_param->read_retry_success_time = g_stFlashErrInjEntry[nLoop].retry_times;
                    break;
                }
            }
        }
    }
    
    // 2.is hit error injection param
    IsHitErrInjParam(pu,p_flash_param,p_flash_readrety_param);    
}


void NFC_ResetErrInjEntry()
{
    U32 nLoop = 0;
    for (nLoop = 0; nLoop < ERR_INJ_TABLE_MAX; nLoop++)
    {
        g_stFlashErrInjEntry[nLoop].valid = FALSE;
        g_stFlashErrInjEntry[nLoop].pu = 0;
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
            if(Pu != g_stFlashErrInjEntry[nLoop].pu)
                continue;
            if(PhyBlk != g_stFlashErrInjEntry[nLoop].block)
                continue;
            if((Page != INVALID_4F) && (Page != g_stFlashErrInjEntry[nLoop].page))
                continue;

            if(CMD_CODE_ERASE == g_stFlashErrInjEntry[nLoop].cmd_code)
                continue;

            Pg = g_stFlashErrInjEntry[nLoop].page;
            g_stFlashErrInjEntry[nLoop].valid = FALSE;
            g_stFlashErrInjEntry[nLoop].pu = 0;
            g_stFlashErrInjEntry[nLoop].block = 0;
            g_stFlashErrInjEntry[nLoop].page = 0;
            g_stFlashErrInjEntry[nLoop].cmd_code = 0;
            g_stFlashErrInjEntry[nLoop].err_type = 0;
            g_stFlashErrInjEntry[nLoop].retry_times = 0;
            DBG_Printf("Pu %d PhyBlk %d Pg %d Inject Error is Cleared!\n",Pu,PhyBlk,Pg); 
            RecInjErrFile(hFileInjErr,"\tPu %2d PhyBlk %4d Pg %2d Inject Error is Cleared!\n",Pu,PhyBlk,Pg);
        }
    }
}