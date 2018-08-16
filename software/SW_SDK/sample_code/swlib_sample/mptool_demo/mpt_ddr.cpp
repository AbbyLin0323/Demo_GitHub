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

/*
set_reg     (0x1ff80c04) 0x80000000
set_reg     (0x1ff80c0c) 0x00079c80
set_reg     (0x1ff80c10) 0x6803625a
set_reg     (0x1ff80c14) 0x02220013
set_reg     (0x1ff80c18) 0x003f2180
set_reg     (0x1ff80c1c) 0x323594b0
set_reg     (0x1ff80c20) 0x2000aafe
set_reg     (0x1ff80c24) 0x00008200
set_reg     (0x1ff80c28) 0x00000580
set_reg     (0x1ff80c48) 0x00000000
set_reg     (0x1ff80c60) 0x0000046e
set_reg     (0x1ff80c64) 0x00008000
set_reg     (0x1ff80c68) 0x00010000
set_reg     (0x1ff80c70) 0x80000000    
set_reg     (0x1ff80c10) 0x6803625a
set_reg     (0x1ff80c1c) 0x323594b0
set_reg     (0x1ff80c60) 0x0000040e
set_reg     (0x1ff80c24) 0x00008200
set_reg     (0x1ff80c28) 0x00000500
set_reg     (0x1ff80c14) 0x0b33001f    
set_reg     (0x1ff80c10) 0x6003625a
set_reg     (0x1ff80c10) 0x6803625a

# MR2
set_reg     (0x1ff80c10) 0x6b03625a
set_reg     (0x1ff80c04) 0x80020008
set_reg     (0x1ff80c04) 0x800a0008
set_reg     (0x1ff80c04) 0x80020008  

# MR3
set_reg     (0x1ff80c04) 0x80030000
set_reg     (0x1ff80c04) 0x800b0000
set_reg     (0x1ff80c04) 0x80030000

    
# MR1 
#DLL disable
set_reg     (0x1ff80c04) 0x80010007   
set_reg     (0x1ff80c04) 0x80090007
set_reg     (0x1ff80c04) 0x80010007
    
#MR0
#DLL off for power mode slow exit
set_reg     (0x1ff80c04) 0x80000818
set_reg     (0x1ff80c04) 0x80080818
set_reg     (0x1ff80c04) 0x80000818
    
# ZQCL
set_reg     (0x1ff80c10) 0x6e03625a
set_reg     (0x1ff80c04) 0x80081918
set_reg     (0x1ff80c04) 0x80001918    
set_reg     (0x1ff80c10) 0x6803625a
set_reg     (0x1ff80c24) 0x0000c300    
set_reg     (0x1ff80c28) 0x00000506
set_reg     (0x1ff80c2c) 0x00000008
#set_reg     (0x1ff80c14) 0x0923001b
#512GB per CE
set_reg     (0x1ff80c14) 0x0923011b

#normal mode
set_reg     (0x1ff80c10) 0x6883625a
set_reg     (0x1ff80c1c) 0x733f9700
set_reg     (0x1ff80c60) 0x0000040e
set_reg     (0x1ff80c70) 0x80000000

#DLL-off mode clk phase and pipe selection
#set_reg     (0x1ff80c04) 0xce001918
#eco.bit
#set_reg     (0x1ff80c04) 0xcd001918
set_reg     (0x1ff80c04) 0xccc01918
set_reg     (0x1ff80068) 0x43e00

*/
//#define C0_FPGA
//#define B0_ASIC
#define PRE_DRAM
#ifdef C0_FPGA
REG g_ddr_reg[MAX_REG_CNT_DDR]={
	{0x1ff80c04,0x80000000},
	{0x1ff80c0c,0x00079c80},
	{0x1ff80c10,0x6803625a},
	{0x1ff80c14,0x02220013},
	{0x1ff80c18,0x003f2180},
	{0x1ff80c1c,0x323594b0},
	{0x1ff80c20,0x2000aafe},
	{0x1ff80c24,0x00008200},
	{0x1ff80c28,0x00000580},
	{0x1ff80c48,0x00000000},
	{0x1ff80c60,0x0000046e},
	{0x1ff80c64,0x00008000},
	{0x1ff80c68,0x00010000},
	{0x1ff80c70,0x80000000},  
	{0x1ff80c10,0x6803625a},
	{0x1ff80c1c,0x323594b0},
	{0x1ff80c60,0x0000040e},
	{0x1ff80c24,0x00008200},
	{0x1ff80c28,0x00000500},
	{0x1ff80c14,0x0b33001f},  
	{0x1ff80c10,0x6003625a},
	{0x1ff80c10,0x6803625a},


	{0x1ff80c10,0x6b03625a},
	{0x1ff80c04,0x80020008},
	{0x1ff80c04,0x800a0008},
	{0x1ff80c04,0x80020008},


	{0x1ff80c04,0x80030000},
	{0x1ff80c04,0x800b0000},
	{0x1ff80c04,0x80030000},

    
	{0x1ff80c04,0x80010007},   
	{0x1ff80c04,0x80090007},
	{0x1ff80c04,0x80010007},
    
	{0x1ff80c04,0x80000818},
	{0x1ff80c04,0x80080818},
	{0x1ff80c04,0x80000818},
    

	{0x1ff80c10,0x6e03625a},
	{0x1ff80c04,0x80081918},
	{0x1ff80c04,0x80001918},    
	{0x1ff80c10,0x6803625a},
	{0x1ff80c24,0x0000c300},    
	{0x1ff80c28,0x00000506},
	{0x1ff80c2c,0x00000008},


	{0x1ff80c14,0x0923011b},


	{0x1ff80c10,0x6883625a},
	{0x1ff80c1c,0x733f9700},
	{0x1ff80c60,0x0000040e},
	{0x1ff80c70,0x80000000},



	{0x1ff80c04,0xccc01918},
	{0x1ff80068,0x43e00},

	{0xfffffffe,0x00000000}

};
#endif
#ifdef B0_ASIC
REG g_ddr_reg[MAX_REG_CNT_DDR]={
	{0x1ff80c00,0x00233333},
	{0x1ff80c04,0x80000000},
	{0x1ff80c08,0x5005001a},
	{0x1ff80c0c,0x20096080},
	{0x1ff80c10,0x6803625a},
	{0x1ff80c14,0x02220013},
	{0x1ff80c18,0x003f2280},
	{0x1ff80c1c,0x6c16b083},
	{0x1ff80c20,0x2000a254},
	{0x1ff80c24,0x0000c200},
	{0x1ff80c28,0x060005ff},
	{0x1ff80c2c,0x00008008},
	{0x1ff80c30,0x00330000},
	{0x1ff80c34,0x03000f02},
	{0x1ff80c38,0xffff8800},
	{0x1ff80c3c,0x88888888},
	{0x1ff80c58,0x1f1f3f36},
	{0x1ff80c5c,0x32323232},
	{0x1ff80c48,0x00000000},
	{0x1ff80c4c,0x000f0100},
	{0x1ff80c60,0x0000040e},
	{0x1ff80c64,0x00008000},
	{0x1ff80c68,0x00110000},
	{0x1ff80c70,0x00000000},
	{0x1ff80c78,0x00000000},
	{0x1ff80c7c,0x0f0f1012},
	//set_delay 1000
	{0xffffffff,100},

	//#clk gating and address mapping
	{0x1ff80c10,0x6803625a},
	{0x1ff80c1c,0x6c16b083},
	{0x1ff80c20,0x2000a254},
	{0x1ff80c60,0x0000040e},
	{0x1ff80c24,0x0000c200},
	{0x1ff80c28,0x06000500},
	{0x1ff80c14,0x0b33001f},
	//set_delay 2000
	{0xffffffff,200},

	//# MREST
	{0x1ff80c10,0x6003625a},
	//set_delay 150	  
	{0xffffffff,15},
	{0x1ff80c10,0x6803625a},
	//set_delay 2000
	{0xffffffff,200},

	//#MR2
	{0x1ff80c10,0x6b03625a},
	{0x1ff80c04,0x80020208},
	{0x1ff80c04,0x800a0208},
	{0x1ff80c04,0x80020208},

	//set_delay 2000
	{0xffffffff,200},
	//#MR3
	{0x1ff80c04,0x80030000},
	{0x1ff80c04,0x800b0000},
	{0x1ff80c04,0x80030000},

	//set_delay 2000
	{0xffffffff,200},
	//#MR1
	{0x1ff80c04,0x80010002},
	{0x1ff80c04,0x80090002},
	{0x1ff80c04,0x80010002},

	//set_delay 2000
	{0xffffffff,200},
	//#MR0
	{0x1ff80c04,0x80001938},
	{0x1ff80c04,0x80081938},
	{0x1ff80c04,0x80001938},

	//set_delay 2000
	{0xffffffff,200},
	//#ZQCL
	{0x1ff80c10,0x6e03625a},
	{0x1ff80c04,0x80081938},
	{0x1ff80c04,0x80001938},

	//set_delay 2000
	{0xffffffff,200},
	//#Normal mode
	{0x1ff80c10,0x6803625a},
	{0x1ff80c20,0x3000a254},
	{0x1ff80c24,0x0000c300},
	{0x1ff80c28,0x060005ff},
	{0x1ff80c14,0x0923001b},
                                
	//set_delay 2000
	{0xffffffff,200},

	{0x1ff80c10,0x6883625a},
	{0x1ff80c20,0x3000a254},
	{0x1ff80c60,0x0000040e},
	{0x1ff80c70,0x00000000},
	
	{0x1ff80054,0x00000000},

	//end mark
	{0xfffffffe,0x00000000}

};
#endif
#ifdef PRE_DRAM
REG g_ddr_reg[MAX_REG_CNT_DDR]={
{0x1ff81f18,0x00001010},
{0x1ff80c04,0x80000000},
{0x1ff80c08,0x00008080},
{0x1ff80c0C,0x20075c80},
{0x1ff80c10,0x6803625b},
{0x1ff80c14,0xd911013b},
{0x1ff80c18,0x003f2280},
{0x1ff80c1c,0x6418c134},
{0x1ff80c20,0x3465dbfb},
{0x1ff80c24,0x0000c200},
{0x1ff80c28,0x060005ff},
{0x1ff80c2c,0x00008000},
{0x1ff80c34,0x03300e00},
{0x1ff80c38,0x4411ff00},
{0x1ff80c3c,0xf4f4ffff},
{0x1ff80c58,0x00000000},
{0x1ff80c5c,0x32323232},
{0x1ff80c48,0x00000000},
{0x1ff80c4c,0x000f0100},
{0x1ff80c60,0x0000026e},
{0x1ff80c64,0x00008024},
{0x1ff80c68,0x00110000},
{0x1ff80c70,0x00000000},
{0x1ff80c78,0x00000000},
{0x1ff80c7c,0x0f1b0a0e},
{0x1ff80c10,0x6803625b},
{0x1ff80c1c,0x6418c134},
{0x1ff80c20,0x2465dbfb},
{0x1ff80c60,0x0000020e},
{0x1ff80c24,0x0000c200},
{0x1ff80c14,0xd911013b},
{0x1ff80c1c,0x64189134},
{0x1ff80c10,0x6b03625b},
{0x1ff80c04,0x80003f00},
{0x1ff80c04,0x80083f00},
{0x1ff80c04,0x80003f00},
{0x1ff80c10,0x6d03625b},
{0x1ff80c04,0x80000000},
{0x1ff80c04,0x80080000},
{0x1ff80c04,0x80000000},
{0x1ff80c6c,0x00000000},
{0x1ff80c10,0x6b03625b},
{0x1ff80c04,0x80000aff},
{0x1ff80c04,0x80080aff},
{0x1ff80c04,0x80000aff},
{0x1ff80c10,0x6b03625b},
{0x1ff80c04,0x80000183},
{0x1ff80c04,0x80080183},
{0x1ff80c04,0x80000183},
{0x1ff80c04,0x80000204},
{0x1ff80c04,0x80080204},
{0x1ff80c04,0x80000204},
{0x1ff80c04,0x80000301},
{0x1ff80c04,0x80080301},
{0x1ff80c04,0x80000301},
{0x1ff80c04,0x80000b03},
{0x1ff80c04,0x80080b03},
{0x1ff80c04,0x80000b03},
{0x1ff80c10,0x6803625b},
{0x1ff80c24,0x0000c300},
{0x1ff80c10,0x6803625b},
{0x1ff80c1c,0x6418c134},
{0x1ff80c20,0x3465dbfb},
{0x1ff80c60,0x0000020e},
{0x1ff80c70,0x00000000},
{0x1ff80054,0x00000000},
//end mark
	{0xfffffffe,0x00000000}
};

