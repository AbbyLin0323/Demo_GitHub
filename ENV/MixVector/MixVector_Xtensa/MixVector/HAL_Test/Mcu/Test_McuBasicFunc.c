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
Filename    : TEST_McuBasicFunc.c
Version     : Ver 1.0
Author      : TobeyTan
Date        : 2013-11-28
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#include "HAL_DramConfig.h"
#include "HAL_Dmae.h"
#include "Test_McuBasicFunc.h"
#include "TEST_DMAE.h"
#include "HAL_MultiCore.h"

#define TEST_ACCESS_ROMPOSTMEM
#define DRAM_REMAP_2G

LOCAL U32 l_ulMcuID;

extern void MCU12_DRAM_TEXT DBG_Getch();

LOCAL void MCU_SetMutiCoreMode(void)
{
    rGlbMCUSramMap = 0x0;
    return;
}

LOCAL void MCU_SetMCU12IsramSZ(void)
{
    rGLB(0x3C) |= 0x4;
    return;
}

LOCAL void MCU_IsramCacheOpen(void)
{
    rGLB(0x3C) |= 0x3;
    return;
}

LOCAL void MCU_BootMcu12FromIsram(void)
{
    //configure mcu1/2 boot from isram
    rGlbMCUCtrl = 0x0;
    rGlbMCUCtrl |= 0x110;
    
    //enable mcu1/2 boot up
    rGlbMcuSgeRst = 0x0;
    return;
}


LOCAL void MCU_BootMcu12FromRom(void)
{
    //configure mcu1/2 boot from isram
    rGlbMCUCtrl = 0x0;

    //enable mcu1/2 boot up
    rGlbMcuSgeRst = 0x0;
    return;
}

LOCAL void MCU_MCU12BootFromIsram(void)
{
    rGlbMcuSgeRst = 0xf;
    rGlbMCUCtrl |= 0x110;
    rGlbMcuSgeRst = 0x0;
    return;
}

LOCAL void MCU_DRAMRemap2G(BOOL bHighAddr)
{
    //Clear bit[24:22]
    rGlbMCUMisc &= ~(0x7<<22);
    if(FALSE == bHighAddr)
    {
        rGlbMCUMisc |= (0x1<<22);
    }
    else
    {
        rGlbMCUMisc |= (0x2<<22);
    }

    return;
}

LOCAL void MCU_EnableDCache(U32 ulMcuId)
{
    U32 ulBuf = 0;

    /* Dcache open */
    if (MCU0_ID == ulMcuId)
    {
        ulBuf |= 1 << 17; // wire RMCU0_RDCache_EN = REGGLB_40[17];
    }
    else if (MCU1_ID == ulMcuId)
    {
        ulBuf |= 1 << 19; // wire RMCU1_RDCache_EN = REGGLB_40[19];
    }
    else //MCU2_ID
    {
        ulBuf |= 1 << 21; // wire RMCU2_RDCache_EN = REGGLB_40[21];
    }
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_GLB40);
    rGlbMCUMisc |= ulBuf;
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_GLB40);

#ifndef SIM
    xthal_dcache_all_unlock();
    xthal_dcache_all_invalidate();

    /* The 4GB address space is divided into 8*512MB, and cache attribute for each
    512MB is assigned by a 4-bit value.
    1 : cached, write thru
    2 : bypass cache
    4 : cached, write back
    */
    /*Open Dcache range 0xc0000000~0xffffffff*/
    xthal_set_dcacheattr(0x11222222);
    xthal_dcache_sync();
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: MCU_MCU0ComAreaProcess
Description: 
    mcu0 write common area and check last data.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu0 process flow when test common area.
------------------------------------------------------------------------------*/
LOCAL void MCU_SetOriginalData(U32 ulBaseAddr, U32 ulOffsetAddr, U32 ulStep, U32 ulBaseData, U32 ulStartIndex, U32 ulIndexNum)
{
    U32 ulIndex;
    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }

    for (ulIndex = ulStartIndex; ulIndex < ulIndexNum; ulIndex++)
    {
        *((volatile U32*)(ulBaseAddr + ulOffsetAddr + ulIndex*ulStep)) = ulBaseData + ulOffsetAddr + ulIndex;
    }
    
    return;
}

