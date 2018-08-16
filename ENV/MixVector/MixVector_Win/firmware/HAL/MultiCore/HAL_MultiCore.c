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
Filename    : HAL_MultiCore.c
Version     : Ver 1.0
Author      : HenryLuo
Date        : 2012.02.14
Description : encapsulete subsystem boot and SpinLock relative interface.
Others      : 
Modify      :
20140905    Tobey     uniform coding style.
*******************************************U***********************************/

#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"
#include "HAL_GLBReg.h"

/*------------------------------------------------------------------------------
Name: HAL_StartSubSystemMCU
Description: 
    set subsystem MCU start to run.
Input Param:
    U8 ucSubSysIdx: Subsystem num, only 0 or 1 available.  
Output Param:
    none
Return Value:
    none
Usage:
    It's invoke by MCU0 only. to set subsystem MCU start to run.
History:
    20140909    Tobey   add note
------------------------------------------------------------------------------*/
void MULTI_CORE_TEXT_ATTR HAL_StartSubSystemMCU(U8 ucSubSysIdx)
{
    U32 ulSubSysMask;

    if (SUBSYSTEM_NUM_MAX <= ucSubSysIdx)
    {
        return;
    }
    
    /* Subsystem MCU would boot from ISRAM */
    rGlbMCUCtrl = 0x110;

    /* Clear RESET_ signal for specified subsystem MCU. */
    if (0 == ucSubSysIdx)
    {
        ulSubSysMask = ~(R_RST_MCU1 | R_RST_MCU1IF);
    }

    else
    {
        ulSubSysMask = ~(R_RST_MCU2 | R_RST_MCU2IF);
    }
    rGlbMcuSgeRst &= ulSubSysMask;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_MultiCoreReleaseSpinLock
Description: 
    release a appointed SpinLock.
Input Param:
    U8 ucLockId: SpinLock Id.  
Output Param:
    none
Return Value:
    none
Usage: when SpinLock bounded work finished, invoke it to release the SpinLock.
History:
    20140917    Tobey  uniform coding style
------------------------------------------------------------------------------*/
void MULTI_CORE_TEXT_ATTR HAL_MultiCoreReleaseSpinLock(U8 ucLockId)
{
    *(volatile U32*)(SPINLOCK_BASE_ADDRESS + ucLockId*4) = 0;
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_MultiCoreGetSpinLock
Description: 
    Get a appointed SpinLock.
Input Param:
    U8 ucLockId: SpinLock Id.  
Output Param:
    none
Return Value:
    none
Usage:
    all MCU get a SpinLock by invoke it, if more than one core get a appointed 
    SpinLock at the same time, HW do arbitration.
History:
    20140909    Tobey   uniform coding style
------------------------------------------------------------------------------*/
#ifdef VT3514_C0
BOOL MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLock(U8 ucLockId)
{
#ifndef SIM
    U32 ulMcuID;
    U32 ulLockValue;
        
    ulMcuID = HAL_GetMcuId();
    if ((MCU0_ID == ulMcuID) || (MCU1_ID == ulMcuID) || (MCU2_ID == ulMcuID))
    {
        *(volatile U32*)(SPINLOCK_BASE_ADDRESS + ucLockId*4) = ulMcuID;
    }
    else
    {
        DBG_Getch();
    }

    ulLockValue = *(volatile U32*)(SPINLOCK_BASE_ADDRESS + ucLockId*4);

    return (ulLockValue == ulMcuID);
#else
    return TRUE;
#endif
}
#else
BOOL MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLock(U8 ucLockId)
{
#ifndef SIM
    U32 ulMcuID;
    U32 ulLockValue;

    ulMcuID = HAL_GetMcuId();
    if ((MCU1_ID == ulMcuID) || (MCU2_ID == ulMcuID))
    {
        *(volatile U32*)(SPINLOCK_BASE_ADDRESS + ucLockId*4) = ulMcuID-1;
    }
    else
    {
        return TRUE;
    }

    ulLockValue = *(volatile U32*)(SPINLOCK_BASE_ADDRESS + ucLockId*4);

    return (ulLockValue == ulMcuID-1);
#else
    return TRUE;
#endif
}
#endif
/*------------------------------------------------------------------------------
Name: HAL_MultiCoreGetSpinLockWait
Description: 
    try to get a appointed SpinLock again and again until real get it.
Input Param:
    U8 ucLockId: SpinLock Id.  
Output Param:
    none
Return Value:
    none
Usage:
    MCU invoek it to get a appointed SpinLock without fail.
History:
    20140909    Tobey   uniform coding style
------------------------------------------------------------------------------*/
void MULTI_CORE_TEXT_ATTR HAL_MultiCoreGetSpinLockWait(U8 ucLockId)
{
    BOOL bSts = FALSE;
    do
    {
        bSts = HAL_MultiCoreGetSpinLock(ucLockId);
    }while (FALSE == bSts);
    return;
}

/********************** FILE END ***************/

