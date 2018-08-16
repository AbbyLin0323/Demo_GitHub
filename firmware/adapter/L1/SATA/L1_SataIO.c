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
Filename     : L1_SataIO.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20120118 BlakeZhang 001 first create
****************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_BufMap.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"
#include "HAL_TraceLog.h"
#include "COM_Memory.h"
#include "COM_BitMask.h"
#include "HAL_SataDSG.h"
#include "HAL_SataIO.h"
#include "L1_Inc.h"

#define TL_FILE_NUM L1_SataIO_c
//GLOBAL MCU12_VAR_ATTR volatile L1_SATA_DSG_INFO *g_pSataDSGInfo;
GLOBAL MCU12_VAR_ATTR U8 g_aSataPreDSGID[L1_SATA_NCQ_DEPTH];
U8 g_DsgValid;
extern U8 g_Set1stDataReady;

void MCU1_DRAM_TEXT L1_HostIOInit(void)
{
    //g_pSataDSGInfo = (volatile L1_SATA_DSG_INFO *)SATA_DSGID_INFO_MCU12_BASE;

    //COM_MemZero((U32*)g_pSataDSGInfo,sizeof(L1_SATA_DSG_INFO)/sizeof(U32));

    return;
}

void MCU1_DRAM_TEXT L1_HostIOWarmInit(void)
{
    /* reset L1 DSGInfo */
    //COM_MemZero((U32*)g_pSataDSGInfo,sizeof(L1_SATA_DSG_INFO)/sizeof(U32));
    return;
}

void L1_InitSubCmdHostInfo(SUBCMD* pSubCmd)
{
    pSubCmd->SubCmdHostInfo.ucSubDSGId = INVALID_2F;

    return;
}

#ifdef VT3533_A2ECO_DSGRLSBUG
LOCAL void L1_SataDsgGetLock(void)
{
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLC_DSG_OR_HSG);
    return;
}

LOCAL void L1_SataDsgReleaseLock(void)
{
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLC_DSG_OR_HSG);
    return;
}
#endif

