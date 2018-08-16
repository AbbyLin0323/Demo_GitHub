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
Filename    : HAL_MemoryMap.h
Version     : Ver 1.0
Author      : GavinYin
Date        : 2014.09.09
Description : this file define system memory map for VT3514 multi-core mode
Others      :
Modify      :
20140909    GavinYin     rename from HAL_Define.h
20141201    GavinYin     1. Add support for VT3514 C0 address mapping(V0.8)
                         2. delete .para_table segment definition
                         3. delete HALM_GET_SRAM0_ADDR(x) and HALM_GET_SRAM1_ADDR(x)
                         4. remove COM_EVENT parameter memory
*******************************************************************************/
#ifndef __HAL_MEMORY_MAP_H__
#define __HAL_MEMORY_MAP_H__
#include "Proj_Config.h"
#include "HAL_FlashChipDefine.h"

/* attribute for compiler
 * COSIM: In COSIM ENV, DDR is initialized by FW code. We can not put any segment
 *        in DRAM when build a ELF file.
 * SIM_XTENSA: Defined in XTMP VS project setting.
 *             XTMP VS project need FW definition but no "__attribute__" related
 * C_PARSER: defined in TraceLog Parser tool
*/


#if defined(BOOTLOADER)
    #ifndef SIM
    #define DRAM_BL_ATTR __attribute__((section(".BootLoaderPart1.text")))
    #else
    #define DRAM_BL_ATTR
    #endif
#else
    #ifndef SIM
    #define DRAM_BL_ATTR __attribute__((section(".mcu2dram.text")))
    #else
    #define DRAM_BL_ATTR
    #endif
#endif

#if defined(SIM)|| defined(COSIM) || defined(SIM_XTENSA) || defined(C_PARSER) || defined(DRAMLESS_ENABLE)
#define MCU0_HEAD_ATTR
#define MCU0_DRAM_TEXT
#define MCU0_DRAM_DATA
#define MCU1_DRAM_TEXT
#define MCU1_DRAM_DATA
#define MCU2_DRAM_TEXT
#define MCU12_DRAM_TEXT
#define MCU12_SHARE_DATA
#define RESTRICTION_RODATA_IN_DSRAM
#else
#define MCU12_SHARE_DATA __attribute__((section(".mcu12.bss")))
#define MCU0_HEAD_ATTR  __attribute__((section(".mcu0head.text")))
#define MCU0_DRAM_TEXT  __attribute__((section(".mcu0dram.text")))
#define MCU0_DRAM_DATA  __attribute__((section(".mcu0dram.data")))
#define MCU1_DRAM_TEXT  __attribute__((section(".mcu1dram.text")))
#define MCU1_DRAM_DATA  __attribute__((section(".mcu1dram.data")))
#define MCU2_DRAM_TEXT  __attribute__((section(".mcu2dram.text")))
#if defined(MCU1)
#define MCU12_DRAM_TEXT __attribute__((section(".mcu1dram.text")))
#elif defined(MCU2)
#define MCU12_DRAM_TEXT __attribute__((section(".mcu2dram.text")))
#elif defined(BOOTLOADER)
#define MCU12_DRAM_TEXT __attribute__((section(".mcu2dram.text")))
#else
#define MCU12_DRAM_TEXT
#endif
#define RESTRICTION_RODATA_IN_DSRAM __attribute__((rodata_section(".data")))
#endif

#if defined(C_PARSER)
#define MCU12_VAR_ATTR
#define NOINL_ATTR
#define INLINE
#elif defined(SIM) || defined(SIM_XTENSA)
#define MCU12_VAR_ATTR __declspec(thread)
#define NOINL_ATTR
#define INLINE
#else
#define MCU12_VAR_ATTR
#define NOINL_ATTR      __attribute__((noinline))
#define INLINE inline
#endif

/* Total DRAM size is specified in P-Table. This variable is initialized in bootup flow*/
extern MCU12_VAR_ATTR U32 g_ulDramTotalSize;

#ifndef MCU0
extern GLOBAL U32* g_pMCU1DramEndBase;
#endif

