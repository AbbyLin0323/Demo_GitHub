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
Filename    :
Version     :Ver 1.0
Author      :Nina Yang
Date        :2012.09.21    13:10
Description :
Others      :
Modify      :
****************************************************************************/
#ifndef     _SIM_SEARCH_ENGINE_H
#define     _SIM_SEARCH_ENGINE_H

typedef struct _SEMODEL_MSK_GROUP
{
    U32 MSK1_Low;
    U32 MSK1_High;
    U32 MSK2_Low;
    U32 MSK2_High;
    U32 MSK3_Low;
    U32 MSK3_High;
    U32 MSK4_Low;
    U32 MSK4_High;
    U32 MSK5_Low;
    U32 MSK5_High;

    U32 Value_Low;
    U32 Value_High;

    U8 bXorEn: 1;
    U8 SType: 3;
    U8 CType: 1;
    U8 ResvBit: 3;
    U8 ResvByte[3];
}SIMSE_MSK_GROUP;

typedef struct _SIMSE_VALUE_PARAM
{
    U32 ulStartAddr;
    U32 ulLenQW;
    U8  ucPitchSize;
    U8  bItemQW: 1;
    U8  ResvBit: 7;
    U8  ResvByte[3];

    SIMSE_MSK_GROUP aMskGroup[SE_MSK_GROUP_NUM];
}SEMODEL_VALUE_PARAM;

typedef struct _SIMSE_OVERLAP_PARAM
{
    U32 ulListAddr;
    U32 ulListLenQW;

    U32 ulKeyStartVal;
    U32 ulKeyLen;
}SIMSE_OVERLAP_PARAM;

typedef struct _SIMSE_OVERLAP_LIST_ITEM
{
    U32 ulAccessLen;
    U32 ulStartValue;
}SIMSE_OVERLAP_LIST_ITEM;

void SE_ModelInit(void);
void SE_ModelProcess(U8 ucSEID);
BOOL SERegWrite(U32 regaddr ,U32 regvalue ,U32 nsize);

#endif
