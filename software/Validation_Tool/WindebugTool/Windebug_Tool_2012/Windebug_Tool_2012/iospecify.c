/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   io_specify.c                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: disk IO with DeviceIoControl with ATA_PASS_THROUGH
*              can specify the command code and command type except NCQ commands
*              all IOs response in syn mode
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100112 jackeychai first created
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <Windows.h>
#include "ntddscsi.h"
//#include <WinIoCtl.h>
#include <errno.h>
#include <assert.h>
#include <shlobj.h>
#include <io.h>
#include <stddef.h> // offsetof()

#include "defines.h"
#include "atacmd.h"
#include "iospecify.h"

#define MAX_LBA_WITH_DATA (1<<27)

extern U8 data_counter[MAX_LBA_WITH_DATA];
extern FILE *data_all_log;
extern U32 log_line,log_file_cnt;
extern char log_filename[256];

#if(_WIN32_WINNT < 0x0400)
//for smart io control cmd
#pragma pack(1)
typedef struct _SENDCMDINPARAMS {
	U32   cBufferSize;            // Buffer size in bytes
	IDEREGS irDriveRegs;            // Structure with drive register values.
	U8     bDriveNumber;           // Physical drive number to send
	// command to (0,1,2,3).
	U8     bReserved[3];           // Reserved for future expansion.
	U32   dwReserved[4];          // For future use.
	U8     bBuffer[1];                     // Input buffer.
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;
#pragma pack()

#pragma pack(1)
typedef struct _GETVERSIONINPARAMS {
	U8     bVersion;               // Binary driver version.
	U8     bRevision;              // Binary driver revision.
	U8     bReserved;              // Not used.
	U8     bIDEDeviceMap;          // Bit map of IDE devices.
	U32   fCapabilities;          // Bit mask of driver capabilities.
	U32   dwReserved[4];          // For future use.
} GETVERSIONINPARAMS, *PGETVERSIONINPARAMS, *LPGETVERSIONINPARAMS;
#pragma pack()

#pragma pack(1)
typedef struct _DRIVERSTATUS {
	U8     bDriverError;           // Error code from driver,
	// or 0 if no error.
	U8     bIDEError;                      // Contents of IDE Error register.
	// Only valid when bDriverError
	// is SMART_IDE_ERROR.
	U8     bReserved[2];           // Reserved for future expansion.
	U32   dwReserved[2];          // Reserved for future expansion.
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;
#pragma pack()


#pragma pack(1)
typedef struct _SENDCMDOUTPARAMS {
	U32                   cBufferSize;            // Size of bBuffer in bytes
	DRIVERSTATUS            DriverStatus;           // Driver status structure.
	U8                    bBuffer[1];             // Buffer of arbitrary length in which to store the data read from the                                                                                  // drive.
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;
#pragma pack()

//end for smart io control cmd
#endif
typedef struct 
{
	ATA_PASS_THROUGH_EX apt;
	U32 filler;
	U8 data_buf[128 * 1024];
} ATA_PASS_THROUGH_EX_WITH_BUFFERS;
typedef struct 
{
	ATA_PASS_THROUGH_DIRECT apt;
	U32 filler;
	U8 data_buf[128*1024];
} ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS;

extern U32 syscfg_max_lba;

IDEREGS g_outreg = {0};
IDEREGS g_outreg_ext = {0};

BOOL io_specify_read
(
 U8 ata_cmd_code,
 U32 start_lba,
 U16 xfer_cnt,
 U32 inbuf_addr
 )
{
	return TRUE;
}

BOOL io_specify_write
(
 U8 ata_cmd_code,
 U32 start_lba,
 U16 xfer_cnt,
 U32 outbuf_addr
 )
{
	return TRUE;
}

BOOL io_specify_identify_data(HANDLE disk, U32 inbuf_addr)
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;

	IDEREGS reg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;

	reg.bCommandReg = ATA_CMD_IDENTIFY_DEVICE;

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
	apt_ex_with_buf.apt.DataTransferLength = 512;
	apt_ex_with_buf.apt.TimeOutValue = 5;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);

	inbuf = (void*)&(apt_ex_with_buf);
	inbufsz = apt_ex_with_buf.apt.Length;
	outbuf = (void*)&(apt_ex_with_buf);
	outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(reg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( reg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}
	memcpy( (void*)inbuf_addr, apt_ex_with_buf.data_buf, 512); 

	return TRUE;
}

BOOL io_specify_pm_request(HANDLE disk, U32 command_code)
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;

	IDEREGS reg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;

	reg.bCommandReg = command_code;

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	apt_ex_with_buf.apt.AtaFlags = 0;
	apt_ex_with_buf.apt.DataTransferLength = 0;
	apt_ex_with_buf.apt.TimeOutValue = 5;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);

	inbuf = (void*)&(apt_ex_with_buf);
	inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
	outbuf = (void*)&(apt_ex_with_buf);
	outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(reg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( reg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}


	return TRUE;
}

/* Begin: add by henryluo 1488 for sata cmd */
BOOL sata_nondata_cmd_ex
(
 HANDLE disk,
 IDEREGS reg,
 IDEREGS exreg
 )
{
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	IDEREGS outreg;
	ATA_PASS_THROUGH_EX apt;

	memset(&apt, 0, sizeof(ATA_PASS_THROUGH_EX));

	assert(sizeof(apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt.PreviousTaskFile) == sizeof(IDEREGS));

	memcpy(apt.CurrentTaskFile, (void *)(&reg), 8);
	memcpy(apt.PreviousTaskFile, (void *)(&exreg), sizeof(IDEREGS));

	apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	apt.TimeOutValue = 10;


	inbuf = (void*)&(apt);
	inbufsz = apt.Length;
	outbuf = (void*)&(apt);
	outbufsz = apt.Length;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}


	memcpy((void *)&(outreg), apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}
	return TRUE;
}

