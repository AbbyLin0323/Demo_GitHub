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
Filename    :L2_Trim.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.09.19
Description :functions about Trim Process
Others      :
Modify      :
*******************************************************************************/
#include "COM_Memory.h"
#include "L2_Trim.h"
#include "L2_VBT.h"
#include "L2_PMTPage.h"
#include "L2_PMTManager.h"
#include "L2_Defines.h"
#include "L2_GCManager.h"
#include "L2_Erase.h"
#include "L2_Ramdisk.h"
#include "L2_Interface.h"
#ifdef SIM
#include "L2_SimTablecheck.h"
#include "HAL_FlashDriverBasic.h"
#endif
#include "FW_Event.h"

#ifndef SIM
#include <xtensa/tie/xt_datacache.h>
#endif
#ifdef SIM
#define XT_DPFR
#endif

#ifdef L2MEASURE
#include "L2_Evaluater.h"
#endif
#define PMTSRAMSwapSize 1312
#define PMTCntPerSwapSRAM 320
U8 g_ucPMTSwapID;

typedef struct _TRIM_DRAM_SRAM_INFO_t{
    U32 PMTSRAMStartAddr;
    U32 LPNInPMTPageStart;
    U32 LPNInPMTPageEnd;
    U8 ucPu;
}TrimRAMInfo_t;

typedef struct _LPN_PMT_INFO{
    U8 ucPu;
    U16 usPMTIIndex;
    U32 ulLPNInPMTPage;
}LPN2PMTInfo;

TrimRAMInfo_t g_TrimRAMInfo[2];

extern GLOBAL U32 g_PMTSRAMAddr;
extern U8 g_ucPMTItemBitCnt;
extern GLOBAL U8  g_ucOffsetInSuperPageBit;

#ifdef SIM
MCU12_VAR_ATTR volatile L2TrimShareData *g_pShareData;
#else
MCU12_VAR_ATTR L2TrimShareData *g_pShareData;
#endif

#define DMAWAIT

