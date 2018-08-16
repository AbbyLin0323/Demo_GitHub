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
Filename    : HAL_FlashDriverBasic.h
Version     : Ver 1.0
Author      : Tobey
Date        : 2014.09.05
Description : Basic flash driver relative register and interface declare.
Others      : 
Modify      :
20140905    Tobey     Create file
*******************************************************************************/

#ifndef __HAL_FLASH_DRIVER_BASIC_H__
#define __HAL_FLASH_DRIVER_BASIC_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashChipDefine.h"

#define CE_MAX               32
#define NFC_CH_TOTAL         4
#define NFC_CH_MSK           (NFC_CH_TOTAL-1)
#define NFC_PU_PER_CH        8
#define NFC_CH_NUM(PhyPU)     ((PhyPU)%NFC_CH_TOTAL)
#define PU_IN_CH(PhyPU)       ((PhyPU)/NFC_CH_TOTAL)

#ifndef VT3514_C0
#define NFCQ_DEPTH_BIT       1
#else
#define NFCQ_DEPTH_BIT       2
#endif 
#define NFCQ_DEPTH           (1 << NFCQ_DEPTH_BIT)

#define PRCQ_DEPTH_BIT       6
#define PRCQ_DEPTH           (1 << PRCQ_DEPTH_BIT)   

#define NFCQ_SEC_ADDR_COUNT  4
#define NF_RED_HW_MAX_ITEM   1024 //?? 
#define NF_RED_SW_MAX_ITEM   (NF_RED_HW_MAX_ITEM/PLN_PER_PU)

#define CDC_WAIT_TIME        10 // NOP instructions inserted in order to wait for hardware to update some interface register

/*
Base on NFC Interface Register Set  (Rev 0.2 ,2nd Draft,2013/11/22)
*/
#define rPMU(_x_)           (*((volatile U32*)(REG_BASE_PMU + (_x_))))
#define rNFC(_x_)           (*((volatile U32*)(REG_BASE_NDC + (_x_))))

#define rNfcPgCfg           (*((volatile U32*)(REG_BASE_NDC + 0x00)))
#define rNfcTCtrl0          (*((volatile U32*)(REG_BASE_NDC + 0x04)))
#define rNfcTCtrl1          (*((volatile U32*)(REG_BASE_NDC + 0x08)))
#define rNfcSsu0Base        (*((volatile U32*)(REG_BASE_NDC + 0x0c)))
#define rNfcSsu1Base        (*((volatile U32*)(REG_BASE_NDC + 0x10)))
#define rNfcRdEccErrInj     (*((volatile U32*)(REG_BASE_NDC + 0x14)))
#define rNfcPrgEccErrInj    (*((volatile U32*)(REG_BASE_NDC + 0x18)))
#define rNfcBypassCtl       (*((volatile U32*)(REG_BASE_NDC + 0x1c)))
    
#define rNfcDbgSigGrpChg0   (*((volatile U32*)(REG_BASE_NDC + 0x20)))
#define rNfcDbgSigGrpChg1   (*((volatile U32*)(REG_BASE_NDC + 0x24)))
#define rNfcDbgSigGrpChg2   (*((volatile U32*)(REG_BASE_NDC + 0x28)))
#define rNfcToggelModeInDDR (*((volatile U32*)(REG_BASE_NDC + 0x2c)))
#define rNfcEdoTCtrlECO     (*((volatile U32*)(REG_BASE_NDC + 0x30)))
#define rNfcByPass5         (*((volatile U8*)(REG_BASE_NDC + 0x34)))
#define rNfcDelayCtrl       (*((volatile U32*)(REG_BASE_NDC + 0x38)))
#define rNfcIntAcc          (*((volatile U32*)(REG_BASE_NDC + 0x3c)))
#define rNfcReadID0         (*((volatile U32*)(REG_BASE_NDC + 0x40)))
#define rNfcReadID1         (*((volatile U32*)(REG_BASE_NDC + 0x44)))
#define rNfcSetFeature      (*((volatile U32*)(REG_BASE_NDC + 0x48)))
    
#define rNfcLogicPuNumCh0Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x50)))
#define rNfcLogicPuNumCh0Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x54)))
#define rNfcLogicPuNumCh1Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x58)))
#define rNfcLogicPuNumCh1Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x5c)))
#define rNfcLogicPuNumCh2Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x60)))
#define rNfcLogicPuNumCh2Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x64)))
#define rNfcLogicPuNumCh3Pu0_3 (*((volatile U32*)(REG_BASE_NDC + 0x68)))
#define rNfcLogicPuNumCh3Pu4_7 (*((volatile U32*)(REG_BASE_NDC + 0x6c)))
    
