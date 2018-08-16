/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
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
* File Name    : L3_Interface.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "Proj_Config.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_DecStsReport.h"
#include "L2_Interface.h"
#include "L2_TableBBT.h"
#include "L3_Schedule.h"
#include "L3_Debug.h"
#include "L3_FlashMonitor.h"
#include "L3_Interface.h"
#include "L3_Init.h"

#ifdef XOR_ENABLE
#include "L3_DataRecover.h"
#include "HAL_XOR.h"
#include "FW_Debug.h"
#endif

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_IFIsFCmdIntrQEmpty
Input      : ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsFCmdIntrQEmpty(U8 ucTLun)
{
    if ((TRUE == HAL_NfcGetEmpty(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun)))
     && (TRUE != L3_SchGetStsBit(ucTLun, STS_BMP_ERRH))
     && (TRUE != L3_SchGetStsBit(ucTLun, STS_BMP_PEND))
     && (TRUE != L3_SchGetStsBit(ucTLun, STS_BMP_RECYCLE))
     && (TRUE != L3_SchGetStsBit(ucTLun, STS_BMP_EXTH)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*==============================================================================
Func Name  : L3_IFIsAllFCmdIntrQEmpty
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_IFIsAllFCmdIntrQEmpty(void)
{
    U8 ucTLun;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        if (FALSE == L3_IFIsFCmdIntrQEmpty(ucTLun))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*==============================================================================
Func Name  : L3_IFIsIdleDone
Input      : void
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsIdleDone(void)
{
    return L3_IFIsAllFCmdIntrQEmpty();
}

/*==============================================================================
Func Name  : L3_IFHandleChkIdle
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFHandleChkIdle(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U32 ulIdleType = ptReqEntry->bsReqSubType;
    U8 ucTLun = ptReqEntry->bsTLun;
    U8 ucReqPtr = ptReqEntry->bsReqPtr;
    FCMD_REQ_PRI eFCmdPri = ptReqEntry->bsFCmdPri;

    if (TRUE == L3_IFIsIdleDone())
    {
        if (FCMD_REQ_SUBTYPE_IDLE_2 == ulIdleType)
        {
            //TRACE_LOG((void*)ptReqEntry->ulReqStsAddr, sizeof(BOOL), BOOL, 0, "[L3 ForceIdle Error Recovery Done]");
            if (TRUE != *(BOOL*)ptReqEntry->ulReqStsAddr)
            {
                /* Resets SGE for NVMe solution. */
                L3_SGEReset();

                /* Re-initializes DSG pool then. */
                #ifdef VT3533_A2ECO_DSGRLSBUG
                HAL_NormalDsgFifoInit();
                #endif
                DBG_Printf("L3 Handle the ForceIdle Error Recovery Done.\n");
            }
            else
            {
                DBG_Printf("L3 Ignore the ForceIdle Request.\n");
            }
        }

        *(BOOL*)ptReqEntry->ulReqStsAddr = TRUE;
    }

    ASSERT(FCMD_REQ_STS_POP == L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucReqPtr));
    L2_FCMDQSetReqSts(ucTLun, eFCmdPri, ucReqPtr, FCMD_REQ_STS_INIT);

    return;
}

