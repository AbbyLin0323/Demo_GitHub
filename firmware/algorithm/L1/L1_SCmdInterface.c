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
Filename    :L1_SCmdInterface.c
Version     :Ver 1.0
Author      :
Date        :
Description : A subsystem command is the interface between L0 and L1. The routines for maintaining
                and operating the subsystem command queue on L1 are defined here.
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "L1_SCmdInterface.h"

GLOBAL  PSCQ g_pL1SCmdQueue;
GLOBAL  PSCMD g_pCurSCmd;

/****************************************************************************
Name        :L1_InitSCQ
Input       : None
Output      : None
Author      :
Date        :
Description : Initialization routine for SCMD queue on each subsystem.
                It calculates the correct base address for the global queue pointer.
Others      :
Modify      :
****************************************************************************/
void MCU1_DRAM_TEXT L1_InitSCQ(void)
{
    if (MCU1_ID == HAL_GetMcuId())
    {
        g_pL1SCmdQueue = ((PSCQ)DSRAM1_MCU01_SHARE_BASE);
    }

    else
    {
        DBG_Printf("MCUID Error\n");
        DBG_Getch();
        //g_pL1SCmdQueue = ((PSCQ)DSRAM1_MCU02_SHARE_BASE);
    }

    g_pCurSCmd = NULL;
    return;
}

/****************************************************************************
Name        :L1_IsSCQEmpty
Input       : None
Output      : The empty status of SCMD queue for current subsystem.
                TRUE - queue is empty; FALSE - queue is not empty.
Author      :
Date        :
Description : The interface routine for checking whether the SCMD queue of current subsystem is empty
                so that L1 can decide whether it can pop a SCMD node.
Others      :
Modify      :
****************************************************************************/
U32 L1_IsSCQEmpty(void)
{
    U32 ulResult;

    // head == tail means empty, while tail+1 == head means
    // full
    if (L1_SCQ_Tail == L1_SCQ_Head)
    {
        ulResult = TRUE;
    }

    else
    {
        ulResult = FALSE;
    }

    return ulResult;
}

U32 L1_SCQGetCount(void)
{
    U32 ulResult;

    if (L1_SCQ_Tail >= L1_SCQ_Head)
    {
        ulResult = L1_SCQ_Tail - L1_SCQ_Head;
    }

    else
    {
        ulResult = SCQSIZE_PER_SUBSYSTEM + L1_SCQ_Tail - L1_SCQ_Head;
    }

    return ulResult;
}

/****************************************************************************
Name        :L1_GetSCmdFromHead
Input       : None
Output      : The pointer to the SCMD node at queue head of current subsystem.
                If the status of current SCMD node is not "pending", return a null pointer.
Author      :
Date        :
Description : The interface routine allows L1 to retrieve the pending SCMD node at the head of
                the SCMD queue of current subsystem.
Others      :
Modify      :
****************************************************************************/
PSCMD L1_GetSCmdFromHead(void)
{
    /* Warning: Always check whether the current SCMD queue is empty
        before invoking this routine to get a ready SCMD! */
    PSCMD pCurrSCmd;

    pCurrSCmd = &L1_SCQ_Node(L1_SCQ_Head);

    /* ASSERT: The status of current ready SCMD must be "pending" */
    if ((U8)SSTS_PENDING != pCurrSCmd->ucSCmdStatus)
    {
        return NULL;
    }

    else
    {
        return pCurrSCmd;
    }
}

U32 L1_GetCurrentScqHeadPointer(void)
{
    return L1_SCQ_Head;
}

U32 L1_GetCurrentScqTailPointer(void)
{
    return L1_SCQ_Tail;
}

PSCMD L1_TryFetchScmd(U32* pucPointer)
{
    // Sean Gao 20150730
    
    PSCMD pCurrSCmd;

    pCurrSCmd = &L1_SCQ_Node(*pucPointer);

    /* ASSERT: The status of current ready SCMD must be "pending" */
    if ((U8)SSTS_PENDING != pCurrSCmd->ucSCmdStatus)
    {
        return NULL;
    }
    else
    {
        if (SCQSIZE_PER_SUBSYSTEM == (++(*pucPointer)))
        {
            (*pucPointer) = 0;
        }

        return pCurrSCmd;
    }
   
    
}
/****************************************************************************
Name        :L1_PopSCmdNode
Input       : None
Output      : None
Author      :
Date        :
Description : The interface routine allows L1 to remove the pending SCMD node at queue head of
                specified subsystem. It only moves queue head pointer. The return status would be
                updated to the queue node at head before it was deleted.
Others      :
Modify      :
****************************************************************************/
void L1_PopSCmdNode(U32 ulRetStatus)
{
    /* Warning: Always check whether the current SCMD queue is empty
        before invoking this routine to delete a processed SCMD node from queue! */
    U32 ulCurrHead;
    PSCMD pCurrSCmd;

    ulCurrHead = L1_SCQ_Head;
    pCurrSCmd = &L1_SCQ_Node(ulCurrHead);

    pCurrSCmd->ucSCmdStatus = (U8)ulRetStatus;

    if (SCQSIZE_PER_SUBSYSTEM == (++ulCurrHead))
    {
        ulCurrHead = 0;
    }

    L1_SCQ_Head = ulCurrHead;

    return;
}

/****************************************************************************
Name        :L1_GetSCmd
Input       : None
Output      : The pointer to the SCMD node at queue head of current subsystem.
                If the queue is empty or status of current SCMD node is not "pending", return a null pointer.
Author      :
Date        :
Description : The interface routine is encapsulated by L1, which allows L1 to retrieve a pending SCMD
                node at the head of the SCMD queue of current subsystem.
Others      :
Modify      :
****************************************************************************/
PSCMD L1_GetSCmd(void)
{
    PSCMD pSCmd;

    pSCmd = NULL;

    if (FALSE == L1_IsSCQEmpty())
    {
        pSCmd = L1_GetSCmdFromHead();
    }

    return pSCmd;
}

/****************************************************************************
Name        :L1_SCmdFinish
Input       : None
Output      : None
Author      :
Date        :
Description : The interface routine completes the current pending SCMD on queue head with a SUCCESS
                  return status.
Others      :
Modify      :
****************************************************************************/
void L1_SCmdFinish(void)
{
    L1_PopSCmdNode((U32)SSTS_SUCCESS);
    g_pCurSCmd = NULL;

    return;
}

/****************************************************************************
Name        :L1_SCmdFail
Input       : None
Output      : None
Author      :
Date        :
Description : The interface routine completes the current pending SCMD on queue head with a FAILURE
                  return status.
Others      :
Modify      :
****************************************************************************/
void L1_SCmdFail(void)
{
    L1_PopSCmdNode((U32)SSTS_FAILED);
    g_pCurSCmd = NULL;

    return;
}

/****************************************************************************
Name        :L1_SCmdAbort
Input       : None
Output      : None
Author      :
Date        :
Description : The interface routine completes the current pending SCMD on queue head with a ABORT
                  return status.
Others      :
Modify      :
****************************************************************************/
void L1_SCmdAbort(void)
{
    L1_PopSCmdNode((U32)SSTS_ABORTED);
    g_pCurSCmd = NULL;

    return;
}

