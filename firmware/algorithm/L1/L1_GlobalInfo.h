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
Filename     : L1_GlobalInfo.h                                   
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  Blakezhang

Description: 

Modification History:
20130508     blakezhang     001 first create
*****************************************************************************/

#ifndef _L1_GLOBAL_INFO_H
#define _L1_GLOBAL_INFO_H

#include "L0_Interface.h"

extern GLOBAL BOOL gbGlobalInfoSaveFlag;
extern GLOBAL  U8 g_ucSaveRTStatus;
extern GLOBAL  HOST_INFO_PAGE *g_pSubSystemHostInfoPage;
extern GLOBAL MCU12_VAR_ATTR DEVICE_PARAM_PAGE *g_pSubSystemDevParamPage;

extern void L1_GlobalInfoDramMap(U32 *pFreeDramBase);
extern void L1_GlobalInfoInit(void);
extern void L1_SetDefaultDeviceParam(void);
extern BOOL L1_IsDeviceParamValid(void);
extern void L1_SetDefaultHostInfo(void);
extern BOOL L1_IsHostInfoValid(void);
extern void L1_SaveHostInfo(U32 ulBuffAddr);
extern void L1_LoadHostInfo(U32 ulBuffAddr);
extern void L1_SecurityEraseSetDeviceParam(void);
#endif

/********************** FILE END ***************/

