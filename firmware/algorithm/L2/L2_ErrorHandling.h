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
Filename    :L2_ErrorHandling.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.04.05
Description :defines for Error Handling
Others      :
Modify      :
****************************************************************************/
#ifndef __L2_ERROR_HANDLING_H__
#define __L2_ERROR_HANDLING_H__

#include "L2_GCManager.h"

#ifdef SIM
extern CRITICAL_SECTION  g_CriticalAllocFreeBlk;
#define  GET_ALLOC_FREE_BLK_LOCK EnterCriticalSection(&g_CriticalAllocFreeBlk)
#define  RELEASE_ALLOC_FREE_BLK_LOCK LeaveCriticalSection(&g_CriticalAllocFreeBlk)
#endif

typedef enum GC_ERR_HANDLE_STAGE_TAG
{
    LOAD_SPARE_SPACE = 0,
    REBUILD_RPMT,
    RETURN_TO_GC,
    GC_ERROR_STAGE_ALL
}GC_ERR_HANDLE_STAGE;

typedef struct GC_ERROR_HANDLE_TAG
{
    GC_ERR_HANDLE_STAGE m_ErrHandleStage;
    U32 m_ErrPU;
    U32 m_ErrBlock;
    U32 m_ErrHandlePage;
    U16 m_ucErrWL;
    U8 m_ucLun;
    U8 m_ucErrRPMTnum;

    U8 m_FlashStatus[PG_PER_BLK * LUN_NUM_PER_SUPERPU];

} GC_ERROR_HANDLE;

extern GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_GCErrHandle;
extern GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_SWLErrHandle;


void L2_ErrorHandlingInit(U8 ucPuNum);
BOOL L2_ErrorHandlingEntry(U8 ucPuNum, BOOL bGcErrHandle, GCComStruct * ptGCPointer);
BOOL L2_WLErrorHandlingEntry(U8 ucPuNum);

U16 L2_ErrHApplyNewPBN(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, BOOL bTLCBlk);
void L2_ErrHRecostructMapping(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, U16 usNewPhyBlk, U8 ucErrType);
U16 L2_ErrHReplaceBLK(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, BOOL bTLCBlk, U8 ucErrType);
#endif
