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
  File Name     : swlib_sample.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the basic function of demo.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include <tchar.h>
#include ".\include\sw_lib.h"
//#define DEMO_SATA_ROM_TEST
//#define DEMO_UART_ROM
//#define DEMO_MPT
//#define DEMO_PLX
//#define DEMO_FW_UPDATE
#define DEMO_LOGTRACE
//#define DEMO_FLASH
//#define DEMO_MPTOOL
//#define DEMO_PCIE_ENUM
int _tmain(int argc, _TCHAR* argv[])
{
	HINSTANCE hInstLibrary;   
	//system("mode com1 baud=115200");
	hInstLibrary = LoadLibrary(_TEXT(".\\PlxApi650.dll"));
	if(hInstLibrary==NULL)
	{
		DBG_printf("load dll err!!\n");
		DBG_Getch();
	}
#if 0
	hInstLibrary = LoadLibrary(_TEXT(".\\FTCSPI.dll"));
	if(hInstLibrary==NULL)
	{
		DBG_printf("load jtag dll err!!\n");
		DBG_Getch();
	}
	hInstLibrary = LoadLibrary(_TEXT(".\\FTD2XX.dll"));
	if(hInstLibrary==NULL)
	{
		DBG_printf("load ftd2xx dll err!!\n");
		DBG_Getch();
	}
#endif
#ifdef DEMO_FW_UPDATE
	fw_update_main(argc,argv);
#endif
#ifdef DEMO_UART_ROM
	uart_rom_test_main(argc, argv);
#endif
#ifdef DEMO_SATA_ROM_TEST
	sata_rom_test_main(argc,argv);
#endif
#ifdef DEMO_MPT
	mpt_main(argc,argv);
#endif
#ifdef DEMO_LOGTRACE
	logtrace_main(argc,argv);
#endif
#ifdef DEMO_PLX
	plx_main(argc,argv);
#endif
#ifdef DEMO_FLASH
	flash_main(argc,argv);
#endif
#ifdef DEMO_PCIE_ENUM
	pcie_enum_main(argc,argv);
#endif
	return 0;
}


