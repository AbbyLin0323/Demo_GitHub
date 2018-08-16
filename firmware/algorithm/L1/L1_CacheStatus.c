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
Filename     : L1_CacheStatus.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  blakezhang

Description: 

Modification History:
20120118 blakezhang 001 first create
*****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"

GLOBAL  U16 g_usCacheStatusUsed;
GLOBAL  U8* g_pCacheStatus;
GLOBAL  U8* g_pucRawDataCacheStatus;
GLOBAL  CACHESTATUS_ID_ENTRY *g_CS_ID_Entry;
GLOBAL  CACHESTATUS_RANGE_ENTRY *g_CS_Range_Entry;
GLOBAL  CACHESTATUS_FIFO *g_pCSFifoEntry;

//extern RDT OTFB begin address and size
extern GLOBAL U32 g_ulRdtOtfbAddr;
extern GLOBAL U32 g_ulRdtOtfbAddrSize;

extern BOOL L2_IsBootupOK();

void MCU1_DRAM_TEXT L1_CacheStatusSram0Map(U32 *pFreeSramBase)
{
    // this function allocates cache status fifo in SRAM, cache status fifo is basically the
    // free cache status pool, every time we want to allocate a cache status, we take it from
    // from the tail of the fifo, and when a cache status is released, we put it back to the
    // head of the fifo

    U32 ulFreeSramBase;

    // obtain the current SRAM base passed into the function
    ulFreeSramBase = *pFreeSramBase;

    // align the SRAM base address to a 16-dword address before allocation
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    // allocate space for the cache status fifo
    g_pCSFifoEntry = (CACHESTATUS_FIFO *)ulFreeSramBase;

    // first we align the size of the cache status fifo structure to 16-dword aligned,
    // then we increase the current SRAM base by that size, please note that sizeof(g_pCSFifoEntry[0])
    // equals to sizeof(CACHESTATUS_FIFO)
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign( sizeof(g_pCSFifoEntry[0]) ) );

    // align the increased SRAM base again, just in case
    COM_MemAddr16DWAlign( &ulFreeSramBase );

    // update the current SRAM base
    *pFreeSramBase = ulFreeSramBase;

    return;
}

void MCU1_DRAM_TEXT L1_CacheStatusSram1Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    // obtain the current SRAM base passed into the function
    ulFreeSramBase = *pFreeSramBase;

    // align the SRAM base address to a 16-dword address before allocation
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    g_CS_ID_Entry = (CACHESTATUS_ID_ENTRY *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(sizeof(CACHESTATUS_ID_ENTRY)*L1_CACHESTATUS_TOTAL));
    COM_MemAddr16DWAlign( &ulFreeSramBase );

    // update the current SRAM base
    *pFreeSramBase = ulFreeSramBase;

    return;
}

void MCU1_DRAM_TEXT L1_CacheStatusDRAMMap(U32 *pFreeDramBase)
{
    // this function allocates space for cache status manager in DRAM, cache status
    // manager contains detailed information for every cache status entry

    U32 ulFreeDramBase;

    // obtain the current DRAM base passed into the function
    ulFreeDramBase = *pFreeDramBase;

    // align the DRAM base address to a 16-dword address before allocation
    COM_MemAddr16DWAlign( &ulFreeDramBase );

    // allocate space for the cache status manager
#ifdef DCACHE
    g_CS_Range_Entry = (CACHESTATUS_RANGE_ENTRY *)(ulFreeDramBase + DRAM_HIGH_ADDR_OFFSET);
#else
    g_CS_Range_Entry = (CACHESTATUS_RANGE_ENTRY *)ulFreeDramBase;
#endif

    // first we align the size of the cache status manager with 16-dword, then we increase
    // the DRAM base address by that size
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(CACHESTATUS_RANGE_ENTRY)*L1_CACHESTATUS_TOTAL));

    // align the increased DRAM base again, just in case
    COM_MemAddr16DWAlign( &ulFreeDramBase );

    // update the current DRAM base
    *pFreeDramBase = ulFreeDramBase;

    return;
}

