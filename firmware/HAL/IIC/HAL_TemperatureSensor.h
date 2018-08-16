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
Filename    :HAL_TemperatureSensor.h
Version     :
Author      :William Wang
Date        :2015.12
Description :header file for Temperature Sensor driver.
Others      :
Modify      :
20151228    William Create
*******************************************************************************/
#ifndef _HAL_TEMPERATURE_SENSOR_H_
#define _HAL_TEMPERATURE_SENSOR_H_

#include "BaseDef.h"

/*Temperatur Sensor Type define*/
#define NOSENSOR                0x0
#define G752KC2G                0x1
#define LM73CIMK_0              0x2
#define S_5851AAA_I6T1U         0x3
#define TI_TMP102               0x04
#define I2C_SENSOR              0x07

#define IIC_100KHZ              0x0
#define IIC_400KHZ              0x1
#define IIC_1MHZ                0x2

typedef struct _IIC_SENSOR_PARAM
{
    U8 ucDeviceType;    //IIC device Type
    U8 ucDevAddr;       //IIC device address
    U8 ucDevClock;      //IIC device clock
}IIC_SENSOR_PARAM;

typedef struct _IIC_SENSOR_REGISTER
{
    U16 usTempReg;               //IIC device Register 0
    U16 usIDReg;                 //IIC device Register 7
    union
    {
        struct                   //G752
        {
            U16 usTHYSTReg;      //IIC device Register 2
            U16 usTOSReg;        //IIC device Register 3
        };
        struct                   //LM73
        {
            U16 usTHighReg;      //IIC device Register 2
            U16 usTLowReg;       //IIC device Register 3
        };
    };


    union                        //IIC device Register 1
    {
        U16 ucValue;
        struct                   //G752
        {
            U16 bShutDown   : 1;
            U16 bCmtInt     : 1;
            U16 bOSPolarity : 1;
            U16 bFaultQueue : 2;
            U16 bResv       : 11;
        };
        struct                   //LM73
        {
            U16 bRes0       : 2;
            U16 bOneShot    : 1;
            U16 bAlrtRst    : 1;
            U16 bAltPolary  : 1;
            U16 bAlrtEn     : 1;
            U16 bRes1       : 1;
            U16 bPowerDown  : 1;
            U16 bResv3      : 8;
        };
        struct                   //S5851A
        {
            U16 bSD         : 1;
            U16 bRes2       : 6;
            U16 bOS         : 1;
            U16 bResv4      : 8;
        };
        struct                   //TI_TMP102
        {
            U16 bSD_1   : 1;
            U16 bTM     : 1;
            U16 bPOL    : 1;
            U16 bF0F1   : 2;
            U16 bR0R1   : 2;
            U16 bOS_1   : 1;
            U16 bResv5  : 4;
            U16 bEM     : 1;
            U16 bAL     : 1;
            U16 bCR0CR1 : 2;
        };
    }ucConfigReg;

    union                        //IIC device Register 4
    {
        U8 ucValue;
        struct
        {
            U8 bDataAvailable   : 1;
            U8 bTempLow         : 1;
            U8 bTempHigh        : 1;
            U8 bAlrtStats       : 1;
            U8 bRes             : 1;
            U8 bTemResolution   : 2;
            U8 bTimeOutDisable  : 1;
        };
    }ucCtlStatsReg;
}IIC_SENSOR_REGISTER;

//Pointer Register

// Temperature Pointer
#define TM_POINTER            0x0
// Configuration Pointer
#define CONFIG_POINTER        0x1
// THYST Set Pointer
#define THYST_SET_POINTER     0x2
// TOS Set Pointer
#define TOS_SET_POINTER       0x3
// THIGH Set Pointer
#define THIGH_SET_POINTER     0x2
// TLOW Set Pointer
#define TLOW_SET_POINTER      0x3
// CTLSTATS Set Pointer
#define CTLSTATS_SET_POINTER  0x4
// ID Set Pointer
#define ID_SET_POINTER        0x7
// THIGH Set Pointer
#define TMP102_THIGH_SET_POINTER     0x3
// TLOW Set Pointer
#define TMP102_TLOW_SET_POINTER      0x2

// Configuration Register
#define SHUTDOWN_BIT       0x0
#define CMPINT_BIT         0x1
#define OSPOLARITY_BIT     0x2
#define FAULTQUEUE_BIT     0x3
#define OS_BIT_LM73        0x2
#define ALRT_RST           0x3
#define ALRT_POL           0x4
#define ALRT_EN            0x5
#define SHUTDOWN_BIT_LM73  0x7
#define OS_BIT_S5851A      0x7

//Control Status Register
#define DATA_AVAILABLE_BIT  0x0
#define TEMP_LOW_BIT        0x1
#define TEMP_HIGH_BIT       0x2
#define ALERT_STATS_BIT     0x3
#define TEMP_RESOLUTION_BIT 0x5
#define TIMEOUT_DISABLE_BIT 0x7

BOOL HAL_TemperatureSensor_Init(void);
U8  HAL_GetTemperature(void);
U16  HAL_GetConfig(void);
BOOL  HAL_SetConfig(U8 FaultQueue, BOOL OSPolarity, BOOL CmtInt, BOOL ShutDown);
BOOL  HAL_SetConfig_LM73(BOOL ShutDown, BOOL AlertEn, BOOL AlertPol, BOOL AlertRst, BOOL OneShot);
BOOL  HAL_SetConfig_S5851A(BOOL OneShot, BOOL ShutDown);
BOOL  HAL_SetConfig_TMP102(U8 byte1, U8 byte2);

U8  HAL_GetTHYST_TLow(void);
BOOL  HAL_SetTHYST_TLow(U16 THYST);
U16  HAL_GetTOS_THigh(void);
BOOL  HAL_SetTOS_THigh(U16 TOS);
U8  HAL_GetCtlStats(void);
BOOL  HAL_SetCtlStats(BOOL TimeOutDisable, U8 TempResolution);
U16  HAL_GetID(void);
void TemSensor_Test(void);

#endif //_HAL_TEMPERATURE_SENSOR_H_
