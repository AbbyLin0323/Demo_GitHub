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
Filename    :L2_RPMT.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.06
Description :functions about RPMT
1) L2_CopyRPMT()
Others      :
Modify      :
*******************************************************************************/
#include "L2_RPMT.h"
#include "L2_StripeInfo.h"

U32 L2_LookupRPMT(RPMT* pRPMT, PhysicalAddr* pAddr)
{
    U16 usLPNOffset;
    U32 ulLPN;

    usLPNOffset = pAddr->m_PageInBlock * LPN_PER_BUF + pAddr->m_LPNInPage;
    ulLPN = pRPMT->m_RPMT[pAddr->m_OffsetInSuperPage].m_RPMTItems[usLPNOffset];

    return ulLPN;

}

U32 L2_LookupRPMTInTLC(RPMT* pRPMT, PhysicalAddr* pAddr)
{
    U16 Offset;
    U32 LPN;
    U32 PageIndexInSrcBlk;

    PageIndexInSrcBlk = pAddr->m_PageInBlock % (PG_PER_SLC_BLK - 1);
    Offset = PageIndexInSrcBlk * LPN_PER_BUF + pAddr->m_LPNInPage;

    LPN = pRPMT->m_RPMT[pAddr->m_OffsetInSuperPage].m_RPMTItems[Offset];
    return LPN;
}

void L2_UpdateRPMT(U8 ucSuperPu, U8 ucLUNInSuperPU, U32* LPN, TargetType AcutalType, PhysicalAddr* pAddr)
{
    U8 ucLunOrderTS;
    U8 ucLPNInBUF;
    PuInfo* pInfo = g_PuInfo[ucSuperPu];
    U32 ulRPMTBufferPointer = g_PuInfo[ucSuperPu]->m_RPMTBufferPointer[AcutalType];

#ifdef L2_PMTREBUILD_SUPERPAGETS_NOTSAME
    pInfo->m_pRPMT[AcutalType][ulRPMTBufferPointer]->m_RPMT[ucLUNInSuperPU].m_SuperPageTS[pAddr->m_PageInBlock] = L2_GetTimeStampInPu(ucSuperPu);
#endif
    ucLunOrderTS = pInfo->m_TargetOffsetTS[AcutalType][ucLUNInSuperPU];
    pInfo->m_pRPMT[AcutalType][ulRPMTBufferPointer]->m_RPMT[ucLUNInSuperPU].m_LunOrderTS[pAddr->m_PageInBlock] = ucLunOrderTS;
    for (ucLPNInBUF = 0; ucLPNInBUF < LPN_PER_BUF; ucLPNInBUF++)
    {
        pInfo->m_pRPMT[AcutalType][ulRPMTBufferPointer]->m_RPMT[ucLUNInSuperPU].m_RPMTItems[pAddr->m_PageInBlock * LPN_PER_BUF + ucLPNInBUF] = LPN[ucLPNInBUF];  
    }

    return;
}

void L2_RecordRPMTSuperPageTS(RPMT* pRPMT, U16 usPageNum, U32 ulSuperPageTS)
{
    U8 ucLunInSuperPu;
    
    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPageTS[usPageNum] = ulSuperPageTS;
    }

    return;
}

void L2_RecordRPMTPUBlockInfo(RPMT* pRPMT, U8 ucSuperPU, U16 usVBN)
{
    U8 ucLunInSuperPu;

    for (ucLunInSuperPu = 0; ucLunInSuperPu < LUN_NUM_PER_SUPERPU; ucLunInSuperPu++)
    {
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperPU = ucSuperPU;
        pRPMT->m_RPMT[ucLunInSuperPu].m_SuperBlock = usVBN;
    }

   return;
}

