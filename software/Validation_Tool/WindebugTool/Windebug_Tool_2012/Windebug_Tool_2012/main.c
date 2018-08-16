/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   main.c                                    
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: 
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
#include "device.h"
#include "dataverify.h"
#include "ioasync.h"
#include "pattern.h"
#include "hardwareapi.h"

extern int abnormal_pct_fc,abnormal_pct_rw,jedec;
extern HANDLE g_test_disk_async;
extern U32 file_cnt_total;
extern U32 trim_turn_on;

SYSTEMTIME sys_start_test,sys_end_test;
FILE *data_all_log;
U32 log_line,log_file_cnt;
char log_filename[256];

FILE *filelog;

	//char log_filename_1[256];

	//sprintf(log_filename_1,".\\output\\all_cmd_log_%d.txt",log_file_cnt);


void print_main_menu()
{
	DBG_printf("[1]: run special patterns.\n");
	DBG_printf("[12]: run pattern from file.\n");
	DBG_printf("[13]: run power down-up RW\n");
    DBG_printf("[14]: test vendor cmd\n");
	DBG_printf("[15]: run normal power down-up ACPI.\n");
	DBG_printf("[16]: run abnormal power down-up RW\n");
	DBG_printf("[17]: run abnormal power down-up FileCopy\n");
	DBG_printf("[18]: run JEDEC pattern\n");
	DBG_printf("[19]: run abnormal power down-up FileCopy new\n");
	DBG_printf("[20]: run abnormal power down-up FileCopy API\n");
    DBG_printf("[21]: run special rw test\n");
	DBG_printf("[2]: write whole disk.\n");
	DBG_printf("[3]: read whole disk.\n");
	DBG_printf("[4]: load data from disk for compare.\n");
	DBG_printf("[5]: run random write patterns.\n");
	DBG_printf("[6]: load source data from disk.\n");
	DBG_printf("[7]: read special address.\n");
	DBG_printf("[8]: save data to disk\n");
	DBG_printf("[9]: load data from disk.\n");
	DBG_printf("[0]: exit.\n");
	DBG_printf("Enter test items: ");
}

void print_main_menu_begin()
{
	DBG_printf("[1]: Verification Test Pattern.\n");
	DBG_printf("[2]: FAE Test Pattern.\n");
	DBG_printf("[0]: Exit.\n");
	DBG_printf("Enter Test Items: ");
}

