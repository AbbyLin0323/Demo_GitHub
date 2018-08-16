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
Filename     : L1_SpecialSCMD.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20120118     BlakeZhang     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "FW_ViaCmd.h"
#include "L2_FCMDQ.h"
#ifdef DATA_MONITOR_ENABLE
#include "FW_DataMonitor.h"
#endif


GLOBAL  L1_SCMD_LLF_STATUS g_L1SCMDLLFStatus;
GLOBAL  L1_SCMD_BOOT_STATUS g_L1SCMDBootStatus;
GLOBAL  L1_SCMD_SHUTDOWN_STATUS g_L1SCMDShutdownStatus;
GLOBAL  L1_SCMD_IDLE_STATUS g_L1SCMDIdleStatus;
GLOBAL  L1_SCMD_SELFTEST_STATUS g_L1SCMDSelfTestStatus;
GLOBAL  L1_SCMD_HINFO_SAVE_STATUS g_L1SCMDHInfoSaveStatus;

#ifndef LCT_VALID_REMOVED
GLOBAL  U8 g_ucLCTValidLoadStatus; /* 0: waiting for load; 1: load success; 2: load fail */
#endif

extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
extern GLOBAL BOOL g_bL1BackgroundTaskEnable;
extern BOOL L2_IsBootupOK(void);
extern BOOL L2_FtlBuildIdleReq(U16 usIdleType);

#ifdef SIM
extern void Dbg_Record_TableInfo();
#else
#include <xtensa/tie/xt_interrupt.h>
#endif

extern U32 HAL_GetMcuId();
extern void FW_DbgShowAll(void);
extern void L1_HostIOWarmInit(void);
extern void FW_DbgUpdateStaticInfo(void);

void MCU1_DRAM_TEXT L1_SpecialSCMDInit(void)
{
    g_L1SCMDLLFStatus  = L1_SCMD_LLF_INIT;
    g_L1SCMDBootStatus = L1_SCMD_BOOT_INIT;
    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_INIT;
    g_L1SCMDIdleStatus = L1_SCMD_IDLE_INIT;
    g_L1SCMDSelfTestStatus = L1_SCMD_SELFTEST_INIT;
    g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_INIT;

    return;
}

#ifndef LCT_VALID_REMOVED
void MCU1_DRAM_TEXT L1_SetLCTValidLoadStatus(U8 ucStatus)
{
    g_ucLCTValidLoadStatus = ucStatus;
}

U8 MCU1_DRAM_TEXT L1_GetLCTValidLoadStatus(void)
{
    return g_ucLCTValidLoadStatus;
}
#endif

