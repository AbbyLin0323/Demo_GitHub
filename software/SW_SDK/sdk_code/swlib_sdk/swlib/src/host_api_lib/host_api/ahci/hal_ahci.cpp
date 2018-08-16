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
  File Name     : hal_ahci.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the hal function of ahci.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/
#include <stddef.h>
#include "..\\host_api.h"
#include "hal_ahci.h"
#include "ioctrl_ahci.h"
//0xc0 ROM_REG_READ
//0xc1 ROM_REG_WRITE
//0xc4 ROM_JUMP
STATUS Ahci_Cmd_Dma_Read
(
 HANDLE hDevice,
 U32 lba,
 U8 * inbuf_addr,
 U32 trans_sec_cnt
 )
{
	U32 trans_direction;
	IDEREGSEX reg;
	BOOL retval;

	reg.bSectorCountReg = trans_sec_cnt & 0xFF;
	reg.bLBALowReg = (lba & 0xFF);
	reg.bLBAMidReg = ((lba >> 8) & 0xFF);
	reg.bLBAHighReg = ((lba >> 16) & 0xFF);
	reg.bDriveHeadReg = 0xE0;
	reg.bCommandReg = ATA_CMD_READ_DMA;

	trans_direction = DATA_TRANSFER_IN;

	retval = Ahci_Dma_Out_Direct(hDevice,reg, trans_direction, inbuf_addr, trans_sec_cnt);
	if( retval != TRUE ) 
	{   
		return RETURN_FAIL;
	}

	return RETURN_SUCCESS;
}
STATUS Ahci_Cmd_Dma_Write
(
HANDLE hDevice,
U32 lba,
U8 * outbuf_addr,
U32 trans_sec_cnt
)
{
	U32 trans_direction;
	IDEREGSEX reg;
	BOOL retval;

	reg.bSectorCountReg = trans_sec_cnt & 0xFF;
	reg.bLBALowReg = (lba & 0xFF);
	reg.bLBAMidReg = ((lba >> 8) & 0xFF);
	reg.bLBAHighReg = ((lba >> 16) & 0xFF);
	reg.bDriveHeadReg = 0xE0;
	reg.bCommandReg = ATA_CMD_WRITE_DMA;

	trans_direction = DATA_TRANSFER_OUT;

	retval = Ahci_Dma_Out_Direct(hDevice, reg, trans_direction, outbuf_addr, trans_sec_cnt);
	if (retval != TRUE)
	{
		return RETURN_FAIL;
	}

	return RETURN_SUCCESS;
}
STATUS Ahci_Get_Identify_Data(HANDLE hDevice,U8 * readbuf)
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;

	IDEREGS reg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;
	//#define ATA_CMD_IDENTIFY_DEVICE			     0xEC
	reg.bCommandReg = 0xEC;


	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
	apt_ex_with_buf.apt.DataTransferLength = 512;
	apt_ex_with_buf.apt.TimeOutValue = 5;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	memcpy((void*)apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);

	inbuf = (void*)&(apt_ex_with_buf);
	inbufsz = apt_ex_with_buf.apt.Length;
	outbuf = (void*)&(apt_ex_with_buf);
	outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;

	if (!DeviceIoControl(hDevice, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		(LPDWORD)&bytes_rtn, NULL)) 
	{
		
		return RETURN_FAIL;
	}

	memcpy((void *)&(reg), (void*)apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( reg.bFeaturesReg != 0 ) 
	{   
		
		return RETURN_SUCCESS;
	}
	memcpy( (void*)readbuf, (void*)apt_ex_with_buf.data_buf, 512); 

	return RETURN_SUCCESS;
}
STATUS Ahci_Get_ModelNumber(HANDLE hDevice,U8 * identifybuf,U8 mn[40])
{
	PIDINFO pidentify;
	pidentify = (PIDINFO)identifybuf;
	memcpy(mn,pidentify->sModelNumber,40);
	return RETURN_SUCCESS;
}
STATUS Ahci_Read_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf)
{
	U32 trans_direction;
    IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	BOOL retval;
	U32 ulReadSectorCnt;
	U8 *buf_sec_align;

    reg.bFeaturesReg = OP_MEM_READ;//VD_WRITE_DRAM_SRAM;
    reg.bCommandReg = 0XFF;//ATA_CMD_VENDER_DEFINE;    
    reg.bDriveHeadReg = 0xE0;
	

    reg.bLBALowReg=(U8)((U32)startaddr)&0xff;
    reg.bLBAMidReg=(U8)((U32)startaddr>>8)&0xff;
    reg.bLBAHighReg=(U8)((U32)startaddr>>16)&0xff;
	reg.bSectorCountReg=(U8)((U32)startaddr>>24)&0xff;

	exreg.bLBALowReg = (U8)(length & 0xFF);
	exreg.bLBAMidReg = ((length>>8) & 0xFF);  
	exreg.bFeaturesReg = cpuid;

    ulReadSectorCnt = (length>>9)+((length&0x1ff)>0?1:0);
	buf_sec_align = (U8 *)malloc(ulReadSectorCnt*SEC_SIZE);
	trans_direction = DATA_TRANSFER_IN;

//	retval = sata_dma_cmd_ex(disk, reg,exreg,trans_direction, outbuf_addr, ulWriteSectorCnt);
	retval = Ahci_Dma_Out_Ex_Direct(hDevice, reg,exreg,trans_direction, buf_sec_align, ulReadSectorCnt);
	memcpy(readbuf,buf_sec_align,length);
	free(buf_sec_align);
	if( retval != TRUE ) 
	{   
		return RETURN_FAIL;
	}

	return RETURN_SUCCESS;
}
STATUS Ahci_Write_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf)
{
	U32 trans_direction;
    IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	BOOL retval;
	U32 ulWriteSectorCnt;
	U8 *buf_sec_align;

    reg.bFeaturesReg = OP_MEM_WRITE;//VD_WRITE_DRAM_SRAM;
    reg.bCommandReg = 0XFF;//ATA_CMD_VENDER_DEFINE;    
    reg.bDriveHeadReg = 0xE0;
	

    reg.bLBALowReg=(U8)((U32)startaddr)&0xff;
    reg.bLBAMidReg=(U8)((U32)startaddr>>8)&0xff;
    reg.bLBAHighReg=(U8)((U32)startaddr>>16)&0xff;
	reg.bSectorCountReg=(U8)((U32)startaddr>>24)&0xff;

	exreg.bLBALowReg = (U8)(length & 0xFF);
	exreg.bLBAMidReg = ((length>>8) & 0xFF);  
	exreg.bFeaturesReg = cpuid;

	ulWriteSectorCnt = (length>>9)+((length&0x1ff)>0?1:0);
	buf_sec_align = (U8 *)malloc(ulWriteSectorCnt*SEC_SIZE);
	memcpy(buf_sec_align,writebuf,length);
	trans_direction = DATA_TRANSFER_OUT;

//	retval = sata_dma_cmd_ex(disk, reg,exreg,trans_direction, outbuf_addr, ulWriteSectorCnt);
	retval = Ahci_Dma_Out_Ex_Direct(hDevice, reg,exreg,trans_direction,buf_sec_align, ulWriteSectorCnt);
	
	free(buf_sec_align);
	if( retval != TRUE ) 
	{   
		return RETURN_FAIL;
	}
	

	return RETURN_SUCCESS;
}

