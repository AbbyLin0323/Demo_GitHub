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
Filename     : Bootloadr.c
Version      :
Date         :
Author       : Tobey
Others:
Modification History:
*******************************************************************************/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#include "COM_Memory.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashCmd.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_ParamTable.h"
#include "HAL_HostInterface.h"
#include "BootLoader.h"
#include "Proj_Config.h"
#include "HAL_ParamTable.h"
#include "HAL_LdpcEngine.h"
#include "HAL_DecStsReport.h"
#include "BL_OPDefine.h"

#ifndef SIM
#include "uart.h"
#endif

/*-------Define for Tinyloader-------*/

#define BOOT_MAGIC_SIZE     16

//These define should sync with rom (CQE1E, CQE2E, SQEE)
#define TL_CQE1E_A2         15
#define TL_CQE1E_DA         11
#define TL_CQE2E_3000      0
#define TL_SQEE_3E70        0
#define TL_CQE1E_00          5
#define TL_CQE1E_FF          2
#define TL_CQE1E_90          4
#define TL_CQE1E_EF          0
#define TL_CQE2E_E005        6
#define TL_PRCQ_CMD_FINISH   0xFF      //ff : finish or end

#define HEADER__SIZE                     1024
#define HEADER_LOCATION         (OTFB_START_ADDRESS + 0x4000)   //fixed address OTFB offset 16K
#define SYSTEM_IMG_ADDR_SAVE_FW     DRAM_FW_UPDATE_BASE
#define BL_P1_HEADER_TMP_OTFB_OFFSET    0x4000      //16KB
#define BL_P1_TMP_OTFB_ADDR         (HEADER_LOCATION + HEADER__SIZE)

//System image location for saving FW(OTFB size is not enough, also need SRAM)
#define BL_LLF_IMG_OTFB_SIZE     560*1024 //576 (OTFB total) - 16 (rest system img except BL p0 15K & reserverd 1K)
#define BL_LLF_IMG_SRAM1_ADDR   0x1FF85000
#define BL_LLF_IMG_SRAM1_SIZE   60*1024
#define BL_LLF_IMG_SRAM2_ADDR   0x1FFA0000
#define BL_LLF_IMG_SRAM2_SIZE   16*1024
#define BL_LLF_IMG_SRAM3_ADDR   0x1FFC0000
#define BL_LLF_IMG_SRAM3_SIZE   32*1024

#define BOOT_DELAY_SEC  3
#define DELAY_1S        1000000

/*-------Structure Define for Tinyloader-------*/

//VT3533 FW IMAGE : each image starting address is 64B aligned
typedef struct _IMAGE_HDR {
    U8 magic[BOOT_MAGIC_SIZE];

    U32 tinyloader_size;  /* size in bytes */
    U32 tinyloader_addr;  /* physical load addr */
    U32 tinyloader_exec; /* tinyloader exec addr */
    U32 tinyloader_crc32;

    U32 bootloader_size;  /* size in bytes */
    U32 bootloader_addr;  /* physical load addr */
    U32 bootloader_exec; /* bootloader exec addr */
    U32 bootloader_crc32;

    U32 mcu0fw_size;  /* size in bytes */
    U32 mcu0fw_addr;  /* physical load addr */
    U32 mcu0fw_exec; /* mcu0 exec addr */
    U32 mcu0fw_crc32;

    U32 mcu1fw_size;  /* size in bytes */
    U32 mcu1fw_addr;  /* physical load addr */
    U32 mcu1fw_exec; /* mcu1 exec addr */
    U32 mcu1fw_crc32;

    U32 mcu2fw_size;  /* size in bytes */
    U32 mcu2fw_addr;  /* physical load addr */
    U32 mcu2fw_exec; /* mcu2 exec addr */
    U32 mcu2fw_crc32;

    U32 rom_size;  /* size in bytes */
    U32 rom_addr;  /* physical load addr */
    U32 rom_exec; /* rom exec addr */
    U32 rom_crc32;
} IMAGE_HDR;

/* PRCQ QE type define */
typedef enum _TL_PRCQ_QE_TYPE {
    TL_PRCQ_ONE_CYCLE_CMD = 0,
    TL_PRCQ_TWO_CYCLE_CMD,
    TL_PRCQ_TWO_PHASE_CMD,
    TL_PRCQ_THREE_PHASE_CMD,

    TL_PRCQ_ONE_CYCLE_ADDR,
    TL_PRCQ_COL_ADDR,
    TL_PRCQ_ROW_ADDR,
    TL_PRCQ_FIVE_CYCLE_ADDR,

    TL_PRCQ_REG_READ,
    TL_PRCQ_REG_WRITE,
    TL_PRCQ_DMA_READ,
    TL_PRCQ_DMA_WRITE,

    TL_PRCQ_READ_STATUS,
    TL_PRCQ_READ_STATUS_EH,
    TL_PRCQ_IDLE,
    TL_PRCQ_FINISH
} TL_PRCQ_QE_TYPE;

typedef enum _RAW_CMD_INDEX {
    TL_PRCQ_READ_MLC_1PLN = 0,
    TL_PRCQ_READ_SLC_1PLN,
    TL_PRCQ_READ_IM_SLC_1PLN,
    TL_PRCQ_SET_FEATURE,
    TL_PRCQ_CNT
} RAW_CMD_INDEX;


/*-------Gloabl Variables for Tinyloader-------*/
IMAGE_HDR *glHeader = NULL;

volatile NFC_CMD_STS *pNFCQDptr;
volatile NFCQ *pNFCQArray;
volatile U32 *pPrcqArray;
volatile NFC_TRIGGER *pNfcTrigger;

PRCQ_TABLE tl_l_aPrcqTable[TL_PRCQ_CNT];
const PRCQ_TABLE tl_g_aPrcqTable[TL_PRCQ_CNT] = {
    { TL_PRCQ_READ_MLC_1PLN, NF_NORMAL_CMD, TRIG_CMD_READ, MULTI_VALUE_1(9), 0 },
    { TL_PRCQ_READ_SLC_1PLN, NF_NORMAL_CMD, TRIG_CMD_READ, MULTI_VALUE_1(9), 0 },
    { TL_PRCQ_READ_IM_SLC_1PLN, NF_NORMAL_CMD, TRIG_CMD_READ, MULTI_VALUE_1(9), 0 },
    { TL_PRCQ_SET_FEATURE, NF_NORMAL_CMD, TRIG_CMD_WRITE, MULTI_VALUE_1(9), 0 }
};

const U8 tl_l_aQETable[] =
{
    // TL_PRCQ_READ_MLC_1PLN
    // 00 | addr x 5 | 30  | bsy | 00 |data (pln0) | finish
    QE_GRP_ATTR(TL_PRCQ_TWO_PHASE_CMD)      | QE_INDEX(TL_CQE2E_3000),
    QE_GRP_ATTR(TL_PRCQ_FIVE_CYCLE_ADDR)    | QE_INDEX(0x0),
    QE_GRP_ATTR(TL_PRCQ_READ_STATUS)        | QE_INDEX(TL_SQEE_3E70),
    QE_GRP_ATTR(TL_PRCQ_ONE_CYCLE_CMD)      | QE_INDEX(TL_CQE1E_00),
    QE_GRP_ATTR(TL_PRCQ_DMA_READ)           | QE_INDEX(0x0),
    TL_PRCQ_CMD_FINISH,

    // TL_PRCQ_READ_SLC_1PLN
    // A2 | 00 | addr x 5 | 30  | bsy | 00 | addr x 5 | 05 | addr x 5 | E0 | data (pln0) | finish
    QE_GRP_ATTR(TL_PRCQ_ONE_CYCLE_CMD)      | QE_INDEX(TL_CQE1E_A2),
    QE_GRP_ATTR(TL_PRCQ_TWO_PHASE_CMD)      | QE_INDEX(TL_CQE2E_3000),
    QE_GRP_ATTR(TL_PRCQ_FIVE_CYCLE_ADDR)    | QE_INDEX(0x0),
    QE_GRP_ATTR(TL_PRCQ_READ_STATUS)        | QE_INDEX(TL_SQEE_3E70),
    QE_GRP_ATTR(TL_PRCQ_TWO_PHASE_CMD)      | QE_INDEX(TL_CQE2E_E005),
    QE_GRP_ATTR(TL_PRCQ_FIVE_CYCLE_ADDR)    | QE_INDEX(0x0),
    QE_GRP_ATTR(TL_PRCQ_DMA_READ)           | QE_INDEX(0x0),
    TL_PRCQ_CMD_FINISH,

    // TL_PRCQ_READ_IM_SLC_1PLN
    // DA | 00 | addr x 5 | 30  | bsy | 00 |data (pln0) | finish
    //For IM 3D TLC
    QE_GRP_ATTR(TL_PRCQ_ONE_CYCLE_CMD)      | QE_INDEX(TL_CQE1E_DA),
    QE_GRP_ATTR(TL_PRCQ_TWO_PHASE_CMD)      | QE_INDEX(TL_CQE2E_3000),
    QE_GRP_ATTR(TL_PRCQ_FIVE_CYCLE_ADDR)    | QE_INDEX(0x0),
    QE_GRP_ATTR(TL_PRCQ_READ_STATUS)        | QE_INDEX(TL_SQEE_3E70),
    QE_GRP_ATTR(TL_PRCQ_TWO_PHASE_CMD)      | QE_INDEX(TL_CQE2E_E005),
    QE_GRP_ATTR(TL_PRCQ_FIVE_CYCLE_ADDR)    | QE_INDEX(0x0),
    QE_GRP_ATTR(TL_PRCQ_DMA_READ)           | QE_INDEX(0x0),
    TL_PRCQ_CMD_FINISH,

    // TL_PRCQ_SET_FEATURE
    // ef | addr x 1 | data(reg write) | finish
    QE_GRP_ATTR(TL_PRCQ_ONE_CYCLE_CMD)     |QE_INDEX(TL_CQE1E_EF),
    QE_GRP_ATTR(TL_PRCQ_ONE_CYCLE_ADDR)    |QE_INDEX(0x0),
    QE_GRP_ATTR(TL_PRCQ_REG_WRITE)         |QE_INDEX(0x0),
    //QE_GRP_ATTR(PRCQ_READ_STATUS)       |QE_INDEX(SQEE_3E70),
    TL_PRCQ_CMD_FINISH
};

BOOTLOADER_FILE *BL_GetBootLoaderFile(void);
void BL_LoadBLnFW(U32 ulStartAddr);
U32 BL_GetSysImgPageCnt(U32 ulHeaderAddr);

BOOL l_bNfcInitDoneFlag;

LOCAL U32 l_ulPUBitFlag;

LOCAL U8 l_ucPUNum = 0;
LOCAL U8 l_ucGetchFlag = 1;

LOCAL U32 l_ulOldNfcPgCfg;
LOCAL U32 l_ulOldNfcTCtrl1;
LOCAL BL_FW_UPDATE_INFO l_tFWUpdateInfo;
DRAM_BOOTLOADER_ATTR U16 g_HexDataIndexTable[256];
DRAM_BOOTLOADER_ATTR U16 g_StrDataIndexTable[256];

typedef struct __HDR_SEG_DES {
    U32 uPayloadSize;
    U32 uLoadAddr;
    U32 ulExecAddr;
    U32 ulCRC32;
} HDR_SEG_DES;

HDR_SEG_DES *pBLP1HdrSegDes = NULL;
HDR_SEG_DES *pFW0HdrSegDes = NULL;
HDR_SEG_DES *pFW1HdrSegDes = NULL;
HDR_SEG_DES *pFW2HdrSegDes = NULL;

LOCAL INIT_FUNC_ENTRY OTFB_FTABLE_ENTRY_ATTR tFTableFuncEntry[FTABLE_FUNC_TOTAL] =
{
    (U32)BL_GlobalInit, 0, 0, 0,
    (U32)BL_DDRInit, 0, 0, 0,
    (U32)BL_NFCInit, 0, 0, 0,
    (U32)BL_PCIEInit, 0, 0, 0,
    (U32)BL_ClkGatingInit, 0, 0, 0,
    (U32)BL_Flash1stInit, 0, 0, 0,
    (U32)BL_Flash2ndInit, 0, 0, 0,
    (U32)BL_SaveFW, 0, 0, 0,
    (U32)BL_ActiveFW, 0, 0, 0,
    (U32)BL_SavePTable, 0, 0, 0,
    (U32)BL_RunFW, 0, 0, 0,
    (U32)BL_SwitchMode, 0, 0, 0,
    (U32)BL_ClearDiskLock, 0, 0, 0,
    (U32)BL_GetHex, 0, 0, 0,
    (U32)BL_GetString, 0, 0, 0
};

#ifndef SIM
void DBG_Getch(void)
{
    while (1 == l_ucGetchFlag)
        ;

    return;
}
#endif

#if 1
DRAM_BOOTLOADER_ATTR void BL_BootDelay(void)
{
    BOOL bIsBootLoaderNeedDelay;

    bIsBootLoaderNeedDelay = HAL_IsBootLoaderNeedDelay();

    if (bIsBootLoaderNeedDelay == TRUE)
    {
        BOOL bExit = FALSE;
        PTABLE * pPtable;
        U8 i, c;
        U32 ulCountInOneUs = HAL_GetMcuClock() / 1000000;
        U32 ulLastTimeTag;
        U32 ulTargetTime;

        DBG_Printf("boot delay : %d\r", BOOT_DELAY_SEC);
        for (i = BOOT_DELAY_SEC; i > 0; i--)
        {
            ulLastTimeTag = HAL_GetMCUCycleCount();
            ulTargetTime = ulLastTimeTag + DELAY_1S * ulCountInOneUs;
            while ((S32)((U32)HAL_GetMCUCycleCount() - (U32)ulTargetTime) < 0)
            {
                     //DBG_Printf("wait delay\n");
                if ((rUART_LCR & 0x3e))
                {
                    DBG_Printf("wait delay\n");
                    if (HAL_UartRxByte() == ' ')
                    {
                        DBG_Printf("Ready to Boot into Ramdisk\n");
                        /* change P-Table to boot into Ramdisk & Disable Print2Dram */
                        pPtable = HAL_GetPTableAddr();
                        pPtable->sBootStaticFlag.bsRamDiskMode = 1;
                        pPtable->sBootStaticFlag.bsPrinttoDRAMEn = 0;
                        bExit = TRUE;
                        break;
                    }
                }
            }
            if (bExit == TRUE)
            {
                break;
            }
            DBG_Printf("boot delay : %d\r", i - 1);
        }
    }
}
#endif

DRAM_BOOTLOADER_ATTR BOOL BL_FwReadBitmapIsPass(U32 *pFwBitMap, U32 index)
{
    if ((pFwBitMap[index/32]&(1<<(index%32))) == (1<<(index%32)))
        return TRUE;
    else
        return FALSE;
}
DRAM_BOOTLOADER_ATTR U8 BL_FlashReadRetry(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun;
    U8  ucRetryTime,ucErrCode, ucParaNum;
    U32 ulCmdStatus, ulStart;
    RETRY_TABLE tRetryPara = {0};
    BOOL bTlcMode = FALSE;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    bTlcMode = pRdReq->bsTlcMode;

    // reset CQ
    HAL_NfcResetCmdQue(ucPU, ucLun);
    HAL_NfcClearINTSts(ucPU, ucLun);

    // Step1. pre-condition: enter to shift read mode.
    HAL_FlashRetryPreConditon(pFlashAddr);
    ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);

    ucRetryTime = 0;
    while (NFC_STATUS_SUCCESS == HAL_FlashRetryCheck(ucRetryTime, bTlcMode))
    {
        // Step2. set-parameters: adj voltage.
        ulStart = HAL_FlashSelRetryPara(bTlcMode);
        tRetryPara = HAL_FlashGetRetryParaTab(ulStart + ucRetryTime);
        ucParaNum = (FALSE == bTlcMode) ? HAL_FLASH_RETRY_PARA_MAX : HAL_TLC_FLASH_RETRY_PARA_MAX;

        HAL_FlashRetrySendParam(pFlashAddr, &tRetryPara, bTlcMode, ucParaNum);
        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);

        // Step3. read-retry-enable:
        HAL_FlashRetryEn(pFlashAddr, TRUE);
        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);

        while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
        {
            ;
        }

        // Step4. redo read:
        HAL_NfcSinglePlnRead(pFlashAddr, pRdReq, FALSE);

        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);
        if (NFC_STATUS_SUCCESS != ulCmdStatus)
        {
            ucErrCode = HAL_NfcGetErrCode(ucPU, ucLun);

            if ( NF_ERR_TYPE_UECC != ucErrCode )
            {
                DBG_Printf("PU %d ReadRetry-ErrCode Changed To %d.\n", ucPU,ucErrCode);
                ucErrCode = NF_ERR_TYPE_UECC;
            }

            HAL_NfcResetCmdQue(ucPU, ucLun);
            HAL_NfcClearINTSts(ucPU, ucLun);

            DBG_Printf("PU%d LUN%d Blk%d Pg#%d ReadRetry ErrType=%d CurTime:%d.\n", ucPU,ucLun
                      ,pFlashAddr->usBlock,pFlashAddr->usPage,ucErrCode,ucRetryTime);

             ucRetryTime++;
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
        DBG_Printf(" PU %d ReadRetry Terminate wrong!\n", ucPU);
    }

    if(NFC_STATUS_FAIL == HAL_FlashRetryCheck(ucRetryTime, bTlcMode))
    {
        DBG_Printf("Pu %d LUN %d BLK%d PG%d Read Retry Fail!\n",ucPU,ucLun,pFlashAddr->usBlock,pFlashAddr->usPage);
    }
    else
    {
        DBG_Printf("Pu %d LUN %d BLK%d PG%d Read Retry OK!\n",ucPU,ucLun,pFlashAddr->usBlock,pFlashAddr->usPage);
    }

    return ucErrCode;
}

/*------------------------------------------------------------------------------
Name: BL_EnablePCIeOptionRom
Description:
    enable OptionRom(EROM) BAR in PCIE configuration header.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Used by MCU0 only.
    In PCIE mode(AHCI/NVMe), call this function in FW cold boot sequence.
    In SATA mode, do not call it.
History:
    20150116    Gavin create
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_EnablePCIeOptionRom(void)
{
    /* map Option ROM space to DRAM address: DRAM_OPTION_ROM */
    *(volatile U32 *)(REG_BASE_HOSTC + 0x3C) = (DRAM_OPTION_ROM - DRAM_START_ADDRESS);

    /* set rGLB(0x64) bit[3] to 0, which means Option ROM is in DRAM */
    rGLB(0x64) &= ~(1 << 3);

    /* enable PCI EROM BAR with 64KB space */
    *(volatile U32 *)(REG_BASE_HOSTC + 0x38) = 0xFFFF0001;

    return;
}

/*------------------------------------------------------------------------------
Name: BL_DisablePCIeOptionRom
Description:
    Disables OptionRom(EROM) BAR in PCIE configuration header.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Used by MCU0 only.
    In PCIE mode(AHCI/NVMe), call this function in FW cold boot sequence.
    In SATA mode, do not call it.
History:
    20160225    Yao create
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_DisablePCIeOptionRom(void)
{
    /* Sets read-only attribute for expansion ROM BAR in PCI header. */
    *(volatile U32 *)(REG_BASE_HOSTC + 0x38) = 0;

    return;
}

