/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_Debug.c
Version     :Ver 1.0
Author      :Blakezhang
Date        :2012.12.07
Description :Main function for Debug L1
Others      :
Modify      :
****************************************************************************/

#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "L1_Debug.h"
#include "HAL_SataDSG.h"
#ifdef HOST_PATTERN_RECORD
#include "COM_Event.h"
#endif

//#ifdef L2_FORCE_VIRTUAL_STORAGE
#include "L2_VirtualStorage.h"
//#endif


//#include "../tracetool/Trace_Dev.h"

U8  ucL1DbgState;
U32 gTmpLPN;
U32 gTmpFlashStatus;
extern BUFF_TAG gBuffTag[L1_BUFFER_TOTAL];

/*
bPrintUart:
    real-time print pattern by DBG_Printf(), will reduce system performance mostly.
bSaveMemory:
    real-time record pattern but dont print, will not impact system performance.
bSendMemory:
    enable this flag to send pattern saved in memory by means of UART.
*/
BOOL bPrintUart;
BOOL bSaveMemory;
BOOL bSendMemory;
U32 ulPatternIndex;
#if 0
void DRAM_ATTR L1_DbgInit()
{
    ucL1DbgState   = 0;
    ulPatternIndex = 0;
    bPrintUart     = FALSE;
    bSaveMemory    = FALSE;
    bSendMemory    = FALSE;

    return;
}
void DRAM_ATTR L1_DbgPrintHCMD(HCMD *pHCMD)
{
/*    
    if (pHCMD == NULL)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgPrintHCMD pHCMD NULL!\n");
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,10,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,10,"L1_DbgPrintHCMD pHCMD NULL!\n");
        return;
    }

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "HCMD: TAG %d RW %d LBA 0x%x Cnt %d Rem %d\n", 
        pHCMD->ucCmdTag, pHCMD->ucCmdRW, pHCMD->ulCmdLba, pHCMD->ulCmdSectorCnt, pHCMD->ulCmdRemSector);
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,11,(pHCMD->ucCmdTag | pHCMD->ucCmdRW << 8),
      pHCMD->ulCmdLba,pHCMD->ulCmdSectorCnt,pHCMD->ulCmdRemSector);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,11,"HCMD: TAG %u[23:16] RW %u[31:24] LBA %x[63:32] Cnt %u[95:64] Rem %u[127:96]\n");
*/
    return;
}

#ifdef HCMD_PU_QUEUE
void DRAM_ATTR L1_DbgPrintHCMDPUQ(U8 PuNum)
{
    U8 ucLoop;

    if (PuNum >= PU_NUM)
    {
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,70,PuNum,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,70,"L1_DbgPrintHCMDPUQ PuNum %u[23:16] ERROR!\n");
        return;
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,71,PuNum,gPuCmdQ[PuNum].tail, gPuCmdQ[PuNum].head,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,71,"HCMD PUQ PuNum %u[23:16] Tail %u[39:32] Head %u[71:64]\n");

#ifndef HCMD_PUQ_IN_OTFB
    for (ucLoop = 0; ucLoop < PUCMDQ_SLOT_SIZE+1; ucLoop++)
    {
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,72,ucLoop,gPuCmdQ[PuNum].cmdSlot[ucLoop],0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,72,"HCMD PUQ cmdSlot %u[23:16] TAG %u[39:32]\n");
    }
#else
    for (ucLoop = 0; ucLoop < NCQ_DEPTH+1; ucLoop++)
    {
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,73,ucLoop,(U8)L1_GetPuQCmdTag(PuNum, ucLoop),0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,73,"HCMD PUQ cmdSlot %u[23:16] TAG %u[39:32]\n");
    }
#endif

    return;
}
#endif

