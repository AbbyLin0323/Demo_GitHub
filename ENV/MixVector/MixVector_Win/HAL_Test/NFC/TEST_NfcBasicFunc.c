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
Filename    : TEST_NfcBasicFunc.c
Version     : Ver 1.0
Author      : TobeyTan
Date        : 2013-11-28
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "TEST_NfcMain.h"
#include "TEST_NfcOption.h"


#define TEST_SSU0_DATA  ('S')
#define TEST_SSU1_DATA  ('U')
#define TEST_PATTERN_SEL    (rNFC(0x2bc))
/*
#ifndef COSIM
#define ENABLE_SSU          (FALSE)
#define ENABLE_CS           (FALSE)
#define TEST_INJ_ERRTYPE    (0)


#else
#define TEST_INJ_ERRTYPE    (rNFC(0x2b8))
#define ENABLE_SSU          (rNFC(0x2b4)&0x1)
#define ENABLE_CS           (rNFC(0x2b4)&0x2)
#endif
*/
#define P_TIMING_MODE       12
#define P_READ_RETRY        11
#define P_PIO_GET_FEAT      10
#define P_NORMAL_GET_FEAT   9
#define P_NORMAL_SET_FEAT   8
#define P_PIO_SET_FEAT      7
#define P_READ_ID           6
#define P_PART_READ         5
#define P_SSU_CS            4
#define P_SINGLE_PLN        3
#define P_BYTE_MODE_READ    2
#define P_ERR_INJ           1

#define NF_ERR_TYPE_USUP              1  // Un-recognized command/Invalid command
#define NF_ERR_TYPE_PRG               2  // Program error
#define NF_ERR_TYPE_ERS               3  // Erase error
#define NF_ERR_TYPE_UECC              4  // ECC unrecoverable error
#define NF_ERR_TYPE_RECC              5  // ECC recoverable error with error counter>threshold
#define NF_ERR_TYPE_NO_DEV            6  // No device detected

LOCAL U32 l_ulSsu0BaseAddr;
LOCAL U32 l_ulSsu1BaseAddr;
LOCAL U32 l_ulCsBaseAddr;

LOCAL U32 l_ulSsuAddr[CE_MAX];
LOCAL U32 l_ulCsAddr[CE_MAX];
LOCAL BOOL l_bIsSinglePln;

LOCAL U32 l_ulTestPuNum;
LOCAL U32 l_ulEnable;
LOCAL U32 l_ulErrInj;

volatile NFC_RED **pRed;

U32 GetOffset(FLASH_ADDR *pFlashAddr)
{
    return (pFlashAddr->ucPU << NFCQ_DEPTH_BIT) + HAL_NfcGetWP(pFlashAddr->ucPU);
}
/****************************************************************************
Function  : GetCacheStatus
Input     :
Output    :

Purpose   : Get the cache status
Reference :
****************************************************************************/
static U8 GetCacheStatus(U32 ulCacheAddr)
{
    U32 addr = OTFB_START_ADDRESS + ulCacheAddr;
    DBG_Printf("GetCacheStatus ADDR = 0x%x \n",addr);
    return *((U8 *)(addr));
}
/****************************************************************************
Function  : SetCacheStatus
Input     :
Output    :

Purpose   : set the cache status
Reference :
****************************************************************************/
static void SetCacheStatus(U32 CacheAddr, U32 value)
{
   *((U8*)(OTFB_START_ADDRESS+CacheAddr))=value;
}
/****************************************************************************
Function  : GetSsuStatus
Input     :
Output    :

Purpose   : get the Ssu status
Reference :
****************************************************************************/
static U32 GetSsu0Status(U32 SsuAddr)
{
    U32 addr = l_ulSsu0BaseAddr+(SsuAddr<<2);
    return *((U32 *)(addr));
}
/****************************************************************************
Function  : SetCacheStatus
Input     :
Output    :

Purpose   : set the ssu status
Reference :
****************************************************************************/
static void SetSsu0Status(U16 SsuAddr, U32 value)
{

    U32 addr = l_ulSsu0BaseAddr+(SsuAddr<<2);
    *((U32 *)(addr))=value;
}
/****************************************************************************
Function  : GetSsuStatus
Input     :
Output    :

Purpose   : get the Ssu status
Reference :
****************************************************************************/
static U8 GetSsu1Status(U16 SsuAddr)
{
    U32 addr = l_ulSsu1BaseAddr + (SsuAddr);
    return *((U8 *)(addr));
}
/****************************************************************************
Function  : SetCacheStatus
Input     :
Output    :

Purpose   : set the ssu status
Reference :
****************************************************************************/
static void SetSsu1Status(U16 SsuAddr, U32 value)
{
    U32 addr = l_ulSsu1BaseAddr + (SsuAddr);
    *((U8 *)(addr))=(U8)value;
}

