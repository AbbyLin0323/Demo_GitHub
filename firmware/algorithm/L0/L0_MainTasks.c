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


#include "BaseDef.h"
#include "COM_Memory.h"
#include "COM_BitMask.h"
#include "HAL_Inc.h"
#include "HAL_ParamTable.h"
#include "HAL_TraceLog.h"
#include "L0_Config.h"
#include "L0_Interface.h"
#include "L0_Event.h"
#include "L0_TaskManager.h"
#include "HAL_GPIO.h"
#include "Disk_Config.h"
#include "FW_Event.h"

#if defined(HOST_AHCI)
#include "L0_Ahci.h"
#include "L0_AhciHCT.h"
#include "L0_AhciATACmdProc.h"
#include "L0_ATAGenericCmdLib.h"
#elif defined(HOST_NVME)
#include "HAL_HCmdTimer.h"
#include "HAL_NVME.h"
#ifdef AF_ENABLE
#include "HAL_NVMECFGEX.h"
#endif 
#include "L0_NVMe.h"
#include "L0_NVMeErrHandle.h"
#include "L0_NVMeDataIO.h"
#include "L0_NVMeHCT.h"
#elif defined(HOST_SATA)
#include "L0_Init.h"
#include "L0_Schedule.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"
#include "L0_SataErrorHandling.h"
#endif

#ifndef SIM
#include "HAL_PM.h"
#include "L0_Uart.h"
#include "HAL_SpiDriver.h"
#include "uart.h"
#else
#include "Windows.h"
#include "model_config.h"
#include "win_bootloader.h"
#endif

//specify file name for Trace Log
#define TL_FILE_NUM     L0_MainTasks_c

/* Global variables references */
extern PSCQ g_apSCmdQueue[SUBSYSTEM_NUM_MAX];
extern PSMSG g_apSMsgQueue[SUBSYSTEM_NUM_MAX];
extern PSMSG g_apNtfnMsgBox[SUBSYSTEM_NUM_MAX];
extern U32 g_ulNtfnMsgSeq;
extern GLOBAL U32 g_ulVarTableAddr;
extern BOOL g_bFwUpdateOngoing;
extern GLOBAL U32 g_ulVUARTRawDataReadyFlag;
#ifdef HOST_NVME
extern BOOL bLEDSetActive;
#endif

extern void L0_RegScanHelper(L0_TASK_MANAGER *pL0TaskMgr);
extern BOOL L0_AhciDebugModeInit(void *pParam);

#ifdef HOST_NVME
extern U32 *g_pL0DataBuffBusy;
#ifdef AF_ENABLE
extern volatile HCT_CONTROL_REG *g_pHCTControlReg;
extern volatile NVME_CFG_EX     *g_pNVMeCfgExReg;
#endif
#endif

#if ((!defined SIM) && (defined HOST_NVME))
extern GLOBAL BOOL bDPLLGating;
#endif

/* Global variables definition */
/* The number of subsystem implemented in current system configuration. */
GLOBAL U32 g_ulSubsysNum, g_ulSubsysNumBits;

/* The pointer to the boot parameter table provided by the bootloader. */
GLOBAL PTABLE *g_pBootParamTable;
GLOBAL FW_VERSION_INFO *g_pFwVersionInfo;

/* The pointer to global trace log / raw data / host info / device param buffer. */
GLOBAL U32 g_ulTraceLogBuffStart;
GLOBAL U32 g_ulATARawBuffStart;
GLOBAL U32 g_ulHostInfoAddr, g_ulDevParamAddr;

/* The pointer to firmware update for downloading firmware image */
GLOBAL U32 g_ulFWUpdateImageAddr;
GLOBAL FW_UPDATE g_tFwUpdate;

/* The idle/boot/shutdown/flush cache/raw data transfer status for all subsystems */
GLOBAL U32 g_ulL0IdleTaskFinished;
GLOBAL U32 g_ulL0IdleTaskAborted;
GLOBAL U32 g_ulSubSysBootOk;
GLOBAL U32 g_ulShutdownReady;
GLOBAL U32 g_ulRawDataReadyFlag;
GLOBAL U32 g_ulFlushCacheReady;

/* The force SCQ clear finishing status for all subsystems */
GLOBAL U32 g_ulSubsysSCQClrMap;

/* Debug mode stage, see DEBUG_MODE_STATE */
GLOBAL U32 g_ulDebugModeState;
GLOBAL U32 g_ulDebugModeFinishMap;

/* L0 scheduling flag for UART MP mode */
GLOBAL U32 g_ulMPModeBoot;
GLOBAL U32 g_ulUART_MP_Mode;
GLOBAL BOOL g_ulUARTCmdPending;


/* For SATA BIST mode */
GLOBAL volatile U32 g_ulSataBistMode;

GLOBAL L0_TASK_MANAGER g_tL0TaskMgr;
GLOBAL VIA_CMD_STATUS g_tL0ViaCmdSts;

/* Local variables definition */
LOCAL U32 l_ulSubsysHostInfoAddr[SUBSYSTEM_NUM_MAX],
          l_ulSubsysDevParamAddr[SUBSYSTEM_NUM_MAX];

LOCAL U32 l_ulSaveHostInfoPending;
LOCAL U32 l_ulSysLevPMEnable;
LOCAL U32 l_ulIdleTaskTimeOutFlag;

#ifdef SIM
LOCAL U32 l_ulLastTickStamp;
#endif

/* Local routines declaration */
void L0_Init(void);
void L0_Schedule(void);
U32 L0_RAMAlloc(U32 ulRAMBaseAddr, U32 ulAllocLen, U32 *pStartPtr, U32 ulAlignToBits);
U32 L0_MemMapInit(void);
void L0_RegAllEventHandler(void);
void L0_InitHostInfoPage(void);
void L0_SubSystemInit(U32 ulHotStartFlag);
void L0_ProcSubsysMsg(void);
BOOL L0_TaskSwitchToDebugMode(void *pParam);
BOOL L0_TaskWaitBoot( void *);
BOOL L0_TaskXferReset(void *pParam);
BOOL L0_TaskShutDown(void *pParam);
void L0_TaskManagerInit(U32);
BOOL RESTRICTION_RODATA_IN_DSRAM L0_TaskManagement(void);
#ifdef SIM
void L0_SimSetLastTickStamp(void);
#endif
void L0_TimeKeepingInit(void);
void L0_TimeKeepingTask(void);
U32 L0_CheckNeedToExecTimeTask(void);
void L0_UpdatePowerOnTime(void);
U32 L0_CheckNeedToSaveHostInfo(void);
U32 L0_ExecSaveHostInfo(const U32);
void L0_PrepNextTimeKeeping(void);
void L0_PauseTimeKeepingTask(void);

MCU0_DRAM_TEXT void L0_BootLoaderHWInit(void)
{
    U8 ucFuncType;
    PTABLE *pPTable = HAL_GetPTableAddr();

    /* initialize DRAM total size parameter */
    g_ulDramTotalSize = pPTable->tHALFeature.aFeatureDW[0];
  
    /* According to Bootloader P-Table and F-Table to INIT Hardware  */
    if (FALSE == HAL_IsBootLoaderFileExist())
    {
        /* check P-Table and F-Table exist fail, do error handling */
        DBG_Printf("L0_BootLoaderHWInit bootloader not exist ERROR!\n");
        DBG_Getch();
    }

    /* check is current MCU Enable */
    if (FALSE == HAL_IsMCUEnable(HAL_GetMcuId()))
    {
        DBG_Printf("MCU %d Disabled: enter forever loop!\n", HAL_GetMcuId());
        while(TRUE);
    }

    /* check the validation of HW init functions in F-Table, 
        if one or more of them are invalid, FW take the responsibility to register it here */
    for (ucFuncType = 0; ucFuncType < INIT_FUNC_CNT; ucFuncType++)
    {
        if (FALSE == HAL_IsFTableValid(ucFuncType))
        {
            /* This is only an example, the ulFuncAddr that FW provided cannot be NULL */
            HAL_RegisterFTableFunc(ucFuncType, NULL);
        }
    }

    /* Now we check F-Table again and all HW init functions should be valid, 
       whether it is provided by BootLoader or FW */
    if (FALSE == HAL_IsFTableAllValid())
    {
        DBG_Printf("L0_BootLoaderHWInit F-Table invalid ERROR!\n");
        DBG_Getch();
    }

    /* init HW by according to P-Table and F-Table */
    for (ucFuncType = 0; ucFuncType < INIT_FUNC_CNT; ucFuncType++)
    {
        if (FALSE == HAL_IsPTableDone(ucFuncType))
        {
            /* call HW init function here if it hasn't init by bootloader */
            HAL_InvokeFTableFunc(ucFuncType);
        }
    }

    /* double check, HW init functions in P-Table and F-Table should be all done */
    if (FALSE == HAL_IsPTableAllDone())
    {
        DBG_Printf("L0_BootLoaderHWInit HW INIT FAIL!\n");
        DBG_Getch();
    }

    /* clear related HW init bits in P-Table for FW warm init */
    HAL_ClearPTableDoneBit(INIT_PCIE);
    HAL_ClearPTableDoneBit(INIT_CLK_GATING);
    HAL_ClearPTableDoneBit(INIT_NFC);

    return;
}

MCU0_DRAM_TEXT void L0_FwUpdateInit(void)
{
    g_tFwUpdate.ulFwBaseAddr = g_ulFWUpdateImageAddr;
    g_tFwUpdate.ulFwSize     = FW_IMAGE_MEM_SIZE;
    g_tFwUpdate.ulDldSize    = 0;
    //g_tFwUpdate.ulCRC        = 0;
#ifdef HOST_SATA
    g_tFwUpdate.ulPreDataSecCnt = 0;
    g_tFwUpdate.ulPreOffset = 0;
    g_bFwUpdateOngoing = FALSE;
#endif
}

