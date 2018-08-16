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
 * File Name    : Uart.h
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#ifndef OPENRISC_UART_H
#define OPENRISC_UART_H
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define UART_BASE REG_BASE_UART//0x7000

#define rUART_TXR_TRACE (*((volatile U32*) (UART_BASE + 0x4)))
#define rUART_ENABLE (*((volatile U32*) (REG_BASE + 0x68)))

#define rUART_TXR (*((volatile U8*) (UART_BASE + 0x4)))
#define rUART_RXR (*((volatile U8*) (UART_BASE + 0x8)))
#define rUART_LCR (*((volatile  U32*) (UART_BASE + 0xc)))
#define rUART_ICR (*((volatile  U32*) (UART_BASE + 0x10)))

void dbg_putchar(char c);
char *utohex(unsigned int value, char *string, int radix);
char *my_itoa(int value, char *string, int radix);

U32   uart_init(void);
U32   uart_send_str(const U8 *str);
U8    uart_getchar(void);
void  uart_putchar(U8 ch);
U32   uart_download(U8 *downloadaddr);
void dbg_printf(const char *fmt, ...);

#endif

