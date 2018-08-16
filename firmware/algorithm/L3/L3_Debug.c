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
* File Name    : L3_Debug.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L3_Interface.h"
#include "L3_Debug.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
#ifdef SIM
/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
volatile U32 l_aFCmdCnt[SUBSYSTEM_LUN_MAX][L3_DBG_FCMD_REQ_NUM];

/*==============================================================================
Func Name  : L3_DbgFCmdCntPrint
Input      : U8 ucPhyTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgFCmdCntPrint(U8 ucPhyTLun)
{
    U8 ucIdx;

    for (ucIdx = 0; ucIdx < L3_DBG_FCMD_REQ_NUM; ucIdx++)
    {
        DBG_Printf("l_aFCmdCnt[%d][%d] = %ld.\n", ucPhyTLun, ucIdx, l_aFCmdCnt[ucPhyTLun][ucIdx]);
    }

    return;
}

/*==============================================================================
Func Name  : L3_DbgFCmdAllChk
Input      : U8 ucPhyTLun
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgFCmdAllChk(U8 ucPhyTLun)
{
    U8 ucIdx;
    U32 ulFCmdCnt = 0;

    for (ucIdx = L3_DBG_FCMD_REQ_SPECIAL; ucIdx < L3_DBG_FCMD_REQ_HOST_RD_DONE; ucIdx++)
    {
        ulFCmdCnt += l_aFCmdCnt[ucPhyTLun][ucIdx];
    }
    if (ulFCmdCnt != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_ALL])
    {
        DBG_Printf("PhyTLun%d L3 Dbg All Chk Fail.\n", ucPhyTLun);
        L3_DbgFCmdCntPrint(ucPhyTLun);
        DBG_Getch();
    }

    return;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_DbgL2SendCntAdd
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
BOOL L3_DbgL2SendCntAdd(U8 ucTLun)
{
    U8 ucPhyTLun;
    S32 slCurDif;
    U32 aFCmdCnt[2];

    ucPhyTLun = NFC_PU_MAX * L3_GET_LUN_IN_PU(ucTLun) + HAL_NfcGetPhyPU(L3_GET_PU(ucTLun));
    l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_SEND]++;

    aFCmdCnt[0] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_SEND];
    aFCmdCnt[1] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_ALL];

    slCurDif = aFCmdCnt[0] - aFCmdCnt[1];
    if (0 > slCurDif)
    {
        slCurDif = (INVALID_8F - aFCmdCnt[1] + aFCmdCnt[0] + 1) % INVALID_8F;
    }

    if (NFCQ_DEPTH < slCurDif)
    {
        DBG_Printf("PhyTLun%d-TLun%d L2 Send Cnt Fail %d.\n", ucPhyTLun,  ucTLun, slCurDif);
        L3_DbgFCmdCntPrint(ucPhyTLun);
        DBG_Getch();
    }

    #ifdef L3_DBG_PRINT_EN
    return TRUE;
    #else
    return FALSE;
    #endif
}

/*==============================================================================
Func Name  : L3_DbgFCmdCntAdd
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
void L3_DbgFCmdCntAdd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucPhyTLun;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (ptReqEntry->bsReqType == FCMD_REQ_TYPE_SETFEATURE)
    {
        return;
    }

    ucPhyTLun = NFC_PU_MAX * L3_GET_LUN_IN_PU(ptReqEntry->bsTLun) + HAL_NfcGetPhyPU(L3_GET_PU(ptReqEntry->bsTLun));

    if (FALSE == ptCtrlEntry->bsIntrReq)
    {
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_ALL]++;
    }

    if (TRUE == L3_IFIsSpecialFCmd(ptCtrlEntry))
    {
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_SPECIAL]++;
    }
    else if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_HOST_RD]++;
    }
    else if (REQ_STS_UPT_AUTO == ptReqEntry->bsReqUptMod)
    {
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_AUTO]++;
    }
    else if (REQ_STS_UPT_MANUL == ptReqEntry->bsReqUptMod)
    {
        if (FALSE == ptCtrlEntry->bsIntrReq)
        {
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_MANUL]++;
        }
        else
        {
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_INTR]++;
        }
    }
    else if (REQ_STS_UPT_NULL == ptReqEntry->bsReqUptMod)
    {
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_NULL]++;
    }
    else
    {
        DBG_Printf("TLun%d_%d Req Add Setting Miss. 0x%x\n", ptReqEntry->bsTLun, ucPhyTLun, (U32)ptCtrlEntry);
        DBG_Getch();
    }

    return;
}

/*==============================================================================
Func Name  : L3_DbgFCmdCntDec
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
void L3_DbgFCmdCntDec(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucPhyTLun;
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    ucPhyTLun = NFC_PU_MAX * L3_GET_LUN_IN_PU(ptReqEntry->bsTLun) + HAL_NfcGetPhyPU(L3_GET_PU(ptReqEntry->bsTLun));
    ASSERT(0 != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_ALL]);

    if (FALSE == ptCtrlEntry->bsIntrReq)
    {
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_ALL]--;
    }

    if (TRUE == ptReqEntry->tFlashDesc.bsHostRdEn)
    {
        ASSERT(0 != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_HOST_RD]);
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_HOST_RD]--;
    }
    else if (REQ_STS_UPT_AUTO == ptReqEntry->bsReqUptMod)
    {
        ASSERT(0 != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_AUTO]);
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_AUTO]--;
    }
    else if (REQ_STS_UPT_MANUL == ptReqEntry->bsReqUptMod)
    {
        if (FALSE == ptCtrlEntry->bsIntrReq)
        {
            ASSERT(0 != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_MANUL]);
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_MANUL]--;
        }
        else
        {
            ASSERT(0 != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_INTR]);
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_INTR]--;
        }
    }
    else if (REQ_STS_UPT_NULL == ptReqEntry->bsReqUptMod)
    {
        ASSERT(0 != l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_NULL]);
        l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_NULL]--;
    }
    else
    {
        DBG_Printf("TLun%d_%d Req Dec Setting Miss. 0x%x\n", ptReqEntry->bsTLun, ucPhyTLun, (U32)ptCtrlEntry);
        DBG_Getch();
    }

    return;
}


/*==============================================================================
Func Name  : L3_DbgFCmdChk
Input      : U8 ucPhyTLun
             U8 eDbgFCmdType
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgFCmdChk(U8 ucPhyTLun, U8 eDbgFCmdType, BOOL bIntrReq)
{
    S32 slCurDif;
    U32 aFCmdCnt[SUBSYSTEM_LUN_MAX][2] = { 0 };

    switch (eDbgFCmdType)
    {
        case L3_DBG_FCMD_REQ_ALL:
        {
            L3_DbgFCmdAllChk(ucPhyTLun);
            return;
        }
        case L3_DBG_FCMD_REQ_HOST_RD:
        {
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_HOST_RD_DONE]++;
            aFCmdCnt[ucPhyTLun][0] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_HOST_RD];
            aFCmdCnt[ucPhyTLun][1] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_HOST_RD_DONE];
            break;
        }
        case L3_DBG_FCMD_REQ_AUTO:
        {
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_AUTO_DONE]++;
            aFCmdCnt[ucPhyTLun][0] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_AUTO];
            aFCmdCnt[ucPhyTLun][1] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_AUTO_DONE];
            break;
        }
        case L3_DBG_FCMD_REQ_MANUL:
        {
            L3_DbgFCmdAllChk(ucPhyTLun);

            if (FALSE == bIntrReq)
            {
                l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_MANUL_DONE]++;
                aFCmdCnt[ucPhyTLun][0] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_MANUL];
                aFCmdCnt[ucPhyTLun][1] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_MANUL_DONE];
            }
            else
            {
                l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_INTR_DONE]++;
                aFCmdCnt[ucPhyTLun][0] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_INTR];
                aFCmdCnt[ucPhyTLun][1] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_INTR_DONE];
            }
            break;
        }
        case L3_DBG_FCMD_REQ_NULL:
        {
            l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_NULL_DONE]++;
            aFCmdCnt[ucPhyTLun][0] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_NULL];
            aFCmdCnt[ucPhyTLun][1] = l_aFCmdCnt[ucPhyTLun][L3_DBG_FCMD_REQ_NULL_DONE];
            break;
        }
        default:
        {
            DBG_Printf("PhyTLun%d L3 Dbg FCmd Chk Status Error.\n", ucPhyTLun);
            DBG_Getch();
        }
    }

    slCurDif = aFCmdCnt[ucPhyTLun][0] - aFCmdCnt[ucPhyTLun][1];
    if (0 > slCurDif)
    {
        slCurDif = (INVALID_8F - aFCmdCnt[ucPhyTLun][1] + aFCmdCnt[ucPhyTLun][0] + 1) % INVALID_8F;
    }

    if (NFCQ_DEPTH <= slCurDif)
    {
        DBG_Printf("PhyTLun%d L3 Dbg FCmd Chk %d Fail.\n", ucPhyTLun, eDbgFCmdType);
        L3_DbgFCmdCntPrint(ucPhyTLun);
        DBG_Getch();
    }

    return;
}

/*==============================================================================
Func Name  : L3_DbgFCmdAutoChk
Input      : U8 ucPhyTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
BOOL L3_DbgFCmdAutoChk(U8 ucPhyTLun)
{
    L3_DbgFCmdChk(ucPhyTLun, L3_DBG_FCMD_REQ_AUTO, FALSE);

#ifdef L3_DBG_PRINT_EN
    return TRUE;
#else
    return FALSE;
#endif
}
BOOL L3_DbgFCmdManulChk(U8 ucPhyTLun, BOOL bIntrReq)
{
    L3_DbgFCmdChk(ucPhyTLun, L3_DBG_FCMD_REQ_MANUL, bIntrReq);

#ifdef L3_DBG_PRINT_EN
    return TRUE;
#else
    return FALSE;
#endif
}
BOOL L3_DbgFCmdNullChk(U8 ucPhyTLun)
{
    L3_DbgFCmdChk(ucPhyTLun, L3_DBG_FCMD_REQ_NULL, FALSE);

#ifdef L3_DBG_PRINT_EN
    return TRUE;
#else
    return FALSE;
#endif
}
BOOL L3_DbgFCmdHostRdChk(U8 ucPhyTLun)
{
    L3_DbgFCmdChk(ucPhyTLun, L3_DBG_FCMD_REQ_HOST_RD, FALSE);

#ifdef L3_DBG_PRINT_EN
    return TRUE;
#else
    return FALSE;
#endif
}

/*==============================================================================
Func Name  : L3_DbgFCmdPrint
Input      : FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
             U8 *pStr
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.8.2 JasonGuo create function
==============================================================================*/
void L3_DbgFCmdPrint(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, U8 *pStr)
{
#ifndef L3_DBG_PRINT_EN
    return;
#endif

    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    // FIRMWARE_LogInfo  DBG_Printf
    FIRMWARE_LogInfo("TLun%d Blk%d_%d Page%d_%d, CmdType%d_%d_%d Pln%d BlkMod%d UptMod%d Step%d, Ptr Req%d Ctr%d. Nfcq%d. EngID0x%x_0x%x_%d. %s\n",
        ptReqEntry->bsTLun,
        ptReqEntry->tFlashDesc.bsVirBlk,
        ptReqEntry->tFlashDesc.bsPhyBlk,
        ptReqEntry->tFlashDesc.bsVirPage,
        ptCtrlEntry->bsPhyPage,
        ptReqEntry->bsReqType,
        ptReqEntry->bsReqSubType,
        ptCtrlEntry->bsCmdType,
        ptReqEntry->tFlashDesc.bsPlnNum,
        ptReqEntry->tFlashDesc.bsBlkMod,
        ptReqEntry->bsReqUptMod,
        ptCtrlEntry->bsMultiStep,
        ptReqEntry->bsReqPtr,
        ptCtrlEntry->bsCtrlPtr,
        ptCtrlEntry->bsNfcqPtr,
        ptCtrlEntry->tDTxCtrl.bsFstEngineID,
        ptCtrlEntry->tDTxCtrl.bsCurEngineID,
        ptCtrlEntry->tDTxCtrl.bsBdEngineDone,
        pStr
        );

    return;
}
#endif

