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
Filename    :
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.09.20    15:16:52
Description :
Others      :
Modify      :
****************************************************************************/
#include "HAL_SearchEngine.h"
#include "HAL_GLBReg.h"
#include "sim_search_engine.h"
#include "memory_access.h"
#include "model_common.h"
#ifdef SIM_XTENSA
#include "iss/mp.h"
#include "xtmp_common.h"
#include "xtmp_localmem.h"
#endif

LOCAL U32 l_ulBitSearchRegData;
LOCAL U32 l_ulBitSearchMemAddr;
LOCAL U8 l_ucRegLastIndex;
LOCAL U16 l_usMemLastIndex;
LOCAL U16 l_usBitMemSizeQW;
LOCAL U32 *l_pSearchBitMemData;

void SE_ReadRegByByte(U32 ulAddr, U32 ulByteCnt, U8 *pBuf)
{
#ifdef SIM
    U32 i;
    U8 *pReg = (U8 *)ulAddr;

    for ( i = 0; i < ulByteCnt; i++ )
    {
        *pBuf = *pReg;
        pBuf++;
        pReg++;
    }
#else
    regRead( ulAddr, ulByteCnt, pBuf );
#endif

    return;
}

void SE_WriteRegByByte(U32 ulAddr, U32 ulByteCnt, U8 *pBuf)
{
#ifdef SIM
    U32 i;
    U8 *pReg = (U8 *)ulAddr;

    for ( i = 0; i < ulByteCnt; i++ )
    {
        *pReg = *pBuf;
        pReg++;
        pBuf++;
    }
#else
    regWrite( ulAddr, ulByteCnt, pBuf );
#endif

    return;
}

BOOL SE_IsOtfbEn(U8 ucSEID)
{
    U32 ulOtfbEnReg;

    Comm_ReadReg( (U32)&rGlbTrfc, 1, (U32 *)&ulOtfbEnReg);

    if (  (SE0 == ucSEID) && (0 != (ulOtfbEnReg & MSK_EN_SE0_OTFB)) )
    {
        return TRUE;
    }
    else if ( (SE1 == ucSEID) && (0 != (ulOtfbEnReg & MSK_EN_SE1_OTFB)) )
    {
        return TRUE;
    }
    else if ( (SE2 == ucSEID) && (0 != (ulOtfbEnReg & MSK_EN_SE2_OTFB)) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void SE_SearchValReadParam( U8 ucSEID, SEMODEL_VALUE_PARAM *pParam )
{
    SE_TRIGGER_SIZE_REG ucTrigSizeReg;
    SE_TYPE_REG ucTypeReg;
    U32 ulXorEnReg;

    Comm_ReadReg( (U32)&rSE_START_ADDR(ucSEID), 1, (U32 *)&(pParam->ulStartAddr) );
    Comm_ReadReg( (U32)&rSE_SEARCH_LEN_QW(ucSEID), 1, (U32 *)&(pParam->ulLenQW) );
    Comm_ReadReg( (U32)&rSE_PITCH_SIZE(ucSEID), 1, (U32 *)&(pParam->ucPitchSize) );

    /* First MSK group*/
    Comm_ReadReg( (U32)&rSE_FMSK_SELECTION_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK1_Low) );
    Comm_ReadReg( (U32)&rSE_FMSK_SELECTION_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK1_High) );
    Comm_ReadReg( (U32)&rSE_FMASK_CONDITION_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK2_Low) );
    Comm_ReadReg( (U32)&rSE_FMASK_CONDITION_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK2_High) );
    Comm_ReadReg( (U32)&rSE_FMASK_FIELD_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK3_Low) );
    Comm_ReadReg( (U32)&rSE_FMASK_FIELD_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK3_High) );
    Comm_ReadReg( (U32)&rSE_FMASK_XOR_FIELD_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK4_Low) );
    Comm_ReadReg( (U32)&rSE_FMASK_XOR_FIELD_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK4_High) );
    Comm_ReadReg( (U32)&rSE_FMASK_XOR_VALUE_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK5_Low) );
    Comm_ReadReg( (U32)&rSE_FMASK_XOR_VALUE_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].MSK5_High) );
    Comm_ReadReg( (U32)&rSE_FVALUE_SEARCHED_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].Value_Low) );
    Comm_ReadReg( (U32)&rSE_FVALUE_SEARCHED_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_1ST_MSK_GROUP].Value_High) );

    /* Second MSK group*/
    Comm_ReadReg( (U32)&rSE_SMSK_SELECTION_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK1_Low) );
    Comm_ReadReg( (U32)&rSE_SMSK_SELECTION_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK1_High) );
    Comm_ReadReg( (U32)&rSE_SMASK_CONDITION_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK2_Low) );
    Comm_ReadReg( (U32)&rSE_SMASK_CONDITION_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK2_High) );
    Comm_ReadReg( (U32)&rSE_SMASK_FIELD_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK3_Low) );
    Comm_ReadReg( (U32)&rSE_SMASK_FIELD_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK3_High) );
    Comm_ReadReg( (U32)&rSE_SMASK_XOR_FIELD_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK4_Low) );
    Comm_ReadReg( (U32)&rSE_SMASK_XOR_FIELD_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK4_High) );
    Comm_ReadReg( (U32)&rSE_SMASK_XOR_VALUE_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK5_Low) );
    Comm_ReadReg( (U32)&rSE_SMASK_XOR_VALUE_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].MSK5_High) );
    Comm_ReadReg( (U32)&rSE_SVALUE_SEARCHED_LOW(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].Value_Low) );
    Comm_ReadReg( (U32)&rSE_SVALUE_SEARCHED_HIGH(ucSEID), 1, (U32 *)&(pParam->aMskGroup[SE_2ND_MSK_GROUP].Value_High) );

    SE_ReadRegByByte( (U32)&rSE_TRIGGER_SIZE_REG(ucSEID), 1, (U8 *)&ucTrigSizeReg);
    if ( SE_ITEM_SIZE_DWORD == ucTrigSizeReg.bsItemSize )
    {
        pParam->bItemQW = FALSE;
    }
    else
    {
        pParam->bItemQW = TRUE;
    }

    SE_ReadRegByByte( (U32)&rSE_TYPE_REG(ucSEID), 1, (U8 *)&ucTypeReg);
    pParam->aMskGroup[SE_1ST_MSK_GROUP].SType = ucTypeReg.bsFGroupSType;
    pParam->aMskGroup[SE_1ST_MSK_GROUP].CType = ucTypeReg.bsFGroupCType;
    pParam->aMskGroup[SE_2ND_MSK_GROUP].SType = ucTypeReg.bsSGroupSType;
    pParam->aMskGroup[SE_2ND_MSK_GROUP].CType = ucTypeReg.bsSGroupCType;

    Comm_ReadReg( (U32)&rSE_XOR_MASK_ENABLE(ucSEID), 1, &ulXorEnReg);
    if ( 0 != (ulXorEnReg & MSK_FGROUP_XOR_EN) )
    {
        pParam->aMskGroup[SE_1ST_MSK_GROUP].bXorEn = TRUE;
    }
    if ( 0 != (ulXorEnReg & MSK_SGROUP_XOR_EN) )
    {
        pParam->aMskGroup[SE_2ND_MSK_GROUP].bXorEn = TRUE;
    }

    return;
}

