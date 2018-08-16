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
Filename    :L2_Init.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.29
Description :1. declare global variables
2. Allocate memory
3. init data structure to init value
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "FW_BufAddr.h"
#include "L2_StripeInfo.h"
#include "L2_Defines.h"
#include "L2_PMTManager.h"
#include "L2_PMTI.h"
#include "L2_GCManager.h"
#include "L2_Trim.h"
#include "L2_FTL.h"
#include "L2_PMTPage.h"
#include "L2_Boot.h"
#include "L2_StaticWL.h"
#include "L2_ErrorHandling.h"
#include "L2_Interface.h"
#include "L2_RT.h"
#include "L2_TableBlock.h"
#include "L2_SearchEngine.h"
#include "L2_Erase.h"
#include "L2_FullRecovery.h"
#include "L2_Thread.h"
#include "L2_ReadDisturb.h"
#include "HAL_ParamTable.h"
#include "L2_TableBBT.h"
#include "L2_Schedule.h"
#include "L2_FCMDQ.h"
#include "L2_TLCMerge.h"
#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif

#ifdef SIM
#include "Windows.h"
#include "action_define.h"
#include "L2_SimTablecheck.h"
#include "L2_Evaluater.h"
extern void Set_DebugPMTBaseAddr(U32* pFreeDramBase);
extern BOOL g_bWearLevelingStatistic;
#endif

GLOBAL  PMTManager*    g_PMTManager;
GLOBAL  PMTI*          g_PMTI[SUBSYSTEM_LUN_MAX];
GLOBAL  GCManager*     g_GCManager[GC_SRC_MODE];//0 for SLCGC Management; 1 for TLCGC Management;
GLOBAL  DPBM           g_DPBM;     
GLOBAL  DPBM*          pDPBM;

//extern RDT Dram begin address and size
extern GLOBAL U32 g_ulRdtDramAddr;
extern GLOBAL U32 g_ulRdtDramAddrSize;

GLOBAL U32 g_PMTSRAMAddr;
extern GLOBAL  MCU12_VAR_ATTR  PuInfo*        g_PuInfo[SUBSYSTEM_SUPERPU_MAX];
GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;
GLOBAL MCU12_VAR_ATTR GC_ERROR_HANDLE *g_TLCMergeErrHandle;

#if defined(SIM) && (!defined(SWL_OFF))
GLOBAL MCU12_VAR_ATTR SWLRecord *g_SWLRecord[SUBSYSTEM_SUPERPU_MAX];
#endif

#if defined(SWL_EVALUATOR)
GLOBAL MCU12_VAR_ATTR SWLRecord *g_SWLRecord[SUBSYSTEM_SUPERPU_MAX];
#endif

//PBIT* g_PBIT;
GLOBAL  U32 g_RpmtBaseAddr;
GLOBAL MCU12_VAR_ATTR U32 g_RebuildDptrBaseAddr;
GLOBAL  U32 g_DebugPMTBaseAddr;

GLOBAL  U32 g_L2TempBufferAddr;
GLOBAL  U32 g_ulL2DummyDataAddr;// base addrss of dummy data for read without write
GLOBAL  U16 L2RTStartTempBufID;
GLOBAL  U16 L2AT0StartTempBufID;
//mapping table from BlockInCE to PuSN

extern GLOBAL  U32 g_L2EventInit;
extern GLOBAL BOOL g_bL2PbnLoadDone;
//extern GLOBAL  U32 g_GCFreePageCntThs;
extern GLOBAL  U32 g_WLTooColdThs;
extern GLOBAL  U32 g_WLEraseCntThs;
extern GLOBAL  U32 g_DWASustainWriteThs;
extern GLOBAL TLCGCSrcBlkRecord *g_pTLCGCSrcBlkRecord;

//Dirty count rebuild base address for reuse
LOCAL  U32 g_CalcDirtyCntBMBaseAddr;

extern void L2_HostDramMap(U32 *pFreeDramBase);
extern void L2_BbtDramNotPageAlignAllocate(U32* pFreeDramBase);
GLOBAL U32(*g_RPMTAddr)[TARGET_ALL][RPMT_BUFFER_DEPTH];

#ifdef DBG_LC
extern GLOBAL  LC   *dbg_lc;
#endif

//extern BOOL g_bWearLevelingStatistic;

void MCU1_DRAM_TEXT L2_Sram0Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    // g_FTLDptr
    g_FTLDptr = (L2_FTLDptr *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_FTLDptr[0])));

    // g_FTLReadDptr
    g_FTLReadDptr = (L2_FTLReadDptr(*)[LUN_NUM_PER_SUPERPU])ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_FTLReadDptr[0])));

    // l_RdWithoutWtDptr
    L2_HostDramMap(&ulFreeSramBase);
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    // g_PMTFlushManager
    g_PMTFlushManager = (PMTFlushStatistic *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTFlushManager[0])));

    COM_MemAddr16DWAlign(&ulFreeSramBase);

    DBG_Printf("#[L2] allocated %d KB DSRAM0\r\n", (ulFreeSramBase - *pFreeSramBase) / 1024);

    *pFreeSramBase = ulFreeSramBase;
    return;
}

