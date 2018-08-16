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
 * File Name    : TEST_SearchEngine.c
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "HAL_SearchEngine.h"
#include "TEST_SearchEngine.h"
#include "COM_Memory.h"
#include "HAL_GLBReg.h"

LOCAL U32 l_ulTimer;
LOCAL VBT *l_pVBT;
LOCAL TEST_SE_OVERLAP_LIST *l_pSearchList;
LOCAL SE_SEARCH_VALUE_PARAM l_aSECaseParam[SEARCH_CASE_MAX];

void TEST_SECaseParamInit(void)
{
    U8 ucSearchCase;

    for(ucSearchCase =0 ; ucSearchCase < SEARCH_CASE_MAX; ucSearchCase++)
    {
        COM_MemSet( (U32 *)&(l_aSECaseParam[ucSearchCase]), sizeof(SE_SEARCH_VALUE_PARAM)/sizeof(U32), 0 );

        switch(ucSearchCase)
        {
            case ERASE_CNT_MAX:
                /*search EraseCnt max when bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=FALSE */
                l_aSECaseParam[ERASE_CNT_MAX].ulFieldMaskLow = 0xFFFF0000;
                l_aSECaseParam[ERASE_CNT_MAX].ulFieldMaskHigh = 0x0;//EraseCnt MSK

                l_aSECaseParam[ERASE_CNT_MAX].bsXorEn = TRUE;
                l_aSECaseParam[ERASE_CNT_MAX].ulXorFieldMaskLow = 0x0;
                l_aSECaseParam[ERASE_CNT_MAX].ulXorFieldMaskHigh = 0x0000000F;
                l_aSECaseParam[ERASE_CNT_MAX].ulXorValueMaskLow = 0x0;
                l_aSECaseParam[ERASE_CNT_MAX].ulXorValueMaskHigh = 0x1;//bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=FALSE

                l_aSECaseParam[ERASE_CNT_MAX].bsSearchType = SEARCH_TYPE_MAX;

                l_aSECaseParam[ERASE_CNT_MAX].ulItemCnt = (BLK_PER_CE + RSVD_BLK_PER_CE);

                break;

            case ERASE_CNT_MIN:
                /*search EraseCnt max when bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=FALSE */
                l_aSECaseParam[ERASE_CNT_MIN].ulFieldMaskLow = 0xFFFF0000;
                l_aSECaseParam[ERASE_CNT_MIN].ulFieldMaskHigh = 0x0;//EraseCnt MSK

                l_aSECaseParam[ERASE_CNT_MAX].bsXorEn = TRUE;
                l_aSECaseParam[ERASE_CNT_MIN].ulXorFieldMaskLow = 0x0;
                l_aSECaseParam[ERASE_CNT_MIN].ulXorFieldMaskHigh = 0x0000000F;
                l_aSECaseParam[ERASE_CNT_MIN].ulXorValueMaskLow = 0x0;
                l_aSECaseParam[ERASE_CNT_MIN].ulXorValueMaskHigh = 0x1;//bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=FALSE

                l_aSECaseParam[ERASE_CNT_MAX].bsSearchType = SEARCH_TYPE_MIN;

                l_aSECaseParam[ERASE_CNT_MAX].ulItemCnt = (BLK_PER_CE + RSVD_BLK_PER_CE);

                break;

            case BACKUP_ERASE_CNT_MAX:
                /*search EraseCnt max when bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=FALSE */
                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulFieldMaskLow = 0xFFFF0000;
                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulFieldMaskHigh = 0x0;//EraseCnt MSK

                l_aSECaseParam[ERASE_CNT_MAX].bsXorEn = TRUE;
                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulXorFieldMaskLow = 0x0;
                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulXorFieldMaskHigh = 0x0000000F;
                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulXorValueMaskLow = 0x0;
                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulXorValueMaskHigh = 0x9;//bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=TRUE

                l_aSECaseParam[ERASE_CNT_MAX].bsSearchType = SEARCH_TYPE_MAX;

                l_aSECaseParam[BACKUP_ERASE_CNT_MAX].ulItemCnt = (BLK_PER_CE + RSVD_BLK_PER_CE);

                break;

            case BACKUP_ERASE_CNT_MIN:
                /*search EraseCnt max when bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=FALSE */
                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulFieldMaskLow = 0xFFFF0000;
                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulFieldMaskHigh = 0x0; //EraseCnt MSK

                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].bsXorEn = TRUE;
                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulXorFieldMaskLow = 0x0;
                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulXorFieldMaskHigh = 0x0000000F;
                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulXorValueMaskLow = 0x0;
                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulXorValueMaskHigh = 0x9;//bFree=TRUE,bError=FALSE,bAllocated=FALSE,bBackup=TRUE

                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].bsSearchType = SEARCH_TYPE_MIN;

                l_aSECaseParam[BACKUP_ERASE_CNT_MIN].ulItemCnt = (BLK_PER_CE + RSVD_BLK_PER_CE);

                break;

            case DIRTY_CNT_MAX:
                /*search DirtyLPNCnt max when StripID=0x1234,Target(high bit) =1 */
                l_aSECaseParam[DIRTY_CNT_MAX].ulFieldMaskLow = 0x0;
                l_aSECaseParam[DIRTY_CNT_MAX].ulFieldMaskHigh = 0x0000FFFF; //DirtyLPNCnt MSK

                l_aSECaseParam[DIRTY_CNT_MAX].bsXorEn = TRUE;
                l_aSECaseParam[DIRTY_CNT_MAX].ulXorFieldMaskLow = 0xFFFF0000;
                l_aSECaseParam[DIRTY_CNT_MAX].ulXorFieldMaskHigh = 0x00030000; //Target Msk
                l_aSECaseParam[DIRTY_CNT_MAX].ulXorValueMaskLow = 0;
                l_aSECaseParam[DIRTY_CNT_MAX].ulXorValueMaskHigh = 0x00010000; //Target=0x1

                l_aSECaseParam[DIRTY_CNT_MAX].bsSearchType = SEARCH_TYPE_MAX;

                l_aSECaseParam[DIRTY_CNT_MAX].ulItemCnt = BLK_PER_CE;

                break;

            case DIRTY_CNT_MIN:
                /*search DirtyLPNCnt max when StripID=0x1234,Target=0x1 */
                l_aSECaseParam[DIRTY_CNT_MIN].ulFieldMaskLow = 0x0;
                l_aSECaseParam[DIRTY_CNT_MIN].ulFieldMaskHigh = 0x0000FFFF; //DirtyLPNCnt MSK

                l_aSECaseParam[DIRTY_CNT_MIN].bsXorEn = TRUE;
                l_aSECaseParam[DIRTY_CNT_MIN].ulXorFieldMaskLow = 0xFFFF0000;
                l_aSECaseParam[DIRTY_CNT_MIN].ulXorFieldMaskHigh = 0x00030000; //Target Msk
                l_aSECaseParam[DIRTY_CNT_MIN].ulXorValueMaskLow = 0;
                l_aSECaseParam[DIRTY_CNT_MIN].ulXorValueMaskHigh = 0x00010000; //Target=0x1

                l_aSECaseParam[DIRTY_CNT_MIN].bsSearchType = SEARCH_TYPE_MIN;

                l_aSECaseParam[DIRTY_CNT_MIN].ulItemCnt = BLK_PER_CE;

                break;

                break;
        }//switch(ucSearchCase)
    }//for(ucSearchCase =0 ; ucSearchCase < SEARCH_CASE_MAX; ucSearchCase++)
}


