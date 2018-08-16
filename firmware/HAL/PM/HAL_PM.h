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
#include "HAL_MemoryMap.h"

/* PMU main status and control register block */
typedef struct _PMUREG
{
    /* 1. REG00: PMU Global Settings
    Bit[31:27] -- Reserved;
    Bit[26] -- PCIE debug enable, RW:
        0 - PCIE Debug Mode is disabled, for normal usage, 1 - PCIE Debug Mode is enabled, the RST1# signal cannot fall to low-level;
    Bit[25] -- DDR IO DET gating enable on ISO_EN signal, RW:
        0 - Does not gate DDR IO DET when ISO_EN signal is asserted, 1 - Gates DDR IO DET to save pad power on ISO_EN assertion;
    Bit[24] -- DDR IO DET gating enable on self-refresh, RW:
        0 - Does not gate DDR IO DET when DDR enters self-refresh state, 1 - Gates DDR IO DET to save pad power when DDR is in self-refresh;
    Bit[23] -- NFC IO DET enable, RW:
        0 - NFC IO DET pin is disabled, 1 - NFC IO DET pin is enabled;
    Bit[22] -- DDR IO DET gating enable for LP-DDR mode, RW:
        0 - Keeps all DDR IO pads powered, 1 - Turns off unused DDR IO pads in LP-DDR mode;
    Bit[21] -- DDR IO DET gating enable for 16-bit mode, RW:
        0 - Keeps all DDR IO pads powered, 1 - Turns off unused DDR IO pads in 16-bit mode;
    Bit[20] -- DDR IO DET enable, RW:
        0 - DDR IO DET pin is disabled, 1 - DDR IO DET pin is enabled;
    Bit[19] -- PLL stable signal output delay adjust for HPLL/DPLL/NFPLL, RW:
        0 - Immediately with PLL_OK, 1 - Delay 100us after PLL_PU/RST# assert;
    Bit[18] - PLL stable signal output delay adjust for MPLL, RW:
        0 - Immediately with PLL_OK, 1 - Delay 20.48us after MPLLPDB assert (for SATA mode only);
    Bit[17] -- DRAM force self-refresh enable, RW:
        0 - Disable, 1 - Enable;
        DRAM would be forced to maintain self-refresh state when this bit
        has the value of 1. Warning: DRAM is not accessable under this state.
    Bit[16] -- Device Sleep signal input pin enable, RW:
        0 - Disable, 1 - Enable;
    Bit[15] -- Clock select for ring OSC, RW:
        0 - 10MHz in typical, 1 - 5MHz in typical;
    Bit[14] -- Ring OSC enable, RW:
        0 - Disable, 1 - Enable;
        When system is in Hibernate state we need the internal OSC for clock source backup.
    Bit[13] -- Clock source select for SUS domain, RW:
        0 - 25MHz clock from EPHY, 1 - Ring OSC clock;
    Bit[12] -- Clock source select for normal domain, RW:
        0 - 25MHz clock from EPHY, 1 - Ring OSC clock;
    Bit[11:10] -- Whole-chip debug signal select for upper 16 pins, RW;
    Bit[9:8] -- Whole-chip debug signal select for lower 16 pins, RW;
    Bit[7] -- Simulation accelerating enable (for co-sim environment only), RW:
        0 - Disable, 1 - Enable;
    Bit[6] -- Signal reloading enable for SATAC (for SATA mode usage only), RW:
        0 - Disable, 1 - Enable;
    Bit[5] -- Clock gating enable for PMU module, RW:
        0 - Disable, 1 - Enable;
    Bit[4] -- Debug signal output logic enable, RW:
        0 - Disable (Clock gated), 1 - Enable (Debug pins enabled);
    Bit[3:2] -- Reserved;
    Bit[1] -- Power control safety protection enable status, RW:
        0 - Safety protection is not taking effect,
        1 - Safety protection is engaged.
        All power state transition triggered by software would be discarded
        when this bit has the value of 1. It would be set by hardware automatically
        when any activity appeares on PCIe link.
    Bit[0] -- Reserved.
     */
    union
    {
        struct
        {
            U32 bsReg00Rsvd1: 1;
            U32 bsPwrCntlSftyPrt: 1;
            U32 bsReg00Rsvd2: 2;
            U32 bsDbgPinEn: 1;
            U32 bsPMUClkGatEn: 1;
            U32 bsSATAReloadEn: 1;
            U32 bsSimAccEn: 1;
            U32 bsDbgPinSelLo: 2;
            U32 bsDbgPinSelHi: 2;
            U32 bsNorClkSel: 1;
            U32 bsSusClkSel: 1;
            U32 bsRingOSCEn: 1;
            U32 bsRingOSCClkSel: 1;
            U32 bsDevSlpPinEn: 1;
            U32 bsForceSelfRefEn: 1;
            U32 bsMPLLStbDlySel: 1;
            U32 bsHPLLStbDlySel: 1;
            U32 bsDDRIODETEn: 1;
            U32 bsDDR16BitDETOff: 1;
            U32 bsDDRLPDETOff: 1;
            U32 bsNFCIODETEn: 1;
            U32 bsDDRIODETOffInSlfRef: 1;
            U32 bsDDRIODETOffOnISOEn: 1;
            U32 bsPCIeDbgEn: 1;
            U32 bsReg00Rsvd3: 5;
        } tGlobalControl;

        U32 ulGlobalControl;
    };

    /* 2. REG04: PLL/Power Control
    Bit[31:20] -- Reserved;
    Bit[19] -- Indicates hardware MPLL_RDY signal, RO;
    Bit[18] -- Indicates hardware NFPLL_RDY signal, RO;
    Bit[17] -- Indicates hardware HPLL_RDY signal, RO;
    Bit[16] -- Indicates hardware DPLL_RDY signal, RO;
    Bit[15] -- Indicates hardware MPLL_OK signal, RO;
    Bit[14] -- Indicates hardware NFPLL_OK signal, RO;
    Bit[13] -- Indicates hardware HPLL_OK signal, RO;
    Bit[12] -- Indicates hardware DPLL_OK signal, RO;
    Bit[11] -- Initiates SATA EPHY MPLL restart sequence, Write 1 to Trigger;
    Bit[10] -- Initiates NFC PLL restart sequence, Write 1 to Trigger;
    Bit[9] -- Initiates HCLK PLL restart sequence, Write 1 to Trigger;
    Bit[8] -- Initiates DRAMC PLL restart sequence, Write 1 to Trigger;
    Bit[7] -- Initiates SATA EPHY MPLL shut down sequence, Write 1 to Trigger;
    Bit[6] -- Initiates NFC PLL shut down sequence, Write 1 to Trigger;
    Bit[5] -- Initiates HCLK PLL shut down sequence, Write 1 to Trigger;
    Bit[4] -- Initiates DRAMC PLL shut down sequence, Write 1 to Trigger;
    Bit[3] -- Reserved;
    Bit[2] -- Initiates core power down sequence in PCIE mode when link stays in L1.2 state, Write 1 to Trigger;
    Bit[1] -- Initiates core power down sequence for SATA Device Sleep, Write 1 to Trigger;
    Bit[0] -- Initiates core low-power mode in SATA slumber state or PCIE L1.1 state, Write 1 to Trigger;
    Since some present bits inside this register have W1T attribute, 
    we should avoid read-modify-write behavior. Therefore bit section
    define is not suitable for it.
     */ 
    U32 ulPowerStatusAndControl;

    /* 3. REG08: Hardware power on/off configuration
    Bit[31] -- Reserved;
    Bit[30] -- PSO2_EN signal control for DDR, RW:
        0 - PSO2_EN signal always sustains high-level, 1 - PSO2_EN just has the same function as PSO_EN;
    Bit[29] -- Voltage decreasing enable in light sleep state (link in slumber/L1), RW:
        0 - Hardware would not decrease chip voltage when light sleep is engaged by asserting PSO2_EN,
        1 - Hardware would decrease chip voltage when enters light sleep with PSO2_EN signal asserted;
    Bit[28] -- Accessory power off enable (PSO_EN2 signal enable), RW:
        0 - Disables PSO_EN2 signal, 1 - Enables PSO_EN2 signal to output accessory power-off
            signal (besides core power off) through GPIO30;
    Bit[27] -- Minimum core power down time select
        (the minimum time core power must remain in off state before it can resume
        due to a wake-up event), RW:
        0 - 640us, 1 - 1920us;
    Bit[26] -- DPLL input source select, reserved for hardware, RW;
    Bit[25] -- REFCLK slicer enable (for SATA mode usage only), RW:
        0 -- Disable (Power off), 1 - Enable;
    Bit[24] -- REFCLK slicer control (for SATA mode usage only), RW:
        0 -- Active mode, 1 - Power saving mode;
    Bit[23:20] -- Countdown timer value before assertion of PSO_EN signal
        when a core power off operation is initiated, RW;
    Bit[19:16] -- Countdown timer value before assertion of RST# signal when
        a core power off operation is initiated, RW;
    Bit[15:12] -- Countdown timer value before assertion of ISO_EN signal when
        a core power off operation is initiated, RW;
    Bit[11:8] -- Countdown timer value before deassertion of PSO_LOW signal when
        voltage is raised back, RW;
    Bit[7:4] -- Countdown timer value before deassertion of ISO_EN signal when
        power is resumed, RW;
    Bit[3:0] -- Countdown timer value before deassertion of RST# signal when
        power is resumed, RW.
     */
    union
    {
        struct
        {
            U32 bsPwrUpTmr_RST: 4;
            U32 bsPwrUpTmr_ISOEN: 4;
            U32 bsPwrUpTmr_PSOLOW: 4;
            U32 bsPwrDnTmr_ISOEN:4;
            U32 bsPwrDnTmr_RST: 4;
            U32 bsPwrDnTmr_PSOEN: 4;
            U32 bsSATASlicerCntl: 1;
            U32 bsSATASlicerEn: 1;
            U32 bsDPLLInptSrcSel: 1;
            U32 bsMinPwrDnTimeSel: 1;
            U32 bsPwrDnAccEn: 1;
            U32 bsLowSlpVlgEn: 1;
            U32 bsPSO2ENDDRSel: 1;
            U32 bsReg08Rsvd1: 1;
        } tHwPwrOnOffCfg;

        U32 ulHwPwrOnOffCfg;
    };

    /* 4. REG0C: Hardware Squelch Detection configuration (for SATA solution only)
    Bit[31] -- Host OOB signal forwarding to SATA LPHY select (after MPLL powered off), RW:
        0 - PMU only forwards an OOB signal to LPHY after all PLLs are ready,
        1 - PMU forwards an OOB signal to LPHY directly despites of the state of PLLs;
    Bit[30] -- Extend OOB detection enable, RW:
        0 - Disable, 1 - Enable;
    Bit[29] -- Enhanced OOB detection enable, RW:
        0 - Disable, 1 - Enable;
    Bit[28] -- Periodic Squelch Detection enable, RW:
        0 - Disable, 1 - Enable;
    Bit[27:26] -- Reserved;
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
            U32 bsReg0CRsvd2: 2;
            U32 bsPeriodSqDetEn: 1;
            U32 bsEnhOOBDetEn: 1;
            U32 bsOOBExtdEn: 1;
            U32 bsFwdOOBWaitingPLL: 1;
        } tSATASqlchDetCfg;

        U32 ulSATASqlchDetCfg;
    };

    /* 5. REG10: Miscellaneous SATA function configuration
    Bit[31:10] -- Reserved;
    Bit[9] -- PHY ASR function enable, RW:
        0 - Disable, 1 - Enable;
    Bit[8] -- Response to OOB during reset enable, RW:
        0 - Disable, 1 - Enable;
        Whether SATA PHY may respond to host COMWAKE during reset state.
    Bit[7:0] -- SATA function configuration, RW:
        Bit[7] -- SATA command pure-software-decoding enable:
            0 - Hardware decodes NCQ read/write commands as in VT3492,
            1 - Software decodes all ATA commands;
        Bit[6] -- SATA COMINIT ready:
            0 - Clarifies software configuration has not got ready for a COMRESET, and
                Prevents PHY from responding COMINIT OOB to host,
            1 - Declares ready status and allows PHY to respond COMINIT OOB;
        Bit[5] -- SATA COMINIT ready source select:
            0 - The COMINIT ready signal is stored in PMU,
            1 - The COMINIT ready signal is stored in SDC
               (when this mode is selected, software is 
                responsible for programming COMINIT ready
                trigger register in SDC on every COMRESET);
        Bit[4] -- SATA DMAC/CMDM block method select:
            0 - SDC blocks DMAC/CMDM from invoking TP layer only on SERROR,
            1 - SDC blocks DMAC/CMDM on both SERROR and PHY entering DR_Reset state;
        Bit[3] -- SATA DMAC reset method select:
            0 - SDMAC would only be reset on software programming its reset register,
            1 - SDMAC would be reset both on software programming and PHY entering DR_Reset state;
        Bit[2] -- SATA CMDM reset method select:
            0 - SCMDM would only be reset on software programming its reset register,
            1 - SCMDM would be reset both on software programming and PHY entering DR_Reset state;
        Bit[1] -- Link layer reset method select:
            0 - Link layer would only be reset on chip global reset signal,
            1 - Link layer would be reset both on global reset signal and PHY entering DR_Reset state;
        Bit[0] -- SATA COMRESET interrupt issue time select:
            0 - SDC would raise a COMRESET interrupt when it detects the first burst of COMRESET OOB,
            1 - SDC would not raise the interrupt until all 6 bursts of COMRESET OOB are received.
     */
    union
    {
        struct {
            U32 bsCOMRSTIntrSel: 1;
            U32 bsLinkRstSel: 1;
            U32 bsCMDMRstSel: 1;
            U32 bsDMACRstSel: 1;
            U32 bsDMACCMDMBlkSel: 1;
            U32 bsCOMINITRdySel: 1;
            U32 bsCOMINITRdyFlg: 1;
            U32 bsCmdSwDecEn: 1;
            U32 bsOOBRespInRstEn: 1;
            U32 bsPHYASREn: 1;
            U32 bsReg10Rsvd1: 22;
        } tSATAMiscCfg;

        U32 ulSATAMiscCfg;
    };

    /* 6. REG14: PLL gating control
     Bit[31:8] -- Reserved;
     Bit[7] -- MPLL filter enable, RW:
         0 - Disabled (as legacy),
         1 - Enabled (MPLL PDB will be sampled by MPLL clock to eliminate the glitch);
     Bit[6] -- NFPLL gating bypass enable, RW:
         0 - Disabled (NFPLL gating is enabled),
         1 - Enabled (NFPLL gating is bypassed);
     Bit[5] -- HPLL gating bypass enable, RW:
         0 - Disabled (HPLL gating is enabled),
         1 - Enabled (HPLL gating is bypassed);
     Bit[4] -- DPLL gating bypass enable, RW:
         0 - Disabled (DPLL gating is enabled),
         1 - Enabled (DPLL gating is bypassed);
     Bit[3] -- Reserved;
     Bit[2] -- NFPLL output gating enable, RW:
         0 - Disabled (Clock is directly output from NFPLL),
         1 - Enabled (NFPLL would not output clock until NFPLL_READY signal assertion);
     Bit[1] -- HPLL output gating enable, RW:
         0 - Disabled (Clock is directly output from HPLL),
         1 - Enabled (NFPLL would not output clock until HPLL_READY signal assertion);
     Bit[0] -- DPLL output gating enable, RW:
         0 - Disabled (Clock is directly output from DPLL),
         1 - Enabled (NFPLL would not output clock until DPLL_READY signal assertion).
     */
    union
    {
        struct
        {
            U32 bsDPLLOuptGtgEn: 1;
            U32 bsHPLLOuptGtgEn: 1;
            U32 bsNPLLOuptGtgEn: 1;
            U32 bsReg14Rsvd1: 1;
            U32 bsDPLLGtgBypsEn: 1;
            U32 bsHPLLGtgBypsEn: 1;
            U32 bsNPLLGtgBypsEn: 1;
            U32 bsMPLLFltEn: 1;
            U32 bsReg14Rsvd2: 24;
        } tPLLGatingCfg;

        U32 ulPLLGatingCfg;
    };

    /* 7. REG18: PCIe PHY power and clock status/control
    Bit[31:25] -- Reserved;
    Bit[24] -- Reflecting whether PCIe LPHY is in L1 power-saving state, RO:
        0 - PCIe PHY is not in L1, 1 - PCIe PHY is in L1;
    Bit[23:15] -- Reserved;
    Bit[14] - SUSRAM retention load done status, RO:
        0 - Indicates SUSRAM retention loading has not completed,
        1 - Indicates SUSRAM retention loading completed;
    Bit[13] - PMU retention mode status, RO:
        0 - Indicates PMU is not in retention mode,
        1 - Indicates PMU is in retention mode;
    Bit[12] - EPHY BGP/REFCLK slicer power-off enable in L1.2 power-off state, RW:
        0 - The power of BGP/REFCLK slicer is controlled by LPHY (always on),
        1 - The power of BGP/REFCLK slicer is controlled by PSO_EN and would be turned off in L1.2 power-off state;
    Bit[11] -- PCIe power-off mode select, RW:
        0 - ISO_EN/RST_EN would not be asserted when software initiates a PCIe power-off sequence,
        1 - Asserts ISO_EN/RST_EN when software initiates a PCIe power-off sequence;
    Bit[10] -- EPHY TPLL power-off enable, RW:
        0 - The power of TPLL is controlled by LPHY,
        1 - The power of TPLL is controlled by PMU and can be shut down when software programs Reg04[7];
    Bit[9] -- EPHY MPLL power-off enable, RW:
        0 - The power of MPLL is controlled by LPHY,
        1 - The power of MPLL is controlled by PMU and can be shut down when software programs Reg04[7];
    Bit[8] -- Automatic clock gating control enable for PCIe IP in L1 state, RW:
        0 - Clock gating for PCIe IP in L1 state is not allowed,
        1 - Clock gating for PCIe IP in L1 state is allowed;
    Bit[7] -- Interrupt time select on HPLL power resuming, RW:
        0 - PMU would not raise an interrupt when it detects a squelch signal in serial link until HPLL is ready;
        1 - PMU would raise an interrupt when it detects a squelch signal in serial link without waiting for HPLL to get ready,
    Bit[6] -- Resetting EPHY RTN tune function enable, RW:
        0 - Disable, 1 - Enable;
    Bit[5] -- Internal oscillator usage select for EPHY, RW:
        0 - EPHY only uses internal oscillator as clock input in low-power mode,
        1 - EPHY uses internal oscillator as clock input both in low-power mode and active mode;
    Bit[4] -- PCIe PME Turn-Off ACK ready output to PCIe IP, RW:
        0 - Prevents PCIe endpoint from responding PM_PME_TO_Ack to upstream port,
        1 - Allows PCIe endpoint to sending PM_PME_TO_Ack message to upstream port;
    Bit[3] -- SQDET signal waking up PLL enable, RW:
        0 - Disable, 1 - Enable;
    Bit[2] -- CLKREQ# signal waking up PLL enable, RW:
        0 - Disable, 1 - Enable;
    Bit[1] -- CLKREQ# signal hardware control enable, RW:
        0 - CLKREQ# is controlled by software,
        1 - CLKREQ# is controlled by hardware.
    Bit[0] -- CLKREQ# signal tri-state control (this bit is valid only when Bit[1] is 0), RW:
        0 - CLKREQ# in input mode,
        1 - CLKREQ# in output mode.
     */
    union
    {
        struct
        {
            U32 bsCLKREQCntl: 1;
            U32 bsCLKREQHwCntlEn: 1;
            U32 bsWkupByCLKREQEn: 1;
            U32 bsWkupBySQDETEn: 1;
            U32 bsPCIePMUTOACK_F0: 1;
            U32 bsIntOSCUsgSel: 1;
            U32 bsRstPHYRTNEn: 1;
            U32 bsAsstIntrOnHPLLRdy: 1;
            U32 bsPCIeL1ClkGtgEn: 1;
            U32 bsMPLLPwrOffEn: 1;
            U32 bsTPLLPwrOffEn: 1;
            U32 bsRstEnInPCIePwrOff: 1;
            U32 bsBGPSlcPwrOffEn: 1;
            U32 bsPMUIsInRetMod: 1;
            U32 bsRetLoadCplt: 1;
            U32 bsReg18Rsvd1: 9;
            U32 bsPHYinL1State: 1;
            U32 bsReg18Rsvd2: 7;
        } tPCIePHYCfg;

        U32 ulPCIePHYCfg;
    };

    /* 8. REG1C: PCIe reset related status and control
    Bit[31:11] -- Reserved;
    Bit[10] -- The pending status of reset signal of HCT triggered by a PCIe reset, RW0C:
        0 - HCT is not in reset state,
        1 - The reset signal of HCT is asserted by hardware due to a PCIe reset;
        Software must write a 0 to this bit to de-assert the reset signal.
    Bit[9] -- The pending status of reset signal of HOSTC triggered by a PCIe reset, RW0C:
        0 - HOSTC is not in reset state,
        1 - The reset signal of HOSTC is asserted by hardware due to a PCIe reset;
        Software must write a 0 to this bit to de-assert the reset signal.
    Bit[8] -- PCIe out-band reset signal input pin enable, RW:
        0 - Disable, 1 - Enable;
    Bit[7] -- PCIe in-band reset triggered HCT reset enable, RW:
        0 - HCT reset signal would not be asserted due to a PCIe in-band reset,
        1 - PCIe in-band reset would cause hardware to assert HCT reset signal;
    Bit[6] -- PCIe out-band reset triggered HCT reset enable, RW:
        0 - HCT reset signal would not be asserted due to a PCIe out-band reset,
        1 - PCIe out-band reset would cause hardware to assert HCT reset signal;
    Bit[5] -- PCIe in-band reset triggered MMIO reset enable, RW:
        0 - MMIO reset signal would not be asserted due to a PCIe in-band reset,
        1 - PCIe in-band reset would cause hardware to assert MMIO reset signal;
    Bit[4] -- PCIe out-band reset triggered MMIO reset enable, RW:
        0 - MMIO reset signal would not be asserted due to a PCIe out-band reset,
        1 - PCIe out-band reset would cause hardware to assert MMIO reset signal;
    Bit[3] -- PCIe in-band reset triggered HOSTC reset enable, RW:
        0 - HOSTC reset signal would not be asserted due to a PCIe in-band reset,
        1 - PCIe in-band reset would cause hardware to assert HOSTC reset signal;
    Bit[2] -- PCIe out-band reset triggered HOSTC reset enable, RW:
        0 - HOSTC reset signal would not be asserted due to a PCIe out-band reset,
        1 - PCIe out-band reset would cause hardware to assert HOSTC reset signal;
    Bit[1] -- PCIe in-band reset triggered HOSTC downstream reset enable, RW:
        0 - HOSTC downstream reset signal would not be asserted due to a PCIe in-band reset,
        1 - PCIe in-band reset would cause hardware to assert HOSTC downstream reset signal;
        Warning: Generally this bit shall not be set.
    Bit[0] -- PCIe out-band reset triggered HOSTC downstream reset enable, RW:
        0 - HOSTC downstream reset signal would not be asserted due to a PCIe out-band reset,
        1 - PCIe out-band reset would cause hardware to assert HOSTC downstream reset signal.
     */
    union
    {
        struct
        {
            U32 bsObdRstDnstrmEn: 1;
            U32 bsIbdRstDnstrmEn: 1;
            U32 bsObdRstHOSTCEn: 1;
            U32 bsIbdRstHOSTCEn: 1;
            U32 bsObdRstMMIOEn: 1;
            U32 bsIbdRstMMIOEn: 1;
            U32 bsObdRstHCTEn: 1;
            U32 bsIbdRstHCTEn: 1;
            U32 bsObdRstPinEn: 1;
            U32 bsHOSTCPERstSts: 1;
            U32 bsHCTPERstSts: 1;
            U32 bsReg1CRsvd1: 21;
        } tPCIeResetCfg;

        U32 ulPCIeResetCfg;
    };

    /* 9. REG20: Hardware internal clock configuration
    Bit[31:7] -- Reserved;
    Bit[6] -- HCLK source select, RW:
        0 - Normal HCLK, 1 - 25MHz backup clock provided by PMU;
    Bit[5:4] -- NFC ECC clock select, RW:
        X0 - 100MHz, X1 - 200MHz;
    Bit[3:2] -- NFC clock select in slow mode, RW:
        0 - 6.25MHz, 1 - 3.125MHz, 2 - 1.5625MHz, 3 - 0.78125MHz;
    Bit[1:0] -- NFC clock select in normal mode, RW:
        0 - Normal 12.5MHz, 1 - Normal 50MHz,
        2 - ONFI 100MHz, 3 - ONFI 200MHz.
     */
    union
    {
        struct
        {
            U32 bsNFCLKSel: 2;
            U32 bsNFCLKSlowSel: 2;
            U32 bsECCCLKSel: 2;
            U32 bsHCLK25En: 1;
            U32 bsReg20Rsvd1: 25;
        } tHwClkCfg;

        U32 ulHwClkCfg;
    };

    /* 10. REG24: Watchdog control
    Bit[31:7] -- Reserved;
    Bit[6:4] -- Watchdog timer period select, RW:
        0 - 100ms, 1 - 200ms, 2 - 300ms, 3 - 400ms, 4 - 500ms;
    Bit[3:2] -- Reserved;
    Bit[1] -- Watchdog timer reset (feeding dog), WO:
        0 - No effect,
        1 - Watchdog timer is cleared to zero.
    Bit[0] -- Watchdog timer enable, RW:
        0 - Watchdog is disabled,
        1 - Watchdog is enabled and starts to count down.
     */
    union
    {
        struct
        {
            U32 bsWchDogEn: 1;
            U32 bsFeedDog: 1;
            U32 bsReg24Rsvd1: 2;
            U32 bsWchDogCntSel: 3;
            U32 bsReg24Rsvd2: 25;
        } tWatchDogCfg;

        U32 ulWatchDogCfg;
    };

    /* 11. REG28: Real time clock control
    Bit[31:24] -- The upper 8 bits of the RTC counter value, RO/RW;
    Bit[23:1] -- Reserved;
    Bit[0] -- RTC enable, RW:
        0 - RTC is disabled and would not run,
        1 - RTC is enabled to count down.
     */
    union
    {
        struct
        {
            U32 bsRTCEn: 1;
            U32 bsReg28Rsvd1: 23;
            U32 bsRTCCounterHi: 8;
        } tRTCCtrl;

        U32 ulRTCCtrl;
    };

    /* 12. REG2C: Real time clock counter value
    Bit[31:0] -- The lower 32 bits of the RTC counter value, RO/RW.
        Attention: The RTC counter value can only be programmed by software while RTC is disabled.
        It is read-only while RTC is running.
     */
    U32 ulRTCCounterLo;

    /* 13. REG30: Thermal sensor status/control
    Bit[31:20] -- Reserved;
    Bit[19:8] -- The threshold value of built-in thermal sensor, RW:
        When the temperature value input from built-in thermal sensor is beyond it,
        thermal sensor would generate an interrupt to notify MCU;
    Bit[7:2] -- Reserved;
    Bit[1] -- External sensor enable, RW:
        0 - Disable, 1 - Enable;
     Bit[0] -- Built-in sensor enable, RW:
         0 - Disable, 1 - Enable.
     */
    union
    {
        struct
        {
            U32 bsIntSnsrEn: 1;
            U32 bsExtSnsrEn: 1;
            U32 bsReg30Rsvd1: 6;
            U32 bsIntSnrThrshd: 12;
            U32 bsReg30Rsvd2: 12;
        } tThmlSnrCfg;

        U32 ulThmlSnrCfg;
    };

    /* 14. REG34: Power detector status/control
    Bit[31:10] -- Reserved;
    Bit[9] -- Initiates power-off or/and reset protection sequence (depending on Bit[3:2])
        when an abrupt power-loss is detected, Write 1 to Trigger;
    Bit[8] -- Hardware automatic initiating power-off or reset sequence enable on an abrupt power-loss, RW:
        0 - Disabled (hardware would not trigger a power-off or reset sequence),
        1 - Enabled (hardware would automatic trigger power-off and reset on an abrupt power-loss;
    Bit[7:5] -- The threshold value for built-in power detector, RW:
        0 - 1.5V, 1 - 1.7V, 2 - 1.9V, 3 - 2.1V,
        4 - 2.3V, 5 - 2.5V, 6 - 2.7V, 7 - 2.9V;
    Bit[4] -- Power detector global enable, RW:
        0 - Disable,
        1 - Enable;
    Bit[3] -- Power-off enable in protection sequence for power failure detection, RW:
        0 - Disable,
        1 - Enable;
    Bit[2] -- Chip-reset enable in protection sequence for power failure detection, RW:
        0 - Disable,
        1 - Enable;
    Bit[1] -- External power detector enable, RW:
        0 - Disable,
        1 - Enable;
    Bit[0] -- Built-in power detector enable, RW:
        0 - Disable,
        1 - Enable.
     */
    union
    {
        struct
        {
            U32 bsIntlDetrEn: 1;
            U32 bsExtDetrEn: 1;
            U32 bsRstEnForProt: 1;
            U32 bsPwrOffEnForProt: 1;
            U32 bsPwrDetGblEn: 1;
            U32 bsIntlDetrThld: 3;
            U32 bsHwProtSeqEnOnPwrFail: 1;
            U32 bsProtSeqTrgOnPwrFail: 1;
            U32 bsReg34Rsvd1: 22;
        } tPwrDetrCfg;

        U32 ulPwrDetrCfg;
    };

    /* 15. REG38/3C: Reserved. */
    /******** Yao Patched:*************/
    /* Due to A0 PMU design bug, REG74-8C cannot be accessed by firmware.
       So we have to use this two registers to replace signature and resume entry. */
    U32 ulPwrOnSgnt;
    U32 ulResumeEntry;
    //U32 ulReg38Rsvd;
    //U32 ulReg3CRsvd;
    /******** Patch End****************/

    /* 16. REG40: PMU interrupt status/control
    Bit[31:28] -- Reserved;
    Bit[27:16] -- PMU interrupt pending status (Bit-mapped), RW1C:
        Bit[27] - PCIe in-band reset is detected,
        Bit[26] - PCIe out-band reset is detected,
        Bit[25] - Power failure alert has been raised by voltage detector,
        Bit[24] - Chip overheat alert has been raised by temperature sensor,
        Bit[23] - Core power down sequence aborted due to a squelch signal in serial link,
        Bit[22] - PLL shutting down sequence aborted due to a waking-up event,
        Bit[21] - DEVSLP or CLKREQ# signal deasserted by host,
        Bit[20] - DEVSLP or CLKREQ# signal asserted by host,
        Bit[19] - A squelch signal in serial link is detected,
        Bit[18] - PMU Timer 2 expired,
        Bit[17] - PMU Timer 1 expired,
        Bit[16] - PMU Timer 0 expired.
        This field is bit-significant and can only be cleared by writing a 1 to corresponding bit.
        Warning: Any read-modify-write behavior to this field may cause interrupt lost.
    Bit[15:12] -- Reserved;
    Bit[11:0] -- PMU interrupt mask control (Bit-mapped), RW:
        This field is also bit-significant and has the same mapping as Bit[27:16].
     */
    union
    {
        struct
        {
            U32 bsIntrMsk: 12;
            U32 bsReg40Rsvd1: 4;
            U32 bsIntrSts: 12;
            U32 bsReg40Rsvd2: 4;
        } tIntrControl;

        U32 ulIntrControl;
    };

    /* 17. REG44 - 4F: Control register of PMU Timer 0
    REG44:
    Bit[31:9] -- Reserved;
    Bit[8] -- Timer 0 start/stop control, RW:
        0 - Allows timer to count down, 1 - Stops timer;
    Bit[7:5] -- Reserved;
    Bit[4] -- Clock frequency input select, RW:
        0 - 25MHz, 1 - 25MHz dividing by 128;
    Bit[3:2] -- Reserved;
    Bit[1:0] -- Counting mode select, RW:
        00 - Timer disabled,
        01 - Loop mode (Timer restarts counting on overflow),
        1x - One-time mode (Timer stops counting on overflow);
    REG48: The match value for Timer 0 (counting period length), RW;
    REG4C: The current counter value of Timer 0, RW.
     */
    union
    {
        struct
        {
            U32 bsModeSel: 2;
            U32 bsReg44Rsvd1: 2;
            U32 bsClkFreqSel: 1;
            U32 bsReg44Rsvd2: 3;
            U32 bsStop: 1;
            U32 bsReg44Rsvd3: 23;
            U32 ulMchVal;
            U32 ulCurrVal;
        } tTimer0Control;

        U32 ulTimer0Control[3];
    };

    /* 18. REG50 - 5B: Control register of PMU Timer 1
    REG50:
    Bit[31:9] -- Reserved;
    Bit[8] -- Timer 1 start/stop control, RW:
        0 - Allows timer to count down, 1 - Stops timer;
    Bit[7:5] -- Reserved;
    Bit[4] -- Clock frequency input select, RW:
        0 - 25MHz, 1 - 25MHz dividing by 128;
    Bit[3:2] -- Reserved;
    Bit[1:0] -- Counting mode select, RW:
        00 - Timer disabled,
        01 - Loop mode (Timer restarts counting on overflow),
        1x - One-time mode (Timer stops counting on overflow);
    REG54: The match value for Timer 1 (counting period length), RW;
    REG58: The current counter value of Timer 1, RW.
     */
    union
    {
        struct
        {
            U32 bsModeSel: 2;
            U32 bsReg50Rsvd1: 2;
            U32 bsClkFreqSel: 1;
            U32 bsReg50Rsvd2: 3;
            U32 bsStop: 1;
            U32 bsReg50Rsvd3: 23;
            U32 ulMchVal;
            U32 ulCurrVal;
        } tTimer1Control;

        U32 ulTimer1Control[3];
    };

    /* 19. REG5C - 67: Control register of PMU Timer 2
    REG5C:
    Bit[31:19] -- Reserved;
    Bit[18] -- Timer 2 triggering MPLL resuming enable, RW:
        0 - Timer 2 overflow would not cause MPLL power resuming if MPLL is in power-off state,
        1 - Timer 2 overflow shall initiate a power-up sequence for MPLL if it is in power-off state;
        Note: For SATA solution only.
    Bit[17] -- Timer 2 triggering main PLL resuming enable, RW:
        0 - Timer 2 overflow would not cause HPLL/DPLL/NPLL power resuming if they are in power-off state,
        1 - Timer 2 overflow shall initiate a power-up sequence for HPLL/DPLL/NPLL if they are in power-off state;
    Bit[16] -- Timer 2 triggering core power resuming enable, RW:
        0 - Timer 2 overflow would not cause core power resuming if chip is in Suspending/Hibernating state,
        1 - Timer 2 overflow shall initiate a core power restore sequence if chip is in Suspending/Hibernating state;
    Bit[15:9] -- Reserved;
    Bit[8] -- Timer 2 start/stop control, RW:
        0 - Allows timer to count down, 1 -Stops timer;
    Bit[7:5] -- Reserved;
    Bit[4] -- Clock frequency input select, RW:
        0 - 25MHz, 1 - 25MHz dividing by 128;
    Bit[3:2] -- Reserved;
    Bit[1:0] -- Counting mode select, RW:
        00 - Timer disabled,
        01 -Loop mode (Timer restarts counting on overflow),
        1x - One-time mode (Timer stops counting on overflow);
    REG60: The match value for Timer 2 (counting period length), RW;
    REG64: The current counter value of Timer 2, RW.
     */
    union
    {
        struct
        {
            U32 bsModeSel: 2;
            U32 bsReg5CRsvd1: 2;
            U32 bsClkFreqSel: 1;
            U32 bsReg5CRsvd2: 3;
            U32 bsStop: 1;
            U32 bsReg5CRsvd3: 7;
            U32 bsCorePwrRstoEn: 1;
            U32 bsHPLLRsmEn: 1;
            U32 bsMPLLRsmEn: 1;
            U32 bsReg5CRsvd4: 13;
            U32 ulMchVal;
            U32 ulCurrVal;
        } tTimer2Control;

        U32 ulTimer2Control[3];
    };

    /* 20. REG68: HPLL and DPLL frequency control
    Bit[31:29] -- Reserved;
    Bit[28] -- HPLL bypass mode enable, RW:
        0 - Bypass mode is disabled,
        1 - Bypass mode is enabled;
    Bit[27:26] -- Reserved;
    Bit[25:24] -- HPLL output divider select, RW;
    Bit[23] -- HPLL input divider select, RW;
    Bit[22:16] -- HPLL feedback divider select, RW;
    Bit[15:13] -- Reserved;
    Bit[12] -- DPLL bypass mode enable, RW:
        0 - Bypass mode is disabled,
        1 - Bypass mode is enabled;
    Bit[11:10] -- Reserved;
    Bit[9:8] -- DPLL output divider select, RW;
    Bit[7] -- DPLL input divider select, RW;
    Bit[6:0] -- DPLL feedback divider select, RW.
     */
    union
    {
        struct
        {
            U32 bsDPLLFbDivSel: 7;
            U32 bsDPLLInDivSel: 1;
            U32 bsDPLLOutDivSel: 2;
            U32 bsReg68Rsvd1: 2;
            U32 bsDPLLBypsEn: 1;
            U32 bsReg68Rsvd2: 3;
            U32 bsHPLLFbDivSel: 7;
            U32 bsHPLLInDivSel: 1;
            U32 bsHPLLOutDivSel: 2;
            U32 bsReg68Rsvd3: 2;
            U32 bsHPLLBypsEn: 1;
            U32 bsReg68Rsvd4: 3;
        } tHnDPLLFreqCfg;

        U32 ulHnDPLLFreqCfg;
    };

    /* 21. REG6C: NPLL frequency control
    Bit[31:13] -- Reserved;
    Bit[12] -- NPLL bypass mode enable, RW:
        0 - Bypass mode is disabled,
        1 - Bypass mode is enabled;
    Bit[11:10] -- Reserved;
    Bit[9:8] -- NPLL output divider select, RW;
    Bit[7] -- NPLL input divider select, RW;
    Bit[6:0] -- NPLL feedback divider select, RW.
     */
    union
    {
        struct
        {
            U32 bsFdbkDivSel: 7;
            U32 bsInptDivSel: 1;
            U32 bsOuptDivSel: 2;
            U32 bsReg6CRsvd1: 2;
            U32 bsBypsModEn: 1;
            U32 bsReg6CRsvd2: 3;
            U32 bsPMUPCIeMode: 1;
            U32 bsReg6CRsvd3: 11;
            U32 bsSftyPrtOnCLKREQ: 1;
            U32 bsBlkCLKREQinPwrOffSeq: 1;
            U32 bsCLKREQFltr: 2;
        } tNPLLFreqCfg;

        U32 ulNPLLFreqCfg;
    };

    /* 22. REG70: PMU retention timer configuration
    Bit[31:8] -- Reserved;
    Bit[7:0] -- Retention timer counter value, RW.
     */
    union
    {
        struct
        {
            U32 bsTmrCntVal: 8;
            U32 bsReg70Rsvd1: 24;
        } tRttnTmrCfg;

        U32 ulRttnTmrCfg;
    };

    /* 23. REG74 - REG8C: Scratch registers for software usage, RW:
    REG74 -- The entry of resuming routine in DDR SDRAM;
    REG78 -- Power-on signature for distinguishing a Hibernate resuming from a cold boot;
    REG7C - REG8C -- Reserved for future usage.
     */
    /******** Yao Patched:*************/
    /* Due to A0 PMU design bug, REG74-8C cannot be accessed by firmware.
       So we have to use this two registers to replace signature and resume entry. */
    //U32 ulResumeEntry;
    //U32 ulPwrOnSgnt;
    U32 ScrReg[7];
    /******** Patch End****************/
} PMUREGSET, *PPMUREGSET;

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

