/****************************************************************************
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
*****************************************************************************
Filename    :L0_ATAGenericLib.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_ParamTable.h"
#include "HAL_MultiCore.h"
#include "HAL_TraceLog.h"
#include "HAL_ParamTable.h"
#include "HAL_DMAE.h"
#include "L0_Config.h"
#include "L0_Interface.h"
#include "L0_Event.h"
#ifndef SIM
#include "HAL_PM.h"
#endif
#ifdef HOST_SATA
#include "HAL_SataIO.h"
#include "L0_SataErrorHandling.h"
#include "L0_ATALibAdapter.h"
#else
#include "HAL_HCT.h"
#include "L0_Ahci.h"
#include "L0_AhciHCT.h"
#include "L0_AhciATACmdProc.h"
#endif
#include "L0_ATAGenericCmdLib.h"
#include "L0_TrimProcess.h"

#include "L0_ViaCmd.h"
#include "Disk_Config.h"

//specify file name for Trace Log
#define TL_FILE_NUM     L0_ATAGenericCmdLib_c

extern GLOBAL U32 g_ulSubSysBootOk;
extern U32 g_ulSubsysNum;
extern U32 g_ulSubsysNumBits;
extern PTABLE *g_pBootParamTable;
extern FW_VERSION_INFO *g_pFwVersionInfo;
extern U32 g_ulATAInfoIdfyPage;
extern U32 g_ulATARawBuffStart, g_ulHostInfoAddr, g_ulDevParamAddr;
extern U32 g_ulRawDataReadyFlag;
extern U32 g_ulFWUpdateImageAddr;
extern FW_UPDATE g_tFwUpdate;
extern BOOL g_bFwUpdateOngoing;
extern U32 g_ulFlushCacheReady;
extern U32 g_ulATAInfoIdfyPage;

extern void L0_FillFwRuntimeInfo(FW_RUNTIME_INFO *pFwInfo);
extern void L0_FwUpgradeCalcImgLen(void);

extern GLOBAL U32 g_ulVarTableAddr;
extern GLOBAL VCM_PARAM g_VCM_Param;
extern GLOBAL PSCQ g_apSCmdQueue[SUBSYSTEM_NUM_MAX];

void L0_VCMGetCurStage(PCB_MGR pSlot, U32 ulCmdCode);

const GPLD g_aGPLDT[] =
{
    {GPL_LOGADDR_LOGDIR, 1}, /* Log root directory, 1 page */
    {GPL_LOGADDR_NCQERR, 1}, /* NCQ error log, 1 page */
    {GPL_LOGADDR_IDFYDATA, 9}, /* Identify device data log, 9 pages */
    {INVALID_4F, 0} /* End flag */
};

const static U16 g_aGPLDirectoryDefault[256]=
{
    0x0001, 0x0001, 0x0002, 0x0002, 0x0000, 0x0000, 0x0001, 0x0002,  // 000~007
    0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 008~015
    0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 016~023
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 024~031
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 032~039
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 040~047
    0x0009, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 048~055
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 056~063
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 064~071
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 072~079
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 080~087
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 088~095
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 096~103
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 104~111
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 112~119
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 120~127
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,  // 128~135
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,  // 136~143
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,  // 144~151
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,  // 152~159
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 160~167
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 168~175
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 176~183
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 184~191
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 192~199
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 200~207
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 208~215
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 216~223
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 224~231
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 232~239
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 240~247
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000   // 248~255
};

const static U32 g_aGPLIdfyDataPageListDefault[128] =
{
    0x00000001, 0x00000000, 0x02010008, 0x06050403,
    0x00000807 // Offset 0 - 19
};

const static U32 g_aGPLIdfyDataDefault[128] =
{
    0x00080001, 0x80000000, 0x06103087, 0x80000000, // Offset 0 - 15
    0x00000000, 0x80000000, 0x00000000, 0x00000000, // Offset 16 - 31
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // Offset 32 - 47
    0x0000321E, 0x80000000, 0x00000000, 0x00000000, // Offset 48 - 63
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // Offset 64 - 79
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // Offset 80 - 95
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // Offset 96 - 111
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // Offset 112 - 127
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

const static MEMORIGHT_SMART g_aMemoRightSmartDefault[]  = 
{
    /*ID      THS    Worst   Name    */
    {0x05,   0,     100,    "Reallocated Sector Count"},
    {0x09,   0,     100,    "Power On Hours"},
    {0x0C,   0,     100,    "Power Cycle Count"},
    {0xB1,   0,     100,    "Wear Leveling Count"},
    {0xB3,   0,     100,    "Used Reserved Block Count(Total)"},
    {0xB5,   0,     100,    "Program Fail Count(Total)"},
    {0xB6,   0,     100,    "Erase Fail Count(Total)"},
    //{0xBA,   0,     100,    "System S-Error Count"},  //leo temp mark
    {0xBB,   0,     100,    "Uncorrectable Error Count"},
    {0xC0,   0,     100,    "Unsafe Shutdown Count"},
    {0xC2,   0,       0,    "Temperature"},
    {0xE8,  10,     100,    "Available Reserved Space"},
    {0xE9,   0,       0,    "Normalized Media Wear-out"},
    {0xF1,   0,     100,    "Total LBAs Written"},
    {0xF2,   0,     100,    "Total LBAs Read"},
    {0xF9,   0,     100,    "Total NAND Writes"},
    {0x00,   0,       0,    "End of Table"},
};

const static U16 g_aATAIdentifyDataDefault[256] =
{
    0x0040, 0x3FFF, 0xC837, 0x0010, 0x0000, 0x0000, 0x003F, 0x0000,  // 000~007
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 008~015
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x312E,  // 016~023
    0x3039, 0x392E, 0x3331, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 024~031
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 032~039
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8010,  // 040~047
#ifdef DISK_CAPACITY_64M
    0x4000, 0x2E00, 0x4000, 0x0000, 0x0000, 0x0007, 0x3FFF, 0x0010,  // 048~055 for PIO
    0x003F, 0xFFF0, 0x0000, 0x0110, 0x0000, 0x0002, 0x0000, 0x0000,  // 056~063 for PIO, 60~61: Total number of sectors for LBA28
    0x0003, 0x0078, 0x0078, 0x0078, 0x0078, 0x4000, 0x0000, 0x0000,  // 064~071
    0x0000, 0x0000, 0x0000, 0x0000, 0x000E, 0x0000, 0x0000, 0x0000,  // 072~079 for PIO
    0x03FC, 0x0028, 0x0068, 0x7408, 0x4120, 0x0008, 0xB400, 0x4120,  // 080~087
    0x0000, 0x0000, 0x0000, 0x00FE, 0x0000, 0x0000, 0x0000, 0x0000,  // 088~095 for PIO
#else // support both PIO and NCQ
    0x4000, 0x2F00, 0x4000, 0x0000, 0x0000, 0x0006, 0x3FFF, 0x0010,  // 048~055 for NCQ/DMA
    0x003F, 0x0000, 0x0000, 0x0110, 0x0800, 0x0000, 0x0000, 0x0007,  // 056~063 for NCQ/DMA, 60~61: Total number of sectors for LBA28
    0x0003, 0x0078, 0x0078, 0x0078, 0x0078, 0x4000, 0x0000, 0x0000,  // 064~071
    0x0000, 0x0000, 0x0000, 0x001F, 0x010E, 0x0000, 0x0000, 0x0000,  // 072~079 for NCQ/DMA
    0x03FC, 0x0028, 0x0068, 0x7408, 0x4120, 0x0008, 0xB400, 0x4120,  // 080~087
    0x007F, 0x0000, 0x0000, 0x00FE, 0x0000, 0x0000, 0x0000, 0x0000,  // 088~095 for NCQ/DMA
#endif
    0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x0000, 0x0000, 0x0000,  // 096~103. 100~103: Total number of sectors for LBA48
    0x0000, 0x0000, 0x4000, 0x0000, 0x5000, 0x0000, 0x0000, 0x0000,  // 104~111
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000, 0x4000,  // 112~119
    0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 120~127

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 128~135
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 136~143
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 144~151
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 152~159
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 160~167
    0x0003, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 168~175
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 176~183
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 184~191
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 192~199
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 200~207
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 208~215
    0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x103F, 0x0000,  // 216~223
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 224~231
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 232~239
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  // 240~247
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A5   // 248~255
};

LOCAL  CHSPARAM l_tCurrCHSMode;
LOCAL  U32 l_ulTrimMaxBlockNum;

GLOBAL U32 g_ulATAGPLBuffStart;
GLOBAL U32 g_ulATAInfoIdfyPage;
GLOBAL U32 g_ulSysLBAMax;
GLOBAL U32 g_ulUsedLBAMax;
GLOBAL U8  g_ucSmartEnable;
GLOBAL U32 g_ulSmartDataBaseAddr;

GLOBAL U16 g_usSecurityStatus;
GLOBAL U8  g_ucSecurityErasePreFlag;
GLOBAL U8  g_ucSecurityPswdVerifyCnt;

GLOBAL U8  g_ucReadMaxFlag;
GLOBAL U8  g_ucHpaEstablishedFlag;
GLOBAL BOOL g_bMultipleDataOpen;

GLOBAL U32 g_ulPowerStatus;
GLOBAL BOOL g_bStartTimer;
GLOBAL U32 g_ulStandbyTimer;//by second unit
GLOBAL U32 g_ulTimerLeftVal;

LOCAL MCU0_DRAM_TEXT void SetIdentifyDeviceDataChecksum(U16 *);
LOCAL MCU0_DRAM_TEXT RD2HFIS* L0_GetRespFis(PCB_MGR);

void L0_ATASetXferParamNCQ(PCB_MGR pSlot);
void L0_ATASetXferParam48(PCB_MGR pSlot);
void L0_ATASetXferParam28(PCB_MGR pSlot);
U32 L0_ATAMediaAccChkParamValid(PCB_MGR pSlot);

/* This routine is used for calculating buffer address for specific GPL log page. */
MCU0_DRAM_TEXT U32 L0_GPLGetLogPageAddr(U32 ulBuffStart, U32 ulLogAddr, U32 ulPageOffset)
{
    U32 ulCurrPageStart = ulBuffStart;
    PGPLD pCurrGPLDesp = (PGPLD)(&(g_aGPLDT[0]));

    while (INVALID_4F != pCurrGPLDesp->ulLogAddr)
    {
        /* Specified log address has not been found. */
        if (pCurrGPLDesp->ulLogAddr != ulLogAddr)
        {
            /* Start address is accumulated. */
            ulCurrPageStart += (pCurrGPLDesp->ulLogPageNum << SEC_SIZE_BITS);
            pCurrGPLDesp++;
        }

        /* Found specified log address. */
        else
        {
            /* Requested page offset exceeds the page number implemented in specified log address. */
            if (ulPageOffset >= pCurrGPLDesp->ulLogPageNum)
            {
                return INVALID_8F;
            }

            else
            {
                ulCurrPageStart += (ulPageOffset << SEC_SIZE_BITS);
                return ulCurrPageStart;
            }
        }
    }

    return INVALID_8F;
}

/* This routine is used for calculating the number of pages implemented for specific GPL log address. */
MCU0_DRAM_TEXT U32 L0_GPLGetLogPageNum(U32 ulLogAddr)
{
    PGPLD pCurrGPLDesp;

    /* Scanning GPL topology one by one. */
    for (pCurrGPLDesp = (PGPLD)(&(g_aGPLDT[0]));
            INVALID_4F != pCurrGPLDesp->ulLogAddr;
            pCurrGPLDesp++)
    {
        if (pCurrGPLDesp->ulLogAddr == ulLogAddr)
        {
            return pCurrGPLDesp->ulLogPageNum;
        }
    }

    return INVALID_8F;
}

MCU0_DRAM_TEXT void L0_ATAInitIdentifyData(void)
{
    PHOST_INFO_PAGE pHostInfo;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    U32 ulCHSSize;

    // 1. Copies default IDENTIFY DEVICE data from RODATA section to L0 buffer.
#ifndef SIM
    HAL_DMAECopyOneBlock(g_ulATAInfoIdfyPage, (U32)g_aATAIdentifyDataDefault, SEC_SIZE);
#else
    /* Yao: This patch avoids the issue of DMAE model not being able to access the RODATA segment
            assigned by Windows simulation environment. */
    COM_MemCpy((U32 *)g_ulATAInfoIdfyPage, (U32 *)g_aATAIdentifyDataDefault, SEC_SIZE/sizeof(U32));
#endif

    pHostInfo = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    g_ulSysLBAMax = pHostInfo->HPAMaxLBA;
    ulCHSSize = pIdentifyData->NumCylinders * pIdentifyData->NumHeads * pIdentifyData->NumSectorsPerTrack;
    pIdentifyData->CurrentSectorCapacityLow = (U16)(ulCHSSize & MSK_4F);
    pIdentifyData->CurrentSectorCapacityHigh = (U16)(ulCHSSize >> 16);

    if (MSK_7F < g_ulSysLBAMax)
    {
        pIdentifyData->UserAddressableSectors = MSK_7F;
    }

    else
    {
        pIdentifyData->UserAddressableSectors = g_ulSysLBAMax;
    }

    pIdentifyData->Max48BitLBA[0] = g_ulSysLBAMax;

    g_ulUsedLBAMax = g_ulSysLBAMax - 1;
    l_tCurrCHSMode.usNumOfHead = pIdentifyData->NumberOfCurrentHeads;
    l_tCurrCHSMode.usSecPerTrk = pIdentifyData->CurrentSectorsPerTrack;
    l_tCurrCHSMode.ulCurrDRQSize = pIdentifyData->CurrentMultiSectorSetting;

    pIdentifyData->WordsPerLogicalSector[0] = (SEC_SIZE / sizeof(U16));
    pIdentifyData->DeviceNorminalFormFactor = 0x3; //2.5 inch SATA disk
    pIdentifyData->DataSetManagementMaxBlockNum = l_ulTrimMaxBlockNum = 0x8; //4KB for maximum trim table size
    pIdentifyData->DataSetManagementFeature.SupportsTrim = TRUE;
    pIdentifyData->AdditionalSupported.SupportTrimLBARetZeroValue = TRUE; //Trimmed LBA range(s) returning zeroed data is supported
    pIdentifyData->AdditionalSupported.SupportTrimLBARetDtmValue = TRUE;
    pIdentifyData->NominalMediaRotationRate = 0x1; //Reports non-rotating media

#ifdef NOT_SUPPORT_NCQ
    pIdentifyData->QueueDepth = 0;
    pIdentifyData->SataCapabilities.NcqFeatureSet = 0;
#endif

#ifdef NCQ_TAG_1
    pIdentifyData->QueueDepth = 0;
#endif

    pIdentifyData->Signature = 0xA5;

    //Modify value for U-Link test
#ifdef HOST_SATA
    if (TRUE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0HIPMEnable)
#else
    if (TRUE == g_pBootParamTable->tAHCIL0Feature.tAHCIL0Feat.bsAHCIL0HIPMEnable)
#endif
    {
        pIdentifyData->SataCapabilities.ReceiptHostInitPmReq = TRUE;
    }

#ifdef HOST_SATA
    if (TRUE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0DIPMEnable)
#else
    if (TRUE == g_pBootParamTable->tAHCIL0Feature.tAHCIL0Feat.bsAHCIL0DIPMEnable)
#endif
    {
        //pIdentifyData->SataFeatureSupport.DipmSsp = TRUE;              // [Yao] We do not need to support DIMP SSP feature
        pIdentifyData->SataFeatureSupport.InitInterfacePm = TRUE;      //DIMP support
    }

#ifdef HOST_SATA
    if (TRUE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0SSPEnable)
#else
    if (TRUE == g_pBootParamTable->tAHCIL0Feature.tAHCIL0Feat.bsAHCIL0SSPEnable)
#endif
    {
        pIdentifyData->SataFeatureSupport.SwSettingPreserve = TRUE;      //Support SSP
        pIdentifyData->SataFeatureEnable.SwSettingPreserveEn = TRUE;      //Enable SSP
    }

#ifdef HOST_SATA
    pIdentifyData->AdditionalSupported.SupportDownloadMicroCodeDMA = TRUE; //Support Download MicroCode DMA cmd for SATA 
    pIdentifyData->CommandSetSupportExt.DownloadMicrocodeMode3 = TRUE;
    pIdentifyData->CommandSetActiveExt.DownloadMicrocodeMode3 = TRUE;   
    pIdentifyData->MinDnldMcrCodeSecCnt = 0x40;   // minimum SecCnt per DOWNLOAD MICROCODE
    pIdentifyData->MaxDnldMcrCodeSecCnt = 0x40;    //maximum SecCnt per DOWNLOAD MICROCODE
#endif

#if defined (HOST_SATA)
    if (TRUE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0PMUEnable)
#elif defined (HOST_AHCI)
    if (TRUE == g_pBootParamTable->tAHCIL0Feature.tAHCIL0Feat.bsAHCIL0PMUEnable)
#endif
    {
        /* Only reports Device Sleep support when sustem-level PM feature is required. */
        pIdentifyData->SataFeatureSupport.DevSleep = TRUE;
        pIdentifyData->SataAdditionalCapbilities.DevSleepToReducedPwrSt = TRUE;
    }

    /*Support Read/Write buffer command*/
    pIdentifyData->CommandSetActive.WriteBuffer = TRUE;
    pIdentifyData->CommandSetActive.ReadBuffer = TRUE;

    /*Set Disk Name*/
    COM_MemByteCopy((U8*)&pIdentifyData->ModelNumber[0], (U8*)&g_pBootParamTable->aFWDiskName[0], 40);

    /*Set Serial Number*/
    COM_MemByteCopy((U8*)&pIdentifyData->SerialNumber[0], (U8*)&g_pBootParamTable->aFWSerialNum[0], 20);

    /*Set World Wide Name*/
    pIdentifyData->WorldWideName[0] = (g_pBootParamTable->ulWorldWideName[0] >> 16) & 0xffff;
    pIdentifyData->WorldWideName[1] = g_pBootParamTable->ulWorldWideName[0] & 0xffff;
    pIdentifyData->WorldWideName[2] = (g_pBootParamTable->ulWorldWideName[1] >> 16) & 0xffff;
    pIdentifyData->WorldWideName[3] = g_pBootParamTable->ulWorldWideName[1] & 0xffff;

    pIdentifyData->SataFeatureSupport.AutoActivate = TRUE;      //Support DMA Auto Activate

    pIdentifyData->TranslationFieldsValid |= 0x1;      //for test SSP-01

    //pIdentifyWord[ 84 ] |=0x10;  //for test SSP-02

    pIdentifyData->CommandSetSupport.HostProtectedArea = TRUE;      //Support HPA

    //pIdentifyWord[ 82 ] |=0x180;  //for test SSP-09, 10

    //pIdentifyWord[ 86 ] |=0x8000;  //for test SSP-12
    //pIdentifyWord[ 119 ] |=0x2;  //for test SSP-12

    pIdentifyData->CommandSetSupport.WriteBuffer = TRUE;   //Support Read/Write Buffer cmd
    pIdentifyData->CommandSetSupport.ReadBuffer = TRUE;

    /* firmware version information */   
    COM_DwordToString(g_pFwVersionInfo->ulFWRleaseVerion, &pIdentifyData->FirmwareRevision[0]);

    return;
}

LOCAL MCU0_DRAM_TEXT RD2HFIS* L0_GetRespFis(PCB_MGR pSlot)
{
#ifdef HOST_AHCI
    return &(pSlot->RFis->RD2HFis);
#else
    return &g_tRFisOfSataMode;
#endif
}

MCU0_DRAM_TEXT LOCAL void SetIdentifyDeviceDataChecksum(U16 *pIdentifyDataStart)
{
    U8  *pucIdentifyData;
    U8  ucSum = 0;
    U32 ulLoop;

    pucIdentifyData = (U8 *)pIdentifyDataStart;

    for (ulLoop = 0; ulLoop <= 510; ulLoop++)
    {
        ucSum += *(pucIdentifyData++);
    }

    *pucIdentifyData = (~ucSum) + 1;

    return;
}

MCU0_DRAM_TEXT LOCAL void SetSmartCheckSum(U8 *pStartAddr)
{
    U8  *pucData;
    U8  ucSum = 0;
    U32 ulLoop;

    pucData = pStartAddr;

    for (ulLoop = 0; ulLoop <= 510; ulLoop++)
    {
        ucSum += *(pucData++);
    }

    *pucData = ( ~ucSum ) + 1;
    
    return;
}

MCU0_DRAM_TEXT LOCAL void SetSmartDefaultVersion(U8 *pucStartAddr)
{
    U8  *pucData;
    U8  ucSum = 0;
    U32 ulLoop;

    pucData = pucStartAddr;

    for (ulLoop = 0; ulLoop <= 510; ulLoop++)
    {
        if(0 == ulLoop)
        {
            *(pucData++) = SMART_LOG_DEFAULT_VERSION;
        }
        else
        {
            *(pucData++) = 0;
        }

    }

    SetSmartCheckSum(pucStartAddr);
    
    return;
}

MCU0_DRAM_TEXT LOCAL void L0_InitSmartAttribute(void)
{
    U8 i;
    PSMART_ATTR_INFO pSmartAttrInfo;
    PSMART_THRESHOLDS   pSmartThresholds;

    pSmartAttrInfo = (PSMART_ATTR_INFO)(g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS));
    pSmartThresholds = (PSMART_THRESHOLDS)(g_ulSmartDataBaseAddr + ((U32)SMART_THRESHOLDS_SECTOR_OFF << SEC_SIZE_BITS));
    COM_MemZero((U32*)pSmartAttrInfo, sizeof(SMART_ATTR_INFO)/sizeof(U32));
    COM_MemZero((U32*)pSmartThresholds, sizeof(SMART_THRESHOLDS)/sizeof(U32));

    pSmartAttrInfo->usVersion = SMART_ATTRIBUTE_VERSION;
    pSmartThresholds->usRevision = SMART_THRESHOLDS_REVISION;

    for(i = 0; i < SMART_ATTRIBUTE_ENTRY_MAX; i++)
    {
        if(0 == g_aMemoRightSmartDefault[i].ucId)
        {
            break;
        }

        pSmartAttrInfo->aAttributeTbl[i].ucId = g_aMemoRightSmartDefault[i].ucId;
        pSmartAttrInfo->aAttributeTbl[i].ucWorst = g_aMemoRightSmartDefault[i].ucWorst;

        pSmartThresholds->aThresholdsTbl[i].ucId = g_aMemoRightSmartDefault[i].ucId;
        pSmartThresholds->aThresholdsTbl[i].ucThresholds = g_aMemoRightSmartDefault[i].ucThresholds;
    }

    SetSmartCheckSum((U8*)pSmartAttrInfo);
    SetSmartCheckSum((U8*)pSmartThresholds);
    return;
}

