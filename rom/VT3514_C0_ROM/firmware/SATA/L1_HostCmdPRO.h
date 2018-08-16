/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :                                           
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#ifndef _L1_HCMD_PRO_H
#define _L1_HCMD_PRO_H


//TRACE DEFINE START
#define L1_SUBMODULE3_ID        3
#define L1_SUBMODULE3_NAME      "HostCmdPRO"
#define L1_SUBMODULE3_ENABLE    1
#define L1_SUBMODULE3_LEVEL     LOG_LVL_INFO
//TRACE DEFINE END

//extern function
extern void L1_HostCMDProcInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
#ifdef HCMD_PU_QUEUE
extern BOOL L1_AddHCmdtoPu(U8 ucCmdTag, U8 PuNum);
extern HCMD * L1_HostCMDSelectFromPu(void);
extern void L1_MoveHCmdPuHeader(void);
#else
BOOL  L1_AddNewHostCMD(U8 ucNewCmdTag);
HCMD * L1_HostCMDSelect(void);
#endif
extern U8 L1_HostCMDGetCount(void);
extern SUBCMD* L1_SplitHCMD(HCMD *pTgtHCMD);
extern U8 L1_SplitSUBCMD(SUBCMD* pSubCmdIn, SUBCMD* pTgtSUBCMD);
extern BOOL L1_HandleWritePartialHit(void);
extern BOOL  L1_BufferManagment(void);
extern BOOL L1_TaskCacheSearch(SUBCMD* pSubCmd);
extern BOOL L1_TaskCacheManagement(SUBCMD* pSubCmd);
extern BOOL L1_TaskSataIO(SUBCMD* pSubCmd);
extern void L1_TaskRecycle(SUBCMD* pSubCmd);
extern BOOL L1_TaskMergeFlushManagement(void);


//add for windows simulation
extern BOOL L1_HostCMDFifoEmpty();


extern HCMD *gCurHCMD;
extern SUBCMD  *gpCurSubCmd;
extern U8      gCurSubCmd;
extern SUBCMD  gSubCmdEntry[SUBCMD_ENTRY_DEPTH];
extern SUBCMD  gPartialHitSubCmd[LPN_PER_BUF];
extern U8      gPartialHitCnt;
extern U8      gPartialHitBase;
extern U8      gPartialHitFlag;

extern U32 g_L1IdleCount;

//extern macro
#define M_HCMDFIFO_FULL(Head, NextTail)    ((NextTail == Head)? 1:0 )
#define M_HCMDFIFO_EMPTY(Head, Tail)  ((Head  == Tail)? 1:0)


#define M_SUBCMDFIFO_FULL(Head, NextTail)    ((NextTail == Head)? 1:0 )
#define M_SUBCMDFIFO_EMPTY(Head, Tail)  ((Head  == Tail)? 1:0)

#define  HCMDLBA_SUBCMDLBA(lba)    ((lba) & (~SEC_PER_BUF_MSK))

#define  SUBCMD_OFFSET(lba)    ((lba)&SEC_PER_BUF_MSK)

#define  SUBCMDLPN_OFFSET(cmdoffset)  ((cmdoffset)>>SEC_PER_LPN_BITS  )

#define  SUBCMD_OFFSET_IN_LPN(cmdoffset)  ((cmdoffset)&SEC_PER_LPN_MSK )

#define  SUBCMDLPN_COUNT(cmdlength)  (((cmdlength)+SEC_PER_LPN_MSK)>>SEC_PER_LPN_BITS)

#define    SUBCMD_STAGE_IDLE    0x00
#define    SUBCMD_STAGE_SPLIT   0x01
#define    SUBCMD_STAGE_SEARCH  0x02
#define    SUBCMD_STAGE_CACHE   0x03
#define    SUBCMD_STAGE_SATAIO  0x04

#define    PARTIAL_HIT_NONE    0x00
#define    PARTIAL_HIT_PHASE1  0x01
#define    PARTIAL_HIT_PHASE2  0x02
#define    PARTIAL_HIT_PHASE3  0x03

extern U8 g_ucL1returnflag;

#ifdef HCMD_PU_QUEUE
//#define PUCMDQ_SLOT_SIZE  ((NCQ_DEPTH*4)/PU_NUM)
#define PUCMDQ_SLOT_SIZE  NCQ_DEPTH

typedef struct _PU_CMD_Q{
    U8 head;
    U8 tail;
#ifndef HCMD_PUQ_IN_OTFB
  U8 cmdSlot[PUCMDQ_SLOT_SIZE+1];
#endif
}PU_CMD_Q;

extern PU_CMD_Q gPuCmdQ[PU_NUM];
#else
extern U32  LocalHCMDFIFOHead;
extern U32  LocalHCMDFIFOTail;
#endif

typedef struct _L1_BUF_STATUS{
    U16 UsedCnt:4;
    U16 MergeCnt:4;
    U16 FlushCnt:4;
    U16 RecycCnt:4;
}L1_BUF_STATUS;

//extern L1_BUF_STATUS gBufStatus[PU_NUM];

#endif

/********************** FILE END ***************/