MCU0_DRAM_TEXT void L0_Init(void)
{
    U32 ulHotStartFlag;
    U32 ulDramAllocAddr;
    BOOTLOADER_FILE *pBootLoaderFile;
#if defined (SIM) && defined(HOST_SATA)
#else
    U32 i;
#endif


    /* Initializes boot loader parameter table pointer. */
    pBootLoaderFile = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
    g_pBootParamTable = &(pBootLoaderFile->tSysParameterTable);
    g_pFwVersionInfo = (FW_VERSION_INFO *)(DRAM_FW_STATIC_INFO_BASE);  /* fw-ver in offset 0 */ //DRAM_PARAM_TABLE_BASE

    /* first of all, check for warm boot */
#ifndef SIM
    if (SYSPMSTATE_WARM_START == HAL_PMCheckBootType())
    {
        ulHotStartFlag = TRUE;
        g_pBootParamTable->sBootStaticFlag.bsWarmBoot = TRUE;
    }

    else
    {
        ulHotStartFlag = FALSE;
        g_pBootParamTable->sBootStaticFlag.bsWarmBoot = FALSE;
    }
#endif

    /* init HW */
    L0_BootLoaderHWInit();

    /* enable I-Cache */
    HAL_EnableICache(HAL_GetMcuId());

#ifdef DCACHE
    HAL_EnableDramAddrHigh();
#endif

#ifndef SIM
    if ((BOOT_METHOD_MPT == g_pBootParamTable->sBootStaticFlag.bsBootMethodSel)
        && (TRUE == g_pBootParamTable->sBootStaticFlag.bsUartMpMode))
    {
        g_ulUART_MP_Mode = TRUE;
        L0_UartMpInit();
    }
    else
    {
        g_ulUART_MP_Mode = FALSE;
        L0_UartMpInit();
    }

    //Sets the common miscellaneous information area pointer
    g_pMCU12MiscInfo = (PMCU12_COMMON_MISC_INFO)DSRAM1_MCU12_CMNMISC_BASE;

    if(TRUE == g_pBootParamTable->sBootStaticFlag.bsExternalSpiFlashEn)
    {
        HAL_SpiInit();
        DBG_Printf("SPI Init done.\n");
    }

    if (FALSE == ulHotStartFlag)
    {
        HAL_PMInit();
    }

    else
    {
        for (i = 0; i < PMU_TIMER_NUMBER; i++)
        {
            HAL_PMClearTimeOutFlag(i);
        }
    }

#else
    ulHotStartFlag = FALSE;
    g_ulUART_MP_Mode = FALSE;

    //Sets the common miscellaneous information area pointer
    g_pMCU12MiscInfo = (PMCU12_COMMON_MISC_INFO)DSRAM1_MCU12_CMNMISC_BASE;
#endif

    /* Initializations for cold boot. */
    if (FALSE == ulHotStartFlag)
    {
        L0_EventInit();
        
        /* 1) Allocating buffer spaces in DDR SDRAM and DSRAM. */
        ulDramAllocAddr = L0_MemMapInit();

#if defined(HOST_AHCI)
        /* 2) Initializing AHCI system. */
        ulDramAllocAddr = L0_AhciMemAlloc(ulDramAllocAddr);
        L0_EventSet(L0_EVENT_TYPE_XFER_RESET, NULL);
#elif defined(HOST_NVME)
        ulDramAllocAddr = L0_NVMeMemAlloc(ulDramAllocAddr);
#elif defined(HOST_SATA)
        ulDramAllocAddr = L0_SataMemAlloc(ulDramAllocAddr);
        L0_SataInit();
#endif
        
        if (ulDramAllocAddr > DRAM_DATA_BUFF_MCU12_BASE)
        {
            DBG_Printf("MCU0 DRAM allocate %d KB,overflow %d KB ! \n", (ulDramAllocAddr - DRAM_DATA_BUFF_MCU0_BASE) / 1024, (ulDramAllocAddr - DRAM_DATA_BUFF_MCU12_BASE) / 1024);
            DBG_Getch();
        }
        DBG_Printf("MCU0 DRAM allocate %d KB\n", (ulDramAllocAddr - DRAM_DATA_BUFF_MCU0_BASE) / 1024);

#ifdef HOST_NVME
        /* 3) Initializing SGE for local data transfer. */
        L0_HostDataXferInit();

        /* 4) Initializing NVMe protocol related hardware/software affairs. */
        /*get UARTMP mode, if high, uart mp mode*/
#ifndef SIM 		
        if (HAL_UartIsMp() == FALSE) 
        {       
            L0_NVMePhySetting();
        }
#else
        L0_NVMePhySetting();
#endif
        
        L0_NVMeBarSpaceInit();
        L0_NVMeErrorHandleInit();

        //Init MsgQueue
        L0_InitMsgQueue();

        //Init RspMarker
        for (i = 0; i < SUBSYSTEM_NUM_MAX; i++)
        {
            g_apMcShareData[i]->tRspMarker.SmsgUeccRspMarker.ulMarkerType = 0xFFFFFFFF;
            g_apMcShareData[i]->tRspMarker.SmsgUeccRspMarker.ulPUID = 0;
        }
        
        HAL_HCmdTimerInit(L0_NVMeHCmdTOCallback, HCMD_TIMEOUT_US_COUNT);
#endif

        /* 4) Initializing trace log system. */
#ifdef HOST_CMD_REC
        TL_Initialize(MCU0_ID, (void *)(DRAM_START_ADDRESS + g_ulDramTotalSize), L0_MORE_TL_MEM_SIZE, SEC_SIZE);
#else
        TL_Initialize(MCU0_ID, (void *)g_ulTraceLogBuffStart, L0_TL_MEM_SIZE, SEC_SIZE);
#endif

        L0_RegAllEventHandler();
    }

#ifndef SIM
    else
    {
#ifdef HOST_SATA
        PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)L0_ATAGetIdentifyPage();
        HAL_SataInitialize(BUF_SIZE_BITS);
        if(TRUE == pIdentifyData->SataFeatureEnable.DipmEn )
        {
            rSDC_ControlRegister = FW_DMAEXE_STE_EN | DIPM_EN | HW_SLUMBER_EN /*| HW_PARTIAL_EN */| HW_PWR_EN;   //IPM-08 enable DIPM
        }     
#else        
        L0_NVMePrepareL12Exit();
        #ifdef VT3533_A2ECO_DSGRLSBUG
        HAL_NormalDsgFifoInit();
        #endif
#endif
    }
#endif

    /* Initializations for both cold/hot start. */
#if defined(HOST_AHCI)
    L0_AhciInitHCT();
#elif defined(HOST_NVME)

    L0_NVMEInitHCT();

#ifdef AF_ENABLE
    L0_NVMeCfgExInit(); //zhfAF: Move L0_NVMeCfgExInit behind L0_NVMeBarSpaceInit and L0_NVMEInitHCT
#endif

#endif
    L0_TaskManagerInit(ulHotStartFlag);
    L0_SubSystemInit(ulHotStartFlag);

    if (FALSE == g_ulUART_MP_Mode)
    {
        L0_TimeKeepingInit();
    }

    if (TRUE == ulHotStartFlag)
    {
#ifdef HOST_SATA
#ifndef SIM
        HAL_InitInterrupt(TOP_INTSRC_SDC, BIT_ORINT_SDC);
#endif
#endif
    }

#ifdef DCACHE
    /* enable D-Cache */
    HAL_EnableDCache(HAL_GetMcuId());
    if (TRUE == HAL_GetTraceLogEnFlag())
        dbg_tracelog_init();
#endif

#ifdef HOST_NVME
    /* disable LED function clock and disable LED GPIO */
    (*((volatile U32*) (REG_BASE_GLB + 0x04))) |= (0 << 13);
    rGPIOLED = 0x2a2a;
#endif

#ifndef SIM
#ifdef HOST_NVME
    HAL_PcieMpInit();
#endif
    //if (DATA_TRANSFER_NONE == g_ucDATATransferMode)
    {
        DBG_Printf("Enable UART ISR\n");
        HAL_InitInterrupt(TOP_INTSRC_UART, BIT_ORINT_UART);
        L0_UartReadMask(FALSE);
    }
#endif

    DBG_Printf("L0_Init done\n");
    return;
}

MCU0_DRAM_TEXT void L0_TaskUartMp(void)
{
#ifndef SIM
    L0_UartDBG();
#endif
}


void L0_Schedule(void)
{
    while (NULL != L0_RecyleSCmd());

    L0_ProcSubsysMsg();

#if defined(HOST_NVME)
    L0_ProcessMsgQueue();
#endif

    if (FALSE == g_ulUART_MP_Mode)
    {
        L0_TimeKeepingTask();
    }

    if (FALSE != L0_EventProcess())
    {
        #ifndef SIM
        
#ifdef HOST_NVME
        HAL_PcieMpProc();
#endif
        #endif
        if (FALSE != g_ulUARTCmdPending)
        {
            L0_TaskUartMp();
        }

        L0_RegScanHelper(&g_tL0TaskMgr);
        if (TRUE == L0_TaskManagement())
        {
#if defined(HOST_AHCI)
            L0_AhciTask();
#elif defined(HOST_NVME)
            L0_NVMeSchedule();
#elif defined(HOST_SATA)
            L0_SataTask();
#endif
        }
    }

    while (g_ulSataBistMode == 1)
    {
        ;
    }

    return;
}
/****************************************************************************
Name        :L0_RAMAlloc
Input       : ulRAMBaseAddr - The base address for allocation;
                   ulAllocLen - The requested space for allocation;
                   pStartPtr - The label pointer to the space being allocated;
                   ulAlignToBits - The alignment bits if an aligned space is requested.
Output      : The current memory address after space allocated.
Author      :
Date        :
Description : This routine attempts to allocate a space in requested memory address.
                        The starting address of allocated space would be saved in given pointer.
Others      :
Modify      :
****************************************************************************/
U32 L0_RAMAlloc(U32 ulRAMBaseAddr, U32 ulAllocLen, U32 *pStartPtr, U32 ulAlignToBits)
{
    U32 ulAlignBorder, ulAlignMask;

    ulAlignBorder = (1 << ulAlignToBits);
    ulAlignMask = ulAlignBorder - 1;

    if (0 != (ulRAMBaseAddr & ulAlignMask))
    {
        /* If given base address is not aligned to requested alignment,
            an aligned address must be calculated. */
        ulRAMBaseAddr = (ulRAMBaseAddr & (~ulAlignMask)) + ulAlignBorder;
    }

    *pStartPtr = ulRAMBaseAddr;

    ulRAMBaseAddr += ulAllocLen;

    return ulRAMBaseAddr;
}

