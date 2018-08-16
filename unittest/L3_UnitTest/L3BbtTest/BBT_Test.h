/*******************************************************************************
* Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : BBT_Test.h
* Discription  :
* CreateAuthor : Steven
* CreateDate   : 2015.5.12
*===============================================================================
* Modify Record:
* 2015.5.27 Jason Refactoring
*=============================================================================*/
#ifndef _BBT_TEST_H
#define _BBT_TEST_H

typedef enum _BBT_UNITTEST_STATUS_
{
    BBT_UT_INIT = 0,

    // llf
    BBT_UT_LLF_FORMAT_BBT, // format global&local bbt
    BBT_UT_LLF_FORMAT_GBBT,// format global bbt
    BBT_UT_LLF_READ_IDB,   // read factory bbt
    BBT_UT_LLF_REDETECT_IDB,
    BBT_UT_LLF_NORMAL,     // load global bbt

    BBT_UT_BUILD,

    // boot
    BBT_UT_BOOT,

    // running...    
    BBT_UT_LOAD,
    BBT_UT_CHECK,
    BBT_UT_ADD_DUMY_BAD_BLK,
    BBT_UT_SAVE,
    BBT_UT_FINISH
}BBT_UNITTEST_STATUS;

#define BBT_UT_CHANGE_MODE_THRD (300/6)
#define BBT_UT_CHANGE_MODE_STEP (6*BBT_UT_CHANGE_MODE_THRD)

BOOL L3_BbtTest(void);
void L2_BbtTestDramAllocate(U32 *pFreeDramBase);
void L2_BbtTestAddBadBlk(U8 ucTLun, U16 usPhyBlk);

#endif

