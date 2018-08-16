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
  File Name     : ioctrl_scsi.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines ioctrl function of scsi.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include "hal_uart.h"
#include "ioctrl_uart.h"
//#include "ntddscsi.h"


BOOL Hal_Uart_Ctrl(HANDLE Handle, BOOL bRead,U8 *  pRomFinish, U32 ulCmdLen)
{
	BOOL Res;
	LPDWORD Cnt;
	U16 uLoop;
	U32 usTotalLen;
	U8 * pBuf;
	LARGE_INTEGER t1,t2,t3,t4,freq;
	uLoop = 10;
	usTotalLen = 0;
	pBuf = pRomFinish;
	QueryPerformanceFrequency(&freq);
	while (uLoop)
	{ 
		if (bRead == 1)
		{
			QueryPerformanceCounter(&t1);
			Res = ReadFile(Handle, (LPVOID)pBuf, ulCmdLen, (LPDWORD)&Cnt, NULL);
			QueryPerformanceCounter(&t2);
			//printf("rd tick:%f\n",(float)(t2.QuadPart-t1.QuadPart)/freq.QuadPart*1000);
	
		}
		else
		{
			QueryPerformanceCounter(&t3);
			Res = WriteFile(Handle, (LPVOID)pBuf, ulCmdLen, (LPDWORD)&Cnt, NULL);
			QueryPerformanceCounter(&t4);
			//printf("wt tick:%f\n",(float)(t4.QuadPart-t3.QuadPart)/freq.QuadPart*1000);
		}
		
		if (Res == TRUE)
		{
			usTotalLen += (U32)Cnt;
			pBuf += (U32)Cnt;
			if (ulCmdLen == (U32)usTotalLen)
			{
				//printf("loop:%d\n",uLoop);
				return TRUE;
			}
		}
		else
		{
			if (bRead == 1)
				printf("ReadFile err!\n");
			else
				printf("WriteFile err!\n");
			_getch();
		}
		uLoop--;
	}
	printf("exceed time!\n");
	return FALSE;
}
BOOL Hal_Uart_Send_Cmd(HANDLE Handle, U8 * pRomCmd, U32 ulCmdLen)
{
	return Hal_Uart_Ctrl(Handle, 0, pRomCmd, ulCmdLen);
}
BOOL Hal_Uart_Receive_Reply(HANDLE Handle, U8 * pRomCmd, U32 ulCmdLen)
{
	return Hal_Uart_Ctrl(Handle, 1, pRomCmd, ulCmdLen);
}
BOOL Hal_Fw_Uart_Ctrl(HANDLE Handle, U8 * pDataBuf, U32 ulLen,BOOL bRead)
{
	BOOL Res,be_done;
	LPDWORD Cnt;
	U16 uLoop;
	U32 usTotalLen;
	U8 * pBuf;
	uLoop = 10;
	usTotalLen = 0;
	pBuf = pDataBuf;
	be_done = TRUE;
	while (be_done)
	{ 
		if(uLoop == 0)
		{
			be_done = FALSE;
			Res = FALSE;
		}
		if(bRead==1)
		{
			Res = ReadFile(Handle, (LPVOID)pBuf, ulLen, (LPDWORD)&Cnt, NULL);
		}
		else
		{
			Res = WriteFile(Handle, (LPVOID)pBuf, ulLen, (LPDWORD)&Cnt, NULL);
		}
		
		if (Res == TRUE)
		{
			if(Cnt!=0)
			{
				usTotalLen += (U32)Cnt;
				pBuf += (U32)Cnt;
				if (ulLen == (U32)usTotalLen)
				{
					be_done = FALSE;
					Res = TRUE;
				}
			}
			else
			{
				uLoop--;
			}
		}
		else
		{
			if(bRead==1)
				printf("ReadFile err!\n");
			else
				printf("WriteFile err!\n");
			_getch();
		}
	}
	if(uLoop==0)
		printf("exceed time!\n");
	return Res;
}
BOOL Hal_Uart_Receive_Data(HANDLE Handle, U8 * pDataBuf, U32 ulLen)
{
	return Hal_Fw_Uart_Ctrl(Handle,pDataBuf,ulLen,1);
}
BOOL Hal_Uart_Send_Data(HANDLE Handle, U8 * pDataBuf, U32 ulLen)
{
	return Hal_Fw_Uart_Ctrl(Handle,pDataBuf,ulLen,0);
}