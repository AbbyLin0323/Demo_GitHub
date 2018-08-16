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
Filename    :L2_GCManager.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.1
Description :functions about GCManager
1) L2_InitGCManager(): init the value to boot up status
Others      :
Modify      :
****************************************************************************/
#include "L1_Debug.h"
#include "COM_BitMask.h"
#include "L2_GCManager.h"
#include "L2_StripeInfo.h"
#include "L2_Debug.h"
#include "l2_Thread.h"
#include "L2_FTL.h"
#include "L2_Schedule.h"
#include "L2_ErrorHandling.h"
#include "L2_PMTPage.h"
#include "FW_Event.h"
#include "COM_Memory.h"
#include "HAL_TraceLog.h"
#include "L2_StaticWL.h"
#include "L2_PMTManager.h"
#include "L2_Boot.h"
#include "L2_StripeInfo.h"
#include "L2_Interface.h"
#include "L2_TLCMerge.h"
#include "L2_DWA.h"
#include "L2_Erase.h"
#include "L2_FCMDQ.h"
#include "L2_TableBBT.h"

#if  (defined(L2MEASURE)||defined(SWL_EVALUATOR))
#include "L2_Evaluater.h"
#endif

#ifndef SIM
#include <xtensa/tie/xt_timer.h>
#endif

//specify file name for Trace Log
#define TL_FILE_NUM  L2_GCManager_c

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/
extern GLOBAL  LC lc; // Added by jasonguo 20140611

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/
extern void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bsTableReq, BOOL bSLCMode);
#ifdef L2_HANDLE_UECC
extern void L2_AddUECCBlock(U8 ucSuperPu, U8 ucLun, U16 usBlk);
extern BOOL L2_NeedHandleUECC(U8 ucSuperPu);
extern void L2_HandleUECC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap);
#endif

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/
U32 LookupRPMTInGC(U8 ucSuperPu, PhysicalAddr* pAddr, U16 ucRPMTNum, BOOL bTLCWrite, GCComStruct *ptCom);
/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/
GLOBAL  BOOL gbGCTraceRec;

extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;

extern GLOBAL FTL_IDLE_GC_MGR g_tFTLIdleGCMgr;

GLOBAL TLCGCSrcBlkRecord *g_pTLCGCSrcBlkRecord;

LOCAL MCU12_VAR_ATTR U32 l_aGCBufferPointer[SUBSYSTEM_SUPERPU_MAX] = { 0 };

void MCU1_DRAM_TEXT L2_InitDirtyLpnMap(U8 ucSuperPu)
{
    COM_MemSet((U32 *)(&pDPBM->m_LpnMap[ucSuperPu][0]), (sizeof(DPBM_ENTRY) * VIR_BLK_CNT / sizeof(U32)), 0);
}

void  L2_UpdateDirtyLpnMap(PhysicalAddr* pAddr, BOOL bValid)
{
    U8 LPNInPage = pAddr->m_LPNInPage;
    U8 ucPrefix = 0;

#if (LPN_PER_BUF == 8)
    U8* pLpnMap_Page = (U8*)&pDPBM->m_LpnMap[pAddr->m_PUSer][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];
#elif (LPN_PER_BUF == 16)
    U16* pLpnMap_Page = (U16*)&pDPBM->m_LpnMap[pAddr->m_PUSer][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];
#else
    U8* pLpnMap_Page = (U8*)&pDPBM->m_LpnMap[pAddr->m_PUSer][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];
#endif

    if (((*pLpnMap_Page) & (0x1 << LPNInPage)) != bValid)
    {
        if (bValid)
        {
            *pLpnMap_Page |= (0x1 << LPNInPage);
        }
        else
        {
            *pLpnMap_Page &= ~(0x1 << LPNInPage);
        }
    }
    else
    {
        DBG_Printf("SuperPu 0x%x Blk 0x%x Pg 0x%x OffsetInSuperPage %d LpnInPg %d PPN 0x%x LPNMap 0x%x\n", pAddr->m_PUSer, pAddr->m_BlockInPU, pAddr->m_PageInBlock,
            pAddr->m_OffsetInSuperPage, pAddr->m_LPNInPage, pAddr->m_PPN, *pLpnMap_Page);
        DBG_Getch();
    }
}
BOOL  L2_LookupDirtyLpnMap(PhysicalAddr* pAddr)
{
    U8 LPNInPage;

#if (LPN_PER_BUF == 8)
    U8* pLpnMap_Page = (U8*)&pDPBM->m_LpnMap[pAddr->m_PUSer][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];
#elif  (LPN_PER_BUF == 16)
    U16* pLpnMap_Page = (U16*)&pDPBM->m_LpnMap[pAddr->m_PUSer][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];
#else
    U8* pLpnMap_Page = (U8*)&pDPBM->m_LpnMap[pAddr->m_PUSer][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];
#endif

    LPNInPage = pAddr->m_LPNInPage;
    return (((*pLpnMap_Page) & (0x1 << LPNInPage)) ? TRUE : FALSE);
}

void L2_InitCommonInfo(U8 ucSuperPu, GCComStruct *ptCom)
{
    U16 i, j, k;

    /* common information */
    ptCom->m_ucSrcBlkType[ucSuperPu] = VBT_TYPE_INVALID;
    ptCom->m_ucRPMTNum[ucSuperPu] = 0;
    ptCom->m_GCStage[ucSuperPu] = GC_STATE_PREPARE_GC;
    ptCom->m_GCReadOffset[ucSuperPu] = 0;
    ptCom->m_GCUpOfExtraReadOffset[ucSuperPu] = 0;
    ptCom->m_LoadRPMTBitMap[ucSuperPu] = 0;
    ptCom->m_GCBufferPointer[ucSuperPu] = 0; // Which buffer is to be written.

    ptCom->m_FinishBitMap[ucSuperPu] = 0;
    ptCom->m_ErrLun[ucSuperPu] = INVALID_2F;
    ptCom->m_FailLunBitMap[ucSuperPu] = 0;
    ptCom->m_ErrStage[ucSuperPu] = TLC_ERRH_ALL;
    ptCom->m_GoodLunFinish[ucSuperPu] = FALSE;

    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        ptCom->m_GCWriteStatus[ucSuperPu][i] = SUBSYSTEM_STATUS_SUCCESS;

        for (j = 0; j < PG_PER_WL; j++)
        {
            ptCom->m_RPMTStatus[ucSuperPu][i][j] = SUBSYSTEM_STATUS_SUCCESS;
        }
    }

    for (j = 0; j < LPN_PER_SUPERBUF; j++)
    {
        for (i = 0; i < GC_RPMT_RECORD_DEPTH; i++)
        {
             ptCom->m_LPNInBuffer[ucSuperPu][i][j] = INVALID_8F;
        }

        for (k = 0; k < (PG_PER_WL+1); k++)
        {
            ptCom->m_FlushStatus[ucSuperPu][k][j] = SUBSYSTEM_STATUS_SUCCESS;
        }
        ptCom->m_AddrForBuffer[ucSuperPu][0][j].m_PPN = INVALID_8F;
        ptCom->m_AddrForBuffer[ucSuperPu][1][j].m_PPN = INVALID_8F;
    }
}

//////////////////////////////////////////////////////////////////////////
//void L2_InitGCManager
//function:
//    initial the values to boot up status
//    the DRAM of m_pRPMT has been allocated before enter L2_InitGCManager()
//    the DRAM of m_GCBuffer hass been allocated too.
//////////////////////////////////////////////////////////////////////////
void L2_InitGCManager(U8 ucSuperPu, U8 ucGCMode)
{
    /* Init Common information */
    g_GCManager[ucGCMode]->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
    g_GCManager[ucGCMode]->tGCCommon.m_SrcWLO[ucSuperPu] = INVALID_4F;
    g_GCManager[ucGCMode]->tGCCommon.m_SrcPrefix[ucSuperPu] = INVALID_2F;
    g_GCManager[ucGCMode]->tGCCommon.m_SrcOffset[ucSuperPu] = INVALID_4F;
    g_GCManager[ucGCMode]->tGCCommon.m_TLCGCDummyWrite[ucSuperPu] = FALSE;
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    g_GCManager[ucGCMode]->tGCCommon.m_TLCGCNeedtoWaitforRPMT[ucSuperPu] = FALSE;
#endif    
    L2_InitCommonInfo(ucSuperPu, &g_GCManager[ucGCMode]->tGCCommon);

    g_GCManager[ucGCMode]->m_NeedCopyForOneWrite[ucSuperPu] = 0;
    g_GCManager[ucGCMode]->m_HostWriteRealPage[ucSuperPu] = 0;
    g_GCManager[ucGCMode]->m_DirtyLpnCnt[ucSuperPu] = 0;
    g_GCManager[ucGCMode]->m_FreePageCnt[ucSuperPu] = 0;
    g_GCManager[ucGCMode]->m_FreePageForHost[ucSuperPu] = 0;

    gbGCTraceRec = FALSE;
    return;
}

void MCU1_DRAM_TEXT L2_LLFGCManager(void)
{
    U8 ucSuperPU;
    U8 ucGCMode;

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        L2_InitDirtyLpnMap(ucSuperPU);
        for (ucGCMode = 0; ucGCMode < GC_SRC_MODE; ucGCMode++)
        {
            L2_InitGCManager(ucSuperPU, ucGCMode);
        }
    }
}
#ifndef SIM
void MCU1_DRAM_TEXT L2_PrintFreePage(U8 i)
{
    U32 FreePageCnt;

    FreePageCnt = L2_GetFreePageCntInPu(i);

    TRACE_LOG((void*)&i, sizeof(U8), U8, 0, "[L2_PrintFreePage]: PU ? "); 
    TRACE_LOG((void*)&FreePageCnt, sizeof(U32), U32, 0, "[L2_PrintFreePage]: FreePageCnt ? "); 

    return;
}

void MCU1_DRAM_TEXT L2_PrintDirtyCnt(U8 i,U16 BlockSN)
{
    U16 usDirtyCnt;

    usDirtyCnt = L2_GetDirtyCnt(i,BlockSN);
    TRACE_LOG((void*)&i, sizeof(U8), U8, 0, "[L2_PrintDirtyCnt]: PU ? "); 
    TRACE_LOG((void*)&BlockSN, sizeof(U16), U16, 0, "[L2_PrintDirtyCnt]: Block ? "); 
    TRACE_LOG((void*)&usDirtyCnt, sizeof(U16), U16, 0, "[L2_PrintDirtyCnt]: DirtyLPNCnt ? "); 

    return;
}

void MCU1_DRAM_TEXT L2_DbgGC(U16 FlagID,U8 i, U8 ucGCMode)
{
    U32 ulCycleCnt;
    U32 SystemState;

    ulCycleCnt = XT_RSR_CCOUNT();
    TRACE_LOG((void*)&ucGCMode, sizeof(U32), U32, 0, "ucModeIndex ? "); 
    switch(FlagID)
    {
    case 4:
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[CopyValidOneLine]: ulCycleCnt ? "); 
        TRACE_LOG((void*)&i, sizeof(U8), U8, 0, "PU ? "); 
        TRACE_LOG((void*)&g_GCManager[ucGCMode]->tGCCommon.m_SrcPBN[i], sizeof(U16), U16, 0, "PBN ? ");
        TRACE_LOG((void*)&g_GCManager[ucGCMode]->tGCCommon.m_SrcWLO[i], sizeof(U16), U16, 0, "WLO ? ");
        TRACE_LOG((void*)&g_GCManager[ucGCMode]->tGCCommon.m_SrcOffset[i], sizeof(U16), U16, 0, "Offset ? ");
        //TRACE_LOG((void*)&g_GCManager->tGCCommon.m_GCStage[i], sizeof(U32), U32, 0, "GC Stage ? ");
        break;

    case 8:
        SystemState = (U32)L2_GetCurrState(i);
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "[CopyValidOneLine]: ulCycleCnt ? "); 
        TRACE_LOG((void*)&SystemState, sizeof(U32), U32, 0, "System State ? (0:RW,1:GC) "); 
        break;

    case 10:
        TRACE_LOG((void*)&i, sizeof(U8), U8, 0, "[GCPageWrite] PU ? Flush PMT "); 
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "ulCycleCnt ? "); 
        break;

    case 11:
        TRACE_LOG((void*)&g_GCManager[0]->m_NeedCopyForOneWrite[i], sizeof(U16), U16, 0, "m_NeedCopyForOneWrite ? "); 
        TRACE_LOG((void*)&ulCycleCnt, sizeof(U32), U32, 0, "ulCycleCnt ? "); 
        break;

    default:
        break;                  
    }

    return;
}
#else
void MCU1_DRAM_TEXT L2_PrintFreePage(U8 i)
{
}
void MCU1_DRAM_TEXT L2_PrintDirtyCnt(U8 i, U16 BlockSN)
{
}
void MCU1_DRAM_TEXT L2_DbgGC(U16 FlagID, U8 i, U8 ucGCMode)
{
}
#endif

BOOL L2_IsReadLocationContinuous(U8 ucSuperPu, U8 ucBuffIndex, U16 Offset, GCComStruct *ptCom)
{
    U8 ucIndex;
    U8 ucOffset;

    ucIndex = Offset / LPN_PER_BUF;
    ucOffset = Offset % LPN_PER_BUF;

    if (0 == (ptCom->m_ReadStatusLocation[ucSuperPu][ucBuffIndex][ucIndex] & (1 << ucOffset)))
    {
        return TRUE;
    }

    return FALSE;
}

void L2_ReadLocationClear(U8 ucSuperPu, GCComStruct *ptCom)
{
    U8 ucBuffIndex;
    U8 ucBuffNumIndex;

    for (ucBuffIndex = 0; ucBuffIndex < (PG_PER_WL + 1); ucBuffIndex++)
    {
        for (ucBuffNumIndex = 0; ucBuffNumIndex < (LPN_PER_SUPERBUF / LPN_PER_BUF); ucBuffNumIndex++)
        {
            ptCom->m_ReadStatusLocation[ucSuperPu][ucBuffIndex][ucBuffNumIndex] = 0;
        }
    }
    ptCom->m_LastReadLen[ucSuperPu] = 0;

    return;
}

void L2_ReadLocationRecord(U8 ucSuperPu, U8 ucBuffIndex, U16 Offset, U8 Num, GCComStruct *ptCom)
{
    U8 ucIndex;
    U8 ucOffset;
    U8 ucPageType, ucProgPageCnt;
    U8 ucBufO;

    ucIndex = Offset / LPN_PER_BUF;
    ucOffset = Offset % LPN_PER_BUF;
    if (0 == (ptCom->m_ReadStatusLocation[ucSuperPu][ucBuffIndex][ucIndex] & (1 << ucOffset)))
    {
        ptCom->m_ReadStatusLocation[ucSuperPu][ucBuffIndex][ucIndex] |= (1 << ucOffset);
    }
    else
    {
        DBG_Getch();
    }

    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
    ucProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);
    ucBufO = g_TLCManager->ausTLCProgOrder[ucSuperPu] % TLC_BUF_CNT;

#ifndef FLASH_IM_3DTLC_GEN2
    if (ucPageType == L2_EXTRA_PAGE)
    {
        ucProgPageCnt = 1;
    }