/*==============================================================================
Func Name  : L3_DbgInit
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void L3_DbgInit(void)
{
    #ifdef SIM
    COM_MemZero((U32*)&l_aFCmdCnt[0][0], SUBSYSTEM_LUN_MAX*L3_DBG_FCMD_REQ_NUM);
    #endif
}

#ifdef L3_DBG_ERR_INJ_EN
#define L3_DBG_ERR_CNT (8)
LOCAL U8 l_aErrInjList[L3_DBG_ERR_CNT][2] =
{
    {NF_ERR_TYPE_ERS,     NF_ERR_INJ_TYPE_ERS},
    {NF_ERR_TYPE_PRG,     NF_ERR_INJ_TYPE_PRG},
    {NF_ERR_TYPE_PREPRG,  NF_ERR_INJ_TYPE_PRE_PG_PRG},
    {NF_ERR_TYPE_BOTHPRG, NF_ERR_INJ_TYPE_PRE_CUR_PRG},
    {NF_ERR_TYPE_UECC,    INVALID_2F},
    {NF_ERR_TYPE_RECC,    INVALID_2F},
    {NF_ERR_TYPE_LBA,     INVALID_2F},
    {NF_ERR_TYPE_DCRC,    INVALID_2F}
};

#if 0
/*==============================================================================
Func Name  : L3_DbgInjReadErr
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgInjReadErr(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucIdx, ucInjErrTime;
    NFC_ERR_INJ tErrInj = {0};
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;
    FCMD_FLASH_DESC *ptFlashDesc = &ptReqEntry->tFlashDesc;
    BOOL bTLCMode = (FCMD_REQ_TLC_BLK == ptReqEntry->tFlashDesc.bsBlkMod) ? TRUE : FALSE;

    if ((TRUE == ptNfcqEntry->bsRawReadEn)
     || (TRUE == ptNfcqEntry->bsBypassRdErr)
     || (TRUE == ptFlashDesc->bsMergeRdEn)
     || (INVALID_4F != ptReqEntry->atBufDesc[1].bsBufID))
    {
        return;
    }

    if (TRUE != ptFlashDesc->bsHostRdEn)
    {
        return;
    }

    if (2 != ptFlashDesc->bsPhyBlk % 30)
    {
        return;
    }

    ucIdx = 4 + (rand() % 100 == 0 ? 1 : 0);
    #ifdef LDPC_CENTER_RECORD
    l_aErrInjList[ucIdx][0] = NF_ERR_TYPE_UECC;
    #endif
    switch (l_aErrInjList[ucIdx][0])
    {
        case NF_ERR_TYPE_UECC:
        {
            FCMD_INTR_ERRH_ENTRY *ptErrHEntry;

            if (0 == ptCtrlEntry->bsPhyPage % 3)
            {
                ucInjErrTime = HAL_FlashGetMaxRetryTime(bTLCMode, ptCtrlEntry->bsPhyPage);
            }
            else if (1 == ptCtrlEntry->bsPhyPage % 30)
            {
                ucInjErrTime = HAL_FlashGetMaxRetryTime(bTLCMode, ptCtrlEntry->bsPhyPage) / 2;
            }
            else
            {
                break;
            }

            ptErrHEntry = ptCtrlEntry->ptErrHEntry;
            if (NULL == ptErrHEntry || ptErrHEntry->tUeccHCtrl.tRetry.bsTime < ucInjErrTime)
            {
                if (TRUE == ptFlashDesc->bsRdRedOnly)
                {
                    // default setting. refer to (secStart, secLen)
                    tErrInj.bsInjCwStart = 15;
                    tErrInj.bsInjCwLen = 1;
                    tErrInj.bsInjErrBitPerCw = ERR_BIT_PER_RED;
                }
                else
                {
                    tErrInj.bsInjCwStart = ptNfcqEntry->aSecAddr[0].bsSecStart / 2;
                    tErrInj.bsInjCwLen = (ptNfcqEntry->aSecAddr[0].bsSecLength + 1) / 2;  // Modify Min CwLen is 1.
                    tErrInj.bsInjErrBitPerCw = ERR_BIT_PER_CW;
                }

                tErrInj.bsInjErrType = NF_ERR_TYPE_UECC;
                HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);

                ptNfcqEntry->aSecAddr[1].bsSecStart = 0;
                ptNfcqEntry->aSecAddr[1].bsSecLength = 0;
            }

            break;
        }
        case NF_ERR_TYPE_RECC:
        {
            if (0 == ptCtrlEntry->bsPhyPage % 60)
            {
                tErrInj.bsInjErrType = NF_ERR_TYPE_RECC;
                HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);
            }

            break;
        }
        case NF_ERR_TYPE_LBA:
        {
            if (10 == ptCtrlEntry->bsPhyPage % 60)
            {
                tErrInj.bsInjErrType = NF_ERR_TYPE_LBA;
                HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);
            }

            break;
        }
        case NF_ERR_TYPE_DCRC:
        {
            if (20 == ptCtrlEntry->bsPhyPage % 60)
            {
                tErrInj.bsInjErrType = NF_ERR_TYPE_DCRC;
                HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);
            }

            break;
        }
        default:
        {
            DBG_Printf("Inj-Read-Err Type Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }
        
    return;
}

#else//abby add for test
/*==============================================================================
Func Name  : L3_DbgInjReadErr
Input      : NFCQ_ENTRY *ptNfcqEntry            
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgInjReadErr(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucErrType, ucInjErrTime;
    NFC_ERR_INJ tErrInj = {0};
    FCMD_FLASH_DESC *ptFlashDesc = &ptCtrlEntry->ptReqEntry->tFlashDesc;

    if (0 != ptCtrlEntry->ptReqEntry->bsTLun)
        return;
        
    *(volatile U32*)0x1ff81000 |= (0x1<<16); //enable RECC report
    tErrInj.bsInjCwStart = 0;
    tErrInj.bsInjCwLen = 64;    //1 CW for bug patch, more CW maybe not exact
    tErrInj.bsInjErrBitPerCw = 72;
    tErrInj.bsInjErrType = NF_ERR_TYPE_UECC;
    
    HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);

    ptNfcqEntry->aSecAddr[1].bsSecStart = 0;
    ptNfcqEntry->aSecAddr[1].bsSecLength = 0;

    /* DEC FIFO */
    LOCAL U8 l_ucDecFifoIdx = 0;
    l_ucDecFifoIdx++;

