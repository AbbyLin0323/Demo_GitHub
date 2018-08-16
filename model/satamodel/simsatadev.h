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

#ifndef __SIM_SATA_H__
#define __SIM_SATA_H__

#include "simsatainc.h"
#include "HAL_SataDSG.h"
//#include <windows.h>


#ifdef SIM_DBG
#define SDC_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun, TRACE_SATADEVICE_MODULE_ID,TRACE_SATADEVICE_SUBMODULE_ID,log_level,x, __VA_ARGS__);
#else
#define SDC_LogInfo(x, ...)  (void)0
#endif



//extern SATA_PRD_ENTRY* g_pSDCReadPrd;
//extern SATA_PRD_ENTRY* g_pSDCWritePrd;
//extern U32 cachep_p_cache_base_addr;
//extern U8 sim_cur_cmd_tag;

// data check
U32 SDC_DataCacl(U32 lba, U32 cnt);
BOOL SDC_DataCmp(U32 lba, U32 nReadCnt, U32 nReadLba);




void SDC_LchShadowReg();

U8 SDC_GetSectorsInPage();
BOOL SDC_ProcessWritePrd(U16* pseccnt);
BOOL SDC_ProcessReadPrd(U16* psecnt);
void SDC_ModelInit();
void SDC_ModelSchedule();
BOOL SDC_DataTransfer(U32 fk_bit_msk, U8 cmd_tag, SATA_DSG *p_prd);
BOOL SDC_ParsingOnePrd(SATA_DSG *pDsg, U8 uCmdTag);
//BOOL SDC_ParsingOneWritePrd(SATA_PRD_ENTRY * p_wprd, U8 cmd_tag);
BOOL SDC_CmdTrigger();

U32 GetBitMsk(SATA_PRD_BUF_INFO *pBufInfo);
U32 GetBitMap(SATA_DSG *pDsg);

void SDC_ModelProcess();

#ifdef SIM
DWORD WINAPI SDC_ModelThread(LPVOID p);
#endif

#ifdef SIM_XTENSA
void SDC_ModelThread_XTENSA();
#endif


extern CRITICAL_SECTION g_CMDQueueCriticalSection;

extern void SDC_SetD2HFis();
//extern void Init_sata(DWORD dwCapacity);
//extern BOOL m2f();
//extern BOOL ftom();
#endif
