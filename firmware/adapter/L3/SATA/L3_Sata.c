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
* File Name    : L3_SATA.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "FW_SMSG.h"
#include "COM_BitMask.h"
#include "L3_FCMDQ.h"
#include "L3_ErrHBasic.h"
#include "L3_Interface.h"
#include "L3_SATA.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
//specify file name for Trace Log
#define TL_FILE_NUM  L3_Sata_c

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
Func Name  : L3_UpdateBufferMap
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
LOCAL void L3_UpdateBufferMap(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8  ucStartLPNOff;
    U8  ucEndLPNOff;
    U32 ulBufMapValue;
    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = ptCtrlEntry->ptReqEntry;
    ucStartLPNOff = (ptReqEntry->atBufDesc[0].bsSecStart >> SEC_PER_LPN_BITS);
    ucEndLPNOff   = (ptReqEntry->atBufDesc[0].bsSecStart + ptReqEntry->atBufDesc[0].bsSecLen - 1) >> SEC_PER_LPN_BITS;

    ulBufMapValue = COM_GETBITMASK(ucStartLPNOff, ucEndLPNOff);
    HAL_UpdateBufMapValue(ptReqEntry->tHostDesc.tHostDptr.bsPrdID, ulBufMapValue, ~ulBufMapValue);

    return;
}

/*==============================================================================
Func Name  : L3_UpdateReadInfo
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
LOCAL void L3_UpdateReadInfo(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucStartPos;
    U8 ucStartLPNOff, ucEndLPNOff, ucLeftLpnNum;
    U16 usDecSecNum;
    volatile U32 ulBufMapValue;
    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = ptCtrlEntry->ptReqEntry;
    ucStartLPNOff = (ptReqEntry->atBufDesc[0].bsSecStart >> SEC_PER_LPN_BITS);
    ucEndLPNOff   = (ptReqEntry->atBufDesc[0].bsSecStart + ptReqEntry->atBufDesc[0].bsSecLen - 1) >> SEC_PER_LPN_BITS;
    ulBufMapValue = HAL_GetBufMapValue(ptReqEntry->tHostDesc.tHostDptr.bsPrdID);

    ucStartPos = ucStartLPNOff;
    while (0 != (ulBufMapValue & (1<<ucStartPos)))
    {
        ucStartPos++;
        if (ucStartPos > ucEndLPNOff)
        {
            DBG_Printf("FCmdAddr:0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    ucLeftLpnNum = ucEndLPNOff - ucStartPos + 1;
    REG_SET_VALUE((*(volatile U32*)&ulBufMapValue), ucLeftLpnNum, 0, ucStartPos);
    HAL_UpdateBufMapValue(ptReqEntry->tHostDesc.tHostDptr.bsPrdID, ulBufMapValue, ~COM_GETBITMASK(ucStartLPNOff, ucEndLPNOff));

    usDecSecNum = ucStartPos * SEC_PER_LPN - ptReqEntry->atBufDesc[0].bsSecStart;
    ptReqEntry->tFlashDesc.bsSecStart += usDecSecNum;
    ptReqEntry->tFlashDesc.bsSecLen -= usDecSecNum;
    ptReqEntry->atBufDesc[0].bsSecStart += usDecSecNum;
    ptReqEntry->atBufDesc[0].bsSecLen -= usDecSecNum;

    return;

}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_AllocResource
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
BOOL L3_AllocResource(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    return L3_IFAllocDSG(ptCtrlEntry);
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
        ptNfcqEntry->bsBmEn = TRUE;

        if (TRUE == ptReqEntry->tHostDesc.tHostDptr.bsFstCmdEn)
        {
            ptNfcqEntry->bsNcqMd = 0;
            ptNfcqEntry->bsFstDataRdy = TRUE;

            // NcqNum - sata host read/per-fetch read only for first 4k data ready.
            ptNfcqEntry->bsNcqNum = ptReqEntry->tHostDesc.tHostDptr.bsCmdTag;
        }
    }

    if (TRUE == ptCtrlEntry->tDTxCtrl.bsDSGEn)
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
void L3_RstDatTxCtrler(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_DATA_TX_CTRL *ptDsgCtrl;
    FCMD_REQ_ENTRY *ptReqEntry;

    ptDsgCtrl = &ptCtrlEntry->tDTxCtrl;
    ptDsgCtrl->bsBdEngineDone = FALSE;
    ptDsgCtrl->bsFstEngineID = INVALID_4F;
    ptDsgCtrl->bsLpnBitMapPos = 0;
    ptDsgCtrl->bsSecAddrIndex = 0;
    ptDsgCtrl->bsDmaTotalLen = 0;

    ptReqEntry = ptCtrlEntry->ptReqEntry;
    ptReqEntry->tHostDesc.tHostDptr.bsFstCmdEn = FALSE;

    if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        L3_UpdateReadInfo(ptCtrlEntry);
    }

    return;
}

/*==============================================================================
Func Name  : L3_SataHandleUeccRspMarker
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
void L3_SataHandleUeccRspMarker(void)
{
    while (TRUE != FW_IsSMQEmpty());
    return;
}

/*==============================================================================
Func Name  : L3_SataReportSMSG
Input      : U8 ucCmdTag
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.28 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_SataReportSMSG(U8 ucCmdTag)
{
    SMSG tSMSG;

    tSMSG.ulMsgCode     = SMSG_SUBSYS_ERROR;
    tSMSG.ulMsgParam[0] = SUBSYS_ERROR_UECC;
    tSMSG.ulMsgParam[1] = ucCmdTag;
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

    L3_UpdateBufferMap(ptCtrlEntry);

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
        // L3 report the error to L0 and wait for SMsg Empty, then update bufferMap.
        L3_SataReportSMSG(ptCtrlEntry->ptReqEntry->tHostDesc.tHostDptr.bsCmdTag);
        L3_SataHandleUeccRspMarker();
        #endif
    }
    else
    {
        #ifdef XOR_ENABLE
        L3_XorPostSuccessRecover(ptCtrlEntry);
        #endif
    }

    // ignore the error. ahci: redo it without ecc check; sata: update bufferMap directly.
    L3_UpdateBufferMap(ptCtrlEntry);

    return;
}

void L3_SGEReset(void)
{
    #ifndef SIM
    HAL_NormalDsgReset();
    #endif

    return;
}

/*====================End of this file========================================*/

