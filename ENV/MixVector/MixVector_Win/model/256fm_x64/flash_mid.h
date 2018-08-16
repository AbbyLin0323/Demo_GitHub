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
*******************************************************************************/

#ifndef _COMMON_STR
#define _COMMON_STR

#include "windows.h"
#include "BaseDef.h"
#include "HAL_Inc.h"
#include "HAL_FlashChipDefine.h"

// modify for x64
typedef struct _SATA_RAM_DEF
{
    UINT32 ulStartLBA;
    UINT32 ulLen;
    UINT32 *pDataAddr;
}SATA_RAM_DEF;

typedef struct _FLASH_TALBE_BLOCK_MANT
{
    U16 StartLogPge;
    U16 EndLogPge;

}FLASH_TALBE_BLOCK_MANT, *pFLASH_TALBE_BLOCK_MANT;

typedef struct _FLASH_TALBE_PAGE_MANT
{
    U16 bValid;
    U16 PhyPageID;
    U32 NextLogID;

}FLASH_TALBE_PAGE_MANT, *pFLASH_TALBE_PAGE_MANT;

typedef struct _FLASH_COM_DATA
{
    UINT32 ulWriteCnt;
    UINT32 ulLBA;
    
}FLASH_COM_DATA, *pFLASH_COM_DATA;

typedef struct _FLASH_DATA_PAGE
{
    FLASH_COM_DATA ComData[SEC_PER_PG];
    UINT32 RedData[RED_SZ_DW];

}FLASH_DATA_PAGE, *pFLASH_DATA_PAGE;

typedef struct _FLASH_TABLE_PAGE
{
    UINT32 TableData[PG_SZ];
    UINT32 RedData[RED_SZ_DW];

}FLASH_TABLE_PAGE, *pFLASH_TABLE_PAGE;

//typedef struct _FLASH_TABLE_BLOCK
//{
//    UINT32 nBlockID;
//    FLASH_TABLE_PAGE BlockTableData [PG_PER_BLK];
//}FLASH_TABLE_BLOCK, *pFLASH_TABLE_BLOCK;

#define DATA_BLOCK_PER_PLN (BLK_PER_CE + RSVD_BLK_PER_CE - NORMAL_BLOCK_ADDR_BASE)
#define TABLE_BLOCK_PER_PLN 20
#define TABLE_PAGE_PER_PLN  (PG_PER_BLK * TABLE_BLOCK_PER_PLN)
#define TOTAL_BLOCK_PER_PLN (BLK_PER_CE + RSVD_BLK_PER_CE)

typedef struct _FLASH_TABLE_PLN
{
    FLASH_TALBE_BLOCK_MANT BlockMnt[TOTAL_BLOCK_PER_PLN];
    FLASH_TALBE_PAGE_MANT  PageMnt[TABLE_PAGE_PER_PLN];
    FLASH_TABLE_PAGE PageData [TABLE_PAGE_PER_PLN];

}FLASH_TABLE_PLN, *pFLASH_TABLE_PLN;

typedef struct _FLASH_COM_PLN
{
    FLASH_DATA_PAGE DataPage[TOTAL_BLOCK_PER_PLN][PG_PER_BLK];
}FLASH_DATA_PLN, *pFlash_DATA_PLN;

typedef struct _FLASH_COM_PU_DEF
{
    //unsigned int Pu;
    UINT32 nBlockPerPln;
    UINT32 nPagePerPln;
    FLASH_DATA_PLN *pDataPln[PLN_PER_PU];
}FLASH_COM_PU_DEF;

typedef struct _FLASH_TABLE_PU_DEF
{
    //unsigned int Pu;
    UINT32 nBlockPerPln;
    UINT32 nPagePerPln;
    FLASH_TABLE_PLN *pTablePln[PLN_PER_PU];

}FLASH_TABLE_PU_DEF;


#define SATA_MAX_PER_SECTION    (1*1024*1024*1024)
#define SATA_LBA_DATA_LEN        4
#define SATA_LBA_PER_SECTION    (SATA_MAX_PER_SECTION / SATA_LBA_DATA_LEN)



#endif