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
Filename    : TEST_DMAE.c
Version     : Ver 1.0
Author      : Gavin
Date        : 2014.11.04
Description :
Others      : 
Modify      :
20141104    Gavin     Create file
****************************************************************************/
#include "HAL_Dmae.h"
#include "HAL_GLBReg.h"
#include "HAL_Xtensa.h"
#ifdef SIM
#include "sim_DMAE.h"
#endif

#define TEST_DRAM_OTFB_DATAXFR
#define TEST_DRAM_OTFB_SETVALUE
#define TEST_MCU_SRAM_DATAXFR
#define TEST_MCU_SRAM_SETVALUE

#define TEST_DMAE_CHECK_NUM  16

typedef enum _test_addr_type{
    TEST_DMAE_TYPE_DRAM = 0,
    TEST_DMAE_TYPE_OTFB,
    TEST_DMAE_TYPE_MCUSRAM,
    TEST_ADDR_TYPE_NUM
}TEST_ADDR_TYPE;

typedef enum _test_len_index{
    LEN_INDEX_16B = 0,
    LEN_INDEX_128B,
    LEN_INDEX_512B,
    LEN_INDEX_1K,
    LEN_INDEX_4K,
    LEN_INDEX_16K,
    LEN_INDEX_32K,
    LEN_INDEX_64K,
    LEN_INDEX_128K,
    LEN_INDEX_NUM
}TEST_LEN_INDEX;

LOCAL U32 l_aDmaeTestLenByte[LEN_INDEX_NUM];
LOCAL U32 l_aDmaeSrcAddr[TEST_ADDR_TYPE_NUM];
LOCAL U32 l_aDmaeDesAddr[TEST_ADDR_TYPE_NUM];

LOCAL U32 l_ulTimer;

void TEST_DMAEInit(void)
{
    l_ulTimer = 0;

    l_aDmaeTestLenByte[LEN_INDEX_16B] = 16;
    l_aDmaeTestLenByte[LEN_INDEX_128B] = 128;
    l_aDmaeTestLenByte[LEN_INDEX_512B] = 512;
    l_aDmaeTestLenByte[LEN_INDEX_1K] = 1024;
    l_aDmaeTestLenByte[LEN_INDEX_4K] = 4 * 1024;
    l_aDmaeTestLenByte[LEN_INDEX_16K] = 16 * 1024;
    l_aDmaeTestLenByte[LEN_INDEX_32K] = 32 * 1024;
    l_aDmaeTestLenByte[LEN_INDEX_64K] = 64 * 1024;
    l_aDmaeTestLenByte[LEN_INDEX_128K] = 128 * 1024;

    l_aDmaeSrcAddr[TEST_DMAE_TYPE_DRAM] = DRAM_START_ADDRESS;
    l_aDmaeSrcAddr[TEST_DMAE_TYPE_OTFB] = OTFB_START_ADDRESS; 
    l_aDmaeSrcAddr[TEST_DMAE_TYPE_MCUSRAM] = DSRAM1_MCU01_SHARE_BASE;
    
    l_aDmaeDesAddr[TEST_DMAE_TYPE_DRAM] = DRAM_START_ADDRESS + l_aDmaeTestLenByte[LEN_INDEX_128K];
    l_aDmaeDesAddr[TEST_DMAE_TYPE_OTFB] = OTFB_START_ADDRESS + l_aDmaeTestLenByte[LEN_INDEX_128K]; 
    l_aDmaeDesAddr[TEST_DMAE_TYPE_MCUSRAM] = DSRAM1_MCU01_SHARE_BASE;

    HAL_DMAEInit();
}

#if 0
/********************* SPI DMAE ***********************/

void HAL_SpiEnable(void)
{
    GLB_IO_ENABLE *pGLB68 = (GLB_IO_ENABLE *)(0x1ff80000 + 0x68);
    pGLB68->RSPI_NEW = 1;
    pGLB68->RNEWSPI_MODE = 2;
    pGLB68->RSPI_EN = 1;
    pGLB68->RSPI_DIS = 0;
    pGLB68->RSPI_CKG_EN = 0;
    
    //*(volatile U32 *)(0x1ff80040) |= 1<<26;
    
    rGLB_40 = 0x02000000;
}

