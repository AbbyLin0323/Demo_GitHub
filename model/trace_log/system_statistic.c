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

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "model_common.h"
#include "HAL_GLBReg.h"
#include "HAL_HostInterface.h"
#include "HAL_TraceLog.h"
#ifdef SIM_XTENSA
#include "iss/mp.h"
#include "xtmp_config.h"
#include "xtmp_options.h"
#include "xtmp_sysmem.h"
#include "xtmp_common.h"
#endif

#include "system_statistic.h"

#ifdef SIM
U32 g_ss_writelines = 0;
U32 g_ss_filenamecount = 0;
char g_ss_filename[256];

HANDLE  g_ss_hMutex;
HANDLE  g_ss_logfile;
unsigned char g_ss_logbuffer[TRACER_LOG_BUFFER_MAX];
extern char* SIM_GetLogFileFloder();

void SystemStatisticInit(void)
{
    char *pStrLogFolder = SIM_GetLogFileFloder();
    g_ss_hMutex = CreateMutex (NULL, FALSE, NULL);

    g_ss_writelines = 0;
    g_ss_filenamecount = 0;

    sprintf(g_ss_filename, "%s\\LOG_TRACE_%d.txt", pStrLogFolder, g_ss_filenamecount);
    g_ss_logfile =  CreateFile(g_ss_filename,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    return;
}

void SystemStatisticRecord(char *format,...)
{

    unsigned int haswrite;
    unsigned char *pLogBuffer = (unsigned char *)&g_ss_logbuffer;
    char *pStrLogFolder;
    HANDLE hLogFile;
    va_list ap;

    WaitForSingleObject (g_ss_hMutex, INFINITE);

    va_start(ap,format);

    vsprintf_s(pLogBuffer, 512, format,ap);

    g_ss_writelines++;

    if (g_ss_writelines >= 1600000)
    {
      g_ss_writelines = 0;

      CloseHandle(g_ss_logfile);

      pStrLogFolder = SIM_GetLogFileFloder();

      if (g_ss_filenamecount > 3)
      {
          sprintf(g_ss_filename, "%s\\LOG_TRACE_%d.txt", pStrLogFolder, (g_ss_filenamecount - 4));
          DeleteFile(g_ss_filename);
      }

      g_ss_filenamecount++;
      sprintf(g_ss_filename, "%s\\LOG_TRACE_%d.txt", pStrLogFolder, g_ss_filenamecount);
      g_ss_logfile =  CreateFile(g_ss_filename,
          GENERIC_READ|GENERIC_WRITE,
          FILE_SHARE_READ|FILE_SHARE_WRITE,
          NULL,
          CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
          NULL);
    }

    hLogFile = g_ss_logfile;
    WriteFile(hLogFile,(LPVOID)(pLogBuffer), (DWORD)strlen(pLogBuffer), &haswrite,  NULL);

    va_end(ap);
    ReleaseMutex(g_ss_hMutex);
    return;
}
#endif


#ifdef SIM_XTENSA
extern XTMP_core core;

static U32 gCycles = 0;
static U32 gStackPointer = 0xffffffff;

trace_module_list tracemodule_list[TRACER_MODULE_MAX];

////unsigned char logbuffer[][1024];
BOOL bFirmwareBootOk=FALSE;
FW_TRACE_RECORD l_tFwtTraceRecord[TRACER_SUBMODULE_MAX];
HOST_TIME_RECORD l_tHostTimeRecord;
U32 ulMCU2CeFifoBaseAddr;

void SystemStatisticHostTimeRecordIniti(void)
{
    memset((void *)&l_tHostTimeRecord, 0 , sizeof(HOST_TIME_RECORD));
    return;
}

void SystemStatisticHostGetPerformance(U16 usCurSendSecCnt)
{
    int ulTotalByte;
    int ulCosumeUs;
    U32 ulTempCycleCnt;
    U32 ulBandWith;
    l_tHostTimeRecord.ulCurTime = (U32)GET_TIME();
    l_tHostTimeRecord.ulSendSecSum += usCurSendSecCnt;

    if(0 == l_tHostTimeRecord.ulLastTime)
    {
        l_tHostTimeRecord.ulLastTime = l_tHostTimeRecord.ulCurTime;
        return;
    }
    ulTempCycleCnt = SystemCaculateTimePass(l_tHostTimeRecord.ulLastTime, l_tHostTimeRecord.ulCurTime);
    l_tHostTimeRecord.ulCycleSum += ulTempCycleCnt;
    if(l_tHostTimeRecord.ulCycleSum >= 0x1000000)
    {
        ulTotalByte = l_tHostTimeRecord.ulSendSecSum * SEC_SIZE;
        ulCosumeUs = l_tHostTimeRecord.ulCycleSum/HCLK_CYCLE_COUNT_PER_US;
        ulBandWith = (U32)((ulTotalByte/ulCosumeUs) * 1E6);

        printf("Host Send SecCnt %d need cycle %d; bandwith is %dBps \n", l_tHostTimeRecord.ulSendSecSum, l_tHostTimeRecord.ulCycleSum, ulBandWith);

        l_tHostTimeRecord.ulSendSecSum = 0;
        l_tHostTimeRecord.ulCycleSum = 0;
    }
    l_tHostTimeRecord.ulLastTime = l_tHostTimeRecord.ulCurTime;
    return;
}

void SystemStatisticRecord(unsigned int log_fun,unsigned int module_id,unsigned int submodule_id,unsigned int debug_level,char *format,...)
{
    unsigned int haswrite;
    unsigned char *pLogBuffer = &tracemodule_list[module_id].logbuffer[submodule_id][0];
    HANDLE hLogFile;
    va_list ap;

    va_start(ap,format);

    vsprintf_s((char *)pLogBuffer, 512, format,ap);// as TRACER_LOG_BUFFER_MAX == 512

    if (tracemodule_list[module_id].module_debug_enable)
    {
        if (tracemodule_list[module_id].submodule_debug_enable[submodule_id])
        {
            if (debug_level >= tracemodule_list[module_id].submodule_debug_level[submodule_id])
            {
                if ((LOG_FILE & log_fun) != 0)
                {
                    hLogFile = tracemodule_list[module_id].submodule_log_file[submodule_id];
                    WriteFile(hLogFile,(LPVOID)(pLogBuffer), (DWORD)strlen(pLogBuffer), &haswrite,  NULL);
                }
                if ((LOG_PRINT & log_fun) != 0)
                {
                    printf("%s", pLogBuffer);
                }
            }
        }
    }

    va_end(ap);
    return;
}

void SystemStatisticClose()
{
    unsigned int module_id;
    unsigned int submodule_id;

    for (module_id = 0;module_id < TRACER_MODULE_MAX;module_id++)
    {
        for (submodule_id = 0;submodule_id < TRACER_SUBMODULE_MAX;submodule_id++)
        {
            if(tracemodule_list[module_id].submodule_log_file[submodule_id] != INVALID_HANDLE_VALUE)
            {
                CloseHandle(tracemodule_list[module_id].submodule_log_file[submodule_id]);
            }
        }
    }
}

void SystemFwTraceRecordIniti(void)
{
    memset((void *)&l_tFwtTraceRecord, 0, sizeof(FW_TRACE_RECORD) * TRACER_SUBMODULE_MAX);
    return;
}

void SystemStatisticInit()
{
    unsigned int module_id;
    unsigned int submodule_id;

    SystemStatisticHostTimeRecordIniti();
    SystemFwTraceRecordIniti();

    for (module_id = 0;module_id < TRACER_MODULE_MAX;module_id++)
    {
        for (submodule_id = 0;submodule_id < TRACER_SUBMODULE_MAX;submodule_id++)
        {
            tracemodule_list[module_id].module_debug_enable = 0;
            tracemodule_list[module_id].submodule_debug_level[submodule_id] = 0;
            tracemodule_list[module_id].submodule_debug_enable[submodule_id] = 0;
            tracemodule_list[module_id].submodule_log_file[submodule_id] = INVALID_HANDLE_VALUE;
        }
    }

    module_id = 0;
    submodule_id = 0;

    //module 0
    module_id = 0;
    strcpy((char *)tracemodule_list[module_id].module_name,TRACE_MODULE0_NAME);
    tracemodule_list[module_id].module_debug_enable = TRACE_MODULE0_ENABLE;

    //module 0 submodule 0
    submodule_id = 0;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE0_SUBMODULE0_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE0_SUBMODULE0_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE0_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }

    //module 1
    module_id = 1;
    strcpy((char *)tracemodule_list[module_id].module_name,TRACE_MODULE1_NAME);
    tracemodule_list[module_id].module_debug_enable = TRACE_MODULE1_ENABLE;

    //module 1 submodule 0
    submodule_id = 0;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE1_SUBMODULE0_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE1_SUBMODULE0_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE1_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }

    //module 2
    module_id = 2;
    strcpy((char *)tracemodule_list[module_id].module_name,TRACE_MODULE2_NAME);
    tracemodule_list[module_id].module_debug_enable = TRACE_MODULE2_ENABLE;

    //module 2 submodule 0
    submodule_id = 0;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE2_SUBMODULE0_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE2_SUBMODULE0_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE2_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }

    //module 2 submodule 1
    submodule_id = 1;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE2_SUBMODULE1_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE2_SUBMODULE1_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE2_SUBMODULE1_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }

    //module 3
    module_id = 3;
    strcpy((char *)tracemodule_list[module_id].module_name,TRACE_MODULE3_NAME);
    tracemodule_list[module_id].module_debug_enable = TRACE_MODULE3_ENABLE;


    //module 3 submodule 0
    submodule_id = TRACE_FIRMWARE_SUBMODULE_HAL_ID;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE3_SUBMODULE0_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE3_SUBMODULE0_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE3_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }

    //module 3 submodule 1
    submodule_id = TRACE_FIRMWARE_SUBMODULE_L0_ID;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE3_SUBMODULE1_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE3_SUBMODULE1_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE3_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] = tracemodule_list[module_id].submodule_log_file[0];


    //module 3 submodule 1
    submodule_id = TRACE_FIRMWARE_SUBMODULE_L1_ID;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE3_SUBMODULE2_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE3_SUBMODULE2_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE3_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] = tracemodule_list[module_id].submodule_log_file[0];

    //module 3 submodule 2
    submodule_id = TRACE_FIRMWARE_SUBMODULE_L2_ID;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE3_SUBMODULE3_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE3_SUBMODULE3_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE3_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] = tracemodule_list[module_id].submodule_log_file[0];

    //module 3 submodule 3
    submodule_id = TRACE_FIRMWARE_SUBMODULE_L3_ID;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE3_SUBMODULE4_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE3_SUBMODULE4_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE3_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] = tracemodule_list[module_id].submodule_log_file[0];

    //module 3 submodule 4
    submodule_id = TRACE_FIRMWARE_HOST_CMD_SUBMODULE_ID;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE3_SUBMODULE5_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE3_SUBMODULE5_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE3_SUBMODULE5_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }

    //module 4
    module_id = 4;
    strcpy((char *)tracemodule_list[module_id].module_name,TRACE_MODULE4_NAME);
    tracemodule_list[module_id].module_debug_enable = TRACE_MODULE4_ENABLE;

    //module 4 submodule 0
    submodule_id = 0;
    tracemodule_list[module_id].submodule_debug_enable[submodule_id] = TRACE_MODULE4_SUBMODULE0_ENABLE;
    tracemodule_list[module_id].submodule_debug_level[submodule_id] = TRACE_MODULE4_SUBMODULE0_LEVEL;
    strcpy(tracemodule_list[module_id].submodule_name[submodule_id],TRACE_MODULE4_SUBMODULE0_FILENAME);

    tracemodule_list[module_id].submodule_log_file[submodule_id] =  CreateFile((LPCSTR)tracemodule_list[module_id].submodule_name[submodule_id],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(tracemodule_list[module_id].submodule_log_file[submodule_id] == INVALID_HANDLE_VALUE)
    {
        printf("%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Break();
    }
}
void SystemStatisticTicker(void *threadData){
    u32 pc;
    u32 counter;
    u32 tempAR1;
    u32 nLockValue;
    XTMP_register reg;
    XTMP_core core;

    core = *(XTMP_core*)threadData;


    while(1)
    {
        pc = XTMP_getStagePC(core,2);
        counter = (u32)GET_TIME();

        reg = XTMP_getRegisterByName(core,"AR1");

        regRead(0x10100, 4, (u8 *)(&nLockValue));

        if (nLockValue != 0)
        {
            printf("hit at %x\n",pc);
        }

        //XTMP_getAllRegisters(core,&pRegFile,&RegFileNum);
        if(ugCmdCount != 0)
        {
            XTMP_copyRegisterValue(reg,&tempAR1);
            if(tempAR1 < gStackPointer)
            {
                //SystemStatisticRecord(TRACE_STACK_MODULE_ID,TRACE_STACK_SUBMODULE_ID,0,"last AR1 %x %d\n",gStackPointer,0x20000-gStackPointer);
            }
        }

        if((pc != 0xffffffff))
            //SystemStatisticRecord(TRACE_PCTRACER_MODULE_ID,TRACE_PCTRACER_SUBMODULE_ID,0,"%08x\t%08x\n",counter,pc);

                XTMP_wait(SYSTEM_STATISTIC_TICKER_INT);
    }
}

void SystemStartTimer(TRACE_TIMER *pTimer)
{
    DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);

    QueryPerformanceCounter(&(pTimer->nStart));

    SetThreadAffinityMask(GetCurrentThread(), oldmask);
}

