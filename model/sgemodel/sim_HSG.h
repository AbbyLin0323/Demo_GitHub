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
#ifndef __SIM_HSG_H__
#define __SIM_HSG_H__

#include "BaseDef.h"

#include "HAL_MemoryMap.h"
//#include "sim_flash_config.h"
#include "model_common.h"
#include "HAL_HSG.h"



//HSG interface
void HSG_InitHsg(void);
BOOL HSG_AllocateHsg(U8 McuId);
void HSG_SetHsgValid(U16 HsgId);
void HSG_SetHsgInvalid(U16 HsgId);

//for model use
BOOL HSG_IsHsgValid(U16 HsgId);
void HSG_ReleaseHsg(U16 HsgId);
void HSG_FetchHsg(U16 HsgId, HSG_ENTRY * PHsg);

void Comm_HsgModelInit(void);


BOOL HsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize);
#endif