/*------------------------------------------------------------------------------
Name: BL_ClearPUBitFlag
Description:
    Clear PU(VPU) BitFlag, PU's relative bit is "1" stands for this PU has done
    some CE level operation.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to Clear PU(VPU) BitFlag.
History:
    20140122    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_ClearPUBitFlag(void)
{
    l_ulPUBitFlag =  0x0;
    return;
}

/*------------------------------------------------------------------------------
Name: BL_SetAllBrotherPUDone
Description:
    set all brother PU BitFlag to '1' at VPU mode.
Input Param:
    U8 ucPU: PU number from FW's view
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to set all brother PU BitFlag to '1' at VPU mode.
History:
    20140122    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_SetAllBrotherPUDone(U8 ucPU)
{
    U8 ucBrotherPU;
    U8 ucBrotherIndex;
    l_ulPUBitFlag |= (1<<ucPU);

    return;
}

/*------------------------------------------------------------------------------
Name: BL_IsPUNeedCELevleCmd
Description:
    check if PU need do CE level command.
Input Param:
    U8 ucPU: PU number from FW's view
Output Param:
    none
Return Value:
    BOOL: TRUE:need; FALSE:no need
Usage:
    invoke it to check if PU need do CE level command.
History:
    20140122    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR BOOL BL_IsPUNeedCELevleCmd(U8 ucPU)
{
    if (0 != (l_ulPUBitFlag & (1<<ucPU)))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*------------------------------------------------------------------------------
Name: BL_GetFeatureAllFlash
Description:
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
History:
    20140122    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_GetFeatureAllFlash(U8 ucAddr)
{
    U8 ucPUNum;
    U8 ErrCode;
    U8 ucCESel;
    FLASH_ADDR tFlashAddr = { 0 };
    U32 aFeature[2] = {0, 0};

    for (ucCESel = 0; ucCESel < SCAN_CE_PER_PU; ucCESel++)
    {
        tFlashAddr.ucLun = ucCESel * LUN_PER_CE;

        BL_ClearPUBitFlag();
        for (ucPUNum = 0; ucPUNum < l_ucPUNum; ucPUNum++)
        {
            tFlashAddr.ucPU = ucPUNum;
            if (TRUE !=  BL_IsPUNeedCELevleCmd(ucPUNum))
            {
                continue;
            }

            BL_SetAllBrotherPUDone(ucPUNum);
            if (NFC_STATUS_FAIL == HAL_NfcGetFeature(&tFlashAddr, ucAddr))
            {
                //DBG_Printf("set feature pu:%d cmd send fail!\n", tFlashAddr.ucPU);
                continue;
            }
        }

        BL_ClearPUBitFlag();
        for (ucPUNum = 0; ucPUNum < l_ucPUNum; ucPUNum++)
        {
            tFlashAddr.ucPU = ucPUNum;
            if (TRUE !=  BL_IsPUNeedCELevleCmd(ucPUNum))
            {
                continue;
            }

            BL_SetAllBrotherPUDone(ucPUNum);
            ErrCode = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ErrCode)
            {
                DBG_Printf("set feature for pu:%d fail!\n", tFlashAddr.ucPU);
                DBG_Getch();
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);
        }
    }

    return;
}


/*------------------------------------------------------------------------------
Name: BL_ResetAllFlash
Description:
    Reset all flash.
    caution: on FPGA env, CE in channal which has no HW logic, default command
    sts is "d", no real cmd will be send.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to Reset all flash.
History:
    20140122    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_SetFeatureAllFlash(U8 ucAddr, U32 ucValue)
{
    U8 ucPUNum;
    U8 ErrCode;
    U8 ucCESel;
    FLASH_ADDR tFlashAddr = { 0 };

    for (ucCESel = 0; ucCESel < SCAN_CE_PER_PU; ucCESel++)
    {
        tFlashAddr.ucLun = ucCESel * LUN_PER_CE;

        BL_ClearPUBitFlag();
        for (ucPUNum = 0; ucPUNum < l_ucPUNum; ucPUNum++)
        {
            tFlashAddr.ucPU = ucPUNum;
            if (TRUE !=  BL_IsPUNeedCELevleCmd(ucPUNum))
            {
                continue;
            }

            BL_SetAllBrotherPUDone(ucPUNum);
            if (NFC_STATUS_FAIL == HAL_NfcSetFeature(&tFlashAddr, ucValue, ucAddr))
            {
                //DBG_Printf("set feature pu:%d cmd send fail!\n", tFlashAddr.ucPU);
                continue;
            }
        }

        BL_ClearPUBitFlag();
        for (ucPUNum = 0; ucPUNum < l_ucPUNum; ucPUNum++)
        {
            tFlashAddr.ucPU = ucPUNum;
            if (TRUE !=  BL_IsPUNeedCELevleCmd(ucPUNum))
            {
                continue;
            }

            BL_SetAllBrotherPUDone(ucPUNum);
            ErrCode = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ErrCode)
            {
                DBG_Printf("set feature for pu:%d fail!\n", tFlashAddr.ucPU);
                DBG_Getch();
            }
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: BL_GetBootLoaderFile
Description:
    Get BootLoaderFile Base.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOTLOADER_FILE *:pointer direct to BootLoaderFile Base.
Usage:
    invoke it to Get BootLoaderFile Base.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
BOOTLOADER_FILE *BL_GetBootLoaderFile(void)
{
    return ((BOOTLOADER_FILE *)OTFB_BOOTLOADER_BASE);
}

/*------------------------------------------------------------------------------
Name: HAL_NfcGetFlashPhyPageSize
Description:
    Get flash physic page size from PTable.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: lash physic page size
Usage:
    FW get PhyPageSize form PTabel builed by BootLoader.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U32 BL_GetFlashPhyPageSize(void)
{
    U32 ulFlashPageSize;
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = BL_GetBootLoaderFile();
    ulFlashPageSize = pBootLoaderFile->tSysParameterTable.ulFlashPageSize;

    return (U32)(ulFlashPageSize);
}

/*------------------------------------------------------------------------------
Name: BL_GetFlashBlkPageNum
Description:
    Get flash page per blk number from PTable.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: lash physic page size
Usage:
    FW get flash page per blk number form PTabel builed by BootLoader.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U32 DRAM_BOOTLOADER_ATTR BL_GetFlashBlkPageNum(void)
{
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = BL_GetBootLoaderFile();
    return pBootLoaderFile->tSysParameterTable.ulSubSysFWPageNum;
}
/*------------------------------------------------------------------------------
Name: BL_GetFWSavePageCnt
Description:
    Get FW save page count from PTable.
Input Param:
    none
Output Param:
    none
Return Value:
    U32: fw saved page count
Usage:
    BootLoader invoke it to Get FW save page count from PTable.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U32 DRAM_BOOTLOADER_ATTR BL_GetFWSavePageCnt(void)
{
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = BL_GetBootLoaderFile();
    return pBootLoaderFile->tSysParameterTable.ulFWSavePageCnt;
}


/*------------------------------------------------------------------------------
Name: BL_GetBootMethodSel
Description:
    get Boot method.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: boot method id;
Usage:
    BootLoader invoke it get Boot method.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U8 BL_GetBootMethodSel(void)
{
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = BL_GetBootLoaderFile();

    return (U8)pBootLoaderFile->tSysParameterTable.sBootStaticFlag.bsBootMethodSel;
}

/*------------------------------------------------------------------------------
Name: BL_IsOptionRomEnable
Description:
    get OptionRomEn flag.
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL:TRUE: enable option rom. FALSE: disable option rom.
Usage:
    BootLoader invoke it get OptionRomEn flag.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
BOOL DRAM_BOOTLOADER_ATTR BL_IsOptionRomEnable(void)
{
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = BL_GetBootLoaderFile();
    return (BOOL)pBootLoaderFile->tSysParameterTable.sBootStaticFlag.bsOptionRomEn;
}

/*------------------------------------------------------------------------------
Name: BL_GetFlashPhyPageSecBit
Description:
    Get flash physic page size Bits from PTable.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW get PhyPageSize Bits form PTabel builed by BootLoader.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U32 DRAM_BOOTLOADER_ATTR BL_GetFlashPhyPageSecBit(void)
{
    U32 ulBit;
    U32 ulPhyPageMsk;
    U32 ulPhyPageSizeBit;
    U32 ulFlashPhyPageSize;

    ulFlashPhyPageSize = BL_GetFlashPhyPageSize();

    ulPhyPageSizeBit = 0;
    ulPhyPageMsk = (ulFlashPhyPageSize-1);

    for (ulBit = 0; ulBit < 32; ulBit++)
    {
        if (0 != ((1 << ulBit) & ulPhyPageMsk))
        {
            ulPhyPageSizeBit++;
        }
    }

    return ulPhyPageSizeBit;
}

/*------------------------------------------------------------------------------
Name: BL_UpdateCEBitMap
Description:
    Calculate CEBitMap at VPU mode according to ReadId result.
Input Param:
    U32 ulCEBitMapOrig:CEBitMap which ignored brother LUN(other VPU)'s exist.
Output Param:
    none
Return Value:
    U32:CEBitMap at VPU mode.
Usage:
    BL invoke it to Calculate CEBitMap at VPU mode.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U32 DRAM_BOOTLOADER_ATTR BL_UpdateCEBitMap(U32 ulCEBitMapOrig)
{
    U8 ucChan;
    U8 ucIndex;
    U8 ucCE;
    U8 ucLunIndex;
    U32 ulCEBitMap;

    ulCEBitMap = ulCEBitMapOrig;
    for (ucChan = 0; ucChan < NFC_CH_TOTAL; ucChan++)
    {
        for (ucIndex = 0; ucIndex < NFC_PU_PER_CH>>LUN_PER_CE_BITS; ucIndex++)
        {
            ucCE = (NFC_CH_TOTAL<<LUN_PER_CE_BITS)*ucIndex + ucChan;
            if (0 != (ulCEBitMap & (1<<ucCE)))
            {
                for (ucLunIndex = 1; ucLunIndex < LUN_PER_CE; ucLunIndex++)
                {
                    ulCEBitMap |= 1<<(ucCE + ucLunIndex*NFC_CH_TOTAL);
                }
            }
        }
    }

    return ulCEBitMap;
}

/*------------------------------------------------------------------------------
Name: BL_CalBitsNumByMap
Description:
    Calculate CE count accordint to CEBitMap.
Input Param:
    U32 ulSubSysCEMap0:CEBitMap for Subsystem0.
    U32 ulSubSysCEMap1:CEBitMap for Subsystem1.
Output Param:
    none
Return Value:
    U8:CE count.
Usage:
    BL invoke it to Calculate CE count accordint to CEBitMap.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR U8 BL_CalBitsNumByMap(U32 ulBitsMap)
{
    U8  ucBitIndex;
    U8 ucBitsNum = 0;

    for (ucBitIndex = 0; ucBitIndex < NFC_PU_MAX; ucBitIndex++)
    {
        if (0 != (ulBitsMap & (1 << ucBitIndex)))
        {
            ucBitsNum++;
        }
    }

    return ucBitsNum;
}

/*------------------------------------------------------------------------------
Name: BL_ResetAllFlash
Description:
    Reset all flash.
    caution: on FPGA env, CE in channal which has no HW logic, default command
    sts is "d", no real cmd will be send.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to Reset all flash.
History:
    20140122    Tobey   create.
    20160901    Dannier modify, 1. reset flash not wait status and add delay time after reset issue. 2. only LLF need reset all flash.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_ResetAllFlash(void)
{
#ifndef SIM
    U8 ucCESel;
    U8 ucPU;
    BOOL bNfcFlag, RstStatus;
    FLASH_ADDR tFlashAddr = { 0 };
    U32 HPLL;
    PTABLE *pPTable;
    BOOTLOADER_FILE *pBootLoaderFile;
    U32 ulSys0CEMap;
    U32 ulSys1CEMap;

    pBootLoaderFile = BL_GetBootLoaderFile();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    ulSys0CEMap = HAL_GetSubSystemCEMap(MCU1_ID);
    ulSys1CEMap = HAL_GetSubSystemCEMap(MCU2_ID);

    /* if not LLF or Warm Boot, don't need to reset. Rom code already done */
    if ((TRUE == pPTable->sBootStaticFlag.bsWarmBoot) ||
        ((BOOT_METHOD_NORMAL != pPTable->sBootStaticFlag.bsBootMethodSel) &&
        (0 == BL_CalBitsNumByMap(ulSys0CEMap | ulSys0CEMap)))
       )
    {
        for (ucCESel = 0; ucCESel < SCAN_CE_PER_PU; ucCESel++)
        {
            tFlashAddr.ucLun = ucCESel * LUN_PER_CE;

            for (ucPU = 0; ucPU < NFC_PU_MAX; ucPU++)
            {
                tFlashAddr.ucPU = ucPU;

                if (NFC_STATUS_SUCCESS != HAL_NfcResetFlashNoWaitStatus(&tFlashAddr))
                {
                    DBG_Printf("Reset pu:%d cmd send  fail\n", tFlashAddr.ucPU);
                    continue;
                }
            }

            HAL_DelayUs(5000); /* Delay 5ms  for LLF */

            for (ucPU = 0; ucPU < NFC_PU_MAX; ucPU++)
            {
                tFlashAddr.ucPU = ucPU;

                bNfcFlag = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
                if (NFC_STATUS_SUCCESS != bNfcFlag)
                {
                    DBG_Printf("Reset pu:%d fail\n", tFlashAddr.ucPU);
                    continue;
                }
            }
        }
    }
#endif

    return;
}

/*------------------------------------------------------------------------------
Name: BL_ResetAllFlashWaitStatus
Description:
    Reset exist flash.
    caution: on FPGA env, CE in channal which has no HW logic, default command
    sts is "d", no real cmd will be send.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    invoke it to Reset all exist flash.
History:
    20160901    DannierChen   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_ResetAllFlashWaitStatus(void)
{
#ifndef SIM
    U8 ucCESel;
    U8 ucPU;
    BOOL bNfcFlag, RstStatus;
    FLASH_ADDR tFlashAddr = { 0 };
    U32 HPLL;

    for (ucCESel = 0; ucCESel < SCAN_CE_PER_PU; ucCESel++)
    {
        tFlashAddr.ucLun = ucCESel * LUN_PER_CE;

        for (ucPU = 0; ucPU < l_ucPUNum; ucPU++)
        {
            tFlashAddr.ucPU = ucPU;

            if (NFC_STATUS_SUCCESS != HAL_NfcResetFlash(&tFlashAddr))
            {
                DBG_Printf("Reset pu:%d cmd send  fail\n", tFlashAddr.ucPU);
                continue;
            }
        }

        for (ucPU = 0; ucPU < l_ucPUNum; ucPU++)
        {
            tFlashAddr.ucPU = ucPU;

            bNfcFlag = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != bNfcFlag)
            {
                DBG_Printf("Reset pu:%d fail ErrCode=%d\n", tFlashAddr.ucPU, HAL_NfcGetErrCode(ucPU, tFlashAddr.ucLun));
                HAL_NfcResetCmdQue(tFlashAddr.ucPU, 0);
                HAL_NfcClearINTSts(tFlashAddr.ucPU, 0);
                HAL_NfcSinglePlnReadSts(&tFlashAddr);
                if (NFC_STATUS_SUCCESS == HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
                {
                    DBG_Printf("Reset pu:%d read status again pass\n", tFlashAddr.ucPU);
                }
                else
                {
                    DBG_Printf("Reset pu:%d lun%d read status again fail ErrCode=%d flashsts=%x\n",
                        tFlashAddr.ucPU, tFlashAddr.ucLun, HAL_NfcGetErrCode(ucPU, tFlashAddr.ucLun), 
                        *(volatile U8 *)(&g_pDecSramStsBase->aDecStsSram[ucPU][tFlashAddr.ucLun][HAL_NfcGetRP(ucPU, tFlashAddr.ucLun)][0]));
                }
                continue;
            }
        }
    }

#endif

    return;
}

/*------------------------------------------------------------------------------
Name: BL_GetHWSysCEBitMap
Description:
    scan all CE with ReadId CMD to statistic CEBitMap.
    caution:only read the first LUN(VPU) for one target at VPU mode.
Input Param:
    U8 *pPUCnt: pointer of CE count.
    U32 * pCEBitMap: pointer of CEBitMap.
Output Param:
    U8 *pPUCnt:result of CE count.
    U32 * pCEBitMap:result of CEBitMap.
Return Value:
    none
Usage:
    BL invoke it to Calculate CEBitMap at VPU mode.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_GetHWSysCEBitMap(U8 *pPUCnt, U32 *pCEBitMap)
{
    U8 ucCESel;
    U8 ucPU;
    U32 Id[2];
    U32 ulChipId0;
    U32 ulChipId1;
    U8 ucSubSystemCENum;
    PTABLE *pPTable;
    BOOTLOADER_FILE *pBootLoaderFile;
    U8 ucCECnt = 0;
    U32 aCEBitMap[SCAN_CE_PER_PU];
    U32 ulCEBitMap = INVALID_8F;
    U32 ulStatus;
    FLASH_ADDR tFlashAddr = {0};

    pBootLoaderFile = BL_GetBootLoaderFile();
    pPTable = &pBootLoaderFile->tSysParameterTable;

    ulChipId0 = pPTable->aFlashChipId[0];
    ulChipId1 = pPTable->aFlashChipId[1];

    ucSubSystemCENum = pPTable->ulSubSysCeNum;
    COM_MemZero(aCEBitMap, SCAN_CE_PER_PU);

    for (ucCESel = 0; ucCESel < SCAN_CE_PER_PU; ucCESel++)
    {
        // DBG_Printf("ce%d\n",ucCESel);

        tFlashAddr.ucLun = ucCESel * LUN_PER_CE;
        BL_ClearPUBitFlag();
        /* search exist PU */
        for (ucPU = 0; ucPU < NFC_PU_MAX; ucPU++)
        {
            tFlashAddr.ucPU = ucPU;

            if (TRUE ==  BL_IsPUNeedCELevleCmd(ucPU))
            {
                BL_SetAllBrotherPUDone(ucPU);

                HAL_NfcReadFlashId(&tFlashAddr);

                ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
                if (NFC_STATUS_SUCCESS != ulStatus)
                {
                    //DBG_Printf("PU %d Read ID fail!\n",tFlashAddr.ucPU);
                }

                /*    Get ID from SRAM  */
                HAL_DecSramGetFlashId(&tFlashAddr, Id);

                DBG_Printf("id0:0x%x id1:0x%x\n", Id[0], Id[1]);

                if ((Id[0] == ulChipId0) && (Id[1] == ulChipId1))
                {
                    aCEBitMap[ucCESel] |= (1<<ucPU);
                }
            }
            else
            {
                continue;
            }
        }
    }


    /*for Multi-CE case: one PU's all CE should be ReadId ok*/
    for (ucCESel = 0; ucCESel < SCAN_CE_PER_PU; ucCESel++)
    {
        ulCEBitMap &= aCEBitMap[ucCESel];
    }
    ucCECnt = BL_CalBitsNumByMap(ulCEBitMap);

#ifdef XTMP
    ulCEBitMap = 0xffffffff;
    ucCECnt = PU_MAX;
#endif

    *pPUCnt = ucCECnt;
    *pCEBitMap = ulCEBitMap;

    return;

}

/*------------------------------------------------------------------------------
Name: BL_CalcChannalNum
Description:
    Calculate Channal num accordint to CEBitMap.
Input Param:
    U32 ulCEMap:CEBitMap.
Output Param:
    none
Return Value:
    U8:Channal num.
Usage:
    BL invoke it to Calculate Channal num accordint to CEBitMap.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
U8 DRAM_BOOTLOADER_ATTR BL_CalcChannalNum(U32 ulCEMap)
{
    U8 ucChanNum;
    U8 ucPuInChan;
    U8 ucCENum;
    U8 ucChanlCnt = 0;

    for (ucChanNum = 0; ucChanNum < NFC_CH_TOTAL; ucChanNum++)
    {
        for (ucPuInChan = 0; ucPuInChan < NFC_PU_PER_CH; ucPuInChan++)
        {
            ucCENum = ucPuInChan * NFC_CH_TOTAL + ucChanNum;
            if (0 != (ulCEMap & (1<<ucCENum)))
            {
                ucChanlCnt++;
                break;
            }
        }
    }

    return ucChanlCnt;
}

/*------------------------------------------------------------------------------
Name: BL_AllotCEByChannal
Description:
    allocate CE for subsystems by odd or even channal num.
Input Param:
    U32 ulCEBitMap:System CEBitMap before allocation.
Output Param:
    U8 *pSys0CECnt:CE count allocated for subsystem0.
    U32 *pSys0CEBitMap:CEBitMap allocated for sbusystem0.
    U8 *pSys1CECnt:CE count allocated for subsystem1.
    U32 *pSys1CEBitMap:CEBitMap allocated for sbusystem1.
Return Value:
    none
Usage:
    BL invoke it to allocate CE for subsystems by odd or even channal num.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
void DRAM_BOOTLOADER_ATTR BL_AllotCEByChannal(U32 ulCEBitMap, U8 *pSys0CECnt, U32 *pSys0CEBitMap,
U8 *pSys1CECnt, U32 *pSys1CEBitMap)
{
    U8 ucCh;
    U8 ucCEInCh;
    U8 ucCENum;
    U8 ucSubsys0CECnt = 0;
    U8 ucSubsys1CECnt = 0;
    U32 ulSubsys0CEMap = 0;
    U32 ulSubsys1CEMap = 0;

    /* set subsystem PU mapping */
    for (ucCh = 0; ucCh < NFC_CH_TOTAL; ucCh++)
    {
        if (0 == (ucCh & 0x1))
        {
            for (ucCEInCh = 0; ucCEInCh < NFC_PU_PER_CH; ucCEInCh++)
            {
                ucCENum = (ucCEInCh * NFC_CH_TOTAL) + ucCh;
                if (((1 << ucCENum) & ulCEBitMap) != 0)
                {
                    ucSubsys0CECnt++;
                    ulSubsys0CEMap |= (1 << ucCENum);
                }
            }
        }
        else
        {
            for (ucCEInCh = 0; ucCEInCh < NFC_PU_PER_CH; ucCEInCh++)
            {
                ucCENum = (ucCEInCh * NFC_CH_TOTAL) + ucCh;
                if (((1 << ucCENum) & ulCEBitMap) != 0)
                {
                    ucSubsys1CECnt++;
                    ulSubsys1CEMap |= (1 << ucCENum);
                }
            }
        }
    }

    *pSys0CECnt = ucSubsys0CECnt;
    *pSys0CEBitMap = ulSubsys0CEMap;
    *pSys1CECnt = ucSubsys1CECnt;
    *pSys1CEBitMap = ulSubsys1CEMap;

    return;
}

