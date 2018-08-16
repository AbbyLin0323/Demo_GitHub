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
Filename    :HostInc.h
Version     :
Author      :Blakezhang
Date        :
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef _EX_HEADER_H_INCLUDED
#define _EX_HEADER_H_INCLUDED


#ifdef SIM_DBG
#define HOST_LogInfo(log_fun, log_level, x, ...) \
    SystemStatisticRecord(log_fun, TRACE_SATAHOST_MODULE_ID,TRACE_SATAHOST_SUBMODULE_ID,log_level,x, __VA_ARGS__);
#else
#define HOST_LogInfo(log_fun, log_level, x, ...)  (void)0
#endif

#include "BaseDef.h"

#endif
