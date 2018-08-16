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

#include <math.h>
//#include "HAL_Inc.h"
#include "HAL_SataDSG.h"
#include "HAL_BufMap.h"
#include "simsatainc.h"

extern CRITICAL_SECTION g_BUFMAPCriticalSection;
CRITICAL_SECTION g_ReadPrdCriticalSection;
CRITICAL_SECTION g_WritePrdCriticalSection;
CRITICAL_SECTION g_FirstDataReadyCriticalSection;

U32 g_ReadBufferMap[64];
U32 g_WriteBufferMap[64];

//SATA_PRD_ENTRY* g_pSDCReadPrd;//sim_rprd;
//SATA_PRD_ENTRY* g_pSDCWritePrd;//sim_wprd;

U8 l_FirstDataReady[SIM_HOST_CMD_MAX + 1]={0};
U8 g_LastDataReady[SIM_HOST_CMD_MAX]={0};

FIFO_STRUCT l_stFirstDataReady = {0};

BOOL FifoIsEmpty(FIFO_STRUCT *pFifoStrct)
{
    BOOL bEmpty = FALSE;
    U32 ulFifoTail = pFifoStrct->stFifoTail;
    U32 ulFifoHead = pFifoStrct->stFifoHead;
    U32 ulMaxNode = pFifoStrct->stMaxNode;

    if (ulFifoHead == ulFifoTail)
    {
        bEmpty = TRUE;
    }

    return bEmpty;
}

BOOL FifoIsFull(FIFO_STRUCT *pFifoStrct)
{
    BOOL bFull = FALSE;
    U32 ulFifoTail = pFifoStrct->stFifoTail;
    U32 ulFifoHead = pFifoStrct->stFifoHead;
    U32 ulMaxNode = pFifoStrct->stMaxNode;

    if ((ulFifoTail + 1)%ulMaxNode == ulFifoHead)
    {
        bFull = TRUE;
    }

    return bFull;
}

BOOL FifoGetNode(FIFO_STRUCT *pFifoStrct, void *pNodeOut)
{
    BOOL bSuccess = TRUE;
    void *pSourceAddr = 0;
    U32 ulFifoTail = pFifoStrct->stFifoTail;
    U32 ulFifoHead = pFifoStrct->stFifoHead;
    U32 ulMaxNode = pFifoStrct->stMaxNode;

    // Fifo empty
    if (TRUE == FifoIsEmpty(pFifoStrct))
    {
        bSuccess = FALSE;
    }
    else
    {
        pSourceAddr = (U8*)pFifoStrct->pFifo + (pFifoStrct->stFifoHead * pFifoStrct->stNodeByteLen);
        memcpy(pNodeOut, pSourceAddr, pFifoStrct->stNodeByteLen);
    }

    return bSuccess;
}


BOOL FifoSearchNode(FIFO_STRUCT *pFifoStrct, const void *pNodeValue)
{
    BOOL bSuccess = FALSE;
    void *pSourceAddr = 0;
    U32 ulFifoTail = pFifoStrct->stFifoTail;
    U32 ulFifoHead = pFifoStrct->stFifoHead;
    U32 ulMaxNode = pFifoStrct->stMaxNode;
    U32 ulNodeIndex = pFifoStrct->stFifoHead;
    U32 ulNodeByteLen = pFifoStrct->stNodeByteLen;

    // Fifo empty
    if (ulFifoHead == ulFifoTail)
    {
        bSuccess = FALSE;
    }
    else
    {
        while (ulNodeIndex != ulFifoTail)
        {
            pSourceAddr = (U8*)pFifoStrct->pFifo + (ulNodeIndex * ulNodeByteLen);
            if (0 == memcmp(pSourceAddr, pNodeValue, ulNodeByteLen))
            {
                bSuccess = TRUE;
                break;
            }

            ulNodeIndex = (ulNodeIndex+1)%ulMaxNode;
        }

    }

    return bSuccess;
}

