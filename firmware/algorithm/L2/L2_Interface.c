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
Author      :Nina Yang
Date        :2016.03.09
Description :
Others      :
Modify      :
*******************************************************************************/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "COM_BitMask.h"
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "L2_PBIT.h"
#include "L2_PMTManager.h"
#include "L2_GCManager.h"
#include "L2_TableBBT.h"
#include "L2_Interface.h"
#include "L0_Interface.h"
#include "HAL_SearchEngine.h"
#include "L2_StaticWL.h"
#include "L2_SearchEngine.h"
#include "L3_FlashMonitor.h"
#include "FW_Event.h"
#include "L2_Defines.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
GLOBAL MCU12_VAR_ATTR U32 g_ulSsuInOtfbBaseAddr;
GLOBAL MCU12_VAR_ATTR RED *g_pRedBaseAddr;
GLOBAL MCU12_VAR_ATTR PBIT *pPBIT[SUBSYSTEM_SUPERPU_MAX];
GLOBAL MCU12_VAR_ATTR VBT  *pVBT[SUBSYSTEM_SUPERPU_MAX];
GLOBAL MCU12_VAR_ATTR PuInfo *g_PuInfo[SUBSYSTEM_SUPERPU_MAX];
GLOBAL MCU12_VAR_ATTR BLK_QUEUE*  g_pForceGCSrcBlkQueue[SUBSYSTEM_SUPERPU_MAX];
//GLOBAL MCU12_VAR_ATTR RebuildDptr* l_RebuildDptr;
GLOBAL MCU12_VAR_ATTR U32 g_ulGBBT;

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
LOCAL void MCU12_DRAM_TEXT L2_SharedDSRAM1MemMap(U32 *pFreeSharedSRAM1Base)
{
    U32 ulFreeBase = *pFreeSharedSRAM1Base;
    COM_MemAddr16DWAlign(&ulFreeBase);

    // Part0:
#ifdef HOST_NVME
    ulFreeBase += HAL_CHAIN_NUM_MANAGER_SIZE;
    COM_MemAddr16DWAlign(&ulFreeBase);
#endif

    // Part1:
    g_ptFCmdReq = (FCMD_REQ *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_REQ_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_ptFCmdReqDptr = (FCMD_REQ_DPTR *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_REQ_DPTR_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

    // Part2:
    g_ptFMUserMgr = (FM_USER_ITEM *)ulFreeBase;
    COM_MemZero((U32*)g_ptFMUserMgr, FM_USER_TOT_SZ >> DWORD_SIZE_BITS);

    COM_MemIncBaseAddr(&ulFreeBase, FM_USER_TOT_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_pShareData = (L2TrimShareData *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(L2TrimShareData));
    COM_MemAddr16DWAlign(&ulFreeBase);
    if (ulFreeBase - DSRAM1_MCU12_SHARE_BASE > DSRAM1_MCU12_SHARE_SIZE)
    {
        DBG_Printf("MCU#%d SharedDSRAM1 overflow %dK!\n", HAL_GetMcuId(), (ulFreeBase - DSRAM1_MCU12_SHARE_BASE)/1024);
        DBG_Getch();
    }
    
    DBG_Printf("MCU#%d Alloc shared SRAM1 %d KB, Rsvd %d KB.\n", HAL_GetMcuId(), (ulFreeBase - *pFreeSharedSRAM1Base)/1024, (DSRAM1_MCU12_SHARE_SIZE - (ulFreeBase - *pFreeSharedSRAM1Base))/1024);

    *pFreeSharedSRAM1Base = ulFreeBase;
    
    return;
}

LOCAL void MCU12_DRAM_TEXT L2_SharedOTFBSsuMemMap(U32 *pFreeSharedOTFBBase)
{
    U32 ulFreeBase = *pFreeSharedOTFBBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    // Part1:
    g_ulSsuInOtfbBaseAddr = ulFreeBase;
    g_ptFCmdReqSts = (FCMD_REQSTS *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_REQSTS_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

    if (ulFreeBase - OTFB_SSU0_MCU12_SHARE_BASE > OTFB_MCU12_SHARED_SSU0_SIZE)
    {
        DBG_Printf("MCU#%d SharedOTFB OverFlow L2_SharedOTFBSsuMemMap.\n", HAL_GetMcuId());
        DBG_Getch();
    }

    DBG_Printf("MCU#%d L2_SharedOTFBSsuMemMap %d KB, Rsvd %d kB.\n", HAL_GetMcuId(), (ulFreeBase - *pFreeSharedOTFBBase)/1024, (OTFB_MCU12_SHARED_SSU0_SIZE - (ulFreeBase - *pFreeSharedOTFBBase))/1024);

    *pFreeSharedOTFBBase = ulFreeBase;

    return;
}

LOCAL void MCU12_DRAM_TEXT L2_SharedOTFBRedMemMap(U32 *pFreeSharedOTFBBase)
{
    U32 ulFreeBase = *pFreeSharedOTFBBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    #ifndef RED_MAP_TO_DRAM
    g_pRedBaseAddr = (RED *)ulFreeBase;
    COM_MemZero((U32*)g_pRedBaseAddr, 2*SUBSYSTEM_LUN_NUM*NFCQ_DEPTH*PG_PER_WL*sizeof(RED)/sizeof(U32));
    COM_MemIncBaseAddr(&ulFreeBase, 2*SUBSYSTEM_PU_NUM*NFCQ_DEPTH*PG_PER_WL*sizeof(RED));
    COM_MemAddr16DWAlign(&ulFreeBase);
    #endif
    
    *pFreeSharedOTFBBase = ulFreeBase;
    return;
}

LOCAL void MCU12_DRAM_TEXT L2_SharedDRAMRedMemMap(U32 *pFreeSharedDRAMBase)
{
    U32 ulFreeBase = *pFreeSharedDRAMBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    #ifdef RED_MAP_TO_DRAM
    g_pRedBaseAddr = (RED *)ulFreeBase;
    COM_MemZero((U32*)g_pRedBaseAddr, 2 * SUBSYSTEM_LUN_NUM*NFCQ_DEPTH*PG_PER_WL*sizeof(RED) / sizeof(U32));
    COM_MemIncBaseAddr(&ulFreeBase, 2 * SUBSYSTEM_PU_NUM*NFCQ_DEPTH*PG_PER_WL*sizeof(RED));
    COM_MemAddr16DWAlign(&ulFreeBase);
    #endif
    
    *pFreeSharedDRAMBase = ulFreeBase;
    return;
}

LOCAL void MCU12_DRAM_TEXT L2_Shared16DWAlignDramMemMap(U32 *pFreeSharedDramBase)
{
    U32 i, ulDramOffSet = 0;
    U32 ulFreeBase = *pFreeSharedDramBase;

    COM_MemAddr16DWAlign(&ulFreeBase);

#ifdef DCACHE
    ulDramOffSet = DRAM_HIGH_ADDR_OFFSET;
#endif   

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pPBIT[i] = (PBIT *)(ulFreeBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeBase, sizeof(PBIT));
        COM_MemAddr16DWAlign(&ulFreeBase);
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        pVBT[i] = (VBT *)(ulFreeBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeBase, sizeof(VBT));
        COM_MemAddr16DWAlign(&ulFreeBase);
    }

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        g_PuInfo[i] = (PuInfo*)(ulFreeBase + ulDramOffSet);
        COM_MemIncBaseAddr(&ulFreeBase, sizeof(PuInfo));
        COM_MemAddr16DWAlign(&ulFreeBase);
    }
    
    // OverFlow Check and Update Free Base
    if ((ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE) > DRAM_BUFF_MCU12_SIZE)
    {
        DBG_Printf("MCU#%d L2_Shared16DWAlignDramMemMap Allocate Shared Dram OverFlow %d K\n", HAL_GetMcuId(),((ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE) - DRAM_BUFF_MCU12_SIZE) >> 10);
        DBG_Getch();
    }

    DBG_Printf("MCU#%d L2 allocate the shared-16DwAlign Dram size %d KB, Rsvd %d K.\n", HAL_GetMcuId(), (ulFreeBase - *pFreeSharedDramBase) / 1024, (DRAM_BUFF_MCU12_SIZE - (ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE)) >> 10);
    
    *pFreeSharedDramBase = ulFreeBase;

    return;
}

LOCAL void MCU12_DRAM_TEXT L2_SharedBuffAlignDramMemMap(U32 *pFreeSharedDramBase)
{
    U32 ulFreeBase = *pFreeSharedDramBase;
    COM_MemAddrPageBoundaryAlign(&ulFreeBase);

    // Part 1
    g_ulGBBT = ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, GBBT_BUF_SIZE);
    COM_MemAddrPageBoundaryAlign(&ulFreeBase);
        
    // JasonGuo20161128: move the PbnBindingTable to DCache for improving the perofrmance#0.
#ifdef DCACHE
    g_pMCU12MiscInfo->ulPbnBindingTable = ulFreeBase + DRAM_HIGH_ADDR_OFFSET;
#else
    g_pMCU12MiscInfo->ulPbnBindingTable = ulFreeBase;
#endif
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_NUM * BBT_BLK_PER_PLN * PLN_PER_LUN * sizeof(U16));        
    COM_MemAddrPageBoundaryAlign(&ulFreeBase);
    // OverFlow Check and Update Free Base
    if (ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE >= DRAM_BUFF_MCU12_SIZE)
    {
        DBG_Printf("MCU#%d L2_SharedBuffAlignDramMemMap Allocate Shared Dram OverFlow %d K\n", HAL_GetMcuId(), ((ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE) - DRAM_BUFF_MCU12_SIZE) >> 10);
        DBG_Getch();
    }

    DBG_Printf("MCU#%d L2 allocate the shared-BuffAlign Dram size %d KB, Rsvd %d K.\n", HAL_GetMcuId(), (ulFreeBase - *pFreeSharedDramBase) / 1024, (DRAM_BUFF_MCU12_SIZE - (ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE)) >> 10);

    *pFreeSharedDramBase = ulFreeBase;
    
    return;
}


