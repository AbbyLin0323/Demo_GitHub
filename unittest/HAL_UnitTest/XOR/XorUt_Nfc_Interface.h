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
Description :
*******************************************************************************/
#ifndef __XORUT_NFC_INTERFACE_H__
#define __XORUT_NFC_INTERFACE_H__

#include "BaseDef.h"
#include "HAL_FlashDriverBasic.h"

U32 XorUt_GetSsuDramBaseAddr(void);
void XorUt_NfcFullPlnWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor);
void XorUt_NfcFullPlnTlcWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulTlcProgramCycle, U32 ulCurXorPageNumInXore, U32 ulXorPageNumInTotal, U32 ulXoreId,
    BOOL bIsLastNfcInXor, U32 ulXorParityPartNum);
void XorUt_NfcSinglePlnWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor);
void XorUt_NfcSinglePlnTlcWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulTlcProgramCycle, U32 ulCurXorPageNumInXore, U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor);
void XorUt_NfcFullPlnRead(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, BOOL bNeedDoXor, U32 ulXoreId);
void XorUt_NfcSinglePlnRead(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, BOOL bNeedDoXor, U32 ulXoreId, BOOL bIsTlcRecover, U32 ulPageType);
void XorUt_Nfc3dMlcFullPlnWrite(FLASH_ADDR *pFlashAddr, U32 ulPageDataAddr, U32 ulPageRedunAddr,
    U32 ulXorPageNumInTotal, U32 ulXoreId, BOOL bIsLastNfcInXor);

#endif/* __XORUT_NFC_INTERFACE_H__ */
