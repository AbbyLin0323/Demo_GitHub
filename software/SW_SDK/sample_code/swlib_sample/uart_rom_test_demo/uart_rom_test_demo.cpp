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
  File Name     : pcie_enum_demo.cpp
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines demo of pcie enumeration.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/
#include <tchar.h>
#include "..\include\sw_lib.h"
void rom_test_rw_time(U8 ucDiskIndex, U32 ulAddr)
{
	U32 buf;
	void * RomHandle;
	LARGE_INTEGER t1,t2,t3,freq;
	
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	QueryPerformanceFrequency(&freq);
	//while(1)
	{
	QueryPerformanceCounter(&t1);
	//t1 = GetTickCount();
	Api_Rom_Write_Register(RomHandle, ulAddr, ulAddr);
	
	QueryPerformanceCounter(&t2);
	//t2 = GetTickCount();
	Api_Rom_Read_Register(RomHandle, ulAddr, &buf);
	
	QueryPerformanceCounter(&t3);
	//t3 = GetTickCount();
	
	//printf("wt tick:%f\n",(float)(t2.QuadPart-t1.QuadPart)/freq.QuadPart*1000);
	//printf("rd tick:%f\n",(float)(t3.QuadPart-t2.QuadPart)/freq.QuadPart*1000);
	}
	printf("done\n");

}
void rom_test_rw(U8 ucDiskIndex, U32 ulAddr, U32 ulCnt)
{
	U8 *buf;
	void * RomHandle;
	U32 i;

	U32 *pbuf;
	
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	buf = (U8 *)malloc(ulCnt);
	pbuf = (U32 *)buf;
	for (i = 0; i<(ulCnt >> 2); i++)
	{
		
		Api_Rom_Write_Register(RomHandle, ulAddr + i * 4, ulAddr + i * 4);
		
		*pbuf++=(ulAddr + i * 4);

	}
	
	
	free(buf);

}

void uart_rom_test()
{
	U8 ucDiskCnt;
	U32 ulCnt;
	U32 i;
    ucDiskCnt = Api_Disk_Detect_Rom((DISKTYPE)UART);
    //rom_test_rw_time(0,0xfff00000);
	ulCnt = 16*1024;
	i=0;
	for(;;)
	{
		rom_test_rw(0,0xfff00000,ulCnt);
		DBG_printf("trans 0x%x\n",ulCnt*i);
		i++;
		//ulCnt+=4;
		//if(ulCnt>0xfff00100)
			//ulCnt = 64*1024;
	}
	//sata_test_readid(0);
	
    DBG_printf("\n");

}
int uart_rom_test_main(int argc, _TCHAR* argv[])
{
	
	uart_rom_test();
	
	
	return 0;
}