/*----------------------------------------------------------------------------
Name: UpdateAllData
Description: 
    read data out and wirte back after "data+1".
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    in common area test. mcu1 and muc2 invoke it.
------------------------------------------------------------------------------*/
LOCAL void MCU_AllDataIncrement(U32 ulBaseAddr, U32 ulOffsetAddr, U32 ulStep, U32 ulStartIndex, U32 ulIndexNum, U32 ulIncreData)
{
    U32 ulIndex;
    U32 ulTempData;

    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }
    
    for (ulIndex = ulStartIndex; ulIndex <ulIndexNum; ulIndex++)
    {
        ulTempData = *(volatile U32*)(ulBaseAddr + ulOffsetAddr + ulIndex*ulStep);
        *(volatile U32*)(ulBaseAddr + ulOffsetAddr + ulIndex*ulStep) = ulTempData + ulIncreData;
    }
    return;
}

LOCAL void MCU_CheckData(U32 ulBaseAddr, U32 ulOffsetAddr, U32 ulStep, U32 ulBaseData, U32 ulStartIndex, U32 ulIndexNum)
{
    U32 ulTempData;
    U32 ulIndex;
    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }

    for (ulIndex = ulStartIndex; ulIndex < ulIndexNum; ulIndex++)
    {
        ulTempData = *(volatile U32*)(ulBaseAddr + ulOffsetAddr + ulIndex*ulStep);
        if ((ulBaseData + ulOffsetAddr + ulIndex + 2) != ulTempData)
        {
            DBG_Getch();
        }
    }
    
    return;
}

LOCAL void MCU_UpdateAllData(void)
{
    U32 ulOffsetIndex;
    U32 ulAddrOffset = 0;
    U32 ulTempData;
#ifndef DRAM_REMAP_2G
#ifndef TEST_ACCESS_ROMPOSTMEM
    /* update dram 54kB + 96kB */
    ulAddrOffset = 0x4000;
    MCU_AllDataIncrement(REG_BASE, ulAddrOffset, 0x400, 0, 68, 1);
    
    ulAddrOffset = 0x8000;
    MCU_AllDataIncrement(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, 1, 104, 1);
    
    /* update sram 1GB  0x8000000:8MB */
    ulAddrOffset = 0x0;
    MCU_AllDataIncrement(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, 0, 64, 1);

    /* update otfb 320kB */
    ulAddrOffset = 0x0;
    MCU_AllDataIncrement(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, 0, 112, 1);
#else
    ulAddrOffset = 0x0;
    MCU_AllDataIncrement(0xffe06000, ulAddrOffset, 0x4, 0, 256, 1);
#endif
#else
    ulAddrOffset = 0x0;
    MCU_AllDataIncrement(0xc0000000, ulAddrOffset, 0x100000, 0, 768, 1);
#endif

    return;
}

/*----------------------------------------------------------------------------
Name: MCU_MCU0ComAreaProcess
Description: 
    mcu0 write common area and check last data.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu0 process flow when test common area.
------------------------------------------------------------------------------*/
LOCAL void MCU_MCU0ComAreaProcess(void)
{
    U32 ulOffsetIndex = 0;
    U32 ulAddrOffset = 0;
    U32 ulCheckData;
    rTracer_Mcu0 = 0x33;
    rPhaseFlag = MCU0_TOKEN_FLAG;
#ifndef DRAM_REMAP_2G
#ifndef TEST_ACCESS_ROMPOSTMEM
    /* write dram 54kB + 96kB */
    ulAddrOffset = 0x4000;
    MCU_SetOriginalData(REG_BASE, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 68);

    ulAddrOffset = 0x8000;
    MCU_SetOriginalData(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 1, 104);

    /* write sram 1GB 0x8000000:8MB */
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, SRAM_BASE_DATA, 0, 64);

    /* write otfb 320kB 0xfff00000 0x1000:4kB */
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, OTFB_BASE_DATA, 0, 112);
#else
	ulAddrOffset = 0x0;
	MCU_SetOriginalData(0xffe06000, ulAddrOffset, 0x4, OTFB_BASE_DATA, 0, 256);