void MCU1_DRAM_TEXT L2_Sram1Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;

    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    //Allocate for pTBManagement
    pTBManagement = (TB_MANAGEMENT *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(sizeof(TB_MANAGEMENT)));
    COM_MemAddr16DWAlign(&ulFreeSramBase);

    pTBRebManagement = (TB_REBUILD_MANAGEMENT *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(sizeof(TB_REBUILD_MANAGEMENT)));

#ifdef ValidLPNCountSave_IN_DSRAM1
    g_pValidLPNCountSaveL = (U16 *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * PMTPAGE_CNT_PER_SUPERPU_MAX * sizeof(U16)));
    g_pValidLPNCountSaveH = (U8 *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * PMTPAGE_CNT_PER_SUPERPU_MAX));
#endif

#ifdef DirtyLPNCnt_IN_DSRAM1
    g_pDirtyLPNCnt = (U32 *)ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * VIR_BLK_CNT * sizeof(U32)));
#endif

#ifdef NEW_TRIM
    g_PMTSRAMAddr = ulFreeSramBase;
    COM_MemIncBaseAddr(&ulFreeSramBase, 2624);
#endif

    COM_MemAddr16DWAlign(&ulFreeSramBase);
   
    DBG_Printf("[L2] allocated %d KB DSRAM1\r\n", (ulFreeSramBase - *pFreeSramBase) / 1024);

    *pFreeSramBase = ulFreeSramBase;
    return;
}

void MCU1_DRAM_TEXT L2_OtfbMap(U32 *pFreeOtfbBase)
{
    return;
}


//////////////////////////////////////////////////////////////////////////
//LOCAL void MCU1_DRAM_TEXT L2_DramMapForGlobalVar( U32 *pFreeDramBase )
//function: allocate DRAM space( size < 32KB ) for partial global varibles.
//          It's used to support PU number self-adaption.
//parameters:
// U32 *pFreeDramBase Base address of current free dram, which is an in/out
//          parameter. Its input/output address will be aligned to 32KB.
//return:
// none
//history:
// 2015/02/26 Javen Liu Create this function.
// 2015/03/03 Javen Liu 16DW alignment
//
//////////////////////////////////////////////////////////////////////////
LOCAL void MCU1_DRAM_TEXT L2_DramMapForGlobalVar(U32 *pFreeDramBase)
{
    U32 ulFreeDramBase;

    ulFreeDramBase = *pFreeDramBase;
    //COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    // g_BootManagement
    g_BootManagement = (BOOT_MANAGEMENT*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(g_BootManagement[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    // g_GCErrHandle
    g_GCErrHandle = (GC_ERROR_HANDLE*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_LUN_NUM * sizeof(g_GCErrHandle[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    // g_SWLErrHandle
    g_SWLErrHandle = (GC_ERROR_HANDLE*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_SWLErrHandle[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    // g_TLCMergeErrHandle
    g_TLCMergeErrHandle = (GC_ERROR_HANDLE*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_TLCMergeErrHandle[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);
#ifdef SIM
    // g_RPMTAddr
    g_RPMTAddr = (U32(*)[TARGET_ALL][RPMT_BUFFER_DEPTH])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_RPMTAddr[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);
#endif

    // g_TableRW
    g_TableRW = (TableRWManager*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_TableRW[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

#if 0
    // l_RdWithoutWtDptr
    L2_HostDramMap(&ulFreeDramBase);
    COM_MemAddr16DWAlign(&ulFreeDramBase);
#endif    

    //DBG_Printf("#[L2] Each MCU allocated %d KB DRAM for global varibles\r\n", (ulFreeDramBase - *pFreeDramBase)/1024 );

    *pFreeDramBase = ulFreeDramBase;

    return;
}

//////////////////////////////////////////////////////////////////////////
//void L2_AllocateDRAM(U32 ulFreeDramBase)
//function:    allocate DRAM space for data structures.
//            including: 1) PMTManager, 2) GCManager, 3)StriepInfo
//
//
//////////////////////////////////////////////////////////////////////////
void MCU1_DRAM_TEXT L2_DramMap(U32* pFreeDramBase)
{
    U16 i;
    U16 j;
    U16 k;
    U32 ulFreeDramBase;
    U32 ulPuNum;
    U32 ulLUNInSuperPU;
    U32 ulDramOffSet = 0;

#ifdef DCACHE
    ulDramOffSet = DRAM_HIGH_ADDR_OFFSET;
#endif  

    ulFreeDramBase = *pFreeDramBase;
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    L2_BbtDramAllocate(&ulFreeDramBase);

    L2_BbtDramNotPageAlignAllocate(&ulFreeDramBase);
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    //Move not need page size align structure together
    L2_DramMapForGlobalVar(&ulFreeDramBase);
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    L2_PatrolReadMgrDramAllocate(&ulFreeDramBase);
#ifdef SCAN_BLOCK_N1
    L2_ScanBlockN1MgrDramAllocate(&ulFreeDramBase);
#endif

    pTBRedInfo = (TB_RED_INFO *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(TB_RED_INFO)));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    pTBRebManagement->ulTheFirstThreePageRed = (RED* (*)[LUN_NUM_PER_SUPERPU][PG_PER_WL])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(pTBRebManagement->ulTheFirstThreePageRed[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
        {
            for (j = 0; j < PG_PER_WL; j++)
            {
                pTBRebManagement->ulTheFirstThreePageRed[i][ulLUNInSuperPU][j] = (RED*)ulFreeDramBase;
                COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RED)));
            }
        }
    }

    //Allocate for PBIT SEARCH
    pPbitSearch = (TB_PBIT_SEARCH *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(TB_PBIT_SEARCH)));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    g_PMTManager = (PMTManager*)(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PMTManager)));

    // Allocate memory for the member of g_PMTManager, by Javen 2015/02/26
    g_PMTManager->m_PMTBlkManager = (PMTBlkManager*)(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_PMTBlkManager[0])));


#ifdef PMT_ITEM_SIZE_REDUCE
    g_PMTManager->m_pPMTPage = (U32* (*)[PMTPAGE_CNT_PER_PU_MAX])(ulFreeDramBase + ulDramOffSet);
#else
    g_PMTManager->m_pPMTPage = (PMTPage* (*)[PMTPAGE_CNT_PER_PU])(ulFreeDramBase + ulDramOffSet);
#endif

#ifdef PMT_ITEM_SIZE_REDUCE
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_pPMTPage[0])));

    g_PMTManager->m_PMTSpareBuffer = (RED* (*)[PMTPAGE_CNT_PER_SUPERPU_MAX])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_PMTSpareBuffer[0])));

    g_PMTManager->m_FlushStatus = (U8(*)[LUN_NUM_PER_SUPERPU][PMTPAGE_CNT_PER_SUPERPU_MAX])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_FlushStatus[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);
