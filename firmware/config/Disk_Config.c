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
Filename     : Disk_Config.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blake Zhang

Description: 

Modification History:
20140902 blakezhang 001 first create
*******************************************************************************/
#include "Disk_Config.h"
#include "HAL_Xtensa.h"
#ifndef MCU0
#include "FW_Event.h"

GLOBAL MCU12_VAR_ATTR SUBSYSTEM_MEM_BASE g_FreeMemBase;
GLOBAL U32* g_pMCU1DramEndBase;

LOCAL BOOT_STATIC_FLAG l_BootStaticFlag;
LOCAL U32 l_ulSubSysNum;
LOCAL U32 l_ulSubSysCEBitMap;
LOCAL U32 l_ulSubSysPuNum;
LOCAL U32 l_ulSubSysMaxLBACnt;
LOCAL U32 l_ulSubsysNumBits;

void MCU12_DRAM_TEXT DiskConfig_Init(void)
{
    BOOTLOADER_FILE *pBootLoader;

    pBootLoader = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;

    l_BootStaticFlag.ulStaticFlag = pBootLoader->tSysParameterTable.sBootStaticFlag.ulStaticFlag;
    l_ulSubSysNum = pBootLoader->tSysParameterTable.ulSubSysNum;

    l_ulSubsysNumBits = 0;

    /*Calculate Subsystem number bit*/
    while ((1u << l_ulSubsysNumBits) < l_ulSubSysNum)
    {
        l_ulSubsysNumBits++;
    }
    
    if (MCU1_ID == HAL_GetMcuId())
    {
        l_ulSubSysCEBitMap = pBootLoader->tSysParameterTable.ulSubSysCEMap[0];
    }
    else
    {
        l_ulSubSysCEBitMap = pBootLoader->tSysParameterTable.ulSubSysCEMap[1];
    }
    
    l_ulSubSysPuNum     = pBootLoader->tSysParameterTable.ulSubSysCeNum;
    l_ulSubSysMaxLBACnt = (pBootLoader->tSysParameterTable.ulSysMaxLBACnt >> l_ulSubsysNumBits);

    //initialize DRAM total size parameter
    g_ulDramTotalSize = pBootLoader->tSysParameterTable.tHALFeature.aFeatureDW[0];

    //Sets the common miscellaneous information area pointer
    g_pMCU12MiscInfo = (PMCU12_COMMON_MISC_INFO)DSRAM1_MCU12_CMNMISC_BASE;

    if (DSRAM1_MCU12_CMNMISC_SIZE < sizeof(MCU12_COMMON_MISC_INFO))
    {
        DBG_Printf("g_pMCU12MiscInfo oversize\n");
        DBG_Getch();
    }

    HAL_InitGlobalPUNum();

    return;
}

void MCU12_DRAM_TEXT DiskConfig_Check(void)
{
    if ( (BOOT_METHOD_RSVD == l_BootStaticFlag.bsBootMethodSel) /* wrong boot method */
      || (l_ulSubSysNum > SUB_SYSTEM_NUM_MAX) /* wrong Sub System Number */
      || (l_ulSubSysPuNum > SUBSYSTEM_PU_NUM) /* unsupported PU Number */
      || (l_ulSubSysMaxLBACnt > MAX_LBA_IN_DISK)) /* unsupported LBA count */
    {
        DBG_Printf("Disk_Config_Check input Paramter wrong 0x%x !\n", MAX_LBA_IN_DISK);
        #ifndef L1_FAKE
        DBG_Getch();
        #endif
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

U8 MCU12_DRAM_TEXT DiskConfig_GetLLFMethold(void)
{
    U8 ucLLFMethod;
    
    ucLLFMethod = l_BootStaticFlag.bsLLFMethodSel;
    if (LLF_METHOD_NUM <= ucLLFMethod)
    {
        DBG_Printf("Get LLF Method Invalid: %d\n", ucLLFMethod);
        DBG_Getch();
    }

    return ucLLFMethod;
}

void MCU12_DRAM_TEXT DiskConfig_SetLLFMethold(U8 ucLLFMethod)
{
    if (LLF_METHOD_NUM <= ucLLFMethod)
    {
        DBG_Printf("Set LLF Method Invalid: %d\n", ucLLFMethod);
        DBG_Getch();
    }

    l_BootStaticFlag.bsLLFMethodSel = ucLLFMethod;

    return;
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

void BootUpInitMultiCore(void)
{
    if (MCU1_ID == HAL_GetMcuId())
    {
        g_pMCU1DramEndBase = (U32*)DRAM_MCU1_DRAM_END_POINTER_BASE;

        g_FreeMemBase.ulFreeSRAM0Base = DSRAM0_MCU1_BASE;
        g_FreeMemBase.ulFreeSRAM1Base = DSRAM1_MCU1_BASE;
        g_FreeMemBase.ulDRAMBase = DRAM_DATA_BUFF_MCU1_BASE;
        g_FreeMemBase.ulFreeOTFBBase = OTFB_FW_DATA_MCU1_BASE;
        g_FreeMemBase.ulFreeCacheStatusBase = OTFB_SSU1_MCU12_SHARE_BASE;  
    }
    else
    {
        g_pMCU1DramEndBase = (U32*)DRAM_MCU1_DRAM_END_POINTER_BASE;

        g_FreeMemBase.ulFreeSRAM0Base = DSRAM0_MCU2_BASE;
        g_FreeMemBase.ulFreeSRAM1Base = DSRAM1_MCU2_BASE;
        g_FreeMemBase.ulDRAMBase = DRAM_DATA_BUFF_MCU2_BASE;
        g_FreeMemBase.ulFreeOTFBBase = OTFB_FW_DATA_MCU2_BASE;
    }
    
    // normally, enable when xore disable; 
    g_FreeMemBase.ulRedInOtfbSharedBase = OTFB_RED_MCU12_SHARE_BASE;

    // normally, enable when xore enable;
    g_FreeMemBase.ulRedInDramSharedBase = DRAM_RED_MCU12_SHARE_BASE;    

    // FCMDQ Status
    g_FreeMemBase.ulSsuInOtfbSharedBase = OTFB_SSU0_MCU12_SHARE_BASE;

    // normally, disable
    g_FreeMemBase.ulSsuInDramSharedBase = DRAM_SSU0_MCU12_SHARE_BASE;

    // normally, FCMDQ Entry
    g_FreeMemBase.ulFreeSRAM1SharedBase = DSRAM1_MCU12_SHARE_BASE;

    // normally, the left l2 and l3 shared variables. 
    g_FreeMemBase.ulFreeDRAMSharedBase = DRAM_DATA_BUFF_MCU12_BASE;

    return;
}
#else
GLOBAL U32 *const g_pMCU1DramEndBase = (U32*)DRAM_MCU1_DRAM_END_POINTER_BASE;
#endif

