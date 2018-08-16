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
#include "HAL_GLBReg.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#include "HAL_PM.h"
#include "HAL_Interrupt.h"
#include <xtensa/tie/xt_interrupt.h>
#ifdef HOST_SATA
#include "HAL_SataIO.h"
#include "L0_Event.h"
#endif

#ifdef HOST_NVME
void L0_NVMePcieResetISR(void);
GLOBAL BOOL bDPLLGating;
#ifdef AF_ENABLE
#include "L0_NVMe.h"
extern HCT_CONTROL_REG *g_pHCTControlReg;
#endif
#endif


void mcu0warmbootentrance(void);
LOCAL U32 HAL_PMGetIntrPendingStatus(void);
INLINE void HAL_PMClearIntrPendingStatus(U32 ulIntSrc);
LOCAL U32 HAL_PMIsDDRinSelfRef(void);

#ifdef CALC_RINGOSC_CLK
LOCAL void PMCalibrateTimer(void);
#endif

/* The flag for DEVSLP signal assertion by host. */
/* This flag is set in PMU interrupt handler. When host asserts DEVSLP signal, PMU hardware
     would generate an interrupt to notify firmware the event. */

/* Constant pointer for reference to PMU registers. */
LOCAL volatile PMUREGSET *l_pPMURegBlk;

/* Tick count for 1 millisecond of current PMU timers. */
#ifdef CALC_RINGOSC_CLK
LOCAL U32 l_ulPMUTimerTickCountPerMS;
#endif

/* Flag indicating PCIe has already been configured to get ready for L1.2 entry. */
#ifdef HOST_NVME
LOCAL U32 l_ulPCIEL12ReadyFlag;
#endif

/* Flag indicating PCIe has squelch. */
#ifdef PCIE_ASPM_MANAGEMENT
LOCAL U32 l_ulSquelchFlag;
#endif

/* Timeout flag group for general purpose PMU timers. */
LOCAL volatile U32 l_ulPMUTimeOutFlag[PMU_TIMER_NUMBER];

LOCAL volatile U32 l_ulPMUDevSleepFlag;

