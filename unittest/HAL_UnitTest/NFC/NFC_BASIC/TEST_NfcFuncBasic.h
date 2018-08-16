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
Filename    : TEST_NfcFuncBasic.h
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : 
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_BASIC_FUNC_H_
#define _HAL_NFC_TEST_BASIC_FUNC_H_

/****************************************************************************
    INCLUDE FILES DEFINITION
****************************************************************************/
#include "HAL_LdpcEngine.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_DecStsReport.h"
#include "HAL_EncriptionModule.h"
#include "TEST_NfcPattGenBasic.h"

/****************************************************************************
    EXTERN VARIABLES
****************************************************************************/   
extern GLOBAL U32* g_pMCU1DramEndBase;

/****************************************************************************
    MACRO DEFINITION
****************************************************************************/   
//#define RAND_READ

/* DEC status function control */
//#define DEC_FIFO_CHK
#define DEC_SRAM_CHK

#ifdef DEC_FIFO_CHK
//#define HARD_PLUS_BITMAP_CHK      //mux with DEC_FIFO_ERRCNT_CHK
//#define DEC_FIFO_ERRCNT_CHK         //mux with HARD_PLUS_BITMAP_CHK and DEC_SRAM_CHK
#endif

#ifdef DEC_SRAM_CHK
#undef DEC_FIFO_ERRCNT_CHK
#endif

#ifdef HARD_PLUS_BITMAP_CHK
#undef DEC_FIFO_ERRCNT_CHK
#endif

#ifdef DEC_FIFO_ERRCNT_CHK
#undef HARD_PLUS_BITMAP_CHK
#endif

//#define DATA_SYNTAX_MIX       //rand sel data syntax, only support single plane EWR
#ifdef DATA_SYNTAX_MIX
#undef DATA_CHK               
#endif

#define TEST_PG_PER_BLK             (g_bTlcMode ? PG_PER_BLK : SLC_PG_PER_BLK)
#define TEST_PG_PER_WL              (g_bTlcMode ? INTRPG_PER_PGADDR : 1)

/*    LPN related    */
#define LPN_PER_PLN_BITS            (PHYPG_SZ_BITS - LPN_SIZE_BITS)     //2
#define LPN_PER_PLN                 (1 << LPN_PER_PLN_BITS)             //4

#define SEC_PER_PIPE_PG_BITS        (SEC_PER_PHYPG_BITS + PLN_PER_LUN_BITS)
#define SEC_PER_PIPE_PG             (1 << SEC_PER_PIPE_PG_BITS)

#ifndef SEC_PER_CW_BITS
#define SEC_PER_CW_BITS             (CW_INFO_SZ_BITS - SEC_SZ_BITS)
#define SEC_PER_CW                  (1 << SEC_PER_CW_BITS)
#endif

#define TEST_ADDR_OFF               (TEST_LUN_NUM * TEST_PU_NUM * TEST_PAGE_NUM)

#if (0 == OTFB_BASE_SEL)
#define OTFB_NFC_BASE               (OTFB_START_ADDRESS)     //0xfff00000
#define OTFB_DATA_NFC_BASE          (OTFB_FW_DATA_MCU0_BASE) //0xfff0ca00
#elif (1 == OTFB_BASE_SEL)
#define OTFB_NFC_BASE               (SRAM0_START_ADDRESS)    //0x1ff80000
#define OTFB_DATA_NFC_BASE          (MCU012_DSRAM1_BASE)     //0x1ffc8000
#elif (2 == OTFB_BASE_SEL)
#define OTFB_NFC_BASE               (MCU2_ISRAM_BASE)        //0x20000000
#define OTFB_DATA_NFC_BASE          (MCU2_ISRAM_BASE)        //0x20000000
#endif

#define PG_NUM_PER_PRG              (PGADDR_PER_PRG * INTRPG_PER_PGADDR)

#ifdef DRAMLESS_ENABLE
#define TEST_START_ADDRESS          (OTFB_NFC_BASE)
#define START_BUFFER_ID             ((OTFB_DATA_NFC_BASE - OTFB_START_ADDRESS)/(PHYPG_SZ<<PLN_PER_LUN_BITS))// 0xFFF10000
#else
#define TEST_START_ADDRESS          (DRAM_START_ADDRESS)
#define START_BUFFER_ID             ((*g_pMCU1DramEndBase - TEST_START_ADDRESS)/(PHYPG_SZ<<PLN_PER_LUN_BITS))
#endif

