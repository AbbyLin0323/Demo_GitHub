/*******************************************************************************
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
********************************************************************************
Filename    : HAL_EFUSE.c
Version     : Ver 1.0
Author      : Victor
Date        : 2015.1.16
Description : Driver for programing into /reading from EFUSE(one-time-programed) memory. 
Others      : 
Modify      :
20150116    Victor     Create file
*******************************************************************************/
#include "HAL_EFUSE.h"
#include "HAL_GLBReg.h"

volatile EFUSE_REG_SET * const l_pEfuseRegSet = (volatile EFUSE_REG_SET *)(REG_BASE_EFUSE);

/*
FUNC: HAL_EfuseIsProg
IN:     void
OUT:    TRUE -- programed successfully
        FALSE-- not programed
Description:
    Indicate to the programing status.
*/
BOOL HAL_EfuseIsProg(void)
{
    return (BOOL)l_pEfuseRegSet->bsProgramed;
}

/*
FUNC: HAL_EfuseIsReadReady
IN:     void
OUT:    TRUE -- available
        FALSE-- unavailable
Description:
    Indicate to the output data available.  
*/

BOOL HAL_EfuseIsReadReady(void)
{
    l_pEfuseRegSet->bsDataValid = 1;
    return (BOOL)l_pEfuseRegSet->bsDataValid;
}

/*
FUNC: HAL_EfuseProg
IN:     data source address 
        data length in DWORD
OUT:    void
Description:
    Write 8 DWORD data into EFUSE memory  
*/

void HAL_EfuseProg(U32* pSrc,U32 ulLenDW)
{
    COM_MemCpy(l_pEfuseRegSet->aDATA,pSrc,ulLenDW);
}

/*
FUNC: HAL_EfuseRead
IN:     data destine address 
        data length in DWORD
OUT:    void
Description:
    Read 8 DWORD data from EFUSE memory  
*/

BOOL HAL_EfuseRead(U32* pDst,U32 ulLenDW)
{
    while(FALSE == HAL_EfuseIsReadReady())
    {
        ;
    }

    COM_MemCpy(pDst,l_pEfuseRegSet->aDATA,ulLenDW);
    return TRUE;
}

#ifdef EFUSE_TEST
void HAL_EfuseTestMain(void)
{
    U32 i;
    U32 SrcAddr = 0xfff00000;
    for (i=0;i<8;i++)
    {
        *((U32*)SrcAddr + i) = 0x5aa53c30 + i;
    }

    if (FALSE == HAL_EfuseIsProg())
    {
        HAL_EfuseProg((U32*)SrcAddr,8);
    }

    rTracer = 0x8000;
    while(FALSE == HAL_EfuseIsProg())
    {
        ;
    }
    
    COM_MemCpy(SrcAddr+0x100,l_pEfuseRegSet->aDATA,8);
    
    for (i=0;i<8;i++)
    {
        if (0 != *((U32*)(SrcAddr+0x100) + i))
        {
            rTracer = 0x2badc0de;
            rTracer = SrcAddr+0x100;
        //    rTracer = *((U32*)(SrcAddr+0x100) + i);
            while(1);
        }
    }

    if (FALSE  == HAL_EfuseRead((U32*)(SrcAddr + 0x100) ,8))
    {
        rTracer = 0x1badc0de;
        while(1);
    }

    for (i=0;i<8;i++)
    {
        if ((0x5aa53c30 + i) != *((U32*)(SrcAddr+0x100) + i))
        {
            rTracer = 0xbadc0de;
            rTracer = SrcAddr+0x100;
            rTracer = *((U32*)(SrcAddr+0x100) + i);
            while(1);
        }
    }
    rTracer = 0x9000;
}
#endif

