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
Filename    :L2_PMTPage.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.7
Description :functions about PMTPage
Others      :
Modify      :
*******************************************************************************/
#include "L2_PMTPage.h"
#include "L2_Erase.h"

GLOBAL  PhysicalAddr* g_DebugPMT;

/****************************************************************************
Name        :L2_UpdateDebugPMT
Input       :PhysicalAddr* pAddr, U32 LPN
Output      :void
Author      :HenryLuo
Date        :2012.09.10    18:34:08
Description :record Addr in debug PMT.
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L2_UpdateDebugPMT(PhysicalAddr* pAddr, U32 LPN)
{
    g_DebugPMT[LPN].m_PPN = pAddr->m_PPN;
    return;
}

void MCU1_DRAM_TEXT L2_InitDebugPMT(void)
{
    U32 ulLpn;

    for (ulLpn = 0; ulLpn < MAX_LPN_IN_DISK; ulLpn++)
    {
        g_DebugPMT[ulLpn].m_PPN = INVALID_8F;
    }
}

void MCU1_DRAM_TEXT L2_LookupDebugPMT(PhysicalAddr* pAddr, U32 LPN)
{
    pAddr->m_PPN = g_DebugPMT[LPN].m_PPN;
}

void L2_IncreaseOffsetInAddr(PhysicalAddr* pAddr)
{
    pAddr->m_LPNInPage++;
}

#ifdef PMT_ITEM_SIZE_3BYTE
/*****************************************************************************
 Prototype      : L2_PhyAddrToPMTItem
 Description    :
 Input          : PhysicalAddr* pPhyAddr
 PMTItem* pPMTItem
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/10
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_PhyAddrToPMTItem(PhysicalAddr* pPhyAddr, PMTItem* pPMTItem)
{
#if (((BLK_PER_PLN + RSV_BLK_PER_PLN) > 1024) && (LOGIC_PG_PER_BLK_BITS <= 8))
    pPMTItem->m_BlockInPuLow = (U8)(pPhyAddr->m_BlockInPU & 0xFF);
    pPMTItem->m_BlockInPuHigh = (U8)(pPhyAddr->m_BlockInPU >> 8);
   
    #if (LOGIC_PIPE_PG_SZ == (32*1024))
        pPMTItem->m_PageInBlockLow = (U8)(pPhyAddr->m_PageInBlock & 0xF);
        pPMTItem->m_PageInBlockHigh = (U8)(pPhyAddr->m_PageInBlock >> 4);
    #elif (LOGIC_PIPE_PG_SZ == (64*1024))
        pPMTItem->m_PageInBlockLow = (U8)(pPhyAddr->m_PageInBlock & 0x1F);
        pPMTItem->m_PageInBlockHigh = (U8)(pPhyAddr->m_PageInBlock >> 5);
    #endif

#else
    pPMTItem->m_BlockInPuLow = (U8)(pPhyAddr->m_BlockInPU & 0xFF);
    pPMTItem->m_BlockInPuHigh = (U8)(pPhyAddr->m_BlockInPU >> 8);
    pPMTItem->m_PageInBlockLow = (U8)(pPhyAddr->m_PageInBlock & 0x1F);
    pPMTItem->m_PageInBlockHigh = (U8)(pPhyAddr->m_PageInBlock >> 5);
#endif
    pPMTItem->m_OffsetInSuperPage = pPhyAddr->m_OffsetInSuperPage;

    pPMTItem->m_LPNInPage = pPhyAddr->m_LPNInPage;

    return;
}

/*****************************************************************************
 Prototype      : L2_PMTItemToPhyAddr
 Description    :
 Input          : PhysicalAddr* pPhyAddr
 PMTItem* pPMTItem
 Output         : None
 Return Value   : void
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/11/10
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
void L2_PMTItemToPhyAddr(PhysicalAddr* pPhyAddr, PMTItem* pPMTItem)
{
#if (((BLK_PER_PLN + RSV_BLK_PER_PLN) > 1024) && (LOGIC_PG_PER_BLK_BITS <= 8))
        #if (LOGIC_PIPE_PG_SZ == (32*1024))
        pPhyAddr->m_PUSer = INVALID_2F;
        pPhyAddr->m_BlockInPU = ((U16)(pPMTItem->m_BlockInPuHigh) << 8) | pPMTItem->m_BlockInPuLow;
        pPhyAddr->m_PageInBlock = ((U16)(pPMTItem->m_PageInBlockHigh) << 4) | pPMTItem->m_PageInBlockLow;
        #elif (LOGIC_PIPE_PG_SZ == (64*1024))
        pPhyAddr->m_PUSer = INVALID_2F;
        pPhyAddr->m_BlockInPU = ((U16)(pPMTItem->m_BlockInPuHigh) << 8) | pPMTItem->m_BlockInPuLow;
        pPhyAddr->m_PageInBlock = ((U16)(pPMTItem->m_PageInBlockHigh) << 5) | pPMTItem->m_PageInBlockLow;
        #endif
#else
    pPhyAddr->m_PUSer = INVALID_2F;
    pPhyAddr->m_BlockInPU = ((U16)(pPMTItem->m_BlockInPuHigh) << 8) | pPMTItem->m_BlockInPuLow;
    pPhyAddr->m_PageInBlock = ((U16)(pPMTItem->m_PageInBlockHigh) << 5) | pPMTItem->m_PageInBlockLow;
#endif
    pPhyAddr->m_OffsetInSuperPage = pPMTItem->m_OffsetInSuperPage;
    pPhyAddr->m_LPNInPage = pPMTItem->m_LPNInPage;

    if (pPhyAddr->m_PPN != INVALID_8F)
    {
        pPhyAddr->m_PUSer = SUBSYSTEM_SUPERPU_NUM;
    }

    return;
}
#endif



#ifdef PMT_ITEM_SIZE_REDUCE
//PMTItem Reduction Strategy needed
#define INTEL_3D_TLC_VBCnt_MAX512

GLOBAL U32 g_ulLPNCntPerPMTPage;
GLOBAL U32 g_ulPMTPageCntPerSPU;
GLOBAL U8  g_ucPUserBit;
GLOBAL U8  g_ucOffsetInSuperPageBit;
GLOBAL U8  g_ucPMTItemBitCnt;

//[Zac Gao] define the following function for calculating BitCnt
U8 MCU1_DRAM_TEXT L2_Num2Bit_Switch(U8 ucNum)
{
	U8 ucRet;

	U8 i;
	if (0 == ucNum)
	{
		DBG_Printf("wrong input : PuNum or LunNum = 0");
		DBG_Getch();
	}
	else
	{
		ucNum--;
		for (i = 0; i < 8; i++)
		{
			if (0 != ((1 << (7 - i)) & ucNum))
			{
				break;
			}
		}
		ucRet = 8 - i;
	}

	return ucRet;
}

void MCU1_DRAM_TEXT L2_PMTBitInit(U8 ucPUserNum, U8 ucLunNum, U8 ucBlockInPUBit, U8 ucPageinBlockBit, U8 ucLPNInPageBit)
{
    U16 EraseStructNeedSizeBit;

	//printf("ucPUserNum:%d, ucLunNum:%d\n", ucPUserNum, ucLunNum);
	g_ucPUserBit = L2_Num2Bit_Switch(ucPUserNum);
	g_ucOffsetInSuperPageBit = L2_Num2Bit_Switch(ucLunNum);

	g_ucPMTItemBitCnt = g_ucPUserBit + g_ucOffsetInSuperPageBit + ucBlockInPUBit + ucPageinBlockBit + ucLPNInPageBit;
    if (g_ucPMTItemBitCnt < PMT_ITEM_SIZE_MIN)
    {
        DBG_Printf("ERROR! Don't support PMTItemBitCnt < 26 bit, PMTItemBitCnt = %d\n", g_ucPMTItemBitCnt);
        DBG_Getch();
    }

	//g_ulLPNCntPerPMTPage = (PMT_PAGE_SIZE * 8 / g_ucPMTItemBitCnt) & (~ 0x03); //ensure  (g_ulLPNCntPerPMTPage / 4) is a intrger, because the distribution problem about 2/4Bit space
	//g_ulPMTPageCntPerSPU = (LPN_PER_SUPERPU % LPN_CNT_PER_PMTPAGE) ? (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE + 1) : (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE);
	//printf("l_PUserBit:%d, l_OffsetInSuperPageBit:%d, g_ulLPNCntPerPMTPage:%d, g_ulPMTPageCntPerSPU: %d\n", g_ucPUserBit, g_ucOffsetInSuperPageBit, g_ulLPNCntPerPMTPage, g_ulPMTPageCntPerSPU);

    EraseStructNeedSizeBit = ((sizeof(RecordEraseInfo)* 8 / g_ucPMTItemBitCnt) + 1) * g_ucPMTItemBitCnt;
    g_ulLPNCntPerPMTPage = ((PMT_PAGE_SIZE * 8 - EraseStructNeedSizeBit) / g_ucPMTItemBitCnt) & (~ 0x03);
    g_ulPMTPageCntPerSPU = (LPN_PER_SUPERPU % LPN_CNT_PER_PMTPAGE) ? (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE + 1) : (LPN_PER_SUPERPU / LPN_CNT_PER_PMTPAGE);
    DBG_Printf("EraseStructNeedSizeBit %d\n", EraseStructNeedSizeBit);
    DBG_Printf("LPN_PER_SUPERPU %d, LPN_CNT_PER_PMTPAGE %d, \n", LPN_PER_SUPERPU, LPN_CNT_PER_PMTPAGE);
    DBG_Printf("g_ucPMTItemBitCnt %d, g_ulLPNCntPerPMTPage %d, g_ulPMTPageCntPerSPU %d\n", g_ucPMTItemBitCnt, g_ulLPNCntPerPMTPage, g_ulPMTPageCntPerSPU);
}

#if 1
void L2_PhyAddrToPMTItem(PhysicalAddr *pPhyAddr, U32*pPMTPage, U32 LPNInPMTPage)
{
    U32 ulValue0, ulValue1;
    U32 ulBitTotal;
    U32 ulMask, ulTemp, ulOfs;

    if (g_ucPMTItemBitCnt == 32)
    {
        pPMTPage[LPNInPMTPage] = pPhyAddr->m_PPN;
    }
    else
    {
        ulBitTotal = (LPNInPMTPage + 1)*g_ucPMTItemBitCnt;
        ulTemp = ulBitTotal & 0x1F;
        ulMask = (1 << g_ucPMTItemBitCnt) - 1;
        if (pPhyAddr->m_PPN != INVALID_8F)
        {
            ulValue0 = pPhyAddr->m_PPN;
        }
        else
        {
            ulValue0 = (1 << g_ucPMTItemBitCnt) - 1;
        }
        if (ulTemp == 0)
        {
            ulTemp = 32 - g_ucPMTItemBitCnt;
            ulValue1 = ulValue0 << ulTemp;
            ulMask = (1 << ulTemp) - 1;
            pPMTPage[(ulBitTotal / 32) - 1] = (pPMTPage[ulBitTotal / 32 - 1] & ulMask) | ulValue1;
        }
        else if (ulTemp < g_ucPMTItemBitCnt)
        {
            ulOfs = (g_ucPMTItemBitCnt - ulTemp);
            ulMask = (1 << ulOfs) - 1;
            ulValue1 = ulValue0 & ulMask;
            ulOfs = (32 - ulOfs);
            ulValue1 = ulValue1 << ulOfs;
            ulMask = (1 << ulOfs) - 1;
            pPMTPage[(ulBitTotal / 32) - 1] = (pPMTPage[(ulBitTotal / 32) - 1] & ulMask) | ulValue1;

            ulOfs = (g_ucPMTItemBitCnt - ulTemp);
            ulValue1 = ulValue0 >> ulOfs;
            ulMask = ~((1 << ulTemp) - 1);
            pPMTPage[ulBitTotal / 32] = (pPMTPage[ulBitTotal / 32] & ulMask) | ulValue1;
        }
        else
        {
            ulMask = ~(ulMask << (ulTemp - g_ucPMTItemBitCnt));
            ulValue0 = ulValue0 << (ulTemp - g_ucPMTItemBitCnt);
            ulValue1 = pPMTPage[ulBitTotal / 32];
            ulValue1 = (ulValue1 & ulMask) | ulValue0;
            pPMTPage[ulBitTotal / 32] = ulValue1;
        }
    }
}
#else
/* 2D TLC */
void L2_PhyAddrToPMTItem(PhysicalAddr *pPhyAddr, U32*pPMTPage, U32 LPNInPMTPage)
{
	PMTItem32Bit *pPMTItem32Bit = (PMTItem32Bit *)pPMTPage;
	PMTItem24Bit *pPMTItem24Bit = (PMTItem24Bit *)pPMTPage;
	PMTItem4Bit *pPMTItem4Bit = (PMTItem4Bit *)((PMTItem24Bit *)pPMTPage + g_ulLPNCntPerPMTPage);
	PMTItem2Bit *pPMTItem2Bit = (PMTItem2Bit *)((PMTItem24Bit *)pPMTPage + g_ulLPNCntPerPMTPage);
	PMTItem1Bit *pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem2Bit + (g_ulLPNCntPerPMTPage >> 2));

	pPMTItem24Bit[LPNInPMTPage].m_PMTItem[0] = pPhyAddr->m_BytePPN[0];
	pPMTItem24Bit[LPNInPMTPage].m_PMTItem[1] = pPhyAddr->m_BytePPN[1];
	pPMTItem24Bit[LPNInPMTPage].m_PMTItem[2] = pPhyAddr->m_BytePPN[2];


