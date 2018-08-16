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
Filename     : MixVector_HostApp.h                                    
Version      : Ver 1.0                                               
Date         :                                         
Author       : 

Description: 
Windows pattern for MixVector verification in VT3514 PCIE SSD
Modification History:
20140718    Gavin   created
------------------------------------------------------------------------------*/
#ifndef __MIX_VECTOR_HOST_APP__
#define __MIX_VECTOR_HOST_APP__

#include "MixVector_Interface.h"
/****************************************************************************\
 *
 *                       from DOS application for mix vector test
 *
\****************************************************************************/
typedef enum _status{
    CMD_SUCCESS,
    CMD_UNSUCCESSFUL,
    INPUT_PARAMETER_ERROR,
    DATA_SIZE_TOO_LONG,
    SSD_PAGE_OVERWRITE,
    SSD_PAGE_NOT_WRITTEN,
    LOOP_COMPLETED
}STATUS;
typedef enum _cmd_state{
    CMD_IDLE,
    CMD_WRITING,
    CMD_READING,
    CMD_COMPLETED
}CMD_STATE;

typedef STATUS (*PCMD_ROUTINE)( struct _host_cmd_ext* CmdInfoFlash, struct _common_cmd_info* );

typedef struct _common_cmd_info{
    U8      DiskIndex;
    U8*     MemBASw;
    ULONG   Length;         // in units of byte or SECTOR_SZ.
    ULONG   ShiftNum;       // By which to convert Length to the unit of bytes.
    ULONG   MaxNumPerLoop;    // the maximum of CurrentIndex.
    ULONG   MaxLoopNum;
}COMMON_CMD_INFO, *PCOMMON_CMD_INFO;

typedef struct _disk_ext{
    UCHAR       DiskIndex;
    ULONG       StartOffset;  // in units of byte or SECTOR_SZ, respectively.
    ULONG       CurrentIndex; // in units of CommonInfo->Length.
    ULONG       LoopNum;
    ULONG       MagicNum;
    ULONG       Crc;
    CMD_STATE   State;
    PCMD_ROUTINE    pHandler;
}DISK_EXT, *PDISK_EXT;

typedef struct _host_cmd_ext{
    UCHAR           Tag;
    DISK_EXT        DiskInfo[ 3 ];
    HOST_CMD        HostCmd;
    CMD_STATE       State;
}HOST_CMD_EXT, *PHOST_CMD_EXT;

typedef enum _HOST_PATTERN_TYPE
{
    WRITE_A = 0,
    READ_A,
    WRITE_B = 0,
    READ_B,
    ERASE_C = 0,
    WRITE_C,
    READ_C
}HOST_PATTERN_TYPE;

extern HOST_CMD g_pMvHCmdTable[MAX_SLOT_NUM];


/* from DOS application by Charles Zhou */
extern U8 g_ucCmdNum;
void MV_CmdInit();
void MV_SendCmd( U8 ucCmdTag );
BOOL MV_CmdSlotIsIdle(U8 ucSlot);

//void BuildHostCmd( PHOST_CMD_EXT CmdInfoFlash );
U32 MV_GetHostCmdDiskBLbaLength( U8 ucCmdTag );
U32 MV_GetHostCmdDiskCLbaLength( U8 ucCmdTag );
STATUS  BuildSubDiskCmd(HOST_CMD *HostCmd, UCHAR DiskIndex, HOST_REQ_TYPE Type, ULONG Offset, ULONG Length );

STATUS WriteDiskAB( PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );
STATUS ReadDiskAB( PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );
STATUS CheckDataForDiskAB(PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );

STATUS AsyncEraseFlash( PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );
STATUS AsyncWriteFlash( PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );
STATUS AsyncReadFlash( PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );
STATUS CheckDataForReadFlash(PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );
STATUS NullFunction( PHOST_CMD_EXT CmdInfoFlash, PCOMMON_CMD_INFO CommonInfo );

STATUS CheckPayload( PUCHAR Buffer, ULONG Size, PULONG pMagicNum, PULONG Crc );
ULONG FillPayload( PUCHAR Buffer, ULONG Size, PULONG pMagicNum);

#endif//__MIX_VECTOR_HOST_APP__
