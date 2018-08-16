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
* File Name    : NVME_HostCommand.h
* Discription  :
* CreateAuthor : Haven Yang
* CreateDate   : 2014.11.3
*===============================================================================
* Modify Record:
*=============================================================================*/
#ifndef _NVME_HOSTCOMMAND_H
#define _NVME_HOSTCOMMAND_H

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "HAL_HostInterface.h"
#include "HAL_HCT.h"
#include "HAL_NVME.h"
/*============================================================================*/
/* #define region: constant & MACRO defined here                              */
/*============================================================================*/

typedef struct _NVME_CMD_TABLE
{
    U32 ulXferLba;      /* transfered Lba count */
    U32 ulXferByte;     /* transfered Byte count */
    union {
        U32 ulStartLba;
        U32 ulViaSubCmd;
    };
    U32 ulLbaLen;
    U32 ulCmdCode;

    U32 PRP1;
    U32 PRP2;

    U32 ucCmdFinishFlag;

    U32 Protocal;

}NVME_CMD_TABLE;


/*============================================================================*/
/* #typedef region: global data structure & data type typedefed here          */
/*============================================================================*/

/*============================================================================*/
/* function declaration region: declare global function prototype             */
/*============================================================================*/
void NVME_FillHostCmdTable(U32 ulSlotNum, U32 ulHostAddrHigh, U32 ulHostAddrLow, U32 bsCST);

U32 NVME_GetStartLBA(U32 ulSlotNum);

U32 NVME_GetLbaLen(U32 ulSlotNum);

U32 NVME_XferedSecs(U32 ulSlotNum);

U32 NVME_XferedBytes(U32 ulSlotNum);

void NVME_XferBtyes(U32 ulSlotNum,U32 ulXferBytes);

void NVME_XferOneSector(U32 ulSlotNum);
void NVMe_GetLbaFromHostCmdTable(LARGE_INTEGER *pHostAddr, LARGE_INTEGER *pLbaStartAddr,U8 CmdTag, U32 *pByteLen, U32 *pStartLba, U32 *pEndLba);
BOOL NVME_HostIsLbaCmd(U32 ulSlotNum);
U32 NVME_GetCmdCode(U32 ulSlotNum);
void NVME_HostCSetCmdFinishFlag(U8 ucTag);
BOOL NVME_IsHitHostLba(U32 ulStartLba, U32 ulSecCnt);
BOOL NVMe_IsFetchCmdEmpty();
void NVMe_ClearHostCmdTable(void);

#endif
/*====================End of this head file===================================*/

