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
  File Name     : host_api_define.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the basic data structure and macro for host api.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _HOST_API_DEFINE
#define _HOST_API_DEFINE
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "stdlib.h"

typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;

#define MAX_DISK_NAME_LEN 80

#define MAX_DISK 32
#define MAX_UART 32
#define PHY_PG_SZ (16*1024)
#define SEC_SIZE (512)
#define MAX_DMA_TRANS_SIZE (8*1024)
#define HOST_BUFF_SIZE     (32*1024)
#define	DBG_printf(x, ...)	printf(x, __VA_ARGS__)
#define	DBG_Getch   _getch
#define MAX_FLASH_PG_SZ 128*1024
#define MAX_MICROCODE_TRANS_SECCNT (32*1024)/SEC_SIZE
typedef enum _OP_CODE
{
	OP_MEM_READ = 1,
	OP_MEM_WRITE,
	OP_FLASH_READ,
	OP_FLASH_WRITE,
	OP_FLASH_ERASE,
	OP_GET_VAR_TABLE = 8,
	OP_JUMP,
	OP_L2_FORMAT,
	OP_L3_FORMAT,
	OP_REG_READ,
	OP_REG_WRITE,
	OP_TRACELOG_CONTROL,
	OP_BBT_SAVE,
	OP_BBT_LOAD,
  OP_GET_IDB,
  OP_FW_SAVE,
  OP_FW_LOAD,
	OP_FW_ACTIVE,
	OP_DBG_SHOWALL,
    OP_SPI_WRITE,
    OP_SPI_READ,
    OP_CLEAR_DISKLOCK,
	VIA_CMD_FLASH_PRECONDITION,
	VIA_CMD_FLASH_SETPARAM,
	VIA_CMD_FLASH_TERMINATE,
	VIA_CMD_FLASH_RESET
}OP_CODE;
#define OP_UART_STAT 3
#define OP_UART_CLEAR_STAT 4
#define MULTI_PLN 0xff
typedef enum _CPUID
{
	MCU0 = 1,
	MCU1,
	MCU2
}CPUID;
typedef enum _DISKTYPE{
    AHCI = 0,
	NVME,
    SATA,
	UART,
    USB_BRIDGE_SATA,
	DISK_TYPE_CNT
}DISKTYPE;
typedef enum _STATUS{
	RETURN_SUCCESS = 0,
	RETURN_FAIL
}STATUS;
typedef enum _FLASH_OPCODE{
    HOST_FLASH_READ = 0,
	HOST_FLASH_WRITE,
	HOST_FLASH_ERASE,
	HOST_FLASH_RESET,
	HOST_FLASH_PRECONDITION,
	HOST_FLASH_TERMINATE,
	HOST_FLASH_SETPARAM
}FLASH_OPCODE;
typedef enum _TL_CTL
{
	TL_CTL_DISABLE = 0,
	TL_CTL_ENABLE,
	TL_SAVE_TRACE_TO_FLASH,
	TL_UPDATE_INVALID
}TL_CTL;
typedef STATUS(*Fun_Read_Dram_Sram)(HANDLE hDevice, U8 cpuid, U32 startaddr, U32 length, U8 * readbuf);
typedef STATUS(*Fun_Write_Dram_Sram)(HANDLE hDevice, U8 cpuid, U32 startaddr, U32 length, U8 * writebuf);
typedef STATUS(*Fun_Read_Flash)(HANDLE hDevice, U8 cpuid, U8 plnmode, U8 pln, U16 block, U16 page, U8 *pStatusBuf, U32 pumsk);
typedef STATUS(*Fun_Write_Flash)(HANDLE hDevice, U8 cpuid, U8 plnmode, U8 pln, U16 block, U16 page, U8 *pStatusBuf, U32 pumsk);
typedef STATUS(*Fun_Erase_Flash)(HANDLE hDevice, U8 cpuid, U8 plnmode, U8 pln, U16 block, U8 *pStatusBuf, U32 pumsk);

