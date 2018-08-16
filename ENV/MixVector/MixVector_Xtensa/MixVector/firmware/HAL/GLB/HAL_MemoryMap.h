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

/* attribute for compiler
 * COSIM: In COSIM ENV, DDR is initialized by FW code. We can not put any segment
 *        in DRAM when build a ELF file.
 * SIM_XTENSA: Defined in XTMP VS project setting.
 *             XTMP VS project need FW definition but no "__attribute__" related
 * C_PARSER: defined in TraceLog Parser tool
*/
#if defined(BOOTLOADER) || defined(SIM)|| defined(COSIM) || defined(SIM_XTENSA) || defined(C_PARSER)
#define MCU0_HEAD_ATTR
#define MCU0_DRAM_TEXT
#define MCU0_DRAM_DATA
#define MCU12_DRAM_TEXT
#else
#define MCU0_HEAD_ATTR  __attribute__((section(".mcu0head.text")))
#define MCU0_DRAM_TEXT  __attribute__((section(".mcu0dram.text")))
#define MCU0_DRAM_DATA  __attribute__((section(".mcu0dram.data")))
#define MCU12_DRAM_TEXT __attribute__((section(".mcu12dram.text")))
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

/* memory base address */
#ifdef SIM
extern U32 SIM_SRAM0_BASE;
extern U32 SIM_SRAM1_BASE;
extern U32 SIM_DRAM_BASE;
extern U32 SIM_OTFB_BASE;
extern U32 SIM_APB_BASE;
extern U32 SIM_BSS_BASE;

#define SIM_PER_MCU_BSS_SIZE      (32 * 1024)
#define SRAM0_START_ADDRESS       (SIM_SRAM0_BASE)
#define SRAM1_START_ADDRESS       (SIM_SRAM1_BASE)
#define DRAM_START_ADDRESS        (SIM_DRAM_BASE)
#define DRAM_HIGH_START_ADDRESS   (DRAM_START_ADDRESS)
#define OTFB_START_ADDRESS        (SIM_OTFB_BASE)
#define APB_BASE                  (SIM_APB_BASE)
#else
#define SRAM0_START_ADDRESS       0x1ff80000
#define SRAM1_START_ADDRESS       0x1ffc0000
#define DRAM_START_ADDRESS        0x40000000
#define DRAM_HIGH_START_ADDRESS   0x80000000
#define DRAM_HIGH_ADDR_OFFSET     (DRAM_HIGH_START_ADDRESS - DRAM_START_ADDRESS)
#define OTFB_START_ADDRESS        0xFFF00000
#define APB_BASE                  0xFFE07000
#endif

#define HALM_GET_DRAM_ADDR(x) ((U32)(x) - DRAM_START_ADDRESS)

/* base address of DSRAM0 where put FW data section. each MCU has dedicated DSRM0
Note: DSRAM0 is not available in windows simulation ENV.
*/
#define MCU0_DSRAM0_BASE         (SRAM0_START_ADDRESS + 0x24000)
#define MCU1_DSRAM0_BASE         (SRAM0_START_ADDRESS + 0x20000)
#define MCU2_DSRAM0_BASE         (SRAM0_START_ADDRESS + 0x20000)

/* base address of DSRAM1 which is shared by all MCUs */
#define MCU012_DSRAM1_BASE       (SRAM1_START_ADDRESS + 0x8000)

/* base address of ISRAM
Note: ISRAM is not available in windows simulation ENV.
*/
#define MCU0_ISRAM_BASE          (0x20000000)
#define MCU1_ISRAM_BASE          (0x20000000)
#define MCU2_ISRAM_BASE          (0x20000000)

/* size of DSRAM0 */
#define MCU0_DSRAM0_SIZE            (16*1024)
#define MCU1_DSRAM0_SIZE            (32*1024)
#define MCU2_DSRAM0_SIZE            (32*1024)

