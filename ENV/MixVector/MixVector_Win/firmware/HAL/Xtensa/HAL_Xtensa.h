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
Filename    : HAL_Xtensa.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.02
Description : this file declare interface for operation to Tensilica MCU core 
Others      : some interfaces are available only in Xtensa ENV.
Modify      :
20140902    Gavin     Create file
20140904    Gavin     add function declaration
*******************************************************************************/
#ifndef __HAL_XTENSA_H__
#define __HAL_XTENSA_H__

/* MCU's ID: MCU's internal register PRID record its ID */
#define MCU0_ID     1
#define MCU1_ID     2
#define MCU2_ID     3

/* function interface */
U32 HAL_GetMcuId(void);
void HAL_DisableMCUIntAck(void);
void HAL_EnableMCUIntAck(void);
void HAL_DisableMCUIntSrc(U32 ulDisableMap);
void HAL_EnableMCUIntSrc(U32 ulEnableMap);
void HAL_MCUWaitForInt(void);
U32 HAL_GetMCUIntSrc(void);
U32 HAL_GetMCUExcCause(void);
void HAL_SaveMCUExcCause(U32 ulExcCause);
void HAL_MemoryWait(void);
void HAL_EnableICache(U32 ulMcuId);
void HAL_EnableDCache(U32 ulMcuId);
void HAL_InvalidateDCache(void);
U32 HAL_GetMCUCycleCount(void);
void HAL_DelayCycle(U32 ulCycleCnt);
void HAL_WSRVecBase(U32 ulNewVecBase);
U32 HAL_GetBssSegEndAddr(void);
U8 HAL_CLZ(U32 ulDw);
U8 HAL_POPCOUNT(U32 ulDw);
void HAL_Wclzstate(U32 ulDw);
U8 HAL_SCLZ(void);

#endif /* __HAL_XTENSA_H__ */
