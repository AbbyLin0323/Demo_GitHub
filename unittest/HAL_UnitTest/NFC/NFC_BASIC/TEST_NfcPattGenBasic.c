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
Filename    : TEST_NfcPattGenBasic.c
Version     : Ver 1.0
Author      : abby
Description :
Others      :
Modify      :
    20160903    abby    create
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPattGenBasic.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/

//GLOBAL U8 g_ucCheckListInx;  //max 256,check list index which has been tested in once loop

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/


/*------------------------------------------------------------------------------
Name: TEST_NfcParseBasicFile
Description: Parse check list file
Input Param:
    none
Output Param:
    none
Return Value:
Usage:
History:
    20160816    abby   create.
------------------------------------------------------------------------------*/
BASIC_PATT_ENTRY TEST_NfcParseBasicFile(CHECK_LIST_BASIC_FILE *pChkListFile)
{
     
    
    //......

    return pChkListFile->tBasicPattEntry;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcParamCheck
Description: Parse check list file
Input Param:
    none
Output Param:
    none
Return Value:
Usage:
History:
    20160816    abby   create.
------------------------------------------------------------------------------*/
U32 TEST_NfcBasicParamCheck(BASIC_PATT_ENTRY *pPattEntry)
{
#ifdef SIM 
    return 1;
#endif
    ASSERT(pPattEntry->tPattFeature.bsBlkStart < pPattEntry->tPattFeature.bsBlkEnd);
    ASSERT(pPattEntry->tPattFeature.bsPageStart < pPattEntry->tPattFeature.bsPageEnd);
    ASSERT(pPattEntry->tPattFeature.bsLunStart < pPattEntry->tPattFeature.bsLunEnd);
    ASSERT(pPattEntry->tPattFeature.bsPatternId < P_TYPE_CNT);
    
    //ASSERT(TRUE == pPattEntry->tPattFeature.bsTlcMode);

    //.......
    return SUCCESS;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcBasicSharedDSRAM1MemMap
Description: Assign address in DSRAM1 for g_pPattQ and g_pPattQDptr
             g_pPattQ -> g_ptFCmdReq: multi use FCMDQ
Input Param:
Output Param:
Return Value:
Usage:
History:
    20160905    abby   create.
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT TEST_NfcBasicSharedDSRAM1MemMap(U32 *pFreeSharedSRAM1Base)
{
    U32 ulFreeBase = *pFreeSharedSRAM1Base;
    COM_MemAddr16DWAlign(&ulFreeBase);
    
    // Part1:
    g_pPattQ = (BASIC_PATTQ *)ulFreeBase;//g_ptFCmdReq;
    //ulFreeBase = (U32)g_pPattQ;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(BASIC_PATTQ));
    COM_MemAddr16DWAlign(&ulFreeBase);

    // Part2:
    g_pPattQDptr = (BASIC_PATTQ_DPTR *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(BASIC_PATTQ_DPTR));
    COM_MemAddr16DWAlign(&ulFreeBase);

    g_pPattStsQ = (BASIC_PATT_STSQ *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(BASIC_PATT_STSQ));
    COM_MemAddr16DWAlign(&ulFreeBase);

    ASSERT(ulFreeBase - DSRAM1_MCU12_SHARE_BASE <= DSRAM1_MCU12_SHARE_SIZE);
    
    DBG_Printf("NFC BASIC UT Alloc shared SRAM1 0x%x B, Rsvd 0x%x B.\n"
        , ulFreeBase - *pFreeSharedSRAM1Base, DSRAM1_MCU12_SHARE_SIZE - (ulFreeBase - *pFreeSharedSRAM1Base));

    *pFreeSharedSRAM1Base = ulFreeBase;
    
    return;
}

void MCU12_DRAM_TEXT TEST_NfcBasicSharedMemMap(SUBSYSTEM_MEM_BASE * pFreeMemBase)
{    
    TEST_NfcBasicSharedDSRAM1MemMap(&pFreeMemBase->ulFreeSRAM1SharedBase);

    /* checklist files allocate in MCU1 dram */
    if (MCU1_ID == HAL_GetMcuId())
    {
        TEST_NfcCheckListAllocDram(&pFreeMemBase->ulDRAMBase);
    }
}

void MCU1_DRAM_TEXT TEST_NfcBasicPattGenInit(void)
{
    /*  init LOCAL FCMDQ related address and structure    */
    TEST_NfcBasicSharedMemMap(&g_FreeMemBase);

    /*  pattern Q init    */
    TEST_NfcPattQInit();

    /* TLC mode flag init */
#ifdef TLC_MODE_TEST
    g_bTlcMode = TRUE;
#else
    g_bTlcMode = FALSE;
#endif

    DBG_Printf("Basic Pattern Gen Init Done!\n\n");
}

void TEST_NfcBasicPattGen(void)
{
    U8 ucWptr;
    BASIC_PATT_ENTRY *pBasicPatt;

    ucWptr = TEST_NfcPattQGetWptr();

    while(TRUE != TEST_NfcPattQIsWptrFree(ucWptr))
    {
        ;
    }
    
    pBasicPatt = TEST_NfcPattQAllocEntry(ucWptr);
    
    *pBasicPatt = TEST_NfcParseBasicFile((CHECK_LIST_BASIC_FILE*)g_pCheckListPtr);
    
    TEST_NfcBasicParamCheck(pBasicPatt);

    TEST_NfcPattQPushEntry(ucWptr);

    return;
}

/* end of this file  */
