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
  File Name     : hscmd_callback.c
  Version       : Initial Draft
  Author        : Betty Wu
  Created       : 2014/9/15
  Last Modified :
  Description   :
  Function List :
  History       :
  1.Date        : 2014/9/15
    Author      : Betty Wu
    Modification: Created file

******************************************************************************/
#include "model_common.h"
#include "BaseDef.h"
#include "hscmd_parse.h"
#include "action_define.h"
#include "NVME_HSCMDCallBack.h"
#include "nvmeStd.h"



extern void SetSystemMaxLBA(U32 ulMaxLba);

typedef struct _HSCMD_CALLBACK_FUNC_DESC {
    char* HSCMD_Desc;
    PHSCmdCallBack pFunc;
}HSCMD_CALLBACK_FUNC_DESC;

static HSCMD_CALLBACK_FUNC_DESC HSCMD_CALLBACK_FUNC_ARRAY[] =
{
    {"IdentifyDevice" ,NVME_IdentifyCallBack},
    {"Smart_ReadLog"  ,NVME_SmartReadLogCallBack},
    {"Smart_WriteLog" ,NVME_SmartWriteLogCallBack},
    {"Smart_ReadData" ,NVME_SmartReadDataCallBack},
    {"ReadLogExt"     ,NVME_ReadLogExtCallBack},
    {"VIA_GetTraceLogInfo",NVME_GetTLInfoCallBack},
    {"VIA_GetTraceLogData",NVME_GetTLDataCallBack},
    {"VIA_GetVarTableAddr", NVME_GetVarTableCallBack},
    {"GegErrorLogPage",NVME_GetErrorLogPage},
    {"GetSMARTLogPage",NVME_GetSMARTLogPage},
    {"GetSlotInfoLogPage",NVME_GetSlotInfoLogPage},
    {"END",NULL},
};

extern void Parse_IdentifyData(U16* pBuffer);

PHSCmdCallBack* get_hscmd_callback(char* HSCMD_Desc)
{
    HSCMD_CALLBACK_FUNC_DESC *pHScmdCallBackFuncDesc = &HSCMD_CALLBACK_FUNC_ARRAY[0];
    U8 uIndex = 0;

    while(strcmp(pHScmdCallBackFuncDesc->HSCMD_Desc,"END"))
    {
        if(!strcmp(pHScmdCallBackFuncDesc->HSCMD_Desc,HSCMD_Desc))
        {
            return &pHScmdCallBackFuncDesc->pFunc;
        }
        pHScmdCallBackFuncDesc++;
    }
    return NULL;
}

BOOL NVME_IdentifyCallBack(U8 *pCMDData)
{
    U8 CalcCheckSum = 0;
    U32 ulMaxLba = 0;
    //PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)pCMDData;

    Parse_IdentifyData((U16*)pCMDData);
    //ulMaxLba = (U32)pIdentifyData->Max48BitLBA;
    //SetSystemMaxLBA(ulMaxLba);

    DBG_Printf("%s done\n",__FUNCTION__);

    return TRUE;
}

BOOL NVME_SmartReadLogCallBack(U8 *pCMDData)
{
    //check write log data with read log data

    DBG_Printf("%s done\n",__FUNCTION__);

    return TRUE;
}

BOOL NVME_SmartWriteLogCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);

    return TRUE;
}

BOOL NVME_SmartReadDataCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);

    //Calc host read/write sectors,etc

    return TRUE;
}


BOOL NVME_ReadLogExtCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);
    return TRUE;
}


BOOL NVME_GetTLDataCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);
    return TRUE;
}

BOOL NVME_GetTLInfoCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);
    return TRUE;
}

extern NVMe_COMPLETION_QUEUE_ENTRY Admin_CQ[ADMIN_Q_DEPTH];

BOOL NVME_GetVarTableCallBack(U8 *pCMDData)
{
    g_ulDevVarTableAddr = Admin_CQ[Admin_CQHead - 1].DW0;

    DBG_Printf("g_ulDevVarTableAddr 0x%x\n",g_ulDevVarTableAddr);
    return TRUE;
}

BOOL NVME_GetErrorLogPage(U8 *pCMDData)
{
    PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY pErr = (PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY)pCMDData;
    DBG_Printf("Error Count: 0x%x\t",pErr->ErrorCount);
    DBG_Printf("Submission Queue ID: 0x%x\n",pErr->SubmissionQueueID);
    DBG_Printf("Command ID: 0x%x\t",pErr->CommandID);
    DBG_Printf("Status Field: 0x%x\n",pErr->StatusField);
    DBG_Printf("LBA: 0x%x\n",pErr->LBA);
    return TRUE;
}

BOOL NVME_GetSMARTLogPage(U8 *pCMDData)
{
    PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY pSmart = (PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY)pCMDData;
    DBG_Printf("Data Units Read: 0x%08x%08x\t",pSmart->DataUnitsRead.Upper,pSmart->DataUnitsRead.Lower);
    DBG_Printf("Data Units Written: 0x%08x%08x\n",pSmart->DataUnitsWritten.Upper,pSmart->DataUnitsWritten.Lower);
    DBG_Printf("Host Read Commands: 0x%08x%08x\t",pSmart->HostReadCommands.Upper,pSmart->HostReadCommands.Lower);
    DBG_Printf("Host Write Commands: 0x%08x%08x\n",pSmart->HostWriteCommands.Upper,pSmart->HostWriteCommands.Lower);
    DBG_Printf("Power On Hours: 0x%08x%08x\n", pSmart->PowerOnHours.Upper, pSmart->PowerOnHours.Lower);
    DBG_Printf("Number of Error Information Log Entries: 0x%08x%08x\n", pSmart->NumberofErrorInformationLogEntries.Upper, pSmart->NumberofErrorInformationLogEntries.Lower);
    return TRUE;
}

BOOL NVME_GetSlotInfoLogPage(U8 *pCMDData)
{
    return TRUE;
}