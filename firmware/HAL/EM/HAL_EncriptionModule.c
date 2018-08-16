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
Filename    : HAL_EncriptionModule.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.05
Description : In VT3514 C0 design, NFC support encrypt data when read/write
              data to flash. FW need to prepare information about encription key
              in SRAM.
              This file provide basic function interface for this feature.
Others      : Encription and First Byte Check can NOT be enabled simultaneously
Modify      :
20141105    Gavin     Create file
20150109    Gavin     add support for FPGA ENV. optimize key generation flow
20160107    SparkSun  Porting to VT3533
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_Xtensa.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_EncriptionModule.h"


#ifdef FPGA
#define EM_CH_TOTAL 2
#else
#define EM_CH_TOTAL NFC_CH_TOTAL
#endif

#define EM_KEY_NUM_MAX 16
#define EM_KEY_SIZE_DW 8

#define EM_AK0 0x11223344
#define EM_AK1 0x55667788
#define EM_AK2 0x99aabbcc
#define EM_AK3 0xddeeff00

#define EM_P1 0x1
#define EM_P2 0x2

//LOCAL MCU12_VAR_ATTR volatile EM_SEED_AREA *l_pEmSeedArea; // VT3533 do not need seed area
LOCAL MCU12_VAR_ATTR U32 l_aKeyData[EM_KEY_NUM_MAX][EM_KEY_SIZE_DW];
LOCAL MCU12_VAR_ATTR U8 l_ucKeySizeParam;
LOCAL MCU12_VAR_ATTR volatile EM_CTRL_STATUS_REG *l_pEmCtrStatusReg;
LOCAL MCU12_VAR_ATTR volatile EM_KEY_CTRL_REG *l_pEmKeyCtrlReg;

LOCAL void SoftReset(U8 ucChannel);
LOCAL void WaitDEKGenDone(void);
LOCAL void WaitKeyExpansionDone(void);
LOCAL void EmGenKeyData(U8 ucKeySizeParam, U8 ucKeySlotNum, BOOL bDEKPeriodEn, U8 ucChannel, U32 *pKeyData);
LOCAL void EmConfigKeyData(U8 ucKeySizeParam, U8 ucKeySlotNum, BOOL bDEKPeriodEn, U8 ucChannel, U32 *pKeyData);
LOCAL void EmGenAllKeys(BOOL bDekPeriod);
LOCAL void EmConfigAllKeys(BOOL bDekPeriod);

/*------------------------------------------------------------------------------
Name: HAL_EMInit
Description: 
    Init FW software pointer and enable HW function.
Input Param:
    U8 ucKeySizeParam: key size for EM, see eEM_KEY_SIZE
    U8 ucEmMode: EM mode, see eEM_MODE
    const U32 *pIV: pointer to Initial Vector value
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function in boot/init sequence if need EM function;
History:
    20141106    Gavin   create function
    20141117    Gavin   add calling to EmGenAllKeys() and EmConfigAllKeys
    20141224    Gavin   add input parameter
    20150107    SparkSun  porting to VT3533
------------------------------------------------------------------------------*/
void HAL_EMInit(U8 ucKeySizeParam, U8 ucEmMode, const U32 *pIV)
{
    U32 ulIvDwIndex;

    //init FW variable
    //l_pEmSeedArea = (volatile EM_SEED_AREA *)REG_BASE_EM;
    l_ucKeySizeParam = ucKeySizeParam;
    l_pEmCtrStatusReg = (volatile EM_CTRL_STATUS_REG *)&rEmCtrlAndStatus;
    l_pEmKeyCtrlReg = (volatile EM_KEY_CTRL_REG *)&rEmKeyCtrl;
    
    //rNfcModeConfig bit[7] = 1: global enable EM and Data Check check function; bit[7] = 0: disable
    //rNfcModeConfig |= (1 << 7);
    rNfcNfdmaCfg |= 1;

    //clock gating for all channel and EM_CFG
    l_pEmKeyCtrlReg->bsClkGateEnCh = 0xF;
    l_pEmKeyCtrlReg->bsClkGateEnEmCfg = TRUE;
    //set key to all channel
    l_pEmKeyCtrlReg->bsCfgKeyToCh = 0xF;

    //generate keys
    EmGenAllKeys(TRUE);

    //config keys
    for (ulIvDwIndex = 0; ulIvDwIndex < 4; ulIvDwIndex++)
    {
        if (NULL != pIV)
        {
            rEmInitVec(ulIvDwIndex) = *pIV++;
        }
        else
        {
            rEmInitVec(ulIvDwIndex) = 0;
        }
    }
    EmConfigAllKeys(TRUE);
        
    //config mode
    l_pEmCtrStatusReg->bsCipherParam1 = EM_P1;
    l_pEmCtrStatusReg->bsCipherParam2 = EM_P2;
    l_pEmCtrStatusReg->bsSingleKeyMode = FALSE;// 0 = dual key mode
    l_pEmCtrStatusReg->bsEmMode = ucEmMode;

    return;
}

