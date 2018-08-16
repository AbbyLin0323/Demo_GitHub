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

#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include "BaseDef.h"
#include "model_common.h"
#include "nvmeStd.h"
#include "nvmeReg.h"
#include "nvme.h"


#include "base_pattern.h"
#include "HostModel.h"
#include "HostInc.h"
#include "NVME_HostCommand.h"
#include "NVME_ControllerModel.h"
#include "HAL_HostInterface.h"

extern volatile NVMe_CONTROLLER_REGISTERS *g_pCtrlReg;

NVMe_COMMAND Admin_SQ[ADMIN_Q_DEPTH];
NVMe_COMPLETION_QUEUE_ENTRY Admin_CQ[ADMIN_Q_DEPTH];
U16 Admin_SQTail;
U16 Admin_CQHead;
U16 Admin_P_bit = 0;

NVMe_COMMAND IO_SQ[IO_QCNT][IO_Q_DEPTH];
NVMe_COMPLETION_QUEUE_ENTRY IO_CQ[IO_QCNT][IO_Q_DEPTH];
ULONGLONG PRP_list[IO_QCNT][IO_Q_DEPTH][256]; //8 IO queues, 64 PRP_list, each list has 256 PRP entry

U16 IO_SQTail[IO_QCNT];
U16 IO_SQHead[IO_QCNT];
CRITICAL_SECTION g_csCIDCriticalSection;

typedef struct _CIDSTATE
{
    U16 ucStatus;
    U16 usSQPointer;
    U32 ulStartLba;
    U32 ulSecCnt;
}CIDSTATE;

volatile CIDSTATE g_tCIDState[IO_QCNT][IO_Q_DEPTH];

U32 CurQNum; //The queue number used for processing incoming command, use round robin to decide the queue

U16 IO_DriverHead[IO_QCNT];
U8 IO_DriverP[IO_QCNT];

U8 *g_pHostDataBuffer;

HANDLE hEventMSI;
HANDLE hThreadMSI;

U32 CalcPRPCnt(U8* pDataBuffer,U32 ulSecCnt);
LOCAL U32 GetCIDStatus(U32 ucSQId,U32 ulCID);

/*
* Allocate Admin CQ and SQ
* Setup IO CQ and SQ
*/

