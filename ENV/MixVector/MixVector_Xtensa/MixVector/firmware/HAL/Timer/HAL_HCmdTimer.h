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
Filename    : HAL_HCmdTimer.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.12.12
Description : Header file for host command timer driver.
Others      : The host command timer is only supported in VT3514 C0.
Modify      :
20141212    Gavin     Create file
20151215    Gavin     add description
*******************************************************************************/
#ifndef __HAL_HCMD_TIMER_H__
#define __HAL_HCMD_TIMER_H__

//in VT3514 C0, HW implement 64 timers
#define HCMD_TIMER_NUM  64

//reigster definition
#define rTimeOutCtrl    (*(volatile U32 *)(REG_BASE_NVME + 0x11C8))
#define rTimeOutThres   (*(volatile U32 *)(REG_BASE_NVME + 0x11CC))
#define rTimerStart1    (*(volatile U32 *)(REG_BASE_NVME + 0x11D0))
#define rTimerStart2    (*(volatile U32 *)(REG_BASE_NVME + 0x11D4))
#define rTimeOutStatus1 (*(volatile U32 *)(REG_BASE_NVME + 0x11D8))
#define rTimeOutStatus2 (*(volatile U32 *)(REG_BASE_NVME + 0x11DC))
#define rTimerSelect    (*(volatile U32 *)(REG_BASE_NVME_EXT + 0x120))
#define rTimerValue     (*(volatile U32 *)(REG_BASE_NVME_EXT + 0x124))
#define rTimerStop1     (*(volatile U32 *)(REG_BASE_NVME_EXT + 0x128))
#define rTimerStop2     (*(volatile U32 *)(REG_BASE_NVME_EXT + 0x12C))

//definition for call-back function when time-out interrupt happens
typedef void (*pHCmdTimerTimeOutISR)(U8 ucTimerId);

//function interface
void HAL_HCmdTimerInit(pHCmdTimerTimeOutISR pUsrTimeOutISR, U32 ulTimeOutThreshold);
void HAL_HCmdTimerStart(U8 ucTimerId);
void HAL_HCmdTimerStop(U8 ucTimerId);
U32 HAL_HCmdTimerGetCurTime(U8 ucTimerId);
BOOL HAL_HCmdTimerCheckTimeOut(U8 ucTimerId);

#endif // __HAL_HCMD_TIMER_H__

