/*
* Customer ID=5586; Build=0x36b41; Copyright (c) 2004-2009 by Tensilica Inc.  ALL RIGHTS RESERVED.
* 
* These coded instructions, statements, and computer programs are the
* copyrighted works and confidential proprietary information of
* Tensilica Inc.  They may be adapted and modified by bona fide
* purchasers for internal use, but neither the original nor any adapted
* or modified version may be disclosed or distributed to third parties
* in any manner, medium, or form, in whole or in part, without the prior
* written consent of Tensilica Inc.
*/

/*
* A custom XTMP device which implements memory attached to the PIF.
*
* This is similiar to the built-in XTMP_memory device. It accepts read, 
* write, block-read and block-write transactions, one at a time and 
* responds after the configured delays.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "xtmp_config.h"
#include "xtmp_options.h"
#include "xtmp_common.h"
#include "HAL_PM.h"

/*
* Delay parameters for each transaction type. Since the device will be 
* connected to the PIF, each delay value must be at least 1, which is
* the inherent latency of the PIF.  An additional delay would correspond
* to the time expended within the device itself.
*/
extern void handleReadReg(XTMP_address regaddr, u32 regvalue, u32 nsize);
extern bool handleWriteReg(XTMP_address regaddr, u32 regvalue, u32 nsize);
extern void PmuClearIDsramOtfbData(void);

SysMemData sysPIFRegData = { { 0 } };

extern u32 pifWidth;
extern u8 *g_pIsramBaseAddr[];
extern u8 *g_pDsramBaseAddr[];
extern u8 *g_pDsramShareBaseAddr;
extern u8 g_ucSystemStsSuspendFlag;
/*
* A simple memory model that allocates space only for "pages" in use.
*/
static u32 *
    getWordAddress(XTMP_address addr, u32 **pages)
{
    u32 pageIdx = addr >> PAGE_BITS;
    if (!pages[pageIdx])
        pages[pageIdx] = (u32 *)malloc(PAGE_SIZE);
    return pages[pageIdx] + ((addr & PAGE_MASK) >> 2);
}

/*
function: read/write dwords at system mem(NOT just dram) address.
NOT call-back function, it is called by hw model.
but the two function will invoke Peek/Poke call-back
*/
// r/w dram, addr be aligned on a word boundary
void dramRead(u32 addr, u32 nWordss, u32 *buf)
{
    XTMP_peekVirtual(g_core[0], buf, addr, nWordss);
}
void dramWrite(u32 addr, u32 nWordss, const u32* src)
{
    XTMP_pokeVirtual(g_core[0], (u32 *)src, addr, nWordss);
}
void dramReadByByte(u32 addr, u32 nBytes, u8 *buf)
{
    u8 ucBuff[sizeof(u32)];
    u32 ulWords = 0;
    u32 ulOffsetStartBytes = 0;
    u32 ulOffsetEndBytes = 0;
    u32 ulReadBytesStart = 0;
    u32 ulReadBytesMid = 0;
    u32 ulReadBytesEnd = 0;
    u32 ulRemainBytes;

    ulRemainBytes = nBytes;
    if(0 != ulRemainBytes)
    {
        ulOffsetStartBytes = addr%sizeof(u32);
        if(0 != ulOffsetStartBytes)
        {
            ulReadBytesStart = sizeof(u32) - ulOffsetStartBytes;
            XTMP_peekVirtual(g_core[0], (u32 *)ucBuff, addr - ulOffsetStartBytes, 1);
            memcpy((void *)buf, (void *)(ucBuff+ulOffsetStartBytes), ulReadBytesStart);
            ulRemainBytes = nBytes - ulReadBytesStart;
        }
        else
        {
            ulReadBytesStart = 0;
        }
    }

    if(0 != ulRemainBytes)
    {
        ulWords = ulRemainBytes/sizeof(u32);
        if(0 != ulWords)
        {
            XTMP_peekVirtual(g_core[0], (u32 *)(buf + ulReadBytesStart), addr + ulReadBytesStart, ulWords);
            ulReadBytesMid = ulWords*sizeof(u32);
            ulRemainBytes = nBytes - ulReadBytesMid;
        }
    }

    if(0 != ulRemainBytes)
    {
        ulOffsetEndBytes = sizeof(u32) - ulRemainBytes;
        ulReadBytesEnd = sizeof(u32) - ulOffsetEndBytes;
        XTMP_peekVirtual(g_core[0], (u32 *)ucBuff, addr + ulReadBytesStart + ulReadBytesMid, 1);
        memcpy((void *)(buf + ulReadBytesStart + ulReadBytesMid), (void *)ucBuff, ulReadBytesEnd);
    }
}

