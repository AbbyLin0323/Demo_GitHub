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
* NON-INFRINGEMENT.
********************************************************************************
Description :
*******************************************************************************/
#ifndef __HAL_XOR_COMMON_H__
#define __HAL_XOR_COMMON_H__

#include "HAL_XOR.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"

// x*16 + (!x)*8; Get Code-Word count of XOR page size. Note x will be evaluated 2 times.
// And user must ensure size of "x" is bigger than "U8", because of "<<  KB_SIZE_BITS".
#define XOR_CWCNT_BY_PAGESIZE_INDEX(x)          (((((x) << 4) + (!(x) << 3)) << KB_SIZE_BITS) >> CW_INFO_SZ_BITS)
// Each general XOR engine have 32KB OTFB SRAM to store XOR parity, 64B OTFB SRAM to store XOR redundant.
#define OTFB_SRAM_PER_GENERAL_XORE   (32 * KB_SIZE)
#define MAX_REDUN_PER_GENERAL_XORE   64
#define XOR_CWCNT_VALUE_SETS         { 1, 2, 4, 8, 16, 32, 48, 64 }

#if defined(FLASH_TLC)   // TLC:1(48KB) + 4(16KB) XOR engine.
#define OTFB_SRAM_PER_TLC_WRITE_XORE (64 * KB_SIZE)
#define MAX_REDUN_PER_TLC_WRITE_XORE (3 * MAX_REDUN_PER_GENERAL_XORE)
#elif (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC)||defined(FLASH_YMTC_3D_MLC))
#define ONE_PASS_3DMLC_WRITE_XORE_CNT    4
#define MAX_REDUN_PER_3DMLC_WRITE_XORE (2 * MAX_REDUN_PER_GENERAL_XORE)
#elif (defined(FLASH_L85) || defined(FLASH_L95) || defined(FLASH_TSB)) // MLC:8(16KB) XOR engine.
// These situation needn't to do anything.
#else
#error "Flash type must be defined!"
#endif


// DEFINITION OF REGISTER STRUCTURE

typedef struct _XOR_PAGE_DRAM_ADDRESS
{
    U32 ulPage;          // DRAM address of the page(size in XOR page size) that will be processed.
    U32 ulRedundant;     // DRAM address of page's redundant.
}XOR_PG_DADDR;

// All the members which end with BpLs, only need to be set in Bypass NFC mode or
// Load-Store mode, in NFC mode firmware will set these information to NFCQ entry.
// All the members which end with Bp, only need to be set in Bypass NFC mode.
// All the members which end with Nf, only need to be set in NFC mode.
typedef struct _XORE_CONFIG_REGISTER
{
    // DWORD 0
    U32 bsPageSize : 3;          // XOR parity page size. 000:8KB; 001:16KB; 010:32KB; 011:48KB; 100:64KB;
    U32 bsTarget : 1;            // Load-Store mode: 0:OTFB->DRAM; 1:DRAM->OTFB; Others: 0:Recovery; 1:Protection;
    U32 bsMode : 2;              // Working mode. 00:NFC mode; 01:Bypass NFC mode; 10:Load-Store mode;
    U32 bsDramCrcEnBpLs : 1;     // If XOR engine need to do CRC, when it access DRAM data. 0:Disabled; 1:Enabled;
    U32 bsRedunLengthBpLs : 4;   // Redundant length of single plane page. 0:No redundant data; 1:8 bytes; ... 8:64 bytes;
    U32 bsPageCountNfBp : 8;     // The count of pages(size in XOR page size) need to do XOR operation.
                                 // Correspond to RXTGC(XOR times) of hardware specification.
                                 // 0x00:1 page; 0x01:2 pages; ... 0xFF:256 pages;
    U32 bsStartCw : 7;           // Offset of the 1st Code-Word that need to be processed in XOR page.Size of
                                 // Code-Word is 1KB, unit of offset is Code-Word.
    U32 bsBufferIdBpLs : 4;      // Offset of XOR parity page in this XOR engine's OTFB SRAM, offset's unit is 4KB.
    U32 bsEnBpLs : 1;            // Correspond to XOR ID of NFC mode, in Bypass NFC & Load-Store mode, if we want
                                 // use a XOR engine, firmware must make it enable first.
    U32 bsValid : 1;             // Protect in NFC mode, XOR engine hardware is responsible for set it to 1,
                                 // when firmware is responsible for clear it to 0. Other scenarios, firmware
                                 // is responsible for both set and clear operation.

    // DWORD 1
    U16: 2;                      // Placeholder, we don't want access these spaces through this struct.
    U16 bsIntEn : 1;             // Interrupt enable. 0:Disabled; 1:Enabled;
    U16 bsPmEn : 1;              // Power management enable. 0:Clock free running; 1:Clock gating;
    U16: 12;                     // Placeholder, we don't want access these spaces through this struct.

    U16: 4;                      // Placeholder, we don't want access these spaces through this struct.
    U16 bsDsgId0Nf : 9;          // See note 1! ID of the 1st DSG which need to be set valid by XOR engine.
    U16 bsSetDsgVldEnNf : 1;     // See note 1! XOR engine set DSG valid enable. 0:Disabled; 1:Enabled;
    U16 bsAutoLoadEnNfBp : 1;    // XOR engine move the result of XOR operation to DRAM automatically enable.
    U16 bsHave1stPageNfBp : 1;   // 0:No; 1:Yes;

    // DWORD 2 ~ 11
    // In Bypass NFC mode, 4 source DRAM address registers used to describe DRAM address of the page which need
    // to do XOR operation and DRAM address of its redundant.1 destination DRAM address register used to describe
    // DRAM address of XOR parity page in every mode. These address all are relative address to DRAM start address.
    XOR_PG_DADDR aSrcDramAddrBp[XOR_SRC_DADDR_REG_CNT];
    XOR_PG_DADDR tDestDramAddr;

    // DWORD 12
    U8 bsPreRdyPageCntNf : 4;    // In NFC mode, in order to let NFC read the last user data, we need send parity
                                 // ready signal ahead of time. Count of user data page(size in XOR page size) in
                                 // the last user data.
    U8 bsPlaneInfo : 4;          // NAND Flash plane information. 00:Single plane; 01:Multi plane,and page size of
                                 // single plane is 8KB; 10:As previous, but 16KB; 11:As previous, but 32KB;
    U8 bsCodeWordCnt : 4;        // Count of Code-Word that need to be processed.
                                 // 0x00:1; 0x01:2; 0x02:4; 0x03:8; 0x04:16; 0x05:32; 0x06:48; 0x07:64
    U8 bsSrcDramAddrVldBp : 4;   // 4'bxxx1:The 1st item of aSrcDramAddrBp is valid; ... 4'b1xxx:The 4th is valid;
    U16 usDW12Resv0;

    // DWORD 13
    U32 bsDsgId1Nf : 9;          // See note 1! ID of the 2nd DSG which need to be set valid by XOR engine.
    U32 bsDsgId2Nf : 9;          // See note 1! ID of the 3th DSG which need to be set valid by XOR engine.
    U32 bsDsgId3Nf : 9;          // See note 1! ID of the 4th DSG which need to be set valid by XOR engine.
    U32 bsDsgIdVldNf : 4;        // See note 1! 4'b0001:Only DsgId0 valid; 4'b0011:DsgId1,0 valid; ... 4'b1111:DsgId3,2,1,0 valid;
    U32 bsDW13Resv0 : 1;

    // DWORD 14
    U32 bsParityOtfbOffset : 19; // Offset of Parity's base address in OTFB SRAM.
    U32 bsRedunOtfbOffset : 10;  // Offset of Redundant's base address in OTFB SRAM.
    U32 bsIsTlcWriteNf : 1;      // Hardware design use this bit judge whether the data is composed of low, middle
                                 // and up page or not. Nowadays, only TLC write command have three page(48KB).
                                 // 0:SLC ro MLC command; 1:TLC write command.
    U32 bsDW14Resv0 : 2;

    // DWORD 15
    U32 ulLastResv;
}XORE_CFG_REG;

