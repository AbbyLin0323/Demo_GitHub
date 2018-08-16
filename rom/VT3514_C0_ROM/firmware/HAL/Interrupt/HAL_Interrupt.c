/*******************************************************************************
Copyright (c) 2014 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :  HAL_Interrupt.c                                     
Version      :  Ver 1.0                                               
Date         :  20080521                                   
Author       :  Sue Wang

Description: 
    Completing DRAM controller initialization for COSIM. 

Modification History:
20080521     Sue Wang       First create
20140902     Victor Zhang   Modification as new coding style
*******************************************************************************/

#ifndef SIM

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
/*#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "Proj_Config.h"
#include "HAL_Interrupt.h"
*/
#include "COM_Inc.h"

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION
------------------------------------------------------------------------------*/

void HAL_L1IntSharedEntry(void);
void HAL_ProcMCU0Int(void);

/*------------------------------------------------------------------------------
    EXTERNAL FUNCTION DECLARATION
------------------------------------------------------------------------------*/
extern U32 HAL_GetMCUIntSrc();   
extern void HAL_SaveMCUExcCause(U32 ulExcCause);
#if defined(HOST_SATA) && defined(MCU0)
extern void L0_SataISR();    
#endif

#ifndef VT3514_C0
#define INT_SET(IntBit)     (rGLB_5E |= IntBit)
#define INT_Clear(IntBit)   (rGLB_5E &= ~IntBit)
#else
#define INT_SET(IntBit)     (rGLB_5E = IntBit)
#define INT_Clear(IntBit)   (rGLB_5E = IntBit)
#endif


#define MCU_INDEX_NUM (4)    //  as MCU2_ID is 3 ,so create a 4x4 array.
U32 l_aSetInt[MCU_INDEX_NUM][MCU_INDEX_NUM] = 
      { 0,0,0,0,
        0,0,INT_MCU0_1_SET,INT_MCU0_2_SET,
        0,INT_MCU1_0_SET,0,INT_MCU1_2_SET,
        0,INT_MCU2_0_SET,INT_MCU2_1_SET,0};
                        
U32 l_aClearInt[MCU_INDEX_NUM][MCU_INDEX_NUM] = 
      { 0,0,0,0,
        0,0,INT_MCU0_1_CLEAN,INT_MCU0_2_CLEAN,
        0,INT_MCU1_0_CLEAN,0,INT_MCU1_2_CLEAN,
        0,INT_MCU2_0_CLEAN,INT_MCU2_1_CLEAN,0};




/*------------------------------------------------------------------------------
Function   :     HAL_UserExcpEntry
Input      :     None
Output     :     None
Description : 
    User exception handler routine entry (used by Level-1 interrupts as well).
Note: 
Modify History:
    20080521     Sue Wang       First create
    20140905     Victor Zhang   Replace HAL_SaveMCUExcCause() to asm()  
------------------------------------------------------------------------------*/
void HAL_UserExcpEntry(void)
{
    U32 ulExcCause;

    ulExcCause = HAL_GetMCUExcCause();

    switch (ulExcCause)
    {
        /* The exception cause is an external interrupt of level 1. */
        case EXCCAUSE_LEVEL1_INTERRUPT:
            HAL_L1IntSharedEntry();
            break;

        /* The exception is raised by a severe error. */
        default:
            /* Just records exception cause and source instruction address at the start of OTFB and hangs. */
            HAL_SaveMCUExcCause(ulExcCause);
            break;
    }

    return;
}

/*------------------------------------------------------------------------------
Function   :     HAL_L1IntSharedEntry
Input      :     None
Output     :     None
Description : 
    Public interrupt handler routine entry shared by all interrupts of Level 1.
Note: 
Modify History:
    20140905    Victor Zhang Create
------------------------------------------------------------------------------*/
void HAL_L1IntSharedEntry(void)
{
    U32 ulIntSrc = HAL_GetMCUIntSrc();

    /* MCU0 uses an inter-processor interrupt to wake a subsystem up
     since the subsystem MCU would sleep in WAITI state when it 
     completes one stage of IDLE task. */
    if (ulIntSrc & BIT_ORINT_HOSTC)
    {   
        HAL_HostCIntEntry();
    }
    return;
}
    
/*------------------------------------------------------------------------------
Function:   HAL_L4TimerIntEntry
Input:  None
Output: None
Description : 
    Interrupt handler routine entry for Xtensa TIE Timer 1 (at Level 4)
Note:
Modify History:
    20140905    Victor Zhang Create
------------------------------------------------------------------------------*/
void HAL_L4TimerIntEntry(void)
{
    return;
}

