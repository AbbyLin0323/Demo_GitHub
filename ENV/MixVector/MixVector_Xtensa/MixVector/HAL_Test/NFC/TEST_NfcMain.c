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
 * File Name    : TEST_NfcMain.c
 * Discription  : this document is the tesp pattern file for HostRequest.
 * CreateAuthor : tobey
 * CreateDate   : 2013.11.7
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "TEST_NfcMain.h"




ASYNC_TIMING_CONFIG t_aAsyncTConfTab[2][4] =
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

LOCAL void NfcTestInit(void)
{
#ifdef COSIM
    if (TRUE == TEST_NfcDramInitEn())
    {
        HAL_DramcInit();
    }    
    HAL_NfcAsyncTimingInit(TOGGLE2,NFTM_MODE4);
#endif

    HAL_FlashInit();
#ifdef COSIM
    HAL_NfcDefaultPuMapInit();
#endif
    //initilaztion otfb relative reg
    // for B0 version:GLB28 = 0x16a30300; GLB2c = 0x128398a4;
    rGlbOTFBMCtrl1 = 0xa418820;
    rGlbOTFBMCtrl0 = 0xe620600;//((0xe620000 & 0xffff0000)| (0x600 & 0xffff));

    //ahci pcie relative (informeb by vince)
    rGlbSoftRst = 0x0;
    rGLB(0x54) = 0x0;

    HAL_DrqInit();
    HAL_DwqInit();
    HAL_NormalDsgInit();
    HAL_HsgInit();
}

TEST_AsyncOnfiSinglePlnRead()
{
    FLASH_ADDR tFlashAddr = {0};
    U32 i,ulAddr;
#ifdef COSIM
    if(TRUE == TEST_NfcDramInitEn())
    {
        HAL_DramcInit();
    }
#endif
    rNFC(0x00) = 0x61F8C92E;
    rNFC(0x4) = 0x814c810c;
    rNFC(0x8) = 0x01102494;
    rNFC(0x2c) = 0;
    rNFC(0x30) = 0x23000055;
    rNFC(0x80) = 0;
    rNFC(0x88) = 0x00d08900;
    rPMU(0x20) = 0x50;

    HAL_NfcInterfaceInitBasic();
    HAL_NormalDsgInit();

    TRACE_OUT(0x3c3c,0x3c3c,0x3c3c,0x3c3c);

    ulAddr =  DRAM_START_ADDRESS;

    for (i=0;i<SEC_PER_PG;i++)
    {
        *(U32*)(ulAddr + i*SEC_SIZE) = i + 1;
        *(U32*)(ulAddr + i*SEC_SIZE + 4) = i + 1 + 0x8000;
    }

    HAL_NfcSinglePlaneWrite(&tFlashAddr,0,NULL);
    HAL_NfcWaitStatus(tFlashAddr.ucPU);

    HAL_NfcSinglePlnRead(&tFlashAddr,0,SEC_PER_PG,TRUE,0,NULL);
    HAL_NfcWaitStatus(tFlashAddr.ucPU);

    ulAddr =  OTFB_START_ADDRESS;
    for (i=0;i<SEC_PER_PG;i++)
    {
        if ((*(U32*)(ulAddr + i*SEC_SIZE) != i + 1)||
        (*(U32*)(ulAddr + i*SEC_SIZE + 4) != i + 1 +  0x8000))
        {

             TRACE_OUT(0xbadc0de,ulAddr,i+1,*(U32*)(ulAddr + i*SEC_SIZE));
             TRACE_OUT(0xbadc0de,ulAddr,i+1 + 0x8000,*(U32*)(ulAddr + i*SEC_SIZE + 4));
             DBG_Getch();
        }

    }

}

#ifndef FPGA
void TEST_NfcMain(void)
{
    DBG_Printf("Enter Test\n");
    TRACE_PATTREN_RECORD;
    NfcTestInit();
    TEST_NfcBasicInit();
    DBG_Printf("NFC init pass\n");
    //TRACE_OUT(NFC_MODULE_TEST_START,__FILE__,__LINE__,0);

    //TEST_NfcMultiPu();
    //DBG_Printf("MultiPu pass!!\n");

   // TEST_NfcCacheRdAfterWr();
     TEST_MixPatternMCU12();
   // TEST_NfcBasic();
    //TRACE_OUT(0xbeef,__FILE__,__LINE__,NFC_MODULE_TEST_FINISH);

    DBG_Printf("Pattern pass!!\n");
    //TRACE_OUT(0xbeef,__FILE__,__LINE__,NFC_MODULE_TEST_FINISH);
    while(1);
}
#else
void TEST_NfcMain(void)
{
    DBG_Printf("Enter Test\n");
    TRACE_PATTREN_RECORD;
    NfcTestInit();
    TEST_NfcBasicInit();
   // DBG_Printf("WBUF ADDR : 0x%x\n",DRAM_DATA_BUFF_MCU1_BASE);
    //DBG signal for RED,add by abby
   /* rGLB_54           = 0xF0E0;  //SEL NFC DBG SIGNAL
    rNfcDbgSigGrpChg0   = 0x5;     //MOD_SEL:RED
    rNfcDbgSigGrpChg1   = 0x030004;//0x240023;//BYTE 1:DMA_DIN[15:8],BYTE 0:DMA_DIN[7:0]
    */
    //DBG signal for DMA,add by abby
    rGLB(0x54)             = 0xF0E0;
    rNfcDbgSigGrpChg0   = 0x7;
    rNfcDbgSigGrpChg1   = 0x260009; //0x080009;   //BYTE 1:DMA_DIN[15:8],BYTE 0:DMA_DIN[7:0]
    //rNfcDbgSigGrpChg2   = 0x260025;         //BYTE 1:DMA_DIN[15:8],BYTE 0:DMA_DIN[7:0]

    //TEST_AsyncOnfiSinglePlnRead();
    //TEST_ReadBootloader();

    TEST_NfcBasic();
    DBG_Printf("Pattern pass!!\n");

    //TEST_NfcMultiPuForError();
    //TEST_NfcMultiPu();
    DBG_Printf("MultiPu pass!!\n");

   // TEST_NfcCacheRdAfterWr();
   //  TEST_MixPatternMCU12();

    while(1);
}
#endif