void SystemStopTimer(TRACE_TIMER *pTimer)
{
    DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);

    QueryPerformanceFrequency(&(pTimer->nFrequency));

    QueryPerformanceCounter(&(pTimer->nStop));

    SetThreadAffinityMask(GetCurrentThread(), oldmask);


}
DOUBLE SystemGetTime(TRACE_TIMER *pTimer)
{
    //
    DOUBLE result = (DOUBLE)(((pTimer->nStop.QuadPart - pTimer->nStart.QuadPart)*1000000) / pTimer->nFrequency.QuadPart);
    return result;
    //return ((pTimer->nStop.QuadPart - pTimer->nStart.QuadPart));
}

void SystemInitTimer(TRACE_TIMER *pTimer)
{
    pTimer->nStart.QuadPart = 0;
    pTimer->nStop.QuadPart = 0;
    pTimer->nFrequency.QuadPart = 0;

    pTimer->GetTime = SystemGetTime;
    pTimer->StartTimer = SystemStartTimer;
    pTimer->StopTimer = SystemStopTimer;
}

U32 SystemCaculateTimePass(U32 ulStartTime, U32 ulEndTime)
{
    U32 ulTimeTemp;
    if (ulEndTime < ulStartTime)
    {
        ulTimeTemp = (INVALID_8F - ulStartTime) + ulEndTime + 1;
    }
    else
    {
        ulTimeTemp = ulEndTime - ulStartTime;
    }
    return ulTimeTemp;
}

