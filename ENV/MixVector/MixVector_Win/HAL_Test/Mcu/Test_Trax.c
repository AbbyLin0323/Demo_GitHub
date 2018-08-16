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
 * File Name    : TEST_Trax.c
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "BaseDef.h"
#include "HAL_Trax.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "Test_McuBasicFunc.h"
#include <xtensa/tie/xt_externalregisters.h>


#define rOTFBTracer_MCU0  (*((volatile U32*)(OTFB_START_ADDRESS+0x0)))
#define rOTFBTracer_MCU1  (*((volatile U32*)(OTFB_START_ADDRESS+0x4)))
#define rOTFBTracer_MCU2  (*((volatile U32*)(OTFB_START_ADDRESS+0x8)))

typedef void (*FunPointer)(void);

NOINL_ATTR void foo (void)
{
    volatile U8 ucIndex;
    for (ucIndex = 0; ucIndex < 100; ucIndex++)
    {
        ;
        if(ucIndex == 72)
        {
            break;
        }
    }
    return;
}

void foo1 (void)
{
    U8 ucIndex;
    for (ucIndex = 0; ucIndex < 100; ucIndex++)
    {
        ;
        if(ucIndex == 98)
        {
            break;
        }
    }
    return;
}

LOCAL void HalSetMutiCoreMode(void)
{
    rGlbMCUSramMap = 0x0;
    return;
}

LOCAL void HalBootMcu12FromIsram(void)
{
    //configure mcu1/2 boot from isram
    rGlbMCUCtrl = 0x0;
    rGlbMCUCtrl |= 0x110;
    
    //enable mcu1/2 boot up
    rGlbMcuSgeRst = 0x0;
    return;
}

void Test_TraxMain(void)
{
    U16 usStartAddr;
    U16 usEndAddr;
    U32 ulTraceRamSize;
    U16 usDelayCount;
    U8 ucUnitType;
    U32 ulPCStartAddr;
    U32 ulPCEndAddr;
    BOOL bInvert;
    TRAX_CONTEXT tTraxContext;
    U32 ulMcuID;
  
    ulMcuID = HAL_GetMcuId();
    rTracer = 0x11;
    HAL_ConfigTraceBuffSingleMcu(ulMcuID);

    usStartAddr = 0x0;
    usEndAddr = 0x0;
    usDelayCount = 144;
    ucUnitType = 0;

    HAL_TraxInitERI(&tTraxContext);

    rTracer = 0x22;
    ulTraceRamSize = HAL_TraxGetTraceRamSize();
    if (TRACERAMSIZE < ulTraceRamSize)
    {
        DBG_Printf("RamSize 0x%x\n", ulTraceRamSize);
        DBG_Getch();
    }

    /* logic designed TraceBuff is 8k */
    ulTraceRamSize = TRACE_RAM_SIZE_SINGLECORE;//8<<10;
    usEndAddr = (ulTraceRamSize>>2) - 1;

    rTracer = usStartAddr;
    rTracer = usEndAddr;
    HAL_TraxSetRamBoundary(&tTraxContext, usStartAddr, usEndAddr);
    usStartAddr = 0xff;
    usEndAddr = 0xff;
    HAL_TraxGetRamBoundary(&usStartAddr, &usEndAddr);
    DBG_Printf("Ram Boundary Start 0x%x; End 0x%x\n", usStartAddr, usEndAddr);
    rTracer = usStartAddr;
    rTracer = usEndAddr;
    rTracer = 0x33;

    HAL_TraxSetSMPER(ulTraceRamSize);

    HAL_TraxSetPostSize(&tTraxContext, usDelayCount, ucUnitType);
    usDelayCount = 0;
    ucUnitType = 0xff;
    HAL_TraxGetPostSize(&usDelayCount, &ucUnitType);
    DBG_Printf("PostSize DelayCount 0x%x; ucUnitType 0x%x\n", usDelayCount, ucUnitType);
    rTracer = usDelayCount;
    rTracer = ucUnitType;
    rTracer = 0x44;

    HAL_TraxSetPTIStop();
    if (FALSE == HAL_TraxGetPTIStop())
    {
        DBG_Printf("PTIEN setting error!!\n");
    }
    rTracer = 0x55;

    HAL_TraxSetPCStop((U32)(&foo), (U32)(&foo) + 3, FALSE);
    HAL_TraxGetPCStop(&ulPCStartAddr, &ulPCEndAddr, &bInvert);
    DBG_Printf("PC Stop StartAddr 0x%x, EndAddr 0x%x, bInvert 0x%x, \n", ulPCStartAddr, ulPCEndAddr, bInvert);
    rTracer =ulPCStartAddr;
    rTracer = ulPCEndAddr;
    rTracer = bInvert;
    rTracer = 0x66;

    HAL_TraxStart();
    rTracer = 0x77;
    foo1();
    rTracer = 0xff;

    foo();

    rTracer = XT_RER(TRAX_ID);
    rTracer = XT_RER(TRAX_CTRL);
    rTracer = XT_RER(TRAX_STAT);
    rTracer = XT_RER(TRAX_TRIGGERPC);
    rTracer = XT_RER(TRAX_PCMATCHCTRL);
    rTracer = HAL_GetDELAYCOUNT();
    rTracer = HAL_GetMEMSTARTADDR();
    rTracer = HAL_GetMEMENDADDR();

    while(TRUE == HAL_TraxIsTraceActive())
    {
        rTracer = 0x0;
        rTracer = 0xffff;
    }

    HAL_TraxGetTrace(&tTraxContext, (U8 *)DRAM_START_ADDRESS);
    while(1);

    return;
}

