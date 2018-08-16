/*************************************************
* Copyright (c) 2010 VIA Technologies, Inc. All Rights Reserved.
* 
* Information in this file is the U32ellectual property of
* VIA Technologies, Inc., and may contains trade secrets that must be stored 
* and viewed confidentially..
* 
* Filename     :   hior.c                                         
* Version      :   Ver 1.0                                               
* Date         :                                         
* Author       :   
* 
* Description: host io request managerment.
*              free list:free node list
*              pending list:pending io list
* 
* Depend file:
* 
* Export file:
* 
* Modification History:
* 20100112 first created
*************************************************/
#include "windows.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "defines.h"
#include "config.h"
#include "hior.h"

HIOR HIOR_ENTRY[MAX_HIO_DEEP];
HANDLE HIOR_EVENT[MAX_HIO_DEEP];
U32 HIOR_Free_Head;
U32 HIOR_Pending_Head;
CRITICAL_SECTION HIOR_CS;


void hior_init()
{
	U32 i;
	for (i = 0;i < MAX_HIO_DEEP;i++)
	{
		HIOR_EVENT[i] = CreateEvent(NULL,TRUE,FALSE,NULL);
		memset(&HIOR_ENTRY[i].hio_ov,0,sizeof(OVERLAPPED));
		HIOR_ENTRY[i].hio_ov.hEvent = HIOR_EVENT[i];
		HIOR_ENTRY[i].hio_req_lba = 0;
		HIOR_ENTRY[i].hio_req_sec_cnt = 0;
		HIOR_ENTRY[i].hio_req_type_rw = 0;
		HIOR_ENTRY[i].hio_status = HIO_FREE;
		HIOR_ENTRY[i].hip_buffer = VirtualAlloc(NULL,MAX_HIO_REQ_LENGTH,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
		memset(HIOR_ENTRY[i].hip_buffer,i,sizeof(i));

		if (i < MAX_HIO_DEEP-1)
			HIOR_ENTRY[i].hio_req_next = i + 1;
		else
			HIOR_ENTRY[i].hio_req_next = INVALID_2F;

		if (i > 0)
			HIOR_ENTRY[i].hio_req_prev = i - 1;
		else
			HIOR_ENTRY[i].hio_req_prev = INVALID_2F;
	}

	HIOR_Free_Head = 0;
	HIOR_Pending_Head = INVALID_2F;

	InitializeCriticalSection(&HIOR_CS);
}

void hior_close()
{
	U32 i;

	for (i = 0;i < MAX_HIO_DEEP;i++)
	{
		CloseHandle(HIOR_EVENT[i]);
		VirtualFree(HIOR_ENTRY[i].hip_buffer,0,MEM_RELEASE);
	}

	DeleteCriticalSection(&HIOR_CS);

}

U32 hior_cmdslot_getfree()
{
	U32 free_head;
	U32 pend_head;

	// no free node resource pending
	if (HIOR_Free_Head ==	INVALID_2F)
		return INVALID_2F;

	EnterCriticalSection(&HIOR_CS);

	//fetch form the free list
	free_head = HIOR_Free_Head;

#ifdef HIO_DEBUG
	if (HIOR_ENTRY[free_head].hio_status != HIO_FREE)
	{
		printf("status error index %x status %x\n",free_head,HIOR_ENTRY[free_head].hio_status);
		LeaveCriticalSection(&HIOR_CS);
		return HIO_ERROR;
	}
#endif

	HIOR_Free_Head = HIOR_ENTRY[free_head].hio_req_next;

	//add to pending list head
	HIOR_ENTRY[free_head].hio_status = HIO_PENDING;

	pend_head = HIOR_Pending_Head;

	HIOR_Pending_Head = free_head; 

	if (pend_head == INVALID_2F) //pending list is NULL
	{
		HIOR_ENTRY[free_head].hio_req_next = INVALID_2F;
		HIOR_ENTRY[free_head].hio_req_prev = INVALID_2F;
	}
	else
	{
		HIOR_ENTRY[free_head].hio_req_next = pend_head;
		HIOR_ENTRY[free_head].hio_req_prev = INVALID_2F;

		HIOR_ENTRY[pend_head].hio_req_prev = free_head;
	}

	LeaveCriticalSection(&HIOR_CS);

	return free_head;
}


U32 hior_cmdslot_recyclefinish(U32 index)
{
	U32 temp_index;
	U32 free_head;
	U32 pending_head;

#ifdef HIO_DEBUG

	if(index >= MAX_HIO_DEEP)
	{
		printf("index overflow \n");
		return HIO_ERROR;
	}

	if (HIOR_ENTRY[index].hio_status != HIO_FINISH)
	{
		printf("Release_Pending_HIQ_Entry status error index %x \n",index);
		return HIO_ERROR;
	}

#endif

	EnterCriticalSection(&HIOR_CS);

	pending_head = HIOR_Pending_Head;

	free_head = HIOR_Free_Head;

	HIOR_ENTRY[index].hio_status = HIO_FREE;

	if (HIOR_ENTRY[index].hio_req_prev == INVALID_2F)//pending node is head
	{
		temp_index = HIOR_ENTRY[index].hio_req_next;

		HIOR_Pending_Head = temp_index;

		if (temp_index != INVALID_2F)
			HIOR_ENTRY[temp_index].hio_req_prev = INVALID_2F;
	}
	else if (HIOR_ENTRY[index].hio_req_next == INVALID_2F)//pending node is tail
	{
		temp_index = HIOR_ENTRY[index].hio_req_prev;
		HIOR_ENTRY[temp_index].hio_req_next = INVALID_2F;
	}
	else//pending node is in middle
	{
		temp_index = HIOR_ENTRY[index].hio_req_next;
		HIOR_ENTRY[temp_index].hio_req_prev = HIOR_ENTRY[index].hio_req_prev;
		temp_index = HIOR_ENTRY[index].hio_req_prev;
		HIOR_ENTRY[temp_index].hio_req_next = HIOR_ENTRY[index].hio_req_next;
	}


	//add to free list head

	//free list is not empty
	if (free_head != INVALID_2F)
		HIOR_ENTRY[free_head].hio_req_prev = index;

	HIOR_ENTRY[index].hio_req_prev = INVALID_2F;
	HIOR_ENTRY[index].hio_req_next = free_head;

	HIOR_Free_Head = index;
	LeaveCriticalSection(&HIOR_CS);
	return HIO_SUCCESS;
}

U32  hior_pendingslot_empty()
{
	return (HIOR_Pending_Head == INVALID_2F)?1:0;
}

U32 hior_cmdslot_finishpending(U32 index)
{

#ifdef HIO_DEBUG
	if(index >= MAX_HIO_DEEP)
	{
		printf("index overflow \n");
		return HIO_ERROR;
	}

	if (HIOR_ENTRY[index].hio_status != HIO_PENDING)
	{
		printf("Release_Pending_HIQ_Entry status error index %x \n",index);
		return HIO_ERROR;
	}
#endif

	HIOR_ENTRY[index].hio_status = HIO_FINISH;

	return 0;
}

U32 hior_list_check()
{
#ifdef HIO_DEBUG
	U32 sum;

	U32 index;

	sum = 0;

	index = HIOR_Free_Head;

	while (index != INVALID_2F)
	{
		if (HIOR_ENTRY[index].hio_status != HIO_FREE )
		{
			printf("status error at %d\n",index);
		}
		printf("%d \t",index);

		index = HIOR_ENTRY[index].hio_req_next;
		sum ++;
	}

	printf("\n %d free HICO\n",sum);

	index = HIOR_Pending_Head;

	printf("pending node\n");
	while (index != INVALID_2F)
	{
		if (HIOR_ENTRY[index].hio_status != HIO_PENDING && HIOR_ENTRY[index].hio_status != HIO_FINISH)
		{
			printf("status error at %d\n",index);
		}
		printf("%d \t",index);

		index = HIOR_ENTRY[index].hio_req_next;
		sum ++;
	}

	if (sum != MAX_HIO_DEEP)
	{
		printf("node leak \n");
	}

	printf("\ntotal node %d\n",sum);
#endif
	return 0;
}

U32 hior_buffer_getbyindex(U32 index,LPVOID *buffer)
{

#ifdef HIO_DEBUG
	if(index >= MAX_HIO_DEEP)
	{
		printf("index overflow \n");
		return HIO_ERROR;
	}
#endif

	*buffer = HIOR_ENTRY[index].hip_buffer;

	return HIO_SUCCESS;
}


U32 hior_overlapped_getbyindex(U32 index,LPOVERLAPPED *p_ov)
{

#ifdef HIO_DEBUG
	if(index >= MAX_HIO_DEEP)
	{
		printf("index overflow \n");
		return HIO_ERROR;
	}
#endif

	*p_ov = &HIOR_ENTRY[index].hio_ov;

	return HIO_SUCCESS;
}

U32 hior_cmdslot_parameter_setbyindex(U32 index,U32 req_lba,U16 req_sec,U32 req_offset,BOOL req_rw)
{
	U32 act_lba;

#ifdef HIO_DEBUG
	if(index >= MAX_HIO_DEEP)
	{
		printf("index overflow \n");
		return HIO_ERROR;
	}
#endif

	HIOR_ENTRY[index].hio_ov.InternalHigh = 0;
	HIOR_ENTRY[index].hio_ov.Internal = 0;
	HIOR_ENTRY[index].hio_ov.Pointer = NULL;
	HIOR_ENTRY[index].hio_req_offset = req_offset;

	act_lba = req_lba + (req_offset >> SEC_BIT);

	HIOR_ENTRY[index].hio_ov.Offset = (act_lba << SEC_BIT)&~SEC_MSK;

	HIOR_ENTRY[index].hio_ov.OffsetHigh = (act_lba & 0xff800000)>>23;

	HIOR_ENTRY[index].hio_req_lba = req_lba;
	HIOR_ENTRY[index].hio_req_sec_cnt = req_sec;
	HIOR_ENTRY[index].hio_req_type_rw = req_rw;

	return HIO_SUCCESS;
}


U32 hior_cmdslot_parameter_getbyindex(U32 index,U32 *req_lba,U16 *req_sec,U32 *req_offset,BOOL *req_rw)
{
#ifdef HIO_DEBUG
	if(index >= MAX_HIO_DEEP)
	{
		printf("index overflow \n");
		return HIO_ERROR;
	}
#endif

	*req_lba = HIOR_ENTRY[index].hio_req_lba;
	*req_sec = HIOR_ENTRY[index].hio_req_sec_cnt;
	*req_rw  = HIOR_ENTRY[index].hio_req_type_rw;
	*req_offset = HIOR_ENTRY[index].hio_req_offset;

	/*
	HIOR_ENTRY[index].hio_ov.Offset = req_lba;

	HIOR_ENTRY[index].hio_req_lba = req_lba;
	HIOR_ENTRY[index].hio_req_sec_cnt = req_sec;
	HIOR_ENTRY[index].hio_req_type_rw = req_rw;
	*/
	return HIO_SUCCESS;
}

U32 hior_eventgroup_get(HANDLE **event,U32* count)
{
	*count = MAX_HIO_DEEP;
	*event = HIOR_EVENT;

	return HIO_SUCCESS;
}

#if 0

U32 main()
{
	U32 i;
	U32 index;

	U32 p,q;
	U32 times;

	HANDLE * e_group;
	U8 *buffer;
	U32 e_count;
	LPOVERLAPPED ov;
	HIOR_Init();
	srand( (unsigned)time( NULL ) );

	Get_HIOR_EventGroup(&e_group,&e_count);
	Get_HIOR_Buffer(1,&buffer);
	Get_HIOR_Ov(1,&ov);


	return 0;

	times = 100;
	while (times--)
	{
		p = rand()%MAX_HIO_DEEP;
		q = rand()%MAX_HIO_DEEP;

		for (i=0;i<p;i++)
		{
			index = Get_FreeHIQ_Entry();
			if(index == HIO_ERROR)
				printf("HIO_ERROR \n");
			else if( index == HIO_RESOUCE_PENDING)
				printf("no free node\n");
			else
				printf("get free hiq %d\n",index);
		}

		for (i=0;i<q;i++)
		{
			index = Finish_Pending_HIQ_Entry(i);
			index = Release_Pending_HIQ_Entry(i);

			if(index == HIO_ERROR)
				printf("HIO_ERROR \n");
			else
				printf("release pending hiq %d\n",i);
		}
	}

	Check_HIOR_List();


	for (i=0;i<32;i++)
	{
		index = Get_FreeHIQ_Entry();
		if(index == HIO_ERROR)
			printf("HIO_ERROR \n");
		else if( index == HIO_RESOUCE_PENDING)
			printf("no free node\n");
		else
			printf("get free hiq %d\n",index);
	}

	for (i=0;i<32;i++)
	{
		index = Finish_Pending_HIQ_Entry(i);
		index = Release_Pending_HIQ_Entry(i);

		if(index == HIO_ERROR)
			printf("HIO_ERROR \n");
		else
			printf("release pending hiq %d\n",i);
	}
	for (i=0;i<32;i++)
	{
		index = Get_FreeHIQ_Entry();
		if(index == HIO_ERROR)
			printf("HIO_ERROR \n");
		else if( index == HIO_RESOUCE_PENDING)
			printf("no free node\n");
		else
			printf("get free hiq %d\n",index);
	}

	for (i=0;i<32;i++)
	{
		index = Finish_Pending_HIQ_Entry(i);
		index = Release_Pending_HIQ_Entry(i);

		if(index == HIO_ERROR)
			printf("HIO_ERROR \n");
		else
			printf("release pending hiq %d\n",i);
	}
	Check_HIOR_List();

	HIOR_Close();

	return 0;
}

#endif