void TEST_NfcSetSSU(NFCQ_ENTRY* pNFCQEntry, FLASH_ADDR *pFlashAddr)
{
    U16 usSsu0Addr , usSsu1Addr;
    U32 ulOffset = GetOffset(pFlashAddr);
    if(TRUE == TEST_NfcSsuCsEn())
    {
        pNFCQEntry->bsSsu0En = TRUE;
        pNFCQEntry->bsSsu1En = TRUE;
        pNFCQEntry->bsSsuAddr0 = ulOffset;
        pNFCQEntry->bsSsuAddr1 = ulOffset;
        pNFCQEntry->bsSsuData0 = TEST_SSU0_DATA;
        pNFCQEntry->bsSsuData1 = TEST_SSU1_DATA;
        SetSsu0Status(ulOffset,0);
        SetSsu1Status(ulOffset,0);
        l_ulSsuAddr[pFlashAddr->ucPU] = ulOffset;
        l_ulSsuAddr[pFlashAddr->ucPU] = ulOffset;
    }
}

U32 TEST_NfcGetCacheStatusAddr(FLASH_ADDR *pFlashAddr)
{
    return  l_ulCsBaseAddr - OTFB_START_ADDRESS + GetOffset(pFlashAddr) ;
}

void TEST_NfcSSUandCsInit(void)
{
    if (MCU1_ID == HAL_GetMcuId())
    {
        l_ulSsu0BaseAddr = OTFB_SSU0_MCU1_BASE;
        l_ulSsu1BaseAddr = OTFB_SSU1_MCU1_BASE;
        l_ulCsBaseAddr   = OTFB_CACHE_STATUS_MCU1_BASE;
    }
    else if (MCU2_ID == HAL_GetMcuId())
    {
        l_ulSsu0BaseAddr = OTFB_SSU0_MCU2_BASE;
        l_ulSsu1BaseAddr = OTFB_SSU1_MCU2_BASE;
        l_ulCsBaseAddr   = OTFB_CACHE_STATUS_MCU2_BASE;
    }
    else
    {
        DBG_Printf("SSU and CacheStatus init wrong!\n");
    }
    //DBG_Printf("l_ulSsu0BaseAddr = 0x%x\n",l_ulSsu0BaseAddr);
    //DBG_Printf("l_ulSsu1BaseAddr = 0x%x\n",l_ulSsu1BaseAddr);
    //DBG_Printf("l_ulCsBaseAddr = 0x%x\n",l_ulCsBaseAddr);
}

void TEST_NfcNormalSetFeature(void)
{
    //HAL_NfcSetAllFlashFeature();
    if (SUCCESS == HAL_NfcSetAllFlashFeature())
    {
         DBG_Printf("Set all flash feature success!\n");
    }
}

void TEST_NfcPioSetFeature(void)
{
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    FLASH_ADDR tFlashAddr = {0};
    tFlashAddr.ucLun = 0;

    for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
    {
        HAL_NfcPioSetFeature(tFlashAddr.ucPU, 0x2, 0x10);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
        {
            DBG_Printf("PU %d PIO set feature fail! ADDR 0x%x Value 0x%x\n",tFlashAddr.ucPU,0x10,0x2);
        }
        HAL_NfcPioSetFeature(tFlashAddr.ucPU, 0x0, 0x80);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
        {
            DBG_Printf("PU %d PIO set feature fail! ADDR 0x%x Value 0x%x\n",tFlashAddr.ucPU,0x80,0x0);
        }
    }
}

void TEST_SetErrInj(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    if(TRUE == HAL_NfcGetFull(pFlashAddr->ucPU))
    {
        ;
    }

    if(TRUE == HAL_NfcGetErrHold(pFlashAddr->ucPU))
    {
        ;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsPuEnpMsk = TRUE;

    pFlashAddr->usPage = 0;
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr);
    #ifdef MIX_VECTOR
//    pNFCQEntry->bsSsu1En = TRUE;
//    pNFCQEntry->bsSsuAddr1 = NFC_GetSSU1Addr() - OTFB_SSU1_MCU1_BASE;
    #endif
    //Error injection

    pNFCQEntry->bsInjEn = TRUE;
    pNFCQEntry->bsErrTypeS = l_ulErrInj;


    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_ERS_2PLN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_ERS_2PLN);

}


//Function add by abby for test NFC

void SyncTimingInit(void)
{
    REG_SET_VALUE(rNFC(0x4),3,2,7);   // Counter load value for tCAD
    REG_SET_VALUE(rNFC(0x8),2,2,22);  // DDR mode configuration. ONFI3.0/TOGGLE2.0 configuration.
    REG_SET_VALUE(rNFC(0x4),3,0,17);  // Delay from read command sent out to read data output at last.

    return;
}

