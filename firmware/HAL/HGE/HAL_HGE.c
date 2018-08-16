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
Description :
*******************************************************************************/
#include "HAL_HGE.h"
#include "HAL_FlashChipDefine.h"

GLOBAL U32 g_ulHgeResultIndex;
GLOBAL HGE_RESULTS *g_HgeResults;

GLOBAL HGE_PATTERN_INFO g_HgePattern[] = {
    //MLC : find BR
    //pattern0[0] = '1', pattern0[3:1] = don't care
    //patternMask0[0] = '0', patternMask0[3:1] = don't care
    //pattern1~3/patternMask1~3 don't care
    {0x1,0xe,0xf,0xf,0xf,0xf,0xf,0xf},
    //MLC : find AR & CR
    //AR : pattern0[1:0] = '11', pattern0[3:2] = don't care 
    //     patternMask0[1:0] = '00', patternMask0[3:2] = don't care
    //CR : pattern1[1:0] = '00', pattern1[3:2] = don't care
    //     patternMask1[1:0] = '00', patternMask1[3:2] = don't care
    //pattern2~3/patternMask2~3 don't care
    {0x3,0xc,0x0,0xc,0xf,0xf,0xf,0xf},
    //TLC : find AR & BR & CR & DR
    //AR : pattern0[2:0] = '111', pattern0[3] = don't care 
    //     patternMask0[2:0] = '000', patternMask0[3] = don't care
    //BR : pattern1[2:0] = '011', pattern1[3] = don't care
    //     patternMask1[2:0] = '000', patternMask1[3] = don't care
    //CR : pattern2[2:0] = '001', pattern2[3] = don't care
    //     patternMask2[2:0] = '000', patternMask2[3] = don't care
    //DR : pattern3[2:0] = '000', pattern3[3] = don't care
    //     patternMask3[2:0] = '000', patternMask3[3] = don't care
    {0x7,0x8,0x3,0x8,0x1,0x8,0x0,0x8},
    //TLC : find ER & FR & GR
    //ER : pattern0[2:0] = '010', pattern0[3] = don't care 
    //     patternMask0[2:0] = '000', patternMask0[3] = don't care
    //FR : pattern1[2:0] = '110', pattern1[3] = don't care
    //     patternMask1[2:0] = '000', patternMask1[3] = don't care
    //GR : pattern2[2:0] = '100', pattern2[3] = don't care
    //     patternMask2[2:0] = '000', patternMask2[3] = don't care
    //pattern3/patternMask3 don't care
    {0x2,0x8,0x6,0x8,0x4,0x8,0xf,0xf}
};


