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
Filename    : TEST_NfcFuncBasic.c
Version     : Ver 1.0
Author      : abby
Date        : 20160903
Description : 
Others      :
Modify      :
*******************************************************************************/

/*------------------------------------------------------------------------------
    INCLUDING FILES DECLARATION
------------------------------------------------------------------------------*/
#include "TEST_NfcFuncBasic.h"

/*------------------------------------------------------------------------------
    VARIABLE DECLARATION
------------------------------------------------------------------------------*/
GLOBAL BOOL g_bSsuEn;
GLOBAL BOOL g_bCacheStsEn;
GLOBAL BOOL g_bSsu0DramEn;
GLOBAL BOOL g_bSsu1DramEn;
GLOBAL BOOL g_bLbaChk;
GLOBAL BOOL g_bRedOntf;
GLOBAL BOOL g_bDecFifoEn;
GLOBAL volatile BOOL g_bSinglePln;
GLOBAL BOOL g_bForceRetryEn;
GLOBAL BOOL g_bSnapRead;
GLOBAL U8 g_ucEntryIndex;
GLOBAL MCU12_VAR_ATTR BOOL g_bRawDataRead;

/*  NFC feature control, evolution to FCMD */
GLOBAL U8 g_ucTestPuStart, g_ucTestPuEnd;
GLOBAL U8  g_ucTestLunStart, g_ucTestLunEnd;
GLOBAL U16 g_usTestBlkStart, g_usTestBlkEnd;
GLOBAL U16 g_usTestPageStart, g_usTestPageEnd;

LOCAL U32 g_ulPattStart, g_ulPattEnd;

/*  SSU0  */
GLOBAL MCU12_VAR_ATTR U32 g_ulSsuInOtfbBase;
GLOBAL MCU12_VAR_ATTR U32 g_ulSsuInDramBase;
/*  SSU1  */
GLOBAL MCU12_VAR_ATTR U32 g_ulSsu1OtfbBase;
GLOBAL MCU12_VAR_ATTR U32 g_ulSsu1DramBase;
/*  cache status  */
GLOBAL MCU12_VAR_ATTR U32 g_ulCacheStatusAddr;

GLOBAL U8 g_ucDecFifoCmdIndexMap[NFC_PU_MAX][NFC_LUN_PER_PU][PG_PER_BLK * PGADDR_PER_PRG * INTRPG_PER_PGADDR];

GLOBAL volatile NFC_RED *pWrRed;
GLOBAL volatile NFC_RED **pRRed;

extern GLOBAL BOOL g_ErrInjEn;
extern GLOBAL NFC_ERR_INJ g_tErrInj;

extern GLOBAL MCU12_VAR_ATTR PRCQ_TABLE g_aPrcqTable[FLASH_PRCQ_CNT];

extern BOOL TEST_NfcSwitchFlashMode(U8 ucPU);

/*------------------------------------------------------------------------------
Name: TEST_Delay_ms
Description:
   Add delay as ms unit
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:

History:
    20151126    abby    create
------------------------------------------------------------------------------*/
LOCAL void TEST_Delay_ms(U32 ulMsCount)
{
    U32 ulLastTimeTag;
    U32 ulCurrTimeTag;
    U32 ulTargetTime;
    U8  ucTargetOvevflow = FALSE;
    U8  ucCurrOverflow   = FALSE;
    ulLastTimeTag = HAL_GetMCUCycleCount();
    //ulTargetTime = ulLastTimeTag + ulMsCount * 1000 * 300;//COUNT_INoneUS;
    ulTargetTime = ulLastTimeTag + ulMsCount * 100 * 400; //FOR FPGA HCLK=40MHz
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
    return;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcResetAll
Description:
    reset all PU and all LUN
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Test flash reset cmd and LUN reset cmd
History:
    20151123    abby    add header
    20160229    abby    add reset lun test code
------------------------------------------------------------------------------*/
LOCAL void TEST_NfcResetAll(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulStatus;
    
    /*  reset PU    */
    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        HAL_NfcResetFlash(&tFlashAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);

        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d Reset Fail!\n",tFlashAddr.ucPU);
            NFC_GETCH;
        }
        //DBG_Printf("PU %d Reset OK!\n", tFlashAddr.ucPU);
    }
    /*  reset LUN    */
    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            HAL_NfcResetLun(&tFlashAddr);
            ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);

            if (NFC_STATUS_SUCCESS != ulStatus)
            {
                DBG_Printf("PU %d LUN %d Reset Fail!\n",tFlashAddr.ucPU, tFlashAddr.ucLun);
                NFC_GETCH;
            }
            //DBG_Printf("PU %d LUN %d Reset OK!\n", tFlashAddr.ucPU, tFlashAddr.ucLun);
        }
    }
    return;
}

/****************************************************************************
Function      : TEST_NfcReadID
Input         :
Output        :
Description   : To read ID in byte mode.
                Need choose FLASH_ID as the practical flash die
History       :
    20151112    abby    create
****************************************************************************/
LOCAL void TEST_NfcReadID(void)
{
    FLASH_ID ulFalshType;
    FLASH_ADDR tFlashAddr = {0};
    BOOL bsEql = FALSE;
    U32 aID[2] = {0, 0};
    volatile BOOL bStatus;

    ulFalshType = TYPE_INT_3D_TLC;
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < SUBSYSTEM_PU_NUM; tFlashAddr.ucPU++)
    {

        /*    read ID  */
        HAL_NfcReadFlashId(&tFlashAddr);
        bStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != bStatus)
        {
            DBG_Printf("PU %d Read ID fail!\n",tFlashAddr.ucPU);
        }

        /*    Get ID from SRAM  */
        bStatus = HAL_DecSramGetFlashId(&tFlashAddr, aID);
        if (NFC_STATUS_SUCCESS != bStatus)
        {
            DBG_Printf("PU %d Get ID Fail!\n",tFlashAddr.ucPU);
        }
    #ifdef COSIM
        if(aID[0] == 0x7EE4AEEC && aID[1] == 0xC478)//TOGGLE FLASH MODEL ID
            bsEql = TRUE;
    #else
        switch (ulFalshType)
        {
            case (TYPE_TSB_3D_TLC) :
            {
                if(aID[0] == 0xb3983c98 && aID[1] == 0x0608f176)
                    bsEql = TRUE;
            }
            break;

            case (TYPE_INT_3D_TLC) :
            {
                if(aID[0] == 0x3208a489 && aID[1] == 0x000000a1)
                    bsEql = TRUE;
            }
            break;

            case (TYPE_INT_3D_MLC) :
            {
                if(aID[0] == 0x3264a489 && aID[1] == 0x000001aa)
                    bsEql = TRUE;
            }
            break;

            case (TYPE_MICRON_L95) :
            {
                if(aID[0] == 0x54e5a42c && aID[1] == 0x000000A9)
                    bsEql = TRUE;
            }
            break;

            case (TYPE_TSB_TLC) :
            {
                if(aID[0] == 0xa3993c98  && aID[1] == 0x1408517a)
                    bsEql = TRUE;
            }
            break;

            case (TYPE_TSB_A19_BGA) :
            {
                if(aID[0] == 0x93a43a98 && aID[1] == 0x0408d07a)
                    bsEql = TRUE;
            }
            break;

            case (TYPE_TSB_A19_2LUN) :
            {
                if(aID[0] == 0x93a53c98 && aID[1] == 0x0408d07e)
                    bsEql = TRUE;
            }
            break;
            
            case (TYPE_TSB_A15_2LUN) :
            {
                if(aID[0] == 0x93953c98 && aID[1] == 0x0408d17a)
                    bsEql = TRUE;
            }
            break;
            
            case (TYPE_TSB_A15_TLC) :
            {
                if(aID[0] == 0xa3983a98 && aID[1] == 0x14085176)
                    bsEql = TRUE;
            }
            break;

            default : //L85
            {   
                DBG_Printf("Wrong Flash Type!\n");
                DBG_Getch();
            }
           }//end of switch
       #endif
          
        if(FALSE == bsEql)
        {
            //DBG_Printf("PU %d Read ID wrong!\n",tFlashAddr.ucPU);
            DBG_Printf("PU %d ID0:0x%x ID1:0x%x\n",tFlashAddr.ucPU, aID[0], aID[1]);
            //NFC_GETCH;
        }
        else
        {
            DBG_Printf("PU %d Read ID Right!\nID0:0x%x ID1:0x%x\n",tFlashAddr.ucPU,aID[0],aID[1]);
        }
    }
    return;
}


/*------------------------------------------------------------------------------
Name: TEST_NfcGetFeature
Description:
    get flash feature
Input Param:
    U8 ucPU : PU num;
    U8 ucAddr: feature addr
Output Param:
    none
Return Value:
    none
Usage:
    setting like read ID
History:
    20151110    abby    create
------------------------------------------------------------------------------*/
BOOL MCU2_DRAM_TEXT TEST_NfcGetFeature(FLASH_ADDR *pFlashAddr, U8 ucAddr)
{
    NFCQ_ENTRY *pNFCQEntry;
    U8  ucPU, ucLun;

    ucPU = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;

    if (FALSE == HAL_NfcIsLunAccessable(ucPU, ucLun))
    {
        return NFC_STATUS_FAIL;
    }

    pNFCQEntry = HAL_NfcGetNfcqEntryAddr(ucPU, ucLun);
    COM_MemZero((U32*)pNFCQEntry, sizeof(NFCQ_ENTRY)>>2);

    pNFCQEntry->bsDmaByteEn = TRUE;
    pNFCQEntry->aByteAddr.usByteAddr = ucAddr;
    pNFCQEntry->aByteAddr.usByteLength = sizeof(U32);
    pNFCQEntry->bsPrcqStartDw = g_aPrcqTable[NF_PRCQ_GETFEATURE].bsPRCQStartDw;

    HAL_NfcCmdTrigger(pFlashAddr, NF_PRCQ_GETFEATURE, FALSE, INVALID_2F);

    return NFC_STATUS_SUCCESS;
}

#if (defined(NVDDR2) || defined(NVDDR)) 
LOCAL BOOL TEST_NfcSetAllFlashFeature(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U8 ucAddr, ucData;

    //Feature Addresses 10h and 80h: Output Drive Strength
    ucAddr = 0x10;
    ucData = 0x3;
    for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
    }
    for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr%d Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
            NFC_GETCH;
            return FAIL;
        }
    }
    ucAddr = 0x80;
    ucData = 0x3;
    for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ucData, ucAddr);
    }
    for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d Addr%d Data%d\n", tFlashAddr.ucPU, ucAddr, ucData);
            NFC_GETCH;
            return FAIL;
        }
    }

    return SUCCESS;
}
#else// TSB Flash
LOCAL BOOL TEST_NfcSetAllFlashFeature(void)
{
    FLASH_ADDR tFlashAddr = {0};

    //Feature Addresses 10h: Output Drive Strength
    for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, 0x06, 0x10);
    }
    for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
    {
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("set feature fail PU:%d\n", tFlashAddr.ucPU);
            NFC_GETCH;
            return FAIL;
        }
    }  

    return SUCCESS;
}
#endif

/****************************************************************************
Function  : TEST_NfcNormalSetFeature
Input     :
Output    :

Purpose   : to test set feature function for all PU
Reference :
****************************************************************************/
LOCAL void TEST_NfcNormalSetFeature(void)
{
    BOOL bsStatus;
    bsStatus = TEST_NfcSetAllFlashFeature();

    if (SUCCESS != bsStatus)
    {
         DBG_Printf("Set all flash feature fail!\n");
    }
}

/****************************************************************************
Function  : TEST_NfcPioSetFeature
Input     :
Output    :

Purpose   : to test PIO set feature function for all PU
Reference :
****************************************************************************/
LOCAL void TEST_NfcPioSetFeature(void)
{
    FLASH_ADDR tFlashAddr = {0};
    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        HAL_NfcPioSetFeature(tFlashAddr.ucPU, 0, 0x2, 0x10);
        if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
        {
            DBG_Printf("PU %d PIO set feature fail! ADDR 0x%x Value 0x%x\n",tFlashAddr.ucPU,0x10,0x2);
            NFC_GETCH;
        }
    }
    return;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcNormalGetFeature
Description:
   Set feature and Get feature by call get flash ID
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    FW call this function to test get feature function.
History:
    20151126    abby    create
------------------------------------------------------------------------------*/
LOCAL void TEST_NfcNormalGetFeature(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 aFeature[2] = {0, 0};
    U32 ulData = 0x06;
    U8  ucAddr = 0x10;
    U32 ulStatus;
    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        /*    Set feature     */
        HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("PU %d set feature fail! ADDR 0x%x Value 0x%x\n",tFlashAddr.ucPU,ucAddr,ulData);
        }
        DBG_Printf("PU %d Set feature Addr 0x%x, Data 0x%x\n",tFlashAddr.ucPU, ucAddr, ulData);

        /*    Get feature     */
        ulStatus = TEST_NfcGetFeature(&tFlashAddr, ucAddr);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("Get PU %d feature Fail!\nAddr:0x%x,Value:0x%x\n",tFlashAddr.ucPU,ucAddr,ulData);
        }
        ulStatus = HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        if (NFC_STATUS_SUCCESS != ulStatus)
        {
            DBG_Printf("GetFeature Fail!\n");
        }
        else
        {
            HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
            //DBG_Printf("GetFeature 0x%x!\n", aFeature[0]);
        }

    }
    return;
}

/****************************************************************************
Function  : TEST_GetOffset
Input     :
Output    :

Purpose   : Get SSU and the cache status address offset
Reference :
****************************************************************************/
LOCAL U32 TEST_GetOffset(FLASH_ADDR *pFlashAddr)
{
    if(!g_bSsuEn)
    {
        return 0;
    }
    
    //MixVector only support 512 offset for SSUx in one MCU
    return GET_CMD_LEVEL(pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->bsPln
                              ,HAL_NfcGetRP(pFlashAddr->ucPU, pFlashAddr->ucLun));
}

/****************************************************************************
Function  : TEST_GetCacheStatus
Input     :
Output    :

Purpose   : Get the cache status
Reference :
****************************************************************************/
LOCAL U8 TEST_GetCacheStatus(U32 ulCacheAddr)
{
   return *((volatile U8 *)(g_ulCacheStatusAddr + ulCacheAddr));
}
/****************************************************************************
Function  : SetCacheStatus
Input     :
Output    :

Purpose   : set the cache status
Reference :
****************************************************************************/
LOCAL void TEST_SetCacheStatus(U32 ulCacheAddr, U8 ucValue)
{

   *((volatile U8*)(g_ulCacheStatusAddr + ulCacheAddr)) = ucValue;

   return;
}

/****************************************************************************
Function  : TEST_GetSsu0Status
Input     :
Output    :

Purpose   : get the Ssu status
Reference :
****************************************************************************/
LOCAL U8 TEST_GetSsu0Status(U16 usSsuAddr)
{
    U8  ucValue;
    U32 ulBaseAddr;

    if (FALSE != g_bSsu0DramEn)
    {
        ulBaseAddr = g_ulSsuInDramBase;
    }
    else
    {
        ulBaseAddr = g_ulSsuInOtfbBase;
    }

    ucValue = *((volatile U8 *)(ulBaseAddr + usSsuAddr));

    return ucValue;
}
/****************************************************************************
Function  : TEST_SetSsu0Status
Input     :
Output    :

Purpose   : set the ssu status
Reference :
****************************************************************************/
LOCAL void TEST_SetSsu0Status(U16 usSsuAddr, U8 ucValue)
{
    U32 ulBaseAddr;

    if (FALSE != g_bSsu0DramEn)
    {
        ulBaseAddr = g_ulSsuInDramBase;
    }
    else
    {
        ulBaseAddr = g_ulSsuInOtfbBase;
    }

    *((volatile U8 *)(ulBaseAddr + usSsuAddr)) = ucValue;

    return;
}

/****************************************************************************
Function  : TEST_GetSsu1Status
Input     :
Output    :

Purpose   : get the Ssu status
Reference :
****************************************************************************/
LOCAL U8 TEST_GetSsu1Status(U16 usSsuAddr)
{
    U8  ucValue;
    U32 ulBaseAddr;

    if (FALSE != g_bSsu1DramEn)
    {
        ulBaseAddr = g_ulSsu1DramBase;
    }
    else
    {
        ulBaseAddr = g_ulSsu1OtfbBase;
    }

    ucValue = *((volatile U8 *)(ulBaseAddr + usSsuAddr));

    return ucValue;
}

/****************************************************************************
Function  : TEST_SetSsu1Status
Input     :
Output    :

Purpose   : set the ssu 1 status
Reference :
****************************************************************************/
LOCAL void TEST_SetSsu1Status(U16 usSsuAddr, U8 ucValue)
{
    U32 ulBaseAddr;

    if (FALSE != g_bSsu1DramEn)
    {
        ulBaseAddr = g_ulSsu1DramBase;
    }
    else
    {
        ulBaseAddr = g_ulSsu1OtfbBase;
    }

    *((volatile U8 *)(ulBaseAddr + usSsuAddr)) = ucValue;

    return;
}

/****************************************************************************
Function  : TEST_WaitSSU
Input     :
Output    :

Purpose   : wait HW flip SSU data and FW clear it after
Reference :
****************************************************************************/
LOCAL BOOL TEST_WaitSSU(FLASH_ADDR *pFlashAddr, U16 usAddrOffset)
{
    while (0 != TEST_GetSsu0Status(usAddrOffset))
    {
        ;
    }
    TEST_SetSsu0Status(usAddrOffset, TEST_SSU0_DATA);

    while (0 != TEST_GetSsu1Status(usAddrOffset))
    {
        ;
    }
    TEST_SetSsu1Status(usAddrOffset,TEST_SSU1_DATA);

    return NFC_STATUS_SUCCESS;
}