BOOL L1_TryOneReadDSG(void)
{
    BOOL bRet;

    if (HAL_GetAvailableSataDsgCnt(DSG_TYPE_READ) > 2)
    {
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL L1_TryOneWriteDSG(void)
{
    BOOL bRet;

    if (HAL_GetAvailableSataDsgCnt(DSG_TYPE_WRITE) > 2)
    {
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}


U8 L1_GetOneReadDSG(void)
{
    U16  usCurrDSG;

    //L1_SataDsgGetLock();

#if 0
    HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_READ);
    HAL_TriggerSataDsg(DSG_TYPE_READ);
#else
    HAL_GetCurAndTriggerNextSataReadDsg(&usCurrDSG);
#endif

    //L1_SataDsgReleaseLock();

    return (U8)usCurrDSG;
}

U8 L1_GetOneWriteDSG(void)
{
    U16  usCurrDSG;

    //L1_SataDsgGetLock();

#if 0
    HAL_GetCurSataDsg(&usCurrDSG, DSG_TYPE_WRITE);
    HAL_TriggerSataDsg(DSG_TYPE_WRITE);
#else
    HAL_GetCurAndTriggerNextSataWriteDsg(&usCurrDSG);
#endif

    //L1_SataDsgReleaseLock();

    return (U8)usCurrDSG;
}

/*==============================================================================
Func Name  : L1_GetReadDSG
Input      : void  
Output     : NONE
Return Val : 
Discription: when a NON-DATA command need to Xfer data, allocate DSG via this interface.
Usage      : 
History    : 
    1. 2013.11.25 Haven Yang create function
==============================================================================*/
U8  L1_GetReadDSG(void)
{
    U8  ucCurrDSG;

    ucCurrDSG = L1_GetOneReadDSG();

    if (INVALID_2F == ucCurrDSG)
    {
        ucCurrDSG = L1_GetOneReadDSG();
    }

#ifdef SIM
    if (INVALID_2F != ucCurrDSG)
    {
        if (ucCurrDSG >= 64)
        {
            DBG_Printf("L1_GetReadDSG ucCurrDSG 0x%x ERROR!\n", ucCurrDSG);
            DBG_Getch();
        }       
    }
#endif

    return ucCurrDSG;
}

/*==============================================================================
Func Name  : L1_GetWriteDSG
Input      : void  
Output     : NONE
Return Val : 
Discription: when a NON-DATA command need to Xfer data, allocate DSG via this interface.
Usage      : 
History    : 
    1. 2013.11.25 Haven Yang create function
==============================================================================*/
U8  L1_GetWriteDSG(void)
{
    U8 ucCurrDSG;

    ucCurrDSG = L1_GetOneWriteDSG();
    
    if (INVALID_2F == ucCurrDSG)
    {
        ucCurrDSG = L1_GetOneWriteDSG();
    }

    if (INVALID_2F == ucCurrDSG)
    {
        ucCurrDSG = L1_GetOneWriteDSG();
    }

#ifdef SIM
    if (INVALID_2F != ucCurrDSG)
    {
        if ((ucCurrDSG >= 128) || (ucCurrDSG < 64))
        {
            DBG_Printf("L1_GetWriteDSG ucCurrDSG 0x%x ERROR!\n", ucCurrDSG);
            DBG_Getch();
        }       
    }
#endif

    return ucCurrDSG;
}

void L1_UsedSataDSG(U8 ucDSGID)
{
#ifdef SIM
      if(ucDSGID >= SATA_TOTAL_DSG_NUM)
      {
          DBG_Printf("L1_UsedSataDSG ucDSGID 0x%x ERROR!!!\n", ucDSGID);
          DBG_Getch();
      }
#endif
  
    HAL_SetSataDsgValid(ucDSGID);

    return;
}

BOOL L1_GetHostSataDSG(SUBCMD* pSubCmd)
{
    if (INVALID_2F == pSubCmd->SubCmdHostInfo.ucSubDSGId)
    {
        /* get DSG recourse */
        if (DM_WRITE == pSubCmd->pSCMD->tMA.ucOpType)
        {
            pSubCmd->SubCmdHostInfo.ucSubDSGId = L1_GetWriteDSG();
        }
        else
        {
            pSubCmd->SubCmdHostInfo.ucSubDSGId = L1_GetReadDSG();
        }

        /* no DSG recourse, wait */
        if (INVALID_2F == pSubCmd->SubCmdHostInfo.ucSubDSGId)
        {
            return FALSE;
        }
    }

    /* first SubCmd that needs multi-core process, 
        Try next DSG recourse to prevent dead lock casued by no SATA DSG */
    if ((TRUE == pSubCmd->pSCMD->tMA.ucFirst) && (FALSE == pSubCmd->pSCMD->tMA.ucLast))
    {
        if (DM_WRITE == pSubCmd->pSCMD->tMA.ucOpType)
        {
            if(TRUE != L1_TryOneWriteDSG())
            {
                return FALSE;
            }
        }
        else
        {
            if(TRUE != L1_TryOneReadDSG())
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/*****************************************************************************
 Prototype      : L1_CheckDSGResource
 Description    : Check is there free DSGs to use? and fill the DSG id to
                  the subcmd structure
 Input          : SUBCMD* pSubCmd  
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2013/9/10
   Author       : Haven Yang
   Modification : Created function

*****************************************************************************/
BOOL L1_GetHostResource(SUBCMD* pSubCmd)
{
#if 0
    U8  ucTag;
    U8  ucLastDSGId;
    U16 usSCmdIndex;
    U16 usLastIndex;
    volatile SATA_DSG *pLastDSG;
    COMMON_EVENT Event;

    ucTag = pSubCmd->pSCMD->ucSlotNum;

    usSCmdIndex  = L1_GET_CURR_SCMDINDEX(pSubCmd->pSCMD->tMA.usSCmdIndex);
    usLastIndex  = L1_GET_LAST_SCMDINDEX(usSCmdIndex);

    if (FALSE ==  pSubCmd->pSCMD->tMA.ucFirst)
    {
        if (L1_DSGINFO_SATAE_CONFIG != g_pSataDSGInfo->tDSGEntry[ucTag][usLastIndex].ucState)
        {
            /* ignore DSG Info status when L1 ErrorHandling (warm init) */
            CommCheckEvent(COMM_EVENT_OWNER_L1, &Event);

            if (0 == Event.EventErrorHandling)
            {
                /* wait for last DSG configured */
                return FALSE;
            }
        }

        /* current SubCmd needs to finish last DSG, so we wait last DSG configured 
                  then allocate current DSG recourse to prevent no DSG resource deadlock */
        if (FALSE == L1_GetHostSataDSG(pSubCmd))
        {
            return FALSE;
        }

        /* config nextDSGId and set valid for last SubCmd DSG */
        ucLastDSGId = g_pSataDSGInfo->tDSGEntry[ucTag][usLastIndex].ucDSGId;
        pLastDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucLastDSGId);
        pLastDSG->NextDsgId = pSubCmd->SubCmdHostInfo.ucSubDSGId;
        L1_UsedSataDSG(ucLastDSGId);
        g_pSataDSGInfo->tDSGEntry[ucTag][usLastIndex].ucState = L1_DSGINFO_SATAE_FINISH;
    }
    else
    {
        /* current SubCmd doesn't need to finish last DSG, so we allocate current DSG here */
        if (FALSE == L1_GetHostSataDSG(pSubCmd))
        {
            return FALSE;
        }
    }

    /* Init current DSG Info */
    g_pSataDSGInfo->tDSGEntry[ucTag][usSCmdIndex].ucState = L1_DSGINFO_SATAE_INIT;
    g_pSataDSGInfo->tDSGEntry[ucTag][usSCmdIndex].ucDSGId = pSubCmd->SubCmdHostInfo.ucSubDSGId;
#else
    if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
    {
        if (DM_READ == pSubCmd->pSCMD->tMA.ucOpType)
        {
            pSubCmd->SubCmdHostInfo.ucSubDSGId = L1_GetReadDSG();
        }
        else
        {
            pSubCmd->SubCmdHostInfo.ucSubDSGId = L1_GetWriteDSG();
        }

        if (INVALID_2F == pSubCmd->SubCmdHostInfo.ucSubDSGId)
        {
            return FALSE;
        }
    }
#endif

    return TRUE;
}

void L1_FillHostAddrInfo(SUBCMD *pSubCmd, BUF_REQ_READ *pBufReq)
{
    pBufReq->tBufReqHostInfo.ucTag = pSubCmd->pSCMD->ucSlotNum;
    pBufReq->tBufReqHostInfo.ucDSGId = pSubCmd->SubCmdHostInfo.ucSubDSGId;

    /* clear BugMap for Host read */
    HAL_SetBufMapInitValue(pSubCmd->SubCmdHostInfo.ucSubDSGId, 0);

    return;
}

/****************************************************************************
Name        :L1_SataBuildReadDSG
Input       :SUBCMD* pSubCmd
Output      :read DSG id.
Author      :Haven Yang
Date        :2013.8.23
Description :build read DSG. full fill DSG Register bits & trigger it.
Others      :
Modify      :
2014.2.11  kristinwang  modify base on PCIE_SSD/SATA/TRUNK 6264,BuffMap value
****************************************************************************/
BOOL L1_SataBuildReadDSG(SUBCMD* pSubCmd)
{
    U8       ucTag;
    U8       ucDSGId;
    U8       ucPreDSGId; 
    U32      ulBuffMapValue;
    SCMD     *pSCMD;
    volatile SATA_DSG *pCurDSG;
    volatile SATA_DSG *pPreDSG;

#ifdef SIM
    if (INVALID_2F == pSubCmd->SubCmdHostInfo.ucSubDSGId)
    {
        DBG_Printf("L1_SataBuildReadDSG ucSubDSGId ERROR!\n");
        DBG_Getch();
    }
#endif

    pSCMD       = pSubCmd->pSCMD;
    ucTag       = pSCMD->ucSlotNum;
    //usSCmdIndex = L1_GET_CURR_SCMDINDEX(pSubCmd->pSCMD->tMA.usSCmdIndex);
    ucDSGId     = pSubCmd->SubCmdHostInfo.ucSubDSGId;
    pCurDSG     = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucDSGId);

    //COM_MemZero((U32*)pCurDSG, SATA_DSG_SIZE_DW);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
#if 0
    pCurDSG->AtaProtInfo.ProtSel         = ((TRUE == pSCMD->tMA.ucIsPIO) ? PROT_PIO : PROT_DMA_FPDMA);
    pCurDSG->AtaProtInfo.CmdTag          = ucTag;
    pCurDSG->AtaProtInfo.CmdXferSecCnt   = pSCMD->tMA.ulHCmdSecLen;
#else
    pCurDSG->DW0 = (pSCMD->tMA.ulHCmdSecLen & 0xFFFF) |
                   (ucTag << 24) |
                   (~(pSCMD->tMA.ucIsPIO) << 31);
#endif

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
#if 0
    pCurDSG->XferCtrlInfo.BuffLen        = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
    pCurDSG->XferCtrlInfo.BuffMapId      = BUFMAP_ID(ucDSGId);
    pCurDSG->XferCtrlInfo.BuffOffset     = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
    pCurDSG->XferCtrlInfo.BuffMapEn      = BIT_ENABLE;
    pCurDSG->XferCtrlInfo.Eot            = pSCMD->tMA.ucLast;
#else
    pCurDSG->DW1 = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN |
                   (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN << 8 ) |
                   (BUFMAP_ID(ucDSGId) << 16) |
                   (BIT_ENABLE << 23) |
                   (pSCMD->tMA.ucLast << 31);
#endif
    pCurDSG->DW2 = INVALID_2F;
    pCurDSG->DW3 = 0;

    /*========================================================================*/
    /* Read hit Read PreFetch Buffer,need set CacheStaus                      */
    /* must process before fill DW3-- CacheStsAddr                            */
    /*========================================================================*/
    if (L1_CACHE_SE_HIT_RD_PREFETCH == pSubCmd->SubCmdHitResult)
    {
        pCurDSG->XferCtrlInfo.CacheStsEn = BIT_ENABLE;               // DW1
        pCurDSG->CacheStsAddr            = pSubCmd->CacheStatusAddr; // DW3
        pCurDSG->CacheStsData            = SUBSYSTEM_STATUS_SUCCESS; // DW2
    }

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    //pCurDSG->NextDsgId       = INVALID_2F;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    pCurDSG->DataAddr        = (((U32)pSubCmd->SubCmdPhyBufferID) << BUF_SIZE_BITS)
                             + (((U32)pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT) << SEC_SIZE_BITS);
    pCurDSG->CmdLbaLow       = pSCMD->tMA.ulHostStartLBA;

    /*========================================================================*/
    /* set BufMap  & DSG valid                  */
    /*========================================================================*/
    if (L1_CACHE_SE_NO_HIT != pSubCmd->SubCmdHitResult)
    {
        ulBuffMapValue = COM_GETBITMASK(pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN, 
              (pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN + pSubCmd->SubCmdAddInfo.ucSubLPNCountIN - 1));

        HAL_SetBufMapInitValue(pCurDSG->XferCtrlInfo.BuffMapId, ulBuffMapValue);
    }    

    if (TRUE == L1_RamdiskIsEnable())
    {
        pCurDSG->XferCtrlInfo.CacheStsEn = BIT_DISABLE;
        pCurDSG->DataAddr                = L1_RamdiskGetDramAddr(pSCMD);
    }

    if (BIT_TRUE == pSCMD->tMA.ucFirst)
    {
        HAL_SetLastDataReady(ucTag);
        HAL_SetFirstDSGID(ucTag, ucDSGId);
        g_DsgValid = FALSE;
    }
    else
    {
        ucPreDSGId = g_aSataPreDSGID[ucTag];
        pPreDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucPreDSGId);
        pPreDSG->NextDsgId = ucDSGId;
        L1_UsedSataDSG(ucPreDSGId);
        g_DsgValid = TRUE;
    }

    if(BIT_TRUE == pSCMD->tMA.ucLast)
    {
        L1_UsedSataDSG(ucDSGId);
        g_DsgValid = TRUE;
    }

    /* Patch SDC: Need to set FstDataRdy after DSG is valid */
    if ((L1_CACHE_SE_NO_HIT != pSubCmd->SubCmdHitResult) && 
        (TRUE == g_DsgValid) &&
        (FALSE == g_Set1stDataReady))
    {
        HAL_SetFirstReadDataReady(ucTag);
        g_Set1stDataReady = TRUE;
    }
    
    if(BIT_TRUE == pSCMD->tMA.ucLast)
        g_Set1stDataReady = FALSE;
    
    g_aSataPreDSGID[ucTag] = ucDSGId;

    return TRUE;
}


/*****************************************************************************
 Prototype      : L1_SataBuildWriteDSG
 Description    : build write DSG. full fill DSG Register bits & trigger it.
 Input          : SUBCMD* SubCmd  
 Output         : None
 Return Value   : 
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2013/8/28
   Author       : Haven Yang
   Modification : Created function

*****************************************************************************/
BOOL L1_SataBuildWriteDSG(SUBCMD* pSubCmd)
{
    U8       ucTag;
    U8       ucDSGId;
    U8       ucPreDSGId; 
    SCMD     *pSCMD;
    volatile SATA_DSG *pCurDSG;
    volatile SATA_DSG *pPreDSG; 

#ifdef SIM
    if (INVALID_2F == pSubCmd->SubCmdHostInfo.ucSubDSGId)
    {
        DBG_Printf("L1_SataBuildWriteDSG ucSubDSGId ERROR!\n");
        DBG_Getch();
    }
#endif

    pSCMD       = pSubCmd->pSCMD;
    ucTag       = pSCMD->ucSlotNum;
    ucDSGId     = pSubCmd->SubCmdHostInfo.ucSubDSGId;
    pCurDSG     = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucDSGId);

    //usSCmdIndex = L1_GET_CURR_SCMDINDEX(pSubCmd->pSCMD->tMA.usSCmdIndex);

    //COM_MemZero((U32*)pCurDSG, SATA_DSG_SIZE_DW);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
#if 0
    if (TRUE == pSCMD->tMA.ucIsNCQ)
    {
        pCurDSG->AtaProtInfo.AutoActiveEn = BIT_ENABLE;
    }

    pCurDSG->AtaProtInfo.IsWriteCmd      = BIT_TRUE;
    pCurDSG->AtaProtInfo.ProtSel         = pSCMD->tMA.ucIsPIO;
    pCurDSG->AtaProtInfo.CmdTag          = ucTag;
    pCurDSG->AtaProtInfo.CmdXferSecCnt   = pSCMD->tMA.ulHCmdSecLen;
#else
    pCurDSG->DW0 = (pSCMD->tMA.ulHCmdSecLen & 0xFFFF) |
                   (ucTag << 24) |
                   (BIT_TRUE << 29) |
                   (pSCMD->tMA.ucIsNCQ << 30) |
                   (~(pSCMD->tMA.ucIsPIO) << 31);
#endif

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
#if 0
    pCurDSG->XferCtrlInfo.BuffLen        = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
    pCurDSG->XferCtrlInfo.BuffMapId      = BUFMAP_ID(ucDSGId);
    pCurDSG->XferCtrlInfo.BuffOffset     = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
    pCurDSG->XferCtrlInfo.CacheStsEn     = BIT_ENABLE;
    pCurDSG->XferCtrlInfo.Eot            = pSCMD->tMA.ucLast;
#else
    pCurDSG->DW1 = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN |
                   (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN << 8) |
                   (BUFMAP_ID(ucDSGId) << 16) |
                   (BIT_ENABLE << 28) |
                   (pSCMD->tMA.ucLast << 31);
#endif

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
#if 0
    pCurDSG->NextDsgId       = INVALID_2F;
    pCurDSG->CacheStsData    = SUBSYSTEM_STATUS_SUCCESS;
#else
    pCurDSG->DW2 = INVALID_2F | (SUBSYSTEM_STATUS_SUCCESS << 8); 
#endif

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
#if 0
    pCurDSG->CacheStsAddr    = pSubCmd->CacheStatusAddr; 
#else
    pCurDSG->DW3 = pSubCmd->CacheStatusAddr;
#endif
    pCurDSG->DataAddr        = (((U32)pSubCmd->SubCmdPhyBufferID) << BUF_SIZE_BITS)
                             + (((U32)pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT) << SEC_SIZE_BITS);
    pCurDSG->CmdLbaLow       = pSCMD->tMA.ulHostStartLBA;

    if (TRUE == L1_RamdiskIsEnable())
    {
        pCurDSG->XferCtrlInfo.CacheStsEn = BIT_DISABLE;
        pCurDSG->DataAddr                = L1_RamdiskGetDramAddr(pSCMD);
    }

    /*========================================================================*/
    /* set BufMap  & DSG valid                  */
    /*========================================================================*/
    if(BIT_TRUE == pSCMD->tMA.ucLast)
    {
        L1_UsedSataDSG(ucDSGId);
    }

    if (BIT_TRUE == pSCMD->tMA.ucFirst)
    {
        HAL_SetLastDataReady(ucTag);
        HAL_SetFirstDSGID(ucTag, ucDSGId);
        HAL_SetFirstReadDataReady(ucTag);
    }

#if 0
    /* set DSG info status */
    if(BIT_TRUE == pSCMD->tMA.ucLast)
    {
        g_pSataDSGInfo->tDSGEntry[ucTag][usSCmdIndex].ucState = L1_DSGINFO_SATAE_FINISH;
    }
    else
    {
        g_pSataDSGInfo->tDSGEntry[ucTag][usSCmdIndex].ucState = L1_DSGINFO_SATAE_CONFIG;
    }
#else
    if (FALSE == pSCMD->tMA.ucFirst)
    {
        ucPreDSGId = g_aSataPreDSGID[ucTag];
        pPreDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucPreDSGId);
        pPreDSG->NextDsgId = ucDSGId;
        L1_UsedSataDSG(ucPreDSGId);
    }

    g_aSataPreDSGID[ucTag] = ucDSGId;
#endif

    return TRUE;
}