typedef STATUS(*Fun_NoneData)(HANDLE hDevice, U8 ucCpuId, U8 ucOpCode, U32 ulInParam1, U32 ulInParam2, U32 * pOutParam1, U32 * pOutParam2);
typedef STATUS(*Fun_Set_Uart_Baud)(HANDLE hDevice, U32 ulSpeedDev, U32 ulSpeedHost);
typedef STATUS(*Fun_Get_Identify_Data)(HANDLE hDevice, U8 *readbuf);
//STATUS Nvme_Fw_Download(HANDLE hDevice, U32 ulLength, U8 * pFwBuf)
typedef STATUS(*Fun_Fw_Download)(HANDLE hDevice, U32 ulLength, U8 * pFwBuf);
typedef STATUS(*Fun_Get_ModelNumber)(HANDLE hDevice, U8 * identifybuf, U8 mn[40]);
typedef STATUS(*Fun_Get_SmartInfo)(HANDLE hDevice, U8 *readbuf);


typedef STATUS(*Fun_Standard_Cmd_Dma_Read)(HANDLE hDevice, U32 lba, U8 * inbuf_addr, U32 trans_sec_cnt);
typedef STATUS(*Fun_Standard_Cmd_Dma_Write)(HANDLE hDevice, U32 lba, U8 * outbuf_addr, U32 trans_sec_cnt);
#define CACULATE_PG_PLN(page,pln,ucpln) ((page>>8)|((pln&0xf)<<1)|((befullpln&0x1)<<5))


#define MAX_SUPPORT_FLASH_CNT 64
#define MAX_PHY_PU_ID 64
#define MAX_FLASH_SUBSYS_CNT_BITS 1
#define MAX_FLASH_SUBSYS_CNT (1<<MAX_FLASH_SUBSYS_CNT_BITS)
#define PLN_RED_SIZE 64
#define MAX_PLN_NUM  4

/* NF command status define, sync with HW spec */
#define NF_SUCCESS                    0  // No error
#define NF_ERR_TYPE_USUP              1  // Un-recognized command/Invalid command
#define NF_ERR_TYPE_PRG               2  // Program error
#define NF_ERR_TYPE_ERS               3  // Erase error
#define NF_ERR_TYPE_UECC              4  // ECC unrecoverable error
#define NF_ERR_TYPE_RECC              5  // ECC recoverable error with error counter>threshold
#define NF_ERR_TYPE_NO_DEV            6  // No device detected
#define NF_ERR_TYPE_EMPTY_PG          7  // No error with empty page
#define NF_ERR_TYPE_EMPTY_PG_E0       8  // Some sector is empty,but some sector detect "0" bits
#define NF_ERR_TYPE_CRC               9  // CRC error when ECC_MSK on
#define NF_ERR_TYPE_TWO_UECC          12 // data & check sum all error 

typedef struct _VD_NFC_STATUS
{
    U32 ulStatus;
} VD_NFC_STATUS;
typedef struct _FLASH_INFO{
	U8 ucPhyPu;
	U8 ucSubFlashSysId;
	U8 ucPuInSubSys;
	U8 * ulHostFlashDataAddr;
	U8 * ulHostFlashRedAddr;
	U8 * ulHostFlashStatusAddr;
}LOGIC_PU_ENTRY, *PLOGIC_PU_ENTRY;

typedef struct _FLASH_GROUP_INFO{
	U8 * ulHostFlashDataAddr;
	U8 * ulHostFlashStatusAddr;
	U8 * ulHostFlashRedAddr;
	LOGIC_PU_ENTRY LogicPuInfoEntry[MAX_SUPPORT_FLASH_CNT];
}FLASH_GROUP_INFO, *PFLASH_GROUP_INFO;

//#define MAX_SUPPORT_FLASH_CNT 64


