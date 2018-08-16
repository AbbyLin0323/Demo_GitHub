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
Description :
*******************************************************************************/
#include "HAL_MemoryMap.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_XOR.h"
#include "HAL_XOR_Common.h"

#ifdef SIM
#include "Sim_XOR_Interface.h"
#endif


#define XOR_CWCNT_INDEX_BY_PAGESIZE_INDEX(x)    ((x) + 3)


LOCAL XOR_ENGINE l_aXorE[XORE_CNT];
LOCAL const U8 l_aCodeWordCntValueSets[] = XOR_CWCNT_VALUE_SETS;


// IMPLEMENTATION OF LOCAL FUNCTION THAT HOLD UP INTERFACES

U32 XOR_PartialOperateRevise(U32 ulXoreId, U32 *pStartCodeWord, U32 ulCodeWordCnt)
{
    U32 ulCWCntOfXorPage = XOR_CWCNT_BY_PAGESIZE_INDEX(l_aXorE[ulXoreId].pConfigReg->bsPageSize);

    ASSERT(ulXoreId < XORE_CNT);
    ASSERT(pStartCodeWord != NULL);
    ASSERT(*pStartCodeWord < ulCWCntOfXorPage);
    ASSERT((ulCodeWordCnt > 0) && (ulCodeWordCnt <= ulCWCntOfXorPage));
    ASSERT((*pStartCodeWord + ulCodeWordCnt) <= ulCWCntOfXorPage);

    U32 i = 0;
    U32 ulRevisedCWCntIndex = 0;
    U32 ulRevisedCWCnt = 0;

    // Revise ulCodeWordCnt to hardware valid value.
    for (i = 0; i < (sizeof(l_aCodeWordCntValueSets) / sizeof(*l_aCodeWordCntValueSets)); ++i)
    {
        if (l_aCodeWordCntValueSets[i] >= ulCodeWordCnt)
        {
            ulRevisedCWCnt = l_aCodeWordCntValueSets[i];
            ulRevisedCWCntIndex = i;
            break;
        }
    }

    // Revise StartCodeWord according to the revised ulCodeWordCnt.
    if ((*pStartCodeWord + ulRevisedCWCnt) > ulCWCntOfXorPage)
    {
        *pStartCodeWord = ulCWCntOfXorPage - ulRevisedCWCnt;
    }

    return ulRevisedCWCntIndex;
}


// IMPLEMENTATION OF INTERFACE