/**************************************/
/* Memory Base Address and Total Size */
/**************************************/
#ifdef SIM
extern U32 SIM_SRAM0_BASE;
extern U32 SIM_SRAM1_BASE;
extern U32 SIM_DRAM_BASE;
extern U32 SIM_OTFB_BASE;
extern U32 SIM_APB_BASE;
extern U32 SIM_BSS_BASE;
#define SIM_PER_MCU_BSS_SIZE    (32 * 1024)
#define SRAM0_START_ADDRESS     (SIM_SRAM0_BASE)
#define SRAM1_START_ADDRESS     (SIM_SRAM1_BASE)
#define DRAM_START_ADDRESS      (SIM_DRAM_BASE)
#define DRAM_HIGH_START_ADDRESS (DRAM_START_ADDRESS)
#define OTFB_START_ADDRESS      (SIM_OTFB_BASE)
#define APB_BASE                (SIM_APB_BASE)
#else
#define SRAM0_START_ADDRESS     0x1FF80000
#define SRAM1_START_ADDRESS     0x1FFC0000
#define DRAM_START_ADDRESS      0x40000000 // Low 1G Space
#define DRAM_HIGH_START_ADDRESS 0x80000000 // High 1G Space or Remap
#define OTFB_START_ADDRESS      0xFFF00000
#define APB_BASE                0xFFE06000
#endif
#define DRAM_HIGH_ADDR_OFFSET   (DRAM_HIGH_START_ADDRESS - DRAM_START_ADDRESS)

#define DSRAM0_ALLOCATE_SIZE    0x40000 //256K
#define DSRAM1_ALLOCATE_SIZE    0x40000 //256K, the first 32K is not accessible.
#define OTFB_ALLOCATE_SIZE      0x90300 //590592B = 192K + 192K + (64K + 32K +96K) + 768B

/**************************************/
/* DSRAM0 Base Address and Total Size */
/**************************************/
/* Base Address of DSRAM0 which is dedicated by each MCU, where put FW data section.
Note: DSRAM0 is not available in windows simulation ENV.
*/
#define MCU0_DSRAM0_BASE       (SRAM0_START_ADDRESS + 0x24000)//0x1FFA4000
#define MCU1_DSRAM0_BASE       (SRAM0_START_ADDRESS + 0x20000)//0x1FFA0000
#define MCU2_DSRAM0_BASE       (SRAM0_START_ADDRESS + 0x20000)//0x1FFA0000
#define MCU0_DSRAM0_SIZE       0x4000 //16K
#define MCU1_DSRAM0_SIZE       0x8000 //32K, Can be remapped to 64K (+32K from ISRAM-3)
#define MCU2_DSRAM0_SIZE       0x8000 //32K

/**************************************/
/* DSRAM1 Base Address and Total Size */
/**************************************/
/* Base address of DSRAM1 which is shared by all MCUs
Note: Total 96K+8K. The additional 8K following the 96K, but FW do not use it.
*/
#define MCU012_DSRAM1_BASE     (SRAM1_START_ADDRESS + 0x8000)//0x1FFC8000
#define MCU012_DSRAM1_SIZE     0x18000 //96K

/*************************************/
/* ISRAM Base Address and Total Size */
/*************************************/
/* Base Address of ISRAM which is dedicated by each MCU.
Note: ISRAM is not available in windows simulation ENV.
*/
#define MCU0_ISRAM_BASE        0x20000000
#define MCU1_ISRAM_BASE        0x20000000
#define MCU2_ISRAM_BASE        0x20000000
#define MCU0_ISRAM_SIZE        0x0A000 //40K
#define MCU1_ISRAM_SIZE        0x20000 //128K, Can remap -32K to DSRAM0-1
#define MCU2_ISRAM_SIZE        0x10000 //64K

/**********************************************/
/* DSRAM0 Detail Allocation: HW ICB Registers */
/**********************************************/
/* HW register start address
 * NOTE:
 * when run 3-core xtmp, open macro XTMP to relocate dsram0 space:0x1ff80000
 * RegBase  to dsram1 space:0x1ffe0000. as for xtmp env limitation.
 */