/*------------------------------------------------------------------------------
Name: BL_AllotCEByCENum
Description:
    allocate CE for subsystems by CE number in channal.
Input Param:
    U32 ulCEBitMap:System CEBitMap before allocation.
Output Param:
    U8 *pSys0CECnt:CE count allocated for subsystem0.
    U32 *pSys0CEBitMap:CEBitMap allocated for sbusystem0.
    U8 *pSys1CECnt:CE count allocated for subsystem1.
    U32 *pSys1CEBitMap:CEBitMap allocated for sbusystem1.
Return Value:
    none
Usage:
    BL invoke it to allocate CE for subsystems by CE number in channal.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
void DRAM_BOOTLOADER_ATTR BL_AllotCEByCENum(U32 ulCEBitMap, U8 *pSys0CECnt, U32 *pSys0CEBitMap,
U8 *pSys1CECnt, U32 *pSys1CEBitMap)
{
    U8 ucCh;
    U8 ucCEInCh;
    U8 ucCENum;
    U8 ucSubsys0CECnt = 0;
    U8 ucSubsys1CECnt = 0;
    U32 ulSubsys0CEMap = 0;
    U32 ulSubsys1CEMap = 0;


    for (ucCEInCh = 0; ucCEInCh < NFC_PU_PER_CH; ucCEInCh++)
    {
        if (0 == (ucCEInCh&0x1))
        {
            for (ucCh = 0; ucCh < NFC_CH_TOTAL; ucCh++)
            {
                ucCENum = (ucCEInCh * NFC_CH_TOTAL) + ucCh;
                if (((1 << ucCENum) & ulCEBitMap) != 0)
                {
                    ucSubsys0CECnt++;
                    ulSubsys0CEMap |= (1 << ucCENum);
                }
            }
        }
        else
        {
            for (ucCh = 0; ucCh < NFC_CH_TOTAL; ucCh++)
            {
                ucCENum = (ucCEInCh * NFC_CH_TOTAL) + ucCh;
                if (((1 << ucCENum) & ulCEBitMap) != 0)
                {
                    ucSubsys1CECnt++;
                    ulSubsys1CEMap |= (1 << ucCENum);
                }
            }
        }
    }

    *pSys0CECnt = ucSubsys0CECnt;
    *pSys0CEBitMap = ulSubsys0CEMap;
    *pSys1CECnt = ucSubsys1CECnt;
    *pSys1CEBitMap = ulSubsys1CEMap;

    return;
}

/*------------------------------------------------------------------------------
Name: BL_UpdatePuMap
Description:
    scan all CE and prpare SubSysCEMap.
Input Param:
    U32 ulChipId0:low DW of flash chip id.
    U32 ulChipId1:high DW of flash chip id.
Output Param:
    none
Return Value:
    none
Usage:
    BootLoader invoke it to scan all CE and prpare SubSysCEMap.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_UpdatePuMap(void)
{
    U8 ucPU;
    U8 ucCECnt;
    U32 ulCEBitMap;
    U8 ucSubSystemCENum;
    U8 ucSubSysNeedCE;
    U8 ucSubSysCEMin;
    U8 ucChanNum;
    U8 ucCE = 0;
    U8 ucSubSystemCE = 0;
    U8 ucSubsys0CEIndex = 0;
    U8 ucSubsys1CEIndex = 0;
    U8 ucSubsys0CECnt = 0;
    U8 ucSubsys1CECnt = 0;
    U32 ulSubsys0CEMap = 0;
    U32 ulSubsys1CEMap = 0;
    U32 ulSubSys0LogicPUMap = 0;
    U32 ulSubSys1LogicPUMap = 0;
    PTABLE *pPTable;
    BOOTLOADER_FILE *pBootLoaderFile;

    pBootLoaderFile = BL_GetBootLoaderFile();
    pPTable = &pBootLoaderFile->tSysParameterTable;
    ucSubSystemCENum = pPTable->ulSubSysCeNum;

    /*when LLF, and pPTable->ulSubSysCEMap[] all zero, BootLoader need to san CEs,otherwise,
    use the pPTable->ulSubSysCEMap[] configured by BootScript or inform stored in flash*/
    if ((BOOT_METHOD_NORMAL != pPTable->sBootStaticFlag.bsBootMethodSel)
        && (0 == BL_CalBitsNumByMap(pPTable->ulSubSysCEMap[0] | pPTable->ulSubSysCEMap[1])))
    {
        BL_GetHWSysCEBitMap(&ucCECnt, &ulCEBitMap);

        DBG_Printf("ulCEBitMap:0x%x ucPUCnt:%d\n", ulCEBitMap, ucCECnt);

        ucSubSysNeedCE = ucSubSystemCENum;

        if (1 == pPTable->ulSubSysNum)
        {
            if (ucSubSysNeedCE <= ucCECnt)
            {
                while (ucSubSystemCE < ucSubSysNeedCE)
                {
                    if (0 != (ulCEBitMap & (1 << ucCE)))
                    {
                        ulSubSys1LogicPUMap |= 1 << ucCE;
                        ucSubSystemCE++;
                    }
                    ucCE++;
                }

                ulSubSys0LogicPUMap = ulSubSys1LogicPUMap;
            }
            else
            {
                DBG_Printf("gdb SubSysCeNum setting error\n");
                DBG_Getch();
            }
        }
        else if (2 == pPTable->ulSubSysNum)
        {
            ucChanNum = BL_CalcChannalNum(ulCEBitMap);

            if (0 == (ucChanNum&0x1))
            {
                BL_AllotCEByChannal(ulCEBitMap, &ucSubsys0CECnt, &ulSubsys0CEMap, &ucSubsys1CECnt, &ulSubsys1CEMap);
            }
            else
            {
                BL_AllotCEByCENum(ulCEBitMap, &ucSubsys0CECnt, &ulSubsys0CEMap, &ucSubsys1CECnt, &ulSubsys1CEMap);
            }

            ucSubSysCEMin = MIN(ucSubsys0CECnt, ucSubsys1CECnt);
            if (ucSubSysNeedCE <= ucSubSysCEMin)
            {
                for (ucPU = 0; ucPU < ucSubSysNeedCE; ucPU++)
                {
                    while (((1 << ucSubsys0CEIndex) & ulSubsys0CEMap) == 0)
                    {
                        ucSubsys0CEIndex++;
                    }
                    ulSubSys0LogicPUMap |= (1 << ucSubsys0CEIndex);
                    ucSubsys0CEIndex++;

                    while (((1 << ucSubsys1CEIndex) & ulSubsys1CEMap) == 0)
                    {
                        ucSubsys1CEIndex++;
                    }
                    ulSubSys1LogicPUMap |= (1 << ucSubsys1CEIndex);
                    ucSubsys1CEIndex++;
                }
            }
            else
            {
                DBG_Printf("gdb SubSysCeNum setting error\n");
                DBG_Getch();
            }
        }
        else
        {
            DBG_Printf("gdb ulSubSysNum setting error\n");
            DBG_Getch();
        }

        /* set PTable PU map*/
        pPTable->ulSubSysCEMap[0] = ulSubSys0LogicPUMap;
        pPTable->ulSubSysCEMap[1] = ulSubSys1LogicPUMap;

        DBG_Printf("ulSubSysCEMap[0] = ulSubSysCEMap[1] = %x\n", pPTable->ulSubSysCEMap[1]);
    }

    l_ucPUNum = BL_CalBitsNumByMap(pPTable->ulSubSysCEMap[0] | pPTable->ulSubSysCEMap[1]);

    return;
}


/*------------------------------------------------------------------------------
Name: HAL_NfcDefaultPuMapInit
Description:
    Initialize CE map, LogicPU map and  NFC LogicPUReg with one to one correspondence.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In rom or BootLoader. call this funtion to initialize CE and LogicPU map into
    a default mode.
History:
    20140911    Tobey   create.
------------------------------------------------------------------------------*/
void DRAM_BOOTLOADER_ATTR BL_NfcPuMapInit(U32 ulBitMap)
{
    HAL_NfcBuildPuMapping(ulBitMap);

    HAL_NfcSetLogicPUReg(ulBitMap);

    return;
}

 void DRAM_BOOTLOADER_ATTR BL_NFC_InterfaceInit(void)
{
    HAL_NfcInterfaceInitBasic();
    HAL_FlashNfcFeatureInit();
    HAL_DecStsInit();
    HAL_NfcDataSyntaxInit();
    HAL_LdpcInit();
    HAL_NfcQEEInit();

    /*add for low page mode when update BL FW */
    HAL_FlashInitSLCMapping();

    BL_NfcPuMapInit(INVALID_8F);
    HAL_NormalDsgInit();
}
DRAM_BOOTLOADER_ATTR void BL_NFC_UpdatePuMapping(void)
{
    U32 ulSys0CEMap;
    U32 ulSys1CEMap;

    BL_UpdatePuMap();
    ulSys0CEMap = HAL_GetSubSystemCEMap(MCU1_ID);
    ulSys1CEMap = HAL_GetSubSystemCEMap(MCU2_ID);
    BL_NfcPuMapInit(ulSys0CEMap | ulSys1CEMap);
}

DRAM_BOOTLOADER_ATTR void BL_EraseAllPUGB(U8 ucTotalPU)
{
    U8 ucPUIndex;
    FLASH_ADDR tFlashAddr = {0};

    for (ucPUIndex = 0; ucPUIndex < ucTotalPU; ucPUIndex++)
    {
        //DBG_Printf("e %d\n", ucPUIndex);
        tFlashAddr.ucPU = ucPUIndex;

        if (NFC_STATUS_SUCCESS != HAL_NfcSingleBlockErase(&tFlashAddr, TRUE))
        {
            DBG_Printf("PU%d erase GB cmd send fail!!!\n", ucPUIndex);
            DBG_Getch();
        }
    }

    for (ucPUIndex = 0; ucPUIndex < ucTotalPU; ucPUIndex++)
    {
        //DBG_Printf("w %d\n", ucPUIndex);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPUIndex, 0))
        {
            DBG_Printf("PU%d erase GB error!!!\n", ucPUIndex);
            DBG_Getch();
        }
    }
    return;
}

/*------------------------------------------------------------------------------
Name: BL_GlobalInit
Description:
    global registers init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init global registers.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_GlobalInit(void)
{
    //DBG_Printf("GlobalInit-Start\n");
    BL_HWInitCallBackCommon(INIT_GLOBAL);

#if !defined(XTMP) && !defined(SIM)
    uart_init();
#endif

    DBG_Printf("GlobalInit-End\n");

    return;
}

/*------------------------------------------------------------------------------
Name: BL_DDRInit
Description:
    DDR init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init HW DDR
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
void BL_DDRInit(void)
{
    //DBG_Printf("DDRInit-Start\n");
    BL_HWInitCallBackCommon(INIT_DDR);
    DBG_Printf("DDRInit-End\n");

    return;
}

/*------------------------------------------------------------------------------
Name: BL_Flash1stInit
Description:
    Flash 1ST init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init HW Flash 1ST stage
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_Flash1stInit(void)
{
    //DBG_Printf("NFC1stInit-Start\n");
    return;
}

/*------------------------------------------------------------------------------
Name: BL_Flash2ndInit
Description:
    Flash 2ND init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init HW Flash 2ND stage
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_Flash2ndInit(void)
{
    //DBG_Printf("NFC2ndInit-Start\n");
    return;
}

/*------------------------------------------------------------------------------
Name: BL_NFCInit
Description:
    NFC init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init HW NFC
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_NFCInit(void)
{
    BOOTLOADER_FILE *pBootLoaderFile;
    HW_INIT_FLAG *pHWInitFlag;

    //DBG_Printf("NFCInit-Start\n");
    pBootLoaderFile = BL_GetBootLoaderFile();
    pHWInitFlag = &pBootLoaderFile->tSysParameterTable.sHwInitFlag;

    // sinice BLScript do not aware of Flash1st and Flash2st, we have to
    //combine BL_Flash1stInit() and BL_Flash2ndInit() .
    BL_HWInitCallBackCommon(INIT_NFC);
    //DBG_Printf("NFCinit:E\n");
    /*useful when FW heat powerup, FW Invoke NFCInitFunc by call back interface*/
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
    //DBG_Printf("NFCinit:F\n");
    pHWInitFlag->bsFlash1STDone = TRUE;
    pHWInitFlag->bsFlash2NDDone = TRUE;

    DBG_Printf("NFCInit-End\n");

    return;
}

/*------------------------------------------------------------------------------
Name: BL_PCIEInit
Description:
    PCIE init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init HW PCIE
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_PCIEInit(void)
{

    if (0 != (rGLB(0x40) & (1 << 3)))
    {
        if (FALSE == HAL_GetWarmBootFlag())
        {
             /*get UARTMP mode, if high, uart mp mode*/
             if (HAL_UartIsMp() == FALSE) 
             {           
                
                    /* Prevents PCIe link down from resetting IRS. */
                    rPCIe(0x208) &= ~((1 << 10) | (1 << 11));

                    /* Asserts EP_INIT on cold boot for prevent host from attrieving configuration data too early. */
                    rHOSTC(0x50) |= (1 << 31);
             }    
        }
    }

    BL_HWInitCallBackCommon(INIT_PCIE);
    DBG_Printf("pcie backward call end\n");

    return;
}

/*------------------------------------------------------------------------------
Name: BL_ClkGatingInit
Description:
    ClockGating init function.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init HW ClockGating
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_ClkGatingInit(void)
{
    BL_HWInitCallBackCommon(INIT_CLK_GATING);
    DBG_Printf("clock backward call end\n");

    return;
}

/*------------------------------------------------------------------------------
Name: BL_GetBufferIDByMemAddr
Description:
    calculate buffer id by dram or otfb addr.
Input Param:
    U32 TargetAddr: dram or otfb addr
    U8 bDram: TRUE: dram addr; FALSE: otfb addr
    U8 BufferSizeBits: bits of buffer size.
Output Param:
    none
Return Value:
    U32: buff id
Usage:
    BL invoke it to calculate buffer id by dram or otfb addr.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR U32 BL_GetBufferIDByMemAddr(U32 TargetAddr, U8 bDram, U8 BufferSizeBits)
{
    U32 BuffID;
    U32 MemStartAddr;

    MemStartAddr = bDram ? DRAM_START_ADDRESS : OTFB_START_ADDRESS;

    BuffID = (TargetAddr - MemStartAddr) >> BufferSizeBits;

    return BuffID;
}

/*------------------------------------------------------------------------------
Name: BL_IsFWPageNeedSave
Description:
    check FW's special page need be saved to flash or not.
Input Param:
    U8 ucPageNum: pge num.
Output Param:
    none
Return Value:
    BOOL: TRUE: need be saved; FALSE: don't need
Usage:
    BL invoke it to check FW's special page need be saved to flash or not.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR BOOL BL_IsFWPageNeedSave(U8 ucPageNum)
{
    if (ucPageNum < 32)
    {
        if (0 != (FW_PAGE_BITMAP_LOW&(1<<ucPageNum)))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        if (0 != (FW_PAGE_BITMAP_HIGH&(1<<(ucPageNum - 32))))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}


/*------------------------------------------------------------------------------
Name: BL_GetCurrentPU
Description:
    search the current PU which has other content except for BootLoader and FW V0
Input Param:
    U8 ucScanStartPage: every PU's search page num
    U8 ucTotalPU: total PU num
Output Param:
    none
Return Value:
    U8: the current PU num
Usage:
    BL invoke it to search the current PU.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR U8 BL_GetCurrentPU(U8 ucScanStartPage, U8 ucTotalPU)
{
    U8 ucCurPU;
    U8 ucPUNum;
    U8 ucIndex;
    U16 usBuffID;
    U32 ulPUBitFlag;
    U32 ulPhyPageSize;
    U32 ulPhyPageSizeBits;
    BOOL bFlashOpFlag;
    FLASH_ADDR tFlashAddr = {0};
    NFC_READ_REQ_DES tRdDes = {0};

    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.usPage = ucScanStartPage;
    ulPUBitFlag = 0;
    ulPhyPageSize = BL_GetFlashPhyPageSize();
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usBuffID = BL_GetBufferIDByMemAddr(BOOTLOADER_BUFF0_ADDR, TRUE, 10);
    for (ucIndex = 0; ucIndex < ucTotalPU; ucIndex++)
    {
        tFlashAddr.ucPU = ucIndex;

        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = (ulPhyPageSize>>SEC_SIZE_BITS) - 2;
        tRdDes.bsRdBuffId = usBuffID;
        tRdDes.ppNfcRed = NULL;
        tRdDes.bsDsIndex = DS_15K_CRC;
        tRdDes.pErrInj = NULL;

        //DBG_Printf("BL_GetCurrentPU%d page%d\n", tFlashAddr.ucPU, tFlashAddr.usPage);
        if (NFC_STATUS_SUCCESS != HAL_NfcSinglePlnRead(&tFlashAddr, &tRdDes, FALSE))
        {
            DBG_Printf("BL_GetCurrentPU%d page%d send fail!!\n", tFlashAddr.ucPU, tFlashAddr.usPage);
        }

        bFlashOpFlag = HAL_NfcWaitStatus(ucIndex, 0);

        if (NFC_STATUS_SUCCESS == bFlashOpFlag)
        {
            ulPUBitFlag |= (1<<ucIndex);
            ucCurPU = ucIndex;
        }
        else
        {
            HAL_NfcResetCmdQue(tFlashAddr.ucPU, 0);
            HAL_NfcClearINTSts(tFlashAddr.ucPU, 0);
        }
    }

    //DBG_Printf("get cur pu start page:%d\n", ucScanStartPage);
    //DBG_Printf("get cur pu ulPUBitFlag:%x\n", ulPUBitFlag);

    ucPUNum = BL_CalBitsNumByMap(ulPUBitFlag);
    if (0 == ucPUNum)
    {
        ucCurPU = 0;
    }

    if (ucPUNum >= 2)
    {
        /*error handle*/
    }

    return ucCurPU;
}

