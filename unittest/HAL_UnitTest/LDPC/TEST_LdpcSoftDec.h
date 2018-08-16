
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
* File Name    : HAL_LdpcSoftDec.h
* Discription  : 
* CreateAuthor : Maple Xu
* CreateDate   : 2016.01.15
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef __TEST_LDPC_SOFT_DEC_H__
#define __TEST_LDPC_SOFT_DEC_H__

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashCmd.h"
#include "HAL_LdpcEngine.h"
#include "HAL_LdpcSoftDec.h"
#include "TEST_NfcFuncBasic.h"
#include "TEST_NfcExtDataCheck.h"


/*  Soft dec related  */
#define FAKE_CW_BITMAP                      0xF //need 10-desc
//#define LDPC_SOFT_NORMAL
//#define DESC_FIFO_TEST
#define LDPC_SOFT_CWCHK
//#define LDPC_RANDOM_PATTERN
#ifndef NFC_ERR_INJ_EN
//#define LDPC_SOFT_ERRINJ
#endif
//#define NFC_EM_EN

//#define SOFT_TLC_INTERNAL

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*Parameter Definication*/
#define LDPC_SOFT_LLR_NUM                   6
#define LDPC_SOFT_MAX_RDN_CNT               5
#define LDPC_SOFT_DEC_BUF_NUM               (LDPC_SOFT_MAX_RDN_CNT + 1)<<1 //5-rd & 1-wr, DRAM 2KB aligned
#define LDPC_SOFT_MAX_CODESEL               4


#define REG_INT_STS_EN_DONE                 (1<<0)
#define REG_INT_STS_EN_FAIL                 (1<<1)
#define DECS_INT_STS_EN_DONE                (1<<0)
#define DECS_INT_STS_EN_FAIL                (1<<1)

#define COMB_FAIL                           FAIL
#define COMB_SUCCESS                        2

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/



typedef enum _LDPC_SOFT_DEC_STATUS_
{
    SOFT_DEC_STATE_INIT = 0,
    SOFT_DEC_STATE_READRETRYING,
    SOFT_DEC_STATE_BEGIN_DEC,
    SOFT_DEC_STATE_CHECKSTATUS,
    SOFT_DEC_STATE_RETRYFAIL,
    SOFT_DEC_STATE_RETRYSUCCESS     
}LDPC_SOFT_DEC_STATUS;

/*Soft DEC descriptor Definication*/
typedef struct _SOFT_DEC_STATUS_REPORT_
{
    /*DW0: 03-00H*/
    U32 bsRdNum:3;      //0x00[6:4]     R/W     Number of read times for SV generation (1,2,3,4,5)
    U32 bsCwNum:3;      //0x00[9:7]     R/W     Number of code word. Maximum is 4
    U32 bsErrCnt1:11;   //0x00[20:10]   RO      The err_cnt of codeword1 in this descriptor
    U32 bsErrCnt2:11;   //0x00[31:21]   RO      The err_cnt of codeword2 in this descriptor
    U32 bsRsv0:4;
    /*DW1: 07-04H*/
    U32 bsErrCnt3:11;   //0x00[20:10]   RO      The err_cnt of codeword3 in this descriptor
    U32 bsErrCnt4:11;   //0x00[31:21]   RO      The err_cnt of codeword4 in this descriptor
    U32 bsRsv1:10;      //0x04[9:8]     RO      Reserved
    /*DW2: 0B-08H*/
    U32 bsIterNum1:8;   //0x08[7:0]     RO      Iteration number to decode these CW1(valid after CW1 decode)
    U32 bsIterNum2:8;   //0x08[15:8]    RO      Iteration number to decode these CW2
    U32 bsIterNum3:8;   //0x08[23:16]   RO      Iteration number to decode these CW3
    U32 bsIterNum4:8;   //0x08[31:24]   RO      Iteration number to decode these CW4
    /*DW3: 0F-0CH*/
    U32 ulCwWADDR;      //0x0C[31:0]    RO      Corrected CW DRAM Address
}SOFT_DEC_STATUS_REPORT;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/


U32 MCU12_DRAM_TEXT TEST_LdpcSRdBitMap(FLASH_ADDR * pFlashAddr);
BOOL MCU12_DRAM_TEXT TEST_LdpcSFindSeq1(U32 ulBitMap, U32 *ptStartBit, U8 *ptBit1Num, U8 *ptBitPos);
BOOL MCU12_DRAM_TEXT TEST_LdpcSCwAssign(FLASH_ADDR * pFlashAddr);
U8 MCU12_DRAM_TEXT TEST_LdpcSTotalDescCnt(void);
U32 MCU12_DRAM_TEXT TEST_LdpcSGetDmaAddr(U32 BuffID, U32 ulSecInBuf);
void MCU12_DRAM_TEXT TEST_LdpcSGetDescRWAddrs(U32 DescID, U32 BuffID, U8 ucSecStart);
U8 MCU12_DRAM_TEXT TEST_LdpcSChkAllSts(U8 DescNum, U8 DecTime);
void MCU12_DRAM_TEXT TEST_LdpcSReportSts(U8 ucChkSts, U8 DescId, U8 DecTime);
U32 MCU12_DRAM_TEXT TEST_LdpcSCwRawRd(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, BOOL bOTFBBuff);
void MCU12_DRAM_TEXT TEST_LdpcSShiftRead(FLASH_ADDR * pFlashAddr, U32 BuffID, U8 ucDescCnt, U8 ucRetryTime);
void MCU12_DRAM_TEXT TEST_LdpcSShiftReadCw(FLASH_ADDR * pFlashAddr, U32 BuffID, U8 ucDescCnt, U8 ucRetryTime);
void MCU12_DRAM_TEXT TEST_LdpcSShiftReadAll(FLASH_ADDR * pFlashAddr, U32 BuffID, U8 ucRetryTime, U8 ucPageType, BOOL bTlcMode);
void MCU12_DRAM_TEXT TEST_LdpcSCwErrInj(U32 BuffID, U8 ucSecStart, U8 ucRetryTime, U8 ucFlipDwCnt);
BOOL MCU12_DRAM_TEXT TEST_LdpcSDataChk(FLASH_ADDR *pFlashAddr, U32 ulCurDescId, U8 ucPageType);

#endif

/*====================End of this head file===================================*/

