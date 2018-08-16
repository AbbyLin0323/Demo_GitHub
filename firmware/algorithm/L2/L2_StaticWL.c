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
Filename    :L2_StaticWL.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.03.31
Description :functions about Static Wear Leveling

Others      :
Modify      :
*******************************************************************************/
#include "BaseDef.h"
#include "L2_StaticWL.h"
#include "L2_PMTPage.h"
#include "L2_Thread.h"
#include "FW_Event.h"
#include "L2_RT.h"
#include "L2_TableBlock.h"
#include "L2_FTL.h"
#include "L1_GlobalInfo.h"
#include "HAL_TraceLog.h"
#include "COM_BitMask.h"
#include "L2_Interface.h"
#include "L2_TLCMerge.h"
#include "L2_ErrorHandling.h"
#include "L2_TableBBT.h"

#if ((defined(L2MEASURE) || defined(SWL_EVALUATOR)) && ((!defined(SWL_OFF))))
#include "L2_Evaluater.h"
#include <windows.h>
#define ECMAX  300

LOCAL U16 l_usECMaxBlkNum = 0;
#endif

//specify file name for Trace Log
#define TL_FILE_NUM  L2_StaticWL_c


GLOBAL  U32 g_WLTooColdThs = 5;
GLOBAL  U32 g_WLEraseCntThs = 20;
GLOBAL  U32 g_WLAccDegree = 0;

GLOBAL  LC lc;
GLOBAL  WL_INFO *gwl_info;

extern GLOBAL  BOOL bShutdownPending;
extern GLOBAL  LC lc;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;
extern GLOBAL MCU12_VAR_ATTR TLCMerge *g_TLCManager;

#ifdef NEW_SWL
extern GLOBAL TLCGCSrcBlkRecord *g_pTLCGCSrcBlkRecord;
#endif

GLOBAL MCU12_VAR_ATTR BOOL gbWLTraceRec;

extern BOOL L2_HasWaitIdleEvent();

void MCU1_DRAM_TEXT L2_SWLInit(void)
{
    U8 i = 0;
    U16 j = 0;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        L2_SWLClear(i, FALSE);
        L2_InitCommonInfo(i, &gwl_info->tSWLCommon);
        gwl_info->nDstBlk[i] = INVALID_4F;
        gwl_info->nDstPPO[i] = 0;
        gwl_info->nDstBlkBuf[i] = INVALID_4F;
        gwl_info->bTooCold[i] = FALSE;
        gwl_info->nOTimes[i] = 0;

        pPBIT[i]->ulECMin = INVALID_4F;
        pPBIT[i]->ulECMinCnt = 0;
#ifdef SWL_EVALUATOR
        gwl_info->ulSelSrcECminCnt[i] = 0;
#endif
    }
    gbWLTraceRec = FALSE;
    g_WLAccDegree = 0;

    for (i = 0; i < SUBSYSTEM_SUPERPU_NUM; i++)
    {
        for (j = 0; j < ALL_INFO; j++)
        {
            lc.uLockCounter[i][j] = 0;
        }
    }
}

void L2_SWLUnlockTargetBlkBuf(U8 ucSuperPu)
{
    if (INVALID_4F != gwl_info->nDstBlkBuf[ucSuperPu])
    {
        L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlkBuf[ucSuperPu], FALSE);
        gwl_info->nDstBlkBuf[ucSuperPu] = INVALID_4F;
#ifdef DBG_LC
        lc.uLockCounter[ucSuperPu][STATIC_WL]--;
#endif
    }
    return;
}