/****************************************************************************
Name        :L0_MemMapInit
Input       : None.
Output      : None.
Author      :
Date        :
Description : This routine attempts to allocate all required memory space used for
                    L0 except global symbols.
Others      :
Modify      :
****************************************************************************/
MCU0_DRAM_TEXT U32 L0_MemMapInit(void)
{
    U32 ulCurrAllocPos;

    /* shared SRAM allocation */
    /* 1. Allocating SRAM space for SCMD queue related data structures. */
    /* 2. Allocating SRAM space for inter-processor interrupt based subsystem message. */
    ulCurrAllocPos = L0_RAMAlloc(DSRAM1_MCU01_SHARE_BASE,
        sizeof(SCQ),
        (U32 *)(&g_apSCmdQueue[0]),
        0);

    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        sizeof(SMQ),
        (U32 *)(&g_apSMsgQueue[0]),
        0);

    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        sizeof(SMSG),
        (U32 *)(&g_apNtfnMsgBox[0]),
        0);

    (void)L0_RAMAlloc(ulCurrAllocPos,
        sizeof(MCSD),
        (U32 *)&g_apMcShareData[0],
        0);


    #if 0
    ulCurrAllocPos = L0_RAMAlloc(DSRAM1_MCU02_SHARE_BASE,
        sizeof(SCQ),
        (U32 *)(&g_apSCmdQueue[1]),
        0);

    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        sizeof(SMQ),
        (U32 *)(&g_apSMsgQueue[1]),
        0);

    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        sizeof(SMSG),
        (U32 *)(&g_apNtfnMsgBox[1]),
        0);

    (void)L0_RAMAlloc(ulCurrAllocPos,
        sizeof(MCSD),
        (U32 *)&g_apMcShareData[1],
        0);
    #endif
    
    /* DRAM allocation */
    /* 1. The buffer space for trace log recorded by MCU0. */
    ulCurrAllocPos = L0_RAMAlloc(DRAM_DATA_BUFF_MCU0_BASE, 
        L0_TL_MEM_SIZE,
        (U32 *)(&g_ulTraceLogBuffStart),
        BUF_SIZE_BITS);

    /* 2. The buffer space for temporary data transfer in some non-media-access commands, e.g. TRIM.
        One whole buffer block in DDR SDRAM space. */
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        BUF_SIZE,
        (U32 *)(&g_ulATARawBuffStart),
        BUF_SIZE_BITS);

    /* 3. The buffer space allocated for Var table, storage var table for via vendor define command in host tools */
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        BUF_SIZE,
        (U32 *)(&g_ulVarTableAddr),
        BUF_SIZE_BITS);
    
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos, 
        FW_IMAGE_MEM_SIZE,
        &g_ulFWUpdateImageAddr,
        BUF_SIZE_BITS);

    /* 4. The buffer for host information page and device parameter page (replacement for the old global
        information). The exact size of the data structure in DDR SDRAM space. */
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos, 
        sizeof(HOST_INFO_PAGE),
        &g_ulHostInfoAddr,
        BUF_SIZE_BITS);

#ifdef DCACHE
    /* Reallocates host information page to cachable address range to improve L0 performance. */
    g_ulHostInfoAddr += DRAM_HIGH_ADDR_OFFSET;
#endif

    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos, 
        sizeof(DEVICE_PARAM_PAGE),
        &g_ulDevParamAddr,
        0);

#ifdef HOST_NVME
    /* 5. The cache status for L0 local data transfer. */
#ifdef L0_DSG_CACHESTS_IN_OTFB
    g_pL0DataBuffBusy = (U32*)OTFB_FW_DATA_MCU0_BASE;
#else
    ulCurrAllocPos = L0_RAMAlloc(ulCurrAllocPos,
        sizeof(U32),
        (U32 *)(&g_pL0DataBuffBusy),
        0);
#endif
    //DBG_Printf("g_pL0DataBuffBusy = 0x%x\n", (U32)g_pL0DataBuffBusy);
#endif

    return ulCurrAllocPos;
}

void L0_RegAllEventHandler(void)
{
#if defined(HOST_AHCI)
    L0_EventRegHander(L0_EVENT_TYPE_SUBSYS_BOOT, L0_TaskWaitBoot, L0_AhciInitAtaData);
    L0_EventRegHander(L0_EVENT_TYPE_XFER_RESET, L0_TaskXferReset, L0_AhciHBAPortInit);
    L0_EventRegHander(L0_EVENT_TYPE_SHUTDOWN, L0_TaskShutDown, NULL);
    L0_EventRegHander(L0_EVENT_TYPE_DEBUG_MODE_EN, L0_TaskSwitchToDebugMode, L0_AhciDebugModeInit);
#elif defined(HOST_NVME)
    L0_NVMeEventRegister();
    L0_EventRegHander(L0_EVENT_TYPE_DEBUG_MODE_EN, L0_TaskSwitchToDebugMode, NULL);
#elif defined(HOST_SATA)
    L0_EventRegHander(L0_EVENT_TYPE_SUBSYS_BOOT, L0_TaskWaitBoot, L0_SataWaitBootInit);
    L0_EventRegHander(L0_EVENT_TYPE_SHUTDOWN, L0_TaskShutDown, NULL);
    L0_EventRegHander(L0_EVENT_TYPE_DEBUG_MODE_EN, L0_TaskSwitchToDebugMode, L0_SataDebugModeInit);
    L0_EventRegHander(L0_EVENT_TYPE_ERR_HANDLING, L0_SataErrorHandling, NULL);
    L0_EventRegHander(L0_EVENT_TYPE_CMD_REJECT, L0_SataCmdReject, NULL);   
#ifndef SIM
    L0_EventRegHander(L0_EVENT_TYPE_ATASTANDBYTIMEOUT, L0_ATAPowerProcIdleToStandby, NULL);
#endif
#endif
}


void L0_SaveFW2GB(void)
{
    pSaveFW pSaveFWFunc;
    PTABLE *pTable;

    pTable = (PTABLE *)HAL_GetPTableAddr();

    if (0 != pTable->sBootStaticFlag.bsRebuildGB)
    {
        pSaveFWFunc = (pSaveFW)HAL_GetFTableFuncAddr(SAVE_FIRWARE);
        pSaveFWFunc(0, DRAM_FW_UPDATE_BASE);
    }

    return;
}

void L0_SubSystemInit(U32 ulHotStartFlag)
{
    U32 ulBootType;
    U32 ulLLFUseDefault;

    if (FALSE == ulHotStartFlag)
    {
        ulBootType = g_pBootParamTable->sBootStaticFlag.bsBootMethodSel;

        g_ulSubsysNum = 1;

        /* Calculating subsystem number bits. */
        g_ulSubsysNumBits = 0;

        /* Clear local host information data area before normal boot.
                   Each subsystem would load host information data from flash. */
        L0_InitHostInfoPage();

        if (BOOT_METHOD_LLF == ulBootType)
        {
            /*TempFix:Currently, Cann't access Flash in FPGA.*/
            L0_SaveFW2GB();
            
            /* Checking if rolling-back host information page to default value is requested. */
            ulLLFUseDefault = g_pBootParamTable->sBootStaticFlag.bsUseDefault;
            
        }

        /* Starts subsystem MCUs and issues low-level format requests on demand. */
        L0_InitSCQ(0);
        L0_InitSMQ(0);
        g_ulNtfnMsgSeq = 0;

        HAL_StartSubSystemMCU(0);

        if (BOOT_METHOD_LLF == ulBootType)
        {
            /* pipe line two SubSystems to do LLF SCMD to reduce LLF time */
            //L0_WaitForAllSCmdCpl(ulSubSysIdx);
            (void)L0_IssueLLFSCmd(0, g_ulHostInfoAddr, ulLLFUseDefault);
        }

        g_ulSubSysBootOk = FALSE;
        
        (void)L0_IssueBootSCmd(0, g_ulHostInfoAddr);

        /* set boot event for global scheduler handling */
        L0_EventSet(L0_EVENT_TYPE_SUBSYS_BOOT, NULL);
    }

    else
    {
        HAL_StartSubSystemMCU(0);
    }

    return;
}

void L0_SubSystemOnlineShutdown(U32 ulIsSanitize)
{
    // Shutdown Subsystem
    /* ulIsSanitize - The flag for entry from Security Erase operation.
               If subsystems just executed LLF operation, shut down operation is not required. */
    L0_WaitForAllSCmdCpl(0);

    if (FALSE == ulIsSanitize)
    {
        L0_IssuePwrCylSCmd(g_ulHostInfoAddr);
    }

    DBG_Printf("Send Shutdown SCMD done.\n");

    // Force Idle
    if (FALSE == ulIsSanitize)
    {
        L0_ForceAllSubSysIdle();
    }

    DBG_Printf("Send Idle SCMD done.\n");

    // Stall & Reset MCU1&2
#ifndef SIM
    HEAD_StallMcu12(TRUE);
    HEAD_RstMcu12(TRUE);
#endif
    DBG_Printf("Reset MCU 1&2 done. \n");

    return;
}

void L0_SubSystemOnlineReboot(void)
{
    g_ulSubSysBootOk = FALSE;    
    
#ifdef VT3533_A2ECO_DSGRLSBUG
    HAL_NormalDsgReset();
#ifdef HOST_NVME
    HAL_NormalDsgFifoInit();
#endif
#endif

    /* Starts subsystem MCUs and issues low-level format requests on demand. */
    L0_InitSCQ(0);
    L0_InitSMQ(0);
    g_ulNtfnMsgSeq = 0;
    L0_IssueBootSCmd(0, g_ulHostInfoAddr);


    HAL_StartSubSystemMCU(0);
    L0_WaitForAllSCmdCpl(0);

    DBG_Printf("Reboot completed\n");

    return;
}

