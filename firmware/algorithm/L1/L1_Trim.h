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
Filename     : L1_Trim.h
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20130715     blakezhang     001 first create
*****************************************************************************/
#ifndef _L1_TRIM_H
#define _L1_TRIM_H

#define L1_TRIM_BULK_PROCESS_MINIMUM_LCT_COUNT 10

extern GLOBAL  U32 g_ulTrimStartLBA;
extern GLOBAL  U32 g_ulTrimEndLBA;
extern GLOBAL  U32 g_ulTrimReadLBA;
extern GLOBAL  U32 g_ulTrimReadCnt;
#ifndef LCT_TRIM_REMOVED
extern GLOBAL  U32 g_ulPendingTrimLctCount;
#endif

extern U8 L1_TrimProcessInit(U32 ulStartLBA, U32 ulEndLBA);
extern U32 L1_TrimPrerequisiteCheck(void);
extern BOOL L1_TrimProcessNon4KAlignedRange(void);
extern BOOL L1_TrimProcess4KAlignedRange(void);
extern BOOL L1_TrimProcessLctAlignedRange(void);
#endif

/********************** FILE END ***************/


