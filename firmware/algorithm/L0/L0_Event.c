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
Filename    : L0_Event.c
Version     : Ver 0.5
Author      : Gavin
Date        : 2015.01.13
Description : Implement common asynchronous event mechanism in L0
Others      : the implementation for event processing, like reset/error handling/
              shutdown, is not task of this module.
Modify      :
20150113    Gavin     Create file
20150114    Gavin     add and modify some description
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "L0_Event.h"

/* bitmap for all event status: each bit with value 1 indicates one event pending */
GLOBAL volatile U32 g_ulL0Event;

/* event manager: record stage/param pointer/handler for every event */
GLOBAL L0_EVENT_MGR g_aEventMgr[L0_EVENT_MAX_NUM];

BOOL L0_IsTaskMgrActive(void);

/*------------------------------------------------------------------------------
Name: L0_EventInit
Description: 
    set all event to default value.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    Called by L0 after power on, and should be called only once
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
void L0_EventInit(void)
{
    g_ulL0Event = 0;
    COM_MemZero((U32 *)&g_aEventMgr[0], sizeof(g_aEventMgr) / sizeof(U32));

    return;
}

/*------------------------------------------------------------------------------
Name: L0_EventRegHander
Description: 
    hook custom process/complete function for a event.
Input Param:
    U32 ulEventId: event id, see L0_EVENT_TYPE
    pL0EventProcessHandler pProcessHandler: call-back function for process event
    pL0EventCompleteHandler pCompleteHandler: call-back function for complete event
Output Param:
    none
Return Value:
    void
Usage:
    if any custom call-back is needed for a event handling, call this function to register
    the function in boot stage 
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
void L0_EventRegHander(U32 ulEventId,
                        pL0EventProcessHandler pProcessHandler,
                        pL0EventCompleteHandler pCompleteHandler)
{
    if (NULL != pProcessHandler)
    {
        g_aEventMgr[ulEventId].pProcessHandler = pProcessHandler;
    }

    if (NULL != pCompleteHandler)
    {
        g_aEventMgr[ulEventId].pCompleteHandler = pCompleteHandler;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: L0_EventSet
Description: 
    mark a new event with param bound to it.
Input Param:
    U32 ulEventId: event id, see L0_EVENT_TYPE
    void *pEventParam: pointer to evnet param
Output Param:
    none
Return Value:
    void
Usage:
    called when any event happens
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
void L0_EventSet(U32 ulEventId, void *pEventParam)
{
    if (L0_EVENT_STAGE_PROCESSING != g_aEventMgr[ulEventId].ucEventStage)
    {
        g_ulL0Event |= (1 << ulEventId);
        g_aEventMgr[ulEventId].pEventParam = pEventParam;
        g_aEventMgr[ulEventId].ucEventStage = L0_EVENT_STAGE_START;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: L0_EventClear
Description: 
    delete a event.
Input Param:
    U32 ulEventId: event id, see L0_EVENT_TYPE
Output Param:
    none
Return Value:
    void
Usage:
    called when finish processing event or want to discard it.
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
void L0_EventClear(U32 ulEventId)
{
    g_ulL0Event &= ~(1 << ulEventId);
    g_aEventMgr[ulEventId].pEventParam = (void *)NULL;

    return;
}

U32 L0_EventGetMap(void)
{
    return g_ulL0Event;
}

/*------------------------------------------------------------------------------
Name: L0_EventGetStage
Description: 
    get the process stage of event.
Input Param:
    U32 ulEventId: event id. see L0_EVENT_TYPE
Output Param:
    none
Return Value:
    U32: event stage. see L0_EVENT_STAGE
Usage:
    called anywhere needed.
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
U32 L0_EventGetStage(U32 ulEventId)
{
    return (U32)(g_aEventMgr[ulEventId].ucEventStage);
}

/*------------------------------------------------------------------------------
Name: L0_EventSetStage
Description: 
    set the process stage of event.
Input Param:
    U32 ulEventId: event id. see L0_EVENT_TYPE
    U32 ulEventStage: event stage. see L0_EVENT_STAGE
Output Param:
    none
Return Value:
    void
Usage:
    called anywhere process event.
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
void L0_EventSetStage(U32 ulEventId, U32 ulEventStage)
{
    g_aEventMgr[ulEventId].ucEventStage = ulEventStage;

    return;
}

/*------------------------------------------------------------------------------
Name: L0_EventProcess
Description: 
    find a event with highest priority and process it.
Input Param:
    void
Output Param:
    none
Return Value:
    BOOL: TRUE: event process done; FALSE: not done
Usage:
    called in main schedule loop.
History:
    20150114    Gavin   create and add description
------------------------------------------------------------------------------*/
BOOL L0_EventProcess(void)
{
    U32 ulL0TaskInActiveState;
    U32 ulEventIndex;
    L0_EVENT_MGR *pEventMgr;
    BOOL bCompleteFinish = FALSE;

    if (0 != g_ulL0Event)
    {
        ulL0TaskInActiveState = L0_IsTaskMgrActive();

        //polling event by priority(event id 0 with the highest priority)
        for (ulEventIndex = 0; ulEventIndex < L0_EVENT_MAX_NUM; ulEventIndex ++)
        {
            //check event pending or not
            if (0 != ((1 << ulEventIndex) & g_ulL0Event))
            {
                /* We have to wait until task manager returns to ACTIVE state, except for ATA Standby timer expiring event. */
                if ((FALSE == ulL0TaskInActiveState) &&
                    (L0_EVENT_TYPE_ATASTANDBYTIMEOUT != ulEventIndex))
                {
                    return TRUE;
                }

                pEventMgr = &g_aEventMgr[ulEventIndex];

                //process event if needed
                if (L0_EVENT_STAGE_END != pEventMgr->ucEventStage)
                {
                    if (TRUE == pEventMgr->pProcessHandler(pEventMgr->pEventParam))
                    {
                        L0_EventSetStage(ulEventIndex, L0_EVENT_STAGE_END);
                    }
                }

                //do completion if event process finish
                if (L0_EVENT_STAGE_END == pEventMgr->ucEventStage)
                {
                    if (NULL != pEventMgr->pCompleteHandler)
                    {
                        //call custom call-back if it was registered
                        bCompleteFinish = pEventMgr->pCompleteHandler(pEventMgr->pEventParam);
                        if (TRUE == bCompleteFinish)
                        {
                            L0_EventClear(ulEventIndex);
                        }
                    }
                    else
                    {
                        //just clear the event
                        L0_EventClear(ulEventIndex);
                    }
                }
                else
                {
                    return FALSE;//return quickly and do not check next event
                }
            }
        }//for (ulEventIndex = 0...
    }

    return (0 == g_ulL0Event) ? TRUE : FALSE;
}

/* end of file L0_Event.c */

