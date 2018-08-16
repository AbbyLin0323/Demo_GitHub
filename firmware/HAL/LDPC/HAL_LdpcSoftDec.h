
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
#ifndef __HAL_LDPC_SOFT_DEC_H__
#define __HAL_LDPC_SOFT_DEC_H__

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashCmd.h"
#include "HAL_LdpcEngine.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
/*Base Address Definition*/
#define LDPC_SOFT_MCU1_CFG_REG_BASE             (REG_BASE_LDPC + 0x04)     //0x1ff8,1904
#define LDPC_SOFT_MCU2_CFG_REG_BASE             (REG_BASE_LDPC + 0x08)     //0x1ff8,1908
#define LDPC_SOFT_INT_STS_REG                   (REG_BASE_LDPC + 0x0c)     //0x1ff8,190c

#define LDPC_SOFT_RDESADDR_BASE1                0x00    //0x1ff9,4000
#define LDPC_SOFT_RDESADDR_BASE2                0x200    //0x1ff9,4200
#define LDPC_SOFT_DESC_BASE_ADDR                (LDPC_ECM_BASE + LDPC_SOFT_RDESADDR_BASE1)

/*Parameter Definition*/
#define LDPC_SOFT_DEC_BOTH_MCU_MODE             FALSE
#define DESCRIPTOR_DEPTH_DW                     16
#define DESCRIPTOR_DEPTH_BYTE                   (DESCRIPTOR_DEPTH_DW << 2)
#define VALID_DESCRIPTOR_CNT                    (DESCRIPTOR_MAX_NUM - 1)
#define DESC_SHARED_CW_NUM                      4

#if(LDPC_SOFT_DEC_BOTH_MCU_MODE == FALSE)
    #define DESCRIPTOR_MAX_NUM                  16
#else
    #define DESCRIPTOR_MAX_NUM                  8
#endif

#define LDPC_SOFT_DEC_MAX_ITER                  20
#define LDPC_SOFT_DEC_TSB_THRD                  15
#define LDPC_SOFT_DEC_TSB_ACT_ITER              2

#define LDPC_SOFT_LLR_NUM                       6
#define LDPC_SOFT_MAX_DEC_CNT                   5//3

#define LDPC_SOFT_DEC_LAB_EN                    FALSE
#define LDPC_SOFT_DEC_RED_EN                    FALSE
#define SEC_PER_CW_BITS                         (CW_INFO_SZ_BITS - SEC_SZ_BITS)

#define REG_INT_STS_EN_DONE                     (1<<0)
#define REG_INT_STS_EN_FAIL                     (1<<1)
#define DECS_INT_STS_EN_DONE                    (1<<0)
#define DECS_INT_STS_EN_FAIL                    (1<<1)

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
/*Register Definition*/
/*Soft decoder mcu1 configure register*/
typedef struct _SOFT_MCU1_CFG_REG_
{
    U32 bsRDesAddrBase1 : 10;
    U32 bsRptr1         : 5;
    U32 bsRsv           : 1;
    U32 bsWptr1         : 5;
    U32 bsIntEn         : 2;
    U32 bsSoftMaxIter   : 8;
    U32 bsBothMcuMode   : 1;
}SOFT_MCU1_CFG_REG;

/*Soft decoder mcu2 configure register*/
typedef struct _SOFT_MCU2_CFG_REG_
{
    U32 bsRDesAddrBase2 : 10;
    U32 bsRptr2         : 5;
    U32 bsRsv0          : 1;
    U32 bsWptr2         : 5;
    U32 bsRsv1          : 11;
}SOFT_MCU2_CFG_REG;

/*Soft Decoding Interrupt status register*/
typedef struct _SOFT_INT_STS_REG_
{
    U32 bsSoftDecIntSts : 2;
    U32 bsRsv           : 30;
}SOFT_INT_STS_REG;

