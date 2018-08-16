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
Filename    :FW_Event.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Global event management function routines.
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "HAL_Xtensa.h"
#include "FW_Event.h"

#ifdef SIM
GLOBAL U32 g_ulDbgEvent;
GLOBAL U32 g_ulDbgPara1;
GLOBAL U32 g_ulDbgPara2;
GLOBAL U32 g_ulDbgPara3;
#endif

GLOBAL MCU12_VAR_ATTR volatile MCU12_COMMON_MISC_INFO *g_pMCU12MiscInfo;

/****************************************************************************
Name        :CommEventInit
Input       :None
Output      :None
Author      :HenryLuo
Date        :2012.02.15    15:11:36
Description :Initializes memory space for global event system.
Others      :
Modify      :
****************************************************************************/
void CommEventInit()
{
    U8 i, j;

    /* Clear all events and event parameters to 0 initially. */
    for (i = 0 ; i < COMM_EVENT_OWNER_COUNT; i++)
    {
        g_pMCU12MiscInfo->aModuleEvent[i].Event = 0;
        g_pMCU12MiscInfo->aModuleEventParameter[i].EventStatus = 0;

        for (j = 0; j < COMM_EVENT_PARAMETER_LEN; j++)
        {
            g_pMCU12MiscInfo->aModuleEventParameter[i].EventParameterNormal[j] = 0;
        }
    }

#ifdef SIM
    g_ulDbgEvent = 0;
    g_ulDbgPara1 = 0;
    g_ulDbgPara2 = 0;
    g_ulDbgPara3 = 0;
#endif

    return;
}

/****************************************************************************
Name        :CommSetEvent
Input       :EventOwner - Indicates which layer the event belongs to;
                EventID - Indicates which event would be set for that layer.
Output      :Returns the success or failure status of setting the event.
Author      :
Date        :
Description :Signals an event pending state for a layer.
Others      :
Modify      :
****************************************************************************/
U32 CommSetEvent(U8 EventOwner,U8 EventID)
{
    U32 EventEN;

    EventEN = ( 1 << EventID );

    /* An event of the same type is already pending. */
    if (0 != (g_pMCU12MiscInfo->aModuleEvent[EventOwner].Event & EventEN))
        return COMM_EVENT_STATUS_SEND_BUSY;

    /* Sets the event. */
    g_pMCU12MiscInfo->aModuleEvent[EventOwner].Event |= EventEN;

    return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
}

/****************************************************************************
Name        :CommForceSetEvent
Input       :EventOwner - Indicates which layer the event belongs to;
                EventID - Indicates which event would be set for that layer.
Output      :Returns the success or failure status of setting the event.
Author      :
Date        :
Description :Signals an event pending state for a layer.
Others      :
Modify      :
****************************************************************************/
U32 CommForceSetEvent(U8 EventOwner,U8 EventID)
{
    U32 EventEN;

    EventEN = ( 1 << EventID );

    /* Sets the event. */
    g_pMCU12MiscInfo->aModuleEvent[EventOwner].Event |= EventEN;

    return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
}


/****************************************************************************
Name        :CommClearEvent
Input       :EventOwner - Indicates which layer the event belongs to;
                EventID - Indicates which event would be cleared for that layer.
Output      :Returns the success or failure status of setting the event.
Author      :
Date        :
Description :Clears the pending status of the specified event for a layer.
Others      :
Modify      :
****************************************************************************/
U32 CommClearEvent(U8 EventOwner,U8 EventID)
{
    U32 EventEN;

    EventEN = ( 1 << EventID );

    /* Clears the event. */
    g_pMCU12MiscInfo->aModuleEvent[EventOwner].Event &= ~EventEN;

    return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
}


