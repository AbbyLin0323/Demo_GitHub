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
*******************************************************************************/
#ifndef _L0_NVME_H_
#define _L0_NVME_H_

#include "BaseDef.h"
#include "Disk_Config.h"
#include "HAL_Inc.h"

#ifdef SIM
#define SIM_Getch()   DBG_Getch()
#else
#define SIM_Getch()
#endif

//log identifer
#define LID_RESERVED               0x00
#define LID_ERROR                  0x01
#define LID_SMART                  0x02
#define LID_FW_SLOT_INFO           0x03
#define LID_NAMESPACE_CHANGE       0x04
#define LID_TELEMETRY_CONTROLLER   0x08
#define LID_RESVTN_NOTIFICATION    0x80
#define LID_SANITIZE_STATUS        0x81

#define LOG_ERROR_LEN               4096
#define LOG_SMART_LEN               512
#define LOG_FW_SLOT_INFO_LEN        512
#define LOG_RSV_NOTIFY_LEN          64

#define LOG_ERROR_MAX_ENTRY_NUM     64

//feature identifier
/*
#define   FID_ARBIT                   0x01
#define   FID_PM                      0x02
#define   FID_LBA_RANGE_TYPE          0x03
#define   FID_TEMP_THRES              0x04
#define   FID_ERR_RECOVERY            0x05
#define   FID_VOL_WR_CACHE            0x06
#define   FID_NUM_QUEUE               0x07
#define   FID_INT_COAL                0x08
#define   FID_INT_VEC_CONF            0x09
#define   FID_WR_ATOMIC               0x0A
#define   FID_ASYN_EVT_CONF           0x0B
#define   FID_AUTON_PST               0x0C
#define   FID_SW_PROG_MARKER          0x80
#define   FID_HOST_IDENTIFER          0x81
*/

#define   FEA_ARBIT_LEN               4
#define   FEA_PM_LEN                  4
#define   FEA_LBA_RANGE_TYPE_LEN      64
#define   FEA_TEMP_THRES_LEN          4
#define   FEA_ERR_RECOVERY_LEN        4
#define   FEA_VOL_WR_CACHE_LEN        4
#define   FEA_NUM_QUEUE_LEN           4
#define   FEA_INT_COAL_LEN            4
#define   FEA_INT_VEC_CONF_LEN        4
#define   FEA_WR_ATOMIC_LEN           4
#define   FEA_ASYN_EVT_CONF_LEN       4
#define   FEA_AUTON_PST_LEN           12
#define   FEA_SW_PROG_MARKER          4
#define   FEA_HOST_IDENTIFIER_LEN     4


//|---MCU0---|---IDENTIFY---|---LOG PAGE---|--FEATURES--|

#define  MCU0_CONTROLLER_IDENTIFY_OFFSET    0
#define  MCU0_NAMESPACE_IDENTIFY_OFFSET     (MCU0_CONTROLLER_IDENTIFY_OFFSET+4096)
#define  MCU0_NAMESPACE_LIST_OFFSET         (MCU0_NAMESPACE_IDENTIFY_OFFSET+4096)
#define  MCU0_GENERAL_BUFFER                (MCU0_NAMESPACE_LIST_OFFSET+4096)
#define  MCU0_LOG_ERROR_OFFSET              (MCU0_GENERAL_BUFFER+4096)
#define  MCU0_LOG_SMART_OFFSET              (MCU0_LOG_ERROR_OFFSET+LOG_ERROR_LEN)
#define  MCU0_LOG_FW_SLOT_INFO_OFFSET       (MCU0_LOG_SMART_OFFSET+LOG_SMART_LEN)
#define  MCU0_LOG_RSV_NOTIFY_OFFSET         (MCU0_LOG_FW_SLOT_INFO_OFFSET+LOG_FW_SLOT_INFO_LEN)  
 
// keep record of CmdState just for debug.
#define L0_DBG_STATE

/***************************************************************************\
 *
 * Register allocation
 *
 *
 \***************************************************************************/


