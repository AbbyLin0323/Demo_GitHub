/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    : HAL_NormalDSG.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2013.09.02
Description : this file encapsulate Normal DSG driver interface. 
Others      : 
Modify      :
20130902    Gavin     Create file
20140909    Tobey     uniform coding style.
****************************************************************************/
/*
#include "HAL_NormalDSG.h"
#include "HAL_Xtensa.h"
#if defined(SIM)
#include "sim_NormalDSG.h"
#endif*/
#include "COM_Inc.h"

LOCAL  volatile pDSG_REPORT_MCU  l_pDsgReport;
LOCAL  U32 l_ulMcuID;

/*----------------------------------------------------------------------------
Name: HAL_NormalDsgInit
Description: 
    initialize normal DSG base address;
    set DSG to default status.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    in bootup stage, call this function to initialize normal DSG;
    any other normal DSG functions can be called after this function finished.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
void HAL_NormalDsgInit(void)
{
    l_ulMcuID = HAL_GetMcuId();
    if (MCU0_ID == l_ulMcuID)
    {
#ifdef VT3514_C0
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu0);
#else      
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu1);
#endif
    }
    else if (MCU1_ID == l_ulMcuID)
    {
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu1);
    }
    else //MCU2_ID
    {
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu2);
    }      
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetNormalDsg
Description: 
    get current Normal DSG and trigger next Normal DSG
Input Param:
    none
Output Param:
    U16 *PDsgId: pointer to obtained DSG id;
Return Value:
    BOOL:
        0 = DSG got fail;
        1 = DSG got success;
Usage:
    when build Flash DSG chain for CQ entry, call this function to get normal DSG.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
BOOL HAL_GetNormalDsg(U16 *pDsgId)
{
    BOOL bStsFlag;
    bStsFlag = HAL_GetCurNormalDsg(pDsgId);

    HAL_TriggerNormalDsg();
    return bStsFlag;
}

/*------------------------------------------------------------------------------
Name: HAL_IsNormalDsgValid
Description: 
    Check Current Normal DSG valid or not
Input Param:
    none
Output Param:
    nont
Return Value:
    BOOL:
        1 = there has valid normal dsg;
        0 = there hasn't valid normal dsg;
Usage:
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
BOOL HAL_IsNormalDsgValid(void)
{
    return l_pDsgReport->bsDsgValidEn;
}

/*------------------------------------------------------------------------------
Name: HAL_GetCurNormalDsg
Description: 
    Get Current Normal DSG, but not trigger next one.
Input Param:
    none
Output Param:
    U16 *pDsgId: pointer to obtained Dsg Id.    
Return Value:
    BOOL:
        1 = normal dsg got success;
        0 = normal dsg got fail;
Usage:
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
BOOL HAL_GetCurNormalDsg(U16 *pDsgId)
{
    BOOL bStsFlag;
    if (TRUE == l_pDsgReport->bsDsgValidEn)
    {
        *pDsgId = l_pDsgReport->bsDsgId;
        bStsFlag = TRUE;
    }
    else
    {
        *pDsgId = INVALID_4F;
        bStsFlag = FALSE;
    }
    return bStsFlag;
}

/*------------------------------------------------------------------------------
Name: HAL_TriggerNormalDsg
Description: 
   trigger next Normal DSG.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage: trigger dsg module to offer a next dsg.
History:
    201409011 Tobey   uniform coding style
------------------------------------------------------------------------------*/
void HAL_TriggerNormalDsg(void)
{
#ifdef SIM
    DSG_AllocateNormalDsg(l_ulMcuID);
#else
    l_pDsgReport->bsDsgTrigger = TRUE;
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetNormalDsgAddr
Description: 
    get addrress of normal DSG according to DSG ID.
Input Param:
    U16 DsgId: nromal DSG id
Output Param:
    none
Return Value:
    U32: address for normal DSG
Usage:
    if get one DSG by HAL_GetNormalDsg function, call this function to get its address.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
U32 HAL_GetNormalDsgAddr(U16 usDsgId)
{
    if(usDsgId >= NORMAL_DSG_NUM)
    {
        DBG_Getch();
        return 0;
    }
    else
    {
        return (U32)(usDsgId*sizeof(NORMAL_DSG_ENTRY) + NORMAL_DSG_BASE);
    }
}

/*------------------------------------------------------------------------------
Name: HAL_SetNormalDsgSts
Description: 
    set normal DSG sts(valid or not) according to StsValue
Input Param:
    U16 DsgId: nromal DSG id
    U8 StsValue: 1 = valid; 0 = invalid; others not allowed;
Output Param:
    none
Return Value:
    none
Usage:
    set one normal DSG valid after allocated and filled it with contents.
    set one normal DSG invalid if it's not longer needed.
History:
    201409011    Tobey   uniform coding style
------------------------------------------------------------------------------*/
void HAL_SetNormalDsgSts(U16 usDsgId, U8 ucStsValue)
{
#ifdef SIM
    if (NORMAL_DSG_INVALID == ucStsValue)
    {
        DSG_SetNormalDsgInvalid(usDsgId);
    }
    else if (NORMAL_DSG_VALID == ucStsValue)
    {
        DSG_SetNormalDsgValid(usDsgId);
    }
    else
    {
        DBG_Getch();
    }
#else
    l_pDsgReport->bsDsgValue = ucStsValue;
    l_pDsgReport->bsDsgWrIndex = usDsgId;
    l_pDsgReport->bsDsgWrEn = TRUE;
#endif

    return;
}

/* end of file HAL_NormalDSG.c */