#endif
            
    if ((((ucBufO + ucProgPageCnt - 1)% TLC_BUF_CNT) == ucBuffIndex) && ((Offset + Num) >= LPN_PER_SUPERBUF))
    {
        ptCom->m_LastReadLen[ucSuperPu] = Num;
    }

    return;
}

//////////////////////////////////////////////////////////////////////////
//void FillBuffer( U8 ucSuperPu, U16 Offset, PhysicalAddr* pAddr, U8 Num )
//                        
//function:
//    Load data of one LPN into buffer.
//    offset means the n-th LPN in one physical page to read.
//    
//history:
//    2015-01-27 Javenliu  1. add one "Num" parameter, which is the number of LPN
//                         2. delete the "LPN" parameter .
//
//////////////////////////////////////////////////////////////////////////
void FillBuffer(U8 ucSuperPu, U16 Offset, PhysicalAddr* pAddr, U8 Num, GCComStruct *ptCom)
{
    U8* pStatusAddr;
    BOOL bTLCMode = FALSE;
    U32 aBuffAddr[2] = {0};
    U8 ucBuffPointer;

    //U8 ucTLCWL;    
    U8 ucPageType;    
    
    U8 ucBufO;
    U8 ucRealBufNum; 

    if (pAddr->m_PUSer != ucSuperPu)
    {
        DBG_Printf("GC copy original SuperPu error! \n");
        DBG_Printf("SuperPu = %d, copy Num = %d LPNs from physical addr 0x%x!\n", ucSuperPu, Num, pAddr->m_PPN);
        DBG_Getch();
    }

    if (VBT_TYPE_HOST == ptCom->m_ucSrcBlkType[ucSuperPu])
    {
        pStatusAddr = &ptCom->m_FlushStatus[ucSuperPu][0][Offset];

        ucBuffPointer = ptCom->m_GCBufferPointer[ucSuperPu];
        aBuffAddr[0] = (U32)ptCom->m_GCBuffer[ucSuperPu][ucBuffPointer];
        aBuffAddr[1] = (U32)ptCom->m_GCBuffer[ucSuperPu][(ucBuffPointer + 1) % GC_BUFFER_DEPTH];
         
        //FIRMWARE_LogInfo("FillBuffer Pu %d Blk 0x%x Pg 0x%x GCBuffer 0x%x 0x%x FlushStatusOffset %d pStatusAddr 0x%x\n", 
        //    ucSuperPu, pAddr->m_BlockInPU, pAddr->m_PageInBlock, aBuffAddr[0], aBuffAddr[1], Offset, pStatusAddr);
    }
    else if (VBT_TYPE_TLCW == ptCom->m_ucSrcBlkType[ucSuperPu])
    {

        // get the page type we're about to program
        ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);

        // calculate the TLC buffer offset based on the page number
        ucBufO = g_TLCManager->ausTLCProgOrder[ucSuperPu] % TLC_BUF_CNT;

        ucRealBufNum = (ucBufO + g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu]) % TLC_BUF_CNT;

        bTLCMode = TRUE;

        aBuffAddr[0] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][ucRealBufNum][0];
        aBuffAddr[1] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][(ucRealBufNum + 1) % TLC_BUF_CNT][0];

#ifndef FLASH_IM_3DTLC_GEN2
        if ((ucPageType == L2_EXTRA_PAGE) && (g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] == 1))
        {
            aBuffAddr[0] = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][TLC_BUF_CNT][0];
            /* UpOfXtraPage, we should use buffer offset 3 */
            pStatusAddr = &ptCom->m_FlushStatus[ucSuperPu][TLC_BUF_CNT][Offset];
            L2_ReadLocationRecord(ucSuperPu, TLC_BUF_CNT, Offset, Num, ptCom);
        }
        else
#endif
        {
            /* For Low or UpOfLowPage or Xtra page use circle buffer offset 0, 1 and 2 */
            pStatusAddr = &ptCom->m_FlushStatus[ucSuperPu][ucRealBufNum][Offset];
            L2_ReadLocationRecord(ucSuperPu, ucRealBufNum, Offset, Num, ptCom);
        }
            
        //FIRMWARE_LogInfo("FillBuffer Pu %d Blk 0x%x Pg 0x%x buff0 0x%x buff1 0x%x offset %d num %d PageType %d TLCGCBufferCopyCnt %d ucRealBufNum %d\n", ucSuperPu, pAddr->m_BlockInPU, pAddr->m_PageInBlock, 
        //    aBuffAddr[0], aBuffAddr[1], Offset, Num, ucPageType, g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu], ucRealBufNum);
    }

    /* modified by henryluo for NF_DMA CDC issue, read redundant data must read first 1K data, so do not read red here */
    __L2_FtlReadLocal(aBuffAddr, pAddr, pStatusAddr, NULL, Num, Offset, FALSE, !(bTLCMode), FALSE);

    return;
}

//////////////////////////////////////////////////////////////////////////
//void FillDummyBuffer(U8 ucSuperPu, U16 Offset, U8 Num, GCComStruct *ptCom, U16 usCurTLCTargetPage)
//                        
//function:
//    Fill dummy data into buffer with zero.
//    Offset means the n-th LPN offset in the buffer to write to physical super page.
//    Num means the number of LPN to fill up.
//    
//    Caution : Still need to update L2_ReadLocationRecord, to prevent previous read status check over LPN range.
//
//history:
//    2017-04-11 DannierChen  1. add one "Num" parameter, which is the number of LPN
//
//////////////////////////////////////////////////////////////////////////
void FillDummyBuffer(U8 ucSuperPu, U16 Offset, U8 Num, GCComStruct *ptCom, U16 usCurTLCTargetPage)
{
    BOOL bTLCMode = FALSE;
    U32 aBuffAddr;

    U8 ucPageType;
    U8 ucBufO;
    U8 ucRealBufNum;

    /* get the page type we're about to program */
    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);

    /* calculate the TLC buffer offset based on the page number */
    ucBufO = g_TLCManager->ausTLCProgOrder[ucSuperPu] % TLC_BUF_CNT;

    ucRealBufNum = (ucBufO + g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu]) % TLC_BUF_CNT;

    aBuffAddr = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][ucRealBufNum][0];

#ifndef FLASH_IM_3DTLC_GEN2
    if ((ucPageType == L2_EXTRA_PAGE) && (g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] == 1))
    {
        aBuffAddr = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPu][TLC_BUF_CNT][0];
        /* UpOfXtraPage, we should we buffer offset 3 */
        L2_ReadLocationRecord(ucSuperPu, TLC_BUF_CNT, Offset, Num, ptCom);
    }
    else
#endif
    {
        /* For Low or UpOfLowPage or Xtra page use circle buffer offset 0, 1 and 2 */
        L2_ReadLocationRecord(ucSuperPu, ucRealBufNum, Offset, Num, ptCom);
    }

    COM_MemZero((U32*)(aBuffAddr + (Offset * LPN_SIZE)), (Num * LPN_SIZE) / sizeof(U32));
#ifdef SIM
    DBG_Printf("Fill dummy LPN 1st_Type=%d Page=%d LpnOfs=%d LpnCnt=%d\n", ucPageType, usCurTLCTargetPage, Offset, Num);
#endif
    return;
}

U32 L2_GetSLCAreaFreePageCnt(U8 ucSuperPu)
{
    PuInfo *pInfo;
    U32 ulCurFreePg;
    U32  ulCurFreeBlk = 0;

    pInfo = g_PuInfo[ucSuperPu];

    ulCurFreeBlk = pInfo->m_DataBlockCnt[BLKTYPE_SLC] - pInfo->m_AllocateBlockCnt[BLKTYPE_SLC];
    ulCurFreePg = (ulCurFreeBlk * PG_PER_SLC_BLK) + (PG_PER_SLC_BLK - pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);

    return ulCurFreePg;
}

U32 L2_GetSLCAreaFreeBLKCnt(U8 ucSuperPu)
{
	PuInfo *pInfo;

	pInfo = g_PuInfo[ucSuperPu];
	return (pInfo->m_DataBlockCnt[BLKTYPE_SLC] - pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]);
}

void SetSLCGCSrcBlock(U8 ucSuperPu, U16 BlockSN)
{
    U16 PhyBlk = 0;
    U16 usPgPerWL;
    U32 ulTotalLpnPerBlk;
    PuInfo *pInfo;
    U32 ulCurFreePg;
    GCManager * pGCManager;
    S32 ulSLCGCSrcBlkDirtyPage;
    U32 ulValidPageInPU;
    U32 ulHostWriteReal;

    pInfo = g_PuInfo[ucSuperPu];

    /* calc GC schedule ratio for FTL thread */
    /* MLC 2 MLC or SLC 2 SLC or TLC 2 SLC */
    pGCManager = g_GCManager[SLCGC_MODE];
    usPgPerWL = 1;
    ulTotalLpnPerBlk = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;

    pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] = BlockSN;
    pGCManager->tGCCommon.m_SrcWLO[ucSuperPu] = 0;
    pGCManager->tGCCommon.m_SrcPrefix[ucSuperPu] = 0;
    pGCManager->tGCCommon.m_SrcOffset[ucSuperPu] = 0;
    pGCManager->tGCCommon.m_ucRPMTNum[ucSuperPu] = 0;
    pGCManager->m_FreePageCnt[ucSuperPu] = (S32)L2_GetFreePageCntInPu(ucSuperPu) + L2_GetEraseQueueSize(ucSuperPu, TRUE) * PG_PER_SLC_BLK;;
    pGCManager->m_DirtyLpnCnt[ucSuperPu] = L2_GetDirtyCnt(ucSuperPu, BlockSN);

#ifdef SIM
    if ((pGCManager->m_DirtyLpnCnt[ucSuperPu] <= (U32)(usPgPerWL * LPN_PER_SUPERBUF)) && (FALSE == L2_GetBlkRetryFail(ucSuperPu, BlockSN)))  // modify for Blk from ForceGCQueue
    {
        DBG_Printf("PU:%d, BLK:%d, DC:%d\n", ucSuperPu, pGCManager->tGCCommon.m_SrcPBN[ucSuperPu], pGCManager->m_DirtyLpnCnt[ucSuperPu]);
        DBG_Getch();
    }
#endif

    if (ulTotalLpnPerBlk == L2_GetDirtyCnt(ucSuperPu, BlockSN))
    {
        pGCManager->m_NeedCopyForOneWrite[ucSuperPu] = 1;
    }
    else
    {
        ulCurFreePg = L2_GetSLCAreaFreePageCnt(ucSuperPu);
        ulValidPageInPU = ((ulTotalLpnPerBlk - pGCManager->m_DirtyLpnCnt[ucSuperPu]) / LPN_PER_SUPERBUF) + 1;
        pGCManager->m_FreePageForHost[ucSuperPu] = ulCurFreePg - RESERVED_PAGE_CNT;

        if (pGCManager->m_FreePageForHost[ucSuperPu] <= BOOT_STRIPE_CNT)
        {
            /* force not to respone host write untill GC done */
            //DBG_Printf("SuperPu %d m_FreePageForHost = %d !\n", ucSuperPu, g_GCManager->m_FreePageForHost[ucSuperPu]);
            pGCManager->m_NeedCopyForOneWrite[ucSuperPu] = (usPgPerWL * PG_PER_SLC_BLK);
            pGCManager->m_HostWriteRealPage[ucSuperPu] = 0;
        }
        else
        {
            /* Dynamic to decide GC schedule rate by free page and dirty count, modify by henryluo 2013-08-02 */    // modify for sim, this point need to discuss
            ulSLCGCSrcBlkDirtyPage = PG_PER_SLC_BLK - ulValidPageInPU;

            ulHostWriteReal = (U32)min((pGCManager->m_FreePageForHost[ucSuperPu] - BOOT_STRIPE_CNT), ulSLCGCSrcBlkDirtyPage);
            pGCManager->m_NeedCopyForOneWrite[ucSuperPu] = (ulValidPageInPU / ulHostWriteReal)+1;
            pGCManager->m_HostWriteRealPage[ucSuperPu] = ulHostWriteReal;
            if (pGCManager->m_NeedCopyForOneWrite[ucSuperPu] > (usPgPerWL * PG_PER_SLC_BLK))
            {
                pGCManager->m_NeedCopyForOneWrite[ucSuperPu] = (usPgPerWL * PG_PER_SLC_BLK);
            }
        }
    }

    /* lock GC src block */
    L2_PBIT_Set_Lock(ucSuperPu, BlockSN, TRUE);

#ifdef DBG_LC
    if (0 != lc.uLockCounter[ucSuperPu][DATA_GC])
    {
        DBG_Printf("GC Lock Pu %d  BlockSN 0x%x LockCnt %d\n", ucSuperPu, BlockSN, lc.uLockCounter[ucSuperPu][DATA_GC]);
        DBG_Getch();
    }
    lc.uLockCounter[ucSuperPu][DATA_GC]++;
    //FIRMWARE_LogInfo("SuperPu %d SetSLCGCSrcBlock ++\n", ucSuperPu);
#endif

#ifdef SIM
    if (0 == pGCManager->m_NeedCopyForOneWrite[ucSuperPu])
    {
        DBG_Printf("g_GCManager->m_DirtyLpnCnt[ucSuperPu]:%d, IsTLC:%d\n", pGCManager->m_DirtyLpnCnt[ucSuperPu],
            L2_VBT_Get_TLC(ucSuperPu, BlockSN));
        DBG_Getch();
    }
#endif

    return;
}


//////////////////////////////////////////////////////////////////////////
// LOCAL U8 l_aNumTableOfPostOnes[]
//
// This array stores the number of each binary integer's post ones.
// Help to support multiple LPN per GC copy, added by Javenliu, 2015-02-02
//
//////////////////////////////////////////////////////////////////////////
#if (LPN_PER_BUF == 8)
LOCAL MCU12_VAR_ATTR U8 l_aNumTableOfPostOnes[] =
{
    0x0, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0x00 - 0x0F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x4, 0x5,        // 0x10 - 0x1F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0x20 - 0x2F
    0x2, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x3, 0x1, 0x1, 0x2, 0x4, 0x1, 0x5, 0x6,        // 0x30 - 0x3F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0x40 - 0x4F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x4, 0x5,        // 0x50 - 0x5F
    0x2, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0x60 - 0x6F
    0x3, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x4, 0x1, 0x1, 0x2, 0x5, 0x1, 0x6, 0x7,        // 0x70 - 0x7F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0x80 - 0x8F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x4, 0x5,        // 0x90 - 0x9F
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0xA0 - 0xAF
    0x2, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x3, 0x1, 0x1, 0x2, 0x4, 0x1, 0x5, 0x6,        // 0xB0 - 0xBF
    0x2, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0xC0 - 0xCF
    0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x4, 0x5,        // 0xD0 - 0xDF
    0x3, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x1, 0x1, 0x1, 0x2, 0x2, 0x1, 0x3, 0x4,        // 0xE0 - 0xEF
    0x4, 0x1, 0x1, 0x2, 0x1, 0x1, 0x2, 0x3, 0x5, 0x1, 0x1, 0x2, 0x6, 0x1, 0x7, 0x8         // 0xF0 - 0xFF
};
#endif

