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
Filename    :L2_StripeInfo.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.1
Description :functions about PuInfo
1) L2_InitPuInfo(): init the value to boot up status
Others      :
Modify      :
*******************************************************************************/
#include "FW_Event.h"
#include "L2_PMTPage.h"
#include "L2_Boot.h"
#include "HAL_Inc.h"
#include "L2_GCManager.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_StripeInfo.h"
#include "L2_TableBlock.h"
#include "L2_SearchEngine.h"
#include "L2_DWA.h"
#include "L2_FCMDQ.h"
#include "L2_Interface.h"
#include "L2_TLCMerge.h"
#include "L2_RPMT.h"

#include "COM_BitMask.h"
#include "COM_Memory.h"
#include "HAL_TraceLog.h"
#ifdef SIM
#include "sim_search_engine.h"
#include "L2_ErrorHandling.h"
#endif
#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif

//specify file name for Trace Log
#define TL_FILE_NUM  L2_StripeInfo_c

#if defined(SWL_EVALUATOR) && (!defined(SWL_OFF))
extern void SWLRecordGCTLC2SLC(U8 ucSuperPu);
#endif

#ifdef L2MEASURE
extern void MCU1_DRAM_TEXT L2MeasureLogIncWCnt(U8 ucSuperPu, U8 ucWType);
#endif

extern GLOBAL  RebuildCalcDirtyCntDptr* l_CalcDirtyCntDptr;

extern GLOBAL FTL_IDLE_GC_MGR g_tFTLIdleGCMgr;

U32 L2_GetLBNInCEFromLPN(U32 LPN)
{
    U32 LBNInCE;

    LBNInCE = LPN / (SUBSYSTEM_LUN_NUM * LPN_PER_BLOCK);

    return LBNInCE;
}

extern U8 L1_GetSuperPuFromLCT(U32 LPN);
U8 L2_GetSuperPuFromLPN(U32 ulLpn)
{
    return L1_GetSuperPuFromLCT(LCTINDEX_FROM_LPN(ulLpn));
}

U16 L2_GetPuTargetVBN(U8 ucSuperPU, TargetType eTarget)
{
    PuInfo* pInfo;
    U16 usVBN;

    pInfo = g_PuInfo[ucSuperPU];

    if (TARGET_HOST_WRITE_NORAML == eTarget)
    {
        usVBN = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
    }
    else if (TARGET_HOST_GC == eTarget)
    {
        usVBN = pInfo->m_TargetBlk[TARGET_HOST_GC];
    }
    else if (TARGET_TLC_WRITE == eTarget)
    {
        usVBN = pInfo->m_TargetBlk[TARGET_TLC_WRITE];
    }
    else
    {
        DBG_Printf("%s : SPU %d target %d error\n",__FUNCTION__, ucSuperPU, eTarget);
        DBG_Getch();
    }

    return usVBN;
}

U16 L2_GetPuTargetPPO(U8 ucSuperPU, TargetType eTarget)
{
    PuInfo* pInfo;
    U16 ucPPO;

    pInfo = g_PuInfo[ucSuperPU];

    if (TARGET_HOST_WRITE_NORAML == eTarget)
    {
        ucPPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
    }
    else if (TARGET_HOST_GC == eTarget)
    {
        ucPPO = pInfo->m_TargetPPO[TARGET_HOST_GC];
    }
    else
    {
        DBG_Printf("%s : %d target %d error\n",__FUNCTION__, ucSuperPU, eTarget);
        DBG_Getch();
    }

    return ucPPO;
}

//////////////////////////////////////////////////////////////////////////
//void L2_InitPuInfo(U16 SN)
//function:
//    init the value of Pu info to boot up status.
//parameters:
//    SN:        the index of the Puinfo
//    
//description:
//    m_pRPMT has been allocated before firmware call L2_InitPuInfo()
//    m_MaxDataBlockCntPerCE will be set in other functions.
//////////////////////////////////////////////////////////////////////////
void MCU1_DRAM_TEXT L2_InitPuInfo(U8 ucSuperPu)
{
    U16 i, j, k;
    PuInfo* pInfo;
    U8 ucLUNOffset;

    pInfo = g_PuInfo[ucSuperPu];
    pInfo->m_TimeStamp = 0;

    for (i = 0; i < BLKTYPE_ALL; i++)
    {
        if (BLKTYPE_SLC == i)
        {
            pInfo->m_DataBlockCnt[i] = SLC_BLK_CNT;
            pInfo->m_SLCMLCFreePageCnt = (PG_PER_SLC_BLK)* pInfo->m_DataBlockCnt[i];
        }
        else
        {
            pInfo->m_DataBlockCnt[i] = TLC_BLK_CNT;
        }
        pInfo->m_AllocateBlockCnt[i] = 0;
    }

    for (i = 0; i < TARGET_ALL; i++)
    {
        pInfo->m_TargetBlk[i] = INVALID_4F;
        pInfo->m_TargetPPO[i] = INVALID_4F;
        pInfo->m_TargetOffsetBitMap[i] = 0;

        pInfo->m_RPMTFlushBitMap[i] = 0;
        pInfo->m_RPMTBufferPointer[i] = 0;
    }
    for (i = 0; i < (TARGET_ALL - 1); i++)
    {
        for (j = 0; j < RPMT_BUFFER_DEPTH; j++)
        {
            for (ucLUNOffset = 0; ucLUNOffset < LUN_NUM_PER_SUPERPU; ucLUNOffset++)
            {
                /* Begin: fix bug of rebuild PMT read LPN overflowed from RPMT by henryluo 2015-05-14 */
                /* set LPN to INVALID_8F */
                HAL_DMAESetValue((U32)(pInfo->m_pRPMT[i][j]->m_RPMT[ucLUNOffset].m_RPMTItems), (LPN_PER_BLOCK * sizeof(U32)), INVALID_8F);

                /* set TS to zero */
                HAL_DMAESetValue((U32)(pInfo->m_pRPMT[i][j]->m_RPMT[ucLUNOffset].m_SuperPageTS), (PG_PER_SLC_BLK * sizeof(U32)), INVALID_8F);
                /* End: fix bug of rebuild PMT read LPN overflowed from RPMT by henryluo 2015-05-14 */
            }

            for (k = 0; k < LUN_NUM_PER_SUPERPU; k++)
            {
                pInfo->m_RPMTBufferStatus[i][j][k] = SUBSYSTEM_STATUS_SUCCESS;
            }
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_UpdateResCntByRebuild(U8 ucSuperPu)
{
    PuInfo* pInfo;

    DBG_Printf("[%s] code for TLC not build\n", __FUNCTION__);
    DBG_Getch();

    pInfo = g_PuInfo[ucSuperPu];
    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]++;
    pInfo->m_DataBlockCnt[BLKTYPE_SLC]++;

}

void MCU1_DRAM_TEXT L2_SetBlockType(U8 ucSuperPu, U32 VirBlk, BLOCK_TYPE  BlkType)
{
    U32 PhyBlk;
    U32 uLUNInSuperPU;

    for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
    {
        PhyBlk = pVBT[ucSuperPu]->m_VBT[VirBlk].PhysicalBlockAddr[uLUNInSuperPU];
        pPBIT[ucSuperPu]->m_PBIT_Entry[uLUNInSuperPU][PhyBlk].BlockType = BlkType;
    }
}

void MCU1_DRAM_TEXT NewSeqBlock(U8 ucSuperPu, U32 ulRPMTPointer)
{
    PuInfo* pInfo;
    U16 VBN;
    U32 uLUNInSuperPU;

    pInfo = g_PuInfo[ucSuperPu];

    /* allocate sequential target block */
    VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
    if (INVALID_4F == VBN)
    {
        DBG_Printf("Seq Virtual block alloc fail\n");
        DBG_Getch();
    }

    pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = VBN;
    pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = 0;

    /* record Block info in RPMT */
    for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
    {
        pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][ulRPMTPointer]->m_RPMT[uLUNInSuperPU].m_SuperPU = ucSuperPu;
        pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][ulRPMTPointer]->m_RPMT[uLUNInSuperPU].m_SuperBlock = VBN;
    }
    pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML] = ulRPMTPointer;
    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_HOST_W;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_HOST;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;
    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]++;

    //Set PBIT block type
    L2_SetBlockType(ucSuperPu, VBN, BLOCK_TYPE_SEQ);
}


