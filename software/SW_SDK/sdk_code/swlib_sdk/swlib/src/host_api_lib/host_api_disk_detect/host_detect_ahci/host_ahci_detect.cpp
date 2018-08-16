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
  File Name     : host_pcie_detect.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the pcie enumerate function.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include <stdlib.h>
#include <tchar.h>

//#include "....//host_api/host_api.h"
#include "host_api_define.h"
#include "..\host_disk_detect.h"

REGISTER_TABLE_PCIE_INFO g_Reg_Tb_Pcie_Info;
extern DEVICE_OBJECT g_dev_obj[MAX_DISK];
#if 1
U8 Api_Disk_Detect_Ahci()
{
	BOOL res;
	DISKTYPE disktype;
	HANDLE disk;

	U8 count, disk_count, ucPcieNum;
	U8 identifybuf[4096];
	U8 mn[40];
	U8 disk_name[MAX_DISK_NAME_LEN] = "";
	PDEVICE_OBJECT pdevobj;
	DEVICE_OBJECT devobj;
	SCSI_ADDRESS dg;
	U32 bytes_rtn,pcipos;
	//DISK_GEOMETRY 
	//memset(g_disk_entry,0,sizeof(g_disk_entry));
	disk_count = 0;
	for (count = 0; count < MAX_DISK; count++)
	{
		/*See if the physical drive exists.*/
		sprintf_s((char *)disk_name, sizeof(disk_name), (const char *)"\\\\.\\PhysicalDrive%d", count);
		disk = CreateFileA((LPCSTR)disk_name, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
		if(disk==(HANDLE)(0xffffffff))
			continue;
		//type = GetDriveType((LPCWSTR)disk_name);
		//IOCTL_DISK_MEDIA_REMOVAL
		dg.Lun = 0xff;
		dg.TargetId = 0xff;
		//IOCTL_PCI
		DeviceIoControl(disk, IOCTL_SCSI_GET_ADDRESS,
			NULL, 0, &dg, sizeof(dg),
			(LPDWORD)&bytes_rtn, NULL);
		disktype = AHCI;
		sprintf_s((char *)disk_name, sizeof(disk_name), (const char *)"ahci");
		devobj.Handle = disk;
		res = api_disk_try_identify(&devobj, identifybuf, &disktype);
		if (res == TRUE)
		{
			Api_Get_ModelNumber(&devobj, identifybuf, (U8 *)mn);
			reverse_str((U8 *)mn, 40);

			//strstr((char *)mn,(const char *)"VT6710");
			if ((NULL != strstr((char *)mn, (const char *)"VT6710")) || \
				(NULL != strstr((char *)mn, (const char *)"VT3514")) || \
				(NULL != strstr((char *)mn, (const char *)gDiskName)))
			{
				pdevobj = &g_dev_obj[disk_count];
				pdevobj->Handle = disk;
				copy_str(pdevobj->mn, (U8 *)mn, 40);
				pdevobj->FlashInfo.ulHostFlashDataAddr = (U8 *)g_flash_host_data_addr[disk_count][0];
				pdevobj->FlashInfo.ulHostFlashRedAddr = g_flash_host_red_addr[disk_count];
				pdevobj->FlashInfo.ulHostFlashStatusAddr = g_flash_host_status_addr[disk_count];
				Api_Init_Dev_Function(pdevobj, (DISKTYPE)AHCI);
				Api_Write_Register(pdevobj,MCU0,0x1ff83a30,0xFFFFFFFF);
				//g_Rom_Detect_Obj[disk_count].Handle = (void *)pdevobj->Handle;

				//printf("disk:%d type:%s  mn:%s\n", disk_count, disk_name, pdevobj->mn);
				disk_count++;
			}
		}
		else
		{
			continue;
		}
	}
	Plx_Enum_Dev(&ucPcieNum);
	if(ucPcieNum>0)
	{
		for (count = 0; count < disk_count; count++)
		{
			pdevobj = &g_dev_obj[count];
			Api_Read_Register(pdevobj, MCU0, AHCI_BAR5_REG_ADDR, &pcipos);
			pdevobj->PciPos.bus = ((pcipos >> 8) & 0xff);
			pdevobj->PciPos.slot = ((pcipos >> 16) & 0xff);
			pdevobj->PciPos.fun = ((pcipos >> 24) & 0xff);
			printf("disk:%d type:%s bus:%d slot:%d fun:%d mn:%s\n",count,disk_name,pdevobj->PciPos.bus,pdevobj->PciPos.slot,pdevobj->PciPos.fun,pdevobj->mn);
		}
	}
	else
	{
		disk_count = 0;
	}
	return disk_count;
}
#endif
#if 0
ULONG ulCpOffset;
void GetPciPos(TCHAR *posBuf,U8 *ucPos)
{
	U8 i,j,k,l,n;
	//TCHAR buf[] = TEXT("@system32\drivers\pci.sys,#65536;PCI bus %1, device %2, function %3;(0,31,0)");
	TCHAR pci[256];
	TCHAR pos[3][2];
	memset(pci,0,256);
	//memset(buf,0,256);
	//buf =(TCHAR *) 
    
	k=l=0;
	memset(pos,0xff,sizeof(pos));
	for(i=0;i<256;i++)
	{
		if(posBuf[i]=='(')
		{
			memcpy(pci,&posBuf[i],64);
			for(j=0;j<64;j++)
			{
			//if(buf)
				if(pci[j]==')')
					break;
				if(pci[j]=='(')
					continue;
				if(pci[j]!=',')
				{
					pos[k][l++] = pci[j];
				}
				else
				{
					l = 0;
					k++;
				}
			}
            break;
		}
	}
	n = 0;
	for(k=0;k<3;k++)
	{
        ucPos[k] = 0;
		for(l=0;l<2;l++)
		{
			if(pos[k][l]==0xffff)
				continue;
			//n |= (atoi((char *)&pos[k][l])<<((1-l)*4));
			if(l==1)
			{
				//ucPos[k] = atoi((char *)&pos[k][l])*10;
                ucPos[k] = ucPos[k]*10+atoi((char *)&pos[k][l]);
			}
			else
			{
				ucPos[k]=atoi((char *)&pos[k][l]);
			}
		}
		//n = 0;		
	
	}
}

ULONG ScanRegistryByGUID(LPGUID pGUID, LPRegistryInfoEx pRegistryInfo)
{
	HDEVINFO                            hDevInfo;
	SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA    DeviceInterfaceDetailData;
	ULONG                               dwError = 0;
	ULONG                               ItemIndex=0;
	BOOL                                Status;
	ULONG                               requiredSize;
	SP_DEVINFO_DATA                     DeviceInfo;
	ULONG                               Index=0;

	DeviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	// get the handle of the class 
	hDevInfo = SetupDiGetClassDevs( pGUID,
		NULL,
		NULL,
		(DIGCF_PRESENT | DIGCF_INTERFACEDEVICE)
		);
	if (!hDevInfo) 
	{
		dwError = GetLastError();       
	}
	else 
	{
		while (1) 
		{
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

			if (SetupDiEnumDeviceInterfaces(hDevInfo, 0, pGUID, ItemIndex, &deviceInterfaceData)) 
			{
				// figure out the size...
				Status = SetupDiGetDeviceInterfaceDetail( hDevInfo,
					&deviceInterfaceData,
					NULL,
					0,
					&requiredSize,
					NULL
					);

				if (Status) 
				{
					dwError = GetLastError();                    
				}

				DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LPTR, requiredSize);
				DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				Status = SetupDiGetDeviceInterfaceDetail( hDevInfo,
					&deviceInterfaceData,
					DeviceInterfaceDetailData,
					requiredSize,
					NULL,
					&DeviceInfo
					);

				if (!Status) 
				{
					dwError = GetLastError();
					LocalFree(DeviceInterfaceDetailData);
					break;
				}
				else 
				{
					if(NULL!=wcsstr(DeviceInterfaceDetailData->DevicePath,TEXT("vt3514")))
					{
						//capture information that from system query
						lstrcpy(pRegistryInfo->ItemInfo[Index].SymbolicName, DeviceInterfaceDetailData->DevicePath);
						LocalFree(DeviceInterfaceDetailData);
						pRegistryInfo->ItemInfo[Index].DeviceInstance=DeviceInfo.DevInst;
						Index++;
					}
				}

			} //end if (SetupDi.....
			else 
			{
				dwError = GetLastError();
				break;
			}

			ItemIndex++;

		} //end while

		SetupDiDestroyDeviceInfoList(hDevInfo);

	} //end else
	pRegistryInfo->ItemCount=Index;
	return dwError;

}
void RegSearchIde(HKEY hKey, TCHAR rootKey[],TCHAR compStr[])
{
    
	ULONG sta,i,j,numSubKey,sizeSubKey;
    HKEY hSubKey,hSubKeyPrifix;
    
    TCHAR	cFindVal[MAX_KEY_LENGTH];
    TCHAR	subKey[MAX_KEY_LENGTH];
    
    DWORD	cbData;
	g_Reg_Tb_Pcie_Info.ucIdeCnt = 0;
    sta = RegOpenKeyEx(	hKey,
					rootKey,
					0,		
					KEY_READ,
					&hSubKey);

	if(sta!=ERROR_SUCCESS)
	{
		host_detect_printf(TEXT("open key fail rootKey:%s\n"),rootKey);
		return;
	}
    RegQueryInfoKey(	hSubKey,
							NULL,
							NULL,
							NULL,
							&numSubKey,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL);
		
		if (numSubKey)	
		{
			for (i=0; i<numSubKey; i++)
			{
				subKey[0]='\0';
				sizeSubKey=MAX_KEY_LENGTH;
				RegEnumKeyEx(	hSubKey,
								i,
								subKey,
								&sizeSubKey,
								NULL,
								NULL,
								NULL,
								NULL);

                for(j=0;j<g_Reg_Tb_Pcie_Info.ucPciCnt;j++)
				//for(j=0;j<pHostinfo->ItemCount;j++)
                {
                    if(NULL!=wcsstr(subKey,g_Reg_Tb_Pcie_Info.cParentIdPrefix[j]))
                    {
                         sta = RegOpenKeyEx(hSubKey,
					                    subKey,
					                    0,		
					                    KEY_READ,
					                    &hSubKeyPrifix);

	                    if(sta!=ERROR_SUCCESS)
	                    {
		                    host_detect_printf(TEXT("open key fail rootKey:%s\n"),rootKey);
		                    return;
	                    }
                        if (RegQueryValueEx(	hSubKeyPrifix,
							                    compStr,
							                    NULL,
							                    NULL,
							                    (LPBYTE)cFindVal,
							                    &cbData)==ERROR_SUCCESS)
	                    {
							g_Reg_Tb_Pcie_Info.Ide[g_Reg_Tb_Pcie_Info.ucIdeCnt].cPciIndexByIde = (U8)j;
							//IDE
							memcpy((void *)g_Reg_Tb_Pcie_Info.Ide[g_Reg_Tb_Pcie_Info.ucIdeCnt].cIdeParentIdPrefix,cFindVal,MAX_KEY_LENGTH);
							g_Reg_Tb_Pcie_Info.ucIdeCnt++;
                            //memcpy((void *)(dstStr+ulCpOffset),cFindVal,MAX_KEY_LENGTH);
                            //ulCpOffset+=MAX_KEY_LENGTH;
                        }
                        RegCloseKey(hSubKeyPrifix);	
                    }
                }
				
//				RegSearchPci (hKey,subKey,compStr,dstStr,pDevCnt);
			}
		}
        RegCloseKey(hSubKey);	

}
#if 0
void RegSearchPci(HKEY hKey, TCHAR rootKey[],TCHAR compStr[],TCHAR dstStr[],U8 *pDevCnt)
{
	DWORD	numSubKey=0;
	DWORD	i,cbData,cbDataEx;
	DWORD   sizeSubKey;
	TCHAR	subKey[MAX_KEY_LENGTH];
	TCHAR	cFindVal[MAX_KEY_LENGTH];
	TCHAR	cHardwareId[4096];
	TCHAR	cParent[256];
	ULONG sta,addr;

	sta = RegOpenKeyEx(	hKey,
					rootKey,
					0,		
					KEY_READ,
					&hKey);

	if(sta!=ERROR_SUCCESS)
	{
		host_detect_printf(TEXT("open key fail rootKey:%s\n"),rootKey);
		return;
	}
	host_detect_printf(TEXT("open key:  0x%x rootKey:%s\n"),hKey,rootKey);
	
	memset(cFindVal,0,sizeof(cFindVal));
	cbData = MAX_KEY_LENGTH;
	cbDataEx = 4096;
	if (RegQueryValueEx(	hKey,
							compStr,
							NULL,
							NULL,
							(LPBYTE)cFindVal,
							&cbData)==ERROR_SUCCESS)
	{
		if (RegQueryValueEx(	hKey,
							TEXT("HardwareID"),
							NULL,
							NULL,
							(LPBYTE)cHardwareId,
							&cbDataEx)==ERROR_SUCCESS)
		{
			if (RegQueryValueEx(	hKey,
							TEXT("ParentIdPrefix"),
							NULL,
							NULL,
							(LPBYTE)cParent,
							&cbDataEx)==ERROR_SUCCESS)
			{
				if(NULL!=wcsstr(cHardwareId,TEXT("DEV_3514")))
				{
					host_detect_printf(TEXT("find ParentIdPrefix:%s close key:  0x%x\n"),cFindVal,hKey);
					addr = (ULONG)(dstStr+ulCpOffset);
					memcpy((void *)addr,cFindVal,MAX_KEY_LENGTH);
					ulCpOffset+=MAX_KEY_LENGTH;
					(*pDevCnt)+=1;
				}
			}
		}
		return;
	}
	else
	{
		RegQueryInfoKey(	hKey,
							NULL,
							NULL,
							NULL,
							&numSubKey,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL);
		
		if (numSubKey)	
		{
			for (i=0; i<numSubKey; i++)
			{
				subKey[0]='\0';
				sizeSubKey=MAX_KEY_LENGTH;
				RegEnumKeyEx(	hKey,
								i,
								subKey,
								&sizeSubKey,
								NULL,
								NULL,
								NULL,
								NULL);
				
				RegSearchPci (hKey,subKey,compStr,dstStr,pDevCnt);
			}
		}
	}
	host_detect_printf(TEXT("close key:  0x%x\n"),hKey);
	RegCloseKey(hKey);						
}
#endif
void RegSearchPci(HKEY hKey, TCHAR rootKey[],U8 *pDevCnt)
{
	DWORD	numSubKey=0;
	DWORD	i,cbData,cbDataEx;
	DWORD   sizeSubKey;
	TCHAR	*dstStr;
	TCHAR	subKey[MAX_KEY_LENGTH];
	TCHAR	cLocation[MAX_KEY_LENGTH];
	TCHAR	cHardwareId[4096];
	TCHAR	cParent[256];
	ULONG sta,addr;

	sta = RegOpenKeyEx(	hKey,
					rootKey,
					0,		
					KEY_READ,
					&hKey);

	if(sta!=ERROR_SUCCESS)
	{
		host_detect_printf(TEXT("open key fail rootKey:%s\n"),rootKey);
		return;
	}
	host_detect_printf(TEXT("open key:  0x%x rootKey:%s\n"),hKey,rootKey);
	
	memset(cLocation,0,sizeof(cLocation));
	cbData = MAX_KEY_LENGTH;
	cbDataEx = 4096;
	if (RegQueryValueEx(	hKey,
							TEXT("LocationInformation"),
							NULL,
							NULL,
							(LPBYTE)cLocation,
							&cbData)==ERROR_SUCCESS)
	{
		if (RegQueryValueEx(	hKey,
							TEXT("HardwareID"),
							NULL,
							NULL,
							(LPBYTE)cHardwareId,
							&cbDataEx)==ERROR_SUCCESS)
		{
			if (RegQueryValueEx(	hKey,
							TEXT("ParentIdPrefix"),
							NULL,
							NULL,
							(LPBYTE)cParent,
							&cbDataEx)==ERROR_SUCCESS)
			{
				if(NULL!=wcsstr(cHardwareId,TEXT("DEV_3514")))
				{
					host_detect_printf(TEXT("find ParentIdPrefix:%s close key:  0x%x\n"),cFindVal,hKey);
					dstStr = g_Reg_Tb_Pcie_Info.cParentIdPrefix[0];
					addr = (ULONG)(dstStr+ulCpOffset);
					memcpy((void *)addr,cParent,MAX_KEY_LENGTH);
					dstStr = g_Reg_Tb_Pcie_Info.cLocationInfor[0];
					addr = (ULONG)(dstStr+ulCpOffset);
					memcpy((void *)addr,cLocation,MAX_KEY_LENGTH);
					ulCpOffset+=MAX_KEY_LENGTH;
					(*pDevCnt)+=1;
				}
			}
		}
		return;
	}
	else
	{
		RegQueryInfoKey(	hKey,
							NULL,
							NULL,
							NULL,
							&numSubKey,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL);
		
		if (numSubKey)	
		{
			for (i=0; i<numSubKey; i++)
			{
				subKey[0]='\0';
				sizeSubKey=MAX_KEY_LENGTH;
				RegEnumKeyEx(	hKey,
								i,
								subKey,
								&sizeSubKey,
								NULL,
								NULL,
								NULL,
								NULL);
				
				RegSearchPci (hKey,subKey,pDevCnt);
			}
		}
	}
	host_detect_printf(TEXT("close key:  0x%x\n"),hKey);
	RegCloseKey(hKey);						
}
U8 MatchPciWithStor(RegistryInfoEx * pRegistryInfo,REGISTER_TABLE_PCIE_INFO * pRegistryPci)
{
    U8 i,j,cnt;
    cnt = 0;
    for(i=0;i<pRegistryInfo->ItemCount;i++)
    {
        for(j=0;j<g_Reg_Tb_Pcie_Info.ucIdeCnt;j++)
        {
            if(NULL!=wcsstr(pRegistryInfo->ItemInfo[i].SymbolicName,pRegistryPci->Ide[j].cIdeParentIdPrefix))
            {
                memcpy(g_dev_obj[cnt].cSymbolicName,pRegistryInfo->ItemInfo[i].SymbolicName,256);
				memcpy(g_dev_obj[cnt].cPosition,pRegistryPci->cLocationInfor[pRegistryPci->Ide[j].cPciIndexByIde],256);
                cnt++;
            }
            
        }
    }
    return cnt;
}
void CreateDevObj(U8 ucDevCnt,DISKTYPE Type)
{
    U8 i;
    U8 pos[3];
    for(i=0;i<ucDevCnt;i++)
    {
        g_dev_obj[i].Handle = CreateFile(
            g_dev_obj[i].cSymbolicName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if(g_dev_obj[i].Handle==NULL)
        {
            printf("create file fail!\n");
            DBG_Getch();
        }
        Api_Init_Dev_Function(&g_dev_obj[i],Type);

        U8 rdBuf[4096];
        U8 mn[40];
        memset(rdBuf,0,sizeof(rdBuf));
        g_dev_obj[i].Hal_Get_Identify_Data(g_dev_obj[i].Handle,rdBuf);
        memset(mn,0,sizeof(mn));
        g_dev_obj[i].Hal_Get_ModelNumber(g_dev_obj[i].Handle,rdBuf,mn);
        reverse_str(mn,40);
        
        copy_str(g_dev_obj[i].mn,mn,40);
        memcpy(g_dev_obj[i].mn,mn,40);
        
        memset(pos,0,sizeof(pos));
        GetPciPos(g_dev_obj[i].cPosition,pos);
        g_dev_obj[i].PciPos.bus = pos[0];
        g_dev_obj[i].PciPos.slot = pos[1];
        g_dev_obj[i].PciPos.fun = pos[2];

		//g_Detect_Obj[i].Handle = (void *)g_dev_obj[i].Handle;
		//g_Detect_Obj[i].Rom_Reg_Read = g_dev_obj[i].Hal_Rom_Reg_Read;
        printf("dev:%d name:%s\n",i,g_dev_obj[i].mn);
        printf("position: bus%d slot%d fun%d\n",g_dev_obj[i].PciPos.bus,g_dev_obj[i].PciPos.slot,g_dev_obj[i].PciPos.fun);
    }
}
U8 Api_Disk_Detect_Ahci()
{
	U8 ucDiskCnt,ucPciCnt,ucPciPrifixCnt;
	
	RegistryInfoEx HostRegistryInfo;
	DISKTYPE Type = AHCI;
	GUID    HOSTGUID=GUID_DEVINTERFACE_DISK;//GUID_DEVINTERFACE_HOST;//GUID_DEVINTERFACE_STORAGEPORT
	ucDiskCnt = 0;
	HKEY hMainKey;
	
	if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
        NULL,
        0,
        KEY_READ,
        &hMainKey) != ERROR_SUCCESS
    )
	{
		return FALSE;
	}
	memset(&HostRegistryInfo,0,sizeof(HostRegistryInfo));
	ScanRegistryByGUID(&HOSTGUID, &HostRegistryInfo);
	
	ulCpOffset = 0;
    ucPciPrifixCnt = 0;
	RegSearchPci(hMainKey,TEXT("SYSTEM\\CONTROLSET001\\ENUM\\PCI"),&ucPciPrifixCnt);