BOOL sata_nondata_cmd
(
 HANDLE disk,
 IDEREGS reg
 )
{
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	IDEREGS outreg;
	ATA_PASS_THROUGH_EX apt;

	memset(&apt, 0, sizeof(ATA_PASS_THROUGH_EX));

	assert(sizeof(apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt.PreviousTaskFile) == sizeof(IDEREGS));

	memcpy(apt.CurrentTaskFile, (void *)(&reg), 8);


	apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	apt.TimeOutValue = 15;


	inbuf = (void*)&(apt);
	inbufsz = apt.Length;
	outbuf = (void*)&(apt);
	outbufsz = apt.Length;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(outreg), apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	return TRUE;
}

BOOL sata_pio_cmd_ex
(
 HANDLE disk,
 IDEREGS reg,
 IDEREGS exreg,
 U32 direction,
 U32 buf_addr,
 U32 buf_len
 )
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;

	IDEREGS outreg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_EX_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);   
	memcpy(apt_ex_with_buf.apt.PreviousTaskFile, (void *)(&exreg), sizeof(IDEREGS));

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.DataTransferLength = buf_len;
	apt_ex_with_buf.apt.TimeOutValue = 5;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		inbufsz = apt_ex_with_buf.apt.Length;
		outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
	}
	else  /*PIO_OUT*/
	{
		inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		memcpy(apt_ex_with_buf.data_buf, buf_addr, buf_len);   
		outbufsz = apt_ex_with_buf.apt.Length;
	}

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		memcpy(buf_addr, apt_ex_with_buf.data_buf, buf_len);
	}

	return TRUE;
}

BOOL sata_pio_cmd
(
 HANDLE disk,
 IDEREGS* preg,
 U32 direction,
 U32 buf_addr,
 U32 buf_len
 )
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;

	IDEREGS outreg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_EX_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(preg), 8);   
	

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.DataTransferLength = buf_len;
	apt_ex_with_buf.apt.TimeOutValue = 5;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		inbufsz = apt_ex_with_buf.apt.Length;
		outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
	}
	else  /*PIO_OUT*/
	{
		inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		memcpy(apt_ex_with_buf.data_buf, buf_addr, buf_len);   
		outbufsz = apt_ex_with_buf.apt.Length;
	}

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		memcpy(buf_addr, apt_ex_with_buf.data_buf, buf_len);
	}

	return TRUE;
}

BOOL sata_dma_cmd_ex
(
 HANDLE disk,
 IDEREGS reg,
 IDEREGS exreg,
 U32 direction,
 U32 buf_addr,
 U32 trans_sec_cnt
 )
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void * outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	U32 trans_len = trans_sec_cnt >> 9;

	IDEREGS outreg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_EX_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));
	memcpy(apt_ex_with_buf.apt.PreviousTaskFile, (void *)(&exreg), 8);
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);    

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);

	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.AtaFlags |= (ATA_FLAGS_USE_DMA | ATA_FLAGS_48BIT_COMMAND);
	//apt_ex_with_buf.apt.AtaFlags |= ATA_FLAGS_USE_DMA;

	apt_ex_with_buf.apt.DataTransferLength = trans_len;
	apt_ex_with_buf.apt.TimeOutValue = 10;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	if(DATA_TRANSFER_IN == direction) /*DMA_IN*/
	{
		inbufsz = apt_ex_with_buf.apt.Length;
		outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
	}
	else  /*DMA_OUT*/
	{
		inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		memcpy(apt_ex_with_buf.data_buf, buf_addr, trans_len);   
		outbufsz = apt_ex_with_buf.apt.Length;
	}

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		memcpy(buf_addr, apt_ex_with_buf.data_buf, trans_len);
	}

	return TRUE;
}
BOOL sata_dma_out_ex_direct
(
 HANDLE disk,
 IDEREGS reg,
 IDEREGS exreg,
 U32 direction,
 U32 buf_addr,
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

	IDEREGS outreg;

  
	ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS apt_ex_with_buf;
  
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_DIRECT_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));
	memcpy(apt_ex_with_buf.apt.PreviousTaskFile, (void *)(&exreg), 8);
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);    

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_DIRECT);

	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
       DBG_Getch();
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
    		&bytes_rtn, NULL)) 
    	{
    		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}
	else  /*DMA_OUT*/
	{
	//	inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		//inbufsz = sizeof(ATA_PASS_THROUGH_EX) + apt_ex_with_buf.apt.DataTransferLength;
       inbufsz = sizeof(ATA_PASS_THROUGH_DIRECT);
  
		memcpy(apt_ex_with_buf.data_buf, buf_addr, trans_len);   
		outbufsz = apt_ex_with_buf.apt.Length;

    	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH_DIRECT,
    		inbuf, inbufsz, outbuf, outbufsz, 
    		&bytes_rtn, NULL)) 
    	{
    		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		memcpy(buf_addr, apt_ex_with_buf.data_buf, trans_len);
	}

	return TRUE;
}