void MCU12_DRAM_TEXT L1_SharedMemMap(SUBSYSTEM_MEM_BASE * pFreeMemBase)
{
    U32 ulFreeBase = pFreeMemBase->ulFreeDRAMSharedBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_pSubSystemDevParamPage = (DEVICE_PARAM_PAGE *)ulFreeBase;

    COM_MemIncBaseAddr(&ulFreeBase, sizeof(DEVICE_PARAM_PAGE));
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    if (ulFreeBase - DRAM_DATA_BUFF_MCU12_BASE >= DRAM_BUFF_MCU12_SIZE)
    {
        DBG_Printf("MCU#%d Shared Dram Mem OverFlow in L1_SharedMemMap.\n", HAL_GetMcuId());
        DBG_Getch();
    }

    DBG_Printf("MCU#%d L1 allocate the shared Dram size 0x%x KB, Rsvd 0x%x KB\n", HAL_GetMcuId(), (ulFreeBase-pFreeMemBase->ulFreeDRAMSharedBase)/1024, (DRAM_BUFF_MCU12_SIZE - (ulFreeBase-pFreeMemBase->ulFreeDRAMSharedBase))/1024);
    
    pFreeMemBase->ulFreeDRAMSharedBase = ulFreeBase;

    return;
}

void MCU12_DRAM_TEXT L2_SharedMemMap(SUBSYSTEM_MEM_BASE *pFreeMemBase)
{
    L2_SharedDSRAM1MemMap(&pFreeMemBase->ulFreeSRAM1SharedBase);
    L2_SharedOTFBSsuMemMap(&pFreeMemBase->ulSsuInOtfbSharedBase);
    L2_SharedOTFBRedMemMap(&pFreeMemBase->ulRedInOtfbSharedBase);
    L2_SharedDRAMRedMemMap(&pFreeMemBase->ulRedInDramSharedBase);
    L2_Shared16DWAlignDramMemMap(&pFreeMemBase->ulFreeDRAMSharedBase);
    L2_SharedBuffAlignDramMemMap(&pFreeMemBase->ulFreeDRAMSharedBase);
    return;
}

