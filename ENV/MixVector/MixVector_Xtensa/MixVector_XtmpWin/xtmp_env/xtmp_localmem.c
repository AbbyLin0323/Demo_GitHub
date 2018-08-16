/*******************************************************************************
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
********************************************************************************
xtmp_localmem.c: handle for local memory access
*******************************************************************************/
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "xtmp_config.h"
#include "xtmp_common.h"
#include "HAL_Multicore.h"



HANDLE DataRamBusyMutex;
FILE* p_busFile;
bool g_bRedirectPcFlag = false;

LocalMemoryData g_DsramStruct[MAX_DATA_RAM0_SECTION];
LocalMemoryData g_DsramShareStruct[MAX_DATA_RAM1_SECTION];
LocalMemoryData g_IsramStruct[MAX_IRAM_SECTION];

extern void handleReadReg(XTMP_address regaddr, u32 regvalue, u32 nsize);
extern bool handleWriteReg(XTMP_address regaddr, u32 regvalue, u32 nsize);

u8 *
    getByteAddress(LocalMemoryData *localMemory, XTMP_address addr)
{
    return localMemory->data + (addr - localMemory->base);
}

u8 * GetDsramLocalAddr(XTMP_address addr, LocalMemoryData *localMemory)
{
    u8 *buf = 0;
    if ((addr >= localMemory->base) && 
        (addr < localMemory->base + localMemory->size))
    {
        buf = getByteAddress(localMemory, addr);
    }
    return buf;
}

u8* GetVirtualAddrInLocalMem(XTMP_address addr)
{
    LocalMemoryData *localMemory = 0;
    u8 *buf = NULL;
    u8 uDataRamIndex = 0;

    for(uDataRamIndex = 0; uDataRamIndex < MAX_DATA_RAM1_SECTION; uDataRamIndex++)
    {
        localMemory = (LocalMemoryData *)&g_DsramShareStruct[uDataRamIndex];
        buf = GetDsramLocalAddr(addr, localMemory);

        if (NULL != buf)
        {
            return buf;
        }
        else
        {
            continue;
        }
    }

    for(uDataRamIndex = 0; uDataRamIndex < MAX_DATA_RAM0_SECTION; uDataRamIndex++)
    {
        localMemory = (LocalMemoryData *)&g_DsramStruct[uDataRamIndex];
        buf = GetDsramLocalAddr(addr, localMemory);
        if (NULL != buf)
        {
            return buf;
        }
        else
        {
            continue;
        }
    }

    assert(NULL != buf);
    return buf;
}

/***************************************** 
*function:read register value at local port 0
*input: addr: registers'address 
nBytes: data length
*dst: the destination buffer'address
*output: none
******************************************/
void regRead(u32 addr, u32 nBytes, u8 *dst)
{
    LocalMemoryData *localMemory = 0;
    u8 *buf = NULL;
    u8 uDataRamIndex = 0;

    for(uDataRamIndex = 0; uDataRamIndex < MAX_DATA_RAM1_SECTION; uDataRamIndex++)
    {
        localMemory = (LocalMemoryData *)&g_DsramShareStruct[uDataRamIndex];
        buf = GetDsramLocalAddr(addr, localMemory);

        if (NULL != buf)
        {
            memcpy(dst, buf, nBytes);
            return;
        }
        else
        {
            continue;
        }
    }

    //privacy dsram section
    for(uDataRamIndex = 0; uDataRamIndex < MAX_DATA_RAM0_SECTION; uDataRamIndex++)
    {
        localMemory = (LocalMemoryData *)&g_DsramStruct[uDataRamIndex];
        buf = GetDsramLocalAddr(addr, localMemory);

        if (NULL != buf)
        {
            memcpy(dst, buf, nBytes);
            return;
        }
        else
        {
            continue;
        }
    }

    assert(NULL != buf);
    return;
}

//void regRead(u32 addr, u32 nBytes, u8 *dst)
//{
//    LocalMemoryData *localMemory = &dataRam0Struct;
//    u8 *buf = getByteAddress(localMemory, addr);
//    memcpy(dst, buf, nBytes);
//
//    return;
//}
/*
function: read register value at local port 1
NOT call-back function, it is called by hw model.
*/
//void regRead1(u32 addr, u32 nBytes, u8 *dst)
//{
//    LocalMemoryData *localMemory = &dataRam1Struct;
//    u8 *buf = getByteAddress(localMemory, addr);
//    memcpy(dst, buf, nBytes);
//
//    return;
//}

// it's address will be u32 allaigned
void spinLockWrite(u32 regaddr, u32 regvalue)
{
    U32 nCurrentValue = 0;

    regRead(regaddr, sizeof(u32), (U8*)&nCurrentValue);

    if (0 == nCurrentValue)
    {
        regWrite(regaddr, sizeof(u32), (U8*)&regvalue);
    }
    else
    {
        if (0 == regvalue)
        {
            regWrite(regaddr, sizeof(u32), (U8*)&regvalue);
        }
    }

    return;
}