#else
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_pPMTPage[0])));

    g_PMTManager->m_PMTSpareBuffer = (RED* (*)[PMTPAGE_CNT_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_PMTSpareBuffer[0])));

    g_PMTManager->m_FlushStatus = (U8(*)[LUN_NUM_PER_SUPERPU][PMTPAGE_CNT_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_PMTManager->m_FlushStatus[0])));
    COM_MemAddr16DWAlign(&ulFreeDramBase);
#endif

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_PMTI[i] = (PMTI*)(ulFreeDramBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PMTI)));
        COM_MemAddr16DWAlign(&ulFreeDramBase);
    }

    /* allocate g_TLCManager */
    g_TLCManager = (TLCMerge*)(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(TLCMerge)));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    g_TLCManager->m_TLCGCSrcAddr = (PhysicalAddr(*)[PG_PER_WL][LPN_PER_SUPER_BLOCK / PG_PER_WL])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_TLCManager->m_TLCGCSrcAddr[0])));
    g_TLCManager->aucTLCBufStatus = (U8(*)[LUN_NUM_PER_SUPERPU][(TLC_BUF_CNT + 1)])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_TLCManager->aucTLCBufStatus[0])));

    g_TLCManager->aucRPMTStatus = (U8(*)[LUN_NUM_PER_SUPERPU][PG_PER_WL])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_TLCManager->aucRPMTStatus[0])));

    g_TLCManager->atSpare = (RED* (*)[LUN_NUM_PER_SUPERPU][TLC_BUF_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_TLCManager->atSpare[0])));

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        U8 ucLunInSuperPu,ucTLCBufIndex;

        for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
        {
            for (ucTLCBufIndex = 0; ucTLCBufIndex < TLC_BUF_CNT; ucTLCBufIndex++)
            {
                g_TLCManager->atSpare[i][ucLunInSuperPu][ucTLCBufIndex] = (RED*)ulFreeDramBase;
                COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RED)));
            }
        }
    }

    /* Allocate Erase pool */
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_EraseQueue[i]= (EraseQueue*)(ulFreeDramBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(EraseQueue)));
    }
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_pForceGCSrcBlkQueue[i] = (BLK_QUEUE*)(ulFreeDramBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(BLK_QUEUE)));
    }
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    g_pTLCGCSrcBlkRecord = (TLCGCSrcBlkRecord*)(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(TLCGCSrcBlkRecord)));

    for (i = 0; i < GC_SRC_MODE; i++)
    {
        g_GCManager[i] = (GCManager*)(ulFreeDramBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(GCManager)));
    }

    for (i = 0; i < GC_SRC_MODE; i++)
    {
        g_GCManager[i]->tGCCommon.m_RPMTStatus = (U8(*)[LUN_NUM_PER_SUPERPU][PG_PER_WL])ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_GCManager[i]->tGCCommon.m_RPMTStatus[0])));

        g_GCManager[i]->tGCCommon.m_FlushStatus = (U8(*)[PG_PER_WL + 1][LPN_PER_SUPERBUF])ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_GCManager[i]->tGCCommon.m_FlushStatus[0])));

        g_GCManager[i]->tGCCommon.m_GCWriteStatus = (U8(*)[LUN_NUM_PER_SUPERPU])ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(g_GCManager[i]->tGCCommon.m_GCWriteStatus[0])));
    }

    //Allocate mem for PMTSpareBuffer
    for (ulPuNum = 0; ulPuNum < SUBSYSTEM_SUPERPU_NUM; ulPuNum++)
    {
        U32 ulPMTIndexInPu;

        for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndexInPu++)
        {
            g_PMTManager->m_PMTSpareBuffer[ulPuNum][ulPMTIndexInPu] = (RED*)ulFreeDramBase;
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RED)));
        }
    }

    /* WL buffer */
    gwl_info = (WL_INFO*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(WL_INFO)));

    gwl_info->tSWLCommon.m_RPMTStatus = (U8(*)[LUN_NUM_PER_SUPERPU][PG_PER_WL])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(gwl_info->tSWLCommon.m_RPMTStatus[0])));

    gwl_info->tSWLCommon.m_FlushStatus = (U8(*)[PG_PER_WL + 1][LPN_PER_SUPERBUF])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM*sizeof(gwl_info->tSWLCommon.m_FlushStatus[0])));

    gwl_info->tSWLCommon.m_GCWriteStatus = (U8(*)[LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(gwl_info->tSWLCommon.m_GCWriteStatus[0])));

    //Allocate for pRTManagement
    pRTManagement = (RT_MANAGEMENT *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RT_MANAGEMENT)));

    //Allocate for pTBMoveBlockManagement
    pTBMoveBlockManagement = (TB_MOVEBLOCK_MANAGEMENT*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(TB_MOVEBLOCK_MANAGEMENT)));

    /* allocate DRAM for table rebuild */
    g_RebuildDptrBaseAddr = ulFreeDramBase;
    l_RebuildDptr = (RebuildDptr*)(ulFreeDramBase + ulDramOffSet);
    L2_InitRebuildDptr();
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RebuildDptr)));

    // Allocate memory for the member of l_RebuildDptr, by Javen 2015/02/27