/*------------------------------------------------------------------------------
Name: BL_SetGBPageType
Description:
    Set global block page type.
Input Param:
    U8 ucPageType:page type
    BL_REDUNDANT * pBLRed:redundant pointer
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to Set global block page type.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_SetGBPageType(U8 ucPageType, BL_REDUNDANT * pBLRed)
{
#ifdef SIM
    pBLRed->m_RedComm.bcPageType = ucPageType;
#else
    pBLRed->m_GBPageType = ucPageType;
#endif
}

/*------------------------------------------------------------------------------
Name: BL_GetFWRealSavePageNum
Description:
    calculate the real total page fo FW which should be saved to flash.
Input Param:
    none
Output Param:
    none
Return Value:
    U8: real total page num
Usage:
    BL invoke it to calculate the real total page fo FW which should be saved to flash.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR U8 BL_GetFWRealSavePageNum(void)
{
    U8 ucIndex;
    U8 ucFWTotalPageNum;
    U8 ucFWSavePageNum = 0;

    ucFWTotalPageNum = BL_GetFWSavePageCnt();

    for (ucIndex = 0; ucIndex < ucFWTotalPageNum; ucIndex++)
    {
        if (TRUE == BL_IsFWPageNeedSave(ucIndex))
        {
            ucFWSavePageNum++;
        }
    }

    return ucFWSavePageNum;
}

/*------------------------------------------------------------------------------
Name: BL_InitFWUpdateInfo
Description:
    Init FWUpdateInfo by scan current PU all page.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to Init FWUpdateInfo.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_InitFWUpdateInfo(void)
{
    U8 ucCurPU;
    U8 ucTotalPU;
    U8 ucScanStartPage;
    U8 ucFWRealSavePage;
    U32 ulPageIndex;
    U32 ulPagePerBLK;
    U16 usBuffID;
    U32 ulPhyPageSize;
    U32 ulPhyPageSizeBits;
    BOOL bFlashOpFlag;
    BL_REDUNDANT *pBLRed;
    FLASH_ADDR tFlashAddr;
    NFC_READ_REQ_DES tRdDes = {0};

    /*calculate FW real save num*/
    ucFWRealSavePage = BL_GetSysImgPageCnt(HEADER_LOCATION);

    /*get current pu*/
    ucScanStartPage = ucFWRealSavePage + (BL_TOTAL_SIZE>>LOGIC_PG_SZ_BITS); //4 //4 include BL, pattern page and CE_BBT
    ucTotalPU = BL_CalBitsNumByMap(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    //ucCurPU = BL_GetCurrentPU(ucScanStartPage, ucTotalPU);
    ucCurPU = 0;

    tFlashAddr.ucPU = ucCurPU;
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usBlock = 0;
    COM_MemZero((U32 *) &l_tFWUpdateInfo, sizeof(BL_FW_UPDATE_INFO)>>2);
    l_tFWUpdateInfo.ucCurrPU = ucCurPU;
    l_tFWUpdateInfo.ucNextPU = (ucCurPU == (ucTotalPU-1)) ? 0:(ucCurPU+1);
    l_tFWUpdateInfo.ulOtherPUEmpPageStart = ucScanStartPage;
    l_tFWUpdateInfo.ucFwRealSavePGNum = ucFWRealSavePage;
    l_tFWUpdateInfo.ucTotalPU = ucTotalPU;

    ulPagePerBLK = (BL_GetFlashBlkPageNum()/2);
    ulPhyPageSize = BL_GetFlashPhyPageSize();
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usBuffID = BL_GetBufferIDByMemAddr(BOOTLOADER_BUFF0_ADDR, TRUE, 10);
    for (ulPageIndex = ucScanStartPage; ulPageIndex < ulPagePerBLK; )
    {
        tFlashAddr.usPage = HAL_FlashGetSLCPage(ulPageIndex);

        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = (ulPhyPageSize>>SEC_SIZE_BITS) - 2;
        tRdDes.bsRdBuffId = usBuffID;
        tRdDes.ppNfcRed = (NFC_RED **)&pBLRed;
        tRdDes.bsDsIndex = DS_15K_CRC;
        tRdDes.pErrInj = NULL;

        //HAL_NfcSinglePlnRead_A(&tFlashAddr, 0, ulPhyPageSize>>SEC_SIZE_BITS, FALSE, usBuffID, (NFC_RED **)&pBLRed);
        HAL_NfcSinglePlnRead(&tFlashAddr, &tRdDes, FALSE);
        bFlashOpFlag = HAL_NfcWaitStatus(tFlashAddr.ucPU, 0);
        if (NFC_STATUS_SUCCESS == bFlashOpFlag)
        {
#ifdef SIM
            switch (pBLRed->m_RedComm.bcPageType)
#else
            switch (pBLRed->m_GBPageType)
#endif
            {
                case BL_PG_TYPE_FIRMWARE:
                    if (0 == pBLRed->m_FWPageNum)
                    {
                        l_tFWUpdateInfo.ucCurPUExistFlag |= 1<<BL_GB_VX_BIT;
                        l_tFWUpdateInfo.ulCurPUVlastStartPage = ulPageIndex;
                        ulPageIndex += (ucFWRealSavePage-1);
                    }
                    else if ((ucFWRealSavePage-1) == pBLRed->m_FWPageNum)
                    {
                        ulPageIndex++;
                    }
                    else
                    {
                        DBG_Printf("BL_InitFWUpdateInfo error!!\n");
                        DBG_Getch(); //current PU'S VX should be all entire
                    }
                    break;
                case BL_PG_TYPE_ACTIVE0:
                    l_tFWUpdateInfo.ucCurPUExistFlag |= 1<<BL_GB_AX_BIT;
                    l_tFWUpdateInfo.ucCurPUAlast = BL_PG_TYPE_ACTIVE0;
                    ulPageIndex++;
                    break;
                case BL_PG_TYPE_ACTIVE1:
                    l_tFWUpdateInfo.ucCurPUExistFlag |= 1<<BL_GB_AX_BIT;
                    l_tFWUpdateInfo.ucCurPUAlast = BL_PG_TYPE_ACTIVE1;
                    ulPageIndex++;
                    break;
                case BL_PG_TYPE_PTable:
                    l_tFWUpdateInfo.ucCurPUExistFlag |= 1<<BL_GB_PT_BIT;
                    ulPageIndex++;
                    break;
                default:
                    break;
           }
        }
        else
        {
            HAL_NfcResetCmdQue(tFlashAddr.ucPU, 0);
            HAL_NfcClearINTSts(tFlashAddr.ucPU, 0);
            break;
        }
    }
    l_tFWUpdateInfo.ulCurPUEmpPageStart = ulPageIndex;

    return;
}