/*------------------------------------------------------------------------------
Function   :     HAL_SATAIntEntry
Input      :     None
Output     :     None
Description : 
    Interrupt handler routine entry for SATA controller
Note: 
Modify History:
    20140905    Victor Zhang Create
------------------------------------------------------------------------------*/
void HAL_SATAIntEntry(void)
{
//#if defined(HOST_SATA) && defined(MCU0)
    HAL_SataISR();
//#endif
    return;
}

/*------------------------------------------------------------------------------
Function   :     HAL_FlashIntEntry
Input      :     None
Output     :     None
Description : 
    Interrupt handler routine entry for NAND flash
Note: 
Modify History:
    20140905    Victor Zhang Create
------------------------------------------------------------------------------*/
void HAL_FlashIntEntry(void)
{
    return;
}

/*------------------------------------------------------------------------------
Function   :     HAL_InitInterrupt
Input      :     ulHwIntMask : Interrupt Mask
                 ulMcuIntSrc : Interrupt Enable
Output     :     None
Description : 
    Initializes and enables MCU interrupt handling at all interrupt disabled state.

eg. old version
    HAL_ClearHwIntMask(ulMCUID, (TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0));
    HAL_EnableMCUIntSrc(BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0);

    new version

    ulHwIntMask =  TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0;
    ulMcuIntSrc =  BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0;
    HAL_InitInterrupt(ulHwIntMask,ulMcuIntSrc);

Note: 
Modify History:
    20140905    Victor Zhang Create
    20141112    Victor Zhang
                Add two arguments ,making each MCU initialize interrupt independently.
------------------------------------------------------------------------------*/
void HAL_InitInterrupt(U32 ulHwIntMask,U32 ulMcuIntSrc)
{
    U32 ulMCUID = HAL_GetMcuId();

    HAL_DisableMCUIntAck();
    HAL_ClearHwIntMask(ulMCUID, ulHwIntMask);
    HAL_EnableMCUIntSrc(ulMcuIntSrc);
    HAL_EnableMCUIntAck();

    return;
}
/*
void HAL_InitInterrupt(void)
{
    U32 ulMCUID = HAL_GetMcuId();

    HAL_ClearHwIntMask(ulMCUID, (TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0));
    HAL_EnableMCUIntSrc(BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0);

    HAL_EnableMCUIntAck();

    return;
}
*/
/*------------------------------------------------------------------------------
Function   :     HAL_ClearHwIntMask
Input      :     ulCPUID - The ID of subsystem MCU (MCU1 or MCU2);
    ulIntSrc - The interrupt sources map to be unmasked.
Output     :     None
Description : 
    Configures chip top registers to enable per-source level external interrupt
    handling.
Note: 
Modify History:
    20140905    Victor Zhang Create
------------------------------------------------------------------------------*/
void HAL_ClearHwIntMask(U32 ulCPUID, U32 ulIntSrc)
{
    if ( MCU0_ID == ulCPUID )
    {
        rGLB_58 &= (U16)(~ulIntSrc);
    }
    else if ( MCU1_ID == ulCPUID )
    {
        rGLB_5A &= (U16)(~ulIntSrc);
    }
    else if (MCU2_ID == ulCPUID)
    {
        rGLB_5C &= (U16)(~ulIntSrc);
    }

    return;
}

/*===================================================================
Function   :     HAL_AssertIPI
Input      :     ucReqMcuId  the MCU ID of which request a interrupt
                 ucRespMcuId the MCU ID of which response a interrupt
                 bSetInt   TRUE for set interrupt ,FALSE for clear interrupt
Output     :     None
Description : 
        Programs top register to raise an inter-processor interrupt request from 
        Req MCU to Resp MCU.
        eg.
          1 HAL_AssertIPI(MCU1_ID,MCU0_ID,TRUE);

            That means MCU1 raised a interrupt to MUC0,MCU1 set interrupt.

          2 HAL_AssertIPI(MCU1_ID,MCU0_ID,FALSE);

            That means MCU0 received a interrupt from MCU1,MCU0 clear interrupt.
        
Note: 
Modify History:

===================================================================*/
void HAL_AssertIPI(U8 ucReqMcuId,U8 ucRespMcuId,BOOL bSetInt)
{    
    // Set int 
    if (TRUE == bSetInt)
    {
        INT_SET(l_aSetInt[ucReqMcuId][ucRespMcuId]);
    }
    // Clear int
    else
    {
        INT_Clear(l_aSetInt[ucReqMcuId][ucRespMcuId]);
    }
    return;
}
#endif//END #ifndef SIM


