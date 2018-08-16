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

//#include "simsatainc.h"

// include system API
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include "BaseDef.h"

#include "sim_nfccmd.h"
#include "checklist_parse.h"
#include "model_config.h"
#include "win_bootloader.h"
#ifdef DBG_TABLE_REBUILD
#include "L2_SimTablecheck.h"
#endif

#if defined(HOST_AHCI)
#include "AHCI_HostModel.h"
#elif defined(HOST_SATA)
#include "Simsatahost.h"
#elif defined(HOST_NVME)
#include "NVME_ControllerModel.h"
#endif

#include "HostInc.h"
#include "HostModel.h"

#include "action_define.h"

#ifdef SIM_XTENSA
#include "model_common.h"
#endif

#define PATTERN_PATH "D:\\\\pattern"
#define G_PAGE_SIZE ((1024 * 1024 * 1024)/LOGIC_PG_SZ)


LOCAL HANDLE g_SimHostEvent;
LOCAL HANDLE g_hSimHostFileData;
LOCAL HANDLE g_hHostThread;
LOCAL HANDLE l_HostCmdCompletedEvent;
CRITICAL_SECTION g_LbaWriteCntCriticalSection;

extern U32 Trigered_GC;
U32    g_TransTotalTime = 0;
U32    g_TransSecCnt = 0;
volatile BOOL g_bReSetFlag = FALSE;


U32 g_ulHostSendCmdCnt;

WIN32_FIND_DATA FindFileData;
HANDLE hFind;
BOOL bFinish;



U32 TestCaseSN = 0;
U32 RemainCaseSN=0;
U32 true_read_flag =0 ;

extern BOOL SIM_DevIsBootUp(void);
extern BOOL IsDuringRebuildDirtyCnt(U32 ulMcuId);
extern BOOL IsDuringTableRebuild();
extern void ResetTableRebuildFlag();
extern BOOL g_bWearLevelingStatistic;
extern BOOL IsDuringRebuildDirtyCnt(U32 ulMcuId);
extern void Host_SendRWCmdByType(HCMD_INFO* pHcmd);


BOOL Host_IsCMDEmpty()
{
    BOOL bEmpty = FALSE;
    bEmpty = Host_IsCMDEmptyInterface();
    return bEmpty;
}

DWORD WINAPI Host_ModelThread(LPVOID p)
{
    BOOL bContinue = TRUE;
    while (bContinue)
    {
        WaitForSingleObject(g_SimHostEvent,INFINITE);
        if (TRUE == SIM_DevIsBootUp())
        {
            bContinue = Host_ModelProcess();
        }

    }
#ifndef NO_THREAD
    CloseHandle( l_HostCmdCompletedEvent );
    CloseHandle( g_SimHostEvent );
#endif
    return 0;
}

#ifdef SIM_XTENSA
void Host_ModelThread_XTENSA()
{
     BOOL bContinue = TRUE;
    while (bContinue)
    {
        //WaitForSingleObject(g_SimSDCEvent,INFINITE);
        Host_ModelProcess();

        XTMP_wait(1);
    }
    return;
}
#endif


void Host_ModelInit()
{
    // data file init
    Host_OpenDataFile();

    // checkListParser init
    module_run_init();

    // Host Model interface
    Host_ModelInitInterface();

#ifdef SIM
#ifndef NO_THREAD
    // set initial signaled state to start Host model.
    InitializeCriticalSection(&g_LbaWriteCntCriticalSection);
    l_HostCmdCompletedEvent = CreateEvent( NULL, FALSE, TRUE, NULL );
    g_SimHostEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    //g_hHostThread = CreateThread(0, 0, Host_ModelThread, 0, 0, 0);
    g_hHostThread = CreateThread(0, 0, Host_ModelThread, 0, CREATE_SUSPENDED, 0);
    SetThreadPriority(g_hHostThread,THREAD_PRIORITY_HIGHEST);
    ResumeThread(g_hHostThread);
#endif
#endif

}

void Host_ModelSchedule()
{
#ifdef NO_THREAD
    Host_ModelProcess();
#else
    SetEvent(g_SimHostEvent);
    //Host_ModelProcess();
#endif
}

void Host_ClearTimerCount()
{
    g_TransTotalTime = 0;
    g_TransSecCnt = 0;
}

void Host_CmdCompleted( U8 ucTag )
{
    SetEvent( l_HostCmdCompletedEvent );
}

BOOL Host_ModelProcess()
{

    static U8 state = 0;
    switch(state)
    {
    case 0:
        if(TRUE == load_one_checklist())
        {
            state++;
        }
        else
        {
            state = 2;
        }

        break;
    case 1:
        if (TRUE == run_one_checklist())
        {
            if (TRUE == Host_IsCMDEmpty() && FALSE == IsDuringTableRebuild())
            {
                if (FALSE == IsDuringRebuildDirtyCnt(MCU1_ID) && FALSE == IsDuringRebuildDirtyCnt(MCU2_ID))
                {
                    inject_error_init();
                    NFC_ResetErrInjEntry();

#ifdef DBG_TABLE_REBUILD
                    L2_ResetIgnoredLPNOfCheckPMT();
#endif
                    clear_checklist();
                    state = 0;
                }
            }
        }
        break;

    case 2:
        {
            static BOOL bCheckListDone = FALSE;
            if (FALSE == bCheckListDone)
            {
                DBG_Printf("all check list have done\n");
                bCheckListDone = TRUE;
            }
            #if (!defined(HAL_UNIT_TEST) && !defined(L3_UNIT_TEST) && !defined(RDT_SORT))
            #ifdef SCRIPTS_AUTO_RUNNING
            exit(1);
            #else
            DBG_Getch();
            #endif
            #endif
        }
        break;
    default:
        break;

    }
    return TRUE;
}

