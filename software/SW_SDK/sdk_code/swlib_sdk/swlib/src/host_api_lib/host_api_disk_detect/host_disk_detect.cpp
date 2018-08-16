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
  File Name     : host_disk_detect.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the basic function of host enumeration.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include <stdlib.h>
#include <tchar.h>
#include "host_disk_detect.h"
Detect_Obj g_Rom_Detect_Obj[MAX_DISK];
U8 gDiskName[256];
BOOL api_disk_try_identify(PDEVICE_OBJECT pdevobj,U8 * identifybuf,DISKTYPE *type)
{
	
	Api_Init_Dev_Function(pdevobj,(DISKTYPE)*type);
	if(RETURN_SUCCESS == Api_Get_Identify_Data(pdevobj,identifybuf))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
void reverse_str(U8* pStr, U8 count)
{
    U8 ucLoop;
    U8 Temp;
  
    for (ucLoop = 0; ucLoop < count; ucLoop+=2)
    {
        Temp = pStr[ucLoop];
        pStr[ucLoop]   = pStr[ucLoop+1];
        pStr[ucLoop+1] = Temp;
    }

    return;
}
void copy_str(U8* pDstStr, U8* pSrcStr, U8 count)
{

    U8 ucLoop;
  
    for (ucLoop = 0; ucLoop < count; ucLoop++)
    {
        pDstStr[ucLoop] = pSrcStr[ucLoop];
    }

   // pDstStr[count] = 0;

    return;
}
U8 Api_Disk_Detect_Rom(DISKTYPE ucMode)
{
	U8 ucDiskCnt,i;
	switch (ucMode)
	{
		case AHCI:
        case NVME:
			Plx_Enum_Dev(&ucDiskCnt);
			for(i=0;i<ucDiskCnt;i++)
			{
				memset(&g_Rom_Detect_Obj[i],0,sizeof(Detect_Obj));
				Plx_Get_Pcie_Obj(i,&g_Rom_Detect_Obj[i].Handle);
				g_Rom_Detect_Obj[i].Rom_Reg_Read = Plx_Read_Register_Entry;
				g_Rom_Detect_Obj[i].Rom_Reg_Write = Plx_Write_Register_Entry;
				g_Rom_Detect_Obj[i].Rom_Write_Dma = Plx_Write_Dma_Entry;
				g_Rom_Detect_Obj[i].Rom_Jump = Plx_Jump_Entry;

			}
            break;
		case SATA:
		    ucDiskCnt = Api_Disk_Detect_Sata();
			for(i=0;i<ucDiskCnt;i++)
			{
				memset(&g_Rom_Detect_Obj[i],0,sizeof(Detect_Obj));
				g_Rom_Detect_Obj[i].Handle = g_dev_obj[i].Handle;
				g_Rom_Detect_Obj[i].Rom_Reg_Read = Ahci_Rom_Read_Reg;
				g_Rom_Detect_Obj[i].Rom_Reg_Write = Ahci_Rom_Write_Reg;
				g_Rom_Detect_Obj[i].Rom_Write_Dma = Ahci_Rom_Write_Dma;
				g_Rom_Detect_Obj[i].Rom_Jump = Ahci_Rom_Jump;
				

			}
            break;
		case UART:
			ucDiskCnt = Api_Disk_Detect_Uart();
			for (i = 0; i<ucDiskCnt; i++)
			{
				memset(&g_Rom_Detect_Obj[i],0,sizeof(Detect_Obj));
				g_Rom_Detect_Obj[i].Handle = g_dev_obj[i].Handle;
				g_Rom_Detect_Obj[i].Rom_Reg_Read = Uart_Rom_Read_Reg;
				g_Rom_Detect_Obj[i].Rom_Reg_Write = Uart_Rom_Write_Reg;
				g_Rom_Detect_Obj[i].Rom_Jump = Uart_Rom_Jump;
				g_Rom_Detect_Obj[i].Rom_Read_Stat = Uart_Rom_Read_Status;
				g_Rom_Detect_Obj[i].Rom_Write_Dma = Uart_Rom_Write_Dma;
				g_Rom_Detect_Obj[i].Rom_Set_Trans_Speed = Uart_Set_Trans_Speed;
				g_Rom_Detect_Obj[i].Rom_Close_Handle = Uart_Rom_Close_Handle;
				g_Rom_Detect_Obj[i].Rom_Set_Uart_Baud = Uart_Rom_Set_Uart_Baudrate;

			}
			
			break;
        case USB_BRIDGE_SATA:
			break;
		default:
			break;
	}
	if (ucDiskCnt == 0)
	{
		printf("no disk find!!\n");
		DBG_Getch();
	}
	return ucDiskCnt;
}

VOID Api_Disk_Name()
{
	
	U8 ucLen;
	DBG_printf("Please Input disk name(disk full name or partial)\n");
	
	memset(gDiskName, 0, sizeof(gDiskName));
	scanf("%s",gDiskName);
	ucLen = strlen((const char *)gDiskName);
	
	return;
}
U8 Api_Disk_Detect(DISKTYPE ucMode)
{
    U8 ucDiskCnt;
	switch (ucMode)
	{
		case AHCI:
			ucDiskCnt = Api_Disk_Detect_Ahci();
			break;
        case NVME:
		    ucDiskCnt = Api_Disk_Detect_Nvme();
            break;
		case SATA:
		    ucDiskCnt = Api_Disk_Detect_Sata();
            break;
        case UART:
			ucDiskCnt = Api_Disk_Detect_Uart();
            break;
		case USB_BRIDGE_SATA:
			ucDiskCnt = Api_Disk_Detect_Usb();
			break;
		default:
			break;
	}
	if (ucDiskCnt == 0)
	{
		printf("no disk find!!\n");
		DBG_Getch();
	}
	return ucDiskCnt;
}