/****************************************************************************
Function  : TEST_WaitCacheStatus
Input     :
Output    :

Purpose   : wait HW flip Cache Status and FW clear it after
Reference :
****************************************************************************/
LOCAL BOOL TEST_WaitCacheStatus(FLASH_ADDR *pFlashAddr, U16 usAddrOffset)
{
    while (0 != TEST_GetCacheStatus(usAddrOffset))
    {
        ;
    }
    TEST_SetCacheStatus(usAddrOffset, TEST_CACHE_STS_DATA);

    return NFC_STATUS_SUCCESS;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcGetWrBufId
Description:
    Get write data buffe ID
History:
    20141126    abby    add header, unify to meet coding style
------------------------------------------------------------------------------*/
LOCAL U16 TEST_NfcGetWrBufId(FLASH_ADDR* pFlashAddr)
{
    U16 usWrBufId;

    //usWrBufId = START_WBUF_ID + ucPageInWl * TEST_GET_ADDR_OFF(pFlashAddr->ucPU, pFlashAddr->ucLun, pFlashAddr->usPage);
    usWrBufId = START_WBUF_ID;
    
    return usWrBufId;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcGetRdBufId
Description:
    Get read data buffe ID
    Different PU, LUN, Page have different read buffer
History:
    20141126    abby    add header, unify to meet coding style
------------------------------------------------------------------------------*/
LOCAL U16 TEST_NfcGetRdBufId(FLASH_ADDR* pFlashAddr, U8 ucPageType)
{
    U16 usRdBufId;
    //usRdBufId = START_RBUF_ID + TEST_GET_ADDR_OFF(pFlashAddr->ucPU, pFlashAddr->ucLun, pFlashAddr->usPage)
    //            * ucPageInWl + ucPageType;
    usRdBufId = START_RBUF_ID + pFlashAddr->ucPU;
    
    //DBG_Printf("Rd buf ID:0x%x\n",usRdBufId);
    return usRdBufId;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcGetRedOffset
Description:
    Get red data address offset
    Different PU, LUN, CQ level have different red offset
History:
    20141126    abby    add header, unify to meet coding style
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcGetRedOffset(FLASH_ADDR *pFlashAddr)
{
    U32 ulRedOffset = 0;
    U8  ulWp = HAL_NfcGetWP(pFlashAddr->ucPU, pFlashAddr->ucLun);
    U32 ulAddrOff = TEST_GET_ADDR_OFF(pFlashAddr->ucPU, pFlashAddr->ucLun, pFlashAddr->usPage);

    ulRedOffset = (ulAddrOff << NFCQ_DEPTH_BIT) + ulWp;
    
#ifdef DRAMLESS_ENABLE
    ulRedOffset = pFlashAddr->ucPU + ulWp;
#endif

    ulRedOffset = pFlashAddr->ucPU + ulWp;
    //DBG_Printf("ulRedOffset :0x%x\n",ulRedOffset);
    return ulRedOffset;
}

/****************************************************************************
Function      : TEST_NfcCalcuData
Input         : U32 ulPU, U32 ulLun, U32 ulPage, U32 ulSec
                cast input param to U32 for shifting operation
Output        :
                none
return        :
                U32 ulResult: data value
Description   : calculate data value bonding to flash addr
History:
    20141126    abby    create
****************************************************************************/
U32 TEST_NfcCalcuData(U32 ulPU, U32 ulPage, U32 ulSec)
{
    U32 ulResult;
    ulResult = ((((ulPU << 8) | ulPage) << 8) | ulSec) << 8;
    return ulResult;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcGetReadLba
Description:
    Calculate Read LBA value for NFC LBA Check
Input Param:
Output Param:
    none
Return Value:
Usage:
History:
20151105   abby    create
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcGetReadLba(FLASH_ADDR *pFlashAddr, U8 ucSecStart)
{
    U32 ulLba;
    
    /*  Maple modify for EM test  */
    if (TRUE == g_bEmEnable)
    {
        ulLba = ucSecStart;
    }
    else
    {
        ulLba = TEST_NfcCalcuData(pFlashAddr->ucPU,pFlashAddr->usPage,ucSecStart);
    }
    
    return (ulLba >> 3) & MULTI_VALUE_1(29);
}

/****************************************************************************
Function      : TEST_NfcPrepare1PlnData
Input         :
Output        :

Description   : prepara dummy data by 1 plane page size
                One write buff size = PHYPG_SZ * PLN_PER_LUN * PGADDR_PER_PRG * INTRPG_PER_PGADDR
Reference     :
****************************************************************************/
LOCAL void TEST_NfcPrepare1PlnData(FLASH_ADDR *pFlashAddr, U8 ucPageCnt, U8 ucInterPgCnt)
{
    U32 ulDramAddr, ulData, ulDwIndex;
    U16 usWrBufId, usSec;
    U32 ulSecInBuf, ulPgSzBits, ulSecPerPg;

    usWrBufId  = TEST_NfcGetWrBufId(pFlashAddr);
    ulSecPerPg = g_bSinglePln ? SEC_PER_PHYPG : SEC_PER_LOGIC_PIPE_PG;
    ulSecInBuf = (SEC_PER_PHYPG * pFlashAddr->bsPln) + ((ucPageCnt + ucInterPgCnt)* ulSecPerPg);
    ulPgSzBits = g_bSinglePln ? LOGIC_PG_SZ_BITS : LOGIC_PIPE_PG_SZ_BITS;
    
    ulDramAddr = (HAL_NfcGetDmaAddr(usWrBufId, ulSecInBuf, ulPgSzBits) << 1) + TEST_START_ADDRESS;
    //DBG_Printf("Page %d PLN%d ucInterPgCnt%d ulSecInBuf%d Prepara data W BufId:0x%x W Addr:0x%x\n"
    //        ,pFlashAddr->usPage,pFlashAddr->bsPln,ucInterPgCnt,ulSecInBuf,usWrBufId,ulDramAddr);
    
    for (usSec = 0; usSec < SEC_PER_PHYPG; usSec++)
    {        
        for (ulDwIndex = 0; ulDwIndex < SEC_SZ_DW; ulDwIndex++)
        {
            ulData = TEST_NfcCalcuData(((pFlashAddr->ucLun << 6) + pFlashAddr->ucPU)
            , pFlashAddr->usPage, (usSec + ucInterPgCnt * ulSecPerPg))+ulDwIndex;
            
        #ifdef SIM
            if (g_bDecFifoEn)// write data 0, to check err bit 0->1 cnt
            {
                ulData = 0;
            }
        #endif
            *(volatile U32*)(ulDramAddr + (usSec << SEC_SZ_BITS) + ulDwIndex * 4) = ulData;
        }
    }

    return;
}


/****************************************************************************
Function      : TEST_NfcPrepareData
Input         :
Output        :

Description   : Prepare data for once program to DRAM
                
Reference     :
****************************************************************************/
LOCAL void TEST_NfcPrepareData(FLASH_ADDR *pFlashAddr)
{
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    U8 ucPageCnt;       //page number of different page address in once program
    U8 ucInterPgCnt;    //internal page number of the same page addr
    U8 ucPlnNum;

    ucPlnNum = g_bSinglePln ? 1 : PLN_PER_LUN;

    for (ucPageCnt = 0; ucPageCnt < PGADDR_PER_PRG; ucPageCnt++)
    {
        for (ucInterPgCnt = 0; ucInterPgCnt < INTRPG_PER_PGADDR; ucInterPgCnt++)
        {
            for (tFlashAddr.bsPln = 0; tFlashAddr.bsPln < ucPlnNum; tFlashAddr.bsPln++)
            {
                TEST_NfcPrepare1PlnData(&tFlashAddr, ucPageCnt, ucInterPgCnt);
            }
        }
    #ifdef FLASH_INTEL_3DTLC
        if(EXTRA_PAGE == HAL_GetFlashPairPageType(tFlashAddr.usPage))
        {
            tFlashAddr.usPage = HAL_GetHighPageIndexfromExtra(tFlashAddr.usPage);
        }
        else if (LOW_PAGE_WITHOUT_HIGH != HAL_GetFlashPairPageType(tFlashAddr.usPage))
        {
            tFlashAddr.usPage++;
        }
    #else
        tFlashAddr.usPage++;//whatever LP only or 2 shared pages, prepare all data for simplicity
    #endif
    }
    
    return;
}

/****************************************************************************
Function      : TEST_NfcCheck1PlnData
Input         :
Output        :

Description   : one read buff size = PHYPG_SZ * PLN_PER_LUN 
Reference     :
****************************************************************************/
LOCAL void TEST_NfcCheck1PlnData(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U16 usSecEnd, U16 usRdBufId, U8 ucInterPgCnt)
{
    U32 ulRDramAddr, ulWDramAddr;
    U32 ulWrData, ulRdData;
    U16 usWrBufId, usSec;
    U32 ulDwErrCount = 0;
    U32 ulSecInBuf, ulPgSzBits, ulSecPerPg;
    U32 ulDwIndex, ulDwNum;
    BOOL bsErr = FALSE;

#ifdef DATA_CHECK_2DW   //for SIM/COSIM/read stress test
    ulDwNum = 2;
#else
    ulDwNum = SEC_SZ_DW;
#endif
    
    usWrBufId  = TEST_NfcGetWrBufId(pFlashAddr);
    ulSecPerPg = g_bSinglePln ? SEC_PER_PHYPG : SEC_PER_PIPE_PG;

    if (g_bRawDataRead)
    {
        ulSecInBuf = SEC_PER_PHYPG * pFlashAddr->bsPln * 2;
        ulPgSzBits = g_bSinglePln ? PHYPG_SZ_BITS : LOGIC_PIPE_PG_SZ_BITS;
    }
    else
    {
        ulSecInBuf = SEC_PER_PHYPG * pFlashAddr->bsPln;
        ulPgSzBits = g_bSinglePln ? LOGIC_PG_SZ_BITS : LOGIC_PIPE_PG_SZ_BITS;
    }
   
    ulWDramAddr = (HAL_NfcGetDmaAddr(usWrBufId, 0, ulPgSzBits) << 1) + TEST_START_ADDRESS;
    ulRDramAddr = (HAL_NfcGetDmaAddr(usRdBufId, ulSecInBuf, ulPgSzBits) << 1) + TEST_START_ADDRESS;
    //DBG_Printf("Check data:RDramBaseAddr0x%x ulSecInBuf%d ucInterPgCnt%d\n",ulRDramAddr,ulSecInBuf,ucInterPgCnt);
    
    for (usSec = ucSecStart; usSec <= usSecEnd; usSec++)
    {
        if ((0 == usSec % 2)&&(usSec != 0) && g_bRawDataRead)
        {
            ulRDramAddr = ulRDramAddr + CW_INFO_SZ;
            //DBG_Printf("Sec Num:%d ulRDramAddr:0x%x\n",usSec,ulRDramAddr);
        }
        for (ulDwIndex = 0; ulDwIndex < ulDwNum; ulDwIndex++)
        {
            ulWrData = TEST_NfcCalcuData(((pFlashAddr->ucLun << 6) + pFlashAddr->ucPU)
            ,pFlashAddr->usPage,(usSec + ucInterPgCnt * ulSecPerPg))+ ulDwIndex;

        #ifdef SIM
            if (g_bDecFifoEn)// write data 0, to check err bit 0->1 cnt
            {
                ulWrData = 0;
            }
        #endif
        
            ulRdData = *(volatile U32*)(ulRDramAddr + (usSec << SEC_SZ_BITS) + ulDwIndex * 4);
            if( ulWrData != ulRdData )
            {
            #if 1
                DBG_Printf("PU %d LUN%d Block %d Page %d Data Miss-compare!\n",pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usBlock,pFlashAddr->usPage);
                DBG_Printf("Rdaddr:0x%x WtBuffStartAddr:0x%x ulsec:0x%x Rdata:0x%x Wdata:0x%x\n"
                ,ulRDramAddr + (usSec<<SEC_SZ_BITS)+ ulDwIndex*4,ulWDramAddr,usSec,ulRdData,ulWrData);
            #endif
                ulDwErrCount++;
                bsErr = TRUE;
                DBG_Getch();
            }
            else
            {
                *(U32*)(ulRDramAddr + (usSec<<SEC_SZ_BITS) + ulDwIndex * 4) = 0x0;
            }
        }
    }
    if (bsErr)
    {
        DBG_Printf("PU %d Block %d Page %d Error DW Count %d\n",pFlashAddr->ucPU,pFlashAddr->usBlock,pFlashAddr->usPage,ulDwErrCount);
        DBG_Getch();
    }
    return;
}

/****************************************************************************
Function      : TEST_NfcCheckData
Input         : ucPageCnt:    page address of once program
                              for 1-pass program but multi page addr
                ucInterPgCnt: internal page number of the same page addr
                              Only for TSB 2D TLC: 0/1/2, equal to page type
Output        :

Description   : For FPGA or ASIC, check every DW of a page
                verify read data according to write buffer
Reference     :
****************************************************************************/
LOCAL void TEST_NfcCheckData(FLASH_ADDR *pFlashAddr, U8 ucSecStart, U16 usSecLen, U16 usRdBufId, U8 ucInterPgCnt)
{
#ifndef DATA_CHK
    return;
#endif

    FLASH_ADDR tFlashAddr = *pFlashAddr;

    U8 ucPlnStart, ucPlnEnd;
    U16 usSecStartInPln, usSecEndInPln; 

    ucPlnStart  = ucSecStart / SEC_PER_PHYPG;
    ucPlnEnd    = (ucSecStart + usSecLen - 1) / SEC_PER_PHYPG;

    //ucInterPgCnt = (1 == INTRPG_PER_PGADDR)? 0 : ucInterPgCnt;
    for (tFlashAddr.bsPln = ucPlnStart; tFlashAddr.bsPln <= ucPlnEnd; tFlashAddr.bsPln++)
    {
        usSecStartInPln = 0;
        usSecEndInPln   = SEC_PER_PHYPG - 1;
        
        if (ucPlnStart == tFlashAddr.bsPln)
        {
            usSecStartInPln = ucSecStart % SEC_PER_PHYPG;  // SecStartInPlnStart
        }
        if (ucPlnEnd == tFlashAddr.bsPln)
        {
            usSecEndInPln = (ucSecStart + usSecLen - 1) % SEC_PER_PHYPG;
        }
        //DBG_Printf("Page%d ucInterPgCnt%d ucPlnStart%d ucPlnEnd%d\n",pFlashAddr->usPage,ucInterPgCnt,ucPlnStart,ucPlnEnd);
        TEST_NfcCheck1PlnData(&tFlashAddr,(U8)usSecStartInPln,usSecEndInPln,usRdBufId,ucInterPgCnt);
    }

    return;       
}

/****************************************************************************
Function      : TEST_NfcPrepare1PlnRed
Input         :
Output        :

Description   : prepare RED dummy data and LPN for 1 plane
Reference     :
****************************************************************************/
LOCAL void TEST_NfcPrepare1PlnRed(U32 *pRed, FLASH_ADDR *pFlashAddr)
{
    U32 ulDwIndex;
    U8  ucLPNIndex;
    
    SPARE_AREA *pSpare;
    pSpare = (SPARE_AREA *)pRed;

    /*    init RED dummy data    */
    for (ulDwIndex = 0; ulDwIndex < RED_SZ_DW; ulDwIndex++)
    {
        *(pRed + ulDwIndex) =  TEST_RED_DATA;
    }
   
    /*    init LPN, for single pln write, just init 4 LPN for 16KB page;    */
    for(ucLPNIndex = 0; ucLPNIndex < LPN_PER_PLN; ucLPNIndex++)
    {
        /*  Maple modify for EM test  */
        if (TRUE == g_bEmEnable)
        {
            pSpare->aCurrLPN[ucLPNIndex] = ucLPNIndex * 8 + (pFlashAddr->bsPln) * 32;
        }
        else
        {
            pSpare->aCurrLPN[ucLPNIndex] = TEST_NfcCalcuData(0, 0, ucLPNIndex) * 8;
            //DBG_Printf("ucPlnNum %d pSpare->aCurrLPN[%d]=0x%x\n",ucPlnNum,ucLPNIndex,pSpare->aCurrLPN[ucLPNIndex]);
        }
    }
    
    pSpare->bcPageType = PAGE_TYPE_DATA;
    
    return;
}


/****************************************************************************
Function      : TEST_NfcPrepareRed
Input         :
Output        :

Description   : write red data to array aContent
Reference     :
****************************************************************************/
LOCAL void TEST_NfcPrepareRed(U32 *pRed, FLASH_ADDR *pFlashAddr)
{
    FLASH_ADDR tFlashAddr = *pFlashAddr;
    U8 ucPlnNum, ucPgNum, ucPgIdx;

    ucPlnNum = g_bSinglePln ? 1 : PLN_PER_LUN;
    ucPgNum = INTRPG_PER_PGADDR * PGADDR_PER_PRG;
    
    for (ucPgIdx = 0; ucPgIdx < ucPgNum; ucPgIdx++)
    {
        for (tFlashAddr.bsPln = 0; tFlashAddr.bsPln < ucPlnNum; tFlashAddr.bsPln++)
        {
            TEST_NfcPrepare1PlnRed(pRed, &tFlashAddr);
            pRed += RED_SZ_DW;
        } 
    }

    return;
}

/****************************************************************************
Function      : TEST_NfcCheck1PlnRed
Input         :
Output        :

Description   : prepare RED dummy data and LPN for 1 plane
Reference     :
****************************************************************************/
LOCAL void TEST_NfcCheck1PlnRed(U32 *pRed, U16 usSecStart, U16 usSecLen, U8 ucPageType, U8 ucPln)
{
    U32 ulDwIndex, ulLpn;
    U8  ucLPNIndex;
    volatile SPARE_AREA *pSpare = (volatile SPARE_AREA *)pRed;
   
    //DBG_Printf("*pRed Addr 0x%x usSecStart%d usSecLen%d ucPln%d\n", (U32)pRed, usSecStart, usSecLen, ucPln);
    
    if (PAGE_TYPE_DATA != pSpare->bcPageType)
    {
        DBG_Printf("PageType %d is wrong, should be %d\n", pSpare->bcPageType, PAGE_TYPE_DATA);
    }
    else
    {
        pSpare->bcPageType = 0x55;
    }

    /*    check LPN, for single pln write, just init 4 LPN for 16KB page;    */
    for(ucLPNIndex = 0; ucLPNIndex < LPN_PER_PLN; ucLPNIndex++)
    {
        if (TRUE == g_bEmEnable)
        {
            ulLpn = (ucLPNIndex + (ucPln << 2)) << 3;
        }
        else
        {
            ulLpn = TEST_NfcCalcuData(0, 0, ucLPNIndex) << 3;
        }
        
        if (ulLpn != pSpare->aCurrLPN[ucLPNIndex])
        {
            DBG_Printf("m_DataRed.aCurrLPN[%d] = 0x%x is wrong, should be 0x%x\n", ucLPNIndex, pSpare->aCurrLPN[ucLPNIndex], ulLpn);
        }
        else
        {
            pSpare->aCurrLPN[ucLPNIndex] = TEST_RED_DATA;
        }
    }
    
    /*    check RED dummy data    */
    for (ulDwIndex = 0; ulDwIndex < RED_SZ_DW; ulDwIndex++)
    {
        if (TEST_RED_DATA != *(pRed + ulDwIndex))
        {
            DBG_Printf("pRed->aContent[%d] = 0x%x is wrong, should be 0x%x, *pRed Addr 0x%x\n",ulDwIndex,*(pRed + ulDwIndex),TEST_RED_DATA,(U32)pRed);
        }
        else
        {
            *((U32*)pRed + ulDwIndex) = 0;
        }
    }
   
    return;
}

/****************************************************************************
Function      : TEST_NfcCheckRed
Input         :
Output        :

Description    : verify red data, if it is right, then clr it.
Reference     :
****************************************************************************/
LOCAL void TEST_NfcCheckRed(FLASH_ADDR *pFlashAddr,NFC_RED **pRed, U16 usSecStart, U16 usSecLen, U8 ucPageType)
{
#ifndef DATA_CHK
    return;
#endif
    U32 ucPlnStart, ucPlnEnd, ucPlnLen, ucPlnIndex;
    U32 *pRdRed = (U32*)(*pRed);

    ucPlnStart  = usSecStart / SEC_PER_PHYPG;
    ucPlnEnd    = (usSecStart + usSecLen - 1) / SEC_PER_PHYPG;
    ucPlnLen    = usSecLen / SEC_PER_PHYPG;

    if (0 == ucPlnLen) //no RED be read
    {
        return;
    }
    pRdRed += ucPlnStart * RED_SZ_DW;
    
    for (ucPlnIndex = ucPlnStart; ucPlnIndex <= ucPlnEnd; ucPlnIndex++)
    {
        TEST_NfcCheck1PlnRed(pRdRed, usSecStart, usSecLen, ucPageType, ucPlnIndex);
        pRdRed += RED_SZ_DW;
        //DBG_Printf("ucPlnIndex%d\n",ucPlnIndex);
    }
    
    return;
}

LOCAL void TEST_NfcWtCfgReqComm(NFC_PRG_REQ_DES *pWrReq)
{
    pWrReq->bsWrBuffId      = START_WBUF_ID;
    pWrReq->bsRedOntf       = g_bRedOntf;
    pWrReq->bsCSEn          = g_bCacheStsEn;
    pWrReq->bsSsu0En        = g_bSsuEn;
    pWrReq->bsSsu0Ontf      = !g_bSsu0DramEn;
    pWrReq->bsSsu1En        = g_bSsuEn;
    pWrReq->bsSsu1Ontf      = !g_bSsu0DramEn;
    pWrReq->bsLbaChkEn      = g_bLbaChk;
    pWrReq->bsTlcMode       = g_bTlcMode;
    pWrReq->pNfcRed         = (NFC_RED *)pWrRed;
    pWrReq->bsEmEn          = g_bEmEnable;

    if (g_ErrInjEn)
    {
        pWrReq->bsInjErrEn = TRUE;
        pWrReq->pErrInj    = &g_tErrInj;
    }
    else
    {
        pWrReq->pErrInj = NULL;
    }

    /*  scramble disable    */
    if (g_bRawDataRead)
    {
        volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
        pNfcPgCfg->bsScrBps = TRUE;
    }
}

/****************************************************************************
Function      : TEST_NfcWtGetPageAddr
Input         : 
Output        :
Description   : 
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL U16 TEST_NfcWtGetPageAddr(U16 *pPrgIndex)
{
    U16 usPage = *pPrgIndex;

    if (g_bTlcMode)
    {
    #ifdef FLASH_TLC
        usPage = HAL_FlashGetTlcPrgWL(usPage);
    #endif
    
    #ifdef FLASH_3D_MLC
        if (EVEN != usPage % 2)
        {
            //DBG_Printf("1-pass page prg mode, program can't from odd page %d!\n",usPage);
            usPage++;
            *pPrgIndex = usPage;
        }
    #endif

    #if (defined(FLASH_INTEL_3DTLC) || defined(FLASH_MICRON_3DTLC_B16))
        if (HIGH_PAGE == HAL_GetFlashPairPageType(usPage))
        {
            //DBG_Printf("program can't from high page %d!\n",usPage);
            usPage++;
            *pPrgIndex = usPage;
        }
    #endif
    }
    return usPage;
}

/****************************************************************************
Function      : TEST_NfcWtGetPrgCycle
Input         : 
Output        :
Description   : 
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL U8 TEST_NfcWtGetPrgCycle(U16 usPrgIndex)
{
    U8 ucPrgCycle = 0;

    if (g_bTlcMode)
    {
        ucPrgCycle = HAL_FlashGetTlcPrgCycle(usPrgIndex);
    }
    return ucPrgCycle;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcRdGetDsIndex
Description:
    Get data syntax index
Input Param:
Output Param:
    none
Return Value:
Usage:
History:
20160810   abby    create
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcWtGetDsIndex(FLASH_ADDR *pFlashAddr, WRITE_REQ_TYPE WriteType)
{
    U8 ucDsIndex = DS_ENTRY_SEL;

    if (SING_PLN_WRITE != WriteType)//only support single plane to mix data syntax
    {
        return ucDsIndex;
    }
    
#ifdef DATA_SYNTAX_MIX
    ucDsIndex = g_bSinglePln ? (pFlashAddr->usPage % 8) : DS_ENTRY_SEL;
#endif

    return ucDsIndex;
}

/****************************************************************************
Function      : TEST_NfcWtStsChk
Input         : 
Output        :
Description   : 
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL BOOL TEST_NfcWtStsChk(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq)
{
    FLASH_STATUS_ENTRY tFlashSts = {0};

    /*    check SSU and Cache Status update    */
    if(g_bSsuEn)
    {
        TEST_WaitSSU(pFlashAddr, pWrReq->bsSsu0Addr);
    }
    if(g_bCacheStsEn)
    {
        TEST_WaitCacheStatus(pFlashAddr, pWrReq->bsSsu0Addr);
    }

    if (g_ErrInjEn)
    {
        HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun);

        HAL_DecSramGetFlashSts(pFlashAddr, (FLASH_STATUS_ENTRY*)&tFlashSts);
        if (g_tErrInj.bsInjErrSts != tFlashSts.bsFlashStatus)
        {
            DBG_Printf("PU%d LUN%d Page%d Write Inject Error Flash Status:0x%x Actual Flash Status:0x%x\n"
            ,pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usPage,g_tErrInj.bsInjErrSts, tFlashSts.bsFlashStatus);
        }
        else
        {
            //DBG_Printf("PU%d LUN%d Inject Flash read status value OK: 0x%x\n", pFlashAddr->ucPU, pFlashAddr->ucLun, g_tErrInj.bsInjErrSts);
        }
        HAL_NfcResetCmdQue(pFlashAddr->ucPU, pFlashAddr->ucLun);
        HAL_NfcClearINTSts(pFlashAddr->ucPU, pFlashAddr->ucLun);
    }

#ifdef DATA_CHK
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        DBG_Printf("Pu %d LUN%d Block %d Page %d Write Fail!\n",pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usBlock,pFlashAddr->usPage);
        return FAIL;
    }
    else
    {
        if (g_bTlcMode)
        {
            //DBG_Printf("Pu %d Block %d WL %d TLC Write Time %d OK!\n",pFlashAddr->ucPU,pFlashAddr->usBlock
            //         ,pFlashAddr->usPage,pWrReq->bsTlcPgCycle);
        }
        else
        {
            //DBG_Printf("Pu %d Block %d Page %d Write OK!\n",pFlashAddr->ucPU,pFlashAddr->usBlock,pFlashAddr->usPage);
        }
    }
#endif

    return SUCCESS;
 
}

/****************************************************************************
Function      : TEST_NfcRdDecStsChk
Input         : 
Output        :
Description   : 
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL void TEST_NfcRdDecStsChk(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    DEC_SRAM_STATUS_ENTRY tDecSramSts = {0};
    DEC_FIFO_STATUS_ENTRY tDecFifoSts = {0};
    NFC_ERR_INJ tErrInj = {0};
    
    NFCQ_ENTRY *pNfcqEntry = HAL_NfcGetNfcqEntryAddr(pFlashAddr->ucPU, pFlashAddr->ucLun);
    U8  ucErrCwStart, ucErrCwLen;
    U32 ulDecFailBitMap = 0, ulHardDecBitMap = 0, ulErrBitCnt = 0;

    tErrInj = *pRdReq->pErrInj;
    ucErrCwStart = tErrInj.bsInjCwStart;
    ucErrCwLen   = tErrInj.bsInjCwLen;
    ulErrBitCnt  = tErrInj.bsInjErrBitPerCw;

#ifdef DEC_FIFO_CHK        
    /* Read DEC FIFO status entry, check DEC FIFO hard+ bitmap and error cnt acc */
    HAL_DecFifoReadSts();   //Read status from DEC FIFO to FW table

    U8 ucCmdIndex = pRdReq->bsDecFifoIndex;
    tDecFifoSts = *HAL_DecFifoGetStsEntry(ucCmdIndex); //Read one Entries of DEC FIFO from SW Table

#ifdef DBG_SHOW_ALL
    DBG_Printf("ucCmdIndex %d DEC FIFO STS: \n", ucCmdIndex);
    DBG_Printf("ulDecEngBitMap=0x%x bsErrCntAcc0T1=0x%x bsErrCntAcc=0x%x\n", tDecFifoSts.ulDecEngBitMap, tDecFifoSts.bsErrCntAcc0T1, tDecFifoSts.bsErrCntAcc);
    DBG_Printf("bsLdpcDecItr=0x%x bsPlnNum=0x%x bsCmdIndex=0x%x\n\n", tDecFifoSts.bsLdpcDecItr, tDecFifoSts.bsPlnNum, tDecFifoSts.bsCmdIndex);
#endif

#ifdef HARD_PLUS_BITMAP_CHK
    ulHardDecBitMap = (((1<<(ucErrCwStart+ucErrCwLen+1))-1) - ((1<<ucErrCwStart)-1));
    if (tDecFifoSts.ulDecEngBitMap != ulHardDecBitMap)
    {
        DBG_Printf("Hard+ bitmap miss match!!!\nShould be 0x%x, but in DEC FIFO CmdIndex %d ulDecEngBitMap = 0x%x\n\n"
            , ulHardDecBitMap, ucCmdIndex, tDecFifoSts.ulDecEngBitMap);
        //DBG_Getch();
    }
#endif

#ifdef DEC_FIFO_ERRCNT_CHK
    U16 usInjErrCntAcc, usInjErrCntAcc0T1;
#ifdef SIM
    usInjErrCntAcc    = ucErrCwLen * 40;
    usInjErrCntAcc0T1 = ucErrCwLen * 20;
#else
    usInjErrCntAcc    = (ucErrCwLen + 1) * (ulErrBitCnt - 1);
    usInjErrCntAcc0T1 = usInjErrCntAcc;//PRG Data is all 0
#endif

    if (NF_ERR_TYPE_UECC != HAL_NfcGetErrCode(pFlashAddr->ucPU,pFlashAddr->ucLun))
    {
        U8 uDelta = (usInjErrCntAcc+1)/10;
        if ((usInjErrCntAcc - uDelta >= tDecFifoSts.bsErrCntAcc)|| (tDecFifoSts.bsErrCntAcc >= usInjErrCntAcc + uDelta))
        {
            DBG_Printf("Inj ErrCntAcc miss match!!!\nInjErrCntAcc should be %d, but in DEC FIFO EntryIndex %d ErrCntAcc = %d\n"
                , usInjErrCntAcc, ucCmdIndex, tDecFifoSts.bsErrCntAcc);
            //DBG_Getch();
        }
        #if 0
        if ((usInjErrCntAcc0T1 - uDelta >= tDecFifoSts.bsErrCntAcc0T1) || (tDecFifoSts.bsErrCntAcc0T1 >= usInjErrCntAcc0T1 + uDelta))
        {
            DBG_Printf("Inj ErrCntAcc 0T1 miss match!!!\nInjErrCntAcc0T1 should be %d, but in DEC FIFO Entry %d ErrCntAcc0T1 = %d\n"
                , usInjErrCntAcc0T1, ucCmdIndex, tDecFifoSts.bsErrCntAcc0T1);
            //DBG_Getch();
        }
        #endif
    }
#endif
#endif

#ifdef DEC_SRAM_CHK
    /* Read status from DEC SRAM, and check DEC SRAM decode fail bitmap */
    HAL_DecSramGetDecStsEntry(pFlashAddr, &tDecSramSts);
    
#ifdef DBG_SHOW_ALL
    DBG_Printf("PU%d LUN%d RP%d DEC SRAM STS: \n", pFlashAddr->ucPU, pFlashAddr->ucLun, HAL_NfcGetRP(pFlashAddr->ucPU,pFlashAddr->ucLun));
    DBG_Printf("ulDecFailBitMap=0x%x ulRCrcBitMap=0x%x bsN1Accu=0x%x\n", tDecSramSts.ulDecFailBitMap,tDecSramSts.ulRCrcBitMap,tDecSramSts.bsN1Accu);
    DBG_Printf("bsFstItrSyndAccu=0x%x bsErrCntAcc0T1=0x%x bsErrCntAcc=0x%x\n\n", tDecSramSts.bsFstItrSyndAccu,tDecSramSts.bsErrCntAcc0T1,tDecSramSts.bsErrCntAcc);
#endif

    ulDecFailBitMap = (((1 << (ucErrCwStart+ucErrCwLen+1))-1) - ((1<<ucErrCwStart)-1));
    if (tDecSramSts.ulDecFailBitMap != ulDecFailBitMap)
    {
        DBG_Printf("Page%d Dec fail bitmap miss match!!!\nShould be 0x%x, but in DEC SRAM tDecSramSts.ulDecFailBitMap = 0x%x\n"
            , pFlashAddr->usPage, ulDecFailBitMap, tDecSramSts.ulDecFailBitMap);
        //DBG_Printf("PU%d RP%d WP%d\n", pFlashAddr->ucPU, HAL_NfcGetRP(pFlashAddr->ucPU, pFlashAddr->ucLun), HAL_NfcGetWP(pFlashAddr->ucPU, pFlashAddr->ucLun));
    }
#endif
    return;
}

