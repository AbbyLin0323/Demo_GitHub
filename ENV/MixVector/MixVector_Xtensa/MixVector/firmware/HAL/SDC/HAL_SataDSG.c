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
Filename    : HAL_SataDSG.c
Version     : Ver 1.1
Author      : Gavin
Date        : 2014.12.11
Description : Driver for SATA DSG, including trigger/set valid/get address, etc.
Others      : VT3514 C0 HW is different from A0/B0, please refer to VT3514_C0
Modify      :
20141211    Gavin     add support for VT3514 C0 HW
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_SataDSG.h"
#include "HAL_SataIO.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#if defined(SIM)
#include "sim_SataDSG.h"
#endif

LOCAL MCU12_VAR_ATTR volatile SATA_DSG_REPORT *l_pSataDsgReportRead;
LOCAL MCU12_VAR_ATTR volatile SATA_DSG_REPORT *l_pSataDsgReportWrite;
#ifdef VT3514_C0
LOCAL MCU12_VAR_ATTR U32 l_ulMcuId;
#endif

/*------------------------------------------------------------------------------
Name: HalSataDsgGetLock
Description: 
    get Sata DSG report lock for A0/B0 ASIC.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    patch A0/B0 AISC HW multicore bug when access SATA DSG related REGs
------------------------------------------------------------------------------*/
LOCAL void HalSataDsgGetLock(void)
{
#ifndef VT3514_C0
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SATA_DSG_REPORT);
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: HalSataDsgReleaseLock
Description: 
    get Sata DSG report lock for A0/B0 ASIC.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    patch A0/B0 AISC HW multicore bug when access SATA DSG related REGs
------------------------------------------------------------------------------*/
LOCAL void HalSataDsgReleaseLock(void)
{
#ifndef VT3514_C0
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SATA_DSG_REPORT);
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SataDsgInit
Description: 
    initialize sata DSG base address;
    set sata DSG to default status.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    in bootup stage, call this function to initialize sata DSG;
    any other sata DSG functions can be called after this function finished.
------------------------------------------------------------------------------*/
void HAL_SataDsgInit(void)
{
#ifdef VT3514_C0
    l_ulMcuId = HAL_GetMcuId();
    if ((MCU0_ID == l_ulMcuId) || (MCU1_ID == l_ulMcuId))
    {
        l_pSataDsgReportRead = (volatile SATA_DSG_REPORT *)&rSataDsgReportRead1;
        l_pSataDsgReportWrite = (volatile SATA_DSG_REPORT *)&rSataDsgReportWrite1;
    }
    else if (MCU2_ID == l_ulMcuId)
    {
        l_pSataDsgReportRead = (volatile SATA_DSG_REPORT *)&rSataDsgReportRead2;
        l_pSataDsgReportWrite = (volatile SATA_DSG_REPORT *)&rSataDsgReportWrite2;
    }

#else
        l_pSataDsgReportRead = (volatile SATA_DSG_REPORT *)&rSataDsgReportRead1;
        l_pSataDsgReportWrite = (volatile SATA_DSG_REPORT *)&rSataDsgReportWrite1;
#endif
    /* Set Sata mode enable */
    rSGESataMode |= 1;
}

/*------------------------------------------------------------------------------
Name: HAL_GetCurSataDsg
Description: 
    get current valid Sata DSG but not trigger next one.
Input Param:
    U8 Type£º 0 = type0; 1 = type1;
            type0:sata DSG ID =  0~63; 
            type1:sata DSG ID = 64~127;
Output Param:
    U16 *PDsgId: pointer to obtained sata DSG id;
Return Value:
    BOOL:
        0 = no sata DSG got;
        1 = sata DSG got success;
Usage:
    when build sata DSG chain, call this function to get one sata DSG.
    caution:after call funtion HAL_TriggerSataDsg,current sata DSG owns to you really.
------------------------------------------------------------------------------*/
BOOL HAL_GetCurSataDsg(U16 *PDsgId, U8 Type)
{
    BOOL ucStsFlag;

    if(DSG_TYPE_WRITE == Type)
    {
        if(TRUE == l_pSataDsgReportWrite->bsSataDsgValidEn)
        {
            *PDsgId = l_pSataDsgReportWrite->bsSataDsgId;
            ucStsFlag = 1;
        }
        else
        {
            *PDsgId = INVALID_4F;
            ucStsFlag = 0;        
        }
        
    }
    else if(DSG_TYPE_READ == Type)
    {
        if(TRUE == l_pSataDsgReportRead->bsSataDsgValidEn)
        {
            *PDsgId = l_pSataDsgReportRead->bsSataDsgId;        
            ucStsFlag = 1;            
        }
        else
        {
            *PDsgId = INVALID_4F;
            ucStsFlag = 0;    
        }        
    }
    else
    {
        DBG_Getch();
    }

    return ucStsFlag;
}

