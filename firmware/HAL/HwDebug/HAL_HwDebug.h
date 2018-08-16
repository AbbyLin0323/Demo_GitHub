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
Filename    : HAL_HwDebug.h
Version     : Ver 1.0
Author      : Gavin
Date        : 20140728
Description :
    header file for HAL_HwDebug.c.
    this file define format of HW debug information by command tag.
Others      :
Modify      :
20140728     gavinyin     001 create file
20140911     gavinyin     002 modify it to meet coding style
*******************************************************************************/
#ifndef __HAL_HWDEBUG_H__
#define __HAL_HWDEBUG_H__
#include "HAL_MemoryMap.h"
#include "HAL_HCT.h"
#include "HAL_HSG.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_SGE.h"
#include "HAL_HostInterface.h"

#define SGE_DESC_GROUP_CNT_MAX 2

#define TRACE_FCQ_WBQ_MAX_CNT 8
#define TRACE_SGE_ENTRY_MAX_CNT 64

typedef enum _HAL_ENTRY_TYPE
{
    RCD_FCQ = 0,
    RCD_WBQ,
    RCD_DRQ,
    RCD_DWQ,
    RCD_SGQ_R,
    RCD_SGQ_W,
    RCD_SGE_CHAIN,
    RCD_NFC_CHAIN
}HAL_ENTRY_TYPE;

//descriptor of HCT debug information
typedef struct _HCT_DESC
{
    HAL_ENTRY_TYPE eEntryType: 16;
    U32 usEntryId: 16;

    U32 ulFcqWbqEntryOffset;
}HCT_DESC;

typedef struct _TRACE_HCT_INFO
{
    U32 ulHctDescIndex; // index for save next HCT descriptor
    U32 ulFcqWbqSaveOffset; // offset for back-up next FCQ/WBQ
    HCT_DESC aHctDesc[TRACE_FCQ_WBQ_MAX_CNT];
    U8 ucFcqWbqBuff[TRACE_FCQ_WBQ_MAX_CNT * sizeof(HCT_FCQ_WBQ)];//memory for FCQ/WBQ's back-up
}TRACE_HCT_INFO;

//descriptor of SGE debug information
typedef struct _SGE_DESC
{
    HAL_ENTRY_TYPE eEntryType: 16;
    U32 usEntryId: 16;

    SGE_ENTRY tSgeEntry;

    U32 ulHsgEntryOffset;
    union
    {
        U32 ulDsgEntryOffset;//for DRQ/DWQ
        U32 ulNfcqEntryOffset;//for SGQ
    };
}SGE_DESC;

typedef struct _TRACE_SGE_INFO
{
    U32 ulSgeDescIndex; // index for save next SGE descriptor
    U32 ulEntrySaveOffset; // offset for back-up next SGE entry
    BOOL bAllSgeChainBuilt; // FW finish building SGE chain or not
    BOOL bAllNfcChainBuilt; // FW finish building NFC on-the-fly program chain or not
    U16  usSgeTotalChain;  // total number of SGE chain
    U16  usNfcTotalChain;  // total number of NFC on-the-fly program chain
    SGE_DESC aSgeDesc[TRACE_SGE_ENTRY_MAX_CNT];
    U8 ucEntryBuff[TRACE_SGE_ENTRY_MAX_CNT * sizeof(NFCQ_ENTRY) * 2];//memory for HSG/DSG/NFCQ entry's back-up
}TRACE_SGE_INFO;

//record and maintain HCT/SGE entries which were built by FW
typedef struct _TRACE_TAG_INFO
{
    TRACE_HCT_INFO tTraceHctInfo;
    TRACE_SGE_INFO aTraceSgeInfo[SGE_DESC_GROUP_CNT_MAX];
}TRACE_TAG_INFO;

typedef struct _HW_DEBUG_INFO
{
    TRACE_TAG_INFO aTraceTagInfo[MAX_SLOT_NUM];
}HW_DEBUG_INFO;

extern MCU12_VAR_ATTR NFCQ_ENTRY *g_pNfcqForHalDebug;
void HAL_HwDebugInit(void);
void HAL_HwDebugStart(U8 ucTag);
void HAL_HwDebugTrace(U8 ucTag, U16 usTraceType, void *pEntry, U16 usEntryId, NFCQ_ENTRY *pNfcqEntry);

#endif//__HAL_HWDEBUG_H__

