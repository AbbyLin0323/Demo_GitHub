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
#ifndef _SYSTEM_STATISTIC_H_INCLUDED
#define _SYSTEM_STATISTIC_H_INCLUDED

#include "BaseDef.h"
#ifdef SIM_XTENSA
#include "iss/mp.h"
#include "xtmp_common.h"
#endif
#include <windows.h>
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#ifdef SIM_XTENSA
#define GET_TIME() XTMP_clockTime()
#endif


#define LOG_FILE 0x1
#define LOG_PRINT 0x2
#define LOG_ALL   0x3

#define STACK_TRACER
#define FLASH_TRACER
#define SATA_TRACER

//#define SIM_TIME
#define TRACER_LOG_TIMER_LEVEL  5

#define  TRACE_MODULE0_NAME   "SATA_HOST"
#define  TRACE_MODULE0_ENABLE 1
#define  TRACE_MODULE0_SUBMODULE0_FILENAME "SATA_HOST_CMD_RESULT.TXT"
#define  TRACE_MODULE0_SUBMODULE0_ENABLE 1
#ifdef SIM_TIME
#define  TRACE_MODULE0_SUBMODULE0_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE0_SUBMODULE0_LEVEL 0
#endif

#define  TRACE_MODULE1_NAME   "SATA_DEVICE"
#define  TRACE_MODULE1_ENABLE 1
#define  TRACE_MODULE1_SUBMODULE0_FILENAME "SATA_DEVICE.TXT"
#define  TRACE_MODULE1_SUBMODULE0_ENABLE 0

#ifdef SIM_TIME
#define  TRACE_MODULE1_SUBMODULE0_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE1_SUBMODULE0_LEVEL 0
#endif

#define  TRACE_MODULE2_NAME   "FLASH_DEVICE"
#define  TRACE_MODULE2_ENABLE 1
#define  TRACE_MODULE2_SUBMODULE0_FILENAME "FLASH_TRIGGER.TXT"
#define  TRACE_MODULE2_SUBMODULE0_ENABLE 0
#ifdef SIM_TIME
#define  TRACE_MODULE2_SUBMODULE0_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE2_SUBMODULE0_LEVEL 0
#endif

#define  TRACE_MODULE2_SUBMODULE1_FILENAME "FLASH_RESULT.TXT"
#define  TRACE_MODULE2_SUBMODULE1_ENABLE 0
#ifdef SIM_TIME
#define  TRACE_MODULE2_SUBMODULE1_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE2_SUBMODULE1_LEVEL 0
#endif

#define  TRACE_MODULE4_NAME   "FLASH_STORAGE"
#define  TRACE_MODULE4_ENABLE 1
#define  TRACE_MODULE4_SUBMODULE0_FILENAME "FLASH_STORAGE.TXT"
#define  TRACE_MODULE4_SUBMODULE0_ENABLE 0

#ifdef SIM_TIME
#define  TRACE_MODULE4_SUBMODULE0_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define TRACE_MODULE4_SUBMODULE0_LEVEL 0
#endif


#define  TRACE_MODULE3_NAME   "FIRMWARE"
#define  TRACE_MODULE3_ENABLE 1
#define  TRACE_MODULE3_SUBMODULE0_FILENAME "FIRMWARE_DEBUG.TXT"
#define  TRACE_MODULE3_SUBMODULE0_ENABLE 1
#ifdef SIM_TIME
#define  TRACE_MODULE3_SUBMODULE0_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE3_SUBMODULE0_LEVEL 0
#endif
#define  TRACE_MODULE3_SUBMODULE1_ENABLE 1
#ifdef SIM_TIME
#define  TRACE_MODULE3_SUBMODULE1_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE3_SUBMODULE1_LEVEL 0
#endif

#define  TRACE_MODULE3_SUBMODULE2_ENABLE 1
#ifdef SIM_TIME
#define  TRACE_MODULE3_SUBMODULE2_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE3_SUBMODULE2_LEVEL 0
#endif

#define  TRACE_MODULE3_SUBMODULE3_ENABLE 1
#ifdef SIM_TIME
#define  TRACE_MODULE3_SUBMODULE3_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE3_SUBMODULE3_LEVEL 0
#endif

#define  TRACE_MODULE3_SUBMODULE4_ENABLE 1
#ifdef SIM_TIME
#define  TRACE_MODULE3_SUBMODULE4_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE3_SUBMODULE4_LEVEL 0
#endif

