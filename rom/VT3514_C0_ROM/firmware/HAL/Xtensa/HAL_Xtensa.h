/****************************************************************************
*                  Copyright (C), 2014, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    : HAL_Xtensa.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.09.02
Description : this file declare interface for operation to Tensilica MCU core 
Others      : some interfaces are available only in Xtensa ENV.
Modify      :
20140902    Gavin     Create file
20140904    Gavin     add function declaration
****************************************************************************/
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

U8 HAL_CLZ(U32 ulDw);

#endif /* __HAL_XTENSA_H__ */
