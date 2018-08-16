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
Filename    : TEST_NfcPerformance.h
Version     : Ver 1.0
Author      : abby
Date        : 20160905
Description : config NFC test feature in this file
              compile both by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_PERFORMANCE_H_
#define _HAL_NFC_TEST_PERFORMANCE_H_

#include "TEST_NfcConfig.h"
#include "HAL_Xtensa.h"

/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR U32 g_aStartTimer[PERF_TYPE_CNT];
extern GLOBAL MCU12_VAR_ATTR U32 g_aEndTimer[PERF_TYPE_CNT];
extern GLOBAL MCU12_VAR_ATTR volatile U32 *g_pLLFDone;

/*------------------------------------------------------------------------------
    shared macro by basic and ext pattern
------------------------------------------------------------------------------*/

#define LLF_DONE                    (0x55AAAA55)//for L3 performance test

U32 TEST_NfcStartTimer(void);
U32 TEST_NfcEndTimer(U32 ulStartTimer, U32 ulEndTimer, U32 *pTimeDelta);
void TEST_NfcInitLLFFlag(void);
#endif

/*  end of this file  */
