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
/******************************************************************************
Filename    : HAL_NVMECFGEX.h 
Version     : Ver 1.0
Author      : Janka Lun
Date        : 2015.08.05
Description : header file for defining NVMeCFGEx related HW interface in VT3533 
              NVMe mode
Others      : 
Modify      :
20150805    Janka     Create file
*******************************************************************************/

#ifndef _HAL_NVMECFGEX_H_
#define _HAL_NVMECFGEX_H_
#ifdef __cplusplus
extern "C"{
#endif
#include "BaseDef.h"

typedef struct _SQ_CFG_ATTR{
    volatile U32 BaseAddrL;             /* SQx_BASE_ADDRL */
    volatile U32 BaseAddrH;             /* SQx_BASE_ADDRH */
    
    volatile U32 StepAddr:      8;      /* SQx_SETUP_ADDR */
    volatile U32 BM:            8;      /* SQx Max Num of Cmds */
    volatile U32 P:             5;      /* Priority of SQ */
    volatile U32 Rsv0:          3;    
    volatile U32 CQMaped:       4;      /* SQx_CQ_MAP */
    volatile U32 Rsv1:          4;
    
    volatile U32 HWRP:          16;     /* HW Read Pointer */
    volatile U32 Size:          16;     /* SQx Size */
}SQ_CFG_ATTR, *PSQ_CFG_ATTR;


typedef struct _SQ_ENTRY{
    volatile U32 HWRP:          16;     /* SQ HW Read Pointer of CST EntryX */
    volatile U32 SQID:          4;      /* SQ Id of CST EntryX */
    volatile U32 Rsv0:          12;    
}SQ_ENTRY, *PSQ_ENTRY;
    
    
typedef struct _NVME_CFG_EX {
    /* NVMe Auto-Fetch Cfg Registers */
    volatile U32 Rsv0:          4;
    volatile U32 CstAutoTrig:   4;      /* CST_AUTO_TRIG */
    volatile U32 Rsv1:          24;
    
    volatile U32 CmdBaseAddr:   16;     /* NVME_CMD_BASE_ADDR */
    volatile U32 Rsv2:          16;
    
    volatile U32 CmdLen:        8;      /* NVME_CMD_LEN */
    volatile U32 CmdOffset:     8;      /* NVME_CMD_OFFSET */
    volatile U32 Rsv3:          16;
    
    volatile U32 Rsv4;

    /* NVMe SQ Cfg Registers */
    SQ_CFG_ATTR SQCfgAttr[9];           /* MAX_SQ_NUM == 9 */
    
    /* NVME SQ Entry, Containing SlotID<->SQID<->SQIndex(HWRP) bonding information */
    SQ_ENTRY SQEntry[64];               /* MAX_SLOT_NUM == 64 */

}NVME_CFG_EX, *PNVME_CFG_EX;


#define MAX_BM_IOSQ         (8)
#define MAX_BM_ADMINSQ      (1)

/* Priority Macro Definition */
#define PRIORITY_ADMIN      (1 << 0)
#define PRIORITY_URGENT     (1 << 1)
#define PRIORITY_HIGH       (1 << 2)
#define PRIORITY_MEDIUM     (1 << 3)
#define PRIORITY_LOW        (1 << 4)
#define PRIORITY_DEFAULT    PRIORITY_URGENT

#endif //AF_ENABLE

#ifdef __cplusplus
}
#endif /* _HAL_NVMECFGEX_H_ */
