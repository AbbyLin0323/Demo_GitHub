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
Filename    : HAL_FlashDriverExt.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.12
Description : this file declare interface for operation to Tensilica MCU core 
Others      : HAL_FlashDriverExt.h depends on HAL_FlashDriverBasic.h.
Modify      :
20140912    Gavin     Create file
20140916    Gavin     add function declaration
*******************************************************************************/
#ifndef __HAL_FLASHDRVIER_EXT_H__
#define __HAL_FLASHDRVIER_EXT_H__
#include "HAL_FlashChipDefine.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"
#ifndef HOST_SATA
#include "HAL_ChainMaintain.h"
#endif

/*NFC interrupt acc*/
#define NF_INTACC_PENDING           0x100
#define NF_INTACC_BWCQ              0x80
#define NF_INTACC_CQPOS             0x40
#define NF_INTACC_PUMSK             0x3f

// Read and write the specified location value of the register(DW)
#define MULTI_VALUE_1(bits)                  (~((((U32)INVALID_8F)>>(bits))<<(bits)))
#define REG_SET_VALUE(reg,bits,value,offset) ((reg) = ((((reg)&=~(MULTI_VALUE_1(bits)<<(offset)))),((reg)|=((value)<<(offset)))))
#define REG_GET_VALUE(reg,bits,offset)       (((reg)>>(offset)) & MULTI_VALUE_1(bits))

/* Base address 0x1ff81200*/
typedef struct _NF_HPUST_REG{    
    /*DW 0 : NF Status Registers for all Channels (60-63h)  */
    U32 bsAllFull:1; 
    U32 bsres0_0:7;
    U32 bsAnyError:1;
    U32 bsres0_1:7;
    U32 bsAllDone:1; 
    U32 bsres0_2:15;
}NF_HPUST_REG;

typedef struct _NF_PPUST_REG{

    /*Physical PU status registers (70-7Fh)*/
    U32 ulEmptyBitMap;
    U32 ulFullBitMap;
    U32 ulIdleBitMap;
    U32 ulErrorBitMap;
}NF_PPUST_REG;

typedef struct _NF_LPUST_REG{
    /* Logic PU status registers (80-8Fh) */
    U32 ulEmptyBitMap;
    U32 ulFullBitMap; 
    U32 ulIdleBitMap;
    U32 ulErrorBitMap;   
}NF_LPUST_REG;

typedef struct _NF_PUACC_TRIG_REG{
    U32 bsTrig:1;   // when FW write 1 to bit 0, update all PUs status to LPUST
    U32 bsRsv:31;
}NF_PUACC_TRIG_REG;

typedef struct _PUFS_REG{
    U8 bsLevel0:1;
    U8 bsLevel1:1;
    U8 Resv:6;
}PUFS_REG;

typedef struct _NF_PUFS_REG{
    PUFS_REG PufsReg[CE_MAX];
}NF_PUFS_REG;

typedef struct _NF_PUFSB_REG{
    U32 ulPhyPufsbReg;
    U32 ulLogPufsbReg;
}NF_PUFSB_REG;

typedef union _NFC_CHAIN_NUM_REG_{
    U32 ulChainNumReg;
    struct {
        U32 bsValid:1;
        U32 bsClear:1;
        U32 bsHID:6;
        U32 bsChainNum:13;
        U32 bsRsv:11;
    };
}NFC_CHAIN_NUM_REG;

typedef struct ASYNC_TIMING_CONFIG__
{
    U32 ulNFC04 ;
    U32 ulNFC08 ;
    U32 ulNFC2C ;
    U32 ulNFC30 ;
    U32 ulNFC80 ;
    U32 ulNFC88 ;
    U32 ulPMU20 ;
}ASYNC_TIMING_CONFIG;

/* VD NFC defines */
typedef struct _VD_NFC_STATUS
{
    U16 usBlock;
    union
    {
        U16 usPage;
        struct
        {
            U16 usSppo:9;
            U16 usPln:2;
            U16 rsv:5;
        };
    };
    U32 ulStatus;
} VD_NFC_STATUS;

typedef struct _VD_NFC_REPORT_STATUS
{
    VD_NFC_STATUS vd_nfc_status[CE_MAX];
} VD_NFC_REPORT_STATUS;

typedef enum _NFC_LOGIC_PU_BITMAP_TYPE_
{
    LOGIC_PU_BITMAP_ERROR = 0,
    LOGIC_PU_BITMAP_IDLE,
    LOGIC_PU_BITMAP_NOTFULL,
    LOGIC_PU_BITMAP_EMPTY,
    LOGIC_PU_BITMAP_FINISH
} PU_BITMAP_TYPE;

void HAL_FlashInit(void);
void HAL_NfcSetFullPageRowAddr(NFCQ_ENTRY* pNFCQEntry, FLASH_ADDR *pFlashAddr);
U32 HAL_NfcPageRead(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U8 ucSecLength, U16 usBuffID, NFC_RED **ppNfcRed);
U32 HAL_NfcRedOnlyRead(FLASH_ADDR *pFlashAddr, NFC_RED **ppNfcRed);
U32 HAL_NfcFullPageWrite(FLASH_ADDR *pFlashAddr, U16 usBuffID, NFC_RED *pNfcRed);
U32 HAL_NfcFullBlockErase(FLASH_ADDR *pFlashAddr);

#ifndef HOST_SATA
U32 HAL_NfcHostPageWrite(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId, NFC_RED *pNfcRed);
U32 HAL_NfcHostPageRead(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId,
                       U8 ucSecStart, U8 ucSecLength, NFC_RED **ppNfcRed);
#endif


void HAL_NfcInitChainCnt(void);
void HAL_NfcFinishChainCnt(U8 HID, U16 TotalChain);
void HAL_NfcHelpFinishChainCnt(U8 HID);
void HAL_NfcStopChainCnt(U8 HID);
void HAL_NfcUpdLogicPUSts(void);
U32  HAL_NfcGetLogicPuBitMap(PU_BITMAP_TYPE BitMapType);
void HAL_NfcAsyncTimingInit(U8,U8);


#endif//__HAL_FLASHDRVIER_EXT_H__
