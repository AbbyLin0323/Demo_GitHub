/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     : L1_Buffer.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20121126 blakezhang modify for L1 ramdisk design
20120118 peterxiu 001 first create
*************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"
#ifdef FAKE_L1
#include "L2_defines.h"
#endif
//#include "HAL_SearchEngine.h"

/* Buffer Tag, each tag per buffer */
BUFF_TAG gBuffTag[L1_BUFFER_TOTAL];
extern L1_BUF_STATUS gBufStatus[PU_NUM];

/* read buffer */
LOCAL U16 s_aBufReadEntry[L1_BUFFER_COUNT_READ];

/* PU Fifo */
U16 s_aWriteBufFifoEntry[PU_NUM][L1_BUFFER_PER_PU + 1];

/* each buffer will store the LPN address and its corresponding Sector bitmap */
U32 g_ulBufferLpnBaseAddr;
//LOCAL U32 g_ulBufferLpnSectorBitmapBaseAddr;

//read buffer dont need this, should change read partial hit code
U8 gBufSectorBitMap[L1_BUFFER_TOTAL][LPN_PER_BUF];


/* PU Fifo pointers */
/* For each PU:      
Head           Recycle         Flush             Merge        Tail
|                  |                 |                  |               |  
|                  |                 |                  |               |

[Head,   Recycle -1]   --> free node
[Merge,  Tail -1]        --> waiting for merge
[Flush,   Merge -1]     --> waiting for flush
[Recycle, Flush -1]     --> waiting for recycle
*/
U8 PUFifoHead[PU_NUM];
U8 PUFifoTail[PU_NUM];
U8 PUFifoMerge[PU_NUM];
U8 PUFifoFlush[PU_NUM];
U8 PUFifoRecycle[PU_NUM];

/*
CacheStatus:
0~N   16K cache status for sequcen buffer.
4N~4(N+M) 4K cache status for random buffer.

Flash Status:
0 ~ 4M  flash special status updata  for random buffer.
*/
U32* g_pBufBusy;
U32* g_pFlashStatus;


U32 g_ulBufferStartPhyId;
LOCAL U32 g_ulBufferBaseAddr;
U32 g_L1TempBufferAddr;
U32 g_L1TempBufferPhyId;

/* get start logic buffer id from PU Num - only for write buffer */
#define  START_LGCBUFID_FROM_PUNUM(PUNum)  (L1_BUFFER_COUNT_READ + (L1_BUFFER_PER_PU*PUNum))

//LocalFunction
LOCAL void L1_ClearFlashStatus(U16 usLogicBufID);
/********************** FILE END ***************/

