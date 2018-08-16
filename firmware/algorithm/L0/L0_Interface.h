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
Filename     :  L0_Interface.
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  Charles Zhou

Description: 
Define registers and data structure for AHCI specification (v1.3).
SCMD data structure and operation routines declaration are included as well.

Modification History:
2014/06/12   Blake Zhang 001 created

*************************************************/
#ifndef _L0_INTERFACE_H
#define _L0_INTERFACE_H

#include "BaseDef.h"
#include "L0_ViaCmd.h"

/* SMSGQ/SCMDQ related defines */
#define SCQSIZE_PER_SUBSYSTEM 32
#define SMQSIZE_PER_SUBSYSTEM 4

/* Mnemonics for queue members */
#define L0SCQ_Tail(Subsys) (g_apSCmdQueue[(Subsys)]->ulTail)
#define L0SCQ_L0Head(Subsys) (g_apSCmdQueue[(Subsys)]->ulL0Head)
#define L0SCQ_L1Head(Subsys) (g_apSCmdQueue[(Subsys)]->ulL1Head)
#define L0SCQ_Node(Subsys, NodeSeq) (g_apSCmdQueue[(Subsys)]->aSubSysCmdArray[(NodeSeq)])

#define L0SMQ_Tail(Subsys) (g_apSMsgQueue[(Subsys)]->ulTail)
#define L0SMQ_Head(Subsys) (g_apSMsgQueue[(Subsys)]->ulHead)
#define L0SMQ_Node(Subsys, NodeSeq) (g_apSMsgQueue[(Subsys)]->aSubSysMsgArray[(NodeSeq)])

/* Macros used for calculating SCMD parameters in direct media accessing. */

/* x: LBA, y: subsystem number bits */
#define L0M_GET_SUBSYSID_FROM_LBA(x, y) (((x) >> SEC_PER_BUF_BITS) & ((1 << (y)) - 1))

/* x: LBA, y: subsystem number bits */
#define L0M_GET_LANE_FROM_LBA(x, y) ((x) >> (SEC_PER_BUF_BITS + (y)))

/* x: LBA */
#define L0M_GET_OFFSET_IN_LCT_FROM_LBA(x) ((x) & SEC_PER_BUF_MSK)

/* x: LBA, y: subsystem number bits */
#define L0M_GET_SUBSYSLBA_FROM_LBA(x, y) \
            ((L0M_GET_LANE_FROM_LBA(x, y) << SEC_PER_BUF_BITS) \
            + L0M_GET_OFFSET_IN_LCT_FROM_LBA(x))

/* x: LBA */
#define L0M_GET_LCT_FROM_LBA(x) ((x) >> SEC_PER_BUF_BITS)

/* x: subsystem number bits */
#define SEC_PER_LANE_BITS(x) (SEC_PER_BUF_BITS + (x))
#define SEC_PER_LANE(x) (1 << SEC_PER_LANE_BITS(x))
#define SEC_PER_LANE_MSK(x) (SEC_PER_LANE(x) - 1)

/* x: LBA, y: subsystem number bits */
#define L0M_GET_OFFSET_IN_LANE_FROM_LBA(x, y) ((x) & SEC_PER_LANE_MSK(y))

/* Define for all supported subsystem command types */
typedef enum _SUBSYSTEM_CMDTYPE
{
    SCMD_DIRECT_MEDIA_ACCESS = 0,
    SCMD_UNMAP,
    SCMD_FLUSH,
    SCMD_RAW_DATA_REQ,
    SCMD_IDLETASK,
    SCMD_SELFTESTING,
    SCMD_BOOTUP,
    SCMD_POWERCYCLING,
    SCMD_LLF,
    SCMD_ACCESS_HOSTINFO,
    SCMD_ACCESS_DEVPARAM,
    SCMD_SANITIZE,
    SCMD_VIA_DEV_CTRL,
    SCMD_VIA_UART_RAW_DATA_REQ
} SCMD_TYPE;

/* Define for SCMD specific parameter  */
typedef enum _SCMD_SPEC_PARAM
{
    SCMD_SPEC_NONE = 0,
    SCMD_SPEC_TL_INVALIDATE
} SCMD_SPEC_PARAM;

/* Operation type define for direct media access subsystem command */
typedef enum _DM_OPTYPE
{
    DM_READ = 0,
    DM_WRITE
} DM_OPTYPE;

/* Integrity information define for direct media access subsystem command */
typedef enum _DM_OPTION
{
    DM_NONE = 0,
    DM_FUA,
    DM_VERIFY,
    DM_WRITE_ZERO
} DM_OPTION;

/* Operation type define for raw data access subsystem command */
typedef enum _RAWDATA_REQDIR
{
    RAWDRQ_D2H = 0,   /* read */
    RAWDRQ_H2D
} RAWDRQ_DIR;

