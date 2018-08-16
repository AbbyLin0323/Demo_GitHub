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
* File Name    : L3_ErrHBasic.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "Disk_Config.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_DecStsReport.h"
#include "L2_Interface.h"
#include "L2_TableBBT.h"
#include "L3_Interface.h"
#include "L3_Schedule.h"
#include "L3_SoftDecoder.h"
#include "L3_DataRecover.h"
#include "L3_FlashMonitor.h"
#include "L3_Debug.h"
#include "L3_ErrHExtend.h"
#include "L3_ErrHBasic.h"
#include "HAL_MultiCore.h"
#include "HAL_FlashDriverExt.h"
#include "L2_ErrorHandling.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
extern GLOBAL BOOL gbGlobalInfoSaveFlag;
/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL U8 g_aBrthLunMap[SUBSYSTEM_LUN_MAX][LUN_PER_CE] = {0};
#ifdef READ_RETRY_REFACTOR
LOCAL ADAPTIVE_RETRY_TABLE *g_aAdaptiveRetryTable;
#endif

#ifdef N1_SCAN
/* Vt Scan by N1, shift Vt range of all read level in BiCS3 */
LOCAL N1_SHIFT_RANGE l_tL3N1ShiftRange[READ_LEVEL_CNT] =
{
    {0x80 , 0x2B, 0xA5, INVALID_2F},  //rL1_3bpc, FA = A5h
    {0xBE , 0x27, 0xA6, INVALID_2F},  //rL2_3bpc, FA = A6h
    {0xC2 , 0x2B, 0xA7, INVALID_2F},  //rL3_3bpc
    {0xC6 , 0x1D, 0xA8, INVALID_2F},  //rL4_3bpc
    {0xCA , 0x1F, 0xA9, INVALID_2F},  //rL5_3bpc
    {0xC8 , 0x1D, 0xAA, INVALID_2F},  //rL6_3bpc
    {0xC2 , 0x7F, 0xAB, INVALID_2F}   //rL7_3bpc
};
#endif

/*==============================================================================
Func Name  : L3_ErrHBuildBrthLunMap
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHBuildBrthLunMap(void)
{
    U8 ucPU, ucCEInPU, ucLunInCE, ucBrthLunInCE;
    U8 ucTLun, ucBrthLun, ucTBrthLun;

    if (1 == LUN_PER_CE)
    {
        return;
    }

    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        for (ucCEInPU = 0; ucCEInPU < NFC_CE_PER_PU; ucCEInPU++)
        {
            for (ucLunInCE = 0; ucLunInCE < LUN_PER_CE; ucLunInCE++)
            {
                ucTLun = ((ucPU * NFC_CE_PER_PU) + ucCEInPU) + ucLunInCE * SUBSYSTEM_PU_NUM;

                ucBrthLun = 0;
                for (ucBrthLunInCE = 0; ucBrthLunInCE < LUN_PER_CE; ucBrthLunInCE++)
                {
                    ucTBrthLun = ((ucPU * NFC_CE_PER_PU) + ucCEInPU) + ucBrthLunInCE * SUBSYSTEM_PU_NUM;

                    if (ucTLun != ucTBrthLun)
                    {
                        g_aBrthLunMap[ucTLun][ucBrthLun] = ucTBrthLun;
                        ucBrthLun++;
                    }
                }
                g_aBrthLunMap[ucTLun][ucBrthLun] = INVALID_2F;
            }
        }
    }

    for (; ucPU < SUBSYSTEM_PU_MAX; ucPU++)
    {
        for (ucCEInPU = 0; ucCEInPU < NFC_CE_PER_PU; ucCEInPU++)
        {
            for (ucLunInCE = 0; ucLunInCE < LUN_PER_CE; ucLunInCE++)
            {
                ucTLun = ((ucPU * NFC_CE_PER_PU) + ucCEInPU) * LUN_PER_CE + ucLunInCE;
                for (ucBrthLun = 0; ucBrthLun < LUN_PER_CE; ucBrthLun++)
                {
                    g_aBrthLunMap[ucTLun][ucBrthLun] = INVALID_2F;
                }
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHGetTBrthLun
Input      : U8 ucTLun
             U8 ucBrthLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
LOCAL U8 MCU2_DRAM_TEXT L3_ErrHGetTBrthLun(U8 ucTLun, U8 ucBrthLun)
{
    return g_aBrthLunMap[ucTLun][ucBrthLun];
}

/*==============================================================================
Func Name  : L3_ErrHSetBrthLunLockBmp
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHSetBrthLunLockBmp(U8 ucTLun)
{
    U8 ucIndex, ucTBrthLun;
    U8 ucTBrthLunNum = LUN_PER_CE - 1;

    for (ucIndex = 0; ucIndex < ucTBrthLunNum; ucIndex++)
    {
        ucTBrthLun = L3_ErrHGetTBrthLun(ucTLun, ucIndex);
        L3_SchSetStsBit(ucTBrthLun, STS_BMP_LOCK);
        L3_SchClrArbBits(ucTBrthLun);
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHClrBrthLunLockBmp
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHClrBrthLunLockBmp(U8 ucTLun)
{
    U8 ucIndex, ucTBrthLun;
    U8 ucTBrthLunNum = LUN_PER_CE - 1;

    for (ucIndex = 0; ucIndex < ucTBrthLunNum; ucIndex++)
    {
        ucTBrthLun = L3_ErrHGetTBrthLun(ucTLun, ucIndex);
        L3_SchClrStsBit(ucTBrthLun, STS_BMP_LOCK);
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHFailLog
Input      : U8 ucTLun
             U8 ucErrCode
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHFailLog(U8 ucTLun, U8 ucErrCode)
{
    switch (ucErrCode)
    {
        case NF_ERR_TYPE_ERS:
        {
            g_pSubSystemDevParamPage->EraseFailCnt++;
            gbGlobalInfoSaveFlag = TRUE;
            break;
        }
        case NF_ERR_TYPE_PRG:
        case NF_ERR_TYPE_PREPRG:
        case NF_ERR_TYPE_BOTHPRG:
        {
            g_pSubSystemDevParamPage->ProgramFailCnt++;
            gbGlobalInfoSaveFlag = TRUE;
            break;
        }
        case NF_ERR_TYPE_RECC:
        {
            break;
        }
        case NF_ERR_TYPE_UECC:
        case NF_ERR_TYPE_DCRC:
        {
            g_pSubSystemDevParamPage->SYSUECCCnt++;
            gbGlobalInfoSaveFlag = TRUE;
            break;
        }
        case NF_ERR_TYPE_NODEV:
        {
            break;
        }
        default:
        {
            DBG_Printf("TLun%d FailLog ErrCode=%d Err.\n", ucTLun, ucErrCode);
            DBG_Getch();
        }
    }

    L3_FMUpdtUsrFailCnt(ucTLun, ucErrCode);

    L3_FMSetPhyBlk(ucTLun, INVALID_4F);
    L3_FMSetPhyPage(ucTLun, INVALID_4F);
    L3_FMSetPlnNum(ucTLun, MSK_F);
    L3_FMSetCmdType(ucTLun, INVALID_2F);

    return;
}

/*==============================================================================
Func Name  : L3_ErrHDetectEmptyPage
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ErrHDetectEmptyPage(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucPU, ucLunInPU, ucLevel, ucPln, ucStartPln, ucEndPln, ucCWNum;
    U32 ulCwFailBitmap;
    BOOL bIsEmptyPage = TRUE;
    DEC_SRAM_STATUS_ENTRY *ptDecSramSts;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    BOOL bIsFullRecovery;//nick
    BOOL bSingleReq = FALSE;

    // no need to check the empty page status when the retry started.
    if (TRUE == ptErrHEntry->tUeccHCtrl.tRetry.bsEnable) return FALSE;
    if (NF_ERR_TYPE_UECC != ptErrHEntry->bsErrCode) return FALSE;

    ucPU = L3_GET_PU(ptReqEntry->bsTLun);
    ucLunInPU = L3_GET_LUN_IN_PU(ptReqEntry->bsTLun);
    ucLevel = ptCtrlEntry->bsNfcqPtr;

    if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType
     || FCMD_REQ_SUBTYPE_SINGLE_ONEPG == ptReqEntry->bsReqSubType
     || FCMD_REQ_SUBTYPE_SINGLE_TWOPG == ptReqEntry->bsReqSubType)
    {
        ucStartPln = ucEndPln = ptReqEntry->tFlashDesc.bsPlnNum;
        bSingleReq = TRUE;
    }
    else
    {
        ucStartPln = (TRUE == ptReqEntry->tFlashDesc.bsRdRedOnly) ? 0 : ptReqEntry->tFlashDesc.bsSecStart / SEC_PER_LOGIC_PG;
        if (TRUE == ptReqEntry->tFlashDesc.bsMergeRdEn)
        {
            ucEndPln = ptReqEntry->tFlashDesc.bsSecStart / SEC_PER_LOGIC_PG;
        }
        else
        {
            ucEndPln = (TRUE == ptReqEntry->tFlashDesc.bsRdRedOnly) ? (PLN_PER_LUN - 1) : (ptReqEntry->tFlashDesc.bsSecStart + ptReqEntry->tFlashDesc.bsSecLen - 1) / SEC_PER_LOGIC_PG;
        }

    }

    bIsFullRecovery = ptReqEntry->bsTBRebuilding;//nick
    for (ucPln = ucStartPln; ucPln <= ucEndPln; ucPln++)
    {
        if(FALSE == bSingleReq)
        {
            ptDecSramSts = (DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLunInPU][ucLevel][ucPln]);
        }
        else
        {
            /* single plane request always get dec status from index 0 */
            ptDecSramSts = (DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLunInPU][ucLevel][0]);
        }

        if (TRUE == ptReqEntry->tFlashDesc.bsRdRedOnly)
        {
            //ucCWNum = 1;
            //if any palne is not empty page, we should to indicate it as a real uecc.
            if (ptReqEntry->tFlashDesc.bsVirPage == 0)
            {
                if ((U32)ptDecSramSts->bsN1Accu < EMPTY_THRD_LAST_1K)
                {
                    bIsEmptyPage = FALSE;
                    DBG_Printf("TLun%d LAST_1K bsN1Accu(%d)\n", ptReqEntry->bsTLun, ptDecSramSts->bsN1Accu);
                    break;
                }
            }
            else
            {
                if ((U32)ptDecSramSts->bsN1Accu < EMPTY_THRD_LAST_1K_OTHER_PG)
                {
                    bIsEmptyPage = FALSE;
                    DBG_Printf("TLun%d LAST_1K bsN1Accu(%d)\n", ptReqEntry->bsTLun, ptDecSramSts->bsN1Accu);
                    break;
                }
            }
        }
        else
        {
            ucCWNum = 0;
            ulCwFailBitmap = ptDecSramSts->ulDecFailBitMap;
            if (ulCwFailBitmap == 0)
            {
                continue;
            }
            while (0 != ulCwFailBitmap)
            {
                if (0 != (ulCwFailBitmap & 1))
                {
                    ucCWNum++;
                    if (8 <= ucCWNum)
                    {
                        break;
                    }
                }
                ulCwFailBitmap >>= 1;
            }

            // if any palne is not empty page, we should to indicate it is as a real uecc.
            if (ptReqEntry->tFlashDesc.bsVirPage == 0)
            {                
                if ((U32)ptDecSramSts->bsN1Accu < (U32)ucCWNum * EMPTY_THRD_FST_15K)
                {
                    bIsEmptyPage = FALSE;
                    DBG_Printf("TLun%d page0 pln%d_%d bsN1Accu(%d)/ucCWNum(%d)->bsN1Accu(%d)\n", ptReqEntry->bsTLun, ucPln, ucEndPln, ptDecSramSts->bsN1Accu, ucCWNum, (ptDecSramSts->bsN1Accu/ucCWNum));
#ifndef EMPTY_PAGE_INFO
                    break;
#else
                }
                else if (ucCWNum != 0 && ptReqEntry->bsBootUpOk)
                {
#ifdef SCAN_BLOCK_N1
                    if (ptDecSramSts->bsN1Accu < (U32)ucCWNum * EMPTY_THRD_FST_8K)
#endif
                        DBG_Printf("[Empty] TLun%d page0 pln%d_%d bsN1Accu(%d)/ucCWNum(%d)->bsN1Accu(%d)\n", ptReqEntry->bsTLun, ucPln, ucEndPln, ptDecSramSts->bsN1Accu, ucCWNum, (ptDecSramSts->bsN1Accu/ucCWNum));
#endif
                }
           	}
            else
            {
                if ((U32)ptDecSramSts->bsN1Accu < (U32)ucCWNum * EMPTY_THRD_FST_15K_OTHER_PG)
                {
                    bIsEmptyPage = FALSE;
                    DBG_Printf("TLun%d page%d pln%d_%d bsN1Accu(%d)/ucCWNum(%d)->bsN1Accu(%d)\n", ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsVirPage, ucPln, ucEndPln, ptDecSramSts->bsN1Accu, ucCWNum, (ptDecSramSts->bsN1Accu/ucCWNum));
                    break;
#ifndef EMPTY_PAGE_INFO
                    break;
#else
                }
                else if (ucCWNum != 0 && ptReqEntry->bsBootUpOk)
                {
#ifdef SCAN_BLOCK_N1
                    if (ptDecSramSts->bsN1Accu < (U32)ucCWNum * EMPTY_THRD_FST_8K)
#endif
                        DBG_Printf("[Empty] TLun%d page%d pln%d_%d bsN1Accu(%d)/ucCWNum(%d)->bsN1Accu(%d)\n", ptReqEntry->bsTLun, ptReqEntry->tFlashDesc.bsVirPage, ucPln, ucEndPln, ptDecSramSts->bsN1Accu, ucCWNum, (ptDecSramSts->bsN1Accu/ucCWNum));
#endif
                }
            }
        }

    }

    return bIsEmptyPage;
}