// PBIT Interfaces
void MCU12_DRAM_TEXT L2_PBIT_Set_Weak(U8 ucSuperPu, U16 VirBlockAddr, U8 bTrue)
{
    U8 ucLunInSuperPu;
    U16 PBN;

    // Patch the MCU1/2 both can write PBIT issue.
    return;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        PBN = pVBT[ucSuperPu]->m_VBT[VirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu];
        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bWeak = bTrue;
    }

    return;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_Lock(U8 ucSuperPu, U16 VirBlockAddr, U8 bTrue)
{
    U8 ucLunInSuperPu;
    U16 PBN;

    pVBT[ucSuperPu]->m_VBT[VirBlockAddr].bLockedInWL = bTrue;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        PBN = pVBT[ucSuperPu]->m_VBT[VirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu];
        pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bLock = bTrue;
    }

    return;
}
BOOL MCU12_DRAM_TEXT L2_PBIT_Get_Lock(U8 ucSuperPu, U16 VirBlockAddr)
{
    U8 ucLunInSuperPu;
    U16 PBN;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        PBN = pVBT[ucSuperPu]->m_VBT[VirBlockAddr].PhysicalBlockAddr[ucLunInSuperPu];

        if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bLock)
        {
            return TRUE;
        }
    }

    return FALSE;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_Backup(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bBackup = bTrue;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_Error(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bError = bTrue;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_Free(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bFree = bTrue;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_Allocate(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bAllocated = bTrue;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_Table(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bTable = bTrue;
}
void MCU12_DRAM_TEXT L2_PBIT_Increase_EraseCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].EraseCnt++;
}
void MCU12_DRAM_TEXT L2_PBIT_Decrease_AllocBlockCnt(U8 ucSuperPu, U8 ucBlkType)
{
    pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[ucBlkType]--;
}
U16 MCU12_DRAM_TEXT L2_PBIT_GetVirturlBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN)
{
    if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bError == TRUE)
    {
        return INVALID_4F;
    }

    return pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].VirtualBlockAddr;     
}

void MCU12_DRAM_TEXT L2_PBIT_SetVirturlBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U16 VBN)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].VirtualBlockAddr = VBN;
}

void MCU1_DRAM_TEXT L2_PBIT_Set_RetryFail(U8 ucSuperPu, U16 PhyBlockAddr, U8 ucLunInSuperPu, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlockAddr].bRetryFail = bTrue;
}