void AsyncTimingInit(U8 ucIfType, U8 ucMode)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;

    if (ONFI2 == ucIfType)
    {
        pNfcPgCfg->IsMicron = TRUE;
    }

    rNFC(0x4)   = t_aAsyncTConfTab[ucIfType][ucMode].ulNFC04;
    rNFC(0x8)   = t_aAsyncTConfTab[ucIfType][ucMode].ulNFC08;
    rNFC(0x2C)  = t_aAsyncTConfTab[ucIfType][ucMode].ulNFC2C;
    rNFC(0x30)  = t_aAsyncTConfTab[ucIfType][ucMode].ulNFC30;
    rNFC(0x80)  = t_aAsyncTConfTab[ucIfType][ucMode].ulNFC80;
    rNFC(0x88)  = t_aAsyncTConfTab[ucIfType][ucMode].ulNFC88;
    rPMU(0x20)  = t_aAsyncTConfTab[ucIfType][ucMode].ulPMU20;

    return;
}

void TEST_TimingMode(void)
{

    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.usPage = 0;
    //Async mode
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        //reset cmd set flash to async mode
        if (NFC_STATUS_SUCCESS == HAL_NfcResetFlash(tFlashAddr.ucPU))
        {
            DBG_Printf("PU %d Reset success!\n",tFlashAddr.ucPU);
        }
    }
    /*
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<TEST_PU_NUM;tFlashAddr.ucPU++)
    {
        //Feature Address 01h: Timing mode
        HAL_NfcSetFeature(tFlashAddr.ucPU, 0x00, 0x1); //Async mode , timing mode 0
    }


    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < TEST_PU_NUM; tFlashAddr.ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
        {
            DBG_Printf("set Async Mode fail PU:%d\n", tFlashAddr.ucPU);
        }
    }
    */
    AsyncTimingInit(ONFI2,NFTM_MODE1);

    TEST_NfcSinglePlnEWR();
    DBG_Printf("Async Mode Single Plane EWR right!\n");

    //Sync mode
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        //Feature Address 01h: Timing mode
        if (NFC_STATUS_SUCCESS != HAL_NfcSetFeature(tFlashAddr.ucPU, 0x10, 0x1))
        {
            DBG_Getch();
        }
    }

    SyncTimingInit();

    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulTestPuNum; tFlashAddr.ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
        {
            DBG_Printf("set sync Mode fail PU:%d\n", tFlashAddr.ucPU);
        }
    }

    TEST_NfcSinglePlnEWR();
    DBG_Printf("Sync Mode Single Plane EWR right!\n");

}