void TEST_SEVBTInit(BOOL bOTFB)
{
    U32 i,j,k;
    VBT_ENTRY tVbtEntryTemp;

    if ( TRUE == bOTFB )
    {
        l_pVBT = (VBT *)OTFB_RED_DATA_MCU1_BASE;
    }
    else
    {
        l_pVBT = (VBT *)DRAM_DATA_BUFF_MCU1_BASE;
    }

    for( i = 0; i < CE_NUM; i++)
    {
        for(j=0;j<BLK_PER_CE;j++)
        {
            k = i*BLK_PER_CE + j;
            tVbtEntryTemp.BlockType = 0x3;
            tVbtEntryTemp.Rsvd = 0xAB;
            tVbtEntryTemp.PhysicslBlockAddr = 0xFFFF;
            tVbtEntryTemp.StripID = i;
            if(k % 4 == 0)
            {
                tVbtEntryTemp.Target = 0x0;
                tVbtEntryTemp.DirtyLPNCnt = ((k+1-l_ulTimer)*3456)%(1<<16);
            }
            else if(k % 4 == 1)
            {
                tVbtEntryTemp.Target = 0x1;
                tVbtEntryTemp.DirtyLPNCnt =((k+7+l_ulTimer)*3456)%(1<<16);
            }
            else if(k % 4 == 2)
            {
                tVbtEntryTemp.Target = 0x2;
                tVbtEntryTemp.DirtyLPNCnt = ((k+5+l_ulTimer)*3456)%(1<<16);
            }
            else if(k % 4 == 3)
            {
                tVbtEntryTemp.Target = 0x1;                
                tVbtEntryTemp.DirtyLPNCnt = ((k+4+l_ulTimer)*3456)%(1<<16);
            }
            *((U32*)&l_pVBT->m_VBT[i][j]) = *((U32*)&tVbtEntryTemp);
            *((U32*)&l_pVBT->m_VBT[i][j] + 1) = *((U32*)&tVbtEntryTemp + 1);
        }
    }//for( i = 0; i < CE_NUM; i++)

    return;
}