void DRAM_ATTR L1_DbgHCMDShow()
{
/*    
    U8 ucLoop;
    HCMD * pHCMD;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,48,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,48,"L1_DbgHCMDShow Enter\n");

    if (gCurHCMD == NULL)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "Current HCMD: NULL\n"); 
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,12,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,12,"Current HCMD: NULL\n");
    }
    else
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "Current HCMD: \n");
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,13,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,13,"Current HCMD: \n");
        L1_DbgPrintHCMD(gCurHCMD);
    }

    for (ucLoop = 0; ucLoop < NCQ_DEPTH; ucLoop++)
    {
        pHCMD = &HostCmdSlot[ucLoop];
        L1_DbgPrintHCMD(pHCMD);
    }

#ifdef HCMD_PU_QUEUE
    for (ucLoop = 0; ucLoop < PU_NUM; ucLoop++)
    {
        L1_DbgPrintHCMDPUQ(ucLoop);
    }
#endif

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,49,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,49,"L1_DbgHCMDShow Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgPrintBuffer(U16 usPhyBufID)
{
    U8 ucLoop;
    U16 usLocBufID;
    BUFF_TAG *pBufTag;
/*
    usLocBufID = LGCBUFID_FROM_PHYBUFID(usPhyBufID);

    if (usLocBufID >= L1_BUFFER_TOTAL)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgPrintBuffer usPhyBufID 0x%x ERROR\n", usPhyBufID);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,15,usPhyBufID,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,15,"L1_DbgPrintBuffer usPhyBufID %x[31:16] ERROR\n");
        return;
    }

    pBufTag = &gBuffTag[usLocBufID];

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFFER: Loc %d Phy 0x%x Stage %d Type %d Pos %d Size %d SplitSize %d LocalEn %d writePtr %d\n", 
        usLocBufID, usPhyBufID, pBufTag->Stage, pBufTag->Type, pBufTag->Position, pBufTag->BufferSize, pBufTag->SplitSize, pBufTag->LocalREQEnable, pBufTag->ucWritePointer);
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,16,usLocBufID,usPhyBufID,*(U32*)pBufTag,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,16,"BUFFER: Loc %u[31:16] usPhyBufID %x[63:32] Stage %d[67:64] Type %d[71:68] Pos %d[73:72] Size %d[75:74] SplitSize %d[77:76] LocalEn %d[79:78] writePtr %d[87:80]\n");

    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFFER: FlashStatus 4K Split: Offset %d Addr 0x%x Val %d\n", ucLoop,
            (U32)(&g_pFlashStatus[usLocBufID*LPN_PER_BUF + ucLoop]), g_pFlashStatus[usLocBufID*LPN_PER_BUF + ucLoop]);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,17,ucLoop,(U32)(&g_pFlashStatus[usLocBufID*LPN_PER_BUF + ucLoop]),g_pFlashStatus[usLocBufID*LPN_PER_BUF + ucLoop],0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,17,"BUFFER: FlashStatus 4K Split: Offset %u[31:16] Addr %x[63:32] Val %u[95:64]\n");
    }

    if (usLocBufID < L1_BUFFER_COUNT_READ)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFFER: CacheStatus Current Split: Addr 0x%x Val %d\n", 
            (U32)(&g_pBufBusy[usLocBufID]), g_pBufBusy[usLocBufID]);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,18,0,(U32)(&g_pBufBusy[usLocBufID]),g_pBufBusy[usLocBufID],0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,18,"BUFFER: CacheStatus Current Split: Addr %x[63:32] Val %u[95:64]\n");
    }
    else
    {
        for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
        {
            FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFFER: CacheStatus 4K Split: Offset %d Addr 0x%x Val %d\n", ucLoop,
                (U32)(&g_pBufBusy[usLocBufID*LPN_PER_BUF + ucLoop]), g_pBufBusy[usLocBufID*LPN_PER_BUF + ucLoop]);
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,19,ucLoop,(U32)(&g_pBufBusy[usLocBufID*LPN_PER_BUF + ucLoop]),g_pBufBusy[usLocBufID*LPN_PER_BUF + ucLoop],0);
            FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,19,"BUFFER: CacheStatus 4K Split: Offset %u[31:16] Addr %x[63:32] Val %u[95:64]\n");
        }

        for (ucLoop = 0; ucLoop < pBufTag->ucWritePointer; ucLoop++)
        {
            FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFFER: Index %d LPN 0x%x SectorBitmap 0x%x \n", ucLoop, L1_GetBufferLPN(usLocBufID, ucLoop), 
              L1_GetBufferSectorBitmap(usLocBufID, ucLoop));
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,22,ucLoop,L1_GetBufferLPN(usLocBufID, ucLoop),L1_GetBufferSectorBitmap(usLocBufID, ucLoop),0);
            FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,22,"BUFFER: LPNOffset %u[31:16] LPN %x[63:32] SectorBitmap %x[95:64] \n");
        }
    }
*/
    return;
}

