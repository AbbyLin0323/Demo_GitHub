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
Filename    :HAL_TemperatureSensor.c
Version     :
Author      :William Wang
Date        :2015.12
Description :this file encapsulate Temperature Sensor driver interface          
Others      :
Modify      :
20151228    William    create
*******************************************************************************/
#include "HAL_IIC.h"
#include "COM_Memory.h"
#include "HAL_TemperatureSensor.h"
#include "HAL_ParamTable.h"
#include "HAL_Xtensa.h"

GLOBAL MCU12_VAR_ATTR IIC_SENSOR_REGISTER g_IICSensorReg;
GLOBAL MCU12_VAR_ATTR IIC_SENSOR_PARAM g_IICSensorParam;
extern TEMPERATURE_SENSOR_I2C * HAL_GetTemperatureSensorI2C(void);
/*----------------------------------------------------------------------------
Name: HAL_TemperatureSensor_Init
Description: 
    initialize the temperature sensor driver
Input Param: 
    none
Output Param: 
    none
Return Value:
    BOOL : TRUE - initialize the temperature sensor driver successfully
           FALSE - error occur,initialize the temperature sensor driver fail
Usage:
    before write or read temperature sensor register  
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL HAL_TemperatureSensor_Init(void)
{
    TEMPERATURE_SENSOR_I2C *pTempSensor;
    U32 ulMCUID;
    static BOOL bInit = FALSE;

    ulMCUID = HAL_GetMcuId();
    if (((MCU1_ID == ulMCUID) || (MCU0_ID == ulMCUID)) && (bInit == FALSE)) 
    {    
        pTempSensor = HAL_GetTemperatureSensorI2C();
        g_IICSensorParam.ucDeviceType = (pTempSensor->bsTemperatureSensorType) & 0x07;  
        if ( g_IICSensorParam.ucDeviceType & 0x07) {
            g_IICSensorParam.ucDevAddr = pTempSensor->bsI2CAddr;
            g_IICSensorParam.ucDevClock = pTempSensor->bsI2CClock;      
            if ((g_IICSensorParam.ucDevClock >= 3) || ((g_IICSensorParam.ucDevAddr != 0x48) && (g_IICSensorParam.ucDevAddr != 0x4A) && (g_IICSensorParam.ucDevAddr != 0x49))) 
            {
                DBG_Printf("HAL_TemperatureSensor_Init: sensor parameter is not correct! \n");
                DBG_Getch();
            } else {
                bInit = TRUE;
                DBG_Printf("HAL_TemperatureSensor_Init: Init done, ulMCUID %d! \n", ulMCUID);
            }
        } else {
            DBG_Printf("Temperature sensor parameter is not I2C 0x%x! \n", g_IICSensorParam.ucDeviceType);
        }
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
Name: ThermalSensor_SetPointer
Description: 
    set temperature sensor Pointer Register
Input Param: 
    U8 RegisterSelect: Select the register to communication: 0x00 ~ 0x03
Output Param: 
    none
Return Value:
    BOOL : TRUE - set pointer register successfully
           FALSE - error occur,set pointer register fail
Usage:
    before write or read temperature sensor register  
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL ThermalSensor_SetPointer(U8 RegisterSelect)
{
    U8 ucRegisterCounter = 0;

    if (g_IICSensorParam.ucDeviceType == G752KC2G)
    {
        ucRegisterCounter = 4;
    } 
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
    {
        ucRegisterCounter = 6;
    } 
    else if (g_IICSensorParam.ucDeviceType == S_5851AAA_I6T1U)
    {
        ucRegisterCounter = 2;
    }
    if (g_IICSensorParam.ucDeviceType == TI_TMP102)
    {
        ucRegisterCounter = 4;
    }
    
    if (RegisterSelect >= ucRegisterCounter)
    {
        DBG_Printf("ThermalSensor_SetPointer %d fail: out of register range  %d! \n", RegisterSelect, ucRegisterCounter);
        return FALSE;
    }

    HAL_IICInit();
    HAL_IICSetDeviceID(FALSE, (U16)g_IICSensorParam.ucDevAddr);
/*  
   if(HAL_IICWriteData(1, &RegisterSelect) == FALSE)
    {
        DBG_Printf("ThermalSensor_SetPointer %d fail! \n", RegisterSelect);
        return FALSE;
    }
*/
    return TRUE;
}


