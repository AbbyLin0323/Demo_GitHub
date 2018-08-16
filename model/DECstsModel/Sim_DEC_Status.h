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
#ifndef __SIM_DEC_STATUS_H__
#define __SIM_DEC_STATUS_H__

#include "BaseDef.h"
#include "HAL_DecStsReport.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "sim_flash_config.h"
#include "sim_flash_shedule.h"

extern volatile DEC_STATUS_SRAM *g_pModelNfcDecSts;
extern volatile DEC_FIFO_STATUS *g_pModelDecFifoSts;  //Store flash management status in DEC FIFO
extern volatile XOR_DEC_FIFO_CFG_REG *g_pModelDecFifoCfg;  //DEC FIFO Config

void DecStsM_ReadID(void);
void DecStsM_SetFlashSts(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln, U8 ucErrType);
void DecStsM_SRAM_Set_Error(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln, BOOL IsEmptyError, U8 ucErrType, ST_FLASH_CMD_PARAM *pFlashCmdParam);
void DecStsM_SRAM_Set_CRCError(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln);
void DecStsM_SRAM_Clear_Error(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln);
void DecStsM_FIFO_Set(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln, ST_FLASH_CMD_PARAM *pFlashCmdParam, BOOL bsErr);
U8 DecStsM_Set_SRAM_FIFO(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, PLANE_ERR_TYPE aPlaneErrType, U8 ucPln);

#endif // __SIM_DEC_STATUS_H__
