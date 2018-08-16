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
* File Name    : L3_FCMDQ.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.2.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "COM_Memory.h"
#include "HAL_Xtensa.h"
#include "HAL_MemoryMap.h"
#include "L3_FlashMonitor.h"
#include "L3_FCMDQ.h"
#include "L3_Schedule.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
GLOBAL STS_BM_CNT *g_ptStatusBitmapCnt;
/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
FCMD_INTR_CTRL *g_ptFCmdIntrCtrl; // Allocated from Dsram0-mcu2
FCMD_INTR_ERRH *g_ptFCmdIntrErrH; // Allocated from Dram/Dsram0-mcu2
FCMD_INTR_DPTR *g_ptFCmdIntrDptr; // Allocated from Dsram0-mcu2

FCMD_REQ_ENTRY *g_ptFCmdReqBak2;
FCMD_REQ_ENTRY *g_ptFCmdReqBak;
FCMD_INTR_CTRL *g_ptFCmdIntrCtrlBak; // Allocated from Dram/Dsram0-mcu2
FCMD_INTR_ERRH *g_ptFCmdIntrErrHBak; // Allocated from Dram/Dsram0-mcu2
FCMD_INTR_DPTR *g_ptFCmdIntrDptrBak; // Allocated from Dram/Dsram0-mcu2

/*==============================================================================
Func Name  : L3_FCmdIntrCtrlInit
Input      : BOOL bIsBak
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_FCmdIntrCtrlInit(BOOL bIsBak)
{
    U32 ulDWSize;
    FCMD_INTR_CTRL *ptCtrl;

    if (TRUE != bIsBak)
    {
        ptCtrl   = g_ptFCmdIntrCtrl;
        ulDWSize = FCMD_INTR_CTRL_SZ >> DWORD_SIZE_BITS;
    }
    else
    {
        ptCtrl   = g_ptFCmdIntrCtrlBak;
        ulDWSize = FCMD_INTR_CTRL_SZ_BAK/sizeof(U32);
    }

    COM_MemZero((U32*)ptCtrl, ulDWSize);

    return;
}

/*==============================================================================
Func Name  : L3_FCmdIntrErrHInit
Input      : BOOL bIsBak
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_FCmdIntrErrHInit(BOOL bIsBak)
{
    U32 ulDWSize;
    FCMD_INTR_ERRH *ptErrH;

    if (TRUE != bIsBak)
    {
        ptErrH   = g_ptFCmdIntrErrH;
        ulDWSize = FCMD_INTR_ERRH_SZ >> DWORD_SIZE_BITS;
    }
    else
    {
        ptErrH   = g_ptFCmdIntrErrHBak;
        ulDWSize = FCMD_INTR_ERRH_SZ_BAK >> DWORD_SIZE_BITS;
    }

    COM_MemZero((U32*)ptErrH, ulDWSize);

    return;
}

/*==============================================================================
Func Name  : L3_FCmdIntrDptrInit
Input      : void
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_FCmdIntrDptrInit(BOOL bIsBak)
{
    U32 ulTLun, ulDepthIdx, ulTNum;
    FCMD_INTR_DPTR *ptCurDptr;
    FCMD_INTR_Q_DPTR *ptQDptr;

    if (TRUE != bIsBak)
    {
        ulTNum = SUBSYSTEM_LUN_NUM;
        ptCurDptr = g_ptFCmdIntrDptr;
    }
    else
    {
        ulTNum = FCMD_INTR_BAK_NUM;
        ptCurDptr = g_ptFCmdIntrDptrBak;
    }

    for (ulTLun = 0; ulTLun < ulTNum; ulTLun++)
    {
        ptQDptr = &ptCurDptr->atIntrQDptr[ulTLun];
        for (ulDepthIdx = 0; ulDepthIdx < NFCQ_DEPTH; ulDepthIdx++)
        {
            ptQDptr->ucWptr = 0;
            ptQDptr->ucRptr = INVALID_DPTR;
            ptQDptr->ucPptr = INVALID_DPTR;
            ptQDptr->ucEptr = INVALID_DPTR;
        }
    }

    return;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_FCMDQAllocSRAM0
Input      : U32 *pFreeSram0Base
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FCMDQAllocSRAM0(U32 *pFreeSram0Base)
{
    U32 ulFreeBase = *pFreeSram0Base;
    COM_MemAddr16DWAlign(&ulFreeBase);

    // FCMD_INTR_DPTR
    g_ptFCmdIntrDptr = (FCMD_INTR_DPTR *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_INTR_DPTR_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

    // FCMD_INTR_CTRL
    g_ptFCmdIntrCtrl = (FCMD_INTR_CTRL *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, FCMD_INTR_CTRL_SZ);
    COM_MemAddr16DWAlign(&ulFreeBase);

    // StatusBitmapCnt
    g_ptStatusBitmapCnt = (STS_BM_CNT *)ulFreeBase;
    COM_MemIncBaseAddr(&ulFreeBase, sizeof(STS_BM_CNT));
    COM_MemAddr16DWAlign(&ulFreeBase);

    DBG_Printf("L3: FCMDQAllcSRAM0 %dKB, -> %dKB -> totalSRAM0 %dKB\n", (ulFreeBase - *pFreeSram0Base) / 1024, (ulFreeBase - DSRAM0_MCU2_BASE) / 1024, DSRAM0_MCU2_MAX_SIZE / 1024);
    ASSERT(ulFreeBase-DSRAM0_MCU2_BASE < DSRAM0_MCU2_MAX_SIZE);
    *pFreeSram0Base = ulFreeBase;

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQIntrInit
Input      : BOOL bIsBak
Output     : NONE
Return Val :
Discription:
Usage      : Called in L3_TaskInit after the memory allocation done.
History    :
    1. 2016.7.6 JasonGuo create function
==============================================================================*/
void MCU2_DRAM_TEXT L3_FCMDQIntrInit(BOOL bIsBak)
{
    L3_FCmdIntrCtrlInit(bIsBak);
    L3_FCmdIntrErrHInit(bIsBak);
    L3_FCmdIntrDptrInit(bIsBak);

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQGetReqRptr
Input      : U8 ucTLun
             FCMD_REQ_PRI eFCmdPri
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
U8 L3_FCMDQGetReqRptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    return g_ptFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucRptr;
}