BOOL sata_dma_cmd
(
 HANDLE disk,
 void* preg,
 U32 direction,
 U32 buf_addr,
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

	IDEREGS outreg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;
	memset(&apt_ex_with_buf, 0x0, sizeof(ATA_PASS_THROUGH_EX_WITH_BUFFERS));

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));
	
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(preg), 8);    

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);

	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.AtaFlags |= ATA_FLAGS_USE_DMA;

	apt_ex_with_buf.apt.DataTransferLength = trans_len;
	apt_ex_with_buf.apt.TimeOutValue = 5;

	size = offsetof(ATA_PASS_THROUGH_EX_WITH_BUFFERS, data_buf);
	apt_ex_with_buf.apt.DataBufferOffset = size;

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	if(DATA_TRANSFER_IN == direction) /*DMA_IN*/
	{
		inbufsz = apt_ex_with_buf.apt.Length;
		outbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
	}
	else  /*DMA_OUT*/
	{
		inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		memcpy(apt_ex_with_buf.data_buf, buf_addr, trans_len);   
		outbufsz = apt_ex_with_buf.apt.Length;
	}

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}


	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		memcpy(buf_addr, apt_ex_with_buf.data_buf, trans_len);
	}

	return TRUE;
}

BOOL sata_dma_out_direct
(
  HANDLE disk,
  IDEREGSEX reg,
  U32 direction,
  U32 buf_addr,
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

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));

	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(&reg), 8);    

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_DIRECT);

	if(DATA_TRANSFER_IN == direction)
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_IN;
       DBG_Getch();
	}
	else
	{
		apt_ex_with_buf.apt.AtaFlags = ATA_FLAGS_DATA_OUT;
	}

	apt_ex_with_buf.apt.AtaFlags |= ATA_FLAGS_USE_DMA;

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
    		&bytes_rtn, NULL)) 
    	{
    		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}
	else  /*DMA_OUT*/
	{
	//	inbufsz = size + apt_ex_with_buf.apt.DataTransferLength;
		//inbufsz = sizeof(ATA_PASS_THROUGH_EX) + apt_ex_with_buf.apt.DataTransferLength;
       inbufsz = sizeof(ATA_PASS_THROUGH_DIRECT);
  
		memcpy(apt_ex_with_buf.data_buf, buf_addr, trans_len);   
		outbufsz = apt_ex_with_buf.apt.Length;

    	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH_DIRECT,
    		inbuf, inbufsz, outbuf, outbufsz, 
    		&bytes_rtn, NULL)) 
    	{
    		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
    		return FALSE;
    	}
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	if(DATA_TRANSFER_IN == direction) /*PIO_IN*/
	{
		memcpy(buf_addr, apt_ex_with_buf.data_buf, trans_len);
	}

	return TRUE;
}


/* End: add by henryluo 1488 for sata cmd */

BOOL sata_cmd_dma_read
(
 HANDLE disk,
 U32 lba,
 U32 lba_ext,
 U32 inbuf_addr,
 U32 trans_sec_cnt
 )
{
	U32 inbufsz;
	U32 trans_direction;
	IDEREGSEX reg;
	IDEREGSEX exreg;
	BOOL retval;

	reg.bSectorCountReg = trans_sec_cnt & 0xFF;
	reg.bLBALowReg = (lba & 0xFF);
	reg.bLBAMidReg = ((lba >> 8) & 0xFF);
	reg.bLBAHighReg = ((lba >> 16) & 0xFF);
	reg.bDriveHeadReg = 0xE0;
	reg.bCommandReg = ATA_CMD_READ_DMA;

	trans_direction = DATA_TRANSFER_IN;


	retval = sata_dma_cmd(disk, (void*)&reg, trans_direction, inbuf_addr, trans_sec_cnt);
	if( retval != TRUE ) 
	{   
		return FALSE;
	}

	return TRUE;
}

