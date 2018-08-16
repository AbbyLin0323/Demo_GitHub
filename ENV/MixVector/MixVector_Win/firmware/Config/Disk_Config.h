/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :                                           
Version      :  Ver 0.1                                              
Date         :                                         
Author       :  Blake Zhang

Description: 
    project configuration setting
Modification History:
20140902     blakezhang     001 create file
*************************************************/
#ifndef __DISK_CONFIG_H__
#define __DISK_CONFIG_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_ParamTable.h"
#include "HAL_HostInterface.h"
#include "HAL_FlashChipDefine.h"

#define   SUB_SYSTEM_NUM_MAX     2

/* SubSystem used CacheStatus and FlashStatus */
#define SUBSYSTEM_STATUS_INIT         0x2
#define SUBSYSTEM_STATUS_SUCCESS      0x0
#define SUBSYSTEM_STATUS_PENDING      0x1
#define SUBSYSTEM_STATUS_FAIL         0x3
#define SUBSYSTEM_STATUS_EMPTY_PG     0x4
#define SUBSYSTEM_STATUS_INIT_DW      0x02020202  /* for DMAE init */

#define   SEC_PER_BUF_BITS       6
#define   SEC_PER_BUF           (1<<SEC_PER_BUF_BITS)
#define   SEC_PER_BUF_MSK       (SEC_PER_BUF -1)

#define   BUF_SIZE_BITS         (SEC_PER_BUF_BITS + SEC_SIZE_BITS)  
#define   BUF_SIZE              (1 << BUF_SIZE_BITS)      
#define   BUF_SIZE_MSK          (BUF_SIZE - 1)

#define   SEC_PER_LPN_BITS        3
#define   SEC_PER_LPN            (1<<SEC_PER_LPN_BITS)
#define   SEC_PER_LPN_MSK        (SEC_PER_LPN -1)

#define   LPN_PER_BUF_BITS       (SEC_PER_BUF_BITS - SEC_PER_LPN_BITS)
#define   LPN_PER_BUF            (1 << LPN_PER_BUF_BITS)
#define   LPN_PER_BUF_MSK        (LPN_PER_BUF -1)

#define   LPN_SIZE_BITS           12
#define   LPN_SIZE               (1 << LPN_SIZE_BITS)

#define   LPN_SECTOR_BIT     SEC_PER_LPN_BITS
#define   LPN_SECTOR_NUM     SEC_PER_LPN
#define   LPN_SECTOR_MSK     SEC_PER_LPN_MSK

#define PG_PER_SLC_BLK (PG_PER_BLK >> 1)

//6.87% for 2K Blocks/CE
#define RSVD_BLK_PER_PLN          70   //140, modify by henry to hack L3 llf bad block.
#define DATA_BLOCK_PER_PU (BLK_PER_CE - RSVD_BLK_PER_PLN)
#define LPN_PER_BLOCK     (LPN_PER_BUF * PG_PER_BLK)
#define LPN_PER_PU        (DATA_BLOCK_PER_PU * LPN_PER_BLOCK)
#define MAX_LPN_IN_DISK (PU_NUM * LPN_PER_PU)
#define MAX_LBA_IN_DISK (MAX_LPN_IN_DISK << LPN_SECTOR_BIT)


#define GLOBAL_BLOCK_ADDR_BASE  0
#define NORMAL_BLOCK_ADDR_BASE  (BBT_BLOCK_ADDR_BASE+1)

#define GLOBAL_BLOCK_COUNT      1
#define BBT_BLOCK_COUNT         1
#define RT_BLOCK_COUNT          1
#define AT0_BLOCK_COUNT         2
#define AT1_BLOCK_COUNT         8   //Note: for TSB, AT1 block count is 16

#define TRACE_BLOCK_COUNT       2
#define RSVD_BLOCK_COUNT        1

#define RPMT_PAGE_COUNT_PER_PU  2

#define GLOBAL_BLOCK_ADDR_BASE  0
#define BBT_BLOCK_ADDR_BASE    (GLOBAL_BLOCK_ADDR_BASE + GLOBAL_BLOCK_COUNT)
#define NORMAL_BLOCK_ADDR_BASE  (BBT_BLOCK_ADDR_BASE+1)
#define RT_BLOCK_ADDR_BASE (BBT_BLOCK_ADDR_BASE + BBT_BLOCK_COUNT)
#define AT0_BLOCK_ADDR_BASE (RT_BLOCK_ADDR_BASE + RT_BLOCK_COUNT)
#define AT1_BLOCK_ADDR_BASE (AT0_BLOCK_ADDR_BASE + AT0_BLOCK_COUNT)

#ifdef PMT_ITEM_SIZE_3BYTE
//#pragma pack(1) /* force 1 byte aligned */ 
typedef struct PMT_ITEM_Tag
{
    U8 m_BlockInPuLow:8;
    U8 m_BlockInPuHigh:4;
    U8 m_PageInBlockLow:4;
    U8 m_PageInBlockHigh:5;
    U8 m_LPNInPage:3;
}PMTItem;
//#pragma pack()  /* cancel force aligned */ 

#define PMT_ITEM_SIZE  (sizeof(PMTItem))
#else
#define PMT_ITEM_SIZE  (DWORD_SIZE)
#endif