STATUS Ahci_Read_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk)
{
	U32 trans_direction;
    IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	BOOL retval;
	U32 ulWriteSectorCnt;
	U8 befullpln;

    reg.bFeaturesReg = OP_FLASH_READ;//VD_WRITE_DRAM_SRAM;
    reg.bCommandReg = 0XFF;//ATA_CMD_VENDER_DEFINE;    
    reg.bDriveHeadReg = 0xE0;

    reg.bLBALowReg=(U8)((U32)pumsk)&0xff;
    reg.bLBAMidReg=(U8)((U32)pumsk>>8)&0xff;
    reg.bLBAHighReg=(U8)((U32)pumsk>>16)&0xff;
	reg.bSectorCountReg=(U8)((U32)pumsk>>24)&0xff;

	exreg.bLBALowReg = (U8)(block & 0xFF);
	exreg.bLBAMidReg = ((block>>8) & 0xFF);
	exreg.bLBAHighReg = page&0xff;
	//befullpln = (plnmode==MULTI_PLN?0:1);
	//befullpln = (pln==MULTI_PLN?1:0);
	befullpln = plnmode;
	exreg.bSectorCountReg = CACULATE_PG_PLN(page,pln,befullpln);//(page>>8)|((pln&0xf)<<1)|((befullpln&0X1)<<5);
	exreg.bFeaturesReg = cpuid;

	//length = Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
    ulWriteSectorCnt = 1;//(length>>9)+((length&0x1ff)>0?1:0);

	trans_direction = DATA_TRANSFER_IN;

	retval = Ahci_Dma_Out_Ex_Direct(hDevice, reg,exreg,trans_direction,pStatusBuf, ulWriteSectorCnt);
  
	if( retval != TRUE ) 
	{   
		return RETURN_FAIL;
	}
	
	return RETURN_SUCCESS;
}
STATUS Ahci_Write_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk)
{
	U32 trans_direction;
    IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	BOOL retval;
	U32 ulWriteSectorCnt;
	U8 befullpln;

    reg.bFeaturesReg = OP_FLASH_WRITE;//VD_WRITE_DRAM_SRAM;
    reg.bCommandReg = 0XFF;//ATA_CMD_VENDER_DEFINE;    
    reg.bDriveHeadReg = 0xE0;

    reg.bLBALowReg=(U8)((U32)pumsk)&0xff;
    reg.bLBAMidReg=(U8)((U32)pumsk>>8)&0xff;
    reg.bLBAHighReg=(U8)((U32)pumsk>>16)&0xff;
	reg.bSectorCountReg=(U8)((U32)pumsk>>24)&0xff;

	exreg.bLBALowReg = (U8)(block & 0xFF);
	exreg.bLBAMidReg = ((block>>8) & 0xFF);
	exreg.bLBAHighReg = page&0xff;
	//befullpln = (plnmode==MULTI_PLN?0:1);
	//befullpln = (pln==MULTI_PLN?0:1);
	befullpln = plnmode;
	exreg.bSectorCountReg = CACULATE_PG_PLN(page,pln,befullpln);
	exreg.bFeaturesReg = cpuid;

    //ulWriteSectorCnt = 64;//FORCE 32K TRANSFER
	//length = Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
    ulWriteSectorCnt = 1;//(length>>9)+((length&0x1ff)>0?1:0);

	trans_direction = DATA_TRANSFER_IN;

	retval = Ahci_Dma_Out_Ex_Direct(hDevice, reg,exreg,trans_direction,pStatusBuf, ulWriteSectorCnt);
  
	if( retval != TRUE ) 
	{   
		return RETURN_FAIL;
	}
	
	return RETURN_SUCCESS;
}
STATUS Ahci_Erase_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U8 *pStatusBuf,U32 pumsk)
{
	U32 trans_direction;
	U8 befullpln;
    IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	BOOL retval;
	U32 ulWriteSectorCnt;
	

    reg.bFeaturesReg = OP_FLASH_ERASE;//VD_WRITE_DRAM_SRAM;
    reg.bCommandReg = 0XFF;//ATA_CMD_VENDER_DEFINE;    
    reg.bDriveHeadReg = 0xE0;

    reg.bLBALowReg=(U8)((U32)pumsk)&0xff;
    reg.bLBAMidReg=(U8)((U32)pumsk>>8)&0xff;
    reg.bLBAHighReg=(U8)((U32)pumsk>>16)&0xff;
	reg.bSectorCountReg=(U8)((U32)pumsk>>24)&0xff;

	exreg.bLBALowReg = (U8)(block & 0xFF);
	exreg.bLBAMidReg = ((block>>8) & 0xFF);
	exreg.bFeaturesReg = cpuid;
	befullpln = plnmode;
	exreg.bSectorCountReg = CACULATE_PG_PLN(0,pln,befullpln);
    //ulWriteSectorCnt = 64;//FORCE 32K TRANSFER
	//length = Api_Caculate_Pu_Cnt(pumsk)*sizeof(VD_NFC_STATUS);
    ulWriteSectorCnt = 1;//(length>>9)+((length&0x1ff)>0?1:0);

	trans_direction = DATA_TRANSFER_IN;

	retval = Ahci_Dma_Out_Ex_Direct(hDevice, reg,exreg,trans_direction,pStatusBuf, ulWriteSectorCnt);
  
	if( retval != TRUE ) 
	{   
		return RETURN_FAIL;
	}
	
	return RETURN_SUCCESS;
}