#define rNfcChainNumMcu1    (*(volatile U32 *)(REG_BASE_NDC + 0x70))
#define rNfcChainNumMcu2    (*(volatile U32 *)(REG_BASE_NDC + 0x74))
    
#define rNfcClkGateCtrl0         (*((volatile U32*)(REG_BASE_NDC + 0x78)))
#define rNfcClkGateCtrl1         (*((volatile U32*)(REG_BASE_NDC + 0x7c)))
#define rNfcDDrDlyComp0          (*((volatile U32*)(REG_BASE_NDC + 0x80))) 
#define rNfcModeConfig           (*((volatile U32*)(REG_BASE_NDC + 0x84))) 
#define rNfcMisc                 (*((volatile U32*)(REG_BASE_NDC + 0x88)))
#define rNfcProgIOTimingCtrl1    (*((volatile U32*)(REG_BASE_NDC + 0x90)))
#define rNfcProgIOTimingCtrl2    (*((volatile U32*)(REG_BASE_NDC + 0x94)))
#define rNfcProgIOTimingCtrl3    (*((volatile U32*)(REG_BASE_NDC + 0x98)))
    // reserver  9c
#define rNfcReadDataOutputPIO1   (*((volatile U32*)(REG_BASE_NDC + 0xa0)))
#define rNfcReadDataOutputPIO2   (*((volatile U32*)(REG_BASE_NDC + 0xa4)))

/*Base Address*/
#define TRIGGER_REG_BASE     REG_BASE_NDC_TRIG
#define LOGIC_PU_REG_BASE    (REG_BASE_NDC + 0x50)
#define NFC_CMD_STS_BASE     (REG_BASE_NDC + 0x100)

#define HPUST_REG_BASE       (REG_BASE_NDC + 0x260)
#define PPUST_REG_BASE       (REG_BASE_NDC + 0x270)
#define LPUST_REG_BASE       (REG_BASE_NDC + 0x280)
#define PUFSB_REG_BASE       (REG_BASE_NDC + 0x290)
#define PUACC_TRIG_REG_BASE  (REG_BASE_NDC + 0x298)  
#define NF_QE_REG_BASE       (REG_BASE_NDC + 0x300)

/* NF command status define, sync with HW spec */
#define NF_SUCCESS                    0  // No error
#define NF_ERR_TYPE_USUP              1  // Un-recognized command/Invalid command
#define NF_ERR_TYPE_PRG               2  // Program error
#define NF_ERR_TYPE_ERS               3  // Erase error
#define NF_ERR_TYPE_UECC              4  // ECC unrecoverable error
#define NF_ERR_TYPE_RECC              5  // ECC recoverable error with error counter>threshold
#define NF_ERR_TYPE_NO_DEV            6  // No device detected
#define NF_ERR_TYPE_EMPTY_PG          7  // No error with empty page
#define NF_ERR_TYPE_EMPTY_PG_E0       8  // Some sector is empty,but some sector detect "0" bits
#define NF_ERR_TYPE_CRC               9  // CRC error when ECC_MSK on
#define NF_ERR_TYPE_TWO_UECC          12 // data & check sum all error 
#ifdef VT3514_C0                         //for VT3514_C0 NF_ERR added 3 type
#define NF_ERR_TYPE_FirstByte         13 //first byte check error;
#define NF_ERR_TYPE_PrePagePrg        14 // Previous page program fail.
#define NF_ERR_TYPE_PreCurPagePrg     15 //Previous page and current page both program fail. 
#endif

/* NFC relative function's return status*/
typedef enum _NFC_OPERATION_STATUS
{
    NFC_STATUS_FAIL = 0,
    NFC_STATUS_SUCCESS
}NFC_OPERATION_STATUS;

typedef enum _NFC_INIT_PARAM
{
    PGSIZE_4K = 0,
    PGSIZE_8K,
    PGSIZE_16K,
    PGSIZE_32K,

    BLKSIZE_64 = 0,
    BLKSIZE_128,
    BLKSIZE_256,
    BLKSIZE_512,
    BLKSIZE_1K,
    BLKSIZE_2K,
    BLKSIZE_4K,
    BLKSIZE_8K,

    ECCSEL_24 = 0,
    ECCSEL_40,
    ECCSEL_64,

    REDNUM_16BYTE = 0,
    REDNUM_32BYTE,
    REDNUM_48BYTE,
    REDNUM_64BYTE,

    PRCQDEPTH_4DW = 0,
    PRCQDEPTH_8DW,
    PRCQDEPTH_16DW,
    PRCQDEPTH_32DW,

    MULLUN_NONE = 0 ,
    MULLUN_2 ,
    MULLUN_4 ,
    MULLUN_8  ,

    NFTM_MODE1 = 0,
    NFTM_MODE2,
    NFTM_MODE3,
    NFTM_MODE4,

    /* Interface type */
    TOGGLE2 = 0,
    ONFI2
}NFC_INIT_PARAM;

