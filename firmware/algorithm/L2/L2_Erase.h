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
  File Name     : L2_Erase.h
  Version       : Initial Draft
  Author        : Zoe Wen
  Created       : 2015/2/4
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2015/2/4
    Author      : Zoe Wen
    Modification: Created file
2.Date      : 2015/5/6
Author      : Javen Liu
Modification: Change erase pool to fifo

******************************************************************************/
#ifndef __L2_ERASE_H__
#define __L2_ERASE_H__
#include "Disk_Config.h"

/* Erase queue size */
/* (LPN_PER_BUF) one for Host erase */
/* +1 reserve */
#define Rsv_EraseQueueSize      0x0
#define ERASE_QUEUE_SIZE        (SLC_BLK_CNT + 1) 
#define MAX_ERASE_SLCBLK_CNT    2
#define MAX_ERASE_TLCBLK_CNT    1
#define MAX_ERASE_BLKCNT        (MAX_ERASE_SLCBLK_CNT + MAX_ERASE_TLCBLK_CNT) // 64 host super page write may open most 2 SLC block and 1 TLC block

typedef struct _EraseQueue EraseQueue;
typedef struct _RecordEraseInfo RecordEraseInfo;
typedef struct _EraseOperation EraseOperation;
typedef BOOL(*IsEmptyFunc)(EraseQueue*, BOOL);
typedef BOOL(*IsFullFunc)(EraseQueue*, BOOL);
typedef U16(*QueueSizeFunc)(EraseQueue*, BOOL);
typedef void(*EnQueueFunc)(EraseQueue*, U16, BOOL);
typedef U16(*DeQueueFunc)(EraseQueue*, BOOL);

struct _EraseQueue
{
    U16  ucFront;    // Head pointer
    U16  ucRear;     // Rear pointer
    U16  ucTLCFront;    // Head pointer
    U16  ucTLCRear;     // Rear pointer
    U16 aVBN[ERASE_QUEUE_SIZE + 1];    // Free virtual block number
    U16 aTLCVBN[TLC_BLK_CNT + 1];

    //Add for super page,0:not erase,1:erased
    U32 ulEraseBitMap;
    U32 ulTLCEraseBitMap;

    IsEmptyFunc pIsEmpty;
    IsFullFunc  pIsFull;
    QueueSizeFunc pSize;
    EnQueueFunc pEn;    // Enter virtual block into Queue
    DeQueueFunc pDe;    // Delete virtual block from Queue
};

struct _RecordEraseInfo
{
    U16 uNeedEraseSLCPBN[LUN_NUM_PER_SUPERPU][MAX_ERASE_SLCBLK_CNT];
    U16 uNeedEraseTLCPBN[LUN_NUM_PER_SUPERPU][MAX_ERASE_TLCBLK_CNT];
    U16 uNeedEraseSLCVBN[MAX_ERASE_SLCBLK_CNT];
    U16 uNeedEraseTLCVBN[MAX_ERASE_TLCBLK_CNT];
    U8 ucTotalEraseSLCCnt;
    U8 ucTotalEraseTLCCnt;
};

struct _EraseOperation
{
    BOOL L2NeedEraseBlk;
    BOOL bEraseTLC;
    BOOL bEraseScheduleEn;
    U16 uErasingTLCVBN;
};

extern GLOBAL  EraseQueue *g_EraseQueue[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  RecordEraseInfo g_NeedEraseBlkInfo[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  EraseOperation g_EraseOpt[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL void L2_EraseQueueInit(U8 ucSuperPuNum);
extern GLOBAL void L2_EraseQueueShutdown(U8 ucSuperPuNum);
extern GLOBAL U16 L2_GetEraseQueueSize(U8 ucSuperPuNum, BOOL IsSLCBlk);
extern GLOBAL BOOL L2_IsAllEraseQueueEmpty(U8 ucSuperPuNum);
extern GLOBAL void L2_InsertBlkIntoEraseQueue(U8 ucSuperPuNum, U16 usVBN, BOOL IsSLCBlk);
extern GLOBAL U16 L2_PopBlkFromEraseQueue(U8 ucSuperPuNum, BOOL IsSLCBlk);
extern GLOBAL BOOL L2_SetEraseInfoAfterFlushPMT(U8 ucSuperPuNum);
extern GLOBAL void L2_ClearEraseInfo(U8 ucSuperPuNum);

extern GLOBAL U16 L2_SelectNeedEraseBlkFromQueue(U8 ucSuperPuNum);
extern GLOBAL void L2_ClearEraseBitmap(U8 ucSuperPuNum);
//extern GLOBAL void L2_SetEraseQueueSizeLimit(U8 ucSuperPuNum, U16 usVal);

#endif
