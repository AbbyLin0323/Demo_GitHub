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
 * File Name    : TEST_SpinLock.c
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "BaseDef.h"
#include "HAL_MultiCore.h"
#include "HAL_GLBReg.h"
#include "HAL_xtensa.h"
#include "Test_McuBasicFunc.h"


#define MCU_SPINLOCK_WAIT_TIME 20

LOCAL void MCU_SetMultiCoreMode(void)
{
    rGlbMCUSramMap = 0x0;
    return;
}

LOCAL void MCU_SetMCU12IsramSZ(void)
{
    rGLB(0x3C) |= 0x4;
    return;
}

LOCAL void MCU_IsramCacheOpen(void)
{
    rGLB(0x3C) |= 0x3;
    return;
}

LOCAL void MCU_BootMcu12FromIsram(void)
{
    //configure mcu1/2 boot from isram
    rGlbMCUCtrl = 0x0;
    rGlbMCUCtrl |= 0x110;

    //enable mcu1/2 boot up
    rGlbMcuSgeRst = 0x0;
}


void Test_GetReleaseSpinLockOneTime(U8 ucSpinLockId)
{
    U32 ulSpinLockSts;
    do
    {
        ulSpinLockSts = HAL_MultiCoreGetSpinLock(ucSpinLockId);
    }while(TRUE != ulSpinLockSts);
    HAL_MultiCoreReleaseSpinLock(ucSpinLockId);

    HAL_DelayCycle(MCU_SPINLOCK_WAIT_TIME);
    return;
}

void Test_GetReleaseSpinLock(U8 ucSpinLockId)
{
    Test_GetReleaseSpinLockOneTime(ucSpinLockId);
    Test_GetReleaseSpinLockOneTime(ucSpinLockId);
    return;
}

void Test_SpinLockMain(void)
{
    U8 ucSpinLockId;
    U32 ulMcuId;

    ulMcuId = HAL_GetMcuId();
    if (MCU0_ID == ulMcuId)
    {
        //config 3_core mode
        MCU_SetMultiCoreMode();
        MCU_SetMCU12IsramSZ();
        MCU_IsramCacheOpen();
        MCU_BootMcu12FromIsram();

        rTracer_Mcu0 = 0x22;
        do
        {
            ;
        }while((0xff != rTracer_Mcu1) || (0xff != rTracer_Mcu2));
        rTracer_Mcu3 = 0x11;
        rTracer_Mcu0 = 0x0;

        for(ucSpinLockId = 0; ucSpinLockId < SPIN_LOCK_COUNT; ucSpinLockId++)
        {
            Test_GetReleaseSpinLock(ucSpinLockId);
            rTracer_Mcu0++;
        }
        rTracer_Mcu0 = 0xdd;
    }

    if (MCU1_ID == ulMcuId)
    {
           rTracer_Mcu1 = 0xff;
        do
        {
            ;
        }while(rTracer_Mcu3 != 0x11);
        rTracer_Mcu1 = 0x0;
        for(ucSpinLockId = 0; ucSpinLockId < SPIN_LOCK_COUNT; ucSpinLockId++)
        {
            Test_GetReleaseSpinLock(ucSpinLockId);
            rTracer_Mcu1++;
        }
        rTracer_Mcu1 = 0xdd;
    }

    if (MCU2_ID == ulMcuId)
    {
        rTracer_Mcu2 = 0xff;
        do
        {
            ;
        }while(rTracer_Mcu3 != 0x11);
        rTracer_Mcu2 = 0x0;
        for(ucSpinLockId = 0; ucSpinLockId < SPIN_LOCK_COUNT; ucSpinLockId++)
        {
            Test_GetReleaseSpinLock(ucSpinLockId);
            rTracer_Mcu2++;
        }
        rTracer_Mcu2 = 0xdd;
    }
    while(1);
    return;
}