void DRAM_ATTR L1_DbgPuFifoShow()
{
/*
    U8  ucPuNum;
    U8  ucBufIndex;
    U16 usPhyBufID;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,52,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,52,"L1_DbgPuFifoShow Enter\n");

    for (ucPuNum = 0; ucPuNum < PU_NUM; ucPuNum++)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "++++++++++++++++++++  PUFifo %d: Head %d Tail %d Merge %d Flush %d Recycle %d  +++++++++++++++++ \n", 
            ucPuNum, PUFifoHead[ucPuNum], PUFifoTail[ucPuNum], PUFifoMerge[ucPuNum], PUFifoFlush[ucPuNum], PUFifoRecycle[ucPuNum]); 
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,20,ucPuNum, PUFifoHead[ucPuNum] | (PUFifoTail[ucPuNum] << 8) | (PUFifoMerge[ucPuNum] << 16), 
          (PUFifoFlush[ucPuNum] | PUFifoRecycle[ucPuNum] << 8),0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,20,"\n+++++++++++++++  PUFifo %u[23:16]: Head %u[39:32] Tail %u[47:40] Merge %u[55:48] Flush %u[71:64] Recycle %u[79:72]  ++++++++++++ \n");

        for (ucBufIndex = 0; ucBufIndex < L1_BUFFER_PER_PU; ucBufIndex++)
        {
            usPhyBufID = s_aWriteBufFifoEntry[ucPuNum][ucBufIndex];
            FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BufIndex %d usPhyBufID 0x%x Details: \n",ucBufIndex, usPhyBufID);
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,21,ucBufIndex, usPhyBufID,ucPuNum,0);
            FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,21,"PuNum %u[95:64] BufIndex %u[23:16] usPhyBufID %x[47:32]\n");
          //  L1_DbgPrintBuffer(usPhyBufID);
        }
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,53,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,53,"L1_DbgPuFifoShow Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgBufferShow(U16 usPhyBufID)
{    
    L1_DbgPrintBuffer(usPhyBufID);

    return;
}

void DRAM_ATTR L1_DbgBufferShowALL()
{
/*    
    U16 usPhyBufID;
    U16 usPhyBufIDEnd;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,50,0,g_ulBufferStartPhyId,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,50,"L1_DbgBufferShowALL Enter StartPhyId %x[63:32]\n");

    usPhyBufIDEnd = L1_BUFFER_TOTAL + PHYBUFID_FROM_LGCBUFID(0);

    for (usPhyBufID = PHYBUFID_FROM_LGCBUFID(0); usPhyBufID < usPhyBufIDEnd; usPhyBufID++)
    {
        L1_DbgPrintBuffer(usPhyBufID);
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,51,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,51,"L1_DbgBufferShowALL Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgPrintCacheLine (U16 usCacheLine)
{}

void DRAM_ATTR L1_DbgCacheLineShow(U16 usCacheLine)
{
    L1_DbgPrintCacheLine(usCacheLine);
    return;
}

void DRAM_ATTR L1_DbgCacheLineShowALL()
{
/*
    U16 usCacheLine;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,54,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,54,"L1_DbgCacheLineShowALL Enter\n");

    for (usCacheLine = 0; usCacheLine < CACHE_LINE_NUM; usCacheLine++)
    {
        L1_DbgPrintCacheLine(usCacheLine);
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,55,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,55,"L1_DbgCacheLineShowALL Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgPrintSubCmd(SUBCMD *pSubCmd)
{
/*    
    if (pSubCmd == NULL)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgPrintSubCmd pSubCmd NULL!\n");
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,27,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,27,"L1_DbgPrintSubCmd pSubCmd NULL!\n");
        return;
    }

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "SUBCMD TAG %d LBA %x RW %d IN(SOff %d SCnt %d LOff %d LCnt %d) F %d L %d Hit %d PU %d\n", 
        pSubCmd->pHCMD->ucCmdTag, pSubCmd->SubCmdAddInfo.ulSubCmdLBA, pSubCmd->pHCMD->ucCmdRW, 
        pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN, pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN,
        pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN, pSubCmd->SubCmdAddInfo.ucSubLPNCountIN,
        pSubCmd->SubCmdFirst, pSubCmd->SubCmdLast, pSubCmd->SubCmdHitResult, pSubCmd->SubCmdAddInfo.ucPuNum);
    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "SUBCMD Cacheline %d F-Hit %d OUT(SOff %d SCnt %d LOff %d LCnt %d) PRD %d usPhyBufID %x\n",
        pSubCmd->SubCmdAddInfo.usSubCmdCacheTAG, pSubCmd->SubCmdAddInfo.ucFullHitCache,
        pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT, pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT,
        pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT, pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT,
        pSubCmd->SubPRDId, pSubCmd->SubCmdPhyBufferID);

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,28,pSubCmd->pHCMD->ucCmdTag | (pSubCmd->pHCMD->ucCmdRW << 8),pSubCmd->SubCmdAddInfo.ulSubCmdLBA,
      pSubCmd->SubCmdAddInfo.ucSubCmdOffsetIN | (pSubCmd->SubCmdAddInfo.ucSubCmdlengthIN << 8) | 
      (pSubCmd->SubCmdAddInfo.ucSubLPNOffsetIN << 16) | (pSubCmd->SubCmdAddInfo.ucSubLPNCountIN << 24),
      pSubCmd->SubCmdFirst | (pSubCmd->SubCmdLast << 8) | (pSubCmd->SubCmdHitResult << 16) | (pSubCmd->SubCmdAddInfo.ucPuNum << 24));
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,28,"SUBCMD TAG %d[23:16] LBA %x[63:32] RW %d[31:24] IN(SOff %d[71:64] SCnt %d[79:72] LOff %d[87:80] LCnt %d[95:88]) F %d[103:96] L %d[111:104] HitResult %d[119:112] PU %d[127:120]\n");

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,29,pSubCmd->SubCmdAddInfo.usSubCmdCacheTAG | (pSubCmd->SubCmdAddInfo.ucFullHitCache << 8),
      pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT | (pSubCmd->SubCmdAddInfo.ucSubCmdlengthOUT << 8) | 
      (pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT << 16) | (pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT << 24),
      pSubCmd->SubPRDId,pSubCmd->SubCmdPhyBufferID);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,29,"SUBCMD Cacheline %d[23:16] FullHitCache %d[31:24] OUT(SOff %d[39:32] SCnt %d[47:40] LOff %d[55:48] LCnt %d[63:56]) PRD %d[71:64] usPhyBufID %x[111:96]\n");
*/
    return;
}