void SystemFwTracerRecord(U8 ucSubModelId, U32 ulStagePoint)
{
    U32 ulCurrentTime;
    ulCurrentTime =  (U32)GET_TIME();
    if(0 != ulStagePoint)
    {
        l_tFwtTraceRecord[ucSubModelId].ulStageTimeCnt [ulStagePoint - 1] = SystemCaculateTimePass(l_tFwtTraceRecord[ucSubModelId].ulLastPCTriggerTime, ulCurrentTime);

        //save performance data to file
        SystemStatisticRecord(LOG_FILE, TRACE_FIRMWARE_MODULE_ID, ucSubModelId, 1, "FW L%d stage%d to stage%d need time %d\n",
            ucSubModelId-1, l_tFwtTraceRecord[ucSubModelId].ucLastStagePoint, ulStagePoint, l_tFwtTraceRecord[ucSubModelId].ulStageTimeCnt[ulStagePoint - 1]);

        l_tFwtTraceRecord[ucSubModelId].ulAllStageTimeCnt += l_tFwtTraceRecord[ucSubModelId].ulStageTimeCnt[ulStagePoint - 1];
        if (FW_PERFORMANCE_STAGE_NUM_MAX == ulStagePoint)
        {
            //
            SystemStatisticRecord(LOG_FILE, TRACE_FIRMWARE_MODULE_ID, ucSubModelId, 1, "FW L%d all stage need time %d\n",
                ucSubModelId-1, l_tFwtTraceRecord[ucSubModelId].ulAllStageTimeCnt);
            l_tFwtTraceRecord[ucSubModelId].ulAllStageTimeCnt = 0;
        }
    }

    l_tFwtTraceRecord[ucSubModelId].ulLastPCTriggerTime = ulCurrentTime;
    l_tFwtTraceRecord[ucSubModelId].ucLastStagePoint = ulStagePoint;
    return;
}