void L0_InitHostInfoPage(void)
{
    PHOST_INFO_PAGE pHostInfoPage;

    pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    COM_MemZero((U32 *)pHostInfoPage, sizeof(HOST_INFO_PAGE)/sizeof(U32));
    pHostInfoPage->HPAMaxLBA = g_pBootParamTable->ulSysMaxLBACnt;

    return;
}

/****************************************************************************
Name        :L0_ProcessSCMDCompletion
Input       : ulSubSysId - Subsystem index;
                pSCNode - The pointer to the completed SCMD.
Output      : None
Author      :
Date        :
Description : The interface routine checks return status with the SCMD node waiting to be recycled.
                Call-back handlers can be implemented here for processing the completion of some specific
                types of SCMD.
Others      :
Modify      :
****************************************************************************/
U32 L0_ProcessSCMDCompletion(U32 ulSubSysId, PSCMD pSCNode)
{
    U32 ulSCType;
    U32 ulRetStatus;

    ulSCType = pSCNode->ucSCmdType;
    ulRetStatus = pSCNode->ucSCmdStatus;

    switch (ulSCType)
    {
        case (U32)SCMD_RAW_DATA_REQ:
            if ((U32)SSTS_SUCCESS == ulRetStatus)
            {
                g_ulRawDataReadyFlag = TRUE;
            }

            break;

        case (U32)SCMD_VIA_UART_RAW_DATA_REQ:
            if ((U32)SSTS_SUCCESS == ulRetStatus)
            {
                g_ulVUARTRawDataReadyFlag = TRUE;
            }

            break;

        case (U32)SCMD_IDLETASK:
            /* Call-back handler for IDLE SCMD. */
            if ((U32)SSTS_ABORTED == ulRetStatus)
            {
                g_ulL0IdleTaskAborted = TRUE;
            }

            else if ((U32)SSTS_SUCCESS == ulRetStatus)
            {
                g_ulL0IdleTaskFinished = TRUE;
            }

            break;

        case (U32)SCMD_BOOTUP:
            /* Call-back handler for BOOT SCMD. */
            if ((U32)SSTS_SUCCESS == ulRetStatus)
            {
                g_ulSubSysBootOk = TRUE;;
            }

            break;

        case (U32)SCMD_POWERCYCLING:
            /* Call-back handler for ShutDown SCMD. */
            if ((U32)SSTS_SUCCESS == ulRetStatus)
            {
                g_ulShutdownReady = TRUE;
            }

            break;
        
        case (U32)SCMD_FLUSH:
            /* Call-back handler for Flush Cache SCMD. */
            if ((U32)SSTS_SUCCESS == ulRetStatus)
            {
                g_ulFlushCacheReady = TRUE;
            }

            break;
            
        case (U32)SCMD_VIA_DEV_CTRL:
            if ((U32)SSTS_SUCCESS != ulRetStatus)
            {
                g_tL0ViaCmdSts = VCS_OPERATION_FAIL;
                DBG_Printf("L0_ProcessSCMDCompletion: SCMD_VIA_DEV_CTRL Fail, ulRetStatus (%d)\n",ulRetStatus);
            }
            else
            {
                g_tL0ViaCmdSts = VCS_SUCCESS;
                //DBG_Printf("L0_ProcessSCMDCompletion: SCMD_VIA_DEV_CTRL success, ulRetStatus (%d)\n",ulRetStatus);
            }
            break;

        default:
            break;
    }

    return ulRetStatus;
}

void L0_ProcSubsysMsg(void)
{
    U32 ulSubsysID;
    PSMSG pCurrSubsysMsg;
    U32 ulErrorSlotID;
    BOOL bFinish = TRUE;

    /* Marks the error status for an UECC error reported by either subsystem. */
    for (ulSubsysID = 0; ulSubsysID < g_ulSubsysNum; ulSubsysID++)
    {
        /* Attempts to get a message from either subsystem. */
        pCurrSubsysMsg = L0_GetSMSGFromHead(ulSubsysID);

        if (NULL != pCurrSubsysMsg)
        {
            /* Currently the subsystem message is only used for an error notification. */
            if (SMSG_SUBSYS_ERROR == pCurrSubsysMsg->ulMsgCode)
            {
                if (SUBSYS_ERROR_UECC == pCurrSubsysMsg->ulMsgParam[0])
                {
                    /* Parameter 1 is the slot in which the error occured. */
                    ulErrorSlotID = pCurrSubsysMsg->ulMsgParam[1];
#if defined(HOST_AHCI)
                    (void)L0_AhciSetDataError(ulErrorSlotID);
#elif defined(HOST_NVME)
                    //L0_NVMeUecc(ulErrorSlotID);
                    if (FAIL == L0_PushMsgQueueUeccNode(ulSubsysID,
                            (pCurrSubsysMsg->ulMsgParam[1] & L0MSG_SUBTYPE_UECCERROR_SLOTID_MASK)>> L0MSG_SUBTYPE_UECCERROR_SLOTID_SHIF, 
                            (pCurrSubsysMsg->ulMsgParam[1] & L0MSG_SUBTYPE_UECCERROR_PUID_MASK) >> L0MSG_SUBTYPE_UECCERROR_PUID_SHIF))
                    {
                        DBG_Printf("Warning SubSys(%d) Push Uecc SMS(Param[1]:0x%x) to L0MsgQueue(NodeNum:%d) fail\n",
                                    ulSubsysID, pCurrSubsysMsg->ulMsgParam[1], L0_GetMsgQueueNodeNum());
                        bFinish = FAIL;
                    }
#elif defined(HOST_SATA)
                    L0_SataReceiveUECCMsg(ulErrorSlotID);
#endif
                    DBG_Printf("L0 receive SMSG of UECC from SubSys(%d), ErrorSlotID: 0x%x\n",ulSubsysID, ulErrorSlotID);
                    TRACE_LOG((void*)&ulErrorSlotID, sizeof(U32), U32, 0, "L0 receive SMSG of UECC, ErrorSlotID: ");
                }

                else
                {
                    /* We only process an UECC error currently. */
                    ;
                }
            }

            else if (SMSG_SCQ_CLEARED == pCurrSubsysMsg->ulMsgCode)
            {
                g_ulSubsysSCQClrMap |= (1 << ulSubsysID);
            }

            else if (SMSG_INFORM_DBG_GETCH == pCurrSubsysMsg->ulMsgCode)
            {
                L0_EventSet(L0_EVENT_TYPE_DEBUG_MODE_EN, NULL);
                g_ulDebugModeState = DEBUG_MODE_STATE_ENTER;
            }

            else if (SMSG_FINISH_DBG_GETCH == pCurrSubsysMsg->ulMsgCode)
            {
                g_ulDebugModeFinishMap |= (1 << ulSubsysID);
            }

            /* move the head pointer of SMSG FIFO */
            if (TRUE == bFinish)
            {
                L0_PopSMSGNode(ulSubsysID);
            }
        }
    }

    return;
}

BOOL L0_TaskSwitchToDebugMode(void *pParam)
{
    U32 ulSubSysIdx;
    pSavePTable pSavePTableFunc;
    BOOL bDebugSwitchDone = FALSE;

    switch (g_ulDebugModeState)
    {
        case DEBUG_MODE_STATE_ENTER:

            DBG_Printf("L0 start switching to debug mode\n");
            TRACE_LOG((void*)&g_ulDebugModeState, sizeof(U32), U32, 0, "L0 switch to debug mode: ");
            
            g_ulDebugModeState = DEBUG_MODE_STATE_NTFY_SUBSYS;
        break;

        case DEBUG_MODE_STATE_NTFY_SUBSYS:
            g_ulDebugModeFinishMap = 0;
            
            /* Notify all sub-systems by sending interrupt */
            L0_PostNtfnMsg((1 << g_ulSubsysNum) - 1, SMSG_DBG_GETCH_CLEAR_SCQ);

            g_ulDebugModeState = DEBUG_MODE_STATE_WAIT_SUBSYS;
        break;

        case DEBUG_MODE_STATE_WAIT_SUBSYS:
            if (g_ulDebugModeFinishMap != ((1 << g_ulSubsysNum) - 1))
            {
                //just wait
            }
            else
            {
                g_ulDebugModeState = DEBUG_MODE_STATE_RE_INIT;
            }
        break;
        
        case DEBUG_MODE_STATE_RE_INIT:
            g_ulSubSysBootOk = FALSE;

            //require sub-system reload host info page
            for (ulSubSysIdx = 0; ulSubSysIdx < g_ulSubsysNum; ulSubSysIdx++)
            {
                (void)L0_IssueBootSCmd(ulSubSysIdx, g_ulHostInfoAddr);
            }

            g_ulDebugModeState = DEBUG_MODE_STATE_WAIT_INIT;
        break;

        case DEBUG_MODE_STATE_WAIT_INIT:
            if (FALSE == g_ulSubSysBootOk)
            {
                //just wait
            }
            else
            {
                g_ulDebugModeState = DEBUG_MODE_STATE_FINISH;
            }
        break;

        case DEBUG_MODE_STATE_FINISH:
            /* wait SubSystem switch to Ramdisk then save bootloader lock disk page, to prevent Flash operations in SubSystem */
            pSavePTableFunc = (pSavePTable)HAL_GetFTableFuncAddr(SAVE_PTABLE);
            pSavePTableFunc();

            DBG_Printf("L0 finish switching to debug mode\n");
            
            L0_TaskManagerInit(FALSE);
            bDebugSwitchDone = TRUE;
        break;

        default:
            DBG_Getch();
        break;
    }//end switch (g_ulDebugModeState)

    return bDebugSwitchDone;
}

BOOL L0_TaskWaitBoot(void *pParam)
{
    return g_ulSubSysBootOk;
}

BOOL L0_TaskXferReset(void *pParam)
{
    L0_TaskManagerInit(FALSE);

    return TRUE;
}


