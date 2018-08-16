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
Filename    : L0_Event.h
Version     : Ver 0.1
Author      : Gavin
Date        : 2015.01.13
Description : header for common asynchronous event module in L0.
Others      : 
Modify      :
20150113    Gavin     Create file
20150114    Gavin     add function interface
****************************************************************************/
#ifndef __L0_EVENT_H__
#define __L0_EVENT_H__
#include "BaseDef.h"

//event type definition by handling priority from high to low
/* Accelerates boot flow only on ASIC. */
typedef enum _L0_EVENT_TYPE
{
    L0_EVENT_TYPE_SUBSYS_BOOT,
    L0_EVENT_TYPE_CCEN,  //nvme controller enable    
    L0_EVENT_TYPE_XFER_RESET,// HBA.HR, CC.EN etc
    L0_EVENT_TYPE_SHUTDOWN,
    L0_EVENT_TYPE_PCIE_LINKFAILURE, // For NVMe only, upstream retry/abort handling
    L0_EVENT_TYPE_PCIE_FATALERROR,
    L0_EVENT_TYPE_DEBUG_MODE_EN,
    L0_EVENT_TYPE_ERR_HANDLING, //currently for SATA mode only
    L0_EVENT_TYPE_CMD_REJECT,
    L0_EVENT_TYPE_ATASTANDBYTIMEOUT,
    L0_EVENT_MAX_NUM
}L0_EVENT_TYPE;

//event process stage
typedef enum _L0_EVENT_STAGE
{
    L0_EVENT_STAGE_START,
    L0_EVENT_STAGE_PROCESSING,
    L0_EVENT_STAGE_END
}L0_EVENT_STAGE;

//define function type for call-back function when process event
typedef BOOL (*pL0EventProcessHandler)(void *);

//define function type for call-back function when event completes
typedef BOOL (*pL0EventCompleteHandler)(void *);

//event manager: record necessary information for every event
typedef struct _L0_EVENT_MGR
{
    U32  ucEventStage: 8;
    U32  bsEventRsvd: 24;
    pL0EventProcessHandler pProcessHandler;
    pL0EventCompleteHandler pCompleteHandler;
    void *pEventParam;
}L0_EVENT_MGR;

//function interface
void L0_EventInit(void);
void L0_EventRegHander(U32 ulEventId,
                        pL0EventProcessHandler pProcessHandler,
                        pL0EventCompleteHandler pCompleteHandler);
void L0_EventSet(U32 ulEventId, void *pEventParam);
void L0_EventClear(U32 ulEventId);
U32 L0_EventGetMap(void);
U32 L0_EventGetStage(U32 ulEventId);
void L0_EventSetStage(U32 ulEventId, U32 ulEventStage);
BOOL L0_EventProcess(void);

#endif //__L0_EVENT_H__
