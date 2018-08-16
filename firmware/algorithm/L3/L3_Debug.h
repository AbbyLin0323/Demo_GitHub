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
* File Name    : L3_Debug.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_DEBUG_H
#define _L3_DEBUG_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "L3_FCMDQ.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
//#define L3_DBG_PRINT_EN
#define L3_DBG_CHK_EN
//#define L3_DBG_ERR_INJ_EN

#ifdef LDPC_CENTER_RECORD
#define L3_DBG_ERR_INJ_EN
#endif

#ifdef BUFFMAP_DEBUG
#define L3_DBG_ERR_INJ_EN
#endif

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/
typedef enum _L3_DBG_FCMD_TYPE_
{
    L3_DBG_FCMD_REQ_SEND = 0,
    L3_DBG_FCMD_REQ_ALL,
    L3_DBG_FCMD_REQ_SPECIAL,
    L3_DBG_FCMD_REQ_HOST_RD,
    L3_DBG_FCMD_REQ_AUTO,
    L3_DBG_FCMD_REQ_MANUL,
    L3_DBG_FCMD_REQ_NULL,
    L3_DBG_FCMD_REQ_HOST_RD_DONE,
    L3_DBG_FCMD_REQ_AUTO_DONE,
    L3_DBG_FCMD_REQ_MANUL_DONE,
    L3_DBG_FCMD_REQ_NULL_DONE,
    L3_DBG_FCMD_REQ_INTR,
    L3_DBG_FCMD_REQ_INTR_DONE,
    L3_DBG_FCMD_REQ_NUM
}L3_DBG_FCMD_TYPE;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L3_DbgInit(void);
BOOL L3_DbgL2SendCntAdd(U8 ucTLun);
void L3_DbgFCmdCntAdd(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_DbgFCmdCntDec(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_DbgFCmdAutoChk(U8 ucPhyTLun);
BOOL L3_DbgFCmdManulChk(U8 ucPhyTLun, BOOL bIntrReq);
BOOL L3_DbgFCmdNullChk(U8 ucPhyTLun);
BOOL L3_DbgFCmdHostRdChk(U8 ucPhyTLun);
void L3_DbgFCmdPrint(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry, U8 *pStr);
void L3_DbgSetNFCQErrInj(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

#endif
/*====================End of this head file===================================*/

