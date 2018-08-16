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
#include "L3_DataRecover.h"
#include "HAL_XOR.h"
#include "L3_Interface.h"
#include "HAL_Xtensa.h"
#include "L2_Interface.h"
#include "L3_Debug.h"
#include "FW_Debug.h"
#include "HAL_FlashDriverExt.h"

LOCAL const U32 l_kXorRedunBaseAddr = 0x00012000;
// The count of bit of XOR stripe ID in FCMD entry.
LOCAL const U8 l_kXorStripeIdBits = 4;
// Current implement of XOR mapping table use just an array, the items' count of the array equal to
// the count of XOR engine, and the value of each item is the XOR stripe ID corresponds to the XOR
// engine. Therefore, for current implement the key of XOR mapping table is the value of item, the
// value of XOR mapping table is the index of the array.
LOCAL U8 l_aXorMapTable[XORE_CNT] = {0};

LOCAL U8 l_kXorProtectRatio = 4;

void L3_XorInit(void)
{
    // Set the 0x68 offset rigester of GLB, the 27th bit is XOR enable bit. 
    rGLB(0x68) |= (1 << 27);
    // XOR relevant fields of NFC register.
    HAL_NfcXorInit(l_kXorRedunBaseAddr);
    HAL_XorInit(FALSE, TRUE);
    L3_XorMapInit();
    return;
}

void L3_XorMapInit(void)
{
    U32 i = 0;
    for (i = 0; i < XORE_CNT; ++i)
    {
        // Initialize all the value to invalid XOR stripe ID, must not use 0, if you do that, you
        // can't let the XOR stripe ID which sent by L2 to be 0.
        l_aXorMapTable[i] = BIT(l_kXorStripeIdBits);
    }
}

LOCAL void L3_XorMapInsert(U32 ulKey, U32 ulValue)
{
    ASSERT(ulKey < (U32)BIT(l_kXorStripeIdBits));
    ASSERT(ulValue < XORE_CNT);

    l_aXorMapTable[ulValue] = ulKey;
}

LOCAL BOOL L3_XorMapFind(U32 ulKey, U32 *pValue)
{
    ASSERT(ulKey < (U32)BIT(l_kXorStripeIdBits));
    ASSERT(pValue != NULL);

    U32 i = 0;
    for (i = 0; i < XORE_CNT; ++i)
    {
        if (l_aXorMapTable[i] == ulKey)
        {
            *pValue = i;
            return TRUE;
        }
    }

    return FALSE;
}

LOCAL void L3_XorMapErase(U32 ulKey)
{
    ASSERT(ulKey < (U32)BIT(l_kXorStripeIdBits));

    U32 ulValue = 0;
    BOOL bResult = L3_XorMapFind(ulKey, &ulValue);
    ASSERT(bResult == TRUE);

    l_aXorMapTable[ulValue] = BIT(l_kXorStripeIdBits);
    return;
}

