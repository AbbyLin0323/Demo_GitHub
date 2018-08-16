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
Filename    : TEST_NfcPattBasicInsert.h
Version     : Ver 1.0
Author      : abby
Date        : 20160907
Description : compile by MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_FLASH_TEST_BASIC_PATTERN_INSERT_H_
#define _HAL_FLASH_TEST_BASIC_PATTERN_INSERT_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcCheckList.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    MACROS DECLARATION
------------------------------------------------------------------------------*/
/* FOR FLASH ADDR  */
#define TEST_PU_START               0
#ifdef SIM
#define TEST_PU_END                 1//BL_SUBSYSTEM_PU_NUM
#else
#define TEST_PU_END                 2// In 16 CE mode, actually only use PU 0 and PU 1, but mapping to CE 0~3
#endif
#define TEST_PU_NUM                 (TEST_PU_END - TEST_PU_START)

#define TEST_LUN_START              (0)
#define TEST_LUN_END                (NFC_LUN_PER_PU)
#define TEST_LUN_NUM                (TEST_LUN_END - TEST_LUN_START)

/*    Page related    */
#define TEST_PAGE_START             (0)
#define TEST_PAGE_END               (0xFF)  //TLC mode 2304, SLC mode 768
#define TEST_PAGE_NUM               (TEST_PAGE_END - TEST_PAGE_START)

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void TEST_NfcFakeBasicPatt(CHECK_LIST_BASIC_FILE *pCheckList, U8 ucPattId, BOOL bIsLast);
void TEST_NfcFakeBasicChecklist(CHECK_LIST_BASIC_FILE *pCheckList);

#endif

/*  end of this file  */
