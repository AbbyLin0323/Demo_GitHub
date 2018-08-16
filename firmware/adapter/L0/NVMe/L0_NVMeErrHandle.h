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
* File Name    : L0_NVMeErrHandle.h
* Discription  : 
* CreateAuthor : Haven Yang
* CreateDate   : 2015.1.28
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _L0_NVMEERRORHANDLE_H
#define _L0_NVMEERRORHANDLE_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

/*============================================================================*/
/* Completion Queue status field definition  start. haven add 20150128        */
/*============================================================================*/
//interal classification
#define SCT_GENERIC     (0)
#define SCT_SPECIFIC    (1)
#define SCT_MEDIA       (2)
#define SCT_VENDOR      (7)

// Combination of Status Field and Parameter Error Location
#define NVME_ERRLOG_COMPRESS(ByteLoc, BitLoc, StatusField) \
    (((((BitLoc) << 8) | (ByteLoc)) << 16) | (StatusField))
#define NVME_ERRLOG_EXTRACT_LOCATION(CompactCode) \
    ((CompactCode) >> 16)
#define NVME_ERRLOG_EXTRACT_STATUS(CompactCode) \
    ((CompactCode) & MSK_4F)

//Status Field (Generi status code)
#define NVME_SF_SUCCESS             ((SCT_GENERIC << 8) + 0x00)
#define NVME_SF_IVLD_OPCODE         ((SCT_GENERIC << 8) + 0x01)
#define NVME_SF_IVLD_FIELD          ((SCT_GENERIC << 8) + 0x02)
#define NVME_SF_CMDID_CONFLICT      ((SCT_GENERIC << 8) + 0x03)
#define NVME_SF_DATAXFER_ERROR      ((SCT_GENERIC << 8) + 0x04)
#define NVME_SF_ABORT_BY_PWRLOS     ((SCT_GENERIC << 8) + 0x05)
#define NVME_SF_INTERNAL_ERROR      ((SCT_GENERIC << 8) + 0x06)
#define NVME_SF_ABORT_REQUESTED     ((SCT_GENERIC << 8) + 0x07)
#define NVME_SF_ABORT_BY_SQ_DEL     ((SCT_GENERIC << 8) + 0x08)
#define NVME_SF_ABORT_BY_FFC        ((SCT_GENERIC << 8) + 0x09) /* failed fused command */
#define NVME_SF_ABORT_BY_MFC        ((SCT_GENERIC << 8) + 0x0A) /* missing fused command */
#define NVME_SF_IVLD_NS_FMT         ((SCT_GENERIC << 8) + 0x0B)
#define NVME_SF_SEQ_ERROR           ((SCT_GENERIC << 8) + 0x0C)
#define NVME_SF_IVLD_SGL_SEG        ((SCT_GENERIC << 8) + 0x0D)
#define NVME_SF_IVLD_SGL_NUM        ((SCT_GENERIC << 8) + 0x0E)
#define NVME_SF_IVLD_DSGL_LEN       ((SCT_GENERIC << 8) + 0x0F)
#define NVME_SF_IVLD_MSGL_LEN       ((SCT_GENERIC << 8) + 0x10)
#define NVME_SF_IVLD_SGL_TYPE       ((SCT_GENERIC << 8) + 0x11)
#define NVME_SF_IVLD_UCMB           ((SCT_GENERIC << 8) + 0x12) /* use of controller memory buffer */
#define NVME_SF_IVLD_PRP_OFS        ((SCT_GENERIC << 8) + 0x13)
#define NVME_SF_IVLD_AWUE           ((SCT_GENERIC << 8) + 0x14) /* Atomic Write Unit Exceeded */

//Status Field (Generi status code, NVM command set)
#define NVME_SF_LBA_OUTOFRANGE      ((SCT_GENERIC << 8) + 0x80)
#define NVME_SF_CAP_EXCEEDED        ((SCT_GENERIC << 8) + 0x81)
#define NVME_SF_NS_NOTREADY         ((SCT_GENERIC << 8) + 0x82)
#define NVME_SF_RSVT_CONFLICT       ((SCT_GENERIC << 8) + 0x83)
#define NVME_SF_FORMAT_IN_PROGRESS  ((SCT_GENERIC << 8) + 0x84)