/*------------------------------------------------------------------------------
Name: HAL_EmSetSeed
Description: 
    set EM seed to SRAM for NFC load.
Input Param:
    U8 ucPU: PU number
    U8 ucWtPtr: write pointer of PU
    const U32 *pLPN: pointer to LPN array which binds to data in DRAM
    const U8 *pKeySel: pointer to key select factor which binds to LPN
    U8 ucLPNCount: LPN count
Output Param:
    none
Return Value:
    void
Usage:
    FW call this function before trigger NFC;
History:
    20141106    Gavin   create function
    20151113    Jason   low performance for ucLPNCount--.
------------------------------------------------------------------------------*/
void HAL_EmSetSeed(U8 ucPU, U8 ucWtPtr, const U32 *pLPN, const U8 *pKeySel, U8 ucLPNCount)
{
#if 0
    U8 ucLogicPU;
    U8 ucLPNIndex;
    volatile EM_SEED_ENTRY *pEmSeedEntry;
    
    ucLogicPU = HAL_NfcGetLogicPU(ucPU);
    ucLPNIndex = 0;
    pEmSeedEntry = &l_pEmSeedArea->aEmSeedEntry[ucLogicPU][ucWtPtr];

    while(ucLPNCount > 0)
    {
        pEmSeedEntry->aEmSeed[ucLPNIndex].bsLPN = *pLPN++;
        pEmSeedEntry->aEmSeed[ucLPNIndex].bsKeySel = *pKeySel++;
        ucLPNIndex++;
        ucLPNCount--;
    }
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: SoftReset
Description: 
    perform one channel soft reset.
Input Param:
    U8 ucChannel: channel number which need to perform soft reset
Output Param:
    none
Return Value:
    void
Usage:
History:
    20141106    Gavin   create function
    20141117    Gavin   add dealy between write 1/0 to reset bit
------------------------------------------------------------------------------*/
LOCAL void SoftReset(U8 ucChannel)
{
    U8 ucClockGateSetting;

    // backup clock gating setting
    ucClockGateSetting = l_pEmKeyCtrlReg->bsClkGateEnCh;

    //disable clock gating before perform soft reset to channel
    l_pEmKeyCtrlReg->bsClkGateEnCh &= ~(1 << ucChannel);

    l_pEmCtrStatusReg->bsEmResetCh |= (1 << ucChannel);// write 1 to reset
    HAL_DelayCycle(20); //delay cycle for HW do reset operation
    l_pEmCtrStatusReg->bsEmResetCh &= ~(1 << ucChannel);// write 0 to release

    //restore clock gating setting
    l_pEmKeyCtrlReg->bsClkGateEnCh = ucClockGateSetting;

    return;
}

/*------------------------------------------------------------------------------
Name: WaitDEKGenDone
Description: 
    Wait DEK generation done by polling register.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    called in DEK generation flow;
History:
    20141106    Gavin   create function
------------------------------------------------------------------------------*/
LOCAL void WaitDEKGenDone(void)
{
#ifdef SIM
    return;
#endif
    while (FALSE == l_pEmKeyCtrlReg->bsDEKGenDone)
    {
        ;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: WaitKeyExpansionDone
Description: 
    Wait Key Expansion done by polling register.
Input Param:
    none
Output Param:
    none
Return Value:
    void
Usage:
    called in Key Expansion flow;
History:
    20141106    Gavin   create function
------------------------------------------------------------------------------*/
LOCAL void WaitKeyExpansionDone(void)
{
#ifdef SIM
    return;
#endif
    while (FALSE == l_pEmKeyCtrlReg->bsKeyExpDone)
    {
        ;
    }

    return;
}

/*------------------------------------------------------------------------------
Name: EmGenKeyData
Description: 
    generate key.
Input Param:
    U8 ucKeySizeParam: key size, see eEM_KEY_SIZE
    U8 ucKeySlotNum: key slot number
    BOOL bDEKPeriodEn: TRUE = DEK period enable; FALSE = disable DEK period
    U8 ucChannel: channel number which used to generate key
Output Param:
    U32 *pKeyData: pointer to memory for save generated key
Return Value:
    void
Usage:
    called when need to generate one key;
History:
    20141106    Gavin   create function
------------------------------------------------------------------------------*/
LOCAL void EmGenKeyData(U8 ucKeySizeParam, U8 ucKeySlotNum, BOOL bDEKPeriodEn, U8 ucChannel, U32 *pKeyData)
{
    U32 ulKeyDwIndex;

    if (TRUE == bDEKPeriodEn)
    {
        rEmAK0 = EM_AK0;
        rEmAK1 = EM_AK1;
        rEmAK2 = EM_AK2;
        rEmAK3 = EM_AK3;

        l_pEmCtrStatusReg->bsEmMode = EM_MODE_ECB;

        //select channel to generate DEK
        l_pEmKeyCtrlReg->bsGenDEKUseCh = (1 << ucChannel);
        l_pEmKeyCtrlReg->bsKeySlotNum = ucKeySlotNum;
        l_pEmKeyCtrlReg->bsKeySize = ucKeySizeParam;
        l_pEmKeyCtrlReg->bsDEKPeriodEn = TRUE; // enable DEK period
        l_pEmKeyCtrlReg->bsDEKDECEn = FALSE;
        l_pEmKeyCtrlReg->bsDEKGenEn = TRUE; // Trig HW to  generate key

        WaitDEKGenDone();

        for (ulKeyDwIndex = 0; ulKeyDwIndex < EM_KEY_SIZE_DW; ulKeyDwIndex++)
        {
            *pKeyData++ = rEmDEK(ulKeyDwIndex);//DEK generated by HW
        }
    }
    else
    {
        for(ulKeyDwIndex = 0; ulKeyDwIndex < EM_KEY_SIZE_DW; ulKeyDwIndex++)
        {
            *pKeyData++ = ulKeyDwIndex;// random data
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: EmConfigKeyData
Description: 
    config key.
Input Param:
    U8 ucKeySizeParam: key size, see eEM_KEY_SIZE
    U8 ucKeySlotNum: key slot number
    BOOL bDEKPeriodEn: TRUE = DEK period enable; FALSE = disable DEK period
    U8 ucChannel: channel number which used to generate key. valid when bDEKPeriodEn is TRUE
    U32 *pKeyData: pointer to key
Output Param:
    none
Return Value:
    void
Usage:
    called when need to config one key;
History:
    20141106    Gavin   create function
    20141117    Gavin   add delay between trigger and polling HW
    20141204    Gavin   change the delay cycle count from 10 to 60
------------------------------------------------------------------------------*/
LOCAL void EmConfigKeyData(U8 ucKeySizeParam, U8 ucKeySlotNum, BOOL bDEKPeriodEn, U8 ucChannel, U32 *pKeyData)
{
    U32 ulKeyDwIndex;
    U8 ucKeyDataWtCnt;

    if (TRUE == bDEKPeriodEn)
    {
        rEmAK0 = EM_AK0;
        rEmAK1 = EM_AK1;
        rEmAK2 = EM_AK2;
        rEmAK3 = EM_AK3;

        for (ulKeyDwIndex = 0; ulKeyDwIndex < EM_KEY_SIZE_DW; ulKeyDwIndex++)
        {
            rEmDEK(ulKeyDwIndex) = *pKeyData++;
        }

        l_pEmCtrStatusReg->bsEmMode = EM_MODE_ECB;
        
        l_pEmKeyCtrlReg->bsGenDEKUseCh = 1 << (ucChannel);
        l_pEmKeyCtrlReg->bsKeySlotNum = ucKeySlotNum;
        l_pEmKeyCtrlReg->bsKeySize = ucKeySizeParam;
        l_pEmKeyCtrlReg->bsDEKPeriodEn = TRUE; // enable DEK period
        l_pEmKeyCtrlReg->bsDEKGenEn = FALSE;
        l_pEmKeyCtrlReg->bsDEKDECEn = TRUE; // Trig HW to start Key Expansion
    }
    else
    {
        l_pEmKeyCtrlReg->bsDEKPeriodEn = FALSE;
        l_pEmKeyCtrlReg->bsDEKDECEn = FALSE;
        l_pEmKeyCtrlReg->bsDEKGenEn = FALSE;
        l_pEmKeyCtrlReg->bsKeySlotNum = ucKeySlotNum;
        l_pEmKeyCtrlReg->bsKeySize = ucKeySizeParam;

        if (EM_KEY_SIZE_128BIT == ucKeySizeParam)
        {
            ucKeyDataWtCnt = 4;
        }
        else if (EM_KEY_SIZE_192BIT == ucKeySizeParam)
        {
            ucKeyDataWtCnt = 6;
        }
        else if (EM_KEY_SIZE_256BIT == ucKeySizeParam)
        {
            ucKeyDataWtCnt = 8;
        }

        for (ulKeyDwIndex = 0; ulKeyDwIndex < ucKeyDataWtCnt; ulKeyDwIndex++)
        {
            rEmKeyData = *pKeyData++;
        }
    }

    HAL_DelayCycle(60); // delay cycles for HW signal going to stable state
    WaitKeyExpansionDone();

    return;
}


LOCAL void EmGenAllKeys(BOOL bDekPeriod)
{
    U8 ucKeySlotIndex, ucChIndex;

    for (ucKeySlotIndex = 0; ucKeySlotIndex < EM_KEY_NUM_MAX; ucKeySlotIndex++)
    {
        ucChIndex = ucKeySlotIndex % EM_CH_TOTAL;
        EmGenKeyData(l_ucKeySizeParam, ucKeySlotIndex, bDekPeriod, ucChIndex, l_aKeyData[ucKeySlotIndex]);
    }
    
    return;
}

LOCAL void EmConfigAllKeys(BOOL bDekPeriod)
{
    U8 ucKeySlotIndex, ucChIndex;

    for (ucChIndex = 0; ucChIndex < EM_CH_TOTAL; ucChIndex++)
    {
        for (ucKeySlotIndex = 0; ucKeySlotIndex < EM_KEY_NUM_MAX; ucKeySlotIndex++)
        {
            EmConfigKeyData(l_ucKeySizeParam, ucKeySlotIndex, bDekPeriod, ucChIndex, l_aKeyData[ucKeySlotIndex]);
        }
    }

    return;
}

/* end of file HAL_EncriptionModule.c */
