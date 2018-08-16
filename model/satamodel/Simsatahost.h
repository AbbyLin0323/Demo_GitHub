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

#ifndef __SIM_HOST_H__
#define  __SIM_HOST_H__

#include <windows.h>
#include "trace_log/system_statistic.h"

//#define TRIM_ENTRY_FULL(Head,NextTail)  ((NextTail == Head)? 1:0)
//#define TRIM_ENTRY_EMPTY(Head,Tail)     ((Tail == Head)? 1:0)

struct SATA_CNT
{
    DWORD* dwCnt;
    DWORD dwLength;
};
extern struct SATA_CNT g_SC;

//typedef struct _LBA_RANGE_ENTRY
//{
//    U32 StartLbaLow:32;
//
//    U32 StartLbaHigh:16;
//    U32 RangeLength:16;
//
//}LBA_RANGE_ENTRY;

#define SIM_LBA_RANGE_ENTRY_MAX 64

//typedef struct TRIM_CMD_ENTRY_TAG
//{
//    LBA_RANGE_ENTRY LbaRangeEntry[SIM_LBA_RANGE_ENTRY_MAX];
//    U32 SecCnt;
//
//}TRIM_CMD_ENTRY;



//#define HOST_ResultLog(log_fun, log_level, x, ...) \
//    SystemStatisticRecord(log_fun, TRACE_SATAHOST_MODULE_ID,TRACE_SATAHOST_SUBMODULE_ID,log_level,x, __VA_ARGS__);
//

#ifdef SIM_DBG
#define HOST_LogInfo(x, ...) \
    SystemStatisticRecord(LOG_FILE, TRACE_SATAHOST_MODULE_ID,TRACE_SATAHOST_SUBMODULE_ID,1,x, __VA_ARGS__);
    //SystemStatisticRecord(log_fun, TRACE_SATAHOST_MODULE_ID,TRACE_SATAHOST_SUBMODULE_ID,log_level,x, __VA_ARGS__);
#else
#define HOST_LogInfo(log_fun, log_level, x, ...)  (void)0
#endif

BOOL Host_SendCmdToDevice(U8 cmd_code, U32 start_lba, U16 sector_cnt, U32 subcmd_info);
BOOL Host_SendNonDataCmd(U8 cmd_code);
void Host_NoneDataCMDFinish();
void Host_GetPerformance();

//Data Check Section
void Host_DataFileDel();
void Init_sata(DWORD dwHowManyArray);

#ifdef SIM
DWORD WINAPI Host_ModelThread(LPVOID p);
#endif

#ifdef SIM_XTENSA
void Host_ModelThread_XTENSA();
#endif

#endif