void MCU1_DRAM_TEXT L2_SWLClear(U8 ucSuperPu, BOOL bInit)
{

    if (bInit)
    {
        U32 PosPhy;
        if (INVALID_4F != gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu]) //TLC SWL copyvalid
        {
            L2_Set_PBIT_BlkSN(ucSuperPu, gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu]);
            PosPhy = L2_VBT_GetPhysicalBlockAddr(ucSuperPu, 0, gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu]);
            }

        if (TRUE == gbWLTraceRec)
        {
            TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "Update SerialNumber PU ? ");
            TRACE_LOG((void*)&gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu], sizeof(U16), U16, 0, "VBN ? ");
            TRACE_LOG((void*)&PosPhy, sizeof(U32), U32, 0, "PBN ? ");
        }

    }

    /* common information */
    gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
    gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu] = INVALID_4F;
    gwl_info->tSWLCommon.m_SrcPrefix[ucSuperPu] = INVALID_2F;
    gwl_info->tSWLCommon.m_SrcOffset[ucSuperPu] = INVALID_4F;
    gwl_info->tSWLCommon.m_FinishBitMap[ucSuperPu] = 0;
    gwl_info->tSWLCommon.m_ErrLun[ucSuperPu] = INVALID_2F;
    gwl_info->tSWLCommon.m_ErrStage[ucSuperPu] = TLC_ERRH_ALL;
    gwl_info->tSWLCommon.m_GoodLunFinish[ucSuperPu] = FALSE;
    gwl_info->tSWLCommon.m_FailLunBitMap[ucSuperPu] = 0;

    gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;

    gwl_info->bEnable[ucSuperPu] = TRUE;

    gwl_info->bForbidSpeedUp[ucSuperPu] = FALSE;
}

void L2_CalcECMinAndCnt(U8 ucSuperPu, U8 ucLunInSuperPu, U8 bTLC, U8 ucIsGetECMin)
{
    U16 uBlkVBN = INVALID_4F;
    U16 uVirBlk = 0;
    U16 uBlkPhy = 0;
    U32 MinEraseCnt = INVALID_8F;
    PBIT_ENTRY* pPBIT_Entry;

    pPBIT_Entry = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu];
    for (uBlkPhy = g_ulDataBlockStart[ucSuperPu][ucLunInSuperPu]; uBlkPhy < BLK_PER_LUN + RSVD_BLK_PER_LUN; uBlkPhy++)
    {
        uVirBlk = pPBIT_Entry[uBlkPhy].VirtualBlockAddr;
        if (uVirBlk == INVALID_4F)
            continue;

        if (bTLC != pVBT[ucSuperPu]->m_VBT[uVirBlk].bTLC)
        {
            continue;
        }

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bError == TRUE)
            continue;

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bTable)
            continue;

        if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bBackup)
            continue;

        if (TRUE == pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][uBlkPhy].bReserved)
            continue;

        if (ucIsGetECMin)
        {
            if (pPBIT_Entry[uBlkPhy].EraseCnt < pPBIT[ucSuperPu]->ulECMin)
            {
                pPBIT[ucSuperPu]->ulECMin = pPBIT_Entry[uBlkPhy].EraseCnt;
            }
        }
        else
        {
            if (pPBIT_Entry[uBlkPhy].EraseCnt == pPBIT[ucSuperPu]->ulECMin)
            {
                pPBIT[ucSuperPu]->ulECMinCnt++;
            }
        }
    }
}

void L2_ECMinInfoCheck(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN)
{
    U32 ulPhyBlockAddr = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];

    if (pPBIT[ucSuperPu]->ulECMin == INVALID_4F)
    {
        L2_CalcECMinAndCnt(ucSuperPu, ucLunInSuperPu, TRUE, TRUE);
        L2_CalcECMinAndCnt(ucSuperPu, ucLunInSuperPu, TRUE, FALSE);
    }
    else if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][ulPhyBlockAddr].EraseCnt == (pPBIT[ucSuperPu]->ulECMin + 1))
    {
        pPBIT[ucSuperPu]->ulECMinCnt--;
        if (0 == pPBIT[ucSuperPu]->ulECMinCnt)
        {
            pPBIT[ucSuperPu]->ulECMin++;
            L2_CalcECMinAndCnt(ucSuperPu, ucLunInSuperPu, TRUE, FALSE);
        }
    }

    if (pPBIT[ucSuperPu]->ulECMinCnt > TLC_BLK_CNT)
    {
        DBG_Printf("Error! Wrong TLCECMinBlkCnt\n");
        DBG_Getch();
    }
}

