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
Filename    :HostModel.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef _HOST_Model_H
#define _HOST_Model_H

#include "BaseDef.h"
#include <windows.h>
#include "system_statistic.h"
#include "hscmd_parse.h"

//Model process
void Host_FunctionMenu();
void Host_ModelInit();
void Host_ModelSchedule();
BOOL Host_ModelProcess();
void Host_CmdCompleted( U8 ucTag );

//Data Check Section
void Host_DataFileDel();
void Init_sata(DWORD dwHowManyArray);
void Host_OpenDataFile();
void Host_CloseDataFile();
BOOL Host_SaveDataToFile();
BOOL Host_GetDataFromFile();
U32 Host_UpdateDataCnt(U32 lba);
U32 Host_GetDataCnt(U32 lba);
U32 Host_SaveDataCnt(U32 lba, U32 cnt);

void Host_NoneDataCMDFinish();

BOOL Host_IsCIClear( void );
BOOL Host_IsCMDEmpty();
BOOL Host_SendOneCmd(HCMD_INFO* pHcmd);

#endif