BOOL SE_CmpItemDW( SIMSE_MSK_GROUP *pMskGroup, U32 ulItemValue, U32 ulKeyValue )
{
    U32 ulCmpItemVal;
    U32 ulCmpKeyVal;
    BOOL bConform = FALSE;

    if ( 0 != pMskGroup->MSK4_High )
    {
        if ( 0 != ((ulItemValue & pMskGroup->MSK4_High) ^ pMskGroup->MSK5_High) )
        {
            return FALSE;
        }
    }

    if ( 0 != pMskGroup->MSK1_High )
    {
        if ( 0 == (ulItemValue & pMskGroup->MSK1_High) )
        {
            return FALSE;
        }
    }

    if ( 0 != pMskGroup->MSK2_High )
    {
        if ( (0 == pMskGroup->CType) && (0 != (ulItemValue & pMskGroup->MSK2_High)) )
        {
            return FALSE;
        }
        else if ( (1 == pMskGroup->CType) && (0 == (ulItemValue & pMskGroup->MSK2_High)) )
        {
            return FALSE;
        }
    }

    ulCmpItemVal = (ulItemValue & pMskGroup->MSK3_High);
    ulCmpKeyVal = (ulKeyValue & pMskGroup->MSK3_High);
    switch( pMskGroup->SType )
    {
    case SEARCH_TYPE_MAX:
        if ( ulCmpItemVal > ulCmpKeyVal )
        {
            bConform = TRUE;
        }
        break;

    case SEARCH_TYPE_MIN:
        if ( ulCmpItemVal < ulCmpKeyVal )
        {
            bConform = TRUE;
        }
        break;

   case SEARCH_TYPE_SAME:
        if ( ulCmpItemVal == ulCmpKeyVal )
        {
            bConform = TRUE;
        }
        break;

   default:
        DBG_Printf("SE_SearchValueProcess: Search type %d error!\n",pMskGroup->SType);
        DBG_Break();
        break;
    }

    return bConform;
}

