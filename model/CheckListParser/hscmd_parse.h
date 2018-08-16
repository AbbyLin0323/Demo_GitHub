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
  File Name     : hscmd_parse.h
  Version       : Initial Draft
  Author        : Nina Yang
  Created       : 2014/4/13
  Last Modified :
  Description   :
  Function List :
  History       :
  1.Date        : 2014/4/13
    Author      : Nina Yang
    Modification: Created file

******************************************************************************/
#ifndef _HSCMD_PARSE_H
#define _HSCMD_PARSE_H

#include <Windows.h>
// include FW
#include "BaseDef.h"

typedef BOOL (*PHSCmdCallBack)(U8 *pCMDData);
typedef struct _HSCMD_INFO
{
    char HSCmdName[32];
    char CmdDsc[64];

    U16 DataSecCnt;
    U8 CmdId;
    U8 Protocal;
    U16 RowCmdLen;
    U8  RowCmd[64];
    U8 bDataValid;
    U8* pDataBuffer;
    PHSCmdCallBack* pHSCmdCallBackFunc;
    struct _HSCMD_INFO* pHSCmdNext;
}HSCMD_INFO;

/*
Since CMD_TYPE_READ/WRITE use its internal memory in HostNVMe level that upper 
host model cannot access. Adding these two CMD_TYPE_READ/Write_IO type 
will enable the upper host model can set its custom data for data write or read
*/
typedef enum _CMD_TYPE
{
    CMD_TYPE_READ = 0,
    CMD_TYPE_WRITE,
    CMD_TYPE_TRIM,
    CMD_TYPE_STANDBY,
    CMD_TYPE_READ_IO, 
    CMD_TYPE_WRITE_IO,
    CMD_TYPE_HSCMD,
}CMD_TYPE;



typedef struct _HSCMD_LIST
{
    U32 HSCmdCnt;
    HSCMD_INFO* pHSCmdHead;
    HSCMD_INFO* pHSCmdTail;
}HSCMD_LIST;


/*HSCMD_INFO:Host SpecialCmd,include Indentify,Smart,Vender define,etc*/
typedef struct _HCMD_INFO
{
    CMD_TYPE CmdType;
    U32 StartLba;
    U32 SecCnt;
    union {
        U8 McuId;
        U8 RangeNum;
    };

    HSCMD_INFO HSCmd;
}HCMD_INFO;

#endif