void TEST_SEVbtMax(BOOL bOTFB)
{
    SE_SEARCH_VALUE_PARAM* pSEParam;
    SE_SEARCH_VALUE_RESULT tSEResult;
    U16 usStripeId;
    U32 ulSWMaxValueTemp = 0;
    U32 ulSWRtnValLow;
    U32 ulSWRtnValHigh;
    U32 ulSWRtnIndex;
    U32 i,j,k; 

    usStripeId = l_ulTimer%CE_NUM;//( 1 + g_ulLoopCnt ) % (1 << 16 );//can modify

    for( i = 0; i < CE_NUM; i++ )
    {
        for( j = 0; j < BLK_PER_CE; j++)
        {

            if( l_pVBT->m_VBT[i][j].StripID != usStripeId )
            {
                continue;
            }
            if( (l_pVBT->m_VBT[i][j].Target & 0x3) != 0x1 )
            {
                continue;
            }
            if( ulSWMaxValueTemp < (l_pVBT->m_VBT[i][j].DirtyLPNCnt) )//find max
            {
                ulSWMaxValueTemp = l_pVBT->m_VBT[i][j].DirtyLPNCnt;
                k = i*BLK_PER_CE +j;
                ulSWRtnIndex = k;
            }        
        }
    }
    ulSWRtnValLow = *((U32*)&l_pVBT->m_VBT[ulSWRtnIndex/BLK_PER_CE][ulSWRtnIndex%BLK_PER_CE]);
    ulSWRtnValHigh = *((U32*)&l_pVBT->m_VBT[ulSWRtnIndex/BLK_PER_CE][ulSWRtnIndex%BLK_PER_CE] + 1);

    pSEParam = &l_aSECaseParam[DIRTY_CNT_MAX];
    pSEParam->ulXorValueMaskLow = ((U32)usStripeId << 16);
    pSEParam->ulStartAddr = (U32)&l_pVBT->m_VBT[0][0];
    pSEParam->ulItemCnt = CE_NUM*BLK_PER_CE;
    pSEParam->bsOTFB = bOTFB;

    HAL_SESearchValue(pSEParam, &tSEResult); 

    //DBG_Printf("SW find max: Index = %d£¬ ValueLeft = 0x%x, ValueRight = 0x%x\n", ulSWRtnIndex, ulSWRtnValLow, ulSWRtnValHigh);

    if( (ulSWRtnIndex != tSEResult.ulIndex)
        || (ulSWRtnValLow != tSEResult.ulRtnValueLow)
        || (ulSWRtnValHigh != tSEResult.ulRtnValueHigh) )
    {
        DBG_Printf("TEST_SEVbtMax: 12345678, 0x%x\n",l_ulTimer);
        DBG_Getch();
    }

    DBG_Printf("TEST_SEVbtMax: 0x%x,index = 0x%x\n",l_ulTimer, tSEResult.ulIndex);

    return;
}

void TEST_SEVbtMin(BOOL bOTFB)
{
    SE_SEARCH_VALUE_PARAM* pSEParam;
    SE_SEARCH_VALUE_RESULT tSEResult;
    U16 usStripeId;
    U32 ulSWMinValueTemp = 0xFFFFFFFF;
    U32 ulSWRtnValLow;
    U32 ulSWRtnValHigh;
    U32 ulSWRtnIndex;
    U32 i,j,k;

    usStripeId = l_ulTimer%CE_NUM;//( 1 + g_ulLoopCnt ) % (1 << 16 );//can modify

    for( i = 0; i < CE_NUM; i++ )
    {
        for( j = 0;j < BLK_PER_CE; j++ )
        {

            if( l_pVBT->m_VBT[i][j].StripID != usStripeId )
            {
                continue;
            }
            if( (l_pVBT->m_VBT[i][j].Target & 0x3) != 0x1 )
            {
                continue;
            }
            if( ulSWMinValueTemp > (l_pVBT->m_VBT[i][j].DirtyLPNCnt) )//find min
            {
                ulSWMinValueTemp = l_pVBT->m_VBT[i][j].DirtyLPNCnt;
                k = i*BLK_PER_CE +j;
                ulSWRtnIndex = k;
            }        
        }
    }
    ulSWRtnValLow = *((U32*)&l_pVBT->m_VBT[ulSWRtnIndex/BLK_PER_CE][ulSWRtnIndex%BLK_PER_CE]);
    ulSWRtnValHigh = *((U32*)&l_pVBT->m_VBT[ulSWRtnIndex/BLK_PER_CE][ulSWRtnIndex%BLK_PER_CE] + 1);
    //DBG_Printf("SW find min: Index = %d£¬ ValueLeft = 0x%x, ValueRight = 0x%x\n", ulSWRtnIndex, ulSWRtnValLow, ulSWRtnValHigh);

    pSEParam = &l_aSECaseParam[DIRTY_CNT_MIN];
    pSEParam->ulXorValueMaskLow = ((U32)usStripeId << 16);
    pSEParam->ulStartAddr = (U32)&l_pVBT->m_VBT[0][0];
    pSEParam->ulItemCnt= CE_NUM*BLK_PER_CE;
    pSEParam->bsOTFB = bOTFB;

    HAL_SESearchValue(pSEParam, &tSEResult); 

    if( (ulSWRtnIndex != tSEResult.ulIndex)
        || (ulSWRtnValLow != tSEResult.ulRtnValueLow)
        || (ulSWRtnValHigh != tSEResult.ulRtnValueHigh) )
    {
        DBG_Printf("TEST_SEVbtMin: 12345678, 0x%x\n",l_ulTimer);
        DBG_Getch();
    }

    DBG_Printf("TEST_SEVbtMin: 0x%x,index = 0x%x\n",l_ulTimer, tSEResult.ulIndex);
    return;
}

