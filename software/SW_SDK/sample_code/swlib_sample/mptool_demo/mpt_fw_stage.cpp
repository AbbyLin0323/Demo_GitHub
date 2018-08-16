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
#include "mpt_demo.h"

BOOL mpt_fw_chk_bbt(PDEVICE_OBJECT pDevObj,U8 ucSubSys,U8 ucPu,U8 ucPln,U16 ucBlk)
{
	U32 usBlkCnt,usPlnCnt;
	U8 * pBbt;
	U8 * pBytePos;
	usBlkCnt = pDevObj->VarTable.SubSysTable[0].ucBlkNum;
	usPlnCnt = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
	

	pBbt = &pDevObj->BbtBuf[ucSubSys][0];
	pBytePos = pBbt+ucPu*(usPlnCnt)*(usBlkCnt/8)+ucPln*(usBlkCnt/8)+(ucBlk/8);

	 if((*(volatile U8 *)pBytePos)&(1<<(ucBlk%8)))
		 return TRUE;
	 return FALSE;
}
void  fw_print_pu_bbt(PDEVICE_OBJECT pDevObj,U8 ucSubSys,U8 ucSubSyspu)
{
    U16 blk,usBlkCnt;
    U8 pln,usPlnCnt;
    usBlkCnt = pDevObj->VarTable.SubSysTable[0].ucBlkNum;
	usPlnCnt = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
    for(pln=0;pln<usPlnCnt;pln++)
    {
        DBG_printf("subsys:%d pu:%d pln:%d bad blk:\n",ucSubSys,ucSubSyspu,pln);
        for(blk=0;blk<usBlkCnt;blk++)
        {
			if(mpt_fw_chk_bbt(pDevObj,ucSubSys,ucSubSyspu,pln,blk))
                DBG_printf(" %d",blk);
        }
        DBG_printf("\n");
    }

}
/****************************************************************************
Name        :BBT_Print_All_Bbt
Input       :void
Output      :
Author      :JOHNZHANG
Date        :20131224
Description :BBT BBT_Print_All_Bbt
Others      :
Modify      :
****************************************************************************/

