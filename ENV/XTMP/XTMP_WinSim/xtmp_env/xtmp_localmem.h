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
#ifndef __XTMP_LOCALMEM_H__
#define __XTMP_LOCALMEM_H__

#include "iss/mp.h"
#include "xtmp_common.h"
#include "model_common.h"

#define MAX_DATA_RAM0_SECTION CORE_NUM//2 
#define MAX_DATA_RAM1_SECTION 1 //every mcu has one device memory
#define MAX_IRAM_SECTION CORE_NUM//every mcu has one isram

typedef struct {
  u8          *data;
  XTMP_address base;
  u32          size;
  u32          byteWidth;
  bool         bigEndian;
  bool         hasBusy;
} LocalMemoryData;

extern LocalMemoryData dataRam0Struct,dataRam1Struct;
extern LocalMemoryData g_DsramStruct[];
extern LocalMemoryData g_DsramShareStruct[];
extern LocalMemoryData g_IsramStruct[];

extern void regRead(u32 addr, u32 nBytes, u8 *dst);
extern void regWrite(u32 addr, u32 nBytes, const u8 *src);

extern XTMP_deviceStatus
memPeek(void *deviceData, u32 *dst, XTMP_address addr, u32 size);

extern XTMP_deviceStatus
memPoke(void *deviceData, XTMP_address addr, u32 size, const u32 *src);

extern XTMP_deviceStatus
memPost(void *deviceData, XTMP_deviceXfer *xfer);

extern XTMP_deviceStatus
memFastAccess(void *deviceData,
              XTMP_fastAccessRequest request,
              XTMP_address addr);

extern u8* GetVirtualAddrInLocalMem(XTMP_address addr);

#endif//__XTMP_LOCALMEM_H__

