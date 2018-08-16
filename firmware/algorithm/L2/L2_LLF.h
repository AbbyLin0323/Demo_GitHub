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
Filename    :L2_LLF.h
Version     :Ver 1.0
Author      :Peter Xiu
Date        :2012.03.06
Description :
Others      :
Modify      :
****************************************************************************/


#ifndef __L2_LLF_H__
#define __L2_LLF_H__


typedef enum _L2LLFStage
{
    L2_LLF_INIT,
    L2_LLF_BUILD_PBIT,
    L2_LLF_BUILD_PBIT_CMD_SEND,
    L2_LLF_BUILD_PBIT_CMD_WAIT,
    L2_LLF_BUILD_PBIT_CMD_DONE,
    L2_LLF_DONE
}L2LFStage;


typedef enum _L2LLFStatus
{
    L2_LLFSuccess,
    L2_LLFPending,
    L2_LLFFail
}L2LFStatus;

typedef enum _L2_LLF_STATUS
{
    L2_LLF_RECOVER_PBIT,
    L2_LLF_START,
    L2_LLF_BBTBUILD,
    L2_LLF_RT_AT0_TABLE,
    L2_LLF_SAVE_BBT,
    L2_LLF_PMT_TABLE,
    L2_LLF_SAVE_TABLE,
    L2_LLF_FAIL,
    L2_LLF_SUCCESS
}L2_LLF_STATUS;



void L2_LLFInit();
U32 L2_LLF(BOOL bKeepEraseCnt, BOOL bSecurityErase);
#endif
