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
Filename     : L1_Schedule.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20120118     BlakeZhang     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "HAL_TraceLog.h"
#include "FW_SMSG.h"
#include "L1_Inc.h"
#include "COM_Memory.h"

#include "HAL_Xtensa.h"
#ifndef SIM
#include <xtensa/tie/xt_timer.h>
#endif

#if defined(SWL_EVALUATOR)
#include "L2_Evaluater.h"
#endif

#define TL_FILE_NUM L1_Schedule_c

GLOBAL U32 g_ulHostWriteSec = 0;
extern GLOBAL MCU1_DRAM_DATA BOOL g_bSubSystemIdle;
extern GLOBAL PSCMD g_pCurSCmd;
extern GLOBAL  U32 g_ulAllowHostWriteSec;
extern GLOBAL  U32 g_RebuildDCPercent;

extern BOOL L2_IsBootupOK(void);
extern void L1_HostIOInit(void);
extern void MCU1_DRAM_TEXT L1_SharedMemMap(SUBSYSTEM_MEM_BASE *pFreeMemBase);

/****************************************************************************
Name        :L1_TaskSchedule
Input       :
Output      :
Author      :HenryLuo
Date        :2012.02.20    18:46:30
Description :L1 task schedule.
Others      :
Modify      :
****************************************************************************/
BOOL L1_TaskSchedule(void)
{   
    BOOL bRet;
	U32  ulHostWritePercent;

    TL_PERFORMANCE(PTL_LEVEL_TOP, "L1_TaskSchedule start ");

    bRet = FALSE;
  
    if(COMM_EVENT_STATUS_BLOCKING == L1_TaskEventHandle())
    {
        return bRet;
    }


    if ((NULL == g_pCurSCmd) && (NULL == g_pCurSubCmd))
    {
#ifdef HOST_READ_FROM_DRAM
        if ((FALSE == L1_RFDBufferManagment()) && (0 == g_ulRFDFullBitmap))
#endif
        {
            g_pCurSCmd = L1_GetSCmd();

            if (NULL != g_pCurSCmd)
            {
#if defined(SWL_EVALUATOR)
                if (g_pCurSCmd->tMA.ucOpType == DM_WRITE)
                {
                    SWLRecordIncHostWCnt_L1(g_pCurSCmd->tMA.ucSecLen);
                }
#endif
                if ((g_pCurSCmd->tMA.ucOpType == DM_WRITE) && (FALSE == L2_IsBootupOK()))
                {
                    g_ulHostWriteSec += g_pCurSCmd->tMA.ucSecLen;

                #ifdef SIM
                    if(g_pCurSCmd->tMA.ucLast == TRUE)
                    {
                        //FIRMWARE_LogInfo("\t SecLen %d g_ulHostWriteSec %d\n", g_pCurSCmd->tMA.ucSecLen,g_ulHostWriteSec);
                    }
                #endif
                }
                if (((U8)SCMD_DIRECT_MEDIA_ACCESS == g_pCurSCmd->ucSCmdType) 
                    || ((U8)SCMD_RAW_DATA_REQ == g_pCurSCmd->ucSCmdType))
                {
                    g_pMCU12MiscInfo->bSubSystemIdle = FALSE;
                    g_pCurSubCmd = L1_GetNewSubCmd(g_pCurSCmd);
                }
            }
            else
            {
                if((g_ulReadBufReqPuBitMap != 0) || (g_ulWriteBufReqPuBitMap != 0))
                {
                    return FALSE;
                }
            }
        }
    }

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 get scmd ");
    
    if ((NULL != g_pCurSCmd) && (((U8)SCMD_DIRECT_MEDIA_ACCESS != g_pCurSCmd->ucSCmdType) 
                && ((U8)SCMD_RAW_DATA_REQ != g_pCurSCmd->ucSCmdType)))
    {
        /* process special SCMD */
        L1_TaskSpecialSCMD(g_pCurSCmd);
    }

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 proc special scmd ");
    
    L1_BufferManagement();

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 Buffer management ");

    if (NULL != g_pCurSubCmd)
    {
#if 1
        if ((FALSE == g_L1PopCmdEn) && (DM_WRITE == g_pCurSCmd->tMA.ucOpType))
        {		
            if (g_ulHostWriteSec > L1_NONBLOCKING_BUFFER_SEC)
            {
                ulHostWritePercent = 1000 * (g_ulHostWriteSec - L1_NONBLOCKING_BUFFER_SEC) / (g_ulAllowHostWriteSec - L1_NONBLOCKING_BUFFER_SEC);
                //FIRMWARE_LogInfo("g_ulHostWriteSec %d  g_ulAllowHostWriteSec %d ulHostWritePercent %d g_RebuildDCPercent %d\n",
                //    g_ulHostWriteSec, g_ulAllowHostWriteSec, ulHostWritePercent, g_RebuildDCPercent);

                if(ulHostWritePercent >= g_RebuildDCPercent)
                {
                    return FALSE;				
                }
            }
        }
#endif
        L1_TaskResourceCheck(g_pCurSubCmd);

        L1_TaskCacheSearch(g_pCurSubCmd);
        
        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 Cache search ");
        
        L1_TaskCacheManagement(g_pCurSubCmd);
        
        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 cache management ");
        
        if (TRUE == L1_TaskHostIO(g_pCurSubCmd))
        {
            bRet = TRUE;
        }

        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 Host IO ");

        L1_TaskRecycle(g_pCurSubCmd);

        TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 Task Recycle ");
    }
    
    if (TRUE == L1_TaskMergeFlushManagement())
    {
        bRet = TRUE;
    }

    TL_PERFORMANCE(PTL_LEVEL_SCHEDULE,"L1 Merge Flush ");

    L1_TaskInternalIdle();

    TL_PERFORMANCE(PTL_LEVEL_TOP, "L1_TaskSchedule end  ");
    return bRet;
}