/* Priority information define for idle task subsystem command */
typedef enum _IDLEPRIO
{
    IDLE_NORMAL = 0,
    IDLE_CRITICAL,
    IDLE_ERRORRECOVERY
} IDLEPRIO;

/* Starting type for Boot operation */
typedef enum _START_TYPE
{
    COLD_START = 0,
    WARM_START
} START_TYPE;

/* Operation type for the global information access subsystem command */
typedef enum _GLBINFO_OPTYPE
{
    GLBINFO_LOAD = 0,
    GLBINFO_SAVE
} GINFO_OP;

/* Define of the life-cycle statuses for a subsystem command */
typedef enum _SCMD_STATUS
{
    SSTS_NOT_ALLOCATED = 0,
    SSTS_PENDING,
    SSTS_SUCCESS,
    SSTS_FAILED,
    SSTS_ABORTED
} SCMD_STATUS;

/* Data structure define for a subsystem command */
typedef struct _SUBSYSTEM_CMD
{
    /* DWord 0: Public part for all types of subsystem commands. */
    U8 ucSCmdType;
    U8 ucSCmdStatus;
    U8 ucSCmdSpecific;
    U8 ucSlotNum;

    /* DWord 1 - 7: Parameters for different type of subsystem commands */
    union
    {
        /* For direct media access */
        struct tMA
        {
            U32 ulSubSysLBA;

            U8 ucSecLen;
            U8 ucFirst;
            U8 ucLast;
            U8 ucIsSeq;

            U8 ucIsNCQ;
            U8 ucIsPIO; /* For SATA interface only */
            U8 ucOpType;
            U8 ucOption;

            /* Protocol related */
            union
            {
                /* AHCI and NVMe */
                struct
                {
                    U32 ulHPRDMemOffset; 

                    U32 usHPRDEntryID:16;
                    U32 ucNeedCplForPeerSubsys:8;
                    U32 ucRsvd:8;
                    U32 ulHCmdSecCnt;
                };

                /* SATA */
                struct
                {
                    U32 ulHostStartLBA;
                    U32 ulHCmdSecLen;
                    U32 usSCmdIndex:16;
                    U32 usHCmdIndex:16;
                };

                U32 aDMProtoInDW[3];
            };
        }tMA; /* Media Access*/

        /* For unmap operation */
        struct tUnmap
        {
            U32 ulSubSysLBA;
            U32 ulSecLen;
        }tUnmap;

        /* For raw data transfer request: required by some
               indirect media access and non media access host commands. */
        struct tRawData
        {
            U32 ulBuffAddr;

            U8  ucSecLen;
            U8  ucDataDir;
            U8  ucSATAUsePIO; /* For SATA interface only */
            U8  ucRsvd;

            VIA_CMD_PARAM tViaParam;
        }tRawData;

        /* For idle task control operation */
        struct tIdle
        {
            U32 ulPriority;
        }tIdle;

        /* For sanitize operation */
        struct tSanitize
        {
            U32 ulMode;
        }tSanitize;

        /* For saving or loading host information page and device parameter page */
        struct tSystemInfos
        {
            U32 ulBuffAddr;
            U32 ulByteLen;
            U32 ulOpType;
        }tSystemInfos;

        /* for LLF, Boot, Shutdown operations */
        struct tSystemCtrl
        {
            U32 ulParameter;
            U32 ulHInfoBuffAddr;
            U32 ulHInfoByteLen;
        }tSystemCtrl;

        /* For VIA Device control command */
        struct tViaDevCtrl
        {
            U8 ucViaCmdCode;
            U8 aRsvd[2];
            U8 ucCriticalFlag;//Flash operation flag
        
            VIA_CMD_PARAM tViaParam;
            U32 aOutputValue[2];
            U32 ulCmdDefine;
        }tViaDevCtrl;

        /* Providing original DWORD data for debugging purpose */
        U32 aSCmdParamInDW[7];
    };
} SCMD, *PSCMD;

#define SCMD_SIZE_DW (sizeof(SCMD)/DWORD_SIZE)

/* The key data structure for the subsystem command queue. */
typedef struct _SUBSYS_CMD_QUEUE
{
    /* The queue is based on an array located in L0/L1 shared memory space. */
    SCMD aSubSysCmdArray[SCQSIZE_PER_SUBSYSTEM];

    /* The three key pointers on our ring FIFO. */
    /* L0 modifies tail pointer and its own head pointer(submitting and recycling SCMD nodes),
          and monitors L1 head pointer. 
          L1 modifies its own head pointer and monitors tail pointer(acquiring and completing SCMD nodes). */
    volatile U32 ulTail;
    volatile U32 ulL0Head;
    volatile U32 ulL1Head;
} SCQ, *PSCQ;

