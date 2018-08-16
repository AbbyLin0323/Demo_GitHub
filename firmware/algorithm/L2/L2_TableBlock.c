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
  File Name     : L2_TableBlock.c
  Version       : Initial version
  Author        : henryluo
  Created       : 2015/02/28
  Description   : table block management
  Function List :
  History       :
  1.Date        : 2012/09/10
    Author      : AwayWei
    Modification: Created file

*******************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "L3_HostAdapter.h" 
#include "L2_Defines.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_StripeInfo.h"
#include "L2_TLCMerge.h"
#include "L2_GCManager.h"
#include "L2_LLF.h"
#include "L2_Boot.h"
#include "L2_RT.h"
#include "L2_FullRecovery.h"
#include "L2_TableBlock.h"
#include "L2_Interface.h"
#include "L2_Thread.h"
#include "COM_Memory.h"
#include "FW_BufAddr.h"
#include "L2_StaticWL.h"
#include "L2_FCMDQ.h"
#include "L2_TableBBT.h"
#include "COM_BitMask.h"
#include "L2_Erase.h"

#ifdef SIM
#include "sim_search_engine.h"
#include "L2_SimTablecheck.h"
#endif

#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif


GLOBAL  TB_MANAGEMENT *pTBManagement;
GLOBAL  TB_REBUILD_MANAGEMENT *pTBRebManagement;
GLOBAL  TB_PBIT_SEARCH *pPbitSearch;
GLOBAL  TB_RED_INFO *pTBRedInfo;
GLOBAL  TB_MOVEBLOCK_MANAGEMENT* pTBMoveBlockManagement;
GLOBAL  U32 gTBSaveBBTState;
GLOBAL  U32 g_ulTableBlockEnd[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
GLOBAL  U32 g_ulDataBlockStart[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
GLOBAL  U32 g_ulSLCBlockEnd[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
GLOBAL  U32 g_ulTLCBlockEnd[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
GLOBAL  BOOL bRTHasLoad[SUBSYSTEM_SUPERPU_MAX];

LOCAL  U32 gTBSaveBBTStatus;

extern GLOBAL  U16 L2AT0StartTempBufID;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;
#endif

extern void L2_FtlWriteLocal(PhysicalAddr* pAddr, U32* pPage, U32* pSpare, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, XOR_PARAM *pXorParam);
extern void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode);
extern void L2_FtlEraseBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, BOOL bNeedL3ErrorHandle);
extern void L2_EraseQueueShutdown(U8 ucSuperPuNum);

void MCU1_DRAM_TEXT L2_TB_RestoreEraseQueue(void)
{
    U8 ucSuperPuNum;
    U16 uVBN;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (uVBN = 0; uVBN < TLC_BLK_CNT; uVBN++)
        {
            if (pVBT[ucSuperPuNum]->m_VBT[uVBN].DirtyLPNCnt == (LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT))
            {
                if (TRUE != pVBT[ucSuperPuNum]->m_VBT[uVBN].bLockedInWL)
                {
                    L2_InsertBlkIntoEraseQueue(ucSuperPuNum, uVBN, FALSE);
                }
            }
        }

        for (uVBN = TLC_BLK_CNT; uVBN < VIR_BLK_CNT; uVBN++)
        {
            if (pVBT[ucSuperPuNum]->m_VBT[uVBN].DirtyLPNCnt == (LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT))
            {
                if (TRUE != pVBT[ucSuperPuNum]->m_VBT[uVBN].bLockedInWL)
                {
                    L2_InsertBlkIntoEraseQueue(ucSuperPuNum, uVBN, TRUE);
                }
            }
        }
    }
}

/****************************************************************************
Name        :L2_TableErrorHandlingGetNextSrcBlock
Input       :PuNum,usRecentBlock
Output      :Next Src Block
Author      :AwayWei
Date        :2014.01.02
Description :
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TableErrorHandlingGetNextSrcBlock(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usRecentBlock, U16 usStartBlock)
{
    U32 ulBlock, ulLoop;
    BOOL bBadBlk;
    U8 ucTLun = L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu);

    ulBlock = INVALID_4F;

    for (ulLoop = (usRecentBlock - 1); ulLoop >= usStartBlock; ulLoop--)
    {
        bBadBlk = L2_BbtIsGBbtBadBlock(ucTLun, ulLoop);
        if (TRUE == bBadBlk)
        {
            continue;
        }

        ulBlock = ulLoop;
        break;
    }

    return ulBlock;
}

/****************************************************************************
Name        :L2_TableErrorHandlingIsMoveDone
Input       :PuNum,Start Block,Handle Block
Output      :TRUE/FALSE
Author      :AwayWei
Date        :2014.01.02
Description :
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TableErrorHandlingIsMoveDone(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usStartBlock, U16 usHandleBlock)
{
    if (usStartBlock == usHandleBlock)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Name        :L2_TableMoveUpdateTable
Input       :PuNum
Output      :NULL
Author      :AwayWei
Date        :2014.01.02
Description :
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TableMoveUpdateTable(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usSrcBlock, U16 usTgtBlock)
{
    U32 usBlockIndex, usTmpBlock1, usTmpBlock2;
    U16 usPageIndex;

    for (usBlockIndex = 0; usBlockIndex < TRACE_BLOCK_COUNT; usBlockIndex++)
    {
        if (0 == usBlockIndex)
        {
            usTmpBlock1 = pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][(TRACE_BLOCK_COUNT - usBlockIndex - 1)];
            usTmpBlock2 = usTgtBlock;
            pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][(TRACE_BLOCK_COUNT - usBlockIndex - 1)] = usTmpBlock2;
        }
        else
        {
            usTmpBlock2 = usTmpBlock1;
            usTmpBlock1 = pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][(TRACE_BLOCK_COUNT - usBlockIndex - 1)];
            pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][(TRACE_BLOCK_COUNT - usBlockIndex - 1)] = usTmpBlock2;
        }

        if (usTmpBlock1 == usSrcBlock)
        {
            return TRUE;
        }
    }

    /* mark AT1 block */
    for (usBlockIndex = 0; usBlockIndex < AT1_BLOCK_COUNT; usBlockIndex++)
    {
        usTmpBlock2 = usTmpBlock1;
        usTmpBlock1 = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][(AT1_BLOCK_COUNT - usBlockIndex - 1)];
        if (pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] == pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][(AT1_BLOCK_COUNT - usBlockIndex - 1)])
        {
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = usTmpBlock2;
        }
        pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][(AT1_BLOCK_COUNT - usBlockIndex - 1)] = usTmpBlock2;

        if (usTmpBlock1 == usSrcBlock)
        {
            return TRUE;
        }
    }

    /* mark AT0 block */
    for (usBlockIndex = 0; usBlockIndex < AT0_BLOCK_COUNT; usBlockIndex++)
    {

        usTmpBlock2 = usTmpBlock1;
        usTmpBlock1 = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)];
        for (usPageIndex = 0; usPageIndex < PMTI_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }
#ifndef LCT_VALID_REMOVED
        for (usPageIndex = 0; usPageIndex < VBMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }
#endif
        for (usPageIndex = 0; usPageIndex < VBT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }
        for (usPageIndex = 0; usPageIndex < PBIT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }
        for (usPageIndex = 0; usPageIndex < DPBM_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }
        for (usPageIndex = 0; usPageIndex < RPMT_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        for (usPageIndex = 0; usPageIndex < TLCW_SUPERPAGE_COUNT_PER_SUPERPU; usPageIndex++)
        {
            if (pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
            {
                pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex] = usTmpBlock2;
            }
        }
#endif
    
        if (pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] == pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)])
        {
            pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = usTmpBlock2;
        }
        pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(AT0_BLOCK_COUNT - usBlockIndex - 1)] = usTmpBlock2;

        if (usTmpBlock1 == usSrcBlock)
        {
            return TRUE;
        }
    }

    return TRUE;
}


/****************************************************************************
Name        :L2_TableCheckBlockPointer
Input       :PuNum
Output      :TRUE/FALSE
Author      :AwayWei
Date        :2014.01.02
Description :
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TableCheckBlockPointer(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16* pTBMoveTgtBlock)
{
    U16 ReservedBlockAddr;

    ReservedBlockAddr = L2_RT_FindNextGoodBlock(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu],TRUE);

    if (INVALID_4F == ReservedBlockAddr)
    {
        return FALSE;
    }

    if (g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] > ReservedBlockAddr)
    {
        *pTBMoveTgtBlock = ReservedBlockAddr;
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Name        :L2_TableErrorHandlingMoveBlockHandle
Input       :PuNum,Src block,Tgt Block
Output      :Success/Fail
Author      :AwayWei
Date        :2014.01.02
Description :
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TableErrorHandlingMoveBlockHandle(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usSrcBlock, U16 usTgtBlock)
{
    U32 Ret;
    U32* pTBMoveOneBlockState;
    U8* pTBMoveStatus;
    PhysicalAddr* pRdAddr;
    PhysicalAddr* pPrgAddr;
    PhysicalAddr* pErsAddr;
    RED* pTBSpare;
    U16* pErrorPage;
    U16* pErrorBlock;
    U32 pBufferAddr;
    U8 cnt;

    pBufferAddr = COM_GetMemAddrByBufferID((L2AT0StartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);

    pTBMoveOneBlockState = &pTBMoveBlockManagement->m_MoveOneBlockState[ucSuperPuNum][ucLunInSuperPu];
    pTBMoveStatus = &pTBMoveBlockManagement->m_MoveStatus[ucSuperPuNum][ucLunInSuperPu];
    pRdAddr = &pTBMoveBlockManagement->m_RdAddr[ucSuperPuNum][ucLunInSuperPu];
    pPrgAddr = &pTBMoveBlockManagement->m_PrgAddr[ucSuperPuNum][ucLunInSuperPu];
    pErsAddr = &pTBMoveBlockManagement->m_ErsAddr[ucSuperPuNum][ucLunInSuperPu];
    pTBSpare = &pTBMoveBlockManagement->m_TBSpare[ucSuperPuNum][ucLunInSuperPu];
    pErrorPage = &pTBMoveBlockManagement->m_ErrorPage[ucSuperPuNum][ucLunInSuperPu];
    pErrorBlock = &pTBMoveBlockManagement->m_ErrorBlock[ucSuperPuNum][ucLunInSuperPu];

    switch (*pTBMoveOneBlockState)
    {
    case L2_ERR_HANDLE_MOVE_BLOCK_INIT:
        //don't need to move the fail block
        if (0 == *pErrorPage && usSrcBlock == *pErrorBlock)
        {
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_FINISH;
            Ret = TRUE;
            break;
        }
        pRdAddr->m_PPN = 0;
        pPrgAddr->m_PPN = 0;
        pErsAddr->m_PPN = 0;

        pRdAddr->m_PUSer = ucSuperPuNum;
        pRdAddr->m_BlockInPU = usSrcBlock;
        pRdAddr->m_PageInBlock = 0;
        pRdAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        pPrgAddr->m_PUSer = ucSuperPuNum;
        pPrgAddr->m_BlockInPU = usTgtBlock;
        pPrgAddr->m_PageInBlock = 0;
        pPrgAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        pErsAddr->m_PUSer = ucSuperPuNum;
        pErsAddr->m_BlockInPU = usSrcBlock;
        pErsAddr->m_PageInBlock = 0;
        pErsAddr->m_OffsetInSuperPage = ucLunInSuperPu;

        for (cnt = 0; cnt < (sizeof(RED) / 4); cnt++)
            pTBSpare->m_Content[cnt] = 0;

        *pTBMoveStatus = SUBSYSTEM_STATUS_SUCCESS;
        *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_RD;
        Ret = FALSE;
        break;
    case L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_RD:
        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            Ret = FALSE;
            break;
        }
        else
        {
            *pTBMoveStatus = SUBSYSTEM_STATUS_PENDING;
            L2_FtlReadLocal((U32*)pBufferAddr, pRdAddr, pTBMoveStatus, (U32*)pTBSpare, LPN_PER_BUF, 0, TRUE, TRUE);
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_RD_STATUS;
            Ret = FALSE;
            break;
        }


    case L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_RD_STATUS:
        if (SUBSYSTEM_STATUS_SUCCESS == *pTBMoveStatus || SUBSYSTEM_STATUS_RECC == *pTBMoveStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == *pTBMoveStatus)
        {
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_PRG;

        }
        else if (SUBSYSTEM_STATUS_PENDING == *pTBMoveStatus)
        {

        }
        else if (SUBSYSTEM_STATUS_EMPTY_PG == *pTBMoveStatus)
        {
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_ERS;
        }
        else if (SUBSYSTEM_STATUS_FAIL == *pTBMoveStatus)
        {
            COM_MemZero((U32*)pBufferAddr, BUF_SIZE / 4);
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_PRG;
        }
        else
        {
            //error nest case,need coding
            if((SUBSYSTEM_STATUS_SUCCESS != *pTBMoveStatus) && (SUBSYSTEM_STATUS_RECC != *pTBMoveStatus)
               && (SUBSYSTEM_STATUS_RETRY_SUCCESS != *pTBMoveStatus) && (SUBSYSTEM_STATUS_FAIL != *pTBMoveStatus)
               && (SUBSYSTEM_STATUS_EMPTY_PG != *pTBMoveStatus) && (SUBSYSTEM_STATUS_PENDING != *pTBMoveStatus))
            {
                DBG_Getch();
            }
        }
        Ret = FALSE;
        break;
    case L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_PRG:
        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            Ret = FALSE;
            break;
        }
        else
        {
            *pTBMoveStatus = SUBSYSTEM_STATUS_PENDING;
            L2_FtlWriteLocal(pPrgAddr, (U32*)pBufferAddr, (U32*)pTBSpare, pTBMoveStatus, TRUE, TRUE, NULL);

#ifdef L2MEASURE
            L2MeasureLogIncWCnt(ucSuperPuNum, L2MEASURE_TYPE_OTHER);
#endif

            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_PRG_STATUS;
            Ret = FALSE;
            break;
        }
    case L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_PRG_STATUS:
        if (SUBSYSTEM_STATUS_SUCCESS == *pTBMoveStatus)
        {
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_CALC_ADDR;
        }
        else if (SUBSYSTEM_STATUS_PENDING == *pTBMoveStatus)
        {

        }
        else
        {
            if( (SUBSYSTEM_STATUS_SUCCESS != *pTBMoveStatus) && (SUBSYSTEM_STATUS_PENDING != *pTBMoveStatus))
            {
                //error nest case,need coding
                DBG_Getch();
            }
        }
        Ret = FALSE;
        break;

    case L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_ERS:
        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
        {
            Ret = FALSE;
            break;
        }
        else
        {
            *pTBMoveStatus = SUBSYSTEM_STATUS_PENDING;

            if ((ucSuperPuNum != pErsAddr->m_PUSer) || (ucLunInSuperPu != pErsAddr->m_OffsetInSuperPage))
            {
                DBG_Getch();
            }

            L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, pErsAddr->m_BlockInPU, pTBMoveStatus, TRUE, TRUE, FALSE);
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_ERS_STATUS;
            Ret = FALSE;
            break;

        }

    case L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_ERS_STATUS:
        if (SUBSYSTEM_STATUS_SUCCESS == *pTBMoveStatus)
        {
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_FINISH;
            Ret = FALSE;
            break;
        }
        else if (SUBSYSTEM_STATUS_PENDING == *pTBMoveStatus)
        {
            ;
        }
        else
        {
            //error nest case,need coding
            if(pTBMoveBlockManagement->m_ErrorBlock[ucSuperPuNum][ucLunInSuperPu] != pErsAddr->m_BlockInPU)
            {
                DBG_Printf("L2_TableErrorHandlingMoveBlockHandle: PU %d LUN %d erase Blk %d fail !\n", ucSuperPuNum, ucLunInSuperPu, pErsAddr->m_BlockInPU);
                DBG_Getch();
            }
            else
            {
                *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_FINISH;
            }
        }
        Ret = FALSE;
        break;

    case L2_ERR_HANDLE_MOVE_BLOCK_CALC_ADDR:
        pRdAddr->m_PageInBlock++;
        pPrgAddr->m_PageInBlock++;
        if (PG_PER_SLC_BLK == pPrgAddr->m_PageInBlock)
        {
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_ERS;
        }
        else
        {
            //don't read the prg fail page
            if (pRdAddr->m_PageInBlock == *pErrorPage && pRdAddr->m_BlockInPU == *pErrorBlock)
            {
                *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_ERS;
            }
            else
            {
                *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_HANDLE_RD;
            }
        }
        Ret = FALSE;
        break;
    case L2_ERR_HANDLE_MOVE_BLOCK_FINISH:
        *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_INIT;
        Ret = TRUE;
        break;

    }

    return Ret;
}
/****************************************************************************
Name        :L2_IsAllPuMoveBlockDone
Input       :Null
Output      :TRUE or FLASE
Author      :NinaYang
Date        :2014.02.13
Description :Check whether all PU move block done
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_IsAllPuMoveBlockDone(void)
{
    U8 ucCheckPuCnt;
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    U32* pTBMoveState;

    ucCheckPuCnt = 0;
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBMoveState = &pTBMoveBlockManagement->m_MoveState[ucSuperPuNum][ucLunInSuperPu];

            if ((L2_ERR_HANDLE_MOVE_FINISH == *pTBMoveState) || (L2_ERR_HANDLE_MOVE_INIT == *pTBMoveState))
            {
                ucCheckPuCnt++;
            }
        }
    }

    if (ucCheckPuCnt == SUBSYSTEM_SUPERPU_NUM * LUN_NUM_PER_SUPERPU)
    {
        return TRUE;
    }
    return FALSE;
}
/*****************************************************************************
 Prototype      : L2_TBMoveBlockManagement_Init
 Description    : L2_TBMoveBlockManagement_Init
 Input          : None
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/6/4
 Author       : NinaYang
 Modification : Created function

 *****************************************************************************/
void MCU1_DRAM_TEXT L2_TBMoveBlockManagement_Init()
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBMoveBlockManagement->m_MoveState[ucSuperPuNum][ucLunInSuperPu] = L2_ERR_HANDLE_MOVE_INIT;
            pTBMoveBlockManagement->m_MoveOneBlockState[ucSuperPuNum][ucLunInSuperPu] = L2_ERR_HANDLE_MOVE_BLOCK_INIT;

            pTBMoveBlockManagement->m_MoveSrcBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBMoveBlockManagement->m_MoveTgtBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBMoveBlockManagement->m_ErrorBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBMoveBlockManagement->m_ErrorPage[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBMoveBlockManagement->m_RdAddr[ucSuperPuNum][ucLunInSuperPu].m_PPN = 0;
            pTBMoveBlockManagement->m_PrgAddr[ucSuperPuNum][ucLunInSuperPu].m_PPN = 0;
            pTBMoveBlockManagement->m_ErsAddr[ucSuperPuNum][ucLunInSuperPu].m_PPN = 0;
            pTBMoveBlockManagement->m_MoveStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;
        }
    }
}

