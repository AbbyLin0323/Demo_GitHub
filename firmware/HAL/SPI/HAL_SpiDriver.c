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
Filename     :  HAL_SpiDriver.c                                          
Version      :  0.3                                               
Date         :                                            
Author       : 

Description:  
        SPI NOR FLASH Driver
Others: 
Modification History:
20140208    Victor Zhang : Create
20141115    Victor Zhang : Reconstruct the code
*******************************************************************************/

#include "BaseDef.h"
#include "HAL_MemoryMap.h"
#include "HAL_SpiDriver.h"
#include "HAL_SpiChipDefine.h"
#include "HAL_Dmae.h"


LOCAL volatile SPI_REG_SET* const l_pSpiRegSet = (volatile SPI_REG_SET*)(REG_BASE_SPI);

/******************************************************************************
FUNC: HAL_SpiSendCCmd
Input:  ucCmdType  -- control command code 
        ucRespByteNum   -- expect output data length ,count in byte
        ucCContByteNum  -- input data length , count in byte
Output: 
Description:
    Trigger SNFC to issue control command (All commands but write/read data command )

History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
LOCAL void DMAE_TEXT_ATTR HAL_SpiSendCCmd(U8 ucCmdType,U8 ucRespByteNum,U8 ucCContByteNum)
{
    l_pSpiRegSet->bsCCmdCode      = ucCmdType;    
    l_pSpiRegSet->bsCRespNum      = ucRespByteNum;
    l_pSpiRegSet->bsCLength       = ucCContByteNum;
    l_pSpiRegSet->bsAutoStp       = (0 != ucRespByteNum) ? TRUE : FALSE;
    l_pSpiRegSet->bsCCmdValid     = TRUE;
    while(FALSE == l_pSpiRegSet->bsCRespValid)
    {
        ;
    }
    l_pSpiRegSet->bsCRespValid    = FALSE;
    l_pSpiRegSet->bsCCmdValid     = FALSE;
}
/******************************************************************************
FUNC: HAL_SpiReadStatus
Input:  U8 Read status register command code

Output:
Description:
    1. Read SPI Flash status register
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
GLOBAL U8 DMAE_TEXT_ATTR HAL_SpiReadStatus(U8 ulRSRCode)
{
    HAL_SpiSendCCmd(ulRSRCode,1,0);
    return (l_pSpiRegSet->ucCRespByte[0]);
}

/******************************************************************************
FUNC: HAL_SpiGetWIP
Input:  
Output: TRUE for Write in processing , FALSE for idle 
Description:
    1. Get Busy Bit of SPI status register
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL BOOL DMAE_TEXT_ATTR HAL_SpiGetWIP(void)
{
    SPI_STATUS_REG tReg; 
    tReg.ucValue = HAL_SpiReadStatus(SPI_CMD_RDSR1);
    return (1 == tReg.bsWIP) ? TRUE : FALSE;     
}
/******************************************************************************
FUNC: HAL_SpiGetWEL
Input:  
Output: TRUE for Write enabled latch , FALSE for disabled  
Description:
    1. Get write enable latch Bit of SPI status register
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL BOOL DMAE_TEXT_ATTR HAL_SpiGetWEL(void)
{
    SPI_STATUS_REG tReg; 
    tReg.ucValue = HAL_SpiReadStatus(SPI_CMD_RDSR1);
    return (1 == tReg.bsWEL) ? TRUE : FALSE;   
}
/******************************************************************************
FUNC: HAL_SpiWriteEnable
Input:  
Output: 
Description:
    1. Issue req to make SPI flash to enable write 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void DMAE_TEXT_ATTR HAL_SpiWriteEnable(void)
{
    HAL_SpiSendCCmd(SPI_CMD_WREN,0,0);    
}


/******************************************************************************
FUNC: HAL_SpiHWInit
Input:  
Output: 
Description:
    1. SPI register block initiate 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void DMAE_TEXT_ATTR HAL_SpiHWInit(void)
{
    COM_MemZero((U32 *) l_pSpiRegSet,sizeof(SPI_REG_SET)/sizeof(U32));  // reset first 7 dword
    l_pSpiRegSet->bsRCmdCode  = SPI_CMD_READ;
    l_pSpiRegSet->bsWCmdCode  = SPI_CMD_PP;
    l_pSpiRegSet->bsSnf       = TRUE;
    l_pSpiRegSet->bsRCmdEn    = TRUE;   
}

/******************************************************************************
FUNC: HAL_SpiWaitRstFsh
Input:  
Output: 
Description:
    1. Delay for reset command complete
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

LOCAL void DMAE_TEXT_ATTR HAL_SpiWaitRstFsh(void)
{
    //Delay_us(30);
#ifndef FPGA
    HAL_DelayCycle(8000);   // 266M
#else
    HAL_DelayCycle(1200);    // 40M
#endif
}

/******************************************************************************
FUNC: HAL_SpiSoftReset
Input:
Output:
Description:
    1. Issue reset enable
    2. Issue reset
    3. Delay for reset complete
    4. Reset spi register block
    5. Select default write command : page program SPI
    6: Select default read command : normal read SPI
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
void DMAE_TEXT_ATTR HAL_SpiSoftReset(void)
{
    HAL_SpiSendCCmd(SPI_CMD_RSTEN,0,0);
    HAL_SpiSendCCmd(SPI_CMD_RST,0,0);  
    HAL_SpiWaitRstFsh();
}


/******************************************************************************
FUNC: HAL_SpiPageWrite
Input:  ulDst  --  destination address (in SPI)
        ulSrc  --  source address 
        ulLenBLimit --  data request length ,count in byte ,Max value is 256
Output: SUCCESS / FAIL
Description:
       Write data which length equals or be less than 1 page (256) from memory to SPI.
       The length must be 16 byte aligned for DMAE module       
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL U32 DMAE_TEXT_ATTR HAL_SpiPageWrite(U32 ulDst,U32 ulSrc,U32 ulLenBLimit)
{
    U32 ulRem = ulDst & SPI_PAGE_SIZE_MASK ;
    if((0 == ulLenBLimit)||((ulLenBLimit + ulRem) > SPI_PAGE_SIZE))
    {
        return FAIL;
    }

    HAL_SpiWriteEnable();
    while(FALSE == HAL_SpiGetWEL())
    {
        ;
    }

    l_pSpiRegSet->bsWCmdEn = TRUE;
    HAL_DMAECopyOneBlock((const U32)ulDst,(const U32)ulSrc,ulLenBLimit);
    l_pSpiRegSet->bsWCmdEn = FALSE;

    while(TRUE == HAL_SpiGetWIP())
    {
        ;
    }

    return SUCCESS;    
}

/******************************************************************************
FUNC: HAL_SpiErase
Input:
Output:
Description:
    1. Three type of SPI erase be offerred.  Set ulType as bellow
        SPI_ERASE_4K     = 0
        SPI_ERASE_32K   = 1
        SPI_ERAES_64K   = 2
        
    2. ulAddress is the Des Address in SPI where will be erased 

    3. if type is 4k/32k/64k ,then ulAddress should be 4k/32k/64k aligned.
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
void DMAE_TEXT_ATTR HAL_SpiErase(U32 ulAddress,U32 ulType)
{
    U8 ucCmdType;    
    U8 ucCContByteLen;
    U8 aEraseCmdCode[] = {SPI_CMD_SE,SPI_CMD_BE_32K,SPI_CMD_BE_64K};
    U32 ulOffset = ulAddress - SPI_START_ADDRESS;
    
    if (0 != (ulOffset & SPI_SEC_SIZE_MASK))
    {
        DBG_Printf("SPI Erase failed.\n ");
        return ;
    }

    ucCContByteLen = 3;
    if (ulType >= sizeof(aEraseCmdCode))
    {
        DBG_Printf("Wrong SPI ERASE CODE.\n");
    }
    else
    {
        ucCmdType = aEraseCmdCode[ulType];
    }
    HAL_SpiWriteEnable();
    while(FALSE == HAL_SpiGetWEL())
    {
        ;
    }
    
    l_pSpiRegSet->ulAddress = ulOffset;
    HAL_SpiSendCCmd(ucCmdType,0,ucCContByteLen);
    
    while(TRUE == HAL_SpiGetWIP())
    {
        ;
    }
    return ;    
}

/******************************************************************************
FUNC: HAL_SpiDmaWriteDataBlock
Input:
Output:
Description:
    1.Write data which length is multiple pages (256 byte per page) from memory to spi.
    2.Des address should be page aligned.
    3.A erase opration should be processed before.
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
void DMAE_TEXT_ATTR HAL_SpiWriteDataBlock(U32 ulDesAddr,U32 ulSrcAddr,U32 ulDataLen)
{
    U32 i ,len;
    U32 ulDataRemainLen = ulDataLen;
    U32 ulDesCurrAddr,ulSrcCurrAddr;
    ulDesCurrAddr = ulDesAddr;
    ulSrcCurrAddr = ulSrcAddr;
    while(ulDataRemainLen>0)
    {
        if(ulDataRemainLen>SPI_PAGE_SIZE)
        {
            len = SPI_PAGE_SIZE;
        }
        else
        {
            len = ulDataRemainLen;
        }
        
        HAL_SpiPageWrite(ulDesCurrAddr,ulSrcCurrAddr,len);
        ulDesCurrAddr += SPI_PAGE_SIZE;
        ulSrcCurrAddr += SPI_PAGE_SIZE;
        ulDataRemainLen -= len;
    }    
}
/******************************************************************************
FUNC: HAL_SpiDmaRead
Input:  
        ulDst  --  destination address
        ulSrc  --  source address (in SPI)
        ulLenB --  data request length ,count in byte . recommand 4K or less than 4k xfer length per time.
Output: 
Description:
    1. DMA read from SPI 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/

GLOBAL void DMAE_TEXT_ATTR HAL_SpiDmaRead(U32 ulDst,U32 ulSrc,U32 ulLenB)
{
    HAL_DMAECopyOneBlock((const U32)ulDst,(const U32)ulSrc,ulLenB);    
}
/******************************************************************************
FUNC: HAL_SpiDmaWrite4K
Input:
Output:
Description:
    Write 4K data from memory to SPI 
    1.Des address (in SPI flash) should be 4K aligned
    2.Src address should not be in SPI flash.
    3.Transfer data length is 4k 
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
GLOBAL void DMAE_TEXT_ATTR HAL_SpiDmaWrite(U32 ulDesAddr,U32 ulSrcAddr)
{       
    U32 ulLen = 4<<10;    
    // Check address align
    if ((SPI_START_ADDRESS > ulDesAddr)||(ulDesAddr + ulLen > SPI_FLASH_TRACELOG_END))
    {
        DBG_Printf("SPI flash write Des Address is invalid ,the value is 0x%x\n",ulDesAddr);
        DBG_Getch();
    }

    if (0 != (ulDesAddr % ulLen))
    {
        DBG_Printf("SPI flash write address not align!!\n");
        DBG_Getch();
    }
    // Erase sector or block
    
    HAL_SpiErase(ulDesAddr ,SPI_ERASE_4K);

    // Write 
    HAL_SpiWriteDataBlock(ulDesAddr,ulSrcAddr,ulLen);
}
/******************************************************************************
FUNC: TEST_SPI
Input:
Output:
Description:
    For SPI test
    
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
void INLINE TEST_SPI(void)
{
    U32 sum=0,i;
    U32 ulSrcAddr,ulDesAddr,ulOffset,ulLen;

    // Prepare DATA
    
    ulSrcAddr = 0xfff04000;
    for (i=0;i<(4<<8);i++)
    {
        *(volatile U32*)(ulSrcAddr + i*4) = 0x12340001+i;
        sum ^= 0x12341001+i;
    }

    ulDesAddr = SPI_START_ADDRESS;    // offset in SPI 
    HAL_SpiErase(ulDesAddr,SPI_ERASE_4K);
    DBG_Printf("Erase done.\n");
    
    ulDesAddr = SPI_START_ADDRESS;
    
    // Write 4K from OTFB to SPI
    HAL_SpiDmaWrite(ulDesAddr,ulSrcAddr);
    DBG_Printf("SPI Write done.\n");

    // Read 4k from SPI to DRAM
    ulDesAddr = 0x40000000;             // DRAM
    ulSrcAddr = SPI_START_ADDRESS;      // SPI
    ulLen = 0x1000;  // 4k
    HAL_SpiDmaRead(ulDesAddr,ulSrcAddr,ulLen);
    DBG_Printf("SPI Read done.\n");

    for (i=0;i<(4<<8);i++)
    {
        sum ^= *(volatile U32*)(0x40000000 + i*4);
    }

    if (sum == 0)
    {
        DBG_Printf("Data check Pass.\n");
        
    }
    else
    {
        DBG_Printf("Data check Fail.\n");
    }   
  
    while(1);
}

/******************************************************************************
FUNC: HAL_SpiInit
Input:
Output:
Description:
    1. Reset spi register block
    2. Soft reset
History:
    2014/11/17 Victor Zhang  reconstruct

*******************************************************************************/
void DMAE_TEXT_ATTR HAL_SpiInit(void)
{
    GLB_IO_ENABLE *pGLB68 = (GLB_IO_ENABLE *)(&rGLB(0x68));
    pGLB68->RSPI_NEW = 1;
    pGLB68->RNEWSPI_MODE = 2;
    pGLB68->RSPI_EN = 1;
    pGLB68->RSPI_DIS = 0;
    pGLB68->RSPI_CKG_EN = 0;

#ifdef DMAE_ENABLE
    HAL_DMAEInit();
#endif
    HAL_SpiHWInit();
    HAL_SpiSoftReset();    
    //TEST_SPI();
}


/************************ end of file ************************/

