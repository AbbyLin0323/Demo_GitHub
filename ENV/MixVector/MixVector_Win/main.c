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
*******************************************************************************/

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <conio.h>


#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "sim_flash_interface.h"
#include "sim_nfccmd.h"
#include "system_statistic.h"
#include "sim_SGE.h"
#include "sim_HSG.h"
#include "sim_NormalDSG.h"
#include "MixVector_HostApp.h"
#include "win_bootloader.h"
#ifdef SATA_INT
#include "simsatadev.h"
#endif

extern  void sim_module_test(void);

#ifdef AHCI_INT
extern void SIM_HCTRegInit();
extern void SIM_HostCModelSchedule();
extern void L0_init();
extern void L0_Schedule();
#endif



#ifdef L2_FORCE_VIRTUAL_STORAGE
#include "L2_VirtualStorage.h"
#endif

#ifdef  SIM_DBG
extern   TaskStatus ;
#endif

char  debug_buffer[256];
HANDLE hDebugLogFile;
U32   haswrite;

U32 SIM_MEM_BASE;
U32 SIM_SRAM0_BASE;
U32 SIM_SRAM1_BASE;
U32 SIM_DRAM_BASE;
U32 SIM_OTFB_BASE;
U32 SIM_APB_BASE;
U32 SIM_BSS_BASE;

HANDLE sim_mcu_event;
HANDLE sim_mcu_event_end;
CRITICAL_SECTION g_PuBitmapCriticalSection;

U32 rTracerWin;
U32 l_ulSimDevStatus;
extern U32    g_L1LoopCount;
extern U32    g_L3LoopCount;
extern U32    g_L2LoopCount;

#define SIM_BSS_SIZE  (SIM_PER_MCU_BSS_SIZE * 3)
#define SIM_MEM_SIZE  (DRAM_ALLOCATE_SIZE +  \
                        DSRAM0_ALLOCATE_SIZE + \
                        DSRAM1_ALLOCATE_SIZE +  \
                        APB_SIZE + \
                        SIM_BSS_SIZE +\
                        OTFB_ALLOCATE_SIZE)
//void BootUp(void *p);


//tqq
extern void Comm_NFCModelInit(void);
extern void CommDbgEventHandler(void);
extern void Host_ModelInit();
extern void Host_ModelSchedule();
extern int MV_Schedule();
extern void MixVectorInit();
extern void DMAE_ModelInit(void);
extern void SE_ModelInit(void);
extern void HCT_ModelInit();

void SIM_SetReportWLstatistic()
{
    return;
}

U32 SIM_CheckReportWLstatistic()
{
    return 1;
}


void sim_mcu_init()
{
    MixVectorInit();
}



void sim_memory_align(U32 *source_addr,U32 align_bits)
{

}


void sim_init()
{
    U32 MemNewStart;
    U32 MisLength;

    SIM_MEM_BASE = (U32)malloc(SIM_MEM_SIZE + PIPE_PG_SZ);
    MisLength = 0;

    if (SIM_MEM_BASE&PIPE_PG_SZ_MSK)
    {
        MemNewStart = ((SIM_MEM_BASE>>PIPE_PG_SZ_BITS)+1)<<PIPE_PG_SZ_BITS;
        MisLength = MemNewStart - SIM_MEM_BASE;
        SIM_MEM_BASE = MemNewStart;
    }
    else
    {
        SIM_MEM_BASE = (SIM_MEM_BASE>>PIPE_PG_SZ_BITS)<<PIPE_PG_SZ_BITS;
    }

    SIM_SRAM0_BASE = SIM_MEM_BASE;
    SIM_SRAM1_BASE = SIM_SRAM0_BASE + DSRAM0_ALLOCATE_SIZE;
    SIM_DRAM_BASE = SIM_SRAM1_BASE + DSRAM1_ALLOCATE_SIZE;
    SIM_OTFB_BASE = SIM_DRAM_BASE + DRAM_ALLOCATE_SIZE;
    SIM_APB_BASE = SIM_OTFB_BASE + OTFB_ALLOCATE_SIZE;
    SIM_BSS_BASE = SIM_APB_BASE + APB_SIZE;

    memset((void *)SIM_MEM_BASE,0,SIM_MEM_SIZE-MisLength);
#ifdef SIM_DBG
    SystemStatisticInit();
#endif
    InitializeCriticalSection(&g_PuBitmapCriticalSection);
    // InitializeCriticalSection(&lock_host_cmd_queue_lock);
}