/****************************************************************************
Name        :L1_CacheStatusOTFBMap
Input       :
Output      :
Description :allcate otfb for buffer module.
Others      :
Modify      :
2014.2.17 kristinwang
****************************************************************************/
void MCU1_DRAM_TEXT L1_CacheStatusOTFBMap(void)
{
    // this function allocates space for all cache status, which is used to represent the status
    // of a particular operation

    U32 ulCacheStatusOTFBBase;
    
    // obtain the current OTFB base address used for cache status
    ulCacheStatusOTFBBase = g_FreeMemBase.ulFreeCacheStatusBase;
	g_ulRdtOtfbAddr = ulCacheStatusOTFBBase;

    // allocate space for all cache status
    g_pCacheStatus = (U8*)ulCacheStatusOTFBBase;

    // increase the OTFB base address by L1_CACHESTATUS_TOTAL, since each cache status takes exactly 1 byte
    COM_MemIncBaseAddr(&ulCacheStatusOTFBBase, L1_CACHESTATUS_TOTAL);

    // align the increased OTFB base address
    COM_MemAddr16DWAlign(&ulCacheStatusOTFBBase);

    // assign the cache status address used for raw data operations
    g_pucRawDataCacheStatus = (U8*)ulCacheStatusOTFBBase;
    COM_MemIncBaseAddr(&ulCacheStatusOTFBBase, LPN_PER_BUF);

    // align the increased OTFB base address again
    COM_MemAddr16DWAlign(&ulCacheStatusOTFBBase);

    // update the OTFB base address
    g_FreeMemBase.ulFreeCacheStatusBase = ulCacheStatusOTFBBase;

	g_ulRdtOtfbAddrSize = ulCacheStatusOTFBBase - g_ulRdtOtfbAddr;

    return;
}

/****************************************************************************
Function  : L1_CacheStatusInit
Input     :  
Output    :  
Return    : 
Purpose   : this function initializes all cache status related data structure
Reference :
****************************************************************************/
void MCU1_DRAM_TEXT L1_CacheStatusInit(void)
{
    U16 usLoop;

    for (usLoop = 0; usLoop < L1_CACHESTATUS_TOTAL; usLoop++)
    {
        g_pCacheStatus[usLoop] = SUBSYSTEM_STATUS_INIT;

#if 0
        g_pCSManager->CS_Entry[usLoop].usNextID   = INVALID_4F;
        g_pCSManager->CS_Entry[usLoop].usSecLen   = INVALID_4F;
        g_pCSManager->CS_Entry[usLoop].ulStartLBA = INVALID_8F;
        g_pCSManager->CS_Entry[usLoop].usRelatedCachestatusId = INVALID_4F;
#else
        g_CS_Range_Entry[usLoop].usSecLen = INVALID_4F;
        g_CS_Range_Entry[usLoop].usSecOffInBuf = INVALID_4F;
        g_CS_ID_Entry[usLoop].usNextID   = INVALID_4F;
        g_CS_ID_Entry[usLoop].usRelatedCachestatusId = INVALID_4F;
#endif

        g_pCSFifoEntry->usCacheStatusID[usLoop] = usLoop;
        g_pCSFifoEntry->usFifoTail = 0;
        g_pCSFifoEntry->usFifoHead = 0;
    }

    *g_pucRawDataCacheStatus = SUBSYSTEM_STATUS_INIT;

    g_usCacheStatusUsed = 0;

    return;
}

/****************************************************************************
Function  : L1_CacheStatusWarmInit
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
void MCU1_DRAM_TEXT L1_CacheStatusWarmInit(void)
{
    // directly invoke L1_CacheStatusInit since this function should do exactly the
    // same

    L1_CacheStatusInit();
}

/****************************************************************************
Function  : L1_CacheStatusClearHostPending
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference : used it L1 errorhandling to clear all pending host read/write cache status
****************************************************************************/
void MCU1_DRAM_TEXT L1_CacheStatusClearHostPending(void)
{
    U16 usLoop;
    U16 usLogBufID;
    BUFF_TAG *pBufTag;

    usLogBufID = LGCBUFID_FROM_PHYBUFID(g_ulWriteBufStartPhyId);

    for (usLoop = 0; usLoop < L1_BUFFER_COUNT_WRITE; usLoop++, usLogBufID++)
    {
        pBufTag  = &gpBufTag[usLogBufID];
      
        L1_ClearCacheStatusLink(&(pBufTag->usStartHRID));
        L1_ClearCacheStatusLink(&(pBufTag->usStartHWID));
    }

#ifdef HOST_READ_FROM_DRAM
    usLogBufID = LGCBUFID_FROM_PHYBUFID(g_ulRFDBufStartPhyId);

    for (usLoop = 0; usLoop < L1_BUFFER_COUNT_READ_FROM_DRAM; usLoop++, usLogBufID++)
    {
        pBufTag  = &gpBufTag[usLogBufID];
      
        L1_ClearCacheStatusLink(&(pBufTag->usStartHRID));
    }
#endif

#ifndef HOST_SATA
    for (usLoop = 0; usLoop < L1_BUFFER_COUNT_READ; usLoop++)
    {
        L1_ClearCacheStatusLink(L1_GetReadBufferEntryCachestatus(g_ulReadBufStartPhyId + usLoop));
    }
#endif

    *g_pucRawDataCacheStatus = SUBSYSTEM_STATUS_INIT;

    return;
}

