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
20151105    abby      uniform code style and modify to meet VT3533
*******************************************************************************/

#ifndef __HAL_FLASH_DRIVER_BASIC_H__
#define __HAL_FLASH_DRIVER_BASIC_H__

/*------------------------------------------------------------------------------
    INCLUDING FILES
------------------------------------------------------------------------------*/
#include "HAL_GLBReg.h"
#include "COM_Memory.h"
#include "HAL_FlashCmd.h"
#include "HAL_NormalDSG.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
/*  CE related  */
#ifdef CE_DECODE
#define NFC_CE_PER_PU_BITS          1
#else
#define NFC_CE_PER_PU_BITS          0
#endif

#define NFC_CE_PER_PU               (1 << NFC_CE_PER_PU_BITS) // 1 - normal mode; 2 - CE decode

/*  PU and CH related  */
#define NFC_CH_TOTAL_BITS           2
#define NFC_CH_TOTAL                4
#define NFC_CH_MSK                  (NFC_CH_TOTAL-1)
#define NFC_PU_PER_CH               8
#define NFC_PU_MAX                  (NFC_CH_TOTAL * NFC_PU_PER_CH)

#define NFC_CH_NUM(PhyPU)           ((PhyPU) % NFC_CH_TOTAL)
#define NFC_PU_IN_CH(PhyPU)         ((PhyPU) / NFC_CH_TOTAL)

/*  LUN related  */
#define NFC_LUN_MAX_PER_PU          4
#define NFC_LUN_PER_PU_BITS         (LUN_PER_CE_BITS + NFC_CE_PER_PU_BITS)//valid LUN in each PU
#define NFC_LUN_PER_PU              (1 << NFC_LUN_PER_PU_BITS)

#define NFC_PLN_MAX_PER_LUN         4

/*  NFCQ related  */
/*  1LUN/CE or 2LUN/CE: 4 NFCQ_DEPTH; 4LUN/CE: 2 NFCQ_DEPTH  */
#define NFCQ_DEPTH_BIT              ((NFC_LUN_PER_PU == 4)? 1:2)
#define NFCQ_DEPTH                  (1 << NFCQ_DEPTH_BIT)
#define NFCQ_DEPTH_TOTAL            8

#define NFCQ_SEC_ADDR_COUNT         4

/*  Data Syntax related  */
#define NF_DS_ENTRY_MAX             8
#define CW_NUM_MAX_PER_DS_ENTRY     32
#define DS_ENTRY_SEL                0   //DS_ENTRY 0-7
#define DS_15K_CRC                  5   //DS_ENTRY 0-7

/* For sramble seed sel as plane number */
#define SCR_SINGLE_PLN              0
#define SCR_FULL_PLN                PLN_PER_LUN_BITS

/* OTFB Base addr sel */
#define OTFB_BASE_SEL               0   //0-0xfff; 1-0x1ff; 2-0x200

/* error bit injection */
#define NFC_ERR_INJ_BIT_MAX_PER_CW  (256)



/*  calcu red addr offset   */
#define HAL_GET_SW_RED_ID(Pu,Lun,Pln,Level)     (((((((Pu) * NFC_LUN_PER_PU) | (Lun)) << NFCQ_DEPTH_BIT) \
                                                | (Level)) * (PLN_PER_LUN * PGADDR_PER_PRG * INTRPG_PER_PGADDR)) | (Pln))

/*------------------------------------------------------------------------------
   REGISTER LOCATION
------------------------------------------------------------------------------*/
/* NFC REG Base Address*/
#define NF_PUBLIC_REG_BASE      (REG_BASE_NDC + 0x000)    //0x1ff81000
#define NF_CMD_STS_REG_BASE     (REG_BASE_NDC + 0x100)    //0x1ff81100
#define NF_CTL_REG_BASE         (REG_BASE_NDC + 0x300)    //0x1ff81300
#define NF_STATUS_REG_BASE      (REG_BASE_NDC + 0x400)

/* Public Reg related  */
#define NF_LOGIC_PU_REG_BASE    (NF_PUBLIC_REG_BASE + 0x050)    //0x1ff81050
#define rNfcPgCfg               (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x00)))
#define rNfcTCtrl0              (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x04)))
#define rNfcTCtrl1              (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x08)))
#define rNfcTCtrl2              (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x0c)))
#define rNfcRedDramBase         (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x10)))
#define rNfcMisc                (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x14)))
#define rNfcOtfbAdsBase         (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x18)))
#define rNfcSsuBaseAddr         (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x1c)))
#define rNfcDbgSigGrpChg0       (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x20)))
#define rNfcDbgSigGrpChg1       (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x24)))
#define rNfcDbgSigGrpChg2       (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x28)))
#define rNfcModeConfig          (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x2c)))
#define rNfcEdoTCtrlECO         (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x30)))
#define rNfcPRCQDelayTime       (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x34)))
#define rNfcIODelayCtrl         (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x38)))
#define rNfcIntAcc              (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x3c)))
#define rNfcReadID0             (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x40)))
#define rNfcReadID1             (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x44)))
#define rNfcHIDTag1             (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x48)))
#define rNfcHIDTag2             (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x4c)))
#define rNfcLogicPuNumCh0Pu0_3  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x50)))
#define rNfcLogicPuNumCh0Pu4_7  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x54)))
#define rNfcLogicPuNumCh1Pu0_3  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x58)))
#define rNfcLogicPuNumCh1Pu4_7  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x5c)))
#define rNfcLogicPuNumCh2Pu0_3  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x60)))
#define rNfcLogicPuNumCh2Pu4_7  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x64)))
#define rNfcLogicPuNumCh3Pu0_3  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x68)))
#define rNfcLogicPuNumCh3Pu4_7  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x6c)))
#define rNfcClkGateCtrl0        (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x70)))
#define rNfcClkGateCtrl1        (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x74)))
#define rNfcDDrDlyComp          (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x78)))
#define rNfcProgIOTimingCtrl1   (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x7c)))
#define rNfcProgIOTimingCtrl2   (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x80)))
#define rNfcProgIOTimingCtrl3   (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x84)))
#define rNfcReadDataOutputPIO1  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x88)))
#define rNfcReadDataOutputPIO2  (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x8C)))
#define rNfcFlashBsyTimeCH01    (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x90)))
#define rNfcFlashBsyTimeCH23    (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x94)))
#define rNfcCmdCfg              (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x98)))
#define rNfcXorDecFifoCfg       (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0x9C)))
#define rNfcNfdmaCfg            (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0xA0)))
#define rNfcNfdmaCfg2           (*((volatile U32*)(NF_PUBLIC_REG_BASE + 0xA4)))

/* Control Reg related  */
#define NF_QE_REG_BASE            (NF_CTL_REG_BASE + 0x00) //0x1ff81300
#define NF_QE_EXT_REG_BASE        (NF_CTL_REG_BASE + 0xB0) //0x1ff813B0, extend in 533
#define NF_DS_REG_BASE            (NF_CTL_REG_BASE + 0x70) //0x1ff81370

/* Status Reg related  */
#define NF_LLUNST_SW_REG_BASE     (NF_STATUS_REG_BASE + 0x00) //0x1ff81400, for FW,Logic LUN status
#define NF_LLUNST_REG_BASE        (NF_STATUS_REG_BASE + 0x40) //0x1ff81440, Logic LUN status
#define NF_LPUST_REG_BASE         (NF_STATUS_REG_BASE + 0x80) //0x1ff81480, logic PU status
#define NF_HPUST_REG_BASE         (NF_STATUS_REG_BASE + 0x90) //0x1ff81490, all channal status reg
#define NF_PPUST_REG_BASE         (NF_STATUS_REG_BASE + 0x94) //0x1ff81494, physical PU
#define NF_DEBUGST_REG_BASE       (NF_STATUS_REG_BASE + 0xA0) //0x1ff814A0, debug signal status reg

