/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   device.c                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: ATA device related
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100113 JackeyChai first created
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <Windows.h>
#include <errno.h>
#include <cfgmgr32.h>
#include "defines.h"
#include "config.h"
#include "defines.h"
#include "iospecify.h"
#include "device.h"
#include "atacmd.h"

#define MAX_DISK_NAME_LEN 80
#define MAX_DISK 10

#define ASYNC_IO_SUPPORT 

#define LBA_TO_U64()
enum
{
	DISK_OP_SUCCESS,
	DISK_OP_PENDING,
	TEST_DISK_NOT_EXIST,
	DISK_OP_PARAMETER_ERROR,
	DISK_OP_FAIL
};

typedef struct _PHYSICAL_DISK_
{
    /*disk identify*/
    U8 disk_name[MAX_DISK_NAME_LEN];
    
	/*sync op*/
	HANDLE disk_file_sync;

	/*async op*/
	HANDLE disk_file_async;	
//	OVERLAPPED fp_overlapped;

    /*disk parameters*/
    IDINFO disk_info;
    U32 disk_capacity_s;
} PHYSICAL_DISK;

PHYSICAL_DISK disk_array[MAX_DISK];

PHYSICAL_DISK *g_p_test_disk;

HANDLE g_test_disk_sync;
HANDLE g_test_disk_async;

#define	_DISK 1

extern FILE *data_all_log;
extern U32 log_line,log_file_cnt;
extern char log_filename[256];


U8 disk_detect()
{
    U32 count;
    U32 drive_number;
    U8 disk_name[MAX_DISK_NAME_LEN]="";
	U8 model_number[] = "IV";
    HANDLE disk;

	g_p_test_disk = NULL;

	g_test_disk_sync = INVALID_HANDLE_VALUE;
	g_test_disk_async = INVALID_HANDLE_VALUE;
    count = 0;
    drive_number = 0;
#ifdef _DISK
	/*Reporting physical drives.*/
	while( count < 128 ) 
    {
		/*See if the physical drive exists.*/
		sprintf(disk_name, "\\\\.\\PhysicalDrive%d", drive_number);


		disk = CreateFileA(disk_name, GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,0, 0);

		if(disk == INVALID_HANDLE_VALUE) 
        {
            /*reach the end*/
			CloseHandle(disk);
            break;
        }
        
		/*Drive exists, get identify data*/
		if( FALSE == io_specify_identify_data(disk,(U32)(&disk_array[count].disk_info)) )
        {
            /*can not get identify data, ignore it*/
            drive_number++;
			CloseHandle(disk);
            continue;
        }
		else 
		{
			U8 *pFirst = memchr(disk_array[count].disk_info.sModelNumber, model_number[0], 1);
			if (pFirst != NULL)
			{
				if (!memcmp(pFirst, model_number, sizeof(model_number) - 1))
				{
					strcpy(disk_array[count].disk_name, disk_name);
					count++;
				}
			}
		}

		drive_number++;        	
		CloseHandle(disk);
        if( count == MAX_DISK )
        {
            break;
        }

	}
	strcpy(disk_array[count].disk_name, disk_name);
	
    DBG_printf("%d device found in system.\n", count);
#else
	disk = CreateFileA(disk_name, GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,0, 0);

	if(disk == INVALID_HANDLE_VALUE) 
        {
            /*reach the end*/
			CloseHandle(disk);
			DBG_printf("open file fail\n\r",);
        }

	strcpy(disk_array[count].disk_name, disk_name);
	CloseHandle(disk);
#endif
    
    return count;
}

U8 disk_detect_sim()
{
	U32 count;
	U32 drive_number;
	U8 disk_name[MAX_DISK_NAME_LEN];
	HANDLE disk;
	LARGE_INTEGER file_size;

	g_p_test_disk = NULL;

	g_test_disk_sync = INVALID_HANDLE_VALUE;
	g_test_disk_async = INVALID_HANDLE_VALUE;
	count = 0;
	drive_number = 0;

	sprintf(disk_name, "TestFile.bin");


	disk = CreateFileA(disk_name, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,0, 0);

	if(disk == INVALID_HANDLE_VALUE) 
	{
		/*reach the end*/
		CloseHandle(disk);
	}

	file_size.QuadPart = syscfg_max_lba << SEC_BIT;

	//SetFileValidData(disk,file_size.QuadPart);

	SetFilePointerEx(disk,file_size,NULL,FILE_BEGIN);
	SetEndOfFile(disk);
	strcpy(disk_array[count].disk_name, disk_name);
	drive_number++;
	count++;
	CloseHandle(disk);

	DBG_printf("%d device found in system.\n", count);
	return count;
}

