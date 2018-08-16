
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



U8 Api_Disk_Detect_Usb()
{
	BOOL res;
	DISKTYPE disktype;
	HANDLE disk;

	U8 count, disk_count;
	U8 identifybuf[4096];
	U8 mn[40];
	U8 disk_name[MAX_DISK_NAME_LEN] = "";
	
	PDEVICE_OBJECT pdevobj;
	DEVICE_OBJECT devobj;
	SCSI_ADDRESS dg;
	U32 bytes_rtn;
	//DISK_GEOMETRY 
	//memset(g_disk_entry,0,sizeof(g_disk_entry));
	disk_count = 0;
	for (count = 1; count < MAX_DISK; count++)
	{
		/*See if the physical drive exists.*/
		sprintf_s((char *)disk_name, sizeof(disk_name), (const char *)"\\\\.\\PhysicalDrive%d", count);
		disk = CreateFileA((LPCSTR)disk_name, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

		
		DeviceIoControl(disk, IOCTL_SCSI_GET_ADDRESS,
			NULL, 0, &dg, sizeof(dg),
			(LPDWORD)&bytes_rtn, NULL);
		disktype = USB_BRIDGE_SATA;
		

		devobj.Handle = disk;

		res = api_disk_try_identify(&devobj, identifybuf, &disktype);
		if (res == TRUE)
		{


			Api_Get_ModelNumber(&devobj, identifybuf, (U8 *)mn);
			reverse_str((U8 *)mn, 40);

			//strstr((char *)mn,(const char *)"VT6710");
			if ((NULL != strstr((char *)mn, (const char *)"VT6710")) || (NULL != strstr((char *)mn, (const char *)"VT3514")))
			{
				pdevobj = &g_dev_obj[disk_count];
				pdevobj->Handle = disk;
				copy_str(pdevobj->mn, (U8 *)mn, 40);
				pdevobj->FlashInfo.ulHostFlashDataAddr = (U8 *)g_flash_host_data_addr[disk_count][0];
				pdevobj->FlashInfo.ulHostFlashRedAddr = g_flash_host_red_addr[disk_count];
				pdevobj->FlashInfo.ulHostFlashStatusAddr = g_flash_host_status_addr[disk_count];
				Api_Init_Dev_Function(pdevobj, (DISKTYPE)USB_BRIDGE_SATA);
				g_Rom_Detect_Obj[disk_count].Handle = (void *)pdevobj->Handle;

				printf("disk:%d type:%s  mn:%s\n", disk_count, disk_name, pdevobj->mn);
				disk_count++;
			}
		}
		else
		{
			continue;
		}
	}

	return disk_count;

}

