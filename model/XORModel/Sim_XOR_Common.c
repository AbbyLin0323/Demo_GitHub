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
#include "Sim_XOR_Common.h"
#include "HAL_MemoryMap.h"
#include "sim_NormalDSG.h"
#include <string.h>

#define XORM_MAX_DONE_TIMES 256
#define XORM_REDUN_LENGTH_BY_REG(x) (x * 8)
// (x-1)*16 + (!(x-1))*8; Get Code-Word count of single plane page size. Note x will be evaluated 2 times.
// And user must ensure size of "x" is bigger than "U8", because of "*  KB_SIZE_BITS".
#define XORM_CWCNT_PER_SPLN_BY_PLANEINFO(x)          (((((x - 1) * 16) + ((!(x - 1)) * 8)) * KB_SIZE) / CW_INFO_SZ)
#define XORM_PAGE_SIZE_BY_REG(x)     ((((x) * 16) + (!(x) * 8)) * KB_SIZE)
#define XORM_CW_START_BY_REG(x)      (x * (1 << KB_SIZE_BITS))
#define XORM_CW_LENGTH_BY_REG(x)     (x < 4) ? (1 << x) << KB_SIZE_BITS : ((x - 3) << 4) << KB_SIZE_BITS

XORM_ENGINE g_aXorModelE[XORE_CNT];
XORM g_xor_model;
LOCAL const U8 l_aCodeWordCntValueSets[] = XOR_CWCNT_VALUE_SETS;

void XORM_UpdateRegister(U32 ulXoreId, XORM_UPDATE_TYPE eUpdateType)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((eUpdateType >= 0) && (eUpdateType < XORM_UPDATE_TYPE_ASSERT));

    volatile XORE_CFG_REG   *pConfigReg  = g_aXorModelE[ulXoreId].pConfigReg;
    volatile XORE_CTRL_REG  *pControlReg = g_aXorModelE[ulXoreId].pControlReg;
    volatile XORE_STS_REG   *pStatusReg = g_aXorModelE[ulXoreId].pStatusReg;

    switch (eUpdateType)
    {
        case XORM_TRIGGER:
        {
            pControlReg->bsTrigger = FALSE;
            pStatusReg->bsCurStatus = XOR_CALCULATING;
            break;
        }
        case XORM_1CW_CALCULATING:
        {
            pStatusReg->bsCurStatus = XOR_CALCULATING;
            break;
        }
        case XORM_1CW_CALCULATED:
        {
            // Logic of update "bsDonePageCount" register.
            U32 ulMinCwDoneTimes = XORM_GetMinCwDoneTimes(ulXoreId);

            if (ulMinCwDoneTimes > pStatusReg->bsDonePageCount)
            {
                // A new page complete XOR calculation.
                pStatusReg->bsDonePageCount = ulMinCwDoneTimes;

                // Logic of update "bsCurStatus" register.
                if (pStatusReg->bsDonePageCount == pConfigReg->bsPageCountNfBp)
                {                    
                    if (pConfigReg->bsAutoLoadEnNfBp == TRUE)
                    {
                        XorM_AutoLoadToDram(ulXoreId);
                    }

                    pStatusReg->bsCurStatus = XOR_FINISH;
                    XORM_SwitchInternalStatus(ulXoreId, XORM_FINISH);
                }
                else
                {
                    pStatusReg->bsCurStatus = XOR_PARTIAL_FINISH;
                }

                // Update parity ready signal to NFC model.
                if ((pConfigReg->bsMode == XOR_NFC_MODE) && (pConfigReg->bsTarget == XOR_PROTECT))
                {
                    XORM_UpdateRegister(ulXoreId, XORM_PARITY_READY);
                }
            }
            else if (ulMinCwDoneTimes < pStatusReg->bsDonePageCount)
            {
                DBG_Getch();  // Impossible situation.
            }
            
            break;
        }
        case XORM_PARITY_READY:
        {
            U32 ulRemainderPageCnt = pConfigReg->bsPageCountNfBp - pStatusReg->bsDonePageCount;

            if (ulRemainderPageCnt == pConfigReg->bsPreRdyPageCntNf)
            {
                g_aXorModelE[ulXoreId].bParityReady = TRUE;
            }

            break;
        }
        default:
            DBG_Getch();
    }

    return;
}

