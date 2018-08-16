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
File Name     : L2_RT.h
Version       : Initial version
Author        : henryluo
Created       : 2015/02/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2012/09/10
Author      : AwayWei
Modification: Created file

*******************************************************************************/

#ifndef _L2_RT_H
#define _L2_RT_H

#include "L1_GlobalInfo.h"
#include "L1_Cache.h"
#include "FW_Debug.h"
#include "L2_PMTI.h"
#include "L2_VBT.h"
#include "L2_PBIT.h"
#include "L2_GCManager.h"

#include "L3_FlashMonitor.h"

#define L2_RT_FAIL    0
#define L2_RT_SUCCESS 1
#define L2_RT_WAIT    2

typedef enum _L2_RT_LOAD_STATUS
{
    L2_RT_LOAD_START = 0,
    L2_RT_LOAD_SEARCH_BLOCK,
    L2_RT_LOAD_CHECK_BLOCK,
    L2_RT_LOAD_SEARCH_PAGE,
    L2_RT_LOAD_CHECK_PAGE,
    L2_RT_LOAD_READ_RT,
    L2_RT_LOAD_WAIT_FINISH,
    L2_RT_LOAD_FAIL,
    L2_RT_LOAD_SUCCESS
}L2_RT_LOAD_STATUS;

typedef enum _L2_RT_CHECK_STATUS
{
    L2_RT_CHECK_START = 0,
    L2_RT_CHECK_READ_AT0_PPO,
    L2_RT_CHECK_WAIT_AT0_PPO,
    L2_RT_CHECK_READ_AT1_PPO,
    L2_RT_CHECK_WAIT_AT1_PPO,
    L2_RT_CHECK_FAIL,
    L2_RT_CHECK_SUCCESS
}L2_RT_CHECK_STATUS;

typedef enum _L2_RT_SAVE_STATUS
{
    L2_RT_SAVE_START = 0,
    L2_RT_SAVE_WRITE_PAGE,
    L2_RT_SAVE_WAIT_PAGE,
    L2_RT_SAVE_WAIT_ERASE,
    L2_RT_SAVE_FAIL,
    L2_RT_SAVE_SUCCESS
}L2_RT_SAVE_STATUS;