void Test_SetTracer(U32 ulMcuId, U8 ucVal)
{
    if (MCU0_ID == ulMcuId)
    {
        rTracer_Mcu0 = ucVal;
    }
    else if (MCU1_ID == ulMcuId)
    {
        rTracer_Mcu1 = ucVal;
    }
    else
    {
        rTracer_Mcu2 = ucVal;
    }

    return;
}

void Test_TraxRecordVal(U32 ulMcuId, U32 ulVal)
{
    if (MCU0_ID == ulMcuId)
    {
        rOTFBTracer_MCU0 = ulVal;
    }
    else if (MCU1_ID == ulMcuId)
    {
        rOTFBTracer_MCU1 = ulVal;
    }
    else
    {
        rOTFBTracer_MCU2 = ulVal;
    }

    return;
}

void Test_DoTriggerStop(U32 ulMcuId, TRAX_CONTEXT * pContext)
{
    FunPointer pChangePC;
    if (MCU0_ID == ulMcuId)
    {
        pChangePC = (FunPointer)0x20002000;
        pChangePC();      
    }
    else if (MCU1_ID == ulMcuId)
    {
        foo();
    }
    else
    {
        HAL_TraxStopHalt();
    }

    return;
}

void Test_CaclTracBuffRange(U32 ulMcuId, U16 usRamSizeByte, U16 * pStartAddr, U16 *pEndAddr)
{
    U16 usSizDw;

    usSizDw = usRamSizeByte>>2;
    
    *pStartAddr = (ulMcuId - MCU0_ID)*usSizDw;
    *pEndAddr = (*pStartAddr + usSizDw) - 1;

    return;
}

U8 * Test_AllocTraxFileBuff(U32 ulMcuId)
{
    U8 * PTraxFile;
    U32 ulOffset;

    ulOffset = (ulMcuId - MCU0_ID) * (TRACE_RAM_SIZE_MULTICORE + sizeof(TRAX_FILE_HEAD));
    PTraxFile = (U8 *)(SRAM1_START_ADDRESS + 0x8000 + ulOffset);

    return PTraxFile;
}

