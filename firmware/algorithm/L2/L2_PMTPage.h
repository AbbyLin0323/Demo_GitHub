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
Filename    :L2_PMTPage.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.29
Description :defines for PMTPage Structure.
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_PMT_PAGE_H__
#define __L2_PMT_PAGE_H__
#include "Disk_Config.h"
#include "L2_Defines.h"
#include "L2_Init.h"


#ifdef PMT_ITEM_SIZE_3BYTE
typedef struct PMT_PAGE_TAG
{
    union
    {
        SuperPage m_Page;
        PMTItem m_PMTItems[LPN_CNT_PER_PMTPAGE];
    };
}PMTPage;

extern void L2_PhyAddrToPMTItem(PhysicalAddr* pPhyAddr, PMTItem* pPMTItem);
extern void L2_PMTItemToPhyAddr(PhysicalAddr* pPhyAddr, PMTItem* pPMTItem);

#elif defined(PMT_ITEM_SIZE_REDUCE)

//Adjustable PMTItem Strategy needed
extern GLOBAL U32 g_ulLPNCntPerPMTPage;
extern GLOBAL U32 g_ulPMTPageCntPerSPU;

typedef struct PMT_ITEM32Bit_TAG
{
	union {
		U32 m_PMTItem;
		U8 m_BytePMTItem[4];
	};
}PMTItem32Bit;

typedef struct PMT_ITEM24Bit_TAG
{
	U8 m_PMTItem[3];
}PMTItem24Bit;

typedef struct PMT_ITEM4Bit_TAG
{
	union {
		U8 m_PMTItem;
		struct {
			U8 m_BitPMTItem0 : 4;
			U8 m_BitPMTItem1 : 4;
		};
	};
}PMTItem4Bit;

typedef struct PMT_ITEM2Bit_TAG
{
	union {
		U8 m_PMTItem;
		struct {
			U8 m_BitPMTItem0 : 2;
			U8 m_BitPMTItem1 : 2;
			U8 m_BitPMTItem2 : 2;
			U8 m_BitPMTItem3 : 2;
		};
	};
}PMTItem2Bit;

typedef struct PMT_ITEM1Bit_TAG
{
	union {
		U8 m_PMTItem;
		struct {
			U8 m_BitPMTItem0 : 1;
			U8 m_BitPMTItem1 : 1;
			U8 m_BitPMTItem2 : 1;
			U8 m_BitPMTItem3 : 1;
			U8 m_BitPMTItem4 : 1;
			U8 m_BitPMTItem5 : 1;
			U8 m_BitPMTItem6 : 1;
			U8 m_BitPMTItem7 : 1;
		};
	};
}PMTItem1Bit;

extern void L2_PhyAddrToPMTItem(PhysicalAddr* pPhyAddr, U32*pPMTPage, U32 LPNInPMTPage);
extern void L2_PMTItemToPhyAddr(PhysicalAddr* pPhyAddr, U32*pPMTPage, U32 LPNInPMTPage);
extern void L2_PMTBitInit(U8 ucPUserNum, U8 ucLunNum, U8 ucBlockInPUBit, U8 ucPageinBlockBit, U8 ucLPNInPageBit);
#else
typedef struct PMTPage_Tag
{
    union
    {
        SuperPage m_Page;
        U32 m_PMTItems[LPN_CNT_PER_PMTPAGE];
    };
}PMTPage;
#endif

extern GLOBAL  PhysicalAddr* g_DebugPMT;

extern void L2_UpdateDebugPMT(PhysicalAddr* pAddr, U32 LPN);
extern void L2_InitDebugPMT(void);
extern void L2_LookupDebugPMT(PhysicalAddr* pAddr, U32 LPN);
extern void L2_IncreaseOffsetInAddr(PhysicalAddr* pAddr);

#endif

