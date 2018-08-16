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
  File Name     : hal_uart.cpp
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
#include <stddef.h>
#include "..\\host_api.h"
#include "hal_uart.h"
#include "ioctrl_uart.h"
BOOL Uart_Be_NoneData(FW_CMD Fw_Cmd)
{
	if(Fw_Cmd.Feature<OP_GET_VAR_TABLE)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
BOOL Uart_Set_Trans_Speed(HANDLE hDevice,U32 ulSpeed)
{

	DCB commParam;
	GetCommState(hDevice, &commParam);
		
	//memset(&commParam,0,sizeof(commParam));
	
	commParam.BaudRate=ulSpeed;
	
	SetCommState(hDevice, &commParam);
	return TRUE;
}
STATUS Uart_Dma_Dram_Sram(HANDLE hDevice,U32 length,U8 * readbuf,FW_CMD Fw_Cmd,U8 TransToDev)
{
	CMD_STAT_FW Stat;
	PFW_CMD pFwCmd;
	FW_STATUS Fw_Status;
	BOOL be_done, res;
	U32 usDataCrc;
	Stat = FW_CMD_SEND;
	be_done = 1;
	res = TRUE;
	while (be_done)
	{
		switch (Stat)
		{
		case FW_CMD_SEND:
		{
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Fw_Cmd, sizeof(FW_CMD));
			Stat = FW_CMD_PROCESS;
			break;			
		}
		case FW_CMD_PROCESS:
		{
			if(TransToDev == 1)
			{
				res = Hal_Uart_Send_Data(hDevice,readbuf,length);
			}
			else
			{
				res = Hal_Uart_Receive_Data(hDevice,readbuf,length);
			}
			if(!Uart_Be_NoneData(Fw_Cmd))
			{
				Stat = ((res==TRUE)?FW_CMD_SEND_STAT:FW_CMD_SEND_CLR_STAT);
			}
			else
			{
				Stat = ((res==TRUE)?FW_CMD_CHECK_REPLY:FW_CMD_SEND_CLR_STAT);
			}
			break;
		}
		case FW_CMD_CHECK_REPLY:
		{
			pFwCmd = (PFW_CMD)readbuf;
			if(pFwCmd->usCrc != Api_Rom_Gen_Crc((U16 *)readbuf, CMD_PAY_LOAD_CNT))
			{
				Stat = FW_CMD_SEND;
			}
			else
			{
				Stat = FW_CMD_FINISH;
			}
			break;
		}
		case FW_CMD_SEND_STAT:
		{
			memset(&Fw_Status, 0, sizeof(FW_STATUS));
			Fw_Status.ucCmd = OP_UART_STAT;
			if(TransToDev == 1)
			{
				Fw_Status.usDataCrc = Api_Data_Gen_Crc((U32 *)readbuf, length);//Api_Rom_Gen_Crc((U16 *)readbuf, length);
			}
			Fw_Status.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Status, CMD_PAY_LOAD_CNT);
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Fw_Status, sizeof(FW_STATUS));
			Stat = FW_CMD_GET_STAT;
			if(TransToDev == 0)
			{
				usDataCrc = Api_Data_Gen_Crc((U32 *)readbuf, length);
			}
			break;
		}
		case FW_CMD_GET_STAT:
		{
			memset(&Fw_Status, 0, sizeof(FW_STATUS));
			res = Hal_Uart_Receive_Data(hDevice,(U8 *)&Fw_Status,sizeof(FW_STATUS));
			if(res == FALSE)
			{
				Stat = FW_CMD_SEND;
			}
			else
			{
				if(Fw_Status.usCrc!=Api_Rom_Gen_Crc((U16 *)&Fw_Status, CMD_PAY_LOAD_CNT))
				{
					Stat = FW_CMD_SEND_STAT;
					break;
				}
				if((TransToDev == 0)&&(!Uart_Be_NoneData(Fw_Cmd)))
				{
					if(usDataCrc!=Fw_Status.usDataCrc)
					{
						Stat = FW_CMD_SEND;
						break;
					}
				}
				if(Fw_Status.ucErr == TRUE)
				{
					Stat = FW_CMD_SEND_CLR_STAT;
					break;
				}
				else
				{
					Stat = FW_CMD_FINISH;
					break;
				}
				_getch();
			}
			
		}
		case FW_CMD_SEND_CLR_STAT:
		{
			memset(&Fw_Status, 0, sizeof(FW_STATUS));
			Fw_Status.ucCmd = OP_UART_CLEAR_STAT;
			Fw_Status.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Status, CMD_PAY_LOAD_CNT);
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Fw_Status, sizeof(FW_STATUS));
			Stat = FW_CMD_RECV_CLR_STAT;
			break;
		}
		case FW_CMD_RECV_CLR_STAT:
		{
			memset(&Fw_Status, 0, sizeof(FW_STATUS));
			res = Hal_Uart_Receive_Data(hDevice,(U8 *)&Fw_Status,sizeof(FW_STATUS));
			if(res == FALSE)
			{
				Stat = FW_CMD_SEND_CLR_STAT;
			}
			else
			{
				if(Fw_Status.usCrc!=Api_Rom_Gen_Crc((U16 *)&Fw_Status, CMD_PAY_LOAD_CNT))
				{
					Stat = FW_CMD_SEND_CLR_STAT;
					break;
				}
				
				if(Fw_Status.ucErr == TRUE)
				{
					Stat = FW_CMD_SEND_CLR_STAT;
				}
				else
				{
					Stat = FW_CMD_SEND;
				}
				break;
			}
			
		}
		case FW_CMD_FINISH:
		{
			be_done = 0;
			break;
		}
		default:
			break;
		}
	}

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
	
}
STATUS Uart_Read_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf)
{
	FW_CMD Fw_Cmd;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	Fw_Cmd.Byte[0] = (startaddr&0xff);
	Fw_Cmd.Byte[1] = ((startaddr&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((startaddr&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((startaddr&0xff000000)>>24);
	Fw_Cmd.Byte[4] = ((length&0xff));
	Fw_Cmd.Byte[5] = ((length&0xff00)>>8);
	Fw_Cmd.Byte[8] = cpuid;
	Fw_Cmd.Feature = OP_MEM_READ;
	Fw_Cmd.OpCode = 0xff;

	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);
	return Uart_Dma_Dram_Sram(hDevice,length,readbuf,Fw_Cmd,0);
}
STATUS Uart_Write_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf)
{
	
	FW_CMD Fw_Cmd;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	Fw_Cmd.Byte[0] = (startaddr&0xff);
	Fw_Cmd.Byte[1] = ((startaddr&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((startaddr&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((startaddr&0xff000000)>>24);
	Fw_Cmd.Byte[4] = ((length&0xff));
	Fw_Cmd.Byte[5] = ((length&0xff00)>>8);
	Fw_Cmd.Byte[8] = cpuid;
	Fw_Cmd.Feature = OP_MEM_WRITE;
	Fw_Cmd.OpCode = 0xff;

	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);
	return Uart_Dma_Dram_Sram(hDevice,length,writebuf,Fw_Cmd,1);
}
STATUS Uart_Read_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk)
{
	FW_CMD Fw_Cmd;
	U8 befullpln;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	befullpln = plnmode;
	Fw_Cmd.Byte[0] = (pumsk&0xff);
	Fw_Cmd.Byte[1] = ((pumsk&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((pumsk&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((pumsk&0xff000000)>>24);
	Fw_Cmd.Byte[4] = ((block&0xff));
	Fw_Cmd.Byte[5] = ((block&0xff00)>>8);
	Fw_Cmd.Byte[6] = (page&0xff);
	Fw_Cmd.Byte[7] = CACULATE_PG_PLN(page,pln,befullpln);
	Fw_Cmd.Byte[8] = cpuid;
	Fw_Cmd.Feature = OP_FLASH_READ;
	Fw_Cmd.OpCode = 0xff;

	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);
	return Uart_Dma_Dram_Sram(hDevice,FLASH_STATUS_MAX_CNT,pStatusBuf,Fw_Cmd,0);
}
STATUS Uart_Write_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk)
{
	FW_CMD Fw_Cmd;
	U8 befullpln;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	befullpln = plnmode;
	Fw_Cmd.Byte[0] = (pumsk&0xff);
	Fw_Cmd.Byte[1] = ((pumsk&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((pumsk&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((pumsk&0xff000000)>>24);
	Fw_Cmd.Byte[4] = ((block&0xff));
	Fw_Cmd.Byte[5] = ((block&0xff00)>>8);
	Fw_Cmd.Byte[6] = (page&0xff);
	Fw_Cmd.Byte[7] = CACULATE_PG_PLN(page,pln,befullpln);
	Fw_Cmd.Byte[8] = cpuid;
	Fw_Cmd.Feature = OP_FLASH_WRITE;
	Fw_Cmd.OpCode = 0xff;

	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);
	return Uart_Dma_Dram_Sram(hDevice,FLASH_STATUS_MAX_CNT,pStatusBuf,Fw_Cmd,0);
}
STATUS Uart_Erase_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U8 *pStatusBuf,U32 pumsk)
{
	FW_CMD Fw_Cmd;
	U8 befullpln;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	befullpln = plnmode;
	Fw_Cmd.Byte[0] = (pumsk&0xff);
	Fw_Cmd.Byte[1] = ((pumsk&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((pumsk&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((pumsk&0xff000000)>>24);
	Fw_Cmd.Byte[4] = ((block&0xff));
	Fw_Cmd.Byte[5] = ((block&0xff00)>>8);
	
	Fw_Cmd.Byte[7] = CACULATE_PG_PLN(0,pln,befullpln);
	Fw_Cmd.Byte[8] = cpuid;
	Fw_Cmd.Feature = OP_FLASH_ERASE;
	Fw_Cmd.OpCode = 0xff;

	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);
	return Uart_Dma_Dram_Sram(hDevice,FLASH_STATUS_MAX_CNT,pStatusBuf,Fw_Cmd,0);
}
STATUS Uart_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2)
{
	FW_CMD Fw_Cmd;
	FW_CMD Fw_Cmd_Return;
	STATUS Res;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	
	Fw_Cmd.Byte[0] = (ulInParam1&0xff);
	Fw_Cmd.Byte[1] = ((ulInParam1&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((ulInParam1&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((ulInParam1&0xff000000)>>24);
	Fw_Cmd.Byte[4] = (ulInParam2&0xff);
	Fw_Cmd.Byte[5] = ((ulInParam2&0xff00)>>8);
	Fw_Cmd.Byte[6] = ((ulInParam2&0xff0000)>>16);
	Fw_Cmd.Byte[7] = ((ulInParam2&0xff000000)>>24);
	Fw_Cmd.Byte[8] = ucCpuId;
	Fw_Cmd.Feature = ucOpCode;
	Fw_Cmd.OpCode = 0xff;
	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);

	memset(&Fw_Cmd_Return, 0, sizeof(FW_CMD));
	Res = Uart_Dma_Dram_Sram(hDevice,sizeof(FW_CMD),(U8 *)&Fw_Cmd_Return,Fw_Cmd,0);
	if(Res == RETURN_SUCCESS)
	if (NULL != pOutParam1)
	{
		*pOutParam1 = (Fw_Cmd_Return.Byte[0])|((Fw_Cmd_Return.Byte[1])<<8)|((Fw_Cmd_Return.Byte[2])<<16)|((Fw_Cmd_Return.Byte[3])<<24);
	}
	if (NULL != pOutParam2)
	{
	
	}
	return Res;
}

BOOL Hal_Uart_Chk_Crc(U16 * pRomFinish)
{
	BOOL res;
	PROM_FINISH pRomReturn;
	U16 usHostCrc,usDevCrc;
	usHostCrc = Api_Rom_Gen_Crc(pRomFinish,CMD_PAY_LOAD_CNT);
	pRomReturn = (PROM_FINISH)pRomFinish;
	usDevCrc = pRomReturn->usCrc;
	res = (usDevCrc==usHostCrc?TRUE:FALSE);
	return  res;
}

BOOL Uart_Local_Rom_Read_Status(HANDLE hDevice, PROM_FINISH pRomFinish)
{
	CMD_STAT Stat;
	ROM_CMD Rom_Cmd;
	ROM_FINISH Rom_Finish;
	BOOL be_done, res;
	BOOL bStatus;
	U8 retry_cnt;
	Stat = CMD_SEND;
	be_done = 1;
	retry_cnt = 0;
	bStatus = FALSE;
	Stat = CMD_SEND_STAT;
	while (be_done)
	{
		switch (Stat)
		{
		case CMD_SEND_STAT:
		{
			memset(&Rom_Cmd, 0, sizeof(ROM_CMD));
			Rom_Cmd.ucCmd = ROM_READ_STAT;

			Rom_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Rom_Cmd, CMD_PAY_LOAD_CNT);
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Rom_Cmd, sizeof(ROM_CMD));
			Stat = CMD_RECV_STAT;
			break;
		}
		case CMD_RECV_STAT:
		{
			res = Hal_Uart_Receive_Reply(hDevice, (U8 *)&Rom_Finish, sizeof(ROM_FINISH));
			if (res == FALSE)
			{
				Stat = CMD_SEND_STAT;
				break;
			}
			res = Hal_Uart_Chk_Crc((U16 *)&Rom_Finish);
			if (res == FALSE)
			{
				Stat = CMD_SEND_STAT;
				break;
			}
			if (Rom_Finish.ucStatus == 0)
			{
				if (pRomFinish != NULL)
				{
					memcpy((U8 *)pRomFinish, (U8 *)&Rom_Finish, sizeof(ROM_FINISH));
				}
				bStatus = TRUE;
				be_done = 0;
			}
			else
			{
				Stat = CMD_SEND_CLR_STAT;
			}
			break;
		}
		case CMD_SEND_CLR_STAT:
		{
			memset((U8*)&Rom_Cmd, 0, sizeof(ROM_CMD));
			Rom_Cmd.ucCmd = ROM_CLEAR_STAT;
			Rom_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Rom_Cmd, CMD_PAY_LOAD_CNT);
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Rom_Cmd, sizeof(ROM_CMD));
			Stat = CMD_RECV_CLR_STAT;
			break;
		}
		case CMD_RECV_CLR_STAT:
		{
			res = Hal_Uart_Receive_Reply(hDevice, (U8 *)&Rom_Finish, sizeof(ROM_FINISH));
			if (res == FALSE)
			{
				Stat = CMD_SEND_CLR_STAT;
				break;
			}
			res = Hal_Uart_Chk_Crc((U16 *)&Rom_Finish);
			if (res == FALSE)
			{
				Stat = CMD_SEND_CLR_STAT;
				break;
			}

			bStatus = FALSE;
			be_done = 0;
			break;
		}
		default:
			break;
		}
	}

	return bStatus;
}
BOOL Uart_Rom_Read_Reg(HANDLE hDevice, U32 ulAddr, U32 * pData)
{
	CMD_STAT Stat;
	ROM_CMD Rom_Cmd;
	ROM_FINISH Rom_Finish;
	BOOL be_done, res;
	Stat = CMD_SEND;
	be_done = 1;
	
	while (be_done)
	{
		switch (Stat)
		{
		case CMD_SEND:
		{
			memset(&Rom_Cmd, 0, sizeof(ROM_CMD));
			Rom_Cmd.ucCmd = ROM_READ;
			Rom_Cmd.ulAddr = ulAddr;
			Rom_Cmd.ucCnt = 1;

			Rom_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Rom_Cmd, CMD_PAY_LOAD_CNT);
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Rom_Cmd, sizeof(ROM_CMD));
			Stat = CMD_SEND_STAT;
			break;
		}
		case CMD_SEND_STAT:
		{
			res=Uart_Local_Rom_Read_Status(hDevice, &Rom_Finish);
			//Stat = (res == TRUE ? CMD_FINISH : CMD_SEND);
			Stat = CMD_FINISH;
			break;
		}

		case CMD_FINISH:
		{
			if (res==TRUE)
				*pData = Rom_Finish.ulData[0];
			be_done = 0;
			break;
		}
		default:
			break;
		}
	}

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}
BOOL Uart_Rom_Read_Status(HANDLE hDevice)
{
	BOOL bStatus;
	bStatus = Uart_Local_Rom_Read_Status(hDevice,NULL);

	return bStatus;
}


BOOL Uart_Rom_Set_Uart_Baudrate(HANDLE hDevice, U32 ulSpeedDev, U32 ulSpeedHost)
{
	Uart_Rom_Write_Reg(hDevice, 0xffe0700c, ulSpeedDev);
	Sleep(2000);
	Uart_Set_Trans_Speed(hDevice, ulSpeedHost);
	return TRUE;
}
STATUS Uart_Fw_Set_Uart_Baudrate(HANDLE hDevice, U32 ulSpeedDev, U32 ulSpeedHost)
{
	U8 Buf[16];
	DWORD Cnt;
	FW_CMD Fw_Cmd;
	FW_CMD Fw_Cmd_Return;
	memset(&Fw_Cmd, 0, sizeof(Fw_Cmd));
	
	Fw_Cmd.Byte[0] = (0xffe0700c&0xff);
	Fw_Cmd.Byte[1] = ((0xffe0700c&0xff00)>>8);
	Fw_Cmd.Byte[2] = ((0xffe0700c&0xff0000)>>16);
	Fw_Cmd.Byte[3] = ((0xffe0700c&0xff000000)>>24);
	Fw_Cmd.Byte[4] = (ulSpeedDev&0xff);
	Fw_Cmd.Byte[5] = ((ulSpeedDev&0xff00)>>8);
	Fw_Cmd.Byte[6] = ((ulSpeedDev&0xff0000)>>16);
	Fw_Cmd.Byte[7] = ((ulSpeedDev&0xff000000)>>24);
	Fw_Cmd.Byte[8] = MCU0;
	Fw_Cmd.Feature = OP_REG_WRITE;
	Fw_Cmd.OpCode = 0xff;
	Fw_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Fw_Cmd, CMD_PAY_LOAD_CNT);

	memset(&Fw_Cmd_Return, 0, sizeof(FW_CMD));
	Hal_Uart_Send_Cmd(hDevice,(U8 *)&Fw_Cmd,sizeof(FW_CMD));
	Sleep(2000);
	Uart_Set_Trans_Speed(hDevice, ulSpeedHost);
	Cnt = 0xff;
	while(Cnt!=0)
	{
		ReadFile(hDevice, (LPVOID)Buf, 16, (LPDWORD)&Cnt, NULL);
		Sleep(1000);
	}
	
	//Uart_Set_Trans_Speed(hDevice, ulSpeedHost);
	return RETURN_SUCCESS;
}
BOOL Uart_Rom_Write_Reg(HANDLE hDevice, U32 ulAddr, U32 ulData)
{
	ROM_CMD Rom_Cmd;
	BOOL Res;
	
	memset(&Rom_Cmd, 0, sizeof(ROM_CMD));
	Rom_Cmd.ucCmd = ROM_WRITE;
	Rom_Cmd.ulAddr = ulAddr;
	Rom_Cmd.ulData[0] = ulData;
	Rom_Cmd.ucCnt = 1;
	Rom_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Rom_Cmd ,CMD_PAY_LOAD_CNT);
	 
	Res = Hal_Uart_Send_Cmd(hDevice, (U8 *)&Rom_Cmd, sizeof(ROM_CMD));
	if (Res != TRUE)
		return RETURN_FAIL;

	return RETURN_SUCCESS;
}
BOOL Uart_Rom_Write_Dma(HANDLE hDevice, U32 ulAddr, U32 * pBuf,U32 ulCnt)
{
	ROM_CMD Rom_Cmd;
	ROM_WT_DMA_PAYLOAD Wt_PayLoad;
	BOOL Res;
	U32 i,RemLen,Cnt;
	U32 * pData;
	
	memset(&Rom_Cmd, 0, sizeof(ROM_CMD));
	Rom_Cmd.ucCmd = ROM_WRITE_DMA_OP;
	Rom_Cmd.ulAddr = ulAddr;
	//Rom_Cmd.ucCnt = ulCnt;
	Rom_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Rom_Cmd, CMD_PAY_LOAD_CNT);
	Res = Hal_Uart_Send_Cmd(hDevice, (U8 *)&Rom_Cmd, sizeof(ROM_CMD));
	if (Res != TRUE)
		return RETURN_FAIL;
	RemLen = (ulCnt>>2);
	pData = pBuf;
	while (RemLen)
	{
		Cnt = (RemLen>ROM_WT_DMA_DW_SIZE?ROM_WT_DMA_DW_SIZE:RemLen);
		memset(&Wt_PayLoad, 0, sizeof(ROM_CMD));
		Wt_PayLoad.ucCmd = ROM_WRITE_DMA_PAYLOAD;
		Wt_PayLoad.ucCnt = Cnt;
		for (i = 0; i < Cnt; i++)
		{
			Wt_PayLoad.ulData[i] = *pData++;
		}
		Wt_PayLoad.usCrc = Api_Rom_Gen_Crc((U16 *)&Wt_PayLoad, CMD_PAY_LOAD_CNT);
		
		
		Res = Hal_Uart_Send_Cmd(hDevice, (U8 *)&Wt_PayLoad, sizeof(ROM_WT_DMA_PAYLOAD));
		if (Res != TRUE)
			return RETURN_FAIL;
		RemLen -= Cnt;
	}
	
	return RETURN_SUCCESS;
}
BOOL Uart_Rom_Close_Handle(HANDLE hDevice)
{
	CloseHandle(hDevice);
	
	return TRUE;
}
BOOL Uart_Rom_Jump(HANDLE hDevice, U32 ulAddr)
{
	CMD_STAT Stat;
	ROM_CMD Rom_Cmd;
	ROM_FINISH Rom_Finish;
	BOOL be_done, res;
	Stat = CMD_SEND;
	be_done = 1;
	res = TRUE;
	while (be_done)
	{
		switch (Stat)
		{
		case CMD_SEND:
		{
			memset(&Rom_Cmd, 0, sizeof(ROM_CMD));
			Rom_Cmd.ucCmd = ROM_JUMP;
			Rom_Cmd.ulAddr = ulAddr;

			Rom_Cmd.usCrc = Api_Rom_Gen_Crc((U16 *)&Rom_Cmd, CMD_PAY_LOAD_CNT);
			Hal_Uart_Send_Cmd(hDevice, (U8 *)&Rom_Cmd, sizeof(ROM_CMD));
			if (ulAddr == BL_JUMP_ADDR)
				Stat = CMD_FINISH;
			else
				Stat = CMD_SEND_STAT;
			break;
		}
		case CMD_SEND_STAT:
		{
			res = Uart_Local_Rom_Read_Status(hDevice, &Rom_Finish);
			//Stat = (res == TRUE ? CMD_FINISH : CMD_SEND);
			Stat = CMD_FINISH;
			break;
		}

		case CMD_FINISH:
		{
			be_done = 0;
			break;
		}
		default:
			break;
		}
	}

	return (res == TRUE ? RETURN_SUCCESS : RETURN_FAIL);
}