void L2_IsNeedWL(U32 EraseCnt, U8 ucSuperPu, U8 ucLunInSuperPu, U32 uVBN)
{
    COMMON_EVENT L2_Event;
    CommCheckEvent(COMM_EVENT_OWNER_L2, &L2_Event);
#if  defined(SWL_EVALUATOR)

    if (EraseCnt == ECMAX)
    {
        l_usECMaxBlkNum++;
        DBG_DumpPBITEraseCnt();
        DBG_DumpSWLRecordInfo();
        
        if ((TLC_BLK_CNT * 7 / 100) == l_usECMaxBlkNum)
        exit(0);
        
    }

#endif

#ifdef SWL_OFF
    return;
#endif
#ifdef NEW_SWL
    U32 AverageErase;
    if (0 == ucLunInSuperPu)
    {
        // update the related info about ECmin used for judge SWL trigger condition
        L2_ECMinInfoCheck(ucSuperPu, ucLunInSuperPu, uVBN);
    }
#endif

#ifdef SHUTDOWN_IMPROVEMENT_STAGE2
    if (TLC_WRITE_SWL == g_TLCManager->aeTLCWriteType[ucSuperPu])
    {
        return;
    }
#endif

    if ((TRUE == bShutdownPending) || ((TRUE == L2_Event.EventShutDown)))
    {
        return;
    }

    //need double check
    if (0 != ucLunInSuperPu)
    {
        return;
    }
#ifdef NEW_SWL
    U16 PhyAddr;

    PhyAddr = pVBT[ucSuperPu]->m_VBT[uVBN].PhysicalBlockAddr[ucLunInSuperPu];
    AverageErase = (pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU) / TLC_BLK_CNT;
    //step one: whether the erased blk meet the requirment to trigger swl or not

    if (EraseCnt - pPBIT[ucSuperPu]->ulECMin < SWL_TH_MAX2MIN)
    {
        if ((gwl_info->nDstBlkBuf[ucSuperPu] == INVALID_4F) && (gwl_info->nDstBlk[ucSuperPu] == INVALID_4F))
        {
            return;
        }
    }
    else
    {
        if (gwl_info->nDstBlkBuf[ucSuperPu] == INVALID_4F)
        {
            gwl_info->nDstBlkBuf[ucSuperPu] = uVBN;
            L2_PBIT_Set_Lock(ucSuperPu, uVBN, TRUE);
        }
        else
        {
            PhyAddr = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlkBuf[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];

            if (EraseCnt > pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyAddr].EraseCnt)
            {
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlkBuf[ucSuperPu], FALSE);
                L2_PBIT_Set_Lock(ucSuperPu, uVBN, TRUE);
                gwl_info->nDstBlkBuf[ucSuperPu] = uVBN;
            }
        }
    }
      //step two: whether swl is enable or not
    if (gwl_info->bEnable[ucSuperPu])
    {
        if (gwl_info->nDstBlk[ucSuperPu] == INVALID_4F)
        {
            gwl_info->nDstBlk[ucSuperPu] = gwl_info->nDstBlkBuf[ucSuperPu];//object block
            gwl_info->nDstBlkBuf[ucSuperPu] = INVALID_4F;
            gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu] = 0;
            gwl_info->bEnable[ucSuperPu] = FALSE;
            L2_SelectSWLSrcBlk(ucSuperPu);
        }
        else // only occur on boot after normal shutdown or detect both two src blks are all-dirty at second stage in GC flow
        {
            gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu] = 0;
            gwl_info->bEnable[ucSuperPu] = FALSE;
            L2_SelectSWLSrcBlk(ucSuperPu);
        }
    }
