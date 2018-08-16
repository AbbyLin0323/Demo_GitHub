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

extern volatile NFC_CMD_STS     *g_pModelNfcCmdSts;
extern volatile PG_CONF_REG     *g_pModelNfcPageConfigReg;
extern volatile NF_LLUNST_REG   *g_pModelLogicLunStsReg;
extern volatile NF_LLUN_SW_BITMAP_REG   *g_pModelLogicLunSwStsReg;
extern volatile NFCQ *g_pModelNfcq;
extern volatile NFC_TRIGGER *g_pModelNfcTriggerReg;
/**added by vigoss zhang for pu acc 2013.7.9**/
extern NFC_LOGIC_PU *p_flash_pucr_reg;
extern NF_HPUST_REG *p_flash_hpust_reg;
extern NF_LPUST_REG *p_flash_lpust_reg;
//extern NF_PUACC_TRIG_REG *p_flash_puacc_trig_reg;
//extern NF_PUFS_REG *p_flash_pufs_reg;
//extern NF_PUFSB_REG *p_flash_pufsb_reg;

//extern NF_RAWQ_ENTRY *p_flash_raw_cmdq;
extern U32 g_PG_SZ;//phy page size
extern U32 g_RED_SZ_DW;//red size for each phy page

extern U32 g_SSU_BASE;//offset in OTFB
extern U32 g_SSU1_BASE;//offset in OTFB
#define NFC_CONFIG_FAST_SLOW_PAGE
#define NFC_CONFIG_SLOW_PAGE     0
#define NFC_CONFIG_FAST_PAGE     1
#define NFC_CONFIG_NORMAL_PAGE   2

/*  replace by abby
#ifdef FLASH_TLC
#define PAGE_PER_WL         PG_PER_WL
#else
#define PAGE_PER_WL         1
#endif
*/
#define PAGE_PER_WL         INTRPG_PER_PGADDR

//#define BLK_ADDR_MSK        ((1 << g_TotalBlkBits) - 1)

//#define RED_DW_COUNT        (RED_SZ_DW*PLN_PER_LUN*PAGE_PER_WL)

//flash configuration

#define  DEFAULT_NFC_REG_PG_CFG     0xf206246d
#define  DEFAULT_NFC_REG_TCTRL0     0x054080a4
#define  DEFAULT_NFC_REG_TCTRL1     0xc0004112
#define  DEFAULT_NFC_REG_RECC_INJ   0x0
#define  DEFAULT_NFC_REG_WECC_INJ   0x0
#define  DEFAULT_NFC_DEVICE_CFG     0x15

#define NFC_BUFMP_1BIT_MSK          0xff    //1bit for bufmap stands for 4k



//NFC row address config
#define NFC_ROW_ADDR_PG_MSK         0xFF
#define NFC_ROW_ADDR_BLK_OFFSET     9
#define NFC_ROW_ADDR_PLN_OFFSET     8
#define NFC_ROW_ADDR_BLK_MSK        0xFFFF
#define NFC_ROW_ADDR_PLN_MSK        0x1

U8 flash_config_pg_type[PG_PER_BLK * PAGE_PER_WL];          //config fast and slow pages


typedef struct NFCM_LUN_LOCATION_
{
    U8 ucCh;               // Channel number
    U8 ucPhyPuInCh;        // NFC PU number in channel
    U8 ucLunInPhyPu;       // LUN number in NFC PU
} NFCM_LUN_LOCATION;

/* For set DEC status */
typedef struct _PLANE_ERR_TYPE
{
    U8 ErrCode;
    U8 ErrSts;
    BOOL IsEmptyPG;

}PLANE_ERR_TYPE;

#endif

