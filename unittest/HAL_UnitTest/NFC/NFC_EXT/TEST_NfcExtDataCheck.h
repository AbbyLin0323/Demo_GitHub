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
Filename    : TEST_NfcExtDataCheck.h
Version     : Ver 1.0
Author      : abby
Description : compile by MCU1 and MCU2
Others      :
Modify      :
    20160903    abby    create
*******************************************************************************/
#ifndef _HAL_NFC_TEST_EXT_DATA_CHK_H_
#define _HAL_NFC_TEST_EXT_DATA_CHK_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "COM_Memory.h"
#include "TEST_NfcFCMDQ.h"
#include "FW_BufAddr.h"
#include "HAL_FlashDriverExt.h"

//sector:8 bit; page: 12 bit Tlun:4 bit
#define DUMMY_DATA_WITH_ADDR(Tlun, Page, Sector)    ((Tlun<<28)|(Page<<16)|(Sector<<8))

#define RD_LUN_MAX                  (32)//(SUBSYSTEM_LUN_MAX/8)

#define LOGIC_RED_SZ                (RED_SZ << PLN_PER_LUN_BITS)
#define PHYPG_PER_PRG               (INTRPG_PER_PGADDR*PGADDR_PER_PRG)

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
typedef enum _DATA_PATTERN_
{
    RELAVANT_FLASH_ADDR_DATA = 0,
    RANDOM_DATA,
    FIX_AA55,
    FIX_ALL_ZERO,
    FIX_ALL_ONE,
    DATA_PATT_NUM
}DATA_PATTERN;

/* TSB_2D_TLC -> SLC write, only use write buff ID 0;
TLC write use all buffer of each data pattern, each buff size = 32K  */

/* TSB_2D_TLC -> SLC write, only use red write buff ID 0;
TLC write, use 0&1&2, each red size = 2*64B */
typedef struct _WRITE_RED_BUFF_
{
    U32 *pWrRed[PHYPG_PER_PRG];
}WRITE_RED_BUFF;

//LOGIC_RED_SZ : 64B*4PLN
typedef struct _READ_RED_BUFF_
{
    volatile U32 *pRdRed[RD_LUN_MAX][FCMDQ_DEPTH];
}READ_RED_BUFF;

/* BUF_SIZE: PHYPG_SZ * PLN_PER_LUN */
typedef struct _WRITE_DATA_BUFF_
{
    U32 *pWrBuff[DATA_PATT_NUM][FCMDQ_DEPTH][PHYPG_PER_PRG];
}WRITE_DATA_BUFF;

/* BUF_SIZE: PHYPG_SZ * PLN_PER_LUN */
typedef struct _READ_DATA_BUFF_
{
    volatile U32 *pRdBuff[RD_LUN_MAX][FCMDQ_DEPTH];
}READ_DATA_BUFF;

// once TLC program need 3 buffer for 2D TLC 
typedef struct _WRITE_DATA_BUFF_ID_
{
    U32 aBuffID[DATA_PATT_NUM][INTRPG_PER_PGADDR*PGADDR_PER_PRG];
}DATA_BUFF_ID;

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
BOOL TEST_NfcIsEnableDataCheck(U8 ucFCmdReqType);
void TEST_NfcSetDataCheckBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
void TEST_NfcFCmdQClrDataCheckBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
BOOL TEST_NfcIsRemainDataCheckFCMDQ(void);
U32  TEST_NfcExtCheckData(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
void TEST_NfcAdaptReadBuffAddr(U8 ucTLun, U8 ucWptr, FCMD_REQ_ENTRY *ptReqEntry, BOOL bSLCMode);

#endif
/*  end of this file  */
