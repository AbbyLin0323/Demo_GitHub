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
Filename    :
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.20    18:44:39
Description :L1 task schedule.
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "FW_ViaCmd.h"
#include "HAL_TraceLog.h"
#include "HAL_MemoryMap.h"
#include "COM_BitMask.h"

#include "HAL_Xtensa.h"

//specify file name for Trace Log
#define TL_FILE_NUM  L1_SubCmdProc_c

#ifdef DATA_MONITOR_ENABLE
#include "FW_DataMonitor.h"
#endif

/* use for SubCmd */
GLOBAL  SUBCMD *g_pCurSubCmd;
GLOBAL  SUBCMD_ENTRY *gpSubCmdEntry;

/* use for Write Partial Hit */
GLOBAL  U8      gPartialHitLPNCnt;
GLOBAL  U8      gPartialHitLPNBase;
GLOBAL  U8      gPartialHitFlag;
GLOBAL  U8      g_ucPartialHitNeedNewWriteCache;

GLOBAL  U16 g_usPartialReadCachestatus;

GLOBAL  U32 g_L1IdleCount;
GLOBAL BOOL g_L1PopCmdEn;
GLOBAL BOOL g_L2RebuildDC;

extern GLOBAL U32 g_ulHostWriteSec;
extern GLOBAL MCU1_DRAM_DATA BOOL g_bSubSystemIdle;

LOCAL    U8  g_ucCurMergeFlushPu;
LOCAL    U8  g_ucCurrFlushPU;
LOCAL    U8  g_ucCurrRecyclePU;
LOCAL    U8  g_ucCurrFlushCachePU;

#ifdef HOST_READ_FROM_DRAM
LOCAL  MCU12_VAR_ATTR  U8  g_ucCurrRFDPU;
#endif

//GLOBAL  U8 l_ucSubCmdWr;

GLOBAL  U32 g_L1ErrorHandleStatus;

#ifdef HOST_SATA
LOCAL MCU12_VAR_ATTR U8* l_pucCacheLockedFlag;
LOCAL MCU12_VAR_ATTR U8 l_ucSubsystemNum;
#endif

extern GLOBAL  PSCMD g_pCurSCmd;
extern GLOBAL  BOOL g_bL1BackgroundTaskEnable;
extern void L1_DbgPrintSubCmd(SUBCMD *pSubCmd);
extern void L1_InitSubCmdHostInfo(SUBCMD* pSubCmd);
extern BOOL L1_HostIO(SUBCMD* pSubCmd);
extern BOOL L1_GetHostResource(SUBCMD* pSubCmd);
extern void FW_DbgShowAll(void);
extern void L1_DbgPrintSCMD(SCMD *pSCMD);
extern void L1_DbgPrintSubCmd(SUBCMD *pSubCmd);
extern BOOL L2_IsNeedWriteTrim(U8 ucSuperPu);
extern U8 g_Set1stDataReady;

void MCU1_DRAM_TEXT L1_HostCMDProcSram0Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    // gpSubCmdEntry
    gpSubCmdEntry   = (SUBCMD_ENTRY *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign( sizeof(SUBCMD_ENTRY) ) );
    
    COM_MemAddr16DWAlign(&ulFreeSramBase);
    *pFreeSramBase = ulFreeSramBase;
    return;
}

void MCU1_DRAM_TEXT L1_HostCMDProcInit(void)
{
    U8 i;

    g_pCurSubCmd = NULL;
    //l_ucSubCmdWr = 0;
    g_ucCurMergeFlushPu = 0;
    g_ucCurrFlushPU = 0;
    g_ucCurrRecyclePU = 0;
    g_ucCurrFlushCachePU = 0;

    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_ucCurMergePos[i] = 0;
    }

#ifdef HOST_READ_FROM_DRAM
    g_ucCurrRFDPU = 0;
#endif

    gPartialHitFlag   = PARTIAL_HIT_NONE;
    g_ucPartialHitNeedNewWriteCache = FALSE;
    
    g_L1IdleCount = 0;
    g_pMCU12MiscInfo->bSubSystemIdle = FALSE;
    g_L1ErrorHandleStatus = L1_ERRORHANDLE_INIT;
    g_L1PopCmdEn = TRUE;
    g_L2RebuildDC = FALSE;
    g_ulHostWriteSec = 0;

    return;
}

void MCU1_DRAM_TEXT L1_HostCMDProcWarmInit(void)
{
    U8 i;

    g_pCurSCmd   = NULL;
    g_pCurSubCmd = NULL;

    /* disable SubSystem internal idle */
    g_bL1BackgroundTaskEnable = FALSE;

    /* reset partial hit flags */
    gPartialHitLPNCnt  = 0;
    gPartialHitLPNBase = 0;
    gPartialHitFlag    = PARTIAL_HIT_NONE;

    g_ucPartialHitNeedNewWriteCache = FALSE;

    g_L1IdleCount = 0;
    g_pMCU12MiscInfo->bSubSystemIdle = FALSE;

    //l_ucSubCmdWr = 0;
    g_ucCurMergeFlushPu = 0;
    g_ucCurrFlushPU = 0;
    g_ucCurrRecyclePU = 0;
    g_ucCurrFlushCachePU = 0;

    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_ucCurMergePos[i] = 0;
    }

#ifdef HOST_READ_FROM_DRAM
    g_ucCurrRFDPU = 0;
#endif

    g_Set1stDataReady = FALSE;

    return;
}

BOOL L1_HandleWriteNoHit(SUBCMD* pSubCmd)
{
    U8 ucSectorOffsetInLct;
    U8 ucSectorLengthInLct;
    U8 ucLpnOffsetInLct;
    U8 ucLpnCountInLct;
    U32 ulLctStartLpn;
    U32 i;
    U8 ucPuNum;
    U8 ucCacheFreeStatus;

    ucPuNum = pSubCmd->SubCmdAddInfo.ucPuNum;

    ucSectorOffsetInLct = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
    ucSectorLengthInLct = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
    ucLpnOffsetInLct = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
    ucLpnCountInLct = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
    ucCacheFreeStatus = L1_CacheGetTagStatus(ucPuNum, pSubCmd->SubCmdAddInfo.ucSubLPNCountIN);

    if(L1_STATUS_CACHE_NO_SPACE == ucCacheFreeStatus)
    {
        // the current active write buffer doesn't have enough space for the current write request,
        // we have to determine if we can perform a cross buffer write

        U16 usLastLogBufId = L1_GetLastLogBufId(ucPuNum);

        if((usLastLogBufId == LGCBUFID_FROM_PHYBUFID(g_aCacheLine[ucPuNum])) || (ucSectorLengthInLct == SEC_PER_BUF))
        {
            // the current active write buffer is the last buffer of the current PU or the current write request is
            // sequential, we shouldn't perform cross boundary write in both cases

            L1_ReleaseCacheLine(ucPuNum);
            return FALSE;
        }
        else
        {
            // the current write buffer isn't the last one and the current write request isn't sequential,
            // we can perform cross buffer write

            U16 usCurrentPhyBufId;
            U16 usNextPhyBufId;
            U16 usCurrentLogBufId;
            U16 usNextLogBufId;
            U8 ucCurrentWritePointer;
            U8 ucCurrentActiveBufferFreeLpnCount;
            U8 ucNextWriteBufferRemainingLpnCount;

            usNextPhyBufId = L1_AllocateWriteBuf(ucPuNum);

            if(INVALID_4F == usNextPhyBufId)
            {
#ifndef LCT_TRIM_REMOVED
                L1_ProcessPendingTrim();
#endif
                return FALSE;
            }
            
            // obtain the buffer id information for both the current and next write buffer
            usCurrentPhyBufId = g_aCacheLine[ucPuNum];
            usCurrentLogBufId = LGCBUFID_FROM_PHYBUFID(usCurrentPhyBufId);
            usNextLogBufId = LGCBUFID_FROM_PHYBUFID(usNextPhyBufId);

#ifdef SIM
            if(usCurrentPhyBufId == INVALID_4F)
            {
                DBG_Printf("current write buffer physical buffer id error\n");
                DBG_Getch();
            }

            if(usNextLogBufId != (usCurrentLogBufId + 1))
            {
                DBG_Printf("buffer allocation error\n");
                DBG_Getch();
            }
#endif

            // now that both the current and the next write buffer are ready, begin processing
            // process the first write buffer

            ucCurrentWritePointer = L1_BufferGetWritePointer(usCurrentLogBufId);

#ifdef SIM
            // check the validity of the current write pointer
            if((ucCurrentWritePointer == LPN_PER_BUF) || (ucCurrentWritePointer == 0))
            {
                DBG_Printf("current write pointer error\n");
                DBG_Getch();
            }
#endif

            pSubCmd->SubCmdPhyBufferID = usCurrentPhyBufId;
            pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucCurrentWritePointer << LPN_SECTOR_BIT) + (ucSectorOffsetInLct & LPN_SECTOR_MSK);
            pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = ucSectorLengthInLct;
            pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucCurrentWritePointer;
            pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT = ucLpnCountInLct;

            ucCurrentActiveBufferFreeLpnCount = LPN_PER_BUF - ucCurrentWritePointer;

            ulLctStartLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT);

            for(i = 0; i < ucCurrentActiveBufferFreeLpnCount; i++)
            {
                U8 ucCurrLpnBitmap = pSubCmd->LpnSectorBitmap[ucLpnOffsetInLct + i];
                L1_SetBufferLPN(usCurrentLogBufId, ucCurrentWritePointer, (ulLctStartLpn + ucLpnOffsetInLct + i));
                L1_SetBufferSectorBitmap(usCurrentLogBufId, ucCurrentWritePointer, ucCurrLpnBitmap);
                ucCurrentWritePointer++;
            }

            L1_BufferSetWritePointer(usCurrentLogBufId, ucCurrentWritePointer);

#ifdef SIM
            // perform a validity check on the write pointer of the current active write buffer
            if(ucCurrentWritePointer != LPN_PER_BUF)
            {
                DBG_Printf("write pointer error\n");
                DBG_Getch();
            }
#endif

            L1_ReleaseCacheLine(ucPuNum);

            gpBufTag[usNextLogBufId].Stage = BUF_STAGE_CACHE;
            L1_CacheAttachBuffer(ucPuNum, usNextPhyBufId);

            ucCurrentWritePointer = L1_BufferGetWritePointer(usNextLogBufId);

#ifdef SIM
            // perform a validity check for the write pointer
            if(ucCurrentWritePointer != 0)
            {
                DBG_Printf("current write pointer error\n");
                DBG_Getch();
            }
#endif

            // set buffer LPN and sector bitmap for the next write buffer

            ucNextWriteBufferRemainingLpnCount = ucLpnCountInLct - ucCurrentActiveBufferFreeLpnCount;

            for(i = 0; i < ucNextWriteBufferRemainingLpnCount; i++)
            {
                U8 ucCurrLpnBitmap = pSubCmd->LpnSectorBitmap[ucLpnOffsetInLct + ucCurrentActiveBufferFreeLpnCount + i];
                L1_SetBufferLPN(usNextLogBufId, ucCurrentWritePointer, (ulLctStartLpn + ucLpnOffsetInLct + ucCurrentActiveBufferFreeLpnCount + i));
                L1_SetBufferSectorBitmap(usNextLogBufId, ucCurrentWritePointer, ucCurrLpnBitmap);
                ucCurrentWritePointer++;
            }

            L1_BufferSetWritePointer(usNextLogBufId, ucCurrentWritePointer);

#ifdef SIM
            // perform a validity check for the write pointer
            if(ucCurrentWritePointer == LPN_PER_BUF)
            {
                DBG_Printf("current write pointer error\n");
                DBG_Getch();
            }
#endif

            L1_UpdateCacheIdLink(pSubCmd);

#ifdef HOST_NVME
            if (DM_WRITE_ZERO == pSubCmd->pSCMD->tMA.ucOption)
            {
                U32 ulDesAddr = COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID, TRUE, BUF_SIZE_BITS) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT << SEC_SIZE_BITS);
                HAL_DMAESetValue(ulDesAddr, pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN << SEC_SIZE_BITS, 0);
            }
            else
 #endif
            {
                // add cache status nodes to both the current and next write buffer and set up 2 cache status nodes
                // for related relationship
                pSubCmd->CacheStatusAddr = L1_AddRelatedCachestatusNodes(&(gpBufTag[usCurrentLogBufId].usStartHWID), pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, (SEC_PER_BUF - pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT), 
                                                                                &(gpBufTag[usNextLogBufId].usStartHWID), 0, (ucSectorLengthInLct - (SEC_PER_BUF - pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT))) - OTFB_START_ADDRESS;
            }
        }
    }
    else
    {
        // there is no active write buffer or there is a current write buffer that has enough space to serve
        // the current write request

        U16 usCurrentPhyBufId;
        U16 usCurrentLogBufId;
        U8 ucCurrentWritePointer;

        usCurrentPhyBufId = g_aCacheLine[ucPuNum];

        if(usCurrentPhyBufId == INVALID_4F)
        {
            usCurrentPhyBufId = L1_AllocateWriteBuf(ucPuNum);

            if(INVALID_4F == usCurrentPhyBufId)
            {
                // fail to allocate a new buffer, return immediately
#ifndef LCT_TRIM_REMOVED
                L1_ProcessPendingTrim();
#endif
                return FALSE;
            }
            else
            {
                L1_CacheAttachBuffer(ucPuNum, usCurrentPhyBufId);

                usCurrentLogBufId = LGCBUFID_FROM_PHYBUFID(usCurrentPhyBufId);

                gpBufTag[usCurrentLogBufId].Stage = BUF_STAGE_CACHE;
            }
        }
        else
        {
            usCurrentLogBufId = LGCBUFID_FROM_PHYBUFID(usCurrentPhyBufId);
        }

        ucCurrentWritePointer = L1_BufferGetWritePointer(usCurrentLogBufId);

        pSubCmd->SubCmdPhyBufferID = usCurrentPhyBufId;
        pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucCurrentWritePointer << LPN_SECTOR_BIT) + (ucSectorOffsetInLct & LPN_SECTOR_MSK);
        pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = ucSectorLengthInLct;
        pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucCurrentWritePointer;
        pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT = ucLpnCountInLct;

        ulLctStartLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT);

        for(i = 0; i < ucLpnCountInLct; i++)
        {
            U8 ucCurrLpnBitmap = pSubCmd->LpnSectorBitmap[ucLpnOffsetInLct + i];
            L1_SetBufferLPN(usCurrentLogBufId, ucCurrentWritePointer, (ulLctStartLpn + ucLpnOffsetInLct + i));
            L1_SetBufferSectorBitmap(usCurrentLogBufId, ucCurrentWritePointer, ucCurrLpnBitmap);
            ucCurrentWritePointer++;
        }

        L1_BufferSetWritePointer(usCurrentLogBufId, ucCurrentWritePointer);

        gpBufTag[usCurrentLogBufId].bSeqBuf = (SEC_PER_BUF == pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN)? TRUE : FALSE;

        L1_UpdateCacheIdLink(pSubCmd);