BOOL SE_CmpItemQW( SIMSE_MSK_GROUP *pMskGroup, QWORD *pItem, QWORD *pKey )
{
    QWORD qwCmpItemVal;
    QWORD qwCmpKeyVal;
    BOOL bConform = FALSE;

    if ( (0 != pMskGroup->MSK4_High) || (0 != pMskGroup->MSK4_Low) )
    {
        if ( (0 != ((pItem->HighDw & pMskGroup->MSK4_High) ^ pMskGroup->MSK5_High))
            || (0 != ((pItem->LowDw & pMskGroup->MSK4_Low) ^ pMskGroup->MSK5_Low)) )
        {
            return FALSE;
        }
    }

    if ( (0 != pMskGroup->MSK1_High) || (0 != pMskGroup->MSK1_Low) )
    {
        if ( (0 == (pItem->HighDw & pMskGroup->MSK1_High))
            && (0 == (pItem->LowDw & pMskGroup->MSK1_Low)) )
        {
            return FALSE;
        }
    }

    if ( (0 != pMskGroup->MSK2_High) || (0 != pMskGroup->MSK2_Low) )
    {
        if ( 0 == pMskGroup->CType ) 
        {
            if ( (0 != (pItem->HighDw & pMskGroup->MSK2_High))
                || (0 != (pItem->LowDw & pMskGroup->MSK2_Low)) )
            {
                return FALSE;
            }
        }
        else
        {
            if ( (0 == (pItem->HighDw & pMskGroup->MSK2_High))
                && (0 == (pItem->LowDw & pMskGroup->MSK2_Low)) )
            {
                return FALSE;
            }
        }
    }

    qwCmpItemVal.HighDw = (pItem->HighDw & pMskGroup->MSK3_High);
    qwCmpItemVal.LowDw = (pItem->LowDw & pMskGroup->MSK3_Low);
    qwCmpKeyVal.HighDw = (pKey->HighDw & pMskGroup->MSK3_High);
    qwCmpKeyVal.LowDw = (pKey->LowDw & pMskGroup->MSK3_Low);
    switch( pMskGroup->SType )
    {
    case SEARCH_TYPE_MAX:
        if ( qwCmpItemVal.HighDw > qwCmpKeyVal.HighDw )
        {
            bConform = TRUE;
        }
        else if ( qwCmpItemVal.HighDw == qwCmpKeyVal.HighDw  )
        {
            if ( qwCmpItemVal.LowDw > qwCmpKeyVal.LowDw )
            {
                bConform = TRUE;
            }
        }
        break;

    case SEARCH_TYPE_MIN:
        if ( qwCmpItemVal.HighDw < qwCmpKeyVal.HighDw )
        {
            bConform = TRUE;
        }
        else if ( qwCmpItemVal.HighDw == qwCmpKeyVal.HighDw  )
        {
            if ( qwCmpItemVal.LowDw < qwCmpKeyVal.LowDw )
            {
                bConform = TRUE;
            }
        }
        break;

   case SEARCH_TYPE_SAME:
        if ( (qwCmpItemVal.HighDw == qwCmpKeyVal.HighDw) && (qwCmpItemVal.LowDw == qwCmpKeyVal.LowDw) )
        {
            bConform = TRUE;
        }
        break;

   default:
        DBG_Printf("SE_SearchValueProcess: Search type %d error!\n",pMskGroup->SType);
        DBG_Break();
        break;
    }

    return bConform;
}

