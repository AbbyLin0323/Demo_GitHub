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
Filename    :FW_DataMonitor.h
Version     :Ver 1.0
Author      :Blake Zhang
Date        :2014.07.30
Description :
Others      :
Modify      :
****************************************************************************/

#ifndef _HAL_DATAMONITOR_H
#define _HAL_DATAMONITOR_H
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

extern void FW_DataMonitorInit(U32 *pFreeDramBase);
extern void FW_DataMonitorSet(U32 ulLBA, U8 ucSignature);
extern U8 FW_DataMonitorGet(U32 ulLBA);
extern void FW_DataMonitorCheck(U32 ulLBA, U8 ucSignature);
extern void FW_DataMonitorUpdateLPN(U32 ulLPN, U8 ucLPNOffset, U32 ulBufAddr);
extern void FW_DataMonitorResetLBARange(U32 ulStartLBA, U32 ulEndLBA);
extern void FW_DataMonitorCheckRange(U32 ulStartLBA, U8 ucSecLength, U8 ucSecOffset, U32 ulBufAddr);
extern void FW_DataMonitorUpdateWriteData(U8* pucWriteData, U32 ulBufAddr);
extern void FW_DataMonitorCheckWriteData(U8* pucWriteData, U32 ulBufAddr);
extern void FW_DataMonitorSetWriteNFCCheck(U8 ucPU, U8 ucWtPtr, U16 usPhyBufID);
extern void FW_DataMonitorSetReadNFCCheck(U8 ucPU, U8 ucWtPtr, U8 ucSecLen, U32 ulStartLBA);

#endif


