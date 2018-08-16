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
*******************************************************************************
  File Name     : checkList_parse.h
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
#ifndef _CHECKLIST_PARSE_H
#define _CHECKLIST_PARSE_H

#include <Windows.h>
// include FW
#include "BaseDef.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"
#include "Disk_Config.h"
//#include "L0_Ahci.h"

#include "hscmd_parse.h"
// include model
#include "base_pattern.h"

#define ERR_INJ_SLOT_MAX  32

typedef enum _OPERATOR
{
    GTE = 0, // >=
    GT,      // >
    LTE,     // <=
    LT,      // <
    NE,      // !=
    EQUAL,   // ==
    MOD,     // %
    OPERATOR_END,
}OPERATOR;

typedef struct _OPERATOR_DESC
{
    OPERATOR uOperatorType;
    char *pDesc;
}OPERATOR_DESC;

static OPERATOR_DESC OPERATOR_DESC_LIB[OPERATOR_END] = 
{
    {GTE, ">="},
    {GT, ">"},
    {LTE, "<="},
    {LT, "<"},
    {NE, "!="},
    {EQUAL, "=="},
    {MOD, "%"},

};



#define DO_OPERATER(LVavlue, RValue, Operator) \
do{ \
    if (Operator == GTE) \
        return ((LValue >= RValue) ? 1:0); \
    if (Operator == GT) \
        return ((LValue > RValue) ? 1:0); \
    if (Operator == LTE) \
        return ((LValue <= RValue) ? 1:0); \
    if (Operator == LT) \
        return ((LValue < RValue) ? 1:0); \
    if (Operator == NE) \
        return ((LValue != RValue) ? 1:0); \
    if (Operator == EQUAL) \
        return ((LValue == RValue) ? 1:0); \
    if (Operator == EQUAL) \
        return ((LValue == RValue) ? 1:0); \
    if (Operator == MOD) \
        return (( 0 == (LValue%RValue)) ? 1:0); \
 }while(0)

typedef enum _PPO_TYPE
{
    RAND_PPO = 0,
    MOD2_PPO,
    PPO_TYPE_ALL,
}PPO_TYPE;

//2 Dwrod
typedef struct _FCMD_COND
{
    U8 CE;
    U16 PhyBlk;
    U16 PhyPg;
    PPO_TYPE PPOType;
    U8 PageType;

    U8 CmdCode;
    U8 OpType;

    BOOL bNoCond;
    U16 FCmdConHitCnt;
    U16 FCmdConDoCnt;
}FCMD_COND;    

//3 Dword
typedef struct _HCMD_CON
{
    BOOL bNoCond;
    U32 Lba;
    U32 CmdCnt;
    U32 Time;
    OPERATOR Optr;
}HCMD_COND; 

typedef struct _COND
{  
    HCMD_COND HCond;
    FCMD_COND FCond;
}COND;

typedef struct _ERR_INJ_PARRM
{
    U8 CE;
    U16 PhyBlk;
    U16 PhyPg;
    U8 PageType;

    U8 CmdCode;
    U8 OpType;
    U8 ErrType;
    U8 RetryTime;
    BOOL bClear;
    BOOL bHit;

}ERR_INJ_PARRM;

typedef enum _ACTION_TYPE
{
    ACT_TYPE_SETBP = 1,
    ACT_TYPE_NORMAL_SHUTDOWN,
    ACT_TYPE_ABNORMAL_SHUTDOWN,
    ACT_TYPE_ABNORMAL_SHUTDOWN_NOTWAIT_L3EMPTY,
    ACT_TYPE_INJECT_ERROR,
    ACT_TYPE_SEND_HSCMD,
    ACT_TYPE_IDLE,
    ACT_TYPE_LLF,
    ACT_TYPE_TRIM,
    ACT_TYPE_WEAR_LEVELING_STATISTIC,
    ACT_TYPE_LOAD_TRACE_LOG,
    ACT_TYPE_FLASH_HANDLE,
    ACT_TYPE_LOAD_TRACE_FROM_FLASH,
    ACT_TYPE_GET_VARTABLE_DATA,
    ACT_TYPE_EXIT,
    ACT_TYPE_ALL,
}ACTION_TYPE;

typedef struct _BP_PARAM
{
    U16 BPNum;
    pBASE_FUNCTION pBaseFunc; 
}BP_PARAM;

typedef enum _TRIM_TYPE
{
    TRIM_WITH_PARAM = 0,
    RANDOM_TRIM,
    TRIM_TYPE_ALL,
}TRIM_TYPE;
typedef struct TRIM_PARAM
{
    TRIM_TYPE TrimType;
    U32 ulTrimStartLba; 
    U32 ulLength; 
}TRIM_PARAM;

typedef struct _ACTION
{
    ACTION_TYPE ActType;
    ERR_INJ_PARRM m_ErrInjParam;
    TRIM_PARAM m_TrimParam;
    U16 BPNum;
    FUNCTION_PARAMETER m_BpParam;
    U16 HSCmdId;
    struct _ACTION* pActionNext;
}ACTION;


