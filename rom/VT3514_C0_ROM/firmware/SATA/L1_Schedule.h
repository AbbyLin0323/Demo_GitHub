/*************************************************
Copyright (c) 2012 VIA Technologies, Inc. All Rights Reserved.

Information in this file is the intellectual property of
VIA Technologies, Inc., and may contains trade secrets that must be stored 
and viewed confidentially..

Filename     :                                           
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  PeterXiu

Description: 

Modification History:
20120118     peterxiu     001 first create
*************************************************/
#ifndef _FLASH_FILE_H
#define _FLASH_FILE_H

//TRACE DEFINE START
#define L1_SUBMODULE5_ID        5
#define L1_SUBMODULE5_NAME      "L1Schedule"
#define L1_SUBMODULE5_ENABLE    1
#define L1_SUBMODULE5_LEVEL     LOG_LVL_TRACE
//TRACE DEFINE END

//TRACE DEFINE START
#define PERF_SUBMODULE0_ID      0
#define PERF_SUBMODULE0_NAME    "L1Cycle"
#define PERF_SUBMODULE0_ENABLE  1
#define PERF_SUBMODULE0_LEVEL   LOG_LVL_INFO
//TRACE DEFINE END

#if 0
#define  TaskSpeed_SLOW  (1)
#define  TaskSpeed_NORMAL (2)
#define  TaskSpeed_FAST   (5)  // We assume in worst case, eg. randome write, every 5 request will result a FlashReq
#else
#define  TaskSpeed_SLOW   (1)
#define  TaskSpeed_NORMAL (LPN_PER_BUF/2)
#define  TaskSpeed_FAST   (LPN_PER_BUF)
#endif

#define  PU_FIFO_COUNT_LWTHS   (L1_BUFFER_COUNT_WRITE/8)
#define  PU_FIFO_COUNT_UPTHS   (L1_BUFFER_COUNT_WRITE*3/4)

#define  BUF_REQ_COUNT_LWTHS   (PRIO_FIFO_DEPTH/8)
#define  BUF_REQ_COUNT_UPTHS   (PRIO_FIFO_DEPTH/4)

#define  HOSTCMD_COUNT_LWTHS   (TaskSpeed_NORMAL)
#define  HOSTCMD_COUNT_UPTHS   (TaskSpeed_FAST)

#define  CE_FLUSH_COUNT_LWTHS   (CE_NUM)
#define  CE_FLUSH_COUNT_UPTHS   (2*CE_NUM)

/*  
Assume L1 will be scheduled every 500 cycles when system idle.
Then, for 300MHz CPU, 1 sec L1 will be scheduled 600,000 times.
So, we define L1 idle flush threshold with 30,000. (50 ms)
*/
#define  L1_IDLE_FLUSH_THRESHOLD (30000)

#define  L1_TaskSpeed_SLOW   1
#define  L1_TaskSpeed_NORMAL 2
#define  L1_TaskSpeed_RAND_WRITE    (L1_TaskSpeed_NORMAL*4)
#define  L1_TaskSpeed_FAST   (CE_NUM)

#define  L3_TaskSpeed_SLOW 1
#define  L3_TaskSpeed_NORMAL 2
#define  L3_TaskSpeed_FAST   (CE_NUM)

#define  USED_CEFIFO_UPTHS          (CE_NUM* CE_FIFO_DEPTH- (CE_NUM/2))       // up threshold
#define  USED_CEFIFO_DOWNTHS    (CE_NUM/2)       // up threshold

typedef enum L1_MCUPOWEROFF_STAGE_TAG
{
    L1_MCUPOWEROFF_STAGE_IDLE = 0,
    L1_MCUPOWEROFF_STAGE_FLASHCACHE,
    L1_MCUPOWEROFF_STAGE_L1_TRIGGER,
    L1_MCUPOWEROFF_STAGE_L1_WAIT,
    L1_MCUPOWEROFF_STAGE_L2_TRIGGER,
    L1_MCUPOWEROFF_STAGE_L2_WAIT,
    L1_MCUPOWEROFF_STAGE_L3_TRIGGER,
    L1_MCUPOWEROFF_STAGE_L3_WAIT,
    L1_MCUPOWEROFF_STAGE_DONE,
    L1_MCUPOWEROFF_STAGE_ALL
}L1_MCUPOWEROFF_STAGE;

extern U8 L1_TaskSchedule(void);
BOOL L1_TaskScheduleNoneData(HCMD* pCurHCMD);
void MCU0TaskRatio();
void MCU0TaskSchedule();
void MCU1TaskSchedule();
void MCU0TaskInit();
void MCU1TaskInit();
void L1_TaskInit(U32 *pFreeDramBase,U32 *pFreeOTFBBase);
U32 L1_TaskEventHandle(void);


U32 DRAM_ATTR L1_TaskPMCheckBusy();
U32 L2_TaskPMCheckBusy();
U32 L3_TaskPMCheckBusy();
U32 DRAM_ATTR MCU0PowerOffControl();
BOOL DRAM_ATTR MCU0TaskIDLE();

void MCU1TaskRatio();
void MCU1_Schedule_Init();

#endif

/********************** FILE END ***************/