MCU0_DRAM_TEXT void L0_ATAInitSmartData(void)
{
    U8 *pSmartData;
    U32 ulLoop;
    U32 ulSmartLogDirAddr;
    U32 ulSmartSelfTestAddr;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    pIdentifyData->CommandSetSupport.SmartCommands = TRUE;
    pIdentifyData->CommandSetSupport.SmartErrorLog = TRUE;
    pIdentifyData->CommandSetActive.SmartErrorLog = TRUE;
    pIdentifyData->CommandSetSupport.SmartSelfTest = TRUE;
    pIdentifyData->CommandSetActive.SmartSelfTest = TRUE;

    /*state shall be preserved by the device across power cycle*/
    g_ucSmartEnable = pHostInfoPage->SataSmartEnable;
    if(TRUE == g_ucSmartEnable)
    {
        pIdentifyData->CommandSetActive.SmartCommands = TRUE;
    }
    else
    {
        pIdentifyData->CommandSetActive.SmartCommands = FALSE;
    }

    //Need to test Security by Nick
    if(pIdentifyData->CommandSetSupport.SmartCommands == TRUE)
    {
        g_ucSmartEnable = TRUE;
        pIdentifyData->CommandSetActive.SmartCommands = TRUE;
    }

    L0_InitSmartAttribute();

    ulSmartLogDirAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_DIRECTORY_SECTOR_OFF << SEC_SIZE_BITS));    
    COM_MemZero((U32*)ulSmartLogDirAddr, SEC_SIZE/sizeof(U32));

    pSmartData = (U8 *)ulSmartLogDirAddr;

    /*supports multi-block SMART logs*/
    pSmartData[0]   = 0x01;

    /*LogAddr:01h Summary Smart Error log */
    pSmartData[2]   = 0x01;

    /*LogAddr:02h Comprehensive Smart Error log */
    pSmartData[4]   = 0x02;

    /*LogAddr:03h Extended Comprehensive Smart Error log */
    pSmartData[6]   = 0x02;

    /*LogAddr:06h SMART Self-Test log*/
    pSmartData[12]   = 0x01;

    /*LogAddr:07h Extended SMART Self-Test log*/
    pSmartData[14]   = 0x02;    

    /*LogAddr:09h Selective Self-Test log*/
    pSmartData[18]   = 0x01;

    /*LogAddr:10h NCQ Command Error log*/
    pSmartData[32]   = 0x01;

    /*LogAddr:11h Phy Event Counters log*/
    pSmartData[34]   = 0x01;  

    /*LogAddr:80-9fh Host Specific log*/
    for (ulLoop = 0x80; ulLoop <= 0x9f; ulLoop++)
    {
        pSmartData[2*ulLoop] = 0x10;
    } 

    SetSmartCheckSum(pSmartData);

    ulSmartSelfTestAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_SUMMARY_ERR_SECTOR_OFF << SEC_SIZE_BITS));
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr);

    ulSmartSelfTestAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_SELF_TEST_SECTOR_OFF << SEC_SIZE_BITS));
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr);

    ulSmartSelfTestAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_SELECTIVE_TEST_SECTOR_OFF << SEC_SIZE_BITS));
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr);
     
    ulSmartSelfTestAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_COMPREHENSIVE_SMART_ERR_OFF << SEC_SIZE_BITS));
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr);

    ulSmartLogDirAddr = (g_ulSmartDataBaseAddr + (((U32)SMART_COMPREHENSIVE_SMART_ERR_OFF+1) << SEC_SIZE_BITS));    
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr);

    ulSmartSelfTestAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_HOST_SPEC_80H_OFF << SEC_SIZE_BITS));
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr);    
    
    ulSmartSelfTestAddr = (g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_MAX << SEC_SIZE_BITS));
    SetSmartDefaultVersion((U8 *)ulSmartSelfTestAddr); 

    return;
}

MCU0_DRAM_TEXT void L0_ATAInitGPLog(void)
{
    U32 ulGPLogDirAddr;
    U32 ulIdfyPageListAddr;
    U32 ulIdfySATAPageAddr;
    U32 ulNCQErrAddr;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    /* Adding support bits to IDENTIFY DEVICE data */
    pIdentifyData->CommandSetSupport.GpLogging = TRUE;
    pIdentifyData->CommandSetActive.GpLogging  = TRUE;

    ulGPLogDirAddr     = g_ulATAGPLBuffStart + (GPL_LOGDIR_SEC_OFF << SEC_SIZE_BITS);
    ulIdfyPageListAddr = g_ulATAGPLBuffStart + ((GPL_IDFYDATA_SEC_OFF + GPL_IDFYDATA_PAGE_LIST) << SEC_SIZE_BITS);
    ulIdfySATAPageAddr = g_ulATAGPLBuffStart + ((GPL_IDFYDATA_SEC_OFF + GPL_IDFYDATA_PAGE_SATA) << SEC_SIZE_BITS);
    ulNCQErrAddr       = g_ulATAGPLBuffStart + (GPL_NCQERR_SEC_OFF << SEC_SIZE_BITS);

    /* Set default log directory data and identify device log SATA page data */
    COM_MemCpy((U32 *)ulGPLogDirAddr, (U32 *)g_aGPLDirectoryDefault, SEC_SIZE/sizeof(U32));
    COM_MemCpy((U32 *)ulIdfyPageListAddr, (U32 *)g_aGPLIdfyDataPageListDefault, SEC_SIZE/sizeof(U32));
    COM_MemCpy((U32 *)ulIdfySATAPageAddr, (U32 *)g_aGPLIdfyDataDefault, SEC_SIZE/sizeof(U32));

    /* Clear NCQ error log to all zeroes */
    COM_MemZero((U32 *)ulNCQErrAddr, SEC_SIZE/sizeof(U32));

    return;
}

MCU0_DRAM_TEXT void L0_ATAInitSecurity(void)
{
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    PHOST_INFO_PAGE pHostInfoPage       = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
  
    if(TRUE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0SecurityEnable)
    {
        pIdentifyData->CommandSetSupport.SecurityMode = TRUE;
    }
    pIdentifyData->MasterPasswordID = 0xFFFE; 

    g_usSecurityStatus = pHostInfoPage->SataSecurityStatus;  

    /*update identify data according to security status in log page*/
    if (g_usSecurityStatus & SATA_SECURITY_ENABLED)
    {
        g_usSecurityStatus |= SATA_SECURITY_LOCKED;    /*locked*/
        g_usSecurityStatus &= ~SATA_SECURITY_FROZEN;   /*not forzen*/
        g_usSecurityStatus &= ~SATA_SECURITY_COUNTER_EXPIRED; 

        pIdentifyData->CommandSetActive.SecurityMode = TRUE;
        pIdentifyData->SecurityStatus.AsU16 = g_usSecurityStatus; 

        DBG_Printf("disk lock.\n");
    }
    else
    {
        pIdentifyData->CommandSetActive.SecurityMode = FALSE;

        g_usSecurityStatus |= (1<<0);
        g_usSecurityStatus &= ~SATA_SECURITY_LOCKED;  
        pIdentifyData->SecurityStatus.AsU16 = g_usSecurityStatus;
    }
    pIdentifyData->NormalEraseTime.ReportTimeBitsSel = 1;
    pIdentifyData->NormalEraseTime.TimeRequired = 0x1;

    pIdentifyData->EnhancedEraseTime.ReportTimeBitsSel = 0;
    pIdentifyData->EnhancedEraseTime.TimeRequired = 0x8;
    pIdentifyData->SecurityStatus.EnhancedSecurityEraseSupported = TRUE;

    g_ucSecurityErasePreFlag = FALSE;
    g_ucSecurityPswdVerifyCnt = 5;
    g_usSecurityStatus &= ~SATA_SECURITY_COUNTER_EXPIRED;
    pIdentifyData->SecurityStatus.SecurityCountExpired = FALSE;

    return;
}

//#define STANDBY_TIMER_PERIOD  10

#if 0
MCU0_DRAM_TEXT LOCAL void L0_ATACmdProcIdleToStandby(void)
{
    if (0 == g_ulTimerLeftVal)
    {
        g_ulPowerStatus = SATA_POWER_STANDBY;
        DBG_Printf("Device status is Standby.\n");
    }
    else
    {
        L0_ATACmdStartStandbyTimer(g_ulTimerLeftVal);
    }    

    return;
}

#endif

#ifndef SIM
BOOL L0_ATAPowerProcIdleToStandby(void *pParam)
{
    if (0 == g_ulTimerLeftVal)
    {
        g_ulPowerStatus = SATA_POWER_STANDBY;
        //DBG_Printf("Device status is Standby.\n");
    }

    else
    {
        L0_ATAPowerStartStandbyTimer(g_ulTimerLeftVal);
    }    

    return TRUE;
}

void L0_ATAPowerStartStandbyTimer(U32 ulSecondVal)
{
#ifdef CALC_RINGOSC_CLK
    U32 ulPMUTimerTickCountPerSec = HAL_PMGetTimerTickPerMS() * 1000u;
#endif
    if (0 == ulSecondVal)
    {
        ;
    }

    else if (ulSecondVal <= SATA_STANDBY_TIMER_PERIOD)
    {
#ifdef CALC_RINGOSC_CLK
        HAL_PMStartTimer(PMU_TIMER0, ulSecondVal * ulPMUTimerTickCountPerSec);
#else
        HAL_PMStartTimer(PMU_TIMER0, ulSecondVal * PMU_TIMER_TICKCOUNT_1S);
#endif
        g_ulTimerLeftVal = 0;
    }

    else
    {
#ifdef CALC_RINGOSC_CLK
        HAL_PMStartTimer(PMU_TIMER0, SATA_STANDBY_TIMER_PERIOD * ulPMUTimerTickCountPerSec);
#else
        HAL_PMStartTimer(PMU_TIMER0, SATA_STANDBY_TIMER_PERIOD * PMU_TIMER_TICKCOUNT_1S);
#endif
        g_ulTimerLeftVal = ulSecondVal - SATA_STANDBY_TIMER_PERIOD;
    }

    return;
}

void L0_ATAPowerCancelStandbyTimer(void)
{
    HAL_PMStopTimer(PMU_TIMER0);
    HAL_PMClearTimeOutFlag(PMU_TIMER0);

    return;
}
#endif

MCU0_DRAM_TEXT BOOL L0_ATACmdProcPowerStatusCmd(PCB_MGR pSlot)
{
    U32 ulCmdCode;
    U32 ulSec = pSlot->CFis->Count;

    ulCmdCode = pSlot->CFis->Command;
    switch(ulCmdCode)
    {
        case ATA_CMD_SLEEP:
            g_ulPowerStatus = SATA_POWER_SLEEP;
            break;

        case ATA_CMD_IDLE:
        case ATA_CMD_IDLE_IMMEDIATE:
            g_ulPowerStatus = SATA_POWER_IDLE;
            break;

        default:
            g_ulPowerStatus = SATA_POWER_ACTIVEORIDLE;
            break;
    }

    if ((ATA_CMD_IDLE == ulCmdCode) ||
        (ATA_CMD_STANDBY == ulCmdCode))
    {
        if((ulSec >= 0x1) || (ulSec <= 0xf0))
        {
            /* set 5 seconds ~ 1200 seconds */
            g_ulStandbyTimer = ulSec * 5;
        }
        else if((ulSec >= 0xf1) || (ulSec <= 0xfb))
        {
            /* set 30 minutes ~ 330 minutes */
            g_ulStandbyTimer = (ulSec - 240) * 30;
        }
        else if(ulSec == 0xfc)
        {
            /* set 21 minutes */
            g_ulStandbyTimer = 21 * 60;
        }
        else if(ulSec == 0xfd)
        {
            /* set 8 hours (Between 8 hours and 12 hours in spec description) */
            g_ulStandbyTimer = 8 * 60 * 60;
        }
        else if(ulSec == 0xff)
        {
             /* set 21 minutes 15     seconds */
            g_ulStandbyTimer = (21 * 60) + 15;
        }
        else
        {
            /* standby timer disabled */
            g_ulStandbyTimer = 0;
        }
    }

    // set the protocol type
    pSlot->SATAProtocol = SATA_PROT_NONDATA;

    // for obsolete commands, always set the command status
    // to completed
    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

    L0_AhciSendSimpleResp(pSlot);

    // returning TRUE, indicating that the command is processed
    return TRUE;
}

LOCAL BOOL L0ATACmdCodeBinSrch(const U32 *pArray, const U32 ulLen, const U32 ulKey)
{
    S32 lLow = 0, lHigh = (S32)(ulLen - 1), lMid;
    U32 ulFound = FALSE;

    while (lLow <= lHigh)
    {
        lMid = (lLow + lHigh) >> 1;

        if (ulKey == pArray[lMid])
        {
            ulFound = TRUE;
            break;
        }

        else if (ulKey < pArray[lMid])
        {
            lHigh = lMid - 1;
        }

        else
        {
            lLow = lMid + 1;
        }
    }

    return ulFound;
}

