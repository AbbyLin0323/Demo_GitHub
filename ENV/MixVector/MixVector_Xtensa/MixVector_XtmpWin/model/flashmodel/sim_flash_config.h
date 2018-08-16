/*************************************************
*Copyright (C), 2012, VIA Tech. Co., Ltd.
*Filename: defines.h                                            
*Version: 1.0                                                 
*Date: 20120208                                          
*Author: Catherine Li
*
*Description: Flash Simulation Configuration
*
*Modification History:
*catherineli,       20120208,      first created
*************************************************/
#ifndef __SIM_FLASH_CONFIG_H__
#define __SIM_FLASH_CONFIG_H__

/*----------------------------------------------------
define simulation environment
*/
#include "BaseDef.h"

/*----------------------------------------------------*/
#include "HAL_Inc.h"
#ifdef VT3514_C0
#include "HAL_NfcDataCheck.h"
#endif

//#define WINDOWS_DEBUG
#define WINDOWS_DEBUG_CONFIG
//#define TIMER_DEBUG
#define FLASH_MODULE_FAST_STORAGE
//#define READ_TABLE_SECTOR_ENA 1


#if defined(SIM_XTENSA)
#include "iss/mp.h"
//#define    DBG_Getch()    getch()
//#define    DBG_Printf(x, ...) printf(x, __VA_ARGS__)
#endif
#include <windows.h>

extern char FM_LogString[256];
//extern char FM_LogStringErs[32][256];
extern int FM_LogHasWrite;
extern HANDLE hFM_LogFile;
extern CRITICAL_SECTION FMLog_critical_section;

//#define HCLK_200M_TOGGLE_100M
#define HCLK_300M_TOGGLE_75M

#define NFC_ResultLog(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun, TRACE_FLASHDEVICE_RESULT_MODULE_ID,TRACE_FLASHDEVICE_RESULT_SUBMODULE_ID,log_level,x, __VA_ARGS__);

#define NFC_LogErr(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun, TRACE_FLASHDEVICE_RESULT_MODULE_ID,TRACE_FLASHDEVICE_RESULT_SUBMODULE_ID,log_level,x, __VA_ARGS__);

#define NFC_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun, TRACE_FLASHDEVICE_RESULT_MODULE_ID,TRACE_FLASHDEVICE_RESULT_SUBMODULE_ID,log_level,x, __VA_ARGS__);

extern volatile NFC_CMD_STS_REG *p_flash_cq_dptr;
extern NF_CQ_REG_CMDTYPE *p_flash_cq_dptr_cmdtype;
/**added by vigoss zhang for pu acc 2013.7.9**/
extern NFC_LOGIC_PU *p_flash_pucr_reg;
extern NF_HPUST_REG *p_flash_hpust_reg;
extern NF_LPUST_REG *p_flash_lpust_reg;
extern NF_PUACC_TRIG_REG *p_flash_puacc_trig_reg;
//extern NF_PUFS_REG *p_flash_pufs_reg;
extern NF_PUFSB_REG *p_flash_pufsb_reg;

extern NFCQ_ENTRY *p_flash_cq_entry;
//extern NF_RAWQ_ENTRY *p_flash_raw_cmdq;
extern U32 g_PG_SZ;//phy page size
extern U32 g_RED_SZ_DW;//red size for each phy page
extern U32 g_CACHE_STATUS_BASE;//offset in OTFB
extern U32 g_SSU_BASE;//offset in OTFB
extern U32 g_SSU1_BASE;//offset in OTFB
#define NFC_CONFIG_FAST_SLOW_PAGE
#define NFC_CONFIG_SLOW_PAGE     0
#define NFC_CONFIG_FAST_PAGE     1
#define NFC_CONFIG_NORMAL_PAGE   2

#if defined(FLASH_L85) || defined(FLASH_L95)
#define WL_ADDR_MSK 0x1FF
#define WL_ADDR_BITS 9
#define PAGE_PER_WL 1
#define BLK_REAL_BITS 11
#define BLK_REAL_MSK  0x7FF
#endif

#ifdef FLASH_TSB
#define WL_ADDR_MSK 0xFF
#define WL_ADDR_BITS 8
#define PAGE_PER_WL 1
#define BLK_REAL_BITS 12
#define BLK_REAL_MSK  0xFFF
#endif

#define RED_DW_COUNT                (RED_SZ_DW*PLN_PER_PU*PAGE_PER_WL)

//flash configuration

#define  DEFAULT_NFC_REG_PG_CFG 0xf206246d                                                              
#define  DEFAULT_NFC_REG_TCTRL0 0x054080a4
#define  DEFAULT_NFC_REG_TCTRL1 0xc0004112
#define  DEFAULT_NFC_REG_RECC_INJ 0x0
#define  DEFAULT_NFC_REG_WECC_INJ 0x0
#define  DEFAULT_NFC_DEVICE_CFG     0x15

#define NFC_BUFMP_1BIT_MSK    0xff             //1bit for bufmap stands for 4k



//NFC row address config
#define NFC_ROW_ADDR_PG_MSK            0xFF
#define NFC_ROW_ADDR_BLK_OFFSET        9
#define NFC_ROW_ADDR_PLN_OFFSET        8
#define NFC_ROW_ADDR_BLK_MSK           0xFFFF
#define NFC_ROW_ADDR_PLN_MSK           0x1

U8 flash_config_pg_type[PG_PER_BLK];          //config fast and slow pages

#endif

