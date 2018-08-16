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
* File Name    : L3_Schedule.c
* Discription  :
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "FW_Event.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverExt.h"
#include "L3_Interface.h"
#include "L3_ErrHBasic.h"
#include "L3_ErrHExtend.h"
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
extern GLOBAL STS_BM_CNT *g_ptStatusBitmapCnt;
extern void L3_TrimReqProcess(void);

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/
LOCAL volatile U32 l_aTLunStsBmp[STS_BMP_NUM][NFC_LUN_MAX_PER_PU];
LOCAL volatile U32 l_aTLunArbBmp[ARB_BMP_NUM][NFC_LUN_MAX_PER_PU];
LOCAL volatile U8  l_aLunInPU[ARB_BMP_NUM];

/*==============================================================================
Func Name  : L3_SchInitStsBmp
Input      : void
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_SchInitStsBmp(void)
{
    U8 ucLunInPU;

    for (ucLunInPU = 0; ucLunInPU < NFC_LUN_MAX_PER_PU; ucLunInPU++)
    {
        l_aTLunStsBmp[STS_BMP_POPREQ][ucLunInPU] = 0;
        l_aTLunStsBmp[STS_BMP_PEND][ucLunInPU] = 0;
        l_aTLunStsBmp[STS_BMP_RECYCLE][ucLunInPU] = 0;
        l_aTLunStsBmp[STS_BMP_ERRH][ucLunInPU] = 0;
        l_aTLunStsBmp[STS_BMP_EXTH][ucLunInPU] = 0;
        l_aTLunStsBmp[STS_BMP_LOCK][ucLunInPU] = 0;
        l_aTLunStsBmp[STS_BMP_NFC_UNFULL][ucLunInPU] = INVALID_8F;
        l_aTLunStsBmp[STS_BMP_NFC_EMPTY][ucLunInPU] = INVALID_8F;
        l_aTLunStsBmp[STS_BMP_NFC_ERROR][ucLunInPU] = 0;
    }

    g_ptStatusBitmapCnt->ucErrHCnt = 0;
    g_ptStatusBitmapCnt->ucExtHCnt = 0;
    g_ptStatusBitmapCnt->ucPendCnt = 0;
    g_ptStatusBitmapCnt->ucRecycleCnt = 0;

    return;
}

/*==============================================================================
Func Name  : L3_SchGetPopReqStsBmp
Input      : U8 ucLunInPU
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
LOCAL U32 L3_SchGetPopReqStsBmp(U8 ucLunInPU)
{
    U8 ucPU, ucTLun;
    U32 ulPopReqBmp = 0;

    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        ucTLun = L3_GET_TLUN(ucPU, ucLunInPU);
        if (FCMD_REQ_STS_PUSH == L2_FCMDQGetReqSts(ucTLun, 0, L3_FCMDQGetReqRptr(ucTLun, 0)))
        {
            ulPopReqBmp |= 1 << ucPU;
        }
    }

    return ulPopReqBmp;
}

/*==============================================================================
Func Name  : L3_SchUpdateStsBmp
Input      : U8 ucLunInPU
             STS_BITMAP_TYPE eStsBMType
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_SchUpdateStsBmp(U8 ucLunInPU, STS_BITMAP_TYPE eStsBMType)
{
    U32 ulLunStsBitmap;

    if (STS_BMP_POPREQ == eStsBMType)
    {
        ulLunStsBitmap = L3_SchGetPopReqStsBmp(ucLunInPU);
    }
    else if (STS_BMP_NFC_UNFULL == eStsBMType)
    {
        ulLunStsBitmap = HAL_NfcGetLLunSwBitMap(ucLunInPU, LOGIC_LUN_BITMAP_NOTFULL);
    }
    else if (STS_BMP_NFC_ERROR == eStsBMType)
    {
        ulLunStsBitmap = HAL_NfcGetLLunSwBitMap(ucLunInPU, LOGIC_LUN_BITMAP_ERROR);
    }
    else if (STS_BMP_NFC_EMPTY == eStsBMType)
    {
        ulLunStsBitmap = HAL_NfcGetLLunSwBitMap(ucLunInPU, LOGIC_LUN_BITMAP_EMPTY);
    }
    else
    {
        DBG_Printf("Sts Bitmap Type %d Error.\n", eStsBMType);
        DBG_Getch();
    }

    l_aTLunStsBmp[eStsBMType][ucLunInPU] = ulLunStsBitmap;

    return;
}

/*==============================================================================
Func Name  : L3_SchGetStsBit
Input      : U8 ucTLun
             STS_BITMAP_TYPE eStsBmpType
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
BOOL L3_SchGetStsBit(U8 ucTLun, STS_BITMAP_TYPE eStsBmpType)
{
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);

    return (0==(l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1<<ucPU))) ? 0 : 1;
}

/*==============================================================================
Func Name  : L3_SchSetStsBit
Input      : U8 ucTLun
             STS_BITMAP_TYPE eStsBmpType
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_SchSetStsBit(U8 ucTLun, STS_BITMAP_TYPE eStsBmpType)
{
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);

    if (STS_BMP_RECYCLE == eStsBmpType)
    {
        ASSERT((l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1 << ucPU)) == 0);
        g_ptStatusBitmapCnt->ucRecycleCnt++;
    }
    else if (STS_BMP_PEND == eStsBmpType)
    {
        ASSERT((l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1 << ucPU)) == 0);
        g_ptStatusBitmapCnt->ucPendCnt++;
    }
    else if (STS_BMP_EXTH == eStsBmpType)
    {
        ASSERT((l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1 << ucPU)) == 0);
        g_ptStatusBitmapCnt->ucExtHCnt++;
    }

    l_aTLunStsBmp[eStsBmpType][ucLunInPU] |= 1 << ucPU;

    return;
}

/*==============================================================================
Func Name  : L3_SchClrStsBit
Input      : U8 ucTLun
             STS_BITMAP_TYPE eStsBmpType
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_SchClrStsBit(U8 ucTLun, STS_BITMAP_TYPE eStsBmpType)
{
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);

    if (STS_BMP_RECYCLE == eStsBmpType)
    {
        ASSERT((l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1 << ucPU)) != 0);
        g_ptStatusBitmapCnt->ucRecycleCnt--;
    }
    else if (STS_BMP_PEND == eStsBmpType)
    {
        ASSERT((l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1 << ucPU)) != 0);
        g_ptStatusBitmapCnt->ucPendCnt--;
    }
    else if (STS_BMP_EXTH == eStsBmpType)
    {
        ASSERT((l_aTLunStsBmp[eStsBmpType][ucLunInPU] & (1 << ucPU)) != 0);
        g_ptStatusBitmapCnt->ucExtHCnt--;
    }

    l_aTLunStsBmp[eStsBmpType][ucLunInPU] &= ~(1 << ucPU);

    return;
}

/*==============================================================================
Func Name  : L3_SchInitArbBmp
Input      : void
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
LOCAL void MCU2_DRAM_TEXT L3_SchInitArbBmp(void)
{
    U8 ucIdx;

    for (ucIdx = 0; ucIdx < ARB_BMP_NUM; ucIdx++)
    {
        l_aLunInPU[ucIdx] = 0;
    }

    COM_MemZero((U32*)&l_aTLunArbBmp[0][0], ARB_BMP_NUM*NFC_LUN_MAX_PER_PU);

    return;
}

/*==============================================================================
Func Name  : L3_SchUpdateArbBmp
Input      : ARB_BITMAP_TYPE eArbBMType
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
LOCAL void L3_SchUpdateArbBmp(ARB_BITMAP_TYPE eArbBMType)
{
    U8 ucLunInPU;
    U32 ulArbBitmap;

    for (ucLunInPU = 0; ucLunInPU < NFC_LUN_PER_PU; ucLunInPU++)
    {
        if (ARB_BMP_POPREQ == eArbBMType)
        {
            L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_POPREQ);
            L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_UNFULL);
            L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_ERROR);

            ulArbBitmap = l_aTLunStsBmp[STS_BMP_POPREQ][ucLunInPU]
                & (~l_aTLunStsBmp[STS_BMP_PEND][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_RECYCLE][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_ERRH][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_EXTH][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_LOCK][ucLunInPU])
                & (l_aTLunStsBmp[STS_BMP_NFC_UNFULL][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_NFC_ERROR][ucLunInPU]);
        }
        else if (ARB_BMP_PEND == eArbBMType)
        {
            L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_ERROR);

            ulArbBitmap = l_aTLunStsBmp[STS_BMP_PEND][ucLunInPU]
                & (~l_aTLunStsBmp[STS_BMP_NFC_ERROR][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_ERRH][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_LOCK][ucLunInPU]);
        }
        else if (ARB_BMP_RECYCLE == eArbBMType)
        {
            L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_EMPTY);

            ulArbBitmap = l_aTLunStsBmp[STS_BMP_NFC_EMPTY][ucLunInPU]
                & (l_aTLunStsBmp[STS_BMP_RECYCLE][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_ERRH][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_PEND][ucLunInPU]);
        }
        else if (ARB_BMP_ERRH == eArbBMType)
        {
            L3_SchUpdateStsBmp(ucLunInPU, STS_BMP_NFC_ERROR);

            ulArbBitmap = (l_aTLunStsBmp[STS_BMP_NFC_ERROR][ucLunInPU] | l_aTLunStsBmp[STS_BMP_ERRH][ucLunInPU])
                & (~l_aTLunStsBmp[STS_BMP_LOCK][ucLunInPU]);
        }
        else if (ARB_BMP_EXTH == eArbBMType)
        {
            ulArbBitmap = l_aTLunStsBmp[STS_BMP_EXTH][ucLunInPU];
        }
        else
        {
            DBG_Printf("ArbBitmap Type Error %d.\n", eArbBMType);
            DBG_Getch();
        }

        l_aTLunArbBmp[eArbBMType][ucLunInPU] = ulArbBitmap;
    }

    return;
}

/*==============================================================================
Func Name  : L3_SchClrArbBit
Input      : U8 ucTLun
             ARB_BITMAP_TYPE eArbBmpType
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_SchClrArbBit(U8 ucTLun, ARB_BITMAP_TYPE eArbBmpType)
{
    U8 ucPU = L3_GET_PU(ucTLun);
    U8 ucLunInPU = L3_GET_LUN_IN_PU(ucTLun);

    l_aTLunArbBmp[eArbBmpType][ucLunInPU] &= ~(1 << ucPU);

    return;
}

/*==============================================================================
Func Name  : L3_SchClrArbBits
Input      : U8 ucTLun
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_SchClrArbBits(U8 ucTLun)
{
    ARB_BITMAP_TYPE eArbBmpType;

    for (eArbBmpType = 0; eArbBmpType < ARB_BMP_NUM; eArbBmpType++)
    {
        L3_SchClrArbBit(ucTLun, eArbBmpType);
    }

    return;
}

/*==============================================================================
Func Name  : L3_SchSelPUAndLun
Input      : ARB_BITMAP_TYPE eArbBmpType
             U8 *pLunInPU
             U8 *pPU
Output     : NONE
Return Val : LOCAL
Discription: 1. each arbitrat type has a independent bitmap
             2. first, PU; second, LunInPU.
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
LOCAL void L3_SchSelPUAndLun(ARB_BITMAP_TYPE eArbBmpType, U8 *pLunInPU, U8 *pPU)
{
    l_aLunInPU[eArbBmpType] &= NFC_LUN_PER_PU - 1;

    do
    {
        *pPU = HAL_CLZ(l_aTLunArbBmp[eArbBmpType][l_aLunInPU[eArbBmpType]]);
        if (32 != *pPU)
        {
            *pLunInPU = l_aLunInPU[eArbBmpType];
            return;
        }

        l_aLunInPU[eArbBmpType]++;
    } while (l_aLunInPU[eArbBmpType] < NFC_LUN_PER_PU);

    return;
}

/*==============================================================================
Func Name  : L3_SchSelTLun
Input      : ARB_BITMAP_TYPE eArbBmpType
Output     : NONE
Return Val : LOCAL
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
LOCAL U8 L3_SchSelTLun(ARB_BITMAP_TYPE eArbBmpType)
{
    U8 ucPU, ucLunInPU, ucTLun;

    L3_SchSelPUAndLun(eArbBmpType, &ucLunInPU, &ucPU);
    if (32 == ucPU)
    {
        L3_SchUpdateArbBmp(eArbBmpType);
        L3_SchSelPUAndLun(eArbBmpType, &ucLunInPU, &ucPU);
    }

    if (32 != ucPU)
    {
        ucTLun = L3_GET_TLUN(31-ucPU, ucLunInPU);
        L3_SchClrArbBit(ucTLun, eArbBmpType);

        return ucTLun;
    }

    return INVALID_2F;
}


LOCAL U8 L3_SchSelTLun_1(ARB_BITMAP_TYPE eArbBmpType)
{
    U8 ucPU, ucLunInPU, ucTLun;

    L3_SchSelPUAndLun(eArbBmpType, &ucLunInPU, &ucPU);
    if (32 == ucPU)
    {
        L3_SchUpdateArbBmp(eArbBmpType);
        return INVALID_2F;
    }
    else
    {
        ucTLun = L3_GET_TLUN(31 - ucPU, ucLunInPU);
        L3_SchClrArbBit(ucTLun, eArbBmpType);

        return ucTLun;
    }
}

/*==============================================================================
Func Name  : L3_SchEventHandler
Input      : void
Output     : NONE
Return Val : LOCAL
Discription: Response the front-end event with the highest prority.
Usage      :
History    :
1. 2014.12.10 JasonGuo create function
==============================================================================*/
LOCAL U32 L3_SchEventHandler(void)
{
    COMMON_EVENT L3_Event;
    U32 Ret, ulResult;

    Ret = COMM_EVENT_STATUS_BLOCKING;

    ulResult = CommCheckEvent(COMM_EVENT_OWNER_L3, &L3_Event);
    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT == ulResult)
    {
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    if (TRUE == L3_Event.EventL3Trim)
    {
        //DBG_Printf("EventL3Trim\n");
        L3_TrimReqProcess();
        return COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }

    if (TRUE == L3_Event.EventIdle)
    {
        COMM_EVENT_PARAMETER * pParameter;

        if (FALSE == L3_IFIsAllFCmdIntrQEmpty() || FALSE == L2_FCMDQIsAllEmpty())
        {
            Ret = COMM_EVENT_STATUS_GET_EVENTPEND;
        }
        else
        {
            CommGetEventParameter(COMM_EVENT_OWNER_L3, &pParameter);
            if (FCMD_REQ_SUBTYPE_IDLE_2 == pParameter->EventParameterNormal[0])
            {
                /* Resets SGE for NVMe solution. */
                L3_SGEReset();

                /* Re-initializes DSG pool then. */
#ifdef VT3533_A2ECO_DSGRLSBUG
                HAL_NormalDsgFifoInit();
#endif

                DBG_Printf("L3 Handle Idle Error Recovery Done.\n");
            }

            g_pMCU12MiscInfo->bSubSystemIdle = TRUE;
            CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_IDLE);
            Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
    }
    else if (TRUE == L3_Event.EventShutDown)
    {
        if (FALSE == L3_IFIsAllFCmdIntrQEmpty())
        {
            Ret = COMM_EVENT_STATUS_GET_EVENTPEND;
        }
        else
        {
            CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_SHUTDOWN);
            Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
    }
    else if (TRUE == L3_Event.EventSelfTest)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_SELFTEST);
        g_pMCU12MiscInfo->bSubSystemSelfTestDone = TRUE;
        Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
    }
    else if (TRUE == L3_Event.EventDbg)
    {
        DBG_Printf("Not Support Debug Event.\n");
        ulResult = SUCCESS;
        if (SUCCESS == ulResult)
        {
            CommClearEvent(COMM_EVENT_OWNER_L3, COMM_EVENT_OFFSET_DBG);
            Ret = COMM_EVENT_STATUS_SUCCESS_NOEVENT;
        }
    }
    else
    {
        ;
    }

    return Ret;
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L3_SchInit
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.27 JasonGuo create function
==============================================================================*/
void L3_SchInit(void)
{
    L3_SchInitStsBmp();

    L3_SchInitArbBmp();

    return;
}