typedef struct _NF_CQ_REG_CMDTYPE // only for modle test
{
    U8 CmdType[NFCQ_DEPTH];
}NF_CQ_REG_CMDTYPE;

typedef struct _NFC_SEC_ADDR
{
    U16 bsSecLength:8;
    U16 bsSecStart:8;
}NFC_SEC_ADDR;

#ifndef VT3514_C0
/* NFCQ register */
typedef struct _NFCQ_ENTRY
{
    union
    {
        struct//for real NFC design
        {
            /* DW0 byte 0 */
            U32 bsOntfEn:1;       // set - otfb. clear -  dram
            U32 bsDmaByteEn:1;    // set - byte. clear - sector
            U32 bsIntEn:1;        // set - enable NFC generate interrrupt to mcu after cmd process done. clear - disable it
            U32 bsPuEnpMsk:1;     // set - mask = disable scramble; clear - enable scramble
            U32 bsPuEccMsk:1;     // set - mask = disable ECC; clear - enable ECC( ECC valid when PuLdpcEn = 0 )
            U32 bsPuLdpcEn:1;     // set - enable LDPC
            U32 bsRedEn:1;        // enable red
            U32 bsRedOnly:1;      // set - NFC do NOT read sector data even though IO_DATA set in raw cmd phase 

            /* DW0 byte 1 */
            U32 bsSsu0En:1;       // set - enable ssu0 , clear - disable ssu0.
            U32 bsSsu1En:1;       // set - enable ssu1 , clear - disable ssu1. used for cache status
            U32 bsTrigOmEn:1;     // set - Need to trig OTFBM before data xfer.  clear -  Not need 
            U32 bsDsgEn:1;        // set - fetch dsg ,clear - not fetch dsg
            U32 bsInjEn:1;        // set - enable error injection  , clear -disableerror injection
            U32 bsErrTypeS:3;     // 8 kinds of error types
    
            /* DW0 byte 2~3 */
            U32 bsNcqMode:1;      // 0 = First data ready set after the first 4K data transfer done regardless of BCH decode result;
                                  // 1 = First data ready set after the first 1K data transfer done regardless of BCH decode result;
            U32 bsNcqNum:5;       // 32 tag, if we have more than 32 tags, fill bit[5] to TagExt in DW3
            U32 bsN1En:1;         // ? 
            U32 bsFstDsgPtr:9;    // Record First DSG ID for incidental case if need release the DSG 
        };

        struct//for Fake NFC design
        {
            U32 bsFakeNfcOntfEn: 1;  // 1 = Message data is stored in on-the-fly SRAM.
            U32 bsFakeNfcTrigOmEn: 1;// need trig OTFBM before data transfer.
            U32 bsFakeNfcDW0Rsvd: 6;
            U32 ucFakeNfcTotalSec: 8;//As Total data length in this command. The unit is sector.
            U32 usFakeNfcDw0Rsvd: 16;
        };
    };

    /* DW1~2 */
    union
    {  
        NFC_SEC_ADDR aSecAddr[NFCQ_SEC_ADDR_COUNT];     // for DMA mode ,DW 1 means sec start and length
        
        /* for dma byte mode, DW1 redefined, DW2 not used */
        struct                                         // for byte mode ,DW1 means byte addr and length
        {
            U32 bsByteAddr:16;                         //
            U32 bsByteLength:16;                       // 
            U32 ulByteRev1;                            // Not Used In Byte Mode
        };

        struct//for Fake NFC design
        {
            U32 ulFakeNfcDRAMAddr; // NFCQ DW1, used for Fake NFC as DRAM address for DMA.
            U32 ulFakeNFcRsvd;     // NFCQ DW2~15 is not used by Fake NFC
        };
    };
    
    /* DW3 */  
    U32 bsDmaTotalLength:8;       //in sector
    U32 bsRedAddr:16;             // 
    U32 bsBmEn:1;                 // buffer map enable
    U32 bsFstDataRdy:1;           // first data ready enable
    U32 bsTagExt:1;               // bit[5] of bsNcqNum field if we have more than 32 tags
    U32 bsAddSgqCntEn:1;          // 1 = enable NFC add chain cnt after on-the-fly program ok
    U32 bsRsv3:4;                 //

    /* DW4~5 */
    U8 bsScrambleSeed[8];         // 

    /* DW6 */
    U32 bsSsuAddr0:8;             //  ssu addr[2:9]   dw mode
    U32 bsSsuData0:8;
    U32 bsRev6:16;
    
    /* DW7 */
    U32 bsSsuAddr1:16;            // original cachestatus , reserved    byte mode
    U32 bsSsuData1:8;             // reserved
    U32 bsFCmdPtr:4;              // record the FcmdPtr for l3 error handle, added by jasonguo
    U32 bsRev7:4;
    
    /* DW8~15 */
    U32 aRowAddr[8];             // QE Addr group   (AQEE)
}NFCQ_ENTRY;