/*==============================================================================
Func Name  : L3_IFIsSelfTestDone
Input      : void
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsSelfTestDone(void)
{
    return TRUE;
}

/*==============================================================================
Func Name  : L3_IFHandleSelfTest
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFHandleSelfTest(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucTLun = ptReqEntry->bsTLun;
    U8 ucReqPtr = ptReqEntry->bsReqPtr;
    FCMD_REQ_PRI eFCmdPri = ptReqEntry->bsFCmdPri;

    if (TRUE == L3_IFIsSelfTestDone())
    {
        *(BOOL*)ptReqEntry->ulReqStsAddr = TRUE;
    }

    ASSERT(FCMD_REQ_STS_POP == L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucReqPtr));
    L2_FCMDQSetReqSts(ucTLun, eFCmdPri, ucReqPtr, FCMD_REQ_STS_INIT);

    return;
}

/*==============================================================================
Func Name  : L3_IFIsSpecialFCmd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_IFIsSpecialFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucReqType = ptCtrlEntry->ptReqEntry->bsReqType;

    if (FCMD_REQ_TYPE_CHK_IDLE == ucReqType || FCMD_REQ_TYPE_SELF_TEST == ucReqType)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*==============================================================================
Func Name  : L3_IFSendSpecialFCmd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFSendSpecialFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucReqType = ptCtrlEntry->ptReqEntry->bsReqType;

    #ifdef SIM
    L3_DbgFCmdCntAdd(ptCtrlEntry);
    #endif

    switch (ucReqType)
    {
        case FCMD_REQ_TYPE_CHK_IDLE:
        {
            L3_IFHandleChkIdle(ptCtrlEntry);
            break;
        }
        case FCMD_REQ_TYPE_SELF_TEST:
        {
            L3_IFHandleSelfTest(ptCtrlEntry);
            break;
        }
        default:
        {
            DBG_Printf("L3_Special:0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFSearchSeqBit1
Input      : U8 LpnBitMap
             U8 *ptStartBitInLpn
             U8 *ptBit1NumInLpn
             U8 *ptBitPos
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.19 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFSearchSeqBit1(U8 LpnBitMap, U8 *ptStartBitInLpn, U8 *ptBit1NumInLpn, U8 *ptBitPos)
{
    U8 ucCurBitPos = *ptBitPos;
    U8 ucBit1NumInLpn;

    while (!(LpnBitMap & (1<<ucCurBitPos)) && (SEC_PER_LPN >ucCurBitPos))
    {
        ucCurBitPos++;
    }

    if(SEC_PER_LPN <= ucCurBitPos)
    {
        ucCurBitPos = 0;

        *ptBitPos = ucCurBitPos;
        return FALSE;
    }

    *ptStartBitInLpn = ucCurBitPos++;

    ucBit1NumInLpn = 1;
    while ((LpnBitMap&(1<<ucCurBitPos)) && (SEC_PER_LPN>ucCurBitPos))
    {
        ucBit1NumInLpn++;
        ucCurBitPos++;
    }

    *ptBit1NumInLpn = ucBit1NumInLpn;

    *ptBitPos = ucCurBitPos;

    return TRUE;
}

/*==============================================================================
Func Name  : L3_IFAllocDSG
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_IFAllocDSG(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8  ucBitPos, ucBuffPos, ucStartBitInLpn, ucBitLenInLpn, ucTmpSecLen;
    U16 usCurDsgId, usNextDsgId;
    U16 usBufSizeBits;
    BOOL bFind;
    NORMAL_DSG_ENTRY *ptDsgEntry;
    FCMD_DATA_TX_CTRL *ptDsgCtrl;
    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = ptCtrlEntry->ptReqEntry;
    ptDsgCtrl = &ptCtrlEntry->tDTxCtrl;

    if (((FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType)
      && (FALSE == ptReqEntry->tFlashDesc.bsRdRedOnly))
     || (FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType))
    {
        if (TRUE == ptDsgCtrl->bsBdEngineDone)
        {
            return TRUE;
        }

        ptDsgCtrl->bsDSGEn = TRUE;
    }
    else
    {
        if (TRUE == ptReqEntry->tFlashDesc.bsRdRedOnly)
        {
            if (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType)
            {
                ptDsgCtrl->bsDmaTotalLen = PLN_PER_LUN-1;
            }
            else
            {
                ptDsgCtrl->bsDmaTotalLen = 0;
            }
        }

        ptDsgCtrl->bsDSGEn = FALSE;

        return TRUE;
    }

    if (INVALID_4F == ptDsgCtrl->bsFstEngineID)
    {
        if (FALSE == HAL_GetNormalDsg(&usCurDsgId))
        {
            return FALSE;
        }
        ptDsgCtrl->bsFstEngineID = usCurDsgId;
        ptDsgCtrl->bsCurEngineID = usCurDsgId;
    }

    if (TRUE == ptReqEntry->tFlashDesc.bsMergeRdEn)
    {
        while (TRUE != ptDsgCtrl->bsBdEngineDone)
        {
            ucBitPos = ptDsgCtrl->bsLpnBitMapPos;
            bFind = L3_IFSearchSeqBit1(ptReqEntry->tFlashDesc.bsLpnBitmap, &ucStartBitInLpn, &ucBitLenInLpn, &ucBitPos);

            if ((TRUE == bFind) && (ptReqEntry->tFlashDesc.bsLpnBitmap > MULTI_VALUE_1(ucBitPos)))
            {
                if (FALSE == HAL_GetNormalDsg(&usNextDsgId))
                {
                    break;
                }
            }
            else
            {
                usNextDsgId = INVALID_4F;
            }

            ptDsgCtrl->aSecAddr[ptDsgCtrl->bsSecAddrIndex].bsSecStart  = ptReqEntry->tFlashDesc.bsSecStart + ucStartBitInLpn;
            ptDsgCtrl->aSecAddr[ptDsgCtrl->bsSecAddrIndex].bsSecLength = ucBitLenInLpn;
            ptDsgCtrl->bsSecAddrIndex++;
            ptDsgCtrl->bsDmaTotalLen += ucBitLenInLpn;
            ptDsgCtrl->bsLpnBitMapPos = ucBitPos;

            ptDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(ptDsgCtrl->bsCurEngineID);
            COM_MemZero((U32*)ptDsgEntry,sizeof(NORMAL_DSG_ENTRY) >> DWORD_SIZE_BITS);

            usBufSizeBits = BUF_SIZE_BITS;

            ptDsgEntry->bsDramAddr    = HAL_NfcGetDmaAddr(ptReqEntry->atBufDesc[0].bsBufID, ptReqEntry->atBufDesc[0].bsSecStart + ucStartBitInLpn, usBufSizeBits);
            ptDsgEntry->bsXferByteLen = ucBitLenInLpn << SEC_SZ_BITS;
            ptDsgEntry->bsNextDsgId   = usNextDsgId;

            if (INVALID_4F != usNextDsgId)
            {
                HAL_SetNormalDsgSts(ptDsgCtrl->bsCurEngineID, NORMAL_DSG_VALID);
                ptDsgCtrl->bsCurEngineID = usNextDsgId;
            }
            else
            {
                ptDsgEntry->bsLast  = TRUE;
                HAL_SetNormalDsgSts(ptDsgCtrl->bsCurEngineID, NORMAL_DSG_VALID);
                ptDsgCtrl->bsBdEngineDone = TRUE;
            }
        }
    }
#ifdef XOR_ENABLE
    else if (TRUE == L3_IsDataDisperse(ptReqEntry))
    {
        if (FALSE == HAL_GetNormalDsg(&usNextDsgId))
        {
            return FALSE;
        }

        ucBuffPos = (3 - ptCtrlEntry->bsMultiStep) % 3;
        ucTmpSecLen = ptReqEntry->atBufDesc[ucBuffPos].bsSecLen;
        ptDsgCtrl->bsDmaTotalLen = ucTmpSecLen;

        ptDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(ptDsgCtrl->bsCurEngineID);
        COM_MemZero((U32*)ptDsgEntry,sizeof(NORMAL_DSG_ENTRY) >> DWORD_SIZE_BITS);

        ASSERT(FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType);
        ptDsgEntry->bsDramAddr = HAL_NfcGetDmaAddr(ptReqEntry->atBufDesc[ucBuffPos].bsBufID,
                                                   ptReqEntry->atBufDesc[ucBuffPos].bsSecStart,
                                                   BUF_SIZE_BITS);
        ptDsgEntry->bsXferByteLen = (ucTmpSecLen << SEC_SZ_BITS) - PHYPG_SZ;
        ptDsgEntry->bsNextDsgId   = usNextDsgId;

        HAL_SetNormalDsgSts(ptDsgCtrl->bsCurEngineID, NORMAL_DSG_VALID);

        NORMAL_DSG_ENTRY *pt2ndDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usNextDsgId);
        COM_MemZero((U32*)pt2ndDsgEntry, sizeof(NORMAL_DSG_ENTRY) >> DWORD_SIZE_BITS);

        pt2ndDsgEntry->bsOntf = TRUE;
        pt2ndDsgEntry->bsDramAddr = (HAL_XoreGetParityPageAddr(ptCtrlEntry->bsXoreId) +
                                     (ucBuffPos * PHYPG_SZ) - OTFB_START_ADDRESS) >> 1;
        pt2ndDsgEntry->bsXferByteLen = PHYPG_SZ;
        pt2ndDsgEntry->bsLast = TRUE;
        //pt2ndDsgEntry->bsNextDsgId = INVALID_4F;

        HAL_SetNormalDsgSts(usNextDsgId, NORMAL_DSG_VALID);

        ptDsgCtrl->bsBdEngineDone = TRUE;
    }
#endif
    else
    {
        while (TRUE != ptDsgCtrl->bsBdEngineDone)
        {
            ucBuffPos = ptDsgCtrl->bsSecAddrIndex;
            if (DSG_BUFF_SIZE > (ucBuffPos + 1) && (INVALID_4F != ptReqEntry->atBufDesc[ucBuffPos + 1].bsBufID))
            {
                if (FALSE == HAL_GetNormalDsg(&usNextDsgId))
                {
                    break;
                }
            }
            else
            {
                usNextDsgId = INVALID_4F;
            }

            ucTmpSecLen = ptReqEntry->atBufDesc[ptDsgCtrl->bsSecAddrIndex].bsSecLen;
            ptDsgCtrl->aSecAddr[ptDsgCtrl->bsSecAddrIndex].bsSecStart  = ptReqEntry->tFlashDesc.bsSecStart + ptDsgCtrl->bsDmaTotalLen;
            ptDsgCtrl->aSecAddr[ptDsgCtrl->bsSecAddrIndex].bsSecLength = ucTmpSecLen;
            ptDsgCtrl->bsDmaTotalLen += ucTmpSecLen;
            ptDsgCtrl->bsSecAddrIndex++;

            ptDsgEntry = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(ptDsgCtrl->bsCurEngineID);
            COM_MemZero((U32*)ptDsgEntry,sizeof(NORMAL_DSG_ENTRY) >> DWORD_SIZE_BITS);

            //usBufSizeBits = (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType) ? LOGIC_PG_SZ_BITS : BUF_SIZE_BITS;
            if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType && TRUE == ptReqEntry->bsTableReq)
            {
                usBufSizeBits = LOGIC_PG_SZ_BITS;
            }
            else
            {
                usBufSizeBits = BUF_SIZE_BITS;
            }
            ptDsgEntry->bsDramAddr = HAL_NfcGetDmaAddr(ptReqEntry->atBufDesc[ucBuffPos].bsBufID, ptReqEntry->atBufDesc[ucBuffPos].bsSecStart, usBufSizeBits);
            ptDsgEntry->bsXferByteLen = ptReqEntry->atBufDesc[ucBuffPos].bsSecLen << SEC_SZ_BITS;
            ptDsgEntry->bsNextDsgId   = usNextDsgId;

            if (INVALID_4F != usNextDsgId)
            {
                HAL_SetNormalDsgSts(ptDsgCtrl->bsCurEngineID, NORMAL_DSG_VALID);
                ptDsgCtrl->bsCurEngineID = usNextDsgId;
            }
            else
            {
                #if defined(HOST_SATA)
                if(TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
                {
                    ptDsgEntry->bsBuffMapId = ptReqEntry->tHostDesc.tHostDptr.bsPrdID;// Host Read only one buffer now.
                    ptDsgEntry->bsMapSecOff = ptReqEntry->atBufDesc[ucBuffPos].bsSecStart;
                }
                #endif

                ptDsgEntry->bsLast  = TRUE;
                HAL_SetNormalDsgSts(ptDsgCtrl->bsCurEngineID, NORMAL_DSG_VALID);
                ptDsgCtrl->bsBdEngineDone = TRUE;
            }
        }
    }

    return ptDsgCtrl->bsBdEngineDone;
}

/*==============================================================================
Func Name  : L3_IFAllocResource
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFAllocResource(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bResult;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    #if 0
    if (FCMD_REQ_SUBTYPE_SETFEATURE == ptReqEntry->bsReqSubType)
    {
        ASSERT(0x91 == ptReqEntry->ulReqStsAddr);
        ASSERT(0x100 == ptReqEntry->ulSpareAddr || 0x104 == ptReqEntry->ulSpareAddr);//dannier add
        return TRUE;
    }

    if (FCMD_REQ_SUBTYPE_INTRNAL == ptReqEntry->bsReqSubType)
    {
        ASSERT(FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType);
        ASSERT(FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod);
        return TRUE;
    }
    #endif

    if (FCMD_REQ_TYPE_RDSTS == ptReqEntry->bsReqType)
    {
        return TRUE;
    }
#ifdef XOR_ENABLE
    if (TRUE == L3_IsTlcXor6DsgIssue(ptReqEntry))
    {
        if (ptCtrlEntry->bsMultiStep == 0)
        {
            ptCtrlEntry->bsMultiStep = 3;
        }
    }

    if (TRUE == L3_IsNeedDoXor(ptReqEntry))
    {
        if (FALSE == L3_AllocXore(ptCtrlEntry))
        {
            U8 ucTLun = ptReqEntry->bsTLun;
            if (TRUE != L3_SchGetStsBit(ucTLun, STS_BMP_ERRH))
            {
                L3_FCMDQSetIntrPptr(ucTLun, ptCtrlEntry->bsCtrlPtr, FALSE);
            }
            PRINT_DEBUG("L3_IFAllocResource: XOR Engine Allocation Failed, Command Be Pended.\n");
            return FALSE;
        }
    }
#endif

    bResult = L3_AllocResource(ptCtrlEntry);
    if (TRUE != bResult)
    {
        U8 ucTLun = ptReqEntry->bsTLun;
        if (TRUE != L3_SchGetStsBit(ucTLun, STS_BMP_ERRH))
        {
            L3_FCMDQSetIntrPptr(ucTLun, ptCtrlEntry->bsCtrlPtr, FALSE);
        }
    }

    return bResult;
}

/*==============================================================================
Func Name  : L3_IFGetPhyBlk
Input      : FCMD_REQ_ENTRY *ptReqEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.19 JasonGuo create function
==============================================================================*/
LOCAL U32 L3_IFGetPhyBlk(FCMD_REQ_ENTRY *ptReqEntry)
{
    U8 ucSPU = L2_GET_SPU(ptReqEntry->bsTLun);
    U8 ucLunInSPU = L2_GET_LUN_IN_SPU(ptReqEntry->bsTLun);

    return L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, ptReqEntry->tFlashDesc.bsVirBlk);
}