//Admin Command Set
#define    ACS_DELETE_SQ                        0x00
#define    ACS_CREATE_SQ                        0x01
#define    ACS_GET_LOG_PAGE                     0x02
#define    ACS_DELETE_CQ                        0x04
#define    ACS_CREATE_CQ                        0x05
#define    ACS_IDENTIFY                         0x06
#define    ACS_ABORT                            0x08
#define    ACS_SET_FEATURES                     0x09
#define    ACS_GET_FEATURES                     0x0A
#define    ACS_ASYN_EVENT_REQ                   0x0C
#define    ACS_FW_COMMIT                        0x10
#define    ACS_IMG_DOWNLOAD                     0x11
#define    ACS_FORMAT_NVM                       0x80
#define    ACS_VIA_VENDOR_CMD                   0xff

//NVM Command Set
#define    NCS_FLUSH                            0x00
#define    NCS_WRITE                            0x01
#define    NCS_READ                             0x02
#define    NCS_WRITE_ZERO                       0x08
#define    NCS_DATASET_MANAGEMENT               0x09

//PRP check result
#define PRPLISTSTS_READY 0
#define PRPLISTSTS_NEEDFETCH 1
#define PRPLISTSTS_INVALIDADDR 2

typedef enum _slot_state
{
    SLOT_IDLE = 0,
    SLOT_ASYNC_EVT_RESV,
    SLOT_WAITING_SQENTRY,
    SLOT_AFSQE_RDY,
    SLOT_SQENTRY_RDY,
    SLOT_WAITING_PRPLIST,
    SLOT_PRPLIST_RDY,
    SLOT_TRIGGER_WBQ,
} SLOT_STATE;

typedef enum _CMD_STATE
{
    CMD_IDLE = 0,
    CMD_NEW,
    CMD_FETCHPRPLIST,
    CMD_CHECKPRPLIST,
    CMD_HANDLING,
    CMD_WAITING,
    CMD_DSM_PROCDATA,
    CMD_DSM_DISPATCH,
    CMD_DSM_WAITING,
    CMD_COMPLETED
} CMD_STATE;

typedef struct _cb_mgr *PCB_MGR;

typedef enum _HCMD_TYPE
{
    HCMD_TYPE_SOFT_RESET = 0,
    HCMD_TYPE_DIRECT_MEDIA_ACCESS,
    HCMD_TYPE_OTHERS
} HCMD_TYPE;


#define INVALID_CMD_ID 0xFF

// 'MDMC'
#define CB_MGR_SIG      0x4D444D43

typedef struct _command_header{
        U8     OPC;
        U8     FUSE:2;
        U8     RSV:5;
        U8     PSDT:1;
        U16    CID;
        U32    NSID;

        U32    RSV1[2];
        U32    MPTRL;
        U32    MPTRH;
        U32    PRP1L;
        U32    PRP1H;
        U32    PRP2L;
        U32    PRP2H;
    
        U32    DW10;
        U32    DW11;
        U32    DW12;
        U32    DW13;
        U32    DW14;
        U32    DW15;
}COMMAND_HEADER, *PCOMMAND_HEADER;




typedef struct _command_table
{
    PRP     PRPTable[MAX_PRP_NUM];
} COMMAND_TABLE, *PCOMMAND_TABLE;


#define MAX_UECC_PUBMAP_DWSIZE    ((0==SUBSYSTEM_LUN_MAX%32) ? (SUBSYSTEM_LUN_MAX/32): (SUBSYSTEM_LUN_MAX/32+1))

typedef struct _cb_mgr
{
    U32             CmdState; /* CMD_STATE */

    U8              IsWriteDir; // direction in terms of PCIe. 0: read from system memory, 1: write to system memory.
    U8              SlotNum; // 0~63
    U16             CurrentSubCmdIndex;
    U32             TotalRemainingBytes;        

    PCOMMAND_HEADER Ch;

    U32             UeccPUBitMap[MAX_UECC_PUBMAP_DWSIZE]; 

    PPRP            PRPTable;
    U32             TotalPRPCnt;
    U32             TotalSecCnt;
    U32             CurrentLBAL;
    U32             CurrentLBAH;

    U32             RemSecCnt;
    U32             PRPIndex;
    U32             PRPOffset;

    U32             SQID:8;
    U32             CQID:8;
    U32             IsNeedPRPList:1;
    U32             bAbort:1;
    U32             SQEntryIndex:14;   //Max ASQEntryIndex is 4096.

    U32             CSPC;
    U16             CmdSts;
    U8              HasDataXfer;
    U8              DataXferState;
    U32             DataAddr;
} CB_MGR;