/*------------------------------------------------------------------------------
Name: HAL_HgeCheckPatternMatchSLC
Description:
    Use FW to count data pattern match result
Input Param:

Output Param:
    none
Return Value:
    none
Usage:
History:
20160316   Henry Chen    create
------------------------------------------------------------------------------*/
void HAL_HgeCheckPatternMatchSLC(U32* ulLowPageData, U32 ulCompareSize, U32 ulRetryTime, HGE_DESC * HgeDescriptor)
{
    U8 i, j;
    U32 LowXnorResult;
    U32 Count = 0;
    U32 LowPatternBR;

    //LowPattern for BR
    if ((g_HgePattern[PATTERN_IDX_MLC_BR].ucHgePattern0&0x1) == 0x1)
        LowPatternBR = 0xffffffff;
    else
        LowPatternBR = 0x0;
    
    //count BR
    for(i = 0; i < ulCompareSize/4; i++) {
        LowXnorResult = ~((*(ulLowPageData + i))^LowPatternBR);
        for(j = 0; j < 32; j++) {
            if (((LowXnorResult>>j)&0x1) == 0x1)
                Count++;
        }
    }

    if (Count != HgeDescriptor->bsPat0AccumuResult)
    {
        DBG_Printf("HGE HW BR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat0AccumuResult,Count);
        DBG_Getch();
    }
    //DBG_Printf("Retry Time %d : BR count result = %d\n", ulRetryTime, Count);
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HgeCheckPatternMatchMLC
Description:
    Use FW to count data pattern match result
Input Param:

Output Param:
    none
Return Value:
    none
Usage:
History:
20160316   Henry Chen    create
------------------------------------------------------------------------------*/
void HAL_HgeCheckPatternMatchMLC(U32* ulLowPageData, U32* ulHighPageData, U32 ulCompareSize, U32 ulRetryTime, HGE_DESC * HgeDescriptor)
{
    U8 i, j;
    U32 LowXnorResult, HighXnorResult;
    U32 Count = 0;
    U32 LowPatternAR, HighPatternAR, LowPatternCR, HighPatternCR;
    
    //LowPattern for AR
    if ((g_HgePattern[PATTERN_IDX_MLC_AR_CR].ucHgePattern0&0x1) == 0x1)//assume bit0 is low page bit1 is high page?
        LowPatternAR = 0xffffffff;
    else
        LowPatternAR = 0x0;

    //HighPattern for AR
    if ((g_HgePattern[PATTERN_IDX_MLC_AR_CR].ucHgePattern0&0x2) == 0x2)
        HighPatternAR = 0xffffffff;
    else
        HighPatternAR = 0x0;

    //LowPattern for CR
    if ((g_HgePattern[PATTERN_IDX_MLC_AR_CR].ucHgePattern1&0x1) == 0x1)
        LowPatternCR = 0xffffffff;
    else
        LowPatternCR = 0x0;

    //HighPattern for CR
    if ((g_HgePattern[PATTERN_IDX_MLC_AR_CR].ucHgePattern1&0x2) == 0x2)
        HighPatternCR = 0xffffffff;
    else
        HighPatternCR = 0x0;

    //count AR
    for(i = 0; i < ulCompareSize/4; i++) {
        LowXnorResult = ~((*(ulLowPageData + i))^LowPatternAR);
        HighXnorResult = ~((*(ulHighPageData + i))^HighPatternAR);
        for(j = 0; j < 32; j++) {
            if ((((LowXnorResult&HighXnorResult)>>j)&0x1) == 0x1)
                Count++;
        }
    }
    //DBG_Printf("Retry Time %d : AR count result = %d\n", ulRetryTime, Count);
    if (Count != HgeDescriptor->bsPat0AccumuResult)
    {
        DBG_Printf("HGE HW AR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat0AccumuResult,Count);
        DBG_Getch();
    }
    
    //count CR
    Count = 0;
    for(i = 0; i < ulCompareSize/4; i++) {
        LowXnorResult = ~((*(ulLowPageData + i))^LowPatternCR);
        HighXnorResult = ~((*(ulHighPageData + i))^HighPatternCR);
        for(j = 0; j < 32; j++) {
            if ((((LowXnorResult&HighXnorResult)>>j)&0x1) == 0x1)
                Count++;
        }
    }
    //DBG_Printf("Retry Time %d : CR count result = %d\n", ulRetryTime, Count);
    if (Count != HgeDescriptor->bsPat1AccumuResult)
    {
        DBG_Printf("HGE HW CR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat1AccumuResult,Count);
        DBG_Getch();
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HgeCheckPatternMatchTLC
Description:
    Use FW to count data pattern match result
Input Param:
    U32* ulLowPageData
    U32* ulMidPageData
    U32* ulHighPageData
    U32 ulCompareSize
    U32 ulRetryTime
    U8 ucIndex
Output Param:
    none
Return Value:
    none
Usage:
History:
20160316   Henry Chen    create
------------------------------------------------------------------------------*/
void HAL_HgeCheckPatternMatchTLC(U32* ulLowPageData, U32* ulMidPageData, U32* ulHighPageData, U32 ulCompareSize, U32 ulRetryTime, U8 ucIndex, HGE_DESC * HgeDescriptor)
{
    U8 i, j;
    U32 Count = 0;
    U32 LowXnorResult, MidXnorResult, HighXnorResult;
    U32 LowPatternAR, MidPatternAR, HighPatternAR;
    U32 LowPatternBR, MidPatternBR, HighPatternBR;
    U32 LowPatternCR, MidPatternCR, HighPatternCR;
    U32 LowPatternDR, MidPatternDR, HighPatternDR;
    U32 LowPatternER, MidPatternER, HighPatternER;
    U32 LowPatternFR, MidPatternFR, HighPatternFR;
    U32 LowPatternGR, MidPatternGR, HighPatternGR;

    if (ucIndex == 0) {
        //LowPattern for AR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern0&0x1) == 0x1)
            LowPatternAR = 0xffffffff;
        else
            LowPatternAR = 0x0;

        //MidPattern for AR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern0&0x2) == 0x2)
            MidPatternAR = 0xffffffff;
        else
            MidPatternAR = 0x0;
        
        //HighPattern for AR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern0&0x4) == 0x4)
            HighPatternAR = 0xffffffff;
        else
            HighPatternAR = 0x0;

        //LowPattern for BR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern1&0x1) == 0x1)
            LowPatternBR = 0xffffffff;
        else
            LowPatternBR = 0x0;

        //MidPattern for BR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern1&0x2) == 0x2)
            MidPatternBR = 0xffffffff;
        else
            MidPatternBR = 0x0;
        
        //HighPattern for BR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern1&0x4) == 0x4)
            HighPatternBR = 0xffffffff;
        else
            HighPatternBR = 0x0;

        //LowPattern for CR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern2&0x1) == 0x1)
            LowPatternCR = 0xffffffff;
        else
            LowPatternCR = 0x0;

        //MidPattern for CR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern2&0x2) == 0x2)
            MidPatternCR = 0xffffffff;
        else
            MidPatternCR = 0x0;
        
        //HighPattern for CR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern2&0x4) == 0x4)
            HighPatternCR = 0xffffffff;
        else
            HighPatternCR = 0x0;

        //LowPattern for DR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern3&0x1) == 0x1)
            LowPatternDR = 0xffffffff;
        else
            LowPatternDR = 0x0;

        //MidPattern for DR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern3&0x2) == 0x2)
            MidPatternDR = 0xffffffff;
        else
            MidPatternDR = 0x0;
        
        //HighPattern for DR
        if ((g_HgePattern[PATTERN_IDX_TLC_AR_BR_CR_DR].ucHgePattern3&0x4) == 0x4)
            HighPatternDR = 0xffffffff;
        else
            HighPatternDR = 0x0;

        //count AR
        for(i = 0; i < ulCompareSize/4; i++) 
        {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternAR);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternAR);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternAR);
            for(j = 0; j < 32; j++) 
            {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat0AccumuResult)
        {
            DBG_Printf("HGE HW TLC AR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat0AccumuResult,Count);
            DBG_Getch();
        }
        
        //count BR
        Count = 0;
        for(i = 0; i < ulCompareSize/4; i++) 
        {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternBR);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternBR);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternBR);
            for(j = 0; j < 32; j++) 
            {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat1AccumuResult)
        {
            DBG_Printf("HGE HW TLC BR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat1AccumuResult,Count);
            DBG_Getch();
        }

        //count CR
        Count = 0;
        for(i = 0; i < ulCompareSize/4; i++) {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternCR);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternCR);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternCR);
            for(j = 0; j < 32; j++) {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat2AccumuResult)
        {
            DBG_Printf("HGE HW TLC CR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat2AccumuResult,Count);
            DBG_Getch();
        }

        //count DR
        Count = 0;
        for(i = 0; i < ulCompareSize/4; i++) {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternDR);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternDR);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternDR);
            for(j = 0; j < 32; j++) {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat3AccumuResult)
        {
            DBG_Printf("HGE HW TLC DR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat3AccumuResult,Count);
            DBG_Getch();
        }
    } 
    else if (ucIndex == 1) 
    {
        //LowPattern for ER
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern0&0x1) == 0x1)
            LowPatternER = 0xffffffff;
        else
            LowPatternER = 0x0;

        //MidPattern for ER
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern0&0x2) == 0x2)
            MidPatternER = 0xffffffff;
        else
            MidPatternER = 0x0;
        
        //HighPattern for ER
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern0&0x4) == 0x4)
            HighPatternER = 0xffffffff;
        else
            HighPatternER = 0x0;

        //LowPattern for FR
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern1&0x1) == 0x1)
            LowPatternFR = 0xffffffff;
        else
            LowPatternFR = 0x0;

        //MidPattern for FR
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern1&0x2) == 0x2)
            MidPatternFR = 0xffffffff;
        else
            MidPatternFR = 0x0;
        
        //HighPattern for FR
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern1&0x4) == 0x4)
            HighPatternFR = 0xffffffff;
        else
            HighPatternFR = 0x0;

        //LowPattern for GR
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern2&0x1) == 0x1)
            LowPatternGR = 0xffffffff;
        else
            LowPatternGR = 0x0;

        //MidPattern for GR
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern2&0x2) == 0x2)
            MidPatternGR = 0xffffffff;
        else
            MidPatternGR = 0x0;
        
        //HighPattern for GR
        if ((g_HgePattern[PATTERN_IDX_TLC_ER_FR_GR].ucHgePattern2&0x4) == 0x4)
            HighPatternGR = 0xffffffff;
        else
            HighPatternGR = 0x0;

        //count ER
        for(i = 0; i < ulCompareSize/4; i++) 
        {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternER);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternER);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternER);
            for(j = 0; j < 32; j++) 
            {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat0AccumuResult)
        {
            DBG_Printf("HGE HW TLC ER calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat0AccumuResult,Count);
            DBG_Getch();
        }
        
        //count FR
        Count = 0;
        for(i = 0; i < ulCompareSize/4; i++) 
        {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternFR);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternFR);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternFR);
            for(j = 0; j < 32; j++) 
            {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat1AccumuResult)
        {
            DBG_Printf("HGE HW TLC FR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat1AccumuResult,Count);
            DBG_Getch();
        }

        //count GR
        Count = 0;
        for(i = 0; i < ulCompareSize/4; i++) {
            LowXnorResult = ~((*(ulLowPageData + i))^LowPatternGR);
            MidXnorResult = ~((*(ulMidPageData + i))^MidPatternGR);
            HighXnorResult = ~((*(ulHighPageData + i))^HighPatternGR);
            for(j = 0; j < 32; j++) 
            {
                if (((((LowXnorResult&MidXnorResult)&HighXnorResult)>>j)&0x1) == 0x1)
                    Count++;
            }
        }
        if (Count != HgeDescriptor->bsPat2AccumuResult)
        {
            DBG_Printf("HGE HW TLC GR calculating results dis-match with FW results! HW: %d, FW: %d\n",HgeDescriptor->bsPat2AccumuResult,Count);
            DBG_Getch();
        }
    } 
    else 
    {
        DBG_Printf("[HAL_HgeCheckPatternMatchTLC] Wrong ucIndex.\n");
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HgeDsAlloc
Description:
Alloc the histogram related Data Structure
Input Param:
U32 *pDramBase: Valid Dram base
Output Param:
none
Return Value:
none
Usage:
called before using histogram   (This feature should move to L3_task dram DS init in the future!)
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeDsAlloc(U32 *pDramBase) {

    U32 ulFreeDramBase;
    
    ulFreeDramBase = *pDramBase;
    
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    //g_HgeResults = (HGE_RESULTS *)ulFreeDramBase;
    g_HgeResults = (HGE_RESULTS *)0x40100000;
    
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS)));

    COM_MemAddr16DWAlign(&ulFreeDramBase);
    
    *pDramBase = ulFreeDramBase;
    
    return;
}



