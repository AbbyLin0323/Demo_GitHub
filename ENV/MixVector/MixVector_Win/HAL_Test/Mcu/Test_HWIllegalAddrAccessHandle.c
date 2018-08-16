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
Filename    : TEST_HWIllegalAddrAccessHandle.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "BaseDef.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "Test_McuBasicFunc.h"

#define SECTION0_START_MCU0 (0x1ff84000)
#define SECTION0_START_MCU1 (0x1ff84000)
#define SECTION0_START_MCU2 (0x1ff84000)
#define SECTION0_END_MCU0 (0x20000000)
#define SECTION0_END_MCU1 (0x20000000)
#define SECTION0_END_MCU2 (0x20000000)

#define SECTION1_START_MCU0 (0x20008000)
#define SECTION1_START_MCU1 (0x20020000)
#define SECTION1_START_MCU2 (0x20020000)
#define SECTION1_END_MCU0 (0x30000000)
#define SECTION1_END_MCU1 (0x30000000)
#define SECTION1_END_MCU2 (0x30000000)

#define SECTION2_START_MCU0 (0x40000000)
#define SECTION2_START_MCU1 (0x40000000)
#define SECTION2_START_MCU2 (0x40000000)
#define SECTION2_END_MCU0   (0xffe00000)
#define SECTION2_END_MCU1   (0xffffffff)
#define SECTION2_END_MCU2   (0xffffffff)

#define SECTION3_START_MCU0 (0xffe06000)
#define SECTION3_END_MCU0   (0xffffffff)

#define SECTION0_STEP (0x2000) //8k
#define SECTION1_STEP (0x4000)//16k
#define SECTION2_STEP (0x4000)//16k
#define SECTION3_STEP (0x4000)//16k

#define SECTION0_DATA (0x10000000)
#define SECTION1_DATA (0x20000000)
#define SECTION2_DATA (0x30000000)
#define SECTION3_DATA (0x40000000)

LOCAL void MCU_SetMutiCoreMode(void)
{
    rGlbMCUSramMap = 0x0;
    return;
}

LOCAL void MCU_MCU12BootFromIsram(void)
{
    rGlbMcuSgeRst = 0xf;
    rGlbMCUCtrl |= 0x110;
    rGlbMcuSgeRst = 0x0;
    return;
}

LOCAL void MCU_ConfigHWErrorAck()
{
    rGlbMCUMisc |= 0<<15;
    rGlbMCUMisc |= 1<<27;

    return;
}

LOCAL void MCU_WriteData(U32 ulBaseAddr, U32 ulOffsetAddr, U32 ulStep, U32 ulBaseData, U32 ulStartIndex, U32 ulIndexNum)
{
    U32 ulIndex;
    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }

    for (ulIndex = ulStartIndex; ulIndex < ulIndexNum; ulIndex++)
    {
        rTracer = ulBaseAddr + ulOffsetAddr + ulIndex*ulStep;
        *((volatile U32*)(ulBaseAddr + ulOffsetAddr + ulIndex*ulStep)) = ulBaseData + ulOffsetAddr + ulIndex;
    }
    
    return;
}

LOCAL void MCU_ReadData(U32 ulBaseAddr, U32 ulOffsetAddr, U32 ulStep, U32 ulStartIndex, U32 ulIndexNum)
{
    U32 ulIndex;
    U32 ulTempData;

    if (ulIndexNum <= ulStartIndex)
    {
        DBG_Getch();
    }
    
    for (ulIndex = ulStartIndex; ulIndex <ulIndexNum; ulIndex++)
    {
        rTracer = ulBaseAddr + ulOffsetAddr + ulIndex*ulStep;
        ulTempData = *(volatile U32*)(ulBaseAddr + ulOffsetAddr + ulIndex*ulStep);
    }
    
    return;
}


