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
Filename    : L1_RdPreFetch.c
Version     :
Author      :
Date        :
Description :L1 read prefetch
Others      :
Modify      :
****************************************************************************/
//#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "L2_FCMDQ.h"
#include "COM_Memory.h"
#include "COM_BitMask.h"
#include "HAL_Xtensa.h"

GLOBAL  U32  g_ulRdPreFetchPreLCT;
GLOBAL  U32  g_ulRdPreFetchTHS;
GLOBAL  U32  g_ulPendingPrefetchLctInit; 
GLOBAL  U32  g_ulPendingPrefetchLctBitmap;

void L1_RdPreFetchResetThs(void)
{
    g_ulRdPreFetchPreLCT = INVALID_8F;
    g_ulRdPreFetchTHS    = 0;

    g_ulPendingPrefetchLctBitmap = g_ulPendingPrefetchLctInit;

    return;
}

/****************************************************************************
Function  : L1_RdPreFetchInit
Input     : 
Output    : 
Return    : 
Purpose   : 
init data structure for L1 read pre-fetch
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void MCU1_DRAM_TEXT L1_RdPreFetchInit(void)
{
    U8 i;

    // initialize all pu bitmap
    g_ulPendingPrefetchLctInit = 0;

    // set the prefetch LCT index bitmap, note that we start from the leftmost
    // bit of the bitmap instead of the rightmost bit, we do so since the hardware
    // SCLZ function treats the lefmost bit as offset 0
    for(i = 0; i < L1_PREFETCH_LCT_NUM; i++)
    {
        COM_BitMaskSet(&g_ulPendingPrefetchLctInit, 31-i);
    }

    L1_RdPreFetchResetThs();

    return;
}

void MCU1_DRAM_TEXT L1_RdPreFetchWarmInit(void)
{
    U8 i;

    // initialize all pu bitmap
    g_ulPendingPrefetchLctInit = 0;

    // set the prefetch LCT index bitmap, note that we start from the leftmost
    // bit of the bitmap instead of the rightmost bit, we do so since the hardware
    // SCLZ function treats the lefmost bit as offset 0
    for(i = 0; i < L1_PREFETCH_LCT_NUM; i++)
    {
        COM_BitMaskSet(&g_ulPendingPrefetchLctInit, 31-i);
    }

    L1_RdPreFetchResetThs();

    return;
}

/****************************************************************************
Function  : L1_PrefetchTrendCheck
Input     : 
Output    : 
Return    : 
Purpose   : 
check LCT if SeqRead command received
Reference :
Modification History:
20131223   Blake Zhang create
****************************************************************************/
void L1_PrefetchMarkInspectedScmd(SCMD* pScmd)
{
    // Sean Gao 20150812
    // this function marks the subcommand as inspected by the prefetch mechanism
    pScmd->tMA.ucOption = pScmd->tMA.ucOption | (1 << 0);

    return;
}

BOOL L1_PrefetchIsScmdInspected(SCMD* pScmd)
{
    // Sean Gao 20150812
    // this function checks if the subcommand is inspected by the prefetch mechanism
    return ((pScmd->tMA.ucOption & (1 << 0)) != 0) ? TRUE : FALSE;
}

void L1_PrefetchMarkProcessedScmd(SCMD* pScmd)
{
    // Sean Gao 20150812
    // this function marks the subcommand as processed by the prefetch mechanism
    pScmd->tMA.ucOption = pScmd->tMA.ucOption | (1 << 1);

    return;
}

BOOL L1_PrefetchIsScmdProcessed(SCMD* pScmd)
{
    // Sean Gao 20150812
    // this function checks if the subcommand is processed by the prefetch mechanism
    return ((pScmd->tMA.ucOption & (1 << 1)) != 0) ? TRUE : FALSE;
}

