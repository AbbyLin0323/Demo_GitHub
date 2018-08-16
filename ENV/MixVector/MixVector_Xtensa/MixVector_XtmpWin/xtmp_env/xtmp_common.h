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
*******************************************************************************/

#ifndef _XTMP_COMMON_H_INCLUDED
#define _XTMP_COMMON_H_INCLUDED

#include "iss/mp.h"
#include "xtmp_sysmem.h"
#include "xtmp_localmem.h"
#include "Proj_Config.h"
//#define SIM_FOR_PROFILE			
#ifndef MIX_VECTOR		
#define SIM_FOR_3_CORE
//#define ROM_BOOTLOADER_FW_FLOW
#define BOOTLOADER_TEST
#endif

//Header boot relative define
#ifdef VT3514_C0
#define XTENSA_CORE "rfviatieloop16"
#define ISRAM_REST_VECT_OFFSET (0x400)
#define DSRAM0_SHARE_TOTAL_SZ (0x1ff95000 - 0x1ff80000)
#else
#define XTENSA_CORE "viatie"
#define ISRAM_REST_VECT_OFFSET (0x3c0)
#define DSRAM0_SHARE_TOTAL_SZ  (0x1ff8e000 - 0x1ff80000)
#endif
#define MCU0_ROM_VECT_PC   (0xffe00100)
#define MCU_ISRAM_VECT_PC  (0x20000000 + ISRAM_REST_VECT_OFFSET)
#define HEADER_OFFSET      (256<<10) //dram offset for binary file.
#define ENTRANCE_TEXT_PC   (0x40048010)//header boot start pc addr for mcu0

#define DEAD_LOOP_INS      (0xffff06) //machine code of dead loop
#define FILE_SZ_FW         (864<<10)//(768<<10) //size down load to DRAM
#define FILE_SZ_BOOTLOADER BOOTLOADER_SIZE //(16<<10)

#define DSRAM0_MCU0_OFFSET      (0x24000)
#define DSRAM0_MCU1_OFFSET      (0x20000)
#define DSRAM0_MCU2_OFFSET      (0x20000)

#ifdef SIM_FOR_3_CORE
#define CORE_NUM 3
#else
#define CORE_NUM 1
#endif
extern u32 pifWidth, dataRamWidth;
extern bool bigEndian;
extern XTMP_event simSataDevEvent;
extern XTMP_core g_core[];

typedef void(*regReadHandler)(u32 regaddr, u32 regvalue, u32 nsize);

typedef bool(*regWriteHandler)(u32 regaddr, u32 regvalue, u32 nsize);

typedef struct {
	u32 startAddr;
	u32 endAddr;
	regReadHandler regReadFunc;
	regWriteHandler regWriteFunc;
}regHandleCnf;

typedef enum _SYSTEM_STS
{
    SYSTEM_RUNING,
    SYSTEM_SUSPENDSTART,
    SYSTEM_SUSPEND
}SYSTEM_STS;

#define regHandlerCnfMax 30
extern regHandleCnf regHandleCnfArr[regHandlerCnfMax];
extern u32 ugCmdCount;

extern void InsertRegHandleCnf(u32 startAddr, u32 endAddr, regReadHandler regReadFunc, regWriteHandler regWriteFunc);
extern void setInterrupt(u32 bit, bool isSet);

#endif //_XTMP_COMMON_H_INCLUDED