/****************************************************************************
Name:    L1_SataBuildRAWReadDSG
Input:    ucSecCnt ulStartDRamAddr
Output:    No output required.
Author:    Blake Zhang
Date:    2013.02.25
Description: The routine for executing an ATA data access command which follows the PIO in/out protocol.
Others:
Modify:
2014.2.11  kristinwang  same with PCIE_SSD/SATA/TRUNK  6264
****************************************************************************/
BOOL MCU1_DRAM_TEXT L1_SataBuildRAWReadDSG(SUBCMD* pSubCmd)
{
    U8    ucDSGId;
    U32   ulBuffMapValue;
    SCMD *pSCMD;
    volatile SATA_DSG *pCurDSG;

    pSCMD = pSubCmd->pSCMD;

    ucDSGId = L1_GetReadDSG();

    if (INVALID_2F == ucDSGId)
    {
        DBG_Printf("L1_SataBuildRAWReadDSG No free DSG!\n");
        return FALSE;
    }

    pCurDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucDSGId);

    // clear PRD area to 0
    COM_MemZero((U32*)pCurDSG, SATA_DSG_SIZE_DW);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
    pCurDSG->AtaProtInfo.ProtSel         = ((TRUE == pSCMD->tRawData.ucSATAUsePIO)?PROT_PIO:PROT_DMA_FPDMA);
    pCurDSG->AtaProtInfo.CmdTag          = pSCMD->ucSlotNum;
    pCurDSG->AtaProtInfo.CmdXferSecCnt   = pSCMD->tRawData.ucSecLen;

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
    pCurDSG->XferCtrlInfo.BuffLen        = pSCMD->tRawData.ucSecLen;
    pCurDSG->XferCtrlInfo.BuffMapId      = BUFMAP_ID(ucDSGId);
    pCurDSG->XferCtrlInfo.BuffMapEn      = BIT_ENABLE;
    pCurDSG->XferCtrlInfo.Eot            = BIT_TRUE;

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    pCurDSG->NextDsgId       = (U8)INVALID_2F;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    pCurDSG->DataAddr                = pSCMD->tRawData.ulBuffAddr - DRAM_START_ADDRESS;
    pCurDSG->XferCtrlInfo.CacheStsEn = BIT_ENABLE;
    pCurDSG->CacheStsData            = SUBSYSTEM_STATUS_SUCCESS;
    pCurDSG->CacheStsAddr            = pSubCmd->CacheStatusAddr;

    ulBuffMapValue = COM_GETBITMASK(0, ((pSCMD->tRawData.ucSecLen - 1) >> SEC_PER_LPN_BITS));

    HAL_SetBufMapInitValue(pCurDSG->XferCtrlInfo.BuffMapId, ulBuffMapValue);

    HAL_SetLastDataReady(pSCMD->ucSlotNum);
    L1_UsedSataDSG(ucDSGId);
    HAL_SetFirstDSGID(pSCMD->ucSlotNum, ucDSGId);
    HAL_SetFirstReadDataReady(pSCMD->ucSlotNum);

    /* record raw data request DSG ID for debug */
    pSubCmd->SubCmdHostInfo.ucSubDSGId = ucDSGId;

    return TRUE;
}

