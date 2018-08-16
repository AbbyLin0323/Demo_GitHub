/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :                                           
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#ifndef _HAL_DUALCORE_H
#define _HAL_DUALCORE_H


//HW FIFO

U8 HalDualCoreFifo0Empty();
U8 HalDualCoreFifo0Full();
U8 HalDualCoreFifo0Push(void *InBuffer,U8 length);
U8 HalDualCoreFifo0Pop(void *OutBuffer,U8 length);

//MCU SPIN LOCK

//SPIN LOCK base
#define SPINLOCK_BASE_ADDRESS (REG_BASE_GLB + 0x100)

#define HAL_SPINLOCK_BEQ 0
#define HAL_SPINLOCK_FEQ 1
#define HAL_SPINLOCK_CEBASE 0
#define HAL_SPINLOCK_EVENTBASE  64

#define HAL_SPINLOCK_MODE_BLOCKING 0
#define HAL_SPINLOCK_MODE_NONEBLOCKING 1

#define PIN_LOCK_COUNT 128

typedef struct _HAL_PIN_LOCK
{
    U32 PIN_LOCK[PIN_LOCK_COUNT];
}HAL_PIN_LOCK;

void HalDualCoreInit();

void HalDualCoreSpinLockInit();
U32 HalDualCoreGetSpinLock(U8 LockId);
void HalDualCoreReleaseSpinLock(U8 LockId);

#define HAL_MCUID_0 1
#define HAL_MCUID_1 2
U32 HalDualCoreGetCPUID();
//MCU semphore



//Dual Core EVENT
#define HAL_DUALCORE_NOTIFY_MSG_ADDR
#define HAL_DUALCORE_NOTIFY_ACK_ADDR
void HalDualCoreNotify(U32 MsgType,U32 MsgInfo);
U32 HalDualCoreNotifyWaitResult();


//Dual Core share memory
#define SPINLOCK_BASE_ADDRESS (REG_BASE_GLB + 0x100)
#endif

/********************** FILE END ***************/