BOOL L0_ATACmdCheckSecurityStatus(PCB_MGR pCmdSlot)
{
    U32 ulAborted;
    U32 ulCmdCode;
    static U32 aLockCmdList[] = 
    {
        ATA_CMD_DATA_SET_MANAGEMENT,
        ATA_CMD_READ_SECTOR,
        ATA_CMD_READ_SECTOR_EXT,
        ATA_CMD_READ_DMA_EXT,
        ATA_CMD_READ_MULTIPLE_EXT,
        ATA_CMD_WRITE_SECTOR,
        ATA_CMD_WRITE_SECTOR_EXT,
        ATA_CMD_WRITE_DMA_EXT,
        ATA_CMD_SET_MAX_ADDRESS_EXT,
        ATA_CMD_WRITE_MULTIPLE_EXT,
        ATA_CMD_WRITE_LOG_EXT,
        ATA_CMD_READ_VERIFY_SECTOR,
        ATA_CMD_READ_VERIFY_SECTOR_EXT,
        ATA_CMD_READ_FPDMA_QUEUED,
        ATA_CMD_WRITE_FPDMA_QUEUED,
        ATA_CMD_READ_MULTIPLE,
        ATA_CMD_WRITE_MULTIPLE,
        ATA_CMD_READ_DMA,
        ATA_CMD_WRITE_DMA,
        ATA_CMD_FLUSH_CACHE,
        ATA_CMD_FLUSH_CACHE_EXT,
        ATA_CMD_SECURITY_SET_PASSWORD,
        ATA_CMD_SECURITY_FREEZE_LOCK,
        ATA_CMD_SECURITY_DISABLE_PASSWORD,
        ATA_CMD_SET_MAX_ADDRESS
    },
        aFrozenCmdList[] =
    {
        ATA_CMD_SECURITY_SET_PASSWORD,
        ATA_CMD_SECURITY_UNLOCK,
        ATA_CMD_SECURITY_ERASE_PREPARE,
        ATA_CMD_SECURITY_ERASE_UNIT,
        ATA_CMD_SECURITY_DISABLE_PASSWORD
    };

    ulCmdCode = (U32)pCmdSlot->CFis->Command;

    if (0 != (g_usSecurityStatus & SATA_SECURITY_LOCKED))
    {
        ulAborted = L0ATACmdCodeBinSrch(aLockCmdList,
            (sizeof(aLockCmdList) >> DWORD_SIZE_BITS),
            ulCmdCode);
    }

    else if (0 != (g_usSecurityStatus & SATA_SECURITY_FROZEN))
    {
        ulAborted = L0ATACmdCodeBinSrch(aFrozenCmdList,
            (sizeof(aFrozenCmdList) >> DWORD_SIZE_BITS),
            ulCmdCode);
    }

    else
    {
        ulAborted = FALSE;
    }

    return ulAborted;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartWriteLog(PCB_MGR pSlot)
{
    U32 ulFinished;
    U8  ucLogPageNum;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;

    ucLogPageNum = (pSlot->CFis->LBALo & MSK_2F);
    ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);

    if(ucLogPageNum >= 0x80 && ucLogPageNum <= 0x9f)
    {
        ulRAWBuffAddr = g_ulSmartDataBaseAddr + (((U32)SMART_HOST_SPEC_80H_OFF+(ucLogPageNum-0x80)) << SEC_SIZE_BITS);
    }
    else if(SMART_LOGADDR_SELECTIVE_SELF_TEST == ucLogPageNum)
    {
        ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_SELECTIVE_TEST_SECTOR_OFF << SEC_SIZE_BITS);
    }
    else
    {
        DBG_Printf("Smart write log error\n");
        DBG_Getch();
    }

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = FALSE;
            pSlot->PIODRQSize = ulDataSecCnt;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);
            pSlot->HCMDLenSec = ulDataSecCnt;

            /* Utilizes response flow as a direct media access PIO command. */
            L0_AhciDataXferPrepRespFISSeq(pSlot);
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                ulFinished = TRUE;
            }
            else
            {
                ulFinished = FALSE;
            }
            break;

        default:
            ASSERT(FAIL);
            break;
    }
    return ulFinished;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartReadLog(PCB_MGR pSlot,U32* pAbort)
{
    U32 ulFinished;
    U8  ucLogPageNum;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;

    ucLogPageNum = (pSlot->CFis->LBALo & MSK_2F);
    ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);
    if(ucLogPageNum >= 0x80 && ucLogPageNum <= 0x9f)
    {
        ulRAWBuffAddr = g_ulSmartDataBaseAddr + (((U32)SMART_HOST_SPEC_80H_OFF + (ucLogPageNum - 0x80)) << SEC_SIZE_BITS);
    }
    else 
    {
        switch (ucLogPageNum)
        {
        case SMART_LOGADDR_LOG_DIRECTORY:
            /* output log directory data to host */
            ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_DIRECTORY_SECTOR_OFF << SEC_SIZE_BITS);
            break;
        case SMART_LOGADDR_SUMMARY_SMART_ERR:
            /* output smart error data to host */
            ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_SUMMARY_ERR_SECTOR_OFF << SEC_SIZE_BITS);
            break;
        case SMART_LOGADDR_COMPREHENSIVE_SMART_ERR:    
            /* SMART Log Directoty report:support 2 log page */
            ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_COMPREHENSIVE_SMART_ERR_OFF << SEC_SIZE_BITS);
            break;
        case SMART_LOGADDR_SMART_SELF_TEST:
            /* output smart self test data to host */
            ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_SELF_TEST_SECTOR_OFF << SEC_SIZE_BITS);
            break;
        case SMART_LOGADDR_SELECTIVE_SELF_TEST:
            /* output selective self test data to host */
            ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_SELECTIVE_TEST_SECTOR_OFF << SEC_SIZE_BITS);
            break;
        default:
            /* just abort other cmds */
            *pAbort = TRUE;
            DBG_Printf("Abort SmartReadLog 0x%x Cmd\n", ucLogPageNum);
            return TRUE;
            //break;
        }
    }

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = TRUE;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);
            pSlot->PIODRQSize = 1;
            pSlot->HCMDLenSec = ulDataSecCnt;//1;

            /* Utilizes response flow as a direct media access PIO command. */
            L0_AhciDataXferPrepRespFISSeq(pSlot);
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_D2H;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                ulFinished = TRUE;
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        default:
            ASSERT(FAIL);
    }

    return ulFinished;

}