/* NFCQ control register */
typedef struct _PG_CONF_REG
{
    U32 PgSz:2;      // 00-4k ,01-8k ,10-16k ,11-32k
    U32 BlkSz:3;     // 000-64 ,001-128 ,010-256,011-512,100-1k,101-2k,110-4k,111-8k
    U32 EccSel:2;    // 00-Ecc24,01-Ecc40,10-Ecc64
    U32 IntMskEn:1;  // 1-mask ,0-not mask
    
    U32 RedNum:2;    // 00-16byte,01-32byte,10-48byte,11-64byte
    U32 PrcqDepth:3; // 000-4dw ,001-8dw,010-16dw,011-32dw,100-64dw
    U32 RbLoc:3;     // RB bit location in byte.(defined in SQEE. Will remove)
    
    U32 SFLoc:3;     // Success/Fail bit location in a byte.
    U32 EccTh:5;     // ECC error counter threshold. In bit.
                     // [4]: means X4 or not. If it is 1, {ECC_TH[3:0],2'b00} is the real threshold.
                     // [3:0]: Basic value of threshold.
    
    U32 IsMicron:1;  // It is ONFI device or not. 1-MICRON device, 0-Not MICRON device (if DDR_MODE=1, indicate it is toggle NAND).
    U32 BchEccET:2;  // BCHECC early termination enable
    U32 MulLun:2;    // Multi-LUN number:
                     // 00: not multi-LUN
                     // 01: 2 LUN in one target
                     // 10: 4 LUN in one target
                     // 11: 8 LUN in one target
    U32 SkipRed:1;   // 1: enable skip redundant data for LDPC
    U32 ChArbEn:1;   // 1: Arbitrate in 1K data. 0: Arbitrate in 256 byte data.
    U32 DisCmdHP:1;  // Disable command higher priority. 1-disable, 0-not disable.
    
}PG_CONF_REG;

/* NFC trigger register */
typedef struct _NFC_TRIGGER_REG
{
    U8 bsCmdType:2;  // 00 - write, 01 - read, 10 - erase, 11 - other
    U8 bsOtfb:1;     // 1 - data xfer through otfb , through dram
    U8 bsPio:1;      // 1 - PIO command, 0 - Normal command
    U8 bsRev:3;      // reserved
    U8 bsTrig:1;     // MCU write this register to inform command is filled.
}NFC_TRIGGER_REG;

/* NF CQ register*/
typedef struct _NFC_CMD_STS_REG
{
    U8 bsIdle:1;   // 1 - idle, 0 - not idle, idle indicate empty or hold by error
    U8 bsErrh:1;   // 1 - error hold, 0 - no error
    U8 bsEmpty:1;  // 1 - empty, 0 - not empty
    U8 bsFull:1;   // 1 - full, 0 - not full 
    U8 bsRdPtr:1;  // read pointer, NFCLK domain.
    U8 bsWrPtr:1;  // write pointer. HCLK domain.
    U8 bsIntSts:1; // interrupt status, MCU can write this bit or public interrupt bit to clear the status.
    U8 bsPrst:1;   // When read, it returns "ERRH" bit. When write, trun current PU's all level into available
                   // status.when ERRH is cleared. Only software could write this bit.    

    U8 bsErrSts:4; // when Errh == 1, thi indicate error type
    U8 bsFsLv0:1;  // Level 0 finishing status 
    U8 bsFsLv1:1;  // Level 1 finishing status 
    U8 bsRq0Pset:1;// When write, current PU's Wp and Rp is set to 1. ERRH is clear. HW has bug, diacard in multi-core mode.
    U8 bsRes:1;    // reserved

    U8 ucCmdTypeRsv[2]; // reserved
}NFC_CMD_STS_REG;