void MCU1_DRAM_TEXT NewRndBlock(U8 ucSuperPu, U32 ulRPMTPointer)
{
    PuInfo* pInfo;
    U16 VBN;
    U32 uLUNInSuperPU;

    pInfo = g_PuInfo[ucSuperPu];

    /* allocate random target block */
    VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
    if (INVALID_4F == VBN)
    {
        DBG_Printf("Rnd Virtual block alloc fail\n");
        DBG_Getch();
    }

    pInfo->m_TargetBlk[TARGET_HOST_GC] = VBN;
    pInfo->m_TargetPPO[TARGET_HOST_GC] = 0;

    /* record Block info in RPMT */
    for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
    {
        pInfo->m_pRPMT[TARGET_HOST_GC][ulRPMTPointer]->m_RPMT[uLUNInSuperPU].m_SuperPU = ucSuperPu;
        pInfo->m_pRPMT[TARGET_HOST_GC][ulRPMTPointer]->m_RPMT[uLUNInSuperPU].m_SuperBlock = VBN;
    }
    pInfo->m_RPMTBufferPointer[TARGET_HOST_GC] = ulRPMTPointer;

    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_HOST_GC;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_HOST;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;
    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]++;

    //Set PBIT block type
    L2_SetBlockType(ucSuperPu, VBN, BLOCK_TYPE_RAN);
}

void MCU1_DRAM_TEXT NewTLCWriteBlock(U8 ucSuperPu, U32 ulRPMTPointer)
{
    PuInfo* pInfo;
    U16 VBN;

    pInfo = g_PuInfo[ucSuperPu];

    /* allocate random target block */
    VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, TRUE);
    if (INVALID_4F == VBN)
    {
        DBG_Printf("TLCW Virtual block alloc fail\n");
        DBG_Getch();
    }

    pInfo->m_TargetBlk[TARGET_TLC_WRITE] = VBN;
    pInfo->m_TargetPPO[TARGET_TLC_WRITE] = 0;

    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_TLC_W;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_TLCW;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;
    pInfo->m_AllocateBlockCnt[BLKTYPE_TLC]++;

    //Set PBIT block type
    L2_SetBlockType(ucSuperPu, VBN, BLOCK_TYPE_TLC_W);
}