void NVMeCreateCQ(ULONG QID)
{
    // Setup IO CQ
    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_CREATE_IO_COMPLETION_QUEUE;

    Admin_SQ[Admin_SQTail].PRP1 = (ULONGLONG)&IO_CQ[QID][0];

    Admin_SQ[Admin_SQTail].CDW10 = (63 << 16) | (QID + 1);
    Admin_SQ[Admin_SQTail].CDW11 = 3;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;

    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

void NVMeCreateSQ(ULONG QID)
{
    // Setup IO SQ1
    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_CREATE_IO_SUBMISSION_QUEUE;

    Admin_SQ[Admin_SQTail].PRP1 = (ULONGLONG)&IO_SQ[QID][0];

    Admin_SQ[Admin_SQTail].CDW10 = ((IO_Q_DEPTH - 1) << 16) | (QID + 1);
    Admin_SQ[Admin_SQTail].CDW11 = ((QID + 1) << 16) | 1;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

void NVMeDeleteCQ(ULONG QID)
{
    // Delete IO CQ
    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_DELETE_IO_COMPLETION_QUEUE;

    Admin_SQ[Admin_SQTail].PRP1 = (ULONGLONG)&IO_CQ[QID][0];

    Admin_SQ[Admin_SQTail].CDW10 = (63 << 16) | (QID + 1);
    Admin_SQ[Admin_SQTail].CDW11 = 3;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;

    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

void NVMeDeleteSQ(ULONG QID)
{
    //Delete IO SQ
    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_DELETE_IO_SUBMISSION_QUEUE;

    Admin_SQ[Admin_SQTail].PRP1 = (ULONGLONG)&IO_SQ[QID][0];

    Admin_SQ[Admin_SQTail].CDW10 = ((IO_Q_DEPTH - 1) << 16) | (QID + 1);
    Admin_SQ[Admin_SQTail].CDW11 = ((QID + 1) << 16) | 1;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

void NVMeSendIdentify(HSCMD_INFO* pHSCmd)
{
    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_IDENTIFY;
    Admin_SQ[Admin_SQTail].NSID = 0;

    Admin_SQ[Admin_SQTail].PRP1 = (U32)pHSCmd->pDataBuffer;

    Admin_SQ[Admin_SQTail].CDW10 = 0x1;
    Admin_SQ[Admin_SQTail].CDW11 = 0;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

void NVMeSendAbort(HSCMD_INFO* pHSCmd)
{
    U16 usSQID = rand() % IO_QCNT;
    U16 usCID = rand() % IO_Q_DEPTH;

    if(0 == GetCIDStatus(usSQID,usCID))
    {
        return;
    }

    DBG_Printf("Host Abort SQ %d CID 0x%x SQIndex 0x%x,CID Status 0x%x\n",
        usSQID,usCID,g_tCIDState[usSQID][usCID].usSQPointer,g_tCIDState[usSQID][usCID].ucStatus);

    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_ABORT;

    Admin_SQ[Admin_SQTail].PRP1 = (U32)pHSCmd->pDataBuffer;

    Admin_SQ[Admin_SQTail].CDW10 = ((usCID << 16) | usSQID);
    Admin_SQ[Admin_SQTail].CDW11 = 0;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}
void NVMeSendFWActive(HSCMD_INFO* pHSCmd)
{
    NVMe_COMMAND *pAdminCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;
    U8 ucFwSlot = 1;
    //static U8 ucCommitAction = 0;  //Loopback Active test via a static variable.
    U8 ucCommitAction = 0;

    ucCommitAction = pAdminCmd->CDW10 & 0x18;

    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_FIRMWARE_ACTIVATE;

    Admin_SQ[Admin_SQTail].PRP1 = (U32)pHSCmd->pDataBuffer;

    //Admin_SQ[Admin_SQTail].CDW10 =  (((ucCommitAction++)%4) << 3) | (ucFwSlot & 0x7);Loopback Active test
    Admin_SQ[Admin_SQTail].CDW10 =  ucCommitAction | (ucFwSlot & 0x7);
    Admin_SQ[Admin_SQTail].CDW11 = 0;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

void NVMeSendFWImageDownload(HSCMD_INFO* pHSCmd)
{
    NVMe_COMMAND *pAdminCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;
    U32 ulSecCnt;
    U32 ulPRPCnt;
    U32 i;
    U8* pDataBuffer = pHSCmd->pDataBuffer;

    ulSecCnt = (pAdminCmd->CDW10 + 1)*sizeof(U32)/SEC_SIZE;
    ulPRPCnt = CalcPRPCnt(pHSCmd->pDataBuffer,ulSecCnt);

    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_FIRMWARE_IMAGE_DOWNLOAD;

    Admin_SQ[Admin_SQTail].PRP1 = (U32)pHSCmd->pDataBuffer;

    for(i = 0; i < ulPRPCnt; i++)
    {
        if(i == 0) // first PRP
        {
            Admin_SQ[Admin_SQTail].PRP1 = (ULONGLONG)(pDataBuffer);
            Admin_SQ[Admin_SQTail].PRP2 = (ULONGLONG)NULL;
        }
        else if(i == 1) // second PRP
        {
            //PAGE_SZ is 4K,it means every PRP can transfer 4K data
            Admin_SQ[Admin_SQTail].PRP2 = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE) & (~HPAGE_SIZE_MSK);
        }
        else // more than 2 PRPs
        {
            Admin_SQ[Admin_SQTail].PRP2 = (ULONGLONG)&PRP_list[0][0];

            PRP_list[0][0][0] = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE) & (~HPAGE_SIZE_MSK);
            PRP_list[0][0][i-1] = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE * i) & (~HPAGE_SIZE_MSK);
        }
    }
    Admin_SQ[Admin_SQTail].CDW10 = pAdminCmd->CDW10;
    Admin_SQ[Admin_SQTail].CDW11 = pAdminCmd->CDW11;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}
void NVMeSendGetLogPage(HSCMD_INFO* pHSCmd)
{
    NVMe_COMMAND *pAdminCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;

    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = ADMIN_GET_LOG_PAGE;

    Admin_SQ[Admin_SQTail].PRP1 = (U32)pHSCmd->pDataBuffer;

    Admin_SQ[Admin_SQTail].CDW10 = pAdminCmd->CDW10;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
    return;
}

void NVMeSendViaVenderCmd(HSCMD_INFO* pHSCmd)
{
    U32 i;
    U8* pDataBuffer = pHSCmd->pDataBuffer;
    NVMe_COMMAND *pAdminCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;
    U32 ulSecCnt = 0;
    U32 ulPRPCnt = 0;
    U8 ucViaSubCmd = pAdminCmd->CDW12 & 0xFF;

    switch(ucViaSubCmd)
    {
    case VIA_SUBCMD_MEM_READ:
    case VIA_SUBCMD_MEM_WRITE:
        ulSecCnt = pAdminCmd->CDW14/SEC_SIZE;
        ulPRPCnt = CalcPRPCnt(pHSCmd->pDataBuffer,ulSecCnt);
        break;
    case VIA_SUBCMD_FLASH_READ:
    case VIA_SUBCMD_FLASH_WRITE:
    case VIA_SUBCMD_FLASH_ERASE:
        ulSecCnt = 1;//Buffer is used for status
        ulPRPCnt = CalcPRPCnt(pHSCmd->pDataBuffer,ulSecCnt);
        break;
    default:
        break;
    }

    Admin_SQ[Admin_SQTail].CDW0.CID = Admin_SQTail;
    Admin_SQ[Admin_SQTail].CDW0.FUSE = 0;
    Admin_SQ[Admin_SQTail].CDW0.OPC = pAdminCmd->CDW0.OPC;

    Admin_SQ[Admin_SQTail].PRP1 = (U32)pHSCmd->pDataBuffer;

    for(i = 0; i < ulPRPCnt; i++)
    {
        if(i == 0) // first PRP
        {
            Admin_SQ[Admin_SQTail].PRP1 = (ULONGLONG)(pDataBuffer);
            Admin_SQ[Admin_SQTail].PRP2 = (ULONGLONG)NULL;
        }
        else if(i == 1) // second PRP
        {
            //PAGE_SZ is 4K,it means every PRP can transfer 4K data
            Admin_SQ[Admin_SQTail].PRP2 = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE) & (~HPAGE_SIZE_MSK);
        }
        else // more than 2 PRPs
        {
            Admin_SQ[Admin_SQTail].PRP2 = (ULONGLONG)&PRP_list[0][0];

            PRP_list[0][0][0] = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE) & (~HPAGE_SIZE_MSK);
            PRP_list[0][0][i-1] = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE * i) & (~HPAGE_SIZE_MSK);
        }
    }

    Admin_SQ[Admin_SQTail].CDW10 = pAdminCmd->CDW10;
    Admin_SQ[Admin_SQTail].CDW11 = pAdminCmd->CDW11;
    Admin_SQ[Admin_SQTail].CDW12 = pAdminCmd->CDW12;
    Admin_SQ[Admin_SQTail].CDW13 = pAdminCmd->CDW13;
    Admin_SQ[Admin_SQTail].CDW14 = pAdminCmd->CDW14;
    Admin_SQ[Admin_SQTail].CDW15 = CMD_TYPE_ADMIN;

    Admin_SQTail = (Admin_SQTail + 1) % ADMIN_Q_DEPTH;
    g_pCtrlReg->Admin_SQT.QHT = Admin_SQTail;
}

