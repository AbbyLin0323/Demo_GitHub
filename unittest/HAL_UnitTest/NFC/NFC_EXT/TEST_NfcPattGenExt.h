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
Filename    : TEST_NfcPattGenExt.h
Version     : Ver 1.0
Author      : abby
Description : compile by MCU1 and MCU2
Others      :
Modify      :
    20160903    abby    create
*******************************************************************************/
#ifndef _HAL_NFC_TEST_PATT_GEN_EXT_H_
#define _HAL_NFC_TEST_PATT_GEN_EXT_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "L3_FlashMonitor.h"
#include "FW_Event.h"
#include "L1_CacheStatus.h"
#include "TEST_NfcPattExtInsert.h"

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void TEST_NfcExtPattGen(void);
void TEST_NfcExtPattGenInit(void);
void TEST_NfcIsRemainPendingFCmd(void);

#endif

/*  end of this file  */