// Note 1: XOR hardware don't need to set DSG valid, but they haven't removed these register, so we just leave
// them there.


typedef struct _XORE_STATUS_REGISTER
{
    U32: 32;                     // Placeholder, we don't want access these spaces through this struct.
    U32: 4;                      // Placeholder, we don't want access these spaces through this struct.

    U32 bsCurStatus : 8;         // XOR engine status.
                                 // 0x00000001:Idle;
                                 // 0x00000010:Doing XOR operation;
                                 // 0x00000100:XOR operation finished, all pages be processed;
                                 // 0x00001000:XOR operation finished, but not all pages be processed;
                                 // 0x00010000:Doing CRC check;
                                 // 0x00100000:XOR engine detected CRC error;
                                 // 0x01000000:XOR auto move data between DRAM and OTFB;
                                 // 0x10000000:XOR doing CRC;

    U32 bsDonePageCount : 8;     // The count of pages(size in XOR page size) which already be processed done.
    U32: 12;                     // Placeholder, we don't want access these spaces through this struct.
}XORE_STS_REG;

typedef struct _XORE_CONTROL_REGISTER
{
    U32: 32;                     // Placeholder, we don't want access these spaces through this struct.
    U8  bsTrigger : 1;           // Trigger XOR engine to start XOR operation.
    U8  bsStop : 1;              // Set 1 to this bit field will stop the working XOR engine, and all the register
                                 // is accessible. When trigger this XOR engine next time, this bit field must be
                                 // cleared with trigger bit field simultaneously. 0:Start to do XOR operation;
                                 // 1:Stop to do XOR operation;
    U8: 6;                       // Placeholder, we don't want access these spaces through this struct.
}XORE_CTRL_REG;


// DEFINITION OF DATA STRUCTURE

typedef struct _XOR_ENGINE
{
    volatile XORE_CFG_REG   *pConfigReg;
    volatile XORE_STS_REG   *pStatusReg;
    volatile XORE_CTRL_REG  *pControlReg;
    BOOL bIsParityNeedStore;
}XOR_ENGINE;

typedef enum _XOR_MODE
{
    XOR_NFC_MODE = 0,
    XOR_BPSNFC_MODE,
    XOR_LOADSTORE_MODE
}XOR_MODE;

#endif // __HAL_XOR_COMMON_H__