void DRAM_ATTR L1_DbgSubCmdShow()
{
/*    
    U8  ucLoop;
    SUBCMD *pSubCmd;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,56,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,56,"L1_DbgSubCmdShow Enter\n");

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "CURRENT pSubCmd 0x%x SubCmdSlot %d\n", (U32)gpCurSubCmd, gCurSubCmd);
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,30,gCurSubCmd,(U32)gpCurSubCmd,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,30,"CURRENT pSubCmd %x[63:32] SubCmdSlot %d[31:16]\n");
    L1_DbgPrintSubCmd(gpCurSubCmd);

    for (ucLoop = 0; ucLoop < SUBCMD_ENTRY_DEPTH; ucLoop++)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "\nSubCmdEntry Index %d:\n", ucLoop);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,31,ucLoop,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,31,"\nSubCmdEntry Index %d[23:16] Details:\n");
        pSubCmd = &gSubCmdEntry[ucLoop];
        L1_DbgPrintSubCmd(pSubCmd);
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,57,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,57,"L1_DbgSubCmdShow Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgPrintBufReq(BUF_REQ *pBufReq)
{
/*
    U8 ucLoop;

    if (pBufReq == NULL)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgPrintBufReq pBufReq NULL\n");
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,32,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,32,"L1_DbgPrintBufReq pBufReq NULL\n");
        return;
    }

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFREQ TAG %d RW %d Bmap 0x%x BufId 0x%x (SOff %d SCnt %d LOff %d LCnt %d)\n", pBufReq->Tag, pBufReq->ReqType,
        pBufReq->LPNBitMap, pBufReq->PhyBufferID, pBufReq->ReqOffset, pBufReq->ReqLength, pBufReq->LPNOffset, pBufReq->LPNCount);
    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFREQ 1st-Read %d RF-Hit %d PRD %d Seq %d L %d LStatus %d LAddr 0x%x\n", 
        pBufReq->SubCmdIDFirstRCMDEn, pBufReq->ReadFullHitWrite, pBufReq->PrdID, pBufReq->ReqSequenceEn,
        pBufReq->ReqLocalREQFlag, pBufReq->ReqLocalREQStatusEnable, pBufReq->ReqLocalREQStatusAddr);

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,33,pBufReq->Tag | (pBufReq->ReqType << 8),pBufReq->LPNBitMap,pBufReq->PhyBufferID,
      pBufReq->ReqOffset | (pBufReq->ReqLength << 8) | (pBufReq->LPNOffset << 16) | (pBufReq->LPNCount << 24));
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,33,"BUFREQ TAG %d[23:16] RW %d[31:24] Bmap %x[63:32] usPhyBufID %x[95:64] (SOff %d[103:96] SCnt %d[111:104] LOff %d[119:112] LCnt %d[127:120])\n");

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,34,pBufReq->ReadFullHitWrite | (pBufReq->PrdID << 8),
      pBufReq->SubCmdIDFirstRCMDEn | (pBufReq->ReqSequenceEn << 8) | (pBufReq->ReqLocalREQFlag << 16) | (pBufReq->ReqLocalREQStatusEnable << 24),
      pBufReq->ReqLocalREQStatusAddr,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,34,"BUFREQ 1st-Read %d[39:32] RF-Hit %d[23:16] PRD %d[31:24] Seq %d[47:40] L %d[55:48] LStatus %d[63:56] LAddr %x[96:64]\n");


    for (ucLoop = 0; ucLoop < LPN_PER_BUF; ucLoop++)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BUFREQ LPNIndex %d LPN 0x%x\n", ucLoop, pBufReq->LPN[ucLoop]);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,35,ucLoop,pBufReq->LPN[ucLoop],0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,35,"BUFREQ LPNIndex %d[23:16] LPN %x[63:32]\n");
    }
*/
    return;
}

