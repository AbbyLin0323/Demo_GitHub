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
#ifndef _TRACELOG_H_
#define _TRACELOG_H_

#include <assert.h>
#include "BaseType.h"
#define DBG_ERROR              0
#define DBG_WARNING            1
#define DBG_NORMAL             2
#define DBG_TRACE              3
#define DBG_FUNCTION           3
#define COMP_ALL             0xFFFFFFFF

/*****************************************************************************
 *
 *  Debug Functions
 *
 ****************************************************************************/
extern ULONG GlobalLogLevel;
extern ULONG GlobalLogComponents;

STATUS LogOpen( char* LogFileName );
void LogClose();
void LogMsg( ULONG _Comp,ULONG  _Level, const char* fmt, ... );
void LogDumpData( ULONG _Comp, ULONG _Level, char* StrName, PUCHAR Data, ULONG Size );

#define FuncEntry \
{ \
        LogMsg( COMP_ALL, DBG_FUNCTION, "==> " __FUNCTION__ "\n" ); \
}
#define FuncExit \
{ \
        LogMsg( COMP_ALL, DBG_FUNCTION, "<== " __FUNCTION__ "\n" ); \
}

//#define ASSERT( _x_ )  do{ if ( 0 == _x_ ) __debug_break();}while(0);
#define ASSERT( _x_ ) assert( _x_ )

#endif // _TRACELOG_H_
