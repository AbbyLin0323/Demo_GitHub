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
*Filename: sim_flash_shedule.h
*Version: 1.0
*Date: 20120208
*Author: Catherine Li
*
*Description: flash shedule header file
*
*Modification History:
*catherineli,       20120209,      first created
*************************************************/
#ifndef __SIM_FLASH_SHEDULE_H__
#define __SIM_FLASH_SHEDULE_H__

#include "sim_flash_config.h"
#include "sim_flash_interface.h"
#include "HAL_LdpcSoftDec.h"

//tobey add start
//QE TYPE
#define PRCQ_CMD_CQE 0
#define PRCQ_CMD_AQE 1
#define PRCQ_CMD_DQE 2
#define PRCQ_CMD_SQE 3
#define PRCQ_CMD_END 0xFF

//FOR CQE
#define ONE_CYCLE_CMD   0
#define TWO_CYCLE_CMD   1
#define TWO_PHASE_CMD   2
#define THREE_PHASE_CMD 3

//FOR AQE
#define ONE_CYCLE_ADDR  0
#define COLUMN_ADDR     1
#define ROW_ADDR        2
#define FIVE_CYCLE_ADDR 3

//FOR DQE
#define SINGLE_READ      0
#define SINGLE_WRITE     1
#define DMA_READ         2
#define DMA_WRITE        3

//FOR SQE
#define READ_STATUS            0
#define READ_STATUS_ENHANCED   1
#define IDLE                   2

#define MAX_PRCQ_ELEM_PER_CMD  64
#pragma pack(push, 1)

typedef struct _ST_PU_SHEDULE_PARAM
{
    U32 pu_status: 8;           //status of pu
    U32 timer_status: 8;      // timer status
    U32 resv: 16;
    U32 timer_start;
    U32 timer_busy;            //busy time for timer of each pu

}ST_PU_SHEDULE_PARAM;

typedef enum _PU_STATUS_TAG
{
    PU_STATUS_WAIT_CMD = 0,
    PU_STATUS_CMD_BUSY,
    PU_STATUS_WAIT_BUS,
    PU_STATUS_DATA_TRANSFER,
    PU_STATUS_WAIT_OTFB_BUFF

}PU_STATUS;

typedef enum _SGE_OTFB_STATUS
{
    SGE_OTFB_WAIT_PCIE_BUS,
    SGE_OTFB_DATA_OTFB_TO_HOST
}SGE_OTFB_STATUS;

typedef enum _OTFB_BUFF_STS
{
    OTFB_BUFF_FREE,
    OTFB_BUFF_BUSY
}OTFB_BUFF_STS;

typedef enum _TIMER_STATUS_TAG
{
    TIMER_STATUS_SET_START = 0,
    TIMER_STATUS_END
}TIMER_STATUS;

typedef enum _BUS_STATUS_TAG
{
    BUS_STATUS_FREE = 0,
    BUS_STATUS_BUSY
}BUS_STATUS;

typedef enum _CMD_CODE_RETURN_TAG
{
    CMD_CODE_NONE = 0,
    CMD_CODE_READ,
    CMD_CODE_PROGRAM,
    CMD_CODE_ERASE,
    CMD_CODE_RST,
    CMD_CODE_READRETRY_EN,
    CMD_CODE_READRETRY_ADJ,
    CMD_CODE_SET_FEATURE,
    CMD_CODE_GET_FEATURE,
    CMD_CODE_OTHER,
    CMD_CODE_ERR,
    CMD_CODE_READID,
    CMD_CODE_UNKNOWN
}CMD_CODE_RETURN_TAG;
typedef struct _ST_FLASH_READ_RETRY_PARAM
{
    U8 read_retry_en;
    U8 read_retry_success_time;
    U8 read_retry_current_time;
    U8 read_retry_rsvd;
}ST_FLASH_READ_RETRY_PARAM;

typedef struct _ST_FLASH_PAGE_REQ_INFO
{
    U32 row_addr:24;
    U32 part_in_wl:8;

    U32 sec_en[2];

    U32 red_offset_id:4;
    U32 rsvd:28;
}ST_FLASH_PAGE_REQ_INFO;

#define PHY_PAGE_PER_LOGIC_PAGE    PLN_PER_LUN

