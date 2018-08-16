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
Filename    :L2_PMTManager.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.04.05
Description :functions about Error Handling
Others      :
Modify      :
****************************************************************************/
#include "HAL_Inc.h"
#include "L2_RPMT.h"
#include "L2_GCManager.h"
#include "L2_Defines.h"
#include "L2_ErrorHandling.h"
#include "L2_Thread.h"
#include "L2_FCMDQ.h"
#include "L2_TLCMerge.h"
#include "L2_Interface.h"
#include "L2_TableBBT.h"
#include "FW_Event.h"
#ifdef SIM
GLOBAL CRITICAL_SECTION  g_CriticalAllocFreeBlk;
#endif

GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_GCErrHandle;
GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_SWLErrHandle;

extern GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_TLCMergeErrHandle;
extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;
//extern GLOBAL U32 g_ulTLCBlockEnd[SUBSYSTEM_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];

extern GLOBAL U32 g_L2TempBufferAddr;
extern void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode);
extern BOOL L2_IsSLCBlock(U8 ucSuperPu, U16 VBN);
extern void L2_BbtAddGBbtBadBlk(U8 ucTLun, U16 usPhyBlk);

/****************************************************************************
Name        :L2_ErrorHandlingInit(U8 ucSuperPU)
Input       :
Output      :
Author      :HenryLuo
Date        :2013.02.27    18:38:20
Description :
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L2_ErrorHandlingInit(U8 ucSuperPu)
{
    U32 i;
#ifdef SIM
    InitializeCriticalSection(&g_CriticalAllocFreeBlk);
#endif

    g_GCErrHandle[ucSuperPu].m_ErrHandleStage = LOAD_SPARE_SPACE;
    g_GCErrHandle[ucSuperPu].m_ErrPU = INVALID_8F;
    g_GCErrHandle[ucSuperPu].m_ucLun = INVALID_2F;
    g_GCErrHandle[ucSuperPu].m_ErrBlock = INVALID_8F;
    g_GCErrHandle[ucSuperPu].m_ErrHandlePage = 0;

    for (i = 0; i < PG_PER_SLC_BLK; i++)
    {
        g_GCErrHandle[ucSuperPu].m_FlashStatus[i] = SUBSYSTEM_STATUS_SUCCESS;
    }

    g_SWLErrHandle[ucSuperPu].m_ErrHandleStage = LOAD_SPARE_SPACE;
    g_SWLErrHandle[ucSuperPu].m_ErrPU = INVALID_8F;
    g_SWLErrHandle[ucSuperPu].m_ucLun = INVALID_2F;
    g_SWLErrHandle[ucSuperPu].m_ErrBlock = INVALID_8F;
    g_SWLErrHandle[ucSuperPu].m_ErrHandlePage = 0;

    for (i = 0; i < PG_PER_SLC_BLK; i++)
    {
        g_SWLErrHandle[ucSuperPu].m_FlashStatus[i] = SUBSYSTEM_STATUS_SUCCESS;
    }


    g_TLCMergeErrHandle[ucSuperPu].m_ErrHandleStage = LOAD_SPARE_SPACE;
    g_TLCMergeErrHandle[ucSuperPu].m_ErrPU = INVALID_8F;
    g_TLCMergeErrHandle[ucSuperPu].m_ucLun = INVALID_2F;
    g_TLCMergeErrHandle[ucSuperPu].m_ErrBlock = INVALID_8F;
    g_TLCMergeErrHandle[ucSuperPu].m_ErrHandlePage = 0;

    for (i = 0; i < PG_PER_SLC_BLK; i++)
    {
        g_TLCMergeErrHandle[ucSuperPu].m_FlashStatus[i] = SUBSYSTEM_STATUS_SUCCESS;
    }
    return;
}

U16 MCU12_DRAM_TEXT L2_ErrHApplyNewPBN(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, BOOL bTLCBlk)
{
    U8 ucTLun, ucLUNInSuperPU;
    U16 usPhyBlk, usNewVirBlk = INVALID_4F, usNewPhyBlk, usBrokenPBN;

    ucTLun = L2_GET_TLUN(ucSPU, ucLunInSPU);
    usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, usCurVirBlk);

    DBG_Printf("%s SPU_LunInSPU %d_%d VBN_PBN %d_%d bTLCBlk_VBTGetTLC %d_%d\n",__FUNCTION__,
        ucSPU, ucLunInSPU, usCurVirBlk, usPhyBlk, bTLCBlk, L2_VBT_Get_TLC(ucSPU, usCurVirBlk));

    ASSERT(bTLCBlk == L2_VBT_Get_TLC(ucSPU, usCurVirBlk));

#ifdef SIM
    GET_ALLOC_FREE_BLK_LOCK;
#else
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif    

    if (TRUE != L2_BM_BackUpBlockEmpty(ucSPU, ucLunInSPU))
    {
        U32 SLCBlkend, TLCBlkEnd;

        usNewPhyBlk = L2_BM_AllocateBackUpBlock(ucSPU, ucLunInSPU, BLOCK_TYPE_MIN_ERASE_CNT, bTLCBlk);
        ASSERT(INVALID_4F != usNewPhyBlk);
        L2_PBIT_Set_Backup(ucSPU, ucLunInSPU, usNewPhyBlk, FALSE);

        /* modify EC for LUN0's BackupBlock use for replace */
        if ((TRUE == bTLCBlk) && (0 == ucLunInSPU))
        {
            pPBIT[ucSPU]->m_PBIT_Entry[ucLunInSPU][usNewPhyBlk].EraseCnt = pPBIT[ucSPU]->ulECMin;
            pPBIT[ucSPU]->ulECMinCnt++;
        }

        L2_BbtGetSlcTlcBlock(ucTLun, &SLCBlkend, &TLCBlkEnd);
        if (bTLCBlk)
        {
            TLCBlkEnd = usNewPhyBlk;
        }
        else
        {
            SLCBlkend = usNewPhyBlk;
        }
        L2_BbtSetSlcTlcBlock(ucTLun, SLCBlkend, TLCBlkEnd);

