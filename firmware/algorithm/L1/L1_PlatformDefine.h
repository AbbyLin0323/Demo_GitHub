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
Filename     : L1_PlatformDefine.h 
Version      :  Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20141118     BlakeZhang     001 first create
****************************************************************************/
#ifndef _L1_PLATFORMDEFINE_H
#define _L1_PLATFORMDEFINE_H

/* Host Platform Selection */
#if defined(HOST_SATA)
#include "L1_SataIO.h"
#elif defined(HOST_AHCI)
#include "L1_AhciIO.h"
#elif defined(HOST_NVME)
#include "L1_NvmeIO.h"
#endif

#endif

/********************** FILE END ***************/