/* Level 1 clock gating control bitmaps */
#define PMU_L1CKG_SGEAHCI 1
#define PMU_L1CKG_AHCIHOSTC 2
#define PMU_L1CKG_HCT 4
#define PMU_L1CKG_OTFBM 8
#define PMU_L1CKG_PCIE 16
#define PMU_L1CKG_ICB 32
#define PMU_L1CKG_USRAM 64
#define PMU_L1CKG_XORE 128
#define PMU_L1CKG_HGE 256
#define PMU_L1CKG_DAA 512
#define PMU_L1CKG_LED (1 << 13)
#define PMU_L1CKG_HOSTCRC (1 << 14)

#define PMU_L1CKG_NFCCH0 1
#define PMU_L1CKG_NFCCH1 2
#define PMU_L1CKG_NFCCH2 4
#define PMU_L1CKG_NFCCH3 8
#define PMU_L1CKG_NFCSLOW 16
#define PMU_L1CKG_NFCMISC 32
#define PMU_L1CKG_NFCECC 64
#define PMU_L1CKG_NFCLDPC 128
#define PMU_L1CKG_SDCBASE 256
#define PMU_L1CKG_SDCMISC 512
#define PMU_L1CKG_DRAMC 1024
#define PMU_L1CKG_DCLKPH2 2048
#define PMU_L1CKG_OTFB 4096
#define PMU_L1CKG_TRFC 8192
#define PMU_L1CKG_BUFFMAP 16384
#define PMU_L1CKG_PMU 32768
#define PMU_L1CKG_DMAE (1 << 17)
#define PMU_L1CKG_EM (1 << 18)
#define PMU_L1CKG_APB (1 << 19)
#define PMU_L1CKG_SE0 (1 << 20)
#define PMU_L1CKG_SE1 (1 << 21)
#define PMU_L1CKG_SNFC (1 << 22)
#define PMU_L1CKG_TEST (1 << 23)
#define PMU_L1CKG_SGE (1 << 24)
#define PMU_L1CKG_SE2 (1 << 25)
#define PMU_L1CKG_MCU0 (1 << 28)
#define PMU_L1CKG_MCU1 (1 << 29)
#define PMU_L1CKG_MCU2 (1 << 30)
#define PMU_L1CKG_GLB (1 << 31)