void disk_array_show
(
    U8 disk_cnt
)
{
    U32 i;

    DBG_printf("  ID SERIAL_NUM           CAPACITY     \n");
    for( i = 0; i < disk_cnt; i++ )
    {
		DBG_printf("  %2d", i);
		DBG_printf(" %20s", disk_array[i].disk_info.sSerialNumber);
		if( disk_array[i].disk_info.wCmdFeatureEnable.Addr48bit )
		{
			DBG_printf(" %d(= %dGB %dMB)", 
				disk_array[i].disk_info.dwTotalSectors48bit,
				disk_array[i].disk_info.dwTotalSectors48bit >> 21,
				disk_array[i].disk_info.dwTotalSectors48bit >> 11 & 0x3FF);
			disk_array[i].disk_capacity_s =   disk_array[i].disk_info.dwTotalSectors48bit -1;
		}
		else
		{
			DBG_printf(" %d(= %dGB %dMB)", 
				disk_array[i].disk_info.dwTotalSectors,
				disk_array[i].disk_info.dwTotalSectors >> 21,
				disk_array[i].disk_info.dwTotalSectors >> 11 & 0x3FF);
			disk_array[i].disk_capacity_s =   disk_array[i].disk_info.dwTotalSectors -1;
		}
		DBG_printf("\n");
	}
}

void disk_capability_check
(
    U8 disk_id
)
{
    if( disk_id >= MAX_DISK )
        return;

    DBG_printf("Disk information:\n");
    DBG_printf("");
}

void disk_init_testdisk(U8 disk_index)
{
	LARGE_INTEGER file_size;

	DBG_printf("Test Disk select on %20s\n", disk_array[disk_index].disk_info.sSerialNumber );
/*
	disk_array[disk_index].disk_file_sync = 
		CreateFileA(disk_array[disk_index].disk_name, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
*/

#ifdef ASYNC_IO_SUPPORT
	disk_array[disk_index].disk_file_async = 
		CreateFileA(disk_array[disk_index].disk_name, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING, 0);
#else
	disk_array[disk_index].disk_file_async = INVALID_HANDLE_VALUE;
#endif


	if(disk_array[disk_index].disk_file_sync == INVALID_HANDLE_VALUE || 
		disk_array[disk_index].disk_file_async == INVALID_HANDLE_VALUE
		) 
	{
		/*reach the end*/
		DBG_printf("Test Disk open fail\n");
		CloseHandle(disk_array[disk_index].disk_file_sync);
		CloseHandle(disk_array[disk_index].disk_file_async);
		return;
	}
	
	disk_array[disk_index].disk_capacity_s =   disk_array[disk_index].disk_info.dwTotalSectors -1;
	syscfg_max_lba = disk_array[disk_index].disk_capacity_s;
	syscfg_max_user_lba = syscfg_max_lba;
	syscfg_max_accessable_lba = syscfg_max_lba;

	if(MAX_LBA_WITH_DATA < syscfg_max_lba)
	{
			DBG_printf("syscfg_max_lba > MAX_LBA_WITH_DATA data check will have error\n");
			syscfg_max_lba = MAX_LBA_WITH_DATA;
	}

	file_size.QuadPart = syscfg_max_lba << SEC_BIT;

	//SetFileValidData(disk,file_size.QuadPart);

	SetFilePointerEx(disk_array[disk_index].disk_file_async,file_size,NULL,FILE_BEGIN);
	SetEndOfFile(disk_array[disk_index].disk_file_async);


	g_p_test_disk = &disk_array[disk_index];

	g_test_disk_sync = disk_array[disk_index].disk_file_sync ;
	g_test_disk_async = disk_array[disk_index].disk_file_async ;

	GetFileSizeEx(disk_array[disk_index].disk_file_async,&file_size);
	printf("%x\n\r",file_size.QuadPart);
}

