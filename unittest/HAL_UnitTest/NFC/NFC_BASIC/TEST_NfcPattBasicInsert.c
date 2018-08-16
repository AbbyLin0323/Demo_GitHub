/*******************************************************************************
* This file shall not be disclosed to any third party, in whole or in part,    *
* without prior written consent of VIA.                                        *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
Filename    : TEST_NfcPattBasicInsert.c
Version     : Ver 1.0
Author      : abby
Date        : 20160907
Description : 
Others      :
Modify      :
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcPattBasicInsert.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    FUNCTIONS
------------------------------------------------------------------------------*/
void TEST_NfcFakeBasicPatt(CHECK_LIST_BASIC_FILE *pCheckList, U8 ucPattId, BOOL bIsLast)
{ 
    BASIC_PATT_FEATURE tFeature = {0};

    tFeature.bsPuStart = TEST_PU_START;
    tFeature.bsPuEnd = TEST_PU_END;
    tFeature.bsLunStart = TEST_LUN_START;
    tFeature.bsLunEnd = TEST_LUN_END;
    tFeature.bsBlkStart = TEST_BLOCK_START;
    tFeature.bsBlkEnd = TEST_BLOCK_END;
    tFeature.bsPageStart = TEST_PAGE_START;
    tFeature.bsPageEnd = TEST_PAGE_END;
    
#ifdef TLC_MODE_TEST
    tFeature.bsTlcMode = TRUE; 
#endif

#ifdef DATA_EM_ENABLE
    tFeature.bsEMEnable = TRUE; 
#endif

#ifdef RAW_DATA_RD
    tFeature.bsRawDataRead = TRUE;
#endif

    tFeature.bsPatternId = ucPattId;
    
    pCheckList->tFlieAttr.bsFileType = BASIC_CHK_LIST;
    pCheckList->tFlieAttr.bsLastFile = bIsLast;
    
    pCheckList->tBasicPattEntry.tPattFeature = tFeature;
}

void TEST_NfcFakeBasicChecklist(CHECK_LIST_BASIC_FILE *pCheckList)
{
    U8 ucPattId = P_SINGLE_PLN;//P_ERR_INJ_DEC_REP;//P_SINGLE_PLN;

    while(ucPattId < P_RETRY)
    {
        TEST_NfcFakeBasicPatt(pCheckList++, ucPattId++, FALSE);
    } 
    TEST_NfcFakeBasicPatt(pCheckList, ucPattId, TRUE);
}


/* end of this file */