void TEST_RESET(void)
{
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    FLASH_ADDR tFlashAddr = {0};
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        //reset
        if (NFC_STATUS_SUCCESS == HAL_NfcResetFlash(tFlashAddr.ucPU))
        {
            DBG_Printf("PU %d Reset success!\n",tFlashAddr.ucPU);
        }

    }
}
BOOL GetFeature(U8 ucPU, U32 ulData, U8 ucAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;
    tFlashAddr.ucPU = ucPU;
    if (TRUE == HAL_NfcGetFull(ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    if (TRUE == HAL_NfcGetErrHold(ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsDmaByteEn = TRUE;
    rNfcSetFeature = ulData;
    pNFCQEntry->bsByteAddr = ucAddr;
    pNFCQEntry->bsByteLength = sizeof(U32);

    HAL_NfcSetPRCQEntry(ucPU, NF_PRCQ_GETFEATURE);
    HAL_NfcCmdTrigger(&tFlashAddr, NF_PRCQ_GETFEATURE);

    return NFC_STATUS_SUCCESS;
}

void TEST_NfcNormalGetFeature(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulData = 0;
    U8  ucAddr = 1;
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        HAL_NfcPioSetFeature(tFlashAddr.ucPU, 0x10, 0x01);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
        {
            DBG_Printf("PU %d PIO set feature fail! ADDR 0x%x Value 0x%x\n",tFlashAddr.ucPU,0x10,0x2);
        }

        if (NFC_STATUS_SUCCESS != GetFeature(tFlashAddr.ucPU,ulData,ucAddr))
        {
            DBG_Printf("Get PU %d feature Fail!Addr:0x%x,Value:0x%x\n",tFlashAddr.ucPU,ucAddr,ulData);
        }
        else
        {
            DBG_Printf("Get PU %d feature success!Addr:0x%x,Value:0x%x\n",tFlashAddr.ucPU,ucAddr,ulData);
        }
    }
}

void TEST_Delay_ms(U32 ulMsCount)
{
    U32 ulLastTimeTag;
    U32 ulCurrTimeTag;
    U32 ulTargetTime;
    U8  ucTargetOvevflow = FALSE;
    U8  ucCurrOverflow   = FALSE;
    ulLastTimeTag = HAL_GetMCUCycleCount();
    ulTargetTime = ulLastTimeTag + ulMsCount * 1000 * 300;//COUNT_INoneUS;
    if (ulTargetTime < ulLastTimeTag)
    {
        ucTargetOvevflow = TRUE;
    }
    for(;;)
    {
        ulCurrTimeTag = HAL_GetMCUCycleCount();
        if (ulCurrTimeTag < ulLastTimeTag)
        {
            ucCurrOverflow = TRUE;
        }
        if (FALSE == ucTargetOvevflow)
        {
            if ((TRUE == ucCurrOverflow) || (ulCurrTimeTag >= ulTargetTime))
            {
                break;
            }
        }
        else
        {
            if ((TRUE == ucCurrOverflow) && (ulCurrTimeTag >= ulTargetTime))
            {
                break;
            }
        }
    }
}

void TEST_NfcBasicErase(void)
{
    FLASH_ADDR tFlashAddr;
    U32 ulTestPuNum;
    ulTestPuNum = TEST_NfcGetPuNum();

    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        //for (tFlashAddr.ucLun=0;tFlashAddr.ucLun<LUN_NUM;tFlashAddr.ucLun++)
        tFlashAddr.ucLun = GetLun(&tFlashAddr);
        {
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
            {
                ;
            }
            HAL_NfcFullBlockErase(&tFlashAddr);
        }
    }
    for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
    {
        //for (tFlashAddr.ucLun=0;tFlashAddr.ucLun<LUN_NUM;tFlashAddr.ucLun++)
        tFlashAddr.ucLun = GetLun(&tFlashAddr);
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
            {
                DBG_Printf("Erase fail!\n");
            }
        }
    }
}

/*
U8 TEST_NfcReadRetry(U8 Pu, U8 ReqType, U8 SecStart, U8 SecLen, U16 BuffID, NFC_RED **ppOrgRedEntry)
{
    U8  ucRetryTime,ucErrCode;
    U16 usCurDsgId;
    U32 ulCmdStatus;
    NFCQ_ENTRY *pCqEntry,CqBak;
    FLASH_ADDR PhyAddr = {0};
    NORMAL_DSG_ENTRY *pDSG;

    l_ulErrInj = rNFC(0x2b8);


    switch (l_ulErrInj)
    {
        case NF_ERR_TYPE_USUP:
        case NF_ERR_TYPE_NO_DEV:
        case NF_ERR_TYPE_ERS:
        {
            HAL_NfcFullBlockErase(&tFlashAddr);
        }break; 

        case NF_ERR_TYPE_PRG:
        {
            HAL_NfcFullPageWrite(&tFlashAddr,0,NULL);
        }break;

        case NF_ERR_TYPE_UECC:
        case NF_ERR_TYPE_RECC:
        {
            HAL_NfcPageRead(&tFlashAddr,0,SEC_PER_PG * PLN_PER_PU,0,NULL);
        }
    }
    HAL_NfcWaitStatus(0);
    if (HAL_NfcGetErrCode(0) != ucErrType)
    {
        TRACE_OUT("0xbadc0de",0,0,0);
        DBG_Getch();
    }
    
}
*/
void TEST_NfcErrInj(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    NFC_RED tRed = {0};
    ((SpareArea *)&tRed)->PageType = PageType_PMTPage;

//    HalNfcSetErrInj();
    l_ulErrInj = 0xf;
    do
    {
        for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ulTestPuNum;tFlashAddr.ucPU++)
        {

                TEST_SetErrInj(&tFlashAddr);

                if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("Pu %d Erase error!\n",tFlashAddr.ucPU);
                }

                if (HAL_NfcGetErrCode(tFlashAddr.ucPU) != l_ulErrInj)
                {
                    TRACE_OUT("0xbadc0de",0,0,0);
                    DBG_Printf("PU %d Inject Error Type:%d Actual Error Code:%d\n",tFlashAddr.ucPU,l_ulErrInj,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
                else
                {
                    HAL_NfcResetCmdQue(tFlashAddr.ucPU);
                    HAL_NfcClearINTSts(tFlashAddr.ucPU);
                    DBG_Printf("PU %d Inject Error Type:%d Pass!\n",tFlashAddr.ucPU,l_ulErrInj,HAL_NfcGetErrCode(tFlashAddr.ucPU));

                }
        }
    }while(--l_ulErrInj);
}
//--------------------------------------------------------------------------------------------------
void PrepareData(FLASH_ADDR *pFlashAddr)
{
    U32 ulAddr ;
    U32 ulSec;
    U32 ulPlnBits = 0;
    U32 ulData;
    if (TRUE != l_bIsSinglePln)
    {
        ulPlnBits = PLN_PER_PU_BITS;

    }
    ulAddr =  DRAM_START_ADDRESS + ((START_WBUF_ID + pFlashAddr->ucPU) << (PG_SZ_BITS + ulPlnBits));// + PLN_PER_PU_BITS));
    COM_MemZero((U32*)ulAddr,(SEC_PER_PG*SEC_SIZE/sizeof(U32))<<ulPlnBits);

    for (ulSec=0;ulSec<(SEC_PER_PG << ulPlnBits);ulSec++)  // 2 Pln:0-63 sector ,1 Pln:0-31 sector
    {
        ulData = CalcData(pFlashAddr->ucPU,pFlashAddr->usPage,pFlashAddr->ucLun,ulSec);   //add for print
        *(U32*)(ulAddr + ulSec*SEC_SIZE) = CalcData(pFlashAddr->ucPU,pFlashAddr->usPage,pFlashAddr->ucLun,ulSec);
       // DBG_Printf("Prepare data addr:0x%x data:0x%x\n",ulAddr + ulSec*SEC_SIZE,ulData);
    }

}

void TEST_CheckSSU(FLASH_ADDR *pFlashAddr)
{
    if ('S' != GetSsu0Status(l_ulSsuAddr[pFlashAddr->ucPU]))
    {
        TRACE_OUT(0x9badc0de,pFlashAddr->ucPU,pFlashAddr->usPage,0);
        //DBG_Getch();
        DBG_Printf("PU %d Page %d SSU0 check wrong!\n",pFlashAddr->ucPU,pFlashAddr->usPage);
    }
    else
    {
        SetSsu0Status(l_ulSsuAddr[pFlashAddr->ucPU],0);

    }

    if ('U' != GetSsu1Status(l_ulSsuAddr[pFlashAddr->ucPU]))
    {
        TRACE_OUT(0x8badc0de,pFlashAddr->ucPU,pFlashAddr->usPage,0);
        //DBG_Getch();
        DBG_Printf("PU %d Page %d SSU1 check wrong!\n",pFlashAddr->ucPU,pFlashAddr->usPage);
    }
    else
    {
        SetSsu1Status(l_ulSsuAddr[pFlashAddr->ucPU],0);
        //TEST_Delay_ms(10);
    }
    
}

void TEST_SetCs(NORMAL_DSG_ENTRY *pDSG,FLASH_ADDR *pFlashAddr)
{
    if (TRUE == TEST_NfcSsuCsEn())
    {
        U32 ulCacheStsAddr = TEST_NfcGetCacheStatusAddr(pFlashAddr);    
        pDSG->bsCacheStsEn = TRUE;
        pDSG->bsCacheStatusAddr = ulCacheStsAddr;
        pDSG->bsCacheStsData = 'X';
        l_ulCsAddr[pFlashAddr->ucPU] = ulCacheStsAddr;
        SetCacheStatus(ulCacheStsAddr,0);
    }
}


void TEST_CheckCS(FLASH_ADDR *pFlashAddr)
{
    if ('X' != GetCacheStatus(l_ulCsAddr[pFlashAddr->ucPU]))
    {
        TRACE_OUT(0x7badc0de,pFlashAddr->ucPU,pFlashAddr->usPage,0);
        //DBG_Getch();
        DBG_Printf("PU %d Page %d Cache Status check wrong!\n",pFlashAddr->ucPU,pFlashAddr->usPage);
    }
    else
    {
        SetCacheStatus(l_ulCsAddr[pFlashAddr->ucPU],0);
        DBG_Printf("PU %d Page %d CacheStatus check right! Addr offset:0x%x\n",pFlashAddr->ucPU,pFlashAddr->usPage,l_ulSsuAddr[pFlashAddr->ucPU]);
    }
}

void TEST_NfcSSUandCS(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();

    TEST_NfcSSUandCsInit();
    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
            {
                ;
            }
            HAL_NfcFullBlockErase(&tFlashAddr);
        }

        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
            {
                DBG_Printf("Block %d Erase fail!\n",tFlashAddr.usBlock);
            }
        }

        for (tFlashAddr.usPage = 0;tFlashAddr.usPage < ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                HAL_NfcFullPageWrite(&tFlashAddr,START_WBUF_ID + GetOffset(&tFlashAddr),NULL);
            }
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("PU %d Page %d Write fail!\n",tFlashAddr.ucPU,tFlashAddr.usPage);
                }
            }

            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                TEST_CheckSSU(&tFlashAddr);
                TEST_CheckCS(&tFlashAddr);
            }

            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                HAL_NfcPageRead(&tFlashAddr,0,SEC_PER_PG * PLN_PER_PU,START_RBUF_ID + GetOffset(&tFlashAddr),NULL);
            }
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    HAL_NfcResetFlash(tFlashAddr.ucPU);
                    DBG_Printf("PU %d Page %d Read fail!\n",tFlashAddr.ucPU,tFlashAddr.usPage);
                }
                TEST_CheckSSU(&tFlashAddr);
            }
        }
    }
}

