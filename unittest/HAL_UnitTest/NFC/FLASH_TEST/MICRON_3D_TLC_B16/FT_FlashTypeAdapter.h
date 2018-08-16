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
Filename    : FT_FlashTypeAdapter.h
Version     : Ver 1.0
Author      : abby
Date        : 20170626
Description : 
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_FLASH_TEST_FLASH_TYPE_ADAPTER_H_
#define _HAL_FLASH_TEST_FLASH_TYPE_ADAPTER_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "BaseDef.h"

/*------------------------------------------------------------------------------
    MACRONS DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    DATA STRUCTURES DEFINITION
------------------------------------------------------------------------------*/
typedef enum _SECTION_TYPE_IN_BLOCK_
{
    SLC_SEC = 0,
    MLC_SEC,
    TLC_SEC,
    SEC_TYPE_CNT
}SECTION_TYPE;

//shared page type in one WL
typedef enum _FT_SHARED_PAGE_TYPE_
{
    LP = 0,
    UP,
    XP
}FT_SHARED_PAGE_TYPE;


/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
U16 FT_WtGetPageAddr(U16 *pPrgIndex);
U8 FT_GetSectionType(U16 usPage);
U8 FT_GetSharedPageTypeInWL(U16 usPage);
void FT_SetFeature(U8 ucPU, U8 ucAddr, U32 ulData);
void FT_FlashConfig(void);

#endif
/* end of this file */
