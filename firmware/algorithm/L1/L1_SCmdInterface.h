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
Filename     :  L1_SCmdInterface.h
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 
Define registers and data structure for AHCI specification (v1.3).
SCMD data structure and operation routines declaration are included as well.

Modification History:
2014/06/12   Blake Zhang 001 created
****************************************************************************/
#ifndef _L1_SCMDINTERFACE_H
#define _L1_SCMDINTERFACE_H

#include "L0_Interface.h"

/* Mnemonics for queue members */
#define L1_SCQ_Tail (g_pL1SCmdQueue->ulTail)
#define L1_SCQ_Head (g_pL1SCmdQueue->ulL1Head)
#define L1_SCQ_Node(_x_) (g_pL1SCmdQueue->aSubSysCmdArray[(_x_)])

/* Interface APIs for operating the SCQ by L1 */
extern void L1_InitSCQ(void);
extern U32 L1_IsSCQEmpty(void);
extern U32 L1_SCQGetCount(void);
extern PSCMD L1_GetSCmdFromHead(void);
extern void L1_PopSCmdNode(U32 ulRetStatus);
extern PSCMD L1_GetSCmd(void);
extern void L1_SCmdFinish(void);
extern void L1_SCmdFail(void);
extern void L1_SCmdAbort(void);
extern PSCMD L1_TryFetchScmd(U32* pucPointer);
extern U32 L1_GetCurrentScqTailPointer(void);
extern U32 L1_GetCurrentScqHeadPointer(void);

#endif // _L1_SCMDINTERFACE_H

