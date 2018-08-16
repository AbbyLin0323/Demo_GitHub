/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L2_Defines.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.28
Description :defines for algorithm firmware.
Others      :
Modify      :
****************************************************************************/
#ifndef __COM_EVENT_H__
#define __COM_EVENT_H__

extern U32 g_ulDbgEvent;
extern U32 g_ulDbgPara1;
extern U32 g_ulDbgPara2;
extern U32 g_ulDbgPara3;

//TRACE DEFINE START
#define COMM_SUBMODULE1_ID          1
#define COMM_SUBMODULE1_NAME        "Event"
#define COMM_SUBMODULE1_ENABLE      1
#define COMM_SUBMODULE1_LEVEL       LOG_LVL_TRACE
//TRACE DEFINE END

#define  COMM_EVENT_OFFSET_INIT     0
#define  COMM_EVENT_OFFSET_LLF      1
#define  COMM_EVENT_OFFSET_SHUTDOWN 2
#define  COMM_EVENT_OFFSET_RESET    3
#define  COMM_EVENT_OFFSET_ERR      4
#define  COMM_EVENT_OFFSET_BOOT     5
#define  COMM_EVENT_OFFSET_REBUILD  6
#define  COMM_EVENT_OFFSET_LLFINIT  7
#define  COMM_EVENT_OFFSET_DBG  8
#define  COMM_EVENT_OFFSET_RSVD     9

typedef union _COMMON_EVENT
{
    U32 Event;

    struct 
    {
        U32 EventInit:1;
        U32 EventLLF:1;
        U32 EventShutDown:1;
        U32 EventReset:1;
        U32 EventErrorHandling:1;
        U32 EventBoot:1;
        U32 EventRebuild:1;
        U32 EventLLFInit:1;
        U32 EventDbg:1;
        U32 EventRsvd:23;
    };
}COMMON_EVENT;

#define COMM_EVENT_PARAMETER_LEN 4

typedef struct _COMMON_EVNET_PARAMETER
{
    U32 EventStatus;
    U32 EventParameterNormal[COMM_EVENT_PARAMETER_LEN];
    U32 EventParameterFlash[CE_NUM];
}COMM_EVENT_PARAMETER;

#define COMM_EVENT_OWNER_COUNT 3
#define COMM_EVENT_OWNER_L1 0
#define COMM_EVENT_OWNER_L2 1
#define COMM_EVENT_OWNER_L3 2

#define  COMM_EVENT_PARAMETER_LEN 4

typedef enum _COMM_EVENT_STATUS
{
    COMM_EVENT_STATUS_SUCCESS_NOEVENT = 0,
    COMM_EVENT_STATUS_SEND_BUSY,
    COMM_EVENT_STATUS_GET_EVENTPEND,
    COMM_EVENT_STATUS_BLOCKING,
    COMM_EVENT_STATUS_FAIL
}COMM_EVENT_STATUS;

typedef enum _COMM_DEBUG_CODE
{
    /* commone debug code for all level */
    COMM_DEBUG_REPORT_STATUS = 0x0,
    COMM_DEBUG_DISPLAY,
    COMM_DEBUG_WAIT,
    COMM_DEBUG_CODE_MAX
}COMM_DEBUG_CODE;

void CommEventInit();
extern U32 CommSetEvent(U8 EventOnwer,U8 EventID);
U32 CommClearEvent(U8 EventOnwer,U8 EventID);
U32 CommCheckEvent(U8 EventOnwer,COMMON_EVENT* EVENT);
extern void CommGetEventParameter(U8 EventOnwer,COMM_EVENT_PARAMETER **pParamter);
extern void CommSetWaitDebugEventToL2();

#endif