void GenPartReadAddr(U16 *pSecStart ,U16 *pSecLen)
{
    U32 ulRan;
    U32 ulStart;
    ulRan = XT_RSR_CCOUNT();
    ulStart = ulRan % 62;
    *pSecStart = (U16)ulStart;
    *pSecLen = (U16)((ulRan % (62 - ulStart)) + 2);
}

void CheckData(FLASH_ADDR *pFlashAddr,U16 usSecStart,U16 usSecLen)
{
    U32 ulAddr ;
    U32 ulSec;
    U32 ulPlnBits = 0;
    U32 ulWriteAddr;
    U32 ulWriteData;
    U32 ulReadAddr;
    U32 ulReadData;
    if (TRUE != l_bIsSinglePln)
    {
        ulPlnBits = PLN_PER_PU_BITS;

    }
    ulAddr =  DRAM_START_ADDRESS + ((START_RBUF_ID + pFlashAddr->ucPU) << (PG_SZ_BITS + ulPlnBits));
    for (ulSec=usSecStart;ulSec<usSecLen;ulSec++)
    {
        ulWriteAddr = DRAM_START_ADDRESS + ((START_WBUF_ID + pFlashAddr->ucPU) << (PG_SZ_BITS + ulPlnBits)) + ulSec*SEC_SIZE;
        ulReadAddr  = DRAM_START_ADDRESS + ((START_RBUF_ID + pFlashAddr->ucPU) << (PG_SZ_BITS + ulPlnBits)) + ulSec*SEC_SIZE;
        ulWriteData  = CalcData(pFlashAddr->ucPU,pFlashAddr->usPage,pFlashAddr->ucLun,ulSec);

        if (*(U32*)(ulAddr + ulSec*SEC_SIZE)
           != CalcData(pFlashAddr->ucPU,pFlashAddr->usPage,pFlashAddr->ucLun,ulSec))
        {
            //TRACE_OUT(0x6badc0de,ulAddr,*(U32*)(ulAddr + ulSec*SEC_SIZE)
                   //  ,CalcData(pFlashAddr->ucPU,pFlashAddr->usPage,pFlashAddr->ucLun,ulSec));
            //DBG_Getch();

            DBG_Printf("Write Addr:0x%x,value:0x%x\nRead Addr:0x%x,value:0x%x\nCalData:0x%x\n",
            ulWriteAddr,*((volatile U32 *)ulWriteAddr),ulReadAddr,*((volatile U32*)ulReadAddr),ulWriteData);

        }
        else
        {
            *(U32*)(ulAddr + ulSec*SEC_SIZE) = 0;

        }

    }
}