void MCU1_DRAM_TEXT L1_SpecialSCMDLLF(SCMD* pSCMD)
{
    COMMON_EVENT Event;
    COMM_EVENT_PARAMETER *pParameter;

    switch (g_L1SCMDLLFStatus)
    {
        case L1_SCMD_LLF_INIT:
        {
#ifndef SUBSYSTEM_BYPASS_LLF_BOOT
            g_L1SCMDLLFStatus = L1_SCMD_LLF_WAIT_L1;
#else
            g_L1SCMDLLFStatus = L1_SCMD_LLF_DONE;
#endif
            DBG_Printf("MCU%d:SCMD LLF received!\n",HAL_GetMcuId());
        }
        break;

        case L1_SCMD_LLF_WAIT_L1:
        {
            /* L1 LLF */
            L1_SaveHostInfo(pSCMD->tSystemCtrl.ulHInfoBuffAddr);
            
            if (TRUE == pSCMD->tSystemCtrl.ulParameter)
            {
                L1_SetDefaultDeviceParam();
                L1_SetDefaultHostInfo();
            }
            else
            {
                L1_SecurityEraseSetDeviceParam();
            }

#ifndef LCT_TRIM_REMOVED
            L1_CacheResetLCTTag();
#endif

            g_L1SCMDLLFStatus = L1_SCMD_LLF_SET_L2;
        }
        break;
        
        case L1_SCMD_LLF_SET_L2:
        {
            /* set L2 LLF event */
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                /* get parameter from comm event */
                CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);

                /* set keep erase cnt flag*/
                pParameter->EventParameterNormal[0] = TRUE;

                /* set security erase flag*/
                pParameter->EventParameterNormal[1] = FALSE;

                /* set L2 LLF event */
                CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_LLF);
                
                g_L1SCMDLLFStatus = L1_SCMD_LLF_WAIT_L2;
            }
        }
        break;

        case L1_SCMD_LLF_WAIT_L2:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L2,&Event);
            
            if (FALSE == Event.EventLLF)
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
            
                if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == pParameter->EventStatus)
                {
                    g_L1SCMDLLFStatus = L1_SCMD_LLF_DONE;
                }
                else
                {
                    g_L1SCMDLLFStatus = L1_SCMD_LLF_FAIL;
                }
            }
        }
        break;

        case L1_SCMD_LLF_FAIL:
        {
            /* LLF fail */
            g_L1SCMDLLFStatus = L1_SCMD_LLF_INIT;
            DBG_Printf("MCU%d:SCMD LLF fail!\n",HAL_GetMcuId());
            L1_SCmdFail();
        }
        break;

        case L1_SCMD_LLF_DONE:
        {
            /* LLF done */
            g_L1SCMDLLFStatus = L1_SCMD_LLF_INIT;
            DBG_Printf("MCU%d:SCMD LLF done!\n",HAL_GetMcuId());
            L1_SCmdFinish();
        }
        break;

        default:
        {
            DBG_Printf("MCU%d:L1_SpecialSCMDLLF g_L1SCMDLLFStatus 0x%x ERROR!\n", HAL_GetMcuId(), g_L1SCMDLLFStatus);
            DBG_Getch();
        }
        break;
    }

    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDBoot(SCMD* pSCMD)
{
    U32 ulMcuId;
    COMMON_EVENT Event;
    COMM_EVENT_PARAMETER *pParameter;

    ulMcuId = HAL_GetMcuId();
    switch (g_L1SCMDBootStatus)
    {
        case L1_SCMD_BOOT_INIT:
        {
            if ((U32)COLD_START == pSCMD->tSystemCtrl.ulParameter)
            {
#ifndef SUBSYSTEM_BYPASS_LLF_BOOT
                //g_L1SCMDBootStatus = L1_SCMD_BOOT_SET_L3;
                g_L1SCMDBootStatus = L1_SCMD_BOOT_SET_L2;
#ifndef LCT_VALID_REMOVED
                L1_SetLCTValidLoadStatus(0);
#endif
#else
                g_L1SCMDBootStatus = L1_SCMD_BOOT_DONE;
#ifndef LCT_VALID_REMOVED
                L1_SetLCTValidLoadStatus(1);
#endif
#endif

                DBG_Printf("MCU%d SCMD BOOT received!\n", ulMcuId);
            }
            else
            {
                g_L1SCMDBootStatus = L1_SCMD_BOOT_DONE;
            }
        }
        break;
        
        case L1_SCMD_BOOT_SET_L2:
        {
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                /* set L2 Boot event */
                CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_BOOT);
                
                g_L1SCMDBootStatus = L1_SCMD_BOOT_WAIT_L2;
            }
        }
        break;

        case L1_SCMD_BOOT_WAIT_L2:
        {
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);

                //Load RT Success    
                if(COMM_EVENT_STATUS_FAIL != pParameter->EventStatus)
                {
                    g_L1SCMDBootStatus = L1_SCMD_BOOT_WAIT_L1;

                    //Boot Stage 2: set EventBootAfterLoadRT
                    CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_BOOT_AFTER_RT);
                }
                else if(COMM_EVENT_STATUS_FAIL == pParameter->EventStatus)
                {
                    g_L1SCMDBootStatus = L1_SCMD_BOOT_REBUILD_WAIT_L1; 

                    //set table rebuld event
                    CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_REBUILD);   

                    //DBG_Printf("L1_SpecialSCMD - Set COMM_EVENT_OFFSET_REBUILD\n");
                }
                else
                {
                    DBG_Getch(); 
                }
            }
        }
        break;

        case L1_SCMD_BOOT_WAIT_L1:
        {
            /* L1 Boot */
            g_L1SCMDBootStatus = L1_SCMD_BOOT_DONE;          
        }
        break;

        case L1_SCMD_BOOT_REBUILD_WAIT_L1:
        {
#ifndef LCT_VALID_REMOVED
            /* L1 Rebuild */
            L1_RebuildLCTTag();
#endif

            if (TRUE != L1_IsDeviceParamValid())
            {
                L1_SetDefaultDeviceParam();
            }

            if (TRUE != L1_IsHostInfoValid())
            {
                L1_SaveHostInfo(pSCMD->tSystemCtrl.ulHInfoBuffAddr);
            }

            g_L1SCMDBootStatus = L1_SCMD_BOOT_DONE;
        }
        break;

        case L1_SCMD_BOOT_FAIL:
        {
            /* Boot rebuild finished */
            if ((U32)COLD_START == pSCMD->tSystemCtrl.ulParameter)
            {
                L1_LoadHostInfo(pSCMD->tSystemCtrl.ulHInfoBuffAddr);
            }

            g_L1SCMDBootStatus = L1_SCMD_BOOT_INIT;
            DBG_Printf("SCMD BOOT rebuild done!\n");
            L1_SCmdFail();
        }
        break;

        case L1_SCMD_BOOT_DONE:
        {
            /* Boot done */
            if ((U32)COLD_START == pSCMD->tSystemCtrl.ulParameter)
            {
                L1_LoadHostInfo(pSCMD->tSystemCtrl.ulHInfoBuffAddr);
            }
            
            g_L1SCMDBootStatus = L1_SCMD_BOOT_INIT;
            g_bL1BackgroundTaskEnable = TRUE;
            g_pMCU12MiscInfo->bSubSystemIdle = FALSE;

#ifndef LCT_VALID_REMOVED
            /* waiting for LCTValid load done */
            CommSetEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_BOOT_AFTER_RT);
