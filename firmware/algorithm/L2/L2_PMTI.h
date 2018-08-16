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
Filename    :L2_PMTI.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.29
Description :defines for PMTI Structure.
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_PMTI_H__
#define __L2_PMTI_H__
#include "L2_Defines.h"
#include "L2_StripeInfo.h"

typedef struct
{
#ifdef PMT_ITEM_SIZE_REDUCE
    U32 m_DirtyBitMap[PMT_DIRTY_BM_SIZE_MAX];
#else
	U32 m_DirtyBitMap[PMT_DIRTY_BM_SIZE];
#endif
}PMTDirtyBitMap;


typedef struct PMTI_Tag
{
    PMTDirtyBitMap m_PMTDirtyBitMapInCE;
#ifdef PMT_ITEM_SIZE_REDUCE
    PhysicalAddr m_PhyAddr[PMTPAGE_CNT_PER_SUPERPU_MAX][LUN_NUM_PER_SUPERPU];
#else
	PhysicalAddr m_PhyAddr[PMTPAGE_CNT_PER_SUPERPU][LUN_NUM_PER_SUPERPU];
#endif
}PMTI;


#define PMTI_SIZE_PER_PU        ((U32)sizeof(PMTI))
#define PMTI_PAGE_COUNT_PER_PU  ((PMTI_SIZE_PER_PU % BUF_SIZE) ? (PMTI_SIZE_PER_PU/BUF_SIZE + 1) : (PMTI_SIZE_PER_PU/BUF_SIZE))

#define PMTI_SUPERSIZE_PER_SUPERPU          ((U32)sizeof(PMTI))
#define PMTI_SUPERPAGE_COUNT_PER_SUPERPU    ((PMTI_SUPERSIZE_PER_SUPERPU % SUPER_PAGE_SIZE) ? (PMTI_SUPERSIZE_PER_SUPERPU/SUPER_PAGE_SIZE + 1) : (PMTI_SUPERSIZE_PER_SUPERPU/SUPER_PAGE_SIZE))

extern GLOBAL  PMTI* g_PMTI[SUBSYSTEM_LUN_MAX];
void L2_InitPMTI();
U32 L2_GetLPNInPu(U32 LPN);
U32 L2_GetPMTIIndexInPu(U32 LPN);
U32 L2_GetLPNFromPMTI(U8 ucSuperPu, U32 PMTIndexInPu, U32 SN);
U32 L2_GetOffsetInPMTPage(U32 LPN);
U32 L2_GetPMTPhysicalAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U32 ulPMTIndexInPu);
void L2_SetDirty(U8 ucSuperPu, U32 ulPMTIndexInPu);
void L2_ClearDirty(U8 ucSuperPu, U32 ulPMTIndexInPu);
BOOL L2_IsPMTPageDirty(U8 ucSuperPu, U32 ulPMTIndexInPu);
void L2_SetPhysicalAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U32 ulPMTIIndexInPu, PhysicalAddr* pAddr);
#endif
