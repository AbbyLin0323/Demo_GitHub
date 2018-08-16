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
Filename     : L1_Buffer.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20121126 blakezhang modify for L1 ramdisk design
20120118 peterxiu 001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "HAL_TraceLog.h"
#include "COM_BitMask.h"

//specify file name for Trace Log
#define TL_FILE_NUM  L1_Buffer_c

#ifdef DATA_MONITOR_ENABLE
#include "FW_DataMonitor.h"
#endif

/* Buffer Tag, each tag per buffer */
GLOBAL BUFF_TAG *gpBufTag;
GLOBAL MCU12_VAR_ATTR U16 s_aBufReadEntry[L1_BUFFER_COUNT_READ];

#ifndef HOST_SATA
// the following structures are only needed for nvme mode,
// since read buffers don't need to be managed explictly in sata mode
GLOBAL  U16 s_ausReadBufferCacheStatus[L1_BUFFER_COUNT_READ];
GLOBAL  U16 s_usReadBufferHead;
GLOBAL  U16 s_usReadBufferTail;
GLOBAL  U16 s_usReadBufferAllocatedCount;
#endif

/* PU Fifo */
GLOBAL  WRITE_BUF_FIFO_ENTRY  g_WriteBufFifoEntry;
GLOBAL  WRITE_BUF_FIFO_ENTRY *gpWriteBufFifoEntry;

/* each buffer will store the LPN address and its corresponding Sector bitmap */
GLOBAL  U32 g_ulBufferLpnBaseAddr;

/*read buffer dont need this, should change read partial hit code*/
GLOBAL  BUF_SECTOR_BITMAP  g_BufSectorBitMap;
GLOBAL  BUF_SECTOR_BITMAP *gpBufSectorBitMap;

#ifdef HOST_READ_FROM_DRAM
GLOBAL MCU12_VAR_ATTR U32 g_ulRFDFullBitmap;
GLOBAL MCU12_VAR_ATTR RFD_BUF_ENTRY g_RFDEntry;
GLOBAL MCU12_VAR_ATTR RFD_BUF_ENTRY *g_pRFDEntry;
extern GLOBAL  SUBCMD *g_pCurSubCmd;
extern GLOBAL  PSCMD g_pCurSCmd;
#endif

GLOBAL  WRITE_BUF_FIFO_INFO g_WriteBufInfo[SUBSYSTEM_SUPERPU_MAX];

GLOBAL  U32 g_ulFreeCacheAvailableBitmap;
GLOBAL  U32 g_ulActiveWriteBufferBitmap;
GLOBAL  U32 g_ulMergeBufferBitmap;
GLOBAL  U32 g_ulFlushBufferBitmap;

GLOBAL  U8 g_ucCurMergePos[SUBSYSTEM_SUPERPU_MAX];


/*L1 buffer base address and start id related variable definition*/
GLOBAL  U32 g_ulBufferStartPhyId;
GLOBAL  U32 g_ulBufferBaseAddr;


GLOBAL  U32 g_ulReadBufStartPhyId;

GLOBAL  U32 g_ulWriteBufStartPhyId;

#ifdef HOST_READ_FROM_DRAM
GLOBAL  U32 g_ulRFDBufStartPhyId;
#endif

GLOBAL  U32 g_L1TempBufferAddr;
GLOBAL  U32 g_L1TempBufferPhyId;

GLOBAL  U32 g_L1InvalidBufferAddr;
GLOBAL  U32 g_L1InvalidBufferPhyId;

GLOBAL  U8 g_ucIsBufferRegulationOngoing[SUBSYSTEM_SUPERPU_MAX];

void MCU1_DRAM_TEXT L1_BufferSram0Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign( &ulFreeSramBase );

    gpBufTag = (BUFF_TAG *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(L1_BUFFER_TOTAL * sizeof(BUFF_TAG)));

    // g_ulBufferLpnBaseAddr
    if( 4 >= SUBSYSTEM_SUPERPU_NUM ) // 4 PU: DSRAM08 PU: DSRAM116 PU: DSRAM1 (or DRAM)
    {
        //LPN array size:Buffer Total Count*LPN Per Buffer*DWORD Size
        g_ulBufferLpnBaseAddr = ulFreeSramBase;
        COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign( L1_BUFFER_TOTAL*LPN_PER_BUF*DWORD_SIZE ) );
    }

    COM_MemAddr16DWAlign( &ulFreeSramBase );
    *pFreeSramBase = ulFreeSramBase;
    return;
}

void MCU1_DRAM_TEXT L1_BufferSram1Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign( &ulFreeSramBase );

    // g_ulBufferLpnBaseAddr
    if( 4 < SUBSYSTEM_SUPERPU_NUM ) // 4 PU: DSRAM08 PU: DSRAM116 PU: DSRAM1 (or DRAM)
    {
        //LPN array size:Buffer Total Count*LPN Per Buffer*DWORD Size
        g_ulBufferLpnBaseAddr = ulFreeSramBase;
        COM_MemIncBaseAddr(&ulFreeSramBase, (L1_BUFFER_TOTAL*LPN_PER_BUF*DWORD_SIZE));
        COM_MemAddr16DWAlign(&ulFreeSramBase);
    }

    // gpBufSectorBitMap->ucBitMap
    gpBufSectorBitMap = &g_BufSectorBitMap;
    gpBufSectorBitMap->ucBitMap = ( U8 (*)[LPN_PER_BUF] )ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign( L1_BUFFER_TOTAL * sizeof(gpBufSectorBitMap->ucBitMap[0]) ) );
    COM_MemAddr16DWAlign( &ulFreeSramBase );


#ifdef HOST_READ_FROM_DRAM
    if( 8 >= SUBSYSTEM_SUPERPU_NUM )   // 4/8PU:DSRAM1 16PU:DRAM
    {
        g_pRFDEntry = &g_RFDEntry;
        g_pRFDEntry->tRFDBuffer = ( RFD_BUFFER  * )ulFreeSramBase;
        COM_MemIncBaseAddr( &ulFreeSramBase, SUBSYSTEM_SUPERPU_NUM*sizeof(g_pRFDEntry->tRFDBuffer[0]) );
        COM_MemAddr16DWAlign(&ulFreeSramBase);
    }
#endif

    COM_MemAddr16DWAlign( &ulFreeSramBase );
    *pFreeSramBase = ulFreeSramBase;
    return;
}

/****************************************************************************
Name        :L1_BufferDramMap
Input       :ulDramStartAddr
Output      :
Author      :HenryLuo
Date        :2012.03.01    17:41:13
Description :allcate dram for buffer module.
Others      :
Modify      :
2014.2.17 kristinwang
****************************************************************************/
void MCU1_DRAM_TEXT L1_BufferDramMap(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;

    COM_MemAddrPageBoundaryAlign(pFreeDramBase);
    ulFreeDramBase = *pFreeDramBase;

    /*Read Buffer and Write Buffer*/
#if 0 //defined(HOST_SATA)
    g_ulReadBufStartPhyId = COM_GetBufferIDByMemAddr(DRAM_READ_BUFFER_BASE,TRUE,BUF_SIZE_BITS);
#endif
    
    g_ulBufferBaseAddr   = ulFreeDramBase;
    g_ulBufferStartPhyId = COM_GetBufferIDByMemAddr(g_ulBufferBaseAddr,TRUE,BUF_SIZE_BITS);

    g_ulWriteBufStartPhyId    = g_ulBufferStartPhyId;
#ifdef HOST_READ_FROM_DRAM
    g_ulRFDBufStartPhyId      = g_ulWriteBufStartPhyId + L1_BUFFER_COUNT_WRITE;
#endif
    COM_MemIncBaseAddr( &ulFreeDramBase, COM_MemSize16DWAlign(L1_BUFFER_TOTAL * BUF_SIZE) );
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

#if 1 //ndef HOST_SATA
    /* for nvme mode, instead of using a specialized DRAM location,
     read buffers are allocated immediately after write buffers,
     please note that HOST_READ_FROM_DRAM is obsolete and shouldn't be used */

    g_ulReadBufStartPhyId = COM_GetBufferIDByMemAddr(ulFreeDramBase, TRUE, BUF_SIZE_BITS);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(L1_BUFFER_COUNT_READ * BUF_SIZE));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
#endif

    g_L1InvalidBufferAddr  = ulFreeDramBase;
    g_L1InvalidBufferPhyId = COM_GetBufferIDByMemAddr(ulFreeDramBase,TRUE,BUF_SIZE_BITS);
    COM_MemIncBaseAddr( &ulFreeDramBase, COM_MemSize16DWAlign( BUF_SIZE ) );
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    g_L1TempBufferAddr  = ulFreeDramBase;
    g_L1TempBufferPhyId = COM_GetBufferIDByMemAddr(ulFreeDramBase,TRUE,BUF_SIZE_BITS);
    COM_MemIncBaseAddr( &ulFreeDramBase, COM_MemSize16DWAlign( BUF_SIZE ) );
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

#ifdef HOST_READ_FROM_DRAM
    if( 8 < SUBSYSTEM_SUPERPU_NUM )   // 4/8PU:DSRAM1 16PU:DRAM
    {
        g_pRFDEntry = &g_RFDEntry;
        g_pRFDEntry->tRFDBuffer = ( RFD_BUFFER  * )ulFreeDramBase;
        COM_MemIncBaseAddr( &ulFreeDramBase, COM_MemSize16DWAlign( SUBSYSTEM_SUPERPU_NUM*sizeof(g_pRFDEntry->tRFDBuffer[0]) ) );
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    }
#endif

    *pFreeDramBase = ulFreeDramBase;

    return;
}

/****************************************************************************
Name        :L1_SetBufferLPN()
Input       :  LogicBufferID, LPN postion
Output      :BufferLPN()
Author      :BlakeZhang 
Date        :2012.11.19
Description :calculate the addr of BufferLPN. 
Others      :
Modify      :
****************************************************************************/
void L1_SetBufferLPN(U16 usLogBufId, U8 ucLpnOffset, U32 ulLpn)
{
    // Sean Gao 20150602
    // this function updates the LPN stored in a particular buffer entry,
    // it also updates the valid LPN bitmap for the buffer

    U32 ulTargetAddr;

    // calculate the target address for the particular buffer entry
    ulTargetAddr = g_ulBufferLpnBaseAddr + (usLogBufId << (LPN_PER_BUF_BITS + DWORD_SIZE_BITS)) + (ucLpnOffset << DWORD_SIZE_BITS);

    // update the LPN stored in the buffer entry
    *((U32*)ulTargetAddr) = ulLpn;

#ifdef L1_BUFFER_REGULATION
    // update the valid LPN bitmap
    if(ulLpn != INVALID_8F)
    {
        // the LPN is valid, set the corresponding bit in the valid LPN bitmap
        COM_BitMaskSet(&gpBufTag[usLogBufId].ulValidLpnBitmap, ucLpnOffset);
    }
    else
    {
        // the LPN is invalid, clear the corresponding bit in the valid LPN bitmap
        COM_BitMaskClear(&gpBufTag[usLogBufId].ulValidLpnBitmap, ucLpnOffset);
    }
#endif

    return;
}