MCU0_DRAM_TEXT U32 L0_ATACmdProcReadLogExt(PCB_MGR pSlot)
{
    U32 ulFinished;
    U8  ucLogPageNum;
    U32 ulDataSecCnt;
    U32 ulLogDataAddress;
    U16 usPageIndex;
    U8 ucNotSupport = FALSE;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;

    ucLogPageNum = (pSlot->CFis->LBALo & MSK_2F);
    usPageIndex  = ((pSlot->CFis->LBALo & 0xFF00) >> 8) + (pSlot->CFis->LBAHi & 0x00FF00);
    ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);

    if(ucLogPageNum >= 0x80 && ucLogPageNum <= 0x9f)
    {
        ulLogDataAddress = g_ulSmartDataBaseAddr + (((U32)SMART_HOST_SPEC_80H_OFF+(ucLogPageNum-0x80)) << SEC_SIZE_BITS);
    }
    else 
    {
        switch(ucLogPageNum)
        {            
        case GPL_LOGADDR_LOGDIR:  
            if ((0 != usPageIndex) || (1 < ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + (GPL_LOGDIR_SEC_OFF << SEC_SIZE_BITS);
            }
            break;
        case GPL_LOGADDR_NCQERR: 
            if ((0 != usPageIndex) || (1 < ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                //Need TODO other error handle
                ulLogDataAddress = g_ulATAGPLBuffStart + (GPL_NCQERR_SEC_OFF << SEC_SIZE_BITS);
#ifdef HOST_SATA
                if (ATACMD_NEW == pSlot->ATAState)
                {
                    if (TRUE == L0_IsDiskLocked())
                    {
                        HAL_SataSendSDBQueueCleanACT();
                        L0_UnlockDisk();
                    }
                }
#endif
            }
            break;
        case GPL_LOGADDR_SMART_EXTCOMP: 
            //GPL Log Directory report:support 2 log page 
            if (g_aGPLDirectoryDefault[GPL_LOGADDR_SMART_EXTCOMP] < (usPageIndex + ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + ((GPL_SMART_EXTCOMP_OFF + usPageIndex ) << SEC_SIZE_BITS);
            }
            break;                

        case GPL_LOGADDR_EXTENDED_SELF_TEST:
            //GPL Log Directoty report:support 2 log page 
            if (g_aGPLDirectoryDefault[GPL_LOGADDR_EXTENDED_SELF_TEST] < (usPageIndex + ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + ((GPL_EXT_SELF_TEST_OFF + usPageIndex) << SEC_SIZE_BITS);
            }
            break;  

        case GPL_LOGADDR_IDFYDATA:
            /* We have only implemented page 08H -- SATA log page, currently. Thus other pages have dummy data. */
            /* We must reject host access to reserved pages. */
            if (g_aGPLDirectoryDefault[GPL_LOGADDR_IDFYDATA] < (usPageIndex + ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + ((GPL_IDFYDATA_SEC_OFF + usPageIndex) << SEC_SIZE_BITS);
            }
            break;

        default:
            ucNotSupport = TRUE;
            //ASSERT(FAIL);
            break;
        }
    }
    if (TRUE == ucNotSupport) 
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        L0_AhciSendSimpleResp(pSlot);
        ulFinished = TRUE;
    }
    else
    {
        switch (pSlot->ATAState)
        {
            case (U32)ATACMD_NEW:
                /* Sets command parameters. */
                pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
                pSlot->IsWriteDir = TRUE;
                pSlot->HCMDLenSec = ulDataSecCnt;
                pSlot->PIODRQSize = 1;
                pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

                /* Utilizes response flow as a direct media access PIO command. */
                L0_AhciDataXferPrepRespFISSeq(pSlot);
                L0_AhciDataXferSendRespInfo(pSlot);

                /* Changes command execute state. */
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

            case (U32)ATACMD_SPLITTING:
                pCurrSCmd = NULL;

                for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
                {
                    /* Attempts to acquire one SCMD node from either subsystem. */
                    if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                    {
                        pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                        break;
                    }
                }

                if (NULL != pCurrSCmd)
                {
                    /* Sends a RAW data transfer SCMD to L1. */
                    pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                    pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                    pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;
                    pCurrSCmd->tRawData.ulBuffAddr = ulLogDataAddress;
                    pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                    pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_D2H;

                    L0_PushSCmdNode(ulSubSysIdx);
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                    ulFinished = TRUE;
                }

                else
                {
                    ulFinished = FALSE;
                }

                break;

            default:
                ASSERT(FAIL);
        }
    }

    return ulFinished;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcWriteLogExt(PCB_MGR pSlot)
{
    U32 ulFinished;
    U8  ucLogPageNum;
    U32 ulDataSecCnt;
    U32 ulLogDataAddress;
    U8 ucNotSupport = FALSE;
    U16 usPageIndex;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;

    ucLogPageNum = (pSlot->CFis->LBALo & MSK_2F);
    usPageIndex  = ((pSlot->CFis->LBALo & 0xFF00) >> 8) + (pSlot->CFis->LBAHi & 0x00FF00);
    ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);

    if(ucLogPageNum >= 0x80 && ucLogPageNum <= 0x9f)
    {
        ulLogDataAddress = g_ulSmartDataBaseAddr + (((U32)SMART_HOST_SPEC_80H_OFF+(ucLogPageNum-0x80)) << SEC_SIZE_BITS);
    }
    else 
    {
        switch(ucLogPageNum)
        {            
        case GPL_LOGADDR_LOGDIR:  
            if ((0 != usPageIndex) || (1 < ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + (GPL_LOGDIR_SEC_OFF << SEC_SIZE_BITS);
            }
            break;
        case GPL_LOGADDR_NCQERR: 
            if ((0 != usPageIndex) || (1 < ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                //Need TODO other error handle
                ulLogDataAddress = g_ulATAGPLBuffStart + (GPL_NCQERR_SEC_OFF << SEC_SIZE_BITS);
#ifdef HOST_SATA
                if (ATACMD_NEW == pSlot->ATAState)
                {
                    if (TRUE == L0_IsDiskLocked())
                    {
                        HAL_SataSendSDBQueueCleanACT();
                        L0_UnlockDisk();
                    }
                }
#endif
            }
            break;
        case GPL_LOGADDR_SMART_EXTCOMP: 
            //GPL Log Directory report:support 2 log page 
            if (g_aGPLDirectoryDefault[GPL_LOGADDR_SMART_EXTCOMP] < (usPageIndex + ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + ((GPL_SMART_EXTCOMP_OFF + usPageIndex ) << SEC_SIZE_BITS);
            }
            break;                

        case GPL_LOGADDR_EXTENDED_SELF_TEST:
            //GPL Log Directoty report:support 2 log page 
            if (g_aGPLDirectoryDefault[GPL_LOGADDR_EXTENDED_SELF_TEST] < (usPageIndex + ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + ((GPL_EXT_SELF_TEST_OFF + usPageIndex) << SEC_SIZE_BITS);
            }
            break;  

        case GPL_LOGADDR_IDFYDATA:
            /* We have only implemented page 08H -- SATA log page, currently. Thus other pages have dummy data. */
            /* We must reject host access to reserved pages. */
            if (g_aGPLDirectoryDefault[GPL_LOGADDR_IDFYDATA] < (usPageIndex + ulDataSecCnt))
            {
                ucNotSupport = TRUE;
            }
            else
            {
                ulLogDataAddress = g_ulATAGPLBuffStart + ((GPL_IDFYDATA_SEC_OFF + usPageIndex) << SEC_SIZE_BITS);
            }
            break;

        default:
            ucNotSupport = TRUE;
            //ASSERT(FAIL);
            break;
        }
    }

    if (TRUE == ucNotSupport) 
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
            L0_AhciSendSimpleResp(pSlot);
            ulFinished = TRUE;
        }
    else
    {
        switch (pSlot->ATAState)
        {
            case (U32)ATACMD_NEW:
                /* Sets command parameters. */
                pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
                pSlot->IsWriteDir = FALSE;
                pSlot->HCMDLenSec = ulDataSecCnt;
                pSlot->PIODRQSize = 1;
                pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

                /* Utilizes response flow as a direct media access PIO command. */
                L0_AhciDataXferPrepRespFISSeq(pSlot);
                L0_AhciDataXferSendRespInfo(pSlot);

                /* Changes command execute state. */
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

            case (U32)ATACMD_SPLITTING:
                pCurrSCmd = NULL;

                for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
                {
                    /* Attempts to acquire one SCMD node from either subsystem. */
                    if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                    {
                        pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                        break;
                    }
                }

                if (NULL != pCurrSCmd)
                {
                    /* Sends a RAW data transfer SCMD to L1. */
                    pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                    pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                    pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;
                    pCurrSCmd->tRawData.ulBuffAddr = ulLogDataAddress;
                    pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                    pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;

                    L0_PushSCmdNode(ulSubSysIdx);
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                    ulFinished = TRUE;
                }

                else
                {
                    ulFinished = FALSE;
                }

                break;

            default:
                ASSERT(FAIL);
        }
    }
    return ulFinished;
}

MCU0_DRAM_TEXT LOCAL void L0_SmartSetRawData(U8* ucRawData, U16 usValHigh, U32 ulValLow)
{
    U8 i;

    for(i = 0; i < 4; i++)
    {
        *((U8*)ucRawData + i) = (ulValLow >> (8*i)) & 0xFF;
    }

    for(i = 4; i < 6; i++)
    {
        *((U8*)ucRawData + i) = (usValHigh >> (8*(i-4))) & 0xFF;
    }
    
    return;
}

LOCAL void L0_SmartSetDataValue(U8 ucIndex, U16 usValHigh, U32 ulValLow)
{
    PSMART_ATTR_INFO pSmartAttrInfo;
    PDEVICE_PARAM_PAGE pDeviceParamPage = (PDEVICE_PARAM_PAGE)g_ulDevParamAddr;
    pSmartAttrInfo = (PSMART_ATTR_INFO)(g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS));
    
    L0_SmartSetRawData(pSmartAttrInfo->aAttributeTbl[ucIndex].aRowData, usValHigh, ulValLow);

    if ((pSmartAttrInfo->aAttributeTbl[ucIndex].ucId == 0xE8) ||(pSmartAttrInfo->aAttributeTbl[ucIndex].ucId == 0xE9))
    {   //Available Reserved Space(0xE8) || Normalized Media Wear-out(0xE9)
        pSmartAttrInfo->aAttributeTbl[ucIndex].ucValue = ulValLow;
        pSmartAttrInfo->aAttributeTbl[ucIndex].ucWorst = ulValLow;
    }
    else if (pSmartAttrInfo->aAttributeTbl[ucIndex].ucId == 0xC2)
    {   //Temperature
        pSmartAttrInfo->aAttributeTbl[ucIndex].ucValue = ulValLow;
        pSmartAttrInfo->aAttributeTbl[ucIndex].ucWorst = pDeviceParamPage->WorstTemperature;
    }
    else
    {
        pSmartAttrInfo->aAttributeTbl[ucIndex].ucValue = pSmartAttrInfo->aAttributeTbl[ucIndex].ucWorst;
    }

    return;
}


MCU0_DRAM_TEXT LOCAL void L0_FillSmartReadData(void)
{
    U8 ucIndex = 0;
    U8 *pSmartData;
    FW_RUNTIME_INFO *pFirmwareInfo;
    U32 ulRsvdBlockCount;
    U32 ulRemainSparePercent;
    S32 ulLifePercent;
    U32 TotoalNANDWriteInGiB;

    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    PDEVICE_PARAM_PAGE pDeviceParamPage = (PDEVICE_PARAM_PAGE)g_ulDevParamAddr;

    //Reallocated Sector Count
    L0_SmartSetDataValue(ucIndex, 0, 0);
    ucIndex++;

    //Power On Hours
    L0_SmartSetDataValue(ucIndex, 0, pHostInfoPage->PowerOnHours);
    ucIndex++;

    //Power Cycle Count
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->PowerCycleCnt);
    ucIndex++;

    //Wear Leveling Count (Report TLC Block Average Erase Count)
    pDeviceParamPage->WearLevelingCnt = pDeviceParamPage->AvgEraseCount / TLC_BLK_CNT;    
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->WearLevelingCnt);
    ucIndex++;

    //Used Reserved Block Count(Total) : Include Bad Blocks
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->UsedRsvdBlockCnt);
    ucIndex++;

    //Program Fail Count(Total)
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->ProgramFailCnt);
    ucIndex++;
    
    //Erase Fail Count(Total)
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->EraseFailCnt);
    ucIndex++;

    //Uncorrectable Error Count
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->SYSUECCCnt);
    ucIndex++;

    //Unsafe Shutdown Count
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->SYSUnSafeShutdownCnt);
    ucIndex++;

    //Temperature
    L0_SmartSetDataValue(ucIndex, 0, pDeviceParamPage->SYSTemperature);
    ucIndex++;

    //Available Reserved Space (% Percentage) : Exclude Bad Blocks
    ulRsvdBlockCount = pDeviceParamPage->AvailRsvdSpace + pDeviceParamPage->UsedRsvdBlockCnt;

    if (0 != ulRsvdBlockCount)
    {
        ulRemainSparePercent = ((pDeviceParamPage->AvailRsvdSpace) * 100u) / ulRsvdBlockCount;
    }
    else
    {
        ulRemainSparePercent = 100u;
    }

    L0_SmartSetDataValue(ucIndex, 0, ulRemainSparePercent);
    ucIndex++;

    //Normalized Media Wear-out (% Percentage)
    ulLifePercent = 100 - (pDeviceParamPage->AvgEraseCount / TLC_BLK_CNT * 100 / g_pBootParamTable->ulFlashPECycleVal);
    L0_SmartSetDataValue(ucIndex, 0, ulLifePercent);
    ucIndex++;

    //Total LBAs Written
    L0_SmartSetDataValue(ucIndex, pHostInfoPage->TotalLBAWrittenHigh, pHostInfoPage->TotalLBAWrittenLow);
    ucIndex++;

    //Total LBAs Read
    L0_SmartSetDataValue(ucIndex, pHostInfoPage->TotalLBAReadHigh, pHostInfoPage->TotalLBAReadLow);
    ucIndex++;    

    //Total NAND Writes (Unit : GBytes)
    //B0kKB 1 Pages size 64KB = 0x10000 = (<< 16) NANDWrites Unit GBytes (>> 30) => 30 - 16 = 14
    TotoalNANDWriteInGiB = (pDeviceParamPage->TotalNANDWrites) >> 14;
    L0_SmartSetDataValue(ucIndex, 0, TotoalNANDWriteInGiB);
    ucIndex++;

    pSmartData = (U8 *)(g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS));

    if (pSmartData[363] == 0xF0)
    {
        pSmartData[363] = 0xF9;
    }
    else if (pSmartData[363] == 0xF9)
    {
        pSmartData[363] = 0xF6;
    }
    else if (pSmartData[363] == 0xF6)
    {
        pSmartData[363] = 0xF1;
    }
    else
    {
        pSmartData[363] = 0x00;
    }

    pSmartData[364] = 0x01; 

    /* Off-line data collection capability */
    pSmartData[367] = 0x5B;
    pSmartData[368] = 0x02;

    pSmartData[370] |= (1<<0);/* Device error logging supported */
    pSmartData[372] = 0x02;
    pSmartData[373] = 0x0A;

    /* firmware runtime version information and other informations */
    pFirmwareInfo = (FW_RUNTIME_INFO *)(pSmartData + 388);
    L0_FillFwRuntimeInfo(pFirmwareInfo);

    SetSmartCheckSum((U8 *)(g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS)));
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartReadData(PCB_MGR pSlot)
{
    U32 ulFinished;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    U32 ulSubSysIdx;
    PSCMD pCurrSCmd;
    DEVICE_PARAM_PAGE a_tDevParamPage[SUBSYSTEM_NUM_MAX];

    PDEVICE_PARAM_PAGE pDeviceParamPage = (PDEVICE_PARAM_PAGE)g_ulDevParamAddr;

    //1.Get HostInfo and DeviceParam from L1 
    for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
    {
        L0_IssueAccessDevParamSCmd(ulSubSysIdx,(U32)g_ulDevParamAddr,(U32)GLBINFO_LOAD);
        L0_WaitForAllSCmdCpl(ulSubSysIdx);
        COM_MemCpy((U32 *)&a_tDevParamPage[ulSubSysIdx],(U32*)g_ulDevParamAddr,sizeof(DEVICE_PARAM_PAGE)/sizeof(U32));
    }

    //3.Fill Smart Read Data
    L0_FillSmartReadData();

    //4.Send Cmd
    ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS);
    ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = TRUE;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);
            pSlot->PIODRQSize = 1;
            pSlot->HCMDLenSec = 1;

            /* Utilizes response flow as a direct media access PIO command. */
            L0_AhciDataXferPrepRespFISSeq(pSlot);
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = 1;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_D2H;

                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                ulFinished = TRUE;
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        default:
            ASSERT(FAIL);
    }

    return ulFinished;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartReadThresholds(PCB_MGR pSlot)
{
    U32 ulFinished;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    
    ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);
    ulRAWBuffAddr = g_ulSmartDataBaseAddr + ((U32)SMART_THRESHOLDS_SECTOR_OFF << SEC_SIZE_BITS);

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = TRUE;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);
            pSlot->PIODRQSize = 1;
            pSlot->HCMDLenSec = 1;

            /* Utilizes response flow as a direct media access PIO command. */
            L0_AhciDataXferPrepRespFISSeq(pSlot);
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = 1;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_D2H;

                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                ulFinished = TRUE;
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        default:
            ASSERT(FAIL);
    }

    return ulFinished;   
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartReturnStatus(PCB_MGR pSlot)
{
    PSMART_ATTR_INFO pSmartAttrInfo;
    PSMART_THRESHOLDS  pSmartThresholds;
    U8 ucExceed = FALSE;
    U8 i = 0;

    pSmartAttrInfo = (PSMART_ATTR_INFO)(g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS));
    pSmartThresholds = (PSMART_THRESHOLDS)(g_ulSmartDataBaseAddr + ((U32)SMART_THRESHOLDS_SECTOR_OFF << SEC_SIZE_BITS));
    
    for(i = 0; i < SMART_ATTRIBUTE_ENTRY_MAX; i++)
    {
        if(0 == pSmartAttrInfo->aAttributeTbl[i].ucId)
        {
            break;
        }
    }

    if(FALSE == ucExceed)
    {
       pSlot->CFis->LBALo &= 0xFF;
       pSlot->CFis->LBALo |= (0xC24F << 8);
    }

    else
    {
       pSlot->CFis->LBALo &= 0xFF;
       pSlot->CFis->LBALo |= (0x2CF4 << 8);               
    }

    L0_AhciSendSimpleResp(pSlot);
    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartAutosave(PCB_MGR pSlot)
{
    if(0 == (pSlot->CFis->Count & MSK_4F))
    {
        //g_bSataSmartAutoSave = FALSE;
    }
    else if(0xF1 == (pSlot->CFis->Count & MSK_4F))
    {
        //g_bSataSmartAutoSave = TRUE;
    }

    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmartExecOffline(PCB_MGR pSlot)
{
    U8 *pSmartData;

    pSmartData = (U8 *)(g_ulSmartDataBaseAddr + ((U32)SMART_DATA_SECTOR_OFF << SEC_SIZE_BITS));

    pSmartData[363] = 0xF0;

    pSlot->CFis->LBALo &= 0xFF;
    pSlot->CFis->LBALo |= (0xC24F << 8);
    L0_AhciSendSimpleResp(pSlot);
    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATASoftResetProc(PCB_MGR pSlot)
{
#ifdef HOST_AHCI
    L0_AhciPortMarkNewError(pSlot->PortMgr, pSlot->SlotNum, (U32)PORTEVENT_SOFTRESET);
#endif

    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcObsoleteCmd(PCB_MGR pSlot)
{
    // set the protocol type
    pSlot->SATAProtocol = SATA_PROT_NONDATA;

    // for obsolete commands, always set the command status
    // to completed
    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

    L0_AhciSendSimpleResp(pSlot);

    // returning TRUE, indicating that the command is processed
    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcIdentifyDevice(PCB_MGR pSlot)
{
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;

    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    U32 ulFinished = FALSE;

 #ifdef HOST_SATA
    /* Reporting of current signaling speed*/
    pIdentifyData->SataAdditionalCapbilities.CurNegotiatedSpeed = ((rSDC_FW_Ctrl16 >> 11) & 0x3 ) + 1;
#endif

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = TRUE;
            pSlot->PIODRQSize = 1;
            pSlot->HCMDLenSec = 1;
            pSlot->Ch->PRDBC = SEC_SIZE;
            pSlot->CFis->Count = 1;

            /* Utilizes response flow as a direct media access PIO command. */
            L0_AhciDataXferPrepRespFISSeq(pSlot);

            /* Starts response flow as a direct media access PIO command. */
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);
            break;
        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;
                pCurrSCmd->tRawData.ulBuffAddr = g_ulATAInfoIdfyPage;
                pCurrSCmd->tRawData.ucSecLen   = 1;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_D2H;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;
                
                /* Calculate Identify Device checksum before send the  */
                SetIdentifyDeviceDataChecksum((U16 *)g_ulATAInfoIdfyPage);

                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                ulFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
            break;
    }

    return ulFinished;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSmart(PCB_MGR pSlot)
{
    U32 ulFinished = FALSE;
    U32 ulSubCmd;
    PIDENTIFY_DEVICE_DATA pIdentifyData =  (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    U32 ulCmdAbort = FALSE;

    // determine the smart feature
    ulSubCmd = (U32)pSlot->CFis->FeatureLo;
    switch (ulSubCmd)
    {
    case SMART_FEATURE_READ_DATA:
        if(FALSE == g_ucSmartEnable)
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartReadData(pSlot);
        }
        break;
    case SMART_READ_ATTRIBUTE_THRESHOLDS:
        if(FALSE == g_ucSmartEnable)
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartReadThresholds(pSlot);
        }
        break;
    case SMART_READ_LOG:
        if(FALSE == g_ucSmartEnable)
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartReadLog(pSlot, &ulCmdAbort);
        }
        break;
    case SMART_CMD_WRITE_LOG:
        if(FALSE == g_ucSmartEnable)
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartWriteLog(pSlot);
        }
        break;
    case SMART_ENABLE_OPERATIONS:
        pSlot->SATAProtocol = (U8)SATA_PROT_NONDATA;
        if(0xC24F != ((pSlot->CFis->LBALo >> 8) & MSK_4F))
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            g_ucSmartEnable = TRUE;
            pIdentifyData->CommandSetActive.SmartCommands = TRUE;
            L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
            L0_AhciSendSimpleResp(pSlot);
            ulFinished = TRUE;
        }
        break;
    case SMART_DISABLE_OPERATIONS:
        pSlot->SATAProtocol = (U8)SATA_PROT_NONDATA;
        if(FALSE == g_ucSmartEnable || (0xC24F != ((pSlot->CFis->LBALo >> 8) & MSK_4F)))
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            g_ucSmartEnable = FALSE;
            pIdentifyData->CommandSetActive.SmartCommands = FALSE;
            L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
            L0_AhciSendSimpleResp(pSlot);
            ulFinished = TRUE;
        }
        break;
    case SMART_RETURN_STATUS:
        if(FALSE == g_ucSmartEnable)
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartReturnStatus(pSlot);
        }
        break;
    case SMART_FEATURE_AUTOSAVE_DISEN:
        if(FALSE == g_ucSmartEnable)
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartAutosave(pSlot);
        }
        break;
    case SMART_EXEC_OFFLINE_IMMEDIATE:
        if(FALSE == g_ucSmartEnable || (0xC24F != ((pSlot->CFis->LBALo >> 8) & MSK_4F)))
        {
            ulCmdAbort = TRUE;
        }
        else
        {
            ulFinished = L0_ATACmdProcSmartExecOffline(pSlot);
        }
        break;
    default:
        DBG_Printf("smart feature error\n");
        ulCmdAbort = TRUE;
        break;
    }

    if (TRUE == ulCmdAbort)
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        L0_AhciSendSimpleResp(pSlot);
        ulFinished = TRUE;
    }

    return ulFinished;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSetFeatures(PCB_MGR pSlot)
{
    U32 ulSubCmd;
    U32 ulCmdSptd;
    U32 ulXferMode;
    PIDENTIFY_DEVICE_DATA pIdentifyData;

    ulSubCmd = (U32)pSlot->CFis->FeatureLo;
    pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    ulCmdSptd = TRUE;

    switch (ulSubCmd)
    {
        case ENABLE_WRITE_CACHE:
            pIdentifyData->CommandSetActive.WriteCache = TRUE;
            break;

        case DISABLE_WRITE_CACHE:
            pIdentifyData->CommandSetActive.WriteCache = FALSE;
            break;

       case ENABLE_READ_LOOK_AHEAD:
           pIdentifyData->CommandSetActive.LookAhead = TRUE;
           break;
    
       case DISABLE_READ_LOOK_AHEAD:
           pIdentifyData->CommandSetActive.LookAhead = FALSE;
           break;

        case ENABLE_POWER_ON_DEFAULTS:
        case DISABLE_POWER_ON_DEFAULTS:
            // g_ulRevertingPowerOnDef = TRUE;
            // g_ulRevertingPowerOnDef = FALSE;
            /* This flag influents device behavior in responding to a software reset. */
            break;

        case SET_TRANSFER_MODE:
            /* Acquires transfer mode parameter from sector count field. */
            ulXferMode = (U32)(pSlot->CFis->Count & MSK_2F);

            switch (ulXferMode >> 3)
            {
                case 0:
                    /*set PIO mode to default mode*/
                    /* Nothing is needed to do */
                    break;

                case 1:
                    /* YaoChen: PIO flow control transfer mode is specified, bit 7:3 = 00001b  2008/12/13
                         bit 2:0 speifies desired PIO transfer mode. */
                    if ((ulXferMode & 007) > 004)
                    {
                        /* Checking if target mode is supported or not -
                             PIO mode 4 is the maximum supported mode. */
                        ulCmdSptd = FALSE;
                    }

                    /* Current PIO transfer mode is not reflected in IDENTIFY DEVICE data. */
                    break;

                case 4:
                    /* YaoChen: Multiword DMA mode is specified. bit 7:3 = 00100b 2008/12/13
                         bit 2:0 speifies desired Multiword DMA transfer mode. */
                    ulXferMode &= 007;

                    if (ulXferMode > 002)    
                    {
                        /* Checking if target mode is supported or not -
                             Multiword DMA mode 2 is the maximum supported mode. */
                        ulCmdSptd = FALSE;
                    }

                    else
                    {
                        /* First clears current Multiword DMA mode field. */
                        pIdentifyData->MultiWordDMAActive = 0;

                        /* Then clears current Ultra DMA mode field. */
                        pIdentifyData->UltraDMAActive = 0;

                        /* At last sets current Multiword DMA mode field to specified value. */
                        pIdentifyData->MultiWordDMAActive = (1 << ulXferMode);
                    }

                    break;

                case 8:
                    /* YaoChen: Ultra DMA mode is specified. bit 7:3 = 01000b 2008/12/13
                         bit 2:0 speifies desired Ultra DMA transfer mode. */
                        ulXferMode &= 007;
                        
                        if (ulXferMode > 006)    
                        {
                            /* Checking if target mode is supported or not -
                                 Ultra DMA mode 6 is the maximum supported mode. */
                            ulCmdSptd = FALSE;
                        }
                        
                        else
                        {
                            /* First clears current Multiword DMA mode field. */
                            pIdentifyData->MultiWordDMAActive = 0;
                        
                            /* Then clears current Ultra DMA mode field. */
                            pIdentifyData->UltraDMAActive = 0;
                        
                            /* At last sets current Ultra DMA mode field to specified value. */
                            pIdentifyData->UltraDMAActive = (1 << ulXferMode);
                        }
                        
                        break;

                default:
                    /* Invalid transfer mode requested. */
                    ulCmdSptd = FALSE;
            }

            break;

        case ENABLE_APM:
            /* Acquires requested APM level from sector count field. */
            ulXferMode = (U32)(pSlot->CFis->Count & MSK_2F);
        
            if ((0 == ulXferMode) || (INVALID_2F == ulXferMode))
            {
                /* Requested APM level cannot have reserved values. */
                ulCmdSptd = FALSE;
            }
        
            else
            {
                /* Sets corresponding enable bit in IDENTIFY DEVICE data. */
                pIdentifyData->CommandSetActive.AdvancedPm = TRUE;
        
                /* Sets corrent APM level in IDENTIFY DEVICE data. */
                pIdentifyData->CurrentApmLevel = ulXferMode;
            }
        
            break;
        
        case DISABLE_APM:
            /* Clears corresponding enable bit in IDENTIFY DEVICE data. */
            pIdentifyData->CommandSetActive.AdvancedPm = FALSE;

            /* Restores corrent APM level in IDENTIFY DEVICE data to maximum performance value. */
            pIdentifyData->CurrentApmLevel = 0xFE;

            break;

        case ENABLE_SATA_FEATURE:
            /* Acquires specified feature from sector count field. */
            ulXferMode = (U32)(pSlot->CFis->Count & MSK_2F);

            switch (ulXferMode)
            {
                case SATA_FEATURE_AUTO_ACTIVATE:
                    //g_bSataFlagAutoActive = TRUE;
                    pIdentifyData->SataFeatureEnable.AutoActivateEn = TRUE;
                    break;
        
                case SATA_FEATURE_DIPM:
                    //g_bSataFlagDipmEnabled = TRUE;
                    pIdentifyData->SataFeatureEnable.DipmEn = TRUE;
#ifdef HOST_SATA
                    rSDC_ControlRegister |= HW_PWR_EN;   //IPM-08 enable DIPM
#endif
                    break;
        
                case SATA_FEATURE_DIAPTS:
                    /* First DIPM shall be enabled. */
                    if (TRUE == pIdentifyData->SataFeatureEnable.DipmEn)
                    {
                        pIdentifyData->SataFeatureEnable.DevAutoPartSlumberEn = TRUE;
                    }

                    else
                    {
                        ulCmdSptd = FALSE;
                    }

                    break;
        
                case SATA_FEATURE_DEVSLP:
                    /* Sets enable flag in IDENTIFY DEVICE data */
                    pIdentifyData->SataFeatureEnable.DevSleepEn = TRUE;
#ifndef SIM        
                    /* Enables internal OSC */
                    //pPMUKeyRegBlk->SleepControl.IntOSCEn = TRUE;
        
                    /* Enables DEVSLP input */
                    //pPMUKeyRegBlk->DeviceSleepMiscControl |= (1 << 31);
                    HAL_PMSetDevSleepPinEn(TRUE);
#endif

                    break;
        
                case SATA_FEATURE_SETTING_PRESERVATION:
                    pIdentifyData->SataFeatureEnable.SwSettingPreserveEn = TRUE;
                    break;
        
                default:
                    /* Invalid SATA feature specified. */
                    ulCmdSptd = FALSE;
                    break;
            }

            break;

        case DISABLE_SATA_FEATURE:
            /* Acquires specified feature from sector count field. */
            ulXferMode = (U32)(pSlot->CFis->Count & MSK_2F);

            switch (ulXferMode)
            {
                case SATA_FEATURE_AUTO_ACTIVATE:
                    //g_bSataFlagAutoActive = FALSE;
                    pIdentifyData->SataFeatureEnable.AutoActivateEn = FALSE;
                    break;
        
                case SATA_FEATURE_DIPM:
                    //g_bSataFlagDipmEnabled = FALSE;
                    pIdentifyData->SataFeatureEnable.DipmEn = FALSE;
#ifdef HOST_SATA
                    rSDC_ControlRegister &= ~HW_PWR_EN;         //IPM-08 disable DIPM
#endif
                    break;
        
                case SATA_FEATURE_DIAPTS:
                    pIdentifyData->SataFeatureEnable.DevAutoPartSlumberEn = FALSE;
                    break;
        
                case SATA_FEATURE_DEVSLP:
                    /* Clears enable flag in IDENTIFY DEVICE data */
                    pIdentifyData->SataFeatureEnable.DevSleepEn = FALSE;

#ifndef SIM        
                    /* Disables internal OSC */
                    //pPMUKeyRegBlk->SleepControl.IntOSCEn = TRUE;
        
                    /* Disables DEVSLP input */
                    HAL_PMSetDevSleepPinEn(FALSE);
                    //pPMUKeyRegBlk->DeviceSleepMiscControl |= (1 << 31);
#endif
                    break;
        
                case SATA_FEATURE_SETTING_PRESERVATION:
                    pIdentifyData->SataFeatureEnable.SwSettingPreserveEn = FALSE;
                    break;
        
                default:
                    /* Invalid SATA feature specified. */
                    ulCmdSptd = FALSE;
                    break;
            }

            break;

        default:
            /* Invalid subcommand received. */
            ulCmdSptd = FALSE;
            break;
    }

    if (FALSE == ulCmdSptd)
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        L0_AhciSendSimpleResp(pSlot);
    }

    else
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        L0_AhciSendSimpleResp(pSlot);
    }

    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcSetMultipleMode(PCB_MGR pSlot)
{
#define MAXDRQSIZEBITS_SATA 4

    U32 ulDRQSize;
    U32 ulBitShift;
    U32 ulError;

    PIDENTIFY_DEVICE_DATA pIdentifyData;

    pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    /* Acquires specified DRQ size. */
    ulDRQSize = (U32)(pSlot->CFis->Count & MSK_2F);

    if (ulDRQSize > pIdentifyData->MaximumBlockTransfer)
    {
        ulError = TRUE;
    }

    else
    {
        if (0 == ulDRQSize)
        {
            /* Host wants to disable multiple mode in PIO transfer. */
            g_bMultipleDataOpen = FALSE;
            ulError = FALSE;
            pIdentifyData->CurrentMultiSectorSetting = 0;
        }

        else
        {
            ulError = TRUE;

            for (ulBitShift = 0; ulBitShift <= MAXDRQSIZEBITS_SATA; ulBitShift++)
            {
                if ((1 << ulBitShift) == ulDRQSize)
                {
                    /* Requested DRQ size is valid. */
                    g_bMultipleDataOpen = TRUE;
                    ulError = FALSE;
                    pIdentifyData->CurrentMultiSectorSetting = (U8)ulDRQSize;
                    break;
                }
            }
        }
    }

    if (TRUE == ulError)
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        L0_AhciSendSimpleResp(pSlot);
    }

    else
    {
        l_tCurrCHSMode.ulCurrDRQSize = pIdentifyData->CurrentMultiSectorSetting;
        L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        L0_AhciSendSimpleResp(pSlot);
    }

    return TRUE;
