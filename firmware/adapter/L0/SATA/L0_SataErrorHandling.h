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
Filename    :L0_SataErrorHandling.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_SATAERRORHANDLING_H__
#define __L0_SATAERRORHANDLING_H__

#include "BaseDef.h"

enum _SATA_ERR_TYPE
{
    SATAERR_TYPE_SERROR = 0,
    SATAERR_TYPE_COMRESET_FIRST,
    SATAERR_TYPE_COMRESET,
    SATAERR_TYPE_SOFTWARE_RESET,
    SATAERR_TYPE_INTERMIX,
    SATAERR_TYPE_TAG,
    SATAERR_TYPE_NCQ_LBA,
    SATAERR_TYPE_NCQ_UECC,
    SATAERR_TYPE_LEGACY_UECC,
    SATAERR_TYPE_LEGACY_LBA,
    SATAERR_TYPE_ALL
};

enum _SATA_ERR_STATE
{
    SATAERR_STATE_NEW = 0,
    SATAERR_STATE_CLEAN_START,
    SATAERR_STATE_WAIT_SCQ_CLEANUP,
    SATAERR_STATE_FORCE_IDLE,
    SATAERR_STATE_SELF_TEST,
    SATAERR_STATE_WAIT_SUBSYS,
    SATAERR_STATE_SUBSYS_OK,
    SATAERR_STATE_DONE,
    SATAERR_STATE_WAIT_LINK_IDLE
};

enum _SATA_REJECT_REASON
{
    SATA_REJECT_DISK_LOCK = 0,
    SATA_REJECT_SECURITY_LOCK,
    SATA_REJECT_NOT_SUPPORT,
    SATA_REJECT_LEGACY_LBA,
    SATA_REJECT_RAW_DATA_PARAM
};

typedef union  _SATA_ERR_INFO
{
   /* Command Error */
    struct 
    {
        U8 ucTag; 
        U8 aCmdRsv[3];
        U32 ulLBA;
        U32 ulSecCnt;
        U32 ulNcqOutstd;
    };

    /* SERROR */
    struct
    {
        U16 usSErrType;
        U16 usSErrRsv;
        U32 ulSErrCnt;
        U32 aSErrRsv[2];
    };

    U32 aErrInfoInDW[4];
}SATA_ERR_INFO;

typedef struct _SATA_ERR_MARK
{
    U32 ulErrBitmap;
    SATA_ERR_INFO aErrInfo[SATAERR_TYPE_ALL];
}SATA_ERR_MARK;

typedef struct _SATA_ERR_STATUS
{
    U8 bsPending: 1;
    U8 bsRsv: 7;
    U8 ucType;
    U8 ucState;
    SATA_ERR_INFO tErrInfo;
}SATA_ERR_STATUS;

typedef struct _SATA_REJECT_MARK
{
    U8 uctReason;
    U8 ucTag;
    U8 ucCmdCode;
    U8 ucRsv;
}SATA_REJECT_MARK;

void L0_LockDisk(void);
void L0_UnlockDisk(void);
BOOL L0_IsDiskLocked(void);
void L0_SataSetUECCMsgRcv(U8 ucTag);
void L0_SataClearUECCMsgRcv(void);
void L0_SataSetSCQCleanupMsgRcv(U32 ulSubsysId);
BOOL L0_SataIsErrorPending(void);
U8 L0_SataIsUECCTag(void);
BOOL L0_SataIsEncounterUECC(void);
void L0_SataEhInit(void);
void L0_SataMarkCmdError(U8 ucType, U8 ucTag, U32 ulLBA, U32 ulSecCnt);
void L0_SataMarkReset(U8 ucRstType);
void L0_SataMarkSError(U16 usSErrType);
BOOL L0_SATAErrorPending(void);
BOOL L0_SataSelectErrPro(void);
BOOL L0_SataErrorHandling(void *);
void L0_SataReceiveUECCMsg(U8 ucTag);
void L0_SataMarkCmdReject(U8 ucReason, U8 ucTag, U8 ucCmdCode);
BOOL L0_SataCmdReject(void *);

#endif
