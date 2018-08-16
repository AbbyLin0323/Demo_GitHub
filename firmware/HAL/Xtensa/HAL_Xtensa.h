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
Filename    : HAL_Xtensa.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.02
Description : this file declare interface for operation to Tensilica MCU core 
Others      : some interfaces are available only in Xtensa ENV.
Modify      :
20140902    Gavin     Create file
20140904    Gavin     add function declaration
20150407    Gavin     add MCU interrupt definition from HAL_Interrupt.h
*******************************************************************************/
#ifndef __HAL_XTENSA_H__
#define __HAL_XTENSA_H__

#include "BaseDef.h"

/* PMU REG68: HPLL and DPLL frequency control
Bit[31:29] -- Reserved;
Bit[28] -- HPLL bypass mode enable, RW:
    0 - Bypass mode is disabled,
    1 - Bypass mode is enabled;
Bit[27:26] -- Reserved;
Bit[25:24] -- HPLL output divider select, RW;
Bit[23] -- HPLL input divider select, RW;
Bit[22:16] -- HPLL feedback divider select, RW;
Bit[15:13] -- Reserved;
Bit[12] -- DPLL bypass mode enable, RW:
    0 - Bypass mode is disabled,
    1 - Bypass mode is enabled;
Bit[11:10] -- Reserved;
Bit[9:8] -- DPLL output divider select, RW;
Bit[7] -- DPLL input divider select, RW;
Bit[6:0] -- DPLL feedback divider select, RW.
 */
typedef struct _PMU_PLLCNTLREG
{
    U32 bsDPLLFbDivSel: 7;
    U32 bsDPLLInDivSel: 1;
    U32 bsDPLLOutDivSel: 2;
    U32 bsReg68Rsvd1: 2;
    U32 bsDPLLBypsEn: 1;
    U32 bsReg68Rsvd2: 3;
    U32 bsHPLLFbDivSel: 7;
    U32 bsHPLLInDivSel: 1;
    U32 bsHPLLOutDivSel: 2;
    U32 bsReg68Rsvd3: 2;
    U32 bsHPLLBypsEn: 1;
    U32 bsReg68Rsvd4: 3;
} PMU_PLLCNTLREG;

/* MCU's ID: MCU's internal register PRID record its ID */
#define MCU0_ID     1
#define MCU1_ID     2
#define MCU2_ID     3

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

#define BIT_ORINT_PMU     (1 << 13)
#define BIT_ORINT_HOSTC   (1 << 14)
#define BIT_ORINT_BUFFMAP (1 << 15)
#define BIT_ORINT_TIMER2  (1 << 16)
#define BIT_ORINT_MCU2_0    (1 << 17)  // MCU2 -> MCU0 interrupt
#define BIT_ORINT_MCU2_1    (1 << 17)
#define BIT_ORINT_MCU1_2    (1 << 17)
#define BIT_ORINT_LDPC    (1 << 18)
#define BIT_ORINT_PCIE    (1 << 19)
#define BIT_ORINT_XORE    (1 << 20)


//prototype definition of time-out call-back function
typedef void(*pMcuTimerCallBack)(void);

/* function interface */
U32 HAL_GetMcuId(void);
U32 HAL_GetMcuClock(void);
void HAL_DisableMCUIntAck(void);
void HAL_EnableMCUIntAck(void);
void HAL_DisableMCUIntSrc(U32 ulDisableMap);
void HAL_EnableMCUIntSrc(U32 ulEnableMap);
void HAL_MCUWaitForInt(void);
U32 HAL_GetMCUIntSrc(void);
U32 HAL_GetMCUIntEnableBitmap(void);
U32 HAL_GetMCUExcCause(void);
void HAL_SaveMCUExcCause(U32 ulExcCause);
void HAL_MemoryWait(void);
void HAL_EnableICache(U32 ulMcuId);
void HAL_EnableDCache(U32 ulMcuId);
void HAL_EnableDramAddrHigh(void);
void HAL_InvalidateDCache(void);
BOOL HAL_IsDCacheEnabled(U32 ulAddr);
U32 HAL_GetMCUCycleCount(void);
void HAL_DelayCycle(U32 ulCycleCnt);
void HAL_DelayUs(U32 ulUsCount);
void HAL_McuTimerISR(U32 ulTimerID);
void HAL_StartMcuTimer(U32 ulTimeOutMicroSec, pMcuTimerCallBack pCallBack);
void HAL_StartMcuTimer1(U32 ulTimeOutMicroSec, pMcuTimerCallBack pCallBack);
void HAL_StopMcuTimer(void);
void HAL_WSRVecBase(U32 ulNewVecBase);
U32 HAL_GetBssSegEndAddr(void);
U8 HAL_CLZ(U32 ulDw);
U8 HAL_POPCOUNT(U32 ulDw);
void HAL_Wclzstate(U32 ulDw);
U8 HAL_SCLZ(void);

#ifndef SIM
#include <xtensa/tie/via.h>
/*------------------------------------------------------------------------------
Name: HAL_MCUInsertbits
Description: 
    Replaces the bit segment specified within target operand with the
    value of source operand. It is a tie-accelerated calculation.
Input Param:
    TargetOp - Target Operand;
    SourceOp - Source Operand;
    StartBit - Specifies the start bit of the segment to be replaced in target;
    EndBit - Specifies the end bit of the segment to be replaced in target;
    (Both start bit and end bit are included in the replacement.)

Output Param:
    TargetOp
Return Value:
    none
Usage:
    HAL_MCUInsertbits(ulBitmap1, 5, 3, 7);
    This invoking would replace bit[7:3] of the variable ulBitmap1 with value 5(5'b00101).
History:
    20151230 Yao first created.
------------------------------------------------------------------------------*/
#define HAL_MCUInsertbits INSBITS

/*------------------------------------------------------------------------------
Name: HAL_MCUClearbits
Description: 
    Clears the bit segment specified within target operand to all 0's.
    It is a tie-accelerated calculation.
Input Param:
    TargetOp - Target Operand;
    StartBit - Specifies the start bit of the segment to be cleared in target operand;
    EndBit - Specifies the end bit of the segment to be replaced in target operand;
    (Both start bit and end bit would also be cleared.)

Output Param:
    TargetOp
Return Value:
    none
Usage:
    HAL_MCUClearbits(ulBitmap1, 5, 7);
    This invoking would replace bit[7:5] of the variable ulBitmap1 with value 0(3'b000).
History:
    20151230 Yao first created.
------------------------------------------------------------------------------*/
#define HAL_MCUClearbits CLRBITS
#else
#define HAL_MCUInsertbits
#define HAL_MCUClearbits
#endif

#endif /* __HAL_XTENSA_H__ */