void dramWriteByByte(u32 addr, u32 nBytes, u8 *buf)
{
    u8 ucBuff[sizeof(u32)];
    u32 ulWords = 0;
    u32 ulOffsetStartBytes = 0;
    u32 ulOffsetEndBytes = 0;
    u32 ulReadBytesStart = 0;
    u32 ulReadBytesMid = 0;
    u32 ulReadBytesEnd = 0;
    u32 ulRemainBytes;

    ulRemainBytes = nBytes;
    if(0 != ulRemainBytes)
    {
        ulOffsetStartBytes = addr%sizeof(u32);
        if(0 != ulOffsetStartBytes)
        {
            ulReadBytesStart = sizeof(u32) - ulOffsetStartBytes;
            if(ulRemainBytes <ulReadBytesStart)
            {
                ulReadBytesStart = ulRemainBytes;
            }
            XTMP_peekVirtual(g_core[0], (u32 *)ucBuff, addr - ulOffsetStartBytes, 1);
            memcpy((void *)(ucBuff+ulOffsetStartBytes), (void *)buf, ulReadBytesStart);
            XTMP_pokeVirtual(g_core[0], (u32 *)ucBuff, addr - ulOffsetStartBytes, 1);
            ulRemainBytes = nBytes - ulReadBytesStart;
        }
        else
        {
            ulReadBytesStart = 0;
        }
    }

    if(0 != ulRemainBytes)
    {
        ulWords = ulRemainBytes/sizeof(u32);
        if(0 != ulWords)
        {
            XTMP_pokeVirtual(g_core[0], (u32 *)(buf + ulReadBytesStart), addr + ulReadBytesStart, ulWords);
            ulReadBytesMid = ulWords*sizeof(u32);
            ulRemainBytes = nBytes - ulReadBytesMid;
        }
    }

    if(0 != ulRemainBytes)
    {
        ulOffsetEndBytes = sizeof(u32) - ulRemainBytes;
        ulReadBytesEnd = sizeof(u32) - ulOffsetEndBytes;
        XTMP_peekVirtual(g_core[0], (u32 *)ucBuff, addr + ulReadBytesStart + ulReadBytesMid, 1);
        memcpy((void *)ucBuff, (void *)(buf + ulReadBytesStart + ulReadBytesMid), ulReadBytesEnd);
        XTMP_pokeVirtual(g_core[0], (u32 *)ucBuff, addr + ulReadBytesStart + ulReadBytesMid, 1);
    }
}

//
//void dramRead(u32 addr, u32 nBytes, u8 *buf)
//{
//	sysmemFastReadLE(&sysDramData, buf, addr, nBytes);
//}
//void dramWrite(u32 addr, u32 nBytes, const u8* src)
//{
//	sysmemFastWriteLE(&sysDramData, addr, nBytes, src);
//}

/*
* Read data from the location corresponding to the requested address.
*/
XTMP_deviceStatus
    sysmemPeek(void *deviceData, u32 *dst, XTMP_address addr, u32 size)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;
    u32 *buf = getWordAddress(addr, sysMemData->pages);
    u32 sz = (size < 4) ? size : 4;
    u32 i;

    handleReadReg(addr, *(u32*)buf, size);

    for (i = 0; i < size; i += 4) {
        u32 val = XTMP_extract(buf++, addr+i, sz, 4, bigEndian);
        XTMP_insert(dst, addr+i, val, sz, pifWidth, bigEndian);
    }
    return XTMP_DEVICE_OK;
}

