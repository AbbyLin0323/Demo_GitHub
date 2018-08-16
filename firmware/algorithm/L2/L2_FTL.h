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
Filename    :L2_FTL.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.28
Description :defines for Read/Write Dispatcher
Others      :
Modify      :
****************************************************************************/
#ifndef __L2_FTL_H__
#define __L2_FTL_H__
#include "BaseDef.h"
#include "L2_Defines.h"
#include "L2_PMTPage.h"

#include "HAL_Inc.h"
#include "L1_PlatformDefine.h"
#include "L1_SubCmdProc.h"
#include "L1_Cache.h"
#include "L1_Buffer.h"
#include "L1_CacheStatus.h"
#include "L1_Interface.h"

#define WAIT_ERASE_STS_QUE_SIZE (NFCQ_DEPTH)
#define WAIT_ERASE_STS_ARRAY_SIZE (WAIT_ERASE_STS_QUE_SIZE + 1)

#define IDLE_GC_TLC_QUOTA_MAX  9
#define IDLE_GC_SLC_QUOTA_MAX  3

//Host Read/Write Data structure
typedef struct L2_FTLDptr_Tag
{
    BUF_REQ_WRITE *pCurBufREQ;

}L2_FTLDptr;

typedef struct L2_FTLReadDptr_Tag
{
    //read request BufREQ level info
    //U8 m_ReqRemain;
    U8 m_LPNRemain;
    U8 m_LPNOffset;
    U8 m_bPMTLookuped;

    //when Read, get the Physical Address for the LPN.
    PhysicalAddr m_Addr[LPN_PER_BUF];

    //add for super page support
    U8 m_ReqLength[LPN_PER_BUF];
    BUFREQ_HOST_INFO BufReqHostInfo[LPN_PER_BUF];

    U16 m_BufReqLpnBitMap;//Some LPN is INVALID_8F
    U8 m_BufReqID;
    U8 m_usRsvd;
}L2_FTLReadDptr;

typedef struct
{

    U32 m_DevWriteCntOfCE;
    U32 m_LastDevWriteCntOfCE;
    U16 m_CurPMTFlushPosOfCE;
    U16 m_Rsvd;
}PMTFlushStatistic;

typedef struct FTL_SCHEDULER_TAG
{
    FTL_THREAD_TYPE eCurrThread;
    U16 aThreadQuota[FTL_THREAD_TYPE_ALL];
    U16 usThreadQuotaUsed;
    U16 bPassOneW:1;
    U16 bPassQuota:1;
    U16 bTableRebuildFlag:1;
    U16 bChkSPORSLCAllDFlag:1;
    U16 bsRsvd : 12;
    U8 ucCurSchTaskBitFlag;
}FTL_SCHEDULER;

typedef enum _FTL_SCHEDULE_TASK_BIT
{
   // TASK_HOSTWRITE_BIT = 0,
    TASK_SLCGC_BIT = 0,
    TASK_TLCGC_BIT,
    TASK_TLCMERGE_BIT,
    TASK_TLCSWL_BIT    
}FTL_SCHEDULE_TASK_BIT;

typedef struct _FTL_ERASE_STATUS_INFO
{
    U8 aucEraseStatus[WAIT_ERASE_STS_ARRAY_SIZE][LUN_NUM_PER_SUPERPU];
    U16 ausEraseVBN[WAIT_ERASE_STS_ARRAY_SIZE];
    U8 ucInsertPtr;
    U8 ucRecyclePtr;
}FTL_ERASE_STATUS_INFO;

typedef struct _FTL_ERASE_STATUS_MGR
{
    FTL_ERASE_STATUS_INFO tEraseStsInfo[SUBSYSTEM_SUPERPU_MAX];
}FTL_ERASE_STATUS_MGR;

typedef struct _FTL_IDLE_GC_MGR
{
    U16 usTLCAreaQuotaCnt[SUBSYSTEM_SUPERPU_MAX];
    U16 usSLCAreaQuotaCnt[SUBSYSTEM_SUPERPU_MAX];

    BOOL bIdleGC;
}FTL_IDLE_GC_MGR;