/****************************************************************************
Name        :L1_TaskEventHandle
Input       :
Output      :
Author      :Peter Xiu
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
GLOBAL BOOL g_ulReadPrefetchEnable;
U32 L1_TaskEventHandle(void)
{
    COMMON_EVENT L1_Event;
    COMM_EVENT_PARAMETER * pParameter;

    U32 Ret;

    Ret = COMM_EVENT_STATUS_BLOCKING;

    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == CommCheckEvent(COMM_EVENT_OWNER_L1,&L1_Event))
    {
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    if(L1_Event.EventDbg)
    {
        /* switch to debug mode, and dead loop in debug mode, don't need to clear L1 debug event */
        L1_RamdiskSwitchToSpecialMode();

        while (L1_RamdiskIsEnable())
        {
            L1_RamdiskSchedule();
        }

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else if (L1_Event.EventErrorHandling)
    {
        SMSG tSMSG;
      
        /* L1 warm Init */
        if (TRUE == L1_TaskErrorHandle())
        {
            /* clear Errorhandle Event */
            CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
            pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
            CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_ERR);
            
            /* report SMSG to L0 */
            tSMSG.ulMsgCode = SMSG_SCQ_CLEARED;
            
            FW_ReportSMSG(&tSMSG); 
        }

        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;

    }
    else if (L1_Event.EventShutDown)
    {
        if (FALSE == L1_TaskNormalPCCheckBusy())
        {
            //get parameter from comm event
            CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
            pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
            
            //clear event,L1 shutdown finished.
            CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_SHUTDOWN);
        }
        
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else if (L1_Event.EventBootAfterLoadRT)
    {
#ifndef LCT_VALID_REMOVED
        if (0 == L1_GetLCTValidLoadStatus())
        {
            /* blocking, waiting for LCTValid load finish */
            Ret = COMM_EVENT_STATUS_BLOCKING;
        }
        else if (1 == L1_GetLCTValidLoadStatus())
        {
            /* LCTValid load success, clear event */
#endif
            CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
            pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
            
            CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_BOOT_AFTER_RT);

            Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
#ifndef LCT_VALID_REMOVED
        }
        else
        {
            /* LCTValid load fail - rebuild LCT valid then clear event */
            L1_RebuildLCTTag();
            
            CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
            pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
            
            CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_BOOT_AFTER_RT);

            Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
