#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include "defines.h"
#include "config.h"
#include "device.h"
#include "hior.h"
#include "dataverify.h"
#include "ioasync.h"
#include "pattern.h"
#include "hardwareapi.h"
#include<sys\stat.h>

//#define PATTERN_PATH "\\\\bjna02\\PD2\\Platform Componet\\Software\\ssd\\Project\\TestPatterns\\pattern"
#define PATTERN_PATH "D:\\\\Pattern"
#define PATTERN_JEDEC_PATH "F:\\\\test"
#define TESTFILE_PATH "D:\\\\testfile_pct"
#define TESTFILE_PATH_TRIM "D:\\\\test_folder_1"
#define TESTFILE_PATH_TRIM_30 "D:\\\\test_folder_30MB"
#define TESTFILE_PATH_TRIM_80 "D:\\\\test_folder_80GB"
#define TESTFOLDER "test_folder_1"
#define BUFFERSIZE 16 * 1024
//#define PATTERN_PATH "D:\\\\pattern_test"


WIN32_FIND_DATA FindFileData; 
HANDLE hFind; 
BOOL bFinish;

int abnormal_pct_fc,abnormal_pct_rw,jedec;
U32 file_cnt_total;
U32 trim_turn_on;

int SetTimeStart,PowerOn;

extern U32 Error_lba_cnt;
extern U32 Error_lba[MAX_LBA_WITH_DATA];
extern HANDLE g_test_disk_async;
//extern U32 Error_lba_total[MAX_LBA_WITH_DATA];


void FileNameGetInit()
{
	char sFormatFileName[256];
	if(abnormal_pct_fc == 1)
		sprintf(sFormatFileName, "%s\\*.*", TESTFILE_PATH);
	else if(jedec == 1)
		sprintf(sFormatFileName, "%s\\*.*", PATTERN_JEDEC_PATH);
	else
		sprintf(sFormatFileName, "%s\\*.*", PATTERN_PATH);
	

	hFind =  FindFirstFile(sFormatFileName, &FindFileData);

	if(hFind == INVALID_HANDLE_VALUE)
	{
		printf("Pattern path check, find no file!\n");
		getchar();
	}
	bFinish = FALSE;
}

void GetFileName(char *pFileName)
{
	while(1)
	{
		if(bFinish == TRUE)
		{
			*pFileName = '!';
			break;
		}

		if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
		{
			if(jedec == 1)
				sprintf(pFileName, "%s\\%s", PATTERN_JEDEC_PATH,FindFileData.cFileName);
			else
				sprintf(pFileName, "%s\\%s", PATTERN_PATH,FindFileData.cFileName);
		}
		else
		{
			*pFileName = '@';
		}

		if(!FindNextFile(hFind, &FindFileData))  
		{ 
			if (GetLastError() == ERROR_NO_MORE_FILES)  
			{ 
				bFinish = TRUE;  
			}  
			else  
			{ 
				printf("File Find Error!\n");
				getchar();
			}  
		} 

		if(*pFileName != '@')
			break;
	}

}

void test_vendor_command(void)
{
    /* 1. Execute DRAM initialization sequence for target board through Register Read/Write commands: */
    InitializeTargetSDRAM(SDRAM_TYPE_DDR2);

    /* 2. Switch active memory space from OTFB to DDR SDRAM on target */
    SetTargetMemStorageArea(MEMREGION_DRAM);
}

void test_random_pattern(void)
{
	U32 LBAStart=0;
	U16 SectorCount=0;
	U8 cmd_code;
	U32 cmd_cnt = 0;

	U32 total_sec_cnt = 0;


	U8 buf[256];

	int haswrite = 1;

	HANDLE file_handle;

	file_handle = CreateFile("randtestpattern.txt",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,
		NULL);

	if (INVALID_HANDLE_VALUE == file_handle)
	{
		DBG_printf("test_random_pattern file create fail,GetLastError() %x\n",GetLastError());
		return;
	}

	srand((unsigned int)time(0));

	while(1)
	{
		LBAStart = (rand()* rand()) % syscfg_max_lba;
		SectorCount = rand() % 256 + 1;

		cmd_code = OP_WRITE;//ATA_CMD_WRITE_FPDMA_QUEUED;

		if((LBAStart +SectorCount) >= syscfg_max_lba)	
			continue;
		else
			break;
	}

	while(1)
	{
		if( ASYNC_IO_SUCCESS == io_sync_request_execute(LBAStart, SectorCount,cmd_code) )
		{
			total_sec_cnt += SectorCount;
			if( total_sec_cnt >= syscfg_max_lba)
				break;

			sprintf_s(buf, 256, "%d\t%f\t%f\t%d\t%c\t%d\t%d\n",0,0.0,0.0,0,'W', LBAStart, SectorCount);

			WriteFile(file_handle,(LPVOID)buf, strlen(buf), &haswrite, NULL);
			//printf("write lba = 0x%x, count = 0x%x \n", LBAStart, SectorCount);
			while(1)
			{
				LBAStart = (rand()* rand()) % syscfg_max_lba;
				SectorCount = rand() % 256 + 1;

				cmd_code = OP_WRITE;//ATA_CMD_WRITE_FPDMA_QUEUED;

				if((LBAStart +SectorCount) >= syscfg_max_lba)	
					continue;
				else
					break;
			}

			cmd_cnt++;
			//if( cmd_cnt > 14070 )
			{
				if( cmd_cnt % 20000 == 0 )
				{
					DBG_printf("cmd count = %d.\n", cmd_cnt);
				}
			}
		}
	}

	CloseHandle(file_handle);

	test_read_whole_disk_sync();
}


void test_read_whole_disk_sync()
{
	U32 lba;
	U16 xfer_cnt;
	U32 p;

	xfer_cnt = 0x100; 
	dataverify_set(0,1);
	p = GetTickCount();
	//syscfg_max_lba = 0x10000;
	
	for (lba = 0; lba < syscfg_max_lba ;lba += xfer_cnt)
		io_sync_request_execute(lba,xfer_cnt,OP_READ);

	p = GetTickCount()-p;
	DBG_printf("test_read_whole_disk_sync Total time %d\n",p);

}

void test_write_whole_disk_sync()
{
	U32 lba;
	U32 xfer_len;
	U16 xfer_cnt;
	U32 p;
	U32 i;
	U32 data_xfer_mb;
	i = 0;
	xfer_cnt = 0x100;
    dataverify_set(0,1);
//	p = GetTickCount();
p= 0 ;
	//syscfg_max_lba = 0x3ffff;
	data_xfer_mb = 4;
	xfer_len = data_xfer_mb*512;

	for(p=0;p<10000;p++)
	{
		if(i*xfer_len + xfer_len > syscfg_max_lba)
			i=0;

		DBG_printf("loop %d lba %x\t",p,i*xfer_len);
		if (255 == p)
		{
		    DBG_printf("!");
		}
		for (lba = i*xfer_len ; lba < i*xfer_len + xfer_len ;lba += xfer_cnt)
		{
			io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
			//io_sync_request_execute(lba,xfer_cnt,OP_READ);
		}

		disk_special_cmd(0xe7);

		disk_special_cmd(0xe0);

		
		for (lba = i*xfer_len; lba < i*xfer_len + xfer_len ;lba += xfer_cnt)
		{
			io_sync_request_execute(lba,xfer_cnt,OP_READ);
			//io_sync_request_execute(lba,xfer_cnt,OP_READ);
		}

		i++;

		DBG_printf("after check done\n");

	}


	test_read_whole_disk_sync();
	/*
	lba = 0x101; 
	io_sync_request_execute(lba,xfer_cnt,OP_WRITE);

	syscfg_max_lba = 0x20000;

	for (lba = 0x10000; lba < syscfg_max_lba ;lba += xfer_cnt)
	{
		io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
		//io_sync_request_execute(lba,xfer_cnt,OP_READ);
	}*/
//	p = GetTickCount()-p;
//	DBG_printf("test_write_whole_disk_sync Total time %d\n",p);

}

void test_special_pattern(void)
{
	U32 start_lba;
	U16 sec_cnt;
	U8 cmd_code;
	U32 cmd_cnt;
	U32 whole_write_times = 0;

	start_lba = 0;
	sec_cnt = 1;
	cmd_code = OP_WRITE;

	cmd_cnt = 0;
	while(1)
	{
		if( ASYNC_IO_SUCCESS == io_sync_request_execute(start_lba, sec_cnt,cmd_code ))
		{
#if 0		
			//cmd_code = ATA_CMD_READ_DMA;
			start_lba += 16;
			sec_cnt = 2;
#endif
			start_lba += (4+1) << 2;

			sec_cnt++;			
			if( sec_cnt == 256 ) sec_cnt = 1;


			if( start_lba + sec_cnt >= syscfg_max_lba )
			{
				sec_cnt = syscfg_max_lba - start_lba;
			}

			if( start_lba >= syscfg_max_lba)//syscfg_max_lba )
			{
				whole_write_times++;
				start_lba = 0;
				cmd_cnt = 0;
				sec_cnt = 1;

				DBG_printf("Whole write times = %d.\n", whole_write_times);

				if( 1 == whole_write_times )
				{
					DBG_printf("done.\n");
					break;
				}
			}

			cmd_cnt++;

			if( cmd_cnt % 280 == 0)
			{
				DBG_printf("cmd_cnt = %d\n", cmd_cnt);
				//test_read_whole_disk_sync();
			}

			if( cmd_cnt % 10000 == 0)
			{
				DBG_printf("Do table check please.\n");
				//DBG_Getch();
			}

		}
	}
}
void test_sequential_write_pattern(void)
{
	U32 start_lba;
	U16 sec_cnt;
	U8 cmd_code;
	U32 cmd_cnt;
	U32 whole_write_times = 0;

	start_lba = 0x0;
	sec_cnt = 64;
	cmd_code = OP_WRITE;
	
	cmd_cnt = 0;
	while(1)
	{
		//printf("start_lba:0x%x sec_cnt:0x%x\n",start_lba,sec_cnt);
		if( ASYNC_IO_SUCCESS == io_sync_request_execute(start_lba, sec_cnt,cmd_code ))
		{
			start_lba += sec_cnt;

			if( start_lba + sec_cnt >= syscfg_max_lba )
			{
				sec_cnt = syscfg_max_lba - start_lba;
			}
			//if(start_lba>0x20000)
			//	printf("!!\n");

			if( start_lba >= syscfg_max_lba)//syscfg_max_lba )
			{
				whole_write_times++;
				start_lba = 0;
				cmd_cnt = 0;
				sec_cnt = 1;

				DBG_printf("Whole write times = %d.\n", whole_write_times);

				if( 1 == whole_write_times )
				{
					DBG_printf("done.\n");
					break;
				}
			}

			cmd_cnt++;

			if( cmd_cnt % 2000 == 0)
			{
				DBG_printf("cmd_cnt = %d\n", cmd_cnt);
				//test_read_whole_disk_sync();
			}

			if( cmd_cnt % 10000 == 0)
			{
				//DBG_printf("Do table check please.\n");
				//DBG_Getch();
			}

		}
	}
}