BOOL NVMe_HostSendAdminCmd(HSCMD_INFO* pHSCmd)
{
    NVMe_COMMAND *pAdminCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;

    switch(pAdminCmd->CDW0.OPC)
    {
    case ADMIN_IDENTIFY:
        if (FALSE == Host_IsCMDEmptyInterface())
        {
            return FALSE;
        }
        NVMeSendIdentify(pHSCmd);
        break;
    case ADMIN_VIA_VENDER_CMD:
        if (FALSE == Host_IsCMDEmptyInterface())
        {
            return FALSE;
        }
        NVMeSendViaVenderCmd(pHSCmd);
        break;
    case ADMIN_ABORT:
        NVMeSendAbort(pHSCmd);
        break;
    case ADMIN_GET_LOG_PAGE:
        NVMeSendGetLogPage(pHSCmd);
        break;
    case ADMIN_FIRMWARE_ACTIVATE:
        if (FALSE == Host_IsCMDEmptyInterface())
        {
            return FALSE;
        }
        NVMeSendFWActive(pHSCmd);
        break;
    case ADMIN_FIRMWARE_IMAGE_DOWNLOAD:
        if (FALSE == Host_IsCMDEmptyInterface())
        {
            return FALSE;
        }
        NVMeSendFWImageDownload(pHSCmd);
        break;
    default:
        DBG_Break();
    }

    while(FALSE == NVMe_IsAdminCmdFinish())
    {
        Sleep(5);
    }

    return TRUE;
}

void NVMeDriverInit(void)
{
    NVMe_CONTROLLER_CONFIGURATION CC;
    NVMe_CONTROLLER_STATUS CSTS;
    U32 i,j;

    g_pCtrlReg->AQA.ACQS = IO_Q_DEPTH - 1;
    g_pCtrlReg->AQA.ASQS = IO_Q_DEPTH - 1;
    g_pCtrlReg->ACQ.pCQ = &Admin_CQ[0];
    g_pCtrlReg->ASQ.pSQ = &Admin_SQ[0];

    CC.AsUlong = 0;
    CC.CSS = NVME_CC_NVM_CMD;
    CC.MPS = (HPAGE_SIZE >> NVME_MEM_PAGE_SIZE_SHIFT);
    CC.AMS = NVME_CC_ROUND_ROBIN;
    CC.SHN = NVME_CC_SHUTDOWN_NONE;
    CC.IOSQES = NVME_CC_IOSQES;
    CC.IOCQES = NVME_CC_IOSQES;

    CSTS.AsUlong = 0;

    g_pCtrlReg->CC.AsUlong = CC.AsUlong;
    g_pCtrlReg->CSTS.AsUlong = CSTS.AsUlong;

    g_pCtrlReg->Admin_SQT.AsUlong = 0;
    g_pCtrlReg->Admin_CQH.AsUlong = 0;

    for(i = 0; i < IO_QCNT; i++)
    {
        g_pCtrlReg->IO_DB[i][0].AsUlong = 0; // SQ Tail doorbell for Queue i
        g_pCtrlReg->IO_DB[i][1].AsUlong = 0; // CQ Head doorbell for Queue i
    }

    Admin_SQTail = 0;
    Admin_CQHead = 0;
    Admin_P_bit = 0;
    memset(&Admin_SQ[0],0,sizeof(NVMe_COMMAND)*ADMIN_Q_DEPTH);
    memset(&Admin_CQ[0],0,sizeof(NVMe_COMPLETION_QUEUE_ENTRY)*IO_Q_DEPTH);
    InitializeCriticalSection(&g_csCIDCriticalSection);

    CurQNum = 0;
    for(i = 0; i < IO_QCNT; i++)
    {
        IO_SQTail[i] = 0;
        IO_SQHead[i] = 0;
        IO_DriverHead[i] = 0;
        IO_DriverP[i] = 0;

        for(j = 0; j < IO_Q_DEPTH; j++)
        {
            g_tCIDState[i][j].ucStatus = 0;
            g_tCIDState[i][j].usSQPointer = INVALID_4F;
            g_tCIDState[i][j].ulStartLba = INVALID_8F;
            g_tCIDState[i][j].ulSecCnt = INVALID_8F;
        }
        memset(&IO_SQ[i],0,sizeof(NVMe_COMMAND)*IO_Q_DEPTH);
        memset(&IO_CQ[i],0,sizeof(NVMe_COMPLETION_QUEUE_ENTRY)*IO_Q_DEPTH);
    }
}

BOOL NVMe_IsSQEmpty()
{
    BOOL bRtn = TRUE;
    U32 ulSQIndex;

    for(ulSQIndex = 0; ulSQIndex < IO_QCNT; ulSQIndex++)
    {
        if(IO_SQHead[ulSQIndex] != IO_SQTail[ulSQIndex])
        {
            bRtn = FALSE;
            break;
        }
    }
    return bRtn;
}

BOOL NVMe_IsPendingCmdEmpty()
{
    U32 ulSQIndex;
    U32 ulCID;

    for(ulSQIndex = 0; ulSQIndex < IO_QCNT; ulSQIndex++)
    {
        for (ulCID = 0; ulCID < IO_Q_DEPTH; ulCID++)
        {
            if(1 == g_tCIDState[ulSQIndex][ulCID].ucStatus)
            {
                return FALSE;
            }
        }

    }

    return TRUE;
}

BOOL Host_IsCMDEmptyInterface()
{
    BOOL bEmpty = FALSE;

    if(TRUE == NVMe_IsPendingCmdEmpty())
    {
        bEmpty = TRUE;
    }

    return bEmpty;
}

extern void L0_NVMeShutdownISR(void);
extern void L0_NVMeCcEnISR(void);
void NVMe_SetShutdownReg(U32 ulShutdownType)
{
    if (0 != ulShutdownType)
    {
        g_pCtrlReg->CC.SHN = ulShutdownType;
        L0_NVMeShutdownISR();
    }
}

void NVMe_SetCcEn(U32 ulEnable)
{
    g_pCtrlReg->CC.EN = ulEnable;
    L0_NVMeCcEnISR();
}

U32 NVMe_IsControllerReady(void)
{
    return g_pCtrlReg->CSTS.RDY;
}

void NVMe_WaitShutdownFinish(void)
{
    while(0x2 != g_pCtrlReg->CSTS.SHST)
    {
        Sleep(5);
    }
}