void MCU1_DRAM_TEXT L2_RebuildPuInfo(void)
{
    U32 uSuperPU;
    U8 ucTargetType;
    U32 FreePageCnt;
    PuInfo* pInfo;
    U16 ToLoadBlockSN;
    U32 usPhyBlkAddr;
    BLOCK_TYPE usBlockType;
    U32 i;
    U8 ucLunInSuperPu;
    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        for (ToLoadBlockSN = 0; ToLoadBlockSN < VIR_BLK_CNT; ToLoadBlockSN++)
        {
            L3_SetPuIDInVBT(uSuperPU, ToLoadBlockSN, uSuperPU);
            if (TRUE == L2_IsPBNEmtpy(uSuperPU, ToLoadBlockSN))
            {
                pVBT[uSuperPU]->m_VBT[ToLoadBlockSN].Target = VBT_TARGET_INVALID;
                continue;
            }
            else
            {
                pVBT[uSuperPU]->m_VBT[ToLoadBlockSN].Target = VBT_NOT_TARGET;

                //Rebuild VBT Type
                ucLunInSuperPu = L2_Rebuild_GetFirstUsedLUNOfSuperPage(uSuperPU, ToLoadBlockSN, 0);
                usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(uSuperPU, ucLunInSuperPu, ToLoadBlockSN);
                usBlockType = pPBIT[uSuperPU]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlkAddr].BlockType;

                /* all LUN consistency check */
                for (ucLunInSuperPu = 1; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    /*target ppo is 1, super page 0 partial write case */
                    usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(uSuperPU, ucLunInSuperPu, ToLoadBlockSN);
                    if (0 == pPBIT[uSuperPU]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlkAddr].BlockType)
                    {
                        continue;
                    }                    
                    if(usBlockType != pPBIT[uSuperPU]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlkAddr].BlockType)
                    {
                        DBG_Printf("L2_RebuildPuInfo SPU %d LUN %d PhyBlk %d BlockType check error\n", uSuperPU, ucLunInSuperPu, usPhyBlkAddr);
                        DBG_Getch();
                    }
                }

                if (BLOCK_TYPE_SEQ == usBlockType)
                {
                    pVBT[uSuperPU]->m_VBT[ToLoadBlockSN].VBTType = VBT_TYPE_HOST;
                }
                else if (BLOCK_TYPE_RAN == usBlockType)
                {
                    pVBT[uSuperPU]->m_VBT[ToLoadBlockSN].VBTType = VBT_TYPE_HOST;
                }
                else if (BLOCK_TYPE_TLC_W == usBlockType)
                {
                    pVBT[uSuperPU]->m_VBT[ToLoadBlockSN].VBTType = VBT_TYPE_TLCW;
                }
                else if (BLOCK_TYPE_TLC_GC == usBlockType)
                {
                    pVBT[uSuperPU]->m_VBT[ToLoadBlockSN].VBTType = VBT_TYPE_TLCW;
                }
                else
                {
                    DBG_Printf("SPU %d Blk %d Unknown block type %d \n", uSuperPU, ToLoadBlockSN, usBlockType);
                    DBG_Getch();
                }
            }
        }
        //record TLCGCBlkCnt
        pInfo = g_PuInfo[uSuperPU];

        l_RebuildPMTDptr->m_MaxTSOfPU[uSuperPU] = L2_TB_GetDataBlockMaxTS(uSuperPU);
    }

    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        pInfo = g_PuInfo[uSuperPU];

        for (i = 0; i < BLKTYPE_ALL; i++)
        {
            pInfo->m_DataBlockCnt[i] = pPBIT[uSuperPU]->m_TotalDataBlockCnt[i];
            pInfo->m_AllocateBlockCnt[i] = pPBIT[uSuperPU]->m_AllocatedDataBlockCnt[i];
        }
        pInfo->m_SLCMLCFreePageCnt = (pInfo->m_DataBlockCnt[BLKTYPE_SLC] - pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]) * (PG_PER_SLC_BLK);

        for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
        {
            if(SUPERPU_LUN_NUM_BITMSK == pPBIT[uSuperPU]->m_TargetOffsetBitMap[ucTargetType])
            {
                DBG_Printf("SPU %d TargetType %d TargetOffsetBitMap error\n", uSuperPU, ucTargetType, pPBIT[uSuperPU]->m_TargetOffsetBitMap[ucTargetType]);
                DBG_Getch();
            }
            if (INVALID_4F != pPBIT[uSuperPU]->m_TargetBlock[ucTargetType])
            {
                switch (ucTargetType)
                {
                case TARGET_HOST_WRITE_NORAML:
                    pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = pPBIT[uSuperPU]->m_TargetBlock[ucTargetType];
                    pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = pPBIT[uSuperPU]->m_TargetPPO[ucTargetType];
                    pInfo->m_TargetOffsetBitMap[TARGET_HOST_WRITE_NORAML] = pPBIT[uSuperPU]->m_TargetOffsetBitMap[ucTargetType];
                    pVBT[uSuperPU]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML]].Target = VBT_TARGET_HOST_W;
                    pVBT[uSuperPU]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML]].VBTType = VBT_TYPE_HOST;
                    FreePageCnt = PG_PER_SLC_BLK - pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                    //DBG_Printf("PU %d SeqBlock:0x%x,SeqPPO:0x%x\n", PUSer, pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML], pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML]);
                    break;

                case TARGET_HOST_GC:
                    pInfo->m_TargetBlk[TARGET_HOST_GC] = pPBIT[uSuperPU]->m_TargetBlock[ucTargetType];
                    pInfo->m_TargetPPO[TARGET_HOST_GC] = pPBIT[uSuperPU]->m_TargetPPO[ucTargetType];
                    pInfo->m_TargetOffsetBitMap[TARGET_HOST_GC] = pPBIT[uSuperPU]->m_TargetOffsetBitMap[ucTargetType];
                    pVBT[uSuperPU]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_GC]].Target = VBT_TARGET_HOST_GC;
                    pVBT[uSuperPU]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_GC]].VBTType = VBT_TYPE_HOST;
                    FreePageCnt = PG_PER_SLC_BLK - pInfo->m_TargetPPO[TARGET_HOST_GC];

                    //DBG_Printf("PU %d RndBlock:0x%x,RndPPO:0x%x\n", PUSer, pInfo->m_TargetBlk[TARGET_HOST_GC], pInfo->m_TargetPPO[TARGET_HOST_GC]);
                    break;

                case TARGET_TLC_WRITE:

                    DBG_Printf("can not have tlc target\n");
                    DBG_Getch();

                    pInfo->m_TargetBlk[TARGET_TLC_WRITE] = pPBIT[uSuperPU]->m_TargetBlock[ucTargetType];
                    pInfo->m_TargetPPO[TARGET_TLC_WRITE] = pPBIT[uSuperPU]->m_TargetPPO[ucTargetType];
                    pInfo->m_TargetOffsetBitMap[TARGET_TLC_WRITE] = pPBIT[uSuperPU]->m_TargetOffsetBitMap[ucTargetType];
                    pVBT[uSuperPU]->m_VBT[pInfo->m_TargetBlk[TARGET_TLC_WRITE]].Target = VBT_TARGET_TLC_W;
                    pVBT[uSuperPU]->m_VBT[pInfo->m_TargetBlk[TARGET_TLC_WRITE]].VBTType = VBT_TYPE_TLCW;
                    FreePageCnt = PG_PER_SLC_BLK - pInfo->m_TargetPPO[TARGET_TLC_WRITE];

                    //DBG_Printf("PU %d TLCWriteBlock:0x%x,TLCWritePPO:0x%x\n", PUSer, pInfo->m_TargetBlk[TARGET_TLC_WRITE], pInfo->m_TargetPPO[TARGET_TLC_WRITE]);
                    break;
                default:
                    DBG_Printf("Target type error\n");
                    DBG_Getch();
                    break;
                }
                if (ucTargetType != TARGET_TLC_WRITE)
                {
                    pInfo->m_RPMTBufferPointer[ucTargetType] = 0;
                    /* Super block first page partial write case : should finish L2_RecordRPMTPUBlockInfo do
                       or will Getch in L2_FlushCurrRPMT Check */
                    if (1 == pInfo->m_TargetPPO[ucTargetType] && 0 != pInfo->m_TargetOffsetBitMap[ucTargetType])
                    {
                        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                        {
                            if (!COM_BitMaskGet(pInfo->m_TargetOffsetBitMap[ucTargetType], ucLunInSuperPu))
                            {
                                pInfo->m_pRPMT[ucTargetType][0]->m_RPMT[ucLunInSuperPu].m_SuperPU = uSuperPU;
                                pInfo->m_pRPMT[ucTargetType][0]->m_RPMT[ucLunInSuperPu].m_SuperBlock = pInfo->m_TargetBlk[ucTargetType];
                            }
                        }
                    }
                    pInfo->m_SLCMLCFreePageCnt += FreePageCnt;
                }
            }
            else
            {
                switch (ucTargetType)
                {
                case TARGET_HOST_WRITE_NORAML:
                    NewSeqBlock(uSuperPU, 0);
                    break;

                case TARGET_HOST_GC:
                    NewRndBlock(uSuperPU, 0);
                    break;

                case TARGET_TLC_WRITE:
                    NewTLCWriteBlock(uSuperPU, 0);
                    break;

                }
            }
        }
    }

    return;
}