/*----------------------------------------------------------------------------
Name: HAL_GetTemperature
Description: 
    get temperature register value
Input Param: 
    none
Output Param: 
    none
Return Value:
    S8 : temperature register value main part
Usage:
    get device temperature  
History:
20151228    William    create
------------------------------------------------------------------------------*/
U8  HAL_GetTemperature(void)
{
    U8 ucTemp[8];
    U8 ucTempResult;
    BOOL bReadFail = FALSE;

    ThermalSensor_SetPointer(TM_POINTER);

    if (HAL_IICReadData(2, &ucTemp[0])) 
    {
        //DBG_Printf("Device temperature: %x %x \n", ucTemp[0], ucTemp[1]);
        g_IICSensorReg.usTempReg = (ucTemp[0] << 8) | (ucTemp[1]);
    } 
    else 
    {
        DBG_Printf("Read device temperature fail! \n");
        bReadFail = TRUE;
    }

    if (bReadFail) 
    {
        ucTempResult = 38;
    }
    else if (ucTemp[0] & BIT(7)) 
    {
        ucTempResult = 1;//Less than 0, report the temperature is 1 degree
    } 
    else 
    {
        if ((g_IICSensorParam.ucDeviceType == G752KC2G) || (g_IICSensorParam.ucDeviceType == S_5851AAA_I6T1U)) 
        {
            ucTempResult = ucTemp[0];
        } 
        else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
        {
            ucTempResult = (ucTemp[0] << 1) | (ucTemp[1] >> 7);
        } 
        else if (g_IICSensorParam.ucDeviceType == TI_TMP102)
        {
            ucTempResult = (((ucTemp[0] << 8) | ucTemp[1]) >> 4) / 4 / 4;  // return celsius degree
        }
    }
    return (ucTempResult);
}

/*----------------------------------------------------------------------------
Name: HAL_GetConfig
Description: 
    get configuration register value
Input Param: 
    none
Output Param: 
    none
Return Value:
    U8 : configuration register value
Usage:
    get configuration of temperature sensor  
History:
20151228    William    create
------------------------------------------------------------------------------*/
U16  HAL_GetConfig(void)
{
    U8 ucTemp[8];

    ThermalSensor_SetPointer(CONFIG_POINTER);

    if (HAL_IICReadData(2, &ucTemp[0]))
    {
        DBG_Printf("Device configuration: %x  \n", ucTemp[0]);
        if (g_IICSensorParam.ucDeviceType == TI_TMP102)
        {
            g_IICSensorReg.ucConfigReg.ucValue = ucTemp[0] + (ucTemp[1] << 8);
        }
        else
        {
            g_IICSensorReg.ucConfigReg.ucValue = ucTemp[0];
        }

        /*if(g_IICSensorParam.ucDeviceType == G752KC2G)
        {
            ucFaultQueue = ucTemp[0] >> FAULTQUEUE_BIT;
            if(ucFaultQueue == 0)
            {
                ucFaultQueue = 1;
            }
            else
            {
                ucFaultQueue *= 2;
            }
            bOSPolarity = (ucTemp[0] >> OSPOLARITY_BIT) & BIT(0);
            bCmtInt = (ucTemp[0] >> CMPINT_BIT) & BIT(0);
            bShutDown = (ucTemp[0]) & BIT(0);
        }
        else if(g_IICSensorParam.ucDeviceType == LM73CIMK_0)
        {
            bShutDown = (ucTemp[0] >> SHUTDOWN_BIT_LM73) & BIT(0);
            bAlrtEn = (ucTemp[0] >> ALRT_EN) & BIT(0);
            bOSPolarity = (ucTemp[0] >> ALRT_POL) & BIT(0);
            bAlrtRst = (ucTemp[0] >> ALRT_RST) & BIT(0);
            bOneShot = (ucTemp[0] >> OS_BIT_LM73) & BIT(0);
        }
        else if(g_IICSensorParam.ucDeviceType == S_5851AAA_I6T1U)
        {
            bOneShot = (ucTemp[0] >> OS_BIT_S5851A);
            bShutDown = (ucTemp[0]) & BIT(0);
        }*/
    } 
    else 
    {
        DBG_Printf("Get device configuration fail! \n");
    }

    return g_IICSensorReg.ucConfigReg.ucValue ;
}

