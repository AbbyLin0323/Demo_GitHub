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
  File Name     : ioctrl_nvme.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the data structure and macro for ioctrl_nvme.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _IOCTRL_NVME
#define _IOCTRL_NVME
#include <windows.h>
#include <ctype.h>
#include <fcntl.h>
#include "ntddscsi.h"
#include "host_api_define.h"

typedef enum _NVME_IO_STATUS 
{
	NVME_IO_SUCCESS = 0,
	NVME_IO_FAIL
}NVME_IO_STATUS;
#define ADMIN_IDENTIFY                                  0x06


#define NVME_STORPORT_DRIVER 0xE000
#define NVME_PASS_THROUGH_SRB_IO_CODE \
	CTL_CODE( NVME_STORPORT_DRIVER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS  )
#define NVME_GET_NAMESPACE_ID \
    CTL_CODE( NVME_STORPORT_DRIVER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS  )
#define IOCTL_SCSI_MINIPORT             CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define NVME_SIG_STR "NvmeMini"
#define NVME_SIG_STR_LEN                 8
#define NVME_NO_DATA_TX                0 // No data transfer involved
#define NVME_FROM_HOST_TO_DEV 1 // Transfer data from host to device
#define NVME_FROM_DEV_TO_HOST 2 // Transfer data from device to host
#define NVME_BI_DIRECTION               3 // Transfer data from host to device and then vice versa
#define NVME_IOCTL_VENDOR_SPECIFIC_DW_SIZE 6 // Vendor unique qualifier in ULONGs
#define NVME_IOCTL_CMD_DW_SIZE                          16 // NVMe command entry size in ULONGs
#define NVME_IOCTL_COMPLETE_DW_SIZE                4 // NVMe completion entry size in ULONGs
#pragma pack(1)
typedef struct _NVME_PASS_THROUGH_IOCTL 
{
    /* WDK defined SRB_IO_CONTROL structure */
    SRB_IO_CONTROL SrbIoCtrl;
    /* Vendor unique qualifiers for vendor unique commands */
    ULONG          VendorSpecific[NVME_IOCTL_VENDOR_SPECIFIC_DW_SIZE];
    /* 64-byte submission entry defined in NVMe Specification */
    ULONG          NVMeCmd[NVME_IOCTL_CMD_DW_SIZE];
    /* DW[0..3] of completion entry */
    ULONG          CplEntry[NVME_IOCTL_COMPLETE_DW_SIZE];
    /* Data transfer direction, from host to device or vice versa */
    ULONG          Direction; 
    /* 0 means using Admin queue, otherwise, IO queue is used */
    ULONG          QueueId;
    /* Transfer byte length, including Metadata, starting at DataBuffer */
    ULONG          DataBufferLen;
    /* Set to 0 if not supported or interleaved with data */
    ULONG          MetaDataLen; 
    /* Returned byte length from device to host, 
     * including at least the length of this structure, and data if any. */
    ULONG          ReturnBufferLen;
    /* Start with Metadata if present, and then regular data */
	UCHAR          DataBuffer[1];
	
} NVME_PASS_THROUGH_IOCTL, *PNVME_PASS_THROUGH_IOCTL;
enum _IOCTL_STATUS
{
     NVME_IOCTL_SUCCESS,
     NVME_IOCTL_INVALID_IOCTL_CODE,                     
     NVME_IOCTL_INVALID_SIGNATURE,                      
     NVME_IOCTL_INSUFFICIENT_IN_BUFFER,                 
     NVME_IOCTL_INSUFFICIENT_OUT_BUFFER,                 
     NVME_IOCTL_UNSUPPORTED_ADMIN_CMD,                  
     NVME_IOCTL_UNSUPPORTED_NVM_CMD,                    
     NVME_IOCTL_INVALID_ADMIN_VENDOR_SPECIFIC_OPCODE,   
     NVME_IOCTL_INVALID_NVM_VENDOR_SPECIFIC_OPCODE,    
     NVME_IOCTL_ADMIN_VENDOR_SPECIFIC_NOT_SUPPORTED, //AVSCC=0
     NVME_IOCTL_NVM_VENDOR_SPECIFIC_NOT_SUPPORTED, // NVSCC=0
     NVME_IOCTL_INVALID_DIRECTION_SPECIFIED,// when Direction is greater than 3
     NVME_IOCTL_INVALID_META_BUFFER_LENGTH,
     NVME_IOCTL_PRP_TRANSLATION_ERROR,
     NVME_IOCTL_INVALID_PATH_TARGET_ID,
     NVME_IOCTL_FORMAT_NVM_PENDING,      // Only one Format NVM at a time
     NVME_IOCTL_FORMAT_NVM_FAIED,
     NVME_IOCTL_INVALID_NAMESPACE_ID
};

#define NVME_PT_TIMEOUT 40

NVME_IO_STATUS Nvme_IoCtrl(HANDLE hDevice,UCHAR DataTX,ULONG ByteSize,PVOID PDataBuf,ULONG * PNvmeCmd,ULONG *PCplEntry);
#endif