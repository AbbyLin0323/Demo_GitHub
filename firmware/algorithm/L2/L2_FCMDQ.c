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
* File Name    : L2_FCMDQ.c
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.7.14
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "HAL_Xtensa.h"
#include "L2_Interface.h"
#include "L2_FCMDQ.h"
#include "HAL_MultiCore.h"
#include "L2_TableBBT.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
#define TL_FIFLE_NUM L2_FCMDQ_c

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/
extern BOOL g_BootUpOk;
extern BOOL bTBRebuildPending;

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/
MCU12_VAR_ATTR FCMD_REQ      *g_ptFCmdReq;    // Allocated from shared Dsram1
MCU12_VAR_ATTR FCMD_REQSTS   *g_ptFCmdReqSts; // Allocated from OTFB
MCU12_VAR_ATTR volatile FCMD_REQ_DPTR *g_ptFCmdReqDptr;// Allocated from shared Dsram1

LOCAL U32 l_ulFCmdNotFullBitmap = INVALID_8F; // for simple, limitation: SUBSYSTEM_LUN_NUM <= 32
/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*==============================================================================
Func Name  : L2_FCmdReqInit
Input      : void  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.7.12 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_FCmdReqInit(void)
{
    COM_MemZero((U32*)g_ptFCmdReq, FCMD_REQ_SZ/sizeof(U32));
}

/*==============================================================================
Func Name  : L2_FCmdReqStsInit
Input      : void  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.7.12 JasonGuo create function
==============================================================================*/
void MCU12_DRAM_TEXT L2_FCmdReqStsInit(U32 ucMcuId)
{
    U32 ulTLun, ulPriIdx, ulDepthIdx;
    FCMD_REQSTS_Q *pReqStsQ;

    for (ulTLun = 0; ulTLun < SUBSYSTEM_LUN_NUM; ulTLun++)
    {
        for (ulPriIdx = 0; ulPriIdx < FCMD_REQ_PRI_NUM; ulPriIdx++)
        {
            pReqStsQ = &g_ptFCmdReqSts->atReqStsQ[ulTLun][ulPriIdx];
            for (ulDepthIdx = 0; ulDepthIdx < FCMDQ_DEPTH; ulDepthIdx++)
            {
                pReqStsQ->aReqStatus[ulDepthIdx] = (MCU1_ID == ucMcuId) ? INVALID_2F : FCMD_REQ_STS_INIT;
            }
        }
    }

    return;
}

/*==============================================================================
Func Name  : L2_FCmdReqDptrInit
Input      : void  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.7.12 JasonGuo create function
==============================================================================*/
LOCAL void MCU1_DRAM_TEXT L2_FCmdReqDptrInit(void)
{
    COM_MemZero((U32*)g_ptFCmdReqDptr, FCMD_REQ_DPTR_SZ/sizeof(U32));    
}

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/
/*==============================================================================
Func Name  : L2_FCMDQReqInit
Input      : void  
Output     : NONE
Return Val : 
Discription: 
Usage      : Called in L2_TaskInit after the memory allocation done.
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
void MCU1_DRAM_TEXT L2_FCMDQReqInit(void)
{
    L2_FCmdReqInit();
    L2_FCmdReqStsInit(MCU1_ID);
    L2_FCmdReqDptrInit();
}

/*==============================================================================
Func Name  : L2_FCMDQGetReqWptr
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
U8 L2_FCMDQGetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    return g_ptFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucWptr;
}

/*==============================================================================
Func Name  : L2_FCMDQSetReqWptr
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
             U8 ucWptr
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
void L2_FCMDQSetReqWptr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    g_ptFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucWptr = ucWptr;
    //FIRMWARE_LogInfo("TLun#%d MCU%d Wptr#%d.Rptr#%d\n", ucTLun, HAL_GetMcuId(), ucWptr, g_ptFCmdReqDptr->atReqQDptr[ucTLun][eFCmdPri].ucRptr);
    
    // update the FCmdNotFullBitmap
    if (TRUE != L2_FCMDQIsWptrFree(ucTLun, eFCmdPri, ucWptr))
    {
        l_ulFCmdNotFullBitmap &= ~(1 << ucTLun);
    }

    return;
}

/*==============================================================================
Func Name  : L2_FCMDQGetReqStsAddr
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
             U8 ucLevel         
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.18 JasonGuo create function
==============================================================================*/
U32 L2_FCMDQGetReqStsAddr(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    return (U32)&g_ptFCmdReqSts->atReqStsQ[ucTLun][eFCmdPri].aReqStatus[ucLevel];
}

