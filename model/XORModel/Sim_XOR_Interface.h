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
* NON-INFRINGEMENT.
********************************************************************************
Description :
*******************************************************************************/
#ifndef __SIM_XOR_INTERFACE_H__
#define __SIM_XOR_INTERFACE_H__

#include "BaseDef.h"
#include "Sim_XOR_Common.h"

void XORM_IInit(void);
void XORM_ITrigger(U32 ulXoreId);

void XorM_IReceiveConfigFromNfc(U32 ulXoreId, U32 ulBufferId, U32 ulRedunLength, BOOL bDramCrcEn);
void XorM_IReceiveCwConfig(U32 xore_id, U32 code_word_index, U32 buffer_id, U32 atom_redun_length,
                           BOOL dram_crc_en);
void XorM_ICaptureDramData(U32 *dram_data, U32 size_in_dw);
void XORM_ICalculateNFC(U32 ulXoreId, const U32 * pData, U32 ulCodeWordIndex);
BOOL XORM_IIsParityReady(U32 ulXoreId);
void XorM_IReleaseXore(U32 ulXoreId);

void XORM_BypassNfcMode(U32 ulXoreId);
void XORM_LoadStoreMode(U32 ulXoreId);

#endif // __SIM_XOR_INTERFACE_H__