#undef MAXDRQSIZEBITS_SATA
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcExecDevDiag(PCB_MGR pSlot)
{
    /* Assigns correct SATA protocol to command so that a signature FIS would be responded. */
    pSlot->SATAProtocol = (U8)SATA_PROT_DIAG;

    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
    L0_AhciSendSimpleResp(pSlot);

    return TRUE;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcInitDevParam(PCB_MGR pSlot)
{
    U32 ulReqHeadCnt, ulReqSecPerTrk;
    U32 ulReqCylCnt;
    U32 ulCHSTotalSecCnt, ulCHSCurrSecCnt;
    PIDENTIFY_DEVICE_DATA pIdentifyData;
    U32 ulError;

    ulReqHeadCnt = (pSlot->CFis->Device & MSK_1F) + 1;
    ulReqSecPerTrk = (pSlot->CFis->Count & MSK_2F);

    if (0 == ulReqSecPerTrk)
    {
        ulError = TRUE;
    }

    else
    {
        ulError = FALSE;
        pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

        /* According to ATA/ATAPI Specification Release 5,
             maximum accessable sector range under CHS mode is (C16383, H16, S63). */
        if ((16383 * 16 * 63) < g_ulSysLBAMax)
        {
            ulCHSTotalSecCnt = (16383 * 16 * 63);
        }

        else
        {
            ulCHSTotalSecCnt = g_ulSysLBAMax;
        }

        /* Calculating the cylinder count under requested device parameters. */
        ulReqCylCnt = ulCHSTotalSecCnt / (ulReqHeadCnt * ulReqSecPerTrk);

        /* Maximum cylinder count allowed for CHS is 65535. */
        if (65535 < ulReqCylCnt)
        {
            ulReqCylCnt = 65535;
        }

        /* Calculating the current accessable sector range. */
        ulCHSCurrSecCnt = ulReqCylCnt * ulReqHeadCnt * ulReqSecPerTrk;

        /* Sets correct parameters into IDENTIFY DEVICE information. */
        pIdentifyData->NumberOfCurrentCylinders = (U16)ulReqCylCnt;
        l_tCurrCHSMode.usNumOfHead = pIdentifyData->NumberOfCurrentHeads = (U16)ulReqHeadCnt;
        l_tCurrCHSMode.usSecPerTrk = pIdentifyData->CurrentSectorsPerTrack = (U16)ulReqSecPerTrk;
        pIdentifyData->CurrentSectorCapacityLow = (U16)ulCHSCurrSecCnt;
        pIdentifyData->CurrentSectorCapacityHigh = (U16)(ulCHSCurrSecCnt >> 16);
    }

    if (TRUE == ulError)
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        L0_AhciSendSimpleResp(pSlot);
    }

    else
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        L0_AhciSendSimpleResp(pSlot);
    }

    return TRUE;
}

U32 L0_ATACmdProcStandby(PCB_MGR pSlot)
{
    U32 ulFinished = FALSE;
    U16 ulSec = pSlot->CFis->Count;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            pSlot->SATAProtocol = (U8)SATA_PROT_NONDATA;

            /* Just mark a new event. */
            L0_EventSet(L0_EVENT_TYPE_SHUTDOWN, NULL);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SHUTDOWN_WAITING);

            break;

        case (U32)ATACMD_SHUTDOWN_WAITING:
            /* Just waiting for event finish. */
            if (L0_EVENT_STAGE_END == L0_EventGetStage(L0_EVENT_TYPE_SHUTDOWN))
            {
                /* Reporting completion for the non-data command. */
                g_ulPowerStatus = SATA_POWER_STANDBY;

                if(ATA_CMD_STANDBY == pSlot->CFis->Command)
                {
                     if((ulSec >= 0x1) || (ulSec <= 0xf0))
                        {
                            /* set 5 seconds ~ 1200 seconds */
                            g_ulStandbyTimer = ulSec * 5;
                        }
                        else if((ulSec >= 0xf1) || (ulSec <= 0xfb))
                        {
                            /* set 30 minutes ~ 330 minutes */
                            g_ulStandbyTimer = (ulSec - 240) * 30;
                        }
                        else if(ulSec == 0xfc)
                        {
                            /* set 21 minutes */
                            g_ulStandbyTimer = 21 * 60;
                        }
                        else if(ulSec == 0xfd)
                        {
                            /* set 8 hours (Between 8 hours and 12 hours in spec description) */
                            g_ulStandbyTimer = 8 * 60 * 60;
                        }
                        else if(ulSec == 0xff)
                        {
                             /* set 21 minutes 15 seconds */
                            g_ulStandbyTimer = (21 * 60) + 15;
                        }
                        else
                        {
                            /* standby timer disabled */
                            g_ulStandbyTimer = 0;
                        }
                   }

                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
                L0_AhciSendSimpleResp(pSlot);
                ulFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
    }
    
    return ulFinished;
}

U32 L0_ATACmdCheckPowerMode(PCB_MGR pSlot)
{
    RD2HFIS *pRD2HFis;

    pRD2HFis = L0_GetRespFis(pSlot);
    //pRD2HFis->Count = 0xFF;//TODO: verify "0xFF" is always OK
    pRD2HFis->Count = g_ulPowerStatus;    //return power status

    pSlot->CmdType = HCMD_TYPE_NEED_SPECIAL_STATUS;
    pSlot->SATAProtocol = (U8)SATA_PROT_NONDATA;
    
    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
    L0_AhciSendSimpleResp(pSlot);

    return TRUE;
}

BOOL L0_ATAIsDatasetMgmtCmdValid(PCFIS pCFis)
{
    U32 ulDataSecCnt = (pCFis->Count & MSK_2F);
    U32 ulTrimFlag = (pCFis->FeatureLo & 1);

    if ((FALSE == ulTrimFlag) || (0 == ulDataSecCnt)
        || (l_ulTrimMaxBlockNum < ulDataSecCnt))
    {
        return FALSE;
    }

    return TRUE;
}

U32 L0_ATACmdProcDatasetMgmt(PCB_MGR pSlot)
{
    static U32 ulFinished;
    static U32 ulDataSecCnt;
    static U32 ulTrimFlag;
    static U32 ulCurrTail;

    PSCMD pCurrSCmd;

    static U32 ulCurrProcEntry, ulMaxEntryNum;
    U32 ulProcessedEntryNum;

    static LBA_LONGENTRY tCurrSeg;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            pSlot->SATAProtocol = (U8)SATA_PROT_DMA;
            ulFinished = FALSE;
            ulDataSecCnt = (pSlot->CFis->Count & MSK_2F);
            ulCurrTail = 0;
            if (FALSE == L0_ATAIsDatasetMgmtCmdValid(pSlot->CFis))
            {
#ifndef HOST_SATA
                /* Sector count invalid for LBA range entry data. */
                L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
#else
                DBG_Printf("L0_ATACmdProcDatasetMgmt: command invalid, ERROR!\n");
                DBG_Getch();
#endif
            }

            else
            {
                /* Sets command parameters. */
                pSlot->IsWriteDir = FALSE;
                pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);                             

                /* Constructs response flow as a direct media access DMA command. */
                L0_AhciDataXferPrepRespFISSeq(pSlot);

                L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);
            }

            break;

        case (U32)ATACMD_SPLITTING:
            /* Attempts to acquire one SCMD node from either subsystem. */
            if (FALSE == L0_IsSCQFull(0))
            {
                pCurrSCmd = L0_GetNewSCmdNode(0);

                if (NULL != pCurrSCmd)
                {
                    /* Sends a RAW data transfer SCMD to L1. */
                    pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                    pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;
                    pCurrSCmd->tRawData.ulBuffAddr = g_ulATARawBuffStart;
                    pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                    pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                    pCurrSCmd->tRawData.ucSATAUsePIO = FALSE;
                
                    /* For a trim operation, we need to receive LBA range entries first, and then process them. */
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_DSM_WAITINGDATA);
                
                    /* Clears data ready flag. */
                    g_ulRawDataReadyFlag = FALSE;
                
                    L0_PushSCmdNode(0);
                }
            }

            break;

        case (U32)ATACMD_DSM_WAITINGDATA:
            if (TRUE == g_ulRawDataReadyFlag)
            {
                /* Preparing for the LBA range entry traversing. */
                L0_ATASetCmdState(pSlot, (U32)ATACMD_DSM_PROCDATA);
                ulMaxEntryNum = (SEC_SIZE / sizeof(LBA_ENTRY)) * ulDataSecCnt - 1;
                ulCurrProcEntry = 0;
            }

            break;

        case (U32)ATACMD_DSM_PROCDATA:
            /* First we must scan the LBA range entry list for each continuous LBA range. */
            ulProcessedEntryNum = L0_TrimProcLBAEntry(ulCurrProcEntry, ulMaxEntryNum, &tCurrSeg);

            if (INVALID_8F == ulProcessedEntryNum)
            {
                /* Invalid LBA range entry encountered. */
                L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
            }

            else if (0 == ulProcessedEntryNum)
            {
                /* LBA range list has been completed. */
                //L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
				L0_ATASetCmdState(pSlot, (U32)ATACMD_DSM_WAITING);
            }

            else
            {
                /* Prepares parameters for TRIM SCMDs. */
                L0_ATASetCmdState(pSlot, (U32)ATACMD_DSM_DISPATCH);

                /* Preparing to process next LBA segment. */
                ulCurrProcEntry += ulProcessedEntryNum;
				ulCurrTail = L0SCQ_Tail(0);
            }

            break;

        case (U32)ATACMD_DSM_DISPATCH:
            /* Attempts to find one free SCMD node for back-end and issue the SCMD. */
            if (SUCCESS ==
                L0_IssueUnmapSCmd(0,
                tCurrSeg.ulStartLBA,
                tCurrSeg.ulRegionLen,
                &ulCurrTail))
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_DSM_PROCDATA);
            }

            break;

        case (U32)ATACMD_DSM_WAITING:
            pCurrSCmd = &L0SCQ_Node(0, ulCurrTail);
            if ((pCurrSCmd->ucSCmdStatus == (U8)SSTS_SUCCESS) || 
                (pCurrSCmd->ucSCmdStatus == (U8)SSTS_NOT_ALLOCATED))
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
            }

            break;

        case (U32)ATACMD_ABORTED:
            /* Reporting failure for the command as a non-data command. */
            L0_AhciSendSimpleResp(pSlot);
            ulFinished = TRUE;

            break;

        case (U32)ATACMD_COMPLETED:
            if (TRUE == ulTrimFlag)
            {
                /* Reporting completion for the command manually. */
                pSlot->RespWaitData = FALSE;
                L0_AhciDataXferSendRespInfo(pSlot);
            }

            ulFinished = TRUE;

            break;

        default:
            ASSERT(FAIL);
    }

    return ulFinished;
}

U32 L0_ATACmdProcFlushCache(PCB_MGR pSlot)
{
    U32 ulFinished = FALSE;
    U32 ulSubsysIdx;
    static U32 ulSubSysIssueMap, ulSubSysCmplMap;

    switch(pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            ulSubSysIssueMap = 0;
            ulSubSysCmplMap = ((1 << g_ulSubsysNum) - 1);
            g_ulFlushCacheReady = 0;
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);
           
        case (U32) ATACMD_SPLITTING:
            for (ulSubsysIdx = 0; ulSubsysIdx < g_ulSubsysNum; ulSubsysIdx++)
            {
                if (TEST_BIT((ulSubSysCmplMap ^ ulSubSysIssueMap), (1 << ulSubsysIdx)))
                {
                    if (SUCCESS == L0_IssueFlushSCmd(ulSubsysIdx))
                    {
                        ulSubSysIssueMap |= (1 << ulSubsysIdx);
                    }
                }
            }
            
            if(ulSubSysIssueMap == ulSubSysCmplMap)
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_FLUSH_WAITING);
            }          
            
            break;

        case (U32) ATACMD_FLUSH_WAITING:
            if (((1 << g_ulSubsysNum) - 1) == g_ulFlushCacheReady)
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
            }
            break;
            
        default:
            break;
    }

    if(ATACMD_COMPLETED == pSlot->ATAState)
    {
        ulFinished = TRUE;
        L0_AhciSendSimpleResp(pSlot);        
    }
    
    return ulFinished;
}

MCU0_DRAM_TEXT U32 L0_ATACmdProcVIACmd(PCB_MGR pSlot)
{
    U32 ulFinish = FALSE;
    VIA_CMD_STATUS eStatus;
    VIA_CMD_CODE eViaCmd;
    VIA_CMD_PARAM tCmdParam;
    PRD2HFIS pTargetFIS;
    U32 aFeedback[2] = {0};

    switch(pSlot->ATAState)
    {
        case ATACMD_NEW:
            eViaCmd = (VIA_CMD_CODE)(pSlot->CFis->FeatureLo);
            if (g_VCM_Param.CurStage == CMD_STAGE)
            {
                tCmdParam.tMemAccess.ulDevAddr = (U32)&g_VCM_Param.Signature;
                tCmdParam.tMemAccess.bsByteLen = sizeof(g_VCM_Param) - 8;
                tCmdParam.aByte[8] = MCU0_ID;
            }
            else if (g_VCM_Param.CurStage == DATA_STAGE)
            {
                COM_MemCpy((U32*)&tCmdParam.aDW, (U32*)g_VCM_Param.aCmdParam, sizeof(g_VCM_Param.aCmdParam) >> 2);
                eViaCmd = g_VCM_Param.CmdCode;
            }
            else
            {
                /* new command prepare */
                eViaCmd = (VIA_CMD_CODE)(pSlot->CFis->FeatureLo);
                tCmdParam.aByte[0] = BYTE_0(pSlot->CFis->LBALo);
                tCmdParam.aByte[1] = BYTE_1(pSlot->CFis->LBALo);
                tCmdParam.aByte[2] = BYTE_2(pSlot->CFis->LBALo);
                tCmdParam.aByte[3] = BYTE_0(pSlot->CFis->Count);
                tCmdParam.aByte[4] = BYTE_0(pSlot->CFis->LBAHi);
                tCmdParam.aByte[5] = BYTE_1(pSlot->CFis->LBAHi);
                tCmdParam.aByte[6] = BYTE_2(pSlot->CFis->LBAHi);
                tCmdParam.aByte[7] = BYTE_1(pSlot->CFis->Count);
                tCmdParam.aByte[8] = pSlot->CFis->FeatureHi;
            }
            pSlot->ATAState = ATACMD_SPLITTING;
            //break;
            
        case ATACMD_SPLITTING:
            eStatus = L0_ViaHostCmd((U8)pSlot->SlotNum, eViaCmd, &tCmdParam, &aFeedback[0]);
            
            /* The following commands need to return data to host throught VIA_CMD_MEM_READ command */
            if (g_VCM_Param.CurStage == DATA_STAGE)
            {
                switch(g_VCM_Param.CmdCode)
                {
                    case VIA_CMD_VAR_TABLE:
                        g_VCM_Param.MemAccess.ulMemAddr = (U32)&g_ulVarTableAddr;
                        g_VCM_Param.MemAccess.ulByteLen = 4;
                        g_VCM_Param.CmdCode = VIA_CMD_MEM_READ;
                        eStatus = VCS_WAITING_RESOURCE;
                        break;
                    case VIA_CMD_L2_FORMAT:
                    case VIA_CMD_L3_FORMAT:
                        g_VCM_Param.Status = eStatus;
                        g_VCM_Param.MemAccess.ulMemAddr = (U32)&g_VCM_Param.Status;
                        g_VCM_Param.MemAccess.ulByteLen = 4;
                        g_VCM_Param.aCmdParam[2] = MCU0_ID;
                        g_VCM_Param.CmdCode = VIA_CMD_MEM_READ;
                        eStatus = VCS_WAITING_RESOURCE;
                        break;
                    default:
                        break;
                }
            }
            if(VCS_WAITING_RESOURCE == eStatus)
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_NEW);
                ulFinish = FALSE;//wait processing in next loop
            }
            else
            {
                /* Reset VCM parameters after VCM command is completed */
                if (g_VCM_Param.CurStage == CMD_STAGE)
                    g_VCM_Param.CurStage = 0;
                else if (g_VCM_Param.CurStage == DATA_STAGE)
                    COM_MemZero(&g_VCM_Param.CurStage, (sizeof(g_VCM_Param) >> 2) - 1);

                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                if (HCMD_TYPE_NEED_SPECIAL_STATUS == pSlot->CmdType)
                {
                    /* Constructing response information for the command. */
                    pTargetFIS = L0_GetRespFis(pSlot);
                    pTargetFIS->LBALo = (aFeedback[0] & 0xffffff);
                    pTargetFIS->LBAHi = (aFeedback[1] & 0xffffff);
                    pTargetFIS->Count = BYTE_3(aFeedback[0]) | (BYTE_3(aFeedback[1]) << 8);

                    //HCMD_TYPE_NEED_SPECIAL_STATUS command use SATA_PROT_NONDATA protocol
                    L0_AhciSendSimpleResp(pSlot);
                    ulFinish = TRUE;
                }
                else
                {
                    //SATA_PROT_DMA protocol command should come here
                    L0_AhciDataXferPrepRespFISSeq(pSlot);//prepare normal RFIS
                    pSlot->RespWaitData = FALSE;//when come here, data transfering done, no need to wait SGE
                    L0_AhciDataXferSendRespInfo(pSlot);//prepare & trigger WBQ
                    
                    ulFinish = TRUE;
                }
            }
            break;

        case ATACMD_ABORTED:
            //we will come here only when the command protocol is DMA & its param is invalid
            L0_AhciSendSimpleResp(pSlot);
            ulFinish = TRUE;
            break;

        default:
            DBG_Getch();
    }
    
    return ulFinish;
}

