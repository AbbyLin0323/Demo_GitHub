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
File Name     : L2_PBIT.c
Version       : Initial version
Author        : henryluo
Created       : 2015/2/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2012/01/07
Author      : peterxiu
Modification: Created file

*******************************************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "COM_BitMask.h"
#include "L2_Defines.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_TableBlock.h"
#include "L2_PBIT.h"
#include "L2_Interface.h"
#include "L2_SearchEngine.h"
#include "L2_Thread.h"
#include "L2_TLCMerge.h"
#include "L2_StaticWL.h"
#include "COM_Memory.h"
#include "FW_Event.h"
#include "L3_FlashMonitor.h"
#ifdef SIM
#include "sim_search_engine.h"
#endif

#ifdef DBG_LC
extern GLOBAL  LC lc;
extern GLOBAL  LC *dbg_lc;
#endif

LOCAL MCU12_VAR_ATTR S32 nMayBeSelCnt_S = 0;
LOCAL MCU12_VAR_ATTR S32 nMayBeSelCnt_E = 0;

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;
#endif

extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
extern void FW_DbgUpdateStaticInfo(void);

void MCU1_DRAM_TEXT L2_PBIT_Init_Clear_All(void)
{
    U8 ucSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        HAL_DMAESetValue((U32)pPBIT[ucSuperPu], COM_MemSize16DWAlign(sizeof(PBIT)), 0);

        pPBIT[ucSuperPu]->ulECMin = INVALID_4F;
        pPBIT[ucSuperPu]->ulECMinCnt = 0;
    }

    return;
}

void MCU1_DRAM_TEXT L2_PBIT_Init(BOOL bLLF, BOOL bKeepEraseCnt)
{
    U8  ucSuperPu;
    U16 usBlock;
    U8  ucTargetType;
    U8  ucLunInSuperPu;
    U8 ucBlktype;
    U8 ucTLun;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        if (bLLF)
        {
           if (TRUE == bKeepEraseCnt)
           {
               pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_SLC] += SLC_BLK_CNT *(LUN_NUM_PER_SUPERPU);
               pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] += (TLC_BLK_CNT * (LUN_NUM_PER_SUPERPU));
           }
           else
           {
              pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_SLC] = SLC_BLK_CNT *(LUN_NUM_PER_SUPERPU);
              pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] = (TLC_BLK_CNT * (LUN_NUM_PER_SUPERPU));
           }
        }
        for (ucBlktype = 0; ucBlktype < BLKTYPE_ALL; ucBlktype++)
        {
            pPBIT[ucSuperPu]->m_TotalDataBlockCnt[ucBlktype] = 0;
            pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[ucBlktype] = 0;
        }

        for (ucTargetType = 0; ucTargetType < TARGET_ALL; ucTargetType++)
        {
            pPBIT[ucSuperPu]->m_TargetBlock[ucTargetType] = INVALID_4F;
            pPBIT[ucSuperPu]->m_TargetPPO[ucTargetType] = INVALID_4F;
        }

        pPBIT[ucSuperPu]->ulECMin = INVALID_4F;
        pPBIT[ucSuperPu]->ulECMinCnt = 0;
        pPBIT[ucSuperPu]->m_WLDstBlk = INVALID_4F;
        pPBIT[ucSuperPu]->m_WLSrcBlk = INVALID_4F;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (usBlock = 0; usBlock < (BLK_PER_LUN + RSVD_BLK_PER_LUN); usBlock++)
            {
                if (TRUE == bKeepEraseCnt)
                {
                    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt++;
                }
                else
                {
                    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].EraseCnt = 1;
                }

                pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].VirtualBlockAddr = INVALID_4F;
                pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].Value[1] = 0;

                pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usBlock].Value[0] = INVALID_8F;
                pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usBlock].Value[1] = 0;
                pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usBlock].Value[2] = 0;

#ifdef L2MEASURE
                pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usBlock].ECPerPOR = 1;
#endif
            }
        }
    }

    if (bLLF)
    {
        for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
        {
            g_ptFMUserMgr[ucTLun].ulErsTime = pPBIT[0]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU;
        }
    }

    return;
}

