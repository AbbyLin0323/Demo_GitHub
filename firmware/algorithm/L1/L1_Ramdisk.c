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
Filename     : L1_Ramdisk.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20150302     BlakeZhang     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "HAL_TraceLog.h"
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "FW_SMSG.h"
#include "FW_BufAddr.h"
#include "FW_ViaCmd.h"
#include "L1_Inc.h"
#include "L2_Interface.h"

extern void L1_InitSubCmdHostInfo(SUBCMD* pSubCmd);
extern void L1_HostIOInit(void);
extern BOOL L1_HostIO(SUBCMD* pSubCmd);
extern BOOL L1_GetHostResource(SUBCMD* pSubCmd);
extern void L1_HostIOWarmInit(void);
extern void FW_DbgShowAll(void);
extern void FW_FlushSubsystemTrace(SCMD* pSCMD);
extern BOOL L2_FtlBuildIdleReq(U16 usIdleType);


GLOBAL  PSCMD   g_pL1RamdiskSCMD;
GLOBAL  SUBCMD  g_L1RamdiskSubCmd;
GLOBAL  SUBCMD *g_pL1RamdiskSubCmd;
GLOBAL  U32 g_ulL1RamdiskDramBase;
GLOBAL  L1_RAMDISK_MODE g_ulL1RamdiskMode;
GLOBAL  U8* g_pucRamdiskCacheStatus;

/****************************************************************************
Name        :L1_RamdiskDramMap
Input       :ulDramStartAddr
Output      :
Author      :HenryLuo
Date        :2012.03.01    17:41:13
Description :allcate dram for buffer module.
Others      :
Modify      :
2014.2.17 kristinwang
****************************************************************************/
void MCU1_DRAM_TEXT L1_RamdiskDramMap(U32 *pFreeDramBase)
{
#ifdef L1_FAKE
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    g_ulL1RamdiskDramBase = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, L1_DEBUG_RAMDISK_SIZE);

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    *pFreeDramBase = ulFreeDramBase;
#else
    if (HAL_IsRamDiskMode() == TRUE)
    {
        U32 ulFreeDramBase;

        ulFreeDramBase = *pFreeDramBase;
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

        g_ulL1RamdiskDramBase = ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, L1_DEBUG_RAMDISK_SIZE);

        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
        *pFreeDramBase = ulFreeDramBase;
    }
    else
    {
        /* L1 Ramdisk use DRAM reserved 256KB space */
        if (MCU1_ID == HAL_GetMcuId())
        {
            g_ulL1RamdiskDramBase = DRAM_RSVD_BASE;
        }
        else
        {
            g_ulL1RamdiskDramBase = DRAM_RSVD_BASE + L1_DEBUG_RAMDISK_SIZE;
        }
    }
#endif

    return;
}

/****************************************************************************
Name        :L1_RamdiskOTFBMap
Input       :
Output      :
Description :allcate otfb for buffer module.
Others      :
Modify      :
2014.2.17 kristinwang
****************************************************************************/
void MCU1_DRAM_TEXT L1_RamdiskOTFBMap(U32 *pFreeOTFBBase)
{
    U32 ulCacheStatusOTFBBase;
    
    ulCacheStatusOTFBBase = g_FreeMemBase.ulFreeCacheStatusBase;

    COM_MemAddr16DWAlign(&ulCacheStatusOTFBBase);

    g_pucRamdiskCacheStatus = (U8*)ulCacheStatusOTFBBase;
    COM_MemIncBaseAddr(&ulCacheStatusOTFBBase, LPN_PER_BUF);
    COM_MemAddr16DWAlign(&ulCacheStatusOTFBBase);

    g_FreeMemBase.ulFreeCacheStatusBase = ulCacheStatusOTFBBase;
    
    return;
}