/*Soft DEC descriptor Definition*/
typedef struct _SOFT_DEC_DESCRIPTOR_
{
    /*DW0: 03-00H*/
    U32 bsValid         : 1;    //0x00[0]       R/W/U       0 to indicate the descriptor is invalid. 1 to indicate the descriptor is valid and wait for processing. HW writes 0 after finishing task of this descriptor
    U32 bsFailed        : 1;    //0x00[1]       RO          Indicate if Decode failed. 1 : failed, 0 : succeeded
    U32 bsIntStsEn      : 2;    //0x00[3:2]     R/W         Interrupt status enable. bits[2] : enable DONE status; [3] : enable FAIL
    U32 bsRdNum         : 3;    //0x00[6:4]     R/W         Number of read times for SV generation (1,2,3,4,5)
    U32 bsCwNum         : 3;    //0x00[9:7]     R/W         Number of code word. Maximum is 4
    U32 bsErrCnt1       : 11;   //0x00[20:10]   RO          The err_cnt of codeword1 in this descriptor
    U32 bsErrCnt2       : 11;   //0x00[31:21]   RO          The err_cnt of codeword2 in this descriptor
    /*DW1: 07-04H*/
    U32 bsDecSts        : 4;    //0x04[3:0]     RO          Decoding failure status. After one codeword is decoding done, the decoding failure status will be updated.
    U32 bsBitMap        : 4;    //0x04[7:4]     RO          Buff Map indication. 0: FW initial; 1: After one code word is decoding done, the related bit will be set 1.
    U32 bsRsv0          : 2;    //0x04[9:8]     RO          Reserved
    U32 bsErrCnt3       : 11;   //0x00[20:10]   RO          The err_cnt of codeword3 in this descriptor
    U32 bsErrCnt4       : 11;   //0x00[31:21]   RO          The err_cnt of codeword4 in this descriptor
    /*DW2: 0B-08H*/
    U32 bsIterNum1      : 8;    //0x08[7:0]     RO          Iteration number to decode these CW1(valid after CW1 decode)
    U32 bsIterNum2      : 8;    //0x08[15:8]    RO          Iteration number to decode these CW2
    U32 bsIterNum3      : 8;    //0x08[23:16]   RO          Iteration number to decode these CW3
    U32 bsIterNum4      : 8;    //0x08[31:24]   RO          Iteration number to decode these CW4
    /*DW3: 0F-0CH*/
    U32 bsLbaEn1        : 1;    //0x20[0]       RW          LBA enable
    U32 bsRedunEn1      : 1;    //0x20[1]       RW          Redundant enable
    U32 bsRedunLen1     : 3;    //0x20[4:2]     RW          Redundant length
    U32 bsLbaEn2        : 1;    //0x20[5]       RW          LBA enable
    U32 bsRedunEn2      : 1;    //0x20[6]       RW          Redundant enable
    U32 bsRedunLen2     : 3;    //0x20[9:7]     RW          Redundant length
    U32 bsLbaEn3        : 1;    //0x20[10]      RW          LBA enable
    U32 bsRedunEn3      : 1;    //0x20[11]      RW          Redundant enable
    U32 bsRedunLen3     : 3;    //0x20[14:12]   RW          Redundant length
    U32 bsLbaEn4        : 1;    //0x20[15]      RW          LBA enable
    U32 bsRedunEn4      : 1;    //0x20[16]      RW          Redundant enable
    U32 bsRedunLen4     : 3;    //0x20[19:17]   RW          Redundant length
    U32 bsLbaChkEn      : 1;    //0x20[20]      RW          LBA check enable
    U32 bsCrcStatus1    : 1;    //0x20[21]      RW          CRC status for CW1
    U32 bsRcrcStatus1   : 1;    //0x20[22]      RW          RCRC status for CW1
    U32 bsCrcStatus2    : 1;    //0x20[23]      RW          CRC status for CW2
    U32 bsRcrcStatus2   : 1;    //0x20[24]      RW          RCRC status for CW2
    U32 bsCrcStatus3    : 1;    //0x20[25]      RW          CRC status for CW3
    U32 bsRcrcStatus3   : 1;    //0x20[26]      RW          RCRC status for CW3
    U32 bsCrcStatus4    : 1;    //0x20[27]      RW          CRC status for CW4
    U32 bsRcrcStatus4   : 1;    //0x20[28]      RW          RCRC status for CW4
    U32 bsLbaErrStatus  : 1;    //0x20[29]      RW          LBA Error Status, 1: LBA check error 0: LBA check OK
    U32 bsRsv1          : 2;    //0x20[31:30]   RW          Reserve
    /*DW4: 13-10H*/
    U32 bsValueLLR0     : 5;    //0x0C[3:0]     RW          FW configure lookup table, for code 3'h0
    U32 bsValueLLR1     : 5;    //0x0C[7:4]     RW          FW configure lookup table, for code 3'h1
    U32 bsValueLLR2     : 5;    //0x0C[11:8]    RW          FW configure lookup table, for code 3'h2
    U32 bsValueLLR3     : 5;    //0x0C[15:12]   RW          FW configure lookup table, for code 3'h3
    U32 bsValueLLR4     : 5;    //0x0C[19:16]   RW          FW configure lookup table, for code 3'h4
    U32 bsValueLLR5     : 5;    //0x0C[23:20]   RW          FW configure lookup table, for code 3'h5
    U32 bsRsv2          : 2;    //0x0C[31:24]   RW          Reserved
    /*DW5: 17-14H*/
    U32 bsAddrLLR0      : 5;    //0x10[4:0]     RW          Address for LLR0
    U32 bsAddrLLR1      : 5;    //0x10[9:5]     RW          Address for LLR1
    U32 bsAddrLLR2      : 5;    //0x10[14:10]   RW          Address for LLR2
    U32 bsAddrLLR3      : 5;    //0x10[19:15]   RW          Address for LLR3
    U32 bsAddrLLR4      : 5;    //0x10[24:20]   RW          Address for LLR4
    U32 bsAddrLLR5      : 5;    //0x10[29:25]   RW          Address for LLR5
    U32 bsChnN          : 2;    //0x10[31:30]   RW          Soft DEC channel number, 00: channel1, 01: channel2...
    /*DW6: 1B-18H*/
    U32 bsInfoLen1      : 11;   //0x14[10:0]    RW          CW1 Info length
    U32 bsHSel1         : 2;    //0x14[12:11]   RW          CW1 H Matrix select
    U32 bsInfoLen2      : 11;   //0x14[23:13]   RW          CW2 Info length
    U32 bsHSel2         : 2;    //0x14[25:24]   RW          CW2 H Matrix select
    U32 bsRsv3_UsebyFw  : 6;    //0x14[31:26]   RW          Reserve and Borrowed by FW
    /*DW7: 1F-1CH*/
    U32 bsInfoLen3      : 11;   //0x18[10:0]    RW          CW3 Info length
    U32 bsHSel3         : 2;    //0x18[12:11]   RW          CW3 H Matrix select
    U32 bsInfoLen4      : 11;   //0x18[23:13]   RW          CW4 Info length
    U32 bsHSel4         : 2;    //0x18[25:24]   RW          CW4 H Matrix select
    U32 bsRsv4          : 6;    //0x18[31:26]   RW          Reserve
    /*DW8: 23-20H*/
    U32 bsScrEn         : 1;    //0x1C[0]       RW          Scramble enable
    U32 bsEmEn          : 1;    //0x1C[1]       RW          EM enable
    U32 bsScrShfNum     : 9;    //0x1C[10:2]    RW          Scramble shift number
    U32 bsDramCrcEn     : 1;    //0x1C[11]      RW          Enable DRAM CRC check
    U32 bsCrcEn         : 1;    //0x1C[12]      RW          NFC CRC enable
    U32 bsDramOtfbSel   : 1;    //0x1C[13]      RW          Write the soft decoded data to OTFB or DRAM
    U32 bsCwId          : 4;    //0x1C[17:14]   RW          CodeWord ID
    U32 bsRsv5          : 14;   //0x1C[31:18]   RW          Reserve
    /*DW9: 27-24H*/
    U32 ulLba;                  //0x24[31:0]    RW          Scramble seed for NFDMAC
    /*DW10: 2B-28H*/
    U32 ulWADDR;                //0x1C[31:0]    RW          Write address of the codeword
    /*DW11: 2F-2CH*/
    U32 aReadAddr[LDPC_SOFT_MAX_DEC_CNT];        // 0x20~0x30[31:0]    RW          5 address of read data
}SOFT_DEC_DESCRIPTOR;