typedef struct _ST_FLASH_CMD_PARAM
{
    //DWORD 0
    U32 ontf_en:1;
    U32 bsDmaByteEn:1;
    U32 int_en:1;
    U32 red_only:1;
    U32 red_en:1;
    U32 cnup_en:1;
    U32 ssu0_en:1; // updated by jasonguo 20140630
    U32 ssu1_en:1;

    U32 ncq_1st_en:1;
    U32 ncq_mode:1;
    U32 ncq_num:5;
    U32 inj_en:1;

    U32 err_type:4;
    U32 xfer_sec_cnt:9;// sector to transfer on bus.
    U32 bsTrigOmEn:1;
    U32 bsRomRd:1;
    U32 rsvDW0 : 1;

    //DW 1
    U32 bsCmdType:8;
    U32 bsCmdCode:8;
    U32 bsIsTlcMode:1;
    U32 bsPageInWL:2;
    U32 bsPlnNum:3;
    U32 bsIs6DsgIssue:1;
    U32 bsBypassRdErr : 1; // added by jasonguo 20160419
    U32 bsRawReadEn : 1;   // added by jasonguo 20160419
    U32 bsInterCopyEn:1;   // added by jasonguo 20160426
    U32 rsvDW1:6;

    //DW 2
    U32 module_inj_en:8;
    U32 module_inj_type:8;
    U32 module_inj_rsvd:16;

    U32 p_red_addr;
    NFC_RED *p_local_red;
    NFCQ_ENTRY* p_nf_cq_entry;
    U32 buffMapValue;
    BOOL bIsDecSet[PHY_PAGE_PER_LOGIC_PAGE];

    ST_FLASH_PAGE_REQ_INFO phy_page_req[8];
    const NFCM_LUN_LOCATION *pLunLocation;
    ST_FLASH_READ_RETRY_PARAM * pFlashReadretryParam;
}ST_FLASH_CMD_PARAM;
#define ERR_INJ_TABLE_MAX 32
typedef struct _ST_FLASH_ERR_INJ_ENTRY
{
    U8  valid;
    U8  ucPhyPuInTotal;
    U16  block;
    U16  page;
    U8  cmd_code;
    U8  err_type;
    U8  retry_times;
    U8  red_only;
    U16 LBA;
}ST_FLASH_ERR_INJ_ENTRY;

//typedef struct _ST_CE_ERROR_ENTRY
//{
//    U8 err_type;
//    U8 err_flag;        //0, have no error; 1, have error
//}ST_CE_ERROR_ENTRY;
#pragma pack (pop)

/************************************************
GLOBAL value define
*************************************************/
extern U32 g_secAddGroup;
extern U32 g_splitedSecInGroup;
extern U32 g_totalRemainSec;
//extern U8 g_dmaAddrHighByte;
//extern U32 g_dmaEntryGroup;
extern U32 g_finishSecInDmaEntry;
extern U32 g_remainSecToXfer;
extern U32 g_bmBitPos;
extern ST_FLASH_ERR_INJ_ENTRY  g_stFlashErrInjEntry[ERR_INJ_TABLE_MAX];
#ifdef SIM
extern U32 g_RetryTimes[]; // updated by jasonguo 20140815
#endif
//add by abby for BITMAP test
extern GLOBAL BOOL g_bTestBitMap;

extern CRITICAL_SECTION g_aCSUptTLunCmdSts[];
extern CRITICAL_SECTION g_aCSUptTLunSwBitmap;

BOOL NFC_ModelParamInit();
void NFC_SendCMD(U8 ucPhyPuInTotal, U8 ucLunInPhyPu);
void NFC_DbgSendCMD(U8 pu);
void NfcM_CmdHandle(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NfcM_CmdFinish(ST_FLASH_CMD_PARAM *pFlashCmdParam);
void NfcM_CmdProcess(const NFCM_LUN_LOCATION *tNfcOrgStruct);
void NFC_ErrorInjCmd(ST_FLASH_CMD_PARAM*p_flash_param);
BOOL NFC_SetErrInjEntry(const NFCM_LUN_LOCATION *tNfcOrgStruct, U16 block, U16 page, U8 cmd_code,U8 err_type,U8 index,U8 retry_times,U8 redOnly);
void NFC_ResetErrInjEntry();
void NFC_ClearInjErrEntry(U8 Pu,U16 PhyBlk,U16 Page);
#endif

