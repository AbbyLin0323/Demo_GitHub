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
Filename    : TEST_NfcDataCheck.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description : this file provide test pattern for NFC first byte check function
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_HostInterface.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashCmd.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_NfcDataCheck.h"
#include "HAL_EncriptionModule.h"

#ifdef VT3514_C0

//extern void EmConfigAllKeys(void);

LOCAL FIRST_BYTE_ENTRY l_tFirstByteEntry;

/* convert buff id to DRAM address in MCU view */
LOCAL U32 BuffIdToAddr(U16 usBuffId)
{
    return (DRAM_START_ADDRESS + usBuffId * PIPE_PG_SZ);
}

/* create DRAM data pattern for NFC write to flash */
LOCAL void CreateDramData(U32 ulDramAddr, U16 usSecCnt)
{
    U32 ulSecIndex;

    for (ulSecIndex = 0; ulSecIndex < usSecCnt; ulSecIndex++)
    {
        *(volatile U8 *)(ulDramAddr + ulSecIndex * SEC_SIZE) = ulSecIndex;
    }

    return;
}

/* create first byte information */
LOCAL void CreateFirstByteInfo(U8 *pFirstByteInfo, U16 usByteCnt, U8 ucStartVal)
{
    U8 ucFirstByteVal;

    ucFirstByteVal = ucStartVal;

    while(usByteCnt--)
    {
        *pFirstByteInfo++ = ucFirstByteVal++;
    }

    return;
}

//create LPN and Key Select for EM
void CreateLpnAndKeySel(U32 *pLPN, U8 *pKeySel, U8 ucLPNCnt)
{
    U8 ulLpnIndex;

    for (ulLpnIndex = 0; ulLpnIndex < ucLPNCnt; ulLpnIndex++)
    {
        *pLPN++ = ulLpnIndex;
        *pKeySel++ = ulLpnIndex;
    }

    return;
}

LOCAL U32 NfcFullPageWrite(FLASH_ADDR *pFlashAddr, U16 usBuffID, NFC_RED *pNfcRed, BOOL bEmEn)
{
    U16 usCurDsgId;    
//    U32 RedOffset;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;    
    NFC_RED *pTargetRed;

    if(TRUE == HAL_NfcGetFull(pFlashAddr->ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    if(TRUE == HAL_NfcGetErrHold(pFlashAddr->ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    
    if (TRUE == bEmEn)
    {
        pNFCQEntry->bsEMEn = TRUE;
    }
    else
    {
        pNFCQEntry->bsFstByteCheck = TRUE;
    }

    pNFCQEntry->bsDsgEn = TRUE;

    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    if (NULL != pNfcRed)
    {
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU,HAL_NfcGetWP(pFlashAddr->ucPU), 0);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)pNfcRed, sizeof(NFC_RED)>>2);
    }    
    pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), 0);
    pNFCQEntry->bsRedEn = TRUE;
    pNFCQEntry->bsPuEnpMsk = TRUE;

    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr);

    pNFCQEntry->aSecAddr[0].bsSecStart = 0;
    pNFCQEntry->aSecAddr[0].bsSecLength = SEC_PER_PG*PLN_PER_PU;
    pNFCQEntry->bsDmaTotalLength = SEC_PER_PG*PLN_PER_PU;

    pDSG->ulDramAddr = HAL_NfcGetDmaAddr(usBuffID, 0, PIPE_PG_SZ_BITS);
    pDSG->bsXferByteLen = PIPE_PG_SZ ;
    pDSG->bsLast = TRUE;

#ifdef FLASH_DRIVER_TEST_LOW_LEVEL
    TEST_SetCs(pDSG,pFlashAddr);
#endif 
    
    HAL_SetNormalDsgSts(usCurDsgId,1);
    
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_PRG_2PLN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_PRG_2PLN);

    return NFC_STATUS_SUCCESS;
}


LOCAL U32 NfcPageRead(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U8 ucSecLength, U16 usBuffID, NFC_RED **ppNfcRed, BOOL bEmEn)
{
    U16 usCurDsgId;  
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;
//    U8 ucCE;

    if(TRUE == HAL_NfcGetFull(pFlashAddr->ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    if(TRUE == HAL_NfcGetErrHold(pFlashAddr->ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    
    if (TRUE == bEmEn)
    {
        pNFCQEntry->bsEMEn = TRUE;
    }
    else
    {
        pNFCQEntry->bsFstByteCheck = TRUE;
    }

    pNFCQEntry->bsDsgEn = TRUE;

    while(FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);

    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    if (NULL != ppNfcRed)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), 0);
        *ppNfcRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), 0);
    }

    HAL_NfcSetFullPageRowAddr(pNFCQEntry,pFlashAddr);
    pNFCQEntry->bsPuEnpMsk = 1;

    pNFCQEntry->aSecAddr[0].bsSecStart = ucSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = ucSecLength;
    pNFCQEntry->bsDmaTotalLength = ucSecLength;

    pDSG->ulDramAddr =  HAL_NfcGetDmaAddr(usBuffID, ucSecStart, PIPE_PG_SZ_BITS);
    pDSG->bsXferByteLen = ucSecLength << SEC_SIZE_BITS ;
    pDSG->bsLast = TRUE;
    HAL_SetNormalDsgSts(usCurDsgId,1);
    
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_READ_2PLN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READ_2PLN);

    return NFC_STATUS_SUCCESS;
}


