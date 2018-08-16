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
    20150416    abby    create
*******************************************************************************/
#ifndef __HAL_FLASH_CMD_H__
#define __HAL_FLASH_CMD_H__

/*------------------------------------------------------------------------------
    INCLUDING FILES 
------------------------------------------------------------------------------*/
#include "HAL_MemoryMap.h"
#include "HAL_FlashChipDefine.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION 
------------------------------------------------------------------------------*/
/* MLC/SLC retry */
#define HAL_FLASH_READRETRY_CNT         16
#define HAL_FLASH_MAX_RETRY_TIME        16
#define HAL_FLASH_RETRY_PARA_MAX        1   // MICRON MLC:1 value for set festure

#ifdef READ_RETRY_REFACTOR
#define HAL_SLC_FLASH_READRETRY_CNT         (SLC_RETRY_GROUP_CNT + RETRY_CNT)   // The summary of official and SLC homemad read retry entry length, SLC_RETRY_GROUP_CNT + RETRY_CNT
#define HAL_MLC_FLASH_READRETRY_CNT         (MLC_RETRY_GROUP_CNT + RETRY_CNT)   // The summary of official and MLC homemad read retry entry length, MLC_RETRY_GROUP_CNT + RETRY_CNT
#define HAL_TLC_FLASH_READRETRY_CNT         (TLC_RETRY_GROUP_CNT + RETRY_CNT)   // The summary of official and TLC homemad read retry entry length, TLC_RETRY_GROUP_CNT + RETRY_CNT
#else
#define HAL_SLC_FLASH_READRETRY_CNT         HAL_FLASH_READRETRY_CNT   // The summary of official and SLC homemad read retry entry length, SLC_RETRY_GROUP_CNT + RETRY_CNT
#define HAL_MLC_FLASH_READRETRY_CNT         HAL_FLASH_READRETRY_CNT   // The summary of official and MLC homemad read retry entry length, MLC_RETRY_GROUP_CNT + RETRY_CNT
#define HAL_TLC_FLASH_READRETRY_CNT         HAL_FLASH_READRETRY_CNT   // The summary of official and TLC homemad read retry entry length, TLC_RETRY_GROUP_CNT + RETRY_CNT
#endif

#define HAL_TLC_FLASH_RETRY_PARA_MAX    HAL_FLASH_RETRY_PARA_MAX   // only for compile

/* flash pio cmd relative declare start */
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

/*  for async mode PIO, add by abby  20151105  */
#define RAW_PIO_DATAOUT    RAW_PIO_IO_CONFIG1(0x00)|\
    RAW_PIO_IO_CONFIG2(0)|\
    RAW_PIO_CLE_MODE(0)|\
    RAW_PIO_ALE_MODE(0)|\
    RAW_PIO_WE_MODE(1)|\
    RAW_PIO_RE_MODE(0)|\
    RAW_PIO_DQS_MODE(1)|\
    RAW_PIO_IO_MODE(1)|\
    RAW_PIO_IO_TOBECNTD(0)|\
    RAW_PIO_IO_LAST_PHS(0)

/* flash normal cmd relative define */
#define QE_INDEX(x)                 ( x & 0xf)     //QEx[4:0]: INDEX;
#define QE_GRP_ATTR(_Type_)         (_Type_ << 4)  //combination of GROUP and ATTR

/* PRCQ related */
#define QE_CMD_GROUP_DEPTH          16
#define QE_OPRATION_GROUP_DEPTH     8

/* Get byte index align in DW  */
#define ALIGN_IN_DW(ByteCnt)         ((ByteCnt % sizeof(U32))? (ByteCnt + (sizeof(U32) - (ByteCnt % sizeof(U32)))) : ByteCnt)

/* move from HAL_FlashDriverBasic.h    */
#define MULTI_VALUE_1(bits)          (~((((U32)INVALID_8F)>>(bits))<<(bits)))
                                               
/*--------------------------------------------------------
            QE RELATED DECLARATION 
--------------------------------------------------------*/
/* PRCQ One Phase Cmd Element entry index */
typedef enum _CQE1E_INDEX  
{
    CQE1E_EF,CQE1E_61,CQE1E_FF,CQE1E_FC,
    CQE1E_90,CQE1E_00,CQE1E_84,CQE1E_60,
    CQE1E_13,CQE1E_EE,CQE1E_3F,CQE1E_D2,
    CQE1E_80,CQE1E_DA,CQE1E_DF,CQE1E_FA
}CQE1E_INDEX;

