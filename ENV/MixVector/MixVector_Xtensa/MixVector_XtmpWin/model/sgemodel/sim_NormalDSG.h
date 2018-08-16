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
#ifndef __SIM_NORMAL_DSG_H__
#define __SIM_NORMAL_DSG_H__

#include "BaseDef.h"
#include "HAL_NormalDSG.h"
#include "HAL_MemoryMap.h"
#include "model_common.h"

//normal DSG interface
void DSG_InitNormalDsg(void);
BOOL DSG_AllocateNormalDsg(U8 McuId);
void DSG_SetNormalDsgValid(U16 DsgId);
void DSG_SetNormalDsgInvalid(U16 DsgId);

//interface for NFC model use
BOOL DSG_IsNormalDsgValid(U16 DsgId);
void DSG_ReleaseNormalDsg(U16 DsgId);
void DSG_FetchNormalDsg(U16 DsgId, NORMAL_DSG_ENTRY *PNormalDsg);

void Comm_NormalDsgModelInit(void);

BOOL NormalDsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize);
#endif

