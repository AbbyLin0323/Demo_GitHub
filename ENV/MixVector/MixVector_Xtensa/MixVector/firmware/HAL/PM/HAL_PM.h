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
Filename     :  HAL_PM.h                                         
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  Yao Chen

Description: The definition for PMU register file, constants,
    states, etc. Function prototype of interface APIs and declarations
    of corresponding global viarables are also in this header.

Modification History:
*******************************************************************************/
#ifndef _HAL_PM_H
#define _HAL_PM_H

/* PMU main status and control register block */
typedef struct _PMU_CONTROL_REG_BLK
{
    /* 1. REG00: PLL/Power Control
    Bit[31:6] - Reserved;
    Bit[5] -- Initiates core power down for Suspend mode, W1S;
    Bit[4] -- Initiates core power down for Hibernate mode, W1S;
    Bit[3] -- Initiates HCLK PLL shut down sequence, W1S;
    Bit[2] -- Initiates Serial PHY MPLL shut down sequence, W1S;
    Bit[1] -- Initiates NFC PLL shut down sequence, W1S;
    Bit[0] -- Initiates DRAMC PLL shut down sequence, W1S;
    Since all present bits inside this register have W1S attribute, 
    any read-modify-write behavior is prohibited. We must treat
    it as a write-only register, therefore bit section define is not
    suitable for it.
     */
    U32 ulPowerStateControl;

    /* 2. REG04: Miscellaneous hardware status/control 1
    Bit[31] -- Reflecting whether PCIe PHY is in L1 power-saving state, RO:
        0 - PCIe PHY is not in L1, 1 - PCIe PHY is in L1;
    Bit[30:26] -- Reserved;
    Bit[25] -- Periodic Squelch Detection enable, RW:
        0 - Disable, 1 - Enable;
    Bit[24] -- HCLK internal 25MHz enable, RW:
        0 - Normal HCLK, 1 - 25MHz backup clock;
    Bit[23:17] -- Reserved;
    Bit[16] -- Power control safety protection enable status, RW:
        0 - Safety protection is not taking effect,
        1 - Safety protection is engaged.
    All power state transition triggered by software would be discarded
    when this bit has the value of 1. It would be set by hardware automatically
    when any activity appeares on PCIe link.
    Bit[15:3] -- Reserved;
    Bit[2] -- Timer2 wake-up event enable for power down mode, RW:
        0 - Disable, 1 - Enable;
    Bit[1] -- Timer2 wake-up event enable for MPLL, RW:
        0 - Disable, 1 - Enable;
    Bit[0] -- Timer2 wake-up event enable for HPLL/DPLL/NFPLL, RW:
        0 - Disable, 1 - Enable.
     */
    union
    {
        struct
        {
            U32 bsTmr2HPLLWakeEn: 1;
            U32 bsTmr2MPLLWakeEn: 1;
            U32 bsTmr2PwrWakeEn: 1;
            U32 bsReg04Rsvd1: 13;
            U32 bsPwrCntlSftyPrt: 1;
            U32 bsReg04Rsvd2: 7;
            U32 bsHClk25En: 1;
            U32 bsPeriodSqDetEn: 1;
            U32 bsReg04Rsvd3: 5;
            U32 bsPHYinL1State: 1;
        } tMiscControl1;

        U32 ulMiscControl1;
    };

    /* 3. REG08: Hardware internal timer/counter configuration
    Bit[31] -- Enhanced OOB detection enable (for SATA mode usage only), RW:
        0 - Disable, 1 - Enable;
    Bit[30] -- Timer 2 input clock frequenct select, RW:
        0 - 25MHz, 1 - 25MHz devided by 128;
    Bit[29] -- Timer 1 input clock frequenct select, RW:
        0 - 25MHz, 1 - 25MHz devided by 128;
    Bit[28] -- Timer 0 input clock frequenct select, RW:
        0 - 25MHz, 1 - 25MHz devided by 128;
    Bit[27] -- Minimum core power down time select
    (the minimum time core power must remain in off state before it can resume
    due to a wake-up event), RW:
        0 - 640us, 1 - 1920us;
    Bit[26] -- Reserved;
    Bit[25] -- PLL stable signal output delay adjust for HPLL/DPLL/NFPLL, RW:
        0 - Immediately with PLL_OK, 1 - Delay 120us after PLL_PU/RST# assert;
    Bit[24] - PLL stable signal output delay adjust for MPLL, RW:
        0 - Immediately with PLL_OK, 1 - Delay 20.48us after MPLLPDB assert;
    Bit[23] -- Reserved;
    Bit[22] -- PLL on/off control method select, RW:
        0 - Legacy level controlled as in VT3492,
        1 - Controlled by trigger signals from firmware writing to Reg 00;
    Bit[21] -- REFCLK slicer enable (for SATA mode usage only), RW:
        0 -- Disable (Power off), 1 - Enable;
    Bit[20] -- REFCLK slicer control (for SATA mode usage only), RW:
        0 -- Active mode, 1 - Power saving mode;
    Bit[19:16] -- Countdown timer value before deassertion of PSO_EN signal
    when a core power off operation is initiated, RW;
    Bit[15:12] -- Countdown timer value before assertion of RST# signal when
    a core power off operation is initiated, RW;
    Bit[11:8] -- Countdown timer value before assertion of ISO_EN signal when
    a core power off operation is initiated, RW;
    Bit[7:4] -- Countdown timer value before deassertion of ISO_EN signal when
    power is resumed, RW;
    Bit[3:0] -- Countdown timer value before deassertion of RST# signal when
    power is resumed, RW;
     */
    union
    {
        struct
        {
            U32 bsPwrUpTmr_RST: 4;
            U32 bsPwrUpTmr_ISOEN: 4;
            U32 bsPwrDnTmr_ISOEN:4;
            U32 bsPwrDnTmr_RST: 4;
            U32 bsPwrDnTmr_PSOEN: 4;
            U32 bsSATASlicerCntl: 1;
            U32 bsSATASlicerEn: 1;
            U32 bsPLLOnOffCntlSel: 1;
            U32 bsReg08Rsvd1: 1;
            U32 bsMPLLStbDlySel: 1;
            U32 bsHPLLStbDlySel: 1;
            U32 bsReg08Rsvd2: 1;
            U32 bsMPDTSel: 1;
            U32 bsTmr0InputSel: 1;
            U32 bsTmr1InputSel: 1;
            U32 bsTmr2InputSel: 1;
            U32 bsEnhOOBDetEn: 1;
        } tHwTmrCfg;

        U32 ulHwTmrCfg;
    };

    /* 4. REG0C: Hardware Periodic Squelch Detection configuration
    Bit[31:26] -- Reserved;
    Bit[25:16] -- Interval period counter for Periodic Squelch Detection, RW;
    Bit[15:10] -- Reserved;
    Bit[9:0] -- Assertion period counter for Period Squelch Detection, RW.
     */
    union
    {
        struct
        {
            U32 bsPeriodSqDetAsst: 10;
            U32 bsReg0CRsvd1: 6;
            U32 bsPeriodSqDetIntvl: 10;
            U32 bsReg0CRsvd2: 6;
        } tHwPeriodSqDetCfg;

        U32 ulHwPeriodSqDetCfg;
    };

    /* 5. REG10: Miscellaneous hardware status/control 2
    Bit[31:27] -- Reserved;
    Bit[26] -- Extend OOB detection enable (for SATA mode usage only), RW:
        0 - Disable, 1 - Enable;
    Bit[25] -- PHY ASR function enable (for SATA mode usage only), RW:
        0 - Disable, 1 - Enable;
    Bit[24] -- Response to OOB during reset enable (for SATA mode usage only), RW:
        0 - Disable, 1 - Enable;
    Whether SATA PHY may respond to host COMWAKE during reset state.
    Bit[23:16] -- SATA function configuration (for SATA mode usage only), RW:
        Bit[23] -- SATA command pure-firmware-decoding enable;
        Bit[22] -- SATA COMINIT ready;
        Bit[21] -- SATA COMINIT mode select;
        Bit[20] -- SATA DMAC/CMDM block enable;
        Bit[19] -- SATA DMAC reset method select;
        Bit[18] -- SATA CMDM reset method select;
        Bit[17] -- SATA L-RESET method select;
        Bit[16] -- SATA COMRESET interrupt method select;
    Bit[15:14] -- Reserved;
    Bit[13:12] -- Whole-chip debug signal select for lower 16 pins, RW;
    Bit[11:10] -- Whole-chip debug signal select for upper 16 pins, RW;
    Bit[9] -- Clock gating enable for PMU module, RW:
        0 - Disable, 1 - Enable;
    Bit[8] -- Debug signal output logic enable, RW:
        0 - Disable (Clock gated), 1 - Enable (Debug pins enabled);
    Bit[7:4] -- Reserved;
    Bit[3] -- Interrupt enable on HPLL off, RW:
        0 - PMU can issue an interrupt even when HPLL is not ready,
        1 - PMU would not issue interrupts until HPLL gets ready;
    Bit[2] -- Simulation accelerating enable (for co-sim environment only), RW:
        0 - Disable, 1 - Enable;
    Bit[1] -- Resetting EPHY RTN tune function enable, RW:
        0 - Disable, 1 - Enable;
    Bit[0] -- DRAM force self-refresh enable, RW:
        0 - Disable, 1 - Enable;
     DRAM would be forced to maintain self-refresh state when this bit
     has the value of 1. Caution: DRAM is not accessable under this state.
     */
    union
    {
        struct {
            U32 bsForceSelfRefEn: 1;
            U32 bsRstPHYRTNEn: 1;
            U32 bsSimAccEn: 1;
            U32 bsIntOnHPLLRdy: 1;
            U32 bsReg10Rsvd1: 4;
            U32 bsDbgPinEn: 1;
            U32 bsPMUClkGatEn: 1;
            U32 bsDbgPinSelHi: 2;
            U32 bsDbgPinSelLo: 2;
            U32 bsReg10Rsvd2: 2;
            U32 bsSATACRSTIntSel: 1;
            U32 bsSATALRSTSel: 1;
            U32 bsSATACMDMRstSel: 1;
            U32 bsSATADMACRstSel: 1;
            U32 bsSATADMACBlkEn: 1;
            U32 bsSATACINITRdySel: 1;
            U32 bsSATACINITRdyEn: 1;
            U32 bsSATACmdFwDecEn: 1;
            U32 bsOOBRespInResetEn: 1;
            U32 bsSATAASREn: 1;
            U32 bsOOBExtdEn: 1;
            U32 bsReg10Rsvd3: 5;
        } tMiscControl2;

        U32 ulMiscControl2;
    };

    /* 6. REG14: Hardware Hibernate/DEVSLP function related configuration
    Bit[31] -- Device Sleep signal input pin enable, RW:
        0 - Disable, 1 - Enable;
    Bit[30] -- Accessory power off enable, RW:
        0 - Disable, 1 - Enable;
    Allows power shutting off for DRAM/NAND chips etc. as well as shutting down
    core power when initiating a Hibernate state.
    Bit[29:20] -- Reserved;
    Bit[19] -- Clock select for internal OSC, RW:
        0 - Dividing by 64, 1 - Dividing by 128;
    Bit[18] -- Internal OSC enable, RW:
        0 - Disable, 1 - Enable;
    When system is in Hibernate state we need the internal OSC for clock source backup.
    Bit[17:16] -- Clock source select in Hibernate state, RW:
        x0 - 25MHz from serial PHY, 01 - internal OSC, 10 - external GPIO input;
    Bit[15:1] -- Reserved;
    Bit[0] -- Signal reloading enable for SATAC (for SATA mode usage only), RW:
        0 - Disable, 1 - Enable.
    */
    union
    {
        struct
        {
            U32 bsSATAReloadEn: 1;
            U32 bsReg14Rsvd1: 15;
            U32 bsHbnClkSel: 2;
            U32 bsIntOSCEn: 1;
            U32 bsIntOSCClkSel: 1;
            U32 bsReg14Rsvd2: 10;
            U32 bsPwDnAccEn: 1;
            U32 bsDevSlpPinEn: 1;
        } tHwHbnCfg;
        
        U32 ulHwHbnCfg;
    };
    
    /* 7. REG18: Hardware PLL parameter configuration
    Bit[31:19] -- Reserved;
    Bit[18] - PLL analog test pin output select, RW:
        0 - VCNTL output, 1 - VREF output;
    Bit[17] -- PLL analog test pin output enable, RW:
        0 - Disable, 1 - Enable;
    Bit[16] -- PLL clock output test enable, RW:
        0 - Disable, 1 - Enable;
    Bit[15:13] -- Reserved;
    Bit[12:10] -- HPLL regulator reference voltage select, RW;
    Bit[9:8] -- HPLL output frequency select, RW;
    Bit[7:5] -- Reserved;
    Bit[4:2] -- DPLL regulator reference voltage select, RW;
    Bit[1:0] -- DPLL output frequency select, RW.
     */
    union
    {
        struct
        {
            U32 bsDPLLOutputSel: 2;
            U32 bsDPLLRglRefSel: 3;
            U32 bsReg18Rsvd1: 3;
            U32 bsHPLLOutputSel: 2;
            U32 bsHPLLRglRefSel: 3;
            U32 bsReg18Rsvd2: 3;
            U32 bsPLLClkOutTestEn: 1;
            U32 bsPLLAlgOutEn: 1;
            U32 bsPLLAlgOutSel: 1;
            U32 bsReg18Rsvd3: 13;
        } tHwPLLCfg;

        U32 ulHwPLLCfg;
    };

    /* 8. REG1C: PMU status/control related to PCIe
    Bit[31:25] -- Reserved;
    Bit[24] -- RST_EN deassertion enable in PCIe power down mode, RW:
        0 - Disable, 1 - Enable;
    Bit[23:19] -- Reserved;
    Bit[18] -- SQDET signal waking up PLL enable, RW:
        0 - Disable, 1 - Enable;
    Bit[17] -- CLKREQ# signal waking up PLL enable, RW:
        0 - Disable, 1 - Enable;
    Bit[16:10] -- Reserved;
    Bit[9] -- PCIe clock mode select, RW:
        0 - Power-saving mode (uses internal OSC),
        1 - Active mode (uses EPHY clock input);
    Bit[8] -- Core power down in PCIe mode enable, RW:
        0 - Disable, 1 - Enable;
    Bit[7:1] -- Reserved;
    Bit[0] -- CLKREQ# signal tri-state control, RW:
        0 - CLKREQ# in input mode,
        1 - CLKREQ# in output mode.
     */
    union
    {
        struct
        {
            U32 bsCLKREQCntl: 1;
            U32 bsReg1CRsvd1: 7;
            U32 bsPwrDnEn: 1;
            U32 bsClkModeSel: 1;
            U32 bsReg1CRsvd2: 7;
            U32 bsWkupByCLKREQEn: 1;
            U32 bsWkupBySQDETEn: 1;
            U32 bsReg1CRsvd3: 5;
            U32 bsRSTENDasstEn: 1;
            U32 bsReg1CRsvd4: 7;
        } tPCIeControl;

        U32 ulPCIeControl;
    };

    /* 9. REG20: Hardware internal clock configuration
    Bit[31:12] -- Reserved;
    Bit[11:10] -- NFC clock select in slow mode, RW;
    Bit[9:8] -- NFC ECC clock select, RW;
    Bit[7:6] -- NFC clock select in normal mode, RW;
    Bit[5] -- Reserved;
    Bit[4:2] -- NFPLL regulator reference voltage select, RW;
    Bit[1:0] -- NFPLL output frequency select, RW.
     */
    union
    {
        struct
        {
            U32 bsNFPLLOutputSel: 2;
            U32 bsNFPLLRglRefSel: 3;
            U32 bsReg20Rsvd1: 1;
            U32 bsNFCLKSel: 2;
            U32 bsECCCLKSel: 2;
            U32 bsNFCLKSelInSlowMode: 2;
            U32 bsReg20Rsvd2: 20;
        } tHwClkCfg;

        U32 ulHwClkCfg;
    };

    /* 10. REG24 - REG3C: Scratch registers for firmware usage, RW:
    REG24 -- The entry of resuming routine in DDR SDRAM;
    REG28 -- Power-on signature for distinguishing a Hibernate resuming from a cold boot;
    REG2C - REG3C -- Reserved for future usage.
     */
    U32 ulResumeEntry;
    U32 ulPwrOnSgnt;
    U32 ScrReg[5];
} PMU_CONTROL_REGSET, *PPMU_CONTROL_REGSET;

