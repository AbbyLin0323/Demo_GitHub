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
Filename    : HAL_FlashDriverExt.c
Version     : Ver 1.0
Author      : Victor
Date        : 2014.09.11
Description : This file implement Flash operation interface which will be used
              in L3 error handling
Others      :
Modify      :
20120118     PeterXiu    001 first create
20120612     PeterXiu    002 add raw interface and modify red
20140911     Gavin       003 modify for optimize code
*******************************************************************************/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_Xtensa.h"
#include "HAL_MultiCore.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_FlashCmd.h"
#include "HAL_GLBReg.h"
#include "HAL_FlashCmd.h"
#include "HAL_NormalDSG.h"
#include "HAL_ParamTable.h"
#include "HAL_HostInterface.h"
#include "HAL_ParamTable.h"
#ifndef HOST_SATA
#include "HAL_HSG.h"
#include "HAL_HwDebug.h"
#endif

#ifdef SIM
#include "sim_sge.h"
#include "sim_flash_shedule.h"
#include "sim_flash_common.h"
#include "win_bootloader.h"
extern CRITICAL_SECTION g_PuBitmapCriticalSection;
#endif

#define ASYNC_MODE
#ifndef ASYNC_MODE
#define TOGGLE20
#endif

#ifdef SIM
extern CRITICAL_SECTION  g_CriticalChainNumFw;
extern void SGE_CheckChainNumComplete(U8 ucTag);
#define  GET_CHAIN_NUM_LOCK EnterCriticalSection(&g_CriticalChainNumFw)
#define  RELEASE_CHAIN_NUM_LOCK LeaveCriticalSection(&g_CriticalChainNumFw)
#else
#define  GET_CHAIN_NUM_LOCK 
#define  RELEASE_CHAIN_NUM_LOCK 
#endif

/***************************************************************************/
/* Global vars                                                             */
/***************************************************************************/
#define NFC_TIMING_IFTYPE_MAX 2
#define NFC_TIMING_MODE_MAX 4
LOCAL MCU12_VAR_ATTR ASYNC_TIMING_CONFIG g_aAsyncTConfTab[NFC_TIMING_IFTYPE_MAX][NFC_TIMING_MODE_MAX] =
{
    /* TOGGLE 2  MODE 1 ---- invalid */
    {0x814c8125,
    0x00102494,
    0x0,
    0x23000000,
    0x0,
    0x00d08900,
    0x10,

    /* TOGGLE 2  MODE 2 ---- invalid */
    0x814c8125,
    0x00102494,
    0x0,
    0x23000000,
    0x0,
    0x00d08900,
    0x0,

    /* TOGGLE 2  MODE 3 sync ---- invalid */
    0x814d01a5,
    0x2090a494,
    0x3f,
    0x23000000,
    0xffffffff,
    0x0050af00,
    0x1d0,

    /* TOGGLE 2  MODE 4 sync ---- valid */ // NFC cosim (SYNC)
    0xc14d0bbc,
    0x40d12494,
    0x3F,
    0x23000000,
    0x2c2c2c2c,
    0x0a50af00,
    0x1d0} ,    

    /* ONFI 2  MODE 1 async ---- valid */ // NFC cosim  (ASYNC)
    {0x814c810c,
    0x00102494,
    0x0,
    0x23000055,
    0x0,
    0x00d08900,
    0x110,

    /* ONFI 2  MODE 2 async ---- invalid */
    0x814c8125,
    0x00102494,
    0x0,
    0x23000055,
    0x0,
    0x00d08900,
    0x110,

    /* ONFI 2  MODE 3 ---- invalid */
    0x814c8125,
    0x00102494,
    0x0,
    0x23000000,
    0x0,
    0x00d08900,
    0x110,

    /* ONFI 2  MODE 4 ---- invalid */
    0x814c8125,
    0x00102494,
    0x0,
    0x23000000,
    0x0,
    0x00d08900,
    0x110}         
};