/*------------------------------------------------------------------------------
Name: HAL_HgeRegInit
Description:
Init the histogram engine
Input Param:
none
Output Param:
none
Return Value:
none
Usage:
called before using histogram
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeRegInit(void) {

    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;
    
    pHgeReg->ulReadDescAddrBase1 = HGE_DES_ADDR_BASE1;
    pHgeReg->ulReadDescAddrBase2 = HGE_DES_ADDR_BASE2;

    pHgeReg->bsRptr1 = pHgeReg->bsRptr2 = 0;
    pHgeReg->bsWptr1 = pHgeReg->bsWptr2 = 0;

    pHgeReg->bsEntrySize = HGE_ENTRY_MAX;   //Depend on reserved SRAM space for HGE descriptor

    g_ulHgeResultIndex = 0;
    COM_MemZero((U32*)g_HgeResults, (RETRY_TIME_MAX*VT_MAX*sizeof(HGE_DES_RESULTS))>>2);

    //memset zero on descriptors
    COM_MemZero((U32*)HGE_DES_ADDR_BASE1, (sizeof(HGE_DESC)*HGE_ENTRY_MAX) >> 2);
    COM_MemZero((U32*)HGE_DES_ADDR_BASE2, (sizeof(HGE_DESC)*HGE_ENTRY_MAX) >> 2);
    
    //enable Interrupt if need

    return;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeRegReset
Description:
reset the histogram engine
Input Param:
none
Output Param:
none
Return Value:
none
Usage:
called when histogram go wrong
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeRegReset(void) {

    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;

    pHgeReg->bsHgeRst = 1;

    return;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeCheckIdle
Description:
check whether the histogram is idle
Input Param:
U32 ulMcuId: MCU id
Output Param:
none
Return Value:
BOOL:   1: idle
0: not idle
Usage:
called when check histogram idle
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
BOOL HAL_HgeCheckIdle(U32 ulMcuId) {

    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;

    if (ulMcuId == MCU1_ID)
        return ((BOOL)(pHgeReg->bsWptr1 == pHgeReg->bsRptr1));
    else if (ulMcuId == MCU2_ID)
        return ((BOOL)(pHgeReg->bsWptr2 == pHgeReg->bsRptr2));
    else {
        DBG_Printf("[HAL_HgeCheckIdle]Wrong Mcu Id (%d)!\n", ulMcuId);
        DBG_Getch();
    }   

    return FALSE;
}


/*------------------------------------------------------------------------------
Name: GetValidDescriptor
Description:
Find the valid descriptor address
Input Param:
U32 ulMcuId: MCU id
Output Param:
none
Return Value:
U32:    Address of the valid descriptor
NULL:   Fail
Usage:
called when need a histogram descriptor
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
U32 HAL_HgeGetValidDescriptor(U32 ulMcuId) {

    HGE_DES_ID DesId;
    HGE_DESC *HgeDescriptor;
    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;

    DesId.ulDescMcu = ulMcuId;

    if (ulMcuId == MCU1_ID) {
        DesId.ulDescID = pHgeReg->bsWptr1;
        HgeDescriptor = (HGE_DESC *)HAL_HgeGetDescriptorAddress(DesId);
        return (HgeDescriptor->bsValidBit ? NULL : (U32)HgeDescriptor);
    }
    else if (ulMcuId == MCU2_ID) {
        DesId.ulDescID = pHgeReg->bsWptr2;
        HgeDescriptor = (HGE_DESC *)HAL_HgeGetDescriptorAddress(DesId);
        return (HgeDescriptor->bsValidBit ? NULL : (U32)HgeDescriptor);
    }
    else
        return NULL;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeGetCurrentWp
Description:
Find the current Wp
Input Param:
U32 ulMcuId: MCU id
Output Param:
none
Return Value:
U8: current Wp
NULL:   Fail
Usage:
called when need a valid histogram descriptor id
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
U8 HAL_HgeGetCurrentWp(U32 ulMcuId) 
{
    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;

    if (ulMcuId == MCU1_ID)
        return pHgeReg->bsWptr1;
    else if (ulMcuId == MCU2_ID)
        return pHgeReg->bsWptr2;
    else {
        DBG_Printf("[HAL_HgeGetCurrentWp] Wrong mcu id.");
        return NULL;
    }
}



/*------------------------------------------------------------------------------
Name: GetValidDescriptorId
Description:
Find the valid descriptor id
Input Param:
U32 ulMcuId: MCU id
Output Param:
none
Return Value:
U32:    valid descriptor id
NULL:   Fail
Usage:
called when need a histogram descriptor
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
U32 HAL_HgeGetValidDescriptorId(U32 ulMcuId) {

    HGE_DES_ID DesId;
    HGE_DESC *HgeDescriptor;
    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;

    DesId.ulDescMcu = ulMcuId;

    if (ulMcuId == MCU1_ID) {
        DesId.ulDescID = pHgeReg->bsWptr1;
        HgeDescriptor = (HGE_DESC *)HAL_HgeGetDescriptorAddress(DesId);
        return (HgeDescriptor->bsValidBit ? NULL : (U32)HgeDescriptor);
    }
    else if (ulMcuId == MCU2_ID) {
        DesId.ulDescID = pHgeReg->bsWptr2;
        HgeDescriptor = (HGE_DESC *)HAL_HgeGetDescriptorAddress(DesId);
        return (HgeDescriptor->bsValidBit ? NULL : (U32)HgeDescriptor);
    }
    else
        return NULL;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeGetDescriptorAddress
Description:
Convert the descriptor to memory address
Input Param:
HGE_DES_ID DesId: descriptor id
Output Param:
none
Return Value:
U32:    Address of the input descriptor id
Usage:
called when need a histogram descriptor
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
U32 HAL_HgeGetDescriptorAddress(HGE_DES_ID DesId) {
    if (DesId.ulDescMcu == MCU1_ID)
        return HGE_DES_ADDR_BASE1 + DesId.ulDescID*sizeof(HGE_DESC);
    else if (DesId.ulDescMcu == MCU2_ID)
        return HGE_DES_ADDR_BASE2 + DesId.ulDescID*sizeof(HGE_DESC);
    else {
        DBG_Printf("[HAL_HgeGetDescriptorAddress]Wrong Mcu Id (%d)!\n", DesId.ulDescMcu);
        DBG_Getch();
    }

    return NULL;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeSetDescriptor
Description:
set a histogram descriptor
Input Param:
HGE_DES_ID DesId: A valid descriptor id
U8 ucPattern_idx: The index of pattern to set
HGE_READ_ADDRESS_INFO ReadAddrInfo: read address of data for hge
U8 ucRdNum: Depend on finding BR or AR&CR in MLC
Output Param:
none
Return Value:
none
Usage:
called when need to set a histogram descriptor
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeSetDescriptor(HGE_DESC *HgeDescriptor, U8 ucPattern_idx, HGE_READ_ADDRESS_INFO ReadAddrInfo, U8 ucRdNum, U32 ulSize) {

    HGE_PATTERN_INFO PatternInfo = g_HgePattern[ucPattern_idx];

    HgeDescriptor->bsValidBit = 1;
    HgeDescriptor->bsRdNum = ucRdNum;
    HgeDescriptor->bsRdLength = ulSize/sizeof(QWORD);

    //Need 8DW alignment because of HW design.
    if( (HgeDescriptor->bsRdLength & HGE_QW_ALIGN_BITS) != 0)
    {
        HgeDescriptor->bsRdLength = (HgeDescriptor->bsRdLength & (~HGE_QW_ALIGN_BITS));
    }
    
    //set pattern
    HgeDescriptor->bsPattern0 = PatternInfo.ucHgePattern0;
    HgeDescriptor->bsPatternMask0 = PatternInfo.ucHgePatternMask0;
    HgeDescriptor->bsPattern1 = PatternInfo.ucHgePattern1;
    HgeDescriptor->bsPatternMask1 = PatternInfo.ucHgePatternMask1;
    HgeDescriptor->bsPattern2 = PatternInfo.ucHgePattern2;
    HgeDescriptor->bsPatternMask2 = PatternInfo.ucHgePatternMask2;
    HgeDescriptor->bsPattern3 = PatternInfo.ucHgePattern3;
    HgeDescriptor->bsPatternMask3 = PatternInfo.ucHgePatternMask3;

    //set read address
    HgeDescriptor->ulR0Addr = ReadAddrInfo.ulHgeReadAddr0 - DRAM_START_ADDRESS;
    HgeDescriptor->ulR1Addr = ReadAddrInfo.ulHgeReadAddr1 - DRAM_START_ADDRESS;
    HgeDescriptor->ulR2Addr = ReadAddrInfo.ulHgeReadAddr2 - DRAM_START_ADDRESS;
    HgeDescriptor->ulR3Addr = ReadAddrInfo.ulHgeReadAddr3 - DRAM_START_ADDRESS;

    return;
}




/*------------------------------------------------------------------------------
Name: HAL_HgeRegTrigger
Description:
trigger the Hge engine
Input Param:
HGE_DES_ID DesId: descriptor id
Output Param:
none
Return Value:
none
Usage:
called after histogram reg & descriptor are all set
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeRegTrigger(U32 McuId) {
    volatile HISTOGRAM_REG *pHgeReg = (volatile HISTOGRAM_REG *)REG_BASE_EXT_HGE;

    if (McuId == MCU1_ID)
        pHgeReg->bsWptr1 = (pHgeReg->bsWptr1 == (HGE_ENTRY_MAX-1)) ? 0 : (pHgeReg->bsWptr1 + 1);//HGE_ENTRY_MAX
    else if (McuId == MCU2_ID)
        pHgeReg->bsWptr2 = (pHgeReg->bsWptr2 == (HGE_ENTRY_MAX-1)) ? 0 : (pHgeReg->bsWptr2 + 1);
    else {
        DBG_Printf("[HAL_HgeRegTrigger]Wrong Mcu Id (%d)!\n", McuId);
        DBG_Getch();
    }   

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_HgeWaitDescriptorDone
Description:
wait until the Hge engine finish the job of the specific Descriptor
Input Param:
HGE_DES_ID DesId: descriptor id
Output Param:
none
Return Value:
none
Usage:
called after trigger hge engine
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeWaitDescriptorDone(volatile HGE_DESC *HgeDescriptor) 
{
    while (HgeDescriptor->bsValidBit == 1)
    {
        ;
    }

    return;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeSetReadAddrInfo
Description:
Set ReadAddrInfo
Input Param:
HGE_READ_ADDRESS_INFO *ReadAddrInfo : Destination address
U32 AddrInfo    : Source read address
U32 index   : set AddrInfo to ReadAddrInfo by referencing this index

Output Param:
none
Return Value:
none
Usage:
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeSetReadAddrInfo(HGE_READ_ADDRESS_INFO *ReadAddrInfo, U32 AddrInfo, U32 index) {
    
    switch(index) {
        case 0:
            ReadAddrInfo->ulHgeReadAddr0 = AddrInfo;
            break;
        case 1:
            ReadAddrInfo->ulHgeReadAddr1 = AddrInfo;
            break;
        case 2:
            ReadAddrInfo->ulHgeReadAddr2 = AddrInfo;
            break;
        case 3:
            ReadAddrInfo->ulHgeReadAddr3 = AddrInfo;
            break;
        default:
            DBG_Printf("[HAL_HgeSetReadAddrInfo]No such index!(%d)\n", index);
            break;
    }
    
    return;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeGetDescriptorResultsMlc
Description:
get the results from the specific descriptor
Input Param:
HGE_DES_ID DesId: descriptor id
Output Param:
none
Return Value:
none
Usage:
called after Hge engine finish the job of the specific Descriptor
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeGetDescriptorResultsMlc(HGE_DESC *HgeDescriptor, U8 ucVtValue) {

    U8 i;

    //HAL_HgeWaitDescriptorDone(HgeDescriptor);

    for (i = 0; i < HgeDescriptor->bsRdNum + 1; i++) {
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + i].ucVtValue = ucVtValue;//ucRetryTime
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + i].ulResults = (HgeDescriptor->ulDW[6 + i] & 0x3FFFF);
    }

    switch (HgeDescriptor->bsRdNum) {
        case 0:
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex].ucHgeVt = BR;
            break;
        case 1:
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex].ucHgeVt = AR;
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + 1].ucHgeVt = CR;
            break;
        default:
            DBG_Printf("HAL_HgeGetDescriptorResults fail\n");
            break;
    }

    i = 0;
    while (i < HgeDescriptor->bsRdNum + 1) {
        if (g_ulHgeResultIndex == (RETRY_TIME_MAX*VT_MAX))//is   RETRY_TIME_MAX*VT_MAX   OK?
            g_ulHgeResultIndex = 0;
        else
            g_ulHgeResultIndex++;
        i++;
    }

    return;    
}


/*------------------------------------------------------------------------------
Name: HAL_HgeGetDescriptorResultsTlc
Description:
get the results from the specific descriptor
Input Param:
    HGE_DESC *HgeDescriptor : 
    U8 ucVtValue : 
    U8 ucIndex : 0:first AR BR CR DR; 1:second ER FR GR
Output Param:
none
Return Value:
none
Usage:
called after Hge engine finish the job of the specific Descriptor
History:
20160323        Henry Chen      first version
------------------------------------------------------------------------------*/
void HAL_HgeGetDescriptorResultsTlc(HGE_DESC *HgeDescriptor, U8 ucVtValue, U8 ucIndex) {

    U8 i;

    if (ucIndex == 0) {//first
        for (i = 0; i < 4; i++) {
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + i].ucVtValue = ucVtValue;
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + i].ulResults = (HgeDescriptor->ulDW[6 + i] & 0x3FFFF);
        }

        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex].ucHgeVt = AR;
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + 1].ucHgeVt = BR;
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + 2].ucHgeVt = CR;
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + 3].ucHgeVt = DR;

        i = 0;
        while (i < 4) {
            if (g_ulHgeResultIndex == (RETRY_TIME_MAX*VT_MAX))//is   RETRY_TIME_MAX*VT_MAX   OK?
                g_ulHgeResultIndex = 0;
            else
                g_ulHgeResultIndex++;
            i++;
        }
    } else if (ucIndex == 1) {//second
        for (i = 0; i < 3; i++) {
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + i].ucVtValue = ucVtValue;
            g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + i].ulResults = (HgeDescriptor->ulDW[6 + i] & 0x3FFFF);
        }

        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex].ucHgeVt = ER;
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + 1].ucHgeVt = FR;
        g_HgeResults->ulHgeDesResults[g_ulHgeResultIndex + 2].ucHgeVt = GR;

        i = 0;
        while (i < 3) {
            if (g_ulHgeResultIndex == (RETRY_TIME_MAX*VT_MAX))//is   RETRY_TIME_MAX*VT_MAX   OK?
                g_ulHgeResultIndex = 0;
            else
                g_ulHgeResultIndex++;
            i++;
        }
    } else {
        DBG_Printf("HAL_HgeGetDescriptorResults fail\n");
        return;
    }
    
    return;
}


