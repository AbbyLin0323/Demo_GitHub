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
* File Name    : MixVector_DRAM.h
* Discription  : common interface for read/write DRAM interface; cache status interface
* CreateAuthor : Gavin
* CreateDate   : 2013.11.12
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef __MIX_VECTOR_DRAM_H__
#define __MIX_VECTOR_DRAM_H__

#include "HAL_MemoryMap.h"
#include "MixVector_Interface.h"

/* disk B start at: DRAM base + 32MB */
#define DISK_B_DRAM_BASE        (DRAM_START_ADDRESS+0x2000000)
#define CS_BUSY  1
#define CS_FREE  0

#define DISK_B_UNIT  2


extern BOOL CheckCacheStatusBusy(U8 ucCmdSlot);
extern BOOL DWQ_BuildHostWriteReq(HOST_MEM_REQ *pHostReq,BOOL bSplitEnable);
extern BOOL DRQ_BuildHostReadReq(HOST_MEM_REQ *pHostReq,BOOL bSplitEnable);
extern BOOL ProcessDiskB(U8 ucCmdSlot);

extern U8 *g_pCacheStatus;

#endif/* __MIX_VECTOR_DRAM_H__ */

