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
  File Name     : L2_SimTablecheck.c
  Version       : Ver 1.0
  Author        : henryluo
  Created       : 2015/02/28
  Description   : 
  Function List :
  History       :
  1.Date        : 2015/02/28
    Author      : henryluo
    Modification: Created file

*******************************************************************************/
#ifndef __TABLE_REBUILD_CHECK_H__
#define __TABLE_REBUILD_CHECK_H__

#ifdef SIM

#include "Basedef.h"
#include "L2_PMTManager.h"
#include "L2_PBIT.h"
#include "L2_VBT.h"
#include "L2_PMTI.h"
#include "L2_StaticWL.h"
#include "L2_FTL.h"

#define  FW_MCU_COUNT 1

#define IGNORE_LPN_CNT 1024
struct SimRstLPNOfPMTCheck
{
    U32 LPNCnt;
    U32 IgnoredLPN[1024];
    U32 ErrAddrCnt;
    PhysicalAddr ErrAddr[IGNORE_LPN_CNT/LPN_PER_BUF];
};

struct SimRstBlkOfVBTCheck
{
    U16 IgnoredBlk[1024];
    U32 BlkCnt;
};

struct SimRstPMTIOfPMTICheck
{
    U16 IgnoredPMTI[1024];
    U16 Cnt;
};

extern GLOBAL MCU12_VAR_ATTR PBIT *dbg_pPBIT[SUBSYSTEM_LUN_MAX];
extern GLOBAL MCU12_VAR_ATTR VBT *dbg_pVBT[SUBSYSTEM_LUN_MAX];
extern GLOBAL MCU12_VAR_ATTR U32 * DbgForFreeBlockCnt;
extern GLOBAL MCU12_VAR_ATTR PMTI* dbg_g_PMTI[SUBSYSTEM_LUN_MAX];
extern GLOBAL MCU12_VAR_ATTR PuInfo* dbg_g_PuInfo[SUBSYSTEM_LUN_MAX];
extern GLOBAL MCU12_VAR_ATTR PMTBlkManager* dbg_p_m_PMTBlkManager[SUBSYSTEM_LUN_MAX];
extern struct SimRstLPNOfPMTCheck g_SimRstLPNOfPMTCheck;
extern GLOBAL MCU12_VAR_ATTR WL_INFO *dbg_gwl_info;
extern GLOBAL MCU12_VAR_ATTR FTL_ERASE_STATUS_MGR * dbg_g_ptFTLEraseStatusManager;

void L2_ResetIgnoredLPN_WriteCnt();
void L2_AddIgnoredLPNOfCheckPMT(U32 LPN);
void L3_AddIgnoredBlkOfCheckVBT(U8 CE,U16 Blk);
void L3_UpdateIgnoredBlkOfCheckVBT(U32* pLPN,U32 Cnt);
BOOL L2_FindIgnoredPMTICheck(U32 PMTI);
void Dbg_Record_TableRebuild_Info();
void L3_ResetIgnoredBlkOfCheckVBT();
void L2_ResetIgnoredLPNOfCheckPMT();
void L2_AddErrAddrOfCheckPMT(U8 PUSer,U16 VirBlk,U16 Pg);
void L2_RemoveErrAddrOfCheckPMT(U8 PUSer,U16 VirBlk);
void L2_ResetDbgPMTByPhyAdrr(U8 PUSer, U16 PhyBlk, U16 PhyPage);
void L2_RemoveErrAddrOfCheckPMT(U8 PUSer, U16 VirBlk);
BOOL L2_IsNeedIgnoreCheck(U32 LPN, PhysicalAddr* pAddr);
void L2_ResetCollectErrAddr();
void L2_CollectErrAddrOfCheckPMT(U8 ucSubSystemPu, U16 PhyBlk, U16 PhyPage);

void L2_AdjustDebugPMT();
void L3_RecordBlakePuInfo();
void L3_RecordPuInfo();            
void L2_RecordPMTManager();
void L3_RecordPBIT();
void L3_RecordVBT();
void L2_RecordPMT();
void L2_RecordPMTI();
void L2_RecordEraseStatusMangerInfo(void);

void L2_CheckPMT();
U32 L2_CheckPMTI();
U32 L2_CheckPMTManager();
void L3_CheckPBIT();
void L3_CheckPuInfo();
void L3_CheckPBITwithPuInfo();
void L3_CheckVBT();
void L2_CheckVBTMapping(U8 ucSuperPU);

#endif // end #ifdef SIM

#endif