/****************************************************************************
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
*****************************************************************************
 * File Name    : TEST_NfcOption.c
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "HAL_MemoryMap.h"
#include "TEST_NfcOption.h"

volatile NFC_REG_OPTION* const pNfcRegOption = (volatile NFC_REG_OPTION*)(REG_BASE_NDC + 0x2a0);

U32 TEST_NfcGetPuNum(void)
{
#ifdef COSIM
    return pNfcRegOption->bsPuNum;
#else
    return TEST_PU_NUM;
#endif
}

U32 TEST_NfcGetLunNum(void)
{
#ifdef COSIM
    return pNfcRegOption->bsLunNum;
#else
    return 1;
#endif
}

U32 TEST_NfcGetPageNum(void)
{
#ifdef COSIM
    return pNfcRegOption->bsPageNum;
#else
    return TEST_PAGE_NUM;
#endif
}

U32 TEST_NfcGetBlockStart(void)
{
#ifdef COSIM
    return 0;    // cosim start from block 0
#else
    return TEST_BLOCK_START;
#endif
}

U32 TEST_NfcGetBlockEnd(void)
{
#ifdef COSIM
    return pNfcRegOption->bsBlockNum - 1;    // cosim end at block number - 1
#else
    return TEST_BLOCK_END;
#endif
}

BOOL TEST_NfcDataCheckEn(void)
{
#ifdef COSIM
    return (BOOL)pNfcRegOption->bsDataCheckEn;
#else
    return TEST_DATA_CHECK_EN;
#endif
}

BOOL TEST_NfcDxqEn(void)
{
#ifdef COSIM
    return pNfcRegOption->bsDxqModeEn;
#else
    return TEST_DRQDWQ_EN;
#endif
}

U16 TEST_NfcGetSecStart(void)
{
#ifdef COSIM
    return pNfcRegOption->usSecStart;
#else
    return TEST_SEC_START;
#endif
}

U16 TEST_NfcGetSecLen(void)
{
#ifdef COSIM
    return pNfcRegOption->usSecLen;
#else
    return TEST_SEC_LEN;
#endif
}

U8 TEST_NfcGetPatternId(void)
{
#ifdef COSIM
    return pNfcRegOption->bsPatternId;
#else
    return 0;
#endif
}

BOOL TEST_NfcDramInitEn(void)
{
#ifdef COSIM
    return (TRUE == pNfcRegOption->bsDramInitDisable)?FALSE:TRUE;
#else
    return FALSE;
#endif
}

BOOL TEST_NfcSsuCsEn(void)
{
#ifdef COSIM
    return pNfcRegOption->bsSsuCsEn;
#else
    return TEST_SSU_CS_EN;
#endif
}