/****************************************************************************
Name        :L2_TableErrorHandlingMoveBlock
Input       :PuNum,Error start block addr
Output      :Null
Author      :AwayWei
Date        :2014.01.02
Description :Move all table block to next block when error occur.
Others      :
Modify      :
****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TableErrorHandlingMoveBlock(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usStartBlock, U16 usErrorPage, U8 ucErrType)
{
    U32 Status = FALSE;
    U32 Ret;
    U32* pTBMoveState;
    U16* pTBMoveSrcBlock;
    U16* pTBMoveTgtBlock;
    U32* pTBMoveOneBlockState;

    pTBMoveState = &pTBMoveBlockManagement->m_MoveState[ucSuperPuNum][ucLunInSuperPu];
    pTBMoveSrcBlock = &pTBMoveBlockManagement->m_MoveSrcBlock[ucSuperPuNum][ucLunInSuperPu];
    pTBMoveTgtBlock = &pTBMoveBlockManagement->m_MoveTgtBlock[ucSuperPuNum][ucLunInSuperPu];
    pTBMoveOneBlockState = &pTBMoveBlockManagement->m_MoveOneBlockState[ucSuperPuNum][ucLunInSuperPu];

    switch (*pTBMoveState)
    {
    case L2_ERR_HANDLE_MOVE_INIT:
        //Check if there is reseved block

        if (TRUE == L2_TableCheckBlockPointer(ucSuperPuNum, ucLunInSuperPu, pTBMoveTgtBlock))
        {
            *pTBMoveSrcBlock = L2_TableErrorHandlingGetNextSrcBlock(ucSuperPuNum, ucLunInSuperPu, *pTBMoveTgtBlock, usStartBlock);

            if (INVALID_4F == *pTBMoveSrcBlock)
            {
                DBG_Getch();
            }
            *pTBMoveOneBlockState = L2_ERR_HANDLE_MOVE_BLOCK_INIT;
            *pTBMoveState = L2_ERR_HANDLE_MOVE_HANDLE;
            pTBMoveBlockManagement->m_ErrorBlock[ucSuperPuNum][ucLunInSuperPu] = usStartBlock;
            pTBMoveBlockManagement->m_ErrorPage[ucSuperPuNum][ucLunInSuperPu] = usErrorPage;
            Ret = L2_ERR_HANDLING_PENDING;
        }
        else
        {
            *pTBMoveState = L2_ERR_HANDLE_NO_RESEVED_BLOCK;
            Ret = L2_ERR_HANDLING_NO_BLOCK;
        }
        break;

    case L2_ERR_HANDLE_NO_RESEVED_BLOCK:
        //need coding
        DBG_Getch();
        break;

    case L2_ERR_HANDLE_MOVE_HANDLE:
        Status = L2_TableErrorHandlingMoveBlockHandle(ucSuperPuNum, ucLunInSuperPu, *pTBMoveSrcBlock, *pTBMoveTgtBlock);
        if (TRUE == Status)
        {
            if (TRUE == L2_TableErrorHandlingIsMoveDone(ucSuperPuNum, ucLunInSuperPu, usStartBlock, *pTBMoveSrcBlock))
            {
                L2_TableMoveUpdateTable(ucSuperPuNum, ucLunInSuperPu, usStartBlock, L2_RT_FindNextGoodBlock(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu],TRUE));
                g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu] = L2_RT_FindNextGoodBlock(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu],TRUE);

                L2_BbtSetRootPointer(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu], g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu]);

                /* add to BBT */
                L2_BbtAddBbtBadBlk(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), usStartBlock, RT_BAD_BLK, ucErrType);
                gTBSaveBBTState = L2_SAVE_BBT_STATE_SEND_REQ;
                *pTBMoveState = L2_ERR_HANDLE_MOVE_FINISH;
                Ret = L2_ERR_HANDLING_PENDING;
                DBG_Printf("L2_TableErrorHandlingMoveBlockHandle PU %d LUN %d move finish,add Blk %d to BBT\n", ucSuperPuNum, ucLunInSuperPu, usStartBlock);
            }
            else
            {
                *pTBMoveState = L2_ERR_HANDLE_MOVE_ONE_BLOCK_FINISH;
                Ret = L2_ERR_HANDLING_PENDING;
            }
        }
        else
        {
            *pTBMoveState = L2_ERR_HANDLE_MOVE_HANDLE;
            Ret = L2_ERR_HANDLING_PENDING;
        }

        break;

    case L2_ERR_HANDLE_MOVE_ONE_BLOCK_FINISH:
        *pTBMoveTgtBlock = *pTBMoveSrcBlock;
        *pTBMoveSrcBlock = L2_TableErrorHandlingGetNextSrcBlock(ucSuperPuNum, ucLunInSuperPu, *pTBMoveSrcBlock, usStartBlock);

        if (INVALID_4F == *pTBMoveSrcBlock)
        {
            DBG_Printf("No Src Block!\n");
            DBG_Getch();
        }
        *pTBMoveState = L2_ERR_HANDLE_MOVE_HANDLE;
        Ret = L2_ERR_HANDLING_PENDING;
        break;

    case L2_ERR_HANDLE_MOVE_FINISH:
        if (FALSE == L2_IsAllPuMoveBlockDone())
        {
            Ret = L2_ERR_HANDLING_PENDING;
            break;
        }

        if (TRUE == L2_BbtSave(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), INVALID_2F))
        {
            *pTBMoveState = L2_ERR_HANDLE_MOVE_INIT;
            Ret = L2_ERR_HANDLING_SUCCESS;
        }
        else
        {
            *pTBMoveState = L2_ERR_HANDLE_MOVE_FINISH;
            Ret = L2_ERR_HANDLING_PENDING;
        }
        break;

    default:
        DBG_Printf("Fetal ERROR in L2 TB!\n");
        DBG_Getch();
        break;
    }

    return Ret;
}

void MCU1_DRAM_TEXT L2_TBManagement_Init(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    RED* pRed;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBManagement->SaveStatus[ucSuperPuNum] = L2_TB_SAVE_START;
            pTBManagement->LoadStatus[ucSuperPuNum] = L2_TB_LOAD_START;
            pTBManagement->CheckStatus[ucSuperPuNum] = L2_TB_CHECK_START;
            pTBManagement->CheckPMTStatus[ucSuperPuNum] = L2_TB_CHECK_PMT_START;
            pTBManagement->CheckDataBlkStatus[ucSuperPuNum] = L2_TB_CHECK_DATA_BLK_START;
            pTBManagement->ucNeedCheckPMTTS[ucSuperPuNum] = FALSE;

            pTBManagement->UECCStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_UECC_START;
            pTBManagement->usPageIndex[ucSuperPuNum] = INVALID_4F;
            pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;


            pRed = &(pTBRedInfo->PBIT_Red[ucSuperPuNum][ucLunInSuperPu]);

            HAL_DMAESetValue((U32)pRed, (RED_SW_SZ_DW * DWORD_SIZE), 0);

            pRed = &(pTBRedInfo->Data_Red[ucSuperPuNum][ucLunInSuperPu]);

            HAL_DMAESetValue((U32)pRed, (RED_SW_SZ_DW * DWORD_SIZE), 0);
        }
    }

    pTBManagement->BootStatus = L2_TB_BOOT_START;
    pTBManagement->ShutdownStatus = L2_TB_SHUTDOWN_START;

    return;
}

void MCU1_DRAM_TEXT L2_TBRebManagement_Init(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    U8 ucTargetType;
    U8 ucTargetIndex;
    U8 AT0BlkO;
    RED* pRed;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->RebuildStatus[ucSuperPuNum] = L2_TB_REBUILD_START;
            pTBRebManagement->RebuildDataBlockStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_REBUILD_START;
            pTBRebManagement->usUECCErrCnt[ucSuperPuNum][ucLunInSuperPu] = 0;
            pTBRebManagement->usBlkTypeErrCnt[ucSuperPuNum][ucLunInSuperPu] = 0;

            pTBRebManagement->UECCStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_UECC_START;
            pTBRebManagement->EraseAllStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERASEALL_START;

            pTBRebManagement->CheckScanResultStatus[ucSuperPuNum] = L2_TB_REBUILD_CHECK_START;

            pTBRebManagement->ucSLCMode[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->usPageIndex[ucSuperPuNum][ucLunInSuperPu]= INVALID_4F;
            pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBRebManagement->usBlockErase[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBRebManagement->ucFirstPageEmpty[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->ucFirstPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->ucLastPageUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->ucFirstPageBlkTypeErr[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->usSameVirBlock[ucSuperPuNum][ucLunInSuperPu] = INVALID_4F;
            pTBRebManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;

            pTBRebManagement->usReadSuccessLun[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->bIsEraseFail[ucSuperPuNum][ucLunInSuperPu] = FALSE;
            pTBRebManagement->bIsAllRPMTUECC[ucSuperPuNum][ucLunInSuperPu] = FALSE;

            pTBRebManagement->ErrTLCHandleStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERRTLC_HANDLE_START;
            pTBRebManagement->ulErrTLCHandleSN[ucSuperPuNum][ucLunInSuperPu] = 0;
            pTBRebManagement->usErrTLCBlkCnt[ucSuperPuNum][ucLunInSuperPu] = 0;

            for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
            {
                for (ucTargetIndex = 0; ucTargetIndex < TEMP_BLK_CNT_PER_TARGET; ucTargetIndex++)
                {
                    pTBRebManagement->usTargetBlkUECCErrCnt[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = 0;
                    pTBRebManagement->usTargetBlock[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = INVALID_4F;
                    pTBRebManagement->usTargetPPO[ucSuperPuNum][ucLunInSuperPu][ucTargetType][ucTargetIndex] = INVALID_4F;
                }
                pTBRebManagement->ucTargetIndex[ucSuperPuNum][ucLunInSuperPu][ucTargetType] = 0;
                pTBRebManagement->ulTargetOffsetTS[ucSuperPuNum][ucLunInSuperPu][ucTargetType] = INVALID_8F;
            }

            pRed = &(pTBRedInfo->PBIT_Red[ucSuperPuNum][ucLunInSuperPu]);

            HAL_DMAESetValue((U32)pRed, (RED_SW_SZ_DW * DWORD_SIZE), 0);

            pRed = &(pTBRedInfo->Data_Red[ucSuperPuNum][ucLunInSuperPu]);

            HAL_DMAESetValue((U32)pRed, (RED_SW_SZ_DW * DWORD_SIZE), 0);
        }

        /*nick add for super page rebuild table block use, current not use*/
        for (AT0BlkO = 0; AT0BlkO < AT0_BLOCK_COUNT; AT0BlkO++)
        {
            pTBRebManagement->bIsAT0NeedErase[ucSuperPuNum][AT0BlkO] = FALSE;
        }
        pTBRebManagement->TableBlkEraseStatus[ucSuperPuNum] = L2_TB_ERASE_START;
        pTBRebManagement->ucReadLunEmptyCnt[ucSuperPuNum] = 0;
        pTBRebManagement->ucReadLunSuccessCnt[ucSuperPuNum] = 0;
        pTBRebManagement->ucReadLunUECCCnt[ucSuperPuNum] = 0;
        bRTHasLoad[ucSuperPuNum] = FALSE;
    }

    pTBRebManagement->FullRecoveryStatus = L2_TB_FULLRECOVERY_START;
    bTBRebuildPending = FALSE;

    return;
}

void MCU1_DRAM_TEXT L2_TB_Reset_All(void)
{
    /* blakezhang add for Init RT, PBIT and VBT */
    L2_RT_Init_Clear_All();
    //L2_PBIT_Init_Clear_All();
    L2_VBT_Init_Clear_All();

    L2_RTManagement_Init();
    L2_TBManagement_Init();
    L2_TBRebManagement_Init();
    L2_TBMoveBlockManagement_Init();
    return;
}

void MCU1_DRAM_TEXT L2_TableBlock_LLF(BOOL bKeepEraseCnt)
{
    U8  ucLunInSuperPu,i,ucSuperPuNum, ucTLun;
    U16 usBlock,usBlockPos, usBlockIndex;
    U16 GoodVBlockCnt;
    BOOL bFind = TRUE;
    U16 VirtualBlockAddr,usVirBlk;
    U16 usLunBlock[LUN_NUM_PER_SUPERPU];
    U16 usPerGoodPhyBlk[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    BOOL bBadBlk, bHaveFreePhyBlk = TRUE;
    U16 usInvVirBlk;
    U16 usRealTLCBLkCnt[LUN_NUM_PER_SUPERPU];
    U16 usSLCBLkCnt[LUN_NUM_PER_SUPERPU];

    L2_RT_Init();
    L2_PBIT_Init(TRUE, bKeepEraseCnt);
    L2_VBT_Init();

    /* Initialization of global data structures used by trace module. */
    FW_TraceManagementLLF();

    /* mark bad block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            U8 ucTLun = L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu);
            for (usBlock = 0; usBlock < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlock++)
            {
                bBadBlk = L2_BbtIsGBbtBadBlock(ucTLun, usBlock);
                if (TRUE == bBadBlk)
                {
                    L2_PBIT_Set_Error(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
                }
            }
        }
    }

    // Initial value
    usBlockPos = 0;

    /* mark GB block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlockPos, TRUE);
        }
    }

    usBlockPos += GLOBAL_BLOCK_COUNT;

    /* mark BBT block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockPos, TRUE);
            L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
        }
    }

    usBlockPos += BBT_BLOCK_COUNT;

    /* mark RT block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockPos, TRUE);
            L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
        }
    }

    usBlockPos += RT_BLOCK_COUNT;

    /* mark AT0 block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + AT0_BLOCK_COUNT); usBlockIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
                pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][(usBlockIndex - usBlockPos)] = usBlock;
                L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][0];
        }
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = 0;
    }

    usBlockPos += AT0_BLOCK_COUNT;

    /* mark AT1 block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + AT1_BLOCK_COUNT); usBlockIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
                pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][(usBlockIndex - usBlockPos)] = usBlock;
                L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].CurAT1Block[ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][0];
        }
        pRT->m_RT[ucSuperPuNum].CurAT1PPO = 0;
    }

    usBlockPos += AT1_BLOCK_COUNT;

    /* mark Trace block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + TRACE_BLOCK_COUNT); usBlockIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
                pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][(usBlockIndex - usBlockPos)] = usBlock;
                L2_PBIT_Set_Table(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].TraceBlockAddr[ucLunInSuperPu][TRACE_BLOCK_COUNT - 1];
        }
    }

    usBlockPos += TRACE_BLOCK_COUNT;

    /* mark Rsvd block */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (usBlockIndex = usBlockPos; usBlockIndex < (usBlockPos + RSVD_BLOCK_COUNT); usBlockIndex++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlock = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockIndex, TRUE);
                L2_PBIT_Set_Reserve(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
            }
        }
    }

    usBlockPos += RSVD_BLOCK_COUNT;

    /* mark data block start */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] = L2_RT_Search_BlockBBT(ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu, usBlockPos, TRUE);
            usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu] = BLK_PER_PLN + RSV_BLK_PER_PLN;
        }
    }

    /* Enable good block and backup block
       Nina 2015/12/22 Optimize setup VBT&PBIT mapping to reduce LLF/SecurityErase/Nvme Format Cmd time */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        GoodVBlockCnt = 0;
        VirtualBlockAddr = 0;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usSLCBLkCnt[ucLunInSuperPu] = 0;
            usRealTLCBLkCnt[ucLunInSuperPu] = TLC_BLK_CNT;
        }
        for (usBlockIndex = 0; usBlockIndex < (BLK_PER_PLN + RSV_BLK_PER_PLN); usBlockIndex++)
        {
            /* allocate TLC block */
            if (GoodVBlockCnt < TLC_BLK_CNT && TRUE == bHaveFreePhyBlk)
            {
                bHaveFreePhyBlk = TRUE;
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    ucTLun = L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu);
                    usLunBlock[ucLunInSuperPu] = L2_RT_FindNextGoodBlock(ucTLun, usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu], FALSE);
                    if ((INVALID_4F == usLunBlock[ucLunInSuperPu]) && (GoodVBlockCnt < DATA_BLOCK_PER_PU))
                    {
                        DBG_Printf("TLC block not enough!\n");
                        DBG_Getch();
                    }

                    if (usLunBlock[ucLunInSuperPu] < g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu])
                    {
                        bHaveFreePhyBlk = FALSE;
                    }
                }

                if (FALSE == bHaveFreePhyBlk)
                {
                    for (usInvVirBlk = VirtualBlockAddr; usInvVirBlk < TLC_BLK_CNT; usInvVirBlk++)
                    {
                        L2_VBT_SetInvalidPhyBlk(ucSuperPuNum, usInvVirBlk);
                        L2_VBT_Set_Free(ucSuperPuNum, usInvVirBlk, FALSE);
                    }

                    /*Adjust TLCBLkEnd */
                    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
                    {                    
                        ucTLun = L2_GET_TLUN(ucSuperPuNum, i);
                        g_ulTLCBlockEnd[ucSuperPuNum][i] = L2_RT_FindNextGoodBlock(ucTLun, usLunBlock[i], TRUE);

                        /*set PreGoodPhyBlk for SLC Blk */
                        usPerGoodPhyBlk[ucSuperPuNum][i] = g_ulDataBlockStart[ucSuperPuNum][i] - 1;
                        usRealTLCBLkCnt[i] = GoodVBlockCnt;
                    }

                    /* Note: SLC Blk index start from TLC_BLK_CNT */
                    VirtualBlockAddr = TLC_BLK_CNT;
                    continue;
                }

                //TLC Data Blocks
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu], TRUE);
                    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu], VirtualBlockAddr);
                    L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu], TRUE);
                    L2_VBT_Set_TLC(ucSuperPuNum, VirtualBlockAddr, TRUE);

                    /*if (0 == ucSuperPuNum)
                    {
                        FIRMWARE_LogInfo("TLCBlk PU %d LUNInSuperPU %d virBlk %d ==>  PhyBlk %d  \n", ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, usLunBlock[ucLunInSuperPu]);
                    }*/                    

                    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, usLunBlock[ucLunInSuperPu]);
                    L2_VBT_Set_Free(ucSuperPuNum, VirtualBlockAddr, TRUE);
                    /* record previous none bad phyblk */
                    usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu] = usLunBlock[ucLunInSuperPu];

                    if ((TLC_BLK_CNT - 1) == GoodVBlockCnt)
                    {
                        /* record last allocate tlc phy blk num */
                        g_ulTLCBlockEnd[ucSuperPuNum][ucLunInSuperPu] = usLunBlock[ucLunInSuperPu];

                        /*set PreGoodPhyBlk for SLC Blk */
                        usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu] = g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu] - 1;
                    }
                }
            }
            else
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    ucTLun = L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu);
                    usLunBlock[ucLunInSuperPu] = L2_RT_FindNextGoodBlock(ucTLun, usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu], TRUE);
                    if (INVALID_4F == usLunBlock[ucLunInSuperPu])
                    {
                        break;
                    }
                    else
                    {
                        usSLCBLkCnt[ucLunInSuperPu]++;
                    }
                }
                //SLC Data Blocks
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    usVirBlk = L2_PBIT_GetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu]);
                    if (INVALID_4F != usVirBlk)
                    {
                        /*if already mapping to TLC, unmap it*/
                        L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlk, INVALID_4F);
                        L2_VBT_Set_TLC(ucSuperPuNum, usVirBlk, FALSE);
                        //FIRMWARE_LogInfo("unmap TLC SPU %d LUNInSuperPU %d virBlk %d -->  PhyBlk %d -> 0xFFFF \n", ucSuperPuNum, ucLunInSuperPu, usVirBlk, usLunBlock[ucLunInSuperPu]);
                    }

                    L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu], TRUE);
                    L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu], VirtualBlockAddr);
                    L2_PBIT_Set_TLC(ucSuperPuNum, ucLunInSuperPu, usLunBlock[ucLunInSuperPu], FALSE);
                    L2_VBT_Set_TLC(ucSuperPuNum, VirtualBlockAddr, FALSE);

                    /*if (0 == ucSuperPuNum)
                    {
                         FIRMWARE_LogInfo("SLC SPU %d LUNInSuperPU %d virBlk %d ==>  PhyBlk %d  \n", ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, usLunBlock[ucLunInSuperPu]);
                    }*/

                    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, VirtualBlockAddr, usLunBlock[ucLunInSuperPu]);
                    L2_VBT_Set_Free(ucSuperPuNum, VirtualBlockAddr, TRUE);
                    /* record previous none bad phyblk */
                    usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu] = usLunBlock[ucLunInSuperPu];
                    
                    //record last allocate slc phy blk num
                    g_ulSLCBlockEnd[ucSuperPuNum][ucLunInSuperPu] = usLunBlock[ucLunInSuperPu];

                    if (usPerGoodPhyBlk[ucSuperPuNum][ucLunInSuperPu] >= g_ulTLCBlockEnd[ucSuperPuNum][ucLunInSuperPu])
                    {
                        /*Adjust TLCBLkEnd */
                        for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
                        {
                            if (usPerGoodPhyBlk[ucSuperPuNum][i] >= g_ulTLCBlockEnd[ucSuperPuNum][i])
                            {
                                g_ulTLCBlockEnd[ucSuperPuNum][i] = L2_RT_FindNextGoodBlock(ucSuperPuNum*LUN_NUM_PER_SUPERPU + i, usPerGoodPhyBlk[ucSuperPuNum][i], TRUE);
                            }
                            else
                            {
                                g_ulTLCBlockEnd[ucSuperPuNum][i] = L2_RT_FindNextGoodBlock(ucSuperPuNum*LUN_NUM_PER_SUPERPU + i, g_ulTLCBlockEnd[ucSuperPuNum][i], TRUE);
                            }
                            usRealTLCBLkCnt[i]--;
                        }
                    }
                }
            }
            pVBT[ucSuperPuNum]->m_VBT[VirtualBlockAddr].bFree = TRUE;  
            GoodVBlockCnt++;
            VirtualBlockAddr++;

            if (SLC_BLK_CNT == usSLCBLkCnt[0])
            {
                break;
            }
        }
        
        DBG_Printf("SPU %d SLCBlkCnt %d TLCBlkCnt %d\n", ucSuperPuNum, usSLCBLkCnt[0], usRealTLCBLkCnt[0]);
        if (usRealTLCBLkCnt[0] < DATA_BLOCK_PER_PU)
        {
            DBG_Printf("SPU %d TLCBLkCnt %d < %d\n", ucSuperPuNum, usRealTLCBLkCnt[0], DATA_BLOCK_PER_PU);
            //DBG_Getch();
        }

        pPBIT[ucSuperPuNum]->m_TotalDataBlockCnt[BLKTYPE_SLC] = usSLCBLkCnt[0];
        pPBIT[ucSuperPuNum]->m_TotalDataBlockCnt[BLKTYPE_TLC] = usRealTLCBLkCnt[0];
        pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_SLC] = 0;
        pPBIT[ucSuperPuNum]->m_AllocatedDataBlockCnt[BLKTYPE_TLC] = 0;

        //reset usBlockIndex for Backup block
        usBlockIndex = (pPBIT[ucSuperPuNum]->m_TotalDataBlockCnt[BLKTYPE_SLC] + usBlockPos);

        // Set backup block from usBlockIndex position.
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {           
            for (usBlock = g_ulSLCBlockEnd[ucSuperPuNum][ucLunInSuperPu] + 1; usBlock < g_ulTLCBlockEnd[ucSuperPuNum][ucLunInSuperPu]; usBlock++)
            {
                if (TRUE == pPBIT[ucSuperPuNum]->m_PBIT_Entry[ucLunInSuperPu][usBlock].bError)
                {
                    continue;
                }

                usVirBlk = L2_PBIT_GetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usBlock);
                if (INVALID_4F != usVirBlk)
                {   
                    /*if already mapping to TLC, unmap it*/
                    L2_VBT_SetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlk, INVALID_4F);
                    L2_VBT_Set_TLC(ucSuperPuNum, usVirBlk, FALSE);
                    //FIRMWARE_LogInfo("## unmap TLC PU %d LUNInSuperPU %d virBlk %d -->  PhyBlk %d -> 0xFFFF \n", ucSuperPuNum, ucLunInSuperPu, usVirBlk);
                }
                
                // Backup Blocks
                L2_PBIT_Set_Free(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
                L2_PBIT_Set_Backup(ucSuperPuNum, ucLunInSuperPu, usBlock, TRUE);
                L2_PBIT_SetVirturlBlockAddr(ucSuperPuNum, ucLunInSuperPu, usBlock, INVALID_4F);
                //FIRMWARE_LogInfo("## SetBakup TLC PU %d LUNInSuperPU %d PhyBlk %d \n", ucSuperPuNum, ucLunInSuperPu, usBlock);
            }
        }

        /* set VBT info */
        for (usInvVirBlk = VirtualBlockAddr; usInvVirBlk < VIR_BLK_CNT; usInvVirBlk++)
        {
            L2_VBT_SetInvalidPhyBlk(ucSuperPuNum, usInvVirBlk);
            L2_VBT_Set_Free(ucSuperPuNum, usInvVirBlk, FALSE);
        }
    }

    /* save BBT to store block Start/End Pointer */
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            L2_BbtSetRootPointer(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu], g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu]);
            L2_BbtSetSlcTlcBlock(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), g_ulSLCBlockEnd[ucSuperPuNum][ucLunInSuperPu], g_ulTLCBlockEnd[ucSuperPuNum][ucLunInSuperPu]);
        }
    }