MCU0_DRAM_TEXT BOOL L0_ATACmdSecurityVerifyPswd(BOOL bMasterCapabilityChk,U32 ulTgtBufAddr)
{
    U16 *pTemp;
    U32 i;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    pTemp = (U16 *)ulTgtBufAddr;
   
    if (pTemp[0] & 0x1)  /*compare Master password*/
    {
        if (g_usSecurityStatus & SATA_SECURITY_MASTER_CAPABILITY_MAX) 
        {
            /*When the Master Password Capability is set to Maximum, the Master password is not used 
            with the SECURITY DISABLE PASSWORD and SECURITY UNLOCK commands*/
            if( TRUE == bMasterCapabilityChk )
                return FALSE;
        }

        for (i = 0; i < 16; i++)
        {
            if (pTemp[i+1] != pHostInfoPage->SataSecurityMasterPswd[i])
            {
                return FALSE;
            }
        }
    }
    else  /*compare User password*/
    {
        for (i = 0; i <16; i++)
        {
            if (pTemp[i+1] != pHostInfoPage->SataSecurityUserPswd[i])
            {
               return FALSE;
            }
        }
    }

    return TRUE;
}

MCU0_DRAM_TEXT void L0_SystemSecurityErase()
{
    (void)L0_IssueSecurityEraseSCmd(0, g_ulHostInfoAddr, FALSE);
    L0_WaitForAllSCmdCpl(0);

    g_ulSubSysBootOk = FALSE;
    L0_IssueBootSCmd(0, g_ulHostInfoAddr);
    L0_WaitForAllSCmdCpl(0);
    //L0_SubSystemOnlineShutdown(TRUE);
    //L0_SubSystemOnlineReboot();

    return;
}

MCU0_DRAM_TEXT BOOL L0_ATACmdSecurityErase(PCB_MGR pSlot)
{
    U16 *pTemp;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    U32 ulFinished = FALSE;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    /*receive the payload from host first, 1 sector length, to temp buffer*/
    ulDataSecCnt = 1;
    ulRAWBuffAddr = g_ulATARawBuffStart;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = FALSE;
            pSlot->PIODRQSize = ulDataSecCnt;
            pSlot->HCMDLenSec = ulDataSecCnt;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

            L0_AhciDataXferPrepRespFISSeq(pSlot);
            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                L0_PushSCmdNode(ulSubSysIdx);
                g_ulRawDataReadyFlag = FALSE;
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SECURITY_WAITINGDATA);
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        case (U32)ATACMD_SECURITY_WAITINGDATA:
            if(TRUE == g_ulRawDataReadyFlag)
            {
                g_ucSecurityErasePreFlag = FALSE;
                if (TRUE == L0_ATACmdSecurityVerifyPswd(FALSE, g_ulATARawBuffStart))
                {
                    /*the payload is already received in temp buffer in verify pswd*/
                    pTemp = (U16*)g_ulATARawBuffStart;

                    if( pTemp[0] & 0x2 )
                    {
                        DBG_Printf("Enhanced erase mode!\n");
                    }
                    else
                    {
                        DBG_Printf("Normal erase mode!\n");
                    }
                    L0_SystemSecurityErase();

                    pIdentifyData->CommandSetActive.SecurityMode = FALSE;
                    g_usSecurityStatus &= ~SATA_SECURITY_ENABLED;
                    g_usSecurityStatus &= ~SATA_SECURITY_MASTER_CAPABILITY_MAX;
                    g_usSecurityStatus &= ~SATA_SECURITY_LOCKED;
                    g_usSecurityStatus &= ~SATA_SECURITY_FROZEN;

                    pIdentifyData->SecurityStatus.SecurityEnabled = FALSE;
                    pIdentifyData->SecurityStatus.SecurityLocked = FALSE;
                    pIdentifyData->SecurityStatus.SecurityFrozen = FALSE;
                    pIdentifyData->SecurityStatus.SecurityCountExpired = FALSE;
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
                }
                else
                {
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
                }
                ulFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
    }


    return ulFinished;
}

MCU0_DRAM_TEXT BOOL L0_IsSecurityLocked(void)
{
    return((g_usSecurityStatus & SATA_SECURITY_LOCKED) ? TRUE : FALSE);
}

MCU0_DRAM_TEXT BOOL L0_ATACmdSecurityUnlock(PCB_MGR pSlot)
{
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    U32 ulFinished = FALSE;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    /*receive the payload from host first, 1 sector length, to temp buffer*/
    ulDataSecCnt = 1;
    ulRAWBuffAddr = g_ulATARawBuffStart;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = FALSE;
            pSlot->PIODRQSize = ulDataSecCnt;
            pSlot->HCMDLenSec = ulDataSecCnt;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

            L0_AhciDataXferPrepRespFISSeq(pSlot);
            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;
            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                L0_PushSCmdNode(ulSubSysIdx);
                g_ulRawDataReadyFlag = FALSE;
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SECURITY_WAITINGDATA);
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        case (U32)ATACMD_SECURITY_WAITINGDATA:
            if(TRUE == g_ulRawDataReadyFlag)
            {
                if (TRUE == L0_ATACmdSecurityVerifyPswd(TRUE,g_ulATARawBuffStart))
                {
                    g_usSecurityStatus &= (~SATA_SECURITY_LOCKED);        
                    pIdentifyData->SecurityStatus.SecurityLocked = FALSE;

                    DBG_Printf("disk unlock.\n");
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
                }

                else
                {
                    if (TRUE == L0_IsSecurityLocked())
                    { 
                        DBG_Printf("g_ucSecurityPswdVerifyCnt:%d\n",g_ucSecurityPswdVerifyCnt);
                        if (g_ucSecurityPswdVerifyCnt > 0)
                        {                
                            g_ucSecurityPswdVerifyCnt--;
                            if (0 == g_ucSecurityPswdVerifyCnt)
                            {
                                g_usSecurityStatus |= SATA_SECURITY_COUNTER_EXPIRED;
                                pIdentifyData->SecurityStatus.SecurityCountExpired = TRUE;
                            }
                        }
                    }
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
                    ulFinished = TRUE;
                }
            }

            break;

        default:
            ASSERT(FAIL);
    }

    return ulFinished;
}

MCU0_DRAM_TEXT BOOL L0_ATACmdSecurityDisablePassword(PCB_MGR pSlot)
{
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    U32 ulFinished = FALSE;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    /*receive the payload from host first, 1 sector length, to temp buffer*/
    ulDataSecCnt = 1;
    ulRAWBuffAddr = g_ulATARawBuffStart;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = FALSE;
            pSlot->PIODRQSize = ulDataSecCnt;
            pSlot->HCMDLenSec = ulDataSecCnt;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

            L0_AhciDataXferPrepRespFISSeq(pSlot);
            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                g_ulRawDataReadyFlag = FALSE;
                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SECURITY_WAITINGDATA);
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        case (U32)ATACMD_SECURITY_WAITINGDATA:
            if(TRUE == g_ulRawDataReadyFlag)
            {
                if (FALSE == L0_ATACmdSecurityVerifyPswd(TRUE, g_ulATARawBuffStart) )   
                {
                    L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
                }

                else
                {
                    /*disable security*/
                    pIdentifyData->CommandSetActive.SecurityMode = FALSE;
                    g_usSecurityStatus &= ~SATA_SECURITY_ENABLED;
                    g_usSecurityStatus &= ~SATA_SECURITY_MASTER_CAPABILITY_MAX;    
                    pIdentifyData->SecurityStatus.SecurityEnabled = FALSE;
                    pIdentifyData->SecurityStatus.SecurityLevel = FALSE;
                    DBG_Printf("disk unlock.\n");

                    L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
                }

                ulFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
    }

    return ulFinished;
}

MCU0_DRAM_TEXT BOOL L0_ATACmdSecuritySetPassword(PCB_MGR pSlot)
{
    U32 i;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    U32 ulFinished = FALSE;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    U16 *pSetPswd = (U16*)g_ulATARawBuffStart;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    /*receive the payload from host first, 1 sector length, to temp buffer*/
    ulDataSecCnt = 1;
    ulRAWBuffAddr = g_ulATARawBuffStart;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = FALSE;
            pSlot->PIODRQSize = ulDataSecCnt;
            pSlot->HCMDLenSec = ulDataSecCnt;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

            L0_AhciDataXferPrepRespFISSeq(pSlot);
            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;
                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                g_ulRawDataReadyFlag = FALSE;
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SECURITY_WAITINGDATA);
                L0_PushSCmdNode(ulSubSysIdx);
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        case (U32)ATACMD_SECURITY_WAITINGDATA:
            if(TRUE == g_ulRawDataReadyFlag)
            {
                /*parsing the payload*/
                if (pSetPswd[0] & 0x1)
                {
                    DBG_Printf("set master password\n");
                    /*set master password*/
                    for (i = 0; i < 16; i++)
                    {
                        pHostInfoPage->SataSecurityMasterPswd[i] = pSetPswd[1+i];
                    }

                    if (pSetPswd[17] != 0x0000 && pSetPswd[17] != 0xFFFF)
                    {
                        /*update master password identifier in identify data and log page*/
                        pIdentifyData->MasterPasswordID = pSetPswd[17];
                        pHostInfoPage->SataSecurityMasterPswdIdentifier = pSetPswd[17];
                    }
                }

                else
                {
                    /*set user password*/
                    for (i = 0; i < 16; i++)
                    {
                        pHostInfoPage->SataSecurityUserPswd[i] = pSetPswd[1+i];
                    }

                    if (pSetPswd[0] & SATA_SECURITY_MASTER_CAPABILITY_MAX)
                    {
                        DBG_Printf("set user password:maximum\n");
                        /*master password capability maximum*/
                        g_usSecurityStatus |= SATA_SECURITY_MASTER_CAPABILITY_MAX;
                        pIdentifyData->SecurityStatus.SecurityLevel = TRUE;
                    }

                    else
                    {
                        DBG_Printf("set user password:high\n");
                        /*master password capability high*/
                        g_usSecurityStatus &= (~SATA_SECURITY_MASTER_CAPABILITY_MAX);
                        pIdentifyData->SecurityStatus.SecurityLevel = FALSE;
                    }

                    /*enable security after user password set successful*/
                    g_usSecurityStatus |= SATA_SECURITY_ENABLED;
                    pIdentifyData->CommandSetActive.SecurityMode = TRUE;
                    pIdentifyData->SecurityStatus.SecurityEnabled = TRUE;
                }

                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
                ulFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
    }
    return ulFinished;
}

MCU0_DRAM_TEXT BOOL L0_ATACmdProcSecurity(PCB_MGR pSlot)
{
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    PHOST_INFO_PAGE pHostInfoPage       = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    U32 ulSubSysIdx;
    U32 ulFinished = FALSE;

    switch(pSlot->CFis->Command)
    {
    case ATA_CMD_SECURITY_SET_PASSWORD:
        /* If the device is in the Locked or Frozen modes, then the device shall return command aborted */
        if ((g_usSecurityStatus & SATA_SECURITY_FROZEN) || (g_usSecurityStatus & SATA_SECURITY_LOCKED))
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            ulFinished = L0_ATACmdSecuritySetPassword(pSlot);
        }
        break;

    case ATA_CMD_SECURITY_ERASE_PREPARE:
        if (g_usSecurityStatus & SATA_SECURITY_FROZEN)
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            /*report success status*/
            g_ucSecurityErasePreFlag = TRUE;
            L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        }
        break;

    case ATA_CMD_SECURITY_ERASE_UNIT:
        if ((g_usSecurityStatus & SATA_SECURITY_FROZEN) ||
            (g_usSecurityStatus & SATA_SECURITY_COUNTER_EXPIRED) ||
            (TRUE != g_ucSecurityErasePreFlag))
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            ulFinished = L0_ATACmdSecurityErase(pSlot);
        }
        break;

    case ATA_CMD_SECURITY_FREEZE_LOCK:
        DBG_Printf("ATA_CMD_SECURITY_FREEZE_LOCK\n");            
        if (g_usSecurityStatus & SATA_SECURITY_LOCKED)
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            g_usSecurityStatus |= SATA_SECURITY_FROZEN;
            /*update identify data, freeze information does not need to  save back for next power up*/
            pIdentifyData->SecurityStatus.AsU16 |= g_usSecurityStatus;
            L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        }
        break;

    case ATA_CMD_SECURITY_DISABLE_PASSWORD:
        if ((g_usSecurityStatus & SATA_SECURITY_FROZEN) || (g_usSecurityStatus & SATA_SECURITY_LOCKED))
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else if(FALSE == (g_usSecurityStatus & SATA_SECURITY_ENABLED))
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            ulFinished = L0_ATACmdSecurityDisablePassword(pSlot);
        }
        break;

    case ATA_CMD_SECURITY_UNLOCK:
        if ((g_usSecurityStatus & SATA_SECURITY_FROZEN) ||
            (g_usSecurityStatus & SATA_SECURITY_COUNTER_EXPIRED))
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else if(!(g_usSecurityStatus & SATA_SECURITY_ENABLED))
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            ulFinished = L0_ATACmdSecurityUnlock(pSlot);
        }
        break;

    default:
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        break;
    }

    if(((U32)ATACMD_COMPLETED == pSlot->ATAState)
        || ((U32)ATACMD_ABORTED == pSlot->ATAState))
    {
        if((U32)ATACMD_COMPLETED == pSlot->ATAState)
        {
            /* Frozen mode shall be disabled by power-off or hardware reset, don't need save frozen status */
            if (ATA_CMD_SECURITY_FREEZE_LOCK != pSlot->CFis->Command)
            {
                /*waiting for save security status done,then send D2H FIS*/
                pHostInfoPage->SataSecurityStatus = g_usSecurityStatus;
                pHostInfoPage->SataSecurityStatus &= ~(SATA_SECURITY_FROZEN);

                for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
                {
                    L0_IssueAccessHostInfoSCmd(ulSubSysIdx, (U32)g_ulHostInfoAddr, (U32)GLBINFO_SAVE);
                    L0_WaitForAllSCmdCpl(ulSubSysIdx);
                }
            }
        }

        if (SATA_PROT_NONDATA == pSlot->SATAProtocol)
        {
            L0_AhciSendSimpleResp(pSlot);
        }

        else
        {
            L0_AhciDataXferSendRespInfo(pSlot);
        }

        ulFinished = TRUE;
    }

    return ulFinished;
}


MCU0_DRAM_TEXT void L0_ATAInitHPA(void)
{
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    if (TRUE == g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0HPAEnable)
    {
        pIdentifyData->CommandSetSupport.HostProtectedArea = TRUE;
        pIdentifyData->CommandSetActive.HostProtectedArea = TRUE;
    }

    g_ucReadMaxFlag = FALSE;
    g_ucHpaEstablishedFlag = FALSE;

    return;
}

MCU0_DRAM_TEXT void L0_ATAInitPowerStatus(void)
{
    g_ulPowerStatus = SATA_POWER_ACTIVEORIDLE;
    g_bStartTimer = TRUE;
    g_ulStandbyTimer = 0;
}

MCU0_DRAM_TEXT U32 L0_HPAGetNativeMaxAddr()
{    
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    return (pHostInfoPage->HPAMaxLBA);
}

MCU0_DRAM_TEXT BOOL L0_HPASetNativeMaxAddr(BOOL bVolatileValue,U32 ulNativeMaxAddr)
{
    U32 ulSubSysIdx;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    if (ulNativeMaxAddr > L0_HPAGetNativeMaxAddr()) 
    {
        return FALSE;
    }

    DBG_Printf("MaxAccessLBA:0x%x,SetNativeMaxAddr:0x%x\n",L0_HPAGetNativeMaxAddr(),ulNativeMaxAddr);

    /* update Identify Device Data */
    if( ulNativeMaxAddr >= 0x10000000 )
    {
        pIdentifyData->UserAddressableSectors = 0x0FFFFFFF;
    }
    else
    {
        pIdentifyData->UserAddressableSectors = ulNativeMaxAddr + 1;
    }

    /* set WORDS[100..103] */
    pIdentifyData->Max48BitLBA[0] = ulNativeMaxAddr + 1;
    pIdentifyData->Max48BitLBA[1] = 0x0;

    g_pBootParamTable->ulSysMaxLBACnt = ulNativeMaxAddr;

    /* if V_V flag is not set, then the SET MAX ADDRESS value shall be saved for next power up */
    /* MHDD tool,0:tempory 1:persist, but ATA Spec 0:save,1:not save */
    g_ulUsedLBAMax=ulNativeMaxAddr;
    if(TRUE == bVolatileValue )
    {
        pHostInfoPage->HPAMaxLBA = ulNativeMaxAddr;
        for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
        {
            L0_IssueAccessHostInfoSCmd(ulSubSysIdx,(U32)g_ulHostInfoAddr,(U32)GLBINFO_SAVE);
            L0_WaitForAllSCmdCpl(ulSubSysIdx);
        }
    }  

    return TRUE;
}

