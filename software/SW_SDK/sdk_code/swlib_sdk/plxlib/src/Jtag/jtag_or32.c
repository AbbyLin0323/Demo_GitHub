

#include <windows.h>
#include "FTCJTAG.h"
#include "BaseDef.h"
#include "jtag_or32.h"
#include "stdafx.h"
extern unsigned int *jtag_calc_stream_crc(unsigned long stream, int len, unsigned int *p_crc_w);

#define MAX_DEVICE_CNT 16
static FTC_HANDLE ftHandle[MAX_DEVICE_CNT];
static DWORD dwLocationID[MAX_DEVICE_CNT];
static DWORD remap[MAX_DEVICE_CNT];
static DWORD dwNumDevices = 0, dwNumOR32s = 0;

/*
index: index of internal remap index
pidcode: pointer for storing id-code
return value:
	FTC_SUCCESS -- success
	FTC_INSUFFICIENT_RESOURCES -- pidcode is NULL
	other -- see FTCJTAG.h
*/
void ATPG_SCAN_CHAIN_NEW_File_Write(FILE *stream,unsigned char *AppWriteBuffer)
{
	
	int i,j;
	static unsigned int k=0;
	char Buffer[50];
	char temp_high,temp_low;

	
	Buffer[0]='0';
	Buffer[1]='x';
	Buffer[41]='\n';
    /* display the string */
    fopen_s(&stream, "scan_out.txt", "r+");
    fseek(stream, (k++) * 43, SEEK_SET);
	
	for(i=0,j=39;i<20;i++,j-=2)//将字符转换成数字
	{   
		if(i==19)
		{
			temp_low=*(AppWriteBuffer+i)&0x0f;
            if ((0 <= temp_low) && (temp_low <= 9))
			    Buffer[j+1]=temp_low+0x30;
            else if (0x0a <= temp_low&&temp_low <= 0x0f)
			    Buffer[j+1]=temp_low+0x57;
		}
		else
		{
		temp_high=(*(AppWriteBuffer+i)&0xf0)>>4;
		temp_low=*(AppWriteBuffer+i)&0x0f;
		
		if(0<=temp_high&&temp_high<=9)
			    Buffer[j]=temp_high+0x30;
		else if(0x0a<=temp_high&&temp_high<=0x0f)
				Buffer[j]=temp_high+0x57;

        if ((0 <= temp_low)&&(temp_low <= 9))
			    Buffer[j+1]=temp_low+0x30;
		else if(0x0a<=temp_low && temp_low<=0x0f)
				Buffer[j+1]=temp_low+0x57;
		}
	}
	fwrite(Buffer,1,42, stream);
}

static U32 jtag_idcode(unsigned int index, DWORD *pidcode)
{
	WriteDataByteBuffer WriteBuffer;
	ReadDataByteBuffer ReadBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;

	if (!pidcode)
		return FTC_INSUFFICIENT_RESOURCES;

	WriteBuffer[0] = 0x02;
	if ((Status = JTAG_Write(ftHandle[index], TRUE, 4, &WriteBuffer, 1, RUN_TEST_IDLE_STATE)) != FTC_SUCCESS)
		return Status;

	Status = JTAG_Read(ftHandle[index], FALSE, 32, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	memcpy(pidcode, ReadBuffer, sizeof(DWORD));
	return Status;
}

/*
index: 
	> 0 -- index of internal remap index
	-1 -- destroy all
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 jtag_destroy(U32 index)
{
	FTC_STATUS Status = FTC_SUCCESS, ret = FTC_SUCCESS;
	U32 i;

	if (index == -1)
		for (i = 0; i < dwNumDevices; i++)
			ret = ((Status = JTAG_Close(ftHandle[i])) != FTC_SUCCESS) ? Status : ret;
	else
		ret = JTAG_Close(ftHandle[index]);

	return ret;
}

/*
pnum: pointer for storing device count
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pnum is NULL
other -- see FTCJTAG.h
*/
static U32 jtag_init(U32 *pnum)
{
	char szDeviceName[128];
	DWORD i = 0, j = 0;
	FTC_STATUS Status = FTC_SUCCESS;
	jtag_destroy(-1);
	memset(ftHandle, 0x00, sizeof(ftHandle));
	memset(dwLocationID, 0x00, sizeof(dwLocationID));
	memset(remap, 0x00, sizeof(remap));
	dwNumDevices = dwNumOR32s = 0;

	if (!pnum)
		return FTC_INSUFFICIENT_RESOURCES;

	if ((Status = JTAG_GetNumDevices(&dwNumDevices)) != FTC_SUCCESS)
		return Status;



	for (i = 0; i < dwNumDevices; i++)
	{
		if ((Status = JTAG_GetDeviceNameLocID(i, szDeviceName, sizeof(szDeviceName) - 1, dwLocationID + i)) != FTC_SUCCESS)
		{
			jtag_destroy(-1);
			return Status;
		}
		if ((Status = JTAG_OpenEx(szDeviceName, dwLocationID[i], ftHandle + i)) != FTC_SUCCESS)
		{
			jtag_destroy(-1);
			return Status;
		}
		if ((Status = JTAG_InitDevice(ftHandle[i], 15)) != FTC_SUCCESS)
		{
			jtag_destroy(-1);
			return Status;
		}
		if (jtag_idcode(i, &j) == FTC_SUCCESS && (j == 0xdeadbeef))
			remap[dwNumOR32s++] = i;
	}

	*pnum = dwNumOR32s;
	return FTC_SUCCESS;
}





/*
index: index of internal remap index
chain: chain number
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
 U32 jtag_set_chain(unsigned int index, U32 chain)
{
	WriteDataByteBuffer WriteBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	unsigned int crc_w = 0;
	/* set chain cmd */    //chris: here is operating IR,0x03 is "CHAIN_SELECT" command
	WriteBuffer[0] = 0x03;
	//printf("write data [%2x] into IR\r\n",WriteBuffer[0]);
	if ((Status = JTAG_Write(ftHandle[remap[index]], TRUE/*TRUE means IR, FALSE means DR*/, 4, &WriteBuffer, 1, RUN_TEST_IDLE_STATE)) != FTC_SUCCESS)
		return Status;
	/* chain num & crc */
	WriteBuffer[0] = chain & 0x0f, jtag_calc_stream_crc(WriteBuffer[0], 4, &crc_w);
	WriteBuffer[0] &= 0x0f, WriteBuffer[0] |= (crc_w << 4) & 0xf0;
	WriteBuffer[1] = (crc_w >> 4) & 0xff;
	//printf("write data [%2x%2x] into DR\r\n",WriteBuffer[1], WriteBuffer[0]);
	if ((Status = JTAG_Write(ftHandle[remap[index]], FALSE, 13, &WriteBuffer, 2, RUN_TEST_IDLE_STATE)) != FTC_SUCCESS)
		return Status;
	/* set debug cmd */
	WriteBuffer[0] = 0x08;
	//printf("write data [%2x] into IR\r\n",WriteBuffer[0]);
	return JTAG_Write(ftHandle[remap[index]], TRUE, 4, &WriteBuffer, 1, RUN_TEST_IDLE_STATE);
}

