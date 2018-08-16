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
Filename    :L2_DbgMain.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.04.24
Description :Main function for Boot Entry, and regular Boot.
Others      :
Modify      :
****************************************************************************/
#include "HAL_Inc.h"
#include "L2_Boot.h"
#include "L2_PMTManager.h"
#include "L2_Shutdown.h"
#include "L2_PMTI.h"
#include "L2_Debug.h"
#include "L2_Thread.h"
#include "L2_GCManager.h"
#include "L2_Schedule.h"
#include "L2_RT.h"
#include "L2_Interface.h"
#include "L2_FCMDQ.h"
#include "L2_TLCMerge.h"
#include "L2_Defines.h" 
#include "L2_TableBBT.h"
#include "L2_FullRecovery.h"
#include "COM_Memory.h"
#include "HAL_TraceLog.h"
#include "FW_Event.h"
#include "COM_BitMask.h"
#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif

#ifdef ASIC
#include <xtensa/tie/xt_timer.h>
#include <xtensa/hal.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_timer.h>
#include <xtensa/tie/xt_core.h>
#endif
#ifdef SIM
#include "L2_SimTablecheck.h"
#include "L2_SimRebuildTable.h"
#include "sim_flash_common.h"
extern BOOL g_aRebuildDirtyCntFlag[];
#endif

extern PAIR_PAGE_TYPE HAL_GetFlashPairPageType(U16 usPage);
extern void L1_MapLCTWriteCnt(U32 *pFreeDramBase);
extern void L1_ResetLCTWriteCnt(void);
extern U16 MCU12_DRAM_TEXT L2_ErrHReplaceBLK(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, BOOL bTLCBlk, U8 ucErrType);
extern MCU12_VAR_ATTR U32 g_RebuildDptrBaseAddr;
extern GLOBAL U32 g_ulShutdownFlushPageCnt[SUBSYSTEM_SUPERPU_MAX];

//specify file name for Trace Log
#define TL_FILE_NUM  L2_Boot_c

GLOBAL  RebuildDptr* l_RebuildDptr;
GLOBAL  RebuildPMTI* l_RebuildPMTIDptr;
GLOBAL  RebuildPMTManager* l_RebuildPMTManagerDptr;
GLOBAL  RebuildPMT* l_RebuildPMTDptr;
GLOBAL  RebuildErrTLC* l_RebuildErrTLCDptr;
GLOBAL  BufferManager* l_BufferManagerDptr;
GLOBAL  Err_Info_Manager* l_ErrManagerDptr;
GLOBAL  RebuildErrHandleDptr* l_ErrHandleDptr;
GLOBAL  RebuildCalcDirtyCntDptr* l_CalcDirtyCntDptr;
GLOBAL  BOOL g_RebuildDirtyFlag;
GLOBAL  BOOL g_BootUpOk;
GLOBAL  U32  g_RebuildPMTDone;
GLOBAL  U32  g_RebuildDCPercent = 0;
GLOBAL  U32  g_ulAllowHostWriteSec = 0;

#ifdef SIM
extern void NFC_ModelSchedule();
#endif

RebuildState MCU1_DRAM_TEXT L2_GetRebuildState()
{
    return l_RebuildDptr->m_State;
}

void MCU1_DRAM_TEXT L2_SetRebuildState(RebuildState State)
{
    l_RebuildDptr->m_State = State;
}

void MCU1_DRAM_TEXT L2_RebuildSetDRAMForPMTBuffer(U16 PUSer, U16 RebuildBufferSN, U32* pBufferAddr)
{
#ifdef PMT_ITEM_SIZE_REDUCE
    l_BufferManagerDptr->m_PageBuffer[PUSer][RebuildBufferSN] = (U32*)(*pBufferAddr);
    COM_MemIncBaseAddr(pBufferAddr, COM_MemSize16DWAlign(2 * sizeof(SuperPage)));
#else
    l_BufferManagerDptr->m_PageBuffer[PUSer][RebuildBufferSN] = (PMTPage*)(*pBufferAddr);
    COM_MemIncBaseAddr(pBufferAddr, COM_MemSize16DWAlign(2 * sizeof(PMTPage)));
#endif
}

void MCU1_DRAM_TEXT L2_RebuildSetDRAMForSpareBuffer(U16 ucSuperPU, U16 RebuildBufferSN, U32* pBufferAddr)
{
    U16 ucLUNInSuperPU;
    U8 ucTarget;

    for(ucTarget = 0; ucTarget < TARGET_ALL -1; ucTarget++)
    {
        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
#ifdef DCACHE
            l_BufferManagerDptr->m_SpareBuffer[ucSuperPU][ucTarget][RebuildBufferSN][ucLUNInSuperPU] = (RED*)(*pBufferAddr + DRAM_HIGH_ADDR_OFFSET);
#else
            l_BufferManagerDptr->m_SpareBuffer[ucSuperPU][ucTarget][RebuildBufferSN][ucLUNInSuperPU] = (RED*)(*pBufferAddr);
#endif

            COM_MemIncBaseAddr(pBufferAddr, COM_MemSize16DWAlign(sizeof(RED)));
        }
    }
}


void MCU1_DRAM_TEXT L2_InitRebuildDptr()
{
    l_RebuildPMTIDptr = &(l_RebuildDptr->m_RebuildPMTI);
    l_RebuildPMTManagerDptr = &(l_RebuildDptr->m_RebuildPMTManager);
    l_RebuildPMTDptr = &(l_RebuildDptr->m_RebuildPMT);
    l_RebuildErrTLCDptr = &(l_RebuildDptr->m_RebuildErrTLC);
    l_BufferManagerDptr = &(l_RebuildDptr->m_BufManager);
    l_ErrManagerDptr = &(l_RebuildDptr->m_ErrManager);
    l_ErrHandleDptr = &(l_RebuildDptr->m_ErrHanler);
    l_CalcDirtyCntDptr = &(l_RebuildDptr->m_CalcDirtyCntDptr);
}

void  MCU1_DRAM_TEXT L2_InitRebuild()
{
    U32 i, j, k;
    U32 uLUNInSuperPU;
    U32 SuperPU;
    U32 ulPMTIndexInPu;

    //Init PMTDirtyBitMapInCE
#ifdef PMT_ITEM_SIZE_REDUCE
    COM_MemZero((U32*)&(l_RebuildPMTIDptr->m_RebuildPMTDirtyBitMapInCE[0].m_DirtyBitMap), PMT_DIRTY_BM_SIZE_MAX * SUBSYSTEM_SUPERPU_NUM);
#else
    COM_MemZero((U32*)&(l_RebuildPMTIDptr->m_RebuildPMTDirtyBitMapInCE[0].m_DirtyBitMap), PMT_DIRTY_BM_SIZE * SUBSYSTEM_SUPERPU_NUM);
#endif

    L2_SetRebuildState(Rebuild_State_Init);

    //Init RebuildPMTI
    l_RebuildPMTIDptr->m_PMTIState = Rebuild_PMTI_Prepare;
    l_RebuildPMTIDptr->m_CurrBufferPos = 0;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        l_RebuildPMTIDptr->m_RebuildTableBlockSN[i] = 0;
        l_RebuildPMTIDptr->m_RebuildTableBlkPPO[i] = 0;
        l_RebuildPMTIDptr->m_RebuildPMTIAccomplish[i] = FALSE;

        //Init Max PMTTS
        l_RebuildPMTIDptr->m_MaxTableTs[i] = 0;
        l_RebuildPMTIDptr->m_PMTIndexofMaxTableTS[i] = INVALID_8F;

        //Init MIN PMTTS
        l_RebuildPMTIDptr->m_MinTableTs[i] = INVALID_8F;
        l_RebuildPMTIDptr->m_PMTIndexofMinTableTS[i] = INVALID_8F;

        //Dirty PMTIndex num of PU
        l_RebuildPMTIDptr->m_bExistDirtyPMTPageInPU[i] = FALSE;

        for (j = 0; j < RebuildBufferCntInPU; j++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                l_RebuildPMTIDptr->m_TableAddrOfBuffer[i][j][uLUNInSuperPU].m_PPN = INVALID_8F;
            }
        }

        l_RebuildPMTIDptr->m_RebuildClipTSInPu[i] = INVALID_8F;
    }

    for (SuperPU = 0; SuperPU < SUBSYSTEM_SUPERPU_NUM; SuperPU++)
    {
        for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_PU; ulPMTIndexInPu++)
        {
            l_RebuildPMTIDptr->m_PMTPageTS[SuperPU][ulPMTIndexInPu] = 0;
            l_RebuildPMTIDptr->m_PMTPageNoDirtyMaxTS[SuperPU][ulPMTIndexInPu] = 0;

        }
    }

    //Init RebuildPMTManager
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < AT1_BLOCK_COUNT; j++)
        {
            l_RebuildPMTManagerDptr->m_TableBlkPPO[i][j] = 0;
            l_RebuildPMTManagerDptr->m_TableBlkTS[i][j] = 0;
        }
        for (j = 0; j < AT1_BLOCK_COUNT; j++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                l_RebuildPMTManagerDptr->m_TableBlk[i][j][uLUNInSuperPU] = INVALID_8F;
                l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[i][j][uLUNInSuperPU] = INVALID_8F;
                l_RebuildPMTManagerDptr->m_TableBlklastPagePMTI[i][j][uLUNInSuperPU] = INVALID_8F;
            }
        }
    }
    //Init RebuildPU
    l_RebuildPMTDptr->m_RebuildPuState = Rebuild_Pu_Prepare;

    //Init RebuildPMT
    l_RebuildPMTDptr->m_PMTState = Rebuild_PMT_LoadPMT;

    for (SuperPU = 0; SuperPU < SUBSYSTEM_SUPERPU_NUM; SuperPU++)
    {
        l_RebuildPMTDptr->m_LoadedPMTIIndex[SuperPU] = 0;
    }

    for (SuperPU = 0; SuperPU < SUBSYSTEM_SUPERPU_NUM; SuperPU++)
    {
        l_RebuildPMTDptr->m_MaxTSOfPU[SuperPU] = 0;
        l_RebuildPMTDptr->m_RebuildPMTStage[SuperPU] = REBUILD_PMT_STAGE_DONE;

        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            l_RebuildErrTLCDptr->m_ErrTLCBlkCnt[SuperPU][uLUNInSuperPU] = 0;
        }

        for (i = 0; i < TARGET_ALL; i++)
        {
            l_RebuildPMTDptr->m_CurrRebuildBlkSN[SuperPU][i] = 0;
            l_RebuildPMTDptr->m_CurrRebuildPPO[SuperPU][i] = 0;
            l_RebuildPMTDptr->m_TargetFinished[SuperPU][i] = TRUE;
            l_RebuildPMTDptr->m_RebuildBlkCnt[SuperPU][i] = 0;
        }

        for (i = 0; i < TARGET_ALL; i++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                l_RebuildPMTDptr->m_NeedLoadRPMT[SuperPU][i][uLUNInSuperPU] = FALSE;
                l_RebuildPMTDptr->m_LoadSparePPO[SuperPU][i][uLUNInSuperPU] = 0;
            }
        }

        l_RebuildPMTDptr->m_RebuildFullfillPartialPMTPPOState[SuperPU] = 0;
    }
    
    //Init ErrTLCBlk   
    for (SuperPU = 0; SuperPU < SUBSYSTEM_SUPERPU_NUM; SuperPU++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            l_RebuildErrTLCDptr->m_RebuildErrTLCState[SuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_LoadTLCPage;
            l_RebuildErrTLCDptr->m_ErrTLCBlkCnt[SuperPU][uLUNInSuperPU] = 0;
            l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[SuperPU][uLUNInSuperPU] = 0;
            l_RebuildErrTLCDptr->m_CurErrTLCPageSN[SuperPU][uLUNInSuperPU] = 0;
            l_RebuildErrTLCDptr->m_CurLoadDestBlkSN[SuperPU][uLUNInSuperPU] = 0;
        }
    }

    //Init BufManager
    for (i = 0; i < RebuildBufferCntInPU; i++)
    {
        l_BufferManagerDptr->m_BufferUsed[i] = FALSE;
    }

    for (k = 0; k < LUN_NUM_PER_SUPERPU; k++)
    {
        for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
        {
            for (j = 0; j < RebuildBufferCntInPU; j++)
            {
                l_BufferManagerDptr->m_BufferStatus[i][j][k] = SUBSYSTEM_STATUS_SUCCESS;
            }
        }
    }

    //Init BufStatus
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < PG_PER_WL; j++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                l_BufferManagerDptr->m_TLCPageBufferStatus[i][j][uLUNInSuperPU] = SUBSYSTEM_STATUS_SUCCESS;
            }
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < PG_PER_SLC_BLK; j++)
        {
            for (k = 0; k < PG_PER_WL; k++)
            {
                for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    l_BufferManagerDptr->m_TLCSpareBufferStatus[i][j][k][uLUNInSuperPU] = SUBSYSTEM_STATUS_SUCCESS;
                }
            }
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            l_BufferManagerDptr->m_TLCCurWordLineOffset[i][uLUNInSuperPU] = 0;
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        l_BufferManagerDptr->m_LoadRPMTBitMap[i] = 0;
    }

    //Init Err manager of rebuild
    L2_InitErrManagerOfRebuild();

    //Init Err Handler of Rebuild
    L2_InitErrHandleOfRebuild();

    //Init CalcDirtyCnt of Rebuild
    L2_InitCalcDirtyCntOfRebuild();
    g_RebuildDirtyFlag = FALSE;
    g_RebuildPMTDone = FALSE;
    g_RebuildDCPercent = 0;
    g_ulAllowHostWriteSec = L1_WRITE_CACHE_BUFFER * SEC_PER_BUF;
}

BOOL MCU1_DRAM_TEXT IsPMTISpareLoadFinish()
{
    U32 ulSuperPU;
    U32 uSuperBlockSN;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        uSuperBlockSN = l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU];
        if (uSuperBlockSN < AT1_BLOCK_COUNT)
        {
            break;
        }
    }

    if(ulSuperPU >= SUBSYSTEM_SUPERPU_NUM)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

U32 MCU1_DRAM_TEXT L2_RebuildCheckSuperPageReadStatus(U32 ulSuperPU, U32 ulBaseAddr, U32 ulPos)
{
    U32 ulLUNInSuperPU;
    U8 ucEmptyCnt = 0;
    U8 ucUECCLunCnt = 0;
    U8 *pPageStatusBaseAddr;
    
    for (ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
    {
        pPageStatusBaseAddr = (U8 *)(ulBaseAddr + 
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU+
                            sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*ulLUNInSuperPU);
        //DBG_Printf("ppo 0x%x check status addr 0x%x\n",ulPos, (U32)&pPageStatusBaseAddr[ulPos]);
        if(SUBSYSTEM_STATUS_PENDING == pPageStatusBaseAddr[ulPos])
        {
            return SUBSYSTEM_STATUS_PENDING;
        }
        else if(SUBSYSTEM_STATUS_EMPTY_PG == pPageStatusBaseAddr[ulPos])
        {
            ucEmptyCnt++;        
        }
        else if(SUBSYSTEM_STATUS_FAIL == pPageStatusBaseAddr[ulPos])
        {
            ucUECCLunCnt++;
        }
    }

    if(0 != ucUECCLunCnt)
    {
        return SUBSYSTEM_STATUS_FAIL;
    }

    return (0 != ucEmptyCnt)? SUBSYSTEM_STATUS_EMPTY_PG : SUBSYSTEM_STATUS_SUCCESS;    
}
//return value: TRUE if all the PMT Spare has been loaded
BOOL MCU1_DRAM_TEXT LoadPMTSpareToBuffer()
{
    U32 uLUNInSuperPU;
    U32 ulSuperPU;
    U32 AccomplishCnt = 0;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pBufferAddr;
#else
    PMTPage* pBufferAddr;
#endif
    U32 *pBuffer;
    U8*  pStatus;
    RED* pSpare;

    RED *pPMTSpareBaseAddr;
    U8 *pPMTSpareStatusBaseAddr;
    U32 ulStatusTemp;

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    // not send load cmd finish
    while( !IsPMTISpareLoadFinish())
    {
        // no space for render command
        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            while (TRUE)
            {
                if(l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] >= AT1_BLOCK_COUNT*PG_PER_SLC_BLK)
                {
                    break;
                }
                ulStatusTemp = L2_RebuildCheckSuperPageReadStatus(ulSuperPU,
                    (U32)g_PMTManager->m_pPMTPage[ulSuperPU][0],
                    l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU]);
                //FIRMWARE_LogInfo("next read pos %d ; check pos %d ",
                //        l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU]*PG_PER_SLC_BLK + l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU],
                //        l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU]);
                if (SUBSYSTEM_STATUS_PENDING == ulStatusTemp)
                {
                    //FIRMWARE_LogInfo("pending bypass check\n");
                    break;                    
                }
                else
                {    
                    U8 ucBlockIndex = l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] / PG_PER_SLC_BLK;
                    
                    /* encounter empty page, then not load successive pages of this block */
                    /* or UECC cnt have over erase threshold */
                    if (SUBSYSTEM_STATUS_EMPTY_PG == ulStatusTemp || l_RebuildPMTIDptr->m_UECCCnt[ulSuperPU][ucBlockIndex] >= SLC_UECCCNT_ERASE_THRESHOLD)
                    {                        
                        /* need jump to load next block */
                        if (ucBlockIndex == l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU])
                        {
                            l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] += PG_PER_SLC_BLK - l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] % PG_PER_SLC_BLK;
                            l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU]++;
                            l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;                            
                        }
                        else if (ucBlockIndex < l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU])
                        {
                            l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] += PG_PER_SLC_BLK - l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] % PG_PER_SLC_BLK;
                        }
                        //FIRMWARE_LogInfo("empty adjust pos to %d\n",l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU]);
                    }
                    else
                    {
                        /* statistic UECC cnt of each AT1 block */
                        if(SUBSYSTEM_STATUS_FAIL == ulStatusTemp)
                        {
                            l_RebuildPMTIDptr->m_UECCCnt[ulSuperPU][ucBlockIndex]++;
                        }
                        
                        l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU]++;
                        //FIRMWARE_LogInfo("not empty add 1\n");
                    }
                }
            }

            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if(FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulSuperPU, uLUNInSuperPU)))
                {
                    return FALSE;
                }
            }
        }

        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            PhysicalAddr Addr;
            PhysicalAddr* pAddr = &Addr;
            U32 BlockInCE;
            U32 RebuildTableBlkPPO;
            U32 RebuildTableBlkSN;            
            if (l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] >= AT1_BLOCK_COUNT)
            {
                continue;
            }

            RebuildTableBlkPPO = l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU];
            RebuildTableBlkSN = l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU];
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {                
                pPMTSpareBaseAddr = (RED *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);
                pPMTSpareStatusBaseAddr = (U8 *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+ 
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU+
                            sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);
            
                BlockInCE = l_RebuildPMTManagerDptr->m_TableBlk[ulSuperPU][RebuildTableBlkSN][uLUNInSuperPU];

                pAddr->m_PUSer = ulSuperPU;
                pAddr->m_OffsetInSuperPage = uLUNInSuperPU;
                pAddr->m_BlockInPU = BlockInCE;
                pAddr->m_PageInBlock = RebuildTableBlkPPO;
                pAddr->m_LPNInPage = 0;

                pBufferAddr = l_BufferManagerDptr->m_PageBuffer[ulSuperPU][0];
#ifdef PMT_ITEM_SIZE_REDUCE
                pBuffer = (U32*)&((SuperPage *)pBufferAddr)->m_Content[uLUNInSuperPU];
#else
                pBuffer = (U32 *)&pBufferAddr->m_Page.m_Content[uLUNInSuperPU];
#endif
                pStatus = &pPMTSpareStatusBaseAddr[RebuildTableBlkSN*PG_PER_SLC_BLK+RebuildTableBlkPPO];
                pSpare = &pPMTSpareBaseAddr[RebuildTableBlkSN*PG_PER_SLC_BLK+RebuildTableBlkPPO];
                *pStatus = SUBSYSTEM_STATUS_PENDING;

                /* record the last read addr, for status check */
                l_RebuildPMTIDptr->m_LastReadPos[ulSuperPU] = RebuildTableBlkSN*PG_PER_SLC_BLK + RebuildTableBlkPPO;

                /*To patch for rebuild PMTI read redundant success,but rebuild PMT read whole page UECC*/
                if (g_bPMTErrFlag == TRUE)
                {
                    L2_FtlReadLocal((U32*)pBuffer, pAddr, pStatus, (U32*)pSpare, LPN_PER_BUF, 0, TRUE, TRUE);
                }
                else
                {
                    L2_FtlReadLocal((U32*)pBuffer, pAddr, pStatus, (U32*)pSpare, 0, 0, TRUE, TRUE);
                }
            }
            
            RebuildTableBlkPPO++;
            if (RebuildTableBlkPPO >= PG_PER_SLC_BLK)
            {
                RebuildTableBlkPPO = 0;
                RebuildTableBlkSN++;
                l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] = RebuildTableBlkSN;
            }
            l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = RebuildTableBlkPPO;
        }
    }

    //wait all finish
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        if (l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] < AT1_BLOCK_COUNT)
        {
            return FALSE;
        }

        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {            
            pPMTSpareStatusBaseAddr = (U8 *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+ 
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU+
                            sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);
            if (SUBSYSTEM_STATUS_PENDING == 
                pPMTSpareStatusBaseAddr[l_RebuildPMTIDptr->m_LastReadPos[ulSuperPU]])
            {
                return FALSE;
            }
        }
    }
    
    // already load finish.
    return TRUE;            
}

BOOL MCU1_DRAM_TEXT L2_RebuildPMTICheckUECCCnt()
{
    U32 uLUNInSuperPU;
    U32 ulSuperPU;

    U32 ulPos;
    //U32 BlockInCE;
    U32 RebuildTableBlkSN;
    
    while(!IsPMTISpareLoadFinish())
    {
        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {                        
            U32 LoadPageStatus;
            U8 ucEmptyPageLUNNum = 0;
        
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {                                                
                U8 *pPMTSpareStatusBaseAddr = (U8 *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+ 
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU+
                            sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);
            
                RebuildTableBlkSN = l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU];
                //BlockInCE = l_RebuildPMTManagerDptr->m_TableBlk[ulSuperPU][RebuildTableBlkSN][uLUNInSuperPU];
                
                ulPos = RebuildTableBlkSN*PG_PER_SLC_BLK+l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU];                
                LoadPageStatus = pPMTSpareStatusBaseAddr[ulPos];

                /* statistic UECC cnt */
                if (SUBSYSTEM_STATUS_FAIL == LoadPageStatus)
                {
                    l_RebuildPMTIDptr->m_UECCCnt[ulSuperPU][RebuildTableBlkSN]++;
                    break;
                }
                else if(SUBSYSTEM_STATUS_EMPTY_PG == LoadPageStatus)
                {
                    ucEmptyPageLUNNum++;    
                }
                
            }

            if(LUN_NUM_PER_SUPERPU == ucEmptyPageLUNNum)
            {
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = PG_PER_SLC_BLK - 1;    
            }
            l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU]++;
            if (l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] >= PG_PER_SLC_BLK)
            {
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;
                l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU]++;                
            }
        }
                
    }

    return TRUE;
}


BOOL L2_IsPMTDirtyInBitMap(U32 PUSer, U32 PMTIndexInCE, U32* p_DirtyBitMap)
{
    U32 index = PMTIndexInCE / 32;
    U32 bit = PMTIndexInCE % 32;

    return (p_DirtyBitMap[index] & (1 << bit));
}


//Update No_Dirty_PMT_TS by pSpare->m_DirtyBitMap & pSpare->TimeStamp
void L2_UpdatePMTPageNoDirtyTS(U32 ulSuperPU, U32 PMT_TS, U32* p_DirtyBitMap)
{
    U32 PMTIndexInPu;
    U32 OldTS;

    for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; PMTIndexInPu++)
    {
        OldTS = l_RebuildPMTIDptr->m_PMTPageNoDirtyMaxTS[ulSuperPU][PMTIndexInPu];
        if (OldTS > PMT_TS)
        {
            continue;
        }

        if (FALSE == L2_IsPMTDirtyInBitMap(ulSuperPU, PMTIndexInPu, p_DirtyBitMap))
        {
            l_RebuildPMTIDptr->m_PMTPageNoDirtyMaxTS[ulSuperPU][PMTIndexInPu] = PMT_TS;
        }
    }

    return;
}

BOOL L2_RebuildPMTIEraseTooManyUECCBlocks()
{
    U32 ulSuperPU;
    U32 uLUNInSuperPU;
    U8 ucBlockIndex;

    while( !IsPMTISpareLoadFinish())
    {
        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            U32 ulPos;            
            
            ucBlockIndex = l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU];
            if(TRUE == l_RebuildPMTIDptr->m_NeedErase[ulSuperPU][ucBlockIndex])
            {
                for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    pTBRebManagement->usBlockErase[ulSuperPU][uLUNInSuperPU] = 
                        l_RebuildPMTManagerDptr->m_TableBlk[ulSuperPU][ucBlockIndex][uLUNInSuperPU];
                }
                if(L2_TB_SUCCESS == L2_TB_Rebuild_TableBlockErase(ulSuperPU))
                {
                    /* update the spare status of this block's first page to empty */
                    for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                    {                                                
                        U8 *pPMTSpareStatusBaseAddr = (U8 *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+ 
                                    sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU+
                                    sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);                 
                        
                        ulPos = ucBlockIndex*PG_PER_SLC_BLK+l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU];                
                        pPMTSpareStatusBaseAddr[ulPos] = SUBSYSTEM_STATUS_EMPTY_PG;
                    }
#ifdef SIM                    
                    dbg_p_m_PMTBlkManager[ulSuperPU]->m_FreePagesCnt += PG_PER_SLC_BLK;
                    dbg_p_m_PMTBlkManager[ulSuperPU]->m_PMTBlkInfo[ucBlockIndex].m_bFree = TRUE;
                    dbg_p_m_PMTBlkManager[ulSuperPU]->m_PMTBlkInfo[ucBlockIndex].m_DirtyCnt = 0;
#endif
                    l_RebuildPMTIDptr->m_NeedErase[ulSuperPU][ucBlockIndex] = FALSE;
                    DBG_Printf("[%s] erase AT1 block index:%d\n",__FUNCTION__,ucBlockIndex);
                }
                else
                {
                    return FALSE;
                }
            }

            /* the block need time need check */
            l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU]++;
        }        
    }
    return TRUE;    
}


BOOL MCU1_DRAM_TEXT UpdatePMTIBySpare()
{
    U32 uLUNInSuperPU;
    U32 ulSuperPU;
    U32 MaxPMTTS;
    U32 TableBlkTS;
    PhysicalAddr realAddr[LUN_NUM_PER_SUPERPU];
    PhysicalAddr* pAddr[LUN_NUM_PER_SUPERPU];
    
    RED* pSpare[LUN_NUM_PER_SUPERPU];
    U32 ulPos;
    U32 BlockInCE;
    U32 RebuildTableBlkSN;
    RED *pPMTSpareBaseAddr;
    U8 *pPMTSpareStatusBaseAddr;
    
    while(!IsPMTISpareLoadFinish())
    {
        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            U32 TS;
            //U32 DataTS;
            U32 uPMTIndexInPu;            
            U32 LoadPageStatus;
            BOOL bNormal = TRUE;
            U32 *pDirtyBitmap;
            U8 ucEmptyPageLUNNum = 0;
 
            if (l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] >= AT1_BLOCK_COUNT)
            {
                continue;
            }

            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {                                
                pPMTSpareBaseAddr = (RED *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);
                pPMTSpareStatusBaseAddr = (U8 *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]+ 
                            sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU+
                            sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*uLUNInSuperPU);
            
                RebuildTableBlkSN = l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU];
                BlockInCE = l_RebuildPMTManagerDptr->m_TableBlk[ulSuperPU][RebuildTableBlkSN][uLUNInSuperPU];
                pAddr[uLUNInSuperPU] = &realAddr[uLUNInSuperPU];
                pAddr[uLUNInSuperPU]->m_PPN = 0;
                pAddr[uLUNInSuperPU]->m_BlockInPU = BlockInCE;
                pAddr[uLUNInSuperPU]->m_LPNInPage = 0;
                pAddr[uLUNInSuperPU]->m_OffsetInSuperPage = uLUNInSuperPU;
                pAddr[uLUNInSuperPU]->m_PageInBlock = l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU];
                pAddr[uLUNInSuperPU]->m_PUSer = ulSuperPU;
                ulPos = RebuildTableBlkSN*PG_PER_SLC_BLK+l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU];
                pSpare[uLUNInSuperPU] = &pPMTSpareBaseAddr[ulPos];
                LoadPageStatus = pPMTSpareStatusBaseAddr[ulPos];

                //Err handling of PMT page err
                //Just record err info for later processing
                if (LoadPageStatus == SUBSYSTEM_STATUS_PENDING)
                {
                    bNormal = FALSE;
                    continue;
                }
                else if (LoadPageStatus == SUBSYSTEM_STATUS_FAIL)
                {
                    L2_CollectErrInfoOfRebuild(pAddr[uLUNInSuperPU]->m_PUSer, pAddr[uLUNInSuperPU]->m_OffsetInSuperPage, pAddr[uLUNInSuperPU]->m_BlockInPU,
                        pAddr[uLUNInSuperPU]->m_PageInBlock, PMT_PAGE_ERR);
                    bNormal = FALSE;
                    continue;
                }
                else if (LoadPageStatus == SUBSYSTEM_STATUS_SUCCESS || LoadPageStatus == SUBSYSTEM_STATUS_RECC || LoadPageStatus == SUBSYSTEM_STATUS_RETRY_SUCCESS)
                {
                    if (TRUE == L2_IsHavePMTErr(pAddr[uLUNInSuperPU]))
                    {
                        bNormal = FALSE;
                        continue;
                    }
                    else
                    {
                        l_RebuildPMTManagerDptr->m_TableBlklastPagePMTI[ulSuperPU][RebuildTableBlkSN][uLUNInSuperPU] = pSpare[uLUNInSuperPU]->m_PMTIndex;
                    }
                }
                else if (LoadPageStatus == SUBSYSTEM_STATUS_EMPTY_PG)
                {
                    if (INVALID_8F == l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[ulSuperPU][RebuildTableBlkSN][uLUNInSuperPU])
                    {
                        l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[ulSuperPU][RebuildTableBlkSN][uLUNInSuperPU] = pAddr[uLUNInSuperPU]->m_PageInBlock;
                    }
                    bNormal = FALSE;
                    ucEmptyPageLUNNum++;
                    continue;
                }
                else
                {
                    DBG_Printf("SPU %d LUN %d BLK %d load PMT page %d status %d error\n",
                                ulSuperPU, uLUNInSuperPU, BlockInCE, pAddr[uLUNInSuperPU]->m_PageInBlock, LoadPageStatus);
                    DBG_Getch();
                }

                if (pSpare[uLUNInSuperPU]->m_RedComm.bcPageType != PAGE_TYPE_PMT)
                {
                    bNormal = FALSE;
                    continue;
                }
            }

            if (FALSE == bNormal)
            {
                if (LUN_NUM_PER_SUPERPU == ucEmptyPageLUNNum)
                {
                    l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = PG_PER_SLC_BLK - 1;
                }
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU]++;
                if (l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] >= PG_PER_SLC_BLK)
                {
                    l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;
                    l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU]++;
                    
                }
            
                continue;
            }

            /* Todo : inline  function */
            TS = pSpare[0]->m_RedComm.ulTimeStamp;
            /* check */
            for (uLUNInSuperPU = 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if (TS != pSpare[uLUNInSuperPU]->m_RedComm.ulTimeStamp)
                {
                    DBG_Printf("SPU %d LUN %d BLK %d Super PMT page %d TS %d check error\n",
                                ulSuperPU, uLUNInSuperPU, BlockInCE, pAddr[uLUNInSuperPU]->m_PageInBlock, TS);
                    //DBG_Getch();
                }
            }
            for (uLUNInSuperPU = 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                /* find smallest TS of Super PMT page*/
                if (pSpare[uLUNInSuperPU]->m_RedComm.ulTimeStamp < TS)
                {
                    TS = pSpare[uLUNInSuperPU]->m_RedComm.ulTimeStamp;
                }
            }            
            /* End Todo */

            //DataTS = TS;
            uPMTIndexInPu = pSpare[0]->m_PMTIndex;
            for(uLUNInSuperPU = 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if(uPMTIndexInPu != pSpare[0]->m_PMTIndex) 
                {
                    DBG_Printf("SPU %d LUN %d BLK %d Super PMT page %d index %d check error\n",
                                ulSuperPU, uLUNInSuperPU, BlockInCE, pAddr[uLUNInSuperPU]->m_PageInBlock, uPMTIndexInPu);
                    DBG_Getch();
                }               
            }
            if (uPMTIndexInPu > PMTPAGE_CNT_PER_SUPERPU)
            {
                DBG_Printf("SPU %d LUN %d BLK %d page %d Load not correct PMTIndex\n",
                            ulSuperPU, uLUNInSuperPU, BlockInCE, pAddr[uLUNInSuperPU]->m_PageInBlock);
                DBG_Getch();
                return FALSE;
            }

            if (l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][uPMTIndexInPu] <= TS)
            {
                l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][uPMTIndexInPu] = TS;
                for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    L2_SetPhysicalAddr(ulSuperPU, uLUNInSuperPU, uPMTIndexInPu, pAddr[uLUNInSuperPU]);
                }

#ifdef ValidLPNCountSave_IN_DSRAM1
                U24setValue(ulSuperPU, uPMTIndexInPu, pSpare[0]->m_ValidLPNCountSave);
#else
                g_PMTManager->m_PMTSpareBuffer[ulSuperPU][uPMTIndexInPu]->m_ValidLPNCountSave = pSpare[0]->m_ValidLPNCountSave;
#endif

                for(uLUNInSuperPU = 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    if(pSpare[0]->m_ValidLPNCountSave != pSpare[uLUNInSuperPU]->m_ValidLPNCountSave)
                    {
                        DBG_Printf("SPU %d LUN %d BLK %d page %d PMTI %d ValidLPNCountSave %d check error\n",
                                    ulSuperPU, uLUNInSuperPU, BlockInCE,pAddr[uLUNInSuperPU]->m_PageInBlock,
                                    uPMTIndexInPu, pSpare[0]->m_ValidLPNCountSave);
                        DBG_Getch();
                    }
                }
            }

            pDirtyBitmap = pSpare[0]->m_DirtyBitMap;
            for(uLUNInSuperPU = 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                   /*Todo make sure pDirtyBitmap equal in all LUN*/
            }
            //get max PMT time Pos
            MaxPMTTS = l_RebuildPMTIDptr->m_MaxTableTs[ulSuperPU];
            if (TS > MaxPMTTS)
            {
                l_RebuildPMTIDptr->m_MaxTableTs[ulSuperPU] = TS;
                l_RebuildPMTIDptr->m_PMTIndexofMaxTableTS[ulSuperPU] = uPMTIndexInPu;
                //Update PMTDirtyMap
                COM_MemCpy(l_RebuildPMTIDptr->m_RebuildPMTDirtyBitMapInCE[ulSuperPU].m_DirtyBitMap, pDirtyBitmap, PMT_DIRTY_BM_SIZE);
            }

            //Update PMT page no dirty TS in current Pu by pSpare->m_DirtyBitMap & pSpare->TimeStamp
            //L2_UpdatePMTPageNoDirtyTS(ulSuperPU, pSpare[0]->m_RedComm.ulTimeStamp, pDirtyBitmap);

            //L2_PMTISetNotRebuild(PMTIndex);
            //L2_ClearDirty(PMTIndex);

            TableBlkTS = l_RebuildPMTManagerDptr->m_TableBlkTS[ulSuperPU][RebuildTableBlkSN];
            if (TS >= TableBlkTS)
            {
                //l_RebuildPMTManagerDptr->m_TableBlk[PUSer][SN] = pAddr->m_BlockInPU;
                l_RebuildPMTManagerDptr->m_TableBlkPPO[ulSuperPU][RebuildTableBlkSN] = pAddr[0]->m_PageInBlock;
                l_RebuildPMTManagerDptr->m_TableBlkTS[ulSuperPU][RebuildTableBlkSN] = TS;
            }

            l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU]++;
            if (l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] >= PG_PER_SLC_BLK)
            {
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;
                l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU]++;                
            }
        }
                
    }

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        U32 ulPMTIndex;

        /* only consider LUN 0 */
        pPMTSpareBaseAddr = (RED *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0]);        

        for(ulPMTIndex = 0; ulPMTIndex < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndex++)
        {
            if(INVALID_8F == g_PMTI[ulSuperPU]->m_PhyAddr[ulPMTIndex][0].m_PPN)
            {
                DBG_Printf("SPU %d PMTIndex %d have no mapping\n", ulSuperPU, ulPMTIndex);                
            }
            else
            {
                ulPos = g_PMTI[ulSuperPU]->m_PhyAddr[ulPMTIndex][0].m_BlockInPU *PG_PER_SLC_BLK + 
                        g_PMTI[ulSuperPU]->m_PhyAddr[ulPMTIndex][0].m_PageInBlock;
                pSpare[0] = &pPMTSpareBaseAddr[ulPos];
                
                L2_UpdatePMTPageNoDirtyTS(ulSuperPU, pSpare[0]->m_RedComm.ulTimeStamp, pSpare[0]->m_DirtyBitMap);
            }
        }
    }

    return TRUE;
}
void MCU1_DRAM_TEXT L2_RebuildAddErrTLCBlk(U32 ucSuperPU, U32 uLUNInSuperPU, U32 vBlk)
{
    U16 i;
    U16 usErrTLCBlkCnt;

    usErrTLCBlkCnt = l_RebuildErrTLCDptr->m_ErrTLCBlkCnt[ucSuperPU][uLUNInSuperPU];        

    for (i = 0; i < usErrTLCBlkCnt; i++)
    {
        if (vBlk == l_RebuildErrTLCDptr->m_ErrTLCBlk[ucSuperPU][uLUNInSuperPU][i])
            break;
    }

    if (i == usErrTLCBlkCnt)
    {
        l_RebuildErrTLCDptr->m_ErrTLCBlk[ucSuperPU][uLUNInSuperPU][usErrTLCBlkCnt] = vBlk;
        l_RebuildErrTLCDptr->m_ErrTLCBlkCnt[ucSuperPU][uLUNInSuperPU]++;
    }
}