void Host_SimClearHCmdQueueInterface(void)
{
    U32 i;

    NVMe_ClearHostCmdTable();

    for(i = 0;i < IO_QCNT;i++)
    {
        memset(&IO_SQ[i],0,sizeof(NVMe_COMMAND)*IO_Q_DEPTH);
    }
}


BOOL NVMe_IsAdminCmd(U8 OPC)
{
    BOOL bRtn = FALSE;
    switch(OPC)
    {
    case ADMIN_DELETE_IO_SUBMISSION_QUEUE:
    case ADMIN_CREATE_IO_SUBMISSION_QUEUE:
    case ADMIN_GET_LOG_PAGE:
    case ADMIN_DELETE_IO_COMPLETION_QUEUE:
    case ADMIN_CREATE_IO_COMPLETION_QUEUE:
    case ADMIN_IDENTIFY:

    case ADMIN_ABORT:
    case ADMIN_SET_FEATURES:
    case ADMIN_GET_FEATURES:
    case ADMIN_ASYNCHRONOUS_EVENT_REQUEST:
    case ADMIN_FIRMWARE_ACTIVATE:
    case ADMIN_FIRMWARE_IMAGE_DOWNLOAD:
    case ADMIN_FORMAT_NVM:
    case ADMIN_SECURITY_SEND:
    case ADMIN_SECURITY_RECEIVE:

    case ADMIN_VIA_VENDER_CMD:

        bRtn = TRUE;
        break;
    default:
        break;
    }
    return bRtn;
}

void NVMe_HCmdToRowCmd(HCMD_INFO* pHcmd)
{
    U32* pBuf = (U32*)&pHcmd->HSCmd.RowCmd;
    HSCMD_INFO *pHSCmd = &pHcmd->HSCmd;
    NVMe_COMMAND *pNVMeCmd = (NVMe_COMMAND*)&pHcmd->HSCmd.RowCmd;
    U8 ucSubCmd;

    if(sizeof(NVMe_COMMAND) > sizeof(pHcmd->HSCmd.RowCmd))
    {
        DBG_Break();
    }

    pNVMeCmd->CDW0.FUSE = 0;
    switch(pHcmd->CmdType)
    {
    case CMD_TYPE_READ:
    case CMD_TYPE_READ_IO:
        pNVMeCmd->CDW0.OPC = NVM_READ;
        break;
    case CMD_TYPE_WRITE:
    case CMD_TYPE_WRITE_IO:
        pNVMeCmd->CDW0.OPC = NVM_WRITE;
        break;
    case CMD_TYPE_TRIM:
        pHSCmd->pDataBuffer = (U8 *)g_TrimCmdEntry;
        pNVMeCmd->CDW0.OPC = NVM_DATASET_MANAGEMENT;
        pNVMeCmd->CDW10 = pHcmd->RangeNum;
        pNVMeCmd->CDW11 |= (1<<2);
        pNVMeCmd->CDW12 = 0;
        pNVMeCmd->CDW15 = CMD_TYPE_SPECAIL;
        break;
    case CMD_TYPE_STANDBY:
        *pBuf = INVALID_8F;
        break;
    case CMD_TYPE_HSCMD:
        if(NVM_VIA_TRACE_GETLOGINFO == pNVMeCmd->CDW0.OPC)
        {
            pNVMeCmd->CDW12 = pHSCmd->DataSecCnt - 1;
        }
        else if(NVM_VIA_TRACE_GETLOGDATA == pNVMeCmd->CDW0.OPC)
        {
            pNVMeCmd->CDW10 = pHcmd->StartLba;
            pNVMeCmd->CDW11 = pHcmd->McuId;
            pNVMeCmd->CDW12 = pHcmd->SecCnt - 1;
        }
        else if(ADMIN_VIA_VENDER_CMD == pNVMeCmd->CDW0.OPC)
        {
            ucSubCmd = pNVMeCmd->CDW12 & 0xFF;
            if(VIA_SUBCMD_MEM_READ == ucSubCmd || VIA_SUBCMD_MEM_WRITE == ucSubCmd)
            {
                pNVMeCmd->CDW13 = pHcmd->StartLba; //DevAddr
                pNVMeCmd->CDW14 = pHcmd->SecCnt;   //DataLen
            }
            else if(VIA_SUBCMD_FLASH_READ == ucSubCmd || VIA_SUBCMD_FLASH_WRITE == ucSubCmd || VIA_SUBCMD_FLASH_ERASE == ucSubCmd)
            {
                pNVMeCmd->CDW13 = pHcmd->StartLba; //PuMsk
                pNVMeCmd->CDW14 = pHcmd->SecCnt;   //Blk & Page
            }

            pNVMeCmd->CDW12 |= (pHcmd->McuId << 8); //MCUID
        }
        pNVMeCmd->CDW15 = CMD_TYPE_SPECAIL;
        break;
    default:
        DBG_Printf("HCmd CmdType error\n");
        ASSERT(FAIL);
        break;
    }

    if (PROT_ADMIN != pHSCmd->Protocal)
    {
        if (NVM_READ == pNVMeCmd->CDW0.OPC || NVM_WRITE == pNVMeCmd->CDW0.OPC)
        {
            if ((pHcmd->StartLba + pHcmd->SecCnt) >= GetSystemMaxLBA())
            {
                pHcmd->SecCnt = GetSystemMaxLBA() - pHcmd->StartLba;
            }

            if (pHcmd->SecCnt > DATA_BUFFER_PER_CMD / SEC_SIZE)
            {
                pHcmd->SecCnt = DATA_BUFFER_PER_CMD / SEC_SIZE;
            }

            //make LBA 4K align
            pHcmd->StartLba = (pHcmd->StartLba >> SEC_PER_DATABLOCK_BITS) << SEC_PER_DATABLOCK_BITS;
            pHcmd->SecCnt = max(pHcmd->SecCnt, SEC_PER_DATABLOCK);

            pNVMeCmd->CDW10 = pHcmd->StartLba >> SEC_PER_DATABLOCK_BITS;
            pNVMeCmd->CDW11 = 0;
            pNVMeCmd->CDW12 = (pHcmd->SecCnt >> SEC_PER_DATABLOCK_BITS) - 1;

            pNVMeCmd->CDW15 = CMD_TYPE_IO;
        }
    }
}