U8 CalcPopOneNum(U16 usValue)
{
#if (LPN_PER_BUF == 8)

    return l_aNumTableOfPostOnes[(usValue & LPN_PER_BUF_BITMAP)];

#elif (LPN_PER_BUF == 16)
    U16 i;
    U16 Start = 0, End = 0;

    for (i = 0; i < LPN_PER_BUF; i++)
    {
        if (usValue & (1 << i))
        {
            Start = i;
            break;
        }
    }

    for (i = Start; i < LPN_PER_BUF; i++)
    {
        if ((usValue & (1 << i)) == 0)
        {
            break;
        }
    }

    End = i;
    return (End - Start);
#endif
}
//////////////////////////////////////////////////////////////////////////
//PopStage PopGCNumLpn(U8 ucSuperPu, PhysicalAddr* pAddr, OUT U8* pNum )
//                        
//function:
//    Pop "*pNum" LPNs from physical Addr "*pAddr".
//    *pNum = "Num of post ones";
//    
//history:
//    2015-01-28 Javenliu  Initial version.
//    2015-02-02 Javenliu  The value of *pNum is equal to "Num of post ones" rather than 
//                         min{ "Num of post ones", LPN_PER_BUF - g_GCManager->m_GCReadOffset[ucSuperPu] };
//
//////////////////////////////////////////////////////////////////////////
PopStage PopGCNumLpn(U8 ucSuperPu, PhysicalAddr* pAddr, U8* pNum, GCComStruct *ptCom, U16 usCurTLCTargetPage)
{
    U16 LpnMap_Page;
    U8  NumOfPostOnes;
    U8  NumOfPostZeros;
    U16 WLO;
    U8  Offset;
    U8 ucTLun;
    U16 usPrefix;
    U16 usPrefixCnt = 1;
    BOOL ucSrcIsTLC = FALSE;
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    U8  NumExtra;
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED    
    BOOL bWordLineClose = FALSE;
#else
    BOOL bSharedPageClose = FALSE;
#endif    
#endif    

    // set the page per word line parameter based on the GC source block type
    if (VBT_TYPE_TLCW == ptCom->m_ucSrcBlkType[ucSuperPu])
    {
        usPrefixCnt = PG_PER_WL;
        ucSrcIsTLC = TRUE;
    }

    WLO = ptCom->m_SrcWLO[ucSuperPu];           // wordline offset in some SLC section
    usPrefix = ptCom->m_SrcPrefix[ucSuperPu];   // which SLC section
    Offset = (U8)ptCom->m_SrcOffset[ucSuperPu]; // LPN offset in LUN

    // TLC mode
    if (WLO >= (PG_PER_SLC_BLK - 1))
    {
        if (FALSE == ucSrcIsTLC)
        {
            return POP_BLOCK_DONE;
        }
        else
        {
            /* for a TLC block, we have to check if all pages within all word lines are read */
            ptCom->m_SrcPrefix[ucSuperPu] += 1;
            if (PG_PER_WL == ptCom->m_SrcPrefix[ucSuperPu])
            {
                return POP_BLOCK_DONE;
            }
            else
            {
                ptCom->m_SrcWLO[ucSuperPu] = 0;
                ptCom->m_SrcOffset[ucSuperPu] = 0;

                WLO = ptCom->m_SrcWLO[ucSuperPu];
                usPrefix = ptCom->m_SrcPrefix[ucSuperPu];
                Offset = (U8)ptCom->m_SrcOffset[ucSuperPu];
            }
        }
    } 

    pAddr->m_PUSer = ucSuperPu;
    pAddr->m_BlockInPU = ptCom->m_SrcPBN[ucSuperPu];
    if (usPrefixCnt == 1)
    {
        pAddr->m_PageInBlock = WLO;
    }
    else
    {
        pAddr->m_PageInBlock = WLO + usPrefix *(PG_PER_SLC_BLK - 1); 
    }
    pAddr->m_OffsetInSuperPage = ptCom->m_SrcOffset[ucSuperPu] / LPN_PER_BUF;

    LpnMap_Page = pDPBM->m_LpnMap[ucSuperPu][pAddr->m_BlockInPU].m_LpnMapPerPage[pAddr->m_OffsetInSuperPage][pAddr->m_PageInBlock];

    if (0 == (LpnMap_Page & (1 << (Offset % LPN_PER_BUF))))
    {
        NumOfPostZeros = CalcPopOneNum((~LpnMap_Page) & (LPN_PER_BUF_BITMAP << (Offset % LPN_PER_BUF)));
        Offset += NumOfPostZeros;

        if ((Offset >= LPN_PER_BUF) && (Offset % LPN_PER_BUF == 0)) // No valid LPNs in current physical page.
        {
            ptCom->m_SrcOffset[ucSuperPu] = Offset;
            if (ptCom->m_SrcOffset[ucSuperPu] >= LPN_PER_SUPERBUF)
            {
                ptCom->m_SrcOffset[ucSuperPu] = 0;
                ptCom->m_SrcWLO[ucSuperPu]++;
            }
            *pNum = 0;

            return POP_ZERO_LPN;
        }
    }

    ptCom->m_SrcOffset[ucSuperPu] = Offset;
    pAddr->m_LPNInPage = Offset % LPN_PER_BUF;
    pAddr->m_OffsetInSuperPage = Offset / LPN_PER_BUF;

    ucTLun = L2_GetPuFromAddr(pAddr);
    if (FALSE == L2_FCMDQNotFull(ucTLun))
    {
        return POP_NOT_ALLOWED;
    }
    NumOfPostOnes = CalcPopOneNum((LPN_PER_BUF_BITMAP << (Offset % LPN_PER_BUF)) & LpnMap_Page);

    *pNum = (U8)NumOfPostOnes;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
#ifdef SHUTDOWN_STAGE2_WORDLINE_CLOSED
    bWordLineClose = L2_GetWordLineClose(usCurTLCTargetPage); 

    /* GgiaHsu : Aggressive calculation : Prevent from shutdown event.*/
    if (bWordLineClose && (usCurTLCTargetPage < TLC_FORCE_TO_CLOSE_PAGE_NUM))
#else
    bSharedPageClose = L2_GetSharedPageClose(usCurTLCTargetPage); 

    /* GgiaHsu : Aggressive calculation : Prevent from shutdown event.*/
    if (bSharedPageClose && (usCurTLCTargetPage < TLC_FORCE_TO_CLOSE_PAGE_NUM))
#endif
    { 
        if (LPN_PER_SUPERBUF <= (NumOfPostOnes + ptCom->m_GCReadOffset[ucSuperPu]))
        {
            NumExtra = LPN_PER_SUPERBUF - ptCom->m_GCReadOffset[ucSuperPu];
        }
        else
        {
            NumExtra = (U8)NumOfPostOnes;
        }
        
        ptCom->m_SrcOffset_Extra[ucSuperPu] = ptCom->m_SrcOffset[ucSuperPu];
        ptCom->m_SrcWLO_Extra[ucSuperPu]    = ptCom->m_SrcWLO[ucSuperPu];
        
        ptCom->m_SrcOffset_Extra[ucSuperPu] += NumExtra;        
      
        /* Need to check boundary */
        if (ptCom->m_SrcOffset_Extra[ucSuperPu] >= LPN_PER_SUPERBUF)
        {
            ptCom->m_SrcOffset_Extra[ucSuperPu] = 0;
            ptCom->m_SrcWLO_Extra[ucSuperPu]++;
        }
    } 
#endif    
    
    
    if ((PG_PER_TLC_BLK - 4) == usCurTLCTargetPage)
    {
        if (LPN_PER_SUPERBUF <= (NumOfPostOnes + ptCom->m_GCReadOffset[ucSuperPu]))
        {
            *pNum = LPN_PER_SUPERBUF - ptCom->m_GCReadOffset[ucSuperPu];
        }
    }  

    //FIRMWARE_LogInfo("LpnMap_Page 0x%x Blk 0x%x Pg 0x%x LUN %d LPNInPage %d Offset %d Num %d GCReadOffset %d\n", LpnMap_Page, pAddr->m_BlockInPU, pAddr->m_PageInBlock, pAddr->m_OffsetInSuperPage,
    //    pAddr->m_LPNInPage, ptCom->m_SrcOffset[ucSuperPu], (*pNum), ptCom->m_GCReadOffset[ucSuperPu]);

    ptCom->m_SrcOffset[ucSuperPu] += *pNum;
    if (ptCom->m_SrcOffset[ucSuperPu] >= LPN_PER_SUPERBUF)
    {
        ptCom->m_SrcOffset[ucSuperPu] = 0;
        ptCom->m_SrcWLO[ucSuperPu]++;
    }

    return POP_MULTI_LPN;
}

//////////////////////////////////////////////////////////////////////////
//PopStage PopGCUpPageNumLpn
//description :
//    For copy to IM 3D TLC extra page, up page need to write again, select up page address from table.
//function:
//    Pop "*pNum" LPNs from physical Addr "*pAddr".
//    *pNum = "Num of post ones";
//    
//history:
//    2016-09-13 DannierChen Created.
//
//
//////////////////////////////////////////////////////////////////////////
PopStage PopGCUpPageNumLpn(U8 ucSuperPu, PhysicalAddr* pAddr, U8* pNum, GCComStruct *ptCom, U16 usCurTLCTargetPage)
{
    U8  NumOfPostOnes = 1;
    PhysicalAddr TmpAddr;
    PhysicalAddr TmpAddrNext;
    U8 ucTLun;
    U16 TgtOffset;

    TgtOffset = ptCom->m_GCUpOfExtraReadOffset[ucSuperPu];

    pAddr->m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + TgtOffset].m_PPN;

    if (pAddr->m_PPN == INVALID_8F)
    {
        /* if it is dummy write, at most fill super page size of dummy LPN */
        *pNum = NumOfPostOnes = LPN_PER_SUPERBUF - TgtOffset;
#ifdef SIM
        if (TgtOffset != 0)
        {
            DBG_Printf("CurPage=%d TgtOffset=%d\n", usCurTLCTargetPage, TgtOffset);
        }
        if (LPN_PER_SUPERBUF < (NumOfPostOnes + ptCom->m_GCUpOfExtraReadOffset[ucSuperPu]))
        {
            *pNum = LPN_PER_SUPERBUF - ptCom->m_GCUpOfExtraReadOffset[ucSuperPu];
            DBG_Getch();
        }
#endif
        return POP_MULTI_LPN;
    }

    ucTLun = L2_GetPuFromAddr(pAddr);
    if (FALSE == L2_FCMDQNotFull(ucTLun))
    {
        return POP_NOT_ALLOWED;
    }

    while (TRUE)
    {
        TmpAddr.m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + TgtOffset].m_PPN;
        TgtOffset++;
        TmpAddrNext.m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + TgtOffset].m_PPN;

        if ((TmpAddr.m_LPNInPage + 1) == TmpAddrNext.m_LPNInPage &&
            TmpAddr.m_PUSer == TmpAddrNext.m_PUSer &&
            TmpAddr.m_BlockInPU == TmpAddrNext.m_BlockInPU &&
            TmpAddr.m_OffsetInSuperPage == TmpAddrNext.m_OffsetInSuperPage &&
            TmpAddr.m_PageInBlock == TmpAddrNext.m_PageInBlock)
        {
            NumOfPostOnes++;
        }
        else
        {
            break;
        }
    }

    /* each time handle maximum one page of a lun, so maximum NumOfPostOnes = (4lpn * plane)  */
    *pNum = (U8)NumOfPostOnes;

    /* each collect can't over a super page, due to over page will fill to extra page rather than next up page */
    if (LPN_PER_SUPERBUF <= (NumOfPostOnes + ptCom->m_GCUpOfExtraReadOffset[ucSuperPu]))
    {
        *pNum = LPN_PER_SUPERBUF - ptCom->m_GCUpOfExtraReadOffset[ucSuperPu];
    }

    //FIRMWARE_LogInfo("PopGCUpPageNumLpn LUN %d Blk 0x%x Pg 0x%x LUN %d LPNInPage %d Offset %d Num %d GCReadOffset %d\n", pAddr->m_OffsetInSuperPage, pAddr->m_BlockInPU, pAddr->m_PageInBlock, pAddr->m_OffsetInSuperPage,
    //    pAddr->m_LPNInPage, ptCom->m_GCUpOfExtraReadOffset[ucSuperPu], (*pNum), ptCom->m_GCUpOfExtraReadOffset[ucSuperPu]);

    return POP_MULTI_LPN;
}


void L2_LoadRPMTInLun(RPMT* pRPMT, U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN, U8* pStatus, BOOL bSLCMode, U8 ucRPMTO)
{
    PhysicalAddr Addr = { 0 };

#ifdef SIM
    if ((VBT_NOT_TARGET != pVBT[ucSuperPu]->m_VBT[VBN].Target)
        && (TRUE == L2_IsBootupOK()))
    {
        DBG_Printf("L2_LoadRPMTInLun error,PU %d PBN %d Target %d\n", ucSuperPu, VBN, pVBT[ucSuperPu]->m_VBT[VBN].Target);
        DBG_Getch();
    }
#endif

    Addr.m_PUSer = ucSuperPu;
    Addr.m_BlockInPU = VBN;
    Addr.m_OffsetInSuperPage = ucLunInSuperPu;
    Addr.m_LPNInPage = 0;

    if (TRUE == bSLCMode)
    {
        Addr.m_PageInBlock = PG_PER_SLC_BLK - 1;
    }
    else
    {
        if (TRUE == L2_VBT_Get_TLC(ucSuperPu, VBN))
        {
            // set the RPMT page number, ucRPMTO specifies which RPMT page
            // we're going to read, there are 3 in a TLC block
            Addr.m_PageInBlock = (PG_PER_SLC_BLK - 1) * PG_PER_WL + ucRPMTO;
            bSLCMode = FALSE;
        }
        else
        {
            // SLC block
            Addr.m_PageInBlock = (PG_PER_SLC_BLK - 1);
            bSLCMode = TRUE;
        }
    }

    //FIRMWARE_LogInfo("GC LoadRPMT Pu %d Blk %d (PhyBlk %d) Pg %d bSLCMode %d\n", ucSuperPu, VBN, pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[0], Addr.m_PageInBlock, bSLCMode);

    L2_FtlReadLocal((U32*)(&pRPMT->m_RPMT[ucLunInSuperPu]), &Addr, pStatus, NULL, LPN_PER_BUF, 0, FALSE, bSLCMode);
}

FTL_THREAD_TYPE SelectSLCGCBlock(U8 ucSuperPu, U8 ucSrcVBTType)
{
    U16 BlockSN;
    PuInfo* pInfo;
    GCManager* pGCManager;
    BOOL bInFGEQ = FALSE;

    pInfo = g_PuInfo[ucSuperPu];
    pGCManager = g_GCManager[SLCGC_MODE];

    /* select GC src block from ForceGC queue */
    BlockSN = L2_BlkQueuePopBlock(ucSuperPu, g_pForceGCSrcBlkQueue[ucSuperPu], VBT_TYPE_HOST, TRUE);

    if (INVALID_4F == BlockSN)
    {
        /* ForceGC queue doesn't have needed block, select block follow original flow */
        BlockSN = L2_SelectGCBlock(ucSuperPu, ucSrcVBTType);
    }
    else 
    {   
        bInFGEQ = TRUE;
#ifdef SIM
        if (VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[BlockSN].Target)
        {
            g_aDbgForceGCClosedBlkCnt[ucSuperPu] += 1;
            DBG_Printf("MCU%d Force GC SuperPu %d, SrcVBN 0x%x, Target %d, HOSTW_VBN 0x%x HOSTGC_VBN 0x%x  TLCW_VBN 0x%x\n",
                HAL_GetMcuId(), ucSuperPu, BlockSN, pVBT[ucSuperPu]->m_VBT[BlockSN].Target,
                g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML], g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_HOST_GC], g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE]);
        }
        else
        {
            g_aDbgForceGCTargetBlkCnt[ucSuperPu] += 1;
        }
