/****************************************************************************
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
*****************************************************************************
 * File Name    : TEST_NfcMain.h
 * Discription  : this document is the tesp pattern file for HostRequest.
 * CreateAuthor : tobey
 * CreateDate   : 2013.11.7
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#ifndef _HAL_FLASH_TEST_H_
#define _HAL_FLASH_TEST_H_

#include "Proj_Config.h"
#include "Disk_Config.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
//#include "HAL_FlashIO.h"
#include "HAL_GLBReg.h"
#include "HAL_FlashCmd.h"
#include "HAL_NormalDSG.h"
#include "HAL_Hsg.h"
#include "HAL_HostInterface.h"
#include "HAL_Xtensa.h"
#include "HAL_HCT.h"

#ifdef SIM
#include "flashmodel/sim_flash_shedule.h"
#else
#include <xtensa/tie/via.h>
#include <xtensa/tie/xt_timer.h>
#endif
#include "MixVector_Flash.h"

typedef enum BlockType_Tag
{
    BlockType_Init = 0,
    BlockType_TableBlock,
    BlockType_PMTIBlock,
    BlockType_SeqBlock,
    BlockType_RndBlock,
    BlockType_EmptyBlock,
    BlockType_L3Reserved,
    BlockType_All
}BlockTypes;

typedef enum PageType_Tag
{
    PageType_DataPage = 0,
    PageType_PMTPage = 1,
    PageType_RPMTPage = 1,
    PageType_SeqDataBlock,
    PageType_All
}PageTypes;

typedef    union SpareAreaTag
{
    U32 m_Content[RED_SW_SZ_DW];
    struct
    {
        U32 TimeStamp;
        U32 VirBlockAddr:16;
        BlockTypes BlockType: 8;
        PageTypes PageType:8;
        union
        {
            //U32 CurrLPN[LPNInPhysicalPage];
            struct
            {
                U32 m_PMTIndex;
                U32 m_DateTimeStamp;
            };
        };
#ifdef DBG_PMT_SPARE
        PMT_DEBUG m_PMTDebug[LPNInPhysicalPage];
#endif
    };
}SpareArea;

typedef enum _NFC_MODULE_TEST_PATTERN_TRACE_
{
    NFC_MODULE_TEST_START = 0x8000,
    NFC_MODULE_TEST_FINISH = 0x9000,

    NFC_MODULE_TEST_ERROR = 0xbadc0de0,
    NFC_MODULE_TEST_ERROR_SSU,
    NFC_MODULE_TEST_ERROR_CS,
    NFC_MODULE_TEST_ERROR_DATA,
    NFC_MODULE_TEST_ERROR_RED
}NFC_MODULE_TEST_PATTERN_TRACE_;


/****************************************************************************
    MACRO
****************************************************************************/
/*
    FOR TRACE
*/
#ifdef COSIM
#define TRACE_LINE               rTracer = ((__LINE__) | (0xc0de << 16))
#define TRACE_OUT(_OUT1_,_OUT2_,_OUT3_,_OUT4_)    rTracer = (U32)_OUT1_ , rTracer = (U32)_OUT2_ , rTracer = (U32)_OUT3_ , rTracer = (U32)_OUT4_
#else
#define TRACE_LINE               DBG_Printf("Line : %d\n",(U32)__LINE__)
#define TRACE_OUT(_OUT1_,_OUT2_,_OUT3_,_OUT4_)    DBG_Printf("0x%x : 0x%x : 0x%x : 0x%x\n",_OUT1_,_OUT2_,_OUT3_,_OUT4_)
#endif

#define TRACE_PATTREN_RECORD      TRACE_OUT((U32)__FILE__,(U32)__LINE__,(U32)__DATE__,(U32)__TIME__)

/*
    ADDRESS 
*/
/*
typedef struct NFC_COSIM_ARG
{
    U32 bsPuNum:7;
    U32 bsLun:3;
    U32 bsPage:10;
    U32 bsSecLen:7;
    U32 bsDataCheckEn:1;
    U32 bsDrqDwqTestEn:1;
    U32 bsWriteEn:1;
    U32 bsEraseEn:1;
    U32 bsReadEn:1;
}NFC_COSIM_ARG;


#if defined COSIM
#define TEST_PU_NUM                 (rNFC(0x2a0))
#define TEST_SEC_LEN                (rNFC(0x2a4))
#define TEST_DATA_CHECK_EN          (rNFC(0x2a8))
#define TEST_DISABLE_DXQ            (rNFC(0x2ac))
#define TEST_BLOCK_START            0
#define TEST_BLOCK_END              1//(BLK_PER_PLN)


#elif defined FPGA
#define TEST_PU_NUM                 1//4//(PU_NUM)
#define TEST_LUN_NUM                (PLN_PER_PU)
#define TEST_BLOCK_START            46
#define TEST_BLOCK_END              48//(BLK_PER_PLN)
#define TEST_SEC_LEN                (64)
#define TEST_DATA_CHECK_EN          (TRUE)
#define TEST_DISABLE_DXQ            (TRUE)

#else
#define TEST_PU_NUM                 (4)
#define TEST_SEC_LEN                (64)
#define TEST_DATA_CHECK_EN          (TRUE)
#define TEST_DISABLE_DXQ            (TRUE)
#endif
#define TEST_PAGE_NUM               1//(PG_PER_BLK)


*/
#define START_BUFFER_ID             ((DRAM_DATA_BUFF_MCU1_BASE - DRAM_START_ADDRESS)/PIPE_PG_SZ)
#define START_WBUF_ID               (START_BUFFER_ID)
#define START_RBUF_ID               (START_BUFFER_ID + TEST_NfcGetPuNum() * TEST_NfcGetPageNum())   // up to 4 page per PU in cosim
#define START_RED_POINTER_ADDR ((START_RBUF_ID + TEST_NfcGetPuNum() * TEST_NfcGetPageNum())*PIPE_PG_SZ + DRAM_START_ADDRESS)
#define START_RED_ADDR (START_RED_POINTER_ADDR + TEST_NfcGetPuNum() * TEST_NfcGetPageNum()*sizeof(U32))
#define START_RED_ADDR_ALIGN (START_RED_ADDR % PIPE_PG_SZ) ?  (START_RED_ADDR +PIPE_PG_SZ - (START_RED_ADDR % PIPE_PG_SZ)) : START_RED_ADDR
/*


#define START_BUFFER_ID             ((DRAM_DATA_BUFF_MCU1_BASE - DRAM_START_ADDRESS)/PIPE_PG_SZ)
#define START_WBUF_ID               (START_BUFFER_ID + 1)
#define START_RBUF_ID               (START_BUFFER_ID + TEST_NfcGetPuNum() * TEST_NfcGetPageNum())   // up to 4 page per PU in cosim
#define START_RED_POINTER_ADDR      (DRAM_DATA_BUFF_MCU1_BASE)
#define START_RED_ADDR              (START_RED_POINTER_ADDR + TEST_NfcGetPuNum() * TEST_NfcGetPageNum() *sizeof(U32))
#define START_RED_ADDR_ALIGN        (START_RED_ADDR % PIPE_PG_SZ) ?  (START_RED_ADDR +PIPE_PG_SZ - (START_RED_ADDR % PIPE_PG_SZ)) : START_RED_ADDR
*/