LOCAL void NextSubmissionQueue(void)
{
    CurQNum = (CurQNum + 1) % IO_QCNT;
}

#if 0
BOOL NVME_IsHitSQLba(U32 ulStartLba, U32 ulSecCnt)
{
    U32 ulPendingLbaStart = 0;
    U32 ulPendingLbaEnd = 0;
    U32 ulPendingSecCnt = 0;
    BOOL bHit = FALSE;
    U32 ulSQIndex;
    U32 ulSQTail;
    U32 ulSQHead;
    U32 ulSQPointer;
    U32 i;
    U32 ulPendingCmdCnt;
    U32 ulNewLbaStart = ulStartLba;
    U32 ulNewLbaEnd = ulNewLbaStart + ulSecCnt - 1;

    for(ulSQIndex = 0; ulSQIndex < IO_QCNT; ulSQIndex++)
    {
        ulSQHead = IO_SQHead[ulSQIndex];
        ulSQTail = IO_SQTail[ulSQIndex];

        if(ulSQHead <= ulSQTail)
        {
            ulPendingCmdCnt = ulSQTail - ulSQHead;
        }
        else
        {
            ulPendingCmdCnt = ulSQTail + IO_Q_DEPTH - ulSQHead;
        }

        for (i = 0,ulSQPointer = ulSQHead; i < ulPendingCmdCnt; i++,ulSQPointer++,ulSQPointer %= IO_Q_DEPTH)
        {
            bHit = TRUE;
            ulPendingLbaStart = IO_SQ[ulSQIndex][ulSQPointer].CDW10;
            ulPendingSecCnt = IO_SQ[ulSQIndex][ulSQPointer].CDW12 + 1;

            ulPendingLbaEnd = ulPendingLbaStart + ulPendingSecCnt - 1;
            if ((ulNewLbaStart > ulPendingLbaEnd) || (ulNewLbaEnd < ulPendingLbaStart))
            {
                bHit = FALSE;
            }

            if (TRUE == bHit)
            {
                return bHit;
            }
        }
    }
    return bHit;
}
#endif

BOOL NVME_IsHitPendingCmd(U32 ulStartLba, U32 ulSecCnt)
{
    U32 ulPendingLbaStart = 0;
    U32 ulPendingLbaEnd = 0;
    U32 ulPendingSecCnt = 0;
    BOOL bHit = FALSE;
    U32 ulSQIndex;
    U32 ulCID;

    U32 ulNewLbaStart = ulStartLba;
    U32 ulNewLbaEnd = ulNewLbaStart + ulSecCnt - 1;


    for(ulSQIndex = 0; ulSQIndex < IO_QCNT; ulSQIndex++)
    {

        for (ulCID = 0; ulCID < IO_Q_DEPTH; ulCID++)
        {
            if(1 == g_tCIDState[ulSQIndex][ulCID].ucStatus)
            {
                ulPendingLbaStart = g_tCIDState[ulSQIndex][ulCID].ulStartLba;
                ulPendingSecCnt = g_tCIDState[ulSQIndex][ulCID].ulSecCnt;
                if(INVALID_8F == ulPendingLbaStart && INVALID_8F == ulPendingSecCnt)
                {
                    continue;
                }

                ulPendingLbaEnd = ulPendingLbaStart + ulPendingSecCnt - 1;
                if ((ulNewLbaStart > ulPendingLbaEnd) || (ulNewLbaEnd < ulPendingLbaStart))
                {
                    bHit = FALSE;
                }
                else
                {
                    return TRUE;
                }
            }
        }

    }

    return bHit;
}

BOOL Host_IsHitHCmdTable(U32 ulSystemLba)
{
    U32 ulSecCnt = 1;

    if(TRUE == NVME_IsHitPendingCmd(ulSystemLba,ulSecCnt))
    {
        return TRUE;
    }

    return FALSE;
}
LOCAL U32 GetAvailableCID(U8 ucSQId)
{
    U32 i;
    U32 ulCID = INVALID_8F;

    //find available CID for current SQ
    for(i = 0; i < IO_Q_DEPTH; i++)
    {
        if(0 == g_tCIDState[ucSQId][i].ucStatus)
        {
            ulCID = i;
            break;
        }
    }
    return ulCID;
}

LOCAL void SetCIDStatus(U32 ucSQId,U32 ulCID,U16 usSQPointer,U32 ulStartLba,U32 ulSecCnt)
{
    EnterCriticalSection(&g_csCIDCriticalSection);

    if(INVALID_4F != g_tCIDState[ucSQId][ulCID].usSQPointer)
    {
        DBG_Break();
    }
    g_tCIDState[ucSQId][ulCID].ucStatus = 1;
    g_tCIDState[ucSQId][ulCID].usSQPointer = usSQPointer;
    g_tCIDState[ucSQId][ulCID].ulStartLba = ulStartLba;
    g_tCIDState[ucSQId][ulCID].ulSecCnt = ulSecCnt;

    LeaveCriticalSection(&g_csCIDCriticalSection);

}

LOCAL void ClearCIDStatus(U32 ucSQId,U32 ulCID)
{
    EnterCriticalSection(&g_csCIDCriticalSection);

    g_tCIDState[ucSQId][ulCID].ucStatus = 0;
    g_tCIDState[ucSQId][ulCID].usSQPointer = INVALID_4F;
    g_tCIDState[ucSQId][ulCID].ulStartLba = INVALID_8F;
    g_tCIDState[ucSQId][ulCID].ulSecCnt = INVALID_8F;

    LeaveCriticalSection(&g_csCIDCriticalSection);
}

LOCAL U32 GetCIDStatus(U32 ucSQId,U32 ulCID)
{
    U32 ulStatus;

    EnterCriticalSection(&g_csCIDCriticalSection);

    ulStatus =  g_tCIDState[ucSQId][ulCID].ucStatus;

    LeaveCriticalSection(&g_csCIDCriticalSection);

    return ulStatus;
}