typedef struct _COND_LIST
{
    char CondName[255];

    COND CurCond;       
    ACTION* pActHead;
    ACTION* pActTail;
    struct _COND_LIST* pCondListNext;
}COND_LIST;

typedef struct _SECTION
{
    U32 LoopTimes;
    U32 RunCounter;
    U32 ulCMDCnt;
    U32 ulWSecCnt;

    U32 CondListCnt;
    COND_LIST* pCondHead;        
    COND_LIST* pCondTail; 

    char SecName[255];
    struct _SECTION* pSecNext;
    BOOL bDoneOnce;

}SECTION;

typedef struct _CHECK_LIST
{
    U32 SectionCnt;

    SECTION* pSecHead;
    SECTION* pSecTail;

}CHECK_LIST;


//////////////////////////////////////////////////////////////
typedef enum _RUN_SECTION_STATE
{
    LOAD_SECTION = 0,
    INI_SECTION,
    RUN_BASE_PATTERN,
    CHECK_CONDLIST,
    CHECK_SECTION,  
    RUN_SECTION_END,  // 5
}RUN_SECTION_STATE;

typedef BOOL (*FPHostProcess)();

 

extern SECTION* g_CurSec; //current running section pointer
extern COND_LIST* g_CurCond;
extern ACTION* g_CurAct;
extern BP_PARAM* g_BPP; //current BasePattern pointer

typedef struct BP_DESC{
    U8 BPNum;
    char* BP_Desc;
}BP_DESC;

static BP_DESC BP_LIB[] =
{
    {0,"SeqR"},
    {1,"SeqW"},
    {2,"RndR"},
    {3,"RndW"},  
    {4,"File"},
    {5,"MixRW"},
    {9,"END"},
};

typedef struct PAGE_TYPE_DESC{
    PAGE_TYPE PgType;
    char* PgTypeDesc;
}PAGE_TYPE_DESC;

static PAGE_TYPE_DESC PAGE_TYPE_LIB[] =
{
    {PAGE_TYPE_RPMT,"RPMT"},
    {PAGE_TYPE_DATA,"DATA"},
    {PAGE_TYPE_PMT,"PMT"},
    {PAGE_TYPE_GB,"GB"},
    {PAGE_TYPE_BBT,"BBT"},
    {PAGE_TYPE_RT,"RT"},
    {PAGE_TYPE_PMTI,"PMTI"},
    {PAGE_TYPE_VBMT,"VBMT"},
    {PAGE_TYPE_PBIT,"PBIT"},
    {PAGE_TYPE_VBT,"VBT"},
    {PAGE_TYPE_DPBM,"DPBM"},
    {PAGE_TYPE_TRACE,"TRACE"},
    {PAGE_TYPE_RSVD,"END"},
};

typedef struct _OP_TYPE_DESC{
    OP_TYPE OPType;
    char* OPTypeDesc;
}OP_TYPE_DESC;

static OP_TYPE_DESC OP_TYPE_LIB[] =
{
    {OP_TYPE_HOST_WRITE,"HOST_WRITE"},
    {OP_TYPE_GC_WRITE,"GC_WRITE"},
    {OP_TYPE_WL_WRITE,"WL_WRITE"},
    {OP_TYPE_RPMT_WRITE,"RPMT_WRITE"},
    {OP_TYPE_PMT_WRITE,"PMT_WRITE"},
    {OP_TYPE_GB_WRITE,"GB_WRITE"},
    {OP_TYPE_BBT_WRITE,"BBT_WRITE"},
    {OP_TYPE_RT_WRITE,"RT_WRITE"},
    {OP_TYPE_PMTI_WRITE,"PMTI_WRITE"},
    {OP_TYPE_VBMT_WRITE,"VBMT_WRITE"},
    {OP_TYPE_PBIT_WRITE,"PBIT_WRITE"},
    {OP_TYPE_VBT_WRITE,"VBT_WRITE"},
    {OP_TYPE_DPBM_WRITE,"DPBM_WRITE"},
    {OP_TYPE_TRACE_WRITE,"TRACE_WRITE"},
    {OP_TYPE_ALL,"END"},
};

typedef struct _ERR_TYPE_DESC{
    U8 ErrType;
    char* ErrTypeDesc;
}ERR_TYPE_DESC;

static ERR_TYPE_DESC ERR_TYPE_LIB[] = 
{
    {NF_ERR_TYPE_UECC,"UECC"},
    {NF_ERR_TYPE_PRG,"PRG"},
    {NF_ERR_TYPE_ERS,"ERS"},
    {NF_ERR_TYPE_RECC,"RECC"},
    {NF_ERR_TYPE_EMPTY_PG,"EMPTY_PG"},
    {0xFF,"END"},
};

