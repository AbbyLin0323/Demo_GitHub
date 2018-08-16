/*******************************************************************************
*Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.
**
*This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.*
*and may contain trade secrets and/or other confidential information of VIA*
*Technologies,Inc.*
*This file shall not be disclosed to any third party, in whole or in part,*
*without prior written consent of VIA.*
**
*THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,*
*WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,*
*AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF*
*MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR*
*NON-INFRINGEMENT.*
********************************************************************************
Filename:HAL_TSC.c
Version:
Author:
Date:2016.5
Description :this file encapsulate TSC driver interface
Others:
Modify:
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_GLBReg.h"
#include "HAL_TSC.h"
#include "HAL_xtensa.h"
#include "HAL_ParamTable.h"
#include "HAL_TemperatureSensor.h"

#define RTSC_CLK_EN             (1 << 15)
#define DEF_WAIT_CHK_TIME_US    (1000 * 100)
#define DEF_WAIT_CHK_UTIME_US   (100)
#define K_10VALUE               (2781)
#define Y_10VALUE               (7012)
#define TMP_100VALUE(N)         ((N) * (Y_10VALUE) * 10 / 4096 - 10 * (K_10VALUE))

extern TEMPERATURE_SENSOR_I2C *HAL_GetTemperatureSensorI2C(void);
LOCAL volatile TSCREG *l_pTSCCtrlStaReg;
GLOBAL MCU12_VAR_ATTR TSC_SENSOR_PARAM g_TSCSensorParam;

LOCAL BOOL MCU12_DRAM_TEXT HAL_TSCModeChangeWaitChk(BOOL bWaiteChk, U32 ulWaitTime, U32 ulWaitTimeUnit)
{
    U32 ulStepCnt;
    U32 bResult = FALSE;

    if (TRUE == bWaiteChk)
    {
        for (ulStepCnt = 0; ulStepCnt < (ulWaitTime / ulWaitTimeUnit); ulStepCnt++)
        {
            if (TRUE == l_pTSCCtrlStaReg->bsTSCModeEn)
            {
                bResult = TRUE;
                break;
            }
            else
            {
#ifndef SIM
                HAL_DelayUs(ulWaitTimeUnit);
#endif
            }
        }

    }
    else
    {
        while (l_pTSCCtrlStaReg->bsTSCModeEn != 1)
        {
            ;
        }
        bResult = TRUE;
    }

    return bResult;
}

BOOL MCU12_DRAM_TEXT HAL_TSCModeSet(U32 ulModeType, BOOL bWaiteChk)
{
    //DBG_Printf("HAL_TSCModeSet\n");
    BOOL bFinish = FALSE;
    if (TRUE != l_pTSCCtrlStaReg->bsTSCEn)
    {
        DBG_Printf("bsTSCEn error\n");
        DBG_Getch();
    }

    //argument check
    switch (ulModeType)
    {
    case TSC_MODE_CONTINUOUS:
    case TSC_MODE_CHECK:
    case TSC_MODE_POWER_ON:
        break;
    default:
        DBG_Getch();
        break;
    }

    //Switch to target mode directly while current mode is power_on, otherwise
    //switch to power_on first.
    if ((TSC_MODE_POWER_ON != l_pTSCCtrlStaReg->bsTSCMode) && (TSC_MODE_POWER_ON != ulModeType))
    {
        l_pTSCCtrlStaReg->bsTSCMode = TSC_MODE_POWER_ON;
        l_pTSCCtrlStaReg->bsTSCModeValid = 1;
        bFinish = HAL_TSCModeChangeWaitChk(bWaiteChk, DEF_WAIT_CHK_TIME_US, DEF_WAIT_CHK_UTIME_US);
        if (FALSE == bFinish)
        {
            DBG_Printf("Wait Switch TSC_MODE_POWER_ON timeout\n");
            return FALSE;
        }

        l_pTSCCtrlStaReg->bsTSCMode = ulModeType;
        l_pTSCCtrlStaReg->bsTSCModeValid = 1;
        bFinish = HAL_TSCModeChangeWaitChk(bWaiteChk, DEF_WAIT_CHK_TIME_US, DEF_WAIT_CHK_UTIME_US);
    }
    else
    {
        l_pTSCCtrlStaReg->bsTSCMode = ulModeType;
        l_pTSCCtrlStaReg->bsTSCModeValid = 1;
        bFinish = HAL_TSCModeChangeWaitChk(bWaiteChk, DEF_WAIT_CHK_TIME_US, DEF_WAIT_CHK_UTIME_US);
        if (FALSE == bFinish)
        {
            //DBG_Printf("Wait Switch %d timeout\n", ulModeType);
            return FALSE;
        }
    }
    return TRUE;
}

void MCU12_DRAM_TEXT HAL_TSCMeaTimeSet(U32 ulMeasureTime)
{
    l_pTSCCtrlStaReg->bsTSCTime = ulMeasureTime;
    l_pTSCCtrlStaReg->bsTSCTimeValid = 1;
}

void MCU12_DRAM_TEXT HAL_TSCTrimSet(U32 ulTrimValue)
{
    if (ulTrimValue > 15)
        DBG_Getch();

    l_pTSCCtrlStaReg->bsTSCTrim = ulTrimValue;
    l_pTSCCtrlStaReg->bsTSCTrimValid = 1;
}

void MCU12_DRAM_TEXT HAL_TSC2Uart(void)
{
    U32 ulTscTemp = 0, ulTscTemp1 = 0, ulValue = 0, ulI2CValue = 0;
    if (g_TSCSensorParam.ucDeviceType & 0x08)/*bit[3] enable TSC*/
    {
        ulValue = (U32)HAL_TSCTempValueGet(1);
        ulTscTemp = ulValue / 100;
        ulTscTemp1 = ulValue % 100;
       //DBG_Printf("TSC %d.%d oC\n", ulTscTemp, ulTscTemp1);
    }
    if (g_TSCSensorParam.ucDeviceType & 0x07)/*bit[2:0] enable IIC Sensor*/
    {
        ulI2CValue= (U32)HAL_GetTemperature();
        //DBG_Printf("IIC  %d oC\n", ulI2CValue);
    }
    DBG_Printf("TSC, IIC:%d.%d, %d oC\n", ulTscTemp, ulTscTemp1, ulI2CValue);
    HAL_StartMcuTimer1(g_TSCSensorParam.ucTsc2Uart * 1000000, HAL_TSC2Uart);
}

