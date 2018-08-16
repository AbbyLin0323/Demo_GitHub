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
Filename    :MCU1_main.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Firmware boot related routines and main task loop.
Others      :
Modify      :
****************************************************************************/
#include "HAL_Inc.h"
#include "FW_Event.h"
#include "HAL_TraceLog.h"
#include "COM_Memory.h"
#include "Disk_Config.h"
#include "L1_Ramdisk.h"
#include "L1_Schedule.h"
#include "L0_Interface.h"
#include "FW_ViaCmd.h"
#include "FW_SMSG.h"
#ifdef SIM
#include "COM_MainSche.h"
#endif

#ifndef L1_FAKE
#include "L2_Ramdisk.h"
#include "L2_Thread.h"
#include "L2_Schedule.h"
#include "RDT.h"
#endif

#ifdef L3_UNIT_TEST
#include "BkEnd_TestMain.h"
#endif 

#ifdef DATA_MONITOR_ENABLE
#include "FW_DataMonitor.h"
#endif

#ifndef SIM
#include "HAL_ParamTable.h"
#endif

#define TL_FILE_NUM MCU1_Main_c

#ifndef SIM
extern void DBG_Getch_Init(void);
#endif

extern GLOBAL MCU12_VAR_ATTR SUBSYSTEM_MEM_BASE g_FreeMemBase;

GLOBAL  BOOL g_bL1BackgroundTaskEnable;

extern U32 HAL_GetMcuId();
#ifdef HOST_SATA
extern void HAL_SATAHostIOInit(void);
#else
extern void HAL_PCIeHostIOInit(void);
#endif
extern void HAL_CommInit(void);
extern void HAL_CommInitForWarmStart(void);
extern void L1_TaskRamdiskInit( SUBSYSTEM_MEM_BASE * pFreeMemBase );

extern void TEST_NfcMCU1Main(void);
extern void MCU12_DRAM_TEXT TEST_NfcTaskInit(SUBSYSTEM_MEM_BASE * pFreeMemBase);

void MCU1_DRAM_TEXT MCU1_AllocateTraceLogMemory(U32 *pFreeDramBase) 
{ 
    U32 ulTraceLogSize;
    U32 ulFreeDramBase; 

#ifdef L3_UNIT_TEST
    return;
#endif

    COM_MemAddrPageBoundaryAlign(pFreeDramBase); 
    ulFreeDramBase = *pFreeDramBase; 

    /* allocate all free DRAM as Trace Log buffer */
    /* patch for 2CS DRAM in C0 */
    if (g_ulDramTotalSize >= 0x20000000) // 512M or 1G DRAM
    {
        if (ulFreeDramBase > (DRAM_DATA_BUFF_MCU1_BASE + DATA_BUFF_MCU1_SIZE))
        {
            DBG_Printf("MCU1 AllocateTraceLogMemory allocate 2 CS patch 0x%x ERROR!!! \n", ulFreeDramBase);
            DBG_Getch();
        }
        else
        {

            ulTraceLogSize = DRAM_MCU1_TRACE_BUF_SIZE;
            DBG_Printf("MCU1 AllocateTraceLogMemory allocate 2 CS patch TL size 0x%x \n", ulTraceLogSize);
        }
    }
    else
    {
        if ((ulFreeDramBase - DRAM_DATA_BUFF_MCU1_BASE) > (DRAM_DATA_BUFF_MCU2_BASE - DRAM_DATA_BUFF_MCU1_BASE))
        {
            DBG_Printf("MCU1 allocated %d Bytes,overflow %d Kb!\n", ulFreeDramBase - DRAM_DATA_BUFF_MCU1_BASE, (ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE) / 1024);
            DBG_Getch();
        }
        ulTraceLogSize = (DRAM_DATA_BUFF_MCU2_BASE - DRAM_DATA_BUFF_MCU1_BASE) - (ulFreeDramBase - DRAM_DATA_BUFF_MCU1_BASE);
    }


    if (ulTraceLogSize > SUBSYSTEM_TL_MEM_SIZE_MAX)
    {
        ulTraceLogSize = SUBSYSTEM_TL_MEM_SIZE_MAX;
    }

    TL_Initialize(HAL_GetMcuId(), (void *)ulFreeDramBase, ulTraceLogSize, SEC_SIZE); 

    COM_MemIncBaseAddr(&ulFreeDramBase, ulTraceLogSize); 

    *pFreeDramBase = ulFreeDramBase; 
    return; 
}

