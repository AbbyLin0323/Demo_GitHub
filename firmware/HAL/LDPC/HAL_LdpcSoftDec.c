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
* File Name    : HAL_LdpcSoftDec.c
* Discription  :
* CreateAuthor : Maple Xu
* CreateDate   : 2016.01.15
*===============================================================================
* Modify Record:
*=============================================================================*/
#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_LdpcSoftDec.h"

GLOBAL MCU2_DRAM_TEXT volatile SOFT_MCU1_CFG_REG *pSoftMcu1CfgReg;
GLOBAL MCU2_DRAM_TEXT volatile SOFT_MCU2_CFG_REG *pSoftMcu2CfgReg;
GLOBAL MCU2_DRAM_TEXT volatile SOFT_INT_STS_REG *pSoftIntStsReg;
GLOBAL MCU2_DRAM_TEXT volatile DESCRIPTOR *g_pSoftDesc;

/*32-CW per page, need 16-Desc at most and each Desc cannot cover 4-CW*/
GLOBAL MCU2_DRAM_TEXT SOFT_DECO_CW_SEGMENT g_aCodeWordSegment[DESCRIPTOR_MAX_NUM];
GLOBAL MCU2_DRAM_TEXT SOFT_DECO_CW_SEGMENT g_aCwNumPerDesc[DESCRIPTOR_MAX_NUM];

/* Decode result of descriptor before "g_FifoFwReadPtr" already be taken by fw. */
LOCAL MCU2_DRAM_TEXT U8 l_ucFifoFwReadPtr = 0;
LOCAL MCU2_DRAM_TEXT U8 l_ucFifoFwWritePtr = 0;

LOCAL MCU2_DRAM_TEXT CW_CFG_DESCRIPTOR l_aLdpcSDescCfgCw[DESC_SHARED_CW_NUM];

LOCAL const U8 l_aShiftReadTimesSets[] = { 3, 5, 3, 5, 3, 5, 3, 5, 3, 5 };

/*==============================================================================
Func Name  : HAL_LdpcSInit
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Initialize Soft Decoder related registers
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSInit(void)
{
    pSoftMcu1CfgReg = (volatile SOFT_MCU1_CFG_REG *)LDPC_SOFT_MCU1_CFG_REG_BASE;
    pSoftMcu2CfgReg = (volatile SOFT_MCU2_CFG_REG *)LDPC_SOFT_MCU2_CFG_REG_BASE;
    pSoftIntStsReg  = (volatile SOFT_INT_STS_REG  *)LDPC_SOFT_INT_STS_REG;

    pSoftMcu1CfgReg->bsRDesAddrBase1 = LDPC_SOFT_RDESADDR_BASE1;
    pSoftMcu1CfgReg->bsWptr1 = 0;
    pSoftMcu1CfgReg->bsIntEn = FALSE;
    pSoftMcu1CfgReg->bsSoftMaxIter = LDPC_SOFT_DEC_MAX_ITER;
    pSoftMcu1CfgReg->bsBothMcuMode = LDPC_SOFT_DEC_BOTH_MCU_MODE;

    pSoftMcu2CfgReg->bsRDesAddrBase2 = (pSoftMcu1CfgReg->bsBothMcuMode)
                     ? LDPC_SOFT_RDESADDR_BASE2 : LDPC_SOFT_RDESADDR_BASE1;
    pSoftMcu2CfgReg->bsWptr2 = 0;

    pSoftIntStsReg->bsSoftDecIntSts = 0;

    /* Descriptor base addr */
    g_pSoftDesc = (volatile DESCRIPTOR *) LDPC_SOFT_DESC_BASE_ADDR;
    COM_MemZero((U32 *)g_pSoftDesc, sizeof(DESCRIPTOR)>>2);

    COM_MemZero((U32 *)g_aCodeWordSegment, sizeof(g_aCodeWordSegment)>>2);

    return;
}

