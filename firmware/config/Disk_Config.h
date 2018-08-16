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
Filename     :  Disk_Config.h
Version      :  Ver 0.1
Date         :
Author       :  Blake Zhang

Description:
    project configuration setting
Modification History:
20140902     blakezhang     001 create file
*******************************************************************************/
#ifndef __DISK_CONFIG_H__
#define __DISK_CONFIG_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_ParamTable.h"
#include "HAL_HostInterface.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"

#define SUB_SYSTEM_NUM_MAX          (2)

#define LUN_NUM_PER_PU              (NFC_LUN_PER_PU)
#define BLK_PER_LUN                 (BLK_PER_PLN)
#define RSVD_BLK_PER_LUN            (RSV_BLK_PER_PLN)

#if !defined(LUN_NUM_PER_SUPERPU)
#ifdef SIM
#include "model_config.h"
#define LUN_NUM_PER_SUPERPU         (BL_SUBSYSTEM_PU_NUM)
#else
#define LUN_NUM_PER_SUPERPU         (2)
#endif
#endif

#define SUPERPU_LUN_NUM_BITMSK      ((1 << LUN_NUM_PER_SUPERPU) - 1)
#define SUPER_PAGE_SIZE             (LUN_NUM_PER_SUPERPU * LOGIC_PIPE_PG_SZ) //128K

#define SUBSYSTEM_LUN_MAX           (SUBSYSTEM_PU_MAX * LUN_NUM_PER_PU)
#define SUBSYSTEM_SUPERPU_MAX       (SUBSYSTEM_LUN_MAX/LUN_NUM_PER_SUPERPU)
#define SUBSYSTEM_LUN_NUM           (SUBSYSTEM_PU_NUM * LUN_NUM_PER_PU)
#define SUBSYSTEM_SUPERPU_NUM       (SUBSYSTEM_LUN_NUM/LUN_NUM_PER_SUPERPU)

// SPU + LunInSPU <-> TLun
#define L2_GET_TLUN(SPU, LunInSPU)  ((SPU)*LUN_NUM_PER_SUPERPU + (LunInSPU))
#define L2_GET_SPU(TLun)            ((TLun)/LUN_NUM_PER_SUPERPU)
#define L2_GET_LUN_IN_SPU(TLun)     ((TLun)%LUN_NUM_PER_SUPERPU)

#if (LUN_NUM_PER_PU == 1)
#define L3_GET_TLUN(PU, LunInPU)    (PU)
#define L3_GET_PU(TLun)             (TLun)
#define L3_GET_LUN_IN_PU(TLun)      (0)
#else
#define L3_GET_TLUN(PU, LunInPU)    ((PU) + (LunInPU)*SUBSYSTEM_PU_NUM)
#define L3_GET_PU(TLun)             ((TLun)%SUBSYSTEM_PU_NUM)
#define L3_GET_LUN_IN_PU(TLun)      ((TLun)/SUBSYSTEM_PU_NUM)
#endif

// BITMAP Related Define
#define BITMAP_GRP_NUM_PER_SPU      ((0==LUN_NUM_PER_SUPERPU%32) ? (LUN_NUM_PER_SUPERPU/32): (LUN_NUM_PER_SUPERPU/32+1))

/* SubSystem used CacheStatus and FlashStatus */
#define SUBSYSTEM_STATUS_INIT          0x2
#define SUBSYSTEM_STATUS_SUCCESS       0x0
#define SUBSYSTEM_STATUS_PENDING       0x1
#define SUBSYSTEM_STATUS_FAIL          0x3 // Erase-Fail or Program-Fail or Read-Retry-Fail
#define SUBSYSTEM_STATUS_EMPTY_PG      0x4
#define SUBSYSTEM_STATUS_RECC          0x5
#define SUBSYSTEM_STATUS_RETRY_SUCCESS 0x6
#define SUBSYSTEM_STATUS_INIT_DW      0x02020202  /* for DMAE init */

// New Flash Macro Define Adapter
#define PG_PER_WL             (3)

#define BUF_SIZE_BITS         LOGIC_PIPE_PG_SZ_BITS
#define BUF_SIZE              (1 << BUF_SIZE_BITS)
#define BUF_SIZE_MSK          (BUF_SIZE - 1)

#define SEC_PER_BUF_BITS      (BUF_SIZE_BITS - SEC_SIZE_BITS)
#define SEC_PER_BUF           (1<<SEC_PER_BUF_BITS)
#define SEC_PER_BUF_MSK       (SEC_PER_BUF -1)