void test_performance_pattern(void)
{
	U32 start_lba;
	U16 sec_cnt;
	U8 cmd_code;
	U32 cmd_cnt;
	U32 whole_write_times = 0;

	start_lba = 0x0;
	sec_cnt = 64;
	cmd_code = OP_WRITE;
	
	cmd_cnt = 0;
	while(1)
	{
		//printf("start_lba:0x%x sec_cnt:0x%x\n",start_lba,sec_cnt);
		if( ASYNC_IO_SUCCESS == io_sync_request_execute(start_lba, sec_cnt,cmd_code ))
		{
			start_lba += sec_cnt;

			if( start_lba + sec_cnt >= syscfg_max_lba )
			{
				sec_cnt = syscfg_max_lba - start_lba;
			}
			//if(start_lba>0x20000)
			//	printf("!!\n");

			if( start_lba >= syscfg_max_lba)//syscfg_max_lba )
			{
				whole_write_times++;
				start_lba = 0;
				cmd_cnt = 0;
				sec_cnt = 1;

				DBG_printf("Whole write times = %d.\n", whole_write_times);

				if( 1 == whole_write_times )
				{
					DBG_printf("done.\n");
					break;
				}
			}

			cmd_cnt++;

			if (cmd_cnt == 500)
		       {
				cmd_cnt = cmd_cnt;
			       start_lba = 0x0;
				cmd_code = OP_READ;
			}

			if (cmd_cnt >= 1000)
		       {
				cmd_cnt = cmd_cnt;
			}
			

			if( cmd_cnt % 2000 == 0)
			{
				DBG_printf("cmd_cnt = %d\n", cmd_cnt);
				//test_read_whole_disk_sync();
			}

			if( cmd_cnt % 10000 == 0)
			{
				//DBG_printf("Do table check please.\n");
				//DBG_Getch();
			}

		}
	}
}



void test_do_one_file_pattern(char * pfile_name)
{
	int index=0;
	float time1=0.0;
	float time2=0.0;
	int p=0;
	char op[7];
	unsigned long LBAStart=0;
	int SectorCount=0;
	U8 cmd_code;
	U32 cmd_cnt = 0;
	U32 cmd_cnt_read = 0;
	U32 cmd_cnt_write = 0;
	U32 total_sectorCnt;

	int i = 0;
	int j = 100;

	FILE *p_file;

	
	total_sectorCnt = 0;
	p_file = fopen(pfile_name,"r");

	if(p_file == NULL) 
	{
		printf("LbaPatternFile Open Fail\n");
		return;
	}	

	dataverify_set(0,1);

	while(1)
	{
		fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\r\n",
			&index,&time1,&time2,&p,op,&LBAStart,&SectorCount);

		if(op[0]=='R'||op[0]=='r') 
		{
			cmd_code = OP_READ;
		}
		else if(op[0] == 'W'||op[0] == 'w')
		{
			cmd_code = OP_WRITE;
		}

		
		while((LBAStart + SectorCount) >= syscfg_max_lba)	
		{
			LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;

		}

		while((LBAStart + SectorCount) >= syscfg_max_accessable_lba)	
		{
			LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;

		}

		{
			total_sectorCnt += SectorCount;
			//printf("total_sectorCnt = %d\n", total_sectorCnt);
			break;
		}

	}

	while(1)
	{
		if( ASYNC_IO_SUCCESS == io_sync_request_execute(LBAStart, SectorCount,cmd_code) )
		{
			total_sectorCnt += SectorCount;
			//DBG_printf("index %d readCmd = %d, writeCmd = %d, totalsector = %d.\n", index,cmd_cnt_read, cmd_cnt_write, total_sectorCnt);

#if 0			
			if( total_sectorCnt >= syscfg_max_accessable_lba*1000 )
			{

				fclose(p_file);
				DBG_printf("reach ten times write.\n");
				return;

			}
#endif

			while(1)
			{
				if(!feof(p_file))
				{
					fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\r\n",
						&index,&time1,&time2,&p,op,&LBAStart,&SectorCount);
					if(op[0]=='R'||op[0]=='r') 
					{
						
//						continue;
						cmd_code = OP_READ;
						//if( LBAStart == 0x43d827 )
						//	DBG_Printf("got it.\n");
						cmd_cnt_read++;
						cmd_cnt ++;
					}
					else if(op[0]=='W'||op[0]=='w') 
					{
						cmd_code = OP_WRITE;
						cmd_cnt_write++;
						cmd_cnt ++;
					}

					while((LBAStart + SectorCount) >= syscfg_max_accessable_lba)	
					{
						LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;

					}

					{
						//total_sectorCnt += SectorCount;
						//	printf("total_sectorCnt = %d\n", total_sectorCnt);
						if( (cmd_cnt_read % 10000 == 0 && cmd_cnt_read != 0 ) || 
								(cmd_cnt_write % 10000 == 0 && cmd_cnt_write != 0 ) )
						{
							DBG_printf("index %d readCmd = %d, writeCmd = %d, totalsector = %d.\n", index,cmd_cnt_read, cmd_cnt_write, total_sectorCnt);
							//printf("total_sectorCnt = %d\n", total_sectorCnt);
						}

#if 0
						if (cmd_cnt == 800)
						{
							disk_special_cmd(0xe7);

							disk_special_cmd(0xe0);

                            			cmd_cnt = 0;
							DBG_printf("power off point!\n\n");
						}
#endif
						break;
					}

				}
				else
				{
					fclose(p_file);
					DBG_printf("read command = %d, write command = %d.\n", cmd_cnt_read, cmd_cnt_write);
					printf("total_sectorCnt = %d\n", total_sectorCnt);
					return;
				}
			}
		}
	}
}

void test_do_one_file_pattern_trim_flush(char * pfile_name,HANDLE disk)
{
	int index=0;
	float time1=0.0;
	float time2=0.0;
	int p=0;
	char op[7];
	unsigned long LBAStart=0;
	int SectorCount=0;
	U8 cmd_code;
	U32 cmd_cnt = 0;
	U32 cmd_cnt_read = 0;
	U32 cmd_cnt_write = 0;
	U32 total_sectorCnt;

	int i = 0;
	int j = 100;
	int k = 0;
	int y = 1;

	FILE * p_file;

	struct Trim
	{
		U32 Lba_Low;
		U16 Lba_High;
		U16 Range_trim;
	}trim_cmd[128];
	U32 *buf;
	buf = (U32 *)malloc(sizeof(U32)*256);

	
	total_sectorCnt = 0;
	p_file = fopen(pfile_name,"r");

	if(p_file == NULL) 
	{
		printf("LbaPatternFile Open Fail\n");
		return;
	}	

	dataverify_set(0,1);

	while(1)
	{
		fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\r\n",
			&index,&time1,&time2,&p,op,&LBAStart,&SectorCount);

		if(op[0]=='R'||op[0]=='r') 
		{
			cmd_code = OP_READ;
		}
		else if(op[0] == 'W'||op[0] == 'w')
		{
			cmd_code = OP_WRITE;
		}

		
		while((LBAStart + SectorCount) >= syscfg_max_lba)	
		{
			LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;

		}

		while((LBAStart + SectorCount) >= syscfg_max_accessable_lba)	
		{
			LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;

		}

		{
			total_sectorCnt += SectorCount;
			//printf("total_sectorCnt = %d\n", total_sectorCnt);
			break;
		}

	}

	while(1)
	{
		if( ASYNC_IO_SUCCESS == io_sync_request_execute(LBAStart, SectorCount,cmd_code) )
		{
			if(cmd_cnt > (y * 1000))
			{
				printf("finish cmd %d now.\n",cmd_cnt);
				y++;
			}

			total_sectorCnt += SectorCount;

#if 0			
			if( total_sectorCnt >= syscfg_max_accessable_lba*1000 )
			{

				fclose(p_file);
				DBG_printf("reach ten times write.\n");
				return;

			}
#endif

			while(1)
			{
				if(!feof(p_file))
				{
					fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\r\n",
						&index,&time1,&time2,&p,op,&LBAStart,&SectorCount);
					if(op[0]=='R'||op[0]=='r') 
					{
						
//						continue;
						cmd_code = OP_READ;
						//if( LBAStart == 0x43d827 )
						//	DBG_Printf("got it.\n");
						cmd_cnt_read++;
						cmd_cnt ++;
					}
					else if(op[0]=='W'||op[0]=='w') 
					{
						cmd_code = OP_WRITE;
						cmd_cnt_write++;
						cmd_cnt ++;
					}
					else if(op[0] == 'f')
					{
						disk_special_cmd(0xe0);   //flush disk cache
						Sleep(2000);
						cmd_cnt++;
						fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\r\n",&index,&time1,&time2,&p,op,&LBAStart,&SectorCount);
						if(op[0]=='R'||op[0]=='r') 
						{
							
	//						continue;
							cmd_code = OP_READ;
							//if( LBAStart == 0x43d827 )
							//	DBG_Printf("got it.\n");
							cmd_cnt_read++;
							cmd_cnt ++;
						}
						else if(op[0]=='W'||op[0]=='w') 
						{
							cmd_code = OP_WRITE;
							cmd_cnt_write++;
							cmd_cnt ++;
						}
					}
					while(op[0] == 't')
					{
						
						if(k < 128)
						{
							if((LBAStart + SectorCount) >= syscfg_max_accessable_lba)
								LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;
							trim_cmd[k].Lba_Low = LBAStart;
							trim_cmd[k].Lba_High = 0x0;
							trim_cmd[k].Range_trim = SectorCount;
							k++;
							cmd_cnt++;
						}
						else
						{
							memcpy(buf,trim_cmd,(sizeof(trim_cmd)));
							sata_cmd_dma_write_trim(disk,buf,2);
							k = 0;
							if((LBAStart + SectorCount) >= syscfg_max_accessable_lba)
								LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;
							trim_cmd[k].Lba_Low = LBAStart;
							trim_cmd[k].Lba_High = 0x0;
							trim_cmd[k].Range_trim = SectorCount;
							k++;
							cmd_cnt++;
						}
						fscanf(p_file,"%d\t%f\t%f\t%d\t%s\t%d\t%d\r\n",&index,&time1,&time2,&p,op,&LBAStart,&SectorCount);
					}
					if(k != 0 && k < 128)
					{
						while(k < 128)
						{
							trim_cmd[k].Lba_Low = 0x0;
							trim_cmd[k].Lba_High = 0x0;
							trim_cmd[k].Range_trim = 0x0;
							k++;
							cmd_cnt++;
						}
						memcpy(buf,trim_cmd,(sizeof(trim_cmd)));
						sata_cmd_dma_write_trim(disk,buf,2);
						k = 0;
					}

					while((LBAStart + SectorCount) >= syscfg_max_accessable_lba)	
					{
						LBAStart = (LBAStart + SectorCount) % syscfg_max_accessable_lba ;

					}

					{
						//total_sectorCnt += SectorCount;
						//	printf("total_sectorCnt = %d\n", total_sectorCnt);
						if( (cmd_cnt_read % 10000 == 0 && cmd_cnt_read != 0 ) || 
								(cmd_cnt_write % 10000 == 0 && cmd_cnt_write != 0 ) )
						{
							DBG_printf("index %d readCmd = %d, writeCmd = %d, totalsector = %d.\n", index,cmd_cnt_read, cmd_cnt_write, total_sectorCnt);
							//printf("total_sectorCnt = %d\n", total_sectorCnt);
						}

#if 0
						if (cmd_cnt == 800)
						{
							disk_special_cmd(0xe7);

							disk_special_cmd(0xe0);

                            			cmd_cnt = 0;
							DBG_printf("power off point!\n\n");
						}
#endif
						break;
					}

				}
				else
				{
					fclose(p_file);
					DBG_printf("read command = %d, write command = %d.\n", cmd_cnt_read, cmd_cnt_write);
					printf("total_sectorCnt = %d\n", total_sectorCnt);
					return;
				}
			}
		}
	}
	free(buf);
}

