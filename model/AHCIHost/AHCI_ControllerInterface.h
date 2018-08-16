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

Filename     :   AHCI_HostModel.h
Version      :   0.1
Date         :   2013.08.26
Author       :   bettywu

Description:  the interface to other model
Others:
Modification History:
20130826 create

*******************************************************************************/

#ifndef _H_AHCI_CONTROLLER_INTERFACE
#define _H_AHCI_CONTROLLER_INTERFACE
//#include "windows.h"
#include "Basedef.h"
//#include "AHCI_HCT.h"


//void AHCI_HostCFillCMDHeader(U8 CMDTag, U32 PrdTLen, U64 CTBaseAddr, BOOL bWrite);

void AHCI_HostCSetPxCI(U8 CMDTag);
void AHCI_HostCClearPxCI(U8 CMDTag);
U8 AHCI_HostCGetPxCI(U8 CMDTag);

void AHCI_HostCSetPxSACT(U8 CMDTag);
void AHCI_HostCClearSACT(U8 CMDTag);
U8 AHCI_HostCGetSACT(U8 CMDTag);
U8 AHCI_HostCGetCmdFinishFlag(U8 nTag);
void AHCI_HostCSetCmdFinishFlag(U8 nTag);
void AHCI_HostCClearCmdFinishFlag(U8 nTag);

void AHCI_RegRead(U32 ulRegAddr, U32 ulBytes, U8 *pDstAddr);
void AHCI_RegWrite(U32 ulRegAddr, U32 ulBytes, U8 *pSrcAddr);

void AHCI_DramRead(U32 ulDramAddr, U32 ulWordss, U8 *pDestBuf);
void AHCI_DramWrite(U32 ulDramAddr, U32 ulWordss, const U8 *pSrcBuf);

void AHCI_WriteToDevice(U32 ulDeviceAddr, U32 ulBytes, const U8 *pSrcBuf);
void AHCI_ReadFromDevice(U32 ulDeviceAddr, U32 ulBtyes, U8 *pDestBuf);

//bool AhciRegWriteHandle(U32 regaddr, U32 regvalue, U32 nsize);
//bool AhciRegReadHandle(U32 regaddr, U32 regvalue, U32 nsize);


#endif