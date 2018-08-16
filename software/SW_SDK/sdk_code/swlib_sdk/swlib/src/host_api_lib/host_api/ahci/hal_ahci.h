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
  File Name     : hal_ahci.h
  Version       : Release 0.0.1
  Author        : johnzhang
  Created       : 2015/2/28
  Last Modified :
  Description   : Defines the hal function of ahci.
  Function List :
  History       :
  1.Date        : 2015/2/28
    Author      : johnzhang
    Modification: Created file

******************************************************************************/
#ifndef _HAL_AHCI_H
#define _HAL_AHCI_H
#include "host_api_define.h"
STATUS Ahci_Read_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * readbuf);
STATUS Ahci_Write_Dram_Sram(HANDLE hDevice,U8 cpuid,U32 startaddr,U32 length,U8 * writebuf);
STATUS Ahci_Read_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk);
STATUS Ahci_Write_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U16 page,U8 *pStatusBuf,U32 pumsk);
STATUS Ahci_Erase_Flash(HANDLE hDevice,U8 cpuid,U8 plnmode,U8 pln,U16 block,U8 *pStatusBuf,U32 pumsk);
STATUS Ahci_NoneData(HANDLE hDevice,U8 ucCpuId,U8 ucOpCode,U32 ulInParam1,U32 ulInParam2,U32 * pOutParam1,U32 * pOutParam2);
STATUS Ahci_Read_Register(HANDLE hDevice,U32 regaddr,U32 * regdata);
STATUS Ahci_Write_Register(HANDLE hDevice,U32 regaddr,U32 regdata);
STATUS Ahci_Get_Identify_Data(HANDLE hDevice,U8 * readbuf);
STATUS Ahci_Get_ModelNumber(HANDLE hDevice,U8 * identifybuf,U8 mn[40]);
STATUS Ahci_Fw_Download(HANDLE hDevice, U32 ulLength, U8 * pFwBuf);
STATUS Ahci_Get_Smart_Data(HANDLE hDevice,U8 * readbuf);
STATUS Ahci_Cmd_Dma_Read(HANDLE hDevice,U32 lba,U8 * inbuf_addr,U32 trans_sec_cnt);
STATUS Ahci_Cmd_Dma_Write(HANDLE hDevice, U32 lba, U8 * outbuf_addr, U32 trans_sec_cnt);
BOOL Ahci_Rom_Read_Reg(HANDLE hDevice,U32 ulAddr,U32 * pData);
BOOL Ahci_Rom_Write_Reg(HANDLE hDevice, U32 ulAddr, U32 ulData);
BOOL Ahci_Rom_Jump(HANDLE hDevice, U32 ulAddr);
#endif