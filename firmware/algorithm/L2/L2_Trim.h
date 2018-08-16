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
Filename    :L2_Trim.h
Version     :Ver 1.0
Author      :CloudsZhang
Date        :2012.09.17
Description :defines for Trim Enrty functions.
Others      :
Modify      :
*******************************************************************************/
#ifndef __L2_TRIM_H__
#define __L2_TRIM_H__

#include "BaseDef.h"
#include "L2_Defines.h"

extern void L2_TrimReqProcess(U32 TrimStartLPN, U32 TrimLPNCount);

#endif
