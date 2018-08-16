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
  File Name     : hal_nvme.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the hal function of nvme.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include "hal_nvme.h"
#include "ioctrl_nvme.h"
#include "nvme.h"
#include "..\\host_api.h"
STATUS Nvme_Get_Identify_Data(HANDLE hDevice,U8 * pReadBuf)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	PADMIN_IDENTIFY_COMMAND_DW10 pDw10;
	ULONG cnt = sizeof(NVME_PASS_THROUGH_IOCTL);
	//fill NvmeCmd
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = ADMIN_IDENTIFY;
	pDw10 = (PADMIN_IDENTIFY_COMMAND_DW10)&NvmeCmd.CDW10;
	pDw10->CNS = 1;
	
	IoStatus = Nvme_IoCtrl(hDevice,NVME_FROM_DEV_TO_HOST,sizeof(ADMIN_IDENTIFY_CONTROLLER),pReadBuf,(ULONG *)&NvmeCmd,NULL);

	if(IoStatus==NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
}
STATUS Nvme_Get_ModelNumber(HANDLE hDevice,U8 * identifybuf,U8 mn[40])
{
	PADMIN_IDENTIFY_CONTROLLER pidentify;
	pidentify = (PADMIN_IDENTIFY_CONTROLLER)identifybuf;
	memcpy(mn,pidentify->MN,40);
	return RETURN_SUCCESS;
}
STATUS Nvme_Fw_Download(HANDLE hDevice, U32 ulLength, U8 * pFwBuf)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	DW10_COMMIT DwCommit;
	U8 *buf_sec_align;
	U8 * pbuf;
	U32 ulReadSectorCnt,ulRemLen,ulTransCnt;
	/*CID in DWORD0 is handled by driver app no need to care*/
	ulReadSectorCnt = (ulLength >> 9) + ((ulLength & 0x1ff)>0 ? 1 : 0);
	buf_sec_align = (U8 *)malloc(ulReadSectorCnt*SEC_SIZE);
	ulRemLen = ulReadSectorCnt*SEC_SIZE;
	memcpy(buf_sec_align, pFwBuf, ulLength);
	pbuf = buf_sec_align;
	while (ulRemLen)
	{
		memset(&NvmeCmd, 0, sizeof(NVMe_COMMAND));
		NvmeCmd.CDW0.OPC = 0x11;
		ulTransCnt = (ulRemLen > MAX_NVME_TRANS_CNT ? MAX_NVME_TRANS_CNT : ulRemLen);
		NvmeCmd.CDW10 = (ulTransCnt >> 2)-1;
		NvmeCmd.CDW11 = (((U32)pbuf-(U32)buf_sec_align)>>2);

		IoStatus = Nvme_IoCtrl(hDevice, NVME_FROM_HOST_TO_DEV, ulTransCnt, pbuf, (ULONG *)&NvmeCmd, NULL);
		
		if (IoStatus != NVME_IO_SUCCESS)
		
		{
			free(buf_sec_align);
			return RETURN_FAIL;
		}

		ulRemLen -= ulTransCnt;
		pbuf += ulTransCnt;
	}
	free(buf_sec_align);
	memset(&NvmeCmd, 0, sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0x10;
	memset((U8 *)&DwCommit,0,4);
	DwCommit.FS = 2;
	DwCommit.CA = 1;
	memcpy((U8 *)&NvmeCmd.CDW10, (U8 *)&DwCommit,4);
	IoStatus = Nvme_IoCtrl(hDevice, NVME_NO_DATA_TX, NULL, NULL, (ULONG *)&NvmeCmd, NULL);

	if (IoStatus == NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
	
}
STATUS Nvme_Read_Dram_Sram(HANDLE hDevice,U8 ucCpuId,U32 ulStartAddr,U32 ulLength,U8 * readbuf)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	U8 *buf_sec_align;
	U32 ulReadSectorCnt;
	/*CID in DWORD0 is handled by driver app no need to care*/
	ulReadSectorCnt = (ulLength>>9)+((ulLength&0x1ff)>0?1:0);
	buf_sec_align = (U8 *)malloc(ulReadSectorCnt*SEC_SIZE);
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0xff;
	NvmeCmd.CDW12 = (OP_MEM_READ|(ucCpuId<<8));
	NvmeCmd.CDW13 = ulStartAddr;
	NvmeCmd.CDW14 = ulLength;
	NvmeCmd.NDP = (ulLength>>2);

	IoStatus = Nvme_IoCtrl(hDevice,NVME_FROM_DEV_TO_HOST,ulReadSectorCnt*SEC_SIZE,buf_sec_align,(ULONG *)&NvmeCmd,NULL);
	memcpy(readbuf,buf_sec_align,ulLength);
	free(buf_sec_align);
	if(IoStatus==NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
}
STATUS Nvme_Write_Dram_Sram(HANDLE hDevice,U8 ucCpuId,U32 ulStartAddr,U32 ulLength,U8 * writebuf)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	U32 ulWriteSectorCnt;
	U8 *buf_sec_align;
	ulWriteSectorCnt = (ulLength >> 9) + ((ulLength & 0x1ff)>0 ? 1 : 0);
	buf_sec_align = (U8 *)malloc(ulWriteSectorCnt*SEC_SIZE);
	memcpy(buf_sec_align, writebuf, ulLength);
	/*CID in DWORD0 is handled by driver app no need to care*/
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0xff;
	NvmeCmd.CDW12 = (OP_MEM_WRITE|(ucCpuId<<8));
	NvmeCmd.CDW13 = ulStartAddr;
	NvmeCmd.CDW14 = ulLength;
	NvmeCmd.NDP = (ulLength>>2);

	IoStatus = Nvme_IoCtrl(hDevice, NVME_FROM_HOST_TO_DEV, ulWriteSectorCnt*SEC_SIZE, buf_sec_align, (ULONG *)&NvmeCmd, NULL);
	free(buf_sec_align);
	if(IoStatus==NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
}
STATUS Nvme_Read_Flash(HANDLE hDevice,U8 ucCpuId,U8 ucPlnMode,U8 ucPln,U16 usBlock,U16 usPage,U8 *pStatusBuf,U32 ulPuMsk)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	
	/*CID in DWORD0 is handled by driver app no need to care*/
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0xff;
	NvmeCmd.CDW12 = (OP_FLASH_READ|(ucCpuId<<8));
	NvmeCmd.CDW13 = ulPuMsk;
	NvmeCmd.CDW14 = (usBlock)|((usPage&0xff)<<16)|(((usPage>>8)<<24)|((ucPln&0xf)<<25)|(ucPlnMode<<29));
	NvmeCmd.NDP = 32*(sizeof(VD_NFC_STATUS)>>2);
	IoStatus = Nvme_IoCtrl(hDevice,NVME_FROM_DEV_TO_HOST,HOST_BUFF_SIZE,pStatusBuf,(ULONG *)&NvmeCmd,NULL);
	if(IoStatus==NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
	
}
STATUS Nvme_Write_Flash(HANDLE hDevice,U8 ucCpuId,U8 ucPlnMode,U8 ucPln,U16 usBlock,U16 usPage,U8 *pStatusBuf,U32 ulPuMsk)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	/*CID in DWORD0 is handled by driver app no need to care*/
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0xff;
	NvmeCmd.CDW12 = (OP_FLASH_WRITE|(ucCpuId<<8));
	NvmeCmd.CDW13 = ulPuMsk;
	NvmeCmd.CDW14 = (usBlock)|((usPage&0xff)<<16)|(((usPage>>8)<<24)|((ucPln&0xf)<<25)|(ucPlnMode<<29));
	

	NvmeCmd.NDP = 32*(sizeof(VD_NFC_STATUS)>>2);
	IoStatus = Nvme_IoCtrl(hDevice,NVME_FROM_DEV_TO_HOST,HOST_BUFF_SIZE,pStatusBuf,(ULONG *)&NvmeCmd,NULL);
	if(IoStatus==NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
}
STATUS Nvme_Erase_Flash(HANDLE hDevice,U8 ucCpuId,U8 ucPlnMode,U8 ucPln,U16 usBlock,U8 *pStatusBuf,U32 ulPuMsk)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	/*CID in DWORD0 is handled by driver app no need to care*/
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0xff;
	NvmeCmd.CDW12 = (OP_FLASH_ERASE|(ucCpuId<<8));
	NvmeCmd.CDW13 = ulPuMsk;
	NvmeCmd.CDW14 = ((usBlock)|(ucPln&0xf)<<25|(ucPlnMode<<29));
	NvmeCmd.NDP = 32*(sizeof(VD_NFC_STATUS)>>2);
	IoStatus = Nvme_IoCtrl(hDevice,NVME_FROM_DEV_TO_HOST,HOST_BUFF_SIZE,pStatusBuf,(ULONG *)&NvmeCmd,NULL);
	if(IoStatus==NVME_IO_SUCCESS)
		return RETURN_SUCCESS;
	else
		return RETURN_FAIL;
	
}
STATUS Nvme_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2)
{
	NVME_IO_STATUS IoStatus;
	NVMe_COMMAND NvmeCmd;
	ULONG          CplEntry[NVME_IOCTL_COMPLETE_DW_SIZE];
	/*CID in DWORD0 is handled by driver app no need to care*/
	memset(&NvmeCmd,0,sizeof(NVMe_COMMAND));
	NvmeCmd.CDW0.OPC = 0xff;
	NvmeCmd.CDW12 = (ucOpCode|(ucCpuId<<8));
	NvmeCmd.CDW13 = ulInParam1;
	NvmeCmd.CDW14 = ulInParam2;
	
	IoStatus = Nvme_IoCtrl(hDevice,NVME_NO_DATA_TX,0,NULL,(ULONG *)&NvmeCmd,CplEntry);
	if((IoStatus==NVME_IO_SUCCESS) && (pOutParam1 != NULL))
	{
		*pOutParam1 = CplEntry[0];
		//*pOutParam2 = CplEntry[1];
		return RETURN_SUCCESS;
	}
	else
	{
		return RETURN_FAIL;
	}
	
}

STATUS Nvme_Get_Smart_Data(HANDLE hDevice, U8 * pReadBuf)
{
    /*Nothing to do, return fail*/
    return RETURN_FAIL;
}
