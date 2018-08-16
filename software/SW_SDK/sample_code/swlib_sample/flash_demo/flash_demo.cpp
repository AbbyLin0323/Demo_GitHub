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
  File Name     : flash_demo.cpp
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the flash demo function.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\include\sw_lib.h"
void Flash_ShiftRead_Test(PDEVICE_OBJECT pDevObj)
{
	U8 ucPuCnt,ucPu;
	U8 usPlnMode = 0xff;
	U16 usBlk, usPage;
	U32 ulPuMskLow, ulPuMskHigh, ulPuMskStatusLow, ulPuMskStatusHigh;

	usBlk = 128;
	usPage = 0;
	Api_Ext_Get_Pu_Msk(pDevObj, &ulPuMskLow, &ulPuMskHigh, &ucPuCnt);
	Api_Flash_PreCondiTion(pDevObj, ulPuMskLow, ulPuMskHigh);
	Api_Flash_SetParam(pDevObj, ulPuMskLow, ulPuMskHigh,1);

	Api_Read_Flash(pDevObj, usPlnMode, usBlk, usPage, ulPuMskLow, 0);
	Api_Flash_Get_Cmd_Status(pDevObj, ulPuMskLow, ulPuMskHigh,&ulPuMskStatusLow, &ulPuMskStatusHigh);
	for (ucPu = 0; ucPu < ucPuCnt; ucPu++)
	{
		if ((ulPuMskLow&(1 << ucPu)) != (ulPuMskStatusLow&(1 << ucPu)))
		{
			DBG_printf("pu:%d blk:%d pg:%d rd err!\n", ucPu, usBlk, usPage);
		}
	}

	Api_Flash_Terminate(pDevObj, ulPuMskLow, ulPuMskHigh);
	Api_Flash_Reset(pDevObj, ulPuMskLow, ulPuMskHigh);

}
void Flash_Interface_Test(PDEVICE_OBJECT pDevObj)
{
	U8 ucPu;
	U8 ucPuCnt;
	U8 * pDataBuf; 
	U8 * pReadBuf; 
	U16 usBlk,usStartBlk,usBlkCnt,usPage;
	U32 ulPuMsk,ulPuMskStatus; 
	RED Red;
	RED *pRed = NULL;
	ucPuCnt = 8;
	ulPuMsk = ((1<<ucPuCnt)-1);
	U32 ulPuMskError = 0xfa;
	usStartBlk = 700;
	usBlkCnt = 1;
	
	pDataBuf = (U8 *)malloc(pDevObj->VarTable.SubSysTable[0].ucPhyPageSize*pDevObj->VarTable.SubSysTable[0].ucPlnNum);
	U32 ulCount;
	U8 usPlnMode = 0xff;

	//erase
	for(usBlk=usStartBlk;usBlk<(usStartBlk+usBlkCnt);usBlk++)
	{
		Api_Erase_Flash(pDevObj,usPlnMode,usBlk,ulPuMskError,0);
		ulPuMskStatus = 0;
		Api_Flash_Get_Cmd_Status(pDevObj,ulPuMskError,0,&ulPuMskStatus,0);
		if(ulPuMskError!=ulPuMskStatus)
		{
			for(ucPu=0;ucPu<ucPuCnt;ucPu++)
			{
				if((ulPuMskError&(1<<ucPu))!=(ulPuMskStatus&(1<<ucPu)))
					DBG_printf("pu:%d blk:%d ers err!\n",ucPu,usBlk);
			}
			
		}
	}
	//prepare wt data & red to dev
	for(ucPu=0;ucPu<ucPuCnt;ucPu++)
	{
		memset(pDataBuf, ucPu, pDevObj->VarTable.SubSysTable[0].ucPhyPageSize*pDevObj->VarTable.SubSysTable[0].ucPlnNum);
		Api_Flash_Set_Data_To_Pu(pDevObj,ucPu,usPlnMode,pDataBuf);
		memset(&Red,ucPu,sizeof(Red));
		Api_Flash_Set_Red_To_Pu(pDevObj,ucPu,usPlnMode,&Red);
	}

	//write
	for(usBlk=usStartBlk;usBlk<(usStartBlk+usBlkCnt);usBlk++)
	{
		for(usPage=0;usPage<512;usPage++)
		{
			Api_Write_Flash(pDevObj,usPlnMode,usBlk,usPage,ulPuMskError,0);
			ulPuMskStatus = 0;
			Api_Flash_Get_Cmd_Status(pDevObj,ulPuMskError,0,&ulPuMskStatus,0);
			if(ulPuMskError!=ulPuMskStatus)
			{
				for(ucPu=0;ucPu<ucPuCnt;ucPu++)
				{
					if((ulPuMskError&(1<<ucPu))!=(ulPuMskStatus&(1<<ucPu)))
						DBG_printf("pu:%d blk:%d pg:%d wt err!\n",ucPu,usBlk,usPage);
				}
			
			}
		}
	}

	//read & check
	for(usBlk=usStartBlk;usBlk<(usStartBlk+usBlkCnt);usBlk++)
	{
		for(usPage=0;usPage<512;usPage++)
		{
			Api_Read_Flash(pDevObj,usPlnMode,usBlk,usPage,ulPuMsk,0);
			ulPuMskStatus = 0;
			Api_Flash_Get_Cmd_Status(pDevObj,ulPuMsk,0,&ulPuMskStatus,0);
			
			for(ucPu=0;ucPu<ucPuCnt;ucPu++)
			{
				if((ulPuMsk&(1<<ucPu))!=(ulPuMskStatus&(1<<ucPu)))
				{
					DBG_printf("pu:%d blk:%d pg:%d rd err!\n",ucPu,usBlk,usPage);
				}
				else
				{
					pReadBuf = NULL;
					Api_Flash_Get_Data_Addr_By_Pu(pDevObj,ucPu,usPlnMode,&pReadBuf);
					Api_Flash_Get_Red_Addr_By_Pu(pDevObj,ucPu,usPlnMode,&pRed);
					//data check
					for(ulCount = 0;ulCount < PHY_PG_SZ;ulCount++)
					{
						if (pReadBuf[ulCount] != ucPu)
						{
							printf("data check error!\n");
							//DBG_Getch();
						}
					}
					for(ulCount = 0;ulCount < 64;ulCount++)
					{
						if ((pRed->m_content[0][ulCount] != ucPu) || (pRed->m_content[1][ulCount] != ucPu))
						{
							printf("red check error!\n");
							//DBG_Getch();
						}
					}
				}
			}
			
		}
	}

	free(pDataBuf);
	return;
}
void TestFunctionSRAM(PDEVICE_OBJECT pDevObj)
{
	U8 ucCycle;
	U32 ulAlignNonOverTestAddr = 0x44f20000;
	U32 ulAlignNonOverTestAddrNext = 0xfff0f404;
	U32 ulNotAlignNonOverTestAddr = 0x44f20000;
	U32 ulNotAlignOverTestAddr = 0xfff0f403;
	U32 ulReadData = 0;
	U8 *pReadData = (U8 *)&ulReadData;
	CPUID MCUNumWrite = MCU1;
	CPUID MCUNumRead = MCU2;
	U8 ucTestWriteDataMCU1[4] = {1};
	U8 ucTestWriteDataMCU2[4] = {2};

	//SRAM
	//align not over
	for(ucCycle = 1; ucCycle <= 2; ucCycle++)
	{
		Api_Ext_Mem_Write(pDevObj,MCUNumWrite,ulAlignNonOverTestAddr,ucCycle,ucTestWriteDataMCU1);
		Api_Ext_Mem_Write(pDevObj,MCUNumRead,ulAlignNonOverTestAddr,ucCycle,ucTestWriteDataMCU2);
		Api_Ext_Mem_Read(pDevObj,MCUNumWrite,ulAlignNonOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Read(pDevObj,MCUNumRead,ulAlignNonOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:0x%x\n",ulReadData);
		ulReadData = 0;
	}
	//not-align not over
	for(ucCycle = 1; ucCycle <= 2; ucCycle++)
	{
		Api_Ext_Mem_Write(pDevObj,MCUNumWrite,ulNotAlignNonOverTestAddr,ucCycle,ucTestWriteDataMCU1);
		Api_Ext_Mem_Write(pDevObj,MCUNumRead,ulNotAlignNonOverTestAddr,ucCycle,ucTestWriteDataMCU2);
		Api_Ext_Mem_Read(pDevObj,MCUNumWrite,ulNotAlignNonOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Read(pDevObj,MCUNumRead,ulNotAlignNonOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:0x%x\n",ulReadData);
		ulReadData = 0;
	}

	return;
}
void TestFunctionDRAM_OTFB(PDEVICE_OBJECT pDevObj)
{
	U8 ucCycle;
	U8 ucCount;

	U32 ulAlignNonOverTestAddr = 0x44f20000;
	U32 ulAlignNonOverTestAddrNext = 0xfff0f404;
	U32 ulNotAlignNonOverTestAddr = 0x44f20000;
	U32 ulNotAlignOverTestAddr = 0xfff0f403;
	U32 ulRawData = 0xffffffff;
	U32 ulReadData = 0;
	U8 *pReadData = (U8 *)&ulReadData;
	U8 ucTestWriteData[4] = {0};
	U8 ucDMARawData[32] = {0xff};
	U8 ucDMATestWriteData[32] = {0};
	U8 ucDMATestReadData[32*1024] = {0};
	CPUID MCUNum = MCU0;
	
	//DRAM OTFB
	//non-data
	//align not over
	for(ucCycle = 1; ucCycle <= 4; ucCycle++)
	{
		Api_Write_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,ulRawData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,&ulReadData);
		printf("RawData:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Write(pDevObj,MCUNum,ulAlignNonOverTestAddr,ucCycle,ucTestWriteData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,&ulReadData);
		printf("Read all the data:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Read(pDevObj,MCUNum,ulAlignNonOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:");
		for(ucCount = 0; ucCount < ucCycle; ucCount++)
		{
			printf("0x%x",pReadData[ucCount]);
		}
		printf("\n");
		ulReadData = 0;
	}
	//not-align not over
	for(ucCycle = 1; ucCycle <= 3; ucCycle++)
	{
		Api_Write_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,ulRawData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,&ulReadData);
		printf("RawData:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Write(pDevObj,MCUNum,ulNotAlignNonOverTestAddr,ucCycle,ucTestWriteData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,&ulReadData);
		printf("Read all the data:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Read(pDevObj,MCUNum,ulNotAlignNonOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:");
		for(ucCount = 0; ucCount < ucCycle; ucCount++)
		{
			printf("0x%x",pReadData[ucCount]);
		}
		printf("\n");
		ulReadData = 0;
	}
	//not-align over
	for(ucCycle = 2; ucCycle <= 4; ucCycle++)
	{
		Api_Write_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,ulRawData);
		Api_Write_Register(pDevObj,MCUNum,ulAlignNonOverTestAddrNext,ulRawData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,&ulReadData);
		printf("RawData:0x%x\n",ulReadData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddrNext,&ulReadData);
		printf("RawData next:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Write(pDevObj,MCUNum,ulNotAlignOverTestAddr,ucCycle,ucTestWriteData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddr,&ulReadData);
		printf("Read all the data:0x%x\n",ulReadData);
		Api_Read_Register(pDevObj,MCUNum,ulAlignNonOverTestAddrNext,&ulReadData);
		printf("Read all the data next:0x%x\n",ulReadData);
		ulReadData = 0;
		Api_Ext_Mem_Read(pDevObj,MCUNum,ulNotAlignOverTestAddr,ucCycle,pReadData);
		printf("Read writen data:");
		for(ucCount = 0; ucCount < ucCycle; ucCount++)
		{
			printf("0x%x",pReadData[ucCount]);
		}
		printf("\n");
		ulReadData = 0;
	}
	//DMA length
	for(ucCycle = 5; ucCycle <= 32; ucCycle++)
	{
		Api_Write_Dram_Sram(pDevObj,MCUNum,ulNotAlignNonOverTestAddr,32*1024,ucDMARawData);
		Api_Ext_Mem_Write(pDevObj,MCUNum,ulNotAlignNonOverTestAddr,32*1024,ucDMATestWriteData);
		Api_Ext_Mem_Read(pDevObj,MCUNum,ulNotAlignNonOverTestAddr,32*1024,ucDMATestReadData);
		//check
		for(ucCount = 0; ucCount < ucCycle; ucCount++)
		{
			if(ucDMATestReadData[ucCount] != 0)
			{
				printf("data check error\n");
				DBG_Getch();
			}
		}
	}
	ucCount = 0;

	return;
}

int flash_main(int argc, _TCHAR* argv[])
{
	PDEVICE_OBJECT pDevObj;
	U32 ulVarTableAddr;
	U8 ucDiskCnt,ch;
	//ucDiskCnt = Api_Disk_Detect();
	//PlxApiTest();
	while(1)
    {
          ucDiskCnt = Api_Disk_Detect(SATA);         
          
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

	//TestFunctionDRAM_OTFB(pDevObj);
	//TestFunctionSRAM(pDevObj);
	Api_Init_FlashInfo(pDevObj);
	Flash_Interface_Test(pDevObj);
	

	return 0;
}