void MCU1_DRAM_TEXT L2_PBIT_Save_UpdateData(U8 ucSuperPu)
{
    U8 i;
    COMMON_EVENT L2_Event;

    for (i = 0; i < BLKTYPE_ALL; i++)
    {
        pPBIT[ucSuperPu]->m_TotalDataBlockCnt[i] = g_PuInfo[ucSuperPu]->m_DataBlockCnt[i];
        pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[i] = g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[i];
        pPBIT[ucSuperPu]->m_SaveEraseCnt[i] = pPBIT[ucSuperPu]->m_EraseCnt[i];
    }
    pPBIT[ucSuperPu]->m_FreePageCnt = g_PuInfo[ucSuperPu]->m_SLCMLCFreePageCnt;

    for (i = 0; i < TARGET_ALL; i++)
    {
        pPBIT[ucSuperPu]->m_TargetBlock[i] = g_PuInfo[ucSuperPu]->m_TargetBlk[i];
        pPBIT[ucSuperPu]->m_TargetPPO[i] = g_PuInfo[ucSuperPu]->m_TargetPPO[i];
        pPBIT[ucSuperPu]->m_TargetOffsetBitMap[i] = g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[i];
        pPBIT[ucSuperPu]->m_RPMTFlushBitMap[i] = g_PuInfo[ucSuperPu]->m_RPMTFlushBitMap[i];
    }

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2    
    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);
    
    if (L2_Event.EventShutDown)
    {
        if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu])
	    {
		    pPBIT[ucSuperPu]->m_WLDstBlk = gwl_info->nDstBlk[ucSuperPu];
	    }
	    else
	    {
		   if (gwl_info->nDstBlk[ucSuperPu] != INVALID_4F)
		   {
	  	       L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
			   gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
			   DBG_Printf("L2_PBIT_Save_UpdateData() gwl_info->nDstBlk[ucSuperPu] = INVALID_4F\n");				
		   }
	       pPBIT[ucSuperPu]->m_WLDstBlk = INVALID_4F;
	    }
	}
	else
    {
        pPBIT[ucSuperPu]->m_WLDstBlk = gwl_info->nDstBlk[ucSuperPu];
    }
#else
    pPBIT[ucSuperPu]->m_WLDstBlk = gwl_info->nDstBlk[ucSuperPu];
#endif

    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);
    
    if (L2_Event.EventShutDown)
    {
    #ifdef NEW_SWL
        //reset the buffered swl src blk
        if (INVALID_4F != gwl_info->nSrcBlkBuf[ucSuperPu])
        {
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
            gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
        }
    #endif

        if (INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] && 0 == gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu])
        {
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu], FALSE);

            /* unlock SWL srcBlk, and set SWL srcBlk to INVALID, because SATA mode has standy cmd,
             it may only send shutdown event,but no BootUp */
        #ifdef HOST_SATA
            L2_SWLClear(ucSuperPu, FALSE);
        #endif
        }

        pPBIT[ucSuperPu]->m_WLSrcBlk = INVALID_4F;
        pPBIT[ucSuperPu]->m_CurPage = INVALID_8F;
    }
    else
    {
        pPBIT[ucSuperPu]->m_WLSrcBlk = gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu];
        pPBIT[ucSuperPu]->m_CurPage = gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu];
    }

#ifdef DBG_LC
    dbg_lc->uLockCounter[ucSuperPu][DATA_GC] = lc.uLockCounter[ucSuperPu][DATA_GC];
    dbg_lc->uLockCounter[ucSuperPu][STATIC_WL] = lc.uLockCounter[ucSuperPu][STATIC_WL];
#endif

    return;
}

void MCU1_DRAM_TEXT L2_PBIT_Load_ResumeData(U8 ucSuperPu)
{
    U8 i;

    for (i = 0; i < BLKTYPE_ALL; i++)
    {
        g_PuInfo[ucSuperPu]->m_DataBlockCnt[i] = pPBIT[ucSuperPu]->m_TotalDataBlockCnt[i];
        g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[i] = pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[i];
    }
    g_PuInfo[ucSuperPu]->m_SLCMLCFreePageCnt = pPBIT[ucSuperPu]->m_FreePageCnt;

    for (i = 0; i < TARGET_ALL; i++)
    {
        g_PuInfo[ucSuperPu]->m_TargetBlk[i] = pPBIT[ucSuperPu]->m_TargetBlock[i];
        g_PuInfo[ucSuperPu]->m_TargetPPO[i] = pPBIT[ucSuperPu]->m_TargetPPO[i];
        g_PuInfo[ucSuperPu]->m_TargetOffsetBitMap[i] = pPBIT[ucSuperPu]->m_TargetOffsetBitMap[i];
        g_PuInfo[ucSuperPu]->m_RPMTFlushBitMap[i] = pPBIT[ucSuperPu]->m_RPMTFlushBitMap[i];
    }

    gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = pPBIT[ucSuperPu]->m_WLSrcBlk;

    gwl_info->nDstBlk[ucSuperPu] = pPBIT[ucSuperPu]->m_WLDstBlk;
    gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu] = pPBIT[ucSuperPu]->m_CurPage;