/****************************************************************************
Function      : TEST_NfcWtFlashStsChk
Input         : 
Output        :
Description   : check SSU & Cache status
                check NFC CMD RES STS
                check DEC FIFO & SRAM STS
                check red & data
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
BOOL TEST_NfcRdStsAndDataChk(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, READ_REQ_TYPE ReadType)
{
    /* SSU & Cache status check */
    if (g_bSsuEn)
    {
        /* When UECC happen, SSU will not be updated  */
        while (FALSE == HAL_NfcGetIdle(pFlashAddr->ucPU, pFlashAddr->ucLun))
        {
            ;
        }
        if (NF_ERR_TYPE_UECC == HAL_NfcGetErrCode(pFlashAddr->ucPU, pFlashAddr->ucLun))
        {
            DBG_Printf("PU %d Block %d Page %d read UECC!\n",pFlashAddr->ucPU,pFlashAddr->usBlock,pFlashAddr->usPage);
            return FAIL;
        }
        
        TEST_WaitSSU(pFlashAddr, pRdReq->bsSsu0Addr);
    }
    if (g_bCacheStsEn)
    {
        TEST_WaitCacheStatus(pFlashAddr, pRdReq->bsSsu0Addr);
    }

    /* NFC CMD REG status check */
    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(pFlashAddr->ucPU, pFlashAddr->ucLun))
    {
        if (g_bDecFifoEn)
        {        
            TEST_NfcRdDecStsChk(pFlashAddr, pRdReq);
        }
        else
        {
            DBG_Printf("Pu %d LUN %d BLK%d PG%d PageType%d Read Fail, ErrType:%d\n",pFlashAddr->ucPU
            ,pFlashAddr->ucLun,pFlashAddr->usBlock,pFlashAddr->usPage,pRdReq->bsTlcPgType,HAL_NfcGetErrCode(pFlashAddr->ucPU,pFlashAddr->ucLun));
            //DBG_Getch();
        }
        HAL_NfcResetCmdQue(pFlashAddr->ucPU, pFlashAddr->ucLun);
        HAL_NfcClearINTSts(pFlashAddr->ucPU, pFlashAddr->ucLun);
        return SUCCESS;
        //DBG_Getch();
    }
    else
    {
        /* DEC FIFO error cnt check */
        if (g_bDecFifoEn)
        {
            TEST_NfcRdDecStsChk(pFlashAddr, pRdReq);
            return SUCCESS;
        }
    }
    