BOOL sata_cmd_dma_write
(
 HANDLE disk,
 U32 lba,
 U32 buf_addr,
 U32 trans_sec_cnt
 )
{
	U32 inbufsz;
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


	retval = sata_dma_cmd(disk, &reg, trans_direction, buf_addr, trans_sec_cnt);
	if( retval != TRUE ) 
	{   
		return FALSE;
	}

	return TRUE;
}

BOOL sata_cmd_dma_write_trim
(
 HANDLE disk,
 U32 buf_addr,
 U32 trans_sec_cnt
 )
{
	U32 inbufsz;
	U32 trans_direction;
	IDEREGSEX reg;
	BOOL retval;
	
	reg.bFeaturesReg = 1;
	reg.bSectorCountReg = trans_sec_cnt & 0xFF;
	reg.bLBALowReg = 0;
	reg.bLBAMidReg = 0;
	reg.bLBAHighReg = 0;
	reg.bDriveHeadReg = 0xE0;
	reg.bCommandReg = 0x06;

	trans_direction = DATA_TRANSFER_OUT;


	retval = sata_dma_out_direct(disk, reg, trans_direction, buf_addr, trans_sec_cnt);
	if( retval != TRUE ) 
	{   
		return FALSE;
	}

	return TRUE;
}

void dataverify_set_data_before_write_trim
(
 U32 start_lba,U32 End_lba
 )
{
	U32 i;

	for(i = start_lba;i <= End_lba;i++)
	{
		data_counter[i] = 0;
	}

}


BOOL io_specify_trim_request
(
	HANDLE disk
)
{
	struct Trim
	{
		U32 Lba_Low;
		U16 Lba_High;
		U16 Range_trim;
	}trim_cmd[128];
	//struct Trim trim_cmd;
	U32 *buf;
	U32 trim_cmd_start;
	U32 trim_cmd_end;
	int i,j;

	FILE *data_error_log;
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\trim_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}

	buf = (U32 *)malloc(sizeof(U32)*256);
	
	j = 1;
	trim_cmd_start = 0xf99938;
	for(i = 0;i < 64;i++)
	{
		//trim_cmd_list[i] = trim_cmd.Lba_trim;
		//i++;
		trim_cmd[i].Lba_Low = trim_cmd_start;
		trim_cmd[i].Lba_High = 0x0;
		trim_cmd[i].Range_trim = 0xFFF8;
		j++;
		if(j != 9)
		{
			trim_cmd_start = trim_cmd_start + 0xFFF8 + 0x08;
		}
		else
		{
			trim_cmd_start = trim_cmd_start + 0xFFF8;
			j = 1;
		}

	}
	trim_cmd[63].Range_trim = 0x1C30;
	trim_cmd[64].Lba_Low = 0x2006a0;
	trim_cmd[64].Lba_High = 0x0;
	trim_cmd[64].Range_trim = 0x20;
	trim_cmd[65].Lba_Low = 0x4a90a8;
	trim_cmd[65].Lba_High = 0x0;
	trim_cmd[65].Range_trim = 0x98;
	for(i = 66;i < 128;i++)
	{
		trim_cmd[i].Lba_Low = 0x0;
		trim_cmd[i].Lba_High = 0x0;
		trim_cmd[i].Range_trim = 0x0;
	}
	for(i = 0; i < 128; i++)
	{
		fprintf(data_error_log,"%x %x %x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		fprintf(data_all_log,"Trim cmd Lba_Low:%x, Lba_High:%x, Range_Trim:%x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		//fflush(data_all_log);
		check_log_file();
		trim_cmd_start = trim_cmd[i].Lba_Low;
		trim_cmd_end = trim_cmd[i].Lba_Low + trim_cmd[i].Range_trim;
		dataverify_set_data_before_write_trim(trim_cmd_start,trim_cmd_end);

	}

	fclose(data_error_log);

	memcpy(buf,trim_cmd,(sizeof(trim_cmd)));

	//sata_cmd_dma_write_trim(disk,buf,2);
	//sata_cmd_dma_write(disk, 2, buf, 2);
	//sata_cmd_dma_read(disk, 2, 0, buf, 2);
	sata_cmd_dma_write_trim(disk,buf,2);

	free(buf);
	//fclose(data_error_log);

	

}