void TEST_SEVbtSame(BOOL bOTFB)
{
    SE_SEARCH_VALUE_PARAM tSEParam;
    SE_SEARCH_VALUE_RESULT tSEResult;
    U16 usStripeId;
    U32 ulDirtyCntToSearch = (((2+4*(l_ulTimer%BLK_PER_CE)) + 5 + l_ulTimer)*3456)%(0x1<<16);
    U16 usSWDirtyCnt;
    U32 ulSWRtnValLow;
    U32 ulSWRtnValHigh;
    U32 ulSWRtnIndex;
    U32 i,j,k;
    BOOL bSWFind = FALSE;

    usStripeId = l_ulTimer % CE_NUM;

    for( i = 0; i < CE_NUM; i++ )
    {
        for( j = 0; j < BLK_PER_CE; j++ )
        {
            k = i*BLK_PER_CE +j;
            if( ( l_pVBT->m_VBT[i][j].StripID == usStripeId )
                && ( ( l_pVBT->m_VBT[i][j].Target & 0x2 ) == 0x2 ) )
            {
                if(ulDirtyCntToSearch== l_pVBT->m_VBT[i][j].DirtyLPNCnt)//find same
                {
                    usSWDirtyCnt = l_pVBT->m_VBT[i][j].DirtyLPNCnt;
                    ulSWRtnIndex = k;
                    bSWFind = TRUE;
                    //goto search_end;
                }
                
            }
        }
    }    
search_end:
    if ( TRUE == bSWFind )
    {
        ulSWRtnValLow = *((U32*)&l_pVBT->m_VBT[ulSWRtnIndex/BLK_PER_CE][ulSWRtnIndex%BLK_PER_CE]);
        ulSWRtnValHigh = *((U32*)&l_pVBT->m_VBT[ulSWRtnIndex/BLK_PER_CE][ulSWRtnIndex%BLK_PER_CE] + 1);
    }

    COM_MemSet( (U32 *)&tSEParam, sizeof(SE_SEARCH_VALUE_PARAM)/sizeof(U32), 0);

    tSEParam.ulCondMaskLow = 0x0;
    tSEParam.ulCondMaskHigh = 0x00020000; //Target(high bit) MSK
    tSEParam.ulFieldMaskLow = 0x0;
    tSEParam.ulFieldMaskHigh = 0x0000FFFF; //DirtyLPNCnt MSK
    tSEParam.ulXorFieldMaskLow = 0xFFFF0000;
    tSEParam.ulXorFieldMaskHigh = 0x0;
    tSEParam.ulXorValueMaskLow = ((U32)usStripeId << 16);
    tSEParam.ulXorValueMaskHigh = 0x0;

    tSEParam.bsSearchType = SEARCH_TYPE_SAME;
    tSEParam.bsCondType = 1;
    tSEParam.bsXorEn = TRUE;

    tSEParam.ulStartAddr = (U32)&l_pVBT->m_VBT[0][0];
    tSEParam.ulItemCnt = CE_NUM*BLK_PER_CE;    
    tSEParam.ucPitchQW = 0;
    
    tSEParam.ulValToSearchLow = 0x0;
    tSEParam.ulValToSearchHigh  = ulDirtyCntToSearch;
    tSEParam.bsOTFB = bOTFB;

    HAL_SESearchValue(&tSEParam, &tSEResult);

    if(tSEResult.bsFind == TRUE)
    {
        if( (ulSWRtnIndex != tSEResult.ulIndex)
            || (ulSWRtnValLow != tSEResult.ulRtnValueLow)
            || (ulSWRtnValHigh != tSEResult.ulRtnValueHigh)
            || ( (tSEResult.ulRtnValueHigh & 0xFFFF) != ulDirtyCntToSearch ) )
        {
            DBG_Printf("TEST_SEVbtSame: 12345678, 0x%x\n",l_ulTimer);
            DBG_Getch();
        }

        DBG_Printf("TEST_SEVbtSame: 0x%x,index = 0x%x\n",l_ulTimer, tSEResult.ulIndex);
    }
    else if ( FALSE == bSWFind )
    {
        DBG_Printf("TEST_SEVbtSame: 0x%x,not find\n",l_ulTimer);
    }
    else 
    {
        DBG_Printf("TEST_SEVbtSame: 87654321, 0x%x\n",l_ulTimer);
        DBG_Getch();
    }

    return;
}