#ifndef DATA_CHK
    return SUCCESS;      //high pressure read, do not check data
#endif
    
    /* RED check */
    if (!g_bRawDataRead)//RAW DATA READ: NFC won't update RED
    {
        TEST_NfcCheckRed(pFlashAddr, pRdReq->ppNfcRed, pRdReq->bsSecStart, pRdReq->bsSecLen, pRdReq->bsTlcPgType);
    }
    
    /* Data check */
    if (ReadType != RED_ONLY_READ)
    {
        TEST_NfcCheckData(pFlashAddr, pRdReq->bsSecStart, pRdReq->bsSecLen, pRdReq->bsRdBuffId, pRdReq->bsTlcPgType);
    }
    //DBG_Printf("Pu %d LUN %d BLK%d PG%d PageType%d Read OK!\n",pFlashAddr->ucPU,pFlashAddr->ucLun,pFlashAddr->usBlock
    //         ,pFlashAddr->usPage,pRdReq->bsTlcPgType);
             
    return SUCCESS;
}

LOCAL void TEST_NfcReadUppPg(U16 ReadBuffID, U16 usUpperPg, FLASH_ADDR *pFlashAddr)
{
    NFC_READ_REQ_DES tRdDes = {0};
    U32 *pRed;
    FLASH_ADDR tFlashAddr = {0};

    pRed = (U32*)((U32*)pWrRed + sizeof(NFC_RED));
    
    tFlashAddr = *pFlashAddr;
    tFlashAddr.usPage = usUpperPg;
    tRdDes.bsSecStart = 0;
    tRdDes.bsSecLen   = SEC_PER_PIPE_PG;//64K
    tRdDes.bsRdBuffId = ReadBuffID;
    tRdDes.bsRedOntf  = g_bRedOntf;
    tRdDes.ppNfcRed   = (NFC_RED **)&pRed;
    tRdDes.pErrInj    = NULL;
    tRdDes.bsSsu0En   = g_bSsuEn;
    tRdDes.bsSsu0Ontf = !g_bSsu0DramEn;
    tRdDes.bsSsu1En   = g_bSsuEn;
    tRdDes.bsSsu1Ontf = !g_bSsu0DramEn;
    tRdDes.bsCSEn     = g_bCacheStsEn;
                        
    tRdDes.bsSsu0Addr   = TEST_GetOffset(pFlashAddr);
    tRdDes.bsSsu1Addr   = tRdDes.bsSsu0Addr + 0x400;
    tRdDes.bsCsAddrOff  = tRdDes.bsSsu0Addr;
    while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        ;
    }
    //DBG_Printf("Read upp page%d\n", usUpperPg);
    HAL_NfcPageRead(&tFlashAddr, &tRdDes); 
    TEST_NfcRdStsAndDataChk(&tFlashAddr, &tRdDes, FULL_PAGE_READ);
    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("PU %d BLun %d ReadUppPg %d Fail,ErrCode %d.\n", tFlashAddr.ucPU, tFlashAddr.ucLun,tFlashAddr.usPage, HAL_NfcGetErrCode(tFlashAddr.ucPU, tFlashAddr.ucLun));
        HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
        HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
        DBG_Getch();
    }
    
    return;
}

/****************************************************************************
Function      : TEST_NfcWtFlashStsChk
Input         : 
Output        :
Description   : 
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL void TEST_NfcWtDummyDataPrepare(FLASH_ADDR *pFlashAddr, NFC_PRG_REQ_DES *pWrReq)
{
#ifdef DATA_CHK
    /*  initial red data and user data  */
    TEST_NfcPrepareRed((U32*)pWrReq->pNfcRed, pFlashAddr);
    TEST_NfcPrepareData(pFlashAddr);
#endif
    
    return;                
}

void TEST_NfcRdCfgReqComm(NFC_READ_REQ_DES *pRdReq, U8 ucSecStart, U16 usSecLen)
{
    pRdReq->bsSecStart      = ucSecStart;
    pRdReq->bsSecLen        = usSecLen;
    
    pRdReq->bsRedOntf       = g_bRedOntf;
    pRdReq->bsLbaChkEn      = g_bLbaChk;
    pRdReq->bsSsu0En        = g_bSsuEn;
    pRdReq->bsSsu0Ontf      = !g_bSsu0DramEn;
    pRdReq->bsSsu1En        = g_bSsuEn;
    pRdReq->bsSsu1Ontf      = !g_bSsu0DramEn;
    pRdReq->bsCSEn          = g_bCacheStsEn;
    pRdReq->bsTlcMode       = g_bTlcMode;
    pRdReq->bsRawData       = g_bRawDataRead;
    pRdReq->bsEmEn          = g_bEmEnable;
    pRdReq->bsSnapReadEn    = g_bSnapRead;

    if (g_ErrInjEn)
    {
        pRdReq->bsInjErrEn = TRUE;
        pRdReq->pErrInj = &g_tErrInj;
    }
    else
    {
        pRdReq->pErrInj = NULL;
    }
    
    if (g_bDecFifoEn)
    {
        pRdReq->bsDecFifoEn = TRUE;
    }
    
    /*  scramble disable    */
    if (g_bRawDataRead)
    {
        volatile PG_CONF_REG *pNfcPgCfg = (volatile PG_CONF_REG *) &rNfcPgCfg;
        pNfcPgCfg->bsScrBps = TRUE;
    }

    return;
}

/****************************************************************************
Function      : TEST_NfcRdTypeSel
Input         : 
Output        :
Description   : Random generate read type 
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL READ_REQ_TYPE TEST_NfcRdTypeSel(READ_REQ_TYPE CurReadType, NFC_READ_REQ_DES *pRdReq)
{
    READ_REQ_TYPE NewReadType = CurReadType;
    
#ifdef RAND_READ
    if (g_bSinglePln)
    {
        NewReadType = SING_PLN_READ + rand()%FULL_PAGE_READ;
        
        if (SING_PLN_READ == NewReadType)
        {
            pRdReq->bsSecStart = 0;
            pRdReq->bsSecLen = SEC_PER_PHYPG;
        }
        else//SING_PLN_CCL_READ
        {
            pRdReq->bsSecStart = 0 + rand()%(SEC_PER_PHYPG);
            pRdReq->bsSecLen = rand() % (SEC_PER_PHYPG - pRdReq->bsSecStart);
        }
        //DBG_Printf("OldReadType = %d NewReadType = %d SecStart%d Seclen%d\n",CurReadType,NewReadType,pRdReq->bsSecStart,pRdReq->bsSecLen);
    }
    else
    {
        NewReadType = FULL_PAGE_READ + rand()%(HOST_READ - FULL_PAGE_READ);
        
        if ((FULL_PAGE_READ == NewReadType)||(HOST_READ == NewReadType))
        {
            pRdReq->bsSecStart = 0;
            pRdReq->bsSecLen = SEC_PER_PHYPG<<PLN_PER_LUN_BITS;
        }
        else
        {
            pRdReq->bsSecStart = 0 + rand()%(SEC_PER_PHYPG<<PLN_PER_LUN_BITS);
            pRdReq->bsSecLen = rand() % ((SEC_PER_PHYPG<<PLN_PER_LUN_BITS)-pRdReq->bsSecStart);
        }
        //DBG_Printf("OldReadType = %d NewReadType = %d SecStart%d Seclen%d\n",CurReadType,NewReadType,pRdReq->bsSecStart,pRdReq->bsSecLen);
    }
#endif

    return NewReadType;
}


/*------------------------------------------------------------------------------
Name: TEST_DecFifoSetCmdIndex
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
LOCAL void TEST_DecFifoSetCmdIndex(FLASH_ADDR *pFlashAddr, U8 ucPageType, U8 ucCmdIndex)
{
    g_ucDecFifoCmdIndexMap[pFlashAddr->ucPU][pFlashAddr->ucLun][pFlashAddr->usPage* TEST_PG_PER_WL + ucPageType] = ucCmdIndex;

    return;
}

/****************************************************************************
Function      : TEST_NfcRdGetDecFifoCmdInx
Input         : 
Output        :
Description   : get cmdIndex for this cmd
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL U8 TEST_NfcRdGetDecFifoCmdInx(FLASH_ADDR *pFlashAddr, U8 ucPageType)
{
    U8 ucCmdIndex = 0;
    if (g_bDecFifoEn)
    {
        ucCmdIndex = g_ucDecFifoRp;
    }

    return ucCmdIndex;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcRdGetDsIndex
Description:
    Get data syntax index
Input Param:
Output Param:
    none
Return Value:
Usage:
History:
20160810   abby    create
------------------------------------------------------------------------------*/
LOCAL U32 TEST_NfcRdGetDsIndex(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq, READ_REQ_TYPE ReadType)
{
    U8 ucDsIndex = DS_ENTRY_SEL;

    if (SING_PLN_READ != ReadType)//only support single plane to mix data syntax
    {
        return ucDsIndex;
    }
    
#ifdef DATA_SYNTAX_MIX
    ucDsIndex = g_bSinglePln ? (pFlashAddr->usPage % 8) : DS_ENTRY_SEL;
    
    /* 15K page sector lenth shouldn't over 30 */
    volatile NFC_DATA_SYNTAX *pDsReg = (volatile NFC_DATA_SYNTAX*)NF_DS_REG_BASE;
    BOOL bIs15kPage = TRUE;

    bIs15kPage = pDsReg->atDSEntry[ucDsIndex].bsCwNum % CW_PER_PLN;
    pRdReq->bsSecLen = bIs15kPage ? ((SEC_PER_PHYPG - SEC_PER_CW)*INTRPG_PER_PGADDR) : (SEC_PER_PHYPG*INTRPG_PER_PGADDR);

    DBG_Printf("RD Page %d ucDsIndex %d SecLen %d\n",pFlashAddr->usPage,ucDsIndex,pRdReq->bsSecLen);
#endif

    return ucDsIndex;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcPageRetry
Description:
    flash read retry interface.
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:
    Need handle FullPageRead, SinglePageRead, PageReadByteMode.
History:
    20150121 tobey move form HAL_FlashDriverExt.c
------------------------------------------------------------------------------*/
LOCAL U8 TEST_NfcPageRetry(FLASH_ADDR *pFlashAddr, NFC_READ_REQ_DES *pRdReq)
{
    U8  ucPU, ucLun;
    U8  ucRetryTime,ucErrCode, ucParaNum;
    U32 ulCmdStatus, ulIndex;
    RETRY_TABLE tRetryPara = {0};
    BOOL bTlcMode = FALSE;

    ucPU  = pFlashAddr->ucPU;
    ucLun = pFlashAddr->ucLun;
    bTlcMode = pRdReq->bsTlcMode;

    // reset CQ
    //HAL_NfcResetCmdQue(ucPU, ucLun);
    //HAL_NfcClearINTSts(ucPU, ucLun);

    // Step1. pre-condition: enter to shift read mode.
    HAL_FlashRetryPreConditon(pFlashAddr);
    ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);

    ucRetryTime = 0;
    while (NFC_STATUS_SUCCESS == HAL_FlashRetryCheck(ucRetryTime, bTlcMode))
    {
        // Step2. set-parameters: adj voltage.
        ulIndex = HAL_FlashSelRetryPara(bTlcMode);
        tRetryPara = HAL_FlashGetRetryParaTab(ulIndex);
        ucParaNum = (FALSE == bTlcMode) ? HAL_FLASH_RETRY_PARA_MAX : HAL_TLC_FLASH_RETRY_PARA_MAX;

        HAL_FlashRetrySendParam(pFlashAddr, &tRetryPara, bTlcMode, ucParaNum);
        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);

        // Step3. read-retry-enable:
        HAL_FlashRetryEn(pFlashAddr, TRUE);
        ulCmdStatus = HAL_NfcWaitStatus(ucPU, ucLun);

        // Step4. redo read:
        while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
        {
            ;
        }
        if (g_bSinglePln)
        {
            HAL_NfcSinglePlnRead(pFlashAddr, pRdReq, FALSE);
        }
        else
        {
            HAL_NfcPageRead(pFlashAddr, pRdReq);
        }
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

            ucRetryTime++;
            //DBG_Printf("PU%d LUN%d Blk%d Pg#%d ReadRetry ErrType=%d CurTime:%d.\n", ucPU,ucLun
              //        ,pFlashAddr->usBlock,pFlashAddr->usPage,ucErrCode,ucRetryTime);
        }
        else
        {
            /* Data check */
            TEST_NfcCheckRed(pFlashAddr, pRdReq->ppNfcRed, pRdReq->bsSecStart, pRdReq->bsSecLen, pRdReq->bsTlcPgType);
            TEST_NfcCheckData(pFlashAddr, pRdReq->bsSecStart, pRdReq->bsSecLen, pRdReq->bsRdBuffId,  pRdReq->bsTlcPgType);
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
        //DBG_Printf("Pu %d LUN %d BLK%d PG%d Read Retry OK!\n",ucPU,ucLun,pFlashAddr->usBlock,pFlashAddr->usPage);
    }

    return ucErrCode;
}

/****************************************************************************
Function      : TEST_NfcEraseAll
Input         : 
Output        :

Description   : For local test.Support single or multi plane erase.
                Erase all blocks between range:
Reference     :
History       :
    20151112    abby    create
****************************************************************************/
void TEST_NfcEraseAll(void)
{
    FLASH_ADDR tFlashAddr = {0};
    
    tFlashAddr.bsSLCMode = !g_bTlcMode;   

    tFlashAddr.usBlock = g_usTestBlkStart;
    while (tFlashAddr.usBlock < (U16)g_usTestBlkEnd)
    {
        tFlashAddr.ucPU = g_ucTestPuStart;
        while (tFlashAddr.ucPU < g_ucTestPuEnd)
        {
            tFlashAddr.ucLun = g_ucTestLunStart;
            while (tFlashAddr.ucLun < g_ucTestLunEnd)
            {
                while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
                {
                    ;
                }
                if(TRUE == g_bSinglePln)
                {
                    HAL_NfcSingleBlockErase(&tFlashAddr, g_bTlcMode);
                }
                else
                {
                    HAL_NfcFullBlockErase(&tFlashAddr, g_bTlcMode);
                }
                tFlashAddr.ucLun++;
            }
            tFlashAddr.ucPU++;
        }
        /*    check nfc status by LUN as the basic unit    */
        for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
        {
            for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
                {   
                    if (g_ErrInjEn) break;  // do not print if error injection enable

                    DBG_Printf("PU%d LUN %d BLK%d Erase Fail!\n", tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock);
                    HAL_NfcResetLun(&tFlashAddr);//error handle, otherwise program other blk also will be fail
                }
                else
                {
                    //DBG_Printf("PU%d LUN %d BLK%d Erase OK!\n", tFlashAddr.ucPU, tFlashAddr.ucLun, tFlashAddr.usBlock);
                }
            }
        }
        tFlashAddr.usBlock++;
    }
    return;
}

