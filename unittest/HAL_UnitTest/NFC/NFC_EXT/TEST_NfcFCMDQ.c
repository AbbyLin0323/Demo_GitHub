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
Filename    : TEST_NfcMain.c
Version     : Ver 1.0
Author      : abby
Date        : 2015.11.10
Description : This file provide NFC driver test pattern, can be used both in
              COSIM and winsim.
              Contain Basic NFC test pattern.
Others      :
Modify      :
20160825    abby    create
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcFCMDQ.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
GLOBAL MCU12_VAR_ATTR FCMD_REQ *g_pLocalFCmdReq;    // Allocated from shared Dsram1
GLOBAL MCU12_VAR_ATTR volatile FCMD_REQSTS   *g_pLocalFCmdReqSts; // Allocated from OTFB
GLOBAL MCU12_VAR_ATTR volatile FCMD_REQ_DPTR *g_pLocalFCmdReqDptr;// Allocated from shared Dsram1
GLOBAL MCU12_VAR_ATTR NFC_FCMD_MONITOR *g_pLocalFCmdMonitor;
GLOBAL MCU12_VAR_ATTR volatile U32 *g_pPendBMP; //each LUN occupy 1 bit
GLOBAL MCU12_VAR_ATTR NFC_PENDING_FCMDQ *g_pLocalFCmdPend;

extern BOOL TEST_NfcFCmdQIsNeedDataCheck(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);
extern U32 TEST_NfcExtCheckData(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel);
extern U16 MCU12_DRAM_TEXT L2_VBT_GetPhysicalBlockAddr(U8 ucSuperPu, U8 ucLunInSuperPu, U16 VBN);
void TEST_NfcFCmdQClrDataCheckBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr);

FCMD_REQ_ENTRY *TEST_NfcFCmdQGetEntryAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    return &g_pLocalFCmdReq->atReqQ[ucTLun][eFCmdPri].atReq[ucLevel];
}

U8 TEST_NfcFCmdQGetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    return g_pLocalFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucWptr;
}

LOCAL void TEST_NfcFCmdQSetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    g_pLocalFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucWptr = ucWptr;
}

FCMD_REQ_STS TEST_NfcFCmdQGetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    return (FCMD_REQ_STS)g_pLocalFCmdReqSts->atReqStsQ[ucTLun][eFCmdPri].aReqStatus[ucLevel];
}

void TEST_NfcFCmdQSetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel, FCMD_REQ_STS eReqSts)
{
    g_pLocalFCmdReqSts->atReqStsQ[ucTLun][eFCmdPri].aReqStatus[ucLevel] = eReqSts;
}

U32 TEST_NfcFCmdQGetReqStsAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    return (U32)&g_pLocalFCmdReqSts->atReqStsQ[ucTLun][eFCmdPri].aReqStatus[ucLevel];
}

BOOL TEST_NfcFCmdQIsWptrFree(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    BOOL bIsFree = FALSE;
    volatile U8 ucSts = TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr);

    if (LOCAL_FCMD_REQ_STS_INIT == ucSts)
    {
        bIsFree = TRUE;

        #ifdef FLASH_CACHE_OPERATION
        U8 ucNextWptr;
        FCMD_REQ_ENTRY *ptReqEntry;

        ptReqEntry = TEST_NfcFCmdQGetEntryAddr(ucTLun, eFCmdPri, ucWptr);
        if (TRUE == ptReqEntry->bsPrePgEn)
        {
            ucNextWptr = (ucWptr + 1) % FCMDQ_DEPTH;
            if (LOCAL_FCMD_REQ_STS_INIT != TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucNextWptr))
            {
                bIsFree = FALSE;
            }
        }
        #endif
    }
    
    return bIsFree;
}

BOOL TEST_NfcFCmdQIsEmpty(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucWptr;
    BOOL bIsEmpty = TRUE;

    for (ucWptr = 0; ucWptr < FCMDQ_DEPTH; ucWptr++)
    {
        if (LOCAL_FCMD_REQ_STS_INIT != TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr))
        {
            bIsEmpty = FALSE;
            break;
        }
    }

    return bIsEmpty;
}

BOOL TEST_NfcFCmdQIsFull(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucWptr;
    BOOL bIsEmpty = TRUE;

    for (ucWptr = 0; ucWptr < FCMDQ_DEPTH; ucWptr++)
    {
        if (LOCAL_FCMD_REQ_STS_INIT != TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr))
        {
            bIsEmpty = FALSE;
            break;
        }
    }

    return bIsEmpty;
}

BOOL TEST_NfcAllTLunFCmdQIsEmpty(FCMD_REQ_PRI eFCmdPri)
{
    U8 ucWptr, ucTLun;
    BOOL bIsEmpty = TRUE;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (ucWptr = 0; ucWptr < FCMDQ_DEPTH; ucWptr++)
        {
            if (LOCAL_FCMD_REQ_STS_INIT != TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr))
            {
                bIsEmpty = FALSE;
                break;
            }
        }
    }

    return bIsEmpty;
}