/* size of DSRAM1.
   In VT3514 C0 design, there is additional 8K following the 96K, but FW do not use it
*/
#define MCU012_DSRAM1_SIZE          (96*1024)

/* size of ISRAM */
#define MCU0_ISRAM_SIZE             (32*1024)
#ifdef VT3514_C0
//#define MCU1_ISRAM_SIZE             (128*1024)
#define MCU12_ISRAM_SIZE            (128*1024)
#else
#define MCU12_ISRAM_SIZE            (96*1024) //in A0/B0 design, MCU1/2 share ISRAM
#endif

#define BOOTLOADER_SIZE             (16*1024)
#define OTFB_SUSPEND_SIZE           (64*1024)

/* define address for MCU0 accessing MCU12's DSRAM0 and ISRAM:
Note:
1. For A0/B0 design, when system power on, only MCU0 is active, and MCU12's DSRAM0/ISRAM are connected to MCU0's address space.
   MCU0 can access MCU1/2's SRAM by following address if it wants to load instruction/data for MCU1/2.
2. For C0 design, MCU0 always can not access MCU1/2's SRAM, so following definition should not be used
*/
#define MCU0_REMAP_MCU1_DSRAM0_BASE  (MCU0_DSRAM0_BASE + MCU0_DSRAM0_SIZE)
#define MCU0_REMAP_MCU2_DSRAM0_BASE  (MCU0_REMAP_MCU1_DSRAM0_BASE + MCU1_DSRAM0_SIZE)
#define MCU0_REMAP_MCU12_ISRAM_BASE  (MCU0_ISRAM_BASE + MCU0_ISRAM_SIZE)

/* memory size for simulation model allocated from Windows */
#define DSRAM0_ALLOCATE_SIZE (256*1024)
#define DSRAM1_ALLOCATE_SIZE (256*1024)//the first 32K is not accessible in multi-core mode
#define DRAM_ALLOCATE_SIZE   (512*1024*1024)
#ifdef VT3514_C0
#define OTFB_ALLOCATE_SIZE   (448*1024)//the last 256K is for SGE on-the-fly
#else
#define OTFB_ALLOCATE_SIZE   (320*1024)//the last 256K is for SGE on-the-fly
#endif

/* HW register start address
 * NOTE:
 * when run 3-core xtmp, open macro XTMP to relocate dsram0 space:0x1ff80000 RegBase  to dsram1 space:0x1ffe0000.
 * as for xtmp env limitation.
 */
#if defined(XTMP)
#define REG_BASE         (0x1ffe0000)
#else
#define REG_BASE         (SRAM0_START_ADDRESS)
#endif

/* detail of HW register layout */
#define REG_BASE_GLB    (REG_BASE)
#define REG_BASE_SPI    (REG_BASE_GLB + 0xE0)
#define REG_BASE_NVME_EXT   (REG_BASE_GLB + 0x600)//0x1ff80600
#define REG_BASE_SDC    (REG_BASE_GLB + 0x800)//0x1ff80800
#define REG_BASE_DRAMC  (REG_BASE_SDC + 0x400)
#define REG_BASE_NDC    (REG_BASE_DRAMC + 0x400)//0x1ff81000
#define REG_BASE_EM     (REG_BASE_NDC + 0x800)
#define REG_BASE_LDPC   (REG_BASE_EM + 0x100) 
#define REG_BASE_BUFM   (REG_BASE_LDPC + 0x100)//0x1ff81a00
#define REG_BASE_SE0    (REG_BASE_BUFM + 0x100)
#define REG_BASE_SE1    (REG_BASE_SE0 + 0x100)
#define REG_BASE_DMAE   (REG_BASE_SE1 + 0x100)
#define REG_BASE_TEST   (REG_BASE_DMAE + 0x100)
#define REG_BASE_PMU    (REG_BASE_TEST + 0x100)//0x1ff81f00
#define REG_BASE_NVME   (REG_BASE_PMU + 0x100) //0x1ff82000
#define REG_BASE_PCIE_NVME  (REG_BASE_NVME + 0x1200) //0x1ff83200
#define REG_BASE_AHCI   (REG_BASE_PCIE_NVME + 0x100) //0x1ff83300
#define REG_BASE_HCT    (REG_BASE_AHCI + 0x180) //0x1ff83480
#define REG_BASE_PCIE_AHCI    (REG_BASE_HCT + 0x80) //0x1ff83500
#define REG_BASE_PCIE   (REG_BASE_PCIE_AHCI + 0x100) //0x1ff83600
#define REG_BASE_HOSTC  (REG_BASE_PCIE + 0x400) //0x1ff83a00
#define REG_BASE_SGE    (REG_BASE_HOSTC + 0x100)//0x1ff83b00
#define REG_BASE_EPHY   (REG_BASE_SGE + 0x100) //0x1ff83c00
#define REG_BASE_SE2    (REG_BASE_EPHY + 0x100)//0x1ff83d00
#define REG_BASE_XOR    (REG_BASE_SE2 + 0x100) //0x1ff83e00
#define REG_BASE_EFUSE  (REG_BASE_XOR + 0x80)  // 0x1ff83e80
#define REG_BASE_NDC_TRIG     (REG_BASE_EFUSE + 0x80) //0x1ff83f00

