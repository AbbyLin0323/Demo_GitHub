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
//#include "TEST_DMAE.h"
#include "HAL_MultiCore.h"

extern void MCU12_DRAM_TEXT DBG_Getch();

LOCAL void MCU_SetM1DRAM_REMAP_ON(void)
{
    rGlbMCUSramMap |= (0x1<<12);
    return;
}

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
LOCAL void MCU_SetOriginalData(U32 ulMemAddr, U32 ulStep, U32 ulBaseData, U32 ulStartIndex, U32 ulIndexNum)
{
    U32 ulIndex;
    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }

    for (ulIndex = ulStartIndex; ulIndex < ulIndexNum; ulIndex++)
    {
        *((volatile U32*)(ulMemAddr + ulIndex*ulStep)) = ulMemAddr + ulBaseData + ulIndex;
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
LOCAL void MCU_AllDataIncrement(U32 ulMemAddr, U32 ulStep, U32 ulStartIndex, U32 ulIndexNum, U32 ulIncreData)
{
    U32 ulIndex;
    U32 ulTempData;

    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }
    
    for (ulIndex = ulStartIndex; ulIndex <ulIndexNum; ulIndex++)
    {
        ulTempData = *(volatile U32*)(ulMemAddr + ulIndex*ulStep);
        *(volatile U32*)(ulMemAddr + ulIndex*ulStep) = ulTempData + ulIncreData;
    }
    return;
}

LOCAL void MCU_CheckData(U32 ulMemAddr, U32 ulStep, U32 ulBaseData, U32 ulStartIndex, U32 ulIndexNum)
{
    U32 ulTempData;
    U32 ulIndex;
    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }

    for (ulIndex = ulStartIndex; ulIndex < ulIndexNum; ulIndex++)
    {
        ulTempData = *(volatile U32*)(ulMemAddr + ulIndex*ulStep);
        if ((ulMemAddr + ulBaseData + ulIndex + 2) != ulTempData)
        {
            DBG_Getch();
        }
    }
    
    return;
}