#define SSU0_BASE_OTFB              OTFB_SSU0_MCU1_BASE
#define CACHE_STATUS_BASE_OTFB      OTFB_CACHE_STATUS_MCU1_BASE
#define SSU0_DATA_TEST  ('S')
#define SSU1_DATA_TEST  ('U')
#define CACHE_STS_DATA_TEST  ('X')
#define CALC_TEST_DATA_FUNC_0(_x_)      (_x_ + 1)
#define CalcData(_a_,_b_,_c_,_d_) (((_a_+1) << 24)|((_b_+1) << 16)|((_c_+1) << 8)|(_d_+1))
#define READ_RED_OFFSET(_PU_,_LUN_) ((_PU_<<LUN_NUM_BITS)+_LUN_)

/*
    GLOBAL TIMING
*/
//#define NFC_TIMING_IFTYPE_MAX 2
//#define NFC_TIMING_MODE_MAX 4
extern ASYNC_TIMING_CONFIG t_aAsyncTConfTab[2][4];
/* REDUNDANT */
extern volatile NFC_RED **pRed;

/*
    GLOBAL EXTERNAL
*/
extern void COM_MemZero(U32* TargetAddr,U32 LengthDW);
extern void COM_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW);
extern void COM_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue);

/*
    NFC BASIC FUNCTION
*/
void GenPartReadAddr(U16 *pSecStart ,U16 *pSecLen);
U32 GetOffset(FLASH_ADDR *pFlashAddr);
static void SetSsu0Status(U16 SsuAddr, U32 value);
static void SetSsu1Status(U16 SsuAddr, U32 value);
static void SetCacheStatus(U32 CacheAddr, U32 value);
static U32 GetSsu0Status(U32 SsuAddr);
static U8 GetSsu1Status(U16 SsuAddr);
static U8 GetCacheStatus(U32 ulCacheAddr);
void PrepareData(FLASH_ADDR *pFlashAddr);
void CheckData(FLASH_ADDR *pFlashAddr,U16 usSecStart,U16 usSecLen);
void TEST_SetErrInj(FLASH_ADDR *pFlashAddr);
void TEST_Delay_ms(U32 ulMsCount);
void TEST_NfcBasicErase(void);
void TEST_CheckCS(FLASH_ADDR *pFlashAddr);
void TEST_NfcByteModeRead(void);
void TEST_NfcErrInj(void);
U32 TEST_NfcGetCacheStatusAddr(FLASH_ADDR *pFlashAddr);
void TEST_NfcNormalSetFeature(void);
void TEST_NfcPartialRead(void);
void TEST_NfcPioSetFeature(void);
void TEST_NfcSetSSU(NFCQ_ENTRY* pNFCQEntry, FLASH_ADDR *pFlashAddr);
void TEST_NfcSinglePlnEWR(void);
void TEST_NfcSSUandCS(void);
void TEST_NfcSSUandCsInit(void);
void TEST_ReadID(void);
void TEST_SetCs(NORMAL_DSG_ENTRY *pDSG,FLASH_ADDR *pFlashAddr);
void TEST_NfcBasicInit(void);
void TEST_NfcBasic(void);

/*
    NFC MULTIPU EWR FUNCTION
*/
static void DataInit(FLASH_ADDR* pFlashAddr);
static U16 GetWrBufId(FLASH_ADDR* pFlashAddr);
static U16 GetRdBufId(FLASH_ADDR* pFlashAddr);
U32 GetRedOffset(FLASH_ADDR *pFlashAddr);
static U32 GetDramAddr(U16 usBufId);
static void DataCheck(FLASH_ADDR* pFlashAddr);
void RedCheck(FLASH_ADDR *pFlashAddr,NFC_RED *pRed[]);
void TEST_NfcMultiPuErase(void);
void TEST_NfcMultiPuInitData(void);
void TEST_NfcMultiPuInitData(void);
void TEST_NfcMultiPuCacheWrite(void);
void TEST_NfcMultiPuNormalRead(void);
void TEST_NfcMultiPuCacheRead(void);
void TEST_NfcCacheRdAfterWr(void);
void TEST_NfcMultiPu(void);
#endif