#ifndef HOST_SATA
#define PMU_L1CKG_ENABLE (PMU_L1CKG_NFCCH0 | PMU_L1CKG_NFCCH1 | PMU_L1CKG_NFCCH2 | PMU_L1CKG_NFCCH3 |\
                          PMU_L1CKG_NFCSLOW | PMU_L1CKG_NFCMISC | PMU_L1CKG_NFCLDPC | PMU_L1CKG_SNFC |\
                          PMU_L1CKG_OTFB | PMU_L1CKG_TRFC | PMU_L1CKG_SGE | PMU_L1CKG_DRAMC |\
                          PMU_L1CKG_DCLKPH2 | PMU_L1CKG_APB | PMU_L1CKG_MCU1 | PMU_L1CKG_MCU2)

#if 0 /* Used with hardware managed L1 clock gating enabled. */
#define PMU_L1CKG_ENABLE (PMU_L1CKG_SNFC | PMU_L1CKG_OTFB | PMU_L1CKG_TRFC | PMU_L1CKG_SGE |\
                          PMU_L1CKG_DRAMC | PMU_L1CKG_DCLKPH2 | PMU_L1CKG_APB | PMU_L1CKG_MCU1 |\
                          PMU_L1CKG_MCU2)
#endif

#define PMU_L1CKG_ENABLE2 (PMU_L1CKG_SGEAHCI | PMU_L1CKG_HCT | PMU_L1CKG_OTFBM | PMU_L1CKG_USRAM | PMU_L1CKG_LED)
#else
#define PMU_L1CKG_ENABLE (PMU_L1CKG_NFCCH0 | PMU_L1CKG_NFCCH1 | PMU_L1CKG_NFCCH2 | PMU_L1CKG_NFCCH3 |\
                          PMU_L1CKG_NFCSLOW | PMU_L1CKG_NFCMISC | PMU_L1CKG_NFCLDPC | PMU_L1CKG_SNFC |\
                          PMU_L1CKG_BUFFMAP | PMU_L1CKG_OTFB | PMU_L1CKG_TRFC | PMU_L1CKG_APB |\
                          PMU_L1CKG_DRAMC | PMU_L1CKG_DCLKPH2 | PMU_L1CKG_MCU1 | PMU_L1CKG_MCU2)