#define LEN_15K
/*------------------------------------------------------------------------------
Name: BL_WriteBL2FlashGB
Description:
    write FW image to a special flash global block.
Input Param:
    U8 ucPU:PU num
    U32 ulStartPageNum:start page num for FW image save
    U32 ulDramAddr:FW image's dram start addr.
    U8 ucFWRealSavePage:FW image's real save num.
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to write FW image to a special flash global block.
History:
    20150831    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_WriteBL2FlashGB(U8 ucPU, U32 ulStartPageNum, U32 ulDramAddr, U8 ucFWRealSavePage)
{
    U8 ucPageIndex;
    U8 ucRealPageIndex;
    U16 usBuffID;
    U16 usStartBuffID;
    FLASH_ADDR tFlashAddr;
    BL_REDUNDANT tBLRed;
    U32 ulPhyPageSizeBits;
    NFC_PRG_REQ_DES tWrDes = {0};

    COM_MemZero((U32 *)&tBLRed, sizeof(BL_REDUNDANT)/sizeof(U32));
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    #ifdef LEN_15K
    usStartBuffID = BL_GetBufferIDByMemAddr(ulDramAddr, TRUE, 10);
    #else
    usStartBuffID = BL_GetBufferIDByMemAddr(ulDramAddr, TRUE, 14);
    #endif

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.bsPln = 0;

    BL_SetGBPageType(BL_PG_TYPE_LOADER, &tBLRed);
    ucPageIndex = 0;
    while (ucPageIndex < ucFWRealSavePage)
    {
        #ifdef LEN_15K
        usBuffID = usStartBuffID + ucPageIndex * 15;
        #else
        usBuffID = usStartBuffID + ucPageIndex;
        #endif
        tFlashAddr.usPage = HAL_FlashGetSLCPage(ucPageIndex + ulStartPageNum);
        //DBG_Printf("-wr PU%d pg=%d orpg=%d\n", ucPU, tFlashAddr.usPage, ucPageIndex + ulStartPageNum);
        tWrDes.bsWrBuffId = usBuffID;
        tWrDes.bsRedOntf = TRUE;
        tWrDes.pNfcRed = (NFC_RED *)&tBLRed;
        #ifdef LEN_15K
        tWrDes.bsDsIndex = DS_15K_CRC;
        #else
        tWrDes.bsDsIndex = DS_ENTRY_SEL;
        #endif
        tWrDes.pErrInj = NULL;
        tWrDes.bsTlcMode = FALSE;

        HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrDes);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, 0))
        {
            DBG_Printf("BL_WriteBL2FlashGB pu:%d page:%d fail!!\n", ucPU, ucPageIndex);
            DBG_Getch();
        }
        ucPageIndex++;
    }

}

/*------------------------------------------------------------------------------
Name: BL_WriteFW2FlashGB
Description:
    write FW image to a special flash global block.
Input Param:
    U8 ucPU:PU num
    U32 ulStartPageNum:start page num for FW image save
    U32 ulDramAddr:FW image's dram start addr.
    U8 ucFWRealSavePage:FW image's real save num.
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to write FW image to a special flash global block.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_WriteFW2FlashGB(U8 ucPU, U32 ulStartPageNum, U32 ulDramAddr, U8 ucFWRealSavePage)
{
    U8 ucPageIndex;
    U8 ucRealPageIndex;
    U16 usBuffID;
    U16 usStartBuffID;
    FLASH_ADDR tFlashAddr;
    BL_REDUNDANT tBLRed;
    U32 ulPhyPageSizeBits;
    NFC_PRG_REQ_DES tWrDes = {0};

    COM_MemZero((U32 *)&tBLRed, sizeof(BL_REDUNDANT)/sizeof(U32));
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usStartBuffID = BL_GetBufferIDByMemAddr(ulDramAddr, TRUE, 10);

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.bsPln = 0;

    BL_SetGBPageType(BL_PG_TYPE_FIRMWARE, &tBLRed);

    ucPageIndex = 0;
    ucRealPageIndex = 0;
    while (ucRealPageIndex < ucFWRealSavePage)
    {
        if (TRUE == BL_IsFWPageNeedSave(ucPageIndex))
        {
            usBuffID = usStartBuffID + ucPageIndex * 15;
            tFlashAddr.usPage = HAL_FlashGetSLCPage(ucRealPageIndex + ulStartPageNum);
            tBLRed.m_FWPageNum = ucRealPageIndex;
            ucRealPageIndex++;
            tWrDes.bsTlcMode = 0;
            tWrDes.bsWrBuffId = usBuffID;
            tWrDes.bsRedOntf = TRUE;
            tWrDes.pNfcRed = (NFC_RED *)&tBLRed;
            tWrDes.bsDsIndex = DS_15K_CRC;
            tWrDes.pErrInj = NULL;

            HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrDes);
            if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, 0))
            {
                DBG_Printf("BL_WriteFW2FlashGB pu:%d page:%d fail!!\n", ucPU, ucRealPageIndex);
                DBG_Getch();
            }
        }
        ucPageIndex++;
    }
    return;
}

/*------------------------------------------------------------------------------
Name: BL_WriteAX2FlashGB
Description:
    write AX(active FW entry) to a special flash global block.
Input Param:
    U8 ucPU:PU num
    U32 ulPageNum:page num
    U8 ucAXType:AX type
    U32 ulDramAddr:AX entry dram addr.
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to write AX(active FW entry) to a special flash global block.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_WriteAX2FlashGB(U8 ucPU, U32 ulPageNum, U8 ucAXType, U32 ulDramAddr)
{
    U16 usBuffID;
    FLASH_ADDR tFlashAddr;
    BL_REDUNDANT tBLRed;
    U32 ulPhyPageSizeBits;
    NFC_PRG_REQ_DES tWrDes = {0};

    COM_MemZero((U32 *)&tBLRed, sizeof(BL_REDUNDANT)/sizeof(U32));
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usBuffID = BL_GetBufferIDByMemAddr(ulDramAddr, TRUE, 10);

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usPage = HAL_FlashGetSLCPage(ulPageNum);

    BL_SetGBPageType(ucAXType, &tBLRed);

    tWrDes.bsWrBuffId = usBuffID;
    tWrDes.bsRedOntf = TRUE;
    tWrDes.pNfcRed = (NFC_RED *)&tBLRed;
    tWrDes.bsDsIndex = DS_15K_CRC;
    tWrDes.pErrInj = NULL;

    //HAL_NfcSinglePlaneWrite_A(&tFlashAddr, usBuffID, (NFC_RED *)&tBLRed);
    HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrDes);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, 0))
    {
        DBG_Printf("BL_WriteAX2FlashGB fail!!\n");
        DBG_Getch();
    }

    return;
}

/*------------------------------------------------------------------------------
Name: BL_WritePT2FlashGB
Description:
    write PT(PTable active debugmode entry) to a special flash global block.
Input Param:
    U8 ucPU:PU num
    U32 ulPageNum:page num
    U32 ulDramAddr:AX entry dram addr.
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to write PT(PTable active debugmode entry) to a special flash global block.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_WritePT2FlashGB(U8 ucPU, U32 ulPageNum, U32 ulDramAddr)
{
    U16 usBuffID;
    FLASH_ADDR tFlashAddr;
    BL_REDUNDANT tBLRed;
    U32 ulPhyPageSizeBits;
    NFC_PRG_REQ_DES tWrDes = {0};

    COM_MemZero((U32 *)&tBLRed, sizeof(BL_REDUNDANT)/sizeof(U32));
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usBuffID = BL_GetBufferIDByMemAddr(ulDramAddr, TRUE, 10);

    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usPage = HAL_FlashGetSLCPage(ulPageNum);

    //tBLRed.m_GBPageType = BL_PG_TYPE_PTable;
    BL_SetGBPageType(BL_PG_TYPE_PTable, &tBLRed);

    tWrDes.bsWrBuffId = usBuffID;
    tWrDes.bsRedOntf = TRUE;
    tWrDes.pNfcRed = (NFC_RED *)&tBLRed;
    tWrDes.bsDsIndex = DS_15K_CRC;
    tWrDes.pErrInj = NULL;

    //HAL_NfcSinglePlaneWrite_A(&tFlashAddr, usBuffID, (NFC_RED *)&tBLRed);
    HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrDes);

    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, 0))
    {
        DBG_Printf("BL_WritePT2FlashGB fail!!\n");
        DBG_Getch();
    }

    return;
}


DRAM_BOOTLOADER_ATTR U32 BL_ReadGB2Dram(U8 ucPU, U32 ulTargetAddr, BL_REDUNDANT *pRedBuf)
{
    U16 usBuffID;
    U32 ulPGIndex, ulReadPGCnt = 0;
    FLASH_ADDR tSrcFlashAddr = {0};
    U32 ulPhyPageSize;
    U32 ulPhyPageSizeBits;
    U32 ulPhyPageNum;
    NFC_RED *pRed;
    NFC_READ_REQ_DES tRdDes = {0};

    ulPhyPageSize = BL_GetFlashPhyPageSize();
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    ulPhyPageNum = BL_GetFlashBlkPageNum();
    usBuffID = BL_GetBufferIDByMemAddr(ulTargetAddr, TRUE, 10);

    COM_MemZero((U32 *)&tSrcFlashAddr, sizeof(FLASH_ADDR)>>2);

    for (ulPGIndex = 0; ulPGIndex < ulPhyPageNum; ulPGIndex++)
    {
        tSrcFlashAddr.ucPU = ucPU;
        tSrcFlashAddr.usPage = HAL_FlashGetSLCPage(ulPGIndex);

        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = (ulPhyPageSize>>SEC_SIZE_BITS) - 2;
        tRdDes.bsRdBuffId = usBuffID;
        tRdDes.ppNfcRed = &pRed;
        tRdDes.bsDsIndex = DS_15K_CRC;
        tRdDes.pErrInj = NULL;

        //HAL_NfcSinglePlnRead_A(&tSrcFlashAddr, 0, ulPhyPageSize>>SEC_SIZE_BITS, FALSE, usBuffID, &pRed);
        HAL_NfcSinglePlnRead(&tSrcFlashAddr, &tRdDes, FALSE);

        if (NFC_STATUS_FAIL == HAL_NfcWaitStatus(tSrcFlashAddr.ucPU, 0))
        {
            if (TRUE == HAL_NfcDetectEmptyPage(pRed, HAL_NfcGetErrCode(ucPU, 0)))
            {
                HAL_NfcResetCmdQue(ucPU, 0);
                HAL_NfcClearINTSts(ucPU, 0);
                return ulReadPGCnt;
            }
            else
            {
                // temp patch for empty page check
                BL_REDUNDANT *pCurRed = (BL_REDUNDANT *)pRed;
                U32 j, num = 0;

                //  if number of '0' less than 28 then the page should be empty

                for (j = 0; j < 32; j++)
                {
                    if (0 == (pCurRed->ulEmptyPgParity&(1<<j)))
                    {
                        num++;
                    }
                }

                if (num < 28)
                {
                    // firmware comfirms empty page
                    HAL_NfcResetCmdQue(ucPU, 0);
                    HAL_NfcClearINTSts(ucPU, 0);
                    return ulReadPGCnt;
                }
                else
                {
                    DBG_Printf("PU#%d read fail! error code is %d\n", ucPU, HAL_NfcGetErrCode(ucPU, 0));
                    DBG_Getch();
                }
            }
        }

        if (NULL != pRedBuf)
        {
            // back up red
            COM_MemCpy((U32 *)(&pRedBuf[ulPGIndex]), (U32 *)(pRed), sizeof(BL_REDUNDANT)>>2);
        }
        usBuffID += 15;
        ulReadPGCnt++;

    }

    return ulReadPGCnt;

}

DRAM_BOOTLOADER_ATTR void BL_WriteGB2Flash(U8 ucPu, U32 ulPageNum, U32 ulDramAddr, BL_REDUNDANT *pRedBuf)
{
    U32 ucPageIndex;
    U32 ucRealPageIndex;
    U16 usBuffID;
    U16 usStartBuffID;
    FLASH_ADDR tFlashAddr;
    BL_REDUNDANT tBLRed;
    U32 ulPhyPageSizeBits;
    NFC_PRG_REQ_DES tWrDes = {0};

    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usStartBuffID = BL_GetBufferIDByMemAddr(ulDramAddr, TRUE, 10);

    tFlashAddr.ucPU = ucPu;
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usPage = 0;

    if (NFC_STATUS_SUCCESS != HAL_NfcSingleBlockErase(&tFlashAddr, FALSE))
    {
        DBG_Printf("cmd send fail!!!\n");
        DBG_Getch();
    }

    //DBG_Printf("Erase GB of PU #%d.\n", tFlashAddr.ucPU);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, 0))
    {
        DBG_Printf("Erase PU#%d fail!!\n");
        DBG_Getch();
    }

    for (ucPageIndex = 0; ucPageIndex < ulPageNum; ucPageIndex++)
    {
        tFlashAddr.usPage = HAL_FlashGetSLCPage(ucPageIndex);
        COM_MemCpy((U32 *)&tBLRed, (U32 *)&pRedBuf[ucPageIndex], sizeof(BL_REDUNDANT)>>2);

        tWrDes.bsWrBuffId = usStartBuffID;
        tWrDes.bsRedOntf = TRUE;
        tWrDes.pNfcRed = (NFC_RED *)&tBLRed;
        tWrDes.bsDsIndex = DS_15K_CRC;
        tWrDes.pErrInj = NULL;

        //HAL_NfcSinglePlaneWrite_A(&tFlashAddr, usStartBuffID, (NFC_RED *)&tBLRed);
        HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrDes);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, 0))
        {
            DBG_Printf("Write PU#%d ,PG#%d fail.\n", ucPu, ucPageIndex);
            DBG_Getch();
        }
        usStartBuffID += 15;
    }

    return;
}

DRAM_BOOTLOADER_ATTR void BL_SaveBLProc(U32 ulSrcBootloaderAddr, U32 ulBuf, U32 ulRedBuf)
{
    U32 ulPu, ulTotalPU;
    U32 ulPageNum;
    U32 aCRC[32];
    BOOTLOADER_FILE *pImgBootLoaderFile;
    BOOTLOADER_FILE *pCurBootloaderFile;

    ulTotalPU = BL_CalBitsNumByMap(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    for (ulPu = 0; ulPu < ulTotalPU; ulPu++)
    {
        DBG_Printf("Update BL PU #%d\n", ulPu);
        ulPageNum = BL_ReadGB2Dram(ulPu, ulBuf, (BL_REDUNDANT *)ulRedBuf);
        //DBG_Printf("Page number of GB is %d\n", ulPageNum);
        COM_MemCpy((U32 *)ulBuf, (U32 *)ulSrcBootloaderAddr, BL_TOTAL_SIZE>>2);

        pImgBootLoaderFile = (BOOTLOADER_FILE *)ulBuf;
        pCurBootloaderFile = (BOOTLOADER_FILE *)(OTFB_BOOTLOADER_BASE);
        pImgBootLoaderFile->tSysParameterTable.sHwInitFlag.ulHWInitFlag = 0;
        pImgBootLoaderFile->tSysParameterTable.sBootStaticFlag.bsBootMethodSel = BOOT_METHOD_NORMAL;
        pImgBootLoaderFile->tSysParameterTable.ulSubSysCEMap[0]
         = pCurBootloaderFile->tSysParameterTable.ulSubSysCEMap[0] ;
        pImgBootLoaderFile->tSysParameterTable.ulSubSysCEMap[1]
         = pCurBootloaderFile->tSysParameterTable.ulSubSysCEMap[1] ;

#if 0
        // CRC check
        {
            U32 i;
            aCRC[ulPu] = 0;
            for (i = 0; i < (ulPageNum*16*1024/sizeof(U32)); i++)
                aCRC[ulPu] ^= *(volatile U32 *)(ulBuf + i*4);
            //DBG_Printf("PU #%d ,GB CRC is 0x%x\n", ulPu, aCRC[ulPu]);
        }
#endif
        BL_WriteGB2Flash(ulPu, ulPageNum, ulBuf, (BL_REDUNDANT *)ulRedBuf);
    }

#if 0
    for (ulPu = 0; ulPu < ulTotalPU; ulPu++)
    {
        ulPageNum = BL_ReadGB2Dram(ulPu, DRAM_DATA_BUFF_MCU1_BASE+0x200000, NULL);

        // CRC check
        {
            U32 i;
            U32 ulTempBuf = DRAM_DATA_BUFF_MCU1_BASE+0x200000;
            for (i = 0; i < (ulPageNum*16*1024/sizeof(U32)); i++)
            {
                aCRC[ulPu] ^= *(volatile U32 *)(ulTempBuf + i*4);
            }
            if (aCRC[ulPu] != 0)
            {
                DBG_Printf("PU #%d ,Save BL error.\n", ulPu);
                DBG_Getch();
            }
            else
            {
                DBG_Printf("PU #%d , Check GB CRC pass.\n", ulPu);
            }
        }
    }
#endif

    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
}


/*------------------------------------------------------------------------------
Name: BL_SwitchMode
Description:
    BL provides an interface for L0 to switch to flash sync/async mode.
Input Param:
    BOOL ucSwitchToSync: TRUE: switch to sync mode / FALSE: switch to async mode
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to switch to flash sync/async mode.
    And you can add the BL_CHECK_SWITCH_MODE define to check the feature. 
History:
    20171226    Henry   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_SwitchMode(BOOL ucSwitchToSync)
{
//Only For TSB 3D TLC Bics3
#if (defined(FLASH_TLC) && defined(FLASH_TSB_3D))

#if BL_CHECK_SWITCH_MODE
    volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
    FLASH_ADDR tFlashAddr;
    NFC_READ_REQ_DES tRdDes = {0};
    U32 ulStatus;
    U32 ulPMU_bk;
#endif

    //only default async mode flash need
    if (BL_Part0StrapIsAsync())
    {
        if (ucSwitchToSync)
        {
            //Set to Sync Mode
            BL_SetFeatureAllFlash(INTERFACE_FEATURE_ADDR, INTERFACE_VALUE_TO_SYNC);
        }
        else
        {
            BL_SetFeatureAllFlash(INTERFACE_FEATURE_ADDR, INTERFACE_VALUE_TO_ASYNC);
        }
        
        #if BL_CHECK_SWITCH_MODE

            if (ucSwitchToSync)
            {
                pNfcPgCfg->bsDDRCfg = TRUE;
                pNfcPgCfg->bsDDRHfCfg = TRUE;
            }
            else
            {
                //Set to Async Mode
                ulPMU_bk = rPMU(0x20);
                rPMU(0x20) = 0x11;//Henry : read UECC & get feature fail without this PMU setting.
                pNfcPgCfg->bsDDRCfg = FALSE;
                pNfcPgCfg->bsDDRHfCfg = FALSE;
            }
            
            BL_GetFeatureAllFlash(INTERFACE_FEATURE_ADDR);

            //Test the read function. Read FW page 0.
            tFlashAddr.ucPU = 0;
            tFlashAddr.ucLun = 0;
            tFlashAddr.usBlock = 0;
            tFlashAddr.bsPln = 0;
            tFlashAddr.usPage = 0;
            
            tRdDes.bsTlcMode = 0;
            tRdDes.bsSecStart = 0;
            tRdDes.bsSecLen = (BL_GetFlashPhyPageSize()>>SEC_SIZE_BITS) - 2;
            tRdDes.bsRdBuffId = 0;
            tRdDes.ppNfcRed = NULL;
            tRdDes.bsDsIndex = DS_15K_CRC;
            tRdDes.pErrInj = NULL;

            if (NFC_STATUS_SUCCESS != HAL_NfcSinglePlnRead(&tFlashAddr, &tRdDes, FALSE))
            {
                DBG_Printf("[BL_SwitchMode] pu:%d trig read fail!!\n", tFlashAddr.ucPU);
            }

            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, 0);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
                HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
                DBG_Printf("[BL_SwitchMode] Pu %d LUN %d Read Fail, ErrType:%d\n",tFlashAddr.ucPU
                    , tFlashAddr.ucLun, HAL_NfcGetErrCode(tFlashAddr.ucPU, tFlashAddr.ucLun));
            }
            else
            {
                DBG_Printf("[BL_SwitchMode] pu:%d read success!!\n", tFlashAddr.ucPU);
            }

            if (!ucSwitchToSync)
                rPMU(0x20) = ulPMU_bk;
            
        #endif
    }
    
#endif
}


DRAM_BOOTLOADER_ATTR void BL_ClearDiskLock(U32 ulBufferForGB, U32 ulRedBuf)
{
    U32 ulTargetPu = 0xff;
    U32 ulPu, ulCurPageNum, ulPrePageNum, ulTotalPU;

    BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
    BL_InitFWUpdateInfo();

    ulTotalPU = BL_CalBitsNumByMap(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
    ulPrePageNum = 0;
    ulCurPageNum = 0;

#if 0
    DBG_Printf("Start looking for disk lock...\n");
    for (ulPu = 0; ulPu < ulTotalPU; ulPu++)
    {
        ulCurPageNum = BL_ReadGB2Dram(ulPu, ulBufferForGB, NULL);
        if (0 == ulPrePageNum)
        {
            ulPrePageNum = ulCurPageNum;
        }
        else if (ulPrePageNum > ulCurPageNum)
        {
            ulTargetPu = ulPu - 1;
            break;
        }
        else if (ulPrePageNum < ulCurPageNum)
        {
            ulTargetPu = ulPu;
            break;
        }
    }
#endif

    ulTargetPu = l_tFWUpdateInfo.ucCurrPU;
    if (0xff == ulTargetPu)
    {
        // Dont find Disk Lock
        DBG_Printf("Do not find disk lock.\n");
    }
    else
    {
        // clear disk lock
        DBG_Printf("Disk lock is on PU #%d.\n", ulTargetPu);
        ulCurPageNum = BL_ReadGB2Dram(ulTargetPu, ulBufferForGB, (BL_REDUNDANT *)ulRedBuf);
        BL_WriteGB2Flash(ulTargetPu, ulCurPageNum-1, ulBufferForGB, (BL_REDUNDANT *)ulRedBuf);
        DBG_Printf("Clear disk lock done.\n");
    }
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
    return ;
}

/*------------------------------------------------------------------------------
Name: BL_CopyGBFlashPages
Description:
    copy some page from one PU's global block to another.
Input Param:
    U8 ulDstPU:target PU
    U32 ulDstStartPG:target PU start page num
    U8 ulSrcPU:source PU
    U32 ulSrcStartPG:source PU start page num
    U32 ulPageTotal:total copy page num
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to copy some page from one PU's global block to another.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_CopyGBFlashPages(U8 ulDstPU, U32 ulDstStartPG, U8 ulSrcPU,
U32 ulSrcStartPG, U32 ulPageTotal)
{
    U16 usBuffID;
    U32 ulPGIndex;
    FLASH_ADDR tSrcFlashAddr;
    FLASH_ADDR tDstFlashAddr;
    NFC_RED *pNfcRed;
    U32 ulPhyPageSize;
    U32 ulPhyPageSizeBits;
    NFC_READ_REQ_DES tRdDes = {0};
    NFC_PRG_REQ_DES  tWrDes = {0};

    ulPhyPageSize = BL_GetFlashPhyPageSize();
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usBuffID = BL_GetBufferIDByMemAddr(BOOTLOADER_BUFF0_ADDR, TRUE, 10);

    COM_MemZero((U32 *)&tSrcFlashAddr, sizeof(FLASH_ADDR)>>2);
    COM_MemZero((U32 *)&tDstFlashAddr, sizeof(FLASH_ADDR)>>2);

    tSrcFlashAddr.ucPU = ulSrcPU;
    tDstFlashAddr.ucPU = ulDstPU;

    for (ulPGIndex = 0; ulPGIndex < ulPageTotal; ulPGIndex++)
    {
        tSrcFlashAddr.usPage = HAL_FlashGetSLCPage(ulSrcStartPG + ulPGIndex);
        tDstFlashAddr.usPage = HAL_FlashGetSLCPage(ulDstStartPG + ulPGIndex);

        /*read page from src PU*/
        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = (ulPhyPageSize>>SEC_SIZE_BITS) - 2;
        tRdDes.bsRdBuffId = usBuffID;
        tRdDes.ppNfcRed = &pNfcRed;
        tRdDes.bsDsIndex = DS_15K_CRC;
        tRdDes.pErrInj = NULL;

        //HAL_NfcSinglePlnRead_A(&tSrcFlashAddr, 0, ulPhyPageSize>>SEC_SIZE_BITS, FALSE, usBuffID, &pNfcRed);
        HAL_NfcSinglePlnRead(&tSrcFlashAddr, &tRdDes, FALSE);
        if (NFC_STATUS_FAIL == HAL_NfcWaitStatus(tSrcFlashAddr.ucPU, 0))
        {
            DBG_Printf("read GB fail!!\n");
            DBG_Getch();
        }

        /*write to dst PU*/
        tWrDes.bsWrBuffId = usBuffID;
        tWrDes.bsRedOntf = TRUE;
        tWrDes.pNfcRed = pNfcRed;
        tWrDes.bsDsIndex = DS_15K_CRC;
        tWrDes.pErrInj = NULL;

        //HAL_NfcSinglePlaneWrite_A(&tDstFlashAddr, usBuffID, pNfcRed);
        HAL_NfcSinglePlaneWrite(&tDstFlashAddr, &tWrDes);
        if (NFC_STATUS_FAIL == HAL_NfcWaitStatus(tDstFlashAddr.ucPU, 0))
        {
            DBG_Printf("write GB fail!!\n");
            DBG_Getch();
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: BL_ResetCurPU
Description:
    reset current PU: erase current PU and copy BL and FW V0 image back from next PU.
Input Param:
    U8 ucCurPU:current PU
    U8 ucNextPU:next PU
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to reset current PU.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_ResetCurPU(U8 ucCurPU, U8 ucNextPU)
{
    U8 ucCopyPageTotal;
    FLASH_ADDR tCurFlashAddr;

    ucCopyPageTotal = l_tFWUpdateInfo.ulOtherPUEmpPageStart;
    COM_MemZero((U32 *)&tCurFlashAddr, sizeof(FLASH_ADDR)>>2);

    tCurFlashAddr.ucPU = ucCurPU;
    tCurFlashAddr.bsPln = 0;
    tCurFlashAddr.usBlock = 0;

    /*erase current PU*/
    HAL_NfcSingleBlockErase(&tCurFlashAddr, FALSE);
    if (NFC_STATUS_FAIL == HAL_NfcWaitStatus(tCurFlashAddr.ucPU, 0))
    {
        DBG_Printf("erase PU:%d GB fail!!\n", tCurFlashAddr.ucPU);
        DBG_Getch();
    }

    /*copy BL and V0 from next PU to current PU*/
    BL_CopyGBFlashPages(ucCurPU, 0, ucNextPU, 0, ucCopyPageTotal);

    return;
}

LOCAL DRAM_BOOTLOADER_ATTR void BL_SetPtableNormalBoot(U32 ulAddr)
{
    BOOTLOADER_FILE *pImgBootLoaderFile;
    BOOTLOADER_FILE *pCurBootloaderFile;

    pCurBootloaderFile = (BOOTLOADER_FILE *)(OTFB_BOOTLOADER_BASE);
    pImgBootLoaderFile = (BOOTLOADER_FILE *)ulAddr;
    pImgBootLoaderFile->tSysParameterTable.sHwInitFlag.ulHWInitFlag = 0;
    pImgBootLoaderFile->tSysParameterTable.sBootStaticFlag.bsBootMethodSel = BOOT_METHOD_NORMAL;

    if (TRUE == pImgBootLoaderFile->tSysParameterTable.sBootStaticFlag.bsUartMpMode) {
        /*close UartMpMode*/
        pImgBootLoaderFile->tSysParameterTable.sBootStaticFlag.bsUartMpMode = FALSE;
    }

    pImgBootLoaderFile->tSysParameterTable.ulSubSysCEMap[0]
     = pCurBootloaderFile->tSysParameterTable.ulSubSysCEMap[0];
    pImgBootLoaderFile->tSysParameterTable.ulSubSysCEMap[1]
     = pCurBootloaderFile->tSysParameterTable.ulSubSysCEMap[1];
    COM_MemCpy((U32*)pImgBootLoaderFile->tSysParameterTable.aFWSerialNum, (U32*)pCurBootloaderFile->tSysParameterTable.aFWSerialNum, 5);
    COM_MemCpy((U32*)pImgBootLoaderFile->tSysParameterTable.aFWDiskName, (U32*)pCurBootloaderFile->tSysParameterTable.aFWDiskName, 10);
    COM_MemCpy((U32*)pImgBootLoaderFile->tSysParameterTable.ulWorldWideName, (U32*)pCurBootloaderFile->tSysParameterTable.ulWorldWideName, 2);
}

/*------------------------------------------------------------------------------
Name: BL_SaveFWProcess
Description:
    save FW image to flash action. flash addr is determined by FWUpdateInfo.
Input Param:
    U8 ucSlotID:FW update slow, now only slot 1 is permitted.
    U32 ulDramAddr:FW's start dram addr
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to save FW image to flash after FWUpdateInfo is initialized.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
LOCAL DRAM_BOOTLOADER_ATTR BOOL BL_SaveFWProcess(U8 ucSlotID,  U32 ulDramAddr)
{
    U8 ucPU;
    U8 ucPUIndex;
    U8 ucPUAlastType;
    U32 ulRemainPages;
    U32 ulPagePerBLK;
    U32 ulPUPageNum;
    U8 ucFWRealSavePage;
    U32 page_size = 0x3C00;//15K mode
    U32 bl_page_cnt;

    bl_page_cnt = (BL_TOTAL_SIZE_TMP%page_size ? (BL_TOTAL_SIZE_TMP/page_size + 1):BL_TOTAL_SIZE_TMP/page_size);

    ucFWRealSavePage = BL_GetSysImgPageCnt(ulDramAddr + 0x3C00);

    /*SlotID0's BootLoader area should be Init in FW*/
    if (0 == ucSlotID)
    {
        DBG_Printf("Save fw V0 start (ulDramAddr:0x%x)\n", ulDramAddr);
        /*erase GB*/
        BL_EraseAllPUGB(l_ucPUNum);

        /*save V0 to GB*/
        for (ucPUIndex = 0; ucPUIndex < l_ucPUNum; ucPUIndex++)
        {
            BL_SetPtableNormalBoot(ulDramAddr);
            BL_WriteFW2FlashGB(ucPUIndex, 0, ulDramAddr, ucFWRealSavePage);
        }
    }
    else
    {
        ulPagePerBLK = BL_GetFlashBlkPageNum();
        ulRemainPages = (ulPagePerBLK - HAL_FlashGetSLCPage(l_tFWUpdateInfo.ulCurPUEmpPageStart))/2;

        DBG_Printf("Save fw VX start page %d\n", l_tFWUpdateInfo.ulCurPUEmpPageStart);

        for (ucPUIndex = 0; ucPUIndex < l_ucPUNum; ucPUIndex++)
        {
            if (ulRemainPages  < ucFWRealSavePage)
            {
                BL_ResetCurPU(ucPUIndex, (ucPUIndex == l_ucPUNum-1) ? 0:(ucPUIndex+1));

                ulPUPageNum = l_tFWUpdateInfo.ulOtherPUEmpPageStart;
                ucPUAlastType = l_tFWUpdateInfo.ucCurPUAlast;

                /*save P_T to next PU if needed*/
                if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_PT_BIT)))
                {
                    /*write PTable FLAG into next PU*/
                    BL_WritePT2FlashGB(ucPUIndex, ulPUPageNum, BOOTLOADER_BUFF1_ADDR);
                    ulPUPageNum++;
                }

                /*save VX to next PU*/
                BL_WriteFW2FlashGB(ucPUIndex, ulPUPageNum, ulDramAddr, ucFWRealSavePage);
                ulPUPageNum += ucFWRealSavePage;

                /*save AX to next PU if needed*/
                if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_AX_BIT)))
                {
                    BL_WriteAX2FlashGB(ucPUIndex, ulPUPageNum, ucPUAlastType, BOOTLOADER_BUFF1_ADDR);
                    ulPUPageNum++;
                }

            }
            else
            {
                ulPUPageNum = l_tFWUpdateInfo.ulCurPUEmpPageStart;

                /*save VX to current PU*/
                BL_WriteFW2FlashGB(ucPUIndex, ulPUPageNum, ulDramAddr, ucFWRealSavePage);
            }
        }
    }
    return TRUE;
}

/*below interface iS for window FWUpdate verification only*/
#ifdef SIM
void BL_SaveBLAndFWV0(U32 ulDramAddr)
{
    U8 ucPUIndex;
    U8 ucPUTotal;
    U8 ucFWRealSavePage;

    /*Initial PU Mapping*/
    BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    /*write BL and V0*/
    ucFWRealSavePage = BL_GetFWRealSavePageNum();
    ucPUTotal = BL_CalBitsNumByMap(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
    for (ucPUIndex = 0; ucPUIndex < ucPUTotal; ucPUIndex++)
    {
        BL_WriteFW2FlashGB(ucPUIndex, 0, ulDramAddr, ucFWRealSavePage+4);
    }

    /*Init PU Mapping for FW*/
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID), HAL_GetSubSystemCEMap(MCU2_ID));

    return;
}
#endif

/*------------------------------------------------------------------------------
Name: BL_SaveFW
Description:
    save FW image to flash's call back interface for FW invoke.
Input Param:
    U8 ucSlotID:FW update slow, now only slot 1 is permitted.
    U32 ulDramAddr:FW's start dram addr
Output Param:
    none
Return Value:
    BOOL: TRUE:save success; FALSE:save fail;
Usage:
    FW invoke it to save FW image to flash.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR BOOL BL_SaveFW(U8 ucSlotID, U32 ulDramAddr)
{
    BOOL bStsFlag;

//When read/write FW on Intel/Micron 3D MLC/TLC, disable the nfc scramble. Because of the Rom code setting.
#if (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC))
    U32 ulRegBackup = rNfcPgCfg;
    rNfcPgCfg |= (1<<3);
#endif

    /*Initial PU Mapping*/
    BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    if (0 != ucSlotID)
    {
        /*Initial BL_FW_UPDATE_INFO*/
        BL_InitFWUpdateInfo();
    }

    /*do real process*/
    bStsFlag = BL_SaveFWProcess(ucSlotID, ulDramAddr);

    /*Init PU Mapping for FW*/
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

#if (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC))
        rNfcPgCfg = ulRegBackup;