BOOL MCU12_DRAM_TEXT HAL_TSCInit(void)
{
    BOOL bResult = FALSE;
    l_pTSCCtrlStaReg = (volatile TSCREG *)TSC_REG_BASE;

#ifdef MCU1
    TEMPERATURE_SENSOR_I2C *pTempSensor;
    TEMPERATURE_SENSOR_TSC *pTSCTempSensor;

    pTempSensor = HAL_GetTemperatureSensorI2C();
    g_TSCSensorParam.ucDeviceType = (pTempSensor->bsTemperatureSensorType) & 0x3F;
    g_TSCSensorParam.ucInitSts = 0;
    g_TSCSensorParam.ucTscClkEn = 0;
    g_TSCSensorParam.ucTscTime = 0;
    g_TSCSensorParam.ucTsc2Uart = 0;//init
    g_TSCSensorParam.ucDelta =0;
    g_TSCSensorParam.ucLoThreshold = 0;
    g_TSCSensorParam.ucHiThreshold  = 0;
    if (g_TSCSensorParam.ucDeviceType & TSC_SENSOR)//bit 3
    {
        pTSCTempSensor = HAL_GetTSCSmartValue();
        g_TSCSensorParam.ucTscClkEn = pTempSensor->bsTSCCLKEN;
        g_TSCSensorParam.ucTscTime = pTempSensor->bsTSCTime;
        g_TSCSensorParam.ucTsc2Uart = pTempSensor->bsTSC2UART;
        g_TSCSensorParam.ucDelta = pTSCTempSensor->bsDelta;
        g_TSCSensorParam.ucLoThreshold = pTSCTempSensor->bsLoThreshold;
        g_TSCSensorParam.ucHiThreshold = pTSCTempSensor->bsHiThreshold;
        DBG_Printf("TSC Init: sensor parameter is TSC 0x%x\n", g_TSCSensorParam.ucDeviceType);
        DBG_Printf("TSC Init: Delta %d\n", g_TSCSensorParam.ucDelta);
        DBG_Printf("TSC Init: LoThreshold %d\n", g_TSCSensorParam.ucLoThreshold);
        DBG_Printf("TSC Init: HiThreshold %d\n", g_TSCSensorParam.ucHiThreshold);
        //DBG_Printf("TSC Init: ucTsc2Uart %d second\n", g_TSCSensorParam.ucTsc2Uart);

        if (g_TSCSensorParam.ucTscClkEn)
        {
            rGlbMcuSgeRst |= R_RST_TSC;
            HAL_DelayCycle(50);
            rGlbMcuSgeRst &= ~R_RST_TSC;
            rGlbClkCtrl |= RTSC_CLK_EN;
            l_pTSCCtrlStaReg->bsTSCEn = 1;
            bResult = HAL_TSCModeSet(TSC_MODE_POWER_ON, 1);
            if (TRUE == bResult)
            {
                HAL_TSCMeaTimeSet(g_TSCSensorParam.ucTscTime);
                HAL_TSCTrimSet(8);//calibration with room  temperature
                HAL_TSCModeSet(TSC_MODE_CONTINUOUS, 1);
                g_TSCSensorParam.ucInitSts = 1;//init success
                DBG_Printf("HAL TSC Init PASS!\n");
            }
            else
            {
                DBG_Printf("HAL TSC Init FAIL!\n");
            }
            if (g_TSCSensorParam.ucDeviceType & TSC_ENABLE_UART)/*bit[4] enable uart log*/
            {
                HAL_StartMcuTimer1(g_TSCSensorParam.ucTsc2Uart * 1000000, HAL_TSC2Uart);
            }
        }
    }
    else
    {
        DBG_Printf("HAL TSC Init: sensor parameter is not TSC 0x%x\n", g_TSCSensorParam.ucDeviceType);
    }
#endif
    return bResult;
}

