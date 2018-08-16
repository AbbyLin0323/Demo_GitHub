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
Filename     :                                           
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20130715     blakezhang     001 first create
*****************************************************************************/
#ifndef _L1_RD_PREFETCH_H
#define _L1_RD_PREFETCH_H

#include "L1_SubCmdProc.h"

#define L1_RD_PREFETCH_THS (2)

#ifdef HOST_SATA
#define L1_PREFETCH_LCT_NUM ((BUF_SIZE_BITS <= 15) ? 20 : 8)
#else
#define L1_PREFETCH_LCT_NUM (8)
#endif

#define L1_IDLE_REFETCH_ID (0xA5)

// definitions for the prefetch result
#define L1_PREFETCH_INVALID_LCT 0
#define L1_PREFETCH_LCT_IN_CACHE 1
#define L1_PREFETCH_NO_WRITE_BUF 2
#define L1_PREFETCH_REQ_SENT 3

// external function declarations
extern void L1_RdPreFetchResetThs(void);
extern void L1_RdPreFetchInit(void);
extern void L1_RdPreFetchWarmInit(void);
extern void L1_PrefetchTrendCheck(SUBCMD *pSubCmd);
extern BOOL L1_ReadPreFetchCheckAllBufBusy(U16 usPhyBufID);
extern BOOL L1_PrefetchPrerequisiteCheck(void);
extern void L1_Prefetch(void);
extern void L1_TaskPrefetch(void);
extern void L1_PrefetchMarkProcessedScmd(SCMD* pScmd);


#endif

/********************** FILE END ***************/


