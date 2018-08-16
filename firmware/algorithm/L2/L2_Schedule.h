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
File Name     : L2_RT.c
Version       : Initial version
Author        : henryluo
Created       : 2015/02/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2012/01/18
Author      : peterxiu
Modification: Created file

*******************************************************************************/
#ifndef _L2_SCHEDULE_H
#define _L2_SCHEDULE_H

#include "L1_Interface.h"



typedef struct {
    BOOL m_Shutdown;
    BOOL m_ShutdownDone;    //L2 shut down is done, control the L2 idle task
    BOOL m_WriteAfterShutdown;
    BOOL m_ForceIdle;
    BOOL m_WaitIdle;
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    U8 m_ShutdownSharedPageClosedDoneCnt;
#else//SHUTDOWN_IMPROVEMENT_STAGE1
    U8 m_ShutdownEraseTLCBlkDoneCnt;
#endif    
}L2_EventStatus;

extern GLOBAL  BOOL bShutdownPending;
extern GLOBAL  L2_EventStatus g_L2EventStatus;

static U32 L2_HandleEventLLF(void);
static U32 L2_HandleEventShutDown(void);
static U32 L2_HandleEventBoot(void);
static U32 L2_HandleEventBootAfterLoadRT(void);
static U32 L2_HandleEventRebuild(void);
static U32 L2_HandleEventRebuildDirtyCnt(void);
static U32 L2_HandleEventIdle(void);
static U32 L2_HandleEventSaveTable(void);

void L2_TaskInit(void);
extern U32 L2_TaskEventHandle(void);
extern void L2_TaskEventBootInit(void);
extern BOOL L2_IsEventPending();
extern U32 L2_CheckTaskEventStatus(BOOL bCheckWaitlde);

#endif

/********************** FILE END ***************/