/*Patrol relative define start*/
//#define PATROL_READ_WAIT_TIME_FIXED

#define PATROL_READ_UECC_RECC_THS 4//number of UECC(retry SUCCESS) times add RECC times
#define PATROL_READ_SLC_FIRST_BOOT_FBC_THS 30
#define PATROL_READ_SLC_NORMAL_FBC_THS     35
#define PATROL_READ_TLC_FIRST_BOOT_FBC_THS 40
#define PATROL_READ_TLC_NORMAL_FBC_THS     45
#ifdef PATROL_READ_WAIT_TIME_FIXED
#define PATROL_READ_TIME_INTERVAL_THS 5//(3*24) //a period of time for starting next whole disk patrol read; Unit by hour
#else
#ifdef SIM
#define PATROL_READ_TIME_INTERVAL_THS_0 3 
#define PATROL_READ_TIME_INTERVAL_THS_1 3
#define PATROL_READ_TIME_INTERVAL_THS_2 3
#define PATROL_READ_TIME_INTERVAL_THS_3 3
#define PATROL_READ_TIME_INTERVAL_THS_4 3
#define PATROL_READ_TIME_INTERVAL_THS_5 3
#else
#define PATROL_READ_TIME_INTERVAL_THS_0 8
#define PATROL_READ_TIME_INTERVAL_THS_1 8
#define PATROL_READ_TIME_INTERVAL_THS_2 8
#define PATROL_READ_TIME_INTERVAL_THS_3 8
#define PATROL_READ_TIME_INTERVAL_THS_4 8
#define PATROL_READ_TIME_INTERVAL_THS_5 4
#endif

#define PATROL_READ_PE_CYCLE_THS_0  200
#define PATROL_READ_PE_CYCLE_THS_1  500
#define PATROL_READ_PE_CYCLE_THS_2  1000
#define PATROL_READ_PE_CYCLE_THS_3  1500
#define PATROL_READ_PE_CYCLE_THS_4  2000
#define PATROL_READ_PE_CYCLE_THS_5  3000
#endif
typedef struct _PATROL_READ_ENTRY
{
    U32 ulReadLUNBitMap; //LUNBitMap for SUPERPU_LUN_NUM_BITMSK
    U32 ulCurMaxTS;  //Max time stamp at Currnet time patorl read start
    U32 ulLastMaxTS; //Max time stamp at last time patrol read start
    
    U16 usCurVBN;  //current read virtual BLK
    U16 aCurPBN[LUN_NUM_PER_SUPERPU];  // current read physical BLK
    U16 usCurPPO;   //current read page
    U16 usScanBlkCnt; //statistic scaned blk num at one time(whole disk) PatrolRead

    BOOL bSLCBLK;  //SLC or MLC BLK flag
    
    U16 usUECCCnt; //UECC retry success times
    U16 usRECCCnt; //RECC times

    U8 ucReadPStage;      //PatrolRead process stage
    U8 ucFlashSts [LUN_NUM_PER_SUPERPU]; //L3 return read status
#ifdef PATROL_PASS_FBC_TO_L2
    U8 ucMaxCWFBC[LUN_NUM_PER_SUPERPU];  //L3 return max CW FBC for best retry read.
#endif
    //RED tRed[LUN_NUM_PER_SUPERPU]; //patrol read don't need redundant data.
} PATROL_READ_ENTRY;

typedef struct _PATROL_READ_MANAGER
{
    PATROL_READ_ENTRY * pReadEntry[SUBSYSTEM_SUPERPU_MAX];
    U32 bGetFirstBlockAfterInterval:1; //when geting firset block after wait interval or boot up
    U32 bContinueScanAfterBoot:1;      //After boot up, check first block FBC to see if continue scan.
    U32 ucReadStage:8;                 //PatrolRead 8 stage
    U32 bsRsvd : 22;

    U8 ucPatrolReadCmdSendPUBitMap;   //1-bit stand for one Super page  PatrolRead cmd send
    U8 ucPatrolReadPageDoneBitMap;    //1-bit stand for one Super page  PatrolRead done
    U8 ucPatrolReadBlkDoneBitMap;     //1-bit stand for one Super block PatrolRead done
    U8 ucPatrolReadScanSuperPUBitMap; //which super PUs participate this time of patrol read

    U32 ulPatrolWaitStartTime;        //Unit by hour, time of PatrolReadWait start, for SIM unit by minute
    U32 ulPatrolReadCycleCnt;         //statistic patrol read scan disk count.
} PATROL_READ_MANAGER;