#ifndef DBG_TABLE_REBUILD
U32 Host_UpdateDataCnt(U32 lba)
{
    U32 cntEx;
    Mid_Read_SATA(lba, (char*)&cntEx, 4);
    cntEx++;


    Host_SaveDataCnt(lba, cntEx);
    return cntEx;
}

U32 Host_GetDataCnt(U32 lba)
{

    U32 cnt;
    int err_code = 0;

    Mid_Read_SATA(lba, (char*)&cnt, 4);

    return cnt;
}
#else
U32 Host_UpdateDataCnt(U32 lba)
{
    U32 cntEx = 0;
    U32 saveCnt = 0;
    EnterCriticalSection(&g_LbaWriteCntCriticalSection);

    Mid_Read_SATA(lba, (char*)&cntEx, 4);

    saveCnt = cntEx & HOST_WRITE_CNT_MSK;
    saveCnt++;

    cntEx = saveCnt | (1<<HOST_WRITE_CNT_HIGH_BIT);

    Host_SaveDataCnt(lba, cntEx);
    LeaveCriticalSection(&g_LbaWriteCntCriticalSection);

    return saveCnt;
}

void Host_ClearDataCntHighBit(U32 lba,U32 ulWriteCnt)
{
    U32 cntEx;
    U32 realCnt;
    EnterCriticalSection(&g_LbaWriteCntCriticalSection);

    Mid_Read_SATA(lba, (char*)&cntEx, 4);

    realCnt = cntEx & HOST_WRITE_CNT_MSK;
    if(ulWriteCnt != realCnt)
    {
        LeaveCriticalSection(&g_LbaWriteCntCriticalSection);
        return;
    }

    cntEx &= (~(1<<HOST_WRITE_CNT_HIGH_BIT));

    Host_SaveDataCnt(lba, cntEx);
    LeaveCriticalSection(&g_LbaWriteCntCriticalSection);

    return;
}

U32 Host_GetDataCntHighBit(U32 lba)
{
    U32 cntEx;

    Mid_Read_SATA(lba, (char*)&cntEx, 4);

    return (cntEx & (1<<HOST_WRITE_CNT_HIGH_BIT));
}
U32 Host_GetDataCnt(U32 lba)
{
    U32 cnt;

    Mid_Read_SATA(lba, (char*)&cnt, 4);

    return (cnt & HOST_WRITE_CNT_MSK);
}
#endif
void Host_ResetLbaWriteCnt()
{
    U32 lba;

    EnterCriticalSection(&g_LbaWriteCntCriticalSection);

    for(lba = 0; lba < MAX_LBA_IN_SYSTEM; lba++)
    {
        Host_SaveDataCnt(lba,0);
    }
    LeaveCriticalSection(&g_LbaWriteCntCriticalSection);
}
U32 Host_SaveDataCnt(U32 lba, U32 cnt)
{

    Mid_Write_SATA(lba, (char*)&cnt, 4);

    return 1;
}

void Host_OpenDataFile()
{

    // this is now un-useful.
    /*g_hSimHostFileData =  CreateFile((LPSTR)"data.bin",
    GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
    NULL);*/

#ifdef SIM
    Mid_init_SATA();
#else

    // ini sata storage
    {
        int nPu = CE_SUM;
        int nPln = PLN_PER_LUN;
        int nBlk = BLK_PER_PLN;
        int nPge = PG_PER_BLK;
        unsigned long ulPgeSize = LOGIC_PG_SZ;
        unsigned long ulCapacity = ((CE_SUM * PLN_PER_LUN * BLK_PER_PLN * PG_PER_BLK )/G_PAGE_SIZE);
        Mid_Init_SATA((UINT32)ulCapacity);
    }
#endif

}

void Host_CloseDataFile()
{
    //free(g_SC.dwCnt);
    //CloseHandle(g_hSimHostFileData);
    return;
}
BOOL Host_SendOneCmd(HCMD_INFO* pHcmd)
{
    BOOL bSuccess = FALSE;

#ifdef HOST_NVME
    if (pHcmd->CmdType != CMD_TYPE_HSCMD && pHcmd->SecCnt > MAX_CMD_SEC_CNT)
    {
        pHcmd->SecCnt = MAX_CMD_SEC_CNT;
    }
#endif

#ifndef HOST_NVME
    Host_SendRWCmdByType(pHcmd);
#endif


    bSuccess = Host_SendOneCmdInterface(pHcmd);
    if (TRUE == bSuccess)
    {
        g_ulHostSendCmdCnt++;
        if(g_bWearLevelingStatistic)
        {
            if(pHcmd->CmdType == CMD_TYPE_WRITE)
            {
                Dbg_IncHostWriteCnt(pHcmd->SecCnt);
            }
        }
    }
    return bSuccess;
}
void Host_SimClearHCmdQueue()
{

    Host_SimClearHCmdQueueInterface();
}

BOOL Host_SaveDataToFile()
{
    BOOL bRet = TRUE;
    //bRet = WriteFile(g_hSimHostFileData, (LPVOID)g_SC.dwCnt, g_SC.dwLength * sizeof(DWORD), &dwRet, NULL);
    return bRet;
}

BOOL Host_GetDataFromFile()
{
    BOOL bRet = TRUE;
    //bRet = ReadFile(g_hSimHostFileData, (LPVOID)g_SC.dwCnt, g_SC.dwLength * sizeof(DWORD), &dwRet, NULL);
    return bRet;
}