/*
index: index of internal remap index
stall: 1 - stall, 0 - unstall
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 jtag_set_stall(unsigned int index, U32 stall)
{
	WriteDataByteBuffer WriteBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	unsigned int crc_w = 0;

	if ((Status = jtag_set_chain(index, 4)) != FTC_SUCCESS)
		return Status;

	WriteBuffer[0] = 0x24, WriteBuffer[0] |= ((!!stall) << 6) & 0xc0, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = 0x00, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = 0x00, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = 0x00, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = 0x00, jtag_calc_stream_crc(WriteBuffer[4], 6, &crc_w);
	WriteBuffer[4] |= (crc_w << 6) & 0xc0;
	WriteBuffer[5] = (crc_w >> 2) & 0xff;
	return JTAG_Write(ftHandle[remap[index]], FALSE, 47, &WriteBuffer, 6, RUN_TEST_IDLE_STATE);
}

/*
index: index of internal remap index
pstall: pointer for storing stall status
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pstall is NULL
other -- see FTCJTAG.h
*/
static U32 jtag_get_stall(unsigned int index, U32 *pstall)
{
	ReadDataByteBuffer ReadBuffer;
	WriteDataByteBuffer WriteBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc = 0;
	unsigned int crc_r = 0;

	if (!pstall)
		return FTC_INSUFFICIENT_RESOURCES;

	if ((Status = jtag_set_chain(index, 4)) != FTC_SUCCESS)
		return Status;

	WriteBuffer[0] = 0x04, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = 0x00, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = 0x00, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = 0x00, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = 0x00, jtag_calc_stream_crc(WriteBuffer[4], 6, &crc_w);
	WriteBuffer[4] |= (crc_w << 6) & 0xc0;
	WriteBuffer[5] = (crc_w >> 2) & 0xff;

	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 47, &WriteBuffer, 6, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 47, &WriteBuffer, 6, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	WriteBuffer[0] = ((ReadBuffer[0] >> 6) & 0x03) | ((ReadBuffer[1] << 2) & 0xfc);
	WriteBuffer[1] = ((ReadBuffer[1] >> 6) & 0x03) | ((ReadBuffer[2] << 2) & 0xfc);
	WriteBuffer[2] = ((ReadBuffer[2] >> 6) & 0x03) | ((ReadBuffer[3] << 2) & 0xfc);
	WriteBuffer[3] = ((ReadBuffer[3] >> 6) & 0x03) | ((ReadBuffer[4] << 2) & 0xfc);

	crc_r = ((ReadBuffer[4]>>6) & 0x3) | ((ReadBuffer[5] << 2) & 0xfc);
	crc_r = crc_r >> 1;
	crc = 0;

	jtag_calc_stream_crc(WriteBuffer[0], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[1], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[2], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[3], 8, &crc);

	if (crc_r != crc)
	{
	//	printf("crc check error read crc %x gen crc %x\n",crc_r,crc);
		return FTC_CRC_ERROR;
	}

	memcpy(pstall, WriteBuffer, sizeof(U32));
	return Status;
}

/*
index: index of internal remap index
addr: address of or32 component
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 jtag_write(unsigned int index, U32 addr, U32 dat)
{
	WriteDataByteBuffer WriteBuffer;
	unsigned int crc_w = 0;

	WriteBuffer[0] = (addr >> 0x00) & 0xff, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = (addr >> 0x08) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (addr >> 0x10) & 0xff, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = (addr >> 0x18) & 0xff, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = (dat << 1) & 0xfe | 0x01, jtag_calc_stream_crc(WriteBuffer[4], 8, &crc_w);
	WriteBuffer[5] = (dat >> 7) & 0xff, jtag_calc_stream_crc(WriteBuffer[5], 8, &crc_w);
	WriteBuffer[6] = (dat >> 15) & 0xff, jtag_calc_stream_crc(WriteBuffer[6], 8, &crc_w);
	WriteBuffer[7] = (dat >> 23) & 0xff, jtag_calc_stream_crc(WriteBuffer[7], 8, &crc_w);
	WriteBuffer[8] = (dat >> 31) & 0x01, jtag_calc_stream_crc(WriteBuffer[8], 1, &crc_w);
	WriteBuffer[8] &= 0x01, WriteBuffer[8] |= (crc_w << 1) & 0xfe;
	WriteBuffer[9] = (crc_w >> 7) & 0xff;

	return JTAG_AddDeviceWriteCmd(ftHandle[remap[index]], FALSE, 74, &WriteBuffer, 10, RUN_TEST_IDLE_STATE);
	//This function is to write data into a buffer, and only after jtag_execute is called, that the data is written into TDI.
}

/*
index: index of internal remap index
addr: address of or32 component
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 jtag_read(unsigned int index, U32 addr)
{
	WriteDataByteBuffer WriteBuffer;
	unsigned int crc_w = 0;

	WriteBuffer[0] = (addr >> 0x00) & 0xff, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = (addr >> 0x08) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (addr >> 0x10) & 0xff, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = (addr >> 0x18) & 0xff, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = 0x00, jtag_calc_stream_crc(WriteBuffer[4], 8, &crc_w);
	WriteBuffer[5] = 0x00, jtag_calc_stream_crc(WriteBuffer[5], 8, &crc_w);
	WriteBuffer[6] = 0x00, jtag_calc_stream_crc(WriteBuffer[6], 8, &crc_w);
	WriteBuffer[7] = 0x00, jtag_calc_stream_crc(WriteBuffer[7], 8, &crc_w);
	WriteBuffer[8] = 0x00, jtag_calc_stream_crc(WriteBuffer[8], 1, &crc_w);
	WriteBuffer[8] &= 0x01, WriteBuffer[8] |= (crc_w << 1) & 0xfe;
	WriteBuffer[9] = (crc_w >> 7) & 0xff;
	return JTAG_AddDeviceWriteReadCmd(ftHandle[remap[index]], FALSE, 74, &WriteBuffer, 10, RUN_TEST_IDLE_STATE);
}

/*
index: index of internal remap index
pp: pointer for storing returned data
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 jtag_execute(unsigned int index, void *pp)
{
	U8 *pdat = (U8 *) pp;
	ReadCmdSequenceDataByteBuffer ReadCmdSequenceDataBuffer;
	DWORD hasread, i, p;
	U32 in_data;
	U32 r_crc;
	U32 crc;
	U8* check;
	FTC_STATUS Status = FTC_SUCCESS;

	Status = JTAG_ExecuteCmdSequence(ftHandle[remap[index]],&ReadCmdSequenceDataBuffer, &hasread);

	check = (U8*)&in_data;

	if (pdat)
		for(p = 0, i = 0; i < hasread; i += 10) {

			in_data = ((ReadCmdSequenceDataBuffer[i+4] &0xfe)>>1)| 
				(ReadCmdSequenceDataBuffer[i+5] <<7)| 
				(ReadCmdSequenceDataBuffer[i+6]<<15) |
				(ReadCmdSequenceDataBuffer[i+7]<<23) | 
				((ReadCmdSequenceDataBuffer[i+8]&1)<<31);

			r_crc = ((ReadCmdSequenceDataBuffer[i+8]&0xfc)>>2)|((ReadCmdSequenceDataBuffer[i+9]&3)<<6);

			crc = 0;

			jtag_calc_stream_crc(check[0], 8, &crc);
			jtag_calc_stream_crc(check[1], 8, &crc);
			jtag_calc_stream_crc(check[2], 8, &crc);
			jtag_calc_stream_crc(check[3], 8, &crc);

			if (r_crc != crc)
			{
			//	printf("crc check error read crc %x gen crc %x\n",r_crc,crc);
				return FTC_CRC_ERROR;
			}
			else
			 //     printf("crc check ok read crc %x gen crc %x\n",r_crc,crc);


			pdat[p++] = ((ReadCmdSequenceDataBuffer[i + 4] >> 1) & 0x7f) | (((ReadCmdSequenceDataBuffer[i + 5] & 0x01) << 7) & 0x80);
			pdat[p++] = ((ReadCmdSequenceDataBuffer[i + 5] >> 1) & 0x7f) | (((ReadCmdSequenceDataBuffer[i + 6] & 0x01) << 7) & 0x80);
			pdat[p++] = ((ReadCmdSequenceDataBuffer[i + 6] >> 1) & 0x7f) | (((ReadCmdSequenceDataBuffer[i + 7] & 0x01) << 7) & 0x80);
			pdat[p++] = ((ReadCmdSequenceDataBuffer[i + 7] >> 1) & 0x7f) | (((ReadCmdSequenceDataBuffer[i + 8] & 0x01) << 7) & 0x80);
		}

	return Status;
}

/*
index: index of internal remap index
reg: register number
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 or32_write_register(unsigned int index/*chris: I'm not clear about index*/, U32 reg, U32 dat)
{
	FTC_STATUS Status = FTC_SUCCESS;

	if ((Status = jtag_set_chain(index, 1)) != FTC_SUCCESS)
		return Status;

	if ((Status = jtag_write(index, reg, dat)) != FTC_SUCCESS)
		return Status;

	return jtag_execute(index, NULL);
}

