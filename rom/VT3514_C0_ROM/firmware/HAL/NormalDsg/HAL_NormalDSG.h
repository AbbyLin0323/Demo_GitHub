/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    : HAL_NormalDSG.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2013.09.02
Description : this file declare Normal DSG driver interface. 
Others      : 
Modify      :
20130902    Gavin     Create file
20140909    Tobey     uniform coding style
****************************************************************************/

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
typedef struct _DSG_REPORT_MCU 
{
    U32 bsDsgValue:1;    // normal dsg sts set value
    U32 bsDsgWrIndex:10; // normal dsg index
    U32 bsDsgWrEn:1;     // Status Write request ,firmware set, hardware clean
    U32 bsRsv0:8;        // reserved
    U32 bsDsgTrigger:1;  // search normal dsg id trigger
    U32 bsDsgId:10;      // current normal dsg id
    U32 bsDsgValidEn:1;  // current normal dsg valid sts falg
}DSG_REPORT_MCU,*pDSG_REPORT_MCU;

/*Nfc DSG entry define*/
typedef struct _NORMAL_DSG_ENTRY
{
    /* DWORD 0 */
    U32 bsNextDsgId: 9;       // pointer to next dsg
    U32 bsLast: 1;           // 1: last DSG; 0: not last DSG 
    U32 bsRsv:4;              // Reserved
    U32 bsXferByteLen: 18;    // transfer length byte

    /* DWORD 1 */
    U32 ulDramAddr;           // dram addr

    /* DWORD 2 */
    U32 bsCacheStatusAddr: 31; // cache status  address
    U32 bsCsInDram: 1;         // cache status in DRAM, 1 = in DRAM, 0 = in OTFB

    /* DWORD 3 */    
    U32 bsCacheStsData: 8;     // cache status data
    U32 bsCacheStsEn: 1;       // cache status enable 
    U32 bsBuffMapId: 6;        // buffer map id
    U32 bsMapSecLen: 9;        // buffer map id
    U32 bsMapSecOff: 8;        // the sector offset in buffer
}NORMAL_DSG_ENTRY;

/* function interface */
void HAL_NormalDsgInit(void);
BOOL HAL_GetNormalDsg(U16 *pDsgId);
BOOL HAL_IsNormalDsgValid(void);
BOOL HAL_GetCurNormalDsg(U16 *pDsgId);
void HAL_TriggerNormalDsg(void);
U32 HAL_GetNormalDsgAddr(U16 usDsgId);
void HAL_SetNormalDsgSts(U16 usDsgId, U8 ucStsValue);

#endif

