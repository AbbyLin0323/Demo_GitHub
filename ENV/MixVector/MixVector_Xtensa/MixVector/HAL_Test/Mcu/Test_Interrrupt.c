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
Filename    : Test_Interrupt.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_timer.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/tie/xt_core.h>

#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#include "HAL_Interrupt.h"
#include "Test_McuBasicFunc.h"
#ifdef COSIM
#include "HAL_DramConfig.h"
#endif

//#define MCU_TO_MCU_INTERRRUPT_TEST
#define GPIO_INTERRRUPT_TEST

U32 g_ulMcuID;

LOCAL void MCU_SetMutiCoreMode(void)
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
    rGlbMCUCtrl = 0x110;
    
    //enable mcu1/2 boot up
    rGlbMcuSgeRst = 0x0;
}

/*===================================================================
Function   :     MCU_TopClearIntMask
Input      :     None
Output     :     None
Description : 
    clear interrupt mask in global register
Note: 
Modify History:

===================================================================*/
LOCAL void MCU_TopClearIntMask(U16 IntSrc, U32 ulMcuId)
{
    if (MCU0_ID == ulMcuId)
    {
        rGlbIntMsk0 &= ~IntSrc;
    }
    else if (MCU1_ID == ulMcuId)
    {
        rGlbIntMsk1 &= ~IntSrc;
    }
    else //(MCU2_ID == ulMcuId)
    {
        rGlbIntMsk2 &= ~IntSrc;
    }
    
    return;
}

/*===================================================================
Function   :     MCU_SendInterruptSignal
Input      :     None
Output     :     None
Description : 
    send signal to model
Note: 
    just used for gpio interrupte test.

===================================================================*/
void MCU_SendInterruptSignal(U32 ulMcuId)
{
    if(MCU0_ID == ulMcuId)
    {
        rTracer_Mcu0 = 0x33;
    }
    else if(MCU1_ID == ulMcuId)
    {
        rTracer_Mcu1 = 0x33;
    }
    else
    {
        rTracer_Mcu2 = 0x33;
    }
}

