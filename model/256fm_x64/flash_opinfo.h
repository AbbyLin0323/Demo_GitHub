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
#ifndef _FLASH_OPINFO
#define _FLASH_OPINFO

#include "BaseDef.h"
#define MAX_BLOCK_ERR    5
#define MAX_READRETRY_CNT    HAL_FLASH_READRETRY_CNT

#define DATA_BLOCK_START_INDEX  16

typedef struct _ERR_INFO
{
    U32 bsErrCode : 8;
    U32 bsOpStatus : 5;
    U32 bsRetryTime : 5;
    U32 bsErrPage : 16;
    U32 bsRsved: 30;
}ERR_INFO;

typedef struct _ERR_HANDLE
{
    U32 bsErr : 1;
    U32 bsRsved : 31;
    ERR_INFO ErrInfo[MAX_BLOCK_ERR];
}ERR_HANDLE;

typedef struct _FLASH_BLOCK_STATUS
{
    U32 bsBlockType : 4;        // 0: invalid blk, 1: SLC blk, 2: MLC blk, 3: TLC blk; other reserved
    U32 bsBlockStatus : 1;      // 1 is use up, 0 is normal
    U32 bsEraseCnt : 15;
    U32 bsErase : 1;
    U32 bsOpen : 1;
    U32 bsResved : 10;
    U32 ulReadCnt;
    ERR_HANDLE tBlockErrHandel;
}FLASH_BLOCK_STATUS;

typedef struct _FLASH_CE_STATUS
{
    U32 ulBadBlockCnt;
}FLASH_CE_STATUS;

#define MAX_PRE_BBSET 100

typedef struct _FLASH_CE_PRE_EraseErrSetting
{
    U8 ucBadBlockCnt;
    U8 ucRsved;
    U16 usEraseCntIndex[MAX_PRE_BBSET];
}FLASH_CE_PRE_EraseErrSetting;

typedef struct _FLASH_OP_STATISTICS
{
    U32 ulHWritePageCnt;
    U32 ulLWritePageCnt;
    U32 ulHReadPageCnt;
    U32 ulLReadPageCnt;
    U32 ulHEraseCnt;
    U32 ulLEraseCnt;
    UINT64 ulTotalEraseCnt;
    UINT64 ulDataBlockEraseCnt;
    UINT64 ulTotalWriteCnt;
    UINT64 ulTotalReadCnt;

    /* Statistical info for LUN */
    U32 ulCacheRdCnt;
    U32 ulCacheWrCnt;
    
    /* Statistical info for page */
    U32 aPgRetryCnt[PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN][PG_PER_BLK * INTRPG_PER_PGADDR];

    /* Statistical info for Blk andWL */
    U32 ulOpenBlkRdCnt;
    U32 ulOpenWLRdCnt;
    U32 ulNextOpenWLRdCnt;  
    
}FLASH_OP_STATISTICS;

typedef struct _FLASH_ERROR_CONDITION
{
    U32 ulHCondition;
    U32 ulLCondition;
}FLASH_ERROR_CONDITION;

#endif