#endif

#ifdef ASIC
    /* unlock and start HAL_UnitTest */
    *g_pNfcSaveFwDone = TRUE;
#endif

    return bStsFlag;
}

/*------------------------------------------------------------------------------
Name: BL_SaveActiveFWProcess
Description:
    save active FW entry to flash action.
Input Param:
    U8 ucSlotID:slot id0 or id1.
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it to save FW image to flash.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR BOOL BL_SaveActiveFWProcess(U8 ucSlotID)
{
    U8 ucAX;
    U8 ucCurPU;
    U8 ucNextPU;
    U32 ulCurPUEmptyPG;
    U32 ulNextPUEmptyPG;
    U32 ulCurPUVlastPGStart;
    U32 ulFWPGTotal;
    U32 ulPagePerBLK;
    U32 ulRemainPages;
    U32 ucPUIndex;
    BOOL bRet = FALSE;

    ucAX = (0 == ucSlotID) ? BL_PG_TYPE_ACTIVE0:BL_PG_TYPE_ACTIVE1;
    ulPagePerBLK = BL_GetFlashBlkPageNum();
    ulRemainPages = (ulPagePerBLK - HAL_FlashGetSLCPage(l_tFWUpdateInfo.ulCurPUEmpPageStart))/2;//edit for low page mode

    if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_VX_BIT)))
    {
        if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_AX_BIT)))
        {
            if (ucAX == l_tFWUpdateInfo.ucCurPUAlast)
            {
                return TRUE;
            }
        }

        /*save AX*/
        if (0 == ulRemainPages)
        {
            DBG_Printf("active fw fail\n");
            DBG_Getch();

            /*ucCurPU = l_tFWUpdateInfo.ucCurrPU;
            ucNextPU= l_tFWUpdateInfo.ucNextPU;
            ulCurPUVlastPGStart = l_tFWUpdateInfo.ulCurPUVlastStartPage;
            ulNextPUEmptyPG = l_tFWUpdateInfo.ulOtherPUEmpPageStart;
            ulFWPGTotal = l_tFWUpdateInfo.ucFwRealSavePGNum;*/

            /*save PT*/
            /*if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_PT_BIT)))
            {
                BL_WritePT2FlashGB(ucNextPU, ulNextPUEmptyPG, BOOTLOADER_BUFF1_ADDR);
                ulNextPUEmptyPG++;
            }*/

            /*copy Vlast to next PU*/
            /*BL_CopyGBFlashPages(ucNextPU, ulNextPUEmptyPG, ucCurPU, ulCurPUVlastPGStart, ulFWPGTotal);
            ulNextPUEmptyPG += ulFWPGTotal;

            DBG_Printf("active fw start page %d\n", ulNextPUEmptyPG);*/
            /*save AX*/
            //BL_WriteAX2FlashGB(ucNextPU, ulNextPUEmptyPG, ucAX, BOOTLOADER_BUFF1_ADDR);

            /*Reset current PU*/
            //BL_ResetCurPU(ucCurPU, ucNextPU);

        }
        else
        {
            /*save AX to current PU*/
            ulCurPUEmptyPG = l_tFWUpdateInfo.ulCurPUEmpPageStart;
            for (ucPUIndex = 0; ucPUIndex < l_ucPUNum; ucPUIndex++) {
                DBG_Printf("active fw start page %d\n", ulCurPUEmptyPG);
                BL_WriteAX2FlashGB(ucPUIndex, ulCurPUEmptyPG, ucAX, BOOTLOADER_BUFF1_ADDR);
            }
        }

        bRet = TRUE;
    }

    return bRet;
}

/*------------------------------------------------------------------------------
Name: BL_ActiveFW
Description:
    save active FW call-back interface for FW invoke.
Input Param:
    U8 ucSlotID:slot id0 or id1.
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it to active special slot's FW for next update.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR BOOL BL_ActiveFW(U8 ucSlotID)
{
    BOOL bStsFlag;
    /*Initial PU Mapping*/
    BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    /*Initial BL_FW_UPDATE_INFO*/
    BL_InitFWUpdateInfo();

    /*do AX save process*/
    bStsFlag = BL_SaveActiveFWProcess(ucSlotID);

    /*Init PU Mapping for FW*/
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    return bStsFlag;
}

/*------------------------------------------------------------------------------
Name: BL_SavePTableProcess
Description:
    save active debug mode PTable entry to flash action, the falsh location
    is determined by FWUpdateInfo.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to save active debug mode PTable entry to flash.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_SavePTableProcess(void)
{
    U8 ucAlast;
    U8 ucCurPU;
    U8 ucNextPU;
    U32 ulCurPUEmptyPG;
    U32 ulNextPUEmptyPG;
    U32 ulCurPUVlastStartPG;
    U32 ulPagePerBLK;
    U32 ulRemainPages;
    U32 ulFWPGTotal;

    ulPagePerBLK = BL_GetFlashBlkPageNum();
    ulRemainPages = (ulPagePerBLK - HAL_FlashGetSLCPage(l_tFWUpdateInfo.ulCurPUEmpPageStart))/2;//edit for low page mode
    if (0 == (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_PT_BIT)))
    {
        if (0 == ulRemainPages)
        {
            ucAlast = l_tFWUpdateInfo.ucCurPUAlast;
            ucCurPU = l_tFWUpdateInfo.ucCurrPU;
            ulCurPUVlastStartPG = l_tFWUpdateInfo.ulCurPUVlastStartPage;
            ucNextPU = l_tFWUpdateInfo.ucNextPU;
            ulNextPUEmptyPG = l_tFWUpdateInfo.ulOtherPUEmpPageStart;
            ulFWPGTotal = l_tFWUpdateInfo.ucFwRealSavePGNum;

            /*copy current PU Vlast to next PU*/
            if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_VX_BIT)))
            {
                BL_CopyGBFlashPages(ucNextPU, ulNextPUEmptyPG, ucCurPU, ulCurPUVlastStartPG, ulFWPGTotal);
                ulNextPUEmptyPG += ulFWPGTotal;
            }

            /*copy current PU Alast to next PU*/
            if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_AX_BIT)))
            {
                BL_WriteAX2FlashGB(ucNextPU, ulNextPUEmptyPG, ucAlast, BOOTLOADER_BUFF1_ADDR);
                ulNextPUEmptyPG++;
            }

            /*write P_T*/
            BL_WritePT2FlashGB(ucNextPU, ulNextPUEmptyPG, BOOTLOADER_BUFF1_ADDR);

            /*Reset current PU*/
            BL_ResetCurPU(ucCurPU, ucNextPU);
        }
        else
        {
            ucCurPU = l_tFWUpdateInfo.ucCurrPU;
            ulCurPUEmptyPG = l_tFWUpdateInfo.ulCurPUEmpPageStart;
            BL_WritePT2FlashGB(ucCurPU, ulCurPUEmptyPG, BOOTLOADER_BUFF1_ADDR);
        }
    }

    return;
}

/*------------------------------------------------------------------------------
Name: BL_SavePTable
Description:
    save PTable(active FW debug mode) entry to flash call-back interface for FW invoke.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it save PTable entry to flash.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_SavePTable(void)
{
    /*Initial PU Mapping*/
    BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    /*Initial BL_FW_UPDATE_INFO*/
    BL_InitFWUpdateInfo();

    /*do PTable save process*/
    BL_SavePTableProcess();

    /*Init PU Mapping for FW*/
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    return;
}

/*------------------------------------------------------------------------------
Name: BL_GetHex
Description:
Input:
    U8 Id: this is the identical key used to fetch Value.
Return Value:
    U32 Value: the fetched value will be saved to pValue. *pValue = value;
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR U32 BL_GetHex(U8 ucId)
{
    U32 *pValue = (U32 *)((OTFB_BOOTLOADER_BASE + BL_CODE_SIZE) + g_HexDataIndexTable[ucId]);

    if (!BL_CheckAlignU32((U32)pValue))
    {
        DBG_Getch();
    }
    return *pValue;
}

/*------------------------------------------------------------------------------
Name: BL_GetString
Description:
Input:
    U8 Id: this is the identical key used to fetch Value.
Return Value:
    char *pStrPos : *pStrPos = start address of the string in Memory, note that you should not modify it

------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR const char* BL_GetString(U8 ucId)
{
    const char *pStr = (const char *)((OTFB_BOOTLOADER_BASE + BL_CODE_SIZE) + g_StrDataIndexTable[ucId]);
    return pStr;
}

/*------------------------------------------------------------------------------
Name: BL_LoadFW2Dram
Description:
    load special FW image out. FW V0 image not do redundant check. other FW image
    do redundant check.
Input Param:
    U8 ucPU: PU num
    U32 ulFWStartPage: global block start page
    BOOL bV0Flag: TRUE:FW V0; FALSE:FW VX
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it load FW image out.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR BOOL BL_LoadFW2Dram(U8 ucPU, U32 ulFWStartPage, U32 *pFwBitMap)
{
    U8 ucFWPageTotal;
    U32 ucPageCnt;
    U32 ucReadPassPageCnt = 0;
    U16 usBuffID;
    U16 usStartBuffID;
    U16 ulPhyPageSize;
    U32 ulStatus;
    U32 ulPhyPageSizeBits;
    U32 ulFwIndex;
    FLASH_ADDR tFlashAddr;
    BL_REDUNDANT *pBLRed;
    U8  ucErrCode = NF_SUCCESS;
    NFC_READ_REQ_DES tRdDes = {0};

    ulPhyPageSize = BL_GetFlashPhyPageSize();
    ucFWPageTotal = BL_GetSysImgPageCnt(HEADER_LOCATION) - 1;
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usStartBuffID = BL_GetBufferIDByMemAddr(SYSTEM_IMG_ADDR_SAVE_FW, TRUE, 10);

    COM_MemZero((U32 *)&tFlashAddr, sizeof(FLASH_ADDR)>>2);
    tFlashAddr.ucPU = ucPU;
    tFlashAddr.ucLun = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.bsPln = 0;

    for (ucPageCnt = 0; ucPageCnt < ucFWPageTotal; ucPageCnt++)
    {
        if (BL_FwReadBitmapIsPass((U32 *)pFwBitMap, ulFWStartPage + ucPageCnt))
        {
            ucReadPassPageCnt++;
            continue;
        }

        tFlashAddr.usPage = HAL_FlashGetSLCPage(ulFWStartPage + ucPageCnt);
        usBuffID = usStartBuffID + ucPageCnt * 15;
        //DBG_Printf("FW load start pu:%d, page:%d usBuffID:%d ucPageCnt:%d\n", ucPU, tFlashAddr.usPage, usBuffID,ucPageCnt);

        tRdDes.bsTlcMode = 0;
        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = (ulPhyPageSize>>SEC_SIZE_BITS) - 2;
        tRdDes.bsRdBuffId = usBuffID;
        tRdDes.ppNfcRed = (NFC_RED **)&pBLRed;
        tRdDes.bsDsIndex = DS_15K_CRC;
        tRdDes.pErrInj = NULL;

        if (NFC_STATUS_SUCCESS != HAL_NfcSinglePlnRead(&tFlashAddr, &tRdDes, FALSE))
        {
            DBG_Printf("pu:%d trig read fail!!\n", tFlashAddr.ucPU);
            return FALSE;
        }

        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, 0);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            ucErrCode = HAL_NfcGetErrCode(tFlashAddr.ucPU, 0);
            if ((NF_ERR_TYPE_UECC    == ucErrCode)
             || (NF_ERR_TYPE_DCRC    == ucErrCode))
            {
                ucErrCode = BL_FlashReadRetry(&tFlashAddr, &tRdDes);
            }

            if (NF_SUCCESS != ucErrCode)
            {
                DBG_Printf("read PU%d pg%d error type %d\n", tFlashAddr.ucPU, tFlashAddr.usPage, ucErrCode);
                HAL_NfcResetCmdQue(tFlashAddr.ucPU, 0);
                HAL_NfcClearINTSts(tFlashAddr.ucPU, 0);
            }
            else
            {
                //mark read success page
                pFwBitMap[(ulFWStartPage + ucPageCnt)/32] |= (1<<((ulFWStartPage + ucPageCnt)%32));
                ucReadPassPageCnt++;
            }
        }
        else
        {
            //mark read success page
            pFwBitMap[(ulFWStartPage + ucPageCnt)/32] |= (1<<((ulFWStartPage + ucPageCnt)%32));
            ucReadPassPageCnt++;
        }
    }

    if (ucReadPassPageCnt == ucFWPageTotal)
    {
        DBG_Printf("PU%d read all fw pages pass.\n", tFlashAddr.ucPU);
        return TRUE;
    }
    else
    {
        DBG_Printf("PU%d read %d pages pass. (%d pages fail)\n",
            tFlashAddr.ucPU, ucReadPassPageCnt, ucFWPageTotal-ucReadPassPageCnt);
        return FALSE;
    }

//Henry tmp mark this FW check
#if 0
            /*check FW wholeness*/
            if ((ucPageCnt != pBLRed->m_FWPageNum)
#ifdef SIM
            || (BL_PG_TYPE_FIRMWARE != pBLRed->m_RedComm.bcPageType))
#else
            || (BL_PG_TYPE_FIRMWARE != pBLRed->m_GBPageType))
#endif
            {
                DBG_Printf("FW load error!!\n");

                return FALSE;
            }
#endif

    return TRUE;
}

/*------------------------------------------------------------------------------
Name: BL_LoadFWProcess
Description:
    load FW image out action. FW image version is determined by FWUpdate Info.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it load FW image out.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_LoadFWProcess(void)
{
    BOOL bSts;
    U8 ucPUIndex = 0, ucFailCnt = 0, ucStartPU = 0;
    U32 ulReadFwBitMap[FW_IMG_PAGE_NUM_MAX/32] = {0};

    if (0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_PT_BIT)))
    {
        HAL_SetDebugMode();
    }

    if ((0 != (l_tFWUpdateInfo.ucCurPUExistFlag & (1<<BL_GB_AX_BIT)))
    && (BL_PG_TYPE_ACTIVE1 == l_tFWUpdateInfo.ucCurPUAlast))
    {
        //randomly choose PU
        ucPUIndex = ((HAL_GetMCUCycleCount()&0xff0)>>4)%l_ucPUNum;
        DBG_Printf("Randomly choose PU %d to load FW\n", ucPUIndex);

        while (1) {
            /*Load Vlast*/
            bSts = BL_LoadFW2Dram(ucPUIndex, l_tFWUpdateInfo.ulCurPUVlastStartPage, (U32 *)&ulReadFwBitMap);

            if (FALSE == bSts) {
                ucPUIndex = (ucPUIndex == (l_ucPUNum-1)) ? 0:(ucPUIndex+1);

                ucFailCnt++;

                /*Load V0*/
                if (ucFailCnt == l_ucPUNum) {
                    for (ucPUIndex = 0; ucPUIndex < l_ucPUNum; ucPUIndex++) {
                        DBG_Printf("LoadFW at PU%d page%d\n", ucPUIndex, l_tFWUpdateInfo.ulCurPUVlastStartPage);
                        bSts = BL_LoadFW2Dram(ucPUIndex, GB_FWV0_STARTPAGE, (U32 *)&ulReadFwBitMap);
                        if (TRUE == bSts) {
                            DBG_Printf("BL load FW V0 done at PU%d\n", ucPUIndex);
                            break;
                        }
                    }
                    if ((ucPUIndex == l_ucPUNum) && (FALSE == bSts)) {
                        DBG_Printf("BL load FW V0 all PU fail\n");
                        DBG_Getch();
                    }
                    break;
                }
            } else {
                DBG_Printf("BL load FW Vlast done at PU%d\n", ucPUIndex);
                break;
            }
        }
    }
    else
    {
        /*Load V0*/

        for (ucPUIndex = 0; ucPUIndex < l_ucPUNum; ucPUIndex++)
        {
            //(Rom read pg 0, TL read pg1 , BL read pg1 again to last pg)
            bSts = BL_LoadFW2Dram(ucPUIndex, 1, (U32 *)&ulReadFwBitMap);//fix read start from page 1 to dram

            if (TRUE == bSts)
            {
                DBG_Printf("BL load FW V0 done at PU%d\n", ucPUIndex);
                break;
            }
        }

        if ((ucPUIndex == l_ucPUNum) && (FALSE == bSts))
        {
            DBG_Printf("BL load FW V0 all PU fail\n");
            DBG_Getch();
        }
    }

    return;
}


#ifdef SIM
NOINL_ATTR void BL_LoadFW(void)
{
    /*Initial PU Mapping*/
    BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

    /*Initial BL_FW_UPDATE_INFO*/
    BL_InitFWUpdateInfo();

    /*do PTable save process*/
    BL_LoadFWProcess();

    /*Init PU Mapping for FW*/
    HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID), HAL_GetSubSystemCEMap(MCU2_ID));

    return;
}
#endif

/*------------------------------------------------------------------------------
Name: BL_RunFW
Description:
    Run FW interface for both BootLoader and FW call back invoke.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to Run FW.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
NOINL_ATTR DRAM_BOOTLOADER_ATTR void BL_RunFW(void)
{
    FunPointer pFWEntrance;
    U32 ulHPLL, ulDPLL, ulNPLL, ulNdivide;

//When read/write FW on Intel/Micron 3D MLC/TLC, disable the nfc scramble. Because of the Rom code setting.
#if (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC))
    U32 ulRegBackup = rNfcPgCfg;
    rNfcPgCfg |= (1<<3);
#endif

    if (TRUE == l_bNfcInitDoneFlag)
    {
        if ((BOOT_METHOD_NORMAL == BL_GetBootMethodSel()))
        {
            /*Initial PU Mapping*/
            BL_NfcPuMapInit(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));

            /*Initial BL_FW_UPDATE_INFO*/
            BL_InitFWUpdateInfo();

            /*do PTable save process*/
            BL_LoadFWProcess();

            //To do : modify input address, if "read all page to OTFB" is done
            BL_LoadBLnFW(SYSTEM_IMG_ADDR_SAVE_FW+HEADER__SIZE);
        }

        /*Init PU Mapping for FW*/
        HAL_NfcSetLogicPUReg(HAL_GetSubSystemCEMap(MCU1_ID) | HAL_GetSubSystemCEMap(MCU2_ID));
    }

    if (TRUE == BL_IsOptionRomEnable())
    {
        BL_EnablePCIeOptionRom();
        DBG_Printf("OpROM enabled.\n");
    }
    else
    {
        BL_DisablePCIeOptionRom();
        DBG_Printf("OpROM disabled.\n");
    }

#if (defined(FLASH_3D_MLC) || defined(FLASH_INTEL_3DTLC))
    rNfcPgCfg = ulRegBackup;
#endif

    //DSRAM remap to OTFB
    rGLB(0x24) = 0;

    ulHPLL = PLL(rPMU(0x68) >> 16);
    ulDPLL = PLL(rPMU(0x68));
    ulNPLL = PLL(rPMU(0x6C))*(1<<(rPMU(0x20)&0x03))/16;
    ulNdivide = rPMU(0x20)&0x03;
    ulNdivide = 16/(1<<ulNdivide);
    DBG_Printf("HPLL = %d MHz , DPLL = %d MHz , NPLL = %d MHz\n",ulHPLL,ulDPLL,ulNPLL);
    //DDR mode : DCLK and NCLK need to *2
    DBG_Printf("HCLK= %d (MHz), DDR%d (MT/s), NFDDR%d (MT/s)\n",ulHPLL,ulDPLL*2,((ulNPLL*2)/ulNdivide)*2);
    /*jump to FW*/
    DBG_Printf("set pc done 0x%x\n", glHeader->mcu0fw_exec);
    pFWEntrance = (FunPointer)(glHeader->mcu0fw_exec);
    pFWEntrance();

    return;
}

