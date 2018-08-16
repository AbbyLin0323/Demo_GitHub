/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :                                           
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#ifndef _HAL_DEFINE_H
#define _HAL_DEFINE_H
#include "HAL_MemoryMap.h"

#define DRAM_ATTR
#define HEAD_ATTR

#define SSU1_BASE_OTFB              (OTFB_START_ADDRESS)
#define SSU0_BASE_OTFB              (SSU1_BASE_OTFB + 0x1000)
#define CACHE_STATUS_BASE_OTFB      (SSU0_BASE_OTFB+0x400)
#define RED_BASE_OTFB               (CACHE_STATUS_BASE_OTFB+0x1000)

#define OTFB_IDENTIFY_ADDR          (OTFB_START_ADDRESS + 0x4000)
#define OTFB_RAMDISK_CS             (OTFB_IDENTIFY_ADDR + 0x400)
#define STATIC_PARAMETER_COMMON_EVENT_RESULT (OTFB_RAMDISK_CS + 0x400)
#define OTFB_FREE_BASE              (STATIC_PARAMETER_COMMON_EVENT_RESULT + 0x400)

/* 32KB offset, size 128KB */
#define OTFB_RAMDISK_BASE           (OTFB_START_ADDRESS + 0x8000)

#define   SEC_PER_BUF_BITS      6 //(SEC_PER_PG_BITS + PLN_PER_PU_BITS): Buffersize = 8(128K) 7(64K) 6(32K) 5(16K) 4(8K) 3(4K)
#define   SEC_PER_BUF           (1<<SEC_PER_BUF_BITS)
#define   SEC_PER_BUF_MSK       (SEC_PER_BUF -1)

#define   BUF_SIZE_BITS         (SEC_PER_BUF_BITS +  SEC_SZ_BITS)  
#define   BUF_SIZE              (1 << BUF_SIZE_BITS)      
#define   BUF_SIZE_MSK          (BUF_SIZE -1)

#define   BUFID_START_LIMIT_BITS    (7+BUF_SIZE_BITS)
#define   BUFID_START_LIMIT_SIZE     (1<<BUFID_START_LIMIT_BITS)
#define   BUFID_START_LIMIT_MSK      (BUFID_START_LIMIT_SIZE - 1)

#define   SEC_PER_LPN_BITS        3    // Split Size = 4K
#define   SEC_PER_LPN            (1<<SEC_PER_LPN_BITS)
#define   SEC_PER_LPN_MSK        (SEC_PER_LPN -1)
#define   SPLIT_SIZE_BITS         (SEC_SZ_BITS +  SEC_PER_LPN_BITS)   
#define   SPLIT_SIZE              (1 << SPLIT_SIZE_BITS)     

#define   LPN_PER_BUF_BITS       (SEC_PER_BUF_BITS - SEC_PER_LPN_BITS)
#define   LPN_PER_BUF            (1 << LPN_PER_BUF_BITS)
#define   LPN_PER_BUF_MSK        (LPN_PER_BUF -1)

#define   LPN_SIZE_BITS           12
#define   LPN_SIZE               (1 << LPN_SIZE_BITS)

#define   LPN_SECTOR_BIT     SEC_PER_LPN_BITS
#define   LPN_SECTOR_NUM     SEC_PER_LPN
#define   LPN_SECTOR_MSK     SEC_PER_LPN_MSK

/*
DDR map: 
-------------begin-------------0
blank offset
-------------------------------
fw code [head isram dsram otfb ddr]
-------------------------------
bootloader
-------------------------------2M
l3 temp buffer: 32k
-------------------------------
static param: 64k
-------------------------------
save area
-------------------------------
free area
-------------end---------------
*/

#define BOOT_LOADER_DRAM        (DRAM_START_ADDRESS+TOTAL_TABLE_SIZE+DDR_OFFSET)
#define L3TMPBUF_BASE_DRAM      (DRAM_START_ADDRESS+2*1024*1024)
#define STATIC_PARAMETER_BASE_DRAM (L3TMPBUF_BASE_DRAM+32*1024)

#define STATIC_PARAMETER_BOOT_SELECTOR_FLAG  (STATIC_PARAMETER_BASE_DRAM + 0x0)
#define STATIC_PARAMETER_BOOT_FUN_FLAG       (STATIC_PARAMETER_BASE_DRAM + 0x4)
#define STATIC_PARAMETER_DUMP_PBIT_START     (STATIC_PARAMETER_BASE_DRAM + 0x8)
#define STATIC_PARAMETER_DUMP_PBIT_END       (STATIC_PARAMETER_BASE_DRAM + 0xc)

#define STATIC_PARAMETER_SEARCH_ENGINE       (STATIC_PARAMETER_BASE_DRAM + 0x400)
#define STATIC_PARAMETER_ZERO_VALUE          (STATIC_PARAMETER_SEARCH_ENGINE + 0x800)

#define DRAM_SAVE_BASE          (STATIC_PARAMETER_BASE_DRAM + 64*1024)
#define DRAM_SAVE_SIZE          (64 * 1024 * 1024)
#define DRAM_FREE_BASE          (DRAM_SAVE_BASE + DRAM_SAVE_SIZE)
#define DRAM_FREE_SIZE          (DRAM_START_ADDRESS + DRAM_ALLCOATE_SIZE - DRAM_FREE_BASE)

#define   HCMD_WRITE        0x0
#define   HCMD_READ         0x1
#define   HCMD_SHUTDOWN     0x2
#define   HCMD_TRIM         0x3
#define   HCMD_SMART        0x4
#define   HCMD_VD           0x5




void HAL_Init(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
#endif

/********************** FILE END ***************/