/* ICB TRIG NFC REG Base Address*/
#define NF_TRIGGER_REG_BASE         (REG_BASE_TRIG_NDC)  //0x1ff83f00

/* NF command status define, sync with HW spec of VT3533  */
#define NF_SUCCESS                      0  // No error
#define NF_ERR_TYPE_NODEV               1  // No device detected
#define NF_ERR_TYPE_BOTHPRG             2  // Previous page and current page both program fail.
#define NF_ERR_TYPE_PRG                 3  // Program error
#define NF_ERR_TYPE_PREPRG              4  // Previous page program fail
#define NF_ERR_TYPE_ERS                 5  // Erase error
#define NF_ERR_TYPE_UECC                6  // ECC unrecoverable error
#define NF_ERR_TYPE_LBA                 7  // LBA check error
#define NF_ERR_TYPE_DCRC                8  // CRC check error
#define NF_ERR_TYPE_RECC                9  // ECC recoverable error with error counter>threshold
#define NF_ERR_TYPE_RDST_INT            10 // read status "read and check" mode INT

#define NF_ERR_INJ_TYPE_ERS             0xC1
#define NF_ERR_INJ_TYPE_PRG             0xC1
#define NF_ERR_INJ_TYPE_PRE_PG_PRG      0xE2
#define NF_ERR_INJ_TYPE_PRE_CUR_PRG     0xE3

#define LPN_SIZE_BITS                   12
#define LPN_SIZE                        (1 << LPN_SIZE_BITS)

/*  codeword related  */
#define CW_INFO_SZ_BITS                 10
#define CW_INFO_SZ                      (1 << CW_INFO_SZ_BITS)
#define CW_INFO_SZ_DW                   (CW_INFO_SZ >> 2)
#define CW_PER_PLN_BITS                 (PHYPG_SZ_BITS - CW_INFO_SZ_BITS)
#define CW_PER_PLN                      (1 << CW_PER_PLN_BITS)        //16
#define CW_PER_LBA_BITS                 (LPN_SIZE_BITS - CW_INFO_SZ_BITS)
#define CW_PER_LBA                      (1 << CW_PER_LBA_BITS)        //4

/* NFC Data Syntax related  */
#define DS_CRC_LENTH                    2
#define DS_LBA_LENTH                    4

#define NFC_DS_CW_INFO_LENTH            (CW_INFO_SZ)
#define NFC_DS_CW_CRC_LENTH             (CW_INFO_SZ + DS_CRC_LENTH)
#define NFC_DS_CW_LBA_LENTH             (CW_INFO_SZ + DS_LBA_LENTH)
#define NFC_DS_CW_CRC_LBA_LENTH         (CW_INFO_SZ + DS_CRC_LENTH + DS_LBA_LENTH)

// Read and write the specified location value of the register(DW)
#define REG_SET_VALUE(reg,bits,value,offset) ((reg) = ((((reg)&=~(MULTI_VALUE_1(bits)<<(offset)))),((reg)|=((value)<<(offset)))))
#define REG_GET_VALUE(reg,bits,offset)       (((reg)>>(offset)) & MULTI_VALUE_1(bits))

/* Patrol read RECC Value threshold per Cw */
#define PATROL_READ_FBC_TH_CW  50

/* NFC relative function's return status*/
typedef enum _NFC_OPERATION_STATUS
{
    NFC_STATUS_FAIL = 0,
    NFC_STATUS_SUCCESS
}NFC_OPERATION_STATUS;

/* NFCQ control register */ //pending
typedef struct _PG_CONF_REG
{
    U32 bsOtfbmEn           : 1;    // 1: enable OTFBM, need set to 1 in PCIE mode; 0: disable
    U32 bsRowAddrByte       : 1;    // 1: 5 bytes row addr; 0: 4 bytes row addr(for 3D-nand)
    U32 bsOntfMode          : 1;    // 1: ONTF bit valid in DSG; 0: ONTF bit valid in NFCQ
    U32 bsScrBps            : 1;    // Data scramble bypass or not. 1-bypass, 0-not bypass.
    U32 bsColAddrByte       : 1;    // 1: 5byte; 0: 2byte
    U32 bsWbPcieMode        : 1;    // 1: PCIE mode write back; 0: SATA mode write back
    U32 bsIntMskEn          : 1;    // 0: not mask interrupt; 1: mask
    U32 bsDDRHfCfg          : 1;    // 0: for Asyn and NVDDR1; 1: for TOGGLE1, TOGGLE2 and NVDDR2
    U32 bsDDRCfg            : 1;    // 1: DDR mode; 0: not DDR mode
    U32 bsIcbRdy            : 1;    /*
                                    * 0: ICB_RDY de-assert, if clear, del trig CDC time delay;
                                    * 1: ICB_RDY tie to 1
                                    */
    U32 bsODTEn             : 1;    // 1: enable ODT; 0: disable
    U32 bsRTSB3dTlc         : 1;    // 1: TSB TLC mode, for low/middle/high page select
    U32 bsAccProgMode       : 1;    // Accelerate programming mode:
                                    // 1: All CE will pull down for any PU's program command for accelerate programming.
                                    // 0: Normal mode.
    U32 bsLunBdEn           : 1;    // 1: LUN in one CE have interleave restriction; 0: no restriction
    U32 bsFstDatRdyMod      : 1;    // 0: Not set first data ready set when first 1K/4K data decode fail
                                    // 1: Set first data ready no matter decode success or not.
    U32 bsNcrcChkEn         : 1;    // 1: flash CRC check enable
    U32 bsReccEn            : 1;    // 1: Recoverable RECC error report enable
    U32 bsLdpcNvddrMode     : 1;    // The cycle gaps between CWs is guaranteed to be >5, used for NVDDR 100 (10ns per cycle) to eliminate timing violation, gaps between CWs > 50ns
    U32 bsMulLunOnfi        : 1;    // 1: new ONFI multi-LUN command format:78, raw address, 00, 5 cycle address, 05, column address, E0.
    U32 bsTlcParityPogMode  : 1;     // For program XORE parity page:
                                    // 1: NFC send PARITY_PAGE_POS in NFCQ to XORE as page number.
                                    // 0: NFC count page number with TLC_PLN in NFCQ.
    U32 bsSFLoc             : 3;    // Success/Fail bit location in a byte.
    U32 bsMulLunToggle      : 1;    // 0:ONFI MUL_LUN mode; 1:TOGGLE MUL_LUN mode
    U32 bsRdStsMode         : 1;    /*
                                    *0: issue read status after all PU's data transfer done
                                    *1: issue read status after current PU's data transfer done
                                    */
    U32 bsDCrcChkEn         : 1;    // DRAM CRC check enable
    U32 bsMulLun            : 2;    // Multi-LUN number:
                                    // 00: not multi-LUN
                                    // 01: 2 LUN in one target
                                    // 10: 4 LUN in one target
                                    // 11: 8 LUN in one target
    U32 bsIsMicron          : 1;    // 1-MICRON device, 0-Not MICRON device (if DDR_MODE=1, indicate it is toggle NAND).
    U32 bsCeHoldEn          : 1;    // 1: CE not toggle in continuous data transfer for improves performance. Only valid in sync mode.
    U32 bsChArbEn           : 1;    // 1: Arbitrate in 1K data. 0: Arbitrate in 256 byte data.
    U32 bsDelayAccEn        : 1;    // 1: enable hide idle delay time in other PU's active time.
                                    // 0: disable, need clear when multi-lun
}PG_CONF_REG;

// SSU base addr in OTFB and DRAM
typedef struct _NF_SSU_BASEADDR_REG_
{
    U32 bsSsuOtfbBase   : 10;   //SSU base address[19:10] for OTFB.
    U32 bsSsuDramBase   : 22;   //SSU base address[31:10] for DRAM.
}NF_SSU_BASEADDR_REG;