void HAL_XorInit(BOOL bInterruptEn, BOOL bPowerManagementEn)
{
    ASSERT((bInterruptEn == TRUE) || (bInterruptEn == FALSE));
    ASSERT((bPowerManagementEn == TRUE) || (bPowerManagementEn == FALSE));
    ASSERT(PLN_PER_LUN > 0);
    ASSERT((LOGIC_PG_SZ == (8 * KB_SIZE)) || (LOGIC_PG_SZ == (16 * KB_SIZE)) || (LOGIC_PG_SZ == (32 * KB_SIZE)));
    
    U32 i = 0;
    U32 ulRegisterAddr = 0;
    volatile XORE_CFG_REG   *pConfigReg = NULL;

    for (i = 0; i < XORE_CNT; ++i)
    {
        ulRegisterAddr = REG_BASE_XOR + i * sizeof(XORE_CFG_REG);
        // In current XOR hardware design, put all kinds of register together, so current drier's all kinds of
        // register pointer point to the same base address, they use placeholder ensure their member on the right position.
        l_aXorE[i].pConfigReg = (volatile XORE_CFG_REG *)ulRegisterAddr;
        l_aXorE[i].pStatusReg = (volatile XORE_STS_REG *)ulRegisterAddr;
        l_aXorE[i].pControlReg = (volatile XORE_CTRL_REG *)ulRegisterAddr;
        l_aXorE[i].bIsParityNeedStore = FALSE;

        pConfigReg = l_aXorE[i].pConfigReg;
        pConfigReg->bsPlaneInfo = (U8)((PLN_PER_LUN == 1) ? 0 : ((LOGIC_PG_SZ >> 14) + 1));
        pConfigReg->bsIntEn = bInterruptEn;
        pConfigReg->bsPmEn = bPowerManagementEn;

        // XOR hardware don't need to set DSG valid, but they haven't removed these register, so we just
        // set it FALSE forever.
        pConfigReg->bsSetDsgVldEnNf = FALSE;        

#if defined(FLASH_TLC)   // TLC:1(48KB) + 4(16KB) XOR engine.
        // The first one XOR engine is only for TLC write.
        if (i < TLC_WRITE_XORE_CNT)
        {
            pConfigReg->bsParityOtfbOffset = i * OTFB_SRAM_PER_TLC_WRITE_XORE;
            pConfigReg->bsRedunOtfbOffset = i * MAX_REDUN_PER_TLC_WRITE_XORE;
            pConfigReg->bsIsTlcWriteNf = TRUE;
        }
        else
        {
            pConfigReg->bsParityOtfbOffset = ((i - TLC_WRITE_XORE_CNT) * OTFB_SRAM_PER_GENERAL_XORE) +
                (TLC_WRITE_XORE_CNT * OTFB_SRAM_PER_TLC_WRITE_XORE);
            pConfigReg->bsRedunOtfbOffset = ((i - TLC_WRITE_XORE_CNT) * MAX_REDUN_PER_GENERAL_XORE) +
                (TLC_WRITE_XORE_CNT * MAX_REDUN_PER_TLC_WRITE_XORE);
            pConfigReg->bsIsTlcWriteNf = FALSE;
        }
#elif (defined(FLASH_3D_MLC)||defined(FLASH_INTEL_3DTLC)||defined(FLASH_YMTC_3D_MLC))
        pConfigReg->bsParityOtfbOffset = i * OTFB_SRAM_PER_GENERAL_XORE;

        if (i < ONE_PASS_3DMLC_WRITE_XORE_CNT)
        {
            pConfigReg->bsRedunOtfbOffset = i * MAX_REDUN_PER_3DMLC_WRITE_XORE;
            pConfigReg->bsIsTlcWriteNf = TRUE;
        }
        else
        {
            pConfigReg->bsRedunOtfbOffset = ((i - ONE_PASS_3DMLC_WRITE_XORE_CNT) * MAX_REDUN_PER_GENERAL_XORE) +
                (ONE_PASS_3DMLC_WRITE_XORE_CNT * MAX_REDUN_PER_3DMLC_WRITE_XORE);
            pConfigReg->bsIsTlcWriteNf = FALSE;
        }
#elif (defined(FLASH_L85) || defined(FLASH_L95) || defined(FLASH_TSB))   // MLC:8(16KB) XOR engine.
        pConfigReg->bsParityOtfbOffset = i * OTFB_SRAM_PER_GENERAL_XORE;
        pConfigReg->bsRedunOtfbOffset = i * MAX_REDUN_PER_GENERAL_XORE;
        pConfigReg->bsIsTlcWriteNf = FALSE;
#endif // FLASH_TLC
    }

    return;
}