void HAL_SpiSetDmaeWR(void)
{
    rGLB_40 &= (~(1<<26));
}

void HAL_SpiSetMcuWR(void)
{
    rGLB_40 |= 1<<26;
}

void TEST_DMAESpiXfer(void)
{
    U32 ulBlkLen = 64 * 1024;//64KB
    U32 ulBlkID;
    U32 ulBlkNum;
    U8 ucSrcType;
    U8 ucDesType;
    U32 ulSrcAddr;
    U32 ulDesAddr;
    U8 ucLenIndex;
    U32 ulLenByte;
    U8 *pSrcByte;
    U8 *pDesByte;
    U32 ulDataIncByte;
    U32 ulSrcData;
    U32 *pDesDW;
    U32 i;

#if 0
    /*******************DRAM/OTFB <-> SPI********************/
    for ( ucSrcType = TEST_DMAE_TYPE_DRAM; ucSrcType <= TEST_DMAE_TYPE_PIF; ucSrcType++ )
    {
        ulSrcAddr = g_aDmaeSrcAddr[ucSrcType];
        ulDesAddr = SPI_START_ADDRESS;
        
        for ( ucLenIndex = 5; ucLenIndex < 9; ucLenIndex++ )
        {
            ulLenByte = g_aDmaeTestLenByte[ucLenIndex];
            
            /*Init source data*/
            pSrcByte = (U8 *)ulSrcAddr;
            ulDataIncByte= ulLenByte/TEST_DMAE_CHECK_NUM ;
            for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
            {
                *pSrcByte = i+1;
                pSrcByte += ulDataIncByte;
            }
            
            /*Erase SPI*/
            ulBlkID = ( ulDesAddr - SPI_START_ADDRESS ) / ulBlkLen;
            ulBlkNum = ulLenByte / ulBlkLen + 1;
            HAL_SpiBlkErase( ulBlkID, ulBlkNum );
            
            /*Xfer,use DMAE*/
            HAL_SpiSetDmaeWR();
            HAL_SpiBurstWrite( (U32 *)ulDesAddr, (U32 *)ulSrcAddr, ulLenByte/sizeof(U32), TRUE);
            
            /*data check*/
            HAL_SpiSetMcuWR();
            pDesByte = (U8 *)ulDesAddr;
            for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
            {
                if ( (i+1) != *pDesByte )
                {                        
                    rTracer = 0x12345678;
                    DBG_Getch();
                }
                pDesByte += ulDataIncByte;
            }
            rTracer = (ucSrcType + 1) * 0x1000 + 0x400 + ucLenIndex;
            
            /*update address*/
            if ( TEST_DMAE_TYPE_DRAM == ucSrcType )
            {
                ulSrcAddr += ulLenByte;
            }
            ulDesAddr += ulBlkNum * ulBlkLen;
        }//for ( ucLenIndex = 0; ucLenIndex < 9; ucLenIndex++ )
    }//for ( ucSrcType = TEST_DMAE_TYPE_DRAM; ucSrcType <= TEST_DMAE_TYPE_OTFB; ucSrcType++ )
#endif
    
    /*******************SPI <-> DRAM/OTFB********************/
    /*Erase SPI*/
    ulBlkID = 0;
    ulBlkNum = 9;    
    HAL_SpiBlkErase( ulBlkID, ulBlkNum );
    
    for ( ucDesType = TEST_DMAE_TYPE_DRAM; ucDesType <= TEST_DMAE_TYPE_OTFB; ucDesType++ )
    {
        if ( TEST_DMAE_TYPE_DRAM == ucDesType )
        {
            continue;
        }
        
        ulDesAddr = g_aDmaeSrcAddr[ucDesType];
        ulSrcAddr = SPI_START_ADDRESS;
        
        for ( ucLenIndex = 5; ucLenIndex < 9; ucLenIndex++ )
        {
            ulLenByte = g_aDmaeTestLenByte[ucLenIndex];
            
            /*Init source data*/
            HAL_SpiSetMcuWR();
            pSrcByte = (U8 *)DRAM_START_ADDRESS;
            ulDataIncByte= ulLenByte/TEST_DMAE_CHECK_NUM ;
            for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
            {
                *pSrcByte = i+1;
                pSrcByte += ulDataIncByte;
            }
            pSrcByte = (U8 *)DRAM_START_ADDRESS;
            HAL_SpiBurstWrite((U32 *)ulSrcAddr, (U32 *)pSrcByte, ulLenByte, FALSE);
            
            /*set dest memory to 0*/
            HAL_MemZero((U32 *)ulDesAddr, ulLenByte);
            
            /*Xfer*/
            HAL_SpiSetDmaeWR();
            while ( FALSE == HAL_DMAECopyOneBlock( ulDesAddr, ulSrcAddr, ulLenByte) )
            {
            }
            
            /*data check*/
            pDesByte= (U8 *)ulDesAddr;
            for ( i=0; i<TEST_DMAE_CHECK_NUM; i++ )
            {
                if ( (i+1) != *pDesByte )
                {                        
                    rTracer = 0x12345678;
                    DBG_Getch();
                }
                pDesByte += ulDataIncByte;
            }
            rTracer = 0x4000 + (ucDesType + 1) * 0x100 + ucLenIndex;                
            
            /*update address*/
            if ( TEST_DMAE_TYPE_DRAM == ucDesType )
            {
                ulDesAddr += ulLenByte;
            }
            
            ulSrcAddr += ulBlkLen;
        }//for ( ucLenIndex = 0; ucLenIndex < 9; ucLenIndex++ )
    }//for ( ucDesType = TEST_DMAE_TYPE_DRAM; ucDesType <= TEST_DMAE_TYPE_OTFB; ucDesType++ )
    
    rTracer = 0x88888888;
}
#endif