/*==============================================================================
Func Name  : L3_FCMDQSetReqRptr
Input      : U8 ucTLun
             FCMD_REQ_PRI eFCmdPri
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
void L3_FCMDQSetReqRptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucRptr)
{
    g_ptFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucRptr = ucRptr;
    //FIRMWARE_LogInfo("TLun#%d MCU%d Wptr#%d.Rptr#%d\n", ucTLun, HAL_GetMcuId(), g_ptFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucWptr, ucRptr);
}

LOCAL BOOL L3_IFNeedSwithWorkMod(FCMD_REQ_ENTRY *ptReqEntry)
{
    U8 ucTLun = ptReqEntry->bsTLun;
    U8 ucBlkMod = ptReqEntry->tFlashDesc.bsBlkMod;
    BOOL bSLCMode = FALSE, bNeedSw = FALSE;

    if (FCMD_REQ_SLC_BLK == ucBlkMod)
    {
        bSLCMode = TRUE;
    }

    if (bSLCMode != L3_FMGetSLCMode(ucTLun))
    {
        L3_FMSetSLCMode(ucTLun, bSLCMode);
        bNeedSw = TRUE;
    }

    return bNeedSw;
}

/*==============================================================================
Func Name  : L3_FCMDQPopReqEntry
Input      : U8 ucTLun
             FCMD_REQ_PRI eFCmdPri
Output     : NONE
Return Val : FCMD_REQ_ENTRY
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
FCMD_REQ_ENTRY *L3_FCMDQPopReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucRptr;
    FCMD_REQ_ENTRY *ptReqEntry;

    ucRptr = L3_FCMDQGetReqRptr(ucTLun, eFCmdPri);
    ptReqEntry = L2_FCMDQGetReqEntry(ucTLun, eFCmdPri, ucRptr);
    //ASSERT(ucRptr == ptReqEntry->bsReqPtr);

#ifndef SWITCH_MODE_DADF
    if (TRUE == L3_IFNeedSwithWorkMod(ptReqEntry))
    {
        //DBG_Printf("Swith Mode Opeartion.\n");
        return L3_FCMDQAllocTmp2ReqEntry(ucTLun);
    }
#endif

    ASSERT(FCMD_REQ_STS_PUSH == L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucRptr));
    L2_FCMDQSetReqSts(ucTLun, eFCmdPri, ucRptr, FCMD_REQ_STS_POP);

    L3_FCMDQSetReqRptr(ucTLun, eFCmdPri, (ucRptr+1)%FCMDQ_DEPTH);

    return ptReqEntry;
}

/*==============================================================================
Func Name  : L3_FCMDQGetIntrWptr
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
U8 L3_FCMDQGetIntrWptr(U8 ucTLun, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;

    return ptDptr->atIntrQDptr[ucTLun].ucWptr;
}

/*==============================================================================
Func Name  : L3_FCMDQSetIntrWptr
Input      : U8 ucTLun
             U8 ucWptr
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_FCMDQSetIntrWptr(U8 ucTLun, U8 ucWptr, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;
    ptDptr->atIntrQDptr[ucTLun].ucWptr = ucWptr;

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQGetIntrRptr
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
U8 L3_FCMDQGetIntrRptr(U8 ucTLun, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;

    return ptDptr->atIntrQDptr[ucTLun].ucRptr;
}

/*==============================================================================
Func Name  : L3_FCMDQSetIntrRptr
Input      : U8 ucTLun
             U8 ucRptr
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_FCMDQSetIntrRptr(U8 ucTLun, U8 ucRptr, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;
    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;

#ifdef SIM
    ASSERT(INVALID_DPTR == ucRptr || NFCQ_DEPTH > ucRptr);
    ASSERT(INVALID_DPTR == ucRptr || INVALID_DPTR == ptDptr->atIntrQDptr[ucTLun].ucRptr || ucRptr == ptDptr->atIntrQDptr[ucTLun].ucRptr);
    //FIRMWARE_LogInfo("TLun#%d IntrRptr#0x%x 0x%x\n", ucTLun, ptDptr->atIntrQDptr[ucTLun].ucRptr, ucRptr);
#endif

    ptDptr->atIntrQDptr[ucTLun].ucRptr = ucRptr;

    if (NFCQ_DEPTH > ucRptr)
    {
        if (L3_SchGetStsBit(ucTLun, STS_BMP_RECYCLE) == FALSE) //dont set Recycle again
        {
            L3_SchSetStsBit(ucTLun, STS_BMP_RECYCLE);

            L3_SchClrArbBit(ucTLun, ARB_BMP_POPREQ);
        }
    }
    else
    {
        if (L3_SchGetStsBit(ucTLun, STS_BMP_RECYCLE) == TRUE) //dont clear Recycle again
        {
            L3_SchClrStsBit(ucTLun, STS_BMP_RECYCLE);
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQGetIntrPptr
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
U8 L3_FCMDQGetIntrPptr(U8 ucTLun, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;

    return ptDptr->atIntrQDptr[ucTLun].ucPptr;
}

/*==============================================================================
Func Name  : L3_FCMDQSetIntrPptr
Input      : U8 ucTLun
             U8 ucPptr
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_FCMDQSetIntrPptr(U8 ucTLun, U8 ucPptr, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    //ASSERT(INVALID_DPTR == ucPptr || NFCQ_DEPTH > ucPptr);

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;

    //FIRMWARE_LogInfo("TLun#%d IntrPptr#0x%x 0x%x\n", ucTLun, ptDptr->atIntrQDptr[ucTLun].ucPptr, ucPptr);
    ptDptr->atIntrQDptr[ucTLun].ucPptr = ucPptr;

    if (NFCQ_DEPTH > ucPptr)
    {
        if (L3_SchGetStsBit(ucTLun, STS_BMP_PEND) == FALSE) //dont set pending again
        {
            L3_SchSetStsBit(ucTLun, STS_BMP_PEND);

            L3_SchClrArbBit(ucTLun, ARB_BMP_POPREQ);
            L3_SchClrArbBit(ucTLun, ARB_BMP_RECYCLE);
        }
    }
    else
    {
        if (L3_SchGetStsBit(ucTLun, STS_BMP_PEND) == TRUE) //dont clear pending again
        {
            L3_SchClrStsBit(ucTLun, STS_BMP_PEND);
        }
    }

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQGetIntrEptr
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
U8 L3_FCMDQGetIntrEptr(U8 ucTLun, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;

    return ptDptr->atIntrQDptr[ucTLun].ucEptr;
}

/*==============================================================================
Func Name  : L3_FCMDQSetIntrEptr
Input      : U8 ucTLun
             U8 ucEptr
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_FCMDQSetIntrEptr(U8 ucTLun, U8 ucEptr, BOOL bBak)
{
    FCMD_INTR_DPTR *ptDptr;

    //ASSERT(INVALID_DPTR == ucEptr || NFCQ_DEPTH > ucEptr);

    ptDptr = (FALSE == bBak) ? g_ptFCmdIntrDptr : g_ptFCmdIntrDptrBak;
    ptDptr->atIntrQDptr[ucTLun].ucEptr = ucEptr;

    if (NFCQ_DEPTH > ucEptr)
    {
        L3_SchSetStsBit(ucTLun, STS_BMP_ERRH);

        L3_SchClrArbBit(ucTLun, ARB_BMP_POPREQ);
        L3_SchClrArbBit(ucTLun, ARB_BMP_PEND);
        L3_SchClrArbBit(ucTLun, ARB_BMP_RECYCLE);
    }
    else
    {
        L3_SchClrStsBit(ucTLun, STS_BMP_ERRH);
    }

    return;
}

/*==============================================================================
Func Name  : L3_FCMDQAllocIntrEntry
Input      : U8 ucTLun
Output     : NONE
Return Val : FCMD_INTR_CTRL_ENTRY
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
FCMD_INTR_CTRL_ENTRY *L3_FCMDQAllocIntrEntry(U8 ucTLun)
{
    U8 ucWptr;
    FCMD_INTR_CTRL_ENTRY *ptCtrlEntry;

    ucWptr = L3_FCMDQGetIntrWptr(ucTLun, FALSE);
    ptCtrlEntry = L3_FCMDQGetIntrCtrlEntry(ucTLun, ucWptr, FALSE);
    COM_MemZero((U32*)ptCtrlEntry, sizeof(FCMD_INTR_CTRL_ENTRY) >> DWORD_SIZE_BITS);

    ptCtrlEntry->bsCtrlPtr = ucWptr;
    ptCtrlEntry->tDTxCtrl.bsFstEngineID = INVALID_4F;

    L3_FCMDQSetIntrWptr(ucTLun, (ucWptr + 1) % NFCQ_DEPTH, FALSE);

    return ptCtrlEntry;
}

/*==============================================================================
Func Name  : L3_FCMDQGetIntrCtrlEntry
Input      : U8 ucTLun
             U8 ucLevel
Output     : NONE
Return Val : FCMD_INTR_CTRL_ENTRY
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
FCMD_INTR_CTRL_ENTRY *L3_FCMDQGetIntrCtrlEntry(U8 ucTLun, U8 ucLevel, BOOL bBak)
{
    FCMD_INTR_CTRL *ptCtrl;

    //ASSERT(ucLevel < NFCQ_DEPTH);

    ptCtrl = (FALSE==bBak) ? g_ptFCmdIntrCtrl : g_ptFCmdIntrCtrlBak;

    return &ptCtrl->atCtrlQ[ucTLun].atCtrl[ucLevel];
}

/*==============================================================================
Func Name  : L3_FCMDQAllocIntrErrHEntry
Input      : U8 ucTLun
             BOOL bBak
Output     : NONE
Return Val : FCMD_INTR_ERRH_ENTRY
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
FCMD_INTR_ERRH_ENTRY *L3_FCMDQAllocIntrErrHEntry(U8 ucTLun, BOOL bBak)
{
    FCMD_INTR_ERRH *ptErrH;
    FCMD_INTR_ERRH_ENTRY *ptErrHEntry;

    ptErrH = (FALSE == bBak) ? g_ptFCmdIntrErrH : g_ptFCmdIntrErrHBak;

    ptErrHEntry = &ptErrH->atErrHQ[ucTLun];
    COM_MemZero((U32*)ptErrHEntry, sizeof(FCMD_INTR_ERRH_ENTRY) >> DWORD_SIZE_BITS);

    return ptErrHEntry;
}

/*==============================================================================
Func Name  : L3_FCMDQGetIntrErrHEntry
Input      : U8 ucTLun
Output     : NONE
Return Val : FCMD_INTR_ERRH_ENTRY
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
FCMD_INTR_ERRH_ENTRY *L3_FCMDQGetIntrErrHEntry(U8 ucTLun, BOOL bBak)
{
    FCMD_INTR_ERRH *ptErrH;

    ptErrH = (FALSE == bBak) ? g_ptFCmdIntrErrH : g_ptFCmdIntrErrHBak;

    return &ptErrH->atErrHQ[ucTLun];
}

/*==============================================================================
Func Name  : L3_FCMDQArbReqPri
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      : pop the new request from the high/low priority queue. arbitration.
             arbitration solution:
             high:low = 3:1
History    :
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
FCMD_REQ_PRI L3_FCMDQArbReqPri(U8 ucTLun)
{
#if 0
    S8 cArbReqPri;
    FCMD_REQ_PRI eFCmdPri;

    // now, default to pop the high priorty queue firstly.
    for (cArbReqPri = FCMD_REQ_PRI_NUM-1; cArbReqPri >= 0; cArbReqPri--)
    {
        if (FCMD_REQ_STS_PUSH == L2_FCMDQGetReqSts(ucTLun, cArbReqPri, L3_FCMDQGetReqRptr(ucTLun, cArbReqPri)))
        {
            eFCmdPri = cArbReqPri;
            break;
        }
    }
    ASSERT(cArbReqPri >= 0);

    return eFCmdPri;
#else
    return 0;
#endif
}

/*==============================================================================
Func Name  : L3_FCMDQAllocTmpReqEntry
Input      : U8 ucTLun
Output     : NONE
Return Val : FCMD_REQ_ENTRY
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
FCMD_REQ_ENTRY *L3_FCMDQAllocTmpReqEntry(U8 ucTLun)
{
    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = &g_ptFCmdReqBak[ucTLun];
    COM_MemZero((U32*)ptReqEntry, sizeof(FCMD_REQ_ENTRY) >> DWORD_SIZE_BITS);

    ptReqEntry->bsTLun = ucTLun;
    ptReqEntry->bsTBRebuilding = 0;
    ptReqEntry->bsBootUpOk = TRUE;

    return ptReqEntry;
}

FCMD_REQ_ENTRY *L3_FCMDQAllocTmp2ReqEntry(U8 ucTLun)
{
    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = &g_ptFCmdReqBak2[ucTLun];
    COM_MemZero((U32*)ptReqEntry, sizeof(FCMD_REQ_ENTRY) >> DWORD_SIZE_BITS);

    ptReqEntry->bsTLun = ucTLun;
    ptReqEntry->bsTBRebuilding = 0;
    ptReqEntry->bsBootUpOk = TRUE;

    ptReqEntry->bsReqType = FCMD_REQ_TYPE_SETFEATURE;
    ptReqEntry->bsReqSubType = FCMD_REQ_SUBTYPE_SETFEATURE;
    ptReqEntry->ulReqStsAddr = 0x91;
    //ptReqEntry->ulSpareAddr = (TRUE == L3_FMGetSLCMode(ucTLun)) ? 0 : 4;
    ptReqEntry->ulSpareAddr = (TRUE == L3_FMGetSLCMode(ucTLun)) ? 0x100 : 0x104;//dannier set to non-scrash addressing mode.

    return ptReqEntry;
}

/*==============================================================================
Func Name  : L3_FCMDQGetTmpReqEntry
Input      : U8 ucTLun
Output     : NONE
Return Val : FCMD_REQ_ENTRY
Discription:
Usage      :
History    :
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
FCMD_REQ_ENTRY *L3_FCMDQGetTmpReqEntry(U8 ucTLun)
{
    return &g_ptFCmdReqBak[ucTLun];
}

FCMD_REQ_ENTRY *L3_FCMDQGetTmp2ReqEntry(U8 ucTLun)
{
    return &g_ptFCmdReqBak2[ucTLun];
}


/*====================End of this file========================================*/