/****************************************************************************
Function      : TEST_NfcWriteAll
Input         : WRITE_REQ_TYPE WriteType: single pln/full page/host page
Output        :
                none
Description   : For local test.Support single or multi plane page write.
                Write pages between range:
Reference     :
History       :
    20151112    abby    create
    20150229    abby    merge TLC write test code into MLC
****************************************************************************/
void TEST_NfcWriteAll(WRITE_REQ_TYPE WriteType)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_PRG_REQ_DES tWrReq = {0};
    U16 usPrgIndex;

    TEST_NfcWtCfgReqComm(&tWrReq);

    tFlashAddr.bsSLCMode = !g_bTlcMode;

    tFlashAddr.usBlock = g_usTestBlkStart;
    while (tFlashAddr.usBlock < g_usTestBlkEnd)
    {
        tFlashAddr.ucPU = g_ucTestPuStart;
        while (tFlashAddr.ucPU < g_ucTestPuEnd)
        {
            tFlashAddr.ucLun = g_ucTestLunStart;
            while (tFlashAddr.ucLun < g_ucTestLunEnd)
            {
                usPrgIndex = g_usTestPageStart;
                while (usPrgIndex < g_usTestPageEnd * PRG_CYC_CNT)
                {
                    tFlashAddr.usPage = TEST_NfcWtGetPageAddr(&usPrgIndex);
                    if (tFlashAddr.usPage >= g_usTestPageEnd)//page address check
                    {
                        break;
                    }
                    tWrReq.bsTlcPgCycle = TEST_NfcWtGetPrgCycle(usPrgIndex);
                    tWrReq.bsDsIndex    = TEST_NfcWtGetDsIndex(&tFlashAddr, WriteType);
                
                    TEST_NfcWtDummyDataPrepare(&tFlashAddr, &tWrReq);
                    
                    tWrReq.bsSsu0Addr   = TEST_GetOffset(&tFlashAddr);
                    tWrReq.bsSsu1Addr   = tWrReq.bsSsu0Addr + 0x400;
                    tWrReq.bsCsAddrOff  = tWrReq.bsSsu0Addr;
                    while(TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
                    {
                        ;
                    }
                    switch(WriteType)
                    {
                        case SING_PLN_WRITE :
                        {
                            HAL_NfcSinglePlaneWrite(&tFlashAddr, &tWrReq);
                        }break;

                        case FULL_PAGE_WRITE :
                        {                        
                             HAL_NfcFullPageWrite(&tFlashAddr, &tWrReq);
                        }break;

                    #ifndef HOST_SATA
                        case HOST_WRITE :
                        {
                            HAL_NfcHostPageWrite(&tFlashAddr, 0, 0, &tWrReq);
                        }break;
                    #endif

                        default :
                        {
                            DBG_Getch();
                        }break;

                    }//end switch
                    
                    /* check status of NFC CMD REG and DEC SRAM */
                    if (SUCCESS != TEST_NfcWtStsChk(&tFlashAddr, &tWrReq))
                    {
                        //DBG_Getch();
                    }

                    //DBG_Printf("Pu %d Block %d Page %d Write OK!\n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);

                    usPrgIndex++;
                }
                tFlashAddr.ucLun++;
            }
            tFlashAddr.ucPU++;
        }
        /*  check nfc status by LUN as the basic unit  */
        for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
        {
            for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
            {
                if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
                {
                    DBG_Printf("Pu %d Block %d Page %d Write Fail!\n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
                    NFC_GETCH;
                }
                if (g_bTlcMode)
                {
                    //DBG_Printf("Pu %d Block %d PageOrder %d WL %d TLC Write Time %d OK!\n",tFlashAddr.ucPU,tFlashAddr.usBlock
                    //         ,usPrgIndex,tFlashAddr.usPage,tWrReq.bsTlcPgCycle);
                }
                else
                {
                    //DBG_Printf("Pu %d Block %d Page %d Write OK!\n",tFlashAddr.ucPU,tFlashAddr.usBlock,tFlashAddr.usPage);
                }
            }
        }
        tFlashAddr.usBlock++;
    }
    return;
}

/****************************************************************************
Function      : TEST_NfcReadAll
Input         : U16 usSecStart: Sector start addr;
                U16 usSecLen: Transfer sector length;
                READ_REQ_TYPE ReadType: read req type, support
                              SING_PLN_READ/
                              FULL_PAGE_READ/
                              HOST_READ/
                              RED_ONLY_READ
Output        :
Description   : For local test.Support single or multi plane page write.
                read pages between range:
                check data: buffer don't distinct between PU and BLK
Reference     :
History       :
    20151112    abby    create
    20150229    abby    merge TLC write test code into MLC
****************************************************************************/
void TEST_NfcReadAll(U8 ucSecStart, U16 usSecLen, READ_REQ_TYPE ReadType)
{
    FLASH_ADDR tFlashAddr = {0};
    NFC_READ_REQ_DES tRdReq = {0};
    U8  ucPageType;

    TEST_NfcRdCfgReqComm(&tRdReq, ucSecStart, usSecLen);
    
    tFlashAddr.bsSLCMode = !g_bTlcMode;

    tFlashAddr.usBlock = g_usTestBlkStart;
    while (tFlashAddr.usBlock < (U16)g_usTestBlkEnd)
    {
        tFlashAddr.ucPU = g_ucTestPuStart;
        while (tFlashAddr.ucPU < g_ucTestPuEnd)
        {
            tFlashAddr.ucLun = g_ucTestLunStart;
            while (tFlashAddr.ucLun < g_ucTestLunEnd)
            {
                tFlashAddr.usPage = g_usTestPageStart;
                while (tFlashAddr.usPage < g_usTestPageEnd)
                {                    
                    for(ucPageType = 0; ucPageType < TEST_PG_PER_WL; ucPageType++)
                    {
                        tRdReq.bsTlcPgType  = ucPageType;
                        tRdReq.bsRdBuffId   = TEST_NfcGetRdBufId(&tFlashAddr, ucPageType);
                        tRdReq.bsLba        = TEST_NfcGetReadLba(&tFlashAddr, tRdReq.bsSecStart);
                        tRdReq.ppNfcRed     = (NFC_RED **)&pRRed[TEST_NfcGetRedOffset(&tFlashAddr)];
                        tRdReq.bsDecFifoIndex = TEST_NfcRdGetDecFifoCmdInx(&tFlashAddr, ucPageType);
                        tRdReq.bsSsu0Addr   = TEST_GetOffset(&tFlashAddr);
                        tRdReq.bsSsu1Addr   = tRdReq.bsSsu0Addr + 0x400;
                        tRdReq.bsCsAddrOff  = tRdReq.bsSsu0Addr;
                        tRdReq.bsDsIndex    = TEST_NfcRdGetDsIndex(&tFlashAddr, &tRdReq, ReadType);
                        
                        /* select a new random read type or do nothing */
                        ReadType = TEST_NfcRdTypeSel(ReadType, &tRdReq);
                    
                        while (TRUE == HAL_NfcGetFull(tFlashAddr.ucPU, tFlashAddr.ucLun))
                        {
                            ;
                        }
                        switch (ReadType)
                        {
                            case SING_PLN_READ :
                            {
                                HAL_NfcSinglePlnRead(&tFlashAddr, &tRdReq, FALSE);
                            }break;

                            case FULL_PAGE_READ :
                            {
                                HAL_NfcPageRead(&tFlashAddr, &tRdReq);
                            }break;

                        #ifndef HOST_SATA
                            case HOST_READ :
                            {
                                HAL_NfcHostPageRead(&tFlashAddr, 0, 0, &tRdReq);
                            }break;
                        #endif

                            case SING_PLN_CCL_READ :
                            {
                                /* normal page read first    */
                                HAL_NfcSinglePlnRead(&tFlashAddr, &tRdReq, FALSE);
                                HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);

                                TEST_NfcCheckRed(&tFlashAddr, tRdReq.ppNfcRed, tRdReq.bsSecStart, tRdReq.bsSecLen, ucPageType);
                                TEST_NfcCheckData(&tFlashAddr, tRdReq.bsSecStart, tRdReq.bsSecLen, tRdReq.bsRdBuffId, ucPageType);

                                HAL_NfcSinglePlnCCLRead(&tFlashAddr, &tRdReq);
                            }break;

                            case CHANGE_COL_READ :
                            {
                                /* full page read first    */
                                HAL_NfcPageRead(&tFlashAddr, &tRdReq);
                                HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
                                TEST_NfcCheckRed(&tFlashAddr, tRdReq.ppNfcRed, tRdReq.bsSecStart, tRdReq.bsSecLen, ucPageType);
                                TEST_NfcCheckData(&tFlashAddr, tRdReq.bsSecStart, tRdReq.bsSecLen, tRdReq.bsRdBuffId, ucPageType);

                                HAL_NfcChangeColRead(&tFlashAddr, &tRdReq);
                            }break;

                            case RED_ONLY_READ :
                            {
                                HAL_NfcRedOnlyRead(&tFlashAddr, &tRdReq);
                            }break;

                            default :
                            {
                                DBG_Getch();
                            }
                        }//end switch

                        /* check status and data */
                        TEST_NfcRdStsAndDataChk(&tFlashAddr, &tRdReq, ReadType);

                        /* Read Retry */
                        if (g_bForceRetryEn)     //force to retry for test
                        {
                            TEST_NfcPageRetry(&tFlashAddr, &tRdReq);
                        }
                    }
                    tFlashAddr.usPage++;
                }// end page loop
                tFlashAddr.ucLun++;
            }
            tFlashAddr.ucPU++;
        }
        tFlashAddr.usBlock++;
    }
    
#ifndef DATA_CHK//no data check in FW
    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
            {
                HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
                HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
                DBG_Printf("Pu %d LUN %d Read Fail, ErrType:%d\n",tFlashAddr.ucPU
                    , tFlashAddr.ucLun, HAL_NfcGetErrCode(tFlashAddr.ucPU, tFlashAddr.ucLun));
                DBG_Getch();
            }
        }
    }
#endif

    return;
}
BOOL TEST_IsTlcModeSupport(void)
{
#if (defined(FLASH_INTEL_3DTLC) || defined (FLASH_3D_MLC))
    if (g_bTlcMode)
    { 
        DBG_Printf("Single plane operation only support SLC mode in this flash type!\n");
        return FALSE;
    }
#endif

    return TRUE;
}
/****************************************************************************
Function      : TEST_NfcSinglePlnEWR
Input         :
Output        :

Description   : To test single plane EWR
Reference     :
History       :
    20151112    abby    create
****************************************************************************/
void TEST_NfcSinglePlnEWR(void)
{
    if (TRUE != TEST_IsTlcModeSupport())
    { 
        return;
    }

    g_bSinglePln = TRUE;
    
    //Erase
    TEST_NfcEraseAll();
    
    //Program
    TEST_NfcWriteAll(SING_PLN_WRITE);

    //Read
    TEST_NfcReadAll(0, SEC_PER_PHYPG, SING_PLN_READ);

    g_bSinglePln = FALSE;
    
    return;
}

/****************************************************************************
Function      : TEST_NfcMultiPlnEWR
Input         :
Output        :
Description   : To test multi plane EWR
Reference     :
History       :
    20151112    abby    create
****************************************************************************/
LOCAL void TEST_NfcMultiPlnEWR(void)
{
    g_bSinglePln = FALSE;

    TEST_NfcEraseAll();

    TEST_NfcWriteAll(FULL_PAGE_WRITE);

    TEST_NfcReadAll(0, SEC_PER_PIPE_PG, FULL_PAGE_READ);

    return;
}

/****************************************************************************
Function      : TEST_NfcHostWR
Input         :
Output        :
Description   : To test  host WR req, transfer data by OTFB
Reference     :
History       :
    20151207    abby    create
****************************************************************************/
LOCAL void TEST_NfcHostFullPageWR(void)
{
    g_bSinglePln = FALSE;
    
    TEST_NfcEraseAll();

    TEST_NfcWriteAll(HOST_WRITE);

    TEST_NfcReadAll(0, SEC_PER_PIPE_PG, HOST_READ);

    return;
}

/****************************************************************************
Function      : TEST_NfcSinglePlnCCLRead
Input         :
Output        :
Description   : To test single plane change column read, but full plane EW
Reference     :
History       :
    20160308    abby    create
****************************************************************************/
LOCAL void TEST_NfcSinglePlnCCLRead(void)
{
    if (TRUE != TEST_IsTlcModeSupport())
    { 
        return;
    }

    U8 ucSecStart;
    U16 usSecLen;
    
    g_bSinglePln = TRUE;
    
    /*     full block erase    */
    TEST_NfcEraseAll();

    /*     write whole page    */
    TEST_NfcWriteAll(SING_PLN_WRITE);

    /*     change column read read range depends on sector address    */
    for (ucSecStart = 0; ucSecStart < SEC_PER_PHYPG; )
    {
        /* change column read    */
        usSecLen = SEC_PER_PHYPG - ucSecStart;
        //DBG_Printf("Single Pln Change Column Read Start Sec%d, Sec Length%d;\n",ucSecStart, usSecLen);
        TEST_NfcReadAll(ucSecStart, usSecLen, SING_PLN_CCL_READ);
        ucSecStart = ucSecStart + 15;
    }

    g_bSinglePln = FALSE;

    return;
}

/****************************************************************************
Function      : TEST_NfcChangeColumnRead
Input         :
Output        :
Description   : To test full plane change column read
Reference     :
History       :
    20151112    abby    create
****************************************************************************/
LOCAL void TEST_NfcChangeColumnRead(void)
{
    U8  ucSecStart, ucSecStep;
    U16 usSecLen, usSecMax;
    
    ucSecStep = 30; 
    usSecMax  = SEC_PER_PIPE_PG;
    
    g_bSinglePln = FALSE;

    /*     full block erase    */
    TEST_NfcEraseAll();

    /*     write whole page    */
    TEST_NfcWriteAll(FULL_PAGE_WRITE);

    /*     change column read read range depends on sector address    */
    for (ucSecStart = 0; ucSecStart < usSecMax; )
    {
        /* change column read    */
        usSecLen = usSecMax - ucSecStart;
        //DBG_Printf("Change Column Read Start Sec%d, Sec Length%d\n", ucSecStart, usSecLen);
        TEST_NfcReadAll(ucSecStart, usSecLen, CHANGE_COL_READ);
        ucSecStart = ucSecStart + ucSecStep;
    }
    return;
}



/****************************************************************************
Function      : TEST_NfcPartialRead
Input         :
Output        :

Description   : To test partial read, include full page erase/write
Reference     :
History       :
    20151112    abby    create
****************************************************************************/
LOCAL void TEST_NfcPartialRead(void)
{
    U16 usSecLen, usSecMax;
    U8  ucSecStart, ucSecStep;
    
    usSecMax  = SEC_PER_PIPE_PG;
    ucSecStep = 30;
    
    g_bSinglePln = FALSE;

    /*     full block erase    */
    TEST_NfcEraseAll();

    /*     write whole page    */
    TEST_NfcWriteAll(FULL_PAGE_WRITE);

    /*     partial read read range depends on sector address    */
    for (ucSecStart = 0; ucSecStart < usSecMax;)
    {
        usSecLen = usSecMax - ucSecStart;
        //DBG_Printf("Partial Read Start Sec%d, Sec Length%d\n", ucSecStart, usSecLen);
        TEST_NfcReadAll(ucSecStart, usSecLen, FULL_PAGE_READ);
        ucSecStart = ucSecStart + ucSecStep;
    }

    return;
}

/****************************************************************************
Function      : TEST_NfcPartialRead
Input         :
Output        :

Description   : To test partial read, include full page erase/write
Reference     :
History       :
    20151112    abby    create
****************************************************************************/
LOCAL void TEST_NfcRedOnlyRead(void)
{
    g_bSinglePln = FALSE;

    TEST_NfcEraseAll();

    TEST_NfcWriteAll(FULL_PAGE_WRITE);

    /* only read red data */
    TEST_NfcReadAll(0, SEC_PER_PIPE_PG, RED_ONLY_READ);

    return;
}

/****************************************************************************
Function  : TEST_NfcSSUandCS
Input     :
Output    :

Purpose   : FW check if cache status is right, if right, clr it
Reference :
****************************************************************************/
LOCAL void TEST_NfcSSUandCS(void)
{
    /* disable RECC_EN in this case */
    rNfcPgCfg &= ~(1<<16);

    /* enable SSU & Cache Status */
    g_bSsuEn = TRUE;
    g_bCacheStsEn = TRUE;

    /* Erase */
    TEST_NfcEraseAll();

    /* Write */
    TEST_NfcWriteAll(FULL_PAGE_WRITE);
    
    /* Read */
    TEST_NfcReadAll(0, SEC_PER_PIPE_PG, FULL_PAGE_READ);
    
    /* enable RECC_EN after this case */
    rNfcPgCfg |= (1<<16);
    
    /* recover to default value */
    g_bSsuEn = FALSE;
    g_bCacheStsEn = FALSE;

    return;
}

/****************************************************************************
Function  : TEST_NfcSSUDynamUpdate
Input     :
Output    :
Purpose   :   To test SSU dynamically write and read to DRAM or OTFB.
              Test 4 groups of RED WR path for 4 kinds of read req, include:
              OTFB_W_OTFB_R
              OTFB_W_DRAM_R
              DRAM_W_DRAM_R
              DRAM_W_OTFB_R
Reference :
History        :
    20151203    abby    create
****************************************************************************/
LOCAL void TEST_NfcSSUDynamUpdate(void)
{
#ifdef DRAMLESS_ENABLE
    DBG_Printf("DRAMLESS_ENABLE: not support dynamically update SSU, only OTFB valid!\n");
    return;
#endif

    SSU_RED_UPDATE_SEL eRedCaseSel;

    /* disable RECC_EN in this case */
    rNfcPgCfg &= ~(1<<16);
    
    /* enable SSU  */
    g_bSsuEn = TRUE;

    eRedCaseSel = OTFB_W_OTFB_R;
    while (eRedCaseSel < SSU_RED_CASE_CNT)
    {
        /* Erase */
        TEST_NfcEraseAll();

        /* Write */
        g_bSsu0DramEn = ((U32)eRedCaseSel & 0x2) >> 1; //get 2nd bit of pattern sel
        g_bSsu1DramEn = ((U32)eRedCaseSel & 0x2) >> 1; //get 2nd bit of pattern sel
        TEST_NfcWriteAll(FULL_PAGE_WRITE);
    
        /* Read */
        g_bSsu0DramEn = ((U32)eRedCaseSel & 0x1); //get 1st bit of pattern sel
        g_bSsu1DramEn = ((U32)eRedCaseSel & 0x1); //get 1st bit of pattern sel
        TEST_NfcReadAll(0, SEC_PER_PIPE_PG, FULL_PAGE_READ);

        eRedCaseSel++;
    }
    
    /*    recover to default value    */
    g_bSsu0DramEn = FALSE;
    g_bSsu1DramEn = FALSE;
    g_bSsuEn = FALSE;

    /* enable RECC_EN after this case */
    rNfcPgCfg |= (1<<16);

    return;
}