#else
/* NFCQ register */
typedef struct _NFCQ_ENTRY
{
    union
    {
        struct//for real NFC design
        {
            /* DW0 byte 0 */
            U32 bsOntfEn:1;       // set - otfb. clear -  dram
            U32 bsDmaByteEn:1;    // set - byte. clear - sector
            U32 bsIntEn:1;        // set - enable NFC generate interrrupt to mcu after cmd process done. clear - disable it
            U32 bsPuEnpMsk:1;     // set - mask = disable scramble; clear - enable scramble
            U32 bsPuEccMsk:1;     // set - mask = disable ECC; clear - enable ECC( ECC valid when PuLdpcEn = 0 )
            U32 bsPuLdpcEn:1;     // set - enable LDPC
            U32 bsRedEn:1;        // enable red
            U32 bsRedOnly:1;      // set - NFC do NOT read sector data even though IO_DATA set in raw cmd phase 

            /* DW0 byte 1 ~ 3*/
            U32 bsSsu0En:1;       // set - enable ssu0 , clear - disable ssu0.
            U32 bsSsu1En:1;       // set - enable ssu1 , clear - disable ssu1. used for cache status
            U32 bsTrigOmEn:1;     // set - Need to trig OTFBM before data xfer.  clear -  Not need 
            U32 bsDsgEn:1;        // set - fetch dsg ,clear - not fetch dsg
            U32 bsInjEn:1;        // set - enable error injection  , clear -disableerror injection
            U32 bsErrTypeS:4;     // 8 kinds of error types
            U32 bsNcqNum:6;       // 32 tag, if we have more than 32 tags, fill bit[5] to TagExt in DW3
            U32 bsFstDsgPtr:9;    // Record First DSG ID for incidental case if need release the DSG 
        };

        struct//for Fake NFC design
        {
            U32 bsFakeNfcOntfEn: 1;  // 1 = Message data is stored in on-the-fly SRAM.
            U32 bsFakeNfcTrigOmEn: 1;// need trig OTFBM before data transfer.
            U32 bsFakeNfcDW0Rsvd: 6;
            U32 ucFakeNfcTotalSec: 8;//As Total data length in this command. The unit is sector.
            U32 usFakeNfcDw0Rsvd: 16;
        };
    };

    /* DW1~2 */
    union
    {  
        NFC_SEC_ADDR aSecAddr[NFCQ_SEC_ADDR_COUNT];     // for DMA mode ,DW 1 means sec start and length
        
        /* for dma byte mode, DW1 redefined, DW2 not used */
        struct                                         // for byte mode ,DW1 means byte addr and length
        {
            U32 bsByteAddr:16;                         //
            U32 bsByteLength:16;                       // 
            U32 ulByteRev1;                            // Not Used In Byte Mode
        };

        struct//for Fake NFC design
        {
            U32 ulFakeNfcDRAMAddr; // NFCQ DW1, used for Fake NFC as DRAM address for DMA.
            U32 ulFakeNFcRsvd;     // NFCQ DW2~15 is not used by Fake NFC
        };
    };
    
    /* DW3 */  
    U32 bsDmaTotalLength:8;       // in sector
    U32 bsRedAddr:16;             // 16 byte align
    U32 bsBmEn:1;                 // buffer map enable
    U32 bsFstDataRdy:1;           // first data ready enable
    U32 bsN1En:1;                 // calculate N1 enable
    U32 bsAddSgqCntEn:1;          // enable count program success command number for AHCI OTFB write error handling
    U32 bsFstByteCheck:1;         // check first byte data every sector in read command
    U32 bsEMEn:1;                 // get AES seed for every sector, from redundant data in wrie command; form AES_SED SRAM in read command
    U32 bsNcqMode:1;              // 0 = First data ready set after the first 4K data transfer done regardless of BCH decode result;
                                  // 1 = First data ready set after the first 1K data transfer done regardless of BCH decode result;
    U32 bsOtfbBypass:1;           // OTFBM bypass data transfer to OTFB; set:by pass; Clear:not bypass

    /* DW4~5 */
    U8 bsScrambleSeed[8];         

    /* DW6 */
    U32 bsSsuAddr0:8;             // DW align
    U32 bsSsuData0:8;
    U32 bsRev6:16;
    
    /* DW7 */
    U32 bsSsuAddr1:16;            // byte align
    U32 bsSsuData1:8;             
    U32 bsFCmdPtr:4;              // record the FcmdPtr for l3 error handle, added by jasonguo
    U32 bsRev7:4;
    
    /* DW8~15 */
    U32 aRowAddr[8];             // QE Addr group   (AQEE)
}NFCQ_ENTRY;