/****************************************************************************
Name        :L1_GetBufferLPN()
Input       :  LogicBufferID, LPN postion
Output      :BufferLPNAdd()
Author      :BlakeZhang 
Date        :2012.11.19   
Description :calculate the addr of BufferLPN. 
Others      :
Modify      :
****************************************************************************/
U32 L1_GetBufferLPN(U16 LogicBufferID, U8 CacheOffset)
{
     U32 addr;
     
     addr = g_ulBufferLpnBaseAddr + (LogicBufferID << (LPN_PER_BUF_BITS + DWORD_SIZE_BITS)) + (CacheOffset << DWORD_SIZE_BITS);

     return ( *(U32*)addr ) ;
}

/****************************************************************************
Function  : L1BufferInit
Input     : 
Output    : 
Return    : 

Purpose   : 
buffer data structure init,buffer allocate.

Reference :
There are two kind of buffer,sequence buffer and random buffer.
Modification History:
20121120   Blake Zhang modify for L1 ramdisk design
20120209   Brooke Wang create detailed code
20120118   peterxiu   001 first create function
20140214   kristinwang  move dram map out,manage all buffer,copy from vt3514 SATA 0.99
****************************************************************************/
void MCU1_DRAM_TEXT L1_BufferInit(void)
{
    U16 usLogicBufferID =0 ;
    U16 usPhyBufferID =0 ;
    U16 i, j;
    U32 ulInvalidValue;

    gpBufSectorBitMap   = &g_BufSectorBitMap;
    gpWriteBufFifoEntry = &g_WriteBufFifoEntry;
    
    COM_MemZero( (U32 *)g_WriteBufInfo, sizeof(WRITE_BUF_FIFO_INFO)*SUBSYSTEM_SUPERPU_MAX/sizeof(U32) );

    /* init BufTag Entry */
    for (i = 0; i < L1_BUFFER_TOTAL; i++)
    {
        gpBufTag[i].Stage          = BUF_STAGE_FREE; // 0
        gpBufTag[i].bSeqBuf        = 0;
        gpBufTag[i].bPrefetchBuf   = FALSE;
        gpBufTag[i].bIsCertainHit  = FALSE;
        gpBufTag[i].ucWritePointer = 0;
        gpBufTag[i].usNextBufId = INVALID_4F;

        gpBufTag[i].usStartHRID = INVALID_4F;
        gpBufTag[i].usStartHWID = INVALID_4F;
        gpBufTag[i].usStartNRID = INVALID_4F;
        gpBufTag[i].usStartNWID = INVALID_4F;

#ifdef L1_BUFFER_REGULATION
        gpBufTag[i].ulValidLpnBitmap = 0;
#endif
    }

    /* initialize read buffer entries */
    for(i = 0; i < L1_BUFFER_COUNT_READ; i++)
    {
        s_aBufReadEntry[i] = g_ulReadBufStartPhyId + i;

#ifndef HOST_SATA
        s_ausReadBufferCacheStatus[i] = INVALID_4F;
        s_usReadBufferHead = 0;
        s_usReadBufferTail = 0;
        s_usReadBufferAllocatedCount = 0;
#endif
    }

    /*Write Buffer infomation*/
    usLogicBufferID = g_ulWriteBufStartPhyId - g_ulBufferStartPhyId;
    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        // reset start buffer id of all buffer links
        g_WriteBufInfo[i].usFreeCacheStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usMergePendingStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushPendingStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushOngoingStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usReadCacheStartBufId = INVALID_4F;

        // reset end buffer id of all buffer links
        g_WriteBufInfo[i].usFreeCacheEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usMergePendingEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushPendingEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushOngoingEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usReadCacheEndBufId = INVALID_4F;

        // reset buffer count of all buffer links
        g_WriteBufInfo[i].ucFreeCacheBufCnt = 0;
        g_WriteBufInfo[i].ucMergePendingBufCnt = 0;
        g_WriteBufInfo[i].ucFlushPendingBufCnt = 0;
        g_WriteBufInfo[i].ucFlushOngoingBufCnt = 0;
        g_WriteBufInfo[i].ucReadCacheBufCnt = 0;

        g_ulMergeBufferBitmap = 0;
        g_ulFlushBufferBitmap = 0;

        for (j = 0; j < L1_WRITE_BUFFER_PER_PU; j++)
        {
            // add the current buffer to the free cache buffer
            // link
            L1_AddBufToLinkTail((U8)i, FREE_CACHE_LINK, usLogicBufferID);
            /* init RFD Entry */
            gpWriteBufFifoEntry->usPhyBufID[i][j] = PHYBUFID_FROM_LGCBUFID(usLogicBufferID);       
            usLogicBufferID++;
        }
    }

#ifdef HOST_READ_FROM_DRAM
    g_ulRFDFullBitmap = 0;
    usLogicBufferID   = g_ulRFDBufStartPhyId - g_ulBufferStartPhyId;

    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_pRFDEntry->tRFDBuffer[i].ucHead = 0;
        g_pRFDEntry->tRFDBuffer[i].ucTail = 0;
        g_pRFDEntry->tRFDBuffer[i].ucRecycle = 0;
        g_pRFDEntry->tRFDBuffer[i].ucUsedCnt = 0;
      
        for (j = 0; j < L1_READ_FROM_DRAM_BUF_PER_PU; j++)
        {
            /* init RFD Entry */
            g_pRFDEntry->tRFDBuffer[i].aRFDPhyBufID[j] = PHYBUFID_FROM_LGCBUFID(usLogicBufferID);
            
            usLogicBufferID++;
        }
    }
#endif

    /* clear all buffer LPN and Sector Bitmap */
    HAL_DMAESetValue((U32)g_ulBufferLpnBaseAddr, COM_MemSize16DWAlign(L1_BUFFER_TOTAL * LPN_PER_BUF * DWORD_SIZE), INVALID_8F);

    for(i = 0; i < L1_BUFFER_TOTAL; i++)
    {
        for (j = 0; j < LPN_PER_BUF; j++)
        {
            gpBufSectorBitMap->ucBitMap[i][j] = 0;
        }
    }

    /* init invalid buffer */
#ifdef SIM
    ulInvalidValue = INVALID_8F;
#else
    ulInvalidValue = 0;
#endif

    HAL_DMAESetValue(g_L1InvalidBufferAddr, BUF_SIZE, ulInvalidValue);


    // initialize the g_ulFreeCacheAvailableBitmap
    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        COM_BitMaskSet(&g_ulFreeCacheAvailableBitmap, (U8)i);
    }

    // initialize buffer regulation status
    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_ucIsBufferRegulationOngoing[i] = FALSE;
    }

    return;
}

/****************************************************************************
Function  : L1_BufferWarmInit
Input     : 
Output    : 
Return    : 
Purpose   : 
Reference :
Modification History:
20150207   Blake Zhang created
****************************************************************************/
void MCU1_DRAM_TEXT L1_BufferWarmInit(void)
{
    U16 i, j;
    U16 usLogicBufferID;

    /* Init Write buffer FIFO Info */
    //COM_MemZero( (U32 *)g_pWriteBufInfo, sizeof(WRITE_BUF_FIFO_INFO_ENTRY)/sizeof(U32) );

    /* init BufTag Entry */
    for (i = 0; i < L1_BUFFER_TOTAL; i++)
    {
        gpBufTag[i].Stage          = BUF_STAGE_FREE;
        gpBufTag[i].bSeqBuf        = 0;
        gpBufTag[i].bPrefetchBuf   = FALSE;
        gpBufTag[i].bIsCertainHit  = FALSE;
        gpBufTag[i].ucWritePointer = 0;
        gpBufTag[i].usNextBufId = INVALID_4F;

        gpBufTag[i].usStartHRID = INVALID_4F;
        gpBufTag[i].usStartHWID = INVALID_4F;
        gpBufTag[i].usStartNRID = INVALID_4F;
        gpBufTag[i].usStartNWID = INVALID_4F;

#ifdef L1_BUFFER_REGULATION
        gpBufTag[i].ulValidLpnBitmap = 0;
#endif
    }

    /* initialize read buffer entries */
    for(i = 0; i < L1_BUFFER_COUNT_READ; i++)
    {
        s_aBufReadEntry[i] = g_ulReadBufStartPhyId + i;

#ifndef HOST_SATA
        s_ausReadBufferCacheStatus[i] = INVALID_4F;
        s_usReadBufferHead = 0;
        s_usReadBufferTail = 0;
        s_usReadBufferAllocatedCount = 0;
#endif
    }

    /*Write Buffer infomation*/
    usLogicBufferID = g_ulWriteBufStartPhyId - g_ulBufferStartPhyId;
    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        // reset start buffer id of all buffer links
        g_WriteBufInfo[i].usFreeCacheStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usMergePendingStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushPendingStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushOngoingStartBufId = INVALID_4F;
        g_WriteBufInfo[i].usReadCacheStartBufId = INVALID_4F;

        // reset end buffer id of all buffer links
        g_WriteBufInfo[i].usFreeCacheEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usMergePendingEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushPendingEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usFlushOngoingEndBufId = INVALID_4F;
        g_WriteBufInfo[i].usReadCacheEndBufId = INVALID_4F;

        // reset buffer count of all buffer links
        g_WriteBufInfo[i].ucFreeCacheBufCnt = 0;
        g_WriteBufInfo[i].ucMergePendingBufCnt = 0;
        g_WriteBufInfo[i].ucFlushPendingBufCnt = 0;
        g_WriteBufInfo[i].ucFlushOngoingBufCnt = 0;
        g_WriteBufInfo[i].ucReadCacheBufCnt = 0;

        g_ulMergeBufferBitmap = 0;
        g_ulFlushBufferBitmap = 0;

        for (j = 0; j < L1_WRITE_BUFFER_PER_PU; j++)
        {
            // add the current buffer to the free cache buffer
            // link
            L1_AddBufToLinkTail((U8)i, FREE_CACHE_LINK, usLogicBufferID);
            /* init RFD Entry */
            gpWriteBufFifoEntry->usPhyBufID[i][j] = PHYBUFID_FROM_LGCBUFID(usLogicBufferID);       
            usLogicBufferID++;
        }
    }

#ifdef HOST_READ_FROM_DRAM
    L1_RFDBufferReset();
