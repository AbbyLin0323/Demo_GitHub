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
* NON-INFRINGEMENT.
********************************************************************************
Description :
*******************************************************************************/

#include "Sim_DEC_Status.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_MemoryMap.h"
#include "Sim_nfccmd.h"

//#define MATCH_ASIC

GLOBAL U8 ucFifoIndex = 0;

U8 Dec_GetCwNum(U16 ucSecStart, U16 ucSecLen, U8 ucPln)
{
    U8 ucSecCnt = 0;
    U16 ucIndex;

    for (ucIndex = ucSecStart; ucIndex < (ucSecStart + ucSecLen); ucIndex++)
    {
        if (ucIndex >= (ucPln * SEC_PER_LOGIC_PG) && ucIndex < ((ucPln + 1) * SEC_PER_LOGIC_PG))
        {
            ucSecCnt++;
        }
    }

    return ((ucSecCnt +1) /2);
}


U32 Dec_GetFailBitMap(U8 CwNum)
{
    U32 ulFailBitMap = 0;
    U8 ucIndex;

    for(ucIndex = 0; ucIndex < CwNum; ucIndex++)
    {
#ifndef MATCH_ASIC
        if(0 == ucIndex % 2)
#endif
        {
            ulFailBitMap |= (1 << ucIndex);
        }
    }
    /*Set Last bit to 1*/
    ulFailBitMap |= (1 << (ucIndex - 1));

    return ulFailBitMap;
}

U32 Dec_GetInjFailBitMap (U8 StartCW, U8 CwNum, U8 ucPln)
{
    U8 ucIndex;
    U32 ulFailBitMap = 0;
    U8 ucTotalErrCwCnt = 0;

    for (ucIndex = 0; ucIndex < CwNum; ucIndex++)
    {
        if (((ucIndex + StartCW) >= ucPln * CW_PER_PLN) && ((ucIndex + StartCW) < (ucPln+1) * CW_PER_PLN))
        {
            ulFailBitMap |= (1 << (ucIndex + StartCW - ucPln * CW_PER_PLN));
            ucTotalErrCwCnt++;
        }
    }

    /* High 16bits is total error CW count*/
    ulFailBitMap = ulFailBitMap | (ucTotalErrCwCnt << 16);

    return ulFailBitMap;
}

U32 Dec_GetEmptyPgBitMap(U8 CwNum)
{
    U32 ulFailBitMap = 0;
    U8 ucIndex;

    for(ucIndex = 0; ucIndex < CwNum; ucIndex++)
    {
        ulFailBitMap |= (1 << ucIndex);
    }

    return ulFailBitMap;
}

U8 Dec_CalcOneNum (U32 ulFailBitMap)
{
    U32 i;
    U8 ucOneNum = 0;

    for(i = 0; i < 32; i++)
    {
        if(ulFailBitMap & (1 << i))
        {
            ucOneNum++;
        }
    }

    return ucOneNum;
}
void DecStsM_ReadID(void)
{
    U8 ucPuIndex, ucLunIndex, ucNfcqDepth, ucPlnIndex;
    U32 ulFlashID_0 = 0;
    U32 ulFlashID_1 = 0;

#if defined(FLASH_L95)
    ulFlashID_0 = 0x54E5A42C;
    ulFlashID_1 = 0xA9;
#elif (defined(FLASH_TLC) && defined(FLASH_TSB_3D))
    ulFlashID_0 = 0x123;
    ulFlashID_1 = 0x0;
#elif defined(FLASH_TLC)
    ulFlashID_0 = 0x123;
    ulFlashID_1 = 0x0;
#elif defined(FLASH_TSB)
    ulFlashID_0 = 0x9394DE98;
    ulFlashID_1 = 0x04085076;
#elif defined(FLASH_INTEL_3DTLC)
    ulFlashID_0 = 0x3278b489;
    ulFlashID_1 = 0x000001aa;
#elif defined(FLASH_IM_3DTLC_GEN2)
    ulFlashID_0 = 0x3278b489;
    ulFlashID_1 = 0x000001aa;
#endif

    for (ucPuIndex = 0; ucPuIndex < NFC_PU_MAX; ucPuIndex++)
    {
        for (ucLunIndex = 0; ucLunIndex < NFC_LUN_PER_PU; ucLunIndex++)
        {
            for (ucNfcqDepth = 0; ucNfcqDepth < NFCQ_DEPTH_TOTAL / NFC_LUN_PER_PU; ucNfcqDepth++)
            {
                for (ucPlnIndex = 0; ucPlnIndex < DEC_PLN_MAX_PER_LUN; ucPlnIndex++)
                {
                    g_pModelNfcDecSts->aFlashId[ucPuIndex][ucLunIndex][ucNfcqDepth][ucPlnIndex].ulFlashId0 = ulFlashID_0;
                    g_pModelNfcDecSts->aFlashId[ucPuIndex][ucLunIndex][ucNfcqDepth][ucPlnIndex].ulFlashId1 = ulFlashID_1;
                }
            }
        }
    }

}

