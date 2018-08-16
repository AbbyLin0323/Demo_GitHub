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
Filename    : HAL_MultiCore.h
Version     : Ver 1.0
Author      : HenryLuo
Date        : 2012.02.14
Description : encapsulete subsystem boot and SpinLock relative interface.
Others      : 
Modify      :
20140905    Tobey     uniform coding style.
*******************************************************************************/

#ifndef __HAL_MULTICORE_H__
#define __HAL_MULTICORE_H__
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

/* the "MCU0" MACRO is defined in MCU0's Proj_Config.h file */
#ifdef MCU0
#define MULTI_CORE_TEXT_ATTR MCU0_DRAM_TEXT
#else
#define MULTI_CORE_TEXT_ATTR 
#endif

/* SPIN LOCK */
#define SPINLOCK_BASE_ADDRESS (REG_BASE_GLB + 0x100)
#define SPIN_LOCK_COUNT 128

/* Spin lock number used currently */
#define SPINLOCKID_SUBSYS_PRINT 0
#define SPINLOCKID_SUBSYS_SGE   1
#define SPINLOCKID_SUBSYS_INTERMCUINTR 2
#define SPINLOCKID_GLB40 5

#ifdef VT3514_A0
#define SPINLOCKID_DMAE 2
#endif

#if defined(VT3514_A0) || defined(VT3514_B0)
#define SPINLOCKID_SE 3
#endif

#define SPINLOCKID_SATA_DSG    4
#define SPINLOCKID_BUFMAP 6
#define SPINLOCKID_SATA_DSG_REPORT   8
#define SPINLOCKID_FDR 10

/* The maximum subsystem MCU count inside the system. */
#define SUBSYSTEM_NUM_MAX 2

void MULTI_CORE_TEXT_ATTR HAL_StartSubSystemMCU(U8 ulSubSysIdx);
void MULTI_CORE_TEXT_ATTR HAL_MultiCoreReleaseSpinLock(U8 ucLockId);
BOOL MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLock(U8 ucLockId);
void MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLockWait(U8 ucLockId);

#endif/* __HAL_MULTICORE_H__ */

