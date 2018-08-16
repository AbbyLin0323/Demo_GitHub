/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially.

Filename     :                                           
Version       :  Ver 1.0                                               
Date           :                                         
Author        :  PeterXiu

Description: 

Modification History:
20120118  peterxiu    01 first create
*************************************************/
#ifndef _L1_BUFFER_H
#define _L1_BUFFER_H

#include "L1_Cache.h"

/* buffer define  */
#define L1_BUFFER_COUNT_READ        (64)        /* buffer for read - each read buffer is mapped to a RPRD, so we have 64 Read buffers. */
#define L1_BUFFER_PER_PU            (8)

/* For 32G module,            PU_NUM = 4,  Write BuffCnt = 4*16  = 64,  Ramdisk size = 64*16KB  = 1MB
   For two-plane 256G module, PU_NUM = 16, Write BuffCnt = 16*16 = 256, Ramdisk size = 256*32KB = 8MB 
   For one-plane 256G module, PU_NUM = 32, Write BuffCnt = 32*16 = 512, Ramdisk size = 512*16KB = 8MB */
#define L1_BUFFER_COUNT_WRITE          (PU_NUM * L1_BUFFER_PER_PU)
#define L1_BUFFER_TOTAL             (L1_BUFFER_COUNT_READ + L1_BUFFER_COUNT_WRITE)

/* get physical buffer id from logic bufferid - for all read and write buffer*/
#define  PHYBUFID_FROM_LGCBUFID(logicbufID)  (g_ulBufferStartPhyId + (logicbufID))
#define  LGCBUFID_FROM_PHYBUFID(phybufID)  ((phybufID) - g_ulBufferStartPhyId)

/* SINGLE_LPN: find the LPN; NO_HIT: for no hit search, masked lower 2 or 3 bits */
#define L1_BUFF_SEARCHMASK_SINGLE_LPN (INVALID_8F)
#define L1_BUFF_SEARCHMASK_NO_HIT     (INVALID_8F << LPN_PER_BUF_BITS)

/*Buffer stage define*/
typedef enum _BUFFER_STAGE_
{
    BUF_STAGE_FREE = 0,
    BUF_STAGE_CACHED,
    BUF_STAGE_SATA_PENDING,
    BUF_STAGE_SATA_OK,
  BUF_STAGE_MERGE_PENDING,
  BUF_STAGE_MERGE_OK,
    BUF_STAGE_FLUSH_PENDING,
    BUF_STAGE_RECYCLE
}BUFFER_STAGE;


#define L1_BUF_TYPE_READ        0
#define L1_BUF_TYPE_WRITE       1

#define L1_BUF_POS_DRAM         0
#define L1_BUF_POS_OTFB         1

#define L1_BUF_SIZE_4K          0
#define L1_BUF_SIZE_8K          1
#define L1_BUF_SIZE_16K         2
#define L1_BUF_SIZE_32K         3

#define L1_BUF_SPLIT_SIZE_4K    0
#define L1_BUF_SPLIT_SIZE_8K    1
#define L1_BUF_SPLIT_SIZE_16K   2
#define L1_BUF_SPLIT_SIZE_32K   3

#if(SEC_PER_BUF_BITS == 6) //32K Buffer
#define L1_BUF_CURRENT_SZIE         L1_BUF_SIZE_32K
#define L1_BUF_CURRENT_SPLIT_SZIE     L1_BUF_SPLIT_SIZE_32K
#else   //16K Buffer
#define L1_BUF_CURRENT_SZIE         L1_BUF_SIZE_16K
#define L1_BUF_CURRENT_SPLIT_SZIE     L1_BUF_SPLIT_SIZE_16K
#endif

#define L1_BUF_HOST_REQ     0
#define L1_BUF_LOCAL_REQ    1

/* Buffer busy status */
#define BUF_BUSY  0
#define BUF_IDLE  1

/* DWORD size in Byte is 4 */
#define DWORD_SIZE_BITS   2
#define DWORD_SIZE        (1 << DWORD_SIZE_BITS)

