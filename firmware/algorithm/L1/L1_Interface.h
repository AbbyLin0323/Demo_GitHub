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
Filename    :L1_Interface.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    15:11:33
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef _L1_INTERFACE_H
#define _L1_INTERFACE_H

#include "L1_Cache.h"
#include "L2_Defines.h"

#define L1_REQ_REQTYPE_READ       0
#define L1_REQ_REQTYPE_WRITE      1

// set the buffer request fifo depth to 2, please note that due to the design of the buffer request fifo
// mechanism, total outstanding request number is actually (PRIO_FIFO_DEPTH-1), which means by setting
// PRIO_FIFO_DEPTH to 2 only 1 outstanding buffer request is allowed per PU
#define PRIO_FIFO_DEPTH (32/SUBSYSTEM_SUPERPU_NUM)

typedef struct _BUF_REQ_WRITE
{
    /* DWORD 0 */
    
    U32 ReqStatusAddr;

    /*
    the address infomation of the reqeust:

    ReqOffset,ReqLength only use for read request.
    The write request is always full buffer size.The offset is cala in buffer size.

    for example,LPNOffset = 2,LPNcount= 2,ReqOffset = 17,ReqLength = 8
    LPN[0] = empty; offset in buffer 0 length 0,offset in lpn 0 length 0,MSK = 0x0;
    LPN[1] = empty; offset in buffer 8 length 0,offset in lpn 0 length 0,MSK = 0x0;
    LPN[2] = addr2:  offset in buffer 17 length 7,offset in lpn 1 length 7,MSK = 0xe;
    LPN[3] = addr1;  offset in buffer 24 length 1,offset in lpn 0 length 1,MSK =0x1;

    */
    
    /* DWORD 1 */
    U32 LPNOffset : 8;
    U32 LPNCount : 8;
    U32 PhyBufferID : 16;

    /* DWORD 2~9 */
    union
    {
        struct 
        {
            U32 TrimStartLPN;
            U32 TrimLPNCount;
        };

        U32 LPN[LPN_PER_BUF];
    };

#ifdef DATA_MONITOR_ENABLE
    U8 ucWriteData[SEC_PER_BUF];
#endif
}BUF_REQ_WRITE;

#define  BUF_REQ_WRITE_SIZE   (sizeof(BUF_REQ_WRITE)/4)

typedef struct _BUF_REQ_FIFO
{
    //BUF_REQ tBufReq[PRIO_FIFO_DEPTH];
    BUF_REQ_WRITE *tBufReq;
} BUF_REQ_FIFO;

typedef struct _BUF_REQ_READ
{
    /* DWORD 0 */
    U32 bFirstRCMDEn : 1;/*first read SCmd enable*/
    U32 bReqLocalREQFlag : 1; 
    U32 bReadPreFetch : 1;
    U32 bPMTLookuped : 1;
    U32 bSeq : 1;
    U32 bNeedReLookupPMT : 1;
    U32 bsRSVD0 : 2;
    U32 ucNextBufReqID : 8;
    U32 usPhyBufferID : 16;

    /* DWORD 1 */
    U32 ulReqStatusAddr;

    /* DWORD 2 */
    U32 ucLPNOffset : 8;
    U32 ucLPNCount : 8;
    U32 ucReqOffset : 8;  /* start LBA offset in LCT, for Host Read */
    U32 ucReqLength : 8; /* requested sector count, for Host Read */

    /* DWORD 3 */
    U32 ulLPNBitMap; /* for Merge Read */

    /* DWORD 4~11(32KB page) / 4~19(64KB page) */
    U32 aLPN[LPN_PER_BUF];

    /* DWORD 12~14 */
    union
    {
        /* for different host, we have different BufReq HostInfo */
        BUFREQ_HOST_INFO tBufReqHostInfo;

        U32 aHostInfoDW[3];
    };

    /* DWORD 15~22(32KB page) / 23~30(64KB page) */
    PhysicalAddr aFlashAddr[LPN_PER_BUF]; /* filled by L2 after looking up PMT */
}BUF_REQ_READ;