typedef struct _DESCRIPTOR_
{
    SOFT_DEC_DESCRIPTOR aDescriptor[DESCRIPTOR_MAX_NUM];
}DESCRIPTOR;

typedef struct _CW_CFG_DESCRIPTOR_
{
    U32 bsInfoLen   : 11;   //[10:0]        RW          CW1 Info length
    U32 bsHSel      : 2;    //[12:11]       RW          CW1 H Matrix select
    U32 bsLbaEn     : 1;    //[13]          RW          LBA enable
    U32 bsRedunEn   : 1;    //[14]          RW          Redundant enable
    U32 bsRedunLen  : 3;    //[17:15]       RW          Redundant length
    U32 bsRsv       : 14;
}CW_CFG_DESCRIPTOR;

typedef struct _DESC_EN_CTL_
{
    /* DW0 */
    U32 bsIntStsEn          : 2;
    U32 bsLbaEn             : 1;
    U32 bsRedunEn           : 1;
    U32 bsLbaChkEn          : 1;
    U32 bsScrEn             : 1;
    U32 bsEmEn              : 1;
    U32 bsDramCrcEn         : 1;
    U32 bsCrcEn             : 1;
    U32 bsDramOtfbSel       : 1;
    U32 bsDescCnt           : 8;
    U32 bsDecTime           : 8;
    U32 bsRsv               : 6;

    /* DW1 */
    U32 bsShiftReadTimes    : 8;
    U32 bsScrShfNum         : 9;
    U32 bsRsv1              : 24;
    /* DW2 */
    U32 ulLba;
}DESC_EN_CTL;