#define CB_NUM_MAX        32
// for trace
#define TRACE_CMD_DEPTH   4

typedef struct _trace_cmd_info
{
    U32 TotalID;
    U8  SlotNum;
    U8  Command;
    U16 Prdtl;
    U32 LBALo:24;
    U32 FeatureLo:8;
    U32 PxCI;
    U32 PxSACT;
} TRACE_CMD_INFO, *PTRACE_CMD_INFO;

typedef struct _q_info
{
    U32  AdrL;
    U32  AdrH;

    U32   Valid:1;
    U32   PC:1;
    U32   RSV0:30;

    U16  QID;
    U16  QSIZE;
    U16  SqHead;
    U16  CqTail;

    union
    {
        struct
        {
            U16   QPRIO:2;
            U16   RSV1:14;
            U8    RSV2;
            U8    BindCQID;
        };
        struct
        {
            U16  IEN:1;
            U16  RSV3:15;
            U16  IV;
        };
    };

}Q_INFO;

#define MAX_Q_NUM       9
#define MAX_SQ_NUM      MAX_Q_NUM
#define MAX_CQ_NUM      MAX_Q_NUM
#define MAX_IOSQ_DEPTH  1024

#if 0
typedef struct _ASYN_EVENT_MGR_{
    union
    {
        U32 AsU32;
        struct {
            U32 SpareSpace:1;
            U32 Temperature:1;
            U32 NvmRelia:1;
            U32 ReadOnly:1;
            U32 BackupDev:1;
            U32 Rsv0:3;
            U32 NAN:1;//namespace active notice
            U32 FAN:1;//firmware active notice
            U32 RSV1:6;

            U32 SpareSpaceSend:1;
            U32 TemperatureSend:1;
            U32 NvmReliaSend:1;
            U32 ReadOnlySend:1;
            U32 BackupDevSend:1;
            U32 NANSend:1;
            U32 FANSend:1;
            U32 ErrorStsSend:1;
            U32 RSV2:8;
        };
    };

    U16         Rptr;
    U16         Wptr;
    NVME_WBQ    Queue[MAX_ASYN_EVEVNT_NUM];
    U8          OutStandingCnt;
}ASYN_EVENT_MGR;
#else
#define MAX_ASYN_EVENT_NUM   4
//#define ASYNC_EVENT_BUFFER_DEPTH 4

#define ASNYC_EVENT_TYPENUM 5
#define ASYNC_EVENT_TYPE_ERROR 0
#define ASYNC_EVENT_TYPE_SMART 1
#define ASYNC_EVENT_TYPE_NOTICE 2
#define ASYNC_EVENT_TYPE_IOCMD_SPECIFIC 6
#define ASYNC_EVENT_TYPE_VENDOR_SPECIFIC 7

#define ASYNC_EVENT_ERRORINFO_INVLDDBL_WRT 0
#define ASYNC_EVENT_ERRORINFO_DBLVAL_INVLD 1
#define ASYNC_EVENT_ERRORINFO_DIAG_FAILURE 2
#define ASYNC_EVENT_ERRORINFO_PST_INTERR 3
#define ASYNC_EVENT_ERRORINFO_TRS_INTERR 4
#define ASYNC_EVENT_ERRORINFO_FW_LOAD_ERR 5

#define ASYNC_EVENT_SMARTINFO_NVM_RLBLTY 0
#define ASYNC_EVENT_SMARTINFO_TEMP_THRSHD 1
#define ASYNC_EVENT_SMARTINFO_SPARE_THRSHD 2

#define ASYNC_EVENT_NOTICEINFO_NMSP_ATTR 0
#define ASYNC_EVENT_NOTICEINFO_FW_ACTVTN 1
#define ASYNC_EVENT_NOTICEINFO_TLMTRY_LOG 2

#define ASYNC_EVENT_IOCMDINFO_RSVTN_LOG 0
#define ASYNC_EVENT_IOCMDINFO_SANTZ_CMPL 1

#define NVME_LIFEPERCENTUSED_THRESHOLD 90