#if defined(XTMP)
#define REG_BASE                (0x1ffe0000)
#else
#define REG_BASE                (SRAM0_START_ADDRESS)
#endif
/* ICB register layout */
#define REG_BASE_GLB        (REG_BASE)                  // 0x1FF80000
#define REG_BASE_SPI        (REG_BASE_GLB  + 0xE0)      // 0x1FF800E0
#define REG_BASE_XOR        (REG_BASE_GLB  + 0x400)     // 0x1FF80400
#define REG_BASE_SDC        (REG_BASE_XOR  + 0x400)     // 0x1FF80800
#define REG_BASE_DRAMC      (REG_BASE_SDC  + 0x400)     // 0x1FF80C00
#define REG_BASE_NDC        (REG_BASE_DRAMC+ 0x400)     // 0x1FF81000
#define REG_BASE_EM         (REG_BASE_NDC  + 0x800)     // 0x1FF81800
#define REG_BASE_LDPC       (REG_BASE_EM   + 0x100)     // 0x1FF81900
#define REG_BASE_BUFM       (REG_BASE_LDPC + 0x100)     // 0x1FF81A00
#define REG_BASE_SE0        (REG_BASE_BUFM + 0x100)     // 0x1FF81B00
#define REG_BASE_SE1        (REG_BASE_SE0  + 0x100)     // 0x1FF81C00
#define REG_BASE_DMAE       (REG_BASE_SE1  + 0x100)     // 0x1FF81D00
#define REG_BASE_TEST       (REG_BASE_DMAE + 0x100)     // 0x1FF81E00
#define REG_BASE_PMU        (REG_BASE_TEST + 0x100)     // 0x1FF81F00
#define REG_BASE_NVME       (REG_BASE_PMU  + 0x100)     // 0x1FF82000
#define REG_BASE_PCIE_NVME  (REG_BASE_NVME + 0x1200)    // 0x1FF83200
#define REG_BASE_AHCI       (REG_BASE_PCIE_NVME + 0x100)// 0x1FF83300
#define REG_BASE_HCT        (REG_BASE_AHCI + 0x180)     // 0x1FF83480
#define REG_BASE_PCIE_AHCI  (REG_BASE_HCT + 0x80)       // 0x1FF83500
#define REG_BASE_PCIE       (REG_BASE_PCIE_AHCI + 0x100)// 0x1FF83600
#define REG_BASE_HOSTC      (REG_BASE_PCIE + 0x400)     // 0x1FF83A00
#define REG_BASE_SGE        (REG_BASE_HOSTC + 0x100)    // 0x1FF83B00
#define REG_BASE_EPHY       (REG_BASE_SGE   + 0x100)    // 0x1FF83C00
#define REG_BASE_SE2        (REG_BASE_EPHY  + 0x100)    // 0x1FF83D00
#define REG_BASE_XOR_ACC    (REG_BASE_SE2   + 0x100)    // 0x1FF83E00
//#define REG_BASE_EFUSE      (REG_BASE_XOR_ACC+0x100)  //
#define REG_BASE_TRIG_NDC   (REG_BASE_XOR_ACC+0x100)    // 0x1FF83F00
/* ICB1 register layout */
#define REG_BASE_EXT            (REG_BASE + 0x1C000)       //0x1FF9C000
#define REG_BASE_EXT_NVME       (REG_BASE_EXT)             //0x1FF9C000
#define REG_BASE_EXT_NVME_AF    (REG_BASE_EXT_NVME)        //0x1FF9C000
#define REG_BASE_EXT_NVME_CQDW  (REG_BASE_EXT_NVME + 0x200)//0x1FF9C200
#define REG_BASE_EXT_NVME_CRL   (REG_BASE_EXT_NVME + 0x300)//0x1FF9C300
#define REG_BASE_EXT_RV0        (REG_BASE_EXT_NVME + 0x400)//0x1FF9C400
#define REG_BASE_EXT_HGE        (REG_BASE_EXT_RV0  + 0x100)//0x1FF9C500
#define REG_BASE_EXT_SNFC       (REG_BASE_EXT_HGE  + 0x100)//0x1FF9C600
#define REG_BASE_EXT_RV4        (REG_BASE_EXT_SNFC + 0x100)//0x1FF9C700
#define REG_BASE_EXT_RV5        (REG_BASE_EXT_RV4  + 0x100)//0x1FF9C800
#define REG_BASE_EXT_RV6        (REG_BASE_EXT_RV5  + 0x100)//0x1FF9C900
#define REG_BASE_EXT_RV7        (REG_BASE_EXT_RV6  + 0x100)//0x1FF9CA00

//#define DAA_BASE                (SRAM0_START_ADDRESS+0x1A000)//0x1ff9A000

