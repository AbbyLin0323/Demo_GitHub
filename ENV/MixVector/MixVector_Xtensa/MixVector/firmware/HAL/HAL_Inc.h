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
Filename    :HAL_Inc.h
Version     :Ver 1.0
Author      :HenryLuo
Date        :2012.02.21    11:19:39
Description :
Others      :A public collection of all HAL related headers.
Modify      :
*******************************************************************************/
#ifndef __BASE_HALINC_H
#define __BASE_HALINC_H

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashCmd.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_NormalDSG.h"
#include "HAL_DMAE.h"
#include "HAL_SearchEngine.h"
#include "HAL_HostInterface.h"

#if defined(HOST_AHCI) || defined(HOST_NVME)
#include "HAL_HSG.h"
#include "HAL_SGE.h"
#include "HAL_ChainMaintain.h"
#include "HAL_HwDebug.h"
#else // sata mode
#include "HAL_BufMap.h"
#include "HAL_SataDSG.h"
#include "HAL_SataIO.h"
#endif

#ifndef SIM
#ifdef PM_ENABLE
#include "HAL_PM.h"
#endif
#include "HAL_Interrupt.h"
#endif

#endif

/********************** FILE END ***************/