/* PMU timer related control register block */
typedef struct _PMU_TIMER_REG_BLK
{
    /* 1. REG40: Timer control
    Bit[31:25] -- Reserved;
    Bit[24] -- Timer 1 start/stop control, RW:
        0 - Start timer, 1 -Stop timer;
    Bit[23:18] -- Reserved;
    Bit[17:16] -- Counting mode configuration for Timer 1, RW:
        00 - Timer disabled, 
        01 -Loop mode (Timer restarts counting on overflow), 
        1x - One time mode (Timer stops counting on overflow);
    Bit[15:9] -- Reserved;
    Bit[8] -- Timer 0 start/stop control, RW:
            0 - Start timer, 1 -Stop timer;
    Bit[7:2] -- Reserved;
    Bit[1:0] -- Counting mode configuration for Timer 0, RW:
        00 - Timer disabled, 
        01 -Loop mode (Timer restarts counting on overflow), 
        1x - One time mode (Timer stops counting on overflow).
     */
    union
    {
        struct
        {
            U32 bsTimer0Mode: 2;
            U32 bsReg40Rsvd1: 6;
            U32 bsTimer0Stop: 1;
            U32 bsReg40Rsvd2: 7;
            U32 bsTimer1Mode: 2;
            U32 bsReg40Rsvd3: 6;
            U32 bsTimer1Stop: 1;
            U32 bsReg40Rsvd4: 7;
        } tTimerControl;

        U32 ulTimerControl;
    };

    /* 2. REG44: Timer 0 counting period length, RW. */
    U32 ulTimer0Period;

    /* 3. REG48: Timer 0 current counter value, RW. */
    U32 ulTimer0Counter;

    /* 4. REG4C: PMU interrupt status/control plus timer 2 control
    Bit[31:24] -- PMU interrupt pending status (Bit-mapped), RW1C:
        Bit[31] - Core power down sequence aborted due to a waking-up event,
        Bit[30] - PLL shutting down sequence aborted due to a waking-up event,
        Bit[29] - DEVSLP signal deasserted by SATA host controller (for SATA mode usage only),
        Bit[28] - DEVSLP signal asserted by SATA host controller (for SATA mode usage only),
        Bit[27] - A waking-up event occured,
        Bit[26] - Timer 2 expired,
        Bit[25] - Timer 1 expired,
        Bit[24] - Timer 0 expired.
        This field is bit-significant and to be only cleared by writing a 1 to corresponding bit.
        Caution: Any read-modify-write behavior to this field may cause interrupt lost and is 
        therefore prohibited.
    Bit[23:16] -- PMU interrupt mask control (Bit-mapped), RW:
        This field is also bit-significant and has the same mapping as Bit[31:24]. 
    Bit[15:9] -- Reserved;
    Bit[8] -- Timer 2 start/stop control, RW:
            0 - Start timer, 1 -Stop timer;
    Bit[7:2] -- Reserved;
    Bit[1:0] -- Counting mode configuration for Timer 2, RW:
        00 - Timer disabled, 
        01 -Loop mode (Timer restarts counting on overflow), 
        1x - One time mode (Timer stops counting on overflow).
     */
    union
    {
        struct
        {
            U32 bsTimer2Mode: 2;
            U32 bsReg4CRsvd1: 6;
            U32 bsTimer2Stop: 1;
            U32 bsReg4CRsvd2: 7;
            U32 bsIntrMsk: 8;
            U32 bsIntrSts: 8;
        } tIntrControl;

        U32 ulIntrControl;
    };

    /* 5. REG50: Timer 1 counting period length, RW. */
    U32 ulTimer1Period;

    /* 6. REG54: Timer 1 current counter value, RW. */
    U32 ulTimer1Counter;

    /* 7. REG58: Timer 2 counting period length, RW. */
    U32 ulTimer2Period;

    /* 8. REG5C: Timer 2 current counter value, RW. */
    U32 ulTimer2Counter;
} PMU_TIMER_REGSET, *PPMU_TIMER_REGSET;


