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
* File Name    : Proj_Config.h
* Discription  : 
* CreateAuthor : VIA
* CreateDate   : 2016.3.19
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _PROJ_CONFIG_H
#define _PROJ_CONFIG_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
/******************************************************************************/
/* Platform definitions: defined in MakeFile                                  */
/******************************************************************************/
//#define SIM
//#define XTMP
//#define COSIM
//#define FPGA
//#define ASIC

/******************************************************************************/
/* Host definitions: defined in MakeFile                                      */
/******************************************************************************/
//#define HOST_SATA
//#define HOST_NVME

/******************************************************************************/
/* Chip definitions: defined in MakeFile                                      */
/******************************************************************************/
//#define VT3533_A0

/******************************************************************************/
/* HW Function selection: defined in MakeFile                                 */
/******************************************************************************/
//#define DMAE_ENABLE
//#define DCACHE
//#define SEARCH_ENGINE
//#define FLASH_CACHE_OPERATION
//#define CE_DECODE
//#define DRAMLESS_ENABLE
//#define LDPC_SOFT_DEC_TEST
//#define XOR_ENABLE
//#define READ_DISTURB_OPEN

/******************************************************************************/
/* FW unit test selection: defined in MakeFile                                */
/******************************************************************************/
//#define L1_FAKE           // L0 + L1 Ramdisk
//#define L2_FAKE           // L0 + L1 + L2 Ramdisk
//#define L3_UNIT_TEST      // UnitTestCase + L3
//#define HAL_UNIT_TEST     // UnitTestCase + HAL  If use by MPTool, also must open L1_FAKE.

/******************************************************************************/
/* NFC & FLASH unit test selection                                */
/******************************************************************************/
#define FLASH_TEST          //For Flash Characters Test
//#define HAL_NFC_TEST        //For NFC UT 2.0

#ifdef HAL_NFC_TEST
//#define RAW_DATA_RD           
//#define BYPASS_UECC       //for L3
//#define DATA_EM_ENABLE          //feature selection for EM
//#define UT_QD1
//#define FLASH_IM_3DTLC_GEN2
#endif

#ifdef FLASH_TEST
//#define OTP_TEST
//#define SLC_SHIFT_ADDR_TEST     //check SLC mode with or without shift page address

#endif

//#define P_DBG

/******************************************************************************/
/* Compilce Related Macro: defined in MakeFile                                */
/* For some HAL code, like DMAE, is shared by MCU0 and MCU1,2.                */
/* e.g: the "MCU0" MACRO is used to set dedicated linker attribute for MCU0.  */
/******************************************************************************/
//#define MCU0

/******************************************************************************/
/* SUBSYSTEM_PU_NUM is FW Porcess Unit number, and its value should equal with*/
/* the phycial CE Number                                                      */
/******************************************************************************/
#define SUBSYSTEM_PU_MAX    32         // System max support 32 PU (CE_MAX)
#define SUBSYSTEM_PU_NUM    g_ulPuNum  // define in Disk_Config.c (get value from P-Table)
#define LIMITED_LUN_TO_ERASE
#define LDPC_CENTER_RECORD      //abby add for debug
//#define ABBY_UECC_DBG

#ifdef SIM  //board.mk setting
//#define IM_3D_TLC_B16A
//#define IM_3D_TLC_B17A
#endif

#ifdef IM_3D_TLC_B16A
#define FLASH_IM_3DTLC_GEN2
#endif

#ifdef IM_3D_TLC_B17A
#define FLASH_IM_3DTLC_GEN2
#define FLASH_IM_3DTLC_GEN2_B17A
#endif

#ifdef FLASH_IM_3DTLC_GEN2
//#define SNAP_READ_EN
#define LUN_GROUP_MAX       8          // Limited lun number as a group to erase for max degree of power
#else
#define LUN_GROUP_MAX       6          // Limited lun number as a group to erase for max degree of power
#endif

/******************************************************************************/
/* Debug use : After TLC block erase finish, Read and check N1(N0 count) of each page */
/******************************************************************************/
//#define SCAN_BLOCK_N1
#ifdef SCAN_BLOCK_N1
#define EMPTY_PAGE_INFO
#define ERASE_N0_CHK // add by abby to check N0 with read offset
//#define DOUBLE_ERASE_EN
#endif

//#define ERASE_DELAY


/******************************************************************************/
/* Read Retry definitions                                                       */
/******************************************************************************/
//#define READ_RETRY_REFACTOR

/******************************************************************************/
/* HW Patch definitions                                                       */
/******************************************************************************/
#ifdef HOST_NVME
//#define AF_ENABLE
#ifdef SIM  //Enable AF_Model for winSim
//#define AF_ENABLE_M
#endif //SIM
#endif //HOST_NVME

/* Disable several uart print can speed up unh-iol test up to 3 minutes */
#if (defined(ASIC) && defined(HOST_NVME))
//#define SPEEDUP_UNH_IOL

/* Firmware would get involved into PCIe ASPM control and prevents link from entering ASPM L0s/L1 state when there are host commands pending. */
#define PCIE_ASPM_MANAGEMENT
#define L0_DSG_CACHESTS_IN_OTFB
#endif