STATUS Ahci_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2)
{
	IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	IDEREGSEX outreg = {0};
    IDEREGSEX outexreg = {0};
	BOOL retval;
	reg.bCommandReg = 0xff;
	reg.bFeaturesReg = ucOpCode;
	reg.bLBALowReg = (ulInParam1&0xff);
	reg.bLBAMidReg = ((ulInParam1>>8)&0xff);
	reg.bLBAHighReg = ((ulInParam1>>16)&0xff);
	reg.bSectorCountReg = ((ulInParam1>>24)&0xff);

	exreg.bLBALowReg = (ulInParam2&0xff);
	exreg.bLBAMidReg = ((ulInParam2>>8)&0xff);
	exreg.bLBAHighReg = ((ulInParam2>>16)&0xff);
	exreg.bSectorCountReg = ((ulInParam2>>24)&0xff);
	exreg.bFeaturesReg = ucCpuId;
	retval = Ahci_NonData_Cmd_Ex(hDevice,reg,exreg,&outreg,&outexreg);
	if(retval == TRUE)
	{
		if (NULL != pOutParam1)
		{
			*pOutParam1 = (outreg.bLBALowReg)|((outreg.bLBAMidReg)<<8)|((outreg.bLBAHighReg)<<16)|((outreg.bSectorCountReg)<<24);
		}
		if (NULL != pOutParam2)
		{
			//*pOutParam2 = (outexreg.bLBALowReg)|((outexreg.bLBAMidReg)<<8)|((outexreg.bLBAHighReg)<<16)|((outexreg.bSectorCountReg)<<24);
		}
		return RETURN_SUCCESS;
	}
	else
	{
		return RETURN_FAIL;
	}
}
BOOL Ahci_Rom_Read_Reg(HANDLE hDevice,U32 ulAddr,U32 * pData)
{
	IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	IDEREGSEX outreg = {0};
    IDEREGSEX outexreg = {0};
	BOOL retval;
	reg.bCommandReg = 0xff;
	reg.bFeaturesReg = 0xc0;
	
	reg.bSectorCountReg = (ulAddr&0xff);
	reg.bLBALowReg = ((ulAddr>>8)&0xff);
	reg.bLBAMidReg = ((ulAddr>>16)&0xff);
	reg.bLBAHighReg = ((ulAddr>>24)&0xff);

	reg.bDriveHeadReg = 0xE0;
	retval = Ahci_NonData_Cmd_Ex(hDevice,reg,exreg,&outreg,&outexreg);
	if(retval == TRUE)
	{
		if (NULL != pData)
		{
			
			*pData = (outreg.bSectorCountReg)|((outreg.bLBALowReg)<<8)|((outreg.bLBAMidReg)<<16)|((outreg.bLBAHighReg)<<24);
		}
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
BOOL Ahci_Rom_Write_Reg(HANDLE hDevice,U32 ulAddr,U32 ulData)
{
	IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	IDEREGSEX outreg = {0};
    IDEREGSEX outexreg = {0};
	BOOL retval;
	reg.bCommandReg = 0xff;
	reg.bFeaturesReg = 0xc1;
	reg.bDriveHeadReg = 0xE0;
	reg.bSectorCountReg = (ulAddr&0xff);
	reg.bLBALowReg = ((ulAddr>>8)&0xff);
	reg.bLBAMidReg = ((ulAddr>>16)&0xff);
	reg.bLBAHighReg = ((ulAddr>>24)&0xff);
	
	exreg.bSectorCountReg = (ulData&0xff);
	exreg.bLBALowReg = ((ulData>>8)&0xff);
	exreg.bLBAMidReg = ((ulData>>16)&0xff);
	exreg.bLBAHighReg = ((ulData>>24)&0xff);

	
	retval = Ahci_NonData_Cmd_Ex(hDevice,reg,exreg,&outreg,&outexreg);
	if(retval == TRUE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
BOOL Ahci_Rom_Write_Dma(void * pDevObj, U32 ulAddr, U32 * pBuf,U32 ulCnt)
{
	U32 ulRegAddr,ulRegData;
	U32 i;
	U32 * pData;
	pData = pBuf;
	for(i=0;i<(ulCnt>>2);i++)
	{
		ulRegAddr = ulAddr+i*4;
		ulRegData = *pData++;
		Ahci_Rom_Write_Reg((HANDLE)pDevObj,ulRegAddr,ulRegData);
	}
	return TRUE;
}
BOOL Ahci_Rom_Jump(HANDLE hDevice,U32 ulAddr)
{
	IDEREGSEX reg = {0};
    IDEREGSEX exreg = {0};
	IDEREGSEX outreg = {0};
    IDEREGSEX outexreg = {0};
	BOOL retval;
	reg.bCommandReg = 0xff;
	reg.bFeaturesReg = 0xc4;
	
	reg.bSectorCountReg = (ulAddr&0xff);
	reg.bLBALowReg = ((ulAddr>>8)&0xff);
	reg.bLBAMidReg = ((ulAddr>>16)&0xff);
	reg.bLBAHighReg = ((ulAddr>>24)&0xff);

	
	retval = Ahci_NonData_Cmd_Ex(hDevice,reg,exreg,&outreg,&outexreg);
	if(retval == TRUE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

STATUS Ahci_Fw_Download(HANDLE hDevice, U32 ulLength, U8 * pFwBuf)
{
    U32 ulWriteSectorCnt,ulRemLen,ulTransCnt;
    U8 *buf_sec_align;
    U32 trans_direction;
    IDEREGSEX reg = {0};
    BOOL retval;
    U32 ulOffset;
	
    ulWriteSectorCnt = (ulLength >> 9) + ((ulLength & 0x1ff)>0 ? 1 : 0);
    buf_sec_align = (U8 *)malloc(ulWriteSectorCnt*SEC_SIZE);
    ulRemLen = ulWriteSectorCnt;
    memcpy(buf_sec_align, pFwBuf, ulLength);
    ulOffset = 0;

    while(ulRemLen)
    {
        reg.bCommandReg = 0x93;
        reg.bFeaturesReg = 0x3;

		ulTransCnt = (ulRemLen > MAX_MICROCODE_TRANS_SECCNT ? MAX_MICROCODE_TRANS_SECCNT : ulRemLen);
        reg.bSectorCountReg = (U8)((U32)ulTransCnt) & 0xFF;
        reg.bLBALowReg = (U8)((U32)ulTransCnt >> 8) &0xFF;
        
        reg.bLBAMidReg = (U8)((U32)ulOffset) & 0xFF;
        reg.bLBAHighReg = (U8)((U32)ulOffset>> 8) &0xFF;
        
		//reg.bDriveHeadReg = 0xE0;
        trans_direction = DATA_TRANSFER_OUT;

		retval = Ahci_Dma_Out_Direct(hDevice, reg, trans_direction, (buf_sec_align + (ulOffset << 9)), ulTransCnt);
        if( retval != TRUE ) 
        {  
            free(buf_sec_align);
            return RETURN_FAIL;
        }

        ulRemLen -= ulTransCnt;
		ulOffset += ulTransCnt;
    }

    free(buf_sec_align);
    return RETURN_SUCCESS;

	
}

STATUS Ahci_Get_Smart_Data(HANDLE hDevice, U8 *readbuf)
{
    unsigned size;
    void * inbuf;
    U32 inbufsz;
    void * outbuf;
    U32 outbufsz;
    U32 bytes_rtn;

    IDEREGS reg;
    ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;
    
    reg.bFeaturesReg = READ_ATTRIBUTES;
    reg.bCommandReg = SMART_CMD;
    reg.bCylHighReg = SMART_CYL_HI;
    reg.bCylLowReg = SMART_CYL_LOW;

    apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
    apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
    apt_ex_with_buf.apt.DataTransferLength = READ_ATTRIBUTE_BUFFER_SIZE;
    apt_ex_with_buf.apt.TimeOutValue = 5;

    size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
    apt_ex_with_buf.apt.DataBufferOffset = size;

    memcpy((void*)apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);

    inbuf = (void*)&(apt_ex_with_buf);
    inbufsz = apt_ex_with_buf.apt.Length;
    outbuf = (void*)&(apt_ex_with_buf);
    outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;

    if (!DeviceIoControl(hDevice, IOCTL_ATA_PASS_THROUGH,
        inbuf, inbufsz, outbuf, outbufsz,
        (LPDWORD)&bytes_rtn, NULL))
    {
        return RETURN_FAIL;
    }

    memcpy((void *)&(reg), (void*)apt_ex_with_buf.apt.CurrentTaskFile, 8);
    if (reg.bFeaturesReg != 0)
    {
        return RETURN_SUCCESS;
    }
    memcpy((void*)readbuf, (void*)apt_ex_with_buf.data_buf, READ_ATTRIBUTE_BUFFER_SIZE);

    return RETURN_SUCCESS;
}