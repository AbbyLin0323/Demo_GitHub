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
 * File Name    : Uart_fw.c
 * Discription  :
 * CreateAuthor : NickWang
 * CreateDate   : 2016429
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "uart.h"
#include  <stdarg.h>
#include "BaseDef.h"
#include "HAL_MultiCore.h"
#include "HAL_ParamTable.h"
#include "HAL_Xtensa.h"
#include <xtensa/tie/xt_timer.h>

LOCAL U8 *g_buff;
LOCAL U32 *g_wr;
LOCAL U32 *g_rd;
LOCAL U32 *g_buf_size;
extern GLOBAL BOOL bPrinttoDRAM;
GLOBAL BOOL IsMCU0init;
GLOBAL BOOL IsMCU1init;
GLOBAL BOOL IsMCU2init;

/* MCU 0 trace log parameter */
LOCAL U32 *g_tracebuff0;
LOCAL U32 *g_trace_buf_size0;

/* MCU 1 trace log parameter */
LOCAL U32 *g_tracebuff1;
LOCAL U32 *g_trace_buf_size1;

/* MCU 2 trace log parameter */
LOCAL U32 *g_tracebuff2;
LOCAL U32 *g_trace_buf_size2;

GLOBAL BOOL IsMCU0traceinit;
GLOBAL BOOL IsMCU1traceinit;
GLOBAL BOOL IsMCU2traceinit;
GLOBAL BOOL IsMCU0traceEnabled;
GLOBAL BOOL IsMCU1traceEnabled;
GLOBAL BOOL IsMCU2traceEnabled;

void dbg_init_variable()
{
    g_buff = (U8 *)((HAL_GetPrintDRAMStartAddr() << SHIFT_TO_5_BYTE) + DBG_HEADER_SIZE);
    g_wr = (U32 *)((HAL_GetPrintDRAMStartAddr() << SHIFT_TO_5_BYTE) /*+ DRAM_CACHEABLE_OFFSET */+ WPTR_OFFSET);
    g_rd = (U32 *)((HAL_GetPrintDRAMStartAddr() << SHIFT_TO_5_BYTE) /*+ DRAM_CACHEABLE_OFFSET */+ RPTR_OFFSET);
    g_buf_size = (U32 *)((HAL_GetPrintDRAMStartAddr() << SHIFT_TO_5_BYTE) /*+ DRAM_CACHEABLE_OFFSET*/+ BUF_SIZE_OFFSET);
    *g_buf_size = (HAL_GetPrintDRAMSize() << SHIFT_TO_5_BYTE) - DBG_HEADER_SIZE;
    bPrinttoDRAM = HAL_GetPrinttoDRAMEnFlag();
}


void dbg_putchar_init()
{
    if ((IsMCU0init == 0) && (HAL_GetMcuId() == MCU0_ID))
    {
        dbg_init_variable();
        *g_wr = 0x00;
        *g_rd = 0x00;
        IsMCU0init = 1;
    }
    else if ((IsMCU1init == 0) && (HAL_GetMcuId() == MCU1_ID))
    {
        dbg_init_variable();
        IsMCU1init = 1;
    }
    else if ((IsMCU2init == 0) && (HAL_GetMcuId() == MCU2_ID))
    {
        dbg_init_variable();
        IsMCU2init = 1;
    }
}

void dbg_putchar_dram(char c)
{
    U32 wr_index;
    
    wr_index = ((*g_wr) % (*g_buf_size));
    *(g_buff + wr_index) = c;
    (*g_wr)++;
}

#if 1

void MCU12_DRAM_TEXT trace_init_variable()
{
    U8 MCU_id = HAL_GetMcuId();
	if (MCU_id == MCU0_ID)
    {
        g_tracebuff0 = (U32 *)((HAL_GetTraceLogMCU0StartAddr() << SHIFT_TO_6_BYTE) + DBG_HEADER_SIZE);
        g_trace_buf_size0 = (U32 *)((HAL_GetTraceLogMCU0StartAddr() << SHIFT_TO_6_BYTE) + DRAM_CACHEABLE_OFFSET + BUF_SIZE_OFFSET);
        *g_trace_buf_size0 = (U32)((HAL_GetTraceLogDRAMSize() << SHIFT_TO_5_BYTE) - DBG_HEADER_SIZE)/DWORD_SIZE;
    }
    else if (MCU_id == MCU1_ID)
    {
        g_tracebuff1 = (U32 *)((HAL_GetTraceLogMCU1StartAddr() << SHIFT_TO_6_BYTE) + DBG_HEADER_SIZE);
        g_trace_buf_size1 = (U32 *)((HAL_GetTraceLogMCU1StartAddr() << SHIFT_TO_6_BYTE) + DRAM_CACHEABLE_OFFSET + BUF_SIZE_OFFSET);
        *g_trace_buf_size1 = (U32)((HAL_GetTraceLogDRAMSize() << SHIFT_TO_5_BYTE) - DBG_HEADER_SIZE)/DWORD_SIZE;
    }
    else if (MCU_id == MCU2_ID)
    {
        g_tracebuff2 = (U32 *)((HAL_GetTraceLogMCU2StartAddr() << SHIFT_TO_6_BYTE) + DBG_HEADER_SIZE);
        g_trace_buf_size2 = (U32 *)((HAL_GetTraceLogMCU2StartAddr() << SHIFT_TO_6_BYTE) + DRAM_CACHEABLE_OFFSET + BUF_SIZE_OFFSET);
        *g_trace_buf_size2 = (U32)((HAL_GetTraceLogDRAMSize() << SHIFT_TO_5_BYTE) - DBG_HEADER_SIZE)/DWORD_SIZE;
    }
    /* Get trace log method */
    //g_ucDATATransferMode = HAL_GetDataPacketMethod();
}