BOOL FifoPushNode (FIFO_STRUCT *pFifoStrct, const void *pNodeIn)
{
    BOOL bSuccess = TRUE;
    void *pDstAddr = 0;
    U32 ulFifoTail = pFifoStrct->stFifoTail;
    U32 ulFifoHead = pFifoStrct->stFifoHead;
    U32 ulMaxNode = pFifoStrct->stMaxNode;

    // Fifo full
    if (TRUE == FifoIsFull(pFifoStrct))
    {
        DBG_Printf("FifoPushNode: Fifo full! \n");
        bSuccess = FALSE;
    }
    else
    {
        pDstAddr = (U8*)pFifoStrct->pFifo + ((ulFifoTail%ulMaxNode) * pFifoStrct->stNodeByteLen);
        memcpy(pDstAddr, pNodeIn, pFifoStrct->stNodeByteLen);

        pFifoStrct->stFifoTail = (ulFifoTail + 1)%ulMaxNode; // point to next empty node

    }

    return bSuccess;
}

BOOL FifoPopNode (FIFO_STRUCT *pFifoStrct)
{
    BOOL bSuccess = TRUE;
    void *pDstAddr = 0;
    U32 ulFifoTail = pFifoStrct->stFifoTail;
    U32 ulFifoHead = pFifoStrct->stFifoHead;
    U32 ulMaxNode = pFifoStrct->stMaxNode;

    // Fifo empty
    if (TRUE == FifoIsEmpty(pFifoStrct))
    {
        DBG_Printf("FifoPopNode: Fifo empty! \n");
        bSuccess = FALSE;
    }
    else
    {

        pFifoStrct->stFifoHead = (ulFifoHead + 1) %ulMaxNode;
    }

    return bSuccess;
}


/*get next wprd for processing*/

BOOL SDC_IsLastPrd(SATA_DSG* p_prd)
{
    if(p_prd->XferCtrlInfo.Eot)
    {
        return TRUE;
    }

    return FALSE;
}

// function about buffermap
void SDC_SetWriteBufferMap(U8 bufmap_id,U32 mapvalue)
{
    g_WriteBufferMap[bufmap_id] = mapvalue;
}


void SDC_ClearWriteBufferMap(U8 bufmap_id)
{
    g_WriteBufferMap[bufmap_id] = 0x0;
}


U32 SDC_GetWriteBufferMap(U8 bufmap_id)
{
    return g_WriteBufferMap[bufmap_id];
}

/****************************************************************************
Name        :SDC_SetReadBufferMap
Input       :bufmap_vale: bufmapvalue << 8 | opcode <<6 | buffermapid
Output      :
Author      :
Date        :
Description :
Others      : it will be called when the rBufMapSet trigger
Modify      :
    2013.11.05  HavenYang add mask process and change bufmap to 8+8=16 bits
****************************************************************************/
void SDC_SetReadBufferMap(U32 bufmap_value)
{

    U8 bufmap_id = bufmap_value & 0x03F;
    U8 opcode = (bufmap_value >> 6) & 0x03;
    U8 bufmap = (bufmap_value >> 8) & 0xFF;
    U8 mask   = (bufmap_value >> 16)& 0xFF;

    EnterCriticalSection(&g_BUFMAPCriticalSection);

    if (2 == opcode)
    {
        g_ReadBufferMap[bufmap_id] = (g_ReadBufferMap[bufmap_id] & 0x0000FF00)
        | ((bufmap & (~mask)) | (g_ReadBufferMap[bufmap_id] & mask));
    }
    else if(3 == opcode)
    {
        g_ReadBufferMap[bufmap_id] = (g_ReadBufferMap[bufmap_id] & 0x000000FF)
        | (((bufmap & (~mask)) | ((g_ReadBufferMap[bufmap_id]>>8) & mask))<<8);
    }
    else
    {
        printf("set read buffer error. ");
        DBG_Getch();
    }

    LeaveCriticalSection(&g_BUFMAPCriticalSection);
}

void SDC_SetReadBufferMapWin(U8 buffmap_id,U32 buffermap_value)
{
    g_ReadBufferMap[buffmap_id] = buffermap_value;
}


U32 SDC_GetReadBufferMap(U8 buffmap_id)
{
    return g_ReadBufferMap[buffmap_id];
}


void SDC_ClearReadBufferMap(U8 bufmap_id,U32 bufmap_value)
{
    g_ReadBufferMap[bufmap_id] &= ~bufmap_value;
}

void SDC_InitReadBufferMap(U8 bufmap_id,U32 bufmap_value)
{
    g_ReadBufferMap[bufmap_id] = bufmap_value;
}

