/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :bootup.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    16:18:28
Description :Sata interface function
Others      :
Modify      :
****************************************************************************/
#include <stdio.h>
#include "BaseDef.h"


#include "HAL_Inc.h"
#include "L1_Define.h"
#include "L1_HostCmdPRO.h"
#include "L1_Cache.h"
#include "L1_Buffer.h"
#include "L1_Interface.h"
#include "L1_Schedule.h"

#include "COM_Head.h"
#include "COM_Event.h"
#include "COM_Bootup.h"
#include "HAL_Define.h"

#ifdef SIM
#include <Windows.h>
#else
#include <xtensa/hal.h>
#include "HAL_Interrupt.h"
#endif

U32* pBootMethodSeletor;
U32  g_ulSaveDramSize;

#ifndef SIM
volatile U32 g_dbg_enable;
void DRAM_ATTR DBG_Getch()
{
    U32 nTestLoop = 0;

    while(g_dbg_enable)
    {
        nTestLoop++;
    }
    g_dbg_enable = 1;
}
#endif

U32 DRAM_ATTR BootUpInitL1(U32* FreeDramBase, U32* FreeOTFBBase, U32* SaveDramBase)
{
    COMMON_EVENT Event;
    U32 ulFreeDramBase;
    U32 ulFreeOTFBBase;
    U32 ulSaveDramBase;
    COMM_EVENT_PARAMETER * pParameter;


    ulFreeDramBase = *FreeDramBase;
    ulFreeOTFBBase = *FreeOTFBBase;
    ulSaveDramBase = *SaveDramBase;


    //get parameter from comm event
    CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);

    //DWORD0 free dram address,DW1 free otfb sram address
    pParameter->EventParameterNormal[0] = ulFreeDramBase;
    pParameter->EventParameterNormal[1] = ulFreeOTFBBase;
    pParameter->EventParameterNormal[2] = ulSaveDramBase;

    //set init event
    CommSetEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_INIT);

    //L1_TaskSchedule();

    do 
    { 
        L1_TaskSchedule();
        CommCheckEvent(COMM_EVENT_OWNER_L1,&Event);
    } while (Event.EventInit);
    

    //update the free dram/sram address to parameter(status) buffer
    *FreeDramBase = pParameter->EventParameterNormal[0];
    *FreeOTFBBase = pParameter->EventParameterNormal[1];
    *SaveDramBase = pParameter->EventParameterNormal[2];

    return BOOTUP_OK;
}

U32 DRAM_ATTR BootUpInitL2(U32* FreeDramBase, U32* FreeOTFBBase, U32* SaveDramBase)
{
    //RAMDISK
    return TRUE;
}

U32 DRAM_ATTR BootUpInitL3(U32* FreeDramBase, U32* FreeOTFBBase, U32* SaveDramBase)
{
    //RAMDISK
    return TRUE;
}

U32 DRAM_ATTR BootUpNormalBootL1()
{
    return BOOTUP_OK;
}

U32 DRAM_ATTR BootUpNormalBootL2(U32 *TableBlockAddr)
{
    return BOOTUP_OK;
}


U32 DRAM_ATTR BootUpNormalBootL3(U32 *TableBlockAddr)
{
    return BOOTUP_OK;
}



U32 DRAM_ATTR BootUpRebuildBuildL2(U32* TableBlockAddr)
{
    return BOOTUP_OK;
}



U32 DRAM_ATTR BootUpRebuildBuildL3(U32* TableBlockAddr)
{
    return BOOTUP_OK;
}


U32 DRAM_ATTR BootUpLLFL3(U32 bLLFModeL3,U32 *pGlobalPage)
{
    return BOOTUP_OK;
}



U32 DRAM_ATTR BootUpLLFL2()
{
    return BOOTUP_OK;
}


void DRAM_ATTR BootUpInit()
{
#ifndef OTFB_VERSION
    pBootMethodSeletor = (U32*)STATIC_PARAMETER_BOOT_SELECTOR_FLAG;
#endif
    CommEventInit();
}


