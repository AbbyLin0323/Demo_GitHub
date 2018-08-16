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
Filename    :HAL_SearchEngine.c
Version     :
Author      :Kristin Wang
Date        :2014.7
Description :this file encapsulate Search Engine driver interface          
Others      :
Modify      :
20140915    Kristin    1. Coding style uniform
                       2. delete the definition of HAL_SECaseParamInit
*******************************************************************************/
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"
#include "HAL_SearchEngine.h"
#ifdef SIM
#include "sim_search_engine.h"
#endif

LOCAL U8 l_ucSEID;
LOCAL BOOL l_bGroupID;
LOCAL volatile SE_TRIGGER_SIZE_REG *l_pSETrigSizeReg;
LOCAL volatile SE_TYPE_REG *l_pSETypeReg;
LOCAL volatile SE_STATUS_REG *l_pSEStatusReg;

/*----------------------------------------------------------------------------
Name: HAL_SEClockEnable
Description: 
    Enable SEn's clock 
Input Param: 
    U8 ucSEID: SEn(n=0/1/2)
Output Param: 
    none
Return Value:
    void
Usage:
    There are two way to use this function:
    1. enable SEn's clock when initialize, and don't disable it during FW runs
    2. enable SEn's clock before use it, and disable its clock after using.
History:
20140915    Kristin    1. Coding style uniform
                       2. Input Param void -> U8
------------------------------------------------------------------------------*/
void HAL_SEClockEnable(U8 ucSEID)
{
    if (SE0 == ucSEID)
    {
        rGlbClkGating |= MSK_EN_SE0_CLK; 
    }
    else if (SE1 == ucSEID)
    {
        rGlbClkGating |= MSK_EN_SE1_CLK; 
    }
    else
    {
        rGlbClkGating |= MSK_EN_SE2_CLK;
    }

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SEClockDisable
Description: 
    Disable SEn's clock 
Input Param: 
    U8 ucSEID: SEn(n=0/1/2)
Output Param: 
    none
Return Value:
    void
Usage:
History:
20140915    Kristin    1. Coding style uniform
                       2. Input Param void -> U8
------------------------------------------------------------------------------*/
void HAL_SEClockDisable(U8 ucSEID)
{
    if (SE0 == ucSEID)
    {
        rGlbClkGating &= (~MSK_EN_SE0_CLK);
    }
    else if (SE1 == ucSEID)
    {
        rGlbClkGating &= (~MSK_EN_SE1_CLK); 
    }
    else
    {
        rGlbClkGating &= (~MSK_EN_SE2_CLK); 
    }

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SEOtfbEnable
Description: 
    If we want use Search Engine to search in memory, we need to tell HW the
  searched area is DRAM or OTFB.
Input Param: 
    U8 ucSEID: SEn(n=0/1/2)
Output Param: 
    none
Return Value:
    void
Usage:
    When we want Search Engine search in OTFB
History:
20140915    Kristin    1. Coding style uniform
                       2. Input Param void -> U8
                       3. add SpinLock usage when A0 or B0
------------------------------------------------------------------------------*/
void HAL_SEOtfbEnable(U8 ucSEID)
{
#if defined(VT3514_A0) || defined(VT3514_B0)
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SE);
#endif

    if (SE0 == ucSEID)
    {
        rGlbTrfc |= MSK_EN_SE0_OTFB; 
    }
    else if (SE1 == ucSEID)
    {
        rGlbTrfc |= MSK_EN_SE1_OTFB; 
    }
    else
    {
        rGlbTrfc |= MSK_EN_SE2_OTFB; 
    }

#if defined(VT3514_A0) || defined(VT3514_B0)
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SE);
#endif
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SEOtfbDisable
Description: 
    If we want use Search Engine to search in memory, we need to tell HW the
  searched area is DRAM or OTFB.
Input Param: 
    U8 ucSEID: SEn(n=0/1/2)
Output Param: 
    none
Return Value:
    void
Usage:
    When we want Search Engine search in DRAM
History:
20140915    Kristin    1. Coding style uniform
                       2. Input Param void -> U8
                       3. add SpinLock usage when A0 or B0
------------------------------------------------------------------------------*/
void HAL_SEOtfbDisable(U8 ucSEID)
{
#if defined(VT3514_A0) || defined(VT3514_B0)
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SE);
#endif

    if (SE0 == ucSEID)
    {
        rGlbTrfc &= (~MSK_EN_SE0_OTFB); 
    }
    else if (SE1 == ucSEID)
    {
        rGlbTrfc &= (~MSK_EN_SE1_OTFB); 
    }
    else
    {
        rGlbTrfc &= (~MSK_EN_SE2_OTFB); 
    }

#if defined(VT3514_A0) || defined(VT3514_B0)
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SE);
#endif
    
    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SEInit
