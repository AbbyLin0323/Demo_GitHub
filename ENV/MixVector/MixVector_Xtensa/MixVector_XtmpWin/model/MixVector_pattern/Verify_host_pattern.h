/****************************************************************************
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
*****************************************************************************
Filename    :Verify_host_pattern.h
Version     :Ver 1.0
Author      :Brookewang
Date        :2012.07.11
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __VERIFY_HOST_PATTERN__
#define __VERIFY_HOST_PATTERN__


// pattern interface
BOOL sim_read_write_whole_disk(U8 opCode);
BOOL sim_mix_vector();
void mixv_host_main();
void sim_test_random_pattern(void);
void sim_test_random_pattern_1(void);
void sim_test_random_pattern_2(void);
void sim_test_random_pattern_3(void);
void sim_test_random_pattern_4(void);
void sim_test_random_pattern_5(void);
void sim_test_random_pattern_6(void);
void sim_test_random_pattern_7(void);
void sim_test_random_pattern_8(void);
void sim_test_random_pattern_9(void);
void sim_random_write_read_pattern(void);

BOOL sim_test_gc_pattern(void);
BOOL sim_test_gc_trim_pattern(void);
BOOL sim_test_file_pattern(char* file_name);
BOOL sim_test_file_trim_pattern(char* file_name);
BOOL sim_test_do_one_file_pattern(char * pfile_name);
void sim_system_schedule();
BOOL sim_test_normal_shutdown(void);
BOOL sim_test_normal_shutdown_gc(void);
BOOL sim_test_normal_shutdown_file_pattern(void);
BOOL sim_send_trim_cmd(void);
BOOL sim_test_table_rebuild_of_clear_dram(void);
BOOL sim_test_read_prefetch(void);
BOOL sim_abnormal_power_cycle_test_pattern();
BOOL sim_test_do_jedec_pattern();

extern void sim_test_seq_4K_pattern(U8 opCode);
extern void sim_test_rand_4K_pattern(U8 opCode);


extern U8 g_SimHostFunction;//sim_host_function_code
extern U8 g_SimHostFunction_TableRebuild;
extern U8 g_SimAbnormalPDFunction;

#endif