void sim_menu_sel(BOOL *pLlf)
{
    //    char c;
    BOOL bllf;
    char ch;
    BOOL b_rebuild_file; 

SELECT_MENU:
    printf("MENU:\n1. rebuild all.\n2. LLF\n3. Normal Boot\n4. Full recovery\n");
    printf("5. VendCmd AP.\n");
#ifdef SIM_FOR_PROFILE
    ch = '3';
#else
    ch = getchar();
#endif
    if(ch == '1')
    {
        printf("press Y confirm to do the llf\n");
        ch = getchar();
        if(ch == 'y' || ch == 'Y')
        {
            ch = '1';
        }
        else
        {
            printf("please reselect\n");
            goto SELECT_MENU;
        }

    }
    else    if(ch == '2')
    {
        printf("press Y confirm to do the llf\n");
        ch = getchar();
        if(ch == 'y' || ch == 'Y')
        {
            ch = '2';
        }
        else
        {
            printf("please reselect\n");
            goto SELECT_MENU;
        }

    }
    switch(ch)
    {
    case '1':

        bllf = TRUE;     
        b_rebuild_file = TRUE;

        break;

    case '2':
        b_rebuild_file = FALSE;
        bllf = TRUE;
        break;

    case '5':
        //vendcmd_host();
        return;
        break;

    case '4':
        bllf = FALSE;
        b_rebuild_file = FALSE;
        break;

    case '3':
    default:
        bllf = FALSE;
        b_rebuild_file = FALSE;
        break;        

    }

    *pLlf = bllf;
}

#define  G_PAGE_SIZE ((1024*1024*1024)/ PG_SZ)

U32 SIM_GetMCUTreadID(U8 uMCUID)
{

    return 0;
}

BOOL SIM_DevIsBootUp(void)
{
    return TRUE;
}

void SIM_DevSetStatus(U32 ulStatus)
{
    l_ulSimDevStatus = ulStatus;

    return;
}

U8 SIM_DevGetStatus(void)
{
    return 1;
}

/*
void sim_model_init(BOOL bllf)
Description:
init nfc/dsg/host/sdc/storage model
*/
void sim_model_init(BOOL bllf)
{

#ifndef L2_FORCE_VIRTUAL_STORAGE
    Comm_NFCModelInit();
#endif


#ifdef SATA_DSG_MODE
    Comm_SataDsgModelInit();
#endif

#if 0
    if (TRUE == bllf)
        *(U32*)STATIC_PARAMETER_BOOT_SELECTOR_FLAG = BOOT_FLAG_FORCE_LLF;
    else
        *(U32*)STATIC_PARAMETER_BOOT_SELECTOR_FLAG = BOOT_FLAG_NORMAL_BOOT;
#endif    
    Host_ModelInit();

    // initialize Host command table for mix vector test.
    MV_CmdInit();

    HCT_ModelInit();
#ifdef AHCI_INT
    SIM_HCTRegInit();
#endif

    SGE_ModelInit();
#ifdef SATA_INT
    SDC_ModelInit();
#endif

    //initialize DMAE model
    DMAE_ModelInit();

    //initialize Search Engine model
    SE_ModelInit();
}

/*
    In some MixVector host checklist, host app needs to waiting device's response by polling mode
    this function is for this usage in Windows ENV.
*/
void MV_DeviceSchedule(void)
{
    MV_Schedule();
    SIM_HostCModelSchedule();
    NFC_ModelSchedule();
    SGE_ModelSchedule();
}

int main(void)
{
    BOOL bllf = TRUE;

    //Host_OpenLogFile();
    sim_init();
    sim_model_init(bllf);
    SIM_WinBootLoader(FALSE);
    sim_mcu_init();
    while(1)
    {
        Host_ModelSchedule();
        MV_Schedule();
		SIM_HostCModelSchedule();
//        L3TaskSchedule(); 
        NFC_ModelSchedule();
        SGE_ModelSchedule();
    }

    return 0;
}

