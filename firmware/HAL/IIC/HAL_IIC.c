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
Filename    :HAL_IIC.c
Version     :
Author      :Kristin Wang
Date        :2014.11
Description :this file encapsulate IIC driver interface          
Others      :
Modify      :
20141104    Kristin    create
*******************************************************************************/
#include "HAL_IIC.h"
#include "COM_Memory.h"

/*----------------------------------------------------------------------------
Name: HAL_IICInit
Description: 
    necessary initialize for IIC
Input Param: 
    none
Output Param: 
    none
Return Value:
    void
Usage:
    HW initialize  
History:
20141104    Kristin    create
------------------------------------------------------------------------------*/
void HAL_IICInit(void)
{
    volatile IIC_MODE_REG *pModeReg;

    rIIC_SCLLOW_CNTL = SCLLOW_CNTL_RCMD;
    rIIC_SCLHIGH_CNTL = SCLHIGH_CNTL_RCMD;
    rIIC_SUSTA_CNTL = SUSTA_CNTL_RCMD;
    rIIC_SUSTO_CNTL = SUSTO_CNTL_RCMD;
    rIIC_BUF_CNTL = BUF_CNTL_RCMD;
    rIIC_SDAO_CNTL = SDAO_CNTL_RCMD;
    rIIC_HDSTA_CNTL = HDSTA_CNTL_RCMD;

    pModeReg = (volatile IIC_MODE_REG *)&rIIC_MODE;
    pModeReg->bsMsbMode = 1; //MSB first
    pModeReg->bsReadMode = 0; //read one time, pop 4-byte

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_IICSetDeviceID
Description: 
    set IIC accessing device ID
Input Param: 
    BOOL bID10Bit: 0-device ID is 7-bit
                   1-device ID is 10-bit
    U16 usDeviceID: device ID
Output Param: 
    none
Return Value:
    void
Usage:
    before write or read data via IIC  
History:
20141104    Kristin    create
------------------------------------------------------------------------------*/
void HAL_IICSetDeviceID(BOOL bID10Bit, U16 usDeviceID)
{
    volatile IIC_ID_MSK_REG *pIdMskReg;

    pIdMskReg = (volatile IIC_ID_MSK_REG *)&rIIC_ID_MSK;

    pIdMskReg->bsID10BitEn = bID10Bit;
    pIdMskReg->bsDeviceID = usDeviceID;

    return;
}

/*----------------------------------------------------------------------------
Name: HAL_IICWriteData
Description: 
    write not more than 8-byte data to slave device via IIC bus
Input Param: 
    U8 ucLenByte: data length,not more than 8-byte
    U32 ulFirstDW: the first 4-byte data to write, if data length < 4-byte,the 
                   low bytes is valid.
    U32 ulSecondDW: the second 4-byte data to write, if data length <= 4-byte, 
                    this parameter is invalid; if 4-byte < data length < 8-byte,
                    the low bytes is valid.
Output Param: 
    none
Return Value:
    BOOL : TRUE - write data successfully
           FALSE - error occur,write data fail
Usage:
    write short data to slave device via IIC bus 
History:
20141104    Kristin    create
------------------------------------------------------------------------------*/
BOOL HAL_IICWriteData(U8 ucLenByte, U8 *pTxData)
{
    IIC_CMD_REG tCmdReg;
    U8 ucByteIndex;
    U8 ucRegIndex;
    U8 *pDataByte = pTxData;

    if (NULL == pTxData)
    {
        DBG_Printf("HAL_IICWriteData: pTxData is NULL,ERROR!\n");
        DBG_Getch();
    }

    if (ucLenByte > 8)
    {
        DBG_Printf("HAL_IICWriteData: Data to long, %d bytes,ERROR!\n", ucLenByte);
        DBG_Getch();
    }

    for (ucByteIndex = 0; ucByteIndex < ucLenByte; ucByteIndex++)
    {
        ucRegIndex = ucByteIndex % IIC_DATA_REG_SIZE;
        rIIC_TX_DATA(ucRegIndex) = *pDataByte;
        pDataByte++;
    }

    /* trigger IIC send data to slave */
    COM_MemSet((U32 *)&tCmdReg, sizeof(IIC_CMD_REG)/sizeof(U32), 0);
    tCmdReg.bsLen = ucLenByte - 1;
    tCmdReg.bsRW = IIC_TRIGGER_WRITE;
    rIIC_CMD = *((U32 *)&tCmdReg);

    /* waite write finish */
    while (FALSE == (rIIC_INT & (1 << IIC_INT_WFINISH_BIT)))
    {
        if (TRUE == (rIIC_INT & (1 << IIC_INT_ERR_BIT)))
        {
            rIIC_INT |= (1 << IIC_INT_ERR_BIT);
            return FALSE;
        }
    }

    /* clear write finish interrupt */
    rIIC_INT |= (1 << IIC_INT_WFINISH_BIT);

    return TRUE;
}

/*----------------------------------------------------------------------------
Name: HAL_IICReadData
Description: 
    read not more than 8-byte data from slave device via IIC bus
Input Param: 
    U8 ucLenByte: data length,not more than 8-byte
Output Param: 
    U32 *pFirstDW: store the first 4-byte data IIC read, if data length < 4-byte,the 
                   low bytes is valid.
    U32 *pSecondDW: store second 4-byte data IIC read, if data length <= 4-byte, 
                    this parameter can be NULL; if 4-byte < data length < 8-byte,
                    the low bytes is valid.
Return Value:
    BOOL : TRUE - read data successfully
           FALSE - error occur,read data fail
Usage:
    read short data from slave device via IIC bus
History:
20141104    Kristin    create
------------------------------------------------------------------------------*/
BOOL HAL_IICReadData(U8 ucLenByte, U8 *pRxData)
{
    IIC_CMD_REG tCmdReg;
    U32 ulRxDataTemp;
    U8 ucByteIndex;
    U8 ucRegIndex;
    U8 *pDataByte;

    if (NULL == pRxData)
    {
        DBG_Printf("HAL_IICReadData: pRxData is NULL,ERROR!\n");
        DBG_Getch();
    }

    if (ucLenByte > 8)
    {
        DBG_Printf("HAL_IICReadData: Data to long, %d bytes,ERROR!\n", ucLenByte);
        DBG_Getch();
    }

    /* trigger IIC to receive data from slave */
    COM_MemSet((U32 *)&tCmdReg, sizeof(IIC_CMD_REG)/sizeof(U32), 0);
    tCmdReg.bsLen = ucLenByte - 1;
    tCmdReg.bsRW = IIC_TRIGGER_READ;
    rIIC_CMD = *((U32 *)&tCmdReg);

    /* waite read finish */
    while (FALSE == (rIIC_INT & (1 << IIC_INT_RFINISH_BIT)))
    {
        if (TRUE == (rIIC_INT & (1 << IIC_INT_ERR_BIT)))
        {
            rIIC_INT |= (1 << IIC_INT_ERR_BIT);
            return FALSE;
        }
    }

    pDataByte = pRxData;
    for (ucByteIndex = 0; ucByteIndex < ucLenByte; ucByteIndex++)
    {
       if ((0 == ucByteIndex) || (IIC_DATA_REG_SIZE == ucByteIndex))
        {
            ulRxDataTemp = rIIC_RX_DATA;
        }

        ucRegIndex = ucByteIndex % IIC_DATA_REG_SIZE;
        *pDataByte = (U8)(ulRxDataTemp >> (8 * ucRegIndex));
        pDataByte++;
    }

    /* clear read finish interrupt */
    rIIC_INT |= (1 << IIC_INT_RFINISH_BIT);

    return TRUE;
}

/********************** FILE END ***************/