#ifdef SCAN_BLOCK_N1
typedef struct _SCAN_N1_MANAGER
{
    U8 ucFlashSts [LUN_NUM_PER_SUPERPU]; //L3 return read status
    U32 ucCmdSendLunBitMap;   //1-bit stand for one lun page Read cmd send    
    U32 ucPageDoneBitMap;     //1-bit stand for one lun page Read done
} SCAN_N1_MANAGER;
#endif

typedef enum _PATROL_READ_PSTAGE
{
    PATROL_READ_PROCESS_SENDCMD,
    PATROL_READ_PROCESS_WAITCMD
}PATROL_READ_PSTAGE;

typedef enum _PATROL_READ_STAGE
{
    PATROL_READ_PREPARE,
    PATROL_READ_PROCESS,
    PATROL_READ_DONE_CHECK
}PATROL_READ_STAGE;

typedef enum _PATROL_READ_STAGE_NEW
{
    PATROL_READ_FIRST_AFTER_BOOT,
    PATROL_READ_TIME_INTERVAL,
    PATROL_READ_GET_BLOCK,
    PATROL_READ_PAGE_READ,
    PATROL_READ_WAIT_PAGE,
    PATROL_READ_CHECK_STATUS,
    PATROL_READ_BLOCK_DONE,
    PATROL_READ_DISK_DONE
} PATROL_READ_STAGE_NEW;

typedef enum _PATROL_READ_RETURN_STS
{
    PATROL_READ_WAIT,
    PATROL_READ_ONE_PAGE_DONE,
    PATROL_READ_ONE_BLK_DONE,
    PATROL_READ_ONE_SCAN_DONE,
    PATROL_READ_WAIT_TIME_INTERVAL
} PATROL_READ_RETURN_STS;

typedef enum _IDLE_GC_STS
{
    IDLE_GC_PREPARE,
    IDLE_GC_CHECK
}IDLE_GC_STS;

/*Patrol relative define end*/


extern GLOBAL  L2_FTLDptr *g_FTLDptr;
extern GLOBAL  L2_FTLReadDptr(*g_FTLReadDptr)[LUN_NUM_PER_SUPERPU];
extern GLOBAL  PMTFlushStatistic *g_PMTFlushManager;
extern GLOBAL FTL_ERASE_STATUS_MGR * l_ptFTLEraseStatusManager;

extern GLOBAL U32 g_ulPMTFlushRatio;
extern void L2_SetThreadQuota(U8 ucSuperPu, SYS_THREAD_TYPE eThreadSet, U32 ulQuota);

void L2_InitStatistic(void);
void L2_InitFTLDptr(void);
void L2_IncDeviceWrite(U16 PUSer, U32 PageCnt);

U8 L2_GetPuFromAddr(PhysicalAddr* pAddr);
void L2_UpdateFlushPos(U16 PUSer, U16 Pos);
BOOL L2_IsNeedFlushPMT(U16 PUSer);
BOOL L2_IsFTLIdle(void);
void L2_PrintFlushPos(void);
void FillSpareAreaInWriteTable(RED* pSpare, U8 ucSuperPu, U8 ucLUNInSuperPU, PAGE_TYPE PageType, TargetType FlushType);
void L2_PopBufREQ(U8 ucPU);
void L2_RecordFlushPMTIndex(U32 PUSer);