#else
//----------------------------------------------------------------------------------------------------------------
    BOOL bSWL = FALSE;
    U16 PhyAddr, DstPhyAddr;
    U16 usPhyDstBufAddr;
    U32 AverageErase;
    U32 ulDstBufEC, ulDstEC;
    PhyAddr = pVBT[ucSuperPu]->m_VBT[uVBN].PhysicalBlockAddr[ucLunInSuperPu];
    AverageErase = (pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU) / TLC_BLK_CNT;

    gwl_info->nOTimes[ucSuperPu]++;

    if (gwl_info->nDstBlkBuf[ucSuperPu] == INVALID_4F)
    {
        if (gwl_info->nOTimes[ucSuperPu] >= (g_WLTooColdThs * TLC_BLK_CNT + 500)) //Need to check for super page
        {
            //if (gwl_info->nDstBlk[ucSuperPu] == INVALID_4F)
            {
                gwl_info->bTooCold[ucSuperPu] = TRUE;
                bSWL = TRUE;
            }
        }
        else if ((EraseCnt > AverageErase) && (EraseCnt - AverageErase > g_WLEraseCntThs))
        {
            bSWL = TRUE;
        }

        if (bSWL)
        {
            gwl_info->nDstBlkBuf[ucSuperPu] = uVBN;
            L2_PBIT_Set_Lock(ucSuperPu, uVBN, TRUE);

#ifdef DBG_LC
            lc.uLockCounter[ucSuperPu][STATIC_WL]++;
            if (lc.uLockCounter[ucSuperPu][STATIC_WL] >= 3)
            {
                DBG_Printf("ucSuperPu %d lock block counter >= 3 !\n", ucSuperPu);
                DBG_Getch();
            }
#endif
        }
    }
    else
    {
        PhyAddr = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlkBuf[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];

        if (EraseCnt > pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyAddr].EraseCnt)
        {
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlkBuf[ucSuperPu], FALSE);
            L2_PBIT_Set_Lock(ucSuperPu, uVBN, TRUE);
            gwl_info->nDstBlkBuf[ucSuperPu] = uVBN;
        }
        bSWL = TRUE;
    }
    // execute this part when reboot after shutdown (because pbit store the dst blk info when shutdown)
    if ((INVALID_4F != gwl_info->nDstBlk[ucSuperPu]) && (TRUE == gwl_info->bEnable[ucSuperPu]))
    {
        DstPhyAddr = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
        ulDstEC = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][DstPhyAddr].EraseCnt;

        if (INVALID_4F != gwl_info->nDstBlkBuf[ucSuperPu])
        {
            usPhyDstBufAddr = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlkBuf[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
            ulDstBufEC = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyDstBufAddr].EraseCnt;
        }
        else
        {
            ulDstBufEC = 0;
        }

        if ((AverageErase > ulDstEC) || (ulDstBufEC > ulDstEC))
        {
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
            gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
#ifdef DBG_LC
            lc.uLockCounter[ucSuperPu][STATIC_WL]--;
#endif
        }
    }

    /* select Src block for SWL */
    if (TRUE == bSWL)
    {
        if (gwl_info->bEnable[ucSuperPu])
        {
#ifdef SIM
            if (gwl_info->nDstBlkBuf[ucSuperPu] == INVALID_4F)
            {
                DBG_Printf("nDstBlkBuf == INVALID_4F\n");
                DBG_Getch();
            }
#endif

            if (gwl_info->nDstBlk[ucSuperPu] == INVALID_4F)
            {
                gwl_info->nDstBlk[ucSuperPu] = gwl_info->nDstBlkBuf[ucSuperPu];//object block
            }
            else
            {
#ifdef SIM
                DstPhyAddr = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
                ulDstEC = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][DstPhyAddr].EraseCnt;
                usPhyDstBufAddr = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlkBuf[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
                ulDstBufEC = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][usPhyDstBufAddr].EraseCnt;
                if (ulDstBufEC > ulDstEC)
                {
                    DBG_Getch();
                }
#endif

                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlkBuf[ucSuperPu], FALSE);

#ifdef DBG_LC
                lc.uLockCounter[ucSuperPu][STATIC_WL]--;
#endif
            }
            gwl_info->nDstBlkBuf[ucSuperPu] = INVALID_4F;
            gwl_info->tSWLCommon.m_SrcWLO[ucSuperPu] = 0;
            gwl_info->bEnable[ucSuperPu] = FALSE;
            L2_SelectSWLSrcBlk(ucSuperPu);
        }
        else
        {
#if defined(SWL_EVALUATOR) && (!defined(SWL_OFF))
            SWLRecordIncCancelTime(ucSuperPu);
#endif

            bSWL = FALSE;
        }
    }

    if (TRUE == gbWLTraceRec)
    {
        if (bSWL)
        {
            TRACE_LOG((void*)&ucSuperPu, sizeof(U32), U32, 0, "PU ? ");
            TRACE_LOG((void*)&uVBN, sizeof(U32), U32, 0, "VBN ? ");
            TRACE_LOG((void*)&PhyAddr, sizeof(U16), U16, 0, "PhyBlk ? ");
            TRACE_LOG((void*)&EraseCnt, sizeof(U32), U32, 0, "EraseCnt ? ");
            TRACE_LOG((void*)&AverageErase, sizeof(U32), U32, 0, "AverageEraseCnt ? ");
        }
    }