/* System level power management state machine states. */
typedef enum _SYSPM_STATE
{
    SYSPMSTATE_ACTIVE = 0,
    SYSPMSTATE_IDLE,
    /* This state may not be required. */
    /* SYSPMSTATE_SLEEP_PREPARE_START, */
    SYSPMSTATE_SUSPEND_PREPARING,
    SYSPMSTATE_SUSPEND_READY,
    SYSPMSTATE_HIBERNATE_PREPARING,
    SYSPMSTATE_HIBERNATE_READY,
    SYSPMSTATE_COLD_START,
    SYSPMSTATE_BOOTING,
    SYSPMSTATE_WARM_START,
    SYSPMSTATE_RESTORING,
    SYSPMSTATE_HIBERNATE_START
} SYSPM_STATE;

/* Hardware register block base address */
#define PMU_TIMER_REG_ADDRESS (REG_BASE_PMU + 0x40)
#define PMU_RESUME_ENTRY_ADDRESS (REG_BASE_PMU + 0x24)

/* Timer related definitions */
// Timer number
#define PMU_TIMER_NUMBER 3

// Timer index
#define PMU_TIMER0 0
#define PMU_TIMER1 1
#define PMU_TIMER2 2

// Timer modes
#define PMU_TIMER_MODE_DISABLED 0
#define PMU_TIMER_MODE_LOOP 1
#define PMU_TIMER_MODE_NORMAL 2