#endif
#else
	/*step is 64k*/
	ulAddrOffset = 0x0;
	MCU_SetOriginalData(0xc0000000, ulAddrOffset, 0x100000, DRAM_BASE_DATA, 0, 768);
#endif

    rTracer_Mcu0 = 0x44;
    rPhaseFlag = MCU1_TOKEN_FLAG;

    /* wait MCU0_TOKEN_FLAG */
    while(MCU0_TOKEN_FLAG != rPhaseFlag)
    {
        HAL_DelayCycle(5);
    }
#ifndef DRAM_REMAP_2G
#ifndef TEST_ACCESS_ROMPOSTMEM
    /* check dram 54kB + 96kB */
    ulAddrOffset = 0x4000;
    MCU_CheckData(REG_BASE, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 68);
      
    ulAddrOffset = 0x8000;
    MCU_CheckData(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 1, 104);

    /* check dsram 1GB  0x1000000:1024kB */
    ulAddrOffset = 0x0;
    MCU_CheckData(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, SRAM_BASE_DATA, 0, 64);

    /* check otfb 320kB */
    ulAddrOffset = 0x0;
    MCU_CheckData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, OTFB_BASE_DATA, 0, 112);
#else
	ulAddrOffset = 0x0;
	MCU_CheckData(0xffe06000, ulAddrOffset, 0x4, OTFB_BASE_DATA, 0, 256);
#endif
#else
    ulAddrOffset = 0x0;
    MCU_CheckData(0xc0000000, ulAddrOffset, 0x100000, DRAM_BASE_DATA, 0, 768);
#endif

    /* check end */
    rTracer_Mcu0 = 0x55;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU1ComAreaProcess
Description: 
    mcu1 process flow when test common area.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu1 process flow when test common area.