void SE_SearchValueProcess(U8 ucSEID)
{
    SEMODEL_VALUE_PARAM tValueParam;
    SE_STATUS_REG tStatusReg = {0};
    U32 ulItemAddr;
    U32 ulItemCnt;
    U8  ucPitchDW;
    U32 ulItemIndex;
    BOOL bOTFB = SE_IsOtfbEn(ucSEID);
    BOOL bItemQW;
    U8 ucGroupIndex;
    QWORD qwItemValue;
    QWORD aKeyValue[SE_MSK_GROUP_NUM];
    U32 aRtnIndex[SE_MSK_GROUP_NUM];
    BOOL aCmpTrue[SE_MSK_GROUP_NUM];
    BOOL aFind[SE_MSK_GROUP_NUM] = {0};

    /*******************************************************************************
    ************************ Read parameter from registers *************************
    ********************************************************************************/
    SE_SearchValReadParam(ucSEID, &tValueParam);
    bItemQW = tValueParam.bItemQW;
    if ( TRUE == bItemQW )
    {
        ulItemCnt = tValueParam.ulLenQW;
        ucPitchDW = (1 + tValueParam.ucPitchSize) * 2;
    }
    else
    {
        ulItemCnt = tValueParam.ulLenQW * 2;
        ucPitchDW = 1 + tValueParam.ucPitchSize;
    }

    for ( ucGroupIndex = 0; ucGroupIndex < SE_MSK_GROUP_NUM; ucGroupIndex++ )
    {
        switch( tValueParam.aMskGroup[ucGroupIndex].SType )
        {
        case SEARCH_TYPE_MAX:
            aKeyValue[ucGroupIndex].LowDw = 0;
            aKeyValue[ucGroupIndex].HighDw = 0;
            break;

        case SEARCH_TYPE_MIN:
            aKeyValue[ucGroupIndex].LowDw = INVALID_8F;
            aKeyValue[ucGroupIndex].HighDw = INVALID_8F;
            break;

        case SEARCH_TYPE_SAME:
            aKeyValue[ucGroupIndex].LowDw = tValueParam.aMskGroup[ucGroupIndex].Value_Low;
            aKeyValue[ucGroupIndex].HighDw = tValueParam.aMskGroup[ucGroupIndex].Value_High;
            break;

        default:
            DBG_Printf("SE_SearchValueProcess: Search type %d error!\n", tValueParam.aMskGroup[ucGroupIndex].SType);
            DBG_Break();
            break;
        }
    }

    /*********************************************************************
    ***************** search max/min/same  *******************************
    **********************************************************************/
    for ( ulItemAddr = tValueParam.ulStartAddr, ulItemIndex = 0; ulItemIndex < ulItemCnt; ulItemAddr += ucPitchDW*sizeof(U32), ulItemIndex++)
    {
        aCmpTrue[SE_1ST_MSK_GROUP] = FALSE;
        aCmpTrue[SE_2ND_MSK_GROUP] = FALSE;

        if ( TRUE == bItemQW )
        {
            if ( FALSE == bOTFB ) 
            {
                Comm_ReadDram( ulItemAddr, 1, &(qwItemValue.LowDw) );
                Comm_ReadDram( ulItemAddr + 4, 1, &(qwItemValue.HighDw) );
            }
            else
            {
                Comm_ReadOtfb( ulItemAddr, 1, &(qwItemValue.LowDw) );
                Comm_ReadOtfb( ulItemAddr + 4, 1, &(qwItemValue.HighDw) );
            }

            /*compare for first mask group*/
            if ( (SEARCH_TYPE_SAME == tValueParam.aMskGroup[SE_1ST_MSK_GROUP].SType) && (TRUE == aFind[SE_1ST_MSK_GROUP]) )
            {
                //no need compare
            }
            else
            {
                if( TRUE == SE_CmpItemQW( &tValueParam.aMskGroup[SE_1ST_MSK_GROUP], &qwItemValue, &(aKeyValue[SE_1ST_MSK_GROUP]) ) )
                {
                    aCmpTrue[SE_1ST_MSK_GROUP] = TRUE;
                    aFind[SE_1ST_MSK_GROUP] = TRUE;
                }
            }

            /*compare for second mask group*/
            if ( (SEARCH_TYPE_SAME == tValueParam.aMskGroup[SE_2ND_MSK_GROUP].SType) && (TRUE == aFind[SE_2ND_MSK_GROUP]) )
            {
                //no need compare
            }
            else
            {
                if ( TRUE == SE_CmpItemQW( &tValueParam.aMskGroup[SE_2ND_MSK_GROUP], &qwItemValue, &(aKeyValue[SE_2ND_MSK_GROUP]) ) )
                {
                    aCmpTrue[SE_2ND_MSK_GROUP] = TRUE;
                    aFind[SE_2ND_MSK_GROUP] = TRUE;
                }
            }
        }//if ( TRUE == bItemQW )
        else
        {
            if ( FALSE == bOTFB ) 
            {
                Comm_ReadDram( ulItemAddr + 4, 1, &(qwItemValue.HighDw) );// leave a question when item_size is DWORD
            }
            else
            {
                Comm_ReadOtfb( ulItemAddr + 4, 1, &(qwItemValue.HighDw) );// leave a question when item_size is DWORD
            }

            /*compare for first mask group*/
            if ( (SEARCH_TYPE_SAME == tValueParam.aMskGroup[SE_1ST_MSK_GROUP].SType) && (TRUE == aFind[SE_1ST_MSK_GROUP]) )
            {
                //no need compare
            }
            else
            {
                if ( TRUE == SE_CmpItemDW( &tValueParam.aMskGroup[SE_1ST_MSK_GROUP], qwItemValue.HighDw, aKeyValue[SE_1ST_MSK_GROUP].HighDw ) )
                {
                    aCmpTrue[SE_1ST_MSK_GROUP] = TRUE;
                    aFind[SE_1ST_MSK_GROUP] = TRUE;
                }
            }

            /*compare for second mask group*/
            if ( (SEARCH_TYPE_SAME == tValueParam.aMskGroup[SE_2ND_MSK_GROUP].SType) && (TRUE == aFind[SE_2ND_MSK_GROUP]) )
            {
                //no need compare
            }
            else
            {
                if ( TRUE == SE_CmpItemDW( &tValueParam.aMskGroup[SE_2ND_MSK_GROUP], qwItemValue.HighDw, aKeyValue[SE_2ND_MSK_GROUP].HighDw ) )
                {
                    aCmpTrue[SE_2ND_MSK_GROUP] = TRUE;
                    aFind[SE_2ND_MSK_GROUP] = TRUE;
                }
            }
        }//else of if ( TRUE == bItemQW )

        if ( TRUE == aCmpTrue[SE_1ST_MSK_GROUP] )
        {
            aKeyValue[SE_1ST_MSK_GROUP].LowDw = qwItemValue.LowDw;
            aKeyValue[SE_1ST_MSK_GROUP].HighDw = qwItemValue.HighDw;
            aRtnIndex[SE_1ST_MSK_GROUP] = ulItemIndex;
        }
        if ( TRUE == aCmpTrue[SE_2ND_MSK_GROUP] )
        {
            aKeyValue[SE_2ND_MSK_GROUP].LowDw = qwItemValue.LowDw;
            aKeyValue[SE_2ND_MSK_GROUP].HighDw = qwItemValue.HighDw;
            aRtnIndex[SE_2ND_MSK_GROUP] = ulItemIndex;
        }
    }//for ( ulItemAddr = tValueParam.ulStartAddr, ulItemIndex = 0; ulItemIndex < ulItemCnt; ulItemAddr += ucPitchDW * sizeof(U32), ulItemIndex++)

    /*********************************************************************
    ***************** fill result register  ******************************
    **********************************************************************/
    if ( TRUE == aFind[SE_1ST_MSK_GROUP] )
    {
        if ( SEARCH_TYPE_SAME == tValueParam.aMskGroup[SE_1ST_MSK_GROUP].SType )
        {
            tStatusReg.bFGroupFind = TRUE;
        }

        Comm_WriteReg( (U32)&rSE_FINDEX(ucSEID), 1, &(aRtnIndex[SE_1ST_MSK_GROUP]) );
        Comm_WriteReg( (U32)&rSE_RETURN_FVALUE_LOW(ucSEID), 1, &(aKeyValue[SE_1ST_MSK_GROUP].LowDw) );
        Comm_WriteReg( (U32)&rSE_RETURN_FVALUE_HIGH(ucSEID), 1, &(aKeyValue[SE_1ST_MSK_GROUP].HighDw) );
    }
    else
    {
        tStatusReg.bFGroupFind = FALSE;
    }

    if ( TRUE == aFind[SE_2ND_MSK_GROUP] )
    {
        if ( SEARCH_TYPE_SAME == tValueParam.aMskGroup[SE_2ND_MSK_GROUP].SType )
        {
            tStatusReg.bSGroupFind = TRUE;
        }

        Comm_WriteReg( (U32)&rSE_FINDEX(ucSEID), 1, &(aRtnIndex[SE_2ND_MSK_GROUP]) );
        Comm_WriteReg( (U32)&rSE_RETURN_FVALUE_LOW(ucSEID), 1, &(aKeyValue[SE_2ND_MSK_GROUP].LowDw) );
        Comm_WriteReg( (U32)&rSE_RETURN_FVALUE_HIGH(ucSEID), 1, &(aKeyValue[SE_2ND_MSK_GROUP].HighDw) );
    }
    else
    {
        tStatusReg.bSGroupFind = FALSE;
    }

    tStatusReg.bStable = TRUE;
    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(ucSEID), 1, (U8 *)&tStatusReg);

    return;
}