BOOL L0_TaskShutDown(void *pParam)
{
    static U32 ulShutDownStage;
    BOOL bFinished = FALSE;

    if (L0_EVENT_STAGE_START == L0_EventGetStage(L0_EVENT_TYPE_SHUTDOWN))
    {
        ulShutDownStage = 0;
        L0_EventSetStage(L0_EVENT_TYPE_SHUTDOWN, L0_EVENT_STAGE_PROCESSING);
    }

    switch (ulShutDownStage)
    {
        case 0:
            g_ulShutdownReady = FALSE;

#ifdef AF_ENABLE
            STOP_AF();
            ulShutDownStage = 1;
            break;
#else
            ulShutDownStage = 1;
            //go through to next stage
#endif 
        case 1:
            /* Attempts to send a ShutDown SCMD to subsystem. */
            if (SUCCESS == L0_IssuePwrCylSCmd(g_ulHostInfoAddr))
            {
                ulShutDownStage = 2;
            }

            break;

        case 2:
            /* Just waiting for subsystem to finish the SCMD. */
            if (FALSE != g_ulShutdownReady)
            {
                ulShutDownStage = 0;
                bFinished = TRUE;
            }

            break;

        default:
            ASSERT(FAIL);
    }

    return bFinished;
}

MCU0_DRAM_TEXT void L0_TaskManagerInit(U32 ulIsHotStart)
{
    //U32 ulSubsysId;
    //L0_TASK_MANAGER *pL0TaskMgr = &g_tL0TaskMgr;
    
    g_tL0TaskMgr.PrevPortIdle = TRUE;
    //g_tL0TaskMgr.CmdRunning = FALSE;
    g_tL0TaskMgr.IdleLoopCount = 0;
    g_tL0TaskMgr.PMState = (U32)PORTPM_ACTIVE;
    g_tL0TaskMgr.PrevPMState = (U32)PORTPM_ACTIVE;
#ifndef SIM
    HAL_PMClearDevSleepFlag();
#endif
    /* Setting system-level power management enable flag according to P-Table. */
    if (FALSE == ulIsHotStart)
    {
        if(BOOT_METHOD_MPT == HAL_GetBootMethod())
        {
            l_ulSysLevPMEnable = FALSE;
        }
        else
        {
#ifdef HOST_SATA
            l_ulSysLevPMEnable = g_pBootParamTable->tSATAL0Feature.tSATAL0Feat.bsSATAL0PMUEnable;
#else
            l_ulSysLevPMEnable = g_pBootParamTable->tNVMeL0Feature.tNVMEL0Feat.bsNVMeL0PMUEnable;
#endif        
        }
    }

    /* Checking which copy of host information page loaded from subsystems can be used. */
#if 0
    for (ulSubsysId = 0; ulSubsysId < g_ulSubsysNum; ulSubsysId++)
    {
        
    }
#endif

    return;
}

LOCAL void L0ClearIdleTaskTimeOutFlag(void)
{
    l_ulIdleTaskTimeOutFlag = FALSE;
    return;
}

LOCAL void L0SetIdleTaskTimeOutFlag(void)
{
    l_ulIdleTaskTimeOutFlag = TRUE;
    return;
}

LOCAL U32 L0GetIdleTaskTimeOutFlag(void)
{
    return l_ulIdleTaskTimeOutFlag;
}

LOCAL U32 L0PowerManagerIdleProc(L0_TASK_MANAGER *pL0TaskMgr)
{
#ifdef SIM
    static U32 ulIdleTaskExecCnt;
#else
    static U32 ulIdleTaskExecStat;
#endif

    if ((U32)PORTPM_IDLE != pL0TaskMgr->PrevPMState)
    {
        /* Initializes counters and flags when system enters
            IDLE state from ACTIVE state. */
#ifdef SIM
        /* Clears idle "timeslice" counter. */
        ulIdleTaskExecCnt = 0;
#else
        ulIdleTaskExecStat = PORTPM_IDLETASKSTATE_START;
#endif
        /* Clears aborted flag. */
        g_ulL0IdleTaskAborted = FALSE;

        /* Clears completion flag. */
        g_ulL0IdleTaskFinished = FALSE;
    }

    /* Checking if all subsystems have completed idle task, 
            or either subsystem has aborted idle task. */
    if (TRUE == g_ulL0IdleTaskAborted)
    {
#ifndef SIM
        HAL_StopMcuTimer();
#endif
        return PORTPM_IDLETASKSTATUS_ABORTED;
    }

    else if (TRUE == g_ulL0IdleTaskFinished)
    {
#ifndef SIM
        HAL_StopMcuTimer();
#endif
        return PORTPM_IDLETASKSTATUS_FINISHED;
    }

#ifdef SIM
    /* Attempts to issue one IDLE TASK SCMD to a subsystem. */
    if (FALSE == g_ulL0IdleTaskFinished)
    {
        /* Only keeps idle schedule counter in simulation environment. */
        if (IDLETASK_COUNT_MAX > ulIdleTaskExecCnt)
        {
            /* The maximum allowed idle task "timeslice" threshold has not been reached.
                Issues an IDLE subcommand with normal priority. */
            if (SUCCESS == L0_IssueIdleSCmd(0, (U32)IDLE_NORMAL))
            {
                /* Updates "timeslice" count already consumpted only
                    if SCMD has been issued successfully, because an IDLE SCMD
                    can only be issued when no other SCMD is pending. */
                ulIdleTaskExecCnt++;
            }
        }

        else if (IDLETASK_COUNT_MAX == ulIdleTaskExecCnt)
        {
            /* The maximum allowed idle task "timeslice" threshold has just been reached.
                Issues an IDLE subcommand with critical priority to force L1/L2/L3 stopping
                subsequent idle task scheduling. */
            if (SUCCESS == L0_IssueIdleSCmd(0, (U32)IDLE_CRITICAL))
            {
                /* Updates "timeslice" count already consumpted only
                    if SCMD has been issued successfully, because an IDLE SCMD
                    can only be issued when no other SCMD is pending. */
                ulIdleTaskExecCnt++;
            }
        }

        else
        {
            /* The maximum allowed idle task "timeslice" threshold is already exceeded. */
            /* This means the IDLE subcommand with critical priority has already been issued. */
            /* So the only thing we need is waiting for L1 to complete its idle work. */
            ;
        }
    }
#else
    /* Instead of idle task counter, we adopt a time out mechanism in FPGA/ASIC environment. */
    switch (ulIdleTaskExecStat)
    {
        case PORTPM_IDLETASKSTATE_START:
            /* Starts the timer. */
            HAL_StopMcuTimer();
            L0ClearIdleTaskTimeOutFlag();
            HAL_StartMcuTimer(PORTPM_IDLETASK_EXEC_PERIOD, L0SetIdleTaskTimeOutFlag);
            ulIdleTaskExecStat = PORTPM_IDLETASKSTATE_EXEC;

        case PORTPM_IDLETASKSTATE_EXEC:
            if (TRUE == L0GetIdleTaskTimeOutFlag())
            {
                /* Timer has timed out. We must force idle task to stop now. */
                ulIdleTaskExecStat = PORTPM_IDLETASKSTATE_STOP;
            }

            else
            {
                /* Attempts to issue one IDLE TASK SCMD with normal priority to each subsystem. */
                (void)L0_IssueIdleSCmd(0, (U32)IDLE_NORMAL);
            }

            break;
        
        case PORTPM_IDLETASKSTATE_STOP:
            /* Attempts to issue one IDLE TASK SCMD with critical priority to each subsystem. */
            if (SUCCESS == L0_IssueIdleSCmd(0, (U32)IDLE_CRITICAL))
            {
                ulIdleTaskExecStat = PORTPM_IDLETASKSTATE_WAIT;
            }

            break;

        case PORTPM_IDLETASKSTATE_WAIT:
            /* Just waits for g_ulL0IdleTaskFinished changes outside this function. */
            break;

        default:
            DBG_Printf("Idle task state machine abnormal: %d\n", ulIdleTaskExecStat);
            DBG_Getch();
            break;
    }
#endif

    return PORTPM_IDLETASKSTATUS_PROCESSING;
}

BOOL L0_IsTaskMgrActive(void)
{
    if (PORTPM_ACTIVE == g_tL0TaskMgr.PMState)
    {
        return TRUE;
    }

    else
    {
        return FALSE;
    }
}