/***************************************** 
*function:write register value at local port 0
*input: addr: registers'address 
nBytes: data length
*src: the destination buffer'address
*output: none
******************************************/
void regWrite(u32 addr, u32 nBytes, const u8*src)
{
    u8 *buf = 0;
    u8 uDataRamIndex = 0;
    LocalMemoryData *localMemory = 0;

    //share dsram section
    for(uDataRamIndex = 0; uDataRamIndex < MAX_DATA_RAM1_SECTION; uDataRamIndex++)
    {
        localMemory = (LocalMemoryData *)&g_DsramShareStruct[uDataRamIndex];
        buf = GetDsramLocalAddr(addr, localMemory);

        if (NULL != buf)
        {
            memcpy(buf, src, nBytes);
            return;
        }
        else
        {
            continue;
        }
    }

    //privacy dsram section
    for(uDataRamIndex = 0; uDataRamIndex < MAX_DATA_RAM0_SECTION; uDataRamIndex++)
    {
        localMemory = (LocalMemoryData *)&g_DsramStruct[uDataRamIndex];
        buf = GetDsramLocalAddr(addr, localMemory);

        if (NULL != buf)
        {
            memcpy(buf, src, nBytes);
            return;
        }
        else
        {
            continue;
        }
    }

    assert(0 != buf);
    return;
}

/*
* Read data from the location corresponding to the requested address.
*/
XTMP_deviceStatus
    memPeek(void *deviceData, u32 *dst, XTMP_address addr, u32 size)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    u8 *buf = getByteAddress(localMemory, addr);
    u32 i;

    handleReadReg(addr, *(u32*)buf, size);

    for (i = 0; i < size; i++) {
        // Generic insert function
        XTMP_insert(dst, addr+i, buf[i], 1, 
            localMemory->byteWidth, localMemory->bigEndian);
        // Config-specific insert macro for local data memory
        // XTMP_LS_INSERT(8)(dst, addr, buf[i]);

    }

    return XTMP_DEVICE_OK;
}

/*
* Write data into the location corresponding to the requested address.
*/
XTMP_deviceStatus
    memPoke(void *deviceData, XTMP_address addr, u32 size, const u32 *src)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    u8 *buf = getByteAddress(localMemory, addr);
    u32 i;

    u32 spinLockStartAddr = SPINLOCK_BASE_ADDRESS;
    u32 spinLockEndAddr = (SPINLOCK_BASE_ADDRESS + 128 * sizeof(u32));

    bool bWriteThru = handleWriteReg(addr, *(u32*)src, size);

    if ( bWriteThru )
    {  
        if (addr >= spinLockStartAddr && addr <= spinLockEndAddr)
        {
            spinLockWrite((u32)addr, *(u32*)src);
        }
        else
        {
            for (i = 0; i < size; i++) 
            {
                // Generic extract function
                buf[i] = XTMP_extract(src, addr+i, 1,
                    localMemory->byteWidth, localMemory->bigEndian);
                // Config-specific extract macro for local data memory
                // buf[i] = XTMP_LS_EXTRACT(8)(src, addr);
            }
        }
    }

    return XTMP_DEVICE_OK;
}

/*
* Determine if the memory is busy.
* In this example, we use a simple random number computation.
*/

// return TRUE, the local memory is busy
// return FALSE, the local memory is idle
static bool
    setBusy(LocalMemoryData *localMemory)
{
    bool brtn = TRUE;
    WaitForSingleObject(DataRamBusyMutex, INFINITE);
    /*
    p_busFile = fopen("localbusy.log", "a");
    if (NULL != p_busFile)
    fprintf(p_busFile,"the busy is %d\n", localMemory->hasBusy);
    fclose(p_busFile);*/

    if (!localMemory->hasBusy)
    {
        brtn = FALSE;
        localMemory->hasBusy = 1;
    }
    ReleaseMutex(DataRamBusyMutex);


    return brtn;
}

static void
    releaseBusy(LocalMemoryData *localMemory)
{
    WaitForSingleObject(DataRamBusyMutex, INFINITE);
    localMemory->hasBusy = 0;

    /*	p_busFile = fopen("localbusy.log", "a");
    if (NULL != p_busFile)
    fprintf(p_busFile,"release the busy \n");
    fclose(p_busFile);*/
    ReleaseMutex(DataRamBusyMutex);

}

/*
* Device function for the "post" callback.  Since the device is intended
* for connection to an internal memory port, it will repond immediately
* upon receiving a request.  A device counnected to the PIF would need to
* wait for at least one cycle (to model PIF latency) and would require
* a user thread function to implement the delay.
*/
XTMP_deviceStatus
    memPost(void *deviceData, XTMP_deviceXfer *xfer)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    XTMP_address address = XTMP_xferGetAddress(xfer);
    u32 size = XTMP_xferGetSize(xfer);
    u32 *data = XTMP_xferGetData(xfer);
    XTMP_deviceStatus status = XTMP_DEVICE_ERROR;

