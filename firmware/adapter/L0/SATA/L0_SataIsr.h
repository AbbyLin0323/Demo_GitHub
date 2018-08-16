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
Filename    :L0_SataIsr.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_SATAISR_H__
#define __L0_SATAISR_H__

#include "BaseDef.h"
#include "L0_HcmdChain.h"

extern void L0_SataSetTransferParamNCQ(HCMD* pHostCmd);
extern void L0_SataReceiveCmdIsr(void);
extern void L0_SataISR(void);
extern void L0_OOBDoneISR(void);
extern void L0_COMResetISR(void);
extern void L0_SoftResetISR(void);
extern void L0_EotPendingISR(void);
extern void L0_NCQCmdFinishISR(void);
extern void L0_SataSerrISR(void);
extern void L0_BIST_L(void);

#endif