void DRAM_ATTR L1_DbgBufReqShow()
{
/*
    U8  ucLoop;
    BUF_REQ *pBufReq;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,58,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,58,"L1_DbgBufReqShow Enter\n");

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "BufReq CURRENT Popped Fifo %d (0:NONE/1:HIGH/2:LOW)\n", 
        g_ucCurrPoppedFifo);
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,36,g_ucCurrPoppedFifo,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,36,"BufReq CURRENT Popped Fifo %d[31:16] (0:NONE/1:HIGH/2:LOW) \n");

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "++++++++++++++++++++  HighPrioEntry Tail %d Head %d  +++++++++++++++++ \n", 
        HighPrioFifoTail, HighPrioFifoHead); 
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,37,HighPrioFifoTail,HighPrioFifoHead,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,37,"\nHighPrioEntry Tail %d[31:16] Head %d[63:32]:\n");

    for (ucLoop = 0; ucLoop < PRIO_FIFO_DEPTH; ucLoop++)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "++++++++++++++++++++  HighPrioEntry Index %d  +++++++++++++++++ \n", ucLoop);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,38,ucLoop,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,38,"\nHighPrioEntry Index %d[31:16] Details:\n");
        pBufReq = &HighPrioEntry[ucLoop];
        L1_DbgPrintBufReq(pBufReq);
    }

    FIRMWARE_L1_LogInfo(LOG_ALL, 0, "++++++++++++++++++++  LowPrioEntry Tail %d Head %d  +++++++++++++++++ \n", 
        LowPrioFifoTail, LowPrioFifoHead); 
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,39,LowPrioFifoTail,LowPrioFifoHead,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,39,"\nLowPrioEntry Tail %d[31:16] Head %d[63:32]:\n");

    for (ucLoop = 0; ucLoop < PRIO_FIFO_DEPTH; ucLoop++)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "++++++++++++++++++++  LowPrioEntry Index %d  +++++++++++++++++ \n", ucLoop);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,40,ucLoop,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,40,"\nLowPrioEntry Index %d[31:16] Details:\n");
        pBufReq = &LowPrioEntry[ucLoop];
        L1_DbgPrintBufReq(pBufReq);
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,59,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,59,"L1_DbgBufReqShow Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgPrintSataPrd(SATA_PRD_ENTRY  *pSataPRD)
{
/*    
    if (pSataPRD == NULL)
    {
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,60,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,60,"L1_DbgPrintSataPrd pSataPRD NULL\n");
        return;
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,61, pSataPRD->ucTag | (pSataPRD->ucBufMapID << 8), 
      pSataPRD->usBufID[0] | (pSataPRD->ucSecCnt << 16) | (pSataPRD->ucBuffOffset << 24),
      pSataPRD->uResvDW[1],0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,61,"SATAPRD TAG %d[23:16] BufMapID %u[31:24] usPhyBufID %x[47:32] SecCnt %d[55:48] Offset %u[63:56] SubCmdLBA %x[95:64]\n");

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,62, 0, 0,
      pSataPRD->usTransCnt | (pSataPRD->ucSplitSize << 16) | (pSataPRD->bDMAEnable <<24),
      pSataPRD->bUpdateCacheStatusEn | (pSataPRD->bValid << 8) | (pSataPRD->bEOT << 16) | (pSataPRD->bWrite << 24));
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,62,"SATAPRD TransCnt %u[79:64] Split %u[87:80] DMA %u[95:88] CacheStatus %u[103:96] Valid %u[111:104] EOT %u[119:112] Write %u[127:120]\n");
*/
    return;
}

void DRAM_ATTR L1_DbgSataShow(void)
{
/*
    U8 ucLoop;
    volatile SATA_PRD_ENTRY * pSataPRD;

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,63,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,63,"L1_DbgSataShow Enter\n");

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,67,rSDMAC_Status,rSDMAC_CurBufMapId,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,67,"SATA DMA Status: Busy %u[23:16] DMA Current Read BufMapID %u[39:32]\n\n");

    pSataPRD = (volatile SATA_PRD_ENTRY *)(SACMDM_BASE_ADDRESS + 0x10);
    
    WRITE_DBG_LOG_INFO(g_DbgLogInfo,68, pSataPRD->usTransCnt, 
      pSataPRD->ucBufMapID | (pSataPRD->bWrite << 8) | (pSataPRD->ucTag << 16) | (pSataPRD->bDMAEnable << 24),0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,68,"Outstanding SATA Prd DW1: TransCnt %u[31:16] BufMapID %u[39:32] Write %d[47:40] TAG %u[55:48] DMA %u[63:56]\n");

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,69, pSataPRD->ucSplitSize | (pSataPRD->bUpdateCacheStatusEn << 8), 
      pSataPRD->ucSecCnt | (pSataPRD->ucBuffOffset << 8) | (pSataPRD->bValid << 16) | (pSataPRD->bEOT << 24),0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,69,"Outstanding SATA Prd DW2: Split %u[23:16] CacheStatus %u[31:24] SecCnt %d[39:32] Offset %u[47:40] Valid %u[55:48] EOT %u[63:56]\n\n");


    for (ucLoop = 0; ucLoop < RPRD_NUM; ucLoop++)
    {
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,65,ucLoop,&g_pSataReadPRD[ucLoop],HAL_GetBufMapValue(ucLoop),0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,65,"\nSATA ReadPrdID %u[31:16] addr %x[63:32] BufMapValue %x[95:64] Details:\n");
        L1_DbgPrintSataPrd((SATA_PRD_ENTRY *)&g_pSataReadPRD[ucLoop]);
    }

    for (ucLoop = 0; ucLoop < WPRD_NUM; ucLoop++)
    {
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,66,ucLoop,&g_pSataWritePRD[ucLoop],0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,66,"\nSATA WritePrdID %u[31:16] addr %x[63:32] Details:\n");
        L1_DbgPrintSataPrd((SATA_PRD_ENTRY *)&g_pSataWritePRD[ucLoop]);
    }

    WRITE_DBG_LOG_INFO(g_DbgLogInfo,64,0,0,0,0);
    FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
    RecordFlag(0,1,64,"L1_DbgSataShow Done\n\n");
*/
    return;
}