/* PRCQ Two Phase Cmd Element entry index */
typedef enum _CQE2E_INDEX  
{
    CQE2E_3000,CQE2E_1080,CQE2E_1181,CQE2E_D060,
    CQE2E_1180,CQE2E_1081,CQE2E_E005,CQE2E_1185,
    CQE2E_3060,CQE2E_1580,CQE2E_3200,CQE2E_3100,
    CQE2E_3160,CQE2E_1581,CQE2E_2000,CQE2E_E006
}CQE2E_INDEX;

/* PRCQ Status Phase Cmd Element entry index */
typedef enum _SQEE_INDEX  
{
    SQEE_3E70,SQEE_2E70,SQEE_3D70,SQEE_3E71,
    SQEE_BE70,SQEE_3D78,SQEE_2E78,SQEE_3E78
}SQEE_INDEX;

/*--------------------------------------------------------
        PRCQ TABLE RELATED 
--------------------------------------------------------*/
/* raw cmd relative offset in g_aPRCQTable */
typedef enum _PRCQ_TYPE 
{
    FLASH_PRCQ_RESET = 0,
    FLASH_PRCQ_RESET_NOSTS,
    FLASH_PRCQ_SYNC_RESET,
    FLASH_PRCQ_RESET_LUN,
    FLASH_PRCQ_READID,
    FLASH_PRCQ_SETFEATURE,

    /* SLC mode single plane */
    FLASH_PRCQ_SLC_ERS_1PLN,        //7
    FLASH_PRCQ_SLC_ERS_SPD_1PLN,    //erase + suspend
    FLASH_PRCQ_SLC_ERS_RSM_1PLN,
    FLASH_PRCQ_SLC_READ_1PLN,
    FLASH_PRCQ_SLC_SNAP_READ_1PLN,
    FLASH_PRCQ_SLC_PRG_1PLN,        //12
    FLASH_PRCQ_SLC_PRG_SPD_1PLN,
    FLASH_PRCQ_SLC_PRG_RSM_1PLN,
    /* SLC mode single plane END*/
    
    /* TLC mode single plane */
    FLASH_PRCQ_TLC_ERS_1PLN,        //15
    FLASH_PRCQ_TLC_ERS_SPD_1PLN,
    FLASH_PRCQ_TLC_ERS_RSM_1PLN,
    FLASH_PRCQ_TLC_READ_1PLN,
    FLASH_PRCQ_TLC_SNAP_READ_1PLN,
    FLASH_PRCQ_TLC_PRG_1PLN,
    FLASH_PRCQ_TLC_PRG_SPD_1PLN,
    FLASH_PRCQ_TLC_PRG_RSM_1PLN,
    FLASH_PRCQ_TLC_PRG_1PLN_LOW_PG_NO_HIGH,
    FLASH_PRCQ_TLC_PRG_SPD_1PLN_LOW_PG_NO_HIGH,
    FLASH_PRCQ_TLC_PRG_RSM_1PLN_LOW_PG_NO_HIGH,
    /* TLC mode single plane END*/

    FLASH_PRCQ_CHANGEREADCOLUM_1PLN,
    FLASH_PRCQ_READ_STS_ENHANCE,
    FLASH_PRCQ_GETFEATURE,

#ifndef BOOTLOADER

    /* SLC mode multi plane */
    FLASH_PRCQ_SLC_ERS_FULL_PLN,    //28
    FLASH_PRCQ_SLC_ERS_3PLN,
    FLASH_PRCQ_SLC_ERS_2PLN,
    FLASH_PRCQ_SLC_ERS_SPD_FULL_PLN,
    FLASH_PRCQ_SLC_ERS_RSM_FULL_PLN,
    FLASH_PRCQ_SLC_READ_FULL_PLN,   //33
    FLASH_PRCQ_SLC_READ_3PLN,
    FLASH_PRCQ_SLC_READ_2PLN,
    FLASH_PRCQ_SLC_PRG_FULL_PLN,    //36
    FLASH_PRCQ_SLC_PRG_3PLN,
    FLASH_PRCQ_SLC_PRG_2PLN,
    FLASH_PRCQ_SLC_PRG_SPD_FULL_PLN,
    FLASH_PRCQ_SLC_PRG_RSM_FULL_PLN,
    /* SLC mode multi plane END */

    /* TLC mode multi plane */
    FLASH_PRCQ_TLC_ERS_FULL_PLN,    //41
    FLASH_PRCQ_TLC_ERS_3PLN,
    FLASH_PRCQ_TLC_ERS_2PLN,
    FLASH_PRCQ_TLC_ERS_SPD_FULL_PLN,
    FLASH_PRCQ_TLC_ERS_RSM_FULL_PLN,
    FLASH_PRCQ_TLC_READ_FULL_PLN,
    FLASH_PRCQ_TLC_READ_3PLN,
    FLASH_PRCQ_TLC_READ_2PLN,
    FLASH_PRCQ_TLC_PRG_FULL_PLN,    //49
    FLASH_PRCQ_TLC_PRG_3PLN,
    FLASH_PRCQ_TLC_PRG_2PLN,        //51
    FLASH_PRCQ_TLC_PRG_SPD_FULL_PLN,
    FLASH_PRCQ_TLC_PRG_RSM_FULL_PLN,
    FLASH_PRCQ_TLC_PRG_FULL_PLN_LOW_PG_NO_HIGH, //54
    FLASH_PRCQ_TLC_PRG_3PLN_LOW_PG_NO_HIGH,
    FLASH_PRCQ_TLC_PRG_2PLN_LOW_PG_NO_HIGH,     //56
    FLASH_PRCQ_TLC_PRG_SPD_FULL_PLN_LOW_PG_NO_HIGH,
    FLASH_PRCQ_TLC_PRG_RSM_FULL_PLN_LOW_PG_NO_HIGH,
    FLASH_PRCQ_TLC_PRG_FULL_PLN_LOW_PG,
    /* TLC mode multi plane END */
    
    FLASH_PRCQ_CHANGEREADCOLUM_FULL_PLN,
    /* Flash data register read/write, not write to flash cell */
    FLASH_PRCQ_HALF_PRG_1PLN,
    FLASH_PRCQ_HALF_READ_1PLN,

#endif
    FLASH_PRCQ_NON_PIO_NUM,
    FLASH_PRCQ_PIO_SETFEATURE = FLASH_PRCQ_NON_PIO_NUM,
    FLASH_PRCQ_PIO_SETFEATURE_EX,
    FLASH_PRCQ_CNT                            
}PRCQ_TYPE;