void TEST_DMAEDataXferNormal(void)
{
    U32 ulSrcAddr;
    U32 ulDesAddr;
    U32 ulLenByte;
    U8 ucSrcType;
    U8 ucDesType;
    U8 ucLenIndex;
    U32 i;
    U32 ulDataIncByte;
    U8 *pSrcByte;
    U8 *pDesByte;
    
    for ( ucSrcType = TEST_DMAE_TYPE_DRAM; ucSrcType <= TEST_DMAE_TYPE_OTFB; ucSrcType++ )
    {        
        for ( ucDesType = TEST_DMAE_TYPE_DRAM; ucDesType <= TEST_DMAE_TYPE_OTFB; ucDesType++ )
        {                            
            for ( ucLenIndex = 0; ucLenIndex < LEN_INDEX_NUM; ucLenIndex++ )
            {
                ulSrcAddr = l_aDmaeSrcAddr[ucSrcType];
                ulDesAddr = l_aDmaeDesAddr[ucDesType];
                ulLenByte = l_aDmaeTestLenByte[ucLenIndex];
            
                /*Init source data,set dest data to 0*/
                pSrcByte = (U8 *)ulSrcAddr;
                pDesByte = (U8 *)ulDesAddr;
                ulDataIncByte = ulLenByte/TEST_DMAE_CHECK_NUM;
                for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
                {
                    *pSrcByte = i + ucLenIndex + 1;
                    *pDesByte = 0;
                    pSrcByte += ulDataIncByte;
                    pDesByte += ulDataIncByte;
                }
                
                /*reset destination memory*/
                //HAL_MemSet((U32 *)ulDesAddr, ulLenByte/4, 0);

                HAL_DMAECopyOneBlock( ulDesAddr, ulSrcAddr, ulLenByte);

                /*Check dst data*/
                pDesByte = (U8 *)ulDesAddr;
                for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
                {
                    if ( (i + ucLenIndex + 1) != *pDesByte )
                    {                        
                        rTracer = 0xF0000000 + ucLenIndex;
                        DBG_Printf("TEST_DMAEDataXferNormal: 0x%x error\n", l_ulTimer);
                        DBG_Getch();
                    }
                    pDesByte += ulDataIncByte;
                }
            } //for ( ucLenIndex )
        }//for ( ucDstType )
    }//for ( ucSrcType )

    rTracer = 0xE0000000 + l_ulTimer;
    DBG_Printf("TEST_DMAEDataXferNormal: 0x%x OK\n", l_ulTimer);
    return;
}