#if 0
//#ifdef HOST_SATA
BOOL L0_TaskManagement(void)
{
    U32 ulCurrPMState, ulNextPMState;
    L0_TASK_MANAGER *pL0TaskMgr = &g_tL0TaskMgr;
    BOOL bCmdProcessContinue = FALSE;
    U32 ulIdleTaskExecStatus;
        
    /* Implementing system-level power management policy. */
    ulNextPMState = ulCurrPMState = pL0TaskMgr->PMState;

    switch (ulCurrPMState)
    {
        /* This branch decides whether power-saving state may be
            entered when system is active executing host commands. */
        case (U32)PORTPM_ACTIVE:
#if 0
            /* Since subsystems would halt themselves after completing
                all idle task, we must wake all subsystem MCUs up after
                system returns to ACTIVE state. */
            if ((U32)PORTPM_ACTIVE != pL0TaskMgr->PrevPMState)
            {
                L0_PostNtfnMsg(((1 << g_ulSubsysNum) - 1), SMSG_WAKEUP_SUBSYS);
            }
#endif
            /* Checking status of PxCMD.ST bit. */
            if (TRUE == pL0TaskMgr->CmdRunning)
            {
                /* PxCMD.ST bit remains 1. */
                if (PORTPM_IDLE_LOOP_THRESHOLD <= pL0TaskMgr->IdleLoopCount)
                {
                    /* Host idle time reaches our threshold. So we may start idle task
                         after pending SCMDs having been finished. */
                    if (TRUE == L0_CheckSCQAllEmpty())
                    {
                        ulNextPMState = (U32)PORTPM_IDLE;
                    }

                    else
                    {
                        /* Waiting for L1 to finish pending SCMDs. */
                        ;
                    }
                }

                else
                {
                    /* Continues command processing before host remain in
                        idle for enough time. */
                    bCmdProcessContinue = TRUE;
                }
            }

            else
            {
                /* We are able to enter idle state after all DMA transfer finished. */
                ulNextPMState = (U32)PORTPM_IDLE;
            }

            break;

        /* This branch performs local idle tasks such as table flushing
            and GC in host idle condition. */
        case (U32)PORTPM_IDLE:
            /* Checking whether host side remains idle. */
            if (FALSE == pL0TaskMgr->PrevPortIdle)
            {
                if (TRUE == L0_CheckSCQAllEmpty())
                {
                    ulNextPMState = (U32)PORTPM_ACTIVE;
                }

                else
                {
                    /* Allows L1 to complete all IDLE SCMDs before we return to ACTIVE state. */
                    ;
                }
            }

            else
            {
                /* Sends IDLE_TASK subcommands to L1 in this stage. */
                ulIdleTaskExecStatus = L0_PowerManagerIdleProc(pL0TaskMgr);

                if (PORTPM_IDLETASKSTATUS_ABORTED == ulIdleTaskExecStatus)
                {
                    /* Either subsystem has aborted the idle task. We must terminate this round of idle task scheduling. */
                    ulNextPMState = (U32)PORTPM_ACTIVE;
                }

                else if (PORTPM_IDLETASKSTATUS_FINISHED == ulIdleTaskExecStatus)
                {
                    /* All subsystems have completed this round of idle task scheduling. */
                    ulNextPMState = (U32)PORTPM_SUSP_PREP;
                }
            }

            break;

        /* This branch performs backup work before we enter power-saving state. */
        case (U32)PORTPM_SUSP_PREP:
            /* Checking whether host side remains idle. */
            if (FALSE == pL0TaskMgr->PrevPortIdle)
            {
                HAL_HaltSubSystemMCU(FALSE);
                L0_PrepNextTimeKeeping();
                ulNextPMState = (U32)PORTPM_ACTIVE;
            }

#ifndef SIM
            else if (TRUE == l_ulSysLevPMEnable)
            {
                if (TRUE == HAL_PMIsDevInLowPwrSts())
                {
                    L0_PauseTimeKeepingTask();
                    HAL_HaltSubSystemMCU(TRUE);

                    if (TRUE == HAL_PMGetDevSleepFlag())
                    {
                        ulNextPMState = (U32)PORTPM_SUSP_READY;
                    }

                    else
                    {
                        HAL_PMDozeOffMCU(SYSPM_TIME_WAITING_SCHEDULE);
                        HAL_PMShutDownPLL();
                    }
                }
            }
            else
            {
                // For supporting UART MP
                bCmdProcessContinue = TRUE;
            }

            break;

        case (U32)PORTPM_SUSP_READY:
            HAL_PMEnablePLL(PMU_DPLL_CNTL_BIT | PMU_HPLL_CNTL_BIT);
            HEAD_RelocationSramToDram(TRUE);
            HAL_PMDozeOffMCU(SYSPM_TIME_WAITING_SCHEDULE);
            HAL_PMInitiateSuspending();

            HAL_HaltSubSystemMCU(FALSE);
            L0_PrepNextTimeKeeping();
            ulNextPMState = (U32)PORTPM_ACTIVE;
#endif
            break;

        default:
            break;
    }

#if 0
    else
    {
        /* Initiates level 1 clock gating for idle state power saving instead. */
        L0_PauseTimeKeepingTask();
        HAL_HaltSubSystemMCU(TRUE);
        HAL_PMEnableL1ClockGating();

        ulNextPMState = (U32)PORTPM_STANDBY;
    }
#endif

    pL0TaskMgr->PrevPMState = ulCurrPMState;
    pL0TaskMgr->PMState = ulNextPMState;

    return bCmdProcessContinue;
}

#else
BOOL L0_TaskManagement(void)
{
    U32 ulCurrPMState, ulNextPMState;
    L0_TASK_MANAGER *pL0TaskMgr = &g_tL0TaskMgr;
    BOOL bCmdProcessContinue = FALSE;
    U32 ulIdleTaskExecStatus;
        
    /* Implementing system-level power management policy. */
    ulNextPMState = ulCurrPMState = pL0TaskMgr->PMState;

    switch (ulCurrPMState)
    {
        /* These branches roll back system status from power-saving state to active. */
        case (U32)PORTPM_ACTIVE_PREP_STG1:
#if (defined(SIM) || defined (XTMP))
#else
            if (TRUE == HAL_PMIsMainPLLReady())
            {
                /* Restarts subsystems. */
                HAL_HaltSubSystemMCU(FALSE);
                ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG2;
            }

            break;
#endif

        case (U32)PORTPM_ACTIVE_PREP_STG2:
            /* Restarts time keeping task scheduling. */
            L0_PrepNextTimeKeeping();

#ifdef PCIE_ASPM_MANAGEMENT
        case (U32)PORTPM_ACTIVE_PREP_STG3:
            /* Disables PCIe ASPM L1. */
            //HAL_PMEnablePCIeASPM(FALSE);
            /* Clears squelch flag. */
            HAL_PMSetSqlchFlag(FALSE);
#endif
            ulNextPMState = (U32)PORTPM_ACTIVE;

            break;

        /* This branch decides whether power-saving state may be
            entered when system is active executing host commands. */
        case (U32)PORTPM_ACTIVE:
#ifndef PCIE_ASPM_MANAGEMENT
            if (PORTPM_IDLE_LOOP_THRESHOLD <= pL0TaskMgr->IdleLoopCount)
            {
                /* Host idle time reaches our threshold. So we may start idle task
                             after pending SCMDs having been finished. */
                if (TRUE == L0_CheckSCQAllEmpty())
                {
                    L0_PauseTimeKeepingTask();
                    ulNextPMState = (U32)PORTPM_IDLE;
                }

                else
                {
                    /* Waiting for L1 to finish pending SCMDs. */
                    ;
                }
            }

#else
            if (PORTPM_ASPML1_LOOP_THRESHOLD <= pL0TaskMgr->IdleLoopCount)
            {
                HAL_DisableMCUIntAck();
                if (FALSE == HAL_PMGetSqlchFlag())
                {
                    /* Enables PCIe ASPM L1 as needed. */
                    HAL_PMEnablePCIeASPM(TRUE);
                }
                else
                {
                    HAL_PMSetSqlchFlag(FALSE);
                    pL0TaskMgr->IdleLoopCount = 0;
                }
                HAL_EnableMCUIntAck();
                ulNextPMState = (U32)PORTPM_ACTIVE_ASPML1_ENABLED;
            }
#endif
            else
            {
                /* Continues command processing before host remain in
                            idle for enough time. */
                bCmdProcessContinue = TRUE;
            }

            break;

#ifdef PCIE_ASPM_MANAGEMENT
        case (U32)PORTPM_ACTIVE_ASPML1_ENABLED:
            if (PORTPM_ASPML1_LOOP_THRESHOLD > pL0TaskMgr->IdleLoopCount)
            {
                ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG3;
            }

            else if (PORTPM_IDLE_LOOP_THRESHOLD <= pL0TaskMgr->IdleLoopCount)
            {
                /* Host idle time reaches our threshold. So we may start idle task
                             after pending SCMDs having been finished. */
                if (TRUE == L0_CheckSCQAllEmpty())
                {
                    L0_PauseTimeKeepingTask();
                    ulNextPMState = (U32)PORTPM_IDLE;
                }

                else
                {
                    /* Waiting for L1 to finish pending SCMDs. */
                    ;
                }
            }

            break;
#endif

        /* This branch performs local idle tasks such as table flushing
            and GC in host idle condition. */
        case (U32)PORTPM_IDLE:
            /* Checking whether host side remains idle. */
            if (FALSE == pL0TaskMgr->PrevPortIdle)
            {
                if (TRUE == L0_CheckSCQAllEmpty())
                {
                    ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG2;
                }

                else
                {
                    /* Allows L1 to complete all IDLE SCMDs before we return to ACTIVE state. */
                    ;
                }
            }

            else
            {
                /* Sends IDLE_TASK subcommands to L1 in this stage. */
                ulIdleTaskExecStatus = L0PowerManagerIdleProc(pL0TaskMgr);
#ifdef HOST_NVME
                /*disable LED when idle*/
                if(bLEDSetActive == TRUE)
                {
                     /* disable LED function clock and disable LED GPIO */
                    (*((volatile U32*) (REG_BASE_GLB + 0x04))) |= (0 << 13);
                    rGPIOLED = 0x2a2a;
                    bLEDSetActive = FALSE;
                }
#endif

                if (PORTPM_IDLETASKSTATUS_ABORTED == ulIdleTaskExecStatus)
                {
                    /* Clears idle counter to force a new initiating of idle task. */
                    pL0TaskMgr->IdleLoopCount = 0;

                    /* Either subsystem has aborted the idle task. We must terminate this round of idle task scheduling. */
                    ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG2;
                }

                else if (PORTPM_IDLETASKSTATUS_FINISHED == ulIdleTaskExecStatus)
                {
                    /* Stall sub mcu except HAL unit test. */
                    #ifndef HAL_UNIT_TEST
                        /* All subsystems have completed this round of idle task scheduling. */
                        HAL_HaltSubSystemMCU(TRUE);
                    #endif
                    
#if 0  //for selfwake temp test
#ifndef SIM
                    HAL_PMStartTimer(PMU_TIMER2, 25000u * 100u);
#endif
#endif
                    ulNextPMState = (U32)PORTPM_SLEEP_PREP;
                }
            }

            break;

        /* This branch performs backup work before we enter power-saving state. */
        case (U32)PORTPM_SLEEP_PREP:
            /* Checking whether host side remains idle. */
            if (FALSE == pL0TaskMgr->PrevPortIdle)
            {
                //FIRMWARE_LogInfo("L0 Reactive\n");
#if 0  //for selfwake temp test
#ifndef SIM
                HAL_PMStopTimer(PMU_TIMER2);
                HAL_PMClearTimeOutFlag(PMU_TIMER2);
#endif
#endif
                ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
            }
#if 0  //for selfwake temp test
#ifndef SIM
            else if (TRUE == HAL_PMIsTimerTimedOut(PMU_TIMER2))
            {
                //DBG_Printf("L0 SelfWake\n");
                HAL_PMClearTimeOutFlag(PMU_TIMER2);
                ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
            }
#endif
#endif
#ifdef SIM
            else if (PORTPM_SELFWAKE_LOOP_THRESHOLD <= pL0TaskMgr->IdleLoopCount)
            {
                //FIRMWARE_LogInfo("L0 SelfWake\n");
                pL0TaskMgr->IdleLoopCount = 0;
                ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
            }
#endif
#if (defined(SIM) || defined (XTMP))
#else
            else if (TRUE == l_ulSysLevPMEnable)
            {
#ifdef HOST_NVME
                L0_NVMePrepareL12Entry();
#endif
                HEAD_RelocationSramToDram(TRUE);
#ifdef CALC_RINGOSC_CLK
                HAL_PMDozeOffMCU((HAL_PMGetTimerTickPerMS() / 1000u) * 60u); // Gives DRAM some idle time (60us) to enter auto-self-refresh state.
#else
                HAL_PMDozeOffMCU(1536);
#endif
                ulNextPMState = (U32)PORTPM_SLEEP_ENTRY;
            }

            break;

        /* This branch selects which power-saving state to enter when the corresponding
        conditions are met. */
        case (U32)PORTPM_SLEEP_ENTRY:
            /* Checking whether host side remains idle. */
            if (FALSE == pL0TaskMgr->PrevPortIdle)
            {
                ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
            }
#ifdef HOST_SATA
            else if (TRUE == HAL_PMGetDevSleepFlag())
            {
                HAL_PMClearSafetyBlock();
                ulNextPMState = (U32)PORTPM_SUSP_READY;
            }
#endif
            else if (TRUE == HAL_PMIsDevInLowPwrSts())
            {
                ulNextPMState = (U32)PORTPM_LIGHTSLEEP;
            }

#ifdef PM_STANDBY_SUPPORT
            else
            {
                if (TRUE == HAL_PMInitiateStandby())
                {
                    ulNextPMState = (U32)PORTPM_STANDBY;
                }
            }
#endif
            break;

        /* This branch waits serial link to enter low-power state in a clock gated state. */
#ifdef PM_STANDBY_SUPPORT
        case (U32)PORTPM_STANDBY:
            /* Checking whether host side remains idle. */
            if (FALSE == pL0TaskMgr->PrevPortIdle)
            {
                /* Disables level 1 clock gating to return all hardware modules to operable state. */
                if (FALSE != HAL_PMExitStandby())
                {
                    ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
                }
            }

            else if ((FALSE != HAL_PMIsDevInLowPwrSts())
#ifdef HOST_SATA
                || (FALSE != HAL_PMGetDevSleepFlag())
#endif
            )
            {
                /* Now serial link has entered power-saving state. We can roll back from clock gated state.  */
                if (FALSE != HAL_PMExitStandby())
                {
                    ulNextPMState = (U32)PORTPM_SLEEP_ENTRY;
                }
            }

            /* Checks PCIe squelch signal (exiting L1). */
#ifdef PCIE_ASPM_MANAGEMENT
            else if (FALSE != HAL_PMGetSqlchFlag())
            {
                /* Disables level 1 clock gating to return all hardware modules to operable state. */
                if (FALSE != HAL_PMExitStandby())
                {
                    HAL_DisableMCUIntAck();
                    HAL_PMSetSqlchFlag(FALSE);
                    ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
                    HAL_EnableMCUIntAck();
                }
            }
#endif
            break;
#endif

        /* This branch programs hardware to shut down PLLs and takes system to an intermediate power-saving state. */
        case (U32)PORTPM_LIGHTSLEEP:
            /* Enters critical section. */
            HAL_DisableMCUIntAck();
#ifdef HOST_SATA
            if (TRUE == HAL_PMGetDevSleepFlag())
            {
                /* Exits critical section. */
                HAL_EnableMCUIntAck();

                /* L1.2 or DEVSLP has been guaranteed. */
                ulNextPMState = (U32)PORTPM_SUSP_READY;
            }

            else
#else
            /* For NVMe solution, delay 2 microseconds to confirm that link has entered L1.0 idle. */
            HAL_DelayUs(2);

            /* Enables PCIe hardware for releasing CLKREQ# signal and enter L1.2 Entry state. */
            HAL_PMEnablePCIeL12Ready();
#endif
            {
                /* Condition of deep sleep is not met, so
                                    we can only engage light sleep. */
                HAL_PMShutDownPLL();

#ifdef HOST_NVME
                if (FALSE == HAL_NVMeGetL12IdleFlag())
                {
                    /* Reverses PCIe setting back from L1.2 preparing state. */
                    HAL_PMDisablePCIeL12Ready();
                    ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
                }
                
                else
                {
                    HAL_PMInitiateSuspending();
                    ulNextPMState = (U32)PORTPM_SUSP_READY;
                }

                pL0TaskMgr->IdleLoopCount = 0;
#else
                ulNextPMState = (U32)PORTPM_SLEEP_ENTRY;
#endif
            }

            break;
                
        /* This branch checks condition and and enters final power-saving state if met. */
        case (U32)PORTPM_SUSP_READY:
#ifndef HOST_NVME
            if (TRUE == HAL_PMGetDevSleepFlag())
            {
                HAL_PMInitiateSuspending();
            }

            ulNextPMState = (U32)PORTPM_SLEEP_ENTRY;
#else
            HAL_PMDisablePCIeL12Ready();
            ulNextPMState = (U32)PORTPM_ACTIVE_PREP_STG1;
#endif
            break;
#endif
        default:
            break;
    }

    pL0TaskMgr->PrevPMState = ulCurrPMState;
    pL0TaskMgr->PMState = ulNextPMState;

    return bCmdProcessContinue;
}

