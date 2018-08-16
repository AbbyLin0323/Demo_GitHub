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
Filename    : TEST_NfcPattQ.c
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : compile by both MCU1 and MCU2
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPattQ.h"

/*------------------------------------------------------------------------------
    VIARABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR BASIC_PATTQ *g_pPattQ;    
GLOBAL MCU12_VAR_ATTR volatile BASIC_PATT_STSQ *g_pPattStsQ;
GLOBAL MCU12_VAR_ATTR volatile BASIC_PATTQ_DPTR *g_pPattQDptr;

LOCAL MCU1_DRAM_TEXT void TEST_NfcPattQEntryInit(void)
{
    COM_MemZero((U32*)g_pPattQ, sizeof(BASIC_PATTQ)/sizeof(U32));
}

MCU12_DRAM_TEXT void TEST_NfcPattQStsInit(void)
{
    U32 ucWptr;
    U32 ulMcuId;

    ulMcuId = HAL_GetMcuId();
    
    for (ucWptr = 0; ucWptr < PATTQ_DEPTH; ucWptr++)
    {
        g_pPattStsQ->aPattStsQ[ucWptr] = (MCU1_ID == ulMcuId)? INVALID_2F : BASIC_PATTQ_STS_INIT;
    }
}

LOCAL MCU12_DRAM_TEXT void TEST_NfcPattQDptrInit(void)
{
    g_pPattQDptr->ucWptr = 0;
    g_pPattQDptr->ucRptr = 0;
}

void MCU1_DRAM_TEXT TEST_NfcPattQInit(void)
{
    TEST_NfcPattQEntryInit();
    TEST_NfcPattQStsInit();
    TEST_NfcPattQDptrInit();
}

U8 TEST_NfcPattQGetWptr(void)
{
    return g_pPattQDptr->ucWptr;
}

LOCAL void TEST_NfcPattQSetWptr(U8 ucWptr)
{
    g_pPattQDptr->ucWptr = ucWptr;
}

LOCAL U8 TEST_NfcPattQGetRptr(void)
{
    return g_pPattQDptr->ucRptr;
}

LOCAL void TEST_NfcPattQSetRptr(U8 ucRptr)
{
    g_pPattQDptr->ucRptr = ucRptr;
}

U32 TEST_NfcPattQGetStsAddr(U8 ucWptr)
{
    return (U32)&g_pPattStsQ->aPattStsQ[ucWptr];
}

LOCAL U8 TEST_NfcPattQGetSts(U8 ucWptr)
{
    return g_pPattStsQ->aPattStsQ[ucWptr];
}

LOCAL void TEST_NfcPattQSetSts(U8 ucPattId, U8 ucPattQSts)
{
    g_pPattStsQ->aPattStsQ[ucPattId] = ucPattQSts;
}

LOCAL BASIC_PATT_ENTRY *TEST_NfcPattQGetEntry(U8 ucWptr)
{
    return &g_pPattQ->aPattQ[ucWptr];
}

BOOL TEST_NfcPattQIsWptrFree(U8 ucWptr)
{
    BOOL bIsFree = FALSE;
    
    if (BASIC_PATTQ_STS_INIT == TEST_NfcPattQGetSts(ucWptr))
    {
        bIsFree = TRUE;
    }
    
    return bIsFree;
}

BOOL TEST_NfcPattQIsNotFull(void)
{
    U8 ucWptr, ucRptr;

    ucWptr = TEST_NfcPattQGetWptr();
    ucRptr = TEST_NfcPattQGetRptr();

    if((ucWptr % PATTQ_DEPTH) == (ucRptr - 1))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL TEST_NfcPattQIsPushed(void)
{
    U8 ucRptr;
    BOOL bIsPush = FALSE;

    ucRptr = TEST_NfcPattQGetRptr();
    if (BASIC_PATTQ_STS_PUSH == TEST_NfcPattQGetSts(ucRptr))
    {
        bIsPush = TRUE;
    }

    return bIsPush;
}

BASIC_PATT_ENTRY *TEST_NfcPattQAllocEntry(U8 ucWptr)
{
    BASIC_PATT_ENTRY *pPattEntry;
    
    pPattEntry = TEST_NfcPattQGetEntry(ucWptr);
    COM_MemZero((U32*)pPattEntry, sizeof(BASIC_PATT_ENTRY)/sizeof(U32));

    ASSERT(BASIC_PATTQ_STS_INIT == TEST_NfcPattQGetSts(ucWptr));
    TEST_NfcPattQSetSts(ucWptr, BASIC_PATTQ_STS_ALLOC);    

    return pPattEntry;
}

void TEST_NfcPattQPushEntry(U8 ucWptr)
{
    ASSERT(BASIC_PATTQ_STS_ALLOC == TEST_NfcPattQGetSts(ucWptr));
    TEST_NfcPattQSetSts(ucWptr, BASIC_PATTQ_STS_PUSH);
    
    TEST_NfcPattQSetWptr((ucWptr + 1)%PATTQ_DEPTH);

    return;
}

BASIC_PATT_ENTRY *TEST_NfcPattQPopEntry(void)
{
    U8 ucRptr;
    BASIC_PATT_ENTRY *pPattEntry;
    
    ucRptr = TEST_NfcPattQGetRptr();
    pPattEntry = TEST_NfcPattQGetEntry(ucRptr);

    ASSERT(BASIC_PATTQ_STS_PUSH == TEST_NfcPattQGetSts(ucRptr));
    TEST_NfcPattQSetSts(ucRptr, BASIC_PATTQ_STS_POP);

    TEST_NfcPattQSetRptr((ucRptr + 1) % PATTQ_DEPTH);    

    return pPattEntry;
}

void TEST_NfcPattQRecycleEntry(void)
{
    U8 ucRptr;
    ucRptr = TEST_NfcPattQGetRptr();
    ucRptr = (ucRptr + PATTQ_DEPTH - 1)%PATTQ_DEPTH;

    TEST_NfcPattQSetSts(ucRptr, BASIC_PATTQ_STS_INIT);
}
    
/*  end of this file  */
