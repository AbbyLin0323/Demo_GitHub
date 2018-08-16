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
#include "system_statistic.h"
#include "HAL_Xtensa.h"
#include "HAL_GLBReg.h"
#include "sim_NormalDSG.h"
#include "Sim_XOR_Interface.h"
#include "HostModel.h"
#include "xt_library.h"
#include "model_common.h"
#include "memory_access.h"
#include "model_config.h"
#include "action_define.h"
#include "win_bootloader.h"
#include "COM_MainSche.h"
#include "256fm_x64/flash_meminterface.h"
#ifdef HOST_NVME
#include "nvmeStd.h"
#include "NVME_ControllerModel.h"
#endif


extern BOOL g_bReSetFlag;
extern WIN_SIM_MGR g_WinSimMgr;

typedef void (*PSIMEventHandler)();
PSIMEventHandler SimEventHandlerArray[HOST_STATUS_ALL];


extern void Comm_NFCModelInit(void);
extern void DMAE_ModelInit(void);
extern void SE_ModelInit(void);

extern BOOL NFC_IsExit();
extern void NFC_ModelStartUp(void);
extern void Mid_SearchLba(U32 ulLba);

extern void CommDbgEventHandler(void);
extern void NFC_ModelSchedule();

#ifdef HOST_SATA
extern void Comm_SataDsgModelInit(void);
extern BOOL SDC_ModeleThreadExit();
extern void SDC_ModelPowerUp();
extern void SDC_ModelInit();
extern void SDC_ModelSchedule();
extern void SDC_SendCOMReset(void);
extern BOOL SDC_IsDeviceReady(void);
#endif

#ifdef HOST_AHCI
extern void AHCI_HostPortInit();

#endif


#if (TRUE == HCT_ENABLE)
extern void HCT_ModelInit();
extern void SIM_HCTRegInit();
extern void HCT_CSTInit();
extern void SIM_HostCWBQThreadExit();
extern void SIM_HostCFCQThreadExit();
extern void SIM_HostCModelPowerUp();
#endif

#if (TRUE == SGE_ENABLE)
extern void SGE_ParamInit();
extern void SGE_ModelThreadExit();
extern void SGE_ModelInit();
extern void SGE_ModelSchedule();
extern BOOL SGE_IsExit();
#endif

#ifdef AF_ENABLE_M
extern HANDLE g_hContrlAFThread;
extern BOOL SIM_HostCAFThreadExit(void);
#endif 

extern CRITICAL_SECTION g_ThreadIDCriticalSection[MCU_MAX];
extern void Mid_PrintfFlashLog();

void SIM_ModelInit(void)
{
    Host_ModelInit();

    FlashSpecInterfaceFactory();
    Comm_NFCModelInit();

    DMAE_ModelInit();
    SE_ModelInit();

    //Since AF is a part of NVMeController,But the AF_EnableReg is located in HCT, 
    //Init HCT Before Conrtoller_Init
#if (TRUE == HCT_ENABLE)
    HCT_ModelInit();
#endif
#if defined (HOST_SATA)
    SDC_ModelInit();
#endif
    Comm_IMemObjectInit();
    // Initialize XOR model.
    XORM_IInit();

    return;
}

void SIM_ModelResetReg()
{

#if defined HOST_SATA
    Comm_NormalDsgModelInit();
    Comm_SataDsgModelInit();
#elif defined HOST_AHCI
    AHCI_HostPortInit();
#elif defined HOST_NVME
    NVME_ControllerModelInit();
    NVMeDriverInit();
#endif

    NFC_ModelParamInit();

#if (TRUE == HCT_ENABLE)
    SIM_HCTRegInit();
    HCT_CSTInit();
#endif
}

void SIM_ModelThreadExit()
{
#if defined HOST_SATA
    SDC_ModeleThreadExit();
#elif defined HOST_NVME
    MSI_ThreadExit();
#endif

#if (TRUE == SGE_ENABLE)
    SGE_ModelThreadExit();
#endif

#if (TRUE == HCT_ENABLE)
    SIM_HostCWBQThreadExit();
    SIM_HostCFCQThreadExit();
#ifdef AF_ENABLE_M
    SIM_HostCAFThreadExit();
#endif //AF_ENABLE_M
#endif

}

void SIM_ModelThreadStart()
{
#if defined HOST_SATA
    SDC_ModelPowerUp();
#elif defined HOST_NVME
    MSI_ThreadInit();
#endif

#if (TRUE == SGE_ENABLE)
    SGE_ModelInit();
#endif

#if (TRUE == HCT_ENABLE)
    SIM_HostCModelPowerUp();
#endif

}

void SIM_DevPowerUP()
{
    Host_SimClearHCmdQueue();
    SIM_ModelResetReg();
    SIM_SystemRegInit();
    SIM_WinBootLoader(g_WinSimMgr.bNeedLLF);

    SIM_ModelThreadStart();
    SIM_StartMcuThread();

    /* wait MCU0 init ok */
    while(rGlbMcuSgeRst == 0x00FFFFFF)
    {
        Sleep(10);
    }

    g_bReSetFlag = FALSE;

    SIM_DevSetStatus(HOST_STATUS_HANDSHAKING);
}