#ifdef DBG_LC
    lc.uLockCounter[ucSuperPu][DATA_GC] = dbg_lc->uLockCounter[ucSuperPu][DATA_GC];
    lc.uLockCounter[ucSuperPu][STATIC_WL] = dbg_lc->uLockCounter[ucSuperPu][STATIC_WL];
#endif
    return;
}

void MCU1_DRAM_TEXT L2_Set_DataBlock_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usVirBlock, RED* pRed)
{
    U16 usPhyBlock;

    usPhyBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, usVirBlock);

    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlock].BlockType = pRed->m_RedComm.bcBlockType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].PageType = pRed->m_RedComm.bcPageType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].OPType = pRed->m_RedComm.eOPType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].TimeStamp = pRed->m_RedComm.ulTimeStamp;

    return;
}

void MCU1_DRAM_TEXT L2_Set_DataBlock_PBIT_InfoLastTimeStamp(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usVirBlock, RED* pRed)
{
    U16 usPhyBlock;

    usPhyBlock = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, usVirBlock);
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].LastPageTimeStamp = pRed->m_RedComm.ulTimeStamp;

    return;
}

void MCU1_DRAM_TEXT L2_Set_TableBlock_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPhyBlock, RED* pRed)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlock].BlockType = pRed->m_RedComm.bcBlockType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].PageType = pRed->m_RedComm.bcPageType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].OPType = pRed->m_RedComm.eOPType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].TimeStamp = pRed->m_RedComm.ulTimeStamp;

    return;
}
void MCU1_DRAM_TEXT L2_Set_TableBlock_PBIT_Info_TLC(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPhyBlock, RED* pRed)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlock].BlockType = pRed->m_RedComm.bcBlockType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].PageType = pRed->m_RedComm.bcPageType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].OPType = pRed->m_RedComm.eOPType;

    return;
}



extern U32 MCU1_DRAM_TEXT L2_Rebuild_GetFirstUsedLUNOfSuperPage(U32 uSuperPU, U32 uVirtualBlock, U32 uPageNum);
void MCU1_DRAM_TEXT L2_PBIT_GetFirstPageRedInfo(U8 ucSuperPu, U16 usVirBlkAddr, U16 Page, RED *pRed)
{
    U8  ucLoop;
    U16 usPhyBlkAddr;
    U8 ucLunInSuperPu = L2_Rebuild_GetFirstUsedLUNOfSuperPage(ucSuperPu, usVirBlkAddr, 0);

    for (ucLoop = 0; ucLoop < RED_SW_SZ_DW; ucLoop++)
    {
        pRed->m_Content[ucLoop] = 0;
    }

    if (usVirBlkAddr >= VIR_BLK_CNT)
    {
        return;
    }

    usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, usVirBlkAddr);

    if (INVALID_4F == usPhyBlkAddr)
    {
        return;
    }

    pRed->m_RedComm.bcBlockType = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlkAddr].BlockType;
    if (Page == (PG_PER_SLC_BLK - 1))
    {
        pRed->m_RedComm.bcPageType = PAGE_TYPE_RPMT;
    }
    else
    {
        pRed->m_RedComm.bcPageType = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlkAddr].PageType;
    }
    pRed->m_RedComm.eOPType = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlkAddr].OPType;
    pRed->m_RedComm.ulTimeStamp = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlkAddr].TimeStamp;
    pRed->m_RedComm.bsVirBlockAddr = usVirBlkAddr;
    return;
}

void MCU1_DRAM_TEXT L2_PBIT_SetLastPageRedInfo(U8 ucSuperPu, U32 ucLunInSuperPu,U16 usVirBlkAddr, RED *pRed)
{
    U32 i;
    U16 usPhyBlkAddr = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, usVirBlkAddr);

    for (i = 0; i < LPN_PER_BUF; i++)
    {
        pRed->m_DataRed.aCurrLPN[i] = INVALID_8F;
    }
    pRed->m_RedComm.bcPageType = PAGE_TYPE_RPMT;
    pRed->m_RedComm.eOPType = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlkAddr].OPType;
    pRed->m_RedComm.ulTimeStamp = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlkAddr].LastPageTimeStamp;
    pRed->m_RedComm.bsVirBlockAddr = usVirBlkAddr;

    return;
}

