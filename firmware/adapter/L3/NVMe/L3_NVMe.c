/*******************************************************************************
* Copyright (C), 2016 VIA Technologies, Inc. All Rights Reserved.              *
*                                                                              *
* This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc. and may  *
* contain trade secrets and/or other confidential information of VIA           *
* Technologies, Inc. This file shall not be disclosed to any third party, in   *
* whole or in part, without prior written consent of VIA.                      *
*                                                                              *
* THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
* WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
* AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
* NON-INFRINGEMENT.                                                            *
********************************************************************************
* File Name    : L3_NVME.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_HSG.h"
#include "HAL_SGE.h"
#include "HAL_MultiCore.h"
#include "FW_SMSG.h"
#include "L3_BufMgr.h"
#include "L3_FCMDQ.h"
#include "L3_ErrHBasic.h"
#include "L3_Interface.h"
#include "L3_NVME.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
//specify file name for Trace Log
#define TL_FILE_NUM  L3_NVMe_c

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL MCU12_VAR_ATTR NFCQ_ENTRY *g_pNfcqForHalDebug;
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL void L3_HandleHostReadSuccess(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

/*==============================================================================
Func Name  : L3_BuildSGQEntry
Input      : NFCQ_ENTRY* ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL void L3_BuildSGQEntry(NFCQ_ENTRY* ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucPU, ucLunInPU;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_DATA_TX_CTRL *ptHsgCtrl = &ptCtrlEntry->tDTxCtrl;
    HMEM_DPTR *ptHmemDptr = &ptReqEntry->tHostDesc.tHostDptr;

    if (FALSE == ptHsgCtrl->bsAddChainEn)
    {
        ptHsgCtrl->bsAddChainEn = TRUE;

        HAL_AddFinishReqLength(ptHmemDptr->bsCmdTag, ptReqEntry->tFlashDesc.bsSecLen);
    }

    g_pNfcqForHalDebug = ptNfcqEntry;
    ucPU = L3_GET_PU(ptReqEntry->bsTLun);
    ucLunInPU = L3_GET_LUN_IN_PU(ptReqEntry->bsTLun);

    HAL_SgqBuildEntry(ptHmemDptr->bsCmdTag,ptHsgCtrl->bsFstEngineID, ucPU, ucLunInPU, FALSE);

    return;
}

/*==============================================================================
Func Name  : L3_NvmeAllocHSG
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
LOCAL BOOL L3_NvmeAllocHSG(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U16 usCurHsgId, usNextHsgId;
    U32 ulRemainSecLen;
    BOOL bIsSGQIdle;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_DATA_TX_CTRL *ptHsgCtrl = &ptCtrlEntry->tDTxCtrl;
    HMEM_DPTR *ptHmemDptr = &ptReqEntry->tHostDesc.tHostDptr;

    bIsSGQIdle = HAL_SgqIsBusy(L3_GET_PU(ptReqEntry->bsTLun), L3_GET_LUN_IN_PU(ptReqEntry->bsTLun)) ? FALSE : TRUE;

    if (TRUE == ptHsgCtrl->bsBdEngineDone)
    {
        return bIsSGQIdle;
    }

    if (INVALID_4F == ptHsgCtrl->bsFstEngineID)
    {
        if (FALSE == HAL_GetHsg(&usCurHsgId))
        {
            return FALSE;
        }

        ptHsgCtrl->bsFstEngineID = usCurHsgId;
        ptHsgCtrl->bsCurEngineID = usCurHsgId;

        ptHsgCtrl->bsRemSecLen = ptReqEntry->tFlashDesc.bsSecLen << SEC_SZ_BITS;
        ptHsgCtrl->bsPrdOrPrpIdx = ptHmemDptr->bsPrdOrPrpIndex;
        ptHsgCtrl->bsLstSecRemain = ptHmemDptr->bsLastSecRemain;
        ptHsgCtrl->bsOffSet = ptHmemDptr->bsOffset;
    }

    while (0 < ptHsgCtrl->bsRemSecLen)
    {
        if (FALSE == HAL_CheckForLastHsg(ptHmemDptr, ptHsgCtrl->bsRemSecLen))
        {
            if (FALSE == HAL_GetHsg(&usNextHsgId))
            {
                break;
            }
        }
        else
        {
            usNextHsgId = INVALID_4F;
        }

        ulRemainSecLen = ptHsgCtrl->bsRemSecLen;
        HAL_BuildHsg(ptHmemDptr, &ulRemainSecLen, ptHsgCtrl->bsCurEngineID, usNextHsgId);
        ptHsgCtrl->bsRemSecLen = ulRemainSecLen;
        HAL_SetHsgSts(ptHsgCtrl->bsCurEngineID, TRUE);

        if (INVALID_4F != usNextHsgId)
        {
            ptHsgCtrl->bsCurEngineID = usNextHsgId;
        }
        else
        {
            ptHsgCtrl->aSecAddr[0].bsSecStart = ptReqEntry->tFlashDesc.bsSecStart;
            ptHsgCtrl->aSecAddr[0].bsSecLength= ptReqEntry->tFlashDesc.bsSecLen;
            ptHsgCtrl->bsSecAddrIndex = 1;
            ptHsgCtrl->bsDmaTotalLen = ptReqEntry->tFlashDesc.bsSecLen;

            ptHsgCtrl->bsBdEngineDone = TRUE;

            if (FALSE == bIsSGQIdle)
            {
                bIsSGQIdle = HAL_SgqIsBusy(L3_GET_PU(ptReqEntry->bsTLun), L3_GET_LUN_IN_PU(ptReqEntry->bsTLun)) ? FALSE : TRUE;
            }
            break;
        }
    }

    return (bIsSGQIdle && ptHsgCtrl->bsBdEngineDone);
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/

/*==============================================================================
Func Name  : L3_AllocResource
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : GLOBAL
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
GLOBAL BOOL L3_AllocResource(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bResult;

    if (TRUE == ptCtrlEntry->ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        bResult = L3_NvmeAllocHSG(ptCtrlEntry);
    }
    else
    {
        bResult = L3_IFAllocDSG(ptCtrlEntry);
    }

    return bResult;
}

/*==============================================================================
Func Name  : L3_SetNfcqCustom
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
void L3_SetNfcqCustom(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        ptNfcqEntry->bsOntfEn = TRUE;
        ptNfcqEntry->bsTrigOmEn = TRUE;

        L3_BuildSGQEntry(ptNfcqEntry, ptCtrlEntry);
    }
    else if (TRUE == ptCtrlEntry->tDTxCtrl.bsDSGEn)
    {
        ptNfcqEntry->bsDsgEn = TRUE;
        ptNfcqEntry->bsFstDsgPtr = ptCtrlEntry->tDTxCtrl.bsFstEngineID;
    }

    return;
}

/*==============================================================================
Func Name  : L3_RstDatTxCtrler
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : GLOBAL
Discription:
Usage      :
History    :
    1. 2016.6.23 JasonGuo create function
==============================================================================*/
GLOBAL void L3_RstDatTxCtrler(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_DATA_TX_CTRL *ptDTxCtrl = &ptCtrlEntry->tDTxCtrl;
    HMEM_DPTR *ptHmemDptr;

    ptDTxCtrl->bsBdEngineDone = FALSE;
    ptDTxCtrl->bsFstEngineID = INVALID_4F;
    ptDTxCtrl->bsSecAddrIndex = 0;
    ptDTxCtrl->bsDmaTotalLen = 0;

    if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        ptHmemDptr = &ptReqEntry->tHostDesc.tHostDptr;
        ptHmemDptr->bsPrdOrPrpIndex = ptDTxCtrl->bsPrdOrPrpIdx;
        ptHmemDptr->bsLastSecRemain = ptDTxCtrl->bsLstSecRemain;
        ptHmemDptr->bsOffset = ptDTxCtrl->bsOffSet;
    }
    else
    {
        ptDTxCtrl->bsLpnBitMapPos = 0;
    }

    return;
}

