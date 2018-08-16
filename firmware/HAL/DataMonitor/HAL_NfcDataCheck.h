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
Filename    : HAL_NfcDataCheck.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.10.30
Description : this file is function declaration for NFC Data Check driver
Others      :
Modify      :
20141030    Gavin     Create file
*******************************************************************************/
#ifndef __HAL_NFC_DATACHECK_H__
#define __HAL_NFC_DATACHECK_H__

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_FlashDriverBasic.h"

/* define max number of first byte data supported in one PU level */
#define NFC_DATA_CHECK_MAX_BYTE 128

/* define format of first byte data area which binds to one NFCQ */
typedef struct _FIRST_BYTE_ENTRY
{
    U8 aFirstByteVal[NFC_DATA_CHECK_MAX_BYTE];
}FIRST_BYTE_ENTRY;

/* define format of SRAM allocated for save first byte data for all PUs
Note:
    the addressing method for every FIRST_BYTE_ENTRY is {Logic PU, PU level}
*/
typedef struct _FIRST_BYTE_AREA
{
    FIRST_BYTE_ENTRY aFirstByteEntry[NFC_PU_MAX][NFCQ_DEPTH];
}FIRST_BYTE_AREA;

//fucntion interface
void HAL_DataCheckInit(void);
void HAL_DataCheckSetValue(U8 ucPU, U8 ucWtPtr, U8 ucFirstByteVal, U8 ucFirstByteIndex);

#endif /* __HAL_NFC_DATACHECK_H__ */
