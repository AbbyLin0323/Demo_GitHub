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
Filename    : HAL_EFUSE.c
Version     : Ver 1.0
Author      : Victor
Date        : 2015.1.16
Description : Driver for programing into /reading from EFUSE(one-time-programed) memory. 
Others      : 
Modify      :
20150116    Victor     Create file
*******************************************************************************/

#ifndef _HAL_EUFSE_H_
#define _HAL_EUFSE_H_
#include "BaseDef.h"
#include "HAL_MemoryMap.h"

#define EFUSE_WDATA_DEPTH   (8)
#define EFUSE_RDATA_DEPTH   (8)

typedef struct EFUSE_REG_SET_
{
    U32 aDATA[EFUSE_WDATA_DEPTH];
    U32 Res[EFUSE_RDATA_DEPTH];
    struct {
        U32 bsProgramed :1;
        U32 bsDataValid :1;
        U32 Res         :30;
    };
}EFUSE_REG_SET;


#endif // _HAL_EUFSE_H_
