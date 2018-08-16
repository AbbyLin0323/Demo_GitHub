/****************************************************************************
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
*****************************************************************************
Filename     :  DosBaseType.h                                     
Version      :  Ver 1.0                                               
Date         :  20140611                                     
Author       :  Gavin

Description: 
    implement ADD/SUB/CMP operation for U64 variable

Modification History:
20140611     gavinyin     001 create file
****************************************************************************/
#ifndef _TYPES_H_
#define _TYPES_H_

#ifndef  NULL
#define  NULL ((void *)0)
#endif

#define  FALSE			0
#define  TRUE                   1


//#define SUCCESS                 0
//#define UNSUCCESSFUL            (0xFFFFFFFF)
#define INVALID_8F 0xFFFFFFFF

typedef void                    VOID;
typedef void*                   PVOID;

#if 0
#ifndef BOOL
typedef unsigned char		BOOL;
#endif
#endif
typedef unsigned char		BYTE;
typedef unsigned char		UCHAR;
typedef char          		CHAR;

typedef unsigned char*          PBYTE;
typedef unsigned char*          PUCHAR;
typedef char*                   PCHAR;

typedef unsigned short  	USHORT;
typedef unsigned short		WORD;
typedef unsigned short* 	PUSHORT;
typedef unsigned short*         PWORD;

typedef unsigned int		UINT;
typedef unsigned int		ULONG;
//typedef unsigned long long		ULONG;
typedef unsigned int		DWORD;
typedef unsigned int *		PUINT;
typedef unsigned int *		PULONG;
typedef unsigned int * 		PDWORD;
typedef enum _cmd_status
{
	CMD_IDLE,
	CMD_BUSY,
	CMD_WAIT,
	CMD_FINISH
}CMD_STATUS;
typedef enum _status{
    SUCCESS,
    UNSUCCESSFUL,
    INPUT_PARAMETER_ERROR,
    DATA_SIZE_TOO_LONG,
    SSD_PAGE_OVERWRITE,
    SSD_PAGE_NOT_WRITTEN,
    LOOP_COMPLETED
}STATUS;
//typedef unsigned int            STATUS;
typedef unsigned long long	U64;
typedef unsigned long long	ULONGLONG;
typedef unsigned long long*     PULONGLONG;

typedef unsigned int        U32;

#endif
