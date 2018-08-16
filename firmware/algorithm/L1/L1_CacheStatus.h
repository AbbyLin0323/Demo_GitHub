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
Filename     :  L1_CacheStatus.h
Version       :  Ver 1.0                                               
Date           :                                         
Author        :  blakezhang

Description: 

Modification History:
20120118  blakezhang    01 first create
****************************************************************************/
#ifndef _L1_CACHESTATUS_H
#define _L1_CACHESTATUS_H

/* Buffer busy status */
#define BUF_BUSY  0
#define BUF_IDLE  1
#define BUF_FAIL  2

/* Total CacheStatus number */
#define L1_CACHESTATUS_TOTAL  (768)

#define L1_CS_WRITE_BUF_CHECK_CNT       (1)
#define L1_CS_PREFETCH_BUF_CHECK_CNT    (LPN_PER_BUF)

#if 0
typedef struct _CACHESTATUS_ENTRY
{
    U32 usNextID:16;
    U32 usSecLen:16;
    U32 ulStartLBA;
    U16 usRelatedCachestatusId;
    U16 usRsvd;
} CACHESTATUS_ENTRY;

typedef struct _CACHESTATUS_MANAGER
{
    CACHESTATUS_ENTRY CS_Entry[L1_CACHESTATUS_TOTAL];
} CACHESTATUS_MAN;
#else
typedef struct _CACHESTATUS_ID_ENTRY
{
    U16 usNextID;
    U16 usRelatedCachestatusId;
} CACHESTATUS_ID_ENTRY;

typedef struct _CACHESTATUS_RANGE_ENTRY
{
    U16 usSecOffInBuf;
    U16 usSecLen;
} CACHESTATUS_RANGE_ENTRY;
#endif

typedef struct _CACHESTATUS_FIFO
{
    U16 usCacheStatusID[L1_CACHESTATUS_TOTAL + 1];
    U16 usFifoTail;
    U16 usFifoHead;
} CACHESTATUS_FIFO;

extern GLOBAL  U8* g_pCacheStatus;
#if 0
extern GLOBAL  CACHESTATUS_MAN  *g_pCSManager;
#else
extern GLOBAL  CACHESTATUS_ID_ENTRY *g_CS_ID_Entry;
extern GLOBAL  CACHESTATUS_RANGE_ENTRY *g_CS_Range_Entry;
#endif
extern GLOBAL  CACHESTATUS_FIFO *g_pCSFifoEntry;

extern void L1_CacheStatusSram0Map(U32 *pFreeSramBase);
extern void L1_CacheStatusSram1Map(U32 *pFreeSramBase);
extern void L1_CacheStatusDRAMMap(U32 *pFreeSramBase);
extern void L1_CacheStatusOTFBMap(void);
extern void L1_CacheStatusInit(void);
extern void L1_CacheStatusWarmInit(void);
extern void L1_CacheStatusClearHostPending(void);
extern void L1_ClearCacheStatusLink(U16 *pStartID);
extern U8 L1_GetRawCacheStatus(void);
extern U32 L1_GetRawCacheStatusAddr(void);
extern void L1_SetRawCacheStatus(U8 ucStatus);
extern BOOL L1_CacheStatusFifoFull(void);
extern U32 L1_GetCacheStatusAddr(U16 usCStatusID);
extern U16 L1_GetNextCacheStatusID(U16 usCStatusID);
extern void L1_SetNextCacheStatusID(U16 usCurrStatusID, U16 usNextStatusID);
extern void L1_UpdateCacheStatusRangeInfo(U16 usCStatusID, U16 usSecOffset, U16 usSecLen);
extern U16 L1_AllocateCacheStatus(void);
extern void L1_ReleaseCacheStatus(U16 usCStatusID);
extern U32 L1_AddCacheStatusToLink(U16 *pStartID, U16 usSecOffset, U16 usSecLen);
extern U32 L1_AddRelatedCachestatusNodes(U16* pusStartCachestatusId1, U16 usStartSectorOffset1, U16 usSectorLength1, U16* pusStartCachestatusId2, U16 usStartSectorOffset2, U16 usSectorLength2);
extern BOOL L1_CheckCacheStatus(U16 *pStartID, U16 ulSecOffset, U16 usSecLen);
extern U8 L1_CheckPrefetchBufferCacheStatus(U16* pStartCachestatusId);
extern BOOL L1_CheckCacheStatusLinkIdle(U16 *pStartID);
extern void L1_RecycleCacheStatusID(void);
extern BOOL L1_CacheStatusRescourseCheck(U16 ucAllocateCnt);
extern void L1_AddPrefetchCacheStatusToLink(U16 usPhyBufID);
extern U32 L1_L2GetPrefetchCacheStatusAddr(U16 usPhyBufID, U16 usSecOffset, U16 usSecLen);
extern void L1_L2SubPrefetchCacheStatus(U16 usPhyBufID);
extern BOOL L1_CacheStatusWaitIdle(void);

#endif

/********************** FILE END ***************/

