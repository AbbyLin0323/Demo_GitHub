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
Filename    :L2_TableBBT.h
Version     :Ver 1.0
Author      :Via
Date        :2015.05.21
Description :functions about bbt

Others      :
Modify      :
*******************************************************************************/
#ifndef ___L2_TABLEBBT_H___
#define ___L2_TABLEBBT_H___

#include "BaseDef.h"
#include "Proj_Config.h"
#include "L2_Defines.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Redefined:
// For simplicity, we allocate a full page size to save global-bbt, and allocate a single/multi pages to save each local-bbt.
// The general formula of the selected block status:
// Fun(TLun, Pln, Blk) = TLun*BBT_BLK_PER_LUN + Pln*BBT_BLK_PER_PLN + Blk
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BBT_RDT_MARK                (0x20151224)
#define BBT_NEW_MARK                (0x20150525)
#define BBT_NEW_V2_MARK             (0x20180320)

#define GBBT_BLK                    1
#define LBBT_BLK                    0
#define BAD_BLK_TYPE_NUM            15

#define START_PLN_IN_GBBT           0
#define START_PLN_IN_LBBT           ((0 == LBBT_BLK) ? 1 : 0) // pln0 is used for saving fw when LBBT_BLK==0

#define PLN_NUM_PER_GBBT            (PLN_PER_LUN)
#define PLN_NUM_PER_LBBT            (PLN_PER_LUN - START_PLN_IN_LBBT)

#define GBBT_BUF_SIZE               (LOGIC_PG_SZ * PLN_NUM_PER_GBBT)
#define LBBT_BUF_SIZE               (LOGIC_PG_SZ * PLN_NUM_PER_LBBT)
#define LBBT_BUF_SIZE_TOTAL         (LBBT_BUF_SIZE * SUBSYSTEM_LUN_NUM)

#define BBT_PG_PER_BLK              (PG_PER_SLC_BLK)
#define BBT_BLK_PER_PLN             (BLK_PER_PLN + RSV_BLK_PER_PLN)
#define BBT_BLK_PER_LUN             (BBT_BLK_PER_PLN * PLN_PER_LUN)

#define RT_DATA_BYTE_NUM            (2*4) //2 //2DW for root data
#define SOFT_BINDING_TABLE_OFFSET   ((((BBT_BLK_PER_LUN * 2) + 2 * RT_DATA_BYTE_NUM + 1024) >> 10) << 10)

#define BYTE_ALIGN(x)               ((((x)>>3) + (((x)&0x7)?1:0)) << 3)
#define EXT_BAD_BLK_OFFSET          (BYTE_ALIGN(PLN_PER_LUN*BBT_BLK_PER_PLN))


// Format BBT
typedef enum _BBT_FORMAT_STAGE_
{
    BBT_FORMAT_INIT = 0,
    BBT_FORMAT_LOCAL_BBT,
    BBT_FORMAT_GLOBAL_BBT,
    BBT_FORMAT_FINISH
}BBT_FORMAT_STAGE;
typedef enum _LGBBT_FORMAT_STAGE_
{
    LGBBT_FORMAT_INIT = 0,
    LGBBT_FORMAT_ERS,
    LGBBT_FORMAT_ERS_WAIT,
    LGBBT_FORMAT_FINISH
}LGBBT_FORMAT_STAGE;

// Find BBT Target
typedef enum _BBT_FIND_STAGE_
{
    BBT_FIND_INIT = 0,
    BBT_FIND_LOCAL_BBT,
    BBT_FIND_GLOBAL_BBT,
    BBT_FIND_FORMAT_BBT,
    BBT_FIND_FORMAT_GBBT,
    BBT_FIND_FINISH
}BBT_FIND_STAGE;

typedef enum _LBBT_FIND_STAGE_
{
    LBBT_FIND_INIT = 0,
    LBBT_FIND_READ_RED,
    LBBT_FIND_READ_WAIT,
    LBBT_FIND_FINISH
}LBBT_FIND_STAGE;

typedef enum _GBBT_FIND_STAGE_
{
    GBBT_FIND_INIT = 0,
    GBBT_FIND_READ_RED,
    GBBT_FIND_READ_WAIT,
    GBBT_FIND_FINISH
}GBBT_FIND_STAGE;

