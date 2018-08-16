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
Filename    :L2_PMTI.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.1
Description :functions about PMTI
1) L2_InitPMTI(): init the value to boot up status
Others      :
Modify      :
*******************************************************************************/
#include "HAL_Inc.h"
#include "L2_PMTI.h"
#include "L2_Thread.h"
#include "L2_RT.h"
#include "COM_Memory.h"
#include "L2_boot.h"

extern RebuildPMTI* l_RebuildPMTIDptr;

void MCU1_DRAM_TEXT L2_InitPMTI()
{
    U16 uPMTIndexInPu;
    U8 ucSuperPu, ucLunInSuperPu;
    U8 ucBlockIndex;

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        HAL_DMAESetValue((U32)&(g_PMTI[ucSuperPu]->m_PMTDirtyBitMapInCE.m_DirtyBitMap[0]), COM_MemSize16DWAlign(sizeof(PMTDirtyBitMap)), 0);

        for (uPMTIndexInPu = 0; uPMTIndexInPu < PMTPAGE_CNT_PER_SUPERPU; uPMTIndexInPu++)
        {
            for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
            {
                g_PMTI[ucSuperPu]->m_PhyAddr[uPMTIndexInPu][ucLunInSuperPu].m_PPN = INVALID_8F;
            }
        }

        for(ucBlockIndex = 0; ucBlockIndex < AT1_BLOCK_COUNT; ucBlockIndex++)
        {
            l_RebuildPMTIDptr->m_UECCCnt[ucSuperPu][ucBlockIndex] = 0;
            l_RebuildPMTIDptr->m_NeedErase[ucSuperPu][ucBlockIndex] = FALSE;
        }
    }
    
    return;
}

/****************************************************************************
Name        :L2_GetLPNInPu
Input       :U32 LPN
Output      :ulLpnInPu
Author      :HenryLuo
Date        :2012.05.02    18:19:15
Description :get logic lpn number in Pu from LPN
Others      :
Modify      :
****************************************************************************/
U32 L2_GetLPNInPu(U32 LPN)
{
    U32 ulPuArrayNum;
    U32 ulLpnInPuBase;
    U32 ulLpnInPu;

    ulPuArrayNum = LPN / (SUBSYSTEM_SUPERPU_NUM * LPN_PER_BUF);
    ulLpnInPuBase = ulPuArrayNum * LPN_PER_BUF;
    ulLpnInPu = ulLpnInPuBase + LPN % LPN_PER_BUF;

    return ulLpnInPu;
}

//////////////////////////////////////////////////////////////////////////
//U32 GetPMTIIndex(U32 LPN)
//function:
//    Get the Serial Number of the PMTPage for the LPN
//    one PMT Page should only contains the Mapping table of one Pu
//////////////////////////////////////////////////////////////////////////
U32 L2_GetPMTIIndexInPu(U32 LPN)
{
    U32 ulPMTIIndexInPu;
#ifdef PMT_ITEM_SIZE_REDUCE
	U32 LPNInPu;

	LPNInPu = L2_GetLPNInPu(LPN);
	ulPMTIIndexInPu = (LPNInPu / (LPN_CNT_PER_PMTPAGE));
#else
	ulPMTIIndexInPu = (LPN / (SUBSYSTEM_SUPERPU_NUM * LPN_CNT_PER_PMTPAGE));
#endif

    return ulPMTIIndexInPu;
}


/****************************************************************************
Name        :L2_GetLPNFromPMTI
Input       :U32 PMTIndex, U32 SN
Output      :ulLpnInSystem
Author      :HenryLuo
Date        :2012.10.17    09:46:01
Description :Get LPN from PMTI.
Others      :
Modify      :
****************************************************************************/
U32 L2_GetLPNFromPMTI(U8 ucSuperPu, U32 PMTIndexInSuperPu, U32 SN)
{
    U32 ulLPNInPu;
    U32 ulLpnInSystem;

    ulLPNInPu = PMTIndexInSuperPu * LPN_CNT_PER_PMTPAGE + SN;
    ulLpnInSystem = GetLpnInSystem(ucSuperPu, ulLPNInPu);

    return ulLpnInSystem;
}