/*
index: index of internal remap index
reg: register number
pdat: pointer for restoring returned data
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
static U32 or32_read_register(unsigned int index, U32 reg, U32 *pdat)
{
	U32 rdat[2];
	FTC_STATUS Status = FTC_SUCCESS;

	if (!pdat)
		return FTC_INSUFFICIENT_RESOURCES;

	if ((Status = jtag_set_chain(index, 1)) != FTC_SUCCESS)
		return Status;

	if ((Status = jtag_read(index, reg)) != FTC_SUCCESS || (Status = jtag_read(index, reg)) != FTC_SUCCESS)
		return Status;

	Status = jtag_execute(index, rdat);
	*pdat = rdat[1];
	return Status;
}

#define QUEUE_SIZE 0x1b00

/*
index: index of internal remap index
addr: address of or32 component
pdat: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_write_wishbone(unsigned int index, U32 addr, const void *pdat, U32 len)
{
	U32 tmpdat, i;
	FTC_STATUS Status = FTC_SUCCESS;

	if (!pdat)
		return FTC_INSUFFICIENT_RESOURCES;

	if (len > QUEUE_SIZE)
		return FTC_INSUFFICIENT_RESOURCES;

	if ((Status = jtag_set_chain(index, 5)) != FTC_SUCCESS)
		return Status;

	addr &= 0xfffffffc, len &= 0xfffffffc;

	for (i = 0; i < len; i += 4)
	{
		memcpy(&tmpdat, ((const U8 *) pdat) + i, sizeof(tmpdat));
		if ((Status = jtag_write(index, addr + i, tmpdat)) != FTC_SUCCESS)
		{
			JTAG_ClearDeviceCmdSequence(ftHandle[remap[index]]);
			return Status;
		}
	}

	if ((Status = jtag_execute(index, NULL)) != FTC_SUCCESS)
		JTAG_ClearDeviceCmdSequence(ftHandle[remap[index]]);
	return Status;
}

/*
index: index of internal remap index
addr: address of or32 component
pdat: value to read out
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_read_wishbone(unsigned int index, U32 addr, void *pdat, U32 len)
{
	U32 i;
	static U8 rdat[QUEUE_SIZE + 4];
	FTC_STATUS Status = FTC_SUCCESS;

	if (len > QUEUE_SIZE)
		return FTC_INSUFFICIENT_RESOURCES;

	if (!pdat)
		return FTC_INSUFFICIENT_RESOURCES;

	if ((Status = jtag_set_chain(index, 5)) != FTC_SUCCESS)
		return Status;

	addr &= 0xfffffffc, len &= 0xfffffffc;

	for (i = 0; i < len + 4; i += 4)
		if ((Status = jtag_read(index, addr + i)) != FTC_SUCCESS)
		{
			JTAG_ClearDeviceCmdSequence(ftHandle[remap[index]]);
			return Status;
		}

	if ((Status = jtag_execute(index, rdat)) != FTC_SUCCESS)
		JTAG_ClearDeviceCmdSequence(ftHandle[remap[index]]);
	else
		memcpy(pdat, rdat + 4, len);
	return Status;
}

static void *reverse(void *p, U32 len)
{
	char *pc = (char *) p, c;
	U32 i;

	len &= 0xfffffffc;

	for (i = 0; i < len; i += 4)
	{
		c = pc[i], pc[i] = pc[i + 3], pc[i + 3] = c;
		c = pc[i + 1], pc[i + 1] = pc[i + 2], pc[i + 2] = c;
	}

	return pc;
}

#define do_return(status) \
switch (status) \
{ \
	case FTC_INVALID_HANDLE: \
		return _UP_FTC_INVALID_HANDLE; \
	case FTC_DEVICE_NOT_FOUND: \
		return _UP_FTC_DEVICE_NOT_FOUND; \
	case FTC_DEVICE_NOT_OPENED: \
		return _UP_FTC_DEVICE_NOT_OPENED; \
	case FTC_IO_ERROR: \
		return _UP_FTC_IO_ERROR; \
	case FTC_INSUFFICIENT_RESOURCES: \
		return _UP_FTC_INSUFFICIENT_RESOURCES; \
	case FTC_SUCCESS: \
	default: \
		return _UP_FTC_SUCCESS; \
}

static CRITICAL_SECTION ps;
static unsigned int inited = 0;

 U32  orjtag_init(void)
{
	printf("JtagChain application is executed!!!!!!!!\r\n");
	if (!inited)
	{
		InitializeCriticalSection(&ps);
		inited = 1;
	}
	InitDllMain();
	return _UP_FTC_SUCCESS; 
}

/*
mpt_device: info of or32 device
number_of_adapter: pointer for storing device count
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
other -- see FTCJTAG.h
*/
 U32   orjtag_device_enumerate(MPT_DEVICE *mpt_device, U32 *number_of_adapter)
{
	U32 i;
	FTC_STATUS Status = FTC_SUCCESS;

	if (!mpt_device || !number_of_adapter)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if ((Status = jtag_init(number_of_adapter)) != FTC_SUCCESS)
		do_return(Status);

	for (i = 0; i < *number_of_adapter; i++)
	{
		mpt_device[i].location_id = dwLocationID[remap[i]];
		mpt_device[i].adapter_handle = i;
		mpt_device[i].device_exist_flag = 1;
	}

	return _UP_FTC_SUCCESS;
}

