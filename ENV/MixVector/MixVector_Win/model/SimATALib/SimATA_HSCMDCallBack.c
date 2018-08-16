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
#include "Disk_Config.h"
#include "SimATA_HSCMDCallBack.h"
#include "hscmd_parse.h"
#include "action_define.h"
#include "model_config.h"
#include "win_bootloader.h"


PRFisHandler ParseRFisCallBack;

extern void SetSystemMaxLBA(U32 ulMaxLba);

typedef struct _HSCMD_CALLBACK_FUNC_DESC {
    char* HSCMD_Desc;
    PHSCmdCallBack pFunc;
}HSCMD_CALLBACK_FUNC_DESC;

static HSCMD_CALLBACK_FUNC_DESC HSCMD_CALLBACK_FUNC_ARRAY[] = 
{
    {"IdentifyDevice" ,ATA_IdentifyCallBack},
    {"Smart_ReadLog"  ,ATA_SmartReadLogCallBack},
    {"Smart_WriteLog" ,ATA_SmartWriteLogCallBack},
    {"Smart_ReadData" ,ATA_SmartReadDataCallBack},
    {"ReadLogExt"     ,ATA_ReadLogExtCallBack},
    {"VIA_GetTraceLogInfo",ATA_GetTLInfoCallBack},
    {"VIA_GetTraceLogData",ATA_GetTLDataCallBack},
    {"VIA_GetVarTableAddr", ATA_GetVarTableCallBack},
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

BOOL ATA_IdentifyCallBack(U8 *pCMDData)
{
    
    U8 CalcCheckSum = 0;
    U32 ulMaxLba = 0;
    PIDENTIFY_DEVICE_DATA pIdentifyData = (PIDENTIFY_DEVICE_DATA)pCMDData;

    Parse_IdentifyData((U16*)pCMDData);
    ulMaxLba = *(U32*)pIdentifyData->Max48BitLBA;

    if (MAX_LBA_IN_SYSTEM != ulMaxLba)
    {
        DBG_Printf("%s MaxLab is error. MaxLbaInBoot = 0x%x, MaxLbaInIdentify = 0x%x", __FUNCTION__, MAX_LBA_IN_SYSTEM, ulMaxLba);
        DBG_Getch();
    }

    SetSystemMaxLBA(ulMaxLba);

    DBG_Printf("%s done\n",__FUNCTION__);

    return TRUE;
}

BOOL ATA_SmartReadLogCallBack(U8 *pCMDData)
{
    //check write log data with read log data

    DBG_Printf("%s done\n",__FUNCTION__);

    return TRUE;
}

BOOL ATA_SmartWriteLogCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);

    return TRUE;
}

BOOL ATA_SmartReadDataCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);

    //Calc host read/write sectors,etc

    return TRUE;
}


BOOL ATA_ReadLogExtCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);
    return TRUE;
}


BOOL ATA_GetTLDataCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);
    return TRUE;
}

BOOL ATA_GetTLInfoCallBack(U8 *pCMDData)
{
    DBG_Printf("%s done\n",__FUNCTION__);
    return TRUE;
}

BOOL ATA_GetVarTableCallBack(U8 *pCMDData)
{
    U32 aFeedback[2] = {0};
    
    ParseRFisCallBack(0,&aFeedback[0]);

    g_ulDevVarTableAddr = aFeedback[0];

    DBG_Printf("g_ulDevVarTableAddr 0x%x\n",g_ulDevVarTableAddr);
    return TRUE;
}