#define  TRACE_MODULE3_SUBMODULE5_FILENAME "FIRMWARE_HOSTCMD.TXT"
#define  TRACE_MODULE3_SUBMODULE5_ENABLE 0
#ifdef SIM_TIME
#define  TRACE_MODULE3_SUBMODULE4_LEVEL TRACER_LOG_TIMER_LEVEL
#else
#define  TRACE_MODULE3_SUBMODULE5_LEVEL 0
#endif

#define TRACE_SATAHOST_MODULE_ID 0
#define TRACE_SATAHOST_SUBMODULE_ID 0

#define TRACE_SATADEVICE_MODULE_ID 1
#define TRACE_SATADEVICE_SUBMODULE_ID 0

#define TRACE_FLASHDEVICE_TRIGGER_MODULE_ID 2
#define TRACE_FLASHDEVICE_TRIGGER_SUBMODULE_ID 0

#define TRACE_FLASHDEVICE_RESULT_MODULE_ID 2
#define TRACE_FLASHDEVICE_RESULT_SUBMODULE_ID 1

#define TRACE_FLASHSTORAGE_RESULT_MODULE_ID 4
#define TRACE_FLASHSTORAGE_RESULT_SUBMODULE_ID 0

#define TRACE_FIRMWARE_MODULE_ID 3
#define TRACE_FIRMWARE_SUBMODULE_HAL_ID 0
#define TRACE_FIRMWARE_SUBMODULE_L0_ID 1
#define TRACE_FIRMWARE_SUBMODULE_L1_ID 2
#define TRACE_FIRMWARE_SUBMODULE_L2_ID 3
#define TRACE_FIRMWARE_SUBMODULE_L3_ID 4

#define TRACE_FIRMWARE_HOST_CMD_MODULE_ID 3
#define TRACE_FIRMWARE_HOST_CMD_SUBMODULE_ID 5

#define SYSTEM_STATISTIC_TICKER_INT 0x100

#define TRACER_MODULE_MAX  16
#define TRACER_SUBMODULE_MAX 16
#define TRACER_MODULE_LOG_FILE_NAME_MAX 128
#define TRACER_LOG_BUFFER_MAX    512

#define FW_PERFORMANCE_STAGE_NUM_MAX 64
#define IGNORE_PERFORMANCE

//#ifndef IGNORE_PERFORMANCE
#define HCLK_200M_TOGGLE_100M
//#define HCLK_300M_TOGGLE_75M

/////time param begin
#ifdef HCLK_200M_TOGGLE_100M
//#define FLASH_CONFIG_TIMER_INTERVAL    5
#define HCLK_CYCLE_COUNT_PER_US        200
#define BUSY_TIME_BUS_TRANSFER         ((23.1*HCLK_CYCLE_COUNT_PER_US)*(1<<(LOGIC_PIPE_PG_SZ_BITS - NF_4K_BITS))) //  32us per 4K.   6006

#else ifdef HCLK_300M_TOGGLE_75M
//#define FLASH_CONFIG_TIMER_INTERVAL    5
#define HCLK_CYCLE_COUNT_PER_US         300
#define BUSY_TIME_BUS_TRANSFER          ((31*HCLK_CYCLE_COUNT_PER_US)*(1<<(LOGIC_PIPE_PG_SZ_BITS - NF_4K_BITS))) //  32us per 4K.
#endif

#define BUSY_TIME_FAST_PAGE_READ        (44*HCLK_CYCLE_COUNT_PER_US)
#define BUSY_TIME_SLOW_PAGE_READ        (61*HCLK_CYCLE_COUNT_PER_US)
#define BUSY_TIME_FLASH_READ            ((BUSY_TIME_FAST_PAGE_READ + BUSY_TIME_SLOW_PAGE_READ)/2 )   //50us
#define BUSY_TIME_FAST_PAGE_WRITE       (438*HCLK_CYCLE_COUNT_PER_US)//390390//180180   //600
#define BUSY_TIME_SLOW_PAGE_WRITE       (1965*HCLK_CYCLE_COUNT_PER_US) //390390//600600   //2000
#define BUSY_TIME_FLASH_WRITE           ((BUSY_TIME_FAST_PAGE_WRITE + BUSY_TIME_SLOW_PAGE_WRITE)/2)  //(1300-FLASH_CONFIG_TIMER_INTERVAL)
#define BUSY_TIME_FLASH_ERASE           3000
#define PCIE_VALID_BANDWIDTH            (80)//pcie valid bandwith: %80
#define BUSY_TIME_OTFB_TO_HOST          ((32*HCLK_CYCLE_COUNT_PER_US)*100/PCIE_VALID_BANDWIDTH)//pcie bandwith:8Gb/s  time consume per buf

