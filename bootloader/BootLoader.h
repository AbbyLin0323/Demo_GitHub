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
Filename     : Bootloadr.h
Version      : 
Date         : 
Author       : Tobey
Others: 
Modification History:
*******************************************************************************/
#ifndef __BOOT_LOADER_H__
#define __BOOT_LOADER_H__

#include "BaseDef.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashCmd.h"
#include "HAL_ParamTable.h"
#ifdef SIM
#include "Disk_Config.h"
#endif

#define PU_MAX (CE_MAX)

#define FW_PAGE_BITMAP_LOW  0xffffffff
#define FW_PAGE_BITMAP_HIGH 0xffffffff
#define GB_FWV0_STARTPAGE   3 //V0 FW's start page in GB

#define FW_LOCATION_START_ADDR  (0x40300000 + 16*1024)
#define BOOTLOADER_BUFF0_ADDR   DRAM_BL_TEMP_BUFF_BASE//allocate 32k for BootLoader use
#define BOOTLOADER_BUFF1_ADDR   (BOOTLOADER_BUFF0_ADDR + 16*1024)
#define FW_ENTRANCE_PC          (DRAM_START_ADDRESS + 0x400010)
#define FW_IMG_PAGE_NUM_MAX     512//512*15K=7.5MB (current max system image size)

#define MIN(a, b) ((a<b)?a:b)

#ifndef SIM
#define OTFB_FTABLE_ENTRY_ATTR __attribute__((section(".otfb_FTableEntry")))
#define DRAM_BOOTLOADER_ATTR   __attribute__((section(".BootLoaderPart1.text")))
#define DRAM_BOOTLOADER_DATA_ATTR   __attribute__((section(".BootLoaderPart1.data")))
#else
#define OTFB_FTABLE_ENTRY_ATTR
#define DRAM_BOOTLOADER_ATTR
#define DRAM_BOOTLOADER_DATA_ATTR
#endif

typedef enum _BL_GB_PAGE_TYPE
{
    BL_PG_TYPE_LOADER = 1,
    BL_PG_TYPE_FIRMWARE,
    BL_PG_TYPE_ACTIVE0,
    BL_PG_TYPE_ACTIVE1,
    BL_PG_TYPE_PTable
}BL_GB_PAGE_TYPE;

typedef enum _BL_GB_EXIST_BIT
{
    BL_GB_VX_BIT,
    BL_GB_AX_BIT,
    BL_GB_PT_BIT
}BL_GB_EXIST_BIT;

/* Redundant defines */
typedef union _BL_REDUNDANT
{
    U32 m_Content[RED_SW_SZ_DW];
#ifndef SIM
    struct 
    {
        /* Common Info : 3 DW */
        U32 m_GBPageType:8;
        U32 m_DWRsv0:24;
        U32 m_FWPageNum; //it's usful when m_GBPageType is BL_PG_TYPE_FIRMWARE;
        U32 m_aRedData[RED_SW_SZ_DW-2];        
    };

    struct 
    {
        // Red of Page in GB defaults 8DW  , DWORD[7] for patch empty page for C0
        U32 aData[7];
        U32 ulEmptyPgParity;
    };

#else
    struct 
    {
       RedComm m_RedComm;
       U32 m_FWPageNum; //it's usful when m_GBPageType is BL_PG_TYPE_FIRMWARE;    
    };
#endif
}BL_REDUNDANT;

typedef struct _BL_FW_UPDATE_INFO
{
    U8 ucCurrPU;    //current PU num
    U8 ucNextPU;    //Next PU num
    U8 ucTotalPU;
    U8 ucCurPUExistFlag; //bit0:VX exist; bit1:AX exist; bit2:P_T exist; others:reserve;
    U32 ulCurPUEmpPageStart; //current PU remain num
    U32 ulCurPUVlastStartPage;
    U32 ulOtherPUEmpPageStart; //other PU empty page start 
    U8 ucCurPUAlast;
    U8 ucFwRealSavePGNum;
    U8 ucRsv0;
    U8 ucRsv1;
}BL_FW_UPDATE_INFO;


extern BOOL l_bNfcInitDoneFlag;
extern U16 g_HexDataIndexTable[256];
extern U16 g_StrDataIndexTable[256];
BOOTLOADER_FILE *BL_GetBootLoaderFile(void);
void DRAM_BOOTLOADER_ATTR BL_NFC_InterfaceInit(void);
void BL_NFC_UpdatePuMapping(void);
void BL_SetFeatureAllFlash(U8 ucAddr, U32 ucValue);
void BL_NfcPuMapInit(U32 ulBitMap);
void BL_ResetAllFlash(void);
void BL_ResetAllFlashWaitStatus(void);
void BL_GlobalInit(void);
void BL_DDRInit(void);
void BL_NFCInit(void);
void BL_PCIEInit(void);
void BL_ClkGatingInit(void);
void BL_Flash1stInit(void);// can not be used.
void BL_Flash2ndInit(void);// can not be used.
BOOL BL_SaveFW(U8 ucSlotID, U32 ulDramAddr);
BOOL BL_ActiveFW(U8 ucSlotID);
void BL_SavePTable(void);
void BL_RunFW(void);
void BL_SwitchMode(BOOL ucSwitchToSync);
void BL_ClearDiskLock(U32 ulBufferForGB,U32 ulRedBuf);
const char *BL_GetString(U8 ucId);
U32 BL_GetHex(U8 ucId);
void BL_HWInitCallBackCommon(U8 ucHWInitType);
void BL_Part0SetFeature(U8 Pu, U32 ulData, U8 ucAddr);
BOOL BL_Part0IsLunAccessable(U8 ucPU);

//----Henry add for part0 retry----
U32 HAL_FlashRetryCheckForPart0(U8 ucTime);
U32 HAL_FlashSelRetryParaForPart0(void);
RETRY_TABLE HAL_FlashGetRetryParaTabForPart0(U32 ulIndex);
BOOL HAL_FlashRetryPreConditonForPart0(U8 ucPU);
BOOL HAL_FlashRetrySendParamForPart0(U8 ucPU, RETRY_TABLE *pRetryPara, U8 ucParaNum);
BOOL HAL_FlashRetryEnForPart0(U8 ucPU, BOOL bRetry);
BOOL HAL_FlashRetryTerminateForPart0(U8 ucPU);
//----Henry add for part0 retry end----



#ifdef SIM
void BL_LoadFW(void);
void BL_SaveBLAndFWV0(U32 ulDramAddr);
#endif
#endif
