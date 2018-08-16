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
Filename     : L1_NvmeIO.h                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  NinaYang

Description: 

Modification History:
20141124 NinaYang 001 first create
*****************************************************************************/
#ifndef _L1_AHCIIO_H_
#define _L1_AHCIIO_H_

#include "HAL_ChainMaintain.h"

/* AHCI SGE Info */
typedef struct  _SUBCMD_HOST_INFO
{
    U32 HSGBuildByte;
    U32 HSGRemByte;

    U32 PrdIndex: 6;
    U32 PrdOffsetByte: 22;
    U32 Resv0: 4;

    U16 FirstHSGId; //0xFFFF--no first DSG
    U16 FirstDSGId; //0xFFFF--no first HSG

    U16 PreHSGId;
    U16 HSGNon: 1; //1--no HSG have been build
    U16 DSGNon: 1; //1--no DSG have been build
    U16 QBuilt: 1; //1--DRQ/DWQ for this subcmd has been built
    U16 HSGFinish: 1;
    U16 DSGFinish: 1;
    U16 LenAdd: 1;
    U16 Resv1: 10;
} SUBCMD_HOST_INFO;

typedef struct  _BUFREQ_HOST_INFO
{
    HMEM_DPTR HmemDptr;
} BUFREQ_HOST_INFO;

#endif
/****************L1_AhciIO.h END ***************/

