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
Filename     : HAL_TraceLog.h
Version      : 
Date         : 
Author       : Gavin
Others: 
Modification History:
*******************************************************************************/
#ifndef _TRACE_LOG_H_
#define _TRACE_LOG_H_

#ifndef ASSERT
#ifdef _WIN32
#include <assert.h>
#define ASSERT( _x_ ) assert( _x_ )
#else
#define ASSERT( _x_ )
#endif
#endif // ifndef ASSERT

#define REST_SPACE_IN_SEC(_x_, _sec_) ( _sec_ - ( _x_ & ( _sec_ - 1 ) ) )
#define ROUND_TO_NEXT_SEC(_x_, _sec_) ( ( _x_ + _sec_ - 1 ) & ( ~( _sec_ -1 ) ) )
#define ALIGN_TO_CUR_SEC(_x_, _sec_)  ( _x_ & (~( _sec_ - 1 ) ) )

#define TRACE_LOG( _PTR_, _SIZE_, _STRUCT_, ...)\
    TL_TraceLog( TL_FILE_NUM, __LINE__, _PTR_, _SIZE_)

//start of name files
typedef enum{
    HAL_BufMap_c,
    HAL_HSG_c,
    HAL_Init_c,
    HAL_NormalDSG_c,
    HAL_PM_c,
    HAL_SataDSG_c,
    HAL_SearchEngine_c,
    HAL_SGE_c,
    L0_Ahci_c,
    L0_AhciHCT_c,
    L0_AhciList_c,
    L0_AhciTrace_c,
    L0_ATAGenericCmdLib_c,
    L0_ATALibAdapter_c,
    L0_SataIsr_c,
    L0_SataErrorHandling_c,
    L0_MainTasks_c,
    L0_Interface_c,
    L1_AhciIO_c,
    L1_Buffer_c,
    L1_Cache_c,
    L1_HostCmdPRO_c,
    L1_Interface_c,
    L1_SataSmart_c,
    L1_Schedule_c,
    L1_SubCmdProc_c,
    L1_Debug_c,
    L2_Boot_c,
    L2_Debug_c,
    L2_ErrorHandling_c,
    L2_Evaluater_c,
    L2_FTL_c,
    L2_GCManager_c,
    L2_Idle_c,
    L2_Init_c,
    L2_LLF_c,
    L2_PhysicalAddr_c,
    L2_PhysicalPage_c,
    L2_PMTI_c,
    L2_PMTManager_c,
    L2_PMTPage_c,
    L2_RPMT_c,
    L2_Schedule_c,
    L2_Shutdown_c,
    L2_StaticWL_c,
    L2_StripeInfo_c,
    L2_TblChk_c,
    L2_Thread_c,
    L2_Trim_c,
    L2_VirtualStorage_c,
    L3_BBT_c,
    L3_BlockManagement_c,
    L3_Debug_c,
    L3_ErrorHandling_c,
    L3_FullRecovery_c,
    L3_GB_c,
    L3_Interface_c,
    L3_LB_c,
    L3_LLF_c,
    L3_SataSpecialCmd_c,
    L3_Schedule_c,
    L3_TableBlock_c,
    L3_Ahci_c,
    L3_NVMe_c,
    L3_Sata_c,
}C_FILE_NAME_ENUM;

typedef struct _tl_log_header{
    U32 bsLineNum: 24;  // line number in C source file. if equal to 0, means this log is invalid
    U32 bsFileNum: 8;   // C source file number: every C source file in project has a dedicated number.
}TL_LOG_HEADER;

typedef struct _tl_sec_header{
    U16 usLogNum;    // the trace log number in the section.
    U16 usVersion;   // the version of the trace log format.
}TL_SEC_HEADER;

#define TL_VERSION 0x351400
//descriptor for Trace Log information. we allocate descriptor for every MCU
typedef struct _TL_INFO{
    U32 ulTlVersion;  //Trace Log version number
    U32 ulTlMemBase;  //Trace Log memory base in MCU view
    U32 ulTlMemSize;  //Trace Log memory size in byte unit
    U32 ulTlSecSize;  //size of every Trace Log sector, eg. 512Byte

    U32 ulValidSecCnt;//number of sector which are filled with log data.
                      //no matter the sector is full or not
    U32 ulValidFlushCnt;//number of sector which are filled with not flushed log data 

    U32 ulWriteSec;   //index of sector which will be written with log data
    U32 ulReadSec;    //index of sector from which host can get valid log data
    U32 ulFlushSec;   //index of sector has flush into flash(Trace block)
}TL_INFO;

/*
Each sector consists of one Sec Header and some pairs of Log Header and Log, as shown below.

+----------+------------+----------+------------+----------+ +----------+-----------+----------+
|Sec Header| Log Header | Log ...  | Log Header | Log ...  | |Sec Header| Log Header| Log ...  | ...
+----------+------------+----------+------------+----------+ +----------+-----------+----------+


Sec Header: 4 bytes, refer to TL_SEC_HEADER.
Log Header: 4 bytes,
            +----------+----------+
            | Line Num | File Num |
            +----------+----------+
            | 3 bytes  | 1 byte   |

Log       : variable length.

Log consumer should read multiple sectors one time.
*/

/*
Initialization function for the TraceLog (TL) module.
Input parameters:
 ucMcuId  : the caller's MCU id
 pBuffer  : pointed to a buffer for trace logs.
 ulSize   : the buffer size, in units of byte.
 UlSecSize: the size of per Sector.
*/
void TL_Initialize(U8 ucMcuId, void* pBuffer, U32 ulSize, U32 ulSecSize);

/*
Write a trace log.

If trace log buffer is full, the oldest trace log will be overwritten.

Input parameters:

 ucFileNum: the file number of the calling statement.
 ulLineNum: the line number of the calling statement.
 pData    : point to the data that will be written into the trace log buffer.
 ulSize   : the size of the data specified by "pData", in units of byte.

Note:
 The "ulSize" should be less than ( SectorSize - sizeof( TL_HEADER ) ). The overhead part will be discarded.

*/
void TL_TraceLog(U8 ucFileNum, U32 ulLineNum, void* pData, U32 ulSize);

/*
Invalidate trace memory which will be loaded to host side.

Input Param:
    U32 ulSectorCnt: sector count of trace memory, starting from read pointer.

Usage:
    FW call this function when receive host request for reading trace memory
*/
void TL_InvalidateTraceMemory(U32 ulSectorCnt);

BOOL TL_IsFlushMemoryEmpty();
void TL_PrepareFlushData(U32 ulMcuId,U32* pFlushBuf,U8* pTempBuf,U32* pSecCnt,U32 ulSaveSize);
void TL_InvalidateFlushMemory(U32 ulSectorCnt);
TL_INFO* TL_GetTLInfo(U32 ulMcuId);
BOOL TL_IsTraceMemory(U32 ulAddr);
void TL_Enable(void);
void TL_Disable(void);
#endif