void test_write_whole_disk_async()
{
	//	U32 i;
	U32 lba;
	U16 xfer_cnt;
	U32 cmd_index;
	//	U32 cmd_index_new;
	U32 xfer_cnt_new;
	U32 res;
	//	U32 *tgt_data_buf;
	U32 total_sec;
	U32 p;

	lba = 0x0;
	xfer_cnt = 0x200;
	total_sec = 0;
	xfer_cnt_new = 0;

	p = GetTickCount();

	while(1)
	{
		if (total_sec == syscfg_max_lba && io_async_request_empty())
			break;

		if (total_sec == syscfg_max_lba && !io_async_request_empty())
		{
			DBG_printf("wait write queue empty\n");
			io_async_status_get(0);
			continue;
		}

		cmd_index = io_async_build_request(lba,xfer_cnt,OP_WRITE);

		if (cmd_index == ASYNC_IO_NORESOURCE)
		{
			//DBG_printf("ASYNC_IO_NORESOURCE \n");
			io_async_status_get(0);
			continue;
		}

		res = io_async_quene_request(cmd_index);

		total_sec += xfer_cnt;

		if ( lba + xfer_cnt < syscfg_max_lba )
		{
			lba += xfer_cnt;
			//xfer_cnt += 0x100;

			if (lba + xfer_cnt > syscfg_max_lba)
				xfer_cnt = syscfg_max_lba - lba;
		}
		/*
		if (res ==ASYNC_IO_PENDING || cmd_index == ASYNC_IO_NORESOURCE)
		{
		//DBG_printf("Queue Read Requset\n");
		if(p++%4 ==0 )io_async_status_get(0);
		}
		*/

	}
	p = GetTickCount()-p;
	DBG_printf("test_write_whole_disk_async Total time %d\n",p);
	hior_list_check();

}

void test_read_whole_disk_async()
{
	//	U32 i;
	U32 lba;
	U16 xfer_cnt;
	U32 cmd_index;
	//	U32 cmd_index_new;
	U32 xfer_cnt_new;
	U32 res;
	//	U32 *tgt_data_buf;
	U32 total_sec;
	U32 p;

	lba = 0x0;
	xfer_cnt = 0x200;
	total_sec = 0;
	xfer_cnt_new = 0;

	p = GetTickCount();

	while(1)
	{
		if (total_sec == syscfg_max_lba && io_async_request_empty())
			break;

		if (total_sec == syscfg_max_lba && !io_async_request_empty())
		{
			DBG_printf("wait read queue empty\n");
			io_async_status_get(0);
			continue;
		}

		cmd_index = io_async_build_request(lba,xfer_cnt,OP_READ);

		if (cmd_index == ASYNC_IO_NORESOURCE)
		{
			//DBG_printf("ASYNC_IO_NORESOURCE \n");
			io_async_status_get(0);
			continue;
		}

		res = io_async_quene_request(cmd_index);

		total_sec += xfer_cnt;

		if ( lba + xfer_cnt < syscfg_max_lba )
		{
			lba += xfer_cnt;
			//xfer_cnt += 0x100;

			if (lba + xfer_cnt > syscfg_max_lba)
				xfer_cnt = syscfg_max_lba - lba;
		}
		/*
		if (res ==ASYNC_IO_PENDING || cmd_index == ASYNC_IO_NORESOURCE)
		{
		//DBG_printf("Queue Read Requset\n");
		if(p++%4 ==0 )io_async_status_get(0);
		}
		*/

	}
	p = GetTickCount()-p;
	DBG_printf("test_read_whole_disk_async Total time %d\n",p);
	hior_list_check();

}

void test_special_pattern_sync(void)
{
	U32 lba;
	U16 xfer_cnt;
	U32 p;

	xfer_cnt = 0x100;

	p = GetTickCount();

	for (lba = 0; lba < syscfg_max_lba ;lba += xfer_cnt)
		io_sync_request_execute(lba,xfer_cnt,OP_WRITE);

	p = GetTickCount()-p;
	DBG_printf("Total write time %d\n",p);


	p = GetTickCount();

	for (lba = 0; lba < syscfg_max_lba ;lba += xfer_cnt)
		io_sync_request_execute(lba,xfer_cnt,OP_READ);

	p = GetTickCount()-p;
	DBG_printf("Total read time %d\n",p);
	hior_list_check();
}

void test_special_pattern_sync_1(void)
{
	U32 lba;
	U16 xfer_cnt;
	U32 p;

	xfer_cnt = 0x100;

	p = GetTickCount();

	for (lba = 0; lba < syscfg_max_lba ;lba += xfer_cnt)
	{
		io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
		io_sync_request_execute(lba,xfer_cnt,OP_READ);
	}
	p = GetTickCount()-p;

	DBG_printf("Total write/read time %d\n",p);
	hior_list_check();
}



void test_special_pattern_none_ncq(void)
{
	//	U32 i;
	U32 lba;
	U16 xfer_cnt;
	U32 cmd_index;
	U32 cmd_index_new;
	U32 xfer_cnt_new;
	U32 res;
	//	U32 *tgt_data_buf;
	U32 total_sec;
	U32 p;

	total_sec = 0;

	lba = 0;
	xfer_cnt = 1;
	total_sec = 0;
	xfer_cnt_new = 0;

	p = GetTickCount();

	cmd_index = io_async_build_request(lba,xfer_cnt,OP_WRITE);


	while(total_sec < syscfg_max_lba)
	{
		res = io_async_quene_request(cmd_index);

		if (res == ASYNC_IO_SUCCESS)
			total_sec += xfer_cnt;

		if ( lba + xfer_cnt < syscfg_max_lba )
		{
			xfer_cnt_new = xfer_cnt;
			lba += xfer_cnt;
			xfer_cnt += 10;

			if (lba + xfer_cnt > syscfg_max_lba)
				xfer_cnt = syscfg_max_lba - lba;

			cmd_index_new = io_async_build_request(lba,xfer_cnt,OP_WRITE);
		}

		if (res ==ASYNC_IO_PENDING)
		{
			//	DBG_printf("Queue Read Requset\n");
			io_async_status_get_byindex(cmd_index,TRUE);
			total_sec += xfer_cnt_new;
		}


		cmd_index = cmd_index_new;

	}

	p = GetTickCount()-p;
	DBG_printf("Total time %d\n",p);

	hior_list_check();

	lba = 0x0;
	xfer_cnt = 0x1;
	total_sec = 0;
	xfer_cnt_new = 0;
	p = GetTickCount();
	cmd_index = io_async_build_request(lba,xfer_cnt,OP_READ);


	while(total_sec < syscfg_max_lba)
	{
		res = io_async_quene_request(cmd_index);

		if (res == ASYNC_IO_SUCCESS)
			total_sec += xfer_cnt;

		xfer_cnt_new = xfer_cnt;

		if ( lba + xfer_cnt < syscfg_max_lba )
		{
			lba += xfer_cnt;
			xfer_cnt += 0x10;

			if (lba + xfer_cnt > syscfg_max_lba)
				xfer_cnt = syscfg_max_lba - lba;

			cmd_index_new = io_async_build_request(lba,xfer_cnt,OP_READ);
		}

		if (res ==ASYNC_IO_PENDING)
		{
			//	DBG_printf("Queue Read Requset\n");
			io_async_status_get_byindex(cmd_index,TRUE);
			total_sec += xfer_cnt_new;
		}

		cmd_index = cmd_index_new;
	}
	p = GetTickCount()-p;
	DBG_printf("Total time %d\n",p);
	hior_list_check();
}


void test_special_pattern_ncq(void)
{
	//	U32 i;
	U32 lba;
	U16 xfer_cnt;
	U32 cmd_index;
	//	U32 cmd_index_new;
	U32 xfer_cnt_new;
	U32 res;
	//	U32 *tgt_data_buf;
	U32 total_sec;
	U32 p;

	p = 0;
	total_sec = 0;
	/*
	lba = 0;
	xfer_cnt = 0x1;
	total_sec = 0;
	xfer_cnt_new = 0;
	p = GetTickCount();
	cmd_index = io_async_build_request(lba,xfer_cnt,OP_WRITE);

	while(total_sec < MAX_DISK_LBA)
	{
	res = io_async_quene_request(cmd_index,lba,xfer_cnt,OP_WRITE);

	if (res == ASYNC_IO_SUCCESS)
	total_sec += xfer_cnt;

	if ( lba + xfer_cnt < MAX_DISK_LBA )
	{
	xfer_cnt_new = xfer_cnt;
	lba += xfer_cnt;
	xfer_cnt += 10;

	if (lba + xfer_cnt > MAX_DISK_LBA)
	xfer_cnt = MAX_DISK_LBA - lba;

	cmd_index_new = io_async_build_request(lba,xfer_cnt,OP_WRITE);
	}

	if (res ==ASYNC_IO_PENDING)
	{
	DBG_printf("Queue Read Requset\n");
	io_async_status_get_byindex(cmd_index,TRUE);
	total_sec += xfer_cnt_new;
	}


	cmd_index = cmd_index_new;

	}
	hior_list_check();
	p = GetTickCount()-p;
	DBG_printf("Total time %d\n",p);
	*/
	lba = 0x0;
	xfer_cnt = 0x200;
	total_sec = 0;
	xfer_cnt_new = 0;

	p = GetTickCount();

	while(1)
	{
		if (total_sec == syscfg_max_lba && io_async_request_empty())
			break;

		if (total_sec == syscfg_max_lba && !io_async_request_empty())
		{
			DBG_printf("wait queue empty\n");
			io_async_status_get(0);
			continue;
		}

		cmd_index = io_async_build_request(lba,xfer_cnt,OP_WRITE);

		if (cmd_index == ASYNC_IO_NORESOURCE)
		{
			//DBG_printf("ASYNC_IO_NORESOURCE \n");
			io_async_status_get(0);
			continue;
		}

		res = io_async_quene_request(cmd_index);

		total_sec += xfer_cnt;

		if ( lba + xfer_cnt < syscfg_max_lba )
		{
			lba += xfer_cnt;
			//xfer_cnt += 0x100;

			if (lba + xfer_cnt > syscfg_max_lba)
				xfer_cnt = syscfg_max_lba - lba;
		}
		/*
		if (res ==ASYNC_IO_PENDING || cmd_index == ASYNC_IO_NORESOURCE)
		{
		DBG_printf("Queue Read Requset\n");
		if(p++%4 ==0 )io_async_status_get(0);
		}
		*/

	}
	p = GetTickCount()-p;
	DBG_printf("Total time %d\n",p);
	hior_list_check();


	lba = 0x0;
	xfer_cnt = 0x200;
	total_sec = 0;
	xfer_cnt_new = 0;

	p = GetTickCount();

	while(1)
	{
		if (total_sec == syscfg_max_lba && io_async_request_empty())
			break;

		if (total_sec == syscfg_max_lba && !io_async_request_empty())
		{
			DBG_printf("wait read queue empty\n");
			io_async_status_get(0);
			continue;
		}

		cmd_index = io_async_build_request(lba,xfer_cnt,OP_READ);

		if (cmd_index == ASYNC_IO_NORESOURCE)
		{
			//DBG_printf("ASYNC_IO_NORESOURCE \n");
			io_async_status_get(0);
			continue;
		}

		res = io_async_quene_request(cmd_index);

		total_sec += xfer_cnt;

		if ( lba + xfer_cnt < syscfg_max_lba )
		{
			lba += xfer_cnt;
			//xfer_cnt += 0x100;

			if (lba + xfer_cnt > syscfg_max_lba)
				xfer_cnt = syscfg_max_lba - lba;
		}
		/*
		if (res ==ASYNC_IO_PENDING || cmd_index == ASYNC_IO_NORESOURCE)
		{
		//DBG_printf("Queue Read Requset\n");
		if(p++%4 ==0 )io_async_status_get(0);
		}
		*/

	}
	p = GetTickCount()-p;
	DBG_printf("Total time %d\n",p);
	hior_list_check();

}