BOOL HAL_GetXore(U32 ulXorPageSize, U32 *pXoreId)
{
    ASSERT((ulXorPageSize == (8 * KB_SIZE)) || (ulXorPageSize == (16 * KB_SIZE)) ||
        (ulXorPageSize == (32 * KB_SIZE)) || (ulXorPageSize == (48 * KB_SIZE)) ||
        (ulXorPageSize == (64 * KB_SIZE)));
    ASSERT(pXoreId != NULL);

    U32 i = 0;
    BOOL bRequestResult = FALSE;
    U32 ulXoreSearchStart = 0;
    U32 ulXoreSearchEnd = XORE_CNT;

#if defined(FLASH_TLC)   // TLC:1(48KB) + 4(16KB) XOR engine.
    if (ulXorPageSize == (48 * KB_SIZE))
    {
        ulXoreSearchEnd = TLC_WRITE_XORE_CNT;
    }
    else
    {
        ulXoreSearchStart = TLC_WRITE_XORE_CNT;
    }
#elif (defined(FLASH_3D_MLC)||defined(FLASH_INTEL_3DTLC)||defined(FLASH_YMTC_3D_MLC))
    if (ulXorPageSize == (32 * KB_SIZE))
    {
        ulXoreSearchEnd = ONE_PASS_3DMLC_WRITE_XORE_CNT;
    }
    else
    {
        ulXoreSearchStart = ONE_PASS_3DMLC_WRITE_XORE_CNT;
    }
#elif (defined(FLASH_L85) || defined(FLASH_L95) || defined(FLASH_TSB))   // MLC:8(16KB) XOR engine.
// These flash needn't to do anything.
#else
#error "Flash type must be defined!"
#endif // FLASH_TLC

    // Search a valid XOR engine and hold it.
    for (i = ulXoreSearchStart; i < ulXoreSearchEnd; ++i)
    {
        if ((l_aXorE[i].pConfigReg->bsValid == TRUE) && (l_aXorE[i].bIsParityNeedStore == FALSE))
        {
            l_aXorE[i].pConfigReg->bsValid = FALSE;

            if (ulXorPageSize == (48 * KB_SIZE))
            {
                l_aXorE[i].bIsParityNeedStore = TRUE;
            }
            
            *pXoreId = i;
            bRequestResult = TRUE;
            break;
        }
    }

    // Request successful, we will set some default configure item of the XOR engine.
    if (bRequestResult == TRUE)
    {
        volatile XORE_CFG_REG   *pConfigReg = l_aXorE[*pXoreId].pConfigReg;

        pConfigReg->bsPageSize = ulXorPageSize >> 14;

        // Default behavior: Do whole Page operation in every mode.
        pConfigReg->bsStartCw = 0;
        pConfigReg->bsCodeWordCnt = XOR_CWCNT_INDEX_BY_PAGESIZE_INDEX(ulXorPageSize >> 14);

        // Default behavior: XOR engine is not responsible for setting DSG valid; XOR engine don't move the XOR
        // parity to DRAM automatically.
        pConfigReg->bsSetDsgVldEnNf = FALSE;
        pConfigReg->bsAutoLoadEnNfBp = FALSE;
    }

    return bRequestResult;
}

void HAL_ReleaseXore(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((l_aXorE[ulXoreId].pConfigReg->bsValid == FALSE) ||
        (l_aXorE[ulXoreId].bIsParityNeedStore == TRUE));

    l_aXorE[ulXoreId].pConfigReg->bsValid = TRUE;
    l_aXorE[ulXoreId].bIsParityNeedStore = FALSE;

    return;
}

void HAL_XorePartialOperate(U32 ulXoreId, U32 ulStartCodeWord, U32 ulCodeWordCnt)
{
    U32 ulCWCntOfXorPage = XOR_CWCNT_BY_PAGESIZE_INDEX(l_aXorE[ulXoreId].pConfigReg->bsPageSize);

    ASSERT(ulXoreId < XORE_CNT);
    ASSERT(ulStartCodeWord < ulCWCntOfXorPage);
    ASSERT((ulCodeWordCnt > 0) && (ulCodeWordCnt <= ulCWCntOfXorPage));
    ASSERT((ulStartCodeWord + ulCodeWordCnt) <= ulCWCntOfXorPage);

    U32 ulCodeWordCntHw = XOR_PartialOperateRevise(ulXoreId, &ulStartCodeWord, ulCodeWordCnt);

    l_aXorE[ulXoreId].pConfigReg->bsStartCw = ulStartCodeWord;
    l_aXorE[ulXoreId].pConfigReg->bsCodeWordCnt = ulCodeWordCntHw;

    return;
}

// @todo if we can calculate the ulPreRdyPageCnt value automatically?
void HAL_XoreNfcModeConfig(U32 ulXoreId, XOR_TARGET eTarget, U32 ulPageProtectRatio,
    U32 ulPreRdyPageCnt, BOOL bIsSemiFinishedWork)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((eTarget >= 0) && (eTarget < XOR_TARGET_ASSERT));
    ASSERT((ulPageProtectRatio > 1) && (ulPageProtectRatio <= BIT(8)));
    ASSERT(ulPreRdyPageCnt < PLN_PER_LUN);
    ASSERT((bIsSemiFinishedWork == TRUE) || (bIsSemiFinishedWork == FALSE));

    volatile XORE_CFG_REG   *pConfigReg = l_aXorE[ulXoreId].pConfigReg;

    pConfigReg->bsMode = XOR_NFC_MODE;
    pConfigReg->bsTarget = eTarget;
    // In register, value 3 represent use 3 user data page to generate XOR page.
    // In firmware if protect ratio is 64, it represents that use 63 user data
    // pages to generate one XOR page, So need to minus 1.
    pConfigReg->bsPageCountNfBp = ulPageProtectRatio - 1;
    pConfigReg->bsPreRdyPageCntNf = ulPreRdyPageCnt;
    // Stop a XOR engine and store the parity data to somewhere, then start it to
    // continue do the unfinished part, we need set the "bsHave1stPageNfBp" to 0.
    pConfigReg->bsHave1stPageNfBp = !bIsSemiFinishedWork;

    return;
}

