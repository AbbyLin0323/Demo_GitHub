/****************************************************************************
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
*****************************************************************************
Filename    :L1_Interface.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    14:57:15
Description :
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/
#include "HAL_Inc.h"
#include "L1_Inc.h"
#include "COM_Memory.h"
#include "HAL_TraceLog.h"
#include "COM_BitMask.h"
#include "L2_PMTManager.h"
#include "L2_Boot.h"

//specify file name for Trace Log
#define TL_FILE_NUM  L1_Interface_c

GLOBAL  U32 g_ulReadBufReqPuBitMap;
GLOBAL  U32 g_ulWriteBufReqPuBitMap;

GLOBAL  U32 g_ulReadBufReqFullPuBitMap;
GLOBAL  U32 g_ulWriteBufReqFullPuBitMap;

GLOBAL  U32 g_aucReadBufReqCount[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  U32 g_aucWriteBufReqCount[SUBSYSTEM_SUPERPU_MAX];

GLOBAL  BUF_REQ_FIFO g_LowPrioEntry[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  BUF_REQ_FIFO *gpLowPrioEntry[SUBSYSTEM_SUPERPU_MAX];

GLOBAL  U8 LowPrioFifoHead[SUBSYSTEM_SUPERPU_MAX];
GLOBAL  U8 LowPrioFifoTail[SUBSYSTEM_SUPERPU_MAX];

/* g_aReadBufReqEntry[SuperPu] is an array of Read BufReq */
/* g_aReadBufReqEntry[SuperPu][ReadBufReqID] is a Read BufReq */
GLOBAL BUF_REQ_READ *g_ReadBufReqEntry[SUBSYSTEM_SUPERPU_MAX];
/* L1 get empty Read BufReq from g_FreeReadBufReqLink, insert filled Read BufReq to g_HighPrioLink */
/* L2 get HighPrio BufReq from g_HighPrioLink, recycle Read BufReq to g_FreeReadBufReqLink after processing it */
GLOBAL BUF_REQ_READ_LINK g_FreeReadBufReqLink[SUBSYSTEM_SUPERPU_MAX];
GLOBAL BUF_REQ_READ_LINK g_HighPrioLink[SUBSYSTEM_SUPERPU_MAX];

extern void L1_FillHostAddrInfo(SUBCMD *pSubCmd, BUF_REQ_READ *pBufReq);
extern void L1_DbgPrintWriteBufReq(BUF_REQ_WRITE *pBufReq);

U8 g_Set1stDataReady;
void MCU1_DRAM_TEXT L1_BufReqFifoSram0Map(U32 *pFreeSramBase)
{
    U32 ulFreeSramBase;
    U8 ucSuperPu;

    // align the start address 
    ulFreeSramBase = *pFreeSramBase;
    COM_MemAddr16DWAlign( &ulFreeSramBase );

    // for each pu, allocate a high priority fifo and a low priority fifo
    for(ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        g_ReadBufReqEntry[ucSuperPu] = (BUF_REQ_READ *)ulFreeSramBase;
        COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign(PRIO_FIFO_DEPTH * sizeof(BUF_REQ_READ)));

        // allocate low priority fifo
        gpLowPrioEntry[ucSuperPu]  = &g_LowPrioEntry[ucSuperPu];
        gpLowPrioEntry[ucSuperPu]->tBufReq = ( BUF_REQ_WRITE * )ulFreeSramBase;
        COM_MemIncBaseAddr(&ulFreeSramBase, COM_MemSize16DWAlign( PRIO_FIFO_DEPTH*sizeof(gpLowPrioEntry[ucSuperPu]->tBufReq[0]) ) );
    }

    COM_MemAddr16DWAlign( &ulFreeSramBase );
    *pFreeSramBase = ulFreeSramBase;

    return;
}

U8 L1_IsReadBufReqPending(U8 ucSuperPu)
{
    // this function check g_ulReadBufReqPuBitMap to see if the pu
    // has any pending read buffer requests

    return COM_BitMaskGet(g_ulReadBufReqPuBitMap, ucSuperPu);
}