/**********************************************/
/* DSRAM0 Detail Allocation: FW&HW Interfaces */
/**********************************************/
#define IF_SRAM_BASE            (REG_BASE + 0x4000)       //0x1FF84000
#define SATA_DSG_BASE           (IF_SRAM_BASE)
#define SGEQ_ENTRY_BASE         (SATA_DSG_BASE   + 0x3000)//0x1FF87000
#define DRQ_BASE_ADDR           (SGEQ_ENTRY_BASE)
#define DWQ_BASE_ADDR           (DRQ_BASE_ADDR   + 0x100) //0x1FF87100
#define SGQ_BASE_ADDR           (DWQ_BASE_ADDR   + 0x300) //0x1FF87400
#define USRAM45_RSV_BASE        (SGQ_BASE_ADDR   + 0x400) //0x1FF87800
#define NORMAL_DSG_BASE         (USRAM45_RSV_BASE+ 0x800) //0x1FF88000
#define DEC_STATUS_BASE         (NORMAL_DSG_BASE + 0x2000)//0x1FF8A000
#define PRCQ_ENTRY_BASE         (DEC_STATUS_BASE + 0x4000)//0x1FF8E000
#define DS_SRAM_BASE            (PRCQ_ENTRY_BASE + 0x800) //0x1FF8E800
#define DEC_FIFO_BASE           (DS_SRAM_BASE    + 0x800) //0x1FF8F000
#define USRAM3_RSV_BASE         (DEC_FIFO_BASE   + 0x200) //0x1FF8F200
#define NFCQ_ENTRY_BASE         (USRAM3_RSV_BASE + 0xE00) //0x1FF90000
#define LDPC_ECM_BASE           (NFCQ_ENTRY_BASE + 0x4000)//0x1FF94000
#define HGE_ENTRY_BASE          (LDPC_ECM_BASE   + 0x400) //0x1FF94400
#define USRAM6_RSV_BASE         (HGE_ENTRY_BASE  + 0x200) //0x1FF94600

/****************************/
/* DSRAM1 Detail Allocation */
/****************************/
#define DSRAM1_MCU1_SIZE            (0x8A00) // 34.5K
#define DSRAM1_MCU01_SHARE_SIZE     (0x1000) // 4K
#define DSRAM1_MCU2_SIZE            (0x400)  // 1K
#define DSRAM1_MCU12_SHARE_SIZE     (0x6200) // 24.5K
#define DSRAM1_MCU012_SHARE_SIZE    (0xA000) // 32K+8K
#define DSRAM1_MCU1_BASE            (MCU012_DSRAM1_BASE)                                //0x1FFC8000
#define DSRAM1_MCU01_SHARE_BASE     (DSRAM1_MCU1_BASE + DSRAM1_MCU1_SIZE)               //0x1FFCEE00
#define DSRAM1_MCU2_BASE            (DSRAM1_MCU01_SHARE_BASE + DSRAM1_MCU01_SHARE_SIZE) //0x1FFCFE00
#define DSRAM1_MCU12_SHARE_BASE     (DSRAM1_MCU2_BASE + DSRAM1_MCU2_SIZE)               //0x1FFD1E0
#define DSRAM1_MCU12_ERASESTS_SIZE  (0x200)
#define DSRAM1_MCU12_CMNMISC_SIZE   (0x400 - DSRAM1_MCU12_ERASESTS_SIZE) //DSRAM1_MCU12_ERASESTS_SIZE+DSRAM1_MCU12_CMNMISC_SIZE = 0x400 (1k)
#define DSRAM1_MCU12_ERASESTS_BASE  (DSRAM1_MCU012_SHARE_BASE - 0x400)
#define DSRAM1_MCU12_CMNMISC_BASE   (DSRAM1_MCU012_SHARE_BASE - DSRAM1_MCU12_CMNMISC_SIZE) //0x1FFD7C00
#define DSRAM1_MCU012_SHARE_BASE    (DSRAM1_MCU12_SHARE_BASE + DSRAM1_MCU12_SHARE_SIZE)   //0x1FFD8000
#define HCT_SRAM_BASE               (DSRAM1_MCU012_SHARE_BASE)//0x1FFD8000