/*==============================================================================
Func Name  : L3_ErrHDetectErrCode
Input      : U8 ucTLun
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      : Retry meets emptypage, getch in debug mode; change to uecc in
release mode.
History    :
    1. 2016.7.22 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHDetectErrCode(U8 ucTLun, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucErrCode;

    ucErrCode = HAL_NfcGetErrCode(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun));
    ASSERT(NF_SUCCESS != ucErrCode);

    if (TRUE == ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tRetry.bsEnable)
    {
        if (TRUE == L3_ErrHDetectEmptyPage(ptCtrlEntry))
        {
            ucErrCode  = NF_ERR_TYPE_UECC;
            DBG_Printf("TLun%d RetryMeetsEmptyPage.0x%x\n", ucTLun, (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    ptCtrlEntry->ptErrHEntry->bsErrCode = ucErrCode;

    return;
}

/*==============================================================================
Func Name  : L3_ErrHPretreat
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.22 JasonGuo create function
==============================================================================*/
LOCAL FCMD_INTR_CTRL_ENTRY MCU2_DRAM_TEXT *L3_ErrHPretreat(U8 ucTLun)
{
    U8 ucCurEptr, ucCurPptr;
    NFCQ_ENTRY *ptNfcqEntry;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry;

    if (TRUE == HAL_NfcGetErrHold(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun)))
    {
        // pending for pre-page program fail getting in hal nfc driver. ***
        ptNfcqEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr_RP(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun));
        ucCurEptr = ptNfcqEntry->atRowAddr[0].bsFCmdPtr;
        L3_FCMDQSetIntrEptr(ucTLun, ucCurEptr, FALSE);

        // pending fcmd
        ucCurPptr = (ucCurEptr + 1) % NFCQ_DEPTH;
        ucCurPptr = (ucCurPptr == L3_FCMDQGetIntrWptr(ucTLun, FALSE)) ? INVALID_DPTR : ucCurPptr;
        L3_FCMDQSetIntrPptr(ucTLun, ucCurPptr, FALSE);
#ifdef TRI_STAGE_COPY
        if (INVALID_DPTR != ucCurPptr)
        {
            ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucCurPptr, FALSE);
            if (0 < ptCtrlEntry->bsMultiStep)
            {
                ptCtrlEntry->bsMultiStep = TRI_STAGE_NUM;
            }
        }
#endif

        // allcate a errh-entry for the current lun.
        ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucCurEptr, FALSE);
        if (NULL == ptCtrlEntry->ptErrHEntry)
        {
            ptErrHEntry = L3_FCMDQAllocIntrErrHEntry(ucTLun, FALSE);
            ptCtrlEntry->ptErrHEntry = ptErrHEntry;
        }

        ptCtrlEntry->ptErrHEntry->bsErrHold = TRUE;

        L3_ErrHDetectErrCode(ucTLun, ptCtrlEntry);
    }
    else
    {
        ucCurEptr = L3_FCMDQGetIntrEptr(ucTLun, FALSE);
        ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucCurEptr, FALSE);

        ASSERT(NULL != ptCtrlEntry->ptErrHEntry);
        ptCtrlEntry->ptErrHEntry->bsErrHold = FALSE;
    }

    return ptCtrlEntry;
}


/*==============================================================================
Func Name  : L3_ErrHIsPreDone
Input      : U8 ucTLun
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ErrHIsPreDone(U8 ucTLun, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bNfcErr, bNfcEmpty, bDone = TRUE;
    U8 ucLunInPU, ucIndex, ucTBrthLun;
    U8 ucTBrthLunNum = LUN_PER_CE - 1;

    for (ucLunInPU = 0; ucLunInPU < NFC_LUN_PER_PU; ucLunInPU++)
    {
        L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_ERROR);
        L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_EMPTY);
    }

    for (ucIndex = 0; ucIndex < ucTBrthLunNum; ucIndex++)
    {
        ucTBrthLun = L3_ErrHGetTBrthLun(ucTLun, ucIndex);
        bNfcErr = L3_SchGetStsBit(ucTBrthLun, STS_BMP_NFC_ERROR);
        bNfcEmpty = L3_SchGetStsBit(ucTBrthLun, STS_BMP_NFC_EMPTY);
        if ((TRUE != bNfcErr) && (TRUE != bNfcEmpty))
        {
            bDone = FALSE;
            break;
        }
    }

    return bDone;
}

/*==============================================================================
Func Name  : L3_ErrHAssertCmd
Input      : U8 ucFCmdType
             U8 ucErrCode
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHAssertCmd(U8 ucFCmdType, U8 ucErrCode)
{
    switch (ucErrCode)
    {
        case NF_ERR_TYPE_UECC:
        case NF_ERR_TYPE_DCRC:
        case NF_ERR_TYPE_LBA:
        case NF_ERR_TYPE_RECC:
        {
            if (FCMD_REQ_TYPE_READ != ucFCmdType)
            {
                DBG_Printf("ErrCode=%d FCmdType=%d Read-Mis!\n", ucErrCode, ucFCmdType);
                DBG_Getch();
            }
            break;
        }
        case NF_ERR_TYPE_PRG:
        case NF_ERR_TYPE_PREPRG:
        case NF_ERR_TYPE_BOTHPRG:
        {
            if (FCMD_REQ_TYPE_WRITE != ucFCmdType)
            {
                DBG_Printf("ErrCode=%d FCmdType=%d Write-Mis!\n", ucErrCode, ucFCmdType);
                DBG_Getch();
            }
            break;
        }
        case NF_ERR_TYPE_ERS:
        {
            if (FCMD_REQ_TYPE_ERASE != ucFCmdType)
            {
                DBG_Printf("ErrCode=%d FCmdType=%d Erase-Mis!\n", ucErrCode, ucFCmdType);
                DBG_Getch();
            }
            break;
        }
        default:
        {
            DBG_Printf("L3_ErrHAssertCmd ErrCode=%d Err.\n", ucErrCode);
            DBG_Getch();
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHIsNeedExtH
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ErrHIsNeedExtH(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bNeedExtH;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    if ((FCMD_REQ_TYPE_ERASE == ptReqEntry->bsReqType) || (FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType))
    {
        bNeedExtH = ptReqEntry->bsNeedL3ErrHandle;
        return bNeedExtH;
    }

    if ((TRUE == ptReqEntry->bsTableReq)
        || (TRUE == ptCtrlEntry->bsIntrReq)
        || (TRUE == ptReqEntry->tLocalDesc.bsRdtCmd && TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
        || (TRUE == ptReqEntry->tLocalDesc.bsPatrolRdCmd && TRUE != ptReqEntry->tFlashDesc.bsHostRdEn))
    {
        bNeedExtH = FALSE;
    }
#if 0
    else if (FCMD_REQ_TYPE_ERASE == ptReqEntry->bsReqType)
    {
        bNeedExtH = ptReqEntry->bsNeedL3ErrHandle;
    }
#endif
    // L2 needs to handle the read request which needs checking redundant info.
    else if ((FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType)
          && (0 != ptReqEntry->ulSpareAddr))
    {
        bNeedExtH = FALSE;
    }
    // L2 needs to handle tlc-read-fail; including tlc-read-rpmt page fail.
    else if ((FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
          && (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType))
    {
        bNeedExtH = FALSE;
    }
    // L2 needs to handle slc-read-rpmt fail.
    else if ((FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
          && (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType)
          && (PG_PER_SLC_BLK-1 == ptReqEntry->tFlashDesc.bsVirPage))
    {
        bNeedExtH = FALSE;
    }
    else
    {
        bNeedExtH = TRUE;
    }

    return bNeedExtH;
}

/*==============================================================================
Func Name  : L3_ErrHRstCmdQ
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHRstCmdQ(U8 ucTLun)
{
    U8 ucPu = L3_GET_PU(ucTLun);
    U8 ucLunInPu = L3_GET_LUN_IN_PU(ucTLun);

    // Reset CQ
    HAL_NfcResetCmdQue(ucPu, ucLunInPu);
    HAL_NfcClearINTSts(ucPu, ucLunInPu);

    return;
}

/*==============================================================================
Func Name  : L3_ErrHRstLun
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2  JasonGuo create function
    2. 2016.9.29 JasonGuo add SLCMode to support multi-work-mode flash chip.
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHRstLun(U8 ucTLun, BOOL bSLCMode)
{
    BOOL bStatus;
    FLASH_ADDR tFlashAddr = { 0 };

    tFlashAddr.ucPU = L3_GET_PU(ucTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ucTLun);
    tFlashAddr.bsSLCMode = bSLCMode;

    bStatus = HAL_NfcResetLun(&tFlashAddr);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d Reset Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d Reset Wait Fail.\n", ucTLun);
        DBG_Getch();
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHRstCacheOperation
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
    2. 2016.9.29 JasonGuo add SLCMode to support multi-work-mode flash chip
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHRstCacheOperation(U8 ucTLun, BOOL bSLCMode)
{
#ifdef FLASH_CACHE_OPERATION
    L3_ErrHRstLun(ucTLun, bSLCMode);
#endif
    return;
}

/*==============================================================================
Func Name  : L3_ErrHPreCondition
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHPreCondition(U8 ucTLun)
{
    BOOL bStatus;
    FLASH_ADDR tFlashAddr;

    tFlashAddr.ucPU = L3_GET_PU(ucTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ucTLun);

    bStatus = HAL_FlashRetryPreConditon(&tFlashAddr);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d PreCondition Send Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d PreCondition Wait Fail.\n", ucTLun);
        DBG_Getch();
    }

    return;
}
#ifdef READ_RETRY_REFACTOR
/*==============================================================================
Func Name  : L3_ErrHGetRetryVtIndex
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
1. 2017.7.17 JerryNie create function
==============================================================================*/
U8 MCU2_DRAM_TEXT L3_ErrHGetRetryVtIndex(U8 ucTLun, U8 ucIndex, BOOL bTlcMode, U16 usWLType, U8 ucPageType)
{
    U8 ucRetryVtIndex;
    
    if (TRUE == bTlcMode)
    {
        if (SLC_TYPE == usWLType)
        {
            ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aSlcLpTable[ucIndex];
        }
        else if (MLC_TYPE == usWLType)
        {
            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
            {
                ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aMlcLpTable[ucIndex];
            }
            else if (HIGH_PAGE == ucPageType)
            {
                ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aMlcUpTable[ucIndex];
            }
            else
            {
                DBG_Printf("MLC WL page type error!\n");
                DBG_Getch();
            }
        }
        else if (TLC_TYPE == usWLType)
        {
            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
            {
                ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aTlcLpTable[ucIndex];
            }
            else if (HIGH_PAGE == ucPageType)
            {
                ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aTlcUpTable[ucIndex];
            }
            else if (EXTRA_PAGE == ucPageType)
            {
                ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aTlcXpTable[ucIndex];
            }
            else
            {
                DBG_Printf("TLC WL page type error!\n");
                DBG_Getch();
            }
        }
        else
        {
            DBG_Printf("WL type error!\n");
            DBG_Getch();
        }
    }
    else
    {
        ucRetryVtIndex = g_aAdaptiveRetryTable[ucTLun].aSSlcLpTable[ucIndex];
    }

    return ucRetryVtIndex;
}

