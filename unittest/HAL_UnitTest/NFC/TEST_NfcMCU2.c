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
Filename    : TEST_NfcMain.c
Version     : Ver 1.0
Author      : abby
Date        : 2015.11.10
Description : This file provide NFC driver test pattern, can be used both in
              COSIM and winsim.
              Contain Basic NFC test pattern.
Others      :
Modify      :
20151110    abby    porting from VT3514 test code
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcMCU2.h"

/*------------------------------------------------------------------------------
    VARIABLES DECLARATION
------------------------------------------------------------------------------*/
extern GLOBAL NFC_DS_ENTRY_REG g_aDsEntryTab[NF_DS_ENTRY_MAX];
extern GLOBAL MCU12_VAR_ATTR U32 *g_pFileBaseInDram;

/***************************************************************************/
/* External Functions                                                      */
/***************************************************************************/
extern void MCU12_DRAM_TEXT TEST_NfcBasicSharedMemMap(SUBSYSTEM_MEM_BASE * pFreeMemBase);
extern MCU12_DRAM_TEXT void TEST_NfcPattQStsInit(void);
extern BOOL MCU2_DRAM_TEXT TEST_NfcGetFeature(FLASH_ADDR *pFlashAddr, U8 ucAddr);
extern BOOL MCU2_DRAM_TEXT HAL_NfcSyncResetFlash(FLASH_ADDR *pFlashAddr);

#ifdef FLASH_3D_MLC
/*==============================================================================
Func Name  : TEST_Nfc3dMlcInit
Input      : void  
Output     : NONE
Return Val : 
Discription: For Intel 3D MLC flash init
Usage      : 
History    : 
    20160328    abby    create
==============================================================================*/
LOCAL void TEST_Nfc3dMlcInit(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U8  ucAddr, ucData;
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};

    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < SUBSYSTEM_PU_NUM; tFlashAddr.ucPU++)
    {
    #if 0//enable flash internal scramble
        /* step1: set feature, disable internal scramble, 92h/00h data */
        ucAddr = 0x92;
        ucData = 0x0;
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
    #endif
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
        
        /*  NVDDR2 patch  */
        //*(volatile U32*)0x1ff81018 |= (0x1<<30);
        
        /* step4: get feature to confirm flash feature, optional */
        ucAddr = 0x85;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        { 
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
                
        ucAddr = 0x92;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
    }
    
    /*  3D patch  */
    rNfcModeConfig &= (~(0x1<<31));
    rNfcModeConfig |= (0x1<<31);        //insert ccl enable
    
    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert
    
    rNfcPgCfg &= (~(0x1<<29));
    rNfcPgCfg |= (0x1<<29);             //ce hold enable
    
    DBG_Printf("Intel 3D MLC Test Init Done!!\n\n");

    return;
}
#endif

#ifdef FLASH_INTEL_3DTLC
/*==============================================================================
Func Name  : TEST_Nfc3dTlcInit
Input      : void  
Output     : NONE
Return Val : 
Discription: For Intel 3D MLC flash init
Usage      : 
History    : 
    20160328    abby    create
==============================================================================*/
LOCAL void TEST_Nfc3dTlcInit(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U8  ucAddr, ucData;
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};
    
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < SUBSYSTEM_PU_NUM; tFlashAddr.ucPU++)
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
        
    #if 0//option
        /* step3: set feature, switch to TLC mode, 91h/04h data, 00-SLC;01-MLC;04-TLC */
        ucAddr = 0x91;
        ucData = 0x4;
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr 0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
    #endif
        /* step3: set feature, disable internal scramble */
        ucAddr = 0x92;
        //ucData = 0x0;
        ucData = 0x1;//enable
        
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr 0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        
        /* step4: get feature to confirm flash feature, optional */
        ucAddr = 0x85;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {           
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
                
        ucAddr = 0x91;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {           
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        
        ucAddr = 0x92;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {           
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
    }
   
    /* step5: enable NFC scramble engine */
    rNfcTCtrl1 &= (~(1<<4));  // not bypass scramble

    /*  3D patch  */
    rNfcModeConfig &= (~(0x1<<31));
    rNfcModeConfig |= (0x1<<31);        //insert ccl enable
    
    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert
    
    rNfcPgCfg &= (~(0x1<<29));
    rNfcPgCfg |= (0x1<<29);             //ce hold enable

    DBG_Printf("Intel 3D TLC Test Init Done!!\n\n");

    return;
}

/*==============================================================================
Func Name  : TEST_Nfc3dTlcInit
Input      : void  
Output     : NONE
Return Val : 
Discription: For Intel 3D MLC flash init
Usage      : 
History    : 
    20160328    abby    create
==============================================================================*/
LOCAL void TEST_NfcMT3dTlcInit(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U8  ucAddr, ucData;
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};
    
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < SUBSYSTEM_PU_NUM; tFlashAddr.ucPU++)
    {
        /* step3: set feature, disable internal scramble */
        ucAddr = 0x92;
        ucData = 0x0;
        //ucData = 0x1;//enable
        
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr 0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
        
        /* step4: get feature to confirm flash feature, optional */                       
        ucAddr = 0x92;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {           
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
    }
    
   
    /* step5: enable NFC scramble engine */
    rNfcTCtrl1 &= (~(1<<4));  // not bypass scramble

    /*  3D patch  */
    //rNfcModeConfig &= (~(0x1<<31));
    //rNfcModeConfig |= (0x1<<31);        //insert ccl enable
    
    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert
    
    //rNfcPgCfg &= (~(0x1<<29));
    //rNfcPgCfg |= (0x1<<29);             //ce hold enable

    DBG_Printf("Intel 3D TLC Test Init Done!!\n\n");

    return;
}

