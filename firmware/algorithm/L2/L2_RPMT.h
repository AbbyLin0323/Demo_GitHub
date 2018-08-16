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
Filename    :L2_RPMT.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.02.28
Description :defines for RPMT Data Structure.
Others      :
Modify      :
*******************************************************************************/

#ifndef __L2_RPMT_H__
#define __L2_RPMT_H__
#include "L2_Defines.h"
#include "L2_Init.h"
#include "L2_PMTPage.h"


typedef union RPMT_PER_LUN_Tag
{
    PhysicalPage m_Page;
    struct
    {
        U32 m_RPMTItems[LPN_PER_BLOCK / PG_PER_WL];
        U32 m_SuperPageTS[PG_PER_SLC_BLK];
        U32 m_LunOrderTS[PG_PER_SLC_BLK];
        U32 m_SuperPU;
        U32 m_SuperBlock;
    };
}RPMT_PER_LUN;

typedef union RPMT_Tag
{
    RPMT_PER_LUN m_RPMT[LUN_NUM_PER_SUPERPU];
}RPMT;


U32 L2_LookupRPMT(RPMT* pRPMT, PhysicalAddr* pAddr);
U32 L2_LookupRPMTInTLC(RPMT* pRPMT, PhysicalAddr* pAddr);
void L2_UpdateRPMT(U8 ucSuperPu, U8 ucLUNInSuperPU, U32* LPN, TargetType AcutalType, PhysicalAddr* pAddr);
void L2_RecordRPMTSuperPageTS(RPMT* pRPMT, U16 usPageNum, U32 ulSuperPageTS);
void L2_RecordRPMTPUBlockInfo(RPMT* pRPMT, U8 ucSuperPU, U16 usVBN);

#endif