void TEST_NfcFCmdQSetReqEntryRdy(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    BOOL bIsNeedDataChk  = FALSE;

    /* check FCMDQ is not full */
    while (FALSE == TEST_NfcFCmdQIsWptrFree(ucTLun, eFCmdPri, ucWptr))
    {
        ;
    }

#ifndef ACC_EXT_TEST
    /*  for erase/program FCMD or read FCMD but disable data check, go through  */
    bIsNeedDataChk = TEST_NfcFCmdQIsNeedDataCheck(ucTLun, eFCmdPri, ucWptr);
#endif

    if (!bIsNeedDataChk)
    {
        ASSERT(LOCAL_FCMD_REQ_STS_INIT == TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr));
        TEST_NfcFCmdQSetReqSts(ucTLun, eFCmdPri, ucWptr, LOCAL_FCMD_REQ_STS_RDY);
    }
    else
    {
        if (SUCCESS == TEST_NfcExtCheckData(ucTLun, eFCmdPri, ucWptr))
        {
            TEST_NfcFCmdQClrDataCheckBitmap(ucTLun, eFCmdPri, ucWptr);

            ASSERT(LOCAL_FCMD_REQ_STS_INIT == TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr));
            TEST_NfcFCmdQSetReqSts(ucTLun, eFCmdPri, ucWptr, LOCAL_FCMD_REQ_STS_RDY);
        }
        else
        {
            DBG_Printf("ucTLun %d ucWptr %d Data check error!\n", ucTLun, ucWptr);
            DBG_Getch();
        }
    }
    
    return;
}

FCMD_REQ_ENTRY *TEST_NfcFCmdQAllocReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    FCMD_REQ_ENTRY *ptReqEntry;
    
    ptReqEntry = TEST_NfcFCmdQGetEntryAddr(ucTLun, eFCmdPri, ucWptr);
#ifndef ACC_DEBUG
    COM_MemZero((U32*)ptReqEntry, sizeof(FCMD_REQ_ENTRY)/sizeof(U32));
#endif

    ASSERT(LOCAL_FCMD_REQ_STS_RDY == TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucWptr));
    TEST_NfcFCmdQSetReqSts(ucTLun, eFCmdPri, ucWptr, LOCAL_FCMD_REQ_STS_ALLOC);    

    return ptReqEntry;
}

#ifdef P_DBG_1
#define DBG_002_DEPTH 32
U32 g_DbgHostRd = 0;
U32 g_DbgFCMDQIndex[16] = {0};
LOCAL U32 g_DbgFCMDQCnt[16] = {0};
U32 g_DbgFCMDQStatus[16][DBG_002_DEPTH][1] = {0};
#endif

void TEST_NfcFCmdQPushReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    ASSERT(LOCAL_FCMD_REQ_STS_ALLOC == TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucLevel));
    
    TEST_NfcFCmdQSetReqSts(ucTLun, eFCmdPri, ucLevel, LOCAL_FCMD_REQ_STS_PUSH);

    #ifdef P_DBG_1    
    BOOL bNeed = FALSE;
    U32 ulIndex = g_DbgFCMDQIndex[ucTLun];
    if (TRUE == g_DbgHostRd)
    {
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] = TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, ucLevel);
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] <<= 8;
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] |= TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, (ucLevel+1)%FCMDQ_DEPTH);
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] <<= 8;
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] |= TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, (ucLevel+2)%FCMDQ_DEPTH);
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] <<= 8;
        g_DbgFCMDQStatus[ucTLun][ulIndex][0] |= TEST_NfcFCmdQGetReqSts(ucTLun, eFCmdPri, (ucLevel+3)%FCMDQ_DEPTH);

        if (0x02030303 != g_DbgFCMDQStatus[ucTLun][ulIndex][0])
            g_DbgFCMDQCnt[ucTLun]++;
            
        bNeed = TRUE;
    }
    #endif

    TEST_NfcFCmdQSetReqWptr(ucTLun, eFCmdPri, (ucLevel+1)%FCMDQ_DEPTH);

    #ifdef P_DBG_1
    if (TRUE == bNeed)
    {
        //g_DbgFCMDQStatus[ucTLun][ulIndex][1] <<= 8;
        //g_DbgFCMDQStatus[ucTLun][ulIndex][1] |= TEST_NfcFCmdQGetReqWptr(ucTLun, eFCmdPri);
        g_DbgFCMDQIndex[ucTLun] = (ulIndex+1) % DBG_002_DEPTH;
    }
    #endif
}

