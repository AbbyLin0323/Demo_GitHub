/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
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

//TRACE DEFINE START
#define L1_SUBMODULE2_ID        2
#define L1_SUBMODULE2_NAME      "Interface"
#define L1_SUBMODULE2_ENABLE    1
#define L1_SUBMODULE2_LEVEL     LOG_LVL_TRACE
//TRACE DEFINE END

#define L1_REQ_REQTYPE_READ  HCMD_READ
#define L1_REQ_REQTYPE_WRITE HCMD_WRITE
#define L1_REQ_REQTYPE_SHUTDOWN HCMD_SHUTDOWN
#define L1_REQ_REQTYPE_TRIM HCMD_TRIM
#define L1_REQ_REQTYPE_SMART HCMD_SMART
#define L1_REQ_REQTYPE_VD HCMD_VD

#define L1_REQ_LOCALREQ_ENABLE 1
#define L1_REQ_LOCALREQ_DISABLE 0

#define L1_REQ_REQSEQ_ENABLE 1
#define L1_REQ_REQSEQ_DISABLE 0

#define L1_REQ_SIZE_4K        L1_BUF_SIZE_4K
#define L1_REQ_SIZE_8K        L1_BUF_SIZE_8K
#define L1_REQ_SIZE_16K       L1_BUF_SIZE_16K
#define L1_REQ_SIZE_32K       L1_BUF_SIZE_32K

#define L1_REQ_SPLIT_SIZE_4K  L1_BUF_SPLIT_SIZE_4K
#define L1_REQ_SPLIT_SIZE_8K  L1_BUF_SPLIT_SIZE_8K     
#define L1_REQ_SPLIT_SIZE_16K  L1_BUF_SPLIT_SIZE_16K    
#define L1_REQ_SPLIT_SIZE_32K  L1_BUF_SPLIT_SIZE_32K  


typedef struct _BUF_REQ
{
    /* DWORD 0 */
    /*CMD TAG for L3*/
    U32 Tag:8;      

    /*SUBCMD ID for L3*/
    U32 SubCmdID:7;/*subcmd id flag*/
    U32 SubCmdIDFirstRCMDEn:1;/*first read cmd enable*/

    /*PRD ID for L3*/
    U32 PrdID:8;   

    /* Brookewang 2012/04/20 delete Buffer size & Split size */
    /*Buffer size define,0 4K,1 8K,2 16K,3 32K*/
    //  U32 BufferSize:4;

    /*Split size of buffer define,0 4K,1 8K,2 16K,3 32K;
    LPN total max count /= buffer size/split size*/
    //  U32 SplitSize:4;

    // This field is used to mark whether this req is a  read-full hit buf req
    U32  ReadFullHitWrite :8;
    

    /* DWORD 1 */

    U32 ReqType:8;  /*Request Type,0 write 1 read*/
    
    /*Sequecen enbale, 0 disable 1 enable,if enable only the first LPN of
    the request is record in LPN array,L2 can simply increase LPN by 1 for
    following lpn */
    U32 ReqSequenceEn:4;

    /*Local Req enbale,0 disable 1 enable,if enable the request is the local
    request which data is not relavate to host request like merge read */
    U32 ReqLocalREQFlag:1;
    U32 ReqLocalREQStatusEnable:1;
    U32 ReqLocalRSVD:2;

    /* Brookewang 2012/04/20 delete LogicBufferID */
    /*Phy buffer id*/
    U32 PhyBufferID:16;
    
    /* DWORD 2 */
    U32 ReqLocalREQStatusAddr;//for L1 local read operation
    
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
    
    /* DWORD 3 */
    U32 LPNOffset:8;
    U32 LPNCount:8;
    U32 ReqOffset:8;
    U32 ReqLength:8;

    union
    {
        /* DWORD 4~7 */
        U32 LPN[LPN_PER_BUF];
        struct 
        {
            U32 TrimStartLBA;
            U32 TrimLen;
        };
    };

    /* DWORD 8 */
    U32 LPNBitMap;
}BUF_REQ;

#define  BUF_REQ_SIZE   (sizeof(BUF_REQ)/4)

/*FIFO Interface*/
extern void L1_BufReqFifoInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
extern U32 L1_HighPrioFifoFull(void);
extern U32 L1_LowPrioFifoFull(void);
extern U32 L1_BufReqFifoFull(void);
extern U32 L1_HighPrioFifoEmpty(void);
extern U32 L1_LowPrioFifoEmpty(void);
extern U32 L1_BufReqFifoEmpty(void);
extern U32 L1_PrioFifoGetPendingCnt(void);
extern BUF_REQ* L1_HighPrioFifoGetBufReq(void);
extern BUF_REQ* L1_LowPrioFifoGetBufReq(void);
extern void L1_HighPrioMoveTailPtr(void);
extern void L1_LowPrioMoveTailPtr(void);
extern U32 L1_BufReqPush(BUF_REQ* BufReq);
extern U32 L1_BufReqPop(BUF_REQ** BufReq);
extern void L1_BufReqMoveHeadPtr(void);
extern void L1_HostReadBuildBufReq(SUBCMD *pSubCmd);

#define PRIO_FIFO_DEPTH         32

/* PrioFifo select */
typedef enum _PRIO_FIFO_SEL_
{
    PRIO_FIFO_NONE = 0,
    PRIO_FIFO_HIGH,
    PRIO_FIFO_LOW
}PRIO_FIFO_SEL;

extern BUF_REQ HighPrioEntry[PRIO_FIFO_DEPTH];
extern BUF_REQ LowPrioEntry[PRIO_FIFO_DEPTH];

extern U8 HighPrioFifoHead;
extern U8 HighPrioFifoTail;

extern U8 LowPrioFifoHead;
extern U8 LowPrioFifoTail;

extern U8 g_ucCurrPoppedFifo;


//=============FAKE_L1=======================
#ifdef FAKE_L1
#include "L1_Buffer.h"

typedef struct _FakeL1_ReqRecord
{
    BUF_REQ Req;
    U32 uStatusCnt;
    U32 uFlashStatus[8];
}FakeL1_ReqRecord;

typedef struct _FakeL1_LPN_CMD
{
    U32 uCount;
    U32 uLPN[L1_BUFFER_COUNT_SEQW * 8];
}FakeL1_LPN_CMD;

extern FakeL1_ReqRecord* g_ulL1BuffReqRecord;
extern FakeL1_LPN_CMD* g_ulL1LPNCMD;
extern U16* L2_DbgDataLpnCnt;

#endif
#endif

/********************** FILE END ***************/