void disk_get_sync_handle(HANDLE *g_disk_handle)
{
	*g_disk_handle = g_test_disk_sync;
}

void disk_get_async_handle(HANDLE *g_disk_handle)
{
	*g_disk_handle = g_test_disk_async;
}

void disk_scan_changes()
{ 
	DEVINST     devInst; 
	CONFIGRET   status; 

	//  
	// Get the root devnode. 
	//  

	status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL); 

	if (status != CR_SUCCESS) 
	{ 
		printf("CM_Locate_DevNode failed: %x\n", status); 
		return FALSE; 

	} 

	status = CM_Reenumerate_DevNode(devInst, CM_REENUMERATE_NORMAL); 

	if (status != CR_SUCCESS) 
	{ 
		printf("CM_Reenumerate_DevNode failed: %x\n", status); 
		return FALSE; 
	} 

	return TRUE; 
} 

void disk_special_cmd(U32 cmdcode)
{
	io_specify_pm_request(g_test_disk_async,cmdcode);
	fprintf(data_all_log,"io_specify_pm_request 0x%x\n",cmdcode);
	//fflush(data_all_log);
	check_log_file();
}

BOOL ProgramRegByTable(PREGENTRY RegTable)
{
    PREGENTRY CurrRegEntry = RegTable;
    U32 dwRegValue;

    while ( REG_OPEND != CurrRegEntry->dwOpType ) {
        switch ( CurrRegEntry->dwOpType ) {
            case REG_WRITE:
                if ( FALSE == sata_vendor_write_register(g_test_disk_async, CurrRegEntry->dwRegIndex, CurrRegEntry->dwOrMask) )
                    return FALSE;

                break;

            case REG_MODIFY:
                if ( FALSE == sata_vendor_read_register(g_test_disk_async, CurrRegEntry->dwRegIndex, &dwRegValue) )
                    return FALSE;

                dwRegValue = ( ( dwRegValue & CurrRegEntry->dwAndMask ) | CurrRegEntry->dwOrMask );
                
                if ( FALSE == sata_vendor_write_register(g_test_disk_async, CurrRegEntry->dwRegIndex, dwRegValue) )
                    return FALSE;

                break;

            case REG_READCHK:
                do {
                    if ( FALSE == sata_vendor_read_register(g_test_disk_async, CurrRegEntry->dwRegIndex, &dwRegValue) )
                        return FALSE;
                    
                } while( CurrRegEntry->dwOrMask != ( dwRegValue & CurrRegEntry->dwAndMask ) );

                break;

            default:
                return FALSE;
        }

        CurrRegEntry++;
    }

    return TRUE;
}

