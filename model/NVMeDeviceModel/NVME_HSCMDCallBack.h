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
  File Name     : hscmd_callback.h
  Version       : Initial Draft
  Author        : Betty Wu
  Created       : 2014/9/15
  Last Modified :
  Description   :
  Function List :
  History       :
  1.Date        : 2014/9/15
    Author      : Betty Wu
    Modification: Created file

******************************************************************************/

#ifndef _HSCMD_CALLBACK_H
#define _HSCMD_CALLBACK_H
#include "BaseDef.h"


BOOL NVME_IdentifyCallBack(U8 *pCMDData);
BOOL NVME_SmartReadLogCallBack(U8 *pCMDData);
BOOL NVME_SmartWriteLogCallBack(U8 *pCMDData);
BOOL NVME_SmartReadDataCallBack(U8 *pCMDData);
BOOL NVME_ReadLogExtCallBack(U8 *pCMDData);
BOOL NVME_GetTLInfoCallBack(U8 *pCMDData);
BOOL NVME_GetTLDataCallBack(U8 *pCMDData);
BOOL NVME_GetVarTableCallBack(U8 *pCMDData);
BOOL NVME_GetErrorLogPage(U8 *pCMDData);
BOOL NVME_GetSMARTLogPage(U8 *pCMDData);
BOOL NVME_GetSlotInfoLogPage(U8 *pCMDData);

#endif