#endif

#ifdef FLASH_MICRON_3DTLC_B16
LOCAL void TEST_NfcB16GetFeat(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U8  ucAddr;
    U32 ulData;
    U32 ulStatus;
    U32 aFeature[2] = {0, 0};

    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < SUBSYSTEM_PU_NUM; tFlashAddr.ucPU++)
    {
#if 0
        /* step1: set feature, User Selectable Trim Profile, 85h/03h data, 00-2-pass MLC;01-1-pass MLC;03-TLC */
        ucAddr = 0x85;
        ulData = 0x03;
        HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ulData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ulData);
        /* step3: sync reset to make flash reload trim file from ROM block */
        HAL_NfcSyncResetFlash(&tFlashAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("SYNC Reset PU:%d fail!\n", tFlashAddr.ucPU);
        }
        DBG_Printf("SYNC Reset PU:%d OK!\n", tFlashAddr.ucPU);
#endif
        ucAddr = 0x91;
        ulData = 0x104;
        HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
        }
        DBG_Printf("set feature OK PU:%d Addr0x%x Data0x%x\n", tFlashAddr.ucPU, ucAddr, ulData);
        
        
        /*  NVDDR2 patch  */
        *(volatile U32*)0x1ff81018 |= (0x1<<30);
        
        /* step4: get feature to confirm flash feature, optional */
        ucAddr = 0x85;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        { 
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
                
        ucAddr = 0x92;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }

        ucAddr = 0x91;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 0x10;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 0x80;

        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 2;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
        ucAddr = 1;
        if (NFC_STATUS_SUCCESS != TEST_NfcGetFeature(&tFlashAddr, ucAddr))
        {
            DBG_Printf("PU %d Get Feature fail!\n",tFlashAddr.ucPU);
        }
        else
        {
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("GetFeature Fail!\n");
            }
            else
            {
                HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            }
            DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             
        }
    }
    
    /*  3D patch  */
   // rNfcModeConfig &= (~(0x1<<31));
    //rNfcModeConfig |= (0x1<<31);        //insert ccl enable
    
    //rNfcModeConfig &= (~(0x1));
    //rNfcModeConfig |= (0x1);          //disable cmd insert
    
   // rNfcPgCfg &= (~(0x1<<29));
   // rNfcPgCfg |= (0x1<<29);             //ce hold enable
    
    DBG_Printf("B16 Test Get Feature Done!!\n\n");

    return;
}
#endif
/*------------------------------------------------------------------------------
Name: TEST_NfcSetSsu0Base
Description: 
    Initialize global variable g_ulSsuOtfbBase and g_ulSsu0DramBase.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to init SSU base addr.
History:
    20151030    abby    create.  
    20151116    abby    rmv MCU1 support 
    20160304    abby    rmv SSU1 base addr assignment
------------------------------------------------------------------------------*/
LOCAL void MCU2_DRAM_TEXT TEST_NfcSetSsu0Base(void)
{
    volatile NF_SSU_BASEADDR_REG *pSsuBaseAddr;
    pSsuBaseAddr = (volatile NF_SSU_BASEADDR_REG *)&rNfcSsuBaseAddr;

    g_ulSsuInOtfbBase = ((pSsuBaseAddr->bsSsuOtfbBase) << 10) + OTFB_START_ADDRESS;
    g_ulSsuInDramBase = ((pSsuBaseAddr->bsSsuDramBase) << 10) + DRAM_START_ADDRESS;

    return;
}

