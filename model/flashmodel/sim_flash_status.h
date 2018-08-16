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
*******************************************************************************/

#ifndef SIM_FLASH_STATUS_H
#define SIM_FLASH_STATUS_H

#include "BaseDef.h"
#include "sim_nfccmd.h"
#include "sim_flash_config.h"
#include "sim_flash_shedule.h"
#include "flash_meminterface.h"
#include "Flash_opinfo.h"
#include "sim_flash_interface.h"

//abby move the follow macro from HAL_FlashChipDefine.h because it only for model
//#define PAIR_PAGE_PER_BLK           (PG_PER_BLK)
//#define PAIR_PAGE_PRG_MAX           (3)

typedef struct _PRE_CMD_INFO
{
    U32 bsLun       : 8;
    U32 bsCmdType   : 8;
    U32 bsPage      : 16;
    U32 bsTLCMode   : 8;
    U32 Rsvd        : 24;
    U16 bsBlk[PLN_PER_LUN];
}PRE_CMD_INFO;

typedef struct _FLASH_LUN_INFO
{
    U32 bsSLCMode       : 1;    // 0-TLC Mode, 1-SLC Mode
    U32 bsRetryEn       : 1;    // 0-Normal, 1-Read Retry
    U32 bsCacheWr       : 1;    // 0-No Cache Wr operation, 1-Cache Wr
    U32 bsCacheRd       : 1;    // 0-No Cache Rd operation, 1-Cache Rd
    U32 bsBadBlockCnt   : 14;
    U32 bsRsv           : 14;
}FLASH_LUN_INFO;

typedef struct _FLASH_BLOCK_INFO
{
    U32 bsBlockType     : 3;    // 0-Invalid, 1-SLC, 2-MLC, 3-TLC, 4-LP_Only
    U32 bsBlockSts      : 3;    // 0-Open, 1-Closed, 2-Erased, 3-Erasing Suspended, 4-bad Block
    U32 bsPeCnt         : 16;
    U32 bsReadCnt       : 16;
    U32 bsEraseFaiCnt   : 16;
    U32 bsInBBT         : 1;
    U32 bsIsPrgFail     : 1;
    U32 bsRsv           : 8;
    ERR_HANDLE tBlockErrHandel;
}FLASH_BLOCK_INFO;

typedef struct _FLASH_PAIRPG_INFO
{
    U32 bsLpIsWritten   : 1;    // indicate low page is written, 0- no write, 1-write
    U32 bsMpIsWritten   : 1;    // indicate middle page is written, 0- no write, 1-write
    U32 bsUpIsWritten   : 1;    // indicate upper page is written, 0- no write, 1-write
    U32 bsPageSts       : 3;    // 0-Free, 1-Open, 2-Closed, 3-Programming, 4-Programming Suspended, 5-program-fail
    U32 bsProgStep      : 2;    // 0, 1, 2 
    U32 bsSrcSeed0      : 8;
    U32 bsSrcSeed1      : 8;
    U32 bsSrcSeed2      : 8;
}FLASH_PAIRPG_INFO;

typedef struct _FLASH_RUNTIME_STATUS
{
    FLASH_LUN_INFO aFlashLunInfo[NFC_MODEL_LUN_SUM];
    FLASH_BLOCK_INFO aFlashBlkInfo[NFC_MODEL_LUN_SUM][PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN];
    FLASH_PAIRPG_INFO aFlashPgInfo[NFC_MODEL_LUN_SUM][PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN][PAIR_PAGE_PER_BLK];
}FLASH_RUNTIME_STATUS;

typedef enum _FLASH_BLOCK_TYPE
{
    BLOCK_INVALID = 0,
    BLOCK_SLC,
    BLOCK_MLC,
    BLOCK_TLC,
    BLOCK_TLC_LOWPG,
}FLASH_BLOCK_TYPE;

typedef enum _BLOCK_STATUS
{
    BLOCK_OPEN = 0,
    BLOCK_CLOSE,
    BLOCK_ERASED,
    BLOCK_ERASING_SUSPENDED,
    BLOCK_BAD,
}BLOCK_STATUS;

typedef enum _FLASH_PAGE_TYPE
{
    FLASH_LOW_PAGE = 0,
    FLASH_MID_PAGE,         // If not 2D-TLC, it means high page
    FLASH_HIGH_PAGE,        // If intel-3D-TLC, it means extra page
}FLASH_PAGE_TYPE;

typedef enum _FLASH_PAIR_PAGE_TYPE
{
    LOW_PAGE_ONLY = 0,
    LOW_MID_PAGE,
    LOW_MID_HIGH_PAGE,
}FLASH_PAIR_PAGE_TYPE;

