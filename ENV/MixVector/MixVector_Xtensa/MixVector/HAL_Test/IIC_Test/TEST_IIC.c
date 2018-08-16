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
Filename    : TEST_IIC.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "HAL_IIC.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"

/*----------------------------------------------------------------------------
Name: TEST_IICHwCheckList
Description: 
    In ticket 807,IIC checklist:
    write 16 bytes data to slave,then read 16 bytes data from slave.
------------------------------------------------------------------------------*/
void TEST_IICHwCheckList(void)
{
    U8 aTxData[16] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
    U8 aRxData[16];

    /* use HW default device ID 1, so needn't to set device ID */
    while (FALSE == HAL_IICWriteData(6, &(aTxData[0])))
    {
    }
    rTracer = 0xE0000001;

    while (FALSE == HAL_IICWriteData(2, &(aTxData[8])))
    {
    }
    rTracer = 0xE0000002;

    while (FALSE == HAL_IICReadData(6, &(aRxData[0])))
    {
    }
    rTracer = 0xE0000003;

    while (FALSE == HAL_IICReadData(6, &(aTxData[8])))
    {
    }
    rTracer = 0xE0000004;

    return;
}

void TEST_IICMain(void)
{
    if (MCU1_ID == HAL_GetMcuId())
    {
        rTracer = 0xEE000000;

        HAL_IICInit();
        rTracer = 0xE0000000;

        TEST_IICHwCheckList();
    }

    return;
}