// RED base addr in DRAM
typedef struct _NF_RED_DRAM_BASE_REG_
{
    U32 bsRsv               : 3;
    U32 bsRedDramBaseAddr   : 29;   //Red base address[19:10] for DRAM.
}NF_RED_DRAM_BASE_REG;

/* NFC feature option    */
typedef enum _NFC_INIT_PARAM
{
    /* red number support 8 kinds    */
    REDNUM_8BYTE = 0,
    REDNUM_16BYTE,
    REDNUM_24BYTE,
    REDNUM_32BYTE,
    REDNUM_40BYTE,
    REDNUM_48BYTE,
    REDNUM_56BYTE,
    REDNUM_64BYTE,

    /*   DS ENTRY REG select   */
    DS_ENTRY0 = 0,
    DS_ENTRY1,
    DS_ENTRY2,
    DS_ENTRY3,
    DS_ENTRY4,
    DS_ENTRY5,
    DS_ENTRY6,
    DS_ENTRY7,

    /* Timing mode support 4 kinds, but often use 1    */
    NFTM_MODE1 = 0,
    NFTM_MODE2,
    NFTM_MODE3,
    NFTM_MODE4,

    /* Interface type */
    TOGGLE2 = 0,
    ONFI2
}NFC_INIT_PARAM;

/* Data syntax Entries, fill into IRS */
typedef struct _NFC_DS_ENTRY_REG_
{
    /*    DW 0 */
    U32 bsRsv1      : 1;    // 0-EC for every 1K data; 1-ECC for every 2K data
    U32 bsRedNum    : 3;    // Redundant number: 000-8bytes, 001-16bytes, ... , 110-56bytes, 111-64bytes.
    U32 bsLbaEn     : 1;    // 1-every 4 codeword has a 4 byte LBA followed user data.
    U32 bsNCRCEn    : 1;    // 1-every codeword has a 2byte CRC parity.
    U32 bsRsv2      : 3;
    U32 bsCwNum     : 6;    // Codeword number in a plane
    U32 bsRsv3      : 17;

    /*    DW 1 */
    U32 ulRsv4;
}NFC_DS_ENTRY_REG;

typedef struct _NFC_DATA_SYNTAX_
{
    NFC_DS_ENTRY_REG atDSEntry[NF_DS_ENTRY_MAX];
}NFC_DATA_SYNTAX;

/*  DS_SRAM information */
typedef struct _NFC_DS_COLADDR_ENTRY_
{
    U32 bsCwClAddr  : 15;   // CW_Len + LDPC parity, 2 bytes align
    U32 bsCwLenth   : 11;   // Codeword data length = data + CRC + LBA, 2 bytes align
    U32 bsCodeSel   : 2;
    U32 bsRev       : 4;
}NFC_DS_COLADDR_ENTRY;

typedef struct _NFC_DS_COLADDR_
{
    NFC_DS_COLADDR_ENTRY aDSColAddr[NF_DS_ENTRY_MAX][CW_NUM_MAX_PER_DS_ENTRY];
}NFC_DS_COLADDR;

typedef struct _NFC_SEC_ADDR_
{
    U16 bsSecLength : 8;
    U16 bsSecStart  : 8;
}NFC_SEC_ADDR;

typedef struct _NFC_BYTE_ADDR_
{
    U32 usByteAddr      : 16;   // for byte mode ,DW1 means byte addr and length
    U32 usByteLength    : 16;
}NFC_BYTE_ADDR;

typedef struct _NFC_ROW_ADDR_
{
    U32 bsRowAddr   : 30;
    U32 bsFCmdPtr   : 2;
}NFC_ROW_ADDR;

