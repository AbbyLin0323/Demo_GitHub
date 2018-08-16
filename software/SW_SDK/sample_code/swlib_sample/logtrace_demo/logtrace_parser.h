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
  File Name     : host_api_misc.h
  Version       : Release 0.0.1
  Author        : alpha
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines logtrace related macro and structure.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : alpha
    Modification: Created file

******************************************************************************/

#ifndef _LOGTRACE_PARSER
#define _LOGTRACE_PARSER

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include <windows.h>


typedef struct _tl_header{
    UINT16 usLogNum;    // the trace log number in the section.
    UINT16 usVersion;   // the version of the trace log format.
}TL_SEC_HEADER;

typedef struct _tl_log_header{
    UINT32 bsLineNum: 24;    // line number in C source file.
    UINT32 bsFileNum: 8;   // C source file number: every C source file in project has a dedicated number.
}TL_LOG_HEADER;

#define DATABLOCK_SIZE 512
#define FORMAT_FILE_LINE_SIZE	20*MAXBYTE

#define VALUE_ONE 0x1U

#define TYPE_LOW_BOOL               "bool"
#define TYPE_UP_BOOL                "BOOL"
#define TYPE_CHAR                   "char"
#define TYPE_UNSIGNED_CHAR          "unsigned char"
#define TYPE_SHORT                  "short"
#define TYPE_UNSIGNED_SHORT         "unsigned short"
#define TYPE_LONG                   "long"
#define TYPE_UNSIGNED_LONG          "unsigned long"
#define TYPE_INT                    "int"
#define TYPE_UNSIGNED_INT           "unsigned int"
#define TYPE_LONGLONG               "longlong"
#define TYPE_UNSIGNED_LONGLONG      "unsigned longlong"

#define TYPE_BYTE	"BYTE"
#define TYPE_UBYTE	"UBYTE"
#define TYPE_WORD	"WORD"
#define TYPE_UWORD	"UWORD"
#define TYPE_DWORD	"DWORD"
#define TYPE_UDWORD "UDWORD"
#define TYPE_QWORD	"QWORD"
#define TYPE_PTR	"ptr"

struct MEMBERINFORMATION
{
    char MemberType[MAXBYTE];
    char MemberName[MAXBYTE];
    UINT32 MemberSize;
    BOOL SameWord;
    UINT32 StartBit;
    void *pMemberValue;
    UINT32 ShouldbeSize;
    MEMBERINFORMATION *pNextMember;
};

struct TRACEINFORMATION
{
    UINT32 FileNum;
    UINT32 LineNum;
    UINT32 DataSize;
    char StructName[MAXBYTE];
    char StartField[MAXBYTE];
    char NoteInfo[MAXBYTE];
    MEMBERINFORMATION *pMember;
    TRACEINFORMATION *pNextTRACEINFORMATION;
};

#define SECTOR_SIZE 512
#define ONEFILEMAXLINENUM 500000

int Decoder(char *pFormatFile,char *pDataFileName,char *pTraceLogHeaderFile,char *pReportFileDir,char mcuid);

#endif