void MCU1_DRAM_TEXT L2_InheritTBErrTLCBlk()
{
    U32 ulSuperPU;
    U32 uLUNInSuperPU;
    U32 ulPhyBlk;
    U16 ulVirBlk;
    U16 usErrTLCBlkCnt;
    U32 i;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            usErrTLCBlkCnt = pTBRebManagement->usErrTLCBlkCnt[ulSuperPU][uLUNInSuperPU];
            for (i = 0; i < usErrTLCBlkCnt; i++)
            {
                ulPhyBlk = pTBRebManagement->usErrTLCBlk[ulSuperPU][uLUNInSuperPU][i];

                if (INVALID_4F == ulPhyBlk)
                    continue;

                ulVirBlk = L2_PBIT_GetVirturlBlockAddr(ulSuperPU, uLUNInSuperPU, ulPhyBlk);

                if (INVALID_4F == ulVirBlk)
                {
                    DBG_Printf("SPU %d LUN %d PBIT VBT mapping check in L2_InheritTBErrTLCBlk error\n",
                                ulSuperPU, uLUNInSuperPU);
                    DBG_Getch();
                }

                L2_CollectErrInfoOfRebuild(ulSuperPU, uLUNInSuperPU, ulVirBlk, (PG_PER_SLC_BLK - 1), TLC_RPMT_ERR);
                DBG_Printf("SPU %d LUN %d TLC block %d Last Page fail.\n",ulSuperPU, uLUNInSuperPU, ulVirBlk);

                L2_RebuildAddErrTLCBlk(ulSuperPU, uLUNInSuperPU, ulVirBlk);
            }
        }
    }
}
void MCU1_DRAM_TEXT L2_RebuildPrepare()
{
    //prepare the PBN for table blocks, and load Spare for each block in simulation version.
    U32 uLUNInSuperPU;
    U32 SuperPU;
    U32 BlockOffset;
    U32 Blk;

    //table blk
    for (SuperPU = 0; SuperPU < SUBSYSTEM_SUPERPU_NUM; SuperPU++)
    {
        for (BlockOffset = 0; BlockOffset < AT1_BLOCK_COUNT; BlockOffset++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                Blk = pRT->m_RT[SuperPU].AT1BlockAddr[uLUNInSuperPU][BlockOffset];
                l_RebuildPMTManagerDptr->m_TableBlk[SuperPU][BlockOffset][uLUNInSuperPU] = Blk;
            }
        }

    }

    L2_SetRebuildState(Rebuild_State_RebuildPMTI);

    L2_InheritTBErrTLCBlk();

    return;
}


//Rebuild PMTI, and updte the RebuildState if necessary.
void MCU1_DRAM_TEXT L2_RebuildPMTI()
{
    U32 ulSuperPU;
    U32 ulLUNInSuperPU;
    RebuildPMTIState CurrState;
    U8 *pPMTSpareStatusBaseAddr;
    U32 i;

    CurrState = l_RebuildPMTIDptr->m_PMTIState;
    switch (CurrState)
    {
    case Rebuild_PMTI_Prepare:
    {
        L2_InitPMTI();        
        l_RebuildPMTIDptr->m_PMTIState = Rebuild_PMTI_LoadPMTSpare;        
        for(ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            l_RebuildPMTIDptr->m_CheckStatusPos[ulSuperPU] = 0;

            for (ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
            {
                pPMTSpareStatusBaseAddr = (U8 *)((U32)g_PMTManager->m_pPMTPage[ulSuperPU][0] +
                    sizeof(RED)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*LUN_NUM_PER_SUPERPU +
                    sizeof(U8)*PG_PER_SLC_BLK*AT1_BLOCK_COUNT*ulLUNInSuperPU);

                for (i = 0; i < PG_PER_SLC_BLK*AT1_BLOCK_COUNT; i++)
                {
                    pPMTSpareStatusBaseAddr[i] = SUBSYSTEM_STATUS_PENDING;
                }
            }            
        }
    }
        //break;

    case Rebuild_PMTI_LoadPMTSpare:
    {
        
        if (TRUE == LoadPMTSpareToBuffer())
        {
            U8 ucBlockIndex;
            
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {
                l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] = 0;
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;

                L2_TB_Rebuild_TableEraseResetParam(ulSuperPU);
            }
            
            /* if any block need erase */
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {                
                for(ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
                {
                    DBG_Printf("SPU:%d AT1 block_index:%d UECC cnt: %d\n", ulSuperPU, ucBlockIndex, l_RebuildPMTIDptr->m_UECCCnt[ulSuperPU][ucBlockIndex]);
                
                    if(l_RebuildPMTIDptr->m_UECCCnt[ulSuperPU][ucBlockIndex] >= SLC_UECCCNT_ERASE_THRESHOLD)
                    {
                        l_RebuildPMTIDptr->m_NeedErase[ulSuperPU][ucBlockIndex] = TRUE;
                    }
                }
            }
            
            l_RebuildPMTIDptr->m_PMTIState = Rebuild_PMTI_EraseTooManyUECCBlock;
        }       
    }
        break;
#if 0
    case Rebuild_PMTI_CheckUECCCnt:
    {
        if (TRUE == L2_RebuildPMTICheckUECCCnt())
        {
            U8 ucBlockIndex;
            
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {
                l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] = 0;
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;

                L2_TB_Rebuild_TableEraseResetParam(ulSuperPU);
            }

            /* if any block need erase */
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {                
                for(ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
                {
                    if(l_RebuildPMTIDptr->m_UECCCnt[ulSuperPU][ucBlockIndex] >= SLC_UECCCNT_ERASE_THRESHOLD)
                    {
                        l_RebuildPMTIDptr->m_NeedErase[ulSuperPU][ucBlockIndex] = TRUE;
                    }
                }
            }
            
            l_RebuildPMTIDptr->m_PMTIState = Rebuild_PMTI_EraseTooManyUECCBlock;            
        }            
    }
    break;
#endif    
    case Rebuild_PMTI_EraseTooManyUECCBlock:
    {
        if (TRUE == L2_RebuildPMTIEraseTooManyUECCBlocks())
        {
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {
                l_RebuildPMTIDptr->m_RebuildTableBlockSN[ulSuperPU] = 0;
                l_RebuildPMTIDptr->m_RebuildTableBlkPPO[ulSuperPU] = 0;                
            }
            l_RebuildPMTIDptr->m_PMTIState = Rebuild_PMTI_UpdatePMTI;
        }            
    }
    break;
    case Rebuild_PMTI_UpdatePMTI:
    {        
        UpdatePMTIBySpare();            
        {
            l_RebuildPMTIDptr->m_PMTIState = Rebuild_PMTI_Done;
            L2_SetRebuildState(Rebuild_State_RebuildPMTManager);
        }
    }
    break;
    }

}

void MCU1_DRAM_TEXT L2_RebuildPMTManager()
{
    L2_ResetTableManagerByInfo();

#ifdef DBG_TABLE_REBUILD
    L2_CheckPMTManager();
    L2_CheckPMTI();
#endif
}

GLOBAL  BOOT_MANAGEMENT* g_BootManagement;

/*****************************************************************************
Prototype      : L2_BootManagementInit
Description    :
Input          : None
Output         : None
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2013/10/16
Author       : henryluo
Modification : Created function

*****************************************************************************/
void MCU1_DRAM_TEXT L2_BootManagementInit()
{
    U32 i;
    U32 ulPuNum;

    for (ulPuNum = 0; ulPuNum < SUBSYSTEM_LUN_NUM; ulPuNum++)
    {
        for (i = 0; i < BOOT_STRIPE_CNT; i++)
        {
            g_BootManagement->m_BootStripeStatus[ulPuNum][i] = FALSE;
        }
    }

    g_BootManagement->m_BootType = BOOT_NORMAL;
    g_BootUpOk = FALSE;

    for (ulPuNum = 0; ulPuNum < SUBSYSTEM_LUN_NUM; ulPuNum++)
    {
        g_BootManagement->m_BootStatus[ulPuNum] = BOOT_STATUS_BEGIN;
        g_BootManagement->m_BootStripeSN[ulPuNum] = 0;
        g_BootManagement->m_BootStripeStartPMTIndex[ulPuNum] = 0;
        g_BootManagement->m_BootLoadPMTPagePos[ulPuNum] = 0;
        g_BootManagement->m_MissStripeCnt[ulPuNum] = 0;
        g_BootManagement->m_BootUpOkOfPu[ulPuNum] = FALSE;
        g_BootManagement->m_DoingBootStipeFlag[ulPuNum] = FALSE;

        for (i = 0; i < LPN_PER_BUF; i++)
        {
            g_BootManagement->m_MissStripeForCmd[ulPuNum][i] = INVALID_8F;
        }
    }

}

/*****************************************************************************
Prototype      : L2_IsBootStripeNeedLoad
Description    :
Input          : U32* pLPN
Output         : None
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2013/10/16
Author       : henryluo
Modification : Created function

*****************************************************************************/
BOOL L2_IsBootStripeNeedLoad(U32* pLPN, U32 uLPNOffset, U32 uLPNCnt)
{
    U32 ulPMTIndexInPu = INVALID_8F;
    U32 i, j;
    U32 ulBootStripeSN;
    U32 LPN;
    U32 LPNInPu;
    U32 PUSer;
    U32 ulMissCnt = 0;
    U32 MissStripeForCmdTmp[LPN_PER_BUF] = { INVALID_8F };


    for (i = 0; i < uLPNCnt; i++)
    {

        LPN = pLPN[uLPNOffset + i];
        if (LPN == INVALID_8F)
        {
            continue;
        }

        PUSer = L2_GetSuperPuFromLPN(LPN);
        LPNInPu = L2_GetLPNInPu(LPN);
        ulPMTIndexInPu = LPNInPu / (LPN_CNT_PER_PMTPAGE);
        ulBootStripeSN = ulPMTIndexInPu / PMT_PAGE_CNT_PER_BOOT_PU;
        if (TRUE != g_BootManagement->m_BootStripeStatus[PUSer][ulBootStripeSN])
        {
            /* check no duplicate table Pu */
            for (j = 0; j < ulMissCnt; j++)
            {
                if (ulBootStripeSN == MissStripeForCmdTmp[j])
                {
                    break;
                }
            }

            if (j == ulMissCnt)
            {
                MissStripeForCmdTmp[ulMissCnt] = ulBootStripeSN;
                ulMissCnt++;
            }
        }
    }

    if (0 == ulMissCnt)
    {
        return FALSE;
    }
    else
    {
        if (TRUE == g_BootManagement->m_DoingBootStipeFlag[PUSer])
        {
            DBG_Printf("Boot Sttipe is going\n");
            DBG_Getch();
        }

        for (i = 0; i < ulMissCnt; i++)
        {
            g_BootManagement->m_MissStripeForCmd[PUSer][i] = MissStripeForCmdTmp[i];
        }

        g_BootManagement->m_MissStripeCnt[PUSer] = ulMissCnt;
        g_BootManagement->m_DoingBootStipeFlag[PUSer] = TRUE;

        return TRUE;
    }
}

/*****************************************************************************
Prototype      : L2_IsMissTableStripeLoadOk
Description    :
Input          : None
Output         : None
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2013/10/21
Author       : henryluo
Modification : Created function

*****************************************************************************/
BOOL L2_IsMissTableStripeLoadOk(U32 PUSer)
{
    U32 i;
    U32 ulBootPuSN;
    BOOL bTableLoadOk;

    bTableLoadOk = TRUE;
    for (i = 0; i < g_BootManagement->m_MissStripeCnt[PUSer]; i++)
    {
        ulBootPuSN = g_BootManagement->m_MissStripeForCmd[PUSer][i];
        if (FALSE == g_BootManagement->m_BootStripeStatus[PUSer][ulBootPuSN])
        {
            bTableLoadOk = FALSE;
            break;
        }
    }

    return bTableLoadOk;
}

/*****************************************************************************
Prototype      : L2_IsAllTablePuLoadOk
Description    :
Input          : None
Output         : None
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2013/10/21
Author       : henryluo
Modification : Created function

*****************************************************************************/
BOOL L2_IsAllTablePuLoadOk(U32 PUSer)
{
    U32 i;
    BOOL bTableLoadOk;

    bTableLoadOk = TRUE;
    for (i = 0; i < BOOT_STRIPE_CNT; i++)
    {
        if (FALSE == g_BootManagement->m_BootStripeStatus[PUSer][i])
        {
            bTableLoadOk = FALSE;
            break;
        }
    }

    return bTableLoadOk;
}

BOOL L2_IsAllTablePuLoadOkOfAllPu()
{
    U32 PUSer;
    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        if (FALSE == L2_IsAllTablePuLoadOk(PUSer))
        {
            return FALSE;
        }
    }
    return TRUE;
}



/****************************************************************************
Name        :BootUpRebuildBuildL2
Input       :Table block address for each PU.
Output      :The success or failure status of L2 table rebuilding.
Author      :Addison
Date        :
Description : Table rebuilding branch of boot up procedure which is taken due to abnormal
power-loss. Signaling an event and invoke Layer 2 event handler to handle it.
Others      :
Modify      :
****************************************************************************/
U32 GLOBAL  Rebuild_Start;
U32 GLOBAL  Rebuild_End;
/*****************************************************************************
Prototype      : L2_NormalBootEntry
Description    : L2 Normal boot up entry
Input          : U8 ucPuNum
Output         : None
Return Value   : void
Calls          :
Called By      :

History        :
1.Date         : 2013/10/15
Author       : henryluo
Modification : Created function

*****************************************************************************/
BOOL L2_NormalBootEntry(U32 PUSer)
{
    U32 ulBootStripeSN = INVALID_8F;
    U32 i;
    U16 PMTIIndexInPu = 0;
    U16 MissPMTIIndex[TABLE_RW_PAGE_CNT] = { INVALID_4F };
    U16 MissCnt = 0;
    U32 ulStatus;
    U8 ucLunInSuperPu;
    BOOL Ret = FALSE;

    switch (g_BootManagement->m_BootStatus[PUSer])
    {
    case BOOT_STATUS_BEGIN:

        g_BootManagement->m_DoingBootStipeFlag[PUSer] = TRUE;
        /* check wether boot done */
        for (i = 0; i < BOOT_STRIPE_CNT; i++)
        {
            if (FALSE == g_BootManagement->m_BootStripeStatus[PUSer][i])
            {
                g_BootManagement->m_BootStripeSN[PUSer] = i;
                break;
            }
        }

        if (BOOT_STRIPE_CNT == i)
        {
            /* pmt Pu already load done */
            g_BootManagement->m_BootUpOkOfPu[PUSer] = TRUE;     //global BootUpFlag need check later
            DBG_Printf("PU %d table load already done!\n", PUSer);
            DBG_Getch();
        }
        else
        {
            g_BootManagement->m_BootStatus[PUSer] = BOOT_PU_SELECT;
        }
        break;

    case BOOT_PU_SELECT:
        /* FTL trigger to load */
        if (0 != g_BootManagement->m_MissStripeCnt[PUSer])
        {
            for (i = 0; i < g_BootManagement->m_MissStripeCnt[PUSer]; i++)
            {
                ulBootStripeSN = g_BootManagement->m_MissStripeForCmd[PUSer][i];
                if (FALSE == g_BootManagement->m_BootStripeStatus[PUSer][ulBootStripeSN])
                {
                    g_BootManagement->m_BootStripeSN[PUSer] = ulBootStripeSN;
                    break;
                }
            }
        }
        /* boot thread idle load */
        else
        {
            /* SIM debug for boot up load table triggered by host cmd */
            /*
            #ifdef SIM
            if(FetchBufferReq() == FTL_STATUS_NO_CMD)
            {
            g_BootManagement->m_BootStatus = BOOT_STATUS_BEGIN;
            L2_SetSystemState(SYSTEM_STATE_HOST_RW);
            return;
            }
            #endif
            */
        }

        g_BootManagement->m_BootStripeStartPMTIndex[PUSer] = g_BootManagement->m_BootStripeSN[PUSer] * PMT_PAGE_CNT_PER_BOOT_PU;
        g_BootManagement->m_BootStatus[PUSer] = BOOT_PU_LOAD_PMT_PAGE;

        break;

    case BOOT_PU_LOAD_PMT_PAGE:
        if (g_TableRW[PUSer].m_TableStatus != TABLE_STATUS_OVER)
        {
            DBG_Printf("table thread not done!\n");
            return TRUE;
        }

        //(2)Find PMTIIndex need to load
        PMTIIndexInPu = g_BootManagement->m_BootStripeStartPMTIndex[PUSer] + g_BootManagement->m_BootLoadPMTPagePos[PUSer];

        while ((g_BootManagement->m_BootLoadPMTPagePos[PUSer] < PMT_PAGE_CNT_PER_BOOT_PU)
            && (PMTIIndexInPu < PMTPAGE_CNT_PER_PU))
        {
            MissPMTIIndex[MissCnt] = PMTIIndexInPu;
            MissCnt++;
            g_BootManagement->m_BootLoadPMTPagePos[PUSer]++;
            PMTIIndexInPu++;
            if (MissCnt >= TABLE_RW_PAGE_CNT)
            {
                break;
            }
        }

        //Load PMTPage
        if (0 != MissCnt)
        {
            L2_SetTableRW(PUSer, MissPMTIIndex, MissCnt, TABLE_READ);
            Ret = TRUE;
        }
        else
        {
            //clear event,init finished!
            //g_BootManagement->m_BootLoadPMTPagePos[PUSer] = 0;
            g_BootManagement->m_BootStatus[PUSer] = BOOT_PU_WAIT_STATUS;
        }
        break;

    case BOOT_PU_WAIT_STATUS:
        ulStatus = TABLE_QUIT_STAGE_DONE;

        /*we only check flash status after all read PMT request have been built to L3 */
        for (i = PMT_PAGE_CNT_PER_BOOT_PU; i > 0; i--)
        {
            PMTIIndexInPu = g_BootManagement->m_BootStripeStartPMTIndex[PUSer] + i - 1;
            if (PMTIIndexInPu >= PMTPAGE_CNT_PER_PU)
            {
                continue;
            }

            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (SUBSYSTEM_STATUS_FAIL == g_PMTManager->m_FlushStatus[PUSer][ucLunInSuperPu][PMTIIndexInPu])
                {
                    COMM_EVENT_PARAMETER * pParameter;

                    /*TODO Table rebuild*/
                    DBG_Printf("MCU%d Pu %d [NormalBoot]:Load PMT %d Fail,todo table rebuild\n", HAL_GetMcuId(), PUSer, PMTIIndexInPu);

#ifdef SIM
                    SIM_BackTableFromDram();
#endif
                    /* Executes rebuilding process for L2 */
                    g_BootManagement->m_BootType = BOOT_ABNORMAL;

                    CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);
                    CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_REBUILD);

                    return TRUE;

                }
                // How to handle the empty page error type? JasonGuo
                else if (SUBSYSTEM_STATUS_SUCCESS != g_PMTManager->m_FlushStatus[PUSer][ucLunInSuperPu][PMTIIndexInPu]
                    && SUBSYSTEM_STATUS_RECC != g_PMTManager->m_FlushStatus[PUSer][ucLunInSuperPu][PMTIIndexInPu]
                    && SUBSYSTEM_STATUS_RETRY_SUCCESS != g_PMTManager->m_FlushStatus[PUSer][ucLunInSuperPu][PMTIIndexInPu])
                {
                    ulStatus = TABLE_QUIT_WAIT_UNDONE;
                    break;
                }
            }
        }

        if (TABLE_QUIT_STAGE_DONE == ulStatus)
        {
            U32 BootPuSN = g_BootManagement->m_BootStripeSN[PUSer];
            g_BootManagement->m_BootStripeStatus[PUSer][BootPuSN] = TRUE;

#ifdef ValidLPNCountSave_IN_DSRAM1 
            U32 PMTStartIndex = BootPuSN * PMT_PAGE_CNT_PER_BOOT_PU;
            U32 i = 0;

            while ( (i < PMT_PAGE_CNT_PER_BOOT_PU) && (PMTStartIndex + i) < PMTPAGE_CNT_PER_PU )
            {
                U24setValue(PUSer, (PMTStartIndex + i), g_PMTManager->m_PMTSpareBuffer[PUSer][PMTStartIndex + i]->m_ValidLPNCountSave);
                i++;
            }
#endif
            g_BootManagement->m_BootStatus[PUSer] = BOOT_STATUS_DONE;
        }

        break;

    case BOOT_STATUS_DONE:
        /* reset boot management */
        g_BootManagement->m_BootStatus[PUSer] = BOOT_STATUS_BEGIN;
        g_BootManagement->m_BootStripeSN[PUSer] = 0;
        g_BootManagement->m_BootStripeStartPMTIndex[PUSer] = 0;
        g_BootManagement->m_BootLoadPMTPagePos[PUSer] = 0;

        if (0 != g_BootManagement->m_MissStripeCnt[PUSer])
        {
            if (TRUE == L2_IsMissTableStripeLoadOk(PUSer))
            {
                /* reset table miss status */
                for (i = 0; i < g_BootManagement->m_MissStripeCnt[PUSer]; i++)
                {
                    g_BootManagement->m_MissStripeForCmd[PUSer][i] = INVALID_8F;
                }
                g_BootManagement->m_MissStripeCnt[PUSer] = 0;
            }
            else
            {
                //Not finish, load next Miss Pu Table
                return FALSE;
            }
        }

        if (TRUE == L2_IsAllTablePuLoadOk(PUSer))
        {
            g_BootManagement->m_BootUpOkOfPu[PUSer] = TRUE;
        }

        g_BootManagement->m_DoingBootStipeFlag[PUSer] = FALSE;

        //all Pu process
        if (TRUE == L2_IsAllTablePuLoadOkOfAllPu())
        {
            g_BootUpOk = TRUE;
#ifndef SIM
            /* enable global PM */
            //gbGlobalPMEnable = TRUE;
#endif

        }
        Ret = TRUE;
        break;

    default:
        break;
    }
    return Ret;
}

U32 L2_GetL2FreePage(void)
{
    U32 SPU;
    U32 ulL2FreePageCnt = 0;

    for (SPU = 0; SPU < SUBSYSTEM_SUPERPU_NUM; SPU++)
    {
        if ((g_PuInfo[SPU]->m_DataBlockCnt[BLKTYPE_SLC] - g_PuInfo[SPU]->m_AllocateBlockCnt[BLKTYPE_SLC]) > REBUILD_MOVE_BLKCNT)
        {
            ulL2FreePageCnt += PG_PER_SLC_BLK + (PG_PER_SLC_BLK - g_PuInfo[SPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
        }
        else
        {
            //ulL2FreePageCnt += (PG_PER_SLC_BLK - g_PuInfo[SPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
            ulL2FreePageCnt += 0;
        }
    }
    return ulL2FreePageCnt * LUN_NUM_PER_SUPERPU;
}
/*****************************************************************************
 Prototype      : L2_GetCalcDirtyBlockCntInRound
 Description    : get block count need to be calculated in this round.
 Input          : U32 ulRoundSN
 Output         : now set 2 block per round.
 Return Value   : U32
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/01/06
 Author       : henryluo
 Modification : Created function in TaiPei

 *****************************************************************************/
U32 L2_GetCalcDirtyBlockCntInRound(U32 ulRoundSN)
{
    U32 ulRetCount;
    U32 ulRefCnt;
    U32 ulSuperPU , i;
    PuInfo* pInfo;
    U32 ulMaxBlockCnt = 0;
    BOOL bNeedAcc = FALSE;
    U32 ulFtlWriteFreePage;

    bNeedAcc = FALSE;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pInfo = g_PuInfo[i];

        /* forbidden to do host write to make sure system have two empty block for next SPOR, add by henryluo 2015-05-20 */
        if (pInfo->m_DataBlockCnt[BLKTYPE_SLC] <= (REBUILD_MOVE_BLKCNT + pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]))
        {
        #if 0
            ulFtlWriteFreePage = PG_PER_SLC_BLK - pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
            DBG_Printf("##CalcDirtyBlkCntInRound SPU %d SLC DataBlkCnt %d AllocBlkCnt %d FreePage %d\n", i, pInfo->m_DataBlockCnt[BLKTYPE_SLC],pInfo->m_AllocateBlockCnt[BLKTYPE_SLC], ulFtlWriteFreePage);

            if (ulFtlWriteFreePage > 0)
            {
                ulRefCnt = (l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[i] / ulFtlWriteFreePage) + 1;
                l_CalcDirtyCntDptr->m_AccRoundSN = INVALID_8F;
                return ulRefCnt;
            }
            else
            {
                return l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[i];
            }
        #else
            DBG_Printf("##CalcDirtyBlkCntInRound SPU %d SLC DataBlkCnt %d AllocBlkCnt %d \n", i, pInfo->m_DataBlockCnt[BLKTYPE_SLC],pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]);
            return l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[i];
        #endif
        }

        /* find the PU with most block count need to calc */
        if (l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[i] > ulMaxBlockCnt)
        {
            ulMaxBlockCnt = l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[i];
            ulSuperPU  = i;
        }

        /* check free page count */
        if (pInfo->m_SLCMLCFreePageCnt < (TARGET_BLOCK_CNT + ERROR_HANDLE_BLOCK_CNT) * PG_PER_SLC_BLK)
        {
            ulSuperPU  = i;
            bNeedAcc = TRUE;
            break;
        }

        /* check seq RPMT buffer */
        if ((1 == pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML])
            && (pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] >= (PG_PER_SLC_BLK - (SUBSYSTEM_SUPERPU_NUM * 10))))
        {
            ulSuperPU  = i;
            bNeedAcc = TRUE;
            break;
        }

        #if 0
        /* check rand RPMT buffer */
        if ((1 == pInfo->m_RPMTBufferPointer[TARGET_HOST_GC])
            && (pInfo->m_TargetPPO[TARGET_HOST_GC] >= (PG_PER_SLC_BLK - (SUBSYSTEM_SUPERPU_NUM * 10))))
        {
            ulSuperPU  = i;
            bNeedAcc = TRUE;
            break;
        }
        #endif
    }

    ulFtlWriteFreePage = PG_PER_SLC_BLK + (PG_PER_SLC_BLK - g_PuInfo[ulSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
    ulRefCnt = (l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU] / ulFtlWriteFreePage) + 1;

    /* Accelerate to calc dirty count */
    if (TRUE == bNeedAcc)
    {
        if (INVALID_8F == l_CalcDirtyCntDptr->m_AccRoundSN)
        {
            l_CalcDirtyCntDptr->m_AccRoundSN = ulRoundSN;
        }

        ulRetCount = (ulRefCnt << (ulRoundSN - l_CalcDirtyCntDptr->m_AccRoundSN));
        DBG_Printf("SPU %d  Accelerate to calc DC, Round = %d.\n", ulSuperPU, ulRetCount);
    }
    else
    {
        ulRetCount = ulRefCnt; //schedule FTL after calc 2 block dirty count.
        l_CalcDirtyCntDptr->m_AccRoundSN = INVALID_8F;
    }

    return ulRetCount;
}


U32 MCU1_DRAM_TEXT L2_GetAverageRebuildBlkCnt()
{
    U32 i;
    U32 ulTotalRebuildBlkCnt = 0;
    U32 ulAverageRebuildBlkCnt = 0;
    U32 PUSer;

    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        for (i = 0; i < TARGET_ALL; i++)
        {
            ulTotalRebuildBlkCnt += l_RebuildPMTDptr->m_RebuildBlkCnt[PUSer][i];
        }
    }
    ulAverageRebuildBlkCnt = ulTotalRebuildBlkCnt / SUBSYSTEM_SUPERPU_NUM;

    return ulAverageRebuildBlkCnt;
}

void MCU1_DRAM_TEXT L2_SetCanDoFTLWriteFlag()
{
#if 0
    U32 ulRebuildBlkCntPerPu = 0;

    ulRebuildBlkCntPerPu = L2_GetAverageRebuildBlkCnt();

    l_CalcDirtyCntDptr->m_CanDoFTLWrite = (ulRebuildBlkCntPerPu <= REBUILD_BLK_CNT_MAX_PER_PU) ? TRUE : FALSE;

    if (FALSE == l_CalcDirtyCntDptr->m_CanDoFTLWrite)
    {
        DBG_Printf("FW forbiden do FTL Write, RebuildBlkCntPerPu %d. \n", ulRebuildBlkCntPerPu);
    }
#else
    l_CalcDirtyCntDptr->m_CanDoFTLWrite = TRUE;
#endif
}

/*****************************************************************************
 Prototype      : L2_AbnormalBootEntry
 Description    : Abnormal boot up after rebuild PMT.
 Input          : U8 ucPuNum
 Output         : void
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/01/06
 Author       : henryluo
 Modification : Created function in TaiPei

 *****************************************************************************/
U32 L2_AbnormalBootEntry()
{
    U32 ret = COMM_EVENT_STATUS_BLOCKING;
    U32 ulSuperPU, i;
    CALC_DIRTY_COUNT_STATUS Status;
    BOOL bAdjustDirtyCntFinish = TRUE;
    U32 ulRoundOk = 0;
    RebuildCalcDirtyCntStage CalcDirtyCntStage;

    //wait for all Pu FTL or Table done
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        if (SYS_THREAD_BOOT != L2_GetCurrThreadType(ulSuperPU))
        {
            ret = COMM_EVENT_STATUS_GET_EVENTPEND;
            return ret;
        }
    }

    CalcDirtyCntStage = l_CalcDirtyCntDptr->m_RebuildDirtyCntStage;
    switch (CalcDirtyCntStage)
    {
    case Rebuild_DirtyCnt_Bootup:
        //Calc DirtyCnt
        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            if (L2_IsNeedRebuildDirtyCnt(ulSuperPU))
            {
                /* round done in this PU */
                if (TRUE == l_CalcDirtyCntDptr->m_CalcDirtyRoundStatus[ulSuperPU])
                {
                    ulRoundOk |= (1 << ulSuperPU);
                    continue;
                }

                Status = L2_RebuildDirtyCnt(ulSuperPU);
                if (CALC_ALL_BLOCK_DONE == Status)
                {
                    l_CalcDirtyCntDptr->m_CalcDirtyRoundStatus[ulSuperPU] = TRUE;
                    L2_ClearRebuildDirtyCntFlagOfCE(ulSuperPU);
                }
                else if (CALC_ONE_ROUND_DONE == Status)
                {
                    l_CalcDirtyCntDptr->m_CalcDirtyRoundStatus[ulSuperPU] = TRUE;
                }
            }
            else
            {
                ulRoundOk |= (1 << ulSuperPU);
            }
        }

        /* all PU dirty count calc done */
        if (L2_IsRebuildDirtyCntFinish())
        {
            DBG_Printf("rebuild DirtyCnt done, RoundSN = %d\n", l_CalcDirtyCntDptr->m_RoundSN);
            l_CalcDirtyCntDptr->m_RebuildDirtyCntStage = Rebuild_DirtyCnt_Wait_L3_Idle;
        }
        /* response host during calc dirty count */
        else if ((INVALID_8F >> (32 - SUBSYSTEM_SUPERPU_NUM)) == ulRoundOk)
        {

            /* Add total rebuild block count control in table rebuild, if rebuild block count more than thresholds,
               FW forbiden to resopnse host write untill the whole table rebuild sequence done. */
            if (TRUE == l_CalcDirtyCntDptr->m_CanDoFTLWrite)
            {
                /* set thread to FTL */
                for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
                {
                    L2_SetThreadQuota(ulSuperPU, SYS_THREAD_FTL, 1);
                    L2_SetCurrThreadType(ulSuperPU, SYS_THREAD_FTL);
                }
                /* allow FTL thread to response host */
                ret = COMM_EVENT_STATUS_GET_EVENTPEND;
            }

            l_CalcDirtyCntDptr->m_RoundSN++;

            /* clear round status */
            for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
            {
                l_CalcDirtyCntDptr->m_CalcDirtyRoundStatus[i] = FALSE;
            }

            /* set next round calc block count */
            l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCntInRound = L2_GetCalcDirtyBlockCntInRound(l_CalcDirtyCntDptr->m_RoundSN);
        }
        break;

    case Rebuild_DirtyCnt_Wait_L3_Idle:
        /* wait idle for L3 host read empty page error handling */
        if (TRUE == L2_FCMDQIsAllEmpty())
        {
            /* clear all rebuild structure */
            L2_ClearRebuildDirtyCntFlag();

            /* not to response command when adjust dirty count */
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {
                L2_SetThreadQuota(ulSuperPU, SYS_THREAD_FTL, 0);
            }
            l_CalcDirtyCntDptr->m_RebuildDirtyCntStage = Rebuild_DirtyCnt_Adjust;
        }
        break;

    case Rebuild_DirtyCnt_Adjust:
        bAdjustDirtyCntFinish = TRUE;
        for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
        {
            if (FALSE == L2_AdjustDirtyCnt(ulSuperPU))
            {
                bAdjustDirtyCntFinish = FALSE;
            }
        }

        if (TRUE == bAdjustDirtyCntFinish)
        {
            bShutdownPending = TRUE;
            l_CalcDirtyCntDptr->m_RebuildDirtyCntStage = Rebuild_DirtyCnt_Flush_PMT;
            //L2_SetSystemState(SYSTEM_STATE_SHUTDOWN);
            DBG_Printf("adjust dirty count done! dirty PMT page saving...\n");
        }
        break;

    case Rebuild_DirtyCnt_Flush_PMT:

        ret = L2_ShutdownEntry();

        //Table Shutdown finish
        if (FALSE == bShutdownPending)
        {
            /* abnormal bootup done, switch to host read/write */
            g_BootUpOk = TRUE;
            g_RebuildDirtyFlag = FALSE;
            l_CalcDirtyCntDptr->m_RebuildDirtyCntStage = Rebuild_DirtyCnt_Bootup;
            for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
            {
                L2_SetSystemState(ulSuperPU, SYSTEM_STATE_NORMAL);
            }

#ifdef SIM
#ifdef DBG_PMT
            L2_CheckPMT();
#endif
            L2_ResetCollectErrAddr();
            g_aRebuildDirtyCntFlag[HAL_GetMcuId() - 2] = FALSE;
#endif
            DBG_Printf("Abnormal boot up done!\n");
        }
        break;

    default:
        break;
    }

    return ret;
}

