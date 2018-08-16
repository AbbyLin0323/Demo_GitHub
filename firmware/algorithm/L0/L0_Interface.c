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
Filename    :L0_Interface.c
Version     :Ver 1.0
Author      :
Date        :
Description : A subsystem command is the interface between L0 and L1. The routines for maintaining
                and operating the subsystem command queue on L0 are defined here.
Others      :
Modify      :
****************************************************************************/

#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_MultiCore.h"
#include "HAL_Xtensa.h"
#include "HAL_Interrupt.h"
#include "L0_Interface.h"
#include "L0_Config.h"

/* External variables: */

/* The number of subsystem implemented in current system configuration. */
extern U32 g_ulSubsysNum, g_ulSubsysNumBits;

/* The key data structure for L0/L1 interface: subsystem command queue. */
/* The queue is implemented with a ring FIFO located in L0/L1 shared memory space. */
/* L0 pushes SCMD nodes into the queue on its tail, */
/* while L1 pops SCMD nodes from the queue on its head. */
/* When L1 completes or rejects a SCMD, it would update the status area inside the SCMD node. */
/* L0 checks the return status every time it recycles a SCMD node. */
GLOBAL PSCQ g_apSCmdQueue[SUBSYSTEM_NUM_MAX];

/* The data structure for reversed-direction message exchanging interface: subsystem message queue. */
/* Either subsystem would push messages into the queue to post it to MCU0. */
/* L0 would check it and pop the messages from the queue to acquire messages from corresponding subsystem. */
GLOBAL PSMQ g_apSMsgQueue[SUBSYSTEM_NUM_MAX];

/* The message box is introduced for urgent system event notification from L0 to subsystems. */
/* It would be filled and referred to with the use of inter-MCU interrupts. */
GLOBAL PSMSG g_apNtfnMsgBox[SUBSYSTEM_NUM_MAX];

/* The global sequence number of notification messages. */
/* It increases whenever a notification message is posted. */
GLOBAL U32 g_ulNtfnMsgSeq;

//Multi Core Shared Data between MCU0 and MCU1 & MCU0 and MCU2
GLOBAL PMCSD g_apMcShareData[SUBSYSTEM_NUM_MAX];

/* External routines declaration */
U32 L0_ProcessSCMDCompletion(U32 ulSubSysId, PSCMD pSCNode);

/****************************************************************************
Name        :L0_InitSCQ
Input       : ulSubSysId - Subsystem index
Output      : None
Author      :
Date        :
Description : Initialization routine for SCMD queue data structure.
              It resets all ring pointers to zero for a SCMD queue belonging
              to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
void L0_InitSCQ(U32 ulSubSysId)
{
    //U32 ulNodeIdx;

    L0SCQ_Tail(ulSubSysId) = 0;
    L0SCQ_L0Head(ulSubSysId) = 0;
    L0SCQ_L1Head(ulSubSysId) = 0;

    COM_MemZero((U32 *)(g_apSCmdQueue[ulSubSysId]->aSubSysCmdArray),
        (sizeof(SCMD) * SCQSIZE_PER_SUBSYSTEM / sizeof(U32)));

    /* Since SCMD status SSTS_NOT_ALLOCATED is the number 0, we do not need to complicitly assign the
        status to each SCMD node. */
#if 0
    for (ulNodeIdx = 0; ulNodeIdx < SCQSIZE_PER_SUBSYSTEM; ulNodeIdx++ )
    {
        L0SCQ_Node(ulSubSysId, ulNodeIdx).ucSCmdStatus = (U8)SSTS_NOT_ALLOCATED;
    }
#endif

    return;
}

/****************************************************************************
Name        :L0_IsSCQFull
Input       : ulSubSysId - Subsystem index
Output      : The full status of SCMD queue belonging to the specified subsystem.
                TRUE - queue is full; FALSE - queue is not full.
Author      :
Date        :
Description : The interface routine for checking whether the specified SCMD queue is full so that L0 cannot
                submit any new SCMD to it.
Others      :
Modify      :
****************************************************************************/
U32 L0_IsSCQFull(U32 ulSubSysId)
{
    U32 ulNextTail, ulResult;

    /* One bubble is reserved for distinguishing queue full status between queue empty status. */
    ulNextTail = L0SCQ_Tail(ulSubSysId) + 1;

    if (SCQSIZE_PER_SUBSYSTEM == ulNextTail)
    {
        ulNextTail = 0;
    }

    if (ulNextTail == L0SCQ_L0Head(ulSubSysId))
    {
        ulResult = TRUE;
    }
    else
    {
        ulResult = FALSE;
    }

    return ulResult;
}