/* NFCQ register 16DW */
typedef struct _NFCQ_ENTRY
{
    union
    {
        struct//for real NFC design
        {
            /* DW0 byte 0 */
            U32 bsOntfEn    : 1;    // set - otfb. clear -  dram
            U32 bsDmaByteEn : 1;    // set - byte. clear - sector
                                    // For copy back without data transfer, need set to 1
            U32 bsRomRd     : 1;    // set - Rom code read mode. Use Rom code read data syntax in IRS
            U32 bsPuEnpMsk  : 1;    // set - mask = disable scramble; clear - enable scramble
            U32 bsDCrcEn    : 1;    // set - Enable DRAM CRC check for every codeword; clear - disable
            U32 bsLbaChk    : 1;    // set - Enable LBA add and check in write cmd
            U32 bsEMEn      : 1;    // get AES seed for every sector, from redundant data in wrie command; form AES_SED SRAM in read command
            U32 bsDsgEn     : 1;    // set - fetch dsg ,clear - not fetch dsg

            /* DW0 byte 1 */
            U32 bsSsu0En    : 1;    // set - enable ssu0 , clear - disable ssu0.
            U32 bsSsu1En    : 1;    // set - enable ssu1 , clear - disable ssu1. used for cache status
            U32 bsTrigOmEn  : 1;    // set - Need to trig OTFBM before data xfer.  clear -  Not need
            U32 bsDsIndex   : 3;    // select flash page layout from 4 Data syntax,only 2bits valid
            U32 bsTlcPlnNum : 2;    // for TLC program or 3D MLC command or XOR write command
                                    // 00: every plane has dedicate scramble seed index.
                                    // 01: plane 0,1 have same scramble seed index; plane 2,3 have another scramble seed index?-
                                    // 10: every 4 plane have same scramble seed index.
                                    // 11: every 8 plane have same scramble seed index.

            /* DW0 byte 2 ~ 3 */
            U32 bsNcqCntEn  : 1;    /*
                                      PCIE:enable count program success command number for OTFB write error handling
                                      SATA:enable output program done to SATA
                                    */
            U32 bsNcqNum    : 6;    // 32 tag, if we have more than 32 tags, fill bit[5] to TagExt in DW3
            U32 bsFstDsgPtr : 9;    // Record First DSG ID for incidental case if need release the DSG
        };

        struct//for Fake NFC design
        {
            U32 bsFakeNfcOntfEn     : 1;    // 1 = Message data is stored in on-the-fly SRAM.
            U32 bsFakeNfcTrigOmEn   : 1;    // need trig OTFBM before data transfer.
            U32 bsFakeNfcDW0Rsvd    : 6;
            U32 bsFakeNfcTotalSec   : 8;    // As Total data length in this command. The unit is sector.
            U32 bsFakeNfcDw0Rsvd    : 16;
        };
    };

    /* DW1~2 */
    union
    {
        /* For DMA read/write: sector enable  */
        NFC_SEC_ADDR aSecAddr[NFCQ_SEC_ADDR_COUNT];     // for DMA mode ,DW 1 means sec start and length

        /* DW1 for dma byte mode, DW2 for single read/write: set feature value */
        struct
        {
            NFC_BYTE_ADDR aByteAddr; // BYTE Mode
            U32 ulSetFeatVal;        // Set Feature values
        };

        /* For INJ_EN = 1: error injection  */
        struct
        {
            U32 ulErrInjRsv;
            U32 bsInjCwCnt      : 8;    // For read command with error bit injection, only SEC_START0/LENGTH0 is valid
                                        // and covered all sectors of INJ_CW_START/CNT. SEC_LENGTH1 should set to 0.
                                        // INJ_CW_LENGTH is the count of CW with error bit injection, 0 stand for 1CW.
            U32 bsInjCwStart    : 8;    // The start CW with error injection
            U32 bsInjErrCnt     : 8;    // Injection error bits count in one CW
            U32 bsInjStsVal     : 8;    // For write/erase command, INJ_STATUS_VALUE will be filled in DEC_SRAM replace
                                        // of last read status value.
        };

        struct//for Fake NFC design
        {
            U32 ulFakeNfcDRAMAddr;  // NFCQ DW1, used for Fake NFC as DRAM address for DMA.
            U32 ulFakeNFcRsvd;      // NFCQ DW2~15 is not used by Fake NFC
        };
    };

    /* DW3 */
    union
    {
        /* For DRAM path: DMA overall message */
        struct
        {
            /* DW3 byte 0 ~ 2 */
            U32 bsDmaTotalLength    : 8;    // align in sector,8'h0 stand for 256 sectors;
                                            // For write command, NFC use TOTAL_LENGTH and don't care SEC_START/LENGTH.
                                            // For read redundant only command, TOTAL_LENGTH [2:0] stand for pln num
            U32 bsPrcqStartDw       : 9;    // PRCQ start DW
            U32 bsInjEn             : 1;    // set - enable error injection  , clear -disable error injection
            U32 bsErrTypeS          : 4;    // 13 kinds of error types, valid when bsInjEn=1
            U32 bsBsyCntEn          : 1;    // set - enable measure flash prog/read/erase busy time
            U32 bsIntEn             : 1;    // set - enable NFC generate interrrupt to mcu after cmd process done. clear - disable it

            /* DW3 byte3 */
            U32 bsXorId             : 3;    // XOR engine ID
            U32 bsXorBufId          : 4;    // XORE SRAM BUFID of current data, need to align with flash page size
            U32 bsXorEn             : 1;    // set - enable XOR engine; clear - disable
        };

        /* For OTFB path: DMA overall message */
        struct
        {
            U32 bsOtfbPathRsv0  : 27;
            U32 bsOtfbBsSel     : 2;    // For OTFB path only, select NDOA base address:
                                        // 00: 12'hfff; 01: 12'h1ff; 10: 12'h200
            U32 bsOtfbPathRsv1  : 3;
        };
    };

    /* DW4 */
    U32 bsRedOntf   : 1;    // set - redundant data is stored in OTFB
    U32 bsRedEn     : 1;    // set - enable redundant data
    U32 bsRedOnly   : 1;    // set - NFC do NOT read sector data even though IO_DATA set in raw cmd phase
    U32 bsRedAddr   : 17;   // 16 byte align
    U32 bsRawReadEn : 1;    // set - enable read RAW data and flush to DRAM/OTFB
    U32 bsNcqMd     : 1;    // 0-First data ready set after the first 4K data transfer done regardless of LDPC decode result
                            // 1-First data ready set after the first 1K data transfer done regardless of LDPC decode result
    U32 bsBmEn      : 1;    // buffer map enable
    U32 bsFstDataRdy: 1;    // first data ready enable
    U32 bsScrSeed0  : 8;    // First scramble seed from FW.

    /* DW5 */
    union
    {
        /* For Read command */
        struct
        {
            U32 bsDecFifoIdxLsb : 1;    // DEC_FIFO index bit 0 for FW tracing
            U32 bsBypassRdErr   : 1;    // 1-Bypass decode fail/CRC check fail/LBA check fail report for read command
            U32 bsDecFifoEn     : 1;    // Report detail decode status to FIFO
            U32 bsRdLba         : 29;   // LBA for read command
        };
        /* For write command */
        struct
        {
            U32 bsScrSeed1Lsb   : 1;    // Second scramble seed bit 0
            U32 bsScrSeed2Lsb   : 1;    // Third scramble seed bit 0
            U32 bsParPgPos      : 2;    // For TLC multi-plane command, every write command with XOR parity:
                                        // 00 : lower page;
                                        // 01 : middle page;
                                        // 10 : upper page.
                                        // Need set TLC_PARITY_POG_MODE to 1 in IRS
            U32 bsScrRsv        : 4;
            U32 bsRedAddXorPar0 : 8;    // The bit3-9 of XOR parity redundant offset relative to {XOR Redundant
            U32 bsRedAddXorPar1 : 8;    // base address in OTFB SRAM}->this should be configured in NFC register.
            U32 bsRedAddXorPar2 : 8;    // RED_ADD_XOR_PARITY0/1/2 for TLC low/middle/upper page; Other case
                                        // only RED_ADD_XOR_PARITY0 is valid.
        };
    };

    /* DW6 */
    union
    {
        struct
        {
            U32 bsSsu0Addr      : 16;   // Status update address 0, byte align
            U32 bsSsu0Data      : 8;    // Status update data 0, byte align
            U32 bsSsu0Ontf      : 1;    // SSU0 stored in: 1 SRAM, 0 DRAM
            U32 bsScrSeed1Msb   : 7;    // Second scramble seed bit 1-7
        };

        struct
        {
            U32 bsDecFifoRsv1       : 25;
            U32 bsDecFifoIdx7to1b   : 7;    // Second scramble seed bit 7-1
        };
    };

    /* DW7 */
    U32 bsSsu1Addr      : 16;   // Status update address 1, byte align
    U32 bsSsu1Data      : 8;    // Status update data 1, byte align
    U32 bsSsu1Ontf      : 1;    // SSU1 stored in: 1 SRAM, 0 DRAM
    U32 bsScrSeed2Msb   : 7;    // Third scramble seed bit 1-7

    /* DW8~15 */
    union
    {
        NFC_ROW_ADDR atRowAddr[8];
        U32 aRowAddr[8];            // QE Addr group   (AQEE)
    };

}NFCQ_ENTRY;

typedef struct _NFC_SCR_SEED_
{
    U8 bsScrSeedBit0    : 1;
    U8 bsScrSeedBit1to7 : 7;
}NFC_SCR_SEED;

/* NFC CMD TYPE */
typedef enum _NFC_CMD_TYPE_
{
    NFC_WRITE_CMD = 0,
    NFC_READ_CMD,
    NFC_ERASE_CMD,
    NFC_OTHER_CMD
}NFC_CMD_TYPE;

/* NFC trigger register */
typedef union _NFC_TRIGGER_REG_
{
    U8 ucTrigValue;
    struct
    {
        U8 bsCmdType    : 2;    // 00 - write, 01 - read, 10 - erase, 11 - other
        U8 bsCacheOp    : 1;    // 1 - Indicate command in this level is a cache read/write command. 0 - other command
        U8 bsCESel      : 1;    // CE select for 64 CE
        U8 bsExtCmd     : 1;    // 0: Normal command.
                                // 1: for other command, stand for PIO command;
                                //    for write command, stand for write XOR parity;
                                //    for cache read command, stand for single plane cache read command.
        U8 bsExtCmdSel  : 3;    // for write command and EXT_CMD = 1, stand for XOR_ID;
                                // for single plane cache read command and EXT_CMD = 1, stand for plane number
    };
}NFC_TRIGGER_REG;

/* NF CQ register definition */
// NFC Request Registers (Base Address = 0x1FF81100)
typedef struct _NFC_CMD_STS_REG
{
    U8 bsIdle   : 1;    // 1 - idle, 0 - not idle, idle indicate all HW levels cmd all done
    U8 bsErrh   : 1;    // 1 - error hold, 0 - no error
    U8 bsEmpty  : 1;    // 1 - empty, 0 - not empty
    U8 bsFull   : 1;    // 1 - full, 0 - not full
    U8 bsRdPtr  : 2;    // Indicate which HW level the next read cmd pointer directing
    U8 bsWrPtr  : 2;    // Indicate which HW level the next write cmd pointer directing

    U8 bsErrSts : 4;    // when Errh == 1, the indicate error type
    U8 bsIntSts : 1;    // interrupt status, MCU can write this bit or public interrupt bit to clear the status.
    U8 bsPrst   : 1;    // When read, it returns "ERRH" bit. When write, trun current PU's all level into available
                        // status.when ERRH is cleared. Only software could write this bit.
    U8 bsRes1   : 2;    // reserved

    U8 bsCmdType0   : 3;    // Level 0 Cmd type
    U8 bsFsLv0      : 1;    // Level 0 finishing status
    U8 bsCmdType1   : 3;    // Level 1 Cmd type
    U8 bsFsLv1      : 1;    // Level 1 finishing status
    U8 bsCmdType2   : 3;    // Level 2 Cmd type
    U8 bsFsLv2      : 1;    // Level 2 finishing status
    U8 bsCmdType3   : 3;    // Level 3 Cmd type
    U8 bsFsLv3      : 1;    // Level 3 finishing status

}NFC_CMD_STS_REG;