/*==============================================================================
Func Name  : L3_ErrHUpdateAdaptiveRetryTable
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription: Set current Vt to Vector[0], and the other Vt move down sequentially.
Usage      :
History    :
    1. 2017.8.7 JerryNie create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHUpdatAdaptiveRetryTable(U8 ucTLun, U8 ucIndex, BOOL bTLCMode, U16 usWLType, U8 ucPageType)
{
    U8 i, TempVtIdx;
    U8 *aModTable;          // The relevant to-be-modified Retry Table 

    if (TRUE == bTLCMode)
    {
        if (SLC_TYPE == usWLType)
        {
            aModTable = g_aAdaptiveRetryTable[ucTLun].aSlcLpTable;
        }
        else if (MLC_TYPE == usWLType)
        {
            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
            {
                aModTable = g_aAdaptiveRetryTable[ucTLun].aMlcLpTable;
            }
            else if (HIGH_PAGE == ucPageType)
            {
                aModTable = g_aAdaptiveRetryTable[ucTLun].aMlcUpTable;
            }
            else
            {
                DBG_Printf("MLC WL page type error!\n");
                DBG_Getch();
            }
        }
        else if (TLC_TYPE == usWLType)
        {
            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
            {
                aModTable = g_aAdaptiveRetryTable[ucTLun].aTlcLpTable;
            }
            else if (HIGH_PAGE == ucPageType)
            {
                aModTable = g_aAdaptiveRetryTable[ucTLun].aTlcUpTable;
            }
            else if (EXTRA_PAGE == ucPageType)
            {
                aModTable = g_aAdaptiveRetryTable[ucTLun].aTlcXpTable;
            }
            else
            {
                DBG_Printf("TLC WL page type error!\n");
                DBG_Getch();
            }
        }
        else
        {
            DBG_Printf("WL type error!\n");
            DBG_Getch();
        }
    }
    else
    {
        aModTable = g_aAdaptiveRetryTable[ucTLun].aSSlcLpTable;
    }

    /* Exchange current element to the first one */
    TempVtIdx = aModTable[ucIndex];
    for (i = ucIndex; i > 0; i--)
    {
        aModTable[i] = aModTable[i - 1];
    }
    aModTable[0] = TempVtIdx;

    return;
}

/*------------------------------------------------------------------------------
Name: L3_ErrHIsHomemadeReadRetryEntry
Description:
    Whether current read retry Vt entry is homemade
Input Param:
    U8 ucTLun
    U8 ucVthShift
    BOOL bTlcMode
    U16 usWLType
    U8 ucPageType
Output Param:
    none
Return Value:
    BOOL
Usage:
    called when FW need confirm the source of current Vth entry
History:
    20170828    JerryNie   create
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT L3_ErrHIsHomemadeReadRetryEntry(U8 ucTLun, U8 ucVthShift, BOOL bTlcMode, U16 usWLType, U8 ucPageType)
{
    U8 ucVtIndex;

    ucVtIndex = L3_ErrHGetRetryVtIndex(ucTLun, ucVthShift, bTlcMode, usWLType, ucPageType);

    /* First read retry entry in front, homemade behind */
    if (ucVtIndex < RETRY_CNT)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*==============================================================================
Func Name  : L3_ErrHSetNewParameter
Input      : U8 ucTLun
             U8 ucVthShift
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2017.8.7 Jerry create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHSetNewParameter(U8 ucTLun, U8 ucVthShift, BOOL bTLCMode, U8 ucWlType, U8 ucPageType)
{
    BOOL bStatus;
    RETRY_TABLE tRetryPara;
    FLASH_ADDR tFlashAddr;
    U8 ucLevel;

    tFlashAddr.ucPU = L3_GET_PU(ucTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ucTLun);

    //Each LUN & LP/MP/HP have it's own Vt table, which is the refactor of orignal retry parameter order
    // ucVtIndex refers to the retry entry ranking in the global/static retry table defined in g_aTlcRetryPara[] 
    // ucVthShift refers to the number of retry attempts we have made so far (may choose a better name at a later time)

    U8 ucVtIndex = L3_ErrHGetRetryVtIndex(ucTLun, ucVthShift, bTLCMode, ucWlType, ucPageType);
    if (ucVtIndex >= RETRY_CNT)
    {
        ucVtIndex = ucVtIndex - RETRY_CNT;

        U8 ucVthAdjustTime = HAL_GetVthAdjustTime(ucPageType, bTLCMode);
        for (ucLevel = 0; ucLevel < ucVthAdjustTime; ucLevel++)
        {
            tRetryPara = HAL_FlashGetNewRetryParaTab(ucVtIndex, bTLCMode, ucWlType, ucPageType, ucLevel);
            bStatus = HAL_FlashRetrySendParam(&tFlashAddr, &tRetryPara, bTLCMode, HAL_FLASH_RETRY_PARA_MAX);
            if (NFC_STATUS_SUCCESS != bStatus)
            {
                DBG_Printf("TLun%d SetNewParameter Send Fail.\n", ucTLun);
                DBG_Getch();
            }
            bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != bStatus)
            {
                DBG_Printf("TLun%d SetNewParameter Wait Fail.\n", ucTLun);
                DBG_Getch();
            }
        }
    }
    else
    {
        tRetryPara = HAL_FlashGetRetryParaTab(ucVtIndex);
        bStatus = HAL_FlashRetrySendParam(&tFlashAddr, &tRetryPara, bTLCMode, HAL_FLASH_RETRY_PARA_MAX);
        if (NFC_STATUS_SUCCESS != bStatus)
        {
            DBG_Printf("TLun%d SetParameter Send Fail.\n", ucTLun);
            DBG_Getch();
        }
        bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != bStatus)
        {
            DBG_Printf("TLun%d SetParameter Wait Fail.\n", ucTLun);
            DBG_Getch();
        }
    }

    bStatus = HAL_FlashRetryEn(&tFlashAddr, TRUE);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetNewRetryEn Send Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetNewRetryEn Wait Fail.\n", ucTLun);
        DBG_Getch();
    }

    return;
}
#endif

