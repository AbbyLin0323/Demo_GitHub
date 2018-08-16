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
20160416     Abby        004 modify to meet 3D
*******************************************************************************/
/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "HAL_FlashDriverExt.h"
#include "HAL_HostInterface.h"
#ifndef HOST_SATA
#include "HAL_HSG.h"
#include "HAL_HwDebug.h"
#endif
#ifdef SIM
#include "sim_SGE.h"
#include "sim_flash_common.h"
#include "win_bootloader.h"
extern CRITICAL_SECTION g_PuBitmapCriticalSection;
extern CRITICAL_SECTION g_LogicLunStsCriticalSection;
#endif

/*------------------------------------------------------------------------------
    MACRO DECLARATION
------------------------------------------------------------------------------*/
#ifdef SIM
extern void SGE_CheckChainNumComplete(U8 ucTag);
#ifdef NFC_HAL_TEST
extern CRITICAL_SECTION  g_CriticalChainNumFw;
#define  GET_CHAIN_NUM_LOCK EnterCriticalSection(&g_CriticalChainNumFw)
#define  RELEASE_CHAIN_NUM_LOCK LeaveCriticalSection(&g_CriticalChainNumFw)
#endif
#else
#define  GET_CHAIN_NUM_LOCK
#define  RELEASE_CHAIN_NUM_LOCK
#endif


extern S32 g_ulTimeTag1;//Herny test time check for cache read command current trigger to next trigger
extern S32 g_ulTimeTag2;


/*------------------------------------------------------------------------------
    VARIABLE DECLARATION
------------------------------------------------------------------------------*/
/*    change g_aAsyncTConfTab to GLOBAL from LOCAL, modify by abby 200151112    */
LOCAL MCU12_DRAM_TEXT ASYNC_TIMING_CONFIG g_aAsyncTConfTab[NFC_TIMING_IFTYPE_MAX][NFC_TIMING_MODE_MAX] =
{
    /* TOGGLE 2  MODE 1 ---- invalid */  //TSB COSIM ASYNC, modify to 533
    {0x538C8108,                        //@>> TOGGLE | MLC | ASYNC
    0x0010048c,
    0x89,
    0x0BA900,//0x6BA900,
    0x0,
    0x0,
    0x0,        //12.5M

    /* TOGGLE 2  MODE 2 ---- valid */  //TSB BGA SYNC,can used in FPGA, modify to 533
    0x51808208,
    0x10248c,
    0x89,
    0x1F89a900,//0x1FD9a900,
    0x0,
    0x0,
    0x1,        //50M, only valid for ASIC

    /* TOGGLE 2  MODE 3 sync ---- valid */   //TOGGLE1.0, for 2D TLC in FPGA
    0x51808208,
    0x10248c,
    0x2089,//0x89,
    0x1F89a900,//0x1FD9a900,
    0x0,
    0x0,
    0x2,        //100M

    /* TOGGLE 2  MODE 4 sync ---- valid */ // TSB cosim (SYNC)
    0x518D03BD,
    0x0411148c,
    0x30af,//0xAF,
    0x1F89a900,//0x1FE9a900, //fast read
    0x0,
    0x2c2c2c2c,
    0x13} ,     //200M in 533 PMU

    /* ONFI 2  MODE 1 async ---- valid */ // NFC cosim, onfi ASYNC
    {0x538C8910,                        //@>> ONFI | MLC | ASYNC
    0x0010C48c,
    0x89,
    0x9a900,
    0x0,
    0x0,
    0x0,        //12.5M

    /* ONFI 2  MODE 2 sync ---- valid */  // for FPGA, NFCLK = 23M
    0x50610bc8, //0x538C8108,
    0xce119484, //0x0010248c,
    0xaf, //0x89,
    0x1fe9a900, //0x9a900,
    0x0,
    0x0,
    0x1,        //50M

    /* ONFI 2  MODE 3 ---- valid */
#ifdef FPGA                             /* FPGA 100M NVDDR */
    0x53830b90,
    0xc7117484,
    0x89,
    0x9a900,
    0x0,
    0x60606060,
    0x2,
#else                                   /* COSIM sync 100M NVDDR */
    0x53808b10,//0x538C8910,
    0x0010C48c,
    0xAF,
    0x9a900,
    0x0,
    0x2c2c2c2c,
    0x2,        //100M
#endif

    /* ONFI 2  MODE 4 ---- valid */
#ifdef FPGA                             /* FPGA NVDDR2 100M */
    0x50610bc8,
    0xce119484,
    0xaf,
    0x1fc9a900,
    0x0,
    0x0,
#else                                   /* COSIM NVDDR2 200M */
    0x53AC8BC8,
    0xCE11948C,
    0x30af,//0xAF,
    0xF89a900,//0xFE9a900,
    0x0,
    0x2c2c2c2c,
#endif
    0x13} ,     //200M, ECC_CLK = 200M
};

/***************************************************************************/
/*                 VARIABLES DECLARATION                                      */
/***************************************************************************/
GLOBAL MCU12_VAR_ATTR volatile NF_HPUST_REG *pNfHpustREG;
GLOBAL MCU12_VAR_ATTR volatile NF_PPUST_REG *pNfPpustREG;
GLOBAL MCU12_VAR_ATTR volatile NF_LPUST_REG *pNfLpustREG;
GLOBAL MCU12_VAR_ATTR volatile NF_LLUNST_REG *pNfcLlunstREG;
GLOBAL MCU12_VAR_ATTR volatile NF_LLUN_SW_BITMAP_REG *pNfcLLunSwBitmap;

extern GLOBAL MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[FLASH_PRCQ_CNT];
#ifdef HAL_NFC_TEST
extern GLOBAL volatile BOOL g_ErrInjEn;
extern GLOBAL NFC_ERR_INJ g_tErrInj;
#endif

LOCAL volatile NFC_CHAIN_NUM_REG *l_NfcChainNumReg;
LOCAL volatile NFC_CHAIN_NUM_REG *l_NfcPeerChainNumReg;//used when need to set chain number to 0 for peer subsystem
LOCAL U8 MCU12_DRAM_TEXT HAL_NfcReadRetry(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, U8 ReqType);

LOCAL const U32 l_kXorRedunBaseAddrMask = 0x0001FFFF;
LOCAL const U32 l_kConcatLastCwAndRedunPos = 6;

/***************************************************************************/
/* Local Functions                                                         */
/***************************************************************************/
/***************************************************************************/
/*       EXTERN Functions                                                  */
/***************************************************************************/

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
void MCU12_DRAM_TEXT HalNfcInterfaceInitExt(void)
{
    /* PU Accelaration */
    pNfHpustREG     = (NF_HPUST_REG *)      NF_HPUST_REG_BASE;
    pNfPpustREG     = (NF_PPUST_REG*)       NF_PPUST_REG_BASE;
    pNfLpustREG     = (NF_LPUST_REG *)      NF_LPUST_REG_BASE;
    pNfcLlunstREG   = (NF_LLUNST_REG *)     NF_LLUNST_REG_BASE;
    pNfcLLunSwBitmap= (NF_LLUN_SW_BITMAP_REG *) NF_LLUNST_SW_REG_BASE;

    return;
}

/*------------------------------------------------------------------------------
Name: HalNfcFeatureInitExt
Description:
    Initialize additional features which are needed by Normal FW
    SSU update address should set in NFCQ, and support dynamic update to OTFB/DRAM
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
    20151030    abby    move SSU reg base addr init to HAL_NfcInterfaceInitBasic, invalid SSU update OTFB
    20151203    abby    add new feature control variable in VT3533
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HalNfcFeatureInitExt(void)
{
    //Enable Cachestatus(bit1) into OTFB, bit0 SSU enable single invalid
    rGlbTrfc |= 0x2;

    HAL_NfcSetSsuInOtfbBase(OTFB_SSU0_MCU12_SHARE_BASE);
    HAL_NfcSetSsuInDramBase(DRAM_SSU0_MCU12_SHARE_BASE);
    HAL_NfcSetOtfbAdsBase(OTFB_START_ADDRESS, MCU2_ISRAM_BASE, SRAM0_START_ADDRESS);

    return;
}

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
    20151118    abby    support single MCU arch
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_FlashInit(void)
{
    HAL_DecStsInit();

    HAL_NfcInterfaceInitBasic();

    HalNfcInterfaceInitExt();

    HAL_FlashNfcFeatureInit();

    /*    add 533 NFC new feature control variables init */
    HalNfcFeatureInitExt();

    HAL_FlashInitSLCMapping();
    
#ifdef HAL_NFC_TEST
    /* In normal boot case, Logic PU setting has been finished in boot loader script. */
    HAL_NfcSetLogicPUReg(HAL_NfcGetPhyPUBitMap());
#endif

    HAL_NfcBuildPuMapping(HAL_NfcGetPhyPUBitMap());

    return;
}

/****************************************************************************
Function  : BL_NfcRegInit_Substitute
Input     : U8 ucIfType: TOGGLE2/ONFI
            U8 ucMode: NFTM_MODE1-3
Output    :

Purpose   : Replace bootloader init NFC IRS as default for simulation/fpga
Reference :
****************************************************************************/
void MCU12_DRAM_TEXT BL_NfcRegInit_Substitute(U8 ucIfType, U8 ucMode)
{
    rNfcPgCfg             = 0xc1806008;   //0xc1004208;
    rNfcOtfbAdsBase       = 0x020001ff;

    /*    Timing related registers:TOGGLE2    */
    if (NFTM_MODE1 == ucMode)
    {
        HAL_NfcAsyncTimingInit(ucIfType, ucMode);
    }
    else
    {
        HAL_NfcSyncTimingInit(ucIfType, ucMode);
    }

    HAL_NfcSetRedInDramBase(DRAM_RED_MCU12_SHARE_BASE);

    return;
}