#endif

    /* clear all buffer LPN and Sector Bitmap */
    HAL_DMAESetValue((U32)g_ulBufferLpnBaseAddr, COM_MemSize16DWAlign(L1_BUFFER_TOTAL * LPN_PER_BUF * DWORD_SIZE), INVALID_8F);

    for(i = 0; i < L1_BUFFER_TOTAL; i++)
    {
        for (j = 0; j < LPN_PER_BUF; j++)
        {
            gpBufSectorBitMap->ucBitMap[i][j] = 0;
        }
    }


    // initialize the g_ulFreeCacheAvailableBitmap
    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        COM_BitMaskSet(&g_ulFreeCacheAvailableBitmap, (U8)i);
    }

    // initialize buffer regulation status
    for(i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_ucIsBufferRegulationOngoing[i] = FALSE;
    }

    return;
}


/****************************************************************************
Description  : allocate a free read buffer to read subcmd
               calculate CacheStatus address
               set CacheStatus busy
Input     : SUBCMD *  
Output    : pSubCmd->SubCmdPhyBufferID
            pSubCmd->CacheStatusAddr
Return    : BOOL      SUCCESS -- allocate OK
                      FAIL -- no free read buffer ,allocate false
*****************************************************************************/
U16 L1_AllocateReadBuf(SUBCMD *pSubCmd)
{ 
#ifdef HOST_SATA
    // read buffer allocation for sata mode
    if (pSubCmd->SubCmdHostInfo.ucSubDSGId >= L1_BUFFER_COUNT_READ)
    {
        DBG_Printf("L1_AllocateReadBuf ucSubDSGId 0x%x ERROR!\n", pSubCmd->SubCmdHostInfo.ucSubDSGId);
        DBG_Getch();
    }

    pSubCmd->SubCmdPhyBufferID = s_aBufReadEntry[pSubCmd->SubCmdHostInfo.ucSubDSGId];         

    return TRUE;
#else
    // read buffer allocation for nvme mode

    if(s_usReadBufferAllocatedCount < L1_BUFFER_COUNT_READ)
    {

        U16 usCurrentReadBufferHead = s_usReadBufferHead;

#ifdef SIM
        // perform a validity check of the cache status of the read buffer
        if(s_ausReadBufferCacheStatus[usCurrentReadBufferHead] != INVALID_4F)
        {
            DBG_Printf("read buffer cache status error\n");
            DBG_Getch();
        }
#endif

        pSubCmd->SubCmdPhyBufferID = s_aBufReadEntry[usCurrentReadBufferHead];

        s_usReadBufferAllocatedCount++;

        s_usReadBufferHead = ((usCurrentReadBufferHead+1) == L1_BUFFER_COUNT_READ) ? 0 : (usCurrentReadBufferHead+1);

        return pSubCmd->SubCmdPhyBufferID;
    }
    else // no free read buffers
    {

        U16 usCurrentReadBufferTail = s_usReadBufferTail;

        // try recycling the read buffer in the tail
        if(L1_CheckCacheStatusLinkIdle(&(s_ausReadBufferCacheStatus[usCurrentReadBufferTail])) == BUF_IDLE)
        {

            s_usReadBufferAllocatedCount--;
            s_usReadBufferTail = ((usCurrentReadBufferTail+1) == L1_BUFFER_COUNT_READ) ? 0 : (usCurrentReadBufferTail+1);
        }

        return INVALID_4F;
    }
#endif
}

#ifndef HOST_SATA
// read buffer management functions for nvme mode
U16* L1_GetReadBufferEntryCachestatus(U16 usReadBufferPhyId)
{
    U16 usReadBufferEntry;

#ifdef SIM
    // perform validity check on the input read buffer physical id
    if((usReadBufferPhyId < g_ulReadBufStartPhyId) || (usReadBufferPhyId >= (g_ulReadBufStartPhyId+L1_BUFFER_COUNT_READ)))
    {
        DBG_Printf("read buffer physical id error\n");
        DBG_Getch();
    }
#endif

    usReadBufferEntry = usReadBufferPhyId - g_ulReadBufStartPhyId;

#ifdef SIM
    // perform a validity check of the cache status of the read buffer
    if(s_ausReadBufferCacheStatus[usReadBufferEntry] != INVALID_4F)
    {
        DBG_Printf("read buffer cache status error\n");
        DBG_Getch();
    }
#endif

    return &(s_ausReadBufferCacheStatus[usReadBufferEntry]);
}

U8 L1_ReadBufferManagement(void)
{
    // try recycling a read buffer
    if(s_usReadBufferAllocatedCount > 0)
    {

        U16 usCurrentReadBufferTail = s_usReadBufferTail;


        if(L1_CheckCacheStatusLinkIdle(&(s_ausReadBufferCacheStatus[usCurrentReadBufferTail])) == BUF_IDLE)
        {
            s_usReadBufferAllocatedCount--;
            s_usReadBufferTail = ((usCurrentReadBufferTail+1) == L1_BUFFER_COUNT_READ) ? 0 : (usCurrentReadBufferTail+1);
        }

        return TRUE;
    }
    else // there are no pending read buffers
    {

        U8 i;

        for(i = 0; i < L1_BUFFER_COUNT_READ; ++i)
        {
            if(s_ausReadBufferCacheStatus[i] != INVALID_4F)
            {
                DBG_Printf("read buffer cache status error, should be invalid\n");
                DBG_Getch();
            }
        }

        return FALSE;
    }
}
#endif

#ifdef HOST_READ_FROM_DRAM
void MCU1_DRAM_TEXT L1_RFDBufferReset(void)
{
    U8 ucPuNum;
  
    g_ulRFDFullBitmap = 0;
    
    for(ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        g_pRFDEntry->tRFDBuffer[ucPuNum].ucHead = 0;
        g_pRFDEntry->tRFDBuffer[ucPuNum].ucTail = 0;
        g_pRFDEntry->tRFDBuffer[ucPuNum].ucRecycle = 0;
        g_pRFDEntry->tRFDBuffer[ucPuNum].ucUsedCnt = 0;
    }

    return;
}

BOOL L1_IsRFDBufferFull(U8 ucPuNum)
{
    if (g_pRFDEntry->tRFDBuffer[ucPuNum].ucUsedCnt >= (L1_READ_FROM_DRAM_BUF_PER_PU - 1))
    {
        /* RFD buffer full */
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL L1_IsRFDBufferEmpty(U8 ucPuNum)
{
    if (0 != g_pRFDEntry->tRFDBuffer[ucPuNum].ucUsedCnt)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL L1_AllocateReadFromDRAMBuf(SUBCMD *pSubCmd)
{
    U8  ucPuNum;
    U8  ucTail;
    U16 usPhyBufID;
    RFD_BUFFER *pRFDBuffer;

    ucPuNum = pSubCmd->SubCmdAddInfo.ucPuNum;

#ifdef SIM
    if (TRUE == L1_IsRFDBufferFull(ucPuNum))
    {
        DBG_Printf("L1_AllocateReadFromDRAMBuf PU 0x%x FULL ERROR\n", ucPuNum);
        DBG_Getch();
    }
#endif

    pRFDBuffer = &(g_pRFDEntry->tRFDBuffer[ucPuNum]);
    ucTail     = pRFDBuffer->ucTail;
    usPhyBufID = pRFDBuffer->aRFDPhyBufID[ucTail];

    pSubCmd->SubCmdPhyBufferID = usPhyBufID;

    /* save SubCmd Infos */
    COM_MemCpy((U32*)(&pRFDBuffer->tRFDSCMD[ucTail]), (U32*)pSubCmd->pSCMD, sizeof(SCMD)/sizeof(U32));
    COM_MemCpy((U32*)(&pRFDBuffer->tRFDSubCmd[ucTail]), (U32*)pSubCmd, sizeof(SUBCMD)/sizeof(U32));

    pSubCmd->pSCMD = (&pRFDBuffer->tRFDSCMD[ucTail]);

    /* update Tail pointer */
    ucTail++;
    if (ucTail >= L1_READ_FROM_DRAM_BUF_PER_PU)
    {
        ucTail = 0;
    }

    pRFDBuffer->ucTail = ucTail;
    pRFDBuffer->ucUsedCnt++;

    if (TRUE == L1_IsRFDBufferFull(ucPuNum))
    {
        /* update RFD buffer PU full bitmap */
        g_ulRFDFullBitmap |= (1 << ucPuNum);
    }

    return SUCCESS;
}

BOOL L1_ResumeRFDBuf(U8 ucPuNum)
{
    U8  ucHead;
    U16 usPhyBufID;
    U16 usLogicBufID;
    RFD_BUFFER *pRFDBuffer;

    pRFDBuffer = &(g_pRFDEntry->tRFDBuffer[ucPuNum]);

    ucHead = pRFDBuffer->ucHead;

    /* no Read from DRAM buffer to resume */
    if (ucHead == pRFDBuffer->ucTail)
    {
        return FALSE;
    }

    usPhyBufID   = pRFDBuffer->aRFDPhyBufID[ucHead];
    usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    
    /* check buffer CacheStatus */
    if(BUF_BUSY == L1_ReadPreFetchCheckAllBufBusy(usPhyBufID))
    {
        return FALSE;
    }

    /* resume SubCmd Infos */
    g_pCurSCmd   = &pRFDBuffer->tRFDSCMD[pRFDBuffer->ucHead];
    g_pCurSubCmd = &pRFDBuffer->tRFDSubCmd[pRFDBuffer->ucHead];

    g_pCurSubCmd->pSCMD = g_pCurSCmd;

#ifdef SIM
    if (g_pCurSubCmd->SubCmdPhyBufferID != usPhyBufID)
    {
        DBG_Printf("L1_ResumeRFDBuf usPhyBufID 0x%x ERROR\n", usPhyBufID);
        DBG_Getch();
    }
#endif

    /* set SubCmd Infos for AHCI IO */
    g_pCurSubCmd->SubCmdAddInfo.ucSyncFlag = TRUE;
    g_pCurSubCmd->SubCmdStage = SUBCMD_STAGE_CACHE;
    g_pCurSubCmd->CacheStatusAddr = L1_AddCacheStatusToLink(&gpBufTag[usLogicBufID].usStartHRID, 
          g_pCurSCmd->tMA.ulSubSysLBA, (U16)g_pCurSCmd->tMA.ucSecLen) - OTFB_START_ADDRESS;

    ucHead++;
    if (ucHead >= L1_READ_FROM_DRAM_BUF_PER_PU)
    {
        ucHead = 0;
    }

    pRFDBuffer->ucHead = ucHead;

    return TRUE;
}

void L1_RecycleRFDBuf(U8 ucPuNum)
{
    U8  ucRecycle;
    U16 usPhyBufID;
    U16 usLogicBufID;
    RFD_BUFFER *pRFDBuffer;

    pRFDBuffer = &(g_pRFDEntry->tRFDBuffer[ucPuNum]);
  
    ucRecycle = pRFDBuffer->ucRecycle;

    /* Recycle == Flush, no need for recycle */
    while(ucRecycle != pRFDBuffer->ucHead)
    {
        usPhyBufID   = pRFDBuffer->aRFDPhyBufID[ucRecycle];
        usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

        /* check buffer CacheStatus */
        if(BUF_IDLE == L1_ReadPreFetchCheckAllBufBusy(usPhyBufID))
        {
            /* reset the buffer */
            gpBufTag[usLogicBufID].bSeqBuf = 0;

            /* move RecycleHead */
            ucRecycle++;
            if(ucRecycle >= L1_READ_FROM_DRAM_BUF_PER_PU)
            {
                ucRecycle = 0;
            }

            pRFDBuffer->ucRecycle = ucRecycle;
            pRFDBuffer->ucUsedCnt--;

            if (0 != (g_ulRFDFullBitmap & (1 << ucPuNum)))
            {
                /* update RFD buffer PU full bitmap */
                g_ulRFDFullBitmap &= (~((U32)(1 << ucPuNum)));
            }
        }
        else
        {
            return;
        }
    }

    return;
}
#endif

#if 0 /*YJ remove*/
BOOL L1_BufferCheckNFCBusy(U16 usLogBufID)
{
    return L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartNWID);
}

BOOL L1_BufferCheckHostBusy(U16 usLogBufID)
{
    return L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartHRID);
}

U8 L1_BufferCheckPrefetchBusy(U16 usLogBufId)
{
    // 20160412 Sean Gao
    // this function is specifically for checking the status of a prefetch buffer

    U8 ucBufferStatus;

    // ensure the current buffer is a prefetch buffer before processing
    if(gpBufTag[usLogBufId].bPrefetchBuf == FALSE)
    {
        DBG_Printf("prefetch buffer flag error\n");
        DBG_Getch();
    }
    
    // get the status of the prefetch buffer
    ucBufferStatus = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartNRID, 0, INVALID_4F);

    // return the prefetch buffer status
    return ucBufferStatus;
}

