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
Filename     : L1_Debug.c                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20130508     blakezhang     001 first create
*****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "HAL_TraceLog.h"

//specify file name for Trace Log
#define TL_FILE_NUM  L1_Debug_c

extern GLOBAL  PSCMD g_pCurSCmd;
extern GLOBAL  PSCQ g_pL1SCmdQueue;
//extern GLOBAL  U8 l_ucSubCmdWr;  
//extern GLOBAL  CACHESTATUS_MAN *g_pCSManager;

extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;

void MCU1_DRAM_TEXT L1_DbgInit(void)
{
    return;
}
void MCU1_DRAM_TEXT L1_DbgPrintSCMD(SCMD *pSCMD)
{
    SCMD_PRINT ScmdPrint;
  
    if (pSCMD == NULL)
    {
        return;
    }

    ScmdPrint.ulLBA = pSCMD->tMA.ulSubSysLBA;
    ScmdPrint.ucSecLen = pSCMD->tMA.ucSecLen;
    ScmdPrint.ucTag = pSCMD->ucSlotNum;
    ScmdPrint.ucFirst = pSCMD->tMA.ucFirst;
    ScmdPrint.ucLast = pSCMD->tMA.ucLast;
    
    if (pSCMD->tMA.ucOpType == DM_READ)
    {
        TRACE_LOG((void*)&ScmdPrint, sizeof(SCMD_PRINT), SCMD_PRINT, 0, "READ SCMD: ");
    }
    else
    {
        TRACE_LOG((void*)&ScmdPrint, sizeof(SCMD_PRINT), SCMD_PRINT, 0, "WRITE SCMD: ");
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgSCMDShow(void)
{
    U32 ulSCMDCount;

    ulSCMDCount = L1_SCQGetCount();

    if (g_pCurSCmd == NULL)
    {
        TRACE_LOG((void*)&ulSCMDCount, sizeof(U32), U32, 0, "Current SCMD NULL: ");
    }
    else
    {
        TRACE_LOG((void*)&ulSCMDCount, sizeof(U32), U32, 0, "Current SCMD is: ");
        L1_DbgPrintSCMD(g_pCurSCmd);
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgPrintCacheStatusLink(U16 usStartID)
{
    U16  usCurrStatusID;
    CACHE_STATUS_PRINT CacheStatusPrint;

    usCurrStatusID = usStartID;

    while (INVALID_4F != usCurrStatusID)
    {
        CacheStatusPrint.CurrID = usCurrStatusID;
#if 0
        CacheStatusPrint.NextID = g_pCSManager->CS_Entry[usCurrStatusID].usNextID;
        CacheStatusPrint.LBA    = g_pCSManager->CS_Entry[usCurrStatusID].ulStartLBA;
        CacheStatusPrint.SecLen = g_pCSManager->CS_Entry[usCurrStatusID].usSecLen;
#else
        CacheStatusPrint.NextID = g_CS_ID_Entry[usCurrStatusID].usNextID;
        CacheStatusPrint.LBA    = g_CS_Range_Entry[usCurrStatusID].usSecOffInBuf;
        CacheStatusPrint.SecLen = g_CS_Range_Entry[usCurrStatusID].usSecLen;
#endif
        CacheStatusPrint.Status = (U16)(g_pCacheStatus[usCurrStatusID]);

        TRACE_LOG((void*)&CacheStatusPrint, sizeof(CACHE_STATUS_PRINT), CACHE_STATUS_PRINT, 0, "CS_Link: ");

        usCurrStatusID = CacheStatusPrint.NextID;
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgCacheStatusShow(void)
{
    U16  usCurrStatusID;
    CACHE_STATUS_PRINT CacheStatusPrint;

    for (usCurrStatusID = 0; usCurrStatusID < L1_CACHESTATUS_TOTAL; usCurrStatusID++)
    {
        if (SUBSYSTEM_STATUS_INIT == g_pCacheStatus[usCurrStatusID])
        {
            continue;
        }

        if (SUBSYSTEM_STATUS_SUCCESS == g_pCacheStatus[usCurrStatusID])
        {
            continue;
        }

        CacheStatusPrint.CurrID = usCurrStatusID;
#if 0
        CacheStatusPrint.NextID = g_pCSManager->CS_Entry[usCurrStatusID].usNextID;
        CacheStatusPrint.LBA    = g_pCSManager->CS_Entry[usCurrStatusID].ulStartLBA;
        CacheStatusPrint.SecLen = g_pCSManager->CS_Entry[usCurrStatusID].usSecLen;
#else
        CacheStatusPrint.NextID = g_CS_ID_Entry[usCurrStatusID].usNextID;
        CacheStatusPrint.LBA    = g_CS_Range_Entry[usCurrStatusID].usSecOffInBuf;
        CacheStatusPrint.SecLen = g_CS_Range_Entry[usCurrStatusID].usSecLen;
#endif
        CacheStatusPrint.Status = (U16)(g_pCacheStatus[usCurrStatusID]);
    
        TRACE_LOG((void*)&CacheStatusPrint, sizeof(CACHE_STATUS_PRINT), CACHE_STATUS_PRINT, 0, "CS_Show: ");
    }

    return;
}


void MCU1_DRAM_TEXT L1_DbgPrintBuffer(U16 usPhyBufID)
{
    U8  ucOffset;
    U16 usLocBufID;
    U16 usStartID;
    U32 ulLPN;

    BUF_COMM_PRINT BufCommPrint;
    BUF_LPN_PRINT  BufLPNPrint;

    usLocBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

    if (usLocBufID >= L1_BUFFER_TOTAL)
    {
        return;
    }

    BufCommPrint.Stage      = gpBufTag[usLocBufID].Stage;
    BufCommPrint.WPointer   = gpBufTag[usLocBufID].ucWritePointer;
    BufCommPrint.usPhyBufID = usPhyBufID;

    TRACE_LOG((void*)&BufCommPrint, sizeof(BUF_COMM_PRINT), BUF_COMM_PRINT, 0, "BUFF_COMM: ");

    for (ucOffset = 0; ucOffset < gpBufTag[usLocBufID].ucWritePointer; ucOffset++)
    {
        ulLPN = L1_GetBufferLPN(usLocBufID, ucOffset);

        if (INVALID_8F != ulLPN)
        {
            BufLPNPrint.ulLPN     = ulLPN;
            BufLPNPrint.ucPuNum = L1_GetSuperPuFromLCT(LCTINDEX_FROM_LPN(ulLPN));
            BufLPNPrint.ucOffset  = ucOffset;
            BufLPNPrint.SecBitmap = L1_GetBufferSectorBitmap(usLocBufID, ucOffset);

            TRACE_LOG((void*)&BufLPNPrint, sizeof(BUF_LPN_PRINT), BUF_LPN_PRINT, 0, "BUFF_LPN: ");
        }
    }

    if (INVALID_4F != gpBufTag[usLocBufID].usStartHRID)
    {
        usStartID = gpBufTag[usLocBufID].usStartHRID;
        TRACE_LOG((void*)&usStartID, sizeof(U16), U16, 0, "HR Link StartID: ");
        L1_DbgPrintCacheStatusLink(usStartID);
    }

    if (INVALID_4F != gpBufTag[usLocBufID].usStartHWID)
    {
        usStartID = gpBufTag[usLocBufID].usStartHWID;
        TRACE_LOG((void*)&usStartID, sizeof(U16), U16, 0, "HW Link StartID: ");
        L1_DbgPrintCacheStatusLink(usStartID);
    }

    if (INVALID_4F != gpBufTag[usLocBufID].usStartNRID)
    {
        usStartID = gpBufTag[usLocBufID].usStartNRID;
        TRACE_LOG((void*)&usStartID, sizeof(U16), U16, 0, "NR Link StartID: ");
        L1_DbgPrintCacheStatusLink(usStartID);
    }

    if (INVALID_4F != gpBufTag[usLocBufID].usStartNWID)
    {
        usStartID = gpBufTag[usLocBufID].usStartNWID;
        TRACE_LOG((void*)&usStartID, sizeof(U16), U16, 0, "NW Link StartID: ");
        L1_DbgPrintCacheStatusLink(usStartID);
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgBufferShow(void)
{    
    U16 usPhyBufID;
    U16 usPhyBufIDEnd;

    TRACE_LOG((void*)&g_ulBufferStartPhyId, sizeof(U32), U32, 0, "StartPhyId: ");

    usPhyBufIDEnd = L1_BUFFER_TOTAL + g_ulBufferStartPhyId;

    for (usPhyBufID = g_ulBufferStartPhyId; usPhyBufID < usPhyBufIDEnd; usPhyBufID++)
    {
        L1_DbgPrintBuffer(usPhyBufID);
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgPrintWritePuFifo(U8 ucPuNum)
{
    U8  ucBufIndex;
    U16 usPhyBufID;

    TRACE_LOG((void*)&ucPuNum, sizeof(U8), U8, 0, "PU :");
    TRACE_LOG((void*)&(g_WriteBufInfo[ucPuNum]), sizeof(WRITE_BUF_FIFO_INFO), WRITE_BUF_FIFO_INFO, 0);

    for (ucBufIndex = 0; ucBufIndex < (L1_WRITE_BUFFER_PER_PU + 1); ucBufIndex++)
    {
        usPhyBufID = gpWriteBufFifoEntry->usPhyBufID[ucPuNum][ucBufIndex];

        TRACE_LOG((void*)&ucBufIndex, sizeof(U8), U8, 0, "Write Buffer Index in PU : ");
        TRACE_LOG((void*)&usPhyBufID, sizeof(U16), U16, 0, "Write Buffer PhyBufID : ");
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgWritePuFifoShow(void)
{
    U8  ucPuNum;

    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        L1_DbgPrintWritePuFifo(ucPuNum);
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgPrintCacheLine(U8 ucPuNum)
{
    U16 usLocBufID;
    U16 usPhyBufID;

    if (ucPuNum >= SUBSYSTEM_SUPERPU_NUM)
    {
        return;
    }

    usPhyBufID = g_aCacheLine[ucPuNum];
    usLocBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

    if(usPhyBufID != INVALID_4F)
    {
        TRACE_LOG((void*)&ucPuNum, sizeof(U8), U8, 0, "L1_DbgPrintCacheLine with Host Data: ");
        TRACE_LOG((void*)&usPhyBufID, sizeof(U16), U16, 0);
        TRACE_LOG((void*)&usLocBufID, sizeof(U16), U16, 0);
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgCacheLineShow(void)
{
    U8 ucPuNum;

    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        L1_DbgPrintCacheLine(ucPuNum);
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgPrintSubCmd(SUBCMD *pSubCmd)
{
    SUBCMD_PRINT SubCmdPrint;
  
    if (NULL == pSubCmd)
    {
        return;
    }

    SubCmdPrint.ulLCT = pSubCmd->SubCmdAddInfo.ulSubCmdLCT;
    SubCmdPrint.ucPuNum = pSubCmd->SubCmdAddInfo.ucPuNum;
    SubCmdPrint.ucHit = pSubCmd->SubCmdHitResult;
    SubCmdPrint.usPhyBufID = pSubCmd->SubCmdPhyBufferID;
    SubCmdPrint.ucLPNOff = pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT;
    SubCmdPrint.ucLPNCnt = pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT;
    SubCmdPrint.ucSecOff = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT;
    SubCmdPrint.ucSecLen = pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT;
    
    TRACE_LOG((void*)&SubCmdPrint, sizeof(SUBCMD_PRINT), SUBCMD_PRINT, 0, "SubCmd: ");

    return;
}

void MCU1_DRAM_TEXT L1_DbgSubCmdShow(void)
{
#if 0
    U8  ucSubCmdSlotIndex;

    TRACE_LOG((void*)&l_ucSubCmdWr, sizeof(U8), U8, 0, "Current SubCmdSlot:");
    L1_DbgPrintSubCmd(g_pCurSubCmd);

    for (ucSubCmdSlotIndex = 0; ucSubCmdSlotIndex < SUBCMD_ENTRY_DEPTH; ucSubCmdSlotIndex++)
    {
        TRACE_LOG((void*)&ucSubCmdSlotIndex, sizeof(U8), U8, 0, "Slot: ");
        L1_DbgPrintSubCmd(&gpSubCmdEntry->tSubCmd[ucSubCmdSlotIndex]);
    }
#endif

    return;
}

void MCU1_DRAM_TEXT L1_DbgPrintWriteBufReq(BUF_REQ_WRITE *pBufReq)
{
    U8 ucLoop;
    BUF_REQ_PRINT BufReqPrint;
    
    //Fix me : SuperPage LUNs 1:3 & 1:6 TL_Enable() can cause UserExceptionVector().
    return;
    
    if (NULL == pBufReq)
    {
        return;
    }

    BufReqPrint.usPhyBufID = pBufReq->PhyBufferID;
    BufReqPrint.LPNOffset = pBufReq->LPNOffset;
    BufReqPrint.LPNCount = pBufReq->LPNCount;

    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
        BufReqPrint.LPN[ucLoop] = pBufReq->LPN[ucLoop];
    }
    TRACE_LOG((void*)&BufReqPrint, sizeof(BUF_REQ_PRINT), BUF_REQ_PRINT, 0, "BufReq: ");


    return;
}

void MCU1_DRAM_TEXT L1_DbgPrintReadBufReq(BUF_REQ_READ *pBufReq)
{
    U8 ucLoop;
    BUF_REQ_PRINT BufReqPrint;

    //Fix me : SuperPage LUNs 1:3 & 1:6 TL_Enable() can cause UserExceptionVector().
    return;

    if (NULL == pBufReq)
    {
        return;
    }

    BufReqPrint.ucType = L1_REQ_REQTYPE_READ;
    BufReqPrint.usPhyBufID = pBufReq->usPhyBufferID;
    BufReqPrint.LPNOffset = pBufReq->ucLPNOffset;
    BufReqPrint.LPNCount = pBufReq->ucLPNCount;
    BufReqPrint.ReqOffset = pBufReq->ucReqOffset;
    BufReqPrint.ReqLength = pBufReq->ucReqLength;

    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
        BufReqPrint.LPN[ucLoop] = pBufReq->aLPN[ucLoop];
    }

    if (pBufReq->bReqLocalREQFlag == 1)
    {
        TRACE_LOG((void*)&BufReqPrint, sizeof(BUF_REQ_PRINT), BUF_REQ_PRINT, 0, "BufReq Merge: ");
    }
    else
    {
        TRACE_LOG((void*)&BufReqPrint, sizeof(BUF_REQ_PRINT), BUF_REQ_PRINT, 0, "BufReq: ");
    }

    return;
}

void MCU1_DRAM_TEXT L1_DbgBufReqShow(void)
{
    U8 ucHeadID;
    U8  ucBufReqIndex;
    BUF_REQ_READ *pReadBufReq;
    BUF_REQ_WRITE *pWriteBufReq;
    U8 ucPu;

    for(ucPu = 0; ucPu < SUBSYSTEM_SUPERPU_NUM; ++ucPu)
    {
        ucHeadID = g_HighPrioLink[ucPu].ucHeadID;
        TRACE_LOG((void*)&ucHeadID, sizeof(U8), U8, 0, "HighPrioLinkHead: ");

        for (ucBufReqIndex = 0; ucBufReqIndex < PRIO_FIFO_DEPTH; ucBufReqIndex++)
        {
            pReadBufReq = &g_ReadBufReqEntry[ucPu][ucBufReqIndex];
            TRACE_LOG((void*)&ucBufReqIndex, sizeof(U8), U8, 0, "ReadBufReqID: ");
            L1_DbgPrintReadBufReq(pReadBufReq);
        }

        TRACE_LOG((void*)&LowPrioFifoHead[ucPu], sizeof(U8), U8, 0, "LowPrioFifoHead: ");
        TRACE_LOG((void*)&LowPrioFifoTail[ucPu], sizeof(U8), U8, 0, "LowPrioFifoTail: ");

        for (ucBufReqIndex = 0; ucBufReqIndex < PRIO_FIFO_DEPTH; ucBufReqIndex++)
        {
            pWriteBufReq = &gpLowPrioEntry[ucPu]->tBufReq[ucBufReqIndex];
            TRACE_LOG((void*)&ucBufReqIndex, sizeof(U8), U8, 0, "LowPrioFifo: ");
            L1_DbgPrintWriteBufReq(pWriteBufReq);
        }

    }
    return;
}

void MCU1_DRAM_TEXT L1_DbgGlobalInfoShow(void)
{
    TRACE_LOG((void*)g_pSubSystemDevParamPage, sizeof(DEVICE_PARAM_PAGE), DEVICE_PARAM_PAGE, 0);
    return;
}


void MCU1_DRAM_TEXT L1_DbgShowAll(void)
{
  //  DBG_Printf("MCU#%d L1_DbgShowAll.\n", HAL_GetMcuId());
    /* SCMD Status */
    L1_DbgSCMDShow();

    /* All Buffer Status */
    L1_DbgBufferShow();

    /* Write PU Fifo Status */
    L1_DbgWritePuFifoShow();

    /* Cache Status Show */
    L1_DbgCacheStatusShow();

    /* CacheLine Status */
    L1_DbgCacheLineShow();

    /* SubCmd Status */
    L1_DbgSubCmdShow();

    /* BufReq Status */
    L1_DbgBufReqShow();

    /* GlobalInfo Status */
    L1_DbgGlobalInfoShow();

    return;
}

/********************** FILE END ***************/