#ifdef HOST_NVME
        if (DM_WRITE_ZERO == pSubCmd->pSCMD->tMA.ucOption)
        {
            U32 ulDesAddr = COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID, TRUE, BUF_SIZE_BITS) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT << SEC_SIZE_BITS);
            HAL_DMAESetValue(ulDesAddr, pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN << SEC_SIZE_BITS, 0);
        }
        else
#endif
        {
            pSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[usCurrentLogBufId].usStartHWID), pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, pSubCmd->pSCMD->tMA.ucSecLen) - OTFB_START_ADDRESS;
        }

        // release the write buffer directly to the merge pending link if it's already full
        if(ucCurrentWritePointer == LPN_PER_BUF)
        {
            L1_ReleaseCacheLine(pSubCmd->SubCmdAddInfo.ucPuNum);
        }
    }

    return TRUE;
}

/****************************************************************************
Function  : L1_HandleWritePartialHit
Input     : 
Output    : TRUE, process finished; FALSE, process ongoing
Return    : 
Purpose   : handle read partial hit
Reference :
Modification History:
20121129   Blake Zhang  first create function
****************************************************************************/
BOOL L1_HandleWritePartialHit(SUBCMD* pSubCmd)
{
    // Sean Gao 20150514
    // this function processes the write partial hit case

    switch(gPartialHitFlag)
    {
        case PARTIAL_HIT_NONE:
        {
            // we will prepare a cache for the write partial hit in this stage
            U8 ucCacheFreeStatus;
            U16 usPhyBufID;

            // get the status of the current active cache
            ucCacheFreeStatus = L1_CacheGetTagStatus(pSubCmd->SubCmdAddInfo.ucPuNum, pSubCmd->SubCmdAddInfo.ucSubLPNCountIN);
            
            // if the active cache doesn't have enough space left for
            // the write partial hit, release it
            if(L1_STATUS_CACHE_NO_SPACE == ucCacheFreeStatus)
            {
                U32 ulStartLpn;
                U32 ulLastLpnWritten;
                U16 usActiveWriteCacheLogBufId;

                // calculate the logical buffer id of the active write cache
                usActiveWriteCacheLogBufId = LGCBUFID_FROM_PHYBUFID(g_aCacheLine[(U16)pSubCmd->SubCmdAddInfo.ucPuNum]);

                // calculate the start LPN of the current write request
                ulStartLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;

                // get the last LPN written in the active write cache
                ulLastLpnWritten = L1_GetBufferLPN(usActiveWriteCacheLogBufId, L1_BufferGetWritePointer(usActiveWriteCacheLogBufId)-1);

                // if the following 3 conditions hold, we don't release the active write cache and treat this as
                // a special case:
                // 1. the first LPN to be written in the current request is the same as the last LPN written to the
                // active write cache
                // 2. the free LPN entries of the active write cache is only off by 1
                // 3. there are no overlapped sectors in the first LPN
                if((ulStartLpn == ulLastLpnWritten) && ((LPN_PER_BUF - L1_BufferGetWritePointer(usActiveWriteCacheLogBufId) + 1) >= pSubCmd->SubCmdAddInfo.ucSubLPNCountIN) && ((pSubCmd->LpnSectorBitmap[pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN] & L1_GetBufferSectorBitmap(usActiveWriteCacheLogBufId, L1_BufferGetWritePointer(usActiveWriteCacheLogBufId) - 1)) == 0))
                {
                    // if the start lpn of the current write request is the last
                    // lpn written in the active write cache and the space left in
                    // the active write cache is only off by 1 to the requirement,
                    // do not release the active write cache and treat it as a
                    // special case
                    ucCacheFreeStatus = L1_STATUS_CACHE_OK;
                }
                else
                {
                    // the current active write buffer doesn't have enough space and it isn't a special case,
                    // we have to check if we can support cross buffer write

                    U16 usLastLogBufId = L1_GetLastLogBufId(pSubCmd->SubCmdAddInfo.ucPuNum);

                    if((usLastLogBufId == LGCBUFID_FROM_PHYBUFID(g_aCacheLine[pSubCmd->SubCmdAddInfo.ucPuNum])) || (pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN == SEC_PER_BUF))
                    {
                        // the current active write buffer is the last buffer of the PU or the current request is sequential,
                        // we do not perform cross buffer write in both cases

                        // release the active cache and set the status as no buffer
                        L1_ReleaseCacheLine(pSubCmd->SubCmdAddInfo.ucPuNum);
                        ucCacheFreeStatus = L1_STATUS_CACHE_NO_BUFF;
                    }
                    else
                    {
                        // cross buffer write can be performed

                        U16 usNextPhyBufId = L1_AllocateWriteBuf(pSubCmd->SubCmdAddInfo.ucPuNum);

                        // fail to allocate a new buffer, set g_ucPartialHitNeedNewWriteCache as TRUE
                        // and return immediately
                        if(INVALID_4F == usNextPhyBufId)
                        {
#ifndef LCT_TRIM_REMOVED
                            L1_ProcessPendingTrim();
#endif
                            g_ucPartialHitNeedNewWriteCache = TRUE;
                            return FALSE;
                        }
                        else
                        {
#ifdef SIM
                            if(usNextPhyBufId != (g_aCacheLine[pSubCmd->SubCmdAddInfo.ucPuNum] + 1))
                            {
                                DBG_Printf("next write buffer error\n");
                                DBG_Getch();
                            }
#endif
                        }
                    }
                }
            }
      
            if(L1_STATUS_CACHE_NO_BUFF == ucCacheFreeStatus)
            {
                // allocate a new buffer
                usPhyBufID = L1_AllocateWriteBuf(pSubCmd->SubCmdAddInfo.ucPuNum);

                // fail to allocate a new buffer, set g_ucPartialHitNeedNewWriteCache as TRUE
                // and return immediately
                if(INVALID_4F == usPhyBufID)
                {
#ifndef LCT_TRIM_REMOVED
                    L1_ProcessPendingTrim();
#endif
                    g_ucPartialHitNeedNewWriteCache = TRUE;
                    return FALSE;
                }

                // set the new buffer as the active cache
                L1_CacheAttachBuffer(pSubCmd->SubCmdAddInfo.ucPuNum, usPhyBufID);

                // set the buffer stage as cache
                gpBufTag[LGCBUFID_FROM_PHYBUFID(usPhyBufID)].Stage = BUF_STAGE_CACHE;
            }
            else
            {
                usPhyBufID = g_aCacheLine[(U16)pSubCmd->SubCmdAddInfo.ucPuNum];
            }

            // set the partial hit information, including LPN count and
            // start LPN offset
            gPartialHitLPNCnt  = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
            gPartialHitLPNBase = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;

            // set the subcommand information
            pSubCmd->SubCmdPhyBufferID = usPhyBufID;
            pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = L1_BufferGetWritePointer(LGCBUFID_FROM_PHYBUFID(usPhyBufID));
            pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
            pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT << SEC_PER_LPN_BITS) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & SEC_PER_LPN_MSK);
            pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;

            // advance the partial hit phase
            g_ucPartialHitNeedNewWriteCache = FALSE;
            gPartialHitFlag = PARTIAL_HIT_PHASE1;
        }

        // please note that we will directly enter phase 1 without returning

        case PARTIAL_HIT_PHASE1:
        {
            while(gPartialHitLPNCnt != 0)
            {
                // in this phase we search all requested LPNs one by one and
                // handle them accordingly based on the search result

                U32 ulCurLpn;
                U16 usNewCacheLogBufId;
                U8 ucNewCacheLpnOffset;
                LCT_HIT_RESULT HitResult;

                // acquire basic information of the new cache
                usNewCacheLogBufId  = LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID);
                ucNewCacheLpnOffset = L1_BufferGetWritePointer(usNewCacheLogBufId);

                // check if the current write buffer is full
                if(ucNewCacheLpnOffset == LPN_PER_BUF)
                {
                    usNewCacheLogBufId += 1;

                    ucNewCacheLpnOffset = L1_BufferGetWritePointer(usNewCacheLogBufId);

#ifdef SIM
                    // perform a few validity check
                    if((usNewCacheLogBufId-1) == L1_GetLastLogBufId(pSubCmd->SubCmdAddInfo.ucPuNum))
                    {
                        DBG_Printf("next write buffer error");
                        DBG_Getch();
                    }

                    if(ucNewCacheLpnOffset == LPN_PER_BUF)
                    {
                        DBG_Printf("next write buffer write pointer error\n");
                        DBG_Getch();
                    }
#endif
                }

                // search if the current LPN is in a buffer
                ulCurLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + gPartialHitLPNBase;
                HitResult.AsU32 = L1_CacheIDLinkSearchLPN(ulCurLpn, 1, FALSE);

                // take actions based on the search result
                if(L1_CACHE_SE_FULL_HIT == HitResult.ucHitResult)
                {            
                    // the current LPN is in a buffer, we copy its data to
                    // the new cache(if the current LPN is the first or the last)
                    // and invalidate its data on the hit cache

                    U8 i;
                    U8 ucCopySectorBitmap;
                    U8 ucCopySectorCount;
                    U8 ucHitLpnSectorBitmap;
                    U8 ucHitCacheOffset;
                    U16 usHitPhyBufID;

                    // acquire basic hit cache information
                    ucHitCacheOffset = HitResult.ucCacheOffset;
                    usHitPhyBufID = PHYBUFID_FROM_LGCBUFID(HitResult.usLogBufID);
                    ucHitLpnSectorBitmap = L1_GetBufferSectorBitmap(HitResult.usLogBufID, ucHitCacheOffset);

                    // check for the special case
                    if((gPartialHitLPNBase == pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN) && (HitResult.usLogBufID == usNewCacheLogBufId) && (ucHitCacheOffset == (ucNewCacheLpnOffset - 1)) && ((pSubCmd->LpnSectorBitmap[pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN] & L1_GetBufferSectorBitmap(HitResult.usLogBufID, ucHitCacheOffset)) == 0))
                    {
                        // the first LPN of the current write request is the last LPN
                        // written in the active write cache, do not copy data

                        // update subcommand information
                        pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT -= SEC_PER_LPN;
                        pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT -= 1;

                        ucNewCacheLpnOffset -= 1;
                        L1_BufferSetWritePointer(usNewCacheLogBufId, ucNewCacheLpnOffset);
                    }
                    else
                    {
                        // calculate the copy sector bitmap, which indicate which
                        // sectors of the hit LPN need to be copied to the new cache
                        ucCopySectorBitmap = (~pSubCmd->LpnSectorBitmap[gPartialHitLPNBase]) & ucHitLpnSectorBitmap;

                        // copy data from hit LPN to the new cache if necessary
                        if(ucCopySectorBitmap != 0)
                        {
                            U32 ulSrcBufAddr;
                            U32 ulDesBufAddr;

                            // check if the data of the hit LPN is ready, we have to check both the host write
                            // and the NFC read cache status
                            if (BUF_BUSY == L1_HostPartialHitCheckBusy(HitResult.usLogBufID, ucHitCacheOffset))
                            {
                                return FALSE;
                            }

                            // calculate the source and the destination address, which are
                            // the address of the hit LPN and the address of the new cache
                            ulSrcBufAddr = COM_GetMemAddrByBufferID(usHitPhyBufID, TRUE, BUF_SIZE_BITS) + (ucHitCacheOffset << LPN_SIZE_BITS);
                            ulDesBufAddr = COM_GetMemAddrByBufferID(PHYBUFID_FROM_LGCBUFID(usNewCacheLogBufId), TRUE, BUF_SIZE_BITS) + (ucNewCacheLpnOffset << LPN_SIZE_BITS);

                            ucCopySectorCount = HAL_POPCOUNT(ucCopySectorBitmap);
                            HAL_Wclzstate(ucCopySectorBitmap);

                            for(i = 0; i < ucCopySectorCount; i++)
                            {
                                U8 ucSectorOffset = (31 - HAL_SCLZ());
                                HAL_DMAECopyOneBlock(ulDesBufAddr + ucSectorOffset*SEC_SIZE, ulSrcBufAddr + ucSectorOffset*SEC_SIZE, SEC_SIZE);
                            }
                        } // if(ucCopySectorBitmap != 0)
                    }

                    L1_CacheInvalidLPNInLCT(HitResult.usLogBufID, ucHitCacheOffset, 1);

                    L1_SetBufferSectorBitmap(usNewCacheLogBufId, ucNewCacheLpnOffset, pSubCmd->LpnSectorBitmap[gPartialHitLPNBase] | ucHitLpnSectorBitmap);
                    L1_SetBufferLPN(usNewCacheLogBufId, ucNewCacheLpnOffset, ulCurLpn);
                }
                else
                {
                    // the current LPN is not in a buffer, just update the LPN
                    // and sector bitmap of the new cache
                    L1_SetBufferSectorBitmap(usNewCacheLogBufId, ucNewCacheLpnOffset, pSubCmd->LpnSectorBitmap[gPartialHitLPNBase]);
                    L1_SetBufferLPN(usNewCacheLogBufId, ucNewCacheLpnOffset, ulCurLpn);
                }

                L1_BufferSetWritePointer(usNewCacheLogBufId, ucNewCacheLpnOffset + 1);

                gPartialHitLPNBase++;
                gPartialHitLPNCnt--;

                // check if all LPNs are processed
                if(gPartialHitLPNCnt == 0)
                {
                    // update the LCT
                    L1_UpdateCacheIdLink(pSubCmd);

                    // advance the partial hit phase
                    gPartialHitFlag = PARTIAL_HIT_PHASE2;
                }
            } // while(gPartialHitLPNCnt != 0)
        }

        // please note that we will directly enter phase 2 without returning

        case PARTIAL_HIT_PHASE2:
        {
            // add a cachestatus to the new cache

#ifdef SIM
            // check the Stage of the new cache
            if(BUF_STAGE_CACHE != gpBufTag[LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID)].Stage)
            {
                DBG_Printf("L1_HandleWritePartialHit cache stage error\n", pSubCmd->SubCmdPhyBufferID);
                DBG_Getch();
            }
