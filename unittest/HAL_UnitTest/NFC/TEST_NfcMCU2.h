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
Filename    : TEST_NfcMCU2.h
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : 
Others      :
Modify      :
*******************************************************************************/
#ifndef _HAL_NFC_TEST_MCU2_H_
#define _HAL_NFC_TEST_MCU2_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "COM_Memory.h"
#ifndef SIM
#include <xtensa/tie/via.h>
#include <xtensa/tie/xt_timer.h>
#else
#include "sim_flash_common.h"
#include "model_common.h"
#endif

#include "TEST_NfcPattGenExt.h"
#include "TEST_NfcPattGenBasic.h"

#include "TEST_NfcFuncBasic.h"
#include "TEST_NfcFuncExt.h"
#include "HAL_EncriptionModule.h"

/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
BOOL UT_NfcMain(void);

#endif

