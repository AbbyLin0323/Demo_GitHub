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
  File Name     : ioctrl_ahci.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the basic ioctl function of ahci.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/
#include "ioctrl_ahci.h"
BOOL Ahci_Dma_Out_Direct
(
 HANDLE disk,
 IDEREGSEX reg,
 U32 direction,
 U8 * buf_addr,
 U32 trans_sec_cnt
 )
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	U32 trans_len = trans_sec_cnt << 9;

	IDEREGSEX outreg;

  
	ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS apt_ex_with_buf;
  
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGSEX));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGSEX));
	
	memcpy((void*)apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);    

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_DIRECT);

	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
       //DBG_Getch();
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.AtaFlags |= (ATA_FLAGS_USE_DMA);
	//apt_ex_with_buf.apt.AtaFlags |= ATA_FLAGS_USE_DMA;

	apt_ex_with_buf.apt.DataTransferLength = trans_len;
	apt_ex_with_buf.apt.TimeOutValue = 10;

	size = offsetof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS, data_buf);
	//apt_ex_with_buf.apt.DataBufferOffset = size;
	apt_ex_with_buf.apt.DataBuffer = &apt_ex_with_buf.data_buf[0];

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	if(DATA_TRANSFER_IN == direction) /*DMA_IN*/
	{
		inbufsz = apt_ex_with_buf.apt.Length;
		outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;

    	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH_DIRECT,
    		inbuf, inbufsz, outbuf, outbufsz, 
    		(LPDWORD)&bytes_rtn, NULL)) 
    	{
    		printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}
	else  /*DMA_OUT*/
	{
	//	inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		//inbufsz = sizeof(ATA_PASS_THROUGH_EX) + apt_ex_with_buf.apt.DataTransferLength;
       inbufsz = sizeof(ATA_PASS_THROUGH_DIRECT);
  
		memcpy((void*)apt_ex_with_buf.data_buf, (void*)buf_addr, trans_len);   
		outbufsz = apt_ex_with_buf.apt.Length;

    	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH_DIRECT,
    		inbuf, inbufsz, outbuf, outbufsz, 
    		(LPDWORD)&bytes_rtn, NULL)) 
    	{
    		printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}

	memcpy((void *)&(outreg), (void*)apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction)
	{
		memcpy((void*)buf_addr, (void*)apt_ex_with_buf.data_buf, trans_len);
	}

	return TRUE;
}
BOOL Ahci_Dma_Out_Ex_Direct
(
 HANDLE disk,
 IDEREGSEX reg,
 IDEREGSEX exreg,
 U32 direction,
 U8 * buf_addr,
 U32 trans_sec_cnt
 )
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	U32 trans_len = trans_sec_cnt << 9;

	IDEREGSEX outreg;

  
	ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS apt_ex_with_buf;
  
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGSEX));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGSEX));
	memcpy((void*)apt_ex_with_buf.apt.PreviousTaskFile, (void *)(&exreg), 8);
	memcpy((void*)apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);    

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_DIRECT);

	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
       //DBG_Getch();
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.AtaFlags |= (ATA_FLAGS_USE_DMA | ATA_FLAGS_48BIT_COMMAND);
	//apt_ex_with_buf.apt.AtaFlags |= ATA_FLAGS_USE_DMA;

	apt_ex_with_buf.apt.DataTransferLength = trans_len;
	apt_ex_with_buf.apt.TimeOutValue = 10;

	size = offsetof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS, data_buf);
	//apt_ex_with_buf.apt.DataBufferOffset = size;
	apt_ex_with_buf.apt.DataBuffer = &apt_ex_with_buf.data_buf[0];

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	if(DATA_TRANSFER_IN == direction) /*DMA_IN*/
	{
		inbufsz = apt_ex_with_buf.apt.Length;
		outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;

    	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH_DIRECT,
    		inbuf, inbufsz, outbuf, outbufsz, 
    		(LPDWORD)&bytes_rtn, NULL)) 
    	{
    		printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}
	else  /*DMA_OUT*/
	{
	//	inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		//inbufsz = sizeof(ATA_PASS_THROUGH_EX) + apt_ex_with_buf.apt.DataTransferLength;
       inbufsz = sizeof(ATA_PASS_THROUGH_DIRECT);
  
		memcpy((void*)apt_ex_with_buf.data_buf, (void*)buf_addr, trans_len);   
		outbufsz = apt_ex_with_buf.apt.Length;

    	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH_DIRECT,
    		inbuf, inbufsz, outbuf, outbufsz, 
    		(LPDWORD)&bytes_rtn, NULL)) 
    	{
    		printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}

	memcpy((void *)&(outreg), (void*)apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction)
	{
		memcpy((void*)buf_addr, (void*)apt_ex_with_buf.data_buf, trans_len);
	}

	return TRUE;
}
BOOL Ahci_NonData_Cmd_Ex
(
 HANDLE disk,
 IDEREGSEX reg,
 IDEREGSEX exreg,
 IDEREGSEX *p_outreg,
 IDEREGSEX *p_outreg_ext
 )
{
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	//IDEREGS outreg;
	ATA_PASS_THROUGH_EX apt;

	memset(&apt, 0, sizeof(ATA_PASS_THROUGH_EX));

	assert(sizeof(apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt.PreviousTaskFile) == sizeof(IDEREGS));

	memcpy((void*)apt.CurrentTaskFile, (void *)(&reg), 8);
	memcpy((void*)apt.PreviousTaskFile, (void *)(&exreg), sizeof(IDEREGS));

	apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	apt.TimeOutValue = 10;
	apt.AtaFlags |= (ATA_FLAGS_48BIT_COMMAND);

	inbuf = (void*)&(apt);
	inbufsz = apt.Length;
	outbuf = (void*)&(apt);
	outbufsz = apt.Length;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		(LPDWORD)&bytes_rtn, NULL)) 
	{
		printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

#if 0
	memcpy((void *)&(outreg), apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}
#else
	memcpy((void *)(p_outreg), (void*)apt.CurrentTaskFile, 8);
#if 0
	if( p_outreg->bFeaturesReg != 0 ) 
	{   
		printf("read return status error, check parameters.\n");
		return FALSE;
	}
#endif
	memcpy((void *)(p_outreg_ext), (void*)apt.PreviousTaskFile, 8);
#if 0
	if( p_outreg_ext->bFeaturesReg != 0 ) 
	{   
		printf("read return status error, check parameters.\n");
		return FALSE;
	}
#endif
#endif
	return TRUE;
}