/*==============================================================================
Func Name  : HAL_LdpcSGetFifoHwR/Wptr
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: HW RPTR: HW maintained Read pointer
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U8 MCU2_DRAM_TEXT HAL_LdpcSGetFifoHwRptr()
{
    U32 ulMcuId = HAL_GetMcuId();

    if((ulMcuId != MCU1_ID)&&(ulMcuId != MCU2_ID))
    {
        DBG_Getch();
    }

    return ((ulMcuId == MCU1_ID) ? pSoftMcu1CfgReg->bsRptr1 : pSoftMcu2CfgReg->bsRptr2);
}

U8 MCU2_DRAM_TEXT HAL_LdpcSGetFifoHwWptr()
{
    U32 ulMcuId = HAL_GetMcuId();

    if((ulMcuId != MCU1_ID)&&(ulMcuId != MCU2_ID))
    {
        DBG_Getch();
    }

    return ((ulMcuId == MCU1_ID) ? pSoftMcu1CfgReg->bsWptr1 : pSoftMcu2CfgReg->bsWptr2);
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoIsEmpty
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Compair FwRptr & HwWptr
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
    2. Johnson modified by changing FwWptr to HwWptr
==============================================================================*/
BOOL MCU2_DRAM_TEXT HAL_LdpcSFifoIsEmpty()
{
    if (l_ucFifoFwReadPtr == HAL_LdpcSGetFifoHwWptr())
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoWaitEmpty
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription:
Usage      : Used to check if all the descriptors is decoded
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
BOOL MCU2_DRAM_TEXT HAL_LdpcSFifoWaitEmpty()
{
    while(TRUE != HAL_LdpcSFifoIsEmpty())
    {
        HAL_LdpcSFifoPop();
    }

    return TRUE;
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoIsFull
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Compair FwRptr and HwWptr
Usage      : Used before pushing new descriptors into the FIFO
History    :
    1. 2016.01.21 MapleXu create function
    2. Johnson modified by changing FwWptr to HwWptr
==============================================================================*/
BOOL MCU2_DRAM_TEXT HAL_LdpcSFifoIsFull()
{
    if(((HAL_LdpcSGetFifoHwWptr() + 1) % DESCRIPTOR_MAX_NUM) == l_ucFifoFwReadPtr)
    {
        return TRUE;
    }

    return FALSE;
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoGetFreeNum
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
    2. 2016.03.15 Leave L3 update FW RPTR, FifoPop() is only used in driver test
==============================================================================*/
U8 MCU2_DRAM_TEXT HAL_LdpcSFifoGetFreeNum()
{
    U32 ulAvailableDescriptorCnt = 0;
    U32 ulHwWritePtr = HAL_LdpcSGetFifoHwWptr();

    // Get the total available slot number using firmware read pointer.
    if (TRUE == HAL_LdpcSFifoIsEmpty())
    {
        ulAvailableDescriptorCnt = DESCRIPTOR_MAX_NUM;
    }
    else
    {
        ulAvailableDescriptorCnt = ((l_ucFifoFwReadPtr + DESCRIPTOR_MAX_NUM - ulHwWritePtr) % DESCRIPTOR_MAX_NUM);
    }

    // Minus 1 because there is one descriptor can't be used.
    return (ulAvailableDescriptorCnt  - 1);
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoGetDoneNum
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Number of slots that are decoding done and to be poped from FIFO
Usage      :
History    :
    1. 2016.03.09 MapleXu create function
==============================================================================*/
U8 MCU2_DRAM_TEXT HAL_LdpcSFifoGetDoneNum()
{
    U8 ucDecDoneNum, ucFifoHwRptr;

    ucFifoHwRptr = HAL_LdpcSGetFifoHwRptr();
    ucDecDoneNum = (ucFifoHwRptr + DESCRIPTOR_MAX_NUM - l_ucFifoFwReadPtr) % DESCRIPTOR_MAX_NUM;

    return ucDecDoneNum;
}

/*==============================================================================
Func Name  : HAL_LdpcSTrigger
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
    2. 2016.02.25 modify Getch condition to Not zero, for wrapper will clear RPTR&WPTR after dec
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSTrigger(U32 ulDescriptorCnt)
{
    ASSERT((ulDescriptorCnt > 0) && (ulDescriptorCnt <= VALID_DESCRIPTOR_CNT));

    U32 ulMcuId = HAL_GetMcuId();
    U32 ulNewHwWritePtr = (HAL_LdpcSGetFifoHwWptr() + ulDescriptorCnt) % DESCRIPTOR_MAX_NUM;

    if(ulMcuId == MCU1_ID)
    {
        pSoftMcu1CfgReg->bsWptr1 = ulNewHwWritePtr;
    }
    else if (ulMcuId == MCU2_ID)
    {
        pSoftMcu2CfgReg->bsWptr2 = ulNewHwWritePtr;
    }
    else
    {
        DBG_Getch();
    }

    return;
}

void MCU2_DRAM_TEXT HAL_LdpcSDescRelease(U32 ulStartDescriptorId, U32 ulDescriptorCnt)
{
    ASSERT(ulStartDescriptorId < DESCRIPTOR_MAX_NUM);
    ASSERT((ulDescriptorCnt > 0) && (ulDescriptorCnt <= VALID_DESCRIPTOR_CNT));

    U32 i = 0;
    U32 ulDescriptorId = ulStartDescriptorId;
    volatile SOFT_DEC_DESCRIPTOR *pDescriptor = NULL;

    if (ulStartDescriptorId == l_ucFifoFwReadPtr)
    {
        l_ucFifoFwReadPtr = (l_ucFifoFwReadPtr + ulDescriptorCnt) % DESCRIPTOR_MAX_NUM;

        while (LDPC_SOFT_DECO_WAIT_RELEASE == HAL_LdpcSGetDescAddr(l_ucFifoFwReadPtr)->bsRsv3_UsebyFw)
        {
            HAL_LdpcSGetDescAddr(l_ucFifoFwReadPtr)->bsRsv3_UsebyFw = LDPC_SOFT_DECO_RELEASED;
            l_ucFifoFwReadPtr = (l_ucFifoFwReadPtr + 1) % DESCRIPTOR_MAX_NUM;
        }
    }
    else
    {
        for (i = 0; i < ulDescriptorCnt; ++i)
        {
            HAL_LdpcSGetDescAddr(ulDescriptorId)->bsRsv3_UsebyFw = LDPC_SOFT_DECO_WAIT_RELEASE;
            ulDescriptorId = (ulDescriptorId + 1) % DESCRIPTOR_MAX_NUM;
        }
    }

    return;
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoPush
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Write descriptors into HW, update FW WPTR: g_FifoFwWptr, driver
            can use g_FifoFwWptr to trigger HW process these new descriptor later
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSFifoPush(U8 ucNewDescNum)
{
    /* Get new FW WPTR when new descriptors are pushed */
     l_ucFifoFwWritePtr = (ucNewDescNum + l_ucFifoFwWritePtr) % DESCRIPTOR_MAX_NUM;

    /* Trigger to write FW WPTR to register */
    HAL_LdpcSTrigger(ucNewDescNum);

    return;
}

/*==============================================================================
Func Name  : HAL_LdpcSFifoPop
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Update FW RPTR using message from HAL_LdpcSFifoGetDoneNum()
            Should only be used in soft DEC driver test, cannot be used in L3
            because L3 is in chage of FW RPTR update.
Usage      :
History    :
    1. 2016.03.09 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSFifoPop()
{
    U8 ucDecDoneNum;

    /* Get currently decoding done slots number */
    ucDecDoneNum = HAL_LdpcSFifoGetDoneNum();

    /* Update the FW RPTR, cannot be 15 */
    l_ucFifoFwReadPtr = (l_ucFifoFwReadPtr + ucDecDoneNum) % DESCRIPTOR_MAX_NUM;

    return;
}

/*==============================================================================
Func Name  : HAL_LdpcSGetDescId
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Get available descriptor ID from current WPTR
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U32 MCU2_DRAM_TEXT HAL_LdpcSGetDescId()
{
    return HAL_LdpcSGetFifoHwWptr();
}

/*==============================================================================
Func Name  : HAL_LdpcSGetDescAddr
Input      : NONE
Output     : ulDescAddr
Return Val : LOCAL
Discription: Get the descriptor address in SRAM from its ID
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
volatile SOFT_DEC_DESCRIPTOR * MCU2_DRAM_TEXT HAL_LdpcSGetDescAddr(volatile U32 DescID)
{
    return (volatile SOFT_DEC_DESCRIPTOR *) (&g_pSoftDesc->aDescriptor[DescID]);
}

/*==============================================================================
Func Name  : HAL_LdpcSGetReadCnt
Input      : NONE
Output     : ulDescAddr
Return Val : LOCAL
Discription: Get the descriptor address in SRAM from its ID
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
U32 MCU2_DRAM_TEXT HAL_LdpcSGetReadCnt(void)
{
    return (sizeof(l_aShiftReadTimesSets) / sizeof(*l_aShiftReadTimesSets));
}

U32 MCU2_DRAM_TEXT HAL_LdpcSGetReadTime(U32 ulShiftReadTimesIndex)
{
    ASSERT(ulShiftReadTimesIndex < (sizeof(l_aShiftReadTimesSets) / sizeof(*l_aShiftReadTimesSets)));

    return l_aShiftReadTimesSets[ulShiftReadTimesIndex];
}

/*==============================================================================
Func Name  : HAL_LdpcSDescCfgCw
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Descriptor may have more than one CW, use this function to cfg a single one
Usage      :
History    :
    1. 2016.01.27 Maple Xu add function
    2. 2016.02.02 Sync macros to TLC driver
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSDescCfgCw(DESC_EN_CTL *pDescEnCtl, U8 ucStartCwId, U8 ucDescCwCnt)
{
    U8 ucCurCwId = ucStartCwId + ucDescCwCnt;
    U8 ucRmCrcLen = 0;
    U8 ucRmLbaLen = 0;

    /* If CRC is disabled, should remove CRC length */
    if (TRUE != pDescEnCtl->bsCrcEn)
    {
        ucRmCrcLen = DS_CRC_LENTH;
    }

    /* If LBA is disabled, should remove CRC length */
    if (TRUE != pDescEnCtl->bsLbaEn)
    {
        ucRmLbaLen = DS_LBA_LENTH;
    }

    /* Default */
    l_aLdpcSDescCfgCw[ucDescCwCnt].bsInfoLen    = CW_INFO_SZ + DS_CRC_LENTH - ucRmCrcLen;
    l_aLdpcSDescCfgCw[ucDescCwCnt].bsHSel       = LDPC_MAT_SEL_FST_15K;

    /* LBA */
    if (((ucCurCwId%CW_PER_LBA) == (CW_PER_LBA-1)) && (pDescEnCtl->bsLbaEn))
    {
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsInfoLen    = CW_INFO_SZ + DS_CRC_LENTH - ucRmCrcLen + DS_LBA_LENTH - ucRmLbaLen;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsLbaEn      = TRUE;
    }

    /* RED */
    if ((ucCurCwId == (CW_PER_PLN-1)) && (pDescEnCtl->bsRedunEn))
    {
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsHSel       = LDPC_MAT_SEL_LASE_1K;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsInfoLen    = CW_INFO_SZ + DS_CRC_LENTH - ucRmCrcLen + DS_LBA_LENTH - ucRmLbaLen + RED_SZ;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsRedunLen   = (RED_SZ>>3) -1;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsRedunEn    = TRUE;
    }

    return;
}


/*==============================================================================
Func Name  : HAL_LdpcSDescCfgCws
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription: Configure all the CWs related bit in the descriptor
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
    2. 2016.01.27 sync to falsh driver design
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSDescCfgCws(U8 ucPU, U8 ucCurDescId, DESC_EN_CTL *pDescEnCtl)
{
    U8 ucStartCwId;
    U8 ucDescCwNum;
    U8 ucDescCwCnt;

    volatile SOFT_DEC_DESCRIPTOR * pSoftDecDesc;
    pSoftDecDesc = HAL_LdpcSGetDescAddr(ucCurDescId);

    ucDescCwNum = g_aCodeWordSegment[ucCurDescId].bsCodeWordCount;
    ucStartCwId = g_aCodeWordSegment[ucCurDescId].bsStartCodeWord;

    pSoftDecDesc->bsChnN     = HAL_NfcGetPhyPU(ucPU) % NFC_CH_TOTAL; // sync to flash driver design
    pSoftDecDesc->bsCwId     = ucStartCwId;
    pSoftDecDesc->bsCwNum     = ucDescCwNum;

    /* clear descriptor CW cfg buffer */
    for (ucDescCwCnt = 0; ucDescCwCnt < DESC_SHARED_CW_NUM; ucDescCwCnt++)
    {
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsInfoLen= 0;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsHSel = 0;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsLbaEn = FALSE;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsRedunEn = FALSE;
        l_aLdpcSDescCfgCw[ucDescCwCnt].bsRedunLen = 0;
    }

    /* Configure descriptor CW cfg buffer */
    for (ucDescCwCnt = 0; ucDescCwCnt < ucDescCwNum; ucDescCwCnt++)
    {
        HAL_LdpcSDescCfgCw(pDescEnCtl, ucStartCwId, ucDescCwCnt);
    }

    /* Transfer buffer data to descriptor */
    pSoftDecDesc->bsInfoLen1    = l_aLdpcSDescCfgCw[0].bsInfoLen;
    pSoftDecDesc->bsInfoLen2    = l_aLdpcSDescCfgCw[1].bsInfoLen;
    pSoftDecDesc->bsInfoLen3    = l_aLdpcSDescCfgCw[2].bsInfoLen;
    pSoftDecDesc->bsInfoLen4    = l_aLdpcSDescCfgCw[3].bsInfoLen;
    pSoftDecDesc->bsHSel1       = l_aLdpcSDescCfgCw[0].bsHSel;
    pSoftDecDesc->bsHSel2       = l_aLdpcSDescCfgCw[1].bsHSel;
    pSoftDecDesc->bsHSel3       = l_aLdpcSDescCfgCw[2].bsHSel;
    pSoftDecDesc->bsHSel4       = l_aLdpcSDescCfgCw[3].bsHSel;
    pSoftDecDesc->bsLbaEn1      = l_aLdpcSDescCfgCw[0].bsLbaEn;
    pSoftDecDesc->bsLbaEn2      = l_aLdpcSDescCfgCw[1].bsLbaEn;
    pSoftDecDesc->bsLbaEn3      = l_aLdpcSDescCfgCw[2].bsLbaEn;
    pSoftDecDesc->bsLbaEn4      = l_aLdpcSDescCfgCw[3].bsLbaEn;
    pSoftDecDesc->bsRedunEn1    = l_aLdpcSDescCfgCw[0].bsRedunEn;
    pSoftDecDesc->bsRedunEn2    = l_aLdpcSDescCfgCw[1].bsRedunEn;
    pSoftDecDesc->bsRedunEn3    = l_aLdpcSDescCfgCw[2].bsRedunEn;
    pSoftDecDesc->bsRedunEn4    = l_aLdpcSDescCfgCw[3].bsRedunEn;
    pSoftDecDesc->bsRedunLen1   = l_aLdpcSDescCfgCw[0].bsRedunLen;
    pSoftDecDesc->bsRedunLen2   = l_aLdpcSDescCfgCw[1].bsRedunLen;
    pSoftDecDesc->bsRedunLen3   = l_aLdpcSDescCfgCw[2].bsRedunLen;
    pSoftDecDesc->bsRedunLen4   = l_aLdpcSDescCfgCw[3].bsRedunLen;

    return;
}

