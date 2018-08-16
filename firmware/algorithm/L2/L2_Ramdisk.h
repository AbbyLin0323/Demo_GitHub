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
Filename     : L2_Ramdisk.h
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20150302     BlakeZhang     001 first create
****************************************************************************/
#ifndef _L2_RAMDISK_H
#define _L2_RAMDISK_H

#ifdef L2_FAKE
#define L2_RAMDISK_MAX_LBA  (MAX_LBA_IN_DISK)
#define L2_RAMDISK_SIZE     (MAX_LBA_IN_DISK*SEC_SIZE)

extern void L2_RamdiskInit(U32 *pFreeDramBase);
extern void L2_RamdiskTrimProcess(U32 ulStartLPN, U32 ulLPNCount);
extern BOOL L2_RamdiskIsEnable(U8 ucStage);
extern U32 L2_RamdiskGetDramAddr(U32 ulSubSysLBA);
extern void L2_RamdiskSchedule(void);

#endif

#endif

/********************** FILE END ***************/