typedef enum _ERS_WHOLE_DISK_STAGE_
{
    ERS_WHOLE_DISK_INIT = 0,
    ERS_WHOLE_DISK_ERS,
    ERS_WHOLE_DISK_ERS_WAIT,
    ERS_WHOLE_DISK_FINISH,
    ERS_WHOLE_DISK_READ_ERS_FAIL_STATUS,
    ERS_WHOLE_DISK_READ_STATUS_WAIT
}ERS_WHOLE_DISK_STAGE;

typedef enum _BBT_BUILD_STAGE_
{
    BBT_BUILD_INIT = 0,
    BBT_BUILD_READ_IDB,
    BBT_BUILD_FORMAT_BBT,
    BBT_BUILD_FORMAT_GBBT,    
    BBT_BUILD_LOAD_BBT,
    BBT_BUILD_FORMAT_DISK,   
    BBT_BUILD_REDETECT_IDB,
    BBT_BUILD_SAVE_BBT,
    BBT_BUILD_FINISH,
    BBT_BUILD_PBN_BINDING_TABLE
}BBT_BUILD_STAGE;

typedef enum _BBT_LOAD_STAGE_
{
    BBT_LOAD_INIT = 0,
    BBT_LOAD_READ_GBBT,
    BBT_LOAD_READ_LBBT,
    BBT_LOAD_REBUILD,
    BBT_LOAD_FINISH
}BBT_LOAD_STAGE;

typedef enum _BBT_LOAD_RDT_STAGE_
{
	BBT_LOAD_RDT_INIT = 0,
	BBT_LOAD_RDT_FIND_TARGET,
	BBT_LOAD_RDT_READ_TARGET,
	BBT_LOAD_RDT_REBUILD_BBT,
	BBT_LOAD_RDT_FINISH
}BBT_LOAD_RDT_STAGE;

typedef enum _BBT_LOAD_GBBT_STAGE_
{
    BBT_LOAD_GBBT_INIT = 0,
    BBT_LOAD_GBBT_READ,
    BBT_LOAD_GBBT_READ_WAIT,
    BBT_LOAD_GBBT_FINISH
}BBT_LOAD_GBBT_STAGE;

typedef enum _BBT_SAVE_STAGE_
{
    BBT_SAVE_INIT = 0,
    BBT_SAVE_GBBT,
    BBT_SAVE_MERGE_LBBT,
    BBT_SAVE_LBBT,
    BBT_SAVE_FINISH
}BBT_SAVE_STAGE;

typedef enum _LBBT_SAVE_STAGE_
{
    LBBT_SAVE_INIT = 0,
    LBBT_SAVE_WRITE,
    LBBT_SAVE_WRITE_WAIT,
    LBBT_SAVE_ERASE,
    LBBT_SAVE_ERASE_WAIT,
    LBBT_SAVE_FINISH
}LBBT_SAVE_STAGE;

typedef enum _GBBT_SAVE_STAGE_
{
    GBBT_SAVE_INIT = 0,
    GBBT_SAVE_WRITE,
    GBBT_SAVE_WRITE_WAIT,
    GBBT_SAVE_ERASE,
    GBBT_SAVE_ERASE_WAIT,
    GBBT_SAVE_FINISH
}GBBT_SAVE_STAGE;

typedef enum _BBT_REBUILD_STAGE_
{
    BBT_REBUILD_INIT = 0,
    BBT_REBUILD_LOAD_LBBT,
    BBT_REBUILD_MERGE_GBBT,
    BBT_REBUILD_SAVE_BBT,
    BBT_REBUILD_FINISH
}BBT_REBUILD_STAGE;

typedef enum _BBT_REBUILD_LOAD_LBBT_STAGE_
{
    BBT_REBUILD_LOAD_LBBT_INIT = 0,
    BBT_REBUILD_LOAD_LBBT_READ,
    BBT_REBUILD_LOAD_LBBT_READ_WAIT,
    BBT_REBUILD_LOAD_LBBT_FINISH
}BBT_REBUILD_LOAD_LBBT_STAGE;