#ifdef INTEL_3D_TLC_VBCnt_MAX512

	if ((g_ucPMTItemBitCnt == 26) || (g_ucPMTItemBitCnt == 27))
	{
		U8 Bit2 = 0;
        if (g_ucOffsetInSuperPageBit == 0)
            Bit2 = pPhyAddr->m_PUSer & 0x3;
		else if (g_ucOffsetInSuperPageBit == 1)
            Bit2 = (pPhyAddr->m_OffsetInSuperPage & 0x1) | ((pPhyAddr->m_PUSer & 0x1) << 1);
        else if (g_ucOffsetInSuperPageBit >= 2)
			Bit2 = pPhyAddr->m_OffsetInSuperPage & 0x3;

		switch (LPNInPMTPage % 4)
		{
		case 0:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0 = Bit2; break;
		case 1:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1 = Bit2; break;
		case 2:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2 = Bit2; break;
		case 3:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3 = Bit2; break;
		}
	}

	if (g_ucPMTItemBitCnt == 27)
	{
		/**
		* l_PUserBit              0 1 2 3 (4)
		* l_OffsetInSuperPageBit  4 3 2 1 (0)
		*/
		U8 Bit1 = 0;
        if (g_ucOffsetInSuperPageBit == 0)
            Bit1 = (pPhyAddr->m_PUSer & 0x4) >> 2;
        else if(g_ucOffsetInSuperPageBit == 1)
			Bit1 = (pPhyAddr->m_PUSer & 0x2) >> 1;
		else if (g_ucOffsetInSuperPageBit == 2)
			Bit1 = pPhyAddr->m_PUSer;
		else if (g_ucOffsetInSuperPageBit == 3)
            Bit1 = (pPhyAddr->m_OffsetInSuperPage & 0x4) >> 2;
        else
        {
            DBG_Printf("L2_PhyAddrToPMTItem() g_ucPMTItemBitCnt ERROR!\n");
            DBG_Getch();
        }
		switch (LPNInPMTPage % 8)
		{
		case 0:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0 = Bit1; break;
		case 1:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1 = Bit1; break;
		case 2:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2 = Bit1; break;
		case 3:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3 = Bit1; break;
		case 4:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4 = Bit1; break;
		case 5:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5 = Bit1; break;
		case 6:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6 = Bit1; break;
		case 7:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7 = Bit1; break;
		}
	}

    if ((g_ucPMTItemBitCnt == 28) || (g_ucPMTItemBitCnt == 29) || (g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
    {
        /**
        * l_PUserBit              0 1 2 3 4 (5)
        * l_OffsetInSuperPageBit* 4 3 2 1 0
        * l_OffsetInSuperPageBit  5 4 3 2 1 (0)
        */
        U8 Bit4 = 0;
        if (g_ucOffsetInSuperPageBit == 1)
            Bit4 = (pPhyAddr->m_OffsetInSuperPage) | ((pPhyAddr->m_PUSer & 0x7) << 1);
        else if (g_ucOffsetInSuperPageBit == 2)
            Bit4 = (pPhyAddr->m_OffsetInSuperPage & 0x3) | ((pPhyAddr->m_PUSer & 0x3) << 2);
        else if (g_ucOffsetInSuperPageBit == 3)
            Bit4 = (pPhyAddr->m_OffsetInSuperPage & 0x7) | ((pPhyAddr->m_PUSer & 0x1) << 3);
        else if (g_ucOffsetInSuperPageBit > 3)
            Bit4 = pPhyAddr->m_OffsetInSuperPage;


        switch (LPNInPMTPage % 2)
        {
        case 0:pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem0 = Bit4; break;
        case 1:pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem1 = Bit4; break;
        }
        //printf("Bit4 = %d\n", Bit4);
    }
    if (g_ucPMTItemBitCnt == 29)
    {
        U8 Bit1 = 0;
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));

        if(g_ucOffsetInSuperPageBit == 2)
            Bit1 = (pPhyAddr->m_PUSer & 0x4) >> 2;
        else if (g_ucOffsetInSuperPageBit == 3)
            Bit1 = (pPhyAddr->m_PUSer & 0x2) >> 1;
        else if (g_ucOffsetInSuperPageBit == 4)
            Bit1 = pPhyAddr->m_PUSer & 0x1;
        else
        {
            DBG_Printf("L2_PhyAddrToPMTItem() g_ucPMTItemBitCnt ERROR!\n");
            DBG_Getch();
        }
        switch (LPNInPMTPage % 8)
        {
        case 0:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0 = Bit1; break;
        case 1:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1 = Bit1; break;
        case 2:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2 = Bit1; break;
        case 3:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3 = Bit1; break;
        case 4:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4 = Bit1; break;
        case 5:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5 = Bit1; break;
        case 6:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6 = Bit1; break;
        case 7:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7 = Bit1; break;
        }
    }

    if ((g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
    {        
        U8 Bit2 = 0;
        pPMTItem2Bit = (PMTItem2Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
        
        if(g_ucOffsetInSuperPageBit == 3)
            Bit2 = (pPhyAddr->m_PUSer & 0x6) >> 1;
        else if (g_ucOffsetInSuperPageBit == 4)
            Bit2 = (pPhyAddr->m_PUSer & 0x3);
        else
        {
            DBG_Printf("L2_PhyAddrToPMTItem() g_ucPMTItemBitCnt ERROR!\n");
            DBG_Getch();
        }
        switch (LPNInPMTPage % 4)
        {
        case 0:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0 = Bit2; break;
        case 1:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1 = Bit2; break;
        case 2:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2 = Bit2; break;
        case 3:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3 = Bit2; break;
        }
    }

    if (g_ucPMTItemBitCnt == 31)
    {
        U8 Bit1 = 0;
        PMTItem1Bit *pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem2Bit + (g_ulLPNCntPerPMTPage >> 2));
        if (g_ucOffsetInSuperPageBit == 4)
            Bit1 = (pPhyAddr->m_PUSer & 0x4) >> 2;
        else
        {
            DBG_Printf("L2_PhyAddrToPMTItem() g_ucPMTItemBitCnt ERROR!\n");
            DBG_Getch();
        }
        switch (LPNInPMTPage % 8)
        {
        case 0:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0 = Bit1; break;
        case 1:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1 = Bit1; break;
        case 2:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2 = Bit1; break;
        case 3:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3 = Bit1; break;
        case 4:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4 = Bit1; break;
        case 5:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5 = Bit1; break;
        case 6:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6 = Bit1; break;
        case 7:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7 = Bit1; break;
        }
    }
#else

	if (g_ucPMTItemBitCnt == 27) // now g_ucOffsetInSuperPageBit = 0
	{
		U8 Bit1 = 0;
		U8 Bit2 = 0;
		if (g_ucOffsetInSuperPageBit == 0)
		{
			Bit2 = (pPhyAddr->m_BytePPN[3] & 0x01) | ((pPhyAddr->m_PUSer & 0x1) << 1);
			Bit1 = (pPhyAddr->m_PUSer & 0x2) >> 1;
		}
		else if (g_ucOffsetInSuperPageBit == 1)
		{
			Bit2 = (pPhyAddr->m_BytePPN[3] & 0x01) | ((pPhyAddr->m_OffsetInSuperPage & 0x1) << 1);
			Bit1 = (pPhyAddr->m_PUSer & 0x1);
		}
		else if (g_ucOffsetInSuperPageBit == 2)
		{
			Bit2 = (pPhyAddr->m_BytePPN[3] & 0x01) | ((pPhyAddr->m_OffsetInSuperPage & 0x1) << 1);
			Bit1 = (pPhyAddr->m_OffsetInSuperPage & 0x2) >> 1;
		}

		switch (LPNInPMTPage % 4)
		{
		case 0:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0 = Bit2; break;
		case 1:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1 = Bit2; break;
		case 2:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2 = Bit2; break;
		case 3:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3 = Bit2; break;
		}

		switch (LPNInPMTPage % 8)
		{
		case 0:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0 = Bit1; break;
		case 1:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1 = Bit1; break;
		case 2:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2 = Bit1; break;
		case 3:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3 = Bit1; break;
		case 4:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4 = Bit1; break;
		case 5:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5 = Bit1; break;
		case 6:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6 = Bit1; break;
		case 7:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7 = Bit1; break;
		}
	}
	if ((g_ucPMTItemBitCnt == 28) || (g_ucPMTItemBitCnt == 29) || (g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
	{
		/**
		* l_PUserBit              0 1 2 3 4 (5)
		* l_OffsetInSuperPageBit* 4 3 2 1 0
		* l_OffsetInSuperPageBit  5 4 3 2 1 (0)
		*/
		U8 Bit4 = 0;
		if (g_ucOffsetInSuperPageBit == 0)
			Bit4 = ((pPhyAddr->m_BytePPN[3] & 0x01)) | ((pPhyAddr->m_PUSer & 0x7) << 1);
		else if (g_ucOffsetInSuperPageBit == 1)
			Bit4 = ((pPhyAddr->m_BytePPN[3] & 0x01)) | ((pPhyAddr->m_OffsetInSuperPage & 0x1) << 1) | ((pPhyAddr->m_PUSer & 0x3) << 2);
		else if (g_ucOffsetInSuperPageBit == 2)
			Bit4 = ((pPhyAddr->m_BytePPN[3] & 0x1)) | ((pPhyAddr->m_OffsetInSuperPage & 0x3) << 1) | ((pPhyAddr->m_PUSer & 0x1) << 3);
		else if (g_ucOffsetInSuperPageBit >= 3)
			Bit4 = ((pPhyAddr->m_BytePPN[3] & 0x1)) | ((pPhyAddr->m_OffsetInSuperPage & 0x7) << 1);

		switch (LPNInPMTPage % 2)
		{
		case 0:pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem0 = Bit4; break;
		case 1:pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem1 = Bit4; break;
		}
		//printf("Bit4 = %d\n", Bit4);
	}
	if (g_ucPMTItemBitCnt == 29)
	{
		U8 Bit1 = 0;
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
		if (g_ucOffsetInSuperPageBit == 1)
			Bit1 = (pPhyAddr->m_PUSer & 0x4) >> 2;
		else if (g_ucOffsetInSuperPageBit == 2)
			Bit1 = (pPhyAddr->m_PUSer & 0x2) >> 1;
		else if (g_ucOffsetInSuperPageBit == 3)
			Bit1 = (pPhyAddr->m_PUSer & 0x1);
		else if (g_ucOffsetInSuperPageBit == 4)
			Bit1 = (pPhyAddr->m_OffsetInSuperPage & 0x8) >>3;

		switch (LPNInPMTPage % 8)
		{
		case 0:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0 = Bit1; break;
		case 1:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1 = Bit1; break;
		case 2:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2 = Bit1; break;
		case 3:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3 = Bit1; break;
		case 4:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4 = Bit1; break;
		case 5:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5 = Bit1; break;
		case 6:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6 = Bit1; break;
		case 7:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7 = Bit1; break;
		}
	}

	if ((g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
	{
        U8 Bit2 = 0;
        pPMTItem2Bit = (PMTItem2Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
		if (g_ucOffsetInSuperPageBit == 2)
		{
			Bit2 = (pPhyAddr->m_PUSer & 0x6) >> 1;
		}
		else if (g_ucOffsetInSuperPageBit == 3)
		{
			Bit2 = (pPhyAddr->m_PUSer & 0x3);
		}
		else if (g_ucOffsetInSuperPageBit == 4)
		{
			Bit2 = ((pPhyAddr->m_OffsetInSuperPage & 0x8) >> 3) | ((pPhyAddr->m_PUSer & 0x1) << 1);
		}

		switch (LPNInPMTPage % 4)
		{
		case 0:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0 = Bit2; break;
		case 1:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1 = Bit2; break;
		case 2:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2 = Bit2; break;
		case 3:pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3 = Bit2; break;
		}
	}

	if (g_ucPMTItemBitCnt == 31)
	{
		U8 Bit1 = 0;
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem2Bit + (g_ulLPNCntPerPMTPage >> 2));
		if (g_ucOffsetInSuperPageBit == 3)
			Bit1 = (pPhyAddr->m_PUSer & 0x4) >> 2;
		else if (g_ucOffsetInSuperPageBit == 4)
			Bit1 = (pPhyAddr->m_PUSer & 0x2) >> 1;

		switch (LPNInPMTPage % 8)
		{
		case 0:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0 = Bit1; break;
		case 1:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1 = Bit1; break;
		case 2:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2 = Bit1; break;
		case 3:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3 = Bit1; break;
		case 4:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4 = Bit1; break;
		case 5:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5 = Bit1; break;
		case 6:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6 = Bit1; break;
		case 7:pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7 = Bit1; break;
		}
	}
#endif

	if (g_ucPMTItemBitCnt == 32)
	{
		pPMTItem32Bit[LPNInPMTPage].m_PMTItem = pPhyAddr->m_PPN;
	}
}
#endif

#if 1
void L2_PMTItemToPhyAddr(PhysicalAddr* pPhyAddr, U32*pPMTPage, U32 LPNInPMTPage)
{
    U32 ulValue0, ulValue1;
    U32 ulBitTotal;
    U32 ulMask, ulTemp, ulPPN;

    ulPPN = 0;

    if (g_ucPMTItemBitCnt == 32)
    {
        ulPPN = pPMTPage[LPNInPMTPage];
        ulMask = INVALID_8F;
    }
    else
    {
        ulBitTotal = (LPNInPMTPage + 1)*g_ucPMTItemBitCnt;
        ulMask = (1 << g_ucPMTItemBitCnt) - 1;
        ulTemp = ulBitTotal & 0x1F;
        if (ulTemp == 0)
        {
            ulValue1 = pPMTPage[(ulBitTotal / 32) - 1];
            ulPPN = (ulValue1 >> (32 - g_ucPMTItemBitCnt))&ulMask;
        }
        else if(ulTemp < g_ucPMTItemBitCnt)
        {
            ulValue1 = pPMTPage[(ulBitTotal / 32)];
            ulTemp = g_ucPMTItemBitCnt - ulTemp;
            ulValue0 = pPMTPage[(ulBitTotal / 32) - 1] >> (32 - ulTemp);
            ulValue1 = ulValue1 << ulTemp;
            ulPPN = (ulValue0 | ulValue1) & ulMask;
        }
        else 
        {
            ulValue1 = pPMTPage[(ulBitTotal / 32)];
            ulPPN = (ulValue1 >> (ulTemp - g_ucPMTItemBitCnt)) & ulMask;
        }
    }
    if (ulPPN == ulMask)
    {
        pPhyAddr->m_PPN = INVALID_8F;
    }
    else
    {
        pPhyAddr->m_PPN = ulPPN;
    }
}
#else
/* TLC */
void L2_PMTItemToPhyAddr(PhysicalAddr* pPhyAddr, U32*pPMTPage, U32 LPNInPMTPage)
{
    U8 Bit1 = 0;
    U8 Bit2 = 0;
    U8 Bit4 = 0;
    PMTItem32Bit *pPMTItem32Bit = (PMTItem32Bit *)pPMTPage;
    PMTItem24Bit *pPMTItem24Bit = (PMTItem24Bit *)pPMTPage;
    PMTItem4Bit *pPMTItem4Bit = (PMTItem4Bit *)((PMTItem24Bit *)pPMTPage + g_ulLPNCntPerPMTPage);
    PMTItem2Bit *pPMTItem2Bit = (PMTItem2Bit *)((PMTItem24Bit *)pPMTPage + g_ulLPNCntPerPMTPage);
    PMTItem1Bit *pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem2Bit + (g_ulLPNCntPerPMTPage >> 2));

    pPhyAddr->m_PPN = 0;

    pPhyAddr->m_BytePPN[0] = pPMTItem24Bit[LPNInPMTPage].m_PMTItem[0];
    pPhyAddr->m_BytePPN[1] = pPMTItem24Bit[LPNInPMTPage].m_PMTItem[1];
    pPhyAddr->m_BytePPN[2] = pPMTItem24Bit[LPNInPMTPage].m_PMTItem[2];


#ifdef INTEL_3D_TLC_VBCnt_MAX512

    if ((g_ucPMTItemBitCnt == 26) || (g_ucPMTItemBitCnt == 27))
    {
        switch (LPNInPMTPage % 4)
        {
        case 0:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0; break;
        case 1:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1; break;
        case 2:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2; break;
        case 3:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3; break;
        }
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit2 == 0x3) && (g_ucPMTItemBitCnt == 26))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {

            if (g_ucOffsetInSuperPageBit == 0)
            {
                pPhyAddr->m_PUSer = Bit2;
            }
            else if (g_ucOffsetInSuperPageBit == 1)
            {
                pPhyAddr->m_OffsetInSuperPage = Bit2 & 0x1;
                pPhyAddr->m_PUSer = (Bit2 & 0x2) >> 1;
            }
            else if (g_ucOffsetInSuperPageBit >= 2)
            {
                pPhyAddr->m_OffsetInSuperPage = Bit2;
            }

        }
    }

    if (g_ucPMTItemBitCnt == 27)
    {
        /**
        * l_PUserBit              0 1 2 3 (4)
        * l_OffsetInSuperPageBit  4 3 2 1 (0)
        */
        switch (LPNInPMTPage % 8)
        {
        case 0:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0; break;
        case 1:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1; break;
        case 2:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2; break;
        case 3:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3; break;
        case 4:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4; break;
        case 5:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5; break;
        case 6:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6; break;
        case 7:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7; break;
        }
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit2 == 0x3) && (Bit1 == 0x1))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 0)
                pPhyAddr->m_PUSer |= Bit1 << 2;
            else if (g_ucOffsetInSuperPageBit == 1)
                pPhyAddr->m_PUSer |= Bit1 << 1;
            else if (g_ucOffsetInSuperPageBit == 2)
                pPhyAddr->m_PUSer = Bit1;
            else if (g_ucOffsetInSuperPageBit == 3)
                pPhyAddr->m_OffsetInSuperPage |= Bit1 << 2;
            else
            {
                DBG_Printf("L2_PMTItemToPhyAddr() g_ucPMTItemBitCnt ERROR!\n");
                DBG_Getch();
            }
        }
    }

    if ((g_ucPMTItemBitCnt == 28) || (g_ucPMTItemBitCnt == 29) || (g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
    {
        /**
        * l_PUserBit              0 1 2 3 4 (5)
        * l_OffsetInSuperPageBit* 4 3 2 1 0
        * l_OffsetInSuperPageBit  5 4 3 2 1 (0)
        */
        switch (LPNInPMTPage % 2)
        {
        case 0: Bit4 = pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem0; break;
        case 1: Bit4 = pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem1; break;
        }
        //printf("Bit4 = %d\n", Bit4);
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (g_ucPMTItemBitCnt == 28))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 1)
            {
                pPhyAddr->m_OffsetInSuperPage = (Bit4 & 0x1);
                pPhyAddr->m_PUSer = (Bit4 & 0xE) >> 1;
            }
            else if (g_ucOffsetInSuperPageBit == 2)
            {
                pPhyAddr->m_OffsetInSuperPage = (Bit4 & 0x3);
                pPhyAddr->m_PUSer = (Bit4 & 0xC) >> 2;
            }
            else if (g_ucOffsetInSuperPageBit == 3)
            {
                pPhyAddr->m_OffsetInSuperPage = (Bit4 & 0x7);
                pPhyAddr->m_PUSer = (Bit4 & 0x8) >> 3;
            }
            else if (g_ucOffsetInSuperPageBit == 4)
            {
                pPhyAddr->m_OffsetInSuperPage = Bit4;
            }
            else
            {
                DBG_Printf("L2_PMTItemToPhyAddr() g_ucPMTItemBitCnt ERROR!\n");
                DBG_Getch();
            }
        }
    }

    if (g_ucPMTItemBitCnt == 29)
    {
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
        switch (LPNInPMTPage % 8)
        {
        case 0:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0; break;
        case 1:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1; break;
        case 2:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2; break;
        case 3:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3; break;
        case 4:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4; break;
        case 5:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5; break;
        case 6:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6; break;
        case 7:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7; break;
        }
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (Bit1 == 0x1))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 2)
                pPhyAddr->m_PUSer |= (Bit1 << 2);
            else if (g_ucOffsetInSuperPageBit == 3)
                pPhyAddr->m_PUSer |= (Bit1 << 1);
            else if (g_ucOffsetInSuperPageBit == 4)
                pPhyAddr->m_PUSer = Bit1;
            else
            {
                DBG_Printf("L2_PMTItemToPhyAddr() g_ucPMTItemBitCnt ERROR!\n");
                DBG_Getch();
            }
        }
    }

    if ((g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
    {
        pPMTItem2Bit = (PMTItem2Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
        switch (LPNInPMTPage % 4)
        {
        case 0:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0; break;
        case 1:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1; break;
        case 2:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2; break;
        case 3:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3; break;
        }

        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (Bit2 == 0x3) && (g_ucPMTItemBitCnt == 30))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 3)
                pPhyAddr->m_PUSer |= (Bit2 << 1);
            else if (g_ucOffsetInSuperPageBit == 4)
                pPhyAddr->m_PUSer = Bit2;
            else
            {
                DBG_Printf("L2_PMTItemToPhyAddr() g_ucPMTItemBitCnt ERROR!\n");
                DBG_Getch();
            }
        }
    }

    if (g_ucPMTItemBitCnt == 31)
    {
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem2Bit + (g_ulLPNCntPerPMTPage >> 2));
        switch (LPNInPMTPage % 8)
        {
        case 0:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0; break;
        case 1:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1; break;
        case 2:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2; break;
        case 3:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3; break;
        case 4:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4; break;
        case 5:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5; break;
        case 6:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6; break;
        case 7:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7; break;
        }
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (Bit2 == 0x3) && (Bit1 == 0x1))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 4)
                pPhyAddr->m_PUSer |= (Bit1 << 2);
            else
            {
                DBG_Printf("L2_PMTItemToPhyAddr() g_ucPMTItemBitCnt ERROR!\n");
                DBG_Getch();
            }
        }
    }