void DRAM_ATTR L1_DbgShowAll()
{
    DBG_Printf("L1_DbgShowAll\n");

    /* HCMD Status */
    L1_DbgHCMDShow();

    /* All Buffer Status */
    L1_DbgBufferShowALL();

    /* PU Fifo Status */
    L1_DbgPuFifoShow();

    /* CacheLine Status */
    L1_DbgCacheLineShowALL();

    /* SubCmd Status */
    L1_DbgSubCmdShow();

    /* Sata Status */
    L1_DbgSataShow();

    /* BufReq Status */
    L1_DbgBufReqShow();

    return;
}

//give a LBA address, if it is in L1 Cache, print the related Buffer and Cache infomation
void DRAM_ATTR L1_DbgLBAShow(U32 ulLBA)
{
/*    
//    U8  ucLoop;
    U8  ucPuNum;
    U8  ucSubCmdOffset;
    U8  ucLPNOffset;
    U16 usCacheLine;
//    U16 usLocBufID;
    U16 usPhyBufID;
    U32 ulSubCmdLBA;
    U32 ulStartLpn;
    //BUFF_TAG *pBufTag;

    if (ulLBA > VIRTUAL_STORAGE_MAX_LBA)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBAShow ulLBA ERROR\n");
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,41,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,41,"L1_DbgLBAShow ulLBA ERROR\n");
        return;
    }

    ulSubCmdLBA = HCMDLBA_SUBCMDLBA(ulLBA);
    usCacheLine = L1_GetCacheLine(ulSubCmdLBA);
    ucPuNum     = (usCacheLine % PU_NUM);
    ulStartLpn  = ulSubCmdLBA >> LPN_SECTOR_BIT;

    ucSubCmdOffset = SUBCMD_OFFSET(ulSubCmdLBA);
    ucLPNOffset = SUBCMDLPN_OFFSET(ucSubCmdOffset);


    usPhyBufID = L1_BufferSearchLPN(ucPuNum, (ulStartLpn + ucLPNOffset), L1_BUFF_SEARCHMASK_SINGLE_LPN);

    if (usPhyBufID == INVALID_4F)
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBAShow LBA 0x%x LPN 0x%x PU %d CacheLine %d, Not In L1 Buffer!\n", 
          ulLBA, (ulStartLpn + ucLPNOffset), ucPuNum, usCacheLine);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,42,usCacheLine,ulLBA,(ulStartLpn + ucLPNOffset),ucPuNum);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,42,"L1_DbgLBAShow LBA %x[63:32] LPN %x[95:64] PU %d[103:96] CacheLine %d[31:16], Not In L1 Buffer!\n");
    }
    else
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBAShow LBA 0x%x LPN 0x%x PU %d CacheLine %d, In L1 Buffer 0x%x!\n", 
          ulLBA, (ulStartLpn + ucLPNOffset), ucPuNum, usCacheLine, usPhyBufID);
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,43,usCacheLine,ulLBA,(ulStartLpn + ucLPNOffset),ucPuNum | (usPhyBufID << 8));
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,43,"L1_DbgLBAShow LBA %x[63:32] LPN %x[95:64] PU %d[103:96] CacheLine %d[31:16], usPhyBufID %x[119:104]!\n");

        L1_DbgBufferShow(usPhyBufID);
        L1_DbgPrintCacheLine(usCacheLine);
    }

*/
    return;
}

