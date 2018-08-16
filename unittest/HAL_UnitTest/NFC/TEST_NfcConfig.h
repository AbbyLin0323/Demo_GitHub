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
Filename    : TEST_NfcConfig.h
Version     : Ver 1.0
Author      : abby
Date        : 20160905
Description : config NFC test feature in this file
              compile both by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_CONFIG_H_
#define _HAL_NFC_TEST_CONFIG_H_

#include "Proj_Config.h"
#include "Disk_Config.h"

/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR BOOL g_bTlcMode;
extern GLOBAL MCU12_VAR_ATTR BOOL g_bEmEnable;

/*------------------------------------------------------------------------------
    shared macro by basic and ext pattern
------------------------------------------------------------------------------*/
/*    Rocord trace info     */
#define TRACE_LINE                                DBG_Printf("Line : %d\n",(U32)__LINE__)
#define TRACE_OUT(_OUT1_,_OUT2_,_OUT3_,_OUT4_)    DBG_Printf("0x%x : 0x%x : 0x%x : 0x%x\n",_OUT1_,_OUT2_,_OUT3_,_OUT4_)
#ifdef SIM
#define NFC_GETCH \
    do{\
        DBG_Getch(); \
    }while(0)
#else
#define NFC_GETCH
#endif

/*  test function control  */
#define TLC_MODE_TEST             //enable: TLC mode; disable: SLC mode

//#define BURN_IN_FOREVER           //always run pattern gen, never stop
//#define DBG_SHOW_ALL              //enable DBG info print
//#define DISABLE_WHOLE_BLK_ERASE   //disable LLF erase to accelate test
//#define DBG_SINGNAL               //enable NFC debug signals
//#define WITHOUT_RED

/* for simulation and debug */
#define FAKE_EXT_CHKLIST
//#define FAKE_BASIC_CHKLIST

/* for performance test */
#define EXT_PERFORMANCE_TEST

/* mux macro */
#ifdef FAKE_EXT_CHKLIST
//#define MIX_VECTOR
#undef  FAKE_BASIC_CHKLIST
#endif

#ifdef FAKE_BASIC_CHKLIST
#undef EXT_PERFORMANCE_TEST
#undef FAKE_EXT_CHKLIST
#undef MIX_VECTOR
#endif

#ifdef MIX_VECTOR
#undef EXT_PERFORMANCE_TEST
#endif

#ifdef EXT_PERFORMANCE_TEST
#define ACC_EXT_TEST                //disable fault tolerant to accelate test
//#define ACC_DEBUG
//#define P_DBG_1
//#define JUMP_RETRY_BLK
#endif
//#define L3_ONE_CMD_CYC_CNT        //recode each CMD cost time in L3 schedule
//#define PATT_GEN_ONE_CMD_CYC_CNT  //recode each CMD cost time in pattern gen

/* for data check */
//#define DATA_CHK                  //prepare and check data * Red, high pressure WR
#ifdef SIM
#define DATA_CHECK_2DW
#endif
/* For data pattern */
#define DATA_PATTERN_SEL            (FIX_AA55)//(RELAVANT_FLASH_ADDR_DATA)//(FIX_AA55)
#define TEST_RED_DATA               (0x55aa1234)

#define TEST_TLUN_START             (0)

/*  BLK define  */
#define TLC_BLK_MAX                 (TLC_BLK_CNT) //456 for tsb 3d tlc
#ifdef FAKE_BASIC_CHKLIST
#define TEST_BLOCK_START            (g_bTlcMode? 122 : 300)
#else
#define TEST_BLOCK_START            (g_bTlcMode? (100): (425))//(g_bTlcMode? (TLC_BLK_MAX - 121): (TLC_BLK_MAX + 20))
#endif
#define TEST_BLOCK_END              (TEST_BLOCK_START + 1)//(SLC_MLC_BLK_END + SLC_MLC_BLK_NUM + 1)

#define SEC_SZ_DW                   (1 << (SEC_SZ_BITS - 2))

/* Redundant defines */
#ifndef DATA_EM_ENABLE
typedef struct _SPARE_AREA_
{
    /* Common Info : 3 DW */
    U32 ulTimeStamp;
    U32 ulTargetOffsetTS;

    U32 bsVirBlockAddr : 16;
    U32 bcBlockType : 8;
    U32 bcPageType : 8;

#ifdef SIM
    U32 ulMCUId;
#endif

    /* Data Block */
    U32 aCurrLPN[LPN_PER_BUF];
}SPARE_AREA;
#else
typedef struct _SPARE_AREA_
{
    /* Data Block */
    U32 aCurrLPN[LPN_PER_BUF];

    /* Common Info : 3 DW */
    U32 ulTimeStamp;
    U32 ulTargetOffsetTS;

    U32 bsVirBlockAddr : 16;
    U32 bcBlockType : 8;
    U32 bcPageType : 8;

#ifdef SIM
    U32 ulMCUId;
#endif

}SPARE_AREA;
#endif

/* performance test type */
typedef enum _PERFOMANCE_TEST_TYPE_
{
    SLC_SEQ_WT_PERF = 0,
    SLC_SEQ_RD_PERF,
    TLC_SEQ_WT_PERF,
    TLC_SEQ_RD_PERF,
    SLC_RAND4K_RD_PERF,
    TLC_RAND4K_RD_PERF,
    SLC2TLC_PERF,
    PATT_GEN_ONE_CMD_PERF,
    L3_ONE_CMD_PERF,
    PERF_TYPE_CNT
}PERFOMANCE_TEST_TYPE;

#endif

/*  end of this file  */