/*==============================================================================
Func Name  : HAL_LdpcSDescCfg
Input      : NONE
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.01.21 MapleXu create function
==============================================================================*/
void MCU2_DRAM_TEXT HAL_LdpcSDescCfg(U8 ucPU, U8 ucCurDescId, DESC_EN_CTL *pDescEnCtl)
{
    volatile SOFT_DEC_DESCRIPTOR * pSoftDecDesc = {0};
    pSoftDecDesc = HAL_LdpcSGetDescAddr(ucCurDescId);

    /*clear old descriptor messages*/
    COM_MemZero((U32 *)pSoftDecDesc, sizeof(SOFT_DEC_DESCRIPTOR)>>2);

    /*step1: Basic*/
    pSoftDecDesc->bsValid       = TRUE;
    pSoftDecDesc->bsIntStsEn    = pDescEnCtl->bsIntStsEn;
    pSoftDecDesc->bsRdNum       = pDescEnCtl->bsShiftReadTimes;

    /*step2: features enable*/
    pSoftDecDesc->bsEmEn        = pDescEnCtl->bsEmEn;
    pSoftDecDesc->bsDramCrcEn   = pDescEnCtl->bsDramCrcEn;
    pSoftDecDesc->bsCrcEn       = pDescEnCtl->bsCrcEn;
    pSoftDecDesc->bsDramOtfbSel = pDescEnCtl->bsDramOtfbSel;
    pSoftDecDesc->bsScrEn       = pDescEnCtl->bsScrEn;
    pSoftDecDesc->bsLbaChkEn    = pDescEnCtl->bsLbaChkEn;
    pSoftDecDesc->ulLba         = pDescEnCtl->ulLba;

    if (pSoftDecDesc->bsScrEn)
    {
        pSoftDecDesc->bsScrShfNum = pDescEnCtl->bsScrShfNum;
    }

    /*step3: 4-CW related cfg*/
    HAL_LdpcSDescCfgCws(ucPU, ucCurDescId, pDescEnCtl);

    return;
}

