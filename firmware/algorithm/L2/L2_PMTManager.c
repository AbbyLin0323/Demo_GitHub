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
Date        :2012.03.1
Description :functions about PMTManager
1) L2_InitPMTManager(): init the value to boot up status
2) New Table GC. (Author: addison su)
Others      :
Modify      :
*******************************************************************************/
#include "L2_PMTManager.h"
#include "L2_PMTPage.h"
#include "L2_Debug.h"
#include "HAL_MemoryMap.h"
#include "FW_Event.h"
#include "L2_Boot.h"
#include "L2_Thread.h"
#include "L2_Schedule.h"
#include "L2_TableBBT.h"
#include "L2_RT.h"
#include "L2_TableBlock.h"
#include "L2_Interface.h"
#include "L2_FCMDQ.h"
#include "COM_Memory.h"
#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif
#include "L2_TLCMerge.h"

#ifdef SIM
#include "HostModel.h"
#include "L2_SimTablecheck.h"
#endif


#define TL_FILE_NUM L2_PMTManager_c

extern void L2_FtlWriteLocal(PhysicalAddr* pAddr, U32* pPage, U32* pSpare, U8* pStatus, BOOL bTableReq, BOOL bSLCMode, XOR_PARAM *pXorParam);
extern void L2_FtlReadLocal(U32* pBuffer, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode);
extern void __L2_FtlReadLocal(U32* pBuffAddr, PhysicalAddr* pAddr, U8* pStatus, U32* pSpare, U32 ReadLPNCnt, U32 LPNOffset, BOOL bTableReq, BOOL bSLCMode, BOOL bPatrol);

GLOBAL  U32 g_FindTableGCPos[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  TableRWManager *g_TableRW;
GLOBAL  const U32 c_ulTableGCThs = 100;
GLOBAL  BOOL g_bPMTErrFlag;
GLOBAL  U16 bReWritePMTIndex[TABLE_RW_PAGE_CNT];
GLOBAL  U32 bReWritePMTCnt = 0;
extern GLOBAL U32 g_ulPMTFlushing;

#ifdef ValidLPNCountSave_IN_DSRAM1
GLOBAL  U16* g_pValidLPNCountSaveL;
GLOBAL  U8*  g_pValidLPNCountSaveH;

INLINE U32 U24getValue(U8 x, U32 y)
{
    U32 index = x*PMTPAGE_CNT_PER_SUPERPU_MAX + y;
    return (U32)(*(g_pValidLPNCountSaveL + index)) | (U32)(*(g_pValidLPNCountSaveH + index) << 16);
}

INLINE void U24setValue(U8 x, U32 y, U32 value)
{
    U32 index = x*PMTPAGE_CNT_PER_SUPERPU_MAX + y;
    *(g_pValidLPNCountSaveH + index) = (U8)(value >> 16);
    *(g_pValidLPNCountSaveL + index) = (U16)value;
    return;
}

INLINE void U24incOne(U8 x, U32 y)
{
    U32 index = x*PMTPAGE_CNT_PER_SUPERPU_MAX + y;

    (*(g_pValidLPNCountSaveL + index))++;
    if (*(g_pValidLPNCountSaveL + index) == 0)
        (*(g_pValidLPNCountSaveH + index))++;
}

INLINE void U24decOne(U8 x, U32 y)
{
    U32 index = x*PMTPAGE_CNT_PER_SUPERPU_MAX + y;

    (*(g_pValidLPNCountSaveL + index))--;
    if (*(g_pValidLPNCountSaveL + index) == 0xFFFF)
        (*(g_pValidLPNCountSaveH + index))--;
}
#endif

void MCU1_DRAM_TEXT L2_InitPMTBlkManager()
{
    U8 ucSuperPu;
    U16 BlkSN;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurBlkSN = 0;
        g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurPPO = 0;
        g_PMTManager->m_PMTBlkManager[ucSuperPu].m_FreePagesCnt = AT1_BLOCK_COUNT * PG_PER_SLC_BLK;
        g_PMTManager->m_PMTBitMap[ucSuperPu] = 0;
        g_PMTManager->m_CurFlushPMTIndex[ucSuperPu] = INVALID_8F;
        g_PMTManager->m_PMTSaveStage[ucSuperPu] = L2_PMT_SAVE_PREPARE;

        for (BlkSN = 0; BlkSN < AT1_BLOCK_COUNT; BlkSN++)
        {
            if (0 == BlkSN)
            {
                g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlkSN].m_bFree = FALSE;
            }
            else
            {
                g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlkSN].m_bFree = TRUE;
            }

            L2_ClearDirtyCntOfPMT(ucSuperPu, BlkSN);
        }

    }
}

U16 L2_GetCurBlockSNOfPMT(U8 ucSuperPu)
{
    return g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurBlkSN;
}
U16 L2_GetCurPPOOfPMT(U8 ucSuperPu)
{
    return g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurPPO;
}

U32 L2_GetFreePagesCntofPMT(U8 ucSuperPu)
{
    return g_PMTManager->m_PMTBlkManager[ucSuperPu].m_FreePagesCnt;
}

BOOL L2_IncCurPOOfPMT(U8 ucSuperPu)
{
    U16 usCurPPO;

    usCurPPO = g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurPPO;

    usCurPPO++;

    if (usCurPPO >= PG_PER_SLC_BLK)
    {
        U16 uFreeBlockSN;

        //(1)CurPPO clear
        usCurPPO = 0;

        //(2)find free PMT_BLK_SN        
        uFreeBlockSN = L2_FindFreeBlockofPMT(ucSuperPu);
        if (INVALID_4F == uFreeBlockSN)
        {
            DBG_Printf("PU %d has no Free Block of PMT\n", ucSuperPu);
            DBG_Getch();
            return FAIL;
        }

        //(3)update CurBlkSN
        g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurBlkSN = uFreeBlockSN;

        //(4)set blk state no free
        g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[uFreeBlockSN].m_bFree = FALSE;
    }

    //(5)update CurPPO
    g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurPPO = usCurPPO;

    //(6)Free pages--
    g_PMTManager->m_PMTBlkManager[ucSuperPu].m_FreePagesCnt--;

    return SUCCESS;
}

U16 L2_FindFreeBlockofPMT(U8 ucSuperPu)
{
    U16 usBlockSN;
    U16 usCurBlkSN;
    U16 i;

    usCurBlkSN = g_PMTManager->m_PMTBlkManager[ucSuperPu].m_CurBlkSN;
    for (i = 0; i < AT1_BLOCK_COUNT; i++)
    {
        usBlockSN = (usCurBlkSN + i) % AT1_BLOCK_COUNT;
        if (TRUE == g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[usBlockSN].m_bFree)
        {
            return usBlockSN;
        }
    }

    return INVALID_4F;
}

U16 L2_GetBlockSNOfPMTPBN(U8 ucSuperPu, U8 ucLunInSuperPu, U16 BlockInCE)
{
    U8 uBlockSN;

    for (uBlockSN = 0; uBlockSN < AT1_BLOCK_COUNT; uBlockSN++)
    {
        if (BlockInCE == L2_RT_GetAT1BlockPBNFromSN(ucSuperPu, ucLunInSuperPu, uBlockSN))
        {
            return uBlockSN;
        }
    }

    return INVALID_4F;
}

U16 L2_GetDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN)
{
    return g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlockSN].m_DirtyCnt;
}
void L2_IncDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN)
{
    g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlockSN].m_DirtyCnt++;
}
void L2_DecDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN)
{
    g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlockSN].m_DirtyCnt--;
}
void L2_ClearDirtyCntOfPMT(U8 ucSuperPu, U16 BlockSN)
{
    g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlockSN].m_DirtyCnt = 0;
}

void L2_UpdatePhysicalAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 uPMTIIndexInPu, PhysicalAddr* pAddr)
{
    PhysicalAddr OldAddr = { 0 };
    U8 uCESer;
    U16 uBlockInCE, uBlockSN;
    BOOL bReWritePMT = FALSE;
    U8 ucRWCnt;

    OldAddr.m_PPN = L2_GetPMTPhysicalAddr(ucSuperPu, ucLunInSuperPu, uPMTIIndexInPu);
    if (INVALID_8F != OldAddr.m_PPN)
    {
        uCESer = OldAddr.m_PUSer;
        uBlockInCE = OldAddr.m_BlockInPU;
        uBlockSN = L2_GetBlockSNOfPMTPBN(uCESer, OldAddr.m_OffsetInSuperPage, uBlockInCE);

        if (SUPERPU_LUN_NUM_BITMSK == g_PMTManager->m_PMTBitMap[ucSuperPu])
        {
            for(ucRWCnt = 0; ucRWCnt < bReWritePMTCnt; ucRWCnt++)
            {
                if(bReWritePMTIndex[ucRWCnt] == uPMTIIndexInPu)
                {   
                    bReWritePMT = TRUE;
                }
            }
            if(bReWritePMT == FALSE)
            {
                L2_IncDirtyCntOfPMT(uCESer, uBlockSN);
            }
            //FIRMWARE_LogInfo("L2_UpdatePhysicalAddr PMTIndex %d ucSuperPu %d uBlockSN %d DirtyCnt %d\n", uPMTIIndexInPu, ucSuperPu, uBlockSN, g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[uBlockSN].m_DirtyCnt);
        }
    }

    L2_SetPhysicalAddr(ucSuperPu, ucLunInSuperPu, uPMTIIndexInPu, pAddr);
}


U16 L2_FindGCBlockSNOfPMT(U8 ucSuperPu)
{
    U16 uBlockSN, uCurBlkSN;
    U16 uGCBlkSN = INVALID_4F;
    U16  uMaxDirtyCnt = 0;
    U16  uDirtyCnt;
    U16 i;

    uCurBlkSN = L2_GetCurBlockSNOfPMT(ucSuperPu);
    for (i = 0; i < AT1_BLOCK_COUNT; i++)
    {
        uBlockSN = (g_FindTableGCPos[ucSuperPu] + i) % AT1_BLOCK_COUNT;
        /* not select current BlkSN */
        if (uBlockSN == uCurBlkSN)
        {
            continue;
        }

        uDirtyCnt = L2_GetDirtyCntOfPMT(ucSuperPu, uBlockSN);
        if (uDirtyCnt > uMaxDirtyCnt)
        {
            uGCBlkSN = uBlockSN;
            uMaxDirtyCnt = uDirtyCnt;
        }
    }

    g_FindTableGCPos[ucSuperPu]++;
    if (g_FindTableGCPos[ucSuperPu] >= AT1_BLOCK_COUNT)
    {
        g_FindTableGCPos[ucSuperPu] = 0;
    }

    return uGCBlkSN;
}