GLOBAL void L3_SGEReset(void)
{
#ifndef SIM
    /* Resets SGE. */
    HAL_SGEReset();
#endif
    return;
}

/*==============================================================================
Func Name  : L3_NVMeHandleUeccRspMarker
Input      : U8 ucPu
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
BOOL L3_NVMeHandleUeccRspMarker(U8 ucPu)
{
    while (SMS_UECC_CLEAR != g_apSubMcShareData->tRspMarker.SmsgUeccRspMarker.ulMarkerType);

    if (ucPu != g_apSubMcShareData->tRspMarker.SmsgUeccRspMarker.ulPUID)
    {
        DBG_Printf("Error RspMarker.PUID(%d) != Waiting PUID(%d)\n",
            g_apSubMcShareData->tRspMarker.SmsgUeccRspMarker.ulPUID, ucPu);
        DBG_Getch();
    }
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_UECC_RSP_MARKER);
    g_apSubMcShareData->tRspMarker.SmsgUeccRspMarker.ulPUID = 0;
    g_apSubMcShareData->tRspMarker.SmsgUeccRspMarker.ulMarkerType = SMS_UECC_MARKER_VALID;
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_UECC_RSP_MARKER);

    return TRUE;
}

/*==============================================================================
Func Name  : L3_NvmeReportSMSG
Input      : U8 ucCmdTag
             U8 ulPUID
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void L3_NvmeReportSMSG(U8 ucCmdTag, U8 ulPUID)
{
    SMSG tSMSG;

    tSMSG.ulMsgCode      = SMSG_SUBSYS_ERROR;
    tSMSG.ulMsgParam[0]  = SUBSYS_ERROR_UECC;
    tSMSG.ulMsgParam[1]  = ucCmdTag & 0xFF;
    tSMSG.ulMsgParam[1] |= ulPUID << 8 & 0xFF00;
    FW_ReportSMSG(&tSMSG);

    return;
}

/*==============================================================================
Func Name  : L3_HostReadEmpty
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void L3_HostReadEmpty(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    ASSERT(NF_ERR_TYPE_UECC == ptErrHEntry->bsErrCode);
    ASSERT(TRUE == ptErrHEntry->bsEmptyPg);

    ptCtrlEntry->bsBypassEcc = TRUE;
    while (TRUE != L3_ErrHRead(ptCtrlEntry));
    ASSERT(FALSE == ptErrHEntry->bsErrHold);
    while (TRUE != L3_IFIsFCmdFinished(ptCtrlEntry));

    return;
}

/*==============================================================================
Func Name  : L3_HostReadRecover
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
             BOOL bRcvFail
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void L3_HostReadRecover(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, BOOL bRcvFail)
{
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    ASSERT(NF_ERR_TYPE_UECC == ptErrHEntry->bsErrCode);
    ASSERT(NF_ERR_TYPE_UECC == ptErrHEntry->bsErrCode || NF_ERR_TYPE_DCRC == ptErrHEntry->bsErrCode);
    ASSERT(TRUE != ptErrHEntry->bsEmptyPg);

    if (TRUE == bRcvFail)
    {
        #if (defined(FPGA) || defined(ASIC))
        // L3 report the error to L0 and wait for SMsg Empty, then Redo the fcmd without uecc
        L3_NvmeReportSMSG(ptCtrlEntry->ptReqEntry->tHostDesc.tHostDptr.bsCmdTag, ptCtrlEntry->ptReqEntry->bsTLun);
        (void)L3_NVMeHandleUeccRspMarker(ptCtrlEntry->ptReqEntry->bsTLun);
        #endif

        // ignore the error. nvme: redo it without ecc check; sata: update bufferMap directly.
        ptCtrlEntry->bsBypassEcc = TRUE;
        while (TRUE != L3_ErrHRead(ptCtrlEntry));
        ASSERT(FALSE == ptErrHEntry->bsErrHold);
        while (TRUE != L3_IFIsFCmdFinished(ptCtrlEntry));
    }
    else
    {
        L3_HandleHostReadSuccess(ptCtrlEntry);
    }

    return;
}

/*==============================================================================
Func Name  : L3_HandleHostReadSuccess
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
BOOL bRcvFail
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2017.03.01 NickLiou create function
==============================================================================*/
LOCAL void L3_HandleHostReadSuccess(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U16 usCurHsgId, usNextHsgId;
    U32 ulRemainSecLen;

    L3_RstDatTxCtrler(ptCtrlEntry);
    ptCtrlEntry->tDTxCtrl.bsRemSecLen = ptCtrlEntry->ptReqEntry->tFlashDesc.bsSecLen << SEC_SIZE_BITS;

    // Build HSG
    if (INVALID_4F == ptCtrlEntry->tDTxCtrl.bsFstEngineID)
    {
        while (FALSE == HAL_GetHsg(&usCurHsgId));

        ptCtrlEntry->tDTxCtrl.bsFstEngineID = usCurHsgId;
        ptCtrlEntry->tDTxCtrl.bsCurEngineID = usCurHsgId;
    }

    while (0 < ptCtrlEntry->tDTxCtrl.bsRemSecLen)
    {
        if (FALSE == HAL_CheckForLastHsg(&ptCtrlEntry->ptReqEntry->tHostDesc.tHostDptr, ptCtrlEntry->tDTxCtrl.bsRemSecLen))
        {
            while (FALSE == HAL_GetHsg(&usNextHsgId));
        }
        else
        {
            usNextHsgId = INVALID_4F;
        }

        ulRemainSecLen = ptCtrlEntry->tDTxCtrl.bsRemSecLen;
        HAL_BuildHsg(&ptCtrlEntry->ptReqEntry->tHostDesc.tHostDptr, &ulRemainSecLen, ptCtrlEntry->tDTxCtrl.bsCurEngineID, usNextHsgId);
        ptCtrlEntry->tDTxCtrl.bsRemSecLen = ulRemainSecLen;
        HAL_SetHsgSts(ptCtrlEntry->tDTxCtrl.bsCurEngineID, TRUE);

        if (INVALID_4F != usNextHsgId)
        {
            ptCtrlEntry->tDTxCtrl.bsCurEngineID = usNextHsgId;
        }
        else
        {
            ptCtrlEntry->tDTxCtrl.aSecAddr[0].bsSecStart = ptCtrlEntry->ptReqEntry->tFlashDesc.bsSecStart;
            ptCtrlEntry->tDTxCtrl.aSecAddr[0].bsSecLength = ptCtrlEntry->ptReqEntry->tFlashDesc.bsSecLen;
            ptCtrlEntry->tDTxCtrl.bsDmaTotalLen = ptCtrlEntry->ptReqEntry->tFlashDesc.bsSecLen;
            ptCtrlEntry->tDTxCtrl.bsBdEngineDone = TRUE;

            break;
        }
    }

    // Build DSG
    U16 usDsgId;
    NORMAL_DSG_ENTRY *pDsg;

    while (FALSE == HAL_GetNormalDsg(&usDsgId));

    pDsg = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usDsgId);
    COM_MemZero((U32 *)pDsg, sizeof(NORMAL_DSG_ENTRY) >> DWORD_SIZE_BITS);
    pDsg->bsLast = TRUE;
    pDsg->bsDramAddr = HAL_NfcGetDmaAddr(L3_BufMgrGetBufIDByNode(ptCtrlEntry->ptErrHEntry->tUeccHCtrl.ptBufNodePtr), ptCtrlEntry->ptReqEntry->atBufDesc[0].bsSecStart, BUF_SIZE_BITS);
    pDsg->bsXferByteLen = ptCtrlEntry->ptReqEntry->atBufDesc[0].bsSecLen << SEC_SIZE_BITS;
    HAL_SetNormalDsgSts(usDsgId, NORMAL_DSG_VALID);

    // Build DRQ
    while (TRUE == HAL_DrqIsFull());

    HAL_DrqBuildEntry(ptCtrlEntry->ptReqEntry->tHostDesc.tHostDptr.bsCmdTag,
        ptCtrlEntry->tDTxCtrl.bsFstEngineID,
        usDsgId);

    while (FALSE == HAL_DrqIsEmpty());

    return;
}

/*====================End of this file========================================*/