void TEST_NfcPartialRead(void)
{
    FLASH_ADDR tFlashAddr;
    NFC_RED tRed = {0};

    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    U32 ulTestSecLen = TEST_NfcGetSecLen();
    U8 usSecStart,usSecLen;
    tFlashAddr.ucLun = 0;
    ((SpareArea *)&tRed)->PageType = PageType_PMTPage;

    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
            {
                ;
            }
            HAL_NfcFullBlockErase(&tFlashAddr);
        }

        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
            {
                DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
            }
        }

        for (tFlashAddr.usPage = 0;tFlashAddr.usPage < ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }

                PrepareData(&tFlashAddr);
                HAL_NfcFullPageWrite(&tFlashAddr,START_WBUF_ID + tFlashAddr.ucPU,NULL);
            }
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
            }
        }

        for (tFlashAddr.usPage = 0;tFlashAddr.usPage < ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                //GenPartReadAddr(&usSecStart ,&usSecLen);
                for(usSecStart=0;usSecStart<(ulTestSecLen-1);usSecStart++)
                {
                    usSecLen = ulTestSecLen-usSecStart;
                    HAL_NfcPageRead(&tFlashAddr,usSecStart,usSecLen,(U16)(START_RBUF_ID + tFlashAddr.ucPU),NULL);
                    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                    {
                        HAL_NfcResetCmdQue(tFlashAddr.ucPU);
                        HAL_NfcClearINTSts(tFlashAddr.ucPU);
                        DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                    }

                    CheckData(&tFlashAddr,usSecStart,usSecLen);
                }
            }

        }
         DBG_Printf("Block %d Partial Read right\n",tFlashAddr.usBlock);
    }
}

