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
Filename    : HAL_PerfMon.h
Version     : Ver 1.0
Author      : AbbyLin
Date        : 2017.03.28
Description : this file declare Performance Monitor driver interface. 
Others      : 
Modify      :
20140328    Abby     Create file
*******************************************************************************/
#ifndef __HAL_PERFORMANCE_MONITOR_H__
#define __HAL_PERFORMANCE_MONITOR_H__

#include "BaseDef.h"
#include "Proj_Config.h"

#ifndef ERI_ADDR_BASE
#define ERI_ADDR_BASE (0x100000) 
#endif

#define PM_ADDR_BASE    (ERI_ADDR_BASE + 0x1000)
#define PMG_REG         (PM_ADDR_BASE)          //Performance counters control register
#define INTPC_REG       (PM_ADDR_BASE + 0x10)   

#define PM_COUNT_BASE             (PM_ADDR_BASE + 0x0080) //Performance counter values
#define PM_CTRL_REG_BASE          (PM_ADDR_BASE + 0x0100) //Performance counter control registers
#define PM_STAT_REG_BASE          (PM_ADDR_BASE + 0x0180) //Performance counter status registers

#define PM_COUNT_MAX    8

#define PM_STAT0        (PM_ADDR_BASE + 0x0180) 
#define PM_STAT1        (PM_ADDR_BASE + 0x0184)
#define PM_STAT2        (PM_ADDR_BASE + 0x0188)
#define PM_STAT3        (PM_ADDR_BASE + 0x018C)
#define PM_STAT4        (PM_ADDR_BASE + 0x0190)
#define PM_STAT5        (PM_ADDR_BASE + 0x0194)
#define PM_STAT6        (PM_ADDR_BASE + 0x0198)
#define PM_STAT7        (PM_ADDR_BASE + 0x019C)

/* session states of PM counting */
typedef enum _PM_COUNT_STATE_
{
    SESS_SETUP = 0,
    COUNTING,
    INT_TRIGD,
    DENOUEMENT      //end of counting
}PM_COUNT_STATE;

/* types of interrupt */
typedef enum _PM_INT_TYPE_
{
    PM_NONE_INT = 0,    //none interrupt
    PM_OVFL_INT,        //overflow interrupt
    PM_INTASRT,         //interrupt assert
    PM_ALL_INT          //both PM_OVFL_INT and PM_INTASRT
}PM_INT_TYPE;

/* select & mask bits in each control reg: to determine which event is counted  */
/* select bits to choose event type*/
typedef enum _COUNT_EVENT_TYPE_
{
    CYCLE_CNT = 0,      //Always Increment
    OVERFLOW,           //Overflow of counter n-1(Assume this is counter n.)
    RETIRED_INSTR,      //Successfully Retired Instructions

    DATA_GLB_STALL,     //Data-related GlobalStall cycles
    INSTR_GLB_STALL,    //Instruction-related and Other GlobalStall cycles

    //...pending some events
    
    INSTR_MEM_ACCESS = 8,   //Instruction Memory Accesses
    //...pending some events
    DATA_MEM_ACCESS = 12,   //Data Memory Accesses

    COUNT_EVENT_NUM
}COUNT_EVENT_TYPE;

