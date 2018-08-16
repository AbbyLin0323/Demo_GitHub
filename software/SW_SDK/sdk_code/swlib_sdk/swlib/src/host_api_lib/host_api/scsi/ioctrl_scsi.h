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
  File Name     : ioctrl_scsi.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the data structure and macro for scsi_iotrl
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _SCSI_IOCTRL
#define _SCSI_IOCTRL
#include <ntddscsi.h>
#define DATA_LENGTH ( 128 * 1024 )
#define SENSEDATASIZE 18
#define SCSI_MAX_WT_LEN (8*1024)
#define SCSI_MAX_RD_LEN (8*1024)//(32*1024)
typedef enum _TRANS_MODE{
	DMA_READ,
	DMA_WRITE,
	NONE_DATA
}TRANS_MODE;
typedef struct _TEST_DESC
{
	const BYTE* Cdb;
	UCHAR CdbLength;
	UCHAR Directory;
	DWORD DataLength;
	TCHAR* Description;
}TEST_DESC, *PTEST_DESC;
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
typedef struct _CDB_EXT
{
	unsigned char opcode;
	
	unsigned char extend:1;
	unsigned char protocol:4;
	unsigned char multiple_count:3;

	unsigned char t_length:2;
	unsigned char byte_block:1;
	unsigned char t_dir:1;
	unsigned char rev1:1;
	unsigned char ck_cond:1;
	unsigned char off_line:2;

	unsigned char features_h;
	unsigned char features_l;

	unsigned char sector_count_h;
	unsigned char sector_count_l;

	unsigned char lba_low_h;
	unsigned char lba_low_l;

	unsigned char lba_mid_h;
	unsigned char lba_mid_l;

	unsigned char lba_high_h;
	unsigned char lba_high_l;

	unsigned char device;
	unsigned char command;
	unsigned char control;

}CDB_EXT;
typedef struct _REGISTER_REQUEST
{
	SCSI_PASS_THROUGH ScsiRequest;  
	BYTE              SenseData[ SENSEDATASIZE ];
	BYTE             Data[DATA_LENGTH];

}REGISTER_REQUEST, *PREGISTER_REQUEST;
BOOL Scsi_Ioctrl(HANDLE hDevice,U8 * pRegCdb,U8 ucCdbLen,U8 ucDir,U32 ulDataLen,U8 * pDataBuf);
BOOL Scsi_Ioctrl_Ex(HANDLE hDevice, U8 * pRegCdb, U8 ucCdbLen, U8 ucDir, U32 ulDataLen, U8 * pDataBuf);
#endif