#ifdef HOST_NVME
#define HAL_CHAIN_NUM_MANAGER_SIZE      (0x300) // 768B
#define HAL_CHAIN_NUM_MANAGER_BASE      (DSRAM1_MCU12_SHARE_BASE)
#endif

#ifdef HOST_SATA
#define WRITE_BUF_INFO_MCU01_BASE     (DSRAM1_MCU012_SHARE_BASE)
#define WRITE_BUF_INFO_MCU02_BASE     (WRITE_BUF_INFO_MCU01_BASE + 0x200)
#define SATA_DSGID_INFO_MCU12_BASE    (WRITE_BUF_INFO_MCU02_BASE + 0x200)
#define SATA_CACHE_LOCKED_INFO        (SATA_DSGID_INFO_MCU12_BASE + 0x200)
#endif

/**************************/
/* DRAM Detail Allocation */
/**************************/
#define DRAM_RSVD_SIZE               0x40000 // 256K

/*modify below code to adjust TraceLogParser tool*/
#define BOOTLOADER_PART0_SIZE        (15*1024)//0x03C00 // 15K
#define BOOTLOADER_PART1_SIZE        (15*1024)//0x03C00 // 15K
#define HEAD_MCU0_SIZE               (12*1024)//0x03000 // 12K
#define FW_STATIC_INFO_SIZE          (4*1024)//0x01000 // 4K
#define DATA_TEXT_MCU0_SIZE          (60*1024)//0xf000 // 60K
#define DATA_TEXT_MCU1_SIZE          (288*1024)//0x48000
#define DATA_TEXT_MCU2_SIZE          (80*1024)//0x14000
#define RODATA_MCU1_SIZE             (24*1024)//0x06000 // 24K
#define RODATA_MCU2_SIZE             (8*1024)//0x02000 // 8K
#define TEXT_MCU1_SIZE               (160*1024)//0x28000 // 160K
#define TEXT_MCU2_SIZE               (96*1024)//0x18000 // 96K
#define OPTION_ROM_SIZE              (64*1024)//0x10000 // 64K
#define BL_TMP_BUFF_SIZE             (32*1024)//0x08000 // 32K
#define DSRAM1_SUSPEND_SIZE          (96*1024)//0x18000 // 96K
#define OTFB_SUSPEND_SIZE            (64*1024)//0x10000 // 64K
#define DRAM_RED_MCU12_SHARE_SIZE    (192*1024)//0x30000 // 192K
#define DRAM_SSU_MCU12_SHARE_SIZE    (1*1024)//0x00400 // 1K
#define ALIGN_128K_RSV_SIZE          (23*1024)//0x05C00 // 23K

#ifdef HOST_SATA
#define DRAM_FW_HAL_TRACE_SIZE          0       // 640K
#else
#define DRAM_FW_HAL_TRACE_SIZE          (0x8000) // 32K
#endif
/* Rsvd 640K for update firmware */
#define FW_UPDATE_SIZE                  (0xB0000) //704K
#define DATA_BUFF_MCU0_SIZE             0x140000 //1280K
#define MCU1_DRAM_END_POINTER_SIZE      4
#define DRAM_BUFF_MCU12_SIZE            (0xB0000 - MCU1_DRAM_END_POINTER_SIZE)  //704K
#define DEC_FIFO_STATUS_SIZE            0x800 //2K