void MCU12_DRAM_TEXT dbg_tracelog_init(void)
{
    if (IsMCU0traceinit == 0 && (HAL_GetMcuId() == MCU0_ID))
    {
        trace_init_variable();
        IsMCU0traceinit = 1;
    }
    else if (IsMCU1traceinit == 0 && (HAL_GetMcuId() == MCU1_ID))
    {
        trace_init_variable();
        IsMCU1traceinit = 1; 
    }
    else if (IsMCU2traceinit == 0 && (HAL_GetMcuId() == MCU2_ID))
    {
        trace_init_variable();
        IsMCU2traceinit = 1;
    }
}

#if 0
void dbg_tracelog_MCU0(U32 ul_Lable)
{
    static U32 ulMCU0_idx = 0;
    *(g_tracebuff0 + (ulMCU0_idx % (*g_trace_buf_size0))) = ul_Lable;
    ulMCU0_idx += 1;

    *(g_tracebuff0 + (ulMCU0_idx % (*g_trace_buf_size0))) = XT_RSR_CCOUNT();
    ulMCU0_idx += 1;
}

void dbg_tracelog_MCU1(U32 ul_Lable)
{
#if 0
    static U32 ulMCU1_idx = 0;
    *(g_tracebuff1 + (ulMCU1_idx % (*g_trace_buf_size1))) = ul_Lable;
    ulMCU1_idx += 1;

    *(g_tracebuff1 + (ulMCU1_idx % (*g_trace_buf_size1))) = XT_RSR_CCOUNT();
    ulMCU1_idx += 1;
#else
	static U32 ulMCU1_idx = 0;
    if (IsMCU1traceEnabled == 1)
    {
        *(g_tracebuff1 + (ulMCU1_idx % (*g_trace_buf_size1))) = ul_Lable;
        ulMCU1_idx += 1;

        *(g_tracebuff1 + (ulMCU1_idx % (*g_trace_buf_size1))) = XT_RSR_CCOUNT();
        ulMCU1_idx += 1;
    }
#endif
}

void dbg_tracelog_MCU2(U32 ul_Lable)
{
    static U32 ulMCU2_idx = 0;
    *(g_tracebuff2 + (ulMCU2_idx % (*g_trace_buf_size2))) = ul_Lable;
    ulMCU2_idx += 1;

	*(g_tracebuff2 + (ulMCU2_idx % (*g_trace_buf_size2))) = XT_RSR_CCOUNT();
    ulMCU2_idx += 1;
}
#endif


void dbg_getrealcycle_MCU0(U32 ulLable, U32 ulCycle)
{
    static U32 ulMCU0_idx = 0;
    if (IsMCU0traceEnabled == 1)
    {
        *(g_tracebuff0 + (ulMCU0_idx % (*g_trace_buf_size0))) = ulLable;
        ulMCU0_idx += 1;

        *(g_tracebuff0 + (ulMCU0_idx % (*g_trace_buf_size0))) = ulCycle;
        ulMCU0_idx += 1;
	}
}

void dbg_getrealcycle_MCU1(U32 ulLable, U32 ulCycle)
{
    static U32 ulMCU1_idx = 0;
    if (IsMCU1traceEnabled == 1)
    {
        *(g_tracebuff1 + (ulMCU1_idx % (*g_trace_buf_size1))) = ulLable;
        ulMCU1_idx += 1;

        *(g_tracebuff1 + (ulMCU1_idx % (*g_trace_buf_size1))) = ulCycle;
        ulMCU1_idx += 1;
	}
}

void dbg_getrealcycle_MCU2(U32 ulLable, U32 ulCycle)
{
    static U32 ulMCU2_idx = 0;
    if (IsMCU2traceEnabled == 1)
    {
        *(g_tracebuff2 + (ulMCU2_idx % (*g_trace_buf_size2))) = ulLable;
        ulMCU2_idx += 1;

        *(g_tracebuff2 + (ulMCU2_idx % (*g_trace_buf_size2))) = ulCycle;
        ulMCU2_idx += 1;
	}
}

#endif
