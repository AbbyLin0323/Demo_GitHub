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
Filename    :HAL_PM.c
Version     :Ver 1.0
Author      :
Date        :
Description : Routines for operating hardware power management interface.
Others      :
Modify      :
*******************************************************************************/

#include "Basedef.h"
#include "HAL_PM.h"
#include "HAL_Interrupt.h"
#include <xtensa/tie/xt_interrupt.h>

void mcu0entrance(void);

/* The flag for DEVSLP signal assertion by host. */
/* This flag is set in PMU interrupt handler. When host asserts DEVSLP signal, PMU hardware
     would generate an interrupt to notify firmware the event. */

/* Constant pointer for reference to PMU registers. */
LOCAL volatile PMU_CONTROL_REGSET  * const l_pPMUCtrlRegBlk = ((volatile PMU_CONTROL_REGSET *)REG_BASE_PMU);
LOCAL volatile PMU_TIMER_REGSET    * const l_pPMUTimerRegBlk = ((volatile PMU_TIMER_REGSET *)PMU_TIMER_REG_ADDRESS);

/* Timeout flag group for general purpose PMU timers. */
LOCAL U32 l_ulPMUTimeOutFlag[PMU_TIMER_NUMBER];

/****************************************************************************
Name        : HAL_PMInit
Input       : None
Output      : None
Author      :
Date        :
Description : Initializing PMU parameters and registers .
Others      :
Modify      :
****************************************************************************/
void HAL_PMInit(void)
{
    U32 ulLoopIndex;

    // Sets Timer 0~2 to run in normal mode 
    l_pPMUTimerRegBlk->tTimerControl.bsTimer0Mode = PMU_TIMER_MODE_NORMAL;
    l_pPMUTimerRegBlk->tTimerControl.bsTimer1Mode = PMU_TIMER_MODE_NORMAL;
    l_pPMUTimerRegBlk->tIntrControl.bsTimer2Mode = PMU_TIMER_MODE_NORMAL;

    // Clears timeout flag for all timers
    for (ulLoopIndex = 0; ulLoopIndex < PMU_TIMER_NUMBER; ulLoopIndex++)
    {
        HAL_PMClearTimeOutFlag(ulLoopIndex);
    }

    // Enable all interrupts 
    l_pPMUTimerRegBlk->tIntrControl.bsIntrMsk = 0;
    
    // Set Timer 2 feature : PLL & power wake up enable  
    l_pPMUCtrlRegBlk->tMiscControl1.bsTmr2HPLLWakeEn = TRUE;
    l_pPMUCtrlRegBlk->tMiscControl1.bsTmr2MPLLWakeEn = TRUE;
    l_pPMUCtrlRegBlk->tMiscControl1.bsTmr2PwrWakeEn  = TRUE;

    // Sets reset control registers for HOSTC/HCT/etc.
#ifdef VT3514_C0
    l_pPMUCtrlRegBlk->ulHwPLLCfg |= 0x85000000; // Enables hardware reset functions on an out-band PCIE reset but disables them on an in-band PCIE reset.
    l_pPMUCtrlRegBlk->tPCIeControl.bsRSTENDasstEn = TRUE; // Enables PCIe outband reset input.
#endif

    return;
}

/****************************************************************************
Name        :   HAL_PMShutDownPLL
Input       :   None
Output      :   None
Author      :
Date        :
Description :   Shut down all PLL for power saving
Others      :
Modify      :
****************************************************************************/
void HAL_PMShutDownPLL(void)
{
    l_pPMUCtrlRegBlk->ulPowerStateControl = PMU_INITIATE_PLLOFF;
    HAL_MCUWaitForInt();

    return;
}

/****************************************************************************
Name        :HAL_PMInitiateSuspending
Input       : None
Output      : None
Author      :
Date        :
Description : This routine programs hardware registers and attempts to initiate a suspending
                    power saving state.
Others      :
Modify      :
****************************************************************************/
void HAL_PMInitiateSuspending(void)
{
#ifndef MIX_VECTOR
    // 1.Set hot boot enterance
    l_pPMUCtrlRegBlk->ulResumeEntry = (U32)mcu0entrance;
#endif
    // 2.Shut down PLL
    l_pPMUCtrlRegBlk->ulPowerStateControl = (PMU_TURNOFF_CORE_SUSP | PMU_INITIATE_PLLOFF);

    HAL_MCUWaitForInt();

    return;
}

