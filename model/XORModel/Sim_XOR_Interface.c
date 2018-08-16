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
#include "windows.h"
#include "Sim_XOR_Interface.h"
#include "Sim_XOR_Common.h"
#include "HAL_MemoryMap.h"
#include <platform/memory_access.h>


// IMPLEMENTATION OF INTERFACE

void XORM_IInit(void)
{
    U32 i = 0;
    U32 ulRegAddr = 0;

    for (i = 0; i < XORE_CNT; ++i)
    {
        ulRegAddr = REG_BASE_XOR + i * sizeof(XORE_CFG_REG);
        // In current XOR hardware design, put all kinds of register together, so current drier's all kinds of
        // register pointer point to the same base address, they use placeholder ensure their member on the right position.
        g_aXorModelE[i].pConfigReg = (volatile XORE_CFG_REG *)ulRegAddr;
        g_aXorModelE[i].pStatusReg = (volatile XORE_STS_REG *)ulRegAddr;
        g_aXorModelE[i].pControlReg = (volatile XORE_CTRL_REG *)ulRegAddr;

        g_aXorModelE[i].pStatusReg->bsCurStatus = XOR_IDLE;
        g_aXorModelE[i].pStatusReg->bsDonePageCount = 0;
        g_aXorModelE[i].pControlReg->bsTrigger = FALSE;
        g_aXorModelE[i].pControlReg->bsStop = FALSE;
        g_aXorModelE[i].pConfigReg->bsValid = TRUE;

        g_aXorModelE[i].eInternalStatus = XORM_IDLE;
        memset(g_aXorModelE[i].usCodeWordDoneTimes, 0, (XORM_MAX_CW_CNT * sizeof(U16)));
        g_aXorModelE[i].bParityReady = FALSE;
    }

    // Set the initial value to a invalid value to avoid use it before "XorM_IReceiveCwConfig" be
    // called.
    g_xor_model.activated_xore_id = XORE_CNT;
    g_xor_model.code_word_index = XORM_MAX_CW_CNT;
    g_xor_model.is_last_cw_done = FALSE;
    g_xor_model.need_care_dram_data = FALSE;
    
    Comm_IRegisterDramObserver(XorM_ICaptureDramData);
    return;
}

void XORM_ITrigger(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    ASSERT(g_aXorModelE[ulXoreId].pControlReg->bsStop == FALSE);
    ASSERT(g_aXorModelE[ulXoreId].pControlReg->bsTrigger == TRUE);

    if (g_aXorModelE[ulXoreId].eInternalStatus == XORM_FINISH)
    {
        g_aXorModelE[ulXoreId].pStatusReg->bsDonePageCount = 0;
    }

    XORM_UpdateRegister(ulXoreId, XORM_TRIGGER);
    XORM_SwitchInternalStatus(ulXoreId, XORM_TRIGGERED);

    switch (g_aXorModelE[ulXoreId].pConfigReg->bsMode)
    {
        case XOR_NFC_MODE:
        {
            memset(g_aXorModelE[ulXoreId].usCodeWordDoneTimes, 0, (XORM_MAX_CW_CNT * sizeof(U16)));
            g_aXorModelE[ulXoreId].bParityReady = FALSE;
            break;
        }
        case XOR_BPSNFC_MODE:
        {
            XORM_BypassNfcMode(ulXoreId);
            break;
        }
        case XOR_LOADSTORE_MODE:
        {
            XORM_LoadStoreMode(ulXoreId);
            break;
        }
        default:
            DBG_Getch();
    }
    
    return;
}

void XorM_IReceiveConfigFromNfc(U32 ulXoreId, U32 ulBufferId, U32 ulRedunLength, BOOL bDramCrcEn)
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

