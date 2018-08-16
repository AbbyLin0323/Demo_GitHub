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
Filename    :HostModel.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/

// include system API
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <stdio.h>

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "verify_host_pattern.h"
#include "sim_nfccmd.h"
#include "AHCI_ControllerInterface.h"
#include "sim_SGE.h"
#include "HostInc.h"
#include "HostModel.h"
#include "model_common.h"
#include "AHCI_HostModel.h"

#define G_PAGE_SIZE ((1024 * 1024 * 1024)/PG_SZ)

extern BOOL Host_IsCMDEmptyInterface();
extern void Host_ModelInitInterface();

BOOL g_bReSetFlag = FALSE;
U32 TestCaseSN = 0;

LOCAL HANDLE g_SimHostEvent;
LOCAL HANDLE g_hSimHostFileData;
LOCAL HANDLE g_hHostThread;

//extern void MV_GenPattern(HOST_CMD *pHCMD,U32 ulSend);
BOOL Host_IsCMDEmpty()
{
    BOOL bEmpty = FALSE;
#ifdef SATA_INT
    bEmpty = Sata_HostIsCMDEmpty();
#endif

#ifdef AHCI_INT
    bEmpty = Host_IsCMDEmptyInterface();
#endif
    return bEmpty;
}

void Host_FunctionMenu()
{
    while(1)
    {
        DBG_Printf("[15]: run mix vector test.\n");
		DBG_Printf("[16]: run mix vector test.\n");
        DBG_Printf("Enter test items: ");

        scanf("%d", &g_SimHostFunction);

        if (16 < g_SimHostFunction)
            printf("please select right function");
        else
            break;
    }

}

DWORD WINAPI Host_ModelThread(LPVOID p)
{
    while(1)
    {
        WaitForSingleObject(g_SimHostEvent,INFINITE);
        Host_ModelProcess();
    }
    
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
    Host_OpenDataFile();

#ifdef AHCI_INT
    Host_ModelInitInterface();
#endif

#ifdef SIM
#ifndef NO_THREAD
    g_SimHostEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hHostThread = CreateThread(0, 0, Host_ModelThread, 0, 0, 0);
#endif
#endif
}

void Host_ModelSchedule()
{
#ifdef NO_THREAD
    Host_ModelProcess();
#else
    SetEvent(g_SimHostEvent);
#endif
}

BOOL Host_ModelProcess()
{
    static BOOL bEnd = TRUE;
    if (bEnd)
    {
        Host_FunctionMenu();
        bEnd = FALSE;
    }

    if (g_SimHostFunction == 15)   
    {
        sim_mix_vector();
        //return FALSE;
    }
#ifndef SIM_XTENSA
	else if (g_SimHostFunction == 16)   
    {
        mixv_host_main();
        //return FALSE;
    }
#endif
    else
    {
        DBG_Break();//for mix vector test, we can not get here
    }

    return TRUE;
}


U32 Host_UpdateDataCnt(U32 lba)
{
    U32 cntEx;

    Mid_Read_SATA(lba, (char*)&cntEx, 4);
    cntEx++;


    //cnt = cntEx;

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

U32 Host_SaveDataCnt(U32 lba, U32 cnt)
{

    Mid_Write_SATA(lba, (char*)&cnt, 4);

    return 1;
}


void Host_OpenDataFile()
{
    g_hSimHostFileData =  CreateFile("data.bin",
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if(g_hSimHostFileData == INVALID_HANDLE_VALUE)
    {
        HOST_LogInfo(LOG_PRINT, 0, "%s file create error %d\r\n", "log.txt", GetLastError());
        DBG_Getch();
    }    

#ifdef SIM 
    Mid_init_SATA();  
#else

    // ini sata storage
    {
        int nPu = CE_NUM;
        int nPln = PLN_PER_PU;
        int nBlk = BLK_PER_PLN;
        int nPge = PG_PER_BLK;
        unsigned long ulPgeSize = PG_SZ;
        unsigned long ulCapacity = ((CE_NUM * PLN_PER_PU * BLK_PER_PLN * PG_PER_BLK )/G_PAGE_SIZE);
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
    // by Charles Zhou
    return FALSE;
}

void Host_SimClearHCmdQueue()
{
#ifdef SATA_INT
    Sata_HostSimClearHCmdQueue();
#endif

#ifdef AHCI_INT
#endif
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

void Host_NoneDataCMDFinish()
{
#ifdef SATA_INT
    Sata_HostNoneDataCMDFinish();
#endif
#ifdef AHCI_INT
#endif
}