BOOL SystemPerformanceRecord(U32 addr, U32 regvalue, U32 len)
{
    U32 funflag;
    U32 ulWholeScheduleTime = 0;
#if 0
    funflag = regvalue;

    if (addr == (U32)&rTraceData6)
    {
        if (TRACER_FW_BOOT_UP_VALUE == funflag)
        {
            bFirmwareBootOk = TRUE;
        }
    }

    if (FALSE == bFirmwareBootOk)
    {
        return FALSE;
    }

    //HAL Statistic
    if (addr == (U32)&rTraceData0)
    {
        SystemFwTracerRecord(TRACE_FIRMWARE_SUBMODULE_HAL_ID, funflag - HAL_TRACER_STAGE_VALUE_START);

        if (HAL_TRACER_STAGE_VALUE_END == funflag)
        {
            memset((void *)l_tFwtTraceRecord[TRACE_FIRMWARE_SUBMODULE_HAL_ID].ulStageTimeCnt, INVALID_8F , sizeof(U32)*FW_PERFORMANCE_STAGE_NUM_MAX);
        }
    }

    //L0 Statistic
    if (addr == (U32)&rTraceData1)
    {
        SystemFwTracerRecord(TRACE_FIRMWARE_SUBMODULE_L0_ID, funflag - L0_TRACER_STAGE_VALUE_START);

        if (L0_TRACER_STAGE_VALUE_END == funflag)
        {
            memset((void *)l_tFwtTraceRecord[TRACE_FIRMWARE_SUBMODULE_L0_ID].ulStageTimeCnt, INVALID_8F , sizeof(U32)*FW_PERFORMANCE_STAGE_NUM_MAX);
        }
    }

    //L1 Statistic
    if (addr == (U32)&rTraceData2)
    {
        SystemFwTracerRecord(TRACE_FIRMWARE_SUBMODULE_L1_ID, funflag - L1_TRACER_STAGE_VALUE_START);

        if (L1_TRACER_STAGE_VALUE_END == funflag)
        {
            memset((void *)l_tFwtTraceRecord[TRACE_FIRMWARE_SUBMODULE_L1_ID].ulStageTimeCnt, INVALID_8F , sizeof(U32)*FW_PERFORMANCE_STAGE_NUM_MAX);
        }
    }

    //L2 statistic
    if (addr == (U32)&rTraceData3)
    {
        SystemFwTracerRecord(TRACE_FIRMWARE_SUBMODULE_L2_ID, funflag - L2_TRACER_STAGE_VALUE_START);

        if (L2_TRACER_STAGE_VALUE_END == funflag)
        {
            memset((void *)l_tFwtTraceRecord[TRACE_FIRMWARE_SUBMODULE_L2_ID].ulStageTimeCnt, INVALID_8F , sizeof(U32)*FW_PERFORMANCE_STAGE_NUM_MAX);
        }
    }

    //L3 statistic
    if (addr == (U32)&rTraceData4)
    {
        SystemFwTracerRecord(TRACE_FIRMWARE_SUBMODULE_L3_ID, funflag - L3_TRACER_STAGE_VALUE_START);

        if (L3_TRACER_STAGE_VALUE_END == funflag)
        {
            memset((void *)l_tFwtTraceRecord[TRACE_FIRMWARE_SUBMODULE_L3_ID].ulStageTimeCnt, INVALID_8F , sizeof(U32)*FW_PERFORMANCE_STAGE_NUM_MAX);
        }
    }
#endif
    return FALSE;
}
#endif