/* Resuming modes from PM state */
#define PM_POWERON_COLD_START 0
#define PM_POWERON_RESUME_FROM_SUSPEND 1
#define PM_POWERON_RESUME_FROM_HIBERNATE 2

/* Bit mapping for interrupt mask/pending */
#define PMU_INTBIT_TIMER0 0
#define PMU_INTBIT_TIMER1 1
#define PMU_INTBIT_TIMER2 2
#define PMU_INTBIT_SQUELCH 3
#define PMU_INTBIT_DEVSLP_ASST 4
#define PMU_INTBIT_DEVSLP_DEASST 5
#define PMU_INTBIT_PLL_BLKD 6
#define PMU_INTBIT_PWR_BLKD 7
#define PMU_INT_TIMER0 ( 1 << PMU_INTBIT_TIMER0 )
#define PMU_INT_TIMER1 ( 1 << PMU_INTBIT_TIMER1 )
#define PMU_INT_TIMER2 ( 1 << PMU_INTBIT_TIMER2 )
#define PMU_INT_SQUELCH ( 1 << PMU_INTBIT_SQUELCH )
#define PMU_INT_DEVSLP_ASST ( 1 << PMU_INTBIT_DEVSLP_ASST )
#define PMU_INT_DEVSLP_DEASST ( 1 << PMU_INTBIT_DEVSLP_DEASST )
#define PMU_INT_PLL_BLKD ( 1 << PMU_INTBIT_PLL_BLKD )
#define PMU_INT_PWR_BLKD ( 1 << PMU_INTBIT_PWR_BLKD )

