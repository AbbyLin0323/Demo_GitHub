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
 * File Name    : TEST_SPI.c
 * Discription  : 
 * CreateAuthor : VictorZhang
 * CreateDate   : 2014-11-4
 *===============================================================================
 * Modify Record:
 *=============================================================================*/
#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_GlbReg.h"
#include "HAL_SpiDriver.h"
#include "HAL_SpiChipDefine.h"
#include "Proj_Config.h"

#ifdef XTMP
#define SPI_START_ADDRESS   (OTFB_START_ADDRESS + 0x20000)
#else
#define SPI_START_ADDRESS   (0xc0000000)
#endif

U32 TEST_GetSpiRCmdIndex(void)
{
#ifdef XTMP
    return 0;
#else
    return *(volatile U32*)(0x1ff812a0);
#endif
}

U32 TEST_GetSpiWCmdIndex(void)
{
#ifdef XTMP
    return 0;
#else
    return *(volatile U32*)(0x1ff812a4);
#endif
}

U32 TEST_GetSpiPageNum(void)
{
#ifdef XTMP
    return 4;
#else
    return *(volatile U32*)(0x1ff812a8);
#endif
}


void TEST_SpiDataPrepare(U32 ulPageNum)
{
    U32 ulSrcAddr = OTFB_START_ADDRESS;
    U32 i;

    for(i=0;i<64*ulPageNum;i++)   // 4 page
    {
        *(volatile U32*)(ulSrcAddr + i*sizeof(U32)) = 0xc0de0000 + i + 1;
    }
    DBG_Printf("BUF0 : 0x%x\n",*(volatile U32*)(ulSrcAddr));
    DBG_Printf("BUF1 : 0x%x\n",*(volatile U32*)(ulSrcAddr + SPI_PAGE_SIZE));
    DBG_Printf("BUF2 : 0x%x\n",*(volatile U32*)(ulSrcAddr + SPI_PAGE_SIZE*2));
    DBG_Printf("BUF3 : 0x%x\n",*(volatile U32*)(ulSrcAddr + SPI_PAGE_SIZE*3));
}


void TEST_SpiPattern(U32 ulPageNum)
{
    U32 ulDstAddr,ulSrcAddr,i;

    TEST_SpiDataPrepare(ulPageNum);
#ifndef XTMP
    HAL_SpiSetWCmd(TEST_GetSpiWCmdIndex());
#endif
    ulDstAddr = SPI_START_ADDRESS + SPI_BLK_SIZE;
    ulSrcAddr = OTFB_START_ADDRESS;
    DBG_Printf("W Src: 0x%x Dst: 0x%x\n",ulSrcAddr,ulDstAddr);
    for (i=0;i<ulPageNum;i++)
    {
#ifdef XTMP
        DBG_Printf("Write %d\n",i);
        HAL_DMAECopyOneBlock(ulDstAddr+i*SPI_PAGE_SIZE,ulSrcAddr+i*SPI_PAGE_SIZE,SPI_PAGE_SIZE);
#else
        HAL_SpiDmaPageWrite(ulDstAddr+i*SPI_PAGE_SIZE,ulSrcAddr+i*SPI_PAGE_SIZE,SPI_PAGE_SIZE);
#endif
    }

#ifndef XTMP
    HAL_SpiSetRCmd(TEST_GetSpiRCmdIndex());
#endif

    ulDstAddr = OTFB_START_ADDRESS + SPI_BLK_SIZE;
    ulSrcAddr = SPI_START_ADDRESS + SPI_BLK_SIZE;
    DBG_Printf("R Src: 0x%x Dst: 0x%x\n",ulSrcAddr,ulDstAddr);
    for (i=0;i<ulPageNum;i++)
    {
        DBG_Printf("Read %d\n",i);
#ifdef XTMP
        HAL_DMAECopyOneBlock(ulDstAddr+i*SPI_PAGE_SIZE,ulSrcAddr+i*SPI_PAGE_SIZE,SPI_PAGE_SIZE);
#else
        HAL_SpiDmaRead(ulDstAddr+i*SPI_PAGE_SIZE,ulSrcAddr+i*SPI_PAGE_SIZE,SPI_PAGE_SIZE);
#endif
    }
    for (i=0;i<64*ulPageNum;i++)
    {
        if(*(volatile U32*)(ulDstAddr + i*sizeof(U32)) != 0xc0de0000 + i + 1)
        {
            rTracer = 0x6002;
            rTracer = ulDstAddr + i*sizeof(U32);
            rTracer = *(volatile U32*)(ulDstAddr + i*sizeof(U32));
            rTracer = 0xc0de0000 + i + 1;
            DBG_Printf("Data check error: 0x%x,%d\n",ulDstAddr + i*sizeof(U32),i);
            while(1);
        }
    }
}

void TEST_SpiInit(void)
{
    HAL_DMAEInit();
#ifdef COSIM
    HAL_SpiInit();
#endif
}


void TEST_SpiErase(void)
{
    U32 i;
    for(i=0;i<4;i++)
    {
        HAL_SpiBlockErase(i << SPI_BLK_SIZE_BITS);
    }
}

/*
 * TEST SPI
 * */

#if 1
void TEST_SpiMain(void)
{
    U32 ulPageNum =  ((16<<10) + 0x200)/256;
    U32 i;
    U32 ulDstAddr = SPI_START_ADDRESS;
    U32 ulSrcAddr = OTFB_START_ADDRESS;

    HAL_SpiInit();

    HAL_SpiBlockErase(0);


    for (i=0;i<ulPageNum;i++)
    {
        HAL_SpiDmaPageWrite(ulDstAddr+i*SPI_PAGE_SIZE,ulSrcAddr+i*SPI_PAGE_SIZE,SPI_PAGE_SIZE);
    }

    HAL_SpiDmaRead (OTFB_START_ADDRESS + 0x10000,SPI_START_ADDRESS,16<<10);
}

#else
void TEST_SpiMain(void)
{
    GLB_IO_ENABLE *pGLB68 = (GLB_IO_ENABLE *)(0x1ff80000 + 0x68);
    U32 i;
    U32 ulBuf = 0;
    pGLB68->RSPI_NEW = 1;
    pGLB68->RNEWSPI_MODE = 2;
    pGLB68->RSPI_EN = 1;
    pGLB68->RSPI_DIS = 0;
    pGLB68->RSPI_CKG_EN = 0;

    TEST_SpiInit();
#ifdef COSIM
    TEST_SpiErase();
    rTracer = 0x8101;
#endif
    TEST_SpiPattern(TEST_GetSpiPageNum());


    ulBuf = HAL_SpiReadStatus(SPI_CMD_RDSR1);
    ((SPI_STATUS_REG*)&ulBuf)->bsRes = 0xf;
    ulBuf = ulBuf << 8;
    ulBuf |= 3;
    HAL_SpiWriteStatus(SPI_CMD_WRSR1,ulBuf,2);

    ulBuf = HAL_SpiReadStatus(SPI_CMD_RDSR1);
    if (((SPI_STATUS_REG*)&ulBuf)->bsRes != 0xf)
    {
        rTracer = 0x6100;
        while(1);
    }
    ulBuf = HAL_SpiReadStatus(SPI_CMD_RDSR2);
    if ((ulBuf & 7) != 3)
    {
        rTracer = 0x6101;
        while(1);
    }
    rTracer = 0x9000;
    while(1);
}
#endif

