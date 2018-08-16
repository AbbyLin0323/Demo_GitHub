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
Filename    : HAL_LdpcEngine.h
Version     : Ver 1.0
Author      : Abby
Date        : 2015.12.21
Description : Basic LDPC driver relative register and interface declare.
Others      :
Modify      :
20151221    abby     Create file
*******************************************************************************/

#ifndef __HAL_LDPC_ENGINE_H__
#define __HAL_LDPC_ENGINE_H__

/*------------------------------------------------------------------------------
    INCLUDING FILES
------------------------------------------------------------------------------*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
/*    OTF & Hard+ CFG default value    */
#define OTF_MAX_INTERATION              0x14
#define OTF_TSB_ACT_INTERATION          0x2
#define HARD_MAX_INTERATION             0x14
#define HARD_TSB_ACT_INTERATION         0x2

/*    Download H matrix configure related    */
/*    matrix DW number */
#define ENC1_MAT_NUM                    848
#define ENC2_MAT_NUM                    2448
#define OTF_MAT_NUM                     944
#define SOFT_HARD_MAT_NUM               1550

/*    matrix start addr in DRAM */
#ifdef COSIM
#define DRAM_CODE_BASE_ADDR             0x600d0000
#else
#define DRAM_CODE_BASE_ADDR             0x500d0000
#endif

#define DRAM_CODE1_BASE_ADDR            (DRAM_CODE_BASE_ADDR)
#define DRAM_CODE2_BASE_ADDR            (DRAM_CODE1_BASE_ADDR + ENC1_MAT_NUM * 4)
#define DRAM_CODE3_BASE_ADDR            (DRAM_CODE2_BASE_ADDR + ENC2_MAT_NUM * 4)
#define DRAM_CODE4_BASE_ADDR            (DRAM_CODE3_BASE_ADDR + OTF_MAT_NUM  * 4)

/*------------------------------------------------------------------------------
   REGISTER LOCATION
------------------------------------------------------------------------------*/
/* NFC REG Base Address*/
#define rLDPC(_x_)                      (*((volatile U32*)(REG_BASE_LDPC + (_x_))))

#define LDPC_HARD_CFG_REG_BASE          (REG_BASE_LDPC + 0x10)    //0x1ff81910
#define LDPC_OTF_CFG_REG_BASE           (REG_BASE_LDPC + 0x14)    //0x1ff81914
#define LDPC_H_MAT_CFG_REG_BASE         (REG_BASE_LDPC + 0x18)    //0x1ff81918
#define LDPC_H_MAT_ADDR_REG_BASE        (REG_BASE_LDPC + 0x1C)    //0x1ff8191c
#define LDPC_RECC_THRE_REG_BASE         (REG_BASE_LDPC + 0x28)    //0x1ff81928

/*  Download matrix code sel  */
typedef enum _H_MATRIX_SEL_
{
    ENCODE_RAM1 = 1,
    ENCODE_RAM2,
    OTF_DEC_RAM,
    SOFT_HARD_DEC_RAM,
    LDPC_MAT_CNT
}H_MATRIX_SEL;

/* LDPC Hard+ decode config register */
typedef struct _HARD_DEC_CONF_REG_
{
    U32 bsHardDecMaxIntr    : 8;    // Max interaction number of Hard+ decoder
    U32 bsHardDecLLR0       : 5;    // SV value for 1'b0 in hard+ decode
    U32 bsHardDecLLR1       : 5;    // SV value for 1'b1 in hard+ decode
    U32 bsHardTsbEn         : 1;    // 1: Enable Hard+ decode TSB feature
    U32 bsHardTsbActIter    : 3;    // Hard+ decoder TSB active interaction number
    U32 bsRsv0              : 10;
}HARD_DEC_CONF_REG;

/* LDPC OTF decode config register */
typedef struct _OTF_CONF_REG_
{
    U32 bsOTFMaxIntr        : 8;    // Max interaction number of OTF decoder
    U32 bsOTFtoHardCtl      : 1;    // 1: Bypass; 0: Not bypass
    U32 bsOtfTsbEn          : 1;    // 1: Enable OTF decode TSB feature
    U32 bsOtfTsbActIter     : 3;    // OTF decoder TSB active interaction number
    U32 bsRsv0              : 19;
}OTF_CONF_REG;

/* LDPC RECC threshold config register */
typedef struct _LDPC_RECC_THRE_REG_
{
    U32 bsReccThr0  : 8;    // Code 0 RECC error count threshold
    U32 bsReccThr1  : 8;    // Code 1 RECC error count threshold
    U32 bsReccThr2  : 8;    // Code 2 RECC error count threshold
    U32 bsReccThr3  : 8;    // Code 3 RECC error count threshold
}LDPC_RECC_THRE_REG;

/* LDPC download H matrix config parameters */
typedef struct _DL_MAT_PARA_
{
    U32 bsCodeLen   : 12;
    U32 bsCodeStaAddr;
}DL_MAT_PARA;

/* LDPC download H matrix config register */
typedef struct _DL_MAT_COFG_REG_
{
    U32 bsDlHTrigger    : 1;    //Download Trigger, set by FW, auto cleared by HW when finish.
    U32 bsHCodeSel      : 3;    //Select download code RAM;1/2:encoder code; 3: OTF decoder code; 4/5: Soft/hard+ decoder code
    U32 bsHCodeLen      : 12;   //length of H matrix (8 bytes align)
    U32 bsRsv           : 16;
}DL_MAT_COFG_REG;

/* LDPC download H matrix 0 information reg */
typedef struct _DL_MAT0_INFO_REG_
{
    U32 bsH0InfoLen : 12;   //Information length of H matrix code0 (in bytes)
    U32 bsH0ParLen  : 12;   //Parity length of H matrix code0 (in bytes)
    U32 bsRsv       : 8;
}DL_MAT0_INFO_REG;

/*------------------------------------------------------------------------------
    VARIABLES
------------------------------------------------------------------------------*/
/* LDPC parity length in DS SRAM*/
extern GLOBAL U8 g_atLdpcMatTable[2][4];
/*------------------------------------------------------------------------------
   FUNCTION DECLARATION
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT HAL_LdpcReccInit(void);
void MCU2_DRAM_TEXT HAL_LdpcOtfHardInit(void);
void MCU2_DRAM_TEXT HAL_LdpcInit(void);
void MCU2_DRAM_TEXT HAL_LdpInitHMatix(H_MATRIX_SEL eCodeSel);
void MCU2_DRAM_TEXT HAL_LdpcMatParaInit(void);
void MCU2_DRAM_TEXT HAL_LdpcDownloadHMatix(void);

#endif //#ifndef __HAL_LDPC_ENGINE_H__


/*    end of this file    */