MCU0_DRAM_TEXT BOOL L0_ATACmdProcHPA(PCB_MGR pSlot)
{
    U32 ulNativeMaxAddr;
    BOOL bVolatileValue;
    RD2HFIS* pRfis;

    switch (pSlot->CFis->Command)
    {
    case ATA_CMD_READ_NATIVE_MAX_ADDRESS:
        ulNativeMaxAddr = L0_HPAGetNativeMaxAddr();
        /*in 28 bits mode, if max address is out of range, then return 0xFFFFFFF*/
        if (ulNativeMaxAddr > 0xFFFFFFF) 
        {
            ulNativeMaxAddr = 0xFFFFFFF;
        }

        // get the pointer to D2H FIS, which contains
        // information to be reported to the host
        pRfis = L0_GetRespFis(pSlot);

        // fill the RFIS with the max LBA
        pRfis->LBALo = (ulNativeMaxAddr & 0xFFFFFF);
        pRfis->LBAHi = ((ulNativeMaxAddr >> 24) & 0xF);

        // set the command type to HCMD_TYPE_NEED_SPECIAL_STATUS,
        // indicating we need to report status the host
        pSlot->CmdType = HCMD_TYPE_NEED_SPECIAL_STATUS;

        g_ucReadMaxFlag = TRUE;
        L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        break;
    case ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT:
        ulNativeMaxAddr = L0_HPAGetNativeMaxAddr();

        // get the pointer to D2H FIS, which contains
        // information to be reported to the host
        pRfis = L0_GetRespFis(pSlot);

        // fill the RFIS with the max LBA
        pRfis->LBALo = (ulNativeMaxAddr & 0xFFFFFF);
        pRfis->LBAHi = ((ulNativeMaxAddr >> 24) & 0xFF);

        // set the command type to HCMD_TYPE_NEED_SPECIAL_STATUS,
        // indicating we need to report status the host
        pSlot->CmdType = HCMD_TYPE_NEED_SPECIAL_STATUS;

        g_ucReadMaxFlag = TRUE;
        L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
        break;

    case ATA_CMD_SET_MAX_ADDRESS:
    case ATA_CMD_SET_MAX_ADDRESS_EXT:
        if (FALSE == g_ucReadMaxFlag || TRUE == L0_IsSecurityLocked())
        {
            /*if not executed a previous success read max command, there
            are two options: 
            1. the set max command shall directly aborted
            2. process one of the following commands, SET MAX SET PASSWORD,
            SET MAX LOCK, SET MAX UNLOCK, SET MAX FREEZE LOCK, that 
            depends on the feature code
            As we do not support HPA Security extensions now, we chose
            directly abort the command*/
            L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        }
        else
        {
            L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

            if (ATA_CMD_SET_MAX_ADDRESS == pSlot->CFis->Command)
            {
                ulNativeMaxAddr = pSlot->CFis->LBALo + ((pSlot->CFis->Device & 0xF) << 24);
            }
            else if (ATA_CMD_SET_MAX_ADDRESS_EXT == pSlot->CFis->Command)
            {
                ulNativeMaxAddr = pSlot->CFis->LBALo + (pSlot->CFis->LBAHi << 24);
            }
            
            if(TRUE== g_ucHpaEstablishedFlag && (ulNativeMaxAddr <= L0_HPAGetNativeMaxAddr()))
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
            }
            
            bVolatileValue = pSlot->CFis->Count & 0x1;
            if (FALSE == L0_HPASetNativeMaxAddr(bVolatileValue,ulNativeMaxAddr))
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
            }

            if((ulNativeMaxAddr < L0_HPAGetNativeMaxAddr()) &&  (TRUE == bVolatileValue))
            {
                g_ucReadMaxFlag = FALSE;
                g_ucHpaEstablishedFlag = TRUE;
            }

        }
        break;
    default:
        ASSERT(FAIL);
        break;
    }

    L0_AhciSendSimpleResp(pSlot);
    return TRUE;
}

BOOL L0_ATACmdProcReadBuffer(PCB_MGR pSlot)
{
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    U32 ulRAWBuffAddr;

    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    U32 ulFinished = FALSE;

    ulRAWBuffAddr = g_ulATARawBuffStart;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = TRUE;
            pSlot->PIODRQSize = 1;
            pSlot->HCMDLenSec = 1;
            pSlot->Ch->PRDBC = SEC_SIZE;
            pSlot->CFis->Count = 1;

            /* Utilizes response flow as a direct media access PIO command. */
            L0_AhciDataXferPrepRespFISSeq(pSlot);

            /* Starts response flow as a direct media access PIO command. */
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);
            break;
        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;

            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;
                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = 1;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_D2H;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                L0_PushSCmdNode(ulSubSysIdx);
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                ulFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
            break;
    }

    return ulFinished;
}

BOOL L0_ATACmdProcWriteBuffer(PCB_MGR pSlot)
{
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr;
    U32 ulFinished = FALSE;
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

    /*receive the payload from host first, 1 sector length, to temp buffer*/
    ulDataSecCnt = 1;
    ulRAWBuffAddr = g_ulATARawBuffStart;

    switch (pSlot->ATAState)
    {
        case (U32)ATACMD_NEW:
            /* Sets command parameters. */
            pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            pSlot->IsWriteDir = FALSE;
            pSlot->PIODRQSize = ulDataSecCnt;
            pSlot->HCMDLenSec = ulDataSecCnt;
            pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

            L0_AhciDataXferPrepRespFISSeq(pSlot);
            L0_AhciDataXferSendRespInfo(pSlot);

            /* Changes command execute state. */
            L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            pCurrSCmd = NULL;
            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                /* Attempts to acquire one SCMD node from either subsystem. */
                if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                {
                    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                    break;
                }
            }

            if (NULL != pCurrSCmd)
            {
                /* Sends a RAW data transfer SCMD to L1. */
                pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                pCurrSCmd->tRawData.ucSecLen   = ulDataSecCnt;
                pCurrSCmd->tRawData.ucDataDir  = (U8)RAWDRQ_H2D;
                pCurrSCmd->tRawData.ucSATAUsePIO = TRUE;

                L0_PushSCmdNode(ulSubSysIdx);
                g_ulRawDataReadyFlag = FALSE;
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SECURITY_WAITINGDATA);
            }

            else
            {
                ulFinished = FALSE;
            }

            break;

        case (U32)ATACMD_SECURITY_WAITINGDATA:
            if(TRUE == g_ulRawDataReadyFlag)
            {
                L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);
                ulFinished = TRUE;
            }

            break;

        default:
        ASSERT(FAIL);
    }

    return ulFinished;
}

#ifdef HOST_SATA
BOOL L0_ATAIsMicroCodeCmdValid(PCFIS pCFis)
{
    U8 ucCmdFeature;
    U32 ulDataSecCnt;
    U32 ulOffset;
    
    ucCmdFeature = pCFis->FeatureLo & MSK_2F;
    ulDataSecCnt = ((pCFis->LBALo & MSK_2F) << 8) + (pCFis->Count & MSK_2F);
    ulOffset = ((pCFis->LBALo >> 8) & MSK_4F) << SEC_SIZE_BITS;

    /*if fis param. not correct, return false*/ 
    if ((0x3 != ucCmdFeature) && (NULL != ulOffset))
    {
        //DBG_Printf("Download params error!\n");
        return FALSE;
    }

    /*If the current offset is not equal to the sum of the previous DOWNLOAD MICROCODE 
    command offset and the previous block count, return false*/
    if (ulOffset != (g_tFwUpdate.ulPreOffset + (g_tFwUpdate.ulPreDataSecCnt << SEC_SIZE_BITS)))
    {
        //DBG_Printf("Download offset error!\n");
        return FALSE;
    }

    if ((ulOffset + (ulDataSecCnt << SEC_SIZE_BITS)) > FW_IMAGE_MEM_SIZE)
    {
        //DBG_Printf("Download buffer overflow error!\n");
        return FALSE;
    }

    return TRUE;
}
#endif

#ifdef HOST_SATA
BOOL L0_ATACmdProcMicroCode(PCB_MGR pSlot)
{
    PSCMD pCurrSCmd;
    U32 ulSubSysIdx; 
    U32 ulOffset;
    U32 ulDataSecCnt;
    U32 ulRAWBuffAddr; 
    U32 ulFinished = FALSE;
    BOOL  bOverLap = FALSE;
    U32 ulCmdAbort = FALSE;
    U8 ucCmdFeature;
    
    ucCmdFeature = pSlot->CFis->FeatureLo & MSK_2F;
    ulDataSecCnt = ((pSlot->CFis->LBALo & MSK_2F) << 8) + (pSlot->CFis->Count & MSK_2F);
    ulOffset = ((pSlot->CFis->LBALo >> 8) & MSK_4F) << SEC_SIZE_BITS;
    ulRAWBuffAddr = g_ulFWUpdateImageAddr + ulOffset;

    
    if (FALSE == L0_ATAIsMicroCodeCmdValid(pSlot->CFis))
    {
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        DBG_Printf("L0_ATACmdProcMicroCode: command invalid, abort it!\n");
    }
    else
    {
        switch (pSlot->ATAState)
        {
            case (U32)ATACMD_NEW:
                pSlot->SATAProtocol = (U8)SATA_PROT_DMA;
                pSlot->IsWriteDir = FALSE;
                pSlot->PIODRQSize = ulDataSecCnt;
                pSlot->HCMDLenSec = ulDataSecCnt;
                pSlot->Ch->PRDBC = (ulDataSecCnt << SEC_SIZE_BITS);

                L0_AhciDataXferPrepRespFISSeq(pSlot);
      
                L0_ATASetCmdState(pSlot, (U32)ATACMD_SPLITTING);

            case (U32)ATACMD_SPLITTING:
                pCurrSCmd = NULL;
            
                for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
                {
                    /*Attempts to acquire one SCMD node from either subsystem. */
                    if (FALSE == L0_IsSCQFull(ulSubSysIdx))
                    {
                        pCurrSCmd = L0_GetNewSCmdNode(ulSubSysIdx);
                        break;
                    }
                }

                if (NULL != pCurrSCmd)
                {
                    /*Sends a RAW data transfer SCMD to L1. */
                    pCurrSCmd->ucSCmdType = (U8)SCMD_RAW_DATA_REQ;
                    pCurrSCmd->ucSlotNum = (U8)pSlot->SlotNum;

                    pCurrSCmd->tRawData.ulBuffAddr = ulRAWBuffAddr;
                    pCurrSCmd->tRawData.ucSecLen = ulDataSecCnt;
                    pCurrSCmd->tRawData.ucDataDir = (U8)RAWDRQ_H2D;
                    pCurrSCmd->tRawData.ucSATAUsePIO = FALSE;
 
                    if (ulOffset == 0)
                    {
                        g_bFwUpdateOngoing = TRUE;
                        COM_MemSet((U32*)g_tFwUpdate.ulFwBaseAddr, g_tFwUpdate.ulFwSize >> 2, 0x00);
                        g_tFwUpdate.ulDldSize = 0;
                        g_tFwUpdate.bUpdateFwSize = FALSE;
                    }
                    
                    g_tFwUpdate.ulDldSize += (ulDataSecCnt << SEC_SIZE_BITS);
                    if ((g_tFwUpdate.ulDldSize > g_tFwUpdate.ulFwSize)||((ulOffset + (ulDataSecCnt << SEC_SIZE_BITS)) > g_tFwUpdate.ulFwSize))
                    {
                        DBG_Printf("Download overlap error!\n");
                        bOverLap = TRUE;
                    }

                    if(TRUE == bOverLap)
                    {
                        ulCmdAbort = TRUE;
                    }
                    else
                    {
                        DBG_Printf("admin cmd: download image (0x%x ~ 0x%x)\n", ulOffset, ulOffset + (ulDataSecCnt << SEC_SIZE_BITS));
                        
                        /* record the previous microcode command offset and DataSecCnt*/
                        g_tFwUpdate.ulPreOffset = ulOffset;
                        g_tFwUpdate.ulPreDataSecCnt = ulDataSecCnt;
                        L0_PushSCmdNode(ulSubSysIdx);
                        L0_ATASetCmdState(pSlot, (U32)ATACMD_COMPLETED);

                        if ((FALSE == g_tFwUpdate.bUpdateFwSize ) && (g_tFwUpdate.ulDldSize >= 16384))
                        {
                            L0_WaitForAllSCmdCpl(ulSubSysIdx);
                            L0_FwUpgradeCalcImgLen();
                            if (g_tFwUpdate.ulFwSize > FW_IMAGE_MEM_SIZE)
                            {
                                ulCmdAbort = TRUE;
                            }
                            g_tFwUpdate.bUpdateFwSize = TRUE;
                        }
                        ulFinished = TRUE;          
                    }  
                }
                else
                {
                    ulFinished = FALSE;
                }
                
                break;
            default:
                ASSERT(FAIL);
                break;
        }    
    }

    //If the last MicroCode cmd coming, do the FW commit
    if((ulOffset + (ulDataSecCnt << SEC_SIZE_BITS)) == g_tFwUpdate.ulFwSize)
    {
        for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
        {
            L0_WaitForAllSCmdCpl(ulSubSysIdx);
        }
    
        if (FALSE == L0_SATAFwCommit())
        {
           DBG_Printf("FW commit fail\n");
           ulCmdAbort = TRUE;
        }
        else
        {
            DBG_Printf("FW update done!!\n");
            g_tFwUpdate.ulPreDataSecCnt = 0;
            g_tFwUpdate.ulPreOffset = 0;
            g_bFwUpdateOngoing = FALSE;
        }
    }
    else if((ulOffset + (ulDataSecCnt << SEC_SIZE_BITS)) > g_tFwUpdate.ulFwSize)
    {
        ulCmdAbort = TRUE;
    }
    

    if(TRUE == ulCmdAbort)
    {
        DBG_Printf("MicroCode cmd abort\n");
        L0_ATASetCmdState(pSlot, (U32)ATACMD_ABORTED);
        L0_AhciSendSimpleResp(pSlot);
        L0_FwUpdateInit();
        ulFinished = TRUE;
    }  
    
    return ulFinished;
}
#endif

void L0_ATASetCmdParam(PCB_MGR pSlot)
{
    U32 ulCmdCode;
    VIA_CMD_CODE eViaCmd;
    //VIA_CMD_PARAM tCmdParam;

    //TRACE_LOG((void*)pSlot->CFis, sizeof(CFIS), CFIS, 0, "CFIS: ");

    /* 1. Checking the command is an ATA command or a Software Reset request. */
    if (FALSE == pSlot->CFis->C)
    {
        pSlot->CmdType = (U8)HCMD_TYPE_SOFT_RESET;
    }

    else
    {
        ulCmdCode = pSlot->CFis->Command;

        if (g_VCM_Param.EnterVCM == 1)
        {
            L0_VCMGetCurStage(pSlot, ulCmdCode);
            if (g_VCM_Param.CurStage == CMD_STAGE)
            {
                pSlot->CFis->FeatureLo = VIA_CMD_MEM_WRITE;
                pSlot->CFis->Command = ATA_CMD_VENDER_DEFINE;
                ulCmdCode = ATA_CMD_VENDER_DEFINE;
            }
            else if (g_VCM_Param.CurStage == DATA_STAGE)
            {
                pSlot->CFis->FeatureLo = g_VCM_Param.CmdCode;
                pSlot->CFis->Command = ATA_CMD_VENDER_DEFINE;
                ulCmdCode = ATA_CMD_VENDER_DEFINE;
            }
        }

        /* 2. Setting command parameters according to its Register H2D FIS. */
        switch (ulCmdCode)
        {
            /* TODO: SATA 3.2 support command 0x63/0x64/0x65 in NCQ protocol */
            case ATA_CMD_READ_FPDMA_QUEUED:
            case ATA_CMD_WRITE_FPDMA_QUEUED:
                if (ATA_CMD_READ_FPDMA_QUEUED == ulCmdCode)
                {
                    pSlot->IsWriteDir = TRUE;
                }

                else
                {
                    pSlot->IsWriteDir = FALSE;
                }

                pSlot->CmdType = (U8)HCMD_TYPE_DIRECT_MEDIA_ACCESS;
                pSlot->SATAProtocol = (U8)SATA_PROT_FPDMA;

                L0_ATASetXferParamNCQ(pSlot);

                break;

            case ATA_CMD_READ_DMA_EXT:
            case ATA_CMD_WRITE_DMA_EXT:
                if (ATA_CMD_READ_DMA_EXT == ulCmdCode)
                {
                    pSlot->IsWriteDir = TRUE;
                }

                else
                {
                    pSlot->IsWriteDir = FALSE;
                }

                pSlot->CmdType = (U8)HCMD_TYPE_DIRECT_MEDIA_ACCESS;
                pSlot->SATAProtocol = (U8)SATA_PROT_DMA;

                L0_ATASetXferParam48(pSlot);

                break;

            case ATA_CMD_READ_DMA:
            case ATA_CMD_WRITE_DMA:
                if (ATA_CMD_READ_DMA == ulCmdCode)
                {
                    pSlot->IsWriteDir = TRUE;
                }

                else
                {
                    pSlot->IsWriteDir = FALSE;
                }

                pSlot->CmdType = (U8)HCMD_TYPE_DIRECT_MEDIA_ACCESS;
                pSlot->SATAProtocol = (U8)SATA_PROT_DMA;

                L0_ATASetXferParam28(pSlot);

                break;

            case ATA_CMD_READ_SECTOR:
            case ATA_CMD_READ_MULTIPLE:
            case ATA_CMD_WRITE_SECTOR:
            case ATA_CMD_WRITE_MULTIPLE:
                if ((ATA_CMD_READ_SECTOR == ulCmdCode)
                    || (ATA_CMD_READ_MULTIPLE == ulCmdCode))
                {
                    pSlot->IsWriteDir = TRUE;
                }

                else
                {
                    pSlot->IsWriteDir = FALSE;
                }

                pSlot->CmdType = (U8)HCMD_TYPE_DIRECT_MEDIA_ACCESS;
                pSlot->SATAProtocol = (U8)SATA_PROT_PIO;

                if ((ATA_CMD_READ_SECTOR == ulCmdCode)
                    || (ATA_CMD_WRITE_SECTOR == ulCmdCode))
                {
                    pSlot->PIODRQSize = 1;
                }

                else
                {
                    pSlot->PIODRQSize  = l_tCurrCHSMode.ulCurrDRQSize;
                }

                L0_ATASetXferParam28(pSlot);

                break;

            case ATA_CMD_READ_SECTOR_EXT:
            case ATA_CMD_READ_MULTIPLE_EXT:
            case ATA_CMD_WRITE_SECTOR_EXT:
            case ATA_CMD_WRITE_MULTIPLE_EXT:
                if ((ATA_CMD_READ_SECTOR_EXT == ulCmdCode)
                    || (ATA_CMD_READ_MULTIPLE_EXT == ulCmdCode))
                {
                    pSlot->IsWriteDir = TRUE;
                }
            
                else
                {
                    pSlot->IsWriteDir = FALSE;
                }
            
                pSlot->CmdType = (U8)HCMD_TYPE_DIRECT_MEDIA_ACCESS;
                pSlot->SATAProtocol = (U8)SATA_PROT_PIO;
            
                if ((ATA_CMD_READ_SECTOR_EXT == ulCmdCode)
                    || (ATA_CMD_WRITE_SECTOR_EXT == ulCmdCode))
                {
                    pSlot->PIODRQSize = 1;
                }
                
                else
                {
                    pSlot->PIODRQSize  = l_tCurrCHSMode.ulCurrDRQSize;
                }

                L0_ATASetXferParam48(pSlot);
            
                break;

            case ATA_CMD_DOWNLOAD_MICROCODE_DMA:
            case ATA_CMD_DATA_SET_MANAGEMENT:
                pSlot->CmdType = (U8)HCMD_TYPE_OTHERS;
                pSlot->SATAProtocol = (U8)SATA_PROT_DMA;
                break;
            
            case ATA_CMD_VENDER_DEFINE:
                if ((g_VCM_Param.CurStage == CMD_STAGE) || (g_VCM_Param.CurStage == DATA_STAGE))
                {
                    pSlot->CmdType = HCMD_TYPE_OTHERS;
                    pSlot->SATAProtocol = (U8)SATA_PROT_DMA;
                }
                else
                {
                    eViaCmd = pSlot->CFis->FeatureLo;
                    switch (eViaCmd)
                    {
                        case VIA_CMD_MEM_READ:
                        case VIA_CMD_MEM_WRITE:
                        case VIA_CMD_FLASH_READ:
                        case VIA_CMD_FLASH_WRITE:
                        case VIA_CMD_FLASH_ERASE:
                            pSlot->CmdType = HCMD_TYPE_OTHERS;
                            pSlot->SATAProtocol = (U8)SATA_PROT_DMA;

#if 0
                            //for DMA command which is to be aborted, we should abort it
                            //soon after receiving, so we check parammeters here
                            tCmdParam.aByte[0] = BYTE_0(pSlot->CFis->LBALo);
                            tCmdParam.aByte[1] = BYTE_1(pSlot->CFis->LBALo);
                            tCmdParam.aByte[2] = BYTE_2(pSlot->CFis->LBALo);
                            tCmdParam.aByte[3] = BYTE_0(pSlot->CFis->Count);
                            tCmdParam.aByte[4] = BYTE_0(pSlot->CFis->LBAHi);
                            tCmdParam.aByte[5] = BYTE_1(pSlot->CFis->LBAHi);
                            tCmdParam.aByte[6] = BYTE_2(pSlot->CFis->LBAHi);
                            tCmdParam.aByte[7] = BYTE_1(pSlot->CFis->Count);
                            tCmdParam.aByte[8] = pSlot->CFis->FeatureHi;

                            if (VCS_SUCCESS != L0_ViaCmdCheckParam(eViaCmd, &tCmdParam))
                            {
                                L0_ATASetCmdState(pSlot, ATACMD_ABORTED);
                            }
#endif
                            break;

                        default:
                            pSlot->CmdType = HCMD_TYPE_NEED_SPECIAL_STATUS;
                            pSlot->SATAProtocol = (U8)SATA_PROT_NONDATA;
                            break;
                    }//end switch (eViaCmd)
                }
                break;

            default:
                pSlot->CmdType = (U8)HCMD_TYPE_OTHERS;

                /* The accurate protocol type shall be assigned in the stage of command processing. */
                pSlot->SATAProtocol = (U8)SATA_PROT_NONDATA;

                break;
        }

        /* Sets WBQ execution to waiting SGE data transfer completion by default. */
        pSlot->RespWaitData = TRUE;
    }

    return;
}