/****************************************************************************
Name        :HAL_PMDisableSuspending
Input       : None
Output      : None
Author      :
Date        :
Description : This routine reverses hardware state back in order to make the system able to
                    execute host commands normally again. It is invoked after an power-saving transition
                    attempt failed.
Others      :
Modify      :
****************************************************************************/
void HAL_PMDisableSuspending(void)
{    
    return;
}

/****************************************************************************
Name        :HAL_PMInitiateHibernating
Input       : None
Output      : None
Author      :
Date        :
Description : This routine programs hardware registers and attempts to initiate a hibernating
                    power saving state.
Others      :
Modify      :
****************************************************************************/
void HAL_PMInitiateHibernating(void)
{
    l_pPMUCtrlRegBlk->ulResumeEntry = 0;
    l_pPMUCtrlRegBlk->ulPowerStateControl = (PMU_TURNOFF_CORE_HBN | PMU_INITIATE_PLLOFF);
    HAL_MCUWaitForInt();

    return;
}

/****************************************************************************
Name        :HAL_PMDisableHibernating
Input       : None
Output      : None
Author      :
Date        :
Description : This routine reverses hardware state back in order to make the system able to
                    execute host commands normally again. It is invoked after an power-saving transition
                    attempt failed.
Others      :
Modify      :
****************************************************************************/
void HAL_PMDisableHibernating(void)
{
    return;
}

/****************************************************************************
Name        :HAL_PMCheckBootType
Input       : None
Output      : The current boot reason:
                A cold start, a resuming from suspending due to waking up from host, and a resuming
                from suspending due to host assertion of DEVSLP.
Author      :
Date        :
Description : This routine inquires hardware register to get the current boot reason.
Others      :
Modify      :
****************************************************************************/
SYSPM_STATE HAL_PMCheckBootType(void)
{
    SYSPM_STATE eBootType;

    if (0 != l_pPMUCtrlRegBlk->ulResumeEntry)
    {
        /* Resuming from suspending branch */
        eBootType = SYSPMSTATE_WARM_START;
    }

    else if (0 != l_pPMUCtrlRegBlk->ulPwrOnSgnt)
    {        
        eBootType = SYSPMSTATE_HIBERNATE_START;
    }

    else
    {
        /* Cold starting branch */
        eBootType = SYSPMSTATE_COLD_START;
    }

    return eBootType;
}

/****************************************************************************
Name        :   HAL_PMSetSignature
Input       :   value to set signature register
Output      :   None
Author      :
Date        :
Description :   Set signature for checking start mode 
Others      :
Modify      :
****************************************************************************/
void HAL_PMSetSignature(U32 ulSig)
{
    l_pPMUCtrlRegBlk->ulPwrOnSgnt = ulSig;

    return;
}

/****************************************************************************
Name        :   HAL_PMSetSignature
Input       :   None
Output      :   value of signature register
Author      :   Victor Zhang
Date        :
Description :   Set signature for checking start mode 
Others      :
Modify      :
****************************************************************************/

U32 HAL_PMGetSignature(void)
{
    return l_pPMUCtrlRegBlk->ulPwrOnSgnt;
}

/****************************************************************************
Name        :HAL_PMDozeOffMCU
Input       : ulSleepTime - The dozing period for MCU allowed. Unit is timer tick.
Output      : None
Author      :
Date        :
Description : This routine utilizes hardware timer to generate an interrupt for waking MCU up
                from dozing state.
Others      :
Modify      :
****************************************************************************/
void HAL_PMDozeOffMCU(U32 ulSleepTime)
{
#ifndef XTMP
    /* 1. Programing hardware timer with given dozing period. */
    HAL_PMStartTimer(PMU_TIMER0,ulSleepTime);

    /* 2. Putting MCU into dozing state. */
    HAL_MCUWaitForInt();
#endif

    return;
}