U8 MCU1_DRAM_TEXT L2_PBIT_IsTLC(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN)
{
    return pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bTLC;
}
void MCU12_DRAM_TEXT L2_PBIT_Set_TLC(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U8 bTrue)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PBN].bTLC = bTrue;
}

void MCU12_DRAM_TEXT L2_VBT_Set_Free(U8 ucSuperPu, U16 VBN, U8 bTrue)
{
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = bTrue;
    return;
}

BOOL L2_VBT_Get_Free(U8 ucSuperPu, U16 VBN)
{
    return pVBT[ucSuperPu]->m_VBT[VBN].bFree;
}

U8 L2_VBT_Get_TLC(U8 ucSuperPu, U16 VBN)
{
    return pVBT[ucSuperPu]->m_VBT[VBN].bTLC;
}

BOOL L2_IsSubPPOLUNWritten(U32 TargetOffsetBitmap, U8 ucLUNInSuperPU)
{
    BOOL bRet;
    if (0 == TargetOffsetBitmap)
    {
        bRet = TRUE;
    }
    else
    {
        if (0 != (TargetOffsetBitmap & (1 << ucLUNInSuperPU)))
        {
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL MCU12_DRAM_TEXT L2_IsTargetSuperBlkSubPageWritten(U8 ucSuperPU, U8 uLUNInSuperPU, U16 usVirBlk, U16 usVirPage)
{
    U8 ucTarget;
    BOOL bRet = FALSE;
    PuInfo* pInfo;

    // get Target Type from by VirBlk
    pInfo = g_PuInfo[ucSuperPU];
    if (usVirBlk == pInfo->m_TargetBlk[TARGET_HOST_WRITE_NORAML])
    {
        ucTarget = TARGET_HOST_WRITE_NORAML;
    }
    else if (usVirBlk == pInfo->m_TargetBlk[TARGET_HOST_GC])
    {
        ucTarget = TARGET_HOST_GC;
    }
    else if (usVirBlk == pInfo->m_TargetBlk[TARGET_TLC_WRITE])
    {
        ucTarget = TARGET_TLC_WRITE;
    }
    else
    {
        DBG_Printf("L2_IsTargetVirBlkWritten Check Fail. [%d %d %d %d]", ucSuperPU, uLUNInSuperPU, usVirBlk, usVirPage);
        DBG_Getch();
    }
    
    if (usVirPage < (g_PuInfo[ucSuperPU]->m_TargetPPO[ucTarget] - 1))
    {
        bRet = TRUE;
    }
    else if ((usVirPage == (g_PuInfo[ucSuperPU]->m_TargetPPO[ucTarget] - 1)) && TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ucSuperPU]->m_TargetOffsetBitMap[ucTarget], uLUNInSuperPU))
    {
        bRet = TRUE;
    }
    else if ((PG_PER_SLC_BLK - 1) == usVirPage && TRUE == L2_IsSubPPOLUNWritten(g_PuInfo[ucSuperPU]->m_RPMTFlushBitMap[ucTarget], uLUNInSuperPU))
    {
        bRet = TRUE;
    }    

    return bRet;
}

U16 MCU12_DRAM_TEXT L2_GetTargetSuperBlkSubPPO(U8 ucTLun, U16 usVirBlk)
{
    U8 ucSuperPU = L2_GET_SPU(ucTLun);
    U8 ucLUNInSuperPU = L2_GET_LUN_IN_SPU(ucTLun);
    U16 usPage;

    for (usPage = 0; usPage < PG_PER_SLC_BLK; usPage++)
    {
        if (FALSE == L2_IsTargetSuperBlkSubPageWritten(ucSuperPU, ucLUNInSuperPU, usVirBlk, usPage))
        {
            break;
        }
    }

    return usPage;
}

BOOL MCU12_DRAM_TEXT L2_IsClosedSuperBlk(U8 ucSuperPU, U16 usVirBlk)
{
    return VBT_NOT_TARGET == pVBT[ucSuperPU]->m_VBT[usVirBlk].Target ? TRUE : FALSE;
}

void MCU12_DRAM_TEXT L2_SetBlkRetryFail(U16 PUSer, U16 VBN, BOOL RetryFail)
{
    pVBT[PUSer]->m_VBT[VBN].bsRetryFail = RetryFail;
}

void MCU12_DRAM_TEXT L2_Exchange_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usSrcPhyBlock, U16 usTarPhyBlock)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usTarPhyBlock].BlockType = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usSrcPhyBlock].BlockType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usTarPhyBlock].PageType = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usSrcPhyBlock].PageType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usTarPhyBlock].OPType = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usSrcPhyBlock].OPType;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usTarPhyBlock].TimeStamp = pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usSrcPhyBlock].TimeStamp;

    return;
}
void MCU12_DRAM_TEXT L2_Reset_PBIT_Info(U8 ucSuperPu, U8 ucLunInSuperPu, U16 usPhyBlock)
{
    pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyBlock].BlockType = BLOCK_TYPE_GB;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].PageType = PAGE_TYPE_RSVD;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].OPType = 0;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].TimeStamp = 0;
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][usPhyBlock].LastPageTimeStamp = 0;

    return;
}

