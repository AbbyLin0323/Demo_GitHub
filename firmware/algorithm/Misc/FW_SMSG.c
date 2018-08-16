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
Filename    :FW_SMSG.c
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "FW_Event.h"
#include "FW_SMSG.h"

LOCAL MCU12_VAR_ATTR PSMQ l_pFWSMsgQueue;
LOCAL MCU12_VAR_ATTR U32  l_ulMsgSeqNum;
LOCAL MCU12_VAR_ATTR PSMSG l_pFWNtfnMsg;
LOCAL MCU12_VAR_ATTR U32  l_ulNtfnMsgSeqNum;

GLOBAL MCU12_VAR_ATTR volatile PMCSD g_apSubMcShareData;

/*==============================================================================
Func Name  : FW_InitSMSG
Input      : None 
Output     : None
Return Val : 
Discription: init the retry fail message queue.
Usage      : get a message entry and push it, report the retry fail to L0.
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT FW_InitSMSG(void)
{    
    if (MCU1_ID == HAL_GetMcuId())
    {
        l_pFWSMsgQueue = (PSMQ)(DSRAM1_MCU01_SHARE_BASE + sizeof(SCQ));
        l_pFWNtfnMsg = (PSMSG)(DSRAM1_MCU01_SHARE_BASE + sizeof(SCQ) + sizeof(SMQ));
    }

    else
    {
        l_pFWSMsgQueue = (PSMQ)(DSRAM1_MCU01_SHARE_BASE + sizeof(SCQ));
        l_pFWNtfnMsg = (PSMSG)(DSRAM1_MCU01_SHARE_BASE + sizeof(SCQ) + sizeof(SMQ));    
    }

    l_ulMsgSeqNum = 0;
    l_ulNtfnMsgSeqNum = 0;

    return;
}


GLOBAL void MCU12_DRAM_TEXT FW_InitMSD(void)
{
    if (MCU1_ID == HAL_GetMcuId())
    {
        g_apSubMcShareData = (PMCSD)(DSRAM1_MCU01_SHARE_BASE + sizeof(SCQ) + sizeof(SMQ) + sizeof(SMSG));
    }
    else
    {
        g_apSubMcShareData = (PMCSD)(DSRAM1_MCU01_SHARE_BASE + sizeof(SCQ) + sizeof(SMQ) + sizeof(SMSG));
    }

    return;
}


/*==============================================================================
Func Name  : FW_IsSMQEmpty
Input      : None 
Output     : None
Return Val : BOOL
Discription: check the message queue empty.
Usage      : 
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
GLOBAL BOOL MCU12_DRAM_TEXT FW_IsSMQEmpty(void)
{
    return l_pFWSMsgQueue->ulHead == l_pFWSMsgQueue->ulTail;
}

/*==============================================================================
Func Name  : FW_IsSMQFull
Input      : None 
Output     : None
Return Val : BOOL
Discription: check the message queue full.
Usage      : ignore the current message when the queue is full.
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
LOCAL BOOL MCU12_DRAM_TEXT FW_IsSMQFull(void)
{
    return ((l_pFWSMsgQueue->ulTail+1)%SMQSIZE_PER_SUBSYSTEM == l_pFWSMsgQueue->ulHead);
}

/*==============================================================================
Func Name  : FW_GetNewSMsg
Input      : None 
Output     : None
Return Val : PSMSG
Discription: get a new Message Entry from the tail of the queue. 
Usage      : allocate a new message entry for construction.
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
LOCAL PSMSG MCU12_DRAM_TEXT FW_GetNewSMsg(void)
{
    return &l_pFWSMsgQueue->aSubSysMsgArray[l_pFWSMsgQueue->ulTail];
}

/*==============================================================================
Func Name  : FW_PushSMsg
Input      : None
Output     : None
Return Val : 
Discription: push a retry fail message to the message queue. 
Usage      : 
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
LOCAL void MCU12_DRAM_TEXT FW_PushSMsg(void)
{
    l_pFWSMsgQueue->ulTail = (l_pFWSMsgQueue->ulTail+1)%SMQSIZE_PER_SUBSYSTEM;
}

/*==============================================================================
Func Name  : FW_ReportSMSG
Input      : 
Output     : None
Return Val : 
Discription: push a retry fail message entry to the message queue. 
Usage      : report the retry fail to L0.
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT FW_ReportSMSG(PSMSG pSMSG)
{
    PSMSG ptSMSG;

    if (TRUE != FW_IsSMQFull())
    {    
        ptSMSG = FW_GetNewSMsg();

        ptSMSG->ulMsgSeqNum   = l_ulMsgSeqNum++;
        ptSMSG->ulMsgCode     = pSMSG->ulMsgCode;
        ptSMSG->ulMsgParam[0] = pSMSG->ulMsgParam[0];
        ptSMSG->ulMsgParam[1] = pSMSG->ulMsgParam[1];
        
        FW_PushSMsg();
    }

    return;
}

/*==============================================================================
Func Name  : FW_ChkNtfnMsg
Input      : None 
Output     : None
Return Val : None
Discription: Processes the notification message from L0 in the ISR of inter-MCU interrupt. 
Usage      : allocate a new message entry for construction.
History    : 
    1. 2014.11.11 JasonGuo create function
==============================================================================*/
GLOBAL void MCU12_DRAM_TEXT FW_ChkNtfnMsg(void)
{
    //U32 ulGlobalSeqNum;
    U32 ulMCUId = HAL_GetMcuId();

    /* Yao Chen - 12/16/2015: 
        Now checking sequence number of notification message is not necessary.
        It may cause mistaken error report on single-subsystem systems.
        We can manually compare the local sequence number and message sequence
        number by reading them out through JTAG if we suspect a notification message
        may be lost. */
#if 0
    if (MCU1_ID == ulMCUId)
    {
        ulGlobalSeqNum = (l_ulNtfnMsgSeqNum << 1);
    }
    else
    {
        ulGlobalSeqNum = (l_ulNtfnMsgSeqNum << 1) + 1;
    }

    if (ulGlobalSeqNum != l_pFWNtfnMsg->ulMsgSeqNum)
    {
        /* Mismatched sequence number between L0/L1 may signal an inter-MCU interrupt loss. */
        DBG_Printf("Notification message sequence mismatch!\n");
        DBG_Getch();
    }
#endif

    switch (l_pFWNtfnMsg->ulMsgCode)
    {
        case SMSG_FORCE_CLEAR_SCQ:
            /* We only used the notification message to pass force clear SCQ request. */
            CommSetEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_ERR);
            break;

        case SMSG_WAKEUP_SUBSYS:
            /* This notification message only wakes a subsystem MCU up from WAITI state. */
            break;

        case SMSG_DBG_GETCH_CLEAR_SCQ:
            /* Notify MCU12 to clear SCQ and enter debug mode after debug getch */
            CommSetEvent(COMM_EVENT_OWNER_L1, COMM_EVENT_OFFSET_DBG);
            break;

        default:
            break;
    }

    /* Increasing local message sequence number after receiving one message. */
    l_ulNtfnMsgSeqNum++;

    return;
}