typedef union _ASYNC_EVENT_RESPONSE
{
    struct
    {
        U32 bsAsyncEventType: 3;
        U32 bsResv1: 5;
        U32 bsAsyncEventInfo: 8;
        U32 bsLogPageId: 8;
        U32 bsResv2: 8;
    };

    U32 ulCQDW0;
} ASYNC_EVENT_RESPONSE, *PASYNC_EVENT_RESPONSE;

typedef union _ASYNC_EVENT_CONFIGURATION
{
    struct
    {
        U32 bsSMARTSpareSpcWrng :1;
        U32 bsSMARTTmptWrng :1;
        U32 bsSMARTRlblWrng :1;
        U32 bsSMARTRdOnlyWrng :1;
        U32 bsSMARTVtlMemFailWrng :1;
        U32 bsResv1 :3;
        U32 bsNmspAttrNtcs :1;
        U32 bsFwActvnNtcs :1;
        U32 bsTlmLogNtcs :1;
        U32 bsResv2 :21;
    };

    U32 ulCmdDW11;
} ASYNC_EVENT_CONFIGURATION, *PASYNC_EVENT_CONFIGURATION;

typedef struct _ASYNC_EVENT_CONTROL
{
    ASYNC_EVENT_CONFIGURATION tHostConfig;
    ASYNC_EVENT_RESPONSE tPendingEvent;
    U32 ulEventTypeMask;
    U32 ulNoticeTypeUnmaskLogId, ulIOSpecificTypeUnmaskLogId;
    U32 ulEventPendingStatus;
    U32 ulOutstdReqCount;
} ASYNC_EVENT_CONTROL, *PASYNC_EVENT_CONTROL;

typedef struct _NVME_TEMPERATURE_THRESHOLD_ENTRY
{
    U32 ulOverTempThrshld;
    U32 ulUnderTempThrshld;
} NVME_TEMPERATURE_THRESHOLD_ENTRY;

typedef struct _NVME_TEMPERATURE_THRESHOLD_TABLE
{
    NVME_TEMPERATURE_THRESHOLD_ENTRY aSensorEntryArray[9];
    NVME_TEMPERATURE_THRESHOLD_ENTRY tDefaultCompositeEntry;
} NVME_TEMPERATURE_THRESHOLD_TABLE;

#if 0
typedef struct _ASYNC_EVENT_REQ_QUEUE
{
    U16 aCIDQueue[MAX_ASYN_EVENT_NUM];
    U8 ucCIDHead;
    U8 ucCIDTail;
    U8 ucOutstdReqNum;
} ASYNC_EVENT_REQ_QUEUE;

typedef struct _ASYNC_EVENT_PENDING_QUEUE
{
    ASYNC_EVENT_RESPONSE aRespQueue[ASYNC_EVENT_BUFFER_DEPTH];
    U32 ucRespHead;
    U32 ucRespTail;
    U32 ucOutstdReqNum;
} ASYNC_EVENT_REQ_QUEUE;
#endif

#endif

#define SQ_NOT_COMPLETE       0
#define SQ_COMPLETE           1

typedef struct _nvme_mgr *PNVME_MGR;
typedef U32 (*PNVMEMGR_HANDLER)( PNVME_MGR );

typedef struct _nvme_mgr
{
    U32        NVMeCmdRunning;
    U32        AdminProcessing;
    U32        AdminProcessingSlot; 
    U32        ActiveSlot;
    U32        IOSQCnt;
    U32        ValidIOSQMap;

    U16        IOSQState[MAX_IOSQ_DEPTH];//each bit for one sq
    Q_INFO     SQ[MAX_SQ_NUM];
    Q_INFO     CQ[MAX_CQ_NUM];
    ASYNC_EVENT_CONTROL    AsyncEvtCtrl;
    CB_MGR     CbMgr[MAX_SLOT_NUM];
} NVME_MGR;//,*PNVME_MGR;

#define MAX_L0MSG_QDEPTH    32
#define MAX_L0MSG_PARAM_NUM  4   