void NVMe_DeleteIOQ()
{
    U32 i;

    for(i = 0; i < IO_QCNT; i++)
    {
        NVMeDeleteSQ(i);
        NVMeDeleteCQ(i);
    }
}

BOOL NVMe_IsAdminCmdFinish()
{
    while (Admin_CQ[Admin_CQHead].DW3.SF.P != Admin_P_bit)
    {
        if(Admin_CQHead == (ADMIN_Q_DEPTH - 1))
        {
            Admin_CQHead = 0;
            Admin_P_bit ^= 1; //revert P bit
        }
        else
        {
            Admin_CQHead++;
        }

        //Update Admin CQ head doorbell
        g_pCtrlReg->Admin_CQH.QHT = Admin_CQHead;
    }

    if(Admin_CQHead == Admin_SQTail)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void NVMe_FillNVMeCmd( NVMe_COMMAND *pNVMeCmd,U8* pDataBuffer,U32 ulPRPCnt)
{
    U32 i;
    U32 ulSQTail = IO_SQTail[CurQNum];
    U32 ulCID = pNVMeCmd->CDW0.CID;
    U32 CmdCode = pNVMeCmd->CDW0.OPC;

    IO_SQ[CurQNum][ulSQTail].CDW0.FUSE = 0;
    IO_SQ[CurQNum][ulSQTail].CDW0.OPC  = pNVMeCmd->CDW0.OPC;
    IO_SQ[CurQNum][ulSQTail].CDW0.CID  = pNVMeCmd->CDW0.CID;
    IO_SQ[CurQNum][ulSQTail].NSID = pNVMeCmd->NSID;

    if(NVM_WRITE == CmdCode || NVM_READ == CmdCode)
    {
        SetCIDStatus(CurQNum,ulCID,ulSQTail,(pNVMeCmd->CDW10 << SEC_PER_DATABLOCK_BITS),(pNVMeCmd->CDW12+1) << SEC_PER_DATABLOCK_BITS);
    }
    else
    {
        SetCIDStatus(CurQNum,ulCID,ulSQTail,INVALID_8F,INVALID_8F);
    }

    for(i = 0; i < ulPRPCnt; i++)
    {
        if(i == 0) // first PRP
        {
            IO_SQ[CurQNum][ulSQTail].PRP1 = (ULONGLONG)(pDataBuffer);
            IO_SQ[CurQNum][ulSQTail].PRP2 = (ULONGLONG)NULL;
        }
        else if(i == 1) // second PRP
        {
            //PAGE_SZ is 4K,it means every PRP can transfer 4K data
            IO_SQ[CurQNum][ulSQTail].PRP2 = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE) & (~HPAGE_SIZE_MSK);
        }
        else // more than 2 PRPs
        {
            IO_SQ[CurQNum][ulSQTail].PRP2 = (ULONGLONG)&PRP_list[CurQNum][ulCID];

            PRP_list[CurQNum][ulCID][0] = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE) & (~HPAGE_SIZE_MSK);
            PRP_list[CurQNum][ulCID][i-1] = ((ULONGLONG)(pDataBuffer) + HPAGE_SIZE * i) & (~HPAGE_SIZE_MSK);
        }
    }

    IO_SQ[CurQNum][ulSQTail].MPTR = 0;
    IO_SQ[CurQNum][ulSQTail].CDW10 = pNVMeCmd->CDW10;
    IO_SQ[CurQNum][ulSQTail].CDW11 = pNVMeCmd->CDW11;
    IO_SQ[CurQNum][ulSQTail].CDW12 = pNVMeCmd->CDW12 ;
    IO_SQ[CurQNum][ulSQTail].CDW13 = 0;
    IO_SQ[CurQNum][ulSQTail].CDW14 = 0;
    IO_SQ[CurQNum][ulSQTail].CDW15 = pNVMeCmd->CDW15;

    ulSQTail = (ulSQTail + 1) % IO_Q_DEPTH;
    IO_SQTail[CurQNum] = ulSQTail;

    //update SQ Tail doorbell
    g_pCtrlReg->IO_DB[CurQNum][0].QHT = IO_SQTail[CurQNum];

    return;
}

BOOL NVMe_HostSendIOCmd(HSCMD_INFO* pHSCmd)
{
    U32 ulPRPCnt = 0;
    U8 *pDataBuffer = NULL;
    U32 ulCID;
    U32 ulPRP1XferBytes = 0;
    U32 ulPRP2XferBytes = 0;
    static U32 ulCmdCnt = 0;
    NVMe_COMMAND *pNVMeCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;

    U32 ulStartLba = pNVMeCmd->CDW10 << SEC_PER_DATABLOCK_BITS;
    U32 ulSecCnt = (pNVMeCmd->CDW12 + 1) << SEC_PER_DATABLOCK_BITS;

    //check Admin create SQ/CQ cmd done or not
    if(FALSE == NVMe_IsAdminCmdFinish())
    {
        return FALSE;
    }

    //2.find available CID for current SQ
    ulCID = GetAvailableCID(CurQNum);
    if(INVALID_8F == ulCID)
    {
        return FALSE;
    }

    //check SQ is full or not
    if(IO_SQHead[CurQNum] == ((g_pCtrlReg->IO_DB[CurQNum][0].QHT + 1) % IO_Q_DEPTH))
    {
        return FALSE;
    }

    /* avoid LBA overlap with processing commands */
    if (TRUE == NVME_IsHitPendingCmd(ulStartLba, ulSecCnt))
    {
        return FALSE;
    }

    pDataBuffer = (U8*)(g_pHostDataBuffer + (CurQNum * IO_Q_DEPTH * DATA_BUFFER_PER_CMD) + (ulCID * DATA_BUFFER_PER_CMD));

    //change Host Buffer not 4K align
    if(ulSecCnt < (DATA_BUFFER_PER_CMD - HPAGE_SIZE)/SEC_SIZE)
    {
        //pDataBuffer += (sizeof(QWORD) * rand()) % HPAGE_SIZE;

        if(1 == ulCmdCnt % 3)
        {
            pDataBuffer += (sizeof(QWORD) * rand()) % HPAGE_SIZE;
        }
        else if(2 == ulCmdCnt % 3)
        {
            pDataBuffer += (SEC_SIZE * rand()) % HPAGE_SIZE;
        }
        ulPRPCnt = CalcPRPCnt(pDataBuffer,ulSecCnt);
    }
    else
    {
        ulPRPCnt = (ulSecCnt + SEC_PER_HPAGE - 1)/SEC_PER_HPAGE;
    }

    if(NVM_WRITE == pNVMeCmd->CDW0.OPC)
    {
        //if Call NVME_HostPrepareData here, writecnt will be updated,
        //so don't send LBA overlapped cmd
        NVME_HostPrepareData(ulStartLba,ulSecCnt,(U32*)pDataBuffer);
    }

    pNVMeCmd->CDW0.CID = ulCID;
    pNVMeCmd->NSID = 1;
    NVMe_FillNVMeCmd(pNVMeCmd,pDataBuffer,ulPRPCnt);

    NextSubmissionQueue();
    ulCmdCnt++;

    return TRUE;
}

