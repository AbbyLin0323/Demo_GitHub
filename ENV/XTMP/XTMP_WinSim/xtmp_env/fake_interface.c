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
Filename     : fake_interface.c                                    
Version      : 0.1                                               
Date         : 20150528                                        
Author       : Gavin

Description: 
Fake interface for fix linking issue in compiling
Modification History:
20150528    Gavin   created
********************************************************************************/
#include "BaseDef.h"
#include "hfmid.h"

BOOL IsDuringTableRebuild(void)
{
    return FALSE;
}

BOOL IsDuringRebuildDirtyCnt(U8 ucMcuId)
{
    return FALSE;
}

void ResetTableRebuildFlag(void)
{
    return;
}

void Mid_FlashSetBlockErase(U16 uMaxEraseCnt, U16 uMinEraseCnt, U32 ulMaxPercent)
{
    return;
}

void Mid_FlashSetErrRate(U16 uUECCRate, U16 uRECCRate, U16 uPragramErrRate, U16 uEraseErrRate)
{
    return;
}

void Host_ReadFromDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 ulBufferAddr, U32 ByteLen, U32 nTag)
{
    return;
}

void Host_WriteToDeviceInterface(U32 HostAddrHigh, U32 HostAddrLow, U32 ulBufferAddr, U32 ByteLen, U32 nTag)
{
    return;
}

BOOL AHCI_IsHBARegActive(void)
{
    return TRUE;
}

void L3_DbgChkFCmdNoRecycleCnt(U8 ucCE)
{
    return;
}

void L3_DbgChkFCmdHostReadCnt(U8 ucCE)
{
    return;
}

U8 Mid_Read_FlashIDB(PFLASH_PHY pFlash_phy)
{
    return 0;
}

U8 Mid_GetFlashErrCode(PFLASH_PHY pFlash_phy)
{
    return 0;
}

void Mid_Read_RedData(PFLASH_PHY pFlash_phy, char *pRedBuf)
{
    *(U32 *)pRedBuf = 0xFFFFFFFF;
    *(U32 *)((U32)pRedBuf + sizeof(U32)) = 0xFFFFFFFF;
    *(U32 *)((U32)pRedBuf + sizeof(U32)  * 2) = 0xFFFFFFFF;
    return;
}

BOOL Mid_FlashInjError(PFLASH_PHY pFlash_phy, U8 usErrCode, U8 usRetryTime)
{
    return TRUE;
}