LOCAL void MCU_UpdateAllData(void)
{

#ifdef TEST_ACCESS_COMAREA
    /* update dram 54kB + 96kB */
    MCU_AllDataIncrement(COM_DSRAM0_ACCESS_START, S_1KB, 0, COM_DSRAM0_SIZE/S_1KB, 1);
    
    MCU_AllDataIncrement(COM_DSRAM1_ACCESS_START, S_1KB, 1, COM_DSRAM1_SIZE/S_1KB, 1);
    
    /* update sram S_2GB  0x8000000:S_8MB */
    MCU_AllDataIncrement(COM_DRAM_ACCESS_START, S_8MB, 0, COM_DRAM2G_SIZE/S_8MB, 1);

    /* update otfb 320kB */
    MCU_AllDataIncrement(COM_OTFB_ACCESS_START, S_4KB, 0, COM_OTFB_INTER_SIZE/S_4KB, 1);
    /* write XOR otfb 64kB */
    MCU_AllDataIncrement((COM_OTFB_ACCESS_START+0x80000), S_1KB, 0, COM_OTFB_XOR_SIZE/S_1KB, 1);
#endif

#ifdef TEST_ACCESS_ROMPOSTMEM
    MCU_AllDataIncrement(COM_ROM_POST_ACCESS_START, S_1KB, 0, (36*S_1KB)/S_1KB, 1);
#endif

#ifdef TEST_ACCESS_ICBBUS
    MCU_AllDataIncrement(COM_ICB_ACCESS_START, S_1DW, 0x6c0, (0x1b40)/S_1DW, 1);
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
    rTracer_Mcu0 = 0x33;
    rPhaseFlag = MCU0_TOKEN_FLAG;
    
#ifdef TEST_ACCESS_COMAREA
    /* write dsram0 68kB */
    MCU_SetOriginalData(COM_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 0, COM_DSRAM0_SIZE/S_1KB);

    MCU_SetOriginalData(COM_DSRAM1_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 1, COM_DSRAM1_SIZE/S_1KB);

    /* write dram S_2GB 0x8000000:S_8MB */
    MCU_SetOriginalData(COM_DRAM_ACCESS_START, S_8MB, DRAM_BASE_DATA, 0, COM_DRAM2G_SIZE/S_8MB);//modify according real DRAM size

    /* write internal otfb 320kB 0xfff00000~0xfff4ffff:4kB */
    MCU_SetOriginalData(COM_OTFB_ACCESS_START, S_4KB, OTFB_BASE_DATA, 0, COM_OTFB_INTER_SIZE/S_4KB);
    /* write XOR otfb 64kB 0xfff80000~0xfff903ff:4kB */
    MCU_SetOriginalData((COM_OTFB_ACCESS_START+0x80000), S_1KB, OTFB_BASE_DATA, 0, COM_OTFB_XOR_SIZE/S_1KB);
#endif

#ifdef TEST_ACCESS_ROMPOSTMEM
    MCU_SetOriginalData(COM_ROM_POST_ACCESS_START, S_1KB, APB_BASE_DATA, 0, (40*S_1KB)/S_1KB);
#endif

#ifdef TEST_ACCESS_ICBBUS
    MCU_SetOriginalData(COM_ICB_ACCESS_START, S_1DW, ICB_BASE_DATA, 0x6c0, (0x1b40)/S_1DW);
#endif

    rTracer_Mcu0 = 0x44;
    rPhaseFlag = MCU1_TOKEN_FLAG;

    /* wait MCU0_TOKEN_FLAG */
    while(MCU0_TOKEN_FLAG != rPhaseFlag)
    {
        HAL_DelayCycle(5);
    }

#ifdef TEST_ACCESS_COMAREA
    /* check dsram 54kB + 96kB */
    MCU_CheckData(COM_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 0, COM_DSRAM0_SIZE/S_1KB);
      
    MCU_CheckData(COM_DSRAM1_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 1, COM_DSRAM1_SIZE/S_1KB);

    /* check dram S_2GB  0x1000000:1024kB */
    MCU_CheckData(COM_DRAM_ACCESS_START, S_8MB, DRAM_BASE_DATA, 0, COM_DRAM2G_SIZE/S_8MB);

    /* check internal otfb 320kB */
    MCU_CheckData(COM_OTFB_ACCESS_START, S_4KB, OTFB_BASE_DATA, 0, COM_OTFB_INTER_SIZE/S_4KB);
    /* check xor otfb 64kB */
    MCU_CheckData((COM_OTFB_ACCESS_START+0x80000), S_1KB, OTFB_BASE_DATA, 0, COM_OTFB_XOR_SIZE/S_4KB);
    
#endif

#ifdef TEST_ACCESS_ROMPOSTMEM
    MCU_CheckData(COM_ROM_POST_ACCESS_START, S_1KB, APB_BASE_DATA, 0, (36*S_1KB)/S_1KB);
#endif

#ifdef TEST_ACCESS_ICBBUS
    MCU_CheckData(COM_ICB_ACCESS_START, S_1DW, ICB_BASE_DATA, 0x6c0, (0x1b40)/S_1DW);
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
    MCU_SetOriginalData(COM_DSRAM0_ACCESS_START, S_1KB, ulMcuId, 1, COM_DSRAM0_SIZE/S_1KB);

    MCU_Sync(ulMcuId);
    MCU_SetOriginalData(COM_DSRAM1_ACCESS_START, S_1KB, ulMcuId, 1, COM_DSRAM1_SIZE/S_1KB);

    /* write sram S_2GB  0x1000000:1024kB */
    MCU_Sync(ulMcuId);
    MCU_SetOriginalData(COM_DRAM_ACCESS_START, S_8MB, ulMcuId, 0, (COM_DRAM_SIZE/S_8MB)*2);

    /* write otfb 320kB 0xfff00000 */
    MCU_Sync(ulMcuId);
    MCU_SetOriginalData(COM_OTFB_ACCESS_START, S_4KB, ulMcuId, 0, COM_OTFB_INTER_SIZE/S_4KB);
    
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
    
    rTracer_Mcu0 = 0x11;

    MCU_Sync(MCU0_ID);
    
    /* 14kB DSRAM0-0*/
    MCU_SetOriginalData(PRI_MCU0_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 2, PRI_MCU0_DSRAM0_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU0_DSRAM0_ACCESS_START, S_1KB, 2, PRI_MCU0_DSRAM0_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU0_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 2, PRI_MCU0_DSRAM0_SIZE/S_1KB);

    rTracer_Mcu0 = 0x22;

    /* 28kB ISRAM, Add by Regina*/
    MCU_SetOriginalData(PRI_MCU0_ISRAM_ACCESS_START, S_1KB, ISRAM_BASE_DATA, 2, PRI_MCU0_ISRAM_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU0_ISRAM_ACCESS_START, S_1KB, 2, PRI_MCU0_ISRAM_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU0_ISRAM_ACCESS_START, S_1KB, ISRAM_BASE_DATA, 2, PRI_MCU0_ISRAM_SIZE/S_1KB);
    
    rTracer_Mcu0 = 0x33;

    /* 16kB CacheDSRAM0, Add by Regina*/
    MCU_SetOriginalData(PRI_MCU0_CacheDSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 0, PRI_MCU0_CacheDSRAM0_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU0_CacheDSRAM0_ACCESS_START, S_1KB, 0, PRI_MCU0_CacheDSRAM0_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU0_CacheDSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 0, PRI_MCU0_CacheDSRAM0_SIZE/S_1KB);

    rTracer_Mcu0 = 0x44;

    /* 32kB CacheDSRAM1, Add by Regina*/
    MCU_SetOriginalData(PRI_MCU0_CacheDSRAM1_ACCESS_START, S_1KB, DSRAM1_BASE_DATA, 0, PRI_MCU0_CacheDSRAM1_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU0_CacheDSRAM1_ACCESS_START, S_1KB, 0, PRI_MCU0_CacheDSRAM1_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU0_CacheDSRAM1_ACCESS_START, S_1KB, DSRAM1_BASE_DATA, 0, PRI_MCU0_CacheDSRAM1_SIZE/S_1KB);

    rTracer_Mcu0 = 0x55;
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
    U32 ulOffsetIndex;
    U32 ulDataTemp;
    
    /* SYNC */
    MCU_Sync(MCU1_ID);
    
    /* 30kB DSRAM1 */
    MCU_SetOriginalData(PRI_MCU1_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 5, PRI_MCU1_DSRAM0_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU1_DSRAM0_ACCESS_START, S_1KB, 5, PRI_MCU1_DSRAM0_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU1_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 5, PRI_MCU1_DSRAM0_SIZE/S_1KB);

    rTracer_Mcu1 = 0x22;

    /* 84kB/116kB ISRAM, Add by Regina*/
    MCU_SetOriginalData(PRI_MCU1_ISRAM_ACCESS_START, S_1KB, ISRAM_BASE_DATA, 5, PRI_MCU1_ISRAM_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU1_ISRAM_ACCESS_START, S_1KB, 5, PRI_MCU1_ISRAM_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU1_ISRAM_ACCESS_START, S_1KB, ISRAM_BASE_DATA, 5, PRI_MCU1_ISRAM_SIZE/S_1KB);

    rTracer_Mcu1 = 0x33;
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
    U32 ulOffsetIndex;
    
    /* SYNC */
    MCU_Sync(MCU2_ID);
    
    /* 30kB DSRAM1 */
    MCU_SetOriginalData(PRI_MCU2_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 5, PRI_MCU2_DSRAM0_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU2_DSRAM0_ACCESS_START, S_1KB, 5, PRI_MCU2_DSRAM0_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU2_DSRAM0_ACCESS_START, S_1KB, DSRAM_BASE_DATA, 5, PRI_MCU2_DSRAM0_SIZE/S_1KB);

    rTracer_Mcu2 = 0x22;

    /* 52kB ISRAM, Add by Regina*/
    MCU_SetOriginalData(PRI_MCU2_ISRAM_ACCESS_START, S_1KB, ISRAM_BASE_DATA, 5, PRI_MCU2_ISRAM_SIZE/S_1KB);
    MCU_AllDataIncrement(PRI_MCU2_ISRAM_ACCESS_START, S_1KB, 5, PRI_MCU2_ISRAM_SIZE/S_1KB, 2);
    MCU_CheckData(PRI_MCU2_ISRAM_ACCESS_START, S_1KB, ISRAM_BASE_DATA, 5, PRI_MCU2_ISRAM_SIZE/S_1KB);
     
    rTracer_Mcu2 = 0x33;
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
    U32 ulCacheBaseAddr;

    #ifndef DRAM_REMAP_2G_HIGH
        //ulCacheBaseAddr = DRAM_HIGH_START_ADDRESS;
        ulCacheBaseAddr = DRAM_START_ADDRESS;
    #else
        ulCacheBaseAddr = COM_DRAM_HIGH2G_ACCESS_START;
    #endif
    

    //for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
    //{
    #ifndef DRAM_REMAP_2G_HIGH
    
        rTracer_Mcu0 = 0x00;
        for (ulIndex = 0; ulIndex < 256; ulIndex++)
        {        
            *(U32*)(ulCacheBaseAddr + ulIndex*4) = 0x12345678 + ulIndex;
        }    
        for (ulIndex = 0; ulIndex < 256; ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulIndex*4);
            if ((0x12345678 + ulIndex) != usTest)
            {
                rTracer_Mcu0 = 0xff;
                while (1);
            }    
        }

        rTracer_Mcu0 = 0x01;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + 0x2000 + ulIndex*4);
            if ((0x23456789 + ulIndex) != usTest)
            {
                rTracer_Mcu0 = 0xff;
                while (1);
            }    
        } 

        rTracer_Mcu0 = 0x02;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + 0x40000000 + ulIndex*4) = 0x3456789a + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + 0x40000000 + ulIndex*4);
            if ((0x3456789a + ulIndex) != usTest)
            {
                rTracer_Mcu0 = 0xff;
                while (1);
            }    
        } 

        rTracer_Mcu0 = 0x03;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + 0x40000000 + 0x2000 + ulIndex*4) = 0x456789ab + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + 0x40000000 + 0x2000 + ulIndex*4);
            if ((0x456789ab + ulIndex) != usTest)
            {
                rTracer_Mcu0 = 0xff;
                while (1);
            }    
        }

    #else
        rTracer_Mcu0 = 0x00;
        for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
        {
            for (ulIndex = 0; ulIndex < 256; ulIndex++)        
            {        
                *(U32*)(ulCacheBaseAddr + ulIndex*4) = 0x12345678 + ulIndex;
            }    
            for (ulIndex = 0; ulIndex < 256; ulIndex++)        
            {        
                usTest = *(U32*)(ulCacheBaseAddr + ulIndex*4);
                if ((0x12345678 + ulIndex) != usTest)
                {
                    rTracer_Mcu0 = 0xff;
                    while (1);
                }    
            }
            
            rTracer_Mcu0 = 0x01;
            for (ulIndex = 0;ulIndex < 256;ulIndex++)        
            {        
                *(U32*)(ulCacheBaseAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
            }    
            for (ulIndex = 0;ulIndex < 256;ulIndex++)        
            {        
                usTest = *(U32*)(ulCacheBaseAddr + 0x2000 + ulIndex*4);
                if ((0x23456789 + ulIndex) != usTest)
                {
                    rTracer_Mcu0 = 0xff;
                    while (1);
                }    
            }
            rTracer_Mcu0 = 0x02;
        }
         
    #endif
        
    //}
    rTracer_Mcu0 = 0x04;
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
    U32 ulCacheBaseAddr;

    #ifndef DRAM_REMAP_2G_HIGH
        //ulCacheBaseAddr = DRAM_HIGH_START_ADDRESS;
        ulCacheBaseAddr = DRAM_START_ADDRESS;
    #else
        ulCacheBaseAddr = COM_DRAM_HIGH2G_ACCESS_START;
    #endif

    ulOffsetAddr = 0x10000;
    
    #ifndef DRAM_REMAP_2G_HIGH
    
        rTracer_Mcu1 = 0x00;
        for (ulIndex = 0; ulIndex < 256; ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4) = 0x12345678 + ulIndex;
        }    
        for (ulIndex = 0; ulIndex < 256; ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4);
            if ((0x12345678 + ulIndex) != usTest)
            {
                rTracer_Mcu1 = 0xff;
                while (1);
            }    
        }
        
        rTracer_Mcu1 = 0x01;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4);
            if ((0x23456789 + ulIndex) != usTest)
            {
                rTracer_Mcu1 = 0xff;
                while (1);
            }    
        } 
        
        rTracer_Mcu1 = 0x02;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + ulIndex*4) = 0x3456789a + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + ulIndex*4);
            if ((0x3456789a + ulIndex) != usTest)
            {
                rTracer_Mcu1 = 0xff;
                while (1);
            }    
        } 

        rTracer_Mcu1 = 0x03;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + 0x2000 + ulIndex*4) = 0x456789ab + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + 0x2000 + ulIndex*4);
            if ((0x456789ab + ulIndex) != usTest)
            {
                rTracer_Mcu1 = 0xff;
                while (1);
            }    
        }

    #else
        rTracer_Mcu1 = 0x00;
        for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
        {
            for (ulIndex = 0; ulIndex < 256; ulIndex++)        
            {        
                *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4) = 0x12345678 + ulIndex;
            }    
            for (ulIndex = 0; ulIndex < 256; ulIndex++)        
            {        
                usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4);
                if ((0x12345678 + ulIndex) != usTest)
                {
                    rTracer_Mcu1 = 0xff;
                    while (1);
                }    
            }
            
            rTracer_Mcu1 = 0x01;
            for (ulIndex = 0;ulIndex < 256;ulIndex++)        
            {        
                *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
            }    
            for (ulIndex = 0;ulIndex < 256;ulIndex++)        
            {        
                usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4);
                if ((0x23456789 + ulIndex) != usTest)
                {
                    rTracer_Mcu1 = 0xff;
                    while (1);
                }    
            }
            rTracer_Mcu1 = 0x02;
        }
         
    #endif
    
    rTracer_Mcu1 = 0x04;
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
    U32 ulCacheBaseAddr;

    /*access the last 1kB memory*/
    #ifndef DRAM_REMAP_2G_HIGH
        ulCacheBaseAddr = DRAM_START_ADDRESS;
        //ulOffsetAddr = COM_DRAM_HIGH_CACHE_SIZE-0x400;
        //Regina Modify
        //ulOffsetAddr = COM_DRAM_SIZE-0x400-0x2000;
    #else
        ulCacheBaseAddr = COM_DRAM_HIGH2G_ACCESS_START;
        //ulOffsetAddr = COM_DRAM_HIGH2G_CACHE_SIZE-0X400;
        //Regina Modify
        //ulOffsetAddr = COM_DRAM_HIGH2G_CACHE_SIZE-0X400-0x2000;
    #endif

    ulOffsetAddr = 0x20000;
    
    #ifndef DRAM_REMAP_2G_HIGH
    
        rTracer_Mcu2 = 0x00;
        for (ulIndex = 0; ulIndex < 256; ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4) = 0x12345678 + ulIndex;
        }    
        for (ulIndex = 0; ulIndex < 256; ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4);
            if ((0x12345678 + ulIndex) != usTest)
            {
                rTracer_Mcu2 = 0xff;
                while (1);
            }    
        }
        
        rTracer_Mcu2 = 0x01;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4);
            if ((0x23456789 + ulIndex) != usTest)
            {
                rTracer_Mcu2 = 0xff;
                while (1);
            }    
        } 
        
        rTracer_Mcu2 = 0x02;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + ulIndex*4) = 0x3456789a + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + ulIndex*4);
            if ((0x3456789a + ulIndex) != usTest)
            {
                rTracer_Mcu2 = 0xff;
                while (1);
            }    
        } 

        rTracer_Mcu2 = 0x03;
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + 0x2000 + ulIndex*4) = 0x456789ab + ulIndex;
        }    
        for (ulIndex = 0;ulIndex < 256;ulIndex++)        
        {        
            usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x40000000 + 0x2000 + ulIndex*4);
            if ((0x456789ab + ulIndex) != usTest)
            {
                rTracer_Mcu2 = 0xff;
                while (1);
            }    
        }

    #else
        rTracer_Mcu2 = 0x00;
        for (ulCycleIndex = 0; ulCycleIndex < 2; ulCycleIndex++)
        {
            for (ulIndex = 0; ulIndex < 256; ulIndex++)        
            {        
                *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4) = 0x12345678 + ulIndex;
            }    
            for (ulIndex = 0; ulIndex < 256; ulIndex++)        
            {        
                usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + ulIndex*4);
                if ((0x12345678 + ulIndex) != usTest)
                {
                    rTracer_Mcu2 = 0xff;
                    while (1);
                }    
            }
            
            rTracer_Mcu2 = 0x01;
            for (ulIndex = 0;ulIndex < 256;ulIndex++)        
            {        
                *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4) = 0x23456789 + ulIndex;
            }    
            for (ulIndex = 0;ulIndex < 256;ulIndex++)        
            {        
                usTest = *(U32*)(ulCacheBaseAddr + ulOffsetAddr + 0x2000 + ulIndex*4);
                if ((0x23456789 + ulIndex) != usTest)
                {
                    rTracer_Mcu2 = 0xff;
                    while (1);
                }    
            }
            rTracer_Mcu2 = 0x02;
        }
         
    #endif
    
    rTracer_Mcu2 = 0x04;
    while(1);
}

