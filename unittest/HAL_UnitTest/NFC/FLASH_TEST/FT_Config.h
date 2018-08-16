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
Filename     : FT_Config.h
Version      : 
Date         : 
Author       : Abby
Others: 
Modification History:
*******************************************************************************/
#ifndef __FLASH_TEST_CONFIG_H__
#define __FLASH_TEST_CONFIG_H__

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "BaseDef.h"
#include "HAL_ParamTable.h"
#include "HAL_FlashDriverBasic.h"

/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL BOOL g_bFTMlcMode;
extern GLOBAL BOOL g_bFTRawDataRead;
extern GLOBAL BOOL g_bFTSnapRead;
extern GLOBAL BOOL g_bFTDecFifoEn;
extern GLOBAL volatile U8 g_ucFTDecFifoRp;
extern GLOBAL BOOL g_bFTSinglePgProg;  //YMTC MLC single page programming mode

extern GLOBAL FT_PARAM g_tFTParam;
extern GLOBAL BOOL g_bFTGetParamExternal;
extern GLOBAL U32 g_ulFTStartTimer, g_ulFTEndTimer, g_ulFTDiffCycle;

extern GLOBAL U32 g_ulFTN0Cnt;

/*------------------------------------------------------------------------------
    DATA STRUCTURES DEFINITION
------------------------------------------------------------------------------*/

typedef enum _FT_DATA_PATTERN_
{
    ALL_ZERO = 0,
    ALL_ONE,
    ALL_55AA55AA,
    INC_DATA,
    CELL_TYPE,  //pending
    FT_DATA_PATT_NUM
}FT_DATA_PATT;

/*------------------------------------------------------------------------------
    MACRONS DECLARATION
------------------------------------------------------------------------------*/

/* ============================= test pattern select ================================== */
#define FT_DATA_CHK               //For data prepare and check
//#define N0_CHECK                  //Check N0 plane by plane
//#define GET_PARAM_FROM_PTABLE     //config and get parameter by ptable scripts
//#define ERR_BIT_CNT_CHK
//#define IDB_READ
//#define SNAP_READ_TEST      //check snap read in IM or partial read in YMTC
//#define MLC_SINGLE_PAGE_MODE    //check MLC single page programming
//#define GET_DEFAULT_FEAT          //check default feature

/* ============================== target address info ================================= */
#define FT_PU_START     (g_bFTGetParamExternal ? (g_tFTParam.ucPu) : (0))
#define FT_PU_END       (g_bFTGetParamExternal ? (g_tFTParam.ucPu+g_tFTParam.ucTestPuNum) : (1))

#define FT_LUN_START    0
#define FT_LUN_END      (NFC_LUN_PER_PU)

#define FT_PLN_START    0
#define FT_PLN_END      (PLN_PER_LUN)

#define FT_BLK_START    (31)//(g_bFTGetParamExternal ? (g_tFTParam.usBlock) : (300))
#define FT_BLK_END      (FT_BLK_START + 1)//(BLK_PER_PLN+RSV_BLK_PER_PLN)//(FT_BLK_START + 1)//(g_bFTGetParamExternal ? (g_tFTParam.usBlock+g_tFTParam.usTestBlockNum) : (FT_BLK_START + 1))

#define FT_PAGE_START   0
#define FT_PAGE_END     ((g_bFTMlcMode) ? (PG_PER_BLK): (SLC_PG_PER_BLK))

/* ================================== Data Pattern ==================================== */
#define FT_DATA_PATT_SEL               (CELL_TYPE)//(ALL_55AA55AA)//(g_bFTGetParamExternal ? (g_tFTParam.ucDataPatternSelect) : (ALL_55AA55AA))
#define FT_DUMMY_RED_DATA              (0x55aa1234)

/* ================================== debug info ====================================== */
#define TRACE_LINE                                DBG_Printf("Line : %d\n",(U32)__LINE__)



#endif
