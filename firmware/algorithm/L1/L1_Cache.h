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
Filename    : L1_Cache.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.20    18:28:48
Description :
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/
#ifndef _L1_CACHE_H
#define _L1_CACHE_H

#include "L1_Buffer.h"

#ifdef HOST_SATA
/* SATA mode: LCT count of flush trim when receive, 2GB per each SubSystem */
#define L1_NON_BUFF_TRIM_THS    ((BUF_SIZE_BITS <= 15) ? (0x10000) : (0x8000))
#else
/* NVMe mode: Since the Trim command is sent in IOQ, 
host will send multiple Trim and R/W at the same time to deivce,
non-buffered Trim will be more risky in NVMe mode.
It should be fine tune to decided the non-buffered range.
Currently, only non-buffer 64MB range for each SubSystem. */
#define L1_NON_BUFF_TRIM_THS    ((BUF_SIZE_BITS <= 15) ? (0x800) : (0x200))
#endif

/*
StartBit and EndBit are from [0, 7], 0 is the least significant sector of a LPN, 
7 is the most significant sector of a LPN. for example:

ucSubCmdOffset = 3, ucSubCmdlength = 3 that means Bitmap 0b(00111000)
the sector from least to most significant are:  0-0-0-1-1-1-0-0
                                                              |                     |
                                                              |                     |
                                                             bit0                 bit7
StartBit = 3  ====> ucSubCmdOffset
EndBit   = 5  ====> ucSubCmdOffset + ucSubCmdlength -1
*/
#define SET_LPN_SECTOR_BIT_MAP(StartBit, EndBit)\
              ((0xFF >> (7 - (EndBit - StartBit))) << StartBit)

#define L1_CACHE_SE_NO_HIT                0   //no hit
#define L1_CACHE_SE_PART_HIT              1   //partial hit
#define L1_CACHE_SE_FULL_HIT              2   //full hit
#define L1_CACHE_SE_FULL_HIT_WAIT         3   //wait for buffer ready
#define L1_CACHE_SE_HIT_RD_PREFETCH       4   //hit read prefetch buffer
#define L1_CACHE_SE_RD_INVALID            5   //read invalid LCT
#define L1_CACHE_SE_RD_BITMAP_PART_HIT    6   //read partial hit, only bitmap not match
#define L1_CACHE_SE_WT_FULL_HIT_RECYCLE   7   //write full hit Recycle or flush stage buffer

#define L1_STATUS_CACHE_OK        0
#define L1_STATUS_CACHE_NO_SPACE  1
#define L1_STATUS_CACHE_NO_BUFF   2

#define L1_CACHEID_NUM  (L1_BUFFER_COUNT_WRITE * LPN_PER_BUF)

#define LCT_ENTRY_DEPTH     ((MAX_LBA_IN_DISK >> SEC_PER_BUF_BITS) + 1)

#ifdef CACHE_LINK_MULTI_LCT
#define L1_CACHE_LINK_LCT_NUM      2
#define LCT_ENTRY_DEPTH_REDUCED  ((LCT_ENTRY_DEPTH % L1_CACHE_LINK_LCT_NUM) ? (LCT_ENTRY_DEPTH / L1_CACHE_LINK_LCT_NUM + 1) : (LCT_ENTRY_DEPTH / L1_CACHE_LINK_LCT_NUM))
#endif

#define LCT_ENTRY_DEPTH_MAX (((U32)MAX_LBA_IN_DISK_MAX >> SEC_PER_BUF_BITS) + 1)
#define LCT_TAG_DEPTH       ((LCT_ENTRY_DEPTH >> DWORD_BIT_SIZE_BITS) + 1)
#define LCT_TAG_DEPTH_MAX   ((LCT_ENTRY_DEPTH_MAX >> DWORD_BIT_SIZE_BITS) + 1)

#define LCTINDEX_FROM_LBA(LBA)    ( (LBA) >> SEC_PER_BUF_BITS )
#define LCTINDEX_FROM_LPN(LPN)    ( (LPN) >> LPN_PER_BUF_BITS )
#define LPN_FROM_LCTINDEX(Index)  ((Index) << LPN_PER_BUF_BITS)

