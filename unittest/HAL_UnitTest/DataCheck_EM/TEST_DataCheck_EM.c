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
20160110    SparkSun  Porting to VT3533
****************************************************************************/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
#include "BaseDef.h"
#include "COM_Memory.h"
#include "HAL_MemoryMap.h"
#include "HAL_GLBReg.h"
#include "HAL_HostInterface.h"
#include "HAL_NormalDSG.h"
#include "HAL_FlashChipDefine.h"
#include "HAL_FlashCmd.h"
#include "HAL_FlashDriverExt.h"
#include "HAL_FlashDriverBasic.h"
#include "HAL_NfcDataCheck.h"
#include "HAL_EncriptionModule.h"
#include "FW_BufAddr.h"
#include "HAL_MultiCore.h"
#include "Disk_Config.h"

/* main entry of EM test pattern */
void TEST_NfcEmMain(void)
{
    FLASH_ADDR tFlashAddr;
    U32 TargetAddr, ulStatus;
    U32 SecIndex, SecNum;
    NFC_RED tWriteRedSW={0}, *ptReadRedSW;
    U32 WriteBuffID = 0;
    U32 ReadBuffID = 1;
    U32 aIv[4] = {0};   

    COM_MemZero(&aIv[0], 4);
    /* Config and enable Encryption Module */
    HAL_EMInit(EM_KEY_SIZE_256BIT, EM_MODE_XTS,&aIv[0]);
    
    /* use PU2 for test */
    tFlashAddr.ucPU = 2;
    tFlashAddr.ucLun = 0;
    tFlashAddr.bsPln = 0;
    tFlashAddr.usBlock = 0;
    tFlashAddr.usPage = 0;

    /* 1. erase block first */
    ulStatus = HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
    if (NFC_STATUS_SUCCESS != ulStatus)
    {
        DBG_Printf("MCU#%d pu%d HalNfcRawBlockErase err1.\n", HAL_GetMcuId(), tFlashAddr.ucPU);
        DBG_Getch();
    }
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("MCU#%d pu%d HalNfcRawBlockErase err2.\n", HAL_GetMcuId(), tFlashAddr.ucPU);
        DBG_Getch();
    }

    /* 2. write data to flash */ 
    TargetAddr = COM_GetMemAddrByBufferID(WriteBuffID,TRUE, BUF_SIZE_BITS);
    SecNum     = SEC_PER_BUF;
    for (SecIndex=0; SecIndex<SecNum; SecIndex++)
    {
        COM_MemSet((U32*)(TargetAddr+SecIndex*SEC_SIZE/sizeof(U32)), 1, 0x5a5a5a00 + tFlashAddr.usPage);
    }
    COM_MemSet((U32*)&tWriteRedSW, RED_SZ_DW, 0x4a4a4a00+tFlashAddr.usPage);
    /* full page write now */
    ulStatus = HAL_NfcFullPageWrite(&tFlashAddr, WriteBuffID, (NFC_RED *)&tWriteRedSW);
    if (NFC_STATUS_SUCCESS != ulStatus)
    {
        DBG_Printf("MCU#%d pu%d HalNfcRawPageWrite err1.\n",HAL_GetMcuId(),tFlashAddr.ucPU);
        DBG_Getch();
    }
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("MCU#%d pu%d HalNfcRawPageWrite err2.\n",HAL_GetMcuId(),tFlashAddr.ucPU);
        DBG_Getch();
    }
        
    /* 3. page write done, read back data from flash */
    ulStatus = HAL_NfcPageRead(&tFlashAddr, 0,SEC_PER_BUF,ReadBuffID,(NFC_RED **)&ptReadRedSW);
    TargetAddr = COM_GetMemAddrByBufferID(ReadBuffID, TRUE, BUF_SIZE_BITS);
    SecNum     = SEC_PER_BUF;
    if (NFC_STATUS_SUCCESS != ulStatus)
    {
        DBG_Printf("MCU#%d pu%d HalNfcRawPageRead err1.\n",HAL_GetMcuId(),tFlashAddr.ucPU);
        DBG_Getch();
    }
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("MCU#%d pu%d HalNfcRawPageRead err2.\n",HAL_GetMcuId(),tFlashAddr.ucPU);
        DBG_Getch();
    }
    
    /* 4. data compare and check */
    for (SecIndex=0; SecIndex<SecNum; SecIndex++)
    {
         if (*(U32*)(TargetAddr+SecIndex*SEC_SIZE/sizeof(U32)) != 0x5a5a5a00 + tFlashAddr.usPage)
         {
            DBG_Printf("MCU#%d Data Check Err: 0x%x, 0x%x\n", HAL_GetMcuId(), 
                    *(U32*)(TargetAddr+SecIndex*SEC_SIZE/sizeof(U32)), 0x5a5a5a00 + tFlashAddr.usPage);
            DBG_Getch();
         }
    }
    if (*(U32*)ptReadRedSW != 0x4a4a4a00+tFlashAddr.usPage)
    {
        DBG_Printf("MCU#%d Red Check Err: 0x%x, 0x%x\n", HAL_GetMcuId(),
                                    *(U32*)ptReadRedSW, 0x4a4a4a00+tFlashAddr.usPage);
        DBG_Getch();
    }

    /* 5. if data check ok, Encryption Module works ok */
    DBG_Printf("MCU#%d PU %d Blk %d Page %d: EM data check ok\n", 
                        HAL_GetMcuId(),tFlashAddr.ucPU, tFlashAddr.usBlock, tFlashAddr.usPage);
    return;
}


LOCAL void DramNfcInit(void)
{
#ifdef COSIM
    HAL_DramcInit();
    HAL_NfcAsyncTimingInit(TOGGLE2, NFTM_MODE4);
#endif

    HAL_FlashInit();

#ifdef COSIM
    HAL_NfcDefaultPuMapInit();
#endif

    //initilaztion OTFB relative reg
    rGlbOTFBMCtrl1 = 0xa418820;
    rGlbOTFBMCtrl0 = 0xe620600;//((0xe620000 & 0xffff0000)| (0x600 & 0xffff));

    rTracer = 0x0F;
    rGlbSoftRst = 0x0;
    rGLB(0x54) = 0x0;

    HAL_NormalDsgInit();
}

/* main entry of EM test pattern */
void TEST_DataCheck_EM(void)
{
    DramNfcInit();

    rTracer = 0xDF;

    TEST_NfcEmMain();

}