#define PMT_PAGE_SIZE  (BUF_SIZE)
#define LPN_CNT_PER_PMTPAGE  ((PMT_PAGE_SIZE / PMT_ITEM_SIZE) & (~ LPN_PER_BUF_MSK))    //10920

#define PMTPAGE_CNT_PER_PU ((LPN_PER_PU % LPN_CNT_PER_PMTPAGE) ? (LPN_PER_PU / LPN_CNT_PER_PMTPAGE + 1) : (LPN_PER_PU / LPN_CNT_PER_PMTPAGE))
#define PMT_DIRTY_BM_SIZE ((PMTPAGE_CNT_PER_PU % 32) ? (PMTPAGE_CNT_PER_PU / 32 + 1) : (PMTPAGE_CNT_PER_PU / 32))
#define PMTPAGE_CNT_TOTAL (PMTPAGE_CNT_PER_PU * PU_NUM)


typedef struct PhysicalPage_Tag
{
    U32 m_Content[(BUF_SIZE / DWORD_SIZE)];
}PhysicalPage;

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
    BLOCK_TYPE_EMPTY
} BLOCK_TYPE;

typedef enum _PAGE_TYPE
{
    PAGE_TYPE_GB = 0,
    PAGE_TYPE_BBT,
    PAGE_TYPE_RT,
    PAGE_TYPE_PMTI,
    PAGE_TYPE_VBMT,
    PAGE_TYPE_PBIT,
    PAGE_TYPE_VBT,
    PAGE_TYPE_RPMT,
    PAGE_TYPE_DPBM,
    PAGE_TYPE_PMT,
    PAGE_TYPE_TRACE,
    PAGE_TYPE_DATA,
    PAGE_TYPE_RSVD
} PAGE_TYPE;

typedef enum _OPERATION_TYPE
{
    OP_TYPE_GB_WRITE = 0,
    OP_TYPE_BBT_WRITE,
    OP_TYPE_RT_WRITE,
    OP_TYPE_PMTI_WRITE,
    OP_TYPE_VBMT_WRITE,
    OP_TYPE_PBIT_WRITE,
    OP_TYPE_VBT_WRITE,
    OP_TYPE_RPMT_WRITE,
    OP_TYPE_DPBM_WRITE,
    OP_TYPE_PMT_WRITE,
    OP_TYPE_TRACE_WRITE,
    OP_TYPE_HOST_WRITE,
    OP_TYPE_GC_WRITE,
    OP_TYPE_WL_WRITE,
    OP_TYPE_DUMMY_WRITE,

    OP_TYPE_GB_READ,
    OP_TYPE_BBT_READ,
    OP_TYPE_RT_READ,
    OP_TYPE_PMTI_READ,
    OP_TYPE_VBMT_READ,
    OP_TYPE_PBIT_READ,
    OP_TYPE_VBT_READ,
    OP_TYPE_RPMT_READ,
    OP_TYPE_DPBM_READ,
    OP_TYPE_PMT_READ,
    OP_TYPE_TRACE_READ,
    OP_TYPE_HOST_READ,
    OP_TYPE_GC_READ,
    OP_TYPE_WL_READ,

    OP_TYPE_GB_ERASE,
    OP_TYPE_BBT_ERASE,
    OP_TYPE_RT_ERASE,
    OP_TYPE_PMTI_ERASE,
    OP_TYPE_VBMT_ERASE,
    OP_TYPE_PBIT_ERASE,
    OP_TYPE_VBT_ERASE,
    OP_TYPE_RPMT_ERASE,
    OP_TYPE_DPBM_ERASE,
    OP_TYPE_PMT_ERASE,
    OP_TYPE_TRACE_ERASE,
    OP_TYPE_HOST_ERASE,
    OP_TYPE_GC_ERASE,
    OP_TYPE_WL_ERASE,
    OP_TYPE_ALL,
} OP_TYPE;

/* Redundant common head defines */
typedef struct _REDUNDANT_COMM_
{
    /* Common Info : 3 DW */
    U32 ulTimeStamp;
    U32 bsVirBlockAddr:16;
    BLOCK_TYPE bcBlockType:8;
    PAGE_TYPE  bcPageType:8;
    OP_TYPE    eOPType;

#ifdef SIM
    U32 ulMCUId;
#endif
}RedComm;

typedef struct _DATA_REDUNDANT_
{
    /* Data Block */
    U32 aCurrLPN[LPN_PER_BUF];
}DataRed;

extern void DiskConfig_Init(void);
extern void DiskConfig_Check(void);
extern U8 DiskConfig_GetBootMethod(void);
extern BOOL DiskConfig_IsColdStart(void);
extern BOOL DiskConfig_IsLocalTestEn(void);
extern BOOL DiskConfig_IsRollBackECTEn(void);
extern BOOL DiskConfig_IsRebuildGBEn(void);
extern BOOL DiskConfig_IsFormatBBTEn(void);
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

extern BOOL DiskConfig_IsScanIDBEn(void);
extern BOOL DiskConfig_IsUseDefaultECT(void);
extern U8 DiskConfig_GetSubSysNum(void);
extern U32 DiskConfig_GetSubSysCEBitMap(void);
extern U32 DiskConfig_GetSubSysMaxLBACnt(void);

#endif/* __DISK_CONFIG_H__ */