/*****************************************************************************
 Prototype      : L2_BootEntry
 Description    : L2 boot up entry
 Input          : U8 ucPuNum
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/10/15
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
BOOL L2_BootEntry(U8 ucSuperPu)
{
    if (BOOT_ABNORMAL == g_BootManagement->m_BootType)
    {
        //L2_AbnormalBootEntry();
        return FALSE;
    }
    else
    {
        return L2_NormalBootEntry(ucSuperPu);
    }
}

RebuildPMTState L2_RebuildGetPMTState()
{
    return l_RebuildPMTDptr->m_PMTState;
}

void L2_RebuildSetPMTState(RebuildPMTState State)
{
    l_RebuildPMTDptr->m_PMTState = State;
}

void  SwapTwoValue(U32* pValue1, U32* pValue2)
{
    U32 Tmp;
    Tmp = *pValue1;
    *pValue1 = *pValue2;
    *pValue2 = Tmp;
}

void  SortByTS(U32 ulSuperPU)
{
    U32* pRebuildDataBlk = NULL;
    U32* pRebuildDataBlkTS = NULL;
    U32 RebuildBlockCnt = 0;
    U32 i, j;
    U32 uLUNInSuperPU;
    U32 MinTS;
    U32 MinPos;
    U8 ucWriteTarget;

    /* sort data block by TS on all target */
    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        pRebuildDataBlk = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget];
        pRebuildDataBlkTS = l_RebuildPMTDptr->m_RebuildBlkTS[ulSuperPU][ucWriteTarget];
        RebuildBlockCnt = l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucWriteTarget];

        if (0 != RebuildBlockCnt)
        {
            for (i = 0; i < RebuildBlockCnt - 1; i++)
            {
                MinTS = pRebuildDataBlkTS[i];
                MinPos = i;

                for (j = i + 1; j < RebuildBlockCnt; j++)
                {
                    if (pRebuildDataBlkTS[j] < MinTS)
                    {
                        MinPos = j;
                        MinTS = pRebuildDataBlkTS[j];
                    }
                }

                if (i != MinPos)
                {
                    SwapTwoValue(&pRebuildDataBlk[i], &pRebuildDataBlk[MinPos]);
                    SwapTwoValue(&pRebuildDataBlkTS[i], &pRebuildDataBlkTS[MinPos]);
                }
            }

            /* */
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                l_RebuildPMTDptr->m_NeedLoadRPMT[ulSuperPU][ucWriteTarget][uLUNInSuperPU] = TRUE;
                l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU] = NOT_BYPASS;
            }
            l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucWriteTarget] = FALSE;
            l_RebuildPMTDptr->m_RebuildPMTStage[ulSuperPU] = REBUILD_PMT_STAGE_LOAD_RPMT;

            DBG_Printf("MCU%d target %d  block rebuild: ", HAL_GetMcuId(), ucWriteTarget);
            for (i = 0; i < RebuildBlockCnt; i++)
            {
                DBG_Printf(" 0x%x", pRebuildDataBlk[i]);
            }
            DBG_Printf("\n");
        }
        else
        {
            //DBG_Printf("target %d no block need rebuild. \n", ucWriteTarget);
        }
    }

    return;
}

void L2_GetRebuldClipTS(U32 ulSuperPU)
{
    U32* TS;
    U32 MinTS = INVALID_8F;
    U32 ulPMTIndexInPu;

#if 1
    /* if PMT page have err, use PMTPageTS to clip */
    if (L2_IsHavePMTErrOfRebuild(ulSuperPU))
    {
        TS = &l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][0];
    }
    /* use updated PMT page TS to clip */
    else
    {
        TS = &l_RebuildPMTIDptr->m_PMTPageNoDirtyMaxTS[ulSuperPU][0];
    }
#else

    //for TLC rebuild, can not use dirty bit map optimze Timestamp for PMT later update
    TS = &l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][0];
#endif

    /* search min TS in pu */
    for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndexInPu++)
    {
        if (TS[ulPMTIndexInPu] < MinTS)
        {
            MinTS = TS[ulPMTIndexInPu];
        }
    }

    l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU] = MinTS;
    DBG_Printf("PU %d ClipTs 0x%x\n", ulSuperPU, l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU]);

    return;
}

#if 0
void MCU1_DRAM_TEXT ClipByTSAndBlockType(U32 PUSer)
{
    U32* pBlkBuffer = l_RebuildPMTDptr->m_DataBlock[PUSer];
    U32* pBlkTSBuffer = l_RebuildPMTDptr->m_DataBlockTS[PUSer];

    U32* pRebuildBlk = NULL;
    U32* pRebuildBlkTS = NULL;
    U32 RebuildBlkNum = 0;

    U32 rebuild_index;
    U32 index;
    U32 MinTSPosofBlk = 0;
    U8 ucWriteTarget;

    /* get rebuild clip TS */
    L2_GetRebuldClipTS(PUSer);

    /* clip for all target */
    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        pRebuildBlk = l_RebuildPMTDptr->m_RebuildBlk[PUSer][ucWriteTarget];
        pRebuildBlkTS = l_RebuildPMTDptr->m_RebuildBlkTS[PUSer][ucWriteTarget];
        RebuildBlkNum = l_RebuildPMTDptr->m_RebuildBlkCnt[PUSer][ucWriteTarget];

        /* get data Blk which TS < MinTS */
        if (RebuildBlkNum > 0)
        {
            for (MinTSPosofBlk = RebuildBlkNum - 1; MinTSPosofBlk > 0; MinTSPosofBlk--)
            {
                if (pRebuildBlkTS[MinTSPosofBlk] > l_RebuildPMTIDptr->m_RebuildClipTSInPu[PUSer])
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            continue;
        }

        /* copy data Blk Info which need table rebuild */
        rebuild_index = 0;
        for (index = MinTSPosofBlk; index < RebuildBlkNum; index++)
        {
            pBlkBuffer[rebuild_index] = pRebuildBlk[index];
            pBlkTSBuffer[rebuild_index] = pRebuildBlkTS[index];
            rebuild_index++;
        }

        if (0 != rebuild_index)
        {
            l_RebuildPMTDptr->m_RebuildBlkCnt[PUSer][ucWriteTarget] = rebuild_index;
            l_RebuildPMTDptr->m_NeedLoadRPMT[PUSer][ucWriteTarget][0] = TRUE;
            l_RebuildPMTDptr->m_TargetFinished[PUSer][ucWriteTarget] = FALSE;
            l_RebuildPMTDptr->m_RebuildPMTStage[PUSer] = REBUILD_PMT_STAGE_LOAD_RPMT;

            DBG_Printf("target %d  block rebuild: ", ucWriteTarget);
            for (index = 0; index < rebuild_index; index++)
            {
                pRebuildBlk[index] = pBlkBuffer[index];
                pRebuildBlkTS[index] = pBlkTSBuffer[index];
                DBG_Printf(" 0x%x", pRebuildBlk[index]);
            }
            DBG_Printf("\n");
        }
        else
        {
            DBG_Printf("target %d no block need rebuild. \n", ucWriteTarget);
        }
    }

    return;
}
#endif

void MCU1_DRAM_TEXT ResetBufferStatus(U32 ulSuperPU)
{
    U32 uLUNInSuperPU, j;

    for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
    {
        for (j = 0; j < RebuildBufferCntInPU; j++)
        {
            l_BufferManagerDptr->m_BufferStatus[ulSuperPU][j][uLUNInSuperPU] = SUBSYSTEM_STATUS_SUCCESS;
        }
    }
}

//for debug.
#ifdef SIM
#include "xt_library.h"
GLOBAL BOOL l_bInitMCU1 = 0;
GLOBAL BOOL l_bInitMCU2 = 0;
extern U32 TestCaseSN;

#if 0
void MCU1_DRAM_TEXT Dump_RebuildDataBlkInfoBeforeClip(U32 PUSer)
{
    U32 count;
    FILE* pDump;
    extern U32 TestCaseSN;
    U32 BlockCnt = l_RebuildPMTDptr->m_RebuildBlockCnt[PUSer];
    U32 ulMcuID = XT_RSR_PRID();

#if 0
    if (l_bInitMCU1 == 0 && l_bInitMCU2 == 0)
    {
        pDump = fopen("RebuildBlockCnt.txt", "w");
        if (MCU1_ID == ulMcuID)
        {
            l_bInitMCU1 = 1;
        }
        else if (MCU2_ID == ulMcuID)
        {
            l_bInitMCU2 = 1;
        }
    }
    else
    {
        pDump = fopen("RebuildBlockCnt.txt", "a+");
    }
#else

    pDump = fopen(".\\WinLog\\RebuildBlockCnt.txt", "w");
#endif

    if (pDump != NULL)
    {
        fprintf(pDump, "\nTestCaseSN %d MCU%d:\n", TestCaseSN, ulMcuID - 1);
        fprintf(pDump, "%s Pu [%d,%d]  \n", __FUNCTION__, 0, SUBSYSTEM_LUN_NUM);
        fprintf(pDump, "PUSer\t%d\t", PUSer);
        fprintf(pDump, "BlockCnt\t%d\t", BlockCnt);
        fprintf(pDump, "\n");
    }
    else
    {
        return;
    }

    fprintf(pDump, "BlockCnt\t%d\n", BlockCnt);
    fprintf(pDump, "Block         :\t");
    for (count = 0; count < BlockCnt; count++)
    {
        fprintf(pDump, "%06d\t", l_RebuildPMTDptr->m_DataBlock[PUSer][count]);
    }
    fprintf(pDump, "\n");

    fprintf(pDump, "BlockType :\t");
    for (count = 0; count < BlockCnt; count++)
    {
        fprintf(pDump, "%06d\t", l_RebuildPMTDptr->m_DataBlockType[PUSer][count]);
    }
    fprintf(pDump, "\n");
    fprintf(pDump, "pTS          :\t");
    for (count = 0; count < BlockCnt; count++)
    {
        fprintf(pDump, "%06d\t", l_RebuildPMTDptr->m_DataBlockTS[PUSer][count]);
    }
    fprintf(pDump, "\n");
    fclose(pDump);


}
void Dump_RebuildDataBlkInfoAfterClip(U32 PUSer)
{
    U32 count;
    FILE* pDump;
    U32 BlockCnt = l_RebuildPMTDptr->m_RebuildBlockCnt[PUSer];


    pDump = fopen(".\\WinLog\\RebuildBlockCnt.txt", "a+");


    fprintf(pDump, "BlockCnt\t%d\n", BlockCnt);

    fprintf(pDump, "m_RebuildClipTSInPu\t%d\n", l_RebuildPMTIDptr->m_RebuildClipTSInPu[PUSer]);

    for (count = 0; count < BlockCnt; count++)
    {
        fprintf(pDump, "%d\t", l_RebuildPMTDptr->m_DataBlock[PUSer][count]);
    }
    fprintf(pDump, "\n");

    fclose(pDump);
}
#endif

void Dump_RebuildErrInfoOfRebuild()
{
    FILE* pDump;
    U32 i;
    U32 Cnt = l_ErrManagerDptr->m_Cnt;
    PhysicalAddr ErrAddr = { 0 };
    ERR_TAG_OF_REBUILD ErrTag;

    pDump = fopen(".\\WinLog\\RebuildeErrInfo.txt", "a+");

    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        if (0 == l_ErrManagerDptr->m_Cnt)
        {
            continue;
        }

        fprintf(pDump, "TestSN %d\n", TestCaseSN);
        ErrAddr.m_PPN = l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr.m_PPN;
        ErrTag = l_ErrManagerDptr->m_Err_Info[i].m_ErrTag;

        fprintf(pDump, "Add Err Addr of Rebuild: PU %d Blk %d Page %d,", ErrAddr.m_PUSer, ErrAddr.m_BlockInPU, ErrAddr.m_PageInBlock);

        fprintf(pDump, "ErrTag:");
        switch (ErrTag)
        {
        case UNDEF_ERR:
            DBG_Printf("UNDEF_ERR\n");
            fprintf(pDump, "UNDEF_ERR\n");
            break;
        case FIRST_PAGE_ERR:
            DBG_Printf("FIRST_PAGE_ERR\n");
            fprintf(pDump, "FIRST_PAGE_ERR\n");

            break;
        case PMT_PAGE_ERR:
            DBG_Printf("PMT_PAGE_ERR\n");
            fprintf(pDump, "PMT_PAGE_ERR\n");
            break;
        case RPMT_ERR:
            DBG_Printf("RPMT_ERR\n");
            fprintf(pDump, "RPMT_ERR\n");
            break;
        case DATA_SPARE_ERR:
            DBG_Printf("DATA_SPARE_ERR\n");
            fprintf(pDump, "DATA_SPARE_ERR\n");
            break;
        }
    }

    fclose(pDump);

}
#endif

void MCU1_DRAM_TEXT ResetRebuildPMTStates(U32  ulSuperPU)
{
    ResetBufferStatus(ulSuperPU);
}

BOOL L2_IsNeedClipBlk(U32 ulSuperPU, U32 Blk)
{
    U32 TLCTargetBlk;
    U32 ulLUNInSuperPU;

    TLCTargetBlk = g_PuInfo[ulSuperPU]->m_TargetBlk[TARGET_TLC_WRITE];

    for(ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
    {
        if( !L2_FindBlkOfErrTLCRPMTC(ulSuperPU, ulLUNInSuperPU, Blk))
        {
            break;
        }
    }

    if ((ulLUNInSuperPU < LUN_NUM_PER_SUPERPU) && (FALSE != pVBT[ulSuperPU]->m_VBT[Blk].bPhyAddFindInFullRecovery) && TLCTargetBlk != Blk)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

U32 L2_Rebuild_GetFirstUsedLUNOfSuperPage(U32 ulSuperPU, U32 uVirtualBlock, U32 uPageNum)
{
    U32 uLUNInSuperPU = 0;
    U32 uWriteType;
    
    
    for(uWriteType= 0; uWriteType < TARGET_ALL; uWriteType++)
    {
        if(uVirtualBlock == pPBIT[ulSuperPU]->m_TargetBlock[uWriteType] && 
           uPageNum == pPBIT[ulSuperPU]->m_TargetPPO[uWriteType]-1)
        {            
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if (pPBIT[ulSuperPU]->m_TargetOffsetBitMap[uWriteType] & (1 << uLUNInSuperPU))
                {
                    break;
                }
            }
        }
    }    
    if(uLUNInSuperPU >= LUN_NUM_PER_SUPERPU)
    {
        uLUNInSuperPU = 0;
    }
    
    return uLUNInSuperPU;   
}


//mark the PMTIndex to load
//Select the Block to Rebuild
//Sort the Spare Sequence by TS
void MCU1_DRAM_TEXT L2_PrepareToRebuildPMT(U32 ulSuperPU)
{
    U32 BlkSN;
    U32 uLUNInSuperPU = 0;
    U32 RebuildBlockCnt = 0;
    RED* pCurrSpare = NULL;
    U32 CurrTS;
    U8  ucTargetType;
    U16 usPhyBlkAddr;
    U16 usBlockType;
    U32 uMinTLCBlkTS = INVALID_8F;
    BOOL bBypassTSAdjust = FALSE;
    U32 uLUNSelected = INVALID_8F;

    /* init RebuildBlkCnt */
    for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
    {
        l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucTargetType] = 0;
    }

    /* get rebuild clip TS */
    L2_GetRebuldClipTS(ulSuperPU);

RECAL:
    //Parse Spare, select the Block to rebuild, and sort in m_DataBlock[PU_NUM][BLK_PER_PLN]
    for (BlkSN = 0; BlkSN < VIR_BLK_CNT; BlkSN++)
    {
        if (TRUE == L2_IsPBNEmtpy(ulSuperPU, BlkSN))
        {
            continue;
        }
        else if (L2_IsNeedClipBlk(ulSuperPU, BlkSN))
        {
            uLUNSelected = L2_Rebuild_GetFirstUsedLUNOfSuperPage(ulSuperPU, BlkSN, 0);
            usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNSelected, BlkSN);
            usBlockType = pPBIT[ulSuperPU]->m_PBIT_Entry[uLUNSelected][usPhyBlkAddr].BlockType;
            /* consistency check */
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                U16 usPhyBlkAddrTmp = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                /* target ppo=1 case */
                if (0 == pPBIT[ulSuperPU]->m_PBIT_Entry[uLUNInSuperPU][usPhyBlkAddrTmp].BlockType)
                {
                    continue;
                }
                if (usBlockType != pPBIT[ulSuperPU]->m_PBIT_Entry[uLUNInSuperPU][usPhyBlkAddrTmp].BlockType)
                {
                    DBG_Printf("L2_PrepareToRebuildPMT SPU %d SuperVBlock %d block type %d of each LUN check error\n",
                                ulSuperPU, BlkSN, usBlockType);
                    DBG_Getch();
                }
            }
            /* target block is empty, allocated in rebuild Pu info */
            if ((0 == usBlockType) || (BLOCK_TYPE_EMPTY == usBlockType))
            {
                continue;
            }
            else if ((usBlockType < BLOCK_TYPE_SEQ) || (usBlockType >= BLOCK_TYPE_EMPTY))
            {
                DBG_Printf("L2_PrepareToRebuildPMT SPU %d VBN %d, PBN %d BlockType %d ERROR!\n", 
                            ulSuperPU, BlkSN, usPhyBlkAddr, usBlockType);
                DBG_Getch();
            }
            else
            {
                U32 uSuperPageToUse = 0;
                ucTargetType = usBlockType - BLOCK_TYPE_SEQ;
                if (ucTargetType > TARGET_TLC_WRITE)
                {
                    ucTargetType = TARGET_TLC_WRITE;
                }

                if (BlkSN == L2_GetPuTargetVBN(ulSuperPU, ucTargetType))
                {
                    if (L2_GetPuTargetPPO(ulSuperPU, ucTargetType) > 1)
                    {
                        uSuperPageToUse = L2_GetPuTargetPPO(ulSuperPU, ucTargetType) - 1;
                    }
                }
                else if (L2_VBT_Get_TLC(ulSuperPU, BlkSN))
                {
                    uSuperPageToUse = PG_PER_SLC_BLK*PG_PER_WL - 1;
                }
                else
                {
                    uSuperPageToUse = PG_PER_SLC_BLK - 1;
                }

                if (BlkSN == L2_GetPuTargetVBN(ulSuperPU, ucTargetType))
                {
                    /*Since SuperPage TS not same of one SuperPage, select the biggest */
                    uLUNSelected = L2_Rebuild_GetFirstUsedLUNOfSuperPage(ulSuperPU, BlkSN, uSuperPageToUse);
                    usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNSelected, BlkSN);
                    CurrTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNSelected][usPhyBlkAddr].LastPageTimeStamp;
                    for (uLUNInSuperPU = uLUNSelected + 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                    {
                        U16 usPhyBlkAddrTmp;
                        if (TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucTargetType], uLUNInSuperPU))
                        {
                            usPhyBlkAddrTmp = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                            if (pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp > CurrTS)
                            {
                                uLUNSelected = uLUNInSuperPU;
                                usPhyBlkAddr = usPhyBlkAddrTmp;
                                CurrTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp;
                            }
                        }
                    }
                }
                else
                {
                    uLUNSelected = 0;
                    usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNSelected, BlkSN);
                    CurrTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNSelected][usPhyBlkAddr].LastPageTimeStamp;
                    for (uLUNInSuperPU = uLUNSelected + 1; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                    {
                        U16 usPhyBlkAddrTmp;
                        usPhyBlkAddrTmp = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                        if (pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp > CurrTS)
                        {
                            uLUNSelected = uLUNInSuperPU;
                            usPhyBlkAddr = usPhyBlkAddrTmp;
                            CurrTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp;
                        }
                    }

                }

                /* consistency check */
                for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    U16 usPhyBlkAddrTmp = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                    /*target blk TS check */
                    if (BlkSN == L2_GetPuTargetVBN(ulSuperPU, ucTargetType))
                    {
                        /*to open later*/
                        if (0 == g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucTargetType])
                        {
                            break;
                        }
                        if (COM_BitMaskGet(g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucTargetType], uLUNInSuperPU))
                        {
                            if (CurrTS != pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp)
                            {
                                //DBG_Getch();
                            }
                        }
                        else
                        {
                            if (CurrTS < pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp + 1)
                            {
                                DBG_Printf("L2_PrepareToRebuildPMT SPU %d LUN %d target SuperVBlock %d TS %d valid check error-1\n",
                                            ulSuperPU, uLUNInSuperPU, BlkSN, CurrTS);
                                //DBG_Getch();
                            }
                        }

                    }

                    else
                    {
                        if (CurrTS < pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddrTmp].LastPageTimeStamp)
                        {
                            DBG_Printf("L2_PrepareToRebuildPMT SPU %d LUN %d target SuperVBlock %d TS %d valid check error-2\n",
                                            ulSuperPU, uLUNInSuperPU, BlkSN, CurrTS);
                            DBG_Getch();
                        }
                    }
                }

                /* clip by last page TS */
                /*if ((CurrTS > l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU])
                    || (BlkSN == L2_GetPuTargetVBN(ulSuperPU, ucTargetType)) || (0 == CurrTS))*/
                if (CurrTS > l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU])
                {
                    U32 ulSmallestFirstPageTS = INVALID_8F;

                    RebuildBlockCnt = l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucTargetType];
                    l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucTargetType][RebuildBlockCnt] = BlkSN;

                    // calc SLC first page smallest ts
                    if (BlkSN == L2_GetPuTargetVBN(ulSuperPU, ucTargetType))
                    {
                        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                        {
                            if (1 == L2_GetPuTargetPPO(ulSuperPU, ucTargetType) &&
                                0 != g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucTargetType] &&
                                !COM_BitMaskGet(g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucTargetType], uLUNInSuperPU))
                            {
                                continue;
                            }

                            usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                            if (pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddr].TimeStamp < ulSmallestFirstPageTS)
                            {
                                ulSmallestFirstPageTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddr].TimeStamp;
                            }
                        }
                    }
                    else
                    {
                        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                        {
                            usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                            if (pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddr].TimeStamp < ulSmallestFirstPageTS)
                            {
                                ulSmallestFirstPageTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddr].TimeStamp;
                            }
                        }
                    }

                    if (ucTargetType != TARGET_TLC_WRITE)
                    {
                        /*SLC last page, target block or error handled block with last page ts not right*/
                        l_RebuildPMTDptr->m_RebuildBlkTS[ulSuperPU][ucTargetType][RebuildBlockCnt] = ulSmallestFirstPageTS;
                    }
                    else
                    {
                        /* sort by last page TS,internal copy cause first page TS  not real*/
                        l_RebuildPMTDptr->m_RebuildBlkTS[ulSuperPU][ucTargetType][RebuildBlockCnt] = CurrTS;
                    }

                    if (TARGET_TLC_WRITE == ucTargetType)
                    {
                        /*select the smallest FirstPageRealTS of one superpage */
                        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                        {
                            usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, BlkSN);
                            if (pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddr].FirstPageRealTS < uMinTLCBlkTS)
                            {
                                uMinTLCBlkTS = pPBIT[ulSuperPU]->m_PBIT_Info[uLUNInSuperPU][usPhyBlkAddr].FirstPageRealTS;
                            }
                        }
                    }

                    l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucTargetType]++;
                }
            }
        }
    }

    if (bBypassTSAdjust == FALSE)
    {
        if (uMinTLCBlkTS < l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU])
        {
            l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU] = uMinTLCBlkTS;
            bBypassTSAdjust = TRUE;

            /*reset count*/
            for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
            {
                l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucTargetType] = 0;
            }
            goto RECAL;
        }
    }

    SortByTS(ulSuperPU);

    //ClipByTSAndBlockType(ulSuperPU);

    DBG_Printf("SPU %d Rebuild SeqBlockCnt %d RandBlockCnt %d TLCWriteBlockCnt %d.\n", ulSuperPU,
        l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][TARGET_HOST_WRITE_NORAML], l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][TARGET_HOST_GC],
        l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][TARGET_TLC_WRITE]);

    //TRACE_LOG((void*)&ulSuperPU, sizeof(U32), U32, 0, "[L2_PrepareToRebuildPMT]: PU ? ");
    //TRACE_LOG((void*)&l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][WRITE_TYPE_SEQ], sizeof(U32), U32, 0, "[L2_PrepareToRebuildPMT]: Rebuild Seq block count ? ");
    //TRACE_LOG((void*)&l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][WRITE_TYPE_RND], sizeof(U32), U32, 0, "[L2_PrepareToRebuildPMT]: Rebuild Rnd block count ? ");

    return;
}

//return TRUE, Finish;
//return FALSE, Not Finish
BOOL MCU1_DRAM_TEXT L2_RebuildLoadPMT()
{
    U32 ulSuperPU;
    U32 uLUNInSuperPU;
    U32 ulPMTIndexInPu;
    PhysicalAddr Addr = { 0 };
    U32 j;
    BOOL bInvalidePMTPage;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++) 
    {         
        while(l_RebuildPMTDptr->m_LoadedPMTIIndex[ulSuperPU] < PMTPAGE_CNT_PER_SUPERPU)
        {            
            //get PMTIndex to Load
            bInvalidePMTPage = FALSE;
            ulPMTIndexInPu = l_RebuildPMTDptr->m_LoadedPMTIIndex[ulSuperPU];
                       
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                Addr.m_PPN = L2_GetPMTPhysicalAddr(ulSuperPU, uLUNInSuperPU, ulPMTIndexInPu);
                if (Addr.m_PPN == INVALID_8F)
                {
                    bInvalidePMTPage = TRUE;
                    break;
                }
            }
            if (bInvalidePMTPage)
            {
                for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    /* clear PMT page in DRAM */
                    for (j = 0; j < PMT_PAGE_SIZE / sizeof(U32); j++)
                    {
#ifdef PMT_ITEM_SIZE_REDUCE
                        *((U32*)&((SuperPage*)g_PMTManager->m_pPMTPage[ulSuperPU][ulPMTIndexInPu])->m_Content[uLUNInSuperPU] + j) = INVALID_8F;
#else
                        *((U32*)&g_PMTManager->m_pPMTPage[ulSuperPU][ulPMTIndexInPu]->m_Page.m_Content[uLUNInSuperPU] + j) = INVALID_8F;
#endif
                    }
                }
                l_RebuildPMTDptr->m_LoadedPMTIIndex[ulSuperPU] ++;
                continue;
            }
            if (TRUE == L2_LoadPMTPage(ulSuperPU, ulPMTIndexInPu))
            {
                //Next PMTIndex
                l_RebuildPMTDptr->m_LoadedPMTIIndex[ulSuperPU] ++;
            }
            else
            {
                break;
            }
        }
    }

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        if (l_RebuildPMTDptr->m_LoadedPMTIIndex[ulSuperPU] < PMTPAGE_CNT_PER_SUPERPU)
        {
            return FALSE;
        }
    }

    //wait all finish
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            if (SUBSYSTEM_STATUS_PENDING == g_PMTManager->m_FlushStatus[ulSuperPU][uLUNInSuperPU][PMTPAGE_CNT_PER_SUPERPU - 1])
            {
                return FALSE;
            }
        }
    }
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndexInPu++)
            {
                if (SUBSYSTEM_STATUS_FAIL == g_PMTManager->m_FlushStatus[ulSuperPU][uLUNInSuperPU][ulPMTIndexInPu])
                {
                    DBG_Printf("MCU#%d SPU %d LUN %d PMTIndexInPu %d In table rebuild,Load PMT Fail\n",
                                HAL_GetMcuId(), ulSuperPU, uLUNInSuperPU, ulPMTIndexInPu);
                    g_bPMTErrFlag = TRUE;
                    return TRUE;
                }
            }
        }
    }    
    g_bPMTErrFlag = FALSE;
    return TRUE;
}

//Find two least values in vector,
//return value: if the least value is 0xFFFFFFFF, return FALSE;
BOOL MCU1_DRAM_TEXT FindLeastSN(U32* Pos1, U32* Pos2, U32* pValue, U32 Cnt)
{
    U32 i;
    if (Cnt < 2)
    {
        DBG_Printf("The buffer cnt is less than 2, cannot load RPMT for rebuild");
        DBG_Getch();
    }

    *Pos1 = 0;
    *Pos2 = 1;

    if (pValue[*Pos1] > pValue[*Pos2])
    {
        SwapTwoValue(Pos1, Pos2);
    }

    for (i = 2; i < Cnt; i++)
    {
        U32 CurrValue = pValue[i];


        if (CurrValue > pValue[*Pos2])
        {
            continue;
        }
        if (CurrValue < pValue[*Pos1])
        {
            *Pos2 = *Pos1;
            *Pos1 = i;
        }
        else
        {
            *Pos2 = i;
        }
    }

    if (pValue[*Pos1] == INVALID_8F)
        return FALSE;
    else
        return TRUE;
}

void L2_LoadSpare(PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, BOOL bSLCMode)
{
    L2_FtlReadLocal(NULL, pAddr, pStatus, pSpare, 0, 0, FALSE, bSLCMode);
}

void L2_TableLoadSpare(PhysicalAddr* pAddr, U8* pStatus, U32* pSpare)
{
    L2_FtlReadLocal(NULL, pAddr, pStatus, pSpare, 0, 0, TRUE, TRUE);
}

/*****************************************************************************
 Prototype      : L2_RebuildLoadRPMT
 Description    : load RPMT for rebuild block
 Input          : None
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/11
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_RebuildLoadRPMT(U8 ucSuperPU)
{
    RPMT* pBufferAddr = NULL;
    U8* pStatus = NULL;
    U8 ucWriteTarget, ucTargetTmp;
    U16 usCurrBlkSN;
    U16 usCurrBlkCnt;
    BOOL bRet = TRUE;
    U32 uLUNInSuperPU = 0;

    /* load all write target rebuild block's RPMT page for rebuild */
    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            /* if this target have block to rebuild, then load it's RPMT page */
            if (TRUE == l_RebuildPMTDptr->m_NeedLoadRPMT[ucSuperPU][ucWriteTarget][uLUNInSuperPU])
            {
                if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPU, uLUNInSuperPU)))
                {
                    return FALSE;
                }

                usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ucSuperPU][ucWriteTarget];
                if (usCurrBlkSN >= l_RebuildPMTDptr->m_RebuildBlkCnt[ucSuperPU][ucWriteTarget])
                {
                    DBG_Printf("SPU %d LUN %d Target %d rebuild block SN %d overflowed!\n",ucSuperPU, uLUNInSuperPU, ucWriteTarget, usCurrBlkSN);
                    DBG_Getch();
                }
                usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ucSuperPU][ucWriteTarget][usCurrBlkSN];

                //is TLC write target
                if (TRUE == L2_VBT_Get_TLC(ucSuperPU, usCurrBlkCnt))
                {
                    U32 uTLCCurWordLineOffset = l_BufferManagerDptr->m_TLCCurWordLineOffset[ucSuperPU][uLUNInSuperPU];

                    //Get TLC buffer
                    pBufferAddr = (RPMT*)l_BufferManagerDptr->m_TLCPageBuffer[ucSuperPU][uTLCCurWordLineOffset];
                    pStatus = (U8*)&l_BufferManagerDptr->m_TLCPageBufferStatus[ucSuperPU][uTLCCurWordLineOffset][uLUNInSuperPU];

                    L2_LoadRPMTInLun(pBufferAddr, ucSuperPU, uLUNInSuperPU, usCurrBlkCnt, (U8*)pStatus, FALSE, uTLCCurWordLineOffset);

                    //DBG_Printf(" ce %d load block %d RPMT .\n", ucTLun, usCurrBlkCnt);

                    /* set use RPMT flag temporarily, if load fail, this flag would be set FLASE */
                    l_RebuildPMTDptr->m_UseRPMTFlag[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = TRUE;

                    uTLCCurWordLineOffset++;
                    l_BufferManagerDptr->m_TLCCurWordLineOffset[ucSuperPU][uLUNInSuperPU] = uTLCCurWordLineOffset;

                    //if TLC Word Line Finish
                    if (uTLCCurWordLineOffset >= PG_PER_WL)
                    {
                        //clear TLCWordLineOffset
                        l_BufferManagerDptr->m_TLCCurWordLineOffset[ucSuperPU][uLUNInSuperPU] = 0;

                        l_RebuildPMTDptr->m_NeedLoadRPMT[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = FALSE;
                    }
                }
                else
                {
                    pBufferAddr = (RPMT *)l_BufferManagerDptr->m_PageBuffer[ucSuperPU][ucWriteTarget];
                    pStatus = &l_BufferManagerDptr->m_BufferStatus[ucSuperPU][ucWriteTarget][uLUNInSuperPU];

                    /* Target Blk no need load RPMT again, L2 fullrecoery already load to PuInfo, just copy to rebuild buffer */
                    if (L2_IsTargetBlk(ucSuperPU, usCurrBlkCnt, &ucTargetTmp))
                    {
                        #ifdef SIM
                        if (ucTargetTmp != ucWriteTarget)
                        {
                            DBG_Printf("%s : SPU %d VBLK %d target %d error", __FUNCTION__, ucSuperPU, usCurrBlkCnt, ucTargetTmp);
                            DBG_Getch();
                        }
                        #endif

                        HAL_DMAECopyOneBlock((U32)&pBufferAddr->m_RPMT[uLUNInSuperPU], (U32)&g_PuInfo[ucSuperPU]->m_pRPMT[ucWriteTarget][0]->m_RPMT[uLUNInSuperPU], sizeof(RPMT_PER_LUN));
                        *pStatus = SUBSYSTEM_STATUS_SUCCESS;
                    }
                    else
                    {
                        L2_LoadRPMTInLun(pBufferAddr, ucSuperPU, uLUNInSuperPU, usCurrBlkCnt, (U8*)pStatus, TRUE, 0);

                        //FIRMWARE_LogInfo("%s SPU %d LUN %d load block %d RPMT, pBufferAddr 0x%x pStatus %d\n",__FUNCTION__,
                        //    ucSuperPU, uLUNInSuperPU, usCurrBlkCnt, pBufferAddr, *pStatus);
                    }

                    /* set use RPMT flag temporarily, if load fail, this flag would be set FLASE */
                    l_RebuildPMTDptr->m_UseRPMTFlag[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = TRUE;
                    l_RebuildPMTDptr->m_NeedLoadRPMT[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = FALSE;
                }
            }
        }
    }

    /* check all write target RPMT load cmd send */
    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            if (TRUE == l_RebuildPMTDptr->m_NeedLoadRPMT[ucSuperPU][ucWriteTarget][uLUNInSuperPU])
            {
                bRet = FALSE;
            }
        }
    }

    return bRet;
}