/*==============================================================================
Func Name  : L3_IFAdaptBlkPage
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
    2. 2016.9.26 JasonGuo l3 supports xx-nand-flash-adapter-part #1/4
==============================================================================*/
LOCAL void L3_IFAdaptBlkPage(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
#ifdef ERRH_MODIFY_PENDING_CMD
    ptCtrlEntry->bsPhyPage = ptReqEntry->tFlashDesc.bsVirPage;
    return;
#else

    BOOL bSLCBlock = (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;

#if 0
    if (FCMD_REQ_SUBTYPE_SETFEATURE == ptReqEntry->bsReqSubType)
    {
        ASSERT(0x91 == ptReqEntry->ulReqStsAddr);
        ASSERT(0x100 == ptReqEntry->ulSpareAddr || 0x104 == ptReqEntry->ulSpareAddr);//dannier add
        return;
    }
#endif

    if (TRUE == bSLCBlock)
    {
        // Slc Table block fail -> L2 handle.

        // Slc Data block fail -> L3 handle;
        if (TRUE != ptReqEntry->bsTableReq)
        {
            if (TRUE == L2_BbtIsGBbtBadBlock(ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsPhyBlk))
            {
                ptReqEntry->tFlashDesc.bsPhyBlk = L3_IFGetPhyBlk(ptReqEntry);
            }
        }

        // Slc PhyPage -> L3 handle;
        ptCtrlEntry->bsPhyPage = HAL_FlashGetSLCPage(ptReqEntry->tFlashDesc.bsVirPage);
    }
    else
    {
        // Tlc Data block fail -> L2 handle in whole chip, L3 handle in L3 Unit Test;
#ifdef L3_UNIT_TEST
        if (TRUE != ptReqEntry->bsTableReq && TRUE != ptReqEntry->bsRDTEn)
        {
            if (TRUE == L2_BbtIsGBbtBadBlock(ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsPhyBlk))
            {
                ptReqEntry->tFlashDesc.bsPhyBlk = L3_IFGetPhyBlk(ptReqEntry);
            }
        }
#endif

        // Tlc PhyPage -> L3 handle;
        ptCtrlEntry->bsPhyPage = ptReqEntry->tFlashDesc.bsVirPage;

    }

    return;
#endif
}

/*==============================================================================
Func Name  : L3_IFIsReadCmdHit
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.4 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsReadCmdHit(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, U8 ucCmdType)
{
    BOOL bReadHit = FALSE;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucTLun = ptReqEntry->bsTLun;

    /* For Intel/Micron flash, the previous page# is equal to the current page#. And the same physical blk number */
    if (ptCtrlEntry->bsPhyPage == L3_FMGetPhyPage(ucTLun) && ptReqEntry->tFlashDesc.bsPhyBlk == L3_FMGetPhyBlk(ucTLun))
    {
        /* If the current cmd is equal to the previous cmd, we can use change column read. */
        if (ucCmdType == L3_FMGetCmdType(ucTLun))
        {
            /* If current cmd is single-plane level, need to add plane number check. */
            if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType)
            {
                if (ptReqEntry->tFlashDesc.bsPlnNum == L3_FMGetPlnNum(ucTLun))
                {
                    bReadHit = TRUE;
                }
            }
            else
            {
                bReadHit = TRUE;
            }
        }
        else
        {
            /*
                If the current cmd isn't equal to the previous cmd, we still have two possible conditions can use change column read.
                1. The previous cmd is multi-plane level, but the current cmd is single-plane level.
                2. The previous cmd is change column read.

                If last cmd is snap, we shouldn't use CCR.
                However, current cmd type will never be "snap read" under current design. This check is removable.
            */
            if (NF_PRCQ_CCLR_MULTIPLN == L3_FMGetCmdType(ucTLun) || NF_PRCQ_READ_MULTIPLN == L3_FMGetCmdType(ucTLun) || NF_PRCQ_TLC_READ_MULTIPLN == L3_FMGetCmdType(ucTLun))
            {
                bReadHit = TRUE;
            }
            else if (NF_PRCQ_CCLR == L3_FMGetCmdType(ucTLun))
            {
                if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType)
                {
                    if (ptReqEntry->tFlashDesc.bsPlnNum == L3_FMGetPlnNum(ucTLun))
                    {
                        bReadHit = TRUE;
                    }
                }
            }
        }
    }

    return bReadHit;
}

/*==============================================================================
Func Name  : L3_IFIsSnapRead
Input      : FCMD_FLASH_DESC *pFlashDesc
Output     :
Return Val : LOCAL
Discription: Check if we can use snap read. Only support first two 4K. So last sector should be <= 16.(HW limitation.)
Usage      :
History    :
    1. 2017.6.28 HenryChen create function
==============================================================================*/
LOCAL BOOL L3_IFIsSnapRead(FCMD_FLASH_DESC tFlashDesc)
{
    //DBG_Printf("Snap! Sec Start=%d Len=%d Merge%d\n", tFlashDesc.bsSecStart, tFlashDesc.bsSecLen, tFlashDesc.bsMergeRdEn);

    if (tFlashDesc.bsMergeRdEn)
        return ((tFlashDesc.bsSecStart <= 8 )? TRUE : FALSE);
    else
        return ((tFlashDesc.bsSecLen != 0 && tFlashDesc.bsSecStart + tFlashDesc.bsSecLen <= 16)? TRUE : FALSE);
}

