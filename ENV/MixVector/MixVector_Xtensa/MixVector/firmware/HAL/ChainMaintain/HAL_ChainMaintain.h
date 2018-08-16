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
Filename    : HAL_ChainMaintain.h
Version     : Ver 1.0
Author      : Gavin
Date        : 20140611
Description : this file declare interface for maintain host memory pointer and 
              SGE chain
Others      :
Modify      :
20140611     gavinyin     001 create file
20140905     gavinyin     002 modify to meet coding stytle
*******************************************************************************/
#ifndef _HAL_CHAIN_MAINTAIN_H__
#define _HAL_CHAIN_MAINTAIN_H__

#include "BaseDef.h"

/* Chain number descriptor for every host command tag */
typedef struct _CHAIN_NUM_MGR
{
    U32 bsTotalSecCntValid: 1;// 1 means all request received
    U32 bsStartProcess: 1;// use this bit for debug
    U32 bsDw0Rsvd: 14;// reserved
    U32 usTotalSecCnt: 16;// record total received request length

    U32 usFinishSecCnt: 16;// record finish request length
    U32 usFinishChainCnt: 16;// record finish chain cnt
}CHAIN_NUM_MGR;

/* host memory discriptor, define information for building HSG */
typedef struct _HMEM_DPTR
{
    U32 bsCmdTag: 6;// support max 64 for NVMe mode.
    U32 bsPrdOrPrpIndex: 6;// support max 64 PRD/PRP entry in a command
    U32 bsLastSecRemain: 9;// last sector remain byte 
    U32 bsDw0Rsv: 11;// reserved
    
    U32 bsOffset: 22;//byte offset in PRD entry(AHCI mode). This field is not used in NVMe mode
    U32 bsDw1Rsv: 10;// reserved

    U32 ulLBA;// sector address
}HMEM_DPTR;

//chain number manager interface
void HAL_ChainMaintainInit(void);
void HAL_L1AddHostReqLength(U8 ucHID, BOOL bFirst, BOOL bLast, U16 usSecCnt);
void HAL_AddFinishReqLength(U8 ucHID, U16 usSecCnt);
U32 HAL_GetLocalPrdEntryAddr(U8 ucCmdTag, U16 usPrdIndex); // AHCI mode only
U32 HAL_GetLocalPrpEntryAddr(U8 ucCmdTag, U16 usPrpIndex); // NVMe mode only
BOOL HAL_HostAddrMeetHwLimit(HMEM_DPTR *pHmemDptr, U32 ulByteLen);
void HAL_BuildHsg(HMEM_DPTR *pHmemDptr, U32 * pByteLen, U16 usCurHsgId, U16 usNextHsgId);
BOOL HAL_CheckForLastHsg(HMEM_DPTR *pHmemDptr, U32 ulByteLenRemain);

#endif/* _HAL_CHAIN_MAINTAIN_H__ */