void DecStsM_SetFlashSts(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln, U8 ucErrType)
{
    U8 ucPhyPuInTotal = tNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + tNfcOrgStruct->ucCh;
    U8 ucLun = tNfcOrgStruct->ucLunInPhyPu;

    g_pModelNfcDecSts->aFlashStatus[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsFlashStatus= ucErrType;
}

void DecStsM_SRAM_Set_Error(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln, BOOL IsEmptyError, U8 ucErrType, ST_FLASH_CMD_PARAM *pFlashCmdParam)
{
    U8 ucPhyPuInTotal = tNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + tNfcOrgStruct->ucCh;
    U8 ucLun = tNfcOrgStruct->ucLunInPhyPu;
    U8 ucCwNum = 0;
    U16 usCwBit = 0;
    U32 ulInjFailBitMap = 0;
    U8 ucTotalErrCwCnt = 0;

    if (TRUE == pFlashCmdParam->p_nf_cq_entry->bsRedOnly)
    {
        usCwBit = EMPTY_THRD_LAST_1K;
    }
    else
    {
        usCwBit = EMPTY_THRD_FST_15K;
    }

    if( TRUE == pFlashCmdParam->p_nf_cq_entry->bsInjEn)
    {
        /* Use Cmd error injection param to set CwNum & CwBit*/
        if (TRUE == pFlashCmdParam->p_nf_cq_entry->bsRedOnly)
        {
            ulInjFailBitMap = 0x1;
            ucCwNum = 1;
            ucTotalErrCwCnt = 1;
        }
        else
        {
            /*pFlashCmdParam->p_nf_cq_entry->bsInjCwCnt count from 0 snyc to HW*/
            ulInjFailBitMap = Dec_GetInjFailBitMap(pFlashCmdParam->p_nf_cq_entry->bsInjCwStart, pFlashCmdParam->p_nf_cq_entry->bsInjCwCnt+1, ucPln);

            /*Get Total error CwCnt for high 16bits*/
            ucTotalErrCwCnt = (ulInjFailBitMap >> 16) & 0xFFFF;

            /*Clear high 16bits*/
            ulInjFailBitMap = ulInjFailBitMap & 0x0000FFFF;
            ucCwNum = Dec_CalcOneNum(ulInjFailBitMap);
        }
    }
    else
    {
        /* Use NFCQ param to set CwNum & CwBit */
        if (TRUE == pFlashCmdParam->p_nf_cq_entry->bsRedOnly)
        {
            ucCwNum = 1;
        }
        else
        {
            ucCwNum = Dec_GetCwNum(pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecStart, pFlashCmdParam->p_nf_cq_entry->aSecAddr[0].bsSecLength, ucPln);
        }
    }

    if(NF_ERR_TYPE_UECC == ucErrType && TRUE == IsEmptyError)
    {
        /* Read empty page */
        if(ucCwNum > 8)
        {
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = usCwBit * 8;
        }
        else
        {
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = usCwBit * ucCwNum;
        }

        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap = Dec_GetEmptyPgBitMap(ucCwNum);
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulRCrcBitMap = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsFstItrSyndAccu = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc0T1 = 0;
    }

    else if(NF_ERR_TYPE_UECC == ucErrType && FALSE == IsEmptyError)
    {
        /* Read page UECC */
        if(ucCwNum > 8)
        {
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = (usCwBit /2) * 8;
        }
        else
        {
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = (usCwBit /2) * ucCwNum;
        }

        if( TRUE == pFlashCmdParam->p_nf_cq_entry->bsInjEn)
        {

            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap = ulInjFailBitMap;
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc = 40 * (ucTotalErrCwCnt -ucCwNum);
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc0T1 = 20 * (ucTotalErrCwCnt -ucCwNum);
        }
        else
        {
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap = Dec_GetFailBitMap(ucCwNum);
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc = 40 * (ucCwNum - Dec_CalcOneNum(g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap));
            g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc0T1 = 20 * (ucCwNum - Dec_CalcOneNum(g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap));
        }
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulRCrcBitMap = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsFstItrSyndAccu = 0;

    }

    else if(NF_ERR_TYPE_RECC == ucErrType)
    {
        /* Read page RECC */
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulRCrcBitMap = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsFstItrSyndAccu = 0;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc = 40 * ucCwNum;
        g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc0T1 = 20 * ucCwNum;
    }

}

void DecStsM_SRAM_Set_CRCError(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln)
{
    U8 ucPhyPuInTotal = tNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + tNfcOrgStruct->ucCh;
    U8 ucLun = tNfcOrgStruct->ucLunInPhyPu;

    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap = 0x7;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulRCrcBitMap = 0xF;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsFstItrSyndAccu = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc0T1 = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc = 0;
}

void DecStsM_SRAM_Clear_Error(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln)
{
    U8 ucPhyPuInTotal = tNfcOrgStruct->ucPhyPuInCh * NFC_CH_TOTAL + tNfcOrgStruct->ucCh;
    U8 ucLun = tNfcOrgStruct->ucLunInPhyPu;

    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulDecFailBitMap = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].ulRCrcBitMap = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsN1Accu = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsFstItrSyndAccu = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc0T1 = 0;
    g_pModelNfcDecSts->aDecStsSram[ucPhyPuInTotal][ucLun][ucNfcqDepth][ucPln].bsErrCntAcc = 0;
}