void SE_SearchOverlapReadParam( U8 ucSEID, SIMSE_OVERLAP_PARAM *pParam )
{
    Comm_ReadReg( (U32)&rSE_LIST_BASE_ADDRESS(ucSEID), 1, (U32 *)&(pParam->ulListAddr) );
    Comm_ReadReg( (U32)&rSE_LIST_ITEM_NUM(ucSEID), 1, (U32 *)&(pParam->ulListLenQW) );
    Comm_ReadReg( (U32)&rSE_KEY_START_VALUE(ucSEID), 1, (U32 *)&(pParam->ulKeyStartVal) );
    Comm_ReadReg( (U32)&rSE_KEY_ACCESS_LEN(ucSEID), 1, (U32 *)&(pParam->ulKeyLen) );
}

void SE_SearchOverlapProcess(U8 ucSEID)
{
    SIMSE_OVERLAP_PARAM tParam;
    U32 ulItemCnt;
    U32 i;
    BOOL bOTFB = SE_IsOtfbEn(ucSEID);
    U32 ulItemAddr;
    SIMSE_OVERLAP_LIST_ITEM tItem;
    U32 ulItemEndVal;
    U32 ulKeyEndVal;
    U8 ucBitMapDWID;
    U8 ucBitMapBitID;
    U32 aBitMap[SE_OVERLAP_BITMAP_LEMDW] = {0};
    SE_STATUS_REG tStatusReg = {0};

    SE_SearchOverlapReadParam(ucSEID, &tParam);
    ulItemCnt = tParam.ulListLenQW;
    ulKeyEndVal = tParam.ulKeyStartVal + tParam.ulKeyLen - 1;

    for ( ulItemAddr = tParam.ulListAddr, i = 0; i < ulItemCnt; ulItemAddr += sizeof(U32) * 2, i++  )
    {
        if ( FALSE == bOTFB )
        {
            Comm_ReadDram( ulItemAddr, 2, (U32 *)&tItem);
        }
        else
        {
            Comm_ReadOtfb( ulItemAddr, 2, (U32 *)&tItem);
        }

        ulItemEndVal = tItem.ulStartValue + tItem.ulAccessLen - 1;        

        if ( (tItem.ulStartValue <= ulKeyEndVal )
            && (ulItemEndVal >= tParam.ulKeyStartVal) )
        {
            ucBitMapDWID = i / DWORD_BIT_SIZE;
            ucBitMapBitID = i % DWORD_BIT_SIZE;

            aBitMap[ucBitMapDWID] |= (1 << ucBitMapBitID);
        }
    }

    Comm_WriteReg( (U32)&rSE_OVERLAP_BITMAP(ucSEID,0), SE_OVERLAP_BITMAP_LEMDW, &(aBitMap[0]) );

    tStatusReg.bOverlapDone = TRUE;
    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(ucSEID), 1, (U8 *)&tStatusReg );

    return;
}

