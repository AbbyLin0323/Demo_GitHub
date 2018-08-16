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
Filename    : TEST_NfcMCU1.c
Version     : Ver 1.0
Author      : abby
Date        : 20160905
Description : This file is compiled by MCU1
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPerformance.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR volatile U32 *g_pLLFDone;
GLOBAL MCU12_VAR_ATTR U32 g_aStartTimer[PERF_TYPE_CNT];
GLOBAL MCU12_VAR_ATTR U32 g_aEndTimer[PERF_TYPE_CNT];

void TEST_NfcInitLLFFlag(void)
{
    g_pLLFDone = (U32*)(DRAM_START_ADDRESS + 0x8000000);//multi-use LLF status address
    *g_pLLFDone = 0;
}

U32 TEST_NfcStartTimer(void)
{
    return (U32)HAL_GetMCUCycleCount();
}

U32 TEST_NfcEndTimer(U32 ulStartTimer, U32 ulEndTimer, U32 *pTimeDelta)
{
    U32 ulCycInUs, ulTimeInUs;
    
    ulEndTimer = (U32)HAL_GetMCUCycleCount();
    *pTimeDelta = COM_DiffU32(ulStartTimer, ulEndTimer);
    
    ulCycInUs = (U32)HAL_GetMcuClock() / 1000000;
    ulTimeInUs = *pTimeDelta / ulCycInUs;

    return ulTimeInUs;
}


/*  end of this file  */