/*------------------------------------------------------------------------------
Name: HAL_TriggerSataDsg
Description: 
    trigger next sata DSG.
Input Param:
    U8 Type£º 0 = sata type0; 1 = sata type1;
            type0:sata DSG ID = 0~63;
            type1:sata DSG ID = 64~127; 
Output Param:
    none
Return Value:
   none
Usage:
    when build sata DSG chain, call this function to triger next sata DSG.
------------------------------------------------------------------------------*/
void HAL_TriggerSataDsg( U8 Type)
{
#if defined (SIM)
#ifdef VT3514_C0

    if(DSG_TYPE_READ == Type)
    {
        if ((MCU0_ID == l_ulMcuId) || (MCU1_ID == l_ulMcuId))
        {
            DSG_AllocateSataReadDsg1();
        }
        else if (MCU2_ID == l_ulMcuId)
        {
            DSG_AllocateSataReadDsg2();
        }
    }
    else
    {
        if ((MCU0_ID == l_ulMcuId) || (MCU1_ID == l_ulMcuId))
        {
            DSG_AllocateSataWriteDsg1();
        }
        else if (MCU2_ID == l_ulMcuId)
        {
            DSG_AllocateSataWriteDsg2();
        }
        
    }


#else
    if(DSG_TYPE_READ == Type)
    {
        DSG_AllocateSataReadDsg1();
    }
    else if(DSG_TYPE_WRITE == Type)
    {
        DSG_AllocateSataWriteDsg1();
    }
    else
    {
        DBG_Getch();
    }    
#endif
#else
    HalSataDsgGetLock();

    if(DSG_TYPE_WRITE == Type)
    {
        l_pSataDsgReportWrite->bsSataDsgTrigger = TRUE;
    }
    else
    {
        l_pSataDsgReportRead->bsSataDsgTrigger = TRUE;
    }

    HalSataDsgReleaseLock();

#endif    
}

/*------------------------------------------------------------------------------
Name: HAL_GetSataDsgAddr
Description: 
    get addrress of sata DSG according to DSG ID.
Input Param:
    U16 DsgId: sata DSG id
Output Param:
    none
Return Value:
    U32: address for sata DSG
Usage:
    if get one sata DSG by HAL_GetNormalDsg function, call this function to get its address.
------------------------------------------------------------------------------*/
U32 HAL_GetSataDsgAddr(U16 DsgId)
{
    if(DsgId  >= SATA_TOTAL_DSG_NUM)
    {
        DBG_Getch();
    }
    else
    {
        return (U32)(DsgId*sizeof(SATA_DSG)+SATA_DSG_BASE);
    }

    return 0;
}

/*==============================================================================
Func Name  : HAL_SetSataDsgValid
Input      : U16 usDsgId  
Output     : NONE
Return Val : NONE
Discription: Set one Sata Dsg Valid. to confirm hardware this DSG is used.
Usage      : 
History    : 
    1. 2013.11.6 Haven Yang create function
==============================================================================*/
void HAL_SetSataDsgValid(U16 usDsgId)
{
#ifdef SIM
    DSG_SetSataDsgValid(usDsgId);
#else
    HalSataDsgGetLock();

    if(usDsgId >= SATA_TOTAL_DSG_NUM)
    {
        DBG_Getch();
    }
    if(usDsgId < SATA_TYPE0_DSG_NUM)
    {      
        l_pSataDsgReportRead->bsSataDsgWrIndex = usDsgId;
        l_pSataDsgReportRead->bsSataDsgValue = 1;
        l_pSataDsgReportRead->bsSataDsgWrEn = TRUE;
    }
    else
    {
        l_pSataDsgReportWrite->bsSataDsgWrIndex = usDsgId;
        l_pSataDsgReportWrite->bsSataDsgValue = 1;
        l_pSataDsgReportWrite->bsSataDsgWrEn = TRUE;
    }

    HalSataDsgReleaseLock();
#endif    
}

/*------------------------------------------------------------------------------
Name: HAL_ResetSataDSG
Description: 
    reset all SATA DSG to initial status.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    called in error handling case.
------------------------------------------------------------------------------*/
void HAL_ResetSataDSG(void)
{
    rSGESataMode &= 0xFFFFFFFE;
    rSGESataMode |= 1;
}

/*------------------------------------------------------------------------------
Name: HAL_SetFirstDSGID
Description: 
    set the first DSG id in DSG chain to SDC module.
Input Param:
    U8 ucCmdTag: host command tag
    U8 ucDSGID: the first DSG id in the DSG chain
Output Param:
    none
Return Value:
    void
Usage:
    called before set fisrt data ready.
------------------------------------------------------------------------------*/
void HAL_SetFirstDSGID(U8 ucCmdTag, U8 ucDSGID)
{
    rFIRST_DSG_ID[ucCmdTag] = ucDSGID;
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_GetAvailableSataDsgCnt
Description: 
    report total number of read/write DSG that can be used by FW(no matter MCU1 or
    MCU2 use), including the one which is being shown in report register.
Input Param:
    U8 Type£º 0 = sata type0; 1 = sata type1;
            type0(read) : sata DSG ID = 0~63;
            type1(write): sata DSG ID = 64~127; 
Output Param:
    none
Return Value:
    U8: total number of read/write DSG which can be used by FW
Usage:
    In multi-core ENV, FW may need this function to check peer MCU can get next DSG or not.
    supported in C0 HW only.
------------------------------------------------------------------------------*/
U8 HAL_GetAvailableSataDsgCnt( U8 Type)
{
    if(DSG_TYPE_READ == Type)
    {
        return rAvailableReadDsgCnt;
    }
    else
    {
        return rAvailableWriteDsgCnt;
    }
}
/* end of file HAL_SataDSG.c */