#endif
    }
    else
    {

    }

    if (L1_Event.EventLowQD)
    {
        CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
        if (pParameter->EventStatus == TRUE)
            g_ulReadPrefetchEnable = TRUE;
        else
            g_ulReadPrefetchEnable = FALSE;

        CommClearEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_LOWQD);
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
	}

    return Ret;
}

void MCU1_DRAM_TEXT L1_Sram0Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;
    
    ulFreeSramBase = *pFreeSramBase;

    L1_BufferSram0Map( &ulFreeSramBase );

    L1_HostCMDProcSram0Map( &ulFreeSramBase );

    L1_BufReqFifoSram0Map( &ulFreeSramBase );

    L1_CacheStatusSram0Map( &ulFreeSramBase );

    DBG_Printf("#[L1] allocated %d KB DSRAM0\r\n", ( ulFreeSramBase - *pFreeSramBase )/1024 );

    *pFreeSramBase = ulFreeSramBase;

    return;
}

void MCU1_DRAM_TEXT L1_Sram1Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;
    
    ulFreeSramBase = *pFreeSramBase;

    L1_BufferSram1Map( &ulFreeSramBase );

    L1_CacheSram1Map( &ulFreeSramBase );

    L1_CacheStatusSram1Map( &ulFreeSramBase );

    DBG_Printf("#[L1] allocated %d KB DSRAM1\r\n", ( ulFreeSramBase - *pFreeSramBase )/1024 );

    *pFreeSramBase = ulFreeSramBase;

    return;
}

/****************************************************************************
Function  : L1_DramMap
Input     : Free Dram Base
Output    : None
Return    : None 
Purpose   : Dram mapping for L1 
Modify    :
2014.2.17  kristinwang  creat
****************************************************************************/
void MCU1_DRAM_TEXT L1_DramMap(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;
    
    ulFreeDramBase = *pFreeDramBase;

    /* allocate dram for buffer */
    L1_BufferDramMap( &ulFreeDramBase );

    /* allocate dram for L1 Debug Ramdisk */
    L1_RamdiskDramMap(&ulFreeDramBase);

    /*allocate dram for g_pLCT, g_pCacheLink*/
    L1_CacheDramMap( &ulFreeDramBase );

    /* allocate dram for global info. */
    L1_GlobalInfoDramMap( &ulFreeDramBase );

    /* allocate dram for g_pCSManager */
    L1_CacheStatusDRAMMap( &ulFreeDramBase );
    
    DBG_Printf("#[L1] MCU allocated %d MB (%d KB) DRAM\r\n", (ulFreeDramBase - *pFreeDramBase) / 1024 / 1024, (ulFreeDramBase - *pFreeDramBase) / 1024);

    *pFreeDramBase = ulFreeDramBase;

    return ;
}

void MCU1_DRAM_TEXT L1_OtfbMap(U32 *pFreeOtfbBase)
{
    /*allocate OTFB for CacheStatus*/
    L1_CacheStatusOTFBMap();

    /*allocate OTFB for Ramdisk*/
    L1_RamdiskOTFBMap(pFreeOtfbBase);

    return;
}

void MCU1_DRAM_TEXT L1_TaskInit(void)
{
    L1_Sram0Map( &g_FreeMemBase.ulFreeSRAM0Base );
    L1_Sram1Map( &g_FreeMemBase.ulFreeSRAM1Base );
    L1_DramMap( &g_FreeMemBase.ulDRAMBase );
    L1_OtfbMap( &g_FreeMemBase.ulFreeOTFBBase );

    L1_SharedMemMap(&g_FreeMemBase);

    L1_DbgInit();
    L1_HostIOInit();
    L1_HostCMDProcInit();

    L1_InitSCQ();
    L1_SpecialSCMDInit();
    L1_GlobalInfoInit();

    L1_BufferInit();
    L1_CacheStatusInit();
    L1_CacheInit();
    L1_BufReqInit();
    L1_RdPreFetchInit();
    L1_RamdiskInit();

#ifdef HOST_SATA
    L1_InitCacheLockedFlag();
#endif

#ifdef SUBSYSTEM_BYPASS_LLF_BOOT
    L1_SetDefaultDeviceParam();
#endif

    return;
}

/********************** FILE END ***************/

