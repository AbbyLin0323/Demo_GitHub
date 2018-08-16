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
Filename    :HAL_BufMap.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    19:18:57
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/

#ifndef _COM_MEMORY_H
#define _COM_MEMORY_H

#include "BaseDef.h"

void COM_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue);
void COM_MemZero(U32* TargetAddr,U32 LengthDW);
extern void COM_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW);
extern void COM_MemIncBaseAddr(U32* Addr, U32 Offset);
extern void COM_MemAddrPageBoundaryAlign(U32 *pMemAddr);
extern void COM_MemAddr16DWAlign(U32 *pMemAddr);
extern U32 COM_MemSize16DWAlign(U32 ulSize);
void COM_MemByteCopy(U8* TargetAddr,U8* SrcAddr,U32 ByteLen);
U32 COM_DiffU32(U32 ulStart, U32 ulEnd);
U32 COM_DwordToString(U32 ulInputDword, U8 *pOutputString);


#endif

