/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
Filename    : HAL_DecStsReport.h
Version     : Ver 1.0
Author      : Abby
Date        : 2016.03.10
Description : this file declare interface for DEC status report, both include 
              DEC FIFO and DEC SRAM module
Others      : 
Modify      :
    20160310    abby     create
*******************************************************************************/
#ifndef __HAL_DEC_STATUS_REPORT_H__
#define __HAL_DEC_STATUS_REPORT_H__

/*------------------------------------------------------------------------------
        INCLUDING FILES
------------------------------------------------------------------------------*/
#include "HAL_MemoryMap.h"
#include "COM_Memory.h"
#include "HAL_FlashDriverBasic.h"

/*------------------------------------------------------------------------------
        MACRO DECLARATION
------------------------------------------------------------------------------*/
#define DEC_STS_SRAM_BASE           DEC_STATUS_BASE
#define DEC_STS_FIFO_BASE           DEC_FIFO_BASE
#define DEC_FIFO_CFG_REG            (REG_BASE_NDC + 0x9C)

#define DEC_PLN_MAX_PER_LUN_BIT     2
#define DEC_PLN_MAX_PER_LUN         (1 << DEC_PLN_MAX_PER_LUN_BIT) //4
#define DEC_FIFO_DEPTH              128

/*------------------------------------------------------------------------------
        DECLARATION OF REGISTER STRUCTURE
------------------------------------------------------------------------------*/

/*--------------------------  DEC SRAM start ---------------------------------*/

/*  4DWs DEC STATUS ENTRY stored in DEC SRAM */
typedef struct _DEC_SRAM_STATUS_ENTRY_
{
    /*  DW0  */
    U32 ulDecFailBitMap;        //ECC decode or CRC failure, 0-no fail, 1-normal CRC fail or decode fail

    /*  DW1  */
    U32 ulRCrcBitMap;           //0-no R_CRC fail, 1-R_CRC fail

    /*  DW2  */
    U32 bsN1Accu:17;            //Accumulation of N1, at most 8 CWs
    U32 bsFstItrSyndAccu:14;    //Accumulation of 1st Iteration syndrome, at most 8 CWs
    U32 bsRsv : 1;

    /*  DW3  */
    U32 bsErrCntAcc0T1:16;      //Error bit count of 0->1
    U32 bsErrCntAcc:16;         //Error bit count 
}DEC_SRAM_STATUS_ENTRY;

/*  ID Entry stored in DEC SRAM */
typedef struct _FLASH_ID_ENTRY_
{
    U32 ulFlashId0;
    U32 ulFlashId1;
    U32 ulRsv1;
    U32 ulRsv2;
}FLASH_ID_ENTRY;

/*  FLASH STATUS Entry stored in DEC SRAM */
typedef struct _FLASH_STATUS_ENTRY_
{
    U32 bsFlashStatus:8;
    U32 bsRsv0:24;
    U32 ulRsv1;
    U32 ulRsv2;
    U32 ulRsv3;
}FLASH_STATUS_ENTRY;

/*   DEC SRAM UNION ENTRY  */
typedef union _DEC_STATUS_SRAM_
{
    DEC_SRAM_STATUS_ENTRY aDecStsSram[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH_TOTAL/NFC_LUN_PER_PU][DEC_PLN_MAX_PER_LUN];
    FLASH_ID_ENTRY aFlashId[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH_TOTAL/NFC_LUN_PER_PU][DEC_PLN_MAX_PER_LUN];    //just pln0 valid
    FLASH_STATUS_ENTRY aFlashStatus[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH_TOTAL/NFC_LUN_PER_PU][DEC_PLN_MAX_PER_LUN]; //just pln0 valid
}DEC_STATUS_SRAM;
/*--------------------------  DEC SRAM end -----------------------------------*/

/*--------------------------  DEC FIFO start ---------------------------------*/
/*  3DWs DEC STATUS ENTRY stored in DEC FIFO */
typedef struct _DEC_FIFO_STATUS_ENTRY_
{
    /*  DW0  */
    U32 ulDecEngBitMap;         //0-OTF dec, 1-Hard+ dec

    /*  DW1  */
    U32 bsErrCntAcc0T1:16;      //Error bit count of 0->1
    U32 bsErrCntAcc:16;         //Error bit count 

    /*  DW2  */
    U32 bsLdpcDecItr:13;        //Accumulation of LDPC decording Iteration, at most 32 CWs
    U32 bsPlnNum:2;
    U32 bsCmdIndex: 8;          //Entry index = {bsCmdIndex, bsPlnNum}
    U32 bsRsv0:9;
}DEC_FIFO_STATUS_ENTRY;

typedef struct _DEC_FIFO_STATUS_
{
    DEC_FIFO_STATUS_ENTRY aDecFifoSts[DEC_FIFO_DEPTH];
}DEC_FIFO_STATUS;

/* 0x1ff8009c */
typedef struct _XOR_DEC_FIFO_CFG_REG_
{   
    union 
    {
        struct
        {
            U32 bsRedBaseForXorPar:17;  //Redundant base address for XOR parity page
            U32 bsRsv:7;
            U32 bsDecFifoRst:1;         //When write this bit, Write pointer will clear to 0
            U32 bsRsv1:7;
        };
        struct
        {
            U32 bsRsv2:24;
            U32 bsDecFifoWp:8;          //Write pointer for DEC_FIFO
        };
    };
}XOR_DEC_FIFO_CFG_REG;

/*--------------------------  DEC FIFO end -----------------------------------*/


/*------------------------------------------------------------------------------
        DECLARATION OF EXTERNAL VARIABLES
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR volatile DEC_STATUS_SRAM *g_pDecSramStsBase;
extern GLOBAL MCU12_VAR_ATTR volatile U8 g_ucDecFifoRp;

/*------------------------------------------------------------------------------
        DECLARATION OF FUNCTIONS
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_DecStsInit(void);
void MCU12_DRAM_TEXT HAL_DecSramGetDecStsEntry(FLASH_ADDR *pFlashAddr, DEC_SRAM_STATUS_ENTRY *pDecSramSts);
void MCU12_DRAM_TEXT HAL_DebugDecSramGetDecStsEntry(FLASH_ADDR *pFlashAddr, DEC_SRAM_STATUS_ENTRY *pDecSramSts, NFC_CMD_STS_REG CurNfcCmdSts);
BOOL MCU12_DRAM_TEXT HAL_DecSramGetFlashId(FLASH_ADDR * pFlashAddr, U32 *pID);
void MCU12_DRAM_TEXT HAL_DecSramGetFlashSts(FLASH_ADDR *pFlashAddr, FLASH_STATUS_ENTRY *pFlashSts);
BOOL HAL_DecFifoTrigNfc(NFCQ_ENTRY *pNFCQEntry, U8 ucDecFifoIndex);
BOOL HAL_DecFifoReadSts(void);
DEC_FIFO_STATUS_ENTRY* HAL_DecFifoGetStsEntry(U8 ucDecFifoIndex);
void MCU12_DRAM_TEXT HAL_DecSramGetDecStsEntryInErr(FLASH_ADDR *pFlashAddr, DEC_SRAM_STATUS_ENTRY *pDecSramSts);

#endif