/****************************************************************************
Function      : TEST_NfcRedDynamUpdate
Input         :
Output        :

Description    : To test RED dynamically write and read to DRAM or OTFB.
              Test 4 groups of RED WR path for 4 kinds of read req, include:
              OTFB_W_OTFB_R
              OTFB_W_DRAM_R
              DRAM_W_DRAM_R
              DRAM_W_OTFB_R
Reference     :
History        :
    20151203    abby    create
****************************************************************************/
LOCAL void TEST_NfcRedDynamUpdate(void)
{
#ifdef DRAMLESS_ENABLE
    DBG_Printf("DRAMLESS_ENABLE: not support dynamically update RED, only OTFB valid!\n");
    return;
#endif

    SSU_RED_UPDATE_SEL eRedCaseSel;
    READ_REQ_TYPE eReadTypeSel = SING_PLN_READ;
    U16 usSecLen;
    U16 usSecMax;
    U8  ucSecStart, ucSecStep;

    /*    case 1: single plane, write RED from OTFB, read to DRAM    */
    switch (eReadTypeSel)
    {
        case SING_PLN_READ :
        {
            for (eRedCaseSel = OTFB_W_OTFB_R; eRedCaseSel < SSU_RED_CASE_CNT; eRedCaseSel++)
            {
                if (TRUE != TEST_IsTlcModeSupport())
                { 
                    break;
                }

                g_bSinglePln = TRUE;

                TEST_NfcEraseAll();
                g_bRedOntf = !(((U32)eRedCaseSel & 0x2) >> 1); //get 2nd bit of pattern sel
                TEST_NfcWriteAll(SING_PLN_WRITE);
                g_bRedOntf = !((U32)eRedCaseSel & 0x1); //get 1st bit of pattern sel
                TEST_NfcReadAll(0, SEC_PER_PHYPG, SING_PLN_READ);

                g_bSinglePln = FALSE;
            }
            DBG_Printf("SING_PLN_READ Red Dynamically Update OK!\n");
        }//break;

        case FULL_PAGE_READ :
        {
            usSecLen = SEC_PER_PIPE_PG;
            for (eRedCaseSel = OTFB_W_OTFB_R; eRedCaseSel < SSU_RED_CASE_CNT; eRedCaseSel++)
            {
                g_bSinglePln = FALSE;
                TEST_NfcEraseAll();
                g_bRedOntf = ((U32)eRedCaseSel & 0x2) >> 1; //get 2nd bit of pattern sel
                TEST_NfcWriteAll(FULL_PAGE_WRITE);
                g_bRedOntf = (U32)eRedCaseSel & 0x1; //get 1st bit of pattern sel
                TEST_NfcReadAll(0, usSecLen, FULL_PAGE_READ);
            }
            DBG_Printf("FULL_PAGE_READ Red Dynamically Update OK!\n");
        }//break;

        case RED_ONLY_READ :
        {
            usSecLen = SEC_PER_PIPE_PG;
            for (eRedCaseSel = OTFB_W_OTFB_R; eRedCaseSel < SSU_RED_CASE_CNT; eRedCaseSel++)
            {
                g_bSinglePln = FALSE;
                TEST_NfcEraseAll();
                g_bRedOntf = ((U32)eRedCaseSel & 0x2) >> 1; //get 2nd bit of pattern sel
                TEST_NfcWriteAll(FULL_PAGE_WRITE);
                g_bRedOntf = (U32)eRedCaseSel & 0x1; //get 1st bit of pattern sel
                TEST_NfcReadAll(0, usSecLen, RED_ONLY_READ);
            }
            DBG_Printf("RED_ONLY_READ Red Dynamically Update OK!\n");
        }//break;
        
        case CHANGE_COL_READ :
        {
            ucSecStep = 30;
            usSecMax  = SEC_PER_PIPE_PG;
            for (eRedCaseSel = OTFB_W_OTFB_R; eRedCaseSel < SSU_RED_CASE_CNT; eRedCaseSel++)
            {
                g_bSinglePln = FALSE;
                TEST_NfcEraseAll();
                g_bRedOntf = ((U32)eRedCaseSel & 0x2) >> 1; //get 2nd bit of pattern sel
                TEST_NfcWriteAll(FULL_PAGE_WRITE);
                g_bRedOntf = (U32)eRedCaseSel & 0x1; //get 1st bit of pattern sel
                for (ucSecStart = 0; ucSecStart < usSecMax; )
                {
                    /* change column read    */
                    usSecLen = usSecMax - ucSecStart;
                    //DBG_Printf("Change Column Read Start Sec%d, Sec Length%d;\n",ucSecStart, usSecLen);
                    TEST_NfcReadAll(ucSecStart, usSecLen, CHANGE_COL_READ);
                    ucSecStart = ucSecStart + ucSecStep;
                }
            }
            DBG_Printf("CHANGE_COL_READ Red Dynamically Update OK!\n");
        }break;

    
        default : 
        {
            DBG_Printf("Wrong Pattern Sel for Red Dynamically Update!\n");
            DBG_Getch();
        }
    }//end of switch
    g_bRedOntf = TRUE;

    return;
}

/*------------------------------------------------------------------------------
Name: TEST_NfcReadRetry
Description:
    Calculate Read LBA value for NFC LBA Check
Input Param:
Output Param:
    none
Return Value:
Usage:
History:
    20160217   abby    create
------------------------------------------------------------------------------*/
LOCAL void TEST_NfcReadRetry(void)
{
    g_bForceRetryEn = TRUE;
    
    /*  1 plane EWR  */
    if (!g_bTlcMode)
    { 
        g_bSinglePln = TRUE;

        TEST_NfcEraseAll();
        TEST_NfcWriteAll(SING_PLN_WRITE);
        
        TEST_NfcReadAll(0, SEC_PER_PHYPG, SING_PLN_READ);
        DBG_Printf("Single Plane Read Retry Done!\n");
    }

    /*  Full plane EWR  */
    g_bSinglePln = FALSE;
    
    TEST_NfcEraseAll();
    TEST_NfcWriteAll(FULL_PAGE_WRITE);
    
    TEST_NfcReadAll(0, SEC_PER_PIPE_PG, FULL_PAGE_READ);
    DBG_Printf("Full Plane Read Retry Done!\n");

    g_bForceRetryEn = FALSE;

    return;
}

/****************************************************************************
Function  : TEST_NfcPuBitMap
Input     :
Output    :

Purpose   :
Reference :
****************************************************************************/
LOCAL void TEST_NfcPuBitMap(void)
{
    FLASH_ADDR tFlashAddr = {0};
    volatile U32 ulPuBitMap;
    U32 ulPuIndex;
    U32 ulTrigTimes;

    tFlashAddr.bsSLCMode = !g_bTlcMode;
    tFlashAddr.usBlock = g_usTestBlkStart;
    tFlashAddr.ucLun = g_ucTestLunStart;
    
    /*  check PU idle before trigger cmd  */
    ulPuBitMap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_IDLE);
    for (ulPuIndex = g_ucTestPuStart; ulPuIndex < g_ucTestPuEnd; ulPuIndex++)
    {
        if (FALSE == (ulPuBitMap & (1 << ulPuIndex))) // current PU not idle
        {
            DBG_Printf("Wrong STS! PU %d is Busy before trig cmd, PU BitMap = 0x%x\n",ulPuIndex, ulPuBitMap);
        }
    }

    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.ucLun = 0;tFlashAddr.ucLun < NFC_LUN_PER_PU;tFlashAddr.ucLun++)
        {
            /*  Erase: just do a cmd  */
            HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
        }
    }

    /*  check PU idle after trigger cmd  */
    ulPuBitMap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_IDLE);
    for (ulPuIndex = g_ucTestPuStart; ulPuIndex < g_ucTestPuEnd; ulPuIndex++)
    {
        if (FALSE == (ulPuBitMap & (1 << ulPuIndex)))
        {
            DBG_Printf("Right STS! PU %d is Busy after trig cmd, PU BitMap = 0x%x\n",ulPuIndex, ulPuBitMap);
        }
    }

    /*    check PU empty before trigger cmd    */
    ulPuBitMap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_EMPTY);
    for (ulPuIndex = g_ucTestPuStart; ulPuIndex < g_ucTestPuEnd; ulPuIndex++)
    {
        if (FALSE == (ulPuBitMap & (1 << ulPuIndex))) // current PU not idle
        {
            DBG_Printf("Wrong STS! PU %d is not empty before trig cmd, PU BitMap = 0x%x\n",ulPuIndex, ulPuBitMap);
        }
    }

    for (tFlashAddr.ucPU =  g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.ucLun = 0; tFlashAddr.ucLun < NFC_LUN_PER_PU; tFlashAddr.ucLun++)
        {
            /*     Erase: just do a cmd    */
            HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
        }
    }

    /*    check PU empty after trigger cmd  */
    ulPuBitMap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_EMPTY);
    for (ulPuIndex = g_ucTestPuStart; ulPuIndex < g_ucTestPuEnd; ulPuIndex++)
    {
        if (FALSE == (ulPuBitMap & (1 << ulPuIndex)))
        {
            DBG_Printf("Right STS! PU %d is not Empty after trig cmd, PU BitMap = 0x%x\n",ulPuIndex, ulPuBitMap);
        }
    }

    /*    check PU not full before trigger cmd    */
    ulPuBitMap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_NOTFULL);
    for (ulPuIndex = g_ucTestPuStart; ulPuIndex < g_ucTestPuEnd; ulPuIndex++)
    {
        if (FALSE == (ulPuBitMap & (1 << ulPuIndex))) // current PU not idle
        {
            DBG_Printf("Wrong STS! PU %d is Full before trig cmd, PU BitMap = 0x%x\n",ulPuIndex, ulPuBitMap);
        }
    }
    tFlashAddr.ucPU =  g_ucTestPuStart;
    for (ulTrigTimes = 0; ulTrigTimes < NFCQ_DEPTH; ulTrigTimes++)
    {
        for (tFlashAddr.ucLun = 0; tFlashAddr.ucLun < NFC_LUN_PER_PU; tFlashAddr.ucLun++)
        {
            /*     Erase: just do a cmd    */
            HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
        }
    }

    /*    check PU not full after trigger cmd  */
    ulPuBitMap = HAL_NfcGetLogicPuBitMap(LOGIC_PU_BITMAP_NOTFULL);
    for (ulPuIndex = g_ucTestPuStart; ulPuIndex < g_ucTestPuEnd; ulPuIndex++)
    {
        if (FALSE == (ulPuBitMap & (1 << ulPuIndex)))
        {
            DBG_Printf("Right STS! PU %d is Full after trig %d cmd, PU BitMap = 0x%x\n", ulPuIndex, NFCQ_DEPTH, ulPuBitMap);
        }
    }

    /* reset CQ */
    for (tFlashAddr.ucPU =  g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        for (tFlashAddr.ucLun = 0; tFlashAddr.ucLun < NFC_LUN_PER_PU; tFlashAddr.ucLun++)
        {
        #ifndef SIM
            HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        #else
            HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
            HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
        #endif
        }
    }
    
    return;
}

/****************************************************************************
Function  : TEST_NfcLunBitMap
Input     :
Output    :

Purpose   :
Reference :
****************************************************************************/
LOCAL void TEST_NfcLunBitMap(void)
{
    FLASH_ADDR tFlashAddr = {0};
    volatile U32 ulLunBitMap;
    U32 ulTrigTimes;
    
    tFlashAddr.bsSLCMode = !g_bTlcMode;
    tFlashAddr.usBlock = g_usTestBlkStart;

    for (tFlashAddr.ucPU = g_ucTestPuStart;tFlashAddr.ucPU < g_ucTestPuEnd;tFlashAddr.ucPU++)
    {
        /*    check PU idle before trigger cmd    */
        ulLunBitMap = HAL_NfcGetLLunStsBitMap(tFlashAddr.ucPU, LOGIC_LUN_BITMAP_IDLE);
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if (FALSE == (ulLunBitMap & (1 << (tFlashAddr.ucLun + NFC_LUN_PER_PU * (tFlashAddr.ucPU%8)))))
            {
                DBG_Printf("PU %d LUN %d is Busy before trig cmd, LUN BitMap = 0x%x\n",tFlashAddr.ucPU, tFlashAddr.ucLun, ulLunBitMap);
            }
        }

        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            /*     Erase: just do a cmd    */
            HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
        }

        /*    check PU idle after trigger cmd  */
        ulLunBitMap = HAL_NfcGetLLunStsBitMap(tFlashAddr.ucPU, LOGIC_LUN_BITMAP_IDLE);
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if (FALSE == (ulLunBitMap & (1 << (tFlashAddr.ucLun + NFC_LUN_PER_PU * (tFlashAddr.ucPU%8)))))
            {
                DBG_Printf("Right STS! PU %d LUN %d is Busy after trig cmd, LUN BitMap = 0x%x\n",tFlashAddr.ucPU, tFlashAddr.ucLun, ulLunBitMap);
            }
        }

        /*  check PU empty before trigger cmd  */
        ulLunBitMap = HAL_NfcGetLLunStsBitMap(tFlashAddr.ucPU, LOGIC_LUN_BITMAP_EMPTY);
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if (FALSE == (ulLunBitMap & (1 << (tFlashAddr.ucLun + NFC_LUN_PER_PU * (tFlashAddr.ucPU%8)))))
            {
                DBG_Printf("PU %d LUN %d is not empty before trig cmd, LUN BitMap = 0x%x\n",tFlashAddr.ucPU, tFlashAddr.ucLun, ulLunBitMap);
            }
        }

        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            /*   Erase: just do a cmd   */
            HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
        }

        /*  check PU empty after trigger cmd  */
        ulLunBitMap = HAL_NfcGetLLunStsBitMap(tFlashAddr.ucPU, LOGIC_LUN_BITMAP_EMPTY);
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if (FALSE == (ulLunBitMap & (1 << (tFlashAddr.ucLun + NFC_LUN_PER_PU * (tFlashAddr.ucPU%8)))))
            {
                DBG_Printf("Right STS! PU %d LUN %d is not Empty after trig cmd, LUN BitMap = 0x%x\n",tFlashAddr.ucPU, tFlashAddr.ucLun, ulLunBitMap);
            }
        }

        /*  check PU not full before trigger cmd    */
        ulLunBitMap = HAL_NfcGetLLunStsBitMap(tFlashAddr.ucPU, LOGIC_LUN_BITMAP_NOTFULL);
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if (FALSE == (ulLunBitMap & (1 << (tFlashAddr.ucLun + NFC_LUN_PER_PU * (tFlashAddr.ucPU%8)))))
            {
                DBG_Printf("PU %d LUN %d is Full before trig cmd, LUN BitMap = 0x%x\n",tFlashAddr.ucPU, tFlashAddr.ucLun, ulLunBitMap);
            }
        }
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            for (ulTrigTimes = 0; ulTrigTimes < NFCQ_DEPTH; ulTrigTimes++)
            {
                /*     Erase: just do a cmd    */
                HAL_NfcFullBlockErase(&tFlashAddr, FALSE);
            }
        }

        /*    check PU not full after trigger cmd  */
        ulLunBitMap = HAL_NfcGetLLunStsBitMap(tFlashAddr.ucPU, LOGIC_LUN_BITMAP_NOTFULL);
        for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
        {
            if (FALSE == (ulLunBitMap & (1 << (tFlashAddr.ucLun + NFC_LUN_PER_PU * (tFlashAddr.ucPU%8)))))
            {
                DBG_Printf("Right STS! PU %d LUN %d is Full after trig %d cmd, LUN BitMap = 0x%x\n", tFlashAddr.ucPU, NFCQ_DEPTH, tFlashAddr.ucLun, ulLunBitMap);
            }
        }

        /* reset CQ */
        for (tFlashAddr.ucLun = 0; tFlashAddr.ucLun < NFC_LUN_PER_PU; tFlashAddr.ucLun++)
        {
        #ifndef SIM
            HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun);
        #else
            HAL_NfcResetCmdQue(tFlashAddr.ucPU, tFlashAddr.ucLun);
            HAL_NfcClearINTSts(tFlashAddr.ucPU, tFlashAddr.ucLun);
        #endif
        }
    }

    return;
}

/****************************************************************************
Function  : TEST_NfcErrTypeInj
Input     :
Output    :

Purpose   : test error type report
Reference :
****************************************************************************/
LOCAL void TEST_NfcErrTypeInj(void)//SLC mode, seems to exist issue,pending
{
    U8 ucErrType;
    FLASH_ADDR tFlashAddr = {0};

    /*  enable error injection  */
    g_ErrInjEn = TRUE;

    /* Error type injection  */
    for(ucErrType = 0; ucErrType <= NF_ERR_TYPE_RDST_INT; ucErrType++)
    {
        if ((NF_ERR_TYPE_UECC == ucErrType) || (NF_ERR_TYPE_RECC == ucErrType))
        {
            continue;
        }
        g_tErrInj.bsInjErrType = ucErrType;
        
        TEST_NfcEraseAll();
        
        /* Check error type */
        for (tFlashAddr.ucPU = g_ucTestPuStart; tFlashAddr.ucPU < g_ucTestPuEnd; tFlashAddr.ucPU++)
        {
            for (tFlashAddr.ucLun = g_ucTestLunStart; tFlashAddr.ucLun < g_ucTestLunEnd; tFlashAddr.ucLun++)
            {
                if (HAL_NfcGetErrCode(tFlashAddr.ucPU, tFlashAddr.ucLun) != ucErrType)
                {
                    DBG_Printf("PU%d LUN%d Erase Inject Error Type:%d Actual Error Code:%d\n"
                    ,tFlashAddr.ucPU,tFlashAddr.ucLun,ucErrType,HAL_NfcGetErrCode(tFlashAddr.ucPU,tFlashAddr.ucLun));
                    //NFC_GETCH;
                }
                else//clr error
                {
                    HAL_NfcResetCmdQue(tFlashAddr.ucPU,tFlashAddr.ucLun);
                    HAL_NfcClearINTSts(tFlashAddr.ucPU,tFlashAddr.ucLun);
                    //DBG_Printf("PU%d LUN%d Erase Inject Error Type:%d Pass\n"
                    //,tFlashAddr.ucPU,tFlashAddr.ucLun,ucErrType);
                }
            }
        }
    }
    /*  Disable error injection  */
    g_tErrInj.bsInjErrType = 0;  
    g_ErrInjEn = FALSE;
    
    return;
}


/****************************************************************************
Function  : TEST_NfcErrStsInj
Input     :
Output    :

Purpose   : test error status injection of program
Reference :
****************************************************************************/
LOCAL void TEST_NfcErrStsInj(void)
{
    U8 ucIdx;
    g_bSinglePln = FALSE;

    FLASH_ADDR tFlashAddr = {0};

    for (ucIdx = 0; ucIdx < 3; ucIdx++)
    {
        TEST_NfcEraseAll();

        /* enable error injection and error status injection  */
        g_ErrInjEn = TRUE;
        
        if (0 == ucIdx)
            g_tErrInj.bsInjErrSts = NF_ERR_INJ_TYPE_PRG;//program fail 
        else if (1 == ucIdx)
            g_tErrInj.bsInjErrSts = NF_ERR_INJ_TYPE_PRE_PG_PRG;//pre page program fail
        else
            g_tErrInj.bsInjErrSts = NF_ERR_INJ_TYPE_PRE_CUR_PRG;//cur page program fail
        
        TEST_NfcWriteAll(FULL_PAGE_WRITE);

        /* disable error injection and error status injection  */
        g_ErrInjEn = FALSE;
        g_tErrInj.bsInjErrSts = 0;
    }
    
    return;
}