void MCU12_DRAM_TEXT HAL_TSCDisable(void)
{
    rGlbMcuSgeRst |= R_RST_TSC;
    rGlbClkCtrl &= ~RTSC_CLK_EN;
}


S32 MCU12_DRAM_TEXT HAL_TSCTempValueGet(BOOL bWait)
{
    U32 ulValue;
    S32 slMdValue;
    U32 ulStepCnt = 0;
    U32 bFinish = FALSE;
    U32 bStatus = TRUE;

    switch (l_pTSCCtrlStaReg->bsTSCMode & 0x3)
    {
    case TSC_MODE_CONTINUOUS:
    case TSC_MODE_CHECK:
        break;
    case TSC_MODE_POWER_ON:
        bStatus = HAL_TSCModeSet(TSC_MODE_CONTINUOUS, 1);//Use default setting
        break;
    default:
        DBG_Printf("Warning:Please call HAL_TSCInit() before invoke HAL_TSCTempValueGet\n");
        bStatus = HAL_TSCInit();
        break;
    }
    if (FALSE == bWait)
    {
        while (1 != l_pTSCCtrlStaReg->bsTSCValid);

        ulValue = l_pTSCCtrlStaReg->bsTSCValue;
        bFinish = TRUE;
    }
    else
    {
        for (ulStepCnt = 0; ulStepCnt < (DEF_WAIT_CHK_TIME_US / DEF_WAIT_CHK_UTIME_US); ulStepCnt++)
        {
            if (TRUE == l_pTSCCtrlStaReg->bsTSCValid)
            {
                ulValue = l_pTSCCtrlStaReg->bsTSCValue;
                bFinish = TRUE;
                break;
            }
            else
            {
#ifndef SIM
                HAL_DelayUs(DEF_WAIT_CHK_UTIME_US);
#endif
            }
        }
    }
    if ((bFinish == FALSE) || (bStatus == FALSE))
    {
        DBG_Printf("Get Temputure fail\n");
        slMdValue = 3800;//38.00c
    }
    else
    {
        slMdValue = TMP_100VALUE(ulValue);
    }

    return slMdValue;
}


