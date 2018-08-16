/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_SpiFlashFormat.h
Version     :Ver 1.0
Author      :Victor Zhang
Date        :2014.11.13    19:55
Description :
    
Others      :
Modify      :
****************************************************************************/

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