LOCAL void Test_McuCommonAreaAccess(void)
{
    if (MCU0_ID == HAL_GetMcuId())
    {
        MCU_MCU0ComAreaProcess();        
    }
    if (MCU1_ID == HAL_GetMcuId())
    {
        MCU_MCU1ComAreaProcess();
    }    
    if (MCU2_ID == HAL_GetMcuId())
    {
        MCU_MCU2ComAreaProcess();
    }
    
    return;
}

LOCAL void Test_McuPrivacyAreaAccess(void)
{
    if (MCU0_ID == HAL_GetMcuId())
    {
        MCU_MCU0PrivacyAreaProcess();
    }
    if (MCU1_ID == HAL_GetMcuId())
    {
        MCU_MCU1PrivacyAreaProcess();
    }    
    if (MCU2_ID == HAL_GetMcuId())
    {
        MCU_MCU2PrivacyAreaProcess();
    }

    return;
}

LOCAL void Test_McuCommonAreaAbitration(void)
{
    MCU_ComAreaAbitration(HAL_GetMcuId());
    return;
}

LOCAL void Test_McuIDcache(void)
{         
    if (MCU0_ID == HAL_GetMcuId())
    {
    #ifndef DRAM_REMAP_2G_HIGH
        HAL_EnableDCache(MCU0_ID);
    #else
        MCU_EnableDCache(MCU0_ID);
    #endif
    
        HAL_EnableICache(MCU0_ID);
        MCU_MCU0IDcacheProcess();
    }
    if (MCU1_ID == HAL_GetMcuId())
    {
    #ifndef DRAM_REMAP_2G_HIGH
        HAL_EnableDCache(MCU1_ID);
    #else
        MCU_EnableDCache(MCU1_ID);
    #endif
    
        HAL_EnableICache(MCU1_ID);
        MCU_MCU1IDcacheProcess();
    }
    if (MCU2_ID == HAL_GetMcuId())
    {
    #ifndef DRAM_REMAP_2G_HIGH
        HAL_EnableDCache(MCU2_ID); 
    #else
        MCU_EnableDCache(MCU2_ID);
    #endif
    
        HAL_EnableICache(MCU2_ID);
        MCU_MCU2IDcacheProcess();
    }

    return;
}