void XORM_SwitchInternalStatus(U32 ulXoreId, XORM_INTERNAL_STATUS eTargetStatus)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((eTargetStatus >= 0) && (eTargetStatus < XORM_INTERNAL_STATUS_ASSERT));

    g_aXorModelE[ulXoreId].eInternalStatus = eTargetStatus;
    return;
}

U32 XORM_GetMinCwDoneTimes(U32 ulXoreId)
{
    // The XorM_IReceiveConfigFromNfc must be called before this function. 
    ASSERT(g_aXorModelE[ulXoreId].eInternalStatus == XORM_SEND_CONFIG_NFC);
    
    ASSERT(ulXoreId < XORE_CNT);

    U32 i = 0;
    U32 ulMinDoneTimes = XORM_MAX_DONE_TIMES;
    U32 ulStartCodeWord = g_aXorModelE[ulXoreId].pConfigReg->bsStartCw;
    U32 ulEndCodeWord = ulStartCodeWord + XorM_GetCodeWordCntByReg(ulXoreId);

    // Only care the Code-Word in [ulStartCodeWord, ulEndCodeWord].
    for (i = ulStartCodeWord; i < ulEndCodeWord; ++i)
    {
        if (g_aXorModelE[ulXoreId].usCodeWordDoneTimes[i] < ulMinDoneTimes)
        {
            ulMinDoneTimes = g_aXorModelE[ulXoreId].usCodeWordDoneTimes[i];
        }
    }

    return ulMinDoneTimes;
}

void XorM_ConfigXoreFromNfc(U32 ulXoreId, U32 ulBufferId, U32 ulRedunLength, BOOL bDramCrcEn)
{
    // The XORM_Trigger must be called before this function. If XOR engine
    // do a partial task, then this function may be called after internal status
    // switch to finish.
    ASSERT((g_aXorModelE[ulXoreId].eInternalStatus == XORM_TRIGGERED) ||
        (g_aXorModelE[ulXoreId].eInternalStatus == XORM_SEND_CONFIG_NFC) ||
        (g_aXorModelE[ulXoreId].eInternalStatus == XORM_FINISH));

    ASSERT(ulXoreId < XORE_CNT);
    ASSERT(ulBufferId < 16);
    ASSERT((ulRedunLength <= 64) && (ulRedunLength % 8 == 0));
    ASSERT((bDramCrcEn == TRUE) || (bDramCrcEn == FALSE));

    g_aXorModelE[ulXoreId].pConfigReg->bsBufferIdBpLs = ulBufferId;
    g_aXorModelE[ulXoreId].pConfigReg->bsRedunLengthBpLs = ulRedunLength >> 3;
    g_aXorModelE[ulXoreId].pConfigReg->bsEnBpLs = TRUE;
    g_aXorModelE[ulXoreId].pConfigReg->bsDramCrcEnBpLs = bDramCrcEn;

    if (g_aXorModelE[ulXoreId].eInternalStatus != XORM_FINISH)
    {
        XORM_SwitchInternalStatus(ulXoreId, XORM_SEND_CONFIG_NFC);
    }

    return;
}

U32 XorM_GetCwCntOfSinglePlane(U32 xor_page_size_index, U32 plane_info)
{
    return (plane_info == 0) ? XOR_CWCNT_BY_PAGESIZE_INDEX(xor_page_size_index) :
        XORM_CWCNT_PER_SPLN_BY_PLANEINFO(plane_info);
}

