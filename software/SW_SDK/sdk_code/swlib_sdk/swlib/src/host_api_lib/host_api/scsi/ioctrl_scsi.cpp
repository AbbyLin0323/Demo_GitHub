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
  File Name     : ioctrl_scsi.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines ioctrl function of scsi.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#include "hal_scsi.h"
#include "ioctrl_scsi.h"
//#include "ntddscsi.h"

BOOL Scsi_Ioctrl(HANDLE hDevice,U8 * pRegCdb,U8 ucCdbLen,U8 ucDir,U32 ulDataLen,U8 * pDataBuf)
{
	PREGISTER_REQUEST req,reqout;
	U32 cnt;//,buf_addr,dev_mem_addr,rem_data_len,trans_data_len,max_trans_len;
	BOOL ret,status;
	status = TRUE;
	req = new REGISTER_REQUEST;
	reqout = new REGISTER_REQUEST;
	memset( req, 0, sizeof( SCSI_PASS_THROUGH ) + SENSEDATASIZE );
	memset(reqout, 0, sizeof(SCSI_PASS_THROUGH) + SENSEDATASIZE);

	req->ScsiRequest.Length= sizeof (SCSI_PASS_THROUGH);
	req->ScsiRequest.CdbLength = ucCdbLen;
	req->ScsiRequest.SenseInfoLength = 0x12;
	req->ScsiRequest.DataTransferLength = ulDataLen;
	req->ScsiRequest.TimeOutValue = 0x14;
	req->ScsiRequest.DataBufferOffset = (ULONG_PTR)(sizeof (SCSI_PASS_THROUGH) + SENSEDATASIZE );
	req->ScsiRequest.SenseInfoOffset = sizeof (SCSI_PASS_THROUGH);
	req->ScsiRequest.DataIn = ucDir;
	memcpy( req->ScsiRequest.Cdb, pRegCdb, ucCdbLen);
	if((ucDir==SCSI_IOCTL_DATA_OUT)&&(pDataBuf!=NULL))
	{
		memcpy(req->Data,(char *)(pDataBuf),ulDataLen);
	}
		
	ret = DeviceIoControl(
				hDevice,
				IOCTL_SCSI_PASS_THROUGH,
				//IOCTL_SCSI_PASS_THROUGH_EX,
				req,
				sizeof(REGISTER_REQUEST),//Data lenght should be the same with the buffer size
				reqout,
				sizeof(REGISTER_REQUEST),
				(LPDWORD)&cnt,
				FALSE);
	//*scsi_err = (U32)req->ScsiRequest.ScsiStatus;
	if(!ret)
	{
		status = FALSE;
	}
	else
	{
		if(ucDir==SCSI_IOCTL_DATA_IN&&(pDataBuf!=NULL))
		{
			memcpy((char *)(pDataBuf),reqout->Data,ulDataLen);
		}
	}
	delete req;
	delete reqout;
	
	return status;
}
BOOL Scsi_Ioctrl_Ex(HANDLE hDevice, U8 * pRegCdb, U8 ucCdbLen, U8 ucDir, U32 ulDataLen, U8 * pDataBuf)
{
	PREGISTER_REQUEST req, reqout;
	U32 cnt;//,buf_addr,dev_mem_addr,rem_data_len,trans_data_len,max_trans_len;
	BOOL ret, status;
	status = TRUE;
	req = new REGISTER_REQUEST;
	reqout = new REGISTER_REQUEST;
	memset(req, 0, sizeof(SCSI_PASS_THROUGH) + SENSEDATASIZE);
	memset(reqout, 0, sizeof(SCSI_PASS_THROUGH) + SENSEDATASIZE);

	req->ScsiRequest.Length = sizeof(SCSI_PASS_THROUGH);
	req->ScsiRequest.CdbLength = ucCdbLen;
	req->ScsiRequest.SenseInfoLength = 0x12;
	req->ScsiRequest.DataTransferLength = ulDataLen;
	req->ScsiRequest.TimeOutValue = 0x14;
	req->ScsiRequest.DataBufferOffset = (ULONG_PTR)(sizeof(SCSI_PASS_THROUGH) + SENSEDATASIZE);
	req->ScsiRequest.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH);
	req->ScsiRequest.DataIn = ucDir;
	memcpy(req->ScsiRequest.Cdb, pRegCdb, ucCdbLen);
	if ((ucDir == SCSI_IOCTL_DATA_OUT) && (pDataBuf != NULL))
	{
		memcpy(req->Data, (char *)(pDataBuf), ulDataLen);
	}

	ret = DeviceIoControl(
		hDevice,
		IOCTL_SCSI_PASS_THROUGH,
		req,
		sizeof(REGISTER_REQUEST),//Data lenght should be the same with the buffer size
		reqout,
		sizeof(REGISTER_REQUEST),
		(LPDWORD)&cnt,
		FALSE);
	//*scsi_err = (U32)req->ScsiRequest.ScsiStatus;
	if (!ret)
	{
		status = FALSE;
	}
	else
	{
		if (ucDir == SCSI_IOCTL_DATA_IN && (pDataBuf != NULL))
		{
			memcpy((char *)(pDataBuf), reqout->Data, ulDataLen);
		}
	}
	delete req;
	delete reqout;

	return status;
}