void TEST_NfcSinglePlnEWR(void)
{
    FLASH_ADDR tFlashAddr;
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();

    l_bIsSinglePln = TRUE;
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;

    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
            {
                ;
            }
            HAL_NfcSingleBlockErase(&tFlashAddr);

        }
        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
            {
                DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
            }
        }
        for (tFlashAddr.usPage = 0;tFlashAddr.usPage < ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                PrepareData(&tFlashAddr);
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                HAL_NfcSinglePlaneWrite(&tFlashAddr,START_WBUF_ID + tFlashAddr.ucPU,NULL);
                //DBG_Printf("Block %d Page %d Pu %d Write\n",tFlashAddr.usBlock,tFlashAddr.usPage,tFlashAddr.ucPU);

            }

            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("Pu %d write fail,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
            }
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                HAL_NfcSinglePlnRead(&tFlashAddr,0,SEC_PER_PG,FALSE,START_RBUF_ID + tFlashAddr.ucPU,NULL);
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    HAL_NfcResetCmdQue(tFlashAddr.ucPU);
                    HAL_NfcClearINTSts(tFlashAddr.ucPU);
                    DBG_Printf("Pu %d read fail,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
                CheckData(&tFlashAddr,0,SEC_PER_PG);
                DBG_Printf("Block %d Page %d PU %d Signle Plane EWR right!\n",tFlashAddr.usBlock,tFlashAddr.usPage,tFlashAddr.ucPU);
            }
        }
    }
}

void TEST_NfcByteModeRead(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    U32 ulTestBlockStart = TEST_NfcGetBlockStart();
    U32 ulTestBlockEnd   = TEST_NfcGetBlockEnd();
    U32 ulTestPageNum = TEST_NfcGetPageNum();

    U32 i,ulAddr;
    U32 *pDataAddr;
    NFC_RED tRed;
    ((SpareArea *)&tRed)->PageType = PageType_PMTPage;

    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;

    DBG_Printf("ByteModeRead start!\n");

    for (tFlashAddr.usBlock=ulTestBlockStart;tFlashAddr.usBlock<ulTestBlockEnd;tFlashAddr.usBlock++)
    {
        for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
        {
            while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
            {
                ;
            }
            HAL_NfcFullBlockErase(&tFlashAddr);
        }
        for (tFlashAddr.usPage = 0;tFlashAddr.usPage < ulTestPageNum;tFlashAddr.usPage++)
        {
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
            }

            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                //ulAddr = DRAM_DATA_BUFF_MCU1_BASE;
                ulAddr = DRAM_START_ADDRESS;

                for (i=0;i<128;i++)
                {
                    *(U8*)(ulAddr + i) = i + 1;
                }
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    ;
                }
                HAL_NfcFullPageWrite(&tFlashAddr,0,&tRed);
            }
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
                DBG_Printf("Pu %d write success!\n",tFlashAddr.ucPU);

            }

            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU))
                {
                    DBG_Printf("Pu %d cmd full!\n",tFlashAddr.ucPU);
                }
                HAL_NfcByteModeRead(&tFlashAddr,0,64,(U32 **)&pDataAddr);
            }
            for (tFlashAddr.ucPU = 0;tFlashAddr.ucPU < ulTestPuNum;tFlashAddr.ucPU++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU))
                {
                    HAL_NfcResetCmdQue(tFlashAddr.ucPU);
                    HAL_NfcClearINTSts(tFlashAddr.ucPU);
                    DBG_Printf("Pu %d,ErrType :%d\n",tFlashAddr.ucPU,HAL_NfcGetErrCode(tFlashAddr.ucPU));
                }
                DBG_Printf("Pu %d Block %d Page %d Read success!\n"
                    ,tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
            }

            if (TRUE == TEST_NfcDataCheckEn())
            {
                for (i=0;i<64;i++)
                {
                    if (*((U8*)pDataAddr + i) != i + 1 )
                    {
                        //TRACE_OUT(0x6badc0de,(U32)pRed + i,*(U8*)((U32)pRed + i),i + 1);
                        DBG_Printf("Wrong data:0x%x Addr:0x%x\n",
                        *((U8*)pDataAddr + i),(U8*)pDataAddr + i);
                        DBG_Printf("*0x1ff81100=0x%x\n",*(U32 *)0x1ff81100);
                        DBG_Getch();
                    }
                    else
                    {
                        DBG_Printf("ByteModeRead 0-64 Byte Success!\n");
                    }
                }
            }
        }
    }
}

