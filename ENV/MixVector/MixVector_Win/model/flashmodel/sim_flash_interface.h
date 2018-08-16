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

//CMD CODE
/************cmd code**************************/
#define NF_CMD_READID NF_PRCQ_READID
#define NF_CMD_ERS_1PLN NF_PRCQ_ERS_1PLN
#define NF_CMD_RD_1PLN NF_PRCQ_READ_1PLN
#define NF_CMD_WT_1PLN NF_PRCQ_PRG_1PLN

#define NF_CMD_RST NF_PRCQ_RESET
#define NF_CMD_ERS NF_PRCQ_ERS_2PLN
#define NF_CMD_RD  NF_PRCQ_READ_2PLN
#define NF_CMD_WT  NF_PRCQ_PRG_2PLN

#define NF_CMD_SETFEATURE NF_PRCQ_SETFEATURE
#define NF_CMD_GETFEATURE NF_PRCQ_GETFEATURE
#define NF_CMD_CHANGECOLUM_1PLN NF_PRCQ_CHANGEREADCOLUM_1PLN
#define NF_CMD_CHANGECOLUM_2PLN NF_PRCQ_CHANGEREADCOLUM_2PLN

#define NF_CMD_READRETRY_EN NF_PRCQ_READRETRY_EN
#define NF_CMD_READRETRY_ADJ_START NF_PRCQ_READRETRY_ADJUSTMENT_0 
#define NF_CMD_READRETRY_ADJ1 NF_PRCQ_READRETRY_ADJUSTMENT_1
#define NF_CMD_READRETRY_ADJ2 NF_PRCQ_READRETRY_ADJUSTMENT_2
#define NF_CMD_READRETRY_ADJ3 NF_PRCQ_READRETRY_ADJUSTMENT_3
#define NF_CMD_READRETRY_ADJ4 NF_PRCQ_READRETRY_ADJUSTMENT_4
#define NF_CMD_READRETRY_ADJ_END NF_PRCQ_READRETRY_ADJUSTMENT_5

#define NF_CMD_PIO_SETFEATURE NF_PRCQ_PIO_SETFEATURE
#define NF_CMD_PIO_SETFEATURE_EX NF_PRCQ_PIO_SETFEATURE_EX

#define NF_CMD_TRIGER_BIT         (0x1 <<7)
#define NF_CMD_OTHER

#pragma pack(push, 1)

typedef struct _ST_BMID_LVL
{
    U8 bmid:6;
    U8 bmenw:1;
    U8 bmls:1;
}ST_BMID_LVL;

#pragma pack (pop)

#define CQENTRY_BMOFF_OFFSET    12
#define CQENTRY_BUFID_OFFSET    8

#define CQDPTR_DW1_OFFSET       1
#define CQDPTR_BYTE_CMD0_OFFSET 6
#define CQDPTR_BYTE_CMD0_DW_OFFSET 2
#define CQDPTR_BYTE_CMD1_OFFSET 7
#define CQDPTR_BYTE_CMD1_DW_OFFSET 3
#define CQDPTR_BYTE_RST_OFFSET  1
#define CQDPTR_BYTE_BM_OFFSET   5
#define CQDPTR_BYTE_BM_DW_OFFSET   1
#define CQDPTR_BIT_DW1_CMD0_OFFSET  16
#define CQDPTR_BIT_DW0_RESET0_OFFSET  (1<<15)
#define CQDPTR_BIT_DW0_RESET1_OFFSET  (1<<14)


extern U32 sim_bufmap[64];


/************************************************
Function define
*************************************************/
extern ST_BMID_LVL flash_bmid_lvl[CE_MAX][NFCQ_DEPTH];

void NFC_InterfaceUpdateHwPuSts(void);
void NFC_InterfaceUpdateLogicPuSts(void);
BOOL NFC_InterfaceIsCQEmpty(U8 pu);
U8 NFC_InterfaceCQRP(U8 pu);
U8 NFC_InterfaceCQWP(U8 pu);
void NFC_InterfaceSetFirstDataReady(U8 tag);
void NFC_InterfaceSetBuffermap(U8 pu, U32 buffMapvalue, U32 Bmid);
void NFC_InterfaceClearBufferMap(U8 pu);
void NFC_InterfaceClearCacheBusy(U32* buf_id_addr, U32 sec_en_low, U32 sec_en_high);
void NFC_InterfaceClearSSU(U32 ssu_addr,U8 value);
void NFC_InterfaceJumpCQRP(U8 pu);
void NFC_InterfaceJumpCQWP(U8 pu);
void NFC_InterfaceSetInt(U8 pu);
void NFC_InterfaceResetCQ(U8 pu, U8 level);
#ifdef HOST_SATA
void SDC_SetReadBufferMapWin(U8 buffmap_id,U32 buffermap_value);
#endif
U32 SDC_GetReadBufferMap(U8 buffmap_id);
void NFC_InterfaceRecvBufferMapID(U8 pu);
void nfcReadReg(U32 addr, U32 nBytes, U8 *dst);
void nfcWriteReg(U32 addr, U32 nBytes, const U8*src);
void nfcReadDram(U32 addr, U32 nWordss, U8 *buf);
void nfcWriteDram(U32 addr, U32 nWordss, const U8* src);
void NFC_InterfaceSetErrParameter(U8 nPU, U8 nErrType);
void NFC_InterfaceRecordErr(U8 pu, U8 errtype);

void NFC_InterfaceUpdateSsu(U32 ssu_addr, U32 value,U8 index);
void NFC_UpdateLogicPuSts(U8 uPhyPu);
U8 NFC_InterfaceGetCmdType(U8 pu, U8 level);
BOOL NFC_GetErrFlag(U8 pu);

//tobey end

#endif

