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
  File Name     : hal_uart.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the hal function of ahci.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/
#ifndef _HAL_Uart_H
#define _HAL_Uart_H
#include "host_api_define.h"
#define BL_JUMP_ADDR 0xfff00008
#define ROM_CMD_SIZE 16
#define CMD_PAY_LOAD_CNT (ROM_CMD_SIZE-2)
#define FLASH_STATUS_MAX_CNT 64
#define ROM_WT_DMA_DW_SIZE 3
typedef enum _CMD_STAT
{
	CMD_SEND,
	CMD_SEND_STAT,
	CMD_RECV_STAT,
	CMD_SEND_CLR_STAT,
	CMD_RECV_CLR_STAT,
	CMD_FINISH,
	CMD_CNT
	
}CMD_STAT;
typedef enum _CMD_STAT_FW
{
	FW_CMD_SEND,
	FW_CMD_PROCESS,
	FW_CMD_SEND_STAT,
	FW_CMD_GET_STAT,
	FW_CMD_CHECK_REPLY,
	FW_CMD_SEND_CLR_STAT,
	FW_CMD_RECV_CLR_STAT,
	FW_CMD_FINISH,
	FW_CMD_CNT
	
}CMD_STAT_FW;
typedef enum _ROM_READ_CMD_STAT
{
	CMD_PROCESS_OK,
	CMD_SEND_FAIL,
	CMD_RECEIVE_REPLY_FAIL,
	CMD_CRC_FAIL,
	CMD_RETURN_STATUS_ERR
}ROM_READ_CMD_STAT;
typedef enum _ROM_CMD_TYPE
{
	ROM_READ,
	ROM_WRITE,
	ROM_JUMP,
	ROM_READ_STAT,
	ROM_CLEAR_STAT,
	ROM_WRITE_DMA_OP,
	ROM_WRITE_DMA_PAYLOAD
}ROM_CMD_TYPE;
typedef struct _ROM_WT_DMA_PAYLOAD
{
	U32 ulData[3];
	U8 ucCmd;
	U8 ucCnt;
	U16 usCrc;
}ROM_WT_DMA_PAYLOAD, *PROM_WT_DMA_PAYLOAD;
typedef struct _ROM_CMD
{
	U32 ulAddr;
	U32 ulData[2];
	U8 ucCmd;
	U8 ucCnt;
	U16 usCrc;
	
}ROM_CMD, *PROM_CMD;

typedef struct _FW_CMD
{
	U8 Byte[9];
	U8 RsvByte[3];
	U8 OpCode;
	U8 Feature;
	U16 usCrc;
	
}FW_CMD, *PFW_CMD;

typedef struct _FW_STATUS
{
	U32 usDataCrc;
	
	U32 Rsv1[2];
	U8 ucCmd;
	U8 ucErr;
	U16 usCrc;
	
}FW_STATUS, *PFW_STATUS;

typedef struct _ROM_FINISH
{
	U32 ulData[3];
	U8 ucCmd;
	U8 ucStatus;
	U16 usCrc;

}ROM_FINISH, *PROM_FINISH;


STATUS Uart_Read_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf);
STATUS Uart_Write_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf);
STATUS Uart_Read_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk);
STATUS Uart_Write_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk);
STATUS Uart_Erase_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U8 *pStatusBuf,U32 pumsk);
STATUS Uart_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2);
STATUS Uart_Read_Register(HANDLE hDevice,U32 regaddr,U32 * regdata);
STATUS Uart_Write_Register(HANDLE hDevice,U32 regaddr,U32 regdata);
STATUS Uart_Fw_Set_Uart_Baudrate(HANDLE hDevice, U32 ulSpeedDev, U32 ulSpeedHost);

STATUS Uart_Cmd_Dma_Read(HANDLE hDevice,U32 lba,U8 * inbuf_addr,U32 trans_sec_cnt);
STATUS Uart_Cmd_Dma_Write(HANDLE hDevice, U32 lba, U8 * outbuf_addr, U32 trans_sec_cnt);
BOOL Uart_Rom_Read_Reg(HANDLE hDevice,U32 ulAddr,U32 * pData);
BOOL Uart_Rom_Write_Reg(HANDLE hDevice, U32 ulAddr, U32 ulData);
BOOL Uart_Rom_Jump(HANDLE hDevice, U32 ulAddr);
#endif