/*==============================================================================
Func Name  : L2_FCMDQGetReqSts
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
             U8 ucLevel         
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
FCMD_REQ_STS L2_FCMDQGetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    return g_ptFCmdReqSts->atReqStsQ[ucTLun][eFCmdPri].aReqStatus[ucLevel];
}

/*==============================================================================
Func Name  : L2_FCMDQSetReqSts
Input      : U8 ucTLun             
             FCMD_REQ_PRI eFCmdPri     
             U8 ucLevel            
             FCMD_REQ_STS eReqSts  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
void L2_FCMDQSetReqSts(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel, FCMD_REQ_STS eReqSts)
{
    g_ptFCmdReqSts->atReqStsQ[ucTLun][eFCmdPri].aReqStatus[ucLevel] = eReqSts;
    //FIRMWARE_LogInfo("TLun#%d MCU%d Level#%d Sts#%d.\n", ucTLun, HAL_GetMcuId(), ucLevel, eReqSts);
}

/*==============================================================================
Func Name  : L2_FCMDQIsWptrFree
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
             U8 ucWptr          
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
BOOL L2_FCMDQIsWptrFree(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucWptr)
{
    BOOL bIsFree = FALSE;
    
    if (FCMD_REQ_STS_INIT == L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucWptr))
    {
        bIsFree = TRUE;

        #if 0 //Henry mark: not support cache write error handling yet (FLASH_CACHE_OPERATION)
        //#ifdef FLASH_CACHE_OPERATION
        U8 ucNextWptr;
        FCMD_REQ_ENTRY *ptReqEntry;

        ptReqEntry = L2_FCMDQGetReqEntry(ucTLun, eFCmdPri, ucWptr);
        if (TRUE == ptReqEntry->bsPrePgEn)
        {
            ucNextWptr = (ucWptr + 1) % FCMDQ_DEPTH;
            if (FCMD_REQ_STS_INIT != L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucNextWptr))
            {
                bIsFree = FALSE;
            }
        }
        #endif
    }
    
    return bIsFree;
}

/*==============================================================================
Func Name  : L2_FCMDQIsNotFull
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
BOOL L2_FCMDQIsNotFull(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucWptr;

    ucWptr = L2_FCMDQGetReqWptr(ucTLun, eFCmdPri);
    
    return L2_FCMDQIsWptrFree(ucTLun, eFCmdPri, ucWptr);
}

BOOL L2_FCMDQNotFull(U8 ucTLun)
{
    return L2_FCMDQIsNotFull(ucTLun, 0);
}
/*==============================================================================
Func Name  : L2_FCMDQGetNotFullBitmap
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
Output     : NONE
Return Val : 
Discription: 
Usage      : Only support 32 Lun max.
History    : 
    1. 2016.7.14 JasonGuo create function
    2. 2017.2.28 JasonGuo reduce the search cycle cnt
==============================================================================*/
U32 L2_FCMDQGetNotFullBitmap(FCMD_REQ_PRI eFCmdPri)
{
    U32 ucTLun;    

    if (INVALID_8F != l_ulFCmdNotFullBitmap)
    {        
        for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
        {
            if (0 == (l_ulFCmdNotFullBitmap & (1 << ucTLun)))
            {
                if (TRUE == L2_FCMDQIsNotFull((U32)ucTLun, eFCmdPri))
                {
                    l_ulFCmdNotFullBitmap |= (1 << ucTLun);
                }
            }
        }
    }

    if (SUBSYSTEM_LUN_NUM < 32)
    {
        return l_ulFCmdNotFullBitmap & INVALID_BIT(SUBSYSTEM_LUN_NUM);
    }
    else
    {
        return l_ulFCmdNotFullBitmap & INVALID_8F;
    }
}