/* The data structure for inter-processor message exchange initiated by subsystems. */
#define SMSG_SUBSYS_ERROR 0
#define SMSG_UNEXP_POWERLOSS 1
#define SMSG_FORCE_CLEAR_SCQ 2
#define SMSG_SCQ_CLEARED 3
#define SMSG_WAKEUP_SUBSYS 4
#define SMSG_INFORM_DBG_GETCH 5
#define SMSG_DBG_GETCH_CLEAR_SCQ 6 
#define SMSG_FINISH_DBG_GETCH 7

#define SUBSYS_ERROR_UECC 0
#define SUBSYS_ERROR_WP 1

typedef union _SUBSYS_MESSAGE
{
    struct
    {
        U32 ulMsgSeqNum;
        U32 ulMsgCode;
        U32 ulMsgParam[2];
    };

    U32 ulMessageData[4];
} SMSG, *PSMSG;

typedef struct _SUBSYS_MESSAGE_QUEUE
{
    /* The queue is based on an array located in L0/L1 shared memory space. */
    SMSG aSubSysMsgArray[SMQSIZE_PER_SUBSYSTEM];

    /* The pointers on the simple FIFO. */
    /* L0 modifies head pointer (popping SMSG nodes from FIFO),
          and monitors tail pointer for checking whether FIFO is empty. 
          L3 modifies tail pointer (pushing SMSG nodes into FIFO) and
          monitors head pointer to check whether FIFO is full. */
    volatile U32 ulHead;
    volatile U32 ulTail;
} SMQ, *PSMQ;


/*
UeccError Marker(Shared Memory with MCU1/2)
Since UECC error should not be handled immediately when a UECC error is Push from L3.
L0 should handle this error in a queue handling schedule.
When this UeccError is handled by L0, a Completion Message should be sent to L3 from L0.
And L3 should make a response for L0. Therefor, UeccError Marker is introduced*/
typedef struct _SMSG_UECC_RSP_MARKER
{
    /*
    Indicate MarkerType&MarkerStatus.This value should be 
    set as SMSG_UECC_CLEAR by L0 when L0 has processed UECC SMS
    This value should be set as 0xFFFFFFFF indicating when 
    L3 has handled this Marker by L3 and is invalid for L3.
    L0 can send next UECC Marker when this value is 0xFFFFFFFF*/
    U32 ulMarkerType;    

    U32 ulPUID;          //We can use BitMap for expansion
}SMSG_UECC_RSP_MARKER, *PSMSG_UECC_RSP_MARKER;
#define SMS_UECC_MARKER_VALID     0xFFFFFFFF
#define SMS_UECC_CLEAR              0x08

//Multi Core Shared Data between MCU0 and MCU1 & MCU0 and MCU2
typedef struct _MULTI_CORE_SHARE_DATA
{
    struct {
        SMSG_UECC_RSP_MARKER SmsgUeccRspMarker;
    }tRspMarker;
}MCSD, *PMCSD;


/* Signature in global information data */
#define GLOBAL_INFO_CHECK_1   (0x5AA5A55A)
#define GLOBAL_INFO_CHECK_2   (0xC33C3CC3)

/* Data structure define for host information page maintained by L0. */
typedef struct _HOST_INFO_PAGE
{
    /* Check DWORD1 -- 1DW */
    U32 Check1;

    union
    {
        /* host information page */
        struct
        {
            /* 
                        IDENTIFY DEVICE data -- 128 DWs
                        note: Identify data must be aligned to 4DW border because DMAE would access it
                        So MCU0 should allocate System GlobalInfo data structure in at least 4DW aligned address
                    */
            U16 usIdentifyData[256];

            /* Statistics infos -- 9 DWs */
            U32 PowerOnSecs;
            U32 PowerOnMins;
            U32 PowerOnHours;
            U32 TotalLBAWrittenHigh;
            U32 TotalLBAWrittenLow;
            U32 TotalLBAReadHigh;
            U32 TotalLBAReadLow;
            U32 SataSmartEnable;
            U32 SataSmartAutoSave;

            /* Security Infos -- 17 DWs */
            U16 SataSecurityStatus;
            U16 SataSecurityUserPswd[16];
            U16 SataSecurityMasterPswd[16];
            U16 SataSecurityMasterPswdIdentifier;

            /* HPA Infos -- 1DW */
            U32 HPAMaxLBA;

            /* Power management Infos -- 5 DWs */
            BOOL g_bSataFlagDipmEnabled;
            BOOL g_bSataFlagAPTS_Enabled;
            BOOL g_bMultipleDataOpen;
            BOOL gbSATALinkSleeping;
            U8 gucSATALPState;
            U8 gucSystemPMState;
            U8 ucReserved[2];

            /* SERROR count Infos -- 1 DWs*/
            U32 SErrorCnt;

            /* Resered -- 1 DWs*/
            U32 ulReserved;

            U32 NvmeErrorCntDW0;
            U32 NvmeErrorCntDW1;
            U32 NvmeHostReadCntDW0;
            U32 NvmeHostReadCntDW1;
            U32 NvmeHostReadCntDW2;
            U32 NvmeHostReadCntDW3;
            U32 NvmeHostWriteCntDW0;
            U32 NvmeHostWriteCntDW1;
            U32 NvmeHostWriteCntDW2;
            U32 NvmeHostWriteCntDW3;
        };

        /* Total 128 + 9 + 17 + 1 + 5 + 1 + 1 = 162 DWs */
        U32 aMCU0Infos[162];
    };

    /* Check DWORD2 */
    U32 Check2;
} HOST_INFO_PAGE, *PHOST_INFO_PAGE;