void SDC_InitFirstDataRead(void)
{
    l_stFirstDataReady.pFifo = &l_FirstDataReady;
    l_stFirstDataReady.stMaxNode = SIM_HOST_CMD_MAX + 1;
    l_stFirstDataReady.stFifoHead = 0;
    l_stFirstDataReady.stFifoTail = 0;
    l_stFirstDataReady.stNodeByteLen = sizeof(U8);
    memset(&g_LastDataReady, 0, SIM_HOST_CMD_MAX*sizeof(U8));
}

void SDC_SetFirstReadDataReady(U8 uRegValue)
{
    U8 uTag = uRegValue &0x1F;
    U8 nRegValue = 0;

    EnterCriticalSection(&g_FirstDataReadyCriticalSection);
    if (TRUE == FifoSearchNode(&l_stFirstDataReady, &uTag))
    //if (1 == g_FirstReadDataReady[uTag])
    {
        DBG_Printf("SDC_SetFirstReadDataReady %d cmd set ready again! \n", uTag);
        DBG_Getch();
    }
    else
    {
        //g_FirstReadDataReady[uTag] = 1;
        FifoPushNode(&l_stFirstDataReady, &uTag);
        regWrite((U32)&rReadDataStatusInit, 1, (U8*)&nRegValue);
    }
    LeaveCriticalSection(&g_FirstDataReadyCriticalSection);
}

void SDC_ClearFirstReadDataReady(U8 uTag)
{

    U8 ulTagInFifo = 0xFF;
    BOOL bSuccess = FALSE;

    EnterCriticalSection(&g_FirstDataReadyCriticalSection);
    bSuccess = FifoGetNode(&l_stFirstDataReady, &ulTagInFifo);
    if ((FALSE == bSuccess) || (ulTagInFifo != uTag))
    {
        DBG_Printf("SDC_ClearFirstReadDataReady %d cmd clear ready again! \n", uTag);
        DBG_Getch();
    }

    bSuccess = FifoPopNode(&l_stFirstDataReady);
    if (FALSE == bSuccess)
    {
        DBG_Printf("SDC_ClearFirstReadDataReady %d cmd clear ready failuer! \n", uTag);
        DBG_Getch();
    }
    LeaveCriticalSection(&g_FirstDataReadyCriticalSection);

    return;
}

BOOL SDC_IsFirstReadDataReady(U8 uTag)
{
    U8 uDstTag = uTag;
    return FifoSearchNode(&l_stFirstDataReady, &uTag);
}

BOOL SDC_GetFirstDataReady(U8 *pTag)
{
    U8 ulTagInFifo = 0xFF;
    BOOL bSuccess = FALSE;

    bSuccess = FifoGetNode(&l_stFirstDataReady, &ulTagInFifo);
    //if ((FALSE == bSuccess))
    //{
    //    DBG_Printf("SDC_GetFirstDataReady failure \n");
    //    DBG_Getch();
    //}

    *pTag = ulTagInFifo;
    return bSuccess;
}

void SDC_SetLastDataReady(U8 uRegValue)
{
    U8 uTag = uRegValue &0x1F;
    U8 nRegValue = 0;

    if (1 == g_LastDataReady[uTag])
    {
        DBG_Printf("SDC_SetLastDataReady %d cmd set ready again! \n", uTag);
        DBG_Getch();
    }
    else
    {
        g_LastDataReady[uTag] = 1;
        regWrite((U32)&rLastDataStatusInit, 1, (U8*)&nRegValue);
    }
}

void SDC_ClearLastDataReady(U8 uTag)
{
    if (0 == g_LastDataReady[uTag])
    {
        DBG_Printf("SDC_ClearLastDataReady %d cmd clear last ready again! \n", uTag);
        DBG_Getch();
    }

    g_LastDataReady[uTag] = 0;
}

BOOL SDC_IsLastDataReady(U8 uTag)
{
    return g_LastDataReady[uTag];
}

BOOL SDC_NoLastDataReady(void)
{
    U8 ucTag;

    for(ucTag = 0; ucTag < SIM_HOST_CMD_MAX; ucTag++)
    {
        if(1 == g_LastDataReady[ucTag])
        {
            return FALSE;
        }
    }

    return TRUE;
}