BOOL L1_BufferCheckMergeBusy(U16 usLogBufID)
{
    return L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartNRID);
}
#endif

BOOL L1_BufferCheckFlushBusy(U16 usLogBufID)
{
    BOOL bRet;

    bRet = L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartNRID);

    if (BUF_IDLE == bRet)
    {
        bRet = L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartHWID);
    }

    return bRet;
}

BOOL L1_HostFullHitWriteCheckBusy(U16 usLogBufId, U16 usSecOffInBuf, U16 usSecLen)
{
    U8 ucNfcReadIdle;
    U8 ucHostWriteIdle;
    U8 ucHostReadIdle;

    ucNfcReadIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartNRID, usSecOffInBuf, usSecLen);
    ucHostWriteIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartHWID, usSecOffInBuf, usSecLen);
    ucHostReadIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartHRID, usSecOffInBuf, usSecLen);

    return ((ucNfcReadIdle == BUF_IDLE) && (ucHostWriteIdle == BUF_IDLE) && (ucHostReadIdle == BUF_IDLE)) ? BUF_IDLE : BUF_BUSY;
}

BOOL L1_HostFullHitReadCheckBusy(U16 usLogBufId, U16 usSecOffInBuf, U16 usSecLen)
{
    U8 ucNfcReadIdle;
    U8 ucHostWriteIdle;

    ucNfcReadIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartNRID, usSecOffInBuf, usSecLen);
    ucHostWriteIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartHWID, usSecOffInBuf, usSecLen);

    return ((ucNfcReadIdle == BUF_IDLE) && (ucHostWriteIdle == BUF_IDLE)) ? BUF_IDLE : BUF_BUSY;
}

// Sean Gao 20150513
// this function checks if there are any pending merge read
// or host write operations on the input LPN
BOOL L1_HostPartialHitCheckBusy(U16 usLogBufId, U16 usCacheOffset)
{
    U32 NfcReadIdle;
    U32 HostWriteIdle;

    NfcReadIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartNRID, usCacheOffset << SEC_PER_LPN_BITS, SEC_PER_LPN);
    HostWriteIdle = L1_CheckCacheStatus(&gpBufTag[usLogBufId].usStartHWID, usCacheOffset << SEC_PER_LPN_BITS, SEC_PER_LPN);

    return ((NfcReadIdle == BUF_IDLE) && (HostWriteIdle == BUF_IDLE)) ? BUF_IDLE : BUF_BUSY;
}

/****************************************************************************
Function  : L1_AllocateWriteBuf
Input      :   
Output    : Output an effective Write Buffer
Return    : INVALID_4F,  no buffer;   or else, return usPhyBufferID

Purpose   : 
allocate dram for buffer

Reference :
Modification History:
20121120   Blakezhang   001 first create function
****************************************************************************/
U16 g_usLastPhyBufId;
U16 L1_AllocateWriteBuf(U8 ucPuNum)
{
    // Sean Gao 20150318
    // this function allocates a new write cache, if we haven't
    // reached the maximum

    U16 usPhyBufId;

    // check the number of active write caches, we can't
    // allocate a new one if we've already at the maximum, even
    // if there are still free buffers in the free cache link

    // check buffers in the flush ongoing buffer link, see if their NFC operations
    // are done and ready to be converted to read caches
    if(g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt >= L1_WRITE_CACHE_BUFFER_PER_PU && g_WriteBufInfo[ucPuNum].ucFlushOngoingBufCnt != 0)
    {
        L1_RecycleWriteBuf(ucPuNum);
    }

    if(g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt >= L1_WRITE_CACHE_BUFFER_PER_PU)
    {
        // we've already reached the maximum
        return INVALID_4F;
    }
    else
    {
        // get a new buffer from the free cache buffer link
        usPhyBufId = L1_RemoveBufFromLinkHead(ucPuNum, FREE_CACHE_LINK);

        // check if we've successfully allocate a new buffer,
        // please note that even if the active write cache count
        // hasn't reached the maximum, it's still possible that
        // the free buffer link is empty, this is due to read
        // cache link buffer count exceeds its
        // maximum(temporarily)
        if(usPhyBufId != INVALID_4F)
        {
            // a new buffer is allocated, update the active
            // write cache count
            g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt++;

            // update the free cache available bitmap
            if(g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt == L1_WRITE_CACHE_BUFFER_PER_PU)
            {
                COM_BitMaskClear(&g_ulFreeCacheAvailableBitmap, ucPuNum);
            }

        }

        // return the physical buffer id
        return usPhyBufId;
    }
}

// Sean Gao 20160126
// this function returns the last logical buffer of the current input PU
U16 L1_GetLastLogBufId(U8 ucSuperPu)
{
    U16 usFirstLogBufId = g_ulWriteBufStartPhyId - g_ulBufferStartPhyId;

    return (usFirstLogBufId + (L1_WRITE_BUFFER_PER_PU * (ucSuperPu + 1)) - 1);
}

/****************************************************************************
Function  : L1_ReleaseWriteBuf
Input     :   usPhyBufID
Output    : 
Return    : 
Purpose   : 
   add a buffer into PUTail

Reference :
Modification History:
20121120   Blakezhang   001 first create function
20150319  Sean Gao this function releases an active write cache and add
             it to the merge pending buffer link
****************************************************************************/
BOOL L1_ReleaseWriteBuf(U8 ucPuNum, U16 usPhyBufID)
{

#ifdef SIM
    // check the stage of the active write cache, it must be
    // BUF_STAGE_CACHE
    if(BUF_STAGE_CACHE != gpBufTag[LGCBUFID_FROM_PHYBUFID(usPhyBufID)].Stage)
    {
        DBG_Printf("active write cache stage error\n");
        DBG_Getch();
    }
#endif

    // add the released write cache to the tail of the merge
    // pending buffer link
    L1_AddBufToLinkTail((U8)ucPuNum, MERGE_PENDING_LINK, LGCBUFID_FROM_PHYBUFID(usPhyBufID));

    COM_BitMaskSet(&g_ulMergeBufferBitmap, ucPuNum);

    return TRUE;
}

