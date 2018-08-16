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
Filename    : HAL_FlashCmd.h
Version     : Ver 1.0
Author      : peter
Date        : 2012.01.18
Description : this file declare flash chip relative attribute and prcq initi-
              lization interface.
Others      : 
Modify      :
20140904    tobey     uniform code style and wipe off unrelative code
*******************************************************************************/

#ifndef __HAL_FLASH_CMD_H__
#define __HAL_FLASH_CMD_H__

#include "BaseDef.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_MemoryMap.h"

/* flash pio cmd relative declare start */
#define HAL_FLASH_READRETRY_CNT         7
#define HAL_FLASH_MAX_RETRY_TIME        16

#define PIO_CFG_VALUE(x)       ((x)&0xFF)
#define RAW_PIO_IO_CONFIG1(x)  ((x)&0xFF)
#define RAW_PIO_IO_CONFIG2(x)  (((x)&0xFF)<<8)
#define RAW_PIO_CLE_MODE(x)    (((x)&0x3)<<16) // 0:hold low, 1: hold high PIO_CYC, 2:sync mode toggle 
#define RAW_PIO_ALE_MODE(x)    (((x)&0x3)<<18) // 0:hold low, 1: hold high PIO_CYC, 2:sync mode toggle 
#define RAW_PIO_WE_MODE(x)     (((x)&0x3)<<20) // 0:hold high, 1:async toggle, 2:sync clock toggle, 3:hold low
#define RAW_PIO_RE_MODE(x)     (((x)&0x3)<<22) // 0:hold high, 1:async toggle, 2:hold low in sync, 3:toggle in sync
#define RAW_PIO_DQS_MODE(x)    (((x)&0x1)<<24) // 0:not drive, 1:toggle
#define RAW_PIO_IO_MODE(x)     (((x)&0x7)<<25) // 0:not drive&sample, 1:output in async, 2:output in sync, 3: sample in async, 4:sample in sync
#define RAW_PIO_IO_TOBECNTD(x) (((x)&0x1)<<30) // 0:not hold CE low?  1:hold CE low
#define RAW_PIO_IO_LAST_PHS(x) (((x)&0x1)<<31) // 0:not last phase, 1:last phase

#define RAW_PIO_CMD    RAW_PIO_IO_CONFIG1(0x0)|\
    RAW_PIO_IO_CONFIG2(0)|\
    RAW_PIO_CLE_MODE(1)|\
    RAW_PIO_ALE_MODE(0)|\
    RAW_PIO_WE_MODE(1)|\
    RAW_PIO_RE_MODE(0)|\
    RAW_PIO_DQS_MODE(0)|\
    RAW_PIO_IO_MODE(1)|\
    RAW_PIO_IO_TOBECNTD(0)|\
    RAW_PIO_IO_LAST_PHS(0)

#define RAW_PIO_ADDR    RAW_PIO_IO_CONFIG1(0x0)|\
    RAW_PIO_IO_CONFIG2(0)|\
    RAW_PIO_CLE_MODE(0)|\
    RAW_PIO_ALE_MODE(1)|\
    RAW_PIO_WE_MODE(1)|\
    RAW_PIO_RE_MODE(0)|\
    RAW_PIO_DQS_MODE(0)|\
    RAW_PIO_IO_MODE(1)|\
    RAW_PIO_IO_TOBECNTD(0)|\
    RAW_PIO_IO_LAST_PHS(0)

#define RAW_PIO_DATAOUT    RAW_PIO_IO_CONFIG1(0x00)|\
    RAW_PIO_IO_CONFIG2(0)|\
    RAW_PIO_CLE_MODE(0)|\
    RAW_PIO_ALE_MODE(0)|\
    RAW_PIO_WE_MODE(1)|\
    RAW_PIO_RE_MODE(0)|\
    RAW_PIO_DQS_MODE(0)|\
    RAW_PIO_IO_MODE(1)|\
    RAW_PIO_IO_TOBECNTD(0)|\
    RAW_PIO_IO_LAST_PHS(0)

#define NF_RAW_CMD_READRETRYEN_L0 (RAW_PIO_CMD)|RAW_PIO_IO_CONFIG1(0x5C)
#define NF_RAW_CMD_READRETRYEN_L1 (RAW_PIO_CMD)|RAW_PIO_IO_LAST_PHS(1)|RAW_PIO_IO_CONFIG1(0xc5)
/* flash pio cmd relative declare end */


/* flash normal cmd relative define */
#define QE_INDEX_NUM                16
#define QE_CMD_GROUP_DEPTH          16
#define QE_OPRATION_GROUP_DEPTH     8

#define QE_GROUP(x)                 ((x & 0x3)<<6) //QEx[7:6]: GROUP;
#define QE_ATTRIBUTE(x)             ((x & 0x3)<<4) //QEx[5:4]: ATTR;
#define QE_INDEX(x)                 (x & 0xf)      //QEx[5:4]: INDEX;
#define QE_GRP_ATTR(_Type_)         (_Type_ << 4)  //combination of GROUP and ATTR