#endif

            // ensure we have enough cachestatus nodes
            if(FALSE == L1_CacheStatusRescourseCheck(2))
            {
                return FALSE;
            }

            // first we have to check if this is a cross buffer write
            if((pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT + pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT - 1) >= SEC_PER_BUF)
            {
                // cross buffer write
                U16 usCurrentLogBufId = LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID);
                U16 usNextLogBufId = usCurrentLogBufId + 1;


 #ifdef HOST_NVME
                if (DM_WRITE_ZERO == pSubCmd->pSCMD->tMA.ucOption)
                {   
                    U32 ulDesAddr = COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID, TRUE, BUF_SIZE_BITS) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT << SEC_SIZE_BITS);
                    HAL_DMAESetValue(ulDesAddr, pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN << SEC_SIZE_BITS, 0);
                }
                else
 #endif
                 {
                    // add cache status nodes to both the current and next write buffer and set up 2 cache status nodes
                    // for related relationship
                    pSubCmd->CacheStatusAddr = L1_AddRelatedCachestatusNodes(&(gpBufTag[usCurrentLogBufId].usStartHWID), pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, (SEC_PER_BUF - pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT), &(gpBufTag[usNextLogBufId].usStartHWID), 0, (pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN - (SEC_PER_BUF - pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT))) - OTFB_START_ADDRESS;
                }
                // release the currrent write buffer since it's already full
                L1_ReleaseCacheLine(pSubCmd->SubCmdAddInfo.ucPuNum);

                // set the next write buffer as the active one
                L1_CacheAttachBuffer(pSubCmd->SubCmdAddInfo.ucPuNum, PHYBUFID_FROM_LGCBUFID(usNextLogBufId));

                // set the buffer stage as cache
                gpBufTag[usNextLogBufId].Stage = BUF_STAGE_CACHE;
            }
            else
            {
                // this is not a cross buffer write
#ifdef HOST_NVME
                if (DM_WRITE_ZERO == pSubCmd->pSCMD->tMA.ucOption)
                {
                    U32 ulDesAddr = COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID, TRUE, BUF_SIZE_BITS) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT << SEC_SIZE_BITS);
                    HAL_DMAESetValue(ulDesAddr, pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN << SEC_SIZE_BITS, 0);
                }
                else
 #endif
                {
                    pSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID)].usStartHWID), pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, pSubCmd->pSCMD->tMA.ucSecLen) - OTFB_START_ADDRESS;
                }
 
                if(L1_BufferGetWritePointer(LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID)) == LPN_PER_BUF)
                {
                    L1_ReleaseCacheLine(pSubCmd->SubCmdAddInfo.ucPuNum);
                }
            }

            // the process of partial write hit is completed
            gPartialHitFlag = PARTIAL_HIT_NONE;
        }
        break;

        default:
        {
            DBG_Printf("L1_HandleWritePartialHit switch case error\n");
            DBG_Getch();
        }
    } //switch(gPartialHitFlag)

    // return TRUE only if the process of write partial hit is completed
    return (gPartialHitFlag == PARTIAL_HIT_NONE) ? TRUE : FALSE;
}

#if 0
/****************************************************************************
Function  : L1_HandleWriteFullHitRecycle
Input     : 
Output    : TRUE, process finished; FALSE, process ongoing
Return    : 
Purpose   : handle write full hit recycle
Reference :
Modification History:
20121129   Blake Zhang  first create function
****************************************************************************/
BOOL L1_HandleWriteFullHitRecycle(SUBCMD* pSubCmd)
{
    U8  ucLoop;
    U8  ucLoopBitmap;
    U8  ucUseTempBuf;
    U8  ucLPNCount;
    U8  ucLPNOffset;
    U8  ucLPNOffsetIN;
    U8  ucCacheOffset;
    U8  ucCurrLpnBitmap;
    U8  ucCacheFreeStatus;
    U16 usLogBufID;
    U16 usPhyBufID;
    U16 usHitPhyBufID;
    U16 usHitLogBufID;
    U32 ulStartLPN;
    U32 ulSrcBufAddr;
    U32 ulDesBufAddr;
    U32 ulTmpBufAddr;

    ucLPNCount        = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
    ucLPNOffsetIN     = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
    ucCacheFreeStatus = L1_CacheGetTagStatus(pSubCmd->SubCmdAddInfo.ucPuNum, ucLPNCount);
    
    /* no enough space in cacheline, release the cacheline and processed as no buffer */
    if (L1_STATUS_CACHE_NO_SPACE == ucCacheFreeStatus)
    {
        /* release current cacheline */
        L1_ReleaseCacheLine(pSubCmd->SubCmdAddInfo.ucPuNum);
        ucCacheFreeStatus = L1_STATUS_CACHE_NO_BUFF;
    }

    if (L1_STATUS_CACHE_NO_BUFF == ucCacheFreeStatus)
    {
        usPhyBufID = L1_AllocateWriteBuf(pSubCmd->SubCmdAddInfo.ucPuNum);

        /* No free write buffer, need to wait */
        if(INVALID_4F == usPhyBufID)
        {
            L1_ProcessPendingTrim();
            g_ucPartialHitNeedNewWriteCache = TRUE;
            return FALSE;
        }

        /* attach new buffer to cache */
        L1_CacheAttachBuffer(pSubCmd->SubCmdAddInfo.ucPuNum, usPhyBufID);
        gpBufTag[LGCBUFID_FROM_PHYBUFID(usPhyBufID)].Stage = BUF_STAGE_CACHE;
    }
    else
    {
        usPhyBufID = g_aCacheLine[(U16)pSubCmd->SubCmdAddInfo.ucPuNum];
    }

    g_ucPartialHitNeedNewWriteCache = FALSE;

    /* hit buffer infos */
    usHitPhyBufID = pSubCmd->SubCmdPhyBufferID;
    usHitLogBufID = LGCBUFID_FROM_PHYBUFID(usHitPhyBufID);
    ucCacheOffset = pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT;

    // check if the hit buffer is a prefetch buffer, if it is,
    // we have to check if the data is ready
    if(gpBufTag[usHitLogBufID].Stage == BUF_STAGE_CACHE && gpBufTag[usHitLogBufID].bPrefetchBuf == TRUE)
    {
        if (L1_CheckCacheStatusLinkIdle(&gpBufTag[usHitLogBufID].usStartNRID == BUF_BUSY)
        {
            return FALSE;
        }
    }

    /* cache buffer infos */
    usLogBufID  = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    ucLPNOffset = L1_BufferGetWritePointer(usLogBufID);
    ulStartLPN  = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + ucLPNOffsetIN;

#ifdef SIM
    if (BUF_STAGE_CACHE != gpBufTag[usLogBufID].Stage)
    {
        DBG_Printf("L1_HandleWriteFullHitRecycle Buf 0x%x Stage ERROR\n", usPhyBufID);
        DBG_Getch();
    }
#endif

    /* the allocated new CacheLine buffer is the hit buffer (just freed), in this case, we use a temp buffer in data copy */
    if (BUF_STAGE_CACHE == gpBufTag[usHitLogBufID].Stage && gpBufTag[usHitLogBufID].bPrefetchBuf == FALSE)
    {
        ucUseTempBuf = TRUE;
        ulTmpBufAddr = COM_GetMemAddrByBufferID(g_L1TempBufferPhyId, TRUE, BUF_SIZE_BITS);
    }
    else
    {
        ucUseTempBuf = FALSE;
    }

    /* copy no hit sectors from hit buffer */
    ulSrcBufAddr = COM_GetMemAddrByBufferID(usHitPhyBufID, TRUE, BUF_SIZE_BITS);
    ulDesBufAddr = COM_GetMemAddrByBufferID(usPhyBufID, TRUE, BUF_SIZE_BITS);
    
    ulSrcBufAddr += (ucCacheOffset << LPN_SIZE_BITS);
    ulDesBufAddr += (ucLPNOffset << LPN_SIZE_BITS);

    for (ucLoop = 0; ucLoop < ucLPNCount; ucLoop++)
    {
        ucCurrLpnBitmap = ~(pSubCmd->LpnSectorBitmap[ucLPNOffsetIN + ucLoop]);

        if (0 == ucCurrLpnBitmap)
        {
            ulSrcBufAddr += LPN_SIZE;
            ulDesBufAddr += LPN_SIZE;
            continue;
        }

        for(ucLoopBitmap = 0; ucLoopBitmap < SEC_PER_LPN; ucLoopBitmap++)
        {
            if (0 != (ucCurrLpnBitmap & (1 << ucLoopBitmap)))
            {
                if (FALSE == ucUseTempBuf)
                {
                    HAL_DMAECopyOneBlock(ulDesBufAddr, ulSrcBufAddr, SEC_SIZE);
                }
                else
                {
                    /* use temp buffer in copy data */
                    HAL_DMAECopyOneBlock(ulTmpBufAddr, ulSrcBufAddr, SEC_SIZE);
                    HAL_DMAECopyOneBlock(ulDesBufAddr, ulTmpBufAddr, SEC_SIZE);
                }
            }

            ulSrcBufAddr += SEC_SIZE;
            ulDesBufAddr += SEC_SIZE;
        }
    }

    // for read caches and prefetch buffers, we have to
    // invalidate hit cache ids, we don't have to do this if
    // the hit buffer was a read cache and is allocated as the
    // new cache
    if (BUF_STAGE_FLUSH <= gpBufTag[usHitLogBufID].Stage || (gpBufTag[usHitLogBufID].Stage == BUF_STAGE_CACHE && gpBufTag[usHitLogBufID].bPrefetchBuf == TRUE))
    {
        U8 i;
        for(i = 0; i < ucLPNCount; i++)
        {
            L1_CacheInvalidLPNInLCT(usHitLogBufID, ucCacheOffset+i, 1);
        }
    }

    /* set read buffer sector bit map and buffer LPN and write pointer */
    for (ucLoop = 0; ucLoop < ucLPNCount; ucLoop++)
    {
        L1_SetBufferSectorBitmap(usLogBufID, (ucLPNOffset + ucLoop), INVALID_2F);
        L1_SetBufferLPN(usLogBufID, (ucLPNOffset + ucLoop), (ulStartLPN + ucLoop));
    }

    /* set Write Pointer */
    L1_BufferSetWritePointer(usLogBufID, ucLPNOffset + ucLPNCount);

    /* set SubCmd Infos */
    pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucLPNOffset << LPN_SECTOR_BIT) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & LPN_SECTOR_MSK);
    pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucLPNOffset;
    pSubCmd->SubCmdPhyBufferID = usPhyBufID;

    /* update CacheID Link */
    L1_UpdateCacheIdLink(pSubCmd);

    /* allocate cache status */
    pSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[usLogBufID].usStartHWID),
        pSubCmd->pSCMD->tMA.ulSubSysLBA, pSubCmd->pSCMD->tMA.ucSecLen) - OTFB_START_ADDRESS;

    return TRUE;
}
#endif