BOOL L2_TrimLPN(U32 LPN)
{
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
    PMTPage* pPMTPage;
#endif
    U32 LPNInPMTPage;
    U16 uPMTIIndexInPu = INVALID_4F;
    BOOL bPMTPageInRAM = FALSE;
    BOOL bPMTPageBuilt = FALSE;
    PhysicalAddr OriAddr = { 0 };
    U32 ulPuNum;
    //U32 ulTotalLpnPerBlk;

    ulPuNum = L2_GetSuperPuFromLPN(LPN);
    uPMTIIndexInPu = L2_GetPMTIIndexInPu(LPN);

    /* if current PMTPage is flushing, return directly */
    if (SUBSYSTEM_STATUS_PENDING == g_PMTManager->m_FlushStatus[ulPuNum][0][uPMTIIndexInPu])
    {
        //DBG_Getch();
        return TRUE;
    }

    pPMTPage = g_PMTManager->m_pPMTPage[ulPuNum][uPMTIIndexInPu];
    LPNInPMTPage = L2_GetOffsetInPMTPage(LPN);

#ifdef PMT_ITEM_SIZE_3BYTE
    L2_PMTItemToPhyAddr(&OriAddr, &pPMTPage->m_PMTItems[LPNInPMTPage]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
    L2_PMTItemToPhyAddr(&OriAddr, pPMTPage, LPNInPMTPage);
#else
    OriAddr.m_PPN = pPMTPage->m_PMTItems[LPNInPMTPage];
#endif

    if (OriAddr.m_PPN == INVALID_8F)
    {
        return FALSE;
    }
    else //LPN is valid
    {
        OriAddr.m_PUSer = ulPuNum;

        /* dirty PMT page in PMTI table */
        L2_SetDirty(ulPuNum, uPMTIIndexInPu);

#ifdef DBG_PMT
        if (INVALID_8F != g_DebugPMT[LPN].m_PPN)
        {
            if (g_DebugPMT[LPN].m_PPN != OriAddr.m_PPN)
            {
                if (FALSE == L2_IsNeedIgnoreCheck(LPN, &OriAddr))
                {
                    DBG_Printf("lpn:0x%x, PMT:0x%x, DBG_PMT:0x%x\n", LPN, OriAddr.m_PPN, g_DebugPMT[LPN].m_PPN);
                    DBG_Getch();
                }
            }
        }
        g_DebugPMT[LPN].m_PPN = INVALID_8F;
#endif
        //FIRMWARE_LogInfo("Trim LPN 0x%x -> 0xFFFFFFFF\n", LPN);

        L2_IncreaseDirty(OriAddr.m_PUSer, OriAddr.m_BlockInPU, 1);
        // for reduce cycle count , GC will do it
#if 0
        if (TRUE == L2_VBT_Get_TLC(OriAddr.m_PUSer, OriAddr.m_BlockInPU))
        {
            ulTotalLpnPerBlk = LPN_PER_SUPER_BLOCK - LPN_PER_TLC_RPMT;
        }
        else
        {
            /* TLC mode: block type is SLC, LPN_PER_SUPER_BLOCK/3 */
            ulTotalLpnPerBlk = LPN_PER_SUPER_SLCBLK - LPN_PER_SLC_RPMT;
        }

        if (ulTotalLpnPerBlk == L2_GetDirtyCnt(OriAddr.m_PUSer, OriAddr.m_BlockInPU)
            && FALSE == L2_PBIT_Get_Lock(OriAddr.m_PUSer, OriAddr.m_BlockInPU)
            && VBT_NOT_TARGET == pVBT[ulPuNum]->m_VBT[OriAddr.m_BlockInPU].Target)
        {
            if (TRUE == L2_VBT_Get_TLC(OriAddr.m_PUSer, OriAddr.m_BlockInPU))
            {
                L2_InsertBlkIntoEraseQueue(OriAddr.m_PUSer, OriAddr.m_BlockInPU, FALSE);
            }
            else
            {
                L2_InsertBlkIntoEraseQueue(OriAddr.m_PUSer, OriAddr.m_BlockInPU, TRUE);
            }
            //FIRMWARE_LogInfo("L2_TrimLPN SuperPU %d BLK 0x%x DiryCnt 0x%x into EreaseQue\n", OriAddr.m_PUSer, OriAddr.m_BlockInPU, L2_GetDirtyCnt(OriAddr.m_PUSer, OriAddr.m_BlockInPU));
            
#ifdef L2MEASURE
            L2MeasureLogIncECTyepCnt(OriAddr.m_PUSer, L2_VBT_GetPhysicalBlockAddr(OriAddr.m_PUSer,0, OriAddr.m_BlockInPU), L2MEASURE_ERASE_HOST);
#endif
        }
#endif

#ifdef ValidLPNCountSave_IN_DSRAM1
        U24decOne(ulPuNum, uPMTIIndexInPu); 
#else
        g_PMTManager->m_PMTSpareBuffer[ulPuNum][uPMTIIndexInPu]->m_ValidLPNCountSave--;
#endif

        L2_UpdateDirtyLpnMap(&OriAddr, B_DIRTY);

#ifdef PMT_ITEM_SIZE_3BYTE
        OriAddr.m_PPN = INVALID_8F;
        L2_PhyAddrToPMTItem(&OriAddr, &pPMTPage->m_PMTItems[LPNInPMTPage]);
#elif defined(PMT_ITEM_SIZE_REDUCE)
        OriAddr.m_PPN = INVALID_8F;
        L2_PhyAddrToPMTItem(&OriAddr, pPMTPage, LPNInPMTPage);
#else
        pPMTPage->m_PMTItems[LPNInPMTPage] = INVALID_8F;
#endif

        return TRUE;
    }
}

void L2_TrimDMAESRAMInit(U8 ucPu, U16 usPMTIIndex, U32 ulLPNInPMTPage)
{
    U32 ulDesAddr, ulSrcAddr, ulLenInByte, ulBitTotal;
    U32 ulLPNMaxCnt = 0;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32 *pPMTPage = g_PMTManager->m_pPMTPage[ucPu][usPMTIIndex];
#else
    PMTPage *pPMTPage = g_PMTManager->m_pPMTPage[ucPu][usPMTIIndex];
#endif

    g_TrimRAMInfo[0].ucPu = ucPu;
    g_TrimRAMInfo[0].LPNInPMTPageStart = ulLPNInPMTPage;
    if (ulLPNInPMTPage%PMTCntPerSwapSRAM)
   	{
        ulLPNMaxCnt = ((ulLPNInPMTPage / PMTCntPerSwapSRAM) + 1)*PMTCntPerSwapSRAM;
        ulLPNMaxCnt = ulLPNMaxCnt - ulLPNInPMTPage;
   	    if (ulLPNMaxCnt < 64)
   	    {
            ulLPNMaxCnt += 256;
   	    }
    }
    else
    {
        ulLPNMaxCnt = PMTCntPerSwapSRAM;
    }
    if ((LPN_CNT_PER_PMTPAGE-ulLPNInPMTPage) < ulLPNMaxCnt)
   	{
        ulLPNMaxCnt = (LPN_CNT_PER_PMTPAGE - ulLPNInPMTPage);
   	}
    g_TrimRAMInfo[0].LPNInPMTPageEnd = ulLPNInPMTPage + ulLPNMaxCnt;
    if (g_TrimRAMInfo[0].LPNInPMTPageEnd > LPN_CNT_PER_PMTPAGE)
   	{
        DBG_Printf("err0 = %d ( should < %d)\n", g_TrimRAMInfo[0].LPNInPMTPageEnd, LPN_CNT_PER_PMTPAGE);
        DBG_Getch();
    }
    
    ulLenInByte = ((g_ucPMTItemBitCnt*(ulLPNMaxCnt + 1)) + 7) >> 3;
    ulLenInByte = (ulLenInByte + 15) >> 4 << 4;
  	if (g_ucPMTItemBitCnt == 32)
  	{
        ulSrcAddr = (U32)&pPMTPage[ulLPNInPMTPage];
  	}
  	else
  	{
      	if (ulLPNInPMTPage)
      	{
            ulBitTotal = (ulLPNInPMTPage + 1)*g_ucPMTItemBitCnt;
            ulSrcAddr = (U32)&pPMTPage[(ulBitTotal / 32) - 1];
      	}
      	else
      	{
            ulSrcAddr = (U32)&pPMTPage[0];
      	}
    }
    ulDesAddr = g_PMTSRAMAddr;
    g_TrimRAMInfo[0].PMTSRAMStartAddr = ulDesAddr;
    if (ulSrcAddr & 0x0F)
    {
        g_TrimRAMInfo[0].PMTSRAMStartAddr += (ulSrcAddr & 0x0F);
        ulSrcAddr &= ~0x0F;
        ulLenInByte += 16;
        // if g_ucPMTItemBitCnt == 32 & ulLPNMaxCnt = PMTCntPerSwapSRAM
        // we use ulLenInByte = 512*4byte = 2048 
        // if ulSrcAddr not alignmnet -> ulLenInByte = 2048+16 --> error !!!!
    }
    if (ulDesAddr & 0x0F)
    {
        DBG_Printf("SRAM address not alignment 0x%x !!\n", ulDesAddr);
        DBG_Getch();
    }
    if (ulLenInByte&0x0F)
    {
        DBG_Printf("0 ulLenInByte %d !!\n", ulLenInByte);
        DBG_Getch();
    }

    //DBG_Printf("Init(%d) 0x%x 0x%x(0x%x) %d !!\n",ulLPNInPMTPage,ulSrcAddr,ulDesAddr,g_usPMTSRAMStartAddr[0],ulLenInByte);
#ifdef DMAWAIT
    HAL_DMAECopyOneBlock(ulDesAddr, ulSrcAddr, ulLenInByte);
#else
    HAL_DMAECopyOneBlockLenLimit_noWait(ulDesAddr, ulSrcAddr, ulLenInByte, (U8)HAL_GetMcuId());
#endif
    g_ucPMTSwapID = 0;
    if ((ulDesAddr + ulLenInByte) > (g_PMTSRAMAddr + PMTSRAMSwapSize))
    {
        DBG_Printf("init : out of memory\n");
        DBG_Printf("ulDesAddr = 0x%x, ulLenInByte = %d ,ulLPNMaxCnt = %d , g_PMTSRAMAddr = 0x%x\n", ulDesAddr, ulLenInByte, ulLPNMaxCnt, g_PMTSRAMAddr);
        DBG_Getch();
    }
}

void L2_PMTSRAMSwap(U8 ucPu, U16 usPMTIIndex, U32 ulLPNInPMTPage)
{
    U32 ulDesAddr, ulSrcAddr, ulLenInByte, ulLPNMaxCnt, ulBitTotal;
    U32 ulOfs;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32 *pPMTPage = g_PMTManager->m_pPMTPage[ucPu][usPMTIIndex];
#else
    PMTPage *pPMTPage = g_PMTManager->m_pPMTPage[ucPu][usPMTIIndex];
#endif

    //ulOfs = (g_ucPMTSwapID+1)&0x01;
    ulOfs = g_ucPMTSwapID;

    if (g_ucPMTItemBitCnt == 32)
    {
        ulSrcAddr = (U32)&pPMTPage[ulLPNInPMTPage];
    }
    else
    {
        if (ulLPNInPMTPage)
        {
            ulBitTotal = (ulLPNInPMTPage + 1)*g_ucPMTItemBitCnt;
            ulSrcAddr = (U32)&pPMTPage[(ulBitTotal / 32) - 1];
        }
        else
        {
            ulSrcAddr = (U32)&pPMTPage[0];
        }
    }
    g_TrimRAMInfo[ulOfs].LPNInPMTPageStart = ulLPNInPMTPage;
    g_TrimRAMInfo[ulOfs].LPNInPMTPageEnd = ulLPNInPMTPage + PMTCntPerSwapSRAM;
    if (g_TrimRAMInfo[ulOfs].LPNInPMTPageEnd > LPN_CNT_PER_PMTPAGE)
   	{
        g_TrimRAMInfo[ulOfs].LPNInPMTPageEnd = LPN_CNT_PER_PMTPAGE;
   	}
    ulLPNMaxCnt = g_TrimRAMInfo[ulOfs].LPNInPMTPageEnd - ulLPNInPMTPage;
    ulLenInByte = ((g_ucPMTItemBitCnt*(ulLPNMaxCnt + 1)) + 7) >> 3;
   	if (ulLenInByte&0x0F)
   	{
        ulLenInByte = (ulLenInByte + 15) >> 4 << 4;
   	}

    ulDesAddr = g_PMTSRAMAddr + (ulOfs*PMTSRAMSwapSize);
    g_TrimRAMInfo[ulOfs].PMTSRAMStartAddr = ulDesAddr;
    if (ulSrcAddr & 0x0F)
    {
        g_TrimRAMInfo[ulOfs].PMTSRAMStartAddr += (ulSrcAddr & 0x0F);
        ulSrcAddr &= ~0x0F;
        ulLenInByte += 16;
    }
    if (ulDesAddr & 0x0F)
    {
        DBG_Printf("1 SRAM address not alignment 0x%x !!\n", ulDesAddr);
        DBG_Getch();
    }
    if (ulLenInByte&0x0F)
    {
        DBG_Printf("1 ulLenInByte %d !!\n", ulLenInByte);
        DBG_Getch();
    }
    //DBG_Printf("1 0x%x 0x%x %d!!\n",ulSrcAddr,ulDesAddr,ulLenInByte);
#ifdef DMAWAIT
    HAL_DMAECopyOneBlock(ulDesAddr, ulSrcAddr, ulLenInByte);
#else
    HAL_DMAECopyOneBlockLenLimit_noWait(ulDesAddr, ulSrcAddr, ulLenInByte, (U8)HAL_GetMcuId());
#endif
    g_TrimRAMInfo[g_ucPMTSwapID].ucPu = ucPu;
    //DBG_Printf("swap%d , pu = %d  0x%x (0x%x) -> 0x%x(0x%x) , Index = %d (%d ~ %d)\n",g_ucPMTSwapID,ucPu,(U32)pPMTPage,ulSrcAddr,g_TrimRAMInfo[ulOfs].PMTSRAMStartAddr,ulDesAddr,usPMTIIndex,g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageStart, g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageEnd);
    //DBG_Printf("swap : %d ~ %d\n",g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageStart, g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageEnd);
}

void L2_SetPMTInvalid(U8 ucPu, U16 usPMTIIndex, U32 ulStartLPN, U32 ulLPNTotal)
{
#ifdef PMT_ITEM_SIZE_REDUCE
    U32 *pPMTPage = g_PMTManager->m_pPMTPage[ucPu][usPMTIIndex];
#else
    PMTPage *pPMTPage = g_PMTManager->m_pPMTPage[ucPu][usPMTIIndex];
#endif
    U32 ulTotalCnt;
    U32 ulBitTotal, ulMask, ulTemp, i;
    U32 ulDesAddr, ulLenInByte;

    ulDesAddr = (U32)&pPMTPage[0];
    ulTotalCnt = ulLPNTotal*g_ucPMTItemBitCnt;
    
    //DBG_Printf("L2_SetPMTInvalid : %d : %d ~ %d (0x%x)\n",usPMTIIndex,ulStartLPN,(ulStartLPN+ulLPNTotal),(U32)pPMTPage);
    if (g_ucPMTItemBitCnt == 32)
    {
        ulDesAddr = (U32)&pPMTPage[ulStartLPN];
        ulLenInByte = ulTotalCnt / 8;
    }
    else
    {
        ulBitTotal = (ulStartLPN + ulLPNTotal)*g_ucPMTItemBitCnt;
        ulTemp = ulBitTotal % 32;
        if (ulTemp == 0)
        {
            ulMask = 0xFFFFFFFF;
            ulTotalCnt -= 32;
            pPMTPage[(ulBitTotal / 32) - 1] |= ulMask;
        }
        else
        {
            ulMask = (1 << ulTemp) - 1;
            pPMTPage[(ulBitTotal / 32)] |= ulMask;
            ulTotalCnt -= ulTemp;
        }

        if (!ulTotalCnt)
        {
            return;
        }
        ulBitTotal = (ulStartLPN + 1)*g_ucPMTItemBitCnt;
        ulTemp = ulBitTotal % 32;
        if (ulTemp == 0)
        {
            ulMask = ~((1 << (32 - g_ucPMTItemBitCnt)) - 1);
            pPMTPage[(ulBitTotal / 32) - 1] = pPMTPage[ulBitTotal / 32 - 1] | ulMask;
            ulTotalCnt -= g_ucPMTItemBitCnt;
            ulDesAddr = (U32)&pPMTPage[(ulBitTotal / 32)];
        }
        else if (ulTemp >= g_ucPMTItemBitCnt)
        {
            ulMask = ~((1 << (ulTemp - g_ucPMTItemBitCnt)) - 1);
            pPMTPage[(ulBitTotal / 32)] |= ulMask;
            if (ulTotalCnt < (32-(ulTemp-g_ucPMTItemBitCnt)))
            {
                DBG_Printf("err1 %d %d %d\n", ulTotalCnt, g_ucPMTItemBitCnt, ulTemp);
                DBG_Getch();
            }
            ulTotalCnt -= (32 - (ulTemp - g_ucPMTItemBitCnt));
            ulDesAddr = (U32)&pPMTPage[(ulBitTotal / 32) + 1];
        }
        else
        {
            ulMask = ~((1 << (32 - (g_ucPMTItemBitCnt - ulTemp))) - 1);
            pPMTPage[(ulBitTotal / 32) - 1] |= ulMask;
            if (ulTotalCnt < (g_ucPMTItemBitCnt-ulTemp))
            {
                DBG_Printf("err2 %d %d %d\n", ulTotalCnt, g_ucPMTItemBitCnt, ulTemp);
                DBG_Getch();
            }
            ulTotalCnt -= (g_ucPMTItemBitCnt - ulTemp);
            ulDesAddr = (U32)&pPMTPage[(ulBitTotal / 32)];
        }

        if (ulTotalCnt&0x1F)
        {
            DBG_Printf("err3 %d %d %d %d\n", ulTotalCnt, ulStartLPN, g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageEnd, g_ucPMTItemBitCnt);
            DBG_Getch();
        }
        if (!ulTotalCnt)
        {
            return;
        }
        ulLenInByte = ulTotalCnt / 8;
    }
    
    if (ulLenInByte > 240)
    {
        if (ulDesAddr&0x0F)
        {
            ulTemp = 16 - (ulDesAddr & 0x0F);
            for (i = 0; i < ulTemp; i++)
            {
                *(U8 *)(ulDesAddr++) = 0xFF;
            }
            ulLenInByte -= ulTemp;
        }
        ulTemp = ulLenInByte & 0xFFFFFFF0;
#ifdef DMAWAIT
        HAL_DMAESetValue(ulDesAddr, ulTemp, 0xFFFFFFFF);
#else
        HAL_DMAESetValueLenLimit_nowait(ulDesAddr, ulTemp, 0xFFFFFFFF, (U8)HAL_GetMcuId());
#endif
        ulLenInByte = ulLenInByte & 0x0F;
        ulDesAddr += ulTemp;
    }
    if (ulLenInByte)
    {
        for (i = 0; i < ulLenInByte; i++)
        {
            *(U8 *)(ulDesAddr + i) = 0xFF;
        }
    }

}

#ifndef SIM
U32 CheckReadPinter(U8 index)
{
    U32 ShareAddr = (U32)g_pShareData;
    U32 ulAddr, ulTemp1 = 0, ulTemp2 = INVALID_8F;
/*
                while(1) 
                {
                    if (pTrimPhyAddr[sIndex].m_PPN == INVALID_8F)
                    {
                        break;
                    }
                }
*/
    asm ("10:\n\t \
        addi %1, %3, 12\n\t \
        addx4 %1, %0, %1\n\t \
        l32i %1, %1, 0\n\t \
        //addi %2, %2, 1\n\t \
        bne %1, %4, 10b\n\t"
        :"+a"(index),"+a"(ulAddr),"+a"(ulTemp1)
        :"a"(ShareAddr),"a"(ulTemp2)
        :"memory");

    return ulTemp1;
}

U32 CheckL3Status(U32 status)
{
    U32 ShareAddr = (U32)g_pShareData;
    U32 ulTemp, ulTemp1 = 0;
    asm ("10:\n\t \
        l8ui %1, %3, 1\n\t \
        //addi %2, %2, 1\n\t \
        bne %0, %1, 10b\n\t"
        :"+a"(status),"+a"(ulTemp),"+a"(ulTemp1)
        :"a"(ShareAddr)
        :"memory");

    return ulTemp1;
}
#endif
#define TrimSwapPreCnt 64
void L2_TrimReqProcess(U32 TrimStartLPN, U32 TrimLPNCount)
{
#ifndef L2_FAKE
    U32 ulLoop, ulTemp;
    U32 ulLoopEnd, ulTrimLPNEnd;
    U16 usPMTIIndexInPu = INVALID_4F;
    U16 usPMTIIndexNext;
    U32 ulLPNInPMTPage;
    BOOL bDone = FALSE;
    BOOL bChange = FALSE;
    U32 ulTrimOK = 0;
#ifdef PMT_ITEM_SIZE_REDUCE
    U32* pPMTPage;
#else
    PMTPage* pPMTPage;
#endif
    U32 ulStartLPN;
    U32 ulPageStart;
    U32 ulValue0, ulValue1;
    U32 ulPageOfs;
    U32 ulMask;
    PhysicalAddr *pTrimPhyAddr;
    PhysicalAddr TrimAddr;
    U16 *pPMTIIndexInPu;
    S8 sIndex;
#ifdef DBG_PMT
    U32 ulDebugLPN = TrimStartLPN;
#endif
#ifdef SIM
    LPN2PMTInfo PMTInfoBegin[12];
    LPN2PMTInfo PMTInfoEnd[12];
    U32 ulTrimCount[12];
#else
    LPN2PMTInfo PMTInfoBegin[SUBSYSTEM_SUPERPU_NUM];
    LPN2PMTInfo PMTInfoEnd[SUBSYSTEM_SUPERPU_NUM];
    U32 ulTrimCount[SUBSYSTEM_SUPERPU_NUM];
#endif
    U32 ulLPN;
    U8 ucPu;
    U8 ucPuCount;

    //DBG_Printf("Trim %d , len = %d\n", TrimStartLPN, TrimLPNCount);

    ulTrimLPNEnd = TrimStartLPN + TrimLPNCount;
    if (ulTrimLPNEnd >= MAX_LPN_IN_DISK)
    {
        ulTrimLPNEnd = MAX_LPN_IN_DISK;
    }
#ifdef NEW_TRIM
    if (TrimLPNCount <= LPN_PER_BUF)
#endif
    {
        for (ulLoop = TrimStartLPN; ulLoop < ulTrimLPNEnd; ulLoop++)
        {
            L2_TrimLPN(ulLoop);
        }
        return;
    }
    for (ulLoop = 0; ulLoop < SUBSYSTEM_SUPERPU_NUM; ulLoop++)
    {
        PMTInfoBegin[ulLoop].ucPu = 0xFF;
        PMTInfoEnd[ulLoop].ucPu = 0xFF;
    }

    ulLPN = TrimStartLPN;
    for (ulLoop = 0; ulLoop < SUBSYSTEM_SUPERPU_NUM; ulLoop++)
    {
        PMTInfoBegin[ulLoop].ucPu = L2_GetSuperPuFromLPN(ulLPN);
        PMTInfoBegin[ulLoop].usPMTIIndex = L2_GetPMTIIndexInPu(ulLPN);
        PMTInfoBegin[ulLoop].ulLPNInPMTPage = L2_GetOffsetInPMTPage(ulLPN);
        ulLPN = ulLPN >> LPN_PER_BUF_BITS;
        ulLPN = ulLPN + 1;
        ulLPN = ulLPN << LPN_PER_BUF_BITS;
        if (ulLPN >= ulTrimLPNEnd)
        {
            break;
        }
        //DBG_Printf("begin : ucPu = %d , usPMTIIndex = %d ulLPNInPMTPage = %d\n", PMTInfoBegin[ulLoop].ucPu, PMTInfoBegin[ulLoop].usPMTIIndex, PMTInfoBegin[ulLoop].ulLPNInPMTPage);
    }

    ulLPN = ulTrimLPNEnd - 1;
    for (ulLoop = 0; ulLoop < SUBSYSTEM_SUPERPU_NUM; ulLoop++)
    {
        ucPu = L2_GetSuperPuFromLPN(ulLPN);
        for (ulValue0 = 0; ulValue0 < SUBSYSTEM_SUPERPU_NUM; ulValue0++)
        {
            if (PMTInfoBegin[ulValue0].ucPu == 0xFF)
            {
                continue;
            }
            if (ucPu == PMTInfoBegin[ulValue0].ucPu)
            {
                break;
            }
        }

        PMTInfoEnd[ulValue0].ucPu = ucPu;
        PMTInfoEnd[ulValue0].usPMTIIndex = L2_GetPMTIIndexInPu(ulLPN);
        PMTInfoEnd[ulValue0].ulLPNInPMTPage = L2_GetOffsetInPMTPage(ulLPN);
        ulLPN = ulLPN >> LPN_PER_BUF_BITS;
        if (ulLPN == 0)
        {
            break;
        }
        ulLPN = (ulLPN << LPN_PER_BUF_BITS) - 1;
        //DBG_Printf("end%d : ucPu = %d , usPMTIIndex = %d ulLPNInPMTPage = %d\n", ulValue0, PMTInfoEnd[ulValue0].ucPu, PMTInfoEnd[ulValue0].usPMTIIndex, PMTInfoEnd[ulValue0].ulLPNInPMTPage);
    }

    ulTrimOK = 0;
    for (ulLoop = 0; ulLoop < SUBSYSTEM_SUPERPU_NUM; ulLoop++)
    {
        if (PMTInfoBegin[ulLoop].usPMTIIndex == PMTInfoEnd[ulLoop].usPMTIIndex)
        {
            ulTrimCount[ulLoop] = PMTInfoEnd[ulLoop].ulLPNInPMTPage - PMTInfoBegin[ulLoop].ulLPNInPMTPage + 1;
            //DBG_Printf("ulTrimCount = %d , ", ulTrimCount[ulLoop]);
            ulTrimOK += ulTrimCount[ulLoop];
            continue;
        }
        ulTrimCount[ulLoop] = (LPN_CNT_PER_PMTPAGE - PMTInfoBegin[ulLoop].ulLPNInPMTPage) + PMTInfoEnd[ulLoop].ulLPNInPMTPage + 1;
        ulTrimCount[ulLoop] += ((PMTInfoEnd[ulLoop].usPMTIIndex - PMTInfoBegin[ulLoop].usPMTIIndex - 1)*LPN_CNT_PER_PMTPAGE);
        ulTrimOK += ulTrimCount[ulLoop];
        //DBG_Printf("ulTrimCount = %d , ", ulTrimCount[ulLoop]);
    }
    //DBG_Printf(" total = %d %d\n", ulTrimOK, TrimLPNCount);
    if (ulTrimOK != TrimLPNCount)
    {
        DBG_Printf("error : TrimLPNCount = %d %d\n", ulTrimOK, TrimLPNCount);
        DBG_Getch();
    }
    ulTrimOK = 0;
    pTrimPhyAddr = (PhysicalAddr *)&g_pShareData->ulTrimPhyAddr[0];
    pPMTIIndexInPu = (U16 *)&g_pShareData->usPMTIIndexInPu[0];

    usPMTIIndexInPu = PMTInfoBegin[0].usPMTIIndex;
    ulLPNInPMTPage = PMTInfoBegin[0].ulLPNInPMTPage;
    ucPu = PMTInfoBegin[0].ucPu;

    L2_TrimDMAESRAMInit(ucPu, usPMTIIndexInPu, ulLPNInPMTPage);
    usPMTIIndexNext = usPMTIIndexInPu;

    bChange = FALSE;
    if ((PMTInfoBegin[0].usPMTIIndex == PMTInfoEnd[0].usPMTIIndex) && ((g_TrimRAMInfo[0].LPNInPMTPageEnd - g_TrimRAMInfo[0].LPNInPMTPageStart) >= ulTrimCount[0]))
    {
        bChange = TRUE;
    }

    if (bChange == FALSE)
    {
        if (g_TrimRAMInfo[0].LPNInPMTPageEnd >= LPN_CNT_PER_PMTPAGE)
        {
            usPMTIIndexNext++;
            g_ucPMTSwapID = 1;
            //DBG_Printf("initswap : %d %d \n", ucPu, usPMTIIndexNext);
            L2_PMTSRAMSwap(ucPu, usPMTIIndexNext, 0);
        }
        else
        {
            g_ucPMTSwapID = 1;
            //DBG_Printf("initswap : %d %d %d\n", ucPu, usPMTIIndexNext, g_TrimRAMInfo[0].LPNInPMTPageEnd);
            L2_PMTSRAMSwap(ucPu, usPMTIIndexNext, g_TrimRAMInfo[0].LPNInPMTPageEnd);
        }
        g_ucPMTSwapID = 0;
    }

    g_ucPMTSwapID = 0;
    bDone = FALSE;
    for (ulLoop = 0; ulLoop < SUBSYSTEM_SUPERPU_NUM; ulLoop++)
    {
        g_pShareData->ulDirtyBitMapAddr[ulLoop] = (U32)&g_PMTI[ulLoop]->m_PMTDirtyBitMapInCE.m_DirtyBitMap[0];
        g_pShareData->ulTrimLPNMap[ulLoop] = (U32)&pDPBM->m_LpnMap[ulLoop][0];
#ifndef ValidLPNCountSave_IN_DSRAM1
        g_pShareData->ulValidLPNCountSaveAddr[ulLoop] = (U32)g_PMTManager->m_PMTSpareBuffer[ulLoop][0];
#endif
    }
    g_pShareData->sWritePoint = -1;
    g_pShareData->sReadPoint = -1;
    g_pShareData->ulL2TrimTotal = 0;
    g_pShareData->ulL3TrimTotal = 0;
#ifdef DirtyLPNCnt_IN_DSRAM1
    g_pShareData->ulDirtyLPNCntAddr = (U32)g_pDirtyLPNCnt;
#endif
#ifdef ValidLPNCountSave_IN_DSRAM1
    g_pShareData->ulValidLPNCountSaveL_addr = (U32)g_pValidLPNCountSaveL;
    g_pShareData->ulValidLPNCountSaveH_addr = (U32)g_pValidLPNCountSaveH;
#endif
    pPMTIIndexInPu[0] = usPMTIIndexInPu;
    ucPuCount = 0;
    bChange = FALSE;
    bDone = FALSE;
    ulTrimLPNEnd = ulTrimCount[ucPuCount];
    ulStartLPN = ulLPNInPMTPage;

    g_pShareData->ucL2TrimStatus = Trim_L2_Start;
    g_pShareData->ucL3TrimStatus = Trim_L2_Start;
 
    while(1)
    {
        ulValue0 = CommSetEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_TrimL3);
        if (ulValue0 == COMM_EVENT_STATUS_SUCCESS_NOEVENT)
        {
            break;
        }
    }
    for (ulLoop = 0; ulLoop < TrimBufferSize; ulLoop++)
    {
        pTrimPhyAddr[ulLoop].m_PPN = INVALID_8F;
    }

    while(1)
    {
        ulLoopEnd = g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageEnd - ulLPNInPMTPage;
        if (ulTrimLPNEnd <= ulLoopEnd)
        {
            ulLoopEnd = ulTrimLPNEnd;
#ifdef IM_3D_TLC_1TB
            if ((ucPuCount + 1) == SUBSYSTEM_SUPERPU_NUM)
            {
                bDone = TRUE;
            }
            else
            {
                ucPuCount++;
                if (ucPuCount == SUBSYSTEM_SUPERPU_NUM)
                {
                    DBG_Printf("ucPuCount error %d %d\n", ucPuCount, SUBSYSTEM_SUPERPU_NUM);
                    DBG_Getch();
                }
                g_ucPMTSwapID = (g_ucPMTSwapID + 1) & 0x01;
                L2_PMTSRAMSwap(PMTInfoBegin[ucPuCount].ucPu, PMTInfoBegin[ucPuCount].usPMTIIndex, PMTInfoBegin[ucPuCount].ulLPNInPMTPage);
                g_ucPMTSwapID = (g_ucPMTSwapID + 1) & 0x01;
                bChange = TRUE;
            }
#else
            bDone = TRUE;
#endif
        }

        if (g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageStart)
        {
            ulPageStart = (((g_TrimRAMInfo[g_ucPMTSwapID].LPNInPMTPageStart + 1)*g_ucPMTItemBitCnt) / 32) - 1;
        }
        else
        {
            ulPageStart = 0;
        }
        pPMTPage = (U32 *)g_TrimRAMInfo[g_ucPMTSwapID].PMTSRAMStartAddr;
        ulMask = (1 << g_ucPMTItemBitCnt) - 1;
        for (ulLoop = 0; ulLoop < ulLoopEnd; ulLoop++)
        {
			ulPageOfs = (ulLPNInPMTPage + 1)*g_ucPMTItemBitCnt;
            ulTemp = ulPageOfs & 0x1F;
            ulPageOfs = ulPageOfs / 32 - ulPageStart;
            if (ulTemp == 0)
            {
                ulValue1 = pPMTPage[ulPageOfs - 1];
                TrimAddr.m_PPN = (ulValue1 >> (32 - g_ucPMTItemBitCnt))&ulMask;
            }
            else if (ulTemp < g_ucPMTItemBitCnt)
            {
                ulValue1 = pPMTPage[ulPageOfs];
                ulTemp = g_ucPMTItemBitCnt - ulTemp;
                ulValue0 = pPMTPage[ulPageOfs - 1] >> (32 - ulTemp);
                ulValue1 = ulValue1 << ulTemp;
                TrimAddr.m_PPN = (ulValue0 | ulValue1) & ulMask;
            }
            else
            {
                ulValue1 = pPMTPage[ulPageOfs];
                TrimAddr.m_PPN = (ulValue1 >> (ulTemp - g_ucPMTItemBitCnt))&ulMask;
            }
#ifdef DBG_PMT
            if (TrimAddr.m_PPN == ulMask)
            {
                TrimAddr.m_PPN = INVALID_8F;
            }
#ifdef IM_3D_TLC_1TB
            ulDebugLPN = (usPMTIIndexInPu*LPN_CNT_PER_PMTPAGE)+ulLPNInPMTPage;
            ulValue0 = ulDebugLPN&((1<<LPN_PER_BUF_BITS)-1);
            ulDebugLPN = ((ulDebugLPN-ulValue0)*SUBSYSTEM_SUPERPU_NUM)+ulValue0;
            ulDebugLPN += (ucPu*LPN_PER_BUF);
#endif
            if (INVALID_8F != g_DebugPMT[ulDebugLPN].m_PPN)
            {
                if (g_DebugPMT[ulDebugLPN].m_PPN != TrimAddr.m_PPN)
                {
                    if (FALSE == L2_IsNeedIgnoreCheck(ulDebugLPN, &TrimAddr))
                    {
						DBG_Printf("lpn:0x%x, PMT:0x%x, DBG_PMT:0x%x\n", ulDebugLPN, TrimAddr.m_PPN, g_DebugPMT[ulDebugLPN].m_PPN);
                        DBG_Getch();
                    }
                }
            }
            g_DebugPMT[ulDebugLPN].m_PPN = INVALID_8F;
            ulDebugLPN++;
            if (TrimAddr.m_PPN != INVALID_8F)
#else
            if (TrimAddr.m_PPN != ulMask)
#endif
			{
                sIndex = (g_pShareData->sWritePoint + 1) % TrimBufferSize;
                g_pShareData->sWritePoint = sIndex;
#ifdef SIM
                while (1)
                {
                    if (g_pShareData->ulTrimPhyAddr[sIndex] == INVALID_8F)
                    {
                        break;
                    }
                }
#else
                CheckReadPinter(sIndex);
#endif
                pPMTIIndexInPu[sIndex] = usPMTIIndexInPu;
#ifdef IM_3D_TLC_1TB
                g_pShareData->ucPu[sIndex] = ucPu;
#endif
                //DBG_Printf("L2 : w%d , r%d , status %d , usPMTIIndexInPu = %d PPN : 0x%x -> ", g_pShareData->sWritePoint, g_pShareData->sReadPoint, g_pShareData->ucL2TrimStatus, usPMTIIndexInPu, pTrimPhyAddr[sIndex].m_PPN);
                pTrimPhyAddr[sIndex].m_PPN = TrimAddr.m_PPN;
                //DBG_Printf(" 0x%x\n", pTrimPhyAddr[sIndex].m_PPN);
                g_pShareData->ulL2TrimTotal++;
                g_pShareData->ucL2TrimStatus = Trim_L3_Run;
            }
            ulLPNInPMTPage++;
        }

        ulTrimOK += ulLoopEnd;
        if (bDone == TRUE)
        {
            g_pShareData->ucL2TrimStatus = Trim_L2_Finish;
            //DBG_Printf("L2 Trim_L2_Finish %d , 0x%x = 0x%x\n", sIndex, (U32)&pTrimPhyAddr[sIndex], pTrimPhyAddr[sIndex].m_PPN);
            L2_SetPMTInvalid(ucPu, usPMTIIndexInPu, ulStartLPN, ulTrimOK);
#ifdef SIM
            while(1)
            {
                if (g_pShareData->ucL3TrimStatus == Trim_L3_Finish)
                {
                    break;
                }
            }
#else
            CheckL3Status(Trim_L3_Finish);
#endif
            g_pShareData->ucL2TrimStatus = Trim_Finish;
#ifdef SIM
			while (1)
			{
				if (g_pShareData->ucL3TrimStatus == Trim_Finish)
				{
					break;
				}
			}
#else
			CheckL3Status(Trim_Finish);
#endif
            /*
            if (g_pShareData->ulL2TrimTotal != g_pShareData->ulL3TrimTotal)
            {
                DBG_Printf("L2 : error : Trim total not mach %d %d , w%d , r%d, status %d %d\n", g_pShareData->ulL2TrimTotal, g_pShareData->ulL3TrimTotal, g_pShareData->sWritePoint, g_pShareData->sReadPoint, g_pShareData->ucL2TrimStatus, g_pShareData->ucL3TrimStatus);
                DBG_Printf("L2 : 0x%x 0x%x 0x%x 0x%x , 0x%x 0x%x 0x%x 0x%x ", pTrimPhyAddr[0].m_PPN, pTrimPhyAddr[1].m_PPN, pTrimPhyAddr[2].m_PPN, pTrimPhyAddr[3].m_PPN, pTrimPhyAddr[4].m_PPN, pTrimPhyAddr[5].m_PPN, pTrimPhyAddr[6].m_PPN, pTrimPhyAddr[7].m_PPN);
                DBG_Printf(", 0x%x 0x%x 0x%x 0x%x , 0x%x 0x%x 0x%x 0x%x\n", pTrimPhyAddr[8].m_PPN, pTrimPhyAddr[9].m_PPN, pTrimPhyAddr[10].m_PPN, pTrimPhyAddr[11].m_PPN, pTrimPhyAddr[12].m_PPN, pTrimPhyAddr[13].m_PPN, pTrimPhyAddr[14].m_PPN, pTrimPhyAddr[15].m_PPN);
                DBG_Getch();
            }
            DBG_Printf("L2 : return : %d %d %d\n", g_pShareData->ulL2TrimTotal, g_pShareData->ulL3TrimTotal);
            */
            //if no cache , trim will be fail because cache coherence
#ifdef DCACHE
            HAL_InvalidateDCache();
#endif
            return;
        }
#ifdef IM_3D_TLC_1TB
        if (bChange == TRUE)
        {
            L2_SetPMTInvalid(ucPu, usPMTIIndexInPu, ulStartLPN, ulTrimOK);
            //DBG_Printf("swap sPu%d -> sPu%d ", ucPu, PMTInfoBegin[ucPuCount].ucPu);
            ulTrimLPNEnd = ulTrimCount[ucPuCount];
            //DBG_Printf(" , cout = %d\n", ulTrimLPNEnd);
            ulStartLPN = ulLPNInPMTPage = PMTInfoBegin[ucPuCount].ulLPNInPMTPage;
            usPMTIIndexInPu = usPMTIIndexNext = PMTInfoBegin[ucPuCount].usPMTIIndex;
            ucPu = PMTInfoBegin[ucPuCount].ucPu;
            ulTrimOK = 0;
            bChange = FALSE;
        }
        else
#endif
        {
            ulTrimLPNEnd -= ulLoopEnd;
        }
        if (ulLPNInPMTPage >= LPN_CNT_PER_PMTPAGE)
        {
            //L2_SetPMTInvalid(ucPu, usPMTIIndexInPu, ulStartLPN, (LPN_CNT_PER_PMTPAGE - ulStartLPN));
            L2_SetPMTInvalid(ucPu, usPMTIIndexInPu, ulStartLPN, ulTrimOK);
            ulStartLPN = 0;
            ulLPNInPMTPage = 0;
            usPMTIIndexInPu = usPMTIIndexNext;
			ulTrimOK = 0;
        }

        g_pShareData->ucL2TrimStatus = Trim_L3_Wait_L2;
        if (g_TrimRAMInfo[(g_ucPMTSwapID + 1) & 0x01].LPNInPMTPageEnd >= LPN_CNT_PER_PMTPAGE)
        {
            usPMTIIndexNext = usPMTIIndexInPu;
            usPMTIIndexNext++;
            while(1)
            {
                if (SUBSYSTEM_STATUS_PENDING == g_PMTManager->m_FlushStatus[ucPu][0][usPMTIIndexNext])
                {
                    usPMTIIndexNext++;
                    continue;
                }
                else
                {
                    break;
                }
            }
            L2_PMTSRAMSwap(ucPu, usPMTIIndexNext, 0);
        }
        else
        {
            L2_PMTSRAMSwap(ucPu, usPMTIIndexInPu, g_TrimRAMInfo[(g_ucPMTSwapID + 1) & 0x01].LPNInPMTPageEnd);
        }
        g_ucPMTSwapID = (g_ucPMTSwapID + 1) & 0x01;
    }
#else
    L2_RamdiskTrimProcess(TrimStartLPN, TrimLPNCount);
#endif

}

