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
Filename    :L1_Debug.h
Version     :Ver 1.0
Author      :Blakezhang
Date        :2012.12.07
Description :include some POSIX C header file
Others      :
Modify      :
****************************************************************************/

#ifndef __L1_DEBUG_H__
#define __L1_DEBUG_H__

#include "BaseDef.h"

typedef struct _SCMD_PRINT
{
    U32 ulLBA;
    
    U8 ucTag;
    U8 ucSecLen;
    U8 ucFirst;
    U8 ucLast;

} SCMD_PRINT;

typedef struct _SUBCMD_PRINT
{
    U32 ulLCT;
  
    U32 ucPuNum:8;
    U32 ucHit:8;
    U32 usPhyBufID:16;

    U8  ucLPNOff;
    U8  ucLPNCnt;
    U8  ucSecOff;
    U8  ucSecLen;
}SUBCMD_PRINT;

typedef struct _BUF_REQ_PRINT
{
    U32 ucTag:8;
    U32 ucType:8;
    U32 usPhyBufID:16;

    U32 LPNOffset:8;
    U32 LPNCount:8;
    U32 ReqOffset:8;
    U32 ReqLength:8;

#if (BUF_SIZE_BITS <= 15)
    U32 LPN[8];
#else
    U32 LPN[16];
#endif
}BUF_REQ_PRINT;

typedef struct _BUF_COMM_PRINT
{
    U32 usPhyBufID:16;
    U32 Stage:8;
    U32 WPointer:8;
} BUF_COMM_PRINT;


typedef struct _BUF_LPN_PRINT
{
    U32 ulLPN;

    U32 ucPuNum:8;
    U32 ucOffset:8;
    U32 SecBitmap:8;
    U32 Rsvd:8;
} BUF_LPN_PRINT;

typedef struct _CACHE_STATUS_PRINT
{
    U16 CurrID;
    U16 NextID;

    U32 LBA;

    U32 SecLen:16;
    U32 Status:16;

} CACHE_STATUS_PRINT;

extern void L1_DbgInit(void);
extern void L1_DbgSCMDShow(void);
extern void L1_DbgBufferShow(void);
extern void L1_DbgWritePuFifoShow(void);
extern void L1_DbgCacheLineShow(void);
extern void L1_DbgSubCmdShow(void);
extern void L1_DbgBufReqShow(void);
extern void L1_DbgGlobalInfoShow(void);
extern void L1_DbgShowAll(void);

#endif