#ifdef SIM
        RELEASE_ALLOC_FREE_BLK_LOCK;
#else
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif        
        DBG_Printf("%s AllocateBackup SPU %d ErrLun %d NewPBN %d SLCBlockEnd %d TLCBlockEnd %d bTLCBlk %d\n", __FUNCTION__, ucSPU, ucLunInSPU, usNewPhyBlk, SLCBlkend, TLCBlkEnd, bTLCBlk);
    }
    /* select block from broken block */
    else if (TRUE != L2_BM_BrokenBlockEmpty(ucSPU, ucLunInSPU, bTLCBlk))
    {
        usNewPhyBlk = L2_BM_AllocateBrokenBlock(ucSPU, ucLunInSPU, BLOCK_TYPE_MIN_ERASE_CNT, bTLCBlk);
        ASSERT(INVALID_4F != usNewPhyBlk);

#ifdef SIM
        RELEASE_ALLOC_FREE_BLK_LOCK;
#else
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif        

        DBG_Printf("%s AllocateBrokenBlk Pu %d ErrLun %d NewPBN %d bTLCBlk %d done\n", __FUNCTION__, ucSPU, ucLunInSPU, usNewPhyBlk, bTLCBlk);
    }
    else
    {        
        usNewVirBlk = L2_BM_AllocateFreeBlock(ucSPU, BLOCK_TYPE_MIN_ERASE_CNT, bTLCBlk);
        ASSERT(INVALID_4F != usNewVirBlk);

#ifdef SIM
        if (usNewVirBlk == usCurVirBlk)
        {
            DBG_Printf("ErrBlock and NewAllocate replace block are the same, L2 must have some problem usNewVirBlk_usCurVirBlk %d_%d\n", usNewVirBlk, usCurVirBlk);
            DBG_Getch();
        }
#endif

        for (ucLUNInSuperPU = 0; ucLUNInSuperPU < LUN_NUM_PER_SUPERPU; ucLUNInSuperPU++)
        {
            if (ucLunInSPU == ucLUNInSuperPU)
            {
                usNewPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, usNewVirBlk);
                L2_VBT_SetPhysicalBlockAddr(ucSPU, ucLunInSPU, usNewVirBlk, INVALID_4F);
                ASSERT(INVALID_4F != usNewPhyBlk);
            }
            else
            {
                /* set other phy block as broken block */
                usBrokenPBN = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLUNInSuperPU, usNewVirBlk);
                L2_PBIT_Set_Broken(ucSPU, ucLUNInSuperPU, usBrokenPBN, TRUE);
                L2_PBIT_Set_Allocate(ucSPU, ucLUNInSuperPU, usBrokenPBN, FALSE);
                L2_PBIT_Set_Free(ucSPU, ucLUNInSuperPU, usBrokenPBN, TRUE);
                L2_PBIT_SetVirturlBlockAddr(ucSPU, ucLUNInSuperPU, usBrokenPBN, INVALID_4F);
                L2_VBT_SetPhysicalBlockAddr(ucSPU, ucLUNInSuperPU, usNewVirBlk, INVALID_4F);
                DBG_Printf("%s MarkAsBroken Pu %d LunInSuperPu %d BrokenPBN %d done\n", __FUNCTION__, ucSPU, ucLUNInSuperPU, usBrokenPBN);
            }
        }
        L2_VBT_Set_Free(ucSPU, usNewVirBlk, FALSE);

