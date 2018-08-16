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
Filename    :L0_Schedule.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef __L0_SCHEDULE_H__
#define __L0_SCHEDULE_H__

#include "BaseDef.h"
#include "L0_ATALibAdapter.h"
#include "L0_ATAGenericCmdLib.h"

// the following 2 definitions are used to represent the status
// of proccessing the current host command
#define L0_HCMD_PROCESSING (0x88)
#define L0_HCMD_FINISH (0x99)

extern GLOBAL BOOL g_bMultiCoreSCMDFlag;

extern U8 L0_SataTask(void);
extern BOOL L0_HcmdSplitCheck(PCB_MGR pSlot);
extern BOOL L0_SataIsDeviceIdle(void);

#endif