/* NFCQ control register */
typedef struct _PG_CONF_REG
{
    U32 PgSz:2;         // 00-4k ,01-8k ,10-16k ,11-32k
    U32 FinishMode:1;   // 0: set FINISH before update SSU.
                        // 1: set FINISH after update SSU    
    U32 bsCloseHWCmdPatch:1; // 0:with HW patch '78' 1:without
    U32 ColAddrByte:1;  // 1- 5byte ,0 - 2byte    
    U32 EccSel:2;       // 00-Ecc24,01-Ecc40,10-Ecc64
    U32 IntMskEn:1;     // 1-mask ,0-not mask
    
    U32 RedNum:2;       // 00-16byte,01-32byte,10-48byte,11-64byte    
    U32 PrcqDepth:2;    // 00-4DW,01-8DW,10-16DW,11-32DW.
    U32 ScrRotEn:1;     // 0 - Scramble seed not rotate ,1 - rotate 
    U32 RbLoc:3;        // RB bit location in byte.(defined in SQEE. Will remove)

    U32 SFLoc:3;        // Success/Fail bit location in a byte.
    U32 EccTh:5;        // ECC error counter threshold. In bit.
                        // [4]: means X4 or not. If it is 1, {ECC_TH[3:0],2'b00} is the real threshold.
                        // [3:0]: Basic value of threshold.
                            
    U32 IsMicron:1;     // It is ONFI device or not. 1-MICRON device, 0-Not MICRON device (if DDR_MODE=1, indicate it is toggle NAND).
    U32 BchEccET:2;     // BCHECC early termination enable
    U32 MulLun:2;       // Multi-LUN number:
                        // 00: not multi-LUN
                        // 01: 2 LUN in one target
                        // 10: 4 LUN in one target
                        // 11: 8 LUN in one target
    U32 SkipRed:1;      // 1: enable skip redundant data for LDPC
    U32 ChArbEn:1;      // 1: Arbitrate in 1K data. 0: Arbitrate in 256 byte data.
    U32 DisCmdHP:1;     // Disable command higher priority. 1-disable, 0-not disable.
    
}PG_CONF_REG;

/* NFC trigger register */
typedef struct _NFC_TRIGGER_REG
{
    U16 bsCmdType:2;   // 00 - write, 01 - read, 10 - erase, 11 - other
    U16 bsInsert:1;    // 1 - Indicate command in this level could insert into other level's command
                      // 0 - Disable insert feature. 
    U16 bsPio:1;       // 1 - PIO command ,0 -Normal command
    U16 bsLun:2;       // Lun Index  0~3
    U16 bsLunEn:1;     // 1 -  Indicate command in this level is a multi-LUN command and could jump to other level.
                      // 0 -  Other command.    
    U16 bsCacheOp:1;   // 1-    Indicate command in this level is a cache read/write command. 0 - other command

    U16 bsCESel:1;
    U16 bsRsv:7;
}NFC_TRIGGER_REG;

/* NF CQ register*/
typedef struct _NFC_CMD_STS_REG
{
    U8 bsIdle:1;     // 1 - idle, 0 - not idle, idle indicate all HW levels cmd all done
    U8 bsErrh:1;     // 1 - error hold, 0 - no error
    U8 bsEmpty:1;    // 1 - empty, 0 - not empty
    U8 bsFull:1;     // 1 - full, 0 - not full 
    U8 bsRdPtr:2;    // Indicate which HW level the next read cmd pointer directing
    U8 bsWrPtr:2;    // Indicate which HW level the next write cmd pointer directing
    
    U8 bsErrSts:4;   // when Errh == 1, thi indicate error type
    U8 bsIntSts:1;   // interrupt status, MCU can write this bit or public interrupt bit to clear the status.
    U8 bsPrst:1;     // When read, it returns "ERRH" bit. When write, trun current PU's all level into available
                     // status.when ERRH is cleared. Only software could write this bit. 
    U8 bsRq0Pset:1;  // When write, current PU's Wp and Rp is set to 1. ERRH is clear. HW has bug, diacard in multi-core mode.
    U8 bsres:1;      // reserved
    
    U8 bsCmdType0:3; // Level 0 Cmd type
    U8 bsFsLv0:1;    // Level 0 finishing status 
    U8 bsCmdType1:3; // Level 1 Cmd type
    U8 bsFsLv1:1;    // Level 1 finishing status 
    U8 bsCmdType2:3; // Level 2 Cmd type
    U8 bsFsLv2:1;    // Level 2 finishing status 
    U8 bsCmdType3:3; // Level 3 Cmd type
    U8 bsFsLv3:1;    // Level 3 finishing status 
}NFC_CMD_STS_REG;
#endif

