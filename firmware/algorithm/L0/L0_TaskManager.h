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
Filename     :  L0_TaskManager.h                                         
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  Yao Chen

Description: System-level background task and power management related
    macros/data structures definition for L0.

Modification History:
Gavin    20150119    rename to L0_TaskManager.h
*************************************************/
#ifndef _L0_TASK_MANAGER_H
#define _L0_TASK_MANAGER_H

typedef struct _L0_TASK_MANAGER
{
    U32  PrevPortIdle;
    U32  IdleLoopCount;
    
    U32  PMState;
    U32  PrevPMState;
}L0_TASK_MANAGER;

typedef enum _port_state
{
    PORTPM_ACTIVE = 0,
    PORTPM_ACTIVE_PREP_STG1,
    PORTPM_ACTIVE_PREP_STG2,
#ifdef PCIE_ASPM_MANAGEMENT
    PORTPM_ACTIVE_PREP_STG3,
    PORTPM_ACTIVE_ASPML1_ENABLED,
#endif
    PORTPM_IDLE,
    PORTPM_STANDBY,
    PORTPM_SLEEP_PREP,
    PORTPM_SLEEP_ENTRY,
    PORTPM_LIGHTSLEEP,
    PORTPM_SUSP_READY,
} PORT_STATE;

//define stage for Debug Mode switching
typedef enum _DEBUG_MODE_STATE
{
    DEBUG_MODE_STATE_ENTER = 0,
    DEBUG_MODE_STATE_NTFY_SUBSYS,
    DEBUG_MODE_STATE_WAIT_SUBSYS,
    DEBUG_MODE_STATE_RE_INIT,
    DEBUG_MODE_STATE_WAIT_INIT,
    DEBUG_MODE_STATE_FINISH
} DEBUG_MODE_STATE;

// PMU timer ticked under 25M internal clock mode. (1T = 40ns)
#ifdef XTMP
#define PORTPM_IDLE_LOOP_THRESHOLD 40//4000
#define IDLETASK_COUNT_MAX 10//1000
#define SYSPM_TIME_WAITING_SCHEDULE  1//100000  // 4ms
#else
#ifndef SIM
#define PORTPM_IDLE_LOOP_THRESHOLD 15000 // Approximate 40ms
#define PORTPM_ASPML1_LOOP_THRESHOLD 1200 // Approximate 3ms
#else
#define PORTPM_IDLE_LOOP_THRESHOLD 4000
#define PORTPM_SELFWAKE_LOOP_THRESHOLD 20000
#endif
#define IDLETASK_COUNT_MAX    5 //1000
#define SYSPM_TIME_WAITING_SCHEDULE  100000  // 4ms
#endif

#define PORTPM_IDLETASKSTATE_START 0
#define PORTPM_IDLETASKSTATE_EXEC 1
#define PORTPM_IDLETASKSTATE_STOP 2
#define PORTPM_IDLETASKSTATE_WAIT 3

#define PORTPM_IDLETASKSTATUS_PROCESSING 0
#define PORTPM_IDLETASKSTATUS_FINISHED 1
#define PORTPM_IDLETASKSTATUS_ABORTED 2

//#define PORTPM_IDLETASK_EXEC_PERIOD (1 << 20) // Approximate 40ms
#define PORTPM_IDLETASK_EXEC_PERIOD (50 * 1000u) // Approximate 50ms

#ifdef SIM
#define L0_TIMETASK_INTVL_TICKCOUNT (1 << 9) // Approximate 5s - In Windows one tick is about 10-16ms
#else
#define L0_TIMETASK_INTVL_TICKCOUNT (1 << 27) // Approximate 5s - To PMU timer one tick is 40ns
#endif

#define L0_TIMETASK_INTVL_SEC 5 // 5 seconds

#define SEC_PER_MIN 60
#define MIN_PER_HOUR 60

#define L0_TL_MEM_SIZE (32u * 1024u)
#define L0_MORE_TL_MEM_SIZE (64 * 1024u * 1024u)
#define HCMD_TIMEOUT_US_COUNT (5000u * 1000u)

#endif //_L0_TASK_MANAGER_H