void MCU2_DRAM_TEXT HAL_LdpcSDescCfgLlr(U32 ulDescriptorId, U32 ulShiftReadTimes)
{
    ASSERT(ulDescriptorId < DESCRIPTOR_MAX_NUM);
    ASSERT((ulShiftReadTimes > 0) && (ulShiftReadTimes <= LDPC_SOFT_MAX_DEC_CNT));

    volatile SOFT_DEC_DESCRIPTOR * pSoftDecDesc = HAL_LdpcSGetDescAddr(ulDescriptorId);
    LDPC_LLR_ENTRY tLdpcLlrEntry = { 0 };

    tLdpcLlrEntry = HAL_LdpcSoftDecGetLLR(ulShiftReadTimes);

    pSoftDecDesc->bsAddrLLR0 = tLdpcLlrEntry.bsAddrLLR0;
    pSoftDecDesc->bsAddrLLR1 = tLdpcLlrEntry.bsAddrLLR1;
    pSoftDecDesc->bsAddrLLR2 = tLdpcLlrEntry.bsAddrLLR2;
    pSoftDecDesc->bsAddrLLR3 = tLdpcLlrEntry.bsAddrLLR3;
    pSoftDecDesc->bsAddrLLR4 = tLdpcLlrEntry.bsAddrLLR4;
    pSoftDecDesc->bsAddrLLR5 = tLdpcLlrEntry.bsAddrLLR5;

    pSoftDecDesc->bsValueLLR0 = tLdpcLlrEntry.bsValueLLR0;
    pSoftDecDesc->bsValueLLR1 = tLdpcLlrEntry.bsValueLLR1;
    pSoftDecDesc->bsValueLLR2 = tLdpcLlrEntry.bsValueLLR2;
    pSoftDecDesc->bsValueLLR3 = tLdpcLlrEntry.bsValueLLR3;
    pSoftDecDesc->bsValueLLR4 = tLdpcLlrEntry.bsValueLLR4;
    pSoftDecDesc->bsValueLLR5 = tLdpcLlrEntry.bsValueLLR5;

    return;
}