/********************************************************
*Return: U8 - the index of lowest bit 1 in a DWORD,
              INVALID_2F means not find bit 1
*********************************************************/
U8 SE_SearchBitOneInDW( U32 ulData, U8 ucStartIndex )
{
    U8 i;

    for ( i = ucStartIndex; i < DWORD_BIT_SIZE; i++)
    {
        if ( 0 != ( ulData & (1 << i) ) )
        {
            break;
        }
    }

    if ( i >= DWORD_BIT_SIZE )
    {
        return INVALID_2F;
    }
    else
    {
        return i;
    }
}

/********************************************************
*Return: U8 - the index of lowest bit 0 in a DWORD,
              INVALID_2F means not find bit 0
*********************************************************/
U8 SE_SearchBitZeroInDW( U32 ulData, U8 ucStartIndex )
{
    U8 i;

    for ( i = ucStartIndex; i < DWORD_BIT_SIZE; i++)
    {
        if ( 0 != ( (~ulData) & (1 << i) ) )
        {
            break;
        }
    }

    if ( i >= DWORD_BIT_SIZE )
    {
        return INVALID_2F;
    }
    else
    {
        return i;
    }
}

void SE_SearchRegBitProcess(U8 ucSEID, BOOL bBit, BOOL bLoad)
{
    U8 ucFindReg = 0;
    U8 ucFindIndex;
    U8 ucRegStartIndex;
    SE_STATUS_REG tStatusReg = {0};

    if ( TRUE ==  bLoad )
    {
        Comm_ReadReg( (U32)&rSE_BIT_SEARCH_DATA(ucSEID), 1, &l_ulBitSearchRegData );
        ucRegStartIndex = 0;
    }
    else
    {
        ucRegStartIndex = l_ucRegLastIndex + 1;
    }

    if ( 1 == bBit )
    {
        ucFindIndex = SE_SearchBitOneInDW(l_ulBitSearchRegData, ucRegStartIndex);
        l_ucRegLastIndex = ucFindIndex;

        if ( INVALID_2F != ucFindIndex )
        {
            ucFindReg |= MSK_BIT_FIND;
            SE_WriteRegByByte( (U32)&rSE_REG_BIT1_INDEX(ucSEID), 1, &ucFindIndex);
        }
        SE_WriteRegByByte( (U32)&rSE_REG_BIT1_FIND(ucSEID), 1, &ucFindReg);
    }
    else
    {
        ucFindIndex = SE_SearchBitZeroInDW(l_ulBitSearchRegData, ucRegStartIndex);
        l_ucRegLastIndex = ucFindIndex;

        if ( INVALID_2F != ucFindIndex )
        {
            ucFindReg |= MSK_BIT_FIND;
            SE_WriteRegByByte( (U32)&rSE_REG_BIT0_INDEX(ucSEID), 1, &ucFindIndex);
        }
        SE_WriteRegByByte( (U32)&rSE_REG_BIT0_FIND(ucSEID), 1, &ucFindReg);
    }

    tStatusReg.bBitSearchDone = TRUE;
    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(ucSEID), 1, (U8 *)&tStatusReg );

    return;
}

