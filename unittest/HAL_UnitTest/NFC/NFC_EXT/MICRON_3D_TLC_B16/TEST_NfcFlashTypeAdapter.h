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
Filename    : TEST_NfcFlashTypeAdapter.h
Version     : Ver 1.0
Author      : abby
Date        : 20160928
Description : compile by MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/
#ifndef BOOTLOADER
#ifndef _HAL_NFC_TEST_TLC_WRITE_H_
#define _HAL_NFC_TEST_TLC_WRITE_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "HAL_FlashChipDefine.h"
#include "TEST_NfcConfig.h"
#include "TEST_NfcExtDataCheck.h"
#include "L2_Interface.h"
#include "HAL_Dmae.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
#define TLC_PRG_ORDER_MAX           (1536)

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
/* for INTEL 3D TLC PRG CONTROL */
typedef struct _TLC_PRG_INFO_
{
#if 0
    //DW 0
    U32 bsPhyPage:16;       //1st write page num
    U32 bsPairPhyPage:16;   //2nd write page num, pair page of 1st

    //DW 1
    U32 bsPageType:8;       //LOW/HIGH/EXTRA/LOW_WITHOUT_HIGH
    U32 bsPairPageType:8;
    
    U32 bsPrgBufNum:4;      // 1 or 2
    U32 bsRsv:12;
#else
    U16 bsPhyPage;
#endif
}TLC_PRG_INFO;

typedef struct _TLC_PRG_CNT_TABLE_
{
    TLC_PRG_INFO aTlcPrgTab[TLC_PRG_ORDER_MAX];
}TLC_PRG_TABLE;

typedef struct _TLC_PHY_TO_LOGIC_PAGE_MAP_
{
    U16 aLogicPage[PG_PER_BLK]; //phy page -> logic page mapping
}TLC_P2L_PG_MAP;

void MCU1_DRAM_TEXT TEST_NfcTLCPrgTableInit(void);
void MCU1_DRAM_TEXT TEST_NfcTLCP2LMapInit(void);
U16 TEST_NfcGetPrgVirPageFromPhyPage(U16 usPhyPage, U8 ucPrgCycle, BOOL bSLCMode);
U16 TEST_NfcGetPrgOrderMax(BOOL bSLCMode);
U16 TEST_NfcGetReadOrderMax(BOOL bSLCMode);
U16 TEST_NfcGetReadPhyPageFromVirPage(U16 usVirPage, BOOL bSLCMode);
U16 TEST_NfcGetPrgPhyPageFromVirPage(U16 usVirPage, BOOL bSLCMode);
#endif

#endif// BOOTLOADER

/*  end of this file  */
