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
* File Name    : memory_access.h
* Discription  :
* CreateAuthor : HavenYang
* CreateDate   : 2014.6.18
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _MEMORY_ACCESS_H
#define _MEMORY_ACCESS_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

typedef void (* COMM_DRAM_DATA_CAPTURER)(U32 *dram_data, U32 size_in_dw);

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/

void Comm_IMemObjectInit(void);
void Comm_IRegisterDramObserver(COMM_DRAM_DATA_CAPTURER pDramDataCapturer);

void Comm_ReadDram(U32 addr, U32 nWords, U32 *buf);
void Comm_WriteDram(U32 addr, U32 nWords, U32 *buf);
void Comm_WriteDramByByte(U32 addr, U32 nBytes, U8 *buf);

void Comm_ReadOtfb(U32 addr, U32 nWords, U32 *buf);
void Comm_WriteOtfb(U32 addr, U32 nWords, U32 *buf);
void Comm_WriteOtfbByByte(U32 addr, U32 nBytes, U8 *buf);

void Comm_ReadReg(U32 addr, U32 nWords, U32 *buf);
void Comm_WriteReg(U32 addr, U32 nWords, U32 *buf);
void Comm_WriteRegByByte(U32 ulAddr, U32 ulBytes, U8 *pRegVal);

#endif
/*====================End of this head file===================================*/

