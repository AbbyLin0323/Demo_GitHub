/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   ioasync.c                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: disk IO with ReadFile, WriteFile, using asynchronse mode,
*              only do read/write, the command type depends on the driver
*              that the host support, so it may PIO/DMA/NCQ
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100112 first created
*************************************************/
#include "defines.h"
#include <windows.h>
#include  <stdio.h>
#include "config.h"
#include "hior.h"
#include "device.h"
#include "dataverify.h"
#include "ioasync.h"

HANDLE g_test_disk_handle;
HANDLE *g_p_event_group;
U32    g_event_group_count;

extern SYSTEMTIME sys_end_test;
extern FILE *data_all_log;
extern U32 log_line,log_file_cnt;
extern char log_filename[256];


U32 io_async_init()
{
	g_test_disk_handle = INVALID_HANDLE_VALUE;

	disk_get_async_handle(&g_test_disk_handle);

	if (INVALID_HANDLE_VALUE == g_test_disk_handle)
		return	ASYNC_IO_ERROR;

	hior_init();

	hior_eventgroup_get(&g_p_event_group,&g_event_group_count);

	return ASYNC_IO_SUCCESS;
}

U32 io_async_close()
{

	hior_close();

	return ASYNC_IO_SUCCESS;
}

U32 io_async_request_empty()
{
	//for async ncq command process
	return hior_pendingslot_empty();
}


void check_log_file()
{
	int i;
	log_line++;
	if(log_line > 1600000)
	{
		fclose(data_all_log);
		log_line = 0;
		log_file_cnt++;
		if(log_file_cnt >= 10)
		{
			//for(i = 0;i<3;i++)
			//{
			log_file_cnt = 0;	
			sprintf(log_filename,".\\output\\all_cmd_log_%d.txt",log_file_cnt);
			remove(log_filename);
			//}
			//log_file_cnt = 0;
		}
		else
		{
			sprintf(log_filename,".\\output\\all_cmd_log_%d.txt",log_file_cnt);
			remove(log_filename);
		}
		sprintf(log_filename,".\\output\\all_cmd_log_%d.txt",log_file_cnt);
		if((data_all_log = fopen(log_filename,"wt"))==NULL) 
		{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
		}
	}
}

U32 io_sync_request_execute
(
 U32 start_lba,
 U16 xfer_cnt,
 U32 rw_flag
 )
{
	U32 cmd_slot_index;
	U32 res;
	U32 io_done;
	U32 *p_data_buffer;
	U32 i;
	U32 lba_next;
	U32 lba_pre;

	io_done = 0;

	if (start_lba + xfer_cnt >syscfg_max_lba)
	{
		start_lba = syscfg_max_lba - xfer_cnt;
	//	DBG_printf("MAX_DISK_LBA LBA overflow\n");
	}

	cmd_slot_index = io_async_build_request(start_lba,xfer_cnt,rw_flag);

	if (cmd_slot_index == ASYNC_IO_NORESOURCE)
	{
		DBG_printf("io_sync_request_execute ASYNC_IO_NORESOURCE\n");
		return ASYNC_IO_NORESOURCE;
		//DBG_Getch();
	}

	res = io_async_quene_request(cmd_slot_index);

	if(ASYNC_IO_ERROR == res)
	{
		DBG_printf("io_sync_request_execute io_async_quene_request error\n");
		DBG_Getch();

	}

	if (ASYNC_IO_PENDING == res)
	{
		while (!io_done)
		{
			res = io_async_status_get_byindex(cmd_slot_index,TRUE);

			if (res == ASYNC_IO_ERROR || res == ASYNC_IO_SUCCESS)
				io_done = 1;
			else //ASYNC_IO_PARTIAL
				io_done = 0;
		}

	}

	if(rw_flag == OP_WRITE)
	{
		hior_buffer_getbyindex(cmd_slot_index,&p_data_buffer);
#if 0
		for(i = 0 ; i < xfer_cnt - 1 ;i++)
		{
			lba_next = p_data_buffer[i*128 + 128];
			lba_pre = p_data_buffer[i*128];
			if(lba_next != lba_pre + 1)
			{
				printf("fatal error\n");
				DBG_Getch();
			}
		}
#endif
	}
	if (ASYNC_IO_SUCCESS != res)
	{
		DBG_printf("io_sync_request_execute io_async_status_get_byindex error\n");
		DBG_Getch();
	}

	 if (ASYNC_IO_SUCCESS == res)
	 {
		 fprintf(data_all_log,"cmd start_lba:%x, xfer_cnt:%x, rw_flag:%x\n",start_lba,xfer_cnt,rw_flag);
		 //fflush(data_all_log);
		 check_log_file();
	 }
	return ASYNC_IO_SUCCESS;
}

