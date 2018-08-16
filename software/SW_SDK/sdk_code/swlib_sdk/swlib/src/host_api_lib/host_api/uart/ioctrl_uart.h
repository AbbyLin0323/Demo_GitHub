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
  File Name     : ioctrl_scsi.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the data structure and macro for scsi_iotrl
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _UART_IOCTRL
#define _UART_IOCTRL
BOOL Hal_Uart_Send_Cmd(HANDLE Handle, U8 * pRomCmd, U32 ulCmdLen);
BOOL Hal_Uart_Receive_Reply(HANDLE Handle, U8 *  pRomFinish, U32 ulCmdLen);
BOOL Hal_Uart_Check_Res(PROM_FINISH pRomFinish);
BOOL Hal_Uart_Receive_Data(HANDLE Handle, U8 * pDataBuf, U32 ulLen);
BOOL Hal_Uart_Send_Data(HANDLE Handle, U8 * pDataBuf, U32 ulLen);
#endif