GLOBAL MCU12_VAR_ATTR volatile NF_HPUST_REG *pNfHpustREG;
GLOBAL MCU12_VAR_ATTR volatile NF_PPUST_REG *pNfPpustREG;
GLOBAL MCU12_VAR_ATTR volatile NF_LPUST_REG *pNfLpustREG;
GLOBAL MCU12_VAR_ATTR volatile NF_PUFSB_REG *pNfPufsbREG;
GLOBAL MCU12_VAR_ATTR volatile NF_PUACC_TRIG_REG *pNfPuaccTrigREG;

LOCAL volatile NFC_CHAIN_NUM_REG *l_NfcChainNumReg;
LOCAL volatile NFC_CHAIN_NUM_REG *l_NfcPeerChainNumReg;//used when need to set chain number to 0 for peer subsystem

#ifdef MIX_VECTOR
extern U32 NFC_GetSSU1Addr();
#endif
extern GLOBAL MCU12_VAR_ATTR U32 g_ulPuNum; 

/***************************************************************************/
/* Local Functions                                                         */
/***************************************************************************/
LOCAL void HalNfcInterfaceInitExt(void);
LOCAL void  HalNfcFeatureInitExt(void);
LOCAL U32  HalNfcFlashResetAll(void);
LOCAL void HalNfcAsyncTimingInit(U8 ucIfType, U8 ucMode);
LOCAL void HalNfcSyncTimingInit(void);

LOCAL U32  GetSubSystemLPUSTBitMap(U32 ulLPUSTBitMap);

/*------------------------------------------------------------------------------
Name: HAL_FlashInit
Description: 
    1. initialize all FW pointer to HW resource
    2. initialize PRCQ
    3. config NFC features
    4. initialize read retry value
    5. update local CE/LogicPU mapping table according to CE bitmap
    6. do NOT config Logic PU register and timing register, which is bootloader's task
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in FW init sequence
History:
    20141020    tobey   create to sync FW with BootLoader
    20141024    Gavin   adjust code strcuture
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashInit(void)
{
    HAL_NfcInterfaceInitBasic();

    HalNfcInterfaceInitExt();

    HalNfcFeatureInitExt();
  
    HAL_FlashNfcFeatureInit();

    HAL_FlashInitSLCMapping();

    HAL_FlashRetryValueInit();

    HAL_NfcPuMapping(HAL_NfcGetCEBitMap(HAL_GetMcuId()));

    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcInterfaceInitExt
Description: 
    Initialize additional FW pointer which are needed by Normal FW 
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in normal FW initialization sequence
History:
    20141020    tobey   create
    20141024    Gavin   remove SSU0/1 initialization to other function
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HalNfcInterfaceInitExt(void)
{
    /* PU Accelaration */
    pNfHpustREG     = (NF_HPUST_REG *)      HPUST_REG_BASE;
    pNfPpustREG     = (NF_PPUST_REG*)       PPUST_REG_BASE;
    pNfLpustREG     = (NF_LPUST_REG *)      LPUST_REG_BASE;    
    pNfPufsbREG     = (NF_PUFSB_REG *)      PUFSB_REG_BASE; 
    pNfPuaccTrigREG = (NF_PUACC_TRIG_REG *) PUACC_TRIG_REG_BASE;
    
    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcFeatureInitExt
Description: 
    Initialize additional features which are needed by Normal FW 
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in normal FW initialization sequence
History:
    20141024    Gavin   create. merge feature initialization to this function
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HalNfcFeatureInitExt(void)
{
    /* SSU Base Address */    
    rNfcSsu0Base = OTFB_SSU0_MCU1_BASE - OTFB_START_ADDRESS;
    rNfcSsu1Base = OTFB_SSU1_MCU1_BASE - OTFB_START_ADDRESS;

    rGlbTrfc |= 0x3;  // Enable update SSU & Cachestatus into OTFB
    rNfcRdEccErrInj |= (0xF<<19);// all channle Empty page detect enable

    return;
}