/*==============================================================================
Func Name  : L3_IFAdaptCmdType
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
TLCMode SLCMode bTable bIntCopy Request
0       0       0      0        Mlc - data multi-pln erase/write/read
0       0       0      1        Mlc - Invalid
0       0       1      x        Mlc - Invalid
0       1       0      0        Slc - data multi-pln erase/write/read
0       1       0      1        Slc - data internal copy to Slc - not support
0       1       1      0        Slc - table erase, write, read
0       1       1      1        Slc - table internal copy to Slc - not support
1       0       0      0        Tlc - data multi/single-pln erase, multi pln-write/read
1       0       0      1        Tlc - data multi-pln slc internal copy to Tlc
1       0       1      x        Tlc - Invalid
1       1       x      x        Invalid
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
    2. 2016.9.26 JasonGuo l3 supports xx-nand-flash-adapter-part #2/4
==============================================================================*/
LOCAL void L3_IFAdaptCmdType(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucReqType = ptReqEntry->bsReqType;
    U8 ucReqSubType = ptReqEntry->bsReqSubType;
    U8 ucIntrPage = ptCtrlEntry->bsIntrPage;
    BOOL bSlcBlk = (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
    U8 ucCmdType, ucScrMod = INVALID_2F;

    ASSERT(FCMD_REQ_MLC_BLK != ptReqEntry->tFlashDesc.bsBlkMod);

    if (FCMD_REQ_TYPE_READ == ucReqType)
    {
        if (FCMD_REQ_SUBTYPE_NORMAL == ucReqSubType)
        {
            ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_READ_MULTIPLN : NF_PRCQ_TLC_READ_MULTIPLN;

            if (TRUE == L3_IFIsReadCmdHit(ptCtrlEntry, ucCmdType))
            {
                ucCmdType = NF_PRCQ_CCLR_MULTIPLN;
            }
        }
        else
        {
            ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_READ : NF_PRCQ_TLC_READ;

            if (TRUE == L3_IFIsReadCmdHit(ptCtrlEntry, ucCmdType))
            {
                ucCmdType = NF_PRCQ_CCLR;
            }
#ifdef SNAP_READ_EN
            //Snap read (Snap read didn't support read retry. Disable it while UECC handling.)
            else if (NULL == ptCtrlEntry->ptErrHEntry && L3_IFIsSnapRead(ptReqEntry->tFlashDesc))
            {
                ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_SLC_SNAP_READ : NF_PRCQ_TLC_SNAP_READ;
            }
#endif
        }
        ucScrMod = NORMAL_SCR;
    }
    else if (FCMD_REQ_TYPE_WRITE == ucReqType)
    {
        if (FCMD_REQ_SUBTYPE_NORMAL == ucReqSubType)
        {

            if (TRUE == bSlcBlk)
            {
                ucCmdType = NF_PRCQ_PRG_MULTIPLN;
                ucScrMod = NORMAL_SCR;
            }
            else
            {
                ucCmdType = NF_PRCQ_TLC_PRG_MULTIPLN;
                ucScrMod = TLC_RW_TWO_PG;
            }
        }
        else if (FCMD_REQ_SUBTYPE_ONEPG == ucReqSubType)
        {
            ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_PRG : NF_PRCQ_MLC_PRG;
            ucScrMod = NORMAL_SCR;
        }
        else if (FCMD_REQ_SUBTYPE_SINGLE == ucReqSubType)
        {
            ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_PRG : NF_PRCQ_MLC_PRG;
            ucScrMod = (TRUE == bSlcBlk) ? NORMAL_SCR : MLC_RW_TWO_PG;
        }
        else if (FCMD_REQ_SUBTYPE_SINGLE_TWOPG == ucReqSubType)
        {
            ucCmdType = NF_PRCQ_TLC_PRG;
            ucScrMod = TLC_RW_TWO_PG;
        }
        else if (FCMD_REQ_SUBTYPE_SINGLE_ONEPG == ucReqSubType)
        {
            ucCmdType = NF_PRCQ_TLC_PRG_LOW_PG;
            ucScrMod = NORMAL_SCR;
        }
        else
        {
            DBG_Getch();
        }
    }
    else if (FCMD_REQ_TYPE_ERASE == ucReqType)
    {
        if (FCMD_REQ_SUBTYPE_SINGLE == ucReqSubType)
        {
            ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_ERS : NF_PRCQ_TLC_ERS;
        }
        else
        {
            ucCmdType = (TRUE == bSlcBlk) ? NF_PRCQ_ERS_MULTIPLN : NF_PRCQ_TLC_ERS_MULTIPLN;
        }
    }
    else if (FCMD_REQ_TYPE_SETFEATURE == ucReqType)
    {
        ucCmdType = NF_PRCQ_SETFEATURE;
    }
    else if (FCMD_REQ_TYPE_RDSTS == ucReqType)
    {
        if (FCMD_REQ_SUBTYPE_SINGLE == ucReqSubType)
        {
            ucCmdType = NF_PRCQ_READ_STS_ENHANCE;
        }
        else
        {
            DBG_Printf("notmeet\n");
            DBG_Getch();
        }
    }
    else
    {
        DBG_Printf("L3_Adapter2 0x%x\n", (U32)ptCtrlEntry);
        DBG_Getch();
    }

    ptCtrlEntry->bsCmdType = ucCmdType;
    ptCtrlEntry->bsScrMod = ucScrMod;
    
#ifdef RAW_DATA_RD
    /* add for rawdata */
    ptCtrlEntry->bsRdRawData = (FCMD_REQ_TYPE_READ == ucReqType) ? TRUE : FALSE;
#endif
#ifdef BYPASS_UECC
    ptCtrlEntry->bsBypassEcc = (FCMD_REQ_TYPE_READ == ucReqType) ? TRUE : FALSE;   
#endif
    return;
}

/*==============================================================================
Func Name  : L3_IFAdaptFCmd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFAdaptFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    L3_IFAdaptBlkPage(ptCtrlEntry);

    L3_IFAdaptCmdType(ptCtrlEntry);

    return;
}

/*==============================================================================
Func Name  : L3_IFAllocNFCQ
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL NFCQ_ENTRY *L3_IFAllocNFCQ(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun;
    NFCQ_ENTRY *ptNfcqEntry;

    ucTLun = ptCtrlEntry->ptReqEntry->bsTLun;
    ptNfcqEntry = (NFCQ_ENTRY*)HAL_NfcGetNfcqEntryAddr(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun));
    COM_MemZero((U32*)ptNfcqEntry, sizeof(NFCQ_ENTRY) >> DWORD_SIZE_BITS);

    return ptNfcqEntry;
}

/*==============================================================================
Func Name  : L3_IFIsScrEn
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.20 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsScrEn(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bIsScrEn;

    #if 0
    if (TRUE == HAL_NfcBypassScrb())
    {
        bIsScrEn = FALSE;
    }
    else
    #endif
    if (TRUE == ptCtrlEntry->ptReqEntry->tLocalDesc.bsRawRdCmd && TRUE != ptCtrlEntry->ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        bIsScrEn = FALSE;
    }
    else
    {
        bIsScrEn = TRUE;
    }

    return bIsScrEn;
}

/*==============================================================================
Func Name  : L3_IFGetScrSeedPlnNum
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.20 JasonGuo create function
==============================================================================*/
LOCAL U8 L3_IFGetScrSeedPlnNum(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucPlnNum = 0;
    U8 ucReqSubType = ptCtrlEntry->ptReqEntry->bsReqSubType;;

    //if (TRUE == HAL_NfcIsChkSeedSel())
    //{
        //ucReqSubType = ptCtrlEntry->ptReqEntry->bsReqSubType;
        ucPlnNum = (FCMD_REQ_SUBTYPE_SINGLE == ucReqSubType || FCMD_REQ_SUBTYPE_SINGLE_ONEPG == ucReqSubType || FCMD_REQ_SUBTYPE_SINGLE_TWOPG == ucReqSubType) ? 0 : PLN_PER_LUN_BITS;
    //}

    return ucPlnNum;
}

/*==============================================================================
Func Name  : L3_IFSetNFCQScrmable
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
    2. 2016.9.26 JasonGuo l3 supports xx-nand-flash-adapter-part #3/4
==============================================================================*/
LOCAL void L3_IFSetNFCQScrmable(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bScrEn;
    U8 ucScrMod;
    U16 usSeed;

    bScrEn = L3_IFIsScrEn(ptCtrlEntry);
    if (TRUE != bScrEn)
    {
        ptNfcqEntry->bsPuEnpMsk = TRUE;
        return;
    }

    ucScrMod = ptCtrlEntry->bsScrMod;
    switch (ucScrMod)
    {
        case NORMAL_SCR:
        {
            ptNfcqEntry->bsScrSeed0 = HAL_FlashGetScrSeed(ptCtrlEntry->bsPhyPage, ucScrMod, 0);
            break;
        }
        case TLC_RW_TWO_PG:
        {
            ptNfcqEntry->bsScrSeed0 = HAL_FlashGetScrSeed(ptCtrlEntry->bsPhyPage, ucScrMod, 0);

            if (EXTRA_PAGE == ptCtrlEntry->ptReqEntry->tLocalDesc.bsPairPageType)
            {
                usSeed = HAL_FlashGetScrSeed(ptCtrlEntry->ptReqEntry->tLocalDesc.bsPairPageNum, ucScrMod, 0);
                ptNfcqEntry->bsScrSeed1Lsb = usSeed & 0x1;            //bit0
                ptNfcqEntry->bsScrSeed1Msb = (usSeed & 0xFE) >> 1;     //bit7-1
            }
            else
            {
                usSeed = HAL_FlashGetScrSeed((ptCtrlEntry->bsPhyPage + 1), ucScrMod, 0);
                ptNfcqEntry->bsScrSeed1Lsb = usSeed & 0x1;            //bit0
                ptNfcqEntry->bsScrSeed1Msb = (usSeed & 0xFE) >> 1;    //bit7-1
            }
            break;
        }
        //#ifdef XOR_ENABLE
        case TLC_WT_XOR_6DSG:
        {
            DBG_Getch();
            // For the 6 DSG issue of TLC XOR program.
            ptNfcqEntry->bsScrSeed0 = HAL_FlashGetScrSeed(ptCtrlEntry->bsPhyPage, ucScrMod, (3 - ptCtrlEntry->bsMultiStep));
            break;
        }
        //#endif
        default:
        {
            DBG_Printf("L3_ScrambleSet.0x%x.\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

   ptNfcqEntry->bsTlcPlnNum = L3_IFGetScrSeedPlnNum(ptCtrlEntry);

    return;
}

/*==============================================================================
Func Name  : L3_IFGetRedAddrOffSet
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.20 JasonGuo create function
==============================================================================*/
LOCAL U32 L3_IFGetRedAddrOffSet(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U32 ulAddrOffSet;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (TRUE != ptCtrlEntry->bsIntrReq)
    {
        ulAddrOffSet = RED_RELATIVE_ADDR(MCU1_ID, ptReqEntry->bsTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);
    }
    else
    {
        ulAddrOffSet = RED_RELATIVE_ADDR(MCU2_ID, ptReqEntry->bsTLun, 0, ptCtrlEntry->bsCtrlPtr);
    }

    return ulAddrOffSet;
}

/*==============================================================================
Func Name  : L3_IFSetNFCQDataTxfer
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFSetNFCQDataTxfer(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucSecAddrIdx;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_DATA_TX_CTRL *ptDTxCtrl = &ptCtrlEntry->tDTxCtrl;

    for (ucSecAddrIdx = 0; ucSecAddrIdx < ptDTxCtrl->bsSecAddrIndex; ucSecAddrIdx++)
    {
        ptNfcqEntry->aSecAddr[ucSecAddrIdx] = ptDTxCtrl->aSecAddr[ucSecAddrIdx];
    }

    if (FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType)
    {
        ptNfcqEntry->bsDmaTotalLength = ptDTxCtrl->bsDmaTotalLen >> 1; // Sec -> CW
    }
    else
    {
        ptNfcqEntry->bsDmaTotalLength = ptDTxCtrl->bsDmaTotalLen;
    }

    if (0 != ptReqEntry->ulSpareAddr)
    {
        ptNfcqEntry->bsRedEn   = TRUE;
        ptNfcqEntry->bsRedOntf = FALSE;
        ptNfcqEntry->bsRedOnly = ptReqEntry->tFlashDesc.bsRdRedOnly;
        ptNfcqEntry->bsRedAddr = L3_IFGetRedAddrOffSet(ptCtrlEntry);
        #ifdef XOR_ENABLE
        if (TRUE == L3_IsDataDisperse(ptReqEntry))
        {
            U32 ulXorParityPartNum = (3 - ptCtrlEntry->bsMultiStep) % 3;
            ptNfcqEntry->bsRedAddr = L3_IFGetRedAddrOffSet(ptCtrlEntry) +
                                     ((ulXorParityPartNum * RED_SW_SZ_DW * sizeof(U32)) >> 3);
            ptNfcqEntry->bsRedAddXorPar0 = (HAL_XoreGetParityRedunOffset(ptCtrlEntry->bsXoreId) +
                                            ulXorParityPartNum * RED_SZ) >> XOR_REDUN_ALIGN_BITS;
        }
        #endif
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFIsEnableSsu0
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      : ssu0 is used to auto-update the request status by nfc.
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsEnableSsu0(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bEnable = TRUE;

    if (TRUE == L3_IFIsRecycled(ptCtrlEntry) || TRUE == L3_IFIsForceRecycle(ptCtrlEntry))
    {
        bEnable = FALSE;
    }

    return bEnable;
}

/*==============================================================================
Func Name  : L3_IFIsEnableSsu1
Input      : FCMD_REQ_ENTRY *ptReqEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_IFIsEnableSsu1(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bEnable = TRUE;

    if (TRUE != L3_IFIsAutoRptReq(ptCtrlEntry) || TRUE == L3_IFIsForceRecycle(ptCtrlEntry))
    {
        bEnable = FALSE;
    }

    return bEnable;
}

/*==============================================================================
Func Name  : L3_IFGetSsu0AddrOffSet
Input      : FCMD_REQ_ENTRY *ptReqEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL U16 L3_IFGetSsu0AddrOffSet(FCMD_REQ_ENTRY *ptReqEntry)
{
    U32 ulAddr;

    ulAddr = L2_FCMDQGetReqStsAddr(ptReqEntry->bsTLun, ptReqEntry->bsFCmdPri, ptReqEntry->bsReqPtr);

    return ulAddr - g_ulSsuInOtfbBaseAddr;
}

/*==============================================================================
Func Name  : L3_IFGetSsu1AddrOffSet
Input      : FCMD_REQ_ENTRY *ptReqEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL U16 L3_IFGetSsu1AddrOffSet(FCMD_REQ_ENTRY *ptReqEntry)
{
    return ptReqEntry->ulReqStsAddr - g_ulSsuInOtfbBaseAddr;
}

/*==============================================================================
Func Name  : L3_IFSetNFCQRowAddr
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
    2. 2016.9.26 JasonGuo l3 supports xx-nand-flash-adapter-part #4/4
==============================================================================*/
LOCAL void L3_IFSetNFCQRowAddr(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucLun;
    U8 ucPln, ucRowAddrIdx = 0;
    FLASH_ADDR atDstFlashAddr[PLN_PER_LUN];
    FLASH_ADDR *pDstFlashAddr[PLN_PER_LUN] = { NULL };
    FLASH_ADDR tFlashAddr = {0};
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    BOOL bSLCMode = (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;

    ucLun = L3_GET_LUN_IN_PU(ptReqEntry->bsTLun);
    if (FCMD_REQ_SUBTYPE_INTRNAL == ptReqEntry->bsReqSubType)
    {
        DBG_Printf("IM 3D-TLC do not support internal copy\n");
        DBG_Getch();
    }

    if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType
        || FCMD_REQ_SUBTYPE_SINGLE_ONEPG == ptReqEntry->bsReqSubType
        || FCMD_REQ_SUBTYPE_SINGLE_TWOPG == ptReqEntry->bsReqSubType)
    {
        ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
        atDstFlashAddr[ucPln].ucLun = ucLun;
        atDstFlashAddr[ucPln].bsPln = ucPln;
        atDstFlashAddr[ucPln].bsSLCMode = bSLCMode;
        atDstFlashAddr[ucPln].usPage = ptCtrlEntry->bsPhyPage;
        atDstFlashAddr[ucPln].usBlock = L2_BbtGetPbnBindingTable(ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsPhyBlk, ucPln);
        pDstFlashAddr[0] = &atDstFlashAddr[ucPln];

        #ifdef SIM
        if (ptReqEntry->tFlashDesc.bsPhyBlk == 1 && (ptReqEntry->bsReqType == FCMD_REQ_TYPE_WRITE || ptReqEntry->bsReqType == FCMD_REQ_TYPE_READ))
        {
            DBG_Printf("  ##L3_IFSetNFCQRowAddr ReqType %d GBBT LUN %d Pln %d GBlk %d_PBNBindingBlk %d Pg %d\n", ptReqEntry->bsReqType, ptReqEntry->bsTLun, ucPln, ptReqEntry->tFlashDesc.bsPhyBlk, atDstFlashAddr[ucPln].usBlock, ptCtrlEntry->bsPhyPage);
        }
        #endif
    }
    else
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            atDstFlashAddr[ucPln].ucLun = ucLun;
            atDstFlashAddr[ucPln].bsPln = ucPln;
            atDstFlashAddr[ucPln].bsSLCMode = bSLCMode;
            atDstFlashAddr[ucPln].usPage = ptCtrlEntry->bsPhyPage;
            atDstFlashAddr[ucPln].usBlock = L2_BbtGetPbnBindingTable(ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsPhyBlk, ucPln);
            pDstFlashAddr[ucPln] = &atDstFlashAddr[ucPln];
        }
    }

    tFlashAddr.ucLun = pDstFlashAddr[0]->ucLun;
    tFlashAddr.bsSLCMode = pDstFlashAddr[0]->bsSLCMode;

    // Target Flash Address Setting

    if (NULL == pDstFlashAddr[1]) // single plane operation, only for slc mode
    {
        tFlashAddr.usPage = pDstFlashAddr[0]->usPage;
        tFlashAddr.bsPln = pDstFlashAddr[0]->bsPln;
        tFlashAddr.usBlock = pDstFlashAddr[0]->usBlock;
        ptNfcqEntry->atRowAddr[ucRowAddrIdx++].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);

        if (FCMD_REQ_SUBTYPE_SINGLE_TWOPG == ptReqEntry->bsReqSubType)
        {
            tFlashAddr.usPage = pDstFlashAddr[0]->usPage;
            if (LOW_PAGE == ptReqEntry->tLocalDesc.bsPairPageType)//dannier HAL_GetFlashPairPageType(tFlashAddr.usPage))
            {
#ifdef FLASH_IM_3DTLC_GEN2
                tFlashAddr.usPage = ptReqEntry->tLocalDesc.bsPairPageNum;
#else
                tFlashAddr.usPage++;
#endif
            }
            else if (EXTRA_PAGE == ptReqEntry->tLocalDesc.bsPairPageType) //dannier HAL_GetFlashPairPageType(tFlashAddr.usPage))
            {
                tFlashAddr.usPage = ptReqEntry->tLocalDesc.bsPairPageNum; //dannier HAL_GetHighPageIndexfromExtra(tFlashAddr.usPage);
            }
            else
            {
                return;
            }
            ptNfcqEntry->atRowAddr[ucRowAddrIdx++].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
        }
    }
    else
    {
        tFlashAddr.usPage = pDstFlashAddr[0]->usPage;
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            tFlashAddr.bsPln = ucPln;
            tFlashAddr.usBlock = pDstFlashAddr[ucPln]->usBlock;
            ptNfcqEntry->atRowAddr[ucRowAddrIdx++].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
        }

        // for simple: only the tlc 2page program need the following process.
        if (TRUE == tFlashAddr.bsSLCMode || ptReqEntry->bsReqType != FCMD_REQ_TYPE_WRITE) return;

        if (LOW_PAGE == ptReqEntry->tLocalDesc.bsPairPageType) //dannier HAL_GetFlashPairPageType(tFlashAddr.usPage))
        {
            tFlashAddr.usPage++;
        }
        else if (EXTRA_PAGE == ptReqEntry->tLocalDesc.bsPairPageType) //dannier HAL_GetFlashPairPageType(tFlashAddr.usPage))
        {
            tFlashAddr.usPage = ptReqEntry->tLocalDesc.bsPairPageNum;//dannier HAL_GetHighPageIndexfromExtra(tFlashAddr.usPage);
        }
        else
        {
            return;
        }

        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            tFlashAddr.bsPln = ucPln;
            tFlashAddr.usBlock = pDstFlashAddr[ucPln]->usBlock;
            ptNfcqEntry->atRowAddr[ucRowAddrIdx++].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
        }
        ASSERT(ucRowAddrIdx <= (PLN_PER_LUN << 1));
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFSetNFCQTrace
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFSetNFCQTrace(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun = ptCtrlEntry->ptReqEntry->bsTLun;

    ptNfcqEntry->atRowAddr[0].bsFCmdPtr = ptCtrlEntry->bsCtrlPtr;
    ptCtrlEntry->bsNfcqPtr = HAL_NfcGetWP(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun));

    return;
}