#if 0
/****************************************************************************
Function  : L1_HandleReadBitmapPartialHit
Input     : 
Output    : TRUE, process finished; FALSE, process ongoing
Return    : 
Purpose   : handle read partial hit, that only bitmap doesn't match
Reference :
Modification History:
20121129   Blake Zhang  first create function
****************************************************************************/
BOOL L1_HandleReadBitmapPartialHit(SUBCMD* pSubCmd)
{
    U8  ucLPNOffset;
    U16 usLogBufID;
    U16 usPhyBufID;
    U32 ulCurHitLPN;
#ifdef HOST_SATA
    U32 ulSrcBufAddr;
    U32 ulDesBufAddr;
#endif

    switch (gPartialHitFlag)
    {
        /* target write buffer and SubCmd output infos already set in Cache Search  */
        case PARTIAL_HIT_NONE:
        {
#ifdef SIM
            if (BUF_STAGE_CACHE != gpBufTag[LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID)].Stage)
            {
                DBG_Printf("L1_HandleReadBitmapPartialHit Buf 0x%x Enter Stage ERROR\n", pSubCmd->SubCmdPhyBufferID);
                DBG_Getch();
            }
#endif
          
            gPartialHitLPNCnt  = pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT;
            gPartialHitLPNBase = pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT;
            gPartialHitFlag    = PARTIAL_HIT_PHASE1;
        }
        break;

        /* PHASE1: merge read no hit part and copy hit part for each LPN */
        case PARTIAL_HIT_PHASE1:
        {
            /* check HighPrioFifo full or not */
            if (TRUE == L1_FreeReadBufReqLinkEmpty(pSubCmd->SubCmdAddInfo.ucPuNum))
            {
                /* wait for HighPrioFifo free */
                return FALSE;
            }

            if (FALSE == L1_CacheStatusRescourseCheck(L1_CS_WRITE_BUF_CHECK_CNT))
            {
                /* wait for CacheStatus recourse */
                return FALSE;
            }

            /* check current LPN sector bitmap */
            usPhyBufID  = pSubCmd->SubCmdPhyBufferID;
            usLogBufID  = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
            ucLPNOffset = gPartialHitLPNBase;
            ulCurHitLPN = L1_GetBufferLPN(usLogBufID, ucLPNOffset);

            if (INVALID_2F != L1_GetBufferSectorBitmap(usLogBufID, ucLPNOffset))
            {
                /* not all data ready in current hit buffer, merge read */
                L1_BufferMergeLpn(pSubCmd->SubCmdAddInfo.ucPuNum, usPhyBufID, ucLPNOffset);
            }

            /* check if hit range in current LPN idle */
            if (BUF_BUSY == L1_HostPartialHitCheckBusy(usLogBufID, ulCurHitLPN))
            {
                /* wait for hit buffer idle */
                return FALSE;
            }

#ifdef DATA_MONITOR_ENABLE
            FW_DataMonitorUpdateLPN(ulCurHitLPN, ucLPNOffset, COM_GetMemAddrByBufferID(usPhyBufID, TRUE, BUF_SIZE_BITS));
#endif

            gPartialHitLPNBase++;
            gPartialHitLPNCnt--;

            /* all LPN are merged */
            if (gPartialHitLPNCnt == 0)
            {
                gPartialHitFlag = PARTIAL_HIT_PHASE2;
            }
        }
        break;

        /* PHASE2: wait all merge requests done! - actually no need to wait, just in case */
        case PARTIAL_HIT_PHASE2:
        {
#ifdef SIM
            if (BUF_STAGE_CACHE != gpBufTag[LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID)].Stage)
            {
                DBG_Printf("L1_HandleReadBitmapPartialHit Buf 0x%x Stage ERROR\n", pSubCmd->SubCmdPhyBufferID);
                DBG_Getch();
            }
#endif

#ifdef HOST_SATA
            /* To prevent read first data ready set by NFC that 
                         could make Read/Write command first data ready not in sequence, 
                         then cause dead lock, in SATA mode, read full/partial will use read buffer */
            usPhyBufID = pSubCmd->SubCmdPhyBufferID;

            /* allocate read buffer */
            L1_AllocateReadBuf(pSubCmd);

            /* Copy data from hit buffer to read buffer */
            ulSrcBufAddr = COM_GetMemAddrByBufferID(usPhyBufID,TRUE,BUF_SIZE_BITS);
            ulDesBufAddr = COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID,TRUE,BUF_SIZE_BITS);

            ulSrcBufAddr += ((pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT) << SEC_SIZE_BITS);
            ulDesBufAddr += ((pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT) << SEC_SIZE_BITS);

            HAL_DMAECopyOneBlock(ulDesBufAddr, ulSrcBufAddr, (SEC_SIZE*(pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT)));
#else
            if (FALSE == L1_CacheStatusRescourseCheck(L1_CS_WRITE_BUF_CHECK_CNT))
            {
                /* wait for CacheStatus recourse */
                return FALSE;
            }

            pSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID)].usStartHRID),
                pSubCmd->pSCMD->tMA.ulSubSysLBA, pSubCmd->pSCMD->tMA.ucSecLen) - OTFB_START_ADDRESS;
#endif

            /* all data are prepared, process SATA as read Full Hit */
            gPartialHitFlag = PARTIAL_HIT_NONE;
        }
        break;

        default:
        {
#ifdef SIM
            DBG_Printf("L1_HandleReadBitmapPartialHit default ERROR!!!\n");
            DBG_Getch();
#endif
        }
    }//switch (gPartialHitFlag)

    if (gPartialHitFlag != PARTIAL_HIT_NONE)
    {
        /* process is on going, dont build read PRD */
        return FALSE;
    }

    return TRUE;
}
#endif //end of #if 0

BOOL L1_HandleReadNoHit(SUBCMD* pSubCmd)
{
    // first we have to check if the read buffer request FIFO is full before
    // trying to allocate one
    if (TRUE == L1_FreeReadBufReqLinkEmpty(pSubCmd->SubCmdAddInfo.ucPuNum))
    {
        return FALSE;
    }

#if defined(HOST_SATA)
    // for sata mode, we have to allocate a read buffer for the current read subcommand,
    // we don't have to do this in nvme mode since it will utilize OTFB when handling
    // read no hit
    L1_AllocateReadBuf(pSubCmd);
#endif

    pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
    pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
    pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
    pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;

#ifdef HOST_READ_FROM_DRAM
    // read from DRAM mode, this mode is available for nvme and ahci mode as an
    // alternative, however, it is no longer maintained and is left here for reference,
    // be cautious about this
    if(FALSE == L1_CacheStatusRescourseCheck(L1_CS_PREFETCH_BUF_CHECK_CNT))
    {
        return FALSE;
    }

    if(FAIL == L1_AllocateReadFromDRAMBuf(pSubCmd))
    {
        return FALSE;
    }
#endif

    L1_HostReadBuildBufReq(pSubCmd);

    return TRUE;
}

BOOL L1_HandleReadFullHit(SUBCMD* pSubCmd)
{
    U16 usLogicBufId;

    usLogicBufId = LGCBUFID_FROM_PHYBUFID(pSubCmd->SubCmdPhyBufferID);

    // now that we know all requested data is in the buffer,
    // before proceeding the process, we must ensure that all the
    // data in the buffer is ready
    if (BUF_BUSY == L1_HostFullHitReadCheckBusy(usLogicBufId, pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, pSubCmd->pSCMD->tMA.ucSecLen))
    {
        // for a read full hit, we only have to check for NFC read cache status and host write cache
        // status, we shouldn't check for host read cache status since it might cause a deadlock in
        // sata mode, where the hit buffer is already locked by a host read cache status and that
        // host read cache status can never be released due to SDC first data ready FIFo issue
        return FALSE;
    }
    else
    {
        if(gpBufTag[usLogicBufId].bIsCertainHit == TRUE)
        {
            gpBufTag[usLogicBufId].bIsCertainHit = FALSE;
        }

        pSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[usLogicBufId].usStartHRID), pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, pSubCmd->pSCMD->tMA.ucSecLen) - OTFB_START_ADDRESS;

#ifdef HOST_SATA
        // set the hit result to
        // L1_CACHE_SE_HIT_RD_PREFETCH so we will
        // set up cachestatus later when configuring
        // DSG
        pSubCmd->SubCmdHitResult = L1_CACHE_SE_HIT_RD_PREFETCH;

        // set the cache locked flag for the current
        // subsystem, indicating there's a write cache locked with a host read cache status
        L1_SetCacheLockedFlag();