/****************************************************************************
Name        :HAL_PMGetIntrPendingStatus
Input       :None.
Output      :None.
Author      :Yao Chen
Date        :
Description : This routine acquires the interrupt pending status of PMU.
Others      :
Modify      :
****************************************************************************/
INLINE U8 HAL_PMGetIntrPendingStatus(void)
{
    return l_pPMUTimerRegBlk->tIntrControl.bsIntrSts;
}

/****************************************************************************
Name        :HAL_PMClearIntrPendingStatus
Input       :ucIntSrc - Interrupt to be cleared (bit mapped and a 1 in corresponding bit
                                takes effect.
Output      :None.
Author      :Yao Chen
Date        :
Description : This routine clears the specified interrupt pending status bits of PMU.
Others      :
Modify      :
****************************************************************************/
INLINE void HAL_PMClearIntrPendingStatus(U8 ucIntSrc)
{
    /* Writes one to specified bit to clear interrupt pending status. */
    l_pPMUTimerRegBlk->tIntrControl.bsIntrSts = ucIntSrc;

    return;
}
/****************************************************************************
Name        :HAL_PMIsPCIeInLowPwrSts
Input       :void
Output      : Be or not be under L1 state.
Author      : Victor Zhang
Date        :
Description : Check whether the PCIe link layeris under L1 state.
Others      :
Modify      :
****************************************************************************/

U32 HAL_PMIsPCIeInLowPwrSts(void)
{
#ifndef XTMP
    return (l_pPMUCtrlRegBlk->tMiscControl1.bsPHYinL1State);    
#else
    return TRUE;
#endif 
}

/****************************************************************************
Name        :   HAL_PMClearTimeOutFlag
Input       : U32 ulTimerID - Timer 0 or Timer 1 or Timer 2
Output      :   None
Author      :
Date        :
Description :   Clears time out flag for specified timer
Others      :
Modify      :
****************************************************************************/
void HAL_PMClearTimeOutFlag(U32 ulTimerID)
{
    if (ulTimerID < PMU_TIMER_NUMBER)
    {
        l_ulPMUTimeOutFlag[ulTimerID] = FALSE;
    }

    else
    {
        DBG_Printf("Invalid PMU timer ID!\n");
        DBG_Getch();
    }

    return;
}

/****************************************************************************
Name        :   HAL_PMIsTimerTimedOut
Input       : U32 ulTimerID - Timer 0 or Timer 1 or Timer 2
Output      :   The timeout flag for specified timer
Author      :
Date        :
Description :   Checks whether the specified timer has already overflowed.
Others      :
Modify      :
****************************************************************************/
U32 HAL_PMIsTimerTimedOut(U32 ulTimerID)
{
    U32 ulTimeout;

    if (ulTimerID >= PMU_TIMER_NUMBER)
    {
        DBG_Printf("Invalid PMU timer ID!\n");
        DBG_Getch();
    }

    else
    {
        ulTimeout = l_ulPMUTimeOutFlag[ulTimerID];
    }

    return ulTimeout;
}

/****************************************************************************
Name        :   HAL_PMStartTimer
Input       :   U32 ulTickCount : Specifies time period by clock cycle 
                U32 ulTimerID  Timer 0 or Timer 1 or Timer 2
Output      :   None
Author      :
Date        :
Description :   Start Timer 
Others      :
Modify      :
****************************************************************************/
void HAL_PMStartTimer(U32 ulTimerID ,U32 ulTickCount)
{
    if (PMU_TIMER0 == ulTimerID)
    {
        l_pPMUTimerRegBlk->ulTimer0Counter = 0;
        l_pPMUTimerRegBlk->ulTimer0Period = ulTickCount;
        l_pPMUTimerRegBlk->tTimerControl.bsTimer0Stop = FALSE;
    }

    else if (PMU_TIMER1 == ulTimerID)
    {
        l_pPMUTimerRegBlk->ulTimer1Counter = 0;
        l_pPMUTimerRegBlk->ulTimer1Period = ulTickCount;
        l_pPMUTimerRegBlk->tTimerControl.bsTimer1Stop = FALSE;
    }

    else if (PMU_TIMER2 == ulTimerID)
    {
        l_pPMUTimerRegBlk->ulTimer2Counter = 0;
        l_pPMUTimerRegBlk->ulTimer2Period = ulTickCount;
        l_pPMUTimerRegBlk->tIntrControl.bsTimer2Stop = FALSE;
    }

    else
    {
        DBG_Getch();
    }

    return;
}