#endif
    }


    if (BlockSN == INVALID_4F)
    {
        DBG_Printf("SuperPu:%d Can't Get SLCGC victim block, IdleGC=%d SLC AllocatedBlkCnt=%d\n",
            ucSuperPu, g_tFTLIdleGCMgr.bIdleGC, pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]);
        return FTL_THREAD_TYPE_ALL;
    }
    else
    {
        // if the selected GC source block is a SLC block and the SLC block is actually not that dirty
        // (dirty count < 1/3 total LPN count), we ignore target SLC GC and move to TLC merge
        if (L2_GetDirtyCnt(ucSuperPu, BlockSN) < (LPN_PER_SUPER_SLCBLK / 3) && !bInFGEQ) //next GC type : TLC Write
        {
            return FTL_THREAD_GC_2TLC;
        }

        //if (VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[BlockSN].Target)
        {
            SetSLCGCSrcBlock(ucSuperPu, BlockSN);
            pGCManager->tGCCommon.m_ucSrcBlkType[ucSuperPu] = ucSrcVBTType;

            if ((LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT) == L2_GetDirtyCnt(ucSuperPu, BlockSN))
            {
                pGCManager->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_ERASE;
#ifdef READ_DISTURB_OPEN
                L1_ForL2SetReLookupPMTFlag(ucSuperPu);
#endif
            }
            else
            {
                pGCManager->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_PREPARE_GC;
            }

            if (TRUE == gbGCTraceRec)
            {
                L2_PrintDirtyCnt((U8)ucSuperPu, BlockSN);
            }
        }
#if 0
        else
        {

            /* if enter this branch, BlockSN shoud be poped from g_pForceGCSrcBlkQueue*/
            pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] = BlockSN;
            pGCManager->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_TARGET_BLK_PRO;
            pGCManager->m_TargetBlkGCState[ucSuperPu] = TARGET_BLK_GC_STATE_PREPARE;
            pGCManager->m_NeedCopyForOneWrite[ucSuperPu] = 1;

            DBG_Printf("SLCGC select SrcBlk shouldn't be here as ReadDisturb don't statistic SLC area read\n");
            DBG_Getch();
        }
#endif
    }

    return FTL_THREAD_SLCGC;
}

void SelectTLCGCBlock(U8 ucSuperPu, U8 ucSrcVBTType, U8 ucIsSWL)
{
    U16 BlockSN;
    PuInfo* pInfo;
    GCManager* pGCManager;
    U16 usPhyBlk;
    GCComStruct *ptCom;

    pInfo = g_PuInfo[ucSuperPu];
    pGCManager = g_GCManager[TLCGC_MODE];
    ptCom = &pGCManager->tGCCommon;
#ifdef NEW_SWL
    if (ucIsSWL)
    {
        if (pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] == INVALID_4F)
        {
            if (gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] != INVALID_4F)
            {
                BlockSN = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                g_TLCManager->ausSrcBlk[ucSuperPu][0] = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
            }
            else if (gwl_info->nSrcBlkBuf[ucSuperPu] != INVALID_4F)
            {
                BlockSN = gwl_info->nSrcBlkBuf[ucSuperPu];
                g_TLCManager->ausSrcBlk[ucSuperPu][0] = gwl_info->nSrcBlkBuf[ucSuperPu];
                gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
            }
            else
            {
                L2_SelectSWLSrcBlk(ucSuperPu);
                if (gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] != INVALID_4F)
                {
                    BlockSN = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                    g_TLCManager->ausSrcBlk[ucSuperPu][0] = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                    gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
                }
                else
                {
                   //here need jump to Dummy Write Status or loosen the selection condition
                    BlockSN = INVALID_4F;
                }
            }
        }
        else
        {
            DBG_Printf("Error! Wrong Src blk SPU=%d\n", ucSuperPu);
            DBG_Getch();
        }
#ifdef SWL_EVALUATOR
        DBG_DumpTLCSrcBlkDCnt(BlockSN, L2_GetDirtyCnt(ucSuperPu, BlockSN), TRUE);
#endif       
    }
    else
#else
    if (ucIsSWL)
    {
        if (pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] == INVALID_4F)
        {
            if (gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] != INVALID_4F)
            {
                BlockSN = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                g_TLCManager->ausSrcBlk[ucSuperPu][0] = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
            }
            else
            {
                L2_SelectSWLSrcBlk(ucSuperPu);
                if (gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] != INVALID_4F)
                {
                    BlockSN = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                    g_TLCManager->ausSrcBlk[ucSuperPu][0] = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
                    gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
                }
                else
                {
                    //here need jump to Dummy Write Status or loosen the selection condition
                    DBG_Printf("Error! Wrong Src blk selection SPU=%d\n", ucSuperPu);
                    DBG_Getch();
                }
            }
        }
        else
        {
            //here need jump to Dummy Write Status or loosen the selection condition
            BlockSN = INVALID_4F;
        }
#ifdef SWL_EVALUATOR
        DBG_DumpTLCSrcBlkDCnt(BlockSN, L2_GetDirtyCnt(ucSuperPu, BlockSN), TRUE);
#endif
}
    else
#endif
    {
        /* select GC src block from ForceGC queue */
        BlockSN = L2_BlkQueuePopBlock(ucSuperPu, g_pForceGCSrcBlkQueue[ucSuperPu], VBT_TYPE_TLCW, FALSE);

        if (INVALID_4F == BlockSN)
        {
            /* ForceGC queue doesn't have needed block, select block follow original flow */
            BlockSN = L2_SelectGCBlock(ucSuperPu, ucSrcVBTType);
        }
#ifdef SIM
        else 
        {
            if (VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[BlockSN].Target)
            {
                g_aDbgForceGCClosedBlkCnt[ucSuperPu] += 1;
                DBG_Printf("MCU%d Force GC SuperPu %d, SrcVBN 0x%x, Target %d, HOSTW_VBN 0x%x HOSTGC_VBN 0x%x TLCW_VBN 0x%x\n",
                HAL_GetMcuId(), ucSuperPu, BlockSN, pVBT[ucSuperPu]->m_VBT[BlockSN].Target,
                g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_HOST_WRITE_NORAML], g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_HOST_GC], g_PuInfo[ucSuperPu]->m_TargetBlk[TARGET_TLC_WRITE]);
            }
            else
            {
                g_aDbgForceGCTargetBlkCnt[ucSuperPu] += 1;
            }
        }
#endif

#ifdef SWL_EVALUATOR
        DBG_DumpTLCSrcBlkDCnt(BlockSN, L2_GetDirtyCnt(ucSuperPu, BlockSN), FALSE);
#endif
    }

    if (BlockSN == INVALID_4F)
    {
        DBG_Printf("SuperPu:%d Can't Get TLCGC victim block, ucSrcVBTType:%d, write dummy LPN\n", ucSuperPu, ucSrcVBTType);
        ptCom->m_TLCGCDummyWrite[ucSuperPu] = TRUE;
        pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
        pGCManager->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_COPY_VALID;

#ifdef SIM
        DBG_Printf("TLCGC 1st_type=%d GCOfs=%d UpOfs=%d\n",	L2_GetPageTypeByProgOrder(ucSuperPu),
        ptCom->m_GCReadOffset[ucSuperPu], ptCom->m_GCUpOfExtraReadOffset[ucSuperPu]);
        g_pTLCGCSrcBlkRecord->m_CopyDummyLPNCnt[ucSuperPu] = 0;
#endif
    }
    else
    {
        //if (VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[BlockSN].Target)
        {
            pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] = BlockSN;
            pGCManager->tGCCommon.m_SrcWLO[ucSuperPu] = 0;
            pGCManager->tGCCommon.m_SrcPrefix[ucSuperPu] = 0;
            pGCManager->tGCCommon.m_SrcOffset[ucSuperPu] = 0;
            pGCManager->tGCCommon.m_ucSrcBlkType[ucSuperPu] = ucSrcVBTType;
            pGCManager->tGCCommon.m_ucRPMTNum[ucSuperPu] = 0;

            /* lock GC src block */
            L2_PBIT_Set_Lock(ucSuperPu, BlockSN, TRUE);

            if ((LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT) == L2_GetDirtyCnt(ucSuperPu, BlockSN))
            {
                /*add Current block into TLCGCSrcBlkQue*/
                L2_PBIT_Set_Lock(ucSuperPu, BlockSN, FALSE);
                L2_InsertBlkIntoEraseQueue(ucSuperPu, pGCManager->tGCCommon.m_SrcPBN[ucSuperPu], FALSE);

#ifdef READ_DISTURB_OPEN
                L1_ForL2SetReLookupPMTFlag(ucSuperPu);
#endif

                //FIRMWARE_LogInfo("SuperPU %d SrcBlk 0x%x Push into EraseQue when TLCGCSrcSelect\n", ucSuperPu, pGCManager->tGCCommon.m_SrcPBN[ucSuperPu]);

                pGCManager->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
                //DBG_Printf("SelectTLCGCBlock(BlockSN=%d) ucIsSWL(%d) => GC_STATE_SELECT_TLCGC_SRC\n", BlockSN, ucIsSWL);
                pGCManager->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_SELECT_TLCGC_SRC; //GC_STATE_ERASE;

                //if both the first two src blks are all-dirty(generally, DC ratio of src blk is about 10%), then cancel this SWL , but keep the dst blk till next TLC erase
                if (ucIsSWL)
                {
                    if ((INVALID_4F == gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu]) && (INVALID_4F == gwl_info->nSrcBlkBuf[ucSuperPu])
                        && (0 == g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]))
                    {
                        U32 ulPBN, j;
                        L2_FTLTaskTLCSWLClear(ucSuperPu);
                        L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_WL);
                        L2_InitGCManager(ucSuperPu, TLCGC_MODE);
                        L2_ResetTLCManager(ucSuperPu);

                        for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
                        {
                            ulPBN = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[j];
                            pPBIT[ucSuperPu]->m_PBIT_Entry[j][ulPBN].bFree = TRUE;
                            pPBIT[ucSuperPu]->m_PBIT_Entry[j][ulPBN].bAllocated = FALSE;
                            L2_Set_PBIT_BlkSN_Blk(ucSuperPu, j, ulPBN, INVALID_8F);
                        }
                        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].StripID = INVALID_4F;
                        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].Target = VBT_TARGET_INVALID;
                        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].bFree = TRUE;
                        pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].VBTType = VBT_TYPE_INVALID;
                        g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC]--;
                        L2_PBIT_Decrease_AllocBlockCnt(ucSuperPu, BLKTYPE_TLC);

                        DBG_Printf("SWL source blk selection: both the first two src blks are all-dirty\n");
                        return;
                    }                    
                }
            }
            else
            {
                //FIRMWARE_LogInfo("SuperPU %d TLGC SrcBlk 0x%x DirtyCnt 0x%x\n", ucSuperPu, pGCManager->tGCCommon.m_SrcPBN[ucSuperPu], L2_GetDirtyCnt(ucSuperPu, BlockSN));
            #ifdef SIM
                g_pTLCGCSrcBlkRecord->m_CopyValidLPNCnt[ucSuperPu][g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]] = 0;
                g_pTLCGCSrcBlkRecord->m_NewAddDirtyCnt[ucSuperPu][g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]] = 0;
                g_pTLCGCSrcBlkRecord->m_DirtyCnt[ucSuperPu][g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]] = L2_GetDirtyCnt(ucSuperPu, BlockSN);
            #endif
                if (0 == L2_GetDirtyCnt(ucSuperPu, pGCManager->tGCCommon.m_SrcPBN[ucSuperPu]) && (g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] == 0))
                {
                    if (g_pTLCGCSrcBlkRecord->bIsSrcBlkAllValid[ucSuperPu] == TRUE)
                    {
                        DBG_Printf("Wrong Src Blk valid info\n");
                        DBG_Getch();
                    }
                    else
                    {
                        g_pTLCGCSrcBlkRecord->bIsSrcBlkAllValid[ucSuperPu] = TRUE;
                    }
                }

                if ((0 == g_TLCManager->ausTLCProgOrder[ucSuperPu])
                    && (0 == g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]))
                {   
                    usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, pGCManager->tGCCommon.m_SrcPBN[ucSuperPu]);
                    g_pTLCGCSrcBlkRecord->m_TLCGC1stSrcBlkTS[ucSuperPu] = pPBIT[ucSuperPu]->m_PBIT_Info[0][usPhyBlk].LastPageTimeStamp;
                    g_pTLCGCSrcBlkRecord->l_aTLC1stGCSrcBlk[ucSuperPu] = pGCManager->tGCCommon.m_SrcPBN[ucSuperPu];
                }

                //DBG_Printf("SelectTLCGCBlock(BlockSN=%d) ucIsSWL(%d) => GC_STATE_LOAD_RPMT\n", BlockSN, ucIsSWL);
                pGCManager->tGCCommon.m_GCStage[ucSuperPu] = GC_STATE_LOAD_RPMT;
                /* Consume current TLC GC(WL) Quota avoid host write timeout issue. */
                if(g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] > 2)
                {
                   if (ucIsSWL)
                   {
                       L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_WL);
                   }
                   else
                   {
                       L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_TLCGC);
                   }
                }
            }

            if (TRUE == gbGCTraceRec)
            {
                L2_PrintDirtyCnt((U8)ucSuperPu, BlockSN);
            }
        }
#if 0
        else
        {
            DBG_Printf("TLCGC select SrcBlk shouldn't be here, as TLC block PMT update after block close\n");
            DBG_Getch();
        }
#endif
    }

    return;
}

