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
#define rSPINLOCK ((volatile U32 *)SPINLOCK_BASE_ADDRESS)

/* Spin lock number used currently
   Note: In VT3514 A0/B0, we can not use spin lock with id = x, where: (x % 16 == 5) || (x % 16 == 7)
*/
#define SPINLOCKID_SUBSYS_PRINT                 0
#define SPINLOCKID_SUBSYS_SGE                   1
#define SPINLOCKID_SUBSYS_INTERMCUINTR          2
#define SPINLOCKID_SE                           3
#define SPINLOCKID_SATA_DSG                     4
#define SPINLOCKID_GLB40                        5
#define SPINLOCKID_BUFMAP                       6
#define SPINLOCKID_DMAE                         7
#define SPINLOCKID_SATA_DSG_REPORT              8
#define SPINLOCKID_DATA_CHECK                   9
#define SPINLOCKID_FDR                          10
#define SPINLOCKID_SUBSYS_INIT                  11
#define SPINLOCKID_CLKGATING                    12
#define SPINLOCKID_UECC_RSP_MARKER              13
#define SPINLOCKID_ALLC_DSG_OR_HSG              14
#define SPINLOCKID_ERRH_PEND_CMD                15
#define SPINLOCKID_ALLOCATE_FREE_BLK            16
#define SPINLOCKID_ALLOCATE_BROKEN_BLK          17
#define SPINLOCKID_CHAIN_COUNT                  18

/* The maximum subsystem MCU count inside the system. */
#define SUBSYSTEM_NUM_MAX 1

void HAL_StartSubSystemMCU(U8 ulSubSysIdx);
void HAL_HaltSubSystemMCU(U32 ulStall);
void MULTI_CORE_TEXT_ATTR HAL_MultiCoreReleaseSpinLock(U8 ucLockId);
BOOL MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLock(U8 ucLockId);
void MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLockWait(U8 ucLockId);

#endif/* __HAL_MULTICORE_H__ */