// LDPC Soft-Decode Code-Word segment descriptor
typedef struct _LDPC_SOFT_DECODE_CODE_WORD_SEGMENT
{
    U8 bsStartCodeWord  : 5;
    U8 bsCodeWordCount  : 3;
}SOFT_DECO_CW_SEGMENT;

// LDPC RAW Data read use diff Vth from ReadRetry
typedef struct _LDPC_SOFT_DECODE_RETRY_DATA_
{
    U32 bsFrontData : 8;
    U32 bsLastData  : 8;
}LDPC_SOFT_DECODE_RETRY_DATA;

// LDPC SoftDEC Descriptors Releasing Status
typedef enum _LDPC_SOFT_DEC_DESC_STS_
{
    LDPC_SOFT_DECO_RELEASED = 0,
    LDPC_SOFT_DECO_WAIT_RELEASE
}LDPC_SOFT_DEC_DESC_STS;

// LDPC LLR Value & Address
typedef struct _LDPC_LLR_ENTRY_
{
    //DW0
    U32 bsValueLLR0 : 5;
    U32 bsValueLLR1 : 5;
    U32 bsValueLLR2 : 5;
    U32 bsValueLLR3 : 5;
    U32 bsValueLLR4 : 5;
    U32 bsValueLLR5 : 5;
    U32 bsRsv0      : 2;
    //DW1
    U32 bsAddrLLR0  : 5;
    U32 bsAddrLLR1  : 5;
    U32 bsAddrLLR2  : 5;
    U32 bsAddrLLR3  : 5;
    U32 bsAddrLLR4  : 5;
    U32 bsAddrLLR5  : 5;
    U32 bsRsv1      : 2;
}LDPC_LLR_ENTRY;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSInit(void);
U8 MCU2_DRAM_TEXT HAL_LdpcSGetFifoHwRptr();
BOOL MCU2_DRAM_TEXT HAL_LdpcSFifoIsEmpty();
BOOL MCU2_DRAM_TEXT HAL_LdpcSFifoWaitEmpty();
BOOL MCU2_DRAM_TEXT HAL_LdpcSFifoIsFull();
U8 MCU2_DRAM_TEXT HAL_LdpcSFifoGetFreeNum();
U8 MCU2_DRAM_TEXT HAL_LdpcSFifoGetDoneNum();
void MCU2_DRAM_TEXT HAL_LdpcSFifoPush(U8 ucNewDescNum);
void MCU2_DRAM_TEXT HAL_LdpcSFifoPop();
U32 MCU2_DRAM_TEXT HAL_LdpcSGetDescId();
volatile SOFT_DEC_DESCRIPTOR * MCU2_DRAM_TEXT HAL_LdpcSGetDescAddr(volatile U32 DescID);
void MCU2_DRAM_TEXT HAL_LdpcSDescCfgCw(DESC_EN_CTL *pDescEnCtl, U8 ucStartCwId, U8 ucDescCwCnt);
BOOL MCU2_DRAM_TEXT HAL_LdpcSoftDecRdDecSts(volatile U8 ucCurDescId);