#define START_WBUF_ID               (START_BUFFER_ID * PG_NUM_PER_PRG )
#define START_RBUF_ID               (START_WBUF_ID + TEST_PU_NUM * PG_NUM_PER_PRG)
#define START_WRED_ADDR             (((START_RBUF_ID + TEST_ADDR_OFF * PG_NUM_PER_PRG)* (PHYPG_SZ<<PLN_PER_LUN_BITS)) + TEST_START_ADDRESS)
#define START_RRED_BASE             (START_WRED_ADDR + RED_PRG_SZ * PG_NUM_PER_PRG)
#define START_RRED_ADDR             (START_RRED_BASE + RED_PRG_SZ * PG_NUM_PER_PRG)//(START_RRED_BASE + sizeof(U32))  //align in RED_SZ

#define TEST_SSU0_DATA              ('S')
#define TEST_SSU1_DATA              ('U')
#define TEST_CACHE_STS_DATA         ('X')

#define TEST_PRG_FAIL_STS           0xE1
#define TEST_PRG_SUCC_STS           0xE0

//PU -> LUN -> PLN -> WP
#define TEST_GET_ADDR_OFF(Pu,Lun,Page)      (((Pu * TEST_LUN_NUM + Lun) * TEST_PAGE_NUM) + Page)

#define GET_CMD_LEVEL(Pu,Lun,Pln,WP)        ((((Pu * NFC_LUN_MAX_PER_PU) | Lun) << NFCQ_DEPTH_BIT) | WP)                                           

/****************************************************************************
    VARIABLE DEFINITION
****************************************************************************/
/*  SSU0  */
extern GLOBAL MCU12_VAR_ATTR U32 g_ulSsuInDramBase;
/*  SSU1  */
extern GLOBAL MCU12_VAR_ATTR U32 g_ulSsu1DramBase;
/*  cache status  */
extern GLOBAL MCU12_VAR_ATTR U32 g_ulCacheStatusAddr;
/* REDUNDANT */
extern volatile NFC_RED **pRRed;
extern volatile NFC_RED *pWrRed;
/* DEC FIFO index */
extern GLOBAL U8 g_ucDecFifoCmdIndexMap[NFC_PU_MAX][NFC_LUN_PER_PU][PG_PER_BLK * PGADDR_PER_PRG * INTRPG_PER_PGADDR];

/*  For NFC read test    */
typedef enum _TEST_READ_REQ_TYPE_
{
    SING_PLN_READ = 0,
    SING_PLN_CCL_READ,
    FULL_PAGE_READ,
    RED_ONLY_READ,
    CHANGE_COL_READ,
    HOST_READ,
    READ_TYPE_CNT
}READ_REQ_TYPE;

/*  For NFC write test    */
typedef enum _TEST_WRITE_REQ_TYPE_
{
    SING_PLN_WRITE = 0,
    FULL_PAGE_WRITE,
    HOST_WRITE
}WRITE_REQ_TYPE;

typedef enum _TEST_NFC_PRG_MODE_
{
    NORM_PRG = 0,    // normal page program, 128K, for intel 3D MLC/TLC
    LOW_PRG,         // low page program, 64KB
    WR_MODE_CNT,
}TEST_NFC_PRG_MODE;

/*  For NFC read test    */
typedef enum _TEST_FLASH_ID_
{
    TYPE_MICRON_L95 = 0,
    TYPE_TSB_TLC,
    TYPE_TSB_A19_BGA,
    TYPE_TSB_A19_2LUN,
    TYPE_TSB_A15_2LUN,
    TYPE_TSB_A15_TLC,
    TYPE_INT_3D_MLC,
    TYPE_INT_3D_TLC,
    TYPE_TSB_3D_TLC
}FLASH_ID;

typedef enum _TEST_SSU_RED_UPDATE_SEL_
{
    OTFB_W_OTFB_R = 0,
    OTFB_W_DRAM_R,
    DRAM_W_OTFB_R,
    DRAM_W_DRAM_R,
    SSU_RED_CASE_CNT
}SSU_RED_UPDATE_SEL;

typedef enum _TLC_COPY_OPERATION_
{
    SLC_TO_SLC_EXT,
    SLC_TO_TLC_EXT,
    SLC_TO_SLC_INT,
    SLC_TO_TLC_INT_ONE_STAGE,
    SLC_TO_TLC_INT_THREE_STAGE
}TLC_COPY_OPERATION;


/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL BOOL g_bDecFifoEn;
extern GLOBAL BOOL g_bEmEnable;

/*------------------------------------------------------------------------------
   FUNCTION DECLARATION
------------------------------------------------------------------------------*/
void TEST_NfcEraseAll(void);
void TEST_NfcWriteAll(WRITE_REQ_TYPE WriteType);
void TEST_NfcReadAll(U8 ucSecStart, U16 usSecLen, READ_REQ_TYPE ReadType);
void TEST_NfcSinglePlnEWR(void);
void TEST_NfcPatternSel(void);
void TEST_NfcBasicPattRun(void);

#endif

/* end of this file  */