U32 io_async_build_request
(	
 U32 start_lba,
 U16 xfer_cnt,
 U32 rw_flag
 )
{
	U32 cmd_slot_index;
	U32 *p_data_buffer;
	U32 i;

	if (xfer_cnt > MAX_SEC_CNT)
	{
	//	DBG_printf("request xfer_cnt %x max xfer_cnt %x\n",xfer_cnt,MAX_SEC_CNT);
		xfer_cnt = MAX_SEC_CNT;
	}

	if (start_lba + xfer_cnt >syscfg_max_lba)
	{
		start_lba = syscfg_max_lba - xfer_cnt;
		DBG_printf("MAX_DISK_LBA LBA overflow start_lba 0x%x xfer_cnt 0x%x syscfg_max_lba 0x%x\n",
			start_lba, xfer_cnt, syscfg_max_lba);
	}

	cmd_slot_index = hior_cmdslot_getfree();

	if (HIO_RESOUCE_PENDING == cmd_slot_index)
		return ASYNC_IO_NORESOURCE;

	hior_cmdslot_parameter_setbyindex(cmd_slot_index,start_lba,xfer_cnt,0,rw_flag);

	if (OP_WRITE == rw_flag)
	{
		hior_buffer_getbyindex(cmd_slot_index,&p_data_buffer);
	//	memset(p_data_buffer,start_lba,1024);
		
	/*	for(i = 0; i<128*512;i++)
			p_data_buffer[i] = start_lba;*/

		dataverify_set_data_before_write(start_lba,xfer_cnt,p_data_buffer);
	}
	else
	{
		hior_buffer_getbyindex(cmd_slot_index,&p_data_buffer);
		for(i = 0 ; i < 512*128 ; i++)
			p_data_buffer[i] = 0;

	}

	return cmd_slot_index;
}

