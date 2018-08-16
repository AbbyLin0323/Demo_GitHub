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
*******************************************************************************
* File Name    : memory_access.c
* Discription  :
* CreateAuthor : HavenYang
* CreateDate   : 2014.6.18
*===============================================================================
* Modify Record:
*=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "memory_access.h"
#include "HAL_MemoryMap.h"
#include "single_list.h"

typedef struct COMM_MEMORY_SPACE
{
    SL_LISTPTR pObservers;
} COMM_MEM_SPACE;

typedef struct COMM_MEMORY_OBJECT
{
    COMM_MEM_SPACE tDram;
    COMM_MEM_SPACE tOtfb;
} COMM_MEM_OBJECT;

LOCAL COMM_MEM_OBJECT l_tMemory;

void Comm_IMemObjectInit(void)
{
    SL_RESULT eResult = Sl_ListCreate(&(l_tMemory.tDram.pObservers));
    if (Sl_kBadAlloc == eResult)
    {
        DBG_Printf("No available memory to allocate!\n");
        DBG_Getch();
    }
}

void Comm_IRegisterDramObserver(COMM_DRAM_DATA_CAPTURER pDramDataCapturer)
{
    SL_RESULT eResult = Sl_PushBack(l_tMemory.tDram.pObservers, pDramDataCapturer);
    if (Sl_kBadAlloc == eResult)
    {
        DBG_Printf("No available memory to allocate!\n");
        DBG_Getch();
    }
}

/*------------------------------------------------------------------------------
function: void Comm_ReadDram(U32 addr, U32 nWords, U32 *buf);
Description:
    read data form addr+DRAM_START_ADDRESS to buf
Input Param:
    U32 addr: source offset addr to DRAM BASE
    U32 nWords: copy num
    U32 *buf: destination addr
Output Param:
    none
Return Value:
    void
Usage:
    when write copy data form DRAM to local buffer in NFC model
History:
------------------------------------------------------------------------------*/
void Comm_ReadDram(U32 addr, U32 nWords, U32 *buf)
{
    U32 i = 0;
    U32 *pSrc = (U32 *)(addr + SIM_DRAM_BASE);
    U32 *pDst = buf;
    for (i = 0; i < nWords; i++)
    {
        *pDst++ = *pSrc++;
    }

    // Pass data to all observers.
    for (SL_ITERATOR tIterator = Sl_Begin(l_tMemory.tDram.pObservers);
         tIterator != Sl_End(l_tMemory.tDram.pObservers); Sl_Forward(&tIterator))
    {
        COMM_DRAM_DATA_CAPTURER pDramDataCapturer = (COMM_DRAM_DATA_CAPTURER)Sl_GetContent(tIterator);
        pDramDataCapturer((U32 *)(addr + SIM_DRAM_BASE), nWords);
    }
    return;
}

/*------------------------------------------------------------------------------
function: void Comm_WriteDram(U32 addr, U32 nWords, U32 *buf);
Description:
    write data to addr+DRAM_START_ADDRESS
Input Param:
    U32 addr: destination offset addr to DRAM BASE
    U32 nWords: copy num
    U32 *buf: source addr
Output Param:
    none
Return Value:
    void
Usage:
    when read write data to DRAM form local buffer in NFC model
History:
------------------------------------------------------------------------------*/
void Comm_WriteDram(U32 addr, U32 nWords, U32 *buf)
{
    U32 i = 0;
    U32 *pDst = (U32 *)(addr + SIM_DRAM_BASE);
    U32 *pSrc = buf;
    for (i = 0; i < nWords; i++)
    {
        *pDst++ = *pSrc++;
    }
    return;
}


