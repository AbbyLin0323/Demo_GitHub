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
Filename    :HAL_SpiFlashFormat.h
Version     :Ver 1.0
Author      :Victor Zhang
Date        :2014.11.13    19:55
Description :
    
Others      :
Modify      :
*******************************************************************************/

#ifndef _HAL_SPI_FLASH_FORMAT_H_
#define _HAL_SPI_FLASH_FORMAT_H_
#include "BaseDef.h"

#define SPI_FLASH_FILE_HEAD_LEN (8) 
typedef union SPI_FLASH_FILE_HEAD
{
    U32 ulDW[SPI_FLASH_FILE_HEAD_LEN];
    struct{
        U32 ulSignature;
        U32 ulSegOffset;
        U32 ulSegLength;
        U32 ulTargetAddr;
        U32 ulExecEntry;
    };
}SPI_FLASH_FILE_HEAD;

#define SPI_FLASH_FILE_HEAD_ADDR    (SPI_START_ADDRESS)

#endif