/****************************************************************************
Function  : HAL_NfcAsyncTimingInit
Input     : U8 ucIfType   ----  Interface type :ONFI2 ,TOGGLE2
Output    : U8 ucMode   ---- Timing mode : NFTM_MODE1,NFTM_MODE2, NFTM_MODE3, NFTM_MODE4
Usage     : Normal FW do not need this function. keep it for COSIM test only.

Purpose   : Set registers to configurate the basic timing parameter 
so that the NFC could work under async mode .
We may pick up a set of register configuration from a table according to
the interface type (ONFI/TOGGLE) and sub mode (1~4).
Reference :
History :
    2014/10/28  Victor Zhang
    rename HalNfcAsyncTimingInit as HAL_NfcAsyncTimingInit
    Modify local function into global function for cosim


****************************************************************************/
GLOBAL void MCU12_DRAM_TEXT HAL_NfcAsyncTimingInit(U8 ucIfType, U8 ucMode)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
    
    if (ONFI2 == ucIfType)
    {
        pNfcPgCfg->IsMicron = TRUE;
    }

    rNFC(0x4)   = g_aAsyncTConfTab[ucIfType][ucMode].ulNFC04;
    rNFC(0x8)   = g_aAsyncTConfTab[ucIfType][ucMode].ulNFC08;
    rNFC(0x2C)  = g_aAsyncTConfTab[ucIfType][ucMode].ulNFC2C;
    rNFC(0x30)  = g_aAsyncTConfTab[ucIfType][ucMode].ulNFC30;
    rNFC(0x80)  = g_aAsyncTConfTab[ucIfType][ucMode].ulNFC80;
    rNFC(0x88)  = g_aAsyncTConfTab[ucIfType][ucMode].ulNFC88;
    rPMU(0x20)  = g_aAsyncTConfTab[ucIfType][ucMode].ulPMU20;
    
    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcSyncTimingInit
Description: 
    Set timing for working under sync mode
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Normal FW do not need this function. keep it for COSIM test only.
History:
    20140917    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