#endif

            DBG_Printf("MCU%d Load RT Success!\n", ulMcuId);
            L1_SCmdFinish();
        }
        break;

        default:
        {
            DBG_Printf("L1_SpecialSCMDBoot g_L1SCMDBootStatus 0x%x ERROR!\n", g_L1SCMDBootStatus);
            DBG_Getch();
        }
        break;
    }

    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDShutDown(SCMD* pSCMD)
{
    U32 ulMcuId;
    COMMON_EVENT Event;
    COMM_EVENT_PARAMETER *pParameter;

    ulMcuId = HAL_GetMcuId();
    switch (g_L1SCMDShutdownStatus)
    {
        case L1_SCMD_SHUTDOWN_INIT:
        {
            L1_RdPreFetchResetThs();
            L1_SaveHostInfo(pSCMD->tSystemCtrl.ulHInfoBuffAddr);
            g_bL1BackgroundTaskEnable = FALSE;
            gbGlobalInfoSaveFlag = FALSE;
            L1_ReleaseAllWriteCache();
            CommSetEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_SHUTDOWN);

            DBG_Printf("MCU%d SCMD Shutdown received!\n", (ulMcuId - 1));
            g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_WAIT_L1;
        }
            break;

        case L1_SCMD_SHUTDOWN_WAIT_L1:
        {
            /* L1 Shutdown */
            CommCheckEvent(COMM_EVENT_OWNER_L1,&Event);
            if (FALSE == Event.EventShutDown)
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);

                if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == pParameter->EventStatus)
                {
                    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_SET_L2;
                }
                else
                {
                    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_FAIL;
                }
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_SET_L2:
        {
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                /* set L2 shutdown event */
                CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_SHUTDOWN);
                g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_WAIT_L2;
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_WAIT_L2:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L2,&Event);
            
            if (0 == Event.EventShutDown)
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
            
                if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == pParameter->EventStatus)
                {
                    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_SET_L3;
                }
                else
                {
                    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_FAIL;
                }
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_SET_L3:
        {
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L3, &Event))
            {
                /* set L3 Shutdown event */
                CommSetEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_SHUTDOWN);
                g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_WAIT_L3;
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_WAIT_L3:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L3, &Event);
            if (0 == Event.EventShutDown)
            {
                g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_SET_L2_SAVETABLE;
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_SET_L2_SAVETABLE:
        {
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                /* set L2 shutdown save table event */
                CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_SAVETABLE);
                g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_WAIT_L2_SAVETABLE;
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_WAIT_L2_SAVETABLE:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L2,&Event);

            if (0 == Event.EventSaveTable)
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);

                if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == pParameter->EventStatus)
                {
                    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_DONE;
                }
                else
                {
                    g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_FAIL;
                }
            }
        }
        break;

        case L1_SCMD_SHUTDOWN_FAIL:
        {
            /* Boot fail */
            g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_INIT;
            DBG_Printf("SCMD Shutdown fail!\n");
            L1_SCmdFail();
        }
        break;

        case L1_SCMD_SHUTDOWN_DONE:
        {
            /* Boot done */
            g_bL1BackgroundTaskEnable = TRUE;
            gbGlobalInfoSaveFlag = FALSE;

            g_L1SCMDShutdownStatus = L1_SCMD_SHUTDOWN_INIT;

            /* clear DRAM for windows simulation ENV */
        #ifdef SIM  
            /*if normal boot fail,table rebuild path will check PBIT/PuInfo/PMTI/PMTManager 
            when define DBG_TABLE_REBUILD,so backup these structures */
            Dbg_Record_TableInfo();
        #endif

            DBG_Printf("MCU%d SCMD Shutdown done!\n", (ulMcuId - 1));
            L1_SCmdFinish();
        }
        break;

        default:
        {
            DBG_Printf("L1_SpecialSCMDShutdown g_L1SCMDShutdownStatus 0x%x ERROR!\n", g_L1SCMDShutdownStatus);
            DBG_Getch();
        }
        break;
    }

    return;
}

