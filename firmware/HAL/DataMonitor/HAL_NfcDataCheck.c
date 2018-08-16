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
Filename    : HAL_NfcDataCheck.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.10.30
Description : In VT3514 C0 design, NFC support checking first byte of every sector
              when read/write data to flash. FW need to prepare information of
              first byte in SRAM.
              This file provide basic function interface for this feature.
Others      : The data check function only supported in C0 design.
Modify      :
20141030    Gavin     Create file
20141104    Gavin     add description
20150723    Gavin     add spin-lock protection for multi-core usage
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_MultiCore.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_NfcDataCheck.h"

/* pointer to SRAM for first byte data */
LOCAL MCU12_VAR_ATTR volatile FIRST_BYTE_AREA *l_pFirstByteArea;

/*------------------------------------------------------------------------------
Name: HAL_DataCheckInit
Description: 
    Init FW software pointer and enable NFC Data Check function.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function in boot/init sequence if need Data Check support by NFC;
History:
    20141030    Gavin   create function
------------------------------------------------------------------------------*/
void HAL_DataCheckInit(void)
{
    //l_pFirstByteArea = (volatile FIRST_BYTE_AREA *)EM_LBA_BASE;

    /*YJ remove spinlock*/
    //HAL_MultiCoreGetSpinLockWait(SPINLOCKID_DATA_CHECK);

    /* enable First Byte Check function and set its mode
    rNfcModeConfig bit[6] = 0: check every sector; bit[6] = 1: only check even sector
    rNfcModeConfig bit[7] = 1: global enable EM and Data Check check function; bit[7] = 0: disable
    */
    rNfcModeConfig &= ~(1 << 6);
    rNfcModeConfig |= (1 << 7);

    //HAL_MultiCoreReleaseSpinLock(SPINLOCKID_DATA_CHECK); 

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_DataCheckSetValue
Description: 
    set first byte value of one sector to SRAM for NFC load.
Input Param:
    U8 ucPU: PU number
    U8 ucWtPtr: write pointer of PU
    U8 ucFirstByteVal: first byte value of sector
    U8 ucFirstByteIndex: the first byte index in SRAM entry of one PU level
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function before trigger NFC;
History:
    20141030    Gavin   create function
    20141125    Gavin   modify input parameter
------------------------------------------------------------------------------*/
void HAL_DataCheckSetValue(U8 ucPU, U8 ucWtPtr, U8 ucFirstByteVal, U8 ucFirstByteIndex)
{
    volatile FIRST_BYTE_ENTRY *pFirstByteEntry;

    pFirstByteEntry = &l_pFirstByteArea->aFirstByteEntry[ucPU][ucWtPtr]; //rmv logic PU by abby

    pFirstByteEntry->aFirstByteVal[ucFirstByteIndex] = ucFirstByteVal;

    return;
}

/* end of file HAL_NfcDataCheck.c */