#ifdef PMT_ITEM_SIZE_REDUCE
    l_RebuildDptr->m_RebuildPMTI.m_PMTPageTS = (U32(*)[PMTPAGE_CNT_PER_PU_MAX])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM*sizeof(l_RebuildDptr->m_RebuildPMTI.m_PMTPageTS[0])));

    l_RebuildDptr->m_RebuildPMTI.m_PMTPageNoDirtyMaxTS = (U32(*)[PMTPAGE_CNT_PER_PU_MAX])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMTI.m_PMTPageNoDirtyMaxTS[0])));
#else
    l_RebuildDptr->m_RebuildPMTI.m_PMTPageTS = (U32(*)[PMTPAGE_CNT_PER_PU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM*sizeof(l_RebuildDptr->m_RebuildPMTI.m_PMTPageTS[0])));

    l_RebuildDptr->m_RebuildPMTI.m_PMTPageNoDirtyMaxTS = (U32(*)[PMTPAGE_CNT_PER_PU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMTI.m_PMTPageNoDirtyMaxTS[0])));
#endif

    l_RebuildDptr->m_RebuildPMT.m_DataBlock = (U32(*)[VIR_BLK_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMT.m_DataBlock[0])));

    l_RebuildDptr->m_RebuildPMT.m_DataBlockType = (U32(*)[VIR_BLK_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMT.m_DataBlockType[0])));

    l_RebuildDptr->m_RebuildPMT.m_DataBlockTS = (U32(*)[VIR_BLK_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMT.m_DataBlockTS[0])));

    l_RebuildDptr->m_RebuildPMT.m_LoadSpareStatus = (U8(*)[RebuildBufferCntInPU][PG_PER_SLC_BLK][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMT.m_LoadSpareStatus[0])));

    l_RebuildDptr->m_RebuildPMT.m_RebuildBlk = (U32(*)[TARGET_ALL][VIR_BLK_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMT.m_RebuildBlk[0])));

    l_RebuildDptr->m_RebuildPMT.m_RebuildBlkTS = (U32(*)[TARGET_ALL][VIR_BLK_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_RebuildPMT.m_RebuildBlkTS[0])));

    l_RebuildDptr->m_BufManager.m_SpareBuffer = (RED* (*)[TARGET_ALL-1][PG_PER_SLC_BLK][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM *sizeof(l_RebuildDptr->m_BufManager.m_SpareBuffer[0])));

    l_RebuildDptr->m_BufManager.m_TLCSpareBuffer = (RED* (*)[PG_PER_SLC_BLK][PG_PER_WL][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM *sizeof(l_RebuildDptr->m_BufManager.m_TLCSpareBuffer[0])));

    l_RebuildDptr->m_BufManager.m_BufferStatus = (U8(*)[PG_PER_SLC_BLK][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_BufManager.m_BufferStatus[0])));

    l_RebuildDptr->m_BufManager.m_TLCPageBufferStatus = (U8(*)[PG_PER_WL][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_BufManager.m_TLCPageBufferStatus[0])));

    l_RebuildDptr->m_BufManager.m_TLCSpareBufferStatus = (U8(*)[PG_PER_SLC_BLK][PG_PER_WL][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_BufManager.m_TLCSpareBufferStatus[0])));

    l_RebuildDptr->m_ErrHanler.m_NeedMoveBlk = (U32(*)[MAXERRBLK])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_ErrHanler.m_NeedMoveBlk[0])));

    l_RebuildDptr->m_ErrHanler.m_NeedMoveBlkTargetType = (U32(*)[MAXERRBLK])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_ErrHanler.m_NeedMoveBlkTargetType[0])));

    l_RebuildDptr->m_ErrHanler.m_MoveTargetBlk = (U16(*)[MAXERRBLK])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_ErrHanler.m_MoveTargetBlk[0])));

    l_RebuildDptr->m_ErrHanler.m_ErrPageCntOfNeedMoveBlk = (U32(*)[MAXERRBLK])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_ErrHanler.m_ErrPageCntOfNeedMoveBlk[0])));

    l_RebuildDptr->m_CalcDirtyCntDptr.m_NeedCalcDirtyBlk = (U32(*)[VIR_BLK_CNT])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_NeedCalcDirtyBlk[0])));