void TEST_ReadID(void)
{
    U8  ucPU;
    U32 aID[2];
    U32 ulTestPuNum = TEST_NfcGetPuNum();
    aID[0] = 0;
    aID[1] = 0;
    for (ucPU=0;ucPU<ulTestPuNum;ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcReadId(ucPU,aID))
        {
            DBG_Printf("PU %d Read ID fail!\n",ucPU);
        }
        else
        {
            if(aID[0] == 0x3C64842C && aID[1] == 0x000000A5)
            {
                DBG_Printf("PU %d Read ID Right!\nID0:0x%x ID1:0x%x\n",ucPU,aID[0],aID[1]);
            }
            else
            {
                DBG_Printf("PU %d Read ID wrong!\nID0:0x%x ID1:0x%x\n",ucPU,aID[0],aID[1]);
            }

        }
    }
}
/*
enum PATTERN_SEL
{
    P_NORMAL_SET_FEAT = 1,
    P_PIO_SET_FEAT,
    P_ERR_INJ,
    P_SSU_CS,
    P_SINGLE_PLN,
    P_PART_READ,
    P_BYTE_MODE_READ,
    P_READ_ID
};
*/

void TEST_NfcBasicInit(void)
{
    U32 i;
    U32 ulTestPageNum = TEST_NfcGetPageNum();

    if (TRUE == TEST_NfcDramInitEn())
    {
        pRed = (volatile NFC_RED **)START_RED_POINTER_ADDR;
        for(i = 0; i < (CE_MAX*ulTestPageNum); i++)
        {
            pRed[i] = (NFC_RED *)(START_RED_ADDR_ALIGN + i * sizeof(NFC_RED));
        }
    }

    l_bIsSinglePln = FALSE;
    l_ulEnable = 0;
    l_ulErrInj = 0;
    l_ulTestPuNum = TEST_NfcGetPuNum();

}

void TEST_NfcBasic(void)
{
    #if 0  // for multi pattern burn-in
    {
        U8 ucPatternNum = 1;
        ucPatternNum = P_NORMAL_SET_FEAT;
        switch(ucPatternNum)
        {

            case P_NORMAL_SET_FEAT :
            {
                TEST_NfcNormalSetFeature();
            }
            DBG_Printf("Normal Set Feature Pattern Pass!\n");

            case P_PIO_SET_FEAT :
            {
                TEST_NfcPioSetFeature();
            }
            DBG_Printf("Pio Set Feature Pattern Pass!\n");

            case P_READ_ID :
            {
                TEST_ReadID();
            }
            DBG_Printf("Read ID Pattern Pass!\n");

            case P_PART_READ :
            {
                TEST_NfcPartialRead();
            }
            DBG_Printf("Partial Read Pattern Pass!\n");

            case P_SSU_CS :
            {
                TEST_NfcSSUandCS();
            }
            DBG_Printf("SSU & Cache Status Check Pattern Pass!\n");

            case P_SINGLE_PLN :
            {
                TEST_NfcSinglePlnEWR();
            }
            DBG_Printf("Single Plane EWR Pattern Pass!\n");

            case P_TIMING_MODE :
            {
                TEST_TimingMode();
            }
            DBG_Printf("Timing Mode Switch Pattern Pass!\n");
            break;

            case P_BYTE_MODE_READ:
            {
                TEST_NfcByteModeRead();
            }
            DBG_Printf("Byte Mode Read Pattern Pass!\n");
            break;

            case P_ERR_INJ :
            {
                TEST_NfcErrInj();
            }
            DBG_Printf("Byte Mode Read Pattern Pass!\n");
            break;

            default :
            {
                DBG_Getch();
            }
        }
    }
    #else

        #ifndef COSIM
            U32 ulSel = P_PART_READ;
        #else
            U32 ulSel = TEST_NfcGetPatternId();
        #endif

        switch(ulSel)
        {
            case P_TIMING_MODE :
            {
                TEST_TimingMode();
            }break;

            case P_NORMAL_SET_FEAT :
            {
                TEST_NfcNormalSetFeature();
            }break;

            case P_PIO_SET_FEAT :
            {
                TEST_NfcPioSetFeature();
            }break;

            case P_NORMAL_GET_FEAT :
            {
                TEST_NfcNormalGetFeature();
            }break;

            case P_ERR_INJ :
            {
                TEST_NfcErrInj();
            }break;

            case P_SSU_CS :
            {
                TEST_NfcSSUandCS();
            }break;

            case P_SINGLE_PLN :
            {
                TEST_NfcSinglePlnEWR();
            }break;

            case P_PART_READ :
            {
                TEST_NfcPartialRead();
            }break;

            case P_BYTE_MODE_READ:
            {
                TEST_NfcByteModeRead();
            }break;

            case P_READ_ID:
            {
                TEST_ReadID();
            }break;


            default :
            {
                DBG_Getch();
            }
        }

    #endif
}