typedef enum _PAIR_PAGE_STATUS
{
    PAIR_PAGE_FREE = 0,
    PAIR_PAGE_OPEN,
    PAIR_PAGE_CLOSE,
    PAIR_PAGE_PROGRAM_SUSPENDED,
    PAIR_PAGE_PROGRAM_FAIL,
}PAIR_PAGE_STATUS;

/* flash runtime status related interface */
void FlashStsM_SetLunMode(U8 ucLun, U8 bTrue);
void FlashStsM_SetReadRetryEn(U8 ucLun, U8 bTrue);
void FlashstsM_SetLunCacheWr(U8 ucLun, U8 bTrue);
void FlashstsM_SetLunCacheRd(U8 ucLun, U8 bTrue);
void FlashstsM_AddLunBBCnt(U8 ucLun);

U8 FlashstsM_GetLunMode(U8 ucLun);
BOOL FlashStsM_GetReadRetryEn(U8 ucLun);
BOOL FlashstsM_GetLunCacheWr(U8 ucLun);
BOOL FlashstsM_GetLunCacheRd(U8 ucLun);
U16 FlashstsM_GetLunBBCnt(U8 ucLun);

void FlashStsM_SetBlkSts(U8 ucLun, U8 ucPln, U16 usBlk, U8 ucStatus);
void FlashStsM_AddBlkPeCnt(U8 ucLun, U8 ucPln, U16 usBlk);
void FlashStsM_AddBlkReadCnt(U8 ucLun, U8 ucPln, U16 usBlk);
void FlashStsM_AddBlkEraseFailCnt(U8 ucLun, U8 ucPln, U16 usBlk);
U8 FlashStsM_GetBlkType(U8 ucLun, U8 ucPln, U16 usBlk);
U8 FlashStsM_GetBlkSts(U8 ucLun, U8 ucPln, U16 usBlk);
U16 FlashStsM_GetBlkPeCnt(U8 ucLun, U8 ucPln, U16 usBlk);
U32 FlashStsM_GetBlkReadCnt(U8 ucLun, U8 ucPln, U16 usBlk);
U32 FlashStsM_GetBlkEraseFailCnt(U8 ucLun, U8 ucPln, U16 usBlk);
BOOL FlashStsM_CheckStsFree(U8 ucLun, U8 ucPln, U16 usBlk);
BOOL FlashStsM_CheckEraseFail(U8 ucLun, U8 ucPln,U16 usBlk);

void FlashStsM_SetLpMpUpWrite(PFLASH_PHY pFlash_phy);
void FlashStsM_SetPairPgSts(PFLASH_PHY pFlash_phy, U8 ucPgSts);
void FlashStsM_SetPairPgSrcSeed(PFLASH_PHY pFlash_phy, NFCQ_ENTRY *pNFCQEntry, U32 ulSeedIndex);
U8 FlashStsM_GetLpMpUpWrite(PFLASH_PHY pFlash_phy);
U8 FlashStsM_GetPairPgSts(PFLASH_PHY pFlash_phy);
U8 FlashStsM_GetNextPairPgSts(PFLASH_PHY pFlash_phy);
U8 FlashStsM_GetPairPgPrgStep(PFLASH_PHY pFlash_phy);
U8 FlashStsM_GetPairPgSrcSeed(PFLASH_PHY pFlash_phy);
BOOL FlashStsM_CheckPairPgType(PFLASH_PHY pFlash_phy);  //check page's prog type is correct (LP-only, LP+MP, LP+MP+UP, Mp+Up)
BOOL FlashStsM_CheckPairPgPrg(PFLASH_PHY pFlash_phy);
BOOL FlashStsM_CheckPairPgSrcSeed(U8 ucSeed, NFCQ_ENTRY *pNFCQEntry, U32 ulSeedIndex);
BOOL FlashStsM_CheckPrgFail(PFLASH_PHY pFlash_phy);
BOOL FlashStsM_NextWLOpenCheck(PFLASH_PHY pFlash_phy); 
void FlashStsM_SetPreCmdInfo(PFLASH_PHY pFlash_phy, ST_FLASH_CMD_PARAM *pFlashCmdParam);
void FlashStsM_ClrPreCmdInfo(U8 ucLun);
void FlashStsM_CheckCCRParam(const NFCM_LUN_LOCATION *pLunLocation, ST_FLASH_CMD_PARAM *pFlashCmdParam);

/* Init Flash ststu */
void FlashStsM_InitFlashStatus(void);

/* Reset pair page status */
void FlashStsM_ResetSts(U8 ucLun, U8 ucPln, U16 usBlk);

#endif
