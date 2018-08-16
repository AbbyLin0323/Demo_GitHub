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
Filename    : FT_BasicFunc.h
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : 
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_FLASH_TEST_BASIC_FUNC_H_
#define _HAL_FLASH_TEST_BASIC_FUNC_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "BaseDef.h"
#include "Disk_Config.h"
#include "HAL_ParamTable.h"

/*------------------------------------------------------------------------------
    DATA STRUCTURES DEFINITION
------------------------------------------------------------------------------*/

typedef struct _FT_SPARE_AREA_
{
    /* Common Info : 3 DW */
    U32 ulTimeStamp;
    U32 ulTargetOffsetTS;

    U32 bsVirBlockAddr : 16;
    U32 bcBlockType : 8;
    U32 bcPageType : 8;

#ifdef SIM
    U32 ulMCUId;
#endif

    /* Data Block */
    U32 aCurrLPN[LPN_PER_BUF];
}FT_SPARE_AREA;

typedef struct _FT_DATA_BUFF_
{
    //volatile U32 *pBuffAddr[PG_PER_WL];
    U32 *pBuffAddr[PG_PER_WL];
}FT_DATA_BUFF;

/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    MACRONS DECLARATION
------------------------------------------------------------------------------*/
/*  DATA Buffer Allocation */
#define FT_PG_NUM_PER_PRG              (PGADDR_PER_PRG * INTRPG_PER_PGADDR)
#define FT_PG_MAX_PER_BLK              (PG_PER_BLK)   

//#define DATA_BUFF_START_BASE           ((*g_pMCU1DramEndBase - TEST_START_ADDRESS))

#define DW_SZ_PER_SEC                  (1 << (SEC_SZ_BITS - 2))

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void FT_DataDRAMMemMap(void);

void FT_WriteOnePage(FLASH_ADDR *pFlashAddr);
U32 FT_ReadOnePage(U8 ucPageType, U8 ucSecStart, U16 usSecLen, FLASH_ADDR *pFlashAddr);
U32 FT_ReadLowPage(FLASH_ADDR *pFlashAddr);

BOOL FT_EraseOneBlk(FLASH_ADDR *pFlashAddr);
void FT_WriteOneBlk(U16 usStartPage, U16 usEndPage, FLASH_ADDR *pFlashAddr);
void FT_ReadOneBlk(U16 usStartPage, U16 usEndPage, FLASH_ADDR *pFlashAddr);

void FT_EraseSuspendOneBlk(FLASH_ADDR *pFlashAddr);
void FT_EraseResumeOneBlk(FLASH_ADDR *pFlashAddr);
void FT_WriteSuspendOnePage(FLASH_ADDR *pFlashAddr);
void FT_WriteResumeOnePage(FLASH_ADDR *pFlashAddr);

#endif
/* end of this file */