#if (defined(FLASH_L95) ||defined(FLASH_3D_MLC)|| defined(FLASH_INTEL_3DTLC))
/****************************************************************************
Function      : BL_FlashInit_Substitute
Input         :
Output        :
Description   : replace bootloader to init flash hw interface
Reference     :
History       :
20151112    abby    create
****************************************************************************/
LOCAL void BL_FlashInit_Substitute(void)
{
    U32 ulPhyPUMap = 0;
    U32 ulPuCnt = 0;
    U32 ulStatus;
    U32 aID[2] = {0,0};
    FLASH_ADDR tFlashAddr = { 0 };
    U32 aFeature[2] = { 0, 0 };
    U8 ucAddr, ucData;

    /*    Default PU mapping  */
    HAL_NfcBuildPuMapping(INVALID_8F);
    HAL_NfcSetLogicPUReg(INVALID_8F);

    /*    Reset all flash  */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        HAL_NfcResetFlash(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);

        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Reset Fail!\n", tFlashAddr.ucPU);
        }
    }

    /*    Read ID to set PhyPU mapping    */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        /*    read ID  */
        HAL_NfcReadFlashId(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Read ID fail!\n", tFlashAddr.ucPU);
        }

        /*    Get ID from SRAM  */
        ulStatus = HAL_DecSramGetFlashId(&tFlashAddr, aID);

        if ((0 != aID[0]) && (0 != aID[1]))
        {
            ulPhyPUMap |= (1 << tFlashAddr.ucPU);
            ulPuCnt++;
            DBG_Printf("PU %d Read ID[0] = 0x%x ID[1] = 0x%x!\n", tFlashAddr.ucPU, aID[0], aID[1]);
        }
    }
    /*    Update PU mapping  */
    HAL_NfcBuildPuMapping(ulPhyPUMap);
    HAL_NfcSetLogicPUReg(ulPhyPUMap);

    /*    Set feature  */
    //Feature Addresses 10h and 80h: Output Drive Strength
    ucAddr = 0x10;
    ucData = 0x3;
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr%d Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
    }
    ucAddr = 0x80;
    ucData = 0x3;
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr%d Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
    }
#ifdef FLASH_3D_MLC
    ucAddr = 0x2;
#ifdef NVDDR2
    ucData = 0x16;
#else
    ucData = 0x0;
#endif
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr%d Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
    }
#endif
    //Feature Addresses 01h: Timing Mode;Data Async: 0x40; Sync NVDDR 100M: 0x55; Sync NVDDR2 200M: 0x67
    ucAddr = 0x1;
    ucData = 0x40;
#ifdef NVDDR
    ucData = 0x55;
    //ucData = 0x57;
#endif
#ifdef NVDDR2
    ucData = 0x60;
#endif

    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr%d Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
    }

    /* switch to sync mode */
#ifdef NVDDR
    HAL_NfcSyncTimingInit(ONFI2, NFTM_MODE3); //FPGA NVDDR 100M
#elif defined(NVDDR2)
    HAL_NfcSyncTimingInit(ONFI2, NFTM_MODE4); //FPGA NVDDR2 100M
#endif
#ifdef NVDDR2
    /*  patch FPGA IO delay  */
    rNfcOtfbAdsBase &= (~(0x3 << 30));
    rNfcOtfbAdsBase |= (0x1 << 30);     //NFIO_SEL
#endif

#ifdef FLASH_INTEL_3DTLC
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        /* step1: set feature, User Selectable Trim Profile, 85h/03h data, 00-2-pass MLC;01-1-pass MLC;03-TLC */
        ucAddr = 0x85;
        ucData = 0x03;
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);

        /* step2: sync reset to make flash reload trim file from ROM block */
        HAL_NfcSyncResetFlash(&tFlashAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("SYNC Reset PU:%d fail!\n", tFlashAddr.ucPU);
        }
        DBG_Printf("SYNC Reset PU:%d OK!\n", tFlashAddr.ucPU);

        /* step3: set feature, enable internal scramble */
        ucAddr = 0x92;
        ucData = 0x1;

        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr 0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);

    }

    /*  3D patch  */
    rNfcModeConfig &= (~(0x1<<31));
    rNfcModeConfig |= (0x1<<31);        //insert ccl enable

    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert

    rNfcPgCfg &= (~(0x1<<29));
    rNfcPgCfg |= (0x1<<29);             //ce hold enable
#endif

#ifdef FLASH_3D_MLC
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        /* step1: set feature, disable internal scramble, 92h/00h data */
        ucAddr = 0x92;
        ucData = 0x0;
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);

        /* step2: set feature, switch to 1-pass mode, 85h/01h data */
        ucAddr = 0x85;
        ucData = 0x1;
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr 0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);

        /* step3: sync reset to make flash reload trim file from ROM block */
        HAL_NfcSyncResetFlash(&tFlashAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("SYNC Reset PU:%d fail!\n", tFlashAddr.ucPU);
        }
        DBG_Printf("SYNC Reset PU:%d OK!\n", tFlashAddr.ucPU);
    }

    /*  3D patch  */
    rNfcModeConfig &= (~(0x1<<31));
    rNfcModeConfig |= (0x1<<31);        //insert ccl enable

    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert, option

    rNfcPgCfg &= (~(0x1<<29));
    rNfcPgCfg |= (0x1<<29);             //ce hold enable

#endif

    return;
}
#elif defined(FLASH_TLC)//toggle1.0
/****************************************************************************
Function      : BL_FlashInit_Substitute
Input         :
Output        :

Description    : replace bootloader to init flash hw interface
Reference     :
History        :
20151112    abby    create
****************************************************************************/
LOCAL void BL_FlashInit_Substitute(void)
{
    U32 ulPhyPUMap = 0;
    U32 ulPuCnt = 0;
    U32 ulStatus;
    U32 aID[2];
    FLASH_ADDR tFlashAddr = { 0 };
    U32 aFeature[2] = { 0, 0 };

    /*    Default PU mapping  */
    HAL_NfcBuildPuMapping(INVALID_8F);
    HAL_NfcSetLogicPUReg(INVALID_8F);

    /*    Reset all flash  */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        HAL_NfcResetFlash(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);

        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Reset Fail!\n", tFlashAddr.ucPU);
        }
    }

    /*    Read ID to set PhyPU mapping    */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        /*    read ID  */
        HAL_NfcReadFlashId(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Read ID fail!\n", tFlashAddr.ucPU);
        }

        /*    Get ID from SRAM  */
        ulStatus = HAL_DecSramGetFlashId(&tFlashAddr, aID);

        if ((0 != aID[0]) && (0 != aID[1]))
        {
            ulPhyPUMap |= (1 << tFlashAddr.ucPU);
            ulPuCnt++;
            DBG_Printf("PU %d Read ID[0] = 0x%x ID[1] = 0x%x!\n", tFlashAddr.ucPU, aID[0], aID[1]);
        }
    }
    /*    Update PU mapping  */
    HAL_NfcBuildPuMapping(ulPhyPUMap);
    HAL_NfcSetLogicPUReg(ulPhyPUMap);

    //Feature Addresses 10h: Output Drive Strength
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, 0x06, 0x10);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d\n", tFlashAddr.ucPU);
        }
    }
    /*    Set feature, switch to sync mode   */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, 0x0, 0x80);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d\n", tFlashAddr.ucPU);
        }
    }
    HAL_NfcSyncTimingInit(TOGGLE2, NFTM_MODE3);//toggle1.0 mode only for FPGA, remove when bootload prepared

    return;
}

#else //TSB flash
/****************************************************************************
Function      : BL_FlashInit_Substitute
Input         :
Output        :

Description    : replace bootloader to init flash hw interface
Reference     :
History        :
20151112    abby    create
****************************************************************************/
LOCAL void BL_FlashInit_Substitute(void)
{
    U32 ulPhyPUMap = 0;
    U32 ulPuCnt = 0;
    U32 ulStatus;
    U32 aID[2];
    FLASH_ADDR tFlashAddr = { 0 };
    U32 aFeature[2] = { 0, 0 };

    /*    Set NFC Reg  */
    //rNfcPgCfg                 = 0xc100c208;//0xc1004208;

    /*    Default PU mapping  */
    HAL_NfcBuildPuMapping(INVALID_8F);
    HAL_NfcSetLogicPUReg(INVALID_8F);

    /*    Reset all flash  */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        HAL_NfcResetFlash(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);

        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Reset Fail!\n", tFlashAddr.ucPU);
        }
    }

    /*    Read ID to set PhyPU mapping    */
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        /*    read ID  */
        HAL_NfcReadFlashId(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Read ID fail!\n", tFlashAddr.ucPU);
        }

        /*    Get ID from SRAM  */
        ulStatus = HAL_DecSramGetFlashId(&tFlashAddr, aID);

        if ((0 != aID[0]) && (0 != aID[1]))
        {
            ulPhyPUMap |= (1 << tFlashAddr.ucPU);
            ulPuCnt++;
            DBG_Printf("PU %d Read ID[0] = 0x%x ID[1] = 0x%x!\n", tFlashAddr.ucPU, aID[0], aID[1]);
        }
    }
    /*    Update PU mapping  */
    HAL_NfcBuildPuMapping(ulPhyPUMap);
    HAL_NfcSetLogicPUReg(ulPhyPUMap);

    /* switch to sync mode */
    rNfcPgCfg = 0xc100c288;//0xc1004208;
    rNfcModeConfig = 0x1FE9a900;
    rNfcTCtrl0 = 0x518D03BD;//0x51808208;
    rNfcTCtrl0 &= (~(0x7 << 17));
    rNfcTCtrl0 |= (0 << 17);
    /*    Set feature  */
    //Feature Addresses 10h: Output Drive Strength
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < ulPuCnt; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, 0x06, 0x10);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d\n", tFlashAddr.ucPU);
        }
    }
    rNfcPgCfg = 0xc100c188;
    rNfcModeConfig = 0x1FD9a900;
    rNfcTCtrl0 = 0x51808208;

    return;
}
#endif


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
    20151112    abby    modify to meet VT3533 NFC IRS, rename from HalNfcAsyncTimingInit