// VT3533_A0: fw patch the hw bug in asic. the sge will report the same dsg-id to two different mcus in a corner case.
#ifdef ASIC
//#define VT3533_A2ECO
#ifdef VT3533_A2ECO
#define PWR_LOSS_WRITE_PROTECT
#define VT3533_A2ECO_DSGRLSBUG
#define VT3533_A2ECO_HOSTCPFMC
#define VT3533_A2ECO_PCIE_128BDATAERR
#endif
#endif

/******************************************************************************/
/* FW function selection                                                      */
/******************************************************************************/
#if (defined (ASIC) && defined (MCU0))
/* We should calculate the clock period of ring OSC first if we decide to use it. */
#define CALC_RINGOSC_CLK

/* Whether to enable PLL root gating in Active Idle state. */
#ifdef HOST_SATA
#define PM_STANDBY_SUPPORT
#else
#define PM_STANDBY_SUPPORT
#endif
#endif

//#define L1_BUFFER_REGULATION

// SLC-Merge-To-TLC three step operation
//#define TRI_STAGE_COPY
//#define TRI_STAGE_NUM  (3)
//#define LOAD_AT0_ACCELERATE    //load DPBM,RPMT,VBMT not using buffer

#ifdef HOST_SATA
// please note that we should always enable L0_HCMD_FIFO when in sata mode,
// since the host command selection mechanism in L0 is completely outdated now
// and shouldn't be used
#define L0_HCMD_FIFO   
#endif

#ifndef MCU0
//#define PMT_ITEM_SIZE_3BYTE     //feature selection of PMT table size 1G - 1M
//#define DATA_MONITOR_ENABLE     //B0 only support software Gloden Disk data check, C0 support NFC HW data check
//#define DATA_EM_ENABLE          //feature selection for EM 
#endif

#ifndef SWL_OFF
#define SWL_CPV_OFF         //disable swl copy valid
#endif
#define ERRH_MODIFY_PENDING_CMD
//#define L2MEASURE
#define L2_PMTREBUILD_SUPERPAGETS_NOTSAME

#ifdef SIM
//#define RDT_SORT
#define PMT_ITEM_SIZE_REDUCE     //Function: reduce DRAM overhead of PMT table
#define UECC_SOFT_DECODE_EN
#if (!(defined(L1_FAKE) && !defined(HAL_UNIT_TEST)) && !defined(L2_FAKE) && !defined(L3_UNIT_TEST) && !defined(RDT_SORT))
#define DBG_TABLE_REBUILD        //Windows SIM SPOR used Table Rebuild data check
#define DBG_PMT                  //Windows SIM POR/SPOR used PMT data check
#define DBG_LC                   //windows sim debug lc
#define SEARCH_ENGINE
#define CACHE_LINK_MULTI_LCT     //Function: reduce DRAM overhead of L1 LCT depth 
#define NEW_SWL                  //Function: open/close new swl algorithm
//#define SWL_EVALUATOR            //For SWL Info Collections
#ifndef HAL_UNIT_TEST
#define DATA_EM_ENABLE
#endif
#endif

#ifdef AF_ENABLE
#define AF_ENABLE_M
#endif

#endif

#ifdef XTMP
#define SUBSYSTEM_BYPASS_LLF_BOOT //Bypass SubSystem LLF and Boot in XTMP env
#endif

#define DIRECT_TRIM

#ifdef DIRECT_TRIM
#define NEW_TRIM
#define LCT_VALID_REMOVED
#define LCT_TRIM_REMOVED
#define ValidLPNCountSave_IN_DSRAM1
#define DirtyLPNCnt_IN_DSRAM1
#endif

#define L2_HANDLE_UECC
//#define SHUTDOWN_IMPROVEMENT_STAGE2
//#define SHUTDOWN_STAGE2_WORDLINE_CLOSED
//#define SPOR_DEBUGLOG_ENABLE
#define DEBUG_NOTLCFREEBLK

/******************************************************************************/
/* print/halt/assert for FW debug                                             */
/******************************************************************************/
#ifdef SIM
#include <stdio.h>
#include "system_statistic.h"
#define DBG_Printf printf
#define DBG_Getch  __debugbreak
#define ASSERT( _x_ ) { if (FALSE == (_x_) ) __debugbreak();}
#else
#define FIRMWARE_LogInfo(format, ...) (void)0

#ifdef COSIM
#define DBG_Printf(x,...)
#else
#define DBG_Printf dbg_printf
#endif

extern void DBG_Getch();

#define ASSERT( _x_ )\
    do{\
        if ( (_x_) == FALSE )\
        {\
            DBG_Printf("ASSERT:%s LINE#%d\n", __FUNCTION__, __LINE__); \
            while(1); \
        }\
    }while(0)
#endif

//add by abby for debug
//#include "HAL_TraceLog.h"
extern volatile unsigned int g_ulPMCurTime, g_ulDiffCycle;

#define PM(_s_) do \
    {\
        g_ulDiffCycle = COM_DiffU32(g_ulPMCurTime, HAL_GetMCUCycleCount());\
        DBG_Printf(_s_, g_ulDiffCycle);\
        g_ulPMCurTime = HAL_GetMCUCycleCount();\
    }while(0)


#endif
/*====================End of this head file===================================*/

