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
* File Name    : action_define.h
* Discription  :
* CreateAuthor :
* CreateDate   : 2014.7.3
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _ACTION_DEFINE_H
#define _ACTION_DEFINE_H
#include "model_config.h"

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/

typedef enum _TABLE_REBUILD_TYPE
{
    TABLE_REBUILD_NONE = 0,
    TABLE_REBUILD_WAIT_L3_EMPTY,
    TABLE_REBUILD_NOT_WAIT_L3_EMPTY,

}TABLE_REBUILD_TYPE;

typedef struct
{
    U32 m_GlobalSN;
    UINT64 m_HostWriteSecCnt;
    UINT64 m_HostWritePgCnt;
    UINT64 m_DeviceWritePgCnt;
    UINT64 m_DevWTablePgCnt;
    UINT64 m_DevWDataPgCnt;
    UINT64 m_DevRTablePgCnt;
    UINT64 m_DevRDataPgCnt;

}StatisticDptr;



extern U8 g_TableRebuildType;
extern BOOL g_bInjectError;
extern U32 g_ulDevVarTableAddr;
extern U32 g_ulDevFlashRedAddr;
extern U32 g_ulDevFlashDataAddr[MCU_MAX - 1][32];

U8* SendReadMemoryCmd(U32 ulReadDevAddr, U16 usLength, U8 ucMCUID);
BOOL Sim_NormalBootSchdedule(void);
BOOL Sim_AbnormalBoot_WaitL3Empty();
BOOL Sim_AbnormalBoot_NotWaitL3Empty();
BOOL Sim_Redo_LLF();
BOOL Sim_SecurityErase();
BOOL Sim_SystemIdle(void);
BOOL Sim_Send_HSCMD(U32 HSCmdId);
BOOL Sim_LoadTraceLog(void);
BOOL Sim_FlashHandle(void);
BOOL Sim_LoadTraceFromFlash(void);
BOOL Sim_GetVarTableData();
BOOL Sim_FirmwareUpdate();
BOOL Sim_FirmwareImageDL();
BOOL Sim_WearLevelingStatus();
BOOL Sim_SecurityStatusCheck(U8 ucSecurityStatus);
BOOL Sim_NVMeWriteZero(void);

void DBG_ResetCounter();
void Dbg_InitStatistic();
void Dbg_IncGlobalSN(int Cnt);
void Dbg_IncHostWriteCnt(int SecCnt);
void Dbg_IncDevWriteCnt(int PageCnt);
void Dbg_ReportStatistic();
void Dbg_IncDevDataReadCnt(int PageCnt);
void Dbg_IncDevTableReadCnt(int PageCnt);
void Dbg_IncDevDataWriteCnt(int PageCnt);
void Dbg_IncDevTableWriteCnt(int PageCnt);

BOOL Sim_SendStandby(void);

#endif
/*====================End of this head file===================================*/