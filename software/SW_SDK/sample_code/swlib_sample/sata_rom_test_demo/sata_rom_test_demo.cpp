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


void sata_test_readid(U8 ucDiskIndex)
{
	void * RomHandle;
	U32 id[2];
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	//init nfc
	Api_Rom_Jump(RomHandle,0xffe03a40);
	//read id
	Api_Rom_Jump(RomHandle,0xffe012f8);

	Api_Rom_Read_Register(RomHandle,0xfff00000,&id[0]);
	Api_Rom_Read_Register(RomHandle,0xfff00004,&id[1]);
}
void sata_test_rw(U8 ucDiskIndex,U32 ulAddr,U32 ulCnt)
{
	U8 *buf;
	void * RomHandle;
	U32 i;
	U32 *pbuf;
	
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	buf = (U8 *)malloc(ulCnt);
	pbuf = (U32 *)buf;
	for(i=0;i<(ulCnt>>2);i++)
	{
		Api_Rom_Write_Register(RomHandle,ulAddr+i*4,ulAddr+i*4);
	}
	for(i=0;i<(ulCnt>>2);i++)
	{
		Api_Rom_Read_Register(RomHandle,ulAddr+i*4,pbuf);
		if(*pbuf!=ulAddr+i*4)
		{
			DBG_printf("data chk err data:0x%x correct data:0x%x\n",(*pbuf,ulAddr+i*4));
			DBG_Getch();

		}
		pbuf++;
	}

	free(buf);
	
}
void sata_rom_test()
{
	U8 ucDiskCnt,ch;
#if 1
	while(1)
    {
          {
              DBG_printf("0:AHCI\n");
              DBG_printf("1:NVME\n");
              DBG_printf("2:SATA\n");
              DBG_printf("3:USB_BRIDGE\n");
              DBG_printf("Please Input 0 - 3 to select mode:\n");
              ch  = DBG_Getch();
              
              ch -= 0x30;
			  DBG_printf("input: %d\n",ch);
              if (ch>3)
              {
                  DBG_printf("Invalid Input %d\n", ch);
                  continue;
              }
              else
              {
                  ucDiskCnt = Api_Disk_Detect_Rom((DISKTYPE)ch);
                  break;
              }
          }
          
    }
	//sata_test_rw(0,0xfff00000,1024);
	sata_test_readid(0);
	
    DBG_printf("\n");
#endif

}
int sata_rom_test_main(int argc, _TCHAR* argv[])
{
	
	sata_rom_test();
	
	
	return 0;
}