void L2_LLFPuInfo(U8 ucSuperPu)
{
    PuInfo* pInfo;
    U16 VBN;
    U8 ucLunInSuperPu;
    U8 i;
    TargetType eTargetType;
    U8 ucLUNIndex;

    pInfo = g_PuInfo[ucSuperPu];
    pInfo->m_TimeStamp = 0;

    for (eTargetType = TARGET_HOST_WRITE_NORAML; eTargetType < TARGET_ALL; eTargetType++)
    {
        pInfo->m_TargetOffsetBitMap[eTargetType] = 0;
        pInfo->m_RPMTFlushBitMap[eTargetType] = 0;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pInfo->m_TargetOffsetTS[eTargetType][ucLunInSuperPu] = 0;
        }
    }

    for (i = 0; i < BLKTYPE_ALL; i++)
    {
        if (BLKTYPE_SLC == i)
        {
            pInfo->m_DataBlockCnt[i] = pPBIT[ucSuperPu]->m_TotalDataBlockCnt[i] - pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[i]; // 2 block for WL target.
            pInfo->m_SLCMLCFreePageCnt = PG_PER_SLC_BLK * pInfo->m_DataBlockCnt[i];
        }
        else
        {
            pInfo->m_DataBlockCnt[i] = pPBIT[ucSuperPu]->m_TotalDataBlockCnt[i];
        }
    }


    /* allocate sequential target block */
    VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
    if (INVALID_4F == VBN)
    {
        DBG_Getch();
    }

    pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = VBN;
    pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = 0;

    /* record Block info in RPMT */
    for (ucLUNIndex = 0; ucLUNIndex < LUN_NUM_PER_SUPERPU; ucLUNIndex++)
    {
        pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0]->m_RPMT[ucLUNIndex].m_SuperPU = ucSuperPu;
        pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][0]->m_RPMT[ucLUNIndex].m_SuperBlock = VBN;
    }
    
    pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML] = 0;

    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_HOST_W;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_HOST;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;
    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC] ++;

    /* allocate host gc target block */
    VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
    if (INVALID_4F == VBN)
    {
        DBG_Getch();
    }

    pInfo->m_TargetBlk[TARGET_HOST_GC] = VBN;
    pInfo->m_TargetPPO[TARGET_HOST_GC] = 0;

    /* record Block info in RPMT */
    for (ucLUNIndex = 0; ucLUNIndex < LUN_NUM_PER_SUPERPU; ucLUNIndex++)
    {
        pInfo->m_pRPMT[TARGET_HOST_GC][0]->m_RPMT[ucLUNIndex].m_SuperPU = ucSuperPu;
        pInfo->m_pRPMT[TARGET_HOST_GC][0]->m_RPMT[ucLUNIndex].m_SuperBlock = VBN;
    }    
    pInfo->m_RPMTBufferPointer[TARGET_HOST_GC] = 0;

    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_HOST_GC;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_HOST;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;
    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC] ++;

    /* allocate tlc write target block */
    VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, TRUE);
    if (INVALID_4F == VBN)
    {
        DBG_Getch();
    }

    pInfo->m_TargetBlk[TARGET_TLC_WRITE] = VBN;
    pInfo->m_TargetPPO[TARGET_TLC_WRITE] = 0;


    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_TLC_W;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_TLCW;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;
    pInfo->m_AllocateBlockCnt[BLKTYPE_TLC] ++;

}

U8 L2_GetTargetOffsetTS(U32* pBitMap)
{
    U8 ucTargetOffsetTS = 0;
    U8 ucLunInSuperPu;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if ((*pBitMap) & (1 << ucLunInSuperPu))
        {
            ucTargetOffsetTS++;
        }
    }
    return ucTargetOffsetTS;
}
//////////////////////////////////////////////////////////////////////////
//U32 L2_FlushCurrRPMT(WriteType FlushType)
//function:
//    Flush current RPMT.
//    1) when the physical block is used up, the RPMT will be flushed into
//        last page of the block
//    2) when shutdown, RPMT will be flushed into current PPO
//////////////////////////////////////////////////////////////////////////
BOOL L2_FlushCurrRPMT(U8 ucSuperPu, TargetType FlushType)
{
    PuInfo* pInfo;
    PhysicalAddr Addr = { 0 };
    U16 PBN;
    U16 PPO;
    U8* pStatus;
    RPMT* pRPMTPage;
    RED Spare;//not used in current version.
    U32 ulRPMTBufferPointer;
    U16 i;
    U8 ucTLun, ucLunInSuperPu;

    pInfo = g_PuInfo[ucSuperPu];

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (0 == L2_FCMDQNotFull(ucTLun))
        {
            continue;
        }
        if (TRUE == COM_BitMaskGet(pInfo->m_RPMTFlushBitMap[FlushType], ucLunInSuperPu))
        {
            continue;
        }
        if (FlushType == TARGET_HOST_WRITE_NORAML)
        {
            PBN = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
            PPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
            Spare.m_RedComm.bcBlockType = BLOCK_TYPE_SEQ;

            if ((PG_PER_SLC_BLK - 1) != PPO)
            {
                DBG_Printf("flush RPMT error! PG_PER_SLC_BLK - 1) != PPO. \n");
                DBG_Getch();
            }
        }
        else if (FlushType == TARGET_HOST_GC)
        {
            PBN = pInfo->m_TargetBlk[TARGET_HOST_GC];
            PPO = pInfo->m_TargetPPO[TARGET_HOST_GC];
            Spare.m_RedComm.bcBlockType = BLOCK_TYPE_RAN;

            if ((PG_PER_SLC_BLK - 1) != PPO)
            {
                DBG_Printf("flush RPMT error! PG_PER_SLC_BLK - 1) != PPO. \n");
                DBG_Getch();
            }
        }

        Addr.m_PUSer = ucSuperPu;
        Addr.m_BlockInPU = PBN;
        Addr.m_PageInBlock = PPO;
        Addr.m_OffsetInSuperPage = ucLunInSuperPu;
        Addr.m_LPNInPage = 0;

        ulRPMTBufferPointer = pInfo->m_RPMTBufferPointer[FlushType];
        pStatus = &pInfo->m_RPMTBufferStatus[FlushType][ulRPMTBufferPointer][ucLunInSuperPu];
        pRPMTPage = pInfo->m_pRPMT[FlushType][ulRPMTBufferPointer];

        /* check */
        if ((pRPMTPage->m_RPMT[ucLunInSuperPu].m_SuperPU != ucSuperPu)
            || (pRPMTPage->m_RPMT[ucLunInSuperPu].m_SuperBlock != Addr.m_BlockInPU))
        {
            DBG_Printf("Flush RPMT Pu 0x%x Block 0x%x error,%d %d !\n", pRPMTPage->m_RPMT[ucLunInSuperPu].m_SuperPU, pRPMTPage->m_RPMT[ucLunInSuperPu].m_SuperBlock, ucSuperPu, Addr.m_BlockInPU);
            DBG_Getch();
        }

#ifdef SIM
        for (i = 0; i < LPN_PER_BUF; i++)
        {
            if (INVALID_8F != pRPMTPage->m_RPMT[ucLunInSuperPu].m_RPMTItems[i])
                break;
        }
        if (LPN_PER_BUF == i)
        {
            DBG_Printf("SPU %d Blk %d LUN %d RPMT all INVALID_8F\n", ucSuperPu, PBN, ucLunInSuperPu);
            DBG_Getch();
        }
#endif

        FillSpareAreaInWriteTable(&Spare, ucSuperPu, ucLunInSuperPu, PAGE_TYPE_RPMT, FlushType);     //Page Type RPMT
        Spare.m_RedComm.bsVirBlockAddr = Addr.m_BlockInPU;
        Spare.m_RedComm.eOPType = OP_TYPE_RPMT_WRITE;

        /* Update RPMT */
        COM_MemSet(&pRPMTPage->m_RPMT[ucLunInSuperPu].m_RPMTItems[PPO*LPN_PER_BUF], LPN_PER_BUF, INVALID_8F);

        pRPMTPage->m_RPMT[ucLunInSuperPu].m_SuperPageTS[PPO] = pInfo->m_TimeStamp;

        g_PuInfo[ucSuperPu]->m_TargetOffsetTS[FlushType][ucLunInSuperPu] = L2_GetTargetOffsetTS(&pInfo->m_RPMTFlushBitMap[FlushType]);
        pRPMTPage->m_RPMT[ucLunInSuperPu].m_LunOrderTS[PPO] = g_PuInfo[ucSuperPu]->m_TargetOffsetTS[FlushType][ucLunInSuperPu];

        L2_Set_DataBlock_PBIT_InfoLastTimeStamp(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, &Spare);

        //FIRMWARE_LogInfo("FlushRPMT SPU %d LUN %d Blk %d Pg %d\n", ucSuperPu, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, Addr.m_PageInBlock);

        L2_FtlWriteLocal(&Addr, (U32*)&pRPMTPage->m_RPMT[ucLunInSuperPu].m_Page , (U32*)&Spare, pStatus, FALSE, TRUE, NULL);

#ifdef L2MEASURE
        L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_RPMT_SLC);
