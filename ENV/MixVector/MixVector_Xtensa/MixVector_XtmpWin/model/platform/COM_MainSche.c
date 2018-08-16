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
#include "HAL_Inc.h"
#include "model_common.h"
#include "model_config.h"
#include "xt_library.h"
#include "HostModel.h"
#include "action_define.h"
#include "COM_MainSche.h"

//#include "L2_Evaluater.h"
#include "action_define.h"

WIN_SIM_MGR g_WinSimMgr = {0};

U32 SIM_MEM_BASE;
U32 SIM_SRAM0_BASE;
U32 SIM_SRAM1_BASE;
U32 SIM_DRAM_BASE;
U32 SIM_OTFB_BASE;
U32 SIM_APB_BASE;
U32 SIM_BSS_BASE;

//extern CRITICAL_SECTION g_PuBitmapCriticalSection;

#define SIM_BSS_SIZE   (SIM_PER_MCU_BSS_SIZE * 3)

#define SIM_MEM_SIZE  (DRAM_ALLOCATE_SIZE +  \
                        DSRAM0_ALLOCATE_SIZE + \
                        DSRAM1_ALLOCATE_SIZE +  \
                        APB_SIZE + \
                        SIM_BSS_SIZE + \
                        OTFB_ALLOCATE_SIZE)

extern void COM_MemZero(U32* TargetAddr,U32 LengthDW);
extern void HostInterface_Schedule(void);
extern void HostInterface_Init();
extern void SubSystem_Schedule(void);
extern void SIM_ModelInit(void);
extern void SubSystem_Init(void);

extern void L2_InitDebugPMT();
extern BOOL L3_IsAllFCmdFifoEmpty(U8 ucSkipPu);

extern void SIM_DevNormalRun();
extern void SIM_DevPowerUP();
extern void SIM_DevPowerDown();
extern void SIM_DevRedoLLF();
extern BOOL NFC_IsExit();
extern void SIM_BakupTableRebuildInfo(U8 uTableRebuildSource);

extern void DBG_DumpPBITEraseCnt();

GLOBAL U32 g_DebugPMTBaseAddr1 = 0;
GLOBAL U32 g_DebugPMTBaseAddr2 = 0;

#define  G_PAGE_SIZE ((1024*1024*1024)/ PG_SZ)

void Sim_ClearDram()
{

    COM_MemZero((U32 *)DRAM_DATA_BUFF_MCU1_BASE, (g_DebugPMTBaseAddr1 - DRAM_DATA_BUFF_MCU1_BASE) / 4);

#ifndef SINGLE_SUBSYSTEM
    COM_MemZero((U32 *)DRAM_DATA_BUFF_MCU2_BASE, (g_DebugPMTBaseAddr2 - DRAM_DATA_BUFF_MCU2_BASE) / 4);
#endif

}

void Set_DebugPMTBaseAddr(U32* pFreeDramBase)
{
    if(MCU1_ID == XT_RSR_PRID())  
    {
        g_DebugPMTBaseAddr1 = *pFreeDramBase;
    }
    else
    {
        g_DebugPMTBaseAddr2 = *pFreeDramBase;
    }
}

U32 SIM_GetMCUTreadID(U8 uMCUID)
{
    return (U32)g_WinSimMgr.ulMCUThreadID[uMCUID];
}

void SIM_DevSetStatus(U32 ulStatus)
{
    g_WinSimMgr.ulStatus = ulStatus;

    return;
}

void SIM_MemInit()
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

    memset((void *)SIM_MEM_BASE,0x00,SIM_MEM_SIZE-MisLength);

    SystemStatisticInit();    
}

//initialize system's Reg 
void SIM_SystemRegInit()
{
    rGlbMcuSgeRst = 0x00FFFFFF;
}

void SIM_ParamInit()
{
    U8 uMcuIndex = 0;
    // init simulation event used by mcu
    for (uMcuIndex = 0; uMcuIndex < MCU_MAX; uMcuIndex++)
    {
        g_WinSimMgr.hlMCUSyncEvent[uMcuIndex] = CreateEvent( NULL, FALSE, FALSE, NULL );
        g_WinSimMgr.hlMCUResetEvent[uMcuIndex] = CreateEvent( NULL, TRUE, FALSE, NULL );
    }
    
}

// model scheduler function
void SIM_MCU0ThreadHandler(void)
{
    HostInterface_Schedule();
}

DWORD WINAPI SIM_MCU0Thread(LPVOID p)
{
    BOOL bContinue = TRUE;
    DWORD dwWaitResult = WAIT_TIMEOUT;

    HostInterface_Init();

    while (bContinue)
    {
        
        if (HOST_STATUS_DO_POWERDOWN == g_WinSimMgr.ulStatus ||
            HOST_STATUS_REDO_LLF == g_WinSimMgr.ulStatus ||
            HOST_STATUS_WAIT_L3_IDLE == g_WinSimMgr.ulStatus)
        {
            return 0;
        }

        WaitForSingleObject(g_WinSimMgr.hlMCUSyncEvent[0],INFINITE);
        SIM_MCU0ThreadHandler();

    }

    return 0;
}