// Before NFC fetch data from DRAM, it should invoke this function to tell XOR which XOR engine is
// responsible to process the Code-Word data will be fetched, the index the Code-Word in whole
// XOR page size, and so on. After calling this funciton, NFC will fetch the Code-Word data through
// some memory read funciotn, the Code-Word data will be passed to XOR model inside memory read 
// function.
void XorM_IReceiveCwConfig(U32 xore_id, U32 code_word_index, U32 buffer_id, U32 atom_redun_length, BOOL dram_crc_en)
{
  ASSERT(xore_id < XORE_CNT);
  // The XORM_Trigger must be called before this function. If XOR engine
  // do a partial task, then this function may be called after internal status
  // switch to finish.
  ASSERT((g_aXorModelE[xore_id].eInternalStatus == XORM_TRIGGERED) ||
         (g_aXorModelE[xore_id].eInternalStatus == XORM_SEND_CONFIG_NFC) ||
         (g_aXorModelE[xore_id].eInternalStatus == XORM_FINISH));
  ASSERT(buffer_id < 16);
  ASSERT((atom_redun_length <= XORM_MAX_ATOM_REDUN) && (atom_redun_length % 8 == 0));
  ASSERT((dram_crc_en == TRUE) || (dram_crc_en == FALSE));
  U32 cw_cnt_of_xor_page = XOR_CWCNT_BY_PAGESIZE_INDEX(g_aXorModelE[xore_id].pConfigReg->bsPageSize);
  ASSERT(code_word_index < cw_cnt_of_xor_page);

  g_xor_model.activated_xore_id = xore_id;
  g_xor_model.code_word_index = code_word_index;
  g_xor_model.need_care_dram_data = TRUE;

  XorM_ConfigXoreFromNfc(xore_id, buffer_id, atom_redun_length, dram_crc_en);
  return;
}

void XorM_ICaptureDramData(U32 *dram_data, U32 size_in_dw)
{
    ASSERT(dram_data != NULL);
    
    if (TRUE == g_xor_model.need_care_dram_data) 
    {
        U32 xore_id = g_xor_model.activated_xore_id;

        // XOR need use XOR engine to do XOR operation with current DRAM data.
        XORM_DATA_BUFFER *data_buffer = &(g_xor_model.data_buffer);
        U32 code_word_index = g_xor_model.code_word_index;

        if (TRUE == g_xor_model.is_last_cw_done)
        {
            COM_MemCpy(data_buffer->atom_redun, dram_data, size_in_dw);
            XORM_ICalculateNFC(xore_id, data_buffer->code_word, code_word_index);
            g_xor_model.is_last_cw_done = FALSE;
            g_xor_model.need_care_dram_data = FALSE;
        }
        else 
        {
            COM_MemCpy(data_buffer->code_word, dram_data, size_in_dw);

            U32 cw_cnt_of_single_plane = XorM_GetCwCntOfSinglePlane(g_aXorModelE[xore_id].pConfigReg->bsPageSize, g_aXorModelE[xore_id].pConfigReg->bsPlaneInfo);

            g_xor_model.is_last_cw_done = (code_word_index % cw_cnt_of_single_plane ==  (cw_cnt_of_single_plane - 1)) ? TRUE : FALSE;

            if (FALSE == g_xor_model.is_last_cw_done)
            {
                XORM_ICalculateNFC(xore_id, data_buffer->code_word, code_word_index);
                g_xor_model.need_care_dram_data = FALSE;
            }
        }
    }
    return;
}


// Calculate 1 Code-Word data each time. So, NFC need to tell XOR engine the index
// of current Code-Word in whole page(size in XOR page size), redundant of page 
// is follow the last Code-Word, and its index is same as the last Code-Word.
void XORM_ICalculateNFC(U32 ulXoreId, const U32 *pData, U32 ulCodeWordIndex)
{
    // The XorM_IReceiveConfigFromNfc must be called before this function. If XOR engine
    // do a partial task, then this function may be called after internal status
    // switch to finish.
    ASSERT((g_aXorModelE[ulXoreId].eInternalStatus == XORM_SEND_CONFIG_NFC) ||
        (g_aXorModelE[ulXoreId].eInternalStatus == XORM_FINISH));

    U32 ulCWCntOfXorPage = XOR_CWCNT_BY_PAGESIZE_INDEX(g_aXorModelE[ulXoreId].pConfigReg->bsPageSize);

    ASSERT(ulXoreId < XORE_CNT);
    ASSERT(pData != NULL);
    ASSERT(ulCodeWordIndex < ulCWCntOfXorPage);

    U32 ulStartCodeWord = g_aXorModelE[ulXoreId].pConfigReg->bsStartCw;
    U32 ulEndCodeWord = ulStartCodeWord + XorM_GetCodeWordCntByReg(ulXoreId);

    // Only do the calculation in [ulStartCodeWord, ulEndCodeWord).
    if ((ulCodeWordIndex >= ulStartCodeWord) && (ulCodeWordIndex < ulEndCodeWord))
    {
        // Do the actual XOR operation.
        XORM_XorCalculate(ulXoreId, pData, ulCodeWordIndex);

        ++(g_aXorModelE[ulXoreId].usCodeWordDoneTimes[ulCodeWordIndex]);

        // Update current XOR engine's all status register.
        XORM_UpdateRegister(ulXoreId, XORM_1CW_CALCULATED);
    }

    return;
}