#ifdef SIM
    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        L2_CheckVBTMapping(ucSuperPuNum);
    }
#endif

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetBootStatus(void)
{
    pTBManagement->BootStatus = L2_TB_BOOT_START;
    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetShutdownStatus(void)
{
    pTBManagement->ShutdownStatus = L2_TB_SHUTDOWN_START;
    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetFullRecoveryStatus(void)
{
    pTBRebManagement->FullRecoveryStatus = L2_TB_FULLRECOVERY_START;
    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetSaveStatus(void)
{
    U8 ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        pTBManagement->SaveStatus[ucSuperPuNum] = L2_TB_SAVE_START;
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetLoadStatus(void)
{
    U8 ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        pTBManagement->LoadStatus[ucSuperPuNum] = L2_TB_LOAD_START;
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetCheckStatus(void)
{
    U8 ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        pTBManagement->CheckStatus[ucSuperPuNum] = L2_TB_CHECK_START;
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetRebuildStatus(void)
{
    U8 ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        pTBRebManagement->RebuildStatus[ucSuperPuNum] = L2_TB_REBUILD_START;
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetUECCStatus(void)
{
    U8 ucLunInSuperPu;
    U8 ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->UECCStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_UECC_START;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetEraseAllStatus(void)
{
    U8 ucLunInSuperPu;
    U8 ucSuperPuNum;

    for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBRebManagement->EraseAllStatus[ucSuperPuNum][ucLunInSuperPu] = L2_TB_ERASEALL_START;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_TB_ResetDataBlockMaxTS(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    pTBRebManagement->ulCurBlockMaxTS[ucSuperPuNum][ucLunInSuperPu] = 0;
    pTBRebManagement->ulDataBlockMaxTS[ucSuperPuNum][ucLunInSuperPu] = 0;

    return;
}

U32 MCU1_DRAM_TEXT L2_TB_GetDataBlockMaxTS(U8 ucSuperPuNum)
{
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
    U32 ulLUNInSuperPU;
    U32 ulMaxTs = 0;;

    for(ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
    {
        if(pTBRebManagement->ulDataBlockMaxTS[ucSuperPuNum][ulLUNInSuperPU] > ulMaxTs)
        {
            ulMaxTs = pTBRebManagement->ulDataBlockMaxTS[ucSuperPuNum][ulLUNInSuperPU];
        }
    }
    return ulMaxTs;
#else        
    return (pTBRebManagement->ulDataBlockMaxTS[ucSuperPuNum][0]);
#endif
}

void MCU1_DRAM_TEXT L2_TB_ResetCurBlockMaxTS(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    pTBRebManagement->ulCurBlockMaxTS[ucSuperPuNum][ucLunInSuperPu] = 0;

    return;
}

void MCU1_DRAM_TEXT L2_TB_UpdateCurBlockMaxTS(U8 ucSuperPuNum, U8 ucLunInSuperPu, RED *pRed)
{
    if (pRed->m_RedComm.ulTimeStamp > pTBRebManagement->ulCurBlockMaxTS[ucSuperPuNum][ucLunInSuperPu])
    {
        pTBRebManagement->ulCurBlockMaxTS[ucSuperPuNum][ucLunInSuperPu] = pRed->m_RedComm.ulTimeStamp;
    }

    pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]].LastPageTimeStamp = pRed->m_RedComm.ulTimeStamp;
    pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][pTBRebManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu]].FirstPageRealTS = pRed->m_DataRed.m_uTLCFirstPageTS;

    return;
}

void MCU1_DRAM_TEXT L2_TB_UpdateDataBlockMaxTS(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    if (pTBRebManagement->ulCurBlockMaxTS[ucSuperPuNum][ucLunInSuperPu] > pTBRebManagement->ulDataBlockMaxTS[ucSuperPuNum][ucLunInSuperPu])
    {
        pTBRebManagement->ulDataBlockMaxTS[ucSuperPuNum][ucLunInSuperPu] = pTBRebManagement->ulCurBlockMaxTS[ucSuperPuNum][ucLunInSuperPu];
    }

    return;
}

U16 MCU1_DRAM_TEXT L2_TB_GetPageCntPerCE(PAGE_TYPE PageType)
{
    U16 usPageCount;

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        usPageCount = PBIT_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;

    case PAGE_TYPE_VBT:
    {
        usPageCount = VBT_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;

    case PAGE_TYPE_RPMT:
    {
        usPageCount = RPMT_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
    {
        usPageCount = TLCW_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;
#endif

    case PAGE_TYPE_DPBM:
    {
        usPageCount = DPBM_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;

    case PAGE_TYPE_PMTI:
    {
        usPageCount = PMTI_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;

#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
    {
        usPageCount = (U16)VBMT_SUPERPAGE_COUNT_PER_SUPERPU;
    }
        break;
#endif

    default:
    {
        DBG_Printf("L2_TB_GetPageCntPerCE invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }
        break;
    }

    return usPageCount;
}

OP_TYPE MCU1_DRAM_TEXT L2_TB_GetOPType(PAGE_TYPE PageType)
{
    OP_TYPE ulOPType;

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        ulOPType = OP_TYPE_PBIT_WRITE;
    }
        break;

    case PAGE_TYPE_VBT:
    {
        ulOPType = OP_TYPE_VBT_WRITE;
    }
        break;

    case PAGE_TYPE_RPMT:
    {
        ulOPType = OP_TYPE_RPMT_WRITE;
    }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
    {
        ulOPType = OP_TYPE_TLCW_WRITE;
    }
        break;
#endif

    case PAGE_TYPE_DPBM:
    {
        ulOPType = OP_TYPE_DPBM_WRITE;
    }
        break;

    case PAGE_TYPE_PMTI:
    {
        ulOPType = OP_TYPE_PMTI_WRITE;
    }
        break;

#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
    {
        ulOPType = OP_TYPE_VBMT_WRITE;
    }
        break;
#endif

    default:
    {
        DBG_Printf("L2_TB_GetOPType invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }
        break;
    }

    return ulOPType;
}


U32 MCU1_DRAM_TEXT L2_TB_LoadGetFlashAddr(PAGE_TYPE PageType, U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex, PhysicalAddr* FlashAddr)
{
    U32 ulRet;

    ulRet = L2_TB_SUCCESS;

    FlashAddr->m_PPN = 0;

    if (L2_TB_GetPageCntPerCE(PageType) <= usPageIndex)
    {
        DBG_Printf("L2_TB_LoadGetFlashAddr PageType 0x%x usPageIndex 0x%x ERROR\n", PageType, usPageIndex);
        DBG_Getch();
    }

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].PBITPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;

    case PAGE_TYPE_VBT:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].VBTPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;

    case PAGE_TYPE_RPMT:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].RPMTPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].TLCWPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;
#endif

    case PAGE_TYPE_DPBM:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].DPBMPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;

    case PAGE_TYPE_PMTI:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].PMTIPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;

#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
    {
        FlashAddr->m_PUSer = ucSuperPuNum;
        FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
        FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex];
        FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].VBMTPPO[usPageIndex];
        FlashAddr->m_LPNInPage = 0;
    }
        break;
#endif

    default:
    {
        DBG_Printf("L2_TB_LoadGetFlashAddr invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}


void MCU1_DRAM_TEXT L2_TB_Allocate_NextFreeBlock(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
#if 0
    U16 CurrentPos,Pos;

    CurrentPos = INVALID_4F;

    for(Pos = 0; Pos < AT0_BLOCK_COUNT; Pos++)
    {
        if(pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][Pos] == pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu])
        {
            CurrentPos = Pos;
            break;
        }
    }

    if(CurrentPos == INVALID_4F)
    {
        DBG_Printf("L2_TB_Allocate_NextFreeBlock PU 0x%x LUN 0x%x CurAT0Block 0x%x\n", 
            ucSuperPuNum, ucLunInSuperPu, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu]);
        DBG_Getch();
    }

    /* set current block erase flag */
    pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[0][CurrentPos] = TRUE;

    CurrentPos++;
    if(CurrentPos >= AT0_BLOCK_COUNT)
    {
        CurrentPos = 0;
    }

    /* allocate free block */
    pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][CurrentPos];

    if (TRUE == pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[0][CurrentPos])
    {
        DBG_Printf("L2_TB_Allocate_NextFreeBlock PU 0x%x LUN 0x%x no free block!\n", ucSuperPuNum, ucLunInSuperPu);
        DBG_Getch();
    }

    return;
#else  
    U16 usCurrentPos, usPos, usNextPos;

    usCurrentPos = INVALID_4F;

    for (usPos = 0; usPos < AT0_BLOCK_COUNT; usPos++)
    {
        if (pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usPos] == pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu])
        {
            usCurrentPos = usPos;
            break;
        }
    }

    if (usCurrentPos == INVALID_4F)
    {
        DBG_Printf("L2_TB_Allocate_NextFreeBlock PU 0x%x LUN 0x%x CurAT0Block 0x%x\n",
            ucSuperPuNum, ucLunInSuperPu, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu]);
        DBG_Getch();
    }

    /* set current block erase flag */
    pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usCurrentPos] = TRUE;

    usNextPos = usCurrentPos;
    while (1)
    {
        usNextPos++;
        if (usNextPos >= AT0_BLOCK_COUNT)
        {
            usNextPos = 0;
        }

        if (usCurrentPos == usNextPos)
        {
            DBG_Printf("L2_TB_Allocate_NextFreeBlock PU 0x%x LUN 0x%x no free block!\n", ucSuperPuNum, ucLunInSuperPu);
            DBG_Getch();
        }

        /* allocate free block */
        if (FALSE == pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usNextPos])
        {
            pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usNextPos];
            break;
        }
    }
    return;
#endif
}

void MCU1_DRAM_TEXT L2_TB_SaveGetFlashAddr(U8 ucSuperPuNum, U8 ucLunInSuperPu, PhysicalAddr* FlashAddr)
{
    FlashAddr->m_PUSer = ucSuperPuNum;
    FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
    FlashAddr->m_BlockInPU = pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu];
    FlashAddr->m_PageInBlock = pRT->m_RT[ucSuperPuNum].CurAT0PPO;
    FlashAddr->m_LPNInPage = 0;

    return;
}

void MCU1_DRAM_TEXT L2_TB_SaveSetFlashAddr(PAGE_TYPE PageType, U8 ucSuperPuNum, U16 usPageIndex)
{
    PhysicalAddr FlashAddr = { 0 };
    U8 ucLunInSuperPu;
    U16 aCurAT0Block[LUN_NUM_PER_SUPERPU];

    FlashAddr.m_PUSer = ucSuperPuNum;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        aCurAT0Block[ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu];
    }
    FlashAddr.m_PageInBlock = pRT->m_RT[ucSuperPuNum].CurAT0PPO;
    FlashAddr.m_LPNInPage = 0;

    if (pRT->m_RT[ucSuperPuNum].CurAT0PPO >= (PG_PER_SLC_BLK - 1))
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            /* last page of current Block: need to change block */
            L2_TB_Allocate_NextFreeBlock(ucSuperPuNum, ucLunInSuperPu);
        }
        pRT->m_RT[ucSuperPuNum].CurAT0PPO = 0;
    }
    else
    {
        /* update CurAT0PPO point to next free page */
        pRT->m_RT[ucSuperPuNum].CurAT0PPO++;
    }

    if (L2_TB_GetPageCntPerCE(PageType) <= usPageIndex)
    {
        DBG_Printf("L2_TB_SaveSetFlashAddr PageType 0x%x usPageIndex 0x%x ERROR\n", PageType, usPageIndex);
        DBG_Getch();
    }

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }

        pRT->m_RT[ucSuperPuNum].PBITPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;

    case PAGE_TYPE_VBT:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }

        pRT->m_RT[ucSuperPuNum].VBTPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;

    case PAGE_TYPE_RPMT:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }
        pRT->m_RT[ucSuperPuNum].RPMTPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }
        pRT->m_RT[ucSuperPuNum].TLCWPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;
