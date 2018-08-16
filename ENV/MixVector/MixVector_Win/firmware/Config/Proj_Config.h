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
*******************************************************************************/
#ifndef __PROJ_CONFIG_H__
#define __PROJ_CONFIG_H__

/* Platform definitions ------------------------------------------------------*/
//#define SIM
//#define XTMP
//#define COSIM
//#define FPGA
//#define ASIC
#define HCLK_FREQ  (50 * 1000000)
//#define VT3514_C0

#define NO_THREAD

#define SINGLE_SUBSYSTEM
#define CE_NUM  4

/* PU_NUM is FW Porcess Unit number, and its value should equal phycial CE Number.
    In L1 and L2 algorithm, only PU_NUM should be called. */
#define PU_NUM  CE_NUM

//#define HOST_SATA
#define HOST_AHCI
//#define HOST_NVME

/* FW function selection -----------------------------------------------------*/
#if (PU_NUM == 16)
#define SUBSYSTEM_SMALL_SRAMMAP
#endif

//#define SSD_SATA_SIMPLE
//#define L0_USELOCALDATAXFER

#ifndef VT3514_C0
#define A0_B0_DSG_SSU_STATUS_PATCH  //This patch is for A0 and B0 HW whole chip. Ramdisk don't need this patch.
#undef L0_USELOCALDATAXFER
#endif
#define EMPTY_PG_DETECT_FW
//#define TRACE_ALL_MODULE_ENABLE 
//#define ECT_TRACE // trace flash chip erase count in ASIC Env.
//#define DBG_TABLE_REBUILD
//#define DBG_DIRTYCNT
//#define TB_CHK
//#define DBG_PMT
//#define CHECK_DIRTYCNT
//#define DATA_MONITOR_ENABLE
#define HAL_NFC_RD_RETRY
#define L3_PROCESS_ACC
//#define HOST_READ_FROM_DRAM
#define SEL_WL_SRC
//#define GLOBAL_INJ_ERROR
//#define GLOBAL_ERROR_HANDLE
//#define ERROR_INJ
//#define PMT_ITEM_SIZE_3BYTE

/* FW unit test selection ----------------------------------------------------*/
//#define L3_LOCAL_TEST
#ifdef L3_LOCAL_TEST
//#define FLASH_DRIVER_TEST_HIGH_LEVEL
#define FLASH_DRIVER_TEST_LOW_LEVEL
#endif
//#define DMAE_TEST
//#define SE_TEST
//#define DMAE_TEST
//#define SPINLOCK_TEST
//#define EM_DATACHECK_TEST

#define MIX_VECTOR
//#define SUB_MODEL_TEST

/* HW Function selection -----------------------------------------------------*/
#define DMAE_ENABLE // Enable firmware to use hardware DMA engine in many general memory copy operations.
//#define DCACHE
//#define SEARCH_ENGINE
//#define NFC_CACHE_READ

/* print/halt/assert for FW debug --------------------------------------------*/
#ifdef SIM
#include <stdio.h>

#define DBG_Printf printf
#define DBG_Getch  __debugbreak
#define ASSERT( _x_ ) { if (FALSE == (_x_) ) __debugbreak();}
#else
#include "uart.h"

#define DBG_Printf dbg_printf
extern void DBG_Getch();
#define ASSERT( _x_ )\
    do{\
        if ( (_x_) == FALSE )\
        {\
            while(1); \
        }\
    }while(0)
#endif

#endif/* __PROJ_CONFIG_H__ */
