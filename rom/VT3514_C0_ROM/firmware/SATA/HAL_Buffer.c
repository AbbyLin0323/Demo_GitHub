/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_Buffer.c
Version     :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "HAL_Define.h"
#include "HAL_Buffer.h"
#include "HAL_GLBReg.h"
U32* pHalMemZeroDW;
void HAL_MemInit()
{

    pHalMemZeroDW = (U32*)STATIC_PARAMETER_ZERO_VALUE;
    *pHalMemZeroDW = 0;

#ifdef DCACHE
    rGLB_40 |= (1<<19);//map DDR to 0x10000000 and 0x80000000 both 
#endif

}

U32 HAL_MemGetZeroValueInDRAM()
{
    return *pHalMemZeroDW;
}

/****************************************************************************
Name        :HAL_MemSet
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_MemSet(U32* TargetAddr,U32 LengthDW,U32 SetValue)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = SetValue;
    }
}

/****************************************************************************
Name        :HAL_MemZero
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_MemZero(U32* TargetAddr,U32 LengthDW)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = 0;
    }
}

/****************************************************************************
Name        :HAL_MemCpy
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
void HAL_MemCpy(U32* TargetAddr,U32* SrcAddr,U32 LengthDW)
{
    U32 i;

    for (i = 0 ; i < LengthDW; i++)
    {
        *TargetAddr++ = *SrcAddr++;
    }
}


/****************************************************************************
Name        :HAL_GetBufferIDByMemAddr
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
U32 HAL_GetBufferIDByMemAddr(U32 TargetAddr,U8 bDram,U8 BufferSizeBits)
{
    U32 BuffID;
    U32 MemStartAddr;

    MemStartAddr = bDram ? DRAM_START_ADDRESS : OTFB_START_ADDRESS;

    BuffID = (TargetAddr - MemStartAddr) >> BufferSizeBits;

    return BuffID;
}


/****************************************************************************
Name        :HAL_GetMemAddrByBufferID
Input       :
Output      :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
U32 HAL_GetMemAddrByBufferID(U32 BufferID,U8 bDram,U8 BufferSizeBits)
{
    U32 MemStartAddr;
    U32 MemAddr;

    MemStartAddr = bDram ? DRAM_START_ADDRESS : OTFB_START_ADDRESS;

    MemAddr = (BufferID << BufferSizeBits) + MemStartAddr;

    return MemAddr;
}