/****************************************************************************
Name        :BootUpInit
Input       : None 
Output      :None
Author      :
Date        :
Description : Basic initialization for global pointers and event storage area.
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT BootUpInit()
{
    g_bL1BackgroundTaskEnable = FALSE;
    COM_MemZero((U32 *)g_pMCU12MiscInfo, (sizeof(MCU12_COMMON_MISC_INFO) >> DWORD_SIZE_BITS));

    /* Copies boot loader image from OTFB to DRAM and prepares to program it to flash. */
    if (BOOT_METHOD_LLF == DiskConfig_GetBootMethod())
    {
        //HEAD_SaveBootloaderIntoDRAM();
    }

    BootUpInitMultiCore();

    return;
}

void MCU1_DRAM_TEXT SubSystemInitCheck(void)
{
    U32 ulAllocSize;

    if(MCU1_ID == HAL_GetMcuId())    
    {
        /* Checking whether allocated free SRAM0 space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulFreeSRAM0Base - DSRAM0_MCU1_BASE;
        if(ulAllocSize > DSRAM0_MCU1_MAX_SIZE )
        {
            DBG_Printf("MCU1 Free SRAM0 allocate %d K Bytes! overflowed %d Btyes \n", (ulAllocSize / (1024)), ulAllocSize - DSRAM0_MCU1_MAX_SIZE);
            DBG_Getch();
        }
        
        DBG_Printf("MCU1 Free SRAM0 allocate %d K Bytes! \n", (ulAllocSize / (1024)));

        /* Checking whether allocated free SRAM1 space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulFreeSRAM1Base - DSRAM1_MCU1_BASE;
        if(ulAllocSize > (DSRAM1_MCU01_SHARE_BASE - DSRAM1_MCU1_BASE))
        {
            DBG_Printf("MCU1 Free SRAM1 allocate %d K Bytes! overflowe %d Bytes \n", (ulAllocSize / (1024)), (g_FreeMemBase.ulFreeSRAM1Base - DSRAM1_MCU01_SHARE_BASE));
            DBG_Getch();
        }
        
        DBG_Printf("MCU1 Free SRAM1 allocate %d K Bytes!\n", (ulAllocSize / (1024)));

        /* Checking whether allocated free DRAM space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulDRAMBase - DRAM_DATA_BUFF_MCU1_BASE;
        if(ulAllocSize > (DRAM_DATA_BUFF_MCU2_BASE - DRAM_DATA_BUFF_MCU1_BASE))
        {
            DBG_Printf("MCU1 Free DRAM allocate %d M Bytes! overflowed (%d-%d) Bytes = %dKb !!! \n", (ulAllocSize / (1024 * 1024)), ulAllocSize, 
                DRAM_DATA_BUFF_MCU2_BASE - DRAM_DATA_BUFF_MCU1_BASE, (ulAllocSize - (DRAM_DATA_BUFF_MCU2_BASE - DRAM_DATA_BUFF_MCU1_BASE))/1024);
            DBG_Getch();
        }
        DBG_Printf("MCU1 Free DRAM allocate %d K Bytes! \n", (ulAllocSize / (1024)));
#ifdef SIM
        ulAllocSize = g_FreeMemBase.ulDRAMBase - DRAM_START_ADDRESS;
        if (ulAllocSize > (DRAM_ALLOCATE_SIZE))
        {
            DBG_Printf("MCU1 Free DRAM allocate %d M Bytes! overflowed %d KBytes!!! \n",
                (g_FreeMemBase.ulDRAMBase - DRAM_DATA_BUFF_MCU1_BASE) / 1024 / 1024, (ulAllocSize - (DRAM_ALLOCATE_SIZE)) / 1024);
            DBG_Getch();
        }
#endif
        /* checking for 2CS DRAM patch in C0 */
        if (g_ulDramTotalSize >= 0x20000000) // 512M or 1G DRAM
        {
            if (g_FreeMemBase.ulDRAMBase > (DRAM_DATA_BUFF_MCU1_BASE + DATA_BUFF_MCU1_SIZE))
            {
                DBG_Printf("MCU1 DRAM allocate 2 CS patch 0x%x ERROR!!! \n", g_FreeMemBase.ulDRAMBase);
                DBG_Getch();
            }
        }

        /* Checking whether allocated free OTFB space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulFreeOTFBBase - OTFB_FW_DATA_MCU1_BASE;
        if(ulAllocSize > (OTFB_FW_DATA_MCU2_BASE - OTFB_FW_DATA_MCU1_BASE))
        {
            DBG_Printf("MCU1 Free OTFB allocate %d K Bytes! overflowed !!! \n", (ulAllocSize / (1024)));
            DBG_Getch();
        }
        
        DBG_Printf("MCU1 Free OTFB allocate %d K Bytes! \n", (ulAllocSize / (1024)));
    }
    else
    {
        /* Checking whether allocated free SRAM0 space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulFreeSRAM0Base - DSRAM0_MCU2_BASE;
        if(ulAllocSize > DSRAM0_MCU2_MAX_SIZE )
        {
            DBG_Printf("MCU2 Free SRAM0 allocate %d K Bytes! overflowed !!! \n", (ulAllocSize / (1024)));
            DBG_Getch();
        }
        
        DBG_Printf("MCU2 Free SRAM0 allocate %d K Bytes! \n", (ulAllocSize / (1024)));

        /* Checking whether allocated free SRAM1 space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulFreeSRAM1Base - DSRAM1_MCU2_BASE;
        if (ulAllocSize > (DSRAM1_MCU12_SHARE_BASE - DSRAM1_MCU2_BASE))
        {
            DBG_Printf("MCU2 Free SRAM1 allocate %d K Bytes! overflowed !!! \n", (ulAllocSize / (1024)));
            DBG_Getch();
        }
        
        DBG_Printf("MCU2 Free SRAM1 allocate %d K Bytes! \n", (ulAllocSize / (1024)));
        
        /* Checking whether allocated free DRAM space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulDRAMBase - DRAM_DATA_BUFF_MCU2_BASE;
        if(ulAllocSize > (DRAM_DATA_BUFF_MCU2_BASE - DRAM_DATA_BUFF_MCU1_BASE))
        {
            DBG_Printf("MCU2 Free DRAM allocate %d M Bytes! overflowed !!! \n", (ulAllocSize / (1024 * 1024)));
            DBG_Getch();
        }
        
        DBG_Printf("MCU2 Free DRAM allocate %d M Bytes! \n", (ulAllocSize / (1024 * 1024)));
        
        /* Checking whether allocated free OTFB space is overflowed. */
        ulAllocSize = g_FreeMemBase.ulFreeOTFBBase - OTFB_FW_DATA_MCU2_BASE;
        if(ulAllocSize > (OTFB_FW_DATA_MCU2_BASE - OTFB_FW_DATA_MCU1_BASE))
        {
            DBG_Printf("MCU2 Free OTFB allocate %d K Bytes! overflowed !!! \n", (ulAllocSize / (1024)));
            DBG_Getch();
        }
        
        DBG_Printf("MCU2 Free OTFB allocate %d K Bytes! \n", (ulAllocSize / (1024)));
    }

    return;
}