BOOL io_specify_trim_request_random
(
	HANDLE disk
)
{
	struct Trim
	{
		U32 Lba_Low;
		U16 Lba_High;
		U16 Range_trim;
	}trim_cmd[128];
	//struct Trim trim_cmd;
	U32 *buf;
	U32 trim_cmd_start;
	U32 trim_cmd_end;
	U16 trim_cmd_range;
	U32 trim_max_lba;
	int i,j;

	FILE *data_error_log;
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\trim_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}

	buf = (U32 *)malloc(sizeof(U32)*256);

	if(MAX_LBA_WITH_DATA<syscfg_max_lba)
	{
		trim_max_lba = MAX_LBA_WITH_DATA;
	}
	else
	{
		trim_max_lba = syscfg_max_lba;
	}
	
	trim_cmd_start = (rand()*rand()) % trim_max_lba;
	trim_cmd_range = (rand()*rand()) % 0xFFFF;
	if(trim_cmd_start + trim_cmd_range >= trim_max_lba)
	{
		trim_cmd_start -= trim_cmd_range;
	}

	for(i = 0;i < 128;i++)
	{
		trim_cmd[i].Lba_Low = trim_cmd_start;
		trim_cmd[i].Lba_High = 0x0;
		trim_cmd[i].Range_trim = trim_cmd_range;
		
		trim_cmd_start = (rand()*rand()) % trim_max_lba;
		trim_cmd_range = (rand()*rand()) % 0xFFFF;
		if(trim_cmd_start + trim_cmd_range >= trim_max_lba)
		{
			trim_cmd_start -= trim_cmd_range;
		}
	}
	
	for(i = 0; i < 128; i++)
	{
		fprintf(data_error_log,"%x %x %x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		fprintf(data_all_log,"Trim cmd Lba_Low:%x, Lba_High:%x, Range_Trim:%x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		//fflush(data_all_log);
		check_log_file();
		trim_cmd_start = trim_cmd[i].Lba_Low;
		trim_cmd_end = trim_cmd[i].Lba_Low + trim_cmd[i].Range_trim;
		dataverify_set_data_before_write_trim(trim_cmd_start,trim_cmd_end);

	}

	fclose(data_error_log);

	memcpy(buf,trim_cmd,(sizeof(trim_cmd)));

	//sata_cmd_dma_write_trim(disk,buf,2);
	//sata_cmd_dma_write(disk, 2, buf, 2);
	//sata_cmd_dma_read(disk, 2, 0, buf, 2);
	sata_cmd_dma_write_trim(disk,buf,2);

	free(buf);
	//fclose(data_error_log);

	

}

LARGE_INTEGER  large_interger;  
/*long double CaculateTimeF(__int64 time_start,__int64 time_end)
{
    double dff;  
    __int64  c1, c2;  
    QueryPerformanceFrequency(&large_interger);  
    dff = large_interger.QuadPart;  
    QueryPerformanceCounter(&large_interger);  
    c1 = large_interger.QuadPart;  
    Sleep(800);  
    QueryPerformanceCounter(&large_interger);  
    c2 = large_interger.QuadPart;  
}*/