#endif

    case PAGE_TYPE_DPBM:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }
        pRT->m_RT[ucSuperPuNum].DPBMPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;

    case PAGE_TYPE_PMTI:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }
        pRT->m_RT[ucSuperPuNum].PMTIPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;

#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][usPageIndex] = aCurAT0Block[ucLunInSuperPu];
        }
        pRT->m_RT[ucSuperPuNum].VBMTPPO[usPageIndex] = FlashAddr.m_PageInBlock;
    }
        break;
#endif

    default:
    {
        DBG_Printf("L2_TB_SaveSetFlashAddr invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }
        break;
    }

    return;
}

BOOL MCU1_DRAM_TEXT L2_TB_IsDataAddrPageAlign(PAGE_TYPE PageType)
{
    BOOL bRet;

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    case PAGE_TYPE_VBT:
    case PAGE_TYPE_PMTI:
        bRet = FALSE;
        break;

    case PAGE_TYPE_RPMT:
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
#endif
    case PAGE_TYPE_DPBM:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
        bRet = TRUE;
        break;

    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}

U32 MCU1_DRAM_TEXT L2_TB_GetLoadDestAddr(PAGE_TYPE PageType, U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex)
{
    if (TRUE == L2_TB_IsDataAddrPageAlign(PageType))
    {
        return L2_TB_GetDataAddr(PageType, ucSuperPuNum, ucLunInSuperPu, usPageIndex);
    }
    else
    {
        return L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
    }
}

U32 MCU1_DRAM_TEXT L2_TB_GetDataAddr(PAGE_TYPE PageType, U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex)
{
    U32 ulDataAddr;
    U32 ulRPMTBufferPointer;

    ulDataAddr = INVALID_8F;

    //RPMT is special, so not check
    if (PAGE_TYPE_RPMT == PageType)
    {
    }
    else if (L2_TB_GetPageCntPerCE(PageType) <= usPageIndex)
    {
        DBG_Printf("L2_TB_SaveUpdateData PageType 0x%x usPageIndex 0x%x ERROR\n", PageType, usPageIndex);
        DBG_Getch();
    }

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        ulDataAddr = (U32)pPBIT[ucSuperPuNum];
        ulDataAddr += (usPageIndex*LUN_NUM_PER_SUPERPU + ucLunInSuperPu)*BUF_SIZE;
    }
        break;

    case PAGE_TYPE_VBT:
    {
        ulDataAddr = (U32)pVBT[ucSuperPuNum];
        ulDataAddr += (usPageIndex*LUN_NUM_PER_SUPERPU + ucLunInSuperPu)*BUF_SIZE;
    }
        break;

    case PAGE_TYPE_RPMT:
    {
        ulRPMTBufferPointer = g_PuInfo[ucSuperPuNum]->m_RPMTBufferPointer[usPageIndex];
        ulDataAddr = (U32)(g_PuInfo[ucSuperPuNum]->m_pRPMT[usPageIndex][ulRPMTBufferPointer]);
        ulDataAddr += ucLunInSuperPu*BUF_SIZE;
    }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
    {
        if(usPageIndex < 3)
        {
           ulDataAddr = (U32)&g_TLCManager->pRPMT[ucSuperPuNum][usPageIndex]->m_RPMT[ucLunInSuperPu];
        }
        else if(usPageIndex == 3)
        {
           ulDataAddr = (U32)g_TLCManager->m_TLCGCSrcAddr[ucSuperPuNum];
           ulDataAddr += ucLunInSuperPu * BUF_SIZE;
        }
#ifdef FLASH_IM_3DTLC_GEN2
        else if(usPageIndex == 4)
        {
           ulDataAddr = (U32)g_TLCManager->m_TLCGCSrcAddr[ucSuperPuNum] + (LUN_NUM_PER_SUPERPU * BUF_SIZE);
           ulDataAddr += ucLunInSuperPu * BUF_SIZE;
        } 
        else if(usPageIndex == 5)
        {
           ulDataAddr = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPuNum][0][ucLunInSuperPu];
        }
#else
        else if(usPageIndex == 4)
        {
           ulDataAddr = (U32)g_TLCManager->ptWriteTLCBuffer[ucSuperPuNum][0][ucLunInSuperPu];
        }
#endif
    }
        break;
#endif

    case PAGE_TYPE_DPBM:
    {
        ulDataAddr = (U32)&(pDPBM->m_LpnMap[ucSuperPuNum][0]);
        ulDataAddr += (usPageIndex*LUN_NUM_PER_SUPERPU + ucLunInSuperPu)*BUF_SIZE;
    }
        break;

    case PAGE_TYPE_PMTI:
    {
        ulDataAddr = (U32)g_PMTI[ucSuperPuNum];
        ulDataAddr += (usPageIndex*LUN_NUM_PER_SUPERPU + ucLunInSuperPu)*BUF_SIZE;
    }
        break;

#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
    {
        ulDataAddr = (U32)g_pulLCTValid;
        ulDataAddr += (usPageIndex*LUN_NUM_PER_SUPERPU + ucLunInSuperPu)*BUF_SIZE;
    }
        break;
#endif

    default:
    {
        DBG_Printf("L2_TB_SaveUpdateData invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }
        break;
    }

    return ulDataAddr;
}

U32 MCU1_DRAM_TEXT L2_TB_GetDataLength(PAGE_TYPE PageType, U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex)
{
    S32 ulLength = 0;

    if (L2_TB_GetPageCntPerCE(PageType) <= usPageIndex)
    {
        DBG_Printf("L2_TB_SaveUpdateData PageType 0x%x usPageIndex 0x%x ERROR\n", PageType, usPageIndex);
        DBG_Getch();
    }

    switch (PageType)
    {
    case PAGE_TYPE_PBIT:
    {
        if (0 == usPageIndex)
        {
            ulLength = (sizeof(PBIT) > BUF_SIZE*ucLunInSuperPu) ? min(sizeof(PBIT) - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
        else if ((usPageIndex > 0) && (usPageIndex < L2_TB_GetPageCntPerCE(PageType)))
        {
            ulLength = sizeof(PBIT) - SUPER_PAGE_SIZE * usPageIndex;
            ulLength = (ulLength > BUF_SIZE*ucLunInSuperPu) ? min(ulLength - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
    }
        break;

    case PAGE_TYPE_VBT:
    {
        if (0 == usPageIndex)
        {
            ulLength = (sizeof(VBT) > BUF_SIZE*ucLunInSuperPu) ? min(sizeof(VBT) - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
        else if ((usPageIndex > 0) && (usPageIndex < L2_TB_GetPageCntPerCE(PageType)))
        {
            ulLength = sizeof(VBT) - SUPER_PAGE_SIZE * usPageIndex;
            ulLength = (ulLength > BUF_SIZE*ucLunInSuperPu) ? min(ulLength - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
    }
        break;

    case PAGE_TYPE_PMTI:
    {
        if (0 == usPageIndex)
        {
            ulLength = (sizeof(PMTI) > BUF_SIZE*ucLunInSuperPu) ? min(sizeof(PMTI) - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
        else if ((usPageIndex > 0) && (usPageIndex < L2_TB_GetPageCntPerCE(PageType)))
        {
            ulLength = sizeof(PMTI) - SUPER_PAGE_SIZE * usPageIndex;
            ulLength = (ulLength > BUF_SIZE*ucLunInSuperPu) ? min(ulLength - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
    }
        break;

    case PAGE_TYPE_DPBM:
    {
        if (0 == usPageIndex)
        {
            ulLength = (sizeof(DPBM_ENTRY)*VIR_BLK_CNT > BUF_SIZE*ucLunInSuperPu) ? min((sizeof(DPBM_ENTRY)*VIR_BLK_CNT), BUF_SIZE) : 0;
        }
        else if ((usPageIndex > 0) && (usPageIndex < L2_TB_GetPageCntPerCE(PageType)))
        {
            ulLength = sizeof(DPBM_ENTRY)*VIR_BLK_CNT - SUPER_PAGE_SIZE * usPageIndex;
            ulLength = (ulLength > BUF_SIZE*ucLunInSuperPu) ? min(ulLength - BUF_SIZE*ucLunInSuperPu, BUF_SIZE) : 0;
        }
    }
        break;

    case PAGE_TYPE_RPMT:
#ifndef LCT_VALID_REMOVED
    case PAGE_TYPE_VBMT:
#endif
    {
        ulLength = SUPER_PAGE_SIZE;
        ulLength = (ulLength > BUF_SIZE*ucLunInSuperPu) ? (min((ulLength - BUF_SIZE*ucLunInSuperPu), BUF_SIZE)) : 0;
    }
        break;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case PAGE_TYPE_TLCW:
    {
        ulLength = BUF_SIZE;
    }
       break;
#endif

    default:
    {
        DBG_Printf("L2_TB_SaveUpdateData invalid PageType 0x%x\n", PageType);
        DBG_Getch();
    }
        break;
    }

    return ulLength;
}

U32 MCU1_DRAM_TEXT L2_TB_GetDRAMBufAddr(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U32 ulBufferAddr;

    ulBufferAddr = COM_GetMemAddrByBufferID((L2AT0StartTempBufID + ucSuperPuNum*LUN_NUM_PER_SUPERPU + ucLunInSuperPu), TRUE, BUF_SIZE_BITS);

    return ulBufferAddr;
}

RED* MCU1_DRAM_TEXT L2_TB_GetRedAddr(PAGE_TYPE PageType, U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    RED* pRed;

    if (PAGE_TYPE_PBIT == PageType)
    {
        pRed = &(pTBRedInfo->PBIT_Red[ucSuperPuNum][ucLunInSuperPu]);
    }
    else
    {
        pRed = &(pTBRedInfo->Data_Red[ucSuperPuNum][ucLunInSuperPu]);
    }

    return pRed;
}

U32 MCU1_DRAM_TEXT L2_TB_SaveSetRed(PAGE_TYPE PageType, U8 ucSuperPuNum, U16 usPageIndex, RED* pRed)
{
    U8  ucLoop;
    U32 ulRet;

    if (NULL == pRed)
    {
        ulRet = L2_TB_FAIL;
    }
    else
    {
        for (ucLoop = 0; ucLoop < RED_SW_SZ_DW; ucLoop++)
        {
            pRed->m_Content[ucLoop] = 0;
        }

        pRed->m_RedComm.ulTimeStamp = L2_GetTimeStampInPu(ucSuperPuNum);
        pRed->m_RedComm.DW1 = INVALID_8F;
        pRed->m_RedComm.bcBlockType = BLOCK_TYPE_AT0;
        pRed->m_RedComm.bcPageType = PageType;
        pRed->m_RedComm.eOPType = L2_TB_GetOPType(PageType);
        pRed->m_PageIndex = (U32)usPageIndex;

        ulRet = L2_TB_SUCCESS;
    }

    return ulRet;
}

U16 MCU1_DRAM_TEXT L2_TB_SaveGetRecycleBlock(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usPos;
    U16 usCurrentPos = INVALID_4F;
    U16 usBlock;
    COMMON_EVENT Event;

    for (usPos = 0; usPos < AT0_BLOCK_COUNT; usPos++)
    {
        if (pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usPos] == pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu])
        {
            usCurrentPos = usPos;
            continue;
        }

        if (FALSE == pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPos])
            continue;

        usBlock = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usPos];

        /* if Save PBIT event,don't need check whether AT0 Table is saved in two blocks */
        CommCheckEvent(COMM_EVENT_OWNER_L2, &Event);
        if (Event.EventSavePbit)
        {
            if (pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][0] == usBlock)
            {
                continue;
            }
            else
            {
                return usBlock;
            }
        }

        if ((pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][0] == usBlock)
#ifndef LCT_VALID_REMOVED
            || (pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][0] == usBlock)
#endif
            || (pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][0] == usBlock)
            || (pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][0] == usBlock)
            || (pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][0] == usBlock)
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            || (pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][0] == usBlock)
#endif            
            || (pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][0] == usBlock))
        {
            continue;
        }
        else
        {
            return usBlock;
        }
    }


    if (usCurrentPos == INVALID_4F)
    {
        DBG_Printf("L2_TB_SaveGetRecycleBlock PU 0x%x LUN 0x%x CurAT0Block 0x%x not found\n",
            ucSuperPuNum, ucLunInSuperPu, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu]);
        DBG_Getch();
    }
    if (FALSE != pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usCurrentPos])
    {
        DBG_Printf("L2_TB_SaveGetRecycleBlock PU 0x%x LUN 0x%x CurAT0Block 0x%x Wrong EraseFlag\n",
            ucSuperPuNum, ucLunInSuperPu, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu]);
        DBG_Getch();
    }
    return INVALID_4F;
}

void MCU1_DRAM_TEXT L2_TB_SaveBlockRecycled(U8 ucSuperPuNum, U8 ucLunInSuperPu)
{
    U16 usPos;
    U16 usBlock;
    COMMON_EVENT Event;

    for (usPos = 0; usPos < AT0_BLOCK_COUNT; usPos++)
    {
        if (FALSE == pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPos])
            continue;

        usBlock = pRT->m_RT[ucSuperPuNum].AT0BlockAddr[ucLunInSuperPu][usPos];

        /* if Save PBIT event,don't need check whether AT0 Table is saved in two blocks */
        CommCheckEvent(COMM_EVENT_OWNER_L2, &Event);
        if (Event.EventSavePbit)
        {
            pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPos] = FALSE;
            return;
        }

        if ((pRT->m_RT[ucSuperPuNum].PMTIBlock[ucLunInSuperPu][0] == usBlock)
#ifndef LCT_VALID_REMOVED
            || (pRT->m_RT[ucSuperPuNum].VBMTBlock[ucLunInSuperPu][0] == usBlock)
#endif
            || (pRT->m_RT[ucSuperPuNum].VBTBlock[ucLunInSuperPu][0] == usBlock)
            || (pRT->m_RT[ucSuperPuNum].PBITBlock[ucLunInSuperPu][0] == usBlock)
            || (pRT->m_RT[ucSuperPuNum].DPBMBlock[ucLunInSuperPu][0] == usBlock)
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            || (pRT->m_RT[ucSuperPuNum].TLCWBlock[ucLunInSuperPu][0] == usBlock)
#endif            
            || (pRT->m_RT[ucSuperPuNum].RPMTBlock[ucLunInSuperPu][0] == usBlock))
        {
            continue;
        }
        else
        {
            pRT->m_RT[ucSuperPuNum].AT0BlockEraseFlag[ucLunInSuperPu][usPos] = FALSE;
            return;
        }
    }

    if (usPos == AT0_BLOCK_COUNT)
    {
        DBG_Printf("L2_TB_SaveBlockRecycled PU 0x%x LUN 0x%x erased block not found\n",
            ucSuperPuNum, ucLunInSuperPu, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu]);
        DBG_Getch();
    }
}

void MCU1_DRAM_TEXT L2_TB_SpecialHandlingRPMTIndex(PAGE_TYPE PageType, U16* PageIndex)
{
    if (PAGE_TYPE_RPMT == PageType && TARGET_TLC_WRITE == *PageIndex)
    {
        //TLC_Write Target need not load RPMT
        *PageIndex = *PageIndex + 1;
    }
}


U32 MCU1_DRAM_TEXT L2_TB_LoadTableBlock(U8 ucSuperPuNum, PAGE_TYPE PageType)
{
    U8  ucLunInSuperPu;
    U32 ulRet;
    U32 ulDataAddr;
    U32 ulLength;
#ifdef LOAD_AT0_ACCELERATE
    U32 ulLoadDesAddr;
#endif
    U32 ulBufferAddr;
    U8  ucFlashStatus;
    RED *pRed;
    L2_TB_LOAD_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->LoadStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
    case L2_TB_LOAD_START:
    {
        /* Init global values */
        pTBManagement->usPageIndex[ucSuperPuNum] = 0;
        pTBManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
        *pStatus = L2_TB_LOAD_READ_PAGE;
    }
        break;

    case L2_TB_LOAD_READ_PAGE:
    {
        /* read Table pages from Flash  */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* get flashaddr, temp red and data buffer and send read request to L3 */
                L2_TB_LoadGetFlashAddr(PageType, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum], &FlashAddr);
                pRed = L2_TB_GetRedAddr(PageType, ucSuperPuNum, ucLunInSuperPu);
#ifndef LOAD_AT0_ACCELERATE
                ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
#else
                ulLoadDesAddr = L2_TB_GetLoadDestAddr(PageType, ucSuperPuNum, ucLunInSuperPu,  pTBManagement->usPageIndex[ucSuperPuNum]);
#endif
                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

#ifndef LOAD_AT0_ACCELERATE
                L2_FtlReadLocal((U32 *)ulBufferAddr, &FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                    (U32 *)pRed, LPN_PER_BUF, 0, TRUE, TRUE);
#else
                L2_FtlReadLocal((U32 *)ulLoadDesAddr, &FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]),
                    (U32 *)pRed, LPN_PER_BUF, 0, TRUE, TRUE);
#endif

                //FIRMWARE_LogInfo("L2_TB_LoadTableBlock PageType %d PageIndex %d SuperPu %d ucLunInSuperPu %d  Blk 0x%x Pg 0x%x pRed 0x%x\n",
                //    PageType, pTBManagement->usPageIndex[ucSuperPuNum], ucSuperPuNum, ucLunInSuperPu, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, pRed);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_LOAD_WAIT_PAGE;
        }
    }
        break;

    case L2_TB_LOAD_WAIT_PAGE:
    {
        /* wait for flash idle */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PageType, ucSuperPuNum, ucLunInSuperPu);

                if (pRed->m_RedComm.bcPageType != (U8)PageType)
                {
                    /* load TB read page PageType wrong */
                    *pStatus = L2_TB_LOAD_FAIL;
                }
                else
                {
#ifndef LOAD_AT0_ACCELERATE
                    /* copy data to Table */
                    ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
                    ulDataAddr = L2_TB_GetDataAddr(PageType, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum]);
                    ulLength = L2_TB_GetDataLength(PageType, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum]);
                    COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength / 4);

                    /*if (PageType == PAGE_TYPE_VBMT)
                    {
                        DBG_Printf("SPU %d LUN %d usPageIndex %d ulDataAddr 0x%x\n",ucSuperPuNum,ucLunInSuperPu,pTBManagement->usPageIndex[ucSuperPuNum],ulDataAddr);
                    }*/
