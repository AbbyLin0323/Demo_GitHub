/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   dataverify.c                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: do data verify
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100112 first created
*************************************************/
#include <Windows.h>
#include <stdio.h>
#include "defines.h"
#include "config.h"
U32 host_data_send[MAX_LBA_WITH_DATA];  //record the sector's address
U8 data_counter[MAX_LBA_WITH_DATA];     //record the sector's counts of writing 
U32 data_timestamp[MAX_LBA_WITH_DATA];
//U8 host_data_recv[MAX_LBA_WITH_DATA];

U32 Error_lba_cnt;
U32 Error_lba[MAX_LBA_WITH_DATA];
//U32 Error_lba_total[3];

extern int abnormal_pct_fc,abnormal_pct_rw;
extern FILE *data_all_log;
extern FILE *filelog;


U8 bDump;
U8 bCheck;
void dataverify_set(U8 bDumpFlag,U8 bCheckFlag)
{
	bDump = bDumpFlag;
	bCheck = bCheckFlag;
}


void dataverify_init()
{
	U32 i;

	//data_timestamp = malloc(MAX_LBA_WITH_DATA*4);
	for( i = 0; i < MAX_LBA_WITH_DATA; i++ )
	{
		data_counter[i] = 0;
		host_data_send[i] = i;
		data_timestamp[i] = 0;
		//	host_data_recv[i] = INVALID_2F;
	}
	bDump = 0;
	bCheck = 1;
}

void dataverify_reset()
{
	dataverify_init();
}


void dataverify_set_data_before_write
(
 U32 start_lba,
 U16 sec_cnt,
 U32* tgt_data_buf
 )
{
	U32 i;
	U32 j;
	U32 cur_lba;
	U32 data_offset;

	if (start_lba+sec_cnt>syscfg_max_lba)
	{
		DBG_printf("LBA Overflow\n");
		DBG_Getch();
	}

	for (i = 0;i<sec_cnt;i++)
	{
		cur_lba = start_lba+i;

		//host_data_send[cur_lba] = cur_lba*data_counter[cur_lba];

		data_counter[cur_lba]++;
		data_timestamp[cur_lba] = GetTickCount();

		data_offset = i<<(SEC_BIT-2);
		tgt_data_buf[data_offset] = host_data_send[cur_lba];
		
		for(j=0 ; j<128 ; j++)
			tgt_data_buf[data_offset + j] =  host_data_send[cur_lba];

		tgt_data_buf[data_offset + 1] = data_timestamp[cur_lba];

		data_offset = (i+1)<<(SEC_BIT-2);
		tgt_data_buf[data_offset-1] = data_counter[cur_lba];
		tgt_data_buf[data_offset-2] = host_data_send[cur_lba];
		tgt_data_buf[data_offset-3] = data_timestamp[cur_lba];
	}

}




/*
void dataverify_get_data_after_read
(
U32 start_lba,
U16 sec_cnt,
U32 tgt_data_buf
)
{
if (start_lba+sec_cnt>MAX_LBA_WITH_DATA)
{
DBG_printf("LBA Overflow\n");
DBG_Getch();
}

}
*/