/*------------------------------------------------------------------------------
Function  : TEST_NfcSetSsu1Base
Input     :
Output    :

Purpose   : Set SSU1 BaseAddr
Reference :
------------------------------------------------------------------------------*/
LOCAL void TEST_NfcSetSsu1Base(void)
{
    g_ulSsu1OtfbBase = OTFB_SSU1_MCU12_SHARE_BASE;//g_ulSsuInOtfbBase + 1024;
    g_ulSsu1DramBase = g_ulSsuInDramBase + 1024;

    return;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcSetCacheStatusBase
Description: 
    Initialize global variable g_ulCacheStatusAddr.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    In FW. call this funtion to init Cache Status base addr
History:
    20151112    abby       create.  
------------------------------------------------------------------------------*/
LOCAL void TEST_NfcSetCacheStatusBase(void)
{    
    g_ulCacheStatusAddr = OTFB_TLG_MCU012_SHARE_BASE;
    
    return;
}

/*------------------------------------------------------------------------------
Name: TEST_DecFifoCmdIndexMapInit
Description:
    Mapping DEC FIFO cmdIndex to flash address for validation
    But FW don't need to adopt it in this way
Input Param:
Output Param:
    none
Return Value:
Usage:
History:
    20160505   abby    create
------------------------------------------------------------------------------*/
LOCAL void TEST_DecFifoCmdIndexMapInit(void)
{
    FLASH_ADDR tFlashAddr = {0};
    
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < NFC_PU_MAX; tFlashAddr.ucPU++)
    {
        for (tFlashAddr.ucLun = 0; tFlashAddr.ucLun < NFC_LUN_PER_PU; tFlashAddr.ucLun++)
        {
            for (tFlashAddr.usPage = TEST_PAGE_START; tFlashAddr.usPage < (U16)(TEST_PAGE_END * PRG_CYC_CNT); tFlashAddr.usPage++)
            {
                g_ucDecFifoCmdIndexMap[tFlashAddr.ucPU][tFlashAddr.ucLun][tFlashAddr.usPage] = 0;
            }
        }
    }
    return;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcDSInit
Description:
    Init all NFC DS entry for test
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    only for test, 8 types of data syntax, 4 kinds of codes of ldpc
History:
    20151225    abby    create
------------------------------------------------------------------------------*/
void MCU2_DRAM_TEXT TEST_NfcDSInit(void)
{
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    volatile NFC_DS_COLADDR_ENTRY *pDsSramEntry, *pPreDsSramEntry;
    U8  ucDsIndex = 0;
    U8  ucCodesel;
    U32 ulCwIndex = 0;
    U32 ulCwlength;
    
    /* initial all DS entry */
    for (ucDsIndex = DS_ENTRY0; ucDsIndex <= DS_ENTRY7; ucDsIndex++)
    {
        pDsReg->atDSEntry[ucDsIndex].bsRedNum = g_aDsEntryTab[ucDsIndex].bsRedNum;
        pDsReg->atDSEntry[ucDsIndex].bsNCRCEn = g_aDsEntryTab[ucDsIndex].bsNCRCEn;
        pDsReg->atDSEntry[ucDsIndex].bsLbaEn  = g_aDsEntryTab[ucDsIndex].bsLbaEn;
        pDsReg->atDSEntry[ucDsIndex].bsCwNum  = g_aDsEntryTab[ucDsIndex].bsCwNum;
    }

    COM_MemZero((U32*)g_pNfcDsColAddr, sizeof(NFC_DS_COLADDR)/sizeof(U32));
    
    for (ucDsIndex = DS_ENTRY0; ucDsIndex <= DS_ENTRY7; ucDsIndex++)
    {
        /*    init DW 0 - 3(bonding with CW 0 info) firstly, sel code 1    */
        ucCodesel = 1;
        pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, 0);
        DBG_Printf("TEST_NfcDSInit pDsSramEntry addr 0x%x ucDsIndex %d ulCwIndex %d ucCodesel%d\n", (U32)pDsSramEntry, ucDsIndex, ulCwIndex, ucCodesel);
        if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
        {
            ulCwlength = NFC_DS_CW_INFO_LENTH >> 1;
            //disable NCRC check
            rNfcPgCfg &= ~(1<<15);
            DBG_Printf("disable NCRC\n");
        }
        else
        {
            ulCwlength = NFC_DS_CW_CRC_LENTH >> 1;
        }
        pDsSramEntry->bsCodeSel = ucCodesel;
        pDsSramEntry->bsCwLenth = ulCwlength;
        pDsSramEntry->bsCwClAddr = 0;
        /* CW 1-12  */
        for (ulCwIndex = 1; ulCwIndex < 13; ulCwIndex++)
        {
            pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex);
            pPreDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex - 1);
            pDsSramEntry->bsCodeSel = ucCodesel;
            DBG_Printf("TEST_NfcDSInit ulCwIndex%d ucCodesel%d\n", ulCwIndex, ucCodesel);
            if ( (0 == (ulCwIndex + 1) % CW_PER_LBA) && (FALSE != (pDsReg->atDSEntry[ucDsIndex].bsLbaEn)) )
            {
                if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
                {
                    pDsSramEntry->bsCwLenth = NFC_DS_CW_LBA_LENTH >> 1;
                }
                else
                {
                    pDsSramEntry->bsCwLenth = NFC_DS_CW_CRC_LBA_LENTH >> 1;
                }
            }
            else
            {
                pDsSramEntry->bsCwLenth = ulCwlength;
            }
            pDsSramEntry->bsCwClAddr = pPreDsSramEntry->bsCwClAddr + pPreDsSramEntry->bsCwLenth + (g_atLdpcMatTable[LDPC_MAT_MODE][ucCodesel] >> 1);
        }
        /* CW13 */
        ulCwIndex = 13;
        ucCodesel = 1;//2;
        pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex);
        pPreDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex - 1);
        pDsSramEntry->bsCodeSel = ucCodesel;
        DBG_Printf("TEST_NfcDSInit ulCwIndex%d ucCodesel%d\n", ulCwIndex, ucCodesel);
        if ( (0 == (ulCwIndex + 1) % CW_PER_LBA) && (FALSE != (pDsReg->atDSEntry[ucDsIndex].bsLbaEn)) )
        {
            if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
            {
                pDsSramEntry->bsCwLenth = NFC_DS_CW_LBA_LENTH >> 1;
            }
            else
            {
                pDsSramEntry->bsCwLenth = NFC_DS_CW_CRC_LBA_LENTH >> 1;
            }
        }
        else
        {
            pDsSramEntry->bsCwLenth = ulCwlength;
        }
        pDsSramEntry->bsCwClAddr = pPreDsSramEntry->bsCwClAddr + pPreDsSramEntry->bsCwLenth + (g_atLdpcMatTable[LDPC_MAT_MODE][ucCodesel] >> 1);
        /* CW14 */
        ulCwIndex = 14;
        ucCodesel = 1;//3;
        pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex);
        pPreDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex - 1);
        pDsSramEntry->bsCodeSel = ucCodesel;
        DBG_Printf("TEST_NfcDSInit ulCwIndex%d ucCodesel%d\n", ulCwIndex, ucCodesel);
        if ( (0 == (ulCwIndex + 1) % CW_PER_LBA) && (FALSE != (pDsReg->atDSEntry[ucDsIndex].bsLbaEn)) )
        {
            if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
            {
                pDsSramEntry->bsCwLenth = NFC_DS_CW_LBA_LENTH >> 1;
            }
            else
            {
                pDsSramEntry->bsCwLenth = NFC_DS_CW_CRC_LBA_LENTH >> 1;
            }
        }
        else
        {
            pDsSramEntry->bsCwLenth = ulCwlength;
        }
        pDsSramEntry->bsCwClAddr = pPreDsSramEntry->bsCwClAddr + pPreDsSramEntry->bsCwLenth + (g_atLdpcMatTable[LDPC_MAT_MODE][ucCodesel] >> 1);
        /* CW15 */
        ulCwIndex = (g_aDsEntryTab[ucDsIndex].bsCwNum - 1);//15;
        ucCodesel = 0;
        pDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex);
        pPreDsSramEntry = HAL_NfcGetDsSramEntryAddr(ucDsIndex, ulCwIndex - 1);
        pDsSramEntry->bsCodeSel = ucCodesel;
        DBG_Printf("TEST_NfcDSInit ulCwIndex%d ucCodesel%d\n", ulCwIndex, ucCodesel);
        if ( (0 == (ulCwIndex + 1) % CW_PER_LBA) && (FALSE != (pDsReg->atDSEntry[ucDsIndex].bsLbaEn)) )
        {
            if (FALSE == (pDsReg->atDSEntry[ucDsIndex].bsNCRCEn))
            {
                pDsSramEntry->bsCwLenth = NFC_DS_CW_LBA_LENTH >> 1;
            }
            else
            {
                pDsSramEntry->bsCwLenth = NFC_DS_CW_CRC_LBA_LENTH >> 1;
            }
        }
        else
        {
            pDsSramEntry->bsCwLenth = ulCwlength;
        }
        pDsSramEntry->bsCwClAddr = pPreDsSramEntry->bsCwClAddr + pPreDsSramEntry->bsCwLenth + (g_atLdpcMatTable[LDPC_MAT_MODE][ucCodesel] >> 1);
    } 

    return;
}