/* FW/HW interface( DSG, HSG, DRQ, etc. ) SRAM start address */
#define IF_SRAM_BASE     (REG_BASE+0x4000)

/* detail of FW/HW interface( DSG, HSG, DRQ, etc. ) SRAM */
#ifdef VT3514_C0
//in AHCI/NVMe mode, HSG memory share sata DSG
#define SATA_DSG_BASE             (IF_SRAM_BASE)//0x1FF84000
#define SGEQ_ENTRY_BASE           (SATA_DSG_BASE+0x3000)//0x1FF87000
#define DRQ_BASE_ADDR             (SGEQ_ENTRY_BASE)//0x1FF87000
#define DWQ_BASE_ADDR             (SGEQ_ENTRY_BASE + 0x100)//0x1FF87100
#define SGQ_BASE_ADDR             (SGEQ_ENTRY_BASE + 0x400)//0x1FF87400
#define LDPC_N1_BASE              (SGEQ_ENTRY_BASE+0x800)//0x1FF87800
#define NORMAL_DSG_BASE           (LDPC_N1_BASE+0x800)//0x1FF88000
#define CQ_ENTRY_BASE             (NORMAL_DSG_BASE+0x2000)//0x1FF8A000
#define PRCQ_ENTRY_BASE           (CQ_ENTRY_BASE+0x2000)//0x1FF8C000   
#define EM_LBA_BASE               (PRCQ_ENTRY_BASE+0x4000)//0x1FF90000
#define LDPC_VALUE_BASE           (EM_LBA_BASE+0x4000)//0x1FF94000
#else
//in AHCI/NVMe mode, HSG memory share sata DSG
#define SATA_DSG_BASE             (IF_SRAM_BASE)//0x1FF84000
#define CQ_ENTRY_BASE             (SATA_DSG_BASE+0x3000)//0x1FF87000
#define NORMAL_DSG_BASE           (CQ_ENTRY_BASE+0x1000)//0x1FF88000
#define PRCQ_ENTRY_BASE           (NORMAL_DSG_BASE+0x2000)//0x1FF8A000   
#define SGEQ_ENTRY_BASE           (PRCQ_ENTRY_BASE+0x2000)//0x1FF8C000
#define DRQ_BASE_ADDR             (SGEQ_ENTRY_BASE)//0x1FF8C000
#define DWQ_BASE_ADDR             (DRQ_BASE_ADDR + 0x100)//0x1FF8C100
#define SGQ_BASE_ADDR             (DWQ_BASE_ADDR + 0x200)//0x1FF8C300
#define LDPC_N1_BASE              (SGEQ_ENTRY_BASE+0x400)//0x1FF8C400
#define DECOMP_BASE               (LDPC_N1_BASE+0x400)//0x1FF8C800
#define LDPC_VALUE_BASE           (DECOMP_BASE+0x800)//0x1FF8D000
#endif //VT3514_C0

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