void Test_DmaeCopyAcrossMcuMain(void)
{
    rTracer_Mcu0 = 0x11;
    HAL_DMAEInit();
    /* prepare data in OTFB */
    MCU_SetOriginalData(COM_OTFB_ACCESS_START, sizeof(U32), OTFB_BASE_DATA, 0, 512);
    *((volatile U32 *)(OTFB_START_ADDRESS)) = 0xffff06;
    rTracer_Mcu0 = 0x22;

    MCU_BootMcu12FromRom();
    rTracer_Mcu0 = 0x33;

    //DMAE copy OTFB to mcu12 ISRAM 0x20000400
    //HAL_DMAECopyOneBlock(0x20000400, OTFB_START_ADDRESS, 512*sizeof(U32), MCU1_ID);
    //HAL_DMAECopyOneBlock(0x20000400, OTFB_START_ADDRESS, 512*sizeof(U32), MCU2_ID);
    rTracer_Mcu0 = 0x44;

    /* change MCU12 Boot from Isram */
    MCU_MCU12BootFromIsram();
    rTracer_Mcu0 = 0x55;

    while(1);
    return;
}

void Test_McuOtfbRemapMain(void)
{
    rGlbMCUSramMap |= 0x2f;

    //OTFB All Size Accessed
    rTracer_Mcu0 = 0x11;
    MCU_SetOriginalData(COM_OTFB_ACCESS_START, S_4KB, OTFB_BASE_DATA, 0, OTFB_ALL_SIZE/S_4KB);    
    MCU_AllDataIncrement(COM_OTFB_ACCESS_START, S_4KB, 0, OTFB_ALL_SIZE/S_4KB, 2);
    MCU_CheckData(COM_OTFB_ACCESS_START, S_4KB, OTFB_BASE_DATA, 0, OTFB_ALL_SIZE/S_4KB);
    
    rTracer_Mcu0 = 0x22;
    return;
}