#else
                    if (FALSE == L2_TB_IsDataAddrPageAlign(PageType))
                    {
                        /* copy data to Table */
                        ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
                        ulDataAddr = L2_TB_GetDataAddr(PageType, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum]);
                        ulLength = L2_TB_GetDataLength(PageType, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum]);
                        COM_MemCpy((U32*)ulDataAddr, (U32*)ulBufferAddr, ulLength/4);
                    }
#endif
                }
            }
            else
            {
                /* load TB read page UECC or EMPTY_PG */
                *pStatus = L2_TB_LOAD_FAIL;
            }
        }

        // All Luns are done.
        if (L2_TB_LOAD_WAIT_PAGE == *pStatus)
        {
            /* update params for next time read or finish */
            pTBManagement->usPageIndex[ucSuperPuNum]++;

            //RPMT special process, for ACC & TLC_W target have no RPMT
            L2_TB_SpecialHandlingRPMTIndex(PageType, &pTBManagement->usPageIndex[ucSuperPuNum]);
            if (pTBManagement->usPageIndex[ucSuperPuNum] >= L2_TB_GetPageCntPerCE(PageType))
            {
                if (PAGE_TYPE_PBIT == PageType)
                {
                    L2_PBIT_Load_ResumeData(ucSuperPuNum);
                }
                #if 0
                else if (PAGE_TYPE_VBT == PageType)
                {
                    if (INVALID_4F != g_GCManager->tGCCommon.m_SrcPBN[ucSuperPuNum])
                    {
                        if (TRUE == L2_VBT_Get_TLC(ucSuperPuNum, g_GCManager->tGCCommon.m_SrcPBN[ucSuperPuNum]))
                        {
                            g_GCManager->tGCCommon.m_ucSrcBlkType[ucSuperPuNum] = VBT_TYPE_TLCW;
                        }
                        else
                        {
                            g_GCManager->tGCCommon.m_ucSrcBlkType[ucSuperPuNum] = VBT_TYPE_HOST;
                        }
                    }
                }
                #endif
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                else if (PAGE_TYPE_TLCW == PageType)
                {
                    L2_TLCSharedPageClosedLoad(ucSuperPuNum);

                    if (g_TLCManager->aeTLCWriteType[ucSuperPuNum] != TLC_WRITE_ALL)
                    {
                        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                        {
                            if (g_TLCManager->pRPMT[ucSuperPuNum][0]->m_RPMT[ucLunInSuperPu].m_SuperPageTS[0] !=
                            L2_TB_CheckGetTimeStamp(ucSuperPuNum, ucLunInSuperPu, TARGET_TLC_WRITE))
                            {
                                *pStatus = L2_TB_LOAD_FAIL;
                                return L2_TB_FAIL;
                            }
                        }
                    }
                }
#endif        

                *pStatus = L2_TB_LOAD_SUCCESS;
            }
            else
            {
                // Reset
                pTBManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
                *pStatus = L2_TB_LOAD_READ_PAGE;
            }
        }
    }
        break;

    case L2_TB_LOAD_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_LOAD_SUCCESS:
    {
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_LoadTableBlock invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

BOOL MCU1_DRAM_TEXT L2_TB_CheckIsTargetAddrValid(U8 ucSuperPuNum)
{
    BOOL bRet = TRUE;
    U8 ucWriteType;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    for (ucWriteType = 0; ucWriteType < TARGET_ALL; ucWriteType++)
#else
    for (ucWriteType = 0; ucWriteType < TARGET_TLC_WRITE; ucWriteType++)
#endif
    {
        /*Skip Check RndTarget if no RndTarget exist*/
        if ((TARGET_HOST_GC == ucWriteType) && (INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetBlock[ucWriteType]) && (INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetPPO[ucWriteType]))
        {
            continue;
        }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        if ((TARGET_TLC_WRITE == ucWriteType) && (INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetBlock[ucWriteType]) &&
            ((INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetPPO[ucWriteType]) || (0 == pPBIT[ucSuperPuNum]->m_TargetPPO[ucWriteType])))
        {
            continue;
        }
#endif


#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        if (((pPBIT[ucSuperPuNum]->m_TargetBlock[ucWriteType] >= (BLK_PER_LUN + RSVD_BLK_PER_LUN)) || (pPBIT[ucSuperPuNum]->m_TargetPPO[ucWriteType] > PG_PER_TLC_BLK))
#else
        if ((pPBIT[ucSuperPuNum]->m_TargetBlock[ucWriteType] >= VIR_BLK_CNT) || (pPBIT[ucSuperPuNum]->m_TargetPPO[ucWriteType] > PG_PER_SLC_BLK)
#endif
            || (pPBIT[ucSuperPuNum]->m_TargetOffsetBitMap[ucWriteType] >= SUPERPU_LUN_NUM_BITMSK))
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

U32 MCU1_DRAM_TEXT L2_TB_CheckGetFlashAddr(L2_TB_CHECK_STATUS Status, U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex, PhysicalAddr* FlashAddr)
{
    U16 usBlock;
    U16 usPage;
    U32 ulTargetOffsetBitMap;
    U32 ulRet;

    ulRet = L2_TB_SUCCESS;

    FlashAddr->m_PPN = 0;

    if (usPageIndex == TARGET_HOST_WRITE_NORAML)
    {
        usBlock = pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_HOST_WRITE_NORAML];
        usPage = pPBIT[ucSuperPuNum]->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
        ulTargetOffsetBitMap = pPBIT[ucSuperPuNum]->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML];
    }
    else if (usPageIndex == TARGET_HOST_GC)
    {
        usBlock = pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_HOST_GC];
        usPage = pPBIT[ucSuperPuNum]->m_TargetPPO[TARGET_HOST_GC];
        ulTargetOffsetBitMap = pPBIT[ucSuperPuNum]->m_TargetOffsetBitMap[TARGET_HOST_GC];
    }
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    else if (usPageIndex == TARGET_TLC_WRITE)
    {
        if (pPBIT[ucSuperPuNum]->m_WLDstBlk != INVALID_4F)
            usBlock = pPBIT[ucSuperPuNum]->m_WLDstBlk;
        else
            usBlock = pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_TLC_WRITE];
        
		usPage = pPBIT[ucSuperPuNum]->m_TargetPPO[TARGET_TLC_WRITE];
		ulTargetOffsetBitMap = pPBIT[ucSuperPuNum]->m_TargetOffsetBitMap[TARGET_TLC_WRITE];
    }
#endif

    switch (Status)
    {
    case L2_TB_CHECK_READ_FIRST_PAGE:
    {
        // If FIRST page is a whole page, checked here.
        // otherwise, this page is a LAST page or PPO page.
        if (0 == usPage)
        {
            ulRet = L2_TB_FAIL;
        }
        else
        {
            if ((1 == usPage) && (0 != ulTargetOffsetBitMap))
            {
                if (ulTargetOffsetBitMap & (1 << ucLunInSuperPu))
                {
                    FlashAddr->m_PUSer = ucSuperPuNum;
                    FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
                    FlashAddr->m_BlockInPU = usBlock;
                    FlashAddr->m_PageInBlock = 0;
                    FlashAddr->m_LPNInPage = 0;
                }
                else
                {
                    ulRet = L2_TB_FAIL;
                }
            }
            else
            {
                FlashAddr->m_PUSer = ucSuperPuNum;
                FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr->m_BlockInPU = usBlock;
                FlashAddr->m_PageInBlock = 0;
                FlashAddr->m_LPNInPage = 0;
            }
        }
    }
        break;

    case L2_TB_CHECK_READ_LAST_PAGE:
    {
        if (0 == usPage)
        {
            ulRet = L2_TB_FAIL;
        }
        else
        {
            // Last page is a whole page.
            if (0 == ulTargetOffsetBitMap)
            {
                FlashAddr->m_PUSer = ucSuperPuNum;
                FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr->m_BlockInPU = usBlock;
                FlashAddr->m_PageInBlock = usPage - 1;
                FlashAddr->m_LPNInPage = 0;
            }
            else
            {
                // If the page is a partial page, it's regarded as the LAST page.
                if (ulTargetOffsetBitMap & (1 << ucLunInSuperPu))
                {
                    FlashAddr->m_PUSer = ucSuperPuNum;
                    FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
                    FlashAddr->m_BlockInPU = usBlock;
                    FlashAddr->m_PageInBlock = usPage - 1;
                    FlashAddr->m_LPNInPage = 0;
                }
                else
                {
                    ulRet = L2_TB_FAIL;
                }
            }
        }
    }
        break;

    case L2_TB_CHECK_READ_PPO_PAGE:
    {
        
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        if (((usPageIndex  < TARGET_TLC_WRITE) && (PG_PER_SLC_BLK == usPage)) ||
            ((usPageIndex == TARGET_TLC_WRITE) && (PG_PER_TLC_BLK == usPage)))
#else        
        if (PG_PER_SLC_BLK == usPage)
#endif
        {
            ulRet = L2_TB_FAIL;
        }
        else
        {
            if (0 != ulTargetOffsetBitMap)
            {
                if (0 != (ulTargetOffsetBitMap & (1 << ucLunInSuperPu)))
                {
                    FlashAddr->m_PageInBlock = usPage;
                }
                else
                {
                    FlashAddr->m_PageInBlock = usPage - 1;
                }
            }
            else
            {
                FlashAddr->m_PageInBlock = usPage;
            }

            FlashAddr->m_PUSer = ucSuperPuNum;
            FlashAddr->m_OffsetInSuperPage = ucLunInSuperPu;
            FlashAddr->m_BlockInPU = usBlock;
            FlashAddr->m_LPNInPage = 0;

            // Check PMT if none page is written.
            if (0 == usPage && 0 == ulTargetOffsetBitMap)
            {
                pTBManagement->ucNeedCheckPMTTS[ucSuperPuNum] = TRUE;
            }
        }
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_CheckGetFlashAddr invalid Status 0x%x\n", Status);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_TB_CheckGetTimeStamp(U8 ucSuperPuNum, U8 ucLunInSuperPu, U16 usPageIndex)
{
    U16 usVirBlock;
    U16 usPhyBlock;
    U32 ulTimeStamp;

    ulTimeStamp = INVALID_8F;

    if (usPageIndex == TARGET_HOST_WRITE_NORAML)
    {
        usVirBlock = pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_HOST_WRITE_NORAML];
    }
    else if (usPageIndex == TARGET_HOST_GC)
    {
        usVirBlock = pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_HOST_GC];
    }
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    else if (usPageIndex == TARGET_TLC_WRITE)
    {
        if (pPBIT[ucSuperPuNum]->m_WLDstBlk != INVALID_4F)
            usVirBlock = pPBIT[ucSuperPuNum]->m_WLDstBlk;
        else
        usVirBlock = pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_TLC_WRITE];
    }
#endif

    usPhyBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, ucLunInSuperPu, usVirBlock);
    ulTimeStamp = pPBIT[ucSuperPuNum]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].TimeStamp;

    return ulTimeStamp;
}


BLOCK_TYPE MCU1_DRAM_TEXT L2_TB_CheckGetBlockType(U16 usPageIndex)
{
    BLOCK_TYPE BlockType;

    if (usPageIndex == TARGET_HOST_WRITE_NORAML)
    {
        BlockType = BLOCK_TYPE_SEQ;
    }
    else if (usPageIndex == TARGET_HOST_GC)
    {
        BlockType = BLOCK_TYPE_RAN;
    }
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    else if (usPageIndex == TARGET_TLC_WRITE)
    {
        BlockType = BLOCK_TYPE_TLC_W;
    }
#endif

    return BlockType;
}
/*****************************************************************************
 Prototype      : L2_TB_BootCheckDataBlk
 Description    : L2_TB_BootCheckDataBlk
 Input          : U8 ucSuperPuNum  
 Output         : None
 Return Value   : U32
 Calls          : 
 Called By      : 
 
 History        :
 1.Date         : 2016/8/12
   Author       : Nina Yang
   Modification : Created function

   This function is used to check a corner case to avoid abnormal shutdown boot check to normal boot,it may cause error.
   for example:
   1.LLF,PBIT save SeqTarget Blk and PPO,ie target block=VirBlk0x4ee(PhyBlk0x10) PPO = 0
   2.Host write 3 pages,target block=VirBlk0x4ee(PhyBlk0x10) PPO = 4,abnormal shutdown
   3.table rebuild will move blk,if VirBlk0x4ee(PhyBlk0x10) PPO = 4 move to VirBlk0x4f1(PhyBlk0x13),then erase VirBlk 0x4ee
     and abnormal shutdown
   4.power on,boot check will read target Blk VirBlk 0x4ee,PPO = 0,because it is saved in PBIT when LLF
     this case RT TS is largest if don't check other data blk.it will porcess as normal boot.

*****************************************************************************/
U32 MCU1_DRAM_TEXT L2_TB_BootCheckDataBlk(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu;
    U8  ucTimeStampFlag;
    U32 ulRet;
    U16 usBlock;
    U16 usCurPBN;
    PhysicalAddr FlashAddr = { 0 };
    U8  ucFlashStatus;
    RED *pRed;
    L2_TB_CHECK_DATA_BLK_STATUS *pCheckDataBlkStatus;

    ulRet = L2_TB_WAIT;
    pCheckDataBlkStatus = &(pTBManagement->CheckDataBlkStatus[ucSuperPuNum]);

    switch (*pCheckDataBlkStatus)
    {
    case L2_TB_CHECK_DATA_BLK_START:

        /* locate SLC block Start,it use VirBlk */
        pTBManagement->usBlockIndex[ucSuperPuNum][0] = TLC_BLK_CNT;

        /* Skip VBN no PBN mapping BLK */
        do
        {
            usCurPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, 0, pTBManagement->usBlockIndex[ucSuperPuNum][0]);
            pTBManagement->usBlockIndex[ucSuperPuNum][0]++;
        } while (INVALID_4F == usCurPBN);

        pTBManagement->usBlockIndex[ucSuperPuNum][0]--;
        pTBManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
        *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_FIRST_PAGE;

        break;
    case L2_TB_CHECK_DATA_BLK_READ_FIRST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBManagement->usBlockIndex[ucSuperPuNum][0];

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* update page index to 1st page */
                pTBManagement->usPageIndex[ucSuperPuNum] = 0;

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = 0;
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, TRUE);

                //FIRMWARE_LogInfo("L2_TB_CHECK_DATA_BLK_READ_FIRST_PAGE PU %d Blk %d Page %d Spare 0x%x FlashStatus 0x%x.\n", FlashAddr.m_PUSer, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, (U32)pRed, &pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pCheckDataBlkStatus = L2_TB_CHECK_PMT_WAIT_FIRST_PAGE;
        }
    }
        break;

    case L2_TB_CHECK_DATA_BLK_WAIT_FIRST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        ucTimeStampFlag = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_DATA) || (pRed->m_RedComm.bcBlockType > BLOCK_TYPE_TLC_W))
                {
                    DBG_Printf("%s PageType %d BlockType %d error!\n", __FUNCTION__, pRed->m_RedComm.bcPageType, pRed->m_RedComm.bcBlockType);
                    DBG_Getch();
                }
                else
                {
                    /* check 1st page TS with RT */
                    if (pRed->m_RedComm.ulTimeStamp > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        ucTimeStampFlag = FALSE;
                    }
                }
            }
            else
            {
                /* DATA block 1st page read UECC or EMPTY_PG */
                *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE;
            }
        }

        if (FALSE == ucTimeStampFlag)
        {
            *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_FAIL;
        }
        else
        {
            pTBManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            if (L2_TB_CHECK_DATA_BLK_WAIT_FIRST_PAGE == *pCheckDataBlkStatus)
            {
                *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_LAST_PAGE;
            }
            else if (L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE == *pCheckDataBlkStatus)
            {
                pTBManagement->usPageIndex[ucSuperPuNum]++;
            }
            else
            {
                DBG_Getch();
            }
        }
    }
        break;
    case L2_TB_CHECK_DATA_BLK_READ_LAST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBManagement->usBlockIndex[ucSuperPuNum][0];

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* update page index to 1st page */
                pTBManagement->usPageIndex[ucSuperPuNum] = 0;

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = (PG_PER_SLC_BLK - 1);
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, TRUE);

                //FIRMWARE_LogInfo("L2_TB_CHECK_DATA_BLK_READ_LAST_PAGE PU %d Blk %d Page %d Spare 0x%x FlashStatus 0x%x.\n", FlashAddr.m_PUSer, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, (U32)pRed, &pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_WAIT_LAST_PAGE;
        }
    }
        break;
    case L2_TB_CHECK_DATA_BLK_WAIT_LAST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        ucTimeStampFlag = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_RPMT) || (pRed->m_RedComm.bcBlockType > BLOCK_TYPE_TLC_W))
                {
                    DBG_Printf("%s PageType %d BlockType %d error!\n", __FUNCTION__, pRed->m_RedComm.bcPageType, pRed->m_RedComm.bcBlockType);
                    DBG_Getch();
                }
                else
                {
                    /* check last page TS with RT */
                    if (pRed->m_RedComm.ulTimeStamp > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        ucTimeStampFlag = FALSE;
                    }
                }
            }
            else
            {
                /* data block last page read UECC or EMPTY_PG */
                *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE;
            }
        }

        if (FALSE == ucTimeStampFlag)
        {
            *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_FAIL;
        }
        else
        {
            pTBManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;
            if (L2_TB_CHECK_PMT_WAIT_LAST_PAGE == *pCheckDataBlkStatus)
            {
                do
                {
                    pTBManagement->usBlockIndex[ucSuperPuNum][0]++;
                    usCurPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, 0, pTBManagement->usBlockIndex[ucSuperPuNum][0]);
                } while ((INVALID_4F == usCurPBN) && (pTBManagement->usBlockIndex[ucSuperPuNum][0] < VIR_BLK_CNT));

                /*all SLC Data blk scan done */
                if (pTBManagement->usBlockIndex[ucSuperPuNum][0] >= VIR_BLK_CNT)
                {
                    *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_SUCCESS;
                }
                else
                {
                    *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_FIRST_PAGE;
                }
            }
            else if (L2_TB_CHECK_PMT_READ_NEXT_PAGE == *pCheckDataBlkStatus)
            {
                pTBManagement->usPageIndex[ucSuperPuNum] = 1; // Read from second page.
            }
            else
            {
                DBG_Getch();
            }
        }
    }

        break;
    case L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (pTBManagement->ulLunLoadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBManagement->usBlockIndex[ucSuperPuNum][0];

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunLoadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = pTBManagement->usPageIndex[ucSuperPuNum];
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, TRUE);

                //FIRMWARE_LogInfo("L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE PU %d Blk %d Page %d Spare 0x%x FlashStatus 0x%x.\n", FlashAddr.m_PUSer, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, (U32)pRed, &pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunLoadBitmap[ucSuperPuNum]))
        {
            *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_WAIT_NEXT_PAGE;
        }
    }
        break;
    case L2_TB_CHECK_DATA_BLK_WAIT_NEXT_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        ucTimeStampFlag = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_DATA) || (pRed->m_RedComm.bcBlockType > BLOCK_TYPE_TLC_W))
                {
                    DBG_Printf("%s PageType %d BlockType %d error!\n", __FUNCTION__, pRed->m_RedComm.bcPageType, pRed->m_RedComm.bcBlockType);
                    DBG_Getch();
                }
                else
                {
                    /* check last page TS with RT MaxTS */
                    if (pRed->m_RedComm.ulTimeStamp > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        ucTimeStampFlag = FALSE;
                    }
                    else
                    {
                        *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE;
                    }
                }
            }
            else if (SUBSYSTEM_STATUS_FAIL == ucFlashStatus)
            {
                *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE;
            }
        }

        if (FALSE == ucTimeStampFlag)
        {
            *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_FAIL;
        }
        else
        {
            pTBManagement->ulLunLoadBitmap[ucSuperPuNum] = 0;

            //need double check
            if (L2_TB_CHECK_DATA_BLK_READ_NEXT_PAGE == *pCheckDataBlkStatus)
            {
                pTBManagement->usPageIndex[ucSuperPuNum]++;
                if (pTBManagement->usPageIndex[ucSuperPuNum] >= PG_PER_SLC_BLK - 1)
                {
                    do
                    {
                        pTBManagement->usBlockIndex[ucSuperPuNum][0]++;
                        usCurPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, 0, pTBManagement->usBlockIndex[ucSuperPuNum][0]);
                    } while ((INVALID_4F == usCurPBN) && (pTBManagement->usBlockIndex[ucSuperPuNum][0] < VIR_BLK_CNT));

                    /*all SLC Data blk scan done */
                    if (pTBManagement->usBlockIndex[ucSuperPuNum][0] >= VIR_BLK_CNT)
                    {
                        *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_SUCCESS;
                    }
                    else
                    {
                        *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_FIRST_PAGE;
                    }
                }
            }
            else if (L2_TB_CHECK_DATA_BLK_WAIT_NEXT_PAGE == *pCheckDataBlkStatus)
            {
                /*Next Page EmptyPage,means Page0 & Pg1 all empty, it mostly is a free blk, skip to read next blk */
                do
                {
                    pTBManagement->usBlockIndex[ucSuperPuNum][0]++;
                    usCurPBN = L2_VBT_GetPhysicalBlockAddr(ucSuperPuNum, 0, pTBManagement->usBlockIndex[ucSuperPuNum][0]);
                } while ((INVALID_4F == usCurPBN) && (pTBManagement->usBlockIndex[ucSuperPuNum][0] < VIR_BLK_CNT));

                /*all SLC Data blk scan done */
                if (pTBManagement->usBlockIndex[ucSuperPuNum][0] >= VIR_BLK_CNT)
                {
                    *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_SUCCESS;
                }
                else
                {
                    *pCheckDataBlkStatus = L2_TB_CHECK_DATA_BLK_READ_FIRST_PAGE;
                }
            }
            else
            {
                DBG_Getch();
            }
        }
    }

        break;

    case L2_TB_CHECK_DATA_BLK_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;
    case L2_TB_CHECK_DATA_BLK_SUCCESS:
    {
        ulRet = L2_TB_SUCCESS;
    }
        break;
    default:
    {
        DBG_Printf("%s invalid Status 0x%x\n", __FUNCTION__, *pCheckDataBlkStatus);
        DBG_Getch();
    }
        break;
    }

    return  ulRet;
}

