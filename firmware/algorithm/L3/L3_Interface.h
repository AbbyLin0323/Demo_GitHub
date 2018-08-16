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
* File Name    : L3_Interface.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_INTERFACE_H
#define _L3_INTERFACE_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L3_FCMDQ.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L3_IFNFCIRSInit(void);

void L3_IFPopFCmdQ(U8 ucTLun);
void L3_IFRecycleFCmdQ(U8 ucTLun);

BOOL L3_IFSendFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_IFSendNormalFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

BOOL L3_IFIsAllFCmdIntrQEmpty(void);
BOOL L3_IFAllocDSG(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_IFIsRecycled(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_IFIsSpecialFCmd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_IFIsFCmdFinished(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_IFUpdtReqStatus(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

void L3_FCMDQMarkPrePageEn(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

BOOL L3_IFIsManulRptReq(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_IFIsAutoRptReq(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_IFIsForceRecycle(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

void L3_IFSetFlashMonitor(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_IFClearFlashMonitor(U8 ucTLun);

// The abstract interfaces for dif-host type.
BOOL L3_AllocResource(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_SetNfcqCustom(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_RstDatTxCtrler(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_HostReadEmpty(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_HostReadRecover(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, BOOL bRcvFail);
void L3_SGEReset(void);
#endif
/*====================End of this head file===================================*/

