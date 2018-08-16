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
Filename     : L1_SubCmdProc.h
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
****************************************************************************/
#ifndef _L1_HCMD_PRO_H
#define _L1_HCMD_PRO_H

#include "Disk_Config.h"
#include "L1_PlatformDefine.h"
#include "L1_SCmdInterface.h"
#include "L1_SubCmdProc.h"

#define  SUBCMD_ENTRY_DEPTH  (1) //(16)  /* SubCmd Entry > 1 only for debug use */

#define  SUBCMD_OFFSET(lba)    ((lba)&SEC_PER_BUF_MSK)

#define  SUBCMDLPN_OFFSET(cmdoffset)  ((cmdoffset) >> SEC_PER_LPN_BITS)

#define  SUBCMD_OFFSET_IN_LPN(cmdoffset)  ((cmdoffset) & SEC_PER_LPN_MSK)

#define  SUBCMDLPN_COUNT(cmdlength)  (((cmdlength) + SEC_PER_LPN_MSK) >> SEC_PER_LPN_BITS)

#define    SUBCMD_STAGE_IDLE    0x00
#define    SUBCMD_STAGE_SPLIT   0x01
#define    SUBCMD_STAGE_RESOURCE_CHECK 0x05
#define    SUBCMD_STAGE_SEARCH  0x02
#define    SUBCMD_STAGE_CACHE   0x03
#define    SUBCMD_STAGE_HOSTIO  0x04

#define    PARTIAL_HIT_NONE    0x00
#define    PARTIAL_HIT_PHASE1  0x01
#define    PARTIAL_HIT_PHASE2  0x02
#define    PARTIAL_HIT_PHASE3  0x03

#define L1_TRIM_WRITE_MAX_LCT_CNT   (0x10)   //Trim/Write ratio 16:1
#define L1_TRIM_BUSY_MAX_LCT_CNT    (0x40)   //2MB data, L2 will use 0.73ms
#define L1_TRIM_IDLE_MAX_LCT_CNT    (0x400)   //32MB data, L2 will use 11.7ms
#define L1_TRIM_FLUSH_MAX_LOOP_CNT  (0x4000)  //search needs about 5ms in HCLK300M  

#define L1_TRIM_BUSY_FLUSH_THS      ((PRIO_FIFO_DEPTH*3)/4)

#ifdef SIM
#define L1_FLUSH_CACHE_THRESHOLD    (200)  //200ms
#else
#define L1_FLUSH_CACHE_THRESHOLD    (10000000)  //200ms in HCLK 500MHz
#endif

typedef struct _SUBCMD_ADD_INFO
{
    /* total 4 DWORD */
    /*Input SUBCMD info*/
    U32 ulSubCmdLCT;

    U8 ucSubCmdOffsetIN;//sector offset in buffer
    U8 ucSubCmdlengthIN;
    U8 ucSubLPNOffsetIN;//LPN offset in buffer
    U8 ucSubLPNCountIN;//LPN Count in SubCmd

    U8  ucPuNum;
    U8  ucSyncFlag;
    U8  ucSyncIndex;
    U8  ucRsvd;

    /*Output Buffer info*/
    U8 ucBufLPNOffsetOUT;
    U8 ucBufLPNCountOUT;
    U8 ucSubCmdOffsetOUT;
    U8 ucSubCmdlengthOUT;
}SUBCMD_ADD_INFO;

typedef struct _SUBCMD
{
    SCMD *pSCMD;

    /*4-DW*/
    SUBCMD_ADD_INFO SubCmdAddInfo; //same as SATA

    U8 SubCmdStage;
    U8 SubCmdHitResult;
    U16 SubCmdPhyBufferID;

    U32 CacheStatusAddr;

    /*2-DW*/
    U8 LpnSectorBitmap[LPN_PER_BUF]; // Warniing: 4 != LPN_PER_BUF JasonGuo

    /*7-DW*/
    union
    {
        /* for different host, we have different SubCmdHostInfo */
        SUBCMD_HOST_INFO SubCmdHostInfo;

        U32 aHostInfoDW[7];
    };
}SUBCMD;

#define SUBCMD_SIZE_DW  (sizeof(SUBCMD)/sizeof(U32))

//extern function
extern void L1_HostCMDProcInit(void);
extern void L1_HostCMDProcWarmInit(void);
extern BOOL  L1_BufferManagement(void);
extern U8 L1_ReadBufferManagement(void);
extern BOOL L1_TaskCacheSearch(SUBCMD* pSubCmd);
extern BOOL L1_TaskResourceCheck(SUBCMD* pSubCmd);
extern BOOL L1_TaskCacheManagement(SUBCMD* pSubCmd);
extern void L1_TaskRecycle(SUBCMD* pSubCmd);
extern BOOL L1_TaskMergeFlushManagement(void);
extern SUBCMD *L1_GetNewSubCmd(SCMD* pSCMD);
extern BOOL L1_TaskHostIO(SUBCMD* pSubCmd);

extern void L1_HostCMDProcSram0Map(U32 *pFreeSramBase);
#ifdef HOST_READ_FROM_DRAM
extern BOOL L1_RFDBufferManagment(void);
#endif
extern void L1_TaskInit_Fake();
extern void L1_TaskAhciIO_FAKE(SUBCMD *pSubCmdInfo);
extern BOOL L1_TaskFlushCacheLine(void);
extern BOOL L1_TaskSystemIdle(void);
extern void L1_TaskInternalIdle(void);
extern BOOL L1_IsIdle(void);
extern U32 L1_TaskNormalPCCheckBusy(void);
extern BOOL L1_TaskErrorHandle(void);
#ifdef HOST_SATA
extern BOOL L1_GetCacheLockedFlag(void);
extern BOOL L1_GetMcuCacheLockedFlag(void);
extern void L1_InitCacheLockedFlag(void);
extern void L1_SetCacheLockedFlag(void);
extern BOOL L1_CacheLockedCheck(SUBCMD* pSubCmd);
extern void L1_ClearCacheLockedFlag(void);
#endif


typedef struct _SUBCMD_ENTRY{
    SUBCMD tSubCmd[SUBCMD_ENTRY_DEPTH];
}SUBCMD_ENTRY;

typedef enum _L1_ERRORHANDLE_STATUS_
{
    L1_ERRORHANDLE_INIT = 0,
    L1_ERRORHANDLE_WAIT_SUBCMD,
    L1_ERRORHANDLE_WAIT_IDLE,
    L1_ERRORHANDLE_WARM_INIT,
    L1_ERRORHANDLE_DONE
} L1_ERRORHANDLE_STATUS;

extern GLOBAL  SUBCMD  *g_pCurSubCmd;
extern GLOBAL  SUBCMD_ENTRY *gpSubCmdEntry;

extern GLOBAL  U32 g_L1IdleCount;
extern GLOBAL  U8  gPartialHitFlag;
extern GLOBAL  U8  g_ucPartialHitNeedNewWriteCache;
extern GLOBAL BOOL g_L1PopCmdEn;
extern GLOBAL BOOL g_L2RebuildDC;

#endif

/********************** FILE END ***************/