void SIM_MCU0Schedule()
{
#ifdef NO_THREAD
    SIM_MCU0ThreadHandler();
#else
    SetEvent(g_WinSimMgr.hlMCUSyncEvent[0]);
#endif
}

void SIM_MCUSchedule(U32 ulMcuID)
{
#ifdef NO_THREAD
    switch (ulMcuID)
    {
        case MCU0_ID:
        {
            SIM_MCU0ThreadHandler();
        }
            break;
        case MCU1_ID:
        case MCU2_ID:
        {
            SIM_MCU1ThreadHandler();
        }
            break;
        default:
            break;
        
    }
#else
    SetEvent(g_WinSimMgr.hlMCUSyncEvent[ulMcuID - MCU0_ID]);
#endif
}

void SIM_MCU1ThreadHandler(void)
{
    SubSystem_Schedule();
}

BOOL SIM_SubSystemIsPowerUp(U32 ulMcuID)
{
    U32 ulGLBMcuRstReg = rGlbMcuSgeRst;
    BOOL bRtn = FALSE;
    U8  ulRstMCUValue = 0;
    U8  ulRstMCUMask = 0;

     // Get RstMcuValue by MCU ID
    if (MCU1_ID == ulMcuID)
    {
        ulRstMCUMask  = 0xF3;
        ulRstMCUValue = ~(R_RST_MCU1 | R_RST_MCU1IF) & ulRstMCUMask;
    }
    else if (MCU2_ID == ulMcuID)
    {
        ulRstMCUMask  = 0xFC;
        ulRstMCUValue = ~(R_RST_MCU2 | R_RST_MCU2IF) & ulRstMCUMask;
    }

    // wait MCU Rst value
    if( (rGlbMcuSgeRst & ulRstMCUMask) != ulRstMCUValue)
    {
        ulGLBMcuRstReg = rGlbMcuSgeRst;
        bRtn = FALSE;
    }
    else
    {
        bRtn = TRUE;
    }
    
    return bRtn;
}

DWORD WINAPI SIM_SubSystemThread(LPVOID p)
{
    
    HANDLE hlResetEvent = 0;
    HANDLE hlSyncEvent = 0;
    
    U32 ulMcuID = XT_RSR_PRID();
   
    BOOL bContinue = FALSE;
    DWORD dwWaitResult = WAIT_TIMEOUT; 

    hlSyncEvent = g_WinSimMgr.hlMCUSyncEvent[ulMcuID - MCU0_ID];
    hlResetEvent = g_WinSimMgr.hlMCUResetEvent[ulMcuID - MCU0_ID];
   
    // wait SubSystem power up
    while (FALSE == SIM_SubSystemIsPowerUp(ulMcuID))
    {
        Sleep(1);
    }

    bContinue = TRUE;
    if (TRUE == bContinue)
    {
        SubSystem_Init();
    }

    while (bContinue)
    {
        if (HOST_STATUS_WAIT_L3_IDLE == g_WinSimMgr.ulStatus)
        {
            if (TRUE == L3_IsAllFCmdFifoEmpty(INVALID_2F))
            {
                DBG_Printf("MCU %d IDLE\n",ulMcuID - 1);
                SIM_BakupTableRebuildInfo(0);

                return 0;
            }
        }
        else if (HOST_STATUS_DO_POWERDOWN == g_WinSimMgr.ulStatus)
        {          
            if (TRUE == NFC_IsExit())
            {
                SIM_BakupTableRebuildInfo(1);
                return 0;
            }
        }
        else if(HOST_STATUS_REDO_LLF == g_WinSimMgr.ulStatus)
        {
        #ifdef DBG_PMT
            L2_InitDebugPMT();
        #endif
            return 0;
        }
        else if(1 == g_WinSimMgr.bReportWL[ulMcuID - 2])
        {
            SIM_DevWLStatistic(ulMcuID);
        }

        WaitForSingleObject(hlSyncEvent,INFINITE);
        SIM_MCU1ThreadHandler();
    }

    return 0;
}

void SIM_MCU1Schedule()
{
#ifdef NO_THREAD
    SIM_MCU1ThreadHandler();
#else
    SetEvent(g_WinSimMgr.hlMCUSyncEvent[1]);
#endif
}

void SIM_MCU2Schedule()
{
#ifdef NO_THREAD
    SIM_MCU1ThreadHandler();
#else
    SetEvent(g_WinSimMgr.hlMCUSyncEvent[2]);
#endif
}