#ifdef PMT_ITEM_SIZE_REDUCE
    g_CalcDirtyCntBMBaseAddr = ulFreeDramBase;
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[i] = (U32(*)[PMTPAGE_VALID_LPN_MAP_SIZE_MAX])(ulFreeDramBase  + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(PMTPAGE_CNT_PER_PU * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[0][0])));
    }
    l_RebuildDptr->m_CalcDirtyCntDptr.m_ValidLPNCountCalc = (U32(*)[PMTPAGE_CNT_PER_PU_MAX])(ulFreeDramBase + ulDramOffSet);
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_ValidLPNCountCalc[0])));
#else    
    l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap = (U32(*)[PMTPAGE_CNT_PER_PU][PMTPAGE_VALID_LPN_MAP_SIZE])(ulFreeDramBase + ulDramOffSet);  
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_bValidBitMap[0])));
    
    l_RebuildDptr->m_CalcDirtyCntDptr.m_ValidLPNCountCalc = (U32(*)[PMTPAGE_CNT_PER_PU])(ulFreeDramBase + ulDramOffSet);    
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_RebuildDptr->m_CalcDirtyCntDptr.m_ValidLPNCountCalc[0])));
#endif
  
    l_ErrHandleDptr->m_EraseStatus = (U8(*)[TARGET_HOST_ALL][LUN_NUM_PER_SUPERPU])ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(l_ErrHandleDptr->m_EraseStatus[0])));
  
    U32 ulFreeDramBase_Reuse = g_CalcDirtyCntBMBaseAddr;
    /* add PBIT Temp buffer for full recovery roll back erase cnt*/
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pPBIT_Temp[i] = (PBIT *)(ulFreeDramBase_Reuse + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase_Reuse, COM_MemSize16DWAlign(sizeof(PBIT)));
        COM_MemAddr16DWAlign(&ulFreeDramBase_Reuse);
    }

    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase_Reuse);
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; (j < RebuildBufferCntInPU - 1); j++)
        {
            /*this buffer is used to load one whole block's redundant data, it need 64Kbyte size */
#ifdef PMT_ITEM_SIZE_REDUCE	
            l_BufferManagerDptr->m_PageBuffer[i][j] = (U32*)(ulFreeDramBase_Reuse + ulDramOffSet);
            COM_MemIncBaseAddr(&ulFreeDramBase_Reuse, COM_MemSize16DWAlign(2 * sizeof(SuperPage)));
#else
            l_BufferManagerDptr->m_PageBuffer[i][j] = (PMTPage*)(ulFreeDramBase_Reuse + ulDramOffSet);
            COM_MemIncBaseAddr(&ulFreeDramBase_Reuse, COM_MemSize16DWAlign(2 * sizeof(PMTPage)));
#endif
            COM_MemAddrPageBoundaryAlign(&ulFreeDramBase_Reuse);
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < PG_PER_SLC_BLK; j++)
        {
            L2_RebuildSetDRAMForSpareBuffer(i, j, &ulFreeDramBase_Reuse);
        }
    }

    if (ulFreeDramBase_Reuse > ulFreeDramBase)
    {
        ulFreeDramBase = ulFreeDramBase_Reuse;
    }
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    //Allocate TLC buffer
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < PG_PER_WL; j++)
        {
#ifdef PMT_ITEM_SIZE_REDUCE
            l_BufferManagerDptr->m_TLCPageBuffer[i][j] = (U32*)(ulFreeDramBase + ulDramOffSet);
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(1 * sizeof(SuperPage)));
#else
            l_BufferManagerDptr->m_TLCPageBuffer[i][j] = (PMTPage*)(ulFreeDramBase + ulDramOffSet);
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(1 * sizeof(PMTPage)));
#endif

        }
    }

    //Allocate TLC spare buffer
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j <PG_PER_SLC_BLK; j++)
        {
            for (k = 0; k < PG_PER_WL; k++)
            {
                for (ulLUNInSuperPU = 0; ulLUNInSuperPU < LUN_NUM_PER_SUPERPU; ulLUNInSuperPU++)
                {
                    l_BufferManagerDptr->m_TLCSpareBuffer[i][j][k][ulLUNInSuperPU] = (RED*)(ulFreeDramBase + ulDramOffSet);
                    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RED)));                    
                }                
            }
        }
    }

    //Move need page size align buffer together
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    // pDPBM
    pDPBM = &g_DPBM;    // Moved from DRAM to SRAM by Javen, 2015/02/27
    pDPBM->m_LpnMap = (DPBM_ENTRY(*)[VIR_BLK_CNT])(ulFreeDramBase + ulDramOffSet);

    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * sizeof(pDPBM->m_LpnMap[0])));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);


	g_ulRdtDramAddr = ulFreeDramBase;
    //Allocate mem for PMTPage
    for (ulPuNum = 0; ulPuNum < SUBSYSTEM_SUPERPU_NUM; ulPuNum++)
    {
        U32 ulPMTIndexInPu;
        for (ulPMTIndexInPu = 0; ulPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; ulPMTIndexInPu++)
        {
#ifdef PMT_ITEM_SIZE_REDUCE
            g_PMTManager->m_pPMTPage[ulPuNum][ulPMTIndexInPu] = (U32*)(ulFreeDramBase + ulDramOffSet);
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(SuperPage)));
#else
            g_PMTManager->m_pPMTPage[ulPuNum][ulPMTIndexInPu] = (PMTPage*)(ulFreeDramBase + ulDramOffSet);
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PMTPage)));
#endif

            COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
        }
    }
	g_ulRdtDramAddrSize = ulFreeDramBase - g_ulRdtDramAddr;

    //allocate DRAM for StriepInfo member variables:
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < (TARGET_ALL - 1); j++)
        {
            for (k = 0; k < RPMT_BUFFER_DEPTH; k++)
            {
                g_PuInfo[i]->m_pRPMT[j][k] = (RPMT*)ulFreeDramBase;
                COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RPMT)));
                COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
            }
        }
    }

    for (k = 0; k < GC_SRC_MODE; k++)
    {
        for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
        {
            if (k == SLCGC_MODE)
            {
                g_GCManager[k]->tGCCommon.m_pRPMT[i][0] = (RPMT*)ulFreeDramBase;
                COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RPMT)));
                COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
            }
            else
            {
                for (j = 0; j < PG_PER_WL; j++)
                {
                    g_GCManager[k]->tGCCommon.m_pRPMT[i][j] = (RPMT*)ulFreeDramBase;
                    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RPMT)));
                    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
                }
            }
        }
    }

    /*TLCGC use TLCManager 10 buffer, no need allocate at present*/
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        /* Modified by Javenliu, 2015-02-02
        * Extra m_GCBuffer[i][1] is needed by GC.
        */
        for (j = 0; j < GC_BUFFER_DEPTH; j++)
        {
            g_GCManager[0]->tGCCommon.m_GCBuffer[i][j] = (SuperPage*)ulFreeDramBase;
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(SuperPage)));
            COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
        }
    }

    /* allocate dram for TLC external copy used */
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        /* ptWriteTLCBuffer[j] : RingBuffers[0]~[2] + Extra's UpperPage[3] */
        for (j = 0; j < (TLC_BUF_CNT + 1); j++)
        {
            for (k = 0; k < LUN_NUM_PER_SUPERPU; k++)
            {
                g_TLCManager->ptWriteTLCBuffer[i][j][k] = (PhysicalPage*)ulFreeDramBase;
                COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PhysicalPage)));
                COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
            }
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < PG_PER_WL; j++)
        {
            g_TLCManager->pRPMT[i][j] = (RPMT*)ulFreeDramBase;
            COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RPMT)));
            COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
        }
    }