#endif

        return TRUE;
    }
}

/****************************************************************************
Function  : L1_HandleReadPartialHit
Input     : 
Output    : TRUE, process finished; FALSE, process ongoing
Return    : 
Purpose   : handle read partial hit
Reference :
Modification History:
20121129   Blake Zhang  first create function
****************************************************************************/
#ifdef DBG_DATA_MIS_COMPARE
#define L1_DATA_MIS_SIZE  (128)
U32 MCU1_DRAM_TEXT g_aL1DataMisFull[L1_DATA_MIS_SIZE] = { 0 };
U32 MCU1_DRAM_TEXT g_aL1DataMisMerge[L1_DATA_MIS_SIZE] = { 0 };
U32 MCU1_DRAM_TEXT g_aL1DataMisLpn[L1_DATA_MIS_SIZE] = { 0 };
U32 g_L1DataMisFull = 0;
U32 g_L1DataMisMerge = 0;
U32 g_L1DataMisLpn = 0;
#endif

BOOL L1_HandleReadPartialHit(SUBCMD* pSubCmd)
{
    // Sean Gao 20150427
    // revised for the sata mode

    switch(gPartialHitFlag)
    {
        case PARTIAL_HIT_NONE:
            {          
                // the purpose of this stage is to prepare a buffer
                // for the read partial hit

                U16 usPhyBufId;
                U8 ucStartLpnOffset;

                // for SATA host interface, we have to use read
                // buffers for read partial hit instead to prevent
                // buffer allocation deadlock
                if(L1_AllocateReadBuf(pSubCmd) == INVALID_4F)
                {
                    return FALSE;
                }

                // set the physical buffer id of the read buffer
                usPhyBufId = pSubCmd->SubCmdPhyBufferID;

                // initialize read partial hit cachestatus start id
                g_usPartialReadCachestatus = INVALID_4F;

                // update global partial hit information

                // gPartialHitLPNCnt is the LPN count of the current
                // partial hit scmd
                gPartialHitLPNCnt  = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;

                // gPartialHitLPNBase is the start offset of the
                // partial hit scmd, this variable will be incremented
                // in the next stage, whenever a lpn is processed
                gPartialHitLPNBase = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;

                // determine the target buffer start lpn offset
                ucStartLpnOffset = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;

                // update subcommand information
                pSubCmd->SubCmdPhyBufferID = usPhyBufId;
                pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucStartLpnOffset;
                pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
                pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucStartLpnOffset << SEC_PER_LPN_BITS) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & SEC_PER_LPN_MSK);
                pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;

                // advance the stage
                gPartialHitFlag = PARTIAL_HIT_PHASE1;
            }
            break;

        case PARTIAL_HIT_PHASE1:
            {
                // process all lpns in the read request one by one

                U32 ulCurrentLpn;
                LCT_HIT_RESULT HitResult;
                U8 ucDesBufLpnOffset;

                // check if the high priority buffer request if full
                if (TRUE == L1_FreeReadBufReqLinkEmpty(pSubCmd->SubCmdAddInfo.ucPuNum))
                {
                    return FALSE;
                }

                // check if we have enough cachestatus nodes
                if(FALSE == L1_CacheStatusRescourseCheck(L1_CS_WRITE_BUF_CHECK_CNT))
                {
                    return FALSE;
                }

                // set the target lpn offset in the destination buffer
                ucDesBufLpnOffset = gPartialHitLPNBase;

                // calculate the current lpn
                ulCurrentLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + gPartialHitLPNBase;

                // search the current lpn
                HitResult.AsU32 = L1_CacheIDLinkSearchLPN(ulCurrentLpn, 1, FALSE);

                // process based on the search result
                if(L1_CACHE_SE_FULL_HIT == HitResult.ucHitResult)
                {
                    // for a hit lpn, first we ensure all of its data is ready
                    // in the buffer then we copy it to the target buffer

                    U32 ulSrcBufAddr;
                    U32 ulDesBufAddr;
                    U16 usHitPhyBufId;
                    U8 ucHitBufferOffset;
                    U8 ucBufferLpnSectorBitmap;

                    // acquire the hit buffer information, including its physical
                    // buffer id and hit buffer offset
                    usHitPhyBufId = PHYBUFID_FROM_LGCBUFID(HitResult.usLogBufID);
                    ucHitBufferOffset = HitResult.ucCacheOffset;

                    // get the buffer sector bitmap of the hit lpn
                    ucBufferLpnSectorBitmap = L1_GetBufferSectorBitmap(HitResult.usLogBufID, ucHitBufferOffset);

                    // check the hit lpn sector bitmap to ensure all the
                    // requested data is ready in the buffer
                    if(ucBufferLpnSectorBitmap != INVALID_2F)
                    {
                        // not all data ready, merge the lpn
                        L1_BufferMergeLpn(pSubCmd->SubCmdAddInfo.ucPuNum, usHitPhyBufId, ucHitBufferOffset);
 
                    }

                    // check if all NFC read operation on the hit lpn is
                    // complete, note that we have to check both the NFC read
                    // link and the host write link, since both operations could
                    // modify the data and thus the sector bitmap
                    if (L1_HostPartialHitCheckBusy(HitResult.usLogBufID, ucHitBufferOffset) == BUF_BUSY)
                    {
                        return FALSE;
                    }

                    // now that the data of the hit lpn is ready, we copy it
                    // from the hit buffer to the newly allocated buffer

                    // get the memory address of the source buffer
                    ulSrcBufAddr = COM_GetMemAddrByBufferID(usHitPhyBufId, TRUE, BUF_SIZE_BITS);

                    // get the memory address of the destination buffer
                    ulDesBufAddr = COM_GetMemAddrByBufferID(pSubCmd->SubCmdPhyBufferID, TRUE, BUF_SIZE_BITS);

                    // shift the address to the hit lpn
                    ulSrcBufAddr += (ucHitBufferOffset << LPN_SIZE_BITS);

                    // shift the address to the target lpn offset
                    ulDesBufAddr += (ucDesBufLpnOffset << LPN_SIZE_BITS);


                    // copy the data to the newly allocated buffer
                    HAL_DMAECopyOneBlock(ulDesBufAddr, ulSrcBufAddr, SEC_PER_LPN*SEC_SIZE);

#ifdef DATA_MONITOR_ENABLE
                    FW_DataMonitorUpdateLPN(ulCurrentLpn, ucHitBufferOffset, COM_GetMemAddrByBufferID(usHitPhyBufId, TRUE, BUF_SIZE_BITS));
#endif
                }
                else
                {
                    // the current buffer isn't in the buffer, simply issue
                    // a merge request to read it to the target buffer

                    // read buffers belong to a different system than that of
                    // write buffers, so we can't invoke L1_BufferMergeLpn,
                    // instead, we use a special function for this purpose,
                    // also note that we maintain a cachestatus link too
                    L1_BufferReadLpn(pSubCmd->SubCmdAddInfo.ucPuNum, pSubCmd->SubCmdPhyBufferID, ucDesBufLpnOffset, ulCurrentLpn, &g_usPartialReadCachestatus);
                }

                // update global partial hit information
                gPartialHitLPNBase++;
                gPartialHitLPNCnt--;

                if(gPartialHitLPNCnt == 0)
                {
                    // advance the stage
                    gPartialHitFlag = PARTIAL_HIT_PHASE2;
                }
            }
            break;

        case PARTIAL_HIT_PHASE2:
            {
                // wait until all issued merge requests are done

                if(L1_CheckCacheStatusLinkIdle(&g_usPartialReadCachestatus) == BUF_BUSY)
                {
                    return FALSE;
                }

#ifndef HOST_SATA
                // only nvme mode need host read cache status
                if(L1_CacheStatusRescourseCheck(L1_CS_WRITE_BUF_CHECK_CNT) == FALSE)
                {
                    return FALSE;
                }

                // add a cachestatus node to the host read cachestatus link
                pSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(L1_GetReadBufferEntryCachestatus(pSubCmd->SubCmdPhyBufferID), pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, pSubCmd->pSCMD->tMA.ucSecLen) - OTFB_START_ADDRESS;
#endif

                // the data is ready in the buffer, the partial hit process is
                // complete, reset the stage
                gPartialHitFlag = PARTIAL_HIT_NONE;
            }
            break;

        default:
            {
                DBG_Printf("read partial hit error\n");
                DBG_Getch();
            }
    } // switch(gPartialHitFlag)

    // return TRUE only if the read partial hit process is completed
    if(gPartialHitFlag != PARTIAL_HIT_NONE)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/****************************************************************************
Function  : L1_BufferManagement
Input     : none
Output    : none
Return    : 
Purpose   : recycle buffer,allocate rand buffer and sequcence buffer to schedule
Reference :
SUBCMD buffer service.
The schedule always keep one rand buffer and sequence buffer.

Modification History:
20121121   Blake Zhang  modify for ramdisk design
20120209   Brooke Wang create detailed code
20120118   peterxiu   001 first create function
****************************************************************************/
BOOL L1_BufferManagement(void)
{
    U8 ucPuNum;

    ucPuNum = g_ucCurrRecyclePU;

    if ((0 != g_WriteBufInfo[ucPuNum].ucFlushOngoingBufCnt)
        || (L1_LAST_RECYCLE_BUFFER_PER_PU < g_WriteBufInfo[ucPuNum].ucReadCacheBufCnt))
    {
        L1_RecycleWriteBuf(ucPuNum);
    }

#ifdef HOST_READ_FROM_DRAM
    L1_RecycleRFDBuf(ucPuNum);
#endif

    ucPuNum++;
    if(ucPuNum >= SUBSYSTEM_SUPERPU_NUM)
    {
        ucPuNum = 0;
    }

    g_ucCurrRecyclePU = ucPuNum;

    return TRUE;
}

#ifdef HOST_READ_FROM_DRAM
BOOL L1_RFDBufferManagment(void)
{
    U8 ucPuNum;
    BOOL bRet;

    ucPuNum = g_ucCurrRFDPU;

    bRet = L1_ResumeRFDBuf(ucPuNum);

    ucPuNum++;
    if(ucPuNum == SUBSYSTEM_SUPERPU_NUM)
    {
        ucPuNum = 0;
    }

    g_ucCurrRFDPU = ucPuNum;

    return bRet;
}
#endif