#if 0 /* Used with hardware managed L1 clock gating enabled. */
#define PMU_L1CKG_ENABLE (PMU_L1CKG_SNFC | PMU_L1CKG_BUFFMAP | PMU_L1CKG_OTFB | PMU_L1CKG_TRFC |\
                          PMU_L1CKG_DRAMC | PMU_L1CKG_DCLKPH2 | PMU_L1CKG_APB | PMU_L1CKG_MCU1 |\
                          PMU_L1CKG_MCU2)
#endif

#define PMU_L1CKG_ENABLE2 (PMU_L1CKG_USRAM | PMU_L1CKG_LED)
#endif

#define PMU_L1CKG_SUBSYS (PMU_L1CKG_MCU1 | PMU_L1CKG_MCU2)

/* Hardware register block base address */
#define PMU_RESUME_ENTRY_ADDRESS (REG_BASE_PMU + 0x3C)

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

// Useful timer tick counts
#define PMU_TIMER_TICKCOUNT_1US  25
#define PMU_TIMER_TICKCOUNT_1MS (PMU_TIMER_TICKCOUNT_1US * 1000u)
#define PMU_TIMER_TICKCOUNT_1S  (PMU_TIMER_TICKCOUNT_1MS * 1000u)

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
#define PMU_INTBIT_CHIP_OVERHEAT 8
#define PMU_INTBIT_CHIP_PWRFAILURE 9
#define PMU_INTBIT_PCIE_OBDRST 10
#define PMU_INTBIT_PCIE_IBDRST 11
#define PMU_INT_TIMER0 ( 1 << PMU_INTBIT_TIMER0 )
#define PMU_INT_TIMER1 ( 1 << PMU_INTBIT_TIMER1 )
#define PMU_INT_TIMER2 ( 1 << PMU_INTBIT_TIMER2 )
#define PMU_INT_SQUELCH ( 1 << PMU_INTBIT_SQUELCH )
#define PMU_INT_DEVSLP_ASST ( 1 << PMU_INTBIT_DEVSLP_ASST )
#define PMU_INT_DEVSLP_DEASST ( 1 << PMU_INTBIT_DEVSLP_DEASST )
#define PMU_INT_PLL_BLKD ( 1 << PMU_INTBIT_PLL_BLKD )
#define PMU_INT_PWR_BLKD ( 1 << PMU_INTBIT_PWR_BLKD )
#define PMU_INT_CHIP_OVERHEAT ( 1 << PMU_INTBIT_CHIP_OVERHEAT )
#define PMU_INT_CHIP_PWRFAILURE ( 1 << PMU_INTBIT_CHIP_PWRFAILURE )
#define PMU_INT_PCIE_OBDRST ( 1 << PMU_INTBIT_PCIE_OBDRST )
#define PMU_INT_PCIE_IBDRST ( 1 << PMU_INTBIT_PCIE_IBDRST )