typedef struct _L0_MSG_NODE
{
    union
    {
        struct {
            U32 ucParamLen : 8;
            U32 ucRsv : 8;
            U32 ucSubType : 8;
            U32 ucMsgType : 8;
        };
        U32 ulHeader;
    };

    U32 Param[MAX_L0MSG_PARAM_NUM];
}L0MSGNODE, *PL0MSGNODE;

typedef struct _L0_MSG_QUEUE
{
    volatile U32 ulTail;
    volatile U32 ulHead;
    volatile U32 ulMaxSize: 16;
    volatile U32 ulNodeNum: 16;

    L0MSGNODE MsgNode[MAX_L0MSG_QDEPTH];
}L0MSGQ, *PL0MSGQ;

BOOL L0_InitMsgQueue(void);
BOOL L0_IsMsgQueueEmpty(void);
BOOL L0_IsMsgQueueFull(void);
U32  L0_GetMsgQueueNodeNum(void);
BOOL L0_PushMsgQueueNode(PL0MSGNODE pNewMsgNode);
PL0MSGNODE L0_GetMsgQueueNode(void);
BOOL L0_MoveMsgQueueHead(void);
BOOL L0_MoveMsgQueueTail(void);
BOOL L0_PopMsgQueueNode(PL0MSGNODE pMsgNode);
BOOL L0_PushMsgQueueUeccNode(U32 ulSubSytemID, U32 ulSlotID, U32 ulPUID);
BOOL L0_ProceesMsgQueueNode(PL0MSGNODE);
void L0_ProcessMsgQueue(void);

#define L0MSG_TYPE_MASK             0xFF000000
#define L0MSG_TYPE_SHIF             (24)
#define L0MSG_SUBTYPE_MASK          0x00FF0000
#define L0MSG_SUBTYPE_SHIF          (16)
#define L0MSG_PARAMLEN_MASK         0x000000FF
#define L0MSG_PARAMLEN_SHIF         (0) 

//L0MSG Major Type
#define L0MSG_TYPE_ERROR            0x01

//L0MSG Minor(Sub) Type
#define L0MSG_SUBTYPE_UECCERROR    0x01


//UeccError
#define L0MSG_SUBTYPE_UECCERROR_PARAMSIZE   (1)
#define L0MSG_SUBTYPE_UECCERROR_SYSID_MASK  0xFF000000
#define L0MSG_SUBTYPE_UECCERROR_SYSID_SHIF  (24)    
#define L0MSG_SUBTYPE_UECCERROR_PUID_MASK  0x0000FF00
#define L0MSG_SUBTYPE_UECCERROR_PUID_SHIF  (8) 
#define L0MSG_SUBTYPE_UECCERROR_SLOTID_MASK 0x000000FF
#define L0MSG_SUBTYPE_UECCERROR_SLOTID_SHIF (0) 




#define DISK_BASE_ADDRESS       ( DRAM_START_ADDRESS + 0x4220000)   // 0x4422 0000

U32 L0_NVMeBarSpaceInit(void);
#ifdef AF_ENABLE
void L0_NVMeCfgExInit(void);
void L0_NVMeDisableSQAF(U32);
void L0_UpdateSQHead(PCB_MGR pSlot);
#endif
U32 L0_NVMeMemAlloc(U32 ulStartAddr);
void L0_NVMeTask(void);

void L0_NVMeMgrInit(PNVME_MGR Mgr);
void L0_MovePRPtoPRPList(PCB_MGR pSlot);
U32 L0_CheckNeedToFetchPRPList(PCB_MGR pCurrSlot);
U32 L0_CalcPRPCount(U32 ulPrp1DW0,U32 ulCmdDataLen);
U32 L0_IsPRPListOffsetValid(PCB_MGR pSlot);

void L0_InitIdentifyData(void);
void L0_NVMeRecycleCbMgr( PCB_MGR CbMgr );
void L0_NVMeCheckSQDoorBell(void);
void L0_NVMeUpdateCrtclWrng(U32 ulRepAsyncEvent);
void L0_UpdateIOSQHead(PCB_MGR pSlot);
void L0_UpdateASQHead(PCB_MGR pSlot);


