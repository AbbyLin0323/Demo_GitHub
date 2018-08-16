/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :L1_Interface.c
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    14:57:15
Description :
Others      :
Modify      :20120118     peterxiu     001 first create
****************************************************************************/

#include "BaseDef.h"
#include "HAL_Inc.h"
#include "L1_Inc.h"

BUF_REQ HighPrioEntry[PRIO_FIFO_DEPTH];
BUF_REQ LowPrioEntry[PRIO_FIFO_DEPTH];

/*
Prio fifo
Head                      Tail
|                            |
|                            |
*/
U8 HighPrioFifoHead;
U8 HighPrioFifoTail;

U8 LowPrioFifoHead;
U8 LowPrioFifoTail;

U8 g_ucCurrPoppedFifo;

/****************************************************************************
Function  : L1_BufReqFifoInit
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :

****************************************************************************/
void L1_BufReqFifoInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase)
{
    BUF_REQ* pBufReq;
    U8    i, j;

    for(i = 0; i < PRIO_FIFO_DEPTH; i++)
    {
      pBufReq = &HighPrioEntry[i];
      for(j = 0; j < sizeof(BUF_REQ)/sizeof(U32); j++)
      {
          *((U32*)pBufReq + j) = 0 ;
      }

      pBufReq = &LowPrioEntry[i];
      for(j = 0; j < sizeof(BUF_REQ)/sizeof(U32); j++)
      {
           *((U32*)pBufReq + j) = 0 ;
      }
    }

    HighPrioFifoHead = 0;
    HighPrioFifoTail = 0;
    
    LowPrioFifoHead = 0;
    LowPrioFifoTail = 0;

    g_ucCurrPoppedFifo = PRIO_FIFO_NONE;

    return;
}

/****************************************************************************
Function  : L1_HighPrioFifoFull
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_HighPrioFifoFull(void)
{
    U8 ucTail;

    /* check Tail +1 == Head */
    ucTail = HighPrioFifoTail + 1;

    if(ucTail >= PRIO_FIFO_DEPTH)
    {
        ucTail -= PRIO_FIFO_DEPTH;
    }

    if(ucTail == HighPrioFifoHead)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Function  : L1_LowPrioFifoFull
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_LowPrioFifoFull(void)
{
    U8 ucTail;

    /* check Tail +1 == Head */
    ucTail = LowPrioFifoTail + 1;

    if(ucTail >= PRIO_FIFO_DEPTH)
    {
        ucTail -= PRIO_FIFO_DEPTH;
    }

    if(ucTail == LowPrioFifoHead)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Function  : L1_BufReqFifoFull
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_BufReqFifoFull(void)
{
    if(L1_LowPrioFifoFull() == TRUE && L1_HighPrioFifoFull() == TRUE)
    {
        return TRUE;
    }   

    return FALSE;
}

/****************************************************************************
Function  : L1_HighPrioFifoEmpty
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_HighPrioFifoEmpty(void)
{
    if(HighPrioFifoHead == HighPrioFifoTail)
    {
        return TRUE;
    }   

    return FALSE;
}

/****************************************************************************
Function  : L1_LowPrioFifoEmpty
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_LowPrioFifoEmpty(void)
{
    if(LowPrioFifoHead == LowPrioFifoTail)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
Function  : L1InterfaceBUFREQFifoEmpty
Input     : 
Output    :
Return    : 
Purpose   : 
Reference :
****************************************************************************/
U32 L1_BufReqFifoEmpty(void)
{
    if(L1_LowPrioFifoEmpty() == TRUE && L1_HighPrioFifoEmpty() == TRUE)
    {
        return TRUE;
    }   

    return FALSE;
}

/****************************************************************************
Name        :L1_PrioFifoGetPendingCnt
Input        :
Output      :
Author      :BlakeZhang
Date        :2012.12.11
Description :get total pending request count of Prio Fifo.
Others      :
Modify      :
****************************************************************************/
U32 L1_PrioFifoGetPendingCnt(void)
{
    U32 ulPendingCnt;

    ulPendingCnt = 0;

    if (TRUE != L1_HighPrioFifoEmpty())
    {
        if (HighPrioFifoTail >= HighPrioFifoHead)
        {
            ulPendingCnt = HighPrioFifoTail - HighPrioFifoHead;
        }
        else
        {
            ulPendingCnt = PRIO_FIFO_DEPTH + HighPrioFifoTail - HighPrioFifoHead;
        }

        return ulPendingCnt;
    }

    if (TRUE != L1_LowPrioFifoEmpty())
    {
        if (LowPrioFifoTail >= LowPrioFifoHead)
        {
            ulPendingCnt = LowPrioFifoTail - LowPrioFifoHead;
        }
        else
        {
            ulPendingCnt = PRIO_FIFO_DEPTH + LowPrioFifoTail - LowPrioFifoHead;
        }
    }
    
    return ulPendingCnt;
}

/****************************************************************************
Function  : L1_HighPrioFifoGetBufReq
Input     : 
Output    :
Return    : 
Purpose   : 
Reference : Get a BufReq for HighPrioFifo Tail
****************************************************************************/
BUF_REQ* L1_HighPrioFifoGetBufReq(void)
{
      /* check HighPrio Fifo full */
    if (L1_HighPrioFifoFull() == TRUE)
    {
       DBG_Printf("L1_HighPrioFifoGetBufReq Error: L1_HighPrioFifoFull!!!\n");
       DBG_Getch();
    }

    return (&HighPrioEntry[HighPrioFifoTail]);
}

/****************************************************************************
Function  : L1_HighPrioMoveTailPtr
Input     : 
Output    :
Return    : 
Purpose   : 
Reference : move HighPrioFifo Tailptr
****************************************************************************/
void L1_HighPrioMoveTailPtr(void)
{
    U8 ucTail;
    
    ucTail = HighPrioFifoTail;

    ucTail++;
    if (PRIO_FIFO_DEPTH == ucTail)
    {
        ucTail = 0;
    }

    HighPrioFifoTail = ucTail;

    return;
}



/********************** FILE END ***************/