BOOL NVMe_HostSendIOCmdCustBuf(HSCMD_INFO* pHSCmd)
{
    U32 ulPRPCnt = 0;
    U8 *pDataBuffer = NULL;
    U32 ulCID;
    U32 ulPRP1XferBytes = 0;
    U32 ulPRP2XferBytes = 0;
    static U32 ulCmdCnt = 0;
    NVMe_COMMAND *pNVMeCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;

    U32 ulStartLba = pNVMeCmd->CDW10 << SEC_PER_DATABLOCK_BITS;
    U32 ulSecCnt = (pNVMeCmd->CDW12 + 1) << SEC_PER_DATABLOCK_BITS;

    //check Admin create SQ/CQ cmd done or not
    if (FALSE == NVMe_IsAdminCmdFinish())
    {
        return FALSE;
    }

    //2.find available CID for current SQ
    ulCID = GetAvailableCID(CurQNum);
    if (INVALID_8F == ulCID)
    {
        return FALSE;
    }

    //check SQ is full or not
    if (IO_SQHead[CurQNum] == ((g_pCtrlReg->IO_DB[CurQNum][0].QHT + 1) % IO_Q_DEPTH))
    {
        return FALSE;
    }

    /* avoid LBA overlap with processing commands */
    if (TRUE == NVME_IsHitPendingCmd(ulStartLba, ulSecCnt))
    {
        return FALSE;
    }

    //check length of databuffer >= length of request data
    if (pHSCmd->DataSecCnt < ulSecCnt)
    {
        DBG_Printf("The size of data buffer of HostCmd is smaller than the size of data of cmd send or request\n");
        return FALSE;
    }
    pDataBuffer = pHSCmd->pDataBuffer;

    //change Host Buffer not 4K align
    if (ulSecCnt < (DATA_BUFFER_PER_CMD - HPAGE_SIZE) / SEC_SIZE)
    {
        //pDataBuffer += (sizeof(QWORD) * rand()) % HPAGE_SIZE;

        if (1 == ulCmdCnt % 3)
        {
            pDataBuffer += (sizeof(QWORD) * rand()) % HPAGE_SIZE;
        }
        else if (2 == ulCmdCnt % 3)
        {
            pDataBuffer += (SEC_SIZE * rand()) % HPAGE_SIZE;
        }
        ulPRPCnt = CalcPRPCnt(pDataBuffer, ulSecCnt);
    }
    else
    {
        ulPRPCnt = (ulSecCnt + SEC_PER_HPAGE - 1) / SEC_PER_HPAGE;
    }

    if (NVM_WRITE == pNVMeCmd->CDW0.OPC)
    {
        //if Call NVME_HostPrepareData here, writecnt will be updated,
        //so don't send LBA overlapped cmd
        //The nfc model will check the data whether the data is expect one or not.Use carefully when using custom buffer for WriteCMD.
        NVME_HostPrepareData(ulStartLba, ulSecCnt, (U32*)pDataBuffer);
    }

    pNVMeCmd->CDW0.CID = ulCID;
    NVMe_FillNVMeCmd(pNVMeCmd, pDataBuffer, ulPRPCnt);

    NextSubmissionQueue();
    ulCmdCnt++;

    return TRUE;
}

U32 CalcPRPCnt(U8* pDataBuffer,U32 ulSecCnt)
{
    U32 ulPRPCnt = 0;
    U32 ulPRP1XferBytes = 0;
    U32 ulPRP2XferBytes = 0;

    ulPRP1XferBytes = HPAGE_SIZE - ((U32)pDataBuffer & HPAGE_SIZE_MSK);
    ulPRP2XferBytes = (ulPRP1XferBytes > (ulSecCnt*SEC_SIZE)) ? (0): ((ulSecCnt*SEC_SIZE) - ulPRP1XferBytes);
    ulPRPCnt = (ulPRP1XferBytes + HPAGE_SIZE - 1)/HPAGE_SIZE + (ulPRP2XferBytes + HPAGE_SIZE - 1)/HPAGE_SIZE;

    return ulPRPCnt;
}