typedef struct _NFCQ
{
    NFCQ_ENTRY aNfcqEntry[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH_TOTAL/NFC_LUN_PER_PU];
}NFCQ;

/* command Trigger register define */
typedef struct _NFC_TRIGGER
{
    NFC_TRIGGER_REG aNfcTriggerReg[NFC_PU_MAX][NFC_LUN_PER_PU][NFCQ_DEPTH_TOTAL/NFC_LUN_PER_PU];
}NFC_TRIGGER;

typedef struct _NFC_CMD_STS
{
    NFC_CMD_STS_REG aNfcqCmdStsReg[NFC_PU_MAX][NFC_LUN_MAX_PER_PU];
}NFC_CMD_STS;

/* Redundant Area relative define */
typedef struct _NFC_RED
{
    U32 aContent[RED_SW_SZ_DW];
}NFC_RED;

/* CE to logic PU mapping register */
typedef struct _NFC_LOGIC_PU_REG
{
    U8 bsLogicPU    : 5;
    U8 bsPuEnable   : 1;
    U8 bsRsvd       : 2;
}NFC_LOGIC_PU_REG;

typedef struct _NFC_LOGIC_PU
{
    NFC_LOGIC_PU_REG aNfcLogicPUReg[NFC_CH_TOTAL][NFC_PU_PER_CH];
}NFC_LOGIC_PU;


/* flash addr descriptor */
typedef struct _FLASH_ADDR
{
    U32 ucPU        : 8;
    U32 ucLun       : 8;
    U32 bsPln       : 4;
    U32 bsSLCMode   : 1;    // 1: SLC Mode; 0: MLC/TLC Mode
    U32 bsSLCShiftAddrByFW : 1; // SLC mode shift page address by FW, enable when flash disable shift
    U32 bsRsv       : 10;
    U32 usBlock     : 16;
    U32 usPage      : 16;
}FLASH_ADDR;

/* cmd level descriptor */
typedef struct _NFC_CMD_LEVEL_
{
    U8  ucPhyPU;
    U8  ucLun;
    U8  ucWp;
    U8  ucRsv;
}NFC_CMD_LEVEL;

/* scamble seed config type */
typedef enum _NFC_SCR_CONF_TYPE_
{
    NORMAL_SCR = 0,         // for normal cmd, RW 1 page; or for 3D MLC write low pages only; or for 3D MLC read 1 page
    TLC_WT_THREE_PG,        // for TLC write 3 pages
    TLC_RD_ONE_PG,          // for TLC read 1 page
    MLC_RW_TWO_PG,          // for 3D MLC read/write 2 pages: low + high
    TLC_RW_TWO_PG,
//#ifdef XOR_ENABLE
    TLC_WT_XOR_6DSG,        // For the 6 DSG issue of TLC XOR program.
//#endif
}NFC_SCR_CONF;

/* error injection related */
typedef struct _NFC_ERR_INJ_
{
    U32 bsInjCwStart    : 8;    // The start CW with error injection
    U32 bsInjCwLen      : 8;    // The count of CW with error bit injection, 0 stand for 1CW
    U32 bsInjErrBitPerCw: 8;    // Injection error bits count in one CW
    U32 bsInjErrSts     : 8;    // For write/erase command, replace of last read status value

    U32 bsInjErrType    : 4;    // Error type
    U32 bsRsv           : 28;
}NFC_ERR_INJ;

/* NFC read request descriptor */
typedef struct _NFC_READ_REQ_DES_
{
    /*  DW0  */
    //DMA related
    U32 bsSecStart  : 8;
    U32 bsSecLen    : 8;
    U32 bsRdBuffId  : 16;

    /*  DW1  */
    //NFC feature enable or disable
    U32 bsRedOntf   : 1;    //1-update to OTFB; 0-update to DRAM
    U32 bsLbaChkEn  : 1;
    U32 bsLba       : 29;
    U32 bsCSEn      : 1;    //cache status enable

    /*  DW2  */
    U32 bsSsu0En    : 1;    //SSU0 enable
    U32 bsSsu0Ontf  : 1;    //SSU0, 1-update to OTFB; 0-update to DRAM
    U32 bsSsu1En    : 1;    //SSU1 enable
    U32 bsSsu1Ontf  : 1;    //SSU1, 1-update to OTFB; 0-update to DRAM
    //TLC related
    U32 bsTlcPgType : 2;    //read page type, LOW:0 MID:1 UPP:2
    U32 bsTlcMode   : 1;    //1-TLC/MLC cmd; 0-SLC cmd
    //DEC FIFO status
    U32 bsDecFifoEn     : 1;    //1-enable DEC FIFO status report
    U32 bsDecFifoIndex  : 8;    //1-DEC FIFO index, assign by FW
    //Raw data read
    U32 bsRawData       : 1;    //1-read without ECC/Scramble/EM
    U32 bsInjErrEn      : 1;
    U32 bsDsIndex       : 3;    //Data Syntax type
    U32 bsEmEn          : 1;    //Maple add to enable EM function
    U32 bsSnapReadEn    : 1;    //Snap read
    U32 bsRsv1          : 9;

    /*  DW3  */
    U32 bsSsu0Addr      : 16;   //SSU0 addr
    U32 bsSsu1Addr      : 16;   //SSU1 addr

    /*  DW4  */
    U32 bsCsAddrOff;            //Cache status addr offset

    /*  DW5  */
    //RED pointer
    NFC_RED **ppNfcRed;

    /*  DW6  */
    NFC_ERR_INJ *pErrInj;       //error injection
}NFC_READ_REQ_DES;

/* NFC write request descriptor */
typedef struct _NFC_PRG_REQ_DES_
{
    /*  DW0  */
    //DMA related
    U32 bsWrBuffId      : 16;
    //NFC feature enable or disable
    U32 bsRedOntf       : 1;    //1-update to OTFB; 0-update to DRAM
    U32 bsLbaChkEn      : 1;
    U32 bsCSEn          : 1;    //cache status enable
    U32 bsSsu0En        : 1;    //SSU0 enable
    U32 bsSsu0Ontf      : 1;    //SSU0, 1-update to OTFB; 0-update to DRAM
    U32 bsSsu1En        : 1;    //SSU1 enable
    U32 bsSsu1Ontf      : 1;    //SSU1, 1-update to OTFB; 0-update to DRAM
    //TLC related
    U32 bsTlcPgCycle    : 4;    //page program cycle: 0/1/2
    U32 bsTlcMode       : 1;    //1-TLC/MLC cmd; 0-SLC cmd
    U32 bsTlcPgType     : 2;    //LOW:0 MID:1 UPP:2
    U32 bsInjErrEn      : 1;
    U32 bsSinglePgProg  : 1;    //YMTC single page programming mode
    
    /*  DW1  */
    U32 bsSsu0Addr      : 16;   //SSU0 addr
    U32 bsSsu1Addr      : 16;   //SSU1 addr

    /*  DW2  */
    U32 bsCsAddrOff;            //Cache status addr offset

    /*  DW3  */
    //RED pointer
    NFC_RED *pNfcRed;

    U32 bsDsIndex   : 3;    //Data Syntax type
    U32 bsEmEn      : 1;    //Maple add to enable EM function
    U32 bsRsv2      : 28;

    /*  DW4  */
    NFC_ERR_INJ *pErrInj;  //error injection
}NFC_PRG_REQ_DES;