/****************************************************************************
Function  : L1_RecycleWriteBuf
Input     : 
Output    : 
Return    : 
Purpose   :  recycle a write buffers
Reference :
Modification History:
20121120   Blakezhang   001 first create function
****************************************************************************/
void L1_RecycleWriteBuf(U8 ucPuNum)
{
    // Sean Gao 20150306
    // this function is revised for processing prefetch buffers
    // major purposes of this function are checking flush ongoing
    // buffers to see if they're ready to be converted to read
    // caches and maintain the number of read caches

    U8  ucLoop;
    U16 usLogBufID;
    U8 ucNfcWriteStatus;
    U8 ucNfcReadStatus;

    // examine all flush ongoing buffers, see if they are done
    // being flushed and ready to be converted to read caches
    while (1)
    {
        // get the flush ongoing buffer to be processed

        usLogBufID = g_WriteBufInfo[ucPuNum].usFlushOngoingStartBufId;
        if (usLogBufID == INVALID_4F)
            break;

#ifdef SIM
        // there are 2 kinds of buffers we process in this
        // function:
        // 1. normal write caches, which are added to the flush
        // ongoing FIFO following the normal buffer processing
        // procedure
        // 2. prefetch buffers, which are added directly to the
        // flush ongoing FIFO
        if(gpBufTag[usLogBufID].Stage == BUF_STAGE_CACHE && gpBufTag[usLogBufID].bPrefetchBuf == TRUE)
        {
            // prefetch buffers, both host write cache status
            // link and NFC write cache status link must be
            // empty
            if(gpBufTag[usLogBufID].usStartHWID != INVALID_4F || gpBufTag[usLogBufID].usStartNWID != INVALID_4F)
            {
                DBG_Printf("prefetch buffer cache status link error\n");
                DBG_Getch();
            }
        }
        else if(gpBufTag[usLogBufID].Stage == BUF_STAGE_FLUSH && gpBufTag[usLogBufID].bPrefetchBuf == FALSE)
        {
            // normal write caches, NFC read cache status link
            // and host write cache status link must be empty
            if(gpBufTag[usLogBufID].usStartHWID != INVALID_4F || gpBufTag[usLogBufID].usStartNRID != INVALID_4F)
            {
                DBG_Printf("write cache cache status link error\n");
                DBG_Getch();
            }
        }
        else
        {
            DBG_Printf("buffer stage error\n");
            DBG_Getch();
        }
#endif

        // check the cache status link of the buffer, note that
        // we also have to check the NFC read cache status link
        // if the buffer is a prefetch buffer

        if(gpBufTag[usLogBufID].bPrefetchBuf == TRUE)
        {
            // prefetch buffer, check NFC read status
            ucNfcWriteStatus = BUF_IDLE;
            ucNfcReadStatus = L1_CheckPrefetchBufferCacheStatus(&gpBufTag[usLogBufID].usStartNRID);
        }
        else
        {
            // not a prefetch buffer, check NFC write status
            ucNfcWriteStatus = L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartNWID);
            ucNfcReadStatus = BUF_IDLE;
        }
        
        if((ucNfcWriteStatus == BUF_IDLE) && (ucNfcReadStatus == BUF_IDLE || ucNfcReadStatus == BUF_FAIL))
        {
            // the cache status of the buffer has been cleared,
            // convert it to a read cache
            gpBufTag[usLogBufID].Stage = BUF_STAGE_RECYCLE;

            // remove the prefetch buffer if a read UECC occurred
            if(ucNfcReadStatus == BUF_FAIL)
            {
#ifdef SIM
                // perform a check on the prefetch flag
                if(gpBufTag[usLogBufID].bPrefetchBuf == FALSE)
                {
                    DBG_Printf("prefetch flag error\n");
                    DBG_Getch();
                }
#endif

                // remove all cache ids of the buffer entries from the
                // lct link
                L1_CacheLinkRecycleBuf(usLogBufID);

                // reset all buffer information
                gpBufTag[usLogBufID].ucWritePointer = 0;
                gpBufTag[usLogBufID].bSeqBuf = 0;
                gpBufTag[usLogBufID].bIsCertainHit = FALSE;

                // reset all buffer lpn and sector bitmaps
                for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
                {
                    L1_SetBufferLPN(usLogBufID, ucLoop, INVALID_8F);
                    L1_SetBufferSectorBitmap(usLogBufID, ucLoop, 0);
                }
            }

            // clear the bPrefetchBuf flag
            gpBufTag[usLogBufID].bPrefetchBuf = FALSE;

#ifdef SIM
            // double check cache status links
            if((INVALID_4F != gpBufTag[usLogBufID].usStartNRID) || (INVALID_4F != gpBufTag[usLogBufID].usStartHWID) || (INVALID_4F != gpBufTag[usLogBufID].usStartNWID))
            {
                DBG_Printf("read cache cache status error\n");
                DBG_Getch();
            }
#endif

            // remove the buffer from the flush ongoing link
            L1_RemoveBufFromLinkHead(ucPuNum, FLUSH_ONGOING_LINK);


            // add the buffer to the read cache link
            L1_AddBufToLinkTail(ucPuNum, READ_CACHE_LINK, usLogBufID);

            // update flush ongoing buffer count, read cache
            // count and active write cache count
            g_WriteBufInfo[ucPuNum].ucActiveWriteCacheCnt--;

            // update the free cache available bitmap
            COM_BitMaskSet(&g_ulFreeCacheAvailableBitmap, ucPuNum);

        }
        else
        {
            // the cache status hasn't yet been cleared
            break;
        }
    } // while(1)

    // now that we've processed buffers in the flush ongoing
    // FIFO, we have to maintain read caches to ensure that
    // the maximum number of read caches is not exceeded
    while(L1_LAST_RECYCLE_BUFFER_PER_PU < g_WriteBufInfo[ucPuNum].ucReadCacheBufCnt)
    {
        // get the buffer in the head of read cache FIFO
        usLogBufID = g_WriteBufInfo[ucPuNum].usReadCacheStartBufId;

#ifdef SIM
        // check the stage of the buffer, note that the stage
        // of read caches must be BUF_STAGE_RECYCLE, no matter
        // if it's a prefetch buffer
        if(BUF_STAGE_RECYCLE != gpBufTag[usLogBufID].Stage)
        {
            DBG_Printf("L1_RecycleWriteBuf read cache 0x%x stage error\n", usLogBufID);
            DBG_Getch();
        }
#endif

        // check the host read cache status link, ensure it's
        // empty
        if (L1_CheckCacheStatusLinkIdle(&gpBufTag[usLogBufID].usStartHRID) != BUF_IDLE)
        {
            return;
        }

        // check if
        if(gpBufTag[usLogBufID].bIsCertainHit == TRUE)
        {
            return;
        }

#ifdef SIM
        // double check cache status links, at this point all of
        // them should be empty
        if((INVALID_4F != gpBufTag[usLogBufID].usStartNRID) || (INVALID_4F != gpBufTag[usLogBufID].usStartHWID) || (INVALID_4F != gpBufTag[usLogBufID].usStartNWID) || (INVALID_4F != gpBufTag[usLogBufID].usStartHRID))
        {
            DBG_Printf("L1_RecycleWriteBuf read cache 0x%x cache status link error\n", usLogBufID);
            DBG_Getch();
        }
#endif

        // remove all cache ids of the buffer entries from the
        // lct link
        L1_CacheLinkRecycleBuf(usLogBufID);
    
#if 0 
        gpBufTag[usLogBufID].Stage = BUF_STAGE_FREE;
        gpBufTag[usLogBufID].ucWritePointer = 0;
        gpBufTag[usLogBufID].bSeqBuf = 0;
        gpBufTag[usLogBufID].bPrefetchBuf = FALSE;
        gpBufTag[usLogBufID].bIsCertainHit = FALSE;
#else
        gpBufTag[usLogBufID].usDW0L = 0;
#endif

        // reset all buffer lpn and sector bitmaps
        for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
        {
            L1_SetBufferLPN(usLogBufID, ucLoop, INVALID_8F);
            L1_SetBufferSectorBitmap(usLogBufID, ucLoop, 0);
        }
    
        // remove the buffer from the read cache link
        L1_RemoveBufFromLinkHead(ucPuNum, READ_CACHE_LINK);
    
        // add the freed buffer to the tail of the free cache
        // link
        L1_AddBufToLinkTail(ucPuNum, FREE_CACHE_LINK, usLogBufID);
    }

    return;
}

/****************************************************************************
Name        :L1_BufferMergeLpn
Input       :U16 usPhyBufID, U8 ucLpnOffset
Output      :
Author      :Blakezhang
Date        :2012.11.22
Description :merge one lpn for a buffer.
Others      :
Modify      :2016.11.24  KristinWang   add Input ucSuperPu
****************************************************************************/
BOOL L1_BufferMergeLpn(U8 ucSuperPu, U16 usPhyBufID, U8 ucLpnOffset)
{
    U8  ucBufferBitmap;
    U16 usLogicBufID;
    U8 ucBufReqID;
    BUF_REQ_READ* pBufReq;

#ifdef SIM
    /* validation check */
    if(INVALID_4F == usPhyBufID)
    {
        DBG_Printf("L1_BufferMergeLpn usPhyBufID ERROR! \n");
        DBG_Getch();
    }
#endif

    usLogicBufID   = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
    ucBufferBitmap = L1_GetBufferSectorBitmap(usLogicBufID, ucLpnOffset);

#ifdef SIM
    if(0 == (~(ucBufferBitmap)))
    {
        DBG_Printf("L1_BufferMergeLpn MergeBitmap ERROR! \n");
        DBG_Getch();
    }
#endif

    /*merge LPN from flash ,send BufReq*/
    ucBufReqID = L1_AllocateReadBufReq(ucSuperPu);

#ifdef SIM
    if (INVALID_2F == ucBufReqID)
    {
        DBG_Printf("L1_BufferMergeLpn allocate Read BufReq ERROR! \n");
        DBG_Getch();
    }
#endif

    pBufReq = L1_GetReadBufReq(ucSuperPu, ucBufReqID);

    pBufReq->usPhyBufferID = usPhyBufID;

    pBufReq->ucLPNOffset = ucLpnOffset;
    pBufReq->ucLPNCount = 1;

    /* indicates sector that need merge */
    pBufReq->ulLPNBitMap = (U32)(~(ucBufferBitmap));

    /* this is LPN's address, in unit of SEC/LPN */
    pBufReq->aLPN[ucLpnOffset] = L1_GetBufferLPN(usLogicBufID, ucLpnOffset);

    pBufReq->bReqLocalREQFlag = TRUE;

    pBufReq->ulReqStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[usLogicBufID].usStartNRID), 
                              ((U16)ucLpnOffset << SEC_PER_LPN_BITS), SEC_PER_LPN);


    gpBufTag[usLogicBufID].Stage = BUF_STAGE_CACHE;

    /* update SectroBitMap */
    L1_SetBufferSectorBitmap(usLogicBufID, ucLpnOffset, INVALID_2F);

    /* send the BufReq to L2 */
    L1_InsertBufReqToHighPrioLinkTail(ucSuperPu, ucBufReqID);

    return TRUE;
}

BOOL L1_BufferReadLpn(U8 ucSuperPu, U16 usPhyBufID, U8 ucLpnOffset, U32 ulLpn, U16* pusCachestatus)
{
    // Sean Gao 201504027
    // this function reads a lpn to a specified location
    U8 ucBufReqID;
    BUF_REQ_READ* pBufReq;

    // get a buffer request entry
    ucBufReqID = L1_AllocateReadBufReq(ucSuperPu);
    pBufReq = L1_GetReadBufReq(ucSuperPu, ucBufReqID);

    // fill in the buffer request
    pBufReq->usPhyBufferID = usPhyBufID;
    pBufReq->ucLPNOffset = ucLpnOffset;
    pBufReq->ucLPNCount = 1;
    pBufReq->ulLPNBitMap = (U32)INVALID_2F;
    pBufReq->aLPN[ucLpnOffset] = ulLpn;
    pBufReq->bReqLocalREQFlag = TRUE;
    pBufReq->ulReqStatusAddr = L1_AddCacheStatusToLink(pusCachestatus, ((U16)ucLpnOffset << SEC_PER_LPN_BITS), SEC_PER_LPN);


    // trigger the buffer request
    L1_InsertBufReqToHighPrioLinkTail(ucSuperPu, ucBufReqID);

    return TRUE;
}