#ifdef L2MEASURE
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pMeasureErase[i] = (MeasureBlkEC *)(ulFreeDramBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(MeasureBlkEC)));
        COM_MemAddr16DWAlign(&ulFreeDramBase);
    }
#endif

    /* L2 temp buffer */
    g_L2TempBufferAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * BUF_SIZE * 2));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    //Allocate for temp buffer for RT
    L2RTStartTempBufID = COM_GetBufferIDByMemAddr(ulFreeDramBase, TRUE, BUF_SIZE_BITS);
    ulFreeDramBase += COM_MemSize16DWAlign((BUF_SIZE * SUBSYSTEM_LUN_NUM));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    //Allocate for temp buffer for AT0 table
    L2AT0StartTempBufID = COM_GetBufferIDByMemAddr(ulFreeDramBase, TRUE, BUF_SIZE_BITS);
    ulFreeDramBase += COM_MemSize16DWAlign((SUPER_PAGE_SIZE * SUBSYSTEM_SUPERPU_NUM));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    //Allocate for RT,need 32K align
    pRT = (RT *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RT)));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

#ifdef HOST_NVME

    //allocate DRAM for read without write
    g_ulL2DummyDataAddr = ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(LOGIC_PIPE_PG_SZ));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

#ifdef SIM
    COM_MemSet((U32 *)g_ulL2DummyDataAddr, LOGIC_PIPE_PG_SZ >> 2, INVALID_8F);