#endif
    return;
}

void MCU1_DRAM_TEXT L2_SelectSWLSrcBlk(U8 ucSuperPu)
{
    //Update the Src PU and Src block
    U32 PhyBlk;
    U32 PhyBlkDst;
    U16 VBN = INVALID_4F;
    U32 uAverage = 0;
    U32 uMinEraseCnt = 0;
    BOOL bDidWL = FALSE;
    U8 ucLunInSuperPu = 0;
    BOOL bTLCBlk = FALSE;

#ifdef NEW_SWL
    U32 CurTime_SN;
    PhyBlkDst = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
    uAverage = (pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU) / TLC_BLK_CNT;

    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, gwl_info->nDstBlk[ucSuperPu]))
    {
        bTLCBlk = TRUE;
    }

    //To avoid unsigned overflow when EraseCnt < uAverage
    if (uAverage - pPBIT[ucSuperPu]->ulECMin > SWL_TH_AVE2MIN)
    {
        VBN = L2_Get_WLSrcBlk_E(ucSuperPu, ucLunInSuperPu, bTLCBlk);

        //(VBN == INVALID_4F) indicates all the blks are free (such as trim), then don't execute dummy-write
        if ((FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)) && (VBN == INVALID_4F))
        {
            FIRMWARE_LogInfo("////SWL select EC src cancel\n");
            gwl_info->bEnable[ucSuperPu] = TRUE;
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
            gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
            return;
        }
        //get the first available src blk
        else if ((FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)) && (VBN != INVALID_4F))
        {
            PhyBlk = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
            CurTime_SN = L2_Get_PBIT_BlkSN_Blk(ucSuperPu, ucLunInSuperPu, PhyBlk);
            if (L2_Get_PBIT_BlkSN_CE(ucSuperPu) < CurTime_SN + 500)
            {
                FIRMWARE_LogInfo("////SWL select EC src cancel\n");
                gwl_info->bEnable[ucSuperPu] = TRUE;
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);
                gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
                gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
                return;
            }
            if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt < pPBIT[ucSuperPu]->ulECMin)
            {
                DBG_Printf("ChooseECminAsSrc - SPU %d LUN %d Blk %d_%d,srcEC %d Wrong ECMin %d Blk record\n", ucSuperPu,
                    ucLunInSuperPu, VBN, PhyBlk, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt, pPBIT[ucSuperPu]->ulECMin);
                DBG_Getch();
            }
        }
        //do EC check when get second or more available src blks in one SWL Process, now don't need cancel
        else if (VBN != INVALID_4F)
        {
            PhyBlk = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
            if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt < pPBIT[ucSuperPu]->ulECMin)
            {
                DBG_Printf("ChooseECminAsSrc - SPU %d LUN %d Blk %d_%d,srcEC %d Wrong ECMin %d Blk record\n", ucSuperPu,
                    ucLunInSuperPu, VBN, PhyBlk, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt, pPBIT[ucSuperPu]->ulECMin);
                DBG_Getch();
            }
        }