typedef struct _BUF_REQ_READ_LINK
{
    /* head -> ... -> tail */
    U32 ucHeadID : 8; /* INVALID_2F means link is empty */
    U32 ucTailID : 8; /* INVALID_2F means link is empty */
    U32 usRSVD : 16;
}BUF_REQ_READ_LINK;

extern GLOBAL  U32 g_ulReadBufReqPuBitMap;
extern GLOBAL  U32 g_ulWriteBufReqPuBitMap;

extern GLOBAL  U32 g_ulReadBufReqFullPuBitMap;
extern GLOBAL  U32 g_ulWriteBufReqFullPuBitMap;

extern GLOBAL  U32 g_aucReadBufReqCount[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  U32 g_aucWriteBufReqCount[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL BUF_REQ_READ *g_ReadBufReqEntry[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL BUF_REQ_READ_LINK g_FreeReadBufReqLink[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL BUF_REQ_READ_LINK g_HighPrioLink[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL  BUF_REQ_FIFO g_LowPrioEntry[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  BUF_REQ_FIFO *gpLowPrioEntry[SUBSYSTEM_SUPERPU_MAX];

extern GLOBAL  U8 LowPrioFifoHead[SUBSYSTEM_SUPERPU_MAX];
extern GLOBAL  U8 LowPrioFifoTail[SUBSYSTEM_SUPERPU_MAX];

/*FIFO Interface*/
extern U8 L1_IsReadBufReqPending(U8 ucSuperPu);
extern U8 L1_IsWriteBufReqPending(U8 ucSuperPu);

extern void L1_BufReqInit(void);
extern void L1_BufReqWarmInit(void);
extern U32 L1_FreeReadBufReqLinkEmpty(U8 ucSuperPu);
extern U32 L1_LowPrioFifoFull(U8 ucSuperPu);
extern U32 L1_HighPrioLinkEmpty(U8 ucSuperPu);
extern U32 L1_LowPrioFifoEmpty(U8 ucSuperPu);
extern U32 L1_BufReqEmpty(void);
extern U32 L1_AllHighPrioLinkEmpty(void);
extern U32 L1_GetLowPrioFifoPendingCnt(void);
extern BUF_REQ_READ* L1_GetReadBufReq(U8 ucSuperPu, U8 ucReadBufReqID);
extern U8 L1_AllocateReadBufReq(U8 ucSuperPu);
extern BUF_REQ_WRITE* L1_LowPrioFifoGetBufReq(U8 ucSuperPu);
extern void L1_InsertBufReqToHighPrioLinkTail(U8 ucSuperPu, U8 ucReadBufReqID);
extern void L1_LowPrioMoveTailPtr(U8 ucSuperPu);
extern U8 L1_GetHighPrioLinkHead(U8 ucSuperPu);
extern U8 L1_GetHighPrioLinkNextNode(U8 ucSuperPu, U8 ucCurBufReqID);
extern BUF_REQ_READ* L1_GetReadBufReq(U8 ucSuperPu, U8 ucReadBufReqID);
extern void L1_RemoveBufReqFromHighPrioLink(U8 ucSuperPu, U8 ucPreNodeID);
extern BUF_REQ_WRITE* L1_WriteBufReqPop(U8 ucSuperPu);

extern void L1_RecycleReadBufReq(U8 ucSuperPu, U8 ucReadBufReqID);
extern void L1_WriteBufReqMoveHeadPtr(U8 ucSuperPu);

extern void L1_HostReadBuildBufReq(SUBCMD *pSubCmd);
extern void L1_BufReqFifoSram0Map(U32 *pFreeSramBase);
extern U32 L1_GetRWBufReqPuBitMap(void);
extern void L1_ForL2SetReLookupPMTFlag(U8 ucSuperPu);
#endif

/********************** FILE END ***************/