void HAL_XoreAutoLoad(U32 ulXoreId, U32 ulDestXorPageDramAddr, U32 ulDestRedunDramAddr)
{
    ASSERT(ulXoreId < XORE_CNT);

    l_aXorE[ulXoreId].pConfigReg->bsAutoLoadEnNfBp = TRUE;
    l_aXorE[ulXoreId].pConfigReg->tDestDramAddr.ulPage = ulDestXorPageDramAddr - DRAM_START_ADDRESS;
    l_aXorE[ulXoreId].pConfigReg->tDestDramAddr.ulRedundant = ulDestRedunDramAddr - DRAM_START_ADDRESS;

    return;
}

void HAL_XoreBpsModeConfig(U32 ulXoreId, XOR_TARGET eTarget, U32 ulPageProtectRatio,
    U32 ulRedunLength, BOOL bDramCrcEn)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((eTarget >= 0) && (eTarget < XOR_TARGET_ASSERT));
    ASSERT((ulPageProtectRatio > 1) && (ulPageProtectRatio <= BIT(8)));
    ASSERT((ulRedunLength <= MAX_REDUN_PER_GENERAL_XORE) && (ulRedunLength % 8 == 0));
    ASSERT((bDramCrcEn == TRUE) || (bDramCrcEn == FALSE));

    volatile XORE_CFG_REG   *pConfigReg = l_aXorE[ulXoreId].pConfigReg;

    pConfigReg->bsMode = XOR_BPSNFC_MODE;
    pConfigReg->bsTarget = eTarget;
    // In register, value 3 represent use 3 user data page to generate XOR page.
    // In firmware if protect ratio is 64, it represents that use 63 user data
    // pages to generate one XOR page, So need to minus 1.
    pConfigReg->bsPageCountNfBp = ulPageProtectRatio - 1;
    pConfigReg->bsRedunLengthBpLs = ulRedunLength >> 3;
    pConfigReg->bsDramCrcEnBpLs = bDramCrcEn;
    // Set bufferId nonzero value, only when we want to regard one XOR engine as two or more.
    pConfigReg->bsBufferIdBpLs = 0;
    pConfigReg->bsEnBpLs = TRUE;

    return;
}

// Only used in bypass NFC mode, will be called repeatedly.
void HAL_XoreSetSrcDataAddr(U32 ulXoreId, const XOR_SRC_DATA *pSourceDataAddr, BOOL bHaveFirstXorPage)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT(pSourceDataAddr != NULL);
    ASSERT((pSourceDataAddr->ulValidAddrCnt > 0) &&
           (pSourceDataAddr->ulValidAddrCnt <= XOR_SRC_DADDR_REG_CNT));
    ASSERT((bHaveFirstXorPage == TRUE) || (bHaveFirstXorPage == FALSE));

    U32 i = 0;
    volatile XORE_CFG_REG   *pConfigReg = l_aXorE[ulXoreId].pConfigReg;

    pConfigReg->bsSrcDramAddrVldBp = 0;
    pConfigReg->bsHave1stPageNfBp = bHaveFirstXorPage;

    for (i = 0; i < pSourceDataAddr->ulValidAddrCnt; ++i)
    {
        pConfigReg->aSrcDramAddrBp[i].ulPage = pSourceDataAddr->aPageDramAddr[i] - DRAM_START_ADDRESS;
        pConfigReg->aSrcDramAddrBp[i].ulRedundant = pSourceDataAddr->aRedunDramAddr[i] - DRAM_START_ADDRESS;
        pConfigReg->bsSrcDramAddrVldBp |= BIT(i);
    }

    return;
}

