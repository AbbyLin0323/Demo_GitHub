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
********************************************************************************
Filename     : HAL_TraceLog.c                                 
Version      : 
Date         : 
Author       : Gavin
Others: 
Modification History:
*******************************************************************************/

#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_TraceLog.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "Disk_Config.h"

/* local pointer to TL_INFO descriptor */
LOCAL MCU12_VAR_ATTR TL_INFO *l_pTlInfo;
/* Trace Log sector size */
LOCAL MCU12_VAR_ATTR U32 l_ulSectorSize;
/* total sector count */
LOCAL MCU12_VAR_ATTR U32 l_ulSectorCnt;
/* specifies write pointer withing a sector */
LOCAL MCU12_VAR_ATTR U32 l_ulWritePtr;
LOCAL MCU12_VAR_ATTR U32 l_ulTracelogEnable;


#define VERSION_NUM 1

#ifdef SIM
/*------------------------------------------------------------------------------
Name: TlSaveMemToFile
Description: 
    save trace memory to file
Input Param:
    U8 *pSrcAddr: start address of trace memory
    U32 ulByteLen: byte length of trace memory
    const char *pFileName: name of the target file
Output Param:
    none
Return Value:
    void
Usage:
    Used on Windows platform only
History:
    20140807    Gavin   create function
------------------------------------------------------------------------------*/
LOCAL void TlSaveMemToFile(U8 *pSrcAddr, U32 ulByteLen, const char *pFileName)
{
    FILE *fp;
    U32 ulWriteByteCnt;

    fp = fopen(pFileName, "wb");//write only, binary format

    if (NULL == fp)
    {
        DBG_Printf("fopen file %s fail\n", pFileName);
        DBG_Getch();
    }
    else
    {
        ulWriteByteCnt = (U32)fwrite(pSrcAddr, 1, ulByteLen, fp);
        if (ulByteLen != ulWriteByteCnt)
        {
            DBG_Printf("fwrite file %s fail, ulByteLen = %d, ulWriteByteCnt = %d\n", pFileName, ulByteLen, ulWriteByteCnt);
            DBG_Getch();
        }

        fclose(fp);
    }

    return;
}
#endif

/*------------------------------------------------------------------------------
Name: TlMemCpy
Description: 
    copy memory by byte unit
Input Param:
    U8 *pDstAddr: target address
    U8 *pDstAddr: source address
    U32 ulByteLen: byte length
Output Param:
    none
Return Value:
    void
Usage:
    only used in this module when need to copy memory by byte
History:
    20140725    Gavin   create function
------------------------------------------------------------------------------*/
LOCAL void TlMemCpy(U8 *pDstAddr, U8 *pSrcAddr, U32 ulByteLen)
{
    while (0 != ulByteLen)
    {
        *pDstAddr++ = *pSrcAddr++;
        ulByteLen--;
    }
}

/*------------------------------------------------------------------------------
Name: TlMoveToNextSec
Description: 
    this routine update local status when a new sector is needed.
Input Param:
    BOOL bUpdateReadPtr: TRUE = update read pointer when write pointer meet it.
                         FALSE = do not update read pointer.
Output Param:
    none
Return Value:
    void
Usage:
    called when previous sector is full or read by host
History:
    20140820    Gavin   create function
    20141022    Gavin   fix a bug when judge log memory round-back condition
    20141229    Gavin   add support for moving read pointer when write pointer meet it.
    20150203    Nina    add support for moving flush pointer when write pointer meet it.
------------------------------------------------------------------------------*/
LOCAL void TlMoveToNextSec(BOOL bUpdateReadPtr,BOOL bUpdataFlushPtr)
{
    TL_SEC_HEADER *pTlSecHeader;
    
    //move write pointer
    l_pTlInfo->ulWriteSec = (l_pTlInfo->ulWriteSec + 1) % l_ulSectorCnt;

    if ((TRUE == bUpdateReadPtr) && (l_pTlInfo->ulReadSec == l_pTlInfo->ulWriteSec))
    {
        //move read pointer
        l_pTlInfo->ulReadSec = (l_pTlInfo->ulReadSec + 1) % l_ulSectorCnt;
    }

    if ((TRUE == bUpdataFlushPtr) && (l_pTlInfo->ulFlushSec == l_pTlInfo->ulWriteSec))
    {
        //move flush pointer
        l_pTlInfo->ulFlushSec = (l_pTlInfo->ulFlushSec + 1) % l_ulSectorCnt;
    }

    //add valid sector count
    l_pTlInfo->ulValidSecCnt++;
    if (l_pTlInfo->ulValidSecCnt > l_ulSectorCnt)
    {
        l_pTlInfo->ulValidSecCnt = l_ulSectorCnt;
    }

    //add valid flush sector count
    l_pTlInfo->ulValidFlushCnt++;
    if (l_pTlInfo->ulValidFlushCnt > l_ulSectorCnt)
    {
        l_pTlInfo->ulValidFlushCnt = l_ulSectorCnt;
    }

    pTlSecHeader = (TL_SEC_HEADER *)((l_pTlInfo->ulWriteSec * l_ulSectorSize) + l_pTlInfo->ulTlMemBase);
    //initialize the new sector
    pTlSecHeader->usLogNum = 0;
    pTlSecHeader->usVersion = 0;

    //set write pointer for next new log 
    l_ulWritePtr = sizeof(TL_SEC_HEADER);

    return;
}