/*------------------------------------------------------------------------------
function: void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf);
Description:
    write data to addr+DRAM_START_ADDRESS
Input Param:
    U32 addr: destination offset addr to DRAM BASE
    U32 nBytes: copy num by bytes.
    U32 *buf: source addr
Output Param:
    none
Return Value:
    void
Usage:
    when read write data to DRAM form local buffer in NFC model
History:
------------------------------------------------------------------------------*/
void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf)
{
    U32 i = 0;
    U8 *pDst = (U8 *)(addr + SIM_DRAM_BASE);
    U8 *pSrc = buf;
    for(i = 0; i < nBytes; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*------------------------------------------------------------------------------
function: void Comm_ReadOtfb(U32 addr, U32 nWords, U32 *buf);
Description:
    read data form addr+OTFB_START_ADDRESS to buf
Input Param:
    U32 addr: source offset addr to OTFB BASE
    U32 nWords: copy num
    U32 *buf: destination addr
Output Param:
    none
Return Value:
    void
Usage:
    when write copy data form OTFB to local buffer in NFC model
History:
------------------------------------------------------------------------------*/
void Comm_ReadOtfb(U32 addr, U32 nWords, U32 *buf)
{
    U32 i = 0;
    U32 *pSrc = (U32 *)(addr + SIM_OTFB_BASE);
    U32 *pDst = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*------------------------------------------------------------------------------
function: void Comm_WriteOtfb(U32 addr, U32 nWords, U32 *buf);
Description:
    write data to addr+OTFB_START_ADDRESS
Input Param:
    U32 addr: destination offset addr to OTFB BASE
    U32 nWords: copy num
    U32 *buf: source addr
Output Param:
    none
Return Value:
    void
Usage:
    when read write data to OTFB form local buffer in NFC model
History:
------------------------------------------------------------------------------*/
void Comm_WriteOtfb(U32 addr, U32 nWords, U32 *buf)
{
    U32 i = 0;
    U32 *pDst = (U32 *)(addr + SIM_OTFB_BASE);
    U32 *pSrc = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*------------------------------------------------------------------------------
function: void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf);
Description:
    write data to addr+OTFB_START_ADDRESS
Input Param:
    U32 addr: destination offset addr to OTFB BASE
    U32 nBytes: copy num by bytes.
    U8 *buf: source addr
Output Param:
    none
Return Value:
    void
Usage:
    when read write data to OTFB form local buffer in NFC model
History:
------------------------------------------------------------------------------*/
void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf)
{
    U32 i = 0;
    U8 *pDst = (U8 *)(addr + SIM_OTFB_BASE);
    U8 *pSrc = buf;
    for(i = 0; i < nBytes; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*------------------------------------------------------------------------------
function: void Comm_ReadReg(U32 addr, U32 nWords, U32 *buf);
Description:
    read data form reg to buf
Input Param:
    U32 addr: reg addr
    U32 nWords: copy num
    U32 *buf: destination addr
Output Param:
    none
Return Value:
    void
Usage:
    when read reg
History:
------------------------------------------------------------------------------*/
void Comm_ReadReg(U32 addr, U32 nWords, U32 *buf)
{
    U32 i = 0;
    U32 *pSrc = (U32 *)(addr);
    U32 *pDst = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*------------------------------------------------------------------------------
function: void Comm_WriteReg(U32 addr, U32 nWords, U32 *buf);
Description:
    write data to reg
Input Param:
    U32 addr: destination reg addr
    U32 nWords: copy num
    U32 *buf: reg addr
Output Param:
    none
Return Value:
    void
Usage:
    when write reg
History:
------------------------------------------------------------------------------*/
void Comm_WriteReg(U32 addr, U32 nWords, U32 *buf)
{
    U32 i = 0;
    U32 *pDst = (U32 *)(addr);
    U32 *pSrc = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*------------------------------------------------------------------------------
function: Comm_WriteRegByByte
Description:
    write data to reg by byte unit
Input Param:
    U32 ulAddr: destination reg addr
    U32 ulBytes: number of bytes to write
    U8 *pRegVal: pointer to register value
Output Param:
    none
Return Value:
    void
Usage:
    update register whose address is byte aligned
History:
20141218   Gavin  Create
------------------------------------------------------------------------------*/
void Comm_WriteRegByByte(U32 ulAddr, U32 ulBytes, U8 *pRegVal)
{
    U32 i = 0;
    U8 *pDst = (U8 *)(ulAddr);
    U8 *pSrc = pRegVal;
    for(i = 0; i < ulBytes; i++){
        *pDst++ = *pSrc++;
    }
    return;
}

/*====================End of this file========================================*/

