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
Filename    : HAL_Pciemp.h
Version     : Ver 1.0
Author      : John
Date        : 2017.08.17
Description : this file declare pciemp function
Modify      :

*******************************************************************************/
#ifndef __HAL_PCIEMP_H__
#define __HAL_PCIEMP_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#define PCIE_SIG            (0x6740)
#define REG_BASE_AHCI_MP    (REG_BASE_AHCI + 0xA0)
#define REG_BASE_NVME_MP    (REG_BASE_NVME + 0x200)
#define BIT_MP_INT      (1<<8)
#define BIT_HR_INT      (1<<16)
#define rHostIntSrcReg  (*(volatile U32 *)(REG_BASE_HOSTC + 0x10))

typedef union HOSTC_INTMSK_REG_ {
    struct {
        U32 bsRes1_0 :8;
        U32 bsIntMMptTrig :1;
        U32 bsRes1_1 :7;
        U32 bsIntMGhcHrSet:1;
        U32 bsRes1_2 :15;
    };
    U32 ulValue;
} HOSTC_INTMSK_REG;

typedef struct _MP_REG_SET {
#if 1
    U32 bsSig :16;
    U32 bsRes0 :15;
    U32 bsMpEn :1;

    U32 bsRes1_1 :7;
    U32 bsIntEn :1;
    U32 bsCmdType :8;
    U32 bsCmdStatus :8;
    U32 bsRes1_2 :7;
    U32 bsCmdTrig :1;
#endif
   U32 ulParam[4];

} MP_REG_SET;

typedef enum _PCIE_MP_STAT{
    STAT_ISSUE_CMD,
    STAT_WAITING,
    STAT_SEND_STAT,
    STAT_FINISH
    
}PCIE_MP_STAT;
#endif /* __HAL_XTENSA_H__ */

