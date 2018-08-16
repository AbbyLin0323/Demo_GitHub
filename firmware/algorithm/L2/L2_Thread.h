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
Filename    :L2_Thread.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.28
Description :defines for Thread Dispatcher
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_THREAD_H__
#define __L2_THREAD_H__
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L2_Defines.h"
#include "L1_Cache.h"
#include "L1_Buffer.h"

typedef struct SCHEDULER_TAG
{
    SYS_THREAD_TYPE eCurrThread;
    U16 aThreadQuota[SYS_THREAD_TYPE_ALL];
    U16 usThreadQuotaUsed;
}SCHEDULE;

extern void L2_InitScheduler(U8 ucPuNum);
extern void L2_SetSystemState(U8 ucPuNum, SYSTEM_STATE eSystemState);
extern SYSTEM_STATE L2_GetCurrSystemState(U8 ucPuNum);
extern void L2_SetCurrThreadType(U8 ucPuNum, SYS_THREAD_TYPE eThreadSet);
extern void L2_SetThreadQuota(U8 ucPuNum, SYS_THREAD_TYPE eThreadSet, U32 ulQuota);
extern SYS_THREAD_TYPE L2_GetCurrThreadType(U8 ucPuNum);
void L2_Scheduler();
extern U32 GetLpnInSystem(U32 ulPuID, U32 ulLpnInPu);

#endif