BOOL MCU2_DRAM_TEXT HAL_LdpcSIsDone(U32 ulDescriptorId)
{
    ASSERT(ulDescriptorId < DESCRIPTOR_MAX_NUM);

    volatile SOFT_DEC_DESCRIPTOR *pDescriptor = HAL_LdpcSGetDescAddr(ulDescriptorId);
    U32 ulDoneBitMap = 0x0F >> (DESC_SHARED_CW_NUM - pDescriptor->bsCwNum);

    if (pDescriptor->bsBitMap == ulDoneBitMap)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCU2_DRAM_TEXT HAL_LdpcSAllDone(U32 ulStartDescriptorId, U32 ulDescriptorCnt)
{
    ASSERT(ulStartDescriptorId < DESCRIPTOR_MAX_NUM);
    ASSERT((ulDescriptorCnt > 0) && (ulDescriptorCnt <= VALID_DESCRIPTOR_CNT));

    U32 i = 0;
    U32 ulDescriptorId = ulStartDescriptorId;

    for (i = 0; i < ulDescriptorCnt; ++i)
    {
        if (FALSE == HAL_LdpcSIsDone(ulDescriptorId))
        {
            return FALSE;
        }
        else
        {
            ulDescriptorId = (ulDescriptorId + 1) % DESCRIPTOR_MAX_NUM;
        }
    }

    return TRUE;
}

BOOL MCU2_DRAM_TEXT HAL_LdpcSAllSuccess(U32 ulStartDescriptorId, U32 ulDescriptorCnt)
{
    ASSERT(ulStartDescriptorId < DESCRIPTOR_MAX_NUM);
    ASSERT((ulDescriptorCnt > 0) && (ulDescriptorCnt <= VALID_DESCRIPTOR_CNT));

    U32 i = 0;
    U32 ulDescriptorId = ulStartDescriptorId;
    volatile SOFT_DEC_DESCRIPTOR *pDescriptor = NULL;

    for (i = 0; i < ulDescriptorCnt; ++i)
    {
        pDescriptor = HAL_LdpcSGetDescAddr(ulDescriptorId);

        if (TRUE == pDescriptor->bsFailed)
        {
            return FALSE;
        }
        else
        {
            #ifdef DATA_LBA_CHECK
            if (TRUE == pDescriptor->bsLbaChkEn && TRUE == pDescriptor->bsLbaErrStatus)
            {
                DBG_Printf("LDPC Soft Decoder LBA check fail, LBA = 0x%x!\n",pDescriptor->ulLba);
                DBG_Getch();
            }
            #endif
            ulDescriptorId = (ulDescriptorId + 1) % DESCRIPTOR_MAX_NUM;
        }
    }

    return TRUE;
}

BOOL MCU2_DRAM_TEXT HAL_LdpcSCrcStsCheck(U32 ulStartDescriptorId, U32 ulDescriptorCnt)
{
    ASSERT(ulStartDescriptorId < DESCRIPTOR_MAX_NUM);
    ASSERT((ulDescriptorCnt > 0) && (ulDescriptorCnt <= VALID_DESCRIPTOR_CNT));

    U32 i = 0;
    U32 ulDescriptorId = ulStartDescriptorId;
    U32 ulCrcSts = 0, ulRcrcSts = 0;

    volatile SOFT_DEC_DESCRIPTOR *pDescriptor = NULL;

    for (i = 0; i < ulDescriptorCnt; ++i)
    {
        pDescriptor = HAL_LdpcSGetDescAddr(ulDescriptorId);

        // Check Crc and Rcrc status by Cw, and default is 0.
        // CRC  : 1 - Success, 0 - Fail
        // RCRC : 0 - Success, 1 - Fail
        ulCrcSts = pDescriptor->bsCrcStatus1 + pDescriptor->bsCrcStatus2 + pDescriptor->bsCrcStatus3 + pDescriptor->bsCrcStatus4;
        ulRcrcSts = pDescriptor->bsRcrcStatus1 + pDescriptor->bsRcrcStatus2 + pDescriptor->bsRcrcStatus3 + pDescriptor->bsRcrcStatus4;

        if (ulDescriptorCnt != ulCrcSts || 0 != ulRcrcSts)
        {
            DBG_Printf("CRC or RCRC Error! CRC cnt:%d, RCRC cnt:%d\n", ulCrcSts, ulRcrcSts);
            return FALSE;
        }
        else
        {
            ulDescriptorId = (ulDescriptorId + 1) % DESCRIPTOR_MAX_NUM;
        }
    }

    return TRUE;
}

/*=============================== End of this file ===================================*/

