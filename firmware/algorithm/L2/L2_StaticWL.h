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
Filename    :L2_StaticWL.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.31
Description :defines for Static WL Data Structure
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_STATIC_WL_H__
#define __L2_STATIC_WL_H__

#include "Disk_Config.h"
#include "L2_Defines.h"
#include "L2_Init.h"
#include "L2_GCManager.h"

#define READ 1
#define WRITE 0
#define MEET_CON_CNT 10
#define HIT_CON_SWL_CNT 10
#define ACCSWL_MAX_DEGREE 7
//trigger and src blk selection thresholds for new SWL 
#define SWL_TH_MAX2MIN 60
#define SWL_TH_AVE2MIN 50


typedef struct _WL_INFO
{
    GCComStruct tSWLCommon;
    
    BOOL bForbidSpeedUp[SUBSYSTEM_SUPERPU_MAX];

    U16 nDstBlk[SUBSYSTEM_SUPERPU_MAX];
    U16 nDstPPO[SUBSYSTEM_SUPERPU_MAX]; //For debug table rebuild
    BOOL bEnable[SUBSYSTEM_SUPERPU_MAX];
    U32 nOTimes[SUBSYSTEM_SUPERPU_MAX];
    BOOL bTooCold[SUBSYSTEM_SUPERPU_MAX];
    U16 nDstBlkBuf[SUBSYSTEM_SUPERPU_MAX];
    U16 nSrcBlkBuf[SUBSYSTEM_SUPERPU_MAX];

    //add for super page,0:not read/write,1:has been read/written
#ifdef SWL_EVALUATOR
    U32 ulSelSrcECminCnt[SUBSYSTEM_SUPERPU_MAX];
#endif
}WL_INFO;

enum {
    STATIC_WL = 0,
    DATA_GC,
    TABLE_GC,
    ALL_INFO
};
typedef struct LOCK_COUNTER
{
    U16 uLockCounter[SUBSYSTEM_LUN_MAX][ALL_INFO];
}LC;

extern GLOBAL MCU12_VAR_ATTR BOOL gbWLTraceRec;
extern GLOBAL  WL_INFO* gwl_info;
BOOL L2_StaticWLEntry(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
void L2_SWLInit(void);
void L2_IsNeedWL(U32 EraseCnt, U8 ucSuperPu,U8 ucLunInSuperPu, U32 uVBN);
BOOL L2_WL_Erase(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
void L2_SelectSWLSrcBlk(U8 ucSuperPu);
void L2_SWLClear(U8 ucSuperPu, BOOL bInit);
U8 LimitValueSetting(U16 EraseCnt, U16 AveEC);
BOOL L2_IsWL(U8 ucSuperPu);
void L2_SWLUnlockTargetBlkBuf(U8 ucSuperPu);
extern void L2_SetSWLInfoAfterBoot(U8 ucSuperPu);


extern BOOL L2_ProcessTLCGC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucThreadType, U8 ucWriteType);



#endif

