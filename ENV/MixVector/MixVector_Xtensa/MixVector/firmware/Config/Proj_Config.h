/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :                                           
Version      :  Ver 0.1                                              
Date         :                                         
Author       :  Gavin

Description: 
    project configuration setting
Modification History:
20140611     gavinyin     001 create file
20140703     gavinyin     002 restructure MARCO
*************************************************/
#ifndef __PROJ_CONFIG_H__
#define __PROJ_CONFIG_H__

/* Platform definitions ------------------------------------------------------*/
//#define SIM
#define XTMP
//#define COSIM
//#define FPGA
//#define ASIC
#define HCLK_FREQ  (50 * 1000000)
//#define VT3514_C0

#define NO_THREAD

#define SINGLE_SUBSYSTEM
#define CE_NUM  2
#define PU_NUM  CE_NUM

//#define HOST_SATA
#define HOST_AHCI
//#define HOST_NVME

/* FW function selection -----------------------------------------------------*/
#define EMPTY_PG_DETECT_FW
//#define TRACE_ALL_MODULE_ENABLE 
//#define ECT_TRACE // trace flash chip erase count in ASIC Env.
//#define DBG_TABLE_REBUILD
//#define DBG_DIRTYCNT
//#define TB_CHK
//#define DBG_PMT
//#define CHECK_DIRTYCNT
//#define DATA_PATH_CHECK
#define HAL_NFC_RD_RETRY
//#define L3_RAND_READ_ACC
#define HOST_READ_FROM_DRAM
#define SMART_DEBUG_INFO
#define SEL_WL_SRC
//#define GLOBAL_INJ_ERROR
//#define GLOBAL_ERROR_HANDLE
//#define ERROR_INJ

/* FW unit test selection ----------------------------------------------------*/
//#define L3_LOCAL_TEST
#ifdef L3_LOCAL_TEST
//#define FLASH_DRIVER_TEST_HIGH_LEVEL
#define FLASH_DRIVER_TEST_LOW_LEVEL
//#define HOST_REQUEST_TEST
#endif
//#define DMAE_TEST
//#define SE_TEST
//#define SPINLOCK_TEST
//#define MCU_BASIC_FUNC_TEST
//#define EM_DATACHECK_TEST
//#define SPI_TEST

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
#ifdef COSIM
#define DBG_Printf(x,...)
#else
#define DBG_Printf dbg_printf
#endif

extern void DBG_Getch();
#define ASSERT( _x_ )\
    do{\
        if ( (_x_) == FALSE )\
        {\
            DBG_Getch(); \
        }\
    }while(0)
#endif

#endif/* __PROJ_CONFIG_H__ */