/*------------------------------------------------------------------------------
   EXTERN VARIABLES
------------------------------------------------------------------------------*/
extern GLOBAL MCU12_VAR_ATTR volatile NFC_DS_COLADDR *g_pNfcDsColAddr;
/* for run HAL_UnitTest on ASIC use by MPTool */
#ifdef ASIC
extern GLOBAL volatile U8 *g_pNfcSaveFwDone;
#endif

/*------------------------------------------------------------------------------
   FUNCTION DECLARATION
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcBuildPuMapping(U32 ulBitMap);
U32  MCU12_DRAM_TEXT HAL_NfcGetPhyPUBitMap(void);
void MCU12_DRAM_TEXT HAL_NfcSetLogicPUReg(U32 ulPhyPUBitMap);
void MCU12_DRAM_TEXT HAL_NfcBaseAddrInit(void);
void MCU12_DRAM_TEXT HAL_NfcInterfaceInitBasic(void);
NFC_DS_COLADDR_ENTRY* HAL_NfcGetDsSramEntryAddr(U8 ucDSIndex, U8 ucCwIndex);
void MCU12_DRAM_TEXT HAL_NfcDataSyntaxInit(void);
U8 HAL_NfcGetPhyPU(U8 ucPU);
U8 HAL_NfcGetWP(U8 ucPU, U8 ucLun);
U8 HAL_NfcGetRP(U8 ucPU, U8 ucLun);
BOOL HAL_NfcGetIdle(U8 ucPU, U8 ucLun);
BOOL HAL_NfcGetFull(U8 ucPU, U8 ucLun);
BOOL HAL_NfcGetEmpty(U8 ucPU, U8 ucLun);
BOOL HAL_NfcGetErrHold(U8 ucPU, U8 ucLun);
U8   HAL_NfcGetErrCode(U8 ucPU, U8 ucLun);
BOOL HAL_NfcGetFinish(U8 ucPU, U8 ucLun, U8 ucLevel);
U32  HAL_NfcGetDmaAddr(U32 ulBuffID, U32 ulSecInBuff, U32 ulBufSizeBits);
NFCQ_ENTRY * HAL_NfcGetNfcqEntryAddr(U8 ucPU, U8 ucLun);
NFCQ_ENTRY * HAL_NfcGetNfcqEntryAddr_RP(U8 ucPU, U8 ucLun);
U32  HAL_NfcGetPrcqEntryAddr(U8 ucReqType);
U32  HAL_NfcGetFlashRowAddr(FLASH_ADDR * pFlashAddr);
void MCU12_DRAM_TEXT HAL_NfcResetCmdQue(U8 ucPU, U8 ucLun);
void MCU12_DRAM_TEXT HAL_NfcClearINTSts(U8 ucPU, U8 ucLun);
void HAL_NfcCmdTrigger(FLASH_ADDR *pFlashAddr, U8 ucReqType, BOOL bIsXorParityWriteCmd, U8 ucXorId);
U32  HAL_NfcGetRedAbsoulteAddr(U8 ucPU, U8 ucLun, U8 ucWp, U8 ucPln, BOOL bIsOntf);
U32  HAL_NfcGetRedRelativeAddr(U8 ucPU, U8 ucLun, U8 ucWp, U8 ucPln, BOOL bIsOntf);
BOOL HAL_NfcWaitStatus(U8 ucPU, U8 ucLun);
BOOL HAL_NfcIsLunAccessable(U8 ucPU, U8 ucLun);
BOOL HAL_NfcIsPairPageAccessable(U16 usPage);
BOOL MCU12_DRAM_TEXT HAL_NfcResetFlashNoWaitStatus(FLASH_ADDR *pFlashAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcResetFlash(FLASH_ADDR *pFlashAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcResetLun(FLASH_ADDR *pFlashAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcSyncResetFlash(FLASH_ADDR *pFlashAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcHardResetFlash(FLASH_ADDR *pFlashAddr);

BOOL MCU12_DRAM_TEXT HAL_NfcReadFlashId(FLASH_ADDR * pFlashAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcPioSetFeature(U8 ucPU, U8 ucLun, U32 ulData, U8 ucAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcSetFeature(FLASH_ADDR *pFlashAddr, U32 ulData, U8 ucAddr);
BOOL MCU12_DRAM_TEXT HAL_NfcGetFeature(FLASH_ADDR *pFlashAddr, U8 ucAddr);

void HAL_ConfigScramble(NFCQ_ENTRY *pNFCQEntry, U16 usPage, U8 ucPlnNum, U8 ucPageType, NFC_SCR_CONF eScrType);
BOOL MCU12_DRAM_TEXT HAL_NfcSingleBlockErase(FLASH_ADDR *pFlashAddr, BOOL bTlcMode);
void HAL_NfcSetErrInj(NFCQ_ENTRY *pNFCQEntry, NFC_ERR_INJ *pErrInj);
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlnRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, BOOL bOTFBBuff);
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlaneWrite(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq);
BOOL MCU12_DRAM_TEXT HAL_NfcSinglePlnCCLRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq);
BOOL MCU2_DRAM_TEXT HAL_NfcSinglePlaneWriteBuf(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq);
BOOL MCU2_DRAM_TEXT HAL_NfcSinglePlnReadBuf(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, BOOL bOTFBBuff);

/*Interface in HAL_FlashChipFeature.c*/
U8 HAL_FlashNfcFeatureGetScanVt(U8 ucIdx);
void HAL_FlashNfcFeatureInit(void);
void HAL_FlashInitSLCMapping(void);
U16  HAL_FlashGetSLCPage(U16 usLogicPage);
PAIR_PAGE_TYPE HAL_GetFlashPairPageType(U16 usPage);
WL_TYPE HAL_GetFlashWlType(U16 usPage, BOOL bsTLC);
U8 HAL_GetVthAdjustTime(U8 ucPageType, BOOL bsTLC);
U32 MCU12_DRAM_TEXT HAL_FlashRetryCheck(U8 ucTime, BOOL bTlcMode);
U32 MCU12_DRAM_TEXT HAL_FlashSelRetryPara(BOOL bTlcMode);
RETRY_TABLE MCU12_DRAM_TEXT HAL_FlashGetRetryParaTab(U32 ulIndex);
U8 MCU12_DRAM_TEXT HAL_FlashGetMaxRetryTime(BOOL bTLCMode, U16 usPage);
#ifdef READ_RETRY_REFACTOR
RETRY_TABLE MCU12_DRAM_TEXT HAL_FlashGetNewRetryParaTab(U32 ulIndex, BOOL bTlcMode, U8 ucWlType, U8 ucPageType, U8 ucLevel);
BOOL MCU12_DRAM_TEXT HAL_FlashHomemadeVtTerminate(U8 ucPU, U8 ucLun, BOOL bTlcMode, U8 ucWlType, U8 ucPageType);
#endif

BOOL MCU12_DRAM_TEXT HAL_FlashRetryPreConditon(FLASH_ADDR *pFlashAddr);
BOOL MCU12_DRAM_TEXT HAL_FlashRetrySendParam(FLASH_ADDR *pFlashAddr, RETRY_TABLE *pRetryPara, BOOL bTlcMode, U8 ucParaNum);
BOOL MCU12_DRAM_TEXT HAL_FlashRetryEn(FLASH_ADDR *pFlashAddr, BOOL bRetry);
BOOL MCU12_DRAM_TEXT HAL_FlashRetryTerminate(U8 ucPU, U8 ucLun, BOOL bTlcMode);
BOOL MCU12_DRAM_TEXT HAL_FlashIs1PlnOp(U8 ucReqType);
U8   MCU12_DRAM_TEXT HAL_FlashGetPlnNumFromNfcq(NFCQ_ENTRY * pNfcqEntry);
BOOL HAL_FlashIsBlockBad(U8 ucPu, U8 ucLun, U8 ucPlane, U16 usBlk);
U32  HAL_IsFlashPageSafed(U8 ucPu, U16 usWritePPO, U16 usReadPPO);// the unit is ucSuperPu.
BOOL HAL_FlashIsTrigCacheOp(U8 ucCurrCmdType);

