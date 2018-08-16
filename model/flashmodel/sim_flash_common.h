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
#include "HAL_DecStsReport.h"
#ifdef SIM
#include "flash_meminterface.h"
#else
#include "hfmid.h"
#endif

#include "sim_NormalDSG.h"
#include "model_config.h"

#define g_ulModelSSUOffSetInOtfb  ((((volatile NF_SSU_BASEADDR_REG *)&rNfcSsuBaseAddr)->bsSsuOtfbBase)<<10)
#define g_ulModelSSUOffSetInDram  ((((volatile NF_SSU_BASEADDR_REG *)&rNfcSsuBaseAddr)->bsSsuDramBase)<<10)
#define g_ulModelRedOffSetInDram  ((((volatile NF_RED_DRAM_BASE_REG *)&rNfcRedDramBase)->bsRedDramBaseAddr)<<3)
#define g_ulModelRedOffSetInOtfb  (OTFB_RED_MCU12_SHARE_BASE)

// The max redundant length that NFC model support is 64 bytes.
#define NFCM_MAX_REDUN_SZ_BITS  6
#define NFCM_MAX_REDUN_SZ       (1 << NFCM_MAX_REDUN_SZ_BITS)

typedef enum _NFCM_CHECK_FIRMWARE_ERROR_TYPE
{
    NFCM_CHK_XOR_ON,
    NFCM_CHK_XOR_OFF,
    NFCM_CHK_FW_ERR_TYPE_ASSERT
}NFCM_CHK_FW_ERR_TYPE;

typedef enum _NFCM_PAGE_DATA_LOCATION
{
    NFCM_PAGE_DATA_IN_DRAM,
    NFCM_PAGE_DATA_IN_OTFB,
    NFCM_PAGE_DATA_LOCATION_ASSERT
}NFCM_PAGE_DATA_LOCATION;

/* cmd type for nfc model use, SIM and XTMP env need */
extern U8 g_aNfcModelCmdType[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH];
extern BOOL g_aNfcModelCmdMode[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH];
extern U32 *g_pDataBufferIn;
extern U32 g_aDataBufferOut[PU_SUM][NFC_LUN_PER_PU][LOGIC_PIPE_PG_SZ / sizeof(U32)];

extern volatile DEC_STATUS_SRAM *g_pModelNfcDecSts;//Store LDPC status & flash ID & flash status in DEC STATUS SRAM

NFCQ_ENTRY* COM_GetNFCQEntry(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 rp);
void NFC_ModelProcess();
void NFC_ModelSchedule();
DWORD WINAPI NFC_ModelThread(LPVOID p);

void Comm_NFCModelInit(void);

const volatile NFC_TRIGGER_REG * NfcM_GetTriggerReg(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel);
const U8 NFC_GetCmdCode(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel);
const BOOL NFC_GetCmdMode(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel);
BOOL NfcM_IsXorParityWriteCmd(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel);
U32 NfcM_GetPlaneCount(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level);
U32 NfcM_Get1stAtomRedunOffset(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level);
BOOL NfcM_IsXorParityPageInWriteCmd(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level, U32 plane_index);
BOOL NfcM_IsFullPlaneXorParityPageInWriteCmd(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level);
BOOL NfcM_Is1PlnXorParityWrite(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level);
NFCM_PAGE_DATA_LOCATION NfcM_GetPageDataLocation(const NFCM_LUN_LOCATION *pNfcOrgStruct, U32 ulLevel, U32 ulPlaneNum);
U32 NfcM_GetXoreId(const NFCM_LUN_LOCATION *lun_location, U32 cmd_level);
U32 NfcM_GetPhyPuInTotal(const NFCM_LUN_LOCATION *lun_location);
BOOL NfcM_Is6DsgIssueCmd(U32 ulCmdCode);
BOOL NFC_IsReSet();

#endif