------------------------------------------------------------------------------*/
LOCAL void MCU_MCU1ComAreaProcess(void)
{
    rTracer_Mcu1 = 0x11;
    
    /* wait MCU1_TOKEN_FLAG */
    while (MCU1_TOKEN_FLAG != rPhaseFlag)
    {
        HAL_DelayCycle(5);
    }

    MCU_UpdateAllData();
    
    rPhaseFlag = MCU2_TOKEN_FLAG;
    rTracer_Mcu1 = 0x22;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU2ComAreaProcess
Description: 
    mcu2 write common area and check last data.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu2 process flow when test common area.
------------------------------------------------------------------------------*/
LOCAL void MCU_MCU2ComAreaProcess(void)
{
    rTracer_Mcu2 = 0x11;
    
    /* wait MCU2_TOKEN_FLAG */
    while (MCU2_TOKEN_FLAG != rPhaseFlag)
    {
        HAL_DelayCycle(5);
    }

    MCU_UpdateAllData();
    
    rPhaseFlag = MCU0_TOKEN_FLAG;
    rTracer_Mcu2 = 0x22;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_SyncMid
Description: 
    mcu synchronous logic.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        when sync all mcu, every mcu invoke it after first step.
------------------------------------------------------------------------------*/
LOCAL void MCU_Sync(U32 ulMcuId)
{
    rPhaseFlag = MCU_INIT_fLAG;
    if (MCU0_ID == ulMcuId)
    {
        while ((rMCU1Done != MCU1_DONE_FLAG) || (rMCU2Done != MCU2_DONE_FLAG))
        {
            HAL_DelayCycle(SYC_DELAY_CYCLE);
        }
        rPhaseFlag = MCU_SYNC_fLAG;
    }
    else if (MCU1_ID == ulMcuId)
    {
        rMCU1Done = MCU1_DONE_FLAG;
        while (rPhaseFlag != MCU_SYNC_fLAG)
        {
            HAL_DelayCycle(SYC_DELAY_CYCLE);
        }
        rMCU1Done = 0;
    }
    else  //MCU2_ID
    {
        rMCU2Done = MCU2_DONE_FLAG;
        while (rPhaseFlag != MCU_SYNC_fLAG)
        {
            HAL_DelayCycle(SYC_DELAY_CYCLE);
        }
        rMCU2Done = 0;
    }
}

/*----------------------------------------------------------------------------
Name: MCU_ComAreaAbitration
Description: 
    common area arbitration test flow. every mcu access common area at the same time.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        when test common area arbitration, every mcu invoke it.
------------------------------------------------------------------------------*/
LOCAL void MCU_ComAreaAbitration(U32 ulMcuId)
{
    U32 ulOffsetIndex = 0;
    U32 ulAddrOffset = 0;
    U32 ulCheckData;
    if(MCU0_ID == ulMcuId)
    {
        rTracer_Mcu0 = 0x11;
    }
    else if(MCU1_ID == ulMcuId)
    {
        rTracer_Mcu1 = 0x11;
    }
    else
    {
        rTracer_Mcu2 = 0x11;
    }

    /* write dram 54kB + 96kB */
    MCU_Sync(ulMcuId);   
    ulAddrOffset = 0x4000;
    MCU_SetOriginalData(REG_BASE, ulAddrOffset, 0x400, ulMcuId, 1, 68);

    MCU_Sync(ulMcuId);
    ulAddrOffset = 0x8000;
    MCU_SetOriginalData(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, ulMcuId, 1, 104);

    /* write sram 1GB  0x1000000:1024kB */
    MCU_Sync(ulMcuId);
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, ulMcuId, 0, 64);

    /* write otfb 448kB 0xfff00000 */
    MCU_Sync(ulMcuId);
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, ulMcuId, 0, 112);
    
    /* task done */
    if(MCU0_ID == ulMcuId)
    {
        rTracer_Mcu0 = 0x22;
    }
    else if(MCU1_ID == ulMcuId)
    {
        rTracer_Mcu1 = 0x22;
    }
    else
    {
        rTracer_Mcu2 = 0x22;
    }
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU0PrivacyAreaProcess
Description: 
    mcu0 privacy area process flow.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu0 invoke it when test privacy area access.
------------------------------------------------------------------------------*/
LOCAL void MCU_MCU0PrivacyAreaProcess(void)
{
    U32 ulAddrOffset;
    U32 ulOffsetIndex;
    U32 ulDataTemp;
    U32 ulDelayIdex;
    
    rTracer_Mcu0 = 0x11;

    MCU_Sync(MCU0_ID);
    
    /* 14kB DSRAM1*/
    ulAddrOffset = 0x24800;
    MCU_SetOriginalData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 14);
    MCU_AllDataIncrement(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, 0, 14, 2);
    MCU_CheckData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 14);
    
    rTracer_Mcu0 = 0x22;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU1PrivacyAreaProcess
Description: 
    mcu1 privacy area process flow.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu1 invoke it when test privacy area access.
------------------------------------------------------------------------------*/
LOCAL void MCU_MCU1PrivacyAreaProcess(void)
{
    U32 ulAddrOffset;
    U32 ulOffsetIndex;
    U32 ulDataTemp;
    
    /* SYNC */
    MCU_Sync(MCU1_ID);
    
    /* 30kB DSRAM1 */
    ulAddrOffset = 0x20800;
    MCU_SetOriginalData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 30);
    MCU_AllDataIncrement(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, 0, 30, 2);
    MCU_CheckData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 30);

    rTracer_Mcu1 = 0x22;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU2PrivacyAreaProcess
Description: 
    mcu2 privacy area process flow.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu2 invoke it when test privacy area access.
------------------------------------------------------------------------------*/
LOCAL void MCU_MCU2PrivacyAreaProcess(void)
{
    U32 ulAddrOffset;
    U32 ulOffsetIndex;
    U32 ulDataTemp;
    
    /* SYNC */
    MCU_Sync(MCU2_ID);
    
    /* 30kB DSRAM1 */
    ulAddrOffset = 0x20800;
    MCU_SetOriginalData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 30);
    MCU_AllDataIncrement(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, 0, 30, 2);
    MCU_CheckData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 30);
      
    rTracer_Mcu2 = 0x22;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU0IDcacheProcess
Description: 
    3 core idcache test, mcu0 access ddr sram.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu0 invoke it when test 3 core idcache.