/* PRCQ(programmable raw cmd queue) relative define */
typedef struct _NFC_PRCQ_ELEM
{
    U8 bsIndex:4;
    U8 bsAttribute:2;
    U8 bsGroup:2;
}NFC_PRCQ_ELEM;

/* PRCQ queue element field */
typedef struct _NFC_PRCQ_QE
{
    /* 1 cycle cmd  group */
    U8 aPRCQ_CQE1E[QE_CMD_GROUP_DEPTH];

    /* 2 cycle cmd group */
    U16 aPRCQ_CQE2E[QE_CMD_GROUP_DEPTH];

    /* reserved */
    U32 aRsv[8];

    /* status group */
    U16 aPRCQ_SQEE[QE_OPRATION_GROUP_DEPTH];
}NFC_PRCQ_QE;

typedef struct _NFC_PRCQ_QE_EXT
{
    /* 1 cycle cmd  group */
    U8 aPRCQ_CQE1E[QE_CMD_GROUP_DEPTH];

    /* 2 cycle cmd group */
    U16 aPRCQ_CQE2E[QE_CMD_GROUP_DEPTH];
}NFC_PRCQ_QE_EXT;

/* if is PIO or XOR cmd in g_aPRCQTable */
typedef enum _XOR_PIO_TYPE_
{
    NF_NORMAL_CMD = 0,
    NF_PIO_CMD
}XOR_PIO_TYPE;

/* FLASH ADDR index */
typedef enum _QE_PLN_INDEX_
{
    ADDR0 = 0,
    ADDR1,
    ADDR2,
    ADDR3,
    ADDR4,
    ADDR5,
    ADDR6,
    ADDR7
}_QE_PLN_INDEX;