****************************************************************************/
void MCU12_DRAM_TEXT HAL_NfcAsyncTimingInit(U8 ucIfType, U8 ucMode)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;

    if (ONFI2 == ucIfType)
    {
        pNfcPgCfg->bsIsMicron = TRUE;
    }
    pNfcPgCfg->bsDDRCfg = FALSE;
    pNfcPgCfg->bsDDRHfCfg = FALSE;

    rNfcTCtrl0      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl0;
    rNfcTCtrl1      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl1;
    rNfcTCtrl2      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl2;
    rNfcModeConfig  = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcModeConfig;
    rNfcIODelayCtrl = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcIODelayCtrl;
    rNfcDDrDlyComp  = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcDDrDlyComp;
    rPMU(0x20)      = g_aAsyncTConfTab[ucIfType][ucMode].ulPMU20;

    return;
}

/****************************************************************************
Function  : HAL_NfcSyncTimingInit
Input     : U8 ucIfType   ----  Interface type :ONFI2 ,TOGGLE2
Output    : U8 ucMode   ---- Timing mode : NFTM_MODE1,NFTM_MODE2, NFTM_MODE3, NFTM_MODE4
Usage     : Normal FW do not need this function. keep it for COSIM or test only.

Purpose   : Set registers to configurate the basic timing parameter
            so that the NFC could work under sync mode .
Reference :
History :
    20160222    abby    create
****************************************************************************/
void MCU12_DRAM_TEXT HAL_NfcSyncTimingInit(U8 ucIfType, U8 ucMode)
{
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;

    if (ONFI2 == ucIfType)
    {
        pNfcPgCfg->bsIsMicron = TRUE;
    }
    #ifdef NVDDR
        pNfcPgCfg->bsCeHoldEn       = FALSE;
        pNfcPgCfg->bsDDRCfg         = TRUE;
        pNfcPgCfg->bsDDRHfCfg       = FALSE;
        pNfcPgCfg->bsLdpcNvddrMode  = TRUE;
    #else//NVDDR2 or TOGGLE
        pNfcPgCfg->bsIsMicron       = FALSE;
        pNfcPgCfg->bsCeHoldEn       = TRUE;
        pNfcPgCfg->bsDDRCfg         = TRUE;
        pNfcPgCfg->bsDDRHfCfg       = TRUE;
        pNfcPgCfg->bsLdpcNvddrMode  = FALSE;
    #endif

    rNfcTCtrl0      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl0;
    rNfcTCtrl1      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl1;
    rNfcTCtrl2      = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcTCtrl2;
    rNfcModeConfig  = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcModeConfig;
    rNfcIODelayCtrl = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcIODelayCtrl;
    rNfcDDrDlyComp  = g_aAsyncTConfTab[ucIfType][ucMode].ulNfcDDrDlyComp;
    rPMU(0x20)      = g_aAsyncTConfTab[ucIfType][ucMode].ulPMU20;

    return;
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
    20151204    abby     remove GetSubSystemLPUSTBitMap function
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
            ulBitMap = pNfLpustREG->ulNotFullBitMap;
        }
        break;

        case LOGIC_PU_BITMAP_EMPTY:
        {
            ulBitMap = pNfLpustREG->ulEmptyBitMap;
        }
        break;

        default:
        {
            //DBG_Printf("HAL_NfcGetLogicPuBitMap BitMapType %d ERROR\n", BitMapType);
            DBG_Getch();
        }
        break;
    }

#ifdef SIM
    LeaveCriticalSection(&g_PuBitmapCriticalSection);
#endif

    return ulBitMap;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetLLunStsBitMap
Description:
    get logic LUN status bitmap for all subsystem.
Input Param:
    U8 ucPU: PU number
    U8 BitMapType: specify status type to get
Output Param:
    none
Return Value:
    U32: status bitmap. 32 bits = 8 Logic PU * 4 LUN
Usage:
    the returned bitmap is for FW acceleration to search LUN.
History:
    20151103    abby    create
------------------------------------------------------------------------------*/
U32 HAL_NfcGetLLunStsBitMap(U8 ucPU, U8 ucBitMapType)
{
    U8 ucRegGroupID;
    U8 ucRegOffSet;
    U32 ulBitMap;

    ucRegGroupID = ucPU / (32 / NFC_LUN_MAX_PER_PU);  //each reg is consist of 8 Logic PU / 32 LUN bitmap
    ucRegOffSet = (ucPU % 8) * 4;   // each logic PU have 4 bit represent 4 LUN

#ifdef SIM
    EnterCriticalSection(&g_LogicLunStsCriticalSection);
#endif

    switch (ucBitMapType)
    {
        case LOGIC_LUN_BITMAP_ERROR:
        {
            ulBitMap = pNfcLlunstREG->aErrorReg[ucRegGroupID].ulBitMap;
        }break;

        case LOGIC_LUN_BITMAP_IDLE:
        {
            ulBitMap = pNfcLlunstREG->aIdleReg[ucRegGroupID].ulBitMap;
        }break;

        case LOGIC_LUN_BITMAP_NOTFULL:
        {
            ulBitMap = pNfcLlunstREG->aNotFullReg[ucRegGroupID].ulBitMap;
        }break;

        case LOGIC_LUN_BITMAP_EMPTY:
        {
            ulBitMap = pNfcLlunstREG->aEmptyReg[ucRegGroupID].ulBitMap;
        }break;

        default:
        {
            DBG_Getch();
        }
    }

#ifdef SIM
    LeaveCriticalSection(&g_LogicLunStsCriticalSection);
#endif

    return ((ulBitMap >> ucRegOffSet) & MULTI_VALUE_1(NFC_LUN_MAX_PER_PU));
}

U32 HAL_NfcGetLLunSwBitMap(U8 ucLunInPU, U8 ucBitMapType)
{
    U32 ulBitMap;

#ifdef SIM
    EnterCriticalSection(&g_aCSUptTLunSwBitmap);
#endif

    switch (ucBitMapType)
    {
        case LOGIC_LUN_BITMAP_ERROR:
        {
            ulBitMap = pNfcLLunSwBitmap->aErrorBitmap[ucLunInPU];
        }break;

        case LOGIC_LUN_BITMAP_IDLE:
        {
            ulBitMap = pNfcLLunSwBitmap->aIdleBitmap[ucLunInPU];
        }break;

        case LOGIC_LUN_BITMAP_NOTFULL:
        {
            ulBitMap = pNfcLLunSwBitmap->aNotFullBitmap[ucLunInPU];
        }break;

        case LOGIC_LUN_BITMAP_EMPTY:
        {
            ulBitMap = pNfcLLunSwBitmap->aEmptyBitmap[ucLunInPU];
        }break;

        default:
        {
            DBG_Getch();
        }
    }

#ifdef SIM
    LeaveCriticalSection(&g_aCSUptTLunSwBitmap);
#endif

    return ulBitMap;
}