void  mpt_print_bbt(PDEVICE_OBJECT pDevObj)
{
	U32 ulPuMskLow,ulPuMskHigh;
    U8 ucPuCnt,ucPu,ucSubSys,ucSubSysPuNum;
	Api_Ext_Get_Pu_Msk(pDevObj,&ulPuMskLow,&ulPuMskHigh,&ucPuCnt);
    for(ucPu=0;ucPu<ucPuCnt;ucPu++)
    {
		Api_Ext_Get_SubSys_Pu(pDevObj,ucPu,&ucSubSys,&ucSubSysPuNum);
        fw_print_pu_bbt(pDevObj,ucSubSys,ucSubSysPuNum);
    }
}
void mpt_fw_add_bbt(PDEVICE_OBJECT pDevObj,U8 ucSubSys,U8 ucPu,U8 ucPln,U16 ucBlk)
{
	U32 usBlkCnt,usPlnCnt;
	
	U8 * pBbt;
	U8 * pBytePos;
	usBlkCnt = pDevObj->VarTable.SubSysTable[0].ucBlkNum;
	usPlnCnt = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
	

	pBbt = &pDevObj->BbtBuf[ucSubSys][0];
	pBytePos = pBbt+ucPu*(usPlnCnt)*(usBlkCnt/8)+ucPln*(usBlkCnt/8)+(ucBlk/8);

	 *(volatile U8 *)pBytePos|=(1<<(ucBlk%8));
}
void mpt_fw_build_bbt(PDEVICE_OBJECT pDevObj)
{
	U8 ucPu;
	U8 ucPuCnt,ucSubSys,ucSubSysPuNum;
	
	U16 usBlk,usStartBlk,usBlkCnt;
	U32 ulPuMskLow,ulPuMskHigh,ulPuMskStatusLow,ulPuMskStatusHigh; 
	U8 usPln,usPlnCnt;
	usStartBlk = BBT_START_BLK;
	usBlkCnt = pDevObj->VarTable.SubSysTable[0].ucBlkNum;
	usPlnCnt = pDevObj->VarTable.SubSysTable[0].ucPlnNum;
	memset(pDevObj->BbtBuf,0,sizeof(pDevObj->BbtBuf));
	Api_Ext_Get_Pu_Msk(pDevObj,&ulPuMskLow,&ulPuMskHigh,&ucPuCnt);
	//erase
	for(usPln=0;usPln<usPlnCnt;usPln++)
	{
		for(usBlk=usStartBlk;usBlk<usBlkCnt;usBlk++)
		{
			Api_Erase_Flash(pDevObj,usPln,usBlk,ulPuMskLow,ulPuMskHigh);
			ulPuMskStatusLow =ulPuMskStatusHigh= 0;
			Api_Flash_Get_Cmd_Status(pDevObj,ulPuMskLow,ulPuMskHigh,&ulPuMskStatusLow,&ulPuMskStatusHigh);
			if((ulPuMskLow!=ulPuMskStatusLow)||(ulPuMskHigh!=ulPuMskStatusHigh))
			{
				for(ucPu=0;ucPu<ucPuCnt;ucPu++)
				{
					if(ucPu<32)
					{
						if((ulPuMskLow&(1<<ucPu))!=(ulPuMskStatusLow&(1<<ucPu)))
						{
							Api_Ext_Get_SubSys_Pu(pDevObj,ucPu,&ucSubSys,&ucSubSysPuNum);
							mpt_fw_add_bbt(pDevObj,ucSubSys,ucSubSysPuNum,usPln,usBlk);
							//DBG_printf("pu:%d pln:%d blk:%d ers err!\n",ucPu,usPln,usBlk);
						}
						
					}
					else
					{
						if((ulPuMskHigh&(1<<ucPu))!=(ulPuMskStatusHigh&(1<<ucPu)))
						{
							Api_Ext_Get_SubSys_Pu(pDevObj,ucPu,&ucSubSys,&ucSubSysPuNum);
							mpt_fw_add_bbt(pDevObj,ucSubSys,ucSubSysPuNum,usPln,usBlk);
							//DBG_printf("pu:%d pln:%d blk:%d ers err!\n",ucPu,usPln,usBlk);
						}
					}
					
				}
			}
		}
	}
	mpt_print_bbt(pDevObj);

	U8 ucSubSysCnt,cpuid;
	ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
	for(cpuid=MCU1;cpuid<(MCU1+ucSubSysCnt);cpuid++)
		Api_Write_Dram_Sram(pDevObj,cpuid,pDevObj->VarTable.SubSysTable[cpuid-MCU1].ulL3BbtTableAddr,MAX_BBT_SIZE,&pDevObj->BbtBuf[cpuid-MCU1][0]);
	
}
void mpt_fw_l3_llf(PDEVICE_OBJECT pDevObj)
{
	U8 ucSubSysCnt,cpuid;
	ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
	for(cpuid=MCU1;cpuid<(MCU1+ucSubSysCnt);cpuid++)
		Api_Ext_L3_Llf(pDevObj,cpuid);
}
void mpt_fw_l2_llf(PDEVICE_OBJECT pDevObj)
{
	U8 ucSubSysCnt,cpuid;
	ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
	for(cpuid=MCU1;cpuid<(MCU1+ucSubSysCnt);cpuid++)
		Api_Ext_L2_Llf(pDevObj,cpuid);
}
void mpt_fw_bbt_save(PDEVICE_OBJECT pDevObj)
{
	U8 ucSubSysCnt,cpuid;
	ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
	for(cpuid=MCU1;cpuid<(MCU1+ucSubSysCnt);cpuid++)
		Api_Ext_Bbt_Save(pDevObj,cpuid);
}
void mpt_fw_bbt_load(PDEVICE_OBJECT pDevObj)
{
	U8 ucSubSysCnt,cpuid;
	ucSubSysCnt = pDevObj->VarTable.VarHeadTable.VarHeadL0Table.ucSubSysCnt;
	for(cpuid=MCU1;cpuid<(MCU1+ucSubSysCnt);cpuid++)
		Api_Ext_Bbt_Load(pDevObj,cpuid);
}
void mpt_fw_stage(DISKTYPE type)
{
	PDEVICE_OBJECT pDevObj;
	U32 ulVarTableAddr;
	U8 ch,ucDiskCnt;
	while(1)
    {
          ucDiskCnt = Api_Disk_Detect((DISKTYPE)type);         
          
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
	Api_Init_FlashInfo(pDevObj);

	mpt_fw_l3_llf(pDevObj);
	Sleep(2000);
	mpt_fw_build_bbt(pDevObj);
	Sleep(2000);
	mpt_fw_bbt_save(pDevObj);
	Sleep(2000);
	mpt_fw_bbt_load(pDevObj);
	Sleep(2000);
	mpt_fw_l2_llf(pDevObj);

}