void MCU1_DRAM_TEXT L2_PBIT_Set_Reserve(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bReserved = bTrue;
}


void MCU12_DRAM_TEXT L2_PBIT_Set_Broken(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bBroken = bTrue;
}

void MCU1_DRAM_TEXT L2_PBIT_Set_PatrolRead(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bPatrolRead = bTrue;
}

void MCU1_DRAM_TEXT L2_PBIT_Increase_AllocBlockCnt(U8 ucSuperPu, U8 ucBlkType)
{
    pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[ucBlkType]++;
}

void MCU1_DRAM_TEXT L2_PBIT_Increase_TotalBlockCnt(U8 ucSuperPu, U8 ucBlkType)
{
    pPBIT[ucSuperPu]->m_TotalDataBlockCnt[ucBlkType]++;
}

void MCU12_DRAM_TEXT L2_PBIT_Decrease_TotalBlockCnt(U8 ucSuperPu, U8 ucBlkType)
{
    pPBIT[ucSuperPu]->m_TotalDataBlockCnt[ucBlkType]--;

    return;
}

void MCU1_DRAM_TEXT L2_PBIT_SetEvent_SavePbit(U8 ucSuperPu, U8 ucBlkType)
{
    COMM_EVENT_PARAMETER *pParameter;
    U32 ulL2L3_DiffEraseCnt;

    if ((pPBIT[ucSuperPu]->m_EraseCnt[ucBlkType] - pPBIT[ucSuperPu]->m_SaveEraseCnt[ucBlkType]) >= THRES_ERASE_CNT)
    {
        //Set Event SavePbit
        CommGetEventParameter(COMM_EVENT_OWNER_L2, &pParameter);

        pParameter->EventParameterNormal[COMM_EVENT_PARAMETER_LEN - 1] |= (1 << ucSuperPu);
        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_PBIT);
        DBG_Printf("SavePbit Pu:%d BlkType %d SaveEraseCnt:0x%x EraseCnt:0x%x\n", ucSuperPu, ucBlkType, pPBIT[ucSuperPu]->m_SaveEraseCnt[ucBlkType], pPBIT[ucSuperPu]->m_EraseCnt[ucBlkType]);

        FW_DbgUpdateStaticInfo();
        
        if((g_pSubSystemDevParamPage->TotalEraseCount/SUBSYSTEM_SUPERPU_NUM) > pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC])
           ulL2L3_DiffEraseCnt = (g_pSubSystemDevParamPage->TotalEraseCount/SUBSYSTEM_SUPERPU_NUM) - pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC];
        else 
           ulL2L3_DiffEraseCnt = pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] - (g_pSubSystemDevParamPage->TotalEraseCount/SUBSYSTEM_SUPERPU_NUM);
   
        DBG_Printf("SavePbit L3:0x%x L2:0x%x D:0x%x DP:%d\n", (g_pSubSystemDevParamPage->TotalEraseCount/SUBSYSTEM_SUPERPU_NUM), pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC],
                    ulL2L3_DiffEraseCnt, ulL2L3_DiffEraseCnt/LUN_NUM_PER_SUPERPU);
    }
}

U32 MCU1_DRAM_TEXT L2_PBIT_Get_EraseCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN)
{
    return pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].EraseCnt;
}


BOOL MCU1_DRAM_TEXT L2_PBIT_Get_Weak(U8 ucSuperPu, U16 VirBlockAddr)
{
    U8 ucLunInSuperPu;
    U16 PBN;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        PBN = pVBT[ucSuperPu]->m_VBT[VirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu];

        if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bWeak)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL MCU1_DRAM_TEXT L2_PBIT_Get_RetryFail(U8 ucSuperPu, U16 PhyBlockAddr, U8 ucLunInSuperPu)
{
    if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bRetryFail)
    {
        return TRUE;
    }

    return FALSE;
}
void MCU1_DRAM_TEXT L2_PBIT_VBT_Swap_Block(U8 ucSuperPu, U16 VBN1, U16 VBN2)
{
    U16 PhyBlock1, PhyBlock2;
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        PhyBlock1 = pVBT[ucSuperPu]->m_VBT[VBN1].PhysicalBlockAddr[ucLunInSuperPu];
        PhyBlock2 = pVBT[ucSuperPu]->m_VBT[VBN2].PhysicalBlockAddr[ucLunInSuperPu];

        pVBT[ucSuperPu]->m_VBT[VBN1].PhysicalBlockAddr[ucLunInSuperPu] = PhyBlock2;
        pVBT[ucSuperPu]->m_VBT[VBN2].PhysicalBlockAddr[ucLunInSuperPu] = PhyBlock1;
        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlock1].VirtualBlockAddr = VBN2;
        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlock2].VirtualBlockAddr = VBN1;
    }

    return;
}

