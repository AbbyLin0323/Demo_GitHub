/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :COM_Memory.c
Version     :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
/*#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
*/
#include "COM_Inc.h"

/****************************************************************************
Name        :COM_MemSet
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void COM_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = SetValue;
    }
}

/****************************************************************************
Name        :COM_MemZero
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void COM_MemZero(U32* TargetAddr,U32 LengthDW)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = 0;
    }
}

/****************************************************************************
Name        :COM_MemCpy
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void COM_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = *SrcAddr++;
    }
}
