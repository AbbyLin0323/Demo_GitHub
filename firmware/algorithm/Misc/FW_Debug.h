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
Filename    :FW_Debug.h
Version     :Ver 1.0
Author      :AwayWei
Date        :2013.04.03
Description :Debug function related definitions.
Others      :
Modify      :
****************************************************************************/

#ifndef __COM_DEBUG_H__
#define __COM_DEBUG_H__
#include "HAL_MemoryMap.h"
#include "L0_Interface.h"

#ifdef SIM
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#endif

#define PRINT_LEVEL_ERR      0
#define PRINT_LEVEL_WARNING  1
#define PRINT_LEVEL_INFO     2
#define PRINT_LEVEL_DEBUG    3

#define PRINT_LEVEL PRINT_LEVEL_INFO
// Error situation.
#if (PRINT_LEVEL >= PRINT_LEVEL_ERR)
#define PRINT_ERR DBG_Printf
#else
#define PRINT_ERR(x,...)
#endif
// Warning situation.
#if (PRINT_LEVEL >= PRINT_LEVEL_WARNING)
#define PRINT_WARNING DBG_Printf
#else
#define PRINT_WARNING(x,...)
#endif
// Information situation.
#if (PRINT_LEVEL >= PRINT_LEVEL_INFO)
#define PRINT_INFO DBG_Printf 
#else
#define PRINT_INFO(x,...)
#endif
// Debug situation.
#if (PRINT_LEVEL >= PRINT_LEVEL_DEBUG)
#define PRINT_DEBUG DBG_Printf 
#else
#define PRINT_DEBUG(x,...)
#endif

#define DMA_IDLE 0
#define DMA_BUSY 1

#define CACHE_IDLE 0
#define CACHE_BUSY 1

#define CMD_READ 0
#define CMD_WRITE 1

#define PRD_BWRITE_BIT (1<<23) 

typedef enum _CHECK_WT_STATE
{
    WT_CHECK_PRD_PTR = 0x0,
    WT_CHECK_CACHE_STATUS,
    WT_CHECK_BUF_REQ,
    WT_CHECK_FLASH_REQ,
    WT_CHECK_CQ_ENTRY,
    WT_CHECK_FLASH_STATUS,
    WT_CHECK_DONE
}CHECK_WT_STATE;


typedef struct _TRACE_BLOCK_MANAGEMENT
{
    //DWORD 0
    U32 m_TraceStage;

    //DWORD 1  
    U16 m_TraceEraseBlockSN;   
    U16 m_TracePhyPos;  

    //DWORD 2   
    U16 m_TracePhyPageRemain;
    U16 m_NeedRebuild;
    
    //DWORD 3 
    U32 m_TimeStamp;
}TRACE_BLOCK_MANAGEMENT;

typedef enum _TRACE_STAGE
{
    TRACE_INIT = 0,
    TRACE_PREPARE,   
    TRACE_SAVE,
    TRACE_ERASE,
    TRACE_REBUILD,
    TRACE_BLOCK_SAVE_DONE,
}TRACE_STAGE;

extern TRACE_BLOCK_MANAGEMENT g_TraceManagement;
extern U32 COM_DbgCheckL1Status(void);
extern U32 COM_DbgCheckL1Status(void);
extern U32 COM_DbgCheckL3Status(void);
extern U32 COM_DbgCheckCmdType(void);
extern void COM_DbgCheckCacheStatus(void);
extern U32 COM_DbgCheckWriteCmd(void);
extern U32 COM_DbgCheckReadCmd(void);
extern U32 COM_DbgCheckComStatus(void);
extern U32 COM_DbgCheckSysStatus(void);
extern void COM_DbgRecordSysStatus(void);
extern void COM_DbgWarmInit(void);
extern void FW_TraceManagementLLF();
extern void FW_FlushSubsystemTrace(SCMD* pSCMD);
extern void FW_FlushMcu0Trace(SCMD* pSCMD);

#endif

/********************** FILE END ***************/