void DecStsM_FIFO_Set(const NFCM_LUN_LOCATION *tNfcOrgStruct, U8 ucNfcqDepth, U8 ucPln, ST_FLASH_CMD_PARAM *pFlashCmdParam, BOOL bsErr)
{
    U8 ucPhyPu = tNfcOrgStruct->ucPhyPuInCh;
    U8 ucCh = tNfcOrgStruct->ucCh;
    U8 ucLun = tNfcOrgStruct->ucLunInPhyPu;
    U8 ucCwNum;
    static U16 ulTotalCwNum = 0;

    ASSERT(TRUE != pFlashCmdParam->p_nf_cq_entry->bsRedOnly);

    if(FALSE == bsErr)
    {
        ucCwNum = 0;
    }
    else if( TRUE == pFlashCmdParam->p_nf_cq_entry->bsInjEn )
    {
        /* Use Cmd error injection param to set CwNum & CwBit*/
        ucCwNum = pFlashCmdParam->p_nf_cq_entry->bsInjCwCnt;
    }
    else
    {
        /* Use NFCQ param to set CwNum & CwBit */
        ucCwNum = (pFlashCmdParam->xfer_sec_cnt / pFlashCmdParam->bsPlnNum) / 2;    // translate sector to CW
    }

    ulTotalCwNum = ulTotalCwNum + ucCwNum;

    if((PHY_PAGE_PER_LOGIC_PAGE-1) == ucPln)
    {
        g_pModelDecFifoSts->aDecFifoSts[ucFifoIndex].bsCmdIndex = (pFlashCmdParam->p_nf_cq_entry->bsDecFifoIdx7to1b << 1) | (pFlashCmdParam->p_nf_cq_entry->bsDecFifoIdxLsb);
        g_pModelDecFifoSts->aDecFifoSts[ucFifoIndex].bsPlnNum = ucPln;
        g_pModelDecFifoSts->aDecFifoSts[ucFifoIndex].bsErrCntAcc = 40 * ulTotalCwNum;
        g_pModelDecFifoSts->aDecFifoSts[ucFifoIndex].bsErrCntAcc0T1 = 20 * ulTotalCwNum;
        g_pModelDecFifoSts->aDecFifoSts[ucFifoIndex].bsLdpcDecItr = 20 * ulTotalCwNum;
        g_pModelDecFifoSts->aDecFifoSts[ucFifoIndex].ulDecEngBitMap = 0;

        ucFifoIndex = (ucFifoIndex++);
        //ucFifoIndex = ucFifoIndex % DEC_FIFO_DEPTH;

        g_pModelDecFifoCfg->bsDecFifoWp = ucFifoIndex;
        ulTotalCwNum = 0;
    }
}