/****************************************************************************
Name        :L0_IsSCQEmpty
Input       : ulSubSysId - Subsystem index
Output      : The empty status of SCMD queue belonging to the specified subsystem.
                TRUE - queue is empty; FALSE - queue is not empty.
Author      :
Date        :
Description : The interface routine for checking whether the specified SCMD queue is empty so that L0 can
                know that all SCMDs for the specified subsystem had been completed.
Others      :
Modify      :
****************************************************************************/
U32 L0_IsSCQEmpty(U32 ulSubSysId)
{
    U32 ulResult;

    if (L0SCQ_Tail(ulSubSysId) == L0SCQ_L0Head(ulSubSysId))
    {
        ulResult = TRUE;
    }
    else
    {
        ulResult = FALSE;
    }

    return ulResult;
}

/****************************************************************************
Name        :L0_CheckSCQAllEmpty
Input       : None
Output      : The empty status of all SCMD queues.
                TRUE - All queues are empty; FALSE - Any queue is not empty.
Author      :
Date        :
Description : The interface routine for checking whether all SCMD queues are empty
                so that L0 can know that all pending SCMDs had been completed.
Others      :
Modify      :
****************************************************************************/
U32 L0_CheckSCQAllEmpty(void)
{
    U32 ulSubsysIdx;

    /* Because our program runs with single thread only, Scanning SCMD queue of
         each subsystem sequentially is safe. */
    for (ulSubsysIdx = 0; ulSubsysIdx < g_ulSubsysNum; ulSubsysIdx++)
    {
        if (FALSE == L0_IsSCQEmpty(ulSubsysIdx))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************
Name        :L0_GetNewSCmdNode
Input       : ulSubSysId - Subsystem index
Output      : The pointer to the SCMD node at specified subsystem queue tail.
Author      :
Date        :
Description : The interface routine allows L0 to retrieve the currently available SCMD node at
                the tail of one SCMD queue.
Others      :
Modify      :
****************************************************************************/
PSCMD L0_GetNewSCmdNode(U32 ulSubSysId)
{
    /* Warning: Always check whether the specified SCMD queue is full
        before invoking this routine to get a new SCMD node! */
    U32 ulCurrTail;
    PSCMD pNewSCmd;

    ulCurrTail = L0SCQ_Tail(ulSubSysId);
    pNewSCmd = &L0SCQ_Node(ulSubSysId, ulCurrTail);

    /* ASSERT: The status of current new SCMD node must be "available" */
    if ((U8)SSTS_NOT_ALLOCATED != pNewSCmd->ucSCmdStatus)
    {
        return NULL;
    }

    else
    {
        pNewSCmd->tMA.ucOption = 0;
        return pNewSCmd;
    }
}

/****************************************************************************
Name        :L0_PushSCmdNode
Input       : ulSubSysId - Subsystem index
Output      : None
Author      :
Date        :
Description : The interface routine allows L0 to submit the currently programmed SCMD node at
                queue tail to the queue of specified subsystem. It only moves queue tail pointer.
Others      :
Modify      :
****************************************************************************/
void L0_PushSCmdNode(U32 ulSubSysId)
{
    /* Warning: Always check whether the specified SCMD queue is full
        before invoking this routine to append a new SCMD node into queue! */
    U32 ulCurrTail;
    PSCMD pNewSCmd;

    ulCurrTail = L0SCQ_Tail(ulSubSysId);
    pNewSCmd = &L0SCQ_Node(ulSubSysId, ulCurrTail);

    /* Updates the state of new SCMD node. */
    pNewSCmd->ucSCmdStatus = (U8)SSTS_PENDING;

    /* Moves queue tail pointer. */
    if (SCQSIZE_PER_SUBSYSTEM == (++ulCurrTail))
    {
        ulCurrTail = 0;
    }

    L0SCQ_Tail(ulSubSysId) = ulCurrTail;

    return;
}

/****************************************************************************
Name        :L0_GetSCmdNodeToRecycle
Input       : ulSubSysId - Subsystem index
Output      : The pointer to the SCMD node at L0 head of the specified queue.
Author      :
Date        :
Description : The interface routine attempts to acquire an SCMD node from the nodes L1 already
                acquired for recycling. The pointer to the recycled node is returned if a node can be recycled
                was found.
Others      :
Modify      :
****************************************************************************/
PSCMD L0_GetSCmdNodeToRecycle(U32 ulSubSysId)
{
    U32 ulCurrL0Head, ulCurrL1Head;
    PSCMD pSCNodeToBeRecycled;

    ulCurrL0Head = L0SCQ_L0Head(ulSubSysId);
    ulCurrL1Head = L0SCQ_L1Head(ulSubSysId);

    if (ulCurrL0Head == ulCurrL1Head)
    {
        /* No subcommand node is waiting for recycling */
        pSCNodeToBeRecycled = NULL;
    }

    else
    {
        pSCNodeToBeRecycled = &L0SCQ_Node(ulSubSysId, ulCurrL0Head);

        if ((U8)SSTS_PENDING == pSCNodeToBeRecycled->ucSCmdStatus)
        {
            /* Current SCMD has not been completed by L1 */
            pSCNodeToBeRecycled = NULL;
        }

    }

    return pSCNodeToBeRecycled;
}

/****************************************************************************
Name        :L0_RecyleSCmd
Input       : None
Output      : The pointer to the SCMD node last recycled successfully
Author      :
Date        :
Description : The interface routine attempts to recycle an SCMD node from each subsystem queue in turn.
                The return status within each SCMD node waiting to be recycled is checked.
                Call-back routines can be invoked here for monitoring the return status of some specific
                types of subcommand.
Others      :
Modify      :
****************************************************************************/
PSCMD L0_RecyleSCmd(void)
{
    U32 ulSubSysId;
    PSCMD pSCNodeToBeRecycled;
    U32 ulNextL0Head;

    pSCNodeToBeRecycled = NULL;

    /* Always tries to recycle an SCMD node unless the queue of current subsystem is empty. */
    for (ulSubSysId = 0; ulSubSysId < g_ulSubsysNum; ulSubSysId++)
    {
        if (FALSE == L0_IsSCQEmpty(ulSubSysId))
        {
            /* Looking for an SCMD node completed by L1. */
            pSCNodeToBeRecycled = L0_GetSCmdNodeToRecycle(ulSubSysId);

            if (NULL != pSCNodeToBeRecycled)
            {
                /* Checking returning status for the SCMD currently being recycled. */
                (void)L0_ProcessSCMDCompletion(ulSubSysId, pSCNodeToBeRecycled);

                /* Updating the SCMD node status to "recycled". */
                pSCNodeToBeRecycled->ucSCmdStatus = (U8)SSTS_NOT_ALLOCATED;

                /* Moves the L0 head to recycle the SCMD node. */
                ulNextL0Head = L0SCQ_L0Head(ulSubSysId) + 1;

                if (SCQSIZE_PER_SUBSYSTEM == ulNextL0Head)
                {
                    ulNextL0Head = 0;
                }
                
                L0SCQ_L0Head(ulSubSysId) = ulNextL0Head;
            }
        }
    }

    return pSCNodeToBeRecycled;
}

/****************************************************************************
Name        :L0_WaitForAllSCmdCpl
Input       : ulSubSysId - Subsystem index
Output      : None
Author      :
Date        :
Description : The interface routine attempts to recycle all SCMD nodes from specified
                subsystem queue. The return status within each SCMD node waiting to be 
                recycled is checked. Call-back routines can be invoked here for monitoring
                the return status of some specific types of subcommand. This routine blocks
                the current executing thread until all SCMDs in specified SCMD queue are
                completed by lower level.
Others      :
Modify      :
****************************************************************************/
void L0_WaitForAllSCmdCpl(U32 ulSubSysId)
{
    PSCMD pSCNodeToBeRecycled;
    U32 ulNextL0Head;

    while (FALSE == L0_IsSCQEmpty(ulSubSysId))
    {
        /* Looking for an SCMD node completed by L1. */
        pSCNodeToBeRecycled = L0_GetSCmdNodeToRecycle(ulSubSysId);

        if (NULL != pSCNodeToBeRecycled)
        {
            /* Checking returning status for the SCMD currently being recycled. */
            (void)L0_ProcessSCMDCompletion(ulSubSysId, pSCNodeToBeRecycled);
        
            /* Updating the SCMD node status to "recycled". */
            pSCNodeToBeRecycled->ucSCmdStatus = (U8)SSTS_NOT_ALLOCATED;
        
            /* Moves the L0 head to recycle the SCMD node. */
            ulNextL0Head = L0SCQ_L0Head(ulSubSysId) + 1;
        
            if (SCQSIZE_PER_SUBSYSTEM == ulNextL0Head)
            {
                ulNextL0Head = 0;
            }
            
            L0SCQ_L0Head(ulSubSysId) = ulNextL0Head;
        }
    }

    return;
}

/****************************************************************************
Name        :L0_IssueAccessHostInfoSCmd
Input       : ulSubSysId - Subsystem index;
                ulHostInfoAddr - Specifies the buffer address for host information page;
                eAccType - Specifies saving or loading host information page.
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate a host information access SCMD
                and then dispatch it to specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueAccessHostInfoSCmd(U32 ulSubSysId, U32 ulHostInfoAddr, U32 ulAccType)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_ACCESS_HOSTINFO;

    pCurrSCmd->tSystemInfos.ulOpType   = ulAccType;
    pCurrSCmd->tSystemInfos.ulBuffAddr = ulHostInfoAddr;
    pCurrSCmd->tSystemInfos.ulByteLen  = sizeof(HOST_INFO_PAGE);

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueAccessDevParamSCmd
Input       : ulSubSysId - Subsystem index;
                ulDevParamAddr - Specifies the buffer address for device parameter page;
                eAccType - Specifies saving or loading device parameter page.
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate a device parameter access SCMD
                and then dispatch it to specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueAccessDevParamSCmd(U32 ulSubSysId, U32 ulDevParamAddr, U32 ulAccType)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_ACCESS_DEVPARAM;

    pCurrSCmd->tSystemInfos.ulOpType   = ulAccType;
    pCurrSCmd->tSystemInfos.ulBuffAddr = ulDevParamAddr;
    pCurrSCmd->tSystemInfos.ulByteLen  = sizeof(DEVICE_PARAM_PAGE);

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueLLFSCmd
Input       : ulSubSysId - Subsystem index;
                ulHostInfoAddr - Specifies the buffer address for host information page;
                ulResetDevParam - Specifies whether the subsystem is required to reset
                its device parameter page to the default value.
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate a low-level format SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueLLFSCmd(U32 ulSubSysId, U32 ulHostInfoAddr, U32 ulResetDevParam)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_LLF;
    pCurrSCmd->tSystemCtrl.ulHInfoBuffAddr = ulHostInfoAddr;
    pCurrSCmd->tSystemCtrl.ulHInfoByteLen  = sizeof(HOST_INFO_PAGE);
    pCurrSCmd->tSystemCtrl.ulParameter     = ulResetDevParam;

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

U32 L0_IssueSecurityEraseSCmd(U32 ulSubSysId, U32 ulHostInfoAddr, U32 ulResetDevParam)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_SANITIZE;
    pCurrSCmd->tSystemCtrl.ulHInfoBuffAddr = ulHostInfoAddr;
    pCurrSCmd->tSystemCtrl.ulHInfoByteLen  = sizeof(HOST_INFO_PAGE);
    pCurrSCmd->tSystemCtrl.ulParameter     = ulResetDevParam;

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueBootSCmd
Input       : ulSubSysId - Subsystem index
                  ulHostInfoAddr - The starting address of DRAM space which would hold the
                  host information page and L1 would copy host information page to.
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate a Boot Up SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueBootSCmd(U32 ulSubSysId, U32 ulHostInfoAddr)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_BOOTUP;
    pCurrSCmd->tSystemCtrl.ulParameter = COLD_START;
    pCurrSCmd->tSystemCtrl.ulHInfoBuffAddr = ulHostInfoAddr;
    pCurrSCmd->tSystemCtrl.ulHInfoByteLen  = sizeof(HOST_INFO_PAGE);

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssuePwrCylSCmd
Input       : ulSubSysId - Subsystem index
                  ulHostInfoAddr - The starting address of DRAM space which would hold the
                  host information page and L1 would copy host information page from.
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate a Power Cycle SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssuePwrCylSCmd(U32 ulHostInfoAddr)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(0))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(0);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_POWERCYCLING;
    pCurrSCmd->tSystemCtrl.ulHInfoBuffAddr = ulHostInfoAddr;
    pCurrSCmd->tSystemCtrl.ulHInfoByteLen  = sizeof(HOST_INFO_PAGE);

    L0_PushSCmdNode(0);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueIdleSCmd
Input       : ulSubSysId - Subsystem index;
                eIdlePrio - Specifies the priority (normal, critical, or error recovery) of current idle
                task request.
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate an Executing Idle Task SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueIdleSCmd(U32 ulSubSysId, U32 ulIdlePrio)
{
    PSCMD pCurrSCmd;

    /* An IDLE SCMD may only be issued while there is no more
        SCMD pending currently in the specified subsystem. */
    if (FALSE == L0_IsSCQEmpty(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_IDLETASK;
    pCurrSCmd->tIdle.ulPriority = ulIdlePrio;

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueSelfTestingSCmd
Input       : ulSubSysId - Subsystem index;
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate an Self-testing SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueSelfTestingSCmd(U32 ulSubSysId)
{
    PSCMD pCurrSCmd;

    /* An Self-testing SCMD may only be issued while there is no more
        SCMD pending currently in the specified subsystem. */
    if (FALSE == L0_IsSCQEmpty(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_SELFTESTING;

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueFlushSCmd
Input       : ulSubSysId - Subsystem index;
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate a Flush cache SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueFlushSCmd(U32 ulSubSysId)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_FLUSH;

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_IssueUnmapSCmd
Input       : ulSubSysId - Subsystem index;
                ulUnmapStart - Specifies the starting subsystem LBA of the area to be unmapped;
                ulRangeLen - Specifies the sector length of the area to be unmapped;
Output      : FAIL - SCMD has not been successfully issued to specified subsystem;
                SUCCESS - SCMD has been successfully issued to specified subsystem.
Author      :
Date        :
Description : This routine attempts to generate an Unmap SCMD and then
                dispatch it to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
U32 L0_IssueUnmapSCmd(U32 ulSubSysId, U32 ulUnmapStart, U32 ulRangeLen, U32* ulCurrTail)
{
    PSCMD pCurrSCmd;

    /* Checking whether there is any SCMD queue node available. */
    if (TRUE == L0_IsSCQFull(ulSubSysId))
    {
        return FAIL;
    }

    *ulCurrTail = L0SCQ_Tail(0);  
    pCurrSCmd = L0_GetNewSCmdNode(ulSubSysId);

    /* Programs parameters for the SCMD. */
    pCurrSCmd->ucSCmdType = (U8)SCMD_UNMAP;
    pCurrSCmd->tUnmap.ulSubSysLBA = ulUnmapStart;
    pCurrSCmd->tUnmap.ulSecLen    = ulRangeLen;

    L0_PushSCmdNode(ulSubSysId);

    return SUCCESS;
}

/****************************************************************************
Name        :L0_InitSMQ
Input       : ulSubSysId - Subsystem index
Output      : None
Author      :
Date        :
Description : Initialization routine for SMSG queue data structure.
              It resets ring pointers to zero for an SMSG queue belonging
              to the specified subsystem.
Others      :
Modify      :
****************************************************************************/
void L0_InitSMQ(U32 ulSubSysId)
{
    L0SMQ_Tail(ulSubSysId) = 0;
    L0SMQ_Head(ulSubSysId) = 0;

#if 0
    COM_MemZero((U32 *)(g_apSMsgQueue[ulSubSysId]->aSubSysMsgArray),
        (sizeof(SMSG) * SMQSIZE_PER_SUBSYSTEM / sizeof(U32)));
#endif

    return;
}

/****************************************************************************
Name        :L0_IsSMQEmpty
Input       : ulSubSysId - Subsystem index
Output      : The empty status of SMSG queue belonging to the specified subsystem.
                TRUE - queue is empty; FALSE - queue is not empty.
Author      :
Date        :
Description : The interface routine for checking whether the specified SMSG queue is empty
                so that L0 can know that there is no subsystem message outstanding.
Others      :
Modify      :
****************************************************************************/
U32 L0_IsSMQEmpty(U32 ulSubSysId)
{
    U32 ulResult;

    if (L0SMQ_Tail(ulSubSysId) == L0SMQ_Head(ulSubSysId))
    {
        ulResult = TRUE;
    }

    else
    {
        ulResult = FALSE;
    }

    return ulResult;
}

/****************************************************************************
Name        :L0_GetSMSGFromHead
Input       : ulSubSysId - Subsystem index
Output      : The pointer to the SMSG node at queue head of current subsystem.
                If the SMSG queue is empty, return a null pointer.
Author      :
Date        :
Description : The interface routine allows L0 to get the SMSG node at queue head from the 
                    queue of specified subsystem and acquire the subsystem message context.
Others      :
              This routine do not move head pointer of SMSG FIFO
Modify      :
****************************************************************************/
PSMSG L0_GetSMSGFromHead(U32 ulSubSysId)
{
    U32 ulCurrHead, ulCurrTail;
    PSMSG pNewSMsg;

    ulCurrHead = L0SMQ_Head(ulSubSysId);
    ulCurrTail = L0SMQ_Tail(ulSubSysId);

    if (ulCurrHead == ulCurrTail)
    {
        /* Checks whether the specified SMSG queue is empty first. */
        pNewSMsg = NULL;
    }

    else
    {
        /* Gets the head SMSG node. */
        pNewSMsg = &L0SMQ_Node(ulSubSysId, ulCurrHead);
    }

    return pNewSMsg;
}

/****************************************************************************
Name        :L0_PopSMSGNode
Input       : ulSubSysId - Subsystem index
Output      : 
Author      :
Date        :
Description : The interface routine move the head pointer of SMSG FIFO.
Others      :
              This routine must cooperate with L0_GetSMSGFromHead
Modify      :
****************************************************************************/
void L0_PopSMSGNode(U32 ulSubSysId)
{
    U32 ulCurrHead, ulCurrTail;

    ulCurrHead = L0SMQ_Head(ulSubSysId);
    ulCurrTail = L0SMQ_Tail(ulSubSysId);
    
    /* Checks whether the specified SMSG queue is empty first. */
    if (ulCurrHead == ulCurrTail)
    {
        DBG_Getch(); // !! something wrong: there was no SMSG node pushed before this routine
    }

    else
    {        
        /* Moves queue head pointer. */
        if (SMQSIZE_PER_SUBSYSTEM == (++ulCurrHead))
        {
            ulCurrHead = 0;
        }
        
        L0SMQ_Head(ulSubSysId) = ulCurrHead;
    }

    return;
}

/****************************************************************************
Name        :L0_PostNtfnMsg
Input       : ulTargetSubsysMap - subsystem bitmap;
                ulMsgCode - Message code to be posted to specified subsystem.
Output      : None
Author      :
Date        :
Description : This routine posts an urgent notification message to specified subsystem through
                    inter-MCU interrupt mechanism.
Others      :
Modify      :
****************************************************************************/
void L0_PostNtfnMsg(U32 ulTargetSubsysMap, U32 ulMsgCode)
{
    U32 ulSubsysId;

    for (ulSubsysId = 0; ulSubsysId < g_ulSubsysNum; ulSubsysId++)
    {
        if ( 0 != ((1 << ulSubsysId) & ulTargetSubsysMap) )
        {
            /* 1. Fills message content and global sequence number. */
            g_apNtfnMsgBox[ulSubsysId]->ulMsgCode = ulMsgCode;
            g_apNtfnMsgBox[ulSubsysId]->ulMsgSeqNum = g_ulNtfnMsgSeq;
            g_ulNtfnMsgSeq++;

            /* 2. Issues an inter-MCU interrupt to specified subsystem. */
            if (0 == ulSubsysId)
            {
                HAL_AssertIPI(MCU0_ID, MCU1_ID, TRUE);
            }

            else if (1 == ulSubsysId)
            {
                HAL_AssertIPI(MCU0_ID, MCU2_ID, TRUE);
            }

            else
            {
                DBG_Getch();
            }
        }
    }//end for...


    return;
}

void L0_ForceAllSubSysIdle(void)
{
    L0_WaitForAllSCmdCpl(0);
    L0_IssueIdleSCmd(0, IDLE_CRITICAL);
    L0_WaitForAllSCmdCpl(0);

    return;
}


