#ifndef _H_PATTERN
#define  _H_PATTERN

void test_special_pattern_none_ncq(void);
void test_special_pattern_ncq(void);
void test_special_pattern_sync(void);
void test_special_pattern_sync_1();
void test_random_pattern(void);
void test_write_whole_disk_sync();
void test_read_whole_disk_sync();
void test_read_whole_disk_async();
void test_write_whole_disk_async();
void test_load_data_from_disk_for_compare(void);
void test_special_pattern(void);
void test_vendor_command(void);
void test_file_pattern(void);
void test_file_pattern_all(int i);
void test_save_data_to_disk();
void test_load_data_from_disk();
void test_power_down_rw(UINT32 times, UINT32 nDevice);

void test_abnormal_power_down_rw(UINT32 times, UINT32 nDevice);
void test_normal_power_down_ACPI(UINT32 times, UINT32 nDevice);
void CreateFile_1GB();
void test_abnormal_power_down_filecopy(UINT32 times, UINT32 nDevice);
void CreateFile_1GB_Trim();
void test_abnormal_power_down_filecopy_trim(UINT32 times, UINT32 nDevice);
void CompareFile(int loop_cnt,char disksymble_des);

void test_file_pattern_trim_flush(void);
void test_do_one_file_pattern_trim_flush(char * pfile_name,HANDLE disk);

void CreateFile_80GB_Adata(void);
void CreateFile_30MB_Adata(void);
void test_abnormal_power_down_filecopy_trim_Adata(UINT32 times, UINT32 nDevice);
void test_abnormal_power_down_filecopy_trim_Adata_API(UINT32 times, UINT32 nDevice);
void test_sequential_write_pattern(void);

void test_special_rw(void);

#endif