#if defined(SIM_FOR_3_CORE) && !defined(ROM_BOOTLOADER_FW_FLOW) && !defined(BOOTLOADER_TEST)
    u32 ulCurrentMcu0PC;
    ulCurrentMcu0PC = XTMP_getStagePC(g_core[0], 0);
    //3. redirect mcu0 pc to header addr
    if((false == g_bRedirectPcFlag) && (ulCurrentMcu0PC != 0xffffffff))
    {
        g_bRedirectPcFlag = XTMP_redirectPC(g_core[0], ENTRANCE_TEXT_PC); 
        ulCurrentMcu0PC = XTMP_getStagePC(g_core[0], 0);
    }
#endif

    if (XTMP_xferIsRequest(xfer)) 
    {
        if (setBusy(localMemory))
            return XTMP_respond(xfer, XTMP_NACC);
        if (XTMP_xferGetType(xfer) == XTMP_READ) 
        {
            memPeek(deviceData, data, address, size);
            status = XTMP_respond(xfer, XTMP_OK);

        }
        if (XTMP_xferGetType(xfer) == XTMP_WRITE) {
            memPoke(deviceData, address, size, data);
            status = XTMP_respond(xfer, XTMP_OK);	
        }
    }

    releaseBusy(localMemory);

    /*
    p_busFile = fopen("localbusy.log", "a");
    if (NULL != p_busFile)
    fprintf(p_busFile,"the xfer the is finish!\n\n\n");
    fclose(p_busFile);
    */
    /* Local memories receive only single read and write requests. */
    return status;
}


/*
* Fast-access read callback for little endian requests
*/

static void
    memFastReadLE(void *deviceData, u32 *dst, XTMP_address addr, u32 size)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    u8 *buf = getByteAddress(localMemory, addr);
    u32 i;
    dst[0] = 0;
    for (i = 0; i < size; i++) {
        XTMP_insert32LE8(&dst[i>>2], i, buf[i]);
    }
}

/*
* Fast-access read callback for big endian requests
*/

static void
    memFastReadBE(void *deviceData, u32 *dst, XTMP_address addr, u32 size)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    u8 *buf = getByteAddress(localMemory, addr);
    u32 i;
    dst[0] = 0;
    for (i = 0; i < size; i++) {
        u32 dst_offset = size - i - 1;
        XTMP_insert32LE8(&dst[dst_offset>>2], dst_offset, buf[i]);
    }
}

/*
* Fast-access write callback for little endian requests
*/

static void
    memFastWriteLE(void *deviceData, XTMP_address addr, u32 size, const u32 *src)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    u8 *buf = getByteAddress(localMemory, addr);
    u32 i;
    for (i = 0; i < size; i++) {
        buf[i] = XTMP_extract32LE8(&src[i>>2], i);
    }
}

/*
* Fast-access write callback for big endian requests
*/

static void
    memFastWriteBE(void *deviceData, XTMP_address addr, u32 size, const u32 *src)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    u8 *buf = getByteAddress(localMemory, addr);
    u32 i;
    for (i = 0; i < size; i++) {
        u32 dst_offset = size - i - 1;
        buf[i] = XTMP_extract32LE8(&src[dst_offset>>2], dst_offset);
    }
}

/*
* Fast-acess handler
*/
XTMP_deviceStatus
    memFastAccess(void *deviceData,
    XTMP_fastAccessRequest request,
    XTMP_address addr)
{
    LocalMemoryData *localMemory = (LocalMemoryData *) deviceData;
    XTMP_address start = localMemory->base;
    XTMP_address end = start + localMemory->size - 1;

    /* Chose the fast-access data transfer method */
#if !defined(FAST_ACCESS_RAW) && !defined(FAST_ACCESS_CALLBACKS) && !defined(FAST_ACCESS_PEEKPOKE)
#define FAST_ACCESS_RAW       1
#define FAST_ACCESS_CALLBACKS 0
#define FAST_ACCESS_PEEKPOKE  0
#endif

    if (FAST_ACCESS_RAW) {
        XTMP_setFastAccessRaw(request, addr, start, end,
            (u32 *) localMemory->data, 0 /* swizzle */);
    }
    else if (FAST_ACCESS_CALLBACKS) {
        if (!XTMP_isFastAccessRequestBigEndian(request)) {
            XTMP_setFastAccessCallBacks(request, addr, start, end,
                memFastReadLE, memFastWriteLE, deviceData);
        } else {
            XTMP_setFastAccessCallBacks(request, addr, start, end,
                memFastReadBE, memFastWriteBE, deviceData);
        }
    }
    else if (FAST_ACCESS_PEEKPOKE) {
        XTMP_setFastAccessPeekPoke(request, addr, start, end);
    }
    else {
        XTMP_denyFastAccess(request, addr, start, end);
    }
    return XTMP_DEVICE_OK;
}