#define SUBSYSTEM_INFO_LEN  ( 20 + 2 )

/* Data structure define for device parameter page maintained by SubSystem. */
typedef struct _DEVICE_PARAM_PAGE
{
    /* Check DWORD1 -- 1DW */
    U32 Check1;

    /* device parameter page */
    union
    {
        struct
        {
            /* Statistics infos -- 10 DWs */
            U32 PowerCycleCnt;
            U32 WearLevelingCnt;
            U32 UsedRsvdBlockCnt;
            U32 ProgramFailCnt;
            U32 EraseFailCnt;
            U32 SafeShutdownCnt;
            U32 AvailRsvdSpace;
            U32 TotalNANDWrites;
            U32 TotalEraseCount;
            U32 AvgEraseCount;

            /* SYSTEM Error Infos -- 10 DWs */
            U32 SYSUECCCnt;
            U32 SYSErrNest;
            U32 SYSTemperature;
            U32 WorstTemperature;
            U32 SYSUnSafeShutdownCnt;
            U32 SYSRsvd[5];
        };

        /* Total 10 + 10 + SUBSYSTEM_PU_MAX = SUBSYSTEM_INFO_LEN DWs */
        U32 aSubSystemInfos[SUBSYSTEM_INFO_LEN];
    };

    /* Check DWORD2 */
    U32 Check2;
} DEVICE_PARAM_PAGE, *PDEVICE_PARAM_PAGE;

extern PMCSD g_apMcShareData[];


/* Interface APIs for operating the SCQ by L0 */
void L0_InitSCQ(U32 ulSubSysId);
U32 L0_IsSCQFull(U32 ulSubSysId);
U32 L0_IsSCQEmpty(U32 ulSubSysId);
U32 L0_CheckSCQAllEmpty(void);
PSCMD L0_GetNewSCmdNode(U32 ulSubSysId);
void L0_PushSCmdNode(U32 ulSubSysId);
PSCMD L0_GetSCmdNodeToRecycle(U32 ulSubSysId);
PSCMD L0_RecyleSCmd(void);
void L0_WaitForAllSCmdCpl(U32 ulSubSysId);
U32 L0_ProcessSCMDCompletion(U32 ulSubSysId, PSCMD pSCNode);
U32 L0_IssueAccessHostInfoSCmd(U32 ulSubSysId, U32 ulHostInfoAddr, U32 ulAccType);
U32 L0_IssueAccessDevParamSCmd(U32 ulSubSysId, U32 ulDevParamAddr, U32 ulAccType);
U32 L0_IssueLLFSCmd(U32 ulSubSysId, U32 ulHostInfoAddr, U32 ulResetDevParam);
U32 L0_IssueBootSCmd(U32 ulSubSysId, U32 ulHostInfoAddr);
U32 L0_IssuePwrCylSCmd(U32 ulHostInfoAddr);
U32 L0_IssueIdleSCmd(U32 ulSubSysId, U32 ulIdlePrio);
U32 L0_IssueSelfTestingSCmd(U32 ulSubSysId);
U32 L0_IssueFlushSCmd(U32 ulSubSysId);
U32 L0_IssueUnmapSCmd(U32 ulSubSysId, U32 ulUnmapStart, U32 ulRangeLen, U32* ulCurrTail);
U32 L0_IssueSecurityEraseSCmd(U32 ulSubSysId, U32 ulHostInfoAddr, U32 ulResetDevParam);
void L0_InitSMQ(U32 ulSubSysId);
U32 L0_IsSMQEmpty(U32 ulSubSysId);
PSMSG L0_GetSMSGFromHead(U32 ulSubSysId);
void L0_PopSMSGNode(U32 ulSubSysId);
void L0_PostNtfnMsg(U32 ulTargetSubsysMap, U32 ulMsgCode);
void L0_ForceAllSubSysIdle(void);
void L0_SubSystemOnlineShutdown(U32);
void L0_SubSystemOnlineReboot(void);
void L0_FwUpdateInit(void);

#endif // _L0_INTERFACE_H