#if 1
    if (128 == l_ucDecFifoIdx)
    {
        /* Reset DEC FIFO module */
        HAL_DecFifoReset();
        l_ucDecFifoIdx = 0;
    }
#else
    if (TRUE == HAL_DecFifoIsFull())
    {
        /* Reset DEC FIFO module */
        HAL_DecFifoReset();
        l_ucDecFifoIdx = 0;
    }
#endif
    HAL_DecFifoTrigNfc(ptNfcqEntry, l_ucDecFifoIdx);
        
    return;
}
#endif

/*==============================================================================
Func Name  : L3_DbgInjWriteErr
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgInjWriteErr(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    U8 ucIdx;
    NFC_ERR_INJ tErrInj = {0};
    FCMD_FLASH_DESC *ptFlashDesc = &ptCtrlEntry->ptReqEntry->tFlashDesc;

    if ((3 == ptFlashDesc->bsPhyBlk % 20) && (110 == ptCtrlEntry->bsPhyPage))
    {
        ucIdx = 1;
        if (TRUE == ptCtrlEntry->ptReqEntry->bsPrePgEn)
        {
            ucIdx += rand()% 3;
        }

        tErrInj.bsInjErrSts = l_aErrInjList[ucIdx][1];
        tErrInj.bsInjErrType= l_aErrInjList[ucIdx][0];
        HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);
    }

    return;
}

/*==============================================================================
Func Name  : L3_IFInjEraseErr
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.8.12 JasonGuo create function
==============================================================================*/
LOCAL void L3_DbgInjEraseErr(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    NFC_ERR_INJ tErrInj = {0};
    FCMD_FLASH_DESC *ptFlashDesc = &ptCtrlEntry->ptReqEntry->tFlashDesc;

    if (4 == ptFlashDesc->bsPhyBlk % 15)
    {
        tErrInj.bsInjErrSts  = l_aErrInjList[0][1];
        tErrInj.bsInjErrType = l_aErrInjList[0][0];
        HAL_NfcSetErrInj(ptNfcqEntry, &tErrInj);
    }

    return;
}

