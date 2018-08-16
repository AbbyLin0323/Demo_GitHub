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

//#define TEST_ACCESS_COMAREA
//#define TEST_ACCESS_ROMPOSTMEM
//#define TEST_ACCESS_ICBBUS

//#define DRAM_REMAP_2G
#define DRAM_REMAP_2G_HIGH
//#define M1DRAM_REMAP_ON


#define DSRAM_BASE_DATA      0x1FF80000
#define DRAM_BASE_DATA       0x40000000
#define OTFB_BASE_DATA       0xFFF00000
#define ISRAM_BASE_DATA      0x20000000
#define APB_BASE_DATA        0xffe00000

#define ICB_BASE_DATA        0x1ff80000
#define DSRAM1_BASE_DATA     0x1ffc0000

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

//for IDcache test
//#define OTFB_MCU0_ATTR      __attribute__ ((section(".otfb_mcu0.text")))
//#define OTFB_MCU1_ATTR      __attribute__ ((section(".otfb_mcu1.text")))
//#define OTFB_MCU2_ATTR      __attribute__ ((section(".otfb_mcu2.text")))

#define ISRAM_MCU1_0_ATTR //__attribute__ ((section(".mcu1_usr0.text")))
#define ISRAM_MCU1_1_ATTR //__attribute__ ((section(".mcu1_usr1.text")))
#define ISRAM_MCU2_0_ATTR //__attribute__ ((section(".mcu2_usr0.text")))
#define ISRAM_MCU2_1_ATTR //__attribute__ ((section(".mcu2_usr1.text")))

#define S_1DW   (0x4)
#define S_1KB   (1024)//0x400
#define S_4KB   (4*S_1KB)//0x1000
#define S_1MB   (1*S_1KB*S_1KB)//0x100000
#define S_8MB   (8*S_1KB*S_1KB)//0x800000
#define S_1GB   (1024*S_1MB)

/*common area define*/
#define COM_DSRAM0_SIZE (68*S_1KB)
#define COM_DSRAM1_SIZE (104*S_1KB)

#define COM_DRAM_SIZE   (1024*S_1MB)
#define COM_DRAM2G_SIZE (2*1024*S_1MB)
//#define COM_DRAM2G_SIZE (2048*S_1MB)
#define COM_DRAM_HIGH_CACHE_SIZE    (1024*S_1MB)
#define COM_DRAM_HIGH2G_CACHE_SIZE  (768*S_1MB)

//#define COM_OTFB_SIZE     (512*S_1KB)
//#define COM_OTFB_SIZE     (384*S_1KB)
#define COM_OTFB_INTER_SIZE (320*S_1KB)
#define COM_OTFB_XOR_SIZE   (65*S_1KB)
//#define OTFB_ALL_SIZE     (704*S_1KB)  //all remap to OTFB
#define OTFB_ALL_SIZE       (576*S_1KB)  //all remap to OTFB


#define COM_DSRAM0_ACCESS_START         (REG_BASE + 0x4000)
#define COM_DSRAM1_ACCESS_START         (SRAM1_START_ADDRESS + 0x8000)
#define COM_DRAM_ACCESS_START           (DRAM_START_ADDRESS)
#define COM_DRAM_HIGH_ACCESS_START      (DRAM_HIGH_START_ADDRESS)
#define COM_DRAM_HIGH2G_ACCESS_START    (0xc0000000)
#define COM_OTFB_ACCESS_START           (OTFB_START_ADDRESS)
#define COM_ROM_POST_ACCESS_START       (0xffe06000)

#define COM_ICB_ACCESS_START            (0x1ff80000)

/*privacy area define*/
#define PRI_MCU0_DSRAM0_SIZE (14*S_1KB)
#define PRI_MCU2_DSRAM0_SIZE (30*S_1KB)
#ifndef M1DRAM_REMAP_ON
#define PRI_MCU1_DSRAM0_SIZE (30*S_1KB)
#else
#define PRI_MCU1_DSRAM0_SIZE (62*S_1KB)
#endif

#define PRI_MCU0_ISRAM_SIZE  (28*S_1KB)
#ifdef M1DRAM_REMAP_ON
#define PRI_MCU1_ISRAM_SIZE  (84*S_1KB)
#else
#define PRI_MCU1_ISRAM_SIZE  (116*S_1KB)
#endif
#define PRI_MCU2_ISRAM_SIZE  (52*S_1KB)

#define PRI_MCU0_CacheDSRAM0_SIZE (16*S_1KB)
#define PRI_MCU0_CacheDSRAM1_SIZE (32*S_1KB)


#define PRI_MCU0_DSRAM0_ACCESS_START (SRAM0_START_ADDRESS + 0x24800)
#define PRI_MCU1_DSRAM0_ACCESS_START (SRAM0_START_ADDRESS + 0x20800)
#define PRI_MCU2_DSRAM0_ACCESS_START (SRAM0_START_ADDRESS + 0x20800)

#define PRI_MCU0_ISRAM_ACCESS_START  (MCU0_ISRAM_BASE + 0x3000)
#define PRI_MCU1_ISRAM_ACCESS_START  (MCU1_ISRAM_BASE + 0x3000)
#define PRI_MCU2_ISRAM_ACCESS_START  (MCU2_ISRAM_BASE + 0x3000)

#define PRI_MCU0_CacheDSRAM0_ACCESS_START (SRAM0_START_ADDRESS + 0x20000)
#define PRI_MCU0_CacheDSRAM1_ACCESS_START (SRAM1_START_ADDRESS)


#endif /* __MCU_BASIC_TEST__ */