typedef union _VAR_HEAD_L0_TABLE{
	U8 m_content[1024];
	struct{
		U32 ucSubSysCnt;
		U32 ulTLInfoAddr;

		U32 ulCmdHeaderAddr;
		U32 ulCmdHeaderSize;
		U32 ulCmdTableAddr;
		U32 ulCmdTableSize;

		U32 ulWbqBaseAddr;
		U32 ulWbqBaseSize;
		U32 ulFcqBaseAddr;
		U32 ulFcqBaseSize;
		U32 ulDrqBaseAddr;
		U32 ulDrqBaseSize;
		U32 ulDwqBaseAddr;
		U32 ulDwqBaseSize;
		U32 ulDsgBaseAddr;
		U32 ulDsgBaseSize;
		U32 ulHsgBaseAddr;
		U32 ulHsgBaseSize;

		/* add for FW debug Infos */
		U32 ulHInfoBaseAddr;
		U32 ulHInfoBaseSize;
		U32 ulHwTraceBaseAddr;
		U32 ulHwTraceBaseSize;

	};
}VAR_HEAD_L0_TABLE;
typedef union _VAR_HEAD_PARAM_TABLE{
	U8 m_content[1024 * 4];
}VAR_HEAD_PARAM_TABLE;
typedef union _VAR_HEAD_FLASH_INFO{
	U8 m_content[1024];
	struct{
		U32 ucPuNum;
		U32 ucPageNum;
		U32 ucBlkNum;
		U32 ucPlnNum;
		U32 ucPhyPageSize;
		U32 ulFlashId[2];
		
	};
	
}VAR_HEAD_FLASH_INFO;
typedef union _VAR_HEAD_TABLE{
	U8 m_content[8 * 1024];
	struct {
		//VAR_HEAD_FLASH_INFO VarHeadFlashInfo;
		VAR_HEAD_PARAM_TABLE VarHeadParamTable;
		VAR_HEAD_L0_TABLE VarHeadL0Table;
	};
}VAR_HEAD_TABLE;
typedef union _VAR_SUBSYS_TABLE{
	U8 m_content[4 * 1024];

	struct{
		/*flash realated item*/
		U32 ucPuNum;
		U32 ucPageNum;
		U32 ucBlkNum;
        U32 ulRsvBlkNum;
		U32 ucPlnNum;
		U32 ucPhyPageSize;
		U32 ulFlashId[2];

		U32 ulPhyPuMskLow;
		U32 ulPhyPuMskHigh;
		U32 ulDevFlashRedAddr;
		U32 ulDevFlashDataAddr[32];

		/*hw engine realated item*/
		U32 ulDrqBaseAddr;
		U32 ulDrqBaseSize;
		U32 ulDwqBaseAddr;
		U32 ulDwqBaseSize;
		U32 ulDsgBaseAddr;
		U32 ulDsgBaseSize;
		U32 ulHsgBaseAddr;
		U32 ulHsgBaseSize;

		

		/*subsys realated item*/
		U32 ulL1TableAddr;
		U32 ulL2PbitTableAddr;
		U32 ulL2VbtTableAddr;
		U32 ulL3BbtTableAddr;

		/* add for FW debug Infos */
		U32 ulDParamBaseAddr;
		U32 ulDParamBaseSize;
		U32 ulFlashMonitorAddr[16];
		U32 ulFlashMonitorSize[16];
		U32 ulRTBaseAddr;
		U32 ulRTBaseSize;
		U32 ulPBITBaseAddr[16];
		U32 ulPBITBaseSize[16];
		U32 ulVBTBaseAddr[16];
		U32 ulVBTBaseSize[16];

        U32 ulSuperPuNum;
        U32 ulLunInSuperPu;
	};
}VAR_SUBSYS_TABLE;
typedef union _VAR_TABLE{
	U8 m_content[HOST_BUFF_SIZE];
	struct{
		VAR_HEAD_TABLE VarHeadTable;
		VAR_SUBSYS_TABLE SubSysTable[MAX_FLASH_SUBSYS_CNT];
	};
}VAR_TABLE, *PVAR_TABLE;
typedef struct _PCI_POS{
    U8 bus;
    U8 slot;
    U8 fun;
}PCI_POS;
#define MAX_SUB_SYS_CNT 2
#define MAX_BBT_SIZE (128*1024)
typedef struct _DEVICE_OBJECT
{
	HANDLE Handle;
	DISKTYPE Disktype; 
    PCI_POS PciPos;
	U8 mn[80];
	VAR_TABLE VarTable;
	FLASH_GROUP_INFO FlashInfo;
    TCHAR cSymbolicName[256];
    TCHAR cPosition[256];
	U8 BbtBuf[MAX_SUB_SYS_CNT][MAX_BBT_SIZE];
	Fun_Read_Dram_Sram Hal_Read_Dram_Sram;
	Fun_Write_Dram_Sram Hal_Write_Dram_Sram;
	Fun_Read_Flash Hal_Read_Flash;
	Fun_Write_Flash Hal_Write_Flash;
	Fun_Erase_Flash Hal_Erase_Flash;
	Fun_NoneData Hal_NoneData;
	Fun_Get_Identify_Data Hal_Get_Identify_Data;
	Fun_Get_ModelNumber Hal_Get_ModelNumber;
    Fun_Get_SmartInfo HaL_Get_SmartInfo;
	Fun_Fw_Download Hal_Fw_Download;
	Fun_Standard_Cmd_Dma_Read Hal_Standard_Cmd_Dma_Read;
	Fun_Standard_Cmd_Dma_Write Hal_Standard_Cmd_Dma_Write;
	Fun_Set_Uart_Baud Hal_Set_Uart_Baud;
}DEVICE_OBJECT , *PDEVICE_OBJECT;
extern U8 g_flash_host_data_addr[MAX_DISK][MAX_SUPPORT_FLASH_CNT][MAX_FLASH_PG_SZ];
extern U8 g_flash_host_red_addr[MAX_DISK][MAX_FLASH_SUBSYS_CNT*MAX_FLASH_PG_SZ];
extern U8 g_flash_host_status_addr[MAX_DISK][MAX_FLASH_SUBSYS_CNT*MAX_FLASH_PG_SZ];
extern DEVICE_OBJECT g_dev_obj[MAX_DISK];