U16 L2_VBT_GetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN)
{
    return pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
}

void L2_VBT_SetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN, U16 PBN)
{
    pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu] = PBN;
}

U16 MCU12_DRAM_TEXT L2_BM_BackUpBlockEmpty(U8 ucSuperPu, U8 ucLunInSuperPu)
{
    U16 i;
    PBIT_ENTRY* pPBITItem = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];
    U16 Ret;

    Ret = TRUE;
    for (i = 0; i < (BLK_PER_PLN + RSV_BLK_PER_PLN); i++)
    {
        if ((pPBITItem[i].bBackup == TRUE) && (pPBITItem[i].bFree == TRUE))
        {
            Ret = FALSE;
            break;
        }
    }

    return Ret;
}


//L2_BM_AllocateBackUpBlock is use for error handle change vir/phy block mapping
//the return value is different from L2_BM_AllocateFreeBlock
//L2_BM_AllocateFreeBlock return value is vir block addr
//L2_BM_AllocateBackUpBlock return value is phy block addr
U16 MCU12_DRAM_TEXT L2_BM_AllocateBackUpBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 AllocateType, U8 ucTLCBlk)
{
    U16 Pos = INVALID_4F;
    PBIT_ENTRY* pPBITItem = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];

//#ifdef SEARCH_ENGINE
#if 0
    SE_SEARCH_VALUE_RESULT SEGetResult;

    if (BLOCK_TYPE_MAX_ERASE_CNT == AllocateType)
    {
        L2_SESearchBlock(BACKUP_ERASE_CNT_MAX, (U32 *)&(pPBITItem[0]), INVALID_2F, &SEGetResult);
    }
    else
    {
        L2_SESearchBlock(BACKUP_ERASE_CNT_MIN, (U32 *)&(pPBITItem[0]), INVALID_2F, &SEGetResult);
    }

    if (FALSE == SEGetResult.bsFind)
    {
        DBG_Printf("Allocate Fail!\n");
        return INVALID_4F;
    }

    Pos = SEGetResult.ulIndex;
    if ((TRUE != pPBITItem[Pos].bFree) || (TRUE != pPBITItem[Pos].bBackup))
    {
        DBG_Printf("L2_BM_AllocateBackUpBlock: SuperPu %d, LunInSuperPu %d, AllocateType %d, Index 0x%x", 
            ucSuperPu, ucLunInSuperPu, AllocateType, SEGetResult.ulIndex);
        DBG_Getch();
    }

    pPBITItem[Pos].bFree = FALSE;
    pPBITItem[Pos].bAllocated = TRUE;
    pPBITItem[Pos].bTLC = ucTLCBlk;
    g_pSubSystemDevParamPage->UsedRsvdBlockCnt++;
    g_pSubSystemDevParamPage->AvailRsvdSpace--;

    return Pos;
#else
    U32 i;
    U32 MaxEraseCnt = 0;

    if (TRUE != ucTLCBlk)
    {
        for (i = 0; i < (BLK_PER_PLN + RSV_BLK_PER_PLN); i++)
        {
            if (pPBITItem[i].bBackup == FALSE)
            {
                continue;
            }

            if (pPBITItem[i].bFree == FALSE)
            {
                continue;
            }

            Pos = i;
            break;
        }
    }
    else
    {
        for (i = (BLK_PER_PLN + RSV_BLK_PER_PLN); i > 0; i--)
        {
            if (pPBITItem[i].bBackup == FALSE)
            {
                continue;
            }

            if (pPBITItem[i].bFree == FALSE)
            {
                continue;
            }

            Pos = i;
            break;
        }
    }

    if (Pos == INVALID_4F)
    {
        DBG_Printf("Allocate Fail!\n");
        return INVALID_4F;
    }
    else
    {
        pPBITItem[Pos].bFree = FALSE;
        pPBITItem[Pos].bAllocated = TRUE;
        pPBITItem[Pos].bTLC = ucTLCBlk;
        g_pSubSystemDevParamPage->UsedRsvdBlockCnt++;
        g_pSubSystemDevParamPage->AvailRsvdSpace--;

        return Pos;
    }

#endif
}

BOOL MCU12_DRAM_TEXT L2_BM_BrokenBlockEmpty(U8 ucSuperPu, U8 ucLunInSuperPu, U8 ucTLCBlk)
{
    U16 i;
    volatile PBIT_ENTRY* pPBITItem = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];
    BOOL Ret;

    Ret = TRUE;
    for (i = 0; i < (BLK_PER_PLN + RSV_BLK_PER_PLN); i++)
    {
        if ((pPBITItem[i].bBroken == TRUE) && (pPBITItem[i].bFree == TRUE) 
            && (pPBITItem[i].bError == FALSE) && (pPBITItem[i].bTLC == ucTLCBlk))
        {
            Ret = FALSE;
            break; 
        }
    }

    return Ret;
}
U16 MCU12_DRAM_TEXT L2_BM_AllocateBrokenBlock(U8 ucSuperPu, U8 ucLunInSuperPu, U16 AllocateType, U8 ucTLCBlk)
{
    U16 Pos = INVALID_4F;
    PBIT_ENTRY* pPBITItem = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];