//////////////////////////////////////////////////////////////////////////
//input parameters:
//ucSuperPu
//Virtual Block SN.
//description: PBN in Layer2 is VBN.
//////////////////////////////////////////////////////////////////////////
BOOL MCU1_DRAM_TEXT L2_IsPBNEmtpy(U8 ucSuperPu, U32 VBN)
{
    U16 PhysicalBlockAddr;
    U32 IsEmpty = TRUE;
    U8 ucLunInSuperPu = 0;
    U8 bShouldAllInvalid = FALSE;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        PhysicalBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
        if (INVALID_4F == PhysicalBlockAddr)
        {
            //DBG_Printf("%s :Attention  CE %d VirBlk %d has no physical blk ",__FUNCTION__,ucSuperPu,VBN);
            bShouldAllInvalid = TRUE;
            continue;
        }
        else
        {
            if (bShouldAllInvalid)
            {
                U32 i;
                DBG_Printf("%s: %d VBLK %d partial have PBLK mapping\n", __FUNCTION__, ucSuperPu, VBN);
                for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
                {
                    DBG_Printf("LUN %d PBLK %d\n", i, pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[i]);
                }                
                DBG_Getch();
            }
        }

        IsEmpty = IsEmpty && pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhysicalBlockAddr].bFree;
    }

    return (BOOL)IsEmpty;
}

BOOL MCU1_DRAM_TEXT L2_IsPBNError(U8 ucSuperPu, U32 VBN)
{
    U16 PhysicalBlockAddr;
    U8 ucLunInSuperPu = 0;

    PhysicalBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];

    if (INVALID_4F == PhysicalBlockAddr)
    {
        return TRUE;
    }
    return (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhysicalBlockAddr].bError);
}



void MCU1_DRAM_TEXT L2_BM_LLF_Init_BlkSN(void)
{
    U8  ucSuperPu;
    U16 usBlockIndex;
    U8 ucLunInSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        pPBIT[ucSuperPu]->m_CurBlkSN = 0;
        pPBIT[ucSuperPu]->m_MinBlkSN = 0;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (usBlockIndex = g_ulDataBlockStart[ucSuperPu][ucLunInSuperPu]; usBlockIndex < BLK_PER_LUN + RSVD_BLK_PER_LUN; usBlockIndex++)
            {
                pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usBlockIndex].CurSerialNum = 0;
            }
        }
    }

    return;
}

U32 L2_Get_PBIT_BlkSN_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN)
{
    return pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][PBN].CurSerialNum;
}

U32 L2_Get_PBIT_PTR_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN)
{
    return pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][PBN].LastPageTimeStamp;
}

void L2_Set_PBIT_BlkSN_CE(U8 ucSuperPu, U32 Value)
{
    pPBIT[ucSuperPu]->m_CurBlkSN = Value;
}
U32 L2_Get_PBIT_BlkSN_CE(U8 ucSuperPu)
{
    return pPBIT[ucSuperPu]->m_CurBlkSN;
}

void L2_Set_PBIT_MinBlkSN_CE(U8 ucSuperPu, U32 Value)
{
    pPBIT[ucSuperPu]->m_MinBlkSN = Value;
}
U32 L2_Get_PBIT_MinBlkSN_CE(U8 ucSuperPu)
{
    return pPBIT[ucSuperPu]->m_MinBlkSN;
}

void L2_Set_PBIT_BlkSN(U8 ucSuperPu, U16 VBN)
{
    U32 ulBlkSNTemp = INVALID_8F;
    U16 usPos = INVALID_4F;
    U8 ucLunInSuperPu;

    ulBlkSNTemp = L2_Get_PBIT_BlkSN_CE(ucSuperPu);
    ++ulBlkSNTemp;
    L2_Set_PBIT_BlkSN_CE(ucSuperPu, ulBlkSNTemp);
    

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        usPos = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, ucLunInSuperPu, VBN);
        L2_Set_PBIT_BlkSN_Blk(ucSuperPu, ucLunInSuperPu, usPos, ulBlkSNTemp);
    }
}