#ifdef SWL_EVALUATOR
        gwl_info->ulSelSrcECminCnt[ucSuperPu]++;
        U32 PhyBlkSrc = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
        U32 PhyBlkSrcBuf = pVBT[ucSuperPu]->m_VBT[gwl_info->nSrcBlkBuf[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
        FIRMWARE_LogInfo("###DstBlk 0x%x DstBlkEC %d SrcBlk0 0x%x SrcBlk0EC %d SrcBlk1 0x%x SrcBlk1EC %d\n", gwl_info->nDstBlk[ucSuperPu], pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkDst].EraseCnt,
            VBN, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkSrc].EraseCnt, gwl_info->nSrcBlkBuf[ucSuperPu], pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkSrcBuf].EraseCnt);
        FIRMWARE_LogInfo("###AveEC %d TotalECmin %d SrcSelECMinCnt %d\n", uAverage, pPBIT[ucSuperPu]->ulECMin, gwl_info->ulSelSrcECminCnt[ucSuperPu]);
#endif
    }
    else
    {
        VBN = L2_Get_WLSrcBlk_S(ucSuperPu, ucLunInSuperPu, PhyBlkDst, bTLCBlk);

        //(VBN == INVALID_4F) indicates all the blks are free (such as trim), then don't execute dummy-write
        if ((FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT)) && (VBN == INVALID_4F))
        {
            FIRMWARE_LogInfo("////SWL select SN src cancel\n");
            gwl_info->bEnable[ucSuperPu] = TRUE;
            L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
            gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
            return;
        }
        else if (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT) && (VBN != INVALID_4F))
        {
            if ((L2_Get_PBIT_BlkSN_CE(ucSuperPu) < (L2_Get_PBIT_MinBlkSN_CE(ucSuperPu) + 3 * TLC_BLK_CNT + 300))
                && (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkDst].EraseCnt - pPBIT[ucSuperPu]->ulECMin <(SWL_TH_MAX2MIN *3/2)))
            {
                FIRMWARE_LogInfo("////SWL select TS src cancel\n");
                gwl_info->bEnable[ucSuperPu] = TRUE;
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nDstBlk[ucSuperPu], FALSE);
                L2_PBIT_Set_Lock(ucSuperPu, gwl_info->nSrcBlkBuf[ucSuperPu], FALSE);

                gwl_info->nDstBlk[ucSuperPu] = INVALID_4F;
                gwl_info->nSrcBlkBuf[ucSuperPu] = INVALID_4F;
                return;
            }
            PhyBlk = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
            if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt < pPBIT[ucSuperPu]->ulECMin)
            {
                DBG_Printf("ChooseSNminAsSrc - SPU %d LUN %d Blk %d_%d,srcEC %d Wrong ECMin %d Blk record\n", ucSuperPu,
                    ucLunInSuperPu, VBN, PhyBlk, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt, pPBIT[ucSuperPu]->ulECMin);
                DBG_Getch();
            }
        }
        else if (VBN != INVALID_4F)
        {
            PhyBlk = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
            if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt < pPBIT[ucSuperPu]->ulECMin)
            {
                DBG_Printf("ChooseSNminAsSrc - SPU %d LUN %d Blk %d_%d,srcEC %d Wrong ECMin %d Blk record\n", ucSuperPu,
                    ucLunInSuperPu, VBN, PhyBlk, pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt, pPBIT[ucSuperPu]->ulECMin);
                DBG_Getch();
            }
        }
    }

    if (INVALID_4F == VBN)
    {
        DBG_Printf("SWL select SrcBlk Fail SPU %d\n", ucSuperPu);
        gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
        return;
    }

    gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = VBN;
    L2_PBIT_Set_Lock(ucSuperPu, VBN, TRUE);
//----------------------------------------------------------------------------------------------------------
#else
#if (defined(SIM) && (!defined(SWL_OFF)))
    BOOL bTooCold;

    if (gwl_info->bTooCold[ucSuperPu])
    {
        bTooCold = TRUE;
    }
    else
    {
        bTooCold = FALSE;
    }