------------------------------------------------------------------------------*/
LOCAL void OTFB_MCU0_ATTR MCU_MCU0IDcacheProcess(void)
{
    U32 usTest;
    U32 ulCycleIndex;
    U32 ulIndex;
    rTracer_Mcu0 = 0x00;
    for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
    {
        for (ulIndex = 0; ulIndex < 128; ulIndex++)        
        {        
            *(U32*)(DRAM_START_ADDRESS + ulIndex*4) = 0x12345678 + ulIndex;
        }    
        for (ulIndex = 0; ulIndex < 128; ulIndex++)        
        {        
            usTest = *(U32*)(DRAM_START_ADDRESS + ulIndex*4);
            if ((0x12345678 + ulIndex) != usTest)
            {
                rTracer_Mcu0 = 0xff;
                while (1);
            }    
        }
        rTracer_Mcu0 = 0x01;
        for (ulIndex = 0;ulIndex < 128;ulIndex++)        
        {        
            *(U32*)(DRAM_START_ADDRESS + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 128;ulIndex++)        
        {        
            usTest = *(U32*)(DRAM_START_ADDRESS + 0x2000 + ulIndex*4);
            if ((0x23456789 + ulIndex) != usTest)
            {
                rTracer_Mcu0 = 0xff;
                while (1);
            }    
        }    
        rTracer_Mcu0 = 0x02;
    }
    rTracer_Mcu0 = 0x03;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU1IDcacheProcess
Description: 
    3 core idcache test, mcu1 access ddr sram.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu1 invoke it when test 3 core idcache.
------------------------------------------------------------------------------*/
LOCAL void OTFB_MCU1_ATTR MCU_MCU1IDcacheProcess(void)
{
    U32 usTest;
    U32 ulCycleIndex;
    U32 ulIndex;
    U32 ulOffsetAddr;
    rTracer_Mcu1 = 0x00;
    ulOffsetAddr = 0x10000;
    for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
    {
        for (ulIndex = 0; ulIndex < 128; ulIndex++)        
        {        
            *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + ulIndex*4) = 0x12345678 + ulIndex;
        }    
        for (ulIndex = 0; ulIndex < 128; ulIndex++)        
        {        
            usTest = *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + ulIndex*4);
            if ((0x12345678 + ulIndex) != usTest)
            {
                rTracer_Mcu1 = 0xff;
                while (1);
            }    
        }
        rTracer_Mcu1 = 0x01;
        for (ulIndex = 0; ulIndex < 128; ulIndex++)        
        {        
            *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
        }    
        for (ulIndex = 0; ulIndex < 128; ulIndex++)        
        {         
            usTest = *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + 0x2000 + ulIndex*4);
            if ((0x23456789 + ulIndex) != usTest)
            {
                rTracer_Mcu1 = 0xff;
                while (1);
            }    
        }    
        rTracer_Mcu1 = 0x02;
    }
    rTracer_Mcu1 = 0x03;
    while(1);
}

/*----------------------------------------------------------------------------
Name: MCU_MCU2IDcacheProcess
Description: 
    3 core idcache test, mcu2 access ddr sram.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
        mcu2 invoke it when test 3 core idcache.
------------------------------------------------------------------------------*/
LOCAL void OTFB_MCU2_ATTR MCU_MCU2IDcacheProcess(void)
{
    U32 usTest;
    U32 ulCycleIndex;
    U32 ulIndex;
    U32 ulOffsetAddr;
    rTracer_Mcu2 = 0x00;
    ulOffsetAddr = 0x20000;
    for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
    {
        for (ulIndex=0; ulIndex<128; ulIndex++)        
        {        
            *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + ulIndex*4) = 0x12345678 + ulIndex;
        }    
        for (ulIndex=0; ulIndex<128; ulIndex++)        
        {        
            usTest = *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + ulIndex*4);
            if ((0x12345678 + ulIndex) != usTest)
            {
                rTracer_Mcu2 = 0xff;
                while (1);
            }    
        }
        rTracer_Mcu2 = 0x01;
        for (ulIndex=0; ulIndex<128; ulIndex++)        
        {        
            *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
        }    
        for (ulIndex=0; ulIndex<128; ulIndex++)        
        {        
            usTest = *(U32*)(DRAM_START_ADDRESS + ulOffsetAddr + 0x2000 + ulIndex*4);
            if ((0x23456789 + ulIndex) != usTest)
            {
                rTracer_Mcu2 = 0xff;
                while (1);
            }    
        }    
        rTracer_Mcu2 = 0x02;
    }
    rTracer_Mcu2 = 0x03;
    while(1);
}

