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
  File Name     : checkList_parse.c
  Version       : Initial Draft
  Author        : Nina Yang
  Created       : 2014/4/13
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2014/4/13
    Author      : Nina Yang
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "checklist_parse.h"
#include "iniparser.h"
#include "Proj_Config.h"

#include "HAL_MemoryMap.h"

#include "model_config.h"
#include "model_common.h"
#include "action_define.h"
#include "base_pattern.h"
#include "hscmd_parse.h"
#include "win_bootloader.h"

HANDLE hFileNfcErr;
HANDLE hFileInjErr;

GLOBAL SYSTEMTIME sys_time_start;

extern BOOL run_base_pattern(pBASE_FUNCTION pBaseFunction);
extern BOOL sim_send_trim_cmd(void);
extern BOOL sim_trim_one_range(U32* pStartLba,U32* pLen);
extern PHSCmdCallBack* get_hscmd_callback(char* HSCMD_Desc);

extern BOOL g_bWearLevelingStatistic;

HSCMD_LIST* g_pHScmdList;
CHECK_LIST* g_pCheckList; 

SECTION* g_CurSec; //current running section pointer
COND_LIST* g_CurCond;
ACTION* g_CurAct;
BP_PARAM* g_BPP; //current BasePattern pointer
TRIM_PARAM g_TrimParam;
RUN_SECTION_STATE g_HostPatternStatus = LOAD_SECTION;
ERR_INJ_PARRM ErrParamSlot[ERR_INJ_SLOT_MAX];
U32 g_InjErrHead;
U32 g_InjErrTail;

FCMD_COND* g_pFCOND;

BOOL DoEvtLoadSection();
BOOL DoEvtCheckSection();
BOOL DoEvtInitSection();
BOOL DoEvtCheckCondition();
BOOL DoEvtRunPattern();

FPHostProcess l_HosProcessArray[RUN_SECTION_END + 1] = 
{
    (FPHostProcess)DoEvtLoadSection,//LOAD_SECTION = 0,
    (FPHostProcess)DoEvtInitSection,//INI_SECTION,
    (FPHostProcess)DoEvtRunPattern,//RUN_BASE_PATTERN,
    (FPHostProcess)DoEvtCheckCondition,//CHECK_CONDLIST,
    (FPHostProcess)DoEvtCheckSection,//CHECK_SECTION,  
    (FPHostProcess)0,//RUN_SECTION_END,
};

BOOL isFCond(COND_LIST * pCond)
{
    BOOL bReturn = FALSE; 
    if (NULL != pCond)
    {
        if(!strncmp(pCond->CondName,"FC",2))
        {
            bReturn = TRUE;
        }
    }

    return bReturn;
}

BOOL isHCond(COND_LIST * pCond)
{
    BOOL bReturn = FALSE; 
    if (NULL != pCond)
    {
        if(!strncmp(pCond->CondName,"HC",2))
        {
            bReturn = TRUE;
        }
    }

    return bReturn;
}



void strToUpper(char * pBuffer)
{
    char *pIndex = pBuffer;
    if (NULL == pIndex)
    {
        return;
    }
    while (0 != *pIndex )
    {
        if (*pIndex >= 'a' && *pIndex <= 'z')
        {
            *pIndex -= 32;
        }
        pIndex++;
    }

    return;
}

U32 strToInt(const char *pBuffer)
{
    U8 uBase = 10;
    if ((0 == strncmp(pBuffer, "0x", 2)) || (0 == strncmp(pBuffer, "0X", 2)))
    {
        uBase = 16;
    }

    return strtoul(pBuffer,NULL, uBase); 
}

U32 ParameterCount(char* P_string)
{
    U32 i = 0;
    U32 count = 1;

    while(P_string[i] != '\0')
    {
        if(P_string[i] == ',')
        {
            count++;
        }
        i++;
    }
    return count;
}

U32 GetParameters(char* pSrcStr, const char* pSeparator, char **ppDestStr)
{
    char *pTocken = NULL;
    char *pString = pSrcStr;
    char *pStart = pSrcStr;
    U32 ulCount = 0;

    if (NULL == pSrcStr)
    {
        ulCount = 0;
    }
    else
    {
        pStart = pString;
        while ('\0' != (*pString))
        {
            if (*pString != *pSeparator)
            {
                pString++;
            }
            else
            {
                *pString = 0; 
                strcpy(ppDestStr[ulCount], pStart);
                
                pString++;
                pStart = pString;

                ulCount++;
            }
        }

        //get last valid string
        if ('\0' != *pStart)
        {
            strcpy(ppDestStr[ulCount], pStart);
            ulCount++;
        }
    }
   /* pTocken = strtok(pSrcStr, pSeparator);

    while (pTocken != NULL)
    {
        strcpy(ppDestStr[ulCount], pTocken);
        ulCount++;

        pTocken = strtok(NULL, pSeparator);
    }*/

    return ulCount;
}

BOOL set_bp_param(char* BP_Desc,U16* pBPNum,FUNCTION_PARAMETER* pBpParam)
{
    BP_DESC* pBPDesc;
    BOOL bFind = FALSE;
    char real_bp_str[255];
    char buffer[255] = {0};
    char start_lba[255] = {0};
    char sec_cnt[255] = {0};
    char end_lba[255] = {0};
    char sec_range[255] = {0};
    char *pdest;
    char cTocken = ','; 
    int  index;
    U32 ulParaCnt;
    char *pParamArray[4] = {start_lba, sec_cnt, end_lba, sec_range};
    int ulScanfResult = 0;
    pBPDesc = &BP_LIB[0]; 


    pdest = strchr(BP_Desc,'(');
    if (pdest == NULL ) 
    {
        memcpy(real_bp_str,BP_Desc,strlen(BP_Desc)+1);
        //set to default bp param
        return FALSE;
    }

    index = (int)(pdest - BP_Desc);
    memcpy(real_bp_str,BP_Desc,index);
    real_bp_str[index] = '\0';
    strcpy(real_bp_str,strstrip(real_bp_str));

    // find base index function
    while(strcmp(pBPDesc->BP_Desc,"END"))
    {
        if(!strcmp(pBPDesc->BP_Desc,real_bp_str))
        {
            *pBPNum = (U16)pBPDesc->BPNum;
            bFind = TRUE;
            break;
        }
        pBPDesc++;
    }

    // get parameters
    if (TRUE == bFind)
    {
        memcpy(buffer,BP_Desc + index + 1,strlen(BP_Desc)-index-2);
        ulParaCnt = GetParameters(buffer, &cTocken, pParamArray);

        if(strncmp(start_lba,"?",1))
        {
            pBpParam->ulStartLba = atoi(start_lba);
        }
        else
        {
            pBpParam->ulStartLba = 0;
        }

        if(strncmp(sec_cnt,"?",1))
        {
            pBpParam->ulSecCnt = atoi(sec_cnt);
        }
        else
        {
            pBpParam->ulSecCnt = 8;//default value
        }

        if(strncmp(end_lba,"?",1))
        {
            pBpParam->ulEndLba = atoi(end_lba);
        }
        else
        {
            pBpParam->ulEndLba = 0;//default value
        }

        if(strncmp(sec_range,"?",1))
        {
            pBpParam->ulSecRange = atoi(sec_range);
        }
        else
        {
            pBpParam->ulSecRange = 0;//default value
        }
    }

    if (0 == pBpParam->ulSecCnt)
    {
        DBG_Printf("set_bp_param:BP_Desc = %s\n", BP_Desc);
        DBG_Printf("set_bp_param:temp buffer = %s\n", buffer);
        DBG_Printf("set_bp_param: strSecCnt = %s, ulSecCnt = %d \n", sec_cnt, pBpParam->ulSecCnt);
        DBG_Getch();
    }

    return bFind;
}

