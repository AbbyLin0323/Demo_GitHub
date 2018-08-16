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
Filename    :L0_ATALibAdapter.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_ATALIB_ADAPTER_H__
#define __L0_ATALIB_ADAPTER_H__
#include "HAL_HostInterface.h"

typedef struct _dummy_command_header{
    U32     PRDBC;
}DUMMY_COMMAND_HEADER, *PDUMMY_COMMAND_HEADER;

typedef struct _cfis{
    U8      Type;
    U8      PMPort:4;
    U8      Rsvd:3;
    U8      C:1;
    U8      Command;
    U8      FeatureLo;
    U32     LBALo:24;
    U32     Device:8;
    U32     LBAHi:24;
    U32     FeatureHi:8;
    U16     Count;
    U8      ICC;
    U8      Control;
    U32     Auxiliary;
}CFIS, *PCFIS;

typedef struct _cb_mgr{
    U32            ATAState;
    U32            PIODRQSize;

    U8             SlotNum; // 0~31
    U8             CmdType; // media access / soft reset / others
    U8             SATAProtocol; // PIO / DMA / NCQ / NONDATA / DIAG
    U8             IsWriteDir; // direction in terms of AHCI. 0: read from system memory, 1: write to system memory.

    U32            StageRemainingBytes;// trace actual remaining bytes of host command after some processing
    U32            CurrentLBA;
    U32            CurrentSubCmdIndex;
    U32            HCMDLenSec; //always keep the original length of host command in sector unit.

    U32            RespWaitData; // this "RespWaitData" is dummy in SATA mode. we keep it just for adaptation to ATA Lib 
    PDUMMY_COMMAND_HEADER   Ch; // this "Ch" is dummy in SATA mode. we keep it just for adaptation to ATA Lib 
    U32             TotalRemainingBytes; // this "TotalRemainingBytes" always keep the original length of host command in Byte unit.
                                         // and ATA Lib uses "StageRemainingBytes" to trace actual remaining bytes after some processing

    PCFIS          CFis;
} CB_MGR;

typedef struct _cb_mgr *PCB_MGR;

/* Define D2H FIS for saving VIA command processing result */
#define RD2H_FIS_TYPE                   0x34

typedef struct _rfis
{
    U8      Type;
    U8      PMPort:4;
    U8      Rsvd1:2;
    U8      I:1;
    U8      Rsvd2:1;
    U8      Status;
    U8      Error;
    U32     LBALo:24;
    U32     Device:8;
    U32     LBAHi:24;
    U32     Rsvd3:8;
    U16     Count;
    U16     Rsvd4;
    U32     Rsvd5;
}RD2HFIS, *PRD2HFIS;

extern RD2HFIS g_tRFisOfSataMode;

/* function interface */
void L0_AhciDataXferPrepRespFISSeq(PCB_MGR pSlot);
void L0_AhciDataXferUpdateStageInfo(PCB_MGR pSlot);
void L0_AhciDataXferSendRespInfo(PCB_MGR pCmdSlot);
void L0_AhciSendSimpleResp(PCB_MGR pSlot);
BOOL L0_SataParseIncomingCmd(PCB_MGR pSlot);
BOOL L0_SataCompleteCmd(PCB_MGR pSlot);
U32 L0_ATAMediaAccSplitCmd(PCB_MGR pSlot);
BOOL L0_SATAFwCrcChk(U32 ulFwBaseAddr);
BOOL L0_SATAFwCommitSaveFw(U32 ulFwSlot);
BOOL L0_SATAFwCommitActFw(U32 ulFwSlot);
BOOL L0_SATAFwCommit(void);
U32 L0_ATAGetIdentifyPage(void);

#endif //__L0_ATALIB_ADAPTER_H__