/****************************************************************************
Function  : L1_MergeWriteBuf
Input     : PUNum
Output    : 
Return    :   TRUE if a request sends to HighPrioFifo
Purpose   :  send a merge request to HighPrioFifo from a PU
Reference :
Modification History:
20121122   Blakezhang   001 first create function
****************************************************************************/
BOOL L1_MergeWriteBuf(U8 ucPuNum)
{
    // Sean Gao 20150320
    // this function will try issuing a LPN merge request
    // in each entry, until the buffer is fully merged

    U8  ucSent;
    U8  ucLoop;
    U8  ucCurrBitmap;
    U16 usLogicBufID;
    U16 usPhyBufID;

    // ucSent indicates if a merge request is sent in this
    // entry, initiate it with FALSE
    ucSent = FALSE;

    // check if there are buffers in the merge pending link
    while (1)
    {
        // get the buffer in the head of the merge pending link
        usPhyBufID = L1_GetBufFromLinkHead(ucPuNum, MERGE_PENDING_LINK);
        if (usPhyBufID == INVALID_4F)
            break;

        // calculate its logical buffer id
        usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
       
        // process the buffer based on the type of the buffer
        if(gpBufTag[usLogicBufID].bSeqBuf == TRUE)
        {
            // for sequential buffer, no merge operations are
            // needed

            // remove the buffer from the merge pending link
            // directly, since it doesn't need any merge
            // operations
            L1_RemoveBufFromLinkHead(ucPuNum, MERGE_PENDING_LINK);

            if (0 == g_WriteBufInfo[ucPuNum].ucMergePendingBufCnt)
            {
                COM_BitMaskClear(&g_ulMergeBufferBitmap, ucPuNum);
            }

            // add the buffer to the flush pending link
            L1_AddBufToLinkTail(ucPuNum, FLUSH_PENDING_LINK, usLogicBufID);

            COM_BitMaskSet(&g_ulFlushBufferBitmap, ucPuNum);
        } // if(gpBufTag[usLogicBufID].bSeqBuf == TRUE)
        else
        {
            // for random write buffer, we have to check each
            // LPN in the buffer one-by-one
            for(ucLoop = g_ucCurMergePos[ucPuNum]; ucLoop < gpBufTag[usLogicBufID].ucWritePointer; ucLoop++)
            {
                // check if the current LPN is valid
                if(INVALID_8F == L1_GetBufferLPN(usLogicBufID, ucLoop))
                {
                    // current LPN is invalid, set its bitmap
                    // to 0x2F and continue checking the next
                    // LPN in the buffer
                    L1_SetBufferSectorBitmap(usLogicBufID, ucLoop, INVALID_2F);
                    continue;
                }
                else
                {
                    // the current LPN is valid,
                    // get the buffer sector bitmap of the
                    // current lpn
                    ucCurrBitmap = L1_GetBufferSectorBitmap(usLogicBufID, ucLoop);
                }

                // check its bitmap, if it's not 0xFF, break the
                // loop since we've already found a merge target
                if(INVALID_2F != ucCurrBitmap)
                {
                    break;
                }
            } // for(ucLoop = 0; ucLoop < gpBufTag[usLogicBufID].ucWritePointer; ucLoop++)

            // we've found a merge target LPN, send the merge
            // request
            if(ucLoop < gpBufTag[usLogicBufID].ucWritePointer)
            {
                L1_BufferMergeLpn(ucPuNum, usPhyBufID, ucLoop);
                ucSent = TRUE;
                ucLoop++;

                // update g_ucCurMergePos for the current PU
                g_ucCurMergePos[ucPuNum] = ucLoop;
            }

            // check if we've completed all the merge operations
            // needed for this buffer
            if(ucLoop >= gpBufTag[usLogicBufID].ucWritePointer)
            {
                // all merge operations issued

                // remove the buffer from the merge pending link
                // since all merge operations have been issued
                L1_RemoveBufFromLinkHead(ucPuNum, MERGE_PENDING_LINK);

                if (0 == g_WriteBufInfo[ucPuNum].ucMergePendingBufCnt)
                {
                    COM_BitMaskClear(&g_ulMergeBufferBitmap, ucPuNum);
                }

                // add the buffer to the flush pending link
                L1_AddBufToLinkTail(ucPuNum, FLUSH_PENDING_LINK, usLogicBufID);

                COM_BitMaskSet(&g_ulFlushBufferBitmap, ucPuNum);

                // reset g_ucCurMergePos for the current PU
                g_ucCurMergePos[ucPuNum] = 0;
            } // if(ucLoop >= gpBufTag[usLogicBufID].ucWritePointer)

            // break the loop if a merge request has been sent
            if(ucSent == TRUE)
            {
                break;
            }
        }
    } // while(1)

    return ucSent;
}

/****************************************************************************
Name        :L1_BufferMergeLpn
Input       :U16 usPhyBufID
Output      :
Author      :Blakezhang
Date        :2012.11.22
Description :flush a buffer
Others      :
Modify      :
****************************************************************************/
LOCAL BOOL L1_BufferFlush(U8 ucPu, U16 usPhyBufID)
{
    // Sean Gao 20150526
    // this function issues a write buffer request for
    // a buffer, it returns TRUE if a write buffer request is
    // sent, FALSE otherwise

    U8  ucLoop;
    U8  ucBufferValid;
    U16 usLogicBufID;
    U32 ulLPN;
#ifdef DATA_MONITOR_ENABLE
    U32 ulBufAddr;
#endif
    BUF_REQ_WRITE* pBufReq;

    pBufReq = L1_LowPrioFifoGetBufReq(ucPu);

    for (ucLoop = 0; ucLoop < BUF_REQ_WRITE_SIZE; ucLoop++)
    {
        *((U32*)pBufReq + ucLoop) = 0;
    }

    // fill in the buffer request
    pBufReq->ReqStatusAddr = 0;
    pBufReq->PhyBufferID = usPhyBufID;
    pBufReq->LPNOffset = 0;
    pBufReq->LPNCount  = LPN_PER_BUF;

#ifdef DATA_MONITOR_ENABLE
    ulBufAddr =  COM_GetMemAddrByBufferID(usPhyBufID, TRUE, BUF_SIZE_BITS);
    FW_DataMonitorUpdateWriteData(&pBufReq->ucWriteData[0], ulBufAddr);
#endif

    // fill in the LPNs of the buffer
    ucBufferValid = FALSE;
    usLogicBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

    for(ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
        ulLPN = L1_GetBufferLPN(usLogicBufID, ucLoop);

        pBufReq->LPN[ucLoop] = ulLPN;
      
        if(INVALID_8F != ulLPN)
        {
            ucBufferValid = TRUE;

#ifdef DATA_MONITOR_ENABLE
            FW_DataMonitorUpdateLPN(ulLPN, ucLoop, ulBufAddr);
#endif
        }

#ifdef L1_BUFFER_REGULATION
#ifdef SIM
        // verify the valid LPN bitmap
        if((ulLPN != INVALID_8F && COM_BitMaskGet(gpBufTag[usLogicBufID].ulValidLpnBitmap, ucLoop) == FALSE) || (ulLPN == INVALID_8F && COM_BitMaskGet(gpBufTag[usLogicBufID].ulValidLpnBitmap, ucLoop) == TRUE))
        {
            DBG_Printf("valid LPN bitmap error\n");
            DBG_Getch();
        }
#endif
#endif
    }

    gpBufTag[usLogicBufID].Stage = BUF_STAGE_FLUSH;

    if(FALSE == ucBufferValid)
    {
        // all LPNs in this buffer are invalid, return and do not send the buffer request
        gpBufTag[usLogicBufID].ucWritePointer = 0;
    }
    else
    {
        pBufReq->ReqStatusAddr = L1_AddCacheStatusToLink(&(gpBufTag[usLogicBufID].usStartNWID), 0, INVALID_4F);
    
        // trigger the write buffer request
        L1_LowPrioMoveTailPtr(ucPu);
    }

    return ucBufferValid;
}

/****************************************************************************
Function  : L1_FlushWriteBuf
Input     : PUNum
Output    : 
Return    :   TRUE if a request sends to LowPrioFifo
Purpose   :  send a flush write to LowPrioFifo from a PU
Reference :
Modification History:
20121122   Blakezhang   001 first create function
20150525   Sean Gao modify
****************************************************************************/
BOOL L1_FlushWriteBuf(U8 ucPuNum)
{
    U16 usPhyBufID, usLogBufID;
    U8 ucBufReqSent;

    ucBufReqSent = FALSE;

    usPhyBufID = L1_GetBufFromLinkHead(ucPuNum, FLUSH_PENDING_LINK);
    if (usPhyBufID != INVALID_4F)
    {
        usLogBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);
       
        if (BUF_IDLE == L1_BufferCheckFlushBusy(usLogBufID))
        {          
            ucBufReqSent = L1_BufferFlush(ucPuNum, usPhyBufID);

            L1_RemoveBufFromLinkHead(ucPuNum, FLUSH_PENDING_LINK);

            if (0 == g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt)
            {
                COM_BitMaskClear(&g_ulFlushBufferBitmap, ucPuNum);
            }

            L1_AddBufToLinkTail(ucPuNum, FLUSH_ONGOING_LINK, usLogBufID);

        }
    }

    return ucBufReqSent;
}

/****************************************************************************
Name        :L1_BufferGetWritePointer
Input        :usLogicBufID
Output      :ucWritePointer
Author      :BlakeZhang
Date        :2012.11.21
Description :get the buffer's ucWritePointer
Others      :
Modify      :
****************************************************************************/
U8 L1_BufferGetWritePointer(U16 usLogicBufID)
{
    return (U8)(gpBufTag[usLogicBufID].ucWritePointer);
}

/****************************************************************************
Name        :L1_BufferSetWritePointer
Input        :usLogicBufID ucWritePointer
Output      :
Author      :BlakeZhang
Date        :2012.11.22
Description :set the buffer's ucWritePointer
Others      :
Modify      :
****************************************************************************/
void L1_BufferSetWritePointer(U16 usLogicBufID, U8 ucWritePointer)
{
    gpBufTag[usLogicBufID].ucWritePointer = ucWritePointer;
    return;
}

void L1_AddBufToLinkTail(U8 ucPuNum, BUF_LINK_TYPE LinkType, U16 usTargetLogBufId)
{
    // Sean Gao 20150316
    // this function add a buffer to the tail of 1 of the 5 
    // buffer links of the specified PU

    U16* pusEndBufId;
    U16* pusStartBufId;
    U8* pucLinkBufCnt;
#ifdef SIM
    U8 ucBufCnt;
    U16 usCurBufId;
#endif

    // set the pointers to the start and the end buffer id 
    // based on the buffer link type
    switch(LinkType)
    {
        case FREE_CACHE_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucFreeCacheBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usFreeCacheEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFreeCacheStartBufId);
            break;
        case MERGE_PENDING_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucMergePendingBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usMergePendingEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usMergePendingStartBufId);
            break;
        case FLUSH_PENDING_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usFlushPendingEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId);
            break;
        case FLUSH_ONGOING_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucFlushOngoingBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usFlushOngoingEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFlushOngoingStartBufId);
            break;
        case READ_CACHE_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucReadCacheBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usReadCacheEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usReadCacheStartBufId);
            break;
        default:
            DBG_Printf("buffer link type error\n");
            DBG_Getch();
            break;
    }

#ifdef SIM
    // the next buffer id of the target buffer must be
    // INVALID_4F, meaning that it cannot be in any buffer links
    if(gpBufTag[usTargetLogBufId].usNextBufId != INVALID_4F)
    {
        DBG_Printf("next buffer id error\n");
        DBG_Getch();
    }

    // the next buffer id of the end of the link must be
    // INVALID_4F
    if(*pusStartBufId != INVALID_4F && gpBufTag[*pusEndBufId].usNextBufId != INVALID_4F)
    {
        DBG_Printf("next buffer id error\n");
        DBG_Getch();
    }

    // check the integrity of the link
    if(*pusStartBufId == INVALID_4F && *pusEndBufId != INVALID_4F)
    {
        DBG_Printf("buffer link error\n");
        DBG_Getch();
    }

    // check the buffer link count
    ucBufCnt = 0;
    usCurBufId = *pusStartBufId;
    while(usCurBufId != INVALID_4F)
    {
        ucBufCnt++;
        usCurBufId = gpBufTag[usCurBufId].usNextBufId;
    }
    if(ucBufCnt != *pucLinkBufCnt)
    {
        DBG_Printf("buffer count error\n");
        DBG_Getch();
    }
