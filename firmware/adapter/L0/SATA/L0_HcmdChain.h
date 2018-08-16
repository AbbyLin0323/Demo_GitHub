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
Filename    :L0_HcmdChain.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_HCMD_CHAIN_H__
#define __L0_HCMD_CHAIN_H__

#include "BaseDef.h"
#include "L0_ATALibAdapter.h"

#define L0_HOSTCOMMAND_DEBUG

#ifdef ASIC
#ifdef HCLK300M
#define HCMDQ_FORCE_SELECT_LATENCY  (60*1000*1000) // 200ms
#else
#define HCMDQ_FORCE_SELECT_LATENCY  (40*1000*1000)
#endif
#else
#define HCMDQ_FORCE_SELECT_LATENCY  (200)
#endif

typedef struct _HCMD
{
    U8 ucNCQSubCmd;//for NCQ SEND/RECEIVE/NONDATA command defined in SATA 3.2 spec
    U8 ucSubCmdSpec; 
    
    U32 ulStartLba;

    U32 ulRecvTime;

    struct _HCMD* pNextHCMD;

    CB_MGR  tCbMgr;
    CFIS    tCFis;
    DUMMY_COMMAND_HEADER  tCh;
} HCMD;

extern HCMD HostCmdSlot[MAX_SLOT_NUM];
extern HCMD* g_pCurHCMD;

extern void L0_HostCommandInit(void);
extern void L0_AddNewHostCmd(U8 ucNewCmdTag);
extern BOOL L0_HostCmdChainEmpty(void);
extern BOOL L0_IsHcmdNeedForceSelect(void);
extern U8 L0_ForceSelectHcmdHead(void);
extern U8 L0_SelectHcmdFromChain(void);
extern HCMD* L0_HostCmdSelect(void);
extern void L0_SetHcmdLastAccessedLba(HCMD* pHCMD);
extern U8 L0_GetLocalPuFromLba(U32 ulGlobalLba);

#endif