//merge read sectors start with given LBA, only read the LPN with the given LBA
//this function wont check whether the LBA is in L1 Cache or not, it just send a merge read request and wait it finished
BOOL DRAM_ATTR L1_DbgLBARead(U32 ulLBA)
{
/*    
    U8  ucLoop;
    U8  ucSubCmdOffset;
    U8  ucLPNOffset;
    U32 ulSubCmdLBA;
//    U32 ulTempAddr;
    BUF_REQ* pBufReq;

    if (ulLBA > VIRTUAL_STORAGE_MAX_LBA) //MaxLBAInDisk
    {
        FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBARead ulLBA ERROR\n");
        WRITE_DBG_LOG_INFO(g_DbgLogInfo,44,0,0,0,0);
        FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
        RecordFlag(0,1,44,"L1_DbgLBARead ulLBA ERROR\n");
        return TRUE;
    }

    switch (ucL1DbgState)
    {
    case 0: 
        {
            if (L1_HighPrioFifoFull() == TRUE)
            {
                // waiting for the BufReq Fifo have space 
                return FALSE;
            }
            else
            {
                gTmpLPN = INVALID_8F;
                gTmpFlashStatus = HAL_FLASH_REQ_FLASHSTATUS_SUCCESS;
                ucL1DbgState = 1;

                FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBARead PhyBufID %x, StartAddr 0x%x\n", 
                    g_L1TempBufferPhyId, g_L1TempBufferAddr);
                WRITE_DBG_LOG_INFO(g_DbgLogInfo,45,0,g_L1TempBufferPhyId,g_L1TempBufferAddr,0);
                FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
                RecordFlag(0,1,45,"L1_DbgLBARead usPhyBufID %x[63:32], StartAddr %x[95:64]\n");
            }
        }

        // BufReq Fifo have space, we dont break but send a merge read request 
    case 1:
        {
            ulSubCmdLBA = HCMDLBA_SUBCMDLBA(ulLBA);
            ucSubCmdOffset = SUBCMD_OFFSET(ulLBA);
            ucLPNOffset = SUBCMDLPN_OFFSET(ucSubCmdOffset);
            gTmpLPN = (ulSubCmdLBA >> LPN_SECTOR_BIT) + ucLPNOffset;        

            pBufReq = L1_HighPrioFifoGetBufReq();
        
            for(ucLoop = 0; ucLoop < BUF_REQ_SIZE; ucLoop++)
            {
                *((U32*)pBufReq + ucLoop) = 0;
            }

            pBufReq->ReqType = HCMD_READ;
            pBufReq->PhyBufferID = g_L1TempBufferPhyId;

            pBufReq->LPNOffset = 0;
            pBufReq->LPNCount = 1;
            pBufReq->LPNBitMap = (U32)INVALID_2F;
            pBufReq->LPN[0] = gTmpLPN;
            pBufReq->ReqLocalREQFlag = TRUE;
            pBufReq->ReqLocalREQStatusEnable = TRUE;

            gTmpFlashStatus = HAL_FLASH_REQ_FLASHSTATUS_PENDING;
            pBufReq->ReqLocalREQStatusAddr = (U32)(&gTmpFlashStatus);

            // send the BufReq to L2 
            L1_HighPrioMoveTailPtr();

            FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBARead Send MergeRead for LBA 0x%x\n", ulLBA);
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,46,0,ulLBA,0,0);
            FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,46,"L1_DbgLBARead Send MergeRead for LBA %x[63:32]\n");
            L1_DbgPrintBufReq(pBufReq);

            ucL1DbgState = 2;
            return FALSE;
        }
        break;

        // waiting for FlashStatus IDLE 
    case 2: 
        {
            if (gTmpFlashStatus == HAL_FLASH_REQ_FLASHSTATUS_PENDING)
            {
                return FALSE;
            }

            FIRMWARE_L1_LogInfo(LOG_ALL, 0, "L1_DbgLBARead LBA 0x%x read Finished! Detail: \n", ulLBA);
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,47,0,ulLBA,0,0);
            FW_L1_LogInfo_Trace(LOG_TRACE_ALL,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,47,"L1_DbgLBARead LBA %x[63:32] read Finished!\n");

            ucL1DbgState = 0;
            return TRUE;
        }
    }
*/
    return TRUE;
}

#ifdef HOST_PATTERN_RECORD
void DRAM_ATTR L1_DbgPrintPatternUart(void)
{
    switch (gCurHCMD->ucCmdCode)
    {
        case ATA_CMD_READ_DMA:
        case ATA_CMD_READ_DMA_EXT:
        case ATA_CMD_READ_FPDMA_QUEUED:
        case ATA_CMD_READ_SECTOR:
        case ATA_CMD_READ_MULTIPLE:
        case ATA_CMD_READ_SECTOR_EXT:
        case ATA_CMD_READ_MULTIPLE_EXT:
        {
            DBG_Printf("%d Code 0x%x READ 0x%x %d\n",
              ulPatternIndex, gCurHCMD->ucCmdCode, gCurHCMD->ulCmdLba, gCurHCMD->ulCmdSectorCnt);
            ulPatternIndex++;
        }
        break;

        case ATA_CMD_WRITE_DMA:
        case ATA_CMD_WRITE_DMA_EXT:
        case ATA_CMD_WRITE_FPDMA_QUEUED:
        case ATA_CMD_WRITE_SECTOR:
        case ATA_CMD_WRITE_MULTIPLE:
        case ATA_CMD_WRITE_SECTOR_EXT:
        case ATA_CMD_WRITE_MULTIPLE_EXT:
        {
            DBG_Printf("%d Code 0x%x WRITE 0x%x %d\n",
              ulPatternIndex, gCurHCMD->ucCmdCode, gCurHCMD->ulCmdLba, gCurHCMD->ulCmdSectorCnt);
            ulPatternIndex++;
        }
        break;
    
        default:
        {
            DBG_Printf("Non-Data pattern CmdCode 0x%x\n", gCurHCMD->ucCmdCode);
        }
        break;
    }        

    return;
}