BOOL L1_CheckSequentialTrend(U32 ulCurrentLct)
{
    // Sean Gao 20150812
    // this function checks if the incoming lct is within a certain range of the last
    // lct, if it is, we consider it as sequential

    if(((g_ulRdPreFetchPreLCT - 1) <= ulCurrentLct) && ((g_ulRdPreFetchPreLCT + 1) >= ulCurrentLct))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

void L1_PrefetchTrendCheck(SUBCMD* pSubCmd)
{
    // Sean Gao 20150309
    // this function check the access pattern of subcommands to
    // determine if there's a sequential read trend

    U32 ulCurLct;

#ifndef HOST_NVME
    // process the subcommand only if it hasn't been inspected yet
    if(L1_PrefetchIsScmdInspected(pSubCmd->pSCMD) == FALSE)
#endif
    {
        if((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
        {
            if(((U8)DM_READ == pSubCmd->pSCMD->tMA.ucOpType) && (TRUE == pSubCmd->pSCMD->tMA.ucIsNCQ)) 
            {
                // get the current lct
                ulCurLct = pSubCmd->SubCmdAddInfo.ulSubCmdLCT;

                // check if the current lct and the last accessed lct
                // are sequential
                if(L1_CheckSequentialTrend(ulCurLct) == TRUE)
                {
                    // increase the sequential trend
                    g_ulRdPreFetchTHS++;

                    // update the last accessed lct
                    g_ulRdPreFetchPreLCT = (ulCurLct > g_ulRdPreFetchPreLCT) ? ulCurLct : g_ulRdPreFetchPreLCT;
                }
                else
                {
                    // there is no sequential trend, reset the sequential trend
                    g_ulRdPreFetchTHS = 0;
                    g_ulRdPreFetchPreLCT = ulCurLct;
                }

                g_ulPendingPrefetchLctBitmap = g_ulPendingPrefetchLctInit;
            }
            else
            {
                // the current subcommand is not a ncq read subcommand,
                // no sequential trend present, reset all parameters
                g_ulRdPreFetchTHS = 0;
                g_ulRdPreFetchPreLCT = INVALID_8F;
                g_ulPendingPrefetchLctBitmap = g_ulPendingPrefetchLctInit;
            }

#ifndef HOST_NVME
            // mark the subcommand as inspected be the prefetch mechanism
            L1_PrefetchMarkInspectedScmd(pSubCmd->pSCMD);
#endif
        } // if((U8)SCMD_DIRECT_MEDIA_ACCESS == pSubCmd->pSCMD->ucSCmdType)
    } // if(L1_PrefetchIsScmdInspected(pSubCmd->pSCMD) == FALSE)

    return;
}

void L1_PrefetchTrendCheckScmd(SCMD* pScmd)
{
    // Sean Gao 20150812
    // this function check the access pattern of subcommands to
    // determine if there's a sequential read trend

    U32 ulCurLct;

    if(L1_PrefetchIsScmdInspected(pScmd) == FALSE)
    {
        if((U8)SCMD_DIRECT_MEDIA_ACCESS == pScmd->ucSCmdType)
        {
            if(((U8)DM_READ == pScmd->tMA.ucOpType) && (TRUE == pScmd->tMA.ucIsNCQ)) 
            {
                // get the current lct
                ulCurLct = LCTINDEX_FROM_LBA(pScmd->tMA.ulSubSysLBA);

                // check if the current lct and the last accessed lct
                // are sequential
                if(L1_CheckSequentialTrend(ulCurLct) == TRUE)
                {
                    // increase the sequential trend
                    g_ulRdPreFetchTHS++;

                    // update the last accessed lct
                    g_ulRdPreFetchPreLCT = (ulCurLct > g_ulRdPreFetchPreLCT) ? ulCurLct : g_ulRdPreFetchPreLCT;
                }
                else
                {
                    // reset the sequential trend
                    g_ulRdPreFetchTHS = 0;
                    g_ulRdPreFetchPreLCT = ulCurLct;
                }

                g_ulPendingPrefetchLctBitmap = g_ulPendingPrefetchLctInit;
            }
            else
            {
                // no sequential trend present, reset all parameters
                g_ulRdPreFetchTHS = 0;
                g_ulRdPreFetchPreLCT = INVALID_8F;
                g_ulPendingPrefetchLctBitmap = g_ulPendingPrefetchLctInit;
            }

            // mark the subcommand as inspected by the prefetch mechanism
            L1_PrefetchMarkInspectedScmd(pScmd);
        }
        else
        {
            // not even a media access command, mark as inspected and processed
            L1_PrefetchMarkInspectedScmd(pScmd);
            L1_PrefetchMarkProcessedScmd(pScmd);
        }
    }

    return;
}

/****************************************************************************
Function  : L1_ReadPreFetchBuildBufReq
Input     : U32 ulPreFetchLBA
Output    :
Return    : 
Purpose   : set buffer REQ
Reference :
Build buffer request for host read pre fetch
****************************************************************************/
void L1_ReadPreFetchBuildBufReq(U8 ucPuNum, U32 ulPrefetchLct, U16 usPhyBufID)
{
    // 20150420 Sean Gao
    // this function issues a prefetch read buffer request

    U8 i;
    U8 ucBufReqID;
    BUF_REQ_READ* pBufReq;

#ifdef SIM
    // double check the buffer request fifo
    if (L1_FreeReadBufReqLinkEmpty(ucPuNum) == TRUE)
    {
        DBG_Printf("high priority buffer request fifo full\n");
        DBG_Getch();
    }
#endif

    // get an empty buffer request from the fifo
    ucBufReqID = L1_AllocateReadBufReq(ucPuNum);
    pBufReq = L1_GetReadBufReq(ucPuNum, ucBufReqID);

    // fill in the buffer request
    pBufReq->bReadPreFetch = TRUE;
    pBufReq->usPhyBufferID = usPhyBufID;
    pBufReq->bFirstRCMDEn = FALSE;
    pBufReq->bReqLocalREQFlag = FALSE;
    pBufReq->ulReqStatusAddr = 0;
    pBufReq->ucLPNOffset = 0;
    pBufReq->ucLPNCount  = LPN_PER_BUF;
    pBufReq->ucReqOffset = 0;
    pBufReq->ucReqLength = SEC_PER_BUF;
    
    // set buffer LPNs
    pBufReq->aLPN[0] = LPN_FROM_LCTINDEX(ulPrefetchLct);
    for (i = 1; i < LPN_PER_BUF; i++)
    {
        pBufReq->aLPN[i] = pBufReq->aLPN[0] + i;
    }

    // set cachestatus
    for (i = 0; i < LPN_PER_BUF; i++)
    {
        L1_AddPrefetchCacheStatusToLink(usPhyBufID);
    }

    // add the buffer request to the tail of the fifo
    L1_InsertBufReqToHighPrioLinkTail(ucPuNum, ucBufReqID);

    return;
}

BOOL L1_ReadPreFetchCheckAllBufBusy(U16 usPhyBufID)
{
    U16  usLogBufID;
    BOOL bRet;

    usLogBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

    bRet = L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartHRID);

    if (BUF_IDLE == bRet)
    {
        bRet = L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartNRID);
    }

    return bRet;
}

U8 L1_PrefetchLct(U8 ucPuNum, U32 ulTargetLct)
{
    // Sean Gao 20150421
    // this function performs the prefetch operation for the
    // the specified LCT, it returns FALSE if we have to wait
    // for resources, TRUE otherwise

    U8 i;
    U16 usLogicBufID;
    U16 usCurCacheId;
    U32 ulStartLpn;
    U16 usPhyBufID;

    // perform a series of test to ensure the prefetch
    // operation for the current PU is good to go, if
    // one of the tests fails, prefetch operation for the
    // current PU is ignored

    // check the validity of the target LCT, we have to ensure
    // the LCT is within the normal range
#ifdef LCT_VALID_REMOVED
    if((ulTargetLct << SEC_PER_BUF_BITS) > (MAX_LBA_IN_DISK - 1))
#else
    if(((ulTargetLct << SEC_PER_BUF_BITS) > (MAX_LBA_IN_DISK - 1)) || (FALSE == L1_GetLCTValid(ulTargetLct)))
#endif
    {
        return L1_PREFETCH_INVALID_LCT;
    }
    
    // check if the target LCT is already in the write cache
    if(L1_GetLCTStartCache(ulTargetLct) != INVALID_4F)
    {
        return L1_PREFETCH_LCT_IN_CACHE;
    }

    // check if we can successfully allocate a free write
    // cache, please note that this check has to be placed
    // as the last check, otherwise we might cancel the
    // current prefetch operation with a buffer already
    // allocated
    usPhyBufID = L1_AllocateWriteBuf(ucPuNum);
    if(usPhyBufID == INVALID_4F)
    {
        return L1_PREFETCH_NO_WRITE_BUF;
    }

#ifdef SIM
    // check the cache status link of the prefetch buffer,
    // please note that the allocated cache can't be busy
    if(L1_ReadPreFetchCheckAllBufBusy(usPhyBufID) == BUF_BUSY)
    {
        DBG_Printf("prefetch buffer is busy\n");
        DBG_Getch();
    }
#endif

    // if we reach here, all tests have been passed, we then
    // begin the prefetch operation for this PU

    // add the prefetch buffer to the flush ongoing link
    // directly
    L1_AddBufToLinkTail(ucPuNum, FLUSH_ONGOING_LINK, LGCBUFID_FROM_PHYBUFID(usPhyBufID));

    // set the stage and flags of the prefetch buffer
    usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    gpBufTag[usLogicBufID].bSeqBuf = TRUE;
    gpBufTag[usLogicBufID].bPrefetchBuf = TRUE;
    gpBufTag[usLogicBufID].Stage = BUF_STAGE_CACHE;

    // set the write pointer of the prefetch buffer
    L1_BufferSetWritePointer(usLogicBufID, LPN_PER_BUF);

    // set buffer LPN and sector bitmap
    ulStartLpn = LPN_FROM_LCTINDEX(ulTargetLct);
    for(i = 0; i < LPN_PER_BUF; i++)
    {
        L1_SetBufferLPN(usLogicBufID, i, ulStartLpn + i);
        L1_SetBufferSectorBitmap(usLogicBufID, i, 0xFF);
    }

    // configure the LCT
#ifdef SIM
    if(L1_GetLCTStartCache(ulTargetLct) != INVALID_4F)
    {
        DBG_Printf("start cache id should be invalid\n");
        DBG_Getch();
    }
#endif
    // calculate the cache id of the first LPN of the prefetch
    // buffer
    usCurCacheId = L1_CalcCacheID(usLogicBufID, 0);
    
    // set the start cache id of the target LCT
    L1_SetLCTStartCache(ulTargetLct, usCurCacheId);

    // set the next cache id for all entries of the prefetch
    // buffer, except the last one
    for(i = 0; i < LPN_PER_BUF-1; i++)
    {
        L1_SetNextCacheId(usCurCacheId, usCurCacheId+1);
        usCurCacheId++;
    }

    // set the next cache id of the last cache id to invalid
    L1_SetNextCacheId(usCurCacheId, INVALID_4F);
    
    // L1_ReadPreFetchBuildBufReq issues a buffer request
    // to L2 and set up cache status link
    L1_ReadPreFetchBuildBufReq(ucPuNum, ulTargetLct, usPhyBufID);

    return L1_PREFETCH_REQ_SENT;
}

U8 L1_PrefetchSubcommand(U8 ucPuNum, U32 ulTargetLct)
{
    // Sean Gao 20150421
    // this function performs the prefetch operation for the
    // the specified LCT, it returns FALSE if we have to wait
    // for resources, TRUE otherwise

    U8 i;
    U16 usLogicBufID;
    U16 usCurCacheId;
    U32 ulStartLpn;
    U16 usPhyBufID;

    // perform a series of test to ensure the prefetch
    // operation for the current PU is good to go, if
    // one of the tests fails, prefetch operation for the
    // current PU is ignored

    // check the validity of the target LCT, we have to ensure
    // the LCT is within the normal range
#ifdef LCT_VALID_REMOVED
    if((ulTargetLct << SEC_PER_BUF_BITS) > (MAX_LBA_IN_DISK - 1))
#else
    if(((ulTargetLct << SEC_PER_BUF_BITS) > (MAX_LBA_IN_DISK - 1)) || (FALSE == L1_GetLCTValid(ulTargetLct)))
#endif
    {
        return L1_PREFETCH_INVALID_LCT;
    }
    
    // check if the target LCT is already in the write cache
    if(L1_GetLCTStartCache(ulTargetLct) != INVALID_4F)
    {
        return L1_PREFETCH_LCT_IN_CACHE;
    }

    // check if we can successfully allocate a free write
    // cache, please note that this check has to be placed
    // as the last check, otherwise we might cancel the
    // current prefetch operation with a buffer already
    // allocated
    usPhyBufID = L1_AllocateWriteBuf(ucPuNum);
    if(usPhyBufID == INVALID_4F)
    {
        return L1_PREFETCH_NO_WRITE_BUF;
    }

#ifdef SIM
    // check the cache status link of the prefetch buffer,
    // please note that the allocated cache can't be busy
    if(L1_ReadPreFetchCheckAllBufBusy(usPhyBufID) == BUF_BUSY)
    {
        DBG_Printf("prefetch buffer is busy\n");
        DBG_Getch();
    }
#endif

    // if we reach here, all tests have been passed, we then
    // begin the prefetch operation for this PU

    // add the prefetch buffer to the flush ongoing link
    // directly
    L1_AddBufToLinkTail(ucPuNum, FLUSH_ONGOING_LINK, LGCBUFID_FROM_PHYBUFID(usPhyBufID));

    // set the stage and flags of the prefetch buffer
    usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    gpBufTag[usLogicBufID].bSeqBuf = TRUE;
    gpBufTag[usLogicBufID].bPrefetchBuf = TRUE;
    gpBufTag[usLogicBufID].bIsCertainHit = TRUE;
    gpBufTag[usLogicBufID].Stage = BUF_STAGE_CACHE;

    // set the write pointer of the prefetch buffer
    L1_BufferSetWritePointer(usLogicBufID, LPN_PER_BUF);

    // set buffer LPN and sector bitmap
    ulStartLpn = LPN_FROM_LCTINDEX(ulTargetLct);
    for(i = 0; i < LPN_PER_BUF; i++)
    {
        L1_SetBufferLPN(usLogicBufID, i, ulStartLpn + i);
        L1_SetBufferSectorBitmap(usLogicBufID, i, 0xFF);
    }

    // configure the LCT
#ifdef SIM
    if(L1_GetLCTStartCache(ulTargetLct) != INVALID_4F)
    {
        DBG_Printf("start cache id should be invalid\n");
        DBG_Getch();
    }
#endif
    // calculate the cache id of the first LPN of the prefetch
    // buffer
    usCurCacheId = L1_CalcCacheID(usLogicBufID, 0);
    
    // set the start cache id of the target LCT
    L1_SetLCTStartCache(ulTargetLct, usCurCacheId);

    // set the next cache id for all entries of the prefetch
    // buffer, except the last one
    for(i = 0; i < LPN_PER_BUF-1; i++)
    {
        L1_SetNextCacheId(usCurCacheId, usCurCacheId+1);
        usCurCacheId++;
    }

    // set the next cache id of the last cache id to invalid
    L1_SetNextCacheId(usCurCacheId, INVALID_4F);
    
    // L1_ReadPreFetchBuildBufReq issues a buffer request
    // to L2 and set up cache status link
    L1_ReadPreFetchBuildBufReq(ucPuNum, ulTargetLct, usPhyBufID);

    return L1_PREFETCH_REQ_SENT;
}

extern GLOBAL  U8 g_ucAllPuMask;
void L1_TaskPrefetch(void)
{
    // 20150814 Sean Gao
    //
    // this function checks the current status of the system and determines
    // if we should conduct prefetch operations
    //
    // for a read operation, there 2 major time consuming part:
    //
    // 1. NFC read time
    // 2. host data trasfer time
    //
    // to achieve good performance when conducting sequential read,
    // we have to fully utilize all PUs, keep them busy all the time.
    // normally we don't have to intervene, since the firmware keeps processing
    // read subcommands and issuing read buffer requests, making all PUs occupied,
    // which is the ideal situation.
    //
    // however, if the firmware stalls for any reasons and can't continue
    // issuing read buffer requests, we then have to initiate prefetch operations
    //
    // when either one of these 2 conditions hold,
    // it might mean that the firmware is stalling:
    //
    // 1. the subcommand queue is empty
    //
    // if the subcommand queue is empty, it could be one of the following reasons:
    //
    // a. the subcommands of the current host command are all completed and
    // L0 hasn't yet push subcommands of the next host command into the queue
    //
    // b. subcommands of all outstanding host commands are processed
    //
    // 2. current subcommand fails to be completed
    //
    // please note that at this point, if g_pCurSubCmd != NULL,
    // it must mean that the subcommand can't be completed normally,
    // the reasons could be:
    //
    // a. the read subcommand hits a prefetch buffer, whose data
    // is not ready yet
    //
    // b. there aren't enough cachestatus nodes
    //
    // c. there aren't available read DSGs(sata mode only)
    //
    // d. the DSG of the previous subcommand is not yet ready(sata mode only)
    //
    // e. the read buffer request FIFO of the current PU is full
    //
    // when either of the 2 cases occur, we might have to conduct prefetch operation

    // perform a quick check on buffer request fifos,
    // if there are pending buffer request for all PUs, return immediately and
    // do not perform any kind of prefetch at all
    if(g_ulReadBufReqPuBitMap == g_ucAllPuMask)
    {
        return;
    }

#ifdef HOST_SATA
    // phase 1: process pending subcommands
    // if the current subcommand fails to be completed, first we should examine
    // and process pending subcommands in the subcommand queue before perform
    // any predicative prefetch
    //
    // please note only sata should perform this stage, we should avoid busy prefetch
    // when on nvme since reading lots of data from dram hinders performance greatly
    if(g_ulRdPreFetchTHS >= L1_RD_PREFETCH_THS && g_pCurSubCmd != NULL)
    {
        PSCMD pScmd;
        U32 ulCurHeadPointer;
        U32 ulAvailablePuBitmap;

        // first we have to ensure there are no pending active write buffers, merge pending buffers
        // or flush pending buffers before conducting prefetch to guarantee the FIFO property of
        // the buffer management mechanism
        if((g_ulMergeBufferBitmap | g_ulFlushBufferBitmap | g_ulActiveWriteBufferBitmap) != 0)
        {
            L1_ReleaseAllWriteCache();
            return;
        }

        // initialize the pu available bitmap
        ulAvailablePuBitmap = (~g_ulReadBufReqFullPuBitMap) & g_ucAllPuMask;

        // get the current head pointer to the subcommand queue
        ulCurHeadPointer = L1_GetCurrentScqHeadPointer();
        
        // try scanning all pending subcommands 
        while(((pScmd = L1_TryFetchScmd(&ulCurHeadPointer)) != NULL) && ulAvailablePuBitmap != 0)
        {
            // first we have to inspect the current subcommand to update
            // the prefetch trend information
            L1_PrefetchTrendCheckScmd(pScmd);

            // next process the current subcommand if it hasn't been processed by the prefetch mechanism
            if(L1_PrefetchIsScmdProcessed(pScmd) == FALSE)
            {
                if(pScmd->tMA.ucOpType == (U8)DM_READ)
                {
                    // the subcommand is a read subcommand, prefetch its data

                    U32 ulTargetLct;
                    U8 ucTargetPu;

                    // get the lct and target pu of the read subcommand
                    ulTargetLct = LCTINDEX_FROM_LBA(pScmd->tMA.ulSubSysLBA);
                    ucTargetPu = L1_GetSuperPuFromLCT(ulTargetLct);

                    // process only if the PU is available
                    if(COM_BitMaskGet(ulAvailablePuBitmap, ucTargetPu) == TRUE)
                    {
                        if(L1_FreeReadBufReqLinkEmpty(ucTargetPu) == TRUE)
                        {
                            COM_BitMaskClear(&ulAvailablePuBitmap, ucTargetPu);
                        }
                        else if(L1_CacheStatusRescourseCheck(L1_CS_PREFETCH_BUF_CHECK_CNT) == FALSE)
                        {
                            // not enough cache status nodes, set all PUs as unavailable
                            ulAvailablePuBitmap = 0;
                        }
                        else
                        {
                            // the resources are ready, prefetch the data of the subcommand
                            U8 ucPrefetchResult = L1_PrefetchSubcommand(ucTargetPu, ulTargetLct);

                            // check if the prefetch request is processed successfully
                            if(ucPrefetchResult != L1_PREFETCH_NO_WRITE_BUF)
                            {
                                // prefetch request succeeds, mark the subcommand as processed
                                L1_PrefetchMarkProcessedScmd(pScmd);
                            }
                            else
                            {
                                // there are no more write buffers, mark the pu as unavailable
                                COM_BitMaskClear(&ulAvailablePuBitmap, ucTargetPu);
                            }
                        }
                    } // if(COM_BitMaskGet(ulAvailablePuBitmap, ucTargetPu) == TRUE)
                } // if(pScmd->tMA.ucOpType == (U8)DM_READ)
                else
                {
                    // write subcommand, stop processing and break immediately
                    break;
                }
            } // if(L1_PrefetchIsScmdProcessed(pScmd) == FALSE)
        } // while(((pScmd = L1_TryFetchScmd(&ulCurHeadPointer)) != NULL) && ulAvailablePuBitmap != 0)
    } // if(g_ulRdPreFetchTHS >= L1_RD_PREFETCH_THS && g_pCurSubCmd != NULL)
#endif

    // phase 2: predictive prefetch
#ifdef HOST_SATA
    if(g_ulRdPreFetchTHS >= L1_RD_PREFETCH_THS && g_ulPendingPrefetchLctBitmap != 0 && (L1_IsSCQEmpty() == TRUE || g_pCurSubCmd != NULL) && L1_CacheStatusRescourseCheck(L1_CS_PREFETCH_BUF_CHECK_CNT) == TRUE)
#else
    // for nvme mode, we should only perform idle prefetch due to performance consideration
    if(g_ulRdPreFetchTHS >= L1_RD_PREFETCH_THS && g_ulPendingPrefetchLctBitmap != 0 && (L1_IsSCQEmpty() == TRUE) && L1_CacheStatusRescourseCheck(L1_CS_PREFETCH_BUF_CHECK_CNT) == TRUE)
#endif
    {
        U32 ulPrefetchReadyPuBitmap;
#ifndef L2_FAKE
        U32 ulNfcEmptyBitmap;
#endif

        // first we have to ensure there are no pending active write buffers, merge pending buffers
        // or flush pending buffers before conducting prefetch to guarantee the FIFO property of
        // the buffer management mechanism
        if((g_ulMergeBufferBitmap | g_ulFlushBufferBitmap | g_ulActiveWriteBufferBitmap) != 0)
        {
            L1_ReleaseAllWriteCache();
            return;
        }

        // get the empty pu bitmap, which indicates the status of all PUs
        //ulNfcEmptyBitmap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_EMPTY);
#ifndef L2_FAKE
        ulNfcEmptyBitmap = L2_FCMDQGetEmptyBitmap(0);

        // calculate the prefetch ready pu bitmap, a pu is considered
        // prefetch ready if:
        // a. all NFC operations of the PU are completed
        // b. there are no pending buffer requests in the PU
        // c. the active write cache count hasn't reached the maximum
        ulPrefetchReadyPuBitmap = ulNfcEmptyBitmap & (~g_ulReadBufReqPuBitMap) & g_ulFreeCacheAvailableBitmap;
#else
        ulPrefetchReadyPuBitmap = (~g_ulReadBufReqPuBitMap) & g_ulFreeCacheAvailableBitmap;
#endif

#ifdef SIM
        // check the validity of ulPrefetchReadyPuBitmap
        if (HAL_POPCOUNT(ulPrefetchReadyPuBitmap) > SUBSYSTEM_SUPERPU_NUM)
        {
            DBG_Printf("ulPrefetchReadyPuBitmap error\n");
            DBG_Getch();
        }
#endif

        // conduct prefetch operations only if ulPrefetchReadyPuBitmap != 0,
        // which means there are idle PUs
        if(ulPrefetchReadyPuBitmap != 0)
        {
            U8 i;
            U8 ucPendingPrefetchLctNum;

            // get the number of pending prefetch LCTs
            ucPendingPrefetchLctNum = HAL_POPCOUNT(g_ulPendingPrefetchLctBitmap);

            // register the pending prefetch lct bitmap to the
            // hardware
            HAL_Wclzstate(g_ulPendingPrefetchLctBitmap);

            // process all pending prefetch LCTs until all available
            // pus are used or all pending prefetch LCTs are processed
            for(i = 0; i < ucPendingPrefetchLctNum && ulPrefetchReadyPuBitmap != 0; i++)
            {
                U8 ucPrefetchLctIndex;
                U8 ucPuNum;
                U32 ulTargetLct;

                // get the current prefetch index
                ucPrefetchLctIndex = HAL_SCLZ();

#ifdef SIM
                if(COM_BitMaskGet(g_ulPendingPrefetchLctBitmap, 31-ucPrefetchLctIndex) == FALSE)
                {
                    DBG_Printf("pending prefetch lct bitmap error\n");
                    DBG_Getch();
                }
#endif

                // calculate the target lct for the current index
                ulTargetLct = g_ulRdPreFetchPreLCT + 1 + ucPrefetchLctIndex;

                // calculate the pu number the target lct belongs to
                ucPuNum = L1_GetSuperPuFromLCT(ulTargetLct);

                // conduct prefetch operation only if the pu is
                // available
                if(COM_BitMaskGet(ulPrefetchReadyPuBitmap, ucPuNum) == TRUE)
                {
                    U8 ucPrefetchResult = L1_PrefetchLct(ucPuNum, ulTargetLct);

                    // act based on the result of prefetch operation
                    switch(ucPrefetchResult)
                    {
                        case L1_PREFETCH_INVALID_LCT:
                        case L1_PREFETCH_LCT_IN_CACHE:
                            // either the prefetch LCT is invalid or it's in one of the write buffers,
                            // mark the prefetch LCT as done
                            COM_BitMaskClear(&g_ulPendingPrefetchLctBitmap, 31-ucPrefetchLctIndex);
                            break;

                        case L1_PREFETCH_NO_WRITE_BUF:
                            // the prefetch fails due to buffer allocation failure,
                            // unset the prefetch ready bit of the pu
                            COM_BitMaskClear(&ulPrefetchReadyPuBitmap, ucPuNum);
                            break;

                        case L1_PREFETCH_REQ_SENT:
                            // a prefetch request has been sent, mark the prefetch LCT as done
                            // and unset the PU ready bit
                            COM_BitMaskClear(&g_ulPendingPrefetchLctBitmap, 31-ucPrefetchLctIndex);
                            COM_BitMaskClear(&ulPrefetchReadyPuBitmap, ucPuNum);
                            break;

                        default:
                            DBG_Printf("prefetch result error\n");
                            DBG_Getch();
                    }

                } // if(COM_BitMaskGet(ulPrefetchReadyPuBitmap, ucPuNum) == TRUE)

                // check if we have enough free cache status nodes
                if(L1_CacheStatusRescourseCheck(L1_CS_PREFETCH_BUF_CHECK_CNT) == FALSE)
                {
                    break;
                }

            } // for(i = 0; i < ucPendingPrefetchLctNum && ulPrefetchReadyPuBitmap != 0; i++)
        } // if(ulPrefetchReadyPuBitmap != 0)
    } // if(g_ulRdPreFetchTHS >= L1_RD_PREFETCH_THS && g_ulPendingPrefetchLctBitmap != 0 && (L1_IsSCQEmpty() == TRUE || g_pCurSubCmd != NULL) && L1_CacheStatusRescourseCheck(L1_CS_PREFETCH_BUF_CHECK_CNT) == TRUE)

    return;
}