U8 L1_IsWriteBufReqPending(U8 ucSuperPu)
{
    // this function check g_ulWriteBufReqPuBitMap to see if the pu
    // has any pending write buffer requests

    return COM_BitMaskGet(g_ulWriteBufReqPuBitMap, ucSuperPu);
}

U32 L1_GetRWBufReqPuBitMap(void)
{
    return (g_ulWriteBufReqPuBitMap | g_ulReadBufReqPuBitMap);
}

/****************************************************************************
Function  : L1_BufReqInit
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :

****************************************************************************/
void MCU1_DRAM_TEXT L1_BufReqInit(void)
{
    U8 ucSuperPu;
    U8 ucReadBufReqID;

    // for all pus, reset their buffer request related data structures
    for(ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ++ucSuperPu)
    {
        HAL_DMAESetValue((U32)g_ReadBufReqEntry[ucSuperPu], COM_MemSize16DWAlign(PRIO_FIFO_DEPTH*sizeof(BUF_REQ_READ)), 0);
        HAL_DMAESetValue((U32)gpLowPrioEntry[ucSuperPu]->tBufReq, COM_MemSize16DWAlign(PRIO_FIFO_DEPTH*sizeof(gpLowPrioEntry[ucSuperPu]->tBufReq[0])), 0);

        /* init Free Read BufReq Link, 0 -> 1 -> ... -> PRIO_FIFO_DEPTH -1 */
        g_FreeReadBufReqLink[ucSuperPu].ucHeadID = 0;
        g_FreeReadBufReqLink[ucSuperPu].ucTailID = PRIO_FIFO_DEPTH - 1;
        for (ucReadBufReqID = 0; ucReadBufReqID < PRIO_FIFO_DEPTH - 1; ucReadBufReqID++)
        {
            g_ReadBufReqEntry[ucSuperPu][ucReadBufReqID].ucNextBufReqID = ucReadBufReqID + 1;
        }
        g_ReadBufReqEntry[ucSuperPu][PRIO_FIFO_DEPTH - 1].ucNextBufReqID = INVALID_2F;

        /* init High Priority Link, empty */
        g_HighPrioLink[ucSuperPu].ucHeadID = INVALID_2F;
        g_HighPrioLink[ucSuperPu].ucTailID = INVALID_2F;
       
        LowPrioFifoHead[ucSuperPu] = 0;
        LowPrioFifoTail[ucSuperPu] = 0;

        g_aucReadBufReqCount[ucSuperPu]  = 0;
        g_aucWriteBufReqCount[ucSuperPu] = 0;
    }

    // reset the buffer request bitmap
    g_ulReadBufReqPuBitMap  = 0;
    g_ulWriteBufReqPuBitMap = 0;

    // reset the buffer request full bitmap
    g_ulReadBufReqFullPuBitMap = 0;
    g_ulWriteBufReqFullPuBitMap = 0;

    return;
}

