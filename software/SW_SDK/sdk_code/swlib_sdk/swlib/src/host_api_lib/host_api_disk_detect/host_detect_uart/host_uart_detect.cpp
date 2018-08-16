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
File Name     : host_sata_detect.cpp
Version       : Release 0.0.1
Author        : johnzhang
Created       : 2015/2/28
Last Modified :
Description   : Defines the sata enumerate function.
Function List :
History       :
1.Date        : 2015/2/28
Author      : johnzhang
Modification: Created file

******************************************************************************/

#include <stdlib.h>
#include <tchar.h>
#include "../../host_api/host_api.h"
#include "../host_disk_detect.h"
extern DEVICE_OBJECT g_dev_obj[MAX_DISK];

BOOL Api_Get_Uart_Sig(PDEVICE_OBJECT pdevobj)
{
	U8 SigBuf[] = "3514000000000000";
	U32 Cnt, Loop;
	BOOL Res;
	U8 tmpbuf[16];
	while (1)
	{
		Res = ReadFile(pdevobj->Handle, tmpbuf, sizeof(tmpbuf) - 1, (LPDWORD)&Cnt, NULL);
		if (Cnt == 0)
			break;
	}



	Res = WriteFile(pdevobj->Handle, SigBuf, sizeof(SigBuf) - 1, (LPDWORD)&Cnt, NULL);
	if (Res != TRUE)
		return FALSE;
#if 1
	memset(SigBuf, 0, sizeof(SigBuf));
	for (Loop = 0; Loop < 10; Loop++)
	{
		
		Res = ReadFile(pdevobj->Handle, SigBuf, sizeof(SigBuf) - 1, (LPDWORD)&Cnt, NULL);
		if (Res != TRUE)
		{
			Sleep(20);
			continue;
		}
		else
		{
			if (strstr((char *)SigBuf, (const char *)"3514") != NULL)
			{
				return TRUE;
			}
		}
	}
	
	return FALSE;
#endif
	return TRUE;
}

VOID api_uart_init(HANDLE hCom)
{
	DCB commParam;
	SetupComm(hCom, 1024, 1024);
		COMMTIMEOUTS timeouts = {	// 串口超时控制参数
			1000,	 // 读字符间隔超时时间: 100 ms
			1,	 // 读操作时每字符的时间: 1 ms (n个字符总共为n ms)
			500,	 // 基本的(额外的)读超时时间: 500 ms
			100,	 // 写操作时每字符的时间: 1 ms (n个字符总共为n ms)
			500 };	 // 基本的(额外的)写超时时间: 100 ms

		
	SetCommTimeouts(hCom, &timeouts);	// 设置超时
	GetCommState(hCom, &commParam);
		
	memset(&commParam,0,sizeof(commParam));
	commParam.DCBlength=28;
	commParam.BaudRate=115200;
	
	commParam.fBinary=1;
	commParam.fDtrControl=1;
	commParam.fRtsControl=1;
	commParam.XonLim=2048;
	commParam.XoffLim=512;
	commParam.ByteSize=8;
	commParam.XonChar=17;
	commParam.XoffChar=19;
	SetCommState(hCom, &commParam);		
}
U8 Api_Disk_Detect_Uart()
{
	BOOL res;
	DISKTYPE disktype;
	HANDLE hCom;
	U8 count, com_count;
	U8 com_name[512];//="FTDIBUS\VID_0403+PID_6010+ML605-2147B\0000";
	PDEVICE_OBJECT pdevobj;
	DEVICE_OBJECT devobj;
	
	com_count = 0;
	for (count = 0; count < MAX_UART; count++)
	{
		if (count == 1)
			continue;
		/*See if the physical drive exists.*/
		memset(com_name, 0, sizeof(com_name));
		sprintf_s((char *)com_name, sizeof(com_name), (const char *)"COM%d", count);
		//sprintf_s((char *)com_name, sizeof(com_name), (const char *)"FTDIBUS\VID_0403+PID_6010+ML605-2147B\0000");
		
		/*FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED*/
		hCom = CreateFileA((LPCSTR)com_name, GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, 0);
		if (hCom == INVALID_HANDLE_VALUE)
			continue;
		disktype = UART;

		sprintf_s((char *)com_name, sizeof(com_name), (const char *)"uart");

		devobj.Handle = hCom;
		api_uart_init(hCom);
		res = Api_Get_Uart_Sig(&devobj);
		if (res == TRUE)
		{
			//GetCommState(hCom, &commParam);
			pdevobj = &g_dev_obj[com_count];
			pdevobj->Handle = hCom;
			pdevobj->FlashInfo.ulHostFlashDataAddr = (U8 *)g_flash_host_data_addr[com_count][0];
			pdevobj->FlashInfo.ulHostFlashRedAddr = g_flash_host_red_addr[com_count];
			pdevobj->FlashInfo.ulHostFlashStatusAddr = g_flash_host_status_addr[com_count];
				
			Api_Init_Dev_Function(pdevobj, (DISKTYPE)UART);
			
			printf("uart:%d name:%s\n", com_count, com_name);
			com_count++;
		}
		else
		{
			continue;
		}
	}

	return com_count;

}

