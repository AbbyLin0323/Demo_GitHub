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

Filename     :   AHCI_HostModel.h
Version      :   0.1
Date         :   2013.08.26
Author       :   bettywu

Description:  the interface to other model
Others:
Modification History:
20130826 create

*******************************************************************/

#ifndef _H_NVME_CONTROLLERMODEL
#define _H_NVME_CONTROLLERMODEL

#include <windows.h>
#include "base_pattern.h"

#define KB                          (1024)
#define DATA_BUFFER_PER_CMD         (128 * KB)

//Function Macro Definition.For Host never Reaq CQe issue
#define AVOID_CQ_HUNGRY             0

//Basic Queue Macro Definition
#define QUEUE_EMPTY(HEAD, TAIL)             ((HEAD) == (TAIL))
#define QUEUE_NOT_EMPTY(HEAD, TAIL)         (!QUEUE_EMPTY((HEAD), (TAIL)))
#define QUEUE_FULL(HEAD, TAIL, SIZE)        ((HEAD) == ((TAIL + 1) % (SIZE)))
#define QUEUE_NOT_FULL(HEAD, TAIL, SIZE)    (!(QUEUE_FULL(HEAD, TAIL, SIZE)))

//CQ,SQ Size Macro Definition
#define SQ_SIZE_M(SQID)                   (g_pNVMeCfgExReg->SQCfgAttr[(SQID)].Size+1)
#define CQ_SIZE_M(CQID)                   (((CQID) > 0 )?(g_pNVMeCfgReg->cq_size[(CQID-1)]):(g_pNVMeCfgReg->cq0_size))

//CQ,SQ Pointer Macro Definition
#define SQ_HEAD_M(SQID)                   (g_pNVMeCfgReg->sq_ptr[(SQID)].head)
#define SQ_HWRP_M(SQID)                   (g_pNVMeCfgExReg->SQCfgAttr[(SQID)].HWRP)
#define SQ_TAIL_M(SQID)                   (g_pNVMeCfgReg->doorbell[(SQID)].sq_tail)
#define CQ_HEAD_M(CQID)                   (g_pNVMeCfgReg->doorbell[(CQID)].cq_head)
#if (AVOID_CQ_HUNGRY) 
#define CQ_TAIL_M(CQID)                   (g_NVMeCfgExLocInfo.CqPreTail[(CQID)])
#else
#define CQ_TAIL_M(CQID)                   (g_pNVMeCfgReg->cq_info[(CQID)].tail_entry_ptr)
#endif //

//CQ,SQ Function Macro Definition
#define PUSH_SQ_HWRP_M(SQID)              (SQ_HWRP_M(SQID) = (SQ_HWRP_M(SQID)+1) % SQ_SIZE_M(SQID))
#define PUSH_CQ_TAIL_M(CQID)              (CQ_TAIL_M(CQID) = (CQ_TAIL_M(CQID)+1) % CQ_SIZE_M(CQID))
#define SQ_EMPTY_M(SQID)                  (QUEUE_EMPTY(SQ_HWRP_M(SQID), SQ_TAIL_M(SQID)))
#define SQ_NOT_EMPTY_M(SQID)              (!SQ_EMPTY_M(SQID))
#define SQ_FULL_M(SQID)                   (QUEUE_FULL(SQ_HWRP_M(SQID), SQ_TAIL_M(SQID), SQ_SIZE_M(SQID)))
#define SQ_NOT_FULL_M(SQID)               (!SQ_FULL_M(SQID))

#define CQ_EMPTY_M(CQID)                  (QUEUE_EMPTY(CQ_HEAD_M(CQID), CQ_TAIL_M(CQID)))
#define CQ_NOT_EMPTY_M(CQID)              (!CQ_EMPTY_M(CQID))
#define CQ_FULL_M(CQID)                   (QUEUE_FULL(CQ_HEAD_M(CQID), CQ_TAIL_M(CQID), CQ_SIZE_M(CQID)))
#define CQ_NOT_FULL_M(CQID)               (!CQ_FULL_M(CQID))


//SQ Host Address Macro Definition
#define SQ_HOST_BAL_M(SQID)               (g_pNVMeCfgExReg->SQCfgAttr[(SQID)].BaseAddrL)
#define SQ_HOST_BAH_M(SQID)               (g_pNVMeCfgExReg->SQCfgAttr[(SQID)].BaseAddrH)


typedef struct _hct_mgr{
    CRITICAL_SECTION    WBQCriticalSection;
    HANDLE              EventTable[ 4 ];    // 0: for command state trigger event
                                            // 1: for data transfer done event.
                                            // 2: for thread exit event.
    U8                  ActiveWBQBitmap[MAX_SLOT_NUM];    // each bit indicates the corresponding command has been in WBQ trigger state.
    U8                  DataDoneBitmap[MAX_SLOT_NUM];     // each bit indicates the corresponding command has completed data transfer.
    U8                  bsWBQOffset[ MAX_SLOT_NUM ];

}HCT_MGR, *PHCT_MGR;


void NVME_ControllerModelInit();
void NVME_ControllerModelSchedule();
BOOL NVME_HandleCmdWBQ(U8 CmdTag);

void Host_ReadFromDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag);
void Host_WriteToDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 BufferAddr, U32 ByteLen, U8 nTag);
void NVME_HostPrepareData(U32 StartLBA,U32 SecCnt,U32* pDataBuffer);
void NVME_HostCmdTableInit();
void NVME_HostResetTrimData(TRIM_CMD_ENTRY *pTrimBlockEntry, U8 ucBlockCnt);
#endif
