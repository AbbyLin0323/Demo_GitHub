/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_BufMap.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    19:18:57
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/

#ifndef _HAL_BUFFER_H
#define _HAL_BUFFFR_H

#include "BaseDef.h"

void HAL_MemInit();
U32 HAL_MemGetZeroValueInDRAM();
void HAL_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue);
void HAL_MemZero(U32* TargetAddr,U32 LengthDW);
void HAL_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW);
U32 HAL_GetBufferIDByMemAddr(U32 TargetAddr,U8 bDram,U8 BufferSizeBits);
U32 HAL_GetMemAddrByBufferID(U32 BufferID,U8 bDram,U8 BufferSizeBits);

#endif