/*==============================================================================
Func Name  : L3_ErrHSetParameter
Input      : U8 ucTLun
U8 ucVthShift
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
1. 2016.8.12 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHSetParameter(U8 ucTLun, U8 ucVthShift, BOOL bTLCMode)
{
    BOOL bStatus;
    RETRY_TABLE tRetryPara;
    FLASH_ADDR tFlashAddr;

    tFlashAddr.ucPU = L3_GET_PU(ucTLun);
    tFlashAddr.ucLun = L3_GET_LUN_IN_PU(ucTLun);

    tRetryPara = HAL_FlashGetRetryParaTab(ucVthShift);
    bStatus = HAL_FlashRetrySendParam(&tFlashAddr, &tRetryPara, bTLCMode, HAL_FLASH_RETRY_PARA_MAX);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetParameter Send Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetParameter Wait Fail.\n", ucTLun);
        DBG_Getch();
    }

    bStatus = HAL_FlashRetryEn(&tFlashAddr, TRUE);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetRetryEn Send Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetRetryEn Wait Fail.\n", ucTLun);
        DBG_Getch();
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHTerminate
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHTerminate(U8 ucTLun, BOOL bTLC)
{
    BOOL bStatus;
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);

    bStatus = HAL_FlashRetryTerminate(ucPU, ucLunInPU, bTLC);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d Terminate Send Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(ucPU, ucLunInPU);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d Terminate Wait Fail.\n", ucTLun);
        DBG_Getch();
    }

    return;
}

#ifdef READ_RETRY_REFACTOR
/*==============================================================================
Func Name  : L3_ErrHB0KBNewTerminate
Input      : U8 ucTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
    Called this function after each read retry, and it must be used before update ART
History    :
    1. 2017.08.28   JerryNie    create   function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHB0KBNewTerminate(U8 ucTLun, U8 ucVthShift, BOOL bTlcMode, U8 ucWlType, U8 ucPageType)
{
    BOOL bStatus;
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLun = L3_GET_LUN_IN_PU(ucTLun);

    if (TRUE == L3_ErrHIsHomemadeReadRetryEntry(ucTLun, ucVthShift, bTlcMode, ucWlType, ucPageType))
    {
        bStatus = HAL_FlashHomemadeVtTerminate(ucPU, ucLun, bTlcMode, ucWlType, ucPageType);
    }
    else
    {
        bStatus = HAL_FlashRetryTerminate(ucPU, ucLun, bTlcMode);
        if (NFC_STATUS_SUCCESS != bStatus)
        {
            DBG_Printf("TLun%d Terminate Send Fail.\n", ucTLun);
            DBG_Getch();
        }
        bStatus = HAL_NfcWaitStatus(ucPU, ucLun);
        if (NFC_STATUS_SUCCESS != bStatus)
        {
            DBG_Printf("TLun%d Terminate Wait Fail.\n", ucTLun);
            DBG_Getch();
        }
    }

    return;
}
#endif

/*==============================================================================
Func Name  : L3_ErrHGetVthValue
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL U8 MCU2_DRAM_TEXT L3_ErrHGetVthValue(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucVthValue;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (TRUE == ptReqEntry->tFlashDesc.bsShiftRdEn)
    {
        ucVthValue = L3_FMGetVthShiftRd(ptReqEntry->bsTLun);
    }
    else if (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
    {
        ucVthValue = L3_FMGetSlcVthRetry(ptReqEntry->bsTLun);
    }
    else
    {
        ucVthValue = L3_FMGetMlcVthRetry(ptReqEntry->bsTLun);
    }

    return ucVthValue;
}

/*==============================================================================
Func Name  : L3_ErrHUpdtVthRetry
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHUpdtVthRetry(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    UECC_ERRH_CTRL *ptUeccHCtrl;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    U8 ucTLun = ptReqEntry->bsTLun;

    if (FALSE == ptCtrlEntry->bsIntrReq)
    {
        U8 ucPri = ptReqEntry->bsFCmdPri;
        U8 ucReqPtr = ptReqEntry->bsReqPtr;
        ASSERT(FCMD_REQ_STS_POP == L2_FCMDQGetReqSts(ucTLun, ucPri, ucReqPtr));
    }

    if (TRUE == ptReqEntry->tFlashDesc.bsShiftRdEn)
    {
        return;
    }

    ptUeccHCtrl = &ptCtrlEntry->ptErrHEntry->tUeccHCtrl;

    if (READ_RETRY_SUCCESS == ptUeccHCtrl->bsSubStage)
    {
        if (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
        {
            L3_FMSetSlcVthRetry(ucTLun, ptUeccHCtrl->tRetry.bsTime);
        }
        else
        {
            L3_FMSetMlcVthRetry(ucTLun, ptUeccHCtrl->tRetry.bsTime);
        }
    }
    else
    {
        if (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
        {
            L3_FMSetSlcVthRetry(ucTLun, L3_FMGetSlcVthRetryDft(ucTLun));
        }
        else
        {
            L3_FMSetMlcVthRetry(ucTLun, L3_FMGetMlcVthRetryDft(ucTLun));
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHUpdtVthShiftRd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHUpdtVthShiftRd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (TRUE != ptReqEntry->tFlashDesc.bsShiftRdEn)
    {
        return;
    }

    // According PE to detect the VthShiftRd

    return;
}

/*==============================================================================
Func Name  : L3_ErrHRead
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
BOOL MCU2_DRAM_TEXT L3_ErrHRead(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bResult;

    if (FALSE == ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tRetry.bsUnSend)
    {
        L3_RstDatTxCtrler(ptCtrlEntry);
    }

    #ifdef SIM
    L3_DbgFCmdCntDec(ptCtrlEntry);
    #endif
    bResult = L3_IFSendNormalFCmd(ptCtrlEntry);
    #ifdef SIM
    L3_DbgFCmdPrint(ptCtrlEntry, (TRUE == bResult) ? "ShiftRd_Success" : "ShiftRd_Fail");
    #endif

    ptCtrlEntry->ptErrHEntry->tUeccHCtrl.tRetry.bsUnSend = !bResult;

    return bResult;
}

/*==============================================================================
Func Name  : L3_ErrHReDetectEmptyPage
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.23 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU2_DRAM_TEXT L3_ErrHReDetectEmptyPage(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    DBG_Printf("Host Read emptypage Redetect pending..\n");
    return TRUE;
}
/*==============================================================================
Func Name  : L3_ErrHEmptyPage
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHEmptyPage(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;

    U8 ucTLun, ucErrCode, ucPln;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage;
    BOOL bBootUpOk, bHostRdEn;

    ucTLun = ptReqEntry->bsTLun;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    bBootUpOk = ptReqEntry->bsBootUpOk;
    bHostRdEn = ptReqEntry->tFlashDesc.bsHostRdEn;

    L3_ErrHAssertCmd(ptReqEntry->bsReqType, ucErrCode);

    if (TRUE != bHostRdEn)
    {
        ptErrHEntry->bsErrRpt = TRUE;
        ptErrHEntry->bsEmptyPg = TRUE;
    }
    else
    {
        // How to deal with the host read empty page situation ?
        if (TRUE != ptReqEntry->bsBootUpOk)
        {
            // BookUpOk is not finish, ignore it directly.[sata->update buf-bitmap; nvme->bypass ecc and redo]
            ptErrHEntry->bsErrRpt = FALSE;
            ptErrHEntry->bsEmptyPg = TRUE;
            L3_HostReadEmpty(ptCtrlEntry);
        }
        else
        {
            // BootUpOk is finish, getch directly temporarily.
            L3_ErrHReDetectEmptyPage(ptCtrlEntry);
#ifdef FLASH_IM_3DTLC_GEN2
            DBG_Printf("TLun%d Blk%d_%d[%d_%d] Pln%d Page%d_%d SecRange=%d_%d_%d MergeRd=%d HostRd EmptyPage\n", ucTLun, usVirBlk, usPhyBlk,
                L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 0), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 1),
                ucPln, usVirPage, usPhyPage,ptReqEntry->tFlashDesc.bsSecStart, ptReqEntry->tFlashDesc.bsSecLen, ptReqEntry->tFlashDesc.bsLpnBitmap, ptReqEntry->tFlashDesc.bsMergeRdEn);
#else
            DBG_Printf("TLun%d Blk%d_%d[%d_%d_%d_%d] Pln%d Page%d_%d SecRange=%d_%d_%d MergeRd=%d HostRd EmptyPage\n", ucTLun, usVirBlk, usPhyBlk,
                L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 0), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 1), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 2), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 3),
                ucPln, usVirPage, usPhyPage,ptReqEntry->tFlashDesc.bsSecStart, ptReqEntry->tFlashDesc.bsSecLen, ptReqEntry->tFlashDesc.bsLpnBitmap, ptReqEntry->tFlashDesc.bsMergeRdEn);
#endif

            DBG_Getch();
        }
    }

    if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
    {
        L3_IFUpdtReqStatus(ptCtrlEntry);
    }

    L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);
    ptErrHEntry->tUeccHCtrl.bsSubStage = EMPTY_PG_HANDLE_DONE;

    if (TRUE == bBootUpOk)
    {
#ifdef SCAN_BLOCK_N1
        if (TRUE != ptReqEntry->tLocalDesc.bsPatrolRdCmd)
#endif
        DBG_Printf("TLun%d Blk%d_%d Pln%d Page%d_%d SecRange=%d_%d_%d HostRd=%d MergeRd=%d EmptyPage\n", ucTLun, usVirBlk, usPhyBlk, ucPln, usVirPage, usPhyPage,
            ptReqEntry->tFlashDesc.bsSecStart, ptReqEntry->tFlashDesc.bsSecLen, ptReqEntry->tFlashDesc.bsLpnBitmap, bHostRdEn, ptReqEntry->tFlashDesc.bsMergeRdEn);
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHReadRetry
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHReadRetry(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    UECC_ERRH_CTRL *ptUeccHCtrl = &ptErrHEntry->tUeccHCtrl;

    U8 ucTLun, ucPln, ucErrCode, ucSecStart, ucSecLen, ucLpnBitmap;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage;
    U32 ulLBA;
    BOOL bTLCMode;

    ucTLun = ptReqEntry->bsTLun;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ucSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ucSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ucLpnBitmap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;
    bTLCMode = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;
#ifdef READ_RETRY_REFACTOR
    U8 ucWlType, ucPageType, ucReadRetryTimeMax;

    ucWlType = HAL_GetFlashWlType(usPhyPage, bTLCMode);
    ucPageType = HAL_GetFlashPairPageType(usPhyPage);
    ucReadRetryTimeMax = HAL_FlashGetMaxRetryTime(bTLCMode, usPhyPage);

#endif

    L3_ErrHAssertCmd(ptReqEntry->bsReqType, ucErrCode);

    switch (ptUeccHCtrl->bsSubStage)
    {
        case READ_RETRY_INIT:
        {
            L3_ErrHPreCondition(ucTLun);

            ptUeccHCtrl->tRetry.bsEnable = TRUE;
            ptUeccHCtrl->tRetry.bsTime = 0;
#ifdef READ_RETRY_REFACTOR
            ptUeccHCtrl->bsSubStage = READ_RETRY_READ;
            break;
#else
            ptUeccHCtrl->bsSubStage = READ_RETRY_SHIFTRD;
#endif
            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x PreCondition\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA);
            //break;
        }
        case READ_RETRY_SHIFTRD:
        {
            if (FALSE == ptUeccHCtrl->tRetry.bsUnSend)
            {
                L3_ErrHSetParameter(ucTLun, L3_ErrHGetVthValue(ptCtrlEntry), bTLCMode);
            }

            if (TRUE == L3_ErrHRead(ptCtrlEntry))
            {
                ptUeccHCtrl->bsSubStage = READ_RETRY_SHIFTRD_CHK;
            }

            DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ShiftRdVth=%d\n",
                ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, L3_ErrHGetVthValue(ptCtrlEntry));
            break;
        }
        case READ_RETRY_SHIFTRD_CHK:
        {
            if (TRUE == ptErrHEntry->bsErrHold)
            {
                ptUeccHCtrl->bsSubStage = READ_RETRY_READ;
            }
            else
            {
                if (TRUE == L3_IFIsFCmdFinished(ptCtrlEntry))
                {
                    // For L2
                    if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
                    {
                        L2_PBIT_Set_Weak(L2_GET_SPU(ucTLun), usVirBlk, TRUE);
                        L3_IFUpdtReqStatus(ptCtrlEntry);
                    }

                    // For L3
                    L3_ErrHTerminate(ucTLun, bTLCMode);
                    ptUeccHCtrl->bsSubStage = READ_RETRY_SUCCESS;

                    L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);

                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x ShiftRdVth=%d Success. eraseCnt=%d\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, L3_ErrHGetVthValue(ptCtrlEntry),
                        pPBIT[0]->m_PBIT_Entry[L2_GET_SPU(ucTLun)][usPhyBlk].EraseCnt);
                }
            }
            break;
        }
        case READ_RETRY_READ:
        {
            if (FALSE == ptUeccHCtrl->tRetry.bsUnSend)
            {
#ifdef READ_RETRY_REFACTOR
                L3_ErrHSetNewParameter(ucTLun, ptUeccHCtrl->tRetry.bsTime, bTLCMode, ucWlType, ucPageType);
#else
                L3_ErrHSetParameter(ucTLun, ptUeccHCtrl->tRetry.bsTime, bTLCMode);
#endif
            }

            if (TRUE == L3_ErrHRead(ptCtrlEntry))
            {
                ptUeccHCtrl->bsSubStage = READ_RETRY_READ_CHK;
            }

            //DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x RetryTime=%d\n",
            //    ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, ptUeccHCtrl->tRetry.bsTime);
            break;
        }
        case READ_RETRY_READ_CHK:
        {
            if (TRUE == ptErrHEntry->bsErrHold)
            {
                ptUeccHCtrl->tRetry.bsTime++;
#ifdef READ_RETRY_REFACTOR
                L3_ErrHB0KBNewTerminate(ucTLun, ptUeccHCtrl->tRetry.bsTime - 1, bTLCMode, ucWlType, ucPageType);
                if (ucReadRetryTimeMax > ptUeccHCtrl->tRetry.bsTime)
                {
#else
                if (HAL_FLASH_READRETRY_CNT > ptUeccHCtrl->tRetry.bsTime)
                {
#endif
                    ptUeccHCtrl->bsSubStage = READ_RETRY_READ;
                }
                else
                {
                    // For L3
#ifndef READ_RETRY_REFACTOR
                    L3_ErrHTerminate(ucTLun, bTLCMode);
#endif
                    ptUeccHCtrl->bsSubStage = READ_RETRY_FAIL;
                    L3_ErrHUpdtVthRetry(ptCtrlEntry);

                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x RetryTime=%d Fail.eraseCnt=%d\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, ptUeccHCtrl->tRetry.bsTime,
                        pPBIT[0]->m_PBIT_Entry[L2_GET_SPU(ucTLun)][usPhyBlk].EraseCnt);
                }
            }
            else
            {
                if (TRUE == L3_IFIsFCmdFinished(ptCtrlEntry))
                {
                    ptUeccHCtrl->bsSubStage = READ_RETRY_SUCCESS;
#ifdef READ_RETRY_REFACTOR
#if 0
                    U8 ucPrintIndex;
                    DBG_Printf("\nTLun:%d RetryTime:%d Before update", ucTLun, ptUeccHCtrl->tRetry.bsTime);
                    if (bTLCMode)
                    {
                        if (SLC_TYPE == ucWlType)
                        {
                            DBG_Printf(" TLC_SLC_LP ");
                            for (ucPrintIndex = 0; ucPrintIndex < (SLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                            {
                                DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aSlcLpTable[ucPrintIndex]);
                            }
                        }
                        else if (MLC_TYPE == ucWlType)
                        {
                            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                            {
                                DBG_Printf(" TLC_MLC_LP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (MLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aMlcLpTable[ucPrintIndex]);
                                }
                            }
                            else if (HIGH_PAGE == ucPageType)
                            {
                                DBG_Printf(" TLC_MLC_UP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (MLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aMlcUpTable[ucPrintIndex]);
                                }
                            }
                            else
                            {
                                DBG_Printf("MLC WL page type error!\n");
                                DBG_Getch();
                            }
                        }
                        else if (TLC_TYPE == ucWlType)
                        {
                            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                            {
                                DBG_Printf(" TLC_TLC_LP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aTlcLpTable[ucPrintIndex]);
                                }
                            }
                            else if (HIGH_PAGE == ucPageType)
                            {
                                DBG_Printf(" TLC_TLC_UP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aTlcUpTable[ucPrintIndex]);
                                }
                            }
                            else if (EXTRA_PAGE == ucPageType)
                            {
                                DBG_Printf(" TLC_TLC_XP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aTlcXpTable[ucPrintIndex]);
                                }
                            }
                            else
                            {
                                DBG_Printf("TLC WL page type error!\n");
                                DBG_Getch();
                            }
                        }
                        else
                        {
                            DBG_Printf("WL type error!\n");
                            DBG_Getch();
                        }
                    }
                    else
                    {
                        DBG_Printf(" SLC_LP ");
                        for (ucPrintIndex = 0; ucPrintIndex < (SLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                        {
                            DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aSSlcLpTable[ucPrintIndex]);
                        }
                    }
                    DBG_Printf("\n");
#endif
                    L3_ErrHB0KBNewTerminate(ucTLun, ptUeccHCtrl->tRetry.bsTime, bTLCMode, ucWlType, ucPageType);
                    L3_ErrHUpdatAdaptiveRetryTable(ucTLun, ptUeccHCtrl->tRetry.bsTime, bTLCMode, ucWlType, ucPageType);                 
#if 0
                    DBG_Printf("TLun:%d RetryTime:%d After  update", ucTLun, ptUeccHCtrl->tRetry.bsTime);
                    if (bTLCMode)
                    {
                        if (SLC_TYPE == ucWlType)
                        {
                            DBG_Printf(" TLC_SLC_LP ");
                            for (ucPrintIndex = 0; ucPrintIndex < (SLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                            {
                                DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aSlcLpTable[ucPrintIndex]);
                            }
                        }
                        else if (MLC_TYPE == ucWlType)
                        {
                            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                            {
                                DBG_Printf(" TLC_MLC_LP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (MLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aMlcLpTable[ucPrintIndex]);
                                }
                            }
                            else if (HIGH_PAGE == ucPageType)
                            {
                                DBG_Printf(" TLC_MLC_UP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (MLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aMlcUpTable[ucPrintIndex]);
                                }
                            }
                            else
                            {
                                DBG_Printf("MLC WL page type error!\n");
                                DBG_Getch();
                            }
                        }
                        else if (TLC_TYPE == ucWlType)
                        {
                            if (LOW_PAGE == ucPageType || LOW_PAGE_WITHOUT_HIGH == ucPageType)
                            {
                                DBG_Printf(" TLC_TLC_LP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aTlcLpTable[ucPrintIndex]);
                                }
                            }
                            else if (HIGH_PAGE == ucPageType)
                            {
                                DBG_Printf(" TLC_TLC_UP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aTlcUpTable[ucPrintIndex]);
                                }
                            }
                            else if (EXTRA_PAGE == ucPageType)
                            {
                                DBG_Printf(" TLC_TLC_XP ");
                                for (ucPrintIndex = 0; ucPrintIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                                {
                                    DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aTlcXpTable[ucPrintIndex]);
                                }
                            }
                            else
                            {
                                DBG_Printf("TLC WL page type error!\n");
                                DBG_Getch();
                            }
                        }
                        else
                        {
                            DBG_Printf("WL type error!\n");
                            DBG_Getch();
                        }
                    }
                    else
                    {
                        DBG_Printf(" SLC_LP ");
                        for (ucPrintIndex = 0; ucPrintIndex < (SLC_RETRY_GROUP_CNT + RETRY_CNT); ucPrintIndex++)
                        {
                            DBG_Printf("%d ", g_aAdaptiveRetryTable[ucTLun].aSSlcLpTable[ucPrintIndex]);
                        }
                    }
                    DBG_Printf("\n");
#endif
#else
                    L3_ErrHUpdtVthRetry(ptCtrlEntry);
#endif

                    // For L2
                    if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
                    {
                        L2_PBIT_Set_Weak(L2_GET_SPU(ucTLun), usVirBlk, TRUE);

                        // Note:
                        //  If the cmd isn't intrReq, after call L3_IFUpdtReqStatus function.
                        //  L3 can't use the ptCtrlEntry, because L2 maybe update the ptCtrlEntry.
                        L3_IFUpdtReqStatus(ptCtrlEntry);
                    }

                    // For L3
#ifndef READ_RETRY_REFACTOR
                    L3_ErrHTerminate(ucTLun, bTLCMode);
#endif
                    L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);

#ifdef SCAN_BLOCK_N1
                    if (REQ_STS_UPT_MANUL != ptReqEntry->bsReqUptMod)
                    {
                        DEC_SRAM_STATUS_ENTRY *ptDecSramSts;
                        U8 ucPU, ucLunInPU, ucLevel;
                        U16 errBit[PLN_PER_LUN];
                        ucPU = L3_GET_PU(ucTLun);
                        ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);
                        ucLevel = ptCtrlEntry->bsNfcqPtr;
                        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
                        {
                            ptDecSramSts = (DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLunInPU][ucLevel][ucPln]);
                            errBit[ucPln] = ptDecSramSts->bsErrCntAcc;
                        }
                        DBG_Printf("HS L %d B %d %d P %d %d C %d %d S %d %d %d R %d %d E %d F %d %d\n",
                            ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ptUeccHCtrl->tRetry.bsTime,
                            L3_ErrHGetRetryVtIndex(ucTLun, ptUeccHCtrl->tRetry.bsTime, bTLCMode, ucWlType, ucPageType), pPBIT[0]->m_PBIT_Entry[L2_GET_SPU(ucTLun)][usPhyBlk].EraseCnt,
                            errBit[0], errBit[1]);
                    }
#else
                    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x RetryTime=%d Success.EC=%d\n",
                        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ptCtrlEntry->bsCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, ptUeccHCtrl->tRetry.bsTime,
                        pPBIT[0]->m_PBIT_Entry[L2_GET_SPU(ucTLun)][usPhyBlk].EraseCnt);
#endif
                }
            }
            break;
        }
        default:
        {
            DBG_Printf("ReadRetry Stage Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}

#ifdef N1_SCAN
/*==============================================================================
Func Name  : L3_SetTLCReadOffset
Input      : U8 ucTLun
             U8 ucPageType:0-LSB;1-CSB;2-MSB
Output     : NONE
Return Val : 
Discription: Build set feature CMD to set SAR paramters with FA 89h and 8Ah
Usage      : Called in SAR coarse or fine tuning
History    :
    20171226    AbbyLin     Create
==============================================================================*/
LOCAL void L3_SetTLCReadOffset(U8 ucTLun, U8 ucShiftVt, U8 ucAddr)
{
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLun = L3_GET_LUN_IN_PU(ucTLun);
    BOOL bStatus;
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = ucLun;
    
    bStatus = HAL_NfcSetFeature(&tFlashAddr, ucShiftVt, ucAddr);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetSARParameter Send Fail.\n", ucTLun);
        DBG_Getch();
    }
    bStatus = HAL_NfcWaitStatus(ucPU, ucLun);
    if (NFC_STATUS_SUCCESS != bStatus)
    {
        DBG_Printf("TLun%d SetSARParameter Wait Fail.\n", ucTLun);
        DBG_Getch();
    }
    else
    {
        DBG_Printf("N1 Send SetFeature FA=0x%x PA=0x%x.\n", ucAddr, ucShiftVt);
    }
        
}