/* trigger command type define*/
typedef enum _NFC_TRIGGER_CMD_TYPE_
{
    TRIG_CMD_WRITE = 0,
    TRIG_CMD_READ,
    TRIG_CMD_ERASE,
    TRIG_CMD_OTHER
}NFC_TRIGGER_CMD_TYPE;

/* RAW COMMAND struct define */
typedef struct _PRCQ_TABLE_
{
    U32 bsCmdCode:8;          // Operation type
    U32 bsIsPioCmd:1;         // 0 - normal cmd; 1 - PIO cmd 
    U32 bsTrigType:2;         // NFC Trig Type
    U32 bsPRCQStartDw:9;      // Start Label of the current PRCQ type. DW align
    U32 bsRsv:12;
}PRCQ_TABLE;

/* read retry parameters setting struct define */
typedef struct _RETRY_PARA_
{
    U32 bsCmd:8;            
    U32 bsAddr:8;            
    U32 bsData:8; 
    U32 bsRsv:8;
}RETRY_PARA;

/* read retry parameters setting struct define */
typedef struct _RETRY_TABLE_
{
    RETRY_PARA aRetryPara[HAL_FLASH_RETRY_PARA_MAX];
}RETRY_TABLE;

/* PRCQ QE type define */
typedef enum _PRCQ_QE_TYPE_
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

/* Retry Parameter Table Index */
typedef enum _RETRY_PARA_GROUP_INDEX_
{
    TLC_RETRY_CNT = 0,  //only for compatiable with TLC
    /*  SLC or MLC  */
    RETRY_PARA_GROUP0 = TLC_RETRY_CNT,
    RETRY_PARA_GROUP1,
    RETRY_PARA_GROUP2,
    RETRY_PARA_GROUP3,
    RETRY_PARA_GROUP4,
    RETRY_PARA_GROUP5,
    RETRY_PARA_GROUP6,
    RETRY_PARA_GROUP7,
    RETRY_PARA_GROUP8,
    RETRY_PARA_GROUP9,
    RETRY_PARA_GROUPA,
    RETRY_PARA_GROUPB,
    RETRY_PARA_GROUPC,
    RETRY_PARA_GROUPD,
    RETRY_PARA_GROUPE,
    RETRY_PARA_GROUPF,
    RETRY_CNT
}RETRY_PARA_GROUP_INDEX;

#if defined(READ_RETRY_REFACTOR)
/* New Retry Parameter Table Index */
typedef enum _TLC_RETRY_GROUP_INDEX_
{
    /*  TLC  */
    TLC_RETRY_GROUP0 = 0,
    TLC_RETRY_GROUP1,
    TLC_RETRY_GROUP2,
    TLC_RETRY_GROUP3,
    TLC_RETRY_GROUP4,
    TLC_RETRY_GROUP5,
    TLC_RETRY_GROUP6,
    TLC_RETRY_GROUP7,
    TLC_RETRY_GROUP8,
    TLC_RETRY_GROUP9,
    TLC_RETRY_GROUP10,
    TLC_RETRY_GROUP11,
    TLC_RETRY_GROUP12,
    TLC_RETRY_GROUP13,
    TLC_RETRY_GROUP14,
    TLC_RETRY_GROUP15,

    TLC_RETRY_GROUP_CNT,
}TLC_RETRY_GROUP_INDEX;

typedef enum _MLC_RETRY_GROUP_INDEX_
{
    /*  SLC or MLC  */
    MLC_RETRY_GROUP0,
    MLC_RETRY_GROUP1,
    MLC_RETRY_GROUP2,
    MLC_RETRY_GROUP3,
    MLC_RETRY_GROUP4,
    MLC_RETRY_GROUP5,
    MLC_RETRY_GROUP6,
    MLC_RETRY_GROUP7,
    MLC_RETRY_GROUP8,
    MLC_RETRY_GROUP9,
    MLC_RETRY_GROUP10,
    MLC_RETRY_GROUP11,
    MLC_RETRY_GROUP12,
    MLC_RETRY_GROUP13,
    MLC_RETRY_GROUP14,
    MLC_RETRY_GROUP15,

    MLC_RETRY_GROUP_CNT,
}MLC_RETRY_GROUP_INDEX;

