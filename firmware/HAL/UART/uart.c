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
#include "HAL_ParamTable.h"
#include "HAL_Xtensa.h"
#include <xtensa/tie/xt_timer.h>

//LOCAL BOOL l_bUartMp;
GLOBAL BOOL bPrinttoDRAM;
GLOBAL U8 g_ucDATATransferMode;

extern void dbg_putchar_dram(char c);
BOOL HAL_UartIsMp(void)
{
    PTABLE *pPT;

    pPT = (PTABLE*)HAL_GetPTableAddr();

    return (TRUE == pPT->sBootStaticFlag.bsUartMpMode);
}

void HAL_UartMpMode(BOOL bEnabled)
{
    PTABLE *pPT;

    pPT = (PTABLE*)HAL_GetPTableAddr();
    pPT->sBootStaticFlag.bsUartMpMode = bEnabled;
}

void HAL_UartTxDW(U32 ulData)
{
    rUART_TXR_DW = ulData;
}

U32 HAL_UartRxDW(void)
{
    U32 ulBuf = rUART_RXR_DW;
    return ulBuf;
}

void HAL_UartTxByte(U8 ucData)
{
    rUART_TXR = ucData;
}

U8 HAL_UartRxByte(void)
{
    U8 ucBuf = rUART_RXR;
    return ucBuf;
}

BOOL HAL_CheckRxValid(void)
{
    if(rUART_ICR & BIT_UART_READ_MEM_FULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void HAL_ClearRxValid(void)
{
    rUART_ICR = BIT_UART_READ_MEM_FULL;
}

U32 HAL_UartDmaRx(U32* pDes,U32 ulLengthDW)
{
    U32 i,checksum,ulData;
    U32 *pAddr = pDes;
    for(i=0,checksum=0;i<ulLengthDW;)
    {
        if(TRUE == HAL_CheckRxValid())
        {
            ulData = HAL_UartRxDW();
            *pAddr = ulData;
            checksum ^= ulData;
            HAL_ClearRxValid();
            pAddr++;
            i++;
        }
    }
    return checksum;
}

U32 HAL_UartDmaTx(U32* pSrc,U32 ulLengthDW)
{
    U32 i,checksum,ulData;
    U32 *pAddr = pSrc;
    for (i=0,checksum=0;i<ulLengthDW;i++,pAddr++)
    {
        ulData = *pAddr;
        HAL_UartTxDW(ulData);
        checksum ^= ulData;
    }
    return checksum;
}

INLINE U32 uart_init(void)
{
    U32 ulHPLL;
    rUART_LCR = 0;
#ifndef ASIC
    rUART_LCR = 0xCF0000;
#else
    ulHPLL = PLL(rPMU(0x68) >> 16);
    if (TRUE == HAL_IsBaudRate12MEnable())
    {
        rUART_ENABLE &= ~(1 << 12);
        if (ulHPLL != 287)
        {
            ulHPLL = rPMU(0x68);
            ulHPLL &= 0xFFFF;
            ulHPLL |= 0x2160000;
            rPMU(0x68) = ulHPLL;
        }
        rUART_LCR = 0x20000;
        rUART_ENABLE |= 1 << 12;
        return;
    }
    if(TRUE == HAL_UartIsMp())
    {
        if (ulHPLL == 300)
            rUART_LCR = 0x1440000;
        else if (ulHPLL == 287)
            rUART_LCR = 0x1360000;
        else if (ulHPLL == 337)
            rUART_LCR = 0x16D0000;
        else
           rUART_LCR = 0x1440000;
    }
    else
    {
        if (ulHPLL == 300)
            rUART_LCR = 0x1440000;
        else if (ulHPLL == 287)
            rUART_LCR = 0x1360000;
        else if (ulHPLL == 337)
            rUART_LCR = 0x16D0000;
        else
            rUART_LCR = 0x1440000;
    }
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
    while((rUART_LCR & 0x3e) == 0);

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
#if 0
    for (i=0;i<32;i+=4)
    {
        p = (value >> (28 - i))&0xf;
        string[oupt++]=mc[p];
    }
#else
    i = 0;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
    i += 4;
    p = (value >> (28 - i))&0xf;
    string[oupt++]=mc[p];
#endif

    string[oupt] = (char)NULL;

    return string;
}

//static void dbg_putchar(char c)
void dbg_putchar(char c)
{
    //  uart_putchar(c);
    rUART_TXR = c;
}

void dbg_printf(const char *fmt, ...)
{
    const char *s;
    int d;
    char buf[16];
    char ch;
    unsigned print_to_position;
    va_list ap;

    if (FALSE != bPrinttoDRAM)
    {
        print_to_position = PRINT_TO_DRAM;
    }

    else if (FALSE == HAL_UartIsMp())
    {
        print_to_position = PRINT_TO_UART;
    }

    else
    {
        print_to_position = PRINT_TO_NUL;
    }

    if (PRINT_TO_NUL != print_to_position)
    {
        HAL_MultiCoreGetSpinLockWait(SPINLOCKID_SUBSYS_PRINT);
        va_start(ap, fmt);
        while (*fmt)
        {
            if (*fmt != '%')
            {
                if (PRINT_TO_DRAM == print_to_position)
                {
                    /* print to memory */
                    dbg_putchar_dram(*fmt);
                    *fmt++;
                }

                else
                {
                    /* print to UART */
                    dbg_putchar(*fmt++);
                }

                continue;
            }

            switch (*++fmt)
            {
                case 's':
                    s = va_arg(ap, const char *);
                    for ( ; *s; s++)
                    {
                        if (PRINT_TO_DRAM == print_to_position)
                        {
                            /* print to memory */
                            dbg_putchar_dram(*s);
                        }

                        else
                        {
                            /* print to UART */
                            rUART_TXR = *s;
                        }
                    }
                    break;

                case 'd':
                    d = va_arg(ap, int);
                    my_itoa(d, buf, 10);
                    for (s = buf; *s; s++)
                    {
                        if (PRINT_TO_DRAM == print_to_position)
                        {
                            /* print to memory */
                            dbg_putchar_dram(*s);
                        }

                        else
                        {
                            /* print to UART */
                            rUART_TXR = *s;
                        }
                    }
                    break;

                case 'x':
                    d = va_arg(ap, int);
                    utohex(d, buf, 16);
                    for (s = buf; *s; s++)
                    {
                        if (PRINT_TO_DRAM == print_to_position)
                        {
                            /* print to memory */
                            dbg_putchar_dram(*s);
                        }

                        else
                        {
                            /* print to UART */
                            rUART_TXR = *s;
                        }
                    }
                    break;

                case 'c':
                    ch = (char)va_arg(ap, int);
                    if (PRINT_TO_DRAM == print_to_position)
                    {
                        dbg_putchar_dram(ch);
                    }

                    else
                    {
                        rUART_TXR = ch;
                    }
                    break;

                default:
                    if (PRINT_TO_DRAM == print_to_position)
                    {
                        /* print to memory */
                        dbg_putchar_dram(*fmt);
                    }

                    else
                    {
                        rUART_TXR = *fmt;
                    }
                    break;
            }
            fmt++;
        }
        va_end(ap);
        HAL_MultiCoreReleaseSpinLock(SPINLOCKID_SUBSYS_PRINT);
    }

    return;
}



