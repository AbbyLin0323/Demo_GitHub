#ifndef _IO_ASYNC_H
#define _IO_ASYNC_H

enum
{
	ASYNC_IO_SUCCESS,
	ASYNC_IO_PENDING = 0x100,
	ASYNC_IO_PARTIAL,
	ASYNC_IO_NORESOURCE,
	ASYNC_IO_ERROR
};

#define  OP_WRITE 1
#define  OP_READ 0
#define  ASYNC_IO_POLLING_INTETVAL 100 //millsecond

GLOBAL U32 io_async_init();
GLOBAL U32 io_async_build_request(U32 start_lba,U16 xfer_cnt,U32 rw_flag);
GLOBAL U32 io_async_quene_request(U32 cmd_slot_index);
GLOBAL U32 io_async_status_get_byindex(U32 cmd_slot_index,BOOL block);
GLOBAL U32 io_async_status_get(BOOL block);
GLOBAL U32 io_async_close();
GLOBAL U32 io_async_request_empty();

GLOBAL U32 io_sync_request_execute(U32 start_lba,U16 xfer_cnt,U32 rw_flag);
GLOBAL U32 io_async_quene_finish(U32 cmd_slot_index);

#endif



