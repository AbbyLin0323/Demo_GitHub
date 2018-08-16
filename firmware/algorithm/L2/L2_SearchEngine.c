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
File Name     : L2_SearchEngine.c
Version       : Ver 1.0
Author        : henryluo
Created       : 2014/10/16
Description   : L2 search engine use case
Description   :
Function List :
History       :
1.Date        : 2014/10/16
Author      : henryluo
Modification: Created file

*******************************************************************************/

#include "HAL_SearchEngine.h"
#include "HAL_FlashChipDefine.h"
#include "COM_Memory.h"
#include "L2_SearchEngine.h"
#include "L2_VBT.h"
#include "L2_PBIT.h"

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/
LOCAL MCU12_VAR_ATTR SE_SEARCH_VALUE_PARAM g_SearchEngineParam[SEARCH_CASE_MAX];

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


/*****************************************************************************
 Prototype      : HAL_SearchEngineSWInit
 Description    : init all L2 Search engine use case's parameters.
 Input          : None
 Output         : None
 Return Value   :
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/10/16
 Author       : henryluo
 Modification : Created function
 2016/05/26  Kristin modify

 *****************************************************************************/
void MCU12_DRAM_TEXT L2_SearchEngineSWInit(void)
{
    U32 ulSearchCase;

    for (ulSearchCase = 0; ulSearchCase < SEARCH_CASE_MAX; ulSearchCase++)
    {
        /* clear structure */
        COM_MemZero((U32*)&g_SearchEngineParam[ulSearchCase], sizeof(SE_SEARCH_VALUE_PARAM) / 4);

        /* set parameters for the search case */
        switch (ulSearchCase)
        {
        case FREE_ERASE_CNT_MAX:
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulItemCnt = (BLK_PER_PLN + RSV_BLK_PER_PLN);

            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulFieldMaskLow = 0xFFFF0000;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulFieldMaskHigh = 0x0;         //EraseCnt MSK
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulXorFieldMaskLow = 0x0;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulXorFieldMaskHigh = 0x380B; //0x180B;   //bFree,bError,bBackup,bLock,bTLC 
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulXorValueMaskLow = 0x0;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ulXorValueMaskHigh = 0x1;      //bFree=TRUE

            g_SearchEngineParam[FREE_ERASE_CNT_MAX].ucPitchQW = 0;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].bsSearchType = SEARCH_TYPE_MAX;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].bsCondType = 0;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].bsItemSizeDW = FALSE; //item size is QWORD
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].bsXorEn = TRUE;
            g_SearchEngineParam[FREE_ERASE_CNT_MAX].bsOTFB = FALSE;

            break;

        case FREE_ERASE_CNT_MIN:
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulItemCnt = (BLK_PER_PLN + RSV_BLK_PER_PLN);

            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulFieldMaskLow = 0xFFFF0000;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulFieldMaskHigh = 0x0;         //EraseCnt MSK
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulXorFieldMaskLow = 0x0;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulXorFieldMaskHigh = 0x380B; //0x180B;   //bFree,bError,bBackup,bLock,bTLC 
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulXorValueMaskLow = 0x0;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ulXorValueMaskHigh = 0x1;      //bFree=TRUE

            g_SearchEngineParam[FREE_ERASE_CNT_MIN].ucPitchQW = 0;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].bsSearchType = SEARCH_TYPE_MIN;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].bsCondType = 0;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].bsItemSizeDW = FALSE; //item size is QWORD
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].bsXorEn = TRUE;
            g_SearchEngineParam[FREE_ERASE_CNT_MIN].bsOTFB = FALSE;

            break;

        case BACKUP_ERASE_CNT_MAX:
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulItemCnt = (BLK_PER_PLN + RSV_BLK_PER_PLN);

            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulFieldMaskLow = 0xFFFF0000;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulFieldMaskHigh = 0x0;         //EraseCnt MSK
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulXorFieldMaskLow = 0x0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulXorFieldMaskHigh = 0x9;      //bBackup, bFree
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulXorValueMaskLow = 0x0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ulXorValueMaskHigh = 0x9;      //bFree=TRUE,bBackup=TRUE

            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].ucPitchQW = 0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].bsSearchType = SEARCH_TYPE_MAX;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].bsCondType = 0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].bsItemSizeDW = FALSE; ////item size is QWORD
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].bsXorEn = TRUE;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MAX].bsOTFB = FALSE;

            break;

        case BACKUP_ERASE_CNT_MIN:
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulItemCnt = (BLK_PER_PLN + RSV_BLK_PER_PLN);

            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulFieldMaskLow = 0xFFFF0000;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulFieldMaskHigh = 0x0;         //EraseCnt MSK
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulXorFieldMaskLow = 0x0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulXorFieldMaskHigh = 0x9;      //bBackup, bFree
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulXorValueMaskLow = 0x0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ulXorValueMaskHigh = 0x9;      //bFree=TRUE,bBackup=TRUE

            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].ucPitchQW = 0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].bsSearchType = SEARCH_TYPE_MIN;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].bsCondType = 0;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].bsItemSizeDW = FALSE; ////item size is QWORD
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].bsXorEn = TRUE;
            g_SearchEngineParam[BACKUP_ERASE_CNT_MIN].bsOTFB = FALSE;

            break;

        case BROKEN_ERASE_CNT_MAX:
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulItemCnt = (BLK_PER_PLN + RSV_BLK_PER_PLN);

            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulFieldMaskLow = 0xFFFF0000;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulFieldMaskHigh = 0x0;         //EraseCnt MSK
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulXorFieldMaskLow = 0x0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulXorFieldMaskHigh = 0x3003;   //bFree,bError,bTLC,bBroken
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulXorValueMaskLow = 0x0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ulXorValueMaskHigh = 0x2001;   //bFree=TRUE,bBroken=TRUE

            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].ucPitchQW = 0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].bsSearchType = SEARCH_TYPE_MAX;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].bsCondType = 0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].bsItemSizeDW = FALSE; ////item size is QWORD
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].bsXorEn = TRUE;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MAX].bsOTFB = FALSE;

            break;

        case BROKEN_ERASE_CNT_MIN:
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulItemCnt = (BLK_PER_PLN + RSV_BLK_PER_PLN);

            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulFieldMaskLow = 0xFFFF0000;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulFieldMaskHigh = 0x0;         //EraseCnt MSK
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulXorFieldMaskLow = 0x0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulXorFieldMaskHigh = 0x3003;   //bFree,bError,bTLC,bBroken
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulXorValueMaskLow = 0x0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ulXorValueMaskHigh = 0x2001;   //bFree=TRUE,bBroken=TRUE

            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].ucPitchQW = 0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].bsSearchType = SEARCH_TYPE_MIN;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].bsCondType = 0;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].bsItemSizeDW = FALSE; ////item size is QWORD
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].bsXorEn = TRUE;
            g_SearchEngineParam[BROKEN_ERASE_CNT_MIN].bsOTFB = FALSE;

            break;

        default:

            break;
        }
    }
}