/*****************************************************************************
 Prototype      : L2_IsRPMTLoadDone
 Description    : wait all target block RPMT load done.
 Input          : U8 ucTLun
 Output         : None
 Return Value   : BOOL
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/12
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
BOOL L2_IsTLCRPMTLoadFinish(U32 ulSuperPU, U32 uLUNInSuperPU)
{
    U32 bFinishFlag = TRUE;
    U32 uWordLineOffset;

    for (uWordLineOffset = 0; uWordLineOffset < PG_PER_WL; uWordLineOffset++)
    {
        if (SUBSYSTEM_STATUS_PENDING == l_BufferManagerDptr->m_TLCPageBufferStatus[ulSuperPU][uWordLineOffset][uLUNInSuperPU])
        {
            bFinishFlag = FALSE;
            break;
        }
    }

    return bFinishFlag;
}

void L2_InitTLCRPMTStatus(U32 ulSuperPU, U32 uLUNInSuperPU)
{
    U32 uWordLineOffset;

    for (uWordLineOffset = 0; uWordLineOffset < PG_PER_WL; uWordLineOffset++)
    {
        l_BufferManagerDptr->m_TLCPageBufferStatus[ulSuperPU][uWordLineOffset][uLUNInSuperPU] = SUBSYSTEM_STATUS_SUCCESS;
    }
}

BOOL L2_IsTLCRPMTLoadErr(U32 ulSuperPU, U32 uLUNInSuperPU)
{
    U32 bErrFlag = FALSE;
    U32 uWordLineOffset;

    for (uWordLineOffset = 0; uWordLineOffset < PG_PER_WL; uWordLineOffset++)
    {
        if (SUBSYSTEM_STATUS_SUCCESS != l_BufferManagerDptr->m_TLCPageBufferStatus[ulSuperPU][uWordLineOffset][uLUNInSuperPU]
        && SUBSYSTEM_STATUS_RETRY_SUCCESS != l_BufferManagerDptr->m_TLCPageBufferStatus[ulSuperPU][uWordLineOffset][uLUNInSuperPU]
        && SUBSYSTEM_STATUS_RECC != l_BufferManagerDptr->m_TLCPageBufferStatus[ulSuperPU][uWordLineOffset][uLUNInSuperPU])
        {
            bErrFlag = TRUE;
            break;
        }
    }

    return bErrFlag;
}

//return TRUE when finish
/* TLC not enter */
BOOL MCU1_DRAM_TEXT L2_RPMTLoadErrHanding(U8 ucSuperPU, U32 uLUNInSuperPU, U8 ucWriteTarget)
{
    U16 usCurrBlkCnt;
    U16 usCurrBlkSN;
    U16 usTargetPPO;
    BOOL bSLCMode = TRUE;
    U32 PPO;
    PhysicalAddr Addr = { 0 };
    RED* pSpare;
    U8* pStatus;
    BOOL ret = FALSE;


    if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPU, uLUNInSuperPU)))
    {
        return FALSE;
    }

    usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ucSuperPU][ucWriteTarget];
    usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ucSuperPU][ucWriteTarget][usCurrBlkSN];

    //TLC RPMT process specially
    if (TARGET_TLC_WRITE == ucWriteTarget)
    {
        DBG_Printf("L2_RPMTLoadErrHanding:TLC should not enter this function.\n");
        DBG_Getch();
        usTargetPPO = (PG_PER_SLC_BLK * PG_PER_WL - 4);
        bSLCMode = FALSE;
    }
    else
    {
        /* acturally, we only need to load to the last writen page(PPO) on the target */
        if (usCurrBlkCnt == L2_GetPuTargetVBN(ucSuperPU, ucWriteTarget))
        {
            usTargetPPO = L2_GetPuTargetPPO(ucSuperPU, ucWriteTarget);
            /* SuperPage target PPO-1 may partial write (wait for full recovery recover bitmap ok )==Rock*/
            if (0 != g_PuInfo[ucSuperPU]->m_TargetOffsetBitMap[ucWriteTarget])
            {
                if (!(g_PuInfo[ucSuperPU]->m_TargetOffsetBitMap[ucWriteTarget] & (1 << uLUNInSuperPU)))
                {
                    usTargetPPO--;
                }
            }
        }
        else
        {
            /* last page(RPMT) must be read fail, no need to read */
            usTargetPPO = (PG_PER_SLC_BLK - 2);
        }
    }

    PPO = l_RebuildPMTDptr->m_LoadSparePPO[ucSuperPU][ucWriteTarget][uLUNInSuperPU];
    if (PPO <= usTargetPPO)
    {
        /* set the addr for load spare */
        Addr.m_PUSer = ucSuperPU;
        Addr.m_OffsetInSuperPage = uLUNInSuperPU;
        Addr.m_BlockInPU = usCurrBlkCnt;
        Addr.m_PageInBlock = PPO;
        Addr.m_LPNInPage = 0;

        /* pay attention to here, because the whole block spare is 64K, so m_PageBuffer must be 64K size */
        pSpare = (RED*)l_BufferManagerDptr->m_SpareBuffer[ucSuperPU][ucWriteTarget][PPO][uLUNInSuperPU];
        pStatus = &l_RebuildPMTDptr->m_LoadSpareStatus[ucSuperPU][ucWriteTarget][PPO][uLUNInSuperPU];

        L2_LoadSpare(&Addr, pStatus, (U32*)pSpare, bSLCMode);        

        PPO++;
        l_RebuildPMTDptr->m_LoadSparePPO[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = PPO;
    }
    else   //Finish
    {
        pStatus = &l_RebuildPMTDptr->m_LoadSpareStatus[ucSuperPU][ucWriteTarget][usTargetPPO][uLUNInSuperPU];

        //wait for all data Spare finish
        if (SUBSYSTEM_STATUS_PENDING == *pStatus)
        {
            //Wait
        }
        else
        {
            //Set Not use RPMT flag
            l_RebuildPMTDptr->m_UseRPMTFlag[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = FALSE;
            l_RebuildPMTDptr->m_LoadSparePPO[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = 0;

            //Init Flash RPMT status
            l_BufferManagerDptr->m_BufferStatus[ucSuperPU][ucWriteTarget][uLUNInSuperPU] = SUBSYSTEM_STATUS_SUCCESS;

            //Finish
            ret = TRUE;
        }
    }

    return ret;
}



BOOL MCU1_DRAM_TEXT L2_IsRPMTLoadDone(U8 ulSuperPU)
{
    U32 AccomplishCnt = 0;
    U8 ucWriteTarget;
    U16 usCurrBlkSN;
    U16 usCurrBlkCnt;
    PhysicalAddr Addr = { 0 };
    RPMT* pRPMT;
    U32 uLUNInSuperPU = 0;

    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        //This target all blk RPMT Load Finish
        if (TRUE == l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucWriteTarget])
        {
            AccomplishCnt += LUN_NUM_PER_SUPERPU;
            continue;
        }

        //This target blk RPMT load not Finish

        //TLC Write Target Special Process
        if (TARGET_TLC_WRITE == ucWriteTarget)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if (TRUE == L2_IsTLCRPMTLoadFinish(ulSuperPU, uLUNInSuperPU))
                {
                    //Load TLC RPMT success
                    if (TRUE != L2_IsTLCRPMTLoadErr(ulSuperPU, uLUNInSuperPU))
                    {
                        usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
                        usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurrBlkSN];
                        pRPMT = (RPMT*)l_BufferManagerDptr->m_TLCPageBuffer[ulSuperPU][0];// only check WL 0
                        if (pRPMT->m_RPMT[uLUNInSuperPU].m_SuperPU != ulSuperPU || pRPMT->m_RPMT[uLUNInSuperPU].m_SuperBlock != usCurrBlkCnt)
                        {
                            DBG_Printf("Load SPU %d LUN %d TLC Blk %d Target %d RPMT error!!!\n", ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, ucWriteTarget);
                            DBG_Printf("RPMT SPU %d Blk %d\n", pRPMT->m_RPMT[uLUNInSuperPU].m_SuperPU, pRPMT->m_RPMT[uLUNInSuperPU].m_SuperBlock);
                            DBG_Getch();
                        }
                    }
                    else  //TLC RPMT error handling
                    {
                        l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU] = RPMT_UECC_BYPASSS; //UECC Lun ,flag set to= 2
                        usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
                        usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurrBlkSN];

                        L2_RebuildAddErrTLCBlk(ulSuperPU, uLUNInSuperPU, usCurrBlkCnt);

                        L2_CollectErrInfoOfRebuild(ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, (PG_PER_SLC_BLK - 1), TLC_RPMT_ERR);
                        DBG_Printf("SPU %d TLC block %d Lun %d, load RPMT fail.\n", ulSuperPU, usCurrBlkCnt, uLUNInSuperPU);

                    }
                    AccomplishCnt++;
                }
            }
        }
        else
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                /* wait RPMT load cmd done */
                if (l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU] == SUBSYSTEM_STATUS_PENDING)
                {
                    break;
                }
                else if (l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU] == SUBSYSTEM_STATUS_SUCCESS
                    || l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU] == SUBSYSTEM_STATUS_RECC
                    || l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU] == SUBSYSTEM_STATUS_RETRY_SUCCESS)
                {
                    /* RPMT page check */
                    if ((TRUE == l_RebuildPMTDptr->m_UseRPMTFlag[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                        && (TRUE != l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucWriteTarget]))
                    {
                        usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
                        usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurrBlkSN];
                        pRPMT = (RPMT*)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][ucWriteTarget];
                        if (pRPMT->m_RPMT[uLUNInSuperPU].m_SuperPU != ulSuperPU || pRPMT->m_RPMT[uLUNInSuperPU].m_SuperBlock != usCurrBlkCnt)
                        {
                            DBG_Printf("Load SPU %d LUN %d SLC Blk %d Target %d RPMT error!!!\n", ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, ucWriteTarget);
                            DBG_Printf("RPMT PU %d Blk %d\n", pRPMT->m_RPMT[uLUNInSuperPU].m_SuperPU, pRPMT->m_RPMT[uLUNInSuperPU].m_SuperBlock);
                            DBG_Getch();
                        }
                    }

                    AccomplishCnt++;
                }
                /* if RPMT load fail, load all page spare to recovery RPMT */
                else if (l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU] == SUBSYSTEM_STATUS_EMPTY_PG
                    || l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU] == SUBSYSTEM_STATUS_FAIL)
                {
                    /* check L3 resource for load spare */
                    if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulSuperPU, uLUNInSuperPU)))
                    {
                        return FALSE;
                    }

                    usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
                    usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurrBlkSN];

                    //RPMT load Fail err hanling process
                    if (TRUE == L2_RPMTLoadErrHanding(ulSuperPU, uLUNInSuperPU, ucWriteTarget))
                    {
                        AccomplishCnt++;

                        /* Just Collect err info  */
                        if (SUBSYSTEM_STATUS_FAIL == l_BufferManagerDptr->m_BufferStatus[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                        {
                            L2_CollectErrInfoOfRebuild(ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, (PG_PER_SLC_BLK - 1), RPMT_ERR);
                            DBG_Printf("SPU %d LUN %d block %d, load RPMT fail.\n", ulSuperPU, uLUNInSuperPU, usCurrBlkCnt);
                        }
                    }

                }
                else
                {
                    DBG_Printf("the Read Status is not supported in current version");
                    DBG_Getch();
                }
            }
        }
    }


    if (AccomplishCnt == TARGET_ALL*LUN_NUM_PER_SUPERPU)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*****************************************************************************
 Prototype      : L2_RebuildUpdateInfo
 Description    :
 Input          : U8 PUSer
 U8 ucTarget
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/12
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
BOOL L2_IsTargetNeedLoadNextRPMT(U8 ulSuperPU, U8 ucTarget, U16 CurrPos)
{
    U16 usCurrBlkSN, usCurrBlkCnt;
    U16 usTargetPPO;

    usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucTarget];
    usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucTarget][usCurrBlkSN];

    //TLC write target spicial process
    if (TARGET_TLC_WRITE == ucTarget)
    {
        usTargetPPO = (PG_PER_SLC_BLK*PG_PER_WL - 4);
    }
    else if (usCurrBlkCnt == L2_GetPuTargetVBN(ulSuperPU, ucTarget))
    {
        usTargetPPO = L2_GetPuTargetPPO(ulSuperPU, ucTarget)-1;
    }
    else
    {
        /* last page(RPMT) must be read fail, no need to read */
        usTargetPPO = (PG_PER_SLC_BLK - 2);
    }

    if (usTargetPPO <= CurrPos)
    {
        /* increase current rebuild block SN */
        usCurrBlkSN++;
        if (usCurrBlkSN >= l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucTarget])
        {
            /* this target block finished */
            l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucTarget] = usCurrBlkSN;
            l_RebuildPMTDptr->m_CurrRebuildPPO[ulSuperPU][ucTarget] = CurrPos + 1;
            l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucTarget] = TRUE;
        }
        else
        {
            //DBG_Printf("ce %d target %d block %d rebuild done.\n", PUSer, ucTarget, l_RebuildPMTDptr->m_CurrRebuildBlkSN[PUSer][ucTarget]);

            /* need load next RPMT on this target */
            l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucTarget] = usCurrBlkSN;
            l_RebuildPMTDptr->m_CurrRebuildPPO[ulSuperPU][ucTarget] = 0;

            return TRUE;
        }
    }
    else
    {
        /* update current rebuild PPO on this target */
        l_RebuildPMTDptr->m_CurrRebuildPPO[ulSuperPU][ucTarget] = CurrPos + 1;
    }

    return FALSE;
}

extern GLOBAL MCU12_VAR_ATTR INVERSE_TLC_SHARED_PAGE l_aTLCInverseProgOrder[];
/*****************************************************************************
 Prototype      : L2_RebuildPMTByRPMT
 Description    :
 Input          : U32 PUSer
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/12
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void MCU1_DRAM_TEXT L2_RebuildPMTByRPMT(U32 ulSuperPU)
{
    RPMT_PER_LUN * pRPMTPerLUN[LUN_NUM_PER_SUPERPU][TARGET_ALL];
    RED* pSpare[LUN_NUM_PER_SUPERPU][TARGET_ALL];
    U32 CurrPos[TARGET_ALL];
    U32* pSrcLPN[LUN_NUM_PER_SUPERPU][TARGET_ALL];
    U32 TS[TARGET_ALL][LUN_NUM_PER_SUPERPU];
    U32 TSOrder[TARGET_ALL][LUN_NUM_PER_SUPERPU];
    PhysicalAddr Addr = { 0 };
    U32 LPN;
    RED* pCurrSpare;
    U32 ulRPMTIndex, ulPageIndex;

    U16 usCurrBlkSN;
    U16 usCurrBlkCnt;
    U8 ucWriteTarget;
    U8 ucMinTsTarget;
    U32 i;
    U32 AccomplishCnt = 0;
    U32 ulPMTIndexInPu;
    U16 usPagePerBlk;

    U32* pSrcLPNOfTLC[PG_PER_WL][LUN_NUM_PER_SUPERPU];
    U32 TSOfTLC[PG_PER_WL][LUN_NUM_PER_SUPERPU];
    U32 TSOrderOfTLC[PG_PER_WL][LUN_NUM_PER_SUPERPU];
    U32 uWordLineOffset;
    U32 uLUNInSuperPU;
#ifndef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
    U32 uLoop;
#endif
    U32 uTSLUNUse[TARGET_ALL] = { 0 };
    
    U32 ulPageNum[PG_PER_WL];    
    U8 ucPageType;
    U8 ucProgPageCnt;

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    /* Get all target block's Ts & pSrcLPN[8] at current pos */
    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        /* if this target rebuild done, set TS to MAX */
        if (TRUE == l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucWriteTarget])
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                TS[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                TSOrder[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
            }
            AccomplishCnt++;
            continue;
        }

        //get cur blk
        //for TLC, CurrPos[ucWriteTarget] is CurWordLine
        CurrPos[ucWriteTarget] = l_RebuildPMTDptr->m_CurrRebuildPPO[ulSuperPU][ucWriteTarget];
        usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
        usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurrBlkSN];

        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            //TLC Write Process specially
            if (TRUE == L2_VBT_Get_TLC(ulSuperPU, usCurrBlkCnt))
            {
                //TLC Block RPMT right process
                if (TRUE == l_RebuildPMTDptr->m_UseRPMTFlag[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                {                   
                    U32 uPageInWordLine;
                    U32 uPageInWordLineStart;
                    U32 uPageInWordLineEnd;                    
                    RPMT* pRPMTTemp;

                    if (BYPASS == l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU] ||
                        RPMT_UECC_BYPASSS == l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                    {
                        TS[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                        TSOrder[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                        continue;
                    }                                   

                ulPageNum[0] = CurrPos[ucWriteTarget];
                ucPageType = (U8)l_aTLCInverseProgOrder[CurrPos[ucWriteTarget]].m_PageType;             
                ucProgPageCnt = (U8)l_aTLCInverseProgOrder[CurrPos[ucWriteTarget]].m_ProgPageCnt;
                    if (ucProgPageCnt == 2)
                   ulPageNum[1] = l_aTLCInverseProgOrder[CurrPos[ucWriteTarget]].m_2ndPageNum; 

                    if (L2_LOWER_PAGE == ucPageType || L2_LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {                    
                    uPageInWordLineStart = 0;
                    uPageInWordLineEnd = ucProgPageCnt;
                }
                else if (L2_EXTRA_PAGE == ucPageType)
                {
                    #ifdef FLASH_IM_3DTLC_GEN2
                        uPageInWordLineStart = 1;
                        ulPageNum[L2_UPPER_PAGE] = ulPageNum[1];
                    #else
                    uPageInWordLineStart = 2;
                    #endif
                    uPageInWordLineEnd = 3;                    
                    ulPageNum[L2_EXTRA_PAGE] = ulPageNum[0];
                }
                else
                {
                        DBG_Printf("SPU %d PageType error\n", ulSuperPU);
                    DBG_Getch();
                }                                                                   

                    //now, TLC is a WordLine , so sepcial process
                    for (uPageInWordLine = uPageInWordLineStart; uPageInWordLine < uPageInWordLineEnd; uPageInWordLine++)
                    {                        
                        ulRPMTIndex = L2_GetInterCopySrcBlkIndex(ulPageNum[uPageInWordLine]);
                        ulPageIndex = L2_GetInterCopyPgIndex(ulPageNum[uPageInWordLine]);

                        pRPMTTemp = (RPMT*)l_BufferManagerDptr->m_TLCPageBuffer[ulSuperPU][ulRPMTIndex];
                        TSOfTLC[uPageInWordLine][uLUNInSuperPU] = pRPMTTemp->m_RPMT[uLUNInSuperPU].m_SuperPageTS[ulPageIndex];
                        TSOrderOfTLC[uPageInWordLine][uLUNInSuperPU] = pRPMTTemp->m_RPMT[uLUNInSuperPU].m_LunOrderTS[ulPageIndex];
                        pSrcLPNOfTLC[uPageInWordLine][uLUNInSuperPU] = &(pRPMTTemp->m_RPMT[uLUNInSuperPU].m_RPMTItems[ulPageIndex * LPN_PER_BUF]);                    
                    }                    

                    //each target common ts
                    TS[ucWriteTarget][uLUNInSuperPU] = TSOfTLC[uPageInWordLineStart][uLUNInSuperPU];
                    if (INVALID_8F == TS[ucWriteTarget][uLUNInSuperPU])
                    {
                        DBG_Printf("SPU %d LUN %d target %d TS = 0xFFFFFFFF !!!\n", ulSuperPU, uLUNInSuperPU, ucWriteTarget);
                        DBG_Getch();
                    }
                    //each target offset ts
                    TSOrder[ucWriteTarget][uLUNInSuperPU] = TSOrderOfTLC[uPageInWordLineStart][uLUNInSuperPU];
                }
                else  //TLC Block RPMT err process
                {
                    /* just bypass the rpmt page error tlc block and will be processed later */
                    DBG_Printf("L2_RebuildPMTByRPMT:TLC should not enter this function.\n");
                    DBG_Getch();
                        }

                //FIRMWARE_LogInfo("Pu %d Blk %d Page %d TimeStame %d\n",PUSer, usCurrBlkCnt,PG_PER_SLC_BLK-1,pSrcLPNOfTLC[0]);

                //TLC target finish, process next target
                continue;
            }
            else /* SLC process branch */
            {
                if (ucWriteTarget == TARGET_TLC_WRITE)
                {
                    DBG_Printf("Target %d wrong target type! \n", ucWriteTarget);
                    DBG_Getch();
                }
                
                if (BYPASS == l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU] ||
                    RPMT_UECC_BYPASSS == l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                {
                    TS[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                    TSOrder[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                    continue;
                }

                //Not TLC Block RPMT right process
                if (TRUE == l_RebuildPMTDptr->m_UseRPMTFlag[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                {
                    pSpare[uLUNInSuperPU][ucWriteTarget] = NULL;
                    pRPMTPerLUN[uLUNInSuperPU][ucWriteTarget] = (RPMT_PER_LUN *)&(((RPMT *)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][ucWriteTarget])->m_RPMT[uLUNInSuperPU]);

                    TS[ucWriteTarget][uLUNInSuperPU] = pRPMTPerLUN[uLUNInSuperPU][ucWriteTarget]->m_SuperPageTS[CurrPos[ucWriteTarget]];
                    TSOrder[ucWriteTarget][uLUNInSuperPU] = pRPMTPerLUN[uLUNInSuperPU][ucWriteTarget]->m_LunOrderTS[CurrPos[ucWriteTarget]];
                    pSrcLPN[uLUNInSuperPU][ucWriteTarget] = &(pRPMTPerLUN[uLUNInSuperPU][ucWriteTarget]->m_RPMTItems[CurrPos[ucWriteTarget] * LPN_PER_BUF]);

                    #if 0
                    if (INVALID_8F == TS[ucWriteTarget][uLUNInSuperPU])
                    {
                        DBG_Printf("SPU %d LUN %d Blk 0x%x Pg %d target %d TS = 0xFFFFFFFF\n", ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, CurrPos[ucWriteTarget], ucWriteTarget);                        
                    }
                    #endif
                }
                else
                {
                    //pSpare[uLUNInSuperPU][ucWriteTarget] = (RED*)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][ucWriteTarget];                    
                    pRPMTPerLUN[uLUNInSuperPU][ucWriteTarget] = NULL;

                    /* this target block finished */
                    if (SUBSYSTEM_STATUS_EMPTY_PG == l_RebuildPMTDptr->m_LoadSpareStatus[ulSuperPU][ucWriteTarget][CurrPos[ucWriteTarget]][uLUNInSuperPU])
                    {
                        //DBG_Printf("ce %d target %d block %d rebuild done. curr ppo 0x%x.\n", PUSer, ucWriteTarget, l_RebuildPMTDptr->m_CurrRebuildBlkSN[PUSer][ucWriteTarget], CurrPos[ucWriteTarget]);

                        //l_RebuildPMTDptr->m_TargetFinished[PUSer][ucWriteTarget] = TRUE;
                        //l_RebuildPMTDptr->m_CurrRebuildBlkSN[PUSer][ucWriteTarget]++;

                        /* finish check on current target */
                        if (0 != L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget))
                        {
                            U16 usCurBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
                            U16 usCurBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurBlkSN];

                            if (0 == g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucWriteTarget])
                            {
                                if (CurrPos[ucWriteTarget] != L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget))
                                {
                                    DBG_Printf("1-SPU %d, lun %d target %d blk 0x%x empty page happened before ppo! CurrPos = 0x%x. PPO = 0x%x.\n", ulSuperPU, uLUNInSuperPU, ucWriteTarget,
                                        usCurBlkCnt, CurrPos[ucWriteTarget], L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget));
                                }
                            }
                            else
                            {
                                if (COM_BitMaskGet(g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[ucWriteTarget], uLUNInSuperPU))
                                {
                                    if (CurrPos[ucWriteTarget] != L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget))
                                    {
                                        DBG_Printf("2-SPU %d, lun %d target %d blk 0x%x empty page happened before ppo! CurrPos = 0x%x. PPO = 0x%x.\n", ulSuperPU, uLUNInSuperPU, ucWriteTarget,
                                            usCurBlkCnt, CurrPos[ucWriteTarget], L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget));
                                    }
                                }
                                else
                                {
                                    if (CurrPos[ucWriteTarget] + 1 != L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget))
                                    {
                                        DBG_Printf("3-SPU %d, lun %d target %d blk 0x%x empty page happened before ppo! CurrPos = 0x%x. PPO = 0x%x.\n", ulSuperPU, uLUNInSuperPU, ucWriteTarget,
                                            usCurBlkCnt, CurrPos[ucWriteTarget], L2_GetPuTargetPPO(ulSuperPU, ucWriteTarget));
                                    }
                                }
                            }
                        }

                        TS[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                        TSOrder[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                    }
                    /* Err handling when data spare read err */
                    else if (SUBSYSTEM_STATUS_FAIL == l_RebuildPMTDptr->m_LoadSpareStatus[ulSuperPU][ucWriteTarget][CurrPos[ucWriteTarget]][uLUNInSuperPU])
                    {                      
                        usPagePerBlk = PG_PER_SLC_BLK;                       

                        /* if last page err, should not get here */
                        if ((usPagePerBlk - 1) == CurrPos[ucWriteTarget])
                        {
                            DBG_Printf("SPU %d LUN %d target %d last page spare load fail!\n", ulSuperPU, uLUNInSuperPU, ucWriteTarget);
                            DBG_Getch();
                        }
                        else
                        {
                            usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget];
                            usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucWriteTarget][usCurrBlkSN];

                            /* Collect Err info */
                            L2_CollectErrInfoOfRebuild(ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, CurrPos[ucWriteTarget], DATA_SPARE_ERR);

                            TS[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                            TSOrder[ucWriteTarget][uLUNInSuperPU] = INVALID_8F;
                        }
                    }
                    else
                    {                        
                        pCurrSpare = (RED*)l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][ucWriteTarget][CurrPos[ucWriteTarget]][uLUNInSuperPU];                        
                        TS[ucWriteTarget][uLUNInSuperPU] = pCurrSpare->m_RedComm.ulTimeStamp;
                        TSOrder[ucWriteTarget][uLUNInSuperPU] = pCurrSpare->m_RedComm.ulTargetOffsetTS;
                        pSrcLPN[uLUNInSuperPU][ucWriteTarget] = pCurrSpare->m_DataRed.aCurrLPN;
                    }
                }
            }
        }
    }

    /* if all target finished, rebuild PMT done. */
    if (TARGET_ALL == AccomplishCnt)
    {
        for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
        {
            if (l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucWriteTarget] != l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucWriteTarget])
            {
                DBG_Printf("SPU %d target %d rebuild not done!\n", ulSuperPU, ucWriteTarget);
                DBG_Getch();
            }
        }

        l_RebuildPMTDptr->m_RebuildPMTStage[ulSuperPU] = REBUILD_PMT_STAGE_DONE;
        return;
    }
#ifndef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
    DBG_Printf("not support anymore\n");
    DBG_Getch();
#else
{
    U32 ulMinTSOrderLUN = 0;
    U32 aulTSInvLunCnt[TARGET_ALL] = { 0 };
    ucMinTsTarget = 0;

    /* search the earliest page in all target block */
    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            if (INVALID_8F == TS[ucWriteTarget][uLUNInSuperPU])
            {
                aulTSInvLunCnt[ucWriteTarget]++;
            }
            if (TS[ucWriteTarget][uLUNInSuperPU] < TS[ucMinTsTarget][ulMinTSOrderLUN])
            {
                ucMinTsTarget = ucWriteTarget;
                ulMinTSOrderLUN = uLUNInSuperPU;
            }
            else if (TS[ucWriteTarget][uLUNInSuperPU] == TS[ucMinTsTarget][ulMinTSOrderLUN])
            {
                if (TSOrder[ucWriteTarget][uLUNInSuperPU] < TSOrder[ucMinTsTarget][ulMinTSOrderLUN])
                {
                    ucMinTsTarget = ucWriteTarget;
                    ulMinTSOrderLUN = uLUNInSuperPU;
                }
            }
        }
    }

    if (TS[ucMinTsTarget][ulMinTSOrderLUN] != INVALID_8F)
    {
        usCurrBlkSN = l_RebuildPMTDptr->m_CurrRebuildBlkSN[ulSuperPU][ucMinTsTarget];
        usCurrBlkCnt = l_RebuildPMTDptr->m_RebuildBlk[ulSuperPU][ucMinTsTarget][usCurrBlkSN];        

        if (usCurrBlkCnt > VIR_BLK_CNT)
        {
            DBG_Printf("[%s]: SPU %d usCurrBlkCnt 0x%x overflowed! \n", __FUNCTION__ , ulSuperPU, usCurrBlkCnt);
            DBG_Getch();
        }

        if ((usCurrBlkSN >= l_RebuildPMTDptr->m_RebuildBlkCnt[ulSuperPU][ucMinTsTarget])
            || (TRUE == l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucMinTsTarget]))
        {
            DBG_Printf("[%s]: SPU %d, WriteType %d have rebuild done! usCurrBlkSN 0x%x overflowed! \n", __FUNCTION__ , ulSuperPU, ucMinTsTarget, usCurrBlkSN);
            //DBG_Printf("TS0 = 0x%x, TS1 = 0x%x, TS2 = 0x%x...,\n", TS[0][uLUNInSuperPU], TS[1][uLUNInSuperPU], TS[2][uLUNInSuperPU]);
            DBG_Getch();
        }

        uLUNInSuperPU = ulMinTSOrderLUN;
        //for(uLoop =0; uLoop < LUN_NUM_PER_SUPERPU; uLoop++)
        {
            U32 uCurTSOder = INVALID_8F;

            //Not TLC Block ,rebuild PMT
            if (FALSE == L2_VBT_Get_TLC(ulSuperPU, usCurrBlkCnt))
            {
                uCurTSOder = INVALID_8F;

                /* update PMT page */
                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    LPN = pSrcLPN[uLUNInSuperPU][ucMinTsTarget][i];
                    if (LPN == INVALID_8F)
                    {
                        continue;
                    }
                    else if (LPN >= MAX_LPN_IN_DISK)
                    {
                        DBG_Printf("Attention,Rebuild LPN 0x%x exceed MAX_LPN_IN_DISK 0x%x \n", LPN, MAX_LPN_IN_DISK);
                        DBG_Printf("SPU %d LUN %d usCurrBlkCnt 0x%x, WriteType %d, ppo 0x%x, offset %d, bUseRpmt %d.\n",
                            ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, ucMinTsTarget, CurrPos[ucMinTsTarget], i,
                            l_RebuildPMTDptr->m_UseRPMTFlag[ulSuperPU][ucMinTsTarget][uLUNInSuperPU]);
                        DBG_Printf("TS0 = 0x%x, TS1 = 0x%x, TS2 = 0x%x,\n", TS[0][uLUNInSuperPU], TS[1][uLUNInSuperPU], TS[2][uLUNInSuperPU]);
                        DBG_Getch();
                    }
                    else
                    {
                        /* compare data page TS with PMT page TS */
                        ulPMTIndexInPu = L2_GetPMTIIndexInPu(LPN);
                        //if (TS[ucMinTsTarget] < l_RebuildPMTIDptr->m_PMTPageTS[PUSer][ulPMTIndexInPu])
                        if (TS[ucMinTsTarget][uLUNInSuperPU] < l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU])
                        {
                            continue;
                        }
                        else
                        {
                            Addr.m_PUSer = ulSuperPU;
                            Addr.m_OffsetInSuperPage = uLUNInSuperPU;
                            Addr.m_BlockInPU = usCurrBlkCnt;
                            Addr.m_PageInBlock = CurrPos[ucMinTsTarget];
                            Addr.m_LPNInPage = i;

                            //FIRMWARE_LogInfo("SLC Rebuild LPN 0x%x => SPU %d LUN %d blk %d page %d offset %d Time %d,PMTTime %d m_RebuildClipTSInPu %d \n",
                            //    LPN, Addr.m_PUSer, uLUNInSuperPU, Addr.m_BlockInPU, Addr.m_PageInBlock, Addr.m_LPNInPage, TS[ucMinTsTarget], 
                            //    l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][ulPMTIndexInPu], l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU]);

                            L2_UpdatePMT(&Addr, NULL, LPN);
                        }
                    }
                }
            }
            //TLC Write Process specially
            else
            {     
                U32 uWordLineOffsetEnd;
                uCurTSOder = INVALID_8F;                

                ulPageNum[0] = CurrPos[ucMinTsTarget];
                ucPageType = (U8)l_aTLCInverseProgOrder[CurrPos[ucMinTsTarget]].m_PageType;             
                ucProgPageCnt = (U8)l_aTLCInverseProgOrder[CurrPos[ucMinTsTarget]].m_ProgPageCnt;
                    if (ucProgPageCnt == 2)
                   ulPageNum[1] = l_aTLCInverseProgOrder[CurrPos[ucMinTsTarget]].m_2ndPageNum;

                    if (L2_LOWER_PAGE == ucPageType || L2_LOW_PAGE_WITHOUT_HIGH == ucPageType)
                {
                    uWordLineOffset = 0;
                    uWordLineOffsetEnd = ucProgPageCnt;
                }
                else if (L2_EXTRA_PAGE == ucPageType)
                {
            #ifdef FLASH_IM_3DTLC_GEN2
                        uWordLineOffset = 1;
                        ulPageNum[L2_UPPER_PAGE] = ulPageNum[1];
            #else
                    uWordLineOffset = 2;
            #endif 
                    uWordLineOffsetEnd = 3;                    
                    ulPageNum[L2_EXTRA_PAGE] = ulPageNum[0];
                }
                else
                {
                    DBG_Printf("PageType error\n");
                    DBG_Getch();
                }                                        

                for (; uWordLineOffset < uWordLineOffsetEnd; uWordLineOffset++)
                {

                    /* update PMT page */
                    for (i = 0; i < LPN_PER_BUF; i++)
                    {
                        LPN = pSrcLPNOfTLC[uWordLineOffset][uLUNInSuperPU][i];
                        if (LPN == INVALID_8F)
                        {
                            continue;
                        }
                        else if (LPN >= MAX_LPN_IN_DISK)
                        {
                            DBG_Printf("Attention,Rebuild LPN 0x%x exceed MAX_LPN_IN_DISK 0x%x \n", LPN, MAX_LPN_IN_DISK);
                            DBG_Printf("SPU %d LUN %d usCurrBlkCnt 0x%x, WriteType %d, ppo 0x%x, offset %d, bUseRpmt %d.\n",
                                ulSuperPU, uLUNInSuperPU, usCurrBlkCnt, ucMinTsTarget, CurrPos[ucMinTsTarget], i,
                                l_RebuildPMTDptr->m_UseRPMTFlag[ulSuperPU][ucMinTsTarget][uLUNInSuperPU]);
                            DBG_Printf("TS0 = 0x%x, TS1 = 0x%x, TS2 = 0x%x,\n", TS[0][uLUNInSuperPU], TS[1][uLUNInSuperPU], TS[2][uLUNInSuperPU]);
                            DBG_Getch();
                        }
                        else
                        {
                            /* compare data page TS with PMT page TS */
                            ulPMTIndexInPu = L2_GetPMTIIndexInPu(LPN);

                            //if (uTLCLastPageTS[uWordLineOffset] < l_RebuildPMTIDptr->m_PMTPageTS[PUSer][ulPMTIndexInPu])
                            //for TLC block, once need rebuild, all page should participate.
                            //because pmt updated when last WL write over 
#if 0
                            if (TS[ucMinTsTarget][uLUNInSuperPU] < l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU])
                            {
                                continue;
                            }
                            else
#endif                            
                            {
                                Addr.m_PUSer = ulSuperPU;
                                Addr.m_OffsetInSuperPage = uLUNInSuperPU;
                                Addr.m_BlockInPU = usCurrBlkCnt;
                                Addr.m_PageInBlock = ulPageNum[uWordLineOffset];
                                Addr.m_LPNInPage = i;

                                //FIRMWARE_LogInfo("TLC Rebuild LPN 0x%x => Pu %d LUN %d blk %d page %d offset %d WL %d uWordLineOffset %d Time %d  PMTTime %d\n",
                                //    LPN, Addr.m_PUSer, uLUNInSuperPU, Addr.m_BlockInPU, Addr.m_PageInBlock, Addr.m_LPNInPage, CurrPos[ucMinTsTarget], uWordLineOffset, TS[ucMinTsTarget], l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][ulPMTIndexInPu]);

                                L2_UpdatePMT(&Addr, NULL, LPN);
                            }
                        }
                    }
                }

                    if (ucProgPageCnt == 2 && L2_LOWER_PAGE == ucPageType)
                    CurrPos[ucMinTsTarget]++;
#ifdef FLASH_IM_3DTLC_GEN2
                    else if (L2_EXTRA_PAGE == ucPageType)
                    CurrPos[ucMinTsTarget]++;
#endif
            }

            /* set the updated LUN's TSOder to maximum to avoid select again */
            TS[ucMinTsTarget][uLUNInSuperPU] = INVALID_8F;
            TSOrder[ucMinTsTarget][uLUNInSuperPU] = INVALID_8F;
            l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucMinTsTarget][uLUNInSuperPU] = BYPASS;
        }
    }

    aulTSInvLunCnt[ucMinTsTarget]++;

    for (ucWriteTarget = 0; ucWriteTarget < TARGET_ALL; ucWriteTarget++)
    {
        /* whether need load next RPMT on this target */
        if (FALSE == l_RebuildPMTDptr->m_TargetFinished[ulSuperPU][ucWriteTarget] &&
            aulTSInvLunCnt[ucWriteTarget] >= LUN_NUM_PER_SUPERPU)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                    if (RPMT_UECC_BYPASSS == l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU])
                    continue;

                l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU] = NOT_BYPASS;
            }

            if (TRUE == L2_IsTargetNeedLoadNextRPMT(ulSuperPU, ucWriteTarget, CurrPos[ucWriteTarget]))
            {
                for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                {
                    l_RebuildPMTDptr->m_NeedLoadRPMT[ulSuperPU][ucWriteTarget][uLUNInSuperPU] = TRUE;
                    l_RebuildPMTDptr->m_TLCBlkByPassPMTRbuild[ulSuperPU][ucWriteTarget][uLUNInSuperPU] = NOT_BYPASS;
                }
                l_RebuildPMTDptr->m_RebuildPMTStage[ulSuperPU] = REBUILD_PMT_STAGE_LOAD_RPMT;

                //DBG_Printf("pu %d blk %d target %d rebuild done!\n", PUSer, usCurrBlkCnt, ucMinTsTarget);
            }
        }
    }
}
#endif
    return;
}