/*------------------------------------------------------------------------------
Name: TL_Initialize
Description: 
    init trace log module: prepare memory and init local configuration.
Input Param:
    ucMcuId: MCU id who call this function. ucMcuId must be MCU0_ID/MCU1_ID/MCU2_ID
    void *pBuffer: pointer to trace log memory
    U32 ulSize: byte length of total trace log memory
    U32 ulSecSize: trace log memory is divided into sectors, ulSecSize specify
                   the size of every sector.
Note:
    pBuffer/ulSize/ulSecSize must be sector align
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function in boot stage;
History:
    20140725    Gavin   port this function from Charles's code
    20140820    Gavin   modify to support multi-core mode
    20150203    Nina    modify to support save trace to flash
------------------------------------------------------------------------------*/
void TL_Initialize(U8 ucMcuId, void* pBuffer, U32 ulSize, U32 ulSecSize)
{
    TL_SEC_HEADER *pTlSecHeader;

    //TL_INFO struct is located in DSRAM1 to get higher performance
    l_pTlInfo = (TL_INFO *)(DSRAM1_TL_SEC_BASE + (ucMcuId - MCU0_ID) * sizeof(TL_INFO));
    COM_MemZero((U32 *)l_pTlInfo, sizeof(TL_INFO)/sizeof(U32));

    l_pTlInfo->ulTlVersion = TL_VERSION;
    l_pTlInfo->ulTlMemBase = (U32)pBuffer;
    l_pTlInfo->ulTlMemSize = ulSize;
    l_pTlInfo->ulTlSecSize = ulSecSize;
    l_pTlInfo->ulWriteSec = 0;
    l_pTlInfo->ulReadSec = 0;
    l_pTlInfo->ulFlushSec = 0;
    l_pTlInfo->ulValidSecCnt = 1;//we treat current sector as valid sector
    l_pTlInfo->ulValidFlushCnt = 1;

    l_ulSectorSize = ulSecSize;
    l_ulSectorCnt = ulSize / ulSecSize;
    
    //set write pointer for next new log 
    l_ulWritePtr = sizeof(TL_SEC_HEADER);

    //initialize the new sector
    pTlSecHeader = (TL_SEC_HEADER *)pBuffer;
    pTlSecHeader->usLogNum = 0;
    pTlSecHeader->usVersion = 0;

    TL_Enable();
    
    return;
}

/*------------------------------------------------------------------------------
Name: TL_TraceLog
Description: 
    record one log
Input Param:
    ucFileNum: the file number of the calling statement.
    ulLineNum: the line number of the calling statement.
    pData    : point to the data that will be written into the trace log buffer.
    ulSize   : the size of the data specified by "pData", in units of byte.
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function in anywhere need to record log;
History:
    20140820    Gavin   port this function from Charles's code
------------------------------------------------------------------------------*/
void TL_TraceLog(U8 ucFileNum, U32 ulLineNum, void* pData, U32 ulSize)
{
    TL_SEC_HEADER *pTlSecHeader;
    TL_LOG_HEADER *pTlLogHeader;
    TL_LOG_HEADER tTlLogHeader;

    if(FALSE == l_ulTracelogEnable)
    {
        return;
    }

    //log size can not overflow a sector
    if (ulSize > (l_ulSectorSize - sizeof(TL_SEC_HEADER) - sizeof(TL_LOG_HEADER)))
    {
        ulSize = l_ulSectorSize - sizeof(TL_SEC_HEADER) - sizeof(TL_LOG_HEADER);
        ulLineNum = 0;//set line number to 0, which marks the log as invalid
    }
    
    if (REST_SPACE_IN_SEC(l_ulWritePtr, l_ulSectorSize) < (ulSize + sizeof(TL_LOG_HEADER)))
    {
        // skip current sector to use the next sector.
        TlMoveToNextSec(TRUE,TRUE);
    }
    else
    {
        // update log number in current sector
        pTlSecHeader = (TL_SEC_HEADER *)(l_pTlInfo->ulWriteSec * l_ulSectorSize + l_pTlInfo->ulTlMemBase);
        pTlSecHeader->usLogNum++;
    }

    //we can not use pTlLogHeader directly, because it may cause address alignment issue  
    tTlLogHeader.bsFileNum = ucFileNum;
    tTlLogHeader.bsLineNum = ulLineNum;
    pTlLogHeader = (TL_LOG_HEADER *)(l_pTlInfo->ulWriteSec * l_ulSectorSize + l_pTlInfo->ulTlMemBase + l_ulWritePtr);
    TlMemCpy((U8 *)pTlLogHeader, (U8 *)&tTlLogHeader, sizeof(TL_LOG_HEADER));
    
    TlMemCpy((U8 *)((U32)pTlLogHeader + sizeof(TL_LOG_HEADER)), (U8*)pData, ulSize);

    l_ulWritePtr += (ulSize + sizeof(TL_LOG_HEADER));
    if (l_ulWritePtr == l_ulSectorSize)
    {
        TlMoveToNextSec(TRUE,TRUE);
    }

    return;
}

