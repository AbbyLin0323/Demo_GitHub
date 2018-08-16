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
Filename    : TEST_DMAE.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#ifndef _TEST_DMAE_H
#define _TEST_DMAE_H

void TEST_DMAEMain(void);
void DMAE_TEXT_ATTR HAL_DMAEHeadCopyOneBlock(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte, const U32 ulMCUID);

#endif