typedef enum _SLC_RETRY_GROUP_INDEX_
{
    /*  SLC or MLC  */
    SLC_RETRY_GROUP0,
    SLC_RETRY_GROUP1,
    SLC_RETRY_GROUP2,
    SLC_RETRY_GROUP3,
    SLC_RETRY_GROUP4,
    SLC_RETRY_GROUP5,
    SLC_RETRY_GROUP6,
    SLC_RETRY_GROUP7,
    SLC_RETRY_GROUP8,
    SLC_RETRY_GROUP9,
    SLC_RETRY_GROUP10,
    SLC_RETRY_GROUP11,
    SLC_RETRY_GROUP12,
    SLC_RETRY_GROUP13,
    SLC_RETRY_GROUP14,
    SLC_RETRY_GROUP15,

    SLC_RETRY_GROUP_CNT,
}SLC_RETRY_GROUP_INDEX;

/* Retry Parameter Adaptive Retry Table*/
typedef struct _ADAPTIVE_RETRY_TABLE_
{
    U8 aTlcLpTable[(TLC_RETRY_GROUP_CNT + RETRY_CNT)];  // TLC mode, Tlc low page adaptive retry table
    U8 aTlcUpTable[(TLC_RETRY_GROUP_CNT + RETRY_CNT)];  // TLC mode, Tlc upper page adaptive retry table
    U8 aTlcXpTable[(TLC_RETRY_GROUP_CNT + RETRY_CNT)];  // TLC mode, Tlc extra page adaptive retry table

    U8 aMlcLpTable[(MLC_RETRY_GROUP_CNT + RETRY_CNT)];  // TLC mode, Mlc low page adaptive retry table
    U8 aMlcUpTable[(MLC_RETRY_GROUP_CNT + RETRY_CNT)];  // TLC mode, Mlc upper page adaptive retry table

    U8 aSlcLpTable[(SLC_RETRY_GROUP_CNT + RETRY_CNT)];  // TLC mode, Slc low page adaptive retry table

    U8 aSSlcLpTable[(SLC_RETRY_GROUP_CNT + RETRY_CNT)];  // SLC mode, Slc page adaptive retry table
}ADAPTIVE_RETRY_TABLE;
#endif

/*------------------------------------------------------------------------------
   EXTERN VARIABLES DECLARATION  
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ_QE *g_pNfcPRCQQe;
extern GLOBAL MCU12_VAR_ATTR volatile NFC_PRCQ_QE_EXT *g_pNfcPRCQQeExt;
extern GLOBAL MCU12_VAR_ATTR volatile U32 *g_pNfcPRCQ;

/*------------------------------------------------------------------------------
   FUNCTION DECLARATION  
------------------------------------------------------------------------------*/
/*prcq initilization interface*/
void MCU12_DRAM_TEXT HAL_NfcQEEInit(void);
void MCU12_DRAM_TEXT HAL_NfcPrcqInit(void);
void MCU12_DRAM_TEXT HAL_NfcPrcqTableInit(void);
U32 HAL_NfcGetPrcqStartDw(U8 ucCmdCode);

// Comm
#ifdef NF_PRCQ_RESET
#undef NF_PRCQ_RESET
#endif
#define NF_PRCQ_RESET FLASH_PRCQ_RESET

#ifdef NF_PRCQ_RESET_NOSTS
#undef NF_PRCQ_RESET_NOSTS
#endif
#define NF_PRCQ_RESET_NOSTS FLASH_PRCQ_RESET_NOSTS

#ifdef NF_PRCQ_SYNC_RESET
#undef NF_PRCQ_SYNC_RESET
#endif
#define NF_PRCQ_SYNC_RESET FLASH_PRCQ_SYNC_RESET

#ifdef NF_PRCQ_RESET_LUN
#undef NF_PRCQ_RESET_LUN
#endif
#define NF_PRCQ_RESET_LUN FLASH_PRCQ_RESET_LUN

#ifdef NF_PRCQ_READID
#undef NF_PRCQ_READID
#endif
#define NF_PRCQ_READID FLASH_PRCQ_READID

#ifdef NF_PRCQ_SETFEATURE
#undef NF_PRCQ_SETFEATURE
#endif
#define NF_PRCQ_SETFEATURE FLASH_PRCQ_SETFEATURE

#ifdef NF_PRCQ_GETFEATURE
#undef NF_PRCQ_GETFEATURE
#endif
#define NF_PRCQ_GETFEATURE FLASH_PRCQ_GETFEATURE