void XORM_XorCalculate(U32 ulXoreId, const U32 *pData, U32 ulCodeWordIndex)
{ 
    // The XorM_IReceiveConfigFromNfc must be called before this function. 
    ASSERT(g_aXorModelE[ulXoreId].eInternalStatus == XORM_SEND_CONFIG_NFC);
    
    U32 ulCWCntOfXorPage = XOR_CWCNT_BY_PAGESIZE_INDEX(g_aXorModelE[ulXoreId].pConfigReg->bsPageSize);
    
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT(pData != NULL);
    ASSERT(ulCodeWordIndex < ulCWCntOfXorPage);

    U32 i = 0;
    volatile XORE_CFG_REG   *pConfigReg = g_aXorModelE[ulXoreId].pConfigReg;
    U32 ulRedunLength = XORM_REDUN_LENGTH_BY_REG(pConfigReg->bsRedunLengthBpLs);

    U32 *pOtfbCwParity = (U32 *)(XorM_GetParityPageOtfbAddr(ulXoreId) + (ulCodeWordIndex * CW_INFO_SZ));
    U32 *pOtfbRedunParity = (U32 *)(XorM_GetParityRedunOtfbAddr(ulXoreId) +
        (ulCodeWordIndex / (LOGIC_PG_SZ / CW_INFO_SZ)) * ulRedunLength);

    U32 ulCwCntOfSinglePlane = XorM_GetCwCntOfSinglePlane(pConfigReg->bsPageSize,
                                                          pConfigReg->bsPlaneInfo);

    // Update status to calculating.
    XORM_UpdateRegister(ulXoreId, XORM_1CW_CALCULATING);

    // Do XOR page calculation(only the specific Code-Word).
    if (g_aXorModelE[ulXoreId].usCodeWordDoneTimes[ulCodeWordIndex] == 0)
    {
        memcpy(pOtfbCwParity, pData, CW_INFO_SZ);
        
        // Redundant of page is follow the last Code-Word, and its index is same as the last Code-Word.
        if (ulCodeWordIndex % ulCwCntOfSinglePlane == (ulCwCntOfSinglePlane - 1))
        {
            memcpy(pOtfbRedunParity, (pData + CW_INFO_SZ_DW), ulRedunLength);
        }
    }
    else
    {
        // Not the first time, do actual calculating.
        for (i = 0; i < CW_INFO_SZ_DW; ++i)
        {
            pOtfbCwParity[i] = pData[i] ^ pOtfbCwParity[i];
        }

        // Redundant of page is follow the last Code-Word, and its index is same as the last Code-Word.
        if (ulCodeWordIndex % ulCwCntOfSinglePlane == (ulCwCntOfSinglePlane - 1))
        {
            for (i = 0; i < (ulRedunLength / DWORD_SIZE); ++i)
            {
                pOtfbRedunParity[i] = pData[i + CW_INFO_SZ_DW] ^ pOtfbRedunParity[i];
            }
        }
    }

    return;
}

U8 XorM_GetCodeWordCntByReg(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return l_aCodeWordCntValueSets[g_aXorModelE[ulXoreId].pConfigReg->bsCodeWordCnt];
}

U32 XorM_GetParityPageOtfbAddr(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return (g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE);
}

U32 XorM_GetParityRedunOtfbAddr(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return (g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE);
}

void XorM_AutoLoadToDram(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    volatile XORE_CFG_REG   *pConfigReg = g_aXorModelE[ulXoreId].pConfigReg;
    U32 *pOtfbParityPage  = (U32 *)XorM_GetParityPageOtfbAddr(ulXoreId);
    U32 *pOtfbParityRedun = (U32 *)XorM_GetParityRedunOtfbAddr(ulXoreId);
    U32 *pDramParityPage  = (U32 *)(pConfigReg->tDestDramAddr.ulPage + DRAM_START_ADDRESS);
    U32 *pDramParityRedun = (U32 *)(pConfigReg->tDestDramAddr.ulRedundant + DRAM_START_ADDRESS);

    memcpy(pDramParityPage, pOtfbParityPage, XORM_PAGE_SIZE_BY_REG(pConfigReg->bsPageSize));

    if (pConfigReg->bsIsTlcWriteNf == TRUE)
    {
        // @todo TLC write there are three redundant need to be process. Implement it later.
        DBG_Getch();
    }
    else
    {
        memcpy(pDramParityRedun, pOtfbParityRedun, XORM_REDUN_LENGTH_BY_REG(pConfigReg->bsRedunLengthBpLs));
    }

    return;
}