#endif

    if(*pusStartBufId == INVALID_4F)
    {
        // the link was empty, we have to modify both the start
        // and the end buffer id

        // set the start buffer id
        *pusStartBufId = usTargetLogBufId;

        // set the end buffer id
        *pusEndBufId = usTargetLogBufId;
    }
    else
    {
        // the link wasn't empty, all we have to do is to modify
        // the end buffer id
        
        // add the new buffer the end of the list
        gpBufTag[*pusEndBufId].usNextBufId = usTargetLogBufId;

        // set the end buffer
        *pusEndBufId = usTargetLogBufId;
    }

    // update the link buffer count
    *pucLinkBufCnt += 1;

    return;
}

U16 L1_RemoveBufFromLinkHead(U8 ucPuNum, BUF_LINK_TYPE LinkType)
{
    // Sean Gao 20150317
    // this function remove a buffer to the head of 1 of the 5 
    // buffer links of the specified PU

    U16* pusEndBufId;
    U16* pusStartBufId;
    U8* pucLinkBufCnt;
    U16 usRemovePhyBufId;
#ifdef SIM
    U8 ucBufCnt;
    U16 usCurBufId;
#endif

    // set the pointers to the start and the end buffer id 
    // based on the buffer link type
    switch(LinkType)
    {
        case FREE_CACHE_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucFreeCacheBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usFreeCacheEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFreeCacheStartBufId);
            break;
        case MERGE_PENDING_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucMergePendingBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usMergePendingEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usMergePendingStartBufId);
            break;
        case FLUSH_PENDING_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usFlushPendingEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId);
            break;
        case FLUSH_ONGOING_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucFlushOngoingBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usFlushOngoingEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFlushOngoingStartBufId);
            break;
        case READ_CACHE_LINK:
            pucLinkBufCnt = &(g_WriteBufInfo[ucPuNum].ucReadCacheBufCnt);
            pusEndBufId   = &(g_WriteBufInfo[ucPuNum].usReadCacheEndBufId);
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usReadCacheStartBufId);
            break;
        default:
            DBG_Printf("buffer link type error\n");
            DBG_Getch();
            break;
    }

#ifdef SIM
    // the next buffer id of the end of the link must be
    // INVALID_4F
    if(*pusEndBufId != INVALID_4F && gpBufTag[*pusEndBufId].usNextBufId != INVALID_4F)
    {
        DBG_Printf("next buffer id error\n");
        DBG_Getch();
    }

    // check the integrity of the link
    if(*pusStartBufId == INVALID_4F && *pusEndBufId != INVALID_4F)
    {
        DBG_Printf("buffer link error\n");
        DBG_Getch();
    }

    // check the buffer link count
    usCurBufId = *pusStartBufId;
    ucBufCnt = 0;
    while(usCurBufId != INVALID_4F)
    {
        ucBufCnt++;
        usCurBufId = gpBufTag[usCurBufId].usNextBufId;
    }
    if(ucBufCnt != *pucLinkBufCnt)
    {
        DBG_Printf("buffer count error\n");
        DBG_Getch();
    }
#endif

    if(*pusStartBufId == INVALID_4F)
    {
        // the buffer link is empty, return INVALID_4F to
        // indicate there's no buffer to be removed
        usRemovePhyBufId = INVALID_4F;
    }
    else
    {
        // the buffer link is not empty, return the buffer in
        // link head

        U16 usNewHeadBufId;

        // set the physical buffer id of the buffer in the
        // head
        usRemovePhyBufId = PHYBUFID_FROM_LGCBUFID(*pusStartBufId);

        // save the next buffer id of the head buffer, which will
        // be the new start buffer id of the buffer link
        usNewHeadBufId = gpBufTag[*pusStartBufId].usNextBufId;

        // remove the head buffer from the buffer link by
        // setting the next buffer id to INVALID_4F
        gpBufTag[*pusStartBufId].usNextBufId = INVALID_4F;

        // check if the buffer link has only one buffer, if it
        // does, we have to modify the end buffer id too
        if(*pusStartBufId == *pusEndBufId)
        {
            *pusEndBufId = INVALID_4F;
        }

        // update the new start buffer id
        *pusStartBufId = usNewHeadBufId;

        // update the link buffer count
        *pucLinkBufCnt -= 1;
    }

    return usRemovePhyBufId;
}

U16 L1_GetBufFromLinkHead(U8 ucPuNum, BUF_LINK_TYPE LinkType)
{
    // Sean Gao 20150317
    // this function returns the buffer of the head of 1 of the
    // 5 buffer links of the specified PU without removing it

    U16* pusStartBufId;
    U16 usGetPhyBufId;

    // set the pointers to the start and the end buffer id 
    // based on the buffer link type
    switch(LinkType)
    {
        case FREE_CACHE_LINK:
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFreeCacheStartBufId);
            break;
        case MERGE_PENDING_LINK:
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usMergePendingStartBufId);
            break;
        case FLUSH_PENDING_LINK:
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId);
            break;
        case FLUSH_ONGOING_LINK:
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usFlushOngoingStartBufId);
            break;
        case READ_CACHE_LINK:
            pusStartBufId = &(g_WriteBufInfo[ucPuNum].usReadCacheStartBufId);
            break;
        default:
            DBG_Printf("buffer link type error\n");
            DBG_Getch();
            break;
    }

    // return the physical buffer id of the first buffer in the
    // link
    usGetPhyBufId = (*pusStartBufId == INVALID_4F) ? INVALID_4F : PHYBUFID_FROM_LGCBUFID(*pusStartBufId);

    return usGetPhyBufId;
}

#ifdef L1_BUFFER_REGULATION
U16 L1_FindNextNonfullLogBufId(U16 usStartLogBufId)
{
    // Sean Gao 20150601
    // this function find the next non-full buffer, starting from the
    // next buffer of the input logical buffer

    U16 usCurLogBufId = gpBufTag[usStartLogBufId].usNextBufId;

    while(gpBufTag[usCurLogBufId].ulValidLpnBitmap == LPN_PER_BUF_BITMAP)
    {
        usCurLogBufId = gpBufTag[usCurLogBufId].usNextBufId;

#ifdef SIM
        if(usCurLogBufId == INVALID_4F)
        {
            DBG_Printf("next logical buffer id error\n");
            DBG_Getch();
        }
#endif
    }

    return usCurLogBufId;
}

U16 L1_FindNextNonemptyWithMinimumLpnCountLogBufId(U16 usStartLogBufId)
{
    // Sean Gao 20150601
    // this function find the next non-empty buffer with a minimum valid LPN count,
    // starting from the next buffer of the input logical buffer

    U16 usLogBufIdWithMinimumValidLpn;
    U16 usMinimumValidLpnCount;
    U16 usCurLogBufId;

    // initialize the current buffer id as the next buffer id of the head of flush
    // pending buffer link
    usCurLogBufId = gpBufTag[usStartLogBufId].usNextBufId;

    // initialize usLogBufIdWithMinimumValidLpn and usMinimumValidLpnCount
    usLogBufIdWithMinimumValidLpn = INVALID_4F;
    usMinimumValidLpnCount = INVALID_2F;

    while(usCurLogBufId != INVALID_4F)
    {
        // traverse through all buffers in the flush pending buffer link
        
        // get the valid LPN count of the current buffer
        U16 usCurValidLpnCount = HAL_POPCOUNT(gpBufTag[usCurLogBufId].ulValidLpnBitmap);

        // update usLogBufIdWithMinimumValidLpn and usMinimumValidLpnCount if the
        // valid LPN count is not 0
        if(usCurValidLpnCount != 0 && usCurValidLpnCount < usMinimumValidLpnCount) 
        {
            usMinimumValidLpnCount = usCurValidLpnCount;
            usLogBufIdWithMinimumValidLpn = usCurLogBufId;
        }

        // update the current logical buffer id
        usCurLogBufId = gpBufTag[usCurLogBufId].usNextBufId;
    }

    return usLogBufIdWithMinimumValidLpn;
}

