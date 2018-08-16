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

Filename    : Sim_NfcCmd.h
Version      : Ver 1.0
Date          :
Author       : peterlgchen

Description:

Modification History:
20090522    peterlgchen     001 first create
20120214    PengfeiYu     002 modified
*************************************************/
#ifndef _NFC_CMD_H
#define _NFC_CMD_H

#include "sim_flash_config.h"
#include "sim_flash_shedule.h"
#include "sim_flash_common.h"
#include "Disk_Config.h"
#include "model_config.h"
#include "model_common.h"

#define PG_TYPE_DATA_SIMNFC     PAGE_TYPE_DATA
#define PG_TYPE_FREE            0xf


#define NCQMD_4KB_FINISH        0
#define NCQMD_ALL_FINISH        1

#define DATA_4KB_SIZE           0X1000

//move from HAL_FlashChipDefine.h by abby
#define CW_ERR_CNT_THRESHOLD        (128 /2)
#define RED_CW_ERR_CNT_THRESHOLD    (136 / 2)

#define BAD_BLK_MARK_BYTE_LEN       1

#pragma pack(push, 1)
typedef struct _ST_FLASH_ADDR
{
    U32 pu  : 6;
    U32 pln : 2;
    U32 pbn : 16;
    U32 ppo : 8;
}ST_FLASH_ADDR;

typedef struct _ST_DATA_SPARE
{
    U32 lpn;
    U32 rsvd    : 24;
    U32 pg_type : 8;
    U32 rsvd1[6];
}ST_DATA_SPARE;

typedef struct _ST_FLASH_WRITE_CNT_DATA
{
    U16 HostCnt;
    U16 GCCnt;

}ST_FLASH_WRITE_CNT_DATA;

typedef struct _ST_FLASH_WRITE_CNT
{

    ST_FLASH_WRITE_CNT_DATA * pDataCnt;
    U32 nLength;
}ST_FLASH_WRITE_CNT;
#pragma pack (pop)

/* Redundant defines */
typedef union _SIM_NFC_REDUNDANT_
{
    U32 m_Content[RED_SW_SZ_DW];
    struct
    {
        /* Common Info : 3 DW */
        RedComm m_RedComm;

        /* private data */
        union
        {
            /* AT1 PMT Table */
            U32 m_PMTIndex;

            /* Data Block */
            DataRed m_DataRed;
        };
    };
}SIM_NFC_RED;

extern FLASH_PHY g_LastHandleAddr[NFC_MODEL_LUN_SUM];

extern BOOL NFC_PGReadCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param);// updated by jasonguo 20141004
extern BOOL NFC_PGWriteCMD(const NFCM_LUN_LOCATION *pLunLocation, ST_FLASH_CMD_PARAM* pFlashCmdParam);
extern BOOL NFC_BlkEreaseCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam);
U8 NFC_GetSparePGType(const SIM_NFC_RED *p_red);
U32 NFC_GetSpareOpType(U32* pRedData);
void NFC_GetFlashAddr(const NFCM_LUN_LOCATION *tNfcOrgStruct, U32 row_addr, U8 part_in_wl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr);
extern U32 dram_get_addr(U32 nBufId, U8 nBufIdH);
extern void mem_set(U32 dst_addr, U32 cnt_dw, U32 val);
BOOL NFC_DataCmp(U32 nWriteValue);
extern void NFC_SetCheckPmtEnable(BOOL bSetFlag);

void NFC_TransEmRed(SIM_NFC_RED* pRedDst, SIM_NFC_RED* pRedSrc);
BOOL NFC_OtfbPGReadCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param);// updated by jasonguo 20140815
BOOL NFC_OtfbPGWriteCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam);
BOOL NFC_IsAddrUECC(FLASH_PHY* pFlash_phy);

U8 NFC_ErrInj(U8 err_type,  ST_FLASH_CMD_PARAM *pFlashCmdParam);
void NFC_RecordLastFlashAddr(FLASH_PHY *pFlashAddr);
void NFC_InitLastFlashAddr();
BOOL NFC_ReadFlashIDProcess(const NFCM_LUN_LOCATION *tNfcOrgStruct);
BOOL NFC_ReadByteCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam);

BOOL NFC_SlcCopyToSlcCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam);
BOOL NFC_SlcCopyToTlcCMD(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM* pFlashCmdParam, U8 ucTriggerStage, U8 ucPartInWL);

#endif
/********************** FILE END ***************/