#endif
#ifdef C0_ASIC
REG g_ddr_reg[MAX_REG_CNT_DDR]={
	{0x1ff80c04,0x80000000},
	{0x1ff80c08,0x00008080},
	{0x1ff80c0C,0x20075c80},
	{0x1ff80c10,0x6803625b},
	{0x1ff80c14,0xd911013b},
	{0x1ff80c18,0x003f2280},
	{0x1ff80c1c,0x6418c134},
	{0x1ff80c20,0x3465dbfb},
	{0x1ff80c24,0x0000c200},
	{0x1ff80c28,0x060005ff},
	{0x1ff80c2c,0x00008000},
	{0x1ff80c34,0x03300e00},
	{0x1ff80c38,0x4411ff00},
	{0x1ff80c3c,0xf4f4ffff},
	{0x1ff80c58,0x00000000},
	{0x1ff80c5c,0x32323232},
	{0x1ff80c48,0x00000000},
	{0x1ff80c4c,0x000f0100},
	{0x1ff80c60,0x0000026e},
	{0x1ff80c64,0x00008024},
	{0x1ff80c68,0x00110000},
	{0x1ff80c70,0x00000000},
	{0x1ff80c78,0x00000000},
	{0x1ff80c7c,0x0f1b0a0e},

//set_delay 100
	{0xffffffff,100},
//# Addressing/RMA/Clk gating/Reset
{0x1ff80c10,0x6803625b},
{0x1ff80c1c,0x6418c134},
{0x1ff80c20,0x2465dbfb},
{0x1ff80c60,0x0000020e},
{0x1ff80c24,0x0000c200},
{0x1ff80c14,0xd911013b},

//set_delay 200
{0xffffffff,200},
//#MR63
{0x1ff80c1c,0x64189134},
{0x1ff80c10,0x6b03625b},
{0x1ff80c04,0x80003f00},
{0x1ff80c04,0x80083f00},
{0x1ff80c04,0x80003f00},

//set_delay (50)
{0xffffffff,50},
//#MR0
{0x1ff80c10,0x6d03625b},
{0x1ff80c04,0x80000000},
{0x1ff80c04,0x80080000},
{0x1ff80c04,0x80000000},
{0x1ff80c6c,0x00000000},

//set_delay 50
{0xffffffff,50},
//#MR10
{0x1ff80c10,0x6b03625b},
{0x1ff80c04,0x80000aff},
{0x1ff80c04,0x80080aff},
{0x1ff80c04,0x80000aff},

//set_delay 50
{0xffffffff,50},
//#MR1
{0x1ff80c10,0x6b03625b},
{0x1ff80c04,0x80000183},
{0x1ff80c04,0x80080183},
{0x1ff80c04,0x80000183},

//set_delay 50
{0xffffffff,50},
//#MR2
{0x1ff80c04,0x80000204},
{0x1ff80c04,0x80080204},
{0x1ff80c04,0x80000204},

//set_delay 50
{0xffffffff,50},
//#MR3
{0x1ff80c04,0x80000301},
{0x1ff80c04,0x80080301},
{0x1ff80c04,0x80000301},

//set_delay 50
{0xffffffff,50},
//#MR11
{0x1ff80c04,0x80000b03},
{0x1ff80c04,0x80080b03},
{0x1ff80c04,0x80000b03},

//set_delay 100
{0xffffffff,100},
//#Normal mode
{0x1ff80c10,0x6803625b},
{0x1ff80c24,0x0000c300},
{0x1ff80c10,0x6803625b},
{0x1ff80c1c,0x6418c134},
{0x1ff80c20,0x3465dbfb},
{0x1ff80c60,0x0000020e},
{0x1ff80c70,0x00000000},
//# Disable debug signal
//#	(*(volatile U32 *)(0x1ff80054)) ) 0x00000000
//set_delay 1000	
{0xffffffff,1000},
{0x1ff80054,0x00000000},

	//end mark
	{0xfffffffe,0x00000000}

};
#endif
void mpt_rom_config_dram(U8 ucDiskIndex)
{
	void * RomHandle;
	U32 i;
	RomHandle = Api_Get_Rom_Obj(0);
	i=0;
	while(1)
	{
		
		if(RomHandle,g_ddr_reg[i].ulAddr==0xfffffffe)
		{
			DBG_printf("end\n");
			break;
		}
		else if(RomHandle,g_ddr_reg[i].ulAddr==0xffffffff)
		{
			DBG_printf("sleep:%d\n",g_ddr_reg[i].ulData);
			Sleep(g_ddr_reg[i].ulData);
		}
		else
		{
			Api_Rom_Write_Register(RomHandle,g_ddr_reg[i].ulAddr,g_ddr_reg[i].ulData);
			DBG_printf("wt reg addr:0x%x data:0x%x\n",g_ddr_reg[i].ulAddr,g_ddr_reg[i].ulData);
		}
		Sleep(100);
		i++;
	}

}
void mpt_dram_test(U8 ucDiskIndex)
{
	void * RomHandle;
	U32 i,addr,data,testcnt;
	BOOL res;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	addr = 0x40004000;
	testcnt = 512;
	DBG_printf("begin dram test!\n");
	for(i=0;i<testcnt;i++)
	{
		Api_Rom_Write_Register(RomHandle,addr+i*4,addr+i*4);
	}
	res = Api_Rom_Read_Status(RomHandle);
	
	for(i=0;i<testcnt;i++)
	{
		Api_Rom_Read_Register(RomHandle,addr+i*4,&data);
		if(data!=(addr+i*4))
		{
			DBG_printf("data check err data:0x%x correct data:0x%x\n",data,(addr+i*4));
			DBG_Getch();
		}
	}
	DBG_printf("end dram test!\n");
}