#define PCIE_DEV_TO_HOST_EFFICIENT             (80)//pcie valid bandwith: %80
#define BUSY_TIME_DEV_TO_HOST_PER_BUF          ((32*HCLK_CYCLE_COUNT_PER_US)*100/PCIE_DEV_TO_HOST_EFFICIENT)//pcie bandwith:8Gb/s  time consume per buf
#define BUSY_TIME_DEV_TO_HOST_PER_BYTE         (BUSY_TIME_DEV_TO_HOST_PER_BUF/LOGIC_PIPE_PG_SZ)
#define PCIE_HOST_TO_DEV_EFFICIENT             (80)//pcie valid bandwith: %80
#define BUSY_TIME_HOST_TO_DEV_PER_BUF          ((32*HCLK_CYCLE_COUNT_PER_US)*100/PCIE_HOST_TO_DEV_EFFICIENT)//pcie bandwith:8Gb/s  time consume per buf
#define BUSY_TIME_HOST_TO_DEV_PER_BYTE         (BUSY_TIME_HOST_TO_DEV_PER_BUF/LOGIC_PIPE_PG_SZ)
/////time param end

typedef struct _ST_TIME_RECORD
{
    U32 time_busy;           //time need consume

    U32 time_start:8;        // time start point
    U32 rsv:24;
}ST_TIME_RECORD;

typedef enum _PCIE_BUS_STS
{
    PCIE_STS_FREE,
    PCIE_STS_BUSY
}PCIE_BUS_STS;
//#endif

typedef struct _trace_module_list
{
    unsigned char module_name[64];
    unsigned int module_id;
    unsigned int module_debug_enable;
    unsigned int submodule_debug_enable[TRACER_MODULE_MAX];
    unsigned int submodule_debug_level[TRACER_MODULE_MAX];
    HANDLE submodule_log_file[TRACER_SUBMODULE_MAX];
    char submodule_name[TRACER_SUBMODULE_MAX][TRACER_MODULE_LOG_FILE_NAME_MAX];
    unsigned char logbuffer[TRACER_SUBMODULE_MAX][TRACER_LOG_BUFFER_MAX];
}trace_module_list;

typedef struct _FW_TRACE_RECORD
{
    U8 ucLastStagePoint;
    U32 ulLastPCTriggerTime;
    U32 ulAllStageTimeCnt;
    U32 ulStageTimeCnt[FW_PERFORMANCE_STAGE_NUM_MAX];
}FW_TRACE_RECORD;

typedef struct _HOST_TIME_RECORD
{
     U32 ulCycleSum;
     U32 ulSendSecSum;
     U32 ulLastTime;
     U32 ulCurTime;
}HOST_TIME_RECORD;

trace_module_list  tracemodule_list[TRACER_MODULE_MAX];

typedef void (*pFuncTimer)(void *p);
typedef DOUBLE (*pGetTime) (void *p);
typedef struct _TRACE_TIMER
{
    LARGE_INTEGER nStart;
    LARGE_INTEGER nStop;
    LARGE_INTEGER nFrequency;
    pFuncTimer StartTimer;
    pFuncTimer StopTimer;
    pGetTime GetTime;

}TRACE_TIMER;

/*  usage:
      FIRMWARE_LogInfo("Get CacheLine = %d \n", CacheLine);
 */
#define FIRMWARE_LogInfo(format, ...) SystemStatisticRecord(format, __VA_ARGS__) //(void)0

#ifdef    __cplusplus
extern "C" {
#endif
    void SystemStatisticHostTimeRecordIniti(void);
    void SystemStatisticHostGetPerformance(U16 usCurSendSecCnt);
    U32 SystemCaculateTimePass(U32 ulStartTime, U32 ulEndTime);
    BOOL SystemPerformanceRecord(U32 addr, U32 regvalue, U32 len);
#ifdef SIM
    void SystemStatisticRecord(char *format,...);
#else
    void SystemStatisticRecord(unsigned int log_fun,unsigned int module_id,unsigned int submodule_id,unsigned int debug_level,char *format,...);
#endif
    void SystemStatisticClose();
    void SystemStatisticInit();
    //void SystemStatisticTicker(void *threadData);
    void SystemStartTimer(TRACE_TIMER *pTimer);
    void SystemStopTimer(TRACE_TIMER *pTimer);
    DOUBLE SystemGetTime(TRACE_TIMER *pTimer);
    void SystemInitTimer(TRACE_TIMER *pTimer);
#ifdef    __cplusplus
}
#endif

#endif