#endif

    PhyBlkDst = pVBT[ucSuperPu]->m_VBT[gwl_info->nDstBlk[ucSuperPu]].PhysicalBlockAddr[ucLunInSuperPu];
    uAverage = (pPBIT[ucSuperPu]->m_EraseCnt[BLKTYPE_TLC] / LUN_NUM_PER_SUPERPU) / TLC_BLK_CNT;
    if (TRUE == L2_VBT_Get_TLC(ucSuperPu, gwl_info->nDstBlk[ucSuperPu]))
    {
        bTLCBlk = TRUE;
    }

    //To avoid unsigned overflow when EraseCnt < uAverage
    if (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkDst].EraseCnt > (uAverage + 79))
    {
        bDidWL = TRUE;
        VBN = L2_Get_WLSrcBlk_E(ucSuperPu, ucLunInSuperPu, bTLCBlk);
    }
    else
    {
        VBN = L2_Get_WLSrcBlk_S(ucSuperPu, ucLunInSuperPu, PhyBlkDst, bTLCBlk);

        if (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT))
        {
            if (L2_Get_PBIT_BlkSN_CE(ucSuperPu) > (L2_Get_PBIT_MinBlkSN_CE(ucSuperPu) + 3 * TLC_BLK_CNT + 500))
            {
                bDidWL = TRUE;
            }
        }
        else
        {
            bDidWL = TRUE;
        }

    }

    //TRUE indicates SWL is selecting the second or more src blk
    if (FALSE != L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT))
    {
        if (VBN == INVALID_4F)
        {
            gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = INVALID_4F;
            return;
        }
        else
        {
            gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = VBN;
            L2_PBIT_Set_Lock(ucSuperPu, VBN, TRUE);
            return;
        }
    }
    else
    {
        if (VBN == INVALID_4F)
        {
             bDidWL = FALSE;
        }
        else
        {
            PhyBlk = pVBT[ucSuperPu]->m_VBT[VBN].PhysicalBlockAddr[ucLunInSuperPu];
            if (gwl_info->bTooCold[ucSuperPu])
            {
                gwl_info->bTooCold[ucSuperPu] = FALSE;
                if ((pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkDst].EraseCnt > pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt) &&
                    (pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlkDst].EraseCnt - pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt) >= 20)
                {
                    bDidWL = TRUE;
                    gwl_info->nOTimes[ucSuperPu] = 0;
                }
                else
                {
                    /* fix SeqW performance drop issue,because EraseCnt & SN is inherited. */
                    bDidWL = FALSE;
                }
            }
        }
    }

    if (TRUE == gbWLTraceRec)
    {
        U16 usEraseCnt;
        U32 ulDirtyLPNCnt;

#ifdef DirtyLPNCnt_IN_DSRAM1
        ulDirtyLPNCnt = *(g_pDirtyLPNCnt + ucSuperPu*VIR_BLK_CNT + VBN);
#else
        ulDirtyLPNCnt = pVBT[ucSuperPu]->m_VBT[VBN].DirtyLPNCnt;
#endif
        usEraseCnt = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt;

        TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "Source PU ? ");
        TRACE_LOG((void*)&VBN, sizeof(U32), U32, 0, "VirBlk ? ");
        TRACE_LOG((void*)&PhyBlk, sizeof(U32), U32, 0, "PhyBlk ? ");
        TRACE_LOG((void*)&ulDirtyLPNCnt, sizeof(U16), U16, 0, "DirtyCnt ? ");
        TRACE_LOG((void*)&usEraseCnt, sizeof(U16), U16, 0, "EraseCnt ? ");
        //TRACE_LOG((void*)&pPBIT[ucSuperPu]->m_PBIT_Info[ucLunInSuperPu][PhyBlk].CurSerialNum, sizeof(U32), U32, 0, "SerialNum ? ");//??
    }

    //DBG_Printf("PU=%d,SrcPhyBlk=%d,DstPhyBlk=%d,%d\n", ucSuperPu, PhyBlk,gwl_info->nDstBlk[ucSuperPu], L2_Get_PBIT_BlkSN_CE(ucSuperPu) - L2_Get_PBIT_MinBlkSN_CE(ucSuperPu));
    if (!bDidWL)
    {
        if (TRUE == gbWLTraceRec)
        {
            U16 usEraseCnt;
            usEraseCnt = pPBIT[ucSuperPu]->m_PBIT_Entry[ucLunInSuperPu][PhyBlk].EraseCnt;

            TRACE_LOG((void*)&ucSuperPu, sizeof(U16), U16, 0, "Cancle PU ? ");
            TRACE_LOG((void*)&gwl_info->nDstBlk[ucSuperPu], sizeof(U16), U16, 0, "VirBlk ? ");
            TRACE_LOG((void*)&PhyBlkDst, sizeof(U32), U32, 0, "PhyBlk ? ");
            TRACE_LOG((void*)&usEraseCnt, sizeof(U16), U16, 0, "EraseCnt ? ");
            TRACE_LOG((void*)&uAverage, sizeof(U32), U32, 0, "AverageEraseCnt ? ");
        }

        L2_SWLClear(ucSuperPu, FALSE);

#if (defined(SWL_EVALUATOR) && (!defined(SWL_OFF)))
        SWLRecordIncCancelTime(ucSuperPu);
#endif
    }
    else
    {
        gwl_info->tSWLCommon.m_SrcPBN[ucSuperPu] = VBN;
        L2_PBIT_Set_Lock(ucSuperPu, VBN, TRUE);
        //L2_SetSWLMode(ucSuperPu);

#ifdef SIM
        if (L2_VBT_Get_TLC(ucSuperPu, VBN) != L2_VBT_Get_TLC(ucSuperPu, gwl_info->nDstBlk[ucSuperPu]))
        {
            DBG_Printf("PU:%d Src(%d) bTLC:%d, Des(%d) bTLC:%d\n", ucSuperPu, VBN, L2_VBT_Get_TLC(ucSuperPu, VBN)
                , gwl_info->nDstBlk[ucSuperPu], L2_VBT_Get_TLC(ucSuperPu, gwl_info->nDstBlk[ucSuperPu]));
            DBG_Getch();
        }
#endif

#if (defined(SWL_EVALUATOR) && (!defined(SWL_OFF)))
        if (bTooCold)
        {
            SWLRecordIncTooCold(ucSuperPu);
        }
        else
        {
            SWLRecordIncNormal(ucSuperPu);
        }
#endif
    }
