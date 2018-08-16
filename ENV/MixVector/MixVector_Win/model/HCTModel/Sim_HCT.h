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

Filename     :   Sim_HCT.h                                         
Version      :   0.1                                              
Date         :   2014.11.10                                         
Author       :   NinaYang

Description:  implement the function of HCT
Others: 
Modification History:
2014.11.10   create
*******************************************************************/
#ifndef SIM_HCT_H
#define SIM_HCT_H
#include "BaseDef.h"
#include "HAL_HostInterface.h"
#ifdef HOST_NVME
#include "NVME_ControllerModel.h"
#include "NVME_HostCommand.h"
#else
#include "AHCI_HostModelVar.h"
#endif


#define CST_STATUS_COUNT    11
typedef struct _DebugCST_Trace
{   U8  ucPtr;
    U8  tCSTValue[CST_STATUS_COUNT];
}DebugCST_Trace;

extern HCT_MGR g_tHCTMgr;
extern const U8 g_uWBQDepth;
extern U32 g_ulHCTSramStartAddr;
extern U32 g_ulCFisBaseAddr;
extern U32 g_ulCmdHeaderBaseAddr;
extern PHCT_BAINC_REG g_pHCTBaIncReg;
extern CRITICAL_SECTION g_csFCQCriticalSection;

/*HCT function*/
void HCT_ResetOPT();
void HCT_ModelInit();
void HCT_SearchCST();
void HCT_GetCST(U8 CSTID, U8* pCurrentValue);
void HCT_SetCSTByWBQ(U8 CSTID, U8 CurrentValue, U8 NextValue);
void HCT_SetCSTByFw(U8 CSTID, U8 CurrentValue, U8 NextValue);
U32  HCT_GetDeviceAddr(U8 IDBaseEnable, U8 SN, U16 Offset, U8 ID);
void SIM_HCTRegInit();
void HCT_CSTInit();

/*FCQ funtion*/
void SIM_HostCModelTriggerFCQWrite();
void SIM_HostCFCQWriteTrigger();
BOOL SIM_HostCFCQThreadExit();

/*WBQ funtion*/
void SIM_HostCWBQStateTrigger( U8 ucCmdTag );
BOOL SIM_HostCWBQProcessByEvent( U32 ulEventType );
BOOL SIM_HostCHandleCmdWBQ(U8 CmdTag);
BOOL SIM_HostCWBQThreadExit();

void SIM_HostCModelSchedule();
void SIM_HostCModelPowerUp();

#endif