typedef struct _RT_MANAGEMENT
{
    L2_RT_LOAD_STATUS  LoadStatus[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    L2_RT_CHECK_STATUS CheckStatus[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    U8 ucFlashStatus[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    U16 usRTBlock[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    RED RT_Red[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
} RT_MANAGEMENT;

/*Root Table */
typedef struct _ROOT_TABLE_ENTRY
{
    /* PMTI Location Info */
    U16 PMTIBlock[LUN_NUM_PER_SUPERPU][PMTI_SUPERPAGE_COUNT_PER_SUPERPU];
    U16 PMTIPPO[PMTI_SUPERPAGE_COUNT_PER_SUPERPU];

#ifndef LCT_VALID_REMOVED
    /* VBMT Location Info */
    U16 VBMTBlock[LUN_NUM_PER_SUPERPU][VBMT_SUPERPAGE_COUNT_PER_SUPERPU_MAX];
    U16 VBMTPPO[VBMT_SUPERPAGE_COUNT_PER_SUPERPU_MAX];
    //U16 *VBMTBlock;
    //U16 *VBMTPPO;
#endif

    /* VBT Location Info */
    U16 VBTBlock[LUN_NUM_PER_SUPERPU][VBT_SUPERPAGE_COUNT_PER_SUPERPU];
    U16 VBTPPO[VBT_SUPERPAGE_COUNT_PER_SUPERPU];

    /* PBIT Location Info */
    U16 PBITBlock[LUN_NUM_PER_SUPERPU][PBIT_SUPERPAGE_COUNT_PER_SUPERPU];
    U16 PBITPPO[PBIT_SUPERPAGE_COUNT_PER_SUPERPU];

    /* DPBM Location Info */
    U16 DPBMBlock[LUN_NUM_PER_SUPERPU][DPBM_SUPERPAGE_COUNT_PER_SUPERPU];
    U16 DPBMPPO[DPBM_SUPERPAGE_COUNT_PER_SUPERPU];

    /* RPMT Location Info */
    U16 RPMTBlock[LUN_NUM_PER_SUPERPU][RPMT_SUPERPAGE_COUNT_PER_SUPERPU];
    U16 RPMTPPO[RPMT_SUPERPAGE_COUNT_PER_SUPERPU];

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    /* TLCW Location Info */
    U16 TLCWBlock[LUN_NUM_PER_SUPERPU][TLCW_SUPERPAGE_COUNT_PER_SUPERPU];
    U16 TLCWPPO[TLCW_SUPERPAGE_COUNT_PER_SUPERPU];
#endif

    /* Area 0 Table Block management */
    U16 CurAT0Block[LUN_NUM_PER_SUPERPU];
    U16 CurAT0PPO;

    U16 AT0BlockAddr[LUN_NUM_PER_SUPERPU][AT0_BLOCK_COUNT];
    U16 AT0BlockEraseFlag[LUN_NUM_PER_SUPERPU][AT0_BLOCK_COUNT];

    /* Area 1 Table Block management */
    U16 CurAT1Block[LUN_NUM_PER_SUPERPU];
    U16 CurAT1PPO;

    U16 AT1BlockAddr[LUN_NUM_PER_SUPERPU][AT1_BLOCK_COUNT];
    U16 AT1BlockDirtyCnt[AT1_BLOCK_COUNT];
    U16 AT1BlockFreeFlag[AT1_BLOCK_COUNT];

    /* Trace Block management */
    U16 TraceBlockAddr[LUN_NUM_PER_SUPERPU][TRACE_BLOCK_COUNT];

    /* Max TS of current PU */
    U32 ulMaxTimeStamp;

    /* flash monitor */
    FM_USER_ITEM m_FlashMonitorItem;
}ROOT_TABLE_ENTRY;

typedef struct _ROOT_TABLE
{
    HOST_INFO_PAGE    m_HostInfo;
    DEVICE_PARAM_PAGE m_DevParam;

    TRACE_BLOCK_MANAGEMENT m_TraceBlockManager;

    ROOT_TABLE_ENTRY m_RT[SUBSYSTEM_SUPERPU_MAX];

} RT;

#define RT_SIZE (U32)sizeof(RT)
#define RT_PAGE_COUNT ((RT_SIZE % BUF_SIZE) ? (RT_SIZE/BUF_SIZE + 1) : (RT_SIZE/BUF_SIZE))

extern GLOBAL  RT *pRT;
extern GLOBAL  RT_MANAGEMENT *pRTManagement;
extern GLOBAL  PhysicalAddr RootTableAddr[RT_PAGE_COUNT];

void L2_RT_Init_Clear_All(void);
void L2_RT_Init(void);
void L2_RT_Init_TableLocation(void);
void L2_RT_Init_AT0AT1Infor(void);
void L2_RTManagement_Init(void);
void L2_RT_Rebuild_MarkTable(U8 ucSuperPuNum, U8 ucLunInSuperPu);
void L2_RT_ResetSaveStatus(void);
void L2_RT_ResetLoadStatus(void);
void L2_RT_ResetCheckStatus(void);
U8   L2_RT_GetAT1BlockSNFromPBN(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usBlock);
U16  L2_RT_GetAT1BlockPBNFromSN(U8 ucSuperPuNum, U8 ucLunInSuperPu, U8 ucBlockSN);
void L2_RTPrepareSysInfoAT1(void);
void L2_RTPrepareSysInfoFlashMonitor(void);
void L2_RTPrepareSysInfoTrace(void);
void L2_RTPrepareSysInfo(void);
void L2_RTResumeSysInfoAT1(void);
void L2_RTResumeSysInfoFlashMonitor(void);
void L2_RTResumeSysInfoTrace();
void L2_RTResumeSysInfo();
U16 L2_RT_Search_BlockBBT(U8 ucLunNum, U16 usNoneBadPos, BOOL bHead);
U16 L2_RT_FindNextGoodBlock(U8 ucLunNum, U16 usCurBlockPos, BOOL bHead);
U32 L2_RT_LoadRT(U8 ucSuperPuNum, U8 ucLunInSuperPu);
U32 L2_RT_CheckRT(U8 ucSuperPuNum, U8 ucLunInSuperPu);
void L2_RT_Save_WritePage(BOOL bPowerCycle, U8 ucPageIndex);
U32 L2_RT_SaveRT(BOOL bPowerCycle);

#endif

/********************** FILE END ***************/