/****************************************************************************
Function  : L1_GetRawCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U8 L1_GetRawCacheStatus(void)
{
    return (*g_pucRawDataCacheStatus);
}

/****************************************************************************
Function  : L1_GetRawCacheStatusAddr
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_GetRawCacheStatusAddr(void)
{
    return ((U32)g_pucRawDataCacheStatus);
}

/****************************************************************************
Function  : L1_SetRawCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : 
Reference :
****************************************************************************/
void L1_SetRawCacheStatus(U8 ucStatus)
{
    *g_pucRawDataCacheStatus = ucStatus;

    return;
}

/****************************************************************************
Function  : L1_CacheStatusFifoFull
Input     :  
Output    :  
Return    : 
Purpose   :  check if cache status FIFO full
Reference :
****************************************************************************/
BOOL L1_CacheStatusFifoFull(void)
{
    U16 usTail;

    /* check Tail +1 == Head */
    usTail = g_pCSFifoEntry->usFifoHead + 1;

    if(usTail >= (L1_CACHESTATUS_TOTAL + 1))
    {
        usTail = 0;
    }

    if (usTail == g_pCSFifoEntry->usFifoTail)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Function  : L1_CacheStatusWaitIdle
Input     :  
Output    :  
Return    :  FALSE: not idle; TRUE: all CacheStatus idle
Purpose   :  wait all cachestatus idle
Reference :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L1_CacheStatusWaitIdle(void)
{
    if (0 != g_usCacheStatusUsed)
    {
        L1_RecycleCacheStatusID();
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/****************************************************************************
Function  : L1_CacheStatusRescourseCheck
Input     :  
Output    :  
Return    : 
Purpose   :  check the recourse before allocate
Reference :
****************************************************************************/
BOOL L1_CacheStatusRescourseCheck(U16 ucAllocateCnt)
{
    if ((L1_CACHESTATUS_TOTAL - g_usCacheStatusUsed) > ucAllocateCnt)
    {
        return TRUE;
    }
    else
    {
        /* need to add search all buffer links to release cachestatus function */
        L1_RecycleCacheStatusID();
        return FALSE;
    }
}

/****************************************************************************
Function  : L1_GetCacheStatusAddr
Input     :  
Output    :  
Return    : 
Purpose   :
Reference :
****************************************************************************/
U32 L1_GetCacheStatusAddr(U16 usCStatusID)
{
    return (U32)&g_pCacheStatus[usCStatusID];
}

/****************************************************************************
Function  : L1_GetNextCacheStatusID
Input     :  
Output    :  
Return    : 
Purpose   :
Reference :
****************************************************************************/
U16 L1_GetNextCacheStatusID(U16 usCStatusID)
{
    return g_CS_ID_Entry[usCStatusID].usNextID;
}

/****************************************************************************
Function  : L1_SetNextCacheStatusID
Input     :  
Output    :  
Return    : 
Purpose   :
Reference :
****************************************************************************/
void L1_SetNextCacheStatusID(U16 usCurrStatusID, U16 usNextStatusID)
{
    g_CS_ID_Entry[usCurrStatusID].usNextID = usNextStatusID;

    return;
}

/****************************************************************************
Function  : L1_UpdateCacheStatusRangeInfo
Input     :  
Output    :  
Return    : 
Purpose   :
Reference :
****************************************************************************/
void L1_UpdateCacheStatusRangeInfo(U16 usCStatusID, U16 usSecOffset, U16 usSecLen)
{
    g_CS_Range_Entry[usCStatusID].usSecOffInBuf = usSecOffset;
    g_CS_Range_Entry[usCStatusID].usSecLen = usSecLen;

    return;
}

/****************************************************************************
Function  : L1_AllocateCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : 
        allocate a cache status from fifo, return the CacheStatusID and set the value to pending
Reference :
****************************************************************************/
U16 L1_AllocateCacheStatus(void)
{
    U16 usTail;
    U16 usCStatusID;

#if 0
    /* we already ensured the resource */
    if (TRUE == L1_CacheStatusFifoFull())
    {
        DBG_Printf("L1_AllocateCacheStatus FIFO Full\n");
        DBG_Getch();
    }
#endif

    usTail = g_pCSFifoEntry->usFifoHead;
    usCStatusID = g_pCSFifoEntry->usCacheStatusID[usTail];

#ifdef SIM
    if (SUBSYSTEM_STATUS_INIT != g_pCacheStatus[usCStatusID])
    {
        DBG_Printf("L1_AllocateCacheStatus ID 0x%x value 0x%x ERROR!\n", usCStatusID, g_pCacheStatus[usCStatusID]);
        DBG_Getch();
    }
#endif

#ifdef SIM
    if (g_CS_ID_Entry[usCStatusID].usRelatedCachestatusId != INVALID_4F)
    {
        DBG_Printf("related cache status id error\n");
        DBG_Getch();
    }
#endif

    g_pCacheStatus[usCStatusID] = SUBSYSTEM_STATUS_PENDING;

    usTail++;
    if (usTail >= L1_CACHESTATUS_TOTAL)
    {
        usTail = 0;
    }

    g_pCSFifoEntry->usFifoHead = usTail;
    g_usCacheStatusUsed++;

    return usCStatusID;
}

/****************************************************************************
Function  : L1_ReleaseCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : 
        allocate a cache status from fifo, return the CacheStatusID and set the value to pending
Reference :
****************************************************************************/
void L1_ReleaseCacheStatus(U16 usCStatusID)
{  
    U16 usHead;

    usHead = g_pCSFifoEntry->usFifoTail;

#ifdef SIM
    if ((SUBSYSTEM_STATUS_PENDING == g_pCacheStatus[usCStatusID])
        || (SUBSYSTEM_STATUS_INIT == g_pCacheStatus[usCStatusID]))
    {
        DBG_Printf("L1_ReleaseCacheStatus ID 0x%x value 0x%x ERROR!\n", usCStatusID, g_pCacheStatus[usCStatusID]);
        DBG_Getch();
    }

    if (usHead == g_pCSFifoEntry->usFifoHead)
    {
        DBG_Printf("L1_ReleaseCacheStatus usHead 0x%x ERROR!\n", usHead);
        DBG_Getch();
    }
#endif

    g_pCacheStatus[usCStatusID] = SUBSYSTEM_STATUS_INIT;
    g_pCSFifoEntry->usCacheStatusID[usHead] = usCStatusID;

    usHead++;
    if (usHead >= L1_CACHESTATUS_TOTAL)
    {
        usHead = 0;
    }

    g_pCSFifoEntry->usFifoTail = usHead;
    g_usCacheStatusUsed--;

    return;
}

/****************************************************************************
Function  : L1_AddCacheStatusToLink
Input     :  
Output    :  
Return    : 
Purpose   : allocate a free CacheStatus from FIFO and add it to given link, 
                fill range infos and set status value to pending then return the status address
Reference :
****************************************************************************/
U32 L1_AddCacheStatusToLink(U16 *pStartID, U16 usSecOffset, U16 usSecLen)
{
    U16 usNewStatusID;

    usNewStatusID = L1_AllocateCacheStatus();

    /* add new Cache Status to link head */
    L1_SetNextCacheStatusID(usNewStatusID, (*pStartID));
    *pStartID = usNewStatusID;

    /* update LBA Range Infos */
    L1_UpdateCacheStatusRangeInfo(usNewStatusID, usSecOffset, usSecLen);

    return L1_GetCacheStatusAddr(usNewStatusID);
}

U32 L1_AddRelatedCachestatusNodes(U16* pusStartCachestatusId1, U16 usStartSectorOffset1, U16 usSectorLength1, U16* pusStartCachestatusId2, U16 usStartSectorOffset2, U16 usSectorLength2)
{
    U16 usCachestatusId1, usCachestatusId2;

    usCachestatusId1 = L1_AllocateCacheStatus();
    usCachestatusId2 = L1_AllocateCacheStatus();

    L1_SetNextCacheStatusID(usCachestatusId1, *pusStartCachestatusId1);
    L1_SetNextCacheStatusID(usCachestatusId2, *pusStartCachestatusId2);

    *pusStartCachestatusId1 = usCachestatusId1;
    *pusStartCachestatusId2 = usCachestatusId2;

    L1_UpdateCacheStatusRangeInfo(usCachestatusId1, usStartSectorOffset1, usSectorLength1);
    L1_UpdateCacheStatusRangeInfo(usCachestatusId2, usStartSectorOffset2, usSectorLength2);

#ifdef SIM
    // perform a validity check on the related cache status id
    if((g_CS_ID_Entry[usCachestatusId1].usRelatedCachestatusId != INVALID_4F) || (g_CS_ID_Entry[usCachestatusId2].usRelatedCachestatusId != INVALID_4F))
    {
        DBG_Printf("related cache status id error\n");
        DBG_Getch();
    }
#endif

    g_CS_ID_Entry[usCachestatusId1].usRelatedCachestatusId = usCachestatusId2;
    g_CS_ID_Entry[usCachestatusId2].usRelatedCachestatusId = usCachestatusId1;

    return L1_GetCacheStatusAddr(usCachestatusId1);
}

/****************************************************************************
Function  : L1_CheckCacheStatus
Input     :  
Output    :  
Return    : 
Purpose   : check the cache status for given link and ranges.
                will remove finished cache status ID to FIFO when polling
Reference :
****************************************************************************/
BOOL L1_CheckCacheStatus(U16* pStartCSID, U16 usSecOffset, U16 usSecCnt)
{
    U16 usCurCSId;  //Current CacheStatus ID
    U16 usPreCSId;  // Previous CacheStatus ID
    U16 usRelatedCSId; //Related CacheStatus ID
    U8 ucRelatedCSVal; //Related CacheStatus Value
    U8 ucHasFailedOperation = FALSE;

    usCurCSId = *pStartCSID;

    while (INVALID_4F != usCurCSId)
    {
        switch (g_pCacheStatus[usCurCSId])
        {
        case SUBSYSTEM_STATUS_INIT:
            // this is a special case for prefetch read cache status, L2 hasn't
            // configure this cache status properly, return as busy immediately
            return BUF_BUSY;

        case SUBSYSTEM_STATUS_PENDING:
            // check if the current cache status id has a related cache status
            // id, if it does, we have to check its status too
            if (g_CS_ID_Entry[usCurCSId].usRelatedCachestatusId != INVALID_4F)
            {
                usRelatedCSId = g_CS_ID_Entry[usCurCSId].usRelatedCachestatusId;
                ucRelatedCSVal = g_pCacheStatus[usRelatedCSId];

                if ((SUBSYSTEM_STATUS_SUCCESS == ucRelatedCSVal)
                    || (SUBSYSTEM_STATUS_RETRY_SUCCESS == ucRelatedCSVal)
                    || (SUBSYSTEM_STATUS_RECC == ucRelatedCSVal)
                    || (SUBSYSTEM_STATUS_FAIL == ucRelatedCSVal)
                    || (SUBSYSTEM_STATUS_EMPTY_PG == ucRelatedCSVal))
                {
                    // the status of the related cache status id has been changed
                    // perform corresponding actions

                    g_pCacheStatus[usCurCSId] = ucRelatedCSVal;

#if 0
                    g_pCSManager->CS_Entry[usCurCSId].usRelatedCachestatusId = INVALID_4F;
                    g_pCSManager->CS_Entry[usRelatedCSId].usRelatedCachestatusId = INVALID_4F;
#else
                    g_CS_ID_Entry[usCurCSId].usRelatedCachestatusId = INVALID_4F;
                    g_CS_ID_Entry[usRelatedCSId].usRelatedCachestatusId = INVALID_4F;
#endif
                }
                else if (SUBSYSTEM_STATUS_PENDING != ucRelatedCSVal)
                {
                    // perform a validity check on the related cache status
                    DBG_Printf("related cache status error\n");
                    DBG_Getch();
                }
            }

            if (INVALID_4F == usSecCnt)
            {
                return BUF_BUSY;
            }
            else if (0 == usSecCnt)
            {
                usPreCSId = usCurCSId;
                usCurCSId = L1_GetNextCacheStatusID(usPreCSId);
            }
            else
            {
                // a valid sector count has been entered, check if the ranges overlap

                if ((usSecOffset > (g_CS_Range_Entry[usCurCSId].usSecOffInBuf + g_CS_Range_Entry[usCurCSId].usSecLen - 1))
                    || ((usSecOffset + usSecCnt - 1) < g_CS_Range_Entry[usCurCSId].usSecOffInBuf))
                {
                    // ranges don't overlap, proceed to the next cache status id
                    usPreCSId = usCurCSId;
                    usCurCSId = L1_GetNextCacheStatusID(usPreCSId);
                }
                else
                {
                    return BUF_BUSY;
                }
            }

            break;

        case SUBSYSTEM_STATUS_EMPTY_PG:
            if (TRUE == L2_IsBootupOK())
            {
                // if we encounter an empty page error, we will getch directly, since it's impossible for L1 to
                // read an empty page
                DBG_Printf("empty page error,CurCSId %d Addr 0x%x\n", usCurCSId, (U32)&g_pCacheStatus[usCurCSId]);
                DBG_Getch();

                break;
            }

        case SUBSYSTEM_STATUS_SUCCESS:
        case SUBSYSTEM_STATUS_RETRY_SUCCESS:
        case SUBSYSTEM_STATUS_RECC:
        case SUBSYSTEM_STATUS_FAIL:
            // the status of the current cache status is success or fail

            // check if the current cache status id has a related cache status
            // id, if it does, we have to check its status too
            if (g_CS_ID_Entry[usCurCSId].usRelatedCachestatusId != INVALID_4F)
            {
                usRelatedCSId = g_CS_ID_Entry[usCurCSId].usRelatedCachestatusId;

                if (g_pCacheStatus[usRelatedCSId] != SUBSYSTEM_STATUS_PENDING)
                {
                    DBG_Printf("related cache status error\n");
                    DBG_Getch();
                }

                g_pCacheStatus[usRelatedCSId] = g_pCacheStatus[usCurCSId];

                g_CS_ID_Entry[usCurCSId].usRelatedCachestatusId = INVALID_4F;
                g_CS_ID_Entry[usRelatedCSId].usRelatedCachestatusId = INVALID_4F;
            }

            // remove the cache status id from link since its status is success
            if (usCurCSId == *pStartCSID)
            {
                // the current cache status is the start of the link

                *pStartCSID = L1_GetNextCacheStatusID(usCurCSId);

                L1_SetNextCacheStatusID(usCurCSId, INVALID_4F);
                L1_ReleaseCacheStatus(usCurCSId);

                usCurCSId = *pStartCSID;
            }
            else
            {
                // the current cache status id is not the start cache status id

#ifdef SIM
                // perform a validity check on the previous cache status id
                if (INVALID_4F == usPreCSId)
                {
                    DBG_Printf("previous cache status id error\n");
                    DBG_Getch();
                }
#endif

                L1_SetNextCacheStatusID(usPreCSId, L1_GetNextCacheStatusID(usCurCSId));

                L1_SetNextCacheStatusID(usCurCSId, INVALID_4F);
                L1_ReleaseCacheStatus(usCurCSId);

                usCurCSId = L1_GetNextCacheStatusID(usPreCSId);
            }

            break;

        default:
            DBG_Printf("cache status error\n");
            DBG_Getch();

            break;
        } // switch (g_pCacheStatus[usCurCSId])
    } // while (INVALID_4F != usCurCSId)

    return BUF_IDLE;
}

U8 L1_CheckPrefetchBufferCacheStatus(U16* pStartCachestatusId)
{
    U16 usCurrentCachestatusId;
    U16 usPreviousCachestatusId;
    U8 ucHasFailedOperation;

    usCurrentCachestatusId = *pStartCachestatusId;

    // scan the entire cache status link to ensure that there are no pending cache status nodes in the link
    while(INVALID_4F != usCurrentCachestatusId)
    {
        if((SUBSYSTEM_STATUS_INIT == g_pCacheStatus[usCurrentCachestatusId]) || (SUBSYSTEM_STATUS_PENDING == g_pCacheStatus[usCurrentCachestatusId]))
        {
            // this is a special case for prefetch read cache status, L2 hasn't
            // configure this cache status properly, return as busy immediately
            return BUF_BUSY;
        }
        else
        {
            usPreviousCachestatusId = usCurrentCachestatusId;
            usCurrentCachestatusId = L1_GetNextCacheStatusID(usPreviousCachestatusId);
        }
    }

    // if we reach here, it means there are no pending cache status nodes in the link

    // reset the current cache status id
    usCurrentCachestatusId = *pStartCachestatusId;

    // reset the ucHasFailedOperation flag
    ucHasFailedOperation = FALSE;

    // scan the entire link again, remove all success and fail cache status nodes
    while(INVALID_4F != usCurrentCachestatusId)
    {
        if((SUBSYSTEM_STATUS_SUCCESS == g_pCacheStatus[usCurrentCachestatusId])
            || (SUBSYSTEM_STATUS_RETRY_SUCCESS == g_pCacheStatus[usCurrentCachestatusId])
            || (SUBSYSTEM_STATUS_RECC == g_pCacheStatus[usCurrentCachestatusId])
            || (SUBSYSTEM_STATUS_FAIL == g_pCacheStatus[usCurrentCachestatusId]))
        {
            // the status of the current cache status is success or fail

            if (g_CS_ID_Entry[usCurrentCachestatusId].usRelatedCachestatusId != INVALID_4F)
            {
                DBG_Printf("related cache status node error\n");
                DBG_Getch();
            }

            // set the ucHasFailedOperation flag if the status of the current node is fail
            if(g_pCacheStatus[usCurrentCachestatusId] == SUBSYSTEM_STATUS_FAIL)
            {
                ucHasFailedOperation = TRUE;
            }

            // advance to the next cache status node in the link
            *pStartCachestatusId = L1_GetNextCacheStatusID(usCurrentCachestatusId);

            // reset current cache status node
            L1_SetNextCacheStatusID(usCurrentCachestatusId, INVALID_4F);
            L1_ReleaseCacheStatus(usCurrentCachestatusId);

            usCurrentCachestatusId = *pStartCachestatusId;
        }
        else
        {
            DBG_Printf("cache status error\n");
            DBG_Getch();
        }
    }

    return (ucHasFailedOperation == TRUE) ? BUF_FAIL : BUF_IDLE;
}

/****************************************************************************
Function  : L1_ClearCacheStatusLink
Input     :  
Output    :  
Return    : 
Purpose   : clear the cache status link, used in Host Error Handle
Reference :
****************************************************************************/
void MCU1_DRAM_TEXT L1_ClearCacheStatusLink(U16 *pStartID)
{
    U16 usCurrStatusID;
    U16 usNextStatusID;

    usCurrStatusID = *pStartID;

    /* invlid CacheStatus Link */
    *pStartID = INVALID_4F;

    while (INVALID_4F != usCurrStatusID)
    {
        /* get next node */
        usNextStatusID = L1_GetNextCacheStatusID(usCurrStatusID);

        /* remove current node */
        g_pCacheStatus[usCurrStatusID] = SUBSYSTEM_STATUS_SUCCESS;
        L1_SetNextCacheStatusID(usCurrStatusID, INVALID_4F);
        L1_ReleaseCacheStatus(usCurrStatusID);

        /* update current node */
        usCurrStatusID = usNextStatusID;
    }

    return;
}

/****************************************************************************
Function  : L1_CheckCacheStatusLinkIdle
Input     :  
Output    :  
Return    : 
Purpose   : check if the given cache link all idle for all ranges
                remove finished cache status from link to FIFO
Reference :
****************************************************************************/
BOOL L1_CheckCacheStatusLinkIdle(U16 *pStartID)
{
    L1_CheckCacheStatus(pStartID, 0, INVALID_4F);

    if (INVALID_4F == *pStartID)
    {
        return BUF_IDLE;
    }

    return BUF_BUSY;
}

/****************************************************************************
Function  : L1_RecycleCacheStatusID
Input     :  
Output    :  
Return    : 
Purpose   : check and release all finished cache status in all links, 
                call after CacheStatus FIFO full
Reference :
****************************************************************************/
void L1_RecycleCacheStatusID(void)
{
    U16 usLoop;
    U16 usLogBufID;
    BUFF_TAG *pBufTag;

    usLogBufID = LGCBUFID_FROM_PHYBUFID(g_ulWriteBufStartPhyId);

    for (usLoop = 0; usLoop < L1_BUFFER_COUNT_WRITE; usLoop++, usLogBufID++)
    {
        pBufTag  = &gpBufTag[usLogBufID];
      
        L1_CheckCacheStatus(&(pBufTag->usStartHRID), 0, 0);
        L1_CheckCacheStatus(&(pBufTag->usStartHWID), 0, 0);
        L1_CheckCacheStatus(&(pBufTag->usStartNRID), 0, 0);
        L1_CheckCacheStatus(&(pBufTag->usStartNWID), 0, 0);
    }

#ifdef HOST_READ_FROM_DRAM
    usLogBufID = LGCBUFID_FROM_PHYBUFID(g_ulRFDBufStartPhyId);

    for (usLoop = 0; usLoop < L1_BUFFER_COUNT_READ_FROM_DRAM; usLoop++, usLogBufID++)
    {
        pBufTag  = &gpBufTag[usLogBufID];
      
        L1_CheckCacheStatus(&(pBufTag->usStartHRID), 0, 0);
        L1_CheckCacheStatus(&(pBufTag->usStartNRID), 0, 0);
    }
#endif

    return;
}

/****************************************************************************
Function  : L1_AddPrefetchCacheStatusToLink
Input     :  usPhyBufID
Output    :  
Return    : 
Purpose   : 
    allocate a cache status for read prefetch and add to link, set the value to init, range to invalid
Reference :
****************************************************************************/
void L1_AddPrefetchCacheStatusToLink(U16 usPhyBufID)
{
    U16 usLogBufID;
    U32 ulCStatusAddr;

    usLogBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

    ulCStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[usLogBufID].usStartNRID), 0, 0);

    *(U8 *)ulCStatusAddr = SUBSYSTEM_STATUS_INIT;

    return;
}

/****************************************************************************
Function  : L1_L2GetPrefetchCacheStatusAddr
Input     :  usPhyBufID
Output    :  
Return    : 
Purpose   : 
    find a unused cache status for read prefetch and add to link,
    then return read prefetch cache status address to L2
Reference :
****************************************************************************/
U32 L1_L2GetPrefetchCacheStatusAddr(U16 usPhyBufID, U16 usSecOffset, U16 usSecLen)
{
    U8  ucLoop;
    U16 usLogBufID;
    U16 usCStstusID;
    U32 ulCStatusAddr;

    usLogBufID    = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    usCStstusID   = gpBufTag[usLogBufID].usStartNRID;
    ulCStatusAddr = 0;

    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
#ifdef SIM
        if (INVALID_4F == usCStstusID)
        {
            DBG_Printf("L1_L2GetPrefetchCacheStatusAddr usCStstusID ERROR\n");
            DBG_Getch();
        }
#endif
        /* find a cache status with init value for L2, and update the range infos and status value */
        if (SUBSYSTEM_STATUS_INIT == g_pCacheStatus[usCStstusID])
        {
            L1_UpdateCacheStatusRangeInfo(usCStstusID, (usSecOffset & SEC_PER_BUF_MSK), usSecLen);
            ulCStatusAddr = L1_GetCacheStatusAddr(usCStstusID);
            *(U8 *)ulCStatusAddr = SUBSYSTEM_STATUS_PENDING;
            return ulCStatusAddr;
        }

        /* check nextID */
        usCStstusID = L1_GetNextCacheStatusID(usCStstusID);
    }

    return ulCStatusAddr;
}

