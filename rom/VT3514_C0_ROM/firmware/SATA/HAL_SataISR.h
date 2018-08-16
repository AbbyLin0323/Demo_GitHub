/****************************************************************************
*                  Copyright (C), 2012, VIA Tech. Co., Ltd.                 *
*       Information in this file is the intellectual property of VIA        *
*   Technologies, Inc., It may contains trade secrets and must be stored    *
*                         and viewed confidentially.                        *
*****************************************************************************
Filename    :HAL_BufMap.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.14    19:18:57
Description :
Others      :Coding in the Valentine's Day without Valentine.
Modify      :
****************************************************************************/

#ifndef _HAL_SATAISR_H
#define _HAL_SATAISR_H

#include "BaseDef.h"
#include "HAL_SataDefine.h"

#define CBINT_SATACMD 0
#define CBINT_SATARESET 1
#define CBINT_SATARCVPIODATA 2

#define PARAM_INTSRC_SATA 0
#define PARAM_INTSRC_NFC 1


typedef struct {
    U8 ucIntSrc;
    U8 ucEventType;
    U16 usShortParam;
    U32 ulLongParam1;
    U32 ulLongParam2;
    U32 ulLongParam3;
} INTCALLBACKPARAM;

void L1_IntCallBackProxy(INTCALLBACKPARAM *pCBParam);
void L2_IntCallBackProxy(INTCALLBACKPARAM *pCBParam);
void L3_IntCallBackProxy(INTCALLBACKPARAM *pCBParam);
void HAL_SataRcvCmdISR(void);
void HAL_RcvPIODataISR(void);
void HAL_COMResetISR(void);
void HAL_OOBDoneISR(void);
void HAL_SoftResetISR(void);
void HAL_PrdFinishISR(void);
void HAL_EotPendingISR(void);
void HAL_ClearBridgeIntPendingISR(void);
void HAL_SataISR(void);


#endif

