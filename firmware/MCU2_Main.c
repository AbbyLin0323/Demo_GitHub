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
Filename    :MCU12_main.c
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
#include "HAL_LdpcSoftDec.h"
#include "COM_Memory.h"
#include "Disk_Config.h"
#include "L3_Init.h"
#include "L3_Schedule.h"
#include "L3_FlashMonitor.h"
#include "FW_SMSG.h"
#include "Proj_Config.h"

#ifdef HAL_UNIT_TEST
#include "BkEnd_TestMain.h"
#endif

#ifdef DATA_MONITOR_ENABLE
#include "FW_DataMonitor.h"
#endif

#ifdef DATA_EM_ENABLE
#include "HAL_EncriptionModule.h" 
#endif
#include "L3_DataRecover.h"

#ifndef SIM
#include "HAL_ParamTable.h"
#endif

#define TL_FILE_NUM MCU2_Main_c

extern GLOBAL MCU12_VAR_ATTR SUBSYSTEM_MEM_BASE g_FreeMemBase;

extern U32 HAL_GetMcuId();
extern void HAL_CommInit(void);
extern void HAL_CommInitForWarmStart(void);
#ifdef HOST_SATA
extern void HAL_SATAHostIOInit(void);
#else
extern void HAL_PCIeHostIOInit(void);
#endif
extern void HAL_FlashInit(void);
#ifdef SEARCH_ENGINE
extern void L2_SearchEngineSWInit();
#endif

void MCU2_DRAM_TEXT MCU2_AllocateTraceLogMemory(U32 *pFreeDramBase)
{
    U32 ulTraceLogSize;
    U32 ulFreeDramBase;

#ifdef L3_UNIT_TEST
    return;
#endif

    COM_MemAddrPageBoundaryAlign(pFreeDramBase);
    ulFreeDramBase = *pFreeDramBase;

    /* allocate all free DRAM as Trace Log buffer */
    if ((ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE) > DATA_BUFF_MCU2_SIZE)
    {
        DBG_Printf("MCU2 allocated %d Bytes,overflow %d Kb!\n", ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE, (DATA_BUFF_MCU2_SIZE - (ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE)) / 1024);
        DBG_Getch();
    }
	DBG_Printf("MCU2 allocated %d Bytes\n", ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE);
    ulTraceLogSize = DATA_BUFF_MCU2_SIZE - (ulFreeDramBase - DRAM_DATA_BUFF_MCU2_BASE);

    if (ulTraceLogSize > SUBSYSTEM_TL_MEM_SIZE_MAX)
    {
        ulTraceLogSize = SUBSYSTEM_TL_MEM_SIZE_MAX;
    }

    TL_Initialize(HAL_GetMcuId(), (void *)ulFreeDramBase, ulTraceLogSize, SEC_SIZE);

    COM_MemIncBaseAddr(&ulFreeDramBase, ulTraceLogSize);

    *pFreeDramBase = ulFreeDramBase;
    return;
}

void BackEnd_Init(void)
{
    DiskConfig_Init();
#ifdef DCACHE
    HAL_EnableDramAddrHigh();
#endif
    /* check is current MCU Enable */
    if (FALSE == HAL_IsMCUEnable(HAL_GetMcuId()))
    {
        DBG_Printf("MCU %d Disabled: enter forever loop!\n", HAL_GetMcuId());
        while(TRUE);
    }
    
    BootUpInitMultiCore();

    if (TRUE == DiskConfig_IsColdStart())
    {
        HAL_CommInit();

        #ifdef HOST_SATA
        HAL_SATAHostIOInit();
        #else
        HAL_PCIeHostIOInit();
        #endif

        FW_InitSMSG();
        FW_InitMSD();

        #ifdef SEARCH_ENGINE
        L2_SearchEngineSWInit();
        #endif

        #ifndef HAL_UNIT_TEST
        L3_TaskInit( &g_FreeMemBase );
        #endif

        MCU2_AllocateTraceLogMemory(&g_FreeMemBase.ulDRAMBase);
    }
    else
    {
        HAL_CommInitForWarmStart();

        #ifdef VT3533_A2ECO_DSGRLSBUG
        HAL_NormalDsgFifoInit();
        #endif

        L3_FMIntrInit();
    }

    HAL_FlashInit();

    /* move LDPC DL HMatrix from BL */
    HAL_LdpcDownloadHMatix();

    /*  LDPC SoftDEC init  */
    HAL_LdpcSInit();

#ifdef XOR_ENABLE
    L3_XorInit();
#endif

#if (defined(DATA_EM_ENABLE) && !defined(SIM))
    U32 aIV[4] = {0x12121212, 0x34343434, 0x56565656, 0x78787878};
    HAL_EMInit(EM_KEY_SIZE_256BIT, EM_MODE_XTS, aIV);
#endif

    #ifdef DCACHE
    HAL_EnableDCache(HAL_GetMcuId());
    if (TRUE == HAL_GetTraceLogEnFlag())
        dbg_tracelog_init();
    #endif

    return;
}

void BackEnd_Schedule(void)
{
    #ifdef HAL_UNIT_TEST
    HAL_UnitTest();
    #else
    L3_Scheduler();
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
#ifndef SIM
    if (HAL_GetPrinttoDRAMEnFlag())
    {
        dbg_putchar_init();
    }
#endif

    BackEnd_Init();
    
#ifdef FLASH_TEST
    FT_Main();
#endif

#ifdef HAL_NFC_TEST
    /*  set event to blocking MCU2 */    
    CommSetEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_BOOT);

    /* clear MCU1 event after backend init done to enable MCU1 */
    CommClearEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_INIT);

    while (TRUE)
    {
        TEST_NfcMCU2Main();
    }
#endif


    while (TRUE)
    {
        TL_PERFORMANCE(PTL_LEVEL_TOP, "BackEnd schedule start:");
        BackEnd_Schedule();        
        TL_PERFORMANCE(PTL_LEVEL_TOP, "BackEnd schedule end:");
    }
    return 0;
}
#endif
/********************** FILE END ***************/