/* detail of DSRAM1 allocation */
#define DSRAM1_TL_SEC_BASE        (SRAM1_START_ADDRESS+0x8000)//0x1FFC8000
#define DSRAM1_MCU1_BASE          (DSRAM1_TL_SEC_BASE+0x200)//0x1FFC8200
#define DSRAM1_MCU01_SHARE_BASE   (DSRAM1_MCU1_BASE+0x6E00)//0x1FFCF000
#define DSRAM1_RSVD_BASE          (DSRAM1_MCU01_SHARE_BASE+0x1000)//0x1FFD0000
#define DSRAM1_MCU2_BASE          (DSRAM1_RSVD_BASE+0x200)//0x1FFD0200
#define DSRAM1_MCU02_SHARE_BASE   (DSRAM1_MCU2_BASE+0x6E00)//0x1FFD7000
#define DSRAM1_MCU012_SHARE_BASE  (DSRAM1_MCU02_SHARE_BASE+0x1000)//0x1FFD8000

/* HCT SRAM start address */
#define HCT_SRAM_BASE             (DSRAM1_MCU012_SHARE_BASE)//0x1FFD8000

/* detail of DRAM allocation
*/
#define DRAM_RSVD_BASE                 (DRAM_START_ADDRESS)
#define DRAM_BOOTLOADER_LLF_BASE       (DRAM_RSVD_BASE + 256*1024)
#define DRAM_PARAM_TABLE_BASE          (DRAM_BOOTLOADER_LLF_BASE + BOOTLOADER_SIZE)
#define DRAM_HEAD_MCU0_BASE            (DRAM_PARAM_TABLE_BASE + 16*1024)
#define DRAM_DATA_TEXT_MCU0_BASE       (DRAM_HEAD_MCU0_BASE + 16*1024)
#define DRAM_RODATA_MCU12_SHARE_BASE   (DRAM_DATA_TEXT_MCU0_BASE + 128*1024)
#define DRAM_TEXT_MCU12_SHARE_BASE     (DRAM_RODATA_MCU12_SHARE_BASE + 32*1024)
#define DRAM_DSRAM0_MCU0_BASE          (DRAM_TEXT_MCU12_SHARE_BASE + 256*1024)
#define DRAM_DSRAM0_MCU1_BASE          (DRAM_DSRAM0_MCU0_BASE + 16*1024)
#define DRAM_DSRAM0_MCU2_BASE          (DRAM_DSRAM0_MCU1_BASE + 32*1024)
#define DRAM_ISRAM_MCU0_BASE           (DRAM_DSRAM0_MCU2_BASE + 32*1024)
#define DRAM_ISRAM_MCU12_SHARE_BASE    (DRAM_ISRAM_MCU0_BASE + 32*1024)
#define DRAM_OPTION_ROM                (DRAM_ISRAM_MCU12_SHARE_BASE + 128*1024)
#define DRAM_DSRAM1_SUSPEND_BASE       (DRAM_OPTION_ROM + 64*1024)
#define DRAM_OTFB_SUSPEND_BASE         (DRAM_DSRAM1_SUSPEND_BASE + 96*1024)
#define DRAM_BL_TEMP_BUFF_BASE         (DRAM_OTFB_SUSPEND_BASE + 64*1024)
#define DRAM_128K_ALIGN_RSVD_BASE      (DRAM_BL_TEMP_BUFF_BASE + 32*1024)
#define DRAM_FW_HAL_TRACE_BASE         (DRAM_128K_ALIGN_RSVD_BASE + 64*1024)
#define DRAM_DATA_BUFF_MCU0_BASE       (DRAM_FW_HAL_TRACE_BASE + 768*1024)
#define DRAM_DATA_BUFF_MCU1_BASE       (DRAM_DATA_BUFF_MCU0_BASE + 2048*1024)
#define DRAM_DATA_BUFF_MCU2_BASE       (DRAM_DATA_BUFF_MCU1_BASE + 254*1024*1024)