typedef struct _ACTION_TYPE_DESC{
    ACTION_TYPE ActType;
    char* ActTypeDesc;
}ACTION_TYPE_DESC;

static ACTION_TYPE_DESC ACTION_TYPE_LIB[] = 
{
    {ACT_TYPE_SETBP,"SET_BP"},
    {ACT_TYPE_NORMAL_SHUTDOWN,"NormalShutDown"},
    {ACT_TYPE_ABNORMAL_SHUTDOWN,"AbnormalShutDown"},
    {ACT_TYPE_ABNORMAL_SHUTDOWN_NOTWAIT_L3EMPTY,"NotWaitL3Empty_AbnormalShutDown"},
    {ACT_TYPE_INJECT_ERROR,"InjectError"},
    {ACT_TYPE_SEND_HSCMD,"SendHSCMD"},
    {ACT_TYPE_LLF,"LLF"},
    {ACT_TYPE_TRIM,"Trim"},
    {ACT_TYPE_WEAR_LEVELING_STATISTIC,"WLstatistic"},
    {ACT_TYPE_IDLE,"Idle"},
    {ACT_TYPE_LOAD_TRACE_LOG,"LoadTraceLog"},
    {ACT_TYPE_FLASH_HANDLE,"FlashHandle"},
    {ACT_TYPE_LOAD_TRACE_FROM_FLASH,"LoadTraceFromFlash"},
    {ACT_TYPE_GET_VARTABLE_DATA,"GetVarTableData"},
    {ACT_TYPE_EXIT,"Exit"},
    {ACT_TYPE_ALL,"END"},
};
typedef enum _PROTOCOL
{
#ifdef HOST_NVME
    PROT_NVME = 1,
    PROT_ADMIN,
#else
    PROT_NONDATA = 0,
    PROT_PIO_IN,
    PROT_PIO_OUT,
    PROT_DMA_IN,
    PROT_DMA_OUT,
    PROT_NCQ_IN,
    PROT_NCQ_OUT,
#endif
}PROTOCOL;

typedef struct _PROTOCAL_DESC {
    char* Protocal;
    PROTOCOL SataProtocal;
}PROTOCAL_DESC;

static PROTOCAL_DESC PROTOCAL_LIB[] = 
{
#ifdef HOST_NVME
    {"NVM_CMD",PROT_NVME},
    {"ADMIN_CMD",PROT_ADMIN},
#else
    {"NON_DATA",PROT_NONDATA},
    {"PIO_IN"  ,PROT_PIO_IN},
    {"PIO_OUT" ,PROT_PIO_OUT},
    {"DMA_IN"  ,PROT_DMA_IN},
    {"DMA_OUT" ,PROT_DMA_OUT},   
    {"NCQ_IN"  ,PROT_NCQ_IN},
    {"NCQ_OUT" ,PROT_NCQ_OUT},
#endif
};



//////////////////////////////////////////////////////////////////
extern U32 g_InjErrHead;
extern U32 g_InjErrTail;
extern HANDLE hFileNfcErr;
extern HANDLE hFileInjErr;
HANDLE OpenInjErrFile(char* fileName);
void RecInjErrFile(HANDLE hFile,char* fmt,...);
//////////////////////////////////////////////////////////////////
void init_checklist();
void load_checklist();


BOOL set_bp_param(char* BP_Desc,U16* pBPNum,FUNCTION_PARAMETER* pBpParam);
BOOL get_actiontype(char* action,U8* ActType);
BOOL get_pagetype(char* pagetype,U8* PgType);
BOOL get_optype(char* optype,U8* OpType);
BOOL get_errtype(char* errtype,U8* ErrType);

BOOL parse_injecterror_param(ACTION* pAct,char* param);

void parse_action(COND_LIST* tmp_condlist,char* action,char* param);
void parse_hostcond_action(COND_LIST* tmp_condlist,char* cond,char* action);
void parse_flashcond_action(COND_LIST* tmp_condlist,char* cond,char* action,char* param);

SECTION* get_section(char* section);
void add_section(char* section);
BOOL remove_section(char* sec);

COND_LIST* get_condlist(SECTION* sec,char* key);
void add_condlist(char* sec,char* key,char* value);
BOOL remove_condlist(SECTION* sec);
BOOL remove_action(COND_LIST* cond);
BOOL delete_condlist(SECTION* sec,char* CondName,COND_LIST** pre_cond);

ACTION* get_action(COND_LIST* cond_list,U8 ActType);
ACTION* new_action(COND_LIST* tmp_condlist);
void add_action(COND_LIST* tmp_condlist,ACTION* paction);

HSCMD_INFO* lookup_hscmdlist(U32 hscmd_id);
U32 get_hscmd_id(char* Buffer);

void clear_checklist();
void inject_error_init();
BOOL load_one_checklist();


//////////////////////////////////////////////////////////////////
void module_run_init();
BOOL load_one_section();
BOOL run_one_checklist();

#endif


