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
#include "HAL_ParamTable.h"
extern REG g_ddr_reg[MAX_REG_CNT];
//U8 g_Bbt_Buf[MAX_SUB_SYS_CNT][MAX_BBT_SIZE];
void mpt_rom_readid(U8 ucDiskIndex)
{
	void * RomHandle;
	U32 ulBuf;
	U32 id;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);

	ulBuf = 0x20003438;
//	Api_Rom_Read_Register(RomHandle,0xffe06000 - 56,&ulBuf);
	Api_Rom_Jump(RomHandle,ulBuf);
	DBG_printf("Init NFC done\n");
//	Api_Rom_Read_Register(RomHandle,0xffe06000 - 60,&ulBuf);
	ulBuf = 0x200008f8;
	Api_Rom_Jump(RomHandle,ulBuf);
	DBG_printf("Scan PU done\n");	
	Api_Rom_Read_Register(RomHandle,0xfff00000,&id);
	Api_Rom_Read_Register(RomHandle,0xfff00004,&id);
}
void mpt_rom_jump(U8 ucDiskIndex)
{
	void * RomHandle;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	Api_Rom_Jump(RomHandle,0xfff00008);
//	Api_Rom_Write_Register(RomHandle,0x1ff83a10,0x100);
}
void mpt_rom_close_handle(U8 ucDiskIndex)
{
	void * RomHandle;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	Api_Rom_Close_Handle(RomHandle);
}
#if 1
BOOL mpt_download_fw_by_dma(void * RomHandle,U32 * pAddr,U32 ulDevAddr,U32 ulCnt)
{
	U32 * pDataBuf;

	STATUS res;
	DBG_printf("trans bufaddr:0x%x devaddr:0x%x cnt:0x%x\n",pAddr,ulDevAddr,ulCnt);
	pDataBuf = (U32 *)pAddr;
	Api_Rom_Write_Dma(RomHandle, ulDevAddr, pDataBuf, ulCnt);

	res = Api_Rom_Read_Status(RomHandle);

	if(res==RETURN_SUCCESS)
		return TRUE;
	else
		return FALSE;
}
#endif
BOOL mpt_download_fw_by_cnt(void * RomHandle,U32 * pAddr,U32 ulDevAddr,U32 ulCnt)
{
	U32 i;
	U32 * pDataBuf;

	STATUS res;
	DBG_printf("trans bufaddr:0x%x devaddr:0x%x cnt:0x%x\n",pAddr,ulDevAddr,ulCnt);
	pDataBuf = (U32 *)pAddr;
	for(i=0;i<(ulCnt>>2);i++)
	{
		Api_Rom_Write_Register(RomHandle,ulDevAddr+i*4,*(U32 *)pDataBuf);
		pDataBuf++;
	}
	res = Api_Rom_Read_Status(RomHandle);
	//res  = RETURN_SUCCESS;

	if(res==RETURN_SUCCESS)
		return TRUE;
	else
		return FALSE;
}
U32 fwbuf[0x100000];
void mpt_download_fw(U8 ucDiskIndex,U8 * path)
{
	//U32 buf[(FW_SZ>>2)];
	
	BOOL res;
	U8 be_done,retry_cnt;
	void * RomHandle;
	U32 i;
	U32 *pbuf;
	
	FILE *pFile;
	DBG_printf("begin fw download\n");
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	fopen_s(&pFile,".\\mptool_demo\\mpt_bin\\NVME_L85_C0_ASIC.bin","rb");
    //fopen_s(&pFile,(const char *)path,"rb");
	if (pFile != NULL)
	{
		fread_s(fwbuf, FW_SZ, 1, FW_SZ, pFile);
	}
	else
	{
		DBG_printf("fw file open err invalid path!!\n");
		DBG_Getch();
	}

	for(i=0;i<(FW_SZ/TRANS_SZ);i++)
	{
		pbuf =(U32 *)((U32)fwbuf+i*TRANS_SZ);
		DBG_printf("send total cnt:0x%x\n",i*TRANS_SZ);
		be_done = TRUE;
		retry_cnt = 0;
		while(be_done)
		{
			//res = mpt_download_fw_by_cnt(RomHandle,pbuf,FW_START_ADDR+i*TRANS_SZ,TRANS_SZ);
			res = mpt_download_fw_by_dma(RomHandle,pbuf,FW_START_ADDR+i*TRANS_SZ,TRANS_SZ);
			
			if(res==TRUE)
			{
				be_done = FALSE;
			}
			else
			{
				retry_cnt++;
				if(retry_cnt>100)
				{
					DBG_printf("retry > 100\n");
					fclose(pFile);
					DBG_Getch();
				}
			}
		}
	}

	fclose(pFile);
	DBG_printf("end fw download\n");
}
void mpt_recover_nfc_setting(U8 ucDiskIndex)
{
	U8 buf[NFC_REG_SZ];
	void * RomHandle;
	U32 i;
	U32 *pbuf;
	FILE *pFile;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	fopen_s(&pFile,".\\mptool_demo\\bin\\nfcset_uart.bin","rb");
	if(pFile!=NULL)
		fread_s(buf,NFC_REG_SZ,1,NFC_REG_SZ,pFile);

	pbuf = (U32 *)buf;
	for(i=0;i<(NFC_REG_SZ>>2);i++)
		Api_Rom_Write_Register(RomHandle,NFC_REG_START_ADDR+i*4,*pbuf++);

	fclose(pFile);
}
void mpt_reset_nfc(U8 ucDiskIndex)
{
	
	void * RomHandle;
	U32 data;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	
	Api_Rom_Read_Register(RomHandle,0x1ff80018,&data);
	data|=0xf8;
	Api_Rom_Write_Register(RomHandle,0x1ff80018,data);
	data&=(~0xf8);
	Api_Rom_Write_Register(RomHandle,0x1ff80018,data);
}
void mpt_download_bootloader(U8 ucDiskIndex,U8 * path)
{
	U8 buf[BOOTLOADER_SZ];
	void * RomHandle;
	U32 i;
	U32 *pbuf;
	BOOTLOADER_FILE * pBootloader;
	FILE *pFile;
	DBG_printf("begin bootloader download\n");
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	fopen_s(&pFile,".\\mptool_demo\\mpt_bin\\bl_c0_nvme.bin","rb");
	//fopen_s(&pFile, (const char *)path, "rb");
	if (pFile != NULL)
	{
		fread_s(buf, BOOTLOADER_SZ, 1, BOOTLOADER_SZ, pFile);
	}
	else
	{
		DBG_printf("bootloader file open err invalid path!!\n");
		DBG_Getch();
	}

	pbuf = (U32 *)buf;
	pBootloader = (BOOTLOADER_FILE *)buf;
	pBootloader->tSysParameterTable.sBootStaticFlag.bsBootMethodSel = MP_MODE;
	pBootloader->tSysParameterTable.sHwInitFlag.bsPCIeInitDone = 1;
	pBootloader->tSysParameterTable.sHwInitFlag.bsDDRInitDone = 1;
	for(i=0;i<(BOOTLOADER_SZ>>2);i++)
	{
		Api_Rom_Write_Register(RomHandle,BOOLOADER_START_ADDR+i*4,*pbuf++);
	}

	fclose(pFile);
	DBG_printf("end bootloader download\n");
}
void mpt_restore_reg(U8 ucDiskIndex,U32 ulAddr,U32 ulCnt)
{
	U8 *buf;
	void * RomHandle;
	U32 i;
	U32 *pbuf;
	FILE *pFile;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	fopen_s(&pFile,".\\mptool_demo\\bin\\reg.bin","rb");
	if(pFile!=NULL)
	{
		buf = (U8 *)malloc(ulCnt);
		fread_s(buf,ulCnt,1,ulCnt,pFile);
		pbuf = (U32 *)buf;
		for(i=0;i<(ulCnt>>2);i++)
		{
			Api_Rom_Write_Register(RomHandle,ulAddr+i*4,*pbuf++);
		}
		
		free(buf);
	}
	fclose(pFile);
}
void mpt_dump_reg(U8 ucDiskIndex,U32 ulAddr,U32 ulCnt)
{
	U8 *buf;
	void * RomHandle;
	U32 i;
	U32 *pbuf;
	FILE *pFile;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	fopen_s(&pFile,".\\mptool_demo\\bin\\reg.bin","wb+");
	if(pFile!=NULL)
	{
		buf = (U8 *)malloc(ulCnt);
		pbuf = (U32 *)buf;
		for(i=0;i<(ulCnt>>2);i++)
		{
			Api_Rom_Read_Register(RomHandle,ulAddr+i*4,pbuf++);
		}
		fwrite(buf,1,ulCnt,pFile);
		free(buf);
	}
	fclose(pFile);
}
void mpt_rom_set_uart_baudrate(U8 ucDiskIndex,U32 ulSpeedDev,U32 ulSpeedHost)
{
#if 0
	void * RomHandle;
	STATUS res;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	Api_Rom_Write_Register(RomHandle,0xffe0700c,ulSpeedDev);
	Sleep(2000);
	Api_Rom_Set_Trans_Speed(RomHandle,ulSpeedHost);
	res = Api_Rom_Read_Status(RomHandle);
#endif
	void * RomHandle;
	RomHandle = Api_Get_Rom_Obj(ucDiskIndex);
	Api_Rom_Set_Uart_Baudrate(RomHandle, ulSpeedDev, ulSpeedHost);
		
}
void mpt_rom_stage(DISKTYPE * type)
{
	U8 ucDiskCnt,ch;
	U8 ucFwPath[256];
	U8 ucFwBootloader[256];
#if 1
	memset(ucFwPath, 0, sizeof(ucFwPath));
	DBG_printf("please input fw path:\n");
	scanf_s("%s", ucFwPath, 80);
	DBG_printf("please input bootloader path:\n");
	memset(ucFwBootloader, 0, sizeof(ucFwBootloader));
	scanf_s("%s", ucFwBootloader, 80);
	while(1)
    {
          {
              DBG_printf("0:AHCI\n");
              DBG_printf("1:NVME\n");
              DBG_printf("2:SATA\n");
              DBG_printf("3:UART\n");
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
				  *type = (DISKTYPE)ch;
                  ucDiskCnt = Api_Disk_Detect_Rom((DISKTYPE)ch);
                  break;
              }
          }
          
    }
    if(ch==UART)
	    mpt_rom_set_uart_baudrate(0,0x1d0000,1152000);
	//mpt_dump_reg(0,0x1ff80000,0x2000);
	//mpt_rom_readid(0);
	mpt_rom_config_dram(0);
	mpt_dram_test(0);
	mpt_download_bootloader(0, (U8 *)ucFwBootloader);
	mpt_download_fw(0, (U8 *)ucFwPath);
	mpt_rom_jump(0);
	mpt_rom_close_handle(0);
    DBG_printf("\n");
#endif

}