#endif

#ifdef SIM
void L0_SimSetLastTickStamp(void)
{
    l_ulLastTickStamp = GetTickCount();

    return;
}
#endif

void L0_TimeKeepingInit(void)
{
#ifndef SIM
#ifdef CALC_RINGOSC_CLK
    HAL_PMStartTimer(PMU_TIMER1, (5u * 1000u) * HAL_PMGetTimerTickPerMS());
#else
    HAL_PMStartTimer(PMU_TIMER1, L0_TIMETASK_INTVL_TICKCOUNT);
#endif
#else
    L0_SimSetLastTickStamp();
#endif

    return;
}

void L0_TimeKeepingTask(void)
{
#if ((!defined SIM) && (defined HOST_NVME))
    if ((TRUE == L0_CheckNeedToExecTimeTask()) && (FALSE == bDPLLGating))
#else
    if (TRUE == L0_CheckNeedToExecTimeTask())
#endif
    {
        /* 1. Updates power-on time record in host information page every 5 seconds. */
        L0_UpdatePowerOnTime();

        /* 2. Reads Device Param page from L1 */
        (void)L0_IssueAccessDevParamSCmd(0, (U32)g_ulDevParamAddr, (U32)GLBINFO_LOAD);

        /* 3. Checks whether we need to raise critical warnings in SMART/health Log page. */
#ifdef HOST_NVME
        L0_NVMeUpdateCrtclWrng(TRUE);
#endif
        /* 4. Initiates a host information page saving request every 8 minutes. */
        if (TRUE == L0_CheckNeedToSaveHostInfo())
        {
            l_ulSaveHostInfoPending = TRUE;
        }

        /* 5. Attempts to save host information page if there is a pending request. */
        if (FALSE != l_ulSaveHostInfoPending)
        {
            /* Tries to issue host information save SCMDs to subsystems. */
             if (FAIL != L0_IssueAccessHostInfoSCmd(0, g_ulHostInfoAddr, GLBINFO_SAVE))
             {
                l_ulSaveHostInfoPending = FALSE;
             }
        }

        /* 6. Starts next time keeping task timer or counter. */
        L0_PrepNextTimeKeeping();
    }

    return; 
}

U32 L0_CheckNeedToExecTimeTask(void)
{
#ifdef SIM
    U32 ulCurrTickStamp;
    U32 ulTickInterval;
#endif

    U32 ulNeed;

#ifdef SIM
    ulCurrTickStamp = GetTickCount();

    /* Unsigned minus can always obtain correct difference even if a wrap-around occurs. */
    ulTickInterval = ulCurrTickStamp - l_ulLastTickStamp;

    if (ulTickInterval < L0_TIMETASK_INTVL_TICKCOUNT)
    {
        ulNeed = FALSE;
    }

    else
    {
        ulNeed = TRUE;
    }

#else
    ulNeed = HAL_PMIsTimerTimedOut(PMU_TIMER1);
#endif

    return ulNeed;
}

void L0_UpdatePowerOnTime(void)
{
    U32 ulCurrMin, ulCurrSec;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    ulCurrSec = pHostInfoPage->PowerOnSecs + L0_TIMETASK_INTVL_SEC;

    if (ulCurrSec < SEC_PER_MIN)
    {
        pHostInfoPage->PowerOnSecs = ulCurrSec;
    }

    else
    {
        pHostInfoPage->PowerOnSecs = 0;
        ulCurrMin = pHostInfoPage->PowerOnMins + 1;

        if (ulCurrMin < MIN_PER_HOUR)
        {
            pHostInfoPage->PowerOnMins = ulCurrMin;
        }

        else
        {
            pHostInfoPage->PowerOnMins = 0;
            pHostInfoPage->PowerOnHours++;
        }
    }

    return;
}

U32 L0_CheckNeedToSaveHostInfo(void)
{
    U32 ulNeed;
    U32 ulCurrSec, ulCurrMin;
    PHOST_INFO_PAGE pHostInfoPage = (PHOST_INFO_PAGE)g_ulHostInfoAddr;

    ulCurrSec = pHostInfoPage->PowerOnSecs;
    ulCurrMin = pHostInfoPage->PowerOnMins;

    /* Schedules host information data saving every 8 minutes. */
    if ((0 == ulCurrSec) && (0 == (ulCurrMin & 0x7)))
    {
        ulNeed = TRUE;
    }

    else
    {
        ulNeed = FALSE;
    }

    return ulNeed;
}