BOOL HAL_NfcBypassScrb(void);
BOOL HAL_NfcIsChkSeedSel(void);

/*TLC*/
//#ifdef FLASH_TLC
U8  HAL_FlashGetTlcPrgCycle(U16 usPrgIndex);
U16 HAL_FlashGetTlcPrgWL(U16 usPrgIndex);
U8 HAL_FlashGetScrSeed(U16 usPage, U8 ucScrMod, U8 ucPageType);

//#endif

#ifdef FLASH_INTEL_3DTLC
U16 HAL_GetHighPageIndexfromExtra(U16 usExtraPg);
#endif
/* add by jason guo */
BOOL MCU12_DRAM_TEXT HAL_NfcDetectEmptyPage(NFC_RED *p_Red, U8 ErrCode);
void MCU12_DRAM_TEXT HAL_NfcSetRedInDramBase(U32 ulRedBaseAddr);
void MCU12_DRAM_TEXT HAL_NfcSetOtfbAdsBase(U32 ulBaseAddr0, U32 ulBaseAddr1, U32 ulBaseAddr2);
void MCU12_DRAM_TEXT HAL_NfcSetSsuInOtfbBase(U32 ulSsuBaseAddr);
void MCU12_DRAM_TEXT HAL_NfcSetSsuInDramBase(U32 ulSsuBaseAddr);

// Full-Set for All kinds of NAND
// Comm
#ifndef NF_PRCQ_RESET
#define NF_PRCQ_RESET      (INVALID_2F-0)
#endif
#ifndef NF_PRCQ_SYNC_RESET
#define NF_PRCQ_SYNC_RESET (INVALID_2F-1)
#endif
#ifndef NF_PRCQ_HARD_RESET
#define NF_PRCQ_HARD_RESET (INVALID_2F-8)
#endif
#ifndef NF_PRCQ_RESET_LUN
#define NF_PRCQ_RESET_LUN  (INVALID_2F-2)
#endif
#ifndef NF_PRCQ_READID
#define NF_PRCQ_READID     (INVALID_2F-3)
#endif
#ifndef NF_PRCQ_SETFEATURE
#define NF_PRCQ_SETFEATURE (INVALID_2F-4)
#endif
#ifndef NF_PRCQ_GETFEATURE
#define NF_PRCQ_GETFEATURE (INVALID_2F-5)
#endif
#ifndef NF_PRCQ_PIO_SETFEATURE
#define NF_PRCQ_PIO_SETFEATURE (INVALID_2F-6)
#endif
#ifndef NF_PRCQ_PIO_SETFEATURE_EX
#define NF_PRCQ_PIO_SETFEATURE_EX (INVALID_2F-7)
#endif

//===== 2D-MLC or 2D-TLC(SLC Mode) or IM 3D-TLC(SLC mode) =====
//Single plane erase/write/read
#ifndef NF_PRCQ_ERS
#define NF_PRCQ_ERS (INVALID_2F-8)
#endif
#ifndef NF_PRCQ_PRG
#define NF_PRCQ_PRG (INVALID_2F-9)
#endif
#ifndef NF_PRCQ_READ
#define NF_PRCQ_READ (INVALID_2F-10)
#endif
#ifndef NF_PRCQ_CCLR
#define NF_PRCQ_CCLR (INVALID_2F-11)
#endif
//Multi plane erase/write/read
#ifndef NF_PRCQ_ERS_MULTIPLN
#define NF_PRCQ_ERS_MULTIPLN (INVALID_2F-12)
#endif
#ifndef NF_PRCQ_PRG_MULTIPLN
#define NF_PRCQ_PRG_MULTIPLN (INVALID_2F-13)
#endif
#ifndef NF_PRCQ_READ_MULTIPLN
#define NF_PRCQ_READ_MULTIPLN (INVALID_2F-14)
#endif
#ifndef NF_PRCQ_CCLR_MULTIPLN
#define NF_PRCQ_CCLR_MULTIPLN (INVALID_2F-15)
#endif

#ifndef NF_PRCQ_RETRY_PRE
#define NF_PRCQ_RETRY_PRE (INVALID_2F-16)
#endif
#ifndef NF_PRCQ_RETRY_ADJ0
#define NF_PRCQ_RETRY_ADJ0 (INVALID_2F-17)
#endif
#ifndef NF_PRCQ_RETRY_ADJ1
#define NF_PRCQ_RETRY_ADJ1 (INVALID_2F-18)
#endif
#ifndef NF_PRCQ_RETRY_ADJ2
#define NF_PRCQ_RETRY_ADJ2 (INVALID_2F-19)
#endif
#ifndef NF_PRCQ_RETRY_ADJ3
#define NF_PRCQ_RETRY_ADJ3 (INVALID_2F-20)
#endif
#ifndef NF_PRCQ_RETRY_EN
#define NF_PRCQ_RETRY_EN (INVALID_2F-21)
#endif

// ===== YMTC 2D-MLC(MLC Mode) =====
#ifndef NF_PRCQ_MLC_ERS
#define NF_PRCQ_MLC_ERS (INVALID_2F-8)
#endif
#ifndef NF_PRCQ_MLC_PRG
#define NF_PRCQ_MLC_PRG (INVALID_2F-9)
#endif
#ifndef NF_PRCQ_MLC_READ
#define NF_PRCQ_MLC_READ (INVALID_2F-10)
#endif
#ifndef NF_PRCQ_CCLR
#define NF_PRCQ_CCLR (INVALID_2F-11)
#endif

// ===== TSB 2D-TLC(TLC Mode) or IM 3D-TLC(TLC Mode) =====
//Single plane erase
#ifndef NF_PRCQ_TLC_ERS
#define NF_PRCQ_TLC_ERS (INVALID_2F-22)
#endif
//Multi plane erase
#ifndef NF_PRCQ_TLC_ERS_MULTIPLN
#define NF_PRCQ_TLC_ERS_MULTIPLN (INVALID_2F-23)
#endif

