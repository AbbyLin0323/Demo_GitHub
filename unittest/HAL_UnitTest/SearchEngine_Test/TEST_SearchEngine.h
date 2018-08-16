/****************************************************************************
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
*****************************************************************************
 * File Name    : TEST_SearchEngine.h
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#ifndef _TEST_SEARCHENGINE_H
#define _TEST_SEARCHENGINE_H

#include "Disk_Config.h"

//test pattern
#define SE_TEST_VALUE_DRAM
#define SE_TEST_OVERLAP_DRAM
#define SE_TEST_VALUE_OTFB
#define SE_TEST_OVERLAP_OTFB
#define SE_TEST_SEARCH_BIT_MEM
#define SE_TEST_SEARCH_BIT_REG
#define SE_TEST_SET_BIT
//#define SE_TEST_VALUE_DRAM_DWORD
//#define SE_TEST_VALUE_OTFB_DWORD

#define TEST_SE_LIST_LEN  64

typedef enum _SE_SEARCH_CASE
{
    ERASE_CNT_MAX = 0,
    ERASE_CNT_MIN,
    BACKUP_ERASE_CNT_MAX,
    BACKUP_ERASE_CNT_MIN,
    DIRTY_CNT_MAX,
    DIRTY_CNT_MIN,
    SEARCH_CASE_MAX                              //search case param in DRAM support max to 8
}SE_SEARCH_CASE;

typedef union _VBT_ENTRY
{
    U32 Value[2];
    struct 
    {
        U16 PhysicalBlockAddr[LUN_NUM_PER_SUPERPU];
        U16 StripID;

        U32 DirtyLPNCnt:16;     
        U32 Target:2;
        U32 BlockType:2;
        U32 bFree:1;
        U32 bError:1;
        U32 bAllocated:1;
        U32 Rsvd:9;
    };
}VBT_ENTRY;

typedef struct _VBT
{
    VBT_ENTRY m_VBT[SUBSYSTEM_SUPERPU_MAX][BLK_PER_LUN];
}VBT;

typedef struct _TEST_SE_OVERLAP_ITEM
{
    U32 ulAccessLen;
    U32 ulStartValue;
}TEST_SE_OVERLAP_ITEM;

typedef struct _TEST_SE_OVERLAP_LIST
{
    TEST_SE_OVERLAP_ITEM List[TEST_SE_LIST_LEN];
}TEST_SE_OVERLAP_LIST;

void TEST_SEMain(void);

#endif
