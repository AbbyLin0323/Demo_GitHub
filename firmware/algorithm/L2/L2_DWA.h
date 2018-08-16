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
File Name     : L2_DWA.h
Version       : Initial Draft
Author        : henryluo
Created       : 2014/12/4
Description   : dynamic write acceleration
Function List :
History       :
1.Date        : 2014/12/4
Author      : henryluo
Modification: Created file

******************************************************************************/

#ifndef __L2_DWA_H__
#define __L2_DWA_H__

#include "BaseDef.h"
#include "Disk_Config.h"

//#define DWA
#define MAX_AC_PER_PU       160
#define IDLE_GC_THRESHOLD   MAX_AC_PER_PU / 2
#define FLOAT_FACTOR_BIT    10
#define MAX_FLOAT_VALUE     (INVALID_8F >> FLOAT_FACTOR_BIT)
#define DWA_EN_MAX_LS       (85 << FLOAT_FACTOR_BIT)/100

typedef enum _IDLE_GC_STAGE_
{
    IDLE_GC_STAGE_INIT = 0,
    IDLE_GC_STAGE_SLC,
    IDLE_GC_STAGE_MLC,
    IDLE_GC_STAGE_DONE,
    IDLE_GC_STAGE_ALL
} IDLE_GC_STAGE;

typedef enum _IDLE_GC_SLC_STAGE_
{
    IDLE_GC_SLC_STAGE_INIT = 0,
    IDLE_GC_SLC_STAGE_SEL_SRC,
    IDLE_GC_SLC_STAGE_LOAD_RPMT,
    IDLE_GC_SLC_STAGE_READ,
    IDLE_GC_SLC_STAGE_WRITE,
    IDLE_GC_SLC_STAGE_ERASE,
    IDLE_GC_SLC_STAGE_DONE,
    IDLE_GC_SLC_STAGE_ALL
} IDLE_GC_SLC_STAGE;



typedef struct _IDLE_GC_MANAGEMENT_
{
    IDLE_GC_SLC_STAGE m_IdleGCSLCStage;
    U16 m_SrcBlock;

}
IDLE_GC_MANAGEMENT;

typedef struct _DWA_MANAGEMENT_
{
    U32 m_UsedAC[SUBSYSTEM_LUN_MAX];
    
    IDLE_GC_STAGE m_IdleGCStage;
    U32 m_IdleGCDonePuMap;
}DWA_MANAGEMENT;



extern BOOL L2_IsCanBeAccWrite(U8 ucPU);
extern BOOL L2_IsNeedIdleGC(void);

#endif