#ifdef SEARCH_ENGINE
    SE_SEARCH_VALUE_RESULT SEGetResult;

    if (BLOCK_TYPE_MAX_ERASE_CNT == AllocateType)
    {
        L2_SESearchBlock(BROKEN_ERASE_CNT_MAX, (U32 *)&(pPBITItem[0]), ucTLCBlk, &SEGetResult);
    }
    else
    {
        L2_SESearchBlock(BROKEN_ERASE_CNT_MIN, (U32 *)&(pPBITItem[0]), ucTLCBlk, &SEGetResult);
    }

    if (FALSE == SEGetResult.bsFind)
    {
        DBG_Printf("Allocate Fail!\n");
        return INVALID_4F;
    }

    Pos = SEGetResult.ulIndex;
    if ((TRUE != pPBITItem[Pos].bBroken) || (TRUE != pPBITItem[Pos].bFree)
        || (ucTLCBlk != pPBITItem[Pos].bTLC) || (FALSE != pPBITItem[Pos].bError))
    {
        DBG_Printf("L2_BM_AllocateBrokenBlock:SuperPu %d, ucLunInSuperPu %d,AllocateType %d, ucTLCBlk %d, Index 0x%x\n ", 
            ucSuperPu, ucLunInSuperPu, AllocateType, ucTLCBlk, SEGetResult.ulIndex);
        DBG_Getch();
    }

    pPBITItem[Pos].bFree = FALSE;
    pPBITItem[Pos].bAllocated = TRUE;
    pPBITItem[Pos].bBroken = FALSE;
    g_pSubSystemDevParamPage->UsedRsvdBlockCnt++;
    g_pSubSystemDevParamPage->AvailRsvdSpace--;

    return Pos;
#else
    U32 i;
    DBG_Printf("AllocateType: %x\n", AllocateType);
    if (BLOCK_TYPE_MAX_ERASE_CNT == AllocateType)
    {
        U32 MaxEraseCnt = 0;

        if (TRUE != ucTLCBlk)
        {
            for (i = 0; i < (BLK_PER_PLN + RSV_BLK_PER_PLN); i++)
            {
                if (pPBITItem[i].bBroken == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bFree == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bTLC != ucTLCBlk)
                {
                    continue;
                }

                if (pPBITItem[i].bError == TRUE)
                {
                    continue;
                }

                if (pPBITItem[i].EraseCnt >= MaxEraseCnt)
                {
                    MaxEraseCnt = pPBITItem[i].EraseCnt;
                    Pos = i;
                }
            }
        }
        else
        {
            for (i = (BLK_PER_PLN + RSV_BLK_PER_PLN); i > 0; i--)
            {
                if (pPBITItem[i].bBroken == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bFree == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bTLC != ucTLCBlk)
                {
                    continue;
                }

                if (pPBITItem[i].bError == TRUE)
                {
                    continue;
                }

                if (pPBITItem[i].EraseCnt >= MaxEraseCnt)
                {
                    MaxEraseCnt = pPBITItem[i].EraseCnt;
                    Pos = i;
                }
            }
        }

        if (Pos == INVALID_4F)
        {
            DBG_Printf("Allocate Fail!\n");
            return INVALID_4F;
        }
        else
        {
            pPBITItem[Pos].bFree = FALSE;
            pPBITItem[Pos].bAllocated = TRUE;
            pPBITItem[Pos].bBroken = FALSE;
            g_pSubSystemDevParamPage->UsedRsvdBlockCnt++;
            g_pSubSystemDevParamPage->AvailRsvdSpace--;
            return Pos;
        }
    }
    else if (BLOCK_TYPE_MIN_ERASE_CNT == AllocateType)
    {
        U32 MinEraseCnt = INVALID_8F;
        U16 Pos = INVALID_4F;

        if (TRUE != ucTLCBlk)
        {
            for (i = 0; i < (BLK_PER_PLN + RSV_BLK_PER_PLN); i++)
            {
                if (pPBITItem[i].bBroken == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bFree == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bTLC != ucTLCBlk)
                {
                    continue;
                }

                if (pPBITItem[i].bError == TRUE)
                {
                    continue;
                }

                if (pPBITItem[i].EraseCnt < MinEraseCnt)
                {
                    MinEraseCnt = pPBITItem[i].EraseCnt;
                    Pos = i;
                }
            }
        }
        else
        {
            for (i = (BLK_PER_PLN + RSV_BLK_PER_PLN); i > 0; i--)
            {
                if (pPBITItem[i].bBroken == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bFree == FALSE)
                {
                    continue;
                }

                if (pPBITItem[i].bTLC != ucTLCBlk)
                {
                    continue;
                }

                if (pPBITItem[i].bError == TRUE)
                {
                    continue;
                }

                if (pPBITItem[i].EraseCnt < MinEraseCnt)
                {
                    MinEraseCnt = pPBITItem[i].EraseCnt;
                    Pos = i;
                }
            }
        }

        if (Pos == INVALID_4F)
        {
            DBG_Printf("Allocate Fail!\n");
            return INVALID_4F;
        }
        else
        {
            pPBITItem[Pos].bFree = FALSE;
            pPBITItem[Pos].bAllocated = TRUE;
            pPBITItem[Pos].bBroken = FALSE;
            g_pSubSystemDevParamPage->UsedRsvdBlockCnt++;
            g_pSubSystemDevParamPage->AvailRsvdSpace--;
            return Pos;
        }

    }
    else
    {
        DBG_Printf("Allocate Type error! 0x%x\n", AllocateType);
        return INVALID_4F;
    }
