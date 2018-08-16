/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : FlashFile.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#include "BaseDef.h"
#include "HAL_Define.h"
#include "HAL_DualCore.h"

#ifndef SIM
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/tie/xt_core.h>
#endif

volatile HAL_PIN_LOCK *pHalPinLock;


U32 HalDualCoreGetCPUID()
{
    U32 uCpuID = 0;

#ifdef ASIC
    uCpuID = XT_RSR_PRID();
#endif

    return uCpuID;
}

//MCU SPIN LOCK
U32 HalDualCoreGetSpinLock(U8 LockId)
{
   
#ifdef ASIC
    U32 uCpuID;
    U32 uLockValue;    
    
    uCpuID = HalDualCoreGetCPUID();

    *(volatile U32*)(SPINLOCK_BASE_ADDRESS + LockId*4) = uCpuID;

    uLockValue = *(volatile U32*)(SPINLOCK_BASE_ADDRESS + LockId*4);

    //return *(U32*)(SPINLOCK_BASE_ADDRESS + LockId*2);
    //return 0;
    return (uLockValue == uCpuID);
#else
    return 1;
#endif
}

void HalDualCoreReleaseSpinLock(U8 LockId)
{
    *(volatile U32*)(SPINLOCK_BASE_ADDRESS + LockId*4) = 0;
}


void HalDualCoreInit()
{

}

/********************** FILE END ***************/
