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
#ifndef __SIM_SATA_HSG_H__
#define __SIM_SATA_HSG_H__

#include "BaseDef.h"

//#include "HAL_SataDSG.h"
#include "HAL_MemoryMap.h"
//#include "sim_flash_config.h"
#include "model_common.h"
#include "HAL_SATADSG.h"

//sata DSG interface
void DSG_InitSataDsg(void);
BOOL DSG_AllocateSataReadDsg1(void);
BOOL DSG_AllocateSataReadDsg2(void);
BOOL DSG_AllocateSataWriteDsg1(void);
BOOL DSG_AllocateSataWriteDsg2(void);
void DSG_SetSataDsgValid(U16 DsgId);
void DSG_SetSataDsgInvalid(U16 DsgId);

//for model use
BOOL DSG_IsSataDsgValid(U16 DsgId);
void DSG_ReleaseSataDsg(U16 DsgId);
void DSG_FetchSataDsg(U16 DsgId, SATA_DSG *PSataDsg);

void Comm_SataDsgModelInit(void);
#endif
BOOL SataHsgRegWrite(U32 regaddr, U32 regvalue, U32 nsize);
