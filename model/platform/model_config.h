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
#ifndef _MODEL_CONFIG_H
#define _MODEL_CONFIG_H

#include "Proj_Config.h"
#include "HAL_FlashChipDefine.h"

#define MAX_CMD_SEC_CNT     4 * 1024
#define MAX_ONE_CMD_SEC_CNT 256

#if !defined(BL_SUBSYSTEM_PU_NUM)
#ifdef FLASH_IM_3DTLC_GEN2
#ifdef FLASH_IM_3DTLC_GEN2_B17A
#define BL_SUBSYSTEM_PU_NUM 2
#else
#define BL_SUBSYSTEM_PU_NUM 4
#endif
#else
#define BL_SUBSYSTEM_PU_NUM 3
#endif
#endif

#define PU_SUM              (BL_SUBSYSTEM_PU_NUM)
#define MAX_LPN_IN_SYSTEM   (BL_SUBSYSTEM_PU_NUM * DATA_BLOCK_PER_PU * LPN_PER_BLOCK)

#define NFC_MODEL_LUN_SUM   (PU_SUM * NFC_LUN_PER_PU)

#define  MAX_LBA_IN_SYSTEM  (MAX_LPN_IN_SYSTEM << LPN_SECTOR_BIT)

#if  defined(L1_FAKE) || defined(L2_FAKE)
#undef MAX_LBA_IN_SYSTEM
#define MAX_LBA_IN_SYSTEM   (MAX_LBA_IN_DISK_MAX / 2)
#endif

#define MCU_MAX     3
#define MODEL_MAX   4

extern U32 g_ulMaxLBAInModel;

typedef enum _HOST_STATUS
{
    HOST_STATUS_INIT = 0,
    HOST_STATUS_DO_POWERUP,
    HOST_STATUS_HANDSHAKING,
    HOST_STATUS_RUNNING,
    HOST_STATUS_WAIT_L3_IDLE,
    HOST_STATUS_DO_POWERDOWN,
    HOST_STATUS_REDO_LLF,
    HOST_STATUS_ALL,
}HOST_STATUS;

typedef void (*PSIMEventHandler)();
PSIMEventHandler SimEventHandlerArray[HOST_STATUS_ALL];

typedef struct _WIN_SIM_MGR
{
    volatile U32 ulStatus;
    HANDLE ulMCUThreadID[MCU_MAX];
    PSIMEventHandler ulHandler;
    HANDLE hlMCUSyncEvent[MCU_MAX];
    HANDLE hlMCUResetEvent[MCU_MAX];
    HANDLE hlThreadHandle[MCU_MAX];
    HANDLE hlModelResetEvent[MODEL_MAX];
    U8 bSearchLBA;
    U8 bCacSubLBA;
    U8 bPrintFlashLog;
    U32 ulSearchedLBA;
    U8 bNeedLLF;
    U8 bReportWL[MCU_MAX-1];
    U8 bPrintBadBlock;
}WIN_SIM_MGR;

#if defined(HOST_SATA)
#define SGE_ENABLE 0
#define HCT_ENABLE 0
#else
#define SGE_ENABLE 1
#define HCT_ENABLE 1
#endif

#endif