void TEST_SESearchOverlap(BOOL bOTFB)
{    
    SE_SEARCH_OVERLAP_ENTRY tOverlapEntry;
    U32 ulKeyStartVal;
    U32 ulKeyLen;
    U32 ulItemIndex;
    U8 ucBitMapDWID;
    U8 ucBitMapBitID;    
    U32 ulFactor = ( (l_ulTimer*l_ulTimer)/7 + l_ulTimer%7 )%0x100;

    if ( TRUE == bOTFB )
    {
        l_pSearchList = (TEST_SE_OVERLAP_LIST *)OTFB_RED_DATA_MCU1_BASE;
    }
    else
    {
        l_pSearchList = (TEST_SE_OVERLAP_LIST *)DRAM_DATA_BUFF_MCU1_BASE;
    }

    ulKeyStartVal = ulFactor * 0x100000 *  TEST_SE_LIST_LEN
        + (ulFactor * ulFactor + 1) * 0x1000 * TEST_SE_LIST_LEN
        + (ulFactor * ulFactor * ulFactor + ulFactor + 1) * TEST_SE_LIST_LEN;
    ulKeyLen = ((ulFactor * ulFactor + 1) * TEST_SE_LIST_LEN + ulFactor) % 0xFFFF + 1;

    for ( ulItemIndex = 0; ulItemIndex < TEST_SE_LIST_LEN; ulItemIndex++ )
    {
        if ( ulItemIndex == l_ulTimer % TEST_SE_LIST_LEN )
        {
            switch( l_ulTimer%5 )
            {
            case 0:// ss/ks-se/ke
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal;
                l_pSearchList->List[ulItemIndex].ulAccessLen = ulKeyLen;
                break;

            case 1:// ss-ks-se-ke
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal - (ulKeyLen/2)%ulKeyStartVal;
                l_pSearchList->List[ulItemIndex].ulAccessLen = (ulKeyLen/2)%ulKeyStartVal + ulItemIndex + 1;
                break;

            case 2://ks-ss-se-ke
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal + ulItemIndex%ulKeyLen;
                l_pSearchList->List[ulItemIndex].ulAccessLen = ulKeyLen - ulItemIndex%ulKeyLen;
                break;

            case 3://ks-ss-ke-se
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal + ulKeyLen/2;
                l_pSearchList->List[ulItemIndex].ulAccessLen = ulKeyLen + ulItemIndex;
                break;

            default:
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal - ulItemIndex%ulKeyStartVal;
                l_pSearchList->List[ulItemIndex].ulAccessLen = ulKeyLen + 2 * ulItemIndex;
                break;    
            }
        }//if ( ulItemIndex == l_ulTimer % TEST_SE_LIST_LEN )
        else
        {
            if ( 0 == ulItemIndex%2 )//ss-se-ks-ke
            {
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal - (ulKeyLen + ulItemIndex)%ulKeyStartVal;
                l_pSearchList->List[ulItemIndex].ulAccessLen = ulKeyLen%ulKeyStartVal;
            }
            else//ks-ke-ss-se
            {
                l_pSearchList->List[ulItemIndex].ulStartValue = ulKeyStartVal + ulKeyLen + ulItemIndex + 1;
                l_pSearchList->List[ulItemIndex].ulAccessLen = ulKeyLen/2 + ulItemIndex + 1;
            }
        }
    }//for ( i = 0; i < TEST_SE_LIST_LEN; i++ )

    tOverlapEntry.ulKeyStartVal = ulKeyStartVal;
    tOverlapEntry.ulKeyLen = ulKeyLen;
    tOverlapEntry.ulListAddr = (U32)l_pSearchList;
    tOverlapEntry.ulListLenQW = TEST_SE_LIST_LEN;

    HAL_SESearchOverlap( &tOverlapEntry, bOTFB );

    /******* Check *******/
    for ( ucBitMapDWID = 0; ucBitMapDWID < SE_OVERLAP_BITMAP_LEMDW; ucBitMapDWID++ ) 
    {
        for ( ucBitMapBitID = 0; ucBitMapBitID < DWORD_BIT_SIZE; ucBitMapBitID++ )
        {
            ulItemIndex = ucBitMapDWID * DWORD_BIT_SIZE + ucBitMapBitID;

            if ( ulItemIndex >= TEST_SE_LIST_LEN )
            {
                break;
            }

            if ( 0 != (tOverlapEntry.aOverlapBitMap[ucBitMapDWID] & (1 << ucBitMapBitID)) )//range overlap
            {
                if ( ulItemIndex != l_ulTimer % TEST_SE_LIST_LEN )
                {
                    rTracer = 0x12345678;
                    DBG_Printf("TEST_SESearchOverlap: 12345678, 0x%x\n",ulItemIndex * 0x10000 + l_ulTimer);
                    DBG_Getch();
                }
            }
            else
            {
                if ( ulItemIndex == l_ulTimer % TEST_SE_LIST_LEN )
                {
                    rTracer = 0x87654321;
                    DBG_Printf("TEST_SESearchOverlap: 87654321, 0x%x\n",ulItemIndex * 0x10000 + l_ulTimer);
                    DBG_Getch();
                }
            }
        }        
    }
    
    rTracer = l_ulTimer;
    DBG_Printf("TEST_SESearchOverlap: 0x%x\n",l_ulTimer);
}