/* Register trigger bit mapping definition for system-level power management state transition */
#define PMU_LITSLP_BIT 1
#define PMU_SATA_DPSLP_BIT 2
#define PMU_PCIE_DPSLP_BIT 4
#define PMU_DPLL_OFF_BIT 16
#define PMU_HPLL_OFF_BIT 32
#define PMU_NPLL_OFF_BIT 64
#define PMU_MPLL_OFF_BIT 128
#define PMU_DPLL_ON_BIT 256
#define PMU_HPLL_ON_BIT 512
#define PMU_NPLL_ON_BIT 1024
#define PMU_MPLL_ON_BIT 2048

#define PMU_DPLL_OK_FLAG (1 << 12)
#define PMU_HPLL_OK_FLAG (1 << 13)
#define PMU_NPLL_OK_FLAG (1 << 14)
#define PMU_MPLL_OK_FLAG (1 << 15)
#define PMU_DPLL_RDY_FLAG (1 << 16)
#define PMU_HPLL_RDY_FLAG (1 << 17)
#define PMU_NPLL_RDY_FLAG (1 << 18)
#define PMU_MPLL_RDY_FLAG (1 << 19)
#define PMU_INITIATE_PLLOFF (PMU_DPLL_OFF_BIT | PMU_HPLL_OFF_BIT | PMU_NPLL_OFF_BIT | PMU_MPLL_OFF_BIT)
#define PMU_INITIATE_PLLON (PMU_DPLL_ON_BIT | PMU_HPLL_ON_BIT | PMU_NPLL_ON_BIT)
#define PMU_INITIATE_LITSLP (PMU_INITIATE_PLLOFF | PMU_LITSLP_BIT)
#ifdef HOST_SATA
//#define PMU_INITIATE_PLLOFF (PMU_DPLL_CNTL_BIT | PMU_NPLL_CNTL_BIT | PMU_MPLL_CNTL_BIT | PMU_HPLL_CNTL_BIT)
#define PMU_INITIATE_DPSLP (PMU_INITIATE_PLLOFF | PMU_SATA_DPSLP_BIT)
#else
//#define PMU_INITIATE_PLLOFF (PMU_DPLL_CNTL_BIT | PMU_NPLL_CNTL_BIT | PMU_HPLL_CNTL_BIT)
#define PMU_INITIATE_DPSLP (PMU_INITIATE_PLLOFF | PMU_PCIE_DPSLP_BIT)
#endif