LOCAL BOOL L3_N1ScanRead(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    BOOL bResult;

    L3_RstDatTxCtrler(ptCtrlEntry);

    bResult = L3_IFSendNormalFCmd(ptCtrlEntry);

    return bResult;
}

LOCAL U32 L3_CheckN1(U16 usBufID, U8 ucSecStart, U16 usSecLen)
{
    U8 ucShiftIdx = 0, ucBufIdx = 0;
    U16 usSec;
    U32 ulRDramAddr, ulRdData;
    U32 ulDwIndex, ulN1Cnt = 0;
    
    ulRDramAddr = (HAL_NfcGetDmaAddr(usBufID, ucSecStart, BUF_SIZE_BITS)<< 1) + DRAM_START_ADDRESS;
    //DBG_Printf("L3_CheckN1Data usBufID%d ucSecStart%d ucSecLen%d ulRDramAddr=0x%x\n", usBufID, ucSecStart, usSecLen, ulRDramAddr);

    for (usSec = ucSecStart; usSec < (ucSecStart + usSecLen); usSec++)
    {
        if ((0 == usSec % 2)&&(usSec != 0))
        {
            ulRDramAddr = ulRDramAddr + CW_INFO_SZ;
        }
        for (ulDwIndex = 0; ulDwIndex < 128; ulDwIndex++)
        {
            ulRdData = *(volatile U32*)(ulRDramAddr + (usSec << SEC_SZ_BITS) + ulDwIndex * 4);
            if( 0x0 != ulRdData )
            {
                ucShiftIdx = 0;
                while (ucShiftIdx < 32)
                {
                    if (0 != GET_BITS(ulRdData,ucShiftIdx,1))
                    {
                        ulN1Cnt++;
                        //DBG_Printf("ulDwIndex%d ulRdData=0x%x Bit%d = 1 N1 Acc=%d\n", ulDwIndex, ulRdData, ucShiftIdx, ulN1Cnt);
                    }
                    ucShiftIdx++;
                }
            }
        }
    }
   
    DBG_Printf("N1=0x%x\n", ulN1Cnt);
    
    return ulN1Cnt;

}
LOCAL U32 l_ulN1ScanPageNumIn1WL[PG_PER_WL] =
{
    36,     //LP
    61,     //UP
    60      //XP
};
/*==============================================================================
Func Name  : L3_VtScan
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription: Get Vt distribution by checking error WL N1 difference 
             when Vt is shifted among whole range
Usage      : Called when read error can't be recovered
History    :
    1. 2018.3.27 AbbyLin create function
==============================================================================*/
LOCAL void L3_VtScan(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *pReqEntry = ptCtrlEntry->ptReqEntry;
    UECC_ERRH_CTRL *ptUeccHCtrl = &(ptCtrlEntry->ptErrHEntry->tUeccHCtrl);

    if (FCMD_REQ_TLC_BLK != pReqEntry->tFlashDesc.bsBlkMod)  //Vt scan just support TLC mode
    {
        ptUeccHCtrl->bsSubStage = N1_SCAN_DONE;
        return;
    }
    
    U8 ucTLun;
    N1_SCAN_PARA *pN1Para;

    LOCAL U32 l_ulCurN1Cnt = 0, l_ulPreN1Cnt = 0;
    LOCAL U16 l_usPageCnt = 0;
    LOCAL U16 l_usErrPageBak = 0;  //bak the error page address
    LOCAL U32 l_aMinDeltaN1Cnt[READ_LEVEL_CNT] = {0};

    /* for layer(4 WL) and blk N1 test */
    U8 ucWLInLayer = 1; //1-WL base; 4-Layer base; (PG_PER_BLK-1)-Blk base

    ucTLun = pReqEntry->bsTLun;    
    pN1Para = &(ptUeccHCtrl->tN1ScanPara);
    
    switch (ptUeccHCtrl->bsSubStage)
    {
        case N1_SCAN_INIT:
        {
            DBG_Printf("\n--------------------TLUN%d N1 SCAN Start--------------------\n", ucTLun);
            U8 ucRLvlIdx, ucStep, ucEndVt;
            
            /* init N1 scan parameters */
            ucRLvlIdx = pN1Para->bsRdLevel;
            pN1Para->bsPageType = 0;  //Start from LP of a WL
            pN1Para->bsStep = 1;    //can be changed
            pN1Para->bsShiftVt = l_tL3N1ShiftRange[ucRLvlIdx].ucStartVt;
            ucStep = pN1Para->bsStep;
            ucEndVt = l_tL3N1ShiftRange[ucRLvlIdx].ucEndVt;
            l_tL3N1ShiftRange[ucRLvlIdx].ucEndVt = (0 == ucEndVt % ucStep) ? ucEndVt : (ucEndVt + (ucStep - ucEndVt % ucStep));//align the end shift Vt as N*ucStep 
            l_aMinDeltaN1Cnt[ucRLvlIdx] = INVALID_8F;
            //DBG_Printf("N1_SCAN_SHIFT_VT ucRLvlIdx%d ucStep%d Vt Range=0x%x-0x%x\n", ucRLvlIdx, ucStep, l_tL3N1ShiftRange[ucRLvlIdx].ucStartVt, l_tL3N1ShiftRange[ucRLvlIdx].ucEndVt);

            ptUeccHCtrl->bsSubStage = N1_SCAN_SHIFT_VT;
            l_usErrPageBak = pReqEntry->tFlashDesc.bsVirPage;
            break;
        }
        /* set feature to shift certain level Vt */
        case N1_SCAN_SHIFT_VT:
        {
            //DBG_Printf("\nTLun%d N1_SCAN_SHIFT_VT start\n",ucTLun);
            U8 ucRLvlIdx = pN1Para->bsRdLevel;
            
            L3_SetTLCReadOffset(ucTLun, pN1Para->bsShiftVt, l_tL3N1ShiftRange[ucRLvlIdx].ucAddr);
            
            if (pN1Para->bsShiftVt != l_tL3N1ShiftRange[ucRLvlIdx].ucStartVt)
            {
                //DBG_Printf("Shift Read Level %d With Vt=0x%x VtIdx=%d "
                //,pN1Para->bsRdLevel, pN1Para->bsShiftVt, pN1Para->bsVtIdx);
            }
            ptUeccHCtrl->bsSubStage = N1_SCAN_READ;
            break;
        }
        /* shift read raw data of one plane error page, 1 WL = page 36+61+60 */ 
        case N1_SCAN_READ:
        {
            //DBG_Printf("\nTLun%d N1_SCAN_READ start\n",ucTLun);
            
            FCMD_INTR_CTRL_ENTRY tTempCtrlEntry = *ptCtrlEntry;
            FCMD_REQ_ENTRY tReqEntry = *(ptCtrlEntry->ptReqEntry);
            tReqEntry.tFlashDesc.bsVirPage = l_ulN1ScanPageNumIn1WL[pN1Para->bsPageType];
            tReqEntry.tLocalDesc.bsRawRdCmd = TRUE;
            tReqEntry.atBufDesc[0].bsSecStart = 0;
            tReqEntry.atBufDesc[0].bsSecLen = SEC_PER_PHYPG;
            tReqEntry.atBufDesc[1].bsBufID = INVALID_4F;
            
            tTempCtrlEntry.ptReqEntry = &tReqEntry;
            //DBG_Printf("Read VirPage %d PageType%d\n", tReqEntry.tFlashDesc.bsVirPage, pN1Para->bsPageType);
            
            if(TRUE == L3_N1ScanRead(&tTempCtrlEntry))
            {
                ptUeccHCtrl->bsSubStage = N1_SCAN_CHECK_N1;
            }
            break;
        }
        /* Check N1 number and compare the DIF of 1 plane data */
        case N1_SCAN_CHECK_N1:
        {
            if(TRUE == L3_IFIsFCmdFinished(ptCtrlEntry))
            {
                //DBG_Printf("\nTLun%d N1_SCAN_CHECK_N1 start\n",ucTLun);
                U8 ucRdLevel = pN1Para->bsRdLevel;
                
                /*  1) read all page and acc N1 of one WL in current RD level and current Vt;
                    2) do next shift Vt in current RD level until to end Vt;
                    3) then terminate this RD level to do next RD level  */
                l_ulCurN1Cnt += L3_CheckN1(pReqEntry->atBufDesc[0].bsBufID, 0, SEC_PER_PHYPG);
                //DBG_Printf("ucRdLevel%d PageType%d N1 ACC=%d\n", ucRdLevel, pN1Para->bsPageType, l_ulCurN1Cnt);
                
                if ((PG_PER_WL - 1) != pN1Para->bsPageType)
                {
                    pN1Para->bsPageType++;
                    ptUeccHCtrl->bsSubStage = N1_SCAN_READ;
                }
                else//WL all page done
                {
                    pN1Para->bsPageType = 0;

                    #if 0
                    /* To do next page in current layer until layer done */
                    if((ucWLInLayer - 1) != l_usPageCnt)
                    {
                        l_usPageCnt++;
                        pReqEntry->tFlashDesc.bsVirPage++;///temp, need recover when all layer done.
                        ptUeccHCtrl->bsSubStage = N1_SCAN_READ;

                    }
                    else//layer done
                    #endif
                    {
                        /* calculate N1 of current layer */
                        if(pN1Para->bsShiftVt != l_tL3N1ShiftRange[ucRdLevel].ucStartVt)
                        {
                            U32 ulDeltaN1 = 0;
                            
                            if (l_ulCurN1Cnt > l_ulPreN1Cnt)
                                ulDeltaN1 = l_ulCurN1Cnt - l_ulPreN1Cnt;
                            else
                                ulDeltaN1 = l_ulPreN1Cnt - l_ulCurN1Cnt;
                                
                            pN1Para->bsVtIdx += pN1Para->bsStep;
                            DBG_Printf("%dWL Pre_Cur_DIF N1=%d_%d_%d\n", ucWLInLayer, l_ulPreN1Cnt, l_ulCurN1Cnt, ulDeltaN1);
                            if(ulDeltaN1 < l_aMinDeltaN1Cnt[ucRdLevel])
                            {
                                l_aMinDeltaN1Cnt[ucRdLevel] = ulDeltaN1;
                                l_tL3N1ShiftRange[ucRdLevel].ucBestVt = pN1Para->bsShiftVt;
                                //DBG_Printf("Min Delta N1 l_aMinN1Cnt[%d] Update to %d Cur Vt = 0x%x\n", ucRdLevel, l_aMinDeltaN1Cnt[ucRdLevel], pN1Para->bsShiftVt);
                            }
                        }

                        l_ulPreN1Cnt = l_ulCurN1Cnt;
                        l_ulCurN1Cnt = 0;
                        
                        /* recover page address and cnt */
                        l_usPageCnt = 0;
                        pReqEntry->tFlashDesc.bsVirPage = l_usErrPageBak;
                        if (l_tL3N1ShiftRange[ucRdLevel].ucEndVt != pN1Para->bsShiftVt)
                        {
                            pN1Para->bsShiftVt += pN1Para->bsStep;
                            ptUeccHCtrl->bsSubStage = N1_SCAN_SHIFT_VT;
                        }
                        else
                        {
                            ptUeccHCtrl->bsSubStage = N1_SCAN_TERMINATE;
                        }

                    }
                }
            }
            break;
        }

        case N1_SCAN_TERMINATE:
        {
            //DBG_Printf("\nTLun%d N1_SCAN_TERMINATE start\n",ucTLun);
            
            L3_SetTLCReadOffset(ucTLun, 0, l_tL3N1ShiftRange[pN1Para->bsRdLevel].ucAddr);
            DBG_Printf("Read Level %d Min Delta N1 = %d Best Vt = 0x%x\n"
            , pN1Para->bsRdLevel, l_aMinDeltaN1Cnt[pN1Para->bsRdLevel], l_tL3N1ShiftRange[pN1Para->bsRdLevel].ucBestVt);
            
            if((READ_LEVEL_CNT - 1) == pN1Para->bsRdLevel)
            {
                ptUeccHCtrl->bsSubStage = N1_SCAN_DONE;
                DBG_Printf("N1_SCAN_DONE\n");
                while(1);
            }
            else
            {
                pN1Para->bsRdLevel++;
                ptUeccHCtrl->bsSubStage = N1_SCAN_INIT;
                DBG_Printf("To do next RD level %d\n", pN1Para->bsRdLevel);
            }

            break;
        }
               
        default:
        {
            DBG_Printf("N1 Scan Stage Error\n");
            DBG_Getch();
        }
    }

    return;

}

