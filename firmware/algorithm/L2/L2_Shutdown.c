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
Filename    :L2_Shutdown.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.04.25
Description :functions about Shutdown Process
Others      :
Modify      :
*******************************************************************************/
#include "HAL_Inc.h"
#include "L2_Shutdown.h"
#include "L2_Schedule.h"
#include "L2_PMTManager.h"
#include "L2_StripeInfo.h"
#include "L2_PMTI.h"
#include "L2_Thread.h"
#include "L2_FCMDQ.h"
#include "FW_Event.h"


extern GLOBAL U32 g_ulShutdownFlushPagePos[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL U32 g_ulShutdownFlushPageCnt[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL BOOL bShutdownPending;

//All Pu Shutdown
U32  L2_ShutdownEntry()
{
    U16 i = 0;
    U8 ucSuperPu;
    U16 FlushPMTIIndex[TABLE_RW_PAGE_CNT] = { INVALID_4F };
    U32 ret;
    U32 bShutDownDone = TRUE;

    ret = COMM_EVENT_STATUS_BLOCKING;

    //If Table Not Finish, just Wait
    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        if (TABLE_STATUS_OVER != g_TableRW[ucSuperPu].m_TableStatus)
        {
            return COMM_EVENT_STATUS_GET_EVENTPEND;
        }
    }

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        while (g_ulShutdownFlushPageCnt[ucSuperPu] < PMTPAGE_CNT_PER_SUPERPU)
        {
            FlushPMTIIndex[0] = g_ulShutdownFlushPagePos[ucSuperPu] % PMTPAGE_CNT_PER_SUPERPU;

            //Next PMTIndex to Flush
            g_ulShutdownFlushPagePos[ucSuperPu]++;
            g_ulShutdownFlushPageCnt[ucSuperPu]++;
            bShutDownDone = FALSE;

            ret = L2_SetTableRW(ucSuperPu, FlushPMTIIndex, 1, TABLE_WRITE);
            if (FALSE == ret)
            {
                return COMM_EVENT_STATUS_GET_EVENTPEND;
            }

            if (g_TableRW[ucSuperPu].m_TableRWCnt != 0)
            {
                L2_SetThreadQuota((U8)ucSuperPu, SYS_THREAD_TABLE_PMT, 1);
                L2_SetCurrThreadType((U8)ucSuperPu, SYS_THREAD_TABLE_PMT);
                break;
            }
        }
    }

    if (bShutDownDone)
    {
        //Must wait for Flash finish all flush cmd
        U8 ucTLun;
        for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
        {
            if (FALSE == L2_FCMDQIsEmpty(ucTLun, 0))
            {
                return ret;
            }
        }

        L2_TaskEventBootInit();
        return ret;
    }

    return COMM_EVENT_STATUS_GET_EVENTPEND;    
}