void TEST_SESearchRegBitOne(void)
{
    SE_SEARCH_BIT_PARAM tParam;
    SE_SEARCH_BIT_RESULT tResult;
    U32 ulData = 0;
    U8 ucBitIndex = l_ulTimer%(DWORD_BIT_SIZE + 1);

    if ( ucBitIndex < DWORD_BIT_SIZE )
    {
        ulData |= (1 << ucBitIndex);
    }

    tParam.bReg = TRUE;
    tParam.bLoad = TRUE;
    tParam.bBitValue = 1;
    tParam.ulData = ulData;

    HAL_SESearchBit(&tParam, &tResult);

    if ( FALSE == tResult.bFind )
    {
        if ( ucBitIndex < DWORD_BIT_SIZE )
        {
            DBG_Printf("TEST_SESearchRegBitOne: not find, wrong!\n");
            DBG_Getch();
        }
    }
    else
    {
        if ( ucBitIndex != (U8)tResult.usBitIndex )
        {
            DBG_Printf("TEST_SESearchRegBitOne: index wrong!\n");
            DBG_Getch();
        }
    }

    DBG_Printf("TEST_SESearchRegBitOne: 0x%x OK\n",l_ulTimer);
}

void TEST_SESearchRegBitZero(void)
{
    SE_SEARCH_BIT_PARAM tParam;
    SE_SEARCH_BIT_RESULT tResult;
    U32 ulData = INVALID_8F;
    U8 ucBitIndex = l_ulTimer%(DWORD_BIT_SIZE + 1);

    if ( ucBitIndex < DWORD_BIT_SIZE )
    {
        ulData &= (~(1 << ucBitIndex));
    }

    tParam.bReg = TRUE;
    tParam.bLoad = TRUE;
    tParam.bBitValue = 0;
    tParam.ulData = ulData;

    HAL_SESearchBit(&tParam, &tResult);

    if ( FALSE == tResult.bFind )
    {
        if ( ucBitIndex < DWORD_BIT_SIZE )
        {
            DBG_Printf("TEST_SESearchRegBitZero: not find, wrong!\n");
            DBG_Getch();
        }
    }
    else
    {
        if ( ucBitIndex != (U8)tResult.usBitIndex )
        {
            DBG_Printf("TEST_SESearchRegBitZero: index wrong!\n");
            DBG_Getch();
        }
    }

    DBG_Printf("TEST_SESearchRegBitZero: 0x%x OK\n",l_ulTimer);
}

void TEST_SESearchRegBitLoad(BOOL bBitValue)
{
    SE_SEARCH_BIT_PARAM tParam;
    SE_SEARCH_BIT_RESULT tResult;
    U32 ulData;
    U8 ucBitPitch = DWORD_BIT_SIZE / ( l_ulTimer%(DWORD_BIT_SIZE/2) + 1);
    U8 ucFirstBit = l_ulTimer % DWORD_BIT_SIZE;
    U8 i;

    if ( 1 == bBitValue )
    {
        ulData = 0;

        for ( i =  ucFirstBit; i < DWORD_BIT_SIZE; i += ucBitPitch )
        {
            ulData |= (1 << i);
        }
    }
    else
    {
        ulData = INVALID_8F;

        for ( i =  ucFirstBit; i < DWORD_BIT_SIZE; i += ucBitPitch )
        {
            ulData &= ( ~(1 << i) );
        }
    }

    tParam.bReg = TRUE;
    tParam.bLoad = TRUE;
    tParam.bBitValue = bBitValue;
    tParam.ulData = ulData;

    for (  i =  ucFirstBit; i < DWORD_BIT_SIZE; i += ucBitPitch  )
    {
        HAL_SESearchBit(&tParam, &tResult);

        if ( FALSE == tResult.bFind )
        {
            if ( i < DWORD_BIT_SIZE )
            {
                DBG_Printf("TEST_SESearchRegBitLoad: not find, wrong!\n");
                DBG_Getch();
            }
        }
        else
        {
            if ( i != (U8)tResult.usBitIndex )
            {
                DBG_Printf("TEST_SESearchRegBitLoad: index wrong!\n");
                DBG_Getch();
            }
        }

        tParam.bLoad = FALSE;
    }

    DBG_Printf("TEST_SESearchRegBitLoad: 0x%x OK\n",l_ulTimer);
}