/*TLCMerge, TLCGC and SLCGC all need to load RPMT, bMerge*/
void L2_LoadRPMT(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucRPMTNum, U16 usVBN, U8 ucTarget, BOOL bTLCGC)
{
    U8 ucLunInSuperPu = 0;
    U8 ucTLun;
    RPMT *pRPMT;
    U8 *pStatus;
    U32 *pReadBitMap;
    BOOL bSLCMode = FALSE;

    if (VBT_TARGET_TLC_W == ucTarget)
    {
        if (TRUE == bTLCGC)
        {
            /*TLC GC*/
            pRPMT = g_GCManager[TLCGC_MODE]->tGCCommon.m_pRPMT[ucSuperPu][ucRPMTNum];
            pReadBitMap = &g_GCManager[TLCGC_MODE]->tGCCommon.m_LoadRPMTBitMap[ucSuperPu];
        }
        else
        {
            /* TLC merge or TLCSWL use, write to TLC block */
            pRPMT = g_TLCManager->pRPMT[ucSuperPu][ucRPMTNum];
            pReadBitMap = &g_TLCManager->aucTLCReadBitMap[ucSuperPu];
        }
    }
    else if (VBT_TARGET_HOST_GC == ucTarget)
    {
        /* SLC GC */
        pRPMT = g_GCManager[SLCGC_MODE]->tGCCommon.m_pRPMT[ucSuperPu][ucRPMTNum];
        pReadBitMap = &g_GCManager[SLCGC_MODE]->tGCCommon.m_LoadRPMTBitMap[ucSuperPu];
        bSLCMode = TRUE;
    }
    else
    {
        DBG_Printf("L2_LoadRPMT ucTarget%d error\n", ucTarget);
        DBG_Getch();
    }

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);

        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        if (0 != ((*pReadBitMap) & (1 << ucLunInSuperPu)))
        {
            continue;
        }

        if (VBT_TARGET_TLC_W == ucTarget)
        {
            if (TRUE == bTLCGC)
            {
                pStatus = &g_GCManager[TLCGC_MODE]->tGCCommon.m_RPMTStatus[ucSuperPu][ucLunInSuperPu][ucRPMTNum];
            }
            else
            {
                pStatus = &g_TLCManager->aucRPMTStatus[ucSuperPu][ucLunInSuperPu][ucRPMTNum];
            }
        }
        else if (VBT_TARGET_HOST_GC == ucTarget)
        {
            pStatus = &g_GCManager[SLCGC_MODE]->tGCCommon.m_RPMTStatus[ucSuperPu][ucLunInSuperPu][ucRPMTNum];
        }

        L2_LoadRPMTInLun(pRPMT, ucSuperPu, ucLunInSuperPu, usVBN, pStatus, bSLCMode, ucRPMTNum);

        (*pReadBitMap) |= (1 << ucLunInSuperPu);

        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);
    }
    return;
}

GCState L2_GCLoadRPMT(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, GCComStruct *ptCom)
{
    U8 *pucRPMTNum;
    U8 ucRpmtNum;
    U8 ucTargetType;
    U8 ucGCMode;

    if (VBT_TYPE_TLCW == ptCom->m_ucSrcBlkType[ucSuperPu])
    {
        ucGCMode = TLCGC_MODE;
        ucRpmtNum = PG_PER_WL;
        ucTargetType = VBT_TARGET_TLC_W;
    }
    else if (VBT_TYPE_HOST == ptCom->m_ucSrcBlkType[ucSuperPu])
    {
        ucGCMode = SLCGC_MODE;
        ucRpmtNum = 1;
        ucTargetType = VBT_TARGET_HOST_GC;
    }
    else
    {
        DBG_Printf("GC source block type error\n");
        DBG_Getch();
    }

    pucRPMTNum = &(ptCom->m_ucRPMTNum[ucSuperPu]);

    L2_LoadRPMT(ucSuperPu, ulLunAllowToSendFcmdBitmap, *pucRPMTNum, ptCom->m_SrcPBN[ucSuperPu], ucTargetType, TRUE);

    if (SUPERPU_LUN_NUM_BITMSK == ptCom->m_LoadRPMTBitMap[ucSuperPu])
    {
        ptCom->m_LoadRPMTBitMap[ucSuperPu] = 0;

        (*pucRPMTNum)++;
        if ((*pucRPMTNum) >= ucRpmtNum)
        {
            if (TRUE == gbGCTraceRec)
            {
                L2_DbgGC(11, ucSuperPu, ucGCMode);
            }
            return GC_STATE_COPY_VALID;
        }
        else
        {
            return GC_STATE_LOAD_RPMT;
        }
    }
    else
    {
        return GC_STATE_LOAD_RPMT;
    }
}


U32 CopyValidMultiLpn(U8 ucSuperPu, GCComStruct *ptCom, U8 ucGCMode)
{
    U8  Num = 0;
    U32 LPN = 0;
    U8  BufferPointer = 0;
    U8 ucPGO;
    U8 ucPGCnt = 1;
    PhysicalAddr Addr = { 0 };
    PopStage bReadFinish;
    U16 TgtOffset,i;
    U8 ucLunInSuperPu;
    GC_ERROR_HANDLE * ptGCErrorPointer;
    U16 usCurTLCTargetPage = 0;
    U32 ulSrcBlkCnt;
    U8 ucPageType, ucProgPageCnt;
    U16 uc1stPageNum, uc2ndPageNum;   
    U8 ucCopyCnt;
    BOOL usUpOfXtraPage = FALSE;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    if(((0 == ptCom->m_SrcWLO[ucSuperPu]) && (0 == ptCom->m_SrcPrefix[ucSuperPu]) && (ptCom->m_TLCGCDummyWrite[ucSuperPu] == FALSE))
       || (g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCNeedtoWaitforRPMT[ucSuperPu]))
#else
    if ((0 == ptCom->m_SrcWLO[ucSuperPu]) && (0 == ptCom->m_SrcPrefix[ucSuperPu]) && (ptCom->m_TLCGCDummyWrite[ucSuperPu] == FALSE))
#endif        
    {
        if (VBT_TYPE_TLCW == ptCom->m_ucSrcBlkType[ucSuperPu])
        {
            ucPGCnt = PG_PER_WL;
        }

        for (ucPGO = 0; ucPGO < ucPGCnt; ucPGO++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (SUBSYSTEM_STATUS_PENDING == ptCom->m_RPMTStatus[ucSuperPu][ucLunInSuperPu][ucPGO])
                {
                    return FALSE;
                }
                else if (SUBSYSTEM_STATUS_FAIL == ptCom->m_RPMTStatus[ucSuperPu][ucLunInSuperPu][ucPGO])
                {
                    ptGCErrorPointer = g_GCErrHandle;

                    /* trigger error handling to rebuild RPMT for GC */
                    ptGCErrorPointer[ucSuperPu].m_ErrPU = ucSuperPu;
                    ptGCErrorPointer[ucSuperPu].m_ucLun = ucLunInSuperPu;
                    ptGCErrorPointer[ucSuperPu].m_ErrBlock = ptCom->m_SrcPBN[ucSuperPu];
                    DBG_Printf("PU %d block %d load rpmt %d fail in GC, start to do L2 error handling.\n", g_GCErrHandle[ucSuperPu].m_ErrPU, g_GCErrHandle[ucSuperPu].m_ErrBlock, ucPGO);
                    ptCom->m_GCStage[ucSuperPu] = GC_STATE_ERROR_HANDLING;
                    ptGCErrorPointer[ucSuperPu].m_ucErrRPMTnum = ucPGO;
#ifdef L2_HANDLE_UECC
                    if (TLCGC_MODE == ucGCMode)
                    {
                        L2_AddUECCBlock(ucSuperPu, ucLunInSuperPu, g_GCErrHandle[ucSuperPu].m_ErrBlock);
                    }
#endif
                    return FALSE;
                }

                if ((ptCom->m_pRPMT[ucSuperPu][ucPGO]->m_RPMT[ucLunInSuperPu].m_SuperPU != ucSuperPu) || (ptCom->m_pRPMT[ucSuperPu][ucPGO]->m_RPMT[ucLunInSuperPu].m_SuperBlock != ptCom->m_SrcPBN[ucSuperPu]))
                {
                    U16 usPhyBlock;
                    usPhyBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, ptCom->m_SrcPBN[ucSuperPu]);
                    DBG_Printf("[CopyValidOneLine] PU %d block 0x%x (PBN 0x%x) PG 0x%x Load wrong RPMT  !!!\n", ucSuperPu, ptCom->m_SrcPBN[ucSuperPu], usPhyBlock, ucPGO);
                    DBG_Getch();
                }
            }
        }
    }

    if (TLCGC_MODE == ucGCMode)
    {
        
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        if(g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCNeedtoWaitforRPMT[ucSuperPu])
           g_GCManager[TLCGC_MODE]->tGCCommon.m_TLCGCNeedtoWaitforRPMT[ucSuperPu] = FALSE;
#endif        
        
#ifndef FLASH_IM_3DTLC_GEN2
        if (g_TLCManager->ausTLCProgOrder[ucSuperPu] == (TLC_FIRST_RPMT_PAGE_NUM + 1))
        {
            g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] = 1;
        }
#endif
        uc1stPageNum = L2_Get1stPageNumByProgOrder(ucSuperPu);
        ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
        ucProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);
            
        if (ucProgPageCnt == 2)
           uc2ndPageNum = L2_Get2ndPageNumByProgOrder(ucSuperPu);        
        
        usCurTLCTargetPage = uc1stPageNum;

        ucCopyCnt = g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu];          
        
        if (ucCopyCnt == 1)
        {
            if (ucPageType == L2_EXTRA_PAGE)
            {
#ifndef FLASH_IM_3DTLC_GEN2
                usUpOfXtraPage = TRUE;
#endif
                usCurTLCTargetPage = uc2ndPageNum;
            }
            else if (ucPageType == L2_LOWER_PAGE)
            {
                usCurTLCTargetPage = uc2ndPageNum;
            }
        }
    }

    if (usUpOfXtraPage)
    {
        TgtOffset = ptCom->m_GCUpOfExtraReadOffset[ucSuperPu];
    }
    else
    {
        TgtOffset = ptCom->m_GCReadOffset[ucSuperPu];
    }

    while (TRUE)
    {
        Addr.m_PPN = 0;

        if (usUpOfXtraPage)
        {
            bReadFinish = PopGCUpPageNumLpn(ucSuperPu, &Addr, &Num, ptCom, usCurTLCTargetPage);
        }
        else
        {
            if (ptCom->m_TLCGCDummyWrite[ucSuperPu] == TRUE)
            {
                Num = LPN_PER_SUPERBUF - TgtOffset;
                bReadFinish = POP_MULTI_LPN;
                Addr.m_PPN = INVALID_8F;
            }
            else
            {
                bReadFinish = PopGCNumLpn(ucSuperPu, &Addr, &Num, ptCom, usCurTLCTargetPage);
            }
        }

        if (POP_BLOCK_DONE == bReadFinish)
        {
            if (TLCGC_MODE == ucGCMode)
            {
                /*if current TLCGC SrcBlk Copy valid done and current
                three buffer didn't full, go to select a new SrcBlk*/
                ptCom->m_GCStage[ucSuperPu] = GC_STATE_SELECT_TLCGC_SRC;

                /*Record Current block into TLCGCSRCBlkQue*/
                if (g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] >= TLCGC_SRCBLK_MAX)
                {
                    DBG_Printf("TLCGC SrcBlkQue overflow!\n");
                    DBG_Getch();
                }
                
                ulSrcBlkCnt = g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu];

                if ((0 == ulSrcBlkCnt)
                    || ((0 != ulSrcBlkCnt) && (ptCom->m_SrcPBN[ucSuperPu] != g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ulSrcBlkCnt - 1])))
                {
                    g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ulSrcBlkCnt] = ptCom->m_SrcPBN[ucSuperPu];
                    g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]++;

                    /* if TLCGC exhaust too many, over 1/4 of TLC blocks, then use dummy write to close target block */
                    if (g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] > (g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_TLC] >> 2))
                    {
                        ptCom->m_TLCGCDummyWrite[ucSuperPu] = TRUE;
#ifdef SIM
                        DBG_Printf("TLCGC SRC BLK reach THS CurPage=%d, up=%d 1st_type=%d GCOfs=%d UpOfs=%d\n", usCurTLCTargetPage, ucCopyCnt, ucPageType,
                        ptCom->m_GCReadOffset[ucSuperPu], ptCom->m_GCUpOfExtraReadOffset[ucSuperPu]);
                        g_pTLCGCSrcBlkRecord->m_CopyDummyLPNCnt[ucSuperPu] = 0;
#endif
                    }
                }

                //FIRMWARE_LogInfo("SuperPU %d TLCGCSrcBLK 0x%x CopyValid done CurrentSrcBlkCnt %d\n", ucSuperPu, ptCom->m_SrcPBN[ucSuperPu], g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]);
                ptCom->m_SrcPBN[ucSuperPu] = INVALID_4F;
            }
            else
            {
                BufferPointer = ptCom->m_GCBufferPointer[ucSuperPu];
                for (i = TgtOffset; i < LPN_PER_SUPERBUF; i++)
                {
                    ptCom->m_LPNInBuffer[ucSuperPu][BufferPointer][i] = INVALID_8F;
                }
                /* move the read pointer to the last LPN to accelerate GC */
                ptCom->m_GCReadOffset[ucSuperPu] = LPN_PER_SUPERBUF;

                if (TgtOffset == 0 && (LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT) == L2_GetDirtyCnt(ucSuperPu, ptCom->m_SrcPBN[ucSuperPu]))
                {
                    ptCom->m_GCStage[ucSuperPu] = GC_STATE_ERASE;
                }
            }
            return SUCCESS;
        }

        if (POP_NOT_ALLOWED == bReadFinish)
        {
            return FAIL;
        }

        if (POP_ZERO_LPN == bReadFinish)
        {
            continue;
        }
        else
        {
            if (usUpOfXtraPage)
            {
                ptCom->m_GCUpOfExtraReadOffset[ucSuperPu] += Num;
            }
            else
            {
                ptCom->m_GCReadOffset[ucSuperPu] += Num;
            }
        }

        if (Addr.m_PPN != INVALID_8F)
        {
            /* data read */
            FillBuffer(ucSuperPu, TgtOffset, &Addr, Num, ptCom);
        }
        else
        {
            /* if (Addr.m_PPN == INVALID_8F) that is a dummy write, don't need to read src, just fill buffer with zero */
            FillDummyBuffer(ucSuperPu, TgtOffset, Num, ptCom, usCurTLCTargetPage);
        }

        if (SLCGC_MODE == ucGCMode)
        {
            BufferPointer = ptCom->m_GCBufferPointer[ucSuperPu];  // Which buffer is being used.
        }
        else
        {
            if (usUpOfXtraPage)
            {
                return SUCCESS;
            }

            BufferPointer = (usCurTLCTargetPage) % TLC_BUF_CNT;

#ifdef SIM
            if (ptCom->m_TLCGCDummyWrite[ucSuperPu] == TRUE)
            {
                g_pTLCGCSrcBlkRecord->m_CopyDummyLPNCnt[ucSuperPu] += Num;
            }
            else
            {
                g_pTLCGCSrcBlkRecord->m_CopyValidLPNCnt[ucSuperPu][g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]] += Num;
            }