#ifdef NF_PRCQ_PIO_SETFEATURE
#undef NF_PRCQ_PIO_SETFEATURE
#endif
#define NF_PRCQ_PIO_SETFEATURE FLASH_PRCQ_PIO_SETFEATURE

#ifdef NF_PRCQ_PIO_SETFEATURE_EX
#undef NF_PRCQ_PIO_SETFEATURE_EX
#endif
#define NF_PRCQ_PIO_SETFEATURE_EX FLASH_PRCQ_PIO_SETFEATURE_EX

#ifdef NF_PRCQ_CCLR
#undef NF_PRCQ_CCLR
#endif
#define NF_PRCQ_CCLR FLASH_PRCQ_CHANGEREADCOLUM_1PLN

// ==== SLC mode Single plane ====
#ifdef NF_PRCQ_ERS  //L3 use this !
#undef NF_PRCQ_ERS
#endif
#define NF_PRCQ_ERS FLASH_PRCQ_SLC_ERS_1PLN

#ifdef NF_PRCQ_PRG
#undef NF_PRCQ_PRG 
#endif
#define NF_PRCQ_PRG FLASH_PRCQ_SLC_PRG_1PLN

#ifdef NF_PRCQ_PRG_LOW_PG   //L3 use this !   (In SLC mode. We don't need this cmd in the future. Just use NF_PRCQ_PRG )
#undef NF_PRCQ_PRG_LOW_PG   //                      (tmp set this cmd same as NF_PRCQ_PRG for compatity)
#endif
#define NF_PRCQ_PRG_LOW_PG FLASH_PRCQ_SLC_PRG_1PLN
#ifdef NF_PRCQ_READ //L3 use this !
#undef NF_PRCQ_READ
#endif
#define NF_PRCQ_READ FLASH_PRCQ_SLC_READ_1PLN
// ==== SLC mode Single plane END ====


// ==== TLC mode Single plane ====
#ifdef NF_PRCQ_TLC_ERS
#undef NF_PRCQ_TLC_ERS
#endif
#define NF_PRCQ_TLC_ERS FLASH_PRCQ_TLC_ERS_1PLN
#ifdef NF_PRCQ_TLC_PRG
#undef NF_PRCQ_TLC_PRG
#endif
#define NF_PRCQ_TLC_PRG FLASH_PRCQ_TLC_PRG_1PLN
#ifdef NF_PRCQ_TLC_PRG_LOW_PG
#undef NF_PRCQ_TLC_PRG_LOW_PG
#endif
#define NF_PRCQ_TLC_PRG_LOW_PG FLASH_PRCQ_TLC_PRG_1PLN_LOW_PG_NO_HIGH
#ifdef NF_PRCQ_TLC_READ
#undef NF_PRCQ_TLC_READ
#endif
#define NF_PRCQ_TLC_READ FLASH_PRCQ_TLC_READ_1PLN
// ==== TLC mode Single plane END ====

#ifdef NF_PRCQ_TLC_READ_LP
#undef NF_PRCQ_TLC_READ_LP
#endif
#define NF_PRCQ_TLC_READ_LP FLASH_PRCQ_TLC_READ_1PLN


#ifndef BOOTLOADER

// ==== SLC mode Multi plane ====
#ifdef NF_PRCQ_ERS_MULTIPLN     //L3 use this !
#undef NF_PRCQ_ERS_MULTIPLN
#endif
#define NF_PRCQ_ERS_MULTIPLN FLASH_PRCQ_SLC_ERS_FULL_PLN
#ifdef NF_PRCQ_PRG_MULTIPLN     //L3 use this !
#undef NF_PRCQ_PRG_MULTIPLN
#endif
#define NF_PRCQ_PRG_MULTIPLN FLASH_PRCQ_SLC_PRG_FULL_PLN

#ifdef NF_PRCQ_PRG_MULTIPLN_LOW_PG_NO_HIGH  //L3 use this !  (In SLC mode. We don't need this cmd in the future. Just use NF_PRCQ_PRG_MULTIPLN )
#undef NF_PRCQ_PRG_MULTIPLN_LOW_PG_NO_HIGH //                      (tmp set this cmd same as NF_PRCQ_PRG_MULTIPLN for compatity)
#endif
#define NF_PRCQ_PRG_MULTIPLN_LOW_PG_NO_HIGH FLASH_PRCQ_SLC_PRG_FULL_PLN

