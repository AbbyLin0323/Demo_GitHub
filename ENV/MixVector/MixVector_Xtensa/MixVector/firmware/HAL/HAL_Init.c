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
Filename    :HAL_Init.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Initialization code for hardware functions and data structures used by HAL.
Others      :
Modify      :
*******************************************************************************/
#include "HAL_Inc.h"

LOCAL void HalOtfbMemMapInit(void);

/****************************************************************************
Name        :HAL_CommInit
Input       :Starting addresses of memory regions available for HAL usage.
Output      :New starting address of memory regions available after allocating memory space
for HAL data structures. There is no return value..
Author      :
Date        :
Description :This routine initializes hardware functions/modules and may allocate memory
space for data structures used by HAL routines.
Others      :
Modify      :
****************************************************************************/
void MCU12_DRAM_TEXT HAL_CommInit(void)
{
    HalOtfbMemMapInit();

#ifdef DMAE_ENABLE
    HAL_DMAEInit();
#endif

#ifdef SEARCH_ENGINE
    HAL_SEInit();
#endif

#if defined(HOST_NVME)||defined(HOST_AHCI)
    HAL_HwDebugInit();
#endif

    return;
}

#ifdef HOST_SATA
void MCU12_DRAM_TEXT HAL_SATAInit(void)
{
    HAL_NormalDsgInit();
    HAL_SataDsgInit();

    return;
}

#else
void MCU12_DRAM_TEXT HAL_AHCIInit(void)
{
    HAL_NormalDsgInit();
    HAL_HsgInit();
    HAL_DrqInit();
    HAL_DwqInit();
    HAL_SgeInitChainCnt();
    HAL_ChainMaintainInit();

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_EnablePCIeOptionRom
Description: 
    enable OptionRom(EROM) BAR in PCIE configuration header.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Used by MCU0 only.
    In PCIE mode(AHCI/NVMe), call this function in FW cold boot sequence.
    In SATA mode, do not call it.
History:
    20150116    Gavin create
------------------------------------------------------------------------------*/
void HAL_EnablePCIeOptionRom(void)
{
    /* map Option ROM space to DRAM address: DRAM_OPTION_ROM */
    *(volatile U32 *)(REG_BASE_HOSTC + 0x3C) = (DRAM_OPTION_ROM - DRAM_START_ADDRESS);

    /* set rGLB(0x64) bit[3] to 0, which means Option ROM is in DRAM */
    rGlbTrfc &= ~(1 << 3);

    /* enable PCI EROM BAR with 64KB space */
    *(volatile U32 *)(REG_BASE_HOSTC + 0x38) = 0xFFFF0001;

    return;
}
#endif

void HAL_DebugSignalInit(void)
{
    rGlbDbgSigSel = 0x0;
    return;
}

void HAL_PCIEInit(void)
{
    /* rGBG_18[29]: PCIE software reset */
    rGlbSoftRst &= ~(R_RST_PCIE);
    return;
}

/*------------------------------------------------------------------------------
Name: HalOtfbMemMapInit
Description: 
    initialize register about OTFB memory allocation.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in FW init sequence
History:
    20141024    Gavin ported from HAL_NfcGLBRegInit
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HalOtfbMemMapInit(void)
{
    /* remap OTFB memory */ 
#ifdef VT3514_C0
    /*2 OTFB block per Channel, start form ID6 */
    rGlbOTFBMCtrl1 = 0x16a4a0e6;
    rGlbOTFBMCtrl0 = 0x1ac20200;
#else
    /* 1 OTFB block per Channel, start from ID2 */
    rGlbOTFBMCtrl1 = 0x00400c02;
    rGlbOTFBMCtrl0 = 0x00500300;
#endif

    return;
}