#ifdef SIM
        RELEASE_ALLOC_FREE_BLK_LOCK;
#else
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ALLOCATE_FREE_BLK);
#endif        
        
        if (TRUE == bTLCBlk)
        {
            L2_PBIT_Decrease_TotalBlockCnt(ucSPU, BLKTYPE_TLC);
            g_PuInfo[ucSPU]->m_DataBlockCnt[BLKTYPE_TLC]--;
            /* allocate Free BLK in ErrH not real allocate; because it will decrease total BLK number as above */
            pPBIT[ucSPU]->m_AllocatedDataBlockCnt[BLKTYPE_TLC]--;
        }
        else
        {
            L2_PBIT_Decrease_TotalBlockCnt(ucSPU, BLKTYPE_SLC);
            g_PuInfo[ucSPU]->m_DataBlockCnt[BLKTYPE_SLC]--;
            g_PuInfo[ucSPU]->m_SLCMLCFreePageCnt -= PG_PER_SLC_BLK;

            /* allocate Free BLK in ErrH not real allocate; because it will decrease total BLK number as above */
            pPBIT[ucSPU]->m_AllocatedDataBlockCnt[BLKTYPE_SLC]--;
        }

        DBG_Printf("%s AllocateFreeBlk Pu %d ErrLun %d  NewVBN %d NewPBN %d bTLCBlk %d done\n", __FUNCTION__, ucSPU, ucLunInSPU, usNewVirBlk, usNewPhyBlk, bTLCBlk);
    }

    return usNewPhyBlk;
}