/*==============================================================================
Func Name  : L3_IFSetNFCQComm
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFSetNFCQComm(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    #if 0
    if (FCMD_REQ_SUBTYPE_SETFEATURE == ptReqEntry->bsReqSubType)
    {
        ASSERT(0x91 == ptReqEntry->ulReqStsAddr);
        ASSERT(0x100 == ptReqEntry->ulSpareAddr || 0x104 == ptReqEntry->ulSpareAddr);//dannier add
        ptNfcqEntry->bsDmaByteEn = TRUE;

        ptNfcqEntry->aByteAddr.usByteAddr = ptReqEntry->ulReqStsAddr;
        ptNfcqEntry->aByteAddr.usByteLength = sizeof(U32);

        ptNfcqEntry->ulSetFeatVal = ptReqEntry->ulSpareAddr;

        ptNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ptCtrlEntry->bsCmdType);

        L3_IFSetNFCQTrace(ptNfcqEntry, ptCtrlEntry);
        return;
    }

    if (FCMD_REQ_SUBTYPE_INTRNAL == ptReqEntry->bsReqSubType)
    {
        ASSERT(FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType);
        ASSERT(FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod);

        ptNfcqEntry->bsDmaByteEn = TRUE;
    }
    else
    #endif
    if (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType || FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType)
    {
        L3_IFSetNFCQScrmable(ptNfcqEntry, ptCtrlEntry);

        L3_IFSetNFCQDataTxfer(ptNfcqEntry, ptCtrlEntry);

        if (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType)
        {
            ptNfcqEntry->bsBypassRdErr = ptCtrlEntry->bsBypassEcc;
            if (TRUE == ptReqEntry->tLocalDesc.bsRawRdCmd && TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
            {
                ptNfcqEntry->bsRawReadEn = TRUE;
            }
        }
        #ifdef XOR_ENABLE
        else
        {
            if (TRUE == L3_IsNeedDoXor(ptReqEntry))
            {
                L3_IFSetNFCQXor(ptNfcqEntry, ptCtrlEntry);
            }
        }
        #endif
    }

    if (TRUE == L3_IFIsEnableSsu0(ptCtrlEntry))
    {
        ptNfcqEntry->bsSsu0En   = TRUE;
        ptNfcqEntry->bsSsu0Ontf = TRUE;
        ptNfcqEntry->bsSsu0Addr = L3_IFGetSsu0AddrOffSet(ptReqEntry);
        ptNfcqEntry->bsSsu0Data = FCMD_REQ_STS_INIT;
    }

    if (TRUE == L3_IFIsEnableSsu1(ptCtrlEntry))
    {
        ptNfcqEntry->bsSsu1En   = TRUE;
        ptNfcqEntry->bsSsu1Ontf = TRUE;
        ptNfcqEntry->bsSsu1Addr = L3_IFGetSsu1AddrOffSet(ptReqEntry);
        ptNfcqEntry->bsSsu1Data = SUBSYSTEM_STATUS_SUCCESS;
    }

    ptNfcqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ptCtrlEntry->bsCmdType);

    L3_IFSetNFCQRowAddr(ptNfcqEntry, ptCtrlEntry);

    L3_IFSetNFCQTrace(ptNfcqEntry, ptCtrlEntry);

    return;
}

/*==============================================================================
Func Name  : L3_IFSetNFCQCustom
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
void L3_IFSetNFCQCustom(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
#if 0
    if (FCMD_REQ_SUBTYPE_SETFEATURE == ptCtrlEntry->ptReqEntry->bsReqSubType)
    {
        ASSERT(0x91 == ptCtrlEntry->ptReqEntry->ulReqStsAddr);
        ASSERT(0x100 == ptCtrlEntry->ptReqEntry->ulSpareAddr || 0x104 == ptCtrlEntry->ptReqEntry->ulSpareAddr);
        return;
    }

    if (FCMD_REQ_SUBTYPE_INTRNAL == ptCtrlEntry->ptReqEntry->bsReqSubType)
    {
        ASSERT(FCMD_REQ_TYPE_WRITE == ptCtrlEntry->ptReqEntry->bsReqType);
        ASSERT(FCMD_REQ_TLC_BLK == ptCtrlEntry->ptReqEntry->tFlashDesc.bsBlkMod);
        return;
    }
#endif
    if (FCMD_REQ_TYPE_RDSTS == ptCtrlEntry->ptReqEntry->bsReqType)
    {
        return;
    }

    L3_SetNfcqCustom(ptNfcqEntry, ptCtrlEntry);

    return;
}

#ifdef DATA_EM_ENABLE
LOCAL void L3_DataEnDecryption(NFCQ_ENTRY *ptNFCQEntry, FCMD_INTR_CTRL_ENTRY *ptFCtlEntry)
{
#ifndef HAL_NFC_TEST
    FCMD_REQ_ENTRY *pReqEntry = ptFCtlEntry->ptReqEntry;
    FCMD_FLASH_DESC tFlashDes = pReqEntry->tFlashDesc;
    RED *pSpare = (RED *)pReqEntry->ulSpareAddr;

    // only the host write & read need to open the em and lba check.
    if (TRUE == pReqEntry->bsTableReq)
        return;

    if (FCMD_REQ_TYPE_WRITE == pReqEntry->bsReqType)
    {
        ptNFCQEntry->bsEMEn = TRUE;
    }

    if (FCMD_REQ_TYPE_READ == pReqEntry->bsReqType)
    {
        //DBG_Printf("RD EM, LUN %d SecStart %d Len %d Blk %d_%d page %d bsRdRedOnly %d ReqSubType %d\n",
        //    ptFCtlEntry->ptReqEntry->bsTLun, tFlashDes.bsSecStart, tFlashDes.bsSecLen,
        //    tFlashDes.bsVirBlk, tFlashDes.bsPhyBlk, tFlashDes.bsVirPage, tFlashDes.bsRdRedOnly, pReqEntry->bsReqSubType);

        ptNFCQEntry->bsEMEn = TRUE;

        if (TRUE == ptNFCQEntry->bsEMEn || TRUE == ptNFCQEntry->bsLbaChk)
        {
            if (FCMD_REQ_SUBTYPE_NORMAL == pReqEntry->bsReqSubType)
            {
                ptNFCQEntry->bsRdLba = tFlashDes.bsSecStart >> SEC_PER_LPN_BITS;
            }
            else if (FCMD_REQ_SUBTYPE_SINGLE == pReqEntry->bsReqSubType)
            {
                ptNFCQEntry->bsRdLba = (tFlashDes.bsSecStart + (tFlashDes.bsPlnNum << SEC_PER_PHYPG_BITS)) >> SEC_PER_LPN_BITS;
            }
        }
    }
#else//for UT Test

    // only the host write & read need to open the em and lba check.
    if (FCMD_REQ_TYPE_WRITE == ptFCtlEntry->ptReqEntry->bsReqType)
    {
        ptNFCQEntry->bsEMEn = TRUE;
    }

    if (FCMD_REQ_TYPE_READ == ptFCtlEntry->ptReqEntry->bsReqType)
    {
        ptNFCQEntry->bsEMEn = TRUE;
        ptNFCQEntry->bsRdLba = ptFCtlEntry->ptReqEntry->tFlashDesc.bsSecStart >> SEC_PER_LPN_BITS;
    }

#endif
    return;
}
#endif

/*==============================================================================
Func Name  : L3_IFConfigNFCQ
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFConfigNFCQ(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    NFCQ_ENTRY *ptNfcqEntry;

    ptNfcqEntry = L3_IFAllocNFCQ(ptCtrlEntry);

    L3_IFSetNFCQComm(ptNfcqEntry, ptCtrlEntry);

#ifdef DATA_EM_ENABLE
    L3_DataEnDecryption(ptNfcqEntry, ptCtrlEntry);
#endif

    L3_IFSetNFCQCustom(ptNfcqEntry, ptCtrlEntry);

#ifdef L3_DBG_ERR_INJ_EN
    L3_DbgSetNFCQErrInj(ptNfcqEntry, ptCtrlEntry);
#endif

    return;
}

/*==============================================================================
Func Name  : L3_IFTriggerNFC
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
#ifdef P_DBG
#define DBG_001_DEPTH 128
U32 g_DbgNfcqIndex[16] = {0};
LOCAL U32 g_DbgNfcqIndexT[16] = {0};
U32 g_DbgNfcqStatus[16][DBG_001_DEPTH][1] = {0};
#endif
LOCAL void L3_IFTriggerNFC(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun;
    BOOL bIsXorParityWrEn = FALSE;
    FLASH_ADDR tFlashAddr;

#ifdef XOR_ENABLE
    bIsXorParityWrEn = (TRUE == L3_IsDataDisperse(ptCtrlEntry->ptReqEntry)) ? TRUE : FALSE;
#endif
    ucTLun = ptCtrlEntry->ptReqEntry->bsTLun;
    tFlashAddr.ucPU = L3_GET_PU(ucTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ucTLun);
    tFlashAddr.bsPln = ptCtrlEntry->ptReqEntry->tFlashDesc.bsPlnNum;
    tFlashAddr.bsSLCMode = (FCMD_REQ_SLC_BLK == ptCtrlEntry->ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
  
#ifdef P_DBG
    BOOL bNeed = FALSE;
    U32 ulIndex = g_DbgNfcqIndex[ucTLun];
    //if (TRUE == ptCtrlEntry->ptReqEntry->tFlashDesc.bsHostRdEn)
    if (NF_PRCQ_READ_MULTIPLN == ucCmdType && REQ_STS_UPT_AUTO==ptCtrlEntry->ptReqEntry->bsReqUptMod)
    {
        g_DbgNfcqStatus[ucTLun][ulIndex][0] = HAL_NfcGetEmpty(tFlashAddr.ucPU, tFlashAddr.ucLun);
        g_DbgNfcqStatus[ucTLun][ulIndex][0] <<= 8;
        g_DbgNfcqStatus[ucTLun][ulIndex][0] |= HAL_NfcGetWP(tFlashAddr.ucPU, tFlashAddr.ucLun);
        g_DbgNfcqStatus[ucTLun][ulIndex][0] <<= 8;
        g_DbgNfcqStatus[ucTLun][ulIndex][0] |= HAL_NfcGetRP(tFlashAddr.ucPU, tFlashAddr.ucLun);
        bNeed = TRUE;
    }
#endif

    HAL_NfcCmdTrigger(&tFlashAddr, ptCtrlEntry->bsCmdType, bIsXorParityWrEn, ptCtrlEntry->bsXoreId);
    
#ifdef P_DBG
    if (TRUE == bNeed)
    {
        g_DbgNfcqStatus[ucTLun][ulIndex][0] <<= 8;
        g_DbgNfcqStatus[ucTLun][ulIndex][0] |= HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (TRUE != HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
            g_DbgNfcqIndexT[ucTLun]++;
        #if 0
        U8 ucIndex;
        U32 ulCurIndexT = g_DbgNfcqIndexT[ucTLun];
        for (ucIndex = 0; ucIndex < SUBSYSTEM_LUN_NUM; ucIndex++)
        {
            if (ucIndex == ucTLun) continue;

            if (ulCurIndexT >= g_DbgNfcqIndexT[ucIndex])
            {
                g_DbgNfcqStatus[ucTLun][0][1] += ulCurIndexT-g_DbgNfcqIndexT[ucIndex];
            }
            else
            {
                g_DbgNfcqStatus[ucTLun][0][1] += g_DbgNfcqIndexT[ucIndex]-ulCurIndexT;
            }
        }
        #endif
        
        g_DbgNfcqIndex[ucTLun] = (ulIndex+1)%DBG_001_DEPTH;
        //g_DbgNfcqIndexT[ucTLun]++;
    }
#endif
    return;
}

BOOL L3_IFIsManulRptReq(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    return (REQ_STS_UPT_MANUL == ptCtrlEntry->ptReqEntry->bsReqUptMod) ? TRUE : FALSE;
}
BOOL L3_IFIsAutoRptReq(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    return (REQ_STS_UPT_AUTO == ptCtrlEntry->ptReqEntry->bsReqUptMod) ? TRUE : FALSE;
}
BOOL L3_IFIsForceRecycle(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bEnable = FALSE;

    if ((NULL != ptCtrlEntry->ptErrHEntry)
     || (1 < ptCtrlEntry->bsMultiStep))
    {
        bEnable = TRUE;
    }

    return bEnable;
}
/*==============================================================================
Func Name  : L3_IFIsRecycled
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.20 JasonGuo create function
==============================================================================*/
BOOL L3_IFIsRecycled(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bNeedRecyle = FALSE;

    if (TRUE == L3_IFIsManulRptReq(ptCtrlEntry))
    {
        bNeedRecyle = TRUE;
    }

    return bNeedRecyle;
}

