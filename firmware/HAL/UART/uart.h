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

#define UART_BASE       UART_REG_BASE//0x7000

#define rUART_TXR_TRACE (*((volatile U32*) (UART_BASE + 0x4)))
#define rUART_ENABLE (*((volatile U32*) (REG_BASE + 0x68)))

#define rUART_TXR (*((volatile U8*) (UART_BASE + 0x4)))
#define rUART_RXR (*((volatile U8*) (UART_BASE + 0x8)))

//UART REG04    TX Buffer depth 16 byte
//31:0    RW    0    Write data to be sent to this register    UART_TXDATA
#define rUART_TXR_DW (*((volatile U32*) (UART_BASE + 0x4)))


//UART REG08    RX Buffer depth 16 byte
//31:0    RW    0    Read received data from this register    UART_RXDATA
#define rUART_RXR_DW (*((volatile U32*) (UART_BASE + 0x8)))

/* UART REG0C
31:16    RW    0   Configure the baud rate:                                UART_RATE_MOD
                When the UART baud rate and internal CLK is defined,
                count how many clocks will one UART bit lasts
15:14    RW    0   Set the frame parity mode:                              UART_PAR_SEL
               2'b00: no parity bit in the frame
               2'b01: odd parity bit in the frame
               2'b10: even parity bit in the frame
13       RW    0   Set the stop bit number :                               UART_FRAM_MOD
               1'b0: one stop bit in the frame
               1'b1: two stop bit in the frame
12       RW    0   Set UART interface mode:                                UART_MOD
               1'b0: without two handshake signals
               1'b1: with two handshake signals as RTS_n and CTS_n
11       RW    0   Set bit order:                                          MSBMODE
               1'b0: MSB last
               1'b1: MSB first
10       RW    0   Set read mode:                                          READ_BYTE_MODE
               1'b0: read 1 time, pop 4 bytes
               1'b1: read 1 time, pop 1 byte
9        RW    0   Set FIFO read trigger mode                              READ_TRIG_MODE
               1'b0: read can occurred only when there are
                at least 4 bytes data in the RXFIFO
               1'b1: read can occurred as long as RXFIFO is not empty
8        RW    0   Set working mode:                                       UART_SPI_SEL
               1'b0: UART work
               1'b1: SPI work
7        RW    0   Error interrupt mask:                                   ERR_MASK
               1'b0: disable
               1'b1: enable
6        RW    0   Read interrupt mask                                     READ_MASK
               1'b0: disable
               1'b1: enable
5:1      RW    0   Show current RXFIFO depth                                RX_FIFO_CNT
0        RW    0   Set the behavior when RXFIFO overflow                   RX_OVERFLOW_C
               1'b0: don't receive
               1'b1: clear first byte in FIFO
*/

#define rUART_LCR (*((volatile  U32*) (UART_BASE + 0xc)))
typedef struct UART_LCR_REG
{
    U32 RX_OVERFLOW_C  :1;
    U32 RX_FIFO_CNT    :5;
    U32 READ_MASK      :1;
    U32 ERR_MASK       :1;
    U32 UART_SPI_SEL   :1;
    U32 READ_TRIG_MODE :1;
    U32 READ_BYTE_MODE :1;
    U32 MSBMODE        :1;
    U32 UART_MOD       :1;
    U32 UART_FRAM_MOD  :1;
    U32 UART_PAR_SEL   :2;
    U32 UART_RATE_MOD  :16;
}UART_LCR_REG;
/*  UART REG10
31        RW1C    0   1'b0: parity ok                                     UART_PAR_ERR
                      1'b1: when a parity error is
                    detected by serial receive block
30        RW1C    0   1'b0: frame ok                                      UART_FRA_ERR
                      1'b1: when a wrong stop bit is
                    detected by serial receive block
29        RW1C    0   1'b0: external device response normally             UART_TIME_OUT
                      1'b1: when APB master transmit,
                    the external device doesn't response for a long time
28        RW1C    0   1'b0: RXFIFO normally                               RXFIFO_FULL_ERR
                      1'b1: RXFIFO overflow
27        RW1C    0   1'b0: there is not enough data in memory            READ_MEM_FULL
                      1'b1: APB master can read now
26:16     RO      0   reserved
15:0      RW      0   reserved
*/
#define rUART_ICR (*((volatile  U32*) (UART_BASE + 0x10)))
#define BIT_UART_READ_MEM_FULL (1 << 27)
#define BIT_UART_RXFIFO_FULL_ERR (1 << 28)

#define DBG_HEADER_SIZE (0x10) //byte
#define WPTR_OFFSET (0x0)
#define RPTR_OFFSET (0x4)
#define BUF_SIZE_OFFSET (0x8)
#define DRAM_CACHEABLE_OFFSET (0x40000000)
#define SHIFT_TO_5_BYTE (20)
#define SHIFT_TO_6_BYTE (24)
#define DATA_SIZE (0x7FF0) //32KB - 16 byte = 32752

#define PRINT_TO_NUL  0
#define PRINT_TO_UART 1
#define PRINT_TO_DRAM 2

extern GLOBAL BOOL IsMCU0init;
extern GLOBAL BOOL IsMCU1init;
extern GLOBAL BOOL IsMCU2init;

/* global variable for print to memory and Uart MP mode */
extern GLOBAL BOOL bPrinttoDRAM;
extern GLOBAL U8 g_ucDATATransferMode;

/* print to memeory  */
void TracelogInit(void);


void dbg_putchar(char c);
char *utohex(unsigned int value, char *string, int radix);
char *my_itoa(int value, char *string, int radix);

INLINE U32   uart_init(void);
U32   uart_send_str(const U8 *str);
U8    uart_getchar(void);
void  uart_putchar(U8 ch);
U32   uart_download(U8 *downloadaddr);
void dbg_printf(const char *fmt, ...);
BOOL HAL_UartIsMp(void);
void HAL_UartMpMode(BOOL);
U32 HAL_UartDmaTx(U32* pSrc,U32 ulLengthDW);
U32 HAL_UartDmaRx(U32* pDes,U32 ulLengthDW);
void HAL_ClearRxValid(void);
BOOL HAL_CheckRxValid(void);
U8 HAL_UartRxByte(void);
void HAL_UartTxByte(U8 ucData);
U32 HAL_UartRxDW(void);
void HAL_UartTxDW(U32 ulData);
U32 DumpStack(void);
void dbg_putchar_init(void);

#ifndef SIM
void dbg_tracelog_MCU0(U32 uc_Lable);
void dbg_tracelog_MCU1(U32 uc_Lable);
void dbg_tracelog_MCU2(U32 uc_Lable);
#else
#define dbg_tracelog_MCU0(uc_Lable)
#define dbg_tracelog_MCU1(uc_Lable)
#define dbg_tracelog_MCU2(uc_Lable)
#endif

#ifndef SIM
void dbg_getrealcycle_MCU0(U32 uc_Lable, U32 ulCycle);
void dbg_getrealcycle_MCU1(U32 uc_Lable, U32 ulCycle);
void dbg_getrealcycle_MCU2(U32 uc_Lable, U32 ulCycle);
#else
#define dbg_getrealcycle_MCU0(uc_Lable, U32 ulCycle)
#define dbg_getrealcycle_MCU1(uc_Lable, U32 ulCycle)
#define dbg_getrealcycle_MCU2(uc_Lable, U32 ulCycle)
#endif


#endif