U16 L2_FindValidPPOOfPMT(U16* pPMTIndexOfPgs)
{
    U16 uPg;
    for (uPg = 0; uPg < PG_PER_SLC_BLK; uPg++)
    {
        if (INVALID_4F != pPMTIndexOfPgs[uPg])
            return uPg;
    }

    DBG_Printf("L2_FindValidPPOOfPMT find no valid PMT page");
    DBG_Getch();
    return  INVALID_2F;
}

U32 L2_FindValidPMTOnGCBlk(U8 ucSuperPu, U16 BlockSN)
{
    TableGCInfo* GCInfo = NULL;
    PhysicalAddr Addr = { 0 };
    U32 PMTIndexInSuperPu;
    U8 ucLunInSuperPu;

    GCInfo = &g_TableRW[ucSuperPu].m_TableGCInfo;
    if (INVALID_4F == GCInfo->m_BlockSN)
    {
        DBG_Printf("ucSuperPu %d no need to do table GC!\n", ucSuperPu);
        DBG_Getch();
    }

    for (PMTIndexInSuperPu = GCInfo->m_PMTPageInPu; PMTIndexInSuperPu < PMTPAGE_CNT_PER_SUPERPU; PMTIndexInSuperPu++)
    {
        /*  Find valid PMT Page */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            Addr.m_PPN = g_PMTI[ucSuperPu]->m_PhyAddr[PMTIndexInSuperPu][ucLunInSuperPu].m_PPN;
            if (BlockSN == (U16)Addr.m_BlockInPU && ucLunInSuperPu == Addr.m_OffsetInSuperPage)
            {
                return PMTIndexInSuperPu;
            }
        }
    }

    return INVALID_8F;
}

U32 L2_FindValidPMTOnWLBlk(U8 ucSuperPu, U16 BlockSN)
{
    TableGCInfo* pWLInfo = NULL;
    PhysicalAddr Addr = { 0 };
    U32 PMTIndexInSuperPu;
    U8 ucLunInSuperPu;

    pWLInfo = &g_TableRW[ucSuperPu].m_TableWLInfo;
    if (INVALID_4F == pWLInfo->m_BlockSN)
    {
        DBG_Printf("ucSuperPu %d no need to do table WL!\n", ucSuperPu);
        DBG_Getch();
    }

    for (PMTIndexInSuperPu = pWLInfo->m_PMTPageInPu; PMTIndexInSuperPu < PMTPAGE_CNT_PER_SUPERPU; PMTIndexInSuperPu++)
    {
        /*  Find valid PMT Page */
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            Addr.m_PPN = g_PMTI[ucSuperPu]->m_PhyAddr[PMTIndexInSuperPu][ucLunInSuperPu].m_PPN;
            if (BlockSN == (U16)Addr.m_BlockInPU && ucLunInSuperPu == Addr.m_OffsetInSuperPage)
            {
                //pWLInfo->m_PMTPageInPu++;
                return PMTIndexInSuperPu;
            }
        }
    }

    return INVALID_8F;
}

//Return TRUE,if actual need Flush
BOOL L2_SetTableRW(U32 ucSuperPu, U16* pPMTIIndexInPu, U16 TableRWCnt, TableRWType TableRWType)
{
    U16 usPMTIIndexInPu;
    U16 usTableRWCnt = 0;
    U16 i, j;

    if (TableRWCnt > TABLE_RW_PAGE_CNT)
    {
        DBG_Printf("TableRWCnt = %d > TABLE_RW_PAGE_CNT = %d\n", TableRWCnt, TABLE_RW_PAGE_CNT);
        DBG_Getch();
    }

    //Check pre-work done
    if (g_TableRW[ucSuperPu].m_TableStatus != TABLE_STATUS_OVER)
    {
        return FALSE;
    }

    /* init g_TableRW.m_PMTIIndex */
    for (i = 0; i < TABLE_RW_PAGE_CNT; i++)
    {
        g_TableRW[ucSuperPu].m_PMTIIndex[i] = INVALID_4F;
        g_TableRW[ucSuperPu].m_PMTIStatus[i] = 0;
		g_TableRW[ucSuperPu].m_TableRWCnt = 0;
    }

    //Check no repulicate PMTIndex 
    for (i = 0; i < TableRWCnt - 1; i++)
    {
        for (j = i + 1; j < TableRWCnt; j++)
        {
            if (pPMTIIndexInPu[i] == pPMTIIndexInPu[j])
            {
                DBG_Printf("MissPMTIIndex repulicate\n");
                DBG_Getch();
            }
        }
    }

    if (TABLE_READ == TableRWType)
    {
        for (i = 0; i < TableRWCnt; i++)
        {
            usPMTIIndexInPu = pPMTIIndexInPu[i];
            g_TableRW[ucSuperPu].m_PMTIIndex[usTableRWCnt] = usPMTIIndexInPu;
            usTableRWCnt++;
        }
    }
    else if (TABLE_WRITE == TableRWType)
    {
        //Get Pointer
        for (i = 0; i < TableRWCnt; i++)
        {
            usPMTIIndexInPu = pPMTIIndexInPu[i];
            if (TRUE == L2_IsBootupOK())
            {
                if (L2_IsPMTPageDirty(ucSuperPu, usPMTIIndexInPu))
                {
                    g_TableRW[ucSuperPu].m_PMTIIndex[usTableRWCnt] = usPMTIIndexInPu;
                    usTableRWCnt++;
                }
            }
            else
            {
                g_TableRW[ucSuperPu].m_PMTIIndex[usTableRWCnt] = usPMTIIndexInPu;
                usTableRWCnt++;
            }
        }
    }
    else
    {
        DBG_Printf("TableRWType = %d, error!\n", TableRWType);
        DBG_Getch();
    }

    if (0 != usTableRWCnt)
    {
        g_TableRW[ucSuperPu].m_TableRWCnt = usTableRWCnt;
        g_TableRW[ucSuperPu].m_TableRWType = TableRWType;
        g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_BEGIN;
        return TRUE;
    }
    else
    {
        return TRUE;
    }
}


U16 L2_GetBlockSNOfPMTPage(U32 ucSuperPu, U16 PMTIndexInSuperPu)
{
    U16 BlockSN;

    PhysicalAddr Addr = { 0 };
    Addr.m_PPN = L2_GetPMTPhysicalAddr(ucSuperPu, 0, PMTIndexInSuperPu);

    BlockSN = L2_GetBlockSNOfPMTPBN(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU);

    return BlockSN;
}

void MCU1_DRAM_TEXT L2_Init_before_rebuild()
{
    U8 i;
    L2_InitDataStructures(FALSE);
    L2_InitTLCInverseProgOrderTable();

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
        L2_InitGCManager(i, TLCGC_MODE);
        L2_ResetTLCManager(i);
#endif
        L2_InitScheduler(i);
        L2_InitFTLScheduler(i);
    }
    L2_TaskEventBootInit();
}


void L2_InitTableWLStructure(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;

    //init table wl structers
    g_TableRW[ucSuperPu].m_TableWLInfo.m_BlockSN = INVALID_4F;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        g_TableRW[ucSuperPu].m_TableWLInfo.m_OriginPBN[ucLunInSuperPu] = INVALID_4F;
    }
    g_TableRW[ucSuperPu].m_TableWLInfo.m_PMTPageInPu = 0;
}

void MCU1_DRAM_TEXT L2_InitTableGCStructure(U8 ucSuperPu)
{
    U8 ucLunInSuperPu;

    //init table wl structers
    g_TableRW[ucSuperPu].m_TableGCInfo.m_BlockSN = INVALID_4F;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        g_TableRW[ucSuperPu].m_TableGCInfo.m_OriginPBN[ucLunInSuperPu] = INVALID_4F;
    }
    g_TableRW[ucSuperPu].m_TableGCInfo.m_PMTPageInPu = 0;
}

//////////////////////////////////////////////////////////////////////////
//void L2_InitPMTManager()
//function:
//    Reset the value of PMTManager to init status
//    the m_GCBuffer[] has been allocated before enter L2_InitPMTManager()
//    the m_pPMTPage[] has been allocated too.
//////////////////////////////////////////////////////////////////////////
void MCU1_DRAM_TEXT L2_InitPMTManager()
{
    U8 i;
    U16 j;
    U8 ucSuperPu;
    U32 ulPMTIndexInPu;
    U8 ucLunInSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndexInPu++)
            {
                g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][ulPMTIndexInPu] = SUBSYSTEM_STATUS_SUCCESS;
            }
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_PMTManager->m_PMTBlkManager[i].m_CurPPO = INVALID_4F;
        g_PMTManager->m_PMTBlkManager[i].m_CurBlkSN = INVALID_4F;
        g_PMTManager->m_PMTBlkManager[i].m_FreePagesCnt = 0;
        g_PMTManager->m_PMTBitMap[i] = 0;
        g_PMTManager->m_PMTBitMapSPOR[i] = 0;
        g_PMTManager->m_CurFlushPMTIndex[i] = INVALID_8F;
        g_PMTManager->m_PMTSaveStage[i] = L2_PMT_SAVE_PREPARE;

        for (j = 0; j < AT1_BLOCK_COUNT; j++)
        {
            g_PMTManager->m_PMTBlkManager[i].m_PMTBlkInfo[j].m_bFree = FALSE;
        }

        g_FindTableGCPos[i] = 0;

        for (j = 0; j < TABLE_RW_PAGE_CNT; j++)
        {
            g_TableRW[i].m_PMTIIndex[j] = INVALID_4F;
        }
        g_TableRW[i].m_TableRWCnt = 0;
        g_TableRW[i].m_TableRWType = TABLE_RW_ALL;
        g_TableRW[i].m_TableStatus = TABLE_STATUS_OVER;

        L2_InitTableWLStructure(i);
        L2_InitTableGCStructure(i);
        g_TableRW[i].m_TableFlushCntStatistic = 0;
        g_TableRW[i].m_TableWLCntStatistic = 0;
    }
}

