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
Filename    : HAL_EncriptionModule.h
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.05
Description : define register interface and function interface for Encription Module
Others      :
Modify      :
20141105    Gavin     Create file
20141203    Gavin     fix spelling mistakes in register definition
20160107    SparkSun  porting to VT3533
*******************************************************************************/
#ifndef __HAL_ENCRIPTION_MODULE_H__
#define __HAL_ENCRIPTION_MODULE_H__

/* define max number of LPN supported in one PU level */
#define NFC_EM_MAX_LPN 32

#if 0
/* define seed format for every LPN */
typedef struct _EM_SEED_LPN
{
    U32  bsKeySel: 4; //key select
    U32  bsLPN: 28; // LPN
}EM_SEED_LPN;

/* define format of seed which binds to one NFCQ */
typedef struct _EM_SEED_ENTRY
{
    EM_SEED_LPN aEmSeed[NFC_EM_MAX_LPN];
}EM_SEED_ENTRY;

/* define format of SRAM allocated for save seed for all PUs
Note:
    the addressing method for every EM_SEED_ENTRY is {Logic PU, PU level}
*/
typedef struct _EM_SEED_AREA
{
    EM_SEED_ENTRY aEmSeedEntry[NFC_PU_MAX][NFCQ_DEPTH];
}EM_SEED_AREA;
#endif

// EM mode
typedef enum _EM_MODE
{
    EM_MODE_XTS = 1,
    EM_MODE_CBC_ESSIV = 2,
    EM_MODE_CTR_ESSIV = 4,
    EM_MODE_ECB = 9,
    EM_MODE_CBC = 10,
    EM_MODE_CTR = 12
}eEM_MODE;

//sector size for EM
typedef enum _EM_SEC_SIZE
{
    EM_SEC_SIZE_512B = 0,
    EM_SEC_SIZE_1KB,
    EM_SEC_SIZE_2KB,
    EM_SEC_SIZE_4KB,
    EM_SEC_SIZE_8KB,
    EM_SEC_SIZE_16KB
}eEM_SEC_SIZE;

// format definition of control and status register
typedef struct _EM_CTRL_STATUS_REG
{
    U32  bsEmMode: 4; // see eEM_MODE
    U32  bsSecSize: 3; // see eEM_SEC_SIZE
    U32  bsSingleKeyMode: 1; // 1 = single key mode; 0 = dual key mode. valid in sector mode

    U32  bsCipherParam1: 7;
    U32  bsEmCfgSoftRst: 1; // EM_CFG soft reset signal. write 1 to reset, write 0 to release

    U32  bsCipherParam2: 8;

    U32  bsEmBusyCh0: 1; // 1 = channel 0 EM is busy; 0 = idle
    U32  bsEmBusyCh1: 1;
    U32  bsEmBusyCh2: 1;
    U32  bsEmBusyCh3: 1;
    U32  bsEmResetCh: 1; // bsEmResetCh bit[x]: channel x soft reset signal, write 1 to reset, write 0 to release
}EM_CTRL_STATUS_REG;

//key size for EM
typedef enum _EM_KEY_SIZE
{
    EM_KEY_SIZE_128BIT = 0,
    EM_KEY_SIZE_192BIT,
    EM_KEY_SIZE_256BIT
}eEM_KEY_SIZE;

//format definition of key control register
typedef struct _EM_KEY_CTRL_REG
{
    U32  bsKeySize: 2; // see eEM_KEY_SIZE
    U32  bsRsvd0: 2;
    U32  bsKeySlotNum: 4; // key slot number

    U32  bsKeyExpDone: 1; // 1 = key expansion done 
    U32  bsDEKGenEn: 1; // 1 = enable DEK generation
    U32  bsDEKGenDone: 1; // 1 = DEK generation done
    U32  bsDEKDECEn: 1; // 1 = DEK_DEC enable
    U32  bsDEKPeriodEn: 1; // 1 = DEK period enable
    U32  bsRsvd1: 3;

    U32  bsGenDEKUseCh: 4; // bsGenDEKUseCh bit[x] = 1: use chanel x to generate DEK
    U32  bsCfgKeyToCh: 4; // bsCfgKeyToCh bit[x] = 1: configure key to channel x

    U32  bsClkGateEnCh: 4; // bsClkGateEnCh bit[x] = 1: Clock gating enable for EM_CH(x)
    U32  bsClkGateEnEmCfg: 1; // 1 = Clock gating enable for EM_CFG
    U32  bsWtKeyDataRegCnt: 3; // write key data register counter
}EM_KEY_CTRL_REG;

//control and status register
#define rEmCtrlAndStatus    (*(volatile U32 *)(REG_BASE_EM + 0x0))
//key control register
#define rEmKeyCtrl          (*(volatile U32 *)(REG_BASE_EM + 0x4))
//key data register
#define rEmKeyData          (*(volatile U32 *)(REG_BASE_EM + 0x8))
//Initial Vector register
#define rEmInitVec(x)       (*(volatile U32 *)(REG_BASE_EM + 0x10 + (x) * 4))
#define rEmInitVec0         (*(volatile U32 *)(REG_BASE_EM + 0x10))
#define rEmInitVec1         (*(volatile U32 *)(REG_BASE_EM + 0x14))
#define rEmInitVec2         (*(volatile U32 *)(REG_BASE_EM + 0x18))
#define rEmInitVec3         (*(volatile U32 *)(REG_BASE_EM + 0x1C))
//AK reigster
#define rEmAK(x)            (*(volatile U32 *)(REG_BASE_EM + 0x20 + (x) * 4))
#define rEmAK0              (*(volatile U32 *)(REG_BASE_EM + 0x20))
#define rEmAK1              (*(volatile U32 *)(REG_BASE_EM + 0x24))
#define rEmAK2              (*(volatile U32 *)(REG_BASE_EM + 0x28))
#define rEmAK3              (*(volatile U32 *)(REG_BASE_EM + 0x2C))
//DEK register
#define rEmDEK(x)           (*(volatile U32 *)(REG_BASE_EM + 0x30 + (x) * 4))
#define rEmDEK0             (*(volatile U32 *)(REG_BASE_EM + 0x30))
#define rEmDEK1             (*(volatile U32 *)(REG_BASE_EM + 0x34))
#define rEmDEK2             (*(volatile U32 *)(REG_BASE_EM + 0x38))
#define rEmDEK3             (*(volatile U32 *)(REG_BASE_EM + 0x3C))
#define rEmDEK4             (*(volatile U32 *)(REG_BASE_EM + 0x40))
#define rEmDEK5             (*(volatile U32 *)(REG_BASE_EM + 0x44))
#define rEmDEK6             (*(volatile U32 *)(REG_BASE_EM + 0x48))
#define rEmDEK7             (*(volatile U32 *)(REG_BASE_EM + 0x4C))

//function interface
void HAL_EMInit(U8 ucKeySizeParam, U8 ucEmMode, const U32 *pIV);
void HAL_EmSetSeed(U8 ucPU, U8 ucWtPtr, const U32 *pLPN, const U8 *pKeySel, U8 ucLPNCount);
#endif //__HAL_ENCRIPTION_MODULE_H__