#endif
        }

        while (Num-- > 0)
        {
            if (ptCom->m_TLCGCDummyWrite[ucSuperPu] == FALSE)
            {
                /* lookup RPMT to get logic LPN from physical addr */
                LPN = LookupRPMTInGC(ucSuperPu, &Addr, ptCom->m_SrcPrefix[ucSuperPu], FALSE, ptCom);
            }
            else
            {
                LPN = INVALID_8F;
                Addr.m_PPN = INVALID_8F;
            }

            ptCom->m_LPNInBuffer[ucSuperPu][BufferPointer][TgtOffset] = LPN;
            if (SLCGC_MODE == ucGCMode)
            {
                ptCom->m_AddrForBuffer[ucSuperPu][BufferPointer][TgtOffset].m_PPN = Addr.m_PPN;
            }
            else
            {
                g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + TgtOffset].m_PPN = Addr.m_PPN;             
            }

            //FIRMWARE_LogInfo("\t####ReadSrc Addr 0x%8x(SuperPu %d Lun %d VirBlk 0x%x Pg 0x%x LpnInPg %d) LPN 0x%x BufferPointer %d TgtOffset %d\n", Addr.m_PPN, 
            //    Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, Addr.m_PageInBlock ,Addr.m_LPNInPage, LPN, BufferPointer, TgtOffset);

            // update the values of Addr and TgtOffset 
            if (ptCom->m_TLCGCDummyWrite[ucSuperPu] == FALSE)
            {
                Addr.m_LPNInPage++;
            }
            TgtOffset++;

            if (TgtOffset >= LPN_PER_SUPERBUF)  /* TLC mode: one super SLC size */
            {
                TgtOffset = 0;
                if (SLCGC_MODE == ucGCMode)
                {
                    BufferPointer = (BufferPointer + 1) % GC_BUFFER_DEPTH;
                }
                else
                {
                    BufferPointer = (BufferPointer + 1) % TLC_BUF_CNT;
                    usCurTLCTargetPage++;
                }
            }
        }
        return SUCCESS;
    }
}

BOOL L2_TLCGCCheckReadStatus(U8 ucSuperPu)
{
    U8 ucStatusAddr;
    U8 ucRemainLPNNum = 0;
    U32 j;
    U32 i;
    U8 ucPageType, ucProgPageCnt, ucRealBufNum;
    U16 uc1stPageNum;

#if 0
    U32 ucLPNOffSet;;
    U8 ucBufO, ucBuffOffset;
    GCComStruct *ptCom;
    U16 usCurTLCTargetPage;
    U32 LPN;
    PhysicalAddr tSrcAddr,tPMTAddr;
#endif
 
    for (i = 0; i < (PG_PER_WL+1); i++)
    {
        for (j = 0; j < LPN_PER_SUPERBUF; j++)
        {
            ucStatusAddr = g_GCManager[TLCGC_MODE]->tGCCommon.m_FlushStatus[ucSuperPu][i][j];
            if (ucStatusAddr == SUBSYSTEM_STATUS_PENDING)
            {
                return FALSE;
            }
        }
    }

    uc1stPageNum = L2_Get1stPageNumByProgOrder(ucSuperPu);
    ucPageType = L2_GetPageTypeByProgOrder(ucSuperPu);
    ucProgPageCnt = L2_GetProgPageCntByProgOrder(ucSuperPu);

    if (ucPageType == L2_EXTRA_PAGE && ucProgPageCnt == 2)
    {
#ifndef FLASH_IM_3DTLC_GEN2
        ucProgPageCnt = 1; /* UpOfXtraPage don't handle read fail, UpOfLowPage UECC already clean LPN in RPMT and redundant of UP page */
#endif

#if 0 /* don't care UpOfXtraPage UECC. */
        for (j = 0; j < LPN_PER_SUPERBUF; j++)
        {
            ucStatusAddr = g_GCManager[TLCGC_MODE]->tGCCommon.m_FlushStatus[ucSuperPu][PG_PER_WL][j];
            if (ucStatusAddr == SUBSYSTEM_STATUS_FAIL)
            {
                DBG_Printf("GetCh, UpOfXtraPage UECC, it shouldn't read fail, if happen it should be at UpOfLowPage\n");
                DBG_Getch();
            }
        }
#endif
    }
 
    for (i = 0; i < ucProgPageCnt; i++)
    {
        for (j = 0; j < LPN_PER_SUPERBUF; j++)
        {
#ifndef FLASH_IM_3DTLC_GEN2
            if (ucPageType == L2_EXTRA_PAGE && i == 1)
            {
                ucRealBufNum = PG_PER_WL;
            }
            else
#endif
            {
                ucRealBufNum = (uc1stPageNum + i) % PG_PER_WL;
            }
            ucStatusAddr = g_GCManager[TLCGC_MODE]->tGCCommon.m_FlushStatus[ucSuperPu][ucRealBufNum][j];
            if (ucStatusAddr == SUBSYSTEM_STATUS_EMPTY_PG)
            {
                DBG_Printf("TLC GC read src data EMPTY Getch\n");
                DBG_Getch();
            }

            if (ucStatusAddr == SUBSYSTEM_STATUS_FAIL)
            {
#ifdef L2_HANDLE_UECC
                U8 ucBuffOffset;
                U32 ucLPNOffSet;
                U16 usCurTLCTargetPage;
                PhysicalAddr tSrcAddr;

                ucLPNOffSet = j;
                ucBuffOffset = i;

                usCurTLCTargetPage = uc1stPageNum;
                if ((g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] == 1) && ((ucPageType == L2_EXTRA_PAGE) || (ucPageType == L2_LOWER_PAGE)))
                {
                    usCurTLCTargetPage = L2_Get2ndPageNumByProgOrder(ucSuperPu);
                }
                if (ucBuffOffset != 0)
                {
                    usCurTLCTargetPage = usCurTLCTargetPage + ucBuffOffset;
                }
                tSrcAddr.m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + ucLPNOffSet].m_PPN;
                DBG_Printf("L2_TLCGCCheckReadStatus: UECC spu=%d lunInspu=%d blk=%d page=%d Buf %d BuffOffset %d LPN %d\n", ucSuperPu, tSrcAddr.m_OffsetInSuperPage, tSrcAddr.m_BlockInPU, tSrcAddr.m_PageInBlock, ucRealBufNum, ucBuffOffset, ucLPNOffSet);
                L2_AddUECCBlock(ucSuperPu, tSrcAddr.m_OffsetInSuperPage, tSrcAddr.m_BlockInPU);
                g_GCManager[TLCGC_MODE]->tGCCommon.m_FlushStatus[ucSuperPu][ucRealBufNum][j] = SUBSYSTEM_STATUS_SUCCESS;
#else
#if 0
                ptCom = &g_GCManager[TLCGC_MODE]->tGCCommon;

                /*Set FailRead data LPN to "INVALID_8F"*/  
                /*step1: process continuous data in one buffer*/
                ucLPNOffSet = j;
                ucBuffOffset = i;
                do
                {
                    usCurTLCTargetPage = uc1stPageNum + ucBuffOffset;
                    ucBufO = (usCurTLCTargetPage) % TLC_BUF_CNT;
                    LPN = ptCom->m_LPNInBuffer[ucSuperPu][ucBufO][ucLPNOffSet];
                    L2_LookupPMT(&tPMTAddr, LPN, FALSE);
                    tSrcAddr.m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + ucLPNOffSet].m_PPN;
                    DBG_Printf("TLCGC UECC spu=%d lunInspu=%d blk=%d page=%d LPNOffset=%d LPNOffsetInBuffer=%d\n", ucSuperPu, tSrcAddr.m_OffsetInSuperPage, tSrcAddr.m_BlockInPU, tSrcAddr.m_PageInBlock, tSrcAddr.m_LPNInPage, ucLPNOffSet);
                    ptCom->m_LPNInBuffer[ucSuperPu][ucBufO][ucLPNOffSet] = INVALID_8F;
                    if (tSrcAddr.m_PPN == tPMTAddr.m_PPN)
                    {
                        if (TRUE == L2_LookupDirtyLpnMap(&tPMTAddr))
                        {
                            PhysicalAddr NewAddr = { 0 };

                            L2_UpdateDirtyLpnMap(&tSrcAddr, B_DIRTY);
                            L2_IncreaseDirty(tSrcAddr.m_PUSer, tSrcAddr.m_BlockInPU, 1);

                            NewAddr.m_PPN = INVALID_8F;
                            L2_UpdatePMT(&NewAddr, NULL, LPN);
#ifdef DBG_PMT
                            L2_UpdateDebugPMT(&NewAddr, LPN);
#endif
                        }
                    }
                                                        
                    ucLPNOffSet++;
                    if (ucLPNOffSet >= LPN_PER_SUPERBUF)
                    {
                        ucLPNOffSet = 0;
                        if (ucBuffOffset != (ucProgPageCnt - 1))
                        {
                            ucBuffOffset++;
                        }
                        else
                        {
                            ucBuffOffset = 0;
                            ucRemainLPNNum = ptCom->m_LastReadLen[ucSuperPu] - (LPN_PER_SUPERBUF - j);
                            break;
                        }
                    }
                } while (FALSE != L2_IsReadLocationContinuous(ucSuperPu, ((uc1stPageNum + ucBuffOffset) % TLC_BUF_CNT), ucLPNOffSet, ptCom));

                /*step2: process continuous data when buffer jump*/
                if (0 != ucRemainLPNNum)
                {
                    for (ucLPNOffSet = 0; ucLPNOffSet < ucRemainLPNNum; ucLPNOffSet++)
                    {
                        usCurTLCTargetPage = uc1stPageNum + ucProgPageCnt;
                        ucBufO = (usCurTLCTargetPage) % TLC_BUF_CNT;
                        LPN = ptCom->m_LPNInBuffer[ucSuperPu][ucBufO][ucLPNOffSet];
                        L2_LookupPMT(&tPMTAddr, LPN, FALSE);
                        tSrcAddr.m_PPN = g_TLCManager->m_TLCGCSrcAddr[ucSuperPu][usCurTLCTargetPage / (PG_PER_SLC_BLK - 1)][(usCurTLCTargetPage % (PG_PER_SLC_BLK - 1))*LPN_PER_SUPERBUF + ucLPNOffSet].m_PPN;

                        ptCom->m_LPNInBuffer[ucSuperPu][ucBufO][ucLPNOffSet] = INVALID_8F;

                        if (tSrcAddr.m_PPN == tPMTAddr.m_PPN)
                        {
                            if (TRUE == L2_LookupDirtyLpnMap(&tPMTAddr))
                            {
                                PhysicalAddr NewAddr = { 0 };

                                L2_UpdateDirtyLpnMap(&tSrcAddr, B_DIRTY);
                                L2_IncreaseDirty(tSrcAddr.m_PUSer, tSrcAddr.m_BlockInPU, 1);

                                NewAddr.m_PPN = INVALID_8F;
                                L2_UpdatePMT(&NewAddr, NULL, LPN);
#ifdef DBG_PMT
                                L2_UpdateDebugPMT(&NewAddr, LPN);
#endif
                            }
                        }
                    }
                    ucRemainLPNNum = 0;
                }
                g_GCManager[TLCGC_MODE]->tGCCommon.m_FlushStatus[ucSuperPu][ucRealBufNum][j] = SUBSYSTEM_STATUS_SUCCESS;
#endif
                DBG_Printf("L2_TLCGCCheckReadStatus: UECC spu=%d Buf %d LPN %d\n", ucSuperPu, i, j);
#endif
            }
            else if ((ucStatusAddr != SUBSYSTEM_STATUS_SUCCESS) && (ucStatusAddr != SUBSYSTEM_STATUS_RETRY_SUCCESS) && (ucStatusAddr != SUBSYSTEM_STATUS_RECC))
            {
                DBG_Printf("unknow TLCGC etype status=%d\n", ucStatusAddr);
            }
        }
    }
	
    return TRUE;
}

BOOL L2_GCCheckReadStatus(U8 ucSuperPu, U8 ucGCMode)
{
    U8 ucStatusAddr;
    U32 j;

    for (j = 0; j < LPN_PER_SUPERBUF; j++)
    {
        ucStatusAddr = g_GCManager[ucGCMode]->tGCCommon.m_FlushStatus[ucSuperPu][0][j];

        if ((ucStatusAddr != SUBSYSTEM_STATUS_SUCCESS) && (ucStatusAddr != SUBSYSTEM_STATUS_RETRY_SUCCESS) && (ucStatusAddr != SUBSYSTEM_STATUS_RECC))
        {
            return FALSE;
        }

        // Empty Page Error is invalid when GC Read.
        if (ucStatusAddr == SUBSYSTEM_STATUS_EMPTY_PG)
        {
            DBG_Getch();
        }
    }

    return TRUE;
}

BOOL L2_GCCheckWriteStatus(U8 ucSuperPu, U8 ucGCMode)
{
    U8* pFlushStatusAddr;
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pFlushStatusAddr = &g_GCManager[ucGCMode]->tGCCommon.m_GCWriteStatus[ucSuperPu][ucLunInSuperPu];
        if ((*pFlushStatusAddr) != SUBSYSTEM_STATUS_SUCCESS)
        {
            return FALSE;
        }
    }

    return TRUE;
}

U32 GCWritePage(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U32* pGCBufferPoiter)
{
    U16 i, j;
    U8* pFlushStatusAddr;
    RED Spare;
    PhysicalAddr Addr = { 0 };
    PhysicalAddr SrcAddr = { 0 };
    U32 LPN;
    U32 DirtyCnt = 0;
    static MCU12_VAR_ATTR BLOCK_TYPE ActualBlockType;
    U8 ucTLun;
    U8 ucLunInSuperPu;
    PhysicalAddr RefAddr = { 0 };
    U32* pGCBuffer;
    U8 ucTargetType = TARGET_HOST_GC;
    GCComStruct *ptCom;
    U8 ucGCBufferPointer;

    /*get GCComStruct*/
    ptCom = &g_GCManager[SLCGC_MODE]->tGCCommon;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        //1.check whether L3 is allowed to send Fcmd
        if (0 == (ulLunAllowToSendFcmdBitmap & (1 << ucTLun)))
        {
            continue;
        }

        //2.check TragetOffsetBitMap whether is allowed to send Fcmd
        if (TRUE == COM_BitMaskGet(g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType], ucLunInSuperPu) ||
            TRUE == COM_BitMaskGet(ptCom->m_SLCGCWrittenBitMap[ucSuperPu], ucLunInSuperPu))
        {
            continue;
        }

        DirtyCnt = 0;

        /* if host write the LPN or trim the LPN that has been copied during GC, then dirty it */
        for (j = (*pGCBufferPoiter)*LPN_PER_BUF; j < ((*pGCBufferPoiter) + 1)*LPN_PER_BUF; j++)
        {
            ucGCBufferPointer = ptCom->m_GCBufferPointer[ucSuperPu];
            LPN = ptCom->m_LPNInBuffer[ucSuperPu][ucGCBufferPointer][j];
            if (LPN == INVALID_8F)
            {
                DirtyCnt++;
                continue;
            }
            L2_LookupPMT(&RefAddr, LPN, FALSE);

            if (RefAddr.m_PPN != ptCom->m_AddrForBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]][j].m_PPN)
            {
                ptCom->m_LPNInBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]][j] = INVALID_8F;
                DirtyCnt++;
            }
        }
        /* if all dirty, no need to write */
        if (LPN_PER_BUF == DirtyCnt)
        {
            (*pGCBufferPoiter)++;
            if ((*pGCBufferPoiter) >= LUN_NUM_PER_SUPERPU)
            {
                return SUCCESS;
            }
            else
            {
                return FAIL;
            }
        }

        if (0 == g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType])
        {
            ucGCBufferPointer = ptCom->m_GCBufferPointer[ucSuperPu];
            Addr.m_PPN = L2_AllocateOnePage(ucSuperPu, ptCom->m_LPNInBuffer[ucSuperPu][ucGCBufferPointer], ucTargetType, &ActualBlockType);

            if (Addr.m_PPN == INVALID_8F)
            {
                return FAIL; //TODO:
            }
        }
        else
        {
            Addr.m_PPN = 0;
            Addr.m_PUSer = ucSuperPu;
            Addr.m_BlockInPU = g_PuInfo[ucSuperPu]->m_TargetBlk[ucTargetType];
            Addr.m_PageInBlock = g_PuInfo[ucSuperPu]->m_TargetPPO[ucTargetType] - 1;
            
            /* NormalShutdown ActualBlockType will be initialized again */
            ActualBlockType = ucTargetType + BLOCK_TYPE_SEQ;
        }
        Addr.m_OffsetInSuperPage = ucLunInSuperPu;

        /* update PMT before GC Write */
        for (j = (*pGCBufferPoiter)*LPN_PER_BUF; j < ((*pGCBufferPoiter) + 1)*LPN_PER_BUF; j++)
        {
            LPN = ptCom->m_LPNInBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]][j];

            if (INVALID_8F != LPN)
            {
                L2_UpdatePMT(&Addr, NULL, LPN);
                //FIRMWARE_LogInfo("GC LPN 0x%x -> Addr 0x%x TS %d\n", LPN, Addr.m_PPN, L2_GetTimeStampInPu(ucSuperPu));

                L2_UpdateDirtyLpnMap(&Addr, B_VALID);
                L2_IncreaseDirty(ucSuperPu, ptCom->m_SrcPBN[ucSuperPu], 1);

                SrcAddr.m_PPN = ptCom->m_AddrForBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]][j].m_PPN;
                L2_UpdateDirtyLpnMap(&SrcAddr, B_DIRTY);

