/*************************************************
Copyright (c) 2009 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :  BaseDef.h                                           
Version      :  v0.1                                                
Date         :  2008/5/21                                           
Author       :  Sue Wang

Description: A basic header file includes UBS and NFC interface functions.
Others: 
Modification History:
20080603    JackeyChai    001: Add define of GLOBAL MCU12_VAR_ATTR and LOCAL
20080617     peterlgchen    001: add define S32, S16, S8 for signed data
20081126    peterlgchen     002: Change File Name to BaseDef.h
20140902    Victor Zhang    Modification as new coding style 
20141029    Gavin Yin       update interrupt source&mask definition
*************************************************/

#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "BaseDef.h"
#include "Proj_Config.h"
#include "HAL_MemoryMap.h"

#define EXCCAUSE_LEVEL1_INTERRUPT 4

/* define interrupt source bit in INTERRUPT register of MCU */
#define BIT_ORINT_HTDMAC (1 << 0)
#define BIT_ORINT_UART   (1 << 1)
#define BIT_ORINT_GPIO   (1 << 2)
#define BIT_ORINT_SPI    (1 << 3)
#define BIT_ORINT_DMAE   (1 << 4)
#define BIT_ORINT_MCU1_0   (1 << 5) // MCU1 -> MCU0 interrupt
#define BIT_ORINT_MCU0_1   (1 << 5)
#define BIT_ORINT_MCU0_2   (1 << 5)
#define BIT_ORINT_TIMER0 (1 << 6)
#define BIT_ORINT_TIMER1 (1 << 8)
#define BIT_ORINT_SDC    (1 << 9)
#define BIT_ORINT_NFC    (1 << 10)

#ifdef VT3514_C0
#define BIT_ORINT_PMU     (1 << 13)
#define BIT_ORINT_HOSTC   (1 << 14)
#define BIT_ORINT_BUFFMAP (1 << 15)
#define BIT_ORINT_TIMER2  (1 << 16)
#define BIT_ORINT_MCU2_0    (1 << 17)  // MCU2 -> MCU0 interrupt
#define BIT_ORINT_MCU2_1    (1 << 17)
#define BIT_ORINT_MCU1_2    (1 << 17)
#define BIT_ORINT_LDPC    (1 << 18)
#define BIT_ORINT_PCIE    (1 << 19)
#else // A0, B0
#define BIT_ORINT_PMU     (1 << 12)
#define BIT_ORINT_HOSTC   (1 << 13)
#define BIT_ORINT_BUFFMAP (1 << 14)
#define BIT_ORINT_MCU2_0    (1 << 15) // MCU2 -> MCU0 interrupt
#define BIT_ORINT_MCU2_1    (1 << 15)
#define BIT_ORINT_MCU1_2    (1 << 15)
#define BIT_ORINT_TIMER2  (1 << 16)
#define BIT_ORINT_LDPC    (1 << 17)
#define BIT_ORINT_PCIE    (1 << 18)
#endif

/* interrupt mask register in our HW TOP module */
#define rGLB_58 *(volatile U16 *)(REG_BASE_GLB+0x58) // MCU0 INT MSK
#define rGLB_5A *(volatile U16 *)(REG_BASE_GLB+0x5A) // MCU1 INT MSK
#define rGLB_5C *(volatile U16 *)(REG_BASE_GLB+0x5C) // MCU2 INT MSK

// define interrupt mask bit for HW module which connect to TOP module
#define TOP_INTSRC_HTDMAC  (1 << 0)
#define TOP_INTSRC_UART    (1 << 1)
#define TOP_INTSRC_GPIO    (1 << 2)
#define TOP_INTSRC_SPI     (1 << 3)
#define TOP_INTSRC_DMAE    (1 << 4)
#define TOP_MCU0_INTSRC_MCU1_0 (1 << 5) // MCU1 -> MCU0 interrupt
#define TOP_MCU1_INTSRC_MCU0_1 (1 << 5)
#define TOP_MCU2_INTSRC_MCU0_2 (1 << 5)
#define TOP_INTSRC_SDC     (1 << 6)
#define TOP_INTSRC_NFC     (1 << 7)
#define TOP_INTSRC_PMU     (1 << 8)
#define TOP_INTSRC_HOSTC   (1 << 9)
#define TOP_INTSRC_BUFFMAP (1 << 10)
#define TOP_MCU0_INTSRC_MCU2_0 (1 << 11) // MCU2 -> MCU0 interrupt
#define TOP_MCU1_INTSRC_MCU2_1 (1 << 11)
#define TOP_MCU2_INTSRC_MCU1_2 (1 << 11)
#define TOP_INTSRC_LDPC    (1 << 12)
#define TOP_INTSRC_PCIE    (1 << 13)

/* register for sending interrupt between MCUs */
#define rGLB_5E *(volatile U16 *)(REG_BASE_GLB+0x5E)

#ifndef VT3514_C0
#define INT_MCU0_1 (1 << 1) // MCU0 -> MCU1 interrupt
#define INT_MCU0_2 (1 << 2)
#define INT_MCU1_0 (1 << 4)
#define INT_MCU1_2 (1 << 6)
#define INT_MCU2_0 (1 << 8)
#define INT_MCU2_1 (1 << 9)

#define INT_MCU0_1_SET        INT_MCU0_1 
#define INT_MCU0_1_CLEAN      INT_MCU0_1 
#define INT_MCU0_2_SET        INT_MCU0_2 
#define INT_MCU0_2_CLEAN      INT_MCU0_2 
#define INT_MCU1_0_SET        INT_MCU1_0 
#define INT_MCU1_0_CLEAN      INT_MCU1_0
#define INT_MCU1_2_SET        INT_MCU1_2 
#define INT_MCU1_2_CLEAN      INT_MCU1_2
#define INT_MCU2_0_SET        INT_MCU2_0 
#define INT_MCU2_0_CLEAN      INT_MCU2_0
#define INT_MCU2_1_SET        INT_MCU2_1
#define INT_MCU2_1_CLEAN      INT_MCU2_1

#else
#define INT_MCU0_1_SET        (1 << 0) 
#define INT_MCU0_1_CLEAN      (1 << 1) 
#define INT_MCU0_2_SET        (1 << 2) 
#define INT_MCU0_2_CLEAN      (1 << 3) 
#define INT_MCU1_0_SET        (1 << 4) 
#define INT_MCU1_0_CLEAN      (1 << 5) 
#define INT_MCU1_2_SET        (1 << 6) 
#define INT_MCU1_2_CLEAN      (1 << 7) 
#define INT_MCU2_0_SET        (1 << 8) 
#define INT_MCU2_0_CLEAN      (1 << 9) 
#define INT_MCU2_1_SET        (1 << 10)
#define INT_MCU2_1_CLEAN      (1 << 11)
#endif

void HAL_UserExcpEntry(void);
void HAL_L4TimerIntEntry(void);
void HAL_SATAIntEntry(void);
void HAL_FlashIntEntry(void);
void HAL_InitInterrupt(U32 ulHwIntMask,U32 ulMcuIntSrc);
void HAL_ClearHwIntMask(U32, U32);
#endif