BOOL dataverify_do_verify_after_read
(
 U32 start_lba,
 U16 sec_cnt,
 U32* tgt_data_buf
 )
{
	U32 i,dwordcnt,cnt;
	U32 cur_lba;
	U32 data_offset;
	
	U32 Error_Dword_cnt = 0;


	if (start_lba+sec_cnt>syscfg_max_lba)
	{
		DBG_printf("LBA Overflow\n");
		DBG_Getch();
	}

	for (i = 0;i<sec_cnt;i++)
	{
		cur_lba = start_lba+i;

		data_offset = i<<(SEC_BIT-2);

		if(bDump)
			DBG_printf("LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);


		if(bCheck)
		{
			if(data_counter[cur_lba] == 0)
			{

				//	if(i % 0x100 == 0)
				//DBG_printf("Read not write sector LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);
				continue;
			
			}
            for(cnt=0;cnt<128;cnt++)
            {
                if (tgt_data_buf[data_offset+cnt] != host_data_send[cur_lba])
			    {
				    //data_offset = (i+1)<<(SEC_BIT-2);
                    if((cnt==1)||(cnt==127)||(cnt==(127-2)))
                        continue;

					if(abnormal_pct_rw)
					{
							//DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba+cnt]);				   
							//Error_lba = cur_lba;
							Error_Dword_cnt++;
					}
					else
					{
							if(tgt_data_buf[data_offset+cnt] == 0)
							{
								if(data_counter[cur_lba] == 1)
								{
									continue;
								}
								else
								{
									fflush(data_all_log);
									DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);				   
									DBG_Getch();
								}
							}
							else
							{
								//DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba]);				   
								fflush(data_all_log);
								DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);				   
								DBG_Getch();
							}
							//break;
					}
			    }
            }
			if(Error_Dword_cnt != 0)
			{
				Error_lba[cur_lba] = Error_Dword_cnt;
				//Error_lba_total[cur_lba] += Error_Dword_cnt;
				Error_lba_cnt++;
				Error_Dword_cnt = 0;
			}
#if 0
			if (tgt_data_buf[data_offset] != host_data_send[cur_lba])
			{
				data_offset = (i+1)<<(SEC_BIT-2);

				DBG_printf("Data check error LBA %x SecCnt %x  OpCounter %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,data_counter[cur_lba],host_data_send[cur_lba],tgt_data_buf[data_offset],data_counter[cur_lba]);
				DBG_printf("Data check error LBA %x SecCnt %x  OpCounter %x WriteData %x ReadData %x ReadCheckData %x WriteCounter %x\n",cur_lba,sec_cnt,data_counter[cur_lba],host_data_send[cur_lba],tgt_data_buf[i<<(SEC_BIT-2)] ,tgt_data_buf[data_offset-2],data_counter[cur_lba]);
				DBG_Getch();
			}

			data_offset = (i+1)<<(SEC_BIT-2);

			if (tgt_data_buf[data_offset-1] != data_counter[cur_lba])
			{
				DBG_printf("Data check error LBA %x SecCnt %x  OpCounter %x WriteData %x ReadData %x ReadCheckData %x WriteCounter %x\n",cur_lba,sec_cnt,data_counter[cur_lba],host_data_send[cur_lba],tgt_data_buf[data_offset-1] ,tgt_data_buf[data_offset-2],data_counter[cur_lba]);
				DBG_Getch();
			}
#endif


		}
/*
			if(tgt_data_buf[(i<<(SEC_BIT-2))] != host_data_send[cur_lba]*data_counter[cur_lba])
			{
				DBG_printf("Data check error LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);
				//	DBG_printf("Data check error LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);
				DBG_Getch();
			}*/
		
	}

	return TRUE;
}

