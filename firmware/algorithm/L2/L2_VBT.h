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
#ifndef _L2_VBT_H
#define _L2_VBT_H
#include "Disk_Config.h"
#include "L2_Trim.h"

#define VBT_TARGET_INVALID          0
#define VBT_NOT_TARGET              1
#define VBT_TARGET_HOST_W           2
#define VBT_TARGET_HOST_GC          3
#define VBT_TARGET_TLC_W            4

// the following definitions are used for representing GC source block type
#define VBT_TYPE_INVALID            0
#define VBT_TYPE_HOST               1   //SLC blk
#define VBT_TYPE_TLCW               2   //TLC blk


#ifdef DirtyLPNCnt_IN_DSRAM1
extern GLOBAL U32* g_pDirtyLPNCnt;
#endif

/*Virtual block information table*/
typedef union _VBT_ENTRY
{
    U32 Value[2];
    struct 
    {
        U16 PhysicalBlockAddr[LUN_NUM_PER_SUPERPU];
        U16 StripID;

        U32 DirtyLPNCnt;

        U32 Target : 3;
        U32 bFree : 1;
        U32 bLockedInWL : 1;
        //U32 bPhyAddFindInFullRecovery : 1;
        U32 bTableBlk : 1;
        U32 bsInEraseQueue : 1;
        U32 bTLC : 1;

        U32 VBTType : 3;
        U32 bsInForceGCSrcBlkQ : 1;
        U32 bsRetryFail : 1;
        U32 bWaitEraseSts : 1;
        U32 bResv : 10;

        U32 bPhyAddFindInFullRecovery;
    };
}VBT_ENTRY;

typedef struct _VBT
{
    VBT_ENTRY m_VBT[VIR_BLK_CNT];
}VBT;
extern GLOBAL MCU12_VAR_ATTR VBT *pVBT[SUBSYSTEM_SUPERPU_MAX];

extern BOOL L2_IsSLCBlock(U8 ucSuperPu, U16 VBN);

#define VBT_SIZE_PER_PU        ((U32)sizeof(VBT))
#define VBT_PAGE_COUNT_PER_PU  ((VBT_SIZE_PER_PU % BUF_SIZE) ? (VBT_SIZE_PER_PU/BUF_SIZE + 1) : (VBT_SIZE_PER_PU/BUF_SIZE))

#define VBT_SIZE_PER_SUPERPU                ((U32)sizeof(VBT))
#define VBT_SUPERPAGE_COUNT_PER_SUPERPU     ((VBT_SIZE_PER_SUPERPU % SUPER_PAGE_SIZE) ? (VBT_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE + 1) : (VBT_SIZE_PER_SUPERPU/SUPER_PAGE_SIZE))

void L2_VBT_Init(void);
void L2_VBT_Init_Clear_All(void);

extern void L2_VBT_SetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN, U16 PBN);

void L2_VBTSetInEraseQueue(U8 ucSuperPu, U16 usVBN, U8 ucVal);
void L2_VBTSetWaitEraseSts(U8 ucPuNum, U16 usVBN, U8 ucVal);
void L2_VBT_ResetBlk(U8 ucSuperPu, U16 VBN);
void L2_VBT_Set_TLC(U8 ucSuperPu, U16 VBN, U8 bTrue);
void L2_VBT_SetInvalidPhyBlk(U8 ucSuperPu, U16 VBN);
U8 L2_GetBlkType(U16 PUSer, U16 VBN);
BOOL L2_GetBlkRetryFail(U16 PUSer, U16 VBN);
void L2_SetBlkRetryFail(U16 PUSer, U16 VBN, BOOL RetryFail);

#endif

/********************** FILE END ***************/