/*****************************************************************************
 Prototype      : L2_RebuildPMT
 Description    :
 Input          : None
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/11
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void MCU1_DRAM_TEXT L2_RebuildPMT()
{
    U32 ulSuperPU;
    U32 AccomplishCnt = 0;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        switch (l_RebuildPMTDptr->m_RebuildPMTStage[ulSuperPU])
        {
        case REBUILD_PMT_STAGE_LOAD_RPMT:
            if (TRUE == L2_RebuildLoadRPMT(ulSuperPU))
            {
                l_RebuildPMTDptr->m_RebuildPMTStage[ulSuperPU] = REBUILD_PMT_STAGE_WAIT_RPMT;
            }
            break;

        case REBUILD_PMT_STAGE_WAIT_RPMT:
            if (TRUE == L2_IsRPMTLoadDone(ulSuperPU))
            {
#ifdef DCACHE
                HAL_InvalidateDCache();
#endif

                l_RebuildPMTDptr->m_RebuildPMTStage[ulSuperPU] = REBUILD_PMT_STAGE_REBUILD;
            }
            break;

        case REBUILD_PMT_STAGE_REBUILD:
            L2_RebuildPMTByRPMT(ulSuperPU);
            break;

        case REBUILD_PMT_STAGE_DONE:
            AccomplishCnt++;

        default:
            break;
        }
    }

    if (SUBSYSTEM_SUPERPU_NUM == AccomplishCnt)
    {
        l_RebuildPMTDptr->m_PMTState = Rebuild_PMT_Done;
    }

    return;
}

void MCU1_DRAM_TEXT L2_RebuildPuTimeStamp()
{
    U32 ulSuperPU;
    PuInfo* pInfo;
    U32 MaxTimeStamp;
    U8 i;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        pInfo = g_PuInfo[ulSuperPU];
        MaxTimeStamp = max(l_RebuildPMTIDptr->m_MaxTableTs[ulSuperPU], l_RebuildPMTDptr->m_MaxTSOfPU[ulSuperPU]);
        pInfo->m_TimeStamp = max(MaxTimeStamp, pRT->m_RT[ulSuperPU].ulMaxTimeStamp);
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
        /* because one superpage write done ,superpage TS also plus,
           after ftl write must plus, so only check ftlw target,
           after plus even new TS bigger than old ts(abnormal shutdown when not do ftlw),
           there have no effect */
        /* delete below code, fix rebuild ClipTS equal to LastWrite page in rebuild stage,
        root casue:only parcial LUN programed when previous abnormal shutdown,
        next bootup wrtie remain parcial LUN should increase TS */
            pInfo->m_TimeStamp++;        
#endif

        /* record open block info for calc dirty count, add by henryluo */
        for (i = 0; i < TARGET_ALL; i++)
        {
            l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU][i] = pInfo->m_TargetBlk[i];
            l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU][i] = pInfo->m_TargetPPO[i];
            l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][i] = pInfo->m_TargetOffsetBitMap[i];
            l_CalcDirtyCntDptr->m_RPMTFlushBitMap[ulSuperPU][i] = pInfo->m_RPMTFlushBitMap[i];
        }

        l_CalcDirtyCntDptr->m_TimeStamp[ulSuperPU] = pInfo->m_TimeStamp;
#ifdef SIM
        DBG_Printf("SPU %d real ClipTS 0x%x, PuInfoTS 0x%x\n", ulSuperPU, 
                    l_RebuildPMTIDptr->m_RebuildClipTSInPu[ulSuperPU],g_PuInfo[ulSuperPU]->m_TimeStamp);
#endif          
    }
}

void MCU1_DRAM_TEXT L2_RebuildFlushManager()
{
    U32 ulSuperPU;
    U32 PMTIndexInPu;
    U32 PMTPageTS;
    U32 MinPMTPageTS;
    U32 FlushPos;
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        MinPMTPageTS = INVALID_8F;

        for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; PMTIndexInPu++)
        {
            PMTPageTS = l_RebuildPMTIDptr->m_PMTPageTS[ulSuperPU][PMTIndexInPu];
            if (PMTPageTS < MinPMTPageTS)
            {
                MinPMTPageTS = PMTPageTS;
                FlushPos = PMTIndexInPu;
                //record pos
                l_RebuildPMTIDptr->m_MinTableTs[ulSuperPU] = PMTPageTS;
                l_RebuildPMTIDptr->m_PMTIndexofMinTableTS[ulSuperPU] = PMTIndexInPu;
            }
        }
        //Rebuild FlushManager
        L2_UpdateFlushPos(ulSuperPU, FlushPos);
        g_ulShutdownFlushPageCnt[ulSuperPU] = 0;
    }

}


BOOL MCU1_DRAM_TEXT L2_GetRebuildPMTDirtyFlag(U32 ulSuperPU, U32 PMTIndexInCE)
{
    U32 index = PMTIndexInCE / 32;
    U32 bit = PMTIndexInCE % 32;

    return (l_RebuildPMTIDptr->m_RebuildPMTDirtyBitMapInCE[ulSuperPU].m_DirtyBitMap[index] & (1 << bit));
}


BOOL MCU1_DRAM_TEXT L2_IsRebuildPMTPageDirty(U32 ulSuperPU, U32 PMTIndexInCE)
{
    return L2_GetRebuildPMTDirtyFlag(ulSuperPU, PMTIndexInCE);
}



//Err Handling API of Table Rebuild
void MCU1_DRAM_TEXT L2_InitErrManagerOfRebuild()
{
    U32 i;

    for (i = 0; i < MAX_ERR_NUM_OF_REBUILD; i++)
    {
        l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr.m_PPN = INVALID_8F;
        l_ErrManagerDptr->m_Err_Info[i].m_ErrTag = UNDEF_ERR;
    }
    l_ErrManagerDptr->m_Cnt = 0;

}
//Err Handling API of Table Rebuild
BOOL MCU1_DRAM_TEXT L2_IsHavePMTErrOfRebuild(U32 ulSuperPU)
{
    U32 i;
    U32 uLUNInSuperPU;
    U32 Cnt = l_ErrManagerDptr->m_Cnt;

    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
        if (PMT_PAGE_ERR == l_ErrManagerDptr->m_Err_Info[i].m_ErrTag)
        {
                if ((ulSuperPU == l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr.m_PUSer) && (uLUNInSuperPU == l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr.m_OffsetInSuperPage))
            {
                return TRUE;
            }
        }
    }
    }
    return FALSE;
}
//Err Handling API of Table Rebuild
BOOL MCU1_DRAM_TEXT L2_IsHavePMTErr(PhysicalAddr* pAddr)
{
    U32 i;
    U32 Cnt = l_ErrManagerDptr->m_Cnt;

    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        if (PMT_PAGE_ERR == l_ErrManagerDptr->m_Err_Info[i].m_ErrTag)
        {
            if (pAddr->m_PPN == l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr.m_PPN)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

//Err Handling API of Table Rebuild
void MCU1_DRAM_TEXT L2_CollectErrInfoOfRebuild(U8 ucSuperPU, U8 ucLUNInSuperPU, U16 Blk, U16 Page, ERR_TAG_OF_REBUILD ErrTag)
{
    U32 i;
    U32 Cnt = l_ErrManagerDptr->m_Cnt;
    PhysicalAddr ErrAddr = { 0 };

    ErrAddr.m_PUSer = ucSuperPU;
    ErrAddr.m_OffsetInSuperPage = ucLUNInSuperPU;
    ErrAddr.m_BlockInPU = Blk;
    ErrAddr.m_PageInBlock = Page;
    ErrAddr.m_LPNInPage = 0;

    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        if (ErrAddr.m_PPN == l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr.m_PPN)
        {
            l_ErrManagerDptr->m_Err_Info[i].m_ErrTag = ErrTag;
            return;
        }
    }

    DBG_Printf("Add ErrAddr: SPU %d LUN %d Blk %d Page %d\n", ErrAddr.m_PUSer, ErrAddr.m_OffsetInSuperPage, ErrAddr.m_BlockInPU, ErrAddr.m_PageInBlock);

    DBG_Printf("ErrTag ");
    switch (ErrTag)
    {
    case UNDEF_ERR:
        DBG_Printf("UNDEF_ERR");
        break;
    case FIRST_PAGE_ERR:
        DBG_Printf("FIRST_PAGE_ERR");
        break;
    case PMT_PAGE_ERR:
        DBG_Printf("PMT_PAGE_ERR");
        break;
    case RPMT_ERR:
        DBG_Printf("RPMT_ERR");
        break;
    case TLC_RPMT_ERR:
        DBG_Printf("TLC_RPMT_ERR");
        break;
    case DATA_SPARE_ERR:
        DBG_Printf("DATA_SPARE_ERR");
        break;
    case TLC_DATA_SPARE_ERR:
        DBG_Printf("TLC_DATA_SPARE_ERR");
        break;
    case MOVE_BLK_SPARE_ERR:
        DBG_Printf("MOVE_BLK_SPARE_ERR");
        break;
    default:
        DBG_Printf("L2_CollectErrInfoOfRebuild No current ErrTag");
        DBG_Getch();
        break;
    }
    DBG_Printf("\n");

    /* only real insert ErrTag:PMT_PAGE_ERR/TLC_RPMT_ERR; other case only uart print */
    if ((PMT_PAGE_ERR == ErrTag) || (TLC_RPMT_ERR == ErrTag))
    {
        l_ErrManagerDptr->m_Err_Info[Cnt].m_ErrAddr.m_PPN = ErrAddr.m_PPN;
        l_ErrManagerDptr->m_Err_Info[Cnt].m_ErrTag = ErrTag;
        Cnt++;
        l_ErrManagerDptr->m_Cnt = Cnt;
    }

    if (Cnt > MAX_ERR_NUM_OF_REBUILD)
    {
        DBG_Printf("To many Err in Table Rebuild\n");
        DBG_Getch();
    }

    return;
}


BOOL MCU1_DRAM_TEXT L2_FindErrPMTCEOfRebuild(U8 ucSuperPU,U8 ucLUNInSuperPU)
{
    U32 i;
    U32 Cnt = l_ErrManagerDptr->m_Cnt;
    PhysicalAddr Addr = { 0 };

    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        if (PMT_PAGE_ERR == l_ErrManagerDptr->m_Err_Info[i].m_ErrTag)
        {
            Addr = l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr;

            if (ucSuperPU == Addr.m_PUSer && ucLUNInSuperPU == Addr.m_OffsetInSuperPage)
                return TRUE;
        }
    }
    return FALSE;
}

/*
    Ret : 
        partial write or full with error return TRUE;
        full free return FALSE;
*/
BOOL L2_RebuildIsSuperPMTPageHadW(U32 ulSuperPU,U16 BlkSN, U16 page, U32 * uPMTBitMap)
{
    BOOL bHadWrite = TRUE;
    U32 uLUNInSuperPU;
    U32 uEmptyCount = 0;

    for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
    {            
        if(l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[ulSuperPU][BlkSN][uLUNInSuperPU] > page)
        {
            COM_BitMaskSet(uPMTBitMap, uLUNInSuperPU);
        }
        else if(l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[ulSuperPU][BlkSN][uLUNInSuperPU] == page)        
        {
            uEmptyCount++;
        }
        else
        {
            DBG_Printf("L2_RebuildIsSuperPMTPageHadW: SPU %d page %d param is not valid\n",ulSuperPU,page);
            DBG_Getch();
        }
    }
    if(LUN_NUM_PER_SUPERPU == uEmptyCount)
    {
        return FALSE;
    }

    return bHadWrite;
}

/* Super PMT page with error page but without empty page return true */
BOOL MCU1_DRAM_TEXT L2_FindErrPMTPageOfRebuild(U32 ulSuperPU, U16 BlkSN, U16 Page, BOOL *bHadWrite)
{
    U32 Cnt = l_ErrManagerDptr->m_Cnt;
    PhysicalAddr Addr = { 0 };
    U32 uPMTBitMap = 0;

    if(Page >= PG_PER_SLC_BLK)
    {
        /*ppo not need more check*/
        return FALSE;
    }

    *bHadWrite = L2_RebuildIsSuperPMTPageHadW(ulSuperPU, BlkSN, Page, &uPMTBitMap);

    // this page+1 is ppo 
    if(*bHadWrite)     
    {
        if(uPMTBitMap != SUPERPU_LUN_NUM_BITMSK)
        {
            //g_PMTManager->m_PMTBitMapSPOR[ulSuperPU] = uPMTBitMap;
            return FALSE;
        }
        else
        {
            /*ppo need more check*/
            return TRUE;
        }
    }    
    
#if 0    
    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        if (PMT_PAGE_ERR == l_ErrManagerDptr->m_Err_Info[i].m_ErrTag)
        {
            Addr = l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr;
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                if (ulSuperPU == Addr.m_PUSer && uLUNInSuperPU == Addr.m_OffsetInSuperPage &&
                    Blk[uLUNInSuperPU] == Addr.m_BlockInPU &&
                Page == Addr.m_PageInBlock)
                return TRUE;
            }

        }
    }
#endif
    // this page is ppo
    return FALSE;
}

void MCU1_DRAM_TEXT L2_ResumePMTManager()
{
    U8 PUSer;
    U8 ucBlockIndex;
    U16 DirtyPageCnt = 0;
    U16 FreePageCnt = 0;
    U16 CurPPO;
    BOOL bFree;

    for (PUSer = 0; PUSer < SUBSYSTEM_SUPERPU_NUM; PUSer++)
    {
        FreePageCnt = 0;
        CurPPO = g_PMTManager->m_PMTBlkManager[PUSer].m_CurPPO;
        for (ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
        {
            bFree = g_PMTManager->m_PMTBlkManager[PUSer].m_PMTBlkInfo[ucBlockIndex].m_bFree;
            if (bFree)
            {
                FreePageCnt += PG_PER_SLC_BLK;
            }
        }
        FreePageCnt += PG_PER_SLC_BLK - CurPPO;
        g_PMTManager->m_PMTBlkManager[PUSer].m_FreePagesCnt = FreePageCnt;
    }
}
void MCU1_DRAM_TEXT L2_NormalBootResumeInfo()
{
    //L2_ResumePuInfo();
    L2_ResumePMTManager();
}

void MCU1_DRAM_TEXT L2_SetupRebuildDrityCntFlag()
{
    U32 ulSuperPU ;

    g_RebuildDirtyFlag = TRUE;
    for (ulSuperPU  = 0; ulSuperPU  < SUBSYSTEM_SUPERPU_NUM; ulSuperPU ++)
    {
        l_CalcDirtyCntDptr->m_NeedRebuitDirtyCntFlag[ulSuperPU ] = TRUE;
    }

#ifdef SIM
    g_aRebuildDirtyCntFlag[HAL_GetMcuId() - 2] = TRUE;
#endif

    return;
}

void MCU1_DRAM_TEXT L2_ClearRebuildDirtyCntFlag()
{
    U32 uSuperPu;

    //g_RebuildDirtyFlag = FALSE;
    for (uSuperPu = 0; uSuperPu < SUBSYSTEM_SUPERPU_NUM;  uSuperPu++)
    {
        l_CalcDirtyCntDptr->m_NeedRebuitDirtyCntFlag[uSuperPu] = FALSE;
    }

    return;
}

BOOL L2_IsNeedRebuildDirtyCnt(U32 uSuperPu)
{
    return l_CalcDirtyCntDptr->m_NeedRebuitDirtyCntFlag[uSuperPu];
}

void L2_ClearRebuildDirtyCntFlagOfCE(U32 ulSuperPU)
{
    l_CalcDirtyCntDptr->m_NeedRebuitDirtyCntFlag[ulSuperPU] = FALSE;
}

BOOL L2_IsRebuildDirtyCntFinish()
{
    U32 i;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM;  i++)
    {
        if (TRUE == l_CalcDirtyCntDptr->m_NeedRebuitDirtyCntFlag[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}


/*****************************************************************************
Function: Rebuild ErrHandling Pair Page
Date        :2013.10.15
Description :
*****************************************************************************/
void MCU1_DRAM_TEXT L2_InitErrHandleOfRebuild()
{
    U32 i;
    U32 ulSuperPU;
    U8 ucTargetIndex;
    U8 uLUNInSuperPU;

    l_ErrHandleDptr->m_ErrHandleState = Rebuild_ErrHandle_Prepare;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU] = 0;
        l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU] = 0;
        l_ErrHandleDptr->m_SrcBlk[ulSuperPU] = INVALID_8F;
        l_ErrHandleDptr->m_DstBlk[ulSuperPU] = INVALID_8F;
        l_ErrHandleDptr->m_ErrHandlePage[ulSuperPU] = 0;
        l_ErrHandleDptr->m_MoveBlkStage[ulSuperPU] = Rebuild_MoveBlk_Read;
        l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU] = FALSE;
        l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU] = FALSE;

        l_ErrHandleDptr->m_HandleTargetBlkStage[ulSuperPU] = Rebuild_HandleTargetBlk_Prepare;

        for (i = 0; i < MAXERRBLK; i++)
        {
            l_ErrHandleDptr->m_NeedMoveBlk[ulSuperPU][i] = INVALID_8F;
            l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU] = 0;
            l_ErrHandleDptr->m_NeedMoveBlkTargetType[ulSuperPU][i] = TARGET_ALL;            
            l_ErrHandleDptr->m_MoveTargetBlk[ulSuperPU][i] = INVALID_4F;
        }

        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            for (ucTargetIndex = 0; ucTargetIndex < TARGET_HOST_ALL; ucTargetIndex++)
            {
                l_ErrHandleDptr->m_EraseStatus[ulSuperPU][ucTargetIndex][uLUNInSuperPU] = SUBSYSTEM_STATUS_SUCCESS;
            }
        }
    }
}

RebuildErrHandleState MCU1_DRAM_TEXT L2_RebuildGetErrHandleState()
{
    return l_ErrHandleDptr->m_ErrHandleState;
}

void MCU1_DRAM_TEXT L2_RebuildSetErrHandleStateState(RebuildErrHandleState State)
{
    l_ErrHandleDptr->m_ErrHandleState = State;
}
void MCU1_DRAM_TEXT L2_AddErrHandleBlk(U32 ulSuperPU, U32 Blk, U32 uTarget)
{
    U32 i;
    U32 NeedMoveBlkCnt = l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU];

    //find if exist
    for (i = 0; i < NeedMoveBlkCnt; i++)
    {
        if (Blk == l_ErrHandleDptr->m_NeedMoveBlk[ulSuperPU][i])
        {
            l_ErrHandleDptr->m_ErrPageCntOfNeedMoveBlk[ulSuperPU][i] ++;
            break;
        }
    }

    //Not Find, add the blk
    if (i == NeedMoveBlkCnt)
    {
        l_ErrHandleDptr->m_NeedMoveBlk[ulSuperPU][NeedMoveBlkCnt] = Blk;
        l_ErrHandleDptr->m_NeedMoveBlkTargetType[ulSuperPU][NeedMoveBlkCnt] = uTarget;
        l_ErrHandleDptr->m_ErrPageCntOfNeedMoveBlk[ulSuperPU][i] ++;
        NeedMoveBlkCnt++;
        l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU] = NeedMoveBlkCnt;
    }
}

BOOL MCU1_DRAM_TEXT L2_FindBlkOfErrTLCRPMTC(U32 ulSuperPU, U32 ulLUNInSuperPU, U16 Blk)
{
    U32 i;
    U32 Cnt = l_ErrManagerDptr->m_Cnt;
    PhysicalAddr Addr = { 0 };

    //find if already insert
    for (i = 0; i < Cnt; i++)
    {
        if (TLC_RPMT_ERR == l_ErrManagerDptr->m_Err_Info[i].m_ErrTag)
        {
            Addr = l_ErrManagerDptr->m_Err_Info[i].m_ErrAddr;

            if (ulSuperPU == Addr.m_PUSer && ulLUNInSuperPU == Addr.m_OffsetInSuperPage && Blk == Addr.m_BlockInPU)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void MCU1_DRAM_TEXT L2_DumpErrHandleMoveBlkInfo()
{
    U32 PUSer;
    U32 NeedMoveBlkCnt;
    U32 i;
    U32 Blk;

    DBG_Printf("####ErrHandle Info######\n");
    for (PUSer = 0; PUSer < SUBSYSTEM_LUN_NUM; PUSer++)
    {
        NeedMoveBlkCnt = l_ErrHandleDptr->m_NeedMoveBlkCnt[PUSer];

        if (0 != NeedMoveBlkCnt)
        {
            DBG_Printf("PU %d Need ErrHandle Blk: ", PUSer);
            //find if exist
            for (i = 0; i < NeedMoveBlkCnt; i++)
            {
                Blk = l_ErrHandleDptr->m_NeedMoveBlk[PUSer][i];
                DBG_Printf("0x%x  ", Blk);
            }
            DBG_Printf("\n");
        }
    }
}


void MCU1_DRAM_TEXT L2_PrepareErrHanle()
{
    U32 Cnt = l_ErrManagerDptr->m_Cnt;
    U32 ulSuperPU;
    U32 Blk;

    U32 NeedMoveBlkCnt;

    /* copy all open block after table rebuild, modified by henryluo */
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        if ((0 != g_PuInfo[ulSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML]) && (INVALID_4F != g_PuInfo[ulSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML]))
        {
            Blk = g_PuInfo[ulSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
            L2_AddErrHandleBlk(ulSuperPU, Blk, TARGET_HOST_WRITE_NORAML);
        }

        if ((0 != g_PuInfo[ulSuperPU]->m_TargetPPO[TARGET_HOST_GC]) && (INVALID_4F != g_PuInfo[ulSuperPU]->m_TargetBlk[TARGET_HOST_GC]))
        {
            Blk = g_PuInfo[ulSuperPU]->m_TargetBlk[TARGET_HOST_GC];
            L2_AddErrHandleBlk(ulSuperPU, Blk, TARGET_HOST_GC);
        }
    }

    //Init Finish Flag
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        NeedMoveBlkCnt = l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU];
        if (0 == NeedMoveBlkCnt)
        {
            l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU] = TRUE;
            l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU] = TRUE;
        }
    }
}

BOOL MCU1_DRAM_TEXT L2_IsErrHandleFinish(U32 ulSuperPU)
{
    return l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU];
}

U32 MCU1_DRAM_TEXT L2_RebuildGetErrHandleSrcBlk(U32 ulSuperPU,U32* pTargetType)
{
    U32 SrcBlk;
    U32 Blk;

    U32 BlkSN = l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU];
    *pTargetType = l_ErrHandleDptr->m_NeedMoveBlkTargetType[ulSuperPU][BlkSN];

    SrcBlk = l_ErrHandleDptr->m_SrcBlk[ulSuperPU];
    if (INVALID_8F != SrcBlk)
    {
        return SrcBlk;
    }
    else //Find Next Need Move Blk
    {
        Blk = l_ErrHandleDptr->m_NeedMoveBlk[ulSuperPU][BlkSN];
        l_ErrHandleDptr->m_SrcBlk[ulSuperPU] = Blk;
        return Blk;
    }
}

U32 MCU1_DRAM_TEXT L2_RebuildGetErrHandleSrcPage(U32 ulSuperPU)
{
    U32 Page;

    Page = l_ErrHandleDptr->m_ErrHandlePage[ulSuperPU];
    return  Page;
}

U32 MCU1_DRAM_TEXT L2_RebuildIncErrHandleSrcPage(U32 PUSer)
{
    U32 Page;

    Page = l_ErrHandleDptr->m_ErrHandlePage[PUSer];
    Page++;
    l_ErrHandleDptr->m_ErrHandlePage[PUSer] = Page;
    return  Page;
}



U16 MCU1_DRAM_TEXT L2_RebuildGetErrHandleDstBlk(U32 ulSuperPU)
{
    U32 DstBlk;
    U16 Blk;

    DstBlk = l_ErrHandleDptr->m_DstBlk[ulSuperPU];
    if (INVALID_8F != DstBlk)
    {
        return DstBlk;
    }
    else //Allocate a new blk
    {
        Blk = L2_BM_AllocateFreeBlock(ulSuperPU, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
        l_ErrHandleDptr->m_DstBlk[ulSuperPU] = (U32)Blk;
        l_ErrHandleDptr->m_MoveTargetBlk[ulSuperPU][l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU]] = (U16)Blk;
        return Blk;
    }
}

extern GLOBAL U32 g_L2TempBufferAddr;
extern void FillSpareAreaInWrite(RED* pSpare, U32* pLPN, U8 ucSuperPu, U8 ucLUNInSuperPU, BLOCK_TYPE ActualBlockType, TargetType eTargetType);
//Finish One Page Copy, return TRUE
//Not Finish ,return FALSe
BOOL MCU1_DRAM_TEXT L2_RebuildMovePage(U32 ulSuperPU, U32 SrcBlk, U32 DstBlk, U16 Page)
{
    RebuildMoveBlkStage Stage = l_ErrHandleDptr->m_MoveBlkStage[ulSuperPU];
    U8* pStatus;
    U32* pBuffer;
    RED* pSpare;
    U32 i;
    BOOL ret = FALSE;
    U32 ErrHandleBlkSN = l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU];
    U32 uTarget = l_ErrHandleDptr->m_NeedMoveBlkTargetType[ulSuperPU][ErrHandleBlkSN];
    PhysicalAddr Addr = { 0 };
    U32 uLUNInSuperPU;

    for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
    {
        pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][0][uLUNInSuperPU];
#ifdef PMT_ITEM_SIZE_REDUCE
        pBuffer = (U32 *)&((SuperPage*)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][0])->m_Content[uLUNInSuperPU];
#else
        pBuffer = (U32 *)&(l_BufferManagerDptr->m_PageBuffer[ulSuperPU][0])->m_Page.m_Content[uLUNInSuperPU];
#endif
        pSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][0][uLUNInSuperPU];

        switch (Stage)
        {
#if 0
            /* erase target block before copy open block, add by henryluo */
        case Rebuild_MoveBlk_Erase:
            L2_FtlEraseBlock(PUSer, 0, DstBlk, NULL, FALSE, FALSE);
            l_ErrHandleDptr->m_MoveBlkStage[PUSer] = Rebuild_MoveBlk_Read;
            break;
#endif
        case Rebuild_MoveBlk_Read:
        {
            Addr.m_PUSer = ulSuperPU;
            Addr.m_OffsetInSuperPage = uLUNInSuperPU;
            Addr.m_BlockInPU = SrcBlk;
            Addr.m_PageInBlock = Page;
            Addr.m_LPNInPage = 0;

            if (Page < (g_PuInfo[ulSuperPU]->m_TargetPPO[uTarget] - 1))
            {
                /* normal case */
                L2_FtlReadLocal(pBuffer, &Addr, pStatus, (U32*)pSpare, LPN_PER_BUF, 0, FALSE, TRUE);
            }
            else if ((Page == g_PuInfo[ulSuperPU]->m_TargetPPO[uTarget] - 1) && TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[uTarget], uLUNInSuperPU))
            { 
                /* TargetPPO-1 only copy page with data */            
                L2_FtlReadLocal(pBuffer, &Addr, pStatus, (U32*)pSpare, LPN_PER_BUF, 0, FALSE, TRUE);
            }
            else if ((PG_PER_SLC_BLK - 1) == Page && TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ulSuperPU]->m_RPMTFlushBitMap[uTarget], uLUNInSuperPU))
            {
                /*RPMT partial written case,only copy RPMT page with data */
                HAL_DMAECopyOneBlock((U32)pBuffer, (U32)&g_PuInfo[ulSuperPU]->m_pRPMT[uTarget][0]->m_RPMT[uLUNInSuperPU], sizeof(RPMT_PER_LUN));
                L2_PBIT_SetLastPageRedInfo(ulSuperPU, uLUNInSuperPU, SrcBlk, pSpare);
                *pStatus = SUBSYSTEM_STATUS_SUCCESS;
            }
            else
            {
                *pStatus = SUBSYSTEM_STATUS_SUCCESS;
                DBG_Printf("   MovePage SPU %d LUN %d SrcBlk 0x%x Pg 0x%x Read Skiped,TargetOffsetBitMap 0x%x\n", ulSuperPU, uLUNInSuperPU, SrcBlk, Page, g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[uTarget]);
            }

            if ((LUN_NUM_PER_SUPERPU - 1) == uLUNInSuperPU)
            {
                l_ErrHandleDptr->m_MoveBlkStage[ulSuperPU] = Rebuild_MoveBlk_WaitRStatus;
                l_ErrHandleDptr->m_TargetOffsetBitMap[ulSuperPU] = 0;
            }
        }
            break;
        case Rebuild_MoveBlk_WaitRStatus:
        {
            //Wait Status
            if (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                break;
            }
            else if (SUBSYSTEM_STATUS_SUCCESS != *pStatus && SUBSYSTEM_STATUS_RECC != *pStatus && SUBSYSTEM_STATUS_RETRY_SUCCESS != *pStatus)
            {
                U16 usPBN = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, SrcBlk);
                DBG_Printf("L2_RebuildMoveBlk read error: SPU %d LUN %d Blk %d_%d Page %d PairPageType %d ErrorType %d\n",
                    ulSuperPU, uLUNInSuperPU, SrcBlk, usPBN, Page, L3_GetPageTypeByProgOrder(Page), *pStatus);

                //Just Write One Empte Page
#ifdef PMT_ITEM_SIZE_REDUCE
                COM_MemSet(pBuffer, sizeof(SuperPage) / sizeof(U32), INVALID_8F);
#else
                COM_MemSet(pBuffer, sizeof(PMTPage) / sizeof(U32), INVALID_8F);
#endif

                //Fill Spare
                L2_PBIT_GetFirstPageRedInfo(ulSuperPU, SrcBlk, Page, pSpare);

                //Attention,TimeStamp Just use last page TimeStamp
                pSpare->m_RedComm.ulTimeStamp = l_RebuildPMTIDptr->m_MaxTableTs[ulSuperPU];

                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    pSpare->m_DataRed.aCurrLPN[i] = INVALID_8F;
                }
#ifdef SIM
                L2_CollectErrInfoOfRebuild(ulSuperPU, uLUNInSuperPU, SrcBlk, Page, MOVE_BLK_SPARE_ERR);
#endif

            }
            COM_BitMaskSet(&l_ErrHandleDptr->m_TargetOffsetBitMap[ulSuperPU], uLUNInSuperPU);
            if (SUPERPU_LUN_NUM_BITMSK == l_ErrHandleDptr->m_TargetOffsetBitMap[ulSuperPU])
            {
                l_ErrHandleDptr->m_MoveBlkStage[ulSuperPU] = Rebuild_MoveBlk_Write;
                break;
            }
        }
            break;
        case Rebuild_MoveBlk_Write:
        {

#ifdef DCACHE
            HAL_InvalidateDCache();
#endif
            Addr.m_PUSer = ulSuperPU;
            Addr.m_OffsetInSuperPage = uLUNInSuperPU;
            Addr.m_BlockInPU = DstBlk;
            Addr.m_PageInBlock = Page;
            Addr.m_LPNInPage = 0;

            if (0 == Addr.m_PageInBlock)
            {
                L2_Set_DataBlock_PBIT_Info(Addr.m_PUSer, uLUNInSuperPU, Addr.m_BlockInPU, pSpare);
            }

            if (Page < (g_PuInfo[ulSuperPU]->m_TargetPPO[uTarget] - 1))
            {
                L2_FtlWriteLocal(&Addr, pBuffer, (U32*)pSpare, pStatus, FALSE, TRUE, NULL);
            }
            else if ((Page == g_PuInfo[ulSuperPU]->m_TargetPPO[uTarget] - 1) && TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[uTarget], uLUNInSuperPU))
            {
                /* TargetPPO-1 only copy page with data */
                L2_FtlWriteLocal(&Addr, pBuffer, (U32*)pSpare, pStatus, FALSE, TRUE, NULL);
            }
            else if ((PG_PER_SLC_BLK - 1) == Page && TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ulSuperPU]->m_RPMTFlushBitMap[uTarget], uLUNInSuperPU))
            {
                /*RPMT partial written case,only copy RPMT page with data */
                L2_FtlWriteLocal(&Addr, pBuffer, (U32*)pSpare, pStatus, FALSE, TRUE, NULL);
            }
            else
            {
                DBG_Printf("   MovePage SPU %d LUN %d DstBlk 0x%x Pg 0x%x Write Skiped\n", ulSuperPU, uLUNInSuperPU, DstBlk, Page);
            }

            if ((LUN_NUM_PER_SUPERPU - 1) == uLUNInSuperPU)
            {
                l_ErrHandleDptr->m_MoveBlkStage[ulSuperPU] = Rebuild_MoveBlk_WaitWStatus;
                l_ErrHandleDptr->m_TargetOffsetBitMap[ulSuperPU] = 0;
            }
        }
            break;
        case Rebuild_MoveBlk_WaitWStatus:
        {
            //Wait Status
            if (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                break;
            }
            COM_BitMaskSet(&l_ErrHandleDptr->m_TargetOffsetBitMap[ulSuperPU], uLUNInSuperPU);
            if (SUPERPU_LUN_NUM_BITMSK == l_ErrHandleDptr->m_TargetOffsetBitMap[ulSuperPU])
            {
                l_ErrHandleDptr->m_MoveBlkStage[ulSuperPU] = Rebuild_MoveBlk_Read;
                ret = TRUE;
            }
        }
            break;
        default:
            break;
        }
    }

    return ret;

}

