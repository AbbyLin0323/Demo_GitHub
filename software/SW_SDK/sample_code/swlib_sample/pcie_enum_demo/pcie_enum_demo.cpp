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

int pcie_enum_main(int argc, _TCHAR* argv[])
{
	PDEVICE_OBJECT pDevObj;
	U32 ulVarTableAddr;
	U8 ucDiskCnt,ch;

	while(1)
    {
          {
              DBG_printf("0:AHCI\n");
              DBG_printf("1:NVME\n");
              DBG_printf("2:SATA\n");
              DBG_printf("3:USB_BRIDGE\n");
              DBG_printf("Please Input 0 - 3 to select mode:\n");
              ch  = DBG_Getch();
              DBG_printf("input: %d\n");
              ch -= 0x30;

              if (ch>3)
              {
                  DBG_printf("Invalid Input %d\n", ch);
                  continue;
              }
              else
              {
                  
                  break;
              }
          }
          
    }
    DBG_printf("\n");
	while(1)
    {
          ucDiskCnt = Api_Disk_Detect((DISKTYPE)ch);         
          
          if (ucDiskCnt > 0)
          {
              DBG_printf("Please Input 0 - %d to select disk:\n", (ucDiskCnt-1));
              ch  = DBG_Getch();
              ch -= 0x30;

              if (ucDiskCnt <= (U8)ch)
              {
                  DBG_printf("Invalid Input %d\n", ch);
                  continue;
              }
              else
              {
                  //disk_init_testdisk(ch);
                  //io_async_init();
                  break;
              }
          }

          DBG_printf("disk detecting... \n");
          Sleep(10000);
    }



	pDevObj = &g_dev_obj[ch];



	Api_Ext_Get_Var_Table_Addr(pDevObj,&ulVarTableAddr);
	Api_Read_Dram_Sram(pDevObj,MCU0,ulVarTableAddr,HOST_BUFF_SIZE,(U8 *)&pDevObj->VarTable);

	
	return 0;
}