void Test_Mcu1RemapMain(void)
{
    rTracer_Mcu1 = 0x11;

    /*ISRAM*/
    MCU_SetOriginalData((MCU1_ISRAM_BASE+0x18000), S_1KB, ISRAM_BASE_DATA, 0, (32*S_1KB)/S_1KB);
    MCU_AllDataIncrement((MCU1_ISRAM_BASE+0x18000), S_1KB, 0, (32*S_1KB)/S_1KB, 2);
    MCU_CheckData((MCU1_ISRAM_BASE+0x18000), S_1KB, ISRAM_BASE_DATA, 0, (32*S_1KB)/S_1KB);

    rGlbMCUSramMap |= (0x1<<12);  //Open Remap function
    
    rTracer_Mcu1 = 0x22;

    /* Remap to DSRAM0 */
    MCU_SetOriginalData((SRAM0_START_ADDRESS+0x28000), S_1KB, ISRAM_BASE_DATA, 0, (32*S_1KB)/S_1KB);
    MCU_AllDataIncrement((SRAM0_START_ADDRESS+0x28000), S_1KB, 0, (32*S_1KB)/S_1KB, 2);
    MCU_CheckData((SRAM0_START_ADDRESS+0x28000), S_1KB, ISRAM_BASE_DATA, 0, (32*S_1KB)/S_1KB);

    rTracer_Mcu1 = 0x33;
    while(1);
    
    return;
}