/* Register trigger bit mapping definition for system-level power management state transition */
#define PMU_TURNOFF_DPLL 1
#define PMU_TURNOFF_NFPLL 2
#define PMU_TURNOFF_MPLL 4
#define PMU_TURNOFF_HPLL 8
#define PMU_TURNOFF_CORE_HBN 16
#define PMU_TURNOFF_CORE_SUSP 32
#define PMU_INITIATE_PLLOFF (PMU_TURNOFF_DPLL | PMU_TURNOFF_NFPLL | PMU_TURNOFF_MPLL | PMU_TURNOFF_HPLL)
#define PMU_INITIATE_SUSP (PMU_INITIATE_PLLOFF | PMU_TURNOFF_CORE_SUSP)
#define PMU_INITIATE_HBN (PMU_INITIATE_PLLOFF | PMU_TURNOFF_CORE_HBN)

/* PM defined global variables declaration */

/* PM interface routines declaration */
void HAL_PMInit(void);
SYSPM_STATE HAL_PMCheckBootType(void);
void HAL_PMSetSignature(U32);
void HAL_PMStartTimer(U32, U32);
void HAL_PMDozeOffMCU(U32);
U32 HAL_PMIsPCIeInLowPwrSts(void);
void HAL_PMShutDownPLL(void);
void HAL_PMInitiateSuspending(void);
void HAL_PMDisableSuspending(void);
void HAL_PMClearTimeOutFlag(U32);
U32 HAL_PMIsTimerTimedOut(U32);

#endif

/********************** FILE END ***************/


