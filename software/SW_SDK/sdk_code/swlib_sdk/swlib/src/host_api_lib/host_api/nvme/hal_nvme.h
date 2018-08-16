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
  File Name     : host_nvme.cpp
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the hal function of nvme declaration.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/

#ifndef _HAL_NVME_H
#define _HAL_NVME_H
#include "host_api_define.h"
#define MAX_NVME_TRANS_CNT 32*1024//128*1024
STATUS Nvme_Read_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf);
STATUS Nvme_Write_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf);
STATUS Nvme_Read_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk);
STATUS Nvme_Write_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk);
STATUS Nvme_Erase_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U8 *pStatusBuf,U32 pumsk);
STATUS Nvme_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2);
STATUS Nvme_Get_Identify_Data(HANDLE hDevice,U8 * readbuf);
STATUS Nvme_Fw_Download(HANDLE hDevice, U32 ulLength, U8 * pFwBuf);
STATUS Nvme_Get_ModelNumber(HANDLE hDevice,U8 * identifybuf,U8 mn[40]);
STATUS Nvme_Get_Smart_Data(HANDLE hDevice, U8 * readbuf);
#endif