/* mask bits to choose event subtype */
typedef enum _COUNT_EVENT_SUBTYPE_
{
    //select = 0, Counts cycles;select = 1, Overflow of counter n-1
    NON_ZERO = 0xFF,

    //select = 2, Successfully Retired Instructions
    NON_CTI = BIT(15),          //15: Non-branch instr (aka. non-CTI)
    LOOPBACK_FALLTHRO = BIT(11),//11: Last inst of loop and execution falls through to LEND (aka.loopback fallthrough)
    LOOPBACK_TAKEN = BIT(10),   //10: Last inst of loop and execution transfers to LBEG (aka. loopback taken)
    TAKEN_LOOP = BIT(8),        //8: Loop instr where execution falls into loop (aka. taken loop)
    NOTTAKEN_BRANCH = BIT(7),   //7: Conditional branch instr where execution falls through (aka. nottaken branch)
    CALLN = BIT(6),             //6: CALLn instr
    J = BIT(5),                 //5: J instr
    TAKEN_BRANCH = BIT(4),      //4: Conditional branch instr where execution transfers to the target(aka. taken branch), 
                                //or loopgtz/loopnez instr where execution skips the loop (aka. not-taken loop)
    SUPER_RET = BIT(3),         //3: supervisor return instr i.e. RFDE, RFE, RFI, RFWO, RFWU
    CALL_RET = BIT(2),          //2: call return instr i.e. RET, RETW
    CALLXn = BIT(1),            //1: CALLXn instr
    JX = BIT(0),                //0: JX instr

    //select = 8, Instruction Memory Accesses
    INST_BYPS_FETCH = BIT(3),   //3: Bypass (i.e. uncached) fetch
    INSTRAM_INSTROM = BIT(2),   //2: All InstRAM or InstROM accesses
    INST_CACHE_MISS = BIT(1),   //1: Instruction Cache Miss
    INST_CACHE_HIT = BIT(0),    //0: Instruction Cache Hit

    //select = 12, Data Memory Accesses
    DATA_CACHE_MISS = BIT(0)    //0: Data Cache Miss
    
}COUNT_EVENT_SUBTYPE;

/* each counter own a control register */
typedef struct _PM_CNTL_REG_
{
    U32 bIntEn:1;       //Enables assertion of PerfMonInt output when overflow happens
    U32 bRsv0:2;
    U32 bKrnlCnt:1;     //Enables counting when CINTLEVEL* > TRACELEVEL 
                        //(i.e. If this bit is set, this counter counts only when CINTLEVEL >TRACELEVEL;
                        //if this bit is cleared, this counter counts only when CINTLEVEL ¡Ü TRACELEVEL)
    U32 bTraceLevel:4;  //Compares this value to CINTLEVEL* when deciding whether to count
    U32 bSelect:5;      //Selects input to be counted by the counter
    U32 bRsv1:3;
    U32 bMask:16;       //Selects input subsets to be counted (counter will increment only 
                        //once even if more than one condition corresponding to a mask bit occurs)  
}PM_CNTL_REG;

/* each counter own a status register */
/* 
   Overflow is cleared by writing a 1¡¯b1 to bit 0 of the Status register. 
   Similarly, IntAsserted is cleared by writing a 1¡¯b1 to bit 4 of the Status register. 
   Note that clearing these bits does not disable counting. 
*/
typedef struct _PM_STAT_REG_
{
    U32 bOvfl:1;        //Counter Overflow. Sticky bit set when a counter rolls over from 0xffffffff to 0x0.
    U32 bRsv0:3;
    U32 bIntAsrt:1;     //This counter¡¯s overflow caused PerfMonInt to be asserted.
    U32 bRsv1:27;
}PM_STAT_REG;

/*
    Global reg: This bit is generally cleared when setting up counters. 
    When all setup is completed, it can be set to start all counters simultaneously. 
    Clearing this bit will stop all counting.
*/
typedef struct _PM_GLOBAL_REG_
{
    U32 bPMEn:1;        //Overall enable for all performance counting
    U32 bRsv:31;
}PM_GLOBAL_REG;

#ifdef PM_TEST
//add for PM of L2

typedef struct _FCMD_PM_INFO_
{
    //3 DWs
    U32 bsPMWtType:8;       //to distinct different write type
    U32 bsRsv0:24;

    U32 bsRsv1;
    U32 bsRsv2;
}FCMD_PM_INFO;

typedef enum _L2_WRITE_TYPE_
{
    SLC2TLC = 0,
    TLC_SWL,
    TLC_GC,

    HOST_W,
    SLC_GC,

    TABLE_W,    //5, include RPMT W 
    TOTAL_W,

    WRITE_TYPE_NUM
}L2_WRITE_TYPE;

extern GLOBAL volatile U32 g_aPMWCnt[WRITE_TYPE_NUM];
extern GLOBAL volatile U32 g_aPMWCntOvfl[WRITE_TYPE_NUM];
#endif
/*------------------------------------------------------------------------------
    FUNCTIONS DECLARATION
------------------------------------------------------------------------------*/
void HAL_PerfMInit(void);
U32 HAL_PerfMGetCount(U8 ucCntIdx);

#endif
/* end of this file */