/*
mpt_device: info of or32 device
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- device already closed
other -- see FTCJTAG.h
*/
 U32  orjtag_device_close(MPT_DEVICE* mpt_device)
{
	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;
	else
	{
		mpt_device->device_exist_flag = 0;
		return jtag_destroy(mpt_device->adapter_handle);
	}
}

/*
mpt_device: info of or32 device
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
 U32   orjtag_close(MPT_DEVICE* mpt_device)
{
	jtag_destroy(-1);
	inited = 0;
	DeleteCriticalSection(&ps);
	return _UP_FTC_SUCCESS;
}

/*
mpt_device: info of or32 device
address: address of or32 component
reg_value: pointer to store returned value
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_reg_read(MPT_DEVICE *mpt_device, U32 address, U32 *reg_value)
{
	U32 ret;

	if (!mpt_device || !reg_value)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	EnterCriticalSection(&ps);
	ret = or32_read_register(mpt_device->adapter_handle, address, reg_value);
	LeaveCriticalSection(&ps);

	do_return(ret);
}

/*
mpt_device: info of or32 device
address: address of or32 component
reg_value: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32   orjtag_reg_write(MPT_DEVICE *mpt_device, U32 address, U32 reg_value)
{
	U32 ret;

	if (!mpt_device)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	EnterCriticalSection(&ps);
	ret = or32_write_register(mpt_device->adapter_handle, address, reg_value);
	LeaveCriticalSection(&ps);

	do_return(ret);
}

#define min(a, b) (((a) < (b)) ? (a) : (b))

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to be written in
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_wishbone_write(MPT_DEVICE *mpt_device, U32 address, U32 length, const U8 *buffer, BOOL convertflag)
{
	static U8 ppp[QUEUE_SIZE];
	U32 cycle = length / QUEUE_SIZE + !!(length % QUEUE_SIZE);
	U32 i;
	FTC_STATUS Status = FTC_SUCCESS;

	if (!mpt_device || !buffer)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	for (i = 0; i < cycle; i++)
	{
		U32 tlg = min(QUEUE_SIZE, length - QUEUE_SIZE * i);
		EnterCriticalSection(&ps);
		memcpy(ppp, buffer + QUEUE_SIZE * i, tlg);
		Status = or32_write_wishbone(mpt_device->adapter_handle, address + i * QUEUE_SIZE, convertflag ? reverse(ppp, tlg) : ppp, tlg);
		LeaveCriticalSection(&ps);
		if (Status != FTC_SUCCESS)		
			do_return(Status);
	}

	do_return(Status);
}

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to storing returned data
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_wishbone_read(MPT_DEVICE *mpt_device, U32 address, U32 length, U8 *buffer, BOOL convertflag)
{
	U32 i;
	static U8 qqq[QUEUE_SIZE];
	U32 cycle = length / QUEUE_SIZE + !!(length % QUEUE_SIZE);
	FTC_STATUS Status = FTC_SUCCESS;

	if (!mpt_device || !buffer)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	for (i = 0; i < cycle; i++)
	{
		U32 tlg = min(QUEUE_SIZE, length - QUEUE_SIZE * i);
		EnterCriticalSection(&ps);		
		Status = or32_read_wishbone(mpt_device->adapter_handle, address + i * QUEUE_SIZE, qqq, tlg);
		LeaveCriticalSection(&ps);
		if (Status != FTC_SUCCESS)
		{
			do_return(Status);
		} else
			memcpy(buffer + i * QUEUE_SIZE, qqq, tlg);
	}

	if (convertflag)
		reverse(buffer, length);

	do_return(Status);
}

/*
mpt_device: info of or32 device
reg_value: value to be written in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_debug_write(MPT_DEVICE *mpt_device, U32 reg_value)
{
	U32 ret;

	if (!mpt_device)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	EnterCriticalSection(&ps);
	ret = jtag_set_stall(mpt_device->adapter_handle, reg_value);
	LeaveCriticalSection(&ps);

	do_return(ret);
}

/*
mpt_device: info of or32 device
reg_value: pointer for storing returned value
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_debug_read(MPT_DEVICE *mpt_device, U32 *reg_value)
{
	U32 ret;

	if (!mpt_device || !reg_value)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	EnterCriticalSection(&ps);
	ret = jtag_get_stall(mpt_device->adapter_handle, reg_value);
	LeaveCriticalSection(&ps);

	do_return(ret);
}



/*
mpt_device: info of or32 device
reg_value: pointer for storing returned value
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_idcode(MPT_DEVICE *mpt_device, U32 *idcode)
{
	U32 ret;

	if (!mpt_device || !idcode)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	if (!mpt_device->device_exist_flag)
		return _UP_FTC_INSUFFICIENT_RESOURCES;

	EnterCriticalSection(&ps);
	ret = jtag_idcode(mpt_device->adapter_handle, (DWORD *)idcode);
	LeaveCriticalSection(&ps);

	do_return(ret);
}

#if 1   //add by chris zhang , ok

/*
index: index of internal remap index
addr: address of or32 component
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/

static U32 jtag_write_atpg_clock(unsigned int index, U32 bitNum, U32 ByteNum,  const U8 *buffer)
{
	WriteDataByteBuffer WriteBuffer;
	ReadDataByteBuffer ReadBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc_r_calc = 0;
	unsigned int crc_r = 0;
	int iCycle = 0;
	WriteBuffer[0] = (buffer[0]) & 0xff, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = (buffer[1]) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (buffer[2]) & 0x3, jtag_calc_stream_crc(WriteBuffer[2], 2, &crc_w);
	WriteBuffer[2] |= (crc_w << 2) & 0xfc;
	WriteBuffer[3] = (crc_w >> 6) & 0x3;
	printf("write the following data into ATPG_CLOCK_CHAIN: \r\n");
	for(iCycle = 2; iCycle >=0; iCycle--)
	       printf("%2x", WriteBuffer[iCycle]);	
	printf("\r\n");
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 27, &WriteBuffer, 4, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	printf("the data you read out from ATPG_CLOCK_CHAIN should be 0 \r\n");
	return Status;
}

/*
index: index of internal remap index
addr: address of or32 component
pdat: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_write_atpgclock(unsigned int index, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	FTC_STATUS Status = FTC_SUCCESS;

	if ((Status = jtag_set_chain(index, ATPG_CLOCK_CHAIN)) != FTC_SUCCESS)
		//return Status;

	jtag_write_atpg_clock( index,  bitNum, ByteNum,buffer);

	return Status;
}

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to be written in
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_atpgclock_write(MPT_DEVICE *mpt_device, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	FTC_STATUS Status = FTC_SUCCESS;

	EnterCriticalSection(&ps);
	Status = or32_write_atpgclock(mpt_device->adapter_handle, bitNum, ByteNum,buffer);
	LeaveCriticalSection(&ps);

	do_return(Status);
}

#endif

#if 1  //add by chris zhang, ok

/*
index: index of internal remap index
addr: address of or32 component
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/

static U32 jtag_write_atpg_scan_vt3455(unsigned int index, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	WriteDataByteBuffer WriteBuffer;
	ReadDataByteBuffer ReadBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc_r_calc = 0;
	unsigned int crc_r = 0;
	int iCycle = 0;
	FILE *file;
	WriteBuffer[0] = (buffer[0]) & 0xff, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = (buffer[1]) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (buffer[2]) & 0xff, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = (buffer[3]) & 0xff, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = (buffer[4]) & 0xff, jtag_calc_stream_crc(WriteBuffer[4], 8, &crc_w);
	WriteBuffer[5] = (buffer[5]) & 0xff, jtag_calc_stream_crc(WriteBuffer[5], 8, &crc_w);	
	WriteBuffer[6] = (buffer[6]) & 0xff, jtag_calc_stream_crc(WriteBuffer[6], 8, &crc_w);
	WriteBuffer[7] = (buffer[7]) & 0xff, jtag_calc_stream_crc(WriteBuffer[7], 8, &crc_w);
	WriteBuffer[8] = (buffer[8]) & 0xff, jtag_calc_stream_crc(WriteBuffer[8], 8, &crc_w);	
	WriteBuffer[9] = (buffer[9]) & 0xff, jtag_calc_stream_crc(WriteBuffer[9], 8, &crc_w);
	WriteBuffer[10] = (buffer[10]) & 0xff, jtag_calc_stream_crc(WriteBuffer[10], 8, &crc_w);
	WriteBuffer[11] = (buffer[11]) & 0xff, jtag_calc_stream_crc(WriteBuffer[11], 8, &crc_w);	
	WriteBuffer[12] = (buffer[12]) & 0xff, jtag_calc_stream_crc(WriteBuffer[12], 8, &crc_w);
	WriteBuffer[13] = (buffer[13]) & 0xff, jtag_calc_stream_crc(WriteBuffer[13], 8, &crc_w);
	WriteBuffer[14] = (buffer[14]) & 0xff, jtag_calc_stream_crc(WriteBuffer[14], 8, &crc_w);	
	WriteBuffer[15] = (buffer[15]) & 0xff, jtag_calc_stream_crc(WriteBuffer[15], 8, &crc_w);
	WriteBuffer[16] = (buffer[16]) & 0xff, jtag_calc_stream_crc(WriteBuffer[16], 8, &crc_w);
	WriteBuffer[17] = (buffer[17]) & 0xff, jtag_calc_stream_crc(WriteBuffer[17], 8, &crc_w);	
	WriteBuffer[18] = (buffer[18]) & 0xff, jtag_calc_stream_crc(WriteBuffer[18], 8, &crc_w);
	WriteBuffer[19] = (buffer[19]) & 0x7, jtag_calc_stream_crc(WriteBuffer[19], 3, &crc_w);
	WriteBuffer[19] |= (crc_w << 3) & 0xf8;
	WriteBuffer[20] = (crc_w >> 5) & 0x7;
	printf("write the following data into ATPG_SCAN_CHAIN of VT3455: \r\n");
	for(iCycle = 20; iCycle >=0; iCycle--)
	       printf("%2x", WriteBuffer[iCycle]);	
	 printf("\r\n");	
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 164, &WriteBuffer, 21, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	


	jtag_calc_stream_crc(ReadBuffer[0], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[1], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[2], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[3], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[4], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[5], 8, &crc_r_calc);	
	jtag_calc_stream_crc(ReadBuffer[6], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[7], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[8], 8, &crc_r_calc);	
	jtag_calc_stream_crc(ReadBuffer[9], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[10], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[11], 8, &crc_r_calc);	
	jtag_calc_stream_crc(ReadBuffer[12], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[13], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[14], 8, &crc_r_calc);	
	jtag_calc_stream_crc(ReadBuffer[15], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[16], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[17], 8, &crc_r_calc);	
	jtag_calc_stream_crc(ReadBuffer[18], 8, &crc_r_calc);
	jtag_calc_stream_crc(ReadBuffer[19], 3, &crc_r_calc);

	
   fopen_s(&file, "scan_out.txt", "w+");

   ATPG_SCAN_CHAIN_NEW_File_Write(file,ReadBuffer);//写文件中每一行数据min：0
   fclose(file);

   
	crc_r = ((ReadBuffer[19]&0xf0)>>4)|((ReadBuffer[20]&0xf)<<4);

	if (crc_r != crc_r_calc)
	{
		printf("crc check error read crc 0x%x, calculate crc 0x%x\n",crc_r,crc_r_calc);
		return FTC_CRC_ERROR;
	}
	else
	{
		printf("the data you write to ATPG_SCAN_CHAIN last time is : \r\n");
		for(iCycle = 0; iCycle < 20; iCycle ++)
			printf("%2x  ", ReadBuffer[iCycle]);
		
		printf(" \r\n");
	}
	return Status;
	
}

/*
index: index of internal remap index
addr: address of or32 component
pdat: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_write_atpgscan_vt3455(unsigned int index, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	FTC_STATUS Status = FTC_SUCCESS;

	if ((Status = jtag_set_chain(index, ATPG_SCAN_CHAIN)) != FTC_SUCCESS)
		//return Status;

	jtag_write_atpg_scan_vt3455( index,  bitNum, ByteNum,buffer);
	return Status;
}

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to be written in
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_atpgscan_write_vt3455(MPT_DEVICE *mpt_device, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	FTC_STATUS Status = FTC_SUCCESS;

	EnterCriticalSection(&ps);
	Status = or32_write_atpgscan_vt3455(mpt_device->adapter_handle, bitNum, ByteNum,buffer);
	LeaveCriticalSection(&ps);

	do_return(Status);
}

#endif





#if 0   //add by chris zhang, ok

/*
index: index of internal remap index
addr: address of or32 component
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/

static U32 jtag_write_atpg_scan_vt3456(unsigned int index, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	WriteDataByteBuffer WriteBuffer;
	unsigned int crc_w = 0;
	WriteBuffer[0] = (buffer >> 0x00) & 0xff, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = (buffer >> 0x08) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (buffer >> 0x10) & 0xff, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = (buffer >> 0x18) & 0xff, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = (buffer >> 0x20) & 0xff, jtag_calc_stream_crc(WriteBuffer[4], 8, &crc_w);
	WriteBuffer[5] = (buffer >> 0x28) & 0xff, jtag_calc_stream_crc(WriteBuffer[5], 8, &crc_w);	
	WriteBuffer[6] = (buffer >> 0x30) & 0xff, jtag_calc_stream_crc(WriteBuffer[6], 8, &crc_w);
	WriteBuffer[7] = (buffer >> 0x38) & 0xff, jtag_calc_stream_crc(WriteBuffer[7], 8, &crc_w);
	WriteBuffer[8] = (buffer >> 0x40) & 0xff, jtag_calc_stream_crc(WriteBuffer[8], 8, &crc_w);	
	WriteBuffer[9] = (buffer >> 0x48) & 0xff, jtag_calc_stream_crc(WriteBuffer[9], 8, &crc_w);
	WriteBuffer[10] = (buffer >> 0x50) & 0xff, jtag_calc_stream_crc(WriteBuffer[10], 8, &crc_w);
	WriteBuffer[11] = (buffer >> 0x58) & 0xff, jtag_calc_stream_crc(WriteBuffer[11], 8, &crc_w);	
	WriteBuffer[12] = (buffer >> 0x60) & 0xff, jtag_calc_stream_crc(WriteBuffer[12], 8, &crc_w);
	WriteBuffer[13] = (buffer >> 0x68) & 0xff, jtag_calc_stream_crc(WriteBuffer[13], 8, &crc_w);
	WriteBuffer[14] = (buffer >> 0x70) & 0xff, jtag_calc_stream_crc(WriteBuffer[14], 8, &crc_w);	
	WriteBuffer[15] = (buffer >> 0x78) & 0xff, jtag_calc_stream_crc(WriteBuffer[15], 8, &crc_w);
	WriteBuffer[16] = (buffer >> 0x80) & 0xff, jtag_calc_stream_crc(WriteBuffer[16], 8, &crc_w);
	WriteBuffer[17] = (buffer >> 0x88) & 0xff, jtag_calc_stream_crc(WriteBuffer[17], 8, &crc_w);	
	WriteBuffer[18] = (buffer >> 0x90) & 0xff, jtag_calc_stream_crc(WriteBuffer[18], 8, &crc_w);
	WriteBuffer[19] = (buffer >> 0x98) & 0xff, jtag_calc_stream_crc(WriteBuffer[19], 3, &crc_w);
	WriteBuffer[20] = (buffer >> 0xa0) & 0xff, jtag_calc_stream_crc(WriteBuffer[20], 8, &crc_w);
	WriteBuffer[21] = (buffer >> 0xa8) & 0xff, jtag_calc_stream_crc(WriteBuffer[21], 8, &crc_w);	
	WriteBuffer[22] = (buffer >> 0xb0) & 0xff, jtag_calc_stream_crc(WriteBuffer[22], 8, &crc_w);
	WriteBuffer[23] = (buffer >> 0xb8) & 0xff, jtag_calc_stream_crc(WriteBuffer[23], 8, &crc_w);
	WriteBuffer[24] = crc_w;
	WriteBuffer[25] = 0;
	return JTAG_AddDeviceWriteCmd(ftHandle[remap[index]], FALSE, 201, &WriteBuffer, 26, RUN_TEST_IDLE_STATE);
}

/*
index: index of internal remap index
addr: address of or32 component
pdat: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_write_atpgscan_vt3456(unsigned int index, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	U32 tmpdat, i;
	FTC_STATUS Status = FTC_SUCCESS;

	if ((Status = jtag_set_chain(index, ATPG_SCAN_CHAIN)) != FTC_SUCCESS)
		return Status;

	jtag_write_atpg_scan_vt3456( index,  bitNum, ByteNum,buffer);

	if ((Status = jtag_execute(index, NULL)) != FTC_SUCCESS)
		JTAG_ClearDeviceCmdSequence(ftHandle[remap[index]]);
	return Status;
}

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to be written in
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
U32  orjtag_atpgscan_write_vt3456(MPT_DEVICE *mpt_device, U32 bitNum, U32 ByteNum, const U8 *buffer)
{
	FTC_STATUS Status = FTC_SUCCESS;

	EnterCriticalSection(&ps);
	Status = or32_write_atpgscan_vt3456(mpt_device->adapter_handle, bitNum, ByteNum,buffer);
	LeaveCriticalSection(&ps);

	do_return(Status);
}

#endif


#if 1   //add by chris zhang

/*
index: index of internal remap index
addr: address of or32 component
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/
/*
static U32 jtag_write_register_scan(unsigned int index,  U32 address, U32 reg_value, U32 RW)
{
	WriteDataByteBuffer WriteBuffer;
	ReadDataByteBuffer ReadBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc_r_calc = 0;
	unsigned int crc_r = 0;
	int iCycle = 0;
	
	WriteBuffer[0] = address& 0x1f;
	WriteBuffer[0] |= ((RW&0x1)<<5);
	WriteBuffer[0] |= ((reg_value &0x3)<<6), jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);	
	WriteBuffer[1] = (reg_value >> 0x02) & 0x3f, jtag_calc_stream_crc(WriteBuffer[1], 6, &crc_w);
	WriteBuffer[1] |= (crc_w & 0x3)<<6;
	WriteBuffer[2] = (crc_w & 0xfc)>>2;
	printf("write the following data into REGISTER_SCAN_CHAIN: \r\n");
	for(iCycle = 1; iCycle >=0; iCycle--)
	       printf("%2x", WriteBuffer[iCycle]);	
	 printf("\r\n");	
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 23, &WriteBuffer, 3, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	//if(RW == 1)  //according to current design, can do nothing for this case..
	
	if(RW == 0)
	{
		jtag_calc_stream_crc(ReadBuffer[0], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[1], 6, &crc_r_calc);

		crc_r = ((ReadBuffer[1]&0x80)>>7)|((ReadBuffer[2]&0xfe)<<1);

		if (crc_r != crc_r_calc)
		{
			printf("crc check error read crc 0x%x, calculate crc 0x%x\n",crc_r,crc_r_calc);
			return FTC_CRC_ERROR;
		}
		else
		{
			printf("the data you read from REGISTER_SCAN_CHAIN last time is : %2x\r\n", (((ReadBuffer[0]&0xc0)>>6)|((ReadBuffer[1]&0x3f)<<2)) );
		}
		return Status;
	}
	return Status;
}
*/