void SE_SearchMemBitProcess(U8 ucSEID, BOOL bBit, BOOL bLoad)
{
    BOOL bOTFB = SE_IsOtfbEn(ucSEID);
    U32 *pDataDW;
    U32 ulDataAddr;
    U32 ulData;
    U32 ulDWIndex;
    U8 ucBitInDW;
    U8 ucFindReg = 0;
    U16 usFindIndex = INVALID_4F;
    U32 ulStartDWIndex;
    U8 ucStartBitInDW;
    SE_STATUS_REG tStatusReg = {0};

    if ( TRUE == bLoad)
    {
        Comm_ReadReg( (U32)&rSE_BIT_SEARCH_ADDRESS(ucSEID), 1, &ulDataAddr );
        SE_ReadRegByByte( (U32)&rSE_BIT_SEARCH_SIZE(ucSEID), 2, (U8 *)&l_usBitMemSizeQW );

        if ( FALSE == bOTFB )
        {
            Comm_ReadDram( ulDataAddr, l_usBitMemSizeQW * 2, l_pSearchBitMemData);
        }
        else
        {
            Comm_ReadOtfb( ulDataAddr, l_usBitMemSizeQW * 2, l_pSearchBitMemData);
        }

        ulStartDWIndex = 0;
        ucStartBitInDW = 0;
    }
    else
    {
        ulStartDWIndex = (l_usMemLastIndex + 1)/DWORD_BIT_SIZE;
        ucStartBitInDW = (l_usMemLastIndex + 1)%DWORD_BIT_SIZE;
    }

    for ( pDataDW = l_pSearchBitMemData + ulStartDWIndex, ulDWIndex = ulStartDWIndex; ulDWIndex < (U32)(l_usBitMemSizeQW * 2); pDataDW++, ulDWIndex++ )
    {
        ulData = *pDataDW;        

        if ( 1 == bBit )
        {
            ucBitInDW = SE_SearchBitOneInDW(ulData, ucStartBitInDW);

            if ( INVALID_2F != ucBitInDW )
            {
                ucFindReg |= MSK_BIT_FIND;
                usFindIndex = ulDWIndex * DWORD_BIT_SIZE + ucBitInDW;
                SE_WriteRegByByte( (U32)&rSE_MEM_BIT1_INDEX(ucSEID), 2, (U8 *)&usFindIndex);
                break;
            }
        }
        else
        {
            ucBitInDW = SE_SearchBitZeroInDW(ulData, ucStartBitInDW);

            if ( INVALID_2F != ucBitInDW )
            {
                ucFindReg |= MSK_BIT_FIND;
                usFindIndex = ulDWIndex * DWORD_BIT_SIZE + ucBitInDW;
                SE_WriteRegByByte( (U32)&rSE_MEM_BIT0_INDEX(ucSEID), 2, (U8 *)&usFindIndex);
                break;
            }
        }

        ucStartBitInDW = 0;
    }//for ( ulDWIndex = 0; ulDWIndex < usLenQW * 2; ulDataAddr += sizeof(U32), ulDWIndex++ )

    l_usMemLastIndex = usFindIndex;

    if ( 1 == bBit )
    {
        SE_WriteRegByByte( (U32)&rSE_MEM_BIT1_FIND(ucSEID), 1, &ucFindReg);
    }
    else
    {
        SE_WriteRegByByte( (U32)&rSE_MEM_BIT0_FIND(ucSEID), 1, &ucFindReg);
    }

    tStatusReg.bBitSearchDone = TRUE;
    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(ucSEID), 1, (U8 *)&tStatusReg );
}

void SE_SetBitProcess(U8 ucSEID, BOOL bBitValue)
{
    BOOL bOTFB = SE_IsOtfbEn(ucSEID);
    U32 ulStartAddr;
    U16 usBitIndex;
    U32 ulSetAddr;
    U16 usBitInDW;
    U32 ulDataTemp;
    SE_STATUS_REG tStatusReg = {0};

    Comm_ReadReg( (U32)&rSE_BIT_SET_ADDRESS(ucSEID), 1, &ulStartAddr );
    SE_ReadRegByByte( (U32)&rSE_BIT_SET_INDEX(ucSEID), 2, (U8 *)&usBitIndex );

    ulSetAddr = ulStartAddr + (usBitIndex/DWORD_BIT_SIZE)*sizeof(U32);
    usBitInDW = usBitIndex%DWORD_BIT_SIZE;

    if ( FALSE == bOTFB )
    {
        Comm_ReadDram( ulSetAddr, 1, &ulDataTemp);
    }
    else
    {
        Comm_ReadOtfb( ulSetAddr, 1, &ulDataTemp);
    }

    if ( 1 == bBitValue )
    {
        ulDataTemp |= (1 << usBitInDW);
    }
    else
    {
        ulDataTemp &= ( ~(1 << usBitInDW) );
    }

    if ( FALSE == bOTFB )
    {
        Comm_WriteDram( ulSetAddr, 1, &ulDataTemp);
    }
    else
    {
        Comm_WriteOtfb( ulSetAddr, 1, &ulDataTemp);
    }

    tStatusReg.bBitSetDone = TRUE;
    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(ucSEID), 1, (U8 *)&tStatusReg );

    return;
}