void MCU1_DRAM_TEXT L1_BufReqWarmInit(void)
{
    U8 ucSuperPu;
    U8 ucReadBufReqID;

    // for all pus, reset their buffer request related data structures
    for(ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ++ucSuperPu)
    {
        HAL_DMAESetValue((U32)g_ReadBufReqEntry[ucSuperPu], COM_MemSize16DWAlign(PRIO_FIFO_DEPTH*sizeof(BUF_REQ_READ)), 0);
        HAL_DMAESetValue((U32)gpLowPrioEntry[ucSuperPu]->tBufReq, COM_MemSize16DWAlign(PRIO_FIFO_DEPTH*sizeof(gpLowPrioEntry[ucSuperPu]->tBufReq[0])), 0);

        /* init Free Read BufReq Link, 0 -> 1 -> ... -> PRIO_FIFO_DEPTH -1 */
        g_FreeReadBufReqLink[ucSuperPu].ucHeadID = 0;
        g_FreeReadBufReqLink[ucSuperPu].ucTailID = PRIO_FIFO_DEPTH - 1;
        for (ucReadBufReqID = 0; ucReadBufReqID < PRIO_FIFO_DEPTH - 1; ucReadBufReqID++)
        {
            g_ReadBufReqEntry[ucSuperPu][ucReadBufReqID].ucNextBufReqID = ucReadBufReqID + 1;
        }
        g_ReadBufReqEntry[ucSuperPu][PRIO_FIFO_DEPTH - 1].ucNextBufReqID = INVALID_2F;

        /* init High Priority Link, empty */
        g_HighPrioLink[ucSuperPu].ucHeadID = INVALID_2F;
        g_HighPrioLink[ucSuperPu].ucTailID = INVALID_2F;
        
        LowPrioFifoHead[ucSuperPu] = 0;
        LowPrioFifoTail[ucSuperPu] = 0;

        g_aucReadBufReqCount[ucSuperPu]  = 0;
        g_aucWriteBufReqCount[ucSuperPu] = 0;
    }

    // reset the buffer request bitmap
    g_ulReadBufReqPuBitMap  = 0;
    g_ulWriteBufReqPuBitMap = 0;

    // reset the buffer request full bitmap
    g_ulReadBufReqFullPuBitMap = 0;
    g_ulWriteBufReqFullPuBitMap = 0;

    return;
}

U32 L1_FreeReadBufReqLinkEmpty(U8 ucSuperPu)
{
    U8 ucHeadID = g_FreeReadBufReqLink[ucSuperPu].ucHeadID;

#ifdef SIM
    U8 ucTailID = g_FreeReadBufReqLink[ucSuperPu].ucTailID;

    if (((INVALID_2F == ucHeadID) && (INVALID_2F != ucTailID))
        || ((INVALID_2F != ucHeadID) && (INVALID_2F == ucTailID)))
    {
        DBG_Printf("L1_HighPrioLinkEmpty(%d) error : Head = %d, Tail = %d \n", ucSuperPu, ucHeadID, ucTailID);
        DBG_Getch();
    }
#endif
    return (INVALID_2F == ucHeadID) ? TRUE : FALSE;
}

