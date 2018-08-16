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
Filename     : L1_Ramdisk.h
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20150302     BlakeZhang     001 first create
****************************************************************************/
#ifndef _L1_RAMDISK_H
#define _L1_RAMDISK_H

#include "L0_Interface.h"

#ifdef L1_FAKE
#define L1_DEBUG_RAMDISK_MAX_LBA  (MAX_LBA_IN_DISK)
#define L1_DEBUG_RAMDISK_SIZE     (MAX_LBA_IN_DISK*SEC_SIZE)
#else
#define L1_DEBUG_RAMDISK_SIZE     (128 * 1024)
#define L1_DEBUG_RAMDISK_MAX_LBA  (L1_DEBUG_RAMDISK_SIZE/SEC_SIZE)
#endif

typedef enum _L1_RAMDISK_MODE_
{
    L1_RAMDISK_DISABLE = 0,
    L1_RAMDISK_ENABLE_SPECIAL,
    L1_RAMDISK_ENABLE_NORMAL
} L1_RAMDISK_MODE;

extern void L1_RamdiskDramMap(U32 *pFreeDramBase);
extern void L1_RamdiskOTFBMap(U32 *pFreeOTFBBase);
extern void L1_RamdiskInit(void);
extern BOOL L1_RamdiskIsEnable(void);
extern void L1_RamdiskSetMode(U32 ulMode);
extern U32 L1_RamdiskGetDramAddr(SCMD* pSCmd);
extern void L1_RamdiskSCmdFinish(void);
extern void L1_RamdiskSCmdFail(void);
extern BOOL L1_RamdiskSchedule(void);
extern void L1_RamdiskReportGetchSMSG(void);
extern void L1_RamdiskFinishGetchSMSG(void);
extern void L1_RamdiskSwitchToSpecialMode(void);

#endif

/********************** FILE END ***************/