#endif

LOCAL U8 MCU2_DRAM_TEXT L3_ErrHGetPlnErrBitMap(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FLASH_ADDR tFlashAddr = { 0 };
    FLASH_STATUS_ENTRY tFlashSts = { 0 };
    U8 ucTLun, ucPU, ucLunInPU, ucLevel, ucPln;
    DEC_SRAM_STATUS_ENTRY *ptDecSramSts;
    U8 ucPlnErrBitMap = 0;

    ucTLun = ptReqEntry->bsTLun;
    ucPU = L3_GET_PU(ucTLun);
    ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);
    ucLevel = ptCtrlEntry->bsNfcqPtr;

    if (FCMD_REQ_TYPE_ERASE == ptReqEntry->bsReqType || FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType)
    {
        tFlashAddr.ucPU = ucPU;
        tFlashAddr.ucLun = ucLunInPU;
        tFlashAddr.usBlock = ptReqEntry->tFlashDesc.bsVirBlk;
        tFlashAddr.usPage = 0;

        /*Check Single Plane Status, and return status*/
        for (tFlashAddr.bsPln = 0; tFlashAddr.bsPln < PLN_PER_LUN; tFlashAddr.bsPln++)
        {
            HAL_NfcSinglePlnReadSts(&tFlashAddr);
            if (NFC_STATUS_SUCCESS == HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                HAL_DecSramGetFlashSts(&tFlashAddr, (FLASH_STATUS_ENTRY*)&tFlashSts);
                if (0 != (tFlashSts.bsFlashStatus & 0x1))
                {
                    ucPlnErrBitMap = ucPlnErrBitMap | (1 << tFlashAddr.bsPln);//return single plane status
                }
            }
            else
            {
                DBG_Getch();
            }
        }
    }
    else if (FCMD_REQ_TYPE_READ == ptReqEntry->bsReqType)
    {
        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            ptDecSramSts = (DEC_SRAM_STATUS_ENTRY *)(&g_pDecSramStsBase->aDecStsSram[ucPU][ucLunInPU][ucLevel][ucPln]);
            volatile LDPC_RECC_THRE_REG *pReccReg = (volatile LDPC_RECC_THRE_REG *)LDPC_RECC_THRE_REG_BASE;

            if ((0 != ptDecSramSts->ulDecFailBitMap) || (ptDecSramSts->bsErrCntAcc > (16 * pReccReg->bsReccThr0))) // temporary threshold
            {
                ucPlnErrBitMap = ucPlnErrBitMap | (1 << ucPln);//return single plane status, Uecc
            }
        }
    }

    return ucPlnErrBitMap;
}

