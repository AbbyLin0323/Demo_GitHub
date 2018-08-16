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
  File Name     : plx_demo.cpp
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines plx function of accessing rom.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/
#include <tchar.h>
#include "..\include\sw_lib.h"

VOID PlxTestRW(PPCIE_DEVICE pDev)
{
	U32 ulAddr,ulData,ulCnt;
	U32 ulChkData;
	//pDev->DevObj.
	ulAddr = 0xfff08000;
	for(ulCnt=0;ulCnt<(96*1024/4);ulCnt++)
	{
		ulData=ulAddr;
		Plx_Write_Register(pDev,ulAddr,ulData);
		ulChkData = 0xff;
		Plx_Read_Register(pDev,ulAddr,&ulChkData);
		if(ulChkData!=ulAddr)
		{
			printf("check data err data:0x%x addr:0x%x\n",ulChkData,ulAddr);
		}
		ulAddr+=4;
		
	}
}
VOID PlxTestJump(PPCIE_DEVICE pDev,U32 ulAddr)
{
	Plx_Jump(pDev,ulAddr);
}
void PlxApiTest()
{
	U8 ucDevNum;
	//system("..\\plx_api_test\\PlxInstall.bat remove");
	//system("..\\plx_api_test\\PlxInstall.bat install");
	Plx_Enum_Dev(&ucDevNum);
	PlxTestJump(&g_Pcie_Dev[0], 0x20003c04);
	PlxTestRW(&g_Pcie_Dev[0]);
}




void PlxPowerTestFast()
{
	U32 powercycle;
	void * EpHandle;
	void * UpEpHandle;
	U8 pEpNum;
	U32 RegValue,delaytime;
	delaytime = 2000;

	Plx_Enum_Bridge_Ep(&pEpNum);
	Plx_Get_Bridge_Ep_Handle(2,&EpHandle);
	
	jtag_power_control_init();
	//Plx_Disable_Int_Fast(EpHandle);
	Plx_Get_Bridge_Ep_Handle(2,&UpEpHandle);
	//Plx_Enable_Int_Fast(UpEpHandle);
	Plx_Disable_Int_Fast(UpEpHandle);
	//system("devmgmt.msc");
	powercycle = 0;
	while(1)
	{
		DBG_printf("power cycle:%d\n",powercycle++);
		Plx_Power_Status(EpHandle);
		Plx_Read_Fast(EpHandle,&RegValue);
        if (RegValue & POWER_CONTROLLER_CONTROL)	
		//if(!Plx_Power_Status(EpHandle))
		{
			DBG_printf("power on\n");
			Plx_Power_On_Fast(EpHandle,TRUE);
#if 1
			DBG_printf("Wait for dev bootup\n");
			Sleep(delaytime);
			DBG_printf("Scan Hw\n");
			Plx_Rescan_Hw();
#endif
		}
		else
		{
			DBG_printf("power off\n");
			Plx_Power_Off_Fast(EpHandle,TRUE);
			//Plx_Rescan_Hw();
#if 1
			DBG_printf("Wait for dev shutdown\n");
			Sleep(delaytime);
			DBG_printf("Scan Hw\n");
			Plx_Rescan_Hw();
#endif

		}
		Sleep(delaytime);
#if 1
		
#endif
		
	}
	
}
int plx_main(int argc, _TCHAR* argv[])
{
	
	//PlxPowerTest();
	PlxPowerTestFast();

	return 0;
}