/****************************************************************************
Function  : L1_L2SubPrefetchCacheStatus
Input     :  usPhyBufID
Output    :  
Return    : 
Purpose   : 
    subtract a unused cache status node for prefetch buffer
Reference :
****************************************************************************/
void L1_L2SubPrefetchCacheStatus(U16 usPhyBufID)
{
    U8  ucLoop;
    U16 usLogBufID;
    U16 usCurrStatusID;
    U16 usPrevStatusID;
    U32 ulCStatusAddr;

    usLogBufID     = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    usCurrStatusID = gpBufTag[usLogBufID].usStartNRID;
    usPrevStatusID = INVALID_4F;

    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
#ifdef SIM
        if (INVALID_4F == usCurrStatusID)
        {
            DBG_Printf("L1_L2SubPrefetchCacheStatus usCStstusID ERROR\n");
            DBG_Getch();
        }
#endif
        /* find a cache status with init value for L2, and update the range infos and status value */
        if (SUBSYSTEM_STATUS_INIT == g_pCacheStatus[usCurrStatusID])
        {
            ulCStatusAddr = L1_GetCacheStatusAddr(usCurrStatusID);
            *(U8 *)ulCStatusAddr = SUBSYSTEM_STATUS_SUCCESS;

            if (usCurrStatusID == gpBufTag[usLogBufID].usStartNRID)
            {
                /* update startID */
                gpBufTag[usLogBufID].usStartNRID = L1_GetNextCacheStatusID(usCurrStatusID);
                
                /* remove current node */
                L1_SetNextCacheStatusID(usCurrStatusID, INVALID_4F);
                L1_ReleaseCacheStatus(usCurrStatusID);
            }
            else
            {
#ifdef SIM
                if (INVALID_4F == usPrevStatusID)
                {
                    DBG_Printf("L1_L2SubPrefetchCacheStatus usPrevStatusID ERROR\n");
                    DBG_Getch();
                }
#endif
                /* update PrevID to the nextID of currentID */
                L1_SetNextCacheStatusID(usPrevStatusID, L1_GetNextCacheStatusID(usCurrStatusID));

                /* remove current node */
                L1_SetNextCacheStatusID(usCurrStatusID, INVALID_4F);
                L1_ReleaseCacheStatus(usCurrStatusID);
            }

            return;
        }

        /* check nextID */
        usPrevStatusID = usCurrStatusID;
        usCurrStatusID = L1_GetNextCacheStatusID(usPrevStatusID);
    }

    DBG_Printf("L1_L2SubPrefetchCacheStatus ERROR\n");
    DBG_Getch();
    return;
}

/********************** FILE END ***************/

