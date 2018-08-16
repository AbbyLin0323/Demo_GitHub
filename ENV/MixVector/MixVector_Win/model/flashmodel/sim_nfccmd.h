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
#include "model_common.h"

#define PG_TYPE_DATA_SIMNFC         PAGE_TYPE_DATA
//#define PG_TYPE_RPMT                0x01
//#define PG_TYPE_TB_SIMNFC           0x02        
//#define PG_TYPE_BT                  0x03
//#define PG_TYPE_GB_SIMNFC           0x04
//#define PG_TYPE_FW                  0x05
//#define PG_TYPE_LOADER_SIMNFC       0x06
//#define PG_TYPE_EN                  0x07
//#define PG_TYPE_RPMT_LOG            0x08
//#define PG_TYPE_ID                  0x09
#define PG_TYPE_FREE                0xff

#define NCQMD_4KB_FINISH            0
#define NCQMD_ALL_FINISH            1

#define DATA_4KB_SIZE                0X1000

#pragma pack(push, 1)
typedef struct _ST_FLASH_ADDR
{
    U32 pu:6;
    U32 pln:2;
    U32 pbn:16;
    U32 ppo:8;    
}ST_FLASH_ADDR;

typedef struct _ST_DATA_SPARE
{
    U32 lpn;
    U32 rsvd:24;
    U32 pg_type:8; 
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

//extern BOOL NFC_ModelInit(BOOL b_create_new, BOOL b_erase_new, BOOL b_chech_data);
extern BOOL NFC_PGReadCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param);// updated by jasonguo 20141004
extern BOOL NFC_PGWriteCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para);
extern BOOL NFC_BlkEreaseCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para);
extern U32 NFC_GetSpareLPN(U32* pRedData,U8 nLPNInPage);
//extern U8 NFC_GetSparePGType(U32 *p_red);
U8 NFC_GetSparePGType(SIM_NFC_RED * p_red);
U32 NFC_GetSpareOpType(U32* pRedData);
//extern void NFC_GetAddr1st(ST_FLASH_CMD_PARAM* p_flash_cmd_para, U8 ucpu, ST_FLASH_ADDR *pFlashAddr);
//extern void NFC_GetAddr2nd(ST_FLASH_CMD_PARAM* p_flash_cmd_para, U8 ucpu, ST_FLASH_ADDR *pFlashAddr);
void NFC_GetFlashAddr(U32 row_addr, U8 part_in_wl, U8 ucpu, FLASH_PHY *pFlashAddr);
extern U32 dram_get_addr(U32 nBufId, U8 nBufIdH);
extern void mem_set(U32 dst_addr, U32 cnt_dw, U32 val);
BOOL NFC_DataCmp(U32 nWriteValue);
extern void NFC_SetCheckPmtEnable(BOOL bSetFlag);


BOOL NFC_OtfbPGReadCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para,ST_FLASH_READ_RETRY_PARAM* p_flash_readrety_param);// updated by jasonguo 20140815
BOOL NFC_OtfbPGWriteCMD(U8 ucppu, ST_FLASH_CMD_PARAM* p_flash_cmd_para);
BOOL NFC_IsAddrUECC(FLASH_PHY* pFlash_phy);
#endif
/********************** FILE END ***************/
