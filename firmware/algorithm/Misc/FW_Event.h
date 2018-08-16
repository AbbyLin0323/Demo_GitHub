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
Filename    :FW_Event.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.28
Description :Definitions for global event management system.
Others      :
Modify      :
****************************************************************************/
#ifndef __COM_EVENT_H__
#define __COM_EVENT_H__

#include "BaseDef.h"
#include "Disk_Config.h"

#ifdef SIM
extern GLOBAL MCU12_VAR_ATTR U32 g_ulDbgEvent;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulDbgPara1;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulDbgPara2;
extern GLOBAL MCU12_VAR_ATTR U32 g_ulDbgPara3;
#endif

#define COMM_EVENT_OFFSET_INIT              0x00
#define COMM_EVENT_OFFSET_LLF               0x01
#define COMM_EVENT_OFFSET_SHUTDOWN          0x02
#define COMM_EVENT_OFFSET_RESET             0x03
#define COMM_EVENT_OFFSET_ERR               0x04
#define COMM_EVENT_OFFSET_BOOT              0x05
#define COMM_EVENT_OFFSET_REBUILD           0x06
#define COMM_EVENT_OFFSET_LLFINIT           0x07
#define COMM_EVENT_OFFSET_DBG               0x08
#define COMM_EVENT_OFFSET_SERR              0x09
#define COMM_EVENT_OFFSET_IDLE              0x0a
#define COMM_EVENT_OFFSET_FAEDBG            0x0c
#define COMM_EVENT_OFFSET_HCMDERR           0x0d
#define COMM_EVENT_OFFSET_SAVERT            0x0e
#define COMM_EVENT_OFFSET_SELFTEST          0x0f
#define COMM_EVENT_OFFSET_REBUILD_DIRTYCNT  0x10
#define COMM_EVENT_OFFSET_BOOT_AFTER_RT     0x11
#define COMM_EVENT_OFFSET_SAVE_PBIT         0x12
#define COMM_EVENT_OFFSET_SAVE_BBT          0x13
#define COMM_EVENT_OFFSET_TrimL3            0x14
#define COMM_EVENT_OFFSET_LOWQD             0x15
#define COMM_EVENT_OFFSET_SAVETABLE         0x16

#define COMM_EVENT_PARAMETER_LEN            4
#define COMM_EVENT_OWNER_COUNT              3
#define COMM_EVENT_OWNER_L1                 0
#define COMM_EVENT_OWNER_L2                 1
#define COMM_EVENT_OWNER_L3                 2

#define L2_EVENT_PARAME_OFFSET_FORCE_GC     2

typedef union _COMMON_EVENT
{
    U32 Event;

    struct 
    {
        U32 EventInit               : 1;
        U32 EventLLF                : 1;
        U32 EventShutDown           : 1;
        U32 EventReset              : 1; //3
        U32 EventErrorHandling      : 1;
        U32 EventBoot               : 1;
        U32 EventRebuild            : 1;
        U32 EventLLFInit            : 1; //7
        U32 EventDbg                : 1;
        U32 EventSERR               : 1;
        U32 EventIdle               : 1;
        U32 EventSecurityErase      : 1; //11
        U32 EventFAEDbg             : 1;
        U32 EventHCMDError          : 1;
        U32 EventSaveRT             : 1;
        U32 EventSelfTest           : 1; //15
        U32 EventRebuildDirtyCnt    : 1;
        U32 EventBootAfterLoadRT    : 1;
        U32 EventSavePbit           : 1;
        U32 EventSaveBBT            : 1; //19
        U32 EventL3Trim             : 1; //20
        U32 EventLowQD              : 1; //21
        U32 EventSaveTable          : 1; //22
        U32 EventRsvd               : 9;
    };
} COMMON_EVENT;

typedef struct _COMMON_EVENT_PARAMETER
{
    U32 EventStatus;
    U32 EventParameterNormal[COMM_EVENT_PARAMETER_LEN];
} COMM_EVENT_PARAMETER;


typedef enum _COMM_EVENT_STATUS
{
    COMM_EVENT_STATUS_SUCCESS_NOEVENT = 0,
    COMM_EVENT_STATUS_SEND_BUSY,
    COMM_EVENT_STATUS_GET_EVENTPEND,
    COMM_EVENT_STATUS_BLOCKING,
    COMM_EVENT_STATUS_FAIL
} COMM_EVENT_STATUS;

typedef enum _COMM_DEBUG_CODE
{
    /* commone debug code for all level */
    COMM_DEBUG_REPORT_STATUS = 0x0,
    COMM_DEBUG_DISPLAY,
    COMM_DEBUG_WAIT,
    COMM_DEBUG_CODE_MAX
} COMM_DEBUG_CODE;

typedef struct _MCU12_COMMON_MISC_INFO
{
    COMM_EVENT_PARAMETER aModuleEventParameter[COMM_EVENT_OWNER_COUNT];
    COMMON_EVENT aModuleEvent[COMM_EVENT_OWNER_COUNT];
    U32 ulLBBT;
    U32 ulPbnBindingTable;
    U32 ulIsPbnBindingTableReady;
    U32 aLunNeedSaveBBT[LUN_NUM_PER_SUPERPU];
    U32 bSubSystemIdle;
    U32 bSubSystemSelfTestDone;
} MCU12_COMMON_MISC_INFO, *PMCU12_COMMON_MISC_INFO;

void CommEventInit();
U32 CommSetEvent(U8 EventOwner,U8 EventID);
U32 CommForceSetEvent(U8 EventOwner,U8 EventID);
U32 CommClearEvent(U8 EventOwner,U8 EventID);
U32 CommCheckEvent(U8 EventOwner,COMMON_EVENT* EVENT);
void CommGetEventParameter(U8 EventOwner, COMM_EVENT_PARAMETER **pParamter);
void CommDbgEventHandler(void);

extern  MCU12_VAR_ATTR volatile MCU12_COMMON_MISC_INFO *g_pMCU12MiscInfo;

#endif
/********************** FILE END ***************/