BOOL NVMe_HostSendHSCmd(HSCMD_INFO* pHSCmd)
{
    U32 ulPRPCnt = 0;
    U8 *pDataBuffer = NULL;
    U32 ulSecCnt;
    U8 ucRangeNum;
    U32 ulCID;
    BOOL bFind = FALSE;
    U32 ulPRP1XferBytes = 0;
    U32 ulPRP2XferBytes = 0;
    NVMe_COMMAND *pNVMeCmd = (NVMe_COMMAND*)&pHSCmd->RowCmd;

    if (FALSE == Host_IsCMDEmptyInterface())
    {
        return FALSE;
    }

    //2.find available CID for current SQ
    ulCID = GetAvailableCID(CurQNum);
    if(INVALID_8F == ulCID)
    {
        return FALSE;
    }

    if(NVM_DATASET_MANAGEMENT == pNVMeCmd->CDW0.OPC)
    {
        pNVMeCmd->NSID = 1;
        ucRangeNum = pNVMeCmd->CDW10 & 0xFF;
        ulSecCnt = ((ucRangeNum + 1)*sizeof(LBA_RANGE_ENTRY)) >> SEC_SIZE_BITS;

        pDataBuffer = (U8*)(g_pHostDataBuffer + (CurQNum * IO_Q_DEPTH * DATA_BUFFER_PER_CMD) + (ulCID * DATA_BUFFER_PER_CMD));
        NVME_HostResetTrimData((TRIM_CMD_ENTRY *)(pHSCmd->pDataBuffer),ulSecCnt);

        memcpy(pDataBuffer,pHSCmd->pDataBuffer,(ucRangeNum + 1)*sizeof(LBA_RANGE_ENTRY));

        /*Dataset management max range is 256,4096 bytes*/
        ulPRPCnt = 1;
    }
    else if(NVM_VIA_TRACE_GETLOGINFO == pNVMeCmd->CDW0.OPC || NVM_VIA_TRACE_GETLOGDATA == pNVMeCmd->CDW0.OPC)
    {
        pDataBuffer = pHSCmd->pDataBuffer;

        ulSecCnt = pNVMeCmd->CDW12 + 1;

        ulPRPCnt = CalcPRPCnt(pDataBuffer,ulSecCnt);
    }

    pNVMeCmd->CDW0.CID = ulCID;
    NVMe_FillNVMeCmd(pNVMeCmd,pDataBuffer,ulPRPCnt);

    NextSubmissionQueue();

    return TRUE;
}
extern BOOL IsRealPowerDown();
BOOL Host_SendOneCmdInterface(HCMD_INFO* pHcmd)
{
    U32 CmdCode = 0;
    BOOL bRtn = FALSE;
    NVMe_COMMAND *pNVMeCmd = (NVMe_COMMAND*)&pHcmd->HSCmd.RowCmd;

    NVMe_HCmdToRowCmd(pHcmd);
    CmdCode = pNVMeCmd->CDW0.OPC;

    switch(pHcmd->CmdType)
    {
    case CMD_TYPE_READ:
    case CMD_TYPE_WRITE:
        bRtn = NVMe_HostSendIOCmd(&pHcmd->HSCmd);
        break;

    case CMD_TYPE_READ_IO:
    case CMD_TYPE_WRITE_IO:
        bRtn = NVMe_HostSendIOCmdCustBuf(&pHcmd->HSCmd);
        break;

    case CMD_TYPE_TRIM:
        bRtn = NVMe_HostSendHSCmd(&pHcmd->HSCmd);
        break;

    case CMD_TYPE_STANDBY:
        if (FALSE == Host_IsCMDEmptyInterface())
        {
            return bRtn;
        }
        if (TRUE == IsRealPowerDown())
        {
            NVMe_DeleteIOQ();
            //wait all admin cmd complete
            while(FALSE == NVMe_IsAdminCmdFinish())
            {
                Sleep(5);
            }
        }
        NVMe_SetShutdownReg(0x1);
        NVMe_WaitShutdownFinish();
        NVMe_SetShutdownReg(0);
        bRtn = TRUE;
        break;

    case CMD_TYPE_HSCMD:
        if(PROT_NVME == pHcmd->HSCmd.Protocal)
        {
            bRtn = NVMe_HostSendHSCmd(&pHcmd->HSCmd);
        }
        else if(TRUE == NVMe_IsAdminCmd(CmdCode))
        {
            bRtn = NVMe_HostSendAdminCmd(&pHcmd->HSCmd);
        }
        break;

    default:
        DBG_Printf("HCmd CmdType error\n");
        ASSERT(FAIL);
        break;
    }

    return bRtn;
}

BOOL NVMeInterrupt()
{
    U32 i = 0;
    U32 ulCQHead;
    U32 ulCID;

    g_pCtrlReg->IVMS = 0xFFFFFFFF;

    for(i = 0; i < IO_QCNT; i++)
    {
        ulCQHead = IO_DriverHead[i];

        while(IO_CQ[i][ulCQHead].DW3.SF.P != IO_DriverP[i])
        {
            ulCID = IO_CQ[i][ulCQHead].DW3.CID;

            if(0 == g_tCIDState[i][ulCID].ucStatus)
            {
                DBG_Printf("CurQNum %d CID %d\n",i,ulCID);
                DBG_Getch();
            }

            //1.update SQhead
            IO_SQHead[i] = IO_CQ[i][ulCQHead].DW2.SQHD;

            //roll the head of the queue
            if(ulCQHead == (IO_Q_DEPTH - 1))
            {
                ulCQHead = 0;
                IO_DriverP[i] ^= 1; //revert P bit
            }
            else
            {
                ulCQHead++;
            }
            IO_DriverHead[i] = ulCQHead;

            //2.update CQ head doorbell register
            g_pCtrlReg->IO_DB[i][1].QHT = IO_DriverHead[i];

            //Clear CID State
            ClearCIDStatus(i,ulCID);

        }
    }
    g_pCtrlReg->INTMC = 0xFFFFFFFF;

    return TRUE;
}

LOCAL volatile BOOL l_bMSIThreadExit = FALSE;

DWORD WINAPI SIM_ThreadMSI(LPVOID p)
{
    while (FALSE == l_bMSIThreadExit)
    {
        WaitForSingleObject(hEventMSI,INFINITE);
        NVMeInterrupt();/* interrupt handler, host routine*/
    }
    return 0;
}

void MSI_ThreadInit(void)
{
    l_bMSIThreadExit = FALSE;

    hEventMSI = CreateSemaphore( NULL, 0, 256, NULL );

    hThreadMSI = CreateThread(0, 0, SIM_ThreadMSI, 0, 0, 0);
}

void MSI_ThreadExit()
{
    l_bMSIThreadExit = TRUE;
    ReleaseSemaphore( hEventMSI, 1, NULL );

    WaitForSingleObject(hThreadMSI, INFINITE);
    CloseHandle(hThreadMSI);
}

void SIM_MSITrigger(DWORD vector)
{
    ReleaseSemaphore( hEventMSI, 1, NULL );
}
