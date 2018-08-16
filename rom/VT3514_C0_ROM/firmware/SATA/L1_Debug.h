/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_Debug.h
Version     :Ver 1.0
Author      :Blakezhang
Date        :2012.12.07
Description :include some POSIX C header file
Others      :
Modify      :
****************************************************************************/

#ifndef __L1_DEBUG_H__
#define __L1_DEBUG_H__
#ifdef SIM
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#endif

//TRACE DEFINE START
#define L1_SUBMODULE1_ID        1
#define L1_SUBMODULE1_NAME      "Debug"
#define L1_SUBMODULE1_ENABLE    1
#define L1_SUBMODULE1_LEVEL     LOG_LVL_TRACE
//TRACE DEFINE END

#define FISDELAY_LOW_LOW        380
#define FISDELAY_LOW_HIGH       410
#define FISDELAY_HIGH_LOW       560
#define FISDELAY_HIGH_HIGH      580

typedef enum _L1_DEBUG_CODE
{
    /* 0x0 -- 0xf use for commone debug code */

    /* L1 debug code */
    L1_DEBUG_LBA_SHOW = 0x10,
    L1_DEBUG_CACHELINE_SHOW,  /* 0x11 */
    L1_DEBUG_BUFFER_SHOW_SINGLE, /* 0x12 */
    L1_DEBUG_BUFFER_SHOW_ALL,     /* 0x13 */
    L1_DEBUG_LBA_READ,        /* 0x14 */
    L1_DEBUG_PRINT_PATTERN,        /* 0x15 */
    L1_DEBUG_CODE_MAX
}L1_DEBUG_CODE;
U32 DRAM_ATTR L1_DbgEventHandler(void);

void Dbg_SetFISDelay(U16 usDelay);
void Dbg_AutoSetFISDelay(void);

#ifdef HOST_PATTERN_RECORD
void DRAM_ATTR L1_DbgPrintPattern(void);
#endif

#endif