/*------------------------------------------------------------------------------
Name: HAL_ConfigSsu0
Description:
Input Param:
Output Param:
none
Return Value:
Usage:
History:
20151130   abby    create
------------------------------------------------------------------------------*/
void HAL_ConfigSsu0(NFCQ_ENTRY *pNFCQEntry, U16 usSsu0Addr, BOOL bOntf)
{
    pNFCQEntry->bsSsu0En = TRUE;
    pNFCQEntry->bsSsu0Ontf = bOntf;      //bOntf = 1: update to OTFB
    pNFCQEntry->bsSsu0Addr = usSsu0Addr;
    pNFCQEntry->bsSsu0Data = 0;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_ConfigSsu1
Description:
Input Param:
Output Param:
none
Return Value:
Usage:
History:
20151130   abby    create
------------------------------------------------------------------------------*/
void HAL_ConfigSsu1(NFCQ_ENTRY *pNFCQEntry, U16 usSsu1Addr, BOOL bOntf)
{
    pNFCQEntry->bsSsu1En = TRUE;
    pNFCQEntry->bsSsu1Ontf = bOntf;
    pNFCQEntry->bsSsu1Addr = usSsu1Addr;
    pNFCQEntry->bsSsu1Data = 0;// TEST_SSU1_DATA;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_ConfigCacheStatus
Description:
Input Param:
Output Param:
none
Return Value:
Usage:
History:
20151130   abby    create
------------------------------------------------------------------------------*/
void HAL_ConfigCacheStatus(NORMAL_DSG_ENTRY *pDSG, U32 ulCsAddr)
{
    U32 ulBaseAddr;
    ulBaseAddr = OTFB_TLG_MCU012_SHARE_BASE - OTFB_START_ADDRESS;

    pDSG->bsCacheStatusAddr = ulBaseAddr + ulCsAddr;
    pDSG->bsCsInDram = FALSE;
    pDSG->bsCacheStsData = 0;// TEST_CACHE_STS_DATA;
    pDSG->bsCacheStsEn = TRUE;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcSetFullPageRowAddr
Description:
    set nfcq entry's all row address, row address is flash array address form FlashChip's side.
Input Param:
    NFCQ_ENTRY* pNFCQEntry: pointer to nfcq entry.
    FLASH_ADDR *pFlashAddr: flash addr  pointer.
    BOOL bMultiPageMode:1-for 3D flash 128K request, need config high page
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to set a nfcq entry's row address.
History:
    20140909    Tobey   moved from HAL_FlashDriver.c and update code
    20160416    abby    add 3D flash support
------------------------------------------------------------------------------*/
void HAL_NfcSetFullPageRowAddr(NFCQ_ENTRY* pNFCQEntry, FLASH_ADDR *pFlashAddr, BOOL bMultiPageMode)
{
    U8 ucPln;
    U8 ucRowAddr = 0;
    FLASH_ADDR tFlashAddr = { 0 };

    tFlashAddr = *pFlashAddr;
    
    for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
    {
        tFlashAddr.bsPln = ucPln;
        pNFCQEntry->atRowAddr[ucRowAddr].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
        if (0 == tFlashAddr.usPage)
            DBG_Printf("PU%d Blk%d PLN%d PG%d RowAddr%d = 0x%x\n", pFlashAddr->ucPU, tFlashAddr.usBlock, tFlashAddr.bsPln, tFlashAddr.usPage, ucRowAddr, pNFCQEntry->atRowAddr[ucRowAddr].bsRowAddr);
        ucRowAddr++;
    }

    /*  config high page row addr  */
    if (bMultiPageMode)
    {
#ifdef FLASH_INTEL_3DTLC
        //High page row addr
        if(EXTRA_PAGE == HAL_GetFlashPairPageType(tFlashAddr.usPage))
        {
#ifdef FLASH_IM_3DTLC_GEN2
            tFlashAddr.usPage = tFlashAddr.usPage + 1;
#else
            tFlashAddr.usPage = HAL_GetHighPageIndexfromExtra(tFlashAddr.usPage);
#endif
        }
        else
        {
            tFlashAddr.usPage++;
        }
#else
        tFlashAddr.usPage++;
#endif

        for (ucPln = 0; ucPln < PLN_PER_LUN; ucPln++)
        {
            tFlashAddr.bsPln = ucPln;
            pNFCQEntry->atRowAddr[ucRowAddr].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
            ucRowAddr++;
        }
    }
    
    return;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcFullBlockErase
Description:
    erase one block by multi-plane(plane-binding).
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    BOOL bTLCMode : 1: TLC; 0:SLC or MLC
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
    20151106    abby    modify to meet VT3533
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcFullBlockErase(FLASH_ADDR *pFlashAddr, BOOL bTLCMode)
{
#ifdef FLASH_YMTC_3D_MLC
    return;
#endif
    U8 ucPU, ucLun;
    U8 ucReqType;
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr = { 0 };

    tFlashAddr = *pFlashAddr;
    /* force page = 0 in erase to avoid flash abnormal response */
    tFlashAddr.usPage = 0;
    ucPU  = tFlashAddr.ucPU;
    ucLun = tFlashAddr.ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    if(FALSE == bTLCMode)
    {
        ucReqType = NF_PRCQ_ERS_MULTIPLN;
    }
    else
    {
        ucReqType = NF_PRCQ_TLC_ERS_MULTIPLN;
    }

#ifdef HAL_NFC_TEST
    if(g_ErrInjEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, &g_tErrInj);
    }
#endif

    HAL_NfcSetFullPageRowAddr(pNFCQEntry, &tFlashAddr, FALSE);

    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    HAL_NfcCmdTrigger(&tFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcFullPageWrite
Description:
    for NFC driver test
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    NFC_PRG_REQ_DES *pWrReq: write req descriptor
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success;
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    usBuffID will be converted to DSG in this function.
    "plane" in pFlashAddr is not used
History:
    20151127    abby    support RED dynamically update to DRAM or OTFB
    20160108    abby    add SLC/TLC mode support
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcFullPageWrite(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq)
{
    U8  ucPU, ucLun, ucPln, ucRedPlnNum, ucReqType;
    U16 usCurDsgId, usPage;
    U32 ulPgSz;
    NFCQ_ENTRY *pNFCQEntry;
    NFC_RED *pTargetRed;
    NORMAL_DSG_ENTRY * pDSG;
    NFC_SCR_CONF eScrType;
    BOOL bMultiRowAddr;
    BOOL bRedOntf = pWrReq->bsRedOntf;

    ucPU   = pFlashAddr->ucPU;
    ucLun  = pFlashAddr->ucLun;
    ucPln  = pFlashAddr->bsPln;
    usPage = pFlashAddr->usPage;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (FALSE == HAL_NfcIsPairPageAccessable(usPage))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    if(FALSE == pWrReq->bsTlcMode)//SLC mode
    {
        eScrType        = NORMAL_SCR;
        ulPgSz          = PHYPG_SZ << PLN_PER_LUN_BITS;
        ucRedPlnNum     = PLN_PER_LUN;
        bMultiRowAddr   = FALSE;
        ucReqType       = NF_PRCQ_PRG_MULTIPLN;
        //DBG_Printf("MUL-PLN SLC mode WR page%d ScrType%d reqType%d RedPlnLen%d pageSize%dKB\n",pFlashAddr->usPage, eScrType, ucReqType, ucRedPlnNum, ulPgSz/1024);
    }
    else//TLC mode
    {
        if(HIGH_PAGE == HAL_GetFlashPairPageType((usPage+1)%PG_PER_BLK))
        {
            ucReqType       = NF_PRCQ_TLC_PRG_MULTIPLN;  
            eScrType        = TLC_RW_TWO_PG;
            ulPgSz          = LOGIC_PIPE_PG_SZ * PGADDR_PER_PRG * INTRPG_PER_PGADDR;
            ucRedPlnNum     = PLN_PER_LUN * PGADDR_PER_PRG * INTRPG_PER_PGADDR;
            bMultiRowAddr   = TRUE;
        }
        else
        {
            ucReqType       = NF_PRCQ_TLC_PRG_MULTIPLN_LOW_PG;
            eScrType        = NORMAL_SCR;
            ulPgSz          = PHYPG_SZ << PLN_PER_LUN_BITS;
            ucRedPlnNum     = PLN_PER_LUN;
            bMultiRowAddr   = FALSE;
        }
        //DBG_Printf("MUL-PLN WR blk%d page%d ScrType%d reqType%d RedPlnLen%d pageSize%dKB\n",pFlashAddr->usBlock, pFlashAddr->usPage, eScrType, ucReqType, ucRedPlnNum, ulPgSz/1024);
    }

    pNFCQEntry->bsDsgEn = TRUE;
    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);

    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsLbaChk  = pWrReq->bsLbaChkEn;
    pNFCQEntry->bsEMEn    = pWrReq->bsEmEn;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pWrReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pWrReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    /*  Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNFCQEntry->bsDmaTotalLength = ulPgSz >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pWrReq->pNfcRed)
    {
        pNFCQEntry->bsRedOntf = bRedOntf;
        pNFCQEntry->bsRedEn = TRUE;
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, bRedOntf);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)pWrReq->pNfcRed, RED_SZ_DW * ucRedPlnNum);
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, bRedOntf);
    }

    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_FULL_PLN, 0, eScrType);

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pWrReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pWrReq->bsSsu0Addr, pWrReq->bsSsu0Ontf);
    }
    if(FALSE != pWrReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pWrReq->bsSsu1Addr, pWrReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr, bMultiRowAddr);

    pDSG->bsLast = TRUE;
    pDSG->bsXferByteLen = ulPgSz;
    pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pWrReq->bsWrBuffId, 0, LOGIC_PIPE_PG_SZ_BITS);

    /*  DSG DW2-3: cache status  */
    if(FALSE != pWrReq->bsCSEn)
    {
        HAL_ConfigCacheStatus(pDSG, pWrReq->bsCsAddrOff);
    }
    HAL_SetNormalDsgSts(usCurDsgId,1);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);
    
    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcPageRead
Description:
    write DRAM data to flash by multi-plane(plane binding)
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    NFC_READ_REQ_DES *pRdReq:descriptor of read req
Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success;
        NFC_STATUS_FAIL means command sent fail.
Usage:
    usBuffID will be converted to DSG in this function.
    "plane" in pFlashAddr is not used
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
    20151106    abby    modify to meet VT3533, dynamic update RED to OTFB/DRAM
    20160416    abby    add 3D MLC support
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcPageRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun, ucPln, ucReqType, ucPageType;
    U16 usCurDsgId, usPage;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;
    NFC_SCR_CONF eScrType;
    BOOL bIsRedOntf = pRdReq->bsRedOntf;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    usPage = pFlashAddr->usPage;
    
    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }
    
    /* adapter for different flash type */
    if(pRdReq->bsTlcMode)
    {
        ucPageType = pRdReq->bsTlcPgType;
        ucReqType  = NF_PRCQ_TLC_READ_LP_MULTIPLN + ucPageType;
        ucPln      = pFlashAddr->bsPln + (ucPageType * PLN_PER_LUN);
        eScrType   = TLC_RD_ONE_PG;
    }
    else//SLC mode
    {
        ucPageType = 0;
        ucReqType  = NF_PRCQ_READ_MULTIPLN;
        ucPln      = pFlashAddr->bsPln;
        eScrType   = NORMAL_SCR;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;

    /*  DSG configure   */
    while(FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsDsgEn = TRUE;
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedOntf = bIsRedOntf;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,bIsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,bIsRedOntf);
    }

    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_FULL_PLN, ucPageType, eScrType);

    /* Raw data read = true, each CW 2K align */
    pNFCQEntry->bsRawReadEn = pRdReq->bsRawData;

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }

    /*  NFCQ DW5: LBA for read, fill by FW, the same with corresponding LBA in write red   */
    if (pRdReq->bsLbaChkEn || pRdReq->bsEmEn)
    {
        pNFCQEntry->bsRdLba = pRdReq->bsLba;
    }

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pRdReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pRdReq->bsSsu0Addr, pRdReq->bsSsu0Ontf);
    }
    if(FALSE != pRdReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pRdReq->bsSsu1Addr, pRdReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr, FALSE);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);

    /*  DSG DW0: transfer length and chain config, if req secLength = 0, should xfer 256 sectors  */
    pDSG->bsXferByteLen = pRdReq->bsSecLen << SEC_SZ_BITS ;
    pDSG->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    pDSG->bsDramAddr =  HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, pRdReq->bsSecStart, LOGIC_PIPE_PG_SZ_BITS);
    U32 ulAddr = DRAM_START_ADDRESS + (pDSG->bsDramAddr << 1);
    U32 ulData = *(volatile U32*)ulAddr;
    //DBG_Printf("R DRAM addr = 0x%x Data = 0x%x\n", ulAddr, ulData);
    /*  DSG DW2-3: cache status  */
    if(FALSE != pRdReq->bsCSEn)
    {
        HAL_ConfigCacheStatus(pDSG, pRdReq->bsCsAddrOff);
    }
    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcRedOnlyRead
Description:
    read only redundant area
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    BOOL bIsRedOntf: 1-OTFB, 0-DRAM
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
    20151106    abby    modify to meet VT3533, dynamic update RED to OTFB/DRAM
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcRedOnlyRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun, ucPlnStart, ucReqType;
    U8  ucPageType = 0;
    NFCQ_ENTRY *pNFCQEntry;
    NFC_SCR_CONF eScrType = NORMAL_SCR;
    BOOL bIsRedOntf;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPlnStart = pFlashAddr->bsPln;
    bIsRedOntf = pRdReq->bsRedOntf;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /* adapter for different flash type */   
    if(FALSE != pRdReq->bsTlcMode)//TLC
    {
        ucPageType = pRdReq->bsTlcPgType;
        ucReqType  = NF_PRCQ_TLC_READ_LP_MULTIPLN + ucPageType;
        ucPlnStart = pFlashAddr->bsPln + (ucPageType * PLN_PER_LUN);
        eScrType   = TLC_RD_ONE_PG;
    }
    else//SLC mode
    {
        ucPageType = 0;
        ucReqType  = NF_PRCQ_READ_MULTIPLN;
        ucPlnStart = pFlashAddr->bsPln;
        eScrType   = NORMAL_SCR;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    //sector length invalid and ucDmaTotalLength = PLN number - 1 when bsRedOnly = 1
    pNFCQEntry->bsDmaTotalLength = PLN_PER_LUN - 1;
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    pNFCQEntry->bsRedOnly = TRUE;
    pNFCQEntry->bsRedEn = TRUE;
    pNFCQEntry->bsRedOntf = bIsRedOntf;
    pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPlnStart,bIsRedOntf);
    *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPlnStart,bIsRedOntf);
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_FULL_PLN, ucPageType, eScrType);

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr, FALSE);

    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcChangeColRead
Description:
    change column read
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    NFC_READ_REQ_DES *pRdReq;
Output Param:
    none
Return Value:
Usage:
History:
    20151113    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcChangeColRead(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U16 usCurDsgId;
    U8  ucPU, ucLun, ucPln;
    U8  ucPageType;
    NFCQ_ENTRY *pNFCQEntry;
    NORMAL_DSG_ENTRY * pDSG;
    NFC_SCR_CONF eScrType;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /* adapter for different flash type */
    if(pRdReq->bsTlcMode)//TLC
    {
        ucPageType = pRdReq->bsTlcPgType;
        ucPln      = pFlashAddr->bsPln + (ucPageType * PLN_PER_LUN);
        eScrType   = TLC_RD_ONE_PG;
    }
    else
    {
        ucPageType = 0;
        ucPln      = pFlashAddr->bsPln;
        eScrType   = NORMAL_SCR;
    }
    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
#ifdef DRAMLESS_ENABLE
    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsXorEn  = FALSE;
    pNFCQEntry->bsOtfbBsSel = OTFB_BASE_SEL;
#endif

    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;

    /*  DSG configure   */
    while(FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }
    pNFCQEntry->bsDsgEn = TRUE;
    pNFCQEntry->bsFstDsgPtr = usCurDsgId;

    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_CCLR_MULTIPLN);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedOntf = pRdReq->bsRedOntf;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,pRdReq->bsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,(pRdReq->bsRedOntf));
    }
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_FULL_PLN, ucPageType, eScrType);

    /* Raw data read = true, each CW 2K align */
    pNFCQEntry->bsRawReadEn = pRdReq->bsRawData;

    /*  NFCQ DW5: LBA for read, fill by FW, the same with corresponding LBA in write red   */
    if (pRdReq->bsLbaChkEn || pRdReq->bsEmEn)
    {
        pNFCQEntry->bsRdLba = pRdReq->bsLba;
    }

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr, FALSE);

    /*  CONFIGURE DSG   */
    pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);
    /*  DSG DW0: transfer length and chain config, if req secLength = 0, should xfer 256 sectors  */
    pDSG->bsXferByteLen = pRdReq->bsSecLen << SEC_SZ_BITS ;
    pDSG->bsLast = TRUE;
    /*  DSG DW1: dram address  */
    pDSG->bsDramAddr =  HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, pRdReq->bsSecStart, LOGIC_PIPE_PG_SZ_BITS);

    /*  Set DSG valid    */
    HAL_SetNormalDsgSts(usCurDsgId, NORMAL_DSG_VALID);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_CCLR_MULTIPLN, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcReadRetry
Description:
    flash read retry interface.
Input Param:
    FLASH_ADDR *pFlashAddr: Flash address
    NFC_READ_REQ_DES *pRdReq: Read request descriptor
    U8 ReqType: PRCQ_Type, to distinc single pln op with full pln op
Output Param:
    none
Return Value:
    U8 ucErrCode:   0 - retry success;
                    other - retry fail
Usage:
    for driver test
History:
    20160321    abby    move from flash driver and rename from HAL_FlashReadRetry
------------------------------------------------------------------------------*/
LOCAL U8 MCU12_DRAM_TEXT HAL_NfcReadRetry(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, U8 ReqType)
{
    U8  ucPU, ucLun, ucPln;
    U8  ucRetryTime, ucErrCode, ucParaNum;
    U16 usCurDsgId;
    U32 ulCmdStatus, ulIndex;
    NFCQ_ENTRY *pCqEntry,CqBak;
    NORMAL_DSG_ENTRY *pDSG;
    RETRY_TABLE tRetryPara = {0};
    BOOL bTlcMode = FALSE;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    bTlcMode = pRdReq->bsTlcMode;

    /*    copy NFCQ of last err read cmd, to redo this cmd */
    pCqEntry = HAL_NfcGetNfcqEntryAddr_RP(ucPU, ucLun);
    COM_MemCpy((U32 *)&CqBak, (U32 *)pCqEntry, sizeof(NFCQ_ENTRY)/sizeof(U32));

    // reset CQ
    HAL_NfcResetCmdQue(ucPU, ucLun);
    HAL_NfcClearINTSts(ucPU, ucLun);

    // Step1. pre-condition: enter to shift read mode.
    HAL_FlashRetryPreConditon(pFlashAddr);
    ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        //DBG_Printf("PU %d ReadRetry PreConditon Wrong!\n", ucPU);
    }

    ucRetryTime = 0;
    while (NFC_STATUS_SUCCESS == HAL_FlashRetryCheck(ucRetryTime, bTlcMode))
    {
        // Step2. set-parameters: adj voltage.
        ulIndex = HAL_FlashSelRetryPara(bTlcMode);
        tRetryPara = HAL_FlashGetRetryParaTab(ulIndex);
        ucParaNum = (FALSE == bTlcMode) ? HAL_FLASH_RETRY_PARA_MAX : HAL_TLC_FLASH_RETRY_PARA_MAX;

        HAL_FlashRetrySendParam(pFlashAddr, &tRetryPara, bTlcMode, ucParaNum);
        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            //DBG_Printf("PU %d ReadRetry SendParam Wrong!\n", ucPU);
        }

        // Step3. read-retry-enable:
        HAL_FlashRetryEn(pFlashAddr, TRUE);
        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            //DBG_Printf("PU %d ReadRetry Enable Wrong!\n", ucPU);
        }

        // Step4. redo read:
        pCqEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
        COM_MemCpy((U32 *)pCqEntry, (U32 *)&CqBak, sizeof(NFCQ_ENTRY)/sizeof(U32));

        // update the red
        if (NULL != pRdReq->ppNfcRed)
        {
            pCqEntry->bsRedEn   = TRUE;
            pCqEntry->bsRedOntf = pRdReq->bsRedOntf;

            ucPln = HAL_FlashGetPlnNumFromNfcq(pCqEntry);
            pCqEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, pRdReq->bsRedOntf);
            *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, pRdReq->bsRedOntf);
        }

        // update the dsg
        while(FALSE == HAL_GetNormalDsg(&usCurDsgId));
        pDSG = (NORMAL_DSG_ENTRY*)HAL_GetNormalDsgAddr(usCurDsgId);
        COM_MemZero((U32*)pDSG,sizeof(NORMAL_DSG_ENTRY)>>2);
        if (TRUE == HAL_FlashIs1PlnOp(ReqType))
        {
            pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, pRdReq->bsSecStart, LOGIC_PG_SZ_BITS);
        }
        else
        {
            pDSG->bsDramAddr = HAL_NfcGetDmaAddr(pRdReq->bsRdBuffId, pRdReq->bsSecStart, LOGIC_PIPE_PG_SZ_BITS);
        }
        pDSG->bsXferByteLen = pRdReq->bsSecLen << SEC_SZ_BITS ;
        pDSG->bsLast        = TRUE;
        HAL_SetNormalDsgSts(usCurDsgId,1);

        pCqEntry->bsFstDsgPtr = usCurDsgId;

        pCqEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ReqType);
        HAL_NfcCmdTrigger(pFlashAddr, ReqType, FALSE, INVALID_2F);

        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            ucErrCode = HAL_NfcGetErrCode(ucPU, ucLun);
            if ( NF_ERR_TYPE_UECC != ucErrCode )
            {
                //DBG_Printf("PU %d ReadRetry-ErrCode Changed To %d.\n", ucPU,ucErrCode);
                ucErrCode = NF_ERR_TYPE_UECC;
            }

            HAL_NfcResetCmdQue(ucPU, ucLun);
            HAL_NfcClearINTSts(ucPU, ucLun);

            ucRetryTime++;
            DBG_Printf("PU%d LUN%d Blk%d Pg#%d ReadRetry ErrType=%d CurTime:%d.\n", ucPU,ucLun
                      ,pFlashAddr->usBlock,pFlashAddr->usPage,ucErrCode,ucRetryTime);
        }
        else
        {
            ucErrCode = NF_SUCCESS;
            break;
        }
    }

    // Step5. terminate retry: enter to normal mode
    HAL_FlashRetryTerminate(ucPU, ucLun, bTlcMode);
    ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        //DBG_Printf(" PU %d ReadRetry Terminate wrong!\n", ucPU);
    }

    if(NFC_STATUS_FAIL == HAL_FlashRetryCheck(ucRetryTime, bTlcMode))
    {
        DBG_Printf("PU %d ReadRetry Fail!\n", ucPU);
    }
    else
    {
        //DBG_Printf("PU %d ReadRetry Success!\n", ucPU);
    }

    return ucErrCode;
}