/****************************************************************************
Name        :L1_TaskCacheSearch
Input       :pSubCmd
Output      :
Author      :Blakezhang
Date        :2012.11.28
Description :cache search
Others      :
Modify      :
****************************************************************************/
BOOL L1_TaskResourceCheck(SUBCMD* pSubCmd)
{
    // Sean Gao 20150616
    // this function checks all the resources required by a subcommand,
    // the subcommand stage can only be advanced when all resources are
    // ready

    // check the subcommand stage to determine if we should proceed
    if(pSubCmd->SubCmdStage != SUBCMD_STAGE_SPLIT)
    {
        return TRUE;
    }

#ifdef HOST_SATA
    // check if there are any locked write buffers before processing
    // multi-MCU write commands to avoid possible deadlocks
    if(L1_CacheLockedCheck(pSubCmd) == FALSE)
    {
        return FALSE;
    }
#endif

    // check if we have enough cachestatus nodes, this step is actually
    // very important and can not be moved to the cache management stage,
    // otherwise the cache management stage might be interrupted due to
    // insufficient cachestatus nodes, the search result could be invalid
    // when we enter the cache management stage next time
    if(L1_CacheStatusRescourseCheck(2) == FALSE)
    {
        return FALSE;
    }

    // acquire DSG and conduct DSG management
    if(FALSE == L1_GetHostResource(pSubCmd))
    {
        return FALSE;
    }
    
    // at this point, all required resources have been checked and acquired,
    // advance the subcommand stage and proceed to the search stage
    pSubCmd->SubCmdStage = SUBCMD_STAGE_RESOURCE_CHECK;

    return TRUE;
}
extern GLOBAL BOOL g_ulReadPrefetchEnable;
BOOL L1_TaskCacheSearch(SUBCMD* pSubCmd)
{
    U8 ucLoop;
    U8 ucSubcommandType;
    U8 ucLpnOffsetInLct;
    U8 ucLpnCountInLct;
    U32 ulSubcommandStartLpn;
    LCT_HIT_RESULT HitResult;

#ifdef SIM
    // check the validity of the input subcommand
    if(pSubCmd == NULL)
    {
        DBG_Printf("L1_TaskCacheSearch invalid input\n");
        DBG_Getch();
    }
#endif

    // check if we have to process the cache search stage, cache search stage is immediately
    // preceded by the resource check stage
    if(pSubCmd->SubCmdStage != SUBCMD_STAGE_RESOURCE_CHECK)
    {
        return TRUE;
    }

    // obtain the information of the current subcommand
    ucSubcommandType = (U8)pSubCmd->pSCMD->tMA.ucOpType;
    ucLpnOffsetInLct = pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN;
    ucLpnCountInLct = pSubCmd->SubCmdAddInfo.ucSubLPNCountIN;
    ulSubcommandStartLpn = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + ucLpnOffsetInLct;

    // conducting cache search operation based on the type of the subcommand
    if(ucSubcommandType == DM_READ)
    {
#ifndef LCT_VALID_REMOVED
        // first check if the LCT we're trying to read is invalid
        if(FALSE == L1_GetLCTValid(pSubCmd->SubCmdAddInfo.ulSubCmdLCT))
        {
            // read invalid LCT, this case is treated as a read full hit in the later stages of
            // the process

            // the LCT is invalid, fill in the invalid buffer information
            pSubCmd->SubCmdPhyBufferID = g_L1InvalidBufferPhyId;
            pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN;
            pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
            pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucLpnOffsetInLct;
            pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT = ucLpnCountInLct;

            // set the subcommand search result as invalid
            pSubCmd->SubCmdHitResult = L1_CACHE_SE_RD_INVALID;
        }
        else
#endif
        {
            // the LCT is valid, conducting search in the LCT
            U8 ucCurrLpnBitmap;
            U16 usHitLogBufId;
            U16 usHitPhyBufId;
            U8 ucHitLpnOffset;

            // search the requested LPNs
            HitResult.AsU32 = L1_CacheIDLinkSearchLPN(ulSubcommandStartLpn, ucLpnCountInLct, FALSE);

            if(HitResult.ucHitResult == L1_CACHE_SE_FULL_HIT)
            {
                // search result is full hit, meaning all requested LPNs are stored sequentially in
                // write cache

                // for read operations, not only we have to make sure all requested LPNs are stored
                // sequentially in write cache, but also that all requested sectors are present
                U8 ucIsBitmapMatched = TRUE;

                // obtain hit buffer information
                usHitLogBufId = HitResult.usLogBufID;
                ucHitLpnOffset = HitResult.ucCacheOffset;
                usHitPhyBufId = PHYBUFID_FROM_LGCBUFID(usHitLogBufId);

                // check the bitmap of all LPNs
                for(ucLoop = 0; ucLoop < ucLpnCountInLct; ucLoop++)
                {
                    // get the requested LPN sector bitmap
                    ucCurrLpnBitmap = pSubCmd->LpnSectorBitmap[ucLpnOffsetInLct + ucLoop];

                    // check if the buffer has all the sectors we requested
                    if((L1_GetBufferSectorBitmap(usHitLogBufId, ucHitLpnOffset + ucLoop) & ucCurrLpnBitmap) != ucCurrLpnBitmap)
                    {
                        // the buffer doesn't have all requested sectors,
                        // return as a read partial hit
                        ucIsBitmapMatched = FALSE;
                        break;
                    }
                } // for(ucLoop = 0; ucLoop < ucLpnCountInLct; ucLoop++)

                if(ucIsBitmapMatched == TRUE)
                {
                    // read full hit

                    // fill in the hit buffer information
                    pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT = (ucHitLpnOffset << LPN_SECTOR_BIT) + (pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN & LPN_SECTOR_MSK);
                    pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT = pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN;
                    pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT = ucHitLpnOffset;
                    pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT  = ucLpnCountInLct;
                    pSubCmd->SubCmdPhyBufferID = usHitPhyBufId;

                    pSubCmd->SubCmdHitResult = L1_CACHE_SE_FULL_HIT;
                }
                else
                {
                    // read partial hit
                    pSubCmd->SubCmdHitResult = L1_CACHE_SE_PART_HIT;
                }
            } // if(HitResult.ucHitResult == L1_CACHE_SE_FULL_HIT)
            else
            {
                // search result is read no hit or read partial hit
                pSubCmd->SubCmdHitResult = HitResult.ucHitResult;
            }
        }
    }
    else if(ucSubcommandType == DM_WRITE)
    {
        // for write subcommands, just search the LCT and fill in the search result
        HitResult.AsU32 = L1_CacheIDLinkSearchLPN(ulSubcommandStartLpn, ucLpnCountInLct, FALSE);
        pSubCmd->SubCmdHitResult = HitResult.ucHitResult;
    }

    // if we reach here, we have already completed the cache search stage of the current
    // subcommand

    if(!((pSubCmd->SubCmdHitResult == L1_CACHE_SE_FULL_HIT) && (gpBufTag[HitResult.usLogBufID].bPrefetchBuf == TRUE)))
    {
        // update LCT write count if it's a write subcommand

        /*read-prefetch update fetching LCT and THS*/

#ifdef HOST_NVME
        if (g_ulReadPrefetchEnable == TRUE)
#endif
            L1_PrefetchTrendCheck(pSubCmd);
#ifndef HOST_NVME
        L1_PrefetchMarkProcessedScmd(pSubCmd->pSCMD);
#endif

        pSubCmd->SubCmdStage = SUBCMD_STAGE_SEARCH;
    }

    return TRUE;
}

/****************************************************************************
Name        :L1_TaskCacheManagement
Input       :pSubCmd
Output      :
Author      :HenryLuo
Date        :2012.02.20    18:29:53
Description :managment cache hit
Others      :
Modify      :
20121128   Blake Zhang  move cache search to a new function
20121121   Blake Zhang  modify for ramdisk design
****************************************************************************/
BOOL L1_TaskCacheManagement(SUBCMD* pSubCmd)
{
    U8  ucSubCmdRW;
    U8  ucHitResult;

#ifdef SIM
    if(pSubCmd == NULL)
    {
        DBG_Printf("L1_TaskCacheManagement invalid input\n");
        DBG_Getch();
    }
#endif

    if (pSubCmd->SubCmdStage != SUBCMD_STAGE_SEARCH)
    {
        /*subcmd already processed */
        return FALSE;
    }

    ucSubCmdRW  = pSubCmd->pSCMD->tMA.ucOpType;
    ucHitResult = pSubCmd->SubCmdHitResult;

    if ( (U8)DM_READ == ucSubCmdRW )
    {
        switch(ucHitResult)
        {
            case L1_CACHE_SE_RD_INVALID:
            {
                break;
            }

            case L1_CACHE_SE_FULL_HIT:
                {
                    if(FALSE == L1_HandleReadFullHit(pSubCmd))
                    {
                        return FALSE;
                    }
                    break;
                }

            case L1_CACHE_SE_PART_HIT:
                {
                    if(FALSE == L1_HandleReadPartialHit(pSubCmd))
                    {
                        return FALSE;
                    }
                    break;
                }

            case L1_CACHE_SE_NO_HIT:
                {
                    if(FALSE == L1_HandleReadNoHit(pSubCmd))
                    {
                        return FALSE;
                    }
                    break;
                }

            default:
                {
                    DBG_Printf("L1_TaskCacheManagement invalid hit result\n");
                    DBG_Getch();
                }
        }//switch ( ucHitResult )
    } 
    else if(DM_WRITE == ucSubCmdRW)
    {
#ifndef LCT_TRIM_REMOVED
        /* update LCT Valid and Trimed flags */
        L1_UpdateLCTValid(pSubCmd->SubCmdAddInfo.ulSubCmdLCT, pSubCmd);
#endif
      
        switch (ucHitResult)
        {
            case L1_CACHE_SE_FULL_HIT:
            case L1_CACHE_SE_PART_HIT:
                {
                    // both the write full hit and partial hit are handled with
                    // the partial hit process flow
                    if(FALSE == L1_HandleWritePartialHit(pSubCmd))
                    {
                        return FALSE;
                    }
                    break;
                }

            case L1_CACHE_SE_NO_HIT:
                {
                    if(L1_HandleWriteNoHit(pSubCmd) == FALSE)
                    {
                        return FALSE;
                    }
                    break;
                }

            default:
                {
                    DBG_Printf("L1_TaskCacheManagement invalid write hit result\n", ucHitResult);
                    DBG_Getch();
                }
        } //switch (ucHitResult)
    }

    pSubCmd->SubCmdStage = SUBCMD_STAGE_CACHE;
    return TRUE;
}

/****************************************************************************
Name        :L1_TaskRecycle
Input       :pSubCmd
Output      :void
Author      :Blakezhang
Date        :2012.11.25
Description :recycle burrent subcmd.
Others      :
Modify      :
****************************************************************************/
void L1_TaskRecycle(SUBCMD* pSubCmd)
{
#ifdef SIM
    if(pSubCmd == NULL)
    {
        /* no subcmd need to do */
        DBG_Printf("L1_TaskRecycle invalid input\n");
        DBG_Getch();
    }
#endif
  
    if (SUBCMD_STAGE_HOSTIO == pSubCmd->SubCmdStage)
    {
        if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
        {
#ifdef HOST_READ_FROM_DRAM
            if (TRUE == g_pCurSubCmd->SubCmdAddInfo.ucSyncFlag)
            {
                g_pCurSCmd = NULL;
                g_pCurSubCmd = NULL;
            }
            else
#endif
            {
              
#ifdef L1_DEBUG_TRACE
#ifndef SIM
                L1_DbgPrintSubCmd(pSubCmd);
#else
                FIRMWARE_LogInfo("MCU %d L1_TaskRecycle PU 0x%x CS 0x%x Hit %d Buf %d\n", HAL_GetMcuId(),
                    pSubCmd->SubCmdAddInfo.ucPuNum, pSubCmd->CacheStatusAddr, pSubCmd->SubCmdHitResult, pSubCmd->SubCmdPhyBufferID);
#endif

#endif

                if ((U8)DM_WRITE == pSubCmd->pSCMD->tMA.ucOpType)
                {
                    //L1_CacheUpdateWriteTime(pSubCmd);

                    /* Disable L1 Pop Scmd Timer policy for SPOR */ 
                    #if 0 
                    if ((TRUE == g_L2RebuildDC) && (TRUE == g_L1PopCmdEn))
                    {
                        g_L1PopCmdEn = FALSE;
                        HAL_StartMcuTimer(800000, NULL);
                    }
                    #endif
                }

                g_pCurSubCmd = NULL;
                L1_SCmdFinish();
            }
        }
        else
        {
            switch (L1_GetRawCacheStatus())
            {
            case SUBSYSTEM_STATUS_SUCCESS:
                L1_SetRawCacheStatus(SUBSYSTEM_STATUS_INIT);

            case SUBSYSTEM_STATUS_INIT:
                if (VIA_CMD_NULL != pSubCmd->pSCMD->ucSCmdSpecific)
                {
                    FW_ViaCmdXferDataDone(pSubCmd->pSCMD);
                }

                g_pCurSubCmd = NULL;
                L1_SCmdFinish();
                break;

            default:
                break;
            }
        }

    }

    return;
}