#define LCTTAG_FROM_INDEX(Index)  ((Index) >> DWORD_BIT_SIZE_BITS)
#define LCTMSK_FROM_INDEX(Index)  ((Index) & DWORD_BIT_SIZE_MSK)

#define LOGBUFID_FROM_CACHEID(CacheId)  (g_ulWriteBufStartPhyId - g_ulBufferStartPhyId + ((CacheId)/LPN_PER_BUF))
#define CACHE_OFFSET_IN_BUF(CacheId)    ((CacheId) & LPN_PER_BUF_MSK)

#if 0
typedef struct _LCT
{
    // U16 StartCacheID[LCT_ENTRY_DEPTH];
    U16 *StartCacheID;
} LCT;

typedef struct _LCT_WRITECNT
{
    // U16 LCT_WriteCnt[LCT_ENTRY_DEPTH];
    U16 *LCT_WriteCnt;
} LCT_WRITECNT;

typedef struct _CACHE_LINK
{
    //U16 NextCacheID[L1_CACHEID_NUM];
    U16 *NextCacheID;
} CACHE_LINK;

typedef struct _LCT_VALID
{
    // U32 ulValid[LCT_TAG_DEPTH];
    U32 *ulValid;
} LCT_VALID;

typedef struct _LCT_TRIM
{
    //U32 ulTrim[LCT_TAG_DEPTH];
    U32 *ulTrim;
} LCT_TRIM;
#endif

typedef union _LCT_HIT_RESULT
{
    struct
    {
        U8  ucHitResult;
        U8  ucCacheOffset;
        U16 usLogBufID;
    };

    U32 AsU32;
} LCT_HIT_RESULT;

#ifndef LCT_VALID_REMOVED
#define VBMT_PAGE_SIZE_PER_PU           ( (U32)(sizeof(U32)*LCT_TAG_DEPTH) )
#define VBMT_PAGE_SIZE_PER_PU_MAX       ( (U32)(sizeof(U32)*LCT_TAG_DEPTH_MAX) )
#define VBMT_PAGE_COUNT_PER_PU          ((VBMT_PAGE_SIZE_PER_PU % BUF_SIZE) ? (VBMT_PAGE_SIZE_PER_PU/BUF_SIZE + 1) : (VBMT_PAGE_SIZE_PER_PU/BUF_SIZE))
#define VBMT_PAGE_COUNT_PER_PU_MAX      ((VBMT_PAGE_SIZE_PER_PU_MAX % BUF_SIZE) ? (VBMT_PAGE_SIZE_PER_PU_MAX/BUF_SIZE + 1) : (VBMT_PAGE_SIZE_PER_PU_MAX/BUF_SIZE))

#define VBMT_SUPERPAGE_SIZE_PER_SUPERPU         ( (U32)(sizeof(U32)*LCT_TAG_DEPTH) )
#define VBMT_SUPERPAGE_SIZE_PER_SUPERPU_MAX     ( (U32)(sizeof(U32)*LCT_TAG_DEPTH_MAX) )
#define VBMT_SUPERPAGE_COUNT_PER_SUPERPU        ((VBMT_SUPERPAGE_SIZE_PER_SUPERPU % SUPER_PAGE_SIZE) ? (VBMT_SUPERPAGE_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE + 1) : (VBMT_SUPERPAGE_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE))
#define VBMT_SUPERPAGE_COUNT_PER_SUPERPU_MAX    ((VBMT_SUPERPAGE_SIZE_PER_SUPERPU_MAX % SUPER_PAGE_SIZE) ? (VBMT_SUPERPAGE_SIZE_PER_SUPERPU_MAX/SUPER_PAGE_SIZE + 1) : (VBMT_SUPERPAGE_SIZE_PER_SUPERPU_MAX/SUPER_PAGE_SIZE))
#endif