typedef enum _BBT_READ_LBBT_STAGE_
{
    BBT_READ_LBBT_INIT = 0,
    BBT_READ_LBBT_READ,
    BBT_READ_LBBT_READ_WAIT,
    BBT_READ_LBBT_FINISH
}BBT_READ_LBBT_STAGE;

typedef enum _PBN_BINDING_TABLE_LOAD_STAGE
{
    PBN_BINDING_TABLE_LOAD_INIT = 0,
    PBN_BINDING_TABLE_LOAD_READ,
    PBN_BINDING_TABLE_LOAD_READ_WAIT,
    PBN_BINDING_TABLE_LOAD_FINISH
} PBN_BINDING_TABLE_LOAD_STAGE;

//Extend for sorting tool
typedef enum _BAD_BLK_TYPE_
{
    GOOD_BLK = 0,
    IDB_BAD_BLK,
    ST_BAD_BLK,//sorting time bad blk
    RT_BAD_BLK,//runnig time bad blk
    BAD_BLK_NUM
}BAD_BLK_TYPE;

typedef enum _BAD_BLK_ERR_TYPE_
{
    ERASE_ERR = 0,
    WRITE_ERR,
    READ_ERR,
    RSVD
}BAD_BLK_ERR_TYPE;

// sizeof(BBT_MANAGER_ENTRY) = 4DW
typedef struct _BBT_MANAGER_
{
    U32 bsOpStage : 4;    
    U32 bsPln : 4;
    U32 bsDChk : 1;
    U32 bsRsvd : 7;
    U32 bsBlk : 16;

    U32 bsPage : 16;
    U32 bsBuffID : 16;

    U8  *pFlashStatus;
    RED *pSpare;
}BBT_MANAGER;

/////////////////////////////////////////////////////////////////
// the global interfaces declearation
/////////////////////////////////////////////////////////////////
void L2_BbtDramPageAlignAllocate(U32* pFreeDramBase);
void L2_BbtDramAllocate(U32* pFreeDramBase);
U32  L2_BbtGetGBBTAddr(void);
BOOL L2_BbtBuild(BOOL bSecurityErase);
BOOL L2_BbtLoad(BOOL *pValidBBT);
BOOL L2_BbtSave(U8 ucTLun, U8 ucErrHTLun);
void L2_BbtAddBbtBadBlk(U8 ucTLun, U16 usPhyBlk, U8 BadBlkType, U8 ucErrType);
BOOL L2_BbtIsGBbtBadBlock(U8 ucTLun, U16 usPhyBlk);
void L2_BbtSetRootPointer(U8 uTLun, U32 ulTableBlkEnd, U32 ulDataBlkStart);
void L2_BbtGetRootPointer(U8 uTLun, U32 *pTableBlkEnd, U32 *pDataBlkStart);
void L2_BbtGetSlcTlcBlock(U8 ucTLun, U32 *pSLCBlkEnd, U32 *pTLCBlkEnd);
void L2_BbtSetSlcTlcBlock(U8 ucTLun, U32 ulSLCBlkEnd, U32 ulTLCBlkEnd);
void L2_BbtMemZeroGBbt(void);

void L2_BbtSchedule(void);
BOOL L2_BbtIsSavedDone(void);

GLOBAL BOOL L2_BbtLoadPbnBindingTable(void);
GLOBAL void L2_BbtPbnBindingTableCheck(void);
GLOBAL void L2_BbtEnablePbnBindingTable(void);
GLOBAL void L2_BbtDisablePbnBindingTable(void);
GLOBAL U8 L2_BbtPbnBindingTableExistenceCheck(BOOL* pHardBindingTableExist);
GLOBAL U8 L2_BbtIsPbnBindingTableEnable(void);
GLOBAL U16 L2_BbtGetPbnBindingTable(U8 ucTlun, U16 usPbn, U8 ucPlane);

BOOL Ven_BbtLoad(void);
void Ven_BbtSave(void);

void L2_BbtPrintAllBbt(void);

GLOBAL void MCU12_DRAM_TEXT L2_BbtSetLBbtBadBlkBit_Ext(U8 ucTLun, U8 ucPln, U16 usBlock, U8 BadBlkType, U8 ucErrType);

#endif