Description: 
    initialize for Search Engine driver
Input Param: 
    none
Output Param: 
    none
Return Value:
    void
Usage:
    when FW initialize  
History:
20140915    Kristin    1. Coding style uniform
                       2. add A0 support -- don't allocate SE to MCU0
------------------------------------------------------------------------------*/
void HAL_SEInit(void)
{
    U8 ucMCUID;

    ucMCUID = HAL_GetMcuId();
    switch(ucMCUID)
    {
#ifdef VT3514_A0
    case MCU0_ID:
        DBG_Printf("HAL_SEInit: no available SE for MCU0\n");
        DBG_Getch();
        break;
    case MCU1_ID:
        l_ucSEID = SE0;
        break;

    case MCU2_ID:
        l_ucSEID = SE1;
        break;

    default:
        DBG_Printf("HAL_SEInit: MCU ID error %d\n", ucMCUID);
        DBG_Getch();
        break;
#else
    case MCU0_ID:
        l_ucSEID = SE0;
        break;

    case MCU1_ID:
        l_ucSEID = SE1;
        break;

    case MCU2_ID:
        l_ucSEID = SE2;
        break;

    default:
        DBG_Printf("HAL_SEInit: MCU ID error %d\n", ucMCUID);
        DBG_Getch();
        break;
#endif
    }

    HAL_SEClockEnable(l_ucSEID);

    l_pSETrigSizeReg = (volatile SE_TRIGGER_SIZE_REG *)&rSE_TRIGGER_SIZE_REG(l_ucSEID);
    l_pSETypeReg = (volatile SE_TYPE_REG *)&rSE_TYPE_REG(l_ucSEID);
    l_pSEStatusReg = (volatile SE_STATUS_REG *)&rSE_STATUS_REG(l_ucSEID);

    l_bGroupID = SE_1ST_MSK_GROUP;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SETriggerSearchValue
Description: 
    fill search max/min/same params into Search Engine registers, then trigger
  the SE.
Input Param: 
    SE_SEARCH_VALUE_PARAM *ptValParam: the param given to SE, 
  include start address, item count, search type, mask and so on.
Output Param: 
    none
Return Value:
    void
Usage:  
    trigger SE to search max/min/same in DRAM/OTFB
History:
20140915    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_SETriggerSearchValue(SE_SEARCH_VALUE_PARAM *ptValParam)
{
    l_pSETrigSizeReg->bsTrigger = 0;

    if (FALSE == ptValParam->bsOTFB)
    {
        HAL_SEOtfbDisable(l_ucSEID); 
        rSE_START_ADDR(l_ucSEID) = ptValParam->ulStartAddr - DRAM_START_ADDRESS;
    }
    else
    {
        HAL_SEOtfbEnable(l_ucSEID); 
        rSE_START_ADDR(l_ucSEID) = ptValParam->ulStartAddr - OTFB_START_ADDRESS;
    }
    if (FALSE == ptValParam->bsItemSizeDW)
    {
        rSE_SEARCH_LEN_QW(l_ucSEID) = ptValParam->ulItemCnt;
    }
    else
    {
        rSE_SEARCH_LEN_QW(l_ucSEID) = ptValParam->ulItemCnt/2;
    }
    rSE_PITCH_SIZE(l_ucSEID) = ptValParam->ucPitchQW;

    if (SE_1ST_MSK_GROUP == l_bGroupID)
    {  
        rSE_FMSK_SELECTION_LOW(l_ucSEID) = ptValParam->ulSelMaskLow;
        rSE_FMSK_SELECTION_HIGH(l_ucSEID) = ptValParam->ulSelMaskHigh;
        rSE_FMASK_CONDITION_LOW(l_ucSEID) = ptValParam->ulCondMaskLow;
        rSE_FMASK_CONDITION_HIGH(l_ucSEID) = ptValParam->ulCondMaskHigh;
        rSE_FMASK_FIELD_LOW(l_ucSEID) = ptValParam->ulFieldMaskLow;
        rSE_FMASK_FIELD_HIGH(l_ucSEID) = ptValParam->ulFieldMaskHigh;
        rSE_FVALUE_SEARCHED_LOW(l_ucSEID) = ptValParam->ulValToSearchLow;
        rSE_FVALUE_SEARCHED_HIGH(l_ucSEID) = ptValParam->ulValToSearchHigh;

        if ( TRUE == ptValParam->bsXorEn )
        {
            rSE_XOR_MASK_ENABLE(l_ucSEID) |= MSK_FGROUP_XOR_EN;
            rSE_FMASK_XOR_FIELD_LOW(l_ucSEID) = ptValParam->ulXorFieldMaskLow;
            rSE_FMASK_XOR_FIELD_HIGH(l_ucSEID) = ptValParam->ulXorFieldMaskHigh;
            rSE_FMASK_XOR_VALUE_LOW(l_ucSEID) = ptValParam->ulXorValueMaskLow;
            rSE_FMASK_XOR_VALUE_HIGH(l_ucSEID) = ptValParam->ulXorValueMaskHigh;
        }
        else
        {
            rSE_XOR_MASK_ENABLE(l_ucSEID) &= (~MSK_FGROUP_XOR_EN);
        }

        l_pSETypeReg->bsFGroupSType = ptValParam->bsSearchType;
        l_pSETypeReg->bsFGroupCType = ptValParam->bsCondType;
    }
    else
    {
        rSE_SMSK_SELECTION_LOW(l_ucSEID) = ptValParam->ulSelMaskLow;
        rSE_SMSK_SELECTION_HIGH(l_ucSEID) = ptValParam->ulSelMaskHigh;
        rSE_SMASK_CONDITION_LOW(l_ucSEID) = ptValParam->ulCondMaskLow;
        rSE_SMASK_CONDITION_HIGH(l_ucSEID) = ptValParam->ulCondMaskHigh;
        rSE_SMASK_FIELD_LOW(l_ucSEID) = ptValParam->ulFieldMaskLow;
        rSE_SMASK_FIELD_HIGH(l_ucSEID) = ptValParam->ulFieldMaskHigh;
        rSE_SVALUE_SEARCHED_LOW(l_ucSEID) = ptValParam->ulValToSearchLow;
        rSE_SVALUE_SEARCHED_HIGH(l_ucSEID) = ptValParam->ulValToSearchHigh;

        if (TRUE == ptValParam->bsXorEn)
        {
            rSE_XOR_MASK_ENABLE(l_ucSEID) |= MSK_SGROUP_XOR_EN;
            rSE_SMASK_XOR_FIELD_LOW(l_ucSEID) = ptValParam->ulXorFieldMaskLow;
            rSE_SMASK_XOR_FIELD_HIGH(l_ucSEID) = ptValParam->ulXorFieldMaskHigh;
            rSE_SMASK_XOR_VALUE_LOW(l_ucSEID) = ptValParam->ulXorValueMaskLow;
            rSE_SMASK_XOR_VALUE_HIGH(l_ucSEID) = ptValParam->ulXorValueMaskHigh;
        }
        else
        {
            rSE_XOR_MASK_ENABLE(l_ucSEID) &= (~MSK_SGROUP_XOR_EN);
        }

        l_pSETypeReg->bsSGroupSType = ptValParam->bsSearchType;
        l_pSETypeReg->bsSGroupCType = ptValParam->bsCondType;
    }
    
    if (FALSE == ptValParam->bsItemSizeDW)
    {
        l_pSETrigSizeReg->bsItemSize = SE_ITEM_SIZE_QWORD;
    }
    else
    {
        l_pSETrigSizeReg->bsItemSize = SE_ITEM_SIZE_DWORD;
    }

    l_pSETrigSizeReg->bsTrigger = TRUE;
#ifdef SIM
    SE_ModelProcess(l_ucSEID);
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SEGetSearchValueResult
Description: 
    get the search max/min/same result from SE registers
Input Param: 
    U8 ucSType: search type - max, min or same
Output Param: 
    SE_SEARCH_VALUE_RESULT *ptValResult: point to a search result structure,
   function fills result into this structure.
Return Value:
    void
Usage:  
    get the search max/min/same result after trigger SE
History:
20140915    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_SEGetSearchValueResult(U8 ucSType, SE_SEARCH_VALUE_RESULT *ptValResult)
{
    HAL_DelayCycle(SE_WAIT_TIME);

    while ( FALSE == l_pSEStatusReg->bStable )
    {
        /*wait search finish*/
    }

    if (SE_1ST_MSK_GROUP == l_bGroupID)
    {
        if ((SEARCH_TYPE_SAME != ucSType) ||
            ((SEARCH_TYPE_SAME == ucSType) && (TRUE == l_pSEStatusReg->bFGroupFind)))
        {
            ptValResult->bsFind = TRUE;
            ptValResult->ulIndex = rSE_FINDEX(l_ucSEID);
            ptValResult->ulRtnValueLow = rSE_RETURN_FVALUE_LOW(l_ucSEID);
            ptValResult->ulRtnValueHigh = rSE_RETURN_FVALUE_HIGH(l_ucSEID);
        }
        else
        {
            ptValResult->bsFind = FALSE;
        }
    }
    else
    {
        if ((SEARCH_TYPE_SAME != ucSType) ||
            ((SEARCH_TYPE_SAME == ucSType) && (TRUE == l_pSEStatusReg->bSGroupFind)))
        {
            ptValResult->bsFind = TRUE;
            ptValResult->ulIndex = rSE_SINDEX(l_ucSEID);
            ptValResult->ulRtnValueLow = rSE_RETURN_SVALUE_LOW(l_ucSEID);
            ptValResult->ulRtnValueHigh = rSE_RETURN_SVALUE_HIGH(l_ucSEID);
        }
        else
        {
            ptValResult->bsFind = FALSE;
        }
    }

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SESearchValue
Description: 
    call HAL_SETriggerSearchValue and HAL_SEGetSearchValueResult to achieve
   searching for max/min/same
Input Param: 
    SE_SEARCH_VALUE_PARAM *ptValParam: the param given to SE, 
  include start address, item count, search type, mask and so on.
Output Param: 
    SE_SEARCH_VALUE_RESULT *ptValResult: point to a search result structure,
   function fills result into this structure.
Return Value:
    void
Usage:  
    Search for max/min/same value in OTFB/DRAM, and the value satisfies some
  conditions.
History:
20140915    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_SESearchValue(SE_SEARCH_VALUE_PARAM *ptValParam, SE_SEARCH_VALUE_RESULT *ptValResult)
{
    HAL_SETriggerSearchValue(ptValParam);
    HAL_SEGetSearchValueResult(ptValParam->bsSearchType, ptValResult);

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SESearchBit
Description: 
    trigger SE to search bit 0 or 1 in memory or register(the register stores
  the DWORD data we fill), and get the search result.
Input Param: 
    SE_SEARCH_BIT_PARAM *pBitParam: the param given to SE, 
  include start address and length(search in memory) or a DWORD data(search in 
  register), search bit 1/0 and so on. 
Output Param: 
    SE_SEARCH_BIT_RESULT *pBitResult: search result from SE registers
Return Value:
    void
Usage:  
    Search for bit 0 or 1 using Search Engine
History:
20140915    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_SESearchBit(SE_SEARCH_BIT_PARAM *pBitParam, SE_SEARCH_BIT_RESULT *pBitResult)
{
    /* Trigger search bit */
    l_pSETrigSizeReg->bsTrigger = 0;

    if (TRUE == pBitParam->bReg)
    {
        rSE_BIT_SEARCH_DATA(l_ucSEID) = pBitParam->ulData;

        if (1 == pBitParam->bBitValue)  //search bit 1 in register
        {
            if (TRUE == pBitParam->bLoad)
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_REG_BIT1_LOAD;
            }
            else
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_REG_BIT1;
            }
        }
        else                            //search bit 0 in register
        {
            if (TRUE == pBitParam->bLoad)
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_REG_BIT0_LOAD;
            }
            else
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_REG_BIT0;
            }
        }
    }//if ( TRUE == pBitParam->bReg )
    else
    {
        if (FALSE == pBitParam->bOTFB)
        {
            HAL_SEOtfbDisable(l_ucSEID);
            rSE_BIT_SEARCH_ADDRESS(l_ucSEID) = pBitParam->ulStartAddr - DRAM_START_ADDRESS;
        }
        else
        {
            HAL_SEOtfbEnable(l_ucSEID);
            rSE_BIT_SEARCH_ADDRESS(l_ucSEID) = pBitParam->ulStartAddr - OTFB_START_ADDRESS;
        }

        rSE_BIT_SEARCH_SIZE(l_ucSEID) = pBitParam->usSizeQW;

        if (1 == pBitParam->bBitValue)  //search bit 1 in memory
        {
            if (TRUE == pBitParam->bLoad)
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_MEM_BIT1_LOAD;
            }
            else
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_MEM_BIT1;
            }
        }
        else                             //search bit 0 in memory
        {
            if (TRUE == pBitParam->bLoad)
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_MEM_BIT0_LOAD;
            }
            else
            {
                l_pSETrigSizeReg->bsItemSize = SE_SEARCH_MEM_BIT0;
            }
        }
    }//else of if ( TRUE == pBitParam->bReg )

    l_pSETrigSizeReg->bsTrigger = 1;