//Select SrcBlk according to serial number
U16 MCU1_DRAM_TEXT L2_Get_WLSrcBlk_S(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PhyBlkDst, BOOL bTLC)
{

#ifdef NEW_SWL
    U16 uBlkVBN[2] = { INVALID_4F, INVALID_4F };
    U32 MinSerialNum[2] = { INVALID_8F, INVALID_8F };
    U32 CurTime_WL = 0;
#else
    U16 uBlkVBN = INVALID_4F;
    U32 MinSerialNum = INVALID_8F;
    U32 CurTime_WL = 0;
#endif 
    U16 uVirBlk = 0;
    U16 uBlkPhy = 0;
    PBIT_ENTRY* pPBIT_Entry;
    U16 EraseCntDst = INVALID_4F, EraseCntSrc = INVALID_4F;

    pPBIT_Entry = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];
    EraseCntDst = pPBIT_Entry[PhyBlkDst].EraseCnt;
#ifdef NEW_SWL
    U32 ulECave = (pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU) / TLC_BLK_CNT;
#endif

    for (uBlkPhy = g_ulDataBlockStart[ucSuperPu][ucLunInSuperPu]; uBlkPhy < BLK_PER_LUN + RSVD_BLK_PER_LUN; uBlkPhy++)
    {
        uVirBlk = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        if (uVirBlk == INVALID_4F)
            continue;

        if (VBT_NOT_TARGET != pVBT[ucSuperPu]->m_VBT[uVirBlk].Target)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[uVirBlk].bsInEraseQueue)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[uVirBlk].bWaitEraseSts)
        {
            continue;
        }

        if (bTLC != pVBT[ucSuperPu]->m_VBT[uVirBlk].bTLC)
        {
            continue;
        }

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bError == TRUE)
            continue;

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bLock == TRUE)
            continue;

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bTable)
            continue;
        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bBackup)
            continue;
        if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bReserved)
            continue;

        if (pPBIT_Entry[uBlkPhy].bFree)
            continue;

        CurTime_WL = L2_Get_PBIT_BlkSN_Blk(ucSuperPu, ucLunInSuperPu, uBlkPhy);
#ifdef NEW_SWL
        if (CurTime_WL < MinSerialNum[0])
        {
            EraseCntSrc = pPBIT_Entry[uBlkPhy].EraseCnt;
            if (EraseCntSrc > ulECave) // if the condition is >=, then it is possible that we can't find src blk
                continue;

            MinSerialNum[0] = CurTime_WL;
            uBlkVBN[0] = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        }
        else if (CurTime_WL < MinSerialNum[1])
        {
            EraseCntSrc = pPBIT_Entry[uBlkPhy].EraseCnt;
            if (EraseCntSrc > ulECave)
                continue;

            MinSerialNum[1] = CurTime_WL;
            uBlkVBN[1] = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        }
    }

    L2_Set_PBIT_MinBlkSN_CE(ucSuperPu, MinSerialNum[0]);

    nMayBeSelCnt_S = 0;

    if (uBlkVBN[0] == INVALID_4F || uBlkVBN[1] == INVALID_4F)
    {
        return INVALID_4F;
    }
    gwl_info->nSrcBlkBuf[ucSuperPu] = uBlkVBN[1];
    L2_PBIT_Set_Lock(ucSuperPu, uBlkVBN[1], TRUE);

    return uBlkVBN[0];

#else
        if (CurTime_WL < MinSerialNum)
        {
            EraseCntSrc = pPBIT_Entry[uBlkPhy].EraseCnt;
            if (EraseCntSrc >= EraseCntDst)
                continue;
            MinSerialNum = CurTime_WL;
            uBlkVBN = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        }
    }

    L2_Set_PBIT_MinBlkSN_CE(ucSuperPu, MinSerialNum);

    nMayBeSelCnt_S = 0;
    if (uBlkVBN == INVALID_4F)
    {
        return INVALID_4F;
    }
    return uBlkVBN;
#endif

}