void test_load_data_from_disk_for_compare(void)
{
	dataverify_init();
	dataverify_load_src_data_from_disk();
	test_read_whole_disk_sync();
}

void test_file_pattern(void)
{
	char file_name[256];
	U32 file_cnt = 0;

	FileNameGetInit();

	GetFileName(file_name);

	while(file_name[0] != '!')
	{
		printf("%d File Name: %s\n", file_cnt, file_name);
		test_do_one_file_pattern(file_name);
		GetFileName(file_name);
		file_cnt ++;
	}	
}

void test_file_pattern_all(int i)
{
	char file_name[256];
	U32 file_cnt = 1;
	file_cnt_total = file_cnt + (i * 60);

	FileNameGetInit();

	GetFileName(file_name);

	while(file_name[0] != '!')
	{
		printf("%d File Name: %s. Total file number is %d.\n", file_cnt, file_name,file_cnt_total);
		test_do_one_file_pattern(file_name);
		GetFileName(file_name);
		file_cnt ++;
		file_cnt_total = file_cnt + (i * 60);
	}	
}

void test_file_pattern_trim_flush(void)
{
	char file_name[256];
	U32 file_cnt = 0;

	FileNameGetInit();

	GetFileName(file_name);

	while(file_name[0] != '!')
	{
		printf("%d File Name: %s\n", file_cnt, file_name);
		test_do_one_file_pattern_trim_flush(file_name,g_test_disk_async);
		GetFileName(file_name);
		file_cnt ++;
	}	
}

void test_save_data_to_disk()
{
	dataverify_dump_data_to_disk();
}


void test_load_data_from_disk()
{
	dataverify_load_src_data_from_disk();
}

void test_power_down_rw(UINT32 times, UINT32 nDevice)
{
	BOOL rw_normal = TRUE;
	U32 loop_cnt = 0;
	U32 i = 0;

	U32 lba = 0;
	U16 xfer_cnt;
	//U32 p;
	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	U32 lba_start = 0;
	U32 lba_wCount = 4096 * 15;  // 4096 = 2M

#ifdef WITHJTAG
	
	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 100)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;
		xfer_cnt = 0x1000;    // 0x100 = 128K

		for (lba = lba_start; lba - lba_start < lba_wCount ;lba += xfer_cnt)
			io_sync_request_execute(lba,xfer_cnt,OP_WRITE);

		//disk_special_cmd(0xe7);   //flush disk cache
		disk_special_cmd(0xe0);   //flush disk cache
		Sleep(2000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		Sleep(6000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		Sleep(4000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;
		for (lba = lba_start; lba - lba_start < lba_wCount - 0x400 ;lba += xfer_cnt)
			io_sync_request_execute(lba,xfer_cnt,OP_READ);

		// check data;

		lba_start +=  lba_wCount;
		if (lba_start + lba_wCount > syscfg_max_lba)
			lba_start = 0;

			DBG_printf("loop %d pass\n",loop_cnt);

	}
	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

#endif
	
	return;
}




union {
FILETIME ft;
LONG64 val;
}t1,t2;

U32 CaculateTime(SYSTEMTIME sys_time_cur,SYSTEMTIME sys_time_start)
{
    U32 time_dur_min;
    LONG64 secs;
    
    if(sys_time_cur.wYear < sys_time_start.wYear)
    {
        printf("error time!");
        DBG_Getch();
    }

    SystemTimeToFileTime(&sys_time_start, &t1.ft);
    SystemTimeToFileTime(&sys_time_cur, &t2.ft);

    secs=t2.val-t1.val;
    time_dur_min = (U32)(secs/10);

    return time_dur_min;


}

double computetime(SYSTEMTIME sys_start,SYSTEMTIME sys_end)
{
                int Hour, Minute, Second, Millisecond;
                double Result;

				Hour = sys_end.wHour - sys_start.wHour;
                if (Hour < 0)
                    Hour += 24;
				Minute = sys_end.wMinute - sys_start.wMinute;
                if (Minute < 0)
                {
                    Minute += 60;
                    Hour -= 1;
                }
				Second = sys_end.wSecond - sys_start.wSecond;
                if (Second < 0)
                {
                    Second += 60;
                    Minute -= 1;
                }
				Millisecond = sys_end.wMilliseconds - sys_start.wMilliseconds;
                if (Millisecond < 0)
                {
                    Millisecond += 1000;
                    Second -= 1;
                }
                Result = (double)Hour * (double)60 * (double)60 + (double)Minute * (double)60 + (double)Second + (double)Millisecond / (double)1000;
                return (Result);

           
}

void test_abnormal_power_down_rw(UINT32 times, UINT32 nDevice)
{
	BOOL rw_normal = TRUE;
	U32 loop_cnt = 0;
	U32 i = 0;

	U32 lba = 0;
	U16 xfer_cnt;
	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	U32 lba_start = 0;
	U32 lba_check_start,lba_check_end;
	U32 data_error_loop = 0;
	int k=0;

	SYSTEMTIME sys_start,sys_end;
	int power_starttime,power_endtime,durningtime;
	double durningtime_true, durningtime_total;
	FILE *data_error_log_detail,*data_error_log;

#ifdef WITHJTAG
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\data_error_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}

	if((data_error_log_detail = fopen(".\\output\\data_error_log_detail.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_detail!"); 
			system("pause");
			exit(1);
	}


	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 1000)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;
		//xfer_cnt = 0x1000;    // 0x100 = 128K

		
		durningtime = (rand()%10)+1;
		durningtime_total = (double)durningtime;
		//power_endtime = (power_starttime + durningtime) % 60;

		//printf("durning time is %d!\n",durningtime);
		printf("durningtime_total is %f\n",durningtime_total);


		xfer_cnt = ((rand() % 252) + 4) * 2;
		
		if (lba_start + xfer_cnt > syscfg_max_lba)
			lba_start = 0;


		for (lba = lba_start; lba< syscfg_max_lba ;lba += xfer_cnt)
		{
			
			GetLocalTime( &sys_start );
			//lba = (rand()* rand()) % syscfg_max_lba;
			io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
			GetLocalTime( &sys_end );
			durningtime_true = computetime(sys_start,sys_end);
			//printf("durningtime_true is %f\n",durningtime_true);

			if(durningtime_true < (double)2)
				durningtime_total = durningtime_total - durningtime_true;
			//printf("durningtime_total is %f\n",durningtime_total);
			
			if(durningtime_total > (double)0)
			{
				xfer_cnt = ((rand() % 252) + 4) * 2;
			}
			else
			{
				
				break;
			}
			k++;

		}  
		printf("write cmd is %d!\n",k);
		k=0;
		
		lba_check_start = lba_start;
		lba_check_end = lba;
		lba_start = lba;

		//disk_special_cmd(0xe7);   //flush disk cache
		//disk_special_cmd(0xe0);   //flush disk cache
		Sleep(1000);	//to confirm disk write done

		//let disk power down;
		
		jtag_power_control(TRUE, 1);

		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		Sleep(4000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 1000)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		GetLocalTime( &sys_start );
		jtag_power_control(FALSE, 1);
		
		//Sleep(10000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 1000)
		{
			Sleep(1000);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		GetLocalTime( &sys_end );
		durningtime_true = computetime(sys_start,sys_end);

		printf("from power up to identify device time is %f. \n",durningtime_true);
		/*lba = lba_check_start;
		xfer_cnt = 0x100;
		io_sync_request_execute(lba,xfer_cnt,OP_READ);
		GetLocalTime( &sys_end );
		durningtime_true = computetime(sys_start,sys_end);

		printf("from power up to finish read cmd time is %f. \n",durningtime_true);


		lba = lba_check_start;
		xfer_cnt = 0x100;
		io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
		GetLocalTime( &sys_end );
		durningtime_true = computetime(sys_start,sys_end);

		printf("from power up to finish write cmd time is %f. \n",durningtime_true);*/


		if(trim_turn_on == 1)
			io_specify_trim_request_random(g_test_disk_async);
		
		//Sleep(4000);
		//read data from disk;
		xfer_cnt = 0x100;
		for (lba = lba_check_start; lba <= lba_check_end ;lba += 0x100)
			io_sync_request_execute(lba,xfer_cnt,OP_READ);

		// check data;
		for( i = 0; i < MAX_LBA_WITH_DATA; i++ )

		{
			if(Error_lba[i] != 0)
			{
				fprintf(data_error_log_detail,"LBA %d data check error %d Dword\n",i,Error_lba[i]);
				data_error_loop += Error_lba[i];
				Error_lba[i] = 0;
			}
		}
		fprintf(data_error_log,"loop %d found data error %d Dword\n",loop_cnt,data_error_loop);
		data_error_loop = 0;
		DBG_printf("loop %d pass\n",loop_cnt);

	}
	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

	/*for( i = 0; i < MAX_LBA_WITH_DATA; i++ )
	{
		if(Error_lba_total[i] != 0)
			data_error_loop += Error_lba_total[i];
	}*/
	fprintf(data_error_log,"Total error %d sector\n",Error_lba_cnt);
	fclose(data_error_log);
	fclose(data_error_log_detail);

#endif
	return;
}
void test_abnormal_power_down_trim_special(UINT32 times, UINT32 nDevice)
{
	BOOL rw_normal = TRUE;
	U32 loop_cnt = 0;
	U32 i = 0;
	U32 lba = 0;
	U16 xfer_cnt;
	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	U32 lba_start = 0;
	U32 lba_check_start,lba_check_end;
	U32 data_error_loop = 0;
	int k=0;
	SYSTEMTIME sys_start,sys_end;
	int power_starttime,power_endtime,durningtime;
	double durningtime_true, durningtime_total;
	FILE *data_error_log_detail,*data_error_log;
#ifdef WITHJTAG
	if(_access(".\\output", 0)!=0) system("md .\\output");
	if((data_error_log = fopen(".\\output\\data_error_log.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log!"); 
			system("pause");
			exit(1);
	}
	if((data_error_log_detail = fopen(".\\output\\data_error_log_detail.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_detail!"); 
			system("pause");
			exit(1);
	}
	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}
	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 1000)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;
	}
	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}
	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		xfer_cnt = 0x100;
		if (lba_start + xfer_cnt > syscfg_max_lba)
			lba_start = 0;
		for (lba = lba_start; lba< syscfg_max_lba ;lba += xfer_cnt)
		{
			io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
			if(lba + xfer_cnt >= syscfg_max_lba)
				break;
		}  
		Sleep(1000);	//to confirm disk write done
		io_specify_trim_request_max(g_test_disk_async,0);
		io_specify_trim_request_max(g_test_disk_async,33553920);
		jtag_power_control(TRUE, 1);
		io_async_close();
		Sleep(4000);
		disk_scan_changes();
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 1000)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
		GetLocalTime( &sys_start );
		jtag_power_control(FALSE, 1);
		DBG_printf("Sleep done!\n");
		disk_scan_changes();
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 1000)
		{
			Sleep(1000);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}
		disk_init_testdisk(i - 1);
		io_async_init();
		GetLocalTime( &sys_end );
		durningtime_true = computetime(sys_start,sys_end);

		printf("from power up to identify device time is %f. \n",durningtime_true);
		xfer_cnt = 0x100;
		for (lba = 0x0; lba <= 67108864 ;lba += 0x100)
			io_sync_request_execute(lba,xfer_cnt,OP_READ);
		//fflush(filelog);
		getch();
	}
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();
	fprintf(data_error_log,"Total error %d sector\n",Error_lba_cnt);
	fclose(data_error_log);
	fclose(data_error_log_detail);