BOOL L3_IsNeedDoXor(const FCMD_REQ_ENTRY *ptReqEntry)
{
    ASSERT(ptReqEntry != NULL);

    if ((FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType) && (TRUE == ptReqEntry->bsXorEn))
    {
        // For TLC only the first program cycle need to do XOR.
        FCMD_REQ_BLK_MOD eBlockMode = ptReqEntry->tFlashDesc.bsBlkMod;
        ASSERT((eBlockMode == FCMD_REQ_SLC_BLK) || (eBlockMode == FCMD_REQ_MLC_BLK) ||
               (eBlockMode == FCMD_REQ_TLC_BLK));

        if (eBlockMode == FCMD_REQ_TLC_BLK)
        {
            if (0 == HAL_FlashGetTlcPrgCycle(ptReqEntry->tFlashDesc.bsVirPage))
            {
                return TRUE;
            }
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

// Use this to detect some special commands, such as the command containing XOR data of SLC write,
// the 1st program stage of the command containing XOR data of TLC write.
BOOL L3_IsDataDisperse(const FCMD_REQ_ENTRY *ptReqEntry)
{
    ASSERT(ptReqEntry != NULL);

    if ((TRUE == L3_IsNeedDoXor(ptReqEntry)) && (TRUE == ptReqEntry->bsContainXorData))
    {
        return TRUE;
    }
    return FALSE;
}

// The code that enabled by this function only is used to pach the TLC XOR 6 DSG issue, so if this
// hardware bug were fixed, these code would be removed.
BOOL L3_IsTlcXor6DsgIssue(const FCMD_REQ_ENTRY *ptReqEntry)
{
    ASSERT(ptReqEntry != NULL);

    FCMD_REQ_BLK_MOD eBlockMode = ptReqEntry->tFlashDesc.bsBlkMod;
    ASSERT((eBlockMode == FCMD_REQ_SLC_BLK) || (eBlockMode == FCMD_REQ_MLC_BLK) ||
           (eBlockMode == FCMD_REQ_TLC_BLK));

    if ((eBlockMode == FCMD_REQ_TLC_BLK) && (TRUE == L3_IsDataDisperse(ptReqEntry)))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL L3_AllocXore(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    ASSERT(ptCtrlEntry != NULL);
    ASSERT(ptCtrlEntry->ptReqEntry != NULL);

    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U32 ulXorStripeId = ptReqEntry->bsXorStripeId;
    ASSERT(ulXorStripeId < (U32)BIT(l_kXorStripeIdBits));
    U32 ulXoreId = 0;

    if (TRUE == L3_XorMapFind(ulXorStripeId, &ulXoreId))
    {
        ptCtrlEntry->bsXoreId = ulXoreId;
        PRINT_DEBUG("L3_AllocXore: Find Existing XOR engine:%d. XOR Stripe ID:%d.\n", ulXoreId,
                    ulXorStripeId);
        return TRUE;
    }

    FCMD_REQ_BLK_MOD eBlockMode = ptReqEntry->tFlashDesc.bsBlkMod;
    ASSERT((eBlockMode == FCMD_REQ_SLC_BLK) || (eBlockMode == FCMD_REQ_MLC_BLK) ||
           (eBlockMode == FCMD_REQ_TLC_BLK));
    U32 ulXorPageSize = (eBlockMode == FCMD_REQ_TLC_BLK) ? 
                        PHYPG_SZ * INTRPG_PER_PGADDR * PGADDR_PER_PRG : PHYPG_SZ;

    if (TRUE == HAL_GetXore(ulXorPageSize, &ulXoreId))
    {
        FCMD_REQ_SUBTYPE eReqSubType = ptReqEntry->bsReqSubType;
        ASSERT((eReqSubType == FCMD_REQ_SUBTYPE_SINGLE) || (eReqSubType == FCMD_REQ_SUBTYPE_NORMAL));
        U32 ulPreRdyPageCnt = (eReqSubType == FCMD_REQ_SUBTYPE_SINGLE) ? 0 : (PLN_PER_LUN - 1);

        HAL_XoreNfcModeConfig(ulXoreId, XOR_PROTECT, l_kXorProtectRatio, ulPreRdyPageCnt,
                                FALSE);
        HAL_XoreTrigger(ulXoreId);

        L3_XorMapInsert(ulXorStripeId, ulXoreId);
        ptCtrlEntry->bsXoreId = ulXoreId;
        PRINT_DEBUG("L3_AllocXore: Request New XOR engine:%d success.\n", ulXoreId);
        return TRUE;
    }

    return FALSE; 
}

LOCAL U32 L3_GetTlcXorCopyBackDataAddr(const FCMD_REQ_ENTRY *ptReqEntry, U32 ulPageIndex)
{
    ASSERT(ptReqEntry != NULL);
    ASSERT(ulPageIndex < 3);
    ASSERT(FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType);

    return ((ptReqEntry->atBufDesc[ulPageIndex].bsBufID << BUF_SIZE_BITS) + DRAM_START_ADDRESS +
            (LOGIC_PIPE_PG_SZ - LOGIC_PG_SZ));
}

LOCAL U32 L3_GetTlcXorCopyBackRedunAddr(const FCMD_REQ_ENTRY *ptReqEntry, U32 ulPageIndex)
{
    ASSERT(ptReqEntry != NULL);
    ASSERT(ulPageIndex < 3);
    ASSERT(FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType);

    U32 ulRedunBaseAddr = RED_ABSOLUTE_ADDR(MCU1_ID, ptReqEntry->bsTLun, ptReqEntry->bsReqPtr);
    return ulRedunBaseAddr + ((ulPageIndex + 1) * RED_SW_SZ_DW * sizeof(U32) - RED_SZ);
}

LOCAL void L3_TlcXorCopyBackToDram(const FCMD_REQ_ENTRY *ptReqEntry, U32 ulXoreId)
{
    ASSERT(ptReqEntry != NULL);

    U32 i = 0;
    XOR_SATUS eResult = XOR_IDLE;
    U32 ulParityPageDataOffset = HAL_XoreGetParityPageOffset(ulXoreId);
    U32 ulParityPageRedunOffset = HAL_XoreGetParityRedunOffset(ulXoreId);

    U32 ulOldParityPageOffset = ulParityPageDataOffset;
    U32 ulOldParityRedunOffset = ulParityPageRedunOffset;

    HAL_XoreSetXorPageSize(ulXoreId, PHYPG_SZ);
    HAL_XorePartialOperate(ulXoreId, 0, PHYPG_SZ / CW_INFO_SZ);

    for (i = 0; i < 3; ++i)
    {
        HAL_XoreSetParityPageOffset(ulXoreId, ulParityPageDataOffset);
        HAL_XoreSetParityRedunOffset(ulXoreId, ulParityPageRedunOffset);

        U32 ulParityPageDataAddr = L3_GetTlcXorCopyBackDataAddr(ptReqEntry, i);
        U32 ulParityPageRedunAddr = L3_GetTlcXorCopyBackRedunAddr(ptReqEntry, i);

        HAL_XoreLoadStoreModeConfig(ulXoreId, XOR_STORE_TO_DRAM, ulParityPageDataAddr,
                                    ulParityPageRedunAddr, RED_SZ, FALSE);
        PRINT_DEBUG("L3_TlcXorCopyBackToDram: Step %d, OTFB Page Offset:0x%x,"
                    "OTFB Redun Offset:0x%x, DRAM Page Addr:0x%x, DRAM Redun Addr:0x%x.\n", i,
                    ulParityPageDataOffset, ulParityPageRedunOffset, ulParityPageDataAddr,
                    ulParityPageRedunAddr);
        HAL_XoreTrigger(ulXoreId);

        do
        {
            eResult = HAL_XoreGetStatus(ulXoreId);
        } while (eResult != XOR_FINISH);

        ulParityPageDataOffset += PHYPG_SZ;
        ulParityPageRedunOffset += RED_SZ;
    }

    HAL_XoreSetParityPageOffset(ulXoreId, ulOldParityPageOffset);
    HAL_XoreSetParityRedunOffset(ulXoreId, ulOldParityRedunOffset);
    return;
}

BOOL L3_XoreRelease(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    ASSERT(ptCtrlEntry != NULL);
    ASSERT(ptCtrlEntry->ptReqEntry != NULL);

    BOOL bXoreReleaseFinish = FALSE;
    FCMD_REQ_BLK_MOD eBlockMode = ptCtrlEntry->ptReqEntry->tFlashDesc.bsBlkMod;
    ASSERT((eBlockMode == FCMD_REQ_SLC_BLK) || (eBlockMode == FCMD_REQ_MLC_BLK) ||
           (eBlockMode == FCMD_REQ_TLC_BLK));

    if (eBlockMode == FCMD_REQ_TLC_BLK)
    {
        ASSERT(ptCtrlEntry->bsMultiStep > 0);

        if (--(ptCtrlEntry->bsMultiStep) == 0)
        {
            // The TLC write command that contains XOR data is finished. Firstly, we need to copy
            // XOR data back to DRAM, and then release the XOR engine.
            L3_TlcXorCopyBackToDram(ptCtrlEntry->ptReqEntry, ptCtrlEntry->bsXoreId);

            HAL_ReleaseXore(ptCtrlEntry->bsXoreId);
            L3_XorMapErase(ptCtrlEntry->ptReqEntry->bsXorStripeId);
            bXoreReleaseFinish = TRUE;
            PRINT_DEBUG("L3_XoreRelease: The Data Dispersed TLC XOR Command Finished! XOR engine %d"
                        " is released!\n", ptCtrlEntry->bsXoreId);
        }
        else
        {
            PRINT_DEBUG("L3_XoreRelease: The Data Dispersed TLC XOR Command, Multi Step:%d\n",
                        ptCtrlEntry->bsMultiStep);
            ptCtrlEntry->tDTxCtrl.bsBdEngineDone = FALSE;
            ptCtrlEntry->tDTxCtrl.bsFstEngineID = INVALID_4F;
            #ifdef SIM
            L3_DbgFCmdCntDec(ptCtrlEntry);
            #endif
            L3_IFSendNormalFCmd(ptCtrlEntry);
        }
    }
    else
    {
        // The SLC or MLC needn't to do the copy back action, so release the XOR engine directly.
        HAL_ReleaseXore(ptCtrlEntry->bsXoreId);
        L3_XorMapErase(ptCtrlEntry->ptReqEntry->bsXorStripeId);
        bXoreReleaseFinish = TRUE;
        PRINT_DEBUG("L3_XoreRelease: The Data Dispersed SLC XOR Command Finished! XOR engine %d is "
                    "released!\n", ptCtrlEntry->bsXoreId);
    }

    return bXoreReleaseFinish;
}

void L3_IFSetNFCQXor(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    ASSERT(ptNfcqEntry != NULL);
    ASSERT(ptCtrlEntry != NULL);

    ptNfcqEntry->bsXorBufId = 0;
    ptNfcqEntry->bsXorEn = TRUE;
    ptNfcqEntry->bsXorId = ptCtrlEntry->bsXoreId;

    if (TRUE == L3_IsTlcXor6DsgIssue(ptCtrlEntry->ptReqEntry))
    {
        ptNfcqEntry->bsParPgPos = 3 - ptCtrlEntry->bsMultiStep;
    }
    return;
}

/*==============================================================================
Func Name  : L3_DataRecovery
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
void L3_DataRecovery(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;

    //DBG_Printf("DataRecovery Pending...\n");
    
    ptErrHEntry->tUeccHCtrl.bsSubStage = DATA_RECO_FAIL;

    return;
}



/*====================End of this file========================================*/