/****************************************************************************
Name        : HAL_PMInit
Input       : None
Output      : None
Author      :
Date        :
Description : Initializing PMU parameters and registers on each cold boot. Initializing PCIe reset
                   related functions for a PCIe solution.
Others      :
Modify      :
****************************************************************************/
void HAL_PMInit(void)
{
    U32 ulLoopIndex;

    /* 1. First initializes PCIE reset related logic. */
#ifndef HOST_SATA
    /* 1) Waits until PCIE reset signal is released. */

    /*get UARTMP mode, if high, uart mp mode*/
    if (HAL_UartIsMp() == FALSE) 
    {    
        while (0 == (rHOSTC(0x4) & (1 << 31)))
        {
            ;
        }
    }

    /* 2) Allows hardware to reset PCIE controller logic
    on an out-band PCIE reset, but not on an in-band reset. */
    //l_pPMURegBlk->tPCIeResetCfg.bsObdRstDnstrmEn = TRUE;
    //l_pPMURegBlk->tPCIeResetCfg.bsIbdRstDnstrmEn = TRUE;
    //l_pPMURegBlk->tPCIeResetCfg.bsObdRstHOSTCEn = TRUE;
    //l_pPMURegBlk->tPCIeResetCfg.bsIbdRstHOSTCEn = TRUE;
    //l_pPMURegBlk->tPCIeResetCfg.bsObdRstHCTEn = TRUE;
    //l_pPMURegBlk->tPCIeResetCfg.bsIbdRstHCTEn = TRUE;
    l_pPMURegBlk->ulPCIeResetCfg |= 0xCF;
    /*get UARTMP mode, if high, uart mp mode*/    
    if (HAL_UartIsMp() == FALSE) 
    {
        /* Enables link down event to reset PCIe IRS. */
        rPCIe(0x208) |= ((1 << 10) | (1 << 11));
    }
    /* 3) Clears interrupt pending status for
    the first PCIE reset after power on. We should
    just ignore it. */
    rHOSTC_INTPENDING = (INT_HOSTC_PERST_DAST | INT_HOSTC_PERST_INBD);

    /* 4) Enables PCIe outband reset input. */
    l_pPMURegBlk->tPCIeResetCfg.bsObdRstPinEn = TRUE;

    /* 5) Asserts EP_INIT to avoid host reading incorrect value for configuration space registers. */
    /* Trying to move the waiting to boot loader script initialization. */
    //L0_NVMeCfgLock();

    /* 6) Sets PCIe Band Gap and TPLL/MPLL control. */
    //HAL_MCUInsertbits(rPCIe(0x400), 0x3, 0, 1);//bit0 BGP always on, bit1 TPLL/MPLL always on

    /* 7) Allows PCIe module to control CLKREQ# release. */
    HAL_PMEnableHWL12Ctrl(TRUE);

    /* 8) Sets PCIe retention timer. */
    l_pPMURegBlk->tRttnTmrCfg.bsTmrCntVal = INVALID_2F;

    /* 9) Sets MPLL ready signal not delay. */
    /* Yao: Currently we cannot get our controller correctly enumerated by host with this setting. */
    //l_pPMURegBlk->tGlobalControl.bsMPLLStbDlySel = FALSE;

    /* 10) The power of BGP/REFCLK slicer is controlled by PSO_EN, and would be turned off in L1.2 power-off state. */
    l_pPMURegBlk->tPCIePHYCfg.bsBGPSlcPwrOffEn = TRUE;

    /* 11) Reduces power up delay for asserting RST_ and deasserting ISO_EN. */
    l_pPMURegBlk->tHwPwrOnOffCfg.bsPwrUpTmr_RST = 1;
    l_pPMURegBlk->tHwPwrOnOffCfg.bsPwrUpTmr_ISOEN = 1;

    /* 12) Auto clear CLKREQ# release enable by negedge of CLKREQ# or by L1 exit,
        already moved to hardware initialization script. */
    //rPCIe(0x400) |= (0x3 << 3); 

    /* 13) Sets CLKREQ Filtter 2us*/
    l_pPMURegBlk->tNPLLFreqCfg.bsCLKREQFltr = 3;

    /* 14) Block CLKREQ toggle to PCIe after Power down until Power resume and stable */
    l_pPMURegBlk->tNPLLFreqCfg.bsBlkCLKREQinPwrOffSeq = TRUE;

    /* 15) Block Power down if CLKREQ negedge came*/
    l_pPMURegBlk->tNPLLFreqCfg.bsSftyPrtOnCLKREQ = TRUE;
#endif

    /* 2. Then initializes PMU timers. */
    /* 1) Sets Timer 0~2 to run in normal mode. */
    l_pPMURegBlk->tTimer0Control.bsModeSel = PMU_TIMER_MODE_NORMAL;
    l_pPMURegBlk->tTimer1Control.bsModeSel = PMU_TIMER_MODE_NORMAL;
    l_pPMURegBlk->tTimer2Control.bsModeSel = PMU_TIMER_MODE_NORMAL;

    /* 2) Calculates timer clock with Timer 1. */
#ifdef CALC_RINGOSC_CLK
    PMCalibrateTimer();
#endif

    /* 3) Clears timeout flag for all timers. */
    for (ulLoopIndex = 0; ulLoopIndex < PMU_TIMER_NUMBER; ulLoopIndex++)
    {
        HAL_PMClearTimeOutFlag(ulLoopIndex);
    }

    // Sets Timer 2 feature : PLL & power wake up enable
    //l_pPMURegBlk->tTimer2Control.bsCorePwrRstoEn = TRUE;
    //l_pPMURegBlk->tTimer2Control.bsHPLLRsmEn = TRUE;
    //l_pPMURegBlk->tTimer2Control.bsMPLLRsmEn = TRUE;

    /* 3. Enables automatical self-refresh for both banks of DDR SDRAM. */
    rDRAMC(0x28) |= ((1 << 11) | (1 << 12));

    /* 4. Initializes features for light/deep sleep state. */
    if (0 == HAL_PMGetSignature())
    {
        HAL_PMSetSignature(0x3533);
    }

    if (FALSE == l_pPMURegBlk->tGlobalControl.bsSATAReloadEn)
    {
        l_pPMURegBlk->tGlobalControl.bsSATAReloadEn = TRUE;
    }

#ifndef MIX_VECTOR
    /* Sets warm boot entrance. */
    l_pPMURegBlk->ulResumeEntry = (U32)mcu0warmbootentrance;
#endif

#ifdef HOST_NVME
    /* Isolated bsMPLLFltEn = TRUE for SATA GEN2 Tx issue (Resume from Slumber). */
    l_pPMURegBlk->tPLLGatingCfg.bsMPLLFltEn = TRUE;
#endif

#ifdef HOST_SATA
     l_pPMURegBlk->tHwPwrOnOffCfg.bsPwrUpTmr_RST = 0x9;
     l_pPMURegBlk->tHwPwrOnOffCfg.bsPwrUpTmr_ISOEN = 0x8;
#endif

    // Enables lower voltage in light sleep state.
    //l_pPMURegBlk->tHwPwrOnOffCfg.bsLowSlpVlgEn = TRUE;

    // Enables shutting off power in other power domain when shutting down core power in deep sleep.
    //l_pPMURegBlk->tGlobalControl.bsRingOSCEn = TRUE;
    l_pPMURegBlk->tHwPwrOnOffCfg.bsPwrDnAccEn = TRUE;

    /* 5. Initializes deep-sleep detecting flag (SATA DEVSLP or PCIe L1.2). */
    HAL_PMClearDevSleepFlag();

    /* 7. Enable all interrupts */
    /* In chip bring-up stage we do not need to process events from
           SUS domain PCIe reset, temperature sensor, or power detector. */
    l_pPMURegBlk->tIntrControl.bsIntrMsk =
        (PMU_INT_PCIE_IBDRST | PMU_INT_PCIE_OBDRST | PMU_INT_CHIP_OVERHEAT
#ifdef HOST_NVME
            | PMU_INT_DEVSLP_DEASST | PMU_INT_DEVSLP_ASST
#endif
#ifndef PWR_LOSS_WRITE_PROTECT
            | PMU_INT_CHIP_PWRFAILURE
#endif
        );

    return;
}