#endif
	return;
}
void test_trim_time_special(UINT32 nDevice)
{
	U32 lba = 0;
	U32 sector_cnt = 1;
	U32 lba_start = 0;
	U16 xfer_cnt;
	U32 i;
	xfer_cnt = 0x100;
	if (lba_start + xfer_cnt > syscfg_max_lba)
		lba_start = 0;
	for (lba = lba_start; lba< syscfg_max_lba ;lba += xfer_cnt)
	{
		io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
		if(lba + xfer_cnt >= syscfg_max_lba)
			break;
	}  
	lba = 0;
	for(i = 1; i <= 8; i++)
	{
		io_specify_trim_request_sector_time(g_test_disk_async,lba,sector_cnt);
		lba += sector_cnt * 16 * 1024 * 1024 * 2;
		sector_cnt++;
		if( lba + sector_cnt * 16 * 1024 * 1024 * 2 >= syscfg_max_lba)
		{
			for (lba = lba_start; lba< syscfg_max_lba ;lba += xfer_cnt)
			{
				io_sync_request_execute(lba,xfer_cnt,OP_WRITE);
				if(lba + xfer_cnt >= syscfg_max_lba)
					break;
			}  
			lba = 0;
		}
	}

	return;
}

void test_normal_power_down_ACPI(UINT32 times, UINT32 nDevice)
{
	BOOL rw_normal = TRUE;
	U32 loop_cnt = 0;
	U32 i = 0;

	
	
	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	

	U32 pattern_def;

#ifdef WITHJTAG
	
	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 100)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;
		//xfer_cnt = 0x1000;    // 0x100 = 128K

		test_do_one_file_pattern("..\\acpi pattern\\Rebootbefore_anvila.txt");

		

		//disk_special_cmd(0xe7);   //flush disk cache
		disk_special_cmd(0xe0);   //flush disk cache
		
		Sleep(1000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();
		

		// reenumerate device because sometime the os don't refresh device list
		//Sleep(1000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		

		//Sleep(1500);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;
		if(trim_turn_on == 1)
			io_specify_trim_request(g_test_disk_async);

		test_do_one_file_pattern("..\\acpi pattern\\Rebootafter_anvila.txt");


		// check data;


		DBG_printf("Reboot loop %d pass\n",loop_cnt);

	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;

		//xfer_cnt = 0x1000;    // 0x100 = 128K

		test_do_one_file_pattern("..\\acpi pattern\\S5before_anvila.txt");

		

		//disk_special_cmd(0xe7);   //flush disk cache
		disk_special_cmd(0xe0);   //flush disk cache
		Sleep(1000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		//Sleep(4000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		//Sleep(1000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;
		if(trim_turn_on == 1)
			io_specify_trim_request(g_test_disk_async);

		test_do_one_file_pattern("..\\acpi pattern\\S5after_anvila.txt");


		// check data;


		DBG_printf("S5 loop %d pass\n",loop_cnt);

	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;
		//xfer_cnt = 0x1000;    // 0x100 = 128K

		test_do_one_file_pattern("..\\acpi pattern\\S3before_anvila.txt");

		

		//disk_special_cmd(0xe7);   //flush disk cache
		disk_special_cmd(0xe0);   //flush disk cache
		Sleep(1000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		//Sleep(4000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		//Sleep(1000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;

		if(trim_turn_on == 1)
			io_specify_trim_request(g_test_disk_async);

		test_do_one_file_pattern("..\\acpi pattern\\S3after_anvila.txt");


		// check data;


		DBG_printf("S3 loop %d pass\n",loop_cnt);

	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;
		//xfer_cnt = 0x1000;    // 0x100 = 128K

		test_do_one_file_pattern("..\\acpi pattern\\S4before_anvila.txt");

		

		//disk_special_cmd(0xe7);   //flush disk cache
		disk_special_cmd(0xe0);   //flush disk cache
		Sleep(1000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		//Sleep(4000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		//Sleep(1000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;

		if(trim_turn_on == 1)
			io_specify_trim_request(g_test_disk_async);

		test_do_one_file_pattern("..\\acpi pattern\\S4after_anvila.txt");


		// check data;


		DBG_printf("S4 loop %d pass\n",loop_cnt);

	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{
		//write data to disk;
		//xfer_cnt = 0x1000;    // 0x100 = 128K

		pattern_def = rand() % 4;

		switch(pattern_def)
		{
			case 0:
				test_do_one_file_pattern("..\\acpi pattern\\S5before_anvila.txt");
				break;
			case 1:
				test_do_one_file_pattern("..\\acpi pattern\\S3before_anvila.txt");
				break;
			case 2:
				test_do_one_file_pattern("..\\acpi pattern\\S4before_anvila.txt");
				break;
			case 3:
				test_do_one_file_pattern("..\\acpi pattern\\Rebootbefore_anvila.txt");
				break;
			default:
				test_do_one_file_pattern("..\\acpi pattern\\S4before_anvila.txt");
				break;
		}

		//disk_special_cmd(0xe7);   //flush disk cache
		disk_special_cmd(0xe0);   //flush disk cache
		Sleep(1000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		//Sleep(4000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		//Sleep(1000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;

		switch(pattern_def)
		{
			case 0:
				if(trim_turn_on == 1)
					io_specify_trim_request(g_test_disk_async);
				test_do_one_file_pattern("..\\acpi pattern\\S5after_anvila.txt");
				break;
			case 1:
				if(trim_turn_on == 1)
					io_specify_trim_request(g_test_disk_async);
				test_do_one_file_pattern("..\\acpi pattern\\S3after_anvila.txt");
				break;
			case 2:
				if(trim_turn_on == 1)
					io_specify_trim_request(g_test_disk_async);
				test_do_one_file_pattern("..\\acpi pattern\\S4after_anvila.txt");
				break;
			case 3:
				if(trim_turn_on == 1)
					io_specify_trim_request(g_test_disk_async);
				test_do_one_file_pattern("..\\acpi pattern\\Rebootafter_anvila.txt");
				break;
			default:
				if(trim_turn_on == 1)
					io_specify_trim_request(g_test_disk_async);
				test_do_one_file_pattern("..\\acpi pattern\\S4after_anvila.txt");
				break;

		}



		// check data;


		DBG_printf("random ACPI loop %d pass\n",loop_cnt);

	}
	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

#endif
	
	return;
}

void CreateFile_1GB()
{
	FILE *createfile;
	int one_filesize,total_filesize,filesize,filenumber;
	int pattern_format;
	int i;
	char filename_t[256];
	U8 pattern_content;

	total_filesize = 1024 * 1024;
	filesize = 0;
	filenumber = 1;

	if(_access(TESTFILE_PATH, 0)!=0)
	{
		printf("Creating test file...");
		sprintf(filename_t,"md %s",TESTFILE_PATH);
		system(filename_t);
		while(filesize <= total_filesize)
		{
			one_filesize = (rand()%497) + 4;
			sprintf(filename_t,"%s\\%d_%d.bin",TESTFILE_PATH,one_filesize,filenumber);
			//createfile = fopen(filename_t,"wb");
			if((createfile = fopen(filename_t,"wb"))==NULL) 
			{ 
					printf("Cannot create file!"); 
					system("pause");
					exit(1);
			}
			pattern_format = rand()%3;
			switch(pattern_format)
			{
				case 0:
					pattern_content = 0xAA;
					for(i=0;i<one_filesize *1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 1:
					pattern_content = 0x55;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 2:
					for(i=0;i<one_filesize*1024;i++)
					{
						pattern_content = rand()%255;
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					}
					break;
				default:
					pattern_content = 0x00;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;

			}
			filesize += one_filesize;
			filenumber++;
			fclose(createfile);

		}
	}
	else
	{
		printf("Test file has been created!");
	}


}

void CreateFile_1GB_Trim()
{
	FILE *createfile;
	int one_filesize,total_filesize,filesize,filenumber;
	int pattern_format;
	int i,j;
	char filename_t[256];
	U8 pattern_content;

	total_filesize = 1024 * 1024;
	filesize = 0;
	filenumber = 1;
	j = 0;

	if(_access(TESTFILE_PATH_TRIM, 0)!=0)
	{
		printf("Creating test file...");
		sprintf(filename_t,"md %s",TESTFILE_PATH_TRIM);
		system(filename_t);
		while(filesize <= total_filesize)
		{
			if(j < 4)
			{
				one_filesize = 200 * 1024;
				j++;
			}
			else
			{
				one_filesize = (rand()%497) + 4;
			}
			sprintf(filename_t,"%s\\%d_%d.bin",TESTFILE_PATH_TRIM,one_filesize,filenumber);
			//createfile = fopen(filename_t,"wb");
			if((createfile = fopen(filename_t,"wb"))==NULL) 
			{ 
					printf("Cannot create file!"); 
					system("pause");
					exit(1);
			}
			pattern_format = rand()%3;
			switch(pattern_format)
			{
				case 0:
					pattern_content = 0xAA;
					for(i=0;i<one_filesize *1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 1:
					pattern_content = 0x55;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 2:
					for(i=0;i<one_filesize*1024;i++)
					{
						pattern_content = rand()%255;
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					}
					break;
				default:
					pattern_content = 0x00;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;

			}
			filesize += one_filesize;
			filenumber++;
			fclose(createfile);

		}
	}
	else
	{
		printf("Test file has been created!");
	}


}

void CreateFile_30MB_Adata()
{
	FILE *createfile;
	int one_filesize,total_filesize,filesize,filenumber;
	int pattern_format;
	int i,j;
	char filename_t[256];
	U8 pattern_content;

	total_filesize = 30 * 1024;
	filesize = 0;
	filenumber = 1;
	j = 0;

	if(_access(TESTFILE_PATH_TRIM_30, 0)!=0)
	{
		printf("Creating test file...");
		sprintf(filename_t,"md %s",TESTFILE_PATH_TRIM_30);
		system(filename_t);
		while(filesize <= total_filesize)
		{
			if(j < 10)
			{
				one_filesize = 3 * 1024;
				j++;
			}
			sprintf(filename_t,"%s\\%d_%d.bin",TESTFILE_PATH_TRIM_30,one_filesize,filenumber);
			//createfile = fopen(filename_t,"wb");
			if((createfile = fopen(filename_t,"wb"))==NULL) 
			{ 
					printf("Cannot create file!"); 
					system("pause");
					exit(1);
			}
			pattern_format = rand()%3;
			switch(pattern_format)
			{
				case 0:
					pattern_content = 0xAA;
					for(i=0;i<one_filesize *1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 1:
					pattern_content = 0x55;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 2:
					for(i=0;i<one_filesize*1024;i++)
					{
						pattern_content = rand()%255;
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					}
					break;
				default:
					pattern_content = 0x00;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;

			}
			filesize += one_filesize;
			filenumber++;
			fclose(createfile);

		}
	}
	else
	{
		printf("Test file has been created!");
	}


}

void CreateFile_80GB_Adata()
{
	FILE *createfile;
	int one_filesize,total_filesize,filesize,filenumber;
	int pattern_format;
	int i,j;
	char filename_t[256];
	U8 pattern_content;

	total_filesize = 80 * 1024 * 1024;
	filesize = 0;
	filenumber = 1;
	j = 0;

	if(_access(TESTFILE_PATH_TRIM_80, 0)!=0)
	{
		printf("Creating test file...");
		sprintf(filename_t,"md %s",TESTFILE_PATH_TRIM_80);
		system(filename_t);
		while(filesize <= total_filesize)
		{
			if(j < 79)
			{
				one_filesize = 1024 * 1024;
				j++;
			}
			sprintf(filename_t,"%s\\%d_%d.bin",TESTFILE_PATH_TRIM_80,one_filesize,filenumber);
			//createfile = fopen(filename_t,"wb");
			if((createfile = fopen(filename_t,"wb"))==NULL) 
			{ 
					printf("Cannot create file!"); 
					system("pause");
					exit(1);
			}
			pattern_format = rand()%3;
			switch(pattern_format)
			{
				case 0:
					pattern_content = 0xAA;
					for(i=0;i<one_filesize *1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 1:
					pattern_content = 0x55;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;
				case 2:
					for(i=0;i<one_filesize*1024;i++)
					{
						pattern_content = rand()%255;
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					}
					break;
				default:
					pattern_content = 0x00;
					for(i=0;i<one_filesize*1024;i++)
						fwrite(&pattern_content,sizeof(pattern_content),1,createfile);
					break;

			}
			filesize += one_filesize;
			filenumber++;
			fclose(createfile);

		}
	}
	else
	{
		printf("Test file has been created!");
	}


}

void CompareFile(int loop_cnt,char disksymble_des)
{
	FILE *FC_log,*Findstr_log,*Compare_log;
	char filename_t[256];
	int filesize;
	struct stat sb;
	
	if(_access("C:\\output", 0)!=0) system("md C:\\output");
	

	sprintf(filename_t,"C:\\output\\FC_log.txt");
	if((FC_log = fopen(filename_t,"wt"))==NULL) 
	{ 
			printf("Cannot create file FC_log!"); 
			system("pause");
			exit(1);
	}
	sprintf(filename_t,"C:\\output\\Findstr_log.txt");
	if((Findstr_log = fopen(filename_t,"wt"))==NULL) 
	{ 
			printf("Cannot create file \Findstr_log!"); 
			system("pause");
			exit(1);
	}
	

	fclose(FC_log);
	fclose(Findstr_log);

	sprintf(filename_t,"fc /b %s\\*.* %c:\\\\copy_%d\\*.*>C:\\output\\FC_log.txt",TESTFILE_PATH,disksymble_des,loop_cnt);
	printf("loop %d now.",loop_cnt);
	printf("Checking Data...");
	
	system(filename_t);

	sprintf(filename_t,"findstr /b 000 C:\\output\\FC_log.txt>C:\\output\\Findstr_log.txt");
	system(filename_t);

	sprintf(filename_t,"C:\\output\\Findstr_log.txt");
	stat(filename_t, &sb); 
	//Findstr_log = fopen(".\\output\\Findstr_log.txt","rt");

	//filesize = GetFileSize(Findstr_log,NULL);
	filesize = sb.st_size;

	if(filesize != 0)
	{
		sprintf(filename_t,"C:\\output\\Compare_log.txt");
		if((Compare_log = fopen(filename_t,"at"))==NULL) 
		{ 
				printf("Cannot create file Compare_log!"); 
				system("pause");
				exit(1);
		}
		fprintf(Compare_log,"This is the loop %d.\n",loop_cnt);
		fprintf(Compare_log,"There is data compare error.\n");
		fclose(Compare_log);
		sprintf(filename_t,"copy C:\\output\\Compare_log.txt+C:\\output\\FC_log.txt C:\\output\\Compare_log.txt");
		system(filename_t);
	}
	//fclose(FC_log);
	//fclose(Findstr_log);
	//fclose(Compare_log);
	
}

void test_abnormal_power_down_filecopy_trim(UINT32 times, UINT32 nDevice)
{
	U32 loop_cnt = 0;
	U32 i = 0;
	U32 q = 0;

	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	
	U32 file_copy_size,file_check_size,file_check_size_total;
	

	char file_name_src[256],filename_dsp[256],cmd[600];
	char disksymble;
	U32 file_cnt = 0;

	SYSTEMTIME sys_start,sys_end;
	int power_starttime,power_endtime,durningtime;
	double durningtime_true, durningtime_total;

	FILE *data_error_log_detail,*data_error_log;

#ifdef WITHJTAG
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\data_error_log_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_FC!"); 
			system("pause");
			exit(1);
	}

	if((data_error_log_detail = fopen(".\\output\\data_error_log_detail_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_detail_FC!"); 
			system("pause");
			exit(1);
	}


	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 100)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	printf("Please input the disk symble which you want to run test on it:\n");
	disksymble = getche();
	//scanf("%s",&disksymble);

	sprintf(cmd,"chkdsk /x %c:",disksymble);
	system(cmd);
	

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{

		

		file_copy_size = 0;
		file_check_size_total = 1024 *1024;

		sprintf(filename_dsp,"%c:\\\\copy_%d",disksymble,loop_cnt);
		if(_access(filename_dsp,0) != 0)
		{
			sprintf(cmd,"md %s",filename_dsp);
			system(cmd);
		}


		sprintf(file_name_src, "%s\\*.*", TESTFILE_PATH_TRIM);
		sprintf(cmd,"xcopy %s %s",file_name_src,filename_dsp);
		system(cmd);
		
		DBG_printf("loop %d now\n",loop_cnt);

		//Sleep(2000);

		

		//disk_special_cmd(0xe7);   //flush disk cache
		//disk_special_cmd(0xe0);   //flush disk cache
		//Sleep(2000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		Sleep(2000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		GetLocalTime( &sys_start );
		jtag_power_control(FALSE, 1);
		
		//Sleep(10000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		GetLocalTime( &sys_end );
		durningtime_true = computetime(sys_start,sys_end);

		printf("from power up to identify device time is %f. \n",durningtime_true);
		
		Sleep(2000);
		//read data from disk;

		sprintf(cmd,"chkdsk /x %c:",disksymble);
		system(cmd);

		//CompareFile(loop_cnt,disksymble);

		sprintf(filename_dsp,"%c:\\\\copy_%d\\*.*",disksymble,loop_cnt);
		hFind =  FindFirstFile(filename_dsp, &FindFileData);

		if(hFind == INVALID_HANDLE_VALUE)
		{
			printf("Pattern path check, find no file!\n");
			//getchar();
		}
		bFinish = FALSE;

		sprintf(filename_dsp,"%c:\\\\copy_%d",disksymble,loop_cnt);

		while(1)
		{
			if(bFinish == TRUE)
			{
				*file_name_src = '!';
				break;
			}
			
			if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
			{
				sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
				break;
			}
			else
			{
				*file_name_src = '@';
			}

			if(!FindNextFile(hFind, &FindFileData))  
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES)  
				{ 
					bFinish = TRUE;  
				}  
				else  
				{ 
					printf("File Find Error!\n");
					//getchar();
					break;
				}  
			} 

			if(*file_name_src != '@')
				break;
		}

		// check data;
		fprintf(data_error_log_detail,"Loop %d error log:\n",loop_cnt);
		while(file_name_src[0] != '!')
		{
			if(loop_cnt <= 9)
				q = 11;
			else if(loop_cnt <= 99)
				q = 12;
			else if(loop_cnt <= 999)
				q = 13;
			for(i=q;i<100;i++)
			{
				if(file_name_src[i] == '_')
					break;
			}
			if(q == 12)
				i = i - 1;
			else if(q == 13)
				i = i - 2;
			switch(i)
			{
				case 12:
				{
					file_check_size = (file_name_src[q] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 13:
				{
					file_check_size = (file_name_src[q] - '0') * 10 + (file_name_src[q+1] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 14:
				{
					file_check_size = (file_name_src[q] - '0') * 100 + (file_name_src[q+1] - '0') * 10 + (file_name_src[q+2] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 17:
				{
					file_check_size = (file_name_src[q] - '0') * 100000 + (file_name_src[q+1] - '0') * 10000 + (file_name_src[q+2] - '0') * 1000 + (file_name_src[q+3] - '0') * 100 + (file_name_src[q+4] - '0') * 10 + (file_name_src[q+5] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				default:
				{
					printf("data check error\n");
					//getchar();
					break;
				}
			}
			while(1)
			{
				
				if(!FindNextFile(hFind, &FindFileData))  
				{ 
					if (GetLastError() == ERROR_NO_MORE_FILES)  
					{ 
						bFinish = TRUE;  
					}  
					else  
					{ 
						printf("File Find Error!\n");
						//getchar();
						break;
					}  
				} 
				if(bFinish == TRUE)
				{
					*file_name_src = '!';
					break;
				}
				
				if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
				{
					sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
					break;
				}
				else
				{
					*file_name_src = '@';
				}

				

				if(*file_name_src != '@')
					break;
			}
			
			break;		
		}
		fprintf(data_error_log,"loop %d error data size is %d KB.\n",loop_cnt,(file_copy_size - file_check_size_total));
		
		//CloseHandle(hFind);
		//Sleep(2000);
		
		sprintf(cmd,"del /s /q %c:\\copy_%d\\*.*",disksymble,loop_cnt);
		system(cmd);

		//sprintf(cmd,"rd /s /q %c:\\copy_%d",disksymble,loop_cnt);
		//system(cmd);

		//sprintf(cmd,"rd /s /q %c:\\copy_%d",disksymble,loop_cnt);
		//system(cmd);

		Sleep(1000);

		DBG_printf("loop %d pass\n",loop_cnt);

	}
	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

#endif

	//fclose(data_error_log);
	//fclose(data_error_log_detail);

	return;
}


void test_abnormal_power_down_filecopy_trim_Adata(UINT32 times, UINT32 nDevice)
{
	U32 loop_cnt = 0;
	U32 i = 0;
	U32 q = 0;

	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	
	U32 file_copy_size,file_check_size,file_check_size_total;
	int copyfile_cnt;
	

	char file_name_src[256],filename_dsp[256],cmd[600];
	char disksymble;
	U32 file_cnt = 0;

	FILE *data_error_log_detail,*data_error_log;

#ifdef WITHJTAG
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\data_error_log_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_FC!"); 
			system("pause");
			exit(1);
	}

	if((data_error_log_detail = fopen(".\\output\\data_error_log_detail_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_detail_FC!"); 
			system("pause");
			exit(1);
	}


	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 100)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	printf("Please input the disk symble which you want to run test on it:\n");
	disksymble = getche();
	//scanf("%s",&disksymble);

	sprintf(cmd,"chkdsk /x %c:",disksymble);
	system(cmd);

	sprintf(filename_dsp,"%c:\\\\test_folder_80G",disksymble);

	if(_access(filename_dsp, 0)!=0)
	{
		sprintf(file_name_src, "%s\\*.*", TESTFILE_PATH_TRIM_80);
		sprintf(cmd,"xcopy %s %s",file_name_src,filename_dsp);
		system(cmd);
	}
	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{

		

		file_copy_size = 0;
		file_check_size_total = 1024 *1024;

		sprintf(filename_dsp,"%c:\\\\copy_%d",disksymble,loop_cnt);
		if(_access(filename_dsp,0) != 0)
		{
			sprintf(cmd,"md %s",filename_dsp);
			system(cmd);
		}

		copyfile_cnt = rand()%10 + 1;
		sprintf(file_name_src, "%s\\%d\\*.*", TESTFILE_PATH_TRIM_30,copyfile_cnt);
		sprintf(cmd,"xcopy %s %s",file_name_src,filename_dsp);
		system(cmd);
		
		DBG_printf("loop %d now\n",loop_cnt);

		//Sleep(2000);

		

		//disk_special_cmd(0xe7);   //flush disk cache
		//disk_special_cmd(0xe0);   //flush disk cache
		//Sleep(2000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		Sleep(2000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		//Sleep(10000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(2000);
		//read data from disk;

		sprintf(cmd,"chkdsk /x %c:",disksymble);
		system(cmd);

		//CompareFile(loop_cnt,disksymble);

		sprintf(filename_dsp,"%c:\\\\copy_%d\\*.*",disksymble,loop_cnt);
		hFind =  FindFirstFile(filename_dsp, &FindFileData);

		if(hFind == INVALID_HANDLE_VALUE)
		{
			printf("Pattern path check, find no file!\n");
			//getchar();
		}
		bFinish = FALSE;

		sprintf(filename_dsp,"%c:\\\\copy_%d",disksymble,loop_cnt);

		while(1)
		{
			if(bFinish == TRUE)
			{
				*file_name_src = '!';
				break;
			}
			
			if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
			{
				sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
				break;
			}
			else
			{
				*file_name_src = '@';
			}

			if(!FindNextFile(hFind, &FindFileData))  
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES)  
				{ 
					bFinish = TRUE;  
				}  
				else  
				{ 
					printf("File Find Error!\n");
					//getchar();
				}  
			} 

			if(*file_name_src != '@')
				break;
		}

		// check data;
		fprintf(data_error_log_detail,"Loop %d error log:\n",loop_cnt);
		/*while(file_name_src[0] != '!')
		{
			if(loop_cnt <= 9)
				q = 11;
			else if(loop_cnt <= 99)
				q = 12;
			else if(loop_cnt <= 999)
				q = 13;
			for(i=q;i<100;i++)
			{
				if(file_name_src[i] == '_')
					break;
			}
			if(q == 12)
				i = i - 1;
			else if(q == 13)
				i = i - 2;
			switch(i)
			{
				case 12:
				{
					file_check_size = (file_name_src[q] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 13:
				{
					file_check_size = (file_name_src[q] - '0') * 10 + (file_name_src[q+1] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 14:
				{
					file_check_size = (file_name_src[q] - '0') * 100 + (file_name_src[q+1] - '0') * 10 + (file_name_src[q+2] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 17:
				{
					file_check_size = (file_name_src[q] - '0') * 100000 + (file_name_src[q+1] - '0') * 10000 + (file_name_src[q+2] - '0') * 1000 + (file_name_src[q+3] - '0') * 100 + (file_name_src[q+4] - '0') * 10 + (file_name_src[q+5] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				default:
				{
					printf("data check error\n");
					//getchar();
					break;
				}
			}
			while(1)
			{
				
				if(!FindNextFile(hFind, &FindFileData))  
				{ 
					if (GetLastError() == ERROR_NO_MORE_FILES)  
					{ 
						bFinish = TRUE;  
					}  
					else  
					{ 
						printf("File Find Error!\n");
						getchar();
					}  
				} 
				if(bFinish == TRUE)
				{
					*file_name_src = '!';
					break;
				}
				
				if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
				{
					sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
					break;
				}
				else
				{
					*file_name_src = '@';
				}

				

				if(*file_name_src != '@')
					break;
			}
					
		}
		fprintf(data_error_log,"loop %d error data size is %d KB.\n",loop_cnt,(file_copy_size - file_check_size_total));*/
		
		//CloseHandle(hFind);
		//Sleep(2000);
		
		//sprintf(cmd,"del /s /q %c:\\copy_%d\\*.*",disksymble,loop_cnt);
		//system(cmd);

		//sprintf(cmd,"rd /s /q %c:\\copy_%d",disksymble,loop_cnt);
		//system(cmd);

		//sprintf(cmd,"rd /s /q %c:\\copy_%d",disksymble,loop_cnt);
		//system(cmd);

		Sleep(1000);

		DBG_printf("loop %d pass\n",loop_cnt);

	}
	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

#endif

	//fclose(data_error_log);
	//fclose(data_error_log_detail);

	return;
}

void Copy_File(char Disk_Dest,int loop_cnt,int stopfile)
{
	U8 Buffer[BUFFERSIZE];
	U32 haswrite = 1,hasread = 1;

	HANDLE file_handle_src,file_handle_dest;

	char filename_dsp[256];

    int File_Num;
    int i = 0;


	char file_name_src[923][80],file_name_dsp[923][80],cmd[600];

	


	/*SYSTEMTIME sys_start,sys_end;
	int power_starttime,power_endtime,durningtime;
	double durningtime_true, durningtime_total;*/

	sprintf(file_name_src[i], "%s\\*.*", TESTFILE_PATH_TRIM_30);
	
	bFinish = FALSE;
	hFind =  FindFirstFile(file_name_src[i], &FindFileData);
	
	//GetLocalTime( &sys_start );
		
	if(hFind == INVALID_HANDLE_VALUE)
	{
		printf("Pattern path check, find no file!\n");
		getchar();
	}
	sprintf(file_name_dsp[i], "%c:\\Copy_%d\\%s", Disk_Dest,loop_cnt,FindFileData.cFileName);
	while(1)
	{
		if(!FindNextFile(hFind, &FindFileData))  
		{ 
			if (GetLastError() == ERROR_NO_MORE_FILES)  
			{ 
				bFinish = TRUE;  
			}  
			else  
			{ 
				printf("File Find Error!\n");
				getchar();
			}  
		} 
		if(bFinish == TRUE)
		{
			break;
		}
		
		if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
		{
			sprintf(file_name_src[i], "%s\\%s", TESTFILE_PATH_TRIM,FindFileData.cFileName);
			sprintf(file_name_dsp[i], "%c:\\Copy_%d\\%s", Disk_Dest,loop_cnt,FindFileData.cFileName);
			i++;
		}
		else
		{
			file_name_src[i][0] = '@';
		}

		
	}

	

	sprintf(filename_dsp,"%c:\\\\copy_%d",Disk_Dest,loop_cnt);
	if(_access(filename_dsp,0) != 0)
	{
		sprintf(cmd,"md %s",filename_dsp);
		system(cmd);
	}

	//GetLocalTime( &sys_end );
	//	durningtime_true = computetime(sys_start,sys_end);


    for (File_Num = 0; File_Num < 923; File_Num++)
    {

		printf("Copy file %s to %s\n",file_name_src[File_Num],file_name_dsp[File_Num]);
        file_handle_src = CreateFile(file_name_src[File_Num], GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        file_handle_dest = CreateFile(file_name_dsp[File_Num], GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH , 0);
        //Lenth_dest = Lenth_src = 1;
		SetTimeStart = 1;
        while (1)
        {
			if(PowerOn == 1)
			{
				ReadFile(file_handle_src, (LPVOID)Buffer, BUFFERSIZE, &hasread, 0);
				if (hasread != 16384)
				{
					WriteFile(file_handle_dest, (LPVOID)Buffer, hasread, &haswrite, 0);
					break;
				}
				WriteFile(file_handle_dest, (LPVOID)Buffer, BUFFERSIZE, &haswrite, 0);
			}
			else
			{
				File_Num = 924;
				break;
			}
       
        }
        CloseHandle(file_handle_src);
        CloseHandle(file_handle_dest);
		if(File_Num == (stopfile - 1))
			break;


    }

}

unsigned __stdcall Timer()
{
	SYSTEMTIME sys_start,sys_end;
	double durningtime_true;

	while(1)
	{
		if(SetTimeStart == 1)
		{
			GetLocalTime( &sys_start );
			Sleep(10000);
			GetLocalTime( &sys_end );
			durningtime_true = computetime(sys_start,sys_end);
			while(durningtime_true < 0.300)
			{
				GetLocalTime( &sys_end );
				durningtime_true = computetime(sys_start,sys_end);
			}
			jtag_power_control(TRUE, 1);
			io_async_close();
			PowerOn = 0;
			SetTimeStart = 0;
		}
		else
			continue;
	}
	return(FALSE);
}

void test_abnormal_power_down_filecopy_trim_Adata_API(UINT32 times, UINT32 nDevice)
{
	U32 loop_cnt = 0;
	U32 i = 0;
	U32 q = 0;
	int stopfile;

	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	
	U32 file_copy_size,file_check_size,file_check_size_total;
	

	char file_name_src[256],filename_dsp[256],cmd[600];
	char disksymble;
	U32 file_cnt = 0;

	FILE *data_error_log_detail,*data_error_log;
	HANDLE Thread_time;

	

	SYSTEMTIME sys_start,sys_end;
	int power_starttime,power_endtime,durningtime;
	double durningtime_true, durningtime_total;

#ifdef WITHJTAG
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\data_error_log_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_FC!"); 
			system("pause");
			exit(1);
	}

	if((data_error_log_detail = fopen(".\\output\\data_error_log_detail_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_detail_FC!"); 
			system("pause");
			exit(1);
	}


	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 100)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	printf("Please input the disk symble which you want to run test on it:\n");
	disksymble = getche();
	//scanf("%s",&disksymble);

	sprintf(cmd,"chkdsk /x %c:",disksymble);
	system(cmd);

	PowerOn = 1;
	
	Thread_time = (HANDLE)_beginthreadex(NULL,0,Timer,NULL,0,NULL);
	//CloseHandle(Thread_time);

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{

		
		stopfile = 923;//rand()%10 + 1;

	
		ResumeThread(Thread_time);
		//GetLocalTime( &sys_start );
		//pthread_create(&t1,NULL,Copy_File(disksymble,loop_cnt,stopfile),NULL);
		Copy_File(disksymble,loop_cnt,stopfile);
		 //pthread_join(t1,NULL);
		//GetLocalTime( &sys_end );
		//durningtime_true = computetime(sys_start,sys_end);
		//Copy_File(disksymble,loop_cnt,stopfile);
		SuspendThread(Thread_time);
		DBG_printf("loop %d now\n",loop_cnt);

		//Sleep(2000);

		

		//disk_special_cmd(0xe7);   //flush disk cache
		//disk_special_cmd(0xe0);   //flush disk cache
		//Sleep(2000);	//to confirm disk write done

		//let disk power down;


		//jtag_power_control(TRUE, 1);
		//io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		Sleep(2000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		//Sleep(10000);			// wait for the disk ready
		//DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		PowerOn = 1;
		
		//Sleep(2000);
		//read data from disk;*/

		if(loop_cnt%50 == 0)
		{

			sprintf(cmd,"chkdsk /x %c:",disksymble);
			system(cmd);
		}

		//CompareFile(loop_cnt,disksymble);

		sprintf(filename_dsp,"%c:\\\\copy_%d\\*.*",disksymble,loop_cnt);
		hFind =  FindFirstFile(filename_dsp, &FindFileData);

		if(hFind == INVALID_HANDLE_VALUE)
		{
			printf("Pattern path check, find no file!\n");
			//getchar();
		}
		bFinish = FALSE;

		sprintf(filename_dsp,"%c:\\\\copy_%d",disksymble,loop_cnt);

		while(1)
		{
			if(bFinish == TRUE)
			{
				*file_name_src = '!';
				break;
			}
			
			if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
			{
				sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
				break;
			}
			else
			{
				*file_name_src = '@';
			}

			if(!FindNextFile(hFind, &FindFileData))  
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES)  
				{ 
					bFinish = TRUE;  
				}  
				else  
				{ 
					printf("File Find Error!\n");
					break;
					//getchar();
				}  
			} 

			if(*file_name_src != '@')
				break;
		}

		// check data;
		fprintf(data_error_log_detail,"Loop %d error log:\n",loop_cnt);
		while(file_name_src[0] != '!')
		{
			if(loop_cnt <= 9)
				q = 11;
			else if(loop_cnt <= 99)
				q = 12;
			else if(loop_cnt <= 999)
				q = 13;
			for(i=q;i<100;i++)
			{
				if(file_name_src[i] == '_')
					break;
			}
			if(q == 12)
				i = i - 1;
			else if(q == 13)
				i = i - 2;
			switch(i)
			{
				case 12:
				{
					file_check_size = (file_name_src[q] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 13:
				{
					file_check_size = (file_name_src[q] - '0') * 10 + (file_name_src[q+1] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 14:
				{
					file_check_size = (file_name_src[q] - '0') * 100 + (file_name_src[q+1] - '0') * 10 + (file_name_src[q+2] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 17:
				{
					file_check_size = (file_name_src[q] - '0') * 100000 + (file_name_src[q+1] - '0') * 10000 + (file_name_src[q+2] - '0') * 1000 + (file_name_src[q+3] - '0') * 100 + (file_name_src[q+4] - '0') * 10 + (file_name_src[q+5] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				default:
				{
					printf("data check error\n");
					//getchar();
					break;
				}
			}
			while(1)
			{
				
				if(!FindNextFile(hFind, &FindFileData))  
				{ 
					if (GetLastError() == ERROR_NO_MORE_FILES)  
					{ 
						bFinish = TRUE;  
					}  
					else  
					{ 
						printf("File Find Error!\n");
						file_name_src[0] = '!';
						break;
						//getchar();
					}  
				} 
				if(bFinish == TRUE)
				{
					*file_name_src = '!';
					break;
				}
				
				if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
				{
					sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
					break;
				}
				else
				{
					*file_name_src = '@';
				}

				

				if(*file_name_src != '@')
					break;
			}
					
		}
		//fprintf(data_error_log,"loop %d error data size is %d KB.\n",loop_cnt,(file_copy_size - file_check_size_total));
		
		//CloseHandle(hFind);
		//Sleep(2000);
		
		//sprintf(cmd,"del /s /q %c:\\copy_%d\\*.*",disksymble,loop_cnt);
		//system(cmd);

		sprintf(cmd,"rd /s /q %c:\\copy_%d",disksymble,loop_cnt);
		system(cmd);

		//sprintf(cmd,"rd /s /q %c:\\copy_%d",disksymble,loop_cnt);
		//system(cmd);

		Sleep(1000);

		DBG_printf("loop %d pass\n",loop_cnt);

	}

	_endthreadex(Thread_time);
	CloseHandle(Thread_time);

	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

#endif

	fclose(data_error_log);
	fclose(data_error_log_detail);

	return;
}


void test_abnormal_power_down_filecopy(UINT32 times, UINT32 nDevice)
{
	U32 loop_cnt = 0;
	U32 i = 0;

	U32 nLocalDevice = 0;
	U32 nDetectCnt = 0;
	
	U32 file_copy_size,file_check_size,file_check_size_total;
	

	char file_name_src[256],filename_dsp[256],cmd[600];
	char disksymble;
	U32 file_cnt = 0;

	SYSTEMTIME sys;
	int power_starttime,power_endtime,durningtime;
	FILE *data_error_log_detail,*data_error_log;

#ifdef WITHJTAG
	
	if(_access(".\\output", 0)!=0) system("md .\\output");

	if((data_error_log = fopen(".\\output\\data_error_log_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_FC!"); 
			system("pause");
			exit(1);
	}

	if((data_error_log_detail = fopen(".\\output\\data_error_log_detail_FC.txt","wt"))==NULL) 
	{ 
			printf("Cannot create file data_error_log_detail_FC!"); 
			system("pause");
			exit(1);
	}


	if (jtag_power_control_init())
	{
		DBG_printf("jtag init failure!");
		return;
	}

	Sleep(2000);	// to confirn the disk is ready
	disk_scan_changes();	// let os eenumerate the device list 
	nDetectCnt = 0;
	while (nLocalDevice == 0 && nDetectCnt < 100)
	{
		Sleep(100);
		nLocalDevice = disk_detect();
		nDetectCnt++;

	}

	if (nLocalDevice == 0)
	{
		DBG_printf("Device don't been detected!\n");
		return;
	}

	// init enviroment
	disk_init_testdisk(nLocalDevice - 1);
	io_async_init();
	

	if (0 == nLocalDevice)
	{
		DBG_printf("Device power up failure!\n");
		return;
	}

	printf("Please input the disk symble which you want to run test on it:\n");
	disksymble = getche();
	//scanf("%s",&disksymble);
	sprintf(filename_dsp,"%c:\\\\copy",disksymble);
	if(_access(filename_dsp,0) != 0)
	{
		sprintf(cmd,"md %s",filename_dsp);
		system(cmd);
	}

	for (loop_cnt = 0; loop_cnt < times; loop_cnt++)
	{

		GetLocalTime( &sys );

		power_starttime = sys.wSecond;
		durningtime = (rand()%10) + 1;
		power_endtime = (power_starttime + durningtime) % 60;

		

		FileNameGetInit();

		while(1)
		{
			if(bFinish == TRUE)
			{
				*file_name_src = '!';
				break;
			}
			
			if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
			{
				sprintf(file_name_src, "%s\\%s", TESTFILE_PATH,FindFileData.cFileName);
				break;
			}
			else
			{
				*file_name_src = '@';
			}

			if(!FindNextFile(hFind, &FindFileData))  
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES)  
				{ 
					bFinish = TRUE;  
				}  
				else  
				{ 
					printf("File Find Error!\n");
					getchar();
				}  
			} 

			if(*file_name_src != '@')
				break;
		}


		file_copy_size = 0;
		file_check_size_total = 0;

		while(file_name_src[0] != '!')
		{
			if(sys.wSecond != power_endtime)
			{
				sprintf(cmd,"xcopy %s /s %s",file_name_src,filename_dsp);
				system(cmd);
				file_copy_size += (int)FindFileData.nFileSizeLow/1024;
				while(1)
				{
					
					if(!FindNextFile(hFind, &FindFileData))  
					{ 
						if (GetLastError() == ERROR_NO_MORE_FILES)  
						{ 
							bFinish = TRUE;  
						}  
						else  
						{ 
							printf("File Find Error!\n");
							getchar();
						}  
					} 
					if(bFinish == TRUE)
					{
						*file_name_src = '!';
						break;
					}
					
					if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
					{
						sprintf(file_name_src, "%s\\%s", TESTFILE_PATH,FindFileData.cFileName);
						break;
					}
					else
					{
						*file_name_src = '@';
					}

					

					if(*file_name_src != '@')
						break;
				}
				file_cnt ++;
				GetLocalTime( &sys );
			}
			else
			{
				break;
			}
		}	

		

		//disk_special_cmd(0xe7);   //flush disk cache
		//disk_special_cmd(0xe0);   //flush disk cache
		//Sleep(2000);	//to confirm disk write done

		//let disk power down;
		jtag_power_control(TRUE, 1);
		io_async_close();

		// reenumerate device because sometime the os don't refresh device list
		Sleep(4000);
		disk_scan_changes();
		
		// re_detect disk 
		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 1 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 1)
		{
			DBG_printf("Device power down failure!\n");
			return;
		}
			
		//let disk power up;
		jtag_power_control(FALSE, 1);
		
		Sleep(10000);			// wait for the disk ready
		DBG_printf("Sleep done!\n");
		disk_scan_changes();

		i =  disk_detect();
		nDetectCnt = 0;
		while (nLocalDevice - i != 0 && nDetectCnt < 100)
		{
			Sleep(100);
			disk_scan_changes();
			i =  disk_detect();
			nDetectCnt++;
		}
		if (nLocalDevice - i != 0)
		{
			DBG_printf("Device power up failure!\n");
			return;
		}

		disk_init_testdisk(i - 1);
		io_async_init();
		
		//Sleep(4000);
		//read data from disk;
		sprintf(filename_dsp,"%c:\\\\copy\\*.*",disksymble);
		hFind =  FindFirstFile(filename_dsp, &FindFileData);

		if(hFind == INVALID_HANDLE_VALUE)
		{
			printf("Pattern path check, find no file!\n");
			getchar();
		}
		bFinish = FALSE;

		sprintf(filename_dsp,"%c:\\\\copy",disksymble);

		while(1)
		{
			if(bFinish == TRUE)
			{
				*file_name_src = '!';
				break;
			}
			
			if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
			{
				sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
				break;
			}
			else
			{
				*file_name_src = '@';
			}

			if(!FindNextFile(hFind, &FindFileData))  
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES)  
				{ 
					bFinish = TRUE;  
				}  
				else  
				{ 
					printf("File Find Error!\n");
					getchar();
				}  
			} 

			if(*file_name_src != '@')
				break;
		}

		// check data;
		fprintf(data_error_log_detail,"Loop %d error log:\n",loop_cnt);
		while(file_name_src[0] != '!')
		{
			for(i=9;i<15;i++)
			{
				if(file_name_src[i] == '_')
					break;
			}
			switch(i)
			{
				case 10:
				{
					file_check_size = (file_name_src[9] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 11:
				{
					file_check_size = (file_name_src[9] - '0') * 10 + (file_name_src[10] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				case 12:
				{
					file_check_size = (file_name_src[9] - '0') * 100 + (file_name_src[10] - '0') * 10 + (file_name_src[11] - '0');
					if(file_check_size == (FindFileData.nFileSizeLow/1024))
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					else
					{
						fprintf(data_error_log_detail,"%s file copy error,true file size is %d\n",FindFileData.cFileName,(FindFileData.nFileSizeLow/1024));
						file_check_size_total += (FindFileData.nFileSizeLow/1024);
					}
					break;
				}
				default:
				{
					printf("data check error\n");
					getchar();
					break;
				}
			}
			while(1)
			{
				
				if(!FindNextFile(hFind, &FindFileData))  
				{ 
					if (GetLastError() == ERROR_NO_MORE_FILES)  
					{ 
						bFinish = TRUE;  
					}  
					else  
					{ 
						printf("File Find Error!\n");
						getchar();
					}  
				} 
				if(bFinish == TRUE)
				{
					*file_name_src = '!';
					break;
				}
				
				if(!(FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes))
				{
					sprintf(file_name_src, "%s\\%s", filename_dsp,FindFileData.cFileName);
					break;
				}
				else
				{
					*file_name_src = '@';
				}

				

				if(*file_name_src != '@')
					break;
			}
					
		}
		//fprintf(data_error_log,"loop %d error data size is %d KB.\n",loop_cnt,(file_copy_size - file_check_size_total));
		
		
		sprintf(cmd,"rd/s/q %c:\\\\copy",disksymble);
		system(cmd);

		DBG_printf("loop %d pass\n",loop_cnt);

	}
	
	jtag_power_control(TRUE, 1);
	Sleep(4000);
	jtag_close();

	fclose(data_error_log);
	fclose(data_error_log_detail);

#endif

	return;
}