void Test_Trax3Core(U32 ulMcuId)
{
    U16 usStartAddr;
    U16 usEndAddr;
    U32 ulTraceRamSize;
    U16 usDelayCount;
    U8 ucUnitType;
    U32 ulPCStartAddr;
    U32 ulPCEndAddr;
    BOOL bInvert;
    U8 * pTraxFileBuff;
    TRAX_CONTEXT tTraxContext;
    
    usDelayCount = 0;
    ucUnitType = 0;

    Test_SetTracer(ulMcuId, 0x11);

    HAL_TraxInitERI(&tTraxContext);

    Test_SetTracer(ulMcuId, 0x22);
    
    /* logic designed TraceBuff is 8k or 32k */
    ulTraceRamSize = TRACE_RAM_SIZE_MULTICORE;//8<<10;
    Test_CaclTracBuffRange(ulMcuId, ulTraceRamSize, &usStartAddr, &usEndAddr);

    Test_TraxRecordVal(ulMcuId, usStartAddr);
    Test_TraxRecordVal(ulMcuId, usEndAddr);

    HAL_TraxSetRamBoundary(&tTraxContext, usStartAddr, usEndAddr);
    usStartAddr = 0xff;
    usEndAddr = 0xff;
    HAL_TraxGetRamBoundary(&usStartAddr, &usEndAddr);
    DBG_Printf("Ram Boundary Start 0x%x; End 0x%x\n", usStartAddr, usEndAddr);
    
    Test_TraxRecordVal(ulMcuId, usStartAddr);
    Test_TraxRecordVal(ulMcuId, usEndAddr);
    Test_SetTracer(ulMcuId, 0x33);

    HAL_TraxSetSMPER(ulTraceRamSize);

    HAL_TraxSetPostSize(&tTraxContext, usDelayCount, ucUnitType);
    usDelayCount = 0;
    ucUnitType = 0xff;
    HAL_TraxGetPostSize(&usDelayCount, &ucUnitType);
    DBG_Printf("PostSize DelayCount 0x%x; ucUnitType 0x%x\n", usDelayCount, ucUnitType);

    Test_TraxRecordVal(ulMcuId, usDelayCount);
    Test_TraxRecordVal(ulMcuId, ucUnitType);
    Test_SetTracer(ulMcuId, 0x44);

    HAL_TraxSetPTIStop();
    if (FALSE == HAL_TraxGetPTIStop())
    {
        DBG_Printf("PTIEN setting error!!\n");
    }
    Test_SetTracer(ulMcuId, 0x55);

    HAL_TraxSetPCStop((U32)(&foo), (U32)(&foo) + 3, FALSE);
    HAL_TraxGetPCStop(&ulPCStartAddr, &ulPCEndAddr, &bInvert);
    DBG_Printf("PC Stop StartAddr 0x%x, EndAddr 0x%x, bInvert 0x%x \n", ulPCStartAddr, ulPCEndAddr, bInvert);

    Test_TraxRecordVal(ulMcuId, ulPCStartAddr);
    Test_TraxRecordVal(ulMcuId, ulPCEndAddr);
    Test_TraxRecordVal(ulMcuId, bInvert);
    Test_SetTracer(ulMcuId, 0x66);

    HAL_TraxStart();
    Test_SetTracer(ulMcuId, 0x77);
    
    foo1();
    Test_SetTracer(ulMcuId, 0x88);
    Test_DoTriggerStop(ulMcuId, &tTraxContext);
    
    Test_TraxRecordVal(ulMcuId, XT_RER(TRAX_ID));
    Test_TraxRecordVal(ulMcuId, XT_RER(TRAX_CTRL));
    Test_TraxRecordVal(ulMcuId, XT_RER(TRAX_STAT));
    Test_TraxRecordVal(ulMcuId, XT_RER(TRAX_TRIGGERPC));
    Test_TraxRecordVal(ulMcuId, XT_RER(TRAX_PCMATCHCTRL));
    Test_TraxRecordVal(ulMcuId, HAL_GetDELAYCOUNT());
    Test_TraxRecordVal(ulMcuId, HAL_GetMEMSTARTADDR());
    Test_TraxRecordVal(ulMcuId, HAL_GetMEMENDADDR());

    while(TRUE == HAL_TraxIsTraceActive())
    {
        Test_SetTracer(ulMcuId, 0x00);
        Test_SetTracer(ulMcuId, 0xff);
    }

    pTraxFileBuff = Test_AllocTraxFileBuff(ulMcuId);

    HAL_TraxGetTrace(&tTraxContext, pTraxFileBuff);

    while(1);
    return;
}

void Test_Trax3CoreMain()
{
    U32 ulMcuID;

    ulMcuID = HAL_GetMcuId();
    
    if (MCU0_ID == ulMcuID)
    {
        HalSetMutiCoreMode();
        HAL_CofigTraceBuffMutiMcu();
        HalBootMcu12FromIsram();
    }
    Test_Trax3Core(ulMcuID);

    return;
}