U32 MCU1_DRAM_TEXT L2_TB_BootCheckPMT(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu;
    U8  ucTimeStampFlag;
    U32 ulRet;
    U16 usBlock;
    PhysicalAddr FlashAddr = { 0 };
    U8  ucFlashStatus;
    RED *pRed;
    L2_TB_CHECK_PMT_STATUS *pCheckPMTStatus;
    static MCU12_VAR_ATTR U16 AT1BlockIndex[SUBSYSTEM_SUPERPU_MAX] = { 0 };
    U16* pAT1BlockIndex;


    ulRet = L2_TB_WAIT;
    pCheckPMTStatus = &(pTBManagement->CheckPMTStatus[ucSuperPuNum]);
    pAT1BlockIndex = &(AT1BlockIndex[ucSuperPuNum]);
    switch (*pCheckPMTStatus)
    {
    case L2_TB_CHECK_PMT_START:
    {
        *pAT1BlockIndex = 0;

        /* locate PMT base block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][*pAT1BlockIndex];
        }

        pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] = 0;
        *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_FIRST_PAGE;
    }
        break;

    case L2_TB_CHECK_PMT_READ_FIRST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Loaded
            if (pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* update page index to 1st page */
                pTBManagement->usPageIndex[ucSuperPuNum] = 0;

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = 0;
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

                L2_TableLoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);
                
                //FIRMWARE_LogInfo("L2_TB_CHECK_PMT_READ_FIRST_PAGE PU %d Blk %d Page %d Spare 0x%x FlashStatus 0x%x.\n", FlashAddr.m_PUSer, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, (U32)pRed, &pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]);

                //*pCheckPMTStatus = L2_TB_CHECK_PMT_WAIT_FIRST_PAGE;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum]))
        {
            *pCheckPMTStatus = L2_TB_CHECK_PMT_WAIT_FIRST_PAGE;
        }

    }
        break;

    case L2_TB_CHECK_PMT_WAIT_FIRST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        ucTimeStampFlag = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_PMT) || (pRed->m_RedComm.bcBlockType != BLOCK_TYPE_PMT))
                {
                    if (pRed->m_RedComm.bcPageType == (U8)PAGE_TYPE_DUMMY)
                    {
                        /* fullrecovery may write dummy AT1 block 1st page, not need getch go to read next page because it's 2nd page is empty */
                        *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_NEXT_PAGE;
                    }
                    else
                    {
                        DBG_Printf("L2_TB_BootCheckPMT PageType %d BlockType %d error!\n",pRed->m_RedComm.bcPageType, pRed->m_RedComm.bcBlockType);
                        DBG_Getch();
                    }
                }
                else
                {
                    /* check 1st page TS with RT */
                    if (pRed->m_RedComm.ulTimeStamp > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        ucTimeStampFlag = FALSE;
                        //*pCheckPMTStatus = L2_TB_CHECK_PMT_FAIL;
                    }
                }
            }
            else
            {
                /* PMT block 1st page read UECC or EMPTY_PG */
                *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_NEXT_PAGE;
            }
        }

        if (FALSE == ucTimeStampFlag)
        {
            *pCheckPMTStatus = L2_TB_CHECK_PMT_FAIL;
        }
        else
        {
            pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] = 0;
            if (L2_TB_CHECK_PMT_WAIT_FIRST_PAGE == *pCheckPMTStatus)
            {
                *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_LAST_PAGE;
            }
            else if (L2_TB_CHECK_PMT_READ_NEXT_PAGE == *pCheckPMTStatus)
            {
                pTBManagement->usPageIndex[ucSuperPuNum]++;
            }
            else
            {
                DBG_Getch();
            }
        }
    }
        break;

    case L2_TB_CHECK_PMT_READ_LAST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Loaded
            if (pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            usBlock = pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* update page index to last page */
                pTBManagement->usPageIndex[ucSuperPuNum] = (PG_PER_SLC_BLK - 1);

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = (PG_PER_SLC_BLK - 1);
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

                L2_TableLoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);

                //FIRMWARE_LogInfo("L2_TB_CHECK_PMT_READ_LAST_PAGE PU %d Blk %d Page %d Spare 0x%x.\n", FlashAddr.m_PUSer, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, (U32)pRed);

                //*pCheckPMTStatus = L2_TB_CHECK_PMT_WAIT_LAST_PAGE;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum]))
        {
            *pCheckPMTStatus = L2_TB_CHECK_PMT_WAIT_LAST_PAGE;
        }
    }
        break;

    case L2_TB_CHECK_PMT_WAIT_LAST_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        ucTimeStampFlag = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_PMT) || (pRed->m_RedComm.bcBlockType != BLOCK_TYPE_PMT))
                {
                    /* fullrecovery and rebuild PMT may write dummy AT1 block last page*/
                    if (pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_DUMMY)
                    {
                        //DBG_Printf("L2_TB_BootCheckPMT PageType %d BlockType %d error!\n",pRed->m_RedComm.bcPageType, pRed->m_RedComm.bcBlockType);
                        DBG_Getch();
                    }
                }
                else
                {
                    /* check last page TS with RT */
                    if (pRed->m_RedComm.ulTimeStamp > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        ucTimeStampFlag = FALSE;
                        //*pCheckPMTStatus = L2_TB_CHECK_PMT_FAIL;
                    }
                }
            }
            else
            {
                /* PMT block last page read UECC or EMPTY_PG */
                *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_NEXT_PAGE;
            }
        }

        if (FALSE == ucTimeStampFlag)
        {
            *pCheckPMTStatus = L2_TB_CHECK_PMT_FAIL;
        }
        else
        {
            pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] = 0;
            if (L2_TB_CHECK_PMT_WAIT_LAST_PAGE == *pCheckPMTStatus)
            {
                (*pAT1BlockIndex)++;
                if (*pAT1BlockIndex >= AT1_BLOCK_COUNT)
                {
                    *pCheckPMTStatus = L2_TB_CHECK_PMT_SUCCESS;
                }
                else
                {
                    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                    {
                        pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][*pAT1BlockIndex];
                    }
                    *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_FIRST_PAGE;
                }
            }
            else if (L2_TB_CHECK_PMT_READ_NEXT_PAGE == *pCheckPMTStatus)
            {
                pTBManagement->usPageIndex[ucSuperPuNum] = 1;   // Read from second page.
            }
            else
            {
                DBG_Getch();
            }
        }
    }
        break;

    case L2_TB_CHECK_PMT_READ_NEXT_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Loaded
            if (pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            /* update page index to next page */
            usBlock = pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu];

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* get flashaddr, temp red and send read request to L3 */
                FlashAddr.m_PUSer = ucSuperPuNum;
                FlashAddr.m_OffsetInSuperPage = ucLunInSuperPu;
                FlashAddr.m_BlockInPU = usBlock;
                FlashAddr.m_PageInBlock = pTBManagement->usPageIndex[ucSuperPuNum];
                FlashAddr.m_LPNInPage = 0;

                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

                L2_TableLoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed);

                //FIRMWARE_LogInfo("L2_TB_CHECK_PMT_READ_NEXT_PAGE PU %d Blk %d Page %d Spare 0x%x.\n", FlashAddr.m_PUSer, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock, (U32)pRed);
                // *pCheckPMTStatus = L2_TB_CHECK_PMT_WAIT_NEXT_PAGE;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum]))
        {
            *pCheckPMTStatus = L2_TB_CHECK_PMT_WAIT_NEXT_PAGE;
        }
    }
        break;

    case L2_TB_CHECK_PMT_WAIT_NEXT_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        ucTimeStampFlag = TRUE;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_PMT) || (pRed->m_RedComm.bcBlockType != BLOCK_TYPE_PMT))
                {
                    if (pRed->m_RedComm.bcPageType == (U8)PAGE_TYPE_DUMMY)
                    {
                        /* fullrecovery and rebuild PMT may write dummy at any AT1 block next page*/
                        *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_NEXT_PAGE;
                    }
                    else
                    {
                        DBG_Printf("L2_TB_BootCheckPMT PageType %d BlockType %d error!\n",pRed->m_RedComm.bcPageType, pRed->m_RedComm.bcBlockType);
                        DBG_Getch();
                    }
                }
                else
                {
                    /* check last page TS with RT MaxTS */
                    if (pRed->m_RedComm.ulTimeStamp > pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        ucTimeStampFlag = FALSE;
                    }
                    else
                    {
                        *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_NEXT_PAGE;
                    }
                }
            }
            else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
            {
                *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_NEXT_PAGE;
            }
        }

        if (FALSE == ucTimeStampFlag)
        {
            *pCheckPMTStatus = L2_TB_CHECK_PMT_FAIL;
        }
        else
        {
            pTBManagement->ulLunBootCheckPMTBitmap[ucSuperPuNum] = 0;
            if (L2_TB_CHECK_PMT_READ_NEXT_PAGE == *pCheckPMTStatus)
            {
                pTBManagement->usPageIndex[ucSuperPuNum]++;
                if (pTBManagement->usPageIndex[ucSuperPuNum] >= PG_PER_SLC_BLK)
                {
                    (*pAT1BlockIndex)++;
                    if (*pAT1BlockIndex >= AT1_BLOCK_COUNT)
                    {
                        *pCheckPMTStatus = L2_TB_CHECK_PMT_SUCCESS;
                    }
                    else
                    {
                        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                        {
                            pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][*pAT1BlockIndex];
                        }
                        pTBManagement->usPageIndex[ucSuperPuNum] = 0;
                    }
                }
            }
            else if (L2_TB_CHECK_PMT_WAIT_NEXT_PAGE == *pCheckPMTStatus)
            {
                /* PMT block next page read UECC or EMPTY_PG,continue read next block */
                (*pAT1BlockIndex)++;
                if (*pAT1BlockIndex >= AT1_BLOCK_COUNT)
                {
                    *pCheckPMTStatus = L2_TB_CHECK_PMT_SUCCESS;
                }
                else
                {
                    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                    {
                        pTBManagement->usBlockIndex[ucSuperPuNum][ucLunInSuperPu] = pRT->m_RT[ucSuperPuNum].AT1BlockAddr[ucLunInSuperPu][*pAT1BlockIndex];
                    }
                    pTBManagement->usPageIndex[ucSuperPuNum] = 0;
                    *pCheckPMTStatus = L2_TB_CHECK_PMT_READ_FIRST_PAGE;
                }
            }
            else
            {
                DBG_Getch();
            }
        }
    }
        break;

    case L2_TB_CHECK_PMT_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_CHECK_PMT_SUCCESS:
    {
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_BootCheckPMT invalid Status 0x%x\n", *pCheckPMTStatus);
        DBG_Getch();
    }
        break;

    }
    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_TB_BootCheck(U8 ucSuperPuNum)
{
    U8  ucLunInSuperPu;
    U32 ulRet;
    U32 ulCmdRet;
    U32 ulBufferAddr;
    U8  ucFlashStatus;
    RED *pRed;
    L2_TB_CHECK_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->CheckStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
    case L2_TB_CHECK_START:
    {
        if (TRUE == L2_TB_CheckIsTargetAddrValid(ucSuperPuNum))
        {
            /* Init global values */
            pTBManagement->usPageIndex[ucSuperPuNum] = TARGET_HOST_WRITE_NORAML;
            pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] = 0;
            pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] = 0;
            *pStatus = L2_TB_CHECK_READ_FIRST_PAGE;
        }
        else
        {
            /* validation check fail - rebuild */
            *pStatus = L2_TB_CHECK_FAIL;
        }
    }
        break;

    case L2_TB_CHECK_READ_FIRST_PAGE:
    {
        /* read 1st page redundant of Seq/Ran Target block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Read
            if (pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (L2_TB_SUCCESS == L2_TB_CheckGetFlashAddr(*pStatus, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum], &FlashAddr))
            {
                if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                    pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                    ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
                    pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                   if (TARGET_TLC_WRITE == pTBManagement->usPageIndex[ucSuperPuNum])
                   {
                       L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, FALSE);
                   }
                   else
#endif
                   {
                       L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, TRUE);
                   }

                    //FIRMWARE_LogInfo("L2_TB_CHECK_READ_FIRST_PAGE LUN %d Blk %d Pg %d\n", FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);
                    //*pStatus = L2_TB_CHECK_WAIT_FIRST_PAGE;
                }
            }
            else
            {
                pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;

                /* Seq/Ran Target has not been written, check PPO page */
                //*pStatus = L2_TB_CHECK_READ_PPO_PAGE;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum])
        {
            if (SUPERPU_LUN_NUM_BITMSK == pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum])
            {
                pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] = 0;
                pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] = 0;
                *pStatus = L2_TB_CHECK_READ_PPO_PAGE;
            }
            else
            {
                *pStatus = L2_TB_CHECK_WAIT_FIRST_PAGE;
            }
        }
    }
        break;

    case L2_TB_CHECK_WAIT_FIRST_PAGE:
    {
        /* wait for flash idle */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Not read
            if (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Not read
            if (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                if (TARGET_TLC_WRITE == pTBManagement->usPageIndex[ucSuperPuNum])
                {
                    if ((pRed->m_RedComm.bcBlockType == BLOCK_TYPE_SEQ) || (pRed->m_RedComm.bcBlockType == BLOCK_TYPE_RAN) || (pRed->m_RedComm.bcBlockType == BLOCK_TYPE_TLC_GC))
                         pRed->m_RedComm.bcBlockType = BLOCK_TYPE_TLC_W;
                }
#endif

                if ((pRed->m_RedComm.bcPageType != (U8)PAGE_TYPE_DATA) || (pRed->m_RedComm.bcBlockType != (U8)L2_TB_CheckGetBlockType(pTBManagement->usPageIndex[ucSuperPuNum])))
                {
                    /* target block 1st page PageType/BlockType wrong */
                    *pStatus = L2_TB_CHECK_FAIL;
                }
                else
                {
                    /* check 1st page TS with PBIT */
                    if (pRed->m_RedComm.ulTimeStamp == L2_TB_CheckGetTimeStamp(ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum]))
                    {
                        //*pStatus = L2_TB_CHECK_READ_PPO_PAGE;
                    }
                    else
                    {
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                        if (TARGET_TLC_WRITE != pTBManagement->usPageIndex[ucSuperPuNum])
#endif
                        /* target block 1st page TS wrong */
                        *pStatus = L2_TB_CHECK_FAIL;
                    }
                }
            }
            else
            {
                /* target block 1st page read UECC or EMPTY_PG */
                *pStatus = L2_TB_CHECK_FAIL;
            }
        }

        if (L2_TB_CHECK_WAIT_FIRST_PAGE == *pStatus)
        {
            pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] = 0;
            pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] = 0;
            *pStatus = L2_TB_CHECK_READ_PPO_PAGE;
        }
    }
        break;

    case L2_TB_CHECK_READ_PPO_PAGE:
    {
        /* read ppo page redundant of Seq/Ran Target block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Read
            if (pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (L2_TB_SUCCESS == L2_TB_CheckGetFlashAddr(*pStatus, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum], &FlashAddr))
            {
                if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                    pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                    ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
                    pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                    if (TARGET_TLC_WRITE == pTBManagement->usPageIndex[ucSuperPuNum])
                    {
                       L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, FALSE);
                    }
                    else
#endif
                    {
                    L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, TRUE);
                    }
                    
                    //FIRMWARE_LogInfo("L2_TB_CHECK_READ_PPO_PAGE LUN %d Blk %d Pg %d\n", FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);

                    //*pStatus = L2_TB_CHECK_WAIT_PPO_PAGE;
                }
            }
            else
            {
                pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;

                /* last page is RPMT, no ppo page */
                //*pStatus = L2_TB_CHECK_READ_LAST_PAGE;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum])
        {
            if (SUPERPU_LUN_NUM_BITMSK == pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum])
            {
                pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] = 0;
                pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] = 0;
                *pStatus = L2_TB_CHECK_READ_LAST_PAGE;
            }
            else
            {
                *pStatus = L2_TB_CHECK_WAIT_PPO_PAGE;
            }
        }
    }
        break;

    case L2_TB_CHECK_WAIT_PPO_PAGE:
    {
        /* wait for flash idle */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Not read
            if (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Not read
            if (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_EMPTY_PG == ucFlashStatus)
            {
                //*pStatus = L2_TB_CHECK_READ_LAST_PAGE;
            }
            else
            {
                /* target block last page read UECC or SUCCESS */
                *pStatus = L2_TB_CHECK_FAIL;
            }
        }

        if (L2_TB_CHECK_WAIT_PPO_PAGE == *pStatus)
        {
            pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] = 0;
            pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] = 0;
            *pStatus = L2_TB_CHECK_READ_LAST_PAGE;
        }
    }
        break;

    case L2_TB_CHECK_READ_LAST_PAGE:
    {
        /* read ppo page redundant of Seq/Ran Target block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Read
            if (pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (L2_TB_SUCCESS == L2_TB_CheckGetFlashAddr(*pStatus, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum], &FlashAddr))
            {
                if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
                {
                    pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                    pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);
                    ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
                    pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                    if (TARGET_TLC_WRITE == pTBManagement->usPageIndex[ucSuperPuNum])
                    {
                        L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, FALSE);
                    }
                    else
#endif
                    {
                        L2_LoadSpare(&FlashAddr, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), (U32 *)pRed, TRUE);
                    }

                    //FIRMWARE_LogInfo("L2_TB_CHECK_READ_LAST_PAGE LUN %d Blk %d Pg %d\n", FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);
 
                    //*pStatus = L2_TB_CHECK_WAIT_LAST_PAGE;
                }
            }
            else
            {
                pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum]))
        {
            if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum]))
            {
                /* Seq/Ran Target has not been written, no last page */
                if (TRUE == pTBManagement->ucNeedCheckPMTTS[ucSuperPuNum])
                {
                    
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                    if(pTBManagement->usPageIndex[ucSuperPuNum] == TARGET_TLC_WRITE)
                       *pStatus = L2_TB_CHECK_PMT;  
                    else
                       *pStatus = L2_TB_CHECK_WAIT_LAST_PAGE;
#else
                    *pStatus = L2_TB_CHECK_PMT;
#endif                    
                }
                else
                {
                    *pStatus = L2_TB_CHECK_SUCCESS;
                }
            }
            else
            {
                *pStatus = L2_TB_CHECK_WAIT_LAST_PAGE;
            }
        }
    }
        break;

    case L2_TB_CHECK_WAIT_LAST_PAGE:
    {
        /* wait for flash idle */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Not read
            if (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Not read
            if (pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus || SUBSYSTEM_STATUS_RECC == ucFlashStatus || SUBSYSTEM_STATUS_RETRY_SUCCESS == ucFlashStatus)
            {
                pRed = L2_TB_GetRedAddr(PAGE_TYPE_DATA, ucSuperPuNum, ucLunInSuperPu);

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                if (TARGET_TLC_WRITE == pTBManagement->usPageIndex[ucSuperPuNum])
                {
                    if ((pRed->m_RedComm.bcBlockType == BLOCK_TYPE_SEQ) || (pRed->m_RedComm.bcBlockType == BLOCK_TYPE_RAN) || (pRed->m_RedComm.bcBlockType == BLOCK_TYPE_TLC_GC))
                         pRed->m_RedComm.bcBlockType = BLOCK_TYPE_TLC_W;
                }
#endif
                if (((pRed->m_RedComm.bcPageType == (U8)PAGE_TYPE_DATA) || (pRed->m_RedComm.bcPageType == (U8)PAGE_TYPE_RPMT))
                    && (pRed->m_RedComm.bcBlockType == (U8)L2_TB_CheckGetBlockType(pTBManagement->usPageIndex[ucSuperPuNum])))
                {
                    /* check last page TS with RT TS */
                    if ((ucSuperPuNum == RootTableAddr[0].m_PUSer) && (ucLunInSuperPu == RootTableAddr[0].m_OffsetInSuperPage))
                    {
                        if (pRed->m_RedComm.ulTimeStamp >= pRTManagement->RT_Red[ucSuperPuNum][ucLunInSuperPu].m_RedComm.ulTimeStamp)
                        {
                            /* target block last page TS bigger than RT, abnormal power cycling */
                            *pStatus = L2_TB_CHECK_FAIL;
                            break;
                        }
                    }

                    if (pRed->m_RedComm.ulTimeStamp >= pRT->m_RT[ucSuperPuNum].ulMaxTimeStamp)
                    {
                        /* target block last page TS bigger than RT, abnormal power cycling */
                        *pStatus = L2_TB_CHECK_FAIL;
                    }
                }
                else
                {
                    /* target block last page PageType/BlockType wrong */
                    *pStatus = L2_TB_CHECK_FAIL;
                }
            }
            else
            {
                /* target block last page read UECC or EMPTY_PG */
                *pStatus = L2_TB_CHECK_FAIL;
            }
        }

        if (L2_TB_CHECK_WAIT_LAST_PAGE == *pStatus)
        {
            pTBManagement->ulLunBootCheckReadBitmap[ucSuperPuNum] = 0;
            pTBManagement->ulLunBootCheckNoAddrBitmap[ucSuperPuNum] = 0;

            pTBManagement->usPageIndex[ucSuperPuNum]++;

            /*Skip Check RndTarget if no RndTarget exist*/
            if ((TARGET_HOST_GC == pTBManagement->usPageIndex[ucSuperPuNum]) && (INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_HOST_GC]) && (INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetPPO[TARGET_HOST_GC]))
            {
                pTBManagement->usPageIndex[ucSuperPuNum]++;
            }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            if ((TARGET_TLC_WRITE == pTBManagement->usPageIndex[ucSuperPuNum]) && 
                (INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetBlock[TARGET_TLC_WRITE]) &&
                ((INVALID_4F == pPBIT[ucSuperPuNum]->m_TargetPPO[TARGET_TLC_WRITE]) || (0 == pPBIT[ucSuperPuNum]->m_TargetPPO[TARGET_TLC_WRITE])))
            {
                 pTBManagement->usPageIndex[ucSuperPuNum]++;
            }
            
            if (pTBManagement->usPageIndex[ucSuperPuNum] >= TARGET_ALL)
#else
            if (pTBManagement->usPageIndex[ucSuperPuNum] >= TARGET_TLC_WRITE)
#endif
            {
                if (TRUE == pTBManagement->ucNeedCheckPMTTS[ucSuperPuNum])
                {
                    *pStatus = L2_TB_CHECK_PMT;
                }
                else
                {
                    *pStatus = L2_TB_CHECK_SUCCESS;
                }
            }
            else
            {
                *pStatus = L2_TB_CHECK_READ_FIRST_PAGE;
            }
        }
    }
        break;

    case L2_TB_CHECK_PMT:
    {
        ulCmdRet = L2_TB_BootCheckPMT(ucSuperPuNum);

        if (L2_TB_WAIT != ulCmdRet)
        {
            if (L2_TB_FAIL == ulCmdRet)
            {
                *pStatus = L2_TB_CHECK_FAIL;
            }
            else
            {
                /* add check data blk to patch a corner case, it could make abnormal shutdown 
                check to normal boot, TLC alg only need check SLC Blk */
                *pStatus = L2_TB_CHECK_DATA_BLK;
            }

            pTBManagement->CheckPMTStatus[ucSuperPuNum] = L2_TB_CHECK_PMT_START;
            pTBManagement->ucNeedCheckPMTTS[ucSuperPuNum] = FALSE;

        }
    }
        break;
    case L2_TB_CHECK_DATA_BLK:
    {
        ulCmdRet = L2_TB_BootCheckDataBlk(ucSuperPuNum);

        if (L2_TB_WAIT != ulCmdRet)
        {
            if (L2_TB_FAIL == ulCmdRet)
            {
                *pStatus = L2_TB_CHECK_FAIL;
            }
            else
            {
                *pStatus = L2_TB_CHECK_SUCCESS;
            }

            pTBManagement->CheckDataBlkStatus[ucSuperPuNum] = L2_TB_CHECK_DATA_BLK_START;
        }
    }
        break;

    case L2_TB_CHECK_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_CHECK_SUCCESS:
    {
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_BootCheck invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}
BOOL MCU1_DRAM_TEXT L2_TB_SavePbitBlock(U32 *pSuperPuMsk)
{
    U8 ucSuperPu;
    U32 ulCmdRet;
    BOOL bRet = FALSE;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        if (*pSuperPuMsk&(1 << ucSuperPu))
        {
            ulCmdRet = L2_TB_SaveTableBlock(ucSuperPu, PAGE_TYPE_PBIT);
            if (ulCmdRet == L2_TB_SUCCESS)
            {
                *pSuperPuMsk &= (~(1 << ucSuperPu));
                pTBManagement->SaveStatus[ucSuperPu] = L2_TB_SAVE_START;
            }
        }
    }
    if (*pSuperPuMsk == 0)
    {
        bRet = TRUE;
    }

    return bRet;
}
LOCAL MCU12_VAR_ATTR U16 l_usTBBlock[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];