U8 DecStsM_Set_SRAM_FIFO(const NFCM_LUN_LOCATION *tNfcOrgStruct, ST_FLASH_CMD_PARAM *pFlashCmdParam, PLANE_ERR_TYPE aPlaneErrType, U8 ucPln)
{
    U8 usErrCode;
    BOOL bIsEmptyPG;
    BOOL bFifoSetErr = FALSE;    

    /* Set DEC Status SRAM */
    usErrCode = aPlaneErrType.ErrCode;
    bIsEmptyPG = aPlaneErrType.IsEmptyPG;

    if (1 == pFlashCmdParam->bsPlnNum)
    {
        ucPln = 0;
    }
    else /* multi-pln partial data read case */
    {
        U16 ulThisPlnSecStart = ucPln * SEC_PER_LOGIC_PG;
        U16 ulThisPlnSecEnd = (ucPln + 1) * SEC_PER_LOGIC_PG;
        if (TRUE != pFlashCmdParam->red_only)
        {
            for (int i = 0; i < NFCQ_SEC_ADDR_COUNT; i++)
            {
                if (pFlashCmdParam->p_nf_cq_entry->aSecAddr[i].bsSecLength == 0)
                {
                    break;;
                }
                else
                {
                    if ((pFlashCmdParam->p_nf_cq_entry->aSecAddr[i].bsSecStart >= ulThisPlnSecEnd) ||
                        (pFlashCmdParam->p_nf_cq_entry->aSecAddr[i].bsSecStart + pFlashCmdParam->p_nf_cq_entry->aSecAddr[i].bsSecLength <= ulThisPlnSecStart))
                    {
                        usErrCode = NF_SUCCESS;
                        break;
                    }
                }
            }                       
        }
    }    

    if((NF_SUCCESS != usErrCode) && (FALSE == pFlashCmdParam->bIsDecSet[ucPln]))
    {
        DecStsM_SRAM_Set_Error(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct), ucPln, bIsEmptyPG, usErrCode, pFlashCmdParam);
        pFlashCmdParam->bIsDecSet[ucPln] = TRUE;
        bFifoSetErr = TRUE;
    }

    if(FALSE == pFlashCmdParam->bIsDecSet[ucPln])
    {
        DecStsM_SRAM_Clear_Error(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct), ucPln);
    }

    /* If need report DEC_STATUS_FIFO, set FIFO entry */
    if (TRUE == pFlashCmdParam->p_nf_cq_entry->bsDecFifoEn)
    {
        if (TRUE == pFlashCmdParam->red_only)
        {
            DBG_Printf("DecStsM error: red_only cmd request set DecFifio\n");
            DBG_Getch();
        }
        else
        {
            DecStsM_FIFO_Set(tNfcOrgStruct, NFC_GetRP(tNfcOrgStruct), ucPln, pFlashCmdParam, bFifoSetErr);
        }
    }

    return usErrCode;
}