BOOL MCU1_DRAM_TEXT L2_RebuildErrTLC(U32 ulSuperPU, U32 uLUNInSuperPU)
{
    //(1) Load one page of err TLC Blk
    //(2) wait status
    //(3) LookupPMT to find 8 src BLK, send flash cmd to read it 
    //(4) wait status
    //(5) compare timestamp to rebuild PMT
    //(6) finish

    U32 CurErrTLCBlkSN, CurErrTLCPageSN;
    U32 ErrTLCBlk;
    BOOL ret = FALSE;
    PhysicalAddr ErrTLCFlashAddr = { 0 };
    PhysicalAddr DestFlashAddr = { 0 };
    RED* pErrTLCSpare;
    RED* pDestSpare;
    U8* pErrTLCStatus;
    U8* pDestStatus;
    U32 LPN;
    U32 CurLoadDestBlkSN;
    PhysicalAddr* pDestFlashAddr;
    BOOL bTLCFlag;
    U32 i;
    U32 DestLPNOffset;
    U32 uTimeOfErrTLCBlk, uTimeOfDestBlk;
    U32 LPNOfErrTLC;
    U32 LPNOfDest;
    RED* pSpareOfErrTLC;
    RED* pSpareOfDestBlk;
    PhysicalAddr* pDestBlkAddr;

    switch (l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU])
    {
    case Rebuild_ErrTLC_LoadTLCPage:
        CurErrTLCBlkSN = l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[ulSuperPU][uLUNInSuperPU];
        if (CurErrTLCBlkSN >= l_RebuildErrTLCDptr->m_ErrTLCBlkCnt[ulSuperPU][uLUNInSuperPU])
        {
            l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_Done;
            break;
        }

        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulSuperPU, uLUNInSuperPU)))
        {
            break;
        }
        l_RebuildErrTLCDptr->m_CurLoadDestBlkSN[ulSuperPU][uLUNInSuperPU] = 0;

        //get err tlc block
        ErrTLCBlk = l_RebuildErrTLCDptr->m_ErrTLCBlk[ulSuperPU][uLUNInSuperPU][CurErrTLCBlkSN];

        //get err tlc page index
        CurErrTLCPageSN = l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU];

        ErrTLCFlashAddr.m_PUSer = ulSuperPU;
        ErrTLCFlashAddr.m_OffsetInSuperPage= uLUNInSuperPU;
        ErrTLCFlashAddr.m_BlockInPU = ErrTLCBlk;
        ErrTLCFlashAddr.m_PageInBlock = CurErrTLCPageSN;
        ErrTLCFlashAddr.m_LPNInPage = 0;

        pErrTLCSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][0][0][uLUNInSuperPU];
        pErrTLCStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][0][0][uLUNInSuperPU];

        //FIRMWARE_LogInfo("Load TLC(%d_%d_%d_%d_%d) \n", ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, pVBT[PUSer]->m_VBT[ErrTLCFlashAddr.m_BlockInPU].PhysicalBlockAddr[0],
        //    ErrTLCFlashAddr.m_PageInBlock,ErrTLCFlashAddr.m_LPNInPage);

        L2_LoadSpare(&ErrTLCFlashAddr, pErrTLCStatus, (U32 *)pErrTLCSpare, FALSE);

        l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_WaitStautsOfTLCPage;
        break;

    case Rebuild_ErrTLC_WaitStautsOfTLCPage:
        pErrTLCStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][0][0][uLUNInSuperPU];
        if (SUBSYSTEM_STATUS_PENDING == *pErrTLCStatus)
        {
            break;
        }

        if (SUBSYSTEM_STATUS_EMPTY_PG == *pErrTLCStatus)
        {
            CurErrTLCBlkSN = l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[ulSuperPU][uLUNInSuperPU];
            ErrTLCBlk = l_RebuildErrTLCDptr->m_ErrTLCBlk[ulSuperPU][uLUNInSuperPU][CurErrTLCBlkSN];
            DBG_Printf("Rebuild_ErrTLC_LoadTLCPage empty SPU %d LUN %d BLK %d PG %d\n", ulSuperPU, uLUNInSuperPU, ErrTLCBlk, l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU]);
            DBG_Getch();
        }
        else
        {
            l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_LookUpOldMapping;

            if (SUBSYSTEM_STATUS_FAIL != *pErrTLCStatus)
            {
                CurErrTLCPageSN = l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU];
                pErrTLCSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][0][0][uLUNInSuperPU];

                //FIRMWARE_LogInfo("Load TLC succeed page %d time %d LPN %x %x %x %x  %x %x %x %x \n",
                //    CurErrTLCPageSN, pErrTLCSpare->m_RedComm.ulTimeStamp,
                //    pErrTLCSpare->m_DataRed.aCurrLPN[0], pErrTLCSpare->m_DataRed.aCurrLPN[1], pErrTLCSpare->m_DataRed.aCurrLPN[2], pErrTLCSpare->m_DataRed.aCurrLPN[3],
                //    pErrTLCSpare->m_DataRed.aCurrLPN[4], pErrTLCSpare->m_DataRed.aCurrLPN[5], pErrTLCSpare->m_DataRed.aCurrLPN[6], pErrTLCSpare->m_DataRed.aCurrLPN[7]);
            }
        }

        break;
    case Rebuild_ErrTLC_LookUpOldMapping:
        pErrTLCStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][0][0][uLUNInSuperPU];

        //get err tlc block
        CurErrTLCBlkSN = l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[ulSuperPU][uLUNInSuperPU];
        ErrTLCBlk = l_RebuildErrTLCDptr->m_ErrTLCBlk[ulSuperPU][uLUNInSuperPU][CurErrTLCBlkSN];
        CurErrTLCPageSN = l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU];

        if (SUBSYSTEM_STATUS_EMPTY_PG == *pErrTLCStatus)
        {
            DBG_Printf("SPU %d TLC LUN %d block %d Page %d EmptyPage\n", ulSuperPU, uLUNInSuperPU, ErrTLCBlk, CurErrTLCPageSN);
            DBG_Getch();
        }

        //Err handling
        if (SUBSYSTEM_STATUS_FAIL == *pErrTLCStatus)
        {
            L2_CollectErrInfoOfRebuild(ulSuperPU, uLUNInSuperPU,  ErrTLCBlk, CurErrTLCPageSN, TLC_DATA_SPARE_ERR);
            DBG_Printf("SPU %d LUN %d TLC block %d, load Page %d fail.\n", ulSuperPU, uLUNInSuperPU, ErrTLCBlk, CurErrTLCPageSN);

            l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_LoadNextTLCPage;
            break;
        }

        pErrTLCSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][0][0][uLUNInSuperPU];

        while (1)
        {
            //Cur Load Dest SN    
            CurLoadDestBlkSN = l_RebuildErrTLCDptr->m_CurLoadDestBlkSN[ulSuperPU][uLUNInSuperPU];
            LPN = pErrTLCSpare->m_DataRed.aCurrLPN[CurLoadDestBlkSN];

            pDestFlashAddr = &l_RebuildErrTLCDptr->m_LoadDestBlkAddr[ulSuperPU][uLUNInSuperPU][CurLoadDestBlkSN];

            if (INVALID_8F == LPN)
            {
                pDestFlashAddr->m_PPN = INVALID_8F;
            }
            else
            {
                //Look up old Mapping    
                L2_LookupPMT(pDestFlashAddr, LPN,TRUE);
            }

            pDestStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][CurLoadDestBlkSN][uLUNInSuperPU];
            pDestSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][CurLoadDestBlkSN][uLUNInSuperPU];

            //INVALID ADDR
            if (INVALID_8F == pDestFlashAddr->m_PPN)
            {
                *pDestStatus = SUBSYSTEM_STATUS_SUCCESS;

                CurLoadDestBlkSN++;
                l_RebuildErrTLCDptr->m_CurLoadDestBlkSN[ulSuperPU][uLUNInSuperPU] = CurLoadDestBlkSN;
                if (CurLoadDestBlkSN >= LPN_PER_BUF)
                {
                    l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_WaitLoadDestBlk;
                    break;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                ErrTLCFlashAddr.m_PUSer = ulSuperPU;
                ErrTLCFlashAddr.m_OffsetInSuperPage = uLUNInSuperPU;
                ErrTLCFlashAddr.m_BlockInPU = ErrTLCBlk;
                ErrTLCFlashAddr.m_PageInBlock = CurErrTLCPageSN;
                ErrTLCFlashAddr.m_LPNInPage = CurLoadDestBlkSN;

                if (pDestFlashAddr->m_PPN == ErrTLCFlashAddr.m_PPN)
                {
                    //FIRMWARE_LogInfo("TLC(%d_%d_%d_%d) LPN 0x%x ErrTLCFlashAddr == DestFlashAddr!\n", ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, ErrTLCFlashAddr.m_PageInBlock, ErrTLCFlashAddr.m_LPNInPage, LPN);

                *pDestStatus = SUBSYSTEM_STATUS_SUCCESS;

                CurLoadDestBlkSN++;
                l_RebuildErrTLCDptr->m_CurLoadDestBlkSN[ulSuperPU][uLUNInSuperPU] = CurLoadDestBlkSN;
                if (CurLoadDestBlkSN >= LPN_PER_BUF)
                {
                    l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_WaitLoadDestBlk;
                    break;
                }
                else
                {
                    continue;
                }
            }
            }

            bTLCFlag = L2_VBT_Get_TLC(ulSuperPU, pDestFlashAddr->m_BlockInPU);

            if (pPBIT[ulSuperPU]->m_PBIT_Entry[pDestFlashAddr->m_OffsetInSuperPage][L2_VBT_GetPhysicalBlockAddr(ulSuperPU, pDestFlashAddr->m_OffsetInSuperPage, pDestFlashAddr->m_BlockInPU)].bFree)
            {
                *pDestStatus = SUBSYSTEM_STATUS_EMPTY_PG;
            }
            else
            {

                if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulSuperPU, pDestFlashAddr->m_OffsetInSuperPage)))
                {
                    break;
                }

                DestFlashAddr.m_PUSer = pDestFlashAddr->m_PUSer;
                DestFlashAddr.m_OffsetInSuperPage= pDestFlashAddr->m_OffsetInSuperPage;
                DestFlashAddr.m_BlockInPU = pDestFlashAddr->m_BlockInPU;
                DestFlashAddr.m_PageInBlock = pDestFlashAddr->m_PageInBlock;
                DestFlashAddr.m_LPNInPage = 0;
                //Load Dest Page
                L2_LoadSpare(&DestFlashAddr, pDestStatus, (U32 *)pDestSpare, !(bTLCFlag));

                //FIRMWARE_LogInfo("LoadSpare Pu %d Blk %d Pg %d\n", DestFlashAddr.m_PUSer, DestFlashAddr.m_BlockInPU, DestFlashAddr.m_PageInBlock);
            }

            //Next dest blk 
            CurLoadDestBlkSN++;
            l_RebuildErrTLCDptr->m_CurLoadDestBlkSN[ulSuperPU][uLUNInSuperPU] = CurLoadDestBlkSN;
            if (CurLoadDestBlkSN >= LPN_PER_BUF)
            {
                l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_WaitLoadDestBlk;
                break;
            }
        }
        break;
    case Rebuild_ErrTLC_WaitLoadDestBlk:
        for (i = 0; i< LPN_PER_BUF; i++)
        {
            pDestStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][i][uLUNInSuperPU];
            if (SUBSYSTEM_STATUS_PENDING == *pDestStatus)
            {
                break;
            }
        }

        //all finish
        if (LPN_PER_BUF == i)
        {
            l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_RebuildPMT;
        }
        break;
    case Rebuild_ErrTLC_RebuildPMT:
        for (i = 0; i< LPN_PER_BUF; i++)
        {
            //get err tlc block Addr
            CurErrTLCBlkSN = l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[ulSuperPU][uLUNInSuperPU];
            ErrTLCBlk = l_RebuildErrTLCDptr->m_ErrTLCBlk[ulSuperPU][uLUNInSuperPU][CurErrTLCBlkSN];
            CurErrTLCPageSN = l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU];

            pDestFlashAddr = &l_RebuildErrTLCDptr->m_LoadDestBlkAddr[ulSuperPU][uLUNInSuperPU][i];

            ErrTLCFlashAddr.m_PUSer = ulSuperPU;
            ErrTLCFlashAddr.m_OffsetInSuperPage = uLUNInSuperPU;
            ErrTLCFlashAddr.m_BlockInPU = ErrTLCBlk;
            ErrTLCFlashAddr.m_PageInBlock = CurErrTLCPageSN;
            ErrTLCFlashAddr.m_LPNInPage = i;

            //get tlc blk time
            pSpareOfErrTLC = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][0][0][uLUNInSuperPU];

            //LPN of  TLC Page
            LPNOfErrTLC = pSpareOfErrTLC->m_DataRed.aCurrLPN[i];

            if (INVALID_8F == LPNOfErrTLC || pDestFlashAddr->m_PPN == ErrTLCFlashAddr.m_PPN)
            {
                continue;
            }
            //Time of Err TLC Blk
            uTimeOfErrTLCBlk = pSpareOfErrTLC->m_RedComm.ulTimeStamp;

            //Dest Blk Addr
            pDestBlkAddr = &l_RebuildErrTLCDptr->m_LoadDestBlkAddr[ulSuperPU][uLUNInSuperPU][i];

            if (INVALID_8F == pDestBlkAddr->m_PPN)
            {
                //FIRMWARE_LogInfo("TLC(%d_%d_%d_%d) LPN 0x%x -> 0x%x TLCTime %d DestBlkAddr INVALID_8F\n",
                //    ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, ErrTLCFlashAddr.m_PageInBlock, ErrTLCFlashAddr.m_LPNInPage
                //    , LPNOfErrTLC, ErrTLCFlashAddr.m_PPN, uTimeOfErrTLCBlk);

                L2_UpdatePMT(&ErrTLCFlashAddr, NULL, LPNOfErrTLC);
                continue;
            }

            //If dest Blk EmptyPage
            pDestStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][i][uLUNInSuperPU];
            if (SUBSYSTEM_STATUS_EMPTY_PG == *pDestStatus)
            {
                //FIRMWARE_LogInfo("TLC(%d_%d_%d_%d) LPN 0x%x TLCTime %d DestBlkAddr EmptyPage,DestAddr %d_%d_%d_%d\n",
                //    ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, ErrTLCFlashAddr.m_PageInBlock, ErrTLCFlashAddr.m_LPNInPage,
                //    LPNOfErrTLC, uTimeOfErrTLCBlk,pDestBlkAddr->m_PUSer, pDestBlkAddr->m_BlockInPU, pDestBlkAddr->m_PageInBlock, pDestBlkAddr->m_LPNInPage);

                L2_UpdatePMT(&ErrTLCFlashAddr, NULL, LPNOfErrTLC);
                continue;
            }
            else if (SUBSYSTEM_STATUS_FAIL == *pDestStatus)
            {
                DBG_Printf("//////Caution:LPN OldMapping read fail///// TLC(%d_%d_%d_%d) LPN 0x%x TLCTime %d DestBlkAddr FAIL,DestAddr %d_%d_%d_%d\n",
                    ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, ErrTLCFlashAddr.m_PageInBlock, ErrTLCFlashAddr.m_LPNInPage,
                    LPNOfErrTLC, uTimeOfErrTLCBlk,pDestBlkAddr->m_PUSer, pDestBlkAddr->m_BlockInPU, pDestBlkAddr->m_PageInBlock, pDestBlkAddr->m_LPNInPage);
#ifdef SIM
                DBG_Getch();
#endif
            }


            pSpareOfDestBlk = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][i][uLUNInSuperPU];
            DestLPNOffset = pDestBlkAddr->m_LPNInPage;
            LPNOfDest = pSpareOfDestBlk->m_DataRed.aCurrLPN[DestLPNOffset];
            //Trimed item
            if (LPNOfErrTLC != LPNOfDest)
            {
                //FIRMWARE_LogInfo("TLC(%d_%d_%d_%d) LPN 0x%x TLCTime %d DestBlkAddr Trimed\n",
                //    ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, ErrTLCFlashAddr.m_PageInBlock, ErrTLCFlashAddr.m_LPNInPage, LPNOfErrTLC, uTimeOfErrTLCBlk);
                
                L2_UpdatePMT(&ErrTLCFlashAddr, NULL, LPNOfErrTLC);
                continue;
            }

            //Time of Dest Blk
            uTimeOfDestBlk = pSpareOfDestBlk->m_RedComm.ulTimeStamp;

            //if new, update PMT
            if (uTimeOfErrTLCBlk > uTimeOfDestBlk)
            {
                //FIRMWARE_LogInfo("TLC(%d_%d_%d_%d) LPN 0x%x TLCTime %d DestBlkAddr Succeed ,DestAddr %d_%d_%d_%d DestTime %d\n",
                //    ErrTLCFlashAddr.m_PUSer, ErrTLCFlashAddr.m_BlockInPU, ErrTLCFlashAddr.m_PageInBlock, ErrTLCFlashAddr.m_LPNInPage
                //    , LPNOfErrTLC, uTimeOfErrTLCBlk,pDestBlkAddr->m_PUSer, pDestBlkAddr->m_BlockInPU, pDestBlkAddr->m_PageInBlock, pDestBlkAddr->m_LPNInPage,uTimeOfDestBlk);
                if (ErrTLCFlashAddr.m_PPN != pDestBlkAddr->m_PPN)
                {
                    L2_UpdatePMT(&ErrTLCFlashAddr, NULL, LPNOfErrTLC);
                }
            }
        }

        l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_LoadNextTLCPage;
        break;
    case  Rebuild_ErrTLC_LoadNextTLCPage:
        //Next Page        
        CurErrTLCPageSN = l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU];
        CurErrTLCPageSN++;

        //This tlc blk finish
        if (CurErrTLCPageSN >= PG_PER_SLC_BLK * PG_PER_WL - 3)
        {
            CurErrTLCPageSN = 0;
            l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU] = CurErrTLCPageSN;

            //Next tlc blk
            CurErrTLCBlkSN = l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[ulSuperPU][uLUNInSuperPU];
            CurErrTLCBlkSN++;

            //if finish?
            l_RebuildErrTLCDptr->m_CurErrTLCBlkSN[ulSuperPU][uLUNInSuperPU] = CurErrTLCBlkSN;
            if (CurErrTLCBlkSN >= l_RebuildErrTLCDptr->m_ErrTLCBlkCnt[ulSuperPU][uLUNInSuperPU])
            {
                l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_Done;
                break;
            }

        }
        l_RebuildErrTLCDptr->m_CurErrTLCPageSN[ulSuperPU][uLUNInSuperPU] = CurErrTLCPageSN;
        l_RebuildErrTLCDptr->m_RebuildErrTLCState[ulSuperPU][uLUNInSuperPU] = Rebuild_ErrTLC_LoadTLCPage;
        break;
    case Rebuild_ErrTLC_Done:
        ret = TRUE;
        break;
    default:
        break;
    }

    return ret;
}


BOOL MCU1_DRAM_TEXT L2_RebuildErrTLCOfAllPu()
{
    U32 ulSuperPU;
    U32 ret;
    U32 AllFinishFlag = TRUE;
    U32 uLUNInSuperPU;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            ret = L2_RebuildErrTLC(ulSuperPU, uLUNInSuperPU);
            if (FALSE == ret)
            {
                AllFinishFlag = FALSE;
            }
        }
    }
    return AllFinishFlag;
}

//All PU Finish Move Blk , return TRUE
//Not finish , return FALSE
BOOL MCU1_DRAM_TEXT L2_RebuildErrHandleAllCE()
{
    U32 ulSuperPU;
    U32 uLUNInSuperPU;
    U32 SrcBlk;
    U16 DstBlk;
    U32 SrcPage;
    //BOOL All_Finish_Flag = TRUE;
    U32 ret;
    U32 MaxPage;
    U32 ulTarget;
    U8 * pStatus;
    U8 ucMoveIdx;
    U16 usVirBlk;
    U16 FailPhyBlk[LUN_NUM_PER_SUPERPU];

    //single PU move blk
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        BOOL bShouldSkipSuperPU = FALSE;
        //Is Finish Err Handle
        //if (!L2_IsErrHandleFinish(ulSuperPU))
        if (FALSE == l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU])
        {
            //All_Finish_Flag = FALSE;
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {            
                if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulSuperPU, uLUNInSuperPU)))
                {
                    bShouldSkipSuperPU = TRUE;
                    break;
                }
            }

            if(bShouldSkipSuperPU)
            {
                continue;
            }

            //Get Src Blk and page
            SrcBlk = L2_RebuildGetErrHandleSrcBlk(ulSuperPU,&ulTarget);
            SrcPage = L2_RebuildGetErrHandleSrcPage(ulSuperPU);

            //Get Dst Blk
            DstBlk = L2_RebuildGetErrHandleDstBlk(ulSuperPU);
            if (INVALID_4F == DstBlk)
            {
                /* no free block, no to do move open block */
                DBG_Printf("SPU %d no free block to move open block!!!\n",ulSuperPU);

                l_ErrHandleDptr->m_SrcBlk[ulSuperPU] = INVALID_8F;
                l_ErrHandleDptr->m_DstBlk[ulSuperPU] = INVALID_8F;
                l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU] = TRUE;
                l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU] = TRUE;
                continue;
            }

            //Move page
            ret = L2_RebuildMovePage(ulSuperPU, SrcBlk, DstBlk, SrcPage);

            //Finish copy one page
            if (TRUE == ret)
            {
                U32 NextPage = SrcPage + 1;

                MaxPage = g_PuInfo[ulSuperPU]->m_TargetPPO[ulTarget];

                /* If this target block patial closed, targetPPO = PG_PER_SLC_BLK -1 and target bitmap = 0. should adjust copy page num and bitmap */
                if ((PG_PER_SLC_BLK - 1) == MaxPage && 0 != g_PuInfo[ulSuperPU]->m_RPMTFlushBitMap[ulTarget])
                {
                    //FIRMWARE_LogInfo("%s SPU %d SrcBlk 0x%x PPO %d RPMTFlushBitMap 0x%x\n", __FUNCTION__, ulSuperPU, SrcBlk, MaxPage, g_PuInfo[ulSuperPU]->m_RPMTFlushBitMap[ulTarget]);
                    MaxPage += 1;
                }

                /* move finish */
                if (NextPage >= MaxPage)
                {
                    U16 PhySrcBlk[LUN_NUM_PER_SUPERPU], PhyDstBlk[LUN_NUM_PER_SUPERPU];
                    U32 BlkSN;
                    //U32 NeedMoveBlkCnt;

                    BlkSN = l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU];
                    for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                    {
                        PhySrcBlk[uLUNInSuperPU] = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, SrcBlk);
                        PhyDstBlk[uLUNInSuperPU] = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, DstBlk);
                    }

                    L2_PBIT_VBT_Swap_Block(ulSuperPU, SrcBlk, DstBlk);
                    for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
                    {
                        pStatus = &l_ErrHandleDptr->m_EraseStatus[ulSuperPU][BlkSN][uLUNInSuperPU]; 

                        L2_Exchange_PBIT_Info(ulSuperPU, uLUNInSuperPU, PhySrcBlk[uLUNInSuperPU], PhyDstBlk[uLUNInSuperPU]);
                        L2_PBIT_Set_Free(ulSuperPU, uLUNInSuperPU, PhyDstBlk[uLUNInSuperPU], FALSE);
                        L2_PBIT_Set_Allocate(ulSuperPU, uLUNInSuperPU, PhyDstBlk[uLUNInSuperPU], TRUE);
                        L2_FtlEraseBlock(ulSuperPU, uLUNInSuperPU, DstBlk, pStatus, FALSE, TRUE, FALSE);

                        DBG_Printf("TB MoveBlk SPU %d LUN %d SrcBlk 0x%x(PhyBlk 0x%x)<-->DstBlk 0x%x(PhyBlk 0x%x),Copy 0x%x Pages\n",
                            ulSuperPU, uLUNInSuperPU, SrcBlk, PhySrcBlk[uLUNInSuperPU], DstBlk, PhyDstBlk[uLUNInSuperPU], MaxPage);
                    }
                    L2_BM_CollectFreeBlock_Rebuild(ulSuperPU, DstBlk);
                    l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU] = TRUE;
#if 0
                    NextPage = 0;

                    l_ErrHandleDptr->m_SrcBlk[ulSuperPU] = INVALID_8F;
                    l_ErrHandleDptr->m_DstBlk[ulSuperPU] = INVALID_8F;

                    //Get next need move Blk
                    BlkSN++;
                    l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU] = BlkSN;

                    //if finish
                    NeedMoveBlkCnt = l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU];
                    if (BlkSN >= NeedMoveBlkCnt)
                    {
                        l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU] = TRUE;
                    }
#endif
                }
                //Next Page
                SrcPage = NextPage;
                l_ErrHandleDptr->m_ErrHandlePage[ulSuperPU] = SrcPage;
            }
        }
#ifndef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
        else
        {
            /* after fullfill gc last partial write pages, update target ppo and bitmap */
            if (0 != g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[TARGET_HOST_GC])
            {
                g_PuInfo[ulSuperPU]->m_TargetPPO[TARGET_HOST_GC]++;
                g_PuInfo[ulSuperPU]->m_TargetOffsetBitMap[TARGET_HOST_GC] = 0;
            }
        }
#endif        
    }

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        if (TRUE == l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU])
        {
            ucMoveIdx = l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU];
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                pStatus = &l_ErrHandleDptr->m_EraseStatus[ulSuperPU][ucMoveIdx][uLUNInSuperPU];
                if (SUBSYSTEM_STATUS_PENDING == (*pStatus))
                {
                    return FALSE;
                }
            }

            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                pStatus = &l_ErrHandleDptr->m_EraseStatus[ulSuperPU][ucMoveIdx][uLUNInSuperPU];
                usVirBlk = l_ErrHandleDptr->m_MoveTargetBlk[ulSuperPU][ucMoveIdx];
                if (SUBSYSTEM_STATUS_FAIL == (*pStatus))
                {
                    U16 usPhyBlk;
                    U16 usBrokenPBN;
                    U8  ucLUNInSuperPU;
                    U8  ucTLun;

                    DBG_Printf("Original target erase fail after target move done\n");
                    usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, usVirBlk);
                    if (INVALID_4F != usPhyBlk)
                    {
                        L2_PBIT_Set_Error(ulSuperPU, uLUNInSuperPU, usPhyBlk, TRUE);
                    }
                    else
                    {
                        L2_PBIT_Set_Error(ulSuperPU, uLUNInSuperPU, FailPhyBlk[uLUNInSuperPU], TRUE);
                    }

                    if ((TRUE == L2_BM_BackUpBlockEmpty(ulSuperPU, uLUNInSuperPU)) && (TRUE == L2_BM_BrokenBlockEmpty(ulSuperPU, uLUNInSuperPU, FALSE)))
                    {
                        if (INVALID_4F != usPhyBlk)
                        {
                            for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
                            {
                                FailPhyBlk[ucLUNInSuperPU] = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, ucLUNInSuperPU, usVirBlk);
                                if (uLUNInSuperPU != ucLUNInSuperPU)
                                {
                                    /* set other phy block as broken block */
                                    usBrokenPBN = FailPhyBlk[ucLUNInSuperPU];
                                    L2_PBIT_Set_Broken(ulSuperPU, ucLUNInSuperPU, usBrokenPBN, TRUE);
                                    L2_PBIT_Set_Allocate(ulSuperPU, ucLUNInSuperPU, usBrokenPBN, FALSE);
                                    L2_PBIT_Set_Free(ulSuperPU, ucLUNInSuperPU, usBrokenPBN, TRUE);
                                    L2_PBIT_SetVirturlBlockAddr(ulSuperPU, ucLUNInSuperPU, usBrokenPBN, INVALID_4F);
                                    DBG_Printf("%s MarkAsBroken Pu %d LunInSuperPu %d BrokenPBN %d done\n", __FUNCTION__, ulSuperPU, ucLUNInSuperPU, usBrokenPBN);
                                }
                                L2_VBT_SetPhysicalBlockAddr(ulSuperPU, ucLUNInSuperPU, usVirBlk, INVALID_4F);
                            }
                            L2_VBT_Set_Free(ulSuperPU, usVirBlk, FALSE);
                            L2_PBIT_Decrease_TotalBlockCnt(ulSuperPU, BLKTYPE_SLC);
                            g_PuInfo[ulSuperPU]->m_DataBlockCnt[BLKTYPE_SLC]--;
                            g_PuInfo[ulSuperPU]->m_SLCMLCFreePageCnt -= PG_PER_SLC_BLK;
                            /* update bad blk PBIT & insert to BBT */
                            L2_PBIT_Set_Free(ulSuperPU, uLUNInSuperPU, usPhyBlk, FALSE);

                            ucTLun = L2_GET_TLUN(ulSuperPU, uLUNInSuperPU);
                            if (FALSE == L2_BbtIsGBbtBadBlock(ucTLun, usPhyBlk))
                            {
                                L2_BbtAddBbtBadBlk(ucTLun, usPhyBlk, RT_BAD_BLK, ERASE_ERR);
                                L2_BbtSetLunSaveBBTBitMap(ucTLun, TRUE);
                                CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_BBT);
                            }
                            else
                            {
                                DBG_Printf("L2_RebuildErrHandleAllCE: add the same block to BBT again! SuperPU %d LUN %d BadBLK %d\n", ulSuperPU, uLUNInSuperPU, usPhyBlk);
                                DBG_Getch();
                            }
                        }
                        else
                        {
                            L2_PBIT_Set_Free(ulSuperPU, uLUNInSuperPU, FailPhyBlk[uLUNInSuperPU], FALSE);
                            ucTLun = L2_GET_TLUN(ulSuperPU, uLUNInSuperPU);
                            if (FALSE == L2_BbtIsGBbtBadBlock(ucTLun, FailPhyBlk[uLUNInSuperPU]))
                            {
                                L2_BbtAddBbtBadBlk(ucTLun, FailPhyBlk[uLUNInSuperPU], RT_BAD_BLK, ERASE_ERR);
                                L2_BbtSetLunSaveBBTBitMap(ucTLun, TRUE);
                                CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_BBT);
                            }
                            else
                            {
                                DBG_Printf("L2_RebuildErrHandleAllCE: Add the same block to BBT again error! SuperPU %d LUN %d BadBLK %d\n", ulSuperPU, uLUNInSuperPU, FailPhyBlk[uLUNInSuperPU]);
                                DBG_Getch();
                            }
                        }
                    }
                    else
                    {
                        L2_ErrHReplaceBLK(ulSuperPU, uLUNInSuperPU, usVirBlk, FALSE, ERASE_ERR);
                        usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ulSuperPU, uLUNInSuperPU, usVirBlk);
                        L2_PBIT_Set_Free(ulSuperPU, uLUNInSuperPU, usPhyBlk, TRUE);
                        L2_PBIT_Set_Allocate(ulSuperPU, uLUNInSuperPU, usPhyBlk, FALSE);
                        //DBG_Getch();
                    }
                }
            }

            l_ErrHandleDptr->m_ErrHandlePage[ulSuperPU] = 0;
            l_ErrHandleDptr->m_SrcBlk[ulSuperPU] = INVALID_8F;
            l_ErrHandleDptr->m_DstBlk[ulSuperPU] = INVALID_8F;

            //Get next need move Blk
            l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU]++;
            l_ErrHandleDptr->m_FinishErrHandleOneBlock[ulSuperPU] = FALSE;
            //if finish
            if (l_ErrHandleDptr->m_ErrHandleBlockSN[ulSuperPU] >= l_ErrHandleDptr->m_NeedMoveBlkCnt[ulSuperPU])
            {
                l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU] = TRUE;
            }
        }
    }

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        if (FALSE == l_ErrHandleDptr->m_FinishErrHandleFlag[ulSuperPU])
        {
            return FALSE;
        }
    }
    return TRUE;
}

/********************************************************/
//calc dirty cnt
/********************************************************/
extern void MCU1_DRAM_TEXT L2_InitDirtyLpnMap(U8 ucSuperPu);
void MCU1_DRAM_TEXT L2_InitCalcDirtyCntOfRebuild()
{
    U32 ulSuperPU;

    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        l_CalcDirtyCntDptr->m_CalcDirtyCntState[ulSuperPU] = Rebuild_CalcDirtyCnt_Prepare;
        l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT;
        l_CalcDirtyCntDptr->m_LoadSpareState[ulSuperPU] = RebuildLoadSpare_Read;
        l_CalcDirtyCntDptr->m_LoadSparePPO[ulSuperPU] = 0;

        /* clear round status */
        l_CalcDirtyCntDptr->m_CalcDirtyRoundStatus[ulSuperPU] = FALSE;
        l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSNInRound[ulSuperPU] = 0;
		l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU] = 0;
    }

    l_CalcDirtyCntDptr->m_RoundSN = 0;
    l_CalcDirtyCntDptr->m_CanDoFTLWrite = FALSE;
    l_CalcDirtyCntDptr->m_AccRoundSN = INVALID_8F;
    l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCntInRound = VIR_BLK_CNT / PG_PER_SLC_BLK;
    l_CalcDirtyCntDptr->m_RebuildDirtyCntStage = Rebuild_DirtyCnt_Bootup;

    /* clear m_bValidBitMap */
#ifdef PMT_ITEM_SIZE_REDUCE
    U32 i;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        HAL_DMAESetValue((U32)l_CalcDirtyCntDptr->m_bValidBitMap[i], COM_MemSize16DWAlign(PMTPAGE_CNT_PER_PU * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[0][0])), 0x0);
    }
#else    
    HAL_DMAESetValue((U32)l_CalcDirtyCntDptr->m_bValidBitMap, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[0])), 0x0);
#endif
    for (ulSuperPU = 0; ulSuperPU < SUBSYSTEM_SUPERPU_NUM; ulSuperPU++)
    {
        L2_InitDirtyLpnMap(ulSuperPU);
    }

    return;
}

RebuildCalcDirtyCntState MCU1_DRAM_TEXT L2_RebuildGetCalcDirtyCntState(U32 uSuperPUSer)
{
    return l_CalcDirtyCntDptr->m_CalcDirtyCntState[uSuperPUSer];
}

void MCU1_DRAM_TEXT L2_SetDirtyCntInVBT(U32 ulSuperPU, U32 Blk, U32 DirtyCnt)
{
#ifdef DirtyLPNCnt_IN_DSRAM1
    *(g_pDirtyLPNCnt + ulSuperPU*VIR_BLK_CNT + Blk) = DirtyCnt; 
#else
    pVBT[ulSuperPU]->m_VBT[Blk].DirtyLPNCnt = DirtyCnt;
#endif

    return;
}

void MCU1_DRAM_TEXT L2_ResetLpnBitMapInDPBM(U32 ulSuperPU, U32 Blk)
{
    HAL_DMAESetValue((U32)(&pDPBM->m_LpnMap[ulSuperPU][Blk]), sizeof(DPBM_ENTRY), 0);

    return;
}

void  MCU1_DRAM_TEXT L2_PrepareCaldDirtyCnt(U32 ulSuperPU)
{
    U32 BlkSN;
    U32 NeedCalcDirtyCntBlkCnt = 0;
    U32 PMTIndexInPu;

    //Find all need  rebuid dirtycnt blk
    for (BlkSN = 0; BlkSN < VIR_BLK_CNT; BlkSN++)
    {
        if ((FALSE == L2_IsPBNEmtpy(ulSuperPU, BlkSN)) && (FALSE == L2_IsPBNError(ulSuperPU, BlkSN)))
        {
            l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU][NeedCalcDirtyCntBlkCnt] = BlkSN;
            NeedCalcDirtyCntBlkCnt++;
        }
        else
        {
            L2_SetDirtyCntInVBT(ulSuperPU, BlkSN, 0);
            L2_ResetLpnBitMapInDPBM(ulSuperPU, BlkSN);
        }
    }
    l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU] = NeedCalcDirtyCntBlkCnt;
    l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSN[ulSuperPU] = 0;
    l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSNInRound[ulSuperPU] = 0;
	l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU] = 0;
    l_CalcDirtyCntDptr->m_CalcDirtyTLCBlkCnt[ulSuperPU] = 0;
    DBG_Printf("#### SPU %d RbDirtyCntBlkNum %d ####\n", ulSuperPU, NeedCalcDirtyCntBlkCnt);

    for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_PU; PMTIndexInPu++)
    {
        l_CalcDirtyCntDptr->m_ValidLPNCountCalc[ulSuperPU][PMTIndexInPu] = 0;
    }

}
void MCU1_DRAM_TEXT L2_RebuildSetCalcDirtyCntState(U32 ulSuperPU, RebuildCalcDirtyCntState State)
{
    l_CalcDirtyCntDptr->m_CalcDirtyCntState[ulSuperPU] = State;
}