void SE_ModelInit(void)
{
    U8 ucRegValue = 0;

    SE_WriteRegByByte( (U32)&rSE_TRIGGER_SIZE_REG(SE0), 1, &ucRegValue);
    SE_WriteRegByByte( (U32)&rSE_TRIGGER_SIZE_REG(SE1), 1, &ucRegValue);

    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(SE0), 1, &ucRegValue);
    SE_WriteRegByByte( (U32)&rSE_STATUS_REG(SE1), 1, &ucRegValue);

    l_ulBitSearchRegData = 0;
    l_ucRegLastIndex = 0;
    l_usMemLastIndex = 0;
    l_pSearchBitMemData = (U32*)malloc(0x10000 * 8);//512KB
}

void SE_ModelProcess(U8 ucSEID)
{
    SE_TRIGGER_SIZE_REG tTrigSizeReg;

    SE_ReadRegByByte( (U32)&rSE_TRIGGER_SIZE_REG(ucSEID), 1, (U8 *)&tTrigSizeReg);

    if ( (SE_ITEM_SIZE_DWORD == tTrigSizeReg.bsItemSize) || (SE_ITEM_SIZE_QWORD == tTrigSizeReg.bsItemSize) )
    {
        /* search max/min/same */
        SE_SearchValueProcess(ucSEID);
    }
    else if ( SE_SEARCH_OVERLAP == tTrigSizeReg.bsItemSize )        
    {
        /* search range overlap*/
        SE_SearchOverlapProcess(ucSEID);
    }
    else if ( (SE_SEARCH_MEM_BIT0 <= tTrigSizeReg.bsItemSize) && (SE_SEARCH_REG_BIT1_LOAD >= tTrigSizeReg.bsItemSize) )
    {
        /* search bit 0/1 */
        switch ( tTrigSizeReg.bsItemSize )
        {
        case SE_SEARCH_MEM_BIT0:
            SE_SearchMemBitProcess(ucSEID, 0, FALSE);
            break;

        case SE_SEARCH_MEM_BIT0_LOAD:
            SE_SearchMemBitProcess(ucSEID, 0, TRUE);
            break;

        case SE_SEARCH_MEM_BIT1:
            SE_SearchMemBitProcess(ucSEID, 1, FALSE);
            break;

        case SE_SEARCH_MEM_BIT1_LOAD:
            SE_SearchMemBitProcess(ucSEID, 1, TRUE);
            break;

        case SE_SEARCH_REG_BIT0:
            SE_SearchRegBitProcess(ucSEID, 0, FALSE);
            break;

        case SE_SEARCH_REG_BIT0_LOAD:
            SE_SearchRegBitProcess(ucSEID, 0, TRUE);
            break;

        case SE_SEARCH_REG_BIT1:
            SE_SearchRegBitProcess(ucSEID, 1, FALSE);
            break;

        case SE_SEARCH_REG_BIT1_LOAD:
            SE_SearchRegBitProcess(ucSEID, 1, TRUE);
            break;

        default:
            break;
        }
    }
    else if ( SE_SET_BIT0 == tTrigSizeReg.bsItemSize )  
    {
        /* set bit 0 */ 
        SE_SetBitProcess(ucSEID, 0);
    }
    else if ( SE_SET_BIT1 == tTrigSizeReg.bsItemSize )
    {
        /* set bit 1 */ 
        SE_SetBitProcess(ucSEID, 1);
    }
    else
    {
        /* not support */
    }

    tTrigSizeReg.bsTrigger = 0;
    SE_WriteRegByByte( (U32)&rSE_TRIGGER_SIZE_REG(ucSEID), 1, (U8 *)&tTrigSizeReg);

    return;
}

BOOL SERegWrite(U32 regaddr ,U32 regvalue ,U32 nsize)
{
    SE_TRIGGER_SIZE_REG *pTrigReg;

    if ( regaddr == (U32)&rSE_TRIGGER_SIZE_REG(SE0) )
    {
        pTrigReg = (SE_TRIGGER_SIZE_REG *)&regvalue;

        if ( TRUE == pTrigReg->bsTrigger )
        {
            SE_WriteRegByByte( regaddr, nsize, (U8 *)&regvalue);

            SE_ModelProcess(SE0);

            return FALSE;
        }
    }
    else if ( regaddr == (U32)&rSE_TRIGGER_SIZE_REG(SE1) )
    {
        pTrigReg = (SE_TRIGGER_SIZE_REG *)&regvalue;

        if ( TRUE == pTrigReg->bsTrigger )
        {
            SE_WriteRegByByte( regaddr, nsize, (U8 *)&regvalue);

            SE_ModelProcess(SE1);

            return FALSE;
        }
    } 
    else if ( regaddr == (U32)&rSE_TRIGGER_SIZE_REG(SE2) )
    {
        pTrigReg = (SE_TRIGGER_SIZE_REG *)&regvalue;

        if ( TRUE == pTrigReg->bsTrigger )
        {
            SE_WriteRegByByte( regaddr, nsize, (U8 *)&regvalue);

            SE_ModelProcess(SE2);

            return FALSE;
        }
    } 

    return TRUE;
}

