/****************************************************************************
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
*****************************************************************************
Filename    :L1_SpecialSCMD.h
Version     :Ver 1.0
Author      :BlakeZhang
Date        :2016.06.13
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef _L1_SPECIALSCMD_H
#define _L1_SPECIALSCMD_H

#include "L1_SCmdInterface.h"

typedef enum _L1_SCMD_LLF_STATUS_
{
    L1_SCMD_LLF_INIT = 0,
    L1_SCMD_LLF_WAIT_L1,
    L1_SCMD_LLF_SET_L3,
    L1_SCMD_LLF_WAIT_L3,
    L1_SCMD_LLF_SET_L2,
    L1_SCMD_LLF_WAIT_L2,
    L1_SCMD_LLF_FAIL,
    L1_SCMD_LLF_DONE
} L1_SCMD_LLF_STATUS;

typedef enum _L1_SCMD_BOOT_STATUS_
{
    L1_SCMD_BOOT_INIT = 0,
    L1_SCMD_BOOT_SET_L3,
    L1_SCMD_BOOT_WAIT_L3,
    L1_SCMD_BOOT_SET_L2,
    L1_SCMD_BOOT_WAIT_L2,
    L1_SCMD_BOOT_WAIT_L1,
    L1_SCMD_BOOT_INIT_REBUILD,
    L1_SCMD_BOOT_REBUILD_SET_L3,
    L1_SCMD_BOOT_REBUILD_WAIT_L3,
    L1_SCMD_BOOT_REBUILD_SET_L2,
    L1_SCMD_BOOT_REBUILD_WAIT_L2,
    L1_SCMD_BOOT_REBUILD_WAIT_L1,
    L1_SCMD_BOOT_FAIL,
    L1_SCMD_BOOT_DONE
} L1_SCMD_BOOT_STATUS;

typedef enum _L1_SCMD_SHUTDOWN_STATUS_
{
    L1_SCMD_SHUTDOWN_INIT = 0,
    L1_SCMD_SHUTDOWN_WAIT_L1,
    L1_SCMD_SHUTDOWN_SET_L2,
    L1_SCMD_SHUTDOWN_WAIT_L2,
    L1_SCMD_SHUTDOWN_SET_L3,
    L1_SCMD_SHUTDOWN_WAIT_L3,
    L1_SCMD_SHUTDOWN_SET_L2_SAVETABLE,
    L1_SCMD_SHUTDOWN_WAIT_L2_SAVETABLE,
    L1_SCMD_SHUTDOWN_FAIL,
    L1_SCMD_SHUTDOWN_DONE
} L1_SCMD_SHUTDOWN_STATUS;

typedef enum _L1_SCMD_HINFO_SAVE_STATUS_
{
    L1_SCMD_HINFO_SAVE_INIT = 0,
    L1_SCMD_HINFO_SAVE_SET_L2,
    L1_SCMD_HINFO_SAVE_WAIT_L2,
    L1_SCMD_HINFO_SAVE_FAIL,
    L1_SCMD_HINFO_SAVE_DONE
} L1_SCMD_HINFO_SAVE_STATUS;

typedef enum _L1_SCMD_TRIM_STATUS_
{
    L1_SCMD_TRIM_INIT = 0,
    L1_SCMD_TRIM_WAIT_IDLE,
    L1_SCMD_TRIM_PROCESS,
    L1_SCMD_TRIM_PROCESS_NON4K_ALIGNED_TRIM_RANGE,
    L1_SCMD_TRIM_PROCESS_4K_ALIGNED_TRIM_RANGE,
    L1_SCMD_TRIM_PROCESS_LCT_ALIGNED_TRIM_RANGE,
    L1_SCMD_TRIM_NOT_ALGINED,
    L1_SCMD_TRIM_FAIL,
    L1_SCMD_TRIM_DONE
} L1_SCMD_TRIM_STATUS;

typedef enum _L1_SCMD_IDLE_STATUS_
{
    L1_SCMD_IDLE_INIT = 0,
    L1_SCMD_IDLE_DO_L1,
    L1_SCMD_IDLE_WAIT_L1,
    L1_SCMD_IDLE_SET_L2,
    L1_SCMD_IDLE_WAIT_L2,
    L1_SCMD_IDLE_FAIL,
    L1_SCMD_IDLE_DONE
} L1_SCMD_IDLE_STATUS;

typedef enum _L1_SCMD_SELFTEST_STATUS_
{
    L1_SCMD_SELFTEST_INIT = 0,
    L1_SCMD_SELFTEST_DO_L1,
    L1_SCMD_SELFTEST_SET_L2,
    L1_SCMD_SELFTEST_WAIT_L2,
    L1_SCMD_SELFTEST_FAIL,
    L1_SCMD_SELFTEST_DONE
} L1_SCMD_SELFTEST_STATUS;

extern void L1_SpecialSCMDInit(void);
extern void L1_TaskSpecialSCMD(SCMD* pSCMD);
#ifndef LCT_VALID_REMOVED
extern void L1_SetLCTValidLoadStatus(U8 ucStatus);
extern U8 L1_GetLCTValidLoadStatus(void);
#endif

#endif