/*------------------------------------------------------------------------------
Name: HAL_NfcWaitStatusWithRetry
Description:
    wait flash status, do read retry is error occur.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    for driver test
History:
    20160321    abby    move from flash driver
------------------------------------------------------------------------------*/
U8 MCU12_DRAM_TEXT HAL_NfcWaitStatusWithRetry(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, U8 ReqType)
{
    U8  ucErrCode = NF_SUCCESS;
    U8  ucPU, ucLun;
    U32 ulCmdStatus;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
    if (NFC_STATUS_SUCCESS != ulCmdStatus)
    {
        ucErrCode = HAL_NfcGetErrCode(ucPU, ucLun);
        if (NF_ERR_TYPE_UECC  == ucErrCode)
        {
            ucErrCode = HAL_NfcReadRetry(pFlashAddr, pRdReq, ReqType);
        }
    }

    return ucErrCode;
}

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
    20151205    abby    modify reg name to meet VT3533
------------------------------------------------------------------------------*/
void MCU12_DRAM_TEXT HAL_NfcInitChainCnt(void)
{
    l_NfcChainNumReg = (NFC_CHAIN_NUM_REG*)&rNfcHIDTag2;
    l_NfcPeerChainNumReg = (NFC_CHAIN_NUM_REG*)&rNfcHIDTag1;

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
    20151207    abby    modify to meet VT3533 single MCU arch
------------------------------------------------------------------------------*/
#if 0
void HAL_NfcFinishChainCnt(U8 ucHID, U16 usTotalChain)
{
    NFC_CHAIN_NUM_REG tChainNumReg;
    tChainNumReg.ulChainNumReg = 0;
    tChainNumReg.bsHID = ucHID;
    tChainNumReg.bsChainNum = usTotalChain;
    tChainNumReg.bsValid = TRUE;

    HAL_HwDebugTrace(ucHID, RCD_NFC_CHAIN, NULL, usTotalChain, NULL);

    l_NfcChainNumReg->ulChainNumReg = tChainNumReg.ulChainNumReg;

#ifdef SIM
    //SGE_RecordChainNumForMCU1(ucHID, usTotalChain, TRUE);
    SGE_CheckChainNumComplete(ucHID);
#endif

    return;
}
#endif

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

    l_NfcPeerChainNumReg->ulChainNumReg = tChainNumReg.ulChainNumReg;

#ifdef SIM
    //SGE_RecordChainNumForMCU1(ucHID, 0, TRUE);
    SGE_CheckChainNumComplete(ucHID);
#endif


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

/*------------------------------------------------------------------------------
Name: HAL_NfcHostPageWrite
Description:
    program host memory to Flash page by on-the-fly path.
Input Param:
    FLASH_ADDR *pFlashAddr: pointer to target Flash address
    U8 ucCmdTag: host command tag
    U16 usFirstHSGId: first HSG id of host memory chain
    NFC_RED *pNfcRed: pointer to redundant data
    BOOL bIsRedOntf: 1-OTFB, 0-DRAM
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
    20151106    abby    modify to meet VT3533, dynamic update RED to OTFB/DRAM
    20160416    abby    add 3D MLC support
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcHostPageWrite(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId, NFC_PRG_REQ_DES *pWrReq)
{
    U8 ucPU, ucLun, ucPln, ucRedPlnNum, ucReqType;
    U16 usPage;
    U32 ulPgSz;
    NFCQ_ENTRY *pNFCQEntry;
    NFC_RED *pTargetRed;
    NFC_SCR_CONF eScrType;
    BOOL bMultiRowAddr;
    BOOL bRedOntf = pWrReq->bsRedOntf;

    ucPU   = pFlashAddr->ucPU;
    ucLun  = pFlashAddr->ucLun;
    ucPln  = pFlashAddr->bsPln;
    usPage = pFlashAddr->usPage;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (FALSE == HAL_NfcIsPairPageAccessable(usPage))
    {
        return NFC_STATUS_FAIL;
    }

    /*  SGQ busy check   */
    if (TRUE == HAL_SgqIsBusy(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /* adapter for different flash types */
          
    if(pWrReq->bsTlcMode)
    {
        ucReqType  = NF_PRCQ_TLC_PRG_MULTIPLN + pWrReq->bsTlcPgCycle;
        bMultiRowAddr = TRUE;
        eScrType   = TLC_RW_TWO_PG;
    }
    else
    {
        ucReqType   = NF_PRCQ_PRG_MULTIPLN;
        bMultiRowAddr = FALSE;
        eScrType = NORMAL_SCR;
    }

    ulPgSz = (FALSE == bMultiRowAddr) ? (LOGIC_PIPE_PG_SZ) : (LOGIC_PIPE_PG_SZ * PGADDR_PER_PRG * INTRPG_PER_PGADDR);
    ucRedPlnNum = (FALSE == bMultiRowAddr) ? (PLN_PER_LUN) : (PLN_PER_LUN * PGADDR_PER_PRG * INTRPG_PER_PGADDR);

    /*  NFCQ DW0: mode configure   */
    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsLbaChk  = pWrReq->bsLbaChkEn;
    pNFCQEntry->bsEMEn    = pWrReq->bsEmEn;

    //inform this is host request
    pNFCQEntry->bsTrigOmEn = TRUE;
    pNFCQEntry->bsDsgEn    = FALSE;
    pNFCQEntry->bsNcqCntEn = TRUE;

    pNFCQEntry->bsOntfEn = TRUE;
    pNFCQEntry->bsNcqNum = ucCmdTag;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pWrReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pWrReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    /*  Note: for write cmd, the unit of DmaTotalLength is 1KB */
    pNFCQEntry->bsDmaTotalLength = ulPgSz >> CW_INFO_SZ_BITS;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pWrReq->pNfcRed)
    {
        pNFCQEntry->bsRedOntf = bRedOntf;
        pNFCQEntry->bsRedEn = TRUE;
        pTargetRed = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, bRedOntf);
        COM_MemCpy((U32 *)pTargetRed, (U32 *)(pWrReq->pNfcRed), RED_SZ_DW * ucRedPlnNum);
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU, ucLun, HAL_NfcGetWP(ucPU, ucLun), ucPln, bRedOntf);
    }
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_FULL_PLN, 0, eScrType);

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pWrReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pWrReq->bsSsu0Addr, pWrReq->bsSsu0Ontf);
    }
    if(FALSE != pWrReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pWrReq->bsSsu1Addr, pWrReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr, bMultiRowAddr);

    /*  CONFIGURE SGQ   */
    g_pNfcqForHalDebug = pNFCQEntry;

    HAL_SgqBuildEntry(ucCmdTag, usFirstHSGId, pFlashAddr->ucPU, pFlashAddr->ucLun, TRUE);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);

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
    NFC_READ_REQ_DES *pRdReq;
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS means command sent success; FALSE means.
        NFC_STATUS_SUCCESS means command sent fail.
Usage:
    caller should prepared HSG chain before call this function