/****************************************************************************
Name        : HAL_PMIsDDRinSelfRef
Input       : None
Output      : TRUE - DDR is in self-refresh status;
              FALSE - DDR is not in self-refresh status.
Author      :
Date        :
Description : Checks whether DDR SDRAM is already in self-refresh
              status currently.
Others      :
Modify      :
****************************************************************************/
LOCAL U32 HAL_PMIsDDRinSelfRef(void)
{
    return (U32)(rDRAMCByte(0x11) & 1);
}

#ifdef HOST_NVME
INLINE void HAL_PMEnablePCIeASPM(U32 ulEnableOrDisable)
{
    U32 ulHWASPMTimerValue;

    if (FALSE == ulEnableOrDisable)
    {
        ulHWASPMTimerValue = INVALID_2F;
    }

    else
    {
        ulHWASPMTimerValue = 0x50;
    }

    rPCIeByte(0x1D3) = (U8)ulHWASPMTimerValue;
    rPCIeByte(0x1D3) = (U8)ulHWASPMTimerValue;
    rPCIeByte(0x1D3) = (U8)ulHWASPMTimerValue;

    return;
}

/****************************************************************************
Name        : HAL_PMEnablePCIeL12Ready
Input       : None
Output      : None.
Author      :
Date        :
Description : Configures PCIe to get ready for L1.2 entry.
Others      :
Modify      :
****************************************************************************/
void HAL_PMEnablePCIeL12Ready(void)
{
    /* 1. Stops hardware auto-fetch. */
#ifdef AF_ENABLE
    STOP_AF();
#endif

    /* 2. Clears all interrupt pending status in HOSTC. */
    rHOSTC_INTPENDING = INVALID_8F;

    /* 3. Masks LTR state machine temporarily. */
    rEXT(0x300) |= (1 << 3);

    /* 4. Enables CLKREQ# signal release. */
    l_pPMURegBlk->tGlobalControl.bsDevSlpPinEn = TRUE;
    rPCIe(0x450) |= (1 << 12);

    /* 5. Sets PCIe L1.2 ready flag. */
    l_ulPCIEL12ReadyFlag = TRUE;

    return;
}

/****************************************************************************
Name        : HAL_PMDisablePCIeL12Ready
Input       : None
Output      : None.
Author      :
Date        :
Description : Configures PCIe to resume normal operation.
Others      :
Modify      :
****************************************************************************/
void HAL_PMDisablePCIeL12Ready(void)
{
    if (TRUE == l_ulPCIEL12ReadyFlag)
    {
        /* 1. Disables CLKREQ# signal release ASAP. */
        //rPCIe(0x450) &= ~(1 << 12);
        l_pPMURegBlk->tGlobalControl.bsDevSlpPinEn = FALSE;

        /* 2. Re-enables LTR state machine. */
        rEXT(0x300) &= ~(1 << 3);

        /* 3. Resumes hardware auto-fetch. */
#ifdef AF_ENABLE
        START_AF();
#endif

        /* 4. Clears PCIe L1.2 ready flag. */
        l_ulPCIEL12ReadyFlag = FALSE;

        /* 5. Disable PCIe ASPM L1 as needed. */
        HAL_PMEnablePCIeASPM(FALSE);
    }

    return;
}
#endif

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
    if (TRUE == HAL_PMIsDDRinSelfRef())
    {
#ifndef HOST_SATA
        /* EPHY designer recommends not to shut down MPLL and TPLL in L1,
                   so we may select L1 clock gating instead. */
        /*
                l_pPMURegBlk->tPCIePHYCfg.bsWkupBySQDETEn = TRUE;
                l_pPMURegBlk->tPCIePHYCfg.bsPCIeL1ClkGtgEn = TRUE;
                l_pPMURegBlk->tPCIePHYCfg.bsMPLLPwrOffEn = TRUE;
                l_pPMURegBlk->tPCIePHYCfg.bsTPLLPwrOffEn = TRUE;
            */
        //l_pPMURegBlk->ulPCIePHYCfg |= ((1 << 3) | (1 << 8)); // (WkupBySQDETEn | PCIeL1ClkGtgEn)
        //l_pPMURegBlk->ulPCIePHYCfg |= ((1 << 3) | (1 << 9)); // (bsWkupBySQDETEn | bsMPLLPwrOffEn)

        l_pPMURegBlk->ulPCIePHYCfg |= ((1 << 3) | (1 << 9) | (1 << 10)); // (bsWkupBySQDETEn | bsMPLLPwrOffEn | bsTPLLPwrOffEn)
        HAL_DelayCycle(10);
#endif

       l_pPMURegBlk->ulPowerStatusAndControl = PMU_INITIATE_PLLOFF; //(PMU_DPLL_OFF_BIT | PMU_NPLL_OFF_BIT | PMU_HPLL_OFF_BIT);//PMU_INITIATE_PLLOFF; //PMU_INITIATE_LITSLP

       /* Shuts down IO DET for NFC. DRAMC IO DET would be shut down by hardware when DRAM enters self-refresh. */
       l_pPMURegBlk->ulGlobalControl |= (1 << 23);

        /* Allows MCU0 to sleep. And re-enables interrupt. */
        HAL_MCUWaitForInt();
    }

    else
    {
        /* When control goes here we must re-enables interrupt. */
        HAL_EnableMCUIntAck();
    }

    return;
}

