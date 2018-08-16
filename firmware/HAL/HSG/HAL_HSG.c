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
Filename    : HAL_HSG.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2013.09.02
Description : this file encapsulate HSG driver interface. 
Others      : 
Modify      :
20130902    Gavin     Create file
20140909    Tobey     uniform coding style.
*******************************************************************************/

#include "HAL_HSG.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#if defined(SIM)
#include "sim_HSG.h"
#endif

LOCAL MCU12_VAR_ATTR volatile HSG_REPORT_MCU *l_pHsgReport;
LOCAL MCU12_VAR_ATTR U32 l_ulMcuID;

/*----------------------------------------------------------------------------
Name: HAL_HsgInit
Description: 
    initialize HSG base address;
    set HSG to default status.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    in bootup stage, call this function to initialize HSG;
    any other HSG functions can be called after this function finished.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
void HAL_HsgInit(void)
{
    l_ulMcuID = HAL_GetMcuId();
    if (MCU0_ID == l_ulMcuID)
    {
        l_pHsgReport = (pHSG_REPORT_MCU)(&rHsgReportMcu0);
    }  
    else  if (MCU1_ID == l_ulMcuID)
    {
        l_pHsgReport = (pHSG_REPORT_MCU)(&rHsgReportMcu1);
    }
    else //MCU2_ID
    {
        l_pHsgReport = (pHSG_REPORT_MCU)(&rHsgReportMcu2);
    } 

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetHsg
Description: 
    Obtain current valid HSG, and trigger next HSG
Input Param:
    none
Output Param:
    U16 *PHsgId: pointer to obtained HSG id;
Return Value:
    U8: 0 = no HSG got;
        1 = DSG got success;
        others = invalid return value;
Usage:
    when building HSG chain, call this function to get HSG.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
BOOL HAL_GetHsg(U16 *pHsgId)
{
    BOOL bStsFlag;

    bStsFlag = HAL_GetCurHsg(pHsgId);
    HAL_TriggerHsg();

    return bStsFlag;
}

/*------------------------------------------------------------------------------
Name: HAL_IsHsgValid
Description: 
    check if there has valid HSG.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: 
    1:there has valid HSG.
    0:there hasn't valid HSG.
Usage:
History:
    201409011    Tobey   uniform coding style   
------------------------------------------------------------------------------*/
BOOL HAL_IsHsgValid(void)
{
    return l_pHsgReport->bsHsgValidEn;
}

/*------------------------------------------------------------------------------
Name: HAL_GetCurHsg
Description: 
    obtain current valid HSG, but not trigger next HSG.
Input Param:
    none
Output Param:
    U16 *PHsgId:pointer to obtained HSG
Return Value:
    BOOL: 0:obtain success
          1:obtain fail
Usage:
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
BOOL HAL_GetCurHsg(U16 *pHsgId)
{
#if 1
    U32 HsgReport = *(U32 *)l_pHsgReport;
    if (HsgReport & 0x80000000)
    {
        //Bit[30:21] is the HSGID
        *pHsgId = (HsgReport & 0x7FE00000) >> 21;
        return TRUE;
    }

    *pHsgId = INVALID_4F; 
    return FALSE;
#else
    BOOL bStsFlag;
    if (TRUE == l_pHsgReport->bsHsgValidEn)
    {
        *pHsgId = l_pHsgReport->bsHsgId;
        bStsFlag = TRUE;
    }
    else
    {
        *pHsgId = INVALID_4F;
        bStsFlag = FALSE;
    }
    
    return bStsFlag;
#endif
}

/*------------------------------------------------------------------------------
Name: HAL_TriggerHsg
Description: 
    trigger next HSG.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Trigger after get current HSG, obtained current valid hsg owns to it,
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
void HAL_TriggerHsg(void)
{
#ifdef SIM
    HSG_AllocateHsg(l_ulMcuID);
#else
    #ifdef VT3533_A2ECO_DSGRLSBUG
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLC_DSG_OR_HSG);
    #endif

    //l_pHsgReport->bsHsgTrigger = TRUE;
    l_pHsgReport->DW = 0x00100000; //BIT20

    #ifdef VT3533_A2ECO_DSGRLSBUG
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLC_DSG_OR_HSG);
    #endif
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetHsgAddr
Description: 
    get addrress of HSG according to DSG ID.
Input Param:
    U16 HsgId: HSG id
Output Param:
    none
Return Value:
    U32: address for HSG
Usage:
    if get one HSG by HAL_GetHsg function, call this function to get its address.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
U32 HAL_GetHsgAddr(U16 usHsgId)
{
    if (usHsgId >= HSG_NUM)
    {
        DBG_Getch();
        return INVALID_8F;
    }
    else
    {
        return (U32)(usHsgId*sizeof(HSG_ENTRY)+ SATA_DSG_BASE);
    }   
}

/*------------------------------------------------------------------------------
Name: HAL_SetHsgSts
Description: 
    set HSG sts(valid or not) according to StsValue
Input Param:
    U16 HsgId: nromal DSG id
    U8 StsValue: 1 = valid;  0= invalid; others not allowed;
Output Param:
    none
Return Value:
    none
Usage:
    set one HSG valid after allocated and filled it with contents.
    set one HSG invalid if it's not longer needed.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
void HAL_SetHsgSts(U16 usHsgId, U8 ucStsValue)
{
#if defined(SIM)
    if (HSG_INVALID == ucStsValue)
    {
        HSG_SetHsgInvalid(usHsgId);
    }
    else if (HSG_VALID == ucStsValue)
    {
        HSG_SetHsgValid(usHsgId);
    }
    else
    {
        DBG_Getch();
    }
#else
#if 1
    l_pHsgReport->DW = (usHsgId << 1) | 0x00000801; // Due to ucStsValue is 1
#else
    l_pHsgReport->bsHsgValue = ucStsValue;
    l_pHsgReport->bsHsgWrIndex = usHsgId;
    l_pHsgReport->bsHsgWrEn = TRUE;
#endif
#endif
    return;
}

/* end of file HAL_HSG.c */