#ifdef DBG_PMT
                /* add by henryluo 2012-09-10 for L2 PMT debug */
                L2_UpdateDebugPMT(&Addr, LPN);
#endif
            }
            else
            {
                /* if LPN == INVALID_8F, increase the target block dirty count */
                L2_IncreaseDirty(Addr.m_PUSer, Addr.m_BlockInPU, 1);
            }

            L2_IncreaseOffsetInAddr(&Addr);
        }

        Spare.m_RedComm.ulTimeStamp = L2_GetTimeStampInPu(ucSuperPu);
        Spare.m_RedComm.ulTargetOffsetTS = L2_GetTargetOffsetTS(&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType]);
        Spare.m_RedComm.bsVirBlockAddr = Addr.m_BlockInPU;
        Spare.m_RedComm.bcPageType = PAGE_TYPE_DATA;
        Spare.m_RedComm.bcBlockType = ActualBlockType;
        Spare.m_RedComm.eOPType = OP_TYPE_GC_WRITE;

        for (i = 0, j = (*pGCBufferPoiter)*LPN_PER_BUF; i < LPN_PER_BUF; i++, j++)
        {
            Spare.m_DataRed.aCurrLPN[i] = ptCom->m_LPNInBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]][j];
        }

        g_PuInfo[ucSuperPu]->m_TargetOffsetTS[ucTargetType][ucLunInSuperPu] = L2_GetTargetOffsetTS(&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType]);
        //Update RPMT
        L2_UpdateRPMT(ucSuperPu, ucLunInSuperPu, &ptCom->m_LPNInBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]][(*pGCBufferPoiter)*LPN_PER_BUF], ucTargetType, &Addr);

        pFlushStatusAddr = &ptCom->m_GCWriteStatus[ucSuperPu][ucLunInSuperPu];

        pGCBuffer = (U32 *)((U32)ptCom->m_GCBuffer[ucSuperPu][ptCom->m_GCBufferPointer[ucSuperPu]] + (*pGCBufferPoiter)*BUF_SIZE);

        /* update PBIT 1st page Info */
        if (0 == Addr.m_PageInBlock)
        {
            L2_Set_DataBlock_PBIT_Info(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, &Spare);
        }

        //FIRMWARE_LogInfo("GCWrite SPU %d Blk %d Pg %d LUN %d SuperPageTS %d LUNOrder %d\n", ucSuperPu, Addr.m_BlockInPU, Addr.m_PageInBlock, Addr.m_OffsetInSuperPage
        //    , Spare.m_RedComm.ulTimeStamp, Spare.m_RedComm.ulTargetOffsetTS);

        L2_FtlWriteLocal(&Addr, pGCBuffer, (U32*)&Spare, pFlushStatusAddr, FALSE, TRUE, NULL);

#ifdef L2MEASURE
        {
            if (TRUE == L2_VBT_Get_TLC(ucSuperPu, g_GCManager->tGCCommon.m_SrcPBN[ucSuperPu]))
            {
                L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_TLCGC_SLC);
            }
            else
            {
                L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_SLCGC_SLC);
            }
        }
#endif

        COM_BitMaskSet((U32*)&ptCom->m_SLCGCWrittenBitMap[ucSuperPu], ucLunInSuperPu);
        COM_BitMaskSet((U32*)&g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType], ucLunInSuperPu);

        if (SUPERPU_LUN_NUM_BITMSK == g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType])
        {
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
            L2_IncTimeStampInPu(ucSuperPu);
#endif
            g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[ucTargetType] = 0;
        }

        ulLunAllowToSendFcmdBitmap &= ~(1 << ucTLun);

        (*pGCBufferPoiter)++;

        if ((*pGCBufferPoiter) >= LUN_NUM_PER_SUPERPU)
        {
            return SUCCESS;
        }

    }//end while ucLunInSuperPu

    return SUCCESS;
}

U32 GCWriteSuperPage(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
    U32 bRet; 

    bRet = GCWritePage(ucSuperPu, ulLunAllowToSendFcmdBitmap, &l_aGCBufferPointer[ucSuperPu]);

    if (FAIL == bRet)
    {
        return FAIL;
    }

    if (l_aGCBufferPointer[ucSuperPu] >= LUN_NUM_PER_SUPERPU)
    {
        l_aGCBufferPointer[ucSuperPu] = 0;
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}

void L2_CheckTotalDirtyCnt(U8 ucSuperPu, GCComStruct *ptCom)
{
    U32 ulLpnPerBlk;
    U16 usPrefixCnt;
    U16 SrcVBN;
    U16 usSrcBlkIndex;

    if (VBT_TYPE_TLCW == ptCom->m_ucSrcBlkType[ucSuperPu])
    {
        ulLpnPerBlk = LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT;
        usPrefixCnt = PG_PER_WL;
    }
    else
    {
        ulLpnPerBlk = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;
        usPrefixCnt = 1;
    }

    SrcVBN = ptCom->m_SrcPBN[ucSuperPu];

    if (ulLpnPerBlk != L2_GetDirtyCnt(ucSuperPu, SrcVBN))
    {
#ifdef DirtyLPNCnt_IN_DSRAM1
        DBG_Printf("GC not done in PU %d, block %d, dirty cnt = %d\n", ucSuperPu, SrcVBN, *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + SrcVBN));
#else
        DBG_Printf("GC not done in PU %d, block %d, dirty cnt = %d\n", ucSuperPu, SrcVBN, pVBT[ucSuperPu]->m_VBT[SrcVBN].DirtyLPNCnt);
#endif

        /* Begin: check valid data after GC, add by henryluo 2012-12-25 */
        {
            U32 wlo;
            U32 LpnInPage;
            U32 OffsetInSuperPage;
            U32 ulLpn;
            PhysicalAddr TempAddr = { 0 }, RefAddr = { 0 };
            U32 ulCalcDirtyCnt = LPN_PER_BUF*usPrefixCnt;
            U16 usprefix;

            TempAddr.m_PPN = 0;
            TempAddr.m_PUSer = ucSuperPu;
            TempAddr.m_BlockInPU = SrcVBN;
            for (wlo = 0; wlo < PG_PER_SLC_BLK - 1; wlo++)
            {
                for (usprefix = 0; usprefix < usPrefixCnt; usprefix++)
                {
                    TempAddr.m_PageInBlock = ((wlo * usPrefixCnt) + usprefix);
                    ptCom->m_SrcPrefix[ucSuperPu] = usprefix;
                    for (OffsetInSuperPage = 0; OffsetInSuperPage < LUN_NUM_PER_SUPERPU; OffsetInSuperPage++)
                    {
                        TempAddr.m_OffsetInSuperPage = OffsetInSuperPage;
                        for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                        {
                            TempAddr.m_LPNInPage = LpnInPage;

                            if (usPrefixCnt == 1)
                            {
                                usSrcBlkIndex = 0;
                            }

                            ulLpn = LookupRPMTInGC(ucSuperPu, &TempAddr, (U8)usSrcBlkIndex, FALSE, ptCom);
                            if (INVALID_8F == ulLpn)
                            {
                                ulCalcDirtyCnt++;
                                continue;
                            }

                            /* lookup PMT to get reference physical addr from logic LPN */
                            L2_LookupPMT(&RefAddr, ulLpn, FALSE);

                            /* if equel, data is valid */
                            if (TempAddr.m_PPN == RefAddr.m_PPN)
                            {
                                DBG_Printf("SPU %d LUN %d Blk 0x%x Pg 0x%x LpnInPage %d is valid,ValidLPN 0x%x\n",
                                    ucSuperPu, RefAddr.m_OffsetInSuperPage, SrcVBN, wlo, LpnInPage, ulLpn);
                            }
                            else
                            {
                                ulCalcDirtyCnt++;
                            }
                        }
                    }
                }
            }
            DBG_Printf("ulCalcDirtyCnt %d\n", ulCalcDirtyCnt);
        }
        DBG_Getch();

        return;
    }
}


void ResetGCState(U8 ucSuperPu, GCComStruct *ptCom)
{
    U16 AccomplishCnt = 0;
    U16 PhyBlk = 0;
    U16 SrcPBN;

    /* check total dirty count of GC source block */
    L2_CheckTotalDirtyCnt(ucSuperPu, ptCom);

    SrcPBN = ptCom->m_SrcPBN[ucSuperPu];

    L2_PBIT_Set_Lock(ucSuperPu, SrcPBN, FALSE);
    //FIRMWARE_LogInfo("ResetGCState SuperPU %d BLK 0x%x into EreaseQue\n", ucSuperPu, SrcPBN);
    L2_InsertBlkIntoEraseQueue(ucSuperPu, SrcPBN, TRUE);


#ifdef DBG_LC
    if (SrcPBN != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu])
    {
        lc.uLockCounter[ucSuperPu][DATA_GC]--;
		//FIRMWARE_LogInfo("SuperPu %d ResetGCState --\n", ucSuperPu);
    }
#endif


    return;
}

// select the RPMT buffer based on the operation, both GC and TLC merge call this function
U32 LookupRPMTInGC(U8 ucSuperPu, PhysicalAddr* pAddr, U16 ucRPMTNum, BOOL bTLCWrite, GCComStruct *ptCom)
{
    U32 LPN;
    RPMT* pCurrRPMT;
    PhysicalAddr tAddr;

    tAddr.m_PPN = pAddr->m_PPN;

    if (TRUE == bTLCWrite)
    {
        // TLC merge
        pCurrRPMT = g_TLCManager->pRPMT[ucSuperPu][ucRPMTNum];
    }
    else
    {
#ifdef SIM
        if (ptCom->m_SrcPBN[ucSuperPu] != pAddr->m_BlockInPU)
        {
            DBG_Printf("GCType:%d, ptCom->m_SrcPBN != pAddr->m_BlockInPU\n", ptCom->m_ucSrcBlkType[ucSuperPu]);
            DBG_Getch();
        }
#endif
        pCurrRPMT = ptCom->m_pRPMT[ucSuperPu][ucRPMTNum];
        if (TRUE == L2_VBT_Get_TLC(ucSuperPu, tAddr.m_BlockInPU))
        {
            tAddr.m_PageInBlock = tAddr.m_PageInBlock % (PG_PER_SLC_BLK - 1);
        }

    }

    LPN = L2_LookupRPMT(pCurrRPMT, &tAddr);
    return LPN;
}



BOOL L2_ProcessGC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucThreadType)
{
    GCState* pState;
    BOOL bRet = FALSE;
    GCComStruct *ptCom;
    U32 uSrcBlk;

#ifdef SIM
    if (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_SLCGC_BIT))
    {
        DBG_Printf("SuperPU %d GC process TaskInfo 0x%x error\n", ucSuperPu, L2_FTLGetTaskBitInfo(ucSuperPu));
        DBG_Getch();
    }
#endif
    ptCom = &g_GCManager[SLCGC_MODE]->tGCCommon;

    pState = &(ptCom->m_GCStage[ucSuperPu]);
    uSrcBlk = ptCom->m_SrcPBN[ucSuperPu];

    switch (*pState)
    {
    case GC_STATE_PREPARE_GC:
#ifdef SIM
        if (INVALID_4F == ptCom->m_SrcPBN[ucSuperPu])
        {
            DBG_Printf("INVALID_4F == ptCom->m_SrcPBN[%d]\n", ucSuperPu);
            DBG_Getch();
        }
#endif

        l_aGCBufferPointer[ucSuperPu] = 0;
        ptCom->m_SrcWLO[ucSuperPu] = 0;
        ptCom->m_SrcPrefix[ucSuperPu] = 0;
        ptCom->m_SrcOffset[ucSuperPu] = 0;
        *pState = GC_STATE_LOAD_RPMT;

        //FIRMWARE_LogInfo("===GC start Pu %d SrcBlk %d dirtyCnt %d\n",ucSuperPu,uSrcBlk, pVBT[ucSuperPu]->m_VBT[uSrcBlk].DirtyLPNCnt);
        //break;

    case GC_STATE_LOAD_RPMT:
        if (GC_STATE_LOAD_RPMT == (*pState))
        {
            *pState = L2_GCLoadRPMT(ucSuperPu, ulLunAllowToSendFcmdBitmap, ptCom);
        }
        break;

    case GC_STATE_ERROR_HANDLING:
        if (TRUE == L2_ErrorHandlingEntry(ucSuperPu, TRUE, ptCom))
        {
            *pState = GC_STATE_COPY_VALID;
        }
        break;

    case GC_STATE_COPY_VALID:
        if (SUCCESS == CopyValidMultiLpn(ucSuperPu, ptCom, SLCGC_MODE))
        {
            if (ptCom->m_GCReadOffset[ucSuperPu] >= LPN_PER_SUPERBUF)
            {
                ptCom->m_GCReadOffset[ucSuperPu] -= LPN_PER_SUPERBUF;

                if (GC_STATE_COPY_VALID == *pState)
                {
                    *pState = GC_STATE_WAIT_READ_STATUS;
                }
            }
        }
        break;

    case GC_STATE_WAIT_READ_STATUS:
        if (TRUE == L2_GCCheckReadStatus(ucSuperPu, SLCGC_MODE))
        {
            *pState = GC_STATE_WRITE;
            ptCom->m_SLCGCWrittenBitMap[ucSuperPu] = 0;
        }
        break;

    case GC_STATE_WRITE:
        if (SUCCESS == GCWriteSuperPage(ucSuperPu, ulLunAllowToSendFcmdBitmap))
        {
            *pState = GC_STATE_WAIT_WRITE_STATUS;
        }
        break;

    case GC_STATE_WAIT_WRITE_STATUS:
        if (FALSE == L2_GCCheckWriteStatus(ucSuperPu, SLCGC_MODE))
        {
            return FALSE;
        }
        else
        {
            U16 usPagePerBlock = (PG_PER_SLC_BLK - 1);

            if (ptCom->m_SrcWLO[ucSuperPu] >= usPagePerBlock)
            {
                // Notice: ptCom->m_GCReadOffset[ucSuperPu] must be zero when GC_STAGE_DONE.
                if (0 == ptCom->m_GCReadOffset[ucSuperPu])
                {
                    *pState = GC_STATE_ERASE;
                }
                else
                {
                    //FIRMWARE_LogInfo("SPU %d SrcBlk 0x%x m_GCReadOffset %d\n", ucSuperPu, ptCom->m_SrcPBN[ucSuperPu], ptCom->m_GCReadOffset[ucSuperPu]);

                    U32 BufferPointer = ptCom->m_GCBufferPointer[ucSuperPu];
                    U32 TgtOffset = ptCom->m_GCReadOffset[ucSuperPu];
                    U32 i;

                    for (i = TgtOffset; i < LPN_PER_SUPERBUF; i++)
                    {
                        ptCom->m_LPNInBuffer[ucSuperPu][BufferPointer][i] = INVALID_8F;
                    }
                    ptCom->m_GCReadOffset[ucSuperPu] = 0; // Continue to write in the next loop.
                    ptCom->m_SLCGCWrittenBitMap[ucSuperPu] = 0;
                    *pState = GC_STATE_WRITE;
                }
            }
            else
            {
                //FIRMWARE_LogInfo("SPU %d SrcBlk 0x%x m_GCReadOffset %d GC_STATE_COPY_VALID\n", ucSuperPu, ptCom->m_SrcPBN[ucSuperPu], ptCom->m_GCReadOffset[ucSuperPu]);
                *pState = GC_STATE_COPY_VALID;
            }

            ptCom->m_GCBufferPointer[ucSuperPu] = (ptCom->m_GCBufferPointer[ucSuperPu] + 1) % GC_BUFFER_DEPTH;

            bRet = TRUE;
        }
        break;

    case GC_STATE_ERASE:
#ifdef L2MEASURE
         L2MeasureLogIncECTyepCnt(ucSuperPu, L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, ptCom->m_SrcPBN[ucSuperPu]), L2MEASURE_ERASE_SLCGC);