/****************************************************************************
Name        :L1_TaskMergeFlushManagement
Input       :
Output      :
Return      : TRUE, a request is send; FALSE, no request is send
Author      :Blakezhang
Date        :2012.11.22
Description :managment the cache merge and flush operations in PU Fifo
Others      :
Modify      :
****************************************************************************/
BOOL L1_TaskMergeFlushManagement(void)
{
    // Sean Gao 20150525
    // this function manages merge and flush operations for all PUs,
    // it returns TRUE if a buffer request is sent, FALSE otherwise

    BOOL bBufReqSent;
    U32 ulTaskBitmap;

    // first we have to perform vaious checks to determine if 
    // we should conduct merge and flush operations

    // check if there are any merge pending and flush pending buffers,
    // return immediately if there aren't 
    if((0 == g_ulMergeBufferBitmap) && (0 == g_ulFlushBufferBitmap))
    {
        return FALSE;
    }

    // check the status of the current subcommand if it isn't NULL
    if(NULL != g_pCurSubCmd && (U8)SCMD_DIRECT_MEDIA_ACCESS == g_pCurSubCmd->pSCMD->ucSCmdType)
    {
        if(g_pCurSubCmd->pSCMD->tMA.ucOpType == (U8)DM_READ)
        {
            // if there's an ongoing read subcommand, we only conduct
            // merge and flush task only if g_ucPartialHitNeedNewWriteCache is TRUE,
            // which means the current subcommand need a new cache
            if(g_ucPartialHitNeedNewWriteCache == FALSE)
            {
                return FALSE;
            }
        }
        else if(g_pCurSubCmd->pSCMD->tMA.ucOpType == (U8)DM_WRITE)
        {
            // always conduct merge and flush task while a write subcommand
            // is ongoing, the only exception is that we have a write partial
            // hit and gPartialHitFlag isn't set
            if(gPartialHitFlag != PARTIAL_HIT_NONE && g_ucPartialHitNeedNewWriteCache == FALSE)
            {
                return FALSE;
            }
        }
        else
        {
            DBG_Printf("subcommand type error\n");
            DBG_Getch();
        }
    }

    // check if we have enough cachestatus nodes
    if(FALSE == L1_CacheStatusRescourseCheck(L1_CS_WRITE_BUF_CHECK_CNT))
    {
        return FALSE;
    }

    // all checks are performed, if we still haven't returned,
    // it means we've passed all tests, we are now ready to conduct
    // merge and flush operations

    // calculate ulTaskBitmap, which indicates if a particular PU has pending
    // operations
    ulTaskBitmap = g_ulMergeBufferBitmap | g_ulFlushBufferBitmap;

    // find a PU with pending operations, starting from g_ucCurMergeFlushPu
    while(COM_BitMaskGet(ulTaskBitmap, g_ucCurMergeFlushPu) == FALSE)
    {
        g_ucCurMergeFlushPu = (g_ucCurMergeFlushPu+1 == SUBSYSTEM_SUPERPU_NUM) ? 0 : (g_ucCurMergeFlushPu+1);
    }

    // reset bBufReqSent
    bBufReqSent = FALSE;

    // conduct the merge operation for g_ucCurMergeFlushPu
    if(0 != g_WriteBufInfo[g_ucCurMergeFlushPu].ucMergePendingBufCnt)
    {
        // first we have to check if the high priority buffer request fifo
        // is full
        if (L1_FreeReadBufReqLinkEmpty(g_ucCurMergeFlushPu) == FALSE)
        {
            // try performing a merge operation
            bBufReqSent = L1_MergeWriteBuf(g_ucCurMergeFlushPu);
        }
    }

    // please note that we should only conduct flush operation for the current
    // pu if all merge pending buffers are processed
    if((bBufReqSent == FALSE) && (0 == g_WriteBufInfo[g_ucCurMergeFlushPu].ucMergePendingBufCnt) && (0 != g_WriteBufInfo[g_ucCurMergeFlushPu].ucFlushPendingBufCnt))
    {
#ifdef L1_BUFFER_REGULATION
        if(L1_LowPrioFifoFull(g_ucCurMergeFlushPu) == FALSE && L1_FlushPendingBufferRegulation(g_ucCurMergeFlushPu) == TRUE)
#else
        if(L1_LowPrioFifoFull(g_ucCurMergeFlushPu) == FALSE)
#endif
        {
            bBufReqSent = L1_FlushWriteBuf(g_ucCurMergeFlushPu);
        }
    }

    // update g_ucCurMergeFlushPu
    g_ucCurMergeFlushPu = (g_ucCurMergeFlushPu+1 == SUBSYSTEM_SUPERPU_NUM) ? 0 : (g_ucCurMergeFlushPu+1);

    return bBufReqSent;
}

/**********************************************************************
*Description:Copy L0_SUBCMD from L0-L1 interface,fill SUBCMD
*Input: BOOL MCUID -- 0:MCU 1
*                     1:MCU 2
*Output:
*Return:SUBCMD * -- pointer to new SubCmd filled
***********************************************************************/
SUBCMD *L1_GetNewSubCmd(SCMD* pSCMD)
{
    U8 ucDWOffset;
    U8 ucVirlSubCmdLen;
    U32 ulStartLBA;
    SUBCMD *pSubCmd; 
    SUBCMD_ADD_INFO *pAddrInfo;

#if 1
    pSubCmd = &gpSubCmdEntry->tSubCmd[0];
#else
    pSubCmd = &gpSubCmdEntry->tSubCmd[l_ucSubCmdWr];

    l_ucSubCmdWr = (l_ucSubCmdWr + 1) % SUBCMD_ENTRY_DEPTH;
#endif

    /*fill AddrInfo*/
    if ((U8)SCMD_DIRECT_MEDIA_ACCESS == pSCMD->ucSCmdType)
    {      
        pSubCmd->pSCMD = pSCMD;

#ifdef L1_DEBUG_TRACE
#ifndef SIM
        L1_DbgPrintSCMD(pSCMD);
#else
        FIRMWARE_LogInfo("MCU %d L1_GetNewSubCmd Tag 0x%x LBA 0x%x Len 0x%x F %d L %d RW %d\n", HAL_GetMcuId(),
            pSCMD->ucSlotNum, pSCMD->tMA.ulSubSysLBA, pSCMD->tMA.ucSecLen, pSCMD->tMA.ucFirst, pSCMD->tMA.ucLast, pSCMD->tMA.ucOpType);
#endif
#endif

        ulStartLBA = pSCMD->tMA.ulSubSysLBA;

        pAddrInfo = &(pSubCmd->SubCmdAddInfo);

        pAddrInfo->ucPuNum = L1_GetSuperPuFromLCT(LCTINDEX_FROM_LBA(ulStartLBA));

        pAddrInfo->ulSubCmdLCT = LCTINDEX_FROM_LBA(ulStartLBA);  //aligned by buffer size
        pAddrInfo->ucSubCmdOffsetIN = SUBCMD_OFFSET(ulStartLBA);  //sector offset
        pAddrInfo->ucSubCmdlengthIN = pSCMD->tMA.ucSecLen;
        pAddrInfo->ucSubLPNOffsetIN = SUBCMDLPN_OFFSET(pAddrInfo->ucSubCmdOffsetIN);

        ucVirlSubCmdLen = SUBCMD_OFFSET_IN_LPN(pAddrInfo->ucSubCmdOffsetIN) + pAddrInfo->ucSubCmdlengthIN;
        pAddrInfo->ucSubLPNCountIN = SUBCMDLPN_COUNT(ucVirlSubCmdLen);

#ifdef HOST_READ_FROM_DRAM
        pAddrInfo->ucSyncFlag  = 0;
        pAddrInfo->ucSyncIndex = 0; 
#endif

        L1_SubCmdGetLpnSectorMap(pAddrInfo->ucSubCmdOffsetIN, 
            pAddrInfo->ucSubCmdlengthIN, &(pSubCmd->LpnSectorBitmap[0]));

        pSubCmd->SubCmdStage = SUBCMD_STAGE_SPLIT;
    }
    else
    {
        /*clear Raw Data SubCmd*/
        for (ucDWOffset = 0; ucDWOffset < SUBCMD_SIZE_DW; ucDWOffset++)
        {
            *((U32*)(pSubCmd) + ucDWOffset) = 0;
        }

        pSubCmd->pSCMD = pSCMD;

        if (0 == pSCMD->tRawData.ucSecLen)
        {
            DBG_Printf("MCU %d L1_GetNewSubCmd tRawData.ucSecLen ERROR\n", HAL_GetMcuId());
            DBG_Getch();
        }

        L1_SetRawCacheStatus(SUBSYSTEM_STATUS_PENDING);
        pSubCmd->CacheStatusAddr = L1_GetRawCacheStatusAddr() - OTFB_START_ADDRESS;
        pSubCmd->SubCmdStage = SUBCMD_STAGE_CACHE;

        /* Trace Log SCMD special process */
        if (VIA_CMD_NULL != pSCMD->ucSCmdSpecific)
        {
            FW_ViaCmdPrepareData(pSCMD);
        }
    }

    /* Init SubCmd Host Info after get new SubCmd */
    L1_InitSubCmdHostInfo(pSubCmd);

    return pSubCmd;
}

/**********************************************************************
*Description:Build DRQ/DWQ for SubCmd
*Input
*Output
*Return:
***********************************************************************/
BOOL L1_TaskHostIO(SUBCMD* pSubCmd)
{ 
#ifdef SIM
    if(pSubCmd == NULL)
    {
        /* no subcmd need to do */
        DBG_Printf("L1_TaskHostIO invalid input\n");
        DBG_Getch();
    }
#endif

    if (pSubCmd->SubCmdStage != SUBCMD_STAGE_CACHE)
    {
        /* wait for cache management done  */
        return FALSE;
    }
    else if (L1_ERRORHANDLE_WAIT_SUBCMD == g_L1ErrorHandleStatus)
    {
        /* Avoids blocking error handling */
        pSubCmd->SubCmdStage = SUBCMD_STAGE_HOSTIO;
        return TRUE;
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

/****************************************************************************
Function  : L1_TaskFlushCacheLine
Input     : 
Output    : 
Return    : 
Purpose   : 
force flush a cacheline after 16 write buffer flushed in a PU
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
BOOL L1_TaskFlushCacheLine(void)
{
    U8 ucPuNum;
    
    ucPuNum = g_ucCurrFlushCachePU;
    
    if (INVALID_4F != g_aCacheLine[ucPuNum])
    {
        if (L1_CacheCheckTimePass(ucPuNum) > L1_FLUSH_CACHE_THRESHOLD)
        {
            L1_ReleaseCacheLine(ucPuNum);
        }
    }

    ucPuNum++;
    if(ucPuNum == SUBSYSTEM_SUPERPU_NUM)
    {
        ucPuNum = 0;
    }
    
    g_ucCurrFlushCachePU = ucPuNum;
    
    return TRUE;
}

/****************************************************************************
Function  : L1_TaskSystemIdle
Input     : 
Output    : 
Return    : 
Purpose   : 
system idle L1 main entry
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
BOOL MCU1_DRAM_TEXT L1_TaskSystemIdle(void)
{
    BOOL bRet;
  
    if (TRUE == gbGlobalInfoSaveFlag)
    {
        /* set L2 saveRT event */
        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVERT);

        gbGlobalInfoSaveFlag = FALSE;

        bRet = TRUE;
    }
#ifndef LCT_TRIM_REMOVED
    else if (g_ulTrimSearchRangeStartLct < g_ulTrimSearchRangeEndLct)
    {
        /* Idle Trim Processing */
        L1_ProcessPendingTrimBulk(L1_TRIM_IDLE_MAX_LCT_CNT, L1_TRIM_FLUSH_MAX_LOOP_CNT);
        bRet = TRUE;
    }
#endif
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL MCU1_DRAM_TEXT L1_IsRFDIdle(void)
{
#ifdef HOST_READ_FROM_DRAM
    U8 ucPuNum;
  
    /* wait for all read from DRAM buffer idle */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        if (FALSE == L1_IsRFDBufferEmpty(ucPuNum))
        {
            return FALSE;
        }
    }
#endif

    return TRUE;
}

extern GLOBAL  U32  g_ulRdPreFetchTHS;
extern BOOL L2_IsBootupOK(void);
void L1_TaskInternalIdle(void)
{
    if (TRUE == g_bL1BackgroundTaskEnable)
    {
#ifdef HOST_NVME
        if (g_ulReadPrefetchEnable == TRUE)
#endif
        {
            if (TRUE == L2_IsBootupOK())
            {
                L1_TaskPrefetch();
            }
        }

#ifndef LCT_TRIM_REMOVED
        if((!(g_ulRdPreFetchTHS >= L1_RD_PREFETCH_THS)) && (g_ulReadBufReqPuBitMap == 0 && g_ulWriteBufReqPuBitMap == 0) && (L1_IsSCQEmpty() == TRUE) && ((g_ulMergeBufferBitmap | g_ulFlushBufferBitmap) == 0))
        {
            L1_ProcessPendingTrim();
        }
#endif
    }

    return;
}

