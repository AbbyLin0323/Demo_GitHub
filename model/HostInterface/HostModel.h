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

#ifndef _HOST_Model_H
#define _HOST_Model_H

#include <windows.h>
#include "BaseDef.h"
#include "system_statistic.h"
#include "HostInc.h"
#include "checklist_parse.h"

#define HOST_WRITE_CNT_MSK      (0x7FFFFFFF)
#define HOST_WRITE_CNT_HIGH_BIT (31)


extern CRITICAL_SECTION g_LbaWriteCntCriticalSection;

//Model process
void Host_FunctionMenu();
void Host_ModelInit();
void Host_ModelSchedule();
BOOL Host_ModelProcess();
void Host_CmdCompleted( U8 ucTag );


//Data Check Section
void Host_DataFileDel();
void Init_sata(DWORD dwHowManyArray);
void Host_OpenDataFile();
void Host_CloseDataFile();
BOOL Host_SaveDataToFile();
BOOL Host_GetDataFromFile();
U32 Host_UpdateDataCnt(U32 lba);
U32 Host_GetDataCnt(U32 lba);
U32 Host_SaveDataCnt(U32 lba, U32 cnt);
void Host_ResetLbaWriteCnt();
#ifdef DBG_TABLE_REBUILD
void Host_ClearDataCntHighBit(U32 lba,U32 ulWriteCnt);
U32 Host_GetDataCntHighBit(U32 lba);
U32 Host_GetDataCnt(U32 lba);
#endif

// CMD interface
BOOL Host_SendOneCmd(HCMD_INFO* pHcmd);
void Host_SetPDFlag();
void Host_ClearPDFlag();
BOOL Host_IsCMDEmpty();

void Host_SimClearHCmdQueue();


#endif