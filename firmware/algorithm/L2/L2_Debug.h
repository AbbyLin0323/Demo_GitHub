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
Filename    :L2_Debug.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.01
Description :include some POSIX C header file
Others      :
Modify      :
****************************************************************************/

#ifndef __L2_DBGINC_H__
#define __L2_DBGINC_H__
#ifdef SIM
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#endif

typedef enum _L2_DEBUG_CODE
{
    /* 0x0 -- 0xf use for commone debug code */

    /* L2 debug code */
    L2_DEBUG_GET_PMTI_INDEX = 0x10,
    L2_DEBUG_GET_PMT_ITEM,  //0x11
    L2_DEBUG_GET_PMT_PAGE_ADDR, //0x12
    L2_DEBUG_READ_LPN,      //0x13
    L2_DEBUG_LOAD_PAGE,     //0x14
    L2_DEBUG_TABLE_CHECK,   //0x15
    L2_DEBUG_DIRTY_CNT_CHECK,   //0x16
    L2_DEBUG_PMT_ITEM_CHECK,    //0x17
    L2_DEBUG_CODE_MAX
}L2_DEBUG_CODE;

typedef struct _GCManager_Print
{
    U16 McuId;
    U16  m_Pu;

    U16 m_SrcPBN;
    U16 m_SrcPPO;
    U16 m_SrcOffset;
    U16 m_GCReadOffset;
    U32 m_DirtyLpnCnt;
    U32 m_FreePageCnt;
    U32 m_FreePageForHost;
    U32 m_NeedCopyForOneWrite;
}GCManager_Print;

typedef struct _PuInfo_Print
{
    U16 McuId;

    U32 m_TimeStamp;

    U16 m_DataBlockCnt;
    U16 m_AllocateBlockCnt;

    U32 m_FreePageCnt;

    U16 m_TargetBlk[TARGET_ALL];
    U16 m_TargetPPO[TARGET_ALL];

}PuInfo_Print;

typedef struct _PRINT_BLK
{
    U8 ucMcuId;
    U8 ucPu;
    U16 usVirBlk;
    U8 ucWriteType;  
    U8 ucTarget;
}PRINT_BLK;

typedef struct _PRINT_TB_FULLRECOVERY
{
    U8 ucMcuId;
    U8 ucPu;
    U16 usBlock;
    U16 usPage;
    U16 usStatus;    
}PRINT_TB_FULLRECOVERY;


extern void  L2_PrintRebuildDirtyCntBlk(U32 PUSer,U32 VirBlk,U8 ucWriteType,BOOL bTarget);
extern void L2_PrintTBFullRecovery(U16 usStatus, U8 ucPuNum, U16 usBlock, U16 usPage);
extern U32 L2_DbgEventHandler(void);
extern void L2_DbgShowAll();
extern void L2_PrintGCManagerDbg(U16 PU);
#endif