/* main entry of first byte check test pattern */
void TEST_NfcDataCheckMain(void)
{
    FLASH_ADDR tFlashAddr;
    U8 ucFirstByteIndex;
    U16 usBufferId = 0;

    //init data-check driver, dram data, flash address
    HAL_DataCheckInit();

    CreateDramData(BuffIdToAddr(usBufferId), SEC_PER_PG * PLN_PER_PU);
    
    tFlashAddr.ucPU = 0;
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.usPage = 0;
#ifndef COSIM
    //erase Flash
    HAL_NfcFullBlockErase(&tFlashAddr);
    HAL_NfcWaitStatus(tFlashAddr.ucPU);
#endif
    //test write
    CreateFirstByteInfo(&l_tFirstByteEntry.aFirstByteVal[0], SEC_PER_PG * PLN_PER_PU, 0);
    
    //set first byte information to NFC SRAM entry
    for (ucFirstByteIndex = 0; ucFirstByteIndex < SEC_PER_PG * PLN_PER_PU; ucFirstByteIndex++)
    {        
        HAL_DataCheckSetValue(tFlashAddr.ucPU,
                            HAL_NfcGetWP(tFlashAddr.ucPU),
                            l_tFirstByteEntry.aFirstByteVal[ucFirstByteIndex],
                            ucFirstByteIndex);
    }

    NfcFullPageWrite(&tFlashAddr, usBufferId, NULL, FALSE);
    rTracer = 0xD0;
    HAL_NfcWaitStatus(tFlashAddr.ucPU);
    rTracer = 0xD1;
    //test read
    CreateFirstByteInfo(&l_tFirstByteEntry.aFirstByteVal[0], SEC_PER_PG * PLN_PER_PU, 0);

    //set first byte information to NFC SRAM entry
    for (ucFirstByteIndex = 0; ucFirstByteIndex < SEC_PER_PG * PLN_PER_PU; ucFirstByteIndex++)
    {        
        HAL_DataCheckSetValue(tFlashAddr.ucPU,
                            HAL_NfcGetWP(tFlashAddr.ucPU),
                            l_tFirstByteEntry.aFirstByteVal[ucFirstByteIndex],
                            ucFirstByteIndex);
    }

    NfcPageRead(&tFlashAddr, 0, SEC_PER_PG * PLN_PER_PU, usBufferId, NULL, FALSE);
    rTracer = 0xD2;
    HAL_NfcWaitStatus(tFlashAddr.ucPU);
    rTracer = 0xD3;

    return;
}

/* main entry of EM test pattern */
void TEST_NfcEmMain(void)
{
    FLASH_ADDR tFlashAddr;
    U32 aIv[4];
    U32 aLpn[32];
    U8 aKeySel[32];

    U16 usBufferId = 0;

    //init driver and HW
    COM_MemZero(&aIv[0], 4);
    HAL_EMInit(EM_KEY_SIZE_256BIT, EM_MODE_XTS, &aIv[0]);
    rTracer = 0xE0;

    CreateLpnAndKeySel(&aLpn[0], &aKeySel[0], SEC_PER_PG * PLN_PER_PU / 8);
    
    tFlashAddr.ucPU = 2;
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.usPage = 0;
#ifndef COSIM
    //erase Flash
    HAL_NfcFullBlockErase(&tFlashAddr);
    HAL_NfcWaitStatus(tFlashAddr.ucPU);
#endif
    //test write    
    HAL_EmSetSeed(tFlashAddr.ucPU,
                        HAL_NfcGetWP(tFlashAddr.ucPU),
                        &aLpn[0],
                        &aKeySel[0],
                        SEC_PER_PG * PLN_PER_PU / 8);

    NfcFullPageWrite(&tFlashAddr, usBufferId, NULL, TRUE);
    rTracer = 0xE1;
    HAL_NfcWaitStatus(tFlashAddr.ucPU);
    rTracer = 0xE2;
    //configure again
    //EmConfigAllKeys();
    rTracer = 0xE3;
    //test read
    HAL_EmSetSeed(tFlashAddr.ucPU,
                        HAL_NfcGetWP(tFlashAddr.ucPU),
                        &aLpn[0],
                        &aKeySel[0],
                        SEC_PER_PG * PLN_PER_PU / 8);

    NfcPageRead(&tFlashAddr, 0, SEC_PER_PG * PLN_PER_PU, usBufferId, NULL, TRUE);
    rTracer = 0xE4;
    HAL_NfcWaitStatus(tFlashAddr.ucPU);
    rTracer = 0xE5;

    return;
}


LOCAL void DramNfcInit(void)
{
#ifdef COSIM
    HAL_DramcInit();
    HAL_NfcAsyncTimingInit(TOGGLE2,NFTM_MODE4);
#endif
    HAL_FlashInit();
#ifdef COSIM
    HAL_NfcDefaultPuMapInit();
#endif
#if 1
    //initilaztion otfb relative reg
    // for B0 version:GLB28 = 0x16a30300; GLB2c = 0x128398a4;
    rGlbOTFBMCtrl1 = 0xa418820;
    rGlbOTFBMCtrl0 = 0xe620600;//((0xe620000 & 0xffff0000)| (0x600 & 0xffff));

    rTracer = 0xf;
    //ahci pcie relative (informeb by vince)
    rGlbSoftRst = 0x0;
    rGLB(0x54) = 0x0;
#endif
    HAL_NormalDsgInit();
}

/* main entry of EM test pattern */
void TEST_DataCheck_EM(void)
{
    DramNfcInit();

    rTracer = 0xDF;

    TEST_NfcDataCheckMain();

    //TEST_NfcEmMain();

    rTracer = 0xFF;
}

#endif // VT3514_C0