#endif

}

U16 L2_BM_AllocateFreeBlock(U8 ucSuperPu, U16 AllocateType, U8 ucTLCBlk)
{
    U8 ucLunInSuperPu = 0;
    PBIT_ENTRY* pPBITItem = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];
    U16 i = 0;
    U32 j = 0;
    U32 VBN = 0, PBN = 0;
    U8 ucPhyBlkType = BLKTYPE_SLC;

    if (TRUE == ucTLCBlk)
    {
        ucPhyBlkType = BLKTYPE_TLC;
    }

#ifdef SEARCH_ENGINE
    SE_SEARCH_VALUE_RESULT SEGetResult;
    U16 Pos;

#ifdef DCACHE
    HAL_InvalidateDCache();
#endif

    if (BLOCK_TYPE_MAX_ERASE_CNT == AllocateType)
    {
        L2_SESearchBlock(FREE_ERASE_CNT_MAX, (U32 *)&(pPBITItem[0]), ucTLCBlk, &SEGetResult);
    }
    else
    {
        L2_SESearchBlock(FREE_ERASE_CNT_MIN, (U32 *)&(pPBITItem[0]), ucTLCBlk, &SEGetResult);
    }

    if (FALSE == SEGetResult.bsFind)
    {
        DBG_Printf("Allocate Fail!\n");
        return INVALID_4F;
    }

    Pos = SEGetResult.ulIndex;
    if ((TRUE != pPBITItem[Pos].bFree) || (FALSE != pPBITItem[Pos].bBackup) || (FALSE != pPBITItem[Pos].bLock) 
        || (ucTLCBlk != pPBITItem[Pos].bTLC) || (FALSE != pPBITItem[Pos].bError) || (FALSE != pPBITItem[Pos].bBroken))
    {
        DBG_Printf("L2_BM_AllocateFreeBlock:SuperPu %d, ucLunInSuperPu %d,AllocateType %d, bTLCBlk %d FreeBlkCnt %d, Index 0x%x\n ",
            ucSuperPu, ucLunInSuperPu, AllocateType, ucTLCBlk, (g_PuInfo[ucSuperPu]->m_DataBlockCnt[ucTLCBlk] - g_PuInfo[ucSuperPu]->m_AllocateBlockCnt[ucTLCBlk]), SEGetResult.ulIndex);
        return INVALID_4F;
        //DBG_Getch();
    }

    VBN = pPBITItem[Pos].VirtualBlockAddr;
    for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
    {
        PBN = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[j];
        pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bFree = FALSE;
        pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bAllocated = TRUE;

#ifdef READ_DISTURB_OPEN
        if (TRUE == ucTLCBlk)
        {
            L2_ResetBlkReadCnt(ucSuperPu, j, PBN);
        }
#endif
        
        
        L2_Set_PBIT_BlkSN_Blk(ucSuperPu, j, PBN, INVALID_8F);
    }
    pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[ucPhyBlkType]++;

    L2_VBT_Set_Free(ucSuperPu, pPBITItem[Pos].VirtualBlockAddr, FALSE);

#ifdef SIM
    if (pPBITItem[Pos].VirtualBlockAddr == INVALID_4F)
    {
        DBG_Printf("Pos %d\n", Pos);
        DBG_Getch();
    }
#endif
    return pPBITItem[Pos].VirtualBlockAddr;

