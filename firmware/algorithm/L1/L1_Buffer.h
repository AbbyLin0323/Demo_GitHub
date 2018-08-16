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
Filename     : L1_Buffer.h
Version       :  Ver 1.0                                               
Date           :                                         
Author        :  BlakeZhang

Description: 

Modification History:
20120118  BlakeZhang    01 first create
*****************************************************************************/
#ifndef _L1_BUFFER_H
#define _L1_BUFFER_H

#include "L1_RdPreFetch.h"

/* buffer define  */        
/* If buffer size is 64K, L1 write and RFD buffer count should reduce by half, because DRAM size is limited*/

//#ifdef IM_3D_TLC_1TB
#ifdef IM_3D_TLC_512GB
#define L1_WRITE_CACHE_BUFFER_PER_PU    (((BUF_SIZE_BITS <= 15) ? 4 : 2) * LUN_NUM_PER_SUPERPU)
#define L1_LAST_RECYCLE_BUFFER_PER_PU   (((BUF_SIZE_BITS <= 15) ? 4 : 2) * LUN_NUM_PER_SUPERPU)
#else
#define L1_WRITE_CACHE_BUFFER_PER_PU    (((BUF_SIZE_BITS <= 15) ? 8 : 4) * LUN_NUM_PER_SUPERPU)
#define L1_LAST_RECYCLE_BUFFER_PER_PU   (((BUF_SIZE_BITS <= 15) ? 8 : 4) * LUN_NUM_PER_SUPERPU)
#endif
	
#define L1_WRITE_BUFFER_PER_PU          (L1_WRITE_CACHE_BUFFER_PER_PU + L1_LAST_RECYCLE_BUFFER_PER_PU)
#define L1_BUFFER_COUNT_WRITE           (SUBSYSTEM_SUPERPU_NUM * L1_WRITE_BUFFER_PER_PU)
#define L1_WRITE_CACHE_BUFFER           (SUBSYSTEM_SUPERPU_NUM * L1_WRITE_CACHE_BUFFER_PER_PU)
#ifdef IM_3D_TLC_512GB
#define L1_NONBLOCKING_BUFFER           (1)
#else
#define L1_NONBLOCKING_BUFFER           (L1_WRITE_CACHE_BUFFER >> 2)
#endif
#define L1_NONBLOCKING_BUFFER_SEC       (L1_NONBLOCKING_BUFFER * SEC_PER_BUF)

#ifdef HOST_READ_FROM_DRAM
#define L1_READ_FROM_DRAM_BUF_PER_PU    ((BUF_SIZE_BITS <= 15) ? 8 : 4)
#define L1_BUFFER_COUNT_READ_FROM_DRAM  (SUBSYSTEM_SUPERPU_NUM * L1_READ_FROM_DRAM_BUF_PER_PU)
#define L1_BUFFER_TOTAL                 (L1_BUFFER_COUNT_WRITE + L1_BUFFER_COUNT_READ_FROM_DRAM)
#else
#define L1_BUFFER_TOTAL                 (L1_BUFFER_COUNT_WRITE)
#endif

#if defined(HOST_SATA)
#define L1_BUFFER_COUNT_READ           (64)
#else
// read buffer for nvme mode
#define L1_BUFFER_COUNT_READ           (16)

#endif

/* get physical buffer id from logic bufferid - for all read and write buffer*/
#define  PHYBUFID_FROM_LGCBUFID(logicbufID)   (g_ulBufferStartPhyId + (logicbufID))
#define  LGCBUFID_FROM_PHYBUFID(phybufID)     ((phybufID) - g_ulBufferStartPhyId)

/*Buffer stage define*/
/* blakezhang: combine HOST_PENDING and MERGE_PENDING to CACHE_PENDING in CacheStatus Management */
typedef enum _BUFFER_STAGE_
{
    BUF_STAGE_FREE = 0,
    BUF_STAGE_CACHE,
    BUF_STAGE_FLUSH,
    BUF_STAGE_RECYCLE
}BUFFER_STAGE;

typedef enum _BUF_LINK_TYPE
{
    FREE_CACHE_LINK = 0,
    MERGE_PENDING_LINK,
    FLUSH_PENDING_LINK,
    FLUSH_ONGOING_LINK,
    READ_CACHE_LINK
} BUF_LINK_TYPE;