static U32 jtag_write_register_scan(unsigned int index,  U32 address, U32 reg_value, U32 RW)
{
	WriteDataByteBuffer WriteBuffer;
	ReadDataByteBuffer ReadBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc_r_calc = 0;
	unsigned int crc_r = 0;
	int iCycle = 0;
	
	WriteBuffer[0] = address& 0x1f;
	WriteBuffer[0] |= ((RW&0x1)<<5);
	WriteBuffer[0] |= ((reg_value &0x3)<<6), jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);	
	WriteBuffer[1] = (reg_value >> 0x02) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (reg_value >> 0x0a) & 0xff, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = (reg_value >> 0x12) & 0xff, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = (reg_value >> 0x1a) & 0x3f, jtag_calc_stream_crc(WriteBuffer[4], 6, &crc_w);
	
	WriteBuffer[4] |= (crc_w & 0x3)<<6;
	WriteBuffer[5] = (crc_w & 0xfc)>>2;
	printf("write the following data into REGISTER_SCAN_CHAIN: \r\n");
	for(iCycle = 5; iCycle >=0; iCycle--)
	       printf("%2x\n", WriteBuffer[iCycle]);	
	 printf("\r\n");	
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 47, &WriteBuffer, 6, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	//if(RW == 1)  //according to current design, can do nothing for this case..
	
	if(RW == 0)
	{
		WriteBuffer[0] = ((ReadBuffer[0] >> 6) & 0x03) | ((ReadBuffer[1] << 2) & 0xfc);
	WriteBuffer[1] = ((ReadBuffer[1] >> 6) & 0x03) | ((ReadBuffer[2] << 2) & 0xfc);
	WriteBuffer[2] = ((ReadBuffer[2] >> 6) & 0x03) | ((ReadBuffer[3] << 2) & 0xfc);
	WriteBuffer[3] = ((ReadBuffer[3] >> 6) & 0x03) | ((ReadBuffer[4] << 2) & 0xfc);

	crc_r = ((ReadBuffer[4]>>6) & 0x3) | ((ReadBuffer[5] << 2) & 0xfc);
	crc_r = crc_r >> 1;
	crc_r_calc = 0;

	jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_r_calc);
	jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_r_calc);
	jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_r_calc);
	jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_r_calc);

		if (crc_r != crc_r_calc)
		{
			printf("crc check error read crc 0x%x, calculate crc 0x%x\n",crc_r,crc_r_calc);
			return FTC_CRC_ERROR;
		}
		else
		{
			printf("the data you read from REGISTER_SCAN_CHAIN last time is : %2x\r\n", (((ReadBuffer[0]&0xc0)>>6)|((ReadBuffer[1]&0x3f)<<2)) );
		}
		return Status;
	}
	return Status;
}
/*
index: index of internal remap index
addr: address of or32 component
pdat: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_write_registerscan(unsigned int index,  U32 address, U32 reg_value, U32 RW)
{
	FTC_STATUS Status = FTC_SUCCESS;

	if ((Status = jtag_set_chain(index, REGISTER_SCAN_CHAIN)) != FTC_SUCCESS)
		return Status;

	jtag_write_register_scan( index, address, reg_value, RW);

	return Status;
}

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to be written in
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_registerscan_write(MPT_DEVICE *mpt_device,  U32 address, U32 reg_value, U32 RW)
{
	FTC_STATUS Status = FTC_SUCCESS;

	EnterCriticalSection(&ps);
	Status = or32_write_registerscan(mpt_device->adapter_handle, address, reg_value, RW);
	LeaveCriticalSection(&ps);

	do_return(Status);
}

#endif
#if 1 //modify by andy yin
static U32 jtag_tracescan_settrigger(MPT_DEVICE *mpt_device,U32 TSEL,U32 QSEL,U32 SSEL,U32 RECSEL,U32 MODER)
{
	FTC_STATUS Status = FTC_SUCCESS;
	Status=orjtag_registerscan_write(mpt_device, 0x4,TSEL, 1);
	Status=orjtag_registerscan_write(mpt_device, 0x8,QSEL, 1);
	Status=orjtag_registerscan_write(mpt_device, 0xc,SSEL, 1);
	Status=orjtag_registerscan_write(mpt_device, 0x40,RECSEL, 1);
	Status=orjtag_registerscan_write(mpt_device, 0x0,MODER, 1);
	return Status;
}

 U32  orjtag_tracescan_write(MPT_DEVICE *mpt_device,U32 TSEL,U32 QSEL,U32 SSEL,U32 RECSEL,U32 MODER)
{
	FTC_STATUS Status = FTC_SUCCESS;

	EnterCriticalSection(&ps);
	Status = jtag_tracescan_settrigger(mpt_device,TSEL,QSEL,SSEL, RECSEL, MODER);
	LeaveCriticalSection(&ps);

	do_return(Status);
}
 U32  orjtag_tracescan_read(MPT_DEVICE *mpt_device)
 {
 	ReadDataByteBuffer ReadBuffer;
	WriteDataByteBuffer WriteBuffer;
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc = 0;
	unsigned int crc_r = 0;
	unsigned int index= 0;
	index=mpt_device->adapter_handle;
	if ((Status = jtag_set_chain(index, TRACE_SCAN_CHAIN)) != FTC_SUCCESS)
		//return Status;

	WriteBuffer[0] = 0x00, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = 0x00, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = 0x00, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = 0x00, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = 0x00, jtag_calc_stream_crc(WriteBuffer[4], 8, &crc_w);
	WriteBuffer[5] = crc_w& 0xff;

	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 48, &WriteBuffer, 6, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 48, &WriteBuffer, 6, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);
    if ((ReadBuffer[0] & 0x01) == 0)//invalid
	{
		printf("data invalid\n");
		return 0;
	}

	WriteBuffer[0] = ((ReadBuffer[0] >> 4) & 0x0f) | ((ReadBuffer[1] << 4) & 0xf0);
	WriteBuffer[1] = ((ReadBuffer[1] >> 4) & 0x0f) | ((ReadBuffer[2] << 4) & 0xf0);
	WriteBuffer[2] = ((ReadBuffer[2] >> 4) & 0x0f) | ((ReadBuffer[3] << 4) & 0xf0);
	WriteBuffer[3] = ((ReadBuffer[3] >> 4) & 0x0f) | ((ReadBuffer[4] << 4) & 0xf0);
	WriteBuffer[4] = ((ReadBuffer[4] >> 4) & 0x0f) ;
	crc_r =  ReadBuffer[5];
	//crc_r = crc_r >> 1;
	crc = 0;

	jtag_calc_stream_crc(WriteBuffer[0], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[1], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[2], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[3], 8, &crc);
	jtag_calc_stream_crc(WriteBuffer[4], 4, &crc);

	if (crc_r != crc)
	{
	//	printf("crc check error read crc %x gen crc %x\n",crc_r,crc);
		return FTC_CRC_ERROR;
	}
	else
	{
		printf("0x%x%x%x%x%x\n",WriteBuffer[4],WriteBuffer[3],WriteBuffer[2],WriteBuffer[1],WriteBuffer[0]);
	}
	
	return Status;
 }
#endif
#if 1   //add by chris zhang

/*
index: index of internal remap index
addr: address of or32 component
dat: value to write in
return value:
FTC_SUCCESS -- success
other -- see FTCJTAG.h
*/