typedef struct _TL_INFO{
    U32 ulTlVersion;  //Trace Log version number
    U32 ulTlMemBase;  //Trace Log memory base in MCU view
    U32 ulTlMemSize;  //Trace Log memory size in byte unit
    U32 ulTlSecSize;  //size of every Trace Log sector, eg. 512Byte

    U32 ulValidSecCnt;//number of sector which are filled with log data.
                      //no matter the sector is full or not
    U32 ulValidFlushCnt;//number of sector which are filled with not flushed log data 

    U32 ulWriteSec;   //index of sector which will be written with log data
    U32 ulReadSec;    //index of sector from which host can get valid log data
    U32 ulFlushSec;   //index of sector has flush into flash(Trace block)
}TL_INFO;

/*add RED define*/
typedef enum _BLOCK_TYPE
{
    BLOCK_TYPE_GB = 0,
    BLOCK_TYPE_BBT,
    BLOCK_TYPE_RT,
    BLOCK_TYPE_AT0,
    BLOCK_TYPE_PMT,
    BLOCK_TYPE_TRACE,
    BLOCK_TYPE_RSVD,
    BLOCK_TYPE_SEQ,
    BLOCK_TYPE_RAN,
    BLOCK_TYPE_SLC,
    BLOCK_TYPE_EMPTY
} BLOCK_TYPE;

typedef enum _PAGE_TYPE
{
    PAGE_TYPE_GB = 0,
    PAGE_TYPE_BBT,
    PAGE_TYPE_RT,
    PAGE_TYPE_PMTI,
    PAGE_TYPE_VBMT,
    PAGE_TYPE_PBIT,
    PAGE_TYPE_VBT,
    PAGE_TYPE_RPMT,
    PAGE_TYPE_DPBM,
    PAGE_TYPE_PMT,
    PAGE_TYPE_TRACE,
    PAGE_TYPE_DATA,
    PAGE_TYPE_RSVD
} PAGE_TYPE;