/*==============================================================================
Func Name  : L2_FCMDQIsEmpty
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
BOOL L2_FCMDQIsEmpty(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucPtr;
    BOOL bIsEmpty = TRUE;

    for (ucPtr = 0; ucPtr < FCMDQ_DEPTH; ucPtr++)
    {
        if (FCMD_REQ_STS_INIT != L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucPtr))
        {
            bIsEmpty = FALSE;
            break;
        }
    }

    return bIsEmpty;
}

/*==============================================================================
Func Name  : L2_FCMDQGetEmptyBitmap
Input      : FCMD_REQ_PRI eFCmdPri  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
U32 L2_FCMDQGetEmptyBitmap(FCMD_REQ_PRI eFCmdPri)
{
    U8 ucTLun;    
    U32 ulEmptyBitmap = 0;

    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        if (TRUE == L2_FCMDQIsEmpty(ucTLun, eFCmdPri))
        {
            ulEmptyBitmap |= (1<<ucTLun);
        }
    }

    return ulEmptyBitmap;
}

/*==============================================================================
Func Name  : L2_FCMDQIsAllEmpty
Input      : void  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
BOOL L2_FCMDQIsAllEmpty(void)
{
    U8 ucTLun, ucPriIdx;
    
    for (ucTLun = 0; ucTLun < SUBSYSTEM_LUN_NUM; ucTLun++)
    {
        for (ucPriIdx = 0; ucPriIdx < FCMD_REQ_PRI_NUM; ucPriIdx++)
        {
            if (FALSE == L2_FCMDQIsEmpty(ucTLun, ucPriIdx))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/*==============================================================================
Func Name  : L2_FCMDQGetReqEntry
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
             U8 ucLevel         
Output     : NONE
Return Val : FCMD_REQ_ENTRY
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
FCMD_REQ_ENTRY *L2_FCMDQGetReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
    return &g_ptFCmdReq->atReqQ[ucTLun][eFCmdPri].atReq[ucLevel];
}

/*==============================================================================
Func Name  : L2_FCMDQAllocReqEntry
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
Output     : NONE
Return Val : FCMD_REQ_ENTRY
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
FCMD_REQ_ENTRY *L2_FCMDQAllocReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri)
{
    U8 ucWptr;
    FCMD_REQ_ENTRY *ptReqEntry;
    
    ucWptr = L2_FCMDQGetReqWptr(ucTLun, eFCmdPri);
    ptReqEntry = L2_FCMDQGetReqEntry(ucTLun, eFCmdPri, ucWptr);
    COM_MemZero((U32*)ptReqEntry, sizeof(FCMD_REQ_ENTRY)/sizeof(U32));

    ptReqEntry->bsTLun     = ucTLun;
    ptReqEntry->bsFCmdPri  = eFCmdPri;
    ptReqEntry->bsReqPtr   = ucWptr;
#ifndef L1_FAKE
    ptReqEntry->bsBootUpOk = g_BootUpOk;
    ptReqEntry->bsTBRebuilding = bTBRebuildPending;
#else
    ptReqEntry->bsBootUpOk = TRUE;
    ptReqEntry->bsTBRebuilding = FALSE;
#endif

#ifdef SIM
    ASSERT(FCMD_REQ_STS_INIT == L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucWptr));
    L2_FCMDQSetReqSts(ucTLun, eFCmdPri, ucWptr, FCMD_REQ_STS_ALLOC);    
#endif

    return ptReqEntry;
}

#ifdef SIM
/*==============================================================================
Func Name  : L2_FCMDQPrtReqEntry
Input      : U8 ucTLun   
             U8 ucLevel  
Output     : NONE
Return Val : LOCAL
Discription: 
Usage      : 
History    : 
    1. 2016.8.11 JasonGuo create function
==============================================================================*/
LOCAL void L2_FCMDQPrtReqEntry(U8 ucTLun, U8 ucLevel)
{

    FCMD_REQ_ENTRY *ptReqEntry;

    ptReqEntry = L2_FCMDQGetReqEntry(ucTLun, 0, ucLevel);

#if 0
    FIRMWARE_LogInfo("TLun%d Blk%d_%d Page%d CmdType%d_%d Pln%d BlkMod%d UpdtMod%d ReqPtr%d. SendFCmd.\n",
        ptReqEntry->bsTLun,
        ptReqEntry->tFlashDesc.bsVirBlk,
        ptReqEntry->tFlashDesc.bsPhyBlk,
        ptReqEntry->tFlashDesc.bsVirPage,
        ptReqEntry->bsReqType,
        ptReqEntry->bsReqSubType,
        ptReqEntry->tFlashDesc.bsPlnNum,
        ptReqEntry->tFlashDesc.bsBlkMod,
        ptReqEntry->bsReqUptMod,
        ptReqEntry->bsReqPtr);
#endif

    ASSERT(ptReqEntry->bsReqPtr == ucLevel);
    ASSERT(L2_FCMDQGetReqWptr(ucTLun, 0) == ucLevel);

#if 0
    /*
    Host Read (SLC/TLC)
    L1 Merge Read (SLC/TLC)
    L1 Prefetch Read (SLC/TLC)
    L2 Local Read Red Only (SLC/TLC)
    L2 Local Single-Plane Read (SLC)
    L2 Local Multi-Plane Read (SLC/TLC)
    L2 Normal Write (SLC)
    L2 Local Single-Plane Write (SLC)
    L2 Local Multi-Plane Write (SLC)
    L2 Local Multi-Plane External Copy (TLC)
    L2 Local Multi-Plane Internal Copy (TLC)
    L2 Local Single-Plane Erase (SLC)
    L2 Local Single-Plane Erase (TLC)
    L2 Local Multi-Plane Erase (SLC/TLC)
    */
    switch (ptReqEntry->bsReqType)
    {
        case FCMD_REQ_TYPE_READ:
        {
            if (FCMD_REQ_SUBTYPE_SINGLE == ptReqEntry->bsReqSubType)
            {
                
            }
            else if (FCMD_REQ_SUBTYPE_NORMAL == ptReqEntry->bsReqSubType)
            {

            }
            else
            {
                DBG_Printf("Read-FCmd Req SubType %d Error.\n", ptReqEntry->bsReqSubType);
                DBG_Getch();
            }
            break;
        }
        case FCMD_REQ_TYPE_WRITE:
        {
            break;
        }
        case FCMD_REQ_TYPE_ERASE:
        {
            break;
        }
        case FCMD_REQ_TYPE_CHK_IDLE:
        {
            break;
        }
        case FCMD_REQ_TYPE_SELF_TEST:
        {
            break;
        }
        default:
        {
            DBG_Printf("FCmd Req Type %d Error.\n", ptReqEntry->bsReqType);
            DBG_Getch();
        }
    }
#endif

    return;
}
#endif