#if 0
DRAM_BOOTLOADER_ATTR BOOL BL_LoadBootLoaderPart1(void)
{
    U8 ulPUIndex;
    U16 usBuffID;
    FLASH_ADDR tFlashAddr;
    NFC_RED *pNfcRed;
    U32 ulPhyPageSize;
    U32 ulPhyPageSizeBits;
    BOOL bRet = FALSE;
    NFC_READ_REQ_DES tRdDes = {0};

    ulPhyPageSize = BL_GetFlashPhyPageSize();
    ulPhyPageSizeBits = BL_GetFlashPhyPageSecBit();
    usBuffID = BL_GetBufferIDByMemAddr(DRAM_BOOTLOADER_PART1_BASE, TRUE, 10);

    COM_MemZero((U32 *)&tFlashAddr, sizeof(FLASH_ADDR)>>2);

    tFlashAddr.usPage = 1;
    ulPUIndex = 0;
    while (ulPUIndex < l_ucPUNum)
    {
        tFlashAddr.ucPU = ulPUIndex;

        /*read page from src PU*/
        tRdDes.bsSecStart = 0;
        tRdDes.bsSecLen = (ulPhyPageSize>>SEC_SIZE_BITS) - 2;
        tRdDes.bsRdBuffId = usBuffID;
        tRdDes.ppNfcRed = &pNfcRed;
        tRdDes.bsDsIndex = DS_15K_CRC;
        tRdDes.pErrInj = NULL;

        HAL_NfcSinglePlnRead(&tFlashAddr, &tRdDes, FALSE);
        if (NFC_STATUS_FAIL == HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("read BootLoaderPart1 PU:%x fail\n", tFlashAddr.ucPU);
            HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
            HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
        }
        else
        {
            bRet = TRUE;
            break;
        }

        ulPUIndex++;
    }
    return bRet;
}
#endif

/*------------------------------------------------------------------------------
Name: BL_RegEntryProcess
Description:
    process reglist one entry.
Input Param:
    REG_ENTRY *pRegEntry:reglist entry pointer.
Output Param:
    none
Return Value:
    Bool: TRUE:current entry is the last. False:current entry isn't the last
Usage:
    BootLoader invoke it to process reglist one entry.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR BOOL BL_RegEntryProcess(REG_ENTRY *pRegEntry)
{
    U32 ulOpcode;
    U32 ulAddr;
    U32 ulAndMsk;
    U32 ulOrMsk;
    U32 ulSys0CEMap;
    U32 ulSys1CEMap;
    BOOL bFinishEntry = FALSE;

    ulAddr = pRegEntry->ulAddr;
    ulOpcode = pRegEntry->ulOpcode;
    ulAndMsk = pRegEntry->ulAndMsk;
    ulOrMsk = pRegEntry->ulOrMsk;

    DBG_Printf("ulOpcode %x, ulAddr %x, ulAndMsk %x, ulOrMsk %x\n", ulOpcode, ulAddr, ulAndMsk, ulOrMsk);
    switch (ulOpcode & 0xff)
    {
        case OPT_REG_SET:
        {
            *(volatile U32 *)ulAddr = (*(volatile U32 *)ulAddr & ulAndMsk) | ulOrMsk;
            break;
        }
        case OPT_DELAY:
        {
            HAL_DelayUs(ulAddr);
            break;
        }
        case OPT_NFC_INTERFACE_INIT:
        {
            HAL_NfcInterfaceInitBasic();
            HAL_FlashNfcFeatureInit();
            HAL_DecStsInit();
            HAL_NfcDataSyntaxInit();
            //HAL_NfcDataSyntaxInit(DS_15K_CRC, LDPC_MAT_SEL_LASE_1K);//15K CWnum + CRC check
            HAL_LdpcInit();
            HAL_LdpcDownloadHMatix();
            HAL_NfcQEEInit();
            HAL_FlashInitSLCMapping();//add for low page mode when update BL FW
            BL_NfcPuMapInit(INVALID_8F);
            HAL_NormalDsgInit();
            break;
        }
        case OPT_NFC_RESET:
        {
            BL_ResetAllFlash();
            break;
        }
            //Henry tmp mark
            /*case OPT_NFC_SYNC_RESET:
            {
            BL_ResetAllFlash();
            break;
            }*/
        case OPT_NFC_UPDATE_PUMAPPING:
        {
            BL_NFC_UpdatePuMapping();
            /*BL_UpdatePuMap();
            ulSys0CEMap = HAL_GetSubSystemCEMap(MCU1_ID);
            ulSys1CEMap = HAL_GetSubSystemCEMap(MCU2_ID);
            BL_NfcPuMapInit(ulSys0CEMap | ulSys1CEMap);*/

            //test
            //BL_EraseAllPUGB(l_ucPUNum);

            break;
        }
        case OPT_NFC_SETFEATURE:
        {
            BL_SetFeatureAllFlash((ulAddr & 0xff), ulOrMsk); //nfc_setfeature((ulAddr&0xff), ulOrMsk);
            /* if script asserted OPT_NFC_SETFEATURE, then NFC needed, then NfcInit should be done before */
            l_bNfcInitDoneFlag = TRUE;
            break;
        }
        case OPT_FINISH:
        {
            bFinishEntry = TRUE;
            DBG_Printf("bFinishEntry is true");
            break;
        }
        case OPT_NONE:
        {
            break;
        }
        default:
        {
            DBG_Printf("RegList OPT type error!!!\n");
            DBG_Getch();
            //__asm__("break 1,15");
        }
    }

    return bFinishEntry;
}

/*------------------------------------------------------------------------------
Name: BL_RegSegmentProcess
Description:
    process reglist one segment entries.
Input Param:
    U32 ulRegAddrStart:offset start of reglist
    U32 ulRegCnt:reg entry count
Output Param:
    none
Return Value:
    none
Usage:
    BootLoader invoke it to process reglist one segment entries.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
void BL_RegSegmentProcess(U32 ulRegAddrStart, U32 ulRegCnt)
{
    U16 usIndx;
    REG_ENTRY *p_RegEntry;
    BOOTLOADER_FILE *pBootLoaderFile;
    U8 *pOpCodeStart;
    pBootLoaderFile = BL_GetBootLoaderFile();

    // Decode VIA BLScript Format Ops.
    pOpCodeStart = (U8 *)((U32)(&pBootLoaderFile->tSysRegList) + ulRegAddrStart);
    BL_DecodeOpsInRAM(pOpCodeStart, ulRegCnt, OPCODE_ALL);

    return;
}

/*------------------------------------------------------------------------------
Name: BL_HWInitCallBackCommon
Description:
    common part of HW init call back funtion's.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW invoke it from FTable entry to init global registers.
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
void BL_HWInitCallBackCommon(U8 ucHWInitType)
{
    U32 ulStartPos;
    U32 ulRegCnt;
    BOOTLOADER_FILE *pBootLoaderFile;
    HW_INIT_FLAG *pHWInitFlag;

    pBootLoaderFile = BL_GetBootLoaderFile();
    pHWInitFlag = &pBootLoaderFile->tSysParameterTable.sHwInitFlag;

    ulStartPos = pBootLoaderFile->tSysFunctionTable.aInitFuncEntry[ucHWInitType].ulRegPos;
    ulRegCnt = pBootLoaderFile->tSysFunctionTable.aInitFuncEntry[ucHWInitType].ulRegCnt;

    BL_RegSegmentProcess(ulStartPos, ulRegCnt);
    pHWInitFlag->ulHWInitFlag |= (1<<ucHWInitType);

    return;
}
/*------------------------------------------------------------------------------
Name: BL_HWInitAll
Description:
    Init all HW module by flags.
Input Param:
    void
Output Param:
    none
Return Value:
    none
Usage:
    BL invoke it to init all HW module
History:
    201409025    Tobey   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_HWInitAll(void)
{
    U8 ucIndex;
    U32 ulFlag;
    U32 ulUartMP;
    FunPointer  pFunc;
    volatile BOOTLOADER_FILE *pBootLoaderFile;
    pBootLoaderFile = BL_GetBootLoaderFile();

    //DBG_Printf("BL_HWInitAll\n");

    for (ucIndex = 0; ucIndex < INIT_FUNC_CNT; ucIndex++)
    {
        //DBG_Printf("ucIndex = %d\n", ucIndex);
		#if 0
        /*get UARTMP mode, if high, uart mp mode*/
        if (HAL_UartIsMp() == TRUE) 
        {       
            if (ucIndex == INIT_PCIE)
                continue;
        }
		#endif
        /*get GPIO8 value, if low, uart mp mode*/
        ulUartMP = rGPIO(0x18);
        if (!(ulUartMP & 0x00000100)) {
            if (ucIndex == INIT_PCIE)
                continue;
        }


        ulFlag = pBootLoaderFile->tSysParameterTable.sHwInitFlag.ulHWInitFlag; //shouldn't be move out this circle.
        if (0 == (ulFlag & (1 << ucIndex)))
        {
            pFunc = (FunPointer)tFTableFuncEntry[ucIndex].ulEntryAddr;
            //DBG_Printf("HW %d func addr %x\n", ucIndex, tFTableFuncEntry[ucIndex].ulEntryAddr);
            pFunc();
        }
        else {
            DBG_Printf("HW %d func addr %x skipped\n", ucIndex, tFTableFuncEntry[ucIndex].ulEntryAddr);
        }
    }

    return;
}

#ifndef SIM

LOCAL DRAM_BOOTLOADER_ATTR U32 BL_GetRestOpListStart(U32 *pRestOPCodeLength)
{
    /*      4K usage
              =======
                FTable
              ---------
                Ptable
              ---------
                OpList
                    +-- func1 oplist
                    +-- func2 oplist
                    ...
                    +--last func oplist
                    +-->rest of the oplist
                    used to store some other Op, including version info, sethex, setStr, etc.
              =======
    */
    volatile BOOTLOADER_FILE *pBootLoaderFile = BL_GetBootLoaderFile();
    U32 ulLastFuncStart = pBootLoaderFile->tSysFunctionTable.aInitFuncEntry[INIT_FUNC_CNT - 3].ulRegPos;
    U32 ulRegAddrStart = ulLastFuncStart +
    pBootLoaderFile->tSysFunctionTable.aInitFuncEntry[INIT_FUNC_CNT - 3].ulRegCnt;
    U8 *pRestOpListStart = (U8 *)((U32)(&pBootLoaderFile->tSysRegList) + ulRegAddrStart);
    U32 ulOPCodeLength = sizeof(REG_LIST) - ulRegAddrStart;
    *pRestOPCodeLength = ulOPCodeLength;

    //DBG_Printf("%s Start:%x Len:%d\n", __FUNCTION__, pRestOpListStart, OPCodeLength);
    return (U32)pRestOpListStart;
}

DRAM_BOOTLOADER_ATTR void BL_GetScriptVersion(void)
{
    U8 *pRestOpListStart;
    U32 ulOPCodeLength;
    pRestOpListStart = (U8 *)BL_GetRestOpListStart(&ulOPCodeLength);
    //search Version Op from pRestOpListStart to END of OpList.
    //DBG_Printf("%s Start:%x Len:%d\n", __FUNCTION__, pRestOpListStart, ulOPCodeLength);
    BL_DecodeOpsInRAM(pRestOpListStart, ulOPCodeLength, OPCODE_VER);
    //DBG_Printf("~Done\n");
}

DRAM_BOOTLOADER_ATTR void BL_ExecAllRestOpCode(void)
{
    U8 *pRestOpListStart;
    U32 ulOPCodeLength;
    pRestOpListStart = (U8 *)BL_GetRestOpListStart(&ulOPCodeLength);
    BL_DecodeOpsInRAM(pRestOpListStart, ulOPCodeLength, OPCODE_ALL);
}

DRAM_BOOTLOADER_ATTR void BL_TestGetHex(void)
{
    U32 ulId = 0;
    U32 ulValue = 0;
    //DBG_Printf("BL_TestGetHex s\n");
    for (ulId = 0; ulId < 256; ulId++)
    {
        //get offset from index table.
        ulValue = BL_GetHex((U8)ulId);
        //print it out.
        if (ulValue != 0)
        {
            DBG_Printf("HexId:%d:0x%x\n", ulId, ulValue);
        }
    }
    //DBG_Printf("Rests=Zero\n");
}

DRAM_BOOTLOADER_ATTR void BL_TestGetString(void)
{
    U32 ulId;
    const char *pValue = NULL;
    //DBG_Printf("BL_TestGetString s\n");

    for (ulId = 0; ulId < 256; ulId++)
    {
        //get offset from index table.
        pValue = BL_GetString((U8)ulId);

        //add with 4K Base Addr
        //print it out.
        if (pValue && *pValue != '\0')
        {
            DBG_Printf("StrId:%d:%s\n", ulId, pValue);
        }

    }
    //DBG_Printf("Rests=Empty\n");
}

/*------------------------------------------------------------------------------
Name: cdcDelay
Description:
    cdcDelay move from ROM code
Input Param:
    none
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL  void cdcDelay(void)
{
    U32 i;
    i = 200;
    while (i--)
        __asm__("nop");
}

/*------------------------------------------------------------------------------
Name: AlignTo64B
Description:
    Align input to 64B
Input Param:
    U32 addr : Input value whcih need to be 64B aligned.
Output Param:
    none
Return Value:
    U32: 64Bytes Aligned value
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
U32 AlignTo64B(U32 addr)
{
    if (addr&0x3F)
        addr = addr + (0x40 - (addr&0x3F));
    return addr;
}

/*------------------------------------------------------------------------------
Name: BL_GetSysImgPageCnt
Description:
    Get system.bin page count by Header info
Input Param:
    U32 ulHeaderAddr : Header location
Output Param:
    none
Return Value:
    U32 ulPgCnt : total page count for system.bin
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
U32 BL_GetSysImgPageCnt(U32 ulHeaderAddr)
{
    U32 ulSize, ulPgCnt;

    glHeader = (IMAGE_HDR *)(ulHeaderAddr);

    ulSize = AlignTo64B(glHeader->tinyloader_size) + HEADER__SIZE + AlignTo64B(glHeader->bootloader_size)
                                    + AlignTo64B(glHeader->mcu0fw_size) + AlignTo64B(glHeader->mcu1fw_size)
                                    + AlignTo64B(glHeader->mcu2fw_size);

    ulPgCnt = (ulSize%0x3C00 ? (ulSize/0x3C00+1):(ulSize/0x3C00));

    if (ulPgCnt > FW_IMG_PAGE_NUM_MAX)
    {
        DBG_Printf("System image page count(%d) is larger than %d!\n", ulPgCnt, FW_IMG_PAGE_NUM_MAX);
        DBG_Getch();
    }

    return ulPgCnt;
}




/*------------------------------------------------------------------------------
Name: BL_LoadBLnFW
Description:
    Copy BL part 1/FW0/FW1/FW2 to correct address
Input Param:
    U32 ulStartAddr : Start address of BL part 1
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
void BL_LoadBLnFW(U32 ulStartAddr)
{
    U32 ulOtfbFw0Addr, ulOtfbFw1Addr, ulOtfbFw2Addr;

    //Load BL part 1 to DRAM
    COM_MemCpy((U32 *)glHeader->bootloader_addr, (U32 *)ulStartAddr,
    glHeader->bootloader_size%4 ? (glHeader->bootloader_size/4+1):(glHeader->bootloader_size/4));

    //Get FW0 address at OTFB and 64 aligned
    ulOtfbFw0Addr = AlignTo64B(ulStartAddr + glHeader->bootloader_size);

    //Load FW0 to DRAM
    COM_MemCpy((U32 *)glHeader->mcu0fw_addr, (U32 *)ulOtfbFw0Addr,
    glHeader->mcu0fw_size%4 ? (glHeader->mcu0fw_size/4+1):(glHeader->mcu0fw_size/4));

    //Get FW1 address at OTFB and 64 aligned
    ulOtfbFw1Addr = AlignTo64B(ulOtfbFw0Addr + glHeader->mcu0fw_size);

    //Load FW1 to DRAM
    COM_MemCpy((U32 *)glHeader->mcu1fw_addr, (U32 *)ulOtfbFw1Addr,
    glHeader->mcu1fw_size%4 ? (glHeader->mcu1fw_size/4+1):(glHeader->mcu1fw_size/4));

    //Get FW2 address at OTFB and 64 aligned
    ulOtfbFw2Addr = AlignTo64B(ulOtfbFw1Addr + glHeader->mcu1fw_size);

    //Load FW2 to DRAM
    COM_MemCpy((U32 *)glHeader->mcu2fw_addr, (U32 *)ulOtfbFw2Addr,
    glHeader->mcu2fw_size%4 ? (glHeader->mcu2fw_size/4+1):(glHeader->mcu2fw_size/4));
}


void BL_LoadBLnFWforLLF(U32 ulStartAddr)
{
    U32 ulTmpAddr, ulTmpSize;

    //Load BL part 1 to DRAM
    if (glHeader->bootloader_size%4)
        ulTmpSize = glHeader->bootloader_size+(4-(glHeader->bootloader_size%4));
    else
        ulTmpSize = glHeader->bootloader_size;
    COM_MemCpy((U32 *)glHeader->bootloader_addr, (U32 *)ulStartAddr, ulTmpSize/4);

    // Copy the rest img from OTFB to FW MCU0 start address on DRAM
    ulTmpAddr = AlignTo64B(ulStartAddr + ulTmpSize);
    ulTmpSize = (BL_LLF_IMG_OTFB_SIZE - HEADER__SIZE - ulTmpSize);//the rest size of OTFB(MCU0,1,2)
    COM_MemCpy((U32 *)glHeader->mcu0fw_addr, (U32 *)ulTmpAddr, ulTmpSize/4);

    // Copy the rest img from SRAM1 to DRAM after OTFB
    ulTmpAddr = glHeader->mcu0fw_addr+ulTmpSize;
    COM_MemCpy((U32 *)ulTmpAddr, (U32 *)BL_LLF_IMG_SRAM1_ADDR, BL_LLF_IMG_SRAM1_SIZE/4);

    // Copy the rest img from SRAM2 to DRAM after OTFB + SRAM1
    ulTmpAddr = glHeader->mcu0fw_addr + ulTmpSize + BL_LLF_IMG_SRAM1_SIZE;
    COM_MemCpy((U32 *)ulTmpAddr, (U32 *)BL_LLF_IMG_SRAM2_ADDR, BL_LLF_IMG_SRAM2_SIZE/4);

    // Copy the rest img from SRAM3 to DRAM after OTFB + SRAM1 + SRAM2
    ulTmpAddr = glHeader->mcu0fw_addr + ulTmpSize + BL_LLF_IMG_SRAM1_SIZE + BL_LLF_IMG_SRAM2_SIZE;
    COM_MemCpy((U32 *)ulTmpAddr, (U32 *)BL_LLF_IMG_SRAM3_ADDR, BL_LLF_IMG_SRAM3_SIZE/4);

    //MCU0 is now at corrent address but MCU1 & MCU2 are not(Image is 64B align but not on DRAM).

    //Move MCU1 to corrent address according to header info
    ulTmpAddr = AlignTo64B(glHeader->mcu0fw_addr + glHeader->mcu0fw_size);
    COM_MemCpy((U32 *)glHeader->mcu1fw_addr, (U32 *)ulTmpAddr,
        glHeader->mcu1fw_size%4 ? (glHeader->mcu1fw_size/4+1):(glHeader->mcu1fw_size/4));

    //Move MCU2 to corrent address according to header info
    ulTmpAddr = AlignTo64B(ulTmpAddr + glHeader->mcu1fw_size);
    COM_MemCpy((U32 *)glHeader->mcu2fw_addr, (U32 *)ulTmpAddr,
            glHeader->mcu2fw_size%4 ? (glHeader->mcu2fw_size/4+1):(glHeader->mcu2fw_size/4));
}

/*------------------------------------------------------------------------------
Name: BL_CpySystemBin
Description:
    Copy BL System.bin from OTFB to DRAM for MCU0 Save FW.
Input Param:
    U32 ulDstAddr : Destination address
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
DRAM_BOOTLOADER_ATTR void BL_CpySystemBin(U32 ulDstAddr)
{
    U32 ulTmpLocation;

    //copy BL Part0 (15KB total) from OTFB to Save FW Dram Buffer
    COM_MemCpy((U32 *)ulDstAddr, (U32 *)OTFB_START_ADDRESS, 0x3C00/4);

    //copy BL Part1 & MCU0 & MCU1 & MCU2 from OTFB to Save FW Dram Buffer
    ulTmpLocation = ulDstAddr + 0x3C00;
    COM_MemCpy((U32 *)ulTmpLocation, (U32 *)HEADER_LOCATION, BL_LLF_IMG_OTFB_SIZE/4);

    //copy the rest part of MCU0 & MCU1 & MCU2 from SRAM1 to Save FW Dram Buffer
    ulTmpLocation = ulTmpLocation + BL_LLF_IMG_OTFB_SIZE;
    COM_MemCpy((U32 *)ulTmpLocation, (U32 *)BL_LLF_IMG_SRAM1_ADDR, BL_LLF_IMG_SRAM1_SIZE/4);

    //copy the rest part of MCU0 & MCU1 & MCU2 from SRAM2 to Save FW Dram Buffer
    ulTmpLocation = ulTmpLocation + BL_LLF_IMG_SRAM1_SIZE;
    COM_MemCpy((U32 *)ulTmpLocation, (U32 *)BL_LLF_IMG_SRAM2_ADDR, BL_LLF_IMG_SRAM2_SIZE/4);

    //copy the rest part of MCU0 & MCU1 & MCU2 from SRAM3 to Save FW Dram Buffer
    ulTmpLocation = ulTmpLocation + BL_LLF_IMG_SRAM2_SIZE;
    COM_MemCpy((U32 *)ulTmpLocation, (U32 *)BL_LLF_IMG_SRAM3_ADDR, BL_LLF_IMG_SRAM3_SIZE/4);
}

/*------------------------------------------------------------------------------
Name: BL_Part0StrapIsTLC
Description:
    Get Strapping info about TLC/MLC
Input Param:
    none
Output Param:
    none
Return Value:
    BOOL : TRUE: is TLC ; FLASE: is MLC
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
BOOL BL_Part0StrapIsTLC(void)
{
    return (BOOL)((rGLB(0x00)>>16)&0x1);
}

BOOL BL_Part0StrapIsOnfi(void)
{
    return (BOOL)((rGLB(0x40)>>6)&0x1);
}

BOOL BL_Part0StrapIsAsync(void)
{
    return !((BOOL)((rGLB(0x40)>>4)&0x1));
}


/*------------------------------------------------------------------------------
Name: BL_FlashCmdTableInit
Description:
    copy from ROM(HAL_NFC.c) HAL_FlashCmdTableInit( ) and simplify
Input Param:
    none
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL void BL_Part0CmdTableInit(void)
{
    U8 i;
    U32 Len = (sizeof(PRCQ_TABLE)*TL_PRCQ_CNT)/sizeof(U32);

    for (i = 0; i < Len; i++)
        *((U32 *)tl_l_aPrcqTable + i) = *((U32 *)tl_g_aPrcqTable + i);
}

/*------------------------------------------------------------------------------
Name: BL_NfcInitInterface
Description:
    copy from ROM(HAL_NFC.c) HalNfcInitInterface( ) and simplify
Input Param:
    none
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL void BL_Part0InitInterface(void)
{
    pNFCQDptr = (volatile NFC_CMD_STS *)NF_CMD_STS_REG_BASE;
    pNFCQArray = (volatile NFCQ *)NFCQ_ENTRY_BASE;
    pPrcqArray = (volatile U32 *)PRCQ_ENTRY_BASE;
    pNfcTrigger = (volatile NFC_TRIGGER *)REG_BASE_TRIG_NDC;
}

/*------------------------------------------------------------------------------
Name: BL_NfcPrcqInit
Description:
    copy from ROM(HAL_NFC.c) HAL_NfcPrcqInit( ) and simplify
Input Param:
    none
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL void BL_Part0PrcqInit(void)
{
    U32 ulQeCnt = 0;
    U32 ulPioQeCnt = 0;
    U8 ucCmdCodeIndex = 0;

    /*  g_pNfcPRCQ ->PRCQ_ENTRY_BASE  */
    volatile U8 *pNfcQe = (volatile U8 *)pPrcqArray;

    /*  fill every QE into SRAM align in byte unit  */
    for (ucCmdCodeIndex = 0; ucCmdCodeIndex < TL_PRCQ_CNT; ucCmdCodeIndex++) {
        do {
            *pNfcQe = tl_l_aQETable[ulQeCnt];
            pNfcQe++;
            ulQeCnt++;
        } while (TL_PRCQ_CMD_FINISH != tl_l_aQETable[ulQeCnt-1]);
        /*  align in DW    */
        pNfcQe = (volatile U8 *)ALIGN_IN_DW((U32)((U32 *)pNfcQe));
    }

    return;
}