void main()
{
	U32 i;
	U32 lba;
	U32 xfer_cnt;
	U32 op_code;
	//	U32 cmd_index;
	//	U32 cmd_index_new;
	//	U32 xfer_cnt_new;
	//	U32 res;
	//	U32 *tgt_data_buf;
	U32 total_sec;
	U32 c;
	U32 p;
	U32 times;
    U32 nLoop = 0;
	total_sec = 0;

	

	//config_init();
	dataverify_init();
	//i =  disk_detect_sim();
	i =  disk_detect();
	//disk_array_show(i);
	//scanf("%d",&i);
	
	if (i > 0)
		disk_init_testdisk(i - 1);

	
	io_async_init();

	//CreateFile_1GB_Trim();

	//CreateFile_1GB();

	//io_specify_trim_request(g_test_disk_async);
	log_file_cnt = 0;
	sprintf(log_filename,".\\output\\all_cmd_log_%d.txt",log_file_cnt);
	if((data_all_log = fopen(log_filename,"wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}
	log_line = 0;

	if((filelog = fopen(".\\output\\trim_special_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}

	

	c = 0;
	do
	{
		c = 0;
		
		// print main menu

		print_main_menu_begin();
		//print_main_menu();
		
		scanf("%d", &c);

		GetLocalTime( &sys_start_test );

		if( c == 1)
		{
			print_main_menu();
		
			scanf("%d", &c);
			if( c == 1 )
			{
				test_special_pattern();
			}
			else if( c == 12 )
			{
				int j;
				file_cnt_total = 0;
				for(j = 0; j < 100; j++)
				{
					test_file_pattern_all(j);
				}
			}
			else if( c == 21 )
			{
				int j;
				U32 lba_start, lba, xfer_cnt;

				lba_start = 0;
				xfer_cnt = 0x100;

				file_cnt_total = 0;
				for(j = 0; j < 1; j++)
				{
				  test_file_pattern_all(j);
				}
				times = 100;
				trim_turn_on = 1;
				test_normal_power_down_ACPI(times, i);

				for (lba = lba_start; lba< syscfg_max_lba ;lba += xfer_cnt)
				{
					io_sync_request_execute(lba,xfer_cnt,OP_READ);
					if(lba + xfer_cnt >= syscfg_max_lba)
						break;
				}  
				times = 5000*2;
				test_abnormal_power_down_rw(times, i);
				
			}

			else if( c == 22 )
			{
				
				times = 200;
				trim_turn_on = 1;
				test_abnormal_power_down_trim_special(times, i);
				
			}
			else if( c == 23 )
			{
				
				test_trim_time_special(i);
				
			}

			else if (c == 13)
			{

				times = 1000;
				test_power_down_rw(times, i);
			}
			else if (c == 14)
			{
				// test vendor cmd
				test_vendor_command();

			}

			else if (c == 15)
			{
				times = 1000;
				trim_turn_on = 1;

				test_normal_power_down_ACPI(times, i);
			}
			else if (c == 16)
			{
				
				abnormal_pct_rw = 1;
				times = 3000;
				trim_turn_on = 1;
				test_abnormal_power_down_rw(times, i);
				abnormal_pct_rw = 0;
			}
			else if(c == 17)
			{
				CreateFile_1GB_Trim();
				abnormal_pct_fc = 1;
				times = 1000;
				test_abnormal_power_down_filecopy_trim(times, i);
				abnormal_pct_fc = 0;
			}

			else if(c == 18)
			{
				jedec = 1;
				test_file_pattern_trim_flush();
				jedec = 0;
			}

			else if(c == 19)
			{
				CreateFile_80GB_Adata();
				CreateFile_30MB_Adata();
				abnormal_pct_fc = 1;
				times = 1000;
				test_abnormal_power_down_filecopy_trim_Adata(times, i);
				abnormal_pct_fc = 0;
			}

			else if(c == 20)
			{
				//CreateFile_80GB_Adata();
				CreateFile_30MB_Adata();
				abnormal_pct_fc = 1;
				times = 1000;
				test_abnormal_power_down_filecopy_trim_Adata_API(times, i);
				abnormal_pct_fc = 0;
			}

			else if( c == 2 )
			{
				test_write_whole_disk_async();
			}
			else if(c == 3)
			{
				test_write_whole_disk_sync();			
			}
			else if( c == 4 )
			{
				test_load_data_from_disk_for_compare();
			}
			else if(c == 5)
			{
				while(1)
				{
					nLoop++;
					test_random_pattern();
					printf("nLoop = %d \n", nLoop);
				}
			}
			else if(c == 6 )
			{
				test_load_data_from_disk();
			}
			else if (c == 7)
			{
				DBG_printf("Input LBA \n");
				DBG_printf("Input Sec\n");
				DBG_printf("R/W 0 R 1 W\n");

				scanf("%x",&lba);
				scanf("%x",&xfer_cnt);
				scanf("%x",&op_code);

				dataverify_set(1,0);
				io_sync_request_execute(lba, xfer_cnt,op_code);
				dataverify_set(0,1);
			}
			else if (c == 8)
			{
				test_save_data_to_disk();
			}
			else if( c==9)
			{
				dataverify_init();
				dataverify_load_src_data_from_disk();
			}
			else if( c==0xa)
			{
				for (lba = 0; lba < syscfg_max_lba ;lba += 0x200)
					io_sync_request_execute(lba,xfer_cnt,OP_READ);
			}
			else if( c == 0 )
			{
				print_main_menu_begin();
				scanf("%d", &c);
			}

		}
		else if(c == 2)
		{
			;
		}
		else if( c==0 )
		{
			//syscfg_max_lba =(syscfg_max_lba/4);
			while(1)
			{
				nLoop++;
				test_sequential_write_pattern();	
				//test_performance_pattern();
				test_read_whole_disk_sync();
				printf("nLoop = %d \n", nLoop);
			}
			break;	
		}
		else
		{
			continue;
		}
	} while( c != 0);

	printf("run finished\n");
	getchar();
	io_async_close();

}