/****************************************************************************
Name:    L1_SataBuildRAWWriteDSG
Input:    ucSecCnt ulStartDRamAddr
Output:    No output required.
Author:    Blake Zhang
Date:    2013.02.25
Description: The routine for executing an ATA data access command which follows the PIO in/out protocol.
Others:
Modify:
****************************************************************************/
BOOL MCU1_DRAM_TEXT L1_SataBuildRAWWriteDSG(SUBCMD* pSubCmd)
{
    U8    ucDSGId;
    SCMD *pSCMD;
    volatile SATA_DSG *pCurDSG;

    pSCMD = pSubCmd->pSCMD;

    ucDSGId = L1_GetWriteDSG();

    if (INVALID_2F == ucDSGId)
    {
        DBG_Printf("L1_SataBuildRAWWriteDSG No free DSG!\n");
        return FALSE;
    }

    pCurDSG = (volatile SATA_DSG *)HAL_GetSataDsgAddr(ucDSGId);

    // clear PRD area to 0
    COM_MemZero((U32*)pCurDSG, SATA_DSG_SIZE_DW);

    /*========================================================================*/
    /* DW0: ata prot info                                                     */
    /*========================================================================*/
    pCurDSG->AtaProtInfo.IsWriteCmd      = BIT_TRUE;
    pCurDSG->AtaProtInfo.ProtSel         = ((TRUE == pSCMD->tRawData.ucSATAUsePIO)?PROT_PIO:PROT_DMA_FPDMA);
    pCurDSG->AtaProtInfo.CmdTag          = pSCMD->ucSlotNum;
    pCurDSG->AtaProtInfo.CmdXferSecCnt   = pSCMD->tRawData.ucSecLen;

    /*========================================================================*/
    /* DW1: Transfer control info                                             */
    /*========================================================================*/
    pCurDSG->XferCtrlInfo.BuffLen        = pSCMD->tRawData.ucSecLen;
    pCurDSG->XferCtrlInfo.BuffMapId      = BUFMAP_ID(ucDSGId);
    pCurDSG->XferCtrlInfo.Eot            = BIT_TRUE;

    /*========================================================================*/
    /* DW2                                                                    */
    /*========================================================================*/
    pCurDSG->NextDsgId       = INVALID_2F;

    /*========================================================================*/
    /* DW3-5:Cache status Address/ Data Address(DRAM)/ Command LBA Low 32 bit */
    /*========================================================================*/
    pCurDSG->DataAddr                = pSCMD->tRawData.ulBuffAddr - DRAM_START_ADDRESS;
    pCurDSG->XferCtrlInfo.CacheStsEn = BIT_ENABLE;
    pCurDSG->CacheStsData            = SUBSYSTEM_STATUS_SUCCESS;
    pCurDSG->CacheStsAddr            = pSubCmd->CacheStatusAddr;

    HAL_SetLastDataReady(pSCMD->ucSlotNum);
    L1_UsedSataDSG(ucDSGId);
    HAL_SetFirstDSGID(pSCMD->ucSlotNum, ucDSGId);
    HAL_SetFirstReadDataReady(pSCMD->ucSlotNum);

    /* record raw data request DSG ID for debug */
    pSubCmd->SubCmdHostInfo.ucSubDSGId = ucDSGId;

    return TRUE;
}