U8 MCU1_DRAM_TEXT L2_TB_CheckFlashStatus(U8 ucSuperPuNum)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (SUBSYSTEM_STATUS_PENDING == pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu])
        {
            return SUBSYSTEM_STATUS_PENDING;
        }
        else if (SUBSYSTEM_STATUS_FAIL == pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu])
        {
            return SUBSYSTEM_STATUS_FAIL;
        }
    }

    return SUBSYSTEM_STATUS_SUCCESS;
}

U32 MCU1_DRAM_TEXT L2_TB_SaveTableBlock(U8 ucSuperPuNum, PAGE_TYPE PageType)
{
    U8  ucLunInSuperPu;
    U8  ucFlashStatus;
    U32 ulRet;
    U32 ulCmdRet;
    U32 ulBufferAddr;
    U32 ulDataAddr;
    RED *pRed;
    L2_TB_SAVE_STATUS *pStatus;
    PhysicalAddr FlashAddr = { 0 };

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->SaveStatus[ucSuperPuNum]);

    switch (*pStatus)
    {
    case L2_TB_SAVE_START:
    {
        /* Init global values */
        if (PAGE_TYPE_PBIT == PageType)
        {
            //save order PBIT-> VBT
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
            if (TLC_WRITE_SWL != g_TLCManager->aeTLCWriteType[ucSuperPuNum])
#endif
                L2_SWLUnlockTargetBlkBuf(ucSuperPuNum);

            L2_PBIT_Save_UpdateData(ucSuperPuNum);
        }
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        else if (PAGE_TYPE_TLCW == PageType)
        {
            L2_TLCSharedPageClosedSave(ucSuperPuNum);
        }
#endif

        pTBManagement->usPageIndex[ucSuperPuNum] = 0;

        pTBManagement->ulLunSaveBitmap[ucSuperPuNum] = 0x00;
        *pStatus = L2_TB_SAVE_WRITE_PAGE;
    }
        break;

    case L2_TB_SAVE_WRITE_PAGE:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Wirtten
            if (pTBManagement->ulLunSaveBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            /* write Table pages to Flash  */
            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                if(0 == pTBManagement->ulLunSaveBitmap[ucSuperPuNum])
                {
                    L2_IncTimeStampInPu(ucSuperPuNum);
                }
                
                pTBManagement->ulLunSaveBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                /* get flashaddr from AT0 current block and ppo */
                L2_TB_SaveGetFlashAddr(ucSuperPuNum, ucLunInSuperPu, &FlashAddr);

                /* copy table data to TB temp buffer */
                ulBufferAddr = L2_TB_GetDRAMBufAddr(ucSuperPuNum, ucLunInSuperPu);
                ulDataAddr = L2_TB_GetDataAddr(PageType, ucSuperPuNum, ucLunInSuperPu, pTBManagement->usPageIndex[ucSuperPuNum]);
                //COM_MemCpy((U32*)ulBufferAddr, (U32*)ulDataAddr, ((BUF_SIZE) / sizeof(U32)));
                HAL_DMAECopyOneBlock(ulBufferAddr, ulDataAddr, BUF_SIZE);

                /* set table redundant */
                pRed = L2_TB_GetRedAddr(PageType, ucSuperPuNum, ucLunInSuperPu);
                L2_TB_SaveSetRed(PageType, ucSuperPuNum, pTBManagement->usPageIndex[ucSuperPuNum], pRed);

                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;

                /* update PBIT 1st page Info */
                if (0 == FlashAddr.m_PageInBlock)
                {
                    L2_Set_TableBlock_PBIT_Info(FlashAddr.m_PUSer, FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, pRed);
                }

#if 0
                if ((PageType == PAGE_TYPE_PBIT) && (0 == ucSuperPuNum))
                {
                    DBG_Printf("L2_TB_SaveTableBlock Write Type 0x%x PU 0x%x LUN 0x%x BLK 0x%x PG 0x%x\n",
                        PageType, FlashAddr.m_PUSer, FlashAddr.m_OffsetInSuperPage, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);
                }
#endif


                //FIRMWARE_LogInfo("L2_TB_SaveTableBlock PageType %d PageIndex %d SuperPu %d ucLunInSuperPu %d Blk 0x%x Pg 0x%x\n",
                //    PageType, pTBManagement->usPageIndex[ucSuperPuNum],ucSuperPuNum, ucLunInSuperPu, FlashAddr.m_BlockInPU, FlashAddr.m_PageInBlock);

                /* send write request to L3 */
                L2_FtlWriteLocal(&FlashAddr, (U32 *)ulBufferAddr, (U32 *)pRed, &(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, NULL);

#ifdef L2MEASURE
                L2MeasureLogIncWCnt(ucSuperPuNum, L2MEASURE_TYPE_AT0_SLC);
#endif
            }
        }

        /* All LUNs are write done */
        if (SUPERPU_LUN_NUM_BITMSK == pTBManagement->ulLunSaveBitmap[ucSuperPuNum])
        {
            *pStatus = L2_TB_SAVE_WAIT_PAGE;
        }
    }
        break;

    case L2_TB_SAVE_WAIT_PAGE:
    {
        if (SUBSYSTEM_STATUS_PENDING == L2_TB_CheckFlashStatus(ucSuperPuNum))
        {
            return ulRet;
        }

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_FAIL == ucFlashStatus)
            {
                /* clear ulLunSaveBitmap to indicate save fail LUN*/
                pTBManagement->ulLunSaveBitmap[ucSuperPuNum] &= (~(0x01 << ucLunInSuperPu));
                *pStatus = L2_TB_SAVE_FAIL_MOVE_BLK;
                return ulRet;
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunSaveBitmap[ucSuperPuNum]))
        {
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
            L2_IncTimeStampInPu(ucSuperPuNum);
#endif
            L2_TB_SaveSetFlashAddr(PageType, ucSuperPuNum, pTBManagement->usPageIndex[ucSuperPuNum]);

            pTBManagement->usPageIndex[ucSuperPuNum]++;
            if (pTBManagement->usPageIndex[ucSuperPuNum] >= L2_TB_GetPageCntPerCE(PageType))
            {
                pTBManagement->usPageIndex[ucSuperPuNum] = 0;
                *pStatus = L2_TB_SAVE_CHECK_ERASE;
            }
            else
            {
                *pStatus = L2_TB_SAVE_WRITE_PAGE;
            }
            pTBManagement->ulLunSaveBitmap[ucSuperPuNum] = 0;
        }
    }
        break;
    case L2_TB_SAVE_FAIL_MOVE_BLK:
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (0 != (pTBManagement->ulLunSaveBitmap[ucSuperPuNum] & (1 << ucLunInSuperPu)))
            {
                continue;
            }

            /* save AT0 write page fail: errorhandling move block */
            ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPuNum, ucLunInSuperPu, pRT->m_RT[ucSuperPuNum].CurAT0Block[ucLunInSuperPu], pRT->m_RT[ucSuperPuNum].CurAT0PPO, WRITE_ERR);

            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                *pStatus = L2_TB_SAVE_WRITE_PAGE;
            }
            else
            {
                DBG_Printf("L2_TB_SaveTableBlock PU 0x%x LUN 0x%x Write ErrorHandling Fail\n", ucSuperPuNum, ucLunInSuperPu);
                DBG_Getch();
            }
        }

        break;
    case L2_TB_SAVE_CHECK_ERASE:
    {
        *pStatus = L2_TB_SAVE_SUCCESS;
        pTBManagement->ulLunEraseBitmap[ucSuperPuNum] = 0x00;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            l_usTBBlock[ucSuperPuNum][ucLunInSuperPu] = L2_TB_SaveGetRecycleBlock(ucSuperPuNum, ucLunInSuperPu);

            if (INVALID_4F != l_usTBBlock[ucSuperPuNum][ucLunInSuperPu])
            {
                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;
                *pStatus = L2_TB_SAVE_ERASE_BLOCK;
            }
        }
    }
        break;
    case L2_TB_SAVE_ERASE_BLOCK:
    {
        /* erase useless block */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Erase not needed.
            if (INVALID_4F == l_usTBBlock[ucSuperPuNum][ucLunInSuperPu])
            {
                /* Set ulLunEraseBitmap to mark it don't need erase */
                pTBManagement->ulLunEraseBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);
                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;
                continue;
            }

            // Erased.
            if (pTBManagement->ulLunEraseBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu))
            {
                continue;
            }

            if (TRUE == L2_FCMDQNotFull(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu)))
            {
                /* Erase AT0 useless Block */
                pTBManagement->ulLunEraseBitmap[ucSuperPuNum] |= (0x01 << ucLunInSuperPu);

                DBG_Printf("L2_TB_SaveTableBlock ERASE PU 0x%x LUN 0x%x BLK 0x%x start\n", ucSuperPuNum, ucLunInSuperPu, l_usTBBlock[ucSuperPuNum][ucLunInSuperPu]);

                L2_FtlEraseBlock(ucSuperPuNum, ucLunInSuperPu, l_usTBBlock[ucSuperPuNum][ucLunInSuperPu], (U8 *)&(pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu]), TRUE, TRUE, FALSE);
            }
        }

        if (SUPERPU_LUN_NUM_BITMSK == (pTBManagement->ulLunEraseBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_SAVE_WAIT_ERASE;
        }
    }
        break;

    case L2_TB_SAVE_WAIT_ERASE:
    {
        /* wait for flash idle */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            // Erase not needed.
            if (INVALID_4F == l_usTBBlock[ucSuperPuNum][ucLunInSuperPu])
            {
                pTBManagement->ulLunEraseBitmap[ucSuperPuNum] &= (~(0x01 << ucLunInSuperPu));
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];

            if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
            {
                return ulRet;
            }
        }


        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {

            if (0 == (pTBManagement->ulLunEraseBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu)))
            {
                continue;
            }

            ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu];
            if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
            {
                DBG_Printf("L2_TB_SaveTableBlock ERASE PU 0x%x LUN 0x%x success!\n", ucSuperPuNum, ucLunInSuperPu);

                /* clear ulLunEraseBitmap to mark it erase success */
                pTBManagement->ulLunEraseBitmap[ucSuperPuNum] &= (~(0x01 << ucLunInSuperPu));
                L2_TB_SaveBlockRecycled(ucSuperPuNum, ucLunInSuperPu);

                //FIRMWARE_LogInfo("L2_TB_SaveTableBlock ERASE PU 0x%x LUN 0x%x success!\n", ucSuperPuNum, ucLunInSuperPu);

            }
            else
            {
                *pStatus = L2_TB_SAVE_ERASE_BLK_FAIL_MOVE_BLK;
            }
        }

        /* all LUN erase success */
        if (0 == (pTBManagement->ulLunEraseBitmap[ucSuperPuNum]))
        {
            *pStatus = L2_TB_SAVE_SUCCESS;
        }
    }

        break;
    case L2_TB_SAVE_ERASE_BLK_FAIL_MOVE_BLK:
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (0 == (pTBManagement->ulLunEraseBitmap[ucSuperPuNum] & (0x01 << ucLunInSuperPu)))
            {
                continue;
            }

            /* erase AT0 block fail: errorhandling move block */
            ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPuNum, ucLunInSuperPu, L2_TB_SaveGetRecycleBlock(ucSuperPuNum, ucLunInSuperPu), 0, ERASE_ERR);

            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                DBG_Printf("PU %d LUN %d save table block erase fail,MoveBlk Success!\n", ucSuperPuNum, ucLunInSuperPu);
                L2_TB_SaveBlockRecycled(ucSuperPuNum, ucLunInSuperPu);

                pTBManagement->ucFlashStatus[ucSuperPuNum][ucLunInSuperPu] = SUBSYSTEM_STATUS_SUCCESS;

                /* clear ulLunEraseBitmap to mark it erase success */
                pTBManagement->ulLunEraseBitmap[ucSuperPuNum] &= (~(0x01 << ucLunInSuperPu));

                /* all LUN erase success */
                if (0 == (pTBManagement->ulLunEraseBitmap[ucSuperPuNum]))
                {
                    *pStatus = L2_TB_SAVE_SUCCESS;
                }
            }
            else
            {
                DBG_Printf("L2_TB_SaveTableBlock PU 0x%x LUN 0x%x Erase ErrorHandling Fail,ulCmdRet %d\n", ucSuperPuNum, ucLunInSuperPu, ulCmdRet);
                DBG_Getch();
            }
        }
    }
        break;

    case L2_TB_SAVE_FAIL:
    {
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_SAVE_SUCCESS:
    {
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TB_SaveTableBlock invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_LoadRTBeforBoot(void)
{
    U8 ucSuperPuNum;
    U8 ucLunInSuperPu;
    U8 ucCheckPuCnt;
    U32 ulRet;
    U32 ulCmdRet[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
    L2_TB_BOOT_STATUS *pStatus;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->BootStatus);

    switch (*pStatus)
    {
    case L2_TB_BOOT_START:
    {
        /* Init global values */
        L2_RT_ResetLoadStatus();
        L2_TB_ResetLoadStatus();
        L2_RT_ResetCheckStatus();
        L2_TB_ResetCheckStatus();

        /* load block Start/End Pointer from BBT */
        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                L2_BbtGetRootPointer(L2_GET_TLUN(ucSuperPuNum, ucLunInSuperPu), &(g_ulTableBlockEnd[ucSuperPuNum][ucLunInSuperPu]), &(g_ulDataBlockStart[ucSuperPuNum][ucLunInSuperPu]));
            }

            /* this flag notifies L2_FullRecovry that RT has been load when load RT, 
                       so full recovery can skip scan RT block. Default is TRUE to represent RT are load already*/
            bRTHasLoad[ucSuperPuNum] = TRUE;
        }


        *pStatus = L2_TB_BOOT_LOAD_RT;
    }
        break;

    case L2_TB_BOOT_LOAD_RT:
    {
        ucCheckPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ulCmdRet[ucSuperPuNum][ucLunInSuperPu] = L2_RT_LoadRT(ucSuperPuNum, ucLunInSuperPu);

                if (L2_RT_WAIT != ulCmdRet[ucSuperPuNum][ucLunInSuperPu])
                {
                    ucCheckPuCnt++;

                    /* some Lun RT load fail ,reset corresponding superpu flag to false*/
                    if (L2_RT_FAIL == ulCmdRet[ucSuperPuNum][ucLunInSuperPu])
                    {
                        bRTHasLoad[ucSuperPuNum] = FALSE;
                    }
                }
            }
        }

        if (ucCheckPuCnt >= SUBSYSTEM_SUPERPU_NUM*LUN_NUM_PER_SUPERPU)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    if (L2_RT_FAIL == ulCmdRet[ucSuperPuNum][ucLunInSuperPu])
                    {
                        *pStatus = L2_TB_BOOT_FAIL;
                        break;
                    }
                }
                if (L2_TB_BOOT_FAIL == *pStatus)
                {
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_RT_ResetLoadStatus();
                L2_RTResumeSysInfo();
                g_pSubSystemDevParamPage->PowerCycleCnt++;
                gbGlobalInfoSaveFlag = TRUE;
                *pStatus = L2_TB_BOOT_CHECK_RT;

                //DBG_Printf("L2_TableBlock_Boot RT SuperPU 0x%x LUN 0x%x BLK 0x%x PG 0x%x AT0 BLK 0x%x PPO 0x%x\n",
                //    RootTableAddr[0].m_PUSer, RootTableAddr[0].m_OffsetInSuperPage, RootTableAddr[0].m_BlockInPU, RootTableAddr[0].m_PageInBlock, pRT->m_RT[0].CurAT0Block[0], pRT->m_RT[0].CurAT0PPO);
            }
        }
    }
        break;

    case L2_TB_BOOT_CHECK_RT:
    {
        ucCheckPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                ulCmdRet[ucSuperPuNum][ucLunInSuperPu] = L2_RT_CheckRT(ucSuperPuNum, ucLunInSuperPu);
                if (L2_RT_WAIT != ulCmdRet[ucSuperPuNum][ucLunInSuperPu])
                {
                    ucCheckPuCnt++;
                }
            }
        }

        if (ucCheckPuCnt >= SUBSYSTEM_SUPERPU_NUM*LUN_NUM_PER_SUPERPU)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    if (L2_RT_FAIL == ulCmdRet[ucSuperPuNum][ucLunInSuperPu])
                    {
                        //Load RT Faile
                        *pStatus = L2_TB_BOOT_FAIL;
                        break;
                    }
                }
            }

            //Load RT success   
            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_RT_ResetCheckStatus();
                *pStatus = L2_TB_BOOT_LOAD_PBIT;
                ulRet = L2_TB_SUCCESS;
            }
        }
    }
        break;

    case L2_TB_BOOT_FAIL:
    {
        L2_RT_ResetLoadStatus();
        L2_TB_ResetLoadStatus();
        L2_RT_ResetCheckStatus();
        L2_TB_ResetCheckStatus();
        L2_TB_ResetBootStatus();
        ulRet = L2_TB_FAIL;
    }
        break;

    default:
    {
        DBG_Printf("L2_TableBlock_Boot invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_TableBlock_Boot(void)
{
    U8 ucSuperPuNum;
    U8 ucCheckSuperPuCnt;
    U32 ulRet;
    U32 ulCmdRet[SUBSYSTEM_SUPERPU_MAX];
    L2_TB_BOOT_STATUS *pStatus;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->BootStatus);

    switch (*pStatus)
    {
    case L2_TB_BOOT_LOAD_PBIT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_LoadTableBlock(ucSuperPuNum, PAGE_TYPE_PBIT);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_TB_ResetLoadStatus();
                *pStatus = L2_TB_BOOT_LOAD_VBT;
            }
        }
    }
        break;

    case L2_TB_BOOT_LOAD_VBT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_LoadTableBlock(ucSuperPuNum, PAGE_TYPE_VBT);
            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