void HAL_PrintHgeResults(void)
{
    U8 i;
    for (i = 0; i < (RETRY_TIME_MAX*VT_MAX); i++) {
    DBG_Printf("%d: DescID %d, RetryTime %d, AR/BR/CR=%d, Results=%d\n",
        i,
        g_HgeResults->ulHgeDesResults[i].ulDescID.ulDescID,
        g_HgeResults->ulHgeDesResults[i].ucVtValue,
        g_HgeResults->ulHgeDesResults[i].ucHgeVt,
        g_HgeResults->ulHgeDesResults[i].ulResults);
    }

    return;
}


/*------------------------------------------------------------------------------
Name: HAL_HgeDataAnalysis
Description:
    Data Analysis and return the best Vt retry time
    The read Vt in g_HgeResults must be increasing.
Input Param:
    U8 ucType : AR,BR,CR...type of Vt
Output Param:
    none
Return Value:
    U32 : the best Vt retry time
Usage:
    called after all hitogram jobs finished
History:
20151231        Henry Chen      first version
------------------------------------------------------------------------------*/
U8 HAL_HgeDataAnalysis(U8 ucType)
{
    U8 ReturnValue = 0, First = 0;
    U32 i, TmpResultIdx = 0, TmpResult = INVALID_8F;
    HGE_FIN_RESULTS FinHgeResult[RETRY_TIME_MAX-1];
        
    //Move from  g_HgeResults to tmp array
    for (i = 0; i < (RETRY_TIME_MAX*VT_MAX); i++) 
    {
        if (g_HgeResults->ulHgeDesResults[i].ucHgeVt == ucType) 
        {
            if (!First) 
            {//First move to results
                FinHgeResult[TmpResultIdx].ulResults = g_HgeResults->ulHgeDesResults[i].ulResults;
                FinHgeResult[TmpResultIdx].ucVtValueA = g_HgeResults->ulHgeDesResults[i].ucVtValue;//RetryTime
                FinHgeResult[TmpResultIdx].ucDone = 0;//not done
                First++;
            } 
            else 
            {//Do substraction in results
                if (FinHgeResult[TmpResultIdx].ulResults > g_HgeResults->ulHgeDesResults[i].ulResults)
                    FinHgeResult[TmpResultIdx].ulResults = FinHgeResult[TmpResultIdx].ulResults - g_HgeResults->ulHgeDesResults[i].ulResults;
                else
                    FinHgeResult[TmpResultIdx].ulResults = g_HgeResults->ulHgeDesResults[i].ulResults - FinHgeResult[TmpResultIdx].ulResults;

                FinHgeResult[TmpResultIdx].ucVtValueB = g_HgeResults->ulHgeDesResults[i].ucVtValue;
                FinHgeResult[TmpResultIdx].ucDone = 1;//done for current TmpResultIdx
                
                TmpResultIdx++;//prepare next
                FinHgeResult[TmpResultIdx].ulResults = g_HgeResults->ulHgeDesResults[i].ulResults;//First move to results
                FinHgeResult[TmpResultIdx].ucVtValueA = g_HgeResults->ulHgeDesResults[i].ucVtValue;
                FinHgeResult[TmpResultIdx].ucDone = 0;//not done for next TmpResultIdx
            }
        }
    }

    //Do calculation
    for (i = 0; i < TmpResultIdx; i++) 
    {
        if (FinHgeResult[i].ulResults < TmpResult && FinHgeResult[i].ucDone == 1) 
        {
            TmpResult = FinHgeResult[i].ulResults;
            ReturnValue = FinHgeResult[i].ucVtValueA;//A and B are both OK.
        }
    }
    
    return ReturnValue;
}



