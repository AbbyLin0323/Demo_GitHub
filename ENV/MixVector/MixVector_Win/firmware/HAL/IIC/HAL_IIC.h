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
Filename    :HAL_IIC.h
Version     :
Author      :Kristin Wang
Date        :2014.11
Description :header file for IIC driver.
Others      :
Modify      :
20141103    Kristin    Create
*******************************************************************************/
#ifndef _HAL_IIC_H_
#define _HAL_IIC_H_

#include "BaseDef.h"

#define REG_BASE_IIC    0xFFE0D000

#define rIIC_SCLLOW_CNTL     (*(volatile U16 *)(REG_BASE_IIC + 0x0))
#define SCLLOW_CNTL_RCMD     0x1a

#define rIIC_SCLHIGH_CNTL    (*(volatile U16 *)(REG_BASE_IIC + 0x2))
#define SCLHIGH_CNTL_RCMD    0x14

#define rIIC_SUSTA_CNTL      (*(volatile U16 *)(REG_BASE_IIC + 0x4))
#define SUSTA_CNTL_RCMD      0x14

#define rIIC_SUSTO_CNTL      (*(volatile U16 *)(REG_BASE_IIC + 0x6))
#define SUSTO_CNTL_RCMD      0x14

#define rIIC_BUF_CNTL        (*(volatile U16 *)(REG_BASE_IIC + 0x8))
#define BUF_CNTL_RCMD        0x20

#define rIIC_SDAO_CNTL       (*(volatile U16 *)(REG_BASE_IIC + 0xa))
#define SDAO_CNTL_RCMD       0x3

#define rIIC_HDSTA_CNTL      (*(volatile U16 *)(REG_BASE_IIC + 0xc))
#define HDSTA_CNTL_RCMD      0x14

#define rIIC_ID_MSK          (*(volatile U16 *)(REG_BASE_IIC + 0xe))
#define IIC_ID_7BIT          0
#define IIC_ID_10BIT         1
typedef struct _IIC_ID_MSK_REG
{
    U16 bsDeviceID  :10;//IIC accessing device ID
    U16 bsID10BitEn :1; //0:Device ID is 7-bit, 1:Device ID is 10-bit
    U16 bsWIntMsk   :1; //0:mask write interrupt
    U16 bsRIntMsk   :1; //0:mask read interrupt
    U16 bsErrIntMsk :1; //0:mask read interrupt
    U16 bsRsv       :2;
}IIC_ID_MSK_REG;

#define rIIC_MODE            (*(volatile U8 *)(REG_BASE_IIC + 0x10))
typedef struct _IIC_MODE_REG
{
    U8 bsFilterEn  :1;
    U8 bsSpikeCNTL :3;
    U8 bsMsbMode   :1; //bit 4, 0:MSB last, 1:MSB first
    U8 bsReadMode  :1; //bit 5, 0:read one time,pop 4-byte, 1:read one time,pop 1-byte
    U8 bsRsv       :2;
}IIC_MODE_REG;

#define rIIC_INT             (*(volatile U8 *)(REG_BASE_IIC + 0x14))
#define IIC_INT_WFINISH_BIT  (0)
#define IIC_INT_RFINISH_BIT  (1)
#define IIC_INT_ERR_BIT      (2)
typedef struct _IIC_INT_REG
{
    U8 bsWFinishInt :1;
    U8 bsRFinishInt :1;
    U8 bsErrInt     :1;
    U8 bsRsv        :5;
}IIC_INT_REG;

#define rIIC_FIFO_DEPTH      (*(volatile U8 *)(REG_BASE_IIC + 0x15))
typedef struct _IIC_DEPTH_REG
{
    U8 bsRxDepth :3;
    U8 bsTxDepth :3;
    U8 bsRsv     :2;
}IIC_DEPTH_REG;

#define rIIC_CMD             (*(volatile U32 *)(REG_BASE_IIC + 0x20))
#define IIC_TRIGGER_WRITE    0
#define IIC_TRIGGER_READ     1
typedef struct _IIC_CMD_REG
{
    U8 bsLen :3; //b000:1-byte,b111:8-byte
    U8 bsRsv :4;
    U8 bsRW  :1; //0:write, 1:read
    U8 aRsv[3];
}IIC_CMD_REG;

#define IIC_DATA_REG_SIZE     4
#define rIIC_TX_DATA(_n_)    (*(volatile U8 *)(REG_BASE_IIC + 0x24 + (_n_)))
#define rIIC_RX_DATA         (*(volatile U32 *)(REG_BASE_IIC + 0x28))

void HAL_IICInit(void);
void HAL_IICSetDeviceID(BOOL bID10Bit, U16 usDeviceID);
BOOL HAL_IICWriteData(U8 ucLenByte, U8 *pTxData);
BOOL HAL_IICReadData(U8 ucLenByte, U8 *pRxData);

#endif //_HAL_IIC_H_