//Return True: all page spare read finish
//Return False: not finish
BOOL MCU1_DRAM_TEXT L2_RebuildReadRPMTErrHandle(U32 ulSuperPU, U32 Blk)
{
    BOOL ret = FALSE;
    RebuildLoadSpareState ReadRPMTStateState = l_CalcDirtyCntDptr->m_LoadSpareState[ulSuperPU];
    U32 Page = l_CalcDirtyCntDptr->m_LoadSparePPO[ulSuperPU];
    U32 ulPagePerBlk;
    BOOL bTLCFlag;
    U8* pStatus;
    RED* pSpare;
    U32 uWordLine;
    U32 uPageInWordLine;
    U32 ucLunInSuperPu;
    U32 ucTLun;
    U32 uLoadRPMTBitMap;

    bTLCFlag = L2_VBT_Get_TLC(ulSuperPU, Blk);

    if (bTLCFlag)
    {
        uWordLine = Page / PG_PER_WL;
        uPageInWordLine = Page % PG_PER_WL;
    }


    switch (ReadRPMTStateState)
    {
    case RebuildLoadSpare_Read:
    {
        PhysicalAddr Addr = { 0 };

        Addr.m_PUSer = ulSuperPU;    
        Addr.m_BlockInPU = Blk;
        Addr.m_PageInBlock = Page;
        Addr.m_LPNInPage = 0;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            Addr.m_OffsetInSuperPage =  ucLunInSuperPu;

            ucTLun = L2_GET_TLUN(ulSuperPU, ucLunInSuperPu);

            //Check Resource
            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                continue;
            }

            uLoadRPMTBitMap = l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU];

            //if this Lun RPMT Load finish, load next Lun RPMT
            if (0 != (uLoadRPMTBitMap & (1 << ucLunInSuperPu)))
            {
                continue;
            }

            //get status
            if (bTLCFlag)
            {
                pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
                pStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
            }
            else
            {
                pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][Page][ucLunInSuperPu];
                pSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][Page][ucLunInSuperPu];
            }

            L2_LoadSpare(&Addr, pStatus, (U32*)pSpare, !(bTLCFlag));

            uLoadRPMTBitMap |= 1 << ucLunInSuperPu;
            //If all LUN finish
            if (SUPERPU_LUN_NUM_BITMSK == uLoadRPMTBitMap)
            {
                uLoadRPMTBitMap = 0;
                l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU] = uLoadRPMTBitMap;
                l_CalcDirtyCntDptr->m_LoadSpareState[ulSuperPU] = RebuildLoadSpare_WaitStatus;
                goto out;
            }
            l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU] = uLoadRPMTBitMap;

        }
    }
        break;
    case RebuildLoadSpare_WaitStatus:
    {
        //Wait Status
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            //get status
            if (bTLCFlag)
            {
                pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
                pStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
            }
            else
            {
                pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][Page][ucLunInSuperPu];
                pSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][Page][ucLunInSuperPu];
            }

            if (SUBSYSTEM_STATUS_PENDING == *pStatus)
                goto out;
        }

        //empty page
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            //get status
            if (bTLCFlag)
            {
                pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
                pStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
            }
            else
            {
                pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][Page][ucLunInSuperPu];
                pSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][Page][ucLunInSuperPu];
            }

            if (SUBSYSTEM_STATUS_EMPTY_PG == *pStatus)
            {
                DBG_Printf("FUN:L2_RebuildReadRPMTErrHandle read RPMT page EMPTY: PU %d Blk %d Page %d LunInSuperPage %d\n",
                    ulSuperPU, Blk, Page, ucLunInSuperPu);
                DBG_Getch();
            }
        }

        //fail
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            //get status
            if (bTLCFlag)
            {
                pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
                pStatus = (U8*)&l_BufferManagerDptr->m_TLCSpareBufferStatus[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
            }
            else
            {
                pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][Page][ucLunInSuperPu];
                pSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][Page][ucLunInSuperPu];
            }

            if (SUBSYSTEM_STATUS_FAIL == *pStatus)
            {
                U32 i;

                DBG_Printf("FUN:L2_RebuildReadRPMTErrHandle read RPMT page Fail: PU %d Blk %d Page %d LunInSuperPage %d\n",
                    ulSuperPU, Blk, Page, ucLunInSuperPu);
                //Init
                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    pSpare->m_DataRed.aCurrLPN[i] = INVALID_8F;
                }
            }
        }

        //fail and Success status, then m_LoadSpareState change
        l_CalcDirtyCntDptr->m_LoadSpareState[ulSuperPU] = RebuildLoadSpare_Done;
    }
        break;
    case RebuildLoadSpare_Done:
        if (bTLCFlag)
        {
            //TLC Blk
            ulPagePerBlk = PG_PER_SLC_BLK * PG_PER_WL;

            //Next Page
            Page++;
            //If next page is 381
            if (Page >= (ulPagePerBlk - PG_PER_WL))
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    //add Init last page spare LPN to INVALID_8F
                    pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][PG_PER_SLC_BLK - 1][0][ucLunInSuperPu];
                    COM_MemSet((U32*)&pSpare->m_DataRed.aCurrLPN[0], LPN_PER_BUF, INVALID_8F);

                    pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][PG_PER_SLC_BLK - 1][1][ucLunInSuperPu];
                    COM_MemSet((U32*)&pSpare->m_DataRed.aCurrLPN[0], LPN_PER_BUF, INVALID_8F);

                    pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][PG_PER_SLC_BLK - 1][2][ucLunInSuperPu];
                    COM_MemSet((U32*)&pSpare->m_DataRed.aCurrLPN[0], LPN_PER_BUF, INVALID_8F);
                }
                ret = TRUE;
                Page = 0;
            }
        }
        else
        {
            ulPagePerBlk = PG_PER_SLC_BLK;

            //Next Page
            Page++;
            if (Page >= (ulPagePerBlk - 1))
            {
                //add Init last page spare LPN to INVALID_8F
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    COM_MemSet((U32*)&l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][ulPagePerBlk - 1][ucLunInSuperPu]->m_DataRed.aCurrLPN[0], LPN_PER_BUF, INVALID_8F);
                }
                ret = TRUE;
                Page = 0;
            }

        }
        l_CalcDirtyCntDptr->m_LoadSparePPO[ulSuperPU] = Page;
        l_CalcDirtyCntDptr->m_LoadSpareState[ulSuperPU] = RebuildLoadSpare_Read;
        break;
    default:
        break;
    }
out:
    return ret;
}

void UpdateValidBitMapOfLPN(U8 ucSuperPU, U32 PMTIndex, U32 LPNInPMTPage)
{
    U32 index = LPNInPMTPage / 32;
    U32 bit = LPNInPMTPage % 32;

    l_CalcDirtyCntDptr->m_bValidBitMap[ucSuperPU][PMTIndex][index] |= (1 << bit);
}

BOOL L2_RebuildIsValidLPN(U8 ucPU, U32 PMTIndex, U32 LPNInPMTPage)
{
    U32 index = LPNInPMTPage / 32;
    U32 bit = LPNInPMTPage % 32;

    return (l_CalcDirtyCntDptr->m_bValidBitMap[ucPU][PMTIndex][index] & (1 << bit));
}
extern U32 CalcDiffTime(U32 StartTime, U32 EndTime);

void MCU1_DRAM_TEXT L2_ProcessTrimedLPN(U32 ulSuperPU)
{
    U32 uPMTIndexInPu;
    PhysicalAddr InvalidAddr = { 0 };
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMT;
#else
    PMTPage* pPMT;
#endif
    PhysicalAddr Addr = { 0 };
    U32 i;
    U32 ValidLPNCountCalc;
    U32 ValidLPNCountSave;

#ifdef ASIC
    U32 StartTime, EndTime, DiffTime;
    StartTime = XT_RSR_CCOUNT();
#endif

    InvalidAddr.m_PPN = INVALID_8F;

    for (uPMTIndexInPu = 0; uPMTIndexInPu < PMTPAGE_CNT_PER_PU; uPMTIndexInPu++)
    {
        pPMT = g_PMTManager->m_pPMTPage[ulSuperPU][uPMTIndexInPu];

#ifdef ValidLPNCountSave_IN_DSRAM1
        ValidLPNCountSave = U24getValue(ulSuperPU, uPMTIndexInPu);
#else
        ValidLPNCountSave = g_PMTManager->m_PMTSpareBuffer[ulSuperPU][uPMTIndexInPu]->m_ValidLPNCountSave;
#endif
        ValidLPNCountCalc = l_CalcDirtyCntDptr->m_ValidLPNCountCalc[ulSuperPU][uPMTIndexInPu];

        TRACE_LOG((void*)&uPMTIndexInPu, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: PMTIIndexInPu ");
        TRACE_LOG((void*)&ValidLPNCountSave, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: ValidLPNCountSave ");
        TRACE_LOG((void*)&ValidLPNCountCalc, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: ValidLPNCountCalc ");


        /*if((0 == ValidLPNCountCalc) && (0 == ValidLPNCountSave))
        {
            continue;
        }
        else */
        if (ValidLPNCountSave <= ValidLPNCountCalc)
        {
#ifdef ValidLPNCountSave_IN_DSRAM1
            U24setValue(ulSuperPU, uPMTIndexInPu, ValidLPNCountCalc); 
#else
            g_PMTManager->m_PMTSpareBuffer[ulSuperPU][uPMTIndexInPu]->m_ValidLPNCountSave = ValidLPNCountCalc;
#endif
            continue;
        }

        for (i = 0; i < LPN_CNT_PER_PMTPAGE; i++)
        {
            //LPNInSystem = L2_GetLPNFromPMTI(PUSer, uPMTIndexInPu, i);

            /* Is LPN Invalid */
            if (FALSE == L2_RebuildIsValidLPN(ulSuperPU, uPMTIndexInPu, i))
            {
#ifdef PMT_ITEM_SIZE_3BYTE
                L2_PMTItemToPhyAddr(&Addr, &pPMT->m_PMTItems[i]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
                L2_PMTItemToPhyAddr(&Addr, pPMT, i);
#else
                Addr.m_PPN = pPMT->m_PMTItems[i];
#endif

                if (INVALID_8F != Addr.m_PPN)
                {                    
#ifdef PMT_ITEM_SIZE_3BYTE
                    L2_PhyAddrToPMTItem(&InvalidAddr, &pPMT->m_PMTItems[i]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
                    L2_PhyAddrToPMTItem(&InvalidAddr, pPMT, i);
#else
                    pPMT->m_PMTItems[i] = INVALID_8F;
#endif
                    /* dirty PMT page in PMTI table */
                    L2_SetDirty(ulSuperPU, uPMTIndexInPu);
                }
            }
        }

#ifdef ValidLPNCountSave_IN_DSRAM1
        U24setValue(ulSuperPU, uPMTIndexInPu, ValidLPNCountCalc); 
#else
        g_PMTManager->m_PMTSpareBuffer[ulSuperPU][uPMTIndexInPu]->m_ValidLPNCountSave = ValidLPNCountCalc;
#endif
    }

#if 0 //def ASIC
    EndTime = XT_RSR_CCOUNT();
    DiffTime = CalcDiffTime(StartTime, EndTime);
    DiffTime = (DiffTime / 300);

    TRACE_LOG((void*)&PUSer, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: PUSer ");
    TRACE_LOG((void*)&DiffTime, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: cost ? uS. ");
#endif
}

void  L2_RebuildCalcDirtyCntOfOneBlk(U32 ulSuperPU, U32 Blk, U32 ulStartPage, U32 ulPageCount)
{
    U32 DirtyCnt = 0;
    PhysicalAddr TempAddr = { 0 }, RefAddr = { 0 };
    U16 Page;
    U16 LpnInPage;
    U32 ulLpn;
    RPMT* pRPMT;
#if (LPN_PER_BUF == 8)
    U8 LpnBitMap = 0;
#elif (LPN_PER_BUF == 16)
    U16 LpnBitMap = 0;
#else
    U16 LpnBitMap = 0;
#endif
    RED* pSpare;
    U8 ucTargetType;
    U16 usPagePerBlk;
    U16 usPPO;
    U32 ulOffsetInPMTPage;
    U32 ucLunInSuperPu;
    BOOL bTargetBlk = FALSE;
    U32 ulCheckBitmap;
    U16 PMTIIndexInPu = INVALID_4F;

#ifdef ASIC
    U32 StartTime, EndTime, DiffTime;
    StartTime = XT_RSR_CCOUNT();
#endif

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    if ((ulStartPage >= PG_PER_SLC_BLK) || (ulPageCount == 0) || (ulPageCount > PG_PER_SLC_BLK) || ((ulStartPage + ulPageCount) > PG_PER_SLC_BLK))
    {
        DBG_Printf("L2_RebuildCalcDirtyCntOfOneBlk: Incorrect Parameter Error: ulStartPage %d ulPageCount %d\n", ulStartPage, ulPageCount);
        DBG_Getch();
    }
    pRPMT = (RPMT*)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][0];
    if ((TRUE == l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU]) && (ulStartPage == 0))
    {
        /* Blk is target BLK and first page partial write case */
        bTargetBlk = L2_RebuildIsTargetBlk(ulSuperPU, Blk, &ucTargetType);

        if (TRUE == bTargetBlk && 1 == l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU][ucTargetType] && 
            0 != l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType])
        {
            ulCheckBitmap = l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType];
        }
        else
        {
            ulCheckBitmap = SUPERPU_LUN_NUM_BITMSK;
        }
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // only need check LUN do check
            if (COM_BitMaskGet(ulCheckBitmap, ucLunInSuperPu))
            {
                if (pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU != ulSuperPU || pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock != Blk)
                {
                    DBG_Printf("Load SPU %d LUN %d Blk %d RPMT error!!!\n", ulSuperPU, ucLunInSuperPu, Blk);
                    DBG_Printf("RPMT SPU %d Blk %d\n", pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU, pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock);
                    DBG_Printf("bTargetBlk %d ulCheckBitmap 0x%x\n", bTargetBlk, ulCheckBitmap);
                    DBG_Getch();
                }
            }
        }
    }

    DirtyCnt = 0;
    usPagePerBlk = PG_PER_SLC_BLK;

    bTargetBlk = L2_RebuildIsTargetBlk(ulSuperPU, Blk, &ucTargetType);
    if (TRUE == bTargetBlk)
    {
        usPPO = l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU][ucTargetType];     
    }
    else
    {
        /* RPMT don't need to calc DirtyCnt & DPBM */
        usPPO = usPagePerBlk - 1;      
    }

    //calc dirty cnt && recognize valid LPN
    TempAddr.m_PPN = 0;
    TempAddr.m_PUSer = ulSuperPU;
    TempAddr.m_BlockInPU = Blk;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        TempAddr.m_OffsetInSuperPage = ucLunInSuperPu;

        for (Page = ulStartPage; Page < (ulStartPage + ulPageCount); Page++)
        {
            LpnBitMap = 0;

            if (Page < usPPO)
            {
                /*skip no written LUN*/
                if ((Page == (usPPO - 1))
                    && (TRUE == bTargetBlk)
                    && (FALSE == L2_IsSubPPOLUNWritten(l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType], ucLunInSuperPu)))
                {
                    continue;
                }

                for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                {
                    TempAddr.m_PageInBlock = Page;
                    TempAddr.m_LPNInPage = LpnInPage;
                    if (TRUE == l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU])
                    {
                        ulLpn = L2_LookupRPMT(pRPMT, &TempAddr);
                    }
                    else
                    {
                        pSpare = l_BufferManagerDptr->m_SpareBuffer[ulSuperPU][0][Page][ucLunInSuperPu];
                        ulLpn = pSpare->m_DataRed.aCurrLPN[LpnInPage];
                    }

                    if (INVALID_8F == ulLpn)
                    {
                        DirtyCnt++;
                        continue;
                    }

                    /* lookup PMT to get reference physical addr from logic LPN */
                    L2_LookupPMT(&RefAddr, ulLpn,TRUE);

                    /* if not equel, data is dirty */
                    if (TempAddr.m_PPN != RefAddr.m_PPN)
                    {
                        DirtyCnt++;
                    }
                    else
                    {
                        LpnBitMap |= (1 << LpnInPage);
                        PMTIIndexInPu = L2_GetPMTIIndexInPu(ulLpn);
                        ulOffsetInPMTPage = L2_GetOffsetInPMTPage(ulLpn);

                        UpdateValidBitMapOfLPN(ulSuperPU, PMTIIndexInPu, ulOffsetInPMTPage);
                        l_CalcDirtyCntDptr->m_ValidLPNCountCalc[ulSuperPU][PMTIIndexInPu]++;
                    }
                }
            }

            //Save LPNBitMap
            pDPBM->m_LpnMap[ulSuperPU][Blk].m_LpnMapPerPage[ucLunInSuperPu][Page] = LpnBitMap;
        }
    }

    //Save DirtyCnt
    if (DirtyCnt > LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT)
    {
        DBG_Printf("SPU %d, block %d dirtycnt %d overflowed after table rebuild!\n", ulSuperPU, Blk, DirtyCnt);
        DBG_Getch();
    }
    L2_IncreaseDirty(ulSuperPU, Blk, DirtyCnt);

#ifdef SIM
    /* if no target blk, check dirty count */
    if ((ulStartPage + ulPageCount) == PG_PER_SLC_BLK)
    {
        PhysicalAddr Addr = { 0 };
        U32 ValidCnt = 0;
        S16 usEndCheckPG; /* change to singed , because, it may be negative when usPPO=1, and page 0 is partial write case */
        U32 ulLPNSum = 0;

        DirtyCnt = L2_GetDirtyCnt(ulSuperPU, Blk);
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            /*calculate end check page*/
            if (TRUE == bTargetBlk)
            {
                if (0 == l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType])
                {
                    usEndCheckPG = usPPO - 1;
                }
                else
                {
                    usEndCheckPG = (0 != (l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType] & (1 << ucLunInSuperPu))) ? (usPPO - 1) : (usPPO - 2);
                }            
            }
            else
            {
                usEndCheckPG = usPPO - 1;
            }
            for (Page = 0; Page <= usEndCheckPG; Page++)
            {
                Addr.m_PUSer = ulSuperPU;
                Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                Addr.m_BlockInPU = Blk;
                Addr.m_PageInBlock = Page;
                for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                {
                    Addr.m_LPNInPage = LpnInPage;
                    if (TRUE == L2_LookupDirtyLpnMap(&Addr))
                    {
                        ValidCnt++;
                    }
                }
            }
            ulLPNSum += (usEndCheckPG + 1) * LPN_PER_BUF;
        }

        if (ulLPNSum != (DirtyCnt + ValidCnt))
        {
            DBG_Printf("SPU %d, block %d dirtycnt %d ValidCnt %d LPNSum %d\n", ulSuperPU, Blk, DirtyCnt, ValidCnt, ulLPNSum);
            DBG_Getch();
        }
    }
#endif


#if 0 //def ASIC
    EndTime = XT_RSR_CCOUNT();
    DiffTime = CalcDiffTime(StartTime, EndTime);
    DiffTime = (DiffTime / 300);

    TRACE_LOG((void*)&PUSer, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: PUSer ? ");
    TRACE_LOG((void*)&Blk, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: Blk ? ");
    TRACE_LOG((void*)&DiffTime, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: cost ? uS. ");
#endif
}


void  L2_RebuildCalcDirtyCntOfOneTLCBlk(U32 ulSuperPU, U32 Blk, U32 ulStartPage, U32 ulPageCount)
{
    U32 DirtyCnt = 0;
    PhysicalAddr TempAddr = { 0 }, RefAddr = { 0 };
    U16 Page;
    U16 LpnInPage;
    U32 ulLpn;
    RPMT* pRPMT;
#if (LPN_PER_BUF == 8)
    U8 LpnBitMap = 0;
#elif (LPN_PER_BUF == 16)
    U16 LpnBitMap = 0;
#else
    U16 LpnBitMap = 0;
#endif
    RED* pSpare;
    U16 usPagePerBlk;
    U32 ulOffsetInPMTPage;
    U32 ulRPMTNum;
    U32 uWordLine, uPageInWordLine;
    U32 ucLunInSuperPu;
    U16 PMTIIndexInPu = INVALID_4F;

#ifdef ASIC
    U32 StartTime, EndTime, DiffTime;
    StartTime = XT_RSR_CCOUNT();
#endif

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    /* RPMT don't need to calc DirtyCnt & DPBM */
    DirtyCnt = 0;
    usPagePerBlk = (PG_PER_SLC_BLK - 1) * PG_PER_WL;
    if ((ulStartPage >= usPagePerBlk) || (ulPageCount == 0) || (ulPageCount > usPagePerBlk) || ((ulStartPage + ulPageCount) > usPagePerBlk))
    {
        DBG_Printf("L2_RebuildCalcDirtyCntOfOneTLCBlk: Incorrect Parameter Error: ulStartPage %d ulPageCount %d\n", ulStartPage, ulPageCount);
        DBG_Getch();
    }

    //calc dirty cnt && recognize valid LPN
    TempAddr.m_PPN = 0;
    TempAddr.m_PUSer = ulSuperPU;
    TempAddr.m_BlockInPU = Blk;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        TempAddr.m_OffsetInSuperPage = ucLunInSuperPu;

        for (Page = ulStartPage; Page < (ulStartPage + ulPageCount); Page++)
        {
            LpnBitMap = 0;

            for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
            {
                TempAddr.m_PageInBlock = Page;
                TempAddr.m_LPNInPage = LpnInPage;

                if (TRUE == l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU])
                {
                    if (Page < (PG_PER_SLC_BLK - 1)* PG_PER_WL)
                    {
                        uWordLine = Page % (PG_PER_SLC_BLK - 1);
                        ulRPMTNum = Page / (PG_PER_SLC_BLK - 1);
                    }
                    else
                    {
                        uWordLine = Page / PG_PER_WL;
                        ulRPMTNum = Page % PG_PER_WL;
                    }
                    pRPMT = (RPMT*)l_BufferManagerDptr->m_TLCPageBuffer[ulSuperPU][ulRPMTNum];
                    ulLpn = L2_LookupRPMTInTLC(pRPMT, &TempAddr);
                }
                else
                {
                    uWordLine = Page / PG_PER_WL;
                    uPageInWordLine = Page % PG_PER_WL;

                    pSpare = l_BufferManagerDptr->m_TLCSpareBuffer[ulSuperPU][uWordLine][uPageInWordLine][ucLunInSuperPu];
                    ulLpn = pSpare->m_DataRed.aCurrLPN[LpnInPage];
                }

                if (INVALID_8F == ulLpn)
                {
                    DirtyCnt++;
                    continue;
                }

                /* lookup PMT to get reference physical addr from logic LPN */
                L2_LookupPMT(&RefAddr, ulLpn,TRUE);

                /* if not equel, data is dirty */
                if (TempAddr.m_PPN != RefAddr.m_PPN)
                {
                    /*if (g_DebugPMT[ulLpn].m_PPN == RefAddr.m_PPN)
                    {
                        DBG_Printf("valid data recognize for not valid data DebugPMT %d_%d_%d_%d_%d RefAddr %d_%d_%d_%d_%d \n",
                            g_DebugPMT[ulLpn].m_PUSer, g_DebugPMT[ulLpn].m_BlockInPU, g_DebugPMT[ulLpn].m_PageInBlock, g_DebugPMT[ulLpn].m_LPNInPage, g_DebugPMT[ulLpn].m_OffsetInSuperPage,
                            RefAddr.m_PUSer, RefAddr.m_BlockInPU, RefAddr.m_PageInBlock, RefAddr.m_LPNInPage, RefAddr.m_OffsetInSuperPage);
                        DBG_Printf("LPN %d TempAddr %d_%d_%d_%d_%d\n", ulLpn,
                            TempAddr.m_PUSer, TempAddr.m_BlockInPU, TempAddr.m_PageInBlock, TempAddr.m_LPNInPage, TempAddr.m_OffsetInSuperPage);

                        DBG_Getch();
                    }*/
                    DirtyCnt++;
                }
                else
                {
                    LpnBitMap |= (1 << LpnInPage);
                    PMTIIndexInPu = L2_GetPMTIIndexInPu(ulLpn);
                    ulOffsetInPMTPage = L2_GetOffsetInPMTPage(ulLpn);

                    UpdateValidBitMapOfLPN(ulSuperPU, PMTIIndexInPu, ulOffsetInPMTPage);
                    l_CalcDirtyCntDptr->m_ValidLPNCountCalc[ulSuperPU][PMTIIndexInPu]++;
                }
            }

            //Save LPNBitMap
            pDPBM->m_LpnMap[ulSuperPU][Blk].m_LpnMapPerPage[ucLunInSuperPu][Page] = LpnBitMap;
        }
    }

    //Save DirtyCnt
    if (DirtyCnt > LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT)
    {
#ifdef DirtyLPNCnt_IN_DSRAM1
        DBG_Printf("SPU %d, block %d dirtycnt %d overflowed after table rebuild\n", ulSuperPU, Blk, *(g_pDirtyLPNCnt + ulSuperPU*VIR_BLK_CNT + Blk));
#else
        DBG_Printf("SPU %d, block %d dirtycnt %d overflowed after table rebuild\n", ulSuperPU, Blk, pVBT[ulSuperPU]->m_VBT[Blk].DirtyLPNCnt);
#endif
        DBG_Getch();
    }
    L2_IncreaseDirty(ulSuperPU, Blk, DirtyCnt);

#ifdef SIM
    /* if no target blk, check dirty count */
    if ((ulStartPage + ulPageCount) == usPagePerBlk)
    {
        PhysicalAddr Addr = { 0 };
        U32 ValidCnt = 0;

        DirtyCnt = L2_GetDirtyCnt(ulSuperPU, Blk);
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            Addr.m_OffsetInSuperPage = ucLunInSuperPu;
            for (Page = 0; Page < PG_PER_SLC_BLK * PG_PER_WL; Page++)
            {
                Addr.m_PUSer = ulSuperPU;
                Addr.m_BlockInPU = Blk;
                Addr.m_PageInBlock = Page;
                for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                {
                    Addr.m_LPNInPage = LpnInPage;
                    if (TRUE == L2_LookupDirtyLpnMap(&Addr))
                    {
                        ValidCnt++;
                    }
                }
            }
        }

        if ((LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT) != (DirtyCnt + ValidCnt))
        {
            DBG_Printf("SPU %d, block %d dirtycnt %d ValidCnt %d !!!\n", ulSuperPU, Blk, DirtyCnt, ValidCnt);
            DBG_Getch();
        }
    }
#endif


#ifdef ASIC
    EndTime = XT_RSR_CCOUNT();
    DiffTime = CalcDiffTime(StartTime, EndTime);
    DiffTime = (DiffTime / 300);

    TRACE_LOG((void*)&ulSuperPU, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: PUSer ? ");
    TRACE_LOG((void*)&Blk, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: Blk ? ");
    TRACE_LOG((void*)&DiffTime, sizeof(U32), U32, 0, "[L2_ProcessTrimedLPN]: cost ? uS. ");
#endif

}



/*****************************************************************************
 Prototype      : L2_RebuildLookupRPMTBuffer
 Description    : look up RPMT buffer for read cmd before abnormal boot up ok.
 Input          : U32 ulLPN
 Output         : PhysicalAddr* pAddr
 Return Value   : BOOL
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/01/06
 Author       : henryluo
 Modification : Created function in TaiPei

 *****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_RebuildLookupRPMTBuffer(PhysicalAddr* pAddr, U32 ulLPN)
{
    RPMT* pRPMT;
    U8 ucSuperPU;
    U32 ulLUNInSuperPU;
    PuInfo* pInfo;
    U32 i, j, k;
    U32 ulRPMTPointer;
    U32 ulTargetType;
    BOOL bLookUped = FALSE;
    U32 ulHitTS;
    U32 ulHitTSOrder;
    U32 ulSuperPage;
    U32 ulHitSuperPage;
    U32 ulHitLUN;
    U32 ulMaxHitTS = 0;
    U32 ulMaxHitTSOrder = 0;
    U32 ulSearchStartLPN;
    U32 ulLPNInLUN;
    U32 ulLPNInPage;
    U32 ulLPNInPageHit;

    ucSuperPU = L2_GetSuperPuFromLPN(ulLPN);
    pInfo = g_PuInfo[ucSuperPU];

    /* must not lookup the rand RPMT buffer because there is not GC during boot up */
    //for(i = 0; i < TARGET_ALL; i++)
    i = TARGET_HOST_WRITE_NORAML;
    {
        for (j = 0; j < RPMT_BUFFER_DEPTH; j++)
        {
            /* search the buffer start from ppo-1 for LPN expected */
            if (j > pInfo->m_RPMTBufferPointer[i])
            {
                break;
            }
            else if (0 == j && l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][i] >= 1)
            {
                ulSearchStartLPN = (l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][i] - 1) * LPN_PER_SUPERBUF;
            }
            else
            {
                ulSearchStartLPN = 0;
            }

            pRPMT = pInfo->m_pRPMT[i][j];

            for (k = ulSearchStartLPN; k < LPN_PER_SUPER_SLCBLK; k++) //not for TLC block
            {
                ulLUNInSuperPU = (k % LPN_PER_SUPERBUF)/LPN_PER_BUF;                
                ulSuperPage = k / LPN_PER_SUPERBUF;
                ulLPNInPage = k % LPN_PER_BUF;
                ulLPNInLUN = ulSuperPage*LPN_PER_BUF + ulLPNInPage;

                if (j == pInfo->m_RPMTBufferPointer[i])
                {
                    /* skip not written SuperPages */
                    if (ulSuperPage >= pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
                    {
                        break;
                    }                    
                }                 

                /* skip not written PPO-1 LUNs */
                if (ulSuperPage == pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1 && pInfo->m_TargetOffsetBitMap[i] != 0 && !COM_BitMaskGet(pInfo->m_TargetOffsetBitMap[i], ulLUNInSuperPU))
                {
                    continue;
                }
                if (pRPMT->m_RPMT[ulLUNInSuperPU].m_RPMTItems[ulLPNInLUN] == ulLPN)
                {
                    ulHitTS = pRPMT->m_RPMT[ulLUNInSuperPU].m_SuperPageTS[ulSuperPage];
                    ulHitTSOrder = pRPMT->m_RPMT[ulLUNInSuperPU].m_LunOrderTS[ulSuperPage];

                    /* Because L1 cache solution is modified,it may be existed same LPN in different LpnInPage of same Page.
                    for example:
                    1. First LPN0 is written to Pu 1,Blk1,Pg2,LpnInPage0;
                    2. Then LPN0 is written to Pu 1,Blk1,Pg2,LpnInPage1;
                    LpnInPage1 is newest data,so modify this compare TS code */
                    if (ulHitTS > ulMaxHitTS || (ulHitTS == ulMaxHitTS && ulHitTSOrder > ulMaxHitTSOrder))
                    {
                        ulMaxHitTS = ulHitTS;
                        ulMaxHitTSOrder = ulHitTSOrder;
                        ulTargetType = i;
                        ulRPMTPointer = j;
                        ulLPNInPageHit = ulLPNInPage;
                        ulHitLUN = ulLUNInSuperPU;
                        ulHitSuperPage = ulSuperPage;
                        bLookUped = TRUE;
                    }
                }
            }
        }
    }

    if (TRUE == bLookUped)
    {
        pAddr->m_PUSer = ucSuperPU;
        pAddr->m_OffsetInSuperPage = ulHitLUN;
        pAddr->m_PageInBlock = ulHitSuperPage;
        pAddr->m_LPNInPage = ulLPNInPageHit;

        if (TARGET_HOST_WRITE_NORAML != ulTargetType)
        {
            DBG_Printf("SPU %d ulTargetType %d error!\n", ucSuperPU, ulTargetType);
            DBG_Getch();
        }

        if (l_CalcDirtyCntDptr->m_TargetBlk[ucSuperPU][ulTargetType] == pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
        {
            if (0 != ulRPMTPointer)
            {
                DBG_Printf("SPU %d ulRPMTPointer %d error!\n", ucSuperPU, ulRPMTPointer);
                DBG_Getch();
            }

            pAddr->m_BlockInPU = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
        }
        else
        {
            if (0 == ulRPMTPointer)
            {
                pAddr->m_BlockInPU = l_CalcDirtyCntDptr->m_TargetBlk[ucSuperPU][ulTargetType];
            }
            else
            {
                pAddr->m_BlockInPU = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
            }
        }       
        //FIRMWARE_LogInfo("L2_RebuildLookupRPMTBuffer LPN 0x%x Addr %d_0x%x_0x%x_%d_%d\n", ulLPN, ucSuperPU, pAddr->m_BlockInPU, pAddr->m_PageInBlock, pAddr->m_LPNInPage, pAddr->m_OffsetInSuperPage);       

        return TRUE;
    }
    else
    {
        pAddr->m_PPN = INVALID_8F;
        return FALSE;
    }
}

/*****************************************************************************
 Prototype      : L2_AdjustDirtyCntOfOneBlk
 Description    : adjust PMT and dirty count according to RPMT.
 Input          : U32 PUSer,U32 Blk
 Output         : void
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/01/06
 Author       : henryluo
 Modification : Created function in TaiPei

 *****************************************************************************/

/* TargetPPO = LastPage has 3 cases, L2_IsNewData:
1. TargetPPO = LastPage,m_TargetOffsetBitMap != 0,m_RPMTFlushBitMap == 0;
2. TargetPPO = LastPage,m_TargetOffsetBitMap == 0,m_RPMTFlushBitMap == 0;
3. TargetPPO = LastPage,m_TargetOffsetBitMap == 0,m_RPMTFlushBitMap != 0;*/
BOOL MCU1_DRAM_TEXT L2_IsNewData(U8 ucSuperPU, PhysicalAddr TempAddr, ADJUST_BLOCK_STATUS BlockStatus)
{    

    if(SEQ_AND_TARGET_SAME ==  BlockStatus)
    {
        if(TempAddr.m_BlockInPU != g_PuInfo[ucSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] || l_CalcDirtyCntDptr->m_TargetBlk[ucSuperPU][TARGET_HOST_WRITE_NORAML]!= g_PuInfo[ucSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
        {
            DBG_Printf("FUN %s SPU %d SEQ_AND_TARGET_SAME check err\n\n", __FUNCTION__, ucSuperPU);
            DBG_Getch();
        }
        /*
            g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] represent ppo after respond write during rebuild,
            l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML] represent ppo before respond write after rebuild puInfo,
            In case of SEQ_AND_TARGET_SAME (0 == l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML]) is reasonable. 
        */
        if (0 == g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
        {
            DBG_Printf("SPU %d Err case 1\n", ucSuperPU);
            DBG_Getch();
        }

        if(l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML] == g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
        {
            if (TempAddr.m_PageInBlock != l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML] - 1)
            {
                DBG_Printf("SPU %d Err case 2\n", ucSuperPU);
                DBG_Getch();
            }
            
            if (FALSE == L2_IsSubPPOLUNWritten(l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ucSuperPU][TARGET_HOST_WRITE_NORAML], TempAddr.m_OffsetInSuperPage) &&
                TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ucSuperPU]->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML], TempAddr.m_OffsetInSuperPage))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            if (TempAddr.m_PageInBlock == (l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML] - 1) &&
                FALSE == L2_IsSubPPOLUNWritten(l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ucSuperPU][TARGET_HOST_WRITE_NORAML], TempAddr.m_OffsetInSuperPage))
            {
                return TRUE;
            }

            if (TempAddr.m_PageInBlock >= l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML] &&
                TempAddr.m_PageInBlock < (U32)(g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1))
            {
                return TRUE;
            }

            if (TempAddr.m_PageInBlock == g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1 &&
                TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ucSuperPU]->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML], TempAddr.m_OffsetInSuperPage))
            {
                return TRUE;
            }

            return FALSE;
        }
    }
    else if (SEQ_NOT_TARGET == BlockStatus)
    {
        if (TempAddr.m_BlockInPU != l_CalcDirtyCntDptr->m_TargetBlk[ucSuperPU][TARGET_HOST_WRITE_NORAML] || l_CalcDirtyCntDptr->m_TargetBlk[ucSuperPU][TARGET_HOST_WRITE_NORAML] == g_PuInfo[ucSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
        {
            DBG_Printf("FUN %s SPU %d SEQ_AND_TARGET_SAME check err\n\n", __FUNCTION__, ucSuperPU);
            DBG_Getch();
        }

        if (0 == l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML])
        {
            return TRUE;
        }

        if (TempAddr.m_PageInBlock == (l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML] - 1) &&
            FALSE == L2_IsSubPPOLUNWritten(l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ucSuperPU][TARGET_HOST_WRITE_NORAML], TempAddr.m_OffsetInSuperPage))
        {
            return TRUE;
        }

        if (TempAddr.m_PageInBlock >= l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML])
        {
            return TRUE;
        }

        return FALSE;
    }
    else  if (SEQ_AND_TARGET_DIFF == BlockStatus)
    {
        if (TempAddr.m_BlockInPU != g_PuInfo[ucSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML] || l_CalcDirtyCntDptr->m_TargetBlk[ucSuperPU][TARGET_HOST_WRITE_NORAML] == g_PuInfo[ucSuperPU]->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
        {
            DBG_Printf("FUN %s SPU %d SEQ_AND_TARGET_SAME check err\n\n", __FUNCTION__, ucSuperPU);
            DBG_Getch();
        }

        if (0 == g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML])
        {
            return FALSE;
        }

        if (TempAddr.m_PageInBlock < (U32)(g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1))
        {
            return TRUE;
        }

        if (TempAddr.m_PageInBlock == g_PuInfo[ucSuperPU]->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - 1 &&
            TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ucSuperPU]->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML], TempAddr.m_OffsetInSuperPage))
        {
            return TRUE;
        }

        return FALSE;
    }
    else
    {
        DBG_Printf("SPU %d unknown BlockStatus \n", ucSuperPU);
        DBG_Getch();

        return FALSE;
    }

 }
 
 void SortByLunOrderTS(U32* pLunOrderTS,U8* pWriteLunOrder)
 {
     U32 i;
     U32 TS = 0;

     for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
     {
         for (TS = 0; TS < LUN_NUM_PER_SUPERPU; TS++)
         {
             if (pLunOrderTS[i] == TS)
             {
                 pWriteLunOrder[TS] = i;
                 break;
             }
         }
     }

     return;
 }