BOOL L1_FlushPendingBufferRegulation(U8 ucPuNum)
{
    // Sean Gao 20150602
    // this function returns TRUE if the flush pending buffer regulation has
    // been complected, FALSE otherwise

    MCU12_VAR_ATTR U8 static ucBufferToBeFlushedIsSource[SUBSYSTEM_SUPERPU_MAX];
    MCU12_VAR_ATTR U16 static usSourceLogBufId[SUBSYSTEM_SUPERPU_MAX];
    MCU12_VAR_ATTR U16 static usSourcePhyBufId[SUBSYSTEM_SUPERPU_MAX];
    MCU12_VAR_ATTR U16 static usTargetLogBufId[SUBSYSTEM_SUPERPU_MAX];
    MCU12_VAR_ATTR U16 static usTargetPhyBufId[SUBSYSTEM_SUPERPU_MAX];

    // check if a buffer regulation operation is ongoing for the current PU
    if(g_ucIsBufferRegulationOngoing[ucPuNum] == FALSE)
    {
        // there's no ongoing buffer regulation operation, initialize one

        U16 usCurLogBufId;
        U16 usTotalValidLpnCount;
        U16 usLogBufIdWithMinimumValidLpn;
        U16 usMinimumValidLpnCount;

        // first calculte the total valid LPN count of all buffers in
        // the flush pending buffer link, see if we can reduce the number
        // of flush operations

        // initialize the current buffer id as the buffer id of the head of flush
        // pending buffer link
        usCurLogBufId = g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId;

        // initialize the total valid LPN count
        usTotalValidLpnCount = 0;

        // initialize usLogBufIdWithMinimumValidLpn and usMinimumValidLpnCount
        usLogBufIdWithMinimumValidLpn = INVALID_4F;
        usMinimumValidLpnCount = INVALID_4F;

        while(usCurLogBufId != INVALID_4F)
        {
            // traverse through all buffers in the flush pending buffer link
            // to calculate the total valid LPN count

            // get the valid LPN count of the current buffer
            U16 usCurValidLpnCount = HAL_POPCOUNT(gpBufTag[usCurLogBufId].ulValidLpnBitmap);

            // add the valid LPN count of the current buffer to the sum
            usTotalValidLpnCount += usCurValidLpnCount;

            // update usLogBufIdWithMinimumValidLpn and usMinimumValidLpnCount
            if(usCurValidLpnCount < usMinimumValidLpnCount) 
            {
                usMinimumValidLpnCount = usCurValidLpnCount;
                usLogBufIdWithMinimumValidLpn = usCurLogBufId;
            }

            // update the current logical buffer id
            usCurLogBufId = gpBufTag[usCurLogBufId].usNextBufId;
        }

#ifdef SIM
        // check the validity of valid lpn count
        if(usTotalValidLpnCount > g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt * LPN_PER_BUF)
        {
            DBG_Printf("total valid lpn count error\n");
            DBG_Getch();
        }
#endif

        // check the total valid LPN count to determine if the number of flush operations can
        // be reduced
        if(g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt == 1 || (g_WriteBufInfo[ucPuNum].ucFlushPendingBufCnt * LPN_PER_BUF - usTotalValidLpnCount) < LPN_PER_BUF)
        {
            // the number of flush operations can't be reduced, nothing could be done,
            // return TRUE immediately to indicate buffer regulation is complete
            return TRUE;
        }
        else
        {
            // the number of flush operations can be reduced, determine if the buffer
            // to be flushed is the one with minimum valid LPN count
            ucBufferToBeFlushedIsSource[ucPuNum] = (usLogBufIdWithMinimumValidLpn == g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId) ? TRUE : FALSE;
        }

        // if we reach here, it means that the number of flush operations can be reduced

        // there are 2 possible cases:
        // 1. the buffer to be flushed is the one with minimum valid LPN count
        // 2. the buffer to be flushed is NOT the one with minimum valid LPN count
        //
        // case 1, the buffer to be flushed is the one with minimum valid LPN count,
        // we keep moving LPNs in the buffer to be flushed to other buffers, until
        // all valid LPNs are moved out, please note that this operation will always
        // succeed since we have already ensured that the number of invalid LPNs is
        // greater or equal to LPN_PER_BUF
        //
        // case 2, the buffer to be flushed is NOT the one with minimum valid LPN
        // count, we move valid LPNs data from the buffer with minimum valid LPN
        // count to the buffer to be flushed until either the buffer to be flushed
        // is full or there are no more valid data can be moved

        // initialize the source and target buffer based on whether the buffer
        // to be flushed is the one with minimum valid lpn count
        if(ucBufferToBeFlushedIsSource[ucPuNum] == TRUE)
        {
            // case 1, the buffer to be flushed is the one with minimum valid lpn
            // count, in this case, it is the source buffer

            // initialize source buffer information
            usSourceLogBufId[ucPuNum] = usLogBufIdWithMinimumValidLpn;
            usSourcePhyBufId[ucPuNum] = PHYBUFID_FROM_LGCBUFID(usSourceLogBufId[ucPuNum]);

            // initialize target buffer information
            usTargetLogBufId[ucPuNum] = INVALID_4F;
            usTargetPhyBufId[ucPuNum] = INVALID_4F;
        }
        else if(ucBufferToBeFlushedIsSource[ucPuNum] == FALSE)
        {
            // case 2, the buffer to be flushed is NOT the one with minimum valid
            // lpn count, in this case, it is the target buffer

            // initialize source buffer information
            usSourceLogBufId[ucPuNum] = usLogBufIdWithMinimumValidLpn;
            usSourcePhyBufId[ucPuNum] = PHYBUFID_FROM_LGCBUFID(usSourceLogBufId[ucPuNum]);

            // initialize target buffer information
            usTargetLogBufId[ucPuNum] = g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId;
            usTargetPhyBufId[ucPuNum] = PHYBUFID_FROM_LGCBUFID(usTargetLogBufId[ucPuNum]);
        }
        else
        {
            DBG_Printf("ucBufferToBeFlushedIsSource[ucPuNum] error\n");
            DBG_Getch();
        }

        // set g_ucIsBufferRegulationOngoing, indicating there's an ongoing buffer regulation
        // operation
        g_ucIsBufferRegulationOngoing[ucPuNum] = TRUE;
    } // if(g_ucIsBufferRegulationOngoing[ucPuNum] == FALSE)

    // in this while loop we move valid LPN data from the source buffer to the target buffer
    // one by one, this loop will break if one of the participating buffers are busy or
    // the data copy process is complete
    while(TRUE)
    {
        U8 ucCopySourceLpnOffset;
        U32 ulCopySourceLpn;
        U8 ucCopyTargetLpnOffset;
        U32 ulTargetBufferFreeEntryBitmap;
        U32 ulSourceBufAddr;
        U32 ulTargetBufAddr;

        // perform check to determine the data copy process is complete, break the loop
        // immediately if it is
        if(ucBufferToBeFlushedIsSource[ucPuNum] == TRUE)
        {
            if(gpBufTag[usSourceLogBufId[ucPuNum]].ulValidLpnBitmap == 0)
            {
                g_ucIsBufferRegulationOngoing[ucPuNum] = FALSE;
                break;
            }
        }
        else
        {
            if(gpBufTag[usTargetLogBufId[ucPuNum]].ulValidLpnBitmap == LPN_PER_BUF_BITMAP)
            {
                g_ucIsBufferRegulationOngoing[ucPuNum] = FALSE;
                break;
            }
        }

        // prepare the source buffer

        if(ucBufferToBeFlushedIsSource[ucPuNum] == FALSE)
        {
            // determine the source buffer if the current one has already been empty, this is
            // only needed for case 2
            if(gpBufTag[usSourceLogBufId[ucPuNum]].ulValidLpnBitmap == 0)
            {
                // find the next non-empty buffer with minimum valid lpn count,
                // please note this operation could fail when all valid LPN data
                // in other buffers in flush pending buffer link has been copied
                // to the buffer to be flushed
                usSourceLogBufId[ucPuNum] = L1_FindNextNonemptyWithMinimumLpnCountLogBufId(g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId);

                // check if we have successfully found a non-empty source buffer
                if(usSourceLogBufId[ucPuNum] != INVALID_4F)
                {
                    // calculate the logical buffer id
                    usSourcePhyBufId[ucPuNum] = PHYBUFID_FROM_LGCBUFID(usSourceLogBufId[ucPuNum]);
                }
                else
                {
                    // there are no more buffers with valid data, break the loop
                    // immediately
                    g_ucIsBufferRegulationOngoing[ucPuNum] = FALSE;
                    break;
                }
            }
        }

        // register the valid LPN bitmap to the hardware register
        HAL_Wclzstate(gpBufTag[usSourceLogBufId[ucPuNum]].ulValidLpnBitmap);

        // get the next LPN offset in the source buffer with valid LPN data
        ucCopySourceLpnOffset = (31 - HAL_SCLZ());

        // get the valid LPN to be moved
        ulCopySourceLpn = L1_GetBufferLPN(usSourceLogBufId[ucPuNum], ucCopySourceLpnOffset);

        // check both the NFC read cachestatus and host write cachestatus to
        // ensure the data of the LPN to be moved is ready
        if(L1_HostPartialHitCheckBusy(usSourceLogBufId[ucPuNum], ulCopySourceLpn) == BUF_BUSY)
        {
            // the data isn't ready, return FALSE immediately
            return FALSE;
        }

#ifndef HOST_SATA
        // for non-SATA modes, we also have to check the host read cachestatus, please note
        // we don't have to do this in SATA mode since we don't lock write buffers with host
        // read cachestatus in SATA mode
        if(L1_CheckCacheStatus(&gpBufTag[usSourceLogBufId[ucPuNum]].usStartHRID, ulCopySourceLpn << SEC_PER_LPN_BITS, SEC_PER_LPN) == BUF_BUSY)
        {
            return FALSE;
        }
#endif

        // prepare the target buffer

        if(ucBufferToBeFlushedIsSource[ucPuNum] == TRUE)
        {
            // determine the copy target buffer if it hasn't been determined or
            // the current target buffer has already been full, this is only needed
            // for case 1
            if(usTargetLogBufId[ucPuNum] == INVALID_4F || gpBufTag[usTargetLogBufId[ucPuNum]].ulValidLpnBitmap == LPN_PER_BUF_BITMAP)
            {
                // find the next non-full buffer, please note this operation can't
                // fail, since we have already known there are multiple invalid
                // entries in buffers of the flush pending buffer link
                usTargetLogBufId[ucPuNum] = (usTargetLogBufId[ucPuNum] == INVALID_4F) ? L1_FindNextNonfullLogBufId(g_WriteBufInfo[ucPuNum].usFlushPendingStartBufId) : L1_FindNextNonfullLogBufId(usTargetLogBufId[ucPuNum]);

                // calculate the logical buffer id
                usTargetPhyBufId[ucPuNum] = PHYBUFID_FROM_LGCBUFID(usTargetLogBufId[ucPuNum]);
            }
        }

        // check the NFC read cachestatus link to ensure we're clear to copy
        // data into invalid data entries of the target buffer
        if(L1_CheckCacheStatusLinkIdle(&gpBufTag[usTargetLogBufId[ucPuNum]].usStartNRID) == BUF_BUSY)
        {
            return FALSE;
        }
        else if(L1_CheckCacheStatusLinkIdle(&gpBufTag[usTargetLogBufId[ucPuNum]].usStartHRID) == BUF_BUSY)
        {
            return FALSE;
        }

        // obtain the free entry bitmap by negating the valid LPN bitmap
        // of the target buffer
        ulTargetBufferFreeEntryBitmap = (~(gpBufTag[usTargetLogBufId[ucPuNum]].ulValidLpnBitmap)) & LPN_PER_BUF_BITMAP;

        // register the free entry bitmap to the hardware register
        HAL_Wclzstate(ulTargetBufferFreeEntryBitmap);

        // get the next free entry in the target buffer
        ucCopyTargetLpnOffset = (31 - HAL_SCLZ());

        // both the source buffer and the target buffer are ready,
        // start copying data

        // calculating the addresses of the source buffer and the target buffer
        ulSourceBufAddr = COM_GetMemAddrByBufferID(usSourcePhyBufId[ucPuNum], TRUE, BUF_SIZE_BITS) + (ucCopySourceLpnOffset << LPN_SIZE_BITS);
        ulTargetBufAddr = COM_GetMemAddrByBufferID(usTargetPhyBufId[ucPuNum], TRUE, BUF_SIZE_BITS) + (ucCopyTargetLpnOffset << LPN_SIZE_BITS);

        // copy a valid LPN from the source to the target
        HAL_DMAECopyOneBlock(ulTargetBufAddr, ulSourceBufAddr, SEC_PER_LPN*SEC_SIZE);
        
        // update information for the source buffer
        L1_CacheInvalidLPNInLCT(usSourceLogBufId[ucPuNum], ucCopySourceLpnOffset, 1);

        // update information for the target buffer
        L1_AddNewCacheIdNode(ulCopySourceLpn, usTargetLogBufId[ucPuNum], ucCopyTargetLpnOffset);
        L1_SetBufferSectorBitmap(usTargetLogBufId[ucPuNum], ucCopyTargetLpnOffset, INVALID_2F);
        L1_SetBufferLPN(usTargetLogBufId[ucPuNum], ucCopyTargetLpnOffset, ulCopySourceLpn);

        // update the write pointer of the write buffer
        L1_BufferSetWritePointer(usTargetLogBufId[ucPuNum], LPN_PER_BUF);
    } // while(TRUE)

    return TRUE;
}
#endif
/********************** FILE END ***************/