/****************************************************************************
Name        :   HAL_PMGetSafetyBlock
Input       :   None.
Output      :   Current safety block value in hardware.
Author      :
Date        :
Description :   Reads current register value.
Others      :
Modify      :
****************************************************************************/
U32 HAL_PMGetSafetyBlock(void)
{
    U32 ulCurrentSafetyBlock =
        l_pPMURegBlk->tGlobalControl.bsPwrCntlSftyPrt;

    return ulCurrentSafetyBlock;
}

/****************************************************************************
Name        :   HAL_PMClearSafetyBlock
Input       :   None.
Output      :   None.
Author      :
Date        :
Description :   Clears safety block register in hardware.
Others      :
Modify      :
****************************************************************************/
void HAL_PMClearSafetyBlock(void)
{
    l_pPMURegBlk->tGlobalControl.bsPwrCntlSftyPrt = FALSE;
    return;
}

/****************************************************************************
Name        :   HAL_PMSetSafetyBlock
Input       :   None.
Output      :   None.
Author      :
Date        :
Description :   Sets safety block register in hardware.
Others      :
Modify      :
****************************************************************************/
void HAL_PMSetSafetyBlock(void)
{
    l_pPMURegBlk->tGlobalControl.bsPwrCntlSftyPrt = TRUE;
    return;
}