/*** Regina add for remap function ***/
void Test_McuRemapFuncMain(void)
{

    if (MCU0_ID == HAL_GetMcuId())
    {
        Test_McuOtfbRemapMain();
    }
    if (MCU1_ID == HAL_GetMcuId())
    {
        Test_Mcu1RemapMain();
    }    
    if (MCU2_ID == HAL_GetMcuId())
    {
        while(1);
    }

    return;
}


void Test_McuBasicFuncMain(void)
{
   
    /* MCU0 prepare work */
    if (MCU0_ID == HAL_GetMcuId())
    {   
        rTracer_Mcu0 = 0x11;

        MCU_SetMutiCoreMode();
        
/*
#ifndef VT3514_C0
        MCU_SetMCU12IsramSZ();
        //ISRAM cache
        MCU_IsramCacheOpen();
#endif
*/

        /* DRAMC Initi*/

#ifdef COSIM
       HAL_DramcInit();
#endif

/*
#ifdef DRAM_REMAP_2G_HIGH
        MCU_DRAMRemap2G(FALSE);  //Enable 0xc0000000~0xefffffff remap to 0x40000000~0x6fffffff
#endif
*/
        /* MCU0 boot MCU12 from ISRAM */
        MCU_BootMcu12FromIsram();
/*        
#ifdef DRAM_REMAP_2G
        MCU_DRAMRemap2G(TRUE);
#endif
*/

        rTracer_Mcu0 = 0x22;
        rTracer_Mcu3 = 0x0;
    }
/*
#ifdef M1DRAM_REMAP_ON
    MCU_SetM1DRAM_REMAP_ON();
#endif
*/
    Test_McuCommonAreaAccess();

    //Test_McuPrivacyAreaAccess();
    
    //Test_McuCommonAreaAbitration();

    //Test_McuIDcache();
    
    //Test_McuOtfbRemapMain();

    //Test_McuRemapFuncMain();

   
    return;
}
