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
#ifdef SIM_XTENSA
#include "xtmp_localmem.h"
#endif
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* extern region: extern global variable & function prototype                 */
/*============================================================================*/

/*============================================================================*/
/* global region: declare global variable                                     */
/*============================================================================*/

/*============================================================================*/
/* local region:  declare local variable & local function prototype           */
/*============================================================================*/

/*============================================================================*/
/* main code region: function implement                                       */
/*============================================================================*/



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
#if defined(SIM_XTENSA)
    dramRead(DRAM_START_ADDRESS+addr, nWords, buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U32 *pSrc = (U32 *)(addr + SIM_DRAM_BASE);
    U32 *pDst = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    dramWrite(DRAM_START_ADDRESS+addr, nWords, buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U32 *pDst = (U32 *)(addr + SIM_DRAM_BASE);
    U32 *pSrc = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    //dramWrite(DRAM_START_ADDRESS+addr, nWords, buf);
    dramWriteByByte(DRAM_START_ADDRESS+addr, nBytes, buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U8 *pDst = (U8 *)(addr + SIM_DRAM_BASE);
    U8 *pSrc = buf;
    for(i = 0; i < nBytes; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    dramRead(OTFB_START_ADDRESS+addr, nWords, buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U32 *pSrc = (U32 *)(addr + SIM_OTFB_BASE);
    U32 *pDst = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    dramWrite(OTFB_START_ADDRESS+addr, nWords, buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U32 *pDst = (U32 *)(addr + SIM_OTFB_BASE);
    U32 *pSrc = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    //dramWrite(OTFB_START_ADDRESS+addr, nWords, buf);
    dramWriteByByte(OTFB_START_ADDRESS+addr, nBytes, buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U8 *pDst = (U8 *)(addr + SIM_OTFB_BASE);
    U8 *pSrc = buf;
    for(i = 0; i < nBytes; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    regRead(addr, nWords<<2, (U8 *)buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U32 *pSrc = (U32 *)(addr);
    U32 *pDst = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    regWrite(addr, nWords<<2, (U8 *)buf);
    return;
#elif defined(SIM)
    U32 i = 0;
    U32 *pDst = (U32 *)(addr);
    U32 *pSrc = buf;
    for(i = 0; i < nWords; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
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
#if defined(SIM_XTENSA)
    regWrite(ulAddr, ulBytes, pRegVal);
    return;
#elif defined(SIM)
    U32 i = 0;
    U8 *pDst = (U8 *)(ulAddr);
    U8 *pSrc = pRegVal;
    for(i = 0; i < ulBytes; i++){
        *pDst++ = *pSrc++;
    }
    return;
#endif
}

/*====================End of this file========================================*/