/****************************************************************************
Function  : L1_LowPrioFifoFull
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_LowPrioFifoFull(U8 ucSuperPu)
{
    // check if the low priority fifo is full

    U8 ucNextTail;

    // check if tail+1 == head, if it is, it means the fifo is
    // full
    ucNextTail = ((LowPrioFifoTail[ucSuperPu]+1) == PRIO_FIFO_DEPTH)
        ? 0 : (LowPrioFifoTail[ucSuperPu]+1);

    return (ucNextTail == LowPrioFifoHead[ucSuperPu]) ? TRUE : FALSE;
}

/****************************************************************************
Function  : L1_HighPrioLinkEmpty
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_HighPrioLinkEmpty(U8 ucSuperPu)
{
    U8 ucHeadID = g_HighPrioLink[ucSuperPu].ucHeadID;

#ifdef SIM
    U8 ucTailID = g_HighPrioLink[ucSuperPu].ucTailID;

    if (((INVALID_2F == ucHeadID) && (INVALID_2F != ucTailID))
        || ((INVALID_2F != ucHeadID) && (INVALID_2F == ucTailID)))
    {
        DBG_Printf("L1_HighPrioLinkEmpty(%d) error : Head = %d, Tail = %d \n", ucSuperPu, ucHeadID, ucTailID);
        DBG_Getch();
    }
#endif
    return (INVALID_2F == ucHeadID) ? TRUE : FALSE;
}

/****************************************************************************
Function  : L1_LowPrioFifoEmpty
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_LowPrioFifoEmpty(U8 ucSuperPu)
{
    // check if the fifo is empty by checking if head == tail
    return (LowPrioFifoHead[ucSuperPu] == LowPrioFifoTail[ucSuperPu])
        ? TRUE : FALSE;
}

/****************************************************************************
Function  : L1_BufReqEmpty
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_BufReqEmpty(void)
{
    // check if the buffer request fifos of all pus are empty

    U8 ucSuperPu;
    U8 ucAllEmpty;

    ucAllEmpty = TRUE;
    for(ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ++ucSuperPu)
    {
        if(L1_LowPrioFifoEmpty(ucSuperPu) == FALSE || L1_HighPrioLinkEmpty(ucSuperPu) == FALSE
            || g_ulReadBufReqPuBitMap != 0)
        {
            // break immediately if there's a non-empty fifo
            ucAllEmpty = FALSE;
            break;
        }
    }

    return ucAllEmpty;
}

U32 L1_AllHighPrioLinkEmpty(void)
{
    U8 ucSuperPu;

    if (g_ulReadBufReqPuBitMap != 0)
    {
        return FALSE;
    }

    for (ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ucSuperPu++)
    {
        if (FALSE == L1_HighPrioLinkEmpty(ucSuperPu))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************
Name        :L1_GetLowPrioFifoPendingCnt
Input        :
Output      :
Author      :BlakeZhang
Date        :2012.12.11
Description :get total pending request count of Low Prio Fifo.
Others      :
Modify      :
****************************************************************************/
U32 L1_GetLowPrioFifoPendingCnt(void)
{
    // return the number of pending low buffer requests in the
    // system
    U32 ulPendingCnt = 0;
    U8 ucSuperPu;

    // get the the number of pending low buffer requests for
    // each pu
    for(ucSuperPu = 0; ucSuperPu < SUBSYSTEM_SUPERPU_NUM; ++ucSuperPu)
    {
        if (TRUE != L1_LowPrioFifoEmpty(ucSuperPu))
        {
            if (LowPrioFifoTail[ucSuperPu] >= LowPrioFifoHead[ucSuperPu])
            {
                ulPendingCnt += LowPrioFifoTail[ucSuperPu] - LowPrioFifoHead[ucSuperPu];
            }
            else
            {
                ulPendingCnt += PRIO_FIFO_DEPTH + LowPrioFifoTail[ucSuperPu] - LowPrioFifoHead[ucSuperPu];
            }
        }
    }
    
    return ulPendingCnt;
}

BUF_REQ_READ* L1_GetReadBufReq(U8 ucSuperPu, U8 ucReadBufReqID)
{
    return &g_ReadBufReqEntry[ucSuperPu][ucReadBufReqID];
}

/*----------------------------------------------------------------------------
Name: L1_AllocateReadBufReq
Description:
  Allocate a Read BufReq node from the Head of Free Read BufReq Link
Input Param:
  U8 ucSuperPu
Output Param:
  none
Return Value:
  U8 : INVALID_2F -- no Read BufReq can be allocated
       others -- the Read BufReq ID allocated
Usage:
  Before L1 fill a Read BufReq
History:
------------------------------------------------------------------------------*/
U8 L1_AllocateReadBufReq(U8 ucSuperPu)
{
    U8 ucFreeHeadID = g_FreeReadBufReqLink[ucSuperPu].ucHeadID;
    U8 ucFreeTailID = g_FreeReadBufReqLink[ucSuperPu].ucTailID;
    BUF_REQ_READ *pReadBufReq;

    if (INVALID_2F == ucFreeHeadID)
    {
        /* Free Read BufReq Link is empty */
#ifdef SIM
        if (INVALID_2F != ucFreeTailID)
        {
            DBG_Printf("L1_AllocateReadBufReq(%d) error 1!\n", ucSuperPu);
            DBG_Getch();
        }
#endif
        return INVALID_2F;
    }
    else
    {
#ifdef SIM
        if (INVALID_2F == ucFreeTailID)
        {
            DBG_Printf("L1_AllocateReadBufReq(%d) error 2!\n", ucSuperPu);
            DBG_Getch();
        }
#endif
    }

    /* get the Read BufReq from Head of Free Link */
    pReadBufReq = &g_ReadBufReqEntry[ucSuperPu][ucFreeHeadID];

    /* remove this node from Free Link */
    if (ucFreeTailID == ucFreeHeadID)
    {
        /* this is the last BufReq in Free Link, need update Tail */
        g_FreeReadBufReqLink[ucSuperPu].ucTailID = INVALID_2F;

        COM_BitMaskSet(&g_ulReadBufReqFullPuBitMap, ucSuperPu);
    }
    /* update Head of Free Link */
    g_FreeReadBufReqLink[ucSuperPu].ucHeadID = pReadBufReq->ucNextBufReqID;

    COM_MemZero((U32*)pReadBufReq, sizeof(BUF_REQ_READ) / 4);

    return ucFreeHeadID;
}