/*===================================================================
Function   :     MCU_SetMcuToMcuInterrupt
Input      :     None
Output     :     None
Description : 
    
Note: 

===================================================================*/
void MCU_SetMcuToMcuInterrupt(U32 ulSrcMcuId, U32 ulDstMcuId)
{
    if((MCU0_ID == ulSrcMcuId) && (MCU1_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU0_1_SET;
    }
    else if((MCU0_ID == ulSrcMcuId) && (MCU2_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU0_2_SET;
    }
    else if((MCU1_ID == ulSrcMcuId) && (MCU0_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU1_0_SET;
    }
    else if((MCU1_ID == ulSrcMcuId) && (MCU2_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU1_2_SET;
    }
    else if((MCU2_ID == ulSrcMcuId) && (MCU0_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU2_0_SET;
    }
    else if((MCU2_ID == ulSrcMcuId) && (MCU1_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU2_1_SET;
    }
    else
    {
        DBG_Getch();
    }    
}

/*===================================================================
Function   :     MCU_ClearMcuToMcuInterrupt
Input      :     None
Output     :     None
Description : 
    
Note: 

===================================================================*/
void MCU_ClearMcuToMcuInterrupt(U32 ulSrcMcuId, U32 ulDstMcuId)
{
    if((MCU0_ID == ulSrcMcuId) && (MCU1_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU0_1_CLEAN;
    }
    else if((MCU0_ID == ulSrcMcuId) && (MCU2_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU0_2_CLEAN;
    }
    else if((MCU1_ID == ulSrcMcuId) && (MCU0_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU1_0_CLEAN;
    }
    else if((MCU1_ID == ulSrcMcuId) && (MCU2_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU1_2_CLEAN;
    }
    else if((MCU2_ID == ulSrcMcuId) && (MCU0_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU2_0_CLEAN;
    }
    else if((MCU2_ID == ulSrcMcuId) && (MCU1_ID == ulDstMcuId))
    {
        rGlbIPIIntTrig = INT_MCU2_1_CLEAN;
    }
    else
    {
        DBG_Getch();
    }    
}

/*===================================================================
Function   :     CPU_InitInterrupt
Input      :     None
Output     :     None
Description :
    Init MCU Interrupt with all interrupt disable
Note:
Modify History:

===================================================================*/
GLOBAL void MCU_InitInterruptGPIO(U32 ulMcuId)
{
    MCU_TopClearIntMask(TOP_INTSRC_GPIO, ulMcuId);
    HAL_EnableMCUIntSrc(BIT_ORINT_GPIO);

    return;
}

GLOBAL void MCU_InitInterruptMcuToMcu(U32 ulMcuId)
{
    if(MCU0_ID == ulMcuId)
    {
        MCU_TopClearIntMask(TOP_MCU0_INTSRC_MCU1_0, ulMcuId);
        MCU_TopClearIntMask(TOP_MCU0_INTSRC_MCU2_0, ulMcuId);
        HAL_EnableMCUIntSrc(BIT_ORINT_MCU1_0);
        HAL_EnableMCUIntSrc(BIT_ORINT_MCU2_0);
    }
    else if(MCU1_ID == ulMcuId)
    {
        MCU_TopClearIntMask(TOP_MCU1_INTSRC_MCU0_1, ulMcuId);
        MCU_TopClearIntMask(TOP_MCU1_INTSRC_MCU2_1, ulMcuId);
        HAL_EnableMCUIntSrc(BIT_ORINT_MCU0_1);
        HAL_EnableMCUIntSrc(BIT_ORINT_MCU2_1);
    }
    else
    {
        MCU_TopClearIntMask(TOP_MCU2_INTSRC_MCU0_2, ulMcuId);
        MCU_TopClearIntMask(TOP_MCU2_INTSRC_MCU1_2, ulMcuId);
        HAL_EnableMCUIntSrc(BIT_ORINT_MCU0_2);
        HAL_EnableMCUIntSrc(BIT_ORINT_MCU1_2);
    }
    return;
}

LOCAL void Test_ClearGPIOIntSts(U32 ulMcuId)
{
    if (MCU0_ID == ulMcuId)
    {
        if (TRUE == HAL_IsGPIOEdgeOccur(14))
        {
            HAL_GPIOClearEdgeInt(14);
        }
        else
        {
            DBG_Getch();
        }
    }
    else if (MCU1_ID == ulMcuId)
    {
        if (TRUE == HAL_IsGPIOLevelIntOccur(15))
        {
            HAL_GPIOClearLevelInt(15);
        }
        else
        {
            DBG_Getch();
        }
    }
    else
    {
        if (TRUE == HAL_IsGPIOEdgeOccur(16))
        {
            HAL_GPIOClearEdgeInt(16);
        }
        else
        {
            DBG_Getch();
        }

    }

    return;
}

LOCAL void Test_WaitGPIOInterrupRsvDone(U32 ulMcuId)
{
    if (MCU0_ID == ulMcuId)
    {
        while (rTracer_Mcu0 != 0xf)
        {
            ;
        }
        rTracer_Mcu0 = 0xff;
    }
    else if (MCU1_ID == ulMcuId)
    {
        while (rTracer_Mcu1 != 0xf)
        {
            ;
        }
        rTracer_Mcu1 = 0xff;
    }
    else
    {
        while (rTracer_Mcu2 != 0xf)
        {
            ;
        }
        rTracer_Mcu2 = 0xff;
    }
    
    return;
}


#ifdef GPIO_INTERRRUPT_TEST
void level1_interrupt_handler(void)
{
    U32 ulIntSrc;

    ulIntSrc = XT_RSR_INTERRUPT();
    if ( 0 != ( ulIntSrc & BIT_ORINT_GPIO ) )
    {
      HAL_DisableMCUIntSrc(BIT_ORINT_GPIO);
      Test_ClearGPIOIntSts(g_ulMcuID);
      if (MCU0_ID == g_ulMcuID)
      {
        rTracer_Mcu0 = 0x0f;
      }
      else if (MCU1_ID == g_ulMcuID)
      {
        rTracer_Mcu1 = 0x0f;
      }
      else
      {
        rTracer_Mcu2 = 0x0f;
      }
      //XT_WAITI(0);//for debug
    }

    return;
}
#endif /* CPIO_INTERRRUPT_TEST */

#ifdef MCU_TO_MCU_INTERRRUPT_TEST
void level1_interrupt_handler(void)
{
    U32 ulIntSrc;

    ulIntSrc = XT_RSR_INTERRUPT();

    if (MCU0_ID == g_ulMcuID)
    {
        if (0 !=  ( ulIntSrc & BIT_ORINT_MCU1_0 ))
        {
            rTracer_Mcu0++;

            //close mcu1 to mcu0 interrupt
            MCU_ClearMcuToMcuInterrupt(MCU1_ID, MCU0_ID);

            if((2 == rTracer_Mcu0) && (2 == rTracer_Mcu1) && (2 == rTracer_Mcu2))
            {
                rTracer_Mcu3 = 0xdd;
            }
        }

        if (0 != ( ulIntSrc & BIT_ORINT_MCU2_0 ))
        {
            rTracer_Mcu0++;

            //close mcu2 to mcu0 interrupt
            MCU_ClearMcuToMcuInterrupt(MCU2_ID, MCU0_ID);
        }
    }
    else if (MCU1_ID == g_ulMcuID)
    {
        if (0 != ( ulIntSrc & BIT_ORINT_MCU0_1 ))
        {
            rTracer_Mcu1++;

            //close mcu0 to mcu1 interrupt
            MCU_ClearMcuToMcuInterrupt(MCU0_ID, MCU1_ID);
        }

        if (0 != ( ulIntSrc & BIT_ORINT_MCU2_1 ))
        {
            rTracer_Mcu1++;

            //close mcu2 to mcu1 interrupt
            MCU_ClearMcuToMcuInterrupt(MCU2_ID, MCU1_ID);
        }
    }
    else
    {
        if (0 != ( ulIntSrc & BIT_ORINT_MCU0_2 ))
        {
            rTracer_Mcu2++;

            //close mcu0 to mcu2 interrupt
            MCU_ClearMcuToMcuInterrupt(MCU0_ID, MCU2_ID);
        }

        if (0 != ( ulIntSrc & BIT_ORINT_MCU1_2 ))
        {
            rTracer_Mcu2++;

            //close mcu1 to mcu2 interrupt
            MCU_ClearMcuToMcuInterrupt(MCU1_ID, MCU2_ID);
        }
    }

    return;
}
#endif /* MCU_TO_MCU_INTERRRUPT_TEST */
/*===================================================================
Function   :     user_exception_handler
Input      :     None
Output     :     None
Description :
    time execption handler
    step:
    1) get current TTMR value
    2) clear timer interrupt pending
Note:
Modify History:

===================================================================*/
void user_exception_handler()
{
    U32 ulExcCause, ulEPC1;

    ulExcCause = XT_RSR_EXCCAUSE();

    switch(ulExcCause) {
        case EXCCAUSE_LEVEL1_INTERRUPT:
            level1_interrupt_handler();
            break;

        default:
            asm ( "rsr.epc1 %0\n\t \
                      s32i %2, %1, 0\n\t \
                      s32i %0, %1, 4\n\t \
                      waiti 4\n\t"
                      : "+a"(ulEPC1) : "a"(0x30000), "a"(ulExcCause) : "memory" );

            break;
    }

    return;
}


LOCAL void Test_McuGPIOInterrupt(void)
{
    if (MCU0_ID == g_ulMcuID)
    {
        rTracer_Mcu0 = 0x0;
        HAL_GPIORasingEdgeIntInit(14);
    }
    else if (MCU1_ID == g_ulMcuID)
    {
        rTracer_Mcu1 = 0x0;
        while (0xf != rTracer_Mcu0)
        {
            ;
        }
        HAL_GPIOHighLevelIntInit(15);
    }
    else
    {
        rTracer_Mcu2 = 0x0;
        while (0xf != rTracer_Mcu1)
        {
            ;
        }
        HAL_GPIOFallingEdgeIntInit(16);
    }
    
    HAL_DisableMCUIntAck();   
    MCU_InitInterruptGPIO(g_ulMcuID);
    HAL_EnableMCUIntAck();
    MCU_SendInterruptSignal(g_ulMcuID);

    Test_WaitGPIOInterrupRsvDone(g_ulMcuID);

    while(1);
    return;
}

LOCAL void Test_McuToMcuInterrupt(void)
{
    if (MCU0_ID == g_ulMcuID)
    {   
        while(0xcc != rTracer_Mcu3)
        {
            ;
        }
        //test pattern 7: MCU to MCU interrupt**********************//
        HAL_DisableMCUIntAck();
        MCU_InitInterruptMcuToMcu(g_ulMcuID);
        HAL_EnableMCUIntAck();
        
        rTracer_Mcu0 = 0;
        rTracer_Mcu3 = 0;
        
        MCU_SetMcuToMcuInterrupt(g_ulMcuID, MCU1_ID);
        
        while (1 != rTracer_Mcu0)
        {
            ;
        }

        MCU_SetMcuToMcuInterrupt(g_ulMcuID, MCU2_ID);

        while ((0xdd != rTracer_Mcu3) && (2 == rTracer_Mcu0))
        {
            ;
        }
        
        while (1);
    } 
    else
    {
        //test pattern 7: MCU to MCU interrupt**********************//
        HAL_DisableMCUIntAck();
        MCU_InitInterruptMcuToMcu(g_ulMcuID);
        HAL_EnableMCUIntAck();
        
        if (MCU1_ID == g_ulMcuID)
        {   
            rTracer_Mcu3 |= 0xc;
            rTracer_Mcu1 = 0;
            while(1 != rTracer_Mcu1)
            {
                ;
            }
            MCU_SetMcuToMcuInterrupt(g_ulMcuID, MCU2_ID);
            while(2 != rTracer_Mcu1)
            {
                ;
            }
            MCU_SetMcuToMcuInterrupt(g_ulMcuID, MCU0_ID);
            while(1);
        }    
        if (MCU2_ID == g_ulMcuID)
        {
            while(0xc != rTracer_Mcu3)
            {
                ;
            }
            rTracer_Mcu3 |= 0xc0;
            rTracer_Mcu2 = 0;
            while(1 != rTracer_Mcu2)
            {
                ;
            }
            MCU_SetMcuToMcuInterrupt(g_ulMcuID, MCU0_ID);
            while(2 != rTracer_Mcu2)
            {
                ;
            }
            MCU_SetMcuToMcuInterrupt(g_ulMcuID, MCU1_ID);
            while(1);
        }
    }

    return;
}

void Test_McuInterruptMain(void)
{
    g_ulMcuID = HAL_GetMcuId();
    if (MCU0_ID == g_ulMcuID)
    {
        rTracer_Mcu0= 0x11;
        MCU_SetMutiCoreMode();

#ifndef VT3514_C0
        MCU_SetMCU12IsramSZ();
        MCU_IsramCacheOpen();
#endif

        /* Boot MCU12 */
        MCU_BootMcu12FromIsram();

#ifdef COSIM
        //HAL_DramcInit();
#endif

        rTracer_Mcu0= 0x22;
        rTracer_Mcu3= 0x0;
    }
    
    //HAL_EnableICache(g_ulMcuID);
    //HAL_EnableDCache(g_ulMcuID);

#ifdef GPIO_INTERRRUPT_TEST
    Test_McuGPIOInterrupt();
#endif

#ifdef MCU_TO_MCU_INTERRRUPT_TEST
    Test_McuToMcuInterrupt();
#endif
    return;
}
