/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   hior.h                                         
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


#ifndef _HIOR_H
#define _HIOR_H

enum 
{
	HIO_FREE,
	HIO_PENDING,
	HIO_FINISH
};

enum 
{
	HIO_SUCCESS ,
	HIO_RESOUCE_PENDING = INVALID_2F,
	HIO_ERROR
};

typedef struct _HIOR
{
	OVERLAPPED hio_ov;
	LPVOID hip_buffer;
	
	U32  hio_req_lba;
	U32  hio_req_sec_cnt;
	BOOL   hio_req_type_rw;
	U32  hio_req_offset;

	U32  hio_status;
	U32  hio_req_prev;
	U32  hio_req_next;
}HIOR;


#define MAX_HIO_DEEP 16
#define MAX_SEC_CNT  512
#define MAX_HIO_REQ_LENGTH  (MAX_SEC_CNT*SEC_SZ)

void hior_init();
void hior_close();

U32  hior_cmdslot_getfree();
U32  hior_cmdslot_recyclefinish(U32 index);
U32  hior_cmdslot_finishpending(U32 index);
U32  hior_cmdslot_empty();
U32  hior_pendingslot_empty();

U32  hior_buffer_getbyindex(U32 index,LPVOID *buffer);
U32  hior_overlapped_getbyindex(U32 index,LPOVERLAPPED *p_ov);
U32  hior_cmdslot_parameter_setbyindex(U32 index,U32 req_lba,U16 req_sec,U32 hio_req_offset,BOOL req_rw);
U32  hior_cmdslot_parameter_getbyindex(U32 index,U32 *req_lba,U16 *req_sec,U32 *hio_req_offset,BOOL *req_rw);
U32  hior_eventgroup_get(HANDLE **event,U32* count);

U32  hior_list_check();
#define  HIO_DEBUG

#endif