#ifdef NF_PRCQ_PRG_MULTIPLN_LOW_PG   //IM 3D MLC use this & IM 3D TLC use this for low page mode.
#undef NF_PRCQ_PRG_MULTIPLN_LOW_PG   //(tmp set this cmd same as NF_PRCQ_PRG_MULTIPLN for compatity)
#endif                                                  //remove later if there are no low page mode.
#define NF_PRCQ_PRG_MULTIPLN_LOW_PG FLASH_PRCQ_SLC_PRG_FULL_PLN

#ifdef NF_PRCQ_READ_MULTIPLN    //L3 use this !
#undef NF_PRCQ_READ_MULTIPLN
#endif
#define NF_PRCQ_READ_MULTIPLN FLASH_PRCQ_SLC_READ_FULL_PLN
// ==== SLC mode Multi plane END ====

// ==== TLC mode Multi plane ====
#ifdef NF_PRCQ_TLC_ERS_MULTIPLN
#undef NF_PRCQ_TLC_ERS_MULTIPLN
#endif
#define NF_PRCQ_TLC_ERS_MULTIPLN FLASH_PRCQ_TLC_ERS_FULL_PLN
#ifdef NF_PRCQ_TLC_PRG_MULTIPLN
#undef NF_PRCQ_TLC_PRG_MULTIPLN
#endif
#define NF_PRCQ_TLC_PRG_MULTIPLN FLASH_PRCQ_TLC_PRG_FULL_PLN

#ifdef NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG_NO_HIGH
#undef NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG_NO_HIGH
#endif
#define NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG_NO_HIGH FLASH_PRCQ_TLC_PRG_FULL_PLN_LOW_PG_NO_HIGH

#ifdef NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG
#undef NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG
#endif
#define NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG FLASH_PRCQ_TLC_PRG_FULL_PLN_LOW_PG
#ifdef NF_PRCQ_TLC_READ_MULTIPLN
#undef NF_PRCQ_TLC_READ_MULTIPLN
#endif
#define NF_PRCQ_TLC_READ_MULTIPLN FLASH_PRCQ_TLC_READ_FULL_PLN
// ==== TLC mode Multi plane END ====

#ifdef NF_PRCQ_TLC_READ_LP_MULTIPLN
#undef NF_PRCQ_TLC_READ_LP_MULTIPLN
#endif
#define NF_PRCQ_TLC_READ_LP_MULTIPLN FLASH_PRCQ_TLC_READ_FULL_PLN

#ifdef NF_PRCQ_CCLR_MULTIPLN
#undef NF_PRCQ_CCLR_MULTIPLN
#endif
#define NF_PRCQ_CCLR_MULTIPLN FLASH_PRCQ_CHANGEREADCOLUM_FULL_PLN

#ifdef NF_PRCQ_READ_STS_ENHANCE
#undef NF_PRCQ_READ_STS_ENHANCE
#endif
#define NF_PRCQ_READ_STS_ENHANCE FLASH_PRCQ_READ_STS_ENHANCE

#ifdef NF_PRCQ_SLC_SNAP_READ
#undef NF_PRCQ_SLC_SNAP_READ
#endif
#define NF_PRCQ_SLC_SNAP_READ FLASH_PRCQ_SLC_SNAP_READ_1PLN

#ifdef NF_PRCQ_TLC_SNAP_READ
#undef NF_PRCQ_TLC_SNAP_READ
#endif
#define NF_PRCQ_TLC_SNAP_READ FLASH_PRCQ_TLC_SNAP_READ_1PLN

#ifdef NF_PRCQ_HALF_PRG_1PLN
#undef NF_PRCQ_HALF_PRG_1PLN
#endif
#define NF_PRCQ_HALF_PRG_1PLN FLASH_PRCQ_HALF_PRG_1PLN

#ifndef NF_PRCQ_HALF_READ_1PLN
#undef NF_PRCQ_HALF_READ_1PLN
#endif
#define NF_PRCQ_HALF_READ_1PLN FLASH_PRCQ_HALF_READ_1PLN

#endif
#endif/*__HAL_FLASH_CMD_H__*/