/*****************************************************************************
 Prototype      : L2_GetPMTIndexInSystem
 Description    : get PMTIndex in system
 Input          : U32 PMTIndexInSuperPu,U32 PuID
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2013/10/15
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
U32 L2_GetOffsetInPMTPage(U32 LPN)
{
    U32 LPNInPu;
    U32 Offset;

    LPNInPu = L2_GetLPNInPu(LPN);
    Offset = LPNInPu % LPN_CNT_PER_PMTPAGE;
    return Offset;
}

U32 L2_GetPMTPhysicalAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U32 ulPMTIndexInSuperPu)
{
    PhysicalAddr Addr = { 0 };
    U16 usBlockInCE;

    Addr.m_PPN = g_PMTI[ucSuperPu]->m_PhyAddr[ulPMTIndexInSuperPu][ucLunInSuperPu].m_PPN;
    if (Addr.m_PPN == INVALID_8F)
    {
        return INVALID_8F;
    }

    /* save logic block SN in PMTI, modify by henryluo 2014-01-24 */
    usBlockInCE = L2_RT_GetAT1BlockPBNFromSN(Addr.m_PUSer, Addr.m_OffsetInSuperPage, Addr.m_BlockInPU);
    Addr.m_BlockInPU = usBlockInCE;

    return Addr.m_PPN;
}

void L2_SetPMTDirtyFlag(U8 ucSuperPu, U32 PMTIndexInSuperPu, BOOL DirtyFlag)
{
    U32 index = PMTIndexInSuperPu / 32;
    U32 bit = PMTIndexInSuperPu % 32;

    if (DirtyFlag)
    {
        g_PMTI[ucSuperPu]->m_PMTDirtyBitMapInCE.m_DirtyBitMap[index] |= (1 << bit);
    }
    else
    {
        g_PMTI[ucSuperPu]->m_PMTDirtyBitMapInCE.m_DirtyBitMap[index] &= ~(1 << bit);
    }
}

BOOL L2_GetPMTDirtyFlag(U8 ucSuperPu, U32 PMTIndexInSuperPu)
{
    U32 index = PMTIndexInSuperPu / 32;
    U32 bit = PMTIndexInSuperPu % 32;

    return (g_PMTI[ucSuperPu]->m_PMTDirtyBitMapInCE.m_DirtyBitMap[index] & (1 << bit));
}

void L2_SetDirty(U8 ucSuperPu, U32 ulPMTIndexInSuperPu)
{
    L2_SetPMTDirtyFlag(ucSuperPu, ulPMTIndexInSuperPu, TRUE);
}

void L2_ClearDirty(U8 ucSuperPu, U32 ulPMTIndexInSuperPu)
{
    L2_SetPMTDirtyFlag(ucSuperPu, ulPMTIndexInSuperPu, FALSE);
}

BOOL L2_IsPMTPageDirty(U8 ucSuperPu, U32 ulPMTIndexInSuperPu)
{
    return L2_GetPMTDirtyFlag(ucSuperPu, ulPMTIndexInSuperPu);
}
void L2_SetPhysicalAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U32 ulPMTIIndexInPu, PhysicalAddr* pAddr)
{
    U8 ucBlockSN;

    /* save logic block SN in PMTI, modify by henryluo 2014-01-24 */
    ucBlockSN = L2_RT_GetAT1BlockSNFromPBN(pAddr->m_PUSer, pAddr->m_OffsetInSuperPage, pAddr->m_BlockInPU);
    pAddr->m_BlockInPU = ucBlockSN;

    g_PMTI[ucSuperPu]->m_PhyAddr[ulPMTIIndexInPu][ucLunInSuperPu].m_PPN = pAddr->m_PPN;
}