BOOL XORM_IIsParityReady(U32 ulXoreId)
{
    // The XorM_IReceiveConfigFromNfc must be called before this function. If XOR engine
    // do a single plane task, then this function may be called after internal status
    // switch to finish.
    ASSERT((g_aXorModelE[ulXoreId].eInternalStatus == XORM_TRIGGERED) ||
        (g_aXorModelE[ulXoreId].eInternalStatus == XORM_SEND_CONFIG_NFC) ||
        (g_aXorModelE[ulXoreId].eInternalStatus == XORM_FINISH));

    ASSERT(ulXoreId < XORE_CNT);

    return g_aXorModelE[ulXoreId].bParityReady;
}

void XorM_IReleaseXore(U32 ulXoreId)
{
    // The XOR command must be finished before release. If XOR engine
    // do a single plane task, then this function may be called after internal status
    // switch to finish, then switch to XORM_SEND_CONFIG_NFC, the last page(XOR parity)
    // need the information sent by NFC model.
    ASSERT((g_aXorModelE[ulXoreId].eInternalStatus == XORM_FINISH) ||
        (g_aXorModelE[ulXoreId].eInternalStatus == XORM_SEND_CONFIG_NFC));

    // Only protect data in NFC mode can call this function.
    ASSERT((g_aXorModelE[ulXoreId].pConfigReg->bsMode == XOR_NFC_MODE) &&
        (g_aXorModelE[ulXoreId].pConfigReg->bsTarget == XOR_PROTECT));
    ASSERT(g_aXorModelE[ulXoreId].pConfigReg->bsValid == FALSE);

    ASSERT(ulXoreId < XORE_CNT);

    g_aXorModelE[ulXoreId].pConfigReg->bsValid = TRUE;
    
    return;
}

void XORM_BypassNfcMode(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    XORM_ENGINE_PARAM pXorParam;
    U8 ucSrcAddrValid, ucVaildCnt = 0;

    XORM_GetXoreParam(&pXorParam, ulXoreId);
    XORM_CheckCwRange(pXorParam);

    ucSrcAddrValid = g_aXorModelE[ulXoreId].pConfigReg->bsSrcDramAddrVldBp & 0xF;

    if (0 == (ucSrcAddrValid & 0x1))
    {
        DBG_Printf("g_aXorModelE[%d].pConfigReg->bsSrcDramAddrVldBp setting error!\n", ulXoreId);
        DBG_Getch();
    }

    if (TRUE == g_aXorModelE[ulXoreId].pConfigReg->bsHave1stPageNfBp)
    {
        // Source addr: dram; Destination addr: xor otfb sram
        pXorParam.ulSrcDataAddr = g_aXorModelE[ulXoreId].pConfigReg->aSrcDramAddrBp[0].ulPage + pXorParam.bsStartCw + DRAM_START_ADDRESS;
        pXorParam.ulSrcRedAddr = g_aXorModelE[ulXoreId].pConfigReg->aSrcDramAddrBp[0].ulRedundant + DRAM_START_ADDRESS;
        pXorParam.ulDstDataAddr = g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE + pXorParam.bsStartCw;
        pXorParam.ulDstRedAddr = g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE;

        XORM_Comm_CopyData(pXorParam);
        g_aXorModelE[ulXoreId].pStatusReg->bsDonePageCount++;
        ucSrcAddrValid = (ucSrcAddrValid >> 1);
        ucVaildCnt++;
    }

    while (0 != (ucSrcAddrValid & 0x1))
    {
        // Source addr: dram; Destination addr: xor otfb sram
        pXorParam.ulSrcDataAddr = g_aXorModelE[ulXoreId].pConfigReg->aSrcDramAddrBp[ucVaildCnt].ulPage + pXorParam.bsStartCw + DRAM_START_ADDRESS;
        pXorParam.ulSrcRedAddr = g_aXorModelE[ulXoreId].pConfigReg->aSrcDramAddrBp[ucVaildCnt].ulRedundant + DRAM_START_ADDRESS;
        pXorParam.ulDstDataAddr = g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE + pXorParam.bsStartCw;
        pXorParam.ulDstRedAddr = g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE;

        // Xor calculation with data & redundant
        XORM_XorOperation(pXorParam);

        g_aXorModelE[ulXoreId].pStatusReg->bsDonePageCount++;
        ucSrcAddrValid = (ucSrcAddrValid >> 1);
        ucVaildCnt++;
    }


    if (pXorParam.bsFinishTime == g_aXorModelE[ulXoreId].pStatusReg->bsDonePageCount)
    {
        if (g_aXorModelE[ulXoreId].pConfigReg->bsAutoLoadEnNfBp == TRUE)
        {
            // Source addr: xor otfb sram; Destination addr: dram
            pXorParam.ulSrcDataAddr = g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE + pXorParam.bsStartCw;
            pXorParam.ulSrcRedAddr = g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE;
            pXorParam.ulDstDataAddr = g_aXorModelE[ulXoreId].pConfigReg->tDestDramAddr.ulPage + pXorParam.bsStartCw + DRAM_START_ADDRESS;
            pXorParam.ulDstRedAddr = g_aXorModelE[ulXoreId].pConfigReg->tDestDramAddr.ulRedundant + DRAM_START_ADDRESS;
            XORM_Comm_CopyData(pXorParam);
        }
        g_aXorModelE[ulXoreId].pStatusReg->bsCurStatus = XOR_FINISH;
        XORM_SwitchInternalStatus(ulXoreId, XORM_FINISH);
    }
    else if (g_aXorModelE[ulXoreId].pConfigReg->bsPageCountNfBp > g_aXorModelE[ulXoreId].pStatusReg->bsDonePageCount)
    {
        g_aXorModelE[ulXoreId].pStatusReg->bsCurStatus = XOR_PARTIAL_FINISH;
    }
    else
    {
        DBG_Printf("The finish count can't over the page cnt setting.\n");
        DBG_Getch();
    }

    return;
}

