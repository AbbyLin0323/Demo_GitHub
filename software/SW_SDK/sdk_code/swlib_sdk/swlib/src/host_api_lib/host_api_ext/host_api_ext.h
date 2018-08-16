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
  File Name     : host_api_ext.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the extended function declaration
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _HOST_API_EXT
#define _HOST_API_EXT
#include "../host_api/host_api.h"





STATUS Api_Ext_Get_Var_Table_Addr(PDEVICE_OBJECT pDevObj,U32 * pVarTableAddr);
STATUS Api_Ext_Jump_To_Fw(PDEVICE_OBJECT pDevObj);
STATUS Api_Ext_L3_Llf(PDEVICE_OBJECT pDevObj);
STATUS Api_Ext_L2_Llf(PDEVICE_OBJECT pDevObj);
STATUS Api_Ext_Mem_Read(PDEVICE_OBJECT pDevObj,CPUID ucCpuId,U32 ulMemAddr,U32 ulLen,U8 *pMemData);
STATUS Api_Ext_Mem_Write(PDEVICE_OBJECT pDevObj,CPUID ucCpuId,U32 ulMemAddr,U32 ulLen,U8 *pMemData);

STATUS Api_Ext_TraceLogControl(PDEVICE_OBJECT pDevObj,CPUID MCUID,TL_CTL tlCtl,U16 secCount);


#endif