/*==============================================================================
Func Name  : L3_IFUpdtIntrCtrl
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFUpdtIntrCtrl(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun = ptCtrlEntry->ptReqEntry->bsTLun;

    if (TRUE == L3_IFIsRecycled(ptCtrlEntry))
    {
        L3_FCMDQSetIntrRptr(ucTLun, ptCtrlEntry->bsCtrlPtr, FALSE);
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFSetFlashMonitor
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
1. 2017.7.20 NickLiou create function
==============================================================================*/
void L3_IFSetFlashMonitor(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucTLun = ptReqEntry->bsTLun;

    if (FALSE == ptCtrlEntry->bsIntrReq)
    {
        U8 ucPri = ptReqEntry->bsFCmdPri;
        U8 ucReqPtr = ptReqEntry->bsReqPtr;
        ASSERT(FCMD_REQ_STS_POP == L2_FCMDQGetReqSts(ucTLun, ucPri, ucReqPtr));
    }

    L3_FMSetPhyBlk(ucTLun, ptReqEntry->tFlashDesc.bsPhyBlk);
    L3_FMSetPhyPage(ucTLun, ptCtrlEntry->bsPhyPage);
    L3_FMSetPlnNum(ucTLun, ptReqEntry->tFlashDesc.bsPlnNum);
    L3_FMSetCmdType(ucTLun, ptCtrlEntry->bsCmdType);

    return;
}