U32 io_async_quene_request
(
 U32 cmd_slot_index
)
{
	BOOL res;
//	U32 err_code;
	U32 start_lba;
	U16 xfer_cnt;
	U32 rw_flag;
	U32 offset;
	U8 *p_data_buffer;

	LPOVERLAPPED lp_ov;
	
	if (INVALID_HANDLE_VALUE == g_test_disk_handle)
	{
		DBG_printf("io_async_quene_request g_test_disk_handle invalid\n");
		return ASYNC_IO_ERROR;
	}

	hior_cmdslot_parameter_getbyindex(cmd_slot_index,&start_lba,&xfer_cnt,&offset,&rw_flag);

	hior_buffer_getbyindex(cmd_slot_index,&p_data_buffer);

	hior_overlapped_getbyindex(cmd_slot_index,&lp_ov);

	xfer_cnt = xfer_cnt - (offset >> SEC_BIT);

	switch (rw_flag)
	{
	case OP_WRITE:
		res = WriteFile(g_test_disk_handle,p_data_buffer+offset,xfer_cnt<<SEC_BIT,NULL,lp_ov);
		break;

	case OP_READ:
		res = ReadFile(g_test_disk_handle,p_data_buffer+offset,xfer_cnt<<SEC_BIT,NULL,lp_ov);
		break;
	default:
		break;
	}

	if (res)
	{
		ResetEvent(lp_ov->hEvent);
		io_async_quene_finish(cmd_slot_index);
		return ASYNC_IO_SUCCESS;
	}
	else if (ERROR_IO_PENDING == GetLastError())
	{
		return ASYNC_IO_PENDING;
	}
	else
	{
		DBG_printf("io_async_quene_request GetLastError %x\n",GetLastError());
		return ASYNC_IO_ERROR;
	}

	return ASYNC_IO_ERROR;
}



 U32 io_async_quene_finish(U32 cmd_slot_index)
 {
	U32 start_lba;
	U16 xfer_cnt;
	U32 rw_flag;
	U32 *p_data_buffer;
	U32 offset;
	U32 i;

	 hior_cmdslot_parameter_getbyindex(cmd_slot_index,&start_lba,&xfer_cnt,&offset,&rw_flag);

	 hior_buffer_getbyindex(cmd_slot_index,&p_data_buffer);

	 if (OP_READ == rw_flag)
	 {
		 dataverify_do_verify_after_read(start_lba,xfer_cnt,p_data_buffer);
	 }
	 hior_cmdslot_finishpending(cmd_slot_index);
	 hior_cmdslot_recyclefinish(cmd_slot_index);
	
	 return ASYNC_IO_SUCCESS;
 }


 U32 io_async_status_get_byindex(U32 cmd_slot_index,BOOL block)
 {
	 LPOVERLAPPED lp_ov;
	 DWORD res;
	 DWORD req_lba;
	 U16 req_sec;
	 U32 offset;
	 DWORD req_rw;

	 hior_overlapped_getbyindex(cmd_slot_index,&lp_ov);

	 if (TRUE == block)
		 res = WaitForSingleObject(lp_ov->hEvent,INFINITE);
	 else
		 res = WaitForSingleObject(lp_ov->hEvent,ASYNC_IO_POLLING_INTETVAL);

	 if (WAIT_TIMEOUT == res)
		 return ASYNC_IO_PENDING;
	 else if (WAIT_FAILED == res)
		 return ASYNC_IO_ERROR;

	 hior_cmdslot_parameter_getbyindex(cmd_slot_index,&req_lba,&req_sec,&offset,&req_rw);

	 if(lp_ov->InternalHigh !=(req_sec << SEC_BIT)-offset)
	 {
		 GetLocalTime( &sys_end_test );
		 DBG_printf("Partail Request::InternalHigh %x, req_sec %x %x\n",lp_ov->InternalHigh,req_sec,req_sec << SEC_BIT);
		 DBG_Getch();		
		 offset += lp_ov->InternalHigh;
		 hior_cmdslot_parameter_setbyindex(cmd_slot_index,req_lba,req_sec,offset,req_rw);
		 io_async_quene_request(cmd_slot_index);
		 return ASYNC_IO_PARTIAL;
	 }

	 io_async_quene_finish(cmd_slot_index);

	 ResetEvent(lp_ov->hEvent);

	 return ASYNC_IO_SUCCESS;
 }

 U32 io_async_status_get(BOOL block)
 {
	 LPOVERLAPPED lp_ov;
	 DWORD req_lba;
	 U16 req_sec;
	 U32 offset;
	 DWORD req_rw;

	 DWORD res;
	 DWORD index;
	 DWORD t_index;

	 if (TRUE == block)
		 res = WaitForMultipleObjects(g_event_group_count,g_p_event_group,FALSE,INFINITE);
	 else
		 res = WaitForMultipleObjects(g_event_group_count,g_p_event_group,FALSE,ASYNC_IO_POLLING_INTETVAL);

	 if (WAIT_TIMEOUT == res)
		 return ASYNC_IO_PENDING;
	 else if (WAIT_FAILED == res)
		 return ASYNC_IO_ERROR;

	 index = res - WAIT_OBJECT_0;

	 ResetEvent(g_p_event_group[index]);

	 hior_overlapped_getbyindex(index,&lp_ov);

	 hior_cmdslot_parameter_getbyindex(index,&req_lba,&req_sec,&offset,&req_rw);

	 if(lp_ov->InternalHigh !=(req_sec << SEC_BIT)-offset)
	 {
		 DBG_printf("Partail Request::InternalHigh %x, req_sec %x %x\n",lp_ov->InternalHigh,req_sec,req_sec << SEC_BIT);
		 offset += lp_ov->InternalHigh;
		 hior_cmdslot_parameter_setbyindex(index,req_lba,req_sec,offset,req_rw);
		 io_async_quene_request(index);
		 return ASYNC_IO_PARTIAL;
	 }
	 else	 
		 io_async_quene_finish(index++);

	 while (index < g_event_group_count)
	 {
		 res = WaitForMultipleObjects(g_event_group_count-index,&g_p_event_group[index],FALSE,0);

		 switch (res)
		 {
		 case WAIT_TIMEOUT:
			 index = g_event_group_count;
			 break;
		 case WAIT_FAILED:
			 return ASYNC_IO_ERROR;
		 default:
			 {
				 t_index = index + res - WAIT_OBJECT_0;
				 ResetEvent(g_p_event_group[t_index]);
				 //		    DBG_printf("%x \t",index);
				 index++;
				 // io_async_quene_finish(t_index);

				 hior_overlapped_getbyindex(t_index,&lp_ov);

				 hior_cmdslot_parameter_getbyindex(t_index,&req_lba,&req_sec,&offset,&req_rw);

				 if(lp_ov->InternalHigh !=(req_sec << SEC_BIT)-offset)
					{
						DBG_printf("Partail Request::InternalHigh %x, req_sec %x %x\n",lp_ov->InternalHigh,req_sec,req_sec << SEC_BIT);
						offset += lp_ov->InternalHigh;
						hior_cmdslot_parameter_setbyindex(index,req_lba,req_sec,offset,req_rw);
						io_async_quene_request(index);
						return ASYNC_IO_PARTIAL;
					}
				 else	 
					 io_async_quene_finish(t_index);
			 }
			 break;
		 }
	 }

	 //  DBG_printf("\n",index);

	 return ASYNC_IO_SUCCESS;

 }