U32 DRAM_ATTR BootUpLLF()
{
    if(BOOTUP_OK !=BootUpLLFL3(LLF_L3FLAG_LLFMODE_NEWNONE,NULL))
    {
        return BOOTUP_FAIL;
    }

    if (BOOTUP_OK != BootUpLLFL2())
    {
        return BOOTUP_FAIL;
    }

    return BOOTUP_OK;
}


U32 DRAM_ATTR BootUpNormalBoot()
{
#ifndef L2_FORCE_VIRTUAL_STORAGE
    U32 TableBlockAddr[PU_NUM];
#endif

    BootUpNormalBootL1();

#ifndef L2_FORCE_VIRTUAL_STORAGE
    if(BOOTUP_OK !=BootUpNormalBootL3(TableBlockAddr))
    {
        DBG_Printf("BOOTUP_FAIL_L3\n");
        return BOOTUP_FAIL_L3;
    }

    if(BOOTUP_OK !=BootUpNormalBootL2(TableBlockAddr))
    {
         DBG_Printf("BOOTUP_FAIL_L2\n");
         return BOOTUP_FAIL_L2;
    }
        
#endif

    return BOOTUP_OK;
}

U32 DRAM_ATTR BootUpRebuild()
{
    U32 TableBlockAddr[PU_NUM];

    if(BOOTUP_OK !=BootUpRebuildBuildL3(TableBlockAddr))
        return BOOTUP_FAIL_L3;

    if(BOOTUP_OK !=BootUpRebuildBuildL2(TableBlockAddr))
        return BOOTUP_FAIL_L2;

    return BOOTUP_OK;
}

U32 DRAM_ATTR BootUpShutDownL3(U32* TableBlockAddr)
{
    return BOOTUP_OK;
}


U32 DRAM_ATTR BootUpShutdown()
{
    U32 TableBlockAddr[PU_NUM];

    return BootUpShutDownL3(TableBlockAddr);



}


U32 DRAM_ATTR BootUp()
{
    U32 ulFreeDramBase;
    U32 ulFreeOTFBBase;
    U32 ulSaveDramBase;
    U32 Status;
    U32 ulL2DRAMUsed;

    ulFreeDramBase = DRAM_FREE_BASE;
    ulFreeOTFBBase = OTFB_FREE_BASE;
    ulSaveDramBase = DRAM_SAVE_BASE;
    g_ulSaveDramSize = 0;

    BootUpInit();

    //step 1:init
    BootUpInitL1(&ulFreeDramBase, &ulFreeOTFBBase, &ulSaveDramBase);

    return BOOTUP_OK;
}
extern U32    g_L1LoopCount;
extern U32    g_L3LoopCount;
extern U32    g_L2LoopCount;

U32 g_BootUpOk = 0;/* add by henryluo for normal bootup test */

U32 FirmwareMain(U32 FirwmareParameter)
{

    if (BOOTUP_OK == BootUp())
    {
        g_BootUpOk = 1;
        
#ifdef SIM_XTENSA
        rTracer = 0xC0011111;
#endif

#ifndef SIM

        HAL_DisableMCUIntAck();
        HAL_InitInterrupt(TOP_INTSRC_SDC,BIT_ORINT_SDC);
        HAL_EnableMCUIntAck();
#endif
        //   MCUTaskMain();
    }
    else
    {
        DBG_Getch();
    }

    DBG_Printf("boot up ok !\n");

#ifdef SIM_XTENSA
        rTracer = TL_FW_BOOTUP;
#endif
    
    #if 0
    Dbg_TestDSG();
    DBG_Printf("DSG allocate/release test ok\n");
    #endif

#ifndef SIM
    while(1) 
    {           
        HAL_UartDBG();
        MCU0TaskSchedule();
    }
#endif

    return 0;
}

#ifndef SIM


//int cmain(void)
void HAL_MpSataMain (void)
{
    U32 boot_param;
    DBG_TRACE(TRACE_SATA_MP);
    DBG_Printf("Enter sata mp...\n");
    FirmwareMain(boot_param);

//    return 0;
}
#endif


/********************** FILE END ***************/
