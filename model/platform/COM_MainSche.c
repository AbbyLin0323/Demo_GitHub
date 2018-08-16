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
#include "FW_Event.h"

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

extern void FrontEnd_Init(void);
extern void FrontEnd_Schedule(void);
extern void FTL_Init(void);
extern void FTL_Schedule(void);
extern void BackEnd_Init(void);
extern void BackEnd_Schedule(void);

extern void SIM_ModelInit(void);
extern void SubSystem_Init(void);

extern void L2_InitDebugPMT(void);
extern BOOL L2_FCMDQIsAllEmpty(void);
extern BOOL IsDuringTableRebuild();
extern BOOL IsDuringRebuildDirtyCnt(U32 ulMcuId);

extern void SIM_DevNormalRun(void);
extern void SIM_DevPowerUP();
extern void SIM_DevHandshake();
extern void SIM_DevPowerDown();
extern void SIM_DevRedoLLF();
extern BOOL NFC_IsExit();
extern void SIM_BakupTableRebuildInfo(U8 uTableRebuildSource);

extern void DBG_DumpPBITEraseCnt();
extern void L2_BbtPrintAllBbt(void);

GLOBAL U32 g_DebugPMTBaseAddr1 = 0;
GLOBAL U32 g_DebugPMTBaseAddr2 = 0;

LOCAL char l_strLogFolder[256] = {0};

CRITICAL_SECTION g_ThreadIDCriticalSection[MCU_MAX];

