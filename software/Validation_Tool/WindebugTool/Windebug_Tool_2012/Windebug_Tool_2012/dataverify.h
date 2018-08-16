#ifndef _H_DATA_VERIFY
#define _H_DATA_VERIFY

void dataverify_init();
void dataverify_set_data_before_write(U32 start_lba,U16 sec_cnt,U32* tgt_data_buf);
BOOL dataverify_do_verify_after_read(U32 start_lba,U16 sec_cnt,U32* tgt_data_buf);
BOOL dataverify_dump_data_to_disk();

BOOL dataverify_load_src_data_from_disk();
void dataverify_set(U8 bDumpFlag,U8 bCheckFlag);
#endif