typedef struct _BUFF_TAG
{
    /* DWORD 0 */
    union
    {
        struct
        {
            U16 Stage : 5;          // state machine
            U16 bSeqBuf : 1;        // bSeqBuf = 1 means a SeqW SubCmd
            U16 bPrefetchBuf : 1;   // flag indicating if this is a prefetch buffer
            U16 bIsCertainHit : 1;
            U16 ucWritePointer : 8; // cache write pointer
        };
        U16 usDW0L;
    };
    U16 usNextBufId;

    /* DWORD 1 - 2 for cache status management */
    U16 usStartHRID;    /* Host Read DRAM */
    U16 usStartHWID;    /* Host Write DRAM */
    U16 usStartNRID;    /* NFC Read FLASH */
    U16 usStartNWID;    /* NFC Write FLASH */

#ifdef L1_BUFFER_REGULATION
    // DWORD 3 valid LPN bitmap
    U32 ulValidLpnBitmap;
#endif
}BUFF_TAG;

typedef struct _BUF_TAG_ENTRY
{
    // BUFF_TAG BufTag[L1_BUFFER_TOTAL];
    BUFF_TAG *BufTag;
}BUF_TAG_ENTRY;

typedef struct _WRITE_BUF_FIFO_ENTRY
{
    U16 usPhyBufID[SUBSYSTEM_SUPERPU_MAX][L1_WRITE_BUFFER_PER_PU + 1];
}WRITE_BUF_FIFO_ENTRY;

typedef struct _WRITE_BUF_FIFO_INFO
{
    U16 usFreeCacheStartBufId;
    U16 usMergePendingStartBufId;

    U16 usFlushPendingStartBufId;
    U16 usFlushOngoingStartBufId;

    U16 usReadCacheStartBufId;
    U16 usFreeCacheEndBufId;

    U16 usMergePendingEndBufId;
    U16 usFlushPendingEndBufId;
    
    U16 usFlushOngoingEndBufId;
    U16 usReadCacheEndBufId;

    U8 ucFreeCacheBufCnt;
    U8 ucMergePendingBufCnt;
    U8 ucFlushPendingBufCnt;
    U8 ucFlushOngoingBufCnt;
    
    U8 ucReadCacheBufCnt;
    U8 ucActiveWriteCacheCnt;
    U8 Rsvd1[2];
}WRITE_BUF_FIFO_INFO;

typedef struct _WRITE_BUF_FIFO_INFO_ENTRY{
    WRITE_BUF_FIFO_INFO tWriteBufInfo[SUBSYSTEM_SUPERPU_MAX];
}WRITE_BUF_FIFO_INFO_ENTRY;

typedef struct _BUF_SECTOR_BITMAP
{
    //U8 ucBitMap[L1_BUFFER_TOTAL][LPN_PER_BUF];
    U8 (*ucBitMap)[LPN_PER_BUF];
} BUF_SECTOR_BITMAP;

#ifdef HOST_READ_FROM_DRAM
typedef struct _READ_FROM_DRAM_BUFFER
{
  /* allocate RFD buffer Tail++
        finish read from FLASH Head++
        finish read to host Recycle++
    */
  U8  ucHead;
  U8  ucTail;
  U8  ucRecycle;
  U8  ucUsedCnt;
  
  U16 aRFDPhyBufID[L1_READ_FROM_DRAM_BUF_PER_PU];

  SCMD tRFDSCMD[L1_READ_FROM_DRAM_BUF_PER_PU];
  SUBCMD tRFDSubCmd[L1_READ_FROM_DRAM_BUF_PER_PU];
} RFD_BUFFER;

typedef struct _READ_FROM_DRAM_BUF_ENTRY
{
  RFD_BUFFER  *tRFDBuffer;
} RFD_BUF_ENTRY;

extern GLOBAL MCU12_VAR_ATTR U32 g_ulRFDFullBitmap;
#endif