#define SEC_PER_LPN_BITS      3
#define SEC_PER_LPN           (1<<SEC_PER_LPN_BITS)
#define SEC_PER_LPN_MSK       (SEC_PER_LPN -1)

#define LPN_PER_BUF_BITS      (SEC_PER_BUF_BITS - SEC_PER_LPN_BITS)
#define LPN_PER_BUF           (1 << LPN_PER_BUF_BITS)
#define LPN_PER_SUPERBUF      (LPN_PER_BUF * LUN_NUM_PER_SUPERPU)
#define LPN_PER_BUF_MSK       (LPN_PER_BUF -1)
#define LPN_PER_BUF_BITMAP    ((1<< LPN_PER_BUF) - 1)

#define LPN_PER_SLC_RPMT      (LPN_PER_SUPERBUF)
#define LPN_PER_TLC_RPMT      (LPN_PER_SLC_RPMT * PG_PER_WL)

#define LPN_SIZE_BITS         12
#define LPN_SIZE              (1 << LPN_SIZE_BITS)

#define LPN_SECTOR_BIT        SEC_PER_LPN_BITS
#define LPN_SECTOR_NUM        SEC_PER_LPN
#define LPN_SECTOR_MSK        SEC_PER_LPN_MSK

#define GLOBAL_BLOCK_COUNT    1
#define BBT_BLOCK_COUNT       1
#define RT_BLOCK_COUNT        1
#define AT0_BLOCK_COUNT       2
#ifdef FLASH_IM_3DTLC_GEN2
#define AT1_BLOCK_COUNT       12
#else
#define AT1_BLOCK_COUNT       8   //Note: for TSB, AT1 block count is 16
#endif

#define TRACE_BLOCK_COUNT     2
#define RSVD_BLOCK_COUNT      1
#define TABLE_BLOCK_COUNT     (GLOBAL_BLOCK_COUNT + BBT_BLOCK_COUNT + RT_BLOCK_COUNT + AT0_BLOCK_COUNT + AT1_BLOCK_COUNT + TRACE_BLOCK_COUNT + RSVD_BLOCK_COUNT)

#ifdef FLASH_IM_3DTLC_GEN2
/*Increase 128G SLC cache for AS SSD Bench Mark 4k 64thrd ran write Performance */
#if ((4 == LUN_NUM_PER_SUPERPU) && defined(IM_3D_TLC_B16A))
#define TLC_BLK_CNT           422
#elif ((2 == LUN_NUM_PER_SUPERPU) && defined(IM_3D_TLC_B17A))
#define TLC_BLK_CNT           422
#else
#define TLC_BLK_CNT           442
#endif
#define DATA_BLOCK_PER_PU     398
#define VIR_BLK_CNT           (BLK_PER_PLN + RSV_BLK_PER_PLN - TABLE_BLOCK_COUNT - 6)
#else
/*Increase 128G SLC cache for AS SSD Bench Mark 4k 64thrd ran write Performance */
#if (3 == LUN_NUM_PER_SUPERPU) 
#define TLC_BLK_CNT           456
#else
#define TLC_BLK_CNT           472
#endif
#define DATA_BLOCK_PER_PU     424
#define VIR_BLK_CNT           (BLK_PER_PLN) //total Virtual Block Count
#endif

#define SLC_BLK_CNT           (VIR_BLK_CNT - TLC_BLK_CNT) //B0KB 128G:56,Others:40
#define BACKUP_BLK_CNT        (BLK_PER_PLN + RSV_BLK_PER_PLN - VIR_BLK_CNT - TABLE_BLOCK_COUNT)

#define PG_PER_SLC_BLK        (LOGIC_PG_PER_BLK/2)//(LOGIC_PG_PER_BLK/3)
#define PG_PER_TLC_BLK        PG_PER_BLK
#define SLC_PER_TLC           PG_PER_WL
#define TLC_BUF_CNT            3   //3D_TLC WriteTLCBuffers [TLC_BUF_CNT + 1 (Extra's Upper Page)]
#define LPN_PER_BLOCK         (LPN_PER_BUF * LOGIC_PG_PER_BLK)    //TLC mode : TLC size; MLC mode: MLC size
#define LPN_PER_SUPER_BLOCK   (LPN_PER_BLOCK * LUN_NUM_PER_SUPERPU)
#define LPN_PER_PU            (DATA_BLOCK_PER_PU * LPN_PER_BLOCK)
#define LPN_PER_SLC_BLK       (LPN_PER_BUF * PG_PER_SLC_BLK)
#define LPN_PER_SUPERPU       (DATA_BLOCK_PER_PU * LPN_PER_BLOCK * LUN_NUM_PER_SUPERPU)
#define LPN_PER_SUPER_SLCBLK  (LPN_PER_SLC_BLK * LUN_NUM_PER_SUPERPU)