/*****************************************************************************
 Prototype      : L2_GetSearchEnginePara
 Description    : get search engine parameters
 Input          : SE_SEARCH_CASE
 Output         : None
 Return Value   : SE_SEARCH_VALUE_PARAM *
 Calls          :
 Called By      :

 History        :
 1.Date         : 2014/10/16
 Author       : henryluo
 Modification : Created function

 *****************************************************************************/
SE_SEARCH_VALUE_PARAM* L2_GetSearchEnginePara(SE_SEARCH_CASE eSearchType)
{
    return &g_SearchEngineParam[eSearchType];
}

/*****************************************************************************
Prototype      : L2_SESearchBlock
Description    : SE search a block
Input          :
    U32 ulSearchCase: SE_SEARCH_CASE enum
    U32 *pStartItem: pointer to the first item of search range
    U8 ucTLCBlk : INVALID_2F -- not care
                  TRUE -- bTLC is TRUE
                  FALSE -- bTLC is FALSE
    SE_SEARCH_VALUE_RESULT* pSEGetResult
Output         : *pSEGetResult
Return Value   :
Calls          :
Called By      :

History        :
1.Date         : 2016/05/26
Author       : Kristin Wang
Modification : Created function

*****************************************************************************/
void L2_SESearchBlock(U32 ulSearchCase, U32 *pStartItem, U8 ucTLCBlk, SE_SEARCH_VALUE_RESULT* pSEGetResult)
{
    SE_SEARCH_VALUE_PARAM* pSEParamSet;

    if (SEARCH_CASE_MAX <= ulSearchCase)
    {
        DBG_Printf("L2_SESearchBlock: ulSearchCase = %d, ERROR!\n", ulSearchCase);
        DBG_Getch();
    }
    else
    {
        pSEParamSet = &g_SearchEngineParam[ulSearchCase];
    }

#ifdef DCACHE
    pSEParamSet->ulStartAddr = (U32)pStartItem - DRAM_HIGH_ADDR_OFFSET;
#else
    pSEParamSet->ulStartAddr = (U32)pStartItem;
#endif

    switch (ulSearchCase)
    {
    case FREE_ERASE_CNT_MAX:
    case FREE_ERASE_CNT_MIN:
    case BROKEN_ERASE_CNT_MAX:
    case BROKEN_ERASE_CNT_MIN:
        if (TRUE == ucTLCBlk)
        {
            pSEParamSet->ulXorValueMaskHigh |= 0x1000;
        }
        else
        {
            pSEParamSet->ulXorValueMaskHigh &= 0xFFFFEFFF;
        }

        break;

    default:
        break;
    }

    HAL_SESearchValue(pSEParamSet, SE_1ST_MSK_GROUP, pSEGetResult);

    return;
}