LOCAL void MCU12_DRAM_TEXT HalNfcSyncTimingInit(void)
{
    REG_SET_VALUE(rNFC(0x4),3,2,7);   // Counter load value for tCAD

    REG_SET_VALUE(rNFC(0x8),2,2,22);  // DDR mode configuration. ONFI3.0/TOGGLE2.0 configuration.

    REG_SET_VALUE(rNFC(0x4),3,0,17);  // Delay from read command sent out to read data output at last.

    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcFlashResetAll
Description: 
    reset all CE,return error CE back.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Normal FW do not need this function. keep it for COSIM test only.
History:
    20140917    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
LOCAL U32 MCU12_DRAM_TEXT HalNfcFlashResetAll(void)
{
    U8 PuNumber;   
    U8 ErrCode;

    for (PuNumber = 0;PuNumber < SUBSYSTEM_PU_NUM; PuNumber++)
    {
        HAL_NfcResetFlash(PuNumber);
    }
    for (PuNumber = 0;PuNumber < SUBSYSTEM_PU_NUM; PuNumber++)
    {
        ErrCode = HAL_NfcWaitStatus(PuNumber);
        if (NFC_STATUS_SUCCESS != ErrCode)
        {
            DBG_Printf("Reset pu:%d fail\n",PuNumber);
            return ErrCode;
        }    
    }

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: GetSubSystemLPUSTBitMap
Description:
    get logic PU status bitmap for sub system.
Input Param:
    U32 ulLPUSTBitMap: logic PU status bitmap which was read from NFC register
Output Param:
    none
Return Value:
    U32: bitmap for sub system.
Usage:
    In multi-core mode, FW of MCU1/2 process different CEs. FW need to call this
    function to convert HW bitmap for sub system.
History:
    20140912    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
LOCAL U32 GetSubSystemLPUSTBitMap(U32 ulLPUSTBitMap)
{
    return (ulLPUSTBitMap >> (HAL_GetMcuId() - MCU1_ID)*SUBSYSTEM_PU_NUM) & (~(INVALID_8F << SUBSYSTEM_PU_NUM));
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetLogicPuBitMap
Description:
    get logic PU status bitmap for all subsystem.
Input Param:
    PU_BITMAP_TYPE BitMapType: specify status type to get
Output Param:
    none
Return Value:
    U32: status bitmap.
Usage:
    the returned bitmap is for FW acceleration to search PU.
History:
    20141024    Gavin    add description of this function
------------------------------------------------------------------------------*/
U32 HAL_NfcGetLogicPuBitMap(PU_BITMAP_TYPE BitMapType)
{
    U32 ulBitMap;

#ifdef SIM
    EnterCriticalSection(&g_PuBitmapCriticalSection);
#endif

    switch (BitMapType)
    {
        case LOGIC_PU_BITMAP_ERROR:
        {
            ulBitMap = pNfLpustREG->ulErrorBitMap;
        }
        break;

        case LOGIC_PU_BITMAP_IDLE:
        {
            ulBitMap = pNfLpustREG->ulIdleBitMap;
        }
        break;

        case LOGIC_PU_BITMAP_NOTFULL:
        {
            ulBitMap = ~(pNfLpustREG->ulFullBitMap);
        }
        break;

        case LOGIC_PU_BITMAP_EMPTY:
        {
            ulBitMap = pNfLpustREG->ulEmptyBitMap;
        }
        break;

        case LOGIC_PU_BITMAP_FINISH:
        {
            ulBitMap = pNfPufsbREG->ulLogPufsbReg;
        }
        break;

        default:
        {
            DBG_Printf("HAL_NfcGetLogicPuBitMap BitMapType %d ERROR\n", BitMapType);
            DBG_Getch();
        }
        break;
    }

#ifdef SIM
    LeaveCriticalSection(&g_PuBitmapCriticalSection);
#endif

    return GetSubSystemLPUSTBitMap(ulBitMap);
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetAllFlashFeature
Description:
    set features for every CE which belongs to current sub system.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL: SUCCESS = all CE set features OK. FAIL = at least one CE fail
Usage:
    before any normal Erase/Program/Read operation to Flash, call this function
    to adjust some parameter of Flash.
    Normal FW do not need this function. keep it for COSIM test only.
History:
    20140912    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
BOOL MCU12_DRAM_TEXT HAL_NfcSetAllFlashFeature(void)
{
    U8 ucPU;
//    BOOL bSuccess;

    //Feature Addresses 10h and 80h: Programmable Output Drive Strength
    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        HAL_NfcSetFeature(ucPU, 0x01, 0x80);
    }
    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU))
        {
            DBG_Printf("set feature fail PU:%d\n", ucPU);
            return FAIL;
        }
    }

    //Feature Addresses 10h and 80h: Programmable Output Drive Strength
    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        HAL_NfcSetFeature(ucPU, 0x01, 0x10);
    }
    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU))
        {
            DBG_Printf("set feature fail PU:%d\n", ucPU);
            return FAIL;
        }
    }

    //Feature Address 01h: Timing mode
    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        HAL_NfcSetFeature(ucPU, 0x10, 0x1);
    }
    for (ucPU = 0; ucPU < SUBSYSTEM_PU_NUM; ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU))
        {
            DBG_Printf("set feature fail PU:%d\n", ucPU);
            return FAIL;
        }
    }
    
    return SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcUpdLogicPUSts
Description:
    trigger nfc to update logic pu status related registers.
Input Param:
    none
Output Param:
    none
Return Value:
    none.
Usage:
    called before reading logic PU status.
History:
    20140912    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