/****************************************************************************
Name        :   HAL_PMStopTimer
Input       :   U32 ulTimerID  Timer 0 or Timer 1 or Timer 2
Output      :   None
Author      :
Date        :
Description :   Stop Timer 
Others      :
Modify      :
****************************************************************************/
void HAL_PMStopTimer(U32 ulTimerID)
{
    U8 ucIntrPending = HAL_PMGetIntrPendingStatus();
    if (PMU_TIMER0 == ulTimerID)
    {
        l_pPMUTimerRegBlk->tTimerControl.bsTimer0Stop = TRUE;
        l_pPMUTimerRegBlk->ulTimer0Counter = 0;

        if (0 != (ucIntrPending & PMU_INT_TIMER0))
        {
            HAL_PMClearIntrPendingStatus(PMU_INT_TIMER0);
        }
    }

    else if (PMU_TIMER1 == ulTimerID)
    { 
        l_pPMUTimerRegBlk->tTimerControl.bsTimer1Stop = TRUE;
        l_pPMUTimerRegBlk->ulTimer1Counter = 0;

        if (0 != (ucIntrPending & PMU_INT_TIMER1))
        {
            HAL_PMClearIntrPendingStatus(PMU_INT_TIMER1);
        }        
    }

    else if (PMU_TIMER2 == ulTimerID)
    { 
        l_pPMUTimerRegBlk->tIntrControl.bsTimer2Stop = TRUE;
        l_pPMUTimerRegBlk->ulTimer2Counter = 0;

        if (0 != (ucIntrPending & PMU_INT_TIMER2))
        {
            HAL_PMClearIntrPendingStatus(PMU_INT_TIMER2);
        }        
    }    

    else
    {
        DBG_Getch();
    }

    return;
}

/****************************************************************************
Name        :ISR_PMU
Input       :void
Output      :None.
Author      :Yao Chen
Date        :
Description : Interrupt service routine of PMU
Others      :
Modify      :
****************************************************************************/

GLOBAL void ISR_PMU(void)
{
    U32 ulCPUTickCount;
    U8 ucIntrPending = HAL_PMGetIntrPendingStatus();

    /* Squelch interrupt signaled. */
    if ( 0 != ( ucIntrPending & PMU_INT_SQUELCH ) )
    {
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_SQUELCH);

        return;
    }

    /* DEVSLP assertion interrupt signaled. */
    if ( 0 != ( ucIntrPending & PMU_INT_DEVSLP_ASST) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_DEVSLP_ASST);

        return;
    }

    /* Timer 0 overflow interrupt signaled. */
    if ( 0 != ( ucIntrPending & PMU_INT_TIMER0 ) )
    {
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_TIMER0);

        /* Sets time out flag. */
        l_ulPMUTimeOutFlag[0] = TRUE;

        return;
    }

    /* Timer 1 overflow interrupt signaled. */
    if ( 0 != ( ucIntrPending & PMU_INT_TIMER1 ) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_TIMER1);

        /* Sets time out flag. */
        l_ulPMUTimeOutFlag[1] = TRUE;

        return;
    }
    
    /* Timer 2 overflow interrupt signaled. */
    if ( 0 != ( ucIntrPending & PMU_INT_TIMER2 ) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_TIMER2);

        /* Sets time out flag. */
        l_ulPMUTimeOutFlag[2] = TRUE;

        return;
    }

    /* DEVSLP deassertion interrupt signaled. */
    if ( 0 != ( ucIntrPending & PMU_INT_DEVSLP_DEASST ) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_DEVSLP_DEASST);
    }

    return;
}