BOOL MCU1_DRAM_TEXT L1_IsIdle(void)
{
    U8 ucPuNum;

    /* wait for all SCMD and SubCmd finished */
    if(NULL != g_pCurSubCmd)
    {
        return FALSE;
    }

    /* wait for all write buffer idle */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        if (0 != g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt)
        {
            return FALSE;
        }
    }

    /* wait for all BufReq sent */
    if (FALSE == L1_BufReqEmpty())
    {
        return FALSE;
    }

#ifdef HOST_NVME
    if(L1_ReadBufferManagement() == TRUE)
    {
        return FALSE;
    }
#endif

    /* wait for all cache status idle */
    if (FALSE == L1_CacheStatusWaitIdle())
    {
        return FALSE;
    }

    return TRUE;
}

/****************************************************************************
Function  : L1_TaskNormalPCCheckBusy
Input     : 
Output    : 
Return    : 
Purpose   : 
check L1 busy before normal power-off
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
U32 MCU1_DRAM_TEXT L1_TaskNormalPCCheckBusy(void)
{
    U8 ucPuNum;

    if (NULL == g_pCurSCmd)
    {
        DBG_Printf("L1_TaskNormalPCCheckBusy g_pCurSCmd ERROR!\n");
        DBG_Getch();
    }

    /* wait for all SCMD and SubCmd finished */
    if((NULL == g_pCurSubCmd) && ((U8)SCMD_POWERCYCLING == g_pCurSCmd->ucSCmdType))
    {
        ;
    }
    else
    {
        return TRUE;
    }

    /* wait for all write buffer idle */
    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        if (0 != g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt)
        {
            return TRUE;
        }
    }

    /* wait for all BufReq sent */
    if (FALSE == L1_BufReqEmpty())
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Function  : L1_ErrorHandleClearSCQ
Input     : 
Output    : 
Return    : 
Purpose   : 
init all L1 data structures after received COMRESET/SoftReset
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void MCU1_DRAM_TEXT L1_ErrorHandleClearSCQ(void)
{
    /* clear SCMD Queue */
    while (TRUE)
    {
        g_pCurSCmd = L1_GetSCmd();

        if (NULL != g_pCurSCmd)
        {
            L1_SCmdFinish();
        }
        else
        {
            break;
        }
    }

    return;
}

/****************************************************************************
Function  : L1_TaskErrorHandle
Input     : 
Output    : 
Return    : 
Purpose   : 
init all L1 data structures after received COMRESET/SoftReset
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
BOOL MCU1_DRAM_TEXT L1_TaskErrorHandle(void)
{
    BOOL bRet;

    bRet = FALSE;
  
    switch (g_L1ErrorHandleStatus)
    {
        case L1_ERRORHANDLE_INIT:
        {
            FW_DbgShowAll();
            DBG_Printf("MCU %d L1 ErrorHandle start\n", HAL_GetMcuId());
            g_L1ErrorHandleStatus = L1_ERRORHANDLE_WAIT_SUBCMD;
        }
        break;
    
        case L1_ERRORHANDLE_WAIT_SUBCMD:
        {
            /* clear Host Read/Write CacheStatus to make current pending command finish */
            L1_CacheStatusClearHostPending();
          
            if ((NULL == g_pCurSCmd) && (NULL == g_pCurSubCmd))
            {
                g_bL1BackgroundTaskEnable = TRUE;
                L1_RdPreFetchResetThs();
                L1_ReleaseAllWriteCache();
                L1_ErrorHandleClearSCQ();
#ifdef HOST_READ_FROM_DRAM
                L1_RFDBufferReset();
#endif
                g_L1ErrorHandleStatus = L1_ERRORHANDLE_WAIT_IDLE;
            }
        }
        break;

        case L1_ERRORHANDLE_WAIT_IDLE:
        {
            if (TRUE == L1_IsIdle())
            {
                g_L1ErrorHandleStatus = L1_ERRORHANDLE_WARM_INIT;
            }
        }
        break;

        case L1_ERRORHANDLE_WARM_INIT:
        {
            /* all pending command processed, start warm init */
            L1_HostCMDProcWarmInit();
            L1_BufferWarmInit();
            L1_CacheStatusWarmInit();
            L1_CacheWarmInit();
            L1_BufReqWarmInit();
            L1_RdPreFetchWarmInit();
#ifdef HOST_SATA
            L1_InitCacheLockedFlag();
#endif
        
            g_L1ErrorHandleStatus = L1_ERRORHANDLE_DONE;
        }
        break;

        case L1_ERRORHANDLE_DONE:
        {
            g_L1ErrorHandleStatus = L1_ERRORHANDLE_INIT;
            bRet = TRUE;
        }
        break;
    
        default:
        {
            DBG_Printf("L1_TaskErrorHandle g_L1ErrorHandleStatus 0x%x ERROR!\n", g_L1ErrorHandleStatus);
            DBG_Getch();
        }
        break;
    }

    return bRet;
}

#ifdef HOST_SATA
// the following section of code is dedicated to fix the
// deadlock caused by not copying data to read buffers when
// conducting read operations
//
// the issue:
// 
// suppose there're 2 host commands, 1 read and 1 write
//
// the first scmd of the read command is a no hit, while the
// rest of them are full hits, locking caches
//
// the first scmd of the write command is a full hit, it sets
// first data ready, and suppose the first data ready of the
// read command is not yet set, the SDC picks the write command
//
// however, if one of the scmds of the write command can't
// allocate write cache due to locked caches(for example, the
// locked cache is the first one of the read cache link),
// deadlock then happens
//
// the solution:
//
// postpone the execution of write commands, if there're caches
// locked by read operations
//
// please note:
// this mechanism alone can't prevent the deadlock, L0 also has
// to enforce certain split rules(to be exact, before L0 splits
// a multi-MCU write command, it has to wait until all subcommand
// queues are empty, so that we can be sure that all cache locked
// flags that should be set is already set properly before
// starting processing write subcommands)

BOOL L1_GetCacheLockedFlag(void)
{
    // 20150423 Sean Gao
    // this function returns the flag indicating
    // if there're caches locked by read subcommands
    // in subsystems

    U8 i;

    // initialize the pointer to cache locked flags
    U8* pucCacheLocked = (U8*)SATA_CACHE_LOCKED_INFO;
    
    // initialize the return value to FALSE
    BOOL bCacheLocked = FALSE;

    // return TRUE if a flag has been set by one of the
    // subsystems
    for(i = 0; i < l_ucSubsystemNum; i++)
    {
        if(pucCacheLocked[i] == TRUE)
        {
            bCacheLocked = TRUE;
            break;
        }
    }

    // return the result
    return bCacheLocked;
}

BOOL L1_GetMcuCacheLockedFlag(void)
{
    // 20150423 Sean Gao
    // this function returns the flag indicating
    // if there're caches locked by read subcommands
    // in a particular subsystem

    return (*l_pucCacheLockedFlag);
}

void L1_InitCacheLockedFlag(void)
{
    // 20150423 Sean Gao
    // this function initializes all variables related
    // to the cache locked flag

    U8* pucCacheLockedFlag;

    // set the pointer to the cache locked flag memory area
    pucCacheLockedFlag = (U8*)SATA_CACHE_LOCKED_INFO;

    // set the number of subsystems
    l_ucSubsystemNum = DiskConfig_GetSubSysNum();

    // set the pointer to the cache locked flag for the current
    // subsystem
    if(HAL_GetMcuId() == MCU1_ID)
    {
        l_pucCacheLockedFlag = pucCacheLockedFlag;
    }
    else if(HAL_GetMcuId() == MCU2_ID)
    {
        l_pucCacheLockedFlag = pucCacheLockedFlag + 1;
    }
    else
    {
        DBG_Printf("mcu id error\n");
        DBG_Getch();
    }

    // initialize the cache locked flag of the current subsystem
    // as FALSE
    *l_pucCacheLockedFlag = FALSE;

    return;
}

void L1_SetCacheLockedFlag(void)
{
    // 20150423 Sean Gao
    // this function sets the cache locked flag for the
    // current subsystem

    *l_pucCacheLockedFlag = TRUE;

    return;
}

void L1_ClearCacheLockedFlag(void)
{
    // 20150423 Sean Gao
    // this function clears the cache locked flag for the
    // current subsystem

#ifdef SIM
    if(*l_pucCacheLockedFlag == FALSE)
    {
        DBG_Printf("cache locked flag error\n");
        DBG_Getch();
    }
#endif

    *l_pucCacheLockedFlag = FALSE;

    return;
}

BOOL L1_CacheLockedCheck(SUBCMD* pSubCmd)
{
    // Sean Gao 20150421
    // this function checks if the cache locked flag is
    // set, if it is, we have to postpone all multi-MCU write
    // commands to prevent deadlocks

    BOOL bResult = TRUE;

    if(pSubCmd->pSCMD->tMA.ucOpType == DM_WRITE && !(pSubCmd->pSCMD->tMA.ucFirst && pSubCmd->pSCMD->tMA.ucLast) && L1_GetCacheLockedFlag() == TRUE)
    {
        // check if any of the cache locked flags are set,
        // please note that due to the rule enforced by L0,
        // by the time we're processing subcommands of
        // this multi-MCU write
        // command, all subcommands preceding the current
        // multi-MCU write command must already have been
        // executed, so all cache locked flags must be set
        // by now(if they need to be set)
        if(L1_GetMcuCacheLockedFlag() == TRUE)
        {
            // the cache locked flag of the current subsystem is
            // set, we have to check pending host read
            // cachestatus link of all caches

            U16 i;
            U16 usLogBufId;
            U8 ucAllHostReadCachestatusCleared;

            // get the first logical buffer id
            usLogBufId = LGCBUFID_FROM_PHYBUFID(g_ulWriteBufStartPhyId);

            // initialize ucAllHostReadCachestatusCleared to TRUE
            ucAllHostReadCachestatusCleared = TRUE;

            // for all caches, check their host read cachestatus
            // link, ensure they are all empty
            for(i = 0; i < L1_BUFFER_COUNT_WRITE; i++, usLogBufId++)
            {
                // get the buffer tag of the current logical
                // buffer
                BUFF_TAG* pBufTag = &gpBufTag[usLogBufId];
                if(L1_CheckCacheStatusLinkIdle(&(pBufTag->usStartHRID)) == BUF_BUSY)
                {
                    // the host read cachestatus link of a cache
                    // hasn't been cleared
                    ucAllHostReadCachestatusCleared = FALSE;

                    break;
                }
            }

            // check if all host read cachestatus links have been
            // cleared
            if(ucAllHostReadCachestatusCleared == TRUE)
            {
                // there are no pending host read cachestatus
                // links, clear the flag and proceed processing
                // the current subcommand
                L1_ClearCacheLockedFlag();

                // check all cache locked flags again
                if(L1_GetCacheLockedFlag() == TRUE)
                {
                    // the cache locked flag of another MCU is
                    // still set, return FALSE to indicate a 
                    // failed test
                    bResult = FALSE;
                }
                else
                {
                    bResult = TRUE;
                }
            }
            else
            {
                // there are pending host read cachestatus links,
                // postpone the execution of the current write
                // subcommand
                bResult = FALSE;
            }
        } // if(L1_GetMcuCacheLockedFlag() == TRUE)
        else
        {
            // the cache locked flag of another MCU is
            // still set, return FALSE to indicate a 
            // failed test
            bResult = FALSE;
        }
    } // if(pSubCmd->pSCMD->tMA.ucOpType == DM_WRITE && !(pSubCmd->pSCMD->tMA.ucFirst && pSubCmd->pSCMD->tMA.ucLast) && L1_GetCacheLockedFlag() == TRUE)

    return bResult;
}
#endif

/********************** FILE END ***************/