void HAL_NfcUpdLogicPUSts(void)
{
#ifdef SIM
    //EnterCriticalSection(&g_PuBitmapCriticalSection);
    NFC_InterfaceUpdateLogicPuSts();
    //LeaveCriticalSection(&g_PuBitmapCriticalSection);
#else
    pNfPuaccTrigREG->bsTrig = 0x1;
#endif
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetFullPageRowAddr
Description: 
    set nfcq entry's all row address, row address is flash array address form FlashChip's side.
Input Param:
    NFCQ_ENTRY* pNFCQEntry: pointer to nfcq entry.
    FLASH_ADDR *pFlashAddr: flash addr  pointer.
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set a nfcq entry's row address.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
------------------------------------------------------------------------------*/
void HAL_NfcSetFullPageRowAddr(NFCQ_ENTRY* pNFCQEntry, FLASH_ADDR *pFlashAddr)
{
    U8 ucPln;
    FLASH_ADDR tFlashAddr;

    tFlashAddr = *pFlashAddr;

    for (ucPln = 0; ucPln < PLN_PER_PU; ucPln++)
    {
        tFlashAddr.bsPln = ucPln;
        pNFCQEntry->aRowAddr[ucPln] = 0xffffff & HAL_NfcGetFlashRowAddr(&tFlashAddr);
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPageRead
Description: 
    write DRAM data to flash by multi-plane(plane binding)
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U16 usBuffID: buffer id of DRAM
    NFC_RED *pRed: pointer to readundant data
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    usBuffID will be converted to DSG in this function.
    "plane" in pFlashAddr is not used
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcPageRead(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U8 ucSecLength, U16 usBuffID, NFC_RED **ppNfcRed)
{
    U16 usCurDsgId;  
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;

    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

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
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        *ppNfcRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
    }

    HAL_NfcSetFullPageRowAddr(pNFCQEntry,pFlashAddr);
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

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

/*------------------------------------------------------------------------------
Name: HAL_NfcRedOnlyRead
Description: 
    read only redundant area
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
Output Param:
    NFC_RED **ppNfcRed: pointer to address of redundant
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    used for some special opration to flash
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcRedOnlyRead(FLASH_ADDR *pFlashAddr, NFC_RED **ppNfcRed)
{
    NFCQ_ENTRY *pNFCQEntry;

    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsRedOnly = TRUE;
    pNFCQEntry->bsRedEn = TRUE;
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);
    if (NULL != ppNfcRed)
    {       
        *ppNfcRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
    }
    pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);

    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr);
    //set sector address in flash, for red-only read, set [start, length] as full logic page
    //sector data will not be read when bsRedOnly = 1
    pNFCQEntry->aSecAddr[0].bsSecStart = 0;
    pNFCQEntry->aSecAddr[0].bsSecLength = SEC_PER_PG*PLN_PER_PU;
    pNFCQEntry->bsDmaTotalLength = SEC_PER_PG*PLN_PER_PU;

    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_READ_2PLN);
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READ_2PLN);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcFullPageWrite
Description: 
    write DRAM data to flash by multi-plane(plane binding)
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U16 usBuffID: buffer id of DRAM
    NFC_RED *pRed: pointer to readundant data
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    usBuffID will be converted to DSG in this function.
    "plane" in pFlashAddr is not used
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
    20141113    tobey   fix bsRedEn problem
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcFullPageWrite(FLASH_ADDR *pFlashAddr, U16 usBuffID, NFC_RED *pNfcRed)
{
    U16 usCurDsgId;    
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;    
    NFC_RED *pTargetRed;

    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
   
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
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU,HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)pNfcRed, sizeof(NFC_RED)>>2);
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        pNFCQEntry->bsRedEn = TRUE;
    }    

    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

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

/*------------------------------------------------------------------------------
Name: HAL_NfcFullBlockErase
Description: 
    erase one block by multi-plane(plane-binding).
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    "page" and "plane" in pFlashAddr is not used
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcFullBlockErase(FLASH_ADDR *pFlashAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;
    
    tFlashAddr = *pFlashAddr;
    tFlashAddr.usPage = 0;
    
    if (TRUE != HAL_NfcIsPUAccessable(tFlashAddr.ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }
    
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(tFlashAddr.ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    

    HAL_NfcSetFullPageRowAddr(pNFCQEntry, &tFlashAddr);
#ifdef MIX_VECTOR
    pNFCQEntry->bsSsu1En = TRUE;
    pNFCQEntry->bsSsuAddr1 = NFC_GetSSU1Addr() - OTFB_SSU1_MCU1_BASE;
#endif

    HAL_NfcSetPRCQEntry(tFlashAddr.ucPU, NF_PRCQ_ERS_2PLN);
    HAL_NfcCmdTrigger(&tFlashAddr, NF_PRCQ_ERS_2PLN);

    return NFC_STATUS_SUCCESS;
}

#ifndef HOST_SATA //in SATA mode, HW can not access host memory directly
/*------------------------------------------------------------------------------
Name: HAL_NfcHostPageWrite
Description: 
    program host memory to Flash page by on-the-fly path.
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U8 ucCmdTag: host command tag
    U16 usFirstHSGId: first HSG id of host memory chain
    NFC_RED *pNfcRed: pointer to redundant data
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; 
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    caller should prepared HSG chain before call this function
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcHostPageWrite(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId, NFC_RED *pNfcRed) 
{
    NFCQ_ENTRY *pNFCQEntry;
    NFC_RED *pTargetRed;

    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }
	
    if (TRUE == HAL_SgqIsBusy(pFlashAddr->ucPU))
    {
        return NFC_STATUS_FAIL;
    }
	
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    //set redundant
    if (NULL != pNfcRed)
    {
        //copy redundant
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)pNfcRed, sizeof(NFC_RED)>>2);
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        pNFCQEntry->bsRedEn = TRUE;
    }
    
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsNcqNum = ucCmdTag;

    //inform this is host request
    pNFCQEntry->bsTrigOmEn = TRUE;
    pNFCQEntry->bsDsgEn = FALSE;
    pNFCQEntry->bsAddSgqCntEn = TRUE;

    //set flash address
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr);
    //set sector address in flash
    pNFCQEntry->aSecAddr[0].bsSecStart = 0;
    pNFCQEntry->aSecAddr[0].bsSecLength = PIPE_PG_SZ/SEC_SIZE;
    pNFCQEntry->bsDmaTotalLength = PIPE_PG_SZ/SEC_SIZE;

    //fill SGQ
    g_pNfcqForHalDebug = pNFCQEntry;
    HAL_SgqBuildEntry(ucCmdTag, usFirstHSGId, pFlashAddr->ucPU, TRUE);

    //set raw command queue
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_PRG_2PLN);

    //trigger command
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_PRG_2PLN);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcHostPageRead
Description: 
    read flash data to host memory by on-the-fly path.
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U8 ucCmdTag: host command tag
    U16 usFirstHSGId: first HSG id of host memory chain
    U8 ucSecStart: start sector in flash page
Output Param:
    NFC_RED *ppNfcRed: pointer to address of redundant data
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; FALSE means.
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    caller should prepared HSG chain before call this function
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcHostPageRead(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId,
                       U8 ucSecStart, U8 ucSecLength, NFC_RED **ppNfcRed)
{
    U32 ulHostReqLen;
    NFCQ_ENTRY *pNFCQEntry;

    //Full return FALSE;
    if (TRUE != HAL_NfcIsPUAccessable(pFlashAddr->ucPU, FALSE))
    {
        return NFC_STATUS_FAIL;
    }


    
    if (TRUE == HAL_SgqIsBusy(pFlashAddr->ucPU))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU,FALSE);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);
    
    //set redundant addr
    if (NULL != ppNfcRed)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
        *ppNfcRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(pFlashAddr->ucPU, HAL_NfcGetWP(pFlashAddr->ucPU), pFlashAddr->bsPln);
    }

    //inform this is host request
    pNFCQEntry->bsTrigOmEn = TRUE;
    pNFCQEntry->bsDsgEn = FALSE;

    pNFCQEntry->bsOntfEn = TRUE;// MixVector use OTFB write mode,so this bit should enable
    pNFCQEntry->bsNcqNum = ucCmdTag;
    //set flash address
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr);
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage);

    //set sector address in flash
    pNFCQEntry->aSecAddr[0].bsSecStart = ucSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = ucSecLength;
    pNFCQEntry->bsDmaTotalLength = ucSecLength;

    ulHostReqLen = ucSecLength<<(SEC_SIZE_BITS);

    //fill SGQ
    g_pNfcqForHalDebug = pNFCQEntry;
    HAL_SgqBuildEntry(ucCmdTag, usFirstHSGId, pFlashAddr->ucPU, FALSE);

    //set raw command queue
    HAL_NfcSetPRCQEntry(pFlashAddr->ucPU, NF_PRCQ_READ_2PLN);

    //trigger command
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_READ_2PLN);

    return NFC_STATUS_SUCCESS;
}
#endif

#ifndef HOST_SATA // in sata mode, HW do not support NFC chain number feature
/*------------------------------------------------------------------------------
Name: HAL_NfcInitChainCnt
Description: 
    init register pointer for maintaining on-the-fly program total chain number
    ( SGQ with write type )
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    called in FW init sequence
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
void HAL_NfcInitChainCnt(void)
{
    if(MCU1_ID == HAL_GetMcuId())
    {
        l_NfcChainNumReg = (NFC_CHAIN_NUM_REG*)&rNfcChainNumMcu1;
        l_NfcPeerChainNumReg = (NFC_CHAIN_NUM_REG*)&rNfcChainNumMcu2;
    }
    else if(MCU2_ID == HAL_GetMcuId())
    {
        l_NfcChainNumReg = (NFC_CHAIN_NUM_REG*)&rNfcChainNumMcu2;
        l_NfcPeerChainNumReg = (NFC_CHAIN_NUM_REG*)&rNfcChainNumMcu1;
    }
    else
    {
        DBG_Getch();
    }

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcFinishChainCnt
Description: 
    set on-the-fly program chain count for current sub system
Input Param:
    U8 ucHID: the command tag which need to set
    U16 usTotalChain: chain count
Output Param:
    none
Return Value:
    none
Usage:
    used when need to set chain number of all on-the-fly program SGQ
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
void HAL_NfcFinishChainCnt(U8 ucHID, U16 usTotalChain)
{
    NFC_CHAIN_NUM_REG tChainNumReg;
    tChainNumReg.ulChainNumReg = 0;
    tChainNumReg.bsHID = ucHID;
    tChainNumReg.bsChainNum = usTotalChain;
    tChainNumReg.bsValid = TRUE;

    HAL_HwDebugTrace(ucHID, RCD_NFC_CHAIN, NULL, usTotalChain, NULL);
    
    GET_CHAIN_NUM_LOCK;
    l_NfcChainNumReg->ulChainNumReg = tChainNumReg.ulChainNumReg;  

#ifdef SIM
    if (MCU1_ID == HAL_GetMcuId())
    {
        SGE_RecordChainNumForMCU1(ucHID, usTotalChain, TRUE);
    }
    else
    {
        SGE_RecordChainNumForMCU2(ucHID, usTotalChain, TRUE);
    }
    SGE_CheckChainNumComplete(ucHID); 
#endif

    RELEASE_CHAIN_NUM_LOCK;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcHelpFinishChainCnt
Description: 
    help to set chain count as 0 for peer subsystem
Input Param:
    U8 ucHID: the command tag which need to set
Output Param:
    none
Return Value:
    none
Usage:
    used when need to set chain number to 0 for peer subsystem
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
void HAL_NfcHelpFinishChainCnt(U8 ucHID)
{
    NFC_CHAIN_NUM_REG tChainNumReg;    
    tChainNumReg.ulChainNumReg = 0;
    tChainNumReg.bsHID = ucHID;
    tChainNumReg.bsValid = TRUE;
    
    GET_CHAIN_NUM_LOCK;
    l_NfcPeerChainNumReg->ulChainNumReg = tChainNumReg.ulChainNumReg;
    
#ifdef SIM
    if (MCU2_ID == HAL_GetMcuId())
    {
        SGE_RecordChainNumForMCU1(ucHID, 0, TRUE);
    }
    else
    {
        SGE_RecordChainNumForMCU2(ucHID, 0, TRUE);
    }
    SGE_CheckChainNumComplete(ucHID); 
#endif

    RELEASE_CHAIN_NUM_LOCK;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcStopChainCnt
Description: 
    stop NFC counting on-the-fly program and force to finish status.
Input Param:
    U8 ucHID: the command tag which need to stop
Output Param:
    none
Return Value:
    none
Usage:
    FW may need to use it for error handling
History:
    20140911    Gavin   modify to meet coding style
------------------------------------------------------------------------------*/
void HAL_NfcStopChainCnt(U8 ucHID)
{
    l_NfcChainNumReg->bsHID = ucHID;
    l_NfcChainNumReg->bsClear = TRUE;

    return;
}
#endif

/* end of HAL_FlashDriverExt.c */