BOOL InitializeTargetSDRAM(U8 ucSDRAMType)
{
    BOOL bResult;
    static REGENTRY DDR2InitTbl[] = {
        {0x12718, 0, 0x003f0071, REG_WRITE},
        {0x11400, 0, 0x08000008, REG_WRITE},
        {0x11404, 0, 0x00140201, REG_WRITE},
        {0x11408, 0, 0x00000111, REG_WRITE},
        {0x11420, 0, 0x02999999, REG_WRITE},
        {0x11428, 0, 0x0000000a, REG_WRITE},
        {0x1142C, 0, 0xf210083f, REG_WRITE},
        {0x11430, 0, 0x0000000c, REG_WRITE},
        {0x11434, 0, 0x00958205, REG_WRITE},
        {0x11438, 0, 0xa0010080, REG_WRITE},
        {0x11440, 0, 0x45d44414, REG_WRITE},
        {0x11444, 0, 0x300077cd, REG_WRITE},
        {0x11448, 0, 0x00001000, REG_WRITE},
        {0x11450, 0, 0x08811000, REG_WRITE},
        {0x11458, 0, 0x00000018, REG_WRITE},
        {0x11470, 0, 0x51a00000, REG_WRITE},
        {0x11474, 0, 0x82000000, REG_WRITE},
        {0x11490, 0, 0x30004230, REG_WRITE},
        {0x11494, 0, 0x00000010, REG_WRITE},
        {0x11498, 0, 0x70010000, REG_WRITE},
        {0x114A0, 0, 0x00000404, REG_WRITE},
        {0x114A4, 0, 0x20202828, REG_WRITE},
        {0x1142C, 0, 0xf010083f, REG_WRITE},
        {0x11490, 0, 0x31004230, REG_WRITE},
        {0x11490, ( 1 << 24 ), 0, REG_READCHK},
        {0x1142C, 0, 0xf210083f, REG_WRITE},
        {0x1271C, 0, 0x17200d16, REG_WRITE},
        {0x114A0, 0, 0x28280404, REG_WRITE},
        {0x114A4, 0, 0x24242c2c, REG_WRITE},
        {0x114B0, 0, 0x00000088, REG_WRITE},
        {0x114B4, 0, 0x00000088, REG_WRITE},
        {0, 0, 0, REG_OPEND}
    };

    static REGENTRY DDR3InitTbl[] = {
        {0x12718, 0, 0x001e00a1, REG_WRITE},
        {0x1271C, 0, 0x10100808, REG_WRITE},
        {0x11400, 0, 0x08000040, REG_WRITE},
        {0x11404, 0, 0x00140315, REG_WRITE},
        {0x11408, 0xffff0000, 0x00008123, REG_MODIFY},
        {0x11420, 0, 0x02999999, REG_WRITE},
        {0x11428, 0, 0x0000000a, REG_WRITE},
        {0x1142C, 0, 0xf010083f, REG_WRITE},
        {0x11430, 0xffffff00, 0x00000007, REG_MODIFY},
        {0x11434, 0, 0x00958205, REG_WRITE},
        {0x11438, 0, 0xa0010080, REG_WRITE},
        {0x11440, 0, 0x45a64423, REG_WRITE},
        {0x11444, 0, 0x6040b7d5, REG_WRITE},
        {0x11448, 0xffff0000, 0x0000b000, REG_MODIFY},
        {0x11450, 0x00ffffff, 0x1d000000, REG_MODIFY},
        {0x11458, 0xffffff00, 0x00000018, REG_MODIFY},
        {0x11470, 0xff00ffff, 0x00a10000, REG_MODIFY},
        {0x11474, 0, 0x80000000, REG_WRITE},
        {0x11490, 0, 0x1400aa30, REG_WRITE},
        {0x11494, 0xffffff00, 0x00000010, REG_MODIFY},
        {0x11498, 0xff00ffff, 0x00010000, REG_MODIFY},
        {0x114A0, 0xffff0000, 0x00000404, REG_MODIFY},
        {0x114A4, 0, 0x3c3c2828, REG_WRITE},
        {0x11478, 0xffff0000, 0x00008200, REG_MODIFY},
        {0x1142C, 0, 0xf010083f, REG_WRITE},
        {0x11490, 0, 0x1500aa30, REG_WRITE},
        {0x11490, ( 1 << 24 ), 0, REG_READCHK},
        {0x1142C, 0, 0xf210083f, REG_WRITE},
        {0x11478, 0, 0x000083f8, REG_WRITE},
        {0x114B0, 0, 0x000000BB, REG_WRITE},
        {0x114B4, 0, 0x000000BB, REG_WRITE},
        {0x11480, 0, 0x0000ff22, REG_WRITE},
        {0x11484, 0, 0x00000044, REG_WRITE},
        {0x114A0, 0, 0x15150404, REG_WRITE},
        {0x114A8, 0, 0x00000000, REG_WRITE},
        {0x114AC, 0, 0x00000088, REG_WRITE},
        {0x10034, 0, 0x3fffffff, REG_WRITE},
        {0, 0, 0, REG_OPEND}
        };

    if ( SDRAM_TYPE_DDR2 == ucSDRAMType )
        bResult = ProgramRegByTable(DDR2InitTbl);
    else if ( SDRAM_TYPE_DDR3 == ucSDRAMType )
        bResult = ProgramRegByTable(DDR3InitTbl);

    return bResult;
}