/*
* Write data into the location corresponding to the requested address.
*/
XTMP_deviceStatus
    sysmemPoke(void *deviceData, XTMP_address addr, u32 size, const u32 *src)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;
    u32 *buf = getWordAddress(addr, sysMemData->pages);
    u32 sz = (size < 4) ? size : 4;
    u32 i;
    bool bWriteThru = handleWriteReg(addr, *(u32*)src, size);

    if (bWriteThru)
    {
        for (i = 0; i < size; i += 4) {
            u32 val = XTMP_extract(src, addr+i, sz, pifWidth, bigEndian);
            XTMP_insert(buf++, addr+i, val, sz, 4, bigEndian);
        }
    }
    return XTMP_DEVICE_OK;
}

/*
* Device function for the "post" callback.
*/
XTMP_deviceStatus
    sysmemPost(void *deviceData, XTMP_deviceXfer *xfer)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;

    if (XTMP_xferIsRequest(xfer)) {
        if (sysMemData->xfer != NULL) {
            /*
            * Reject request if already processing another.  An XTMP_NACC 
            * response must be returned in the current cycle itself.
            */
            XTMP_respond(xfer, XTMP_NACC);
        }
        else {
            XTMP_time now = XTMP_clockTime(); /* current cycle */
            /*
            * Store the transfer record.  It will be processed later in the 
            * ticker function. 
            */
            sysMemData->xfer = xfer;

            /* Keep track of when the response is to be issued. */
            switch (XTMP_xferGetType(xfer)) {
            case XTMP_READ:
                sysMemData->responseTime = now + READ_DELAY;
                break;
            case XTMP_WRITE:
                sysMemData->responseTime = now + WRITE_DELAY;
                break;
            case XTMP_BLOCKREAD:
                /* A block read transaction has one request, but multiple responses. */
                sysMemData->responseTime = now + BLOCK_READ_DELAY;
                /* Initialize the transfer number. */
                sysMemData->transferNumber = 0;
                break;
            case XTMP_BLOCKWRITE:
                /*
                * The transfer number could potentially change, so the current 
                * transfer accepted must be noted down. 
                */
                sysMemData->transferNumber = XTMP_xferGetTransferNumber(xfer);
                /* A block write transaction has multiple requests and one response. */
                if (sysMemData->transferNumber == 0)
                    sysMemData->responseTime = now + BLOCK_WRITE_DELAY;
                else
                    sysMemData->responseTime = now + BLOCK_WRITE_REPEAT;
                break;
            default:
                return XTMP_DEVICE_ERROR;
            }
        }
        return XTMP_DEVICE_OK;
    }
    else {
        return XTMP_DEVICE_ERROR;
    }
}