/*------------------------------------------------------------------------------
Name: TL_InvalidateTraceMemory
Description: 
    Invalidate trace memory which will be loaded to host side.
Input Param:
    U32 ulSectorCnt: sector count of trace memory, starting from read pointer.
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function when receive host request for reading trace memory
History:
    20141229    Gavin   create function
------------------------------------------------------------------------------*/
void TL_InvalidateTraceMemory(U32 ulSectorCnt)
{
    if (ulSectorCnt > l_pTlInfo->ulValidSecCnt)
    {
        DBG_Printf("Trace Log Err: read sector count(%d) > valid sector count(%d)\n", ulSectorCnt, l_pTlInfo->ulValidSecCnt);
        DBG_Getch();
    }

    // reduce valid sector
    l_pTlInfo->ulValidSecCnt -= ulSectorCnt;
    if (0 == l_pTlInfo->ulValidSecCnt)
    {
        TlMoveToNextSec(FALSE,FALSE);
    }

    // move read pointer
    l_pTlInfo->ulReadSec = (l_pTlInfo->ulReadSec + ulSectorCnt) % l_ulSectorCnt;

    return;
}

/*------------------------------------------------------------------------------
Name: TL_InvalidateFlushMemory
Description: 
    Invalidate trace memory which has been flushed into trace block.
Input Param:
    U32 ulSectorCnt: sector count of trace memory.
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function after save trace to flash.
History:
    20150203    NinaYang   create function
------------------------------------------------------------------------------*/
void TL_InvalidateFlushMemory(U32 ulSectorCnt)
{
    // reduce valid flush sector
    l_pTlInfo->ulValidFlushCnt -= ulSectorCnt;

    if (0 == l_pTlInfo->ulValidFlushCnt)
    {
        TlMoveToNextSec(FALSE,FALSE);
    }

    // move Flush pointer
    l_pTlInfo->ulFlushSec = (l_pTlInfo->ulFlushSec + ulSectorCnt) % l_ulSectorCnt;

    return;
}