void TEST_DMAEDataSetNormal(const U32 ulValueSet)
{
    U32 ulDesAddr;
    U8  ucDesType;
    U8  ucLenIndex;
    U32 ulLenByte;
    U32 *pDesData;
    U32 ulDataIncDW;
    U32 i;

    for ( ucDesType = 0; ucDesType <= TEST_DMAE_TYPE_OTFB; ucDesType++ )
    { 
        ulDesAddr = l_aDmaeSrcAddr[ucDesType];

        for ( ucLenIndex = 0; ucLenIndex < LEN_INDEX_NUM; ucLenIndex++ )
        {                
            ulLenByte = l_aDmaeTestLenByte[ucLenIndex];
                
            /*reset destination memory*/
            //HAL_MemSet((U32 *)ulDesAddr, ulLenByte/4, 0);

            HAL_DMAESetValue(ulDesAddr,ulLenByte,ulValueSet);

            /*Data Check*/
            pDesData = (U32 *)ulDesAddr;
            ulDataIncDW = (ulLenByte/sizeof(U32))/TEST_DMAE_CHECK_NUM ;
            if ( ulLenByte >= TEST_DMAE_CHECK_NUM * sizeof(U32) )
            {
                for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
                {
                    if ( ulValueSet != *pDesData )
                    {                        
                        rTracer = 0xF1000000 + ucLenIndex;
                        DBG_Printf("TEST_DMAEDataSetNormal: 0x%x error\n", l_ulTimer);
                        DBG_Getch();
                    }
                    pDesData += ulDataIncDW;
                }
            }
            else
            {
                for ( i = 0; i < ulLenByte/sizeof(U32); i++ )
                {
                    if ( ulValueSet != *pDesData )
                    {                        
                        rTracer = 0xF1000000 + ucLenIndex;
                        DBG_Printf("TEST_DMAEDataSetNormal: 0x%x error\n", l_ulTimer);
                        DBG_Getch();
                    }
                    pDesData += 1;
                }
            }
               
            ulDesAddr += ulLenByte;
        } //for ( ucLenIndex )
    }//for ( ucDstType )

    rTracer = 0xE1000000 + l_ulTimer;
    DBG_Printf("TEST_DMAEDataSetNormal: 0x%x OK\n", l_ulTimer);
    return;
}

void TEST_DMAEDataXferSram(void)
{
    U32 ulSrcAddr;
    U32 ulDesAddr;
    U32 ulLenByte;
    U8 ucSrcType;
    U8 ucDesType;
    U8 ucLenIndex;
    U32 i;
    U32 ulDataIncByte;
    U8 *pSrcByte;
    U8 *pDesByte;
    
    for ( ucSrcType = TEST_DMAE_TYPE_DRAM; ucSrcType <= TEST_DMAE_TYPE_MCUSRAM; ucSrcType++ )
    {        
        for ( ucDesType = TEST_DMAE_TYPE_DRAM; ucDesType <= TEST_DMAE_TYPE_MCUSRAM; ucDesType++ )
        {                            
            for ( ucLenIndex = LEN_INDEX_16B; ucLenIndex <= LEN_INDEX_16K; ucLenIndex++ )
            {
                if ( (TEST_DMAE_TYPE_MCUSRAM != ucSrcType) && (TEST_DMAE_TYPE_MCUSRAM != ucDesType) )
                {
                    continue;
                }

                if ( (TEST_DMAE_TYPE_MCUSRAM == ucSrcType) && (TEST_DMAE_TYPE_MCUSRAM == ucDesType) )
                {
                    continue;
                }

                ulSrcAddr = l_aDmaeSrcAddr[ucSrcType];
                ulDesAddr = l_aDmaeDesAddr[ucDesType];
                ulLenByte = l_aDmaeTestLenByte[ucLenIndex];
            
                /*Init source data,set dest data to 0*/
                pSrcByte = (U8 *)ulSrcAddr;
                pDesByte = (U8 *)ulDesAddr;
                ulDataIncByte = ulLenByte/TEST_DMAE_CHECK_NUM;
                for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
                {
                    *pSrcByte = i + ucLenIndex + 1;
                    *pDesByte = 0;//clear dest data
                    pSrcByte += ulDataIncByte;
                    pDesByte += ulDataIncByte;
                }
                
                /*reset destination memory*/
                //HAL_MemSet((U32 *)ulDesAddr, ulLenByte/4, 0);

                HAL_DMAECopyOneBlock( ulDesAddr, ulSrcAddr, ulLenByte);

                /*Check dst data*/
                pDesByte = (U8 *)ulDesAddr;
                for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
                {
                    if ( (i + ucLenIndex + 1) != *pDesByte )
                    {                        
                        rTracer = 0xF2000000 + ucLenIndex;
                        DBG_Printf("TEST_DMAEDataXferSram: 0x%x error\n", l_ulTimer);
                        DBG_Getch();
                    }
                    pDesByte += ulDataIncByte;
                }
            } //for ( ucLenIndex )
        }//for ( ucDstType )
    }//for ( ucSrcType )

    rTracer = 0xE2000000 + l_ulTimer;
    DBG_Printf("TEST_DMAEDataXferSram: 0x%x OK\n", l_ulTimer);
    return;


}