void XORM_LoadStoreMode(U32 ulXoreId)
{
    ASSERT(ulXoreId < XORE_CNT);

    XORM_ENGINE_PARAM pXorParam;

    XORM_GetXoreParam(&pXorParam, ulXoreId);
    XORM_CheckCwRange(pXorParam);

    switch (g_aXorModelE[ulXoreId].pConfigReg->bsTarget)
    {
        case XORM_WT_TO_DRAM:
        {
            // Source addr: xor otfb sram; Destination addr: dram
            pXorParam.ulSrcDataAddr = g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE + pXorParam.bsStartCw;
            pXorParam.ulSrcRedAddr = g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE;
            pXorParam.ulDstDataAddr = g_aXorModelE[ulXoreId].pConfigReg->tDestDramAddr.ulPage + pXorParam.bsStartCw + DRAM_START_ADDRESS;
            pXorParam.ulDstRedAddr = g_aXorModelE[ulXoreId].pConfigReg->tDestDramAddr.ulRedundant + DRAM_START_ADDRESS;
            XORM_Comm_CopyData(pXorParam);
            g_aXorModelE[ulXoreId].pStatusReg->bsCurStatus = XOR_FINISH;
            break;
        }
        case XORM_RD_FORM_DRAM:
        {
            // Source addr: dram; Destination addr: xor otfb sram
            pXorParam.ulSrcDataAddr = g_aXorModelE[ulXoreId].pConfigReg->aSrcDramAddrBp[0].ulPage + pXorParam.bsStartCw + DRAM_START_ADDRESS;
            pXorParam.ulSrcRedAddr = g_aXorModelE[ulXoreId].pConfigReg->aSrcDramAddrBp[0].ulRedundant + DRAM_START_ADDRESS;
            pXorParam.ulDstDataAddr = g_aXorModelE[ulXoreId].pConfigReg->bsParityOtfbOffset + OTFB_XOR_PARITY_BASE + pXorParam.bsStartCw;
            pXorParam.ulDstRedAddr = g_aXorModelE[ulXoreId].pConfigReg->bsRedunOtfbOffset + OTFB_XOR_REDUNDANT_BASE;
            XORM_Comm_CopyData(pXorParam);
            g_aXorModelE[ulXoreId].pStatusReg->bsCurStatus = XOR_FINISH;
           break;
        }
        default:
            DBG_Getch();
    }

    return;
}
