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
*Filename: defines.h
*Version: 1.0
*Date: 20120208
*Author: Catherine Li
*
*Description: Flash Interface for visiting registers
*
*Modification History:
*catherineli,       20120208,      first created
*************************************************/
#ifndef __SIM_FLASH_INTERFACE_H__
#define __SIM_FLASH_INTERFACE_H__
#include "sim_flash_config.h"
#include "HAL_LdpcSoftDec.h"

//CMD CODE
/************cmd code**************************/
//#define NF_CMD_READRETRY_ADJ_START FLASH_PRCQ_READRETRY_ADJUSTMENT_0
#define NF_CMD_READRETRY_ADJ_START  NF_PRCQ_RETRY_ADJ0
#define NF_CMD_READRETRY_ADJ_END (NF_PRCQ_RETRY_EN-1)
#define NF_CMD_SHIFTREAD_ADJ_START NF_CMD_READRETRY_ADJ_START
#define NF_CMD_SHIFTREAD_ADJ_END NF_CMD_READRETRY_ADJ_START

#pragma pack(push, 1)
typedef struct _ST_BMID_LVL
{
    U8 bmid     : 6;
    U8 bmenw    : 1;
    U8 bmls     : 1;
}ST_BMID_LVL;
#pragma pack (pop)

#define CQENTRY_BMOFF_OFFSET            12
#define CQENTRY_BUFID_OFFSET            8

#define CQDPTR_DW1_OFFSET               1
#define CQDPTR_BYTE_CMD0_OFFSET         6
#define CQDPTR_BYTE_CMD0_DW_OFFSET      2
#define CQDPTR_BYTE_CMD1_OFFSET         7
#define CQDPTR_BYTE_CMD1_DW_OFFSET      3
#define CQDPTR_BYTE_RST_OFFSET          1
#define CQDPTR_BYTE_BM_OFFSET           5
#define CQDPTR_BYTE_BM_DW_OFFSET        1
#define CQDPTR_BIT_DW1_CMD0_OFFSET      16
#define CQDPTR_BIT_DW0_RESET0_OFFSET    (1<<15)
#define CQDPTR_BIT_DW0_RESET1_OFFSET    (1<<14)

extern U32 sim_bufmap[64];

/************************************************
Function define
*************************************************/
void NFC_InterfaceUpdateHwPuSts(void);
void NFC_InterfaceUpdateLogicPuSts(const NFCM_LUN_LOCATION *tNfcOrgStruct);
BOOL NFC_GetEmpty(const NFCM_LUN_LOCATION *tNfcOrgStruct);
U8 NFC_GetRP(const NFCM_LUN_LOCATION *tNfcOrgStruct);
U8 NFC_InterfaceCQWP(const NFCM_LUN_LOCATION *tNfcOrgStruct);
U8 NFC_GetErrH(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NFC_InterfaceSetFirstDataReady(U8 tag);
void NFC_InterfaceSetBuffermap(U8 ucLunInTotal, U32 buffMapvalue, U32 Bmid);
void NFC_InterfaceClearCacheBusy(U32* buf_id_addr, U32 sec_en_low, U32 sec_en_high);
void NFC_InterfaceClearSSU(U32 ssu_addr,U8 value);
void NFC_InterfaceJumpCQRP(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NFC_InterfaceJumpCQWP(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NFC_InterfaceSetInt(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NFC_InterfaceResetCQ(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 level);
#ifdef HOST_SATA
void SDC_SetReadBufferMapWin(U8 buffmap_id,U32 buffermap_value);
#endif
U32 SDC_GetReadBufferMap(U8 buffmap_id);
void NFC_InterfaceRecvBufferMapID(U8 pu);
void nfcReadReg(U32 addr, U32 nBytes, U8 *dst);
void nfcWriteReg(U32 addr, U32 nBytes, const U8*src);
void nfcReadDram(U32 addr, U32 nWordss, U8 *buf);
void nfcWriteDram(U32 addr, U32 nWordss, const U8* src);
void NFC_InterfaceSetErrParameter(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 nErrType);

void NFC_InterfaceUpdateSsu(U16 ssu_addr, U8 value, U8 index, BOOL ontf);
void NFC_UpdateLogicLunSts(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NFC_UpdateLogicLunSwSts(volatile NFC_CMD_STS_REG *pCurCmdStsReg, U8 ucPhyPuInTotal, U8 ucLunInPhyPu, U32 ulUpdateBitmap);
void NfcM_ISetCmdCode(U32 ulPhyPuInTotal, U32 ulLunInPhyPu, U32 ulLevel, U32 ulCommandCode);
void NfcM_ISetCmdMode(U32 ulPhyPuInTotal, U32 ulLunInPhyPu, U32 ulLevel, BOOL bSLCMode);
void NFC_SoftDecTrigger(U8 pu);
//tobey end

#endif