//Status Field (Command Specific status code)
#define NVME_SF_IVLD_CQ             ((SCT_SPECIFIC << 8) + 0x00)
#define NVME_SF_IVLD_QID            ((SCT_SPECIFIC << 8) + 0x01)
#define NVME_SF_IVLD_QSIZE          ((SCT_SPECIFIC << 8) + 0x02)
#define NVME_SF_ABORT_CMD_LE        ((SCT_SPECIFIC << 8) + 0x03) /* abort command limit exceeded */
#define NVME_SF_RSVD                ((SCT_SPECIFIC << 8) + 0x04)
#define NVME_SF_AE_REQ_LE           ((SCT_SPECIFIC << 8) + 0x05)
#define NVME_SF_IVLD_FW_SLOT        ((SCT_SPECIFIC << 8) + 0x06)
#define NVME_SF_IVLD_FW_IMAGE       ((SCT_SPECIFIC << 8) + 0x07)
#define NVME_SF_IVLD_INT_VECTOR     ((SCT_SPECIFIC << 8) + 0x08)
#define NVME_SF_IVLD_LOG_PAGE       ((SCT_SPECIFIC << 8) + 0x09)
#define NVME_SF_IVLD_FORMAT         ((SCT_SPECIFIC << 8) + 0x0A)
#define NVME_SF_FW_ARCR             ((SCT_SPECIFIC << 8) + 0x0B) /* Activation Requires Conventional Reset */
#define NVME_SF_IVLD_Q_DELETION     ((SCT_SPECIFIC << 8) + 0x0C) /* delete cq */
#define NVME_SF_FINS                ((SCT_SPECIFIC << 8) + 0x0D) /* feature identifier not saveable */
#define NVME_SF_FNC                 ((SCT_SPECIFIC << 8) + 0x0E) /* feature Not Changeable */
#define NVME_SF_FNNS                ((SCT_SPECIFIC << 8) + 0x0F) /* feature Not Namespace Specific */
#define NVME_SF_FW_ACT_R_NVE_RST    ((SCT_SPECIFIC << 8) + 0x10) /* firmware commit: firmware activation requires NVM subsystem reset */
#define NVME_SF_FW_ACT_R_RST        ((SCT_SPECIFIC << 8) + 0x11) /* firmware commit: firmware activation requires Reset */
#define NVME_SF_FW_ACT_R_MAX_TIME   ((SCT_SPECIFIC << 8) + 0x12) /* firmware commit: firmware activation requires Max Time Violation */
#define NVME_SF_FW_ACT_PROHIBITED   ((SCT_SPECIFIC << 8) + 0x13) /* firmware commit: firmware activation prohibited */
#define NVME_SF_OVERLAPPING         ((SCT_SPECIFIC << 8) + 0x14) /* frimware commit, firmware download, set features */



//Status Field (Command Specific status code, NVM command set)
#define NVME_SF_ATTR_CONFLICT       ((SCT_SPECIFIC << 8) + 0x80)
#define NVME_SF_IVLD_PROTECT_INFO   ((SCT_SPECIFIC << 8) + 0x81)
#define NVME_SF_ATTEMPT_WTROR       ((SCT_SPECIFIC << 8) + 0x82) /* write to read only range */


//Status Field (Media and Data integrity Error Values, NVM command set)
#define NVME_SF_WRITE_FAULT         ((SCT_MEDIA << 8) + 0x80)
#define NVME_SF_UECC                ((SCT_MEDIA << 8) + 0x81)
#define NVME_SF_ETE_GUARD_ERR       ((SCT_MEDIA << 8) + 0x82)
#define NVME_SF_ETE_APP_TAG_ERR     ((SCT_MEDIA << 8) + 0x83)
#define NVME_SF_ETE_REFER_TAG_ERR   ((SCT_MEDIA << 8) + 0x84)
#define NVME_SF_COMPARE_FAILURE     ((SCT_MEDIA << 8) + 0x85)
#define NVME_SF_ACCESS_DENIED       ((SCT_MEDIA << 8) + 0x86)
#define NVME_SF_READ_WO_WRITE       ((SCT_MEDIA << 8) + 0x87)

//Status Field (Vendor define)
#define NVME_SF_VENDOR(status)      ((SCT_VENDOR << 8) + 0xC0 + (status))   /* status <= 0x3f */

/*============================================================================*/
/* Completion Queue status field definition  end                              */
/*============================================================================*/

/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

enum _ERRHNDL_SUBSYS_STAGE
{
    ERRHNDL_FORCECLEARSCQ = 0,
    ERRHNDL_WAITSCQCLEAR,
    ERRHNDL_SELFTEST,
    ERRHNDL_FORCEIDLE,
    ERRHNDL_WAITIDLE
};

typedef struct _SUBSYS_ERRHNDL_CTRL
{
    U32 ulSubsysMap;
    U32 ulProcStage;
} SUBSYS_ERRHNDLCTRL, *PSUBSYS_ERRHNDLCTRL;

enum _PORT_ERROR_STATE
{
    ERRORSTATE_NEW = 0,
    ERRORSTATE_PROCSUBSYS,
    ERRORSTATE_LOCALRECOVERY,
    ERRORSTATE_NOTIFYHOST,
    ERRORSTATE_COMPLETED
};

enum _RESET_TYPE
{
    RSTTP_NVM_SUBSYS_RESET = 0,
    RSTTP_PCIE_RESET,
    RSTTP_CONTROLLER_RESET
};

typedef struct _nvme_reset
{
    union
    {
        struct
        {
            U32 bsNVMSubSystemReset:1;
            U32 bsPCIeReset:1;
            U32 bsControllerReset:1;
            U32 bsReserved:29;
        };

        U32 ulResetMap;
    };
}NVME_RESET;

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void L0_NVMeUecc(U32 ulSlot);
void L0_NVMeClearErrorLog(void);
U32 L0_NVMeGetLastErrorLogIndex(void);
void L0_NVMeCmdError(U32 ulSlot, U32 ulStatusField);
void L0_NVMeNonCmdError(U32 ulErrorType);
BOOL L0_NVMeProcResetEvent(void *p);
BOOL L0_NVMeLinkFailure(void *pEventParam);
BOOL L0_NVMeLinkFatalError(void *pEventParam);
void L0_NVMeNVMReset(void);
void L0_NVMePCIeReset(void);
void L0_NVMeControllerReset(void);
U32 L0_NVMeGetResetMap(void);
void L0_NVMeErrorHandleInit(void);
void L0_WaitHostReset(void);
#endif
/*====================End of this head file===================================*/