#else
    COM_MemSet((U32 *)g_ulL2DummyDataAddr, LOGIC_PIPE_PG_SZ >> 2, 0);
#endif

#endif

    L2_BbtDramPageAlignAllocate(&ulFreeDramBase);
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

#ifdef SIM
    Set_DebugPMTBaseAddr(&ulFreeDramBase);
#endif
    g_DebugPMTBaseAddr = ulFreeDramBase;
#ifdef DBG_PMT
    g_DebugPMT = (PhysicalAddr *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PhysicalAddr) * MAX_LPN_IN_DISK + LPN_PER_BUF)); //L1_Prefetch may greater than maxLPN
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
#endif

#ifdef DBG_TABLE_REBUILD
    /* FTL erase status manager */
    dbg_g_ptFTLEraseStatusManager = (FTL_ERASE_STATUS_MGR*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(FTL_ERASE_STATUS_MGR)));
    COM_MemAddr16DWAlign(&ulFreeDramBase);

    /* WL buffer */
    dbg_gwl_info = (WL_INFO*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(WL_INFO)));
    COM_MemAddr16DWAlign(&ulFreeDramBase);


    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        dbg_g_PMTI[i] = (PMTI*)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PMTI)));
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    }

    //Allocate for VBT
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        dbg_pVBT[i] = (VBT *)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(VBT)));
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        dbg_g_PuInfo[i] = (PuInfo*)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PuInfo)));
    }
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    //allocate DRAM for StriepInfo member variables:
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < TARGET_ALL; j++)
        {
            for (k = 0; k < RPMT_BUFFER_DEPTH; k++)
            {
                dbg_g_PuInfo[i]->m_pRPMT[j][k] = (RPMT*)ulFreeDramBase;
                COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(RPMT)));
                COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
            }
        }
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        dbg_p_m_PMTBlkManager[i] = (PMTBlkManager*)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PMTBlkManager)));
    }
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    DbgForFreeBlockCnt = (U32*)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(U32)));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        dbg_pPBIT[i] = (PBIT *)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(PBIT)));
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    }

#endif

#ifdef DBG_LC    
    dbg_lc = (LC *)ulFreeDramBase;
    COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(LC)));
    COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
#endif

    DBG_Printf("L2 allocated %d MB DRAM\r\n", (ulFreeDramBase - *pFreeDramBase) / 1024 / 1024);

#if defined(SWL_EVALUATOR) && (!defined(SWL_OFF))
    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_SWLRecord[i] = (SWLRecord *)ulFreeDramBase;
        COM_MemIncBaseAddr(&ulFreeDramBase, COM_MemSize16DWAlign(sizeof(SWLRecord)));
        COM_MemAddrPageBoundaryAlign(&ulFreeDramBase);
    }