/* Redundant Area relative define */
typedef struct _NFC_RED
{
    U32 aContent[RED_SZ_DW * PLN_PER_PU];
}NFC_RED;

typedef union _NFC_RED_AREA
{
    NFC_RED aNfcRed[NF_RED_SW_MAX_ITEM];
}NFC_RED_AREA;

typedef struct _NFCQ
{
    NFCQ_ENTRY aNfcqEntry[CE_MAX][NFCQ_DEPTH];
}NFCQ;

/* command Trigger register define */
typedef struct _NFC_TRIGGER
{
    NFC_TRIGGER_REG aNfcTriggerReg[CE_MAX][NFCQ_DEPTH];
}NFC_TRIGGER;

typedef struct _NFC_CMD_STS
{
    NFC_CMD_STS_REG aNfcqCmdStsReg[CE_MAX];
}NFC_CMD_STS;

/* cmd type for nfc model use, SIM and XTMP env need */
typedef struct _NFC_SIM_CMDTYPE
{
    U8 aCmdType[CE_MAX][NFCQ_DEPTH];
}NFC_SIM_CMDTYPE;

/* CE to logic PU mapping register */
typedef struct _NFC_LOGIC_PU_REG
{    
    U8 bsLogicPU:5;
    U8 bsPuEnable:1;
    U8 bsRsvd:2;
}NFC_LOGIC_PU_REG;

typedef struct _NFC_LOGIC_PU
{
    NFC_LOGIC_PU_REG aNfcLogicPUReg[NFC_CH_TOTAL][NFC_PU_PER_CH];
}NFC_LOGIC_PU;

/* PRCQ(programmable raw cmd queue) relative define */
typedef struct _NFC_PRCQ_ELEM
{   
    U8 bsIndex:4;
    U8 bsAttribute:2;
    U8 bsGroup:2; 
}NFC_PRCQ_ELEM;

typedef struct _NFC_PRCQ_ENTRY
{
    NFC_PRCQ_ELEM aNfcPRCQElem[PRCQ_DEPTH];
}NFC_PRCQ_ENTRY;

typedef struct _NFC_PRCQ
{
    NFC_PRCQ_ENTRY aNfcPRCQEntry[CE_MAX][NFCQ_DEPTH];
}NFC_PRCQ;


#define QE_INDEX_NUM                16
#define QE_CMD_GROUP_DEPTH          16
#define QE_OPRATION_GROUP_DEPTH     8

/* PRCQ queue element field */
typedef struct _NFC_PRCQ_QE
{
    /* 1 cycle cmd  group */
    U8 aPRCQ_CQE1E[QE_CMD_GROUP_DEPTH];  

    /* 2 cycle cmd group */
    U16 aPRCQ_CQE2E[QE_CMD_GROUP_DEPTH];

    /* reserved */
    U32 aRsv[8];

    /* status group */
    U16 aPRCQ_SQEE[QE_OPRATION_GROUP_DEPTH];

}NFC_PRCQ_QE;

/* flash addr descriptor */
typedef struct _FLASH_ADDR
{
    U32 ucPU:8;
    U32 ucLun:8;
    U32 bsPln:4;
    U32 bsRes:12;    
    U32 usBlock:16;
    U32 usPage:16;
}FLASH_ADDR;

typedef struct _PU_BROTHER
{
    U8 aBrother[LUN_NUM];
}PU_BROTHER;

#if defined (COSIM) 
/*****************************************************************/
/* Multi-Core Related Define
/* DEF_SUB1/2_BITMAP are used in cosim env only, you should modify the
/* them by your target CEs.
/*****************************************************************/
#define DEF_SUB1_BITMAP     0x33333333
#define DEF_SUB2_BITMAP     0xCCCCCCCC
#endif