/****************************************************************************
Function      : TEST_NfcBasicInit
Input         :
Output        :

Description    : Init test function related pointer or data structure
Reference     :
History        :
    20151112    abby    create
****************************************************************************/
void TEST_NfcBasicPattRunInit(void)
{
    U32 i;
    U32 ulRedNum;

    DBG_Printf("START_BUFFER_ID: 0x%x\n", START_BUFFER_ID);
    DBG_Printf("START_WBUF_ID: 0x%x\n", START_WBUF_ID);
    DBG_Printf("START_RBUF_ID: 0x%x\n", START_RBUF_ID);
    DBG_Printf("START_WRED_ADDR: 0x%x\n", START_WRED_ADDR);
    DBG_Printf("START_RRED_BASE: 0x%x\n", START_RRED_BASE);
    DBG_Printf("START_RRED_ADDR: 0x%x\n", START_RRED_ADDR);

    /*  allocate red pointer address     */
    pWrRed = (volatile NFC_RED *)START_WRED_ADDR;
    pRRed = (volatile NFC_RED **)START_RRED_BASE;

    ulRedNum = NFCQ_DEPTH;//TEST_ADDR_OFF << NFCQ_DEPTH_BIT;//NFCQ_DEPTH;//TEST_ADDR_OFF << NFCQ_DEPTH_BIT;
    for(i = 0; i < ulRedNum ; i++)
    {
        pRRed[i] = (NFC_RED *)(START_RRED_ADDR + i * sizeof(NFC_RED));
    }

    /*  Set SSU & Cache Status base addr  */
    TEST_NfcSetSsu0Base();
    TEST_NfcSetSsu1Base();
    TEST_NfcSetCacheStatusBase();
    
    /*  DEC FIFO cmd index mapping */
    TEST_DecFifoCmdIndexMapInit();

#if 0//only for test
    TEST_NfcDSInit();
#endif

    TEST_NfcBasicSharedMemMap(&g_FreeMemBase);
    TEST_NfcPattQStsInit();

    DBG_Printf("NFC Basic UT Init Done!\n\n");
   
    return;
}