#ifdef SIM
    SE_ModelProcess(l_ucSEID);
#endif

    /* Get search result */
    HAL_DelayCycle(SE_WAIT_TIME);

    while (FALSE == l_pSEStatusReg->bBitSearchDone)
    {
        /*wait search finish*/
    }

    if (TRUE == pBitParam->bReg)
    {
        if (1 == pBitParam->bBitValue)
        {
            pBitResult->bFind = (BOOL)rSE_REG_BIT1_FIND(l_ucSEID);
            pBitResult->usBitIndex = rSE_REG_BIT1_INDEX(l_ucSEID);
        }
        else
        {
            pBitResult->bFind = (BOOL)rSE_REG_BIT0_FIND(l_ucSEID);
            pBitResult->usBitIndex = rSE_REG_BIT0_INDEX(l_ucSEID);
        }
    }
    else
    {
        if (1 == pBitParam->bBitValue)
        {
            pBitResult->bFind = (BOOL)rSE_MEM_BIT1_FIND(l_ucSEID);
            pBitResult->usBitIndex = rSE_MEM_BIT1_INDEX(l_ucSEID);
        }
        else
        {
            pBitResult->bFind = (BOOL)rSE_MEM_BIT0_FIND(l_ucSEID);
            pBitResult->usBitIndex = rSE_MEM_BIT0_INDEX(l_ucSEID);
        }
    }

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SESetBit
Description: 
    trigger SE to set bit 0 or 1 at the appointed location in DRAM/OTFB, and
  wait for process done.
