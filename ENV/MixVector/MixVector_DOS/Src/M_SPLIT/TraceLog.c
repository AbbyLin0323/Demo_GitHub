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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "DosBaseType.h"
//#include "list.h"
#include "TraceLog.h"
//#include "mixv_main.h"

/*****************************************************************************
 *
 *  Debug Functions
 *
 ****************************************************************************/
ULONG GlobalLogLevel = DBG_NORMAL;
ULONG GlobalLogComponents = COMP_ALL;
static FILE* FileLog = NULL;
static char sprint_buf[ 1024 ];
static ULONG MemCount = 0;

STATUS LogOpen( char* LogFileName )
{
    if (( LogFileName == NULL ) || ( strlen( LogFileName ) == 0 ) )
    {
        return UNSUCCESSFUL;
    }
    if( FileLog != NULL )
    {
        fclose( FileLog );
    }
    FileLog = fopen( LogFileName, "w+");
    if ( FileLog == NULL )
        return UNSUCCESSFUL;
    else
        return SUCCESS;
}
void LogClose()
{
    if ( FileLog != NULL )
    {
        LogMsg( COMP_ALL, DBG_NORMAL, "Mem Allocation Count %d\n", MemCount );
        fclose( FileLog );
        FileLog = NULL;
    }
}

static ULONG LogCount = 0;
#define PRINT( _str_ ) \
{ \
    printf( _str_ ); \
    if ( FileLog != NULL ) \
    { \
        LogCount++; \
        fprintf( FileLog, _str_ ); \
        if ( ( LogCount % 16 ) == 0 ) \
        { \
            fflush( FileLog ); \
        } \
    } \
}
void LogMsg( ULONG _Comp,ULONG  _Level, const char* fmt, ... )
{
    
    va_list args;
    int n;
    return;
    if ( ( _Comp & GlobalLogComponents ) && ( _Level <= GlobalLogLevel ) )
    {
        va_start( args, fmt );
        n = vsprintf( sprint_buf, fmt, args );
        va_end( args );
        switch ( _Level )
        {
        case DBG_ERROR:
            printf("[ERROR]: ");
            fprintf( FileLog, "[ERROR]: ");
            break;
        case DBG_WARNING:
            printf("[WARNG]: ");
            fprintf( FileLog, "[WARNG]: ");
            break;
        }
        PRINT( sprint_buf );
    }
}
void LogDumpData( ULONG _Comp, ULONG _Level, char* StrName, PUCHAR Data, ULONG Size )
{
    ULONG line, n, OmittedBytes;
    if ( ( _Comp & GlobalLogComponents ) && ( _Level <= GlobalLogLevel ) )
    {
        sprintf( sprint_buf, "\n------- Dump %s -------", StrName );
        PRINT( sprint_buf );
        if ( Size > 64 )
        {
            OmittedBytes = Size - 64;
            Size = 64;
        }else{
            OmittedBytes = 0;
        }
        for( line = 0; line < ( Size / 8 ); line++)
        {
            sprintf( sprint_buf, "\n%08X: %02X %02X %02X %02X %02X %02X %02X %02X", Data, Data[ 0 ], Data[ 1 ], Data[ 2 ], Data[ 3 ], Data[ 4 ], Data[ 5 ], Data[ 6 ], Data[ 7 ] );
            PRINT( sprint_buf );
            Data += 8;
        }
        for ( n = 0; n < ( Size % 8 ); n++ )
        {
            if ( n == 0 )
            {
                sprintf( sprint_buf, "\n%08X: %02X", Data, Data[ n ] );
            }else{
                sprintf( sprint_buf,  "%02X", Data[ n ] );
            }
            PRINT( sprint_buf );
        }
        if ( OmittedBytes > 0 )
        {
            sprintf( sprint_buf, "\n%08X: ... (Omit %d Bytes)\n", Data, OmittedBytes );
            PRINT( sprint_buf );
        }
        sprintf( sprint_buf, "\n\n");
        PRINT( sprint_buf );
    }
}