LOCAL void Test_Mcu1MemoryAccessOnly(void)
{
     U32 ulAddrOffset;
    /* write dram 54kB + 96kB */
    ulAddrOffset = 0x4000;
    MCU_SetOriginalData(REG_BASE, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 68);
    rTracer_Mcu1 = 0x11;

    ulAddrOffset = 0x8000;
    MCU_SetOriginalData(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 96);
    rTracer_Mcu1 = 0x22;

    /* write sram 1GB 0x8000000:8MB */
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, SRAM_BASE_DATA, 0, 64);
    rTracer_Mcu1 = 0x33;

    /* write otfb 320kB 0xfff00000 0x1000:4kB */
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, OTFB_BASE_DATA, 0, 96);
    rTracer_Mcu1 = 0x44;
    

    /* update dram 54kB + 96kB */
    ulAddrOffset = 0x4000;
    MCU_AllDataIncrement(REG_BASE, ulAddrOffset, 0x400, 0, 68, 2);
    rTracer_Mcu1 = 0x55;
    
    ulAddrOffset = 0x8000;
    MCU_AllDataIncrement(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, 0, 96, 2);
    rTracer_Mcu1 = 0x66;
    
    /* update sram 1GB  0x8000000:8MB */
    ulAddrOffset = 0x0;
    MCU_AllDataIncrement(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, 0, 64, 2);
    rTracer_Mcu1 = 0x77;

    /* update otfb 320kB */
    ulAddrOffset = 0x0;
    MCU_AllDataIncrement(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, 0, 96, 2);
    rTracer_Mcu1 = 0x88;

    /* check dram 54kB + 96kB */
    ulAddrOffset = 0x4000;
    MCU_CheckData(REG_BASE, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 68);
    rTracer_Mcu1 = 0x99;
    
    ulAddrOffset = 0x8000;
    MCU_CheckData(SRAM1_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 96);
    rTracer_Mcu1 = 0xaa;

    /* check dsram 1GB  0x1000000:1024kB */
    ulAddrOffset = 0x0;
    MCU_CheckData(DRAM_START_ADDRESS, ulAddrOffset, 0x800000, SRAM_BASE_DATA, 0, 64);
    rTracer_Mcu1 = 0xbb;

    /* check otfb 320kB */
    ulAddrOffset = 0x0;
    MCU_CheckData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, OTFB_BASE_DATA, 0, 96);
    rTracer_Mcu1 = 0xcc;

    /* Mcu1 privacy area */
    /* 30kB DSRAM1 */
    ulAddrOffset = 0x20800;
    MCU_SetOriginalData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 30);
    rTracer_Mcu1 = 0xdd;
    MCU_AllDataIncrement(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, 0, 30, 2);
    rTracer_Mcu1 = 0xee;
    MCU_CheckData(SRAM0_START_ADDRESS, ulAddrOffset, 0x400, DRAM_BASE_DATA, 0, 30);
    rTracer_Mcu1 = 0xff;

    while(1);
    return;
}

LOCAL void Test_McuCommonAreaAccess(void)
{
    if (MCU0_ID == l_ulMcuID)
    {
        MCU_MCU0ComAreaProcess();        
    }
    if (MCU1_ID == l_ulMcuID)
    {
        MCU_MCU1ComAreaProcess();
    }    
    if (MCU2_ID == l_ulMcuID)
    {
        MCU_MCU2ComAreaProcess();
    }
    
    return;
}

LOCAL void Test_McuPrivacyAreaAccess(void)
{
    if (MCU0_ID == l_ulMcuID)
    {
        MCU_MCU0PrivacyAreaProcess();
    }
    if (MCU1_ID == l_ulMcuID)
    {
        MCU_MCU1PrivacyAreaProcess();
    }    
    if (MCU2_ID == l_ulMcuID)
    {
        MCU_MCU2PrivacyAreaProcess();
    }

    return;
}