/*==============================================================================
Func Name  : L3_DbgSetNFCQErrInj
Input      : NFCQ_ENTRY *ptNfcqEntry
             FCMD_INTR_CTRL_ENTRY *ptCtrlEntry
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
void L3_DbgSetNFCQErrInj(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry)
{
    FCMD_REQ_ENTRY *ptReqEntry = ptCtrlEntry->ptReqEntry;

    if (TRUE != ptReqEntry->bsBootUpOk || FCMD_REQ_SUBTYPE_SETFEATURE == ptCtrlEntry->ptReqEntry->bsReqSubType)
    {
        return;
    }

    switch (ptReqEntry->bsReqType)
    {
        case FCMD_REQ_TYPE_READ:
        {
            L3_DbgInjReadErr(ptNfcqEntry, ptCtrlEntry);
            break;
        }
        case FCMD_REQ_TYPE_WRITE:
        {
            //L3_DbgInjWriteErr(ptNfcqEntry, ptCtrlEntry);
            break;
        }
        case FCMD_REQ_TYPE_ERASE:
        {
            //L3_DbgInjEraseErr(ptNfcqEntry, ptCtrlEntry);
            break;
        }
        default:
        {
            DBG_Printf("Err Inject ReqType Error.0x%x\n", (U32)ptCtrlEntry);
            DBG_Getch();
        }
    }

    return;
}
#endif

/*====================End of this file========================================*/