/*----------------------------------------------------------------------------
Name: L1_InsertReadBufReqToHighPrioLinkTail
Description:
  Insert a Read BufReq node to the Tail of High Priority Link
Input Param:
  U8 ucSuperPu
  U8 ucReadBufReqID
Output Param:
  none
Return Value:
  void
Usage:
  After L1 fill a Read BufReq
History:
------------------------------------------------------------------------------*/
void L1_InsertBufReqToHighPrioLinkTail(U8 ucSuperPu, U8 ucReadBufReqID)
{
    U8 ucPreTailID = g_HighPrioLink[ucSuperPu].ucTailID;

    if (INVALID_2F == ucPreTailID)
    {
#ifdef SIM
        if (INVALID_2F != g_HighPrioLink[ucSuperPu].ucHeadID)
        {
            DBG_Printf("L1_RecycleReadBufReq(%d, %d) error 1!\n", ucSuperPu, ucReadBufReqID);
            DBG_Getch();
        }
#endif
        /* High Priority Link is empty, need update Head */
        g_HighPrioLink[ucSuperPu].ucHeadID = ucReadBufReqID;

        COM_BitMaskSet(&g_ulReadBufReqPuBitMap, ucSuperPu);
    }
    else
    {
#ifdef SIM
        if (INVALID_2F == g_HighPrioLink[ucSuperPu].ucHeadID)
        {
            DBG_Printf("L1_RecycleReadBufReq(%d, %d) error 2!\n", ucSuperPu, ucReadBufReqID);
            DBG_Getch();
        }
#endif
        /* Update next pointer of previous High Priority Link Tail */
        g_ReadBufReqEntry[ucSuperPu][ucPreTailID].ucNextBufReqID = ucReadBufReqID;
    }

    /* update Tail of High Priority Link */
    g_HighPrioLink[ucSuperPu].ucTailID = ucReadBufReqID;
    g_ReadBufReqEntry[ucSuperPu][ucReadBufReqID].ucNextBufReqID = INVALID_2F;

    return;
}

/****************************************************************************
Function  : L1_LowPrioFifoGetBufReq
Input     : 
Output    :
Return    : 
Purpose   : 
Reference : Get a BufReq for LowPrioFifo Tail
****************************************************************************/
BUF_REQ_WRITE* L1_LowPrioFifoGetBufReq(U8 ucSuperPu)
{
    // return a free buffer request entry

#ifdef SIM
    // double check if the fifo is full
    if(L1_LowPrioFifoFull(ucSuperPu) == TRUE)
    {
        // this should never happen
        DBG_Printf("L1_LowPrioFifoGetBufReq Error: L1_LowPrioFifoFull!!!\n");
        DBG_Getch();
    }
#endif

    return &( (gpLowPrioEntry[ucSuperPu])->tBufReq[LowPrioFifoTail[ucSuperPu]] );
}