#if defined(L1_FAKE) || defined(L2_FAKE)
#define MAX_LBA_IN_DISK       ((g_ulDramTotalSize / 2) >> SEC_SIZE_BITS ) //use half DRAM Total Size as Disk Size in Ramdisk
#define MAX_LPN_IN_DISK       ( MAX_LBA_IN_DISK >> SEC_PER_LPN_BITS )
#define MAX_LBA_IN_DISK_MAX   ( 256 << 11 )                               //256M Maximun LBA Ranges for table defines (1GB DRAM size)
#define MAX_LPN_IN_DISK_MAX   ( MAX_LBA_IN_DISK_MAX >> SEC_PER_LPN_BITS )
#else
#define MAX_LPN_IN_DISK       ( SUBSYSTEM_LUN_NUM * LPN_PER_PU )
#define MAX_LPN_IN_DISK_MAX   ( SUBSYSTEM_LUN_MAX * LPN_PER_PU )
#define MAX_LBA_IN_DISK       ( MAX_LPN_IN_DISK << LPN_SECTOR_BIT )
#define MAX_LBA_IN_DISK_MAX   ( MAX_LPN_IN_DISK_MAX << LPN_SECTOR_BIT )
#endif

#define RPMT_PAGE_COUNT_PER_PU              (TARGET_ALL - 1)
#define RPMT_SUPERPAGE_COUNT_PER_SUPERPU    (TARGET_ALL - 1)

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef FLASH_IM_3DTLC_GEN2
#define TLCW_SUPERPAGE_COUNT_PER_SUPERPU 6
#else
#define TLCW_SUPERPAGE_COUNT_PER_SUPERPU 5
#endif
#endif

#define GLOBAL_BLOCK_ADDR_BASE  0
#define BBT_BLOCK_ADDR_BASE     (GLOBAL_BLOCK_ADDR_BASE + GLOBAL_BLOCK_COUNT)
#define NORMAL_BLOCK_ADDR_BASE  (BBT_BLOCK_ADDR_BASE + 1)
#define RT_BLOCK_ADDR_BASE      (BBT_BLOCK_ADDR_BASE + BBT_BLOCK_COUNT)
#define AT0_BLOCK_ADDR_BASE     (RT_BLOCK_ADDR_BASE + RT_BLOCK_COUNT)
#define AT1_BLOCK_ADDR_BASE     (AT0_BLOCK_ADDR_BASE + AT0_BLOCK_COUNT)

#ifdef PMT_ITEM_SIZE_3BYTE

typedef struct PMT_ITEM_Tag
{
#if (((BLK_PER_PLN + RSV_BLK_PER_PLN) > 1024) && (LOGIC_PG_PER_BLK_BITS <= 8)) //For TSB flash

    #if (LOGIC_PIPE_PG_SZ == (32*1024)) // For TSB 2pln,15nm has 2048 Blocks
        U8 m_BlockInPuLow : 8;
        U8 m_BlockInPuHigh : 4;
        U8 m_PageInBlockLow : 4;
        U8 m_PageInBlockHigh : 4;
        U8 m_OffsetInSuperPage : 1;
        U8 m_LPNInPage : 3;
    #elif (LOGIC_PIPE_PG_SZ == (64*1024)) //For TSB 4pln
        U8 m_BlockInPuLow : 8;
        U8 m_BlockInPuHigh : 3;
        U8 m_PageInBlockLow : 5;
        U8 m_PageInBlockHigh : 3;
        U8 m_OffsetInSuperPage : 1;
        U8 m_LPNInPage : 4;
    #endif

#else //For L85/L95 flash,L95 has 24 Rsv Blk
        U8 m_BlockInPuLow : 8;
        U8 m_BlockInPuHigh : 3;
        U8 m_PageInBlockLow : 5;
        U8 m_PageInBlockHigh : 4;
        U8 m_OffsetInSuperPage : 1;
        U8 m_LPNInPage : 3;
#endif

}PMTItem;

#define PMT_ITEM_SIZE  (sizeof(PMTItem))
#else
#define PMT_ITEM_SIZE  (DWORD_SIZE)
    #ifdef PMT_ITEM_SIZE_REDUCE
    #define PMT_ITEM_SIZE_MIN  (26)
    #endif