BOOL L1_HostIO(SUBCMD* pSubCmd)
{
    BOOL bRet;
    SCMD *pSCMD;

    bRet  = FALSE;
    pSCMD = pSubCmd->pSCMD;

    if (SCMD_DIRECT_MEDIA_ACCESS == pSCMD->ucSCmdType)
    {      
        if (DM_READ == pSCMD->tMA.ucOpType)
        {
            bRet = L1_SataBuildReadDSG(pSubCmd);
            TL_PERFORMANCE(PTL_LEVEL_NULL, "L1_SataBuildReadDSG");
        }
        else
        {
            bRet = L1_SataBuildWriteDSG(pSubCmd);
            TL_PERFORMANCE(PTL_LEVEL_NULL, "L1_SataBuildWriteDSG");
        }
    }
    else
    {
        if (RAWDRQ_D2H == pSCMD->tRawData.ucDataDir)
        {
            bRet = L1_SataBuildRAWReadDSG(pSubCmd);
            TL_PERFORMANCE(PTL_LEVEL_NULL, "L1_SataBuildRAWReadDSG");
        }
        else
        {
            bRet = L1_SataBuildRAWWriteDSG(pSubCmd);
            TL_PERFORMANCE(PTL_LEVEL_NULL, "L1_SataBuildRAWWriteDSG");
        }
    }    

    return bRet;
}


/**************** FILE END ***************/