void TEST_NfcFCmdQAdaptPhyBlk(FCMD_REQ_ENTRY *ptReqEntry)
{
    U8 ucSPU, ucLunInSPU, ucIdx;
    FCMD_FLASH_DESC *ptFlashDesc;
    FCMD_FLASH_ADDR *ptFlashAddr;

    ptFlashDesc = &ptReqEntry->tFlashDesc;
    
    if (TRUE == ptReqEntry->bsTBRebuilding || TRUE == ptReqEntry->bsTableReq)
    {
        ptFlashDesc->bsPhyBlk = ptFlashDesc->bsVirBlk;
        return;
    }

    ucSPU = L2_GET_SPU(ptReqEntry->bsTLun);
    ucLunInSPU = L2_GET_LUN_IN_SPU(ptReqEntry->bsTLun);
    
    ptFlashDesc->bsPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, ptFlashDesc->bsVirBlk);

    if (FCMD_REQ_SUBTYPE_INTRNAL == ptReqEntry->bsReqSubType)
    {
        ASSERT(FCMD_REQ_TYPE_WRITE == ptReqEntry->bsReqType);
        ASSERT(FCMD_REQ_TLC_BLK == ptFlashDesc->bsBlkMod);
        
        for (ucIdx = 0; ucIdx < PG_PER_WL; ucIdx++)
        {
            ptFlashAddr = &ptReqEntry->atFlashAddr[ucIdx];                
            ptFlashAddr->bsPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, ptFlashAddr->bsVirBlk);
        }
    }
    return;
}

U16 TEST_NfcFCmdQGetPhyBlk(FCMD_REQ_ENTRY *ptReqEntry)
{
    U16 usPhyBlk;
   
    if (TRUE == ptReqEntry->bsTBRebuilding || TRUE == ptReqEntry->bsTableReq)
    {
        usPhyBlk = ptReqEntry->tFlashDesc.bsVirBlk;
    }
    else
    {
        U8 ucSPU, ucLunInSPU;
        ucSPU = L2_GET_SPU(ptReqEntry->bsTLun);
        ucLunInSPU = L2_GET_LUN_IN_SPU(ptReqEntry->bsTLun);
        
        usPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, ptReqEntry->tFlashDesc.bsVirBlk);
    }
    return usPhyBlk;
}


BOOL TEST_NfcHasPendingFCmd(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucDWPos, ucBitPos;
    BOOL bIsPending = FALSE;
    
    ucDWPos  = ucTLun / 32;
    ucBitPos = ucTLun % 32;
    
    bIsPending = COM_BitMaskGet(*(U32*)(g_pPendBMP + ucDWPos), ucBitPos);//(*(g_pPendBMP + ucDWPos))&(1 << ucBitPos);

    return bIsPending;
}

void TEST_NfcFCmdQSetPendingBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucDWPos, ucBitPos;
    
    ucDWPos  = ucTLun / 32;
    ucBitPos = ucTLun % 32;
    
    //*(g_pPendBMP + ucDWPos) |= BIT(ucBitPos);//(1 << ucBitPos);
    COM_BitMaskSet((U32*)(g_pPendBMP + ucDWPos), ucBitPos);
}

void TEST_NfcFCmdQClrPendingBitmap(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucDWPos, ucBitPos;
    
    ucDWPos  = ucTLun / 32;
    ucBitPos = ucTLun % 32;
    
    //*(g_pPendBMP + ucDWPos) &= ~(1 << ucBitPos);
    COM_BitMaskClear((U32*)(g_pPendBMP + ucDWPos), ucBitPos);
}

FCMD_REQ_ENTRY TEST_NfcFCmdQGetPendingEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    return (g_pLocalFCmdPend->aPendFCmd[ucTLun][eFCmdPri]);
}

BOOL TEST_NfcIsSLCMode(FCMD_FLASH_DESC *pFlashDesc)
{
    BOOL bSLCMode = (FCMD_REQ_SLC_BLK == pFlashDesc->bsBlkMod)? TRUE : FALSE;
    return bSLCMode;
}

void TEST_NfcFCmdUpdateMonitor(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, FCMD_REQ_ENTRY *pFCmdReqEntry)
{
    BOOL bSLCMode = TEST_NfcIsSLCMode(&pFCmdReqEntry->tFlashDesc);
    U16 usPhyPage = TEST_NfcGetReadPhyPageFromVirPage(pFCmdReqEntry->tFlashDesc.bsVirPage, bSLCMode);
    
    g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsPhyBlk     = pFCmdReqEntry->tFlashDesc.bsPhyBlk;
    g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsPhyPage    = usPhyPage;
    g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsReqType    = pFCmdReqEntry->bsReqType;
    g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsReqSubType = pFCmdReqEntry->bsReqSubType;
    g_pLocalFCmdMonitor->aFMItem[ucTLun][eFCmdPri].bsBlkMod     = pFCmdReqEntry->tFlashDesc.bsBlkMod;
}


/*  end of this file  */