/*------------------------------------------------------------------------------
Name: BL_NfcPrcqTableInit
Description:
    copy from ROM(HAL_NFC.c) HAL_NfcPrcqTableInit( ) and simplify
Input Param:
    none
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL void BL_Part0PrcqTableInit(void)
{
    U8  ucCmdCodeIndex = 0;
    U32 ulPrcqDwCnt = 0;
    U32 ulByteCnt = 0;  //record byte location of normal cmd PRCQ
    U32 ulQECnt = 0;    //record QE number of normal cmd PRCQ

    /*  init normal cmd: 1 byte for 1 QE, need to align in DW    */
    for (ucCmdCodeIndex = 0; ucCmdCodeIndex < TL_PRCQ_CNT; ucCmdCodeIndex++) {
        /*  get start PRCQ DW of this cmdCode   */
        tl_l_aPrcqTable[ucCmdCodeIndex].bsPRCQStartDw = ulPrcqDwCnt;

        /*  ACC QE and byte number  */
        do {
            ulByteCnt++;
            ulQECnt++;
        } while (TL_PRCQ_CMD_FINISH != tl_l_aQETable[ulQECnt-1]);

        /* align in DW  */
        ulByteCnt = ALIGN_IN_DW(ulByteCnt);

        /*  Get next cmd start DW  */
        ulPrcqDwCnt = ulByteCnt / sizeof(U32);
    }

    return;
}

/*------------------------------------------------------------------------------
Name: BL_NfcSinglePuStatus
Description:
    copy from ROM(HAL_NFC.c) HalNfcSinglePuStatus( ) and simplify
Input Param:
    U8 Pu : Pu number
    U8 Lun : Lun number
Output Param:
    none
Return Value:
    BOOL : TRUE:SUCCESS FALSE:FAIL
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
BOOL BL_Part0WaitStatus(U8 Pu)
{
    while (TRUE != (BOOL)pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsIdle)
    {
        ;
    }
    cdcDelay();

    if (TRUE == (BOOL)pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsErrh)
    {
        pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsPrst = 1;

        return FAIL;
    }
    else
    {
        return SUCCESS;
    }
}

BOOL BL_Part0IsLunAccessable(U8 ucPU)
{
    if ((TRUE == (BOOL)pNFCQDptr->aNfcqCmdStsReg[ucPU][0].bsFull) || (TRUE == (BOOL)pNFCQDptr->aNfcqCmdStsReg[ucPU][0].bsErrh))
    {
        return FALSE;
    }
    return TRUE;
}

void BL_Part0SetFeature(U8 Pu, U32 ulData, U8 ucAddr)
{
    volatile NFCQ_ENTRY *pNfcqEntry;
    volatile NFC_TRIGGER_REG *pTrig;
    NFC_TRIGGER_REG tTrig;
    U8 ucPU, Wp;

    while (FALSE == (BOOL)pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsEmpty)
        ;

    Wp = pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsWrPtr;
    pNfcqEntry = (NFCQ_ENTRY *)&pNFCQArray->aNfcqEntry[Pu][0][Wp];
    COM_MemZero((U32 *)pNfcqEntry, sizeof(NFCQ_ENTRY)>>2);

    pNfcqEntry->bsDmaByteEn = TRUE;
    pNfcqEntry->aByteAddr.usByteAddr = ucAddr;
    pNfcqEntry->aByteAddr.usByteLength = sizeof(U32);
    pNfcqEntry->ulSetFeatVal = ulData;
    pNfcqEntry->bsPrcqStartDw = tl_l_aPrcqTable[TL_PRCQ_SET_FEATURE].bsPRCQStartDw;

    //----HalSetTrigger(gPageReadCmdIndex,ucPhyPu,ucCeSel);----//
    Wp = pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsWrPtr;
    pTrig = (NFC_TRIGGER_REG*)&(pNfcTrigger->aNfcTriggerReg[Pu][0][Wp]);
    tTrig.ucTrigValue = 0;
    tTrig.bsCmdType = tl_l_aPrcqTable[TL_PRCQ_SET_FEATURE].bsTrigType;
    pTrig->ucTrigValue = tTrig.ucTrigValue;
    HAL_DelayCycle(5);
    HAL_MemoryWait();
}

void BL_Part0ReadPart1Page(U8 Pu, U8 CmdIndex)
{
    U8 Wp, Page = 1;//assume second page is all "page num = 1"
    U16 usCurDsgId,i;
    U32 ulDataLen = 0x3C00;
    volatile NFCQ_ENTRY *pNfcqEntry;
    volatile NORMAL_DSG_ENTRY *pDsgAddr;
    volatile NFC_TRIGGER_REG *pTrig;
    NFC_TRIGGER_REG tTrig;

    while (FALSE == (BOOL)pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsEmpty)
        ;

    //----pNfcqEntry = (NFCQ_ENTRY*)HalNfcGetNfcqEntry(Pu);----//
    Wp = pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsWrPtr;
    pNfcqEntry = (NFCQ_ENTRY *)&pNFCQArray->aNfcqEntry[Pu][0][Wp];
    COM_MemZero((U32 *)pNfcqEntry, sizeof(NFCQ_ENTRY)>>2);


    pNfcqEntry->bsDCrcEn = FALSE;
    pNfcqEntry->bsDsIndex = DS_ENTRY_SEL;
    pNfcqEntry->bsOntfEn = TRUE;
    pNfcqEntry->bsRomRd = TRUE;//Get DS column address info from register

    pNfcqEntry->bsPrcqStartDw = tl_l_aPrcqTable[CmdIndex].bsPRCQStartDw;

    //default value : scramble enable and seed = 0
    pNfcqEntry->bsScrSeed0 = HAL_FlashGetScrSeed(Page, 0, 0);
    pNfcqEntry->bsPuEnpMsk = FALSE;

    pNfcqEntry->aSecAddr[0].bsSecStart = 0;
    pNfcqEntry->aSecAddr[0].bsSecLength = ulDataLen >> SEC_SZ_BITS;
    pNfcqEntry->bsDmaTotalLength = ulDataLen >> SEC_SZ_BITS;
    pNfcqEntry->atRowAddr[0].bsRowAddr = Page;
    pNfcqEntry->bsDsgEn = TRUE;

    while (FALSE == HAL_GetNormalDsg(&usCurDsgId))
    {
        ;
    }

    pDsgAddr = (NORMAL_DSG_ENTRY *)HAL_GetNormalDsgAddr(usCurDsgId);
    COM_MemZero((U32 *)pDsgAddr, sizeof(NORMAL_DSG_ENTRY)>>2);
    pDsgAddr->bsDramAddr = (BL_P1_HEADER_TMP_OTFB_OFFSET>>1);
    //pDsgAddr->bsOntf = TRUE;  //bsOntf need set to 1 ?
    pDsgAddr->bsXferByteLen = ulDataLen;
    pDsgAddr->bsLast = TRUE;
    HAL_MemoryWait();
    pNfcqEntry->bsFstDsgPtr = usCurDsgId;
    HAL_MemoryWait();
    HAL_SetNormalDsgSts(usCurDsgId, TRUE);
    HAL_MemoryWait();

    //----HalSetTrigger(gPageReadCmdIndex,ucPhyPu,ucCeSel);----//
    Wp = pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsWrPtr;
    pTrig = (NFC_TRIGGER_REG*)&(pNfcTrigger->aNfcTriggerReg[Pu][0][Wp]);
    tTrig.ucTrigValue = 0;
    tTrig.bsCmdType = tl_l_aPrcqTable[CmdIndex].bsTrigType;
    pTrig->ucTrigValue = tTrig.ucTrigValue;
    HAL_DelayCycle(5);
    HAL_MemoryWait();
}

U8 BL_Part0ReadPart1Retry(U8 Pu, U8 CmdIndex)
{
    U8  ucRetryTime, ucErrCode;
    U32 ulCmdStatus, ulStart;
    RETRY_TABLE tRetryPara = {0};

    // reset CQ
    pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsPrst = TRUE;

    // Step1. pre-condition: enter to shift read mode.
    HAL_FlashRetryPreConditonForPart0(Pu);
    if (FAIL == BL_Part0WaitStatus(Pu))
    {
        return FAIL;
    }

    ucRetryTime = 0;
    while (NFC_STATUS_SUCCESS == HAL_FlashRetryCheckForPart0(ucRetryTime))
    {
        // Step2. set-parameters: adj voltage.
        ulStart = HAL_FlashSelRetryParaForPart0();
        tRetryPara = HAL_FlashGetRetryParaTabForPart0(ulStart + ucRetryTime);

        HAL_FlashRetrySendParamForPart0(Pu, &tRetryPara, HAL_FLASH_RETRY_PARA_MAX);
        if (FAIL == BL_Part0WaitStatus(Pu))
        {
            return FAIL;
        }

        // Step3. read-retry-enable:
        HAL_FlashRetryEnForPart0(Pu, TRUE);
        if (FAIL == BL_Part0WaitStatus(Pu))
        {
            return FAIL;
        }

        while (TRUE == (BOOL)pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsFull)
        {
            ;
        }

        // Step4. redo read:
        BL_Part0ReadPart1Page(Pu, CmdIndex);
        if (FAIL == BL_Part0WaitStatus(Pu))
        {
            pNFCQDptr->aNfcqCmdStsReg[Pu][0].bsPrst = TRUE;
            DBG_Printf("PU%d Current Retry Time:%d.\n", Pu, ucRetryTime);
            ucRetryTime++;
        }
        else
        {
            break;
        }
    }

    // Step5. terminate retry: enter to normal mode
    HAL_FlashRetryTerminateForPart0(Pu);
    if (FAIL == BL_Part0WaitStatus(Pu))
    {
        return FAIL;
    }

    if (FAIL == HAL_FlashRetryCheckForPart0(ucRetryTime))
    {
        DBG_Printf("Pu %d Read Retry Fail!\n", Pu);
        ucErrCode = FAIL;
    }
    else
    {
        DBG_Printf("Pu %d Read Retry OK!(Current Retry Time:%d)\n", Pu, ucRetryTime);
        ucErrCode = SUCCESS;
    }

    return ucErrCode;
}

/*------------------------------------------------------------------------------
Name: BL_Part0LoadPart1
Description:
    Rom-like single plane read function
Input Param:
    U8 CmdIndex : command type (TLC/MLC)
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL BOOL BL_Part0LoadPart1(U8 CmdIndex)
{
    U8 Pu = 0;

    //NFC_PU_MAX should change to PU cnt after read id
    while (Pu < NFC_PU_MAX) {

        BL_Part0ReadPart1Page(Pu, CmdIndex);

        if (FAIL == BL_Part0WaitStatus(Pu)) {
            DBG_Printf("Load BL Part1 FAIL at pu%d\n", Pu);
            if (FAIL == BL_Part0ReadPart1Retry(Pu, CmdIndex)) {
                DBG_Printf("Retry Load BL Part1 FAIL at pu%d\n", Pu);
                Pu++;
            } else {
                DBG_Printf("Retry Load BL Part1 PASS at pu%d\n", Pu);
                return SUCCESS;
            }
        } else {
                DBG_Printf("Load BL Part1 SUCCESS at pu%d lun%d\n", Pu, 0);
                return SUCCESS;
        }
    }
    return FAIL;
}



/*------------------------------------------------------------------------------
Name: BL_Part0Init
Description:
    copy from ROM(HAL_NFC.c) HAL_NfcInit( ) and simplify
Input Param:
    none
Output Param:
    none
Return Value:
    none
History:
    20160811   Henry   create.
------------------------------------------------------------------------------*/
LOCAL void BL_Part0Init()
{
    BL_Part0InitInterface();
    BL_Part0CmdTableInit();
    BL_Part0PrcqInit();
    BL_Part0PrcqTableInit();
    HAL_NormalDsgInit();
    BL_DDRInit();
}

int main(void)
{
    U8 ucReadCmdType;

    /* Enable MCU0 I-Cache to shorten bootloader & firmware booting time */
    rGlbMCUMisc |= 1 << 16;
    xthal_icache_all_unlock();
    xthal_icache_all_invalidate();
    xthal_set_icacheattr(0x11221122);
    xthal_icache_sync();

    HAL_WSRVecBase(MCU0_ISRAM_BASE);
#if !defined(XTMP) && !defined(SIM)
    uart_init();
    bPrinttoDRAM = HAL_GetPrinttoDRAMEnFlag();
#endif

    DBG_Printf("BL!\n");

   /*get GPIO8 value, if low, uart mp mode
   set GPIO8 input
   */
   rGPIO(0) &= ~(0x1 << 8);

#ifdef SIM
    //rGLB(0x50) = 0x3E1E00;//High;
    //rGLB(0x54) = 0x1E0E0; //low
    rGLB(0x54) = 0xF0E0; //low
    rNfcDbgSigGrpChg0 = 0x0000;//0x0000;//0x0001;
    rNfcDbgSigGrpChg1 = 0x230022;//0x4000C;//0x230022;//0x250026;//0x330016;//0x250023;//0x0000b;//0x9000b;
    rNfcDbgSigGrpChg2 = 0x220024;//0x240024;//0x220021;//0x150001;//0x150017;
#endif

    l_bNfcInitDoneFlag = FALSE;

#ifdef ASIC
    /* lock HAL_UnitTest flow */
    *g_pNfcSaveFwDone = FALSE;
#endif

    if ((BOOT_METHOD_NORMAL == BL_GetBootMethodSel()))
    {
        BL_Part0Init();

        if (BL_Part0StrapIsTLC())
            if (BL_Part0StrapIsOnfi() && BL_Part0StrapIsAsync())
                ucReadCmdType = TL_PRCQ_READ_IM_SLC_1PLN;
            else
                ucReadCmdType = TL_PRCQ_READ_SLC_1PLN;
        else
            ucReadCmdType = TL_PRCQ_READ_MLC_1PLN;

        //To do: modify to read all page to OTFB !! (or BL part1 too big will fail)
        if (SUCCESS != BL_Part0LoadPart1(ucReadCmdType))
        {
            DBG_Printf("All Pu Load BL Part1 FAIL\n");
            DBG_Getch();
        }

        glHeader = (IMAGE_HDR *)(HEADER_LOCATION);

        //move part 1 to correct exec address. To do : length should be 0x3c00 (if bl part 1 size not exceed)
        COM_MemCpy((U32 *)(glHeader->bootloader_addr), (U32 *)(HEADER_LOCATION+HEADER__SIZE), 0x1000);
    }
    else
    {
        glHeader = (IMAGE_HDR *)(HEADER_LOCATION);
        /*DBG_Printf("tinyloader_size 0x%x, tinyloader_addr 0x%x\n", glHeader->tinyloader_size,
        glHeader->tinyloader_addr);
        DBG_Printf("bootloader_size 0x%x, bootloader_addr 0x%x\n", glHeader->bootloader_size,
        glHeader->bootloader_addr);
        DBG_Printf("mcu0fw_size 0x%x, mcu0fw_addr 0x%x\n", glHeader->mcu0fw_size, glHeader->mcu0fw_addr);
        DBG_Printf("mcu1fw_size 0x%x, mcu1fw_addr 0x%x\n", glHeader->mcu1fw_size, glHeader->mcu1fw_addr);
        DBG_Printf("mcu2fw_size 0x%x, mcu2fw_addr 0x%x\n", glHeader->mcu2fw_size, glHeader->mcu2fw_addr);*/

        BL_DDRInit();

        BL_LoadBLnFWforLLF(BL_P1_TMP_OTFB_ADDR);

        //Copy system.bin to DRAM_FW_UPDATE_BASE for MCU0 Save FW.
        BL_CpySystemBin(SYSTEM_IMG_ADDR_SAVE_FW);
    }

    BL_HWInitAll();

    //DMA read data when UECC happen!!
    rNFC(0xA0) |= (0x1 << 10);

    DBG_Printf("BL_HWInitAll-Done\n");

#if 0
    BL_GetScriptVersion();
    BL_ExecAllRestOpCode();
    BL_TestGetHex();
    BL_TestGetString();
#endif
    BL_BootDelay();
    BL_RunFW();

    return;
}

#endif

