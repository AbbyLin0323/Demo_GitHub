/*******************************************************************************
*Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.                *
                                                                                *
*his PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.             *
*and may contain trade secrets and/or other confidential information of VIA     *
*Technologies,Inc.                                                              *
*This file shall not be disclosed to any third party, in whole or in part,      *
*without prior written consent of VIA.                                          *
                                                                                *
*THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,    *
*WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,   *
*AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF      *
*MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR          *
*NON-INFRINGEMENT.                                                              *
********************************************************************************
Filename:HAL_TSC.c
Version:
Author:
Date:2016.5
Description :this file encapsulate TSC driver interface
Others:
Modify:
*******************************************************************************/
#ifndef _HAL_TSC_H_
#define _HAL_TSC_H_

#include "BaseDef.h"

//#define TSC_REG_BASE  (0xFFE0D800)    //Should Move to HAL_MemoryMap.h

#define TSC_MODE_DEFAULT        (0x00)
#define TSC_MODE_POWER_ON       (0x01)
#define TSC_MODE_CONTINUOUS     (0x02)
#define TSC_MODE_CHECK          (0x03)

#define TSC_MES_TIME_1MS        (0x00)
#define TSC_MES_TIME_10MS       (0x01)
#define TSC_MES_TIME_100MS      (0x02)
#define TSC_MES_TIME_1S         (0x03)

#define R_RST_TSC               (1<<25)
#define TSC_SENSOR              0x08
#define TSC_ENABLE_UART         0x10
/* REG: Temperature sensor control and status register.
*Bit[17:16]--TSC_Time.
*00 - 1ms measure
*01 - 10ms measure
*10 - 100ms measure
*11 - 1s measure
*Bit[9:8] -- TSC_MODE.
*01 - power on TSC
*10 - continuous measure mode
*11 - MCU check mode
*Bit[7]-- TSC_MODE_EN.
*1 - mcu can config TSC mode
*/
typedef struct _TSCREG
{
    U32 bsTSCEn           : 1;    //TSC_EN.
    U32 bsRsv             : 6;
    U32 bsTSCModeEn       : 1;    //TSC_MODE_EN.R0.1:indicated mcu can config TSC mode.
    U32 bsTSCMode         : 2;    //TSC_MODE.
    U32 bsTSCModeValid    : 1;    //TSC_mode_valid.
    U32 bsTSCTrim         : 4;    //TSC_trim.
    U32 bsTSCTrimValid    : 1;    //TSC_trim_valid.
    U32 bsTSCTime         : 2;    //TSC_time.
    U32 bsTSCTimeValid    : 1;    //TSC_time_valid.
    U32 bsTSCValid        : 1;    //TSC_Valid. RO.
    U32 bsTSCValue        : 12;   //TSC_Value. RO.Return value.
}TSCREG, *PTSCREG;

typedef struct _TSC_SENSOR_PARAM
{
    U8 ucDeviceType;    //TSC device type
    U8 ucTscClkEn;      //TSC clock enable
    U8 ucTscTime;       //TSC measure time
    U8 ucTsc2Uart;      //TSC 2 UART report time
    U8 ucInitSts;
    U8 ucDelta ;        //For smart cmd delta value	
    U8 ucLoThreshold ;  //low threshold
    U8 ucHiThreshold ;  //high threshold
}TSC_SENSOR_PARAM;

BOOL MCU12_DRAM_TEXT HAL_TSCModeSet(U32 ulModeType, BOOL bWaiteChk);
void MCU12_DRAM_TEXT HAL_TSCMeaTimeSet(U32 ulMeasureTime);
void MCU12_DRAM_TEXT HAL_TSCTrimSet(U32 ulTrimValue);
BOOL MCU12_DRAM_TEXT HAL_TSCInit(void);
void MCU12_DRAM_TEXT HAL_TSCDisable(void);
S32 MCU12_DRAM_TEXT HAL_TSCTempValueGet(BOOL bWait);
#endif