void MCU12_DRAM_TEXT L2_ErrHRecostructMapping(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, U16 usNewPhyBlk, U8 ucErrType)
{
    U8 ucTLun;
    U16 usPhyBlk;
    BOOL bLockedBlk;
    BOOL bTLCBlk;   

    usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, usCurVirBlk);

    /* binding new mapping */
    L2_PBIT_SetVirturlBlockAddr(ucSPU, ucLunInSPU, usNewPhyBlk, usCurVirBlk);
    L2_VBT_SetPhysicalBlockAddr(ucSPU, ucLunInSPU, usCurVirBlk, usNewPhyBlk);

    /* update new blk PBIT entry */
    bLockedBlk = L2_PBIT_Get_Lock(ucSPU, usCurVirBlk);
    bTLCBlk = L2_VBT_Get_TLC(ucSPU, usCurVirBlk);
    L2_PBIT_Set_Lock(ucSPU, usCurVirBlk, bLockedBlk);
    L2_PBIT_Set_TLC(ucSPU, ucLunInSPU, usNewPhyBlk, bTLCBlk);

    /* update new blk PBIT info*/
    L2_Exchange_PBIT_Info(ucSPU, ucLunInSPU, usPhyBlk, usNewPhyBlk);
    L2_Reset_PBIT_Info(ucSPU, ucLunInSPU, usPhyBlk);

    /* update bad blk PBIT & insert to BBT */
    L2_PBIT_Set_Error(ucSPU, ucLunInSPU, usPhyBlk, TRUE);
    L2_PBIT_Set_Free(ucSPU, ucLunInSPU, usPhyBlk, FALSE);
    L2_PBIT_SetVirturlBlockAddr(ucSPU, ucLunInSPU, usPhyBlk, INVALID_4F);

    ucTLun = L2_GET_TLUN(ucSPU, ucLunInSPU);
    if (FALSE == L2_BbtIsGBbtBadBlock(ucTLun, usPhyBlk))
    {
        L2_BbtAddBbtBadBlk(ucTLun, usPhyBlk, RT_BAD_BLK, ucErrType);
        L2_BbtSetLunSaveBBTBitMap(ucTLun, TRUE);
        CommSetEvent(COMM_EVENT_OWNER_L2, COMM_EVENT_OFFSET_SAVE_BBT);
    }
    else
    {
        DBG_Printf("L3_ErrHErase Can't add the same block to BBT again! SuperPU %d LUN %d BadBLK %d\n", ucSPU, ucLunInSPU, usPhyBlk);
        DBG_Getch();
    }

    return;
}

U16 MCU12_DRAM_TEXT L2_ErrHReplaceBLK(U8 ucSPU, U8 ucLunInSPU, U16 usCurVirBlk, BOOL bTLCBlk, U8 ucErrType)
{
    U16 usNewPhyBlk;

    usNewPhyBlk = L2_ErrHApplyNewPBN(ucSPU, ucLunInSPU, usCurVirBlk, bTLCBlk);
    L2_ErrHRecostructMapping(ucSPU, ucLunInSPU, usCurVirBlk, usNewPhyBlk, ucErrType);

    return usNewPhyBlk;
}