void set_trim_param(char* TrimDesc,TRIM_PARAM* pTrimParam)
{
    BP_DESC* pBPDesc;
    BOOL bFind = FALSE;
    char real_bp_str[255];
    char buffer[255];
    char start_lba[255];
    char sec_cnt[255];
    char *pdest;
    int  index;
    U32 ulMaxLba = GetSystemMaxLBA();

    pBPDesc = &BP_LIB[0]; 

    pdest = strchr(TrimDesc,'(');
    if (pdest == NULL ) 
    {
        memcpy(real_bp_str,TrimDesc,strlen(TrimDesc)+1);
        pTrimParam->TrimType = RANDOM_TRIM;
        return;
    }
    index = (int)(pdest - TrimDesc);

    memcpy(buffer,TrimDesc + index + 1,strlen(TrimDesc)-index);

    if(sscanf(buffer,"%[^,],%[^)]",start_lba,sec_cnt))
    {
        if(strncmp(start_lba,"?",1))
        {
            pTrimParam->ulTrimStartLba = strToInt(start_lba);
        }
        else
        {
            pTrimParam->ulTrimStartLba = rand()% ulMaxLba;
        }
        if(strncmp(sec_cnt,"?",1))
        {
            pTrimParam->ulLength = strToInt(sec_cnt);
            if(pTrimParam->ulLength == INVALID_8F)
            {
                pTrimParam->ulLength = ulMaxLba;
            }
        }
        else
        {
            pTrimParam->ulLength = rand()% ulMaxLba;
            if(pTrimParam->ulTrimStartLba + pTrimParam->ulLength >= ulMaxLba)
            {
                pTrimParam->ulTrimStartLba = pTrimParam->ulTrimStartLba % (ulMaxLba - pTrimParam->ulLength);
            }
        }
    }
    
}
char* get_bp_str(U16 BPNum)
{
    BP_DESC* pBPDesc;
    pBPDesc = &BP_LIB[0];
    while(strcmp(pBPDesc->BP_Desc,"END"))
    {
        if(pBPDesc->BPNum == BPNum)
        {
            return pBPDesc->BP_Desc;
            break;
        }
        pBPDesc++;
    }
    return NULL;   
}

BOOL get_actiontype(char* action,U8* ActType)
{
    ACTION_TYPE_DESC* pActTypeDesc;
    BOOL bFind = FALSE;

    pActTypeDesc = &ACTION_TYPE_LIB[0];
    while(strcmp(pActTypeDesc->ActTypeDesc,"END"))
    {
        if(!strncmp(pActTypeDesc->ActTypeDesc,action,strlen(pActTypeDesc->ActTypeDesc)))
        {
            *ActType = pActTypeDesc->ActType;
            bFind = TRUE;
            break;
        }
        pActTypeDesc++;
    }
    return bFind;
}
BOOL get_pagetype(char* pagetype,U8* PgType)
{
    PAGE_TYPE_DESC* pPageTypeDesc;
    BOOL bFind = FALSE;
    pPageTypeDesc = &PAGE_TYPE_LIB[0];
    while(strcmp(pPageTypeDesc->PgTypeDesc,"END"))
    {
        if(!strcmp(pPageTypeDesc->PgTypeDesc,pagetype))
        {
            *PgType = pPageTypeDesc->PgType;
            bFind = TRUE;
            break;
        }
        pPageTypeDesc++;
    }
    return bFind;
}
BOOL get_optype(char* optype,U8* OpType)
{
    OP_TYPE_DESC* pOpTypeDesc;
    BOOL bFind = FALSE;
    pOpTypeDesc = &OP_TYPE_LIB[0];
    while(strcmp(pOpTypeDesc->OPTypeDesc,"END"))
    {
        if(!strcmp(pOpTypeDesc->OPTypeDesc,optype))
        {
            *OpType = pOpTypeDesc->OPType;
            bFind = TRUE;
            break;
        }
        pOpTypeDesc++;
    }
    return bFind;
}

BOOL get_errtype(char* errtype,U8* ErrType)
{
    ERR_TYPE_DESC* pERRTypeDesc;
    BOOL bFind = FALSE;
    pERRTypeDesc = &ERR_TYPE_LIB[0];
    while(strcmp(pERRTypeDesc->ErrTypeDesc,"END"))
    {
        if(!strcmp(pERRTypeDesc->ErrTypeDesc,errtype))
        {
            *ErrType = pERRTypeDesc->ErrType;
            bFind = TRUE;
            break;
        }
        pERRTypeDesc++;
    }
    return bFind;
}

BOOL get_operatertype(char* pOperator, U8* pOperatorType)
{
    BOOL bFind = FALSE;
    OPERATOR_DESC *pOperatorDesc = &OPERATOR_DESC_LIB[0];
    U8 uIndex = 0;

    if (NULL != pOperator)
    {
        for (uIndex = 0; uIndex <OPERATOR_END; uIndex++)
        {
            if (!strcmp(OPERATOR_DESC_LIB[uIndex].pDesc, pOperator))
            {
                *pOperatorType = OPERATOR_DESC_LIB[uIndex].uOperatorType;
                bFind = TRUE;
                break;
            }
        }
    }
        
    return bFind;
}

BOOL get_protocal_type(char* protocal,U8* ProtocalType)
{
    BOOL bFind = FALSE;
    PROTOCAL_DESC *pProtDesc = &PROTOCAL_LIB[0];
    U8 uIndex = 0;

    if (NULL != pProtDesc)
    {
        for (uIndex = 0; uIndex <OPERATOR_END; uIndex++)
        {
            if (!strcmp(PROTOCAL_LIB[uIndex].Protocal, protocal))
            {
                *ProtocalType = PROTOCAL_LIB[uIndex].SataProtocal;
                bFind = TRUE;
                break;
            }
        }
    }
        
    return bFind;
}

