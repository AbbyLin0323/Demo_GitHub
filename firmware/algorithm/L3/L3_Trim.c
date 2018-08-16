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
Filename    :L2_Trim.c
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.09.19
Description :functions about Trim Process
Others      :
Modify      :
*******************************************************************************/
#include "COM_Memory.h"
#include "L2_Defines.h"
#include "L2_VBT.h"
#include "FW_Event.h"
#include "L2_GCManager.h"

#ifndef SIM
#include <xtensa/tie/xt_datacache.h>
#endif
#ifdef SIM
#define XT_DPFR
#endif

#ifdef DirtyLPNCnt_IN_DSRAM1
GLOBAL U32 *g_pDirtyLPNCnt;
#endif

extern U32 g_DirtyLPNCntAddr;

#ifndef SIM
MCU12_VAR_ATTR L2TrimShareData *g_pShareData;
#endif

void L3_DirtyCntCopyBack(U8 ucPu)
{
    U32  ulBlockSN;
    U32  *pDirtyCnt = (U32 *)g_DirtyLPNCntAddr;

    for (ulBlockSN = 0; ulBlockSN < VIR_BLK_CNT; ulBlockSN++)
    {
        if (pDirtyCnt[ulBlockSN] != 0)
        {
#ifdef DirtyLPNCnt_IN_DSRAM1
            //DBG_Printf("block%d %d + %d\n",ulBlockSN, *(g_pDirtyLPNCnt + ucPu*VIR_BLK_CNT + ulBlockSN), pDirtyCnt[ulBlockSN]); 
            *(g_pDirtyLPNCnt + ucPu*VIR_BLK_CNT + ulBlockSN) += pDirtyCnt[ulBlockSN];
#else
            //DBG_Printf("block%d %d + %d\n",ulBlockSN, pVBT[ucPu]->m_VBT[ulBlockSN].DirtyLPNCnt, pDirtyCnt[ulBlockSN]);
            pVBT[ucPu]->m_VBT[ulBlockSN].DirtyLPNCnt += pDirtyCnt[ulBlockSN];
#endif
        }
    }
}

#ifndef SIM
U8 CheckWritePinter(U8 index)
{
    U32 ShareAddr = (U32)g_pShareData;
    U32 ulTemp, ulTemp1 = INVALID_8F;
    U8 ucTemp = 0;

    /*
    do
    {
        if (g_pShareData->ucL2TrimStatus == Trim_L2_Finish)
        {
            break;
        }

    }while(pTrimPhyAddr[sIndex].m_PPN == 0);
    */

    asm ("10:\n\t \
        //pTrimPhyAddr[sIndex].m_PPN == 0\n\t \
        addi %1, %3, 12\n\t \
        addx4 %1, %0, %1\n\t \
        l32i %1, %1, 0\n\t \
        bne %1, %4, 20f\n\t \
        //g_pShareData->ucL2TrimStatus == Trim_L2_Finish\n\t \
        l8ui %1, %3, 0\n\t \
        bnei %1, 3, 10b\n\t \
        20:\n\t"
        :"+a"(index),"+a"(ulTemp),"+a"(ucTemp)
        :"a"(ShareAddr),"a"(ulTemp1)
        :"memory");

    return ucTemp;
}