void SIM_DevPowerDown()
{

    U32 ulHaveExit = 0;
    DWORD ulExitCode = 0;
    BOOL bExitModel = FALSE;

    g_bReSetFlag = TRUE;

    while (FALSE == NFC_IsExit())
    {
        NFC_ModelSchedule();
    }

    //check mcu thread
    while (FALSE == SIM_MCUIsAllExit())
    {
        SIM_WaitMCUExit();

        SIM_MCUSchedule(MCU0_ID);
        SIM_MCUSchedule(MCU1_ID);
        SIM_MCUSchedule(MCU2_ID);
    }

    // add SDC mode thread exit
    //clear dram before power up
    SIM_ModelThreadExit();
    Sim_ClearDram();

    SIM_DevSetStatus(HOST_STATUS_DO_POWERUP);
    g_WinSimMgr.bNeedLLF = FALSE;

    return;
}

void SIM_DevHandshake(void)
{
#ifdef HOST_NVME
    U32 i;

    NVMe_SetCcEn(TRUE);
    while (FALSE == NVMe_IsControllerReady())
    {
        SIM_MCUSchedule(MCU0_ID);
        SIM_MCUSchedule(MCU1_ID);
        SIM_MCUSchedule(MCU2_ID);
    }

    for(i = 0; i < IO_QCNT; i++)
    {
        NVMeCreateCQ(i);
        NVMeCreateSQ(i);
    }

    while(FALSE == NVMe_IsAdminCmdFinish())
    {
        NVME_ControllerModelSchedule();
        SIM_MCUSchedule(MCU0_ID);
        SIM_MCUSchedule(MCU1_ID);
        SIM_MCUSchedule(MCU2_ID);
    }
    
    SIM_DevSetStatus(HOST_STATUS_RUNNING);

#else
    SDC_SendCOMReset();

    while (FALSE == SDC_IsDeviceReady())
    {
        SIM_MCUSchedule(MCU0_ID);
        SIM_MCUSchedule(MCU1_ID);
        SIM_MCUSchedule(MCU2_ID);
    }

    SIM_DevSetStatus(HOST_STATUS_RUNNING);
#endif
}

void SIM_DevNormalRun(void)
{
    CommDbgEventHandler();
    Host_ModelSchedule();

    //FW event
    SIM_MCUSchedule(MCU0_ID);

    #if defined (HOST_SATA)
    SDC_ModelSchedule();
    #elif defined (HOST_NVME)
    NVME_ControllerModelSchedule();
    #endif

    SIM_MCUSchedule(MCU1_ID);
    
    SIM_MCUSchedule(MCU2_ID);

    NFC_ModelSchedule();

    #if (TRUE == SGE_ENABLE)
    SGE_ModelSchedule();
    #endif

    return;
}

void SIM_DevRedoLLF()
{
    U32 ulHaveExit = 0;
    DWORD ulExitCode = 0;
    BOOL bExitModel = FALSE;

    while (FALSE == SIM_MCUIsAllExit())
    {
       SIM_WaitMCUExit();
       SIM_DevNormalRun();
    }

    g_bReSetFlag = TRUE;

    SIM_ModelThreadExit();
    Host_ResetLbaWriteCnt();

    //clear dram before power up when LLF
    Sim_ClearDram();

    SIM_DevSetStatus(HOST_STATUS_DO_POWERUP);

    g_WinSimMgr.bNeedLLF = TRUE;

    return;
}

void SIM_CalculateSubLba(U32 ulSearchedLba)
{
    U32 ulSubLba = 0;
    U8 uSubSystem = 2;
    U8 ulInSubSystem = 0;
    U32 ulLine = 0;
    U32 ulSubLpn = 0;
    uSubSystem = 1;

    ulInSubSystem = ((g_WinSimMgr.ulSearchedLBA/SEC_PER_BUF) % uSubSystem);
    ulLine = (g_WinSimMgr.ulSearchedLBA / (SEC_PER_BUF*uSubSystem));
    ulSubLba = (ulLine * SEC_PER_BUF)  + (g_WinSimMgr.ulSearchedLBA % SEC_PER_BUF);
    ulSubLpn = ulSubLba / SEC_PER_LPN;
    DBG_Printf("bSearchLBA = 0x%x, ulSubLba = 0x%x, ulSubLpn = 0x%x, ulInSubSytemt = %d \n", g_WinSimMgr.ulSearchedLBA, ulSubLba, ulSubLpn, ulInSubSystem);
}

int main(void)
{
	U8 ucMcuIndex;
    SIM_EventInit();

    /*Initilize ThreadIDCriticalSection*/
	for (ucMcuIndex = 0; ucMcuIndex < MCU_MAX; ucMcuIndex++)
	{ 
		InitializeCriticalSection(&g_ThreadIDCriticalSection[ucMcuIndex]);
	}

    while(TRUE)
    {
        if (TRUE == g_WinSimMgr.bSearchLBA)
        {
            Mid_SearchLba(g_WinSimMgr.ulSearchedLBA);
            g_WinSimMgr.bSearchLBA = FALSE;
        }

        if (TRUE == g_WinSimMgr.bCacSubLBA)
        {
            SIM_CalculateSubLba(g_WinSimMgr.ulSearchedLBA);
            g_WinSimMgr.bCacSubLBA = FALSE;
        }

        if (TRUE == g_WinSimMgr.bPrintFlashLog)
        {
            Mid_PrintfFlashLog();
            g_WinSimMgr.bPrintFlashLog = FALSE;
        }

        SIM_EventHandler();
    }

   return 0;
}