/* function interface */
void HAL_NfcDefaultPuMapInit(void);
void HAL_NfcSetLogicReg(U32 ulSubsys0BitMap, U32 ulSubsys1BitMap);
void HAL_NfcSetRedDataBase(void);
void HAL_NfcInterfaceInitBasic(void);
U8 HAL_NfcGetCE(U8 ucPU);
U8 HAL_NfcGetLogicPU(U8 ucPU);
U8 HAL_NfcGetWP(U8 ucPU);
U8 HAL_NfcGetRP(U8 ucPU);
BOOL HAL_NfcGetIdle(U8 ucPU);
BOOL HAL_NfcGetFull(U8 ucPU);
BOOL HAL_NfcGetEmpty(U8 ucPU);
BOOL HAL_NfcGetErrHold(U8 ucPU);
U8 HAL_NfcGetErrCode(U8 ucPU);
BOOL HAL_NfcGetFinish(U8 ucPU, U8 ucLevel);
U32 HAL_NfcGetDmaAddr(U32 ulBuffID, U32 ulStartSecInBuff, U32 ulBufSizeBits);
NFCQ_ENTRY * HAL_NfcGetNfcqEntryAddr(U8 ucPU, BOOL bRp);
U32 HAL_NfcGetPRCQEntryAddr(U8 ucPU);
U32 HAL_NfcGetFlashRowAddr(FLASH_ADDR * pFlashAddr);
void HAL_NfcSetPRCQEntry(U8 ucPU, U8 ucReqType);
void HAL_NfcResetCmdQue(U8 ucPU);
void HAL_NfcClearINTSts(U8 ucPU);
void HAL_NfcSetTriggerReg(U8 ucCE, U8 ucLun, U8 ucWp, U8 ucReqType);
void HAL_NfcCmdTrigger(FLASH_ADDR *pFlashAddr, U8 ucReqType);

#define HAL_GET_SW_RED_ID(Pu,Level) (((Pu) << NFCQ_DEPTH_BIT) | (Level))
U32 HAL_NfcGetRedAbsoulteAddr(U8 ucPU, U8 ucLevel, U8 ucPln);
U16 HAL_NfcGetRedRelativeAddr(U8 ucPU, U8 ucLevel, U8 ucPln);

void HAL_NfcInitPU2LUN(void);
U8 HAL_NfcGetLUN(U8 ucPU);
BOOL HAL_NfcWaitStatus(U8 ucPU);
BOOL HAL_NfcIsPUAccessable(U8 ucPU, BOOL bCELevelType);
BOOL HAL_NfcResetFlash(U8 ucPU);
BOOL HAL_NfcReadId(U8 ucPU, U32 *pBuf);
BOOL HAL_NfcPioSetFeature(U8 ucPU, U32 ulData, U8 ucAddr);
BOOL HAL_NfcSetFeature(U8 ucPU, U32 ulData, U8 ucAddr);
BOOL HAL_NfcSingleBlockErase(FLASH_ADDR *pFlashAddr);
BOOL HAL_NfcSinglePlnRead(FLASH_ADDR *pFlashAddr,  U8 ucSecStart, U8 ucSecLength,
                                         BOOL bOTFBBuff, U16 usBuffID, NFC_RED **ppRedAddrOrg);
BOOL HAL_NfcSinglePlaneWrite(FLASH_ADDR *pFlashAddr, U16 usBuffID, NFC_RED *pRed);
U32 HAL_NfcByteModeRead(FLASH_ADDR *pFlashAddr, U32 ulColumAddr, U8 ulLength, NFC_RED **pDataAddr);

U32 HAL_NfcGetCEBitMap(U32 ulMcuID);
void HAL_NfcPuMapping(U32 ulBitMap);
U8 HAL_NfcGetBrPu(U8 ucPu, U8 ucIndex);

void HAL_NfcSetLogicPUReg(U32 ulSubsys0BitMap, U32 ulSubsys1BitMap);


/*Interface in HAL_FlashChipFeature.c*/
void HAL_FlashNfcFeatureInit(void);
void HAL_FlashInitSLCMapping(void);
U16 HAL_FlashGetSLCPage(U16 usLogicPage);
U8   HAL_FlashGetReadRetryVal(U8 ucTime);
U32  HAL_FlashReadRetryCheck(U8 ucTime);
BOOL HAL_FlashSendRetryPreConditon(U8 ucPU);
BOOL HAL_FlashSendReadRetryCmd(U8 ucPU, U8 ucTime);
BOOL HAL_FlashReadRetryTerminate(U8 ucPU);
void HAL_FlashRetryValueInit(void);
U8 HAL_FlashReadRetry(U8 Pu, U8 ReqType, U8 SecStart, U8 SecLen, U16 BuffID, NFC_RED **ppOrgRedEntry);
U8 HAL_NfcWaitStatusWithRetry(U8 Pu, U8 ReqType, U8 SecStart, U8 SecLen, U16 BuffID, NFC_RED **ppOrgRedEntry);
BOOL HAL_FlashIsBlockBad(U8 ucPu,U8 ucPln, U16 usBlk);
U32 HAL_IsFlashPageSafed( U16 usWritePPO, U16 usReadPPO);
void HAL_ConfigScramble(NFCQ_ENTRY *pNFCQEntry, U32 ulPage);
#endif /* __HAL_FLASH_DRIVER_BASIC_H__ */