#endif

    *pFreeDramBase = ulFreeDramBase;
    *g_pMCU1DramEndBase = ulFreeDramBase;

    if (ulFreeDramBase - DRAM_DATA_BUFF_MCU1_BASE > DATA_BUFF_MCU1_SIZE)
    {
        DBG_Printf("MCU#%d Dram OverFlow %d K\n", HAL_GetMcuId(), (ulFreeDramBase - DRAM_DATA_BUFF_MCU1_BASE - DATA_BUFF_MCU1_SIZE) >> 10);
        DBG_Getch();
    }

    return;
}

//////////////////////////////////////////////////////////////////////////
//void L2_InitDataStructures()
//function:    Initial data structures to default value
//            including: 
//            1) Pu mapping Table
//            2) PMTManager, 
//            3) GCManager 
//            4)    StriepInfo
//
//
//////////////////////////////////////////////////////////////////////////
GLOBAL BOOL g_DebugPMTInitMCU1 = 0;
GLOBAL BOOL g_DebugPMTInitMCU2 = 0;
void MCU1_DRAM_TEXT L2_InitDataStructures(BOOL bSecurityErase)
{
    U8 ucSuperPU;
    U8 ucGCMode;

    L2_ClearRebuildDirtyCntFlag();

    L2_InitPMTManager();
    L2_InitPMTI();

    L2_PatrolReadMgrInit();
    L2_IdleGCResetManager();
    L2_FTLEraseStsManagerInit();

    for (ucSuperPU = 0; ucSuperPU < SUBSYSTEM_SUPERPU_NUM; ucSuperPU++)
    {
        for (ucGCMode = 0; ucGCMode < GC_SRC_MODE; ucGCMode++)
        {
            L2_InitGCManager(ucSuperPU, ucGCMode);
        }
        //L2_InitGCEraseBufQ(i);
        L2_InitPuInfo(ucSuperPU);
        L2_ErrorHandlingInit(ucSuperPU);
        L2_EraseQueueInit(ucSuperPU);
        L2_ResetTLCManager(ucSuperPU);
#ifdef L2MEASURE
        L2MeasureLogInit(ucSuperPU);
#endif

#if defined(SWL_EVALUATOR) && (!defined(SWL_OFF))
        SWLRecordInit(ucSuperPU);
#endif
    }
    L2_ForceGCInit();

    L2_InitStatistic();
    L2_InitFTLDptr();/* add by henryluo 2012-08-02 to init g_FTLDptr for L2 */

#ifndef SUBSYSTEM_BYPASS_LLF_BOOT
    if (TRUE != bSecurityErase)
    {
        L2_InitRebuild();
    }
#endif

#ifdef SEARCH_ENGINE
    L2_SearchEngineSWInit();
#endif

    //L2_ErrorHandlingInit();
    L2_SWLInit();
    L2_BootManagementInit();

    g_bPMTErrFlag = FALSE;
    g_L2EventInit = FALSE;
    g_bL2PbnLoadDone = FALSE;
    g_L2EventStatus.m_Shutdown = FALSE;
    g_L2EventStatus.m_WriteAfterShutdown = FALSE;
    g_L2EventStatus.m_ShutdownDone = FALSE;
    g_L2EventStatus.m_ForceIdle = FALSE;
    g_L2EventStatus.m_WaitIdle = FALSE;
#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    g_L2EventStatus.m_ShutdownSharedPageClosedDoneCnt = 0;
#else
    g_L2EventStatus.m_ShutdownEraseTLCBlkDoneCnt = 0;
#endif
    /* init Thresholds form parmmeter table in bootloader */
    {
        BOOTLOADER_FILE *pBootFileAddr = (BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE;
        PTABLE *pPTable = (PTABLE *)&(pBootFileAddr->tSysParameterTable);

        g_WLTooColdThs = pPTable->tL2Feature.aFeatureDW[1];
        g_WLEraseCntThs = pPTable->tL2Feature.aFeatureDW[2];
        g_DWASustainWriteThs = pPTable->tL2Feature.aFeatureDW[3];
    }

    return;
}


#ifdef SIM

//GLOBAL U32 MCU12_VAR_ATTR g_RPMTAddr[PU_NUM][TARGET_ALL][RPMT_BUFFER_DEPTH];
void MCU1_DRAM_TEXT L2_BackupRPMT()
{
    U32 i, j, k;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < (TARGET_ALL - 1); j++)
        {
            for (k = 0; k < RPMT_BUFFER_DEPTH; k++)
            {
                g_RPMTAddr[i][j][k] = (U32)g_PuInfo[i]->m_pRPMT[j][k];
            }
        }
    }
}

void MCU1_DRAM_TEXT L2_RestoreRPMT()
{
    U32 i, j, k;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < (TARGET_ALL - 1); j++)
        {
            for (k = 0; k < RPMT_BUFFER_DEPTH; k++)
            {
                g_PuInfo[i]->m_pRPMT[j][k] = (RPMT*)g_RPMTAddr[i][j][k];
            }
        }
    }
}
#endif