/*
* This is the user thread function for the ticker thread.  It will wake 
* up on every cycle and respond to any requests that have matured. 
*/
u32 ulClock = 0;
void
    sysmemTicker(void *threadData)
{
    bool bRedirectFlag = false;
    u32 *pMcu0ResumeEntryPC;
    ulClock = (u32)XTMP_clockTime();
    while (!XTMP_haveAllCoresExited()) {	/* terminate if the core has exited  */

        SysMemData *sysMemData = (SysMemData *) threadData;
        XTMP_deviceXfer *xfer = sysMemData->xfer;

        /*
        * Check if there is a request being processed, and
        * if it is the right time to respond, if there is one. 
        */
        ulClock = (U32)XTMP_clockTime();

        if (xfer &&	(sysMemData->responseTime == XTMP_clockTime())) {

            XTMP_address address = XTMP_xferGetAddress(xfer);
            u32          size = XTMP_xferGetSize(xfer);
            u32         *data = XTMP_xferGetData(xfer);
            u32         *transferData;
            u8           transferNum;

            switch (XTMP_xferGetType(xfer)) {

            case XTMP_READ:
                /* Read data and respond. */
                sysmemPeek(sysMemData, data, address, size);
                XTMP_respond(xfer, XTMP_OK);
                sysMemData->xfer = NULL;
                break;

            case XTMP_WRITE:
                /* Write data and respond. */
                sysmemPoke(sysMemData, address, size, data);
                XTMP_respond(xfer, XTMP_OK);
                sysMemData->xfer = NULL;
                break;

            case XTMP_BLOCKREAD:
                transferNum = sysMemData->transferNumber++;
                XTMP_xferSetTransferNumber(xfer, transferNum);
                /* Identify the start address for this transfer. */
                address += transferNum * pifWidth;
                /* Locate the beginning of the data buffer for this transfer. */
                transferData = XTMP_xferGetBlockTransferData(xfer, transferNum);
                /* Read data into the buffer. */
                sysmemPeek(sysMemData, transferData, address, pifWidth);
                if (transferNum == size/pifWidth - 1) {
                    /* Done with transaction. */
                    XTMP_xferSetLastData(xfer, 1);
                    sysMemData->xfer = NULL;
                }
                else {
                    /*
                    * Transaction partially complete.  Last data indication must 
                    * be set to 0 and the next transfer must be scheduled.
                    */
                    XTMP_xferSetLastData(xfer, 0);
                    sysMemData->responseTime = XTMP_clockTime() + BLOCK_READ_REPEAT;
                }
                XTMP_respond(xfer, XTMP_OK);
                break;

            case XTMP_BLOCKWRITE:
                transferNum = sysMemData->transferNumber;
                /* Identify the start address for this transfer. */
                address += transferNum * pifWidth;
                /* Locate the beginning of the data buffer for this transfer. */
                transferData = XTMP_xferGetBlockTransferData(xfer, transferNum);
                /* Write data out of the buffer. */
                sysmemPoke(sysMemData, address, pifWidth, transferData);
                /*
                * Indicate that this transfer has been processed, so that the 
                * next transfer can be accepted. 
                */
                sysMemData->xfer = NULL;
                /*
                * There is only one response for block writes, at the end of the 
                * transaction. 
                */
                if (transferNum == size/pifWidth - 1)
                    XTMP_respond(xfer, XTMP_OK);
                break;

            default:
                fprintf(stderr, "Received unsupported transaction.\n");
                exit(1);
            }
        }

        /*core0 wakeup from rom when host model start to operate pxCI,SCAT...
          core0 suspend when g_ucSystemStsSuspendFlag 
        */
        switch(g_ucSystemStsSuspendFlag)
        {
        case SYSTEM_SUSPEND:
            {
                if(true == AHCI_IsHBARegActive())
                {
                    pMcu0ResumeEntryPC = (u32 *)GetVirtualAddrInLocalMem(PMU_RESUME_ENTRY_ADDRESS);
                    bRedirectFlag = XTMP_redirectPC(g_core[0], *pMcu0ResumeEntryPC);
                    if(true == bRedirectFlag)
                    {
                        g_ucSystemStsSuspendFlag = SYSTEM_RUNING;
                    }
                }
            }
            break;

        case SYSTEM_SUSPENDSTART:
            {
                *(u32 *)(g_pIsramBaseAddr[0] + ISRAM_REST_VECT_OFFSET) = DEAD_LOOP_INS;
                bRedirectFlag = XTMP_redirectPC(g_core[0], MCU_ISRAM_VECT_PC);
                if(true == bRedirectFlag)
                {
                    PmuClearIDsramOtfbData();
                    g_ucSystemStsSuspendFlag = SYSTEM_SUSPEND;
                }
            }
            break;
        default:
            break;
        }

        /* Wake up again in the next cycle. */
        XTMP_wait(1);
    }
}


/*
* Fast-access read callback for LittleEndian request and memory interface
*/
void
    sysmemFastReadLE(void *deviceData, u32 *dst, XTMP_address addr, u32 size)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;
    u32 *buf = getWordAddress(addr, sysMemData->pages);
    u32 i;
    switch(size) {
    case 1:
        dst[0] = XTMP_extract32LE8(buf, addr);
        break;
    case 2:
        dst[0] = XTMP_extract32LE16(buf, addr);
        break;
    case 4:
        dst[0] = XTMP_extract32LE32(buf, addr);
        break;
    default:
        for (i = 0; i < size/4; i++) {
            dst[i] = XTMP_extract32LE32(&buf[i], addr+i*4);
        }
        break;
    }
}