/*==============================================================================
Func Name  : L2_FCMDQPushReqEntry
Input      : U8 ucTLun          
             FCMD_REQ_PRI eFCmdPri  
             U8 ucLevel         
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
void L2_FCMDQPushReqEntry(U8 ucTLun, FCMD_REQ_PRI eFCmdPri, U8 ucLevel)
{
#ifdef SIM
    extern BOOL L3_DbgL2SendCntAdd(U8 ucTLun);
    if (TRUE == L3_DbgL2SendCntAdd(ucTLun))
    {
        L2_FCMDQPrtReqEntry(ucTLun, ucLevel);
    }    

    //Due to the ASSERT will access the OTFB and it is just debugging feature.
    //Suggest to remove this ASSERT by using #ifdef SIM
    ASSERT(FCMD_REQ_STS_ALLOC == L2_FCMDQGetReqSts(ucTLun, eFCmdPri, ucLevel));
#endif
    L2_FCMDQSetReqSts(ucTLun, eFCmdPri, ucLevel, FCMD_REQ_STS_PUSH);
    
    L2_FCMDQSetReqWptr(ucTLun, eFCmdPri, (ucLevel + 1) % FCMDQ_DEPTH);

    return;
}

/*==============================================================================
Func Name  : L2_FCMDQAdaptPhyBlk
Input      : FCMD_REQ_ENTRY *ptReqEntry  
Output     : NONE
Return Val : 
Discription: 
Usage      : 
History    : 
    1. 2016.7.14 JasonGuo create function
==============================================================================*/
void L2_FCMDQAdaptPhyBlk(FCMD_REQ_ENTRY *ptReqEntry)
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
#ifdef ERRH_MODIFY_PENDING_CMD
    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_ERRH_PEND_CMD);
#endif
    ptFlashDesc->bsPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, ptFlashDesc->bsVirBlk);
    if (ptFlashDesc->bsPhyBlk >= BBT_BLK_PER_PLN)
    {
        DBG_Printf("L2_FCMDQAdaptPhyBlk: Error: ucSPU %d ucLunInSPU %d VirBlock 0x%x PhyBlock 0x%x BBT_BLK_PER_PLN 0x%x.\n",
            ucSPU, ucLunInSPU, ptFlashDesc->bsVirBlk, ptFlashDesc->bsPhyBlk, BBT_BLK_PER_PLN);
        DBG_Getch();
    }
    if (FCMD_REQ_SUBTYPE_INTRNAL == ptReqEntry->bsReqSubType)
    {
        for (ucIdx = 0; ucIdx < PG_PER_WL; ucIdx++)
        {
            ptFlashAddr = &ptReqEntry->atFlashAddr[ucIdx];                
            ptFlashAddr->bsPhyBlk = L2_VBT_GetPhysicalBlockAddr(ucSPU, ucLunInSPU, ptFlashAddr->bsVirBlk);
        }
    }
#ifdef ERRH_MODIFY_PENDING_CMD
    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_ERRH_PEND_CMD);
#endif
    return;
}


/*====================End of this file========================================*/

