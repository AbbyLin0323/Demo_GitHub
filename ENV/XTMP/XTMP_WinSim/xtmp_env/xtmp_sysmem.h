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
#ifndef __XTMP_SYSMEM_H__
#define __XTMP_SYSMEM_H__

#include "iss/mp.h"

#define READ_DELAY  1
#define BLOCK_READ_DELAY 1
#define BLOCK_READ_REPEAT 1
#define WRITE_DELAY 1
#define BLOCK_WRITE_DELAY 1
#define BLOCK_WRITE_REPEAT 1

#define PAGE_BITS  16
#define PAGE_SIZE  (1 << PAGE_BITS)
#define PAGE_MASK  (PAGE_SIZE - 1)
#define PAGE_COUNT (1<<(32 - PAGE_BITS))// 1 page = 0~0x10000 = 64K.  (1 << (32 - PAGE_BITS)) 4G

typedef struct {
  u32             *pages[PAGE_COUNT];	/* data allocated in pages */
  XTMP_deviceXfer *xfer;		/* transaction being processed */
  XTMP_time        responseTime;	/* when to respond */
  u32              transferNumber;	/* for block transactions only */
} SysMemData;

extern SysMemData sysPIFRegData;

extern void dramRead(u32 addr, u32 nWordss, u32 *buf);
extern void dramWrite(u32 addr, u32 nWordss, const u32* src);
extern void dramReadByByte(u32 addr, u32 nBytes, u8 *buf);
extern void dramWriteByByte(u32 addr, u32 nBytes, u8 *buf);
extern XTMP_deviceStatus sysmemPeek(void *deviceData, u32 *dst, XTMP_address addr, u32 size);
extern XTMP_deviceStatus sysmemPoke(void *deviceData, XTMP_address addr, u32 size, const u32 *src);
extern XTMP_deviceStatus sysmemPost(void *deviceData, XTMP_deviceXfer *xfer);
extern void sysmemTicker(void *threadData);
extern void sysmemFastReadLE(void *deviceData, u32 *dst, XTMP_address addr, u32 size);
extern void sysmemFastWriteLE(void *deviceData, XTMP_address addr, u32 size, const u32 *src);
extern void sysmemFastReadBE(void *deviceData, u32 *dst, XTMP_address addr, u32 size);
extern void sysmemFastWriteBE(void *deviceData, XTMP_address addr, u32 size, const u32 *src);
extern XTMP_deviceStatus sysmemFastAccess(void *deviceData,
										  XTMP_fastAccessRequest request,
										  XTMP_address addr);
#endif//__XTMP_SYSMEM_H__
