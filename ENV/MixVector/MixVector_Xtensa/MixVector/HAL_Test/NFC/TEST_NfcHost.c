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
 * File Name    : TEST_NfcHost.c
 * Discription  : this document is the tesp pattern file for HostRequest.
 * CreateAuthor : tobey
 * CreateDate   : 2013.11.7
 *===============================================================================
 * Modify Record:
 *=============================================================================*/

/*============================================================================*/
/* #include region: include std lib & other head file                         */
/*============================================================================*/
//#include <Windows.h>
#include "TEST_NfcMain.h"

#if defined(TEST_PAGE_NUM) && defined(COSIM)
#undef TEST_PAGE_NUM
#define TEST_PAGE_NUM  (rNFC(0x2b0))
#endif

#define HOST_MEM_SIZE                   (50*1024*1024)
#define HOST_MEM_START_FOR_FLASH        (200*1024*1024)         
#define HOST_MEM_START_FOR_MCU1DXQ      (HOST_MEM_START_FOR_FLASH + HOST_MEM_SIZE)
#define HOST_MEM_START_FOR_MCU2DXQ      (HOST_MEM_START_FOR_MCU1DXQ + HOST_MEM_SIZE)

enum
{
    HMEM_MCU1DXQ,
    HMEM_MCU2DXQ,
    HMEM_FLASH
};

LOCAL U32 l_aHostMemStart[] = {HOST_MEM_START_FOR_MCU1DXQ
                            ,HOST_MEM_START_FOR_MCU2DXQ
                            ,HOST_MEM_START_FOR_FLASH};

LOCAL U32 l_aDevMemStart[] =  {DRAM_DATA_BUFF_MCU1_BASE
                            ,DRAM_DATA_BUFF_MCU2_BASE};

LOCAL U32 l_ulMcuSel;
LOCAL U32 l_aTagOff[] = {16,21,26};
LOCAL U32 l_ulDxqEn;
//#define HOST_MEM_SIZE_PIPE_PAGE_NUM (HOST_MEM_SIZE / (32 * 1024))

LOCAL U32 GetRandOffset()
{
    U32 ulRand = rand();
    return ((ulRand % (HOST_MEM_SIZE >> PIPE_PG_SZ_BITS))<<PIPE_PG_SZ_BITS);
}

LOCAL U32 GetTag(U32 ulIndex)
{
    U32 ulRand = rand();    
    return (ulRand % 5) + l_aTagOff[ulIndex];
}

LOCAL U16 BulidHsg(U32 ulIndex)
{
    U16 usHsgId;
    U16 usSecLen = (U16)TEST_NfcGetSecLen();
    HSG_ENTRY *pHsg;
    U32 ulHMemStart = l_aHostMemStart[ulIndex];
    
    while (FALSE == HAL_GetHsg(&usHsgId))
    {
        ;
    }
    pHsg = (HSG_ENTRY *)HAL_GetHsgAddr(usHsgId);
    COM_MemZero((U32*)pHsg,sizeof(HSG_ENTRY)/sizeof(U32));
    pHsg->bsLast = TRUE;
    pHsg->bsLength = usSecLen << SEC_SIZE_BITS;

    pHsg->ulHostAddrHigh = rP0_CLBU;
#ifndef XTMP
    pHsg->ulHostAddrLow  = rP0_CLB + ulHMemStart + GetRandOffset();
#else
    pHsg->ulHostAddrLow = rP0_CLB;
#endif
    HAL_SetHsgSts(usHsgId,TRUE);
    return usHsgId;
}

LOCAL U16 BulidDsg(U32 ulIndex)
{
    U16 usDsgId;
    U16 usSecLen = (U16)TEST_NfcGetSecLen();
    NORMAL_DSG_ENTRY *pDsg;
    U32 ulDMemStart = l_aDevMemStart[ulIndex];

    while (FALSE == HAL_GetNormalDsg(&usDsgId))
    {
        ;
    }
    pDsg = (NORMAL_DSG_ENTRY *) HAL_GetNormalDsgAddr(usDsgId);
    COM_MemZero((U32*)pDsg,sizeof(NORMAL_DSG_ENTRY)/sizeof(U32));
    pDsg->bsLast = TRUE;
    pDsg->bsXferByteLen = usSecLen << SEC_SIZE_BITS;
    pDsg->ulDramAddr = ulDMemStart + GetRandOffset();
    HAL_SetNormalDsgSts(usDsgId,TRUE);

    return usDsgId;
}
void TEST_BuildDwq()
{
    U16 usHsgId = BulidHsg(l_ulMcuSel);
    U16 usDsgId = BulidDsg(l_ulMcuSel);
    HAL_DwqBuildEntry(GetTag(l_ulMcuSel),usHsgId,usDsgId);
}

void TEST_BuildDrq()
{
    U16 usHsgId = BulidHsg(l_ulMcuSel);
    U16 usDsgId = BulidDsg(l_ulMcuSel);
    HAL_DrqBuildEntry(GetTag(l_ulMcuSel),usHsgId,usDsgId);
}

void TEST_NfcMultiPuOtfbRead(void)
{

    U8 ucTestPuNum = (U8)TEST_NfcGetPuNum();
    U16 usSecLen = (U16)TEST_NfcGetSecLen();
    U32 ulTestPageNum = TEST_NfcGetPageNum();
    U16 usHsgId;
    FLASH_ADDR tFlashAddr = {0};
    
    for (tFlashAddr.usPage=0;tFlashAddr.usPage<ulTestPageNum;tFlashAddr.usPage++)
    {
        for (tFlashAddr.ucPU=0;tFlashAddr.ucPU<ucTestPuNum;tFlashAddr.ucPU++)
        {
            usHsgId = BulidHsg(HMEM_FLASH);
            HAL_NfcHostPageRead(&tFlashAddr,GetTag(HMEM_FLASH),usHsgId,0,usSecLen,NULL);
        }
        if (TRUE == l_ulDxqEn)
        {
            TEST_BuildDwq();
            TEST_BuildDrq();
        }
    }    
}


void TEST_MixPatternMCU12()
{
    l_ulMcuSel = HAL_GetMcuId() - MCU1_ID;  
    l_ulDxqEn = TEST_NfcDxqEn();
    srand((U32)__TIME__);

#ifndef XTMP
    // Waiting for hardware linking completed
    while (1 != (rP0_CMD & 0x1))
    {
        ;
    }
#endif

    if (0 == l_ulMcuSel) // MCU1
    {
        TEST_NfcMultiPuOtfbRead();
    }
    else                 // MCU2 
    {
        TEST_BuildDwq();
        TEST_BuildDrq();        
    }    
}

/*====================End of this file========================================*/