/* PRCQ One Phase Cmd Element entry index */
typedef enum _CQE1E_INDEX  
{
    CQE1E_EF,CQE1E_EE,CQE1E_FF,CQE1E_FA,
    CQE1E_90,CQE1E_00,CQE1E_30,CQE1E_60,
    CQE1E_D0,CQE1E_31,CQE1E_3F,CQE1E_85,
    CQE1E_FC,CQE1E_D4,CQE1E_D5
}CQE1E_INDEX;

/* PRCQ Two Phase Cmd Element entry index */
typedef enum _CQE2E_INDEX  
{
    CQE2E_3000,CQE2E_1080,CQE2E_1181,CQE2E_D060,
    CQE2E_1180,CQE2E_1081,CQE2E_E005,CQE2E_E006,
    CQE2E_3060,CQE2E_1580,CQE2E_3200,CQE2E_3100,
    CQE2E_3160,CQE2E_1581
}CQE2E_INDEX;

/* PRCQ Status Phase Cmd Element entry index */
typedef enum _SQEE_INDEX  
{
    SQEE_3E70,SQEE_2E70,SQEE_3D70,SQEE_0E70,
    SQEE_BE70,SQEE_3D78,SQEE_2E78,SQEE_3E78
}SQEE_INDEX;

/* raw cmd relative offset in g_aRCmdParamTable */
typedef enum _PRCQ_TYPE 
{
    NF_PRCQ_READID = 0,    
    NF_PRCQ_ERS_1PLN,
    NF_PRCQ_READ_1PLN,
    NF_PRCQ_PRG_1PLN,

    NF_PRCQ_RESET,    
    NF_PRCQ_ERS_2PLN,
    NF_PRCQ_READ_2PLN,
    NF_PRCQ_PRG_2PLN,

    NF_PRCQ_SETFEATURE,   
    NF_PRCQ_GETFEATURE,
    NF_PRCQ_CHANGEREADCOLUM_1PLN,
    NF_PRCQ_CHANGEREADCOLUM_2PLN,
    NF_PRCQ_NON_PIO_NUM,

    NF_PRCQ_PIO_SETFEATURE = NF_PRCQ_NON_PIO_NUM,
    NF_PRCQ_PIO_SETFEATURE_EX,

    NF_PRCQ_CNT
}PRCQ_TYPE;

/* PLN index */
typedef enum _QE_PLN_INDEX
{
    QE_PLN0 = 0,
    QE_PLN1,
    QE_PLN2,
    QE_PLN3 
}_QE_PLN_INDEX;

/* RAW COMMAND struct define */
typedef struct _PRCQ_TABLE
{
    U32 bsCmdCode:8;
    U32 bsQEPhase:8;
    U32 bsIsPIO:1;
    U32 bsTrigType:2;
    U32 bsRsv:13;
    U32 bsQEPtr;
}PRCQ_TABLE;

/* trigger command type define*/
typedef enum _NFC_TRIGGER_CMD_TYPE
{
    TRIG_CMD_WRITE = 0,
    TRIG_CMD_READ,
    TRIG_CMD_ERASE,
    TRIG_CMD_OTHER
}NFC_TRIGGER_CMD_TYPE;

/* PRCQ QE type define */
typedef enum _PRCQ_QE_TYPE
{
    PRCQ_ONE_CYCLE_CMD = 0,
    PRCQ_TWO_CYCLE_CMD,
    PRCQ_TWO_PHASE_CMD,
    PRCQ_THREE_PHASE_CMD,

    PRCQ_ONE_CYCLE_ADDR,
    PRCQ_COL_ADDR,
    PRCQ_ROW_ADDR,
    PRCQ_FIVE_CYCLE_ADDR,

    PRCQ_REG_READ,
    PRCQ_REG_WRITE,
    PRCQ_DMA_READ,
    PRCQ_DMA_WRITE,
    
    PRCQ_READ_STATUS,
    PRCQ_READ_STATUS_EH,
    PRCQ_IDLE,
    PRCQ_FINISH
}PRCQ_QE_TYPE;

/* Sequence Control define QE[3:0]*/
typedef enum _SCTRL 
{
    JUMP = 0,
    RETURN,
    SELECT,
    SKIP,
    SELECT_W,
    JUMP_BIT = 0x8,
    END = 0xf
}SCTRL;

/* QE[3] = 0:UNJUMP; QE[3] = 1:JUMP;*/
typedef enum _QE_JUMP_FLAG
{
    QE_UNJUMP = 0,
    QE_JUMP
}QE_JUMP_FLAG;

/*prcq initilization interface*/
void HAL_NfcQEEInit(void);
void HAL_NfcPrcqTableInit(void);
PRCQ_TABLE* HAL_NfcGetPrcqEntry(U8 ucPrcqType);
#endif/*__HAL_FLASH_CMD_H__*/

