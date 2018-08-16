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
  File Name     : ioctrl_nvme.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the ioctrl function of nvme.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include "ioctrl_nvme.h"
NVME_IO_STATUS Nvme_IoCtrl(HANDLE hDevice,UCHAR DataTX,ULONG ByteSize,PVOID PDataBuf,ULONG * PNvmeCmd,ULONG *PCplEntry)
{
	BOOL Status = 0;
	ULONG Count = 0;
	ULONG InputBufLen = 0;
	
	PNVME_PASS_THROUGH_IOCTL pIoBuffer = NULL;
	

	if (DataTX == NVME_NO_DATA_TX)
	{
		/* Allocate input buffer to accommodate size of NVME_PASS_THRUGH_IOCTL only */
		InputBufLen = sizeof(NVME_PASS_THROUGH_IOCTL);
		pIoBuffer = (PNVME_PASS_THROUGH_IOCTL) malloc(InputBufLen); 
	}
	else if (DataTX == NVME_FROM_HOST_TO_DEV)
	{
		/* Allocate input buffer to accommodate size of NVME_PASS_THRUGH_IOCTL and data */
		InputBufLen = sizeof(NVME_PASS_THROUGH_IOCTL) + ByteSize;
		pIoBuffer = (PNVME_PASS_THROUGH_IOCTL) malloc(InputBufLen); 
	
		
	}
	else if (DataTX == NVME_FROM_DEV_TO_HOST)
	{
		/* Allocate input buffer to accommodate size of NVME_PASS_THRUGH_IOCTL only */
		InputBufLen = sizeof(NVME_PASS_THROUGH_IOCTL) + ByteSize;
		pIoBuffer = (PNVME_PASS_THROUGH_IOCTL) malloc(InputBufLen); 
	
		
	}
	else {
		return NVME_IO_FAIL;
	}


	/* Zero out the buffers */
	memset(pIoBuffer, 0, InputBufLen);


	/* Populate SRB_IO_CONTROL fields in input buffer */
	pIoBuffer->VendorSpecific[0] = (DWORD) 0;
    pIoBuffer->VendorSpecific[1] = (DWORD) 0;
	pIoBuffer->SrbIoCtrl.ControlCode = NVME_PASS_THROUGH_SRB_IO_CODE;
	pIoBuffer->SrbIoCtrl.HeaderLength = sizeof(SRB_IO_CONTROL);
	memcpy((UCHAR*)(&pIoBuffer->SrbIoCtrl.Signature[0]), NVME_SIG_STR, NVME_SIG_STR_LEN);
	pIoBuffer->SrbIoCtrl.Timeout = NVME_PT_TIMEOUT;
	pIoBuffer->SrbIoCtrl.Length = InputBufLen - sizeof(SRB_IO_CONTROL);
	if (DataTX == NVME_FROM_HOST_TO_DEV)
		pIoBuffer->DataBufferLen = ByteSize;
	else
		pIoBuffer->DataBufferLen = 0;
	pIoBuffer->QueueId = 0;
	
	pIoBuffer->ReturnBufferLen = InputBufLen;//pIoBuffer->DataBufferLen+sizeof(NVME_PASS_THROUGH_IOCTL);
	pIoBuffer->Direction = DataTX;
	/* Fill in pIoBuffer->NVMeCmd here */
	memcpy(pIoBuffer->NVMeCmd,PNvmeCmd,NVME_IOCTL_CMD_DW_SIZE*sizeof(ULONG));
	/* Fill pIoBuffer->DataBuffer here when transferring data to device */
	if (DataTX == NVME_FROM_HOST_TO_DEV)
	{
		memcpy(pIoBuffer->DataBuffer,PDataBuf,ByteSize);
	}
	Status = DeviceIoControl(
							   hDevice, /* Handle to \\.\scsi device via CreateFile */
							   IOCTL_SCSI_MINIPORT,   /* IO control function to a miniport driver */
							   pIoBuffer,                      /* Input buffer with data sent to driver */
							   InputBufLen,                          /* Length of data sent to driver (in bytes) */
							   pIoBuffer,                             /* Output buffer with data received from driver */
							   InputBufLen,                       /* Length of data received from driver */
							   &Count,                                  /* Bytes placed in DataBuffer */
							   NULL);                                   /* NULL = no overlap */

	if(Status == FALSE)
		return NVME_IO_FAIL;
	if (DataTX == NVME_FROM_DEV_TO_HOST)
	{
		memcpy(PDataBuf,pIoBuffer->DataBuffer,ByteSize);
	}
	if(PCplEntry!=NULL)
	{
		memcpy(PCplEntry,pIoBuffer->CplEntry,NVME_IOCTL_COMPLETE_DW_SIZE);
	}
	free(pIoBuffer);
	
	return NVME_IO_SUCCESS;
}