#else

    if (BLOCK_TYPE_MAX_ERASE_CNT == AllocateType)
    {
        U32 MaxEraseCnt = 0;
        U16 Pos = INVALID_4F;
        for (i = 0; i < BLK_PER_PLN + RSV_BLK_PER_PLN; i++)
        {
            if (pPBITItem[i].bFree == FALSE)
            {
                continue;
            }

            if (pPBITItem[i].bBackup == TRUE)
            {
                continue;
            }

            if (pPBITItem[i].bLock == TRUE)
            {
                continue;
            }

            if (pPBITItem[i].bTLC != ucTLCBlk)
            {
                continue;
            }

            if (pPBITItem[i].bError == TRUE) // added by jasonguo 20140815
            {
                continue;
            }

            if (pPBITItem[i].bBroken == TRUE)
            {
                continue;
            }

            if (pPBITItem[i].EraseCnt > MaxEraseCnt)
            {
                MaxEraseCnt = pPBITItem[i].EraseCnt;
                Pos = i;
            }
        }

        if (Pos == INVALID_4F)
        {
            //DBG_Printf("Allocate Fail !!!\n");
            return INVALID_4F;
        }
        else
        {
            VBN = pPBITItem[Pos].VirtualBlockAddr;
            for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
            {
                PBN = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[j];
                pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bFree = FALSE;
                pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bAllocated = TRUE;

#ifdef READ_DISTURB_OPEN
                if (TRUE == ucTLCBlk)
                {
                    L2_ResetBlkReadCnt(ucSuperPu, j, PBN);
                } 
#endif
                L2_Set_PBIT_BlkSN_Blk(ucSuperPu, j, PBN, INVALID_8F);
            }
            pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[ucPhyBlkType]++;

            //DBG_Printf("SE error patched by SW!\n ");
            L2_VBT_Set_Free(ucSuperPu, pPBITItem[Pos].VirtualBlockAddr, FALSE);
            return pPBITItem[Pos].VirtualBlockAddr;

        }
    }
    else if (BLOCK_TYPE_MIN_ERASE_CNT == AllocateType)
    {
        U32 MinEraseCnt = INVALID_8F;
        U16 Pos = INVALID_4F;

        for (i = 0; i < BLK_PER_PLN + RSV_BLK_PER_PLN; i++)
        {
            if (pPBITItem[i].bFree == FALSE)
            {
                continue;
            }

            if (pPBITItem[i].bBackup == TRUE)
            {
                continue;
            }

            if (pPBITItem[i].bLock == TRUE)
            {
                continue;
            }

            if (pPBITItem[i].bTLC != ucTLCBlk)
            {
                continue;
            }

            if (pPBITItem[i].bError == TRUE) // added by jasonguo 20140815
            {
                continue;
            }

            if (pPBITItem[i].bBroken == TRUE)
            {
                continue;
            }

            if (pPBITItem[i].EraseCnt < MinEraseCnt)
            {
                MinEraseCnt = pPBITItem[i].EraseCnt;
                Pos = i;
            }
        }

        if (Pos == INVALID_4F)
        {
            DBG_Printf("Pu %d Allocate Fail\n", ucSuperPu);
            return INVALID_4F;
        }
        else
        {
            VBN = pPBITItem[Pos].VirtualBlockAddr;
            for (j = 0; j < LUN_NUM_PER_SUPERPU; j++)
            {  
                PBN = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[j];
                pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bFree = FALSE;
                pPBIT[ucSuperPu]->m_PBIT_Entry[j][PBN].bAllocated = TRUE;
#ifdef READ_DISTURB_OPEN
                if (TRUE == ucTLCBlk)
                {
                    L2_ResetBlkReadCnt(ucSuperPu, j, PBN);
                } 
#endif

                L2_Set_PBIT_BlkSN_Blk(ucSuperPu, j, PBN, INVALID_8F);
            }
            pPBIT[ucSuperPu]->m_AllocatedDataBlockCnt[ucPhyBlkType]++;

            //DBG_Printf("Search engine error patched by software!\n ");
            L2_VBT_Set_Free(ucSuperPu, pPBITItem[Pos].VirtualBlockAddr, FALSE);
            return pPBITItem[Pos].VirtualBlockAddr;
        }

    }
    else
    {
        DBG_Printf("Allocate Type error! %x\n", AllocateType);
        return INVALID_4F;
    }


    /*if (Pos != SEGetResult.ulIndex)
    {
        DBG_Printf("ce %d, Pos = 0x%x, ulIndex = 0x%x.\n", ucSuperPu, Pos, SEGetResult.ulIndex);
        DBG_Getch();
    }
    return pPBITItem[Pos].VirtualBlockAddr;
    */

#endif

}

void L2_Set_PBIT_BlkSN_Blk(U8 ucSuperPu, U8 ucLunInSuperPu, U16 PBN, U32 Value)
{
    pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][PBN].CurSerialNum = Value;
}

U32 MCU12_DRAM_TEXT CalcDiffTime(U32 StartTime, U32 EndTime)
{
    U32 DiffTime;
    if (EndTime > StartTime)
    {
        DiffTime = EndTime - StartTime;
    }
    else
    {
        DiffTime = EndTime + (INVALID_8F - StartTime);
    }
    return DiffTime;
}
/*====================End of this file========================================*/

