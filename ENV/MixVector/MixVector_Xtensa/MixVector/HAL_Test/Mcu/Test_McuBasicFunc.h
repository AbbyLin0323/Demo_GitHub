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
Filename    : TEST_McuBasicFunc.h
Version     : Ver 1.0
Author      : TobeyTan
Date        : 2013-11-28
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#ifndef __TEST_MCU_BASIC_FUNC_H__
#define __TEST_MCU_BASIC_FUNC_H__

#include "HAL_MemoryMap.h"

#define MCU_NUM   3
#define DRAM_SIZE 0x20000000

#define DRAM_BASE_DATA      0x1FF80000
#define SRAM_BASE_DATA      0x40000000
#define OTFB_BASE_DATA      0xFFF00000

#define rTracer_Mcu0        (*((volatile U8 *)(REG_BASE + 0x80)))
#define rTracer_Mcu1        (*((volatile U8 *)(REG_BASE + 0x80 + 1)))
#define rTracer_Mcu2        (*((volatile U8 *)(REG_BASE + 0x80 + 2)))
#define rTracer_Mcu3        (*((volatile U8 *)(REG_BASE + 0x80 + 3)))

#define rPhaseFlag          (*((volatile U32*)(SRAM1_START_ADDRESS + 0x8000))) //0x1FFC8000
#define rMCU0Done           (*((volatile U32*)(SRAM1_START_ADDRESS + 0x8000 + 4)))
#define rMCU1Done           (*((volatile U32*)(SRAM1_START_ADDRESS + 0x8000 + 8)))
#define rMCU2Done           (*((volatile U32*)(SRAM1_START_ADDRESS + 0x8000 + 12)))

#define MCU0_TOKEN_FLAG     (MCU0_ID + 0x11111111)
#define MCU1_TOKEN_FLAG     (MCU1_ID + 0x11111111)
#define MCU2_TOKEN_FLAG     (MCU2_ID + 0x11111111)
#define MCU0_DONE_FLAG      (MCU0_ID + 0x11111111)
#define MCU1_DONE_FLAG      (MCU1_ID + 0x11111111)
#define MCU2_DONE_FLAG      (MCU2_ID + 0x11111111)
#define MCU_SYNC_fLAG       (MCU0_ID + MCU1_ID + MCU2_ID)
#define MCU_INIT_fLAG       (0x0)

#define SYC_DELAY_CYCLE     (10)

#define OTFB_MCU0_ATTR      //__attribute__ ((section(".otfb_mcu0.text")))
#define OTFB_MCU1_ATTR      //__attribute__ ((section(".otfb_mcu1.text")))
#define OTFB_MCU2_ATTR      //__attribute__ ((section(".otfb_mcu2.text")))
#define ISRAM_MCU1_0_ATTR //__attribute__ ((section(".mcu1_usr0.text")))
#define ISRAM_MCU1_1_ATTR //__attribute__ ((section(".mcu1_usr1.text")))
#define ISRAM_MCU2_0_ATTR //__attribute__ ((section(".mcu2_usr0.text")))
#define ISRAM_MCU2_1_ATTR //__attribute__ ((section(".mcu2_usr1.text")))

#endif /* __MCU_BASIC_TEST__ */