typedef enum _OPERATION_TYPE
{
    OP_TYPE_GB_WRITE = 0,
    OP_TYPE_BBT_WRITE,
    OP_TYPE_RT_WRITE,
    OP_TYPE_PMTI_WRITE,
    OP_TYPE_VBMT_WRITE,
    OP_TYPE_PBIT_WRITE,
    OP_TYPE_VBT_WRITE,
    OP_TYPE_RPMT_WRITE,
    OP_TYPE_DPBM_WRITE,
    OP_TYPE_PMT_WRITE,
    OP_TYPE_TRACE_WRITE,
    OP_TYPE_HOST_WRITE,
    OP_TYPE_GC_WRITE,
    OP_TYPE_WL_WRITE,
    OP_TYPE_DUMMY_WRITE,

    OP_TYPE_GB_READ,
    OP_TYPE_BBT_READ,
    OP_TYPE_RT_READ,
    OP_TYPE_PMTI_READ,
    OP_TYPE_VBMT_READ,
    OP_TYPE_PBIT_READ,
    OP_TYPE_VBT_READ,
    OP_TYPE_RPMT_READ,
    OP_TYPE_DPBM_READ,
    OP_TYPE_PMT_READ,
    OP_TYPE_TRACE_READ,
    OP_TYPE_HOST_READ,
    OP_TYPE_GC_READ,
    OP_TYPE_WL_READ,

    OP_TYPE_GB_ERASE,
    OP_TYPE_BBT_ERASE,
    OP_TYPE_RT_ERASE,
    OP_TYPE_PMTI_ERASE,
    OP_TYPE_VBMT_ERASE,
    OP_TYPE_PBIT_ERASE,
    OP_TYPE_VBT_ERASE,
    OP_TYPE_RPMT_ERASE,
    OP_TYPE_DPBM_ERASE,
    OP_TYPE_PMT_ERASE,
    OP_TYPE_TRACE_ERASE,
    OP_TYPE_HOST_ERASE,
    OP_TYPE_GC_ERASE,
    OP_TYPE_WL_ERASE,
    OP_TYPE_ALL,
} OP_TYPE;

/* Redundant common head defines */
typedef struct _REDUNDANT_COMM_
{
    /* Common Info : 3 DW */
    U32 ulTimeStamp;
    U32 ulTargetOffsetTS;

    U32 bsVirBlockAddr : 16;
    BLOCK_TYPE bcBlockType : 8;
    PAGE_TYPE  bcPageType : 8;
    OP_TYPE    eOPType;

#ifdef SIM
    U32 ulMCUId;
#endif
}RedComm;

#define   LPN_PER_BUF            (8)
#define   PMT_DIRTY_BM_SIZE      (0xf)

typedef struct _DATA_REDUNDANT_
{
    /* Data Block */
    U32 aCurrLPN[LPN_PER_BUF];
}DataRed;

/* Redundant defines */
typedef union _REDUNDANT
{
    U8 m_content[MAX_PLN_NUM][PLN_RED_SIZE];
    struct 
    {
        /* Common Info : 3 DW */
        RedComm m_RedComm;
        
        /* private data */
        union
        {
            /* Global Block */
            U32 SubPageType;

            /* BBT */
            struct  
            {
                U32 bbt_sn;
                U32 pu_msk[2];
            };

            /* Root Table */
            struct  
            {
                BOOL bPowerCycle;
            };

            /* AT0 Table */
            U32 m_PageIndex;

            /* AT1 PMT Table */
            struct  
            {
                U32 m_PMTIndex;
                U32 m_DirtyBitMap[PMT_DIRTY_BM_SIZE];
                U32 m_ValidLPNCountSave;
            };

            /* Trace Block */
            struct  
            {
                U32 m_McuId;
                U32 m_TraceSN;
                U32 m_TraceItemCNT;
                TL_INFO m_TLInfo;
            };

            /* Data Block */
            DataRed m_DataRed;
        };
    };
}RED;


#endif