#if 0
    ulCpOffset = 0;
    ucPciLocationCnt = 0;
	RegSearchPci(hMainKey,TEXT("SYSTEM\\CONTROLSET001\\ENUM\\PCI"),TEXT("LocationInformation"),g_Reg_Tb_Pcie_Info.cLocationInfor[0],&ucPciLocationCnt);

    if(ucPciPrifixCnt!=ucPciLocationCnt)
    {
        printf("pci search err ucPciPrifixCnt:%d ucPciLocationCnt:%d\n",ucPciPrifixCnt,ucPciLocationCnt);
    }
#endif
    ucPciCnt = ucPciPrifixCnt;
    g_Reg_Tb_Pcie_Info.ucPciCnt = ucPciCnt;

    ulCpOffset = 0;
    RegSearchIde(hMainKey,TEXT("SYSTEM\\CONTROLSET001\\ENUM\\PCIIDE\\IDEChannel"),TEXT("ParentIdPrefix"));//,g_Reg_Tb_Pcie_Info.cIdeParentIdPrefix[0]);
    ucDiskCnt = MatchPciWithStor(&HostRegistryInfo,&g_Reg_Tb_Pcie_Info);

    CreateDevObj(ucDiskCnt,Type);
	return ucDiskCnt;
}
#endif