void MCU1_DRAM_TEXT L2_LLFPMT()
{
    U8 ucSuperPu;
    U16 i;

    L2_InitPMTBlkManager();

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        for (i = 0; i < TABLE_RW_PAGE_CNT; i++)
        {
            g_TableRW[ucSuperPu].m_PMTIIndex[i] = INVALID_4F;
        }
        g_TableRW[ucSuperPu].m_TableRWCnt = 0;
        g_TableRW[ucSuperPu].m_TableRWType = TABLE_RW_ALL;
        g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_OVER;
    }
}

BOOL L2_IsTableNeedGC(U8 ucSuperPu)
{
    U16 FlushPMTCntOfCE = 0;
    U32 ulFreePages;
    BOOL NeedGC = FALSE;

    if (0 == g_TableRW[ucSuperPu].m_TableRWCnt)
    {
        return FALSE;
    }
    else
    {
        ulFreePages = L2_GetFreePagesCntofPMT(ucSuperPu);
        if (ulFreePages > c_ulTableGCThs)
        {
            NeedGC = FALSE;
        }
        else
        {
            NeedGC = TRUE;
        }

        return NeedGC;
    }
}

void L2_SetTableGCInfo(U32 ucSuperPu)
{
    TableGCInfo* GCInfo = NULL;
    U16 uGcBlkSN;
    U32 ulFreePages;
    U8 ucLunInSuperPu;

    //Init GCInfo
    GCInfo = &g_TableRW[ucSuperPu].m_TableGCInfo;

    GCInfo->m_BlockSN = INVALID_4F;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        GCInfo->m_OriginPBN[ucLunInSuperPu] = INVALID_4F;
    }

    /* if there is more free blks, no need to do Table GC in this PU */
    ulFreePages = L2_GetFreePagesCntofPMT(ucSuperPu);
    if (ulFreePages >(2 * c_ulTableGCThs))
    {
        return;
    }

    uGcBlkSN = L2_FindGCBlockSNOfPMT(ucSuperPu);
    if (INVALID_4F == uGcBlkSN)
    {
        DBG_Printf("PU %d find no blk need Table GC\n", ucSuperPu);
        return;
    }

    GCInfo->m_BlockSN = uGcBlkSN;
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        GCInfo->m_OriginPBN[ucLunInSuperPu] = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, ucLunInSuperPu, (U8)uGcBlkSN);
    }
    GCInfo->m_PMTPageInPu = 0;

}


BOOL L2_IsScheduleToTableWL(U32 ucSuperPu)
{
    TableGCInfo* pWLInfo = NULL;

    pWLInfo = &g_TableRW[ucSuperPu].m_TableWLInfo;
    if (INVALID_4F != pWLInfo->m_BlockSN)
    {
        //table flush : table wl = 8:1
        if (0 == (g_TableRW[ucSuperPu].m_TableFlushCntStatistic % RationOfTableFlush2TableWL))
        {
            return TRUE;
        }
    }

    return FALSE;
}

TableQuitStatus L2_TablePreProcess(U8 ucSuperPu)
{
    U16 i = 0;

    if (TABLE_READ == g_TableRW[ucSuperPu].m_TableRWType)
    {
        g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_LOAD;
    }
    else if (TABLE_WRITE == g_TableRW[ucSuperPu].m_TableRWType)
    {
        /* Table GC */
        if (TRUE == L2_IsTableNeedGC(ucSuperPu))
        {
            //DBG_Printf("Table GC\n");
            L2_SetTableGCInfo(ucSuperPu);
            g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_GC;
        }
        else
        {
            if (L2_IsScheduleToTableWL(ucSuperPu))
            {
                g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_WL;
            }
            else
            {
                g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_FLUSH;
            }
        }
    }
    else
    {
        DBG_Printf("g_TableRW[%d].m_TableRWType error!\n", ucSuperPu);
        DBG_Getch();
    }

    return TABLE_QUIT_STAGE_DONE;
}

void L2_PrintfTableBlkEraseCnt(U32 ucSuperPu)
{

    U32 uAverageEraseCnt, uMinEraseCnt, uMaxEraseCnt;
    U32 uEraseCntSum = 0;
    U32 uEraseCnt;
    U16 usBlockSN, usBlkPBN;

    uMinEraseCnt = INVALID_8F;
    uMaxEraseCnt = 0;
    // calc the average value of erase cnt
    for (usBlockSN = 0; usBlockSN < AT1_BLOCK_COUNT; usBlockSN++)
    {
        //get erase cnt
        usBlkPBN = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, 0, (U8)usBlockSN);
        uEraseCnt = L2_PBIT_Get_EraseCnt(ucSuperPu, 0, usBlkPBN);

        uEraseCntSum += uEraseCnt;

        if (uEraseCnt < uMinEraseCnt)
        {
            uMinEraseCnt = uEraseCnt;
        }
        if (uEraseCnt > uMaxEraseCnt)
        {
            uMaxEraseCnt = uEraseCnt;
        }
    }

    uAverageEraseCnt = uEraseCntSum / AT1_BLOCK_COUNT;
}


BOOL L2_IsNeedTableWL(U32 ucSuperPu)
{
    U32 uAverageEraseCnt, uMinEraseCnt, uMaxEraseCnt;
    U32 uEraseCntSum = 0;
    U32 ulLunEraseCnt, ulLunMaxEraseCnt;
    U16 usBlockSN, usBlkPBN;
    U8 ucLunInSuperPu;

    uMinEraseCnt = INVALID_8F;
    uMaxEraseCnt = 0;

    // calc the average value of erase cnt
    for (usBlockSN = 0; usBlockSN < AT1_BLOCK_COUNT; usBlockSN++)
    {
        ulLunMaxEraseCnt = 0;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            usBlkPBN = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, ucLunInSuperPu, (U8)usBlockSN);
            ulLunEraseCnt = L2_PBIT_Get_EraseCnt(ucSuperPu, ucLunInSuperPu, usBlkPBN);

            /* use LunInSuperPu Max EraseCnt as SuperPu AT1 Blk erase cnt */
            if (ulLunEraseCnt > ulLunMaxEraseCnt)
            {
                ulLunMaxEraseCnt = ulLunEraseCnt;
            }
            uEraseCntSum += ulLunEraseCnt;
        }

        if (ulLunMaxEraseCnt < uMinEraseCnt)
        {
            uMinEraseCnt = ulLunMaxEraseCnt;
        }

        if (ulLunMaxEraseCnt > uMaxEraseCnt)
        {
            uMaxEraseCnt = ulLunMaxEraseCnt;
        }
    }

    uAverageEraseCnt = (uEraseCntSum / LUN_NUM_PER_SUPERPU) / AT1_BLOCK_COUNT;

    // if the min or the max erase cnt  deviate from average value, then need wl
    if (((uAverageEraseCnt - uMinEraseCnt) > TableWLThresholdValue) || ((uMaxEraseCnt - uAverageEraseCnt) > TableWLThresholdValue))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void L2_SetTableWLInfo(U32 ucSuperPu)
{
    TableGCInfo* pWLInfo = NULL;
    U16 usBlockSN, usBlkPBN;
    U32 ulLunEraseCnt, ulLunMaxEraseCnt;
    U32 uMinEraseCnt = INVALID_8F;
    U32 uMinEraseBlkSN = INVALID_8F;
    U8 ucLunInSuperPu;

    //chose the min erase cnt blk from not free blk
    for (usBlockSN = 0; usBlockSN < AT1_BLOCK_COUNT; usBlockSN++)
    {
        //chose not free blk 
        if (FALSE == g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[usBlockSN].m_bFree)
        {
            ulLunMaxEraseCnt = 0;
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                usBlkPBN = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, ucLunInSuperPu, (U8)usBlockSN);
                ulLunEraseCnt = L2_PBIT_Get_EraseCnt(ucSuperPu, ucLunInSuperPu, usBlkPBN);

                /* use LunInSuperPu Max EraseCnt as SuperPu AT1 Blk erase cnt */
                if (ulLunEraseCnt > ulLunMaxEraseCnt)
                {
                    ulLunMaxEraseCnt = ulLunEraseCnt;
                }
            }
            if (ulLunMaxEraseCnt < uMinEraseCnt)
            {
                //record min erase blk 
                uMinEraseCnt = ulLunMaxEraseCnt;
                uMinEraseBlkSN = usBlockSN;
            }
        }
    }

    if (INVALID_8F == uMinEraseBlkSN)
    {
        DBG_Printf("not find wl source blk\n");
        DBG_Getch();
    }

    //if min erase cnt blk is cur blk ,no need to do wl
    if (uMinEraseBlkSN != L2_GetCurBlockSNOfPMT(ucSuperPu))
    {
        //set table wl source info
        pWLInfo = &g_TableRW[ucSuperPu].m_TableWLInfo;

        pWLInfo->m_BlockSN = uMinEraseBlkSN;
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            pWLInfo->m_OriginPBN[ucLunInSuperPu] = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, ucLunInSuperPu, (U8)uMinEraseBlkSN);
        }

        pWLInfo->m_PMTPageInPu = 0;
    }

}

U16 L2_GetTableWLSourceBlk(U32 ucSuperPu)
{
    return g_TableRW[ucSuperPu].m_TableWLInfo.m_BlockSN;
}


/************************************************************************
U32 L2_TableWL()
function:
Table WL
parameters:
return value:
If Table WL done, return 0; else return reason code.
************************************************************************/