#define DRAM_RSVD_BASE                  (DRAM_START_ADDRESS)                                //0x40000000
#define DRAM_BOOTLOADER_PART0_BASE      (DRAM_RSVD_BASE + DRAM_RSVD_SIZE)                   //0x40040000
#define DRAM_BOOTLOADER_PART1_BASE      (DRAM_BOOTLOADER_PART0_BASE + BOOTLOADER_PART0_SIZE)//0x40043C00
#define DRAM_FW_STATIC_INFO_BASE        (DRAM_BOOTLOADER_PART1_BASE + BOOTLOADER_PART1_SIZE + 0x800) //0x4004 8000
#define DRAM_DATA_TEXT_MCU0_BASE        (DRAM_FW_STATIC_INFO_BASE + FW_STATIC_INFO_SIZE)    //0x4004 9000
#define DRAM_DATA_TEXT_MCU1_BASE        (DRAM_DATA_TEXT_MCU0_BASE + DATA_TEXT_MCU0_SIZE) //0x4005 8000
#define DRAM_DATA_TEXT_MCU2_BASE        (DRAM_DATA_TEXT_MCU1_BASE + DATA_TEXT_MCU1_SIZE) //0x400a 0000
#define DRAM_DSRAM0_MCU0_BASE           (DRAM_DATA_TEXT_MCU2_BASE + DATA_TEXT_MCU2_SIZE)    //0x400b 4000
#define DRAM_DSRAM0_MCU1_BASE           (DRAM_DSRAM0_MCU0_BASE + MCU0_DSRAM0_SIZE)          //0x400B8000
#define DRAM_DSRAM0_MCU2_BASE           (DRAM_DSRAM0_MCU1_BASE + MCU1_DSRAM0_SIZE)          //0x400C0000
#define DRAM_ISRAM_MCU0_BASE            (DRAM_DSRAM0_MCU2_BASE + MCU2_DSRAM0_SIZE)          //0x400C8000
#define DRAM_ISRAM_MCU1_BASE            (DRAM_ISRAM_MCU0_BASE + MCU0_ISRAM_SIZE)            //0x400D2000
#define DRAM_ISRAM_MCU2_BASE            (DRAM_ISRAM_MCU1_BASE + MCU1_ISRAM_SIZE)            //0x400F2000
#define DRAM_OPTION_ROM                 (DRAM_ISRAM_MCU2_BASE + MCU2_ISRAM_SIZE)            //0x40102000
#define DRAM_BL_TEMP_BUFF_BASE          (DRAM_OPTION_ROM + OPTION_ROM_SIZE)                 //0x40112000
#define DRAM_DSRAM1_SUSPEND_BASE        (DRAM_BL_TEMP_BUFF_BASE + BL_TMP_BUFF_SIZE)         //0x4011A000
#define DRAM_OTFB_SUSPEND_BASE          (DRAM_DSRAM1_SUSPEND_BASE + DSRAM1_SUSPEND_SIZE)    //0x40132000
#define DRAM_RED_MCU12_SHARE_BASE       (DRAM_OTFB_SUSPEND_BASE + OTFB_SUSPEND_SIZE)        //0x40142000
#define DRAM_SSU0_MCU12_SHARE_BASE      (DRAM_RED_MCU12_SHARE_BASE + DRAM_RED_MCU12_SHARE_SIZE)
#define DRAM_128K_ALIGN_RSVD_BASE       (DRAM_SSU0_MCU12_SHARE_BASE + DRAM_SSU_MCU12_SHARE_SIZE)
#define DRAM_FW_UPDATE_BASE             (DRAM_128K_ALIGN_RSVD_BASE + ALIGN_128K_RSV_SIZE)   //0x40178000
#define DRAM_FW_HAL_TRACE_BASE          (DRAM_FW_UPDATE_BASE + FW_UPDATE_SIZE)
#define DRAM_DATA_BUFF_MCU0_BASE        (DRAM_FW_HAL_TRACE_BASE + DRAM_FW_HAL_TRACE_SIZE)   //SATA-4PLN: 0x40160000; NVME: 0x40200000
#define DRAM_DATA_BUFF_MCU12_BASE       (DRAM_DATA_BUFF_MCU0_BASE + DATA_BUFF_MCU0_SIZE)    //SATA-4PLN: 0x40360000; NVME: 0x40400000
#define DRAM_MCU1_DRAM_END_POINTER_BASE (DRAM_DATA_BUFF_MCU12_BASE + DRAM_BUFF_MCU12_SIZE)
#define DRAM_DEC_FIFO_STATUS_BASE       (DRAM_MCU1_DRAM_END_POINTER_BASE + MCU1_DRAM_END_POINTER_SIZE)
#define DRAM_DATA_BUFF_MCU1_BASE        (DRAM_DEC_FIFO_STATUS_BASE + DEC_FIFO_STATUS_SIZE)  //SATA-4PLN: 0x40960000; NVME: 0x40A00000

