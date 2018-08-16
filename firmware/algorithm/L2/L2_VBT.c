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
File Name     : L2_DWA.c
Version       : Initial version
Author        : henryluo
Created       : 2015/02/28
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 20120118
Author      : peterxiu
Modification: Created file

*******************************************************************************/
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L2_Defines.h"
#include "L2_Debug.h"
#include "L2_PMTManager.h"
#include "L2_StripeInfo.h"
#include "L2_GCManager.h"
#include "L2_LLF.h"
#include "L2_VBT.h"
#include "L2_Thread.h"
#include "L1_GlobalInfo.h"
#include "L2_StaticWL.h"
#include "COM_Memory.h"
#ifdef SIM
#include "sim_search_engine.h"
#endif

#ifdef DirtyLPNCnt_IN_DSRAM1
GLOBAL U32* g_pDirtyLPNCnt;
#endif

void MCU1_DRAM_TEXT L2_VBT_Init_Clear_All(void)
{
    U8 ucPuNum;

    for (ucPuNum = 0; ucPuNum < SUBSYSTEM_SUPERPU_NUM; ucPuNum++)
    {
        HAL_DMAESetValue((U32)pVBT[ucPuNum], COM_MemSize16DWAlign(sizeof(VBT)), 0);
    }

#ifdef DirtyLPNCnt_IN_DSRAM1
    COM_MemZero(g_pDirtyLPNCnt, COM_MemSize16DWAlign(SUBSYSTEM_SUPERPU_NUM * VIR_BLK_CNT));
#endif

    return;
}

void MCU1_DRAM_TEXT L2_VBT_Init(void)
{
    L2_VBT_Init_Clear_All();

    return;
}

void MCU1_DRAM_TEXT L2_VBT_Set_TLC(U8 ucSuperPu, U16 VBN, U8 bTrue)
{
    pVBT[ucSuperPu]->m_VBT[VBN].bTLC = bTrue;
}

BOOL L2_IsSLCBlock(U8 ucSuperPu, U16 VBN)
{
    return !(pVBT[ucSuperPu]->m_VBT[VBN].bTLC);
}

void MCU1_DRAM_TEXT L2_VBTSetInEraseQueue(U8 ucPuNum, U16 usVBN, U8 ucVal)
{
    pVBT[ucPuNum]->m_VBT[usVBN].bsInEraseQueue = ucVal;
    return;
}

void MCU1_DRAM_TEXT L2_VBTSetWaitEraseSts(U8 ucPuNum, U16 usVBN, U8 ucVal)
{
    pVBT[ucPuNum]->m_VBT[usVBN].bWaitEraseSts = ucVal;
    return;
}

void MCU1_DRAM_TEXT L2_VBT_ResetBlk(U8 ucSuperPu, U16 VBN)
{
#ifdef DirtyLPNCnt_IN_DSRAM1
    *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN) = 0;
#else
    pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt = 0;
#endif
    pVBT[ucSuperPu]->m_VBT[VBN].StripID = INVALID_4F;
    pVBT[ucSuperPu]->m_VBT[VBN].Target = VBT_TARGET_INVALID;
    pVBT[ucSuperPu]->m_VBT[VBN].bFree = TRUE;
    pVBT[ucSuperPu]->m_VBT[VBN].VBTType = VBT_TYPE_INVALID;
    return;
}

void MCU1_DRAM_TEXT L2_VBT_SetInvalidPhyBlk(U8 ucSuperPu, U16 VBN)
{
    U32 i;

    for (i = 0; i < LUN_NUM_PER_SUPERPU; i++)
    {
        L2_VBT_SetPhysicalBlockAddr(ucSuperPu, i, VBN, INVALID_4F);
    }
    return;
}


BOOL L2_GetBlkRetryFail(U16 PUSer, U16 VBN)
{
    return pVBT[PUSer]->m_VBT[VBN].bsRetryFail;
}


U8 L2_GetBlkType(U16 PUSer, U16 VBN)
{
    return pVBT[PUSer]->m_VBT[VBN].VBTType;
}


/********************** FILE END ***************/