void L0_ATASetXferParamNCQ(PCB_MGR pSlot)
{
    U32 ulSectorCount;

    ulSectorCount = (pSlot->CFis->FeatureHi << 8) + pSlot->CFis->FeatureLo;

    if (0 == ulSectorCount)
    {
        ulSectorCount = (1 << 16);
    }

    pSlot->HCMDLenSec = ulSectorCount;
    pSlot->TotalRemainingBytes = (ulSectorCount << SEC_SIZE_BITS);
    pSlot->CurrentLBA = (pSlot->CFis->LBAHi << 24) + pSlot->CFis->LBALo;

    return;
}

void L0_ATASetXferParam48(PCB_MGR pSlot)
{
    U32 ulSectorCount;

    ulSectorCount = pSlot->CFis->Count;

    if (0 == ulSectorCount)
    {
        ulSectorCount = (1 << 16);
    }

    pSlot->HCMDLenSec = ulSectorCount;
    pSlot->TotalRemainingBytes = (ulSectorCount << SEC_SIZE_BITS);
    pSlot->CurrentLBA = (U32)(pSlot->CFis->LBAHi << 24) + pSlot->CFis->LBALo;
    pSlot->Ch->PRDBC = pSlot->TotalRemainingBytes;

    return;
}

void L0_ATASetXferParam28(PCB_MGR pSlot)
{
    U32 ulStartLBA;
    U32 ulSectorCount;

    U32 ulCylinder, ulHead, ulSectorNum;
    //PIDENTIFY_DEVICE_DATA pIdentifyData;

    if (TEST_BIT(pSlot->CFis->Device, DEVHEAD_LBA_BIT))
    {
        /* LBA mode. */
        ulStartLBA = ((pSlot->CFis->Device & MSK_1F) << 24) + pSlot->CFis->LBALo;
    }

    else
    {
        /* Obsolete CHS mode. */
        ulCylinder = (pSlot->CFis->LBALo >> 8);
        ulHead = (pSlot->CFis->Device & MSK_1F);
        ulSectorNum = (pSlot->CFis->LBALo & MSK_2F);

        //pIdentifyData = (PIDENTIFY_DEVICE_DATA)g_ulATAInfoIdfyPage;

        //ulStartLBA = (ulCylinder * pIdentifyData->NumberOfCurrentHeads + ulHead) * pIdentifyData->CurrentSectorsPerTrack
            //+ ulSectorNum - 1;
        ulStartLBA = (ulCylinder * l_tCurrCHSMode.usNumOfHead + ulHead) * l_tCurrCHSMode.usSecPerTrk
            + ulSectorNum - 1;
    }

    ulSectorCount = (pSlot->CFis->Count & MSK_2F);

    if (0 == ulSectorCount)
    {
        ulSectorCount = (1 << 8);
    }

    pSlot->HCMDLenSec = ulSectorCount;
    pSlot->TotalRemainingBytes = (ulSectorCount << SEC_SIZE_BITS);
    pSlot->CurrentLBA = ulStartLBA;
    pSlot->Ch->PRDBC = pSlot->TotalRemainingBytes;

    return;
}

void L0_ATASetCmdState(PCB_MGR pSlot, U32 ulCmdState)
{
    pSlot->ATAState = ulCmdState;

    return;
}

U32 L0_ATAGetCmdState(PCB_MGR pSlot)
{
    return pSlot->ATAState;
}

U32 L0_ATAGenericCmdProc(PCB_MGR pCmdSlot)
{
    U32 ulFinished;
    U32 ulCmdCode;

    ulCmdCode = pCmdSlot->CFis->Command;

    switch (ulCmdCode)
    {
        case ATA_CMD_READ_VERIFY_SECTOR:
        case ATA_CMD_READ_VERIFY_SECTOR_EXT:
        case ATA_CMD_RECALIBRATE:
            ulFinished = L0_ATACmdProcObsoleteCmd(pCmdSlot);
            break;

        case ATA_CMD_SEEK:
        case ATA_CMD_IDLE:
        case ATA_CMD_IDLE_IMMEDIATE:
        case ATA_CMD_SLEEP:
            ulFinished = L0_ATACmdProcPowerStatusCmd(pCmdSlot);
            break;

        case ATA_CMD_IDENTIFY_DEVICE:
            ulFinished = L0_ATACmdProcIdentifyDevice(pCmdSlot);
            break;

        case ATA_CMD_SET_FEATURES:
            ulFinished = L0_ATACmdProcSetFeatures(pCmdSlot);
            break;

        case ATA_CMD_SET_MULTIPLE_MODE:
            ulFinished = L0_ATACmdProcSetMultipleMode(pCmdSlot);
            break;

        case ATA_CMD_CHECK_POWER_MODE:
            ulFinished = L0_ATACmdCheckPowerMode(pCmdSlot);
            break;

        case ATA_CMD_DATA_SET_MANAGEMENT:
            ulFinished = L0_ATACmdProcDatasetMgmt(pCmdSlot);
            break;

        case ATA_CMD_FLUSH_CACHE:
        case ATA_CMD_FLUSH_CACHE_EXT:
            ulFinished = L0_ATACmdProcFlushCache(pCmdSlot);
            break;

        case ATA_CMD_STANDBY_IMMEDIATE:
        case ATA_CMD_STANDBY:
            ulFinished = L0_ATACmdProcStandby(pCmdSlot);
            break;

        case ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC:
            ulFinished = L0_ATACmdProcExecDevDiag(pCmdSlot);
            break;

        case ATA_CMD_INITIALIZE_DEVICE_PARAMETERS:
            ulFinished = L0_ATACmdProcInitDevParam(pCmdSlot);
            break;

        case ATA_CMD_SMART:
            ulFinished = L0_ATACmdProcSmart(pCmdSlot);
            break;

        case ATA_CMD_READ_LOG_EXT:
            ulFinished = L0_ATACmdProcReadLogExt(pCmdSlot);
            break;

        case ATA_CMD_WRITE_LOG_EXT:
            ulFinished = L0_ATACmdProcWriteLogExt(pCmdSlot);
        break;

        case ATA_CMD_READ_NATIVE_MAX_ADDRESS:
        case ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT:
        case ATA_CMD_SET_MAX_ADDRESS:
        case ATA_CMD_SET_MAX_ADDRESS_EXT:
            ulFinished = L0_ATACmdProcHPA(pCmdSlot);
            break;

        case ATA_CMD_SECURITY_DISABLE_PASSWORD:
        case ATA_CMD_SECURITY_ERASE_PREPARE:
        case ATA_CMD_SECURITY_ERASE_UNIT:
        case ATA_CMD_SECURITY_FREEZE_LOCK:
        case ATA_CMD_SECURITY_SET_PASSWORD:
        case ATA_CMD_SECURITY_UNLOCK:
            ulFinished = L0_ATACmdProcSecurity(pCmdSlot);
            break; 

        case ATA_CMD_READ_BUFFER:
            ulFinished = L0_ATACmdProcReadBuffer(pCmdSlot);
            break;

        case ATA_CMD_WRITE_BUFFER:
            ulFinished = L0_ATACmdProcWriteBuffer(pCmdSlot);
            break;
            
#ifdef HOST_SATA
        case ATA_CMD_DOWNLOAD_MICROCODE_DMA:
            ulFinished = L0_ATACmdProcMicroCode(pCmdSlot);
            break;
#endif

        case ATA_CMD_VENDER_DEFINE:
            ulFinished = L0_ATACmdProcVIACmd(pCmdSlot);
            break;

        default:
            /* Command code is unrecognizable. We must abort it. */
            ulFinished = TRUE;
            L0_ATASetCmdState(pCmdSlot, (U32)ATACMD_ABORTED);
            L0_AhciSendSimpleResp(pCmdSlot);
            DBG_Printf("Unsupport cmd:0x%x!!\n", ulCmdCode);
            break;
    }

    return ulFinished;
}

MCU0_DRAM_TEXT LOCAL void L0_GlobalInfoUpdateLBACnt(PCB_MGR pSlot)
{
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;
    
    if (TRUE == pSlot->IsWriteDir)
    {
        if ((INVALID_8F - pHostInfoPage->TotalLBAReadLow) <= pSlot->HCMDLenSec)
        {
            pHostInfoPage->TotalLBAReadHigh++;
            pHostInfoPage->TotalLBAReadLow = pSlot->HCMDLenSec - (INVALID_8F - pHostInfoPage->TotalLBAReadLow);
        }
        else
        {
            pHostInfoPage->TotalLBAReadLow += pSlot->HCMDLenSec;
        }
    }
    else
    {
        if ((INVALID_8F - pHostInfoPage->TotalLBAWrittenLow) <= pSlot->HCMDLenSec)
        {
            pHostInfoPage->TotalLBAWrittenHigh++;
            pHostInfoPage->TotalLBAWrittenLow = pSlot->HCMDLenSec - (INVALID_8F - pHostInfoPage->TotalLBAWrittenLow);
        }
        else
        {
            pHostInfoPage->TotalLBAWrittenLow += pSlot->HCMDLenSec;
        }
    }
    
    return;
}

U32 L0_ATAMediaAccessProc(PCB_MGR pCmdSlot)
{
    U32 ulFinished;
    U32 ulParamValid;

    switch (pCmdSlot->ATAState)
    {
        /* 1. Preparing for splitting for a new received command
             or a pending command with more PRDT received: */
        case (U32)ATACMD_NEW:
#ifdef HOST_AHCI
            if ((U32)CB_NEW_CMD_READY == pCmdSlot->CbState)
#endif
            {
                /* 1) Checking whether command parameters are valid
                     for a new received command. */
                ulParamValid = L0_ATAMediaAccChkParamValid(pCmdSlot);

                if (FALSE == ulParamValid)
                {
                    /* Abort command due to parameter invalid. */
                    /* Caution: Aborting an NCQ command shall cause disk locked and must wait for host handling... */
                    L0_ATASetCmdState(pCmdSlot, (U32)ATACMD_ABORTED);
                    L0_AhciSendSimpleResp(pCmdSlot);
                    ulFinished = TRUE;
                    break;
                }

                /* 2) Preparing response FIS sequence for a new command. */
                L0_AhciDataXferPrepRespFISSeq(pCmdSlot);
            }

            /* 3) Preparing stage length information before splitting. */
            L0_AhciDataXferUpdateStageInfo(pCmdSlot);

            /* 4) Preparing WBQ sequence for either a new command or a pending command. */
            L0_AhciDataXferSendRespInfo(pCmdSlot);

            /* UpdateLBACnt */
            L0_GlobalInfoUpdateLBACnt(pCmdSlot);

            /* 5) Updating ATA command state to splitting. */
            L0_ATASetCmdState(pCmdSlot, (U32)ATACMD_SPLITTING);

        case (U32)ATACMD_SPLITTING:
            /* 2. Splitting the command from last position. Generating SCMDs for subsystems. */
            ulFinished = L0_ATAMediaAccSplitCmd(pCmdSlot);

            if (TRUE == ulFinished)
            {
                L0_ATASetCmdState(pCmdSlot, (U32)ATACMD_COMPLETED);
            }

            break;

        default:
            /* Control never goes here unless a bug occurs. */
            ASSERT(FAIL);
            break;
    }

    return ulFinished;
}

U32 L0_ATAMediaAccChkParamValid(PCB_MGR pSlot)
{
    BOOL bResult;
    U8 ucCmdCode = pSlot->CFis->Command;

    switch ( ucCmdCode )
    {
        /* For commands implementing PIO data in/out protocol, we shall check the multiple enable flag. */
        case ATA_CMD_READ_MULTIPLE:
        case ATA_CMD_WRITE_MULTIPLE:
        case ATA_CMD_READ_MULTIPLE_EXT:
        case ATA_CMD_WRITE_MULTIPLE_EXT:
            bResult = g_bMultipleDataOpen;
            break;

            /* For commands not implementing PIO data in/out protocol, we shall not perform the check. */
        default:
            bResult = TRUE;
            break;
    }

    return bResult;
}

U32 L0_ATACheckNCQCmdCode(U32 ulCmdCode)
{
    U32 ulIsNCQ;

    switch (ulCmdCode)
    {
        case ATA_CMD_READ_FPDMA_QUEUED:
        case ATA_CMD_WRITE_FPDMA_QUEUED:
        case ATA_CMD_NCQ_NON_DATA:
        case ATA_CMD_SEND_FPDMA_QUEUED:
        case ATA_CMD_RECEIVE_FPDMA_QUEUED:
            ulIsNCQ = TRUE;
            break;

        default:
            ulIsNCQ = FALSE;
    }

    return ulIsNCQ;
}

U32 L0_ATAProcessHostCmd(PCB_MGR pCmdSlot)
{
    U32 ulHostCmdType;
    U32 ulFinished;

    // get the host command type of the current host command
    ulHostCmdType = (U32)pCmdSlot->CmdType;

    // process the host command based on its type, please note
    // that only when the command is completed, should ulFinished
    // be set to TRUE
    if ((U32)HCMD_TYPE_SOFT_RESET == ulHostCmdType)
    {
        ulFinished = L0_ATASoftResetProc(pCmdSlot);
    }
    else if ((U32)HCMD_TYPE_DIRECT_MEDIA_ACCESS == ulHostCmdType)
    {
        if((SATA_POWER_STANDBY == g_ulPowerStatus) ||(SATA_POWER_IDLE == g_ulPowerStatus))
        {
            g_ulPowerStatus = SATA_POWER_ACTIVEORIDLE;
        }

        ulFinished = L0_ATAMediaAccessProc(pCmdSlot);
    }
    else if ((U32)HCMD_TYPE_OTHERS == ulHostCmdType || (U32)HCMD_TYPE_NEED_SPECIAL_STATUS == ulHostCmdType)
    {
        ulFinished = L0_ATAGenericCmdProc(pCmdSlot);
    }
    else
    {
        DBG_Printf("host command type error: %d\n", ulHostCmdType);
        DBG_Getch();
    }

    return ulFinished;
}

U32 L0_ATAGetIdentifyPage(void)
{
    return g_ulATAInfoIdfyPage;
}

void L0_VCMGetCurStage(PCB_MGR pSlot, U32 ulCmdCode)
{
    U32 ulCylinder, ulHead, ulSectorNum;

    g_VCM_Param.CurStage = 0;
    if ((ulCmdCode == ATA_CMD_READ_DMA) || (ulCmdCode == ATA_CMD_WRITE_DMA))
    {
        if (TEST_BIT(pSlot->CFis->Device, DEVHEAD_LBA_BIT))
        {
            /* LBA mode. */
            g_VCM_Param.CurStage = ((pSlot->CFis->Device & MSK_1F) << 24) + pSlot->CFis->LBALo;
        }
        else
        {
            /* Obsolete CHS mode. */
            ulCylinder = (pSlot->CFis->LBALo >> 8);
            ulHead = (pSlot->CFis->Device & MSK_1F);
            ulSectorNum = (pSlot->CFis->LBALo & MSK_2F);
            g_VCM_Param.CurStage = (ulCylinder * l_tCurrCHSMode.usNumOfHead + ulHead) * l_tCurrCHSMode.usSecPerTrk
                + ulSectorNum - 1;
        }
    }
    else if ((ulCmdCode == ATA_CMD_READ_DMA_EXT) || (ulCmdCode == ATA_CMD_WRITE_DMA_EXT))
    {
        g_VCM_Param.CurStage = (U32)(pSlot->CFis->LBAHi << 24) + pSlot->CFis->LBALo;
    }
}

