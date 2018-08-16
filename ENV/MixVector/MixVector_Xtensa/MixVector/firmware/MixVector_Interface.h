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
Filename     : MixVector_Interface.h                                    
Version      : Ver 1.0                                               
Date         :                                         
Author       : 

Description: 
Windows pattern for MixVector verification in VT3514 PCIE SSD
Modification History:
20140718    Gavin   created
------------------------------------------------------------------------------*/
#ifndef __MIX_VECTOR_INTERFACE_H__
#define __MIX_VECTOR_INTERFACE_H__

#include "BaseDef.h"
#include "HAL_FlashChipDefine.h"

/*--------------------------- interface with host ----------------------------*/
#define SUB_DISK_CNT 3
#define SUB_DISK_A 0
#define SUB_DISK_B 1
#define SUB_DISK_C 2
#define FLASH_ADDR_CNT 32//8//CE_NUM


#ifndef MAX_SLOT_NUM
#define MAX_SLOT_NUM 32
#endif

//#define PAGE_SIZE (32<<10)


#define SUB_DISK_A_SIZE  (24 << 10)
#define SUB_DISK_B_SIZE  (128 << 20)
#define SUB_DISK_C_SIZE  (128 << 20)
#define MV_BLOCK_PER_PU  (512)
#define MV_PAGE_PER_BLOCK (256)
#define MV_PAGE_SIZE     (32 << 10)

//#define NEED_BLOCK_PER_PU (SUB_DISK_C_SIZE/(FLASH_ADDR_CNT * MV_PAGE_PER_BLOCK * PAGE_SIZE))
typedef enum _HOST_REQ_TYPE
{
    HOST_REQ_READ = 0,
    HOST_REQ_WRITE,
    HOST_REQ_ERASE,
    HOST_REQ_OTHER,
    HOST_REQ_INVALID
}HOST_REQ_TYPE;

typedef struct _SUB_DISK_CMD
{
    U32 DiskEn: 1;
    U32 ReqType: 2;
    U32 SplitEnable:1;
    U32 Dw0Rsvd: 4;
    U32 StartUnit: 24;//for diskA, unit means byte; for read/write diskB/C, means sector
    U32 UnitLength;// for erase in diskC, UnitLength means number of block to erase
    U32 HostAddrLow;
    U32 HostAddrHigh;
}SUB_DISK_CMD;// 4DW

typedef union _TARGET_FLASH_ADDR
{
    struct {
    U32 PU: 5;
    U32 Block: 11;
    U32 Page: 9;
    U32 Rsvd: 7;
    };
    U32 ulAsU32;
}TARGET_FLASH_ADDR; // 1DW

typedef struct _HOST_CMD
{
    SUB_DISK_CMD SubDiskCmd[SUB_DISK_CNT];//dword0~dowrd 11

    TARGET_FLASH_ADDR FlashAddrGroup[FLASH_ADDR_CNT];//dword12 --19

    U32 FinishCnt[SUB_DISK_CNT];  //calculate the xfer length//dowrd20--22
    
    U32 ErrFlag;//dword23
    U16 HsgLen[64];//dword24--55
}HOST_CMD;

/*-------------------------- interface to sub-module -------------------------*/
/* host command stage */
typedef enum _HOST_CMD_STAGE
{
    HCMD_STAGE_INIT = 0,
    HCMD_STAGE_WAIT_CMD,
    HCMD_STAGE_BUILD_FCQ,
    HCMD_STAGE_WAIT_FCQ_DATA_DONE,
    HCMD_STAGE_PROCESS,
    HCMD_STAGE_WAIT_COMPLETE,
    HCMD_STAGE_INVALID
}HOST_CMD_STAGE;


/* CST status in each slot */
typedef enum _CST_STATUS
{
    CST_STATUS_INIT = 0,
    CST_STATUS_CMD_RCV,
    CST_STATUS_DATA_DONE,
    CST_STATUS_TRIG_WBQ,
    CST_STATUS_INVALID
}CST_STATUS;

typedef struct _HOST_CMD_MANAGER
{
    U32 Ssu1En: 1;// for erase diskC, should enable SSU1
    U32 CacheStatusEn: 1;// when read/write diskB, if erase diskC at the same time, cache status should enable
    U32 WaitCstEn: 1;// for write diskA, wait FCQ done before process diskB/C
    U32 WbqWaitSgeEn: 1;// if read/write diskB/C, enable 'wait data done' in WBQ
    U32 WbqWritePtr: 2;// record WBQ write pointer
    U32 Stage: 4;// see HOST_CMD_STAGE definition
    U32 WbqWaitNfcEn: 1; // control WBQ need wait NFC program ok or not
    U32 bDiskBFirstHSGGet : 1;
    U32 bDiskCFirstHSGGet : 1;
    U32 Rsvd: 19;
    U32 FinishCnt[SUB_DISK_CNT];  //record FW processsed count
    U32 HSGPtr[SUB_DISK_CNT];  //record Hcmd HSG get ptr count
    U16 FirstHSGDiskB;
    U16 FirstHSGDiskC;
    U16 ContinueHSGDiskB;
    U16 ContinueHSGDiskC;
    U32 StartUnitInByte[SUB_DISK_CNT];
    U32 UnitLenthInByte[SUB_DISK_CNT];
    SUB_DISK_CMD SubDiskBCmd;  
    SUB_DISK_CMD SubDiskCCmd;  
}HOST_CMD_MANAGER;

/* descriptor of request to SRAM/DRAM */
typedef struct _HOST_MEM_REQ_
{
    U32 ReqType: 2;
    U32 HID: 6;
    U32 UpdateCstEn: 1;
    U32 NextCst: 4;
    U32 ClearCI: 1;
    U32 WaitSge: 1;
    U32 CmdSubId: 4;
    U32 CacheStsEn:1;
    U32 Dw0Rsvd: 12;
    
    U32 LBA;

    U32 ByteLength;
    U32 DSGByteLength;
    U32 LocalAddr;

    U32 HostAddrLow;
    U32 HostAddrHigh;
}HOST_MEM_REQ;

/* descriptor of request to one NFC PU */
typedef struct _FLASH_REQ_FROM_HOST_
{
    U32 Type: 2;
    U32 HID: 6;
    U32 StartSec: 8;// used for read/write request
    U32 Seclength: 8;// used for read/write request
    U32 CmdSubId: 4;// sub id in a host command to diskC   // ?
    U32 Dw0Rsvd: 4;
    
    U32 PU: 5;
    U32 Block: 11;
    U32 Page: 9;
    U32 Dw1Rsvd: 7;

    U32 StartSecInByte;// used for read/write request
    U32 SeclengthInByte;// used for read/write request
    U32 HostAddrLow;
    U32 HostAddrHigh;
}FLASH_REQ_FROM_HOST;

/* debug and trace interface */
//extern void DBG_Getch(void);
//#define DBG_Printf dbg_printf

extern HOST_CMD *g_pHCmdTable;
extern HOST_CMD_MANAGER g_aHostCmdManager[];



int MV_Schedule(void);
void MixVectorInit(void);
#endif/* __MIX_VECTOR_INTERFACE_H__ */

