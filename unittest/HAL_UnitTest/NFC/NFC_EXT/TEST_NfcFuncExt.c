/*******************************************************************************
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
Filename    : TEST_NfcFuncExt.c
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : 
Others      :
Modify      :
*******************************************************************************/

#include "TEST_NfcFuncExt.h"

extern GLOBAL MCU12_VAR_ATTR U32 g_aStartTimer[PERF_TYPE_CNT];
extern GLOBAL MCU12_VAR_ATTR U32 g_aEndTimer[PERF_TYPE_CNT];

LOCAL U32 TEST_NfcStartOneCMDPerfTimer(void)
{   
#ifdef L3_ONE_CMD_CYC_CNT
    if (LLF_DONE == *g_pLLFDone)//LLF stage: not start timer
    {
        return TEST_NfcStartTimer();
    }
#else
    return 0;
#endif
}

LOCAL void TEST_NfcEndOneCMDPerfTimer(U32 ulStartTimer, U32 ulEndTimer)
{
#ifdef L3_ONE_CMD_CYC_CNT
    if (LLF_DONE == *g_pLLFDone)//LLF stage: not start timer
    {
        U32 ulTimeDelta = 0;
        U32 ulTimeDeltaInUs = TEST_NfcEndTimer(ulStartTimer, ulEndTimer, &ulTimeDelta);
        DBG_Printf("L3_ONE_CMD_CYC_CNT: cycle cnt 0x%x, cost time %d us\n", ulTimeDelta, ulTimeDeltaInUs);
    }
#endif
}

void TEST_NfcExtPattRun(void)
{
    g_aStartTimer[L3_ONE_CMD_PERF] = TEST_NfcStartOneCMDPerfTimer();
    
    /* execute CMD */
    L3_Scheduler();
    
    TEST_NfcEndOneCMDPerfTimer(g_aStartTimer[L3_ONE_CMD_PERF], g_aEndTimer[L3_ONE_CMD_PERF]);
    
    return;
}


//------- end of this file --------//

