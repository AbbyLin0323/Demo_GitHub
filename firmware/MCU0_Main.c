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
Filename    :MCU0_main.c
Version     :Ver 1.0
Author      :YaoChen
Date        :20140707
Description :Main entry&loop for MCU0 in VT3514 AHCI mode.
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "Proj_Config.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#include "HAL_TraceLog.h"
#include "L0_Interface.h"
#if defined(HOST_NVME)||defined(HOST_AHCI)
#include "HAL_HwDebug.h"
#endif
#ifndef SIM
#include "HAL_Interrupt.h"
#include "L0_Uart.h"
#endif

#define TL_FILE_NUM MCU0_Main_c

void L0_Init(void);
void L0_Schedule(void);

#ifdef HCMD_DEBUG
struct HCMDDBG debug_hcmd[DEBUG_HCMD_NUM];
U8 g_ucWritePointer;
U8 g_ucCheckPointer;

void hcmd_debug_init(void)
{
    g_ucWritePointer = 0;
    g_ucCheckPointer = 0;
}
#endif

#ifndef SIM
volatile U32 g_ulDbgEnable;

/****************************************************************************
Name        :DBG_Getch
Input       :None
Output      :None
Author      :HenryLuo
Date        :2012.02.15    15:11:36
Description :Recording context and pending MCU when a fatal error is encountered.
Others      :
Modify      :
****************************************************************************/
void MCU0_DRAM_TEXT DBG_Getch(void)
{
    U32 ulTestLoop = 0;
    DBG_Printf("Fatal Error, DBG_Getch!!!\n");

    while (g_ulDbgEnable)
    {
        ulTestLoop++;
        L0_UartDBG();
    }
    g_ulDbgEnable = 1;
}
#endif

/****************************************************************************
Name        :FrontEnd_Init
Input       :None
Output      :None
Author      :
Date        :
Description : Init AHCI interface, control subsystem to init.
Others      :
Modify      :
****************************************************************************/
void FrontEnd_Init(void)
{
    L0_Init();
#ifdef FLASH_TEST
    while(1);
#endif
#if defined(HOST_NVME)||defined(HOST_AHCI)
    HAL_HwDebugInit();
#endif

#ifndef SIM
#ifndef HOST_SATA
    HAL_HostCIntInit();
    HAL_InitInterrupt(TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0 | TOP_INTSRC_HOSTC | TOP_INTSRC_PMU,
                        BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0 | BIT_ORINT_HOSTC | BIT_ORINT_PMU);
#else
    HAL_InitInterrupt(TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0 | TOP_INTSRC_PMU,
                        BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0 | BIT_ORINT_PMU);//SDC interrupt is enabled after sub system boot up
#endif
#endif

    return;
}

/****************************************************************************
Name        :FrontEnd_Schedule
Input       :None
Output      :None
Author      :
Date        :
Description : main worker of MCU0(HostInterface).
Others      :
Modify      :
****************************************************************************/
void FrontEnd_Schedule(void)
{
    TL_PERFORMANCE(PTL_LEVEL_TOP, "L0_Schedule start ");
    L0_Schedule();
    TL_PERFORMANCE(PTL_LEVEL_TOP, "L0_Schedule end   ");
    return;
}


#ifndef SIM
#ifdef FLASH_TEST
extern void HAL_StartSubSystemMCU(U8 ucSubSysIdx);
#endif
/****************************************************************************
Name        :main
Input       :None
Output      :None
Author      :
Date        :
Description : Main loop for MCU0. In VT3514 AHCI mode, MCU0 is the interface to host.
Others      :
Modify      :
****************************************************************************/
int main(void)
{
#ifdef HCMD_DEBUG
    hcmd_debug_init();
#endif

#ifndef SIM
    if (HAL_GetPrinttoDRAMEnFlag())
    {
        dbg_putchar_init();
    }
#endif

    g_ulDbgEnable = 1;  
    FrontEnd_Init();    
#ifdef HAL_NFC_TEST
    while(1);
#else
    while (TRUE)
    {
        FrontEnd_Schedule();
    }
#endif
    return 0;
}
#endif

/********************** FILE END ***************/