#endif
}


BOOL L2_IsWL(U8 ucPuNum)
{
    if (FALSE == gwl_info->bEnable[ucPuNum])
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }    
}


void MCU1_DRAM_TEXT L2_SWL_TerminateTask(U8 ucSuperPu)
{
    L2_ResetTLCManager(ucSuperPu);
    L2_FTLTaskTLCSWLClear(ucSuperPu);
    L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_WL);

    return;
}


BOOL MCU1_DRAM_TEXT L2_StaticWLEntry(U8 ucSuperPu, U32 ulLunAllowToSendFcmdBitmap)
{
#ifdef SIM
    if (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT))
    {
        DBG_Printf("SuperPU %d TLCSWL process TaskInfo 0x%x error\n", ucSuperPu, L2_FTLGetTaskBitInfo(ucSuperPu));
        DBG_Getch();
    }
#endif

    U8 ucGCMode;
    BOOL bRet;
    bRet = L2_ProcessTLCGC(ucSuperPu, ulLunAllowToSendFcmdBitmap, FTL_THREAD_WL, TLC_WRITE_SWL);
    ucGCMode = TLCGC_MODE;

    if (FALSE == L2_FTLIsTaskSet(ucSuperPu, TASK_TLCSWL_BIT))
    {
        FIRMWARE_LogInfo("SuperPU %d SWLMode %d SWL Done\n", ucSuperPu, ucGCMode);

        //TLC GC flow must include the operation Time_WL++, so fill FALSE in the entry parameter
        L2_SWLClear(ucSuperPu, FALSE);

        L2_FTLUsedThreadQuotaMax(ucSuperPu, FTL_THREAD_WL);
    }
    return bRet;
}
