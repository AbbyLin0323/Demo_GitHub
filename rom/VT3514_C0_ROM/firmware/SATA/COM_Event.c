/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :bootup.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Sata interface function
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "COM_Event.h"

U32 g_ulDbgEvent;
U32 g_ulDbgPara1;
U32 g_ulDbgPara2;
U32 g_ulDbgPara3;
COMMON_EVENT ModuleEvent[COMM_EVENT_OWNER_COUNT];
COMM_EVENT_PARAMETER (*ModuelEventParameter)[COMM_EVENT_OWNER_COUNT];

void CommEventInit()
{
    U8 i;
    U8 p;

    ModuelEventParameter = (COMM_EVENT_PARAMETER(*)[COMM_EVENT_OWNER_COUNT])STATIC_PARAMETER_COMMON_EVENT_RESULT;

    for (i = 0 ; i < COMM_EVENT_OWNER_COUNT;i++)
    {
        ModuleEvent[i].Event = 0;

        for (p = 0 ; p < sizeof(COMM_EVENT_PARAMETER)/sizeof(U32);p++)
        {
            *((U32*)&(*ModuelEventParameter)[i] + p) = 0;
        }
    }

    g_ulDbgEvent = 0;
    g_ulDbgPara1 = 0;
    g_ulDbgPara2 = 0;
    g_ulDbgPara3 = 0;
    
}

U32 CommSetEvent(U8 EventOnwer,U8 EventID)
{
    U32 EventEN;

    EventEN = 1 << EventID;

    if (0 != (ModuleEvent[EventOnwer].Event & EventEN))
        return COMM_EVENT_STATUS_SEND_BUSY;

    while(!HalDualCoreGetSpinLock(HAL_SPINLOCK_EVENTBASE + EventOnwer))
        ;

    ModuleEvent[EventOnwer].Event |= EventEN;

    HalDualCoreReleaseSpinLock(HAL_SPINLOCK_EVENTBASE + EventOnwer);

    return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
}

U32 CommClearEvent(U8 EventOnwer,U8 EventID)
{
    U32 EventEN;

    EventEN = 1 << EventID;

    while(!HalDualCoreGetSpinLock(HAL_SPINLOCK_EVENTBASE + EventOnwer))
        ;

    ModuleEvent[EventOnwer].Event &= ~EventEN;

    HalDualCoreReleaseSpinLock(HAL_SPINLOCK_EVENTBASE + EventOnwer);

    return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
}


U32 CommCheckEvent(U8 EventOnwer,COMMON_EVENT* EVENT)
{
    if (0 == ModuleEvent[EventOnwer].Event)
    {
        EVENT->Event = 0;
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    EVENT->Event = ModuleEvent[EventOnwer].Event;

    return COMM_EVENT_STATUS_GET_EVENTPEND;
}

void CommGetEventParameter(U8 EventOwner,COMM_EVENT_PARAMETER **pParamter)
{
    *pParamter = &(*ModuelEventParameter)[EventOwner];
}

void CommEventValidation()
{
    U8 EventOnwer;
    COMMON_EVENT EventLocal;
    COMM_EVENT_PARAMETER *pParameter;
    //    U8 ParameterLen;
    U8 ParameterCounter;

    CommEventInit();

    for (EventOnwer = 0 ; EventOnwer < COMM_EVENT_OWNER_COUNT; EventOnwer++)
    {
        //check event should free
        if(COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommCheckEvent(EventOnwer,&EventLocal))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        CommGetEventParameter(EventOnwer,&pParameter);

        for (ParameterCounter = 0 ;ParameterCounter < sizeof(COMM_EVENT_PARAMETER)/sizeof(U32); ParameterCounter++)
            *((U32*)pParameter + ParameterCounter) = 0;

        //set event 1 should success
        if(COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommSetEvent(EventOnwer,1))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        //set event 3 should success
        if(COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommSetEvent(EventOnwer,3))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        //set event 1 should fail
        if(COMM_EVENT_STATUS_SEND_BUSY != CommSetEvent(EventOnwer,1))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        //check event should busy
        if(COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(EventOnwer,&EventLocal))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        //Printf event
        DBG_Printf("EVENT %x\n",EventLocal.Event);

        CommGetEventParameter(EventOnwer,&pParameter);

        for (ParameterCounter = 0 ;ParameterCounter < sizeof(COMM_EVENT_PARAMETER)/sizeof(U32); ParameterCounter++)
        {    
            DBG_Printf("P%d %x\n",ParameterCounter,*((U32*)pParameter + ParameterCounter));
        }

        CommClearEvent(EventOnwer,3);

        //check event should busy
        if(COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(EventOnwer,&EventLocal))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        CommClearEvent(EventOnwer,1);

        //check event should busy
        if(COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommCheckEvent(EventOnwer,&EventLocal))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        //set event 1 should success
        if(COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommSetEvent(EventOnwer,1))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }

        //check event should busy
        if(COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(EventOnwer,&EventLocal))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }



        CommClearEvent(EventOnwer,1);

        //check event should busy
        if(COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommCheckEvent(EventOnwer,&EventLocal))
        {
            DBG_Printf("error 0\n");
            DBG_Getch();
        }


    }

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

void CommSetWaitDebugEventToL2()
{
    COMM_EVENT_PARAMETER * pParameter;
    
    //get parameter from comm event
    CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
    pParameter->EventParameterNormal[0] = COMM_DEBUG_WAIT;
    //set wait debug event
    CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_DBG);

    return;
}
/********************** FILE END ***************/