BOOL SetTargetMemStorageArea(U8 ucMemAreaID)
{
    IDEREGS reg = {0};
    IDEREGS exreg = {0};

    reg.bFeaturesReg = SUBCMD_VIA_SWITCH_BUFFER;
    reg.bDriveHeadReg = 0xE0;
    reg.bCommandReg = ATA_CMD_VENDER_DEFINE;
    reg.bSectorCountReg = ucMemAreaID;
    
    return sata_nondata_cmd_ex(g_test_disk_async, reg, exreg);
 }

#if 0
U32 disk_write_sync(U32 start_lba,U16 len,U8*data_buffer)
{
	HANDLE disk;
	U32 has_read;
	if(NULL == g_p_test_disk)
		return  TEST_DISK_NOT_EXIST;

	disk = g_p_test_disk->disk_file_sync;

	SetFilePointer(disk,start_lba<<SEC_BIT,0,FILE_BEGIN);

	if(FAIL == WriteFile(disk,data_buffer,len,&has_read,NULL))
	{
		DBG_printf("WriteFile error %d\n",GetLastError());
		return DISK_OP_FAIL;
	}

	return DISK_OP_SUCCESS;

}

U32 disk_read_sync(U32 start_lba,U16 len,U8*data_buffer)
{
	HANDLE disk;
	U32 has_read;
	if(NULL == g_p_test_disk)
		return  TEST_DISK_NOT_EXIST;

	disk = g_p_test_disk->disk_file_sync;
	
	SetFilePointer(disk,start_lba<<SEC_BIT,0,FILE_BEGIN);
	
	if(FAIL == ReadFile(disk,data_buffer,len,&has_read,NULL))
	{
		DBG_printf("ReadFile error %d\n",GetLastError());
		return DISK_OP_FAIL;
	}

	return DISK_OP_SUCCESS;

}


U32 disk_write_async(U32 start_lba,U16 len,U32*data_buffer)
{
	HANDLE disk;
	U32 sec_cnt;
	
	U32 has_read;
	if(NULL == g_p_test_disk)
		return  TEST_DISK_NOT_EXIST;

	disk = g_p_test_disk->disk_file_async;
	g_p_test_disk->fp_overlapped.hEvent = (HANDLE)data_buffer;
	g_p_test_disk->fp_overlapped.Offset = start_lba<<SEC_BIT;
	g_p_test_disk->fp_overlapped.OffsetHigh = 0;

	if(!WriteFile(disk,data_buffer,len,NULL,&g_p_test_disk->fp_overlapped))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			DBG_printf("WriteFile error %d\n",GetLastError());
			return DISK_OP_FAIL;
		}
		else
			return DISK_OP_PENDING;
	}

	return DISK_OP_SUCCESS;

}

U32 disk_read_async(U32 start_lba,U16 len,U32*data_buffer)
{
	HANDLE disk;
	U32 has_read;
	if(NULL == g_p_test_disk)
		return  TEST_DISK_NOT_EXIST;

	disk = g_p_test_disk->disk_file_async;
	g_p_test_disk->fp_overlapped.hEvent = (HANDLE)data_buffer;
	g_p_test_disk->fp_overlapped.Offset = start_lba<<SEC_BIT;
	g_p_test_disk->fp_overlapped.OffsetHigh = 0;

	if(!ReadFile(disk,data_buffer,len,NULL,&g_p_test_disk->fp_overlapped))
	{
		if(GetLastError() != ERROR_IO_PENDING)
		{
			DBG_printf("WriteFile error %d\n",GetLastError());
			return DISK_OP_FAIL;
		}
		else
			return DISK_OP_PENDING;
	}

	return DISK_OP_SUCCESS;

}

U32 disk_wait_async_finish()
{
	HANDLE disk;
	U32 has_process;
	OVERLAPPED op;
	U32 *data_buffer;
	
	if(NULL == g_p_test_disk)
		return  TEST_DISK_NOT_EXIST;

	disk = g_p_test_disk->disk_file_async;

	if (GetOverlappedResult(disk,
		&op,
		&has_process,
		TRUE))
	{
		data_buffer = (U32*)op.hEvent;
		VirtualFree(data_buffer,0,MEM_RELEASE);
		return DISK_OP_SUCCESS;
	}
	else
	{
		DBG_printf("GetOverlappedResult error %d\n",GetLastError());
		return DISK_OP_FAIL;
	}

}

#endif