void TEST_NfcExtPattRunInit(void)
{
    g_pLocalFCmdReq     = g_ptFCmdReq;
    g_pLocalFCmdReqDptr = g_ptFCmdReqDptr;
    g_pLocalFCmdReqSts  = g_ptFCmdReqSts;

    TEST_NfcInitLLFFlag();
}

LOCAL BOOL TEST_NfcMCU2EventCheck(void)
{
    COMMON_EVENT NFC_Event;

    if (COMM_EVENT_STATUS_SUCCESS_NOEVENT != CommCheckEvent(COMM_EVENT_OWNER_L3, &NFC_Event))
    {
        if (!NFC_Event.EventBoot)
        {
            DBG_Printf("MCU2 Event Invalid!\n");
        }
        return FALSE;
    }

    return TRUE;
}
/****************************************************************************
Function      : TEST_NfcMcu2Main
Input         :
Output        :
Description   : NFC driver test main function
Reference     :
History       :
    20151112    abby    create
    20160904    abby    improve to 2.0
****************************************************************************/
void TEST_NfcMCU2Main(void)
{ 
    /* waiting event clear by MCU1 */
    if (FALSE == TEST_NfcMCU2EventCheck())
    {
        return;
    }

    U8 ucFileType = TEST_NfcGetCheckListFileType();;

    /*  DBG single of NFC   */
#ifdef DBG_SINGNAL
#if 1 //NFC
//    rGLB(0x50) = 0x3E1E00;
//    rGLB(0x54) = 0x801E0E0;

//    rNfcDbgSigGrpChg0  = 0x1111;//IFC:0 ICB:C QPIPE:1
//    rNfcDbgSigGrpChg1  = (0x0<<28)|(0xB<<16)|(0x0<<12)|(0x10);//0x240004;//0x240023;//0x2b0004;
//    rNfcDbgSigGrpChg2  = (0x0<<28)|(0xB<<16)|(0x0<<12)|(0x10);//0x26000c;//0x260025;//0x150001;//0x150017;

    //rGLB(0x50) = (0x155<<13)|0x1E00;
    //rGLB(0x54) = (1<<27)|(0x55<<9)|0xE0;
//    rGLB(0x50) = (0x155<<13)|(0x1E0<<4);
//    rGLB(0x54) = (1<<27)|(0x55<<9)|(0xE0);
//      rGLB(0x50) = (0x155<<13)|(0x154<<4);//B3/B1
//      rGLB(0x54) = (1<<27)|(0x55<<9)|(0x54); //B0/B2 of DMA
//      //rGLB(0x54) = (1<<27)|(0xF0<<9)|(0xE0);//B2/B0 of NFC
//    rNfcNfdmaCfg2 = 0x81806d65;//0x80818181;//B3/B1

//    rNfcDbgSigGrpChg0  = 0x1111;//IFC:0 ICB:C QPIPE:1
//    rNfcDbgSigGrpChg1  = (0x0<<28)|(0x11<<16)|(0x0<<12)|(0x10);//0x240004;//0x240023;//0x2b0004;
//    rNfcDbgSigGrpChg2  = (0x0<<28)|(0x10<<16)|(0x0<<12)|(0x11);//0x26000c;//0x260025;//0x150001;//0x150017;
    
    
    /* DSG */
    rGLB(0x50) = (0x155<<13)|(0x154<<4);//B1/B3 of DMA
    rGLB(0x54) = (1<<27)|(0x55<<9)|(0x54); //B0/B2 of DMA
    
    rNfcNfdmaCfg2 = 0x81658180;
    rNfcDbgSigGrpChg0 = 0;
    rNfcDbgSigGrpChg1  = 0;//(0x0<<28)|(0x6<<16)|(0x0<<12)|(0x7);
    //rNfcDbgSigGrpChg2  = (0x0<<28)|(0x2<<16)|(0x0<<12)|(0x3);

#else//DRAM
    //*(volatile U32*)0x1ff80c10 = 0x1883625a;
    rGLB(0x50) = (0x1E0<<22);//B1 of NFC
    rGLB(0x54) = (1<<28)|(0xE0<<18); //B0 of NFC
    rNfcDbgSigGrpChg0  = 0x5555;
    rNfcDbgSigGrpChg1  = (0x0<<28)|(0x16<<16)|(0x0<<12)|(0xA);//(0x2<<28)|(0xB<<16)|(0x2<<12)|(0xA);
    rNfcDbgSigGrpChg2  = (0x0<<28)|(0x16<<16)|(0x0<<12)|(0xA);

    /* DMA debug signals */
    //rGLB(0x50) = (0x154<<22);//B1 of DMA
    //rGLB(0x54) = (1<<28)|(0x54<<18); //B0 of DMA
    //rNfcDbgSigGrpChg0 = 0;
    //rNfcDbgSigGrpChg1 = 0x0<<12;
    //rNfcNfdmaCfg2 = 0x96969696;
#endif
#endif

    if (BASIC_CHK_LIST == ucFileType)
    {
        TEST_NfcBasicPattRunInit();
    }
    else//ext file
    {
        TEST_NfcExtPattRunInit();
    }

    //TEST_NfcB16GetFeat();
    
    while(TRUE)
    {
        if (BASIC_CHK_LIST == ucFileType)
        {
            TEST_NfcBasicPattRun();
        }
        else//ext file
        {
            TEST_NfcExtPattRun();
        }
    }

    return;
}

/*==============================================================================
Func Name  : UT_NfcMain
Input      : void  
Output     : NONE
Return Val : 
Discription: The TEST_NfcMain should return after n cycles test.
Usage      : 
History    : 
    1. 2016.3.19 VIA create function
==============================================================================*/
BOOL UT_NfcMain(void)
{
    DBG_Printf("\nHAL NFC UNIT TEST START!\n");

    TEST_NfcMCU2Main();

    DBG_Printf("\nHAL NFC UNIT TEST END!\n\n");

    while(1);
    
    return TRUE;
}

//-------- end of this file ----------//

