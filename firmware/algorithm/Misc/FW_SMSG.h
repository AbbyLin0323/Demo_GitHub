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
Filename    :FW_SMSG.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/

#ifndef _FW_SMSG_H
#define _FW_SMSG_H

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "L0_Interface.h"

extern MCU12_VAR_ATTR volatile PMCSD g_apSubMcShareData;

extern void FW_InitSMSG(void);
extern void FW_InitMSD(void);
extern BOOL FW_IsSMQEmpty(void);
extern void FW_ReportSMSG(PSMSG pSMSG);
extern void FW_ChkNtfnMsg(void);

#endif