void HAL_XoreLoadStoreModeConfig(U32 ulXoreId, XOR_TARGET eTarget, U32 ulXorPageDramAddr,
    U32 ulRedunDramAddr, U32 ulRedunLength, BOOL bDramCrcEn)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((eTarget >= 0) && (eTarget < XOR_TARGET_ASSERT));
    ASSERT((ulRedunLength <= MAX_REDUN_PER_GENERAL_XORE) && (ulRedunLength % 8 == 0));
    ASSERT((bDramCrcEn == TRUE) || (bDramCrcEn == FALSE));

    volatile XORE_CFG_REG   *pConfigReg = l_aXorE[ulXoreId].pConfigReg;

    pConfigReg->bsMode = XOR_LOADSTORE_MODE;
    pConfigReg->bsTarget = eTarget;

    if (eTarget == XOR_LOAD_FROM_DRAM)
    {
        pConfigReg->aSrcDramAddrBp[0].ulPage = ulXorPageDramAddr - DRAM_START_ADDRESS;
        pConfigReg->aSrcDramAddrBp[0].ulRedundant = ulRedunDramAddr - DRAM_START_ADDRESS;
    } 
    else
    {
        pConfigReg->tDestDramAddr.ulPage = ulXorPageDramAddr - DRAM_START_ADDRESS;
        pConfigReg->tDestDramAddr.ulRedundant = ulRedunDramAddr - DRAM_START_ADDRESS;
    }

    pConfigReg->bsRedunLengthBpLs = ulRedunLength >> 3;
    pConfigReg->bsDramCrcEnBpLs = bDramCrcEn;
    // Set bufferId nonzero value, only when we want to regard one XOR engine as two or more.
    pConfigReg->bsBufferIdBpLs = 0;
    pConfigReg->bsEnBpLs = TRUE;

    return;
}

void HAL_XoreTrigger(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    l_aXorE[ulXoreId].pControlReg->bsStop = FALSE;
    l_aXorE[ulXoreId].pControlReg->bsTrigger = TRUE;

#ifdef SIM
    XORM_ITrigger(ulXoreId);
#endif

    return;
}

void HAL_XoreStop(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    l_aXorE[ulXoreId].pControlReg->bsStop = TRUE;

    return;
}

XOR_SATUS HAL_XoreGetStatus(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return l_aXorE[ulXoreId].pStatusReg->bsCurStatus;
}

U32 HAL_XoreGetDonePageCount(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return l_aXorE[ulXoreId].pStatusReg->bsDonePageCount;
}

U32 HAL_XoreGetParityPageAddr(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return (l_aXorE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE);
}

void HAL_XoreSetXorPageSize(U32 ulXoreId, U32 ulXorPageSize)
{
    ASSERT(ulXoreId < XORE_CNT);
    ASSERT((ulXorPageSize == (8 * KB_SIZE)) || (ulXorPageSize == (16 * KB_SIZE)) ||
        (ulXorPageSize == (32 * KB_SIZE)) || (ulXorPageSize == (48 * KB_SIZE)) ||
        (ulXorPageSize == (64 * KB_SIZE)));

    l_aXorE[ulXoreId].pConfigReg->bsPageSize = ulXorPageSize >> 14;

    return;
}

U32 HAL_XoreGetParityPageOffset(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return l_aXorE[ulXoreId].pConfigReg->bsParityOtfbOffset;
}

void HAL_XoreSetParityPageOffset(U32 ulXoreId, U32 ulParityOtfbOffset)
{
    ASSERT(ulXoreId < XORE_CNT);

    l_aXorE[ulXoreId].pConfigReg->bsParityOtfbOffset = ulParityOtfbOffset;

    return;
}

U32 HAL_XoreGetParityRedunOffset(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    return l_aXorE[ulXoreId].pConfigReg->bsRedunOtfbOffset;
}

void HAL_XoreSetParityRedunOffset(U32 ulXoreId, U32 ulRedunOtfbOffset)
{
    ASSERT(ulXoreId < XORE_CNT);

    l_aXorE[ulXoreId].pConfigReg->bsRedunOtfbOffset = ulRedunOtfbOffset;

    return;
}


BOOL XorUt_IsXoreValid(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);
    return l_aXorE[ulXoreId].pConfigReg->bsValid;
}

void XorUt_SetXoreValidOnly(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    U32 i = 0;
    
    for (i = 0; i < XORE_CNT; ++i)
    {
        l_aXorE[i].pConfigReg->bsValid = FALSE;
    }

    l_aXorE[ulXoreId].pConfigReg->bsValid = TRUE;
    
    return;
}

