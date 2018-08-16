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
 * File Name    : Uart.c
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "uart.h"
#include  <stdarg.h>
#include "BaseDef.h"
#include "HAL_MultiCore.h"

U32 uart_init(void)
{
    rUART_LCR = 0;
#ifndef ASIC
    rUART_LCR = 0xCF0000;
#else
    rUART_LCR = 0x1200000;
#endif
    rUART_ENABLE |= 1 << 12;
    
    return 0;
}

U32 uart_send_str(const U8 *str)
{
    U32 i = 0;

    while (str[i] != 0) {
        rUART_TXR = str[i++];
    }

    return i;
}

U8 uart_getchar(void)
{
    U8 c;
    while((rUART_LCR & 0x3e000000) == 0);

    c = rUART_RXR;

    return c;
}

void uart_putchar(U8 ch)
{
    rUART_TXR = ch;
}
/*
U32 uart_download( U8 *downloadaddr)
{
U16 checksum;
U32 filesize;
U8 *buf;
U32 recvlen;
U16 temp;

buf = downloadaddr;

recvlen = 4;

while(recvlen--)
*buf++  = uart_getchar();

buf = downloadaddr;

filesize = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];

recvlen = filesize-4;

while(recvlen--)
*buf++ = uart_getchar();

checksum = *(buf-1) | *(buf-2)<<8;

temp = 0;

filesize-=6;

buf = downloadaddr;
while(filesize--)
temp += *buf++;

if(temp != checksum)
return 0;

return 1;
}
*/
//static char *itoa(int value, char *string, int radix)
char *my_itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    if (value < 0)
    {
        *ptr++ = '-';

        value *= -1;
    }

    for (i = 100000000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    *ptr = 0;

    return string;
}

//static const char mc[]="0123456789abcdef";
char mc[]="0123456789abcdef";
//static char *utohex(unsigned int value, char *string, int radix)
char *utohex(unsigned int value, char *string, int radix)
{
    int i,p;
    int oupt;
    oupt = 0;
    for (i=0;i<32;i+=4)
    {
        p = (value >> (28 - i))&0xf;
        string[oupt++]=mc[p];
    }

    string[oupt] = (char)NULL;

    return string;
}

//static void dbg_putchar(char c)
void dbg_putchar(char c)
{
    //  uart_putchar(c);
    rUART_TXR = c;
}
#if defined(FPGA)
U8 g_printf_flag;
#endif
void dbg_printf(const char *fmt, ...)
{
    //return;
    const char *s;
    int d;
    char buf[16];
    va_list ap;

    HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SUBSYS_PRINT);

    va_start(ap, fmt);
    while (*fmt) {
        if (*fmt != '%') {
            dbg_putchar(*fmt++);
            continue;
        }
        switch (*++fmt) {
case 's':
    s = va_arg(ap, const char *);
    for ( ; *s; s++) {
        //  dbg_putchar(*s);    
        rUART_TXR = *s;
    }
    break;
case 'd':
    d = va_arg(ap, int);
    my_itoa(d, buf, 10);
    for (s = buf; *s; s++) {
        //  dbg_putchar(*s);
        rUART_TXR = *s;
    }
    break;
case 'x':
    d = va_arg(ap, int);
    utohex(d, buf, 16);
    for (s = buf; *s; s++) {
        rUART_TXR = *s;
        //  dbg_putchar(*s);
    }
    break;

default:
    rUART_TXR = *fmt;
    //dbg_putchar(*fmt);
    break;
        }
        fmt++;
    }
    va_end(ap);

    HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SUBSYS_PRINT);
}