BOOL MCU1_DRAM_TEXT L2_WLErrorHandlingEntry(U8 ucSuperPu)
{
    GC_ERROR_HANDLE * ptRpmtRebuidInfo;
    PhysicalAddr Addr = { 0 };
    RED* RedBuffer;
    RPMT* pRPMT;
    U8 * pStatus;
    U32 i;
    U8 ucLunInSuperPu = 0;
    U8 ucPrefix = 0;
    U32 ulSPuNum, ulBlock;
    U32 ulSrcPPO;   /* If prefix == 0, Index: 0, 3, 6......  */
    U32 ulDesPPO;  /* Index = 0, 1, 2,.....  */
    U32 ulPagePerBlk;
    U32 LPN;
    U32 LPNInBuff;
    PhysicalAddr RefAddr = {0};

    ptRpmtRebuidInfo = g_SWLErrHandle;

    if ((U8)ptRpmtRebuidInfo[ucSuperPu].m_ErrPU != ucSuperPu)
    {
        DBG_Printf("L2 WLErrH inputSPU:%d != WL records SPU:%d\n", ucSuperPu, ptRpmtRebuidInfo[ucSuperPu].m_ErrPU);
        DBG_Getch();
    }

    //set PU info
    ulSPuNum = ptRpmtRebuidInfo[ucSuperPu].m_ErrPU;
    ucLunInSuperPu = ptRpmtRebuidInfo[ucSuperPu].m_ucLun;
    ulBlock = ptRpmtRebuidInfo[ucSuperPu].m_ErrBlock;
    ucPrefix = ptRpmtRebuidInfo[ucSuperPu].m_ucErrRPMTnum; //read UECC RPMT number
    ulPagePerBlk = PG_PER_SLC_BLK; // total page per belong to 1 RPMT page

    if (FALSE == L2_VBT_Get_TLC(ulSPuNum, ulBlock))
    {
        DBG_Printf("L2_WLErrH Block %d not a TLC block\n", ulBlock);
        DBG_Getch();
    }

    switch (ptRpmtRebuidInfo[ucSuperPu].m_ErrHandleStage)
    {
    case LOAD_SPARE_SPACE:
        if (SUBSYSTEM_STATUS_FAIL != g_TLCManager->aucRPMTStatus[ucSuperPu][ucLunInSuperPu][ucPrefix])
        {
            DBG_Printf("SPU %d is not load RPMT error in TLC SWL!\n", ulSPuNum);
            DBG_Getch();
        }

        /* check if have free PU fifo to do GC copy */
        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulSPuNum, ucLunInSuperPu)))
        {
            return FALSE;
        }

        /* load all spare space in TLC WL source block*/
        ulSrcPPO = ucPrefix * (PG_PER_SLC_BLK - 1) + ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage;
        ulDesPPO = ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage;
        Addr.m_PUSer = ulSPuNum;
        Addr.m_OffsetInSuperPage = ucLunInSuperPu;
        Addr.m_BlockInPU = ulBlock;
        Addr.m_PageInBlock = ulSrcPPO;
        Addr.m_LPNInPage = 0;

        /* the total corresponding pages in 1 RPMT is 511, 1 page RED is  128bytes(64byte * 4).
                 total is 63.875KB for Micron 3DTLC , g_L2TempBufferAddr total allocate 128Kbytes*/
        RedBuffer = (RED*)g_L2TempBufferAddr + ulDesPPO + (PG_PER_SLC_BLK * ucSuperPu);

        pStatus = &ptRpmtRebuidInfo[ucSuperPu].m_FlashStatus[ulDesPPO];
        L2_FtlReadLocal((U32*)g_L2TempBufferAddr, &Addr, pStatus, (U32*)RedBuffer, 0, 0, FALSE, FALSE);

        ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage++;
        if (ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage >= ulPagePerBlk - 1)
        {
            ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage = 0;
            ptRpmtRebuidInfo[ucSuperPu].m_ErrHandleStage = REBUILD_RPMT;
            return FALSE;
        }

        break;

    case REBUILD_RPMT:
        pRPMT = g_TLCManager->pRPMT[ulSPuNum][ucPrefix];
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU = ulSPuNum;
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock = ulBlock;
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[PG_PER_SLC_BLK - 1] = 0;

        for (ulDesPPO = ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage; ulDesPPO < (ulPagePerBlk - 1); ulDesPPO++)
        {
            pStatus = &ptRpmtRebuidInfo[ucSuperPu].m_FlashStatus[ulDesPPO];
            if (SUBSYSTEM_STATUS_PENDING == *pStatus)
            {
                return FALSE;
            }
            else if (SUBSYSTEM_STATUS_FAIL == *pStatus)
            {
                ulSrcPPO = ucPrefix * (PG_PER_SLC_BLK - 1) + ulDesPPO;
                DBG_Printf("L2 WLErrH: PU %d Block %d page %d spare read fail in L2 error handling! \n", ulSPuNum, ulBlock, ulSrcPPO);

                /*step 1: update PMT to invalid and set DPBM to dirty*/
                for (LPNInBuff = 0; LPNInBuff < LPN_PER_BUF; LPNInBuff++)
                {
                    Addr.m_PUSer = ulSPuNum;
                    Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                    Addr.m_BlockInPU = ulBlock;
                    Addr.m_PageInBlock = ulSrcPPO;
                    Addr.m_LPNInPage = LPNInBuff;

                    for (LPN = 0; LPN < MAX_LPN_IN_DISK; LPN++)
                    {
                        L2_LookupPMT(&RefAddr, LPN, FALSE);
                        if (RefAddr.m_PPN == Addr.m_PPN)
                        {
                            if (TRUE == L2_LookupDirtyLpnMap(&RefAddr))
                            {
                                pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[ulDesPPO * LPN_PER_BUF + LPNInBuff] = LPN;

                            #ifdef SIM
                                DBG_Printf("L2_WLErrorHandlingEntry LPN 0x%x SuperPU %d LUNOffset %d Blk 0x%x PPO 0x%x LPNInBuf %d\n", 
                                    LPN, Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, Addr.m_PageInBlock, RefAddr.m_LPNInPage);
                            #endif
                                break;
                            }
                        }
                    }
                }

                pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[ulDesPPO] = 0;
                pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[ulDesPPO] = 0;

            }
            else
            {
                RedBuffer = (RED*)g_L2TempBufferAddr + ulDesPPO + (PG_PER_SLC_BLK * ucSuperPu);

                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[ulDesPPO * LPN_PER_BUF + i] = RedBuffer->m_DataRed.aCurrLPN[i];
                }
                pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[ulDesPPO] = RedBuffer->m_RedComm.ulTimeStamp;
                pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[ulDesPPO] = RedBuffer->m_RedComm.ulTargetOffsetTS;
            }
        }

        /* Rebuild RPMT done, set the RPMT page status success */
        g_TLCManager->aucRPMTStatus[ucSuperPu][ucLunInSuperPu][ucPrefix] = SUBSYSTEM_STATUS_SUCCESS;

        /* reset error handle status */
        ptRpmtRebuidInfo[ucSuperPu].m_ErrHandleStage = LOAD_SPARE_SPACE;
        ptRpmtRebuidInfo[ucSuperPu].m_ErrPU = INVALID_8F;
        ptRpmtRebuidInfo[ucSuperPu].m_ucLun = INVALID_2F;
        ptRpmtRebuidInfo[ucSuperPu].m_ErrBlock = INVALID_8F;
        ptRpmtRebuidInfo[ucSuperPu].m_ErrHandlePage = 0;
        for (i = 0; i < PG_PER_SLC_BLK; i++)
        {
            ptRpmtRebuidInfo[ucSuperPu].m_FlashStatus[i] = SUBSYSTEM_STATUS_SUCCESS;
        }

        return TRUE;

    }

    return FALSE;
}

