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
Filename    :COM_Memory.c
Version     :
Author      :
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_HostInterface.h"

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

void COM_MemIncBaseAddr(U32* Addr, U32 Offset)
{
    *Addr += Offset;

    return;
}

/*address align by 32KBytes size*/
void COM_MemAddrPageBoundaryAlign(U32 *pMemAddr)
{
    U32 ulMemAddr;

    ulMemAddr = *pMemAddr;

    /* page align in memory */
    if( (ulMemAddr & THIRTYTWOKB_SIZE_MSK) != 0)
    {
        ulMemAddr = ( ulMemAddr & (~THIRTYTWOKB_SIZE_MSK)) + THIRTYTWOKB_SIZE;
    }

    *pMemAddr = ulMemAddr;

    return;
}

/* 16DW align */
void COM_MemAddr16DWAlign(U32 *pMemAddr)
{
    U32 ulMemAddr;

    ulMemAddr = *pMemAddr;

    /* 16DW align in memory */
    if( (ulMemAddr & SIXTEENDW_SIZE_MSK) != 0)
    {
        ulMemAddr = (ulMemAddr & (~SIXTEENDW_SIZE_MSK)) + SIXTEENDW_SIZE;
    }

    *pMemAddr = ulMemAddr;

    return;
}

/* 16DW align */
U32 COM_MemSize16DWAlign(U32 ulSize)
{
    /* 16DW align for size */
    if( (ulSize & SIXTEENDW_SIZE_MSK) != 0)
    {
        return ((ulSize & (~SIXTEENDW_SIZE_MSK)) + SIXTEENDW_SIZE);
    }
    else
    {
        return ulSize;
    }
}

void COM_MemByteCopy(U8* TargetAddr,U8* SrcAddr,U32 ByteLen)
{
    U32 i;

    for (i = 0 ; i < ByteLen; i++)
    {
        *TargetAddr++ = *SrcAddr++;
    }
}

U32 COM_DiffU32(U32 ulStart, U32 ulEnd)
{
    U32 ulDiff;
    
    if (ulEnd > ulStart)
    {
        ulDiff = ulEnd - ulStart; 
    }
    else
    {
        ulDiff = INVALID_8F - ulStart + ulEnd;
    }

    return ulDiff;
}

U32 COM_DwordToString(U32 ulInputDword, U8 *pOutputString)
{
    U32 index;
    U8  ucNum;
    
    for (index = 0; index < 8; index++)
    {
        ucNum = ((ulInputDword >> (index*4)) & 0xf);

        if (ucNum <= 9)
        {
            pOutputString[index] = ucNum + '0';
        }
        else
        {
            pOutputString[index] = '.';
        }
    }
    return 0;
}