History:
    20140911    Gavin   modify to meet coding style
    20140918    Gavin   add header for this function
    20151106    abby    modify to meet VT3533, dynamic update RED to OTFB/DRAM
    20160416    abby    add 3D MLC support
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_NfcHostPageRead(FLASH_ADDR *pFlashAddr, U8 ucCmdTag, U16 usFirstHSGId, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun, ucPln, ucReqType;
    U8  ucPageType = 0;
    U16 usPage;
    NFCQ_ENTRY *pNFCQEntry;
    NFC_SCR_CONF eScrType = NORMAL_SCR;
    BOOL bIsRedOntf = pRdReq->bsRedOntf;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    ucPln = pFlashAddr->bsPln;
    usPage = pFlashAddr->usPage;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    if (TRUE == HAL_SgqIsBusy(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /* adapter for different flash type */
    ucReqType   = NF_PRCQ_READ_MULTIPLN;

    if(pRdReq->bsTlcMode)//TLC
    {
        ucPageType = pRdReq->bsTlcPgType;
        ucReqType  = NF_PRCQ_TLC_READ_LP_MULTIPLN + ucPageType;
        ucPln      = pFlashAddr->bsPln + (ucPageType * PLN_PER_LUN);
        eScrType   = TLC_RD_ONE_PG;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /*  NFCQ DW0: mode configure   */
    pNFCQEntry->bsDCrcEn  = FALSE;
    pNFCQEntry->bsDsIndex = DS_ENTRY_SEL;
    pNFCQEntry->bsEMEn    = pRdReq->bsEmEn;

    /*  NFCQ DW1: sector enable and length   */
    pNFCQEntry->aSecAddr[0].bsSecStart = pRdReq->bsSecStart;
    pNFCQEntry->aSecAddr[0].bsSecLength = pRdReq->bsSecLen;

    /*  NFCQ DW2: error injection  */
    if (FALSE != pRdReq->bsInjErrEn)
    {
        HAL_NfcSetErrInj(pNFCQEntry, pRdReq->pErrInj);
    }

    /*  NFCQ DW3: DMA message, total data length  */
    pNFCQEntry->bsDmaTotalLength = pRdReq->bsSecLen;
    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  NFCQ DW4: red and scramble   */
    if (NULL != pRdReq->ppNfcRed)
    {
        pNFCQEntry->bsRedEn = TRUE;
        pNFCQEntry->bsRedOntf = bIsRedOntf;
        pNFCQEntry->bsRedAddr = HAL_NfcGetRedRelativeAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,bIsRedOntf);
        *(pRdReq->ppNfcRed) = (NFC_RED*)HAL_NfcGetRedAbsoulteAddr(ucPU,ucLun,HAL_NfcGetWP(ucPU, ucLun),ucPln,bIsRedOntf);
    }
    HAL_ConfigScramble(pNFCQEntry, pFlashAddr->usPage, SCR_FULL_PLN, ucPageType, eScrType);

    /* Raw data read = true, each CW 2K align */
    pNFCQEntry->bsRawReadEn = pRdReq->bsRawData;

    /* DEC FIFO status report config */
    if (FALSE != pRdReq->bsDecFifoEn)
    {
        HAL_DecFifoTrigNfc(pNFCQEntry, pRdReq->bsDecFifoIndex);
    }

    //set raw command queue
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);
    //inform this is host request
    pNFCQEntry->bsTrigOmEn = TRUE;
    pNFCQEntry->bsDsgEn = FALSE;

    pNFCQEntry->bsOntfEn = TRUE;// MixVector use OTFB write mode,so this bit should enable
    pNFCQEntry->bsNcqNum = ucCmdTag;

    /*  NFCQ DW5: LBA for read, fill by FW, the same with corresponding LBA in write red   */
    if (pRdReq->bsLbaChkEn || pRdReq->bsEmEn)
    {
        pNFCQEntry->bsRdLba = pRdReq->bsLba;
    }

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pRdReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pRdReq->bsSsu0Addr, pRdReq->bsSsu0Ontf);
    }
    if(FALSE != pRdReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pRdReq->bsSsu1Addr, pRdReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    HAL_NfcSetFullPageRowAddr(pNFCQEntry, pFlashAddr, FALSE);

    //fill SGQ
    g_pNfcqForHalDebug = pNFCQEntry;
    HAL_SgqBuildEntry(ucCmdTag, usFirstHSGId, ucPU, ucLun, FALSE);


    //trigger command
    HAL_NfcCmdTrigger(pFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}
#endif

#if ((defined(FLASH_TLC) && !defined(FLASH_TSB_3D)) && !defined(FLASH_INTEL_3DTLC))
/*------------------------------------------------------------------------------
Name: HAL_SlcCopyToSlcExt
Description:
    In external way, read 1 SLC page, then copy it to another SLC page.
    Data will be read out to buffer after ECC.
Input Param:
    FLASH_ADDR *pSrcFlashAddr: source SLC addr
    FLASH_ADDR *pDesFlashAddr: target SLC addr
    NFC_READ_REQ_DES *pRdReq : read descriptor
    NFC_PRG_REQ_DES *pWrReq  : write descriptor
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS: command sent success;
         NFC_STATUS_FAIL: command sent fail.
Usage:
    call by FW need copy SLC to SLC page by external way
    note that target addr and source addr both from the same PU&LUN, and should followed prg order in a blk
History:
     20160117    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_SlcCopyToSlcExt(FLASH_ADDR *pSrcFlashAddr, FLASH_ADDR *pDesFlashAddr
                                    , NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq)
{
    /*    step1: read a SLC page    */
    HAL_NfcPageRead(pSrcFlashAddr, pRdReq);
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pSrcFlashAddr->ucPU, pSrcFlashAddr->ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pWrReq->bsWrBuffId = pRdReq->bsRdBuffId;
    pWrReq->pNfcRed = *(pRdReq->ppNfcRed);

    /*    step2: write a SLC page to another address  */
    HAL_NfcFullPageWrite(pDesFlashAddr, pWrReq);
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pDesFlashAddr->ucPU, pDesFlashAddr->ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_SlcCopyToTlcExt
Description:
    In external way, read 3 SLC page, then copy it to 1 TLC WL.
    Data will be read out to buffer after ECC.
Input Param:
    FLASH_ADDR *pSrcFlashAddr: source SLC addr
    FLASH_ADDR *pDesFlashAddr: target SLC addr
    NFC_READ_REQ_DES *pRdReq : read descriptor
    NFC_PRG_REQ_DES *pWrReq  : write descriptor
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS: command sent success;
         NFC_STATUS_FAIL: command sent fail.
Usage:
    call by FW need copy SLC to TLC page by external way,
    note that target addr and source addr both from the same PU&LUN, and should followed prg order in a blk
History:
     20160117    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_SlcCopyToTlcExt(FLASH_ADDR *pSrcSlcAddr,FLASH_ADDR *pDstTlcAddr, NFC_READ_REQ_DES *pSlcRdReq, NFC_PRG_REQ_DES *pTlcWrReq)
{
    U8 ucPU, ucLun;
    U16 usSlcIndex, usWlIndex, usPrgIndex;
    U32 *pRdRedBase = (U32*)(pSlcRdReq->ppNfcRed);
    NFC_READ_REQ_DES tRdReq = *pSlcRdReq;
    NFC_PRG_REQ_DES tWrReq = *pTlcWrReq;
    FLASH_ADDR tSrcSlcAddr[3];
    FLASH_ADDR tDesTlcAddr = *pDstTlcAddr;

    for (usSlcIndex = 0; usSlcIndex < PGADDR_PER_PRG * INTRPG_PER_PGADDR; usSlcIndex++)
    {
        tSrcSlcAddr[usSlcIndex] = *(pSrcSlcAddr + usSlcIndex);
    }
    /*  pSrcSlcAddr[0-2] all have the same PU/LUN  */
    ucPU = tSrcSlcAddr[0].ucPU;
    ucLun = tSrcSlcAddr[0].ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }
    tRdReq.pErrInj = NULL;
    tRdReq.bsRedOntf  = FALSE;  // RED to DRAM
    /*  step1: read 3 SLC source pages  */
    for (usWlIndex = 0; usWlIndex < PG_PER_BLK; usWlIndex++)
    {
        for (usSlcIndex = 0; usSlcIndex < PGADDR_PER_PRG * INTRPG_PER_PGADDR; usSlcIndex++)
        {
            tSrcSlcAddr[usSlcIndex].usBlock = pSrcSlcAddr[0].usBlock + usSlcIndex;
            tSrcSlcAddr[usSlcIndex].usPage  = usWlIndex;// the same page between different SLC
            tRdReq.bsRdBuffId = pSlcRdReq->bsRdBuffId + usWlIndex * PGADDR_PER_PRG * INTRPG_PER_PGADDR + usSlcIndex;
            tRdReq.ppNfcRed = (NFC_RED **)(pRdRedBase + RED_SW_SZ_DW * (usWlIndex * PGADDR_PER_PRG * INTRPG_PER_PGADDR + usSlcIndex));
            tRdReq.bsTlcMode = FALSE;

            HAL_NfcPageRead(&tSrcSlcAddr[usSlcIndex], &tRdReq);
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
            {
                return NFC_STATUS_FAIL;
            }
            COM_MemCpy((U32 *)tRdReq.ppNfcRed, *(U32**)(tRdReq.ppNfcRed), RED_SW_SZ_DW);
        }
    }

    /*    step2: program des TLC 1WL(lower/middle/upper) page    */
    tWrReq.pErrInj = NULL;
    tWrReq.bsTlcMode = TRUE;
    tWrReq.bsRedOntf = FALSE;
    for (usPrgIndex = 0; usPrgIndex < PG_PER_BLK * PRG_CYC_CNT; usPrgIndex++)
    {
        tWrReq.bsTlcPgCycle    = HAL_FlashGetTlcPrgCycle(usPrgIndex);
        tDesTlcAddr.usPage = HAL_FlashGetTlcPrgWL(usPrgIndex);
        tWrReq.bsWrBuffId = pSlcRdReq->bsRdBuffId + tDesTlcAddr.usPage * PGADDR_PER_PRG * INTRPG_PER_PGADDR;  //data from read buffer ID n~n+2, total 3 pages
        tWrReq.pNfcRed = (NFC_RED *)(pRdRedBase + RED_SW_SZ_DW * (tDesTlcAddr.usPage * PGADDR_PER_PRG * INTRPG_PER_PGADDR));
        while(TRUE == HAL_NfcGetFull(tDesTlcAddr.ucPU, tDesTlcAddr.ucLun))
        {
            ;
        }
        HAL_NfcFullPageWrite(&tDesTlcAddr, &tWrReq);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tDesTlcAddr.ucPU, tDesTlcAddr.ucLun))
        {
            return NFC_STATUS_FAIL;
        }
    }

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: HAL_SlcCopyToSlcInt
Description:
    In external way, read 3 SLC page, then copy it to 1 TLC WL.
    Data will be read out to buffer after ECC.
Input Param:
    FLASH_ADDR *pSrcFlashAddr: source SLC addr
    FLASH_ADDR *pDesFlashAddr: target SLC addr
    NFC_READ_REQ_DES *pRdReq : read descriptor
    NFC_PRG_REQ_DES *pWrReq  : write descriptor
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS: command sent success;
         NFC_STATUS_FAIL: command sent fail.
Usage:
    call by FW need copy SLC to SLC page by internal way with once trigger cmd
    note that target addr and source addr both from the same PU&LUN