void MCU1_DRAM_TEXT L2_AdjustDirtyCntOfOneBlk(U8 ucSuperPU, U32 Blk)
{
    PhysicalAddr TempAddr = { 0 }, RefAddr = { 0 };
    S32 Page;
    U32 LpnInPage;
    U32 ulLpn;
    RPMT* pRPMT;
    U32 LunOrderTS[LUN_NUM_PER_SUPERPU] = { 0 };
    U8 ucWriteLunOrder[LUN_NUM_PER_SUPERPU] = { INVALID_2F };
    U32 ulTargetOffsetTS;

    LPNLookUPQuitCode LookUpStatus;
    ADJUST_BLOCK_STATUS BlockStatus;
    S32 ulAdjustPPOStart;
    S32 ulAdjustPPOEnd;
    U32 ucLUNInSuperPU;
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPU];
    U16 usRecordPPO;
    U32 ulTargetOffsetBitMap;

    BlockStatus = l_CalcDirtyCntDptr->m_CurrentAdjustDirtyBlkStatus[ucSuperPU];
    ulTargetOffsetBitMap= l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ucSuperPU][TARGET_HOST_WRITE_NORAML];
    switch (BlockStatus)
    {
    case SEQ_NOT_TARGET:
        usRecordPPO = l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML];
        ulAdjustPPOStart = (0 == ulTargetOffsetBitMap) ? usRecordPPO : (usRecordPPO - 1);
        ulAdjustPPOEnd = PG_PER_SLC_BLK - 1;
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0];
        break;

    case SEQ_AND_TARGET_SAME:
        usRecordPPO = l_CalcDirtyCntDptr->m_TargetPPO[ucSuperPU][TARGET_HOST_WRITE_NORAML];
        ulAdjustPPOStart = (0 == ulTargetOffsetBitMap) ? usRecordPPO : (usRecordPPO - 1); 
        ulAdjustPPOEnd = min(pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML], PG_PER_SLC_BLK - 1);
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0];
        break;

    case SEQ_AND_TARGET_DIFF:
        ulAdjustPPOStart = 0;
        ulAdjustPPOEnd = min(pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML], PG_PER_SLC_BLK - 1);
        pRPMT = pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][1];
        break;

    default:
        DBG_Printf("L2 adjust dirtycnt BlockStatus %d error!\n", BlockStatus);
        DBG_Getch();
        break;
    }

    //FIRMWARE_LogInfo("SPU %d AdjustDirtyCnt Blk %d BlockStatus %d AdjustPPOStart %d AdjustPPOEnd %d ulTargetOffsetBitMap 0x%x m_RPMTFlushBitMap 0x%x\n", ucSuperPU, Blk,
    //    BlockStatus, ulAdjustPPOStart, ulAdjustPPOEnd, ulTargetOffsetBitMap, l_CalcDirtyCntDptr->m_RPMTFlushBitMap[ucSuperPU][TARGET_HOST_WRITE_NORAML]);

    if (ulAdjustPPOStart >= ulAdjustPPOEnd && (PG_PER_SLC_BLK - 1) != ulAdjustPPOStart)
    {
#ifdef DirtyLPNCnt_IN_DSRAM1
        DBG_Printf("%s SPU %d Blk %d BlockStatus %d AdjustPPOStart %d AdjustPPOEnd %d DirtyCnt %d\n", __FUNCTION__, ucSuperPU, Blk, BlockStatus, ulAdjustPPOStart, ulAdjustPPOEnd, *(g_pDirtyLPNCnt + ucSuperPU*VIR_BLK_CNT + Blk));
#else
        DBG_Printf("%s SPU %d Blk %d BlockStatus %d AdjustPPOStart %d AdjustPPOEnd %d DirtyCnt %d\n", __FUNCTION__, ucSuperPU, Blk, BlockStatus, ulAdjustPPOStart, ulAdjustPPOEnd, pVBT[ucSuperPU]->m_VBT[Blk].DirtyLPNCnt);
#endif
        DBG_Getch();
    }

    TempAddr.m_PPN = 0;
    TempAddr.m_PUSer = ucSuperPU;
    TempAddr.m_BlockInPU = Blk;
    for (Page = ulAdjustPPOStart; Page < ulAdjustPPOEnd; Page++)
    {
        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            /* Just check RPMT once */
            if (Page == ulAdjustPPOStart)
            {
                if (pRPMT->m_RPMT[ucLUNInSuperPU].m_SuperPU != ucSuperPU || pRPMT->m_RPMT[ucLUNInSuperPU].m_SuperBlock != Blk)
                {
                    DBG_Printf("Load SPU %d LUN %d Blk %d RPMT error!!!\n", ucSuperPU, ucLUNInSuperPU, Blk);
                    DBG_Printf("RPMT SPU %d Blk %d\n", pRPMT->m_RPMT[ucLUNInSuperPU].m_SuperPU, pRPMT->m_RPMT[ucLUNInSuperPU].m_SuperBlock);
                    DBG_Getch();
                }
            }
            LunOrderTS[ucLUNInSuperPU] = pRPMT->m_RPMT[ucLUNInSuperPU].m_LunOrderTS[Page];
            ucWriteLunOrder[ucLUNInSuperPU] = INVALID_2F;
        }

        SortByLunOrderTS(LunOrderTS, ucWriteLunOrder);

        for (ulTargetOffsetTS = 0; ulTargetOffsetTS < LUN_NUM_PER_SUPERPU; ulTargetOffsetTS++)
        {
            ucLUNInSuperPU = ucWriteLunOrder[ulTargetOffsetTS];
            if (ucLUNInSuperPU == INVALID_2F)
            {
                continue;
            }
            
            TempAddr.m_OffsetInSuperPage = ucLUNInSuperPU;
            TempAddr.m_PageInBlock = Page;

            /*skip Target PPO-1 page's not write LUN*/
#if (LUN_NUM_PER_SUPERPU > 1)
            if (L2_IsNewData(ucSuperPU, TempAddr, BlockStatus))
#endif
            {
                for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                {
                    TempAddr.m_LPNInPage = LpnInPage;

                    ulLpn = L2_LookupRPMT(pRPMT, &TempAddr);
                    if (INVALID_8F == ulLpn)
                    {
                        L2_IncreaseDirty(ucSuperPU, Blk, 1);
                        if (TRUE == L2_LookupDirtyLpnMap(&TempAddr))
                        {
                            /*for safe, after call L2_InitDirtyLpnMap during rebuild, should not enter this branch*/
                            L2_UpdateDirtyLpnMap(&TempAddr, B_DIRTY);
                        }
                        continue;
                    }

                    /* Dirty original PMT */
                    LookUpStatus = L2_LookupPMT(&RefAddr, ulLpn,TRUE);
                    if (LPN_LOOKUP_FAIL == LookUpStatus)
                    {
                        DBG_Printf("SuperPU %d, block %d, BlockStatus = 0x%x.\n", ucSuperPU, Blk, BlockStatus);
                        DBG_Printf("page = %d, LpnInPage = %d. \n", Page, LpnInPage);
                        DBG_Getch();
                    }

                    if (INVALID_8F != RefAddr.m_PPN)
                    {
                        L2_IncreaseDirty(RefAddr.m_PUSer, RefAddr.m_BlockInPU, 1);
                        L2_UpdateDirtyLpnMap(&RefAddr, B_DIRTY);
                    }

                    /* update PMT */
                    L2_UpdatePMT(&TempAddr, NULL, ulLpn);

                    //FIRMWARE_LogInfo("AdjustDC LPN 0x%x -> Addr 0x%x(%d_0x%x_0x%x_%d_%d) LunOrderTS %d\n", ulLpn, TempAddr.m_PPN, TempAddr.m_PUSer, TempAddr.m_BlockInPU,
                    //    TempAddr.m_PageInBlock, TempAddr.m_LPNInPage, TempAddr.m_OffsetInSuperPage, LunOrderTS[ucLUNInSuperPU]);

                    L2_UpdateDirtyLpnMap(&TempAddr, B_VALID);
                }
            }
        }
    }

#ifdef SIM
    if (SEQ_NOT_TARGET == BlockStatus)
    {
        L2_CheckDirtyCnt((U8)ucSuperPU, Blk);
    }
    else
    {
        PhysicalAddr Addr = { 0 };
        U32 ValidCnt = 0;
        U32 DirtyCnt;
        U32 usLPNSum = 0;
        S16 usEndCheckPG;
        U16 usPPO;

        usPPO = min(pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML], (PG_PER_SLC_BLK - 1));
        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            if (0 == pInfo->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML])
            {
                usEndCheckPG = usPPO - 1;
            }
            else
            {
                usEndCheckPG = (0 != (pInfo->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML] & (1 << ucLUNInSuperPU))) ? (usPPO - 1) : (usPPO - 2);
            }

            for (Page = 0; Page <= usEndCheckPG; Page++)
            {
                Addr.m_PUSer = ucSuperPU;
                Addr.m_OffsetInSuperPage = ucLUNInSuperPU;
                Addr.m_BlockInPU = Blk;
                Addr.m_PageInBlock = Page;
                for (LpnInPage = 0; LpnInPage < LPN_PER_BUF; LpnInPage++)
                {
                    Addr.m_LPNInPage = LpnInPage;
                    if (TRUE == L2_LookupDirtyLpnMap(&Addr))
                    {
                        ValidCnt++;
                    }
                }
            }
            usLPNSum += (usEndCheckPG + 1) * LPN_PER_BUF;
        }

        DirtyCnt = L2_GetDirtyCnt(ucSuperPU, Blk);
        if (usLPNSum != (DirtyCnt + ValidCnt))
        {
            DBG_Printf("SuperPU %d, block %d dirtycnt %d ValidCnt %d LPNSum %d\n", ucSuperPU, Blk, DirtyCnt, ValidCnt, usLPNSum);
            DBG_Getch();
        }
    }
#endif

    return;
}

BOOL L2_RebuildIsTargetBlk(U32 ulSuperPU, U32 Blk, U8* pucTargetType)
{
    U8 ucTargetType;

    for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
    {
        if (Blk == l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU][ucTargetType])
        {
            //L2_PrintRebuildDirtyCntBlk(ulSuperPU, Blk, ucTargetType, TRUE);

            *pucTargetType = ucTargetType;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL L2_IsTargetBlk(U32 ulSuperPU, U32 Blk, U8* pucTargetType)
{
    U8 ucTargetType;

    for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
    {
        if (Blk == g_PuInfo[ulSuperPU]->m_TargetBlk[ucTargetType])
        {
            //L2_PrintRebuildDirtyCntBlk(ulSuperPU, Blk, ucTargetType, TRUE);

            *pucTargetType = ucTargetType;
            return TRUE;
        }
    }

    return FALSE;
}

void MCU1_DRAM_TEXT L2_RebuildReadTargetBlkRPMT(U32 ulSuperPU, U32* pBuffer, U8 ucTargetType)
{
    PuInfo* pInfo;
    RPMT* pRPMT;
    RPMT* pRPMTBuffer;
    U32 ulPgIndex;
    U32 ulPagePerBlk;
    U8 ucLUNInSuperPU;
    U32 ulPPO;

    pInfo = g_PuInfo[ulSuperPU];
    l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU] = TRUE;

    /* copy RPMT buffer */
    pRPMT = pInfo->m_pRPMT[ucTargetType][0];
    pRPMTBuffer = (RPMT*)pBuffer;
    COM_MemCpy((U32*)pRPMTBuffer, (U32*)pRPMT, sizeof(RPMT) / sizeof(U32));

    //Init    
    ulPagePerBlk = PG_PER_SLC_BLK;    

    /*for PPO-1*/
    ulPPO = l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU][ucTargetType];
    if (0 != l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType])
    {
        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            if (0 == (l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU][ucTargetType] & (1<<ucLUNInSuperPU)))
            {
                 COM_MemSet((U32*)&pRPMTBuffer->m_RPMT[ucLUNInSuperPU].m_RPMTItems[(ulPPO -1)*LPN_PER_BUF], LPN_PER_BUF, INVALID_8F);
            }
        }
    }

    /*for other pages*/
    for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
    {
        for (ulPgIndex = ulPPO; ulPgIndex < ulPagePerBlk; ulPgIndex++)
        {
            COM_MemSet((U32*)&pRPMTBuffer->m_RPMT[ucLUNInSuperPU].m_RPMTItems[ulPgIndex*LPN_PER_BUF], LPN_PER_BUF, INVALID_8F);
        }
    }

    return;
}


//Return TRUE: Finish
//Return FALSE: Not Finish
CALC_DIRTY_COUNT_STATUS MCU1_DRAM_TEXT L2_RebuildCalcDirtyCntOfAllBlk(U32 ulSuperPU)
{
    RebuildReadRPMTState ReadRPMTStateState = l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU];
    U8* pStatus;
    U32* pBuffer = (U32*)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][0];
    BOOL ret = CALC_ONE_BLOCK_UNDONE;
    U32 CurrentCalcDirtyBlkSN;
    U32 VirBlk;
    U8 ucTargetType;
    BOOL bTLCBlk;
    U32 ucLunInSuperPu;
    U32 ucTLun;
    U32 uLoadRPMTBitMap;
    U32 ulPage = 0;
    U32 ulPageCnt = 0;

    if (l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU] == 0)
    {
        return CALC_ALL_BLOCK_DONE;
    }

    CurrentCalcDirtyBlkSN = l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSN[ulSuperPU]
        + l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSNInRound[ulSuperPU];
    VirBlk = l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU][CurrentCalcDirtyBlkSN];

    switch (ReadRPMTStateState)
    {
    case Rebuild_ReadRPMT:
    {
        //Is Target Blk
        if (L2_RebuildIsTargetBlk(ulSuperPU, VirBlk, &ucTargetType))
        {
            if (0 == l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU][ucTargetType])
            {
                L2_SetDirtyCntInVBT(ulSuperPU, VirBlk, 0);
                L2_ResetLpnBitMapInDPBM(ulSuperPU, VirBlk);
                l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_Calc_One_Blk_Done;
            }
            else if (ucTargetType != TARGET_TLC_WRITE)
            {
                L2_RebuildReadTargetBlkRPMT(ulSuperPU, pBuffer, ucTargetType);
                l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_Done;
            }
            else
            {
                DBG_Printf("Wrong ucTargetType in L2_RebuildCalcDirtyCntOfAllBlk()\n");
                DBG_Getch();
            }

            break;
        }
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucTLun = L2_GET_TLUN(ulSuperPU, ucLunInSuperPu);

            //Check Resource
            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                continue;
            }

            uLoadRPMTBitMap = l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU];
            if (0 != (uLoadRPMTBitMap & (1 << ucLunInSuperPu)))
            {
                continue;
            }

            //Read RPMT
            bTLCBlk = L2_VBT_Get_TLC(ulSuperPU, VirBlk);

            //TLC Blk process specially
            if (bTLCBlk)
            {
                U32 uTLCCurWordLineOffset;
                RPMT* pTLCRPMT;

                uTLCCurWordLineOffset = l_BufferManagerDptr->m_TLCCurWordLineOffset[ulSuperPU][0];
                //Get TLC buffer

                pTLCRPMT = (RPMT*)l_BufferManagerDptr->m_TLCPageBuffer[ulSuperPU][uTLCCurWordLineOffset];
                pStatus = (U8*)&l_BufferManagerDptr->m_TLCPageBufferStatus[ulSuperPU][uTLCCurWordLineOffset][ucLunInSuperPu];

                L2_LoadRPMTInLun(pTLCRPMT, ulSuperPU, ucLunInSuperPu, VirBlk, (U8*)pStatus, FALSE, uTLCCurWordLineOffset);

                uLoadRPMTBitMap |= 1 << ucLunInSuperPu;
                if (SUPERPU_LUN_NUM_BITMSK == uLoadRPMTBitMap)
                {
                    uLoadRPMTBitMap = 0;
                    uTLCCurWordLineOffset++;
                    l_BufferManagerDptr->m_TLCCurWordLineOffset[ulSuperPU][0] = uTLCCurWordLineOffset;
                }
                l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU] = uLoadRPMTBitMap;

                //if TLC Word Line Finish
                if (uTLCCurWordLineOffset >= PG_PER_WL)
                {
                    //clear TLCWordLineOffset
                    l_BufferManagerDptr->m_TLCCurWordLineOffset[ulSuperPU][0] = 0;

                    l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_WaitStatus;
                    goto out;
                }
            }
            else
            {
                pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][0][ucLunInSuperPu];
                L2_LoadRPMTInLun((RPMT*)pBuffer, ulSuperPU, ucLunInSuperPu, VirBlk, (U8*)pStatus, TRUE, 0);
                uLoadRPMTBitMap |= 1 << ucLunInSuperPu;
                if (SUPERPU_LUN_NUM_BITMSK == uLoadRPMTBitMap)
                {
                    uLoadRPMTBitMap = 0;
                    l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU] = uLoadRPMTBitMap;
                    l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_WaitStatus;
                    goto out;
                }
                l_BufferManagerDptr->m_LoadRPMTBitMap[ulSuperPU] = uLoadRPMTBitMap;
            }
        }
    }
        break;
    case Rebuild_ReadRPMT_WaitStatus:
    {
        RPMT* pRPMT;
        U32 uTLCCurWordLineOffset;

        //Process TLC Write Target specially
        if (L2_VBT_Get_TLC(ulSuperPU, VirBlk))
        {
            //Wait TLC RPMT not Load Finish 
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (FALSE == L2_IsTLCRPMTLoadFinish(ulSuperPU, ucLunInSuperPu))
                {
                    goto out;
                }
            }

            //RPMT err handle 
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (TRUE == L2_IsTLCRPMTLoadErr(ulSuperPU, ucLunInSuperPu))
                {
                    //Err handling
                    DBG_Printf("CalcDirtyCnt Load TLC %d_%d_%d RPMT error \n", ulSuperPU, VirBlk, ucLunInSuperPu);
                    l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU] = FALSE;
                    l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_ErrHandle;
                    goto out;
                }
            }

            //RPMT succeed
            l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU] = TRUE;
            l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_Done;

#ifdef DCACHE
            HAL_InvalidateDCache();
#endif
            for (uTLCCurWordLineOffset = 0; uTLCCurWordLineOffset < PG_PER_WL; uTLCCurWordLineOffset++)
            {
                pRPMT = (RPMT*)l_BufferManagerDptr->m_TLCPageBuffer[ulSuperPU][uTLCCurWordLineOffset];
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    if (pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU != ulSuperPU || pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock != VirBlk)
                    {
                        DBG_Printf("Load SPU %d LUN %d Blk %d RPMT error!!!\n", ulSuperPU, ucLunInSuperPu, VirBlk);
                        DBG_Printf("RPMT SPU %d Blk %d\n", pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU, pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock);
                        DBG_Getch();
                    }
                }
            }

            break;
        }

        //Wait Status
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][0][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                goto out;
            }
        }
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][0][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_FAIL == *pStatus)
            {
                DBG_Printf("FUN:L2_RebuildCalcDirtyCntOfAllBlk read RPMT page FAILE: PU %d Blk %d  LunInSuperPu %d\n",
                    ulSuperPU, VirBlk, ucLunInSuperPu);
                l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU] = FALSE;
                l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_ErrHandle;
                goto out;
            }
        }
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pStatus = &l_BufferManagerDptr->m_BufferStatus[ulSuperPU][0][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_EMPTY_PG == *pStatus)
            {
                DBG_Printf("FUN:L2_RebuildCalcDirtyCntOfAllBlk read RPMT page EMPTY: PU %d Blk %d  LunInSuperPu%d (PhyBlk 0x%x)\n",
                    ulSuperPU, VirBlk, ucLunInSuperPu, pVBT[ulSuperPU]->m_VBT[VirBlk].PhysicalBlockAddr[0]);
                DBG_Getch();
                goto out;
            }
        }
        l_CalcDirtyCntDptr->m_UseRPMTFlag[ulSuperPU] = TRUE;
        l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_Done;

#ifdef DCACHE
        HAL_InvalidateDCache();
#endif
        pRPMT = (RPMT*)l_BufferManagerDptr->m_PageBuffer[ulSuperPU][0];
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU != ulSuperPU || pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock != VirBlk)
            {
                DBG_Printf("Load SPU %d LUN %d Blk %d RPMT error!!!\n", ulSuperPU, ucLunInSuperPu, VirBlk);
                DBG_Printf("RPMT SPU %d Blk %d\n", pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU, pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock);
                DBG_Getch();
            }
        }
    }
        break;
    case Rebuild_ReadRPMT_ErrHandle:
        TRACE_LOG((void*)&ulSuperPU, sizeof(U32), U32, 0, "[L2_RebuildCalcDirtyCntOfAllBlk]: PUSer ? ");
        TRACE_LOG((void*)&VirBlk, sizeof(U32), U32, 0, "[L2_RebuildCalcDirtyCntOfAllBlk]: Block ? Rebuild_ReadRPMT_ErrHandle! ");

        if (L2_RebuildReadRPMTErrHandle(ulSuperPU, VirBlk))
        {
            l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_Done;
        }
        break;
    case Rebuild_ReadRPMT_Done:
        ulPage = l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU];
        ulPageCnt = 2;
        if (L2_VBT_Get_TLC(ulSuperPU, VirBlk))
        {
            while ((ulPage + ulPageCnt) > ((PG_PER_SLC_BLK - 1) * PG_PER_WL))
            {
                ulPageCnt -= 1;
            }
            L2_RebuildCalcDirtyCntOfOneTLCBlk(ulSuperPU, VirBlk, ulPage, ulPageCnt);
            l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU] = ulPage + ulPageCnt;
            if ((ulPage + ulPageCnt) == ((PG_PER_SLC_BLK - 1) * PG_PER_WL))
            {
                //FIRMWARE_LogInfo("\t RebuildDC TLC VirBlk %d done\n", VirBlk);
                l_CalcDirtyCntDptr->m_CalcDirtyTLCBlkCnt[ulSuperPU]++;
                l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU] = 0;
                l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_Calc_One_Blk_Done;
            }
        }
        else
        {
            L2_RebuildCalcDirtyCntOfOneBlk(ulSuperPU, VirBlk, ulPage, ulPageCnt);
            l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU] = ulPage + ulPageCnt;
            if ((ulPage + ulPageCnt) == PG_PER_SLC_BLK)
            {
                //FIRMWARE_LogInfo("\t RebuildDC SLC VirBlk %d done\n", VirBlk);
                l_CalcDirtyCntDptr->m_CurrentCalcDirtyPage[ulSuperPU] = 0;
                l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_Calc_One_Blk_Done;
            }	
        }
        //l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_Calc_One_Blk_Done;
        break;

    case Rebuild_Calc_One_Blk_Done:
        l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT;
        l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSNInRound[ulSuperPU]++;
        CurrentCalcDirtyBlkSN++;

        if (0 == ulSuperPU)
        {
            g_RebuildDCPercent = (1000 * CurrentCalcDirtyBlkSN) / l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU];
        }

        ret = CALC_ONE_BLOCK_DONE;
        if (CurrentCalcDirtyBlkSN >= l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU])
        {
            DBG_Printf("#### SPU %d CalcDirtyCnt SLCBlkCnt %d TLCBlkCnt %d ####\n", ulSuperPU, l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU] - l_CalcDirtyCntDptr->m_CalcDirtyTLCBlkCnt[ulSuperPU],
                l_CalcDirtyCntDptr->m_CalcDirtyTLCBlkCnt[ulSuperPU]);

            ret = CALC_ALL_BLOCK_DONE;
            return ret;
        }

        if (l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSNInRound[ulSuperPU] >= l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCntInRound)
        {
            ret = CALC_ONE_ROUND_DONE;
            l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSNInRound[ulSuperPU] = 0;
            l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSN[ulSuperPU] = CurrentCalcDirtyBlkSN;
        }

        break;
    default:
        break;
    }
out:
    return ret;
}

/*****************************************************************************
 Prototype      : L2_AdjustCalcDirtyCntOfAllBlk
 Description    : mostly two seq block need to adjust per PU.
 Input          : U32 PUSer
 Output         : void
 Return Value   : BOOL
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/01/06
 Author       : henryluo
 Modification : Created function in TaiPei

 *****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_AdjustCalcDirtyCntOfAllBlk(U32 ulSuperPU)
{
    RebuildReadRPMTState ReadRPMTStateState = l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU];
    U32 CurrentCalcDirtyBlkSN;
    U32 VirBlk;

    if (l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU] == 0)
    {
        DBG_Printf("L2_AdjustCalcDirtyCntOfAllBlk Error!\n");
        DBG_Getch();
    }

    CurrentCalcDirtyBlkSN = l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSN[ulSuperPU];
    VirBlk = l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU][CurrentCalcDirtyBlkSN];

    switch (ReadRPMTStateState)
    {
    case Rebuild_ReadRPMT:
    {
        PuInfo* pInfo;

        pInfo = g_PuInfo[ulSuperPU];

        /* adjust block is seq block */
        if ((VirBlk == l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU][TARGET_HOST_WRITE_NORAML]) || (VirBlk == pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML]))
        {
            /* original SeqBlock has been fully writen */
            if (VirBlk == l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU][TARGET_HOST_WRITE_NORAML])
            {
                if (pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML] != l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU][TARGET_HOST_WRITE_NORAML])
                {
#ifdef SIM
                    if (1 != pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML])
                    {
                        DBG_Getch();
                    }
#endif

                    l_CalcDirtyCntDptr->m_CurrentAdjustDirtyBlkStatus[ulSuperPU] = SEQ_NOT_TARGET;
                }
                else
                {
#ifdef SIM
                    if (0 != pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML])
                    {
                        DBG_Getch();
                    }
#endif

                    l_CalcDirtyCntDptr->m_CurrentAdjustDirtyBlkStatus[ulSuperPU] = SEQ_AND_TARGET_SAME;
                }
            }
            else
            {
#ifdef SIM
                if (1 != pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML])
                {
                    DBG_Getch();
                }
#endif

                l_CalcDirtyCntDptr->m_CurrentAdjustDirtyBlkStatus[ulSuperPU] = SEQ_AND_TARGET_DIFF;
            }
        }
        else
        {
            DBG_Printf("SPU %d adjust dirty count block sn 0x%x error !", ulSuperPU, VirBlk);
            DBG_Getch();
        }

        l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT_Done;
    }
        break;

    case Rebuild_ReadRPMT_Done:
        L2_AdjustDirtyCntOfOneBlk(ulSuperPU, VirBlk);
        CurrentCalcDirtyBlkSN++;
        l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSN[ulSuperPU] = CurrentCalcDirtyBlkSN;
        l_CalcDirtyCntDptr->m_ReadRPMTState[ulSuperPU] = Rebuild_ReadRPMT;
        if (CurrentCalcDirtyBlkSN >= l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU])
        {
            return TRUE;
        }

        break;
    default:
        break;
    }

    return FALSE;
}

void L2_CalcDC_ReuseTLCPage_Spare_Buffer()
{  
    U32 i, j;
    U32 ulFreeDramBase_Reuse = NULL; 
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        ulFreeDramBase_Reuse = (U32)(l_BufferManagerDptr->m_TLCPageBuffer[i][0]);
        L2_RebuildSetDRAMForPMTBuffer(i, 0, &ulFreeDramBase_Reuse);
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase_Reuse);
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        l_BufferManagerDptr->m_PageBuffer[i][1] = NULL;
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pPBIT_Temp[i] = NULL;
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < PG_PER_SLC_BLK; j++)
        {
            ulFreeDramBase_Reuse = (U32)(l_BufferManagerDptr->m_TLCSpareBuffer[i][j][0][0]);

            if (ulFreeDramBase_Reuse > DRAM_HIGH_START_ADDRESS)
            {
                ulFreeDramBase_Reuse -= DRAM_HIGH_ADDR_OFFSET;
            }
            L2_RebuildSetDRAMForSpareBuffer(i, j, &ulFreeDramBase_Reuse);
        }
    }
#ifdef PMT_ITEM_SIZE_REDUCE
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        HAL_DMAESetValue((U32)l_CalcDirtyCntDptr->m_bValidBitMap[i], COM_MemSize16DWAlign(PMTPAGE_CNT_PER_PU * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[0][0])), 0x0);
    }
#else    
    HAL_DMAESetValue((U32)l_CalcDirtyCntDptr->m_bValidBitMap, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[0])), 0x0);
#endif
}

//Return True: Finish
//Return False: Not Finish
CALC_DIRTY_COUNT_STATUS MCU1_DRAM_TEXT L2_RebuildDirtyCnt(U32 uSuperPu)  
{
    RebuildCalcDirtyCntState CalcDirtyCntState;
    CALC_DIRTY_COUNT_STATUS Status = CALC_ONE_BLOCK_UNDONE;

    CalcDirtyCntState = L2_RebuildGetCalcDirtyCntState(uSuperPu);
    switch (CalcDirtyCntState)
    {
    case Rebuild_CalcDirtyCnt_Prepare:
        //Init all CalcDirtyCnt Structure
        L2_CalcDC_ReuseTLCPage_Spare_Buffer();
        L2_PrepareCaldDirtyCnt(uSuperPu);
        L2_RebuildSetCalcDirtyCntState(uSuperPu, Rebuild_CalcDirtyCnt_Process);
        break;

    case Rebuild_CalcDirtyCnt_Process:
        Status = L2_RebuildCalcDirtyCntOfAllBlk(uSuperPu);
        if (CALC_ALL_BLOCK_DONE == Status)
        {
            //Rebuild Trim
            L2_ProcessTrimedLPN(uSuperPu);
            L2_RebuildSetCalcDirtyCntState(uSuperPu, Rebuild_CalcDirtyCnt_Prepare);
            //DBG_Printf("#######CalcDirtyCnt of PU %d Rebuild %d Blk\n", PUSer,l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[PUSer]);

#ifdef ASIC
            TRACE_LOG((void*)&uSuperPu, sizeof(U32), U32, 0, "[L2_RebuildDirtyCnt]: #######CalcDirtyCnt of PU ");
            TRACE_LOG((void*)&l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[uSuperPu], sizeof(U32), U32, 0, "[L2_RebuildDirtyCnt]: rebuild block count ");
#endif
        }
        break;

    case Rebuild_CalcDirtyCnt_Done:
        Status = CALC_ALL_BLOCK_DONE;
        break;
    default:
        break;
    }

    return Status;
}

/*****************************************************************************
 Prototype      : L2_AdjustDirtyCnt
 Description    : mostly two seq block need to adjust per PU.
 Input          : U32 PUSer
 Output         : void
 Return Value   : BOOL
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/01/06
 Author       : henryluo
 Modification : Created function in TaiPei

 *****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_AdjustDirtyCnt(U32 ulSuperPU )
{
    RebuildCalcDirtyCntState CalcDirtyCntState;
    BOOL Status = FALSE;
    PuInfo* pInfo;
    U32 NeedCalcDirtyCntBlkCnt = 0;
    U32 ulActualHostWriteCnt = 0;
    U32 ulTheoryHostWriteCnt = 0;

    CalcDirtyCntState = L2_RebuildGetCalcDirtyCntState(ulSuperPU );
    switch (CalcDirtyCntState)
    {
    case Rebuild_CalcDirtyCnt_Prepare:

        pInfo = g_PuInfo[ulSuperPU ];
        if (pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML] == l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU ][TARGET_HOST_WRITE_NORAML])
        {
            if (pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML] != 0)
            {
                DBG_Printf("[L2_AdjustDirtyCnt]: Seq RPMT pointer (!= 0) error!!!\n");
                DBG_Getch();
            }

            if (pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] != l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU ][TARGET_HOST_WRITE_NORAML] || 
                pInfo->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML] != l_CalcDirtyCntDptr->m_TargetOffsetBitMap[ulSuperPU ][TARGET_HOST_WRITE_NORAML])
            {
                l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU ][NeedCalcDirtyCntBlkCnt] = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                NeedCalcDirtyCntBlkCnt++;
                ulTheoryHostWriteCnt += (pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] - l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU ][TARGET_HOST_WRITE_NORAML]);
            }            
        }
        else
        {
            if (pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML] != 1)
            {
                DBG_Printf("[L2_AdjustDirtyCnt]: Seq RPMT pointer (!= 1) error!!!\n");
                DBG_Getch();
            }

            l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU ][NeedCalcDirtyCntBlkCnt] = l_CalcDirtyCntDptr->m_TargetBlk[ulSuperPU ][TARGET_HOST_WRITE_NORAML];
            NeedCalcDirtyCntBlkCnt++;
            ulTheoryHostWriteCnt += (PG_PER_SLC_BLK - l_CalcDirtyCntDptr->m_TargetPPO[ulSuperPU][TARGET_HOST_WRITE_NORAML]);

            l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU ][NeedCalcDirtyCntBlkCnt] = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
            NeedCalcDirtyCntBlkCnt++;
            ulTheoryHostWriteCnt += pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
        }

        //DBG_Printf("Host write %d pages in PU %d during calculate dirty count. \n", ulActualHostWriteCnt, PUSer);
        l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU ] = NeedCalcDirtyCntBlkCnt;
        l_CalcDirtyCntDptr->m_CurrentCalcDirtyBlkSN[ulSuperPU ] = 0;
        if (l_CalcDirtyCntDptr->m_NeedCalcDirtyBlkCnt[ulSuperPU ] > 0)
        {
            U32 i;

            DBG_Printf("SuperPU %d need adjust %d block: ", ulSuperPU, NeedCalcDirtyCntBlkCnt);
            for (i = 0; i < NeedCalcDirtyCntBlkCnt; i++)
            {
                DBG_Printf(" 0x%x", l_CalcDirtyCntDptr->m_NeedCalcDirtyBlk[ulSuperPU ][i]);
            }
            DBG_Printf("\n");

            L2_RebuildSetCalcDirtyCntState(ulSuperPU , Rebuild_CalcDirtyCnt_Process);
        }
        else
        {
            L2_RebuildSetCalcDirtyCntState(ulSuperPU , Rebuild_CalcDirtyCnt_Done);
            Status = TRUE;
        }

        break;

    case Rebuild_CalcDirtyCnt_Process:
        Status = L2_AdjustCalcDirtyCntOfAllBlk(ulSuperPU );
        if (TRUE == Status)
        {
            L2_RebuildSetCalcDirtyCntState(ulSuperPU , Rebuild_CalcDirtyCnt_Done);
            return TRUE;
        }
        break;

    case Rebuild_CalcDirtyCnt_Done:
        return TRUE;
        break;

    default:
        break;
    }
    return Status;
}

BOOL MCU1_DRAM_TEXT L2_IsPMTNeedRebuild(U32 ulSuperPU)
{
    U32 PMTIndexInPu;
    BOOL bExistDirtyPMT;
    U32 MaxTableTS;
    U32 MaxDataTS;

    for (PMTIndexInPu = 0; PMTIndexInPu < PMTPAGE_CNT_PER_PU; PMTIndexInPu++)
    {
        if (L2_IsRebuildPMTPageDirty(ulSuperPU, PMTIndexInPu))
        {
            l_RebuildPMTIDptr->m_bExistDirtyPMTPageInPU[ulSuperPU] = TRUE;
            break;
        }
    }

    bExistDirtyPMT = l_RebuildPMTIDptr->m_bExistDirtyPMTPageInPU[ulSuperPU];
    MaxTableTS = l_RebuildPMTIDptr->m_MaxTableTs[ulSuperPU];
    MaxDataTS = l_RebuildPMTDptr->m_MaxTSOfPU[ulSuperPU];

    if ((FALSE == bExistDirtyPMT) && (MaxTableTS > MaxDataTS))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL L2_IsBootupOK()
{
#ifndef L2_FAKE
    if (TRUE == g_BootUpOk)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    return TRUE;
#endif
}

BOOL L2_IsAdjustDirtyCntDone()
{
    if (TRUE == g_RebuildDirtyFlag)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}