void L0_NVMeDisableAsyncEventReporting(void);
void L0_NVMeClearAsyncEventResvSlots(void);
U32 L0_NVMeGetAsyncEventLogPageId(U32 ulType, U32 ulSubtype);
U32 L0_NVMeIsAsyncEventEnabled(U32 ulType, U32 ulSubtype);
void L0_NVMeReportAsyncEvent(PASYNC_EVENT_RESPONSE pResp);
void L0_NVMeStashAsyncEvent(PASYNC_EVENT_RESPONSE pResp);
void L0_NVMeApplyStashedAsyncEvent(PCB_MGR pSlot);
void L0_NVMeGenerateAsyncEvent(U32 ulType, U32 ulSubtype);
void L0_NVMeClearAsyncEvent(U32 ulLogPageID);


void L0_InitLogData(void);
void L0_NVMeSetupSimpleCompletion(PCB_MGR pSlot, U32 ulWaitSGE, U32 ulUpdateSQHead);
void L0_NVMeSetCmdParam(PCB_MGR pSlot);
void L0_NVMeHCmdTOCallback(U8 ucHID);

void L0_NVMeEventRegister(void);
BOOL L0_NVMeCheckHostIdle(void);
BOOL L0_NVMeCheckPCIEResetIdle(void);
extern void L0_NVMeSchedule(void);

BOOL FwCrcChk(U32 ulFwBaseAddr);
void L0_NVMeForceCompleteCmd(PCB_MGR pSlot, U32 ulStatus);
void L0_NVMePhySetting(void);
void L0_NVMeCfgLock(void);
void L0_NVMeCfgUnlock(void);
void L0_NVMePrepareL12Entry(void);
void L0_NVMePrepareL12Exit(void);

/* haven add for performance optimization and coding style */
extern volatile struct hal_nvmecfg *g_pNVMeCfgReg;
extern GLOBAL NVME_MGR gNvmeMgr;
#define CC_EN()                 (((volatile NVMe_CC*)&g_pNVMeCfgReg->bar.cc)->EN)
#define CC_SHN()                (((volatile NVMe_CC*)&g_pNVMeCfgReg->bar.cc)->SHN)
#define SET_CC_SHN(Value)       (((volatile NVMe_CC*)&g_pNVMeCfgReg->bar.cc)->SHN = (Value))
#define SET_CSTS_SHST(Value)    (((volatile NVMe_CSTS*)&(g_pNVMeCfgReg->bar.csts))->SHST = (Value))

#define CLR_DEV_RDY()           (((volatile NVMe_CSTS*)&(g_pNVMeCfgReg->bar.csts))->RDY = 0)
#define SET_DEV_RDY()           (((volatile NVMe_CSTS*)&(g_pNVMeCfgReg->bar.csts))->RDY = 1)
#define SET_FATAL_ERR()           (((volatile NVMe_CSTS*)&(g_pNVMeCfgReg->bar.csts))->CFS = 1)

#define HAVE_NVME_CMD()         (g_pNVMeCfgReg->cmd_fetch_helper)
#define NOT_ADMIN_QUEUQ(SqId)     (SqId != 0)
#define CQ_HEAD_DOORBELL(CQID)  (g_pNVMeCfgReg->doorbell[(CQID)].cq_head)
#define SQ_TAIL_DOORBELL(SQID)  (g_pNVMeCfgReg->doorbell[(SQID)].sq_tail)
#define SQ_HEAD(SQID)           (g_pNVMeCfgReg->sq_ptr[(SQID)].head)
#define SQ_SIZE(SQID)           (gNvmeMgr.SQ[(SQID)].QSIZE)
#define IOSQ_COUNT()              (gNvmeMgr.IOSQCnt)