BOOL io_specify_trim_request_max
(
	HANDLE disk,U32 Start_Lba
)
{
	struct Trim
	{
		U32 Lba_Low;
		U16 Lba_High;
		U16 Range_trim;
	}trim_cmd[512];
	U32 *buf;
	U32 trim_cmd_start;
	U32 trim_cmd_end;
	U16 trim_cmd_range;
	U32 trim_max_lba;
	int i,j;
	SYSTEMTIME sys_start,sys_end;
	LONG64 durningtime_true;
	FILE *data_error_log;

	double dff;  
    __int64  c1, c2;  

	QueryPerformanceFrequency(&large_interger);  
    dff = large_interger.QuadPart;  

	if(_access(".\\output", 0)!=0) system("md .\\output");
	if((data_error_log = fopen(".\\output\\trim_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}
	buf = (U32 *)malloc(sizeof(U32)*256*4);
	if(MAX_LBA_WITH_DATA<syscfg_max_lba)
	{
		trim_max_lba = MAX_LBA_WITH_DATA;
	}
	else
	{
		trim_max_lba = syscfg_max_lba;
	}
	trim_cmd_start = Start_Lba;
	trim_cmd_range = 0xFFFF;
	if(trim_cmd_start + trim_cmd_range >= trim_max_lba)
	{
		trim_cmd_start -= trim_cmd_range;
	}
	for(i = 0;i < 512;i++)
	{
		trim_cmd[i].Lba_Low = trim_cmd_start;
		trim_cmd[i].Lba_High = 0x0;
		trim_cmd[i].Range_trim = trim_cmd_range;
		trim_cmd_start = trim_cmd_start + trim_cmd_range;
		if(trim_cmd_start + trim_cmd_range >= trim_max_lba)
		{
			trim_cmd_start = 0;
		}
	}
	for(i = 0; i < 512; i++)
	{
		fprintf(data_error_log,"%x %x %x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		fprintf(data_all_log,"Trim cmd Lba_Low:%x, Lba_High:%x, Range_Trim:%x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		check_log_file();
		trim_cmd_start = trim_cmd[i].Lba_Low;
		trim_cmd_end = trim_cmd[i].Lba_Low + trim_cmd[i].Range_trim;
	}
	fclose(data_error_log);
	memcpy(buf,trim_cmd,(sizeof(trim_cmd)));
	//GetLocalTime( &sys_start );
	QueryPerformanceCounter(&large_interger);  
    c1 = large_interger.QuadPart;  
	sata_cmd_dma_write_trim(disk,buf,8);
	QueryPerformanceCounter(&large_interger);  
    c2 = large_interger.QuadPart;  
	//GetLocalTime( &sys_end );
	//durningtime_true = computetime(sys_start,sys_end);
	//durningtime_true = CaculateTime(sys_end,sys_start);
	//printf("Send 16G trim range cmd time is %x um. \n",durningtime_true);
	printf("Send 16G trim range cmd time is %lf mm. \n",(c2 - c1) * 1000 / dff);
	free(buf);
}
BOOL io_specify_trim_request_sector_time
(
	HANDLE disk,U32 Start_Lba,U32 Sector_Cnt
)
{
	struct Trim
	{
		U32 Lba_Low;
		U16 Lba_High;
		U16 Range_trim;
	}trim_cmd[512];
	SYSTEMTIME sys_start,sys_end;
	double durningtime_true;
	U32 Trim_Size = Sector_Cnt * 64;
	U32 *buf;
	U32 trim_cmd_start;
	U32 trim_cmd_end;
	U16 trim_cmd_range;
	U32 trim_max_lba;
	int i,j;
	FILE *data_error_log;
	if(_access(".\\output", 0)!=0) system("md .\\output");
	if((data_error_log = fopen(".\\output\\trim_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}
	buf = (U32 *)malloc(sizeof(U32)*Trim_Size*2);
	if(MAX_LBA_WITH_DATA<syscfg_max_lba)
	{
		trim_max_lba = MAX_LBA_WITH_DATA;
	}
	else
	{
		trim_max_lba = syscfg_max_lba;
	}
	trim_cmd_start = Start_Lba;
	trim_cmd_range = 0xFFFF;
	if(trim_cmd_start + trim_cmd_range >= trim_max_lba)
	{
		trim_cmd_start -= trim_cmd_range;
	}
	for(i = 0;i < Trim_Size;i++)
	{
		trim_cmd[i].Lba_Low = trim_cmd_start;
		trim_cmd[i].Lba_High = 0x0;
		trim_cmd[i].Range_trim = trim_cmd_range;
		trim_cmd_start = trim_cmd_start + trim_cmd_range;
		if(trim_cmd_start + trim_cmd_range >= trim_max_lba)
		{
			trim_cmd_start = 0;
		}
	}
	for(i = 0; i < Trim_Size; i++)
	{
		fprintf(data_error_log,"%x %x %x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		fprintf(data_all_log,"Trim cmd Lba_Low:%x, Lba_High:%x, Range_Trim:%x\n",trim_cmd[i].Lba_Low,trim_cmd[i].Lba_High,trim_cmd[i].Range_trim);
		check_log_file();
		trim_cmd_start = trim_cmd[i].Lba_Low;
		trim_cmd_end = trim_cmd[i].Lba_Low + trim_cmd[i].Range_trim;
	}
	fclose(data_error_log);
	memcpy(buf,trim_cmd,(sizeof(trim_cmd)));
	GetLocalTime( &sys_start );
	sata_cmd_dma_write_trim(disk,buf,Sector_Cnt);
	GetLocalTime( &sys_end );
	durningtime_true = computetime(sys_start,sys_end);
	printf("Send %d sector trim range cmd time is %f. \n",Sector_Cnt,durningtime_true);
	free(buf);
}



BOOL sata_cmd_sector_read
(
 HANDLE disk,
 U32 lba,
 U32 inbuf_addr,
 U32 trans_sec_cnt
 )
{
	U32 inbufsz;
	U32 trans_direction;
	IDEREGSEX reg;
	BOOL retval;
	U32 trans_len;

	trans_len = trans_sec_cnt << 9;
	reg.bSectorCountReg = trans_sec_cnt & 0xFF;
	reg.bLBALowReg = (lba & 0xFF);
	reg.bLBAMidReg = ((lba >> 8) & 0xFF);
	reg.bLBAHighReg = ((lba >> 16) & 0xFF);
	reg.bDriveHeadReg = 0xE0;
	reg.bCommandReg = ATA_CMD_READ_SECTOR;

	trans_direction = DATA_TRANSFER_IN;


	retval = sata_pio_cmd(disk, &reg, trans_direction, inbuf_addr, trans_len);
	if( retval != TRUE ) 
	{   
		return FALSE;
	}

	return TRUE;
}

BOOL sata_cmd_sector_write
(
 HANDLE disk,
 U32 lba,
 U32 buf_addr,
 U32 trans_sec_cnt
 )
{
	U32 inbufsz;
	U32 trans_direction;
	IDEREGSEX reg;
	BOOL retval;
	U32 trans_len;

	trans_len = trans_sec_cnt << 9;
	reg.bSectorCountReg = trans_sec_cnt & 0xFF;
	reg.bLBALowReg = (lba & 0xFF);
	reg.bLBAMidReg = ((lba >> 8) & 0xFF);
	reg.bLBAHighReg = ((lba >> 16) & 0xFF);
	reg.bDriveHeadReg = 0xE0;
	reg.bCommandReg = ATA_CMD_WRITE_SECTOR;

	trans_direction = DATA_TRANSFER_OUT;


	retval = sata_pio_cmd(disk, &reg, trans_direction, buf_addr, trans_len);
	if( retval != TRUE ) 
	{   
		return FALSE;
	}

	return TRUE;
}



U32 sata_register_write_cmd
(
 HANDLE disk,
 IDEREGS* preg,
 IDEREGS* preg_Exp
 )
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	void *outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	IDEREGS outreg;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));
	memset(&apt_ex_with_buf,0,sizeof(ATA_PASS_THROUGH_EX_WITH_BUFFERS));
	memcpy(apt_ex_with_buf.apt.PreviousTaskFile, (void *)(preg_Exp), 8);
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(preg), 8);  

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);

	apt_ex_with_buf.apt.AtaFlags |= ATA_FLAGS_48BIT_COMMAND;

	apt_ex_with_buf.apt.TimeOutValue = 5;

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	//none_data
	inbufsz = apt_ex_with_buf.apt.Length;
	outbufsz = apt_ex_with_buf.apt.Length;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)&(outreg), apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( outreg.bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}
	return TRUE;
}

BOOL sata_register_read_cmd(HANDLE disk, IDEREGS *reg)
{
	void * inbuf;
	U32 inbufsz;
	void *outbuf;
	U32 outbufsz;
	U32 bytes_rtn;
	ATA_PASS_THROUGH_EX_WITH_BUFFERS apt_ex_with_buf;

	assert(sizeof(apt_ex_with_buf.apt.CurrentTaskFile) == sizeof(IDEREGS));
	assert(sizeof(apt_ex_with_buf.apt.PreviousTaskFile) == sizeof(IDEREGS));

	memset(&apt_ex_with_buf,0,sizeof(ATA_PASS_THROUGH_EX_WITH_BUFFERS));
	memcpy(apt_ex_with_buf.apt.CurrentTaskFile, (void *)(reg), 8);  

	apt_ex_with_buf.apt.Length = sizeof(ATA_PASS_THROUGH_EX);


	apt_ex_with_buf.apt.TimeOutValue = 5;

	inbuf = (void*)&(apt_ex_with_buf);
	outbuf = (void*)&(apt_ex_with_buf);	
	//none_data
	inbufsz = apt_ex_with_buf.apt.Length;
	outbufsz = apt_ex_with_buf.apt.Length;

	if (!DeviceIoControl(disk, IOCTL_ATA_PASS_THROUGH,
		inbuf, inbufsz, outbuf, outbufsz, 
		&bytes_rtn, NULL)) 
	{
		DBG_printf("DeviceIoControl error.%d\n", GetLastError());
		return FALSE;
	}

	memcpy((void *)reg, apt_ex_with_buf.apt.CurrentTaskFile, 8);
	if( reg->bFeaturesReg != 0 ) 
	{   
		DBG_printf("read return status error, check parameters.\n");
		return FALSE;
	}

	return TRUE;
}

BOOL io_specify_smart_identify_data(HANDLE disk, U32 inbuf_addr)
{
	unsigned size;
	void * inbuf;
	U32 inbufsz;
	U32 outbufsz;
	U32 bytes_rtn;
	U8 outbuf[sizeof(SENDCMDOUTPARAMS)-1 + 512]; 
	SENDCMDINPARAMS scip;
	IDEREGS reg;
	GETVERSIONINPARAMS gVersionParsams;
	U32 n;

	reg.bCommandReg = ATA_CMD_IDENTIFY_DEVICE;
	reg.bSectorCountReg = 1;
	reg.bDriveHeadReg = 0xA0;

	/* call SMART_GET_VERSION, return device map or -1 on error*/
	
	memset(&gVersionParsams,0,sizeof(GETVERSIONINPARAMS)); 
	

	if(!DeviceIoControl(disk,SMART_GET_VERSION,NULL,NULL,&gVersionParsams,sizeof(GETVERSIONINPARAMS),&bytes_rtn, NULL) 
		|| bytes_rtn==0 || gVersionParsams.bIDEDeviceMap <= 0) 
	{ 
		//CloseHandle(disk); 
		return FALSE; 
	} 

	/*call SMART_RCV_DRIVE_DATA, get identify info*/
	memset(&scip,0,sizeof(SENDCMDINPARAMS)); 
	scip.cBufferSize=512; 
	scip.irDriveRegs = reg;
	memset(outbuf,0,sizeof(outbuf)); 
	bytes_rtn = 0;
	
	if(!DeviceIoControl(disk,SMART_RCV_DRIVE_DATA,&scip,sizeof(SENDCMDINPARAMS), 
		outbuf,sizeof(outbuf),&bytes_rtn,NULL)) 
	{ 
		//CloseHandle(disk); 
		return FALSE; 
	} 
	
	memcpy((void *)inbuf_addr, outbuf + sizeof(SENDCMDOUTPARAMS)-1, 512);

	return TRUE;
}

BOOL sata_vendor_dma_cmd(HANDLE disk, U32 cmd_code, U32 nDirec, U32 buf_addr, U32 trans_sec_cnt)
{
    U32 inbufsz;
    U32 trans_direction;
    IDEREGS reg;
    BOOL retval;

    reg.bSectorCountReg = trans_sec_cnt;
    reg.bFeaturesReg = cmd_code;
    reg.bDriveHeadReg = 0xE0;
    reg.bCommandReg = ATA_CMD_VENDER_DEFINE;	

    trans_direction = DATA_TRANSFER_IN;

    retval = sata_dma_cmd(disk, &reg, trans_direction, nDirec, buf_addr, trans_sec_cnt);
    if( retval != TRUE ) 
    {   
        return FALSE;
    }

    return TRUE;
}

void sata_vendor_nonedata_cmd( HANDLE disk, U32 cmd_code)
{
    IDEREGS reg = {0};
    IDEREGS exreg = {0};

    reg.bFeaturesReg = cmd_code;
    reg.bDriveHeadReg = 0xE0;
    reg.bCommandReg = ATA_CMD_VENDER_DEFINE;

    
    sata_nondata_cmd_ex(disk, reg, exreg);
}



BOOL sata_vendor_read_register(HANDLE disk, U32 addr, U32 *retvaladdr)
{
    IDEREGSEX reg;
    U32 nvalue;
    BOOL cmdrslt;

    //DBG_printf("please read register\r\n");
    reg.bFeaturesReg = 0xc0;
    reg.bSectorCountReg = addr&0xff;
    reg.bLBALowReg=(addr>>8)&0xff;
    reg.bLBAMidReg=(addr>>16)&0xff;
    reg.bLBAHighReg=(addr>>24)&0xff;
    reg.bDriveHeadReg = 0xE0;
    reg.bCommandReg = ATA_CMD_VENDER_DEFINE;	

    cmdrslt = sata_register_read_cmd(disk, &reg);

    if( cmdrslt != TRUE ) 
    {   
        return FALSE;
    }

    nvalue=reg.bSectorCountReg;
    nvalue|=reg.bLBALowReg<<8;
    nvalue|=reg.bLBAMidReg<<16;
    nvalue|=reg.bLBAHighReg<<24;
    *retvaladdr = nvalue;

    DBG_printf("read data from register:addr is 0x%x, value is 0x%x\r\n",addr,nvalue);

    return TRUE;

}

BOOL sata_vendor_write_register(HANDLE disk, U32 addr,U32 nvalue)
{
    IDEREGSEX reg;
    IDEREGSEX reg_Exp;
    BOOL retval;

    reg.bFeaturesReg = 0xc1;
    reg.bSectorCountReg = addr&0xff;
    reg.bLBALowReg=(addr>>8)&0xff;
    reg.bLBAMidReg=(addr>>16)&0xff;
    reg.bLBAHighReg=(addr>>24)&0xff;

    reg_Exp.bSectorCountReg=nvalue&0xff;
    reg_Exp.bLBALowReg=(nvalue>>8)&0xff;
    reg_Exp.bLBAMidReg=(nvalue>>16)&0xff;
    reg_Exp.bLBAHighReg=(nvalue>>24)&0xff;

    reg.bDriveHeadReg = 0xe0;
    reg.bCommandReg = ATA_CMD_VENDER_DEFINE;	

    retval = sata_register_write_cmd(disk, (void*)&reg, (void*)&reg_Exp);

    if( retval != TRUE ) 
    {   
        return FALSE;
    }
    DBG_printf("\n");
    return TRUE;
}

void sata_vendor_pio_cmd(HANDLE disk, U32 cmd_code, U32 nDirec, U32 buf_addr, U32 buf_len)
{
	IDEREGS reg = {0};
	IDEREGS exreg = {0};

    reg.bFeaturesReg = cmd_code;
    reg.bDriveHeadReg = 0xE0;
    reg.bCommandReg = ATA_CMD_VENDER_DEFINE;

	sata_pio_cmd_ex(disk, reg, exreg, nDirec, buf_addr, buf_len);
}