//Select SrcBlk according to erase cnt
U16 MCU1_DRAM_TEXT L2_Get_WLSrcBlk_E(U8 ucSuperPu, U8 ucLunInSuperPu, BOOL bTLC)
{
#ifdef NEW_SWL
    U16 uBlkVBN[2] = { INVALID_4F, INVALID_4F };
    U32 MinEraseCnt[2] = { INVALID_8F, INVALID_8F };
#else
    U16 uBlkVBN = INVALID_4F;
    U32 MinEraseCnt = INVALID_8F;
#endif 
    U16 uVirBlk = 0;
    U16 uBlkPhy = 0;
    PBIT_ENTRY* pPBIT_Entry;

    pPBIT_Entry = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];
    for (uBlkPhy = g_ulDataBlockStart[ucSuperPu][ucLunInSuperPu]; uBlkPhy < BLK_PER_LUN + RSVD_BLK_PER_LUN; uBlkPhy++)
    {
        uVirBlk = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        if (uVirBlk == INVALID_4F)
            continue;

        if (VBT_NOT_TARGET != pVBT[ucSuperPu]->m_VBT[uVirBlk].Target)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[uVirBlk].bsInEraseQueue)
        {
            continue;
        }

        if (TRUE == pVBT[ucSuperPu]->m_VBT[uVirBlk].bWaitEraseSts)
        {
            continue;
        }

        if (bTLC != pVBT[ucSuperPu]->m_VBT[uVirBlk].bTLC)
        {
            continue;
        }

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bError == TRUE)
            continue;

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bLock == TRUE)
            continue;

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bTable)
            continue;
        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bBackup)
            continue;
        if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bReserved)
            continue;

        if (pPBIT_Entry[uBlkPhy].bFree == FALSE)
        {
#ifdef NEW_SWL
            if (pPBIT_Entry[uBlkPhy].EraseCnt < MinEraseCnt[0])
            {
                MinEraseCnt[0] = pPBIT_Entry[uBlkPhy].EraseCnt;
                uBlkVBN[0] = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
            }
            else if (pPBIT_Entry[uBlkPhy].EraseCnt < MinEraseCnt[1])
            {
                MinEraseCnt[1] = pPBIT_Entry[uBlkPhy].EraseCnt;
                uBlkVBN[1] = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
            }
#else
            if (pPBIT_Entry[uBlkPhy].EraseCnt < MinEraseCnt)
            {
                MinEraseCnt = pPBIT_Entry[uBlkPhy].EraseCnt;
                uBlkVBN = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
            }
#endif
        }
    }

    nMayBeSelCnt_E = 0;

#ifdef NEW_SWL
    if (uBlkVBN[0] == INVALID_4F || uBlkVBN[1] == INVALID_4F)
    {
        return INVALID_4F;
    }
    gwl_info->nSrcBlkBuf[ucSuperPu] = uBlkVBN[1];
    L2_PBIT_Set_Lock(ucSuperPu, uBlkVBN[1], TRUE);

    return uBlkVBN[0];
#else
    if (uBlkVBN == INVALID_4F)
    {
        return INVALID_4F;
    }
    return uBlkVBN;
#endif
}

void MCU1_DRAM_TEXT L2_BM_CollectFreeBlock_GC_ErrH(U8 ucSuperPu, U16 VBN, U32 ulFailLun)
{
    U16 PhyBlockAddr;
    U32 i;
    BOOL bTLC;
    BlkType eBlk;

#ifdef DirtyLPNCnt_IN_DSRAM1
    *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN) = 0;
#else
    pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt = 0;
#endif
    pVBT[ucSuperPu]->m_VBT[VBN].StripID = INVALID_4F;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_INVALID;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = TRUE;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_INVALID;

    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, VBN))
    {
        bTLC = TRUE;
        eBlk = BLKTYPE_TLC;
    }
    else
    {
        bTLC = FALSE;
        eBlk = BLKTYPE_SLC;
    }

    pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[eBlk]--;
    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        PhyBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[i];

        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bFree = TRUE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bAllocated = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bTable = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bWeak = FALSE;
        L2_Reset_PBIT_Info(ucSuperPu, i, PhyBlockAddr);

        /* If this LUN fail, the failed physical blk has been replaced by reserved blk*/
        /* do not need increase erase cnt */
        if (FALSE == COM_BitMaskGet(ulFailLun, i))
        {
            pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt++;
#ifdef L2MEASURE
            pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].ECPerPOR++;
#endif

            pPBIT[ucSuperPu]->m_EraseCnt[eBlk]++;

            if (TRUE == bTLC)
            {
                /* check if this block need to do WL */
                L2_IsNeedWL(pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt, ucSuperPu, i, VBN);
            }
        }
    }
}

