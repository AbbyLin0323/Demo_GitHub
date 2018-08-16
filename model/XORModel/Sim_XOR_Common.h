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
#ifndef __SIM_XOR_COMMON_H__
#define __SIM_XOR_COMMON_H__

#include "HAL_XOR_Common.h"
#include "HAL_FlashDriverBasic.h"

#define XORM_MAX_XOR_PAGE_SIZE  (64 * KB_SIZE)
#define XORM_MAX_CW_CNT  (XORM_MAX_XOR_PAGE_SIZE / CW_INFO_SZ)
#define XORM_MAX_ATOM_REDUN   64
#define XORM_MAX_ATOM_REDUN_DW   (XORM_MAX_ATOM_REDUN / sizeof(U32))


// DEFINITION OF DATA STRUCTURE

typedef enum _XOR_MODEL_UPDATE_TYPE
{
    XORM_TRIGGER,
    XORM_1CW_CALCULATING,
    XORM_1CW_CALCULATED,
    XORM_PARITY_READY,
    XORM_UPDATE_TYPE_ASSERT
}XORM_UPDATE_TYPE;

typedef enum _XOR_MODEL_INTERNAL_STATUS
{
    XORM_IDLE,
    XORM_TRIGGERED,
    XORM_SEND_CONFIG_NFC,
    XORM_FINISH,
    XORM_INTERNAL_STATUS_ASSERT
}XORM_INTERNAL_STATUS;

typedef enum _XOR_MODEL_TARGET
{
    XORM_WT_TO_DRAM = 0,
    XORM_RD_FORM_DRAM
}XORM_TARGET;

typedef struct XOR_MODEL_DATA_BUFFER_ {
  U32 code_word[CW_INFO_SZ_DW];
  U32 atom_redun[XORM_MAX_ATOM_REDUN_DW];
}XORM_DATA_BUFFER;

typedef struct _XOR_MODEL_ENGINE
{
    volatile XORE_CFG_REG   *pConfigReg;
    volatile XORE_STS_REG   *pStatusReg;
    volatile XORE_CTRL_REG  *pControlReg;
    XORM_INTERNAL_STATUS eInternalStatus;
    U16 usCodeWordDoneTimes[XORM_MAX_CW_CNT];
    BOOL bParityReady;
}XORM_ENGINE;

typedef struct XOR_MODEL_ {
  U32 activated_xore_id;
  U32 code_word_index;
  BOOL is_last_cw_done;         // Atom redundant is behide the last Code-Word, but have same
                                // Code-Word index, used to distinguish these two.
  BOOL need_care_dram_data;
  XORM_DATA_BUFFER data_buffer;
}XORM;

typedef struct _XOR_MODEL_ENGINE_PARAM
{
    // DWord 0
    U32 bsXoreId:3;
    U32 bsFinishTime:9;   // Finish Time Range: 1 ~ 256
    U32 bsParitySize:16;  // Parity Size Range: 8K ~ 64K
    U32 bsReserved0:4;

    // Paramter for data
    // DWord 1
    U32 bsStartCw:16;     // Cw Start Range: 0 ~ 63KB
    U32 bsCwLength:16;    // Cw Length Range: 1 ~ 48KB

    // DWord 2~3
    U32 ulSrcDataAddr;
    U32 ulDstDataAddr;

    // Paramter for redundant
    // DWord 4
    U32 bsRedLength:7;    // Red Length Range: 0 ~ 64B 
    U32 bsReserved2:16;

    // DWord 5~6
    U32 ulSrcRedAddr;
    U32 ulDstRedAddr;

    // DWord 7
    U32 ulLastResvd;
    
}XORM_ENGINE_PARAM;


extern XORM_ENGINE g_aXorModelE[];
extern XORM g_xor_model;

void XORM_UpdateRegister(U32 ulXoreId, XORM_UPDATE_TYPE eUpdateType);
void XORM_SwitchInternalStatus(U32 ulXoreId, XORM_INTERNAL_STATUS eTargetStatus);
U32 XORM_GetMinCwDoneTimes(U32 ulXoreId);
void XorM_ConfigXoreFromNfc(U32 ulXoreId, U32 ulBufferId, U32 ulRedunLength, BOOL bDramCrcEn);
U32 XorM_GetCwCntOfSinglePlane(U32 xor_page_size_index, U32 plane_info);
void XORM_XorCalculate(U32 ulXoreId, const U32 *pData, U32 ulCodeWordIndex);
U8 XorM_GetCodeWordCntByReg(U32 ulXoreId);
U32 XorM_GetParityPageOtfbAddr(U32 ulXoreId);
U32 XorM_GetParityRedunOtfbAddr(U32 ulXoreId);
void XorM_AutoLoadToDram(U32 ulXoreId);

void XORM_Comm_CopyData(XORM_ENGINE_PARAM pXorParam);
void XORM_XorOperation(XORM_ENGINE_PARAM pXorParam);

void XORM_ClearXoreCommon(U32 ulXoreId);
void XORM_GetXoreParam(XORM_ENGINE_PARAM *pXorParam, U32 ulXoreId);

void XORM_CheckCwRange(XORM_ENGINE_PARAM pXorParam);

#endif // __SIM_XOR_COMMON_H__