/*==============================================================================
Func Name  : L3_Scheduler
Input      : void
Output     : NONE
Return Val :
Discription:
Usage      :
History    :
    1. 2016.7.15 JasonGuo create function
==============================================================================*/
void L3_Scheduler(void)
{
    U8 ucTLun;

    //////////////////////////////////////////////////////////////////////////////////////
    // Response the highest prority system event from the higher layer.
    //////////////////////////////////////////////////////////////////////////////////////
    if (COMM_EVENT_STATUS_BLOCKING == L3_SchEventHandler())
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // Pop one fcmd from the FCMDQ and trigger it to the NFC.                           //
    // There are the following points which need to be noticed:                         //
    // 1. The order of FCMDQ selection: CH -> PPU -> CE -> LUN -> HighPriQ -> LowPriQ,  //
    // this can make the wholechip to get the highest performance.                      //
    // 2. There are two kinds of fcmd, the one is need to trigger nfc, another will not //
    // trigger nfc.                                                                     //
    //////////////////////////////////////////////////////////////////////////////////////
    while (TRUE)
    {
        ucTLun = L3_SchSelTLun_1(ARB_BMP_POPREQ);
        if (INVALID_2F != ucTLun)
        {
            L3_IFPopFCmdQ(ucTLun);
        }
        else
        {
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // The fcmd needs redo or multi-step execution, which needs L3 handle it;           //
    //////////////////////////////////////////////////////////////////////////////////////
    if (0 != g_ptStatusBitmapCnt->ucPendCnt)
    {
        ucTLun = L3_SchSelTLun_1(ARB_BMP_PEND);
        if (INVALID_2F != ucTLun)
        {
            L3_PCmdHandling(ucTLun);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // Update the fcmd request result manually in L3.                                   //
    // It includes the following requests:                                              //
    // 1. The request which needs check the process status, the status address can't be //
    // updated auto by nfc;                                                             //
    // 2. The request which needs check the red info, and the red-info address can't be //
    // updated auto by nfc;                                                             //
    //////////////////////////////////////////////////////////////////////////////////////
    if (0 != g_ptStatusBitmapCnt->ucRecycleCnt)
    {
        ucTLun = L3_SchSelTLun_1(ARB_BMP_RECYCLE);
        if (INVALID_2F != ucTLun)
        {
            L3_IFRecycleFCmdQ(ucTLun);
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////
    // The fcmd generates an error, which needs L3 to do the error handling;            //
    //////////////////////////////////////////////////////////////////////////////////////
    {
        ucTLun = L3_SchSelTLun_1(ARB_BMP_ERRH);
        if (INVALID_2F != ucTLun)
        {
            L3_ErrHHandling(ucTLun);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // Does L3 need to do some extend process after the error handling?                 //
    //////////////////////////////////////////////////////////////////////////////////////
    if (0 != g_ptStatusBitmapCnt->ucExtHCnt)
    {
        ucTLun = L3_SchSelTLun_1(ARB_BMP_EXTH);
        if (INVALID_2F != ucTLun)
        {
            L3_ExtHHandling(ucTLun);
        }
    }

    return;
}

/*====================End of this file========================================*/