TableQuitStatus L2_TableWL(U8 ucSuperPu, BOOL *SendCmd)
{
    TableGCInfo* pWLInfo = NULL;
    U16 usBlockSN;
    U16 uDirtyCnt;
    U32 PMTIIndexInPu;
    U32 ulWLDone = TABLE_QUIT_WL_UNDONE;

    pWLInfo = &g_TableRW[ucSuperPu].m_TableWLInfo;
    usBlockSN = pWLInfo->m_BlockSN;

    *SendCmd = FALSE;
    //Check if wl source blk is free, because gc might erase it
    if (TRUE == g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[usBlockSN].m_bFree)
    {
        //WL done
        L2_InitTableWLStructure(ucSuperPu);

        return TABLE_QUIT_STAGE_DONE;
    }

    /* PU FIFO full */
    if (FALSE == L2_FCMDQNotFull(ucSuperPu))
    {
        return TABLE_QUIT_WL_UNDONE;
    }

    uDirtyCnt = L2_GetDirtyCntOfPMT(ucSuperPu, usBlockSN);
    if (uDirtyCnt >= PG_PER_SLC_BLK && (L2_PMT_SAVE_PREPARE == g_PMTManager->m_PMTSaveStage[ucSuperPu]))  /* table WL done in this PU */
    {
        /* check whether this PU have valid PMT pages */
        PMTIIndexInPu = L2_FindValidPMTOnWLBlk(ucSuperPu, usBlockSN);
        if (INVALID_8F != PMTIIndexInPu)
        {
            DBG_Printf("SuperPu %d BlockSN %d have valid PMT pages(%d) after GC!!!\n", ucSuperPu, usBlockSN, PMTIIndexInPu);
            DBG_Getch();
        }

        //WL done
        L2_InitTableWLStructure(ucSuperPu);
        ulWLDone = TABLE_QUIT_STAGE_DONE;
    }
    else  /* find a valid PMT page in WL source block and flush it */
    {
        if (L2_PMT_SAVE_PREPARE == g_PMTManager->m_PMTSaveStage[ucSuperPu])
        {
            /*Find next WL PMTPage*/
            PMTIIndexInPu = L2_FindValidPMTOnWLBlk(ucSuperPu, usBlockSN);

			g_PMTManager->m_CurFlushPMTIndex[ucSuperPu] = PMTIIndexInPu;
	        if (INVALID_8F == PMTIIndexInPu)
	        {
	            DBG_Printf("SuperPu %d BlockSN %d have no valid PMT pages!!!\n", ucSuperPu, usBlockSN);
	            DBG_Getch();
	        }
		}

		PMTIIndexInPu = g_PMTManager->m_CurFlushPMTIndex[ucSuperPu];
		if (TRUE == L2_FlushPMTPage(ucSuperPu, PMTIIndexInPu))
		{
            g_TableRW[ucSuperPu].m_TableWLInfo.m_PMTPageInPu = PMTIIndexInPu + 1;
            g_TableRW[ucSuperPu].m_TableWLCntStatistic++;
            *SendCmd = TRUE;

            ulWLDone = TABLE_QUIT_STAGE_DONE;
		}
    }

    return ulWLDone;
}


/************************************************************************
U32 L2_TableMissGC()
function:
Table GC
parameters:
return value:
If Table GC done, return 0; else return reason code.
************************************************************************/
TableQuitStatus L2_TableGC(U32 ucSuperPu, BOOL *SendCmd)
{
    U32 i = 0;
    U32 PMTIndexInSuperPu;
    U16 BlockSN;
    U16 OriginPBN;
    TableQuitStatus GCDone = TABLE_QUIT_GC_UNDONE;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage = NULL;
#else
	PMTPage* pPMTPage = NULL;
#endif
    TableGCInfo* GCInfo = NULL;
    U16 uDirtyCnt;
    U8 ucLunInSuperPu, ucTLun;
    U32 ucFlashStatus;
    static U32 ucEraseDoneBitMap = 0;
    U32 ulCmdRet;

    *SendCmd = FALSE;
    /*Table GC*/
    GCInfo = &g_TableRW[ucSuperPu].m_TableGCInfo;
    GCDone = TABLE_QUIT_GC_UNDONE; /*GC not done*/

    BlockSN = GCInfo->m_BlockSN;

    /* This PU no need to do TableGC */
    if (INVALID_4F == BlockSN)
    {
        return TABLE_QUIT_STAGE_DONE;
    }

    uDirtyCnt = L2_GetDirtyCntOfPMT(ucSuperPu, BlockSN);
    if (uDirtyCnt >= PG_PER_SLC_BLK && (L2_PMT_SAVE_PREPARE == g_PMTManager->m_PMTSaveStage[ucSuperPu]))  /* table GC done in this PU */
    {
        /* check whether this PU have valid PMT pages */
        PMTIndexInSuperPu = L2_FindValidPMTOnGCBlk(ucSuperPu, BlockSN);
        if (INVALID_8F != PMTIndexInSuperPu)
        {
            DBG_Printf("SuperPu %d BlockSN %d have valid PMT pages(%d) after GC!\n", ucSuperPu, BlockSN, PMTIndexInSuperPu);
            DBG_Getch();
        }

        //L2_PrintfTableBlkEraseCnt( ucSuperPu);
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                continue;
            }
            if (0 != (g_PMTManager->m_PMTBitMap[ucSuperPu] & (1 << ucLunInSuperPu)))
            {
                continue;
            }
            //OriginPBN = GCInfo->m_OriginPBN[ucLunInSuperPu];
            OriginPBN = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, ucLunInSuperPu, (U8)BlockSN);

            /* erase source block */
            pTBManagement->ucFlashStatus[ucSuperPu][ucLunInSuperPu] = SUBSYSTEM_STATUS_PENDING;            
            L2_FtlEraseBlock((U8)ucSuperPu, ucLunInSuperPu, OriginPBN, (U8 *)&(pTBManagement->ucFlashStatus[ucSuperPu][ucLunInSuperPu]), TRUE, TRUE, FALSE);
            L2_PBIT_Increase_EraseCnt(ucSuperPu, ucLunInSuperPu, OriginPBN);
            pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_SLC]++;


            g_PMTManager->m_PMTBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);
        }

        if (SUPERPU_LUN_NUM_BITMSK == g_PMTManager->m_PMTBitMap[ucSuperPu])
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if(ucEraseDoneBitMap & (1 << ucLunInSuperPu))
                    continue;
                ucFlashStatus = pTBManagement->ucFlashStatus[ucSuperPu][ucLunInSuperPu];
                if (SUBSYSTEM_STATUS_PENDING == ucFlashStatus)
                {
                    ;
                }
                else if (SUBSYSTEM_STATUS_SUCCESS == ucFlashStatus)
                {
        
                    ucEraseDoneBitMap |= (1 << ucLunInSuperPu);
                }
                else
                {
                    /* Erase PMT block fail: errorhandling move block */
                    ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPu, ucLunInSuperPu,
                        GCInfo->m_OriginPBN[ucLunInSuperPu], 0, ERASE_ERR);
        
                    if (L2_ERR_HANDLING_PENDING == ulCmdRet)
                    {
                        ;
                    }
                    else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
                    {
                        ucEraseDoneBitMap |= (1 << ucLunInSuperPu);
                    }
                    else
                    {
                        DBG_Printf("L2_TableGC PU 0x%x LUN 0x%x Erase ErrorHandling Fail\n", ucSuperPu, ucLunInSuperPu);
                        DBG_Getch();
                    }
                }
            }

            if (SUPERPU_LUN_NUM_BITMSK == ucEraseDoneBitMap)
            {
                g_PMTManager->m_PMTBitMap[ucSuperPu] = 0;

                L2_ClearDirtyCntOfPMT(ucSuperPu, BlockSN);
                g_PMTManager->m_PMTBlkManager[ucSuperPu].m_PMTBlkInfo[BlockSN].m_bFree = TRUE;
                g_PMTManager->m_PMTBlkManager[ucSuperPu].m_FreePagesCnt += PG_PER_SLC_BLK;
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    GCInfo->m_OriginPBN[ucLunInSuperPu] = INVALID_4F;
                }
                GCInfo->m_BlockSN = INVALID_4F;
                GCDone = TABLE_QUIT_STAGE_DONE;
                *SendCmd = TRUE;
                ucEraseDoneBitMap = 0;
            }
        }
    }
    else  /* find a valid PMT page in GC source block and flush it */
    {
        if (L2_PMT_SAVE_PREPARE == g_PMTManager->m_PMTSaveStage[ucSuperPu])
		{
			/*Find next GC PMTPage*/
			PMTIndexInSuperPu = L2_FindValidPMTOnGCBlk(ucSuperPu, BlockSN);

			g_PMTManager->m_CurFlushPMTIndex[ucSuperPu] = PMTIndexInSuperPu;

			if (INVALID_8F == PMTIndexInSuperPu)
			{
				DBG_Printf("SuperPu %d BlockSN %d have no valid PMT pages!\n", ucSuperPu, BlockSN);
				DBG_Getch();
			}
		}

        PMTIndexInSuperPu = g_PMTManager->m_CurFlushPMTIndex[ucSuperPu];
        if (TRUE == L2_FlushPMTPage(ucSuperPu, PMTIndexInSuperPu))
        {
            g_TableRW[ucSuperPu].m_TableGCInfo.m_PMTPageInPu = PMTIndexInSuperPu + 1;
            //FIRMWARE_LogInfo("L2_FlushPMTPage SuperPu %d BlockSN %d PMTIndexInSuperPu %d uDirtyCnt %d Done!\n", ucSuperPu, BlockSN, PMTIndexInSuperPu, uDirtyCnt);
            *SendCmd = TRUE;
        }
    }

    return GCDone;
}

TableQuitStatus L2_TableLoad(U8 ucSuperPu)
{
    U32 i = 0;
    U32 j = 0;
    TableQuitStatus RWDone = TABLE_QUIT_STAGE_DONE;
    U16 uPMTIIndexInPu;
    PhysicalAddr Addr = { 0 };
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage = NULL;
#else
	PMTPage* pPMTPage = NULL;
#endif

    /*Load Miss PMTPage*/
    RWDone = TABLE_QUIT_STAGE_DONE;
    for (i = 0; i < g_TableRW[ucSuperPu].m_TableRWCnt; i++)
    {
        if (INVALID_4F == g_TableRW[ucSuperPu].m_PMTIStatus[i])
        {
            continue;
        }
        else
        {
            uPMTIIndexInPu = g_TableRW[ucSuperPu].m_PMTIIndex[i];
            RWDone = TABLE_QUIT_RW_UNDONE; /*Load not done*/

            /*Because PMT flush is super page write,check one LUN is enough*/
            Addr.m_PPN = L2_GetPMTPhysicalAddr(ucSuperPu, 0, uPMTIIndexInPu);
            if (Addr.m_PPN == INVALID_8F)
            {
                /* clear PMT page in DRAM */
                for (j = 0; j < PMT_PAGE_SIZE / sizeof(U32); j++)
                {
                    *((U32*)g_PMTManager->m_pPMTPage[ucSuperPu][uPMTIIndexInPu] + j) = INVALID_8F;
                }

                g_TableRW[ucSuperPu].m_PMTIStatus[i] = INVALID_4F;
                continue;
            }

            if (TRUE == L2_LoadPMTPage(ucSuperPu, uPMTIIndexInPu))
            {
                g_TableRW[ucSuperPu].m_PMTIStatus[i] = INVALID_4F;
            }
            else
            {
                break;
            }
        }
    }

    return RWDone;
}