static U32 jtag_write_wishbone_scan(unsigned int index, ST_WISHBONE_SCAN_CHAIN* p_wishbone_scan_chain)
{
	WriteDataByteBuffer WriteBuffer;
	ReadDataByteBuffer ReadBuffer;
	unsigned char PrintBuffer[10];
	FTC_STATUS Status = FTC_SUCCESS;
	DWORD dwNumBytesReturned = 0;
	unsigned int crc_w = 0;
	unsigned int crc_r_calc = 0;
	unsigned int crc_r = 0;
	int iCycle = 0;
	WriteBuffer[0] = (p_wishbone_scan_chain->ADDR_L >> 0x00) & 0xff, jtag_calc_stream_crc(WriteBuffer[0], 8, &crc_w);
	WriteBuffer[1] = (p_wishbone_scan_chain->ADDR_L >> 0x08) & 0xff, jtag_calc_stream_crc(WriteBuffer[1], 8, &crc_w);
	WriteBuffer[2] = (p_wishbone_scan_chain->ADDR_L >> 0x10) & 0xff, jtag_calc_stream_crc(WriteBuffer[2], 8, &crc_w);
	WriteBuffer[3] = (p_wishbone_scan_chain->ADDR_L >> 0x18) & 0xff, jtag_calc_stream_crc(WriteBuffer[3], 8, &crc_w);
	WriteBuffer[4] = p_wishbone_scan_chain->ADDR_H & 0x3;
	WriteBuffer[4] |= ((p_wishbone_scan_chain->RW& 0x1) << 2)&0x4;
	WriteBuffer[4] |= ((p_wishbone_scan_chain->MIO&0x1) << 3)&0x8;
	WriteBuffer[4] |= (p_wishbone_scan_chain->BE&0xf) << 4,  jtag_calc_stream_crc(WriteBuffer[4], 8, &crc_w);
	WriteBuffer[5] = (p_wishbone_scan_chain->BE&0xf0) >>4;
	WriteBuffer[5] |= (p_wishbone_scan_chain->LTWD_L&0xf) << 4,  jtag_calc_stream_crc(WriteBuffer[5], 8, &crc_w);	
	WriteBuffer[6] = (p_wishbone_scan_chain->LTWD_L>>0x4) & 0xff,  jtag_calc_stream_crc(WriteBuffer[6], 8, &crc_w);	
	WriteBuffer[7] = (p_wishbone_scan_chain->LTWD_L>>0xc) & 0xff,  jtag_calc_stream_crc(WriteBuffer[7], 8, &crc_w);	
	WriteBuffer[8] = (p_wishbone_scan_chain->LTWD_L>>0x14) & 0xff,  jtag_calc_stream_crc(WriteBuffer[8], 8, &crc_w);
	WriteBuffer[9] = (p_wishbone_scan_chain->LTWD_L >> 0x1c)&0x0f;
	WriteBuffer[9] |= (p_wishbone_scan_chain->LTWD_H&0xf) << 4,  jtag_calc_stream_crc(WriteBuffer[9], 8, &crc_w);
	
	WriteBuffer[10] = (p_wishbone_scan_chain->LTWD_H>>0x4) & 0xff,  jtag_calc_stream_crc(WriteBuffer[10], 8, &crc_w);	
	WriteBuffer[11] = (p_wishbone_scan_chain->LTWD_H>>0xc) & 0xff,  jtag_calc_stream_crc(WriteBuffer[11], 8, &crc_w);	
	WriteBuffer[12] = (p_wishbone_scan_chain->LTWD_H>>0x14) & 0xff,  jtag_calc_stream_crc(WriteBuffer[12], 8, &crc_w);
	WriteBuffer[13] = (p_wishbone_scan_chain->LTWD_H >> 0x1c)&0x0f;
       WriteBuffer[13] |= (p_wishbone_scan_chain->LTMMCFG&0x1)<<4;
       WriteBuffer[13] |= (p_wishbone_scan_chain->RCRBH_CYC&0x1)<<5;	   
	WriteBuffer[13] |= (p_wishbone_scan_chain->LTXREGA&0x3)<<6,  jtag_calc_stream_crc(WriteBuffer[13], 8, &crc_w);
	WriteBuffer[14] = (p_wishbone_scan_chain->LTXREGA&0xc)>>2,  jtag_calc_stream_crc(WriteBuffer[14], 2, &crc_w);
	WriteBuffer[14] |= (crc_w & 0x3f)<<2;
	WriteBuffer[15] = (crc_w & 0xc0)>>6;
	printf("write the following data into WISHBONE_SCAN_CHAIN2: crc_wx = 0x%2x, crc_wd = %x\r\n", crc_w, crc_w);
	for(iCycle = 15; iCycle >=0; iCycle--)
	       printf("%2x", WriteBuffer[iCycle]);	
	 printf("\r\n");	
	Status = JTAG_WriteRead(ftHandle[remap[index]], FALSE, 123, &WriteBuffer, 16, &ReadBuffer, &dwNumBytesReturned, RUN_TEST_IDLE_STATE);

	//if(RW == 1)  //according to current design, can do nothing for this case..
	
	if(p_wishbone_scan_chain->RW  == 0)
	{
		jtag_calc_stream_crc(ReadBuffer[0], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[1], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[2], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[3], 8, &crc_r_calc);		
		jtag_calc_stream_crc(ReadBuffer[4], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[5], 8, &crc_r_calc);	
		jtag_calc_stream_crc(ReadBuffer[6], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[7], 8, &crc_r_calc);		
		jtag_calc_stream_crc(ReadBuffer[8], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[9], 8, &crc_r_calc);	
		jtag_calc_stream_crc(ReadBuffer[10], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[11], 8, &crc_r_calc);		
		jtag_calc_stream_crc(ReadBuffer[12], 8, &crc_r_calc);
		jtag_calc_stream_crc(ReadBuffer[13], 8, &crc_r_calc);			
		jtag_calc_stream_crc(ReadBuffer[14], 2, &crc_r_calc);	

printf("%s",ReadBuffer);
		crc_r = ((ReadBuffer[14]&0xf8)>>3)|((ReadBuffer[15]&0x07)<<5);

		if (crc_r != crc_r_calc)
		{
			printf("crc check error read crc 0x%x, calculate crc 0x%x\n",crc_r,crc_r_calc);
			return FTC_CRC_ERROR;
		}
		else
		{
			PrintBuffer[0] = ((ReadBuffer[5]&0xf0)>>4)|((ReadBuffer[6]&0xf)<<4)	;	
			PrintBuffer[1] = ((ReadBuffer[6]&0xf0)>>4)|((ReadBuffer[7]&0xf)<<4)	;	
			PrintBuffer[2] = ((ReadBuffer[7]&0xf0)>>4)|((ReadBuffer[8]&0xf)<<4)	;	
			PrintBuffer[3] = ((ReadBuffer[8]&0xf0)>>4)|((ReadBuffer[9]&0xf)<<4)	;	
			PrintBuffer[4] = ((ReadBuffer[9]&0xf0)>>4)|((ReadBuffer[10]&0xf)<<4);	
			PrintBuffer[5] = ((ReadBuffer[10]&0xf0)>>4)|((ReadBuffer[11]&0xf)<<4);	
			PrintBuffer[6] = ((ReadBuffer[11]&0xf0)>>4)|((ReadBuffer[12]&0xf)<<4);	
			PrintBuffer[7] = ((ReadBuffer[12]&0xf0)>>4)|((ReadBuffer[13]&0xf)<<4);	

			
			printf("the data you read from WISHBONE_SCAN_CHAIN last time is : %2x %2x %2x %2x %2x %2x %2x %2x\r\n",PrintBuffer[0], PrintBuffer[1],PrintBuffer[2], PrintBuffer[3],PrintBuffer[4], PrintBuffer[5],PrintBuffer[6], PrintBuffer[7]);
		}
		return Status;
	}
	return Status;
}

