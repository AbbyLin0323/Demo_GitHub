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
Filename     : L1_GlobalInfo.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20130508     blakezhang     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"


GLOBAL  U8 g_ucSaveRTStatus;
GLOBAL  HOST_INFO_PAGE *g_pSubSystemHostInfoPage;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
extern GLOBAL BOOL gbGlobalInfoSaveFlag;

void MCU1_DRAM_TEXT L1_GlobalInfoDramMap(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;
    
    ulFreeDramBase = *pFreeDramBase;
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    // g_pSubSystemHostInfoPage
    g_pSubSystemHostInfoPage = (HOST_INFO_PAGE *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign( sizeof(HOST_INFO_PAGE) ) );

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    *pFreeDramBase = ulFreeDramBase;

    return;
}

/****************************************************************************
Function  : L1_GlobalInfoInit
Input     : 
Output    : 
Return    : 
Purpose   : 
device patameter and host info data structure init
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void MCU1_DRAM_TEXT L1_GlobalInfoInit(void)
{
    g_pSubSystemDevParamPage->Check1 = 0;
    g_pSubSystemDevParamPage->Check2 = 0;

    g_pSubSystemHostInfoPage->Check1 = 0;
    g_pSubSystemHostInfoPage->Check2 = 0;

    gbGlobalInfoSaveFlag = FALSE;
    return;
}

/****************************************************************************
Function  : L1_SetDefaultDeviceParam
Input     : 
Output    : 
Return    : 
Purpose   : 
device patameter data structure set to default
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void MCU1_DRAM_TEXT L1_SetDefaultDeviceParam(void)
{
    COM_MemZero((U32 *)g_pSubSystemDevParamPage,sizeof(DEVICE_PARAM_PAGE)/sizeof(U32));

    g_pSubSystemDevParamPage->AvailRsvdSpace = SUBSYSTEM_SUPERPU_NUM * RSVD_BLK_PER_LUN * LUN_NUM_PER_SUPERPU;
    g_pSubSystemDevParamPage->Check1 = GLOBAL_INFO_CHECK_1;
    g_pSubSystemDevParamPage->Check2 = GLOBAL_INFO_CHECK_2;

    return;
}

BOOL MCU1_DRAM_TEXT L1_IsDeviceParamValid(void)
{  
    if ((GLOBAL_INFO_CHECK_1 == g_pSubSystemDevParamPage->Check1) 
      && (GLOBAL_INFO_CHECK_2 == g_pSubSystemDevParamPage->Check2))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void MCU1_DRAM_TEXT L1_SetDefaultHostInfo(void)
{
    U32 ulSubSysMaxLBA;
  
    COM_MemZero((U32 *)g_pSubSystemHostInfoPage, sizeof(HOST_INFO_PAGE)/sizeof(U32));

    ulSubSysMaxLBA = DiskConfig_GetSubSysMaxLBACnt();

    if (0 != ulSubSysMaxLBA)
    {
        ulSubSysMaxLBA = min(ulSubSysMaxLBA, MAX_LBA_IN_DISK);
    }
    else
    {
        ulSubSysMaxLBA = MAX_LBA_IN_DISK;
    }

    g_pSubSystemHostInfoPage->HPAMaxLBA = ulSubSysMaxLBA * DiskConfig_GetSubSysNum();
    g_pSubSystemHostInfoPage->Check1 = GLOBAL_INFO_CHECK_1;
    g_pSubSystemHostInfoPage->Check2 = GLOBAL_INFO_CHECK_2;

    return;
}

BOOL MCU1_DRAM_TEXT L1_IsHostInfoValid(void)
{  
    if ((GLOBAL_INFO_CHECK_1 == g_pSubSystemHostInfoPage->Check1) 
      && (GLOBAL_INFO_CHECK_2 == g_pSubSystemHostInfoPage->Check2))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void MCU1_DRAM_TEXT L1_SaveHostInfo(U32 ulBuffAddr)
{
    if (ulBuffAddr >= DRAM_HIGH_START_ADDRESS)
    {
        ulBuffAddr -= DRAM_HIGH_ADDR_OFFSET;
    }

    /* save L0 HostInfo to SubSystem */
    COM_MemCpy((U32*)g_pSubSystemHostInfoPage, (U32 *)ulBuffAddr, sizeof(HOST_INFO_PAGE)/sizeof(U32));
    g_pSubSystemHostInfoPage->Check1 = GLOBAL_INFO_CHECK_1;
    g_pSubSystemHostInfoPage->Check2 = GLOBAL_INFO_CHECK_2;

    return;
}

void MCU1_DRAM_TEXT L1_LoadHostInfo(U32 ulBuffAddr)
{  
    U32 ulSubSysMaxLBA;
      
    ulSubSysMaxLBA = DiskConfig_GetSubSysMaxLBACnt();

    if (0 != ulSubSysMaxLBA)
    {
        ulSubSysMaxLBA = min(ulSubSysMaxLBA, MAX_LBA_IN_DISK);
    }
    else
    {
        ulSubSysMaxLBA = MAX_LBA_IN_DISK;
    }

    /* load HostInfo to L0 buffer */
    g_pSubSystemHostInfoPage->Check1 = GLOBAL_INFO_CHECK_1;
    g_pSubSystemHostInfoPage->Check2 = GLOBAL_INFO_CHECK_2;
    g_pSubSystemHostInfoPage->HPAMaxLBA = ulSubSysMaxLBA * DiskConfig_GetSubSysNum();
    
    if (ulBuffAddr >= DRAM_HIGH_START_ADDRESS)
    {
        ulBuffAddr -= DRAM_HIGH_ADDR_OFFSET;
    }
    COM_MemCpy((U32 *)ulBuffAddr, (U32 *)g_pSubSystemHostInfoPage, sizeof(HOST_INFO_PAGE)/sizeof(U32));

    return;
}

/****************************************************************************
Function  : L1_SecurityEraseSetDeviceParam
Input     : 
Output    : 
Return    : 
Purpose   : 
set part of device parameter data after Security Erase
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void MCU1_DRAM_TEXT L1_SecurityEraseSetDeviceParam(void)
{
    g_pSubSystemDevParamPage->ProgramFailCnt = 0;
    g_pSubSystemDevParamPage->EraseFailCnt = 0;
    g_pSubSystemDevParamPage->SafeShutdownCnt = 0;
    g_pSubSystemDevParamPage->SYSUnSafeShutdownCnt = 0;
    g_pSubSystemDevParamPage->SYSUECCCnt = 0;

    return;
}

/********************** FILE END ***************/