TableQuitStatus L2_TableFlush(U8 ucSuperPu)
{
    U32 i = 0;
    TableQuitStatus RWDone = TABLE_QUIT_STAGE_DONE;
    U16 uPMTIIndexInPu;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage = NULL;
#else
	PMTPage* pPMTPage = NULL;
#endif

    RWDone = TABLE_QUIT_STAGE_DONE;
    /* Flush PMTPage */
    for (i = 0; i < g_TableRW[ucSuperPu].m_TableRWCnt; i++)
    {
        if (INVALID_4F == g_TableRW[ucSuperPu].m_PMTIStatus[i])
        {
            continue;
        }

        uPMTIIndexInPu = g_TableRW[ucSuperPu].m_PMTIIndex[i];

        //The TableGc process may make the Flushing pointer not dirty
        if (L2_IsBootupOK())
        {
            if ((!L2_IsPMTPageDirty(ucSuperPu, uPMTIIndexInPu)) && (L2_PMT_SAVE_PREPARE == g_PMTManager->m_PMTSaveStage[ucSuperPu]))
            {
                g_TableRW[ucSuperPu].m_PMTIStatus[i] = INVALID_4F;
                continue;
            }
        }

        RWDone = TABLE_QUIT_RW_UNDONE;

        if (TRUE == L2_FlushPMTPage(ucSuperPu, uPMTIIndexInPu))
        {
            g_TableRW[ucSuperPu].m_PMTIStatus[i] = INVALID_4F;

            //statistic table flush cnt
            g_TableRW[ucSuperPu].m_TableFlushCntStatistic++;
        }

    }

    return RWDone;
}

/************************************************************************
U32 L2_TableWaitStatus()
function:
Judge load miss PMTPage done or not
parameters:
return value:
If Miss PMTPage load done, return 0; else return reason code.
************************************************************************/
TableQuitStatus L2_TableWaitStatus(U32 ucSuperPu)
{
    U16 i = 0;
    U32 uPMTIndexInPu = INVALID_8F;
    TableQuitStatus WaitDone = TABLE_QUIT_STAGE_DONE;
    U8 ucLunInSuperPu;

    if (TABLE_READ == g_TableRW[ucSuperPu].m_TableRWType)
    {
        if (BOOT_ABNORMAL == g_BootManagement->m_BootType)
        {
            /*Pre Table R/W done or not*/
            WaitDone = TABLE_QUIT_STAGE_DONE;
            for (i = 0; i < g_TableRW[ucSuperPu].m_TableRWCnt; i++)
            {
                uPMTIndexInPu = g_TableRW[ucSuperPu].m_PMTIIndex[i];
                for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
                {
                    if (SUBSYSTEM_STATUS_FAIL == g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][uPMTIndexInPu])
                    {
                        PhysicalAddr Addr = { 0 };

                        DBG_Printf("[AbNormal Boot]:Load PMT Fail,todo table rebuild\n");

                        Addr.m_PPN = L2_GetPMTPhysicalAddr(ucSuperPu, ucLunInSuperPu, g_TableRW[ucSuperPu].m_PMTIIndex[i]);
                        L2_CollectErrInfoOfRebuild(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, Addr.m_PageInBlock, PMT_PAGE_ERR);
                        g_bPMTErrFlag = TRUE;
                        WaitDone = TABLE_QUIT_FLASH_ERROR;
                        break;
                    }
                    else if (g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][uPMTIndexInPu] != SUBSYSTEM_STATUS_SUCCESS
                        && g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][uPMTIndexInPu] != SUBSYSTEM_STATUS_RECC
                        && g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][uPMTIndexInPu] != SUBSYSTEM_STATUS_RETRY_SUCCESS)
                    {
                        WaitDone = TABLE_QUIT_WAIT_UNDONE;
                        break;
                    }
                }
            }
        }
    }

    return WaitDone;
}

//////////////////////////////////////////////////////////////////////////
//BOOL L2_TableEntry(U8 ucPuNum)
//function:
//    flush some table page in idle time.
//flow:
//    TBD...
//
//
//
//////////////////////////////////////////////////////////////////////////
BOOL L2_TableEntry(U8 ucSuperPu)
{
    BOOL Ret = FALSE;
    BOOL bSendCmd = FALSE;
    TableQuitStatus QuitStatus = TABLE_QUIT_STAGE_DONE;
    U32 ulPMTIndexInPu;
    if (L2_IsTLCProLastWordLine(ucSuperPu))
    {
        return TRUE;
    }

    /* Is need flush PMT ? */
    if ((TRUE == L2_IsBootupOK()) && (FALSE == g_L2EventStatus.m_ForceIdle))
    {
        if ((TRUE == L2_IsNeedFlushPMT(ucSuperPu)) && (FALSE == bShutdownPending))
        {
            L2_FlushPMTPageForRebuild(ucSuperPu, &ulPMTIndexInPu);
            if (INVALID_8F == ulPMTIndexInPu)
            {
                /* SPOR Rebuild_DirtyCnt_Flush_PMT stage, already flush all dirty PMT done, it may happen no dirty PMT need flush */
                return TRUE;
            }
        }
    }

    switch (g_TableRW[ucSuperPu].m_TableStatus)
    {
        /* PreProcess */
    case TABLE_STATUS_BEGIN:
        L2_TablePreProcess(ucSuperPu);
        break;

        /* Table GC */
    case TABLE_STATUS_GC:
        QuitStatus = L2_TableGC(ucSuperPu, &bSendCmd);
        if (TABLE_QUIT_STAGE_DONE == QuitStatus)
        {
            //Check need table WL
            if (L2_IsNeedTableWL(ucSuperPu))
            {
                //select wl source blk
                L2_SetTableWLInfo(ucSuperPu);

                //if  find wl source blk, change to TABLE_STATUS_WL		 
                if (INVALID_4F != L2_GetTableWLSourceBlk(ucSuperPu))
                {
                    g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_WL;
                    break;
                }
            }
            g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_FLUSH;
        }
        if ((TRUE == L2_IsBootupOK()) && (TRUE == bSendCmd))
        {
            Ret = TRUE;
        }
        break;

        /* Table WL */
    case TABLE_STATUS_WL:
        QuitStatus = L2_TableWL(ucSuperPu, &bSendCmd);
        if (TABLE_QUIT_STAGE_DONE == QuitStatus)
        {
            g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_FLUSH;
        }
        if ((TRUE == L2_IsBootupOK()) && (TRUE == bSendCmd))
        {
            Ret = TRUE;
        }
        break;

        /* Table load */
    case TABLE_STATUS_LOAD:
        QuitStatus = L2_TableLoad(ucSuperPu);
        if (TABLE_QUIT_STAGE_DONE == QuitStatus)
        {
            g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_WAIT;
        }
        break;

        /* Table flush */
    case TABLE_STATUS_FLUSH:
        QuitStatus = L2_TableFlush(ucSuperPu);
        if (TABLE_QUIT_STAGE_DONE == QuitStatus)
        {
            g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_OVER;
        }
        break;

        /* Table load wait status */
    case TABLE_STATUS_WAIT:
        QuitStatus = L2_TableWaitStatus(ucSuperPu);
        if (TABLE_QUIT_STAGE_DONE == QuitStatus)
        {
            g_TableRW[ucSuperPu].m_TableStatus = TABLE_STATUS_OVER;
        }
        break;

        /* Table entry done */
    case TABLE_STATUS_OVER:
        Ret = TRUE;
        break;
    }

    return Ret;
}

//////////////////////////////////////////////////////////////////////////
//PMTPage* GetPMTPage(U32 LPN)
//function:
//    The PMTPage is stored in DRAM with table swap.
//    "table swap" means some table is load in DRAM, and some in flash.
//        when needed, the table is load from Flash, and dirty PMT table
//        is flush into flash.
//    when this function is entered, the PMT page will be lookuped,
//    and return the address of the PMT Page.
//    if the PMT page is not in DRAM, it will be loaded.
//    scheduller will make sure there is enough DRAM to store the loaded PMT
//    page.
//////////////////////////////////////////////////////////////////////////
#ifdef PMT_ITEM_SIZE_REDUCE
U32* GetPMTPage(U32 LPN)
{
    U8 ucSPU;
    U32 ulPMTIIndexInPu;

    if (1 == SUBSYSTEM_SUPERPU_NUM)
    {
        ucSPU = 0;
    }
    else
    {
        ucSPU = L2_GetSuperPuFromLPN(LPN);
    }
    ulPMTIIndexInPu = L2_GetPMTIIndexInPu(LPN);

    return g_PMTManager->m_pPMTPage[ucSPU][ulPMTIIndexInPu];
}
#endif

#if !defined(PMT_ITEM_SIZE_REDUCE)
PMTPage* GetPMTPage(U32 LPN)
{
    U8 ucPuNum;
    U32 ulPMTIIndexInPu;

    ucPuNum = L2_GetSuperPuFromLPN(LPN);
    ulPMTIIndexInPu = L2_GetPMTIIndexInPu(LPN);

    return g_PMTManager->m_pPMTPage[ucPuNum][ulPMTIIndexInPu];
}

U32 L2_GetPMTAddr(U32 LPN)
{
    PMTPage* pPMTPage;
    U32 LPNInPMTPage;

    pPMTPage = GetPMTPage(LPN);
    LPNInPMTPage = L2_GetOffsetInPMTPage(LPN);

    return (U32)&(pPMTPage->m_PMTItems[LPNInPMTPage]);
}
#endif

extern GLOBAL Err_Info_Manager* l_ErrManagerDptr;
//////////////////////////////////////////////////////////////////////////
//void L2_LookupPMT(PhysicalAddr* pAddr, U32* pLPN)
//function:
//    Get the physical Address for the LPN.
//////////////////////////////////////////////////////////////////////////