GC_ERROR_HANDLE * MCU1_DRAM_TEXT L2_GetRpmtInfo(BOOL bGcErrHandle)
{
    //handle TLC merge error or gc error handle
    if(TRUE == bGcErrHandle)
    {
        return g_GCErrHandle;
    }
    else
    {
        return g_TLCMergeErrHandle;
    }
}

BOOL MCU1_DRAM_TEXT L2_GetTlcMode(BOOL bGcErrHandle, GCComStruct * ptGCPointer, U32 ulPuNum)
{
    BOOL bRet = FALSE;
    
    if(TRUE == bGcErrHandle)
    {
        if(VBT_TYPE_TLCW == ptGCPointer->m_ucSrcBlkType[ulPuNum])
        {
            bRet = TRUE;
        }
    }
        
    return bRet;
}

void MCU1_DRAM_TEXT L2_SetRpmtRebuildAddr(PhysicalAddr* Addr, U32 ulPuNum, U32 ulBlock, U32 ulPPO, U8 ucLunInSuperPu, BOOL bGcErrHandle, GCComStruct * ptGCPointer, U8 ucPrefix)
{
    /* load all spare space in GC source block*/
    Addr->m_PUSer = ulPuNum;
    Addr->m_BlockInPU = ulBlock;
    Addr->m_PageInBlock = ulPPO;
    Addr->m_OffsetInSuperPage = ucLunInSuperPu;
    Addr->m_LPNInPage = 0;

    if(TRUE == bGcErrHandle)
    {
        if(VBT_TYPE_TLCW == ptGCPointer->m_ucSrcBlkType[ulPuNum])
        {
            Addr->m_PageInBlock = (ucPrefix * (PG_PER_SLC_BLK - 1)) + ulPPO;
        }
    }

    return;
}
/****************************************************************************
Name        :L2_ErrorHandlingEntry
Input       :
Output      :
Author      :HenryLuo
Date        :2013.02.27    18:24:20
Description :
Others      :
Modify      :
****************************************************************************/
BOOL MCU1_DRAM_TEXT L2_ErrorHandlingEntry(U8 ucPuNum, BOOL bGcErrHandle, GCComStruct * ptGCPointer)
{
    PhysicalAddr Addr = { 0 };
    PhysicalAddr RefAddr = { 0 };
    RED* RedBuffer;
    U8 * pStatus;
    U32 ulPuNum, ulBlock, ulPPO;
    U16 i;
    RPMT* pRPMT;
    U32 ulPagePerBlk;
    BOOL bTLCMode = FALSE;
    U8 ucLunInSuperPu = 0;
    U8 ucPrefix = 0;
    GC_ERROR_HANDLE * ptRpmtRebuidInfo;
    U32 ulSrcPPO;
    U32 LPN;

    //get rebuild rpmt info
    ptRpmtRebuidInfo = L2_GetRpmtInfo(bGcErrHandle);

    if ((U8)ptRpmtRebuidInfo[ucPuNum].m_ErrPU != ucPuNum)
    {
        DBG_Printf("L2 ErrH inputPU:%d != GC records PU:%d\n", ucPuNum, ptRpmtRebuidInfo[ucPuNum].m_ErrPU);
        DBG_Getch();
    }
    //set PU info
    ulPuNum = ptRpmtRebuidInfo[ucPuNum].m_ErrPU;
    ucLunInSuperPu = ptRpmtRebuidInfo[ucPuNum].m_ucLun;
    ulBlock = ptRpmtRebuidInfo[ucPuNum].m_ErrBlock;
    ucPrefix = ptRpmtRebuidInfo[ucPuNum].m_ucErrRPMTnum;

    ulPagePerBlk = PG_PER_SLC_BLK;
    bTLCMode = L2_GetTlcMode(bGcErrHandle, ptGCPointer, ulPuNum);

    switch (ptRpmtRebuidInfo[ucPuNum].m_ErrHandleStage)
    {
    case LOAD_SPARE_SPACE:
        //gc has two kind of source(SLC or TLC)
        if (TRUE == bGcErrHandle)
        {
            if (SUBSYSTEM_STATUS_FAIL != ptGCPointer->m_RPMTStatus[ulPuNum][ucLunInSuperPu][ucPrefix])
            {
                DBG_Printf("PU %d is not load RPMT error in GC! \n", ulPuNum);
                DBG_Getch();
            }
        }
        else
        {
            if (SUBSYSTEM_STATUS_FAIL != g_TLCManager->aucRPMTStatus[ulPuNum][ucLunInSuperPu][ucPrefix])
            {
                DBG_Printf("PU %d is not load RPMT error in TLC Merge! \n", ulPuNum);
                DBG_Getch();
            }
        }

        /* check if have free PU fifo to do GC copy */
        if (FALSE == L2_FCMDQNotFull(L2_GET_TLUN(ulPuNum, ucLunInSuperPu)))
        {
            return FALSE;
        }

        /* load all spare space in GC source block*/
        ulPPO = ptRpmtRebuidInfo[ucPuNum].m_ErrHandlePage;
        L2_SetRpmtRebuildAddr(&Addr, ulPuNum, ulBlock, ulPPO, ucLunInSuperPu, bGcErrHandle, ptGCPointer, ucPrefix);
        
        RedBuffer = (RED*)g_L2TempBufferAddr + ulPPO + (PG_PER_SLC_BLK * ucPuNum);

        pStatus = &ptRpmtRebuidInfo[ucPuNum].m_FlashStatus[ulPPO];
        L2_FtlReadLocal((U32*)g_L2TempBufferAddr, &Addr, pStatus, (U32*)RedBuffer, 0, 0, FALSE, !bTLCMode);

        ptRpmtRebuidInfo[ucPuNum].m_ErrHandlePage++;
        if (ptRpmtRebuidInfo[ucPuNum].m_ErrHandlePage >= ulPagePerBlk - 1)
        {
            ptRpmtRebuidInfo[ucPuNum].m_ErrHandlePage = 0;
            ptRpmtRebuidInfo[ucPuNum].m_ErrHandleStage = REBUILD_RPMT;
            return FALSE;
        }

        break;

    case REBUILD_RPMT:
        if (TRUE == bGcErrHandle)
        {
            pRPMT = ptGCPointer->m_pRPMT[ulPuNum][ucPrefix];
            pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU = ulPuNum;
            pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock = ulBlock;
        }
        else
        {
            pRPMT = g_TLCManager->pRPMT[ulPuNum][ucPrefix];
            pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU = ulPuNum;
            pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock = ulBlock;
        }

        for (ulPPO = ptRpmtRebuidInfo[ucPuNum].m_ErrHandlePage; ulPPO < (ulPagePerBlk - 1); ulPPO++)
        {
            pStatus = &ptRpmtRebuidInfo[ucPuNum].m_FlashStatus[ulPPO];
            if (*pStatus == SUBSYSTEM_STATUS_PENDING)
            {
                return FALSE;
            }
            else if (*pStatus == SUBSYSTEM_STATUS_FAIL)
            {
                DBG_Printf("PU %d Block %d page %d spare read fail in L2 error handling! \n", ulPuNum, ulBlock, ulPPO);

                ulSrcPPO = ucPrefix * (PG_PER_BLK - 1) + ulPPO;

                /*step 1: update PMT and DPBM*/
                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    Addr.m_PUSer = ulPuNum;
                    Addr.m_OffsetInSuperPage = ucLunInSuperPu;
                    Addr.m_BlockInPU = ulBlock;
                    Addr.m_PageInBlock = ulSrcPPO;
                    Addr.m_LPNInPage = i;

                    for (LPN = 0; LPN < MAX_LPN_IN_DISK; LPN++)
                    {
                        L2_LookupPMT(&RefAddr, LPN, FALSE);
                        if (RefAddr.m_PPN == Addr.m_PPN)
                        {
                            if (TRUE == L2_LookupDirtyLpnMap(&RefAddr))
                            {
                                /*step 2: update RPMT*/
                                pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[ulPPO * LPN_PER_BUF + i] = LPN;
							
                            #ifdef SIM
                                DBG_Printf("L2_ErrH LPN 0x%x (PPN 0x%x -> INVALID_8F) Pu %d Blk %d PPO %d Offset %d DirtyCnt %d\n", LPN, RefAddr.m_PPN, ulPuNum, ulBlock, ulPPO, i, L2_GetDirtyCnt(ulPuNum, ulBlock));
                            #endif
                                break;
                            }
                        }
                    }
                }

                pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[ulPPO] = 0;
                pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[ulPPO] = 0;
            }
            else
            {
                RedBuffer = (RED*)g_L2TempBufferAddr + ulPPO + (PG_PER_SLC_BLK * ucPuNum);

                for (i = 0; i < LPN_PER_BUF; i++)
                {
                    pRPMT->m_RPMT[ucLunInSuperPu].m_RPMTItems[ulPPO * LPN_PER_BUF + i] = RedBuffer->m_DataRed.aCurrLPN[i];
                }
                pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[ulPPO] = RedBuffer->m_RedComm.ulTimeStamp;
                pRPMT->m_RPMT[ucLunInSuperPu].m_LunOrderTS[ulPPO] = RedBuffer->m_RedComm.ulTargetOffsetTS;
            }
        }

        /* set GC load RPMT status to success */
        if (TRUE == bGcErrHandle)
        {
            ptGCPointer->m_RPMTStatus[ulPuNum][ucLunInSuperPu][ucPrefix] = SUBSYSTEM_STATUS_SUCCESS;
        }
        else
        {
            g_TLCManager->aucRPMTStatus[ulPuNum][ucLunInSuperPu][ucPrefix] = SUBSYSTEM_STATUS_SUCCESS;
        }
        /* reset error handle status */
        //L2_ErrorHandlingInit((U8)ulPuNum);
        ptRpmtRebuidInfo[ucPuNum].m_ErrHandleStage = LOAD_SPARE_SPACE;
        ptRpmtRebuidInfo[ucPuNum].m_ErrPU = INVALID_8F;
        ptRpmtRebuidInfo[ucPuNum].m_ucLun = INVALID_2F;
        ptRpmtRebuidInfo[ucPuNum].m_ErrBlock = INVALID_8F;
        ptRpmtRebuidInfo[ucPuNum].m_ErrHandlePage = 0;
        for (i = 0; i < PG_PER_SLC_BLK; i++)
        {
            ptRpmtRebuidInfo[ucPuNum].m_FlashStatus[i] = SUBSYSTEM_STATUS_SUCCESS;
        }

        return TRUE;
    }

    return FALSE;
}