#endif       
        COM_BitMaskSet(&pInfo->m_RPMTFlushBitMap[FlushType], ucLunInSuperPu);

    }//end for 

    if (SUPERPU_LUN_NUM_BITMSK == pInfo->m_RPMTFlushBitMap[FlushType])
    {
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
        L2_IncTimeStampInPu(ucSuperPu);
#endif
        pInfo->m_RPMTFlushBitMap[FlushType] = 0;

        /* record write cont for table flush */
        L2_IncDeviceWrite(ucSuperPu, 1);

        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

/****************************************************************************
Name        :L2_GetRPMTBufferPointer
Input       :U8 ucSuperPu, U16 CESerInPu, WriteType Type
Output      :RPMT Buffer Pointer
Author      :HenryLuo
Date        :2012.05.28    15:00:26
Description :search a free buffer for RPMT
Others      :
Modify      :
****************************************************************************/
U32 L2_GetRPMTBufferPointer(U8 ucSuperPu, TargetType Type)
{
    PuInfo* pInfo = g_PuInfo[ucSuperPu];
    U32 ulPointer;
    U32 i;

    ulPointer = (pInfo->m_RPMTBufferPointer[Type] + 1) % RPMT_BUFFER_DEPTH;

    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        /* check flush RPMT whether done */
        if (SUBSYSTEM_STATUS_SUCCESS != pInfo->m_RPMTBufferStatus[Type][ulPointer][i])
        {
            return INVALID_8F;
        }
    }

    return ulPointer;
}

BOOL L2_IsTLCFreeBlkEnough(U8 ucSuperPu)
{
    if (g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_TLC] - g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC] <= (GC_THRESHOLD_TLCNORMAL))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL L2_IsNeedTLCGC(U8 ucSuperPu)
{
    if ((g_PuInfo[ucSuperPu]->m_DataBlockCnt[BLKTYPE_TLC] - g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[BLKTYPE_TLC] + L2_GetEraseQueueSize(ucSuperPu, FALSE)) <= (GC_THRESHOLD_TLCNORMAL)
        || FALSE == L2_BlkQueueIsEmpty(g_pForceGCSrcBlkQueue[ucSuperPu], LINK_TYPE_TLCW))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////
//BOOL L2_IsNeedSLCGC(U8 ucSuperPu)
//function:
//    whether need perform GC.
//    in this version, GC one block is mux with write about 20 host pages.
//    so if free page cnt less than 20, return TRUE.
//////////////////////////////////////////////////////////////////////////
BOOL L2_IsNeedSLCGC(U8 ucSuperPu)
{
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPu];

    if ((pInfo->m_DataBlockCnt[BLKTYPE_SLC] - pInfo->m_AllocateBlockCnt[BLKTYPE_SLC] + L2_GetEraseQueueSize(ucSuperPu, TRUE)) <= (GC_THRESHOLD_BLOCK_CNT)
        || FALSE == L2_BlkQueueIsEmpty(g_pForceGCSrcBlkQueue[ucSuperPu], LINK_TYPE_HOST))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL L2_IsNeedIdleSLCGC(U8 ucSuperPu)
{
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPu];

    if ((TRUE == g_tFTLIdleGCMgr.bIdleGC)
        &&((pInfo->m_AllocateBlockCnt[BLKTYPE_SLC] - L2_GetEraseQueueSize(ucSuperPu, TRUE)) >= IDLEGC_THRESHOLD_BLOCK_CNT))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////
//U32 L2_AllocateOnePage(U8 ucSuperPu, U32* LPN, WriteType Type)
//function:
//    Allocate one free physical page, and set the RPMT
//parameters:
//    PUSer means the PUSer in the Pu
//
//////////////////////////////////////////////////////////////////////////
U32 L2_AllocateOnePage(U8 ucSuperPu, U32* LPN, TargetType Type, BLOCK_TYPE* pActualType)
{
    U16 BlockSN;
    U16 i = 0;
    U16 VBN;
    U16 PPO;
    TargetType ActualType;
    PhysicalAddr Addr = { 0 };
    PuInfo* pInfo;
    U32 ulRPMTBufferPointer;
    U16 CurrBlockSN;
    U32 Pointer;
    U8 ucLunInSuperPu;

    pInfo = g_PuInfo[ucSuperPu];

    ActualType = Type;
    //get the free page from seq block or rnd block.
    if (Type == TARGET_HOST_WRITE_NORAML)
    {
        if (pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] == (PG_PER_SLC_BLK - 1))
        {        
            if (0 == pInfo->m_RPMTFlushBitMap[TARGET_HOST_WRITE_NORAML])
            {
                L2_IncTimeStampInPu(ucSuperPu);
            }
            if (TRUE == L2_FlushCurrRPMT(ucSuperPu, Type))
            {
                pInfo->m_SLCMLCFreePageCnt--;
                pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML]++;
            }

            return INVALID_8F;
        }
        else if (pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] >= PG_PER_SLC_BLK)
        {
            if (pInfo->m_AllocateBlockCnt[BLKTYPE_SLC] >= pInfo->m_DataBlockCnt[BLKTYPE_SLC])
            {
            #if 0
                if (pInfo->m_TargetPPO[TARGET_HOST_GC] >= (PG_PER_SLC_BLK - 1))
                {
                    TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "[L2_AllocateOnePage]: PU ? no free page in write seq data!");

                    return INVALID_8F;
                }

                BlockSN = pInfo->m_TargetBlk[TARGET_HOST_GC];
                PPO = pInfo->m_TargetPPO[TARGET_HOST_GC];

                pInfo->m_TargetPPO[TARGET_HOST_GC]++;
                ActualType = TARGET_HOST_GC;
            #endif
                DBG_Printf("no free SLC blk\n");
                DBG_Getch();
            }
            else
            {
                /* set RPMT buffer pointer */
                Pointer = L2_GetRPMTBufferPointer(ucSuperPu, TARGET_HOST_WRITE_NORAML);
                if (INVALID_8F == Pointer)
                {
                    return INVALID_8F;
                }
                else
                {
                    if ((TRUE != L2_IsAdjustDirtyCntDone()) && (0 == Pointer))
                    {
                        DBG_Printf("PU %d seq write target overflowed during table rebuild !! \n", ucSuperPu);
                        DBG_Getch();
                    }

                    pInfo->m_RPMTBufferPointer[TARGET_HOST_WRITE_NORAML] = Pointer;
                }
#ifdef SIM
                GET_ALLOC_FREE_BLK_LOCK;
#else
                HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif                
                VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
#ifdef SIM
                RELEASE_ALLOC_FREE_BLK_LOCK;
#else
                HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif                
                if (INVALID_4F == VBN)
                {
                    TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "[L2_AllocateOnePage]: PU ? allocate free block failed!!");
                    return INVALID_8F;
                }
                else
                {
                    /* check dirty count when allocate a new block, add by henryluo 2013/02/06 */
#ifdef DirtyLPNCnt_IN_DSRAM1
                    if (0 != *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN) || VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[VBN].Target)
                    {
                        DBG_Printf("SPU %d Blk %d Target %d DirtyCnt %d error\n", ucSuperPu, VBN, pVBT[ucSuperPu]->m_VBT[VBN].Target, *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN));
                        DBG_Getch();
                    }