/*
index: index of internal remap index
addr: address of or32 component
pdat: value to write in
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- pdat is NULL
FTC_INSUFFICIENT_RESOURCES -- len > 0x1b00
other -- see FTCJTAG.h
*/
static U32 or32_write_wishbonescan(unsigned int index, ST_WISHBONE_SCAN_CHAIN* p_wishbone_scan_chain)
{
	FTC_STATUS Status = FTC_SUCCESS;

	if ((Status = jtag_set_chain(index, WISHBONE_SCAN_CHAIN)) != FTC_SUCCESS)
		//return Status;

	jtag_write_wishbone_scan( index, p_wishbone_scan_chain);

	return Status;
}

/*
mpt_device: info of or32 device
address: address of or32 component
length: count of bytes
buffer: data to be written in
converflag: endian reverse
return value:
FTC_SUCCESS -- success
FTC_INSUFFICIENT_RESOURCES -- any pointer is NULL
FTC_INSUFFICIENT_RESOURCES -- device is closed
other -- see FTCJTAG.h
*/
 U32  orjtag_wishbonescan_write(MPT_DEVICE *mpt_device, ST_WISHBONE_SCAN_CHAIN* p_wishbone_scan_chain)
{
	FTC_STATUS Status = FTC_SUCCESS;

	EnterCriticalSection(&ps);
	Status = or32_write_wishbonescan(mpt_device->adapter_handle, p_wishbone_scan_chain);
	LeaveCriticalSection(&ps);

	do_return(Status);
}

#endif