void Test_HwIllegalHandleMain()
{
    U32 ulNum;
    U32 ulMcuID;
    ulMcuID  =  HAL_GetMcuId();

    if (MCU0_ID == ulMcuID)
    {
        MCU_ConfigHWErrorAck();
        MCU_SetMutiCoreMode();
        MCU_MCU12BootFromIsram();
#if 0
        //0x1ff84000 ~ 0x1fffffff
        ulNum = (SECTION0_END_MCU0 - SECTION0_START_MCU0)/SECTION0_STEP;
        MCU_WriteData(SECTION0_START_MCU0, 0, SECTION0_STEP, SECTION0_DATA, 0, ulNum);
        MCU_ReadData(SECTION0_START_MCU0, 0, SECTION0_STEP, 0, ulNum);

        ulNum = (SECTION1_END_MCU0 - SECTION1_START_MCU0)/SECTION1_STEP;
        MCU_WriteData(SECTION1_START_MCU0, 0, SECTION1_STEP, SECTION1_DATA, 0, ulNum);
        MCU_ReadData(SECTION1_START_MCU0, 0, SECTION1_STEP, 0, ulNum);
#endif

        ulNum = (SECTION2_END_MCU0 - SECTION2_START_MCU0)/SECTION2_STEP;
        MCU_WriteData(SECTION2_START_MCU0, 0, SECTION2_STEP, SECTION2_DATA, 0, ulNum);
        MCU_ReadData(SECTION2_START_MCU0, 0, SECTION2_STEP, 0, ulNum);

        ulNum = ((SECTION3_END_MCU0 - SECTION3_START_MCU0) + 1)/SECTION3_STEP;
        MCU_WriteData(SECTION3_START_MCU0, 0, SECTION3_STEP, SECTION3_DATA, 0, ulNum);
        MCU_ReadData(SECTION3_START_MCU0, 0, SECTION3_STEP, 0, ulNum);

        rTracer_Mcu0 = 0xaa;
    }
    else if (MCU1_ID == ulMcuID)
    {
#if 0
        //0x1ff84000 ~ 0x1fffffff
        ulNum = (SECTION0_END_MCU1 - SECTION0_START_MCU1)/SECTION0_STEP;
        MCU_WriteData(SECTION0_START_MCU1, 0, SECTION0_STEP, SECTION0_DATA, 0, ulNum);
        MCU_ReadData(SECTION0_START_MCU1, 0, SECTION0_STEP, 0, ulNum);
#endif
        ulNum = (SECTION1_END_MCU1 - SECTION1_START_MCU1)/SECTION1_STEP;
        MCU_WriteData(SECTION1_START_MCU1, 0, SECTION1_STEP, SECTION1_DATA, 0, ulNum);
        MCU_ReadData(SECTION1_START_MCU1, 0, SECTION1_STEP, 0, ulNum);

        ulNum = ((SECTION2_END_MCU1 - SECTION2_START_MCU1) + 1)/SECTION2_STEP;
        MCU_WriteData(SECTION2_START_MCU1, 0, SECTION2_STEP, SECTION2_DATA, 0, ulNum);
        MCU_ReadData(SECTION2_START_MCU1, 0, SECTION2_STEP, 0, ulNum);

        rTracer_Mcu1 = 0xbb;

    }
    else//MCU2_ID
    {
#if 0
        //0x1ff84000 ~ 0x1fffffff
        ulNum = (SECTION0_END_MCU2 - SECTION0_START_MCU2)/SECTION0_STEP;
        MCU_WriteData(SECTION0_START_MCU2, 0, SECTION0_STEP, SECTION0_DATA, 0, ulNum);
        MCU_ReadData(SECTION0_START_MCU2, 0, SECTION0_STEP, 0, ulNum);
#endif
        ulNum = (SECTION1_END_MCU2 - SECTION1_START_MCU2)/SECTION1_STEP;
        MCU_WriteData(SECTION1_START_MCU2, 0, SECTION1_STEP, SECTION1_DATA, 0, ulNum);
        MCU_ReadData(SECTION1_START_MCU2, 0, SECTION1_STEP, 0, ulNum);

        ulNum = ((SECTION2_END_MCU2 - SECTION2_START_MCU2) + 1)/SECTION2_STEP;
        MCU_WriteData(SECTION2_START_MCU2, 0, SECTION2_STEP, SECTION2_DATA, 0, ulNum);
        MCU_ReadData(SECTION2_START_MCU2, 0, SECTION2_STEP, 0, ulNum);

        rTracer_Mcu2 = 0xcc;
    }

    while(1);
    
    return;
}