/*
* Fast-access write callback for LittleEndian request and memory interface
*/
void
    sysmemFastWriteLE(void *deviceData, XTMP_address addr, u32 size, const u32 *src)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;
    u32 *buf = getWordAddress(addr, sysMemData->pages);
    u32 i;
    switch(size) {
    case 1:
        XTMP_insert32LE8(buf, addr, src[0]);
        break;
    case 2:
        XTMP_insert32LE16(buf, addr, src[0]);
        break;
    case 4:
        XTMP_insert32LE32(buf, addr, src[0]);
        break;
    default: /* 8, 16 */
        for (i = 0; i < size/4; i++) {
            XTMP_insert32LE32(&buf[i], addr+i*4, src[i]);
        }
        break;
    }
}

/*
* Fast-access read callback for BigEndian request and memory interface
*/
void
    sysmemFastReadBE(void *deviceData, u32 *dst, XTMP_address addr, u32 size)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;
    u32 *buf = getWordAddress(addr, sysMemData->pages);
    u32 i;
    switch(size) {
    case 1:
        dst[0] = XTMP_extract32BE8(buf, addr);
        break;
    case 2:
        dst[0] = XTMP_extract32BE16(buf, addr);
        break;
    case 4:
        dst[0] = XTMP_extract32BE32(buf, addr);
        break;
    default:
        for (i = 0; i < size/4; i++) {
            u32 dst_word = size/4 - i - 1;
            dst[dst_word] = XTMP_extract32BE32(&buf[i], addr+i*4);
        }
        break;
    }
}

/*
* Fast-access write callback for BigEndian request and memory interface
*/
void
    sysmemFastWriteBE(void *deviceData, XTMP_address addr, u32 size, const u32 *src)
{
    SysMemData *sysMemData = (SysMemData *) deviceData;
    u32 *buf = getWordAddress(addr, sysMemData->pages);
    u32 i;
    switch(size) {
    case 1:
        XTMP_insert32BE8(buf, addr, src[0]);
        break;
    case 2:
        XTMP_insert32BE16(buf, addr, src[0]);
        break;
    case 4:
        XTMP_insert32BE32(buf, addr, src[0]);
        break;
    default: /* 8, 16 */
        for (i = 0; i < size/4; i++) {
            u32 src_word = size/4 - i - 1;
            XTMP_insert32BE32(&buf[i], addr+i*4, src[src_word]);
        }
        break;
    }
}

/* 
* Fast-access request handler
*/
XTMP_deviceStatus
    sysmemFastAccess(void *deviceData,
    XTMP_fastAccessRequest request,
    XTMP_address addr)
{
    /* Chose the fast-access data transfer method */
#if !defined(FAST_ACCESS_RAW) && !defined(FAST_ACCESS_CALLBACKS) && !defined(FAST_ACCESS_PEEKPOKE)
#define FAST_ACCESS_RAW       1
#define FAST_ACCESS_CALLBACKS 0
#define FAST_ACCESS_PEEKPOKE  0
#endif

    if (FAST_ACCESS_RAW) {
        SysMemData *sysMemData = (SysMemData *) deviceData;
        u32 pageStart = addr & ~PAGE_MASK;
        XTMP_setFastAccessRaw(request, addr, pageStart, pageStart+PAGE_SIZE-1,
            getWordAddress(pageStart, sysMemData->pages),
            bigEndian ? 3 : 0 /* swizzle */);
    }
    else if (FAST_ACCESS_CALLBACKS) {
        if (bigEndian) {
            XTMP_setFastAccessCallBacks(request, addr, 0, 0xffffffff,
                sysmemFastReadBE, sysmemFastWriteBE, deviceData);
        } else {
            XTMP_setFastAccessCallBacks(request, addr, 0, 0xffffffff,
                sysmemFastReadLE, sysmemFastWriteLE, deviceData);
        }
    }
    else if (FAST_ACCESS_PEEKPOKE) {
        XTMP_setFastAccessPeekPoke(request, addr, 0, 0xffffffff);
    }
    else {
        XTMP_denyFastAccess(request, addr, 0, 0xffffffff);    
    }
    return XTMP_DEVICE_OK;
}



