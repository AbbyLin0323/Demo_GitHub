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
* File Name    : L3_DataRecover.h
* Discription  : 
* CreateAuthor : JasonGuo
* CreateDate   : 2016.6.22
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L3_DATARECOVER_H
#define _L3_DATARECOVER_H

#include "BaseDef.h"
#include "L3_FCMDQ.h"

typedef enum _DATA_RECOVERY_STAGE_
{
    DATA_RECO_INIT = 0x0,
    DATA_RECO_READ,
    DATA_RECO_READ_CHK,
    DATA_RECO_TRIG,
    DATA_RECO_TRIG_CHK,
    DATA_RECO_SUCCESS,
    DATA_RECO_FAIL
}DATA_RECOVERY_STAGE;

void L3_XorInit(void);
void L3_XorMapInit(void);
BOOL L3_IsNeedDoXor(const FCMD_REQ_ENTRY *pReqEntry);
BOOL L3_IsDataDisperse(const FCMD_REQ_ENTRY *ptReqEntry);
BOOL L3_IsTlcXor6DsgIssue(const FCMD_REQ_ENTRY *ptReqEntry);
BOOL L3_AllocXore(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
BOOL L3_XoreRelease(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);
void L3_IFSetNFCQXor(NFCQ_ENTRY *ptNfcqEntry, FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

void L3_DataRecovery(FCMD_INTR_CTRL_ENTRY *ptCtrlEntry);

#endif
/*====================End of this head file===================================*/

