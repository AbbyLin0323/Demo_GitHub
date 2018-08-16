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
Filename     : L1_SataIO.h                                    
Version      :   Ver 1.0                                               
Date         :                                         
Author       :  BlakeZhang

Description: 

Modification History:
20120118 BlakeZhang 001 first create
*****************************************************************************/
#ifndef _L1_AHCIIO_H_
#define _L1_AHCIIO_H_

#include "Disk_Config.h"

#define L1_SATA_NCQ_DEPTH       (32)
#define L1_SATA_DSG_INFO_CNT    (8)

#define L1_DSGINFO_SATAE_INIT   (0)
#define L1_DSGINFO_SATAE_CONFIG (1)
#define L1_DSGINFO_SATAE_FINISH (2)

#define L1_GET_CURR_SCMDINDEX(scmdIndex)  ((scmdIndex) % L1_SATA_DSG_INFO_CNT)
#define L1_GET_LAST_SCMDINDEX(curIndex)   (((U16)((curIndex) - 1)) % L1_SATA_DSG_INFO_CNT)

/* AHCI SGE Info */
typedef struct  _SUBCMD_HOST_INFO
{
    U8 ucSubDSGId;
    U8 ucRsvd[3];
} SUBCMD_HOST_INFO;

typedef struct  _BUFREQ_HOST_INFO
{
    U8 ucTag;
    U8 ucDSGId;
    U8 ucRsvd[2];
} BUFREQ_HOST_INFO;

#if 0
typedef struct  _L1_SATA_DSG_ENTRY
{
      U8 ucState;
      U8 ucDSGId;
} L1_SATA_DSG_ENTRY;

typedef struct  _L1_SATA_DSG_INFO
{
    L1_SATA_DSG_ENTRY tDSGEntry[L1_SATA_NCQ_DEPTH][L1_SATA_DSG_INFO_CNT];
} L1_SATA_DSG_INFO;
#endif

#endif
/****************L1_AhciIO.h END ***************/