Input Param: 
    SE_SET_BIT_PARAM *ptSetParam: the param given to SE
Output Param: 
    none
Return Value:
    void
Usage:  
    set bit 0 or 1 in DRAM/OTFB using Search Engine
History:
20140915    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_SESetBit(SE_SET_BIT_PARAM *ptSetParam)
{
    l_pSETrigSizeReg->bsTrigger = 0;

    if (FALSE == ptSetParam->bOTFB)
    {
        HAL_SEOtfbDisable(l_ucSEID);
        rSE_BIT_SET_ADDRESS(l_ucSEID) = ptSetParam->ulSetAddr - DRAM_START_ADDRESS;
    }
    else
    {
        HAL_SEOtfbEnable(l_ucSEID);
        rSE_BIT_SET_ADDRESS(l_ucSEID) = ptSetParam->ulSetAddr - OTFB_START_ADDRESS;
    }

    rSE_BIT_SET_INDEX(l_ucSEID) = ptSetParam->usSetIndex;

    if (1 == ptSetParam->bBitValue)
    {
        l_pSETrigSizeReg->bsItemSize = SE_SET_BIT1;
    }
    else
    {
        l_pSETrigSizeReg->bsItemSize = SE_SET_BIT0;
    }

    l_pSETrigSizeReg->bsTrigger = 1;
#ifdef SIM
    SE_ModelProcess(l_ucSEID);
#endif

    HAL_DelayCycle(SE_WAIT_TIME);
    while (FALSE == l_pSEStatusReg->bBitSetDone)
    {
        /*wait set finish*/
    }

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_SESearchOverlap
Description: 
    A range of value can be defined by a start value and a length, so we can 
  build a list that every item is consist of a start value and a length,
  and use Search Engine to search if the value ranges in list overlaps with the
  key value range.
    This function trigger SE to search overlap and output the result.
Input Param: 
    SE_SEARCH_OVERLAP_ENTRY *pOverlapEntry -> ulListAddr, ulListLenQW, 
  ulKeyStartVal, ulKeyLen: the param given to SE.
    BOOL bOTFB: 0 - list is in DRAM, 1 - list is in OTFB.
Output Param: 
    SE_SEARCH_OVERLAP_ENTRY *pOverlapEntry -> aOverlapBitMap[4] : 
  bit 1 corresponding item overlaps with the key value range.
Return Value:
    void
Usage:  
    for example, check if the LBA range of new command overlaps with the LBA range
  of processing comamnd.
History:
20140915    Kristin    Coding style uniform
------------------------------------------------------------------------------*/
void HAL_SESearchOverlap(SE_SEARCH_OVERLAP_ENTRY *pOverlapEntry, BOOL bOTFB)
{
    U8 i;

    l_pSETrigSizeReg->bsTrigger = 0;

    if (FALSE == bOTFB)
    {
        HAL_SEOtfbDisable(l_ucSEID); 
        rSE_LIST_BASE_ADDRESS(l_ucSEID) = pOverlapEntry->ulListAddr - DRAM_START_ADDRESS;
    }
    else
    {
        HAL_SEOtfbEnable(l_ucSEID);
        rSE_LIST_BASE_ADDRESS(l_ucSEID) = pOverlapEntry->ulListAddr - OTFB_START_ADDRESS;
    }

    rSE_LIST_ITEM_NUM(l_ucSEID) = pOverlapEntry->ulListLenQW;
    rSE_KEY_START_VALUE(l_ucSEID) = pOverlapEntry->ulKeyStartVal;
    rSE_KEY_ACCESS_LEN(l_ucSEID) = pOverlapEntry->ulKeyLen;

    /*trigger to search range overlap*/
    l_pSETrigSizeReg->bsItemSize = SE_SEARCH_OVERLAP;
    l_pSETrigSizeReg->bsTrigger = 1;
#ifdef SIM
    SE_ModelProcess(l_ucSEID);
#endif

    HAL_DelayCycle(SE_WAIT_TIME);
    while (FALSE == l_pSEStatusReg->bOverlapDone)
    {
        /*wait search finish*/
    }

    for (i = 0; i < SE_OVERLAP_BITMAP_LEMDW; i++)
    {
        pOverlapEntry->aOverlapBitMap[i] = rSE_OVERLAP_BITMAP(l_ucSEID,i);
    }

    return;
}

/********************** FILE END ***************/

