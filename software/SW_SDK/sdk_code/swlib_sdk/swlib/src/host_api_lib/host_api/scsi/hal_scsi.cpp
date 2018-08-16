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
  File Name     : hal_scsi.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the hal function of scsi.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include "hal_scsi.h"
#include "ioctrl_scsi.h"
#include "..\\host_api.h"
STATUS Scsi_Read_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf)
{
	CDB_EXT regcdb;
	BOOL res;
	U8 *buf_addr;
	U8 *host_rd_addr;
	U32 dev_mem_addr,rem_data_len,trans_data_len;
	
	memset(&regcdb,0,sizeof(CDB_EXT));

	host_rd_addr = readbuf;
	rem_data_len = length;
	dev_mem_addr = startaddr;

	while(rem_data_len)
	{
		trans_data_len = (rem_data_len>SCSI_MAX_RD_LEN?SCSI_MAX_RD_LEN:rem_data_len);
		buf_addr = (U8 *)malloc((trans_data_len+SEC_SIZE-1)/SEC_SIZE);
		regcdb.opcode = 0x85;
		regcdb.extend = 1;
		regcdb.protocol = 6;
		regcdb.command  = 0xFF;
		regcdb.t_dir = 1;
		regcdb.device = 0xe0;
		regcdb.features_l = OP_MEM_READ;
		regcdb.byte_block = 1;
		regcdb.t_length = 2;

		regcdb.lba_low_l = (dev_mem_addr)&0xff;
		regcdb.lba_mid_l = (dev_mem_addr>>8)&0xff;
		regcdb.lba_high_l = (dev_mem_addr>>16)&0xff;
		regcdb.sector_count_l = (dev_mem_addr>>24)&0xff;
		regcdb.lba_low_h = (trans_data_len&0xff);
		regcdb.lba_mid_h = ((trans_data_len>>8)&0xff);
		regcdb.features_h = cpuid;
		
		res = Scsi_Ioctrl(hDevice,(U8 *)&regcdb,sizeof(CDB_EXT),SCSI_IOCTL_DATA_IN,(trans_data_len),buf_addr);

		memcpy(host_rd_addr,buf_addr,trans_data_len);
		free(buf_addr);

		if(!res)
		{
			return RETURN_FAIL;
		}
		
		host_rd_addr+= trans_data_len;//host rd buf
		dev_mem_addr +=trans_data_len;//dev mem addr
		rem_data_len-=trans_data_len;
	}
	
	return RETURN_SUCCESS;
}
STATUS Scsi_Write_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf)
{
	CDB_EXT regcdb;
	BOOL res;
	U8 *buf_addr;
	U8 *host_wt_addr;
	U32 dev_mem_addr,rem_data_len,trans_data_len;

	memset(&regcdb,0,sizeof(CDB_EXT));
	rem_data_len = length;
	host_wt_addr = writebuf;
	dev_mem_addr = startaddr;
	while(rem_data_len)
	{
		trans_data_len = (length>SCSI_MAX_WT_LEN?SCSI_MAX_WT_LEN:length);
		buf_addr = (U8 *)malloc((trans_data_len+SEC_SIZE-1)/SEC_SIZE);

		regcdb.opcode = 0x85;
		regcdb.extend = 1;
		regcdb.protocol = 6;
		regcdb.command  = 0xFF;
		regcdb.t_dir = 0;
		regcdb.device = 0xe0;
		regcdb.features_l = OP_MEM_WRITE;
		regcdb.byte_block = 1;
		regcdb.t_length = 2;
		regcdb.lba_low_l = (dev_mem_addr)&0xff;
		regcdb.lba_mid_l = (dev_mem_addr>>8)&0xff;
		regcdb.lba_high_l = (dev_mem_addr>>16)&0xff;
		regcdb.sector_count_l = (dev_mem_addr>>24)&0xff;
		
		regcdb.lba_low_h = (trans_data_len&0xff);
		regcdb.lba_mid_h = ((trans_data_len>>8)&0xff);
		regcdb.features_h = cpuid;
		
		memcpy(buf_addr,host_wt_addr,trans_data_len);
		res = Scsi_Ioctrl(hDevice,(U8 *)&regcdb,sizeof(CDB_EXT),SCSI_IOCTL_DATA_OUT,(trans_data_len+SEC_SIZE-1)/SEC_SIZE,buf_addr);
		free(buf_addr);
		if(!res)
		{
			return RETURN_FAIL;
		}
		host_wt_addr += trans_data_len;//host wt buf
		dev_mem_addr +=trans_data_len;//dev mem addr
		rem_data_len-=trans_data_len;
	}
	return RETURN_SUCCESS;
}
STATUS Scsi_Read_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk)
{
	CDB_EXT regcdb;
	BOOL res;
	U8 befullpln;
	U32 trans_data_len;
	memset(&regcdb,0,sizeof(CDB_EXT));

	regcdb.opcode = 0x85;
	regcdb.extend = 1;
	regcdb.protocol = 6;
	regcdb.command  = 0xFF;
	regcdb.t_dir = 1;
	regcdb.device = 0xe0;
	regcdb.features_l = OP_FLASH_READ;
	regcdb.byte_block = 1;
	regcdb.t_length = 2;
	regcdb.lba_low_l = (pumsk)&0xff;
	regcdb.lba_mid_l = (pumsk>>8)&0xff;
	regcdb.lba_high_l = (pumsk>>16)&0xff;
	regcdb.sector_count_l = (pumsk>>24)&0xff;
		
	regcdb.lba_low_h = (block&0xff);
	regcdb.lba_mid_h = ((block>>8)&0xff);
	regcdb.lba_high_h = (page&0xff);
	befullpln = (plnmode==MULTI_PLN?0:1);
	regcdb.sector_count_h = CACULATE_PG_PLN(page,pln,befullpln);//(page&(1<<8))|((pln&0xf)<<1)|((ucBeFullPln&0X1)<<5);
	regcdb.features_h = cpuid;
		
	trans_data_len = SEC_SIZE;//Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);

		
	res = Scsi_Ioctrl(hDevice,(U8 *)&regcdb,sizeof(CDB_EXT),SCSI_IOCTL_DATA_IN,trans_data_len,pStatusBuf);
	if(!res)
	{
		return RETURN_FAIL;
	}
		
	return RETURN_SUCCESS;
}
STATUS Scsi_Write_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk)
{
	CDB_EXT regcdb;
	BOOL res;
	U8 befullpln;
	U32 trans_data_len;
	memset(&regcdb,0,sizeof(CDB_EXT));

	regcdb.opcode = 0x85;
	regcdb.extend = 1;
	regcdb.protocol = 6;
	regcdb.command  = 0xFF;
	regcdb.t_dir = 1;
	regcdb.device = 0xe0;
	regcdb.features_l = OP_FLASH_WRITE;
	regcdb.byte_block = 1;
	regcdb.t_length = 2;
	regcdb.lba_low_l = (pumsk)&0xff;
	regcdb.lba_mid_l = (pumsk>>8)&0xff;
	regcdb.lba_high_l = (pumsk>>16)&0xff;
	regcdb.sector_count_l = (pumsk>>24)&0xff;
		
	regcdb.lba_low_h = (block&0xff);
	regcdb.lba_mid_h = ((block>>8)&0xff);
	regcdb.lba_high_h = (page&0xff);
	befullpln = (plnmode==MULTI_PLN?0:1);
	regcdb.sector_count_h = CACULATE_PG_PLN(page,pln,befullpln);//(page&(1<<8))|((pln&0xf)<<1)|((ucBeFullPln&0X1)<<5);
	regcdb.features_h = cpuid;
		
	//trans_data_len = Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
	trans_data_len = SEC_SIZE;//Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
		
	res = Scsi_Ioctrl(hDevice,(U8 *)&regcdb,sizeof(CDB_EXT),SCSI_IOCTL_DATA_IN,trans_data_len,pStatusBuf);
	if(!res)
	{
		return RETURN_FAIL;
	}
		
	return RETURN_SUCCESS;
}
STATUS Scsi_Erase_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U8 *pStatusBuf,U32 pumsk)
{
	CDB_EXT regcdb;
	BOOL res;
	U8 befullpln;
	U16 page;
	U32 trans_data_len;
	memset(&regcdb,0,sizeof(CDB_EXT));

	regcdb.opcode = 0x85;
	regcdb.extend = 1;
	regcdb.protocol = 6;
	regcdb.command  = 0xFF;
	regcdb.t_dir = 1;
	regcdb.device = 0xe0;
	regcdb.features_l = OP_FLASH_ERASE;
	regcdb.byte_block = 1;
	regcdb.t_length = 2;
	regcdb.lba_low_l = (pumsk)&0xff;
	regcdb.lba_mid_l = (pumsk>>8)&0xff;
	regcdb.lba_high_l = (pumsk>>16)&0xff;
	regcdb.sector_count_l = (pumsk>>24)&0xff;
		
	regcdb.lba_low_h = (block&0xff);
	regcdb.lba_mid_h = ((block>>8)&0xff);
	page = 0;
	regcdb.lba_high_h = (page&0xff);
	befullpln = (plnmode==MULTI_PLN?0:1);
	regcdb.sector_count_h = CACULATE_PG_PLN(page,pln,befullpln);//(page&(1<<8))|((pln&0xf)<<1)|((ucBeFullPln&0X1)<<5);
	regcdb.features_h = cpuid;
		
	trans_data_len = SEC_SIZE;//Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
		
	res = Scsi_Ioctrl(hDevice,(U8 *)&regcdb,sizeof(CDB_EXT),SCSI_IOCTL_DATA_IN,trans_data_len,pStatusBuf);
	if(!res)
	{
		return RETURN_FAIL;
	}
	
	return RETURN_SUCCESS;
}
STATUS Scsi_NoneData_Common(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2)
{
	CDB_EXT regcdb;
	BOOL res;
	U32 trans_data_len;
	memset(&regcdb,0,sizeof(CDB_EXT));
	
	regcdb.opcode = 0x85;//0xA1;
	regcdb.extend = 1;
	regcdb.protocol = 3;
	regcdb.command  = 0xFF;
	regcdb.t_dir = 0;
	regcdb.device = 0xe0;
	regcdb.features_l = ucOpCode;
	regcdb.byte_block = 1;
	regcdb.t_length = 2;
	regcdb.lba_low_l = (ulInParam1)&0xff;
	regcdb.lba_mid_l = (ulInParam1>>8)&0xff;
	regcdb.lba_high_l = (ulInParam1>>16)&0xff;
	regcdb.sector_count_l = (ulInParam1>>24)&0xff;
		
	regcdb.lba_low_h = (ulInParam2&0xff);
	regcdb.lba_mid_h = ((ulInParam2>>8)&0xff);
	regcdb.lba_high_h = (ulInParam2>>16)&0xff;
	regcdb.sector_count_h = (ulInParam2>>24)&0xff;
	
	regcdb.features_h = ucCpuId;
		
	trans_data_len = 0;//Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
		
	res = Scsi_Ioctrl_Ex(hDevice,(U8 *)&regcdb,sizeof(CDB_EXT),SCSI_IOCTL_DATA_OUT,trans_data_len,NULL);

	memset(&regcdb, 0, sizeof(CDB_EXT));
	regcdb.protocol = 0XF;
	regcdb.extend = 1;
	regcdb.t_dir = 0;
	regcdb.device = 0xe0;
	res = Scsi_Ioctrl_Ex(hDevice, (U8 *)&regcdb, sizeof(CDB_EXT), SCSI_IOCTL_DATA_IN, trans_data_len, NULL);
	//res = Scsi_Ioctrl(hDevice, (U8 *)&regcdb, sizeof(CDB_EXT), SCSI_IOCTL_DATA_IN, trans_data_len, NULL);
	if(!res)
	{
		return RETURN_FAIL;
	}
	*pOutParam1 = 0x40048000;
	return RETURN_SUCCESS;
}