extern GLOBAL  U32 g_ulFreeCacheAvailableBitmap;
extern GLOBAL  U32 g_ulActiveWriteBufferBitmap;
extern GLOBAL  U32 g_ulMergeBufferBitmap;
extern GLOBAL  U32 g_ulFlushBufferBitmap;
extern GLOBAL  U8 g_ucCurMergePos[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL  U32 g_ulBufferLpnBaseAddr;

extern GLOBAL  BUFF_TAG *gpBufTag;
extern GLOBAL  U32 g_ulBufferStartPhyId;
extern GLOBAL  U32 g_ulReadBufStartPhyId;
extern GLOBAL  U32 g_ulWriteBufStartPhyId;
extern GLOBAL  U32 g_L1InvalidBufferPhyId;
extern GLOBAL  U32 g_L1TempBufferPhyId;

extern GLOBAL  WRITE_BUF_FIFO_ENTRY *gpWriteBufFifoEntry;
extern GLOBAL  WRITE_BUF_FIFO_INFO g_WriteBufInfo[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL  BUF_SECTOR_BITMAP  g_BufSectorBitMap;
extern GLOBAL  BUF_SECTOR_BITMAP *gpBufSectorBitMap;

#define L1_SetBufferSectorBitmap(LocBufID,CacheOffset,Data)  (gpBufSectorBitMap->ucBitMap[(LocBufID)][(CacheOffset)] = (Data))
#define L1_GetBufferSectorBitmap(LocBufID,CacheOffset)  (gpBufSectorBitMap->ucBitMap[(LocBufID)][(CacheOffset)])

extern void L1_BufferSram0Map(U32 *pFreeSramBase);
extern void L1_BufferSram1Map(U32 *pFreeSramBase);
extern void L1_BufferDramMap(U32 *pFreeDramBase);
extern void L1_BufferInit(void);
extern void L1_BufferWarmInit(void);
extern void L1_SetBufferLPN(U16 LogicBufferID, U8 CacheOffset, U32 LPN);
extern U32 L1_GetBufferLPN(U16 LogicBufferID, U8 CacheOffset);
extern U16 L1_AllocateReadBuf(SUBCMD *pSubCmd);
extern U16* L1_GetReadBufferEntryCachestatus(U16 usReadBufferEntry);
extern U16 L1_AllocateWriteBuf(U8 PUNum);
extern U16 L1_GetLastLogBufId(U8 ucSuperPu);
extern BOOL L1_ReleaseWriteBuf(U8 PUNum, U16 usPhyBufID);
extern void L1_RecycleWriteBuf(U8 PUNum);
extern BOOL L1_BufferMergeLpn(U8 ucSuperPu, U16 usPhyBufID, U8 ucLpnOffset);
extern BOOL L1_BufferReadLpn(U8 ucSuperPu, U16 usPhyBufID, U8 ucLpnOffset, U32 ulLpn, U16* pusCachestatus);
extern BOOL L1_MergeWriteBuf(U8 PUNum);
extern BOOL L1_FlushWriteBuf(U8 PUNum);
extern U8 L1_BufferGetWritePointer(U16 usLogicBufID);
extern void L1_BufferSetWritePointer(U16 usLogicBufID, U8 ucWritePointer);
extern BOOL L1_HostPartialHitCheckBusy(U16 usLogBufId, U16 usCacheOffset);
extern BOOL L1_HostFullHitWriteCheckBusy(U16 usLogBufId, U16 usSecOffInBuf, U16 usSecLen);
extern BOOL L1_HostFullHitReadCheckBusy(U16 usLogicBufID, U16 usSecOffInBuf, U16 usSecLen);
extern void L1_AddBufToLinkTail(U8 ucPuNum, BUF_LINK_TYPE LinkType, U16 usTargetLogBufId);
extern U16 L1_RemoveBufFromLinkHead(U8 ucPuNum, BUF_LINK_TYPE LinkType);
extern U16 L1_GetBufFromLinkHead(U8 ucPuNum, BUF_LINK_TYPE LinkType);
extern BOOL L1_FlushPendingBufferRegulation(U8 ucPuNum);

#ifdef HOST_READ_FROM_DRAM
extern GLOBAL  U32 g_ulRFDBufStartPhyId;
extern void L1_RFDBufferReset(void);
extern BOOL L1_AllocateReadFromDRAMBuf(SUBCMD *pSubCmd);
extern BOOL L1_ResumeRFDBuf(U8 ucPuNum);
extern void L1_RecycleRFDBuf(U8 ucPuNum);
extern BOOL L1_IsRFDBufferEmpty(U8 ucPuNum);
#endif

#endif

/********************** FILE END ***************/