/*------------------------------------------------------------------------------
Name: TL_IsFlushMemoryEmpty
Description: 
    check whether has trace not flushed to flash
Input Param:
Output Param:
    none
Return Value:
    void
Usage:
History:
    20150203    NinaYang   create function
------------------------------------------------------------------------------*/
BOOL TL_IsFlushMemoryEmpty()
{
    if (l_pTlInfo->ulFlushSec == l_pTlInfo->ulWriteSec && l_ulWritePtr == sizeof(TL_SEC_HEADER))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*------------------------------------------------------------------------------
Name: TL_GetTLInfo
Description: 
    get TLInfo for multiple MCU
Input Param:
    U32 ulMcuId
Output Param:
    none
Return Value:
    TL_INFO*
Usage:
History:
    20150203    NinaYang   create function
------------------------------------------------------------------------------*/
TL_INFO* TL_GetTLInfo(U32 ulMcuId)
{
    return (TL_INFO *)(DSRAM1_TL_SEC_BASE + (ulMcuId - MCU0_ID) * sizeof(TL_INFO));
}

/*------------------------------------------------------------------------------
Name: TL_IsTraceMemory
Description: 
    decide 
Input Param:
    U32 ulMcuId
Output Param:
    none
Return Value:
    TRUE/FALSE
Usage:
History:
    20150211    NinaYang   create function
------------------------------------------------------------------------------*/
BOOL TL_IsTraceMemory(U32 ulAddr)
{
    if(ulAddr >= l_pTlInfo->ulTlMemBase && 
        ulAddr <= (l_pTlInfo->ulTlMemBase + l_pTlInfo->ulTlMemSize))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
/*------------------------------------------------------------------------------
Name: TL_PrepareFlushData
Description: 
    prepare flush memory.
Input Param:
    U32* pFlushBuf:Need to flush buffer addr,
    U8* pTempBuf:
        1.if flush trace buffer addr is not flash page align,need copy data to TempBuf.
        2.if flush trace buffer is not continous,need copy data to TempBuf.
    U32* pSecCnt:Flush sector cnt once.
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function when save trace to flash.
History:
    20150203    NinaYang   create function
------------------------------------------------------------------------------*/
void TL_PrepareFlushData(U32 ulMcuId,U32* pFlushBuf,U8* pTempBuf,U32* pSecCnt,U32 ulSaveSize)
{
    TL_INFO* pTlInfo = (TL_INFO *)(DSRAM1_TL_SEC_BASE + (ulMcuId - MCU0_ID) * sizeof(TL_INFO));

    U32 ulFlushMemSize = pTlInfo->ulValidFlushCnt*l_ulSectorSize;
    U32 ulFlushBufAddr = pTlInfo->ulTlMemBase + pTlInfo->ulFlushSec*l_ulSectorSize;
    U32 ulTLMemEndAddr = pTlInfo->ulTlMemBase + pTlInfo->ulTlMemSize;

    if(MCU0_ID == ulMcuId)
    {
        ulFlushBufAddr = pTlInfo->ulTlMemBase + ulSaveSize;
        *pFlushBuf = ulFlushBufAddr;
        *pSecCnt = BUF_SIZE/l_ulSectorSize;
        return;
    }
 
    if(ulFlushMemSize > (ulTLMemEndAddr - ulFlushBufAddr))
    {
        if( (ulTLMemEndAddr - ulFlushBufAddr) >= BUF_SIZE)
        {
            COM_MemCpy((U32*)pTempBuf,(U32*)ulFlushBufAddr, BUF_SIZE/sizeof(U32));
        }
        else
        {
            COM_MemCpy((U32*)pTempBuf,(U32*)ulFlushBufAddr, (ulTLMemEndAddr - ulFlushBufAddr)/sizeof(U32));
            pTempBuf += (ulTLMemEndAddr - ulFlushBufAddr);
            if(ulFlushMemSize >= BUF_SIZE)
            {
                COM_MemCpy((U32*)pTempBuf,(U32*)pTlInfo->ulTlMemBase,(BUF_SIZE - (ulTLMemEndAddr-ulFlushBufAddr))/sizeof(U32));
            }
            else
            {
                COM_MemCpy((U32*)pTempBuf,(U32*)pTlInfo->ulTlMemBase,(ulFlushMemSize - (ulTLMemEndAddr-ulFlushBufAddr))/sizeof(U32));
            }
        }
        *pSecCnt = min(BUF_SIZE/l_ulSectorSize,pTlInfo->ulValidFlushCnt);
        *pFlushBuf = (U32)pTempBuf;
    }
    else
    {
        //Flash page align
        if(0 == (ulFlushBufAddr & BUF_SIZE_MSK))
        {
            *pFlushBuf = ulFlushBufAddr;
            *pSecCnt = min(BUF_SIZE/l_ulSectorSize,pTlInfo->ulValidFlushCnt);
        }
        //Not flash page align,need memory copy
        else
        {
            *pFlushBuf = (U32)pTempBuf;
            *pSecCnt = min(BUF_SIZE/l_ulSectorSize,pTlInfo->ulValidFlushCnt);
            COM_MemCpy((U32*)pTempBuf,(U32*)ulFlushBufAddr,((*pSecCnt)*l_ulSectorSize)/sizeof(U32));
        }
    }
    return;
}

/*==============================================================================
Func Name  : TL_Enable
Input      : void  
Output     : NONE
Return Val : 
Discription: enable recording trace log.
Usage      : 
History    : 
    1. 2015.2.12 Haven Yang create function
==============================================================================*/
void TL_Enable(void)
{
    l_ulTracelogEnable = TRUE;
}

/*==============================================================================
Func Name  : TL_Disable
Input      : void  
Output     : NONE
Return Val : 
Discription: disable recording trace log, if call this interface, trace log will
             never record untill TL_Enable() was called again.
Usage      : 
History    : 
    1. 2015.2.12 Haven Yang create function
==============================================================================*/
void TL_Disable(void)
{
    l_ulTracelogEnable = FALSE;
}


/* end of file HAL_TraceLog.c */