LPNLookUPQuitCode L2_LookupPMT(PhysicalAddr* pAddr, U32 LPN, BOOL bRebuild)
{
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
	PMTPage* pPMTPage;
#endif
    U32 LPNInPMTPage;

    if (LPN >= MAX_LPN_IN_DISK)
    {
        DBG_Printf("L2_LookupPMT LPN 0x%x exceed MAX_LPN_IN_DISK 0x%x \n", LPN, MAX_LPN_IN_DISK);
        DBG_Getch();
    }

    TL_PERFORMANCE(PTL_LEVEL_DETAIL, "FTL L2_LookupPMT start");

    pPMTPage = GetPMTPage(LPN);
    LPNInPMTPage = L2_GetOffsetInPMTPage(LPN);

#ifdef PMT_ITEM_SIZE_3BYTE
    L2_PMTItemToPhyAddr(pAddr, &pPMTPage->m_PMTItems[LPNInPMTPage]);
    if (INVALID_8F != pAddr->m_PPN)
    {
        pAddr->m_PUSer = L2_GetSuperPuFromLPN(LPN);
    }
#elif defined(PMT_ITEM_SIZE_REDUCE)
	L2_PMTItemToPhyAddr(pAddr, pPMTPage, LPNInPMTPage);

#else
    pAddr->m_PPN = pPMTPage->m_PMTItems[LPNInPMTPage];
#endif

#ifdef DBG_PMT
    if (FALSE == bRebuild)
    {
        U32 ucSuperPu;
        U16 PMTIndexInSuperPu = INVALID_4F;

        ucSuperPu = L2_GetSuperPuFromLPN(LPN);
        PMTIndexInSuperPu = L2_GetPMTIIndexInPu(LPN);
        if (INVALID_8F != g_DebugPMT[LPN].m_PPN)
        {
            if (g_DebugPMT[LPN].m_PPN != pAddr->m_PPN)
            {
                if (FALSE == L2_IsNeedIgnoreCheck(LPN, pAddr))
                {
                    U32 ulSPU;
                    U32 ulBlk;
                    U32 ulLUN;

                    //Check TLC ppo 384
                    ulSPU = pAddr->m_PUSer;
                    ulBlk = pAddr->m_BlockInPU;
                    ulLUN = pAddr->m_OffsetInSuperPage;

                    if (L2_FindBlkOfErrTLCRPMTC(ulSPU,ulLUN,ulBlk))
                    {
                        //PMT is right ,DBG_PMT is not right because TLC update PMT not done before abnormal shutdown
                        g_DebugPMT[LPN].m_PPN = pAddr->m_PPN;
                    }
                    else
                    {
                        if (INVALID_8F != pAddr->m_PPN && pAddr->m_BlockInPU == dbg_g_PuInfo[pAddr->m_PUSer]->m_TargetBlk[TARGET_TLC_WRITE] && PG_PER_BLK == dbg_g_PuInfo[pAddr->m_PUSer]->m_TargetPPO[TARGET_TLC_WRITE])
						{
							//ignor check for tlc is closed, and update PMT not finish befor abnormal shutdown
                            g_DebugPMT[LPN].m_PPN = pAddr->m_PPN;
						}
						else
						{
							DBG_Printf("lpn:0x%x, PMT:0x%x, DBG_PMT:0x%x\n", LPN, pAddr->m_PPN, g_DebugPMT[LPN].m_PPN);
							DBG_Printf("Pu %d PMTIIndex %d LPNInPMTPage %d\n", ucSuperPu, PMTIndexInSuperPu, LPNInPMTPage);
                            DBG_Printf("PMT SuperPU 0x%x BLK 0x%x PG 0x%x OffSet 0x%x LPNOffset 0x%x \n", pAddr->m_PUSer, pAddr->m_BlockInPU, pAddr->m_PageInBlock, pAddr->m_OffsetInSuperPage, pAddr->m_LPNInPage);
                            DBG_Printf("DebugPMT SuperPU 0x%x BLK 0x%x PG 0x%x OffSet 0x%x LPNOffset 0x%x \n", g_DebugPMT[LPN].m_PUSer, g_DebugPMT[LPN].m_BlockInPU, g_DebugPMT[LPN].m_PageInBlock, g_DebugPMT[LPN].m_OffsetInSuperPage, g_DebugPMT[LPN].m_LPNInPage);
							DBG_Getch();
						}
                    }
                }
            }
        }
    }
#endif

    TL_PERFORMANCE(PTL_LEVEL_DETAIL, "FTL L2_LookupPMT end");

    return LPN_LOOKUP_SUCCESS;
}

LPNLookUPQuitCode L2_UpdatePMT(PhysicalAddr* pNewAddr, PhysicalAddr* pOriginalAddr, U32 ulLpn)
{
    // Sean Gao 20150818
    //
    // 1. modify L2_UpdatePMT so that we can pass the original address of the target LPN if it
    // has already been looked up, otherwise we have to access the same PMT information in the
    // FTL process flow twice, which means 1 extra pointless dram read for every LPN we process
    //
    // 2. optimize the structure and the style

#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPmtPage;
#else
	PMTPage* pPmtPage;
#endif
    U32 ulLpnOffsetInPmt;
    PhysicalAddr OriginalAddr = { 0 };
    U8 ucSuperPu;
    U32 ulPmtIndex;
    U32* pVaildLPNAddr;

#ifdef SIM
    // check the legitimacy of the input LPN
    if (ulLpn >= MAX_LPN_IN_DISK)
    {
        DBG_Printf("L2_UpdatePMT ulLpn 0x%x exceeds MAX_LPN_IN_DISK 0x%x\n", ulLpn, MAX_LPN_IN_DISK);
        DBG_Getch();
    }
#endif

    // get the corresponding PMT page of the current LPN
    pPmtPage = GetPMTPage(ulLpn);

    // get the offset in the PMT page for the current LPN
    ulLpnOffsetInPmt = L2_GetOffsetInPMTPage(ulLpn);

    // calculate the pu number the current LPN belongs to
    ucSuperPu = L2_GetSuperPuFromLPN(ulLpn);

    // calculate the PMT index
    ulPmtIndex = L2_GetPMTIIndexInPu(ulLpn);

    pVaildLPNAddr = (U32*)&(g_PMTManager->m_PMTSpareBuffer[ucSuperPu][ulPmtIndex]->m_ValidLPNCountSave);

    // fetch the original address information for the LPN,
    // note that we should only do this if pOriginalAddr is NULL,
    // that is, the original address information of the LPN hasn't
    // been looked up, otherwise, we're wasting time
    if ((pOriginalAddr == NULL) || (pOriginalAddr->m_PPN == INVALID_8F))
    {
        if (pOriginalAddr == NULL)
        {
            pOriginalAddr = &OriginalAddr;
        }
        // fetch the original address information from the PMT
#ifdef PMT_ITEM_SIZE_3BYTE
        L2_PMTItemToPhyAddr(pOriginalAddr, &pPmtPage->m_PMTItems[ulLpnOffsetInPmt]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
		L2_PMTItemToPhyAddr(pOriginalAddr, pPmtPage, ulLpnOffsetInPmt);
#else
        OriginalAddr.m_PPN = pPmtPage->m_PMTItems[ulLpnOffsetInPmt];
#endif
    }

    // update valid LPN count of the PMT page(this is why we need the original address information)
    if((INVALID_8F == pOriginalAddr->m_PPN) && (INVALID_8F != pNewAddr->m_PPN))
    {
#ifdef ValidLPNCountSave_IN_DSRAM1
        U24incOne(ucSuperPu, ulPmtIndex);
#else
        (*pVaildLPNAddr)++;
#endif
    }
    else if((INVALID_8F != pOriginalAddr->m_PPN) && (INVALID_8F == pNewAddr->m_PPN))
    {
#ifdef ValidLPNCountSave_IN_DSRAM1
        U24decOne(ucSuperPu, ulPmtIndex); 
#else
        (*pVaildLPNAddr)--;
#endif
    }

    // update the address information of the LPN in the PMT with the new address
#ifdef PMT_ITEM_SIZE_3BYTE
    L2_PhyAddrToPMTItem(pNewAddr, &pPmtPage->m_PMTItems[ulLpnOffsetInPmt]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
	L2_PhyAddrToPMTItem(pNewAddr, pPmtPage, ulLpnOffsetInPmt);
#else
    pPmtPage->m_PMTItems[ulLpnOffsetInPmt] = pNewAddr->m_PPN;
#endif

    // mark the PMT page as dirty, since it has just been modified
    L2_SetDirty(ucSuperPu, ulPmtIndex);

    return LPN_LOOKUP_SUCCESS;
}

BOOL L2_LoadPMTPage(U8 ucSuperPu, U16 PMTIndexInSuperPu)
{
    PhysicalAddr Addr = { 0 };
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
	PMTPage* pPMTPage;
#endif
    U8* pStatus;
    RED* pPMTSpareBuffer;
    U8 ucTLun, ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
        if (FALSE == L2_FCMDQNotFull(ucTLun))
        {
            continue;
        }

        if (0 != (g_PMTManager->m_PMTBitMap[ucSuperPu] & (1 << ucLunInSuperPu)))
        {
            continue;
        }

        g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu] = SUBSYSTEM_STATUS_PENDING;
        Addr.m_PPN = L2_GetPMTPhysicalAddr(ucSuperPu, ucLunInSuperPu, PMTIndexInSuperPu);

        pPMTPage = g_PMTManager->m_pPMTPage[ucSuperPu][PMTIndexInSuperPu];
        pStatus = &g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu];

        //Note:
        pPMTSpareBuffer = g_PMTManager->m_PMTSpareBuffer[ucSuperPu][PMTIndexInSuperPu];

#ifdef PMT_ITEM_SIZE_REDUCE
        L2_FtlReadLocal((U32*)((U32)pPMTPage + ucLunInSuperPu*BUF_SIZE), &Addr, pStatus, (U32 *)pPMTSpareBuffer, LPN_PER_BUF, 0, TRUE, TRUE);
#else
		L2_FtlReadLocal((U32*)((U32)&pPMTPage->m_Page + ucLunInSuperPu*BUF_SIZE), &Addr, pStatus, (U32 *)pPMTSpareBuffer, LPN_PER_BUF, 0, TRUE, TRUE);
#endif

        g_PMTManager->m_PMTBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);
    }

    if (SUPERPU_LUN_NUM_BITMSK == g_PMTManager->m_PMTBitMap[ucSuperPu])
    {
        g_PMTManager->m_PMTBitMap[ucSuperPu] = 0;
        L2_ClearDirty(ucSuperPu, PMTIndexInSuperPu);
        return TRUE;
    }

    return FALSE;
}