void L1_SpecialSCMDTrim(SCMD* pSCMD)
{
    static MCU12_VAR_ATTR L1_SCMD_TRIM_STATUS s_L1ScmdTrimStatus = L1_SCMD_TRIM_INIT;

    switch (s_L1ScmdTrimStatus)
    {
        case L1_SCMD_TRIM_INIT:
        {
            // in this stage we perform prerequisite tasks for trim operation

            // perform trim initialization 
            if(TRUE == L1_TrimProcessInit(pSCMD->tUnmap.ulSubSysLBA, (pSCMD->tUnmap.ulSubSysLBA + pSCMD->tUnmap.ulSecLen)))
            {
                // trim initialization is successful

                // reset prefetch
                L1_RdPreFetchResetThs();

                // disable L1 background tasks
                g_bL1BackgroundTaskEnable = FALSE;

                // release all active caches
                L1_ReleaseAllWriteCache();
                
                // advance to the next stage
                s_L1ScmdTrimStatus = L1_SCMD_TRIM_WAIT_IDLE;
            }
            else
            {
                // trim initialization is not successful, mark the trim subcommand as failure
                s_L1ScmdTrimStatus = L1_SCMD_TRIM_FAIL;
            }
        }
        break;

        case L1_SCMD_TRIM_WAIT_IDLE:
        {
            // this stage makes sure that all pending write buffers and pending buffer
            // requests are processed
            if(TRUE == L1_TrimPrerequisiteCheck())
            {
                // advance to the next stage
                s_L1ScmdTrimStatus = L1_SCMD_TRIM_PROCESS_NON4K_ALIGNED_TRIM_RANGE;
            }
        }
        break;

        case L1_SCMD_TRIM_PROCESS_NON4K_ALIGNED_TRIM_RANGE:
        {
            // this stage processes all non-4K aligned trim range
            if(TRUE == L1_TrimProcessNon4KAlignedRange())
            {
                // advance to the next stage
                s_L1ScmdTrimStatus = L1_SCMD_TRIM_PROCESS_4K_ALIGNED_TRIM_RANGE;
            }
        }
        break;

        case L1_SCMD_TRIM_PROCESS_4K_ALIGNED_TRIM_RANGE:
        {
            // this stage processes all 4K aligned trim range
            L1_TrimProcess4KAlignedRange();

            // advance to the next stage
            s_L1ScmdTrimStatus = L1_SCMD_TRIM_PROCESS_LCT_ALIGNED_TRIM_RANGE;
        }
        break;

        case L1_SCMD_TRIM_PROCESS_LCT_ALIGNED_TRIM_RANGE:
        {
#ifdef LCT_TRIM_REMOVED
            if (TRUE == L1_TrimProcessLctAlignedRange())
            {
                s_L1ScmdTrimStatus = L1_SCMD_TRIM_DONE;
            }
#else
            // this stage processes all LCT aligned trim range
            L1_TrimProcessLctAlignedRange();
            // advance to the next stage
            s_L1ScmdTrimStatus = L1_SCMD_TRIM_DONE;
#endif
        }
        break;

        case L1_SCMD_TRIM_FAIL:
        {
            // trim command fails

            // enable L1 background tasks
            g_bL1BackgroundTaskEnable   = TRUE;

            // reset the trim stage
            s_L1ScmdTrimStatus = L1_SCMD_TRIM_INIT;

            // report the trim subcommand as failure
            L1_SCmdFail();
        }
        break;

        case L1_SCMD_TRIM_DONE:
        {
            // trim command succeeds

#ifdef DATA_MONITOR_ENABLE
            FW_DataMonitorResetLBARange(pSCMD->tUnmap.ulSubSysLBA, (pSCMD->tUnmap.ulSubSysLBA + pSCMD->tUnmap.ulSecLen));
#endif
            // enable L1 background tasks
            g_bL1BackgroundTaskEnable   = TRUE;

            // reset the trim stage
            s_L1ScmdTrimStatus = L1_SCMD_TRIM_INIT;

            // report the trim subcommand as success
            L1_SCmdFinish();
        }
        break;

        default:
        {
            DBG_Printf("trim stage error %x\n", s_L1ScmdTrimStatus);
            DBG_Getch();
        }
    }
  
    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDHostInfo(SCMD* pSCMD)
{
    COMMON_EVENT Event;
  
    if ((U32)GLBINFO_LOAD == pSCMD->tSystemInfos.ulOpType)
    {
        L1_LoadHostInfo(pSCMD->tSystemInfos.ulBuffAddr);
        
        L1_SCmdFinish();
    }
    else
    {
        switch (g_L1SCMDHInfoSaveStatus)
        {
            case L1_SCMD_HINFO_SAVE_INIT:
            {
                L1_SaveHostInfo(pSCMD->tSystemInfos.ulBuffAddr);
                g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_SET_L2;
            }
            break;
                
            case L1_SCMD_HINFO_SAVE_SET_L2:
            {
                if (FALSE == L2_IsBootupOK())
                {
                    /* set L2 saveRT event force, don't check for other events */
                    CommForceSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVERT);
                    g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_WAIT_L2;
                }
                else
                {
                    if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
                    {
                        /* set L2 saveRT event */
                        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVERT);
                        g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_WAIT_L2;
                    }
                }
            }
            break;
        
            case L1_SCMD_HINFO_SAVE_WAIT_L2:
            {
                CommCheckEvent(COMM_EVENT_OWNER_L2, &Event);
              
                /* L2 saveRT event process finished */
                if (FALSE == Event.EventSaveRT)
                {
                    g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_DONE;
                }
            }
            break;
        
            case L1_SCMD_HINFO_SAVE_FAIL:
            {
                gbGlobalInfoSaveFlag    = FALSE;
                g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_INIT;
                L1_SCmdFail();
            }
            break;
        
            case L1_SCMD_HINFO_SAVE_DONE:
            {
                gbGlobalInfoSaveFlag    = FALSE;
                g_L1SCMDHInfoSaveStatus = L1_SCMD_HINFO_SAVE_INIT;
                L1_SCmdFinish();
            }
            break;
        
            default:
            {
                DBG_Printf("L1_SpecialSCMDHostInfo g_L1SCMDHInfoSaveStatus 0x%x ERROR!\n", g_L1SCMDHInfoSaveStatus);
                DBG_Getch();
            }
            break;
        }
    }
  
    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDDevParam(SCMD* pSCMD)
{
    if ((U32)GLBINFO_LOAD == pSCMD->tSystemInfos.ulOpType)
    {
        FW_DbgUpdateStaticInfo();
        COM_MemCpy((U32 *)pSCMD->tSystemInfos.ulBuffAddr, (U32 *)g_pSubSystemDevParamPage, 
          (pSCMD->tSystemInfos.ulByteLen >> DWORD_SIZE_BITS));
    }
    else
    {
        /* don't need to merge the device parameters in L0 buffer to SubSystem */
        gbGlobalInfoSaveFlag = TRUE;
    }

    L1_SCmdFinish();
  
    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDIdle(SCMD* pSCMD)
{
    COMMON_EVENT Event;
    COMM_EVENT_PARAMETER *pParameter;

    switch (g_L1SCMDIdleStatus)
    {
        case L1_SCMD_IDLE_INIT:
        {
            g_bL1BackgroundTaskEnable = TRUE;

            if ((U32)IDLE_ERRORRECOVERY == pSCMD->tIdle.ulPriority)
            {
                FW_DbgShowAll();
                DBG_Printf("MCU %d IDLE_ERRORRECOVERY start\n", HAL_GetMcuId());
            }
            
            L1_RdPreFetchResetThs();
            L1_ReleaseAllWriteCache();

            if ((U32)IDLE_NORMAL == pSCMD->tIdle.ulPriority)
            {
                if (TRUE != L2_IsBootupOK())
                {
                    /* abort normal idle SCMD before SubSystem bootup ok */
                    L1_SCmdAbort();
                }
                else
                {
                    g_L1SCMDIdleStatus = L1_SCMD_IDLE_DO_L1;
                }
            }
            else
            {
                g_L1SCMDIdleStatus = L1_SCMD_IDLE_WAIT_L1;
            }
        }
        break;
    
        case L1_SCMD_IDLE_DO_L1:
        {
            if (TRUE == L1_TaskSystemIdle())
            {
                g_L1SCMDIdleStatus = L1_SCMD_IDLE_FAIL;
            }
            else
            {
                g_L1SCMDIdleStatus = L1_SCMD_IDLE_WAIT_L1;
            }
        }
        break;

        case L1_SCMD_IDLE_WAIT_L1:
        {
            if (TRUE == L1_IsIdle())
            {
                g_L1SCMDIdleStatus = L1_SCMD_IDLE_SET_L2;
            }
            else
            {
                if ((U32)IDLE_NORMAL == pSCMD->tIdle.ulPriority)
                {
                    g_L1SCMDIdleStatus = L1_SCMD_IDLE_FAIL;
                }
            }
        }
        break;

        case L1_SCMD_IDLE_SET_L2:
        {
            if (TRUE != g_pMCU12MiscInfo->bSubSystemIdle)
            {
                /* set L2 idle event */
                if ((COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2, &Event))
                    || ((U32)IDLE_NORMAL != pSCMD->tIdle.ulPriority))
                {
                    CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
                    pParameter->EventParameterNormal[0] = pSCMD->tIdle.ulPriority;
                    CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_IDLE);

                    g_L1SCMDIdleStatus = L1_SCMD_IDLE_WAIT_L2;
                }
                else
                {
                    if ((U32)IDLE_NORMAL == pSCMD->tIdle.ulPriority)
                    {
                        g_L1SCMDIdleStatus = L1_SCMD_IDLE_FAIL;
                    }
                }
            }
            else
            {
                g_L1SCMDIdleStatus = L1_SCMD_IDLE_DONE;
            }
        }
        break;

        case L1_SCMD_IDLE_WAIT_L2:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L2, &Event);
            /* L2 idle event process finished */
            if (FALSE == Event.EventIdle)
            {
                if (TRUE != g_pMCU12MiscInfo->bSubSystemIdle)
                {
                    g_L1SCMDIdleStatus = L1_SCMD_IDLE_FAIL;
                }
                else
                {
                    g_L1SCMDIdleStatus = L1_SCMD_IDLE_DONE;
                }
            }
            else
            {
                if ((U32)IDLE_NORMAL == pSCMD->tIdle.ulPriority)
                {
                    g_L1SCMDIdleStatus = L1_SCMD_IDLE_FAIL;
                }
            }

        }
        break;

        case L1_SCMD_IDLE_FAIL:
        {
            g_L1SCMDIdleStatus = L1_SCMD_IDLE_INIT;
            L1_SCmdFail();
        }
        break;
    
        case L1_SCMD_IDLE_DONE:
        {
            if ((U32)IDLE_ERRORRECOVERY == pSCMD->tIdle.ulPriority)
            {
                L1_HostIOWarmInit();
                #ifdef VT3533_A2ECO_DSGRLSBUG
                HAL_NormalDsgFifoInit();
                #endif
                DBG_Printf("MCU %d IDLE_ERRORRECOVERY done\n", HAL_GetMcuId());
            }
            
            g_L1SCMDIdleStatus = L1_SCMD_IDLE_INIT;
            g_pMCU12MiscInfo->bSubSystemIdle = FALSE;
            L1_SCmdFinish();
#if defined(ASIC) || defined(XTMP)
      //     XT_WAITI(0);  //blakezhang: temp remove in all platforms, since there is a pending issue for it.
#endif
        }
        break;
    
        default:
        {
            DBG_Printf("L1_SpecialSCMDIdle g_L1SCMDIdleStatus 0x%x ERROR!\n", g_L1SCMDIdleStatus);
            DBG_Getch();
        }
        break;
    }
  
    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDSelfTest(SCMD* pSCMD)
{
    COMMON_EVENT Event;
  
    switch (g_L1SCMDSelfTestStatus)
    {
        case L1_SCMD_SELFTEST_INIT:
        {
            if (FALSE == L2_IsBootupOK())
            {
                /* directly finish selftest SCMD before SubSystem boot done */
                L1_SCmdFinish();
            }
            else
            {
                L1_DbgShowAll();
                g_pMCU12MiscInfo->bSubSystemSelfTestDone = FALSE;
                g_L1SCMDSelfTestStatus   = L1_SCMD_SELFTEST_DO_L1;
            }
        }
        break;
    
        case L1_SCMD_SELFTEST_DO_L1:
        {
            /* need add more selftest functions in L1 */
            g_L1SCMDSelfTestStatus = L1_SCMD_SELFTEST_SET_L2;
        }
        break;

        case L1_SCMD_SELFTEST_SET_L2:
        {
            /* set L2 selftest event */
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SELFTEST);
                g_L1SCMDSelfTestStatus = L1_SCMD_SELFTEST_WAIT_L2;
            }
        }
        break;

        case L1_SCMD_SELFTEST_WAIT_L2:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L2,&Event);
          
            /* L2 selftest event process finished */
            if (FALSE == Event.EventSelfTest)
            {
                if (TRUE == g_pMCU12MiscInfo->bSubSystemSelfTestDone)
                {
                    g_L1SCMDSelfTestStatus = L1_SCMD_SELFTEST_DONE;
                }
                else
                {
                    g_L1SCMDSelfTestStatus = L1_SCMD_SELFTEST_SET_L2;
                }
            }
        }
        break;

        case L1_SCMD_SELFTEST_FAIL:
        {
            g_pMCU12MiscInfo->bSubSystemSelfTestDone = FALSE;
            g_L1SCMDSelfTestStatus   = L1_SCMD_SELFTEST_INIT;
            L1_SCmdFail();
        }
        break;
    
        case L1_SCMD_SELFTEST_DONE:
        {
            g_pMCU12MiscInfo->bSubSystemSelfTestDone = FALSE;
            g_L1SCMDSelfTestStatus   = L1_SCMD_SELFTEST_INIT;
            L1_SCmdFinish();
        }
        break;
    
        default:
        {
            DBG_Printf("L1_SpecialSCMDSelfTest g_L1SCMDSelfTestStatus 0x%x ERROR!\n", g_L1SCMDSelfTestStatus);
            DBG_Getch();
        }
        break;
    }
  
    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDSecurityErase(SCMD* pSCMD)
{
    COMMON_EVENT Event;
    COMM_EVENT_PARAMETER *pParameter;

    switch (g_L1SCMDLLFStatus)
    {
        case L1_SCMD_LLF_INIT:
        {
#ifndef SUBSYSTEM_BYPASS_LLF_BOOT
            g_L1SCMDLLFStatus = L1_SCMD_LLF_WAIT_L1;
#else
            g_L1SCMDLLFStatus = L1_SCMD_LLF_DONE;
#endif
            DBG_Printf("MCU%d:SCMD SecurityErase received!\n",HAL_GetMcuId());

        }
        break;

        case L1_SCMD_LLF_WAIT_L1:
        {
            /* L1 LLF */
            L1_SaveHostInfo(pSCMD->tSystemCtrl.ulHInfoBuffAddr);
            
            if (TRUE == pSCMD->tSystemCtrl.ulParameter)
            {
                L1_SetDefaultDeviceParam();
                L1_SetDefaultHostInfo();
            }

            /* L1 data structure init */
#ifndef LCT_TRIM_REMOVED
            L1_CacheResetLCTTag();            
#endif
            L1_BufReqInit();
            L1_CacheInit();
            L1_BufferInit();
            L1_CacheStatusInit();

            g_L1SCMDLLFStatus = L1_SCMD_LLF_SET_L2;      
        }
        break;

        case L1_SCMD_LLF_SET_L2:
        {
            if (COMM_EVENT_STATUS_GET_EVENTPEND != CommCheckEvent(COMM_EVENT_OWNER_L2,&Event))
            {
                /* get parameter from comm event */
                CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
                
                /* set keep erase cnt flag*/
                pParameter->EventParameterNormal[0] = TRUE;
                
                /* set security erase flag*/
                pParameter->EventParameterNormal[1] = TRUE;
                
                /* set L2 LLF event */
                CommSetEvent(COMM_EVENT_OWNER_L2,COMM_EVENT_OFFSET_LLF);
                
                g_L1SCMDLLFStatus = L1_SCMD_LLF_WAIT_L2;
            }
        }
        break;

        case L1_SCMD_LLF_WAIT_L2:
        {
            CommCheckEvent(COMM_EVENT_OWNER_L2,&Event);
            
            if (FALSE == Event.EventLLF)
            {
                CommGetEventParameter(COMM_EVENT_OWNER_L2,&pParameter);
            
                if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == pParameter->EventStatus)
                {
                    g_L1SCMDLLFStatus = L1_SCMD_LLF_DONE;
                }
                else
                {
                    g_L1SCMDLLFStatus = L1_SCMD_LLF_FAIL;
                }
            }
        }
        break;

        case L1_SCMD_LLF_FAIL:
        {
            /* LLF fail */
            g_L1SCMDLLFStatus = L1_SCMD_LLF_INIT;
            DBG_Printf("MCU%d:SCMD SecurityErase fail!\n",HAL_GetMcuId());
            L1_SCmdFail();
        }
        break;

        case L1_SCMD_LLF_DONE:
        {
            /* LLF done */
            g_L1SCMDLLFStatus = L1_SCMD_LLF_INIT;
            DBG_Printf("MCU%d:SCMD SecurityErase done!\n",HAL_GetMcuId());
            L1_SCmdFinish();
        }
        break;

        default:
        {
            DBG_Printf("MCU%d:SecurityErase g_L1SCMDLLFStatus 0x%x ERROR!\n", HAL_GetMcuId(), g_L1SCMDLLFStatus);
            DBG_Getch();
        }
        break;
    }

    return;
}