/****************************************************************************
Function  : L1_LowPrioMoveTailPtr
Input     : 
Output    :
Return    : 
Purpose   : 
Reference : move LowPrioFifo Tailptr
****************************************************************************/
void L1_LowPrioMoveTailPtr(U8 ucSuperPu)
{
    // move the tail of the fifo, this function should be called
    // when we're adding a new buffer request to the fifo
    
    // update the tail pointer
    LowPrioFifoTail[ucSuperPu] = ((LowPrioFifoTail[ucSuperPu]+1) == PRIO_FIFO_DEPTH) ? 0 : (LowPrioFifoTail[ucSuperPu]+1);

    // update the buffer request count and the buffer request
    // bitmap
    if(g_aucWriteBufReqCount[ucSuperPu] == 0)
    {
        COM_BitMaskSet(&g_ulWriteBufReqPuBitMap, ucSuperPu);
    }
    g_aucWriteBufReqCount[ucSuperPu]++;

    return;
}

U8 L1_GetHighPrioLinkHead(U8 ucSuperPu)
{
    return g_HighPrioLink[ucSuperPu].ucHeadID;
}

U8 L1_GetHighPrioLinkNextNode(U8 ucSuperPu, U8 ucCurBufReqID)
{
    return g_ReadBufReqEntry[ucSuperPu][ucCurBufReqID].ucNextBufReqID;
}

void L1_RemoveBufReqFromHighPrioLink(U8 ucSuperPu, U8 ucPreNodeID)
{
    U8 ucRemoveNodeID;

    if (INVALID_2F == ucPreNodeID)
    {
        /* remove Head of High Priority Link */
        ucRemoveNodeID = g_HighPrioLink[ucSuperPu].ucHeadID;
        g_HighPrioLink[ucSuperPu].ucHeadID = g_ReadBufReqEntry[ucSuperPu][ucRemoveNodeID].ucNextBufReqID;

        if (ucRemoveNodeID == g_HighPrioLink[ucSuperPu].ucTailID)
        {
            /* only one BufReq in High Priority Link, then the Link is empty */
            g_HighPrioLink[ucSuperPu].ucTailID = INVALID_2F;           
        }
    }
    else
    {
        ucRemoveNodeID = g_ReadBufReqEntry[ucSuperPu][ucPreNodeID].ucNextBufReqID;
        g_ReadBufReqEntry[ucSuperPu][ucPreNodeID].ucNextBufReqID = g_ReadBufReqEntry[ucSuperPu][ucRemoveNodeID].ucNextBufReqID;

        if (ucRemoveNodeID == g_HighPrioLink[ucSuperPu].ucTailID)
        {
            /* remove Tail of High Priority Link (not Head) */
            g_HighPrioLink[ucSuperPu].ucTailID = ucPreNodeID;
        }
    }

    g_ReadBufReqEntry[ucSuperPu][ucRemoveNodeID].ucNextBufReqID = INVALID_2F;

    return;
}

BUF_REQ_WRITE* L1_WriteBufReqPop(U8 ucSuperPu)
{
    // pop a buffer request from read fifo
    BUF_REQ_WRITE* pBufReq;

#if 0 //def SIM
    if(TRUE != L1_HighPrioFifoEmpty(ucSuperPu))
    {
        DBG_Printf("L1_WriteBufReqPop have read req PU 0x%x\n", ucSuperPu);
        DBG_Getch();
    }
#endif

    if(TRUE == L1_LowPrioFifoEmpty(ucSuperPu))
    {
        DBG_Printf("L1_WriteBufReqPop Empty PU 0x%x\n", ucSuperPu);
        DBG_Getch();
    }

    // the low priority fifo is not empty, pop a request from it
    pBufReq = &gpLowPrioEntry[ucSuperPu]->tBufReq[LowPrioFifoHead[ucSuperPu]];

#ifdef L1_DEBUG_TRACE
#ifndef SIM
    L1_DbgPrintWriteBufReq(pBufReq);
#else
    FIRMWARE_LogInfo("MCU %d L1_WriteBufReqPop PU 0x%x Tag 0x%x Buf 0x%x LOff 0x%x LCnt 0x%x LPN 0x%x 0x%x\n", HAL_GetMcuId(),
        ucSuperPu, pBufReq->Tag, pBufReq->PhyBufferID, pBufReq->LPNOffset, pBufReq->LPNCount, pBufReq->LPN[0], pBufReq->LPN[1]);
#endif
#endif

    return pBufReq;
}

