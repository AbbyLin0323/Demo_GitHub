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

#ifndef SIM_FLASH_COMMON_H
#define SIM_FLASH_COMMON_H

#include "BaseDef.h"
#include "sim_flash_config.h"
#include "sim_flash_shedule.h"
#ifdef SIM
#include "flash_meminterface.h"
#else
#include "hfmid.h"
#endif

#include "sim_NormalDSG.h"

NFCQ_ENTRY* Comm_GetCQEntry(U8 pu, U8 rp);
NFC_PRCQ_ENTRY * Comm_GetPRCQEntry(U8 Pu, U8 Rp);

void NFC_ModelProcess();
void NFC_ModelSchedule();
DWORD WINAPI NFC_ModelThread(LPVOID p);

void Comm_NFCModelInit(void);
void Comm_GetRedSize();

#endif