extern GLOBAL  U16 g_aCacheLine[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  U32 g_ulCacheWriteTime[SUBSYSTEM_SUPERPU_MAX];

#ifndef LCT_TRIM_REMOVED
extern GLOBAL  U32* g_pulTrim;
#endif

#ifndef LCT_VALID_REMOVED
extern GLOBAL  U32* g_pulLCTValid;
#endif

#ifndef LCT_TRIM_REMOVED
extern GLOBAL  U32 g_ulTrimSearchRangeStartLct;
extern GLOBAL  U32 g_ulTrimSearchRangeEndLct;
#endif
extern GLOBAL  U32 g_ulCurTrimTargetLct;
extern GLOBAL  U32 g_ulCurTrimTargetLctLpnOffset;

extern GLOBAL  U16 *g_pStartCacheID;
extern GLOBAL  U16 *g_pNextCacheID;

extern void L1_CacheDramMap(U32 *pFreeDramBase);
extern void L1_CacheSram1Map(U32 *pFreeSramBase);
extern void L1_CacheInit(void);
extern void L1_CacheWarmInit(void);
extern BOOL L1_SubCmdGetLpnSectorMap(U8 ucSubCmdOffset, U8 ucSubCmdlength, U8 * pLpnSectorBitmap);
extern U32 L1_CacheIDLinkSearchLPN(U32 ulStartLpn, U8 ucLPNCount, BOOL bCheck);
//extern U8 L1_CacheSearchTag(SUBCMD *pSubCmd);
extern U8 L1_CacheGetTagStatus(U8 ucPuNum, U8 ucSubLPNCount);
extern void L1_CacheAddNewLpn(SUBCMD* pSubCmd, U16 usCacheLine);
extern void L1_ReleaseCacheLine(U8 ucPuNum);
extern void L1_CacheAttachBuffer(U8 ucPuNum, U16 usPhyBuffID);
extern U32 L1_ReleaseAllWriteCache(void);
extern U8 L1_GetSuperPuFromLCT(U32 ulLCT);
#ifndef LCT_VALID_REMOVED
extern void L1_SetLCTValid(U32 ulLCTIndex, BOOL bValid);
extern BOOL L1_GetLCTValid(U32 ulLCTIndex);
#endif
#ifndef LCT_TRIM_REMOVED
extern BOOL L1_UpdateLCTValid(U32 ulLCTIndex, SUBCMD* pSubCmd);
#endif
extern void L1_SetNextCacheId(U16 CurCacheID, U16 NextCacheID);
extern U16 L1_GetLCTStartCache(U32 ulLCTIndex);
extern U16 L1_GetNextCacheId(U16 CurCacheID);
extern U16 L1_CalcCacheID(U16 usLogBufID, U8 ucCacheOffset);
extern void L1_SetLCTStartCache(U32 ulLCTIndex, U16 usStartCache);
extern void L1_UpdateCacheIdLink(SUBCMD * pSubCmd);
extern void L1_AddNewCacheIdNode(U32 ulLpn, U16 usLogBufId, U8 ucCacheOffset);
extern void L1_CacheLinkRecycleBuf(U16 usLogBufID);
#ifndef LCT_TRIM_REMOVED
extern void L1_CacheResetLCTTag(void);
#ifndef LCT_VALID_REMOVED
extern void L1_RebuildLCTTag(void);
#endif
extern void L1_ResetLCTTrim(void);
extern void L1_SetLCTTrim(U32 ulLCTIndex, U8 bTrimed);
extern U8 L1_GetLCTTrim(U32 ulLCTIndex);
#endif
extern void L1_CacheInvalidLPNInLCT(U16 usLogBufID, U8 ucLPNOffset, U8 ucSubLPNCount);
extern BOOL L1_CacheInvalidTrimLCT(U32 trimStartLba, U32 trimEndLba);
extern void L1_CacheTrimAddNewLPN(U8 ucPuNum, U32 ulLPN, U8 ucLpnBitmap);
#ifndef LCT_TRIM_REMOVED
extern void L1_ProcessPendingTrimBulk(U32 ulMaxLCTCnt, U32 ulMaxSearchLctCount);
extern void L1_ProcessPendingTrim(void);
#endif
extern U32 L1_CacheCheckTimePass(U8 ucPuNum);

#endif

/********************** FILE END ***************/