#ifdef AF_ENABLE
#define GET_SQ_ID(SlotNum)      (g_pNVMeCfgExReg->SQEntry[SlotNum].SQID)
#define GET_SQ_INDEX(SlotNum)   (g_pNVMeCfgExReg->SQEntry[SlotNum].HWRP)
#define UPDATE_SQ_STATE(SlotNum)   (gNvmeMgr.IOSQState[GET_SQ_INDEX(SlotNum)] &= ~(1<<(GET_SQ_ID(SlotNum))))
#else
#define SQ_FWRP(SQID)           (g_pNVMeCfgReg->sq_ptr[(SQID)].fw_read)
#define SQ_EMPTY(SQID)          (SQ_FWRP(SQID) == SQ_TAIL_DOORBELL(SQID))
#define SQ_NOT_EMPTY(SQID)      (SQ_FWRP(SQID) != SQ_TAIL_DOORBELL(SQID))
#define SQ_FULL(SQID)           (SQ_FWRP(SQID) == (SQ_TAIL_DOORBELL(SQID) + 1) % SQ_SIZE(SQID))
#define PUSH_SQ_FWRP(SQID)      (SQ_FWRP(SQID) = (SQ_FWRP(SQID) + 1)%SQ_SIZE(SQID))
#define UPDATE_IOSQ_STATE(SQID)   (gNvmeMgr.IOSQState[SQ_FWRP(SQID)] &= ~(1<<(SQID - 1)))
#ifdef SIM
#define HAVE_NVME_ADMIN_CMD()   (SQ_NOT_EMPTY(0))
#define HAVE_NVME_IO_CMD(SQID)  (SQ_NOT_EMPTY(SQID))
#else
#define HAVE_NVME_ADMIN_CMD()   (g_pNVMeCfgReg->cmd_fetch_helper & 0x01)
#define HAVE_NVME_IO_CMD(SQID)  (g_pNVMeCfgReg->cmd_fetch_helper & (1 << (SQID)))
#endif  //SIM
#endif  //NON_AF_ENALBE

#define CQ_TAIL(CQID)           (gNvmeMgr.CQ[CQID].CqTail)
#define CQ_SIZE(CQID)           (gNvmeMgr.CQ[CQID].QSIZE)
#define CQ_FULL(CQID)           (CQ_HEAD_DOORBELL(CQID) == (CQ_TAIL(CQID) + 1)%CQ_SIZE(CQID))
#define CQ_NOT_FULL(CQID)       (CQ_HEAD_DOORBELL(CQID) != (CQ_TAIL(CQID) + 1)%CQ_SIZE(CQID))
#define PUSH_CQ_TAIL(CQID)      (CQ_TAIL(CQID) = (CQ_TAIL(CQID)+1)%CQ_SIZE(CQID))

#define GET_SLOT_MGR(SlotNum)   (&(gNvmeMgr.CbMgr[(SlotNum)]))
#define SQID_IN_SLOT(SlotNum)   (gNvmeMgr.CbMgr[(SlotNum)].SQID)

#define SQ_VLD(SQID)            (TRUE == gNvmeMgr.SQ[(SQID)].Valid)
#define CQ_VLD(CQID)            (TRUE == gNvmeMgr.CQ[(CQID)].Valid)
#define SQ_IVLD(SQID)           (FALSE == gNvmeMgr.SQ[(SQID)].Valid)
#define CQ_IVLD(CQID)           (FALSE == gNvmeMgr.CQ[(CQID)].Valid)
#define BIND_CQID(SQID)         (gNvmeMgr.SQ[(SQID)].BindCQID)
#define SQ_HOST_BAH(SQID)       (gNvmeMgr.SQ[(SQID)].AdrH)
#define SQ_HOST_BAL(SQID)       (gNvmeMgr.SQ[(SQID)].AdrL)


#define STOP_NVME()     (gNvmeMgr.NVMeCmdRunning = FALSE)
#define START_NVME()    (gNvmeMgr.NVMeCmdRunning = TRUE)
#define NVME_RUNNING()  (TRUE == gNvmeMgr.NVMeCmdRunning)
#define ASYNC_EVENT_HAPPENED()  (gNvmeMgr.AsynEvtMgr.AsU32>>16)

#ifdef AF_ENABLE
#define STOP_AF()       (g_pHCTControlReg->bAUTOFCHEN = FALSE)
#define START_AF()      (g_pHCTControlReg->bAUTOFCHEN = TRUE)
#define AF_RUNNING()    (TRUE == g_pHCTControlReg->bAUTOFCHEN)
#endif

#define SET_CQ_DW1(Slot,Value)  (*(volatile U32*)(REG_BASE_EXT_NVME_CQDW + (Slot)*4) = (Value))
#define PRP_ABILITY(ulPrpLow)   (HPAGE_SIZE - ((ulPrpLow) & HPAGE_SIZE_MSK))

#endif // _NVME_H_