/****************************************************************************
Function  : TEST_NfcErrBitInj
Input     :
Output    :

Purpose   : test error bit injection
Reference :
****************************************************************************/
LOCAL void TEST_NfcErrBitInj(void)
{
    U8 ucCwStart, ucCwLen;
    U16 usInjErrCntAcc = 0;
    U16 usInjErrCntAcc0T1 = 0;  
    U32 ulHardDecBitMap = 0;
    DEC_FIFO_STATUS_ENTRY* tDecFifoSts = {0};
    NFC_ERR_INJ tErrInj = {0};
    FLASH_ADDR tFlashAddr = {0};
    
    /*  enable DEC fifo report  */
    g_bDecFifoEn = TRUE;

    rNfcModeConfig |= 0x3<<20;
    rNfcNfdmaCfg |= 0x1<<11;
    
    if (TRUE != TEST_IsTlcModeSupport())
    { 
        return;
    }

    g_bSinglePln = TRUE;

    TEST_NfcEraseAll();
    
    TEST_NfcWriteAll(SING_PLN_WRITE);

    /*  enable error injection  */
    g_ErrInjEn = TRUE;
    g_tErrInj.bsInjErrType = NF_ERR_TYPE_UECC;//NF_ERR_TYPE_RECC;
    
    /* Error Bit injection, 1 plane */
    for (ucCwLen = 0; ucCwLen < CW_PER_PLN; ucCwLen++) // 16 CWs
    {
        for (ucCwStart = 0; ucCwStart < CW_PER_PLN - ucCwLen; ucCwStart++)
        {
            g_tErrInj.bsInjCwStart = ucCwStart;
            g_tErrInj.bsInjCwLen   = ucCwLen;
        #ifdef DEC_FIFO_ERRCNT_CHK
            g_tErrInj.bsInjErrBitPerCw = rand() % 50;
            g_tErrInj.bsInjErrType = NF_ERR_TYPE_RECC;
        #elif (defined (HARD_PLUS_BITMAP_CHK))
            g_tErrInj.bsInjErrBitPerCw = 100;
        #else
            g_tErrInj.bsInjErrBitPerCw = 150;//rand() % NFC_ERR_INJ_BIT_MAX_PER_CW;
        #endif
               
            //DBG_Printf("Read err bit inj start: CwStart%d CwLen%d ErrBitPerCw%d\n", g_tErrInj.bsInjCwStart, g_tErrInj.bsInjCwLen, g_tErrInj.bsInjErrBitPerCw);

            TEST_NfcReadAll(0, SEC_PER_PHYPG, SING_PLN_READ);
        }
    }
    
    /* disable error injection and DEC fifo report  */
    g_ErrInjEn = FALSE;
    g_tErrInj = tErrInj;
    g_bDecFifoEn = FALSE; 
    g_ucEntryIndex = g_ucDecFifoRp;

    g_bSinglePln = FALSE;

    DBG_Printf("ERR BIT INJ PASS CwStart%d CwLen%d ErrBitPerCw%d\n\n"
    , g_tErrInj.bsInjCwStart, g_tErrInj.bsInjCwLen, g_tErrInj.bsInjErrBitPerCw);
    
    return;
}

/****************************************************************************
Function  : TEST_NfcErrStsInj
Input     :
Output    :

Purpose   : test error type report
Reference :
****************************************************************************/
LOCAL void TEST_NfcErrInjDecRep(void)
{
    TEST_NfcErrTypeInj();

    TEST_NfcErrStsInj();

    TEST_NfcErrBitInj();

    return;
}

/* by set feature to switch to snap read mode */
LOCAL void TEST_NfcSnapReadBySetFeat(void)
{
    FLASH_ADDR tFlashAddr = {0};
    U32 ulData;
    U8  ucAddr;
    U32 aFeature[1] = { 0 };

    g_bSinglePln = FALSE;

    TEST_NfcEraseAll();

    TEST_NfcWriteAll(FULL_PAGE_WRITE);

    /* set feature to enable snap read: addr 0xF5, value 0x4-enable 0-default disable */
    ucAddr = 0xF5;
    ulData = 0x4;
    for (tFlashAddr.ucPU = 0; tFlashAddr.ucPU < SUBSYSTEM_PU_NUM; tFlashAddr.ucPU++)
    {
        HAL_NfcSetFeature(&tFlashAddr, ulData, ucAddr);
        DBG_Printf("Snap read set feature OK PU:%d Addr0x%x Data%d\n", tFlashAddr.ucPU, ucAddr, ulData);
    }
    /* get feature to confirm flash feature, optional */
    ucAddr = 0xF5;
    TEST_NfcGetFeature(&tFlashAddr, ucAddr);

    if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(tFlashAddr.ucPU, tFlashAddr.ucLun))
    {
        DBG_Printf("GetFeature Fail!\n");
    }
    else
    {
        HAL_DecSramGetFlashId(&tFlashAddr, aFeature);
    }
    DBG_Printf("PU %d Get Feature Address 0x%x Feature Value 0x%x!\n",tFlashAddr.ucPU, ucAddr, aFeature[0]);             

    /* only read first 8K */
    TEST_NfcReadAll(0, 8<<CW_INFO_SZ_BITS, FULL_PAGE_READ);

    return;
    
}

LOCAL void TEST_NfcSnapRead(void)
{   
    /* enable single plane EWR */
    g_bSinglePln = TRUE;

    TEST_NfcEraseAll();

    TEST_NfcWriteAll(FULL_PAGE_WRITE);

    /* enable snap read */
    g_bSnapRead = TRUE;
    
    /* only read first 8K */
    TEST_NfcReadAll(0, 8<<CW_INFO_SZ_BITS, SING_PLN_READ);

    return;
}

#if defined(FLASH_TLC) && !defined(FLASH_TSB_3D) && !defined(FLASH_INTEL_3DTLC)
/*------------------------------------------------------------------------------
Name: TEST_NfcTlcCopyAll
Description:
   Add delay as ms unit
Input Param:
    none
Output Param:
    none
Return Value:
    none
Usage:

History:
    20160111    abby    create
------------------------------------------------------------------------------*/
LOCAL void TEST_NfcTlcCopyAll(void)
{
    U16 usPrgIndex, usBlkIndex, usWlIndex;
    U32 ulStatus;
    U8 ucPU, ucLun;
    U8 ucOpSel;
    
    FLASH_ADDR tSrcSlcAddr[3] = {0};
    FLASH_ADDR tDesTlcAddr[3] = {0};
    FLASH_ADDR tDesSlcAddr = {0};
    FLASH_ADDR tAddr = {0};   //for data check

    NFC_PRG_REQ_DES  tWrReq = {0};      // For TLC PRG
    NFC_READ_REQ_DES tRdReq = {0};

    /*  Flash address allocation  */
    ucPU  = g_ucTestPuStart;
    while (ucPU < g_ucTestPuEnd)
    {
        ucLun = g_ucTestLunStart;
        while(ucLun < g_ucTestLunEnd)
        {
            /*  SRC SLC BLK 0/1/2; DES TLC BLK 3/4/5; DES SLC BLK 6   */
            for (usBlkIndex = 0; usBlkIndex < PGADDR_PER_PRG * INTRPG_PER_PGADDR; usBlkIndex++)
            {
                tSrcSlcAddr[usBlkIndex].ucPU = ucPU;
                tSrcSlcAddr[usBlkIndex].ucLun = ucLun;
                tSrcSlcAddr[usBlkIndex].usBlock = g_usTestBlkStart + usBlkIndex;
                
                tDesTlcAddr[usBlkIndex].ucPU    = ucPU;
                tDesTlcAddr[usBlkIndex].ucLun   = ucLun;
                tDesTlcAddr[usBlkIndex].usBlock = g_usTestBlkStart + PGADDR_PER_PRG * INTRPG_PER_PGADDR + usBlkIndex;
            }
            tDesSlcAddr.ucPU    = ucPU;
            tDesSlcAddr.ucLun   = ucLun;
            tDesSlcAddr.usBlock = tDesTlcAddr[2].usBlock + 1;

            /*  config request descriptor  */
            tRdReq.bsSecStart   = 0;
            tRdReq.bsSecLen     = SEC_PER_PIPE_PG;
            tRdReq.ppNfcRed     = (NFC_RED **)&pRRed[TEST_NfcGetRedOffset(&tSrcSlcAddr[0])];
            tRdReq.bsRdBuffId   = TEST_NfcGetRdBufId(&tSrcSlcAddr[0], 0);
            tRdReq.pErrInj      = NULL;
            
            /*  write req descriptor shared by PRG SRC SLC and DES TLC  */
            tWrReq.pNfcRed      = (NFC_RED *)pWrRed;
            tWrReq.pErrInj      = NULL;
            
            // Erase and PRG all SRC SLC BLK
            for (usBlkIndex = 0; usBlkIndex < PGADDR_PER_PRG * INTRPG_PER_PGADDR; usBlkIndex++)
            {
                //erase SRC SLC blk
                while(TRUE == HAL_NfcGetFull(tSrcSlcAddr[usBlkIndex].ucPU, tSrcSlcAddr[usBlkIndex].ucLun))
                {
                    ;
                }
                HAL_NfcFullBlockErase(&tSrcSlcAddr[usBlkIndex], TRUE);//SLC blk also use TLC erase
                HAL_NfcWaitStatus(ucPU, ucLun);
                //program whole blk
                for (usWlIndex = 0; usWlIndex < PG_PER_BLK; usWlIndex++)
                {
                    tSrcSlcAddr[usBlkIndex].usPage = usWlIndex;
                    tWrReq.bsWrBuffId = TEST_NfcGetWrBufId(&tSrcSlcAddr[usBlkIndex]);
                    tWrReq.bsTlcMode = FALSE;

                    //prepare data for SRC SLC blk
                    TEST_NfcPrepareRed((U32*)tWrReq.pNfcRed, &tSrcSlcAddr[usBlkIndex]);
                    TEST_NfcPrepareData(&tSrcSlcAddr[usBlkIndex]);

                    //prg SRC SLC blk
                    while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
                    {
                        ;
                    }

                    HAL_NfcFullPageWrite(&tSrcSlcAddr[usBlkIndex], &tWrReq);
                    if(NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
                    {
                        DBG_Printf("SRC SLC%d PU%d LUN %d BLK%d Program Fail!\n"
                        , usBlkIndex, tSrcSlcAddr[usBlkIndex].ucPU, tSrcSlcAddr[usBlkIndex].ucLun, tSrcSlcAddr[usBlkIndex].usBlock);
                        NFC_GETCH;
                    }
                    //DBG_Printf("BLK%d Page%d Erase Done!\n", tSrcSlcAddr[usBlkIndex].usBlock, tSrcSlcAddr[usBlkIndex].usPage);
                }
            }

            // Erase DES TLC and SLC BLK
            for (usBlkIndex = 0; usBlkIndex < PGADDR_PER_PRG * INTRPG_PER_PGADDR; usBlkIndex++)
            {
                //erase SRC SLC and DES TLC blk
                while(TRUE == HAL_NfcGetFull(tSrcSlcAddr[usBlkIndex].ucPU, tSrcSlcAddr[usBlkIndex].ucLun))
                {
                    ;
                }
                HAL_NfcFullBlockErase(&tDesTlcAddr[usBlkIndex], TRUE);//SLC blk also use TLC erase
                HAL_NfcWaitStatus(ucPU, ucLun);
            }   
            HAL_NfcFullBlockErase(&tDesSlcAddr, TRUE);
            HAL_NfcWaitStatus(ucPU, ucLun);
            
            // Copy operation
            for (ucOpSel = SLC_TO_SLC_EXT; ucOpSel < SLC_TO_TLC_INT_THREE_STAGE; ucOpSel++)
            {
                while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
                {
                    ;
                }
                switch (ucOpSel)
                {
                    /*    SLC copy to SLC externally    */
                    case SLC_TO_SLC_EXT :
                    {
                        for (usWlIndex = 0; usWlIndex < PG_PER_BLK; usWlIndex++)
                        {
                            tSrcSlcAddr[0].usPage = usWlIndex;
                            tDesSlcAddr.usPage = usWlIndex;
                            
                            while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
                            {
                                ;
                            }
                            ulStatus = HAL_SlcCopyToSlcExt(&tSrcSlcAddr[0], &tDesSlcAddr, &tRdReq, &tWrReq);

                            if (NFC_STATUS_SUCCESS != ulStatus)
                            {
                                HAL_NfcResetCmdQue(tDesSlcAddr.ucPU, tDesSlcAddr.ucLun);
                                HAL_NfcClearINTSts(tDesSlcAddr.ucPU, tDesSlcAddr.ucLun);
                                DBG_Printf("Src Pu %d Src LUN %d Src BLK%d Src PG%d Copy to Des BLK%d Des PG%d Fail!\n"
                                ,tSrcSlcAddr[0].ucPU, tSrcSlcAddr[0].ucLun,tSrcSlcAddr[0].usBlock,tSrcSlcAddr[0].usPage, tDesSlcAddr.usBlock, tDesSlcAddr.usPage);
                                NFC_GETCH;
                            }
                        }
                        DBG_Printf("PU %d LUN %d SLC BLK%d Copy to SLC BLK%d External OK!\n", ucPU, ucLun, tSrcSlcAddr[0].usBlock, tDesSlcAddr.usBlock);
                    }break;

                    /*    SLC copy to SLC internally    */
                    case SLC_TO_SLC_INT :
                    {
                        for (usWlIndex = 0; usWlIndex < PG_PER_BLK; usWlIndex++)
                        {
                            tRdReq.ppNfcRed = (NFC_RED **)&pRRed[TEST_NfcGetRedOffset(&tSrcSlcAddr[0])];
                            tRdReq.bsRdBuffId = TEST_NfcGetRdBufId(&tSrcSlcAddr[0], 0);

                            tSrcSlcAddr[0].usPage = usWlIndex;
                            tDesSlcAddr.usPage = usWlIndex;
                            
                            while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
                            {
                                ;
                            }
                            ulStatus = HAL_SlcCopyToSlcInt(&tSrcSlcAddr[0], &tDesSlcAddr, &tRdReq, &tWrReq);

                            if (NFC_STATUS_SUCCESS != ulStatus)
                            {
                                HAL_NfcResetCmdQue(tDesSlcAddr.ucPU, tDesSlcAddr.ucLun);
                                HAL_NfcClearINTSts(tDesSlcAddr.ucPU, tDesSlcAddr.ucLun);
                                DBG_Printf("Src Pu %d Src LUN %d Src BLK%d Src PG%d Copy to Des BLK%d Des PG%d Fail!Err Code = %d\n"
                                    , tSrcSlcAddr[0].ucPU, tSrcSlcAddr[0].ucLun, tSrcSlcAddr[0].usBlock, tSrcSlcAddr[0].usPage, tDesSlcAddr.usBlock, tDesSlcAddr.usPage, HAL_NfcGetErrCode(tDesSlcAddr.ucPU, tDesSlcAddr.ucLun));
                                NFC_GETCH;
                            }
                        }
                        DBG_Printf("PU %d LUN %d SLC BLK%d Copy to SLC BLK%d Internal OK!\n", ucPU, ucLun, tSrcSlcAddr[0].usBlock, tDesSlcAddr.usBlock);
                    }break;

                    /*    SLC copy to TLC externally    */
                    case SLC_TO_TLC_EXT :
                    {
                        ulStatus = HAL_SlcCopyToTlcExt(tSrcSlcAddr, &tDesTlcAddr[0], &tRdReq, &tWrReq);
                        if (NFC_STATUS_SUCCESS != ulStatus)
                        {
                            HAL_NfcResetCmdQue(ucPU, ucLun);
                            HAL_NfcClearINTSts(ucPU, ucLun);
                            DBG_Printf("Src SLC BLK%d-%d Copy to TLC BLK%d Externally Fail!\n",tSrcSlcAddr[0].usBlock,tSrcSlcAddr[2].usBlock,tDesTlcAddr[0].usBlock);
                            DBG_Getch();
                        }
                        DBG_Printf("PU %d LUN %d SLC BLK%d-%d Copy to TLC BLK%d External OK!\n", ucPU, ucLun, tSrcSlcAddr[0].usBlock, tSrcSlcAddr[2].usBlock, tDesTlcAddr[0].usBlock);
                    }break;

                    /*    SLC copy to TLC internally    */
                    case SLC_TO_TLC_INT_ONE_STAGE:
                    {
                        for (usPrgIndex = 0; usPrgIndex < PG_PER_BLK * PGADDR_PER_PRG * INTRPG_PER_PGADDR; usPrgIndex++)
                        {
                            tWrReq.bsTlcPgCycle   = HAL_FlashGetTlcPrgCycle(usPrgIndex);
                            tDesTlcAddr[1].usPage = HAL_FlashGetTlcPrgWL(usPrgIndex);

                            tSrcSlcAddr[0].usPage = tDesTlcAddr[1].usPage;
                            tSrcSlcAddr[1].usPage = tDesTlcAddr[1].usPage;
                            tSrcSlcAddr[2].usPage = tDesTlcAddr[1].usPage;

                            while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
                            {
                                ;
                            }

                            HAL_SlcCopyToTlcIntOnce(tSrcSlcAddr, &tDesTlcAddr[1], &tRdReq, &tWrReq);

                            if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
                            {
                                HAL_NfcResetCmdQue(ucPU, ucLun);
                                HAL_NfcClearINTSts(ucPU, ucLun);
                                DBG_Printf("Src SLC BLK%d-%d Copy to TLC BLK%d WL %d Internally Fail!\n",tSrcSlcAddr[0].usBlock,tSrcSlcAddr[2].usBlock,tDesTlcAddr[1].usBlock,tDesTlcAddr[1].usPage);
                                DBG_Getch();
                            }
                        }
                        DBG_Printf("PU %d LUN %d SLC BLK%d-%d Copy to TLC BLK%d Internal in 1 stage OK!\n", ucPU, ucLun, tSrcSlcAddr[0].usBlock, tSrcSlcAddr[2].usBlock, tDesTlcAddr[1].usBlock);
                    }break;
                    
                #ifdef TRI_STAGE_COPY
                    /*    SLC copy to TLC internally    */
                    case SLC_TO_TLC_INT_THREE_STAGE:
                    {
                        U8 ucPageType;
                        for (usPrgIndex = 0; usPrgIndex < PG_PER_BLK * PRG_CYC_CNT; usPrgIndex++)
                        {
                            tWrReq.bsTlcPgCycle = HAL_FlashGetTlcPrgCycle(usPrgIndex);
                            tDesTlcAddr[2].usPage  = HAL_FlashGetTlcPrgWL(usPrgIndex);

                            for(ucPageType = 0; ucPageType < PGADDR_PER_PRG * INTRPG_PER_PGADDR; ucPageType++)
                            {
                                tWrReq.bsTlcPgType = ucPageType;
                                tSrcSlcAddr[ucPageType].usPage = tDesTlcAddr[2].usPage;
                                
                                while(TRUE == HAL_NfcGetFull(ucPU, ucLun))
                                {
                                    ;
                                }
                                HAL_SlcCopyToTlcInt(&tSrcSlcAddr[ucPageType], &tDesTlcAddr[2], &tRdReq, &tWrReq);
                                if (NFC_STATUS_SUCCESS != HAL_NfcWaitStatus(ucPU, ucLun))
                                {
                                    HAL_NfcResetCmdQue(ucPU, ucLun);
                                    HAL_NfcClearINTSts(ucPU, ucLun);
                                    DBG_Printf("Src SLC BLK%d-%d Copy to TLC BLK%d WL%d Internally Fail!\n",tSrcSlcAddr[ucPageType].usBlock,tSrcSlcAddr[2].usBlock,tDesTlcAddr[2].usBlock,tDesTlcAddr[2].usPage);
                                    DBG_Getch();
                                }
                                //DBG_Printf("Src SLC BLK%d WL%d Copy to TLC BLK%d WL%d PageType%d Times %d Internally OK!\n",tSrcSlcAddr[ucPageType].usBlock,tSrcSlcAddr[ucPageType].usPage
                                //            ,tDesTlcAddr[2].usBlock,tDesTlcAddr[2].usPage,ucPageType,tWrReq.bsTlcPgCycle);
                            }
                        }
                        DBG_Printf("PU %d LUN %d SLC BLK%d-%d Copy to TLC BLK%d Internal in 3 stage OK!\n", ucPU, ucLun, tSrcSlcAddr[0].usBlock, tSrcSlcAddr[2].usBlock, tDesTlcAddr[2].usBlock);
                    }break;
                #endif
                
                    default :
                    {
                        DBG_Getch();
                    }

                }
                // Data Check
                #ifdef DATA_CHK
                    // read DES BLK data
                    for(usWlIndex = 0; usWlIndex < PG_PER_BLK; usWlIndex++)
                    {
                        switch (ucOpSel)
                        {
                            case SLC_TO_SLC_EXT :
                            case SLC_TO_SLC_INT :
                            {
                                tAddr = tDesSlcAddr;
                                tAddr.usPage  = usWlIndex;
                                tRdReq.bsTlcMode = FALSE;
                            }
                            break;

                            case SLC_TO_TLC_EXT:
                            {
                                tAddr = tDesTlcAddr[0];
                                tAddr.usPage  = usWlIndex;
                                tRdReq.bsTlcMode = TRUE;
                            }break;
                            
                            case SLC_TO_TLC_INT_ONE_STAGE:
                            {
                                tAddr = tDesTlcAddr[1];
                                tAddr.usPage  = usWlIndex;
                                tRdReq.bsTlcMode = TRUE;
                            }break;
                            
                            case SLC_TO_TLC_INT_THREE_STAGE:
                            {
                                tAddr = tDesTlcAddr[2];
                                tAddr.usPage  = usWlIndex;
                                tRdReq.bsTlcMode = TRUE;
                            }
                            break;
                        }//end switch

                        tRdReq.bsRdBuffId   = TEST_NfcGetRdBufId(&tSrcSlcAddr[0], 0);
                        tRdReq.ppNfcRed     = (NFC_RED **)&pRRed[TEST_NfcGetRedOffset(&tAddr)];

                        HAL_NfcPageRead(&tAddr, &tRdReq);
                        ulStatus = HAL_NfcWaitStatus(ucPU, ucLun);

                        if (NFC_STATUS_SUCCESS != ulStatus)
                        {
                            DBG_Printf("Des SLC BLK%d Des PG%d Read Fail!Err code = %d\n", tAddr.usBlock, tAddr.usPage, HAL_NfcGetErrCode(ucPU, ucLun));
                            HAL_NfcResetCmdQue(ucPU, ucLun);
                            HAL_NfcClearINTSts(ucPU, ucLun);
                            NFC_GETCH;
                        }
                        tSrcSlcAddr[0].usPage = usWlIndex;
                        TEST_NfcCheckRed(&tSrcSlcAddr[0], tRdReq.ppNfcRed, 0, SEC_PER_PIPE_PG, 0);
                        TEST_NfcCheckData(&tSrcSlcAddr[0], 0, SEC_PER_PIPE_PG, tRdReq.bsRdBuffId, 0);
                    }
                    DBG_Printf("Copy Data Check OK!\n");
                #endif
            }
            ucLun++;
        }
        ucPU++;
    }
    return;
}

#endif //#ifdef FLASH_TLC

/****************************************************************************
Function      : TEST_NfcReadStatus
Input         :
Output        :

Description   : To test multi PU EWR and insert cmd case
Reference     :
History       :
    20160426    abby    create
****************************************************************************/
LOCAL void TEST_NfcReadStatus(void)
{
    FLASH_ADDR tFlashAddr = {0};

    tFlashAddr.ucPU = g_ucTestPuStart;
    tFlashAddr.ucLun = g_ucTestLunStart;
    tFlashAddr.usBlock = g_usTestBlkStart;
    tFlashAddr.usPage = g_usTestPageStart;
    tFlashAddr.bsSLCMode = !g_bTlcMode;

    for (tFlashAddr.bsPln = 0; tFlashAddr.bsPln < PLN_PER_LUN; tFlashAddr.bsPln++)
    {
        HAL_NfcSinglePlnReadSts(&tFlashAddr);
    }
}

/*------------------------------------------------------------------------------
Name: TEST_LdpcDlMat
Description:
    Just check LDPC matrix download, not normal pattern
Input Param:
Output Param:
    none
Return Value:
Usage:
History:
    20151105   abby    create
------------------------------------------------------------------------------*/
LOCAL void TEST_LdpcDlMat(void)
{
    U32 ulCodesel;

    HAL_LdpcDownloadHMatix();
    //TEST_NfcDSInit(LDPC_CODE_FST_15K);
    
    /*   Basic EWR test */
    TEST_NfcSinglePlnEWR();
    DBG_Printf("LDPC Download matrix %d done!\n", ulCodesel);

    return;
}

/****************************************************************************
Function      : TEST_NfcPatternSel
Input         :
Output        :

Description   : Sel a start pattern to do NFC UT test
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
void TEST_NfcPatternSel(void)
{ 
    NFC_ERR_INJ tErrTmp = { 0 };

    /* init NFC feature as default value */
    g_bSinglePln    = FALSE;
    g_bSsuEn        = FALSE;
    g_bCacheStsEn   = FALSE;
    g_bSsu0DramEn   = FALSE;
    g_bSsu1DramEn   = FALSE;
    g_bRedOntf      = TRUE;
    g_bLbaChk       = FALSE;
    g_bForceRetryEn = FALSE;
    g_bEmEnable     = FALSE;
    
    //err injection
    g_ErrInjEn      = FALSE;
    g_tErrInj       = tErrTmp;
    g_ucEntryIndex  = 0;
    //DEC FIFO status report
    g_bDecFifoEn    = FALSE;

    /* Test mode */
#ifdef DRAMLESS_ENABLE //only update SSU and RED to OTFB
    g_bRedOntf    = TRUE;
    g_bSsu0DramEn = FALSE;
    g_bSsu1DramEn = FALSE;
#endif

#ifdef TLC_MODE_TEST
    g_bTlcMode = TRUE;
#else
    g_bTlcMode = FALSE;
#endif

    /* Test pattern select */
    g_ulPattStart = P_SINGLE_PLN;

    g_ulPattEnd = P_ERR_INJ_DEC_REP;
    
    //DBG_Printf("\nTest Pattern from ID %d - ID %d, g_bTlcMode = %d\n\n"
    //,g_ulPattStart, g_ulPattEnd, g_bTlcMode);

    return;

}