/*==============================================================================
Func Name  : L3_ErrHPrcUeccFail
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      : Block Replacement (Option) or Request Report
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHPrcUeccFail(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    UECC_ERRH_CTRL *ptUeccHCtrl = &ptErrHEntry->tUeccHCtrl;

    U8 ucTLun, ucCmdType, ucPln, ucErrCode, ucSecStart, ucSecLen, ucLpnBitmap;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage;
    U32 ulLBA;
    BOOL bNeedExtH;

    ucTLun = ptReqEntry->bsTLun;
    ucCmdType = ptCtrlEntry->bsCmdType;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ucSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ucSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ucLpnBitmap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;

    L3_FMSetCmdType(ucTLun, INVALID_2F);

    bNeedExtH = L3_ErrHIsNeedExtH(ptCtrlEntry);
    if (TRUE == bNeedExtH)
    {
        if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn && UECCH_FAILED_INIT == ptUeccHCtrl->bsSubStage)
        {
            L3_HostReadRecover(ptCtrlEntry, TRUE);
        }

        // Trigger the ExtH to complete the block replacement and report the current failed read request status.
        if (TRUE != L3_ExtHTrigger(ucTLun))
        {
            // ExtH Memrory shared by all lun.-> improve it later.
            ptUeccHCtrl->bsSubStage = UECCH_FAILED_PEND;
        }
        else
        {
            ptUeccHCtrl->bsSubStage = UECCH_FAILED_DONE;
        }
    }
    else
    {
        ptErrHEntry->bsErrRpt = TRUE;

        if (TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
        {
            if ((TRUE == ptReqEntry->tLocalDesc.bsRdtCmd) && (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType))
            {
                ptErrHEntry->bsPlnErrInfo = L3_ErrHGetPlnErrBitMap(ptCtrlEntry);
            }
        }

        if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
        {
            // Here: the current block need to be marked as a weak block or a bad block
            // ...

            if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
            {
                L3_HostReadRecover(ptCtrlEntry, TRUE);
            }

            L3_IFUpdtReqStatus(ptCtrlEntry);
        }

        L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);
        L3_ErrHFailLog(ucTLun, ucErrCode);

        #if (defined(DBG_TABLE_REBUILD) && !defined(L3_UNIT_TEST))
        extern void L2_ResetDbgPMTByPhyAdrr(U8 PUSer, U16 PhyBlk, U16 PhyPage);
        L2_ResetDbgPMTByPhyAdrr(ucTLun, usPhyBlk, usPhyPage);
        #endif

        ptUeccHCtrl->bsSubStage = UECCH_FAILED_DONE;
    }
#ifdef FLASH_IM_3DTLC_GEN2
    DBG_Printf("TLun%d Blk%d_%d[%d_%d] Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x UeccHandling Fail ExtH=%d_%d. Partol%d\n",ucTLun, usVirBlk, usPhyBlk,
        L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 0), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 1),
        usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, bNeedExtH, ptUeccHCtrl->bsSubStage, ptReqEntry->tLocalDesc.bsPatrolRdCmd);
#else
    DBG_Printf("TLun%d Blk%d_%d[%d_%d_%d_%d] Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x UeccHandling Fail ExtH=%d_%d. Patrol%d\n", ucTLun, usVirBlk, usPhyBlk,
        L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 0), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 1), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 2), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 3),
        usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, bNeedExtH, ptUeccHCtrl->bsSubStage, ptReqEntry->tLocalDesc.bsPatrolRdCmd);
#endif

    return;
}

/*==============================================================================
Func Name  : L3_ErrHReadRECC
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHReadRECC(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    UECC_ERRH_CTRL *ptUeccHCtrl = &ptErrHEntry->tUeccHCtrl;

    U8 ucTLun, ucCmdType, ucPln, ucErrCode, ucSecStart, ucSecLen, ucLpnBitmap;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage;
    U32 ulLBA;

    ucTLun = ptReqEntry->bsTLun;
    ucCmdType = ptCtrlEntry->bsCmdType;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ucSecStart = ptReqEntry->tFlashDesc.bsSecStart;
    ucSecLen = ptReqEntry->tFlashDesc.bsSecLen;
    ucLpnBitmap = ptReqEntry->tFlashDesc.bsLpnBitmap;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;

    L3_ErrHAssertCmd(ptReqEntry->bsReqType, ucErrCode);

    ptErrHEntry->bsErrRpt = TRUE;
    if (TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        if ((TRUE == ptReqEntry->tLocalDesc.bsRdtCmd) && (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType))
        {
            ptErrHEntry->bsPlnErrInfo = L3_ErrHGetPlnErrBitMap(ptCtrlEntry);
        }
    }

    if (TRUE == ptErrHEntry->tUeccHCtrl.tRetry.bsEnable)
    {
        L3_ErrHTerminate(ucTLun, FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod);
        L3_ErrHUpdtVthRetry(ptCtrlEntry);
    }

    if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
    {
        L2_PBIT_Set_Weak(L2_GET_SPU(ucTLun), usVirBlk, TRUE);

        // Note:
        //  If the cmd isn't intrReq, after call L3_IFUpdtReqStatus function.
        //  L3 can't use the ptCtrlEntry, because L2 maybe update the ptCtrlEntry.
        L3_IFUpdtReqStatus(ptCtrlEntry);
    }

    L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);
    L3_ErrHFailLog(ucTLun, ucErrCode);

    DBG_Printf("TLun%d Blk%d_%d Page%d_%d CmdType%d_%d SecRange=%d_%d_%d LBA=0x%x RetryTime=%d Recc.\n",
        ucTLun, usVirBlk, usPhyBlk, usVirPage, usPhyPage, ucCmdType, ucPln, ucSecStart, ucSecLen, ucLpnBitmap, ulLBA, ptUeccHCtrl->tRetry.bsTime);

    return;
}

/*==============================================================================
Func Name  : L3_ErrHReadUECC
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
1. EmptyPage Detect
-> Yes -> EmptyPage Handling
-> No  -> Read Retry
2. Read Retry
-> Success
-> Fail -> Soft Decoder
3. Soft Decoder
-> Success
-> Fail_NoXor -> Report Status
-> Fail_WithXor -> Data Recovery
4. Data Recovery
-> Success
-> Fail -> Report Status
5. Report Status
-> Need Blk Replacement in L3 -> Trigger ExtH, and report status in ExtH.
-> Need Blk Replacement in L2 -> Mark Blk and reprot status
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHReadUECC(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    UECC_ERRH_CTRL *ptUeccHCtrl = &ptCtrlEntry->ptErrHEntry->tUeccHCtrl;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    switch (ptUeccHCtrl->bsStage)
    {
        case UECC_ERRH_INIT:
        {
            if (TRUE == L3_ErrHDetectEmptyPage(ptCtrlEntry))
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_EMPTY_PAGE;
                ptUeccHCtrl->bsSubStage = EMPTY_PG_HANDLE_INIT;
            }
            else
            {
                if (TRUE != ptReqEntry->tFlashDesc.bsHostRdEn && (TRUE == ptReqEntry->tLocalDesc.bsPatrolRdCmd || TRUE == ptReqEntry->tLocalDesc.bsRdtCmd))
                {
                    ptUeccHCtrl->bsStage = UECC_ERRH_REPORT_FAIL;
                    break;
                }
                else
                {
                #ifdef N1_SCAN
                    ptUeccHCtrl->bsStage = UECC_ERRH_N1_SCAN;
                    ptUeccHCtrl->bsSubStage = N1_SCAN_INIT;
                   
                #else
                    ptUeccHCtrl->bsStage = UECC_ERRH_READ_RETRY;
                    ptUeccHCtrl->bsSubStage = READ_RETRY_INIT;
                #endif
                }
            }

            L3_IFClearFlashMonitor(ptReqEntry->bsTLun);
            L3_ErrHRstCacheOperation(ptReqEntry->bsTLun, (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE);

            break;
        }
        case UECC_ERRH_EMPTY_PAGE:
        {
            L3_ErrHEmptyPage(ptCtrlEntry);

            if (EMPTY_PG_HANDLE_DONE == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_DONE;
            }

            break;
        }
    #ifdef N1_SCAN
        case UECC_ERRH_N1_SCAN:
        {
            L3_VtScan(ptCtrlEntry);
            
            if (N1_SCAN_DONE == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_READ_RETRY;
                ptUeccHCtrl->bsSubStage = READ_RETRY_INIT;

            }
            else
            {
                ;
            }
            break;
        }
    #endif
        case UECC_ERRH_READ_RETRY:
        {
            L3_ErrHReadRetry(ptCtrlEntry);

            if (READ_RETRY_SUCCESS == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_DONE;
            }
            else if (READ_RETRY_FAIL == ptUeccHCtrl->bsSubStage)
            {
                if (TRUE == ptCtrlEntry->ptReqEntry->bsBootUpOk)
                {
                    #ifdef UECC_SOFT_DECODE_EN
                    ptUeccHCtrl->bsStage = UECC_ERRH_SOFT_DECODER;
                    ptUeccHCtrl->bsSubStage = SOFT_DECO_INIT;
                    #else
                    ptUeccHCtrl->bsStage = UECC_ERRH_REPORT_FAIL;
                    ptUeccHCtrl->bsSubStage = UECCH_FAILED_INIT;
                    #endif
                }
                else
                {
                    ptUeccHCtrl->bsStage = UECC_ERRH_REPORT_FAIL;
                    ptUeccHCtrl->bsSubStage = UECCH_FAILED_INIT;
                }
            }
            else
            {
                ; // continue to do retry.
            }

            break;
        }
        case UECC_ERRH_SOFT_DECODER:
        {
            L3_SoftDecoder(ptCtrlEntry);

            if (SOFT_DECO_SUCCESS == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_DONE;
            }
            else if (SOFT_DECO_FAIL2XOR == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_DATA_RECOVER;
                ptUeccHCtrl->bsSubStage = DATA_RECO_INIT;
            }
            else if (SOFT_DECO_FAIL2FAIL == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_REPORT_FAIL;
                ptUeccHCtrl->bsSubStage = UECCH_FAILED_INIT;
            }
            else
            {
                ; // continue to do soft decoder.
            }

            break;
        }
        case UECC_ERRH_DATA_RECOVER:
        {
            L3_DataRecovery(ptCtrlEntry);

            if (DATA_RECO_SUCCESS == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_DONE;
            }
            else if (DATA_RECO_FAIL == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_REPORT_FAIL;
                ptUeccHCtrl->bsSubStage = UECCH_FAILED_INIT;
            }
            else
            {
                ; // continue to do data recovery.
            }
            break;
        }
        case UECC_ERRH_REPORT_FAIL:
        {
            L3_ErrHPrcUeccFail(ptCtrlEntry);

            if (UECCH_FAILED_DONE == ptUeccHCtrl->bsSubStage)
            {
                ptUeccHCtrl->bsStage = UECC_ERRH_DONE;
            }

            break;
        }
        default:
        {
            DBG_Printf("UECC ErrH Stage Error 0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHReadDCRC
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHReadDCRC(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    DBG_Printf("DCRC Fail: report this crc error to the caller...\n");
    L3_ErrHPrcUeccFail(ptCtrlEntry);

    return;
}

/*==============================================================================
Func Name  : L3_ErrHReadLBA
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHReadLBA(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    DBG_Printf("LBAChk Fail: Handle it as uecc error hendling...\n");
    L3_ErrHReadUECC(ptCtrlEntry);

    return;
}

/*==============================================================================
Func Name  : L3_ErrHProgram
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHProgram(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    U8 ucTLun,  ucCmdType, ucErrCode, ucPln;
    U16 usVirBlk, usPhyBlk, usVirPage, usPhyPage;
    U32 ulLBA;
    BOOL bNeedExtH;

    ucTLun = ptReqEntry->bsTLun;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    usVirPage = ptReqEntry->tFlashDesc.bsVirPage;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;
    ulLBA = ptReqEntry->tHostDesc.ulFtlLba;
    usPhyPage = ptCtrlEntry->bsPhyPage;
    ucCmdType = ptCtrlEntry->bsCmdType;

    L3_ErrHAssertCmd(ptReqEntry->bsReqType, ucErrCode);

    // RDT read single plane status before reset LUN
    if (TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        if ((TRUE == ptReqEntry->tLocalDesc.bsRdtCmd) && ((FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType || FCMD_REQ_SUBTYPE_ONEPG == ptReqEntry->bsReqSubType)))
        {
            ptErrHEntry->bsPlnErrInfo = L3_ErrHGetPlnErrBitMap(ptCtrlEntry);
        }
    }

    L3_ErrHRstCacheOperation(ucTLun, (FCMD_REQ_SLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE);

    bNeedExtH = L3_ErrHIsNeedExtH(ptCtrlEntry);
    if (TRUE == bNeedExtH)
    {
        L3_ExtHTrigger(ucTLun);
    }
    else
    {
        ptErrHEntry->bsErrRpt = TRUE;

        if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
        {
            L3_IFUpdtReqStatus(ptCtrlEntry);
        }

        L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);
        L3_ErrHFailLog(ucTLun, ucErrCode);
    }
#ifdef FLASH_IM_3DTLC_GEN2
    DBG_Printf("TLun%d Blk%d_%d[%d_%d] Page%d_%d CmdType%d_%d BlkMode %d LBA=0x%x PrgHandling ExtH=%d.\n",ucTLun, usVirBlk, usPhyBlk,
        L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 0), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 1),
        usVirPage, usPhyPage, ucCmdType, ucPln, ptCtrlEntry->ptReqEntry->tFlashDesc.bsBlkMod, ulLBA, bNeedExtH);
#else
    DBG_Printf("TLun%d Blk%d_%d[%d_%d_%d_%d] Page%d_%d CmdType%d_%d BlkMode %d LBA=0x%x PrgHandling ExtH=%d.\n",ucTLun, usVirBlk, usPhyBlk,
        L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 0), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 1), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 2), L2_BbtGetPbnBindingTable(ucTLun, usPhyBlk, 3),
        usVirPage, usPhyPage, ucCmdType, ucPln, ptCtrlEntry->ptReqEntry->tFlashDesc.bsBlkMod, ulLBA, bNeedExtH);
#endif

    return;
}

/*==============================================================================
Func Name  : L3_ErrHErase
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      : Block Swap (option) + Request Status Report
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHErase(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;
    U8 ucTLun, ucSPU, ucLunInSPU, ucReqType, ucErrCode, ucPln;
    U16 usVirBlk, usPhyBlk, usCurVirBlk, usNewVirBlk = INVALID_4F, usNewPhyBlk;
    BOOL bTLCBlk, bNeedExtH;

    ucTLun = ptReqEntry->bsTLun;
    ucReqType = ptReqEntry->bsReqType;
    ucErrCode = ptErrHEntry->bsErrCode;
    usVirBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    usPhyBlk = ptReqEntry->tFlashDesc.bsPhyBlk;
    ucPln = ptReqEntry->tFlashDesc.bsPlnNum;

    L3_ErrHAssertCmd(ucReqType, ucErrCode);

    // Patch for the tsb-2d-tlc erase status issue.
    if (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod)
    {
        /*can't reset lun after multi-plane erase fail for re-detecting the flash status by read-status plane by plane.*/
        //L3_ErrHRstLun(ucTLun, FALSE);
    }

    bNeedExtH = L3_ErrHIsNeedExtH(ptCtrlEntry);

    if (TRUE == bNeedExtH)
    {
        ucSPU = L2_GET_SPU(ucTLun);
        ucLunInSPU = L2_GET_LUN_IN_SPU(ucTLun);
        usCurVirBlk = L2_PBIT_GetVirturlBlockAddr(ucSPU, ucLunInSPU, usPhyBlk);
        ASSERT(usVirBlk == usCurVirBlk);
        bTLCBlk = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;

        usNewPhyBlk = L2_ErrHReplaceBLK(ucSPU, ucLunInSPU, usVirBlk, bTLCBlk, ERASE_ERR);

#ifdef ERRH_MODIFY_PENDING_CMD
        HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ERRH_PEND_CMD);
        /*Modify Pending command param that physical block has been replaced*/
        L3_ExtHPrcPendingCmd(ucTLun, usCurVirBlk, usNewPhyBlk, FALSE);
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ERRH_PEND_CMD);
#endif
    }

    ptErrHEntry->bsErrRpt = TRUE;
    if (TRUE != ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        if ((TRUE == ptReqEntry->tLocalDesc.bsRdtCmd) && (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType))
        {
            ptErrHEntry->bsPlnErrInfo = L3_ErrHGetPlnErrBitMap(ptCtrlEntry);
        }
    }

    if (TRUE != L3_IFIsRecycled(ptCtrlEntry))
    {
        L3_IFUpdtReqStatus(ptCtrlEntry);
    }

    L3_FCMDQSetIntrEptr(ucTLun, INVALID_DPTR, FALSE);
    L3_ErrHFailLog(ucTLun, ucErrCode);

    if (FALSE == bNeedExtH)
    {
        DBG_Printf("TLun%d Blk%d_%d Pln%d Erase Fail NoExtH\n", ucTLun, usVirBlk, usPhyBlk, ucPln);
    }
    else
    {
        DBG_Printf("TLun%d Blk%d_%d_%d Erase Fail WithExtH [%d,%d]-[%d]\n", ucTLun, usVirBlk, usPhyBlk, usCurVirBlk, usCurVirBlk, usPhyBlk, usNewPhyBlk);
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHNoDev
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHNoDev(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    DBG_Printf("NoDev Fail Pending...\n");
    DBG_Getch();

    return;
}

/*==============================================================================
Func Name  : L3_ErrHProcess
Input      : U8 ucTLun
             FCMD_INTR_ERRH_ENTRY *ptErrHEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.25 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_ErrHProcess(U8 ucTLun, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry = ptCtrlEntry->ptErrHEntry;

    if (TRUE == ptErrHEntry->bsErrHold)
    {
        L3_ErrHRstCmdQ(ucTLun);
    }

    switch (ptErrHEntry->bsErrCode)
    {
        case NF_ERR_TYPE_RECC:
        {
            L3_ErrHReadRECC(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_UECC:
        {
            L3_ErrHReadUECC(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_DCRC:
        {
            L3_ErrHReadDCRC(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_LBA:
        {
            L3_ErrHReadLBA(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_PRG:
        case NF_ERR_TYPE_PREPRG:
        case NF_ERR_TYPE_BOTHPRG:
        {
            L3_ErrHProgram(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_ERS:
        {
            L3_ErrHErase(ptCtrlEntry);
            break;
        }
        case NF_ERR_TYPE_NODEV:
        {
            L3_ErrHNoDev(ptCtrlEntry);
            break;
        }
        default:
        {
            DBG_Printf("ErrCode Error. ErrCode:0x%x FCmd:0x%x\n", ptErrHEntry->bsErrCode, (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_ErrHAllocDram
Input      : U32 *pFreeDramBase
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
    2. 2017.8.07 Jerry Modify
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHAllocDram(U32 *pFreeDramBase)
{
    U32 ulFreeBase = *pFreeDramBase;
    COM_MemAddr16DWAlign(&ulFreeBase);

    // FCMD_INTR_ERRH
    g_ptFCmdIntrErrH = (FCMD_INTR_ERRH *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_INTR_ERRH_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

#if defined(READ_RETRY_REFACTOR)
    // ERRH Retry Adaptive Retry Table
    g_aAdaptiveRetryTable = (ADAPTIVE_RETRY_TABLE *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, SUBSYSTEM_LUN_NUM * sizeof(ADAPTIVE_RETRY_TABLE));
    COM_MemAddr16DWAlign(&ulFreeBase);
#endif

    ASSERT(ulFreeBase-DRAM_DATA_BUFF_MCU2_BASE < DATA_BUFF_MCU2_SIZE);
    *pFreeDramBase = ulFreeBase;

    return;
}

/*==============================================================================
Func Name  : L3_ErrHAdaptiveRetryTableInit
Input      : NONE
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2017.8.7 JerryNie create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_ErrHAdaptiveRetryTableInit(void)
{
#ifdef READ_RETRY_REFACTOR
    U8 ucIndex, ucTLun;
    ADAPTIVE_RETRY_TABLE *pLunAdaptiveRetryTable;          // Component Adaptive Retry Table

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        pLunAdaptiveRetryTable = &g_aAdaptiveRetryTable[ucTLun];
        for (ucIndex = 0; ucIndex < (TLC_RETRY_GROUP_CNT + RETRY_CNT); ucIndex++)
        {
            pLunAdaptiveRetryTable->aTlcLpTable[ucIndex] = ucIndex;
            pLunAdaptiveRetryTable->aTlcUpTable[ucIndex] = ucIndex;
            pLunAdaptiveRetryTable->aTlcXpTable[ucIndex] = ucIndex;
        }

        for (ucIndex = 0; ucIndex < (MLC_RETRY_GROUP_CNT + RETRY_CNT); ucIndex++)
        {
            pLunAdaptiveRetryTable->aMlcLpTable[ucIndex] = ucIndex;
            pLunAdaptiveRetryTable->aMlcUpTable[ucIndex] = ucIndex;
        }

        for (ucIndex = 0; ucIndex < (SLC_RETRY_GROUP_CNT + RETRY_CNT); ucIndex++)
        {
            pLunAdaptiveRetryTable->aSlcLpTable[ucIndex] = ucIndex;
        }

        for (ucIndex = 0; ucIndex < (SLC_RETRY_GROUP_CNT + RETRY_CNT); ucIndex++)
        {
            pLunAdaptiveRetryTable->aSSlcLpTable[ucIndex] = ucIndex;
        }
    }
#endif

    return;
}

/*==============================================================================
Func Name  : L3_PCmdHandling
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.20 JasonGuo create function
==============================================================================*/
void L3_PCmdHandling(U8 ucTLun)
{
    U8 ucCurPptr, ucNxtPptr;
    BOOL bResult;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;

    // Before send fcmd, we need to make sure that the nfcq is not full.
    if (TRUE == HAL_NfcGetFull(L3_GET_PU(ucTLun), L3_GET_LUN_IN_PU(ucTLun)))
    {
        return;
    }

    ucCurPptr = L3_FCMDQGetIntrPptr(ucTLun, FALSE);
    ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucCurPptr, FALSE);

    #ifdef SIM
    L3_DbgFCmdCntDec(ptCtrlEntry);
    #endif
    bResult = L3_IFSendFCmd(ptCtrlEntry);

    if (TRUE == bResult)
    {
        ucNxtPptr = (ucCurPptr+1) % NFCQ_DEPTH;
        ucNxtPptr = (ucNxtPptr == L3_FCMDQGetIntrWptr(ucTLun, FALSE)) ? INVALID_DPTR : ucNxtPptr;
        L3_FCMDQSetIntrPptr(ucTLun, ucNxtPptr, FALSE);
    }

    return;
}

/*==============================================================================
Func Name  : L3_ErrHHandling
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.22 JasonGuo create function
==============================================================================*/
void L3_ErrHHandling(U8 ucTLun)
{
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;

    ptCtrlEntry = L3_ErrHPretreat(ucTLun);
    if (TRUE == L3_ErrHIsPreDone(ucTLun, ptCtrlEntry))
    {
        L3_ErrHProcess(ucTLun, ptCtrlEntry);
    }

    return;
}

/*====================End of this file========================================*/
