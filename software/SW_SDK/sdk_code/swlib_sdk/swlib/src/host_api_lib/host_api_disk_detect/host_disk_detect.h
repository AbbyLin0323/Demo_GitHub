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
  Description   : Defines the data structure and macro for disk detection.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _HOST_DISK_DETECT
#define _HOST_DISK_DETECT
#include "host_api_define.h"
//#include ".\ahci\hal_ahci.h"
#include "..\host_api\host_api.h"
#include <setupapi.h>
#define	host_detect_printf(x, ...)	//_tprintf(x, __VA_ARGS__)
#define AHCI_BAR5_REG_ADDR (0x1ff83500+0x20)
#define NVME_BAR3_REG_ADDR (0x1ff83200+0x18)
#define MAX_VALUE_LENGTH 256
#define MAX_KEY_LENGTH 256
#define GUID_DEVINTERFACE_HOST {0xF18A0E88L, 0xC30C, 0x11D0, 0x88, 0x15, 0x00, 0xA0, 0xC9, 0x06, 0xBE, 0xD8}
#define GUID_DEVINTERFACE_DISK   {0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}
typedef struct _IDE
{
	U8		 cPciIndexByIde;
	TCHAR    cIdeParentIdPrefix[MAX_VALUE_LENGTH];
}IDE;
typedef struct _REGISTER_TABLE_PCIE_INFO{
    U8       ucPciCnt;
	U8		 ucIdeCnt;
	TCHAR    cLocationInfor[MAX_DISK][MAX_VALUE_LENGTH];
	TCHAR    cParentIdPrefix[MAX_DISK][MAX_VALUE_LENGTH];
    IDE		 Ide[MAX_DISK];
	TCHAR    cService[MAX_DISK][MAX_VALUE_LENGTH];
}REGISTER_TABLE_PCIE_INFO;
typedef struct _RegistryInfo
{
	TCHAR	SymbolicName[256];
	DWORD   DeviceInstance;
}RegistryInfo, *LPRegistryInfo;

typedef struct _RegistryInfoEx
{
	ULONG ItemCount;
	RegistryInfo ItemInfo[MAX_DISK+16];
}RegistryInfoEx, *LPRegistryInfoEx;
typedef BOOL (*Rom_Read_Register)(void * hDevice,U32 ulRegAddr,U32 * pRegData);
typedef BOOL (*Rom_Write_Register)(void * hDevice, U32 ulRegAddr, U32 ulRegData);
typedef BOOL (*Rom_Jump)(void * hDevice, U32 ulAddr);

typedef BOOL (*Rom_Read_Status)(void * hDevice);
typedef BOOL (*Rom_Write_Dma)(void * hDevice, U32 ulAddr, U32 * pBuf,U32 ulCnt);
typedef BOOL (*Rom_Set_Trans_Speed)(void * hDevice,U32 ulSpeed);
typedef BOOL(*Rom_Set_Uart_Baud)(HANDLE hDevice, U32 ulSpeedDev, U32 ulSpeedHost);
typedef BOOL (*Rom_Close_Handle)(HANDLE hDevice);
typedef struct _Detect_Obj
{
	void * Handle;
	Rom_Read_Register Rom_Reg_Read;
	Rom_Write_Register Rom_Reg_Write;
	Rom_Jump Rom_Jump;
	Rom_Read_Status Rom_Read_Stat;
	Rom_Write_Dma Rom_Write_Dma;
	Rom_Set_Trans_Speed Rom_Set_Trans_Speed;
	Rom_Set_Uart_Baud Rom_Set_Uart_Baud;
	Rom_Close_Handle Rom_Close_Handle;
}Detect_Obj,*PDetect_Obj;
VOID  Plx_Enum_Dev(unsigned char * pDevNum);
//U8 Api_Disk_Detect_Pcie(DISKTYPE Type);
U8 Api_Disk_Detect_Ahci();
U8 Api_Disk_Detect_Nvme();
U8 Api_Disk_Detect_Sata();
U8 Api_Disk_Detect_Uart();
U8 Api_Disk_Detect_Usb();
BOOL Ahci_Rom_Read_Reg(HANDLE hDevice,U32 ulAddr,U32 * pData);
BOOL Ahci_Rom_Write_Reg(HANDLE hDevice, U32 ulAddr, U32 ulData);
BOOL Ahci_Rom_Write_Dma(void * pDevObj, U32 ulAddr, U32 * pBuf,U32 ulCnt);
BOOL Ahci_Rom_Jump(HANDLE hDevice, U32 ulAddr);
BOOL Uart_Rom_Read_Reg(HANDLE hDevice, U32 ulAddr, U32 * pData);
BOOL Uart_Rom_Write_Reg(HANDLE hDevice, U32 ulAddr, U32 ulData);
BOOL Uart_Set_Trans_Speed(HANDLE hDevice,U32 ulSpeed);
BOOL Uart_Rom_Read_Status(HANDLE hDevice);
BOOL Uart_Rom_Write_Dma(HANDLE hDevice, U32 ulAddr, U32 * pBuf,U32 ulCnt);
BOOL Uart_Rom_Close_Handle(HANDLE hDevice);
BOOL Uart_Rom_Set_Uart_Baudrate(HANDLE hDevice, U32 ulSpeedDev, U32 ulSpeedHost);
BOOL Uart_Rom_Jump(HANDLE hDevice, U32 ulAddr);
BOOL Plx_Jump_Entry(void * pDevObj, U32 ulAddr);
void reverse_str(U8* pStr, U8 count);
void copy_str(U8* pDstStr, U8* pSrcStr, U8 count);
BOOL api_disk_try_identify(PDEVICE_OBJECT pdevobj,U8 * identifybuf,DISKTYPE *type);
BOOL Plx_Read_Register_Entry(void * pDevObj, U32 ulRegAddr, U32 *pRegData);
BOOL Plx_Write_Register_Entry(void * pDevObj, U32 ulRegAddr, U32 ulRegData);
BOOL Plx_Write_Dma_Entry(void * pDevObj, U32 ulAddr, U32 * pBuf,U32 ulCnt);
VOID Plx_Get_Pcie_Obj(U8 ucDevIndex,void ** ppDevObj);
extern Detect_Obj g_Rom_Detect_Obj[MAX_DISK];
extern U8 gDiskName[256];
#endif