U32 CheckL2Status(U32 status)
{
    U32 ShareAddr = (U32)g_pShareData;
    U32 ulTemp,ulTemp1 = 0;

    asm ("10:\n\t \
        l8ui %1, %3, 0\n\t \
        //addi %2, %2, 1\n\t \
        bne %0, %1, 10b\n\t"
        :"+a"(status),"+a"(ulTemp),"+a"(ulTemp1)
        :"a"(ShareAddr)
        :"memory");

    return ulTemp1;
}
#endif
void L3_TrimReqProcess(void)
{
    U16 usPMTIIndexInPu = INVALID_4F;
    U16 usPMTIIndexInPuPrev;
    BOOL bDone = FALSE;
    U32 ulTrimOK, ulLoop;
    S8  sIndex;
    U32  *pDirtyCnt;
    U16 *pPMTIIndexInPu;
    PhysicalAddr *pTrimPhyAddr;
    PhysicalAddr TrimPhyAddrValue;
    U32 *pDirtyBitMap;
    U32 ShareAddr, ulTemp = INVALID_8F;

#if (LPN_PER_BUF == 8)
    U8 *pLpnMap_Page;
#elif (LPN_PER_BUF == 16)
    U16 *pLpnMap_Page;
#else
    U8 *pLpnMap_Page;
#endif

#ifndef SIM
    U16 usTemp;
    U8 ucTemp;
#endif
#ifdef ValidLPNCountSave_IN_DSRAM1
    U32 ulVaildLPNCnt = 0, ulOfs = 0;
    U16 *pValidLPNCountSaveL;
    U8 *pValidLPNCountSaveH;
#else
    RED *pValidLPNCountSaveL;
#endif
    U8 ucPu, ucPuPrev;
    DPBM_ENTRY* pL3DPBM_ENTRY;

    //DBG_Printf("L3 entry\n");
    ShareAddr = (U32)g_pShareData;
    pTrimPhyAddr = (PhysicalAddr *)&g_pShareData->ulTrimPhyAddr[0];
    pPMTIIndexInPu = (U16 *)&g_pShareData->usPMTIIndexInPu[0];

    bDone = FALSE;
    if (g_pShareData->ucL3TrimStatus != Trim_L2_Start)
    {
        DBG_Printf("L3 : start at %d\n",g_pShareData->ucL3TrimStatus);
        DBG_Getch();
    }

#ifdef SIM
    do
    {
        if (g_pShareData->ucL2TrimStatus == Trim_L2_Finish)
        {
            if (pTrimPhyAddr[0].m_PPN != INVALID_8F)
            {
                break;
            }
            g_pShareData->ucL3TrimStatus = Trim_L3_Finish;

            while(1)
            {
                if (g_pShareData->ucL2TrimStatus == Trim_Finish)
                {
                    g_pShareData->ucL3TrimStatus = Trim_Finish;
                    CommClearEvent(COMM_EVENT_OWNER_L3,COMM_EVENT_OFFSET_TrimL3);
                    //DBG_Printf("L3 return 0\n");
                    return;
                }
            }
        }
    } while (pTrimPhyAddr[0].m_PPN == INVALID_8F);
#else
    asm ("\n\t \
        addi %1, %3, 12\n\t \
        10:\n\t \
        //g_pShareData->ucL2TrimStatus == Trim_L2_Finish\n\t \
        l8ui %0, %3, 0\n\t \
        bnei %0, 3, 20f\n\t \
        //if (pTrimPhyAddr[0].m_PPN != INVALID_8F)\n\t \
        l32i %0, %1, 0\n\t \
        bne %0, %4, 30f\n\t \
        //g_pShareData->ucL3TrimStatus = Trim_L3_Finish\n\t \
        movi %0, 6\n\t \
        s8i %0, %3, 1\n\t \
        40:\n\t \
        //g_pShareData->ucL2TrimStatus == Trim_Finish\n\t \
        l8ui %0, %3, 0\n\t \
        bnei %0, 7, 40b\n\t \
        movi %0, 7\n\t \
        s8i %0, %3, 1\n\t \
        movi %2, 1\n\t \
        j 30f\n\t \
        20:\n\t \
        //if (pTrimPhyAddr[0].m_PPN == INVALID_8F)\n\t \
        l32i %0, %1, 0\n\t \
        beq %0, %4, 10b\n\t \
        30:\n\t"
        :"+a"(usTemp),"+a"(ucTemp),"+a"(bDone)
        :"a"(ShareAddr),"a"(ulTemp)
        :"memory");

    if (bDone == TRUE)
    {
        CommClearEvent(COMM_EVENT_OWNER_L3,COMM_EVENT_OFFSET_TrimL3);
        //if no cache , trim will be fail because cache coherence
#ifdef DCACHE
        HAL_InvalidateDCache();
#endif
        return;
    }
#endif

    pDirtyCnt = (U32 *)g_DirtyLPNCntAddr;
    for (ulLoop = 0; ulLoop < VIR_BLK_CNT; ulLoop++)
    {
        pDirtyCnt[ulLoop] = 0;
    }
    ulTrimOK = 0;
#ifdef DirtyLPNCnt_IN_DSRAM1
    g_pDirtyLPNCnt = (U32 *)g_pShareData->ulDirtyLPNCntAddr;
#endif
#ifdef ValidLPNCountSave_IN_DSRAM1
    pValidLPNCountSaveL = (U16 *)g_pShareData->ulValidLPNCountSaveL_addr;
    pValidLPNCountSaveH = (U8 *)g_pShareData->ulValidLPNCountSaveH_addr;
#endif

#ifndef IM_3D_TLC_1TB
    pDirtyBitMap = (U32 *)g_pShareData->ulDirtyBitMapAddr[0];
    ucPu = ucPuPrev = 0;
#else
    ucPu = ucPuPrev = g_pShareData->ucPu[0];
#endif

    pL3DPBM_ENTRY = (DPBM_ENTRY *)(g_pShareData->ulTrimLPNMap[ucPu]);
    usPMTIIndexInPuPrev = usPMTIIndexInPu = pPMTIIndexInPu[0];

    while(1)
    {
        sIndex = (g_pShareData->sReadPoint + 1) % TrimBufferSize;
        g_pShareData->sReadPoint = sIndex;
#ifndef SIM
        CheckWritePinter(sIndex);
#else
 
        /*
        do
        {
            if (g_pShareData->ucL2TrimStatus == Trim_L2_Finish)
            {
                break;
            }

        }while(pTrimPhyAddr[sIndex].m_PPN == 0);
        */

        while (1)
        {
            if ((g_pShareData->ulTrimPhyAddr[sIndex] != INVALID_8F) || (g_pShareData->ucL2TrimStatus == Trim_L2_Finish))
            {
                break;
            }
        }
#endif

        TrimPhyAddrValue.m_PPN = pTrimPhyAddr[sIndex].m_PPN;
        //DBG_Printf("L3 : w%d , r%d , PPN : 0x%x = 0x%x\n", g_pShareData->sWritePoint, sIndex, (U32)&pTrimPhyAddr[sIndex],pTrimPhyAddr[sIndex].m_PPN);
        if (TrimPhyAddrValue.m_PPN != INVALID_8F)
        {
#ifdef IM_3D_TLC_1TB
            ucPu = g_pShareData->ucPu[sIndex];
#endif
            usPMTIIndexInPu = pPMTIIndexInPu[sIndex];
            pTrimPhyAddr[sIndex].m_PPN = INVALID_8F;

#ifdef IM_3D_TLC_1TB
            if ((usPMTIIndexInPu != usPMTIIndexInPuPrev) || (ucPu != ucPuPrev))
#else
            if (usPMTIIndexInPu != usPMTIIndexInPuPrev)
#endif
            {
                if (ulTrimOK)
                {
#ifdef IM_3D_TLC_1TB
                    pDirtyBitMap = (U32 *)g_pShareData->ulDirtyBitMapAddr[ucPuPrev];
#endif
                    pDirtyBitMap[usPMTIIndexInPuPrev >> 5] |= (1 << (usPMTIIndexInPuPrev & 0x1F));

#ifdef ValidLPNCountSave_IN_DSRAM1
                    ulOfs = (ucPuPrev*PMTPAGE_CNT_PER_SUPERPU_MAX) + usPMTIIndexInPuPrev;
                    ulVaildLPNCnt = (U32)(*(pValidLPNCountSaveL + ulOfs)) | (U32)(*(pValidLPNCountSaveH + ulOfs) << 16);
                    if (ulVaildLPNCnt < ulTrimOK)
                    {
                        DBG_Printf("L3_Trim VaildLPNCount error %d : %d < %d\n", usPMTIIndexInPuPrev, ulVaildLPNCnt, ulTrimOK);
                        DBG_Getch();
                    }
                    /*
                    else
                    {
                        DBG_Printf("L3_Trim VaildLPNCount %d : %d < %d\n", usPMTIIndexInPuPrev, ulVaildLPNCnt, ulTrimOK);
                    }
                    */
                    ulVaildLPNCnt -= ulTrimOK;
                    *(pValidLPNCountSaveH + ulOfs) = (U8)(ulVaildLPNCnt >> 16);
                    *(pValidLPNCountSaveL + ulOfs) = (U16)ulVaildLPNCnt;
#else
                    pValidLPNCountSaveL = (RED *)g_pShareData->ulValidLPNCountSaveAddr[ucPuPrev];
                    if (pValidLPNCountSaveL[usPMTIIndexInPuPrev].m_ValidLPNCountSave < ulTrimOK)
                    {
                        DBG_Printf("L3 VaildLPNCount error %d : %d %d\n", usPMTIIndexInPuPrev, pValidLPNCountSaveL[usPMTIIndexInPuPrev].m_ValidLPNCountSave, ulTrimOK);
                        DBG_Getch();
                    }
                    pValidLPNCountSaveL[usPMTIIndexInPuPrev].m_ValidLPNCountSave -= ulTrimOK;
#endif
                }

                ulTrimOK = 1;
                usPMTIIndexInPuPrev = usPMTIIndexInPu;
#ifdef IM_3D_TLC_1TB
                if (ucPuPrev != ucPu)
                {
                    L3_DirtyCntCopyBack(ucPuPrev);
                    for (ulLoop = 0; ulLoop < VIR_BLK_CNT; ulLoop++)
                    {
                        pDirtyCnt[ulLoop] = 0;
                    }
                    ucPuPrev = ucPu;
                    pL3DPBM_ENTRY = (DPBM_ENTRY *)(g_pShareData->ulTrimLPNMap[ucPu]);
                }
#endif
            }
            else
            {
                ulTrimOK++;
            }
            pDirtyCnt[TrimPhyAddrValue.m_BlockInPU]++;
            g_pShareData->ulL3TrimTotal++;
#if (LPN_PER_BUF == 8)
            pLpnMap_Page = (U8*)&pL3DPBM_ENTRY[TrimPhyAddrValue.m_BlockInPU].m_LpnMapPerPage[TrimPhyAddrValue.m_OffsetInSuperPage][TrimPhyAddrValue.m_PageInBlock];
#elif (LPN_PER_BUF == 16)
            pLpnMap_Page = (U16*)&pL3DPBM_ENTRY[TrimPhyAddrValue.m_BlockInPU].m_LpnMapPerPage[TrimPhyAddrValue.m_OffsetInSuperPage][TrimPhyAddrValue.m_PageInBlock];
#else
            pLpnMap_Page = (U8*)&pL3DPBM_ENTRY[TrimPhyAddrValue.m_BlockInPU].m_LpnMapPerPage[TrimPhyAddrValue.m_OffsetInSuperPage][TrimPhyAddrValue.m_PageInBlock];
#endif
            if (((*pLpnMap_Page) & (0x1 << TrimPhyAddrValue.m_LPNInPage)) != 0)
            {
                *pLpnMap_Page &= ~(0x1 << TrimPhyAddrValue.m_LPNInPage);
            }
            else
            {
                DBG_Printf("error : L3_TrimUpdateDirtyLpnMap , PPN = 0x%x , %d %d\n", TrimPhyAddrValue.m_PPN, sIndex, g_pShareData->sWritePoint);
                DBG_Printf("error : SuperPu %d Blk %d Pg %d OffsetInSuperPage %d LpnInPg %d PPN 0x%x LPNMap 0x%x\n", TrimPhyAddrValue.m_PUSer, TrimPhyAddrValue.m_BlockInPU, TrimPhyAddrValue.m_PageInBlock,
                    TrimPhyAddrValue.m_OffsetInSuperPage, TrimPhyAddrValue.m_LPNInPage, TrimPhyAddrValue.m_PPN, *pLpnMap_Page);
                DBG_Getch();
            }
        }
        else
        {
            bDone = TRUE;
        }

        if (bDone == TRUE)
        {
            if (ulTrimOK)
            {
#ifdef IM_3D_TLC_1TB 
                pDirtyBitMap = (U32 *)g_pShareData->ulDirtyBitMapAddr[ucPu];
#endif
                //L2_SetDirty(ucPu, usPMTIIndexInPu);
                pDirtyBitMap[usPMTIIndexInPu >> 5] |= (1 << (usPMTIIndexInPu & 0x1F));
#ifdef ValidLPNCountSave_IN_DSRAM1 
                //g_PMTManager->m_PMTSpareBuffer[ucPu][usPMTIIndexInPu]->m_ValidLPNCountSave -= ulTrimOK;
                ulOfs = (ucPu*PMTPAGE_CNT_PER_SUPERPU_MAX) + usPMTIIndexInPuPrev;
                ulVaildLPNCnt = (U32)(*(pValidLPNCountSaveL + ulOfs)) | (U32)(*(pValidLPNCountSaveH + ulOfs) << 16);
                if (ulVaildLPNCnt < ulTrimOK)
                {
                    DBG_Printf("L3_Trim VaildLPNCount error %d : %d < %d\n", usPMTIIndexInPuPrev, ulVaildLPNCnt, ulTrimOK);
                    DBG_Getch();
                }
                /*
                else
                {
                    DBG_Printf("L3_Trim VaildLPNCount %d : %d < %d\n", usPMTIIndexInPuPrev, ulVaildLPNCnt, ulTrimOK);
                }
                */
                ulVaildLPNCnt -= ulTrimOK;
                *(pValidLPNCountSaveH + ulOfs) = (U8)(ulVaildLPNCnt >> 16);
                *(pValidLPNCountSaveL + ulOfs) = (U16)ulVaildLPNCnt;
#else
                pValidLPNCountSaveL = (RED *)g_pShareData->ulValidLPNCountSaveAddr[ucPuPrev];
                //DBG_Printf("pValidLPNCountSaveL = 0x%x\n",(U32)pValidLPNCountSaveL);
                if (pValidLPNCountSaveL[usPMTIIndexInPuPrev].m_ValidLPNCountSave < ulTrimOK)
                {
                    DBG_Printf("L3 VaildLPNCount error %d : %d %d\n", usPMTIIndexInPuPrev, pValidLPNCountSaveL[usPMTIIndexInPuPrev].m_ValidLPNCountSave, ulTrimOK);
                    DBG_Getch();
                }
                pValidLPNCountSaveL[usPMTIIndexInPuPrev].m_ValidLPNCountSave -= ulTrimOK;
#endif
            }

            L3_DirtyCntCopyBack(ucPu);
            g_pShareData->ucL3TrimStatus = Trim_L3_Finish;

#ifdef SIM
            while(1)
            {
                if (g_pShareData->ucL2TrimStatus == Trim_Finish)
                {
                    break;
                }
            }
#else
            CheckL2Status(Trim_Finish);
#endif

            g_pShareData->ucL3TrimStatus = Trim_Finish;
            CommClearEvent(COMM_EVENT_OWNER_L3,COMM_EVENT_OFFSET_TrimL3);
            /*
            if (g_pShareData->ulL2TrimTotal != g_pShareData->ulL3TrimTotal)
            {
                DBG_Printf("L3 : error : Trim total not mach %d %d ,w%d , r%d , status %d %d  0x%x\n", g_pShareData->ulL2TrimTotal, g_pShareData->ulL3TrimTotal, g_pShareData->sWritePoint, g_pShareData->sReadPoint,g_pShareData->ucL2TrimStatus,g_pShareData->ucL3TrimStatus,TrimPhyAddrValue.m_PPN);
                DBG_Printf("L3 : 0x%x 0x%x 0x%x 0x%x , 0x%x 0x%x 0x%x 0x%x ", pTrimPhyAddr[0].m_PPN, pTrimPhyAddr[1].m_PPN, pTrimPhyAddr[2].m_PPN, pTrimPhyAddr[3].m_PPN, pTrimPhyAddr[4].m_PPN, pTrimPhyAddr[5].m_PPN, pTrimPhyAddr[6].m_PPN, pTrimPhyAddr[7].m_PPN);
                DBG_Printf(", 0x%x 0x%x 0x%x 0x%x , 0x%x 0x%x 0x%x 0x%x\n", pTrimPhyAddr[8].m_PPN, pTrimPhyAddr[9].m_PPN, pTrimPhyAddr[10].m_PPN, pTrimPhyAddr[11].m_PPN, pTrimPhyAddr[12].m_PPN, pTrimPhyAddr[13].m_PPN, pTrimPhyAddr[14].m_PPN, pTrimPhyAddr[15].m_PPN);
            }
            DBG_Printf("L3 : return : %d %d \n", g_pShareData->ulL2TrimTotal, g_pShareData->ulL3TrimTotal);
            */

            //if no cache , trim will be fail because cache coherence
#ifdef DCACHE
            HAL_InvalidateDCache();
#endif
            return;
        }
        //g_pShareData->sReadPoint = sIndex;
    }
}