LOCAL void Test_McuCommonAreaAbitration(void)
{
    MCU_ComAreaAbitration(l_ulMcuID);
    return;
}

LOCAL void Test_McuIDcache(void)
{     
    if (MCU0_ID == l_ulMcuID)
    {
        HAL_EnableICache(l_ulMcuID);
        HAL_EnableDCache(l_ulMcuID);
        MCU_MCU0IDcacheProcess();
    }
    if (MCU1_ID == l_ulMcuID)
    {
        HAL_EnableICache(l_ulMcuID);
        HAL_EnableDCache(l_ulMcuID);
        MCU_MCU1IDcacheProcess();
    }
    if (MCU2_ID == l_ulMcuID)
    {
        HAL_EnableICache(l_ulMcuID);
        HAL_EnableDCache(l_ulMcuID);
        MCU_MCU2IDcacheProcess();
    }

    return;
}

void Test_McuBasicFuncMain(void)
{
    U8 ucSpinLockId;

    l_ulMcuID = HAL_GetMcuId();

    /* MCU0 prepare work */
    if (MCU0_ID == l_ulMcuID)
    {   
        rTracer_Mcu0 = 0x11;

        MCU_SetMutiCoreMode();
#ifndef VT3514_C0
        MCU_SetMCU12IsramSZ();
        /* ISRAM cache*/
        MCU_IsramCacheOpen();
#endif

        /* DRAMC Initi*/
#ifdef COSIM
       HAL_DramcInit();
#endif

        /* MCU0 boot MCU12 from ISRAM */
        MCU_BootMcu12FromIsram();
        
#ifdef DRAM_REMAP_2G
        MCU_DRAMRemap2G(TRUE);
#endif

        rTracer_Mcu0 = 0x22;
        rTracer_Mcu3 = 0x0;
    }

#if 0
    if (MCU1_ID == l_ulMcuID)
    {
#ifdef COSIM
        HAL_DramcInit();
#endif
        Test_Mcu1MemoryAccessOnly();
    }
#endif

#ifdef DRAM_REMAP_2G
    MCU_EnableDCache(l_ulMcuID);
#endif
    Test_McuCommonAreaAccess();

    //Test_McuPrivacyAreaAccess();
    //Test_McuCommonAreaAbitration();

    //Test_McuIDcache();
    
    return;
}

void Test_DmaeCopyAcrossMcuMain(void)
{
    rTracer_Mcu0 = 0x11;
    HAL_DMAEInit();
    /* prepare data in OTFB */
    MCU_SetOriginalData(OTFB_START_ADDRESS, 0, sizeof(U32), OTFB_BASE_DATA, 0, 512);
    *((volatile U32 *)(OTFB_START_ADDRESS)) = 0xffff06;
    rTracer_Mcu0 = 0x22;

    MCU_BootMcu12FromRom();
    rTracer_Mcu0 = 0x33;

    //DMAE copy OTFB to mcu12 ISRAM 0x20000400
    HAL_DMAEHeadCopyOneBlock(0x20000400, OTFB_START_ADDRESS, 512*sizeof(U32), MCU1_ID);
    HAL_DMAEHeadCopyOneBlock(0x20000400, OTFB_START_ADDRESS, 512*sizeof(U32), MCU2_ID);
    rTracer_Mcu0 = 0x44;

    /* change MCU12 Boot from Isram */
    MCU_MCU12BootFromIsram();
    rTracer_Mcu0 = 0x55;

    while(1);
    return;
}

void Test_McuOtfbRemapMain(void)
{
    U32 ulAddrOffset;
    rGlbMCUSramMap |= 0x2f;

    rTracer_Mcu0 = 0x11;
    ulAddrOffset = 0x0;
    MCU_SetOriginalData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, OTFB_BASE_DATA, 0, 176);
    rTracer_Mcu0 = 0x22;
    MCU_AllDataIncrement(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, 0, 176, 2);
    rTracer_Mcu0 = 0x33;
    MCU_CheckData(OTFB_START_ADDRESS, ulAddrOffset, 0x1000, OTFB_BASE_DATA, 0, 176);
    rTracer_Mcu0 = 0x44;

    while(1);
    
    return;
}