#ifndef BOOTLOADER
#define DRAM_MCU1_TRACE_BUF_SIZE        (0)
#define DATA_BUFF_MCU1_SIZE             (*g_pMCU1DramEndBase - DRAM_DATA_BUFF_MCU1_BASE + DRAM_MCU1_TRACE_BUF_SIZE)
#define DRAM_DATA_BUFF_MCU2_BASE        (DRAM_DATA_BUFF_MCU1_BASE + DATA_BUFF_MCU1_SIZE)    //SATA-4PLN: 0x54560000; NVME: 0x54600000
#define DATA_BUFF_MCU2_SIZE             ((g_ulDramTotalSize > (DRAM_DATA_BUFF_MCU2_BASE - DRAM_START_ADDRESS)) ? (g_ulDramTotalSize - (DRAM_DATA_BUFF_MCU2_BASE - DRAM_START_ADDRESS)) : (0))
#endif
/***************************************************************/
/* OTFB Detail Allocation : 0xFFF00000 - 0xFFF10000 (size 64KB)*/
/***************************************************************/
#define OTFB_MCU12_SHARED_RED_SIZE      (0x8000)// 32K
#define OTFB_MCU12_SHARED_SSU0_SIZE     (0x400) // 1K
#define OTFB_MCU12_SHARED_SSU1_SIZE     (0x400) // 1K
#define OTFB_TL_MCU012_SHARED_SIZE      (0x200) // 0.5K
#define OTFB_MCU0_FW_DATA_SIZE          (0x600) // 1.5K
#define OTFB_MCU1_FW_DATA_SIZE          (0x2000)// 8K
#define OTFB_MCU2_FW_DATA_SIZE          (0x1000)// 4K
#define OTFB_SGE_FLY_SIZE               (0x20000) // 128K
#define OTFB_XOR_PARITY_SIZE            (0x30000) // 192K
#define OTFB_XOR_RED_SIZE               (0x300)   // 768B
#define OTFB_BOOTLOADER_BASE            (OTFB_START_ADDRESS)                                      //0xFFF00000
#define OTFB_RED_MCU12_SHARE_BASE       (OTFB_BOOTLOADER_BASE + BOOTLOADER_PART0_SIZE)            //0xFFF04000
#define OTFB_SSU0_MCU12_SHARE_BASE      (OTFB_RED_MCU12_SHARE_BASE  + OTFB_MCU12_SHARED_RED_SIZE) //0xFFF0C000
#define OTFB_SSU1_MCU12_SHARE_BASE      (OTFB_SSU0_MCU12_SHARE_BASE + OTFB_MCU12_SHARED_SSU0_SIZE)//0xFFF0C400
#define OTFB_TLG_MCU012_SHARE_BASE      (OTFB_SSU1_MCU12_SHARE_BASE + OTFB_MCU12_SHARED_SSU1_SIZE)//0xFFF0C800
#define OTFB_FW_DATA_MCU0_BASE          (OTFB_TLG_MCU012_SHARE_BASE + OTFB_TL_MCU012_SHARED_SIZE) //0xFFF0CA00
#define OTFB_FW_DATA_MCU1_BASE          (OTFB_FW_DATA_MCU0_BASE + OTFB_MCU0_FW_DATA_SIZE)         //0xFFF0D000
#define OTFB_FW_DATA_MCU2_BASE          (OTFB_FW_DATA_MCU1_BASE + OTFB_MCU1_FW_DATA_SIZE)         //0xFFF0F000
#define OTFB_SGE_FLY_BASE               (OTFB_FW_DATA_MCU2_BASE + OTFB_MCU2_FW_DATA_SIZE)         //0xFFF10000
#define OTFB_XOR_PARITY_BASE            (OTFB_START_ADDRESS + 0x30000)                            //0xFFF30000
#define OTFB_XOR_REDUNDANT_BASE         (OTFB_START_ADDRESS + 0x90000)                            //0xFFF90000

/******************************************************************************/
/* APB Detail Allocation: SUSRAM+UART+GPIO+RSV0+IIC+TSC+RSV1, The SUSRAM is suspend for FW save info, no use now.        */
/******************************************************************************/
#define APB_SIZE            0xA000  // 40K
#define APB_SUSRAM_SIZE     0x1000  // 4K
#define APB_UART_SIZE       0x1000  // 4K
#define APB_GPIO_SIZE       0x1000  // 4K
#define APB_RSV0_SIZE       0x4000  // 16K
#define APB_IIC_SIZE        0x800   // 2K
#define APB_TSC_SIZE        0x800   // 2k
#define ABB_RSV1_SIZE       0x2000  // 8K
#define SUSRAM_REG_BASE     (APB_BASE)                          // 0xFFE06000
#define UART_REG_BASE       (SUSRAM_REG_BASE + APB_SUSRAM_SIZE) // 0xFFE07000
#define GPIO_REG_BASE       (UART_REG_BASE + APB_UART_SIZE)     // 0xFFE08000
#define APB_RSV0_BASE       (GPIO_REG_BASE + APB_GPIO_SIZE)     // 0xFFE09000
#define IIC_REG_BASE        (APB_RSV0_BASE + APB_RSV0_SIZE)     // 0xFFE0D000
#define TSC_REG_BASE        (IIC_REG_BASE + APB_IIC_SIZE)       // 0xFFE0D800