void MCU2_DRAM_TEXT HAL_LdpcSTrigger(U32 ulDescriptorCnt);
void MCU2_DRAM_TEXT HAL_LdpcSDescRelease(U32 ulStartDescriptorId, U32 ulDescriptorCnt);
U32 MCU2_DRAM_TEXT HAL_LdpcSGetReadCnt(void);
U32 MCU2_DRAM_TEXT HAL_LdpcSGetReadTime(U32 ulShiftReadTimesIndex);
void MCU2_DRAM_TEXT HAL_LdpcSDescCfgCws(U8 ucPU, U8 ucCurDescId, DESC_EN_CTL *pDescEnCtl);
void MCU2_DRAM_TEXT HAL_LdpcSDescCfg(U8 ucPU, U8 ucCurDescId, DESC_EN_CTL *pDescEnCtl);
void MCU2_DRAM_TEXT HAL_LdpcSDescCfgLlr(U32 ulDescriptorId, U32 ulShiftReadTimes);

BOOL MCU2_DRAM_TEXT HAL_LdpcSIsDone(U32 ulDescriptorId);
BOOL MCU2_DRAM_TEXT HAL_LdpcSAllDone(U32 ulStartDescriptorId, U32 ulDescriptorCnt);
BOOL MCU2_DRAM_TEXT HAL_LdpcSAllSuccess(U32 ulStartDescriptorId, U32 ulDescriptorCnt);
BOOL MCU2_DRAM_TEXT HAL_LdpcSCrcStsCheck(U32 ulStartDescriptorId, U32 ulDescriptorCnt);

RETRY_TABLE MCU2_DRAM_TEXT HAL_LdpcSoftDecGetShiftRdParaTab(U8 ucCurrentShiftRdTime, U8 ucTotalShiftRdTime, BOOL bTlcMode, U8 ucWLType, U8 ucPageType, U8 ucLevel);
LDPC_LLR_ENTRY MCU2_DRAM_TEXT HAL_LdpcSoftDecGetLLR(U8 ucTotalShiftRdTime);
U32 MCU2_DRAM_TEXT HAL_LdpcSoftDecVthCal(U32 ulSoftDecVth, U32 ulRetryVth);
#endif
/*====================End of this head file===================================*/