void TEST_SESearchMemBit(BOOL bOTFB, BOOL bBitValue)
{
    SE_SEARCH_BIT_PARAM tParam;
    SE_SEARCH_BIT_RESULT tResult;
    U32 ulStartAddr;
    U32 *pData;
    U16 usLenQW= (l_ulTimer/DWORD_BIT_SIZE)%512 + 1;
    U16 usBitIndex = l_ulTimer%(usLenQW * DWORD_BIT_SIZE * 2);
    U32 i;

    if ( TRUE == bOTFB )
    {
        ulStartAddr = OTFB_RED_DATA_MCU1_BASE;
    }
    else
    {
        ulStartAddr = DRAM_DATA_BUFF_MCU1_BASE;
    }

    if ( 1 == bBitValue )
    {
        for ( pData = (U32 *)ulStartAddr, i = 0; i < (U32)(usLenQW * 2); pData++, i++)
        {
            *pData = 0;

            if ( (usBitIndex/DWORD_BIT_SIZE == i) && (0 != l_ulTimer%20) )
            {
                *pData |= (1 << (usBitIndex%DWORD_BIT_SIZE));
            }
        }

        tParam.bBitValue = 1;
    }
    else
    {
        for ( pData = (U32 *)ulStartAddr, i = 0; i < (U32)(usLenQW * 2); pData++, i++)
        {
            *pData = INVALID_8F;

            if ( (usBitIndex/DWORD_BIT_SIZE == i) && (0 != l_ulTimer%20) )
            {
                *pData &= ( ~(1 << (usBitIndex%DWORD_BIT_SIZE)) );
            }
        }

        tParam.bBitValue = 0;
    }
    
    tParam.ulStartAddr = ulStartAddr;
    tParam.usSizeQW = usLenQW;
    tParam.bLoad = TRUE;
    tParam.bOTFB = bOTFB;
    tParam.bReg = FALSE;

    HAL_SESearchBit( &tParam, &tResult );

    if ( FALSE == tResult.bFind )
    {
        if ( 0 != l_ulTimer%20 )
        {
            DBG_Printf("TEST_SESearchMemBit: %d, not find, wrong!\n", l_ulTimer);
            DBG_Getch();
        }
    }
    else
    {
        if ( 0 == l_ulTimer%20 )
        {
            DBG_Printf("TEST_SESearchMemBit: %d, find, wrong!\n", l_ulTimer);
            DBG_Getch();
        }
        else if ( usBitIndex != tResult.usBitIndex )
        {
            DBG_Printf("TEST_SESearchMemBit: index wrong!\n");
            DBG_Getch();
        }
    }

    DBG_Printf("TEST_SESearchMemBit: 0x%x OK\n",l_ulTimer);
}

void TEST_SESearchMemBitLoad(BOOL bOTFB, BOOL bBitValue)
{
    SE_SEARCH_BIT_PARAM tParam;
    SE_SEARCH_BIT_RESULT tResult;
    U32 ulStartAddr;
    U32 *pData;
    U16 usLenQW= (l_ulTimer/DWORD_BIT_SIZE)%512 + 1;
    U16 usBitPitch;
    U16 usFirstBit = l_ulTimer%(usLenQW * DWORD_BIT_SIZE * 2);
    U16 usBitIndex;

    if ( 0 == l_ulTimer%2 )
    {
        usBitPitch = DWORD_BIT_SIZE;
    }
    else
    {
        usBitPitch = DWORD_BIT_SIZE/2;
    }

    if ( TRUE == bOTFB )
    {
        ulStartAddr = OTFB_RED_DATA_MCU1_BASE;
    }
    else
    {
        ulStartAddr = DRAM_DATA_BUFF_MCU1_BASE;
    }

    if ( 1 == bBitValue )
    {
        COM_MemSet( (U32 *)ulStartAddr, usLenQW * 2, 0);

        for ( usBitIndex = usFirstBit;  usBitIndex < usLenQW * 2 * DWORD_BIT_SIZE; usBitIndex += usBitPitch)
        {
            pData = (U32 *)ulStartAddr + usBitIndex/DWORD_BIT_SIZE;
            *pData |= ( 1 << (usBitIndex%DWORD_BIT_SIZE) );
        }

        tParam.bBitValue = 1;
    }
    else
    {
        COM_MemSet( (U32 *)ulStartAddr, usLenQW * 2, INVALID_8F);

        for ( usBitIndex = usFirstBit;  usBitIndex < usLenQW * 2 * DWORD_BIT_SIZE; usBitIndex += usBitPitch)
        {
            pData = (U32 *)ulStartAddr + usBitIndex/DWORD_BIT_SIZE;
            *pData &= ( ~( 1 << (usBitIndex%DWORD_BIT_SIZE) ) );
        }

        tParam.bBitValue = 0;
    }
    
    tParam.ulStartAddr = ulStartAddr;
    tParam.usSizeQW = usLenQW;
    tParam.bLoad = TRUE;
    tParam.bOTFB = bOTFB;
    tParam.bReg = FALSE;

    for ( usBitIndex = usFirstBit;  usBitIndex < usLenQW * 2 * DWORD_BIT_SIZE; usBitIndex += usBitPitch)
    {
        HAL_SESearchBit( &tParam, &tResult );

        if ( FALSE == tResult.bFind )
        {
            DBG_Printf("TEST_SESearchMemBitLoad: not find, wrong!\n");
            DBG_Getch();
        }
        else
        {
            if ( usBitIndex != tResult.usBitIndex )
            {
                DBG_Printf("TEST_SESearchMemBitLoad: index wrong!\n");
                DBG_Getch();
            }
        }

        tParam.bLoad = FALSE;
    }

    DBG_Printf("TEST_SESearchMemBitLoad: 0x%x OK\n",l_ulTimer);
}

