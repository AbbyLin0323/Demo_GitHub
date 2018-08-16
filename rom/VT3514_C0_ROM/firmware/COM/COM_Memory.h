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

#ifndef _COM_MEMORY_H
#define _COM_MEMORY_H

#include "BaseDef.h"

void COM_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue);
void COM_MemZero(U32* TargetAddr,U32 LengthDW);
extern void COM_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW);
extern void COM_MemIncBaseAddr(U32* Addr, U32 Offset);
extern void COM_MemAddr32KBAlign(U32 *pMemAddr);
extern void COM_MemAddr16DWAlign(U32 *pMemAddr);
extern U32 COM_MemSize16DWAlign(U32 ulSize);

#endif

