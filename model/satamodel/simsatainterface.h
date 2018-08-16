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
#ifndef _SIM_SATA_INTERFACE_H_
#define _SIM_SATA_INTERFACE_H_
#include "simsatainc.h"
#include <windows.h>

#ifdef SIM_DBG
#define SDC_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun, TRACE_SATADEVICE_MODULE_ID,TRACE_SATADEVICE_SUBMODULE_ID,log_level,x, __VA_ARGS__);
#else
#define SDC_LogInfo(x, ...)  (void)0
#endif

typedef struct _FIFO_STRUCT
{
    void *pFifo;
    U32 stNodeByteLen;
    U32 stMaxNode;
    U32 stFifoHead;
    U32 stFifoTail;

}FIFO_STRUCT;

// about prd
void SDC_GetPrdBaseAddr();
void SDC_IniReadPrd();
void SDC_IniWritePrd();
BOOL SDC_IsReadPrdFull();
BOOL SDC_IsReadPrdEmpty();
U8 SDC_AllocateReadPrd();
U8 SDC_GetCurRPTRForReadPrd();
U8 SDC_AccumulateRPTRReadPrd();
BOOL SDC_IsWritePrdFull();
BOOL SDC_IsWritePrdEmpty();
U8 SDC_AllocateWritePrd();
U8 SDC_GetCurRPtrForWritePrd();
U8 SDC_AccumulateRPTRWritePrd();
BOOL SDC_IsLastPrd(SATA_DSG* p_rprd);

// buffer map function
void SDC_SetWriteBufferMap(U8 bufmap_id,U32 mapvalue);
void SDC_ClearWriteBufferMap(U8 bufmap_id);
U32 SDC_GetWriteBufferMap(U8 bufmap_id);
void SDC_SetReadBufferMap(U32 bufmap_value);
void SDC_SetReadBufferMapWin(U8 buffmap_id,U32 buffermap_value);
U32 SDC_GetReadBufferMap(U8 buffmap_id);
void SDC_ClearReadBufferMap(U8 bufmap_id,U32 bufmap_value);
void SDC_InitReadBufferMap(U8 bufmap_id,U32 bufmap_value);

// cache status function
void SDC_ClearCacheStatus(U32 buf_id);
void SDC_SetCacheStatus(U32 uCacheStatusAddr, U32 uCacheStatusData);

// first data ready function
void SDC_ClearFirstReadDataReady(U8 uTag);
void SDC_SetFirstReadDataReady(U8 uRegValue);
BOOL SDC_IsFirstReadDataReady(U8 uTag);
BOOL SDC_GetFirstDataReady(U8 *pTag);
void SDC_InitFirstDataRead();
void SDC_ClearLastDataReady(U8 uTag);
void SDC_SetLastDataReady(U8 uRegValue);
BOOL SDC_IsLastDataReady(U8 uTag);
BOOL SDC_NoLastDataReady(void);

extern CRITICAL_SECTION g_ReadPrdCriticalSection;
extern CRITICAL_SECTION g_WritePrdCriticalSection;
extern CRITICAL_SECTION g_FirstDataReadyCriticalSection;
//extern SATA_PRD_ENTRY* g_pSDCReadPrd;//sim_rprd;
//extern SATA_PRD_ENTRY* g_pSDCWritePrd;//sim_wprd;
extern U8 g_FirstReadDataReady[SIM_HOST_CMD_MAX];

#endif