#else
                    if (0 != pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt || VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[VBN].Target)
                    {
                        DBG_Printf("SPU %d Blk %d Target %d DirtyCnt %d error\n", ucSuperPu, VBN, pVBT[ucSuperPu]->m_VBT[VBN].Target, pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt);
                        DBG_Getch();
                    }
#endif
                    /* record Block info in RPMT */
                    //pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][Pointer]->m_PU = ucSuperPu;
                    //pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][Pointer]->m_Block = VBN;

                    /* record Block info in RPMT */   
                    L2_RecordRPMTPUBlockInfo(pInfo->m_pRPMT[TARGET_HOST_WRITE_NORAML][Pointer], ucSuperPu, VBN);
                    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
                    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_HOST_W;
                    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_HOST;
                    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;

                    CurrBlockSN = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                    pVBT[ucSuperPu]->m_VBT[CurrBlockSN].Target = VBT_NOT_TARGET;

                    L2_Set_PBIT_BlkSN(ucSuperPu, CurrBlockSN);

                    pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] = 0;
                    PPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                    pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML] = VBN;
                    BlockSN = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                    pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML]++;

                    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]++;

                    //FIRMWARE_LogInfo("AllocateFreeBlk Pu %d VirBlk 0x%x HostW SLCBlk %d_Used %d\n", ucSuperPu, VBN,
                    //    pInfo->m_DataBlockCnt[BLKTYPE_SLC], pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]);
                }
            }
        }
        else
        {
            if (VBT_TARGET_HOST_W != pVBT[ucSuperPu]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML]].Target)
            {
                DBG_Printf("SPU %d Alloc page target %d error-1\n", ucSuperPu, pVBT[ucSuperPu]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML]].Target);
                DBG_Getch();
            }
            PPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
            BlockSN = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];

            pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML]++;
        }

    }
    else if (Type == TARGET_HOST_GC)
    {
        if (pInfo->m_TargetPPO[TARGET_HOST_GC] == (PG_PER_SLC_BLK - 1))
        {
            if (0 == pInfo->m_RPMTFlushBitMap[TARGET_HOST_GC])
            {
                VBN = pInfo->m_TargetBlk[TARGET_HOST_GC];             
                L2_IncTimeStampInPu(ucSuperPu);
            }

            if (TRUE == L2_FlushCurrRPMT(ucSuperPu, Type))
            {
                pInfo->m_SLCMLCFreePageCnt--;
                pInfo->m_TargetPPO[TARGET_HOST_GC]++;
            }

            return INVALID_8F;
        }

        if ((pInfo->m_TargetPPO[TARGET_HOST_GC] >= PG_PER_SLC_BLK)
            || (INVALID_4F == pInfo->m_TargetPPO[TARGET_HOST_GC]))
        {
            if (pInfo->m_AllocateBlockCnt[BLKTYPE_SLC] >= pInfo->m_DataBlockCnt[BLKTYPE_SLC])
            {
            #if 0
                if (pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML] >= (PG_PER_SLC_BLK - 1))
                {
                    TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "[L2_AllocateOnePage]: PU ? no free page in write rand data!");

                    return INVALID_8F;
                }

                BlockSN = pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML];
                PPO = pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML];
                pInfo->m_TargetPPO[TARGET_HOST_WRITE_NORAML]++;
                ActualType = TARGET_HOST_WRITE_NORAML;
            #endif
                DBG_Printf("no free SLC blk\n");
                DBG_Getch();
            }
            else
            {
                /* set RPMT buffer pointer */
                Pointer = L2_GetRPMTBufferPointer(ucSuperPu, TARGET_HOST_GC);
                if (INVALID_8F == Pointer)
                {
                    return INVALID_8F;
                }
                else
                {
                    pInfo->m_RPMTBufferPointer[TARGET_HOST_GC] = Pointer;
                }
#ifdef SIM
                GET_ALLOC_FREE_BLK_LOCK;
#else
                HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif                
                VBN = L2_BM_AllocateFreeBlock(ucSuperPu, BLOCK_TYPE_MIN_ERASE_CNT, FALSE);
#ifdef SIM
                RELEASE_ALLOC_FREE_BLK_LOCK;
#else
                HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif                
                if (INVALID_4F == VBN)
                {
                    TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "[L2_AllocateOnePage]: PU ? allocate free block failed!!");
                    return INVALID_8F;
                }
                else
                {
                    /* check dirty count when allocate a new block, add by henryluo 2013/02/06 */
#ifdef DirtyLPNCnt_IN_DSRAM1
                    if (0 != *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN) || VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[VBN].Target)
                    {
                        DBG_Printf("SPU %d Blk %d Target %d DirtyCnt %d error\n", ucSuperPu, VBN, pVBT[ucSuperPu]->m_VBT[VBN].Target, *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN));
                        DBG_Getch();
                    }
#else
                    if (0 != pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt || VBT_NOT_TARGET == pVBT[ucSuperPu]->m_VBT[VBN].Target)
                    {
                        DBG_Printf("SPU %d Blk %d Target %d DirtyCnt %d error\n", ucSuperPu, VBN, pVBT[ucSuperPu]->m_VBT[VBN].Target, pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt);
                        DBG_Getch();
                    }
