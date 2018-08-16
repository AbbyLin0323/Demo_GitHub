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

#ifndef _SIM_SATA_LOC_H_
#define _SIM_SATA_LOC_H_

#include "SimATA_Interface.h"
//#define SIM_SATA_CMD_FIFO        // cmd input first, then cmd output first

#define SIM_CMD_NONE            0x0
#define SIM_CMD_PENDING         0x1
#define SIM_CMD_SENT            0x2
#define SIM_CMD_FAIL            0x3
#define SIM_CMD_SUCCESS         0x4
#define SIM_CMD_READ_CRCERR        0x5

#define SIM_HOST_CMD_MAX 32
#define SATA_4K_BITS  12
//#define SATA_SEC_PER_4K_BITS 3


//#define dram_get_addr(buf_id, split_code) (((buf_id)<< (LOGIC_PG_SZ_BITS - (split_code))) + DRAM_START_ADDRESS)
#define dram_buf_id(dram_addr) ((dram_addr - DRAM_START_ADDRESS) >> LOGIC_PG_SZ_BITS)
#define    GetSectorBitMsk(startOff, endOff)    ( (0xFFFFFFFF << (startOff)) & (0xFFFFFFFF >> (31 - (endOff))) )
#define Get4KBitMsk(startOff, endOff)((0xFF << (startOff)) & (0xFF >>(7-(endOff))))


#define SIM_MAX_LBA        (2*25*1024*1024)

#pragma pack(push, 1)



typedef struct _SIM_SATA_CMD
{

    SATA_H2D_REGISTER_FIS FCMDFis;
    U8 cmd_code;
    U8 cmd_status;
    U32 sector_cnt;
    U32 start_lba;
    U32 start_time;
    U32 end_time;


    U32 trans_cnt; /*transfer counter in simulation*/
    U8  ncq_subcmd;
    U8  ncq_subcmdspec;
    U8  *pDataBuffer;
} SIM_SATA_CMD;

typedef struct _SIM_SATA_HOST_MGR
{
    U8 bHandleNonNCQCmd;
    U8 bNonNCQCmdTag;

}SIM_SATA_HOST_MGR;

typedef struct _SHADOW_REG
{
    U8 sdc_data;
    U8 sdc_fea;
    U8 sdc_seccnt;
    U8 sdc_lbalow;
    U8 sdc_lbamid;
    U8 sdc_lbahight;
    U8 sdc_device;
    U8 sdc_status;
}SHADOW_REG;

typedef struct _SHADOW_REG_EXP
{
    U8 sdc_fea_exp;
    U8 sdc_seccnt_exp;
    U8 sdc_lbalow_exp;
    U8 sdc_lbamid_exp;
    U8 sdc_lbahight_exp;
    U8 sdc_device_exp;
    U8 sdc_status_exp;
}SHADOW_REG_EXP;

//#define rSDC_SHRLCH_LBA28       (*(volatile UINT32 *)(SDC_BASE_ADDRESS + 0xD0)) //34
//#define rSDC_SHRLCH_LBA48       (*(volatile UINT32 *)(SDC_BASE_ADDRESS + 0xD4))
//#define rSDC_SHRLCH_SECCNT8     (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xD8))
//#define rSDC_SHRLCH_SECCNT16     (*(volatile UINT16 *)(SDC_BASE_ADDRESS + 0xD8))
//#define rSDC_SHRLCH_FEATURE8    (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xDA))
//#define rSDC_SHRLCH_FEATURE16  (*(volatile UINT16 *)(SDC_BASE_ADDRESS + 0xDA))
//#define rSDC_SHRLCH_COMMAND     (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xDC))
//#define rSDC_SHRLCH_NCQTAG       (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xDE))
//#define rSDC_SHRLCH_EN             (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xE0))
//#define rSDC_SHRLCH_CONTROL     (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xE1))
//#define rSDC_SHRLCH_DEV6           (*(volatile UINT8 *)(SDC_BASE_ADDRESS + 0xE2))

typedef struct _LATCH_SHADOW_REG
{
    U32 sdc_shrlch_lba28;  //D0
    U32 sdc_shrlch_lba48;  //D4
    U16 sdc_shrlch_seccnt; //D8
    U16 sdc_shrlch_feature; //DA
    U8  sdc_shrlch_cmd;     //DC
    U8  sdc_shrlch_rsv0;    //DD
    U8  sdc_shrlch_ncqtag;  //DE
    U8  sdc_shrlch_rsv1;    //DF
    U8  sdc_shrlch_en;      //E0
    U8  sdc_shrch_control;  //E1
    U8  sdc_shrch_dev6;     //E2
}LATCH_SHADOW_REG;

typedef struct _SATA_PRD_BUF_INFO
{
    U32 usBufOffset:8;
    U32 usBufLen:8;
    U32 usBufStartAddrL:16;

}SATA_PRD_BUF_INFO;

typedef struct _SATA_PRD_CACHE_INFO
{
    U32 usCacheAddr:16;
    U32 usCacheData:8;
    U32 bUpdateCacheEn:1;
    U32 nResv:7;

}SATA_PRD_CACHE_INFO;

typedef    struct    _SATA_PRD_ENTRY_EX
{
    /* DWORD0 */
    U32 usTransCnt: 16;
    U32 ucBufMapID: 6;
    U32 uResv0: 1;
    U32 bWrite: 1;
    U32 ucTag: 5;
    U32 bInterrupt: 1;
    U32 bAutoActivate: 1;
    U32 bDMAEnable: 1;

    /* DWORD1 */
    U32 ucSecCnt: 8;
    U32 usBufStartAddrH: 8;
    U32 uResv1:5;
    U32 bCacheStatusAddrSel: 1;
    U32 bUpdateWriteBufmapEn: 1;
    U32 bDramSramSel: 1;
    U32 bFinishInterrupt: 1;
    U32 bAutoAckHost: 1;
    U32 ucKeySel: 2;
    U32 bEncryptionEnable: 1;
    U32 bSendDummyData: 1;
    U32 bValid: 1;
    U32 bEOT: 1;

    /* DWORD2~5 */
    SATA_PRD_BUF_INFO BufInfo[4];

    /* DWORD6~7 */
    SATA_PRD_CACHE_INFO CacheInfo[2];

}SATA_PRD_ENTRY_EX;



#pragma pack (pop)

typedef enum _SDC_HANDLE_STATUS
{
    SDC_HANDLE_NONE,
    SDC_HANDLE_PROCESS,
    SDC_HANDLE_WAIT_SDBFIS,
    SDC_HANDLE_READ_CRC_ERR,
}SDC_HANDLE_STATUS;



extern volatile SIM_SATA_CMD g_SimHostCmd[SIM_HOST_CMD_MAX];
//extern U8    sim_cmd_tag;



#endif