U32 L2_CheckFlushPMTStatus(U8 ucSuperPu, U16 PMTIndexInSuperPu)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        if (SUBSYSTEM_STATUS_FAIL == g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu])
        {
            return L2_SAVE_FAIL;
        }
        else if (SUBSYSTEM_STATUS_SUCCESS != g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu])
        {
            return L2_SAVE_PENDING;
        }
    }

    return L2_SAVE_SUCCESS;
}

void L2_ResetSavePMTStage()
{
    U8 ucSuperPu;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        g_PMTManager->m_PMTSaveStage[ucSuperPu] = L2_PMT_SAVE_WRITE_PAGE;
    }

    return;
}

BOOL L2_FlushPMTPage(U32 ucSuperPu, U16 PMTIndexInSuperPu)
{
    U16 BlockSN;
    U16 PPO;
    U16 PBN;
    PhysicalAddr Addr = { 0 };
    RED Spare;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage = NULL;
#else
	PMTPage* pPMTPage = NULL;
#endif
    U8* pStatus;
    U8* ulPMTBufferAddr;
    U8 ucTLun, ucLunInSuperPu;
    L2_PMT_SAVE_STAGE* pPMTSaveStage = &(g_PMTManager->m_PMTSaveStage[ucSuperPu]);
    U32 ulCmdRet;
    U8 ucRWCnt;
	U32 EraseStructSize;
	U8 *TempAddr;

    switch (*pPMTSaveStage)
    {
    case L2_PMT_SAVE_PREPARE:
            g_ulPMTFlushing = TRUE;
            *pPMTSaveStage = L2_PMT_SAVE_WRITE_PAGE;

    case L2_PMT_SAVE_WRITE_PAGE:
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            ucTLun = L2_GET_TLUN(ucSuperPu, ucLunInSuperPu);
            if (FALSE == L2_FCMDQNotFull(ucTLun))
            {
                continue;
            }

            if (0 != (g_PMTManager->m_PMTBitMap[ucSuperPu] & (1 << ucLunInSuperPu)))
            {
                continue;
            }

            g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu] = SUBSYSTEM_STATUS_PENDING;
            pStatus = &g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu];
            pPMTPage = g_PMTManager->m_pPMTPage[ucSuperPu][PMTIndexInSuperPu];
            BlockSN = L2_GetCurBlockSNOfPMT(ucSuperPu);
            PBN = L2_RT_GetAT1BlockPBNFromSN((U8)ucSuperPu, ucLunInSuperPu, (U8)BlockSN);
            PPO = L2_GetCurPPOOfPMT(ucSuperPu);

            Addr.m_PUSer = ucSuperPu;
            Addr.m_BlockInPU = PBN;
            Addr.m_PageInBlock = PPO;
            Addr.m_OffsetInSuperPage = ucLunInSuperPu;
            Addr.m_LPNInPage = 0;

            if (0 == g_PMTManager->m_PMTBitMap[ucSuperPu])
            {
                L2_ClearDirty(ucSuperPu, PMTIndexInSuperPu);
                L2_IncTimeStampInPu(ucSuperPu);
            }

            Spare.m_RedComm.ulTimeStamp = L2_GetTimeStampInPu(ucSuperPu);
            Spare.m_RedComm.ulTargetOffsetTS = L2_GetTargetOffsetTS(&g_PMTManager->m_PMTBitMap[ucSuperPu]);
            Spare.m_RedComm.bsVirBlockAddr = Addr.m_BlockInPU;
            Spare.m_RedComm.bcPageType = PAGE_TYPE_PMT;
            Spare.m_RedComm.bcBlockType = BLOCK_TYPE_PMT;
            Spare.m_RedComm.eOPType = OP_TYPE_PMT_WRITE;
            Spare.m_PMTIndex = PMTIndexInSuperPu;

            COM_MemCpy(Spare.m_DirtyBitMap, g_PMTI[ucSuperPu]->m_PMTDirtyBitMapInCE.m_DirtyBitMap, PMT_DIRTY_BM_SIZE);

#ifdef ValidLPNCountSave_IN_DSRAM1
            Spare.m_ValidLPNCountSave = U24getValue(ucSuperPu, PMTIndexInSuperPu);
#else
            Spare.m_ValidLPNCountSave = g_PMTManager->m_PMTSpareBuffer[ucSuperPu][PMTIndexInSuperPu]->m_ValidLPNCountSave;
#endif

            //FIRMWARE_LogInfo("L2_FlushPMTPage SPU %d PMTIndexInSuperPu %d LUN %d BlockSN %d_Blk %d Pg %d\n", ucSuperPu, PMTIndexInSuperPu, ucLunInSuperPu, BlockSN, PBN, PPO);

            ulPMTBufferAddr = (U8*)pPMTPage + ucLunInSuperPu * BUF_SIZE;

            // record erase infor in PMT page
            if (ucLunInSuperPu == (LUN_NUM_PER_SUPERPU - 1))
            {
                EraseStructSize = sizeof(RecordEraseInfo);
                TempAddr = (U8*)ulPMTBufferAddr + BUF_SIZE - EraseStructSize;
                //if (g_L2EventStatus.m_Shutdown != TRUE)
                //{
                    if(L2_SetEraseInfoAfterFlushPMT(ucSuperPu))
                    {
                        COM_MemByteCopy(TempAddr, (U8 *)&g_NeedEraseBlkInfo[ucSuperPu], EraseStructSize);
                        g_EraseOpt[ucSuperPu].bEraseScheduleEn = TRUE;
                    }
                    else
                    {
					    //DBG_Printf("no blk need erase, S%d_T%d\n", L2_GetEraseQueueSize(ucSuperPu,TRUE), L2_GetEraseQueueSize(ucSuperPu,FALSE));
                        L2_ClearEraseInfo(ucSuperPu);
                        COM_MemByteCopy(TempAddr, (U8 *)&g_NeedEraseBlkInfo[ucSuperPu], EraseStructSize);
                    }
                //}
                //else
                //{
                    //L2_ClearEraseInfo(ucSuperPu);
                    //COM_MemByteCopy(TempAddr, (U8 *)&g_NeedEraseBlkInfo[ucSuperPu], EraseStructSize);
                //}
        	}
			
            /* update PBIT 1st page Info */
            if (0 == Addr.m_PageInBlock)
            {
                L2_Set_TableBlock_PBIT_Info(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU, &Spare);
            }

	        if (L2_IsTLCProLastWordLine(ucSuperPu))
	        {
	            DBG_Printf("Flush Pmt during tlc last wordline program error\n");
	            DBG_Getch();
	        }

	        L2_FtlWriteLocal(&Addr, (U32*)ulPMTBufferAddr, (U32*)&Spare, pStatus, TRUE, TRUE, NULL);

	#ifdef L2MEASURE
	        L2MeasureLogIncWCnt(ucSuperPu, L2MEASURE_TYPE_AT1_SLC);
	#endif

            g_PMTManager->m_PMTBitMap[ucSuperPu] |= (1 << ucLunInSuperPu);

            L2_UpdatePhysicalAddr(ucSuperPu, ucLunInSuperPu, PMTIndexInSuperPu, &Addr);
        }

        if (SUPERPU_LUN_NUM_BITMSK == g_PMTManager->m_PMTBitMap[ucSuperPu])
        {
            *pPMTSaveStage = L2_PMT_SAVE_WAIT_WRITE_PAGE;
        }
        break;

    case L2_PMT_SAVE_WAIT_WRITE_PAGE:
        if (L2_SAVE_PENDING == L2_CheckFlushPMTStatus(ucSuperPu, PMTIndexInSuperPu))
        {
            return FALSE;
        }
        else if (L2_SAVE_SUCCESS == L2_CheckFlushPMTStatus(ucSuperPu, PMTIndexInSuperPu))
        {
            U8 ucCount;
#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
            L2_IncTimeStampInPu(ucSuperPu);
#endif
            g_PMTManager->m_PMTBitMap[ucSuperPu] = 0;
            ucCount = bReWritePMTCnt;
            for(ucRWCnt = 0; ucRWCnt < ucCount; ucRWCnt++)
            {
                if(bReWritePMTIndex[ucRWCnt] == PMTIndexInSuperPu)
                {
                    bReWritePMTIndex[ucRWCnt] = INVALID_4F;
                    bReWritePMTCnt--;
                }
            }
            //L2_ClearDirty(ucSuperPu, PMTIndexInSuperPu);
            L2_IncCurPOOfPMT(ucSuperPu);
            if (g_EraseOpt[ucSuperPu].bEraseScheduleEn == TRUE)
            {
                if (g_EraseOpt[ucSuperPu].uErasingTLCVBN != INVALID_4F)
                    g_EraseOpt[ucSuperPu].bEraseTLC = TRUE;

                g_EraseOpt[ucSuperPu].L2NeedEraseBlk = TRUE;
                g_EraseOpt[ucSuperPu].bEraseScheduleEn = FALSE;
            }

            /* Reset PMTSaveStage */
            *pPMTSaveStage = L2_PMT_SAVE_PREPARE;
            g_ulPMTFlushing = FALSE;
            return TRUE;
        }
        else
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                if (SUBSYSTEM_STATUS_FAIL == g_PMTManager->m_FlushStatus[ucSuperPu][ucLunInSuperPu][PMTIndexInSuperPu])
                {
                    *pPMTSaveStage = L2_PMT_SAVE_FAIL_MOVE_BLOCK;

                    /* Use PMTBitMap to indicate save fail LUN*/
                    g_PMTManager->m_PMTBitMap[ucSuperPu] &= ~(1 << ucLunInSuperPu);

                    BlockSN = L2_GetCurBlockSNOfPMT(ucSuperPu);
                    PPO = L2_GetCurPPOOfPMT(ucSuperPu);

                    DBG_Printf("L2_FlushPMTPage fail,SuperPu %d LUN %d PMTIIndexInPu=%d, BlockSN=%d, PPO=%d\n", ucSuperPu, ucLunInSuperPu,
                        PMTIndexInSuperPu, L2_RT_GetAT1BlockPBNFromSN(ucSuperPu, ucLunInSuperPu, (U8)BlockSN), PPO);
                }
            }
        }

        break;

    case L2_PMT_SAVE_FAIL_MOVE_BLOCK:
        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            if (0 != (g_PMTManager->m_PMTBitMap[ucSuperPu] & (1 << ucLunInSuperPu)))
            {
                continue;
            }

            /* save PMT write page fail: errorhandling move block */
            BlockSN = L2_GetCurBlockSNOfPMT(ucSuperPu);
            PPO = L2_GetCurPPOOfPMT(ucSuperPu);

            ulCmdRet = L2_TableErrorHandlingMoveBlock(ucSuperPu, ucLunInSuperPu, L2_RT_GetAT1BlockPBNFromSN(ucSuperPu, ucLunInSuperPu, (U8)BlockSN), PPO, WRITE_ERR);

            if (L2_ERR_HANDLING_PENDING == ulCmdRet)
            {
                ;
            }
            else if (L2_ERR_HANDLING_SUCCESS == ulCmdRet)
            {
                bReWritePMTIndex[bReWritePMTCnt] = PMTIndexInSuperPu;
                bReWritePMTCnt++;                /* Rewrite Save fail Lun, use g_PMTManager->m_PMTBitMap to indicate redo LUN*/
                *pPMTSaveStage = L2_PMT_SAVE_WRITE_PAGE;
            }
            else
            {
                DBG_Printf("L2_LLF_SavePMT PU 0x%x LUN 0x%x Write ErrorHandling Fail\n", ucSuperPu, ucLunInSuperPu);
                DBG_Getch();
            }
        }
        break;

    case L2_PMT_SAVE_SUCCESS:
        break;

    default:
        DBG_Printf("SuperPu %d Invalid flush PMT stage 0x%x\n",ucSuperPu,*pPMTSaveStage);
        DBG_Getch();
        break;
    }

    return FALSE;
}

