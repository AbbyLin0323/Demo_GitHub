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
Filename     :  L0_NVMeISR.c                                     
Version      :  Ver 1.0                                               
Date         :  20141209                                   
Author       :  Alpha Liu

Description: 
    Completing Interrupt handling for Nvme. 
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "HAL_NVME.h"
#include "HAL_Interrupt.h"
#include "L0_NVMe.h"
#include "L0_NVMeErrhandle.h"
#include "L0_Event.h"

extern volatile U32 l_ulRevMpInt;

LOCAL volatile BOOL l_ulNVMeL12IdleFlag;

void L0_NVMeCcEnISR(void)
{
    if (TRUE == CC_EN())
    {
        L0_EventSet(L0_EVENT_TYPE_CCEN, NULL);
    }
    else
    {
        L0_NVMeControllerReset();
    }
    
    return;
}

void L0_NVMeShutdownISR(void)
{
    if (NORMAL_SHN == CC_SHN())
    {
        SET_CSTS_SHST(SHN_PROCESS_OCCURING);
        L0_EventSet(L0_EVENT_TYPE_SHUTDOWN, NULL);
    }
    else if(ABRUPT_SHN == CC_SHN())
    {   
        SET_CSTS_SHST(SHN_PROCESS_COMPLETE);
        /* lost control behavior, do nothing, resign to table rebuild. */
    }
    return;
}

void L0_NVMePcieResetISR(void)
{
    L0_NVMePCIeReset();
    return;
}

void L0_NVMePCIeFailureISR(void)
{
    /* Marks the event for L0. */
    L0_EventSet(L0_EVENT_TYPE_PCIE_LINKFAILURE, NULL);
    return;
}

void L0_NVMePCIeFatalISR(void)
{
    /* Disables PCI Bus Master ability temporarily for 5 us. */
#ifdef VT3533_A2ECO_PCIE_128BDATAERR
    rPCIeNCfg(0x4) &= ~(1 << 2);
    HAL_DelayUs(5);
    rPCIeNCfg(0x4) |= (1 << 2);
#endif
    /* Marks the event for L0. */
    L0_EventSet(L0_EVENT_TYPE_PCIE_FATALERROR, NULL);
    return;
}

INLINE BOOL HAL_NVMeGetL12IdleFlag(void)
{
    return l_ulNVMeL12IdleFlag;
}

INLINE void HAL_NVMeSetL12IdleFlag(void)
{
    l_ulNVMeL12IdleFlag = TRUE;
    return;
}

INLINE void HAL_NVMeClearL12IdleFlag(void)
{
    l_ulNVMeL12IdleFlag = FALSE;
    return;
}

void L0_NVMeISR(void)
{
    U32 ulIntStatus;
    U32 ulClrStatus;

    //HAL_DisableMCUIntAck();
    
    ulIntStatus = (rHOSTC_INTPENDING) & ~(rHOSTC_INTMASK);//*(volatile U32 *)(0x1ff83a10);
    ulClrStatus = 0;

    if (ulIntStatus & INT_HOSTC_NVME_CMDEN)
    {
        L0_NVMeCcEnISR();
        ulClrStatus |= INT_HOSTC_NVME_CMDEN;
    }
    
    if (ulIntStatus & INT_HOSTC_NVME_SHUTDOWN)
    {
        L0_NVMeShutdownISR();
        ulClrStatus |= INT_HOSTC_NVME_SHUTDOWN;
    }

    if (ulIntStatus & INT_HOSTC_PERST_DAST)
    {
        L0_NVMePcieResetISR();
        ulClrStatus |= (INT_HOSTC_PERST_DAST | INT_HOSTC_PERST_INBD);
    }

    if (ulIntStatus & INT_HOSTC_PERST_INBD)
    {
        L0_NVMePcieResetISR();
        ulClrStatus |= INT_HOSTC_PERST_INBD;
    }

    if (ulIntStatus & INT_HOSTC_FATALERR)
    {
        L0_NVMePCIeFatalISR();
        ulClrStatus |= INT_HOSTC_FATALERR;
    }

    if (ulIntStatus & INT_HOSTC_PCIEFAILURE)
    {
        L0_NVMePCIeFailureISR();
        ulClrStatus |= INT_HOSTC_PCIEFAILURE;
    }

    if (ulIntStatus & INT_HOSTC_L12_IDLE)
    {
        HAL_NVMeSetL12IdleFlag();
        ulClrStatus |= INT_HOSTC_L12_IDLE;
    }

#ifndef SIM
    if(ulIntStatus & INT_HOSTC_MPTOOL)
    {
        l_ulRevMpInt = TRUE;
        ulClrStatus |= INT_HOSTC_MPTOOL;
    }
#endif

#if 0
    if (ulIntStatus & INT_HOSTC_BUSMASTER)
    {
        L0_NVMeBMEChgISR();
        ulClrStatus |= INT_HOSTC_BUSMASTER;
    }
#endif

    rHOSTC_INTPENDING = ulClrStatus;

    //HAL_EnableMCUIntAck();
    return;
}