void L0_PrepNextTimeKeeping(void)
{
#ifdef SIM
    L0_SimSetLastTickStamp();
#else
    HAL_PMClearTimeOutFlag(PMU_TIMER1);
#ifdef CALC_RINGOSC_CLK
    HAL_PMStartTimer(PMU_TIMER1, (5u * 1000u) * HAL_PMGetTimerTickPerMS());
#else
    HAL_PMStartTimer(PMU_TIMER1, L0_TIMETASK_INTVL_TICKCOUNT);
#endif
#endif

    return;
}

void L0_PauseTimeKeepingTask(void)
{
#ifndef SIM
    HAL_PMStopTimer(PMU_TIMER1);
    HAL_PMClearTimeOutFlag(PMU_TIMER1);
#endif

    return;
}

/****************************************************************************
Name        :L0_FwUpgradeCalcImgLen
Input        : 
Output      : 
Author       :
Date         :
Description : Parse the image head to calculate the total length of image for sata fw upgrade 
Others      :
Modify      :
****************************************************************************/

MCU0_DRAM_TEXT void L0_FwUpgradeCalcImgLen(void)
{
    U32 sum64;
    U8 *pImage = (U8*)(g_tFwUpdate.ulFwBaseAddr + 0x3C00);
    IMAGE_HDR *pHead = (IMAGE_HDR *)(pImage);

    /**
    * Because MCU1&MCU2 file last 8 bytes is used to keep the binary file information
    * file length(4B) + file offset of the section description(4B)
    * This extra 8 bytes should not been included into the final integration image.
    * But the Image_Integrate tool add the 8 bytes of MCU2 file to the image, the 8 byte
    * is not included in the header information of mcu2fw_size.
    */
    sum64 = COM_MemSize16DWAlign(pHead->tinyloader_size) + HEADER_SIZE + COM_MemSize16DWAlign(pHead->bootloader_size)
                                    + COM_MemSize16DWAlign(pHead->mcu0fw_size) + COM_MemSize16DWAlign(pHead->mcu1fw_size)
                                    + pHead->mcu2fw_size + 8;
    DBG_Printf(" L0_FwUpgradeCalcImgLen: sum64: 0x%x\n", sum64);

    // replace the buffer length with real image length
    if ((sum64 > 0)||(sum64 <= g_tFwUpdate.ulFwSize))
    {
#if defined(HOST_SATA)
        if(sum64%SEC_SIZE)
        {
            sum64 = (sum64/SEC_SIZE+1)*SEC_SIZE;
        }
#elif defined(HOST_NVME)
        if(sum64%DWORD_SIZE)
        {
            sum64 = (sum64/DWORD_SIZE+1)*DWORD_SIZE;
        }
#endif

        g_tFwUpdate.ulFwSize = sum64;
        //DBG_Printf("IMG TOTAL LENGTH is %d Byte\n",g_tFwUpdate.ulFwSize);
    }

    return;
}

MCU0_DRAM_TEXT BOOL L0_FWCheckCRC32(U32 ulOffsetInDW, U32 ulCrc32, U32 ulLenInDW)
{
    U32    *pImage;
    U32     p, ulCheckSum;

    pImage = (U32*)((U32*)g_tFwUpdate.ulFwBaseAddr + ulOffsetInDW);

    for(p=0,ulCheckSum=0; p<ulLenInDW; p++)
    {
        ulCheckSum ^= *(pImage+p);
    }

    if(ulCheckSum != ulCrc32)
    {
        DBG_Printf("InputCRC32: %x  CalCRC32: %x.\r\n", ulCrc32, ulCheckSum);
        return FALSE;
    }
    return TRUE;
}

MCU0_DRAM_TEXT BOOL L0_FwUpgradeCheckImgValid(void) 
{
    U8       *pImage = (U8*)(g_tFwUpdate.ulFwBaseAddr + 0x3C00);
    IMAGE_HDR *pHead = (IMAGE_HDR *)(pImage);   
    U32        ulOffInDW, ulLenInDW;

    //check tiny loader
    ulOffInDW = 0;
    ulLenInDW = pHead->tinyloader_size/sizeof(U32);
    if(!L0_FWCheckCRC32(ulOffInDW, pHead->tinyloader_crc32, ulLenInDW))
    {
        DBG_Printf("Tinyloader check fail: offsetInDW: 0x%x, LenInDw: 0x%x.\r\n", ulOffInDW, ulLenInDW);
        return FALSE;
    }

    //check bootloader
    ulOffInDW += (COM_MemSize16DWAlign(pHead->tinyloader_size) + HEADER_SIZE)/sizeof(U32);
    ulLenInDW = pHead->bootloader_size/sizeof(U32);
    if(!L0_FWCheckCRC32(ulOffInDW, pHead->bootloader_crc32, ulLenInDW))
    {
        DBG_Printf("bootloader check fail: offsetInDW: 0x%x, LenInDw: 0x%x.\r\n", ulOffInDW, ulLenInDW);
        return FALSE;
    }

    //check mcu0
    ulOffInDW += COM_MemSize16DWAlign(pHead->bootloader_size)/sizeof(U32);
    ulLenInDW = pHead->mcu0fw_size/sizeof(U32);
    if(!L0_FWCheckCRC32(ulOffInDW, pHead->mcu0fw_crc32, ulLenInDW))
    {
        DBG_Printf("mcu0 check fail: offsetInDW: 0x%x, LenInDw: 0x%x.\r\n", ulOffInDW, ulLenInDW);
        return FALSE;
    }

    //check mcu1
    ulOffInDW += COM_MemSize16DWAlign(pHead->mcu0fw_size)/sizeof(U32);
    ulLenInDW = pHead->mcu1fw_size/sizeof(U32);
    if(!L0_FWCheckCRC32(ulOffInDW, pHead->mcu1fw_crc32, ulLenInDW))
    {
        DBG_Printf("mcu1 check fail: offsetInDW: 0x%x, LenInDw: 0x%x.\r\n", ulOffInDW, ulLenInDW);
        return FALSE;
    }

    //check mcu2
    ulOffInDW += COM_MemSize16DWAlign(pHead->mcu1fw_size)/sizeof(U32);
    ulLenInDW = pHead->mcu2fw_size/sizeof(U32);
    if(!L0_FWCheckCRC32(ulOffInDW, pHead->mcu2fw_crc32, ulLenInDW))
    {
        DBG_Printf("mcu2 check fail: offsetInDW: 0x%x, LenInDw: 0x%x.\r\n", ulOffInDW, ulLenInDW);
        return FALSE;
    }

    return TRUE;
}


MCU0_DRAM_TEXT  void L0_UpgradeBootloader(U32 ulFwSlot)
{
    pUpgradeBL pUpgradeBlFunc = (pUpgradeBL)HAL_GetFTableFuncAddr(UPGRADE_BL);
    /*pUpgradeBlFunc  (g_tFwUpdate.aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_BL_IMG].ulPayloadAddrInDram
                    ,DRAM_DATA_BUFF_MCU1_BASE
                    ,DRAM_DATA_BUFF_MCU2_BASE);*/
}

MCU0_DRAM_TEXT  void L0_UpgradeSpiRom(U32 ulFwSlot)
{
#ifndef SIM
/*    HAL_SpiDmaWrite (SPI_START_ADDRESS
                    ,g_tFwUpdate.aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_SPI_IMG].ulPayloadAddrInDram
                    ,g_tFwUpdate.aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_SPI_IMG].ulPayloadLenByte); */
#else
    /*COM_MemCpy((U32 *)SPI_START_ADDRESS
              ,(U32 *)g_tFwUpdate.aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_SPI_IMG].ulPayloadAddrInDram
              ,g_tFwUpdate.aPayloadInfo[FW_UPGRADE_PAYLOAD_TYPE_SPI_IMG].ulPayloadLenByte>>2);*/
#endif
}


MCU0_DRAM_TEXT void L0_FillFwRuntimeInfo(FW_RUNTIME_INFO *pFwInfo)
{
    if (NULL == pFwInfo)
    {
        return;
    }

    COM_MemZero((U32*)pFwInfo, sizeof(FW_RUNTIME_INFO)/sizeof(U32));

    COM_MemCpy((U32*)&pFwInfo->FWVersionInfo, (U32*)g_pFwVersionInfo, sizeof(FW_VERSION_INFO)/sizeof(U32));
    COM_MemCpy((U32*)&pFwInfo->BLVersionInfo, (U32*)&g_pBootParamTable->tBLVersion, sizeof(FW_VERSION_INFO)/sizeof(U32));

    pFwInfo->ucPlnNum = g_pBootParamTable->ulSubSysPlnNum;
    //pFwInfo->ucLunNum = ?
    pFwInfo->usBlockPerCE = g_pBootParamTable->ulSubSysBlkNum;
    pFwInfo->usPagePerBlock = g_pBootParamTable->ulSubSysFWPageNum;
    pFwInfo->usPhyPageSize = g_pBootParamTable->ulFlashPageSize;
    pFwInfo->ulFlashId[0] = g_pBootParamTable->aFlashChipId[0];
    pFwInfo->ulFlashId[1] = g_pBootParamTable->aFlashChipId[1];
    pFwInfo->ulPhyCeMap[0] = g_pBootParamTable->ulSubSysCEMap[0];
    pFwInfo->ulPhyCeMap[1] = g_pBootParamTable->ulSubSysCEMap[1];
    pFwInfo->ucCeCount = g_pBootParamTable->ulSubSysCeNum * g_pBootParamTable->ulSubSysNum;
    pFwInfo->ucMcuCount = g_pBootParamTable->ulSubSysNum + 1;
    //pFwInfo->usPad = ?
    pFwInfo->ulDRAMSize = g_ulDramTotalSize;
    //pFwInfo->ulMcuConfigBytes[?] = ?
    
}