void SIM_StartMcuThread()
{ 
    g_WinSimMgr.hlThreadHandle[0] = CreateThread(0, 0, SIM_MCU0Thread, 0, 0, (LPDWORD)&g_WinSimMgr.ulMCUThreadID[0]);
    g_WinSimMgr.hlThreadHandle[1] = CreateThread(0, 0, SIM_SubSystemThread, 0, 0, (LPDWORD)&g_WinSimMgr.ulMCUThreadID[1]);
#ifndef SINGLE_SUBSYSTEM
    g_WinSimMgr.hlThreadHandle[2] = CreateThread(0, 0, SIM_SubSystemThread, 0, 0, (LPDWORD)&g_WinSimMgr.ulMCUThreadID[2]);
#endif
}


BOOL SIM_MCUIsAllExit()
{
    U8 uMCUIndex = 0;
    BOOL bMCUExit = TRUE;
    for(uMCUIndex = 0; uMCUIndex < MCU_MAX; uMCUIndex++)
    {
        if (g_WinSimMgr.hlThreadHandle[uMCUIndex] != 0 ||
            g_WinSimMgr.ulMCUThreadID[uMCUIndex] != 0)
        {
            bMCUExit = FALSE;
            break;
        }
    }

    return bMCUExit;
}

void SIM_WaitMCUExit()
{
    DWORD ret;
    
    ret = WaitForMultipleObjects(MCU_MAX, g_WinSimMgr.hlThreadHandle, TRUE, IGNORE );
    if (ret == WAIT_OBJECT_0)
    {
        CloseHandle(g_WinSimMgr.hlThreadHandle[0]);
        CloseHandle(g_WinSimMgr.hlThreadHandle[1]);
        CloseHandle(g_WinSimMgr.hlThreadHandle[2]);

        g_WinSimMgr.hlThreadHandle[0] = 0;
        g_WinSimMgr.hlThreadHandle[1] = 0;
        g_WinSimMgr.hlThreadHandle[2] = 0;

        g_WinSimMgr.ulMCUThreadID[0] = 0;
        g_WinSimMgr.ulMCUThreadID[1] = 0;
        g_WinSimMgr.ulMCUThreadID[2] = 0;
    }

}

void SIM_DevWaitL3IDLE()
{
    
    //check mcu thread 
    while (FALSE == SIM_MCUIsAllExit())
    {       
       SIM_WaitMCUExit();
       SIM_DevNormalRun();
    }

    SIM_DevSetStatus(HOST_STATUS_DO_POWERDOWN);
}


void SIM_DevInit()
{
    
    SIM_MemInit();
    SIM_ModelInit();

    SIM_ParamInit();
    g_WinSimMgr.bNeedLLF = TRUE;

    SIM_DevSetStatus(HOST_STATUS_DO_POWERUP);
    return;
}


U8 SIM_DevGetStatus(void)
{
    return g_WinSimMgr.ulStatus;
}

BOOL SIM_DevIsBootUp(void)
{
    BOOL bBootUp = FALSE;
    if ((HOST_STATUS_RUNNING == g_WinSimMgr.ulStatus) ||
        (HOST_STATUS_WAIT_L3_IDLE == g_WinSimMgr.ulStatus))
    {
        bBootUp = TRUE;
    }
    return bBootUp;
}


void SIM_EventInit()
{
    SimEventHandlerArray[HOST_STATUS_INIT] = SIM_DevInit;
    SimEventHandlerArray[HOST_STATUS_DO_POWERUP] = SIM_DevPowerUP;
    SimEventHandlerArray[HOST_STATUS_RUNNING] = SIM_DevNormalRun;
    SimEventHandlerArray[HOST_STATUS_WAIT_L3_IDLE] = SIM_DevWaitL3IDLE;
    SimEventHandlerArray[HOST_STATUS_DO_POWERDOWN] = SIM_DevPowerDown;
    SimEventHandlerArray[HOST_STATUS_REDO_LLF] = SIM_DevRedoLLF;
}


void SIM_SetReportWLstatistic()
{
    U32 ulMcuIndex;

    for(ulMcuIndex = 0; ulMcuIndex < MCU_MAX - 1; ulMcuIndex++)
    {
        g_WinSimMgr.bReportWL[ulMcuIndex] = 1;
    }
    return;
}

void SIM_ClearReportWLstatistic(U32 ulMcuID)
{
    g_WinSimMgr.bReportWL[ulMcuID - 2] = 0;
    return;
}

BOOL SIM_CheckReportWLstatistic()
{
    BOOL bRtn = TRUE;
    U32 ulMcuIndex;
    for(ulMcuIndex = 0; ulMcuIndex < MCU_MAX - 1; ulMcuIndex++)
    {
        if(0 != g_WinSimMgr.bReportWL[ulMcuIndex])
        {
            bRtn = FALSE;
            break;
        }
    }
    return bRtn;
}

void SIM_DevWLStatistic(U32 ulMcuID)
{
#ifdef SIM

    if(_access(".\\WLstatistic", 0)!=0) system("md .\\WLstatistic");
    
    DBG_DumpPBITEraseCnt();

    DBG_Printf("MCU %d reprot WL Done!\n",ulMcuID-1);

    SIM_ClearReportWLstatistic(ulMcuID);

#endif // SIM
}




