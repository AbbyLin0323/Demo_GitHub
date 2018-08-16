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
*******************************************************************************/

#ifndef COM_MAINSCHE_H
#define COM_MAINSCHE_H

#include "BaseDef.h"

#define DRAM_ALLOCATE_SIZE  (640 * 1024 * 1024)

BOOL SIM_SubSystemIsPowerUp(U32 ulMcuID);
void SIM_MCU1ThreadHandler(void);
void SIM_SystemRegInit();
void SIM_StartMcuThread();
void SIM_DevSetStatus(U32 ulStatus);
BOOL SIM_MCUIsAllExit(void);
void SIM_WaitMCUExit(void);

void SIM_MCUSchedule(U32 ulMcuID);
void Sim_ClearDram();

void SIM_EventInit(void);
void SIM_EventHandler(void);
BOOL SIM_SubSystemIsPowerUp(U32 ulMcuID);

void SIM_SetReportWLstatistic();
void SIM_ClearReportWLstatistic();
BOOL SIM_CheckReportWLstatistic();

void SIM_DevWLStatistic(U32 ulMcuID);



#endif