/****************************************************************************
Name        :HAL_PMInitiateSuspending
Input       : None
Output      : None
Author      :
Date        :
Description : This routine programs hardware registers and attempts to initiate a power-saving
                   state in which the power of DRAM retains while core power is cut down.
Others      :
Modify      :
****************************************************************************/
void HAL_PMInitiateSuspending(void)
{
#ifdef HOST_NVME
    U32 ulPCIePLLRdy;
#endif

    HAL_DisableMCUIntAck();
    HAL_PMClearDevSleepFlag();

#ifdef HOST_NVME
    HAL_NVMeClearL12IdleFlag();

    /* 1. Enables L1 clock gating of PCIe module. */
    rGlbClkCtrl &= ~PMU_L1CKG_PCIE;

    /* 2. Shut down core power. */
    l_pPMURegBlk->ulPowerStatusAndControl = PMU_PCIE_DPSLP_BIT;

    /* 3. Re-enables interrupt and allows MCU0 to sleep. */
    HAL_MCUWaitForInt();

    /* 4. When woken up by PMU interrupt, which reflects that host reasserted
    CLKREQ#, we need to patch a hardware issue by waiting for TPLL/MPLL stable. */
    for (ulPCIePLLRdy = 0; ulPCIePLLRdy < 5;)
    {
        /* Debounce here */
        if((0x3 << 24) == (rPCIe(0x670) & (0x3 << 24)))
        {
            ulPCIePLLRdy++;
        }
        else
        {
            ulPCIePLLRdy = 0;
        }
    }
    
    /* Control would NEVER go here if power off succeeded. */
    /* 5. Disables L1 clock gating of PCIe module if power off failed.*/
    /* Control would NEVER go here if power off succeeded. */
    rGlbClkCtrl |= PMU_L1CKG_PCIE;

#else
    HAL_PMClearDevSleepFlag();

    /* Disable power detect. */
    l_pPMURegBlk->ulPwrDetrCfg = 0x0;

    /* Shut down PLL and power. */
    l_pPMURegBlk->ulPowerStatusAndControl = PMU_INITIATE_DPSLP;

    /*  Re-enables interrupt and allows MCU0 to sleep. */
    HAL_MCUWaitForInt();

    /* Enable power detect (RST# + PWREN1 at 2.7). */
    l_pPMURegBlk->ulPwrDetrCfg = 0x1D0;
    HAL_DelayCycle(30);
    l_pPMURegBlk->ulPwrDetrCfg = 0x1DD;

#endif

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
Description : This routine programs hardware registers and attempts to initiate a power-saving
                   state in which both DRAM power and core power are cut down.
Others      :
Modify      :
****************************************************************************/
void HAL_PMInitiateHibernating(void)
{
    HAL_DisableMCUIntAck();
    l_pPMURegBlk->ulResumeEntry = 0;
    l_pPMURegBlk->ulPowerStatusAndControl = PMU_INITIATE_DPSLP;
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
Output      : The current boot type:
                1. Cold start; 2. Resuming from suspending; 3. Resuming rom hibernating.
Author      :
Date        :
Description : This routine should be invoked on boot time. It assignes correct PMU register
                   block base to our global pointer, and inquires hardware register to get the
                   current boot type.
Others      :
Modify      :
****************************************************************************/
SYSPM_STATE HAL_PMCheckBootType(void)
{
    SYSPM_STATE eBootType;

    /* Assigns PMU register block base address to local pointers. */
    l_pPMURegBlk = ((volatile PMUREGSET *)REG_BASE_PMU);

    if (0 != l_pPMURegBlk->ulResumeEntry)
    {
        /* Resuming from suspending branch */
        eBootType = SYSPMSTATE_WARM_START;
#ifdef HOST_NVME
        l_ulPCIEL12ReadyFlag = TRUE;
#endif
    }

    else if (0 != l_pPMURegBlk->ulPwrOnSgnt)
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
Description :   Sets the power-on signature to mark a different boot type on next warm start.
Others      :
Modify      :
****************************************************************************/
void HAL_PMSetSignature(U32 ulSig)
{
    l_pPMURegBlk->ulPwrOnSgnt = ulSig;

    return;
}

/****************************************************************************
Name        :   HAL_PMGetSignature
Input       :   None
Output      :   value of signature register
Author      :   Victor Zhang
Date        :
Description :   Gets signature recorded in PMU hardware before.
Others      :
Modify      :
****************************************************************************/

U32 HAL_PMGetSignature(void)
{
    return l_pPMURegBlk->ulPwrOnSgnt;
}

/****************************************************************************
Name        :HAL_PMDozeOffMCU
Input       : ulSleepTime - The dozing period for MCU allowed. Unit is timer tick.
Output      : None
Author      :
Date        :
Description : This routine sets a specified period for hardware Timer 0 to generate an interrupt
                   for waking MCU up, and then put MCU to the dozing state (for saving power).
Others      :
Modify      :
****************************************************************************/
void HAL_PMDozeOffMCU(U32 ulSleepTime)
{
#ifndef XTMP
    /* 1. Programing hardware timer with given dozing period. */
    HAL_PMStartTimer(PMU_TIMER1, ulSleepTime);

    /* 2. Putting MCU into dozing state. */
    HAL_MCUWaitForInt();
    HAL_PMStopTimer(PMU_TIMER1);
    HAL_PMClearTimeOutFlag(PMU_TIMER1);
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
LOCAL INLINE U32 HAL_PMGetIntrPendingStatus(void)
{
    return (U32)(l_pPMURegBlk->tIntrControl.bsIntrSts &
        ~(l_pPMURegBlk->tIntrControl.bsIntrMsk));
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
INLINE void HAL_PMClearIntrPendingStatus(U32 ulIntSrc)
{
    /* Writes one to specified bit to clear interrupt pending status. */
    l_pPMURegBlk->tIntrControl.bsIntrSts = (U16)ulIntSrc;

    return;
}

/****************************************************************************
Name        :HAL_PMIsPCIeInLowPwrSts
Input       :void
Output      : Be or not be under L1 state.
Author      : Victor Zhang
Date        :
Description : Checks whether the PCIe link is in L1 power-saving state.
Others      :
Modify      :
****************************************************************************/
U32 HAL_PMIsPCIeInLowPwrSts(void)
{
#ifndef XTMP
    U32 ulL1State;

    /* Added LTR message state machine check. */
    if ((TRUE == l_pPMURegBlk->tPCIePHYCfg.bsPHYinL1State) &&
        (7 == ((rEXT(0x300) >> 24) & MSK_1F))
       )
    {
        ulL1State = TRUE;
    }

    else
    {
        ulL1State = FALSE;
    }

    return ulL1State;
#else
    return TRUE;
#endif
}

/****************************************************************************
Name        :HAL_PMIsSATAInLowPwrSts
Input       :void
Output      : Be or not be under L1 state.
Author      : Victor Zhang
Date        :
Description : Check whether the SATA link layeris under Slumber state.
Others      :
Modify      :
****************************************************************************/

U32 HAL_PMIsSATAInLowPwrSts(void)
{
#ifdef HOST_SATA
    return (TRUE == HAL_SataIsSlumber());
#else
    return TRUE;
#endif
}

/****************************************************************************
Name        :HAL_PMIsDevInLowPwrSts
Input       :void
Output      : Be or not be under L1 state.
Author      : Victor Zhang
Date        :
Description : Check whether the SATA link layeris under Slumber state.
Others      :
Modify      :
****************************************************************************/
U32 HAL_PMIsDevInLowPwrSts(void)
{
#ifdef HOST_SATA
    return HAL_PMIsSATAInLowPwrSts();
#else
    return HAL_PMIsPCIeInLowPwrSts();
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
Name        :   PMCalibrateTimer
Input       :   None.
Output      :   None / Global variable TickPerMS would store correct PMU timer clock.
Author      :
Date        :
Description :   Calibrates PMU timer clock. Calculates current PMU clock frequency with
                     MCU clock for possible clock source from ring oscillator.
Others      :
Modify      :
****************************************************************************/
#ifdef CALC_RINGOSC_CLK
LOCAL void PMCalibrateTimer(void)
{
    /* 1. Starts Timer 1 with match value setting to 0. */
    l_pPMURegBlk->tTimer1Control.ulCurrVal = 0;
    l_pPMURegBlk->tTimer1Control.ulMchVal = INVALID_8F;
    l_pPMURegBlk->tTimer1Control.bsStop = FALSE;

    /* 2. Waits 1 millisecond using MCU cycle count. */
    HAL_DelayUs(1000);

    /* 3. Stops Timer 1 and records its current counter value. */
    l_pPMURegBlk->tTimer1Control.bsStop = TRUE;
    l_ulPMUTimerTickCountPerMS = l_pPMURegBlk->tTimer1Control.ulCurrVal;
    l_pPMURegBlk->tTimer1Control.ulCurrVal = 0;

    return;
}

/****************************************************************************
Name        :   HAL_PMGetTimerTickPerMS
Input       :   None.
Output      :   Global variable TickPerMS which stores correct PMU timer clock.
Author      :
Date        :
Description :   Gets current PMU timer clock after calibrating timer.
Others      :
Modify      :
****************************************************************************/
U32 HAL_PMGetTimerTickPerMS(void)
{
    return l_ulPMUTimerTickCountPerMS;
}
#endif

/****************************************************************************
Name        :   HAL_PMStartTimer
Input       :   ulTickCount - Specifies time period by clock cycle;
                   ulTimerID - Specifies which timer to use.
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
        l_pPMURegBlk->tTimer0Control.ulCurrVal = 0;
        l_pPMURegBlk->tTimer0Control.ulMchVal = ulTickCount;
        l_pPMURegBlk->tTimer0Control.bsStop = FALSE;
    }

    else if (PMU_TIMER1 == ulTimerID)
    {
        l_pPMURegBlk->tTimer1Control.ulCurrVal = 0;
        l_pPMURegBlk->tTimer1Control.ulMchVal = ulTickCount;
        l_pPMURegBlk->tTimer1Control.bsStop = FALSE;
    }

    else if (PMU_TIMER2 == ulTimerID)
    {
        l_pPMURegBlk->tTimer2Control.ulCurrVal = 0;
        l_pPMURegBlk->tTimer2Control.ulMchVal = ulTickCount;
        l_pPMURegBlk->tTimer2Control.bsStop = FALSE;
    }

    else
    {
        DBG_Getch();
    }

    return;
}

/****************************************************************************
Name        :   HAL_PMStopTimer
Input       :   ulTimerID - Specifies which timer to use.
Output      :   None
Author      :
Date        :
Description :   Stops the specified timer before it overflows and generates an interrupt.
Others      :
Modify      :
****************************************************************************/
void HAL_PMStopTimer(U32 ulTimerID)
{
    U32 ulIntrPending = HAL_PMGetIntrPendingStatus();
    if (PMU_TIMER0 == ulTimerID)
    {
        l_pPMURegBlk->tTimer0Control.bsStop = TRUE;
        l_pPMURegBlk->tTimer0Control.ulCurrVal = 0;

        if (0 != (ulIntrPending & PMU_INT_TIMER0))
        {
            HAL_PMClearIntrPendingStatus(PMU_INT_TIMER0);
        }
    }

    else if (PMU_TIMER1 == ulTimerID)
    {
        l_pPMURegBlk->tTimer1Control.bsStop = TRUE;
        l_pPMURegBlk->tTimer1Control.ulCurrVal = 0;

        if (0 != (ulIntrPending & PMU_INT_TIMER1))
        {
            HAL_PMClearIntrPendingStatus(PMU_INT_TIMER1);
        }
    }

    else if (PMU_TIMER2 == ulTimerID)
    {
        l_pPMURegBlk->tTimer2Control.bsStop = TRUE;
        l_pPMURegBlk->tTimer2Control.ulCurrVal = 0;

        if (0 != (ulIntrPending & PMU_INT_TIMER2))
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

GLOBAL void HAL_PMSetDevSleepPinEn(BOOL bEnable)
{
    l_pPMURegBlk->tGlobalControl.bsDevSlpPinEn = bEnable;

    return;
}

INLINE BOOL HAL_PMGetDevSleepFlag(void)
{
    return l_ulPMUDevSleepFlag;
}

INLINE void HAL_PMSetDevSleepFlag(void)
{
    l_ulPMUDevSleepFlag = TRUE;
}

INLINE void HAL_PMClearDevSleepFlag(void)
{
    l_ulPMUDevSleepFlag = FALSE;
}

#ifdef PCIE_ASPM_MANAGEMENT
INLINE U32 HAL_PMGetSqlchFlag(void)
{
    return l_ulSquelchFlag;
}

INLINE void HAL_PMSetSqlchFlag(const U32 ulFlag)
{
    l_ulSquelchFlag = ulFlag;
    return;
}
#endif

#ifdef PM_STANDBY_SUPPORT
GLOBAL U32 HAL_PMInitiateStandby(void)
{
    U32 ulStbSucc;
    ulStbSucc = HAL_PMIsDDRinSelfRef();

    if (TRUE == ulStbSucc)
    {
#if 1
        ulStbSucc = FALSE;
        HAL_PMClearSafetyBlock();

        /* Only leaves MPLL enabled. */
        l_pPMURegBlk->tHwClkCfg.bsHCLK25En = TRUE;

        if (TRUE == l_pPMURegBlk->tHwClkCfg.bsHCLK25En)
        {
#ifdef HOST_NVME
            bDPLLGating = TRUE;
#endif
            HAL_DelayCycle(10);
            l_pPMURegBlk->ulPLLGatingCfg &= ~0x77;
            ulStbSucc = TRUE;
        }
#else
        /* Engages L1 clock gating. */
        HAL_PMEnableL1ClockGating(TRUE);
#endif
    }

    return ulStbSucc;
}

GLOBAL U32 HAL_PMExitStandby(void)
{
    U32 ulExitSucc = FALSE;
#if 1
    HAL_PMClearSafetyBlock();

    l_pPMURegBlk->ulPLLGatingCfg |= 0x77;
    HAL_DelayCycle(10);
    l_pPMURegBlk->tHwClkCfg.bsHCLK25En = FALSE;

    if(FALSE == l_pPMURegBlk->tHwClkCfg.bsHCLK25En)
    {
#ifdef HOST_NVME
        bDPLLGating = FALSE;
#endif
        ulExitSucc = TRUE;
    }
#else
    /* Disengages L1 clock gating. */
    HAL_PMEnableL1ClockGating(FALSE);
    HAL_DelayUs(10);
#endif

    return ulExitSucc;
}
#endif

GLOBAL void HAL_PMEnablePLL(U32 ulPlls)
{
    l_pPMURegBlk->ulPowerStatusAndControl = ulPlls;

    return;
}

GLOBAL U32 HAL_PMIsMainPLLReady(void)
{
    if ((PMU_DPLL_RDY_FLAG | PMU_NPLL_RDY_FLAG) != (l_pPMURegBlk->ulPowerStatusAndControl &
        (PMU_DPLL_RDY_FLAG | PMU_NPLL_RDY_FLAG)))
    {
        return FALSE;
    }

    else
    {
        return TRUE;
    }
}

INLINE void HAL_PMEnableHWL12Ctrl(U32 ulEnableOrDisable)
{
#ifndef HOST_SATA
    l_pPMURegBlk->tPCIePHYCfg.bsCLKREQHwCntlEn = ulEnableOrDisable;
#endif
    return;
}

INLINE void HAL_PMEnableDDRSelfRefresh(U32 ulEnableOrDisable)
{
    if (TRUE == ulEnableOrDisable)
    {
        // Enables automatical self-refresh for both banks of DDR3
        rDRAMC(0x28) |= ((1 << 11) | (1 << 12));
        //l_pPMUCtrlRegBlk->tMiscControl2.bsForceSelfRefEn = TRUE;
    }

    else
    {
        // Disables automatical self-refresh for both banks of DDR3
        //l_pPMUCtrlRegBlk->tMiscControl2.bsForceSelfRefEn = FALSE;
        rDRAMC(0x28) &= ~((1 << 11) | (1 << 12));
    }

    return;
}

INLINE void HAL_PMEnableWatchDog(U32 ulPeriodSel)
{
    l_pPMURegBlk->tWatchDogCfg.bsWchDogCntSel = ulPeriodSel;
    l_pPMURegBlk->tWatchDogCfg.bsWchDogEn = TRUE;
    return;
}

INLINE void HAL_PMDisableWatchDog(void)
{
    l_pPMURegBlk->tWatchDogCfg.bsWchDogEn = FALSE;
    return;
}

INLINE void HAL_PMFeedWatchDog(void)
{
    l_pPMURegBlk->tWatchDogCfg.bsFeedDog = TRUE;
    l_pPMURegBlk->tWatchDogCfg.bsFeedDog = FALSE;
    return;
}

void HAL_PMEnableL1ClockGating(U32 ulEnableOrDisable)
{
    //HAL_PMGetSpinLockWait(SPINLOCKID_CLKGATING);
    //HAL_MultiCoreGetSpinLockWait(SPINLOCKID_CLKGATING);
    if (TRUE == ulEnableOrDisable)
    {
        rGlbClkGating &= ~PMU_L1CKG_ENABLE;
        rGlbClkCtrl &= ~PMU_L1CKG_ENABLE2;
    }

    else
    {
        rGlbClkCtrl |= PMU_L1CKG_ENABLE2;
        rGlbClkGating |= PMU_L1CKG_ENABLE;
        HAL_DelayCycle(50);
    }

    //HAL_PMReleaseSpinLock(SPINLOCKID_CLKGATING);
    //HAL_MultiCoreReleaseSpinLock(SPINLOCKID_CLKGATING);
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
    U32 ulIntrPending = HAL_PMGetIntrPendingStatus();

#if 0
#ifndef FPGA
#ifdef HOST_NVME
    /* A PCIe out-band reset occurred. */
    if ( 0 != ( ulIntrPending & PMU_INT_PCIE_OBDRST) )
    {
        L0_NVMePcieResetISR();
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_PCIE_OBDRST);

        return;
    }

    /* A PCIe in-band reset occurred. */
    if ( 0 != ( ulIntrPending & PMU_INT_PCIE_IBDRST) )
    {
        /* Just clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_PCIE_IBDRST);

        return;
    }
#endif

    /* A power failure is detected by hardware. */
    if ( 0 != ( ulIntrPending & PMU_INT_CHIP_PWRFAILURE) )
    {
        /* Just clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_CHIP_PWRFAILURE);

        return;
    }

    /* An overheat is detected by hardware. */
    if ( 0 != ( ulIntrPending & PMU_INT_CHIP_OVERHEAT) )
    {
        /* Just clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_CHIP_OVERHEAT);

        return;
    }
#endif
#endif

#ifdef PWR_LOSS_WRITE_PROTECT
    /* Power failure detect. */
    if ( 0 != ( ulIntrPending & PMU_INT_CHIP_PWRFAILURE) )
    {
        /* set flash write protect pin */
        rGLB(0x0c) &= ~(1<<16);

        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_CHIP_PWRFAILURE);

        /* Initiates a power-off protection sequence when an abrupt power-loss is detected. */
        l_pPMURegBlk->ulPwrDetrCfg |= ((1 << 3) | (1 << 9));

        return;
    }
#endif


    /* Core power down sequence terminated. */
    if ( 0 != ( ulIntrPending & PMU_INT_PWR_BLKD) )
    {
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_PWR_BLKD);

        return;
    }

    /* PLL shutdown sequence terminated. */
    if ( 0 != ( ulIntrPending & PMU_INT_PLL_BLKD) )
    {
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_PLL_BLKD);

        return;
    }

    /* Squelch interrupt signaled. */
    if ( 0 != ( ulIntrPending & PMU_INT_SQUELCH ) )
    {
        U32 ulPCIePLLRdy;
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_SQUELCH);

        /* Patch for hardware issue: when link is woken up and PMU had not asserted RBLOCK,
        PLLs would be shut down improperly. Firmware must re-power them up and assert
        RBLOCK manually. */
        if (FALSE == HAL_PMGetSafetyBlock())
        {
            HAL_PMEnablePLL(PMU_INITIATE_PLLON);
            HAL_PMSetSafetyBlock();
        }

#ifdef HOST_NVME
        if (0 == (rGlbClkCtrl & PMU_L1CKG_PCIE))
        {
            while (TRUE)
            {
                if (5 == ulPCIePLLRdy)
                {
                    break;
                }

                /* Debounce here */
                if((0x3 << 24) == (rPCIe(0x670) & (0x3 << 24)))
                {
                    ulPCIePLLRdy++;
                }
                else
                {
                    ulPCIePLLRdy = 0;
                }
            }
            rGlbClkCtrl |= PMU_L1CKG_PCIE;
        }

        HAL_NVMeClearL12IdleFlag();
#ifdef PCIE_ASPM_MANAGEMENT
        HAL_PMEnablePCIeASPM(FALSE);
        HAL_PMSetSqlchFlag(TRUE);
#endif
#endif
        return;
    }

#ifdef HOST_SATA
    /* DEVSLP deassertion interrupt signaled. */
    if ( 0 != ( ulIntrPending & PMU_INT_DEVSLP_DEASST ) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_DEVSLP_DEASST);

        /* Clears enable flag */
        HAL_PMClearDevSleepFlag();

        return;
    }

    /* DEVSLP assertion interrupt signaled. */
    if ( 0 != ( ulIntrPending & PMU_INT_DEVSLP_ASST) )
    {
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_DEVSLP_ASST);

        /* Sets enable flag */
        HAL_PMSetDevSleepFlag();

        return;
    }
#endif

    /* Timer 0 overflow interrupt signaled. */
    if ( 0 != ( ulIntrPending & PMU_INT_TIMER0 ) )
    {
        /* Clears interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_TIMER0);

        /* Sets time out flag. */
        l_ulPMUTimeOutFlag[0] = TRUE;

        /* For SATA, sets standby timer expire event. */
#ifdef HOST_SATA
        L0_EventSet(L0_EVENT_TYPE_ATASTANDBYTIMEOUT, NULL);
#endif

        return;
    }

    /* Timer 1 overflow interrupt signaled. */
    if ( 0 != ( ulIntrPending & PMU_INT_TIMER1 ) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_TIMER1);

        /* Sets time out flag. */
        l_ulPMUTimeOutFlag[1] = TRUE;

        return;
    }

    /* Timer 2 overflow interrupt signaled. */
    if ( 0 != ( ulIntrPending & PMU_INT_TIMER2 ) )
    {
        /* Clear interrupt pending status */
        HAL_PMClearIntrPendingStatus(PMU_INT_TIMER2);

        /* Sets time out flag. */
        l_ulPMUTimeOutFlag[2] = TRUE;

        return;
    }

    return;
}

