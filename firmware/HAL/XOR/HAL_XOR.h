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
#ifndef __HAL_XOR_H__
#define __HAL_XOR_H__

#include "BaseDef.h"
#include "HAL_FlashChipDefine.h"

//#if defined(FLASH_TLC)   // TLC:1(48KB) + 4(16KB) XOR engine.
//#define TLC_WRITE_XORE_CNT  1
//#define XORE_CNT   (6 - TLC_WRITE_XORE_CNT)
//#elif (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC) || defined(FLASH_L85) ||\
//       defined(FLASH_L95) || defined(FLASH_TSB) || defined(FLASH_YMTC_3D_MLC))// MLC:8(16KB) XOR engine.
//#define XORE_CNT   6
//#else
//#error "Flash type must be defined!"
#define TLC_WRITE_XORE_CNT  2
#define XORE_CNT   8
//#endif

// Only used in Bypass NFC mode, indicate the count of source page DRAM Address register.
#define XOR_SRC_DADDR_REG_CNT   4
// Restricted by "bsRedAddXorPar0" of NFCQ entry.
#define XOR_REDUN_ALIGN_BITS    3  

typedef enum _XOR_TARGET
{
    XOR_RECOVER = 0,
    XOR_PROTECT,
    XOR_STORE_TO_DRAM = 0,
    XOR_LOAD_FROM_DRAM,
    XOR_TARGET_ASSERT
}XOR_TARGET;

typedef enum _XOR_SATUS
{
    XOR_IDLE = 1,
    XOR_CALCULATING = 2,
    XOR_FINISH = 4,           // XOR operation finished, all pages be processed;
    XOR_PARTIAL_FINISH = 8,   // XOR operation finished, but not all pages be processed;
}XOR_SATUS;

typedef struct _XOR_SOURCE_DATA_DRAM_ADDRESS
{
    U32 aPageDramAddr[XOR_SRC_DADDR_REG_CNT];
    U32 aRedunDramAddr[XOR_SRC_DADDR_REG_CNT];
    U32 ulValidAddrCnt;
}XOR_SRC_DATA;

// Only need to be called once, when the system is initializing.
void HAL_XorInit(BOOL bInterruptEn, BOOL bPowerManagementEn);


// Used in any mode, and must be called.
BOOL HAL_GetXore(U32 ulXorPageSize, U32 * pXoreId);

// Used in all scene except protection scene of NFC mode, and must be called in these scenes.
void HAL_ReleaseXore(U32 ulXoreId);

// Can be used in any mode, and can be called optionally. Don't use in protection scene.
void HAL_XorePartialOperate(U32 ulXoreId, U32 ulStartCodeWord, U32 ulCodeWordCnt);

// Only used in NFC mode, and must be called.
void HAL_XoreNfcModeConfig(U32 ulXoreId, XOR_TARGET eTarget, U32 ulPageProtectRatio,
    U32 ulPreRdyPageCnt, BOOL bIsSemiFinishedWork);

// Only used in Bypass NFC mode, and must be called.
void HAL_XoreBpsModeConfig(U32 ulXoreId, XOR_TARGET eTarget, U32 ulPageProtectRatio,
    U32 ulRedunLength, BOOL bDramCrcEn);
void HAL_XoreSetSrcDataAddr(U32 ulXoreId, const XOR_SRC_DATA *pSourceDataAddr, BOOL bHaveFirstXorPage);

// Only used in NFC mode or Bypass NFC mode, and can be called optionally.
void HAL_XoreAutoLoad(U32 ulXoreId, U32 ulDestXorPageDramAddr, U32 ulDestRedunDramAddr);

// Only used in Load-Store mode, and must be called.
void HAL_XoreLoadStoreModeConfig(U32 ulXoreId, XOR_TARGET eTarget, U32 ulXorPageDramAddr,
    U32 ulRedunDramAddr, U32 ulRedunLength, BOOL bDramCrcEn);


// Used in any mode, and must be called.
void HAL_XoreTrigger(U32 ulXoreId);
void HAL_XoreStop(U32 ulXoreId);


// Used in any mode, and can be called optionally.
XOR_SATUS HAL_XoreGetStatus(U32 ulXoreId);
U32 HAL_XoreGetDonePageCount(U32 ulXoreId);

// Used in any mode, and can be called optionally.
U32 HAL_XoreGetParityPageAddr(U32 ulXoreId);
void HAL_XoreSetXorPageSize(U32 ulXoreId, U32 ulXorPageSize);
U32 HAL_XoreGetParityPageOffset(U32 ulXoreId);
void HAL_XoreSetParityPageOffset(U32 ulXoreId, U32 ulParityOtfbOffset);
U32 HAL_XoreGetParityRedunOffset(U32 ulXoreId);
void HAL_XoreSetParityRedunOffset(U32 ulXoreId, U32 ulRedunOtfbOffset);

BOOL XorUt_IsXoreValid(U32 ulXoreId);
void XorUt_SetXoreValidOnly(U32 ulXoreId);
    
#endif // __HAL_XOR_H__

