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
* File Name    : L0_ViaCmd.h
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2015.1.13
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L0_VIACMD_H
#define _L0_VIACMD_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"

/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/
/* VIA OpCode definition */
typedef enum _ViaCmdCode
{
    /* VIA raw data req command */
    VIA_CMD_NULL = 0,
    VIA_CMD_MEM_READ,
    VIA_CMD_MEM_WRITE,
    VIA_CMD_FLASH_READ,
    VIA_CMD_FLASH_WRITE,
    VIA_CMD_FLASH_ERASE,

    /* via device control command */
    VIA_CMD_VAR_TABLE = 8,
    VIA_CMD_JUMP,
    VIA_CMD_L2_FORMAT,
    VIA_CMD_L3_FORMAT,
    VIA_CMD_REG_READ,
    VIA_CMD_REG_WRITE,
    VIA_CMD_TRACELOG_CONTROL,
    VIA_CMD_BBT_SAVE,
    VIA_CMD_BBT_LOAD,
    VIA_CMD_GET_IDB,
    VIA_CMD_FW_SAVE,
    VIA_CMD_FW_LOAD,
    VIA_CMD_FW_ACTIVE,
    VIA_CMD_DBG_SHOWALL,
    VIA_CMD_SPI_FLASH_WRITE,
    VIA_CMD_SPI_FLASH_READ,
    VIA_CMD_CLEAR_DISK_LOCK,
    VIA_CMD_FLASH_PRECONDITION,
    VIA_CMD_FLASH_SETPARAM,
    VIA_CMD_FLASH_TERMINATE,
    VIA_CMD_FLASH_RESET,
    VIA_CMD_FLASH_NONE_DATA_READ,
	VIA_CMD_FLASH_NONE_DATA_WRITE,
	VIA_CMD_FLASH_NONE_DATA_ERASE,
	VIA_CMD_FLASH_NONE_DATA_GET_STAT,
	OP_DISABLE_PMU,
	OP_ENABLE_PMU
    
}VIA_CMD_CODE;

typedef enum _trace_log_control_type
{
    TLCT_DISABLE_TL = 0,
    TLCT_ENABLE_TL,
    TLCT_FLUSH_DATA,
    TLCT_INVALID_DATA,
}VIA_TLCT;

typedef enum _ViaCmdStatus
{
    VCS_SUCCESS = 0,
    VCS_WAITING_RESOURCE,
    VCS_CMD_CODE_ERROR,
    VCS_MCUID_ERROR,
    VCS_MEM_RANGE_ERROR,
    VCS_FLASH_PARAM_ERROR,
    VCS_ADDR_NOT_DW_ALIGN,
    VCS_DATALEN_NOT_DW_ALIGN,
    VCS_DATALEN_OVERFLOW,
    VCS_INVALID_PARAM,
    VCS_OPERATION_FAIL
}VIA_CMD_STATUS;


#define FULL_PLANE_MODE     0
#define SINGLE_PLANE_MODE   1

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

typedef struct _ViaCmdParam
{
    union
    {
        U32  aDW[3];    /* total 3DWs, only low 9 bytes can be used */
        U8   aByte[9];

        /* for Flash Read/Write/Erase */
        struct tFlashAccess
        {
            /* DW 0*/
            U32 ulPuMask;

            /* DW 1*/
            U32 bsBlock:16;
            U32 bsPage:9;
            U32 bsPlane:4;
            U32 bsPlaneMode:1;
            U32 bsRsv1:2;

            /* DW 2 */
            U32 bsMcuID:4;
            U32 bsRsv2:28;
        }tFlashAccess;

        /* for Mem read/write */
        struct tMemAccess
        {
            /* DW 0 */
            U32 ulDevAddr;

            /* DW 1 */
            U32 bsByteLen:16;
            U32 bsRsv1:16;

            /* DW 2 */
            U32 bsMcuID:4;
            U32 bsRsv2:28;
        }tMemAccess;

        struct tRegAccess
        {
            U32 ulAddr;
            U32 ulData;
            U32 bsMcuID:4;
            U32 bsRsv2:28;
        }tRegAccess;

        struct tDevCtrl
        {
            U32 ulRsv[2];
            U32 bsMcuID:4;
            U32 bsRsv2:28;
        }tDevCtrl;

        struct tSpiFlashAccess{
            U32 ulDramAddr;
            U32 ulSpiOffset:24;
            U32 ulDataBlockNum:8;   // 4k per data block
            U32 bsMcuID:4;
            U32 bsRsv2:28;   
        }tSpiFlashAccess;
        struct tFlashSetParam{
            U32 ulLunMsk;
            U32 ulFlashParam;
            U32 bsMcuID:4;
            U32 bsRsv2:28;   
        }tFlashSetParam;

        /* for other operation, to be continue */
        /* ... */
    };
    
}VIA_CMD_PARAM;

/* Var Table definition area */
typedef union _VAR_HEAD_L0_TABLE
{
    U8 m_content[1024];
    struct
    {
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

typedef union _VAR_HEAD_PARAM_TABLE
{
    U8 m_content[1024*4];
}VAR_HEAD_PARAM_TABLE;


typedef union _VAR_HEAD_TABLE
{
    U8 m_content[8*1024];
    struct 
    {
        VAR_HEAD_PARAM_TABLE    tParamTable;
        VAR_HEAD_L0_TABLE       tL0Table;
    };
}VAR_HEAD_TABLE;

typedef union _VAR_SUBSYS_TABLE
{
    U8 m_content[4*1024];
    struct
    {
        /*flash realated item*/
        U32 ulPuNum;
        U32 ulPageNum;
        U32 ulBlkNum;
        U32 ulRsvBlkNum;
        U32 ulPlnNum;
        U32 ulPhyPageSize;
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

typedef union _VAR_TABLE
{
    U8 m_content[32*1024];
    struct
    {
        VAR_HEAD_TABLE      tVarHeadTable;
        VAR_SUBSYS_TABLE    aSubSysTable[2];
    };
}VAR_TABLE,*PVAR_TABLE;

typedef struct _VcmParam
{
    U32 EnterVCM;
    U32 CurStage;
    U32 Signature;
    U32 CmdCode;
    union
    {
        U32  aCmdParam[3];    /* total 3DWs, only low 9 bytes can be used */

        /* for Mem read/write */
        struct MemAccess
        {
            /* DW 0 */
            U32 ulMemAddr;

            /* DW 1 */
            U32 ulByteLen;
        }MemAccess;
    };
    U32 Status;
}VCM_PARAM;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L0_ViaCmdInit(void);
VIA_CMD_STATUS L0_ViaCmdCheckParam(VIA_CMD_CODE eViaCmd, const VIA_CMD_PARAM *pCmdParam);
VIA_CMD_STATUS L0_ViaHostCmd(U8 ucSlot, VIA_CMD_CODE eViaCmd, const VIA_CMD_PARAM *pCmdParam, U32* pOut);

#define VIA_TBD 0   /* to be defined, all using this code must modify later */

/* VCM Stage */
#define CMD_STAGE         0x11
#define DATA_STAGE        0x12

/* VCM command */
#define VIA_CMD_ENTER_VCM   0xC6
#define VIA_CMD_EXIT_VCM     0xC7

#endif
/*====================End of this head file===================================*/