void Sim_ClearDram()
{
    BOOTLOADER_FILE *pBootLoader;
    U32 ulMCU2DataBuffSize;
    U32 ulMCU2DramBase = DRAM_DATA_BUFF_MCU2_BASE;
    U32 ulMCU2BufSize = (DRAM_DATA_BUFF_MCU2_BASE - DRAM_START_ADDRESS);
    //Clear SRAM
    memset((void *)IF_SRAM_BASE, 0xCC, DRAM_DATA_BUFF_MCU1_BASE - IF_SRAM_BASE);

    //Clear DRAM
    memset((void *)DRAM_DATA_BUFF_MCU1_BASE, 0xCC, (g_DebugPMTBaseAddr1 - DRAM_DATA_BUFF_MCU1_BASE));

    pBootLoader = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
    ulMCU2DataBuffSize = pBootLoader->tSysParameterTable.tHALFeature.aFeatureDW[0] - ulMCU2BufSize;
    memset((void *)ulMCU2DramBase, 0xCC, ulMCU2DataBuffSize);//DATA_BUFF_MCU2_SIZE

    //Clear OFTB
    memset((void *)SIM_OTFB_BASE, 0xCC, (SIM_BSS_BASE - SIM_OTFB_BASE + SIM_BSS_SIZE));
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
    U32 ulMCUThreadID;

    EnterCriticalSection(&g_ThreadIDCriticalSection[uMCUID]);
    ulMCUThreadID = (U32)g_WinSimMgr.ulMCUThreadID[uMCUID];
    LeaveCriticalSection(&g_ThreadIDCriticalSection[uMCUID]);

    return ulMCUThreadID;
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

    SIM_MEM_BASE = (U32)malloc(SIM_MEM_SIZE + LOGIC_PIPE_PG_SZ);
    MisLength = 0;

    if (SIM_MEM_BASE&LOGIC_PIPE_PG_SZ_MSK)
    {
        MemNewStart = ((SIM_MEM_BASE >> LOGIC_PIPE_PG_SZ_BITS) + 1) << LOGIC_PIPE_PG_SZ_BITS;
        MisLength = MemNewStart - SIM_MEM_BASE;
        SIM_MEM_BASE = MemNewStart;
    }
    else
    {
        SIM_MEM_BASE = (SIM_MEM_BASE >> LOGIC_PIPE_PG_SZ_BITS) << LOGIC_PIPE_PG_SZ_BITS;
    }

    SIM_SRAM0_BASE = SIM_MEM_BASE;
    SIM_SRAM1_BASE = SIM_SRAM0_BASE + DSRAM0_ALLOCATE_SIZE;
    SIM_DRAM_BASE = SIM_SRAM1_BASE + DSRAM1_ALLOCATE_SIZE;
    SIM_OTFB_BASE = SIM_DRAM_BASE + DRAM_ALLOCATE_SIZE;
    SIM_APB_BASE = SIM_OTFB_BASE + OTFB_ALLOCATE_SIZE;
    SIM_BSS_BASE = SIM_APB_BASE + APB_SIZE;

    memset((void *)SIM_SRAM0_BASE, 0xCC, SIM_MEM_SIZE - MisLength);
    memset((void *)SIM_SRAM0_BASE, 0x00, IF_SRAM_BASE - SIM_SRAM0_BASE);

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

LOCAL void SIM_MCU0ThreadInit(void)
{
    FrontEnd_Init();
}
LOCAL void SIM_MCU0ThreadHandler(void)
{
    FrontEnd_Schedule();
}
LOCAL void SIM_MCU1ThreadInit(void)
{
    FTL_Init();
}
LOCAL void SIM_MCU1ThreadHandler(void)
{
    FTL_Schedule();
}
LOCAL void SIM_MCU2ThreadInit(void)
{
    BackEnd_Init();
}
LOCAL void SIM_MCU2ThreadHandler(void)
{
    BackEnd_Schedule();
}

LOCAL BOOL SIM_MCU0ThreadFinish(void)
{
    BOOL bResult = FALSE;

    if (HOST_STATUS_DO_POWERDOWN == g_WinSimMgr.ulStatus ||
        HOST_STATUS_REDO_LLF == g_WinSimMgr.ulStatus ||
        HOST_STATUS_WAIT_L3_IDLE == g_WinSimMgr.ulStatus)
    {
        bResult = TRUE;
    }

    return bResult;
}

extern GLOBAL BOOL L2_FCMDQIsAllEmpty(void);
extern GLOBAL BOOL L3_IFIsAllFCmdIntrQEmpty(void);
LOCAL BOOL SIM_MCU1ThreadFinish(void)
{
    BOOL bResult = FALSE;

    switch (g_WinSimMgr.ulStatus)
    {
        case HOST_STATUS_WAIT_L3_IDLE:
        {
            if (TRUE == L2_FCMDQIsAllEmpty() && FALSE == IsDuringTableRebuild())
            {
                SIM_BakupTableRebuildInfo(0);
                bResult = TRUE;
            }
            break;
        }
        case HOST_STATUS_DO_POWERDOWN:
        {
            if (TRUE == NFC_IsExit())
            {
                SIM_BakupTableRebuildInfo(1);
                bResult = TRUE;
            }
            break;
        }
        case HOST_STATUS_REDO_LLF:
        {
            bResult = TRUE;
            break;
        }
        default:
        {
            if (1==g_WinSimMgr.bReportWL[0])
            {
                SIM_DevWLStatistic(MCU1_ID);
            }
        }
    }

    if (TRUE == g_WinSimMgr.bPrintBadBlock)
    {
        L2_BbtPrintAllBbt();
    }

    return bResult;
}

LOCAL BOOL SIM_MCU2ThreadFinish(void)
{
    COMMON_EVENT L3_Event;
    U32 ulCurEvent;
    BOOL bResult = FALSE;

    ulCurEvent = CommCheckEvent(COMM_EVENT_OWNER_L3, &L3_Event);
    switch (g_WinSimMgr.ulStatus)
    {
        case HOST_STATUS_WAIT_L3_IDLE:
        {
            if ((TRUE == L2_FCMDQIsAllEmpty()) && TRUE == L3_IFIsAllFCmdIntrQEmpty())
            {
                if (WAIT_OBJECT_0 == WaitForSingleObject(g_WinSimMgr.hlThreadHandle[1], 0))
                {
                    bResult = TRUE;
                }
            }
            break;
        }
        case HOST_STATUS_DO_POWERDOWN:
        {
            if ((TRUE == NFC_IsExit()) && (COMM_EVENT_STATUS_SUCCESS_NOEVENT == ulCurEvent))
            {
                bResult = TRUE;
            }
            break;
        }
        case HOST_STATUS_REDO_LLF:
        {
            if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == ulCurEvent)
            {
                bResult = TRUE;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return bResult;
}

DWORD WINAPI SIM_MCU0Thread(LPVOID p)
{
    SIM_MCU0ThreadInit();

    while (TRUE)
    {
        if (TRUE == SIM_MCU0ThreadFinish())
        {
            return 0;
        }

        WaitForSingleObject(g_WinSimMgr.hlMCUSyncEvent[0],INFINITE);
        SIM_MCU0ThreadHandler();
    }

    return 0;
}

DWORD WINAPI SIM_MCU1Thread(LPVOID p)
{
    // wait SubSystem power up
    while (FALSE == SIM_SubSystemIsPowerUp(MCU1_ID))
    {
        Sleep(1);
    }

    SIM_MCU1ThreadInit();

    while (TRUE)
    {
        if (TRUE == SIM_MCU1ThreadFinish())
        {
            return 0;
        }

        WaitForSingleObject(g_WinSimMgr.hlMCUSyncEvent[1],INFINITE);
        SIM_MCU1ThreadHandler();
    }

    return 0;
}


DWORD WINAPI SIM_MCU2Thread(LPVOID p)
{
    // wait SubSystem power up
    while (FALSE == SIM_SubSystemIsPowerUp(MCU2_ID))
    {
        Sleep(1);
    }

    SIM_MCU2ThreadInit();

    while (TRUE)
    {
        if (TRUE == SIM_MCU2ThreadFinish())
        {
            return 0;
        }

        WaitForSingleObject(g_WinSimMgr.hlMCUSyncEvent[2],INFINITE);
        SIM_MCU2ThreadHandler();
    }

    return 0;
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

void SIM_MCUSchedule(U32 ulMcuID)
{
#ifdef NO_THREAD
    switch (ulMcuID)
    {
        case MCU0_ID:
        {
            SIM_MCU0ThreadHandler();
            break;
        }
        case MCU1_ID:
        {
            SIM_MCU1ThreadHandler();
            break;
        }
        case MCU2_ID:
        {
            SIM_MCU2ThreadHandler();
            break;
        }
        default:
        {
            DBG_Getch();
        }
    }
#else
    SetEvent(g_WinSimMgr.hlMCUSyncEvent[ulMcuID-MCU0_ID]);
#endif

    return;
}

void SIM_StartMcuThread(void)
{
    EnterCriticalSection(&g_ThreadIDCriticalSection[0]);
    g_WinSimMgr.hlThreadHandle[0] = CreateThread(0, 0, SIM_MCU0Thread, 0, 0, (LPDWORD)&g_WinSimMgr.ulMCUThreadID[0]);
    LeaveCriticalSection(&g_ThreadIDCriticalSection[0]);
    EnterCriticalSection(&g_ThreadIDCriticalSection[1]);
    g_WinSimMgr.hlThreadHandle[1] = CreateThread(0, 0, SIM_MCU1Thread, 0, 0, (LPDWORD)&g_WinSimMgr.ulMCUThreadID[1]);
    LeaveCriticalSection(&g_ThreadIDCriticalSection[1]);
    EnterCriticalSection(&g_ThreadIDCriticalSection[2]);
    g_WinSimMgr.hlThreadHandle[2] = CreateThread(0, 0, SIM_MCU2Thread, 0, 0, (LPDWORD)&g_WinSimMgr.ulMCUThreadID[2]);
    LeaveCriticalSection(&g_ThreadIDCriticalSection[2]);

}


BOOL SIM_MCUIsAllExit(void)
{
    U8 uMCUIndex = 0;
    BOOL bMCUExit = TRUE;
    for(uMCUIndex = 0; uMCUIndex < MCU_MAX; uMCUIndex++)
    {
        EnterCriticalSection(&g_ThreadIDCriticalSection[uMCUIndex]);
        if (g_WinSimMgr.hlThreadHandle[uMCUIndex] != 0 ||
            g_WinSimMgr.ulMCUThreadID[uMCUIndex] != 0)
        {
            bMCUExit = FALSE;
        }
        LeaveCriticalSection(&g_ThreadIDCriticalSection[uMCUIndex]);

        if (FALSE == bMCUExit)
        {
            break;
        }
    }

    return bMCUExit;
}

void SIM_WaitMCUExit(void)
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

        EnterCriticalSection(&g_ThreadIDCriticalSection[0]);
        g_WinSimMgr.ulMCUThreadID[0] = 0;
        LeaveCriticalSection(&g_ThreadIDCriticalSection[0]);
        EnterCriticalSection(&g_ThreadIDCriticalSection[1]);
        g_WinSimMgr.ulMCUThreadID[1] = 0;
        LeaveCriticalSection(&g_ThreadIDCriticalSection[1]);
        EnterCriticalSection(&g_ThreadIDCriticalSection[2]);
        g_WinSimMgr.ulMCUThreadID[2] = 0;
        LeaveCriticalSection(&g_ThreadIDCriticalSection[2]);

    }

    return;
}

void SIM_DevWaitL3IDLE(void)
{
    //check mcu thread
    while (FALSE == SIM_MCUIsAllExit())
    {
       SIM_WaitMCUExit();
       SIM_DevNormalRun();
    }

    SIM_DevSetStatus(HOST_STATUS_DO_POWERDOWN);
}

void SIM_LogFileInit()
{
    char strLogFolder[] = "\\WinLog";

    _getcwd(l_strLogFolder, sizeof(l_strLogFolder));
    strcat(l_strLogFolder, strLogFolder);

    if (0 != _access(l_strLogFolder, 0))
    {
        if (0 != _mkdir(l_strLogFolder))
        {
            DBG_Printf("There have error id = %d, info = \n",GetLastError());
        }
    }
}

char* SIM_GetLogFileFloder()
{
   return l_strLogFolder;
}

void SIM_DevInit()
{
    SIM_LogFileInit();
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


void SIM_EventInit(void)
{
    SimEventHandlerArray[HOST_STATUS_INIT] = SIM_DevInit;
    SimEventHandlerArray[HOST_STATUS_DO_POWERUP] = SIM_DevPowerUP;
    SimEventHandlerArray[HOST_STATUS_HANDSHAKING] = SIM_DevHandshake;
    SimEventHandlerArray[HOST_STATUS_RUNNING] = SIM_DevNormalRun;
    SimEventHandlerArray[HOST_STATUS_WAIT_L3_IDLE] = SIM_DevWaitL3IDLE;
    SimEventHandlerArray[HOST_STATUS_DO_POWERDOWN] = SIM_DevPowerDown;
    SimEventHandlerArray[HOST_STATUS_REDO_LLF] = SIM_DevRedoLLF;
}

void SIM_EventHandler(void)
{
    if (NULL != SimEventHandlerArray[g_WinSimMgr.ulStatus])
    {
        SimEventHandlerArray[g_WinSimMgr.ulStatus]();
    }
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