void MCU1_DRAM_TEXT L1_SpecialSCMDFlush(SCMD* pSCMD)
{
    L1_ReleaseAllWriteCache();

    if (TRUE == L1_IsIdle())
    {
        L1_SCmdFinish();
    }

    return;
}

void L1_TaskSpecialSCMD(SCMD* pSCMD)
{
    switch (pSCMD->ucSCmdType)
    {
        case (U8)SCMD_LLF:
        {
            L1_SpecialSCMDLLF(pSCMD);
        }
        break;

        case (U8)SCMD_BOOTUP:
        {
            L1_SpecialSCMDBoot(pSCMD);
        }
        break;

        case (U8)SCMD_POWERCYCLING:
        {
            L1_SpecialSCMDShutDown(pSCMD);
        }
        break;

        case (U8)SCMD_UNMAP:
        {
            L1_SpecialSCMDTrim(pSCMD);
        }
        break;

        case (U8)SCMD_FLUSH:
        {
            L1_SpecialSCMDFlush(pSCMD);
        }
        break;

        case (U8)SCMD_ACCESS_HOSTINFO:
        {
            L1_SpecialSCMDHostInfo(pSCMD);
        }
        break;

        case (U8)SCMD_ACCESS_DEVPARAM:
        {
            L1_SpecialSCMDDevParam(pSCMD);
        }
        break;

        case (U8)SCMD_IDLETASK:
        {
            L1_SpecialSCMDIdle(pSCMD);
        }
        break;

        case (U8)SCMD_SELFTESTING:
        {
            L1_SpecialSCMDSelfTest(pSCMD);
        }
        break;

        case (U8)SCMD_SANITIZE:
        {
            L1_SpecialSCMDSecurityErase(pSCMD);
        }
        break;

        case (U8)SCMD_VIA_DEV_CTRL:
        {
            if (TRUE == FW_ViaCmdDevCtrl(pSCMD))
            {
                L1_SCmdFinish();
            }
            else
            {
                L1_SCmdFail();
            }
        }
        break;

        case (U8)SCMD_VIA_UART_RAW_DATA_REQ:
        {
            FW_ViaCmdPrepareData(pSCMD);

            L1_SCmdFinish();
        }
        break;
        
        default:
        {
            DBG_Printf("L1_TaskSpecialSCMD ucSCmdType 0x%x ERROR!\n", pSCMD->ucSCmdType);
            DBG_Getch();
        }
        break;
    }

    return;
}