void TEST_DMAEDataSetSram(const U32 ulValueSet)
{
    U32 ulDesAddr;
    U8  ucLenIndex;
    U32 ulLenByte;
    U32 *pDesData;
    U32 ulDataIncDW;
    U32 i;

    ulDesAddr = l_aDmaeSrcAddr[TEST_DMAE_TYPE_MCUSRAM];

    for ( ucLenIndex = LEN_INDEX_16B; ucLenIndex <= LEN_INDEX_16K; ucLenIndex++ )
    {                
        ulLenByte = l_aDmaeTestLenByte[ucLenIndex];
                
        /*reset destination memory*/
        //HAL_MemSet((U32 *)ulDesAddr, ulLenByte/4, 0);

        HAL_DMAESetValue(ulDesAddr,ulLenByte,ulValueSet);

        /*Data Check*/
        pDesData = (U32 *)ulDesAddr;
        ulDataIncDW = (ulLenByte/sizeof(U32))/TEST_DMAE_CHECK_NUM ;
        if ( ulLenByte >= TEST_DMAE_CHECK_NUM * sizeof(U32) )
        {
            for ( i = 0; i < TEST_DMAE_CHECK_NUM; i++ )
            {
                if ( ulValueSet != *pDesData )
                {                        
                    rTracer = 0xF3000000 + ucLenIndex;
                    DBG_Printf("TEST_DMAEDataSetSram: 0x%x error\n", l_ulTimer);
                    DBG_Getch();
                }
                pDesData += ulDataIncDW;
            }
        }
        else
        {
            for ( i = 0; i < ulLenByte/sizeof(U32); i++ )
            {
                if ( ulValueSet != *pDesData )
                {                        
                    rTracer = 0xF3000000 + ucLenIndex;
                    DBG_Printf("TEST_DMAEDataSetSram: 0x%x error\n", l_ulTimer);
                    DBG_Getch();
                }
                pDesData += 1;
            }
        }
               
        ulDesAddr += ulLenByte;
    } //for ( ucLenIndex )

    rTracer = 0xE3000000 + l_ulTimer;
    DBG_Printf("TEST_DMAEDataSetSram: 0x%x OK\n", l_ulTimer);
    return;
}


void TEST_DMAEMain(void)
{
#if defined(COSIM) //only COSIM ENV need FW code to initialize DDR
    HAL_DramcInit();
#endif

    TEST_DMAEInit();

    while (1)
    {
#ifdef TEST_DRAM_OTFB_DATAXFR
        TEST_DMAEDataXferNormal();
#endif

#ifdef TEST_DRAM_OTFB_SETVALUE
        TEST_DMAEDataSetNormal(0);
        TEST_DMAEDataSetNormal(INVALID_8F);
        TEST_DMAEDataSetNormal(l_ulTimer + 1);
#endif

#ifdef TEST_MCU_SRAM_DATAXFR
        TEST_DMAEDataXferSram();
#endif

#ifdef TEST_MCU_SRAM_SETVALUE
        TEST_DMAEDataSetSram(0);
        TEST_DMAEDataSetSram(INVALID_8F);
        TEST_DMAEDataSetSram(l_ulTimer + 1);
#endif

        l_ulTimer++;
    }
}