History:
     20160117    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_SlcCopyToSlcInt(FLASH_ADDR *pSrcAddr, FLASH_ADDR *pDesAddr
                                    , NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq)
{
    U8  ucPU, ucLun, ucPln;
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;
    U8 ucAddrIndex = 0;
    U8 ucPlnIndex;

    // pSrcSlcAddr and pDstTlcAddr should share the same PU & LUN & PLN
    ucPU  = pSrcAddr->ucPU;
    ucLun = pSrcAddr->ucLun;
    ucPln = pSrcAddr->bsPln;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /* enable bsDmaByteEn when internal copy back */
    pNFCQEntry->bsDmaByteEn = TRUE;

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pWrReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pWrReq->bsSsu0Addr, pWrReq->bsSsu0Ontf);
    }
    if(FALSE != pWrReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pWrReq->bsSsu1Addr, pWrReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    /*  SRC SLC ROW addr: 0-1, 2PLN */
    for (ucPlnIndex = 0; ucPlnIndex < PLN_PER_LUN; ucPlnIndex++)
    {
        tFlashAddr = *pSrcAddr;
        tFlashAddr.bsPln = ucPlnIndex;
        pNFCQEntry->atRowAddr[ucAddrIndex++].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
    }
    /*  DES SLC ROW addr: 2-3, 2PLN */
    for (ucPlnIndex = 0; ucPlnIndex < PLN_PER_LUN; ucPlnIndex++)
    {
        tFlashAddr = *pDesAddr;
        tFlashAddr.bsPln = ucPlnIndex;
        pNFCQEntry->atRowAddr[ucAddrIndex++].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
    }

    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(NF_PRCQ_SLC_COPY_SLC_MULTIPLN);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(&tFlashAddr, NF_PRCQ_SLC_COPY_SLC_MULTIPLN, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

//#ifdef TRI_STAGE_COPY
/*------------------------------------------------------------------------------
Name: HAL_SlcCopyToTlcIntOnce
Description:
    In internal way, read 3 SLC page, then copy it to 1 TLC WL in 3 times.
    Data will be read out to buffer after ECC.
Input Param:
    FLASH_ADDR *pSrcFlashAddr: source SLC addr
    FLASH_ADDR *pDesFlashAddr: target SLC addr
    NFC_READ_REQ_DES *pRdReq : read descriptor
    NFC_PRG_REQ_DES *pWrReq  : write descriptor
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS: command sent success;
         NFC_STATUS_FAIL: command sent fail.
Usage:
    call by FW need copy SLC to TLC page by internal way with once trigger cmd
    note that target addr and source addr both from the same PU&LUN, and should followed prg order in a blk
History:
     20160117    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_SlcCopyToTlcInt(FLASH_ADDR *pSrcSlcAddr,FLASH_ADDR *pDstTlcAddr
                                        , NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq)
{
    U8  ucPU, ucLun, ucPln;
    U8  ucReqType;
    U8  ucAddrIndex = 0;
    NFCQ_ENTRY *pNFCQEntry;

    // pSrcSlcAddr and pDstTlcAddr should share the same PU & LUN & PLN
    ucPU  = pDstTlcAddr->ucPU;
    ucLun = pDstTlcAddr->ucLun;
    ucPln = pDstTlcAddr->bsPln;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /* enable bsDmaByteEn when internal copy back */
    pNFCQEntry->bsDmaByteEn = TRUE;

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pWrReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pWrReq->bsSsu0Addr, pWrReq->bsSsu0Ontf);
    }
    if(FALSE != pWrReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pWrReq->bsSsu1Addr, pWrReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    /*  SLC ROW addr: 0-1, 2PLN */
    for (ucAddrIndex = 0; ucAddrIndex < PLN_PER_LUN; ucAddrIndex++)
    {
        //tFlashAddr = *pSrcSlcAddr;
        pSrcSlcAddr->bsPln = ucAddrIndex;
        pNFCQEntry->atRowAddr[ucAddrIndex].bsRowAddr = HAL_NfcGetFlashRowAddr(pSrcSlcAddr);
    }
    /*  TLC ROW addr: 2-3, 1 pages * 2PLN */
    for ( ; ucAddrIndex < (PLN_PER_LUN * 2); ucAddrIndex++)
    {
        //tFlashAddr = *pDstTlcAddr;
        pDstTlcAddr->bsPln = ucAddrIndex % PLN_PER_LUN;
        pNFCQEntry->atRowAddr[ucAddrIndex].bsRowAddr = HAL_NfcGetFlashRowAddr(pDstTlcAddr);
    }
    ucReqType = NF_PRCQ_SLC_COPY_TLC_1ST_LP_MULTIPLN + (pWrReq->bsTlcPgCycle * PGADDR_PER_PRG * INTRPG_PER_PGADDR) + pWrReq->bsTlcPgType;

    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(pSrcSlcAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}
//#endif

/*------------------------------------------------------------------------------
Name: HAL_SlcCopyToTlcIntOnce
Description:
    In external way, read 3 SLC page, then copy it to 1 TLC WL.
    Data will be read out to buffer after ECC.
Input Param:
    FLASH_ADDR *pSrcFlashAddr: source SLC addr
    FLASH_ADDR *pDesFlashAddr: target SLC addr
    NFC_READ_REQ_DES *pRdReq : read descriptor
    NFC_PRG_REQ_DES *pWrReq  : write descriptor
Output Param:
    none
Return Value:
    U32: NFC_STATUS_SUCCESS: command sent success;
         NFC_STATUS_FAIL: command sent fail.
Usage:
    call by FW need copy SLC to TLC page by internal way with once trigger cmd
    note that target addr and source addr both from the same PU&LUN, and should followed prg order in a blk
History:
     20160117    abby    create
------------------------------------------------------------------------------*/
U32 MCU12_DRAM_TEXT HAL_SlcCopyToTlcIntOnce(FLASH_ADDR *pSrcSlcAddr,FLASH_ADDR *pDstTlcAddr
                                        , NFC_READ_REQ_DES *pRdReq, NFC_PRG_REQ_DES *pWrReq)
{
    U8  ucPU, ucLun, ucPln;
    U8  ucReqType;
    U8 ucAddrIndex = 0;
    NFCQ_ENTRY *pNFCQEntry;
    FLASH_ADDR tFlashAddr;

    // pSrcSlcAddr and pDstTlcAddr should share the same PU & LUN & PLN
    ucPU  = pDstTlcAddr->ucPU;
    ucLun = pDstTlcAddr->ucLun;
    ucPln = pDstTlcAddr->bsPln;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    /*  CONFIGURE NFCQ   */
    pNFCQEntry = (NFCQ_ENTRY *)HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    /* enable bsDmaByteEn when internal copy back */
    pNFCQEntry->bsDmaByteEn = TRUE;

    /*  NFCQ DW6-7: SSU 0 and SSU 1   */
    if(FALSE != pWrReq->bsSsu0En)
    {
        HAL_ConfigSsu0(pNFCQEntry, pWrReq->bsSsu0Addr, pWrReq->bsSsu0Ontf);
    }
    if(FALSE != pWrReq->bsSsu1En)
    {
        HAL_ConfigSsu1(pNFCQEntry, pWrReq->bsSsu1Addr, pWrReq->bsSsu1Ontf);
    }

    /*  NFCQ DW8-15: flash address  */
    /*  SLC ROW addr: 0-5, 3 pages * 2PLN */
    for (ucAddrIndex = 0; ucAddrIndex < (PLN_PER_LUN * PGADDR_PER_PRG * INTRPG_PER_PGADDR); ucAddrIndex++)
    {
        tFlashAddr = pSrcSlcAddr[ucAddrIndex/PLN_PER_LUN];
        tFlashAddr.bsPln = ucAddrIndex % PLN_PER_LUN;
        pNFCQEntry->atRowAddr[ucAddrIndex].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
    }
    /*  TLC ROW addr: 6-7, 1 pages * 2PLN */
    for ( ; ucAddrIndex < (PLN_PER_LUN * 4); ucAddrIndex++)
    {
        tFlashAddr = *pDstTlcAddr;
        tFlashAddr.bsPln = ucAddrIndex % PLN_PER_LUN;
        pNFCQEntry->atRowAddr[ucAddrIndex].bsRowAddr = HAL_NfcGetFlashRowAddr(&tFlashAddr);
    }
    ucReqType = NF_PRCQ_SLC_COPY_TLC_1ST_MULTIPLN + pWrReq->bsTlcPgCycle;

    /*  PRCQ start DW   */
    pNFCQEntry->bsPrcqStartDw = HAL_NfcGetPrcqStartDw(ucReqType);

    /*  Trigger NFC    */
    HAL_NfcCmdTrigger(&tFlashAddr, ucReqType, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

#endif//if (defined(FLASH_TLC) && !defined(FLASH_TSB_3D))



// Initialize XOR relevant fields of NFC register.
void HAL_NfcXorInit(U32 ulXorRedunBaseAddr)
{
    // If we want to use XOR, then we must teel NFC to use the ONTF bit in DSG.
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *)&rNfcPgCfg;
    pNfcPgCfg->bsOntfMode = TRUE;
#ifdef FLASH_TLC
    if (PLN_PER_LUN > 1)
    {
        pNfcPgCfg->bsTlcParityPogMode = TRUE;
    }
#endif

    // Set the base address of XOR redundant, this address is 8 bytes aligned, so it is the [19:3]
    // bits of the address.
    rNfcXorDecFifoCfg &= ~l_kXorRedunBaseAddrMask;
    rNfcXorDecFifoCfg |= (ulXorRedunBaseAddr & l_kXorRedunBaseAddrMask);

    // If we want to use XOR, then we must tell NFC to concatenate the last Code-Word and redundant
    // together.
    rNfcNfdmaCfg |= (1 << l_kConcatLastCwAndRedunPos);
    return;
}

/* end of HAL_FlashDriverExt.c */