void TEST_EMInit(void)
{
    LOCAL U8 ucEmKeySize = 0;
    LOCAL U8 ucEmModeSel = 0;
    U8 a_ucEmMode[6] = {EM_MODE_XTS, EM_MODE_CBC_ESSIV, EM_MODE_CTR_ESSIV, EM_MODE_ECB, EM_MODE_CBC, EM_MODE_CTR};
    U32 aIV[4] = {0x12121212, 0x34343434, 0x56565656, 0x78787878};
    
    while (ucEmModeSel <= 6)
    {
        while (ucEmKeySize <= EM_KEY_SIZE_256BIT)
        {
            /* Change EM engine with KeySize & Mode setting */
            HAL_EMInit(ucEmKeySize, a_ucEmMode[ucEmModeSel], aIV);
            
            DBG_Printf("EM TEST: key size = %d, mode = %d\n", ucEmKeySize, a_ucEmMode[ucEmModeSel]);
            
            ucEmKeySize++;
        
            return;
        }
        ucEmModeSel++;
        ucEmKeySize = EM_KEY_SIZE_128BIT;
    }
#if 0
    /* Select a EM Mode */
    switch (ucEmModeSel)
    {
        case 0:
        {
            ucEmModeSel = EM_MODE_XTS;
        }
            break;

        case EM_MODE_XTS:
        {
            ucEmModeSel = EM_MODE_CBC_ESSIV;
        }
            break;

        case EM_MODE_CBC_ESSIV:
        {
            ucEmModeSel = EM_MODE_CTR_ESSIV;
        }
            break;

        case EM_MODE_CTR_ESSIV:
        {
            ucEmModeSel = EM_MODE_ECB;
        }
            break;

        case EM_MODE_ECB:
        {
            ucEmModeSel = EM_MODE_CBC;
        }
            break;

        case EM_MODE_CBC:
        {
            ucEmModeSel = EM_MODE_CTR;
        }
            break;

        case EM_MODE_CTR:
        {
            ucEmModeSel = 1;
        }
            break;

        default:
        {
            DBG_Printf("Wrong EM Mode, ModeSel=%d!", ucEmModeSel);
            DBG_Getch();
        }
    }
#endif

    /* Enable NFC EM Function  */
    //g_bEmEnable = TRUE;
}

/****************************************************************************
Function      : TEST_NfcBasicTestPattern
Input         :
                U32 ulBurnInLoop: loop of burn in all pattern
Output        :

Description   : The body of multi test case
Reference     :
History       :
    20160808    abby    create
****************************************************************************/
LOCAL void TEST_NfcBasicTestPattern(U8 ucPattId)
{   
    U8 ucPattSel;
    LOCAL U32 l_ulLoop = 0;
    
    ucPattSel = ucPattId;        //next pattern ID sel
    ASSERT(ucPattSel < P_TYPE_CNT);

#if 0//def DATA_EM_ENABLE
    /* change a key */
    if (ucPattSel == P_SINGLE_PLN)
    {
        TEST_EMInit();
    }
#endif

    switch (ucPattSel)
    {
        case P_SINGLE_PLN :
        {
            TEST_NfcSinglePlnEWR();
            DBG_Printf("Single Plane EWR Pass!\n\n");
        }break;

        case P_MULTI_PLN :
        {
            TEST_NfcMultiPlnEWR();
            DBG_Printf("Multi Plane EWR Pass!\n\n");
        }break;

        case P_PART_READ :
        {
            TEST_NfcPartialRead();
            DBG_Printf("Partial Read Pass!\n\n");
        }break;
        
        case P_SING_PLN_CCL_READ :
        {
            TEST_NfcSinglePlnCCLRead();
            DBG_Printf("Single Plane Change Column Read Pass!\n\n");
        }break;

        case P_CHANGE_COL_READ :
        {
            TEST_NfcChangeColumnRead();
            DBG_Printf("Change Column Read Pass!\n\n");
        }break;
    
        case P_RED_ONLY_READ:
        {
            TEST_NfcRedOnlyRead();
            DBG_Printf("RED Only Read Pass!\n\n");
        }break;
        
        case P_SSU_CS :
        {
            TEST_NfcSSUandCS();
            DBG_Printf("SSU & Cache Status Check Pass!\n\n");
        }break;

        case P_SSU_UPDATE :
        {
            TEST_NfcSSUDynamUpdate();
            DBG_Printf("SSU Dynamically Update to DRAM Check Pass!\n\n");
        }break;

        case P_RED_UPDATE :
        {
            TEST_NfcRedDynamUpdate();
            DBG_Printf("Red Dynamically Update to DRAM or OTFB Pass!\n\n");
        }break;
        
        case P_ERR_INJ_DEC_REP ://error injection and dec report
        {
            TEST_NfcErrInjDecRep();
            DBG_Printf("Err Inj and DEC STS Report Pass!\n\n");
        }break;
        
        case P_RETRY :
        {
            TEST_NfcReadRetry();
            DBG_Printf("Read Retry Pass!\n\n");
        }break;  

        case P_PU_BITMAP :
        {
            TEST_NfcPuBitMap();
            DBG_Printf("PU Status BitMap Pass!\n\n");
        }break;

        case P_LUN_BITMAP :
        {
            TEST_NfcLunBitMap();
            DBG_Printf("LUN Status BitMap Pass!\n\n");
        }break;
        
        case P_READ_STS :
        {
            TEST_NfcReadStatus();
            DBG_Printf("READ STS Pass!\n\n");
        }break;

        
    /*   TSB 2D TLC Copy   */
    #if (defined(FLASH_TLC) && defined(TLC_MODE_TEST) && !defined(FLASH_TSB_3D)) && !defined(FLASH_INTEL_3DTLC)
        case P_TLC_COPY :
        {
            TEST_NfcTlcCopyAll();
            DBG_Printf("Tlc COPY BACK Pass!\n\n");
        }break;
    #endif

        default:
        {
            DBG_Printf("Not support pattern ID %d\n", ucPattSel);
            DBG_Getch();
        }
    }//end switch
    
    /* recycle pattern Q */
    TEST_NfcPattQRecycleEntry();
    
    DBG_Printf("NFC HAL TEST PASS %d BASIC PATT!\n\n",l_ulLoop++);
}

void TEST_NfcBasicPattConfig(BASIC_PATT_ENTRY *pBasicPatt)
{
    g_bTlcMode          = pBasicPatt->tPattFeature.bsTlcMode;
    g_bEmEnable         = pBasicPatt->tPattFeature.bsEMEnable;
    
    g_ucTestPuStart     = pBasicPatt->tPattFeature.bsPuStart;
    g_ucTestPuEnd       = pBasicPatt->tPattFeature.bsPuEnd;
    g_ucTestLunStart    = pBasicPatt->tPattFeature.bsLunStart;

    g_ucTestLunEnd      = pBasicPatt->tPattFeature.bsLunEnd;
    g_usTestBlkStart    = pBasicPatt->tPattFeature.bsBlkStart;
    g_usTestBlkEnd      = pBasicPatt->tPattFeature.bsBlkEnd;
    g_usTestPageStart   = pBasicPatt->tPattFeature.bsPageStart;
    g_usTestPageEnd     = pBasicPatt->tPattFeature.bsPageEnd;
    g_usTestPageEnd     = (0xFF == g_usTestPageEnd) ? TEST_PG_PER_BLK : g_usTestPageEnd;
    g_bSsuEn            = pBasicPatt->tPattFeature.bsSsuEn;
    g_bCacheStsEn       = pBasicPatt->tPattFeature.bsCacheStsEn;
    g_bSsu0DramEn       = pBasicPatt->tPattFeature.bsSsu0DramEn;
    g_bSsu1DramEn       = pBasicPatt->tPattFeature.bsSsu1DramEn;
    g_bLbaChk           = pBasicPatt->tPattFeature.bsLbaChk;
    g_bRedOntf          = !pBasicPatt->tPattFeature.bsRedDramEn;
    g_bDecFifoEn        = pBasicPatt->tPattFeature.bsDecFifoEn;
    g_bSinglePln        = pBasicPatt->tPattFeature.bsSinglePln;
    g_bSnapRead         = pBasicPatt->tPattFeature.bsSnapRead;
    g_bRawDataRead      = pBasicPatt->tPattFeature.bsRawDataRead;
    g_ErrInjEn          = pBasicPatt->tPattFeature.bsErrInjEn;
    g_tErrInj           = pBasicPatt->tPattFeature.tErrInj;
}

void TEST_NfcBasicPattRun(void)
{
    U32 ulBurnLoop = 1;
    BASIC_PATT_ENTRY *pBasicPatt;

    //TEST_NfcReadID();   //option operation

    while (FALSE == TEST_NfcPattQIsPushed())
    {
        ;
    }
   
    pBasicPatt = TEST_NfcPattQPopEntry();
    
    TEST_NfcBasicPattConfig(pBasicPatt);
    
    TEST_NfcBasicTestPattern(pBasicPatt->tPattFeature.bsPatternId);

    return;
}

/*    end of this file    */