/*对于SCSI PATHROUGH 由于478 bridge不支持return fis,所以用dma命令返回data赋值替代,比如register read*/
STATUS Scsi_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2)
{
	if(ucOpCode == OP_REG_READ)
	{
		/*用dma命令替代 register read*/
		return Scsi_Read_Dram_Sram(hDevice, ucCpuId, ulInParam1, 4, (U8 *)pOutParam1);
		
	}
	else
	{
		return Scsi_NoneData_Common(hDevice,ucCpuId,ucOpCode,ulInParam1,ulInParam2,pOutParam1,pOutParam2);
	}
	
}
const BYTE identify_device[] = { 0xA1, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xEC, 0x00, 0x00 };
/*
	typedef struct _CDB
{
	unsigned char opcode;
	
	unsigned char rev:1;
	unsigned char protocol:4;
	unsigned char multiple_count:3;

	unsigned char t_length:2;
	unsigned char byte_block:1;
	unsigned char t_dir:1;
	unsigned char rev1:1;
	unsigned char ck_cond:1;
	unsigned char off_line:2;

	unsigned char feature;
	unsigned char sector_count;
	unsigned char lba_low;
	unsigned char lba_mid;
	unsigned char lba_high;
	unsigned char device;
	unsigned char command;
	unsigned char reserved;
	unsigned char control;


}CDB;
	
	*/
STATUS Scsi_Get_Identify_Data(HANDLE hDevice,U8 * readbuf)
{
	CDB regcdb;
	BOOL res;
	U32 trans_data_len;
	memset(&regcdb,0,sizeof(CDB));

	memcpy((U8 *)&regcdb,identify_device,sizeof(CDB));
		
	trans_data_len = SEC_SIZE;//Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
		
	res = Scsi_Ioctrl(hDevice,(U8 *)&regcdb,sizeof(CDB),SCSI_IOCTL_DATA_IN,trans_data_len,readbuf);
	if(!res)
	{
		return RETURN_FAIL;
	}
	
	return RETURN_SUCCESS;
}
STATUS Scsi_Get_ModelNumber(HANDLE hDevice,U8 * identifybuf,U8 mn[40])
{
	PIDINFO pidentify;
	pidentify = (PIDINFO)identifybuf;
	memcpy(mn, pidentify->sModelNumber, 40);
	return RETURN_SUCCESS;
}