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


#ifndef __L0_TRIM_PROCESS_H__
#define __L0_TRIM_PROCESS_H__
#include "BaseDef.h"

/* The data structure describing a LBA range entry within the data request inside a DATASET_MANAGEMENT command. */
typedef struct _LBA_RANGE_ENTRY_
{
#ifdef HOST_NVME
    U32 ulCntxAttr;
    U32 bsRgLen;
    U32 ulLBALow;
    U32 bsLBAHigh;
#else //SATA or AHCI
    U32 ulLBALow;
    U32 bsLBAHigh: 16;
    U32 bsRgLen: 16;
#endif
} LBA_ENTRY, *PLBA_ENTRY;

typedef struct _LBA_LONG_ENTRY_
{
    U32 ulStartLBA;
    U32 ulRegionLen;
} LBA_LONGENTRY, *PLBA_LONGENTRY;

typedef struct _Subsys_Range_Param_
{
    U32 ulStartLaneNum;
    U32 ulStartOffsetInLCT;
    U32 ulSecLen;
} SUBSYSRANGE_TABLE, *PSUBSYSRANGE_TABLE;

U32 L0_TrimProcLBAEntry(U32 ulStartEntry, U32 ulMaxEntry, PLBA_LONGENTRY pSegLBA);
void L0_TrimProcSegment(const LBA_LONGENTRY *pSegLBA, PLBA_LONGENTRY pSubsysLBA);

#endif //__L0_TRIM_PROCESS_H__