#ifdef XTMP
#define TIME_BETWEEN_CHECK_DEVSLP  1//100000  // 4ms
#else
#define TIME_BETWEEN_CHECK_DEVSLP  100000  // 4ms
#endif

#define WATCH_DOG_PERIOD_100MS  0
#define WATCH_DOG_PERIOD_200MS  1
#define WATCH_DOG_PERIOD_300MS  2
#define WATCH_DOG_PERIOD_400MS  3
#define WATCH_DOG_PERIOD_500MS  4

/* PM defined global variables declaration */

/* PM interface routines declaration */
void HAL_PMInit(void);
SYSPM_STATE HAL_PMCheckBootType(void);
INLINE U32 HAL_PMGetSignature(void);
INLINE void HAL_PMSetSignature(U32);
void HAL_PMStartTimer(U32, U32);
void HAL_PMStopTimer(U32 ulTimerID);
U32 HAL_PMGetTimerTickPerMS(void);
U32 HAL_PMGetSafetyBlock(void);
void HAL_PMClearSafetyBlock(void);
void HAL_PMDozeOffMCU(U32);
U32 HAL_PMIsPCIeInLowPwrSts(void);
void HAL_PMShutDownPLL(void);
U32 HAL_PMIsMainPLLReady(void);
void HAL_PMInitiateSuspending(void);
void HAL_PMDisableSuspending(void);
void HAL_PMClearTimeOutFlag(U32);
U32 HAL_PMIsTimerTimedOut(U32);
INLINE void HAL_PMSetDevSleepPinEn(BOOL bEnable);
INLINE BOOL HAL_PMGetDevSleepFlag(void);
INLINE void HAL_PMSetDevSleepFlag(void);
INLINE void HAL_PMClearDevSleepFlag(void);
INLINE U32 HAL_PMGetSqlchFlag(void);
INLINE void HAL_PMSetSqlchFlag(const U32 ulFlag);
INLINE void HAL_PMEnableHWL12Ctrl(U32 ulEnableOrDisable);
void HAL_PMEnablePCIeL12Ready(void);
void HAL_PMDisablePCIeL12Ready(void);
void HAL_PMEnableDDRSelfRefresh(U32 ulEnableOrDisable);
void HAL_PMEnableL1ClockGating(U32 ulEnableOrDisable);
U32 HAL_PMInitiateStandby(void);
void HAL_PMEnablePCIeASPM(U32 ulEnableOrDisable);
U32 HAL_PMExitStandby(void);
void HAL_PMEnablePLL(U32);
void HAL_PMEnableWatchDog(U32 ulPeriodSel);
void HAL_PMDisableWatchDog(void);
void HAL_PMFeedWatchDog(void);
void HAL_PMPrintRTC(void);

#endif

/********************** FILE END ***************/