// ===== TSB 2D-TLC(TLC Mode) =====
//Single plane write/read
#ifndef NF_PRCQ_TLC_PRG_1ST
#define NF_PRCQ_TLC_PRG_1ST (INVALID_2F-24)
#endif
#ifndef NF_PRCQ_TLC_PRG_2ND
#define NF_PRCQ_TLC_PRG_2ND (INVALID_2F-25)
#endif
#ifndef NF_PRCQ_TLC_PRG_3RD
#define NF_PRCQ_TLC_PRG_3RD (INVALID_2F-26)
#endif
#ifndef NF_PRCQ_TLC_READ_LP
#define NF_PRCQ_TLC_READ_LP (INVALID_2F-27)
#endif
#ifndef NF_PRCQ_TLC_READ_MP
#define NF_PRCQ_TLC_READ_MP (INVALID_2F-28)
#endif
#ifndef NF_PRCQ_TLC_READ_UP
#define NF_PRCQ_TLC_READ_UP (INVALID_2F-29)
#endif
//Multi plane write/read
#ifndef NF_PRCQ_TLC_PRG_1ST_MULTIPLN
#define NF_PRCQ_TLC_PRG_1ST_MULTIPLN (INVALID_2F-30)
#endif
#ifndef NF_PRCQ_TLC_PRG_2ND_MULTIPLN
#define NF_PRCQ_TLC_PRG_2ND_MULTIPLN (INVALID_2F-31)
#endif
#ifndef NF_PRCQ_TLC_PRG_3RD_MULTIPLN
#define NF_PRCQ_TLC_PRG_3RD_MULTIPLN (INVALID_2F-32)
#endif
#ifndef NF_PRCQ_TLC_PRG_1ST_LP_MULTIPLN
#define NF_PRCQ_TLC_PRG_1ST_LP_MULTIPLN (INVALID_2F-33)
#endif
#ifndef NF_PRCQ_TLC_PRG_1ST_MP_MULTIPLN
#define NF_PRCQ_TLC_PRG_1ST_MP_MULTIPLN (INVALID_2F-34)
#endif
#ifndef NF_PRCQ_TLC_PRG_1ST_UP_MULTIPLN
#define NF_PRCQ_TLC_PRG_1ST_UP_MULTIPLN (INVALID_2F-35)
#endif
#ifndef NF_PRCQ_TLC_PRG_2ND_LP_MULTIPLN
#define NF_PRCQ_TLC_PRG_2ND_LP_MULTIPLN (INVALID_2F-36)
#endif
#ifndef NF_PRCQ_TLC_PRG_2ND_MP_MULTIPLN
#define NF_PRCQ_TLC_PRG_2ND_MP_MULTIPLN (INVALID_2F-37)
#endif
#ifndef NF_PRCQ_TLC_PRG_2ND_UP_MULTIPLN
#define NF_PRCQ_TLC_PRG_2ND_UP_MULTIPLN (INVALID_2F-38)
#endif
#ifndef NF_PRCQ_TLC_PRG_3RD_LP_MULTIPLN
#define NF_PRCQ_TLC_PRG_3RD_LP_MULTIPLN (INVALID_2F-39)
#endif
#ifndef NF_PRCQ_TLC_PRG_3RD_MP_MULTIPLN
#define NF_PRCQ_TLC_PRG_3RD_MP_MULTIPLN (INVALID_2F-40)
#endif
#ifndef NF_PRCQ_TLC_PRG_3RD_UP_MULTIPLN
#define NF_PRCQ_TLC_PRG_3RD_UP_MULTIPLN (INVALID_2F-41)
#endif
#ifndef NF_PRCQ_TLC_READ_LP_MULTIPLN
#define NF_PRCQ_TLC_READ_LP_MULTIPLN (INVALID_2F-42)
#endif
#ifndef NF_PRCQ_TLC_READ_MP_MULTIPLN
#define NF_PRCQ_TLC_READ_MP_MULTIPLN (INVALID_2F-43)
#endif
#ifndef NF_PRCQ_TLC_READ_UP_MULTIPLN
#define NF_PRCQ_TLC_READ_UP_MULTIPLN (INVALID_2F-44)
#endif
//Read retry
#ifndef NF_PRCQ_TLC_RETRY_PRE
#define NF_PRCQ_TLC_RETRY_PRE (INVALID_2F-45)
#endif
#ifndef NF_PRCQ_TLC_RETRY_ADJ0
#define NF_PRCQ_TLC_RETRY_ADJ0 (INVALID_2F-46)
#endif
#ifndef NF_PRCQ_TLC_RETRY_ADJ1
#define NF_PRCQ_TLC_RETRY_ADJ1 (INVALID_2F-47)
#endif
#ifndef NF_PRCQ_TLC_RETRY_ADJ2
#define NF_PRCQ_TLC_RETRY_ADJ2 (INVALID_2F-48)
#endif
#ifndef NF_PRCQ_TLC_RETRY_ADJ3
#define NF_PRCQ_TLC_RETRY_ADJ3 (INVALID_2F-49)
#endif
#ifndef NF_PRCQ_TLC_RETRY_EN
#define NF_PRCQ_TLC_RETRY_EN (INVALID_2F-50)
#endif

// ===== TSB 2D-TLC(TLC Mode) =====
// SLC to SLC
#ifndef NF_PRCQ_SLC_COPY_SLC
#define NF_PRCQ_SLC_COPY_SLC (INVALID_2F-51)
#endif
#ifndef NF_PRCQ_SLC_COPY_SLC_MULTIPLN
#define NF_PRCQ_SLC_COPY_SLC_MULTIPLN (INVALID_2F-52)
#endif
// SLC to TLC
#ifndef NF_PRCQ_SLC_COPY_TLC_1ST
#define NF_PRCQ_SLC_COPY_TLC_1ST (INVALID_2F-53)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_2ND
#define NF_PRCQ_SLC_COPY_TLC_2ND (INVALID_2F-54)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_3RD
#define NF_PRCQ_SLC_COPY_TLC_3RD (INVALID_2F-55)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_1ST_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_1ST_MULTIPLN (INVALID_2F-56)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_2ND_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_2ND_MULTIPLN (INVALID_2F-57)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_3RD_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_3RD_MULTIPLN (INVALID_2F-58)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_1ST_LP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_1ST_LP_MULTIPLN (INVALID_2F-59)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_1ST_MP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_1ST_MP_MULTIPLN (INVALID_2F-60)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_1ST_UP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_1ST_UP_MULTIPLN (INVALID_2F-61)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_2ND_LP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_2ND_LP_MULTIPLN (INVALID_2F-62)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_2ND_MP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_2ND_MP_MULTIPLN (INVALID_2F-63)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_2ND_UP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_2ND_UP_MULTIPLN (INVALID_2F-64)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_3RD_LP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_3RD_LP_MULTIPLN (INVALID_2F-65)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_3RD_MP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_3RD_MP_MULTIPLN (INVALID_2F-66)
#endif
#ifndef NF_PRCQ_SLC_COPY_TLC_3RD_UP_MULTIPLN
#define NF_PRCQ_SLC_COPY_TLC_3RD_UP_MULTIPLN (INVALID_2F-67)
#endif

// ===== IM 3D-TLC(TLC Mode) =====
//Single plane write/read
#ifndef NF_PRCQ_TLC_PRG
#define NF_PRCQ_TLC_PRG (INVALID_2F-68)
#endif
#ifndef NF_PRCQ_TLC_PRG_LOW_PG
#define NF_PRCQ_TLC_PRG_LOW_PG (INVALID_2F-69)
#endif
#ifndef NF_PRCQ_TLC_READ
#define NF_PRCQ_TLC_READ (INVALID_2F-70)
#endif
//Multi plane write/read
#ifndef NF_PRCQ_TLC_PRG_MULTIPLN
#define NF_PRCQ_TLC_PRG_MULTIPLN (INVALID_2F-71)
#endif
#ifndef NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG
#define NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG (INVALID_2F-72)
#endif
#ifndef NF_PRCQ_TLC_READ_MULTIPLN
#define NF_PRCQ_TLC_READ_MULTIPLN (INVALID_2F-73)
#endif


#ifndef NF_PRCQ_READ_STS_ENHANCE
#define NF_PRCQ_READ_STS_ENHANCE (INVALID_2F-74)
#endif

#ifndef NF_PRCQ_RESET_NOSTS
#define NF_PRCQ_RESET_NOSTS (INVALID_2F-75)
#endif

#ifndef NF_PRCQ_SLC_SNAP_READ
#define NF_PRCQ_SLC_SNAP_READ (INVALID_2F-76)
#endif

#ifndef NF_PRCQ_TLC_SNAP_READ
#define NF_PRCQ_TLC_SNAP_READ (INVALID_2F-77)
#endif

#ifndef NF_PRCQ_HALF_PRG_1PLN
#define NF_PRCQ_HALF_PRG_1PLN (INVALID_2F-78)
#endif

#ifndef NF_PRCQ_HALF_READ_1PLN
#define NF_PRCQ_HALF_READ_1PLN (INVALID_2F-79)
#endif

#endif /* __HAL_FLASH_DRIVER_BASIC_H__ */