/****************************************************************************
Name        :CommCheckEvent
Input       :EventOwner - Indicates for which layer we shall check event pending status;
                EVENT - Indicates where we shall store the event pending status.
Output      :The event pending status of specified layer would be stored to specified address,
                while return value reflects whether there is an event pending.
Author      :
Date        :
Description :Copy the event pending status of a specified layer to the specified memory space.
Others      :
Modify      :
****************************************************************************/
U32 CommCheckEvent(U8 EventOwner,COMMON_EVENT* EVENT)
{
    EVENT->Event = g_pMCU12MiscInfo->aModuleEvent[EventOwner].Event;

    if (0 == EVENT->Event)
    {
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    else
    {
        return COMM_EVENT_STATUS_GET_EVENTPEND;
    }
}

/****************************************************************************
Name        :CommGetEventParameter
Input       :EventOwner - Indicates for which layer we shall get parameters for a pending event;
                pParamter - Indicates where we shall store the start address of event parameters.
Output      :The start address of event parameters for specified layer would be stored to
                specified address.
Author      :
Date        :
Description :Copy the start address of event parameter for a specified layer to the specified
                memory space.
Others      :
Modify      :
****************************************************************************/
void CommGetEventParameter(U8 EventOwner, COMM_EVENT_PARAMETER **pParamter)
{
    *pParamter = (COMM_EVENT_PARAMETER *)&(g_pMCU12MiscInfo->aModuleEventParameter[EventOwner]);
}

/****************************************************************************
Name        :CommDbgEventHandler
Input       :void
Output      :void
Author      :HenryLuo
Date        :2012.11.29    18:29:57
Description :debug event entrance.
Others      :input g_ulDbgEvent by JTAG to enable debug event.
g_ulDbgEvent[15:8]: debug level.
g_ulDbgEvent[7:0]: debug code.
Modify      :
****************************************************************************/
#ifdef SIM
void CommDbgEventHandler(void)
{
    U32 ulEventOwner;
    U8 ucDbgCode;
    U8 ucDbgLevel;
    COMM_EVENT_PARAMETER * pParameter;

    if(0 == g_ulDbgEvent)
    {
        return;
    }
    else
    {
        ucDbgLevel = (U8)((g_ulDbgEvent & 0xFF00) >> 8);
        ucDbgCode = (U8)(g_ulDbgEvent & 0xFF);

        /* DbgCode work on all level, use for common debug code */
        if(0xFF == ucDbgLevel)
        {
            if(ucDbgCode >= COMM_DEBUG_CODE_MAX)
            {
                DBG_Printf("input wrong usDbgCode! \n");
                DBG_Getch();
            }
            else
            {
                for(ulEventOwner = COMM_EVENT_OWNER_L1; 
                    ulEventOwner < COMM_EVENT_OWNER_COUNT; ulEventOwner++)
                {
                    //get parameter from comm event
                    CommGetEventParameter(ulEventOwner,&pParameter);
                    pParameter->EventParameterNormal[0] = ucDbgCode;
                    pParameter->EventParameterNormal[1] = g_ulDbgPara1;
                    pParameter->EventParameterNormal[2] = g_ulDbgPara2;
                    pParameter->EventParameterNormal[3] = g_ulDbgPara3;

                    //set init event
                    CommSetEvent(ulEventOwner, COMM_EVENT_OFFSET_DBG);
                }
            }
        }

        else
        {
            switch(ucDbgLevel)
            {
                case 1:
                    //get parameter from comm event
                    CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
                    pParameter->EventParameterNormal[0] = ucDbgCode;
                    pParameter->EventParameterNormal[1] = g_ulDbgPara1;
                    pParameter->EventParameterNormal[2] = g_ulDbgPara2;
                    pParameter->EventParameterNormal[3] = g_ulDbgPara3;

                    //set init event
                    CommSetEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_DBG);
                    break;

                case 2:
                    //get parameter from comm event
                    CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
                    pParameter->EventParameterNormal[0] = ucDbgCode;
                    pParameter->EventParameterNormal[1] = g_ulDbgPara1;
                    pParameter->EventParameterNormal[2] = g_ulDbgPara2;
                    pParameter->EventParameterNormal[3] = g_ulDbgPara3;

                    //set init event
                    CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_DBG);
                    break;

                case 3:
                    //get parameter from comm event
                    CommGetEventParameter(COMM_EVENT_OWNER_L3,&pParameter);
                    pParameter->EventParameterNormal[0] = ucDbgCode;
                    pParameter->EventParameterNormal[1] = g_ulDbgPara1;
                    pParameter->EventParameterNormal[2] = g_ulDbgPara2;
                    pParameter->EventParameterNormal[3] = g_ulDbgPara3;

                    //set init event
                    CommSetEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_DBG);
                    break;

                default:
                    DBG_Printf("input wrong DbgEvent! \n");
                    break;
            }
        }

        g_ulDbgEvent = 0;
        g_ulDbgPara1 = 0;
        g_ulDbgPara2 = 0;
        g_ulDbgPara3 = 0;
    }
}
#endif

/********************** FILE END ***************/