#endif


#ifdef PMT_ITEM_SIZE_REDUCE

#define PMT_PAGE_SIZE  (LUN_NUM_PER_SUPERPU * BUF_SIZE)
#define LPN_CNT_PER_PMTPAGE_MIN  ((PMT_PAGE_SIZE / PMT_ITEM_SIZE) & (~ DWORD_BIT_SIZE_MSK))    //18720
#define PMTPAGE_VALID_LPN_MAP_SIZE_MAX (((PMT_PAGE_SIZE * 8) / PMT_ITEM_SIZE_MIN) % 32) ? (((PMT_PAGE_SIZE * 8) / PMT_ITEM_SIZE_MIN) / 32 + 1) : (((PMT_PAGE_SIZE * 8) / PMT_ITEM_SIZE_MIN) / 32 )

#define PMTPAGE_CNT_PER_PU_MAX ((LPN_PER_SUPERPU % LPN_CNT_PER_PMTPAGE_MIN) ? (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE_MIN + 1) : (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE_MIN))
#define PMTPAGE_CNT_PER_SUPERPU_MAX ((LPN_PER_SUPERPU % LPN_CNT_PER_PMTPAGE_MIN) ? (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE_MIN + 1) : (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE_MIN))//(1:1)358

#define PMT_DIRTY_BM_SIZE_MAX ((PMTPAGE_CNT_PER_SUPERPU_MAX % 32) ? (PMTPAGE_CNT_PER_SUPERPU_MAX / 32 + 1) : (PMTPAGE_CNT_PER_SUPERPU_MAX / 32))

#define PMT_DIRTY_BM_SIZE       ((g_ulPMTPageCntPerSPU % 32) ? (g_ulPMTPageCntPerSPU / 32 + 1) : (g_ulPMTPageCntPerSPU / 32))
#define LPN_CNT_PER_PMTPAGE     g_ulLPNCntPerPMTPage
#define PMTPAGE_CNT_PER_PU      g_ulPMTPageCntPerSPU
#define PMTPAGE_CNT_PER_SUPERPU g_ulPMTPageCntPerSPU
#else
#define PMT_PAGE_SIZE  (LUN_NUM_PER_SUPERPU * BUF_SIZE)
#define LPN_CNT_PER_PMTPAGE  ((PMT_PAGE_SIZE / PMT_ITEM_SIZE) & (~ DWORD_BIT_SIZE_MSK))    //10920

#define PMTPAGE_VALID_LPN_MAP_SIZE (LPN_CNT_PER_PMTPAGE / 32)

#define PMTPAGE_CNT_PER_PU ((LPN_PER_SUPERPU % LPN_CNT_PER_PMTPAGE) ? (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE + 1) : (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE))
#define PMTPAGE_CNT_PER_SUPERPU ((LPN_PER_SUPERPU % LPN_CNT_PER_PMTPAGE) ? (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE + 1) : (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE))//(1:1)358

#define PMT_DIRTY_BM_SIZE ((PMTPAGE_CNT_PER_SUPERPU % 32) ? (PMTPAGE_CNT_PER_SUPERPU / 32 + 1) : (PMTPAGE_CNT_PER_SUPERPU / 32))
#define PMTPAGE_CNT_TOTAL (PMTPAGE_CNT_PER_SUPERPU * SUBSYSTEM_SUPERPU_NUM) //1432 //358
#endif


typedef struct PhysicalPage_Tag
{
    U32 m_Content[(BUF_SIZE / DWORD_SIZE)];
}PhysicalPage;

typedef struct SuperPage_Tag
{
    PhysicalPage m_Content[LUN_NUM_PER_SUPERPU];
}SuperPage;

typedef enum _BLOCK_TYPE
{
    BLOCK_TYPE_GB = 0,
    BLOCK_TYPE_BBT,
    BLOCK_TYPE_RT,
    BLOCK_TYPE_AT0,
    BLOCK_TYPE_PMT,
    BLOCK_TYPE_TRACE,
    BLOCK_TYPE_RSVD,
    BLOCK_TYPE_SEQ,
    BLOCK_TYPE_RAN,
    BLOCK_TYPE_TLC_W,
    BLOCK_TYPE_TLC_GC,
    BLOCK_TYPE_EMPTY,
    BLOCK_TYPE_ERROR,
} BLOCK_TYPE;