BOOL IsSetBP(char* action_split,char* real_action,char* Temp)
{

    char *pdest;
    int  index;

    pdest = strchr(action_split,'=');
    if (pdest == NULL ) 
    {
        memcpy(real_action,action_split,strlen(action_split)+1);
        return FALSE;
    }
    index = (int)(pdest - action_split);
    memcpy(real_action,action_split,index);
    real_action[index] = '\0';
    strcpy(real_action,strstrip(real_action));

    memcpy(Temp,action_split + index + 1,strlen(action_split)-index);
    strcpy(Temp,strstrip(Temp));

    if(!strcmp(real_action,"SET_BP"))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

ACTION* new_action(COND_LIST* tmp_condlist)
{
    ACTION* tmp_action = NULL;
    tmp_action = (ACTION*)malloc(sizeof(ACTION));
    memset(tmp_action,0,sizeof(ACTION));
    if(tmp_action == NULL)
    {
        printf("Can't malloc memory!\n");
        DBG_Break();
    }
    tmp_action->pActionNext = NULL;

    return tmp_action;    
}
void add_action(COND_LIST* tmp_condlist,ACTION* paction)
{
 
    if(tmp_condlist->pActHead == NULL)
    {
        tmp_condlist->pActHead  = paction;
        tmp_condlist->pActTail  = paction;
    }
    else
    {
        tmp_condlist->pActTail->pActionNext  = paction;
        tmp_condlist->pActTail  = paction;
    }
}

U32 get_hscmd_id(char* Buffer)
{
    U32 CmdId = INVALID_8F;

    if (!strncmp("HSCMD",Buffer,5))
    {
        U32 i;
        char cmdid[64];
        char* s = Buffer;
        U32 len = (U32)strlen(Buffer);

        for(i=0;i<len;i++,s++)
        {
            if(*s >= '0' && *s <= '9')
            {
                memcpy(cmdid,s,len-i);
                break;
            }    
        }
        CmdId = strToInt(cmdid);
    }
    else
    {
        HSCMD_INFO* pHScmd = g_pHScmdList->pHSCmdHead;

        while(NULL != pHScmd)
        {
            if(0 == strcmp(pHScmd->CmdDsc,Buffer))
            {
                CmdId = pHScmd->CmdId;
                break;
            }
            pHScmd = pHScmd->pHSCmdNext;
        }
    }

    if(INVALID_8F == CmdId)
    {
        DBG_Printf("Can't get HSCmd Id!\n");
        DBG_Break();
    }

    return CmdId;
}

void parse_action(COND_LIST* tmp_condlist,char* action,char* param)
{
    char tmp[255];
    char *s;
    char action_split[255];
    char real_action[255];
    U8 start_pos = 0;
    U8 end_pos = 0;
    U8 len;
    U8 action_cnt = 0;
    U8 ActType = 0;
    char Temp[255];
    U8 BpNum = 0;
    ACTION* pACT = NULL;

    memset(tmp, 0, 255);
    strcpy(tmp, action);
    s = tmp;
    while(*s)
    {
        if(*s == '{')
        {            
            start_pos = end_pos + 1;
        }
        if(*s == '/' || *s == '}')
        {
            len = end_pos - start_pos;
            memcpy(action_split,action+start_pos,len);
            action_split[len] = '\0';

            //1.new a action
            pACT = new_action(tmp_condlist);

            //2.set action type and param
            if(TRUE == IsSetBP(action_split,real_action,Temp))
            {
                if(FALSE == set_bp_param(Temp,&pACT->BPNum,&pACT->m_BpParam))
                {
                    printf("Not find BP: %s\n",Temp);
                    DBG_Break();
                }
            }

            if(FALSE == get_actiontype((char*)real_action,(U8*)&pACT->ActType))
            {
                printf("Not find action type %s\n",real_action);
                DBG_Break();
            }

            if(pACT->ActType == ACT_TYPE_TRIM)
            {
                set_trim_param(real_action,&pACT->m_TrimParam);
            }
            else if(NULL != param && pACT->ActType == ACT_TYPE_INJECT_ERROR)
            {            
                if(FALSE == parse_injecterror_param(pACT,param))
                {
                    printf("parse_injecterror_param error!\n");
                    DBG_Break();
                }
            }
            else if(pACT->ActType == ACT_TYPE_SEND_HSCMD)
            {
                pACT->HSCmdId = get_hscmd_id(Temp);
            }
            else if(pACT->ActType == ACT_TYPE_WEAR_LEVELING_STATISTIC)
            {
                g_bWearLevelingStatistic = TRUE;
            }

            //3.add action to condlist
            add_action(tmp_condlist,pACT);

            start_pos = end_pos + 1;
        }
        s++;
        end_pos++;
    }
}

void parse_hostcond_action(COND_LIST* tmp_condlist,char* cond,char* action)
{
    char buffer[10] = {0};
    char oper[10] = {0};
    char cValue[20] = {0};
    U32 ulValue;
    U32 ulMaxLba = GetSystemMaxLBA();
    if(!strncmp(cond,"{0}",3))
    {
        tmp_condlist->CurCond.HCond.bNoCond = TRUE;                        
    }
    else
    {
        sscanf(cond, "{%s %s %[^}]", buffer, oper, cValue);

        strToUpper(buffer);
        ulValue = strToInt(cValue);
        if (0 == strcmp(buffer, "LBA"))
        {
            if (INVALID_8F == ulValue)
            {
                tmp_condlist->CurCond.HCond.Lba = ulMaxLba;
            }
            else
            {
                tmp_condlist->CurCond.HCond.Lba = ulValue;
            }
        }
        else if (0 == strcmp(buffer, "CMDCNT"))
        {
            tmp_condlist->CurCond.HCond.CmdCnt = ulValue;
        }
        else if (0 == strcmp(buffer, "TIME"))
        {
            tmp_condlist->CurCond.HCond.Time = ulValue;
        }

        if (FALSE == get_operatertype(oper, (U8*)&tmp_condlist->CurCond.HCond.Optr))
        {
            DBG_Printf("parse_hostcond_action: don't find the operator \n");
            DBG_Getch();
        }
    }

    parse_action(tmp_condlist,action,NULL);

    return;
}
U32 parse_data(char* buffer,U8* data)
{
    char* s;
    U32 len = 0;
    U32 Index = 0;
    char Temp[4] = {'0'};
    U32 i,j;

    s = buffer;
    len = (U32)strlen(buffer);
    j = 0;
    for(i=0; i<(len+1); i++)
    {
        if(*s == ',' || *s == '\r')
        {
            memcpy(Temp,&buffer[j],(i-j));
            data[Index] = strToInt(Temp);
            Index++;
            j = i+1;
        }
        s++;
    }
    return Index;
}
void parse_rowcmd_param(HSCMD_INFO* tmp_hscmd,char* value)
{
    char RowCmd[64*5];

    if(sscanf(value,"{%[^'.']}",RowCmd))
    {    
        parse_data(RowCmd,tmp_hscmd->RowCmd);
    }
    else
    {
        DBG_Printf("parse_cfis_param error\n");
        DBG_Break();
    }
}

SECTION* get_section(char* section)
{
    BOOL bFound = FALSE;
    SECTION* pSection = g_pCheckList->pSecHead;
    while(pSection != NULL)
    {
        if(strcmp(pSection->SecName,section) == 0)
        {
            bFound = TRUE;
            break;
        }
        pSection = pSection->pSecNext;
    }
    if(bFound == TRUE)
    {
        return pSection;
    }
    else
    {
        return NULL;
    }
}
void add_section(char* section)
{
    SECTION* temp;
    temp = get_section(section);

    if(temp == NULL)
    {
        temp = (SECTION*)malloc(sizeof(SECTION));
        memset(temp,0,sizeof(SECTION));
        if(temp == NULL)
        {
            printf("Can't malloc memory!\n");
            DBG_Break();
        }
        strcpy(temp->SecName,section);

        temp->pCondHead = NULL;
        temp->pCondTail = NULL;
        temp->pSecNext = NULL;

        temp->CondListCnt = 0;
        
        g_pCheckList->SectionCnt++;

        if(g_pCheckList->pSecHead == NULL)
        {
            g_pCheckList->pSecHead = temp;
            g_pCheckList->pSecTail = temp;
        }
        else
        {
            g_pCheckList->pSecTail->pSecNext = temp;
            g_pCheckList->pSecTail = temp;
        }
        
    }
}
HSCMD_INFO* get_hscmdlist(char* scmd_name)
{
    BOOL bRet = FALSE;
    HSCMD_INFO* pHScmd = g_pHScmdList->pHSCmdHead;

    while(NULL != pHScmd)
    {
        if(0 == strcmp(pHScmd->HSCmdName,scmd_name))
        {
            bRet = TRUE;
            break;
        }
        pHScmd = pHScmd->pHSCmdNext;
    }

    if(TRUE == bRet)
    {
        return pHScmd;
    }
    else
    {
        return NULL;
    }
}

HSCMD_INFO* lookup_hscmdlist(U32 hscmd_id)
{
    BOOL bRet = FALSE;
    HSCMD_INFO* pHScmd = g_pHScmdList->pHSCmdHead;

    while(NULL != pHScmd)
    {
        if(hscmd_id == pHScmd->CmdId)
        {
            bRet = TRUE;
            break;
        }
        pHScmd = pHScmd->pHSCmdNext;
    }

    if(TRUE == bRet)
    {
        return pHScmd;
    }
    else
    {
        return NULL;
    }
}

void add_hscmdlist(char* scmd_name)
{
    HSCMD_INFO* temp;
    temp = get_hscmdlist(scmd_name);

    if(NULL == temp)
    {
        temp = (HSCMD_INFO*)malloc(sizeof(HSCMD_INFO));
        if(NULL == temp)
        {
            printf("Can't malloc memory\n");
            DBG_Break();
        }
        memset(temp,0,sizeof(HSCMD_INFO));

        strcpy(temp->HSCmdName,scmd_name);

        g_pHScmdList->HSCmdCnt++;

        if(NULL == g_pHScmdList->pHSCmdHead)
        {
            g_pHScmdList->pHSCmdHead = temp;
            g_pHScmdList->pHSCmdTail = temp;
        }
        else
        {
            g_pHScmdList->pHSCmdTail->pHSCmdNext = temp;
            g_pHScmdList->pHSCmdTail = temp;
        }
    }

}

void add_hscmd_define(char* scmd_name,char* key,char* value)
{
    HSCMD_INFO* tmp_hscmd;
    char buffer[256];

    char line[1024];
    U32 Index = 0;
    char file_name[256];
    FILE* pFile;

    tmp_hscmd = get_hscmdlist(scmd_name);
    tmp_hscmd->CmdId = get_hscmd_id(scmd_name);
    if(!strcmp(key,"Desc"))
    {
        memcpy(tmp_hscmd->CmdDsc,value,strlen(value));
#ifndef MIX_VECTOR
        tmp_hscmd->pHSCmdCallBackFunc = get_hscmd_callback(tmp_hscmd->CmdDsc);
#endif
    }
    else if(!strcmp(key,"Protocal"))
    {
        if(FALSE == get_protocal_type(value,&tmp_hscmd->Protocal))
        {
            DBG_Printf("get_protocal_type error\n");
            DBG_Break();
        }
    }
    else if(!strcmp(key,"RowCmdLen"))
    {
        tmp_hscmd->RowCmdLen = strToInt(value);
    }
    else if(!strcmp(key,"RowCmd"))
    {
        parse_rowcmd_param(tmp_hscmd,value);
    }
    else if(!strcmp(key,"SecCnt"))
    {
        tmp_hscmd->DataSecCnt = strToInt(value);
    }
    else if(!strcmp(key,"Data"))
    {
#ifdef HOST_NVME
        //make buffer 4K align for NVMe
        tmp_hscmd->pDataBuffer = (U8*)malloc((tmp_hscmd->DataSecCnt * SEC_SIZE) + HPAGE_SIZE - 1);
        tmp_hscmd->pDataBuffer = (U8*)(((U32)tmp_hscmd->pDataBuffer + HPAGE_SIZE)  &(~ HPAGE_SIZE_MSK));
#else
        tmp_hscmd->pDataBuffer = (U8*)malloc(tmp_hscmd->DataSecCnt * SEC_SIZE);
#endif
        if(strlen(value) > 2)
        {
            if(!strncmp(value,"{",1))
            {
                if(sscanf(value,"{%[0-9a-zA-Z,]}",buffer))
                {
                    parse_data(buffer,tmp_hscmd->pDataBuffer);
                }
            }
            else
            {
                sprintf(file_name,"checklist/special_cmd_list/%s",value);
                pFile = fopen(file_name,"r+b");
                while(fgets(line,sizeof(line),pFile) != NULL)
                {
                    Index += parse_data(line,&tmp_hscmd->pDataBuffer[Index]);
                }
            }
        }
    }

}
COND_LIST* get_condlist(SECTION* sec,char* key)
{
    BOOL bFound = FALSE;
    COND_LIST* tmp;

    tmp = sec->pCondHead;

    while(tmp != NULL)
    {
        if(strcmp(key,tmp->CondName) == 0)
        {
            bFound = TRUE;
            break;
        }
        tmp = tmp->pCondListNext;
    }
    if(bFound == TRUE)
    {
        return tmp;
    }
    else
    {
        return NULL;
    }
}

void add_condlist(char* sec,char* key,char* value)
{
    SECTION* tmp_sec;
    COND_LIST* tmp_condlist;   
    char cond[256];
    char action[256];
    char param[256];
    U8 ret = 0;

    tmp_sec = get_section(sec);
    if(!strcmp(key,"Loop_begin"))
    {
        tmp_sec->LoopTimes = atoi(value);
        return;
    }
    else if(!strcmp(key,"Loop_end"))
    {
        return;
    }
    if(tmp_sec != NULL)
    {
        tmp_condlist = get_condlist(tmp_sec,key);
        if(tmp_condlist != NULL)
        {
            printf("%s condition %s already exist!\n",sec,key);
            DBG_Break();
        }
        tmp_condlist = (COND_LIST*)malloc(sizeof(COND_LIST));
        memset(tmp_condlist,0,sizeof(COND_LIST));

        if(tmp_condlist == NULL)
        {
            printf("Can't malloc memory!\n");
            DBG_Break();
        }
        tmp_condlist->pCondListNext = NULL;

        strcpy(tmp_condlist->CondName,key);

        if(!strncmp(key,"HC",2))
        {                
            if(sscanf(value,"%[^:]:%[^'\n']",cond,action) == 2
                || sscanf(value,"%[^:]:%[^;]",cond,action) == 2)
            {
                strcpy(cond,strstrip(cond));
                strcpy(action,strstrip(action));
                parse_hostcond_action(tmp_condlist,cond,action);
            }               
        }
        else if(!strncmp(key,"FC",2))
        {
            if(sscanf(value,"%[^:]:%[^:]:%[^'\n'])",cond,action,param) == 3
                || sscanf(value,"(%[^:]:%[^:]:%[^;])",cond,action,param) == 3)
            {
                strcpy(cond,strstrip(cond));
                strcpy(action,strstrip(action));
                strcpy(param,strstrip(param));
                parse_flashcond_action(tmp_condlist,cond,action,param);

            }  
            else if(sscanf(value,"%[^:]:%[^'\n'])",cond,action) == 2
                || sscanf(value,"(%[^:]:%[^;])",cond,action) == 2)
            {
                strcpy(cond,strstrip(cond));
                strcpy(action,strstrip(action));
                parse_flashcond_action(tmp_condlist,cond,action,NULL);

            } 
            else
            {
                printf("checklist format is not right\n");
                DBG_Getch();
            }
        }

        tmp_sec->CondListCnt++;
        if(tmp_sec->pCondHead == NULL)
        {
            tmp_sec->pCondHead = tmp_condlist;
            tmp_sec->pCondTail = tmp_condlist;
        }
        else
        {
            tmp_sec->pCondTail->pCondListNext = tmp_condlist;
            tmp_sec->pCondTail = tmp_condlist;
        }
        
    }
    else
    {
        add_section(sec);
        add_condlist(sec,key,value);
    }
}
ACTION* get_action(COND_LIST* cond_list,U8 ActType)
{
    BOOL bFound = FALSE;
    ACTION* tmp;

    tmp = cond_list->pActHead;

    while(tmp != NULL)
    {
        if(ActType == tmp->ActType)
        {
            bFound = TRUE;
            break;
        }
        tmp = tmp->pActionNext;
    }
    if(bFound == TRUE)
    {
        return tmp;
    }
    else
    {
        return NULL;
    }
}
BOOL delete_condlist(SECTION* sec,char* CondName,COND_LIST** pre_cond)
{
    COND_LIST* tmp;
    COND_LIST* tmp_pre;
    BOOL bRet = FALSE;

    if(sec == NULL)
    {
        return FALSE;
    }

    tmp = sec->pCondHead;
    while(tmp != NULL)
    {
        if(!strcmp(tmp->CondName,CondName))
        {
            //find this condlist,delete it
            if(tmp == sec->pCondHead)
            {
                sec->pCondHead = tmp->pCondListNext;
            }
            else
            {
                tmp_pre->pCondListNext = tmp->pCondListNext;
                *pre_cond = tmp_pre;
            }
            g_CurCond = NULL;
            sec->CondListCnt--;
            remove_action(tmp);
            free(tmp);
            bRet = TRUE;
            break;
        }
        tmp_pre = tmp;
        tmp = tmp->pCondListNext;
    }
    return bRet;
}
BOOL remove_action(COND_LIST* cond)
{
    ACTION* act;
    BOOL bRet = FALSE;

    if(cond == NULL)
    {
        return FALSE;
    }

    act = cond->pActHead;
    while(act != NULL)
    {
        cond->pActHead = act->pActionNext;
        free(act);
        act = cond->pActHead;
        bRet = TRUE;
    }
    return bRet;
}
BOOL remove_condlist(SECTION* sec)
{
    COND_LIST* tmp;
    BOOL bRet = FALSE;

    if(sec == NULL)
    {
        return FALSE;
    }
    tmp = sec->pCondHead;
    while(tmp != NULL)
    {
        sec->pCondHead = tmp->pCondListNext;
        sec->CondListCnt--;
        remove_action(tmp);
        free(tmp);
        tmp = sec->pCondHead;
        bRet = TRUE;       
    }
    return bRet;
}
BOOL remove_section(char* sec)
{
    SECTION* pSec = g_pCheckList->pSecHead;
    SECTION* temp;

    if(pSec == NULL)
        return FALSE;

    if(strcmp(sec,pSec->SecName) == 0)
    {
        remove_condlist(pSec);
        g_pCheckList->pSecHead = pSec->pSecNext;
        g_pCheckList->SectionCnt--;
        free(pSec);
        return TRUE;
    }

    while(pSec != NULL)
    {
        if(strcmp(pSec->pSecNext->SecName,sec) == 0)
        {
            remove_condlist(pSec->pSecNext);
            temp = pSec->pSecNext;
            pSec->pSecNext = pSec->pSecNext->pSecNext;
            g_pCheckList->SectionCnt--;
            free(temp);
            break;
        }
        pSec = pSec->pSecNext;
    }
    return TRUE;
}

void clear_checklist()
{
    SECTION* sec = g_pCheckList->pSecHead;
    if(g_pCheckList == NULL)
        return;

    sec = g_pCheckList->pSecHead;
    while(sec != NULL)
    {
        g_pCheckList->pSecHead = sec->pSecNext;
        g_pCheckList->SectionCnt--;
        remove_condlist(sec);
        free(sec);
        sec = g_pCheckList->pSecHead;
    }

    g_CurSec = NULL;
    g_CurCond = NULL;
    g_CurAct = NULL;
}
void print_checklist()
{
    SECTION* sec = g_pCheckList->pSecHead;
    COND_LIST* condlist;
    ACTION* action;

    while(sec != NULL)
    {
        printf("[%s]\n",sec->SecName);
        condlist = sec->pCondHead; 

        printf("loop_cnt: %d\n",sec->LoopTimes);
        while(condlist != NULL)
        {
            printf("%s,[cond] ",condlist->CondName);
            if(condlist->CurCond.HCond.bNoCond)
            {
                printf("bNoCond:%d,",condlist->CurCond.HCond.bNoCond);
            }
            else if(condlist->CurCond.HCond.Lba)
            {
                printf("lba:0x%x,",condlist->CurCond.HCond.Lba);
            }
            else if(condlist->CurCond.HCond.CmdCnt)
            {
                printf("CmdCnt:%d,",condlist->CurCond.HCond.CmdCnt);
            }

            if(condlist->CurCond.FCond.bNoCond)
            {
                printf("bNoCond:%d,",condlist->CurCond.FCond.bNoCond);
            }
            else if(condlist->CurCond.FCond.OpType)
            {
                printf("OpType:%d,",condlist->CurCond.FCond.OpType);
            }
            action = condlist->pActHead;
            while(action != NULL)
            {
                printf("[action] ActType:%d,",action->ActType);
                if(action->ActType == ACT_TYPE_SETBP)
                {
                    printf("BPNum:%d,lba %d,sec_cnt %d",action->BPNum,
                        action->m_BpParam.ulStartLba,action->m_BpParam.ulSecCnt);
                }
                else if(action->ActType == ACT_TYPE_INJECT_ERROR)
                {
                    printf("CE 0x%x Blk 0x%x Pg 0x%x CmdCode 0x%x ErrType %d\n",action->m_ErrInjParam.CE,action->m_ErrInjParam.PhyBlk,action->m_ErrInjParam.PhyPg,
                        action->m_ErrInjParam.CmdCode,action->m_ErrInjParam.ErrType);
                }
                action = action->pActionNext;
            }
            printf("\n");
            condlist = condlist->pCondListNext;
        }
        sec = sec->pSecNext;
    }
    printf("\n");

}
void init_checklist()
{
    g_pCheckList = (CHECK_LIST*)malloc(sizeof(CHECK_LIST));
    memset(g_pCheckList,0,sizeof(CHECK_LIST));
    g_pCheckList->pSecHead = NULL;
    g_pCheckList->pSecTail = NULL;
    g_pCheckList->SectionCnt = 0;
    g_HostPatternStatus = LOAD_SECTION;


}

void init_checkListfile()
{
    char FilePath[256] = "checklist";

    static char file_name[256];
    //char file_dstname[256];

    sprintf(file_name, "%s\\*.*", FilePath);
    GetChecklistInit(file_name);

    return;
}
void parse_scmd_list_ini()
{
#ifndef HOST_NVME
    static char file_name[256] = "checklist/special_cmd_list/special_cmd_list.ini";
#else
    static char file_name[256] = "checklist/special_cmd_list/special_cmd_list_nvme.ini";
#endif

    g_pHScmdList = (HSCMD_LIST*)malloc(sizeof(HSCMD_LIST));
    memset(g_pHScmdList,0,sizeof(HSCMD_LIST));
    g_pHScmdList->pHSCmdHead = NULL;
    g_pHScmdList->pHSCmdTail = NULL;    

    scmd_parser_load(file_name);
}

void dump_checklist_info(char* fileName)
{
    FILE* pDump = NULL;
    static U32 bInit = 0;
    if(bInit == 0)
    {
        bInit = 1;
        pDump = fopen("RunChecklistInfo.txt", "w");
    }
    else
        pDump = fopen("RunChecklistInfo.txt", "a+");

    if(NULL == pDump)
    {
        perror("Open RunChecklistInfo.txt fail\n");
        DBG_Getch();
    }

    fprintf(pDump, "start to run %s \n",fileName);

    fclose(pDump);
}

BOOL load_one_checklist()
{
    //static U32 state = 0;
    char FilePath[256] = "checklist";

    static char file_name[256];
    char file_dstname[256];
    BOOL bRet = FALSE;

    memset(file_name, 0, 256);
    GetChecklistName(file_name);

    if ((file_name[0] != '@') && (file_name[0] != '!')) 
    {
        printf("File Name: %s\n", file_name);
        dump_checklist_info(file_name);
        sprintf(file_dstname, "%s\\%s", FilePath, file_name);

        init_checklist();
        if(iniparser_load(file_dstname) == FALSE)
        {
            printf("iniparser_load fail\n");
            DBG_Break();
        }
        print_checklist();
        bRet = TRUE;
        //clear_checklist();
    }
    else
    {
        bRet = FALSE;
    }
        
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////////////

BOOL load_one_section()
{
    if(g_CurSec == NULL)
    {
        g_CurSec = g_pCheckList->pSecHead;
    }
    else
    {
        g_CurSec = g_CurSec->pSecNext;
    }

    if(g_CurSec != NULL)
    {
        DBG_Printf("Load Section: %s\n", g_CurSec->SecName);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
BOOL load_one_condlist()
{
    if(g_CurCond == NULL)
    {
        g_CurCond = g_CurSec->pCondHead;
    }
    else
    {
        g_CurCond = g_CurCond->pCondListNext;
    }

    if(g_CurCond != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL load_one_fcondlist(COND_LIST **pFCond)
{
    COND_LIST *pCurCond = NULL;
    
    BOOL bRtn = TRUE;

    if(NULL == *pFCond)
    {
        if (NULL == g_CurSec)
        {
            pCurCond = NULL;
        }
        else
        {
            pCurCond = g_CurSec->pCondHead;
        }
    }
    else
    {
        pCurCond = (*pFCond)->pCondListNext;
    }
   
    while ((NULL != pCurCond) && (FALSE == isFCond(pCurCond)))
    {
        pCurCond = pCurCond->pCondListNext;
    }

    if (NULL == pCurCond)
        bRtn = FALSE;

    *pFCond = pCurCond;

    return bRtn;
}

BOOL load_one_action()
{
    if(g_CurAct == NULL)
    {
        g_CurAct = g_CurCond->pActHead;
    }
    else
    {
        g_CurAct = g_CurAct->pActionNext;
    }

    if(g_CurAct != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void DoExitAction()
{
    g_CurSec->RunCounter++;
    g_CurSec->bDoneOnce = TRUE;
}
BOOL sim_trim_cmd_process()
{
    static U8 state = 0;
    BOOL bRet = FALSE;

    switch(g_CurAct->m_TrimParam.TrimType)
    {
    case RANDOM_TRIM:
            if(TRUE == sim_send_trim_cmd())
            {
                bRet = TRUE;
            }

        break;
    case TRIM_WITH_PARAM:
        {
            if(state == 0)
            {
                memcpy(&g_TrimParam,&g_CurAct->m_TrimParam,sizeof(TRIM_PARAM));
                state = 1;
            }
            else if(state == 1)
            {
                if(TRUE == sim_trim_one_range(&g_TrimParam.ulTrimStartLba,&g_TrimParam.ulLength))
                {
                    DBG_Printf("Send trim cmd startLba 0x%x,len 0x%x\n",
                        g_CurAct->m_TrimParam.ulTrimStartLba ,g_CurAct->m_TrimParam.ulLength);
                    bRet = TRUE;
                    state = 0;
                }
            }
        }
        break;
    default:
        break;
    }
    return bRet;
}
BOOL do_one_action()
{
    BOOL bRet = FALSE;
    switch(g_CurAct->ActType)
    {
    case ACT_TYPE_SETBP:
        g_BPP->pBaseFunc = GetBaseFunction(g_CurAct->BPNum);
        g_BPP->BPNum = g_CurAct->BPNum;
        memcpy(&g_BPP->pBaseFunc->FuncParameter,&g_CurAct->m_BpParam,sizeof(FUNCTION_PARAMETER));
        DBG_Printf("---------start %s----------\n",get_bp_str(g_BPP->BPNum));
        bRet = TRUE;
        break;
    case ACT_TYPE_NORMAL_SHUTDOWN:
        if(TRUE == Sim_NormalBootSchdedule())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_ABNORMAL_SHUTDOWN:
        if(TRUE == Sim_AbnormalBoot_WaitL3Empty())
        {
            bRet = TRUE;
        }
        break;
   case ACT_TYPE_ABNORMAL_SHUTDOWN_NOTWAIT_L3EMPTY:
        if(TRUE == Sim_AbnormalBoot_NotWaitL3Empty())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_TRIM:
        if(TRUE == sim_trim_cmd_process())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_LOAD_TRACE_LOG:
        if (TRUE == Sim_LoadTraceLog())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_FLASH_HANDLE:
        if (TRUE == Sim_FlashHandle())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_LOAD_TRACE_FROM_FLASH:
        if (TRUE == Sim_LoadTraceFromFlash())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_GET_VARTABLE_DATA:
#ifndef MIX_VECTOR
        if (TRUE == Sim_GetVarTableData())
        {
            bRet = TRUE;
        }
#endif
        break;
    case ACT_TYPE_INJECT_ERROR:
        memcpy(&ErrParamSlot[g_InjErrTail],&g_CurAct->m_ErrInjParam,sizeof(ERR_INJ_PARRM));
        g_InjErrTail++;
        bRet = TRUE;
        break;
    case ACT_TYPE_LLF:
        if(TRUE == Sim_Redo_LLF())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_EXIT:
        {
            DoExitAction();
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_IDLE:
        if(TRUE == Sim_SystemIdle())
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_SEND_HSCMD:
        if(TRUE == Sim_Send_HSCMD(g_CurAct->HSCmdId))
        {
            bRet = TRUE;
        }
        break;
    case ACT_TYPE_WEAR_LEVELING_STATISTIC:
        if(TRUE == Sim_WearLevelingStatus())
        {
            bRet = TRUE;
        }
        break;
    default:
        break;
    }
   
    return bRet;
}
BOOL is_NoCondAct()
{
    if(g_CurCond->CurCond.HCond.bNoCond == TRUE || g_CurCond->CurCond.FCond.bNoCond == TRUE)
        return TRUE;
    else
        return FALSE;
}

void ResetSectionParam(void)
{
    // InitSection
    g_CurSec->ulCMDCnt = 0;
    g_CurSec->ulWSecCnt = 0;
    g_CurSec->bDoneOnce = FALSE;
    g_BPP->pBaseFunc = NULL;
    g_BPP->BPNum = 0;
}


BOOL DoEvtLoadSection()
{
    BOOL bLoad =FALSE;
    bLoad = load_one_section();
    if(TRUE == bLoad)
    {  
        GetLocalTime( &sys_time_start );
        ResetSectionParam();
        g_HostPatternStatus = INI_SECTION;
    }
    else
    {
        DBG_Printf("\n\n********************The checklist run done!********************\n");
        g_HostPatternStatus = RUN_SECTION_END;
    }

    return bLoad;
}

BOOL DoEvtInitSection()
{
    BOOL bLoad = FALSE;
    BOOL bDone = FALSE;
    BOOL bExit = FALSE;
    BOOL bIsNoCondAct = FALSE;
    COND_LIST* pPreCond = NULL;
    
    g_CurSec->ulCMDCnt = 0;
    g_CurSec->ulWSecCnt = 0;
    g_CurSec->bDoneOnce = FALSE;
    // 1. load one condition
    if (NULL == g_CurAct)
    {
        do
        {
            bIsNoCondAct = FALSE;
            bLoad = load_one_condlist();
            if (TRUE == bLoad)
            {
                bIsNoCondAct = is_NoCondAct();
            }
            if (TRUE == bIsNoCondAct)
            {
                break;
            }
    
        }while (TRUE == bLoad);
    }
    
    // 2. load one action
    if ( TRUE == bLoad &&  NULL == g_CurAct)
    {
        load_one_action();

    }

    //3. do one action, then load one action
    if (NULL != g_CurAct)
    {
        bDone = do_one_action();
        if (TRUE == bDone)
        {
            if(g_CurAct->ActType == ACT_TYPE_INJECT_ERROR || g_CurAct->ActType == ACT_TYPE_LLF)
            {
                delete_condlist(g_CurSec,g_CurCond->CondName,&pPreCond);
                g_CurCond = pPreCond;
                g_CurAct = NULL;
            }
            else
            {
                load_one_action();
            }
        }
    }
    
    //4. if all NoCondition action have been handle
    //   exit the initial process
    if (NULL == g_CurCond)
    {
        bExit = TRUE;
        g_HostPatternStatus = RUN_BASE_PATTERN;
    }

    return bExit;
}


BOOL DoOperator(U32 LValue, U32 RValue, U8 uOperator)
{
    DO_OPERATER(LValue, RValue, uOperator);

    return TRUE;
}
BOOL check_flash_cond()
{
    BOOL bRet = FALSE;
    static U16 ulPPO = 0;

    FCMD_COND* pFCmdCond = (FCMD_COND*)&(g_CurCond->CurCond.FCond);

    if( (0 != pFCmdCond->FCmdConDoCnt) &&(pFCmdCond->FCmdConDoCnt >= pFCmdCond->FCmdConHitCnt))
    {
        switch(pFCmdCond->PPOType)
        {
        case MOD2_PPO:
            pFCmdCond->PhyPg = ulPPO;
            ulPPO += 2;
            if(ulPPO >= PG_PER_BLK)
            {
                ulPPO = 0;
            }
            break;
        case RAND_PPO:
            pFCmdCond->PhyPg = rand()%(PG_PER_BLK-1);
            break;
        default:
            break;
        }
        pFCmdCond->FCmdConDoCnt = 0;     
        bRet = TRUE;
    }

    return bRet;
}

union {
FILETIME ft;
LONG64 val;
}t1,t2;

U32 CaculateTime(SYSTEMTIME sys_time_cur,SYSTEMTIME sys_time_start)
{
    U32 time_dur_min;
    LONG64 secs;
    
    if(sys_time_cur.wYear < sys_time_start.wYear)
    {
        printf("error time!");
        DBG_Getch();
    }

    SystemTimeToFileTime(&sys_time_start, &t1.ft);
    SystemTimeToFileTime(&sys_time_cur, &t2.ft);

    secs=t2.val-t1.val;
    time_dur_min = (U32)(secs/600000000);

    return time_dur_min;


}

BOOL IsHitCondition(COND_LIST *pCond)
{
    BOOL bLBAHit = FALSE;
    BOOL bCmdHit = FALSE;
    BOOL bTimeHit = FALSE;

    SYSTEMTIME sys_time_cur;
    U32 Time_Min = 0;

    if(TRUE == isFCond(pCond))
    {
        if(TRUE == check_flash_cond())
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else  if(TRUE == isHCond(pCond))
    {
        if (pCond->CurCond.HCond.Lba != 0)
        {
            bLBAHit = DoOperator(g_BPP->pBaseFunc->FuncParameter.ulStartLba, 
                pCond->CurCond.HCond.Lba,
                pCond->CurCond.HCond.Optr);

            if (TRUE == bLBAHit)
            {
                printf("Hit lba:0x%x\n",g_CurCond->CurCond.HCond.Lba);
            }
        }
        else
        {
            bLBAHit = TRUE;
        }

        if (pCond->CurCond.HCond.CmdCnt != 0)
        {
            bCmdHit = DoOperator(g_CurSec->ulCMDCnt,
                pCond->CurCond.HCond.CmdCnt,
                pCond->CurCond.HCond.Optr);
            if (TRUE == bCmdHit)
            {
                printf("Hit CmdCnt:%d,cmd_cnt %d\n",g_CurCond->CurCond.HCond.CmdCnt,g_CurSec->ulCMDCnt);
            }
        }
        else 
        {
            //cmd_cnt = 0;
            bCmdHit = TRUE;
        }
        
        if(pCond->CurCond.HCond.Time != 0)
        {
              GetLocalTime( &sys_time_cur );
              //printf("start time: %d:%d:%d\n",sys_time_start.wHour,sys_time_start.wMinute,sys_time_start.wSecond);
                  //printf("current time: %d:%d:%d\n",sys_time_cur.wHour,sys_time_cur.wMinute,sys_time_cur.wSecond);
                  Time_Min = CaculateTime(sys_time_cur,sys_time_start);
                  //printf("during time: %d\n",Time_Min);
                  bTimeHit = DoOperator(Time_Min,
                pCond->CurCond.HCond.Time,
                pCond->CurCond.HCond.Optr);
                  if (TRUE == bTimeHit)
            {
                printf("Hit Time:%d,cmd_cnt %d\n",g_CurCond->CurCond.HCond.Time,Time_Min);
                        GetLocalTime( &sys_time_start );
            }
            }
            else
            {
                   bTimeHit = TRUE;
            }
        
        return (bCmdHit & bLBAHit & bTimeHit);
    }
    else
    {
        DBG_Printf("No such condition:%s\n",pCond->CondName);
        DBG_Getch();
    }
    return FALSE;
}

BOOL DoEvtCheckCondition()
{
    BOOL bLoad = FALSE;
    BOOL bDone = FALSE;
    BOOL bExit = FALSE;
    BOOL bIsCondAct = FALSE;
    BOOL bHit = FALSE;
    
    // 1. load on condition
    if (NULL == g_CurAct)
    {
        do
        {
            bIsCondAct = FALSE;
            bLoad = load_one_condlist();

            if (TRUE == bLoad)
            {
                bIsCondAct = !is_NoCondAct();

            }
            if (TRUE == bIsCondAct)
            {
                
                bHit = IsHitCondition(g_CurCond);
                if (TRUE == bHit)
                {
                    break;
                }
            }
    
        }while (TRUE == bLoad);
    }
    
    // 2. load one action
    if ( TRUE == bLoad &&  NULL == g_CurAct)
    {
        load_one_action();

    }

    //3. do one action, then load one action
    if (NULL != g_CurAct)
    {
        bDone = do_one_action();
        if (TRUE == bDone)
        {
            load_one_action();
        }
    }
    
    //4. if all NoCondition action have been handle
    //   exit the initial process
    if (NULL == g_CurCond)
    {
        bExit = TRUE;
        g_HostPatternStatus = CHECK_SECTION;
    }

    return bExit;
}

BOOL DoEvtCheckSection()
{
    if((g_CurSec->RunCounter < g_CurSec->LoopTimes) || (g_CurSec->LoopTimes == 0))
    {
        if (TRUE == g_CurSec->bDoneOnce)
        {
            DBG_Printf("%s has run %d times!\n",g_CurSec->SecName,g_CurSec->RunCounter);
            g_HostPatternStatus = INI_SECTION;
        }
        else
        {
            g_HostPatternStatus = RUN_BASE_PATTERN;
        }
    }
    else
    {
        g_HostPatternStatus = LOAD_SECTION;
    }

    return TRUE;
}

BOOL DoEvtRunPattern()
{
    
    if (0 == GetSystemMaxLBA())
    {
        SetSystemMaxLBA(MAX_LBA_IN_SYSTEM);    
    }

    if (NULL != g_BPP->pBaseFunc)
    {
        if(TRUE == run_base_pattern(g_BPP->pBaseFunc))
        {
      
            g_CurSec->ulCMDCnt++;

            if (0 == (g_CurSec->ulCMDCnt % 10000))
            {
                printf("cmd_cnt = %d\n",g_CurSec->ulCMDCnt);
            }

            g_HostPatternStatus = CHECK_CONDLIST;

        }
    }
    else
    {
        g_HostPatternStatus = CHECK_CONDLIST;
    }

    return TRUE;
}


BOOL run_one_checklist()
{
  
    if ( NULL == l_HosProcessArray[g_HostPatternStatus])
    {
        return TRUE;
    }
    else
    {
        l_HosProcessArray[g_HostPatternStatus]();
        return FALSE;
    }

}
void inject_error_init()
{
    g_InjErrHead = 0;
    g_InjErrTail = 0;
    memset(&ErrParamSlot[0],0,ERR_INJ_SLOT_MAX*sizeof(ERR_INJ_PARRM));
}
void module_run_init()
{
    g_CurSec = NULL;
    g_CurCond = NULL;
    g_CurAct = NULL;
    g_BPP = (BP_PARAM*)malloc(sizeof(BP_PARAM));

    //g_Fcnd.OpType = OP_TYPE_ALL;
    g_pFCOND = NULL; 

    init_checkListfile();
    ini_base_fuction();   
    inject_error_init();

    parse_scmd_list_ini();

    g_HostPatternStatus = LOAD_SECTION;

    g_TableRebuildType = TABLE_REBUILD_NONE;
    g_bInjectError = FALSE;

    hFileInjErr = OpenInjErrFile("InjectErrRecord.txt");
}


HANDLE OpenInjErrFile(char* fileName)
{
    HANDLE  hFile;

    hFile = CreateFile(fileName,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "fileName", GetLastError());
        DBG_Break();
    }  
    return hFile;
}






