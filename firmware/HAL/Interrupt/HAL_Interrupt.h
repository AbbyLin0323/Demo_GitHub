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
Filename     :                                       
Version      : 
Date         : 
Author       : 
Others: 
Modification History:
*******************************************************************************/

#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define EXCCAUSE_LEVEL1_INTERRUPT 4

/* Interrupt status and mask register inside HOSTC function */
#define rHOSTC_INTPENDING *(volatile U32 *)(REG_BASE_HOSTC + 0x10)
#define rHOSTC_INTMASK *(volatile U32 *)(REG_BASE_HOSTC + 0x14)

/* interrupt mask register in our HW TOP module */
#define rGlbIntMsk0 (*(volatile U16 *)(REG_BASE_GLB+0x58)) // MCU0 INT MSK
#define rGlbIntMsk1 (*(volatile U16 *)(REG_BASE_GLB+0x5A)) // MCU1 INT MSK
#define rGlbIntMsk2 (*(volatile U16 *)(REG_BASE_GLB+0x5C)) // MCU2 INT MSK

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
#define rGlbIPIIntTrig (*(volatile U16 *)(REG_BASE_GLB+0x5E))



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

/* Interrupt sources inside HOSTC function */
#define INT_HOSTC_EROM_ENABLE (1 << 0)
#define INT_HOSTC_EROM_DISABLE (1 << 1)
#define INT_HOSTC_BUSMASTER (1 << 2)
#define INT_HOSTC_PCIPOWERSTATE (1 << 3)
#define INT_HOSTC_PERST_ASST (1 << 4)
#define INT_HOSTC_CRRTBLERR (1 << 5)
#define INT_HOSTC_NONFATLERR (1 << 6)
#define INT_HOSTC_FATALERR (1 << 7)
#define INT_HOSTC_MPTOOL (1 << 8)
#define INT_HOSTC_TIMER (1 << 9)
#define INT_HOSTC_MASTPARITYERR (1 << 10)
#define INT_HOSTC_SIGTGTABRT (1 << 11)
#define INT_HOSTC_RCVTGTABRT (1 << 12)
#define INT_HOSTC_RCVMASTABRT (1 << 13)
#define INT_HOSTC_SIGSYSERR (1 << 14)
#define INT_HOSTC_DETPARITYERR (1 << 15)
#define INT_HOSTC_AHCI_HRST (1 << 16)
#define INT_HOSTC_AHCI_CMDSTART (1 << 17)
#define INT_HOSTC_AHCI_CMDSTOP (1 << 18)
#define INT_HOSTC_AHCI_SCTL (1 << 19)
#define INT_HOSTC_NVME_SHUTDOWN (1 << 24)
#define INT_HOSTC_NVME_CMDEN (1 << 25)
#define INT_HOSTC_L12_IDLE (1 << 28)
#define INT_HOSTC_PEUP_ABTRTY (1 << 29)
#define INT_HOSTC_PERST_DAST (1 << 30)
#define INT_HOSTC_PERST_INBD (1 << 31)

#define INT_HOSTC_PCIEFAILURE (INT_HOSTC_MASTPARITYERR | INT_HOSTC_SIGTGTABRT | \
                                INT_HOSTC_RCVTGTABRT | INT_HOSTC_RCVMASTABRT | INT_HOSTC_SIGSYSERR | \
                                INT_HOSTC_DETPARITYERR | INT_HOSTC_PEUP_ABTRTY)

void HAL_UserExcpEntry(void);
void HAL_L4TimerIntEntry(void);
void HAL_L5TimerIntEntry(void);
void HAL_SATAIntEntry(void);
void HAL_FlashIntEntry(void);
void HAL_InitInterrupt(U32 ulHwIntMask,U32 ulMcuIntSrc);
void HAL_ClearHwIntMask(U32, U32);
void HAL_AssertIPI(U8 ucReqMcuId,U8 ucRespMcuId,BOOL bSetInt);
void HAL_TSC2Uart(void);
#endif