void MCU1_DRAM_TEXT SubSystem_Init(void)
{
    /* check is current MCU Enable */
    if (FALSE == HAL_IsMCUEnable(HAL_GetMcuId()))
    {
        DBG_Printf("MCU %d Disabled: enter forever loop!\n", HAL_GetMcuId());
        while(TRUE);
    }

#ifndef SIM
    DBG_Getch_Init();
#endif

    DiskConfig_Init();

    DiskConfig_Check();

    /* enable I-Cache */
    HAL_EnableICache(HAL_GetMcuId());

#ifdef DCACHE
    HAL_EnableDramAddrHigh();
#endif

    if (TRUE == DiskConfig_IsColdStart())
    {
        BootUpInit();
    
        HAL_CommInit();

#ifdef HOST_SATA
        HAL_SATAHostIOInit();
#else
        HAL_PCIeHostIOInit();
#endif

#ifndef FLASH_TEST 
#ifdef HAL_NFC_TEST
    TEST_NfcTaskInit(&g_FreeMemBase);  
#else

#ifndef L1_FAKE
        L1_TaskInit();
#else
        L1_TaskRamdiskInit(&g_FreeMemBase);
#endif


#if (!defined(L1_FAKE)) && (!defined(L2_FAKE))
   
        #ifndef HAL_UNIT_TEST
        L2_TaskInit();
        #endif

        #ifdef L3_UNIT_TEST
        extern void MCU1_DRAM_TEXT UT_MemInit(void);
        UT_MemInit();
        #endif

#elif defined(L2_FAKE)
        L2_RamdiskInit( &g_FreeMemBase.ulDRAMBase );
#endif
#endif
#endif//FLASH_TEST

#ifdef DATA_MONITOR_ENABLE
        FW_DataMonitorInit( &g_FreeMemBase.ulDRAMBase );
#endif
        FW_ViaCmdInit( &g_FreeMemBase.ulDRAMBase );

        FW_InitSMSG();
        
        FW_InitMSD();

        *g_pMCU1DramEndBase = g_FreeMemBase.ulDRAMBase;

        //allocate memory for TraceLog and initialize TL module
        MCU1_AllocateTraceLogMemory( &g_FreeMemBase.ulDRAMBase );
    
        SubSystemInitCheck();
    
#ifndef SIM
        HAL_InitInterrupt(  TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0,
                            BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0);
#endif
    }
    /* WARM_START */
    else
    {
        //HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SUBSYS_INIT);
        //HAL_FlashInit();
        //HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SUBSYS_INIT);
        HAL_CommInitForWarmStart();

        #ifdef VT3533_A2ECO_DSGRLSBUG
        HAL_NormalDsgFifoInit();
        #endif


#ifndef SIM
        HAL_InitInterrupt(  TOP_MCU0_INTSRC_MCU1_0 | TOP_MCU0_INTSRC_MCU2_0,
                            BIT_ORINT_MCU1_0 | BIT_ORINT_MCU2_0);
#endif

        DBG_Printf("MCU %d WARM_START\n", HAL_GetMcuId());
    }

#ifdef DCACHE
    /* enable D-Cache */
    HAL_EnableDCache(HAL_GetMcuId());
    if (TRUE == HAL_GetTraceLogEnFlag())
        dbg_tracelog_init();
#endif

#ifdef SIM
    HAL_NfcBuildPuMapping(HAL_NfcGetPhyPUBitMap());
    HAL_FlashInitSLCMapping();
#endif

    return;
}

