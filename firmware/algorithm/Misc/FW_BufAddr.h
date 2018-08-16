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
Filename    :FW_BufAddr.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/

#ifndef _FW_BUFADDR_H
#define _FW_BUFADDR_H

#include "BaseDef.h"

#define K1 31
#define K2 29
#define K3 28
#define K4 25
#define Q1 6
#define Q2 2
#define Q3 13
#define Q4 3
#define S1 18
#define S2 10
#define S3 23
#define S4 11

typedef struct {
    U32 z1, z2, z3, z4;
} taus_state_t;

#define LCGCTG(n) (65413 * n)

extern U32 COM_GetBufferIDByMemAddr(U32 TargetAddr,U8 bDram,U8 BufferSizeBits);
extern U32 COM_GetMemAddrByBufferID(U32 BufferID,U8 bDram,U8 BufferSizeBits);
extern void COM_MemClearSectorData(U32 ulBufAddr, U8 ucBitMap);

void  COM_SetCTGSeeds(U32 s, taus_state_t *state);
U32 COM_TausGet(taus_state_t *state);
void COM_WriteDataBuffInit(void);

#endif