U16 MCU1_DRAM_TEXT L2_RebuildFindCurAT1TargetBlkSN(U8 uSuperPU)
{
    U16 BlkSN, CurAT1Blk;

    /*use Lun 0 AT1 block number to find corresponding AT1 SN*/ 
    CurAT1Blk = pRT->m_RT[uSuperPU].CurAT1Block[0];
    for (BlkSN = 0; BlkSN < AT1_BLOCK_COUNT; BlkSN++)
    {
        if (CurAT1Blk == pRT->m_RT[uSuperPU].AT1BlockAddr[0][BlkSN])
            break;
    }

    return BlkSN;
}

U32 MCU1_DRAM_TEXT L2_RebuildFindEmptyTableBlkSN(U32 ucSuperPu)
{
    U32 BlkSN;
    BOOL BlkEmptyFlag;
    U32 uLUNInSuperPU;

    //find free blk
    for (BlkSN = 0; BlkSN < AT1_BLOCK_COUNT; BlkSN++)
    {
        BlkEmptyFlag = TRUE;
        for(uLUNInSuperPU=0 ; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
        {
            BlkEmptyFlag = BlkEmptyFlag && (0 == l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[ucSuperPu][BlkSN][uLUNInSuperPU]);
        }
        if (BlkEmptyFlag)
        {
            //Find Empty Tablle Blk
            return BlkSN;
        }
    }

    //Not find empty Talble Blk
    return INVALID_8F;
}


extern BOOL L2_RebuildIsSuperPMTPageHadW(U32 uSuperPU, U16 BlkSN, U16 page, U32 * uPMTBitMap);
void MCU1_DRAM_TEXT L2_ResetTableManagerByInfo()
{
    U32 uSuperPU;
    U32 uLUNInSuperPU;
    U32 BlkSN;
    U32 BlkInCE;
    U32 uFreePagesCnt[SUBSYSTEM_SUPERPU_MAX] = { 0 };
    U16 CurBlkSN;
    U16 CurPPO;
    U16 PMTIndexInSuperPu;
    U32 TableBlkTS;
    BOOL bHadWrite = FALSE;

    //Reset PMTBlk Mapping Table
    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        for (BlkSN = 0; BlkSN < AT1_BLOCK_COUNT; BlkSN++)
        {
            for (uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                BlkInCE = l_RebuildPMTManagerDptr->m_TableBlk[uSuperPU][BlkSN][uLUNInSuperPU];

                if (INVALID_8F == BlkInCE)
                {
                    DBG_Printf("Table Blk can not INVALID_4F\n");
                    DBG_Getch();
                }
            }
        }
    }

    //Reset Current PMT CurBlkSN & PPO
    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        U32 MaxTS = 0;
        CurBlkSN = 0;
        CurPPO = 0;
        bHadWrite = FALSE;
        U32 uPMTBitMap = 0;
        
        for (BlkSN = 0; BlkSN < AT1_BLOCK_COUNT; BlkSN++)
        {
            TableBlkTS = l_RebuildPMTManagerDptr->m_TableBlkTS[uSuperPU][BlkSN];
            if (TableBlkTS >= MaxTS)
            {
                CurBlkSN = BlkSN;
                CurPPO = l_RebuildPMTManagerDptr->m_TableBlkPPO[uSuperPU][BlkSN];
                MaxTS = TableBlkTS;
            }
        }

        //Reset Current PPO by err info
        //find newest ppo & blk
   
        
        /* this place can do rebuild ppo bitmap work ==>TODO*/
        
        while (L2_FindErrPMTPageOfRebuild(uSuperPU, CurBlkSN, CurPPO + 1,&bHadWrite))
        {
            CurPPO++;
        }

        //Reset Current PMT CurBlkSN & PPO
        if ((PG_PER_SLC_BLK - 1) == CurPPO)
        {
            //find free blk
            BlkSN = L2_RebuildFindEmptyTableBlkSN(uSuperPU);
            //Check  PMT-Blk is free
            if (INVALID_8F == BlkSN)
            {
                /* patch almost AT1 block is closed, only 1 AT1 block is opened and first page is dummy write issue*/
                if (pRT->m_RT[uSuperPU].CurAT1PPO != 1)
                {
                    DBG_Printf("CurAT1 blk %d PPO %d error\n", pRT->m_RT[uSuperPU].CurAT1Block[0], pRT->m_RT[uSuperPU].CurAT1PPO);
                    DBG_Printf("%d Can find free PMT blk is not free\n", uSuperPU);
                    DBG_Getch();
                }
                else
                {
                    BlkSN = L2_RebuildFindCurAT1TargetBlkSN(uSuperPU);
                    CurPPO = pRT->m_RT[uSuperPU].CurAT1PPO;
                    l_RebuildPMTManagerDptr->m_TableBlkPPO[uSuperPU][BlkSN] = CurPPO;
                    uFreePagesCnt[uSuperPU] += PG_PER_SLC_BLK - CurPPO;
                }
            }
            else
            {
                CurPPO = 0;
            }

            CurBlkSN = BlkSN;
            g_PMTManager->m_PMTBitMapSPOR[uSuperPU] = 0;
        }
        else
        {
            uPMTBitMap = 0;
            CurPPO += 1;
            if(L2_RebuildIsSuperPMTPageHadW(uSuperPU, CurBlkSN, CurPPO, &uPMTBitMap))
            {
                CurPPO++;
            }
            g_PMTManager->m_PMTBitMapSPOR[uSuperPU] = uPMTBitMap;
            uFreePagesCnt[uSuperPU] += PG_PER_SLC_BLK - CurPPO;
        }
        
        g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurBlkSN = CurBlkSN;
        g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurPPO = CurPPO;
    }

    //reset free status
    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        CurBlkSN = g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurBlkSN;
        CurPPO = g_PMTManager->m_PMTBlkManager[uSuperPU].m_CurPPO;

        for (BlkSN = 0; BlkSN < AT1_BLOCK_COUNT; BlkSN++)
        {
            U32 PPO = l_RebuildPMTManagerDptr->m_TableBlkPPO[uSuperPU][BlkSN];
            BOOL BlkEmptyFlag = TRUE;
            for(uLUNInSuperPU = 0; uLUNInSuperPU < LUN_NUM_PER_SUPERPU; uLUNInSuperPU++)
            {
                BlkEmptyFlag = BlkEmptyFlag && (0 == l_RebuildPMTManagerDptr->m_TableBlkFirstEmptyPage[uSuperPU][BlkSN][uLUNInSuperPU]);
            }

            if (BlkEmptyFlag) //if blk is free
            {
                //Set Blk Free;but when current ppo=0,current blk is not free
                if (CurBlkSN == BlkSN)
                {
                    g_PMTManager->m_PMTBlkManager[uSuperPU].m_PMTBlkInfo[BlkSN].m_bFree = FALSE;
                }
                else
                {
                    g_PMTManager->m_PMTBlkManager[uSuperPU].m_PMTBlkInfo[BlkSN].m_bFree = TRUE;
                }

                L2_ClearDirtyCntOfPMT(uSuperPU, BlkSN);
                uFreePagesCnt[uSuperPU] += PG_PER_SLC_BLK;
            }
            //if blk not free
            else
            {
                g_PMTManager->m_PMTBlkManager[uSuperPU].m_PMTBlkInfo[BlkSN].m_bFree = FALSE;

                if (BlkSN == CurBlkSN)
                    g_PMTManager->m_PMTBlkManager[uSuperPU].m_PMTBlkInfo[BlkSN].m_DirtyCnt = CurPPO;
                else
                    g_PMTManager->m_PMTBlkManager[uSuperPU].m_PMTBlkInfo[BlkSN].m_DirtyCnt = PG_PER_SLC_BLK;
            }

        }
        //recaculate m_FreePageCnt
        g_PMTManager->m_PMTBlkManager[uSuperPU].m_FreePagesCnt = uFreePagesCnt[uSuperPU];
    }

    //Update dirtycnt
    for (uSuperPU = 0; uSuperPU < SUBSYSTEM_SUPERPU_NUM; uSuperPU++)
    {
        for (PMTIndexInSuperPu = 0; PMTIndexInSuperPu < PMTPAGE_CNT_PER_SUPERPU; PMTIndexInSuperPu++)
        {
            BlkSN = L2_GetBlockSNOfPMTPage(uSuperPU, PMTIndexInSuperPu);
            if (INVALID_4F == BlkSN)
            {
                //DBG_Printf("PMT BLKSN invalid \n");
                //DBG_Getch();
            }
            else
            {
                L2_DecDirtyCntOfPMT(uSuperPU, BlkSN);
            }
        }
    }

}

BOOL L2_IsPMTTableGC(U8 ucPuNum)
{
    if (INVALID_4F != g_TableRW[ucPuNum].m_TableGCInfo.m_BlockSN)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
