/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :BaseDef.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    11:19:39
Description :
Others      :updated from BaseDef.h
Modify      :
****************************************************************************/
#ifndef __BASE_DEF_H__
#define __BASE_DEF_H__
#ifdef __cplusplus
extern "C" {
#endif


/*some type definitions*/
typedef unsigned char    U8;
typedef unsigned short   U16;
typedef unsigned int     U32;
typedef int              S32;
typedef short            S16;
typedef char             S8;


#ifndef BOOL
typedef S32 BOOL;
//typedef S32 bool;
#endif

#ifndef FAIL
#define    FAIL    0
#endif

#ifndef SUCCESS
#define    SUCCESS    1
#endif

#ifndef FALSE
#define FALSE   FAIL
#endif

#ifndef TRUE
#define TRUE    SUCCESS
#endif

#define    GLOBAL    
#define    LOCAL     static

/*long*/
#ifndef    INVALID_8F        
#define    INVALID_8F        0xFFFFFFFF
#endif

/*short*/
#ifndef    INVALID_4F        
#define    INVALID_4F        0xFFFF
#endif

/*char*/
#ifndef    INVALID_2F        
#define    INVALID_2F        0xFF
#endif

#ifndef     MSK_F    
#define     MSK_F        0xf

#define     MSK_1F         0xf
#define     MSK_2F         0xff
#define     MSK_3F         0xfff
#define     MSK_4F         0xffff
#define     MSK_5F         0xfffff
#define     MSK_6F         0xffffff
#define     MSK_7F         0xfffffff
#define     MSK_8F         0xffffffff
#endif


#ifndef NULL
#define NULL 0
#endif

//#define ROM_COSIM
#define RAMDISK
#define CDC_WAIT_TIME 10
//#define SIM_DBG          // this is for DBG log printf
//#define  SIM_XTENSA       // this is for XTMP env
//#define  SIM              // this is for WINDOEWS env
//#define NO_THREAD
//#define  ASIC
//#define  ECT_TRACE          // trace flash chip erase count in ASIC Env.
//#define SIM_HOST_CMD_DBG
#define HCLK300M
//#define HAL_NFC_RD_RETRY
#define L1_OPTIMIZATION
#define L2_OPTIMIZATION
//#define L3_OPTIMIZATION
//#define GLOBAL_INJ_ERROR
//#define L2_FORCE_VIRTUAL_STORAGE
//#define L3_LOCAL_TEST
#ifdef L3_LOCAL_TEST
#define L3_DRIVER_TEST
//#define HAL_DRIVER_TEST
#endif
//#define DMAE_ENABLE
//#define DEBUG_BM
//#define DEBUG_SIGNAL_EN
//#define SEARCH_ENGINE
//#define L3_RAND_READ_ACC
//#define DCACHE
//#define DUAL_CORE_ENABLE
//#define LPN_TO_CE_REMAP
//#define HCMD_PU_QUEUE
#ifdef HCMD_PU_QUEUE
//#define HCMD_PUQ_IN_OTFB
#endif

//#define HOST_PATTERN_RECORD
#ifdef HOST_PATTERN_RECORD
#undef HCMD_PU_QUEUE
#define TRACE_ALL_MODULE_ENABLE
#endif

#ifdef SIM
#define EMPTY_PG_DETECT_FW
#undef DMAE_ENABLE
#endif

//#define FW_CTRL_ALL_SDBFISREADY
#define OTFB_VERSION  /* ifdef OTFB_VERSION, unspport dram */
//#define SUPPORT_HW_TRIG_SLUMBER
//#define ONLY_ODD_CMD_TAG_DMA_AUTO_ACTIVE_EN /* odd ncq cmd tag set dma_auto_active_en only. ack gavin*/ 

#ifdef SIM_XTENSA
#define TRACE_REG_BASE 0x1ff80000
#define rTracer  (*((volatile U32*)(TRACE_REG_BASE+0x80)))
#define rTraceData rTracer

typedef enum _TRACE_LOCATION
{
    TL_L1_ENTRY = 0xA0010000,
    //TL_L1_GET_HCMD_START,
    //TL_L1_GET_HCMD_FINISH,
    TL_L1_SPLIT_HCMD_START,
    TL_L1_SPLIT_HCMD_FINISH,
    //TL_L1_BUFFMANAGE_START,
    //TL_L1_BUFFMANAGE_FINISH,
    //TL_L1_CACHESEARCH_START,
    //TL_L1_CACHESEARCH_FINISH,
    //TL_L1_CACHEMANAGE_START,
    //TL_L1_CACHEMANAGE_FINISH,
    TL_L1_SATAIO_START,
    TL_L1_SATAIO_FINISH,
    //TL_L1_MERGEFLUSH_START,
    //TL_L1_MERGEFLUSH_FINISH,
    //TL_L1_RECYCLE_SUBCMD,
    TL_L1_ALL_FINISH = 0xA001FFFF,

    TL_L2_ENTRY         = 0xA0020000,
    TL_L2_ALL_FINISH    = 0xA002FFFF,

    TL_L3_ENTRY         = 0xA0030000,
    TL_L3_ALL_FINISH    = 0xA003FFFF,

    TL_SATA_INTERRUPT_ENTRY = 0xB0000000,
    TL_SATA_INTERRUPT_FINISH= 0xBFFFFFFF,

    TL_FW_INIT_DDR = 0xC0010001,
    TL_FW_BOOTUP,
    TL_FW_HAL_INIT_START,
    TL_FW_COMRESET,
    TL_FW_OOBDONE,

    TL_SATAIO_CK_DSG = 0xD0000000,
    TL_SATAIO_BD_DSG = 0xD0ff0000, 

    TL_PIO_STAGE     = 0xE0000000,

    TL_WRITE_WHOLEDISK_FINISH,
    TL_ALL_HOSTCOMMAND_FINISH
}TRACE_LOCATION;


typedef enum _FW_CMD_CODE_TAG
{
    INIT_PMT_MANAGER = 0,
    INIT_PMTI,
    INIT_PMT_PAGE_ADDR,
    INIT_PBIT,
    INIT_VBT,
    INIT_ECT,
    INIT_PMT_TABLE,
    INIT_PBIT_VBT_TABLE,
    LLF_PMT_TABLE,
    FW_CMD_CODE_END

}FW_CMD_CODE;
#endif

#ifdef SIM_DBG
#include "../system_statistic.h"

#define FIRMWARE_HAL_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun,TRACE_FIRMWARE_MODULE_ID,TRACE_FIRMWARE_SUBMODULE_HAL_ID,log_level,x, __VA_ARGS__);

#define FIRMWARE_L1_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun,TRACE_FIRMWARE_MODULE_ID,TRACE_FIRMWARE_SUBMODULE_L1_ID,log_level,x, __VA_ARGS__);

#define FIRMWARE_L2_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun,TRACE_FIRMWARE_MODULE_ID,TRACE_FIRMWARE_SUBMODULE_L2_ID,log_level,x, __VA_ARGS__);

#define FIRMWARE_L3_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun,TRACE_FIRMWARE_MODULE_ID,TRACE_FIRMWARE_SUBMODULE_L3_ID,log_level,x, __VA_ARGS__);

#define HOSTCMD_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun,TRACE_FIRMWARE_MODULE_ID,TRACE_FIRMWARE_HOST_CMD_SUBMODULE_ID,log_level,x, __VA_ARGS__);

#else

#define FIRMWARE_HAL_LogInfo(x, ...)  (void)0
#define FIRMWARE_L1_LogInfo(x, ...) (void)0
#define FIRMWARE_L2_LogInfo(x, ...) (void)0
#define FIRMWARE_L3_LogInfo(x, ...) (void)0
#define HOSTCMD_LogInfo(x, ...) (void)0

#endif
//#define PMT_CHECK
//#define FAKE_L1
//#define FAKE_L3
//#define FAKE_L2
//#define TB_CHK
//#define DBG_PMT
/*there are two methods to implement function L2_schedule, one is use the flattened firmwarecode,
the other is to use Write_normal() , read_normal() function*/
//#define FAKE_L2_USE_FLAT_CODE
//#define TRACE_PERFORMANCE_AISC

#ifdef SIM
#include "stdio.h"
extern U32 SIM_DRAM_BASE;
extern U32 SIM_OTFB_BASE;
extern U32 SIM_SRAM_BASE;
extern U32 SIM_REG_BASE;
extern U32 SIM_TRI_REG_BASE;
extern U32 SIM_PRD_BASE;

#ifndef DBG_Printf
#define DBG_Printf 
#endif

#ifndef DBG_Getch
#define DBG_Getch  getch
#endif

//#define ERROR_INJ
#else
#include "uart.h"
#define DBG_Printf dbg_printf
#define DBG_Nop(...) __asm__("nop")
#define INLINE inline
#endif

#define max(a,b)            (((a) > (b)) ? (a) : (b))


/*add by WinterYang*/
#ifdef SIM
//#define TABLE_MISS_VERIFY
//#define _RANDPMTI_
#endif

#ifdef RAMDISK
#define CE_NUM  2
#define PU_NUM  CE_NUM

#define CE_MAX  32
#define HAL_FLASH_REQ_FLASHSTATUS_PENDING    0x0
#define HAL_FLASH_REQ_FLASHSTATUS_SUCCESS    0x1
#define HAL_FLASH_REQ_FLASHSTATUS_FAIL       0x2
#define HAL_FLASH_REQ_FLASHSTATUS_EMPTY_PG   0x3

#define HAL_CACHESTATUS_INIT_VALUE  0x2


#define PG_SZ_BITS                14
#define PLN_PER_PU_BITS           1

#define PIPE_PG_SZ_BITS           (PG_SZ_BITS + PLN_PER_PU_BITS)
#define PIPE_PG_SZ                (1 << PIPE_PG_SZ_BITS)
#define PIPE_PG_SZ_MSK            (PIPE_PG_SZ - 1)

#define PAGE_SIZE_IN_BYTE BUF_SIZE
#define LPN_SIZE_IN_BYTE LPN_SIZE
#define SEC_SIZE_IN_BYTE SEC_SZ
#define SECTOR_PER_LPN SEC_PER_LPN
#define PMT_ITEM_SIZE 4


#endif

typedef void(*PFUNC)(void);

#endif

