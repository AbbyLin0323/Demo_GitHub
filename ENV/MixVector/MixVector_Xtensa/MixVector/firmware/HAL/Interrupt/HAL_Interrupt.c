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

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_Interrupt.h"
#include "HAL_MultiCore.h"
#ifndef SIM
#include "HAL_HCmdTimer.h"
#endif

/*------------------------------------------------------------------------------
    LOCAL FUNCTION DECLARATION
------------------------------------------------------------------------------*/
void HAL_L1IntSharedEntry(void);
void HAL_ProcMCU0Int(void);
void HAL_HostCIntEntry(void);

#ifdef MCU0
void HAL_HostCIntInit(void);
#else
void HAL_IPIIntEntry(void);
#endif

/*------------------------------------------------------------------------------
    EXTERNAL FUNCTION DECLARATION
------------------------------------------------------------------------------*/
#if defined(HOST_SATA) && defined(MCU0)
extern void L0_SataISR(void);
#endif

#if defined(HOST_AHCI) && defined(MCU0)
extern U32 L0_AHCISetExtEvent(U32 ulIntPending);
#endif

#if defined(HOST_NVME) && defined(MCU0)
extern void L0_NVMeISR(void);
#endif

#ifndef MCU0
extern void FW_ChkNtfnMsg(void);
#else
extern void ISR_PMU(void);
#endif

#ifndef VT3514_C0
#define INT_SET(IntBit)     (rGlbIPIIntTrig |= IntBit)
#define INT_Clear(IntBit)   (rGlbIPIIntTrig &= ~IntBit)
#else
#define INT_SET(IntBit)     (rGlbIPIIntTrig = IntBit)
#define INT_Clear(IntBit)   (rGlbIPIIntTrig = IntBit)
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

#if 0
/*------------------------------------------------------------------------------
Function   :     HAL_ProcMCU0Int
Input      :     None
Output     :     None
Description : 
    The handler for the inter-MCU interrupt from MCU0.
Note: 
Modify History:
    20080521     Sue Wang       First create
------------------------------------------------------------------------------*/

void HAL_ProcMCU0Int(void)
{
    /* Since now the interrupt is only used for waking a subsystem
     MCU up from the WAITI state, nothing needs to be handled except
     clearing interrupt pending status. */
    U32 ulMCUID;
    U8 ucIPIPending;

    /* 1. Confirming an inter-MCU interrupt is pending. */
    ucIPIPending = rGlbIPIIntTrig;
    ASSERT(0 != (ucIPIPending & (INT_MCU0_1 | INT_MCU0_2)));

    ulMCUID = HAL_GetMcuId();

    /* 2. Attempts to acquiring the spin lock. */
    if (TRUE == HAL_MultiCoreGetSpinLock(SPINLOCKID_SUBSYS_INTERMCUINTR))
    {
        /* 3. Clearing the interrupt pending bit in read-modify-write manner. */
        if (MCU1_ID == ulMCUID)
        {
            ucIPIPending &= ~(INT_MCU0_1);
        }

        else
        {
            ucIPIPending &= ~(INT_MCU0_2);
        }

        rGlbIPIIntTrig = ucIPIPending;

        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SUBSYS_INTERMCUINTR);
    }

    return;
}
#endif 

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
    if (0 != (ulIntSrc & BIT_ORINT_HOSTC))
    {
        HAL_HostCIntEntry();
    }

#ifndef MCU0
    /* Only MCU1/MCU2 processes inter-processor interrupts which come from MCU0. */
    if (0 != (ulIntSrc & (BIT_ORINT_MCU0_1 | BIT_ORINT_MCU0_2)))
    {
        HAL_IPIIntEntry();
    }

#else
    /* Only MCU0 processes interrupt request from PMU. */
    if (0 != (ulIntSrc & BIT_ORINT_PMU))
    {
        ISR_PMU();
    }
#endif

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
#if defined(HOST_SATA) && defined(MCU0)
    L0_SataISR();    
#endif
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

void HAL_HostCIntInit(void)
{
#if defined (HOST_NVME)
    rHOSTC_INTMASK &=
        ~(INT_HOSTC_PERST_DAST | INT_HOSTC_NVME_SHUTDOWN | INT_HOSTC_NVME_CMDEN);
#elif defined (HOST_AHCI)
#ifdef VT3514_C0
    rHOSTC_INTMASK &=
        ~(INT_HOSTC_PERST_DAST | INT_HOSTC_AHCI_HRST | INT_HOSTC_AHCI_CMDSTART | INT_HOSTC_AHCI_CMDSTOP | INT_HOSTC_AHCI_SCTL);
#else
    rHOSTC_INTMASK &=
        ~(INT_HOSTC_AHCI_HRST | INT_HOSTC_AHCI_CMDSTART | INT_HOSTC_AHCI_CMDSTOP);
#endif
#endif

    return;
}

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
        rGlbIntMsk0 &= (U16)(~ulIntSrc);
    }
    else if ( MCU1_ID == ulCPUID )
    {
        rGlbIntMsk1 &= (U16)(~ulIntSrc);
    }
    else if (MCU2_ID == ulCPUID)
    {
        rGlbIntMsk2 &= (U16)(~ulIntSrc);
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
        INT_Clear(l_aClearInt[ucReqMcuId][ucRespMcuId]);
    }
    return;
}

void HAL_HostCIntEntry(void)
{
    U32 ulHostCIntPending, ulHostCIntClear;

    ulHostCIntPending = rHOSTC_INTPENDING;

#if defined (HOST_AHCI) && defined (MCU0)
    ulHostCIntClear = L0_AHCISetExtEvent(ulHostCIntPending);
#else
    ulHostCIntClear = 0;
#endif

#if defined (HOST_NVME) && defined (MCU0)
#if 0
    /* NVMe: Host clears CC.EN bit */
    if (0 != (ulHostCIntPending & INT_HOSTC_NVME_CMDEN))
    {
        /* Clears interrupt pending status */
        rHOSTC_INTPENDING = INT_HOSTC_NVME_CMDEN;
    }

    /* NVMe: Host changes CC.SHN field */
    if (0 != (ulHostCIntPending & INT_HOSTC_NVME_SHUTDOWN))
    {
        /* Clears interrupt pending status */
        rHOSTC_INTPENDING = INT_HOSTC_NVME_SHUTDOWN;
    }
#endif
    L0_NVMeISR();
#endif

#ifndef SIM
    /* Common: Timer overflows. */
    if (0 != (ulHostCIntPending & INT_HOSTC_TIMER))
    {
        HAL_HCmdTimerInterruptEntry();

        ulHostCIntClear |= INT_HOSTC_TIMER;
    }
#endif

    /* Clears interrupt pending status */
    rHOSTC_INTPENDING = ulHostCIntClear;

    return;
}

#ifndef MCU0
void HAL_IPIIntEntry(void)
{
    U32 ulMCUId = HAL_GetMcuId();

#ifndef MIX_VECTOR
    FW_ChkNtfnMsg();
#endif

    HAL_AssertIPI(MCU0_ID, ulMCUId, FALSE);

    return;
}
#endif