/* detail of OTFB( MCU accessible part )
NOTE:
    in AHCI single core mode, FW can access:
    0xFFF00000 - 0xFFF10000 (size 64KB)
*/
#define OTFB_BOOTLOADER_BASE            (OTFB_START_ADDRESS)
#define OTFB_SSU0_MCU1_BASE             (OTFB_BOOTLOADER_BASE+BOOTLOADER_SIZE)
#define OTFB_SSU0_MCU2_BASE             (OTFB_SSU0_MCU1_BASE+512)
#define OTFB_CACHE_STATUS_MCU1_BASE     (OTFB_SSU0_MCU2_BASE+512)
#define OTFB_CACHE_STATUS_MCU2_BASE     (OTFB_CACHE_STATUS_MCU1_BASE+0x1800)
#define OTFB_RED_DATA_MCU1_BASE         (OTFB_CACHE_STATUS_MCU2_BASE+0x1800)
#define OTFB_RED_DATA_MCU2_BASE         (OTFB_RED_DATA_MCU1_BASE+0x4000)

/* It is not recommended that FW put any data structure in following range */
#define OTFB_FW_DATA_MCU0_BASE          (OTFB_RED_DATA_MCU2_BASE+0x4000)
#define OTFB_FW_DATA_MCU1_BASE          (OTFB_FW_DATA_MCU0_BASE+0x400)
#define OTFB_FW_DATA_MCU2_BASE          (OTFB_FW_DATA_MCU1_BASE+0x400)

/* SSU1 base address for MCU1/2
Note:
    the SSU1 range overlaps with OTFB_FW_DATA_* range, if FW want to use 
    SSU1, it should not put any data structure in OTFB_FW_DATA_* range
*/
#ifdef MIX_VECTOR
#define OTFB_SSU1_MCU1_BASE             (OTFB_RED_DATA_MCU2_BASE+0x4000)// 1KB
#define OTFB_SSU1_MCU2_BASE             (OTFB_SSU1_MCU1_BASE+0x400) // 1KB
#else
#define OTFB_SSU1_MCU1_BASE             (OTFB_CACHE_STATUS_MCU1_BASE) // be used for cache status
#endif

/* in VT3514 C0, FW have more 64K OTFB. We may use this 64K in future. */
#ifdef VT3514_C0
#define OTFB_FW_RSVD_BASE           (OTFB_START_ADDRESS+0x10000)
/* OTFB memory for SGE, FW can not access SGE part (size 256KB) */
#define OTFB_SGE_FLY_BASE           (OTFB_START_ADDRESS+0x20000)
#else
/* OTFB memory for SGE, FW can not access SGE part (size 256KB) */
#define OTFB_SGE_FLY_BASE           (OTFB_START_ADDRESS+0x10000)
#endif

/* uart: UART is in APB.
Note: HW uart is available on Xtensa platform only */
#define APB_SIZE        0x9000
#define REG_BASE_UART   (APB_BASE)

/* definition of MCU012 shared data structures base address for SATA mode */
#define WRITE_BUF_INFO_MCU01_BASE     (DSRAM1_MCU012_SHARE_BASE)
#define WRITE_BUF_INFO_MCU02_BASE     (WRITE_BUF_INFO_MCU01_BASE + 0x200)
#define SATA_DSGID_INFO_MCU12_BASE    (WRITE_BUF_INFO_MCU02_BASE + 0x200)

//operator for global register
#define rGLB(_Offset_)  (*(volatile U32*)(REG_BASE_GLB + (_Offset_)))

#endif//__HAL_MEMORY_MAP_H__