void SubSystem_TaskSchedule(void)
{
    if (L1_RamdiskIsEnable())
    {
        L1_RamdiskSchedule();
    }
#ifndef L1_FAKE
    else
    {
        while (FALSE != L1_TaskSchedule())
        {
            ;
        }

#ifndef L2_FAKE
        TL_PERFORMANCE(PTL_LEVEL_TOP, "L2_Schedule start");
        L2_Scheduler();        
        TL_PERFORMANCE(PTL_LEVEL_TOP, "L2_Schedule end");
#else
        L2_RamdiskSchedule();
#endif  //#ifndef L2_FAKE
    }
#endif  //#ifndef L1_FAKE
}

#ifndef L1_FAKE
void RDT_Schedule(void)
{
    BOOT_STATIC_FLAG *pBootStaticFlag;

    pBootStaticFlag = &(HAL_GetPTableAddr()->sBootStaticFlag);

    //pBootStaticFlag->bsRdtTestEnable = 1;
    //pBootStaticFlag->bsBootMethodSel = 0;
    if (1 == pBootStaticFlag->bsRdtTestEnable && 0 == pBootStaticFlag->bsBootMethodSel)
    {
        while (TRUE)
        {
            L3_RDTTest();
        }
    }

    return;
}
#endif

void FTL_Init(void)
{
    SubSystem_Init();

    /*MP tool need MCU2 to save FW in Ramdisk mode, so must to wake up MCU2 */
//#if !(defined(L1_FAKE) || defined(L2_FAKE))
    HAL_StartSubSystemMCU(1);
//#endif

    return;
}

void FTL_Schedule(void)
{
#ifdef RDT_SORT
   L3_RDTTest();
#endif

#ifdef L3_UNIT_TEST
    L3_UnitTest();
#endif

#if (!defined(L3_UNIT_TEST) && !(defined(HAL_UNIT_TEST) && defined(SIM)) && !defined(RDT_SORT))
    SubSystem_TaskSchedule();
#endif
}

#ifndef SIM


/****************************************************************************
Name        :main
Input       :None
Output      :None
Author      :
Date        :
Description : Firmware C program entry. After firmware program and data have been correctly
distributed to memory space this routine is called. The name is cmain instead of
main since main is occupied by simulation program for Windows environment.
Others      :
Modify      :
****************************************************************************/
int main(void)
{
    if (HAL_GetPrinttoDRAMEnFlag())
    {
        dbg_putchar_init();
    }

    FTL_Init();
    
#ifdef FLASH_TEST
    while(1);
#endif

#ifdef HAL_NFC_TEST   
    /* set MCU1 event to wait MCU2 BackEnd_Init done */
    CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_INIT);
    
    while (TRUE)
    {
        TEST_NfcMCU1Main();
    }
#endif
    #ifndef L1_FAKE
    RDT_Schedule();
    #endif

    while (TRUE)
    {
        FTL_Schedule();        
    }
    
    return 0;
}
#endif
/********************** FILE END ***************/