#endif
        ResetGCState(ucSuperPu, ptCom);
        L2_FTLTaskSLCGCClear(ucSuperPu);
        L2_FTLUsedThreadQuotaMax(ucSuperPu, ucThreadType);
        L2_InitGCManager(ucSuperPu, SLCGC_MODE);
        break;


    case GC_STATE_ALL:
    default:
        DBG_Getch();
        break;
    }

    return bRet;
}

BOOL L2_ProcessTLCGC(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap, U8 ucThreadType, U8 ucWriteType)
{
    GCState* pState;
    BOOL bRet = FALSE;
    GCComStruct *ptCom;
    U32 ucTLCGCSrcBlkIndex;
#ifndef SHUTDOWN_IMPROVEMENT_STAGE2
    COMMON_EVENT L2_Event;
#endif
    ptCom = &g_GCManager[TLCGC_MODE]->tGCCommon;
    pState = &(ptCom->m_GCStage[ucSuperPu]);

#ifdef SIM
    if (((FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCGC_BIT)) && (TLC_WRITE_TLCGC == ucWriteType)) 
        || ((FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)) && (TLC_WRITE_SWL == ucWriteType)))
    {
        DBG_Printf("SuperPU %d TLCGC/TLCSWL process TaskInfo 0x%x error\n", ucSuperPu, L2_FTLGetTaskBitInfo(ucSuperPu));
        DBG_Getch();
    }
#endif

#ifndef SHUTDOWN_IMPROVEMENT_STAGE2
    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);
    if (L2_Event.EventShutDown && ((GC_STATE_LOAD_RPMT == *pState) || (GC_STATE_COPY_VALID == *pState)))
    {
        DBG_Printf("Shutdown TLCGC PPO %d\n", g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_TLC_WRITE]);
        if (g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_TLC_WRITE] > 0)
        {
            *pState = GC_STATE_SHUTDOWN_TLCBLK_ERASE;
            if (0 != g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[TARGET_TLC_WRITE])
            {
                DBG_Printf("TLCGC TargetPPO %d BitMap 0x%x\n", g_PuInfo[ucSuperPu]->m_TargetPPO[TARGET_TLC_WRITE], g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[TARGET_TLC_WRITE]);
                DBG_Getch();
            }
        }
        else
        {
            *pState = GC_STATE_SHUTDOWN_WAIT_TLCBLK_ERASE;
        }
    }
#endif

    switch (*pState)
    {
    case GC_STATE_PREPARE_GC:
        g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] = 0;
        g_pTLCGCSrcBlkRecord->l_aTLC1stGCSrcBlk[ucSuperPu] = INVALID_4F;
        g_pTLCGCSrcBlkRecord->bIsSrcBlkAllValid[ucSuperPu] = FALSE;
        for (ucTLCGCSrcBlkIndex = 0; ucTLCGCSrcBlkIndex < TLCGC_SRCBLK_MAX; ucTLCGCSrcBlkIndex++)
        {
            g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ucTLCGCSrcBlkIndex] = INVALID_4F;
        }
        
        L2_ReadLocationClear(ucSuperPu, ptCom); 
        *pState = GC_STATE_SELECT_TLCGC_SRC;
        bRet = L2_SetTLCMergeInfo(ucSuperPu, VBT_TYPE_HOST, ucWriteType);
        break;

    case GC_STATE_SELECT_TLCGC_SRC:
        /* if TLCGC exhaust too many, over 1/4 of TLC blocks, then use dummy write to close target block */
        if (ptCom->m_TLCGCDummyWrite[ucSuperPu] == FALSE)
        {
            //check first src blk
            if (ucWriteType == TLC_WRITE_SWL)
            {
                SelectTLCGCBlock(ucSuperPu, VBT_TYPE_TLCW, TRUE);
            }
            else
            {
                SelectTLCGCBlock(ucSuperPu, VBT_TYPE_TLCW, FALSE);
            }
        }
        else
        {
            g_GCManager[TLCGC_MODE]->tGCCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
            *pState = GC_STATE_COPY_VALID;
        }
        break;

    case GC_STATE_LOAD_RPMT:
        if (GC_STATE_LOAD_RPMT == (*pState))
        {
            *pState = L2_GCLoadRPMT(ucSuperPu, ulLunAllowToSendFcmdBitmap, ptCom);
        }
        break;

    case GC_STATE_ERROR_HANDLING:
        if (TRUE == L2_ErrorHandlingEntry(ucSuperPu, TRUE, ptCom))
        {
            *pState = GC_STATE_COPY_VALID;
        }
        break;

    case GC_STATE_COPY_VALID:
        if (SUCCESS == CopyValidMultiLpn(ucSuperPu, ptCom, TLCGC_MODE))
        {
            if (ptCom->m_GCReadOffset[ucSuperPu] >= LPN_PER_SUPERBUF)
            {
                /* only for Low page, UpOfLowPage and extra page */
                ptCom->m_GCReadOffset[ucSuperPu] -= LPN_PER_SUPERBUF;

                g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu]++;
                if (g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] >= L2_GetProgPageCntByProgOrder(ucSuperPu))
                {
                    g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] = 0;
                    *pState = GC_STATE_WAIT_READ_STATUS;
                }   
            }
#ifndef FLASH_IM_3DTLC_GEN2
            else if (ptCom->m_GCUpOfExtraReadOffset[ucSuperPu] >= LPN_PER_SUPERBUF)
            {
                /* only for UpOfXtraPage */
#ifdef SIM
                if (ptCom->m_GCUpOfExtraReadOffset[ucSuperPu] > LPN_PER_SUPERBUF || g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] != 1)
                {
                    DBG_Getch();
                }
#endif
                ptCom->m_GCUpOfExtraReadOffset[ucSuperPu] = 0;
                g_TLCManager->m_TLCGCBufferCopyCnt[ucSuperPu] = 0;
                *pState = GC_STATE_WAIT_READ_STATUS;
            }
#endif //end of FLASH_IM_3DTLC_GEN2
        }     
        break;

    case GC_STATE_WAIT_READ_STATUS:
        if (TRUE == L2_TLCGCCheckReadStatus(ucSuperPu))
        {
            L2_ReadLocationClear(ucSuperPu, ptCom);
            *pState = GC_STATE_WRITE;
        }
        break;

    case GC_STATE_WRITE:
        if (TRUE == L2_TLCWriteTLC(ucSuperPu, ulLunAllowToSendFcmdBitmap))
        {
            *pState = GC_STATE_WAIT_WRITE_STATUS;
            bRet = TRUE;
        }
        break;
       
    case GC_STATE_WAIT_WRITE_STATUS:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {            
            if (g_TLCManager->ausTLCProgOrder[ucSuperPu] >= (PG_PER_SLC_BLK * PG_PER_WL))
            {
                if ((g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu] == 0) && (g_pTLCGCSrcBlkRecord->bIsSrcBlkAllValid[ucSuperPu] == TRUE))
                {
                    g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]] = ptCom->m_SrcPBN[ucSuperPu];
                    g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]++;
                }
                *pState = GC_STATE_ERASE;
            }
            else if (TLC_STAGE_READSRC == g_TLCManager->aeTLCStage[ucSuperPu])
            {
                /*RPMT needn't load for TLCGC, as it's prepared before*/
                if (g_TLCManager->ausTLCProgOrder[ucSuperPu] >= TLC_FIRST_RPMT_PAGE_NUM
#ifndef FLASH_IM_3DTLC_GEN2
                &&  g_TLCManager->ausTLCProgOrder[ucSuperPu] != (TLC_FIRST_RPMT_PAGE_NUM + 1)
#endif
                )
                {
                    *pState = GC_STATE_WRITE;
                }
                else
                {
                    *pState = GC_STATE_COPY_VALID;
                }
            }
            else if (TLC_STAGE_WRITETLC == g_TLCManager->aeTLCStage[ucSuperPu])
            {
                *pState = GC_STATE_WRITE;
            }
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            else if(TLC_STAGE_SHUTDOWN_SHARED_PAGE_CLOSED == g_TLCManager->aeTLCStage[ucSuperPu])
            {
                *pState =  GC_STATE_SHUTDOWN_SHARED_PAGE_CLOSED;
            }
#else//SHUTDOWN_IMPROVEMENT_STAGE1
            else if(TLC_STAGE_SHUTDOWN_TLCBLK_ERASE == g_TLCManager->aeTLCStage[ucSuperPu])
            {
                *pState = GC_STATE_SHUTDOWN_TLCBLK_ERASE;
            }
#endif
        }
        else
        {
            if (TLC_STAGE_ERRH_PROG == g_TLCManager->aeTLCStage[ucSuperPu])
            {
                *pState = GC_STATE_ERRH_PROG;
                DBG_Printf("SuperPU %d GC_STATE_ERRH_PROG blk=%d, page=%d\n", ucSuperPu, g_TLCManager->ausDesTLCBlk[ucSuperPu], g_TLCManager->ausTLCProgOrder[ucSuperPu]);
            }
        }
        break;

    case GC_STATE_ERRH_PROG:
        L2_TLCHandleProgFail(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        if (TLC_ERRH_ALL == g_TLCManager->aeErrHandleStage[ucSuperPu])
        {
            /*step1: release all TLCGC SrcBlk*/
            for (ucTLCGCSrcBlkIndex = 0; ucTLCGCSrcBlkIndex < g_pTLCGCSrcBlkRecord->m_TLCGCAllDirtySrcBlkCnt[ucSuperPu]; ucTLCGCSrcBlkIndex++)
            {
                L2_PBIT_Set_Lock(ucSuperPu, g_pTLCGCSrcBlkRecord->l_aTLCGCSrcBlk[ucSuperPu][ucTLCGCSrcBlkIndex], FALSE);
            }
            L2_PBIT_Set_Lock(ucSuperPu, ptCom->m_SrcPBN[ucSuperPu], FALSE);

            //if SWL DstBuf is not 4F, then continue, or finish this SWL process
            if (ucWriteType == TLC_WRITE_SWL)
            {
                L2_FTLTaskTLCSWLClear(ucSuperPu);
            }
            else
            {
                /* step2: terminate TLCGC task */
                L2_FTLTaskTLCGCClear(ucSuperPu);
                L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_TLCGC);
            }
            L2_InitGCManager(ucSuperPu, TLCGC_MODE);

            L2_ResetTLCManager(ucSuperPu);

        }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case GC_STATE_SHUTDOWN_SHARED_PAGE_CLOSED:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
            L2_TLCSharedPageClosedReset(ucSuperPu);
            g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt++;
            //*pState = GC_STATE_ALL;
            bRet = TRUE;
        }
        break;
#else//SHUTDOWN_IMPROVEMENT_STAGE1
    case GC_STATE_SHUTDOWN_TLCBLK_ERASE:
        if (TRUE == L2_TLCCheckWriteStatus(ucSuperPu))
        {
            L2_EraseTLCWriteBlk(ucSuperPu, ulLunAllowToSendFcmdBitmap);
        }
        break;

    case GC_STATE_SHUTDOWN_WAIT_TLCBLK_ERASE:
        if (TRUE == L2_TLCCheckEraseStatus(ucSuperPu))
        {
            L2_TLCShutdownReset(ucSuperPu);
            L2_InitGCManager(ucSuperPu, TLCGC_MODE);
            g_L2EventStatus.m_ShutdownEraseTLCBlkDoneCnt++;
            bRet = TRUE;
        }
        break;
#endif

    case GC_STATE_ERASE:
#ifdef L2_HANDLE_UECC
        if (TRUE == L2_NeedHandleUECC(ucSuperPu))
        {
            *pState = GC_STAGE_ERRH_UECC;
            g_TLCManager->aeErrHandleUECCStage[ucSuperPu] = TLC_ERRH_ALLOCATE_NEWBLK;
            break;
        }
#endif
        if (TRUE == L2_TLCReset(ucSuperPu))
        {

            if (ucWriteType == TLC_WRITE_SWL)
            {
                L2_FTLTaskTLCSWLClear(ucSuperPu);
#if defined(SWL_EVALUATOR)
                SWLRecordIncDoneTime(ucSuperPu);
#endif
            }
            else
            {
                L2_FTLTaskTLCGCClear(ucSuperPu);
                L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_TLCGC);                
            //FIRMWARE_LogInfo("SuperPU %d TLCGCDone\n", ucSuperPu);
#if defined(SWL_EVALUATOR)
                SWLRecordTLCGCCnt(ucSuperPu);
#endif
            }
            L2_InitGCManager(ucSuperPu, TLCGC_MODE);

        }
        break;

#ifdef L2_HANDLE_UECC
    case GC_STAGE_ERRH_UECC:
        L2_HandleUECC(ucSuperPu, ulLunAllowToSendFcmdBitmap);

        if (TLC_ERRH_ALL == g_TLCManager->aeErrHandleUECCStage[ucSuperPu])
        {
            *pState = GC_STATE_ERASE;
        }
        break;
#endif

    case GC_STATE_ALL:
    default:
        DBG_Getch();
        break;
    }

    return bRet;
}