void XORM_Comm_CopyData(XORM_ENGINE_PARAM pXorParam)
{
    U16 i;
    U32 *pSrcData = (U32 *)pXorParam.ulSrcDataAddr;
    U32 *pDstData = (U32 *)pXorParam.ulDstDataAddr;
    U32 *pSrcRed = (U32 *)pXorParam.ulSrcRedAddr;
    U32 *pDstRed = (U32 *)pXorParam.ulDstRedAddr;

    for (i = 0; i < (pXorParam.bsCwLength /sizeof(U32)); i++)
    {
        *pDstData++ = *pSrcData++;
    }

    // If need to copy the redundant, the redundant of page is follow the last Code-Word.
    if ((pXorParam.bsStartCw + pXorParam.bsCwLength) == pXorParam.bsParitySize)
    {
        for (i = 0; i < (pXorParam.bsRedLength / sizeof(U32)); i++)
        {
            *pDstRed++ = *pSrcRed++;
        }
    }

    return;
}

void XORM_XorOperation(XORM_ENGINE_PARAM pXorParam)
{
    U16 i;
    U32 *pSrcData = (U32 *)pXorParam.ulSrcDataAddr;
    U32 *pDstData = (U32 *)pXorParam.ulDstDataAddr;
    U32 *pSrcRed = (U32 *)pXorParam.ulSrcRedAddr;
    U32 *pDstRed = (U32 *)pXorParam.ulDstRedAddr;

    for (i = 0; i < (pXorParam.bsCwLength / sizeof(U32)); i++)
    {
        *pDstData = *pSrcData ^ *pDstData;
        pSrcData++;
        pDstData++;

    }

    // If need to calculate the redundant, the redundant of page is follow the last Code - Word.
    if ((pXorParam.bsStartCw + pXorParam.bsCwLength) == pXorParam.bsParitySize)
    {
        for (i = 0; i < (pXorParam.bsRedLength / sizeof(U32)); i++)
        {
            *pDstRed = *pSrcRed ^ *pDstRed;
            pSrcRed++;
            pDstRed++;
        }
    }

    return;
}

void XORM_ClearXoreCommon(U32 ulXoreId)
{
    U32 *pTargetData = (U32 *)(g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE);
    U32 *pTargetRed = (U32 *)(g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE);

    g_aXorModelE[ulXoreId].pStatusReg->bsCurStatus = XOR_IDLE;
    g_aXorModelE[ulXoreId].pStatusReg->bsDonePageCount = 0;
    g_aXorModelE[ulXoreId].eInternalStatus = XORM_IDLE;

    memset(pTargetData, 0, XORM_PAGE_SIZE_BY_REG(g_aXorModelE[ulXoreId].pConfigReg->bsPageSize));
    memset(pTargetRed, 0, XORM_REDUN_LENGTH_BY_REG(g_aXorModelE[ulXoreId].pConfigReg->bsRedunLengthBpLs));

}

void XORM_GetXoreParam(XORM_ENGINE_PARAM *pXorParam, U32 ulXoreId)
{
    pXorParam->bsXoreId = ulXoreId;
    pXorParam->bsFinishTime = g_aXorModelE[ulXoreId].pConfigReg->bsPageCountNfBp;
    pXorParam->bsParitySize = XORM_PAGE_SIZE_BY_REG(g_aXorModelE[ulXoreId].pConfigReg->bsPageSize);
    pXorParam->bsStartCw = XORM_CW_START_BY_REG(g_aXorModelE[ulXoreId].pConfigReg->bsStartCw);
    pXorParam->bsCwLength = XORM_CW_LENGTH_BY_REG(g_aXorModelE[ulXoreId].pConfigReg->bsCodeWordCnt);
    pXorParam->bsRedLength = XORM_REDUN_LENGTH_BY_REG(g_aXorModelE[ulXoreId].pConfigReg->bsRedunLengthBpLs);

    return;
}

void XORM_CheckCwRange(XORM_ENGINE_PARAM pXorParam)
{
    if (pXorParam.bsStartCw >= pXorParam.bsParitySize)
    {
        DBG_Printf("The start cw addr is over parity size\n");
        DBG_Getch();
    }

    if ((pXorParam.bsStartCw + pXorParam.bsCwLength) > pXorParam.bsParitySize)
    {
        DBG_Printf("The read otfb addr is over the engine range\n");
        DBG_Getch();
    }
}
