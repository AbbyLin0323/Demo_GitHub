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
Filename    :L2_Init.h
Version     :Ver 1.0
Author      :Henry Luo
Date        :2013.12.23
Description :defines for L2 Init.
Others      :
Modify      :
****************************************************************************/

#ifndef __L2_INIT_H__
#define __L2_INIT_H__

#include "L2_Defines.h"

extern void L2_DSRAM0TrimMap(U32* pFreeDSram0TrimBase);//nickwang add for trim in dsram init
extern void L2_DramMap(U32* pFreeDramBase);
extern void L2_Sram0Map(U32 *pFreeSramBase);
extern void L2_Sram1Map(U32 *pFreeSramBase);
extern void L2_OtfbMap(U32 *pFreeOtfbBase);
extern void L2_InitDataStructures(BOOL bSecurityErase);

#endif