/*----------------------------------------------------------------------------
Name: HAL_SetConfig
Description: 
    set G752 configuration register value
Input Param: 
    U8 FaultQueue: Number of faults necessary to detect before setting O.S. 
                   output to avoid false tripping due to noise
    BOOL OSPolarity: 0 is active low, 1 is active high.
    BOOL CmtInt: 0 is Comparator mode, 1 is Interrupt mode
    BOOL ShutDown: When set to 1 the G752 goes to low power shutdown mode
Output Param: 
    none
Return Value:
    BOOL : TRUE - set configuration register successfully
           FALSE - error occur,set configuration register fail
Usage:
    configure the temperature sensor working mode and parameter 
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL  HAL_SetConfig(U8 FaultQueue, BOOL OSPolarity, BOOL CmtInt, BOOL ShutDown)
{
    U8 ucTemp[8];

    if ((FaultQueue != 1) && (FaultQueue != 2) && (FaultQueue != 4) && (FaultQueue != 6)) 
    {
        DBG_Printf("Set device configuration fail: FaultQueue is out of range! \n");
        return FALSE;
    }

    ThermalSensor_SetPointer(CONFIG_POINTER);
    if (FaultQueue == 1) 
    {
        FaultQueue = 0;
    } 
    else 
    {
        FaultQueue >>= 1;
    }

    ucTemp[0] = (U8)((FaultQueue << FAULTQUEUE_BIT) | (OSPolarity << OSPOLARITY_BIT)
               | (CmtInt << CMPINT_BIT) | ShutDown);


    if (HAL_IICWriteData(1, &ucTemp[0])) 
    {
        return TRUE;
    }
    else
    {
        DBG_Printf("Set device configuration fail! \n");
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: HAL_SetConfig_LM73
Description: 
    set configuration register value
Input Param: 
    BOOL ShutDown: When set to 1 the LM73 goes to low power shutdown mode 
    BOOL AlertEn: Enable the alert output
    BOOL AlertPol: 0 is active low, 1 is active high.
    BOOL AlertRst: Reset the alert pin and alert status
    BOOL OneShot: When set to 1, trigger the temperature conversion in shut down mode
Output Param: 
    none
Return Value:
    BOOL : TRUE - set configuration register successfully
           FALSE - error occur,set configuration register fail
Usage:
    configure the temperature sensor working mode and parameter 
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL  HAL_SetConfig_LM73(BOOL ShutDown, BOOL AlertEn, BOOL AlertPol, BOOL AlertRst, BOOL OneShot)
{
    U8 ucTemp[8];

    ThermalSensor_SetPointer(CONFIG_POINTER);
    
    ucTemp[0] = (U8)((ShutDown << SHUTDOWN_BIT_LM73) | (AlertEn << ALRT_EN) | (AlertPol << ALRT_POL)
               | (AlertRst << ALRT_RST) | OneShot | 0x40);


    if (HAL_IICWriteData(1, &ucTemp[0]))
    {
        return TRUE;
    } 
    else 
    {
        DBG_Printf("Set device configuration fail! \n");
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: HAL_SetConfig_TMP102
Description: 
    set configuration register value
Input Param: 
Output Param: 
    none
Return Value:
    BOOL : TRUE - set configuration register successfully
           FALSE - error occur,set configuration register fail
Usage:
    configure the temperature sensor working mode and parameter 
History:

------------------------------------------------------------------------------*/
BOOL  HAL_SetConfig_TMP102(U8 byte1, U8 byte2)
{
    U8 ucTemp[8];
    //DBG_Printf("Set device configuration start! \n");

    ThermalSensor_SetPointer(CONFIG_POINTER);
    
    ucTemp[0] = byte1;
    ucTemp[1] = byte2;

    if (HAL_IICWriteData(2, &ucTemp[0]))
    {
        return TRUE;
    } 
    else
    {
        DBG_Printf("Set device configuration fail! \n");
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: HAL_SetConfig_LM73
Description: 
    set configuration register value
Input Param: 
    BOOL OneShot: When set to 1, trigger the temperature conversion in shut down mode 
    BOOL ShutDown: When set to 1 the S5851A goes to low power shutdown mode
Output Param: 
    none
Return Value:
    BOOL : TRUE - set configuration register successfully
           FALSE - error occur,set configuration register fail
Usage:
    configure the temperature sensor working mode and parameter 
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL  HAL_SetConfig_S5851A(BOOL OneShot, BOOL ShutDown)
{
    U8 ucTemp[8];
    //DBG_Printf("Set device configuration start! \n");

    ThermalSensor_SetPointer(CONFIG_POINTER);
    
    ucTemp[0] = (U8)((OneShot << OS_BIT_S5851A) | ShutDown);


    if (HAL_IICWriteData(1, &ucTemp[0])) 
    {
        return TRUE;
    }
    else 
    {
        DBG_Printf("Set device configuration fail! \n");
        return FALSE;
    }
}


/*----------------------------------------------------------------------------
Name: HAL_GetTHYST_TLow
Description: 
    get THYST register value
Input Param: 
    none
Output Param: 
    none
Return Value:
    U8 : THYST register value main part
Usage:
    get THYST Trip Temperature Data  
History:
20151228    William    create
------------------------------------------------------------------------------*/
U8  HAL_GetTHYST_TLow(void)
{
    S8 ucTemp[8];
    U8 ucTempResult;

    //DBG_Printf("Get device THYST start! \n");
    if (g_IICSensorParam.ucDeviceType == G752KC2G) 
    {
        ThermalSensor_SetPointer(THYST_SET_POINTER);
    } 
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0)
    {
        ThermalSensor_SetPointer(TLOW_SET_POINTER);
    } 
    else if (g_IICSensorParam.ucDeviceType == TI_TMP102) 
    {
        ThermalSensor_SetPointer(TMP102_TLOW_SET_POINTER);
    }

    //DBG_Printf("Read device THYST IIC read data start! \n");
    if (HAL_IICReadData(2, &ucTemp[0])) 
    {
        DBG_Printf("Device THYST: %x %x \n", ucTemp[0], ucTemp[1]);
        //sTHYST = (ucTemp[0] << 8) | (ucTemp[1]);
        if(g_IICSensorParam.ucDeviceType == G752KC2G) 
        {
            g_IICSensorReg.usTHYSTReg = (ucTemp[0] << 8) | (ucTemp[1]);
        }
        else if(g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
        {
            g_IICSensorReg.usTLowReg = (ucTemp[0] << 8) | (ucTemp[1]);
        } 
        else if(g_IICSensorParam.ucDeviceType == TI_TMP102)
        {
            g_IICSensorReg.usTHighReg = (ucTemp[0] << 8) | (ucTemp[1]);
        }
    } 
    else
    {
        DBG_Printf("Read THYST fail! \n");
    }
    
    if (ucTemp[0] & BIT(7))
    {
        ucTempResult = 1;//Less than 0, report the temperature is 1 degree
    } 
    else 
    {
        if (g_IICSensorParam.ucDeviceType == G752KC2G)
        {
            ucTempResult = ucTemp[0];
        } 
        else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
        {
            ucTempResult = (ucTemp[0] << 1) | (ucTemp[1] >> 7);
        } 
        else if (g_IICSensorParam.ucDeviceType == TI_TMP102)
        {
            ucTempResult = (((ucTemp[0] << 8) | (ucTemp[1] )) >> 4) /4 /4; //celsius degree
        }
    }

    return (ucTempResult);
}

/*----------------------------------------------------------------------------
Name: HAL_SetTHYST_TLow
Description: 
    set THYST register value
Input Param: 
    U8 THYST: THYST Trip Temperature Data
Output Param: 
    none
Return Value:
    BOOL : TRUE - set THYST register successfully
           FALSE - error occur,set THYST register fail
Usage:
    set THYST Trip Temperature Data  
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL  HAL_SetTHYST_TLow(U16 THYST)
{
    S8 ucTemp[8];
    //DBG_Printf("Set device THYST start! \n");

    if (g_IICSensorParam.ucDeviceType == G752KC2G) 
    {
        ThermalSensor_SetPointer(THYST_SET_POINTER);
    }
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0)
    {
        ThermalSensor_SetPointer(TLOW_SET_POINTER);
    }
    else if (g_IICSensorParam.ucDeviceType == TI_TMP102)
    {
        ThermalSensor_SetPointer(TMP102_TLOW_SET_POINTER);
    }

    //DBG_Printf("Set device THYST IIC write data start! \n");
    if (g_IICSensorParam.ucDeviceType == G752KC2G)
    {
        ucTemp[0] = THYST & 0xff;
        ucTemp[1] = 0;
    }
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
    {
        ucTemp[0] = (THYST & 0xff) >> 1;
        ucTemp[1] = ((THYST & 0xff) & BIT(0)) << 7;
    }
    else if (g_IICSensorParam.ucDeviceType == TI_TMP102) 
    {
        ucTemp[0] = ((THYST << 4) & 0xff00) >> 8;
        ucTemp[1] = ((THYST << 4) & 0xff);
    }

    if (HAL_IICWriteData(2, &ucTemp[0]))
    {
        return TRUE;
    }
    else
    {
        DBG_Printf("Set THYST fail! \n");
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: HAL_GetTOS_THigh
Description: 
    get TOS register value
Input Param: 
    none
Output Param: 
    none
Return Value:
    U8 : TOS register value main part
Usage:
    get TOS Trip Temperature Data  
History:
20151228    William    create
------------------------------------------------------------------------------*/
U16  HAL_GetTOS_THigh(void)
{
    S8 ucTemp[8];
    U16 ucTempResult;
    //DBG_Printf("Get device TOS start! \n");

    if (g_IICSensorParam.ucDeviceType == G752KC2G)
    {
        ThermalSensor_SetPointer(TOS_SET_POINTER);
    }
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
    {
        ThermalSensor_SetPointer(THIGH_SET_POINTER);
    }
    else if (g_IICSensorParam.ucDeviceType == TI_TMP102)
    {
        ThermalSensor_SetPointer(TMP102_THIGH_SET_POINTER);
    }

    //DBG_Printf("Read device TOS IIC read data start! \n");
    if (HAL_IICReadData(2, &ucTemp[0]))
    {
        DBG_Printf("Device TOS: %x %x \n", ucTemp[0], ucTemp[1]);
        if (g_IICSensorParam.ucDeviceType == G752KC2G)
        {
            g_IICSensorReg.usTOSReg = (ucTemp[0] << 8) | (ucTemp[1]);
        }
        else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
        {
            g_IICSensorReg.usTHighReg = (ucTemp[0] << 8) | (ucTemp[1]);
        }
        else if (g_IICSensorParam.ucDeviceType == TI_TMP102) 
        {
            g_IICSensorReg.usTHighReg = (ucTemp[0] << 8) | (ucTemp[1]);
        }
    }
    else 
    {
        DBG_Printf("Read TOS fail! \n");
    }

    if (ucTemp[0] & BIT(7))
    {
        ucTempResult = 1;//Less than 0, report the temperature is 1 degree
    }
    else 
    {
        if (g_IICSensorParam.ucDeviceType == G752KC2G)
        {
            ucTempResult = ucTemp[0];
        }
        else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0)
        {
            ucTempResult = (ucTemp[0] << 1) | (ucTemp[1] >> 7);
        }
        else if (g_IICSensorParam.ucDeviceType == TI_TMP102)
        {
            ucTempResult = (((ucTemp[0] << 8) | (ucTemp[1])) >> 4) / 4 / 4; //celsius degree
        }
    }
    return (ucTempResult);
}

/*----------------------------------------------------------------------------
Name: HAL_SetTOS_THigh
Description: 
    set TOS register value
Input Param: 
    U8 TOS: TOS Trip Temperature Data: Bit15 ~ Bit7 are valid
Output Param: 
    none
Return Value:
    BOOL : TRUE - set TOS register successfully
           FALSE - error occur,set TOS register fail
Usage:
    set TOS Trip Temperature Data  
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL  HAL_SetTOS_THigh(U16 TOS)
{
    S8 ucTemp[8];
    //DBG_Printf("Set device TOS start! \n");

    if (g_IICSensorParam.ucDeviceType == G752KC2G) 
    {
        ThermalSensor_SetPointer(TOS_SET_POINTER);
    }
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0)
    {
        ThermalSensor_SetPointer(THIGH_SET_POINTER);
    }
    else if (g_IICSensorParam.ucDeviceType == TI_TMP102) 
    {
        ThermalSensor_SetPointer(TMP102_THIGH_SET_POINTER);
    }
    //DBG_Printf("Set device TOS IIC write data start! \n");
    if (g_IICSensorParam.ucDeviceType == G752KC2G)
    {
        ucTemp[0] = TOS & 0xff;
        ucTemp[1] = 0;
    }
    else if (g_IICSensorParam.ucDeviceType == LM73CIMK_0) 
    {
        ucTemp[0] = (TOS & 0xff) >> 1;
        ucTemp[1] = ((TOS & 0xff) & BIT(0)) << 7;
    }
    else if (g_IICSensorParam.ucDeviceType == TI_TMP102) 
    {
        ucTemp[0] = ((TOS << 4) & 0xff00) >> 8;
        ucTemp[1] = ((TOS << 4) & 0xff);
    }
    if (HAL_IICWriteData(2, &ucTemp[0])) 
    {
        return TRUE;
    }
    else
    {
        DBG_Printf("Set TOS fail! \n");
        return FALSE;
    }
}


/*----------------------------------------------------------------------------
Name: HAL_GetCtlStats
Description: 
    get control status register value
Input Param: 
    none
Output Param: 
    none
Return Value:
    U8 : control status register value
Usage:
    get control status of temperature sensor  
History:
20151228    William    create
------------------------------------------------------------------------------*/
U8  HAL_GetCtlStats(void)
{
    U8 ucTemp[8];
    //DBG_Printf("Get device control status start! \n");

    ThermalSensor_SetPointer(CTLSTATS_SET_POINTER);

    if (HAL_IICReadData(1, &ucTemp[0])) 
    {
        DBG_Printf("Device control status: %x  \n", ucTemp[0]);

        if (g_IICSensorParam.ucDeviceType == LM73CIMK_0)
        {
            /*bTimeOutDisable = (ucTemp[0] >> TIMEOUT_DISABLE_BIT) & BIT(0);
            ucTemResolution = (ucTemp[0] >> TEMP_RESOLUTION_BIT) & (BIT(1) | BIT(0));
            bAlrtStats = (ucTemp[0] >> ALERT_STATS_BIT) & BIT(0);
            bTempHigh = (ucTemp[0] >> TEMP_HIGH_BIT) & BIT(0);
            bTempLow = (ucTemp[0] >> TEMP_LOW_BIT) & BIT(0);
            bDataAvailable = (ucTemp[0]) & BIT(0);*/
            g_IICSensorReg.ucCtlStatsReg.ucValue = ucTemp[0];
        }
    } 
    else  
    {
        DBG_Printf("Get device control status fail! \n");
    }
    return ucTemp[0];
}

/*----------------------------------------------------------------------------
Name: HAL_SetCtlStats
Description: 
    set control status register value
Input Param: 
    BOOL TimeOutDisable: Disable the time-out feature on the SMBDAT and SMBCLK lines 
    U8 TempResolution: 0: 11 bit  1: 12 bit  2: 13 bit  3: 14 bit
Output Param: 
    none
Return Value:
    BOOL : TRUE - set control status register successfully
           FALSE - error occur,set control status register fail
Usage:
    control the temperature sensor working mode 
History:
20151228    William    create
------------------------------------------------------------------------------*/
BOOL  HAL_SetCtlStats(BOOL TimeOutDisable, U8 TempResolution)
{
    U8 ucTemp[8];
    //DBG_Printf("Set device control status start! \n");

    if (TempResolution >= 4)
    {
        DBG_Printf("Set device control status fail: TempResolution is out of range! \n");
        return FALSE;
    }
    ThermalSensor_SetPointer(CTLSTATS_SET_POINTER);
    
    ucTemp[0] = (U8)((TimeOutDisable << TIMEOUT_DISABLE_BIT)
                | (TempResolution << TEMP_RESOLUTION_BIT));


    if (HAL_IICWriteData(1, &ucTemp[0]))
    {
        return TRUE;
    } 
    else 
    {
        DBG_Printf("Set device control status fail! \n");
        return FALSE;
    }
}

/*----------------------------------------------------------------------------
Name: HAL_GetID
Description: 
    get temperature sensor ID
Input Param: 
    none
Output Param: 
    none
Return Value:
    U16 : temperature sensor ID
Usage:
    get temperature sensor ID  
History:
20151228    William    create
------------------------------------------------------------------------------*/
U16  HAL_GetID(void)
{
    U8 ucTemp[8];
    U16 ucTempResult;

    ThermalSensor_SetPointer(ID_SET_POINTER);
    
    if (HAL_IICReadData(2, &ucTemp[0]))
    {
        //DBG_Printf("Device ID: %x %x \n", ucTemp[0], ucTemp[1]);
        g_IICSensorReg.usIDReg = (ucTemp[0] << 8) | (ucTemp[1]);
    } 
    else 
    {
        DBG_Printf("Read device ID fail! \n");
    }

    ucTempResult = (ucTemp[0] << 8) | ucTemp[1];
    return (ucTempResult);
}

/*----------------------------------------------------------------------------
Name: TemSensor_Test
Description: 
    Do temperature sensor test
Input Param: 
    none
Output Param: 
    none
Return Value:
    none
Usage:
    Test G752 Temperature sensor  
History:
20151228    William    create
------------------------------------------------------------------------------*/
void TemSensor_Test(void)
{
    S8  sTemp;
    //U8  ucTemp;

    sTemp = HAL_GetTemperature();
    DBG_Printf("Device temperature: %d \n", sTemp);

    /*ucTemp = HAL_GetConfig();
    DBG_Printf("Device configuration: 0x%x FaultQueue %d OSPolarity %d CmtInt %d ShutDown %d\n",
               ucTemp, g_IICSensorReg.ucConfigReg.bFaultQueue, g_IICSensorReg.ucConfigReg.bOSPolarity,
               g_IICSensorReg.ucConfigReg.bCmtInt, g_IICSensorReg.ucConfigReg.bShutDown);

    sTemp = HAL_GetTHYST_TLow();
    DBG_Printf("Device THYST: %d \n", sTemp);

    sTemp = HAL_GetTOS_THigh();
    DBG_Printf("Device TOS: %d \n", sTemp);*/
}
/********************** FILE END ***************/
