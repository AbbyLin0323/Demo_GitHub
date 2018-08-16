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
Filename    : HAL_HSG.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2013.09.02
Description : this file declare HSG driver interface. 
Others      : 
Modify      :
20130902    Gavin     Create file
20140909    Tobey     uniform coding style
*******************************************************************************/

#ifndef __HAL_HSG_H__
#define __HAL_HSG_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

/*HSG and Sata Dsg use the same HW module, 
  when this module used as HSG should define macro HSG_MODE, 
  otherwise should define macro SATA_DSG_MODE*/
#define HSG_MODE

/*define HSG number 768*/
#define HSG_NUM     768
#define HSG_VALID   1
#define HSG_INVALID 0

#define rHsgBaseAddr       (*((volatile U32 *)(REG_BASE_SGE + 0x04)))
#define rHsgReportMcu1     (*((volatile U32 *)(REG_BASE_SGE + 0x3c)))
#define rHsgReportMcu2     (*((volatile U32 *)(REG_BASE_SGE + 0x40)))
#define rSGESataMode       (*((volatile U32*)(REG_BASE_SGE + 0x1c)))
#define SGE_SATAMODE_ENABLE 1
#define rHsgReportMcu0     (*((volatile U32 *)(REG_BASE_SGE + 0x88)))

typedef union _HSG_REPORT_MCU
{
    struct {
        U32 bsHsgValue  : 1;        // Set HSG value, Bit[0]
        U32 bsHsgWrIndex: 10;       // HSG index, Bit[1:10]
        U32 bsHsgWrEn   : 1;        // HSG Status set trigger ,FW set, HW clean, Bit[11]
        U32 bsRsv0      : 8;        // Reserved, Bit[12:19]
        U32 bsHsgTrigger: 1;        // Search HSG trigger, Bit[20]
        U32 bsHsgId     : 10;       // current HSG ID, Bit[21:30]
        U32 bsHsgValidEn: 1;        // current HSG ID valid flag, set by FW, Bit[31]
    };
    U32 DW;
} HSG_REPORT_MCU,*pHSG_REPORT_MCU;

/*HSG entry define*/
typedef struct _HSG_ENTRY
{
    /* DWORD 0 */
    U32 bsNextHsgId     : 10;   // pointer of next HSG
    U32 bsLast          : 1;    // 1: last HSG; 0: not last HSG
    U32 bsLenRsv        : 3;    // reserved
    U32 bsLength        : 18;   // transfer length

    /* DWORD 1 */
    U32 ulHostAddrLow;    // host memory address low 32 bits

    /* DWORD 2 */
    U32 ulHostAddrHigh;   // host memory address high 32 bits

    /* DWORD 3 */
    U32 ulLBA;            // LBA according to the the HSG
}HSG_ENTRY;

/* function interface */
void HAL_HsgInit(void);
BOOL HAL_GetHsg(U16 *pHsgId);
BOOL HAL_IsHsgValid(void);
BOOL HAL_GetCurHsg(U16 *pHsgId);
void HAL_TriggerHsg(void);
U32 HAL_GetHsgAddr(U16 usHsgId);
void HAL_SetHsgSts(U16 usHsgId, U8 ucStsValue);

#endif