void DRAM_ATTR L1_DbgSavePatternMemory(void)
{
    switch (gCurHCMD->ucCmdCode)
    {
        case ATA_CMD_READ_DMA:
        case ATA_CMD_READ_DMA_EXT:
        case ATA_CMD_READ_FPDMA_QUEUED:
        case ATA_CMD_READ_SECTOR:
        case ATA_CMD_READ_MULTIPLE:
        case ATA_CMD_READ_SECTOR_EXT:
        case ATA_CMD_READ_MULTIPLE_EXT:
        {
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,200,gCurHCMD->ucCmdCode,ulPatternIndex,gCurHCMD->ulCmdLba,gCurHCMD->ulCmdSectorCnt);
            FW_L1_LogInfo_Trace(LOG_TRACE_MEMORY,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,200,"%u[63:32] Code %x[31:16] READ %x[95:64] %d[127:96]\n");
            ulPatternIndex++;
        }
        break;

        case ATA_CMD_WRITE_DMA:
        case ATA_CMD_WRITE_DMA_EXT:
        case ATA_CMD_WRITE_FPDMA_QUEUED:
        case ATA_CMD_WRITE_SECTOR:
        case ATA_CMD_WRITE_MULTIPLE:
        case ATA_CMD_WRITE_SECTOR_EXT:
        case ATA_CMD_WRITE_MULTIPLE_EXT:
        {
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,201,gCurHCMD->ucCmdCode,ulPatternIndex,gCurHCMD->ulCmdLba,gCurHCMD->ulCmdSectorCnt);
            FW_L1_LogInfo_Trace(LOG_TRACE_MEMORY,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,201,"%u[63:32] Code %x[31:16] WRITE %x[95:64] %d[127:96]\n");
            ulPatternIndex++;
        }
        break;

        default:
        {
            WRITE_DBG_LOG_INFO(g_DbgLogInfo,202,gCurHCMD->ucCmdCode,0,0,0);
            FW_L1_LogInfo_Trace(LOG_TRACE_MEMORY,1,LOG_LVL_TRACE,&g_DbgLogInfo);
            RecordFlag(0,1,202,"Non-Data pattern CmdCode %x[31:16]\n");
        }
        break;
    }        

    return;
}

void DRAM_ATTR L1_DbgPrintPattern(void)
{
    if (bSendMemory != FALSE)
    {
        g_ulDbgEvent = 0x115;
        bSendMemory = FALSE;
    }

    if (gCurHCMD == NULL)
    {
        return;
    }

    if (bPrintUart != FALSE)
    {
        L1_DbgPrintPatternUart();
    }

    if (bSaveMemory != FALSE)
    {
        L1_DbgSavePatternMemory();
    }

    return;
}
#endif

/****************************************************************************
Name        :L1_DbgEventHandler
Input       :void DRAM_ATTR
Output      :U32
Author      :Blakezhang
Date        :2012.12.07
Description :L1 debug event handler.
Others      :
Modify      :
****************************************************************************/
U32 DRAM_ATTR L1_DbgEventHandler(void)
{
    COMM_EVENT_PARAMETER * pParameter;
    U32 ulDbgCode;
    U32 ulPara1;
    U32 ulPara2;
    U32 ulPara3;

    CommGetEventParameter(COMM_EVENT_OWNER_L1,&pParameter);
    ulDbgCode = pParameter->EventParameterNormal[0];
    ulPara1 = pParameter->EventParameterNormal[1];
    ulPara2 = pParameter->EventParameterNormal[2];
    ulPara3 = pParameter->EventParameterNormal[3];

    switch(ulDbgCode)
    {
        case COMM_DEBUG_REPORT_STATUS:
            L1_DbgShowAll();
            break;

        case COMM_DEBUG_DISPLAY:
            L1_DbgShowAll();
            break;

        case L1_DEBUG_LBA_SHOW:
            L1_DbgLBAShow(ulPara1);
            break;

        case L1_DEBUG_CACHELINE_SHOW:
            L1_DbgCacheLineShowALL();
            break;

        case L1_DEBUG_BUFFER_SHOW_SINGLE:
            L1_DbgBufferShow((U16)ulPara1);
            break;

        case L1_DEBUG_BUFFER_SHOW_ALL:
            L1_DbgBufferShowALL();
            break;

        case L1_DEBUG_LBA_READ:
            if (L1_DbgLBARead(ulPara1) == FALSE)
            {
                return FAIL;
            }
            break;

        case L1_DEBUG_PRINT_PATTERN:
#ifdef HOST_PATTERN_RECORD
            if (Send_TraceMemoryLog() == FALSE)
            {
                return FAIL;
            }
#endif
            break;

        default:
            break;
    }

    /* clear debug parameters */
    pParameter->EventParameterNormal[0] = 0;
    pParameter->EventParameterNormal[1] = 0;
    pParameter->EventParameterNormal[2] = 0;
    pParameter->EventParameterNormal[3] = 0;

    return SUCCESS;
}



#define DP(str)     DBG_Printf("unit test hanging at %s, line %d (%s)\n", __FILE__, __LINE__, (str))

void Dbg_TestDSG(void)
{}

void Dbg_SetFISDelay(U16 usDelay)
{
    rSDC_FISDelayControl = (rSDC_FISDelayControl & (~0xFFFF)) | usDelay;
}

void Dbg_AutoSetFISDelay(void)
{
    /* --> FDLL -++-> FDLH --> FDHL -++-> FDHH --> FDLL --> ... */
    static U16 s_usFisDelay = FISDELAY_LOW_LOW;

    Dbg_SetFISDelay(s_usFisDelay);
    
    if (FISDELAY_LOW_HIGH == s_usFisDelay)
    {
        s_usFisDelay = FISDELAY_HIGH_LOW;
    }
    else if (FISDELAY_HIGH_HIGH <= s_usFisDelay)
    {
        s_usFisDelay = FISDELAY_LOW_LOW;
    }
    else
    {
        s_usFisDelay++;
    }
}

#endif 

