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

#ifndef _H_FMODULE_H
#define _H_FMODULE_H
#include <windows.h>
#include <COM/BaseDef.h>
#include "sim_flash_shedule.h"

#define COM_DATA_TYPE 0x01
#define RAND_DATA_TYPE 0x02
#define RSV_DATA_TYPE 0x04
#define BAD_BLK_MARK    0x00 //move from HAL_FlashChipDefine.c by abby

int crs_un_init(void);

typedef struct _FLASH_PRG_ORDER_
{
    U16 bsPrePrgPPO[PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN];
    U16 bsNextPrgPPO[PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN];
    U16 bsNextPrgWL[PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN];
    U16 bsPrgOrder[PLN_PER_LUN][BLK_PER_PLN + RSV_BLK_PER_PLN];

}FLASH_PRG_ORDER;

typedef struct _FLASH_PHY
{
    union
    {
        struct
        {
            UINT16 bsPhyPuInTotal : 8;
            UINT16 bsLunInCe : 8;
            UINT8 ucLunInTotal;
            UINT8 nPln;
            UINT16 nBlock;
            UINT16 nPage;
        };

        UINT64 u64Addr;
    };

}FLASH_PHY,*PFLASH_PHY;

typedef struct FlashSpecialInterface
{
    U8 (*GetCmdType) (U8 ucCmdCode);
    U8 (*GetPgType) (U16 usPage, U8 ucBlkType);
    U8 (*GetPairPgType) (U16 usPairPage, U8 ucBlkType);
    U8(*GetPlnNum) (U8 ucCmdCode);
    U16 (*GetPairPage) (U16 usPage, U8 ucBlkType);
    void (*SetBlkType) (U8 ucLun, U8 ucPln, U16 usBlk, U16 usPage, U8 ucCmdType, BOOL bsTLCMode);
    void (*IncPairPgPrgStep) (PFLASH_PHY pFlash_phy);
    void (*ResetPrgOrder) (U8 ucLun, U16 usBlk, U8 ucPln);
    void (*GetWriteFlashAddr) (const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, U32 ulPlnIndex, U32 ulAtomPageIndex, U8 ulPageInWl, BOOL IsTLCMode, FLASH_PHY *pFlashAddr);
    void (*CheckTransferLen) (U8 ucComCode, NFCQ_ENTRY * pNfcqEntry);
    BOOL (*CheckBlkType) (U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdType, BOOL bsTLCMode);
    BOOL (*CheckErsType) (U8 ucLun, U8 ucPln, U16 usBlk, U8 ucCmdCode);
    BOOL (*CheckPrgOrder) (PFLASH_PHY pFlash_phy, char *pDataBuf, char *pRedBuf, int nType, U8 ucTlcPrgCycle, ST_FLASH_CMD_PARAM *pFlashCmdParam);
    BOOL (*ReadCheck) (PFLASH_PHY pFlash_phy);
    BOOL (*GetBlkMode) (U8 ucCmdCode);
    BOOL (*GetInterCopy) (U8 ucCmdCode);

}FlashSpecInterface;

extern FlashSpecInterface g_tFlashSpecInterface;
// sata interface
int Mid_init_SATA();
//void Mid_Read_SATA(unsigned long ulLBA, char* pBuf, int nLength);
void Mid_Read_SATA(UINT32 ulLBA, char* pBuf, int nLength);
void Mid_Write_SATA(unsigned long ulLBA, char* pBuf, int nLength);
void Mid_Erase_SATA(unsigned long ulLBA);
//void MID_Un_Init_SATA();

// flash interface
void Mid_Init_Ex();
//void Mid_Init(int nPu, int nPln, int nBlk, int nPge, unsigned long ulCapacity, unsigned long ulPgeSize, int nRsvPer);
U8 Mid_Read(PFLASH_PHY pFlash_phy, char* pDataBuf, char* pRedBuf, int nDataLen, BOOL bCheckErr, ST_FLASH_CMD_PARAM *pFlashCmdParam, const NFCM_LUN_LOCATION *tNfcOrgStruct, U32 ulSeedIndex);
U8 Mid_Write(PFLASH_PHY pFlash_phy, int nType, char* pDataBuf, char * pRedBuf, int nLength, BOOL bCheckErr, ST_FLASH_CMD_PARAM *pFlashCmdParam, const NFCM_LUN_LOCATION *tNfcOrgStruct, U32 ulSeedIndex);
U8 Mid_Erase(PFLASH_PHY pFlash_phy, BOOL bCheckErr, BOOL bsLLF, ST_FLASH_CMD_PARAM *pFlashCmdParam, const NFCM_LUN_LOCATION *tNfcOrgStruct);
void Mid_CopyTableData(PFLASH_PHY pFlash_phy_source, PFLASH_PHY pFlash_phy_target, BOOL bsTLCMode);
U8 Mid_CopyData(PFLASH_PHY pFlash_phy_source, PFLASH_PHY pFlash_phy_target, BOOL bsTLCMode);
void Mid_Read_RedData(PFLASH_PHY pFlash_phy, char *pRedBuf);
U8 Mid_Read_FlashIDB(PFLASH_PHY pFlash_phy);
U8 Mid_GetFlashErrCode(PFLASH_PHY pFlash_phy);
BOOL Mid_FlashInjError(PFLASH_PHY pFlash_phy, U8 usErrCode, U8 usRetryTime);

void FlashSpecInterfaceFactory(void);
UINT32 Flash_Talbe_GetPageType(PFLASH_PHY pFlash_phy);

#endif
