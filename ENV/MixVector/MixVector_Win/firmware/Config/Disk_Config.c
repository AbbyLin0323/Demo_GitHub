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
#include "Disk_Config.h"
#include "HAL_Xtensa.h"

LOCAL MCU12_VAR_ATTR BOOT_STATIC_FLAG l_BootStaticFlag;
LOCAL MCU12_VAR_ATTR U32 l_ulSubSysNum;
LOCAL MCU12_VAR_ATTR U32 l_ulSubSysCEBitMap;
LOCAL MCU12_VAR_ATTR U32 l_ulSubSysPuNum;
LOCAL MCU12_VAR_ATTR U32 l_ulSubSysMaxLBACnt;

void MCU12_DRAM_TEXT DiskConfig_Init(void)
{
    BOOTLOADER_FILE *pBootLoader;

    pBootLoader = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;

    l_BootStaticFlag.ulStaticFlag = pBootLoader->tSysParameterTable.sBootStaticFlag.ulStaticFlag;
    l_ulSubSysNum = pBootLoader->tSysParameterTable.ulSubSysNum;

    if (MCU1_ID == HAL_GetMcuId())
    {
        l_ulSubSysCEBitMap = pBootLoader->tSysParameterTable.ulSubSysCEMap[0];
    }
    else
    {
        l_ulSubSysCEBitMap = pBootLoader->tSysParameterTable.ulSubSysCEMap[1];
    }
    
    l_ulSubSysPuNum     = pBootLoader->tSysParameterTable.ulSubSysCeNum;
    l_ulSubSysMaxLBACnt = pBootLoader->tSysParameterTable.ulSubSysMaxLBACnt;

    return;
}

void MCU12_DRAM_TEXT DiskConfig_Check(void)
{
    if ( (BOOT_METHOD_RSVD == l_BootStaticFlag.bsBootMethodSel) /* wrong boot method */
      || (l_ulSubSysNum > SUB_SYSTEM_NUM_MAX) /* wrong Sub System Number */
      || (l_ulSubSysPuNum > PU_NUM) /* unsupported PU Number */
      || (l_ulSubSysMaxLBACnt > MAX_LBA_IN_DISK)) /* unsupported LBA count */
    {
        DBG_Printf("Disk_Config_Check input Paramter wrong 0x%x !\n", MAX_LBA_IN_DISK);
        DBG_Getch();
    }

    return;
}

U8 MCU12_DRAM_TEXT DiskConfig_GetBootMethod(void)
{
    return (U8)(l_BootStaticFlag.bsBootMethodSel);
}

BOOL MCU12_DRAM_TEXT DiskConfig_IsColdStart(void)
{
    if (TRUE == l_BootStaticFlag.bsWarmBoot)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
BOOL MCU12_DRAM_TEXT DiskConfig_IsLocalTestEn(void)
{
    if (TRUE == l_BootStaticFlag.bsLocalTest)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCU12_DRAM_TEXT DiskConfig_IsRollBackECTEn(void)
{
    if (TRUE == l_BootStaticFlag.bsRollBackECT)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCU12_DRAM_TEXT DiskConfig_IsRebuildGBEn(void)
{
    if (TRUE == l_BootStaticFlag.bsRebuildGB)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCU12_DRAM_TEXT DiskConfig_IsFormatBBTEn(void)
{
    if (TRUE == l_BootStaticFlag.bsFormatBBT)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCU12_DRAM_TEXT DiskConfig_IsScanIDBEn(void)
{
    if (TRUE == l_BootStaticFlag.bsFirstLLFScanIDB)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCU12_DRAM_TEXT DiskConfig_IsUseDefaultECT(void)
{
    if (TRUE == l_BootStaticFlag.bsUseDefault)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


U8 MCU12_DRAM_TEXT DiskConfig_GetSubSysNum(void)
{
    return (U8)(l_ulSubSysNum);
}

U32 MCU12_DRAM_TEXT DiskConfig_GetSubSysCEBitMap(void)
{
    return l_ulSubSysCEBitMap;
}

U32 MCU12_DRAM_TEXT DiskConfig_GetSubSysMaxLBACnt(void)
{
    return l_ulSubSysMaxLBACnt;
}