void MCU1_DRAM_TEXT L2_BM_CollectFreeBlock_WL_ErrH(U8 ucSuperPu, U16 VBN, U32 ulFailLun)
{
    U16 PhyBlockAddr;
    U32 i;
    BlkType eBlk;

    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, VBN))
    {
        eBlk = BLKTYPE_TLC;
    }
    else
    {
        eBlk = BLKTYPE_SLC;
    }

    L2_VBT_Set_Free(ucSuperPu, VBN, TRUE);
    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        PhyBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[i];

        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bFree = TRUE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bAllocated = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bTable = FALSE;
        L2_Reset_PBIT_Info((U8)ucSuperPu, i, PhyBlockAddr);

        /* If this LUN fail, the failed physical blk has been replaced by reserved blk*/
        /* do not need increase erase cnt */
        if (FALSE == COM_BitMaskGet(ulFailLun, i))
        {
            pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt++;
#ifdef L2MEASURE
            pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].ECPerPOR++;
#endif

            pPBIT[ucSuperPu]->m_EraseCnt[eBlk]++;
        }
    }
}

void L2_BM_CollectFreeBlock(U8 ucSuperPu, U16 VBN)
{
    U16 PhyBlockAddr;
    U32 i;
    BOOL bTLC;
    BlkType eBlk;

#ifdef DirtyLPNCnt_IN_DSRAM1
    *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN) = 0;
#else
    pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt = 0;
#endif
    pVBT[ucSuperPu]->m_VBT[VBN].StripID = INVALID_4F;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_INVALID;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = TRUE;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_INVALID;

    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, VBN))
    {
        bTLC = TRUE;
        eBlk = BLKTYPE_TLC;
    }
    else
    {
        bTLC = FALSE;
        eBlk = BLKTYPE_SLC;
    }

    pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[eBlk]--;

    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        PhyBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[i];

        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bFree = TRUE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bAllocated = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bTable = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bWeak = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt++;        
#ifdef L2MEASURE
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].ECPerPOR++;
#endif 

        pPBIT[ucSuperPu]->m_EraseCnt[eBlk]++;

        L2_Reset_PBIT_Info(ucSuperPu, i, PhyBlockAddr);

        if (TRUE == bTLC)
        {
            //FIRMWARE_LogInfo("SuperPU %d EraseBlk 0x%x\n", ucSuperPu, VBN);

            /* check if this block need to do WL */
            L2_IsNeedWL(pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt, ucSuperPu, i, VBN);
        }
    }    
}

void MCU1_DRAM_TEXT L2_BM_CollectFreeBlock_WL(U8 ucSuperPu, U16 VBN)
{
    U16 PhyBlockAddr;
    U32 i;
    BlkType eBlk;

    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, VBN))
    {
        eBlk = BLKTYPE_TLC;
    }
    else
    {
        eBlk = BLKTYPE_SLC;
    }
    
    L2_VBT_Set_Free(ucSuperPu, VBN, TRUE);
    
    pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[eBlk]--;
    
    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        PhyBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[i];

        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bFree = TRUE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bAllocated = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bTable = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt++;       
        pPBIT[ucSuperPu]->m_EraseCnt[eBlk]++;

#ifdef L2MEASURE
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].ECPerPOR++;
#endif 

        L2_Reset_PBIT_Info((U8)ucSuperPu, i, PhyBlockAddr);
    }
}

void MCU1_DRAM_TEXT L2_BM_CollectFreeBlock_Rebuild(U8 ucSuperPu, U16 VBN)
{
    U16 PhyBlockAddr;
    U32 i;
    BlkType eBlk;

    L2_VBT_Set_Free(ucSuperPu, VBN, TRUE);
    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        PhyBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[i];

        if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bTLC)
        {
            eBlk = BLKTYPE_TLC;
        }
        else
        {
            eBlk = BLKTYPE_SLC;
        }

        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bFree = TRUE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bAllocated = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].bTable = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].EraseCnt++;
        pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[eBlk]--;

#ifdef L2MEASURE
        pPBIT[ucSuperPu]->m_PBIT_Entry[i][PhyBlockAddr].ECPerPOR++;
#endif

        pPBIT[ucSuperPu]->m_EraseCnt[eBlk]++;

        L2_Reset_PBIT_Info((U8)ucSuperPu, i, PhyBlockAddr);
    }
}



/********************** FILE END ***************/