void TEST_SESetBit(BOOL bBitValue, BOOL bOTFB)
{
    U32 ulStartAddr;
    U32 *pData;
    U16 usBitIndex = l_ulTimer%INVALID_4F;
    U16 usMemLenDW = usBitIndex/DWORD_BIT_SIZE + 1;
    SE_SET_BIT_PARAM tParam;
    U16 i;

    if ( TRUE == bOTFB )
    {
        ulStartAddr = OTFB_RED_DATA_MCU1_BASE;
    }
    else
    {
        ulStartAddr = DRAM_DATA_BUFF_MCU1_BASE;
    }

    if ( 1 == bBitValue )
    {
        COM_MemSet( (U32 *)ulStartAddr,  usMemLenDW, 0 );

        tParam.ulSetAddr = ulStartAddr;
        tParam.bBitValue = 1;
        tParam.usSetIndex = usBitIndex;
        tParam.bOTFB = bOTFB;

        HAL_SESetBit(&tParam);

        for ( pData = (U32 *)ulStartAddr, i = 0; i < usMemLenDW; pData++, i++ )
        {
            if ( i != usBitIndex/DWORD_BIT_SIZE )
            {
                if ( 0 != *pData )
                {
                    DBG_Printf("TEST_SESetBit: set bit 1 error\n");
                    DBG_Getch();
                }
            }
            else
            {
                if ( ( 1 << (usBitIndex%DWORD_BIT_SIZE) ) != *pData )
                {
                    DBG_Printf("TEST_SESetBit: set bit 1 error\n");
                    DBG_Getch();
                }
            }
        }//for ( pData = (U32 *)ulStartAddr, i = 0; i < usMemLenDW; i++ )
    }//if ( 1 == bBitValue )
    else
    {
        COM_MemSet( (U32 *)ulStartAddr,  usMemLenDW, INVALID_8F );

        tParam.ulSetAddr = ulStartAddr;
        tParam.bBitValue = 0;
        tParam.usSetIndex = usBitIndex;
        tParam.bOTFB = bOTFB;

        HAL_SESetBit(&tParam);

        for ( pData = (U32 *)ulStartAddr, i = 0; i < usMemLenDW; pData++, i++ )
        {
            if ( i != usBitIndex/DWORD_BIT_SIZE )
            {
                if ( INVALID_8F != *pData )
                {
                    DBG_Printf("TEST_SESetBit: set bit 0 error\n");
                    DBG_Getch();
                }
            }
            else
            {
                if ( (~( 1 << (usBitIndex%DWORD_BIT_SIZE) )) != *pData )
                {
                    DBG_Printf("TEST_SESetBit: set bit 0 error\n");
                    DBG_Getch();
                }
            }
        }//for ( pData = (U32 *)ulStartAddr, i = 0; i < usMemLenDW; i++ )
    }//else of if ( 1 == bBitValue )

    DBG_Printf("TEST_SESetBit: 0x%x OK\n", l_ulTimer);

    return;
}

void TEST_SEMain(void)
{
#if defined(COSIM) //only COSIM ENV need FW code to initialize DDR
    HAL_DramcInit();
#endif

    HAL_SEInit();
#if defined(SE_TEST_VALUE_DRAM) || defined(SE_TEST_VALUE_OTFB)
    TEST_SECaseParamInit();
#endif
    l_ulTimer = 0;

    while (1)
    {
        //test: search overlap area 
#ifdef SE_TEST_OVERLAP_DRAM
        TEST_SESearchOverlap(FALSE);
#endif

        //test: search max/min/same, item size is QW
#ifdef SE_TEST_VALUE_DRAM
        TEST_SEVBTInit(FALSE);
        TEST_SEVbtMax(FALSE);
        TEST_SEVbtMin(FALSE);
        TEST_SEVbtSame(FALSE);
#endif

        //test:search bit
#ifdef SE_TEST_SEARCH_BIT_REG
        TEST_SESearchRegBitOne();
        TEST_SESearchRegBitZero();
        TEST_SESearchRegBitLoad(1);
        TEST_SESearchRegBitLoad(0);
#endif

#ifdef SE_TEST_SEARCH_BIT_MEM
        TEST_SESearchMemBit( FALSE, 1 );
        TEST_SESearchMemBit( FALSE, 0 );
        TEST_SESearchMemBitLoad(FALSE, 1);
        TEST_SESearchMemBitLoad(FALSE, 0);
        TEST_SESearchMemBit( TRUE, 1 );
        TEST_SESearchMemBit( TRUE, 0 );
        TEST_SESearchMemBitLoad(TRUE, 1);
        TEST_SESearchMemBitLoad(TRUE, 0);
#endif

        //test: search overlap area 
#ifdef SE_TEST_OVERLAP_OTFB
        TEST_SESearchOverlap(TRUE);
#endif

        //test: search max/min/same, item size is QW
#ifdef SE_TEST_VALUE_OTFB
        TEST_SEVBTInit(TRUE);
        TEST_SEVbtMax(TRUE);
        TEST_SEVbtMin(TRUE);
        TEST_SEVbtSame(TRUE);
#endif

        //test:search bit
#ifdef SE_TEST_SET_BIT
        TEST_SESetBit(0, FALSE);
        TEST_SESetBit(0, TRUE);
        TEST_SESetBit(1, FALSE);
        TEST_SESetBit(1, TRUE);
#endif

        l_ulTimer++;
    }
}