#else

    if (g_ucPMTItemBitCnt == 27)
    {
        switch (LPNInPMTPage % 8)
        {
        case 0:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0; break;
        case 1:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1; break;
        case 2:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2; break;
        case 3:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3; break;
        case 4:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4; break;
        case 5:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5; break;
        case 6:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6; break;
        case 7:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7; break;
        }
        switch (LPNInPMTPage % 4)
        {
        case 0:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0; break;
        case 1:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1; break;
        case 2:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2; break;
        case 3:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3; break;
        }

        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit2 == 0x3) && (Bit1 == 0x1))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {

            if (g_ucOffsetInSuperPageBit == 0)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit2 & 0x1);
                pPhyAddr->m_PUSer = ((Bit2 & 0x2) >> 1) | (Bit1 << 1);
                //default : pPhyAddr->m_OffsetInSuperPage = 0;
            }
            else if (g_ucOffsetInSuperPageBit == 1)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit2 & 0x1);
                pPhyAddr->m_OffsetInSuperPage = (Bit2 & 0x2) >> 1;
                pPhyAddr->m_PUSer = Bit1;
            }
            else if (g_ucOffsetInSuperPageBit == 2)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit2 & 0x1);
                pPhyAddr->m_OffsetInSuperPage = ((Bit2 & 0x2) >> 1) | (Bit1 << 1);
                //default : pPhyAddr->m_PUSer = 0;
            }
        }
    }


    if ((g_ucPMTItemBitCnt == 28) || (g_ucPMTItemBitCnt == 29) || (g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
    {
        /**
        * l_PUserBit              0 1 2 3 4 (5)
        * l_OffsetInSuperPageBit* 4 3 2 1 0
        * l_OffsetInSuperPageBit  5 4 3 2 1 (0)
        */
        switch (LPNInPMTPage % 2)
        {
        case 0: Bit4 = pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem0; break;
        case 1: Bit4 = pPMTItem4Bit[LPNInPMTPage / 2].m_BitPMTItem1; break;
        }
        //printf("Bit4 = %d\n", Bit4);
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (g_ucPMTItemBitCnt == 28))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 0)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit4 & 0x01);
                pPhyAddr->m_PUSer = (Bit4 >> 1) & (0x7);
            }
            else if (g_ucOffsetInSuperPageBit == 1)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit4 & 0x01);
                pPhyAddr->m_OffsetInSuperPage = (Bit4 >> 1) & (0x1);
                pPhyAddr->m_PUSer = (Bit4 >> 2) & (0x3);
            }
            else if (g_ucOffsetInSuperPageBit == 2)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit4 & 0x1);
                pPhyAddr->m_OffsetInSuperPage = (Bit4 >> 1) & (0x3);
                pPhyAddr->m_PUSer = Bit4 >> 3;
            }
            else if (g_ucOffsetInSuperPageBit >= 3)
            {
                pPhyAddr->m_BytePPN[3] = pPhyAddr->m_BytePPN[3] | (Bit4 & 0x1);
                pPhyAddr->m_OffsetInSuperPage = (Bit4 >> 1);
                pPhyAddr->m_PUSer = 0;
            }
        }
    }

    if (g_ucPMTItemBitCnt == 29)
    {
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
        switch (LPNInPMTPage % 8)
        {
        case 0:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0; break;
        case 1:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1; break;
        case 2:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2; break;
        case 3:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3; break;
        case 4:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4; break;
        case 5:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5; break;
        case 6:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6; break;
        case 7:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7; break;
        }
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (Bit1 == 0x1))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 1)
                pPhyAddr->m_PUSer |= (Bit1 << 2);
            else if (g_ucOffsetInSuperPageBit == 2)
                pPhyAddr->m_PUSer |= (Bit1 << 1);
            else if (g_ucOffsetInSuperPageBit == 3)
                pPhyAddr->m_PUSer |= Bit1;
            else if (g_ucOffsetInSuperPageBit == 4)
                pPhyAddr->m_OffsetInSuperPage |= (Bit1 << 3);
        }
    }

    if ((g_ucPMTItemBitCnt == 30) || (g_ucPMTItemBitCnt == 31))
    {
        pPMTItem2Bit = (PMTItem2Bit *)(pPMTItem4Bit + (g_ulLPNCntPerPMTPage >> 1));
        switch (LPNInPMTPage % 4)
        {
        case 0:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem0; break;
        case 1:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem1; break;
        case 2:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem2; break;
        case 3:Bit2 = pPMTItem2Bit[LPNInPMTPage / 4].m_BitPMTItem3; break;
        }

        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (Bit2 == 0x3) && (g_ucPMTItemBitCnt == 30))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 2)
                pPhyAddr->m_PUSer |= (Bit2 << 1);
            else if (g_ucOffsetInSuperPageBit == 3)
                pPhyAddr->m_PUSer = Bit2;
            else if (g_ucOffsetInSuperPageBit == 4)
            {
                pPhyAddr->m_OffsetInSuperPage |= ((Bit2 & 0x1) << 3);
                pPhyAddr->m_PUSer = (Bit2 & 0x2) >> 1;
            }
        }
    }

    if (g_ucPMTItemBitCnt == 31)
    {
        pPMTItem1Bit = (PMTItem1Bit *)(pPMTItem2Bit + (g_ulLPNCntPerPMTPage >> 2));
        switch (LPNInPMTPage % 8)
        {
        case 0:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem0; break;
        case 1:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem1; break;
        case 2:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem2; break;
        case 3:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem3; break;
        case 4:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem4; break;
        case 5:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem5; break;
        case 6:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem6; break;
        case 7:Bit1 = pPMTItem1Bit[LPNInPMTPage / 8].m_BitPMTItem7; break;
        }
        if ((pPhyAddr->m_BytePPN[0] == 0xFF) && (pPhyAddr->m_BytePPN[1] == 0xFF) && (pPhyAddr->m_BytePPN[2] == 0xFF) && (Bit4 == 0xF) && (Bit2 == 0x3) && (Bit1 == 0x1))
        {
            pPhyAddr->m_PPN = INVALID_8F;
        }
        else
        {
            if (g_ucOffsetInSuperPageBit == 3)
                pPhyAddr->m_PUSer |= (Bit1 << 2);
            else if (g_ucOffsetInSuperPageBit == 4)
                pPhyAddr->m_PUSer |= (Bit1 << 1);
        }
    }


#endif
    if (g_ucPMTItemBitCnt == 32)
    {
        pPhyAddr->m_PPN = pPMTItem32Bit[LPNInPMTPage].m_PMTItem;
    }
}
#endif
#endif