#ifdef DirtyLPNCnt_IN_DSRAM1
                U32 i;
                for (i = 0; i < VIR_BLK_CNT; i++)
                {
                    *(g_pDirtyLPNCnt + ucSuperPuNum*VIR_BLK_CNT + i) = pVBT[ucSuperPuNum]->m_VBT[i].DirtyLPNCnt;
                }
#endif
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_TB_ResetLoadStatus();
                *pStatus = L2_TB_BOOT_CHECK_TS;
            }
        }
    }
        break;

    case L2_TB_BOOT_CHECK_TS:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_BootCheck(ucSuperPuNum);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_TB_ResetCheckStatus();
                L2_TB_RestoreEraseQueue();
                *pStatus = L2_TB_BOOT_LOAD_DPBM;
            }
        }
    }
        break;

    case L2_TB_BOOT_LOAD_DPBM:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_LoadTableBlock(ucSuperPuNum, PAGE_TYPE_DPBM);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_TB_ResetLoadStatus();
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                *pStatus = L2_TB_BOOT_LOAD_TLCW;
#else
                *pStatus = L2_TB_BOOT_LOAD_RPMT;
#endif
            }
        }
    }
        break;
        
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case L2_TB_BOOT_LOAD_TLCW:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_LoadTableBlock(ucSuperPuNum, PAGE_TYPE_TLCW);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {        
                L2_TB_ResetLoadStatus();
                *pStatus = L2_TB_BOOT_LOAD_RPMT;
            }
        }
    }
        break;
#endif

    case L2_TB_BOOT_LOAD_RPMT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_LoadTableBlock(ucSuperPuNum, PAGE_TYPE_RPMT);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_TB_ResetLoadStatus();
                *pStatus = L2_TB_BOOT_LOAD_PMTI;
            }
        }
    }
        break;

    case L2_TB_BOOT_LOAD_PMTI:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_LoadTableBlock(ucSuperPuNum, PAGE_TYPE_PMTI);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_BOOT_FAIL;
                    break;
                }
            }

            if (L2_TB_BOOT_FAIL != *pStatus)
            {
                L2_TB_ResetLoadStatus();
#ifndef LCT_TRIM_REMOVED
                L1_ResetLCTTrim();
#endif
#ifdef LCT_VALID_REMOVED
                *pStatus = L2_TB_BOOT_SUCCESS;
#else
                *pStatus = L2_TB_BOOT_LOAD_VBMT;
#endif
            }
        }
    }
        break;

#ifndef LCT_VALID_REMOVED
    case L2_TB_BOOT_LOAD_VBMT:
    {
        ulCmdRet[0] = L2_TB_LoadTableBlock(0, PAGE_TYPE_VBMT);

        if (L2_RT_FAIL == ulCmdRet[0])
        {
            *pStatus = L2_TB_BOOT_FAIL;
        }
        else if (L2_RT_SUCCESS == ulCmdRet[0])
        {
            /* LCTValid load from flash overlap with LCTTrim buffer */
            L1_ResetLCTTrim();
            *pStatus = L2_TB_BOOT_SUCCESS;
        }
    }
        break;
#endif

    case L2_TB_BOOT_FAIL:
    {
        L2_RT_ResetLoadStatus();
        L2_TB_ResetLoadStatus();
        L2_RT_ResetCheckStatus();
        L2_TB_ResetCheckStatus();
        L2_TB_ResetBootStatus();
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_BOOT_SUCCESS:
    {
        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            if(L2_SetEraseInfoAfterFlushPMT(ucSuperPuNum))
            {
                g_EraseOpt[ucSuperPuNum].L2NeedEraseBlk = TRUE;
            }
        }

        L2_RT_ResetLoadStatus();
        L2_TB_ResetLoadStatus();
        L2_RT_ResetCheckStatus();
        L2_TB_ResetCheckStatus();
        L2_TB_ResetBootStatus();
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TableBlock_Boot invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}

U32 MCU1_DRAM_TEXT L2_TableBlock_Shutdown(void)
{
    U8 ucSuperPuNum;
    U8 ucCheckSuperPuCnt;
    U32 ulRet;
    U32 ulCmdRet[SUBSYSTEM_SUPERPU_MAX];
    L2_TB_SHUTDOWN_STATUS *pStatus;

    ulRet = L2_TB_WAIT;
    pStatus = &(pTBManagement->ShutdownStatus);

    switch (*pStatus)
    {
    case L2_TB_SHUTDOWN_START:
    {
        /* Init global values */
        L2_RT_ResetSaveStatus();
        L2_TB_ResetSaveStatus();
#ifndef LCT_VALID_REMOVED
        *pStatus = L2_TB_SHUTDOWN_SAVE_VBMT;
#else
        *pStatus = L2_TB_SHUTDOWN_SAVE_PMTI;
#endif
    }
        break;

#ifndef LCT_VALID_REMOVED
    case L2_TB_SHUTDOWN_SAVE_VBMT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_VBMT);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
                *pStatus = L2_TB_SHUTDOWN_SAVE_PMTI;
            }
        }
    }
        break;
#endif

    case L2_TB_SHUTDOWN_SAVE_PMTI:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_PMTI);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
                *pStatus = L2_TB_SHUTDOWN_SAVE_RPMT;
            }
        }
    }
        break;

    case L2_TB_SHUTDOWN_SAVE_RPMT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_RPMT);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
                *pStatus = L2_TB_SHUTDOWN_SAVE_TLCW;
#else
                *pStatus = L2_TB_SHUTDOWN_SAVE_DPBM;
#endif
            }
        }
    }
        break;
        
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    case L2_TB_SHUTDOWN_SAVE_TLCW:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_TLCW);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
                *pStatus = L2_TB_SHUTDOWN_SAVE_DPBM;
            }
        }
    }
        break;
#endif        

    case L2_TB_SHUTDOWN_SAVE_DPBM:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_DPBM);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
                *pStatus = L2_TB_SHUTDOWN_SAVE_PBIT;
            }
        }
    }
        break;

    case L2_TB_SHUTDOWN_SAVE_PBIT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_PBIT);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
                *pStatus = L2_TB_SHUTDOWN_SAVE_VBT;
            }
        }
    }
        break;

    case L2_TB_SHUTDOWN_SAVE_VBT:
    {
        ucCheckSuperPuCnt = 0;

        for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
        {
#ifdef DirtyLPNCnt_IN_DSRAM1
            U32 i;
            for (i = 0; i < VIR_BLK_CNT; i++)
            {
                pVBT[ucSuperPuNum]->m_VBT[i].DirtyLPNCnt = *(g_pDirtyLPNCnt + ucSuperPuNum*VIR_BLK_CNT + i);
            }
#endif

            ulCmdRet[ucSuperPuNum] = L2_TB_SaveTableBlock(ucSuperPuNum, PAGE_TYPE_VBT);

            if (L2_TB_WAIT != ulCmdRet[ucSuperPuNum])
            {
                ucCheckSuperPuCnt++;
            }
        }

        if (ucCheckSuperPuCnt >= SUBSYSTEM_SUPERPU_NUM)
        {
            for (ucSuperPuNum = 0; ucSuperPuNum < SUBSYSTEM_SUPERPU_NUM; ucSuperPuNum++)
            {
                if (L2_TB_FAIL == ulCmdRet[ucSuperPuNum])
                {
                    *pStatus = L2_TB_SHUTDOWN_FAIL;
                    break;
                }
            }

            if (L2_TB_SHUTDOWN_FAIL != *pStatus)
            {
                L2_TB_ResetSaveStatus();
                *pStatus = L2_TB_SHUTDOWN_SAVE_RT;
            }
        }
    }
        break;

    case L2_TB_SHUTDOWN_SAVE_RT:
    {
        ulCmdRet[0] = L2_RT_SaveRT(TRUE);

        if (L2_RT_FAIL == ulCmdRet[0])
        {
            *pStatus = L2_TB_SHUTDOWN_FAIL;
        }
        else if (L2_RT_SUCCESS == ulCmdRet[0])
        {
            *pStatus = L2_TB_SHUTDOWN_SUCCESS;
        }
    }
        break;

    case L2_TB_SHUTDOWN_FAIL:
    {
        L2_RT_ResetSaveStatus();
        L2_TB_ResetSaveStatus();
        L2_TB_ResetShutdownStatus();
        ulRet = L2_TB_FAIL;
    }
        break;

    case L2_TB_SHUTDOWN_SUCCESS:
    {
        L2_RT_ResetSaveStatus();
        L2_TB_ResetSaveStatus();
        L2_TB_ResetShutdownStatus();
        ulRet = L2_TB_SUCCESS;
    }
        break;

    default:
    {
        DBG_Printf("L2_TableBlock_Shutdown invalid Status 0x%x\n", *pStatus);
        DBG_Getch();
    }
        break;
    }

    return ulRet;
}



/********************** FILE END ***************/