/*----------------------------------------------------------------------------
Name: L1_RecycleReadBufReq
Description:
Insert a Read BufReq node to the Tail of Free Read BufReq Link
Input Param:
U8 ucSuperPu
U8 ucReadBufReqID
Output Param:
none
Return Value:
void
Usage:
After L2 has processed a Read BufReq
History:
------------------------------------------------------------------------------*/
void L1_RecycleReadBufReq(U8 ucSuperPu, U8 ucReadBufReqID)
{
    U8 ucPreTailID = g_FreeReadBufReqLink[ucSuperPu].ucTailID;

    if (INVALID_2F == ucPreTailID)
    {
        /* Free Link is empty, need update Head */
#ifdef SIM
        if (INVALID_2F != g_FreeReadBufReqLink[ucSuperPu].ucHeadID)
        {
            DBG_Printf("L1_RecycleReadBufReq(%d, %d) error 1!\n", ucSuperPu, ucReadBufReqID);
            DBG_Getch();
        }
#endif
        g_FreeReadBufReqLink[ucSuperPu].ucHeadID = ucReadBufReqID;

        COM_BitMaskClear(&g_ulReadBufReqFullPuBitMap, ucSuperPu);
    }
    else
    {
#ifdef SIM
        if (INVALID_2F == g_FreeReadBufReqLink[ucSuperPu].ucHeadID)
        {
            DBG_Printf("L1_RecycleReadBufReq(%d, %d) error 2!\n", ucSuperPu, ucReadBufReqID);
            DBG_Getch();
        }
#endif
        g_ReadBufReqEntry[ucSuperPu][ucPreTailID].ucNextBufReqID = ucReadBufReqID;
    }

    /* update Tail of Free Link */
    g_FreeReadBufReqLink[ucSuperPu].ucTailID = ucReadBufReqID;
    g_ReadBufReqEntry[ucSuperPu][ucReadBufReqID].ucNextBufReqID = INVALID_2F;

    if (TRUE == L1_HighPrioLinkEmpty(ucSuperPu))
    {
        COM_BitMaskClear(&g_ulReadBufReqPuBitMap, ucSuperPu);
    }

    return;
}

void L1_WriteBufReqMoveHeadPtr(U8 ucSuperPu)
{

#ifdef SIM
    if (0 == g_aucWriteBufReqCount[ucSuperPu])
    {
        DBG_Printf("L1_WriteBufReqMoveHeadPtr PU 0x%x ERROR!\n", ucSuperPu);
        DBG_Getch();
    }
#endif
  
    // update the head of low priority fifo
    LowPrioFifoHead[ucSuperPu] = (LowPrioFifoHead[ucSuperPu] + 1) % PRIO_FIFO_DEPTH;

    // update the buffer request count and the buffer request bitmap
    g_aucWriteBufReqCount[ucSuperPu]--;
    if(0 == g_aucWriteBufReqCount[ucSuperPu])
    {
        COM_BitMaskClear(&g_ulWriteBufReqPuBitMap, ucSuperPu);
    }

    return;
}

