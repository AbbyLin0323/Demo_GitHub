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

#ifndef _BASE_PATTERN_H
#define _BASE_PATTERN_H

#define MAX_BASE_FUNCTION   6

#define MAX_SECTOR_CNT 0xffff


typedef struct _FUNCTION_PARAMETER
{
    U32 ulStartLba;
    U32 ulSecCnt;
    U32 ulEndLba;
    U32 ulSecRange;
}FUNCTION_PARAMETER, *pFUNCTION_PARAMETER;

typedef enum _FILE_TRIM_TYPE
{
    FILE_TRIM_TYPE_2_SECTOR = 0,
    FILE_TRIM_TYPE_ADD_LBA_ENTRY_SUCCESS,
    FILE_TRIM_TYPE_ADD_USELESS_ENTRY_SUCCESS
}FILE_TRIM_TYPE;

typedef struct _FILE_TRIM_RETURN
{
    FILE_TRIM_TYPE ulFileTrimType;
    U32 ulTrimSecCnt;
}FILE_TRIM_RETURN, *pFILE_TRIM_RETURN;

typedef BOOL (*PBF)(pFUNCTION_PARAMETER pFuncParameter);

typedef struct _BASE_FUNCTION
{
    PBF pFunc;
    FUNCTION_PARAMETER FuncParameter;


}BASE_FUNCTION, *pBASE_FUNCTION;
#ifdef HOST_NVME
    typedef struct _LBA_RANGE_ENTRY
    {
        U32 ContextAttr;
        U32 RangeLength;

        U32 StartLbaLow;
        U32 StartLbaHigh;
    }LBA_RANGE_ENTRY;
    #define TOTAL_RANGE_PER_SEC         (32)
    #define TRIM_LBA_RANGE_ENTRY_MAX    (TOTAL_RANGE_PER_SEC)
    #define TRIM_CMD_SEC_CNT_MAX        (8) //??
#else
    typedef struct _LBA_RANGE_ENTRY
    {
        U32 StartLbaLow;
        U16 StartLbaHigh;
        U16 RangeLength;
    }LBA_RANGE_ENTRY;
    #define TRIM_LBA_RANGE_ENTRY_MAX (64)
    #define TRIM_CMD_SEC_CNT_MAX     (64)

#endif

typedef struct TRIM_CMD_ENTRY_TAG
{
    LBA_RANGE_ENTRY LbaRangeEntry[TRIM_LBA_RANGE_ENTRY_MAX];
}TRIM_CMD_ENTRY;



#define TRIM_ENTRY_FULL(Head,NextTail)  ((NextTail == Head)? 1:0)
#define TRIM_ENTRY_EMPTY(Head,Tail)     ((Tail == Head)? 1:0)

//DWORD WINAPI Host_ModelThread(LPVOID p);
TRIM_CMD_ENTRY g_TrimCmdEntry[TRIM_CMD_SEC_CNT_MAX];

extern U32 g_TrimCmdEntryHead;
extern U32 g_TrimCmdEntryTail;

extern U32 g_TransTotalTime;
extern U32 g_TransSecCnt;

extern pBASE_FUNCTION GetBaseFunction(U16 uBPIndex);
extern void ini_base_fuction();
U32 GetSystemMaxLBA();
void SetSystemMaxLBA(U32 ulMaxLba);
#endif