#endif

                    /* record Block info in RPMT */
                    //pInfo->m_pRPMT[TARGET_HOST_GC][Pointer]->m_PU = ucSuperPu;
                    //pInfo->m_pRPMT[TARGET_HOST_GC][Pointer]->m_Block = VBN;
                    
                    /* record Block info in RPMT */   
                    L2_RecordRPMTPUBlockInfo(pInfo->m_pRPMT[TARGET_HOST_GC][Pointer], ucSuperPu, VBN);

                    pVBT[ucSuperPu]->m_VBT[VBN].StripID = ucSuperPu;
                    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_HOST_GC;
                    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_HOST;
                    pVBT[ucSuperPu]->m_VBT[VBN].bFree = FALSE;

                    CurrBlockSN = pInfo->m_TargetBlk[TARGET_HOST_GC];
                    if (INVALID_4F != CurrBlockSN)
                    {
                        pVBT[ucSuperPu]->m_VBT[CurrBlockSN].Target = VBT_NOT_TARGET;
                        L2_Set_PBIT_BlkSN(ucSuperPu, CurrBlockSN);
                    }
                    pInfo->m_TargetPPO[TARGET_HOST_GC] = 0;
                    PPO = pInfo->m_TargetPPO[TARGET_HOST_GC];
                    pInfo->m_TargetBlk[TARGET_HOST_GC] = VBN;
                    BlockSN = pInfo->m_TargetBlk[TARGET_HOST_GC];
                    pInfo->m_TargetPPO[TARGET_HOST_GC]++;

                    pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]++;

                    //FIRMWARE_LogInfo("AllocateFreeBlk Pu %d VirBlk 0x%x HostGC SLCBlk %d_Used %d\n", ucSuperPu, VBN,
                    //    pInfo->m_DataBlockCnt[BLKTYPE_SLC], pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]);
                }
            }
        }
        else
        {
            if (VBT_TARGET_HOST_GC != pVBT[ucSuperPu]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_GC]].Target)
            {
                DBG_Printf("SPU %d Alloc page target %d error-2\n", ucSuperPu, pVBT[ucSuperPu]->m_VBT[pInfo->m_TargetBlk[TARGET_HOST_GC]].Target);
                DBG_Getch();
            }
            PPO = pInfo->m_TargetPPO[TARGET_HOST_GC];
            BlockSN = pInfo->m_TargetBlk[TARGET_HOST_GC];

            pInfo->m_TargetPPO[TARGET_HOST_GC]++;
        }
    }
    else
    {
        DBG_Printf("SPU %d Alloc page error\n",ucSuperPu);
        DBG_Getch();
        return INVALID_8F;
    }

    //set the free page physical address.
    Addr.m_PUSer = ucSuperPu;
    Addr.m_BlockInPU = BlockSN;
    Addr.m_PageInBlock = PPO;
    Addr.m_OffsetInSuperPage = 0;
    Addr.m_LPNInPage = 0;
    *pActualType = ActualType + BLOCK_TYPE_SEQ;

    /* get the RPMT buffer pointer */
    ulRPMTBufferPointer = pInfo->m_RPMTBufferPointer[ActualType];

    L2_IncTimeStampInPu(ucSuperPu);

#ifndef L2_PMTREBUILD_SUPERPAGETS_NOTSAME    
    L2_RecordRPMTSuperPageTS(pInfo->m_pRPMT[ActualType][ulRPMTBufferPointer], PPO, pInfo->m_TimeStamp);
#endif
    pInfo->m_SLCMLCFreePageCnt--;
    pInfo->m_TargetOffsetBitMap[ActualType] = 0;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pInfo->m_TargetOffsetTS[ActualType][ucLunInSuperPu] = 0;
    }

    //record write cont for table flush    
    L2_IncDeviceWrite(ucSuperPu, 1);

    return Addr.m_PPN;
}

//////////////////////////////////////////////////////////////////////////
//BOOL L2_IsNeedWriteTrim(U8 ucSuperPu)
//function:
//    whether need perform Write Trim.
//////////////////////////////////////////////////////////////////////////
BOOL L2_IsNeedWriteTrim(U8 ucSuperPu)
{
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPu];

    if ((pInfo->m_DataBlockCnt - pInfo->m_AllocateBlockCnt) <= ((GC_THRESHOLD_BLOCK_CNT + 1) * 10))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL L2_IsTLCNeedGC(U8 ucSuperPu, U8 ucTLCGCTH)
{
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPu];

    if ((pInfo->m_DataBlockCnt[BLKTYPE_TLC] - pInfo->m_AllocateBlockCnt[BLKTYPE_TLC]) <= ucTLCGCTH)
    {

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL L2_IsNeedTrimLastDefenseMechanism(U8 ucSuperPu)
{
    PuInfo* pInfo;
    pInfo = g_PuInfo[ucSuperPu];
    U32 ulCurrentFreeBlockCount = (pInfo->m_DataBlockCnt[BLKTYPE_TLC] - pInfo->m_AllocateBlockCnt[BLKTYPE_TLC]);

    if (ulCurrentFreeBlockCount <= ((GC_THRESHOLD_BLOCK_CNT + 1)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

U32 L2_GetFreePageCntInPu(U8 ucSuperPu)
{
    PuInfo* pInfo = g_PuInfo[ucSuperPu];

    return pInfo->m_SLCMLCFreePageCnt;
}

U32 L2_GetDirtyCnt(U16 PUSer, U16 VBN)
{
#ifdef DirtyLPNCnt_IN_DSRAM1
    return *(g_pDirtyLPNCnt + PUSer*VIR_BLK_CNT + VBN);
#else
    return pVBT[PUSer]->m_VBT[VBN].DirtyLPNCnt;
#endif
}

//////////////////////////////////////////////////////////////////////////
//U16 SelectGCBlock(U16 PUSer)
//function:
//
//return value:
//the block SN of GC src block.
//////////////////////////////////////////////////////////////////////////

U16 L2_SelectGCBlock(U8 ucSuperPu, U8 ucSrcVBTType)
{
    U32 MaxDirtyCnt = 0;
    U16 SN = INVALID_4F;
    U32 ulCurTime_WL, ulCurTime_WL_MaxDirtyCnt = INVALID_8F;

    U16 i = 0, usBlkPhy;
    PuInfo* pInfo = NULL;
    U32 DirtyCnt = 0;

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    pInfo = g_PuInfo[ucSuperPu];
    for (i = 0; i < VIR_BLK_CNT; i++)
    {
        if (ucSuperPu != pVBT[ucSuperPu]->m_VBT[i].StripID)
        {
            continue;
        }

        if (VBT_NOT_TARGET != pVBT[ucSuperPu]->m_VBT[i].Target)
        {
            continue;
        }

        if (ucSrcVBTType != pVBT[ucSuperPu]->m_VBT[i].VBTType)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[i].bsInEraseQueue)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[i].bLockedInWL)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[i].bWaitEraseSts)
        {
            continue;
        }

        //Lock by GC
        if (TRUE == L2_PBIT_Get_Lock(ucSuperPu, i))
        {
            continue;
        }

        DirtyCnt = L2_GetDirtyCnt(ucSuperPu, i);
        if (DirtyCnt > MaxDirtyCnt)
        {
            MaxDirtyCnt = DirtyCnt;
            SN = i;
            usBlkPhy = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, i);
            ulCurTime_WL_MaxDirtyCnt = L2_Get_PBIT_BlkSN_Blk(ucSuperPu, 0, usBlkPhy);
        }
        else if (DirtyCnt == 0 && DirtyCnt == MaxDirtyCnt)
        {
            /* Select min SN when both DirtyCnt is equal as zero */
            usBlkPhy = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, i);
            ulCurTime_WL = L2_Get_PBIT_BlkSN_Blk(ucSuperPu, 0, usBlkPhy);
            if (ulCurTime_WL < ulCurTime_WL_MaxDirtyCnt)
            {
                ulCurTime_WL_MaxDirtyCnt = ulCurTime_WL;
                SN = i;
            }
        }
    }

    return SN;
}

U16 L2_SelectGCSrcBlkMinSN(U8 ucSuperPu, U8 ucTLCBlk, U8 ucVBTType)
{
    U16 i, usBlkPhy, usBlkVBN;
    U32 ulCurTime_WL, ulMinSerialNum;
    PuInfo* pInfo;

    pInfo = g_PuInfo[ucSuperPu];
    ulMinSerialNum = INVALID_8F;
    usBlkVBN = INVALID_4F;

    for (i = 0; i < VIR_BLK_CNT; i++)
    {
        if (ucSuperPu != pVBT[ucSuperPu]->m_VBT[i].StripID)
        {
            continue;
        }

        if (VBT_NOT_TARGET != pVBT[ucSuperPu]->m_VBT[i].Target)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[i].bsInEraseQueue)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[i].bWaitEraseSts)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[i].bLockedInWL)
        {
            continue;
        }

        if (ucTLCBlk != pVBT[ucSuperPu]->m_VBT[i].bTLC)
        {
            continue;
        }

        if (ucVBTType != pVBT[ucSuperPu]->m_VBT[i].VBTType)
        {
            continue;
        }

        /* min SN */
        usBlkPhy = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, i);
        ulCurTime_WL = L2_Get_PBIT_BlkSN_Blk(ucSuperPu, 0, usBlkPhy);
        if (ulCurTime_WL < ulMinSerialNum)
        {
            ulMinSerialNum = ulCurTime_WL;
            usBlkVBN = pPBIT[ucSuperPu]->m_PBIT_Entry[0][usBlkPhy].VirtualBlockAddr;
        }
    }
    return usBlkVBN;
}

void L2_RecycleBlock(U8 ucSuperPu, U16 BlockSN)
{
    PuInfo* pInfo;

    if (VBT_NOT_TARGET != pVBT[ucSuperPu]->m_VBT[BlockSN].Target)
    {
        DBG_Printf("ucSuperPu %d VirBlk %d Target %d error!\n", ucSuperPu, BlockSN, pVBT[ucSuperPu]->m_VBT[BlockSN].Target);
        DBG_Getch();
    }

    pInfo = g_PuInfo[ucSuperPu];
    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, BlockSN))
    {
        pInfo->m_AllocateBlockCnt[BLKTYPE_TLC]--;
    }
    else
    {
        pInfo->m_AllocateBlockCnt[BLKTYPE_SLC]--;
        pInfo->m_SLCMLCFreePageCnt += (PG_PER_SLC_BLK);
    }

    return;
}

void L2_IncreaseDirty(U16 ucSuperPu, U16 VirtualBlockSN, U32 DirtyCnt)
{
    //Enable under SIM or ASIC_BUGCHECK defined.
#if defined(SIM) || defined(ASIC_BUGCHECK)
    U32 ulTotalLpnPerBlk;

    if (VirtualBlockSN > VIR_BLK_CNT)
    {
        DBG_Printf("ce %d, block %d ,block sn larger than max value in set dirty!!!\n", ucSuperPu, VirtualBlockSN);
        DBG_Getch();
    }
#endif

#ifdef DirtyLPNCnt_IN_DSRAM1
    *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VirtualBlockSN) += DirtyCnt;
#else
    pVBT[ucSuperPu]->m_VBT[VirtualBlockSN].DirtyLPNCnt += DirtyCnt;
#endif

#if defined(SIM) || defined(ASIC_BUGCHECK)
    if (TRUE == pVBT[ucSuperPu]->m_VBT[VirtualBlockSN].bTLC)
    {
        ulTotalLpnPerBlk = LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT;
    }
    else
    {
        ulTotalLpnPerBlk = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;
    }

#ifdef DirtyLPNCnt_IN_DSRAM1
    if (*(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VirtualBlockSN) > ulTotalLpnPerBlk)
#else
    if (pVBT[ucSuperPu]->m_VBT[VirtualBlockSN].DirtyLPNCnt > ulTotalLpnPerBlk)
#endif
    {
        DBG_Printf("ce %d, block %d dirtycnt overflowed!!!\n", ucSuperPu, VirtualBlockSN);
        DBG_Getch();
    }
#endif
}

/*****************************************************************************
 Prototype      : L2_CheckDirtyCnt
 Description    :
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/12/23
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_CheckDirtyCnt(U8 ucPU, U16 usVBN)
{
    PhysicalAddr Addr = { 0 };
    U32 ValidCnt = 0;
    U16 Page;
    U16 LpnInPage;
    U32 DirtyCnt;
    U32 usTotalLPNCnt;
    BOOL bTLCFlag;
    U32 PageCnt;
    U32 uLunInSuperPu;

    bTLCFlag = L2_VBT_Get_TLC(ucPU, usVBN);

    if (bTLCFlag)
    {
        PageCnt = PG_PER_SLC_BLK * PG_PER_WL;
    }
    else
    {
        PageCnt = PG_PER_SLC_BLK;
    }  

    for (uLunInSuperPu = 0; uLunInSuperPu < LUN_NUM_PER_SUPERPU; uLunInSuperPu++)
    {
        for (Page = 0; Page < PageCnt; Page++)
        {
            Addr.m_PUSer = ucPU;
            Addr.m_BlockInPU = usVBN;
            Addr.m_OffsetInSuperPage = uLunInSuperPu;
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

    DirtyCnt = L2_GetDirtyCnt(ucPU, usVBN);

    if (TRUE == L2_VBT_Get_TLC(ucPU, usVBN))
    {
        usTotalLPNCnt = LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT;
    }
    else
    {
        usTotalLPNCnt = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;
    }

    if (usTotalLPNCnt != (DirtyCnt + ValidCnt))
    {
        DBG_Printf("ce %d, block %d dirtycnt %d ValidCnt %d\n", ucPU, usVBN, DirtyCnt, ValidCnt);
        DBG_Getch();
    }
}

U32 L2_GetTimeStampInPu(U8 ucSuperPu)
{
    PuInfo* pInfo = g_PuInfo[ucSuperPu];
    
    return pInfo->m_TimeStamp;
}

void L2_IncTimeStampInPu(U8 ucSuperPu)
{
    PuInfo* pInfo = g_PuInfo[ucSuperPu];
    
    pInfo->m_TimeStamp++;
}

void L3_SetPuIDInVBT(U32 uSuperPU, U32 VBN, U32 ucSuperPu)
{
    pVBT[uSuperPU]->m_VBT[VBN].StripID = ucSuperPu;
}