BOOL dataverify_do_verify_after_read_special
(
 U32 start_lba,
 U16 sec_cnt,
 U32* tgt_data_buf
 )
{
	U32 i,dwordcnt,cnt;
	U32 cur_lba;
	U32 data_offset;
	U32 printflag = 0;
	
	U32 Error_Dword_cnt = 0;

	if (start_lba+sec_cnt>syscfg_max_lba)
	{
		DBG_printf("LBA Overflow\n");
		DBG_Getch();
	}

	for (i = 0;i<sec_cnt;i++)
	{
		cur_lba = start_lba+i;

		data_offset = i<<(SEC_BIT-2);

		if(bDump)
			DBG_printf("LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);



		if(bCheck)
		{
			if(data_counter[cur_lba] == 0)
			{

				//	if(i % 0x100 == 0)
				//DBG_printf("Read not write sector LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);
				continue;
			
			}
            for(cnt=0;cnt<128;cnt++)
            {
                if (tgt_data_buf[data_offset+cnt] != host_data_send[cur_lba])
			    {
				    //data_offset = (i+1)<<(SEC_BIT-2);
					//printflag = 1;
                    if((cnt==1)||(cnt==127)||(cnt==(127-2)))
                        continue;

					if(abnormal_pct_rw)
					{
							//DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba+cnt]);				   
							//Error_lba = cur_lba;
							Error_Dword_cnt++;
					}
					else
					{
							if(tgt_data_buf[data_offset+cnt] == 0x0)
							{
								//printflag = 1;	
								continue;
							}
							else
							{
								//if(printflag == 1)
								//{
								//DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba]);				   
								DBG_printf("data chk err read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);				   
								//fprintf(filelog,"data chk err read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);
								//fflush(filelog);
								//printflag = 0;
								//}
								DBG_Getch();
							}
							//break;
					}
			    }
				else
				{
					//if(tgt_data_buf[data_offset+cnt] == 0x0 && host_data_send[cur_lba] != 0)

					//printflag = 1;
					if(tgt_data_buf[data_offset+cnt] == 0x0)
					{
							continue;
						//DBG_printf("Read trim data. read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);				   
						//DBG_Getch();

					}
					else
					{
						//continue;
					//if(printflag == 1)
					//{
						DBG_printf("Read old data. read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);				   
						//fprintf(filelog,"Read old data. read data:addr:0x%x val:0x%x host data:0x%x Write cnt:0x%x\n",&tgt_data_buf[data_offset+cnt],tgt_data_buf[data_offset+cnt],host_data_send[cur_lba],data_counter[cur_lba]);				   

						DBG_Getch();
						//fflush(filelog);
						//printflag = 0;
					//}
					}
				}
            }
			if(Error_Dword_cnt != 0)
			{
				Error_lba[cur_lba] = Error_Dword_cnt;
				//Error_lba_total[cur_lba] += Error_Dword_cnt;
				Error_lba_cnt++;
				Error_Dword_cnt = 0;
			}
#if 0
			if (tgt_data_buf[data_offset] != host_data_send[cur_lba])
			{
				data_offset = (i+1)<<(SEC_BIT-2);

				DBG_printf("Data check error LBA %x SecCnt %x  OpCounter %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,data_counter[cur_lba],host_data_send[cur_lba],tgt_data_buf[data_offset],data_counter[cur_lba]);
				DBG_printf("Data check error LBA %x SecCnt %x  OpCounter %x WriteData %x ReadData %x ReadCheckData %x WriteCounter %x\n",cur_lba,sec_cnt,data_counter[cur_lba],host_data_send[cur_lba],tgt_data_buf[i<<(SEC_BIT-2)] ,tgt_data_buf[data_offset-2],data_counter[cur_lba]);
				DBG_Getch();
			}

			data_offset = (i+1)<<(SEC_BIT-2);

			if (tgt_data_buf[data_offset-1] != data_counter[cur_lba])
			{
				DBG_printf("Data check error LBA %x SecCnt %x  OpCounter %x WriteData %x ReadData %x ReadCheckData %x WriteCounter %x\n",cur_lba,sec_cnt,data_counter[cur_lba],host_data_send[cur_lba],tgt_data_buf[data_offset-1] ,tgt_data_buf[data_offset-2],data_counter[cur_lba]);
				DBG_Getch();
			}
#endif


		}
/*
			if(tgt_data_buf[(i<<(SEC_BIT-2))] != host_data_send[cur_lba]*data_counter[cur_lba])
			{
				DBG_printf("Data check error LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);
				//	DBG_printf("Data check error LBA %x SecCnt %x WriteData %x ReadData %x WriteCounter %x\n",cur_lba,sec_cnt,host_data_send[cur_lba],tgt_data_buf[(i<<(SEC_BIT-2))],data_counter[cur_lba]);
				DBG_Getch();
			}*/
		
	}

	return TRUE;
}


BOOL dataverify_dump_data_to_disk()
{
	HANDLE file_handle;
	U32 haswrite = 1;

	file_handle = CreateFile("data_cnt.bin",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
		NULL);

	WriteFile(file_handle,(LPVOID)data_counter, MAX_LBA_WITH_DATA * sizeof(data_counter[0]), &haswrite, NULL);
	CloseHandle(file_handle);

	file_handle = CreateFile("data_send.bin",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
		NULL);

	WriteFile(file_handle,(LPVOID)host_data_send, MAX_LBA_WITH_DATA * sizeof(host_data_send[0]), &haswrite, NULL);
	CloseHandle(file_handle);
	return TRUE;
}

BOOL dataverify_load_src_data_from_disk()
{
	HANDLE file_handle;    
	U32 hasread = 1;
	U32 err_code = 0;

	file_handle = CreateFile("data_cnt.bin",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
		NULL);

	err_code = ReadFile(file_handle,(LPVOID)data_counter, MAX_LBA_WITH_DATA *  sizeof(data_counter[0]), &hasread, NULL);

	if(!err_code)
	{
		DBG_printf("Read File Fail! ErrorCode %d\n",GetLastError());
		return FALSE;
	}
	CloseHandle(file_handle);

	file_handle = CreateFile("data_send.bin",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
		NULL);

	err_code = ReadFile(file_handle,(LPVOID)host_data_send, MAX_LBA_WITH_DATA * sizeof(host_data_send[0]), &hasread, NULL);
	if(!err_code)
	{
		DBG_printf("Read File Fail! ErrorCode %d\n",GetLastError());
		return FALSE;
	}
	CloseHandle(file_handle);
	return TRUE;
}