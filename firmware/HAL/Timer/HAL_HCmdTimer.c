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
Filename    : HAL_HCmdTimer.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.12.12
Description : Driver code for host command timer, including Init/Start/Stop
              interface.
Others      : The host command timer is only supported in VT3514 C0.
Modify      :
20141212    Gavin     Create file
20141215    Gavin     implement detail function
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_HCmdTimer.h"
#include "HAL_Interrupt.h"

#ifdef FPGA
#define HCLK_COUNT_PER_US (50)
#else
#define HCLK_COUNT_PER_US (300)
#endif

//function pointer for saving user registered time-out call-back ISR
LOCAL pHCmdTimerTimeOutISR l_pUserTimerOutCallBack;

/*------------------------------------------------------------------------------
Name: HAL_HCmdTimerInit
Description: 
    register time-out ISR and set time-out threshold.
Input Param:
    pHCmdTimerTimeOutISR pUsrTimeOutISR:
        pointer to user time-out ISR. if NULL, means disable time-out interrupt;
    U32 ulTimeOutThreshold:
        the time-out threshold in microsecond(us) unit.
Output Param:
    none
Return Value:
    void.
Usage:
    In multi-core FW, this function should be called by one MCU only.
    supported in COSIM/FPGA/ASIC ENV, C0 HW only;
History:
    20141215    Gavin   create
------------------------------------------------------------------------------*/
void HAL_HCmdTimerInit(pHCmdTimerTimeOutISR pUsrTimeOutISR, U32 ulTimeOutThreshold)
{
    if (NULL == pUsrTimeOutISR)
    {
        l_pUserTimerOutCallBack = NULL;
        rTimeOutThres = INVALID_8F;
        
        //disable HW interrupt by setting mask bit to 1
        rHOSTC_INTMASK |= INT_HOSTC_TIMER;
    }
    else
    {
        l_pUserTimerOutCallBack = pUsrTimeOutISR;
        rTimeOutThres = ulTimeOutThreshold * HCLK_COUNT_PER_US;
        
        //enable HW interrupt by clearing mask bit to 0
        rHOSTC_INTMASK &= ~INT_HOSTC_TIMER;
    }

    rTimeOutCtrl |= 1;//bit[0] = 1, global enable for all timers
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCmdTimerInterruptEntry
Description:
    Interrupt entry for this driver when any timer time out.
    The user registered time-out ISR is called in this function.
Input Param:
    void
Output Param:
    none
Return Value:
    void
Usage:
    called by level 1 interrupt main entry;
History:
    20141215    Gavin   create
------------------------------------------------------------------------------*/
void HAL_HCmdTimerInterruptEntry(void)
{
    U8 ucTimerIndex;

    if ((NULL == l_pUserTimerOutCallBack) ||
        (0 == (rTimeOutCtrl & (1 << 1))))
    {
        //can not get here if no call-back function registered or no time-out happened
        DBG_Getch();
    }
    else
    {
        for (ucTimerIndex = 0; ucTimerIndex < HCMD_TIMER_NUM; ucTimerIndex++)
        {
            if (TRUE == HAL_HCmdTimerCheckTimeOut(ucTimerIndex))
            {
                //call user ISR
                l_pUserTimerOutCallBack(ucTimerIndex);
                break;
            }
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCmdTimerStart
Description: 
    trigger specified timer to start counting from zero.
Input Param:
    U8 ucTimerId: timer id (valid id is 0~63)
Output Param:
    none
Return Value:
    void.
Usage:
    the timer will always start counting from 0 when this function called;
    supported in COSIM/FPGA/ASIC ENV, C0 HW only;
History:
    20141215    Gavin   create
------------------------------------------------------------------------------*/
void HAL_HCmdTimerStart(U8 ucTimerId)
{
    if (ucTimerId < 32)
    {
        rTimerStart1 = (1 << ucTimerId);
    }
    else if (ucTimerId < 64)
    {
        rTimerStart2 = (1 << (ucTimerId - 32));
    }
    else
    {
        DBG_Getch();
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCmdTimerStop
Description: 
    stop specified timer from counting.
    the timer will hold its current count when this function called;
Input Param:
    U8 ucTimerId: timer id (valid id is 0~63)
Output Param:
    none
Return Value:
    void.
Usage:
    supported in COSIM/FPGA/ASIC ENV, C0 HW only;
History:
    20141215    Gavin   create
------------------------------------------------------------------------------*/
void HAL_HCmdTimerStop(U8 ucTimerId)
{
    if (ucTimerId < 32)
    {
        rTimerStop1 = (1 << ucTimerId);
    }
    else if (ucTimerId < 64)
    {
        rTimerStop2 = (1 << (ucTimerId - 32));
    }
    else
    {
        DBG_Getch();
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HCmdTimerGetCurTime
Description: 
    get current time of specified timer by microsecond(us) unit.
Input Param:
    U8 ucTimerId: timer id (valid id is 0~63)
Output Param:
    none
Return Value:
    current time by microsecond(us) unit.
Usage:
    supported in COSIM/FPGA/ASIC ENV, C0 HW only;
History:
    20141215    Gavin   create
------------------------------------------------------------------------------*/
U32 HAL_HCmdTimerGetCurTime(U8 ucTimerId)
{
    rTimerSelect = ucTimerId;

    return rTimerValue / HCLK_COUNT_PER_US;
}

/*------------------------------------------------------------------------------
Name: HAL_HCmdTimerCheckTimeOut
Description: 
    check specified timer is time out or not.
Input Param:
    U8 ucTimerId: timer id (valid id is 0~63)
Output Param:
    none
Return Value:
    BOOL: TRUE = the specified timer is time out; FALSE = not time out.
Usage:
    supported in COSIM/FPGA/ASIC ENV, C0 HW only;
History:
    20141215    Gavin   create
------------------------------------------------------------------------------*/
BOOL HAL_HCmdTimerCheckTimeOut(U8 ucTimerId)
{
    U32 ulCheckBitMap;

    if (ucTimerId < 32)
    {
        ulCheckBitMap = rTimeOutStatus1 & (1 << ucTimerId);
    }
    else if (ucTimerId < 64)
    {
        ulCheckBitMap = rTimeOutStatus2 & (1 << (ucTimerId - 32));
    }
    else
    {
        DBG_Getch();
    }

    return (0 == ulCheckBitMap) ? FALSE : TRUE;
}
/* end of file HAL_HCmdTimer.c */