LOCAL BOOL DMAE_TEXT_ATTR HAL_DMAEHeadCopyOneBlockLenLimit(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte, const U32 ulMCUID)
{
    U8 ucCurCmdID;
    volatile DMAE_CMDENTRY *pCurCmdEntry;
    U8 ucDesAreaType;
    U8 ucSrcAreaType;
    U32 ulCmdDesAddr;
    U32 ulCmdSrcAddr;
    U8 ucMCUSel;

    ucCurCmdID = HAL_DMAEGetNextCmdID();
    if (INVALID_2F == ucCurCmdID)
    {
        return FALSE;
    }

    pCurCmdEntry = (volatile DMAE_CMDENTRY *)(DMAE_CMDENTRY_BASE + ucCurCmdID * sizeof(DMAE_CMDENTRY));

    if ((FALSE == HAL_DMAEParseAddress(&ucDesAreaType, &ulCmdDesAddr, ulDesAddr))
        || (FALSE == HAL_DMAEParseAddress(&ucSrcAreaType, &ulCmdSrcAddr, ulSrcAddr)))
    {
        return FALSE;
    }

    pCurCmdEntry->bTrigger = 0;

    pCurCmdEntry->bsDesType = ucDesAreaType;
    pCurCmdEntry->ulDesAddr = ulCmdDesAddr;
    pCurCmdEntry->bsSrcType = ucSrcAreaType;
    pCurCmdEntry->ulSrcAddr = ulCmdSrcAddr;
    pCurCmdEntry->bsLength = HAL_DMAEGetCmdLen(ulBlockLenInByte);

    switch (ulMCUID)
    {
    case MCU0_ID:
        ucMCUSel = DMAE_SEL_MCU0;
        break;

    case MCU1_ID:
        ucMCUSel = DMAE_SEL_MCU1;
        break;

    case MCU2_ID:
        ucMCUSel = DMAE_SEL_MCU2;
        break;

    default:
        DBG_Printf("HAL_DMAESelMCU: get MCU ID %d error", ulMCUID);
        DBG_Getch();
        break;
    }

    pCurCmdEntry->bsMCUSel = ucMCUSel;

    pCurCmdEntry->bTrigger = 1;
#ifdef SIM
    pCurCmdEntry->bsStatus = DMAE_CMDENTRY_STATUS_PENDING;
    DMAE_ModelProcessCmdEntry(ucCurCmdID);
#endif

    while (FALSE == HAL_DMAEIsCmdDone(ucCurCmdID))
    {
    }

    return TRUE;
}

void DMAE_TEXT_ATTR HAL_DMAEHeadCopyOneBlock(const U32 ulDesAddr, const U32 ulSrcAddr, const U32 ulBlockLenInByte, const U32 ulMCUID)
{
    U32 ulCurCopyLen;
    U32 ulCurDesAddr = ulDesAddr;
    U32 ulCurSrcAddr = ulSrcAddr;
    U32 ulRemainLen = ulBlockLenInByte;

    while (ulRemainLen > 0)
    {
        if ( ulRemainLen >= DMAE_ENTRY_MAX_LENGTH_IN_BYTE)
        {
            ulCurCopyLen = DMAE_ENTRY_MAX_LENGTH_IN_BYTE;
        }
        else
        {
            ulCurCopyLen = ulRemainLen;
        }

        while (FALSE == HAL_DMAEHeadCopyOneBlockLenLimit(ulCurDesAddr, ulCurSrcAddr, ulCurCopyLen, ulMCUID))
        {
        }

        ulRemainLen -= ulCurCopyLen;
        ulCurDesAddr += ulCurCopyLen;
        ulCurSrcAddr += ulCurCopyLen;
    }

    return;
}