typedef enum _PAGE_TYPE
{
    PAGE_TYPE_GB = 0,
    PAGE_TYPE_BBT,
    PAGE_TYPE_RT,
    PAGE_TYPE_PMTI,
#ifndef LCT_VALID_REMOVED
    PAGE_TYPE_VBMT,
#endif
    PAGE_TYPE_PBIT,
    PAGE_TYPE_VBT,
    PAGE_TYPE_RPMT,//7
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    PAGE_TYPE_TLCW,
#endif
    PAGE_TYPE_DPBM,
    PAGE_TYPE_PMT,
    PAGE_TYPE_TRACE,
    PAGE_TYPE_DATA,
    PAGE_TYPE_DUMMY,
    PAGE_TYPE_RSVD
} PAGE_TYPE;

typedef enum _OPERATION_TYPE
{
    OP_TYPE_GB_WRITE = 0,
    OP_TYPE_BBT_WRITE,
    OP_TYPE_RT_WRITE,
    OP_TYPE_PMTI_WRITE,
#ifndef LCT_VALID_REMOVED
    OP_TYPE_VBMT_WRITE,
#endif
    OP_TYPE_PBIT_WRITE,
    OP_TYPE_VBT_WRITE,
    OP_TYPE_RPMT_WRITE,
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    OP_TYPE_TLCW_WRITE,
#endif
    OP_TYPE_DPBM_WRITE,
    OP_TYPE_PMT_WRITE,
    OP_TYPE_TRACE_WRITE,
    OP_TYPE_HOST_WRITE,
    OP_TYPE_GC_WRITE,
    OP_TYPE_WL_WRITE,
    OP_TYPE_DUMMY_WRITE,
    OP_TYPE_BAK_WRITE,
    OP_TYPE_ALL,
} OP_TYPE;

/* Redundant common head defines */
typedef struct _REDUNDANT_COMM_
{
    /* Common Info : 2DW */
    U32 ulTimeStamp;

    union
    {
        U32 DW1;
        struct
        {
            U32 ulTargetOffsetTS : 5;
            U32 bsVirBlockAddr : 10;
            U32 bcBlockType : 4;
            U32 bcPageType : 4;
            U32 eOPType : 5;
            U32 bsRsV : 4;
        };
    };

}RedComm;

typedef struct _DATA_REDUNDANT_
{
    /* Data Block */
    U32 aCurrLPN[LPN_PER_BUF];

    /* TLC RPMT red save first page */
    U32 m_uTLCFirstPageTS;

    U16 bsTLCGC1stSrcBlock;
    U32 ulTLCGC1stSrcBlockTS;
}DataRed;

#define RED_MAP_TO_DRAM
typedef struct _SUBSYSTEM_MEMORY_BASE
{
    /* total 5 DWORD */
    U32 ulFreeSRAM0Base;
    U32 ulFreeSRAM1Base;
    U32 ulFreeOTFBBase;
    U32 ulFreeCacheStatusBase;
    U32 ulDRAMBase;

    // MCU12 Shared Base List
    U32 ulFreeSRAM1SharedBase;
    U32 ulFreeDRAMSharedBase;
    U32 ulRedInDramSharedBase;
    U32 ulRedInOtfbSharedBase;
    U32 ulSsuInDramSharedBase;
    U32 ulSsuInOtfbSharedBase;
}SUBSYSTEM_MEM_BASE;

extern GLOBAL MCU12_VAR_ATTR SUBSYSTEM_MEM_BASE g_FreeMemBase;
#ifndef MCU0
extern GLOBAL U32* g_pMCU1DramEndBase;
#else
extern GLOBAL U32* const g_pMCU1DramEndBase;
#endif
extern void DiskConfig_Init(void);
extern void DiskConfig_Check(void);
extern U8 DiskConfig_GetBootMethod(void);
extern BOOL DiskConfig_IsColdStart(void);
extern BOOL DiskConfig_IsLocalTestEn(void);
extern BOOL DiskConfig_IsRollBackECTEn(void);
extern BOOL DiskConfig_IsRebuildGBEn(void);
extern BOOL DiskConfig_IsUseDefaultECT(void);
extern U8 DiskConfig_GetSubSysNum(void);
extern U32 DiskConfig_GetSubSysCEBitMap(void);
extern U32 DiskConfig_GetSubSysMaxLBACnt(void);

U8   DiskConfig_GetLLFMethold(void);
void DiskConfig_SetLLFMethold(U8 ucLLFMethod);
void BootUpInitMultiCore(void);

#endif/* __DISK_CONFIG_H__ */