/****************************************************************************
Function  : L1_HostReadBuildBufReq
Input     : SUBCMD *pSubCmd
Output    :BUF_REQ *pBufReq
Return    : 
Purpose   : set buffer REQ
Reference :
Build buffer request for host read
****************************************************************************/
void L1_HostReadBuildBufReq(SUBCMD *pSubCmd)
{
    U8 i;
    U32 ulStartLPN, LPNOffset;
    U8 ucSuperPu;
    U8 ucBufReqID;
    BUF_REQ_READ* pBufReq;

    ucSuperPu = pSubCmd->SubCmdAddInfo.ucPuNum;
    ucBufReqID = L1_AllocateReadBufReq(ucSuperPu);
#ifdef SIM
    if (INVALID_2F == ucBufReqID)
    {
        DBG_Printf("L1_HostReadBuildBufReq allocate Read BufReq ERROR!\n");
        DBG_Getch();
    }
#endif

    pBufReq = L1_GetReadBufReq(ucSuperPu, ucBufReqID);
    pBufReq->bPMTLookuped = FALSE;

#ifdef HOST_SATA
    /* Patch SDC: Need to set FstDataRdy after DSG is valid */
    if ((FALSE == g_Set1stDataReady) &&
        ((SATA_DSG_VALID_BIT & rSataDsgReportRead1) ||
        (FALSE == pSubCmd->pSCMD->tMA.ucFirst) ||
        (TRUE == pSubCmd->pSCMD->tMA.ucLast)))
    {
        pBufReq->bFirstRCMDEn = TRUE;
        g_Set1stDataReady = TRUE;
    }
#endif

    pBufReq->usPhyBufferID = pSubCmd->SubCmdPhyBufferID;

    pBufReq->ucLPNOffset = pSubCmd->SubCmdAddInfo.ucBufLPNOffsetOUT;
    pBufReq->ucLPNCount  = pSubCmd->SubCmdAddInfo.ucBufLPNCountOUT;
    pBufReq->ucReqOffset = pSubCmd->SubCmdAddInfo.ucSubCmdOffsetOUT;
    pBufReq->ucReqLength = pSubCmd->pSCMD->tMA.ucSecLen;
    pBufReq->bSeq = pSubCmd->pSCMD->tMA.ucIsSeq;

    ulStartLPN = LPN_FROM_LCTINDEX(pSubCmd->SubCmdAddInfo.ulSubCmdLCT) + pBufReq->ucLPNOffset;

    for (i = 0; i < pBufReq->ucLPNCount; i++)
    {
        LPNOffset = pBufReq->ucLPNOffset + i;
        pBufReq->aLPN[LPNOffset] = ulStartLPN + i;

    #ifndef L2_FAKE
        if (1 == pBufReq->ucLPNCount)
        {
            if (TRUE == L2_IsBootupOK())
            {
                L2_LookupPMT(&pBufReq->aFlashAddr[LPNOffset], ulStartLPN + i, FALSE);
                pBufReq->bPMTLookuped = TRUE;
            }
            else if (g_RebuildPMTDone && FALSE == L2_IsAdjustDirtyCntDone())
            {
                if (FALSE == L2_RebuildLookupRPMTBuffer(&pBufReq->aFlashAddr[LPNOffset], ulStartLPN + i))
                {
                    L2_LookupPMT(&pBufReq->aFlashAddr[LPNOffset], ulStartLPN + i, FALSE);
                }
                pBufReq->bPMTLookuped = TRUE;
            }
        }
    #endif
    }

#ifndef HOST_READ_FROM_DRAM
    L1_FillHostAddrInfo(pSubCmd, pBufReq);
#else
    pBufReq->bReadPreFetch = TRUE;

    for (i = 0; i < pBufReq->LPNCount; i++)
    {
        L1_AddPrefetchCacheStatusToLink(pSubCmd->SubCmdPhyBufferID);
    }
#endif

    /* send BugReq to L2 */
    L1_InsertBufReqToHighPrioLinkTail(ucSuperPu, ucBufReqID);
    
    return;
}

void L1_ForL2SetReLookupPMTFlag(U8 ucSuperPu)
{
    U8 ucCheckID;

    ucCheckID = g_HighPrioLink[ucSuperPu].ucHeadID;
    while (INVALID_2F != ucCheckID)
    {
        g_ReadBufReqEntry[ucSuperPu][ucCheckID].bNeedReLookupPMT = TRUE;
        ucCheckID = g_ReadBufReqEntry[ucSuperPu][ucCheckID].ucNextBufReqID;
    }

    return;
}

/********************** FILE END ***************/

