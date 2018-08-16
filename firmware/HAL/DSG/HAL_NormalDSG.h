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
Filename    : HAL_NormalDSG.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2013.09.02
Description : this file declare Normal DSG driver interface. 
Others      : 
Modify      :
20130902    Gavin     Create file
20140909    Tobey     uniform coding style
*******************************************************************************/

#ifndef __HAL_NORMAL_DSG_H__
#define __HAL_NORMAL_DSG_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"

/* define normal DSG number*/
#define NORMAL_DSG_NUM 512
#define NORMAL_DSG_VALID 1
#define NORMAL_DSG_INVALID 0

#define rDsgBaseAddr      (*((volatile U32 *)(REG_BASE_SGE + 0x08)))
#define rDsgReportMcu1    (*((volatile U32 *)(REG_BASE_SGE + 0x34)))
#define rDsgReportMcu2    (*((volatile U32 *)(REG_BASE_SGE + 0x38)))
#define rDsgReportMcu0    (*((volatile U32 *)(REG_BASE_SGE + 0x8c)))

/*DSG report register structure define*/
typedef union _DSG_REPORT_MCU
{
    struct
    {
        U32 bsDsgValue:1;    // normal dsg sts set value
        U32 bsDsgWrIndex:10; // normal dsg index
        U32 bsDsgWrEn:1;     // Status Write request ,firmware set, hardware clean
        U32 bsRsv0:8;        // reserved
        U32 bsDsgTrigger:1;  // search normal dsg id trigger
        U32 bsDsgId:10;      // current normal dsg id
        U32 bsDsgValidEn:1;  // current normal dsg valid sts falg
    };
    U32 DW;
} DSG_REPORT_MCU,*pDSG_REPORT_MCU;

/*Nfc DSG entry define, add bsOntf for VT3533*/
typedef struct _NORMAL_DSG_ENTRY
{
    /* DWORD 0 */
    U32 bsNextDsgId: 9;       // pointer to next dsg
    U32 bsLast: 1;            // 1: last DSG; 0: not last DSG
    U32 bsRsv:4;              // Reserved
    U32 bsXferByteLen: 18;    // transfer length byte

    /* DWORD 1 */
    U32 bsOntf:1;             // 1-Current CW trans to OTFB;0-DRAM
    U32 bsDramAddr:31;        // dram addr

    /* DWORD 2 */
    U32 bsCacheStatusAddr: 31; // cache status  address
    U32 bsCsInDram: 1;         // cache status in DRAM, 1 = in DRAM, 0 = in OTFB

    /* DWORD 3 */
    U32 bsCacheStsData: 8;     // cache status data
    U32 bsCacheStsEn: 1;       // cache status enable
    U32 bsBuffMapId: 6;        // buffer map id
    U32 bsXorClr:1;            // 1: need clear XORE engine after this DSG transfer done.
    U32 bsXorId:3;             // XORE engine ID
    U32 bsRsv2:5;
    U32 bsMapSecOff: 8;        // the sector offset in buffer
} NORMAL_DSG_ENTRY;

#define HALM_GET_DRAM_ADDR(x)  ((U32)(x) - DRAM_START_ADDRESS)

/* function interface */
void HAL_NormalDsgReset(void);
void HAL_NormalDsgInit(void);
BOOL HAL_GetNormalDsg(U16 *pDsgId);
BOOL HAL_IsNormalDsgValid(void);
BOOL HAL_GetCurNormalDsg(U16 *pDsgId);
void HAL_TriggerNormalDsg(void);
U32 HAL_GetNormalDsgAddr(U16 usDsgId);
void HAL_SetNormalDsgSts(U16 usDsgId, U8 ucStsValue);
void HAL_NormalDsgFifoInit(void);
#endif

