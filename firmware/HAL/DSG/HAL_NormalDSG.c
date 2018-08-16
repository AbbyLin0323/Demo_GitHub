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
Filename    : HAL_NormalDSG.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2013.09.02
Description : this file encapsulate Normal DSG driver interface.
Others      :
Modify      :
20130902    Gavin     Create file
20140909    Tobey     uniform coding style.
*******************************************************************************/

#include "HAL_NormalDSG.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#if defined(SIM)
#include "sim_NormalDSG.h"
#endif

LOCAL MCU12_VAR_ATTR volatile DSG_REPORT_MCU  *l_pDsgReport;
LOCAL MCU12_VAR_ATTR U32 l_ulMcuID;

#ifdef VT3533_A2ECO_DSGRLSBUG

#define NORMAL_DSG_FIFO_DEPTH 32
U16 g_aNormalDsgFifo[NORMAL_DSG_FIFO_DEPTH] = { 0 };
U16 g_NormalDsgFifoTail = 0;
U16 g_NormalDsgFifoHead = 0;

void HAL_NormalDsgFifoInit(void)
{
    U16 usDsgId, usIdx = 0;

    while (usIdx < NORMAL_DSG_FIFO_DEPTH)
    {
        if (TRUE == HAL_GetCurNormalDsg(&usDsgId))
        {
            g_aNormalDsgFifo[usIdx] = usDsgId;
            //DBG_Printf("MCU#%d NormalDsgFifoInit usIdx %d. usDsgId %d\n", l_ulMcuID, usIdx,usDsgId);
            usIdx++;
        }
        HAL_TriggerNormalDsg();
    }

    g_NormalDsgFifoHead = 0;
    g_NormalDsgFifoTail = NORMAL_DSG_FIFO_DEPTH - 1;

    return;
}

LOCAL BOOL HAL_NormalDsgFifoIsEmpty(void)
{
    return (g_NormalDsgFifoHead == g_NormalDsgFifoTail) ? TRUE : FALSE;
}

LOCAL BOOL HAL_NormalDsgFifoIsFull(void)
{
    return (g_NormalDsgFifoHead == (g_NormalDsgFifoTail + 1) % NORMAL_DSG_FIFO_DEPTH) ? TRUE : FALSE;
}

LOCAL void HAL_NormalDsgFifoPush(U16 usDsgID)
{
    g_aNormalDsgFifo[g_NormalDsgFifoTail] = usDsgID;

    g_NormalDsgFifoTail = (g_NormalDsgFifoTail + 1) % NORMAL_DSG_FIFO_DEPTH;

    return;
}

LOCAL U16 HAL_NormalDsgFifoPop(void)
{
    U16 usDsgID;

    usDsgID = g_aNormalDsgFifo[g_NormalDsgFifoHead];

    g_NormalDsgFifoHead = (g_NormalDsgFifoHead + 1) % NORMAL_DSG_FIFO_DEPTH;

    return usDsgID;
}

#endif

/*----------------------------------------------------------------------------
Name: HAL_NormalDsgReset
Description:
software reset the normal dsg engine when error handling.
Input Param:
none
Output Param:
none
Return Value:
void
Usage:
History:
201703009    Jason   create
------------------------------------------------------------------------------*/
GLOBAL void HAL_NormalDsgReset(void)
{
    rGlbMcuSgeRst |= R_RST_SGEDSG;
    HAL_DelayCycle(20);  //wait 20 clock
    rGlbMcuSgeRst &= ~R_RST_SGEDSG;

    return;
}

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
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu0);
    }
    else if (MCU1_ID == l_ulMcuID)
    {
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu1);
    }
    else //MCU2_ID
    {
        l_pDsgReport = (pDSG_REPORT_MCU)(&rDsgReportMcu2);
    }

    #ifdef VT3533_A2ECO_DSGRLSBUG
    HAL_NormalDsgFifoInit();
    #endif

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
    20140911    Tobey    uniform coding style
    20161129    JasonGuo dsg_pool + spin_lock -> fixed hw bug in flash not idle.
------------------------------------------------------------------------------*/
BOOL HAL_GetNormalDsg(U16 *pDsgId)
{
    BOOL bStsFlag;

#ifdef VT3533_A2ECO_DSGRLSBUG
    U16 usDsgID;

    if (TRUE == HAL_NormalDsgFifoIsEmpty())
    {
        bStsFlag = FALSE;
    }
    else
    {
        *pDsgId = HAL_NormalDsgFifoPop();
        bStsFlag = TRUE;
    }

    if (TRUE == HAL_GetCurNormalDsg(&usDsgID))
    {
        HAL_NormalDsgFifoPush(usDsgID);
    }
#else
    bStsFlag = HAL_GetCurNormalDsg(pDsgId);
#endif

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
#if 1
    U32 DsgReport = *(U32 *)l_pDsgReport;
    if (DsgReport & 0x80000000)
    {
        //Bit[30:21] is the DSGID
        *pDsgId = (DsgReport & 0x7FE00000) >> 21;
        return TRUE;
    }

    *pDsgId = INVALID_4F;
    return FALSE;
#else
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
#endif
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

    #ifdef VT3533_A2ECO_DSGRLSBUG
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLC_DSG_OR_HSG);
    #endif

    //l_pDsgReport->bsDsgTrigger = TRUE;
    l_pDsgReport->DW = 0x00100000; //BIT20

    #ifdef VT3533_A2ECO_DSGRLSBUG
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLC_DSG_OR_HSG);
    #endif
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

#if 1
    l_pDsgReport->DW = (usDsgId << 1) | 0x00000801; // Due to ucStsValue is NORMAL_DSG_VALID
#else
    l_pDsgReport->bsDsgValue = ucStsValue;
    l_pDsgReport->bsDsgWrIndex = usDsgId;
    l_pDsgReport->bsDsgWrEn = TRUE;
#endif
#endif

    return;
}

/* end of file HAL_NormalDSG.c */

