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
Filename    : TEST_NfcPattGenBasic.h
Version     : Ver 1.0
Author      : abby
Description :
Others      :
Modify      :
    20160903    abby    create
*******************************************************************************/
#ifndef _HAL_NFC_TEST_PATT_GEN_BASIC_H_
#define _HAL_NFC_TEST_PATT_GEN_BASIC_H_

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcCheckList.h"
#include "TEST_NfcPattBasicInsert.h"

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    EXTERN VARIABLES DECLARATION
------------------------------------------------------------------------------*/
/*  NFC feature control, evolution to FCMD */
extern GLOBAL BOOL g_bSsuEn;
extern GLOBAL BOOL g_bCacheStsEn;
extern GLOBAL BOOL g_bSsu0DramEn;
extern GLOBAL BOOL g_bSsu1DramEn;
extern GLOBAL BOOL g_bLbaChk;
extern GLOBAL BOOL g_bRedOntf;
extern GLOBAL BOOL g_bForceRetryEn;
extern GLOBAL U8   g_ucEntryIndex;


/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void TEST_NfcBasicPattGenInit(void);
void TEST_NfcBasicPattGen(void);

#endif

/* end of this file  */