void L2_FillRedForEM(U32 *pTargetRed, U32* pSpare,U32 ulDW);
void L2_FtlWriteNormal(BUF_REQ_WRITE *pReq, PhysicalAddr *pAddr, U32 *pSpare, BOOL bsDSLCMode);
U32 L2_FtlReadMerge(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, BUF_REQ_READ* pReq, PhysicalAddr* pAddr);
U32 L2_FtlReadNormal(U8 ucSuperPu, U8 ucLunInSPU, U32 ulLunAllowToSendFcmdBitmap, BUF_REQ_READ* pReq, PhysicalAddr* pAddr);
void L2_FtlWriteLocal(PhysicalAddr* pAddr, U32* pPage, U32* pSpare, U8* pStatus, BOOL bsTableReq, BOOL bSLCMode, XOR_PARAM *pXorParam);
void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode);
void __L2_FtlReadLocal(U32* pBuffAddr, PhysicalAddr* pAddr, U8* pStatus,
    U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode, BOOL bPatrol);
void L2_FtlEraseBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, BOOL bNeedL3ErrorHandle);
void L2_EraseStatusDramAllocate(U32* pFreeDramBase);
void L2_FTLErase(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U16 usVBN);
void L2_FTLEraseStsManagerInit(void);
U8 * L2_FTLGetCurEraseStsAddr(U8 ucSuperPu, U8 ucLUN);
void L2_FTLRecordEraseStsInfo(U8 ucSuperPu, U16 usVBN);
BOOL L2_FTLCheckEraseSts(U8 ucSuperPu);
BOOL L2_FTLWaitAllEraseDone(U8 ucSuperPu);
BOOL L2_IsPUNeedSchedule(U8 ucPuNum);
L2_FTL_STATUS  L2_DummyWrite(U8 ucPu, TargetType Type);
BOOL L2_FlushPMTPageForRebuild(U32 ulSuperPu, U32* pPMTIndexInPu);
BOOL L2_PopReadBufREQ(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
BOOL L2_FTLEntry(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
void L2_InitFTLScheduler(U8 ucSuperPu);
void L2_FTLUsedThreadQuotaMax(U8 ucSuperPu, FTL_THREAD_TYPE CurrThread);
void L2_SetIdleGC(U8 ucSuperPu, BOOL bVal);
void L2_FTLSetThreadQuota(U8 ucSuperPu, FTL_THREAD_TYPE CurrThread, U32 val);
void L2_ClearAllPUFTLScheduler(void);
void L2_AddPendingWordline(U8 ucSuperPu, U8 ucLunInSuperPu, PhysicalAddr PhyAddr, U16 usPhyBufId, U32 ulCachestatusAddr);
void L2_ScanBlockN1MgrDramAllocate(U32* pFreeDramBase);
void L2_PatrolReadMgrDramAllocate(U32* pFreeDramBase);
void L2_PatrolReadMgrInit(void);
U8 L2_PatrolRead(void);
void L2_EraseTLCBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE, U8* pStatus, BOOL bNeedL3ErrorHandle);
U16 L2_FTLGetUsedThreadQuota(U8 ucSuperPu);
U16 L2_FTLGetThreadQuotaMax(U8 ucSuperPu, FTL_THREAD_TYPE CurrThread);
extern void L2_FtlTLCExternalWriteLocal(PhysicalAddr* pAddr, U32* pPBuffer, U32* pSpare, U8* pStatus, XOR_PARAM *pXorParam);
extern void L2_FtlTLCInternalWriteLocal(PhysicalAddr *pSrcAddr,PhysicalAddr *pDesAddr, U8* pStatus);

void L2_FTLTaskSLCGCClear(U8 ucSuperPu);
void L2_FTLTaskTLCMergeClear(U8 ucSuperPu);
void L2_FTLTaskTLCGCClear(U8 ucSuperPu);
void L2_FTLTaskTLCSWLClear(U8 ucSuperPu);
U8 L2_FTLGetTaskBitInfo(U8 ucSuperPu);
BOOL L2_FTLIsTaskSet(U8 ucSuperPu, U8 TaskBitOffset);

void L2_FTLIncIdleGCQuotaCnt(U8 ucSuperPu, FTL_THREAD_TYPE tThread, U16 usVal);
BOOL L2_FTLIsIdleTaskDone(void);
BOOL L2_IdleGCEntry(void);
void L2_IdleGCResetManager(void);
#endif