/*==============================================================================
Func Name  : L3_IFClearFlashMonitor
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
1. 2017.7.20 NickLiou create function
==============================================================================*/
void L3_IFClearFlashMonitor(U8 ucTLun)
{
    L3_FMSetPhyBlk(ucTLun, INVALID_4F);
    L3_FMSetPhyPage(ucTLun, INVALID_4F);
    L3_FMSetPlnNum(ucTLun, MSK_F);
    L3_FMSetCmdType(ucTLun, INVALID_2F);

    return;
}
/*==============================================================================
Func Name  : L3_IFUpdateFlashMonitor
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFUpdateFlashMonitor(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucTLun = ptReqEntry->bsTLun;
    
#if (defined(HOST_SATA) || defined(SIM))
    if (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
    {
        if (ptReqEntry->bsReqType == FCMD_REQ_TYPE_WRITE)
        {
            if (ptReqEntry->tFlashDesc.bsVirPage < ptReqEntry->tLocalDesc.bsPairPageNum)
            {   
                //B0KB Low + Upper pages : ulPrgTime(+2) ; Upper page num = Low pages num +1
                //ptReqEntry->tLocalDesc.bsPairPageCnt = 2
                L3_FMUpdtUsrOpCnt(ucTLun, FCMD_REQ_TYPE_WRITE, 2);
            }
            else
            {   
                //Low page only : ulPrgTime(+1) or TLC Extra + Upper pages : ulPrgTime(+1)
                L3_FMUpdtUsrOpCnt(ucTLun, FCMD_REQ_TYPE_WRITE, 1);
            }
        }
        else
        {
            L3_FMUpdtUsrOpCnt(ucTLun, ptReqEntry->bsReqType, 0);
        }
    }
    else
    {
        if (ptReqEntry->bsReqType == FCMD_REQ_TYPE_WRITE)
        {  
            //Low page only : ulPrgTime(+1)
            L3_FMUpdtUsrOpCnt(ucTLun, FCMD_REQ_TYPE_WRITE, 1);
        }
    }
#else
    //Only gather smart info statistics of TLC blocks.
    if (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
    {
        L3_FMUpdtUsrOpCnt(ucTLun, ptReqEntry->bsReqType, 1);
    }
#endif

    if (NULL == ptCtrlEntry->ptErrHEntry)
    {
        L3_IFSetFlashMonitor(ptCtrlEntry);
    }
    else
    {
        L3_IFClearFlashMonitor(ucTLun);
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFSendNormalFCmd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_IFSendNormalFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bResult;

#ifdef SIM
    L3_DbgFCmdCntAdd(ptCtrlEntry);
#endif

    bResult = L3_IFAllocResource(ptCtrlEntry);
    if (TRUE == bResult)
    {
        L3_IFAdaptFCmd(ptCtrlEntry);

        L3_IFConfigNFCQ(ptCtrlEntry);

        L3_IFUpdtIntrCtrl(ptCtrlEntry);

        L3_IFUpdateFlashMonitor(ptCtrlEntry);

        L3_IFTriggerNFC(ptCtrlEntry);
    }

    return bResult;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_IFSendFCmd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_IFSendFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bResult = TRUE;

#if 0
    if (TRUE == L3_IFIsSpecialFCmd(ptCtrlEntry))
    {
        L3_IFSendSpecialFCmd(ptCtrlEntry);
        #ifdef SIM
        L3_DbgFCmdPrint(ptCtrlEntry, "Special");
        #endif
    }
    else
#endif
    {
        bResult = L3_IFSendNormalFCmd(ptCtrlEntry);
        #ifdef SIM
        L3_DbgFCmdPrint(ptCtrlEntry, (TRUE == bResult) ? "Normal_Success" : "Normal_Fail");
        #endif
    }

    return bResult;
}

/*==============================================================================
Func Name  : L3_IFIsFCmdFinished
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_IFIsFCmdFinished(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bFinish;
    U8 ucTLun;

    ucTLun = ptCtrlEntry->ptReqEntry->bsTLun;
    bFinish = HAL_NfcGetFinish(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun), ptCtrlEntry->bsNfcqPtr);

    return bFinish;
}

/*==============================================================================
Func Name  : L3_IFUpdtRedData
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
void L3_FillRedForEM(U32 *pTargetRed, U32* pSpare)
{
    U32 Pln;
    U32* pSrc;

    for (Pln = 0; Pln < PLN_PER_LUN; Pln++)
    {
        pSrc = pSpare + 4 + RED_SZ_DW*Pln;
        COM_MemCpy(pTargetRed, pSrc, (RED_SZ_DW - 4));
        pTargetRed += (RED_SZ_DW - 4);
    }
}
LOCAL void L3_IFUpdtRedData(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun;
    U32 *pSrcData, *pDstData;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (FCMD_REQ_TYPE_READ != ptReqEntry->bsReqType || 0 == ptReqEntry->ulSpareAddr)
    {
        return;
    }

    ucTLun = ptReqEntry->bsTLun;
    if (TRUE != ptCtrlEntry->bsIntrReq)
    {
        pSrcData = (U32 *)RED_ABSOLUTE_ADDR(MCU1_ID, ucTLun, ptReqEntry->bsReqPtr);
    }
    else
    {
        pSrcData = (U32 *)RED_ABSOLUTE_ADDR(MCU2_ID, ucTLun, ptCtrlEntry->bsCtrlPtr);
    }

    pDstData = (U32 *)ptReqEntry->ulSpareAddr;

#ifdef DATA_EM_ENABLE
    if (ptReqEntry->bsTableReq || TRUE == ptCtrlEntry->bsIntrReq)
    {
        COM_MemCpy(pDstData, pSrcData, RED_SW_SZ_DW);
    }
    else
    {
        L3_FillRedForEM(pDstData, pSrcData);
    }
#else
    COM_MemCpy(pDstData, pSrcData, RED_SW_SZ_DW);
#endif

    return;
}

LOCAL U8 L3_UpdatePatrolReadStatus(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U16 usErrBitCnt = 0;
    DEC_SRAM_STATUS_ENTRY *ptDecSramSts;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    U8 ucTLun, ucPU, ucLunInPU, ucLevel, ucPln, ucPlnCnt, ucStatus;

    ucTLun = ptReqEntry->bsTLun;
    ucPU = L3_GET_PU(ucTLun);
    ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);
    ucLevel = ptCtrlEntry->bsNfcqPtr;

    if (NULL != ptErrHEntry && NF_ERR_TYPE_UECC == ptErrHEntry->bsErrCode)  //Empty or Uecc process
    {
        if (TRUE == ptErrHEntry->bsEmptyPg)
        {
            ucStatus = SUBSYSTEM_STATUS_EMPTY_PG;
        }
        else
        {
            ucStatus = SUBSYSTEM_STATUS_FAIL;
        }
    }
    else
    {
        ucPlnCnt = (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType) ? 1 : PLN_PER_LUN;

        for (ucPln = 0; ucPln < ucPlnCnt; ucPln++)
        {
            ptDecSramSts = (DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLunInPU][ucLevel][ucPln]);
            usErrBitCnt += ptDecSramSts->bsErrCntAcc;
        }

        if (usErrBitCnt >(16 * ucPlnCnt) * PATROL_READ_FBC_TH_CW)
        {
            ucStatus = SUBSYSTEM_STATUS_FAIL;
        }
        else
        {
            ucStatus = SUBSYSTEM_STATUS_SUCCESS;
        }
    }

    return ucStatus;
}

LOCAL U8 L3_UpdateReadStatus(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucStatus;

#ifdef SIM
    ucStatus = SUBSYSTEM_STATUS_FAIL;
#else
    U8 ucRp, ucPU, ucLun;
    FLASH_STATUS_ENTRY *pFlashStatus;

    ucPU = L3_GET_PU(ptReqEntry->bsTLun);
    ucLun = L3_GET_LUN_IN_PU(ptReqEntry->bsTLun);
    ucRp = ptCtrlEntry->bsNfcqPtr;

    pFlashStatus = (FLASH_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLun][ucRp][0]);

    // DBG_Printf("Flash Status = 0x%x\n", pFlashStatus->bsFlashStatus);
    if ((pFlashStatus->bsFlashStatus & 0x1) != 0)
    {
        ucStatus = SUBSYSTEM_STATUS_FAIL;
    }
    else
    {
        ucStatus = SUBSYSTEM_STATUS_SUCCESS;
    }
#endif

    return ucStatus;
}

/*==============================================================================
Func Name  : L3_IFUpdtReqStatus
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.5 JasonGuo create function
==============================================================================*/
void L3_IFUpdtReqStatus(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucTLun, ucPri, ucReqPtr, ucStatus;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;

    ucTLun = ptReqEntry->bsTLun;

    // Report the request status to the caller
    if (0 != ptReqEntry->ulReqStsAddr)
    {
        if (TRUE == ptReqEntry->tLocalDesc.bsPatrolRdCmd && TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
        {
            ucStatus = L3_UpdatePatrolReadStatus(ptCtrlEntry);
        }
        else if (FCMD_REQ_TYPE_RDSTS == ptReqEntry->bsReqType)
        {
            ucStatus = L3_UpdateReadStatus(ptCtrlEntry);
        }
        else if (NULL == ptErrHEntry)
        {
            ucStatus = SUBSYSTEM_STATUS_SUCCESS;
        }
        else if (FALSE == ptErrHEntry->bsErrRpt)
        {
            if (FALSE == ptErrHEntry->tUeccHCtrl.tRetry.bsEnable)
            {
                ucStatus = SUBSYSTEM_STATUS_SUCCESS;
            }
            else
            {
                ucStatus = SUBSYSTEM_STATUS_RETRY_SUCCESS;
            }
        }
        else
        {
            if (TRUE == ptErrHEntry->bsEmptyPg)
            {
                ASSERT(NF_ERR_TYPE_UECC == ptErrHEntry->bsErrCode);
                ucStatus = SUBSYSTEM_STATUS_EMPTY_PG;
            }
            else if (NF_ERR_TYPE_RECC == ptErrHEntry->bsErrCode)
            {
                ucStatus = SUBSYSTEM_STATUS_RECC;
            }
            else
            {
                ucStatus = SUBSYSTEM_STATUS_FAIL;
            }
        }
        if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
        {
            *(U8 *)ptReqEntry->ulReqStsAddr = ucStatus;
        }
        else
        {
            if (TRUE == ptReqEntry->tLocalDesc.bsRdtCmd)
            {
                if (NULL == ptErrHEntry)
                {
                    *(U8 *)ptReqEntry->ulReqStsAddr = ucStatus;
                }
                else
                {
                    *(U8 *)ptReqEntry->ulReqStsAddr = ucStatus | (ptErrHEntry->bsPlnErrInfo << 4);
                }
            }
            else
            {
                *(U8 *)ptReqEntry->ulReqStsAddr = ucStatus;
            }
        }
    }

#ifdef SIM
    if (TRUE == L3_IFIsManulRptReq(ptCtrlEntry))
    {
        L3_DbgFCmdPrint(ptCtrlEntry, "Normal_Recycle");
        L3_DbgFCmdManulChk(HAL_NfcGetPhyPU(L3_GET_PU(ucTLun)) + L3_GET_LUN_IN_PU(ucTLun) * SUBSYSTEM_PU_MAX, ptCtrlEntry->bsIntrReq);
    }
    else if (TRUE == L3_IFIsAutoRptReq(ptCtrlEntry))
    {
        L3_DbgFCmdPrint(ptCtrlEntry, "Auto_Recycle");
        L3_DbgFCmdAutoChk(HAL_NfcGetPhyPU(L3_GET_PU(ucTLun)) + L3_GET_LUN_IN_PU(ucTLun) * SUBSYSTEM_PU_MAX);
    }
    else if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        L3_DbgFCmdPrint(ptCtrlEntry, "HostRd_Recycle");
        L3_DbgFCmdHostRdChk(HAL_NfcGetPhyPU(L3_GET_PU(ucTLun)) + L3_GET_LUN_IN_PU(ucTLun) * SUBSYSTEM_PU_MAX);
    }
    else
    {
        L3_DbgFCmdPrint(ptCtrlEntry, "Null_Recycle");
        L3_DbgFCmdNullChk(HAL_NfcGetPhyPU(L3_GET_PU(ucTLun)) + L3_GET_LUN_IN_PU(ucTLun) * SUBSYSTEM_PU_MAX);
    }
#endif

    if (FALSE == ptCtrlEntry->bsIntrReq)
    {
        // Release the FCMDQ Level
        ucPri = ptReqEntry->bsFCmdPri;
        ucReqPtr = ptReqEntry->bsReqPtr;
        ASSERT(FCMD_REQ_STS_POP == L2_FCMDQGetReqSts(ucTLun, ucPri, ucReqPtr));
        L2_FCMDQSetReqSts(ucTLun, ucPri, ucReqPtr, FCMD_REQ_STS_INIT);
    }

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQMarkPrePageEn
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      : Low/High <-- FlashMonitor
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_IFMarkPrePageEn(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    return;
}

/*==============================================================================
Func Name  : L3_IFPopFCmdQ
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
void L3_IFPopFCmdQ(U8 ucTLun)
{
    //FCMD_REQ_PRI eFCmdPri;
    FCMD_REQ_ENTRY *ptReqEntry;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;

    //eFCmdPri = L3_FCMDQArbReqPri(ucTLun);
    ptReqEntry = L3_FCMDQPopReqEntry(ucTLun, 0);

    ptCtrlEntry = L3_FCMDQAllocIntrEntry(ucTLun);
    ptCtrlEntry->ptReqEntry = ptReqEntry;

    L3_IFSendFCmd(ptCtrlEntry);

    return;
}

#ifdef SCAN_BLOCK_N1
extern U8 MCU2_DRAM_TEXT L3_ErrHGetRetryVtIndex(U8 ucTLun, U8 ucIndex, BOOL bTlcMode, U16 usWLType, U8 ucPageType);
void L3_LocalReadPrintFBC(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    BOOL bTLCMode = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
    UECC_ERRH_CTRL *ptUeccHCtrl;
    U8 ucTLun, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ucWlType, ucPageType;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage;
    U32 ulLBA;

    ucTLun = ptReqEntry->bsTLun;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ucSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ucSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ucLpnBitmap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;
    ucWlType = HAL_GetFlashWlType(usPhyPage, bTLCMode);
    ucPageType = HAL_GetFlashPairPageType(usPhyPage);

    DEC_SRAM_STATUS_ENTRY *ptDecSramSts;
    U8 ucPU, ucLunInPU, ucLevel;
    U16 errBit[PLN_PER_LUN];
    ucPU = L3_GET_PU(ucTLun);
    ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);
    ucLevel = ptCtrlEntry->bsNfcqPtr;

    if (ptCtrlEntry->ptReqEntry->bsReqType == FCMD_REQ_TYPE_READ && bTLCMode)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            ptDecSramSts = (DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLunInPU][ucLevel][ucPln]);
            errBit[ucPln] = ptDecSramSts->bsErrCntAcc;
        }
        if (ptCtrlEntry->ptErrHEntry != 0 && (TRUE != ptReqEntry->tLocalDesc.bsPatrolRdCmd))
        {
            ptUeccHCtrl = &ptCtrlEntry->ptErrHEntry->tUeccHCtrl;
            DBG_Printf("re L %d B %d %d P %d %d C %d %d S %d %d %d R %d %d E %d F %d %d\n",
            ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap,
            ptUeccHCtrl->tRetry.bsTime, L3_ErrHGetRetryVtIndex(ucTLun, ptUeccHCtrl->tRetry.bsTime, bTLCMode, ucWlType, ucPageType),
            pPBIT[0]->m_PBIT_Entry[L2_GET_SPU(ucTLun)][usPhyBlk].EraseCnt, errBit[0], errBit[1]);
        }
        else if (TRUE != ptReqEntry->tLocalDesc.bsPatrolRdCmd)
        {
            DBG_Printf("re L %d B %d %d P %d %d C %d %d S %d %d %d R %d %d E %d F %d %d\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, 0xFF,
                0xFF, pPBIT[0]->m_PBIT_Entry[L2_GET_SPU(ucTLun)][usPhyBlk].EraseCnt, errBit[0], errBit[1]);
        }
    }
}
#endif
/*==============================================================================
Func Name  : L3_IFRecycleFCmdQ
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
void L3_IFRecycleFCmdQ(U8 ucTLun)
{
    U8 ucRptr;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;

    ucRptr = L3_FCMDQGetIntrRptr(ucTLun, FALSE);
    ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucRptr, FALSE);

    if (TRUE == L3_IFIsFCmdFinished(ptCtrlEntry))
    {
#ifdef XOR_ENABLE
        if (TRUE == L3_IsDataDisperse(ptCtrlEntry->ptReqEntry))
        {
            BOOL bXoreReleaseFinish = L3_XoreRelease(ptCtrlEntry);
            if (bXoreReleaseFinish == TRUE)
            {
                L3_IFUpdtReqStatus(ptCtrlEntry);
                L3_FCMDQSetIntrRptr(ucTLun, INVALID_DPTR, FALSE);
            }
        }
        else
#endif
        {
#ifdef SCAN_BLOCK_N1
            L3_LocalReadPrintFBC(ptCtrlEntry);
#endif
            L3_IFUpdtRedData(ptCtrlEntry);
            L3_IFUpdtReqStatus(ptCtrlEntry);
            L3_FCMDQSetIntrRptr(ucTLun, INVALID_DPTR, FALSE);
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFNFCIRSInit
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void L3_IFNFCIRSInit(void)
{
    HAL_NfcSetRedInDramBase((U32)g_pRedBaseAddr);
    HAL_NfcSetSsuInOtfbBase((U32)g_ptFCmdReqSts);

    return;
}


/*====================End of this file========================================*/