/****************************************************************************
Name        :L1_RamdiskInit
Input       :void
Output      :void
Author      :Blakezhang
Date        :2015.03.02
Description :
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L1_RamdiskInit(void)
{
    BOOTLOADER_FILE *pBootLoader;
    
    pBootLoader = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
  
    g_pL1RamdiskSCMD   = NULL;
    g_pL1RamdiskSubCmd = NULL;

    if (FALSE == pBootLoader->tSysParameterTable.sBootStaticFlag.bsDebugMode)
    {
        if (BOOT_METHOD_MPT == pBootLoader->tSysParameterTable.sBootStaticFlag.bsBootMethodSel)
        {
            g_ulL1RamdiskMode = L1_RAMDISK_ENABLE_NORMAL;
        }
        else
        {
            g_ulL1RamdiskMode = L1_RAMDISK_DISABLE;
        }
    }
    else
    {
        g_ulL1RamdiskMode = L1_RAMDISK_ENABLE_NORMAL;
    }

#ifdef L1_FAKE
        g_ulL1RamdiskMode = L1_RAMDISK_ENABLE_NORMAL;
#else
        if (HAL_IsRamDiskMode() == TRUE)
            g_ulL1RamdiskMode = L1_RAMDISK_ENABLE_NORMAL;
#endif

    *g_pucRamdiskCacheStatus = SUBSYSTEM_STATUS_INIT;

    return;
}

/****************************************************************************
Function  : L1_RamdiskGetRawCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U8 L1_RamdiskGetRawCacheStatus(void)
{
    return (*g_pucRamdiskCacheStatus);
}

/****************************************************************************
Function  : L1_RamdiskGetRawCacheStatusAddr
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_RamdiskGetRawCacheStatusAddr(void)
{
    return ((U32)g_pucRamdiskCacheStatus);
}

/****************************************************************************
Function  : L1_RamdiskSetRawCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
void L1_RamdiskSetRawCacheStatus(U8 ucStatus)
{
    *g_pucRamdiskCacheStatus = ucStatus;

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSetMode(U32 ulMode)
{
    g_ulL1RamdiskMode = ulMode;
  
    return;
}

L1_RAMDISK_MODE MCU1_DRAM_TEXT L1_RamdiskGetMode(void)
{
    return g_ulL1RamdiskMode;
}

BOOL L1_RamdiskIsEnable(void)
{
    if (L1_RAMDISK_DISABLE == g_ulL1RamdiskMode)
    {
        return FALSE;
    }
  
    return TRUE;
}

U32 L1_RamdiskGetDramAddr(SCMD* pSCmd)
{
    U32 ulDramAddr;
    U32 ulSubSysLBA = pSCmd->tMA.ulSubSysLBA;
    U8  ucSecLen = pSCmd->tMA.ucSecLen;

    if (((ulSubSysLBA + ucSecLen -1) << SEC_SIZE_BITS) <= L1_DEBUG_RAMDISK_SIZE)
    {
        ulDramAddr = g_ulL1RamdiskDramBase + (ulSubSysLBA << SEC_SIZE_BITS) - DRAM_START_ADDRESS;
    }
    else
    {
        U32 ulNewSubSysLBA = ulSubSysLBA % L1_DEBUG_RAMDISK_MAX_LBA;
        DBG_Printf("L1_Ramdisk MEDIA_ACCESS SCMD Invalid Range: LBA[0x%x] Len[0x%x] Limit[0x%x]\n", ulSubSysLBA, ucSecLen, L1_DEBUG_RAMDISK_MAX_LBA);
        if (((ulNewSubSysLBA + ucSecLen - 1) << SEC_SIZE_BITS) <= L1_DEBUG_RAMDISK_SIZE)
        {
            ulDramAddr = g_ulL1RamdiskDramBase + (ulNewSubSysLBA << SEC_SIZE_BITS) - DRAM_START_ADDRESS;
        }
        else
        {
            DBG_Printf("L1_Ramdisk MEDIA_ACCESS SCMD Invalid Range AFTER MOD: OrgLBA[0x%x] NewLBA[0x%x] Len[0x%x] Limit[0x%x]\n", ulSubSysLBA, ulNewSubSysLBA, ucSecLen, L1_DEBUG_RAMDISK_MAX_LBA);
            ulDramAddr = g_ulL1RamdiskDramBase - DRAM_START_ADDRESS;
        }
    }  
  
    return ulDramAddr;
}

void MCU1_DRAM_TEXT L1_RamdiskReportGetchSMSG(void)
{
    SMSG tSMSG;
  
    /* report SMSG to L0 */
    tSMSG.ulMsgCode = SMSG_INFORM_DBG_GETCH;
    
    FW_ReportSMSG(&tSMSG); 

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskFinishGetchSMSG(void)
{
    SMSG tSMSG;
  
    /* report SMSG to L0 */
    tSMSG.ulMsgCode = SMSG_FINISH_DBG_GETCH;
    
    FW_ReportSMSG(&tSMSG); 

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSCmdFinish(void)
{
    L1_PopSCmdNode((U32)SSTS_SUCCESS);
    g_pL1RamdiskSCMD = NULL;

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSCmdFail(void)
{
    L1_PopSCmdNode((U32)SSTS_FAILED);
    g_pL1RamdiskSCMD = NULL;

    return;
}

PSCMD MCU1_DRAM_TEXT L1_RamdiskGetSCmd(void)
{
    PSCMD pSCmd;

    pSCmd = NULL;

    if (FALSE == L1_IsSCQEmpty())
    {
        pSCmd = L1_GetSCmdFromHead();
    }

    /* validation check of Media Access SCMD */
    // Don't check valid LBA in Ramdisk mode, mod the target DRAM address --> L1_RamdiskGetDramAddr()
    // Only check if size over Ramdisk size
    if ((NULL != pSCmd) && (SCMD_DIRECT_MEDIA_ACCESS == pSCmd->ucSCmdType))
    {
        if ((pSCmd->tMA.ucSecLen - 1) > L1_DEBUG_RAMDISK_MAX_LBA)
        {
            DBG_Printf("L1_Ramdisk MEDIA_ACCESS SCMD size over limit: LBA 0x%x Len 0x%x, limit 0x%x\n", pSCmd->tMA.ulSubSysLBA, pSCmd->tMA.ucSecLen, L1_DEBUG_RAMDISK_MAX_LBA);
            L1_RamdiskSCmdFail();
            pSCmd = NULL;
        }
    }

    if ((NULL != pSCmd) && (SCMD_VIA_UART_RAW_DATA_REQ == pSCmd->ucSCmdType) && (0 == pSCmd->ucSCmdSpecific))
    {
        DBG_Getch();
    }

    return pSCmd;
}

SUBCMD* MCU1_DRAM_TEXT L1_RamdiskFillSubCmd(SCMD* pSCMD)
{
    U8 ucDWOffset;
    U8 ucVirlSubCmdLen;
    SUBCMD *pSubCmd = NULL;

    pSubCmd = &g_L1RamdiskSubCmd;

    /*clear SubCmd*/
    for (ucDWOffset = 0; ucDWOffset < SUBCMD_SIZE_DW; ucDWOffset++)
    {
        *((U32*)(pSubCmd) + ucDWOffset) = 0;
    }

    pSubCmd->pSCMD = pSCMD;

    /*fill SubCmd Infos, note that Media Access SubCmd don't check CacheStatus */
    if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
    {
        pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN = pSCMD->tMA.ucSecLen;
        pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN = SUBCMD_OFFSET(pSCMD->tMA.ulSubSysLBA);
        pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN = SUBCMDLPN_OFFSET(pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN);

        ucVirlSubCmdLen = SUBCMD_OFFSET_IN_LPN(pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN) + pSCMD->tMA.ucSecLen;
        pSubCmd->SubCmdAddInfo.ucSubLPNCountIN  = SUBCMDLPN_COUNT(ucVirlSubCmdLen);

        pSubCmd->SubCmdHitResult = L1_CACHE_SE_FULL_HIT;
        pSubCmd->SubCmdStage     = SUBCMD_STAGE_SEARCH;
    }
    else
    {
        /* use normal mode Raw CacheStatus */
        L1_RamdiskSetRawCacheStatus(SUBSYSTEM_STATUS_PENDING);
        pSubCmd->CacheStatusAddr = L1_RamdiskGetRawCacheStatusAddr() - OTFB_START_ADDRESS;
        pSubCmd->SubCmdStage     = SUBCMD_STAGE_CACHE;

        /* Vendor define SCMD special process */
        if (VIA_CMD_NULL != pSCMD->ucSCmdSpecific)
        {
            FW_ViaCmdPrepareData(pSCMD);
        }
    }


    /* Init SubCmd Host Info after get new SubCmd */
    L1_InitSubCmdHostInfo(pSubCmd);

    return pSubCmd;
}

BOOL MCU1_DRAM_TEXT L1_RamdiskGetHostResource (SUBCMD* pSubCmd)
{
    if(pSubCmd == NULL)
    {
        /* no subcmd need to do */
        return TRUE;
    }

    if (pSubCmd->SubCmdStage != SUBCMD_STAGE_SEARCH)
    {
        /* subcmd already processed */
        return TRUE;
    }

    /* check if we have resource */    
    if (FALSE == L1_GetHostResource(pSubCmd))
    {
        return FALSE;
    }

    pSubCmd->SubCmdStage = SUBCMD_STAGE_CACHE;
    return TRUE;
}

BOOL MCU1_DRAM_TEXT L1_RamdiskHostIO(SUBCMD* pSubCmd)
{ 
    if(pSubCmd == NULL)
    {
        /*no subcmd or prd resource */
        return FALSE;
    }

    if (pSubCmd->SubCmdStage != SUBCMD_STAGE_CACHE)
    {
        /* wait for get host recourse done  */
        return FALSE;
    }

    if (TRUE != L1_HostIO(pSubCmd))
    {
        return FALSE;
    }
    else
    {
        pSubCmd->SubCmdStage = SUBCMD_STAGE_HOSTIO;
        return TRUE;
    }
}

void MCU1_DRAM_TEXT L1_RamdiskRecycle(SUBCMD* pSubCmd)
{
    if (pSubCmd != NULL && pSubCmd->SubCmdStage == SUBCMD_STAGE_HOSTIO)
    {
        if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
        {
            g_pL1RamdiskSubCmd = NULL;
            L1_RamdiskSCmdFinish();
        }
        else
        {
            if (SUBSYSTEM_STATUS_SUCCESS == L1_RamdiskGetRawCacheStatus())
            {
                L1_RamdiskSetRawCacheStatus(SUBSYSTEM_STATUS_INIT);

                if (VIA_CMD_NULL != pSubCmd->pSCMD->ucSCmdSpecific)
                {
                    FW_ViaCmdXferDataDone(pSubCmd->pSCMD);
                }
                g_pL1RamdiskSubCmd = NULL;
                L1_RamdiskSCmdFinish();
            }
        }
    }

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSpecialSCMDBoot(SCMD* pSCMD)
{
    HOST_INFO_PAGE *pHostInfo = (HOST_INFO_PAGE *)pSCMD->tSystemCtrl.ulHInfoBuffAddr;

    pHostInfo->HPAMaxLBA = L1_DEBUG_RAMDISK_MAX_LBA * DiskConfig_GetSubSysNum();
    pHostInfo->Check1    = GLOBAL_INFO_CHECK_1;
    pHostInfo->Check2    = GLOBAL_INFO_CHECK_2;

    L1_RamdiskSCmdFinish();

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSpecialSCMDHostInfo(SCMD* pSCMD)
{
    HOST_INFO_PAGE *pHostInfo = (HOST_INFO_PAGE *)pSCMD->tSystemInfos.ulBuffAddr;
  
    if ((U32)GLBINFO_LOAD == pSCMD->tSystemInfos.ulOpType)
    {
        pHostInfo->HPAMaxLBA = L1_DEBUG_RAMDISK_MAX_LBA * DiskConfig_GetSubSysNum();
        pHostInfo->Check1    = GLOBAL_INFO_CHECK_1;
        pHostInfo->Check2    = GLOBAL_INFO_CHECK_2;
    }

    L1_RamdiskSCmdFinish();

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSpecialSCMDDevParam(SCMD* pSCMD)
{
    if ((U32)GLBINFO_LOAD == pSCMD->tSystemInfos.ulOpType)
    {
        COM_MemZero((U32 *)pSCMD->tSystemInfos.ulBuffAddr, (pSCMD->tSystemInfos.ulByteLen/DWORD_SIZE));
    }

    L1_RamdiskSCmdFinish();
  
    return;
}
extern void L2_SetL3IdleEvent(U32 ulIdleParam);
extern void L2_WaitL3IdleEventDone();
void MCU1_DRAM_TEXT L1_RamdiskSpecialSCMDIdle(SCMD* pSCMD)
{
    if (IDLE_ERRORRECOVERY == pSCMD->tIdle.ulPriority)
    {
        L1_HostIOWarmInit();
    }
    
#ifndef L1_FAKE
    //patch NFC scramble seed race condition by MCU0&MUC2 while saving FW via MPT
    L2_SetL3IdleEvent(FCMD_REQ_SUBTYPE_IDLE_0);
    L2_WaitL3IdleEventDone();
#endif 

    L1_RamdiskSCmdFinish();

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSpecialSCMDTrim(SCMD* pSCMD)
{
    U32 ulStartLBA, ulSecLen;
    U32 ulDRAMStart, ulDRAMLen;

    /* 1. Getting trim range from subsystem command. */
    ulStartLBA = pSCMD->tUnmap.ulSubSysLBA;
    ulSecLen = pSCMD->tUnmap.ulSecLen;

    if (0 != ulSecLen)
    {
        /* 2. Calculating DRAM address from subsystem LBA range. */
        ulDRAMStart = g_ulL1RamdiskDramBase + (ulStartLBA << SEC_SIZE_BITS);
        ulDRAMLen = ulSecLen << SEC_SIZE_BITS;
    
        COM_MemSet((U32 *)ulDRAMStart, ulDRAMLen/sizeof(U32), INVALID_8F);
    }

    L1_RamdiskSCmdFinish();

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSpecialSCMD(SCMD* pSCMD)
{
    switch (pSCMD->ucSCmdType)
    {
        case (U8)SCMD_BOOTUP:
        {
            L1_RamdiskSpecialSCMDBoot(pSCMD);
        }
        break;

        case (U8)SCMD_UNMAP:
        {
            L1_RamdiskSpecialSCMDTrim(pSCMD);
        }
        break;

        case (U8)SCMD_ACCESS_HOSTINFO:
        {
            L1_RamdiskSpecialSCMDHostInfo(pSCMD);
        }
        break;

        case (U8)SCMD_ACCESS_DEVPARAM:
        {
            L1_RamdiskSpecialSCMDDevParam(pSCMD);
        }
        break;

        case (U8)SCMD_IDLETASK:
        {
            L1_RamdiskSpecialSCMDIdle(pSCMD);
        }
        break;

        case (U8)SCMD_VIA_DEV_CTRL:
        {
            if (TRUE == FW_ViaCmdDevCtrl(pSCMD))
            {
                L1_RamdiskSCmdFinish();
            }
            else
            {
                L1_RamdiskSCmdFail();
            }
        }
        break;

        case (U8)SCMD_VIA_UART_RAW_DATA_REQ:
        {
            FW_ViaCmdPrepareData(pSCMD);

            L1_RamdiskSCmdFinish();
        }
        break;

        /* Other Special SCMDs, just finish, don't need special process */
        default:
        {
            DBG_Printf("L1_RamdiskSpecialSCMD just finish ucSCmdType 0x%x\n", pSCMD->ucSCmdType);
            L1_RamdiskSCmdFinish();
        }
        break;
    }

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskClearSCQ(void)
{
    /* clear SCMD Queue */
    g_pL1RamdiskSCMD   = NULL;
    g_pL1RamdiskSubCmd = NULL;
    
    while (TRUE)
    {
        g_pL1RamdiskSCMD = L1_GetSCmd();

        if (NULL != g_pL1RamdiskSCMD)
        {
            L1_RamdiskSCmdFinish();
        }
        else
        {
            break;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L1_RamdiskSwitchToSpecialMode(void)
{
#ifndef L1_FAKE
    //FW_DbgShowAll();

    //dirtectly save subsystem trace
    FW_FlushSubsystemTrace(NULL);
#endif

    /* enable L1 Ramdisk special mode */
    L1_RamdiskSetMode(L1_RAMDISK_ENABLE_SPECIAL);

    return;
}

U32 MCU1_DRAM_TEXT L1_RamdiskEventHandle(void)
{
    SMSG tSMSG;
    COMMON_EVENT L1_Event;
    COMM_EVENT_PARAMETER *pParameter;

    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == CommCheckEvent(COMM_EVENT_OWNER_L1,&L1_Event))
    {
        if (L1_RAMDISK_ENABLE_NORMAL == L1_RamdiskGetMode())
        {
            return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
    }

    if(L1_Event.EventDbg)
    {
        /* clear all SCQ pending SCMD */
        L1_RamdiskClearSCQ();

        /* reset HostIO */
        L1_HostIOWarmInit();

        /* clear Debug Event */
        CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
        pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_DBG);
        
        /* report SMSG to L0 */
        tSMSG.ulMsgCode = SMSG_FINISH_DBG_GETCH;
        FW_ReportSMSG(&tSMSG);

        /* enter normal debug mode */
        L1_RamdiskSetMode(L1_RAMDISK_ENABLE_NORMAL);
    }
    else if (L1_Event.EventErrorHandling)
    {      
        /* clear Errorhandle Event */
        CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
        pParameter->EventStatus = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        CommClearEvent(COMM_EVENT_OWNER_L1,COMM_EVENT_OFFSET_ERR);
        
        /* report SMSG to L0 */
        tSMSG.ulMsgCode = SMSG_SCQ_CLEARED;
        
        FW_ReportSMSG(&tSMSG); 
    }
    else
    {

    }

    if (L1_Event.EventLowQD)
    {
        CommClearEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_LOWQD);
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    return COMM_EVENT_STATUS_BLOCKING;
}


/****************************************************************************
Name        :L1_TaskRamdiskInit
Input       :
Output      :
Author      :HenryLuo
Date        :2012.02.20    18:46:30
Description :L1 debug Ramdisk schedule.
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L1_TaskRamdiskInit( SUBSYSTEM_MEM_BASE * pFreeMemBase )
{
    /* allocate dram for L1 Debug Ramdisk */
    L1_RamdiskDramMap(&g_FreeMemBase.ulDRAMBase);

    /* allocate dram for global info. */
    L1_GlobalInfoDramMap(&g_FreeMemBase.ulDRAMBase);

    /*allocate OTFB for Ramdisk*/
    L1_RamdiskOTFBMap(&g_FreeMemBase.ulFreeOTFBBase);

    L1_SharedMemMap(&g_FreeMemBase);

    L1_HostIOInit();
    L1_InitSCQ();
    L1_GlobalInfoInit();
    L1_RamdiskInit();

    L1_SetDefaultDeviceParam();

    return;
}

/****************************************************************************
Name        :L1_RamdiskSchedule
Input       :
Output      :
Author      :HenryLuo
Date        :2012.02.20    18:46:30
Description :L1 debug Ramdisk schedule.
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L1_RamdiskSchedule(void)
{   
    /* process debug event */
    if(COMM_EVENT_STATUS_BLOCKING == L1_RamdiskEventHandle())
    {
        return FALSE;
    }

    /* check and get new SCMD */
    if ((NULL == g_pL1RamdiskSCMD) && (NULL == g_pL1RamdiskSubCmd))
    {
        g_pL1RamdiskSCMD = L1_RamdiskGetSCmd();

        if (NULL != g_pL1RamdiskSCMD)
        {
            if ((SCMD_DIRECT_MEDIA_ACCESS == g_pL1RamdiskSCMD->ucSCmdType)
                || (SCMD_RAW_DATA_REQ == g_pL1RamdiskSCMD->ucSCmdType))
            {
                /* get new SubCmd */
                g_pL1RamdiskSubCmd = L1_RamdiskFillSubCmd(g_pL1RamdiskSCMD);
            }
        }
    }

    /* process Special SCMD */
    if ((NULL != g_pL1RamdiskSCMD) && (((U8)SCMD_DIRECT_MEDIA_ACCESS != g_pL1RamdiskSCMD->ucSCmdType) 
                && ((U8)SCMD_RAW_DATA_REQ != g_pL1RamdiskSCMD->ucSCmdType)))
    {
        /* process special SCMD */
        L1_RamdiskSpecialSCMD(g_pL1RamdiskSCMD);
    }

    /* process SubCmd */
    L1_RamdiskGetHostResource(g_pL1RamdiskSubCmd);

    L1_RamdiskHostIO(g_pL1RamdiskSubCmd);

    L1_RamdiskRecycle(g_pL1RamdiskSubCmd);

    return TRUE;
}

/********************** FILE END ***************/