//operator for global/PMU register
#define rGLB(_Offset_)          (*(volatile U32*)(REG_BASE_GLB + (_Offset_)))
#define rGLBByte(_Offset_)      (*(volatile U8*)(REG_BASE_GLB + (_Offset_)))
#define rSDC(_x_)               (*((volatile U32*)(REG_BASE_SDC + (_x_))))
#define rSDCByte(_x_)           (*((volatile U8*)(REG_BASE_SDC + (_x_))))
#define rDRAMC(_x_)             (*((volatile U32*)(REG_BASE_DRAMC + (_x_))))
#define rDRAMCByte(_x_)         (*((volatile U8*)(REG_BASE_DRAMC + (_x_))))
#define rNFC(_x_)               (*((volatile U32*)(REG_BASE_NDC + (_x_))))
#define rNFCByte(_x_)           (*((volatile U8*)(REG_BASE_NDC + (_x_))))
#define rPMU(_x_)               (*(volatile U32*)(REG_BASE_PMU + (_x_)))
#define rPMUByte(_x_)           (*(volatile U8*)(REG_BASE_PMU + (_x_)))
#define rPCIeNCfg(_Offset_)     (*(volatile U32*)(REG_BASE_PCIE_NVME + (_Offset_)))
#define rPCIe(_Offset_)         (*(volatile U32*)(REG_BASE_PCIE + (_Offset_)))
#define rPCIeByte(_Offset_)     (*(volatile U8*)(REG_BASE_PCIE + (_Offset_)))
#define rHOSTC(_Offset_)        (*(volatile U32*)(REG_BASE_HOSTC + (_Offset_)))
#define rEXT(_Offset_)          (*(volatile U32*)(REG_BASE_EXT + (_Offset_)))
#define PLL(_x_)                ((25 * ((((_x_) & 0x7F) + 1) << 1)) / \
                                  (((((_x_) >> 7) & 1) + 1) * (1 << (((_x_) >> 8) & 3))))
/*GPIO8 value, if low, uart mp mode*/
#define rGPIO(_x_)              (*((volatile U32*)(GPIO_REG_BASE + _x_)))

/*define trim DSRAM0 address*/
/*0x1ff90000 and 0x1ff940000 are used for EM and LDPC. However, these two functions are not used in VT3514.*/
/*Therefore, I use these two address to process trim operation*/
#define DSRAM0_MCU1_TRIM_BASE   (SRAM0_START_ADDRESS + 0x10000)
#define DSRAM0_MCU2_TRIM_BASE   (SRAM0_START_ADDRESS + 0x12800)


/* Dynamic allocated SRAM0(32KB) base address except for .DATA/.BSS segment, by Javen 2015/03/03 */
#ifdef SIM
#define DSRAM0_MCU1_MAX_SIZE      ( 20*1024 )   // 20KB
#define DSRAM0_MCU2_MAX_SIZE      ( 20*1024 )   // 20KB
#else
#define DSRAM0_MCU1_MAX_SIZE      ( ( MCU1_DSRAM0_BASE + MCU1_DSRAM0_SIZE ) - HAL_GetBssSegEndAddr() )
#define DSRAM0_MCU2_MAX_SIZE      ( ( MCU2_DSRAM0_BASE + MCU2_DSRAM0_SIZE ) - HAL_GetBssSegEndAddr() )
#endif
#define DSRAM0_MCU1_BASE          ( HAL_GetBssSegEndAddr() )
#define DSRAM0_MCU2_BASE          ( HAL_GetBssSegEndAddr() )

#endif//__HAL_MEMORY_MAP_H__