typedef struct _BUFF_TAG
{
    /* one DWORD */
    U32 Stage:4;      /* state machine */
    U32 Type:4;       /* buffer type: Read or Write */
    U32 Position:2;   /* Dram or OTFB */
    U32 BufferSize:2; /* 4K, 8k, 16k or 32K */
    U32 SplitSize:2;  /* 4K, 16k or 32k */
    U32 LocalREQEnable:2;   /* merge read flag */
    U32 ucWritePointer:8;   /* cache write pointer */
    U32 bSeqBuf:1;     /* bSeqBuf = 1 means a SeqW SubCmd */
    U32 RefCount:7;     /* any where buff refered, RefCount++  */
}BUFF_TAG;

extern BUFF_TAG gBuffTag[L1_BUFFER_TOTAL];
extern U32 g_ulBufferStartPhyId;
extern U32 g_L1TempBufferAddr;
extern U32 g_L1TempBufferPhyId;
extern U32* g_pBufBusy;
extern U32* g_pFlashStatus;

extern U16 s_aWriteBufFifoEntry[PU_NUM][L1_BUFFER_PER_PU + 1];

extern U8 PUFifoHead[PU_NUM];
extern U8 PUFifoTail[PU_NUM];
extern U8 PUFifoMerge[PU_NUM];
extern U8 PUFifoFlush[PU_NUM];
extern U8 PUFifoRecycle[PU_NUM];


extern U8 gBufSectorBitMap[L1_BUFFER_TOTAL][LPN_PER_BUF];

#define L1_SetBufferSectorBitmap(LocBufID,LPNOffset,Data)  (gBufSectorBitMap[(LocBufID)][(LPNOffset)] = (Data))
#define L1_GetBufferSectorBitmap(LocBufID,LPNOffset)  (gBufSectorBitMap[(LocBufID)][(LPNOffset)])

extern void L1_BufferInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
extern void L1_SetBufferLPN(U16 LogicBufferID, U8 LPNOffset, U32 Data);
extern U32 L1_GetBufferLPN(U16 LogicBufferID, U8 LPNOffset);
#if 0
extern void L1_SetBufferSectorBitmap(U16 LogicBufferID, U8 LPNOffset, U8 Data);
extern U8 L1_GetBufferSectorBitmap(U16 LogicBufferID, U8 LPNOffset);
#endif
extern U16 L1_GetReadBuffId(U8 ucReadPrdID);
extern U16 L1_AllocateWriteBuf(U8 PUNum);
extern BOOL L1_ReleaseWriteBuf(U8 PUNum, U16 usPhyBufID);
extern void L1_RecycleWriteBuf(U8 PUNum);
extern BOOL L1_BufferMergeLpn(U16 usPhyBufID, U8 ucLpnOffset);
extern BOOL L1_MergeWriteBuf(U8 PUNum);
extern BOOL L1_FlushWriteBuf(U8 PUNum);
extern U8 L1_BufferSearchLPNOffset(U16 usPhyBufID, U32 ulLPN);
extern U16 L1_BufferSearchLPN(U8 PUNum, U32 ulLPN, U32 ulMask);
extern U8 L1_BufferGetWritePointer(U16 usLogicBufID);
extern void L1_BufferSetWritePointer(U16 usLogicBufID, U8 ucWritePointer);
extern BOOL L1_BufferCheckBusy(U16 usPhyBufID);
extern BOOL L1_HostReadCheckBusy(U16 usPhyBufID, U8 ucLPNOffset, U8 ucLPNCount);
extern BOOL L1_HostWriteCheckBusy(U16 usPhyBufID, U8 ucLPNOffset, U8 ucLPNCount);
extern U32 L1_GetNextCacheStsAddr(U16 usPhyBufID);
extern U32 L1_L3GetCacheStsAddr(U16 usPhyBufID);
extern void L1_AddBufferBusy(U16 usPhyBufID);
extern void L1_ResetBufferBusy(U16 usPhyBufID);
extern BOOL  L1_CheckBufferBusy(U16 usPhyBufID);
extern BOOL L1_CheckFlashStatus(U16 usPhyBufID, U8 ucLPNOffset, U8 ucLPNCount);

#ifdef FAKE_L3
void  L1_SetBufferNoBusy(U16 usPhyBufID);
#endif
#endif

/********************** FILE END ***************/

