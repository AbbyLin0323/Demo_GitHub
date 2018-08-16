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
Filename    : HAL_GPIO.h
Version     : Ver 1.0
Author      : tobey
Date        : 2013.10.28
Description : this file declare GPIO driver interface. 
Others      : 
Modify      :
20141028    Tobey     create file.
*******************************************************************************/

#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define GPIO_REG_BASE (APB_BASE + 0x1000)//0xffe08000
#define GPIO_PIN_MAX (32)
#define rGPIOPD  (*((volatile U32*) (GPIO_REG_BASE + 0x0))) //GPDR    8'h00
#define rGPIOPS  (*((volatile U32*) (GPIO_REG_BASE + 0x4))) //GPSR     8'h04
#define rGPIOPC  (*((volatile U32*) (GPIO_REG_BASE + 0x8))) //GPCR     8'h08
#define rGPIORE  (*((volatile U32*) (GPIO_REG_BASE + 0xc))) //GRER     8'h0c
#define rGPIOFE  (*((volatile U32*) (GPIO_REG_BASE + 0x10))) //GFER     8'h10
#define rGPIOED  (*((volatile U32*) (GPIO_REG_BASE + 0x14))) //GEDR    8'h14
#define rGPIOPL  (*((volatile U32*) (GPIO_REG_BASE + 0x18))) //GPLR     8'h18
#define rGPIOIS  (*((volatile U32*) (GPIO_REG_BASE + 0x1c))) //GISR      8'h1c
#define rGPIOPUP (*((volatile U32*) (GPIO_REG_BASE + 0x20))) //GPUPR   8'h20
#define rGPIOPDN (*((volatile U32*) (GPIO_REG_BASE + 0x24))) //GPDNR   8'h24
#define rGPIOIM  (*((volatile U32*) (GPIO_REG_BASE + 0x28))) //GIMR      8'h28
#define rGPIOLIS (*((volatile U32*) (GPIO_REG_BASE + 0x2c))) //GLISR     8'h2c
#define rGPIOLD  (*((volatile U32*) (GPIO_REG_BASE + 0x30))) //GLDR     8'h30

BOOL HAL_IsGPIOEdgeOccur(U8 ucPinNum);
void HAL_GPIOClearEdgeInt(U8 ucPinNum);
BOOL HAL_IsGPIOLevelIntOccur(U8 ucPinNum);
void HAL_GPIOClearLevelInt(U8 ucPinNum);
void HAL_GPIORasingEdgeIntInit(U8 ucPinNum);
void HAL_GPIOFallingEdgeIntInit(U8 ucPinNum);
void HAL_GPIOHighLevelIntInit(U8 ucPinNum);
void HAL